//
//  statement_semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "statement_semantics.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "compiler.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "bitset.h"
#include "errors.h"

static bool ExpressionHasSideEffects(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(assign):
    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
    case AST_OP(postinc):
    case AST_OP(postdec):
    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(call):
    case AST_OP(inline_call):
    case AST_OP(asm):
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
      return true;
    case AST_OP(comma): {
      BinaryASTNode* n = (BinaryASTNode*)node;
      return ExpressionHasSideEffects(n->left) || ExpressionHasSideEffects(n->right);
    }
    case AST_OP(question): {
      BinaryASTNode* n = (BinaryASTNode*)node;
      BinaryASTNode* arms = (BinaryASTNode*)n->right;
      return ExpressionHasSideEffects(n->left) ||
             (arms != NULL &&
              (ExpressionHasSideEffects(arms->left) ||
               ExpressionHasSideEffects(arms->right)));
    }
    default:
      return false;
  }
}

static void AnalyzeExpressionStatement(ExpressionStatementASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);

  // warn_unused_result: a discarded call to a function so annotated.
  ASTNode* expr = node->expr;
  if (expr != NULL && expr->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)expr;
    if (call->left != NULL && call->left->op == AST_OP(identifier)) {
      Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
      if (callee != NULL && SymbolHasAttribute(callee, "warn_unused_result")) {
        SemanticWarning(expr, "unused-result",
                        "ignoring return value of '%s' declared with "
                        "warn_unused_result",
                        callee->name.value);
      }
    }
  }
  if (expr != NULL && expr->type != NULL && !TypeIsVoid(expr->type) &&
      !ExpressionHasSideEffects(expr)) {
    SemanticWarning(expr, "unused-value", "expression result unused");
  }
}

static bool AsmOutputHasAddress(ASTNode* node) {
  if (node == NULL || TypeIsConst(node->type) || TypeIsFunction(node->type) ||
      TypeIsArray(node->type)) {
    return false;
  }
  switch (node->op) {
    case AST_OP(identifier):
    case AST_OP(subscript):
    case AST_OP(contents):
    case AST_OP(dot):
    case AST_OP(arrow):
    case AST_OP(cast):
    case AST_OP(compound_literal):
      return true;
    default:
      return false;
  }
}

static bool IsAArch64RegisterClobber(const char* name) {
  if (name[0] == 'x' || name[0] == 'w' || name[0] == 'd' || name[0] == 's' ||
      name[0] == 'v') {
    char* end = NULL;
    long reg = strtol(name + 1, &end, 10);
    return end != name + 1 && *end == '\0' && reg >= 0 && reg <= 31;
  }
  return strcmp(name, "lr") == 0 || strcmp(name, "sp") == 0 ||
         strcmp(name, "xzr") == 0 || strcmp(name, "wzr") == 0;
}

static bool IsNumericRegisterClobber(const char* name, const char* prefix,
                                     long max_reg) {
  size_t prefix_len = strlen(prefix);
  if (strncmp(name, prefix, prefix_len) != 0) {
    return false;
  }
  char* end = NULL;
  long reg = strtol(name + prefix_len, &end, 10);
  return end != name + prefix_len && *end == '\0' && reg >= 0 && reg <= max_reg;
}

static const char* AsmTargetName(void) {
  if (compiler->target == NULL) {
    return "target";
  }
  return compiler->target->name.value;
}

static bool IsSupportedAsmConstraint(const char* constraint, bool is_output) {
  const char* target = AsmTargetName();
  if (strcmp(target, "6502") == 0) {
    bool saw_constraint = false;
    for (const char* p = constraint; *p != '\0'; p++) {
      switch (*p) {
        case '=':
        case '+':
        case '&':
          break;
        case 'r':
        case 'i':
        case 'g':
          saw_constraint = true;
          break;
        default:
          return false;
      }
    }
    if (is_output) {
      return saw_constraint && (constraint[0] == '=' || constraint[0] == '+');
    }
    return saw_constraint;
  }
  bool saw_constraint = false;
  for (const char* p = constraint; *p != '\0'; p++) {
    switch (*p) {
      case '=':
      case '+':
      case '&':
      case '%':
        break;
      case 'r':
      case 'm':
      case 'i':
      case 'g':
        saw_constraint = true;
        break;
      case 'w':
        if (strcmp(target, "aarch64") != 0) {
          return false;
        }
        saw_constraint = true;
        break;
      default:
        return false;
    }
  }
  if (is_output) {
    return saw_constraint && (constraint[0] == '=' || constraint[0] == '+');
  }
  return saw_constraint;
}

static bool IsSupportedAsmClobber(const char* name) {
  const char* target = AsmTargetName();
  if (strcmp(name, "memory") == 0 || strcmp(name, "cc") == 0) {
    return true;
  }
  if (strcmp(target, "aarch64") == 0) {
    return IsAArch64RegisterClobber(name);
  }
  if (strcmp(target, "arm") == 0) {
    return IsNumericRegisterClobber(name, "r", 15) || strcmp(name, "lr") == 0 ||
           strcmp(name, "sp") == 0 || strcmp(name, "pc") == 0;
  }
  if (strcmp(target, "riscv") == 0) {
    return IsNumericRegisterClobber(name, "x", 31) ||
           IsNumericRegisterClobber(name, "f", 31) ||
           strcmp(name, "ra") == 0 || strcmp(name, "sp") == 0 ||
           strcmp(name, "fp") == 0;
  }
  if (strcmp(target, "x86_64") == 0 || strcmp(target, "x86-64") == 0) {
    static const char* regs[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};
    for (size_t i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
      if (strcmp(name, regs[i]) == 0) {
        return true;
      }
    }
    return strncmp(name, "xmm", 3) == 0 && IsNumericRegisterClobber(name, "xmm", 15);
  }
  return strcmp(target, "6502") == 0;
}

typedef struct {
  String* label_name;
  ASTNode* label;
} AsmLabelFinder;

static void FindAsmLabel(ASTNode* node, void* data, int child_id,
                         VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  AsmLabelFinder* finder = data;
  if (finder->label != NULL) {
    return;
  }
  if (node->op == AST_OP(label)) {
    LabelASTNode* label = (LabelASTNode*)node;
    if (StringEqualString(finder->label_name, &label->name)) {
      finder->label = node;
    }
  }
}

static void AnalyzeAsmStatement(AsmASTNode* node) {
  if (node->outputs.length + node->inputs.length > 16) {
    SemanticError(&node->base, "Too many asm operands");
  }
  for (size_t i = 0; i < node->outputs.length; i++) {
    AsmOperand* operand = node->outputs.value.p[i];
    operand->expr = AnalyzeExpression(operand->expr);
    if (!AsmOutputHasAddress(operand->expr)) {
      SemanticError(operand->expr, "asm output operand must be an assignable lvalue");
    }
    if (!IsSupportedAsmConstraint(operand->constraint.value, true)) {
      SemanticError(&node->base, "Unsupported %s asm output constraint '%s'",
                    AsmTargetName(), operand->constraint.value);
    }
  }
  for (size_t i = 0; i < node->inputs.length; i++) {
    AsmOperand* operand = node->inputs.value.p[i];
    operand->expr = AnalyzeExpression(operand->expr);
    if (!IsSupportedAsmConstraint(operand->constraint.value, false)) {
      SemanticError(&node->base, "Unsupported %s asm input constraint '%s'",
                    AsmTargetName(), operand->constraint.value);
    }
  }
  for (size_t i = 0; i < node->clobbers.length; i++) {
    String* clobber = node->clobbers.value.p[i];
    if (!IsSupportedAsmClobber(clobber->value)) {
      SemanticError(&node->base, "Unsupported %s asm clobber '%s'",
                    AsmTargetName(), clobber->value);
    }
  }
  for (size_t i = 0; i < node->labels.length; i++) {
    String* label_name = node->labels.value.p[i];
    AsmLabelFinder finder = {.label_name = label_name, .label = NULL};
    ASTNodeVisit(compiler->current_function->info.function.body, FindAsmLabel, 0,
                 &finder);
    if (finder.label == NULL) {
      SemanticError(&node->base, "Undefined asm goto label '%s'",
                    label_name->value);
    } else {
      finder.label->flags |= kASTLabelUsed;
      ((LabelASTNode*)finder.label)->named = true;
      VectorAppend(&node->label_nodes, finder.label);
    }
  }
}

static void AnalyzeIfStatement(IfStatementASTNode* node) {
  node->cond = AnalyzeExpression(node->cond);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->if_part);
  AnalyzeStatement(node->else_part);
  SemanticCheckScalarType(node->cond);
}

static void AnalyzeWhileStatement(CombinedStatementASTNode* node) {
  node->cond = AnalyzeExpression(node->cond);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->cond);
}

static void AnalyzeDoStatement(CombinedStatementASTNode* node) {
  node->cond = AnalyzeExpression(node->cond);
  SemanticCheckScalarType(node->cond);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->cond);
}

// Function to compare case values for the qsort function.  This
// is passed pointers to the elements of the array (which are
// void* pointers in our case because it's a Vector).
static int CompareCaseValue(const void* case1, const void* case2) {
  CaseLabelASTNode* node1 = *(CaseLabelASTNode**)case1;
  CaseLabelASTNode* node2 = *(CaseLabelASTNode**)case2;

  // NOTE that we don't care what the actual value returned is, as long
  // as its sign is correct.
  return (int)(node1->value - node2->value);
}

typedef struct {
  SwitchStatementASTNode* switch_node;
  int switch_level;
} SwitchResolver;

// Check for duplicate case labels and defaults in statement.
// Also fill in the 'cases' vector in the switch statement AST node
// and the default_node if present.
static void ResolveSwitchStatement(ASTNode* node, void* data, int child_id,
                                   VisitorMode mode) {
  SwitchResolver* resolver = data;
  SwitchStatementASTNode* switch_node = resolver->switch_node;
  
  // We can't descend into nested switch statements.  If we are looking
  // at a switch, change the switch_level in the finder.
  if (node->op == AST_OP(switch)) {
    if (mode == kVisitPreChildren) {
      resolver->switch_level++;
    } else if (mode == kVisitPostChildren) {
      resolver->switch_level--;
    }
  }
  
  // If we are in a nested switch statememnt stop here.
  if (resolver->switch_level > 1) {
    return;
  }
  
  if (mode == kVisitPreChildren && node->op == AST_OP(case)) {
     CaseLabelASTNode* case_node = (CaseLabelASTNode*)node;

     if (case_node->expr == NULL) {
       // This is a default node.
       if (switch_node->default_node != NULL) {
         SemanticError(node, "Duplicate default in switch statement");
       }
       switch_node->default_node = case_node;
       return;
     }

     // Convert the case expression to the type of the switch controlling
     // expression.
     NormalConversion(case_node->expr, switch_node->expr->type);

     // Case labels need to be constant integer expressions.
     if (!EvaluateIntegerExpression(case_node->expr, &case_node->value)) {
       SemanticError(switch_node->expr,
                     "Case labels must be constant integral expressions");
     }
    // Calculate min, max and density.
     if (case_node->value < switch_node->min_case_value) {
       switch_node->min_case_value = case_node->value;
     }
     if (case_node->value > switch_node->max_case_value) {
       switch_node->max_case_value = case_node->value;
     }
     VectorAppend(&switch_node->cases, node);
   }
}

// Look at all the cases for the switch and determine what integer type
// we should use for the control expression.
static Type DetermineControlType(SwitchStatementASTNode* node) {
  size_t num_cases = node->cases.length;

  // Calculate the max bit width for all case constants.  This can be used
  // by the backend to optimize comparisons.
  for (size_t i = 0; i < num_cases; i++) {
    int64_t case_value = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
    if (case_value < 0) {
      case_value = -case_value;
    }
    if (case_value == 0) {
      continue;
    }
    int width = 8;
    if (case_value < (1LL << 8)) {
      width = 1;
    } else if (case_value < (1LL << 16)) {
      width = 2;
    } else if (case_value < (1LL << 32)) {
      width = 4;
    }
    if (width > node->max_case_width) {
      node->max_case_width = width;
    }
  }
  
  // Determine int type to which to convert control expression.  This is based
  // on the max width of the cases in the statement.
  Type control_type = kTypeInt;
  switch (node->max_case_width) {
    case 1:
      control_type = kTypeChar;
      break;
    case 2:
      if (compiler->int_size == 2) {
        control_type = kTypeInt;
      } else {
        control_type = kTypeShort;
      }
      break;
    case 4:
      if (compiler->long_size == 4) {
        control_type = kTypeLong;
      } else {
        control_type = kTypeInt;
      }
      break;
    case 8:
      if (compiler->long_size == 8) {
        control_type = kTypeLong;
      } else {
        control_type = kTypeLongLong;
      }
      break;

  }
  return control_type;
}


static void AnalyzeEnumSwitch(SwitchStatementASTNode* node, Type control_type) {
  BitSet enum_constants = {0};
  BitSet found_constants = {0};
  Enum* info = node->expr->type->info.enum_info;
  assert(info != NULL);
  for (size_t i = 0; i < info->constants.length; i++) {
    Symbol* ec = info->constants.value.p[i];
    BitSetInsert(&enum_constants, ec->value.ivalue);
  }
  
  // Go through all the cases and make sure they are in the enum_constants.
  size_t num_cases = node->cases.length;
  for (size_t i = 0; i < num_cases; i++) {
    int64_t case_value = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
     if (BitSetContains(&enum_constants, case_value)) {
       BitSetInsert(&found_constants, case_value);
     } else {
       SemanticWarning(&node->base, "switch",
                       "Case value %" PRId64 " is not valid for enumeration %s",
                       case_value, info->tag_name->value);
     }
  }
  
  // Now check that we have included all the constants as cases.
  Vector missing_constants = {0};
  for (size_t i = 0; i < info->constants.length; i++) {
    Symbol* ec = info->constants.value.p[i];
    if (!BitSetContains(&found_constants,  ec->value.ivalue)) {
      VectorAppend(&missing_constants, ec);
    }
  }
  if (missing_constants.length > 0) {
    const char* warning = node->default_node == NULL ? "switch" : "switch-enum";
    if (missing_constants.length > 4) {
      Symbol* ec = missing_constants.value.p[0];
      SemanticWarning(&node->base, warning,
                      "Enum constant %s and %" PRId64 " others are not present in switch statement",
                      ec->name.value, missing_constants.length - 1);

    } else {
      for (size_t i = 0; i < missing_constants.length; i++) {
        Symbol* ec = missing_constants.value.p[i];
        SemanticWarning(&node->base, warning,
                        "Enum constant %s is not present in switch statement",
                        ec->name.value);
      }
    }
  } else {
    if (node->default_node == NULL) {
      // All cases covered.
      node->all_cases_covered = true;
    }
  }

  // Convert expr to int.  The conversion is to signed or unsigned
  if (node->all_cases_positive) {
    SemanticConvertType(node->expr,
                        NewTypeRecordWithSize(control_type | kTypeUnsigned, kQualPlain), kConvertNormal);
  } else {
    SemanticConvertType(node->expr,
                        NewTypeRecordWithSize(control_type, kQualPlain), kConvertNormal);
  }
  
  VectorDestruct(&missing_constants);
  BitSetDestruct(&enum_constants);
  BitSetDestruct(&found_constants);
}
  
static void AnalyzeSwitchStatement(SwitchStatementASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->expr);

  if (!TypeIsIntegral(node->expr->type)) {
    SemanticError(node->expr, "Switch statements need an integer type");
    return;
  }
  
  SwitchResolver resolver = {
    .switch_node = node,
    .switch_level = 0,
  };
    
  // Visit the switch statement and all its children, collecting
  // case and defaults.
  ASTNodeVisit(&node->base, ResolveSwitchStatement, 0, &resolver);
  if (node->default_node == NULL) {
    SemanticWarning(&node->base, "switch-default",
                    "switch statement has no default label");
  }

  size_t num_cases = node->cases.length;
  
  bool negative_cases = false;
  for (size_t i = 0; i < num_cases; i++) {
    int64_t case_value = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
    if (case_value < 0) {
      negative_cases = true;
      break;
    }
  }
  node->all_cases_positive = !negative_cases;
  
  // Determine int type to which to convert control expression.  This is based
  // on the max width of the cases in the statement.
  Type control_type = DetermineControlType(node);
 
  // If not an enum convert to int.  We want to handle the conversion for
  // an enum differently.  If it has only positive cases we can convert to
  // unsigned int as that makes for better code generation (no sign extension).
  // This conversion is done in AnalyzeEnumSwitch.
  if (!TypeIsEnum(node->expr->type)) {
    // Convert expr to int.  The conversion is to signed or unsigned.
    if (TypeIsUnsigned(node->expr->type)) {
      SemanticConvertType(node->expr,
                          NewTypeRecordWithSize(control_type | kTypeUnsigned, kQualPlain), kConvertNormal);
    } else {
      SemanticConvertType(node->expr,
                          NewTypeRecordWithSize(control_type, kQualPlain), kConvertNormal);
    }
  }
  
  
  // Get an idea of the case density.
  // Density is mass/volume.  Let's say that the number of cases is the
  // masss and the distance between the min and max values is the volume.
  float mass = (node->cases.length + (node->default_node != NULL ? 1 : 0));
  float volume = node->max_case_value - node->min_case_value;
  if (volume != 0) {
    node->density = mass / volume;
  }

  // Now we sort the cases into ascending order.  We do this because it's better
  // for code generation.  We can do a branch table or a binary search if the
  // values are sorted.
  qsort(node->cases.value.p, node->cases.length, sizeof(void*), CompareCaseValue);

  // Check the case labels for duplicates. Since they are sorted we only need
  // to check for two adjacent values being the same.  This is faster than doing
  // an n^2 search for each value;
  for (size_t i = 0; i < num_cases - 1; i++) {
    int64_t case_value1 = ((CaseLabelASTNode*)(node->cases.value.p[i]))->value;
    int64_t case_value2 =
        ((CaseLabelASTNode*)(node->cases.value.p[i + 1]))->value;
    if (case_value1 == case_value2) {
      const char* filename;
      int lineno;
      int start, end;
      DecodeSourceLocation(
          ((CaseLabelASTNode*)node->cases.value.p[i])->expr->location, &filename,
          &lineno, &start, &end);
      SemanticError(((CaseLabelASTNode*)node->cases.value.p[i + 1])->expr,
                    "Duplicate case value %d; previous is at %s:%d",
                    case_value2, filename, lineno);
    }
  }
  
  // If the switch is on enumerated type we need to validate that the
  // cases are part of the enumeration and set the all_cases_covered
  // flag if we have all the valid enumeration constants.
  if (TypeIsEnum(node->expr->type)) {
    AnalyzeEnumSwitch(node, control_type);
  }
}

static void AnalyzeForStatement(ForStatementASTNode* node) {
  if (node->c1 != NULL) {
    if (node->c1->op == AST_OP(decl_list)) {
      // First "expression" might be a list of variable declaration statements.
      AnalyzeStatement(node->c1);
      DeclarationListASTNode* decls = (DeclarationListASTNode*)node->c1;
      for (size_t i = 0; i < decls->declarations->length; i++) {
        VariableDeclarationASTNode* vardecl =
            (VariableDeclarationASTNode*)decls->declarations->value.p[i];
        if (!StorageIs(vardecl->symbol->storage, STO(auto)) &&
            !StorageIs(vardecl->symbol->storage, STO(register)) &&
            vardecl->symbol->storage != STO(implicit)) {
          SemanticError((ASTNode*)vardecl,
                        "Only auto or register variables "
                        "allowed in a for statement "
                        "declaration");
        }
      }
    } else {
      node->c1 = AnalyzeExpression(node->c1);
    }
  }

  node->c2 = AnalyzeExpression(node->c2);
  if (node->c2 != NULL) {
    SemanticCheckScalarType(node->c2);
  }

  // Optional expression 3.
  node->c3 = AnalyzeExpression(node->c3);

  // Finally the statment.
  AnalyzeStatement(node->stmt);
}

static void AnalyzeCompoundStatement(CompoundStatementASTNode* node) {
  for (size_t i = 0; i < node->statements->length; i++) {
    AnalyzeStatement((ASTNode*)node->statements->value.p[i]);
  }
}

// Replace the CombinedStatementASTNode (a return statement) with a
// CompoundStatementASTNode containing arg assignemnts and a goto statement
// to a new label at the start of the function body,
static void AnalyzeTailRecursion(CombinedStatementASTNode* node, VectorASTNode* call) {
  static char tail_label_name[32];   // Unique name for label
  static int tail_label_num = 0;
  CompoundStatementASTNode* function_body =
        (CompoundStatementASTNode*)compiler->current_function->info.function.body;
  if (compiler->current_function->info.function.varargs ||
      compiler->current_function->info.function.unknown_args) {
    // Don't know the arguments, no tail recursion possible.
    return;
  }
  SourceLocation location = call->base.location;
  
  // Insert label as first statement in function body.
  snprintf(tail_label_name, sizeof(tail_label_name), "__tail_label_%d",
          tail_label_num++);
  LabelASTNode* label = (LabelASTNode*)NewLabelASTNode(tail_label_name,
                                                       NULL,
                                                       false,
                                                       location);
  CompoundASTNodeInsertStatement(function_body, (ASTNode*)label, 0);
   
  Vector* statements = NewVector();
  
  // Create assignment statements for all arguments to their formal args via
  // temporaries.
  size_t num_actual_args = call->children->length;
  TypeRecord* subtype = call->left->type;
  
  // Declare temporaries for all arguments and assign them from the call's
  // actual arguments.
  Vector* decls = NewVector();
  for (size_t i = 0; i < num_actual_args; i++) {
    ASTNode* actual = ASTNodeMove((ASTNode*)call->children->value.p[i]);
    Symbol* formal = (Symbol*)subtype->info.function.prototype.value.p[i];
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, formal->type);
  
    ASTNode* assignment = NewBinaryASTNode(AST_OP(assign),
                                            formal->type,
                                            location,
                                            NewIdentifierASTNode(temp,
                                                                 location),
                                            actual);
    ASTNode* decl = NewVariableDeclarationASTNode(temp, assignment, location);
    VectorAppend(decls, decl);
  }
  VectorAppend(statements, NewDeclarationListASTNode(decls, location));
  
  // Now assign all temporaries back to the formals.
  for (size_t i = 0; i < num_actual_args; i++) {
    VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)decls->value.p[i];
    ASTNode* temp = NewIdentifierASTNode(decl->symbol, location);
    Symbol* formal = (Symbol*)subtype->info.function.prototype.value.p[i];
    ASTNode* assignment = NewBinaryASTNode(AST_OP(assign),
                                           formal->type,
                                           location,
                                           NewIdentifierASTNode(formal,
                                                                location),
                                           temp);
    VectorAppend(statements, NewExpressionStatementASTNode(assignment,
                                                         location));
  }
  
  // Create goto statement to the label.
  ASTNode* goto_stmt = NewGotoStatementASTNode(NewString(label->name.value),
                                                   location);
  VectorAppend(statements, goto_stmt);
  ASTNode* result = NewCompoundStatementASTNode(statements, location);
  ASTNodeReplaceChild(node->base.parent, node->base.child_id, result, true);
  AnalyzeStatement(result);
}

static void AnalyzeReturnStatement(CombinedStatementASTNode* node) {
  ASTNode* return_value = node->cond;
  if (return_value != NULL && return_value->op == AST_OP(asm)) {
    // Extension: return asm("foo") is allowed
    // Set the type of the asm statement to the return type of this function.
    ASTNodeSetType(return_value, compiler->current_function->next);
    return;
  }
  return_value = AnalyzeExpression(return_value);

  // Check current function return type.
  if (TypeIsVoid(compiler->current_function->next)) {
    // C does not allow a return statement with a value in a void function.
    // Unless the value being returned is also void (from a function call)
    if (return_value != NULL) {
      if (!TypeIsVoid(return_value->type)) {
        SemanticError(return_value, "Cannot return a value from a void function");
      }
    }
  } else {
    // Function returns a value.
    if (return_value == NULL) {
      SemanticError((ASTNode*)node,
                    "Must return a value from a non-void function");
    } else {
      NormalConversion(return_value, compiler->current_function->next);
    }
  }
  
  // Returns a struct.  If this is a call node it might be subject
  // to RVO.
  if (compiler->optimize) {
    if (TypeIsStructOrUnion(compiler->current_function->next)) {
      if (return_value->op == AST_OP(call)) {
        return_value->flags |= kASTRvoCall;
      } else if (return_value->op == AST_OP(identifier)) {
        // Named RVO places the returned variable directly in the caller's
        // return slot and elides the struct copy.  This is only valid for a
        // local automatic variable -- the backend can allocate such a variable
        // in the return slot.  A static/global/extern variable has its own
        // fixed storage, and a function argument is passed elsewhere, so those
        // must be copied into the return slot explicitly.  Mirror the
        // localvar test in NewIRVariable.
        Symbol* sym = ((IdentifierASTNode*)return_value)->symbol;
        if (sym != NULL && sym->flags.is_local && !sym->flags.is_argument &&
            !sym->flags.is_temp && !StorageIs(sym->storage, STO(static))) {
          return_value->flags |= kASTNrvoMarker;
        }
      }
    }
  }
  
  // Check for tail recursion.  This is a direct call to the current
  // function.
  if (OptLevel2() && return_value != NULL &&
      return_value->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)return_value;
    if (call->left->op == AST_OP(identifier)) {
      IdentifierASTNode* id_node = (IdentifierASTNode*)call->left;
      if (id_node->symbol == compiler->current_function->info.function.symbol) {
        AnalyzeTailRecursion(node, call);
      }
    }
  }
}

static void AnalyzeCaseLabel(CaseLabelASTNode* node) {
  if (node->expr != NULL) {
    // A case with no expression is used for 'default'.
    node->expr = AnalyzeExpression(node->expr);
  }
  if (node->stmt != NULL) {
    AnalyzeStatement(node->stmt);
  }
  // Since we don't know the type of the switch controlling expressions here
  // we delay the analysis of the case label statements to the analysis of the
  // switch statement.
}

void AnalyzeVariableDeclaration(VariableDeclarationASTNode* node) {
  node->initializer = AnalyzeExpression(node->initializer);
  if (node->initializer != NULL) {
    NormalConversion(node->initializer, node->symbol->type);
  }
}

void AnalyzeDeclarationList(DeclarationListASTNode* node) {
  size_t num_decls = node->declarations->length;
  for (size_t i = 0; i < num_decls; i++) {
    AnalyzeStatement(node->declarations->value.p[i]);
  }
}

static void FindLabel(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  GotoStatementASTNode* goto_node = data;
  if (mode != kVisitPreChildren) {
    return;
  }
  if (goto_node->label != NULL) {
    // Already found, nothing to do.
    return;
  }
  if (node->op == AST_OP(label)) {
    LabelASTNode* label = (LabelASTNode*)node;
    if (StringEqualString(goto_node->label_name, &label->name)) {
      goto_node->label = node;
    }
  }
}

static bool ContainsVLA(ASTNode* node) {
  if (node->op != AST_OP(decl_list)) {
    return false;
  }
  DeclarationListASTNode* decl_list = (DeclarationListASTNode*)node;
  for (size_t j = 0; j < decl_list->declarations->length; j++) {
     VariableDeclarationASTNode* decl = decl_list->declarations->value.p[j];
     if (TypeIsVLA(decl->base.type)) {
       return true;
     }
  }
  return false;
}

static void GetVLAs(DeclarationListASTNode* decl_list, Vector* vlas) {
  for (size_t i = 0; i < decl_list->declarations->length; i++) {
     VariableDeclarationASTNode* decl = decl_list->declarations->value.p[i];
     if (TypeIsVLA(decl->base.type)) {
       VectorAppend(vlas, decl);
     }
  }
}

static void ReportJumpError(ASTNode* jump, ASTNode* label,
                            Vector* bypassed_vlas) {
  SemanticError(jump, "Goto cannot jump to this label as it "
                "would bypass a variable length array definition");
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(label->location, &filename, &lineno, &start, &end);
  ReportNote(filename, lineno, "Label is here");
  
  for (size_t i = 0; i < bypassed_vlas->length; i++) {
    VariableDeclarationASTNode* decl = bypassed_vlas->value.p[i];
    DecodeSourceLocation(decl->base.location, &filename, &lineno, &start, &end);
    ReportNote(filename, lineno,
               "Variable length array '%s' is bypassed",
               decl->symbol->name.value);
  }
}

// Check a goto.
//
// Find the Lowest Common Ancestor (LCA) of the label and the jump.
// This is the AST closest AST node that has both as descendants.
//
// Build two vectors of AST nodes, one from the label to the
// and one from the jump to the root.
//
// At some point there will be a sequence of AST nodes that are the
// same.  The first of these is a Lowest Common Ancestor.  That's
// where the two tree branches converge.  This will be a compound
// statement and both branches will emerge from there.
//
// We need to verify that the jump from the goto to the label doesn't
// go past the definition of a variable length array because those
// decrement the stack pointer when they are allocated.
//
// In the LCA we go through the statements in the block starting at
// the branch containing the jump until we reach the branch containing
// the label.  If we see any VLA definitions between the two we have jumped
// over the VLA definition.
//
// If we reach the end of the block there isn't a branch to the label after
// the goto so this is a backward branch.  They can't jump over a VLA.
//
// We then need to look inside the branch
// containing the label.  In each AST node, if it's a compound statement
// we look for VLA decls before we reach the label's branch.


static void CheckGoto(ASTNode* label, ASTNode* jump, GotoStatementASTNode* g) {
  Vector label_path = {0};
  Vector jump_path = {0};
  ASTNode* node = label;
  while (node != NULL) {
    VectorAppend(&label_path, node);
    node = node->parent;
  }
  node = jump;
  while (node != NULL) {
    VectorAppend(&jump_path, node);
    node = node->parent;
  }
#if 0
  // Debugging, enable to see vectors.
  printf("label: ");
  for (size_t i = 0; i < label_path.length; i++) {
    printf("%d ", ((ASTNode*)label_path.value.p[i])->id);
  }
  printf("\njump: ");
  for (size_t i = 0; i < jump_path.length; i++) {
    printf("%d ", ((ASTNode*)jump_path.value.p[i])->id);
  }
  printf("\n");
#endif
  ASTNode* lca = NULL;
  // Go from end of the vectors (root of tree) and find the last common
  // node.
  int label_index = (int)label_path.length - 1;
  int jump_index = (int)jump_path.length - 1;

  // Find Lowest Common Ancestor of jump and label.
  while (label_index >= 0 && jump_index >= 0) {
    if (label_path.value.p[label_index] != jump_path.value.p[jump_index]) {
      lca = label_path.value.p[label_index+1];
      break;
    }
    label_index--;
    jump_index--;
  }
  assert(lca != NULL);
  g->lca = lca;
  
  // We have the LCA.  Start from the jump's branch forward.
  ASTNode* jump_branch = jump_path.value.p[jump_index];
  ASTNode* label_branch = label_path.value.p[label_index];
  assert(lca->op == AST_OP(compound));
  
  // Look in LCA.
  CompoundStatementASTNode* c = (CompoundStatementASTNode*)lca;
  Vector vlas = {0};
  for (size_t i = jump_branch->child_id + 1; i < label_branch->child_id; i++) {
    ASTNode* stmt = c->statements->value.p[i];
    if (ContainsVLA(stmt)) {
      GetVLAs((DeclarationListASTNode*)stmt, &vlas);
    }
  }
  // Now we look down the tree at all blocks parenting the label's
  // block.  In each one we traverse the statments until we find the
  // branch for the next label index and look for a VLA.
  for (size_t i = label_index; i > 0; i--) {
    ASTNode* block = label_path.value.p[i];
    label_branch = label_path.value.p[i-1];
    if (block->op == AST_OP(compound)) {
      CompoundStatementASTNode* c = (CompoundStatementASTNode*)block;
      for (size_t j = 0; j < label_branch->child_id; j++) {
        ASTNode* stmt = c->statements->value.p[j];
        if (ContainsVLA(stmt)) {
          GetVLAs((DeclarationListASTNode*)stmt, &vlas);
        }
      }
    }
  }
  
  if (vlas.length != 0) {
    ReportJumpError(jump, label, &vlas);
  }

  VectorDestruct(&vlas);
  VectorDestruct(&label_path);
  VectorDestruct(&jump_path);
}


void AnalyzeGotoStatement(GotoStatementASTNode* node) {
  // Look for label matching the goto.
  // If we find it, set the 'label' field of the node to point to it.  This
  // does not own the label node.
  // In C, labels are global to the whole function.
  ASTNodeVisit(compiler->current_function->info.function.body, FindLabel, 0, node);

  if (node->label == NULL) {
    SemanticError((ASTNode*)node, "Undefined label '%s'", node->label_name->value);
  } else {
    node->label->flags |= kASTLabelUsed;
    CheckGoto(node->label, &node->base, node);
  }
}


typedef struct {
  String* label_name;
  bool found;
} DuplicateLabelFinder;

static void FindDuplicateLabel(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  DuplicateLabelFinder* finder = data;

  if (mode != kVisitPreChildren) {
    return;
  }
  if (node->op == AST_OP(label)) {
    LabelASTNode* label = (LabelASTNode*)node;
    if (StringEqualString(finder->label_name, &label->name)) {
      if (finder->found) {
        SemanticError(&label->base, "Duplicate label '%s'",
                      finder->label_name->value);
      }
      finder->found = true;
    }
  }
}

void AnalyzeLabel(LabelASTNode* node) {
  DuplicateLabelFinder finder = {
    .label_name = &node->name,
    .found = false,
  };
  
  ASTNodeVisit(compiler->current_function->info.function.body,
               FindDuplicateLabel, 0, &finder);
  if (node->stmt != NULL) {
    AnalyzeStatement(node->stmt);
  }
}

static void FindUnusedLabel(ASTNode* node, void* data, int child_id,
                            VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node->op != AST_OP(label)) {
    return;
  }
  LabelASTNode* label = (LabelASTNode*)node;
  if (!label->named && (node->flags & kASTLabelUsed) == 0) {
    SemanticWarning(node, "unused-label", "label '%s' defined but not used",
                    label->name.value);
  }
}

void CheckUnusedLabels(ASTNode* body) {
  ASTNodeVisit(body, FindUnusedLabel, 0, NULL);
}

void AnalyzeStatement(ASTNode* node) {
  if (node == NULL || (node->flags & kASTAnalyzed) != 0) {
    return;
  }

  switch (node->op) {
    case AST_OP(decl_list):
      AnalyzeDeclarationList((DeclarationListASTNode*)node);
      break;
    case AST_OP(vardecl):
      AnalyzeVariableDeclaration((VariableDeclarationASTNode*)node);
      break;
    case AST_OP(expr):
      AnalyzeExpressionStatement((ExpressionStatementASTNode*)node);
      break;
    case AST_OP(compound):
      AnalyzeCompoundStatement((CompoundStatementASTNode*)node);
      break;
    case AST_OP(if):
      AnalyzeIfStatement((IfStatementASTNode*)node);
      break;
    case AST_OP(while):
      AnalyzeWhileStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(do):
      AnalyzeDoStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(switch):
      AnalyzeSwitchStatement((SwitchStatementASTNode*)node);
      break;
    case AST_OP(for):
      AnalyzeForStatement((ForStatementASTNode*)node);
      break;
    case AST_OP(return ):
      AnalyzeReturnStatement((CombinedStatementASTNode*)node);
      break;
    case AST_OP(case):
      AnalyzeCaseLabel((CaseLabelASTNode*)node);
      break;
    case AST_OP(goto):
      AnalyzeGotoStatement((GotoStatementASTNode*)node);
      break;
    case AST_OP(label):
      AnalyzeLabel((LabelASTNode*)node);
      break;

    case AST_OP(break):
    case AST_OP(continue):
      // No analysis needed for these.
      break;
    case AST_OP(asm):
      AnalyzeAsmStatement((AsmASTNode*)node);
      break;

    default:
      assert(false);
  }
  node->flags |= kASTAnalyzed;
}
