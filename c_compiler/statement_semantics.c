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
#include "compiler.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"

static void AnalyzeExpressionStatement(ExpressionStatementASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
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

static void AnalyzeSwitchStatement(SwitchStatementASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  AnalyzeStatement(node->stmt);
  SemanticCheckScalarType(node->expr);
  if (!TypeIsIntegral(node->expr->type)) {
    SemanticError(node->expr, "Switch statements need an integer type");
    return;
  }

  // The case statement must be a compound statement.
  if (node->stmt->op != AST_OP(compound)) {
    return;
  }
  
  SwitchResolver resolver = {
    .switch_node = node,
    .switch_level = 0,
  };
  
  // Visit the switch statement and all its children, collecting
  // case and defaults.
  ASTNodeVisit(&node->base, ResolveSwitchStatement, 0, &resolver);

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
  size_t num_cases = node->cases.length;
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
  
  // Check for tail recursion.  This is a direct call to the current
  // function.
  if (return_value != NULL && return_value->op == AST_OP(call)) {
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

void AnalyzeGotoStatement(GotoStatementASTNode* node) {
  // Look for label matching the goto.
  // If we find it, set the 'label' field of the node to point to it.  This
  // does not own the label node.
  // In C, labels are global to the whole function.
  ASTNodeVisit(compiler->current_function->info.function.body, FindLabel, 0, node);

  if (node->label == NULL) {
    SemanticError((ASTNode*)node, "Undefined label %s", node->label_name->value);
  }
}

typedef struct {
  String* label_name;
  bool found;
} DuplicateLabelFinder;

static void FindDuplicateLabel(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  DuplicateLabelFinder* finder = data;

  if (node->op == AST_OP(label)) {
    LabelASTNode* label = (LabelASTNode*)node;
    if (StringEqualString(finder->label_name, &label->name)) {
      if (finder->found) {
        SemanticError(&label->base, "Duplicate label %s",
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
    case AST_OP(asm):
      // No analysis needed for these.
      break;

    default:
      assert(false);
  }
  node->flags |= kASTAnalyzed;
}
