//
//  semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "semantics.h"
#include <string.h>
#include "compiler.h"
#include "contracts.h"
#include "coro_semantics.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_semantics.h"
#include "member_pointer.h"
#include "lex.h"
#include "reflection_semantics.h"
#include "statement_semantics.h"
#include "var_analysis.h"

static int semantic_catch_depth = 0;

static bool SemanticExpressionRequiresASTConstexpr(ASTNode* node, void* data) {
  (void)data;
  return node != NULL &&
         (((node->flags & kASTRequiresASTConstexpr) != 0) ||
          (node->op == AST_OP(identifier) &&
           ((IdentifierASTNode*)node)->symbol != NULL &&
           ((IdentifierASTNode*)node)->symbol->requires_ast_constexpr));
}

void SemanticEnterCatchHandler(void) {
  semantic_catch_depth++;
}

void SemanticLeaveCatchHandler(void) {
  if (semantic_catch_depth > 0) {
    semantic_catch_depth--;
  }
}

bool SemanticInCatchHandler(void) {
  return semantic_catch_depth > 0;
}

static TypeRecord* DeduceCXXInitializerListAuto(ASTNode* initializer,
                                                ASTNode* diagnostic_node) {
  if (!CompilerIsCXX() || initializer == NULL ||
      initializer->op != AST_OP(braced_init)) {
    return NULL;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  if (braced->initializers->length == 0) {
    SemanticError(diagnostic_node,
                  "Cannot deduce auto type from empty braced initializer");
    return NULL;
  }

  TypeRecord* element_type = NULL;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* element = braced->initializers->value.p[i];
    if (element == NULL || element->op != AST_OP(expr_init)) {
      SemanticError(diagnostic_node,
                    "Cannot deduce auto type from braced initializer");
      return NULL;
    }
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)element;
    expr_init->expr = AnalyzeExpression(expr_init->expr);
    if (element_type == NULL) {
      element_type = expr_init->expr->type;
    } else if (!TypeEqual(element_type, expr_init->expr->type)) {
      SemanticError(diagnostic_node,
                    "Cannot deduce auto type from mixed braced initializer types");
      return NULL;
    }
  }

  TypeRecord* deduced =
      TypeInstantiateCXXInitializerList(&compiler->syntax, element_type);
  if (deduced == NULL) {
    SemanticError(diagnostic_node,
                  "std::initializer_list must be declared before auto braced deduction");
  }
  return deduced;
}

void SemanticError(ASTNode* node, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VSemanticError(node, format, ap);
  va_end(ap);
}

void VSemanticError(ASTNode* node, const char* format, va_list ap) {
  if (abort_on_error) {
    longjmp(error_abort_state, 1);
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportError(filename, lineno, format, ap);
}

void SemanticWarning(ASTNode* node, const char* warn, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VSemanticWarning(node, warn, format, ap);
  va_end(ap);
}

void SemanticNote(ASTNode* node, const char* format, ...) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  va_list ap;
  va_start(ap, format);
  VReportNote(filename, lineno, format, ap);
  va_end(ap);
}

void SemanticNoteAtLocation(SourceLocation location, const char* format, ...) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  va_list ap;
  va_start(ap, format);
  VReportNote(filename, lineno, format, ap);
  va_end(ap);
}

static bool SymbolIsCompilerGenerated(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->flags.is_temp || symbol->flags.invented) {
    return true;
  }
  if (strncmp(symbol->name.value, "__builtin_", 10) == 0 ||
      strncmp(symbol->name.value, "__invented__", 12) == 0) {
    return true;
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(symbol->location, &filename, &lineno, &start, &end);
  return filename != NULL && strcmp(filename, "builtin") == 0;
}

void SemanticSymbolWarning(Symbol* symbol, const char* warn, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  if (SymbolIsCompilerGenerated(symbol)) {
    va_end(ap);
    return;
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(symbol->location, &filename, &lineno, &start, &end);
  VReportWarning(filename, lineno, warn, format, ap);
  va_end(ap);
}

static bool NodeIsCompilerGenerated(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(identifier): {
      Symbol* sym = ((IdentifierASTNode*)node)->symbol;
      return SymbolIsCompilerGenerated(sym);
    }
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
    case AST_OP(builtin_atomic_load):
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n):
    case AST_OP(builtin_atomic_fence):
    case AST_OP(builtin_expect):
    case AST_OP(builtin_prefetch):
    case AST_OP(builtin_trap):
    case AST_OP(builtin_unreachable):
      return true;
    case AST_OP(label): {
      LabelASTNode* label = (LabelASTNode*)node;
      return label->named;
    }
    default:
      return false;
  }
}

static void FindCompilerGeneratedNode(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  bool* found = data;
  if (*found) {
    return;
  }
  *found = NodeIsCompilerGenerated(node);
}

bool SemanticNodeIsCompilerGenerated(ASTNode* node) {
  bool found = false;
  ASTNodeVisit(node, FindCompilerGeneratedNode, 0, &found);
  return found;
}

void VSemanticWarning(ASTNode* node, const char* warn, const char* format,
                      va_list ap) {
  if (SemanticNodeIsCompilerGenerated(node)) {
    return;
  }
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportWarning(filename, lineno, warn, format, ap);
}

// True for the declared type `auto&&`, which is a forwarding reference rather
// than a plain rvalue reference ([dcl.type.auto.deduct]/[temp.deduct.call]).
static bool AutoTypeIsForwardingReference(TypeRecord* type) {
  return CompilerIsCXX() && type != NULL &&
         type->declarator == kDeclRValueReference && type->next != NULL &&
         type->next->declarator == kDeclPrimitive &&
         (type->next->type & kTypeAuto) != 0;
}

static bool LambdaInitializerHasConcreteClosureType(
    ASTNode* initializer, TypeRecord* initializer_type) {
  ASTNode* expr = initializer;
  if (expr != NULL && expr->op == AST_OP(expr_init)) {
    expr = ((ExpressionInitializerASTNode*)expr)->expr;
  }
  if (expr == NULL || (expr->flags & kASTLambdaExpression) == 0 ||
      !TypeIsStructOrUnion(initializer_type) ||
      initializer_type->info.struct_info == NULL) {
    return false;
  }
  Struct* closure = initializer_type->info.struct_info;
  for (size_t i = 0; i < closure->members.length; i++) {
    StructMember* member = closure->members.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        TypeIsFunction(member->symbol->type)) {
      continue;
    }
    if (TypeContainsTemplateParameter(member->symbol->type)) {
      return false;
    }
  }
  return true;
}

bool SemanticDeduceAutoType(Symbol* sym, ASTNode* initializer,
                            ASTNode* diagnostic_node) {
  if (sym == NULL || !TypeContainsAuto(sym->type)) {
    return true;
  }
  if (initializer == NULL) {
    SemanticError(diagnostic_node, "auto variable requires an initializer");
    return false;
  }
  ASTNode* value_initializer = initializer;
  if (value_initializer->op == AST_OP(init)) {
    value_initializer = ((BinaryASTNode*)value_initializer)->right;
  }
  if (!CompilerIsCXX()) {
    ASTNode* initializer_expr = value_initializer;
    if (initializer_expr != NULL &&
        initializer_expr->op == AST_OP(expr_init)) {
      initializer_expr =
          ((ExpressionInitializerASTNode*)initializer_expr)->expr;
    }
    if (initializer_expr == NULL || initializer_expr->type == NULL) {
      SemanticError(diagnostic_node, "Cannot deduce auto type for %s",
                    sym->name.value);
      return false;
    }
    TypeRecord* deduced =
        TypeDecayForByValueDeduction(initializer_expr->type);
    if (deduced == NULL) {
      SemanticError(diagnostic_node, "Cannot deduce auto type for %s",
                    sym->name.value);
      return false;
    }
    deduced->qualifiers |= sym->type->qualifiers;
    TypeRecordCalculateSize(deduced);
    SymbolSetType(sym, deduced);
    return true;
  }
  // A dependent initializer only has a placeholder type while its enclosing
  // template is parsed. Preserve the declared auto type so deduction runs
  // against the concrete initializer in each specialization. During that
  // specialization, nested generic lambdas can remain expression-dependent on
  // their own parameters even though their closure type is now concrete; in
  // that case auto deduction must proceed.
  TypeRecord* dependency_type = value_initializer->type;
  if (value_initializer->op == AST_OP(expr_init)) {
    ASTNode* expr = ((ExpressionInitializerASTNode*)value_initializer)->expr;
    dependency_type = expr != NULL ? expr->type : NULL;
  }
  bool concrete_lambda =
      LambdaInitializerHasConcreteClosureType(initializer, dependency_type);
  if (CompilerIsCXX() && ExpressionIsTemplateDependent(initializer) &&
      !concrete_lambda) {
    TypeRecord* concrete_init_type = dependency_type;
    if (concrete_init_type == NULL &&
        value_initializer->op == AST_OP(expr_init)) {
      ASTNode* expr = ((ExpressionInitializerASTNode*)value_initializer)->expr;
      if (expr != NULL && expr->op == AST_OP(identifier)) {
        Symbol* init_sym = ((IdentifierASTNode*)expr)->symbol;
        concrete_init_type = init_sym != NULL ? init_sym->type : NULL;
      }
    }
    if (concrete_init_type == NULL || TypeIsUnknown(concrete_init_type) ||
        TypeContainsAuto(concrete_init_type) ||
        TypeContainsTemplateParameter(concrete_init_type)) {
      return true;
    }
  }

  bool decltype_auto =
      sym->type->declarator == kDeclPrimitive &&
      (sym->type->type & kTypeDecltypeAuto) != 0;
  ASTNode* deduction_initializer = value_initializer;
  if (initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced =
        (BracedInitializerASTNode*)initializer;
    if (braced->initializers != NULL &&
        braced->initializers->length == 1) {
      ASTNode* only = braced->initializers->value.p[0];
      if (only != NULL && only->op == AST_OP(designated_init)) {
        DesignatedInitializerASTNode* designated =
            (DesignatedInitializerASTNode*)only;
        if (designated->designators == NULL ||
            designated->designators->length == 0) {
          deduction_initializer = designated->init;
        }
      }
    }
  }
  TypeRecord* initializer_type = deduction_initializer->type;
  TypeRecord* deduced =
      decltype_auto ? NULL
                    : DeduceCXXInitializerListAuto(deduction_initializer,
                                                   diagnostic_node);
  if (deduced != NULL) {
    SymbolSetType(sym, deduced);
    return true;
  }
  ASTNode* initializer_expr = deduction_initializer;
  if (deduction_initializer->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)deduction_initializer;
    initializer_expr = expr_init->expr;
    initializer_type = expr_init->expr->type;
  }
  if (decltype_auto) {
    if (initializer_expr == NULL ||
        initializer_expr->op == AST_OP(braced_init)) {
      SemanticError(diagnostic_node,
                    "Cannot deduce decltype(auto) from braced initializer");
      return false;
    }
    deduced = TypeDeduceDecltypeAuto(initializer_expr);
  } else {
    deduced = TypeDeduceAuto(sym->type, initializer_type);
  }
  bool forwarding_reference = AutoTypeIsForwardingReference(sym->type);
  if (deduced == NULL) {
    SemanticError(diagnostic_node, "Cannot deduce auto type for %s",
                  sym->name.value);
    return false;
  }
  // Collapse `T& &&` to `T&`: `auto&& x = lvalue;` declares an lvalue
  // reference.  Without this the idiomatic `for (auto&& x : range)` -- and the
  // `auto&&` hidden variables the range-for lowering itself introduces -- are
  // rejected for binding an rvalue reference to an lvalue.
  if (forwarding_reference && deduced->declarator == kDeclRValueReference &&
      initializer_expr != NULL &&
      initializer_expr->value_category == kValueCategoryLvalue) {
    deduced->declarator = kDeclReference;
    TypeRecordCalculateSize(deduced);
  }
  SymbolSetType(sym, deduced);
  return true;
}

void SemanticCheckScalarType(ASTNode* node) {
  if (node == NULL) {
    return;
  }
  if (TypeIsVoid(node->type)) {
    SemanticError(node, "Illegal use of void type");
    return;
  }
  if (TypeIsStructOrUnion(node->type)) {
    SemanticError(node, "Illegal use of composite type (struct or union)");
  }
}

// Check that we've used all local variables and parameters and emit a warning
// for each unused one.  Unused locals generally point to a programming error
// where an inner scope variable hides an outer scope of the same name; unused
// parameters (-Wunused-parameter, part of -Wextra) are off by default because
// it is common for parameters to be unused deliberately.
static void CheckForUnusedLocalSymbols(Syntax* syntax, ASTNode* node) {
  String function_name;
  StringInit(&function_name, NULL);
  if (node != NULL && node->type != NULL && TypeIsFunction(node->type)) {
    SymbolFunctionDiagnosticName(node->type->info.function.symbol,
                                 &function_name);
  }
  for (size_t i = 0; i < syntax->all_local_symbols.length; i++) {
    Symbol* symbol = syntax->all_local_symbols.value.p[i];
    if (symbol->flags.is_temp || symbol->flags.invented ||
        symbol->flags.is_name_independent || symbol->name.length == 0 ||
        SymbolHasAttribute(symbol, "unused")) {
      continue;
    }
    // A typedef/alias declared in a function body is diagnosed separately and
    // is never reported as an "unused variable".  Typedef names are marked used
    // when they are resolved as a type (see type_parse.c).
    if (StorageIs(symbol->storage, STO(typedef))) {
      if (!symbol->flags.used) {
        SemanticSymbolWarning(symbol, "unused-local-typedef",
                              "typedef '%s' is not used in function '%s'",
                              symbol->name.value, function_name.value);
      }
      continue;
    }
    if (!symbol->flags.used) {
      SemanticSymbolWarning(symbol, "unused-variable",
                            "Local variable '%s' is not used in function '%s'",
                            symbol->name.value, function_name.value);
      continue;
    }
    // Referenced, but only ever assigned to: its value is never observed.
    // Restricted to scalar objects (matching clang): assigning a whole struct
    // or union can be a meaningful operation even if the result is not read.
    if (!symbol->is_read && TypeIsScalar(symbol->type)) {
      SemanticSymbolWarning(
          symbol, "unused-but-set-variable",
          "Variable '%s' is set but never used in function '%s'",
          symbol->name.value, function_name.value);
    }
  }
  // Parameters live in the function type's prototype rather than in
  // all_local_symbols, so they are checked separately here.  This mirrors the
  // per-function-definition treatment of locals above (template patterns never
  // reach SemanticAnalyzeFunction, so this only fires for concrete functions).
  if (node != NULL && node->type != NULL && TypeIsFunction(node->type)) {
    Vector* prototype = &node->type->info.function.prototype;
    for (size_t i = 0; i < prototype->length; i++) {
      Symbol* param = prototype->value.p[i];
      if (param == NULL) {
        continue;
      }
      // Skip the implicit 'this' parameter of C++ member functions.
      if (StringEqual(&param->name, "this")) {
        continue;
      }
      // Skip unnamed parameters: an unnamed parameter cannot be used, so it is
      // clearly intentional (common in C prototypes and C++ overloads/interface
      // conformance).
      if (param->name.length == 0) {
        continue;
      }
      if (param->flags.used || param->flags.is_temp || param->flags.invented ||
          SymbolHasAttribute(param, "unused")) {
        continue;
      }
      SemanticSymbolWarning(param, "unused-parameter",
                            "Parameter '%s' is not used in function '%s'",
                            param->name.value, function_name.value);
    }
  }
  StringDestruct(&function_name);
}

static void CheckVLAArgs(Syntax* syntax, ASTNode* node) {
  Vector* prototype = &node->type->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* arg = prototype->value.p[i];
    if (TypeIsVLA(arg->type)) {
      // Variable length array, check that all the dimensions are
      // bound to variables.
      if (arg->type->info.array.is_placeholder_vla) {
        SemanticError(node,
                      "Variable length array '%s' dimensions must be bound "
                      "to a variable in function definition", arg->name.value);
      }
    }
  }
}

static bool NodeIsZero(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number): {
      ConstantASTNode* c = (ConstantASTNode*)node;
      return c->value.ivalue == 0;
      }
    case AST_OP(question): {      // Conditional expression:
      node = ((BinaryASTNode*)node)->right;     // Colon.
      ASTNode* left = ((BinaryASTNode*)node)->left;
      ASTNode* right = ((BinaryASTNode*)node)->right;
      return NodeIsZero(left) && NodeIsZero(right);
    }
    case AST_OP(cast): {      // cast
      CastASTNode* c = (CastASTNode*)node;
      return c->expr != NULL && NodeIsZero(c->expr);
    }
    default:
    return false;
  }
}

void SemanticAnalyzeFunction(Syntax* syntax, ASTNode* node) {
  TypeRecord* saved_current_function = compiler->current_function;
  Struct* saved_class_access_context =
      compiler->current_class_access_context;
  int saved_immediate_context_depth =
      compiler->immediate_function_context_depth;
  int saved_constant_evaluation_depth =
      compiler->constant_evaluation_required_depth;
  compiler->current_function = node->type;
  compiler->current_class_access_context =
      node->type->info.function.cxx_member_owner;
  compiler->immediate_function_context_depth = 0;
  compiler->constant_evaluation_required_depth = 0;
  if (TypeIsConstevalOnly(node->type) &&
      !node->type->info.function.is_consteval &&
      !node->type->info.function.is_constexpr) {
    SemanticError(node,
                  "A function with a consteval-only reflection signature "
                  "must be declared constexpr or consteval");
  }
  // Check Variable Langth Array arguments.
  CheckVLAArgs(syntax, node);
  SemanticAnalyzeCoroutineFunction(node);
  Vector* prototype = &node->type->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* param = prototype->value.p[i];
    if (param == NULL) {
      continue;
    }
    SemanticAttachAnnotationAttributes(&param->attributes, param);
    if (param->type != NULL) {
      param->type =
          SemanticResolveDependentSpliceType(param->type, (ASTNode*)node);
    }
  }
  if (node->type->info.function.cxx_member_owner != NULL) {
    Struct* owner = node->type->info.function.cxx_member_owner;
    SemanticResolveStructDependentSplices(owner, (ASTNode*)node);
    if (owner->tag_symbol != NULL) {
      SemanticAttachAnnotationAttributes(&owner->tag_symbol->attributes,
                                         owner->tag_symbol);
    }
  }
  
  // Mark this function's body as in-flight so speculative constant folding
  // reached during its own analysis (e.g. a recursive constexpr call) does not
  // try to lower the still-incomplete body to pcode and crash code generation.
  VectorAppend(&compiler->functions_being_analyzed, node->type);

  // Perform semantic analysis on all the statements in the function body.
  AnalyzeStatement(node->type->info.function.body);
  StatementFinishAutoReturnDeduction(node->type, node);
  SemanticAnalyzeFunctionContracts(node->type, node);
  CheckUnusedLabels(node->type->info.function.body);
  // AnalyzeVariables(node->type->info.function.body);
  CheckForUnusedLocalSymbols(syntax, node);

  // With the body fully typed, inject destructor calls for automatic objects at
  // each return/break/continue (the parser only appends them on fall-through).
  if (NumErrors() == 0) {
    CXXInsertScopeExitDestructors(node->type);
  }

  // Analysis is complete: the body is now fully typed and safe to lower.
  Vector* analyzing = &compiler->functions_being_analyzed;
  for (size_t i = analyzing->length; i-- > 0;) {
    if (analyzing->value.p[i] == node->type) {
      VectorDeleteElement(analyzing, i);
      break;
    }
  }
  compiler->immediate_function_context_depth = saved_immediate_context_depth;
  compiler->constant_evaluation_required_depth =
      saved_constant_evaluation_depth;
  compiler->current_function = saved_current_function;
  compiler->current_class_access_context = saved_class_access_context;
}

static bool TypeHasDoubleRepresentationAndRank(TypeRecord* type) {
  return TypeIsDouble(type) || TypeIsFloat64(type);
}

// This table contains mappings from one type to another.  The 'from'
// and 'to' are pointers to functions that return true if the passed
// type is of the type specified by the function.  The 'op' is a new
// operation to give to a new AST node that will be inserted between
// the parent and the 'from' node.
struct {
  bool (*from)(TypeRecord*);  // Function to determine type of from.
  bool (*to)(TypeRecord*);    // Function to determine type of to.
  ASTOpcode op;               // New opcode for the conversion node.
  bool warning;
} type_conversions[] = {
    // Any pointer can be converted to/from a void*.
    {TypeIsPointer, TypeIsVoidPointer, AST_OP(bad)},
    {TypeIsVoidPointer, TypeIsPointer, AST_OP(bad)},

    {TypeIsInt, TypeIsShort, AST_OP(i2s)},
    {TypeIsInt, TypeIsLong, AST_OP(i2l)},
    {TypeIsInt, TypeIsLongLong, AST_OP(i2ll)},
    {TypeIsInt, TypeIsCharFamily, AST_OP(i2c)},
    {TypeIsInt, TypeUsesFloat32Representation, AST_OP(i2f)},
    {TypeIsInt, TypeHasDoubleRepresentationAndRank, AST_OP(i2d)},
    {TypeIsInt, TypeIsLongDouble, AST_OP(i2ld)},
    {TypeIsInt, TypeIsBool, AST_OP(i2b)},

    {TypeIsCharFamily, TypeIsCharFamily, AST_OP(i2c)},
    {TypeIsCharFamily, TypeIsShort, AST_OP(c2s)},
    {TypeIsCharFamily, TypeIsLong, AST_OP(c2l)},
    {TypeIsCharFamily, TypeIsLongLong, AST_OP(c2ll)},
    {TypeIsCharFamily, TypeIsInt, AST_OP(c2i)},
    {TypeIsCharFamily, TypeUsesFloat32Representation, AST_OP(c2f)},
    {TypeIsCharFamily, TypeHasDoubleRepresentationAndRank, AST_OP(c2d)},
    {TypeIsCharFamily, TypeIsLongDouble, AST_OP(c2ld)},
    {TypeIsCharFamily, TypeIsBool, AST_OP(c2b)},

    {TypeIsShort, TypeIsCharFamily, AST_OP(s2c)},
    {TypeIsShort, TypeIsLong, AST_OP(s2l)},
    {TypeIsShort, TypeIsLongLong, AST_OP(s2ll)},
    {TypeIsShort, TypeIsInt, AST_OP(s2i)},
    {TypeIsShort, TypeUsesFloat32Representation, AST_OP(s2f)},
    {TypeIsShort, TypeHasDoubleRepresentationAndRank, AST_OP(s2d)},
    {TypeIsShort, TypeIsLongDouble, AST_OP(s2ld)},
    {TypeIsShort, TypeIsBool, AST_OP(s2b)},

    {TypeIsLong, TypeIsCharFamily, AST_OP(l2c)},
    {TypeIsLong, TypeIsShort, AST_OP(l2s)},
    {TypeIsLong, TypeIsLongLong, AST_OP(l2ll)},
    {TypeIsLong, TypeIsInt, AST_OP(l2i)},
    {TypeIsLong, TypeUsesFloat32Representation, AST_OP(l2f)},
    {TypeIsLong, TypeHasDoubleRepresentationAndRank, AST_OP(l2d)},
    {TypeIsLong, TypeIsLongDouble, AST_OP(l2ld)},
    {TypeIsLong, TypeIsBool, AST_OP(l2b)},

    {TypeIsLongLong, TypeIsCharFamily, AST_OP(ll2c)},
    {TypeIsLongLong, TypeIsShort, AST_OP(ll2s)},
    {TypeIsLongLong, TypeIsLong, AST_OP(ll2l)},
    {TypeIsLongLong, TypeIsInt, AST_OP(ll2i)},
    {TypeIsLongLong, TypeUsesFloat32Representation, AST_OP(ll2f)},
    {TypeIsLongLong, TypeHasDoubleRepresentationAndRank, AST_OP(ll2d)},
    {TypeIsLongLong, TypeIsLongDouble, AST_OP(ll2ld)},
    {TypeIsLongLong, TypeIsBool, AST_OP(ll2b)},

    {TypeIsBitInt, TypeUsesFloat32Representation, AST_OP(ll2f)},
    {TypeIsBitInt, TypeHasDoubleRepresentationAndRank, AST_OP(ll2d)},
    {TypeIsBitInt, TypeIsLongDouble, AST_OP(ll2ld)},
    {TypeIsBitInt, TypeIsBool, AST_OP(ll2b)},

    {TypeUsesFloat32Representation, TypeIsCharFamily, AST_OP(f2c)},
    {TypeUsesFloat32Representation, TypeIsShort, AST_OP(f2s)},
    {TypeUsesFloat32Representation, TypeIsLongLong, AST_OP(f2ll)},
    {TypeUsesFloat32Representation, TypeIsInt, AST_OP(f2i)},
    {TypeUsesFloat32Representation, TypeIsLong, AST_OP(f2l)},
    {TypeUsesFloat32Representation, TypeHasDoubleRepresentationAndRank,
     AST_OP(f2d)},
    {TypeUsesFloat32Representation, TypeIsLongDouble, AST_OP(f2ld)},
    {TypeUsesFloat32Representation, TypeIsBool, AST_OP(f2b)},
    {TypeUsesFloat32Representation, TypeIsBitInt, AST_OP(f2ll)},

    {TypeHasDoubleRepresentationAndRank, TypeIsCharFamily, AST_OP(d2c)},
    {TypeHasDoubleRepresentationAndRank, TypeIsLong, AST_OP(d2l)},
    {TypeHasDoubleRepresentationAndRank, TypeIsLongLong, AST_OP(d2ll)},
    {TypeHasDoubleRepresentationAndRank, TypeIsInt, AST_OP(d2i)},
    {TypeHasDoubleRepresentationAndRank, TypeUsesFloat32Representation,
     AST_OP(d2f)},
    {TypeHasDoubleRepresentationAndRank, TypeIsShort, AST_OP(d2s)},
    {TypeHasDoubleRepresentationAndRank, TypeIsLongDouble, AST_OP(d2ld)},
    {TypeHasDoubleRepresentationAndRank, TypeIsBool, AST_OP(d2b)},
    {TypeHasDoubleRepresentationAndRank, TypeIsBitInt, AST_OP(d2ll)},

    {TypeIsLongDouble, TypeIsCharFamily, AST_OP(ld2c)},
    {TypeIsLongDouble, TypeIsLong, AST_OP(ld2l)},
    {TypeIsLongDouble, TypeIsLongLong, AST_OP(ld2ll)},
    {TypeIsLongDouble, TypeIsInt, AST_OP(ld2i)},
    {TypeIsLongDouble, TypeUsesFloat32Representation, AST_OP(ld2f)},
    {TypeIsLongDouble, TypeIsShort, AST_OP(ld2s)},
    {TypeIsLongDouble, TypeHasDoubleRepresentationAndRank, AST_OP(ld2d)},
    {TypeIsLongDouble, TypeIsBool, AST_OP(ld2b)},
    {TypeIsLongDouble, TypeIsBitInt, AST_OP(ld2ll)},

    {TypeIsBool, TypeIsCharFamily, AST_OP(b2c)},
    {TypeIsBool, TypeIsShort, AST_OP(b2s)},
    {TypeIsBool, TypeIsLongLong, AST_OP(b2ll)},
    {TypeIsBool, TypeIsInt, AST_OP(b2i)},
    {TypeIsBool, TypeIsLong, AST_OP(b2l)},
    {TypeIsBool, TypeHasDoubleRepresentationAndRank, AST_OP(b2d)},
    {TypeIsBool, TypeIsLongDouble, AST_OP(b2ld)},
    {TypeIsBool, TypeUsesFloat32Representation, AST_OP(b2f)},
};

#define NUM_TYPE_CONVERSIONS (sizeof(type_conversions) / sizeof(type_conversions[0]))

static bool CXXForbidsImplicitVoidPointerConversion(TypeRecord* from,
                                                    TypeRecord* to,
                                                    ConversionContext ctx) {
  return CompilerIsCXX() && ctx != kConvertCast && TypeIsVoidPointer(from) &&
         TypeIsPointer(to) && !TypeIsVoidPointer(to);
}

void SemanticTypeConversionError(ASTNode* from, TypeRecord* to,
                                 const char* format) {
  String from_string = {0};
  String to_string = {0};
  TypeRecordToString(from->type, &from_string);
  TypeRecordToString(to, &to_string);
  SemanticError(from, format, from_string.value, to_string.value);
  StringDestruct(&from_string);
  StringDestruct(&to_string);
}

void SemanticTypeConversionWarning(ASTNode* from, TypeRecord* to,
                                   const char* warn, const char* format) {
  String from_string = {0};
  String to_string = {0};
  TypeRecordToString(from->type, &from_string);
  TypeRecordToString(to, &to_string);
  SemanticWarning(from, warn, format, from_string.value, to_string.value);
  StringDestruct(&from_string);
  StringDestruct(&to_string);
}

// Fold an integer-to-integer constant conversion to type `to`.  The stored
// 64-bit value is reduced to the destination width (a no-op when widening) and
// then re-extended: sign-extended for a signed destination, zero-extended for
// an unsigned one.  Consulting the destination signedness (rather than always
// sign-extending) is required so that, e.g., converting an unsigned int such as
// 0xabcd0000 to (unsigned) long long zero-extends to 0xabcd0000 instead of
// becoming 0xffffffffabcd0000.
static ASTNode* ConvertIntConstantToType(ConstantASTNode* c, TypeRecord* to) {
  // Normalize the source value to a full 64-bit representation according to the
  // source type's width and signedness (char/short constants are stored in
  // their raw low bits, not pre-extended).
  int src_bits = TypeIsBitInt(c->base.type)
                     ? c->base.type->bit_width
                     : (int)c->base.type->size * 8;
  uint64_t v = (uint64_t)c->value.ivalue;
  if (src_bits > 0 && src_bits < 64) {
    uint64_t smask = (1ULL << src_bits) - 1ULL;
    v &= smask;
    if (!TypeIsUnsigned(c->base.type) && (v & (1ULL << (src_bits - 1))) != 0) {
      v |= ~smask;
    }
  }
  // Reduce to the destination width and re-extend per destination signedness.
  int dst_bits =
      TypeIsBitInt(to) ? to->bit_width : (int)to->size * 8;
  if (dst_bits > 0 && dst_bits < 64) {
    uint64_t dmask = (1ULL << dst_bits) - 1ULL;
    v &= dmask;
    if (!TypeIsUnsigned(to) && (v & (1ULL << (dst_bits - 1))) != 0) {
      v |= ~dmask;
    }
  }
  c->value.ivalue = (int64_t)v;
  return (ASTNode*)c;
}

static ASTNode* ConvertIntToDouble(ConstantASTNode* c, int bits) {
  if (TypeIsUnsigned(c->base.type)) {
    int64_t mask = (1LL << bits) - 1LL;
    c->value.ivalue &= mask;
  } else {
    c->value.ivalue <<= 64 - bits;
    c->value.ivalue >>= 64 - bits;
  }
  double v = (double)c->value.ivalue;
  c->value.fvalue = v;
  c->base.op = AST_OP(fnumber);
  return (ASTNode*)c;
}

static ASTNode* ConvertDoubleToInt(ConstantASTNode* c, int bits) {
  c->value.ivalue = (int64_t)c->value.fvalue;
  c->value.ivalue <<= 64 - bits;
  c->value.ivalue >>= 64 - bits;
  c->base.op = AST_OP(number);
  return (ASTNode*)c;
}

static ASTNode* ConvertPotentialConstant(ASTNode* from, TypeRecord* to,
                                         ASTOpcode opcode) {
  ConstantASTNode* c = (ConstantASTNode*)from;
  switch (from->op) {
    case AST_OP(number):
      switch (opcode) {
        default:
          break;
        case AST_OP(i2s):
        case AST_OP(i2c):
        case AST_OP(i2l):
        case AST_OP(i2ll):
          return ConvertIntConstantToType(c, to);
        case AST_OP(i2f):
        case AST_OP(i2d):
        case AST_OP(i2ld):
          return ConvertIntToDouble(c, 32);
        case AST_OP(i2b):
        case AST_OP(c2b):
        case AST_OP(s2b):
        case AST_OP(ll2b):
        case AST_OP(l2b):
          c->value.ivalue = c->value.ivalue != 0;
          return from;
        case AST_OP(c2i):
        case AST_OP(c2s):
        case AST_OP(c2l):
        case AST_OP(c2ll):
          return ConvertIntConstantToType(c, to);
        case AST_OP(c2f):
        case AST_OP(c2d):
        case AST_OP(c2ld):
          return ConvertIntToDouble(c, 8);
        case AST_OP(s2i):
        case AST_OP(s2c):
        case AST_OP(s2l):
        case AST_OP(s2ll):
          return ConvertIntConstantToType(c, to);
        case AST_OP(s2f):
        case AST_OP(s2d):
        case AST_OP(s2ld):
          return ConvertIntToDouble(c, 16);
        case AST_OP(l2i):
        case AST_OP(ll2i):
        case AST_OP(l2c):
        case AST_OP(ll2c):
        case AST_OP(l2s):
        case AST_OP(ll2s):
          return ConvertIntConstantToType(c, to);
        case AST_OP(ll2l):
        case AST_OP(l2ll):
          return from;
        case AST_OP(l2f):
        case AST_OP(ll2f):
        case AST_OP(ll2d):
        case AST_OP(l2d):
        case AST_OP(ll2ld):
        case AST_OP(l2ld):
          return ConvertIntToDouble(c, 64);
        case AST_OP(b2i):
        case AST_OP(b2c):
        case AST_OP(b2s):
        case AST_OP(b2l):
          return from;
        case AST_OP(b2ll):
        case AST_OP(b2f):
        case AST_OP(b2d):
        case AST_OP(b2ld):
          c->value.fvalue = (double)c->value.ivalue;
          return from;
      }
      break;
    case AST_OP(fnumber):
      switch (opcode) {
        default:
          break;
        case AST_OP(f2i):
        case AST_OP(d2i):
        case AST_OP(ld2i):
          return ConvertDoubleToInt(c, 32);
        case AST_OP(f2c):
        case AST_OP(d2c):
        case AST_OP(ld2c):
          return ConvertDoubleToInt(c, 8);
        case AST_OP(f2s):
        case AST_OP(d2s):
        case AST_OP(ld2s):
          return ConvertDoubleToInt(c, 16);
        case AST_OP(f2l):
        case AST_OP(f2ll):
        case AST_OP(d2l):
        case AST_OP(d2ll):
        case AST_OP(ld2l):
        case AST_OP(ld2ll):
          return ConvertDoubleToInt(c, 64);
        case AST_OP(f2d):
        case AST_OP(f2ld):
        case AST_OP(d2f):
        case AST_OP(d2ld):
        case AST_OP(ld2f):
        case AST_OP(ld2d):
          return from;
        case AST_OP(f2b):
        case AST_OP(d2b):
        case AST_OP(ld2b):
          c->value.ivalue = c->value.fvalue != 0;
          return from;
      }
      break;
    default:
      return NULL;
  }
  return NULL;
}

void NormalConversion(ASTNode* from, TypeRecord* to) {
  SemanticConvertType(from, to, kConvertNormal);
}

static bool CXXSameClassTypeIgnoringQualifiers(TypeRecord* from,
                                               TypeRecord* to) {
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to)) {
    return false;
  }
  TypeRecord* from_copy =
      TypeRecordCalculateSize(TypeRecordCloneSpine(from));
  TypeRecord* to_copy =
      TypeRecordCalculateSize(TypeRecordCloneSpine(to));
  if (from_copy == NULL || to_copy == NULL) {
    TypeRecordDelete(from_copy);
    TypeRecordDelete(to_copy);
    return false;
  }
  from_copy->qualifiers = kQualPlain;
  to_copy->qualifiers = kQualPlain;
  bool same = TypeEqual(from_copy, to_copy);
  TypeRecordDelete(from_copy);
  TypeRecordDelete(to_copy);
  return same;
}

static bool CXXStructLayoutCompatibleShortcut(TypeRecord* from,
                                              TypeRecord* to) {
  if (!TypeEqualIgnoringSign(from, to)) {
    return false;
  }
  if (!TypeIsStructOrUnion(from) || !TypeIsStructOrUnion(to)) {
    return true;
  }
  if (CXXSameClassTypeIgnoringQualifiers(from, to)) {
    return true;
  }
  // Preserve the existing derived-to-base value conversion without treating
  // arbitrary, layout-similar class types as interchangeable. Distinct class
  // template specializations such as duration<..., ratio<1>> and
  // duration<..., milli> must go through their converting constructor.
  return CompilerIsCXX() &&
         TypeBaseOffset(from, to, /*public_only=*/true, NULL);
}

static bool TypeDiscardsQualifiers(TypeRecord* from, TypeRecord* to) {
  if (from == NULL || to == NULL) {
    return false;
  }
  return (from->qualifiers & ~to->qualifiers) != 0;
}

static bool PointerSignednessDiffers(TypeRecord* from, TypeRecord* to) {
  if (from == NULL || to == NULL || from->next == NULL || to->next == NULL) {
    return false;
  }
  TypeRecord* from_pointee = from->next;
  TypeRecord* to_pointee = to->next;
  return TypeIsIntegral(from_pointee) && TypeIsIntegral(to_pointee) &&
         TypeEqualIgnoringSign(from_pointee, to_pointee) &&
         TypeIsUnsigned(from_pointee) != TypeIsUnsigned(to_pointee);
}

static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static ASTNode* NewVirtualBaseOffsetLoad(ASTNode* receiver, int vbtable_index) {
  SourceLocation location = receiver->location;
  ASTNode* receiver_clone = ASTNodeClone(receiver, IdentityCloneNode, NULL,
                                        NULL);
  ASTNode* vbptr_name =
      NewStringConstantASTNode(NewString("__vbptr"), NULL, location);
  ASTNode* vbptr =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver_clone,
                       vbptr_name);
  ASTNode* index =
      NewIntConstantASTNode(vbtable_index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* load = NewBinaryASTNode(AST_OP(subscript), NULL, location, vbptr,
                                   index);
  return AnalyzeExpression(load);
}

static ASTNode* AddStaticOffsetToRuntimeOffset(ASTNode* offset, int byte_offset,
                                               SourceLocation location) {
  if (byte_offset == 0) {
    return offset;
  }
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  ASTNode* tail = NewIntConstantASTNode(byte_offset, int_type, location);
  ASTNode* combined =
      NewBinaryASTNode(AST_OP(plus), int_type, location, offset, tail);
  combined->flags |= kASTAnalyzed;
  return combined;
}

static ASTNode* NewRawPointerAdjustment(ASTNode* from, TypeRecord* to,
                                        ASTNode* offset) {
  TypeRecordIncRef(to);
  ASTNode* converted_node =
      NewBinaryASTNode(AST_OP(plus), to, from->location, from, offset);
  converted_node->flags |= kASTAnalyzed;
  return converted_node;
}

static bool TryConvertDerivedPointer(ASTNode* from, TypeRecord* to) {
  if (from == NULL || to == NULL || !TypeIsPointer(from->type) || !TypeIsPointer(to) ||
      from->type->next == NULL || to->next == NULL) {
    return false;
  }

  CXXBaseAdjustment adjustment;
  if (!TypeBaseAdjustment(from->type->next, to->next, /*public_only=*/true,
                          &adjustment)) {
    return false;
  }
  if (from->type->next->info.struct_info == to->next->info.struct_info) {
    return false;
  }
  if (adjustment.kind == kCXXBaseAdjustmentNone ||
      (adjustment.kind == kCXXBaseAdjustmentStatic &&
       adjustment.byte_offset == 0)) {
    ASTNodeSetType(from, to);
    return true;
  }

  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  ASTNode* offset_node = NULL;
  if (adjustment.kind == kCXXBaseAdjustmentVirtual) {
    offset_node = NewVirtualBaseOffsetLoad(from, adjustment.vbtable_index);
    offset_node = AddStaticOffsetToRuntimeOffset(
        offset_node, adjustment.byte_offset, from->location);
  } else {
    offset_node = NewIntConstantASTNode(
        adjustment.byte_offset,
        NewTypeRecordWithSize(kTypeInt, kQualPlain), from->location);
  }
  ASTNode* converted_node = NewRawPointerAdjustment(from, to, offset_node);
  ASTNodeReplaceChild(parent, child_id, converted_node, false);
  return true;
}

static bool ConversionOperatorAllowedInContext(TypeRecord* func,
                                               TypeRecord* to,
                                               ConversionContext ctx) {
  if (!func->info.function.is_explicit_conversion) {
    return true;
  }
  return ctx == kConvertCast ||
         (ctx == kConvertContextualBool && TypeIsBool(to));
}

// Rank of the standard conversion needed to turn a conversion operator's
// result type into the requested target `to`.  Lower is better; -1 means no
// implicit standard conversion exists.  The tiers mirror [over.ics.scs]
// (exact < promotion < conversion) so the best conversion operator can be
// selected and genuine ambiguities detected.
static int ConversionOperatorTrailingRank(TypeRecord* result, TypeRecord* to) {
  if (result == NULL || to == NULL) {
    return -1;
  }
  if (TypeEqual(result, to)) {
    return 0;  // Exact match: no trailing conversion required.
  }
  // A conversion operator yields a prvalue of its result type, which binds to a
  // more-cv-qualified target of the same class (e.g. `operator string_view`'s
  // prvalue binds to `const string_view&`).  Adding top-level qualifiers is a
  // zero-cost qualification adjustment, so treat class results that match the
  // target up to top-level cv as an exact match.  (Restricted to class types;
  // TypeEqualIgnoringQualifiers is a real structural comparison here, not the
  // sign hack guarded against below.)
  if (TypeIsStructOrUnion(result) && TypeIsStructOrUnion(to) &&
      result->qualifiers != to->qualifiers) {
    TypeRecord* bare_result = TypeRecordCopy(result);
    TypeRecord* bare_to = TypeRecordCopy(to);
    bare_result->qualifiers = kQualPlain;
    bare_to->qualifiers = kQualPlain;
    bool equal = TypeEqual(bare_result, bare_to);
    TypeRecordDelete(bare_result);
    TypeRecordDelete(bare_to);
    if (equal) {
      return 0;
    }
  }
  // A trailing standard conversion is only modeled for arithmetic scalars.
  // Class, pointer and reference targets require an exact match: notably, any
  // two distinct struct/union types compare equal-ignoring-sign (their sign
  // bits are meaningless), which would otherwise make an unrelated
  // class-returning operator look like a viable candidate; and a reference
  // target must bind an exact `operator T&` rather than a value-returning one.
  if (TypeIsStructOrUnion(result) || TypeIsStructOrUnion(to) ||
      TypeIsPointerOrArray(result) || TypeIsPointerOrArray(to) ||
      TypeIsReference(result) || TypeIsReference(to)) {
    return -1;
  }
  if (TypeEqualIgnoringSign(result, to)) {
    return 1;  // Differs only in signedness.
  }
  // Integral and floating-point promotions rank above ordinary conversions.
  if (TypeIsInt(to) &&
      (TypeIsCharFamily(result) || TypeIsShort(result) || TypeIsBool(result) ||
       (TypeIsEnum(result) && !TypeIsScopedEnum(result)))) {
    return 2;
  }
  if (TypeIsDouble(to) && TypeIsFloat(result)) {
    return 2;
  }
  // Any other standard scalar conversion the codegen table supports (integral
  // conversions, floating-integral conversions, contextual bool, etc.).
  for (int i = 0; i < NUM_TYPE_CONVERSIONS; i++) {
    if (type_conversions[i].from(result) && type_conversions[i].to(to)) {
      return 3;
    }
  }
  return -1;
}

// Select the best conversion operator of `str` that can reach `to`, allowing a
// trailing standard conversion on the operator's result (a user-defined
// conversion sequence is [conversion operator][standard conversion]).  An exact
// match is preferred; among converting candidates the best-ranked one wins, and
// two equally-ranked candidates with distinct result types are ambiguous (which
// we report by returning NULL so the caller emits a conversion error).
static StructMember* FindConversionOperator(Struct* str, TypeRecord* to,
                                            ConversionContext ctx) {
  if (str == NULL) {
    return NULL;
  }
  Vector candidates;
  VectorInit(&candidates);
  CollectConversionOperators(str, &candidates);

  StructMember* best = NULL;
  TypeRecord* best_result = NULL;
  int best_rank = -1;
  bool ambiguous = false;
  Vector materialized_results;
  VectorInit(&materialized_results);
  for (size_t i = 0; i < candidates.length; i++) {
    StructMember* member = candidates.value.p[i];
    TypeRecord* func = member->symbol->type;
    TypeRecord* result = func->next;
    // Conversion operator templates have a dependent result type that cannot be
    // ranked directly; they are handled separately by deducing their arguments
    // from the target type (see SelectConversionOperatorTemplate).
    if (member->symbol->flags.is_template) {
      continue;
    }
    if (!ConversionOperatorAllowedInContext(func, to, ctx)) {
      continue;
    }
    // The recorded result type can still be a deferred template-id when it
    // names a class template completed later in the TU (e.g.
    // basic_string::operator basic_string_view).  Materialize it here, where
    // `to` is necessarily complete, so ranking compares the real
    // specialization rather than the bare primary.
    TypeRecord* ranked_result =
        TypeMaterializeClassTemplateSpecialization(&compiler->syntax, result);
    if (ranked_result != result) {
      VectorAppend(&materialized_results, ranked_result);
    }
    int rank = ConversionOperatorTrailingRank(ranked_result, to);
    if (rank < 0) {
      continue;
    }
    if (best == NULL || rank < best_rank) {
      best = member;
      best_result = ranked_result;
      best_rank = rank;
      ambiguous = false;
    } else if (rank == best_rank && !TypeEqual(ranked_result, best_result)) {
      ambiguous = true;
    }
  }
  VectorDestruct(&candidates);
  for (size_t i = 0; i < materialized_results.length; i++) {
    TypeRecordDelete(materialized_results.value.p[i]);
  }
  VectorDestruct(&materialized_results);
  return ambiguous ? NULL : best;
}

bool SemanticConvertCXXSwitchCondition(ASTNode* from) {
  if (!CompilerIsCXX() || from == NULL || from->type == NULL ||
      !TypeIsStructOrUnion(from->type) ||
      from->type->info.struct_info == NULL) {
    return false;
  }

  Vector candidates;
  VectorInit(&candidates);
  CollectConversionOperators(from->type->info.struct_info, &candidates);
  TypeRecord* target = NULL;
  size_t viable_count = 0;
  for (size_t i = 0; i < candidates.length; i++) {
    StructMember* member = candidates.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        member->symbol->type == NULL || member->symbol->flags.is_template) {
      continue;
    }
    TypeRecord* function = member->symbol->type;
    if (!ConversionOperatorAllowedInContext(function, function->next,
                                            kConvertNormal)) {
      continue;
    }
    TypeRecord* result = function->next;
    if (TypeIsReference(result)) {
      result = result->next;
    }
    if (result == NULL ||
        (!TypeIsIntegral(result) && !TypeIsEnum(result))) {
      continue;
    }
    viable_count++;
    target = result;
  }
  VectorDestruct(&candidates);
  if (viable_count != 1 || target == NULL) {
    return false;
  }
  SemanticConvertType(from, TypeRecordCopy(target), kConvertNormal);
  return true;
}

// Select a conversion operator template of `str` (or a base) whose target-type
// deduction against `to` succeeds, returning its member symbol via *out_templ
// and the deduced template arguments via *out_args (caller owns).  The
// specialization itself is left to be built by the ordinary member-template
// instantiation path (fed the deduced arguments as explicit template
// arguments), which owns the resulting symbol; this avoids double-freeing an
// instantiation that the template subsystem already tracks.  Returns false when
// no template conversion operator deduces to `to`.
static bool SelectConversionOperatorTemplate(Struct* str, TypeRecord* to,
                                             ConversionContext ctx,
                                             Symbol** out_templ,
                                             Vector** out_args) {
  if (str == NULL) {
    return false;
  }
  Vector candidates;
  VectorInit(&candidates);
  CollectConversionOperators(str, &candidates);

  Symbol* chosen = NULL;
  Vector* chosen_args = NULL;
  bool ambiguous = false;
  for (size_t i = 0; i < candidates.length; i++) {
    StructMember* member = candidates.value.p[i];
    Symbol* templ = member->symbol;
    if (!templ->flags.is_template ||
        !ConversionOperatorAllowedInContext(templ->type, to, ctx)) {
      continue;
    }
    Vector* args = TypeDeduceConversionOperatorTemplateArguments(
        &compiler->syntax, templ, to);
    if (args == NULL) {
      continue;
    }
    if (chosen == NULL) {
      chosen = templ;
      chosen_args = args;
      continue;
    }
    // A second viable template conversion operator: prefer the more specialized
    // one by partial ordering ([temp.func.order]); if neither is more
    // specialized the conversion is ambiguous.
    int order = TypeConversionOperatorTemplateMoreSpecialized(&compiler->syntax,
                                                              templ, chosen);
    VectorDeleteWithContents(
        order > 0 ? chosen_args : args,
        (VectorElementDestructor)TemplateArgumentDelete, /*free_element=*/false);
    if (order > 0) {
      chosen = templ;
      chosen_args = args;
      ambiguous = false;
    } else if (order == 0) {
      ambiguous = true;
    }
  }
  VectorDestruct(&candidates);
  if (chosen == NULL || ambiguous) {
    if (chosen_args != NULL) {
      VectorDeleteWithContents(chosen_args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return false;
  }
  *out_templ = chosen;
  *out_args = chosen_args;
  return true;
}

static bool TryConvertWithConversionOperator(ASTNode* from, TypeRecord* to,
                                             ConversionContext ctx) {
  if (!CompilerIsCXX() || from == NULL || from->type == NULL ||
      !TypeIsStructOrUnion(from->type)) {
    return false;
  }
  StructMember* member =
      FindConversionOperator(from->type->info.struct_info, to, ctx);
  // Explicit template arguments to attach to the member-access name when the
  // selected operator is a conversion operator template; NULL otherwise.
  Vector* template_args = NULL;
  String member_lookup_name;
  StringInit(&member_lookup_name, NULL);
  if (member != NULL) {
    StringSet(&member_lookup_name, member->symbol->name.value);
  } else {
    Symbol* templ = NULL;
    if (!SelectConversionOperatorTemplate(from->type->info.struct_info, to, ctx,
                                          &templ, &template_args)) {
      StringDestruct(&member_lookup_name);
      return false;
    }
    StringSet(&member_lookup_name, templ->name.value);
  }
  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  ASTNode* receiver = ASTNodeMove(from);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member_lookup_name.value), NULL,
                               from->location);
  StringDestruct(&member_lookup_name);
  // The member-access analysis moves these explicit template arguments onto the
  // resolved member node, where overload resolution uses them to instantiate
  // the conversion operator template (it cannot deduce them from a zero-argument
  // call).  Ownership transfers to the name node, which frees them on deletion.
  ((ConstantASTNode*)member_name)->template_arguments = template_args;
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, from->location, receiver,
                       member_name);
  ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, from->location,
                                   member_access, NewVector());
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  call = AnalyzeExpression(call);
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, false);
    // The selected operator may yield a type that only reaches `to` through a
    // trailing standard conversion (e.g. `operator long` used where an `int`
    // is wanted, or `operator int` in a contextual-bool position).  Apply it
    // now; the result is a scalar, so this cannot recurse back into the
    // conversion-operator search.  Reference targets are excluded: the operator
    // was chosen by exact match and its result is an lvalue that binds directly
    // (its expression type is the referent, not the reference itself).
    if (call != NULL && call->type != NULL && !TypeIsReference(to) &&
        !TypeEqual(call->type, to)) {
      SemanticConvertType(call, to, ctx);
    }
  }
  return true;
}

void SemanticConvertType(ASTNode* from, TypeRecord* to, ConversionContext ctx) {
  // [over.over]/[temp.deduct.funcaddr]: a function-template name or overload set
  // used where a specific function pointer is required resolves to the unique
  // matching specialization.  Do this first so the rewritten (concrete) operand
  // then flows through the normal function-to-pointer conversion below.
  if (CXXTryResolveFunctionAddressNode(from, to)) {
    if (TypeEqual(from->type, to)) {
      return;
    }
  }

  if (TryConvertDerivedPointer(from, to)) {
    return;
  }

  if (TypeIsMemberPointer(from->type) && TypeIsMemberPointer(to)) {
    bool rejects_virtual = false;
    bool is_cast = ctx == kConvertCast;
    if (!MemberPointerCanConvert(from->type, to, is_cast, &rejects_virtual)) {
      if (rejects_virtual) {
        SemanticTypeConversionError(
            from, to,
            "Pointer-to-member conversion through virtual base is not allowed");
      } else {
        SemanticTypeConversionError(
            from, to, "Illegal conversion; cannot convert from '%s' to '%s'");
      }
      return;
    }
    if (TypeEqual(from->type, to)) {
      return;
    }
    ASTNode* parent = from->parent;
    int child_id = from->child_id;
    ASTNode* converted =
        MemberPointerBuildConversion(from, from->type, to, is_cast);
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, child_id, converted, false);
    }
    return;
  }

  // If the types are already equal we do nothing.
  if (TypeEqual(from->type, to)) {
    return;
  }

  // The C++23 fixed-width extended types are distinct types, but conversions
  // to and from the standard type with the same IEC 60559 representation do
  // not require an IR operation.
  if ((TypeUsesFloat32Representation(from->type) &&
       TypeUsesFloat32Representation(to)) ||
      (TypeHasDoubleRepresentationAndRank(from->type) &&
       TypeHasDoubleRepresentationAndRank(to))) {
    ASTNodeSetType(from, to);
    return;
  }

  if (TypeIsIntegral(from->type) && TypeIsIntegral(to) &&
      (TypeIsBitInt(from->type) || TypeIsBitInt(to))) {
    if (from->op == AST_OP(number)) {
      ConvertIntConstantToType((ConstantASTNode*)from, to);
      ASTNodeSetType(from, to);
      return;
    }
    ASTOpcode op = TypeIsBool(to)
                       ? AST_OP(ll2b)
                       : (from->type->size <= to->size ? AST_OP(c2ll)
                                                       : AST_OP(i2c));
    ASTNode* parent = from->parent;
    int child_id = from->child_id;
    if (parent == NULL) {
      ASTNodeSetType(from, to);
      return;
    }
    TypeRecordIncRef(to);
    ASTNode* converted = NewUnaryASTNode(op, to, from->location, from);
    ASTNodeReplaceChild(parent, child_id, converted, false);
    return;
  }

  // A user-defined conversion operator must be honored before any "compatible
  // layout" shortcut: two distinct class types can be equal-ignoring-sign (the
  // sign bits are meaningless for aggregates, so any two structs/unions compare
  // equal there), and reinterpreting one as the other would silently bypass a
  // value-changing conversion operator (e.g. Celsius -> Fahrenheit).  Only fall
  // back to the reinterpret when no conversion operator applies, which keeps the
  // existing behavior for derived-to-base value conversions and for distinct
  // re-instantiations of the same class template.
  if (TryConvertWithConversionOperator(from, to, ctx)) {
    return;
  }

  // A class target may also be reached by an implicit user-defined conversion
  // through a converting constructor (e.g. `S s = 5;` or passing `5` where an
  // `S` is expected).  Try this before the layout-compatible reinterpret below
  // so a real constructor is honored.
  if (TryConvertWithConvertingConstructor(from, to, ctx)) {
    return;
  }

  if (CXXStructLayoutCompatibleShortcut(from->type, to)) {
    // Use the 'to' type as the node type.
    ASTNodeSetType(from, to);
    return;
  }

  if (TypeIsNullPointer(from->type) && TypeIsPointer(to)) {
    ASTNodeSetType(from, to);
    return;
  }
  if (TypeIsNullPointer(from->type) && TypeIsBool(to)) {
    ASTNodeSetType(from, to);
    return;
  }

  if (CXXForbidsImplicitVoidPointerConversion(from->type, to, ctx)) {
    SemanticTypeConversionError(
        from, to, "Illegal conversion; cannot convert from '%s' to '%s'");
    return;
  }

  // Search the type conversion table for matching types.
  for (int i = 0; i < NUM_TYPE_CONVERSIONS; i++) {
    if (type_conversions[i].from(from->type) && type_conversions[i].to(to)) {
      if (type_conversions[i].op == AST_OP(bad)) {
        // Nothing to do.
        return;
      }
      if (type_conversions[i].warning) {
        SemanticTypeConversionWarning(from, to, "conversion",
                                      "Dangerous type conversion "
                                      "from '%s' to '%s'");
      }
      // Convert constants at compile time if possible.
      ASTNode* converted_node =
          ConvertPotentialConstant(from, to, type_conversions[i].op);
      if (converted_node != NULL) {
        ASTNodeSetType(converted_node, to);
        return;
      }
      ASTNode* parent = from->parent;
      int child_id = from->child_id;
      TypeRecordIncRef(to);
      converted_node =
          NewUnaryASTNode(type_conversions[i].op, to, from->location, from);
      ASTNodeReplaceChild(parent, child_id, converted_node, false);
      return;
    }
  }

  switch (ctx) {
    case kConvertCast:
      if (!CompilerIsCXX() && TypeIsNullPointer(from->type) &&
          TypeIsVoid(to)) {
        SemanticTypeConversionError(from, to,
                                    "Illegal cast from '%s' to '%s'");
      } else if (TypeIsVoid(to)) {
         // Casting to void is always allowed.
       } else {
         // Convert the expression to the given type.
         // This is different from a normal conversion in that there are
         // very few illegal casts.
         // Illegal casts:
         // 1. struct/union to/from anything
         // 2. void to anything but void
         bool bad_cast = false;
         if (!CompilerIsCXX() && TypeIsNullPointer(from->type) &&
             !TypeIsPointer(to) && !TypeIsBool(to) &&
             !TypeIsNullPointer(to)) {
           bad_cast = true;
         } else if (TypeIsStructOrUnion(from->type) ||
             TypeIsStructOrUnion(to)) {
           bad_cast = true;
         } else if (TypeIsVoid(from->type)) {
           bad_cast = true;
         }
         if (bad_cast) {
           SemanticTypeConversionError(from, to,
                                       "Illegal cast from '%s' to '%s'");
         }
       }
      break;
      
    case kConvertContextualBool:
    case kConvertNormal:
      if (TypeIsVoidPointer(to)) {
        // Can convert any pointer, array or function to void*.
        if (TypeIsFunction(from->type) || TypeIsPointerOrArray(from->type)) {
          return;
        }
        // The only integer we can convert to void* is NULL.
        if (NodeIsZero(from)) {
          return;
        }
      }
      if (TypeIsPointerOrArray(from->type) && TypeIsPointerOrArray(to)) {
        if (CompilerIsCXX() &&
            TypeChar8IdentityDiffers(from->type, to)) {
          SemanticTypeConversionError(from, to,
                                      "Illegal pointer conversion; "
                                      "from '%s' to '%s'");
          return;
        }
        if (TypeDiscardsQualifiers(from->type->next, to->next)) {
          SemanticTypeConversionWarning(from, to, "discarded-qualifiers",
                                        "Pointer conversion discards qualifiers; "
                                        "from '%s' to '%s'");
        }
        if (PointerSignednessDiffers(from->type, to)) {
          SemanticTypeConversionWarning(from, to, "pointer-sign",
                                        "Pointer targets differ in signedness; "
                                        "from '%s' to '%s'");
        }
        if (!TypeAssignmentCompatible(from->type, to)) {
          SemanticTypeConversionWarning(from, to, "incompatible-pointer-types",
                                        "Illegal pointer conversion; "
                                        "from '%s' to '%s'");
        }
        return;
      }
      
      // Pointers to int or bool is fine.
      if (TypeIsPointerOrArray(from->type) && (TypeIsInt(to) || TypeIsBool(to))) {
        SemanticTypeConversionWarning(from, to, "int-conversion",
                                      "Pointer to integer conversion "
                                      "from '%s' to '%s'");
        return;
      }
      
      // Unscoped enums behave like integers. C++ scoped enums require an
      // explicit cast.
      if ((TypeIsEnum(from->type) && !TypeIsScopedEnum(from->type) &&
           TypeIsInt(to)) ||
          (TypeIsInt(from->type) && TypeIsEnum(to) &&
           !TypeIsScopedEnum(to))) {
        return;
      }

      // Allow the number 0 (explicitly) to be converted to a pointer.
      if (NodeIsZero(from) && TypeIsPointer(to)) {
          return;
      }

      if (NodeIsZero(from) && TypeIsMemberPointer(to)) {
        MemberPointerValue null_value;
        MemberPointerEncodeNull(to, &null_value);
        ASTNode* parent = from->parent;
        int child_id = from->child_id;
        ASTNode* converted =
            NewIntConstantASTNode(null_value.ptr, to, from->location);
        ASTNodeReplaceChild(parent, child_id, converted, false);
        return;
      }

      if (TypeIsMemberPointer(from->type) && TypeIsMemberPointer(to) &&
          MemberPointerCanConvert(from->type, to, false, NULL)) {
        return;
      }

      if (TypeIsFunction(from->type) && TypeIsFunctionPointer(to)) {
        // Functions can be converted to function pointers to the same type.
        if (TypeEqual(from->type, to->next)) {
          return;
        }
      }

      if (TypeIsUnknown(to) || TypeIsUnknown(from->type)) {
        // Unknown types don't cause errors.
        return;
      }

      if (TypeIsScopedEnum(to) || TypeIsScopedEnum(from->type)) {
        if (TypeIsScopedEnum(to) && TypeIsScopedEnum(from->type) &&
            to->info.enum_info == from->type->info.enum_info) {
          return;
        }
        SemanticTypeConversionError(
            from, to, "Illegal conversion; cannot convert from '%s' to '%s'");
        return;
      }
      
      if (TypeIsIntegral(to) && TypeIsIntegral(from->type)) {
        return;
      }
      
      SemanticTypeConversionError(
          from, to, "Illegal conversion; cannot convert from '%s' to '%s'");
      break;
  }
}

void SemanticAnalyzeVariableDefinition(Syntax* syntax,
                                       VariableDeclarationASTNode* node) {
  if (TypeIsVoid(node->symbol->type)) {
    SemanticError((ASTNode*)node, "Cannot define a variable with void type");
  }
  if (node->initializer == NULL) {
    return;
  }
  bool object_initializer =
      (TypeIsFixedArray(node->symbol->type) ||
       TypeIsStructOrUnion(node->symbol->type)) &&
      (node->initializer->op == AST_OP(braced_init) ||
       node->initializer->op == AST_OP(call) ||
       (node->initializer->op == AST_OP(expr_init) &&
        (!TypeIsStructOrUnion(node->symbol->type) ||
         node->symbol->type->info.struct_info !=
             compiler->current_class_access_context) &&
        ((ExpressionInitializerASTNode*)node->initializer)->expr != NULL &&
        ((ExpressionInitializerASTNode*)node->initializer)->expr->op ==
            AST_OP(call)));
  if (!object_initializer) {
    node->initializer = AnalyzeExpression(node->initializer);
    if (node->initializer != NULL &&
        node->initializer->op == AST_OP(call)) {
      VectorASTNode* call = (VectorASTNode*)node->initializer;
      if (call->left != NULL && call->left->op == AST_OP(identifier)) {
        Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
        object_initializer =
            callee != NULL && callee->type != NULL &&
            TypeIsFunction(callee->type) &&
            callee->type->info.function.is_constructor;
      }
    }
  }
  if (!object_initializer) {
    if (!SemanticDeduceAutoType(node->symbol, node->initializer,
                                (ASTNode*)node)) {
      return;
    }
    if (TypeContainsAuto(node->symbol->type)) {
      return;
    }
    if (!TypeIsReflection(node->symbol->type)) {
      NormalConversion(node->initializer, node->symbol->type);
    }
  }
  if (TypeIsConstevalOnly(node->symbol->type) &&
      !TypeContainsTemplateParameter(node->symbol->type) &&
      !node->symbol->flags.is_constexpr) {
    SemanticError((ASTNode*)node,
                  "A variable of consteval-only reflection type must be "
                  "declared constexpr");
  }
  if (TypeIsReflection(node->symbol->type)) {
    ASTNode* reflection_expr =
        ConstexprInitializerExpression(node->initializer);
    reflection_expr = AnalyzeExpression(reflection_expr);
    ReflectionValue* reflection =
        SemanticEvaluateReflection(reflection_expr);
    if (reflection != NULL) {
      node->symbol->value.other = reflection;
      node->symbol->flags.value_set = true;
    }
  }
  if (TypeIsConst(node->symbol->type) || node->symbol->flags.is_constexpr ||
      node->symbol->flags.is_constinit) {
    bool scalar = TypeIsReflection(node->symbol->type) &&
                          node->symbol->flags.value_set
                      ? true
                      : EvaluateScalarConstantForSymbol(node->symbol,
                                                        node->initializer);
    if (!scalar && TypeIsPointer(node->symbol->type)) {
      scalar = SemanticEvaluatePointerConstantForSymbol(node->symbol,
                                                        node->initializer);
    }
    bool ordinary_automatic_const =
        node->symbol->flags.is_local &&
        !StorageIs(node->symbol->storage, STO(static) | STO(thread)) &&
        !node->symbol->flags.is_constexpr &&
        !node->symbol->flags.is_constinit;
    if (!scalar && !ordinary_automatic_const) {
      if (ASTNodeAny(node->initializer,
                     SemanticExpressionRequiresASTConstexpr, NULL)) {
        node->symbol->requires_ast_constexpr = true;
      }
      ConstexprEvaluateObjectConstantForSymbol(node->symbol,
                                               node->initializer);
    }
  }
  if (TypeIsMemberPointer(node->symbol->type)) {
    MemberPointerValue value;
    if (MemberPointerTryEvaluateConstant(node->initializer, node->symbol->type,
                                         &value)) {
      StructMember* member =
          MemberPointerReferencedMember(node->initializer);
      if (member != NULL) {
        node->symbol->value.other = member;
      } else {
        node->symbol->value.ivalue = value.ptr;
      }
      node->symbol->flags.value_set = true;
    }
  }
  if ((node->symbol->flags.is_constexpr || node->symbol->flags.is_constinit) &&
      !node->symbol->flags.value_set &&
      !node->symbol->is_constexpr_representable &&
      !ExpressionIsTemplateDependent(node->initializer)) {
    SemanticError(node->initializer,
                  node->symbol->flags.is_constinit
                      ? "constinit variable initializer is not a constant expression"
                      : "constexpr variable initializer is not a constant expression");
  }
  ASTNodeSetType((ASTNode*)node, node->symbol->type);
}
