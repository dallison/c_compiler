//
//  concepts.c
//  c_compiler
//
//  C++20 concepts and constraints.
//
//  Phase 0 provides the data model and its heap-owned constructors.  Parsing
//  (Phase 2), satisfaction checking (Phase 3) and constraint-failure
//  diagnostics (Phase 4) are layered on top of these types.
//

#include "concepts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "compiler.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "set.h"
#include "syntax.h"
#include "type.h"
#include "type_template.h"

static void ReportConstraintFailure(ConstraintExpr* constraint, Vector* arguments);
static bool ConstraintSubsumesWithMapping(ConstraintExpr* stronger,
                                          ConstraintExpr* weaker,
                                          Vector* parameter_mapping);

static ConstraintExpr* NewConstraintExpr(ConstraintExprKind kind,
                                         SourceLocation location) {
  ConstraintExpr* c = malloc(sizeof(ConstraintExpr));
  memset(c, 0, sizeof(*c));
  c->kind = kind;
  c->location = location;
  return c;
}

ConstraintExpr* NewAtomicConstraint(ASTNode* expr, SourceLocation location) {
  ConstraintExpr* c = NewConstraintExpr(kConstraintAtomic, location);
  c->as.atomic.expr = expr;
  return c;
}

ConstraintExpr* NewConjunctionConstraint(ConstraintExpr* left,
                                         ConstraintExpr* right,
                                         SourceLocation location) {
  ConstraintExpr* c = NewConstraintExpr(kConstraintConjunction, location);
  c->as.binary.left = left;
  c->as.binary.right = right;
  return c;
}

ConstraintExpr* NewDisjunctionConstraint(ConstraintExpr* left,
                                         ConstraintExpr* right,
                                         SourceLocation location) {
  ConstraintExpr* c = NewConstraintExpr(kConstraintDisjunction, location);
  c->as.binary.left = left;
  c->as.binary.right = right;
  return c;
}

ConstraintExpr* NewConceptIdConstraint(Symbol* concept_symbol,
                                       Vector* arguments,
                                       SourceLocation location) {
  ConstraintExpr* c = NewConstraintExpr(kConstraintConceptId, location);
  c->as.concept_id.concept_symbol = concept_symbol;
  c->as.concept_id.arguments = arguments;
  return c;
}

ConstraintExpr* NewRequiresConstraint(RequiresExpr* requires_expr,
                                      SourceLocation location) {
  ConstraintExpr* c = NewConstraintExpr(kConstraintRequires, location);
  c->as.requires_.requires_expr = requires_expr;
  return c;
}

static Requirement* NewRequirement(RequirementKind kind,
                                   SourceLocation location) {
  Requirement* r = malloc(sizeof(Requirement));
  memset(r, 0, sizeof(*r));
  r->kind = kind;
  r->location = location;
  return r;
}

Requirement* NewSimpleRequirement(ASTNode* expr, SourceLocation location) {
  Requirement* r = NewRequirement(kRequirementSimple, location);
  r->expr = expr;
  return r;
}

Requirement* NewTypeRequirement(TypeRecord* type, SourceLocation location) {
  Requirement* r = NewRequirement(kRequirementType, location);
  r->type = type;
  return r;
}

Requirement* NewCompoundRequirement(ASTNode* expr, bool is_noexcept,
                                    ConstraintExpr* return_type_constraint,
                                    SourceLocation location) {
  Requirement* r = NewRequirement(kRequirementCompound, location);
  r->expr = expr;
  r->is_noexcept = is_noexcept;
  r->return_type_constraint = return_type_constraint;
  return r;
}

Requirement* NewNestedRequirement(ConstraintExpr* nested,
                                  SourceLocation location) {
  Requirement* r = NewRequirement(kRequirementNested, location);
  r->nested = nested;
  return r;
}

RequiresExpr* NewRequiresExpr(Vector* parameters, Vector* requirements,
                              SourceLocation location) {
  RequiresExpr* e = malloc(sizeof(RequiresExpr));
  memset(e, 0, sizeof(*e));
  e->parameters = parameters;
  e->requirements = requirements;
  e->location = location;
  return e;
}

Concept* NewConcept(const char* name, Vector* template_parameters,
                    ConstraintExpr* constraint, SourceLocation location) {
  Concept* c = malloc(sizeof(Concept));
  memset(c, 0, sizeof(*c));
  StringInit(&c->name, name);
  c->template_parameters = template_parameters;
  c->constraint = constraint;
  c->location = location;
  return c;
}

void ConstraintExprDelete(ConstraintExpr* constraint) {
  if (constraint == NULL) {
    return;
  }
  switch (constraint->kind) {
    case kConstraintConjunction:
    case kConstraintDisjunction:
      ConstraintExprDelete(constraint->as.binary.left);
      ConstraintExprDelete(constraint->as.binary.right);
      break;
    case kConstraintConceptId:
      if (constraint->as.concept_id.arguments != NULL) {
        VectorDeleteWithContents(
            constraint->as.concept_id.arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      break;
    case kConstraintRequires:
      RequiresExprDelete(constraint->as.requires_.requires_expr);
      break;
    case kConstraintAtomic:
      break;
  }
  free(constraint);
}

void RequirementDelete(Requirement* requirement) {
  if (requirement == NULL) {
    return;
  }
  if (requirement->type != NULL) {
    TypeRecordDelete(requirement->type);
  }
  ConstraintExprDelete(requirement->return_type_constraint);
  ConstraintExprDelete(requirement->nested);
  free(requirement);
}

void RequiresExprDelete(RequiresExpr* expr) {
  if (expr == NULL) {
    return;
  }
  if (expr->parameters != NULL) {
    VectorDelete(expr->parameters);
  }
  if (expr->requirements != NULL) {
    VectorDeleteWithContents(expr->requirements,
                             (VectorElementDestructor)RequirementDelete,
                             /*free_element=*/false);
  }
  free(expr);
}

void ConceptDelete(Concept* concept) {
  if (concept == NULL) {
    return;
  }
  StringDestruct(&concept->name);
  if (concept->template_parameters != NULL) {
    VectorDeleteWithContents(
        concept->template_parameters,
        (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
  }
  ConstraintExprDelete(concept->constraint);
  free(concept);
}

static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static Requirement* RequirementClone(Requirement* requirement);
static RequiresExpr* RequiresExprClone(RequiresExpr* expr);

ConstraintExpr* ConceptsCloneConstraint(ConstraintExpr* constraint) {
  if (constraint == NULL) {
    return NULL;
  }
  switch (constraint->kind) {
    case kConstraintAtomic:
      return NewAtomicConstraint(
          ASTNodeClone(constraint->as.atomic.expr, IdentityCloneNode, NULL,
                       NULL),
          constraint->location);
    case kConstraintConjunction:
      return NewConjunctionConstraint(
          ConceptsCloneConstraint(constraint->as.binary.left),
          ConceptsCloneConstraint(constraint->as.binary.right),
          constraint->location);
    case kConstraintDisjunction:
      return NewDisjunctionConstraint(
          ConceptsCloneConstraint(constraint->as.binary.left),
          ConceptsCloneConstraint(constraint->as.binary.right),
          constraint->location);
    case kConstraintConceptId:
      return NewConceptIdConstraint(
          constraint->as.concept_id.concept_symbol,
          TemplateArgumentVectorCopy(constraint->as.concept_id.arguments),
          constraint->location);
    case kConstraintRequires:
      return NewRequiresConstraint(
          RequiresExprClone(constraint->as.requires_.requires_expr),
          constraint->location);
  }
  return NULL;
}

static Requirement* RequirementClone(Requirement* requirement) {
  if (requirement == NULL) {
    return NULL;
  }
  switch (requirement->kind) {
    case kRequirementSimple:
      return NewSimpleRequirement(
          ASTNodeClone(requirement->expr, IdentityCloneNode, NULL, NULL),
          requirement->location);
    case kRequirementType:
      return NewTypeRequirement(TypeRecordCopy(requirement->type),
                                requirement->location);
    case kRequirementCompound:
      return NewCompoundRequirement(
          ASTNodeClone(requirement->expr, IdentityCloneNode, NULL, NULL),
          requirement->is_noexcept,
          ConceptsCloneConstraint(requirement->return_type_constraint),
          requirement->location);
    case kRequirementNested:
      return NewNestedRequirement(ConceptsCloneConstraint(requirement->nested),
                                  requirement->location);
  }
  return NULL;
}

static RequiresExpr* RequiresExprClone(RequiresExpr* expr) {
  if (expr == NULL) {
    return NULL;
  }
  Vector* parameters = NewVector();
  for (size_t i = 0; expr->parameters != NULL &&
                     i < expr->parameters->length; i++) {
    VectorAppend(parameters, expr->parameters->value.p[i]);
  }
  Vector* requirements = NewVector();
  for (size_t i = 0; expr->requirements != NULL &&
                     i < expr->requirements->length; i++) {
    VectorAppend(requirements,
                 RequirementClone(expr->requirements->value.p[i]));
  }
  return NewRequiresExpr(parameters, requirements, expr->location);
}

static bool TemplateArgumentContainsTemplateParameter(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type)) {
    return true;
  }
  for (size_t i = 0; arg->pack_arguments != NULL &&
                     i < arg->pack_arguments->length; i++) {
    if (TemplateArgumentContainsTemplateParameter(
            arg->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameter(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

static void ExpressionContainsTemplateParameterVisitor(ASTNode* node,
                                                       void* data,
                                                       int child_id,
                                                       VisitorMode mode) {
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren || *(bool*)data) {
    return;
  }
  if (TypeContainsTemplateParameter(node->type)) {
    *(bool*)data = true;
    return;
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL &&
        (id->symbol->template_parameter_index >= 0 ||
         TypeContainsTemplateParameter(id->symbol->type) ||
         TemplateArgumentVectorContainsTemplateParameter(
             id->template_arguments))) {
      *(bool*)data = true;
    }
  }
}

static bool ExpressionContainsTemplateParameter(ASTNode* expr) {
  bool contains = false;
  ASTNodeVisit(expr, ExpressionContainsTemplateParameterVisitor, 0,
               &contains);
  return contains;
}

static bool RequirementContainsTemplateParameter(Requirement* requirement);

bool ConceptsConstraintContainsTemplateParameter(ConstraintExpr* constraint) {
  if (constraint == NULL) {
    return false;
  }
  switch (constraint->kind) {
    case kConstraintAtomic:
      return ExpressionContainsTemplateParameter(constraint->as.atomic.expr);
    case kConstraintConjunction:
    case kConstraintDisjunction:
      return ConceptsConstraintContainsTemplateParameter(
                 constraint->as.binary.left) ||
             ConceptsConstraintContainsTemplateParameter(
                 constraint->as.binary.right);
    case kConstraintConceptId:
      return TemplateArgumentVectorContainsTemplateParameter(
          constraint->as.concept_id.arguments);
    case kConstraintRequires: {
      RequiresExpr* expr = constraint->as.requires_.requires_expr;
      for (size_t i = 0; expr != NULL && expr->requirements != NULL &&
                         i < expr->requirements->length; i++) {
        if (RequirementContainsTemplateParameter(
                expr->requirements->value.p[i])) {
          return true;
        }
      }
      return false;
    }
  }
  return false;
}

static bool RequirementContainsTemplateParameter(Requirement* requirement) {
  if (requirement == NULL) {
    return false;
  }
  switch (requirement->kind) {
    case kRequirementSimple:
    case kRequirementCompound:
      return ExpressionContainsTemplateParameter(requirement->expr) ||
             ConceptsConstraintContainsTemplateParameter(
                 requirement->return_type_constraint);
    case kRequirementType:
      return TypeContainsTemplateParameter(requirement->type);
    case kRequirementNested:
      return ConceptsConstraintContainsTemplateParameter(requirement->nested);
  }
  return false;
}

static bool EvaluateConceptDefinitionInteger(Symbol* concept_symbol,
                                             Vector* arguments,
                                             SourceLocation location,
                                             int64_t* result);
static void AppendTemplateArgumentDescription(String* out, Concept* concept,
                                              Vector* arguments);

typedef enum {
  kAtomicConstraintOk,
  kAtomicConstraintSubstitutionFailed,
  kAtomicConstraintNotBool,
  kAtomicConstraintNotConstant,
  kAtomicConstraintFalse,
} AtomicConstraintResult;

static TypeRecord* ConstraintExpressionPrvalueType(ASTNode* expr) {
  if (expr == NULL || expr->type == NULL) {
    return NULL;
  }
  if (expr->value_category == kValueCategoryLvalue ||
      expr->value_category == kValueCategoryXvalue) {
    TypeRecord* type = expr->type;
    if (TypeIsReference(type)) {
      return type->next;
    }
    return type;
  }
  return expr->type;
}

static TypeRecord* CompoundRequirementDecltypeType(ASTNode* expr) {
  if (expr == NULL || expr->type == NULL) {
    return NULL;
  }
  if (expr->value_category == kValueCategoryLvalue ||
      expr->value_category == kValueCategoryXvalue) {
    TypeRecord* base =
        TypeIsReference(expr->type) ? TypeRecordCopy(expr->type->next)
                                    : TypeRecordCopy(expr->type);
    if (base == NULL) {
      return NULL;
    }
    TypeRecord* ref = NewReferenceTypeRecord(
        kQualPlain, expr->value_category == kValueCategoryXvalue);
    TypeRecordChain(ref, base);
    TypeRecordDelete(base);
    ref->type = base->type;
    TypeRecordCalculateSize(ref);
    return ref;
  }
  return TypeRecordCopy(expr->type);
}

static void ClearRequirementExpressionAnalysis(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode);

static AtomicConstraintResult EvaluateAtomicConstraint(ASTNode* expr,
                                                       Vector* arguments,
                                                       SourceLocation location,
                                                       int64_t* result) {
  if (expr == NULL) {
    return kAtomicConstraintSubstitutionFailed;
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  ASTNode* evaluated = NULL;
  if (arguments != NULL) {
    evaluated = TypeSubstituteTemplateExpression(&compiler->syntax, expr,
                                                 arguments, location);
    if (evaluated == NULL || DiagnosticErrorTrapped()) {
      DiagnosticErrorTrapEnd(saved_trap);
      return kAtomicConstraintSubstitutionFailed;
    }
  } else {
    evaluated = ASTNodeClone(expr, IdentityCloneNode, NULL, NULL);
    if (evaluated == NULL) {
      DiagnosticErrorTrapEnd(saved_trap);
      return kAtomicConstraintSubstitutionFailed;
    }
  }
  ASTNodeVisit(evaluated, ClearRequirementExpressionAnalysis, 0, NULL);
  DiagnosticSuppressBegin();
  evaluated = AnalyzeExpression(evaluated);
  bool trapped = evaluated == NULL || DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  if (trapped) {
    ASTNodeDelete(evaluated);
    DiagnosticErrorTrapEnd(saved_trap);
    return kAtomicConstraintSubstitutionFailed;
  }
  TypeRecord* prvalue_type = ConstraintExpressionPrvalueType(evaluated);
  if (prvalue_type == NULL || !TypeIsBool(prvalue_type)) {
    ASTNodeDelete(evaluated);
    DiagnosticErrorTrapEnd(saved_trap);
    return kAtomicConstraintNotBool;
  }
  int64_t value = 0;
  if (!EvaluateIntegerExpression(evaluated, &value)) {
    ASTNodeDelete(evaluated);
    DiagnosticErrorTrapEnd(saved_trap);
    return kAtomicConstraintNotConstant;
  }
  ASTNodeDelete(evaluated);
  DiagnosticErrorTrapEnd(saved_trap);
  if (result != NULL) {
    *result = value;
  }
  return value != 0 ? kAtomicConstraintOk : kAtomicConstraintFalse;
}

static bool EvaluateAtomicConstraintInteger(ASTNode* expr, Vector* arguments,
                                            SourceLocation location,
                                            int64_t* result) {
  AtomicConstraintResult status =
      EvaluateAtomicConstraint(expr, arguments, location, result);
  return status == kAtomicConstraintOk || status == kAtomicConstraintFalse;
}

static const char* AtomicConstraintFailureMessage(AtomicConstraintResult reason) {
  switch (reason) {
    case kAtomicConstraintNotBool:
      return "because this constraint expression is not a prvalue constant "
             "expression of type bool";
    case kAtomicConstraintNotConstant:
      return "because this constraint expression is not a constant expression "
             "of type bool";
    case kAtomicConstraintFalse:
      return "because this constraint expression evaluated to false";
    case kAtomicConstraintSubstitutionFailed:
    case kAtomicConstraintOk:
      return "because this constraint expression was not satisfied";
  }
  return "because this constraint expression was not satisfied";
}

static bool CalleeTypeIsNoexcept(TypeRecord* callee_type) {
  if (callee_type != NULL && TypeIsPointer(callee_type)) {
    callee_type = callee_type->next;
  }
  return callee_type != NULL && TypeIsFunction(callee_type) &&
         callee_type->info.function.is_noexcept;
}

static void ExpressionPotentiallyThrowsVisitor(ASTNode* node, void* data,
                                               int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  bool* throws = (bool*)data;
  if (*throws) {
    return;
  }
  switch (node->op) {
    case AST_OP(throw):
      *throws = true;
      break;
    case AST_OP(call): {
      // A call potentially throws unless the selected callee (function,
      // pointer, member pointer, or an unresolved dependent callee) has a
      // non-throwing exception specification.  Its operands are examined as
      // children of this node during the same traversal, so a struct-returning
      // constructor call is likewise covered by this rule.
      VectorASTNode* call = (VectorASTNode*)node;
      TypeRecord* callee_type = call->left != NULL ? call->left->type : NULL;
      if (!CalleeTypeIsNoexcept(callee_type)) {
        *throws = true;
      }
      break;
    }
    default:
      break;
  }
}

static bool ExpressionPotentiallyThrows(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  bool throws = false;
  ASTNodeVisit(node, ExpressionPotentiallyThrowsVisitor, 0, &throws);
  return throws;
}

static bool ExpressionRequirementNoexceptSatisfied(ASTNode* expr) {
  return !ExpressionPotentiallyThrows(expr);
}

static bool EvaluateReturnTypeRequirement(ConstraintExpr* constraint,
                                          TypeRecord* expr_type,
                                          Vector* arguments) {
  if (constraint == NULL) {
    return true;
  }
  if (expr_type == NULL || constraint->kind != kConstraintConceptId) {
    return false;
  }
  Vector* concrete_args = TypeSubstituteTemplateArgumentVector(
      &compiler->syntax, constraint->as.concept_id.arguments, arguments);
  Vector* full_args = NewVector();
  VectorAppend(full_args, NewTypeTemplateArgument(expr_type));
  if (concrete_args != NULL) {
    for (size_t i = 0; i < concrete_args->length; i++) {
      VectorAppend(full_args, concrete_args->value.p[i]);
    }
    concrete_args->length = 0;
    VectorDelete(concrete_args);
  }
  int64_t value = 0;
  bool ok = EvaluateConceptDefinitionInteger(
      constraint->as.concept_id.concept_symbol, full_args,
      constraint->location, &value);
  VectorDeleteWithContents(full_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return ok && value != 0;
}

typedef enum {
  kRequirementSatisfied,
  kRequirementExpressionInvalid,
  kRequirementNoexceptUnsatisfied,
  kRequirementReturnTypeUnsatisfied,
} RequirementFailureReason;

typedef struct RequirementFailureInfo {
  RequirementFailureReason reason;
  TypeRecord* expression_type;
} RequirementFailureInfo;

static void RequirementFailureInfoInit(RequirementFailureInfo* info) {
  if (info == NULL) {
    return;
  }
  info->reason = kRequirementSatisfied;
  info->expression_type = NULL;
}

static void RequirementFailureInfoDestruct(RequirementFailureInfo* info) {
  if (info == NULL) {
    return;
  }
  TypeRecordDelete(info->expression_type);
  info->expression_type = NULL;
}

static void ClearRequirementExpressionAnalysis(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

static bool EvaluateExpressionRequirement(Requirement* requirement,
                                          RequiresExpr* requires_expr,
                                          Vector* arguments,
                                          RequirementFailureInfo* failure_info) {
  RequirementFailureInfoInit(failure_info);
  Vector saved_types = {0};
  VectorInit(&saved_types);
  if (requires_expr != NULL && requires_expr->parameters != NULL) {
    for (size_t i = 0; i < requires_expr->parameters->length; i++) {
      Symbol* param = requires_expr->parameters->value.p[i];
      if (param == NULL || param->type == NULL) {
        VectorAppend(&saved_types, NULL);
        continue;
      }
      VectorAppend(&saved_types, param->type);
      TypeRecordIncRef(param->type);
      TypeRecord* parameter_pattern = TypeRecordCopy(param->type);
      TypeRecord* concrete =
          TypeSubstituteTemplateType(&compiler->syntax, parameter_pattern,
                                     arguments);
      TypeRecordDelete(parameter_pattern);
      SymbolSetType(param, concrete);
      TypeRecordDelete(concrete);
    }
  }
  SyntaxOpenScope(&compiler->syntax);
  if (requires_expr != NULL && requires_expr->parameters != NULL) {
    for (size_t i = 0; i < requires_expr->parameters->length; i++) {
      Symbol* param = requires_expr->parameters->value.p[i];
      if (param != NULL) {
        SyntaxAddBorrowedSymbol(&compiler->syntax, param);
      }
    }
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  ASTNode* cloned = TypeSubstituteTemplateExpression(
      &compiler->syntax, requirement->expr, arguments,
      requirement->location);
  if (cloned == NULL || DiagnosticErrorTrapped()) {
    DiagnosticErrorTrapEnd(saved_trap);
    SyntaxCloseScope(&compiler->syntax);
    if (requires_expr != NULL && requires_expr->parameters != NULL) {
      for (size_t i = 0; i < requires_expr->parameters->length &&
                         i < saved_types.length; i++) {
        Symbol* param = requires_expr->parameters->value.p[i];
        TypeRecord* saved_type = saved_types.value.p[i];
        if (param != NULL && saved_type != NULL) {
          SymbolSetType(param, saved_type);
          TypeRecordDelete(saved_type);
        }
      }
    }
    VectorDestruct(&saved_types);
    if (failure_info != NULL) {
      failure_info->reason = kRequirementExpressionInvalid;
    }
    return false;
  }
  ASTNodeVisit(cloned, ClearRequirementExpressionAnalysis, 0, NULL);
  DiagnosticSuppressBegin();
  cloned = AnalyzeExpression(cloned);
  bool failed = cloned == NULL || DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  if (failed && failure_info != NULL) {
    failure_info->reason = kRequirementExpressionInvalid;
  }
  if (!failed && requirement->is_noexcept &&
      !ExpressionRequirementNoexceptSatisfied(cloned)) {
    failed = true;
    if (failure_info != NULL) {
      failure_info->reason = kRequirementNoexceptUnsatisfied;
    }
  }
  TypeRecord* decltype_type = NULL;
  if (!failed && requirement->return_type_constraint != NULL) {
    decltype_type = CompoundRequirementDecltypeType(cloned);
    if (!EvaluateReturnTypeRequirement(requirement->return_type_constraint,
                                       decltype_type, arguments)) {
      failed = true;
      if (failure_info != NULL) {
        failure_info->reason = kRequirementReturnTypeUnsatisfied;
        failure_info->expression_type = decltype_type;
        decltype_type = NULL;
      }
    }
  }
  TypeRecordDelete(decltype_type);
  DiagnosticErrorTrapEnd(saved_trap);
  ASTNodeDelete(cloned);
  SyntaxCloseScope(&compiler->syntax);
  if (requires_expr != NULL && requires_expr->parameters != NULL) {
    for (size_t i = 0; i < requires_expr->parameters->length &&
                       i < saved_types.length; i++) {
      Symbol* param = requires_expr->parameters->value.p[i];
      TypeRecord* saved_type = saved_types.value.p[i];
      if (param != NULL && saved_type != NULL) {
        SymbolSetType(param, saved_type);
        TypeRecordDelete(saved_type);
      }
    }
  }
  VectorDestruct(&saved_types);
  return !failed;
}

#if 0  // debug
static void DebugPrintType(const char* label, TypeRecord* type) {
  if (type == NULL) {
    fprintf(stderr, "%s: NULL\n", label);
    return;
  }
  String s;
  StringInit(&s, "");
  TypeRecordToString(type, &s);
  fprintf(stderr, "%s: %s (tpl_idx=%d, has_tpl_param=%d)\n", label, s.value,
          type->template_parameter_index,
          TypeContainsTemplateParameter(type));
  StringDestruct(&s);
}
#endif

static bool EvaluateTypeRequirement(Requirement* requirement,
                                    Vector* arguments) {
  if (requirement == NULL || requirement->type == NULL) {
    return false;
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  TypeRecord* concrete =
      TypeSubstituteTemplateType(&compiler->syntax, requirement->type,
                                 arguments);
  bool failed = concrete == NULL || DiagnosticErrorTrapped() ||
                TypeContainsTemplateParameter(concrete);
  DiagnosticErrorTrapEnd(saved_trap);
  TypeRecordDelete(concrete);
  return !failed;
}

static const char* RequirementFailureReasonMessage(RequirementKind kind,
                                                   RequirementFailureReason reason) {
  switch (reason) {
    case kRequirementExpressionInvalid:
      return kind == kRequirementCompound
                 ? "because this compound requirement expression is invalid after substituting template arguments"
                 : "because this simple requirement expression is invalid after substituting template arguments";
    case kRequirementNoexceptUnsatisfied:
      return "because this compound requirement is not noexcept; the required expression can throw";
    case kRequirementReturnTypeUnsatisfied:
      return "because this compound requirement return type constraint was not satisfied";
    case kRequirementSatisfied:
      return "because this requirement was not satisfied";
  }
  return "because this requirement was not satisfied";
}

static void ReportReturnTypeRequirementDetail(Requirement* requirement,
                                              RequirementFailureInfo* info,
                                              Vector* arguments) {
  if (requirement == NULL || info == NULL ||
      info->reason != kRequirementReturnTypeUnsatisfied ||
      requirement->return_type_constraint == NULL ||
      requirement->return_type_constraint->kind != kConstraintConceptId ||
      info->expression_type == NULL) {
    return;
  }

  ConstraintExpr* constraint = requirement->return_type_constraint;
  Symbol* concept_symbol = constraint->as.concept_id.concept_symbol;
  Concept* concept = concept_symbol != NULL
                         ? concept_symbol->concept_definition
                         : NULL;
  Vector* concrete_args = TypeSubstituteTemplateArgumentVector(
      &compiler->syntax, constraint->as.concept_id.arguments, arguments);
  Vector* full_args = NewVector();
  VectorAppend(full_args, NewTypeTemplateArgument(info->expression_type));
  if (concrete_args != NULL) {
    for (size_t i = 0; i < concrete_args->length; i++) {
      VectorAppend(full_args, concrete_args->value.p[i]);
    }
    concrete_args->length = 0;
    VectorDelete(concrete_args);
  }

  String note;
  StringInit(&note, "because return type ");
  TypeRecordToString(info->expression_type, &note);
  while (note.length > 0 && note.value[note.length - 1] == ' ') {
    note.length--;
    note.value[note.length] = '\0';
  }
  StringAppend(&note, " did not satisfy concept ");
  StringAppend(&note,
               concept_symbol != NULL ? concept_symbol->name.value : "<unknown>");
  AppendTemplateArgumentDescription(&note, concept, full_args);
  SemanticNoteAtLocation(constraint->location, "%s", note.value);
  StringDestruct(&note);

  VectorDeleteWithContents(full_args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
}

static bool EvaluateConstraintInteger(ConstraintExpr* constraint,
                                      Vector* arguments,
                                      int64_t* result) {
  if (constraint == NULL) {
    return false;
  }
  switch (constraint->kind) {
    case kConstraintAtomic:
      return EvaluateAtomicConstraintInteger(constraint->as.atomic.expr,
                                             arguments, constraint->location,
                                             result);
    case kConstraintConjunction: {
      int64_t left = 0;
      if (!EvaluateConstraintInteger(constraint->as.binary.left, arguments,
                                     &left)) {
        return false;
      }
      if (left == 0) {
        *result = 0;
        return true;
      }
      int64_t right = 0;
      if (!EvaluateConstraintInteger(constraint->as.binary.right, arguments,
                                     &right)) {
        return false;
      }
      *result = right != 0;
      return true;
    }
    case kConstraintDisjunction: {
      int64_t left = 0;
      if (!EvaluateConstraintInteger(constraint->as.binary.left, arguments,
                                     &left)) {
        return false;
      }
      if (left != 0) {
        *result = 1;
        return true;
      }
      int64_t right = 0;
      if (!EvaluateConstraintInteger(constraint->as.binary.right, arguments,
                                     &right)) {
        return false;
      }
      *result = right != 0;
      return true;
    }
    case kConstraintConceptId: {
      Vector* concrete_args = TypeSubstituteTemplateArgumentVector(
          &compiler->syntax, constraint->as.concept_id.arguments, arguments);
      bool ok = EvaluateConceptDefinitionInteger(
          constraint->as.concept_id.concept_symbol, concrete_args,
          constraint->location, result);
      if (concrete_args != NULL) {
        VectorDeleteWithContents(
            concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      return ok;
    }
    case kConstraintRequires:
      break;
  }
  RequiresExpr* requires_expr = constraint->as.requires_.requires_expr;
  if (requires_expr == NULL || requires_expr->requirements == NULL) {
    return false;
  }
  for (size_t i = 0; i < requires_expr->requirements->length; i++) {
    Requirement* requirement = requires_expr->requirements->value.p[i];
    if (requirement == NULL) {
      return false;
    }
    switch (requirement->kind) {
      case kRequirementSimple:
      case kRequirementCompound: {
        RequirementFailureInfo failure_info;
        if (EvaluateExpressionRequirement(requirement, requires_expr,
                                          arguments, &failure_info)) {
          RequirementFailureInfoDestruct(&failure_info);
          break;
        }
        RequirementFailureInfoDestruct(&failure_info);
        *result = 0;
        return true;
      }
      case kRequirementType:
        if (EvaluateTypeRequirement(requirement, arguments)) {
          break;
        }
        *result = 0;
        return true;
      case kRequirementNested: {
        int64_t value = 0;
        if (EvaluateConstraintInteger(requirement->nested, arguments, &value) &&
            value != 0) {
          break;
        }
        *result = 0;
        return true;
      }
    }
  }
  *result = 1;
  return true;
}

static bool EvaluateConceptDefinitionInteger(Symbol* concept_symbol,
                                             Vector* arguments,
                                             SourceLocation location,
                                             int64_t* result) {
  if (concept_symbol == NULL || !concept_symbol->flags.is_concept ||
      concept_symbol->concept_definition == NULL) {
    return false;
  }
  Concept* definition = concept_symbol->concept_definition;
  ConstraintExpr* constraint = definition->constraint;
  if (constraint == NULL) {
    return false;
  }

  // Fill in trailing default template arguments (a concept's default arguments
  // may be computed from the earlier ones, e.g. the standard
  // `__comparison_common_type_with_impl<T, U, C = common_type_t<T, U>>`), so
  // the constraint sees a fully-formed argument list.
  Vector* completed = NULL;
  if (arguments != NULL && definition->template_parameters != NULL &&
      arguments->length < definition->template_parameters->length) {
    completed = TypeCompleteConceptArguments(
        &compiler->syntax, definition->template_parameters, arguments);
  }
  Vector* eval_arguments = completed != NULL ? completed : arguments;

  bool ok;
  if (eval_arguments == NULL || constraint->kind != kConstraintAtomic) {
    ok = EvaluateConstraintInteger(constraint, eval_arguments, result);
  } else {
    ok = EvaluateAtomicConstraintInteger(constraint->as.atomic.expr,
                                         eval_arguments, location, result);
  }
  if (completed != NULL) {
    VectorDeleteWithContents(completed,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  return ok;
}

bool ConceptsEvaluateInteger(ASTNode* node, int64_t* result) {
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL || !id->symbol->flags.is_concept ||
      id->symbol->concept_definition == NULL) {
    return false;
  }
  if (TemplateArgumentVectorContainsTemplateParameter(id->template_arguments)) {
    return false;
  }
  bool ok = EvaluateConceptDefinitionInteger(id->symbol, id->template_arguments,
                                          node->location, result);
  return ok;
}

bool ConceptsEvaluateConstraint(ConstraintExpr* constraint, int64_t* result) {
  if (ConceptsConstraintContainsTemplateParameter(constraint)) {
    return false;
  }
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  bool ok = EvaluateConstraintInteger(constraint, NULL, result);
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  return ok;
}

bool ConceptsEvaluateConstraintWithArguments(ConstraintExpr* constraint,
                                             Vector* arguments,
                                             int64_t* result) {
  return EvaluateConstraintInteger(constraint, arguments, result);
}

bool ConceptsConstraintSatisfied(ConstraintExpr* constraint, Vector* arguments) {
  if (!ConceptsHasAssociatedConstraint(constraint)) {
    return true;
  }
  SyntaxOpenScope(&compiler->syntax);
  bool saved_trap = DiagnosticErrorTrapBegin();
  int64_t value = 0;
  bool ok = EvaluateConstraintInteger(constraint, arguments, &value) && value != 0;
  DiagnosticErrorTrapEnd(saved_trap);
  SyntaxCloseScope(&compiler->syntax);
  return ok;
}

bool ConceptsHasAssociatedConstraint(ConstraintExpr* constraint) {
  return constraint != NULL;
}

void ConceptsReportAssociatedConstraintFailure(ConstraintExpr* constraint,
                                               Vector* arguments,
                                               SourceLocation location,
                                               const char* summary) {
  if (!ConceptsHasAssociatedConstraint(constraint)) {
    return;
  }
  if (summary != NULL) {
    SemanticNoteAtLocation(location, "%s", summary);
  }
  ReportConstraintFailure(constraint, arguments);
}

bool ConceptsFunctionTemplateConstraintsSatisfied(Symbol* templ,
                                                 Vector* arguments) {
  if (!ConceptsFunctionTemplateHasAssociatedConstraint(templ)) {
    return true;
  }
  return ConceptsConstraintSatisfied(
      templ->type->info.function.associated_constraint, arguments);
}

bool ConceptsFunctionTemplateHasAssociatedConstraint(Symbol* templ) {
  return templ != NULL && templ->type != NULL && TypeIsFunction(templ->type) &&
         templ->type->info.function.associated_constraint != NULL;
}

static ConstraintExpr* FunctionTemplateConstraint(Symbol* templ) {
  if (!ConceptsFunctionTemplateHasAssociatedConstraint(templ)) {
    return NULL;
  }
  return templ->type->info.function.associated_constraint;
}

// ---------------------------------------------------------------------------
// C++20 constraint normalization and subsumption ([temp.constr.normal],
// [temp.constr.order], [temp.constr.atomic]).
// ---------------------------------------------------------------------------

static TemplateArgument* CopyTemplateArgumentForNormalization(
    TemplateArgument* arg) {
  if (arg == NULL) {
    return NULL;
  }
  TemplateArgument* copy = malloc(sizeof(TemplateArgument));
  memset(copy, 0, sizeof(*copy));
  copy->kind = arg->kind;
  copy->is_pack_expansion = arg->is_pack_expansion;
  copy->type = arg->type != NULL ? TypeRecordCopy(arg->type) : NULL;
  copy->int_value = arg->int_value;
  copy->template_parameter_index = arg->template_parameter_index;
  copy->pack_arguments = TemplateArgumentVectorCopy(arg->pack_arguments);
  copy->dependent_expr = arg->dependent_expr;
  copy->location = arg->location;
  return copy;
}

static bool NormalizationTemplateArgumentsEqual(TemplateArgument* left,
                                                TemplateArgument* right) {
  if (left == NULL || right == NULL || left->kind != right->kind) {
    return left == right;
  }
  if (left->pack_arguments != NULL || right->pack_arguments != NULL) {
    if (left->pack_arguments == NULL || right->pack_arguments == NULL ||
        left->pack_arguments->length != right->pack_arguments->length) {
      return false;
    }
    for (size_t i = 0; i < left->pack_arguments->length; i++) {
      if (!NormalizationTemplateArgumentsEqual(
              left->pack_arguments->value.p[i],
              right->pack_arguments->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  if (left->kind == kTemplateParameterType) {
    return TypeEqual(left->type, right->type) &&
           left->template_parameter_index == right->template_parameter_index;
  }
  return left->int_value == right->int_value &&
         left->template_parameter_index == right->template_parameter_index;
}

typedef struct NormalizedAtomic {
  ASTNode* expr;
  RequiresExpr* requires_expr;
  Vector* parameter_mapping;  // TemplateArgument* owned.
  SourceLocation location;
} NormalizedAtomic;

typedef struct NormalizedClause {
  Vector* atoms;  // NormalizedAtomic* owned.
} NormalizedClause;

typedef struct NormalizedConstraintForm {
  Vector* clauses;  // NormalizedClause* owned.
} NormalizedConstraintForm;

static int SymbolPointerCompare(const void* left, const void* right) {
  if (left < right) {
    return -1;
  }
  if (left > right) {
    return 1;
  }
  return 0;
}

static void NormalizedAtomicDelete(NormalizedAtomic* atom) {
  if (atom == NULL) {
    return;
  }
  if (atom->parameter_mapping != NULL) {
    VectorDeleteWithContents(atom->parameter_mapping,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  free(atom);
}

static void NormalizedClauseDelete(NormalizedClause* clause) {
  if (clause == NULL) {
    return;
  }
  if (clause->atoms != NULL) {
    VectorDeleteWithContents(clause->atoms,
                             (VectorElementDestructor)NormalizedAtomicDelete,
                             /*free_element=*/false);
  }
  free(clause);
}

static void NormalizedConstraintFormDelete(NormalizedConstraintForm* form) {
  if (form == NULL) {
    return;
  }
  if (form->clauses != NULL) {
    VectorDeleteWithContents(form->clauses,
                             (VectorElementDestructor)NormalizedClauseDelete,
                             /*free_element=*/false);
  }
  free(form);
}

static NormalizedConstraintForm* NewNormalizedConstraintForm(void) {
  NormalizedConstraintForm* form = malloc(sizeof(NormalizedConstraintForm));
  memset(form, 0, sizeof(*form));
  form->clauses = NewVector();
  return form;
}

static NormalizedClause* NewNormalizedClause(void) {
  NormalizedClause* clause = malloc(sizeof(NormalizedClause));
  memset(clause, 0, sizeof(*clause));
  clause->atoms = NewVector();
  return clause;
}

static NormalizedAtomic* NewNormalizedAtomic(ASTNode* expr,
                                             RequiresExpr* requires_expr,
                                             Vector* parameter_mapping,
                                             SourceLocation location) {
  NormalizedAtomic* atom = malloc(sizeof(NormalizedAtomic));
  memset(atom, 0, sizeof(*atom));
  atom->expr = expr;
  atom->requires_expr = requires_expr;
  atom->parameter_mapping = parameter_mapping;
  atom->location = location;
  return atom;
}

static Vector* FunctionTemplateParameters(Symbol* templ) {
  if (templ == NULL || templ->type == NULL || !TypeIsFunction(templ->type)) {
    return NULL;
  }
  return &templ->type->info.function.template_parameters;
}

static Vector* NewIdentityParameterMapping(Symbol* templ) {
  Vector* mapping = NewVector();
  Vector* parameters = FunctionTemplateParameters(templ);
  if (parameters == NULL) {
    return mapping;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    TemplateArgument* arg = calloc(1, sizeof(TemplateArgument));
    if (param != NULL) {
      arg->kind = param->kind;
    } else {
      arg->kind = kTemplateParameterType;
    }
    if (arg->kind == kTemplateParameterType) {
      if (param != NULL && param->type != NULL) {
        arg->type = TypeRecordCopy(param->type);
      } else {
        arg->type = NewTypeRecord(kTypeImplicit, kQualPlain);
        arg->type->declarator = kDeclPrimitive;
        arg->type->template_parameter_index = (int)i;
      }
      if (arg->type != NULL && arg->type->template_parameter_index < 0) {
        arg->type->template_parameter_index = (int)i;
      }
    }
    arg->template_parameter_index = (int)i;
    arg->location = SOURCE_LOCATION_MISSING;
    VectorAppend(mapping, arg);
  }
  return mapping;
}

static Vector* CopyParameterMapping(Vector* mapping) {
  if (mapping == NULL) {
    return NULL;
  }
  Vector* copy = NewVector();
  for (size_t i = 0; i < mapping->length; i++) {
    VectorAppend(copy, CopyTemplateArgumentForNormalization(mapping->value.p[i]));
  }
  return copy;
}

static int TemplateArgumentReferencedParameterIndex(TemplateArgument* arg) {
  if (arg == NULL) {
    return -1;
  }
  if (arg->template_parameter_index >= 0) {
    return arg->template_parameter_index;
  }
  if (arg->kind == kTemplateParameterType && arg->type != NULL &&
      arg->type->template_parameter_index >= 0) {
    return arg->type->template_parameter_index;
  }
  return -1;
}

static TemplateArgument* SubstituteTemplateArgumentInMapping(
    TemplateArgument* arg, Vector* mapping) {
  if (arg == NULL) {
    return NULL;
  }
  int index = TemplateArgumentReferencedParameterIndex(arg);
  if (index >= 0 && mapping != NULL && (size_t)index < mapping->length) {
    return CopyTemplateArgumentForNormalization(mapping->value.p[index]);
  }
  return CopyTemplateArgumentForNormalization(arg);
}

static Vector* ConceptParameterMapping(Symbol* concept_symbol, Vector* args,
                                       Vector* outer_mapping) {
  Vector* mapping = NewVector();
  if (concept_symbol == NULL || concept_symbol->concept_definition == NULL) {
    return mapping;
  }
  Concept* concept = concept_symbol->concept_definition;
  for (size_t i = 0; concept->template_parameters != NULL &&
                     i < concept->template_parameters->length; i++) {
    TemplateArgument* arg =
        args != NULL && i < args->length ? args->value.p[i] : NULL;
    VectorAppend(mapping,
                 SubstituteTemplateArgumentInMapping(arg, outer_mapping));
  }
  return mapping;
}

static bool ParameterMappingsEquivalent(Vector* left, Vector* right) {
  if (left == NULL || right == NULL) {
    return left == right;
  }
  if (left->length != right->length) {
    return false;
  }
  for (size_t i = 0; i < left->length; i++) {
    if (!NormalizationTemplateArgumentsEqual(left->value.p[i],
                                             right->value.p[i])) {
      return false;
    }
  }
  return true;
}

static bool NormalizedAtomsIdentical(NormalizedAtomic* left,
                                     NormalizedAtomic* right) {
  if (left == NULL || right == NULL) {
    return left == right;
  }
  if (left->requires_expr != NULL || right->requires_expr != NULL) {
    return left->requires_expr == right->requires_expr &&
           ParameterMappingsEquivalent(left->parameter_mapping,
                                       right->parameter_mapping);
  }
  return left->expr == right->expr &&
         ParameterMappingsEquivalent(left->parameter_mapping,
                                     right->parameter_mapping);
}

static bool ExprIsFoldConstraint(ASTNode* expr) {
  return expr != NULL && (expr->flags & kASTFoldExpression) != 0;
}

static bool ExprIsConceptIdentifier(ASTNode* expr, Symbol** concept_symbol,
                                    Vector** arguments) {
  if (expr == NULL || expr->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)expr;
  if (id->symbol == NULL || !id->symbol->flags.is_concept) {
    return false;
  }
  if (concept_symbol != NULL) {
    *concept_symbol = id->symbol;
  }
  if (arguments != NULL) {
    *arguments = id->template_arguments != NULL
                     ? TemplateArgumentVectorCopy(id->template_arguments)
                     : NewVector();
  }
  return true;
}

static NormalizedConstraintForm* NormalizedFormFromAtom(NormalizedAtomic* atom) {
  NormalizedConstraintForm* form = NewNormalizedConstraintForm();
  NormalizedClause* clause = NewNormalizedClause();
  VectorAppend(clause->atoms, atom);
  VectorAppend(form->clauses, clause);
  return form;
}

static NormalizedAtomic* CopyNormalizedAtomic(NormalizedAtomic* atom) {
  if (atom == NULL) {
    return NULL;
  }
  return NewNormalizedAtomic(atom->expr, atom->requires_expr,
                           CopyParameterMapping(atom->parameter_mapping),
                           atom->location);
}

static NormalizedConstraintForm* NormalizedFormDnfMergeClauses(
    NormalizedClause* left, NormalizedClause* right) {
  NormalizedConstraintForm* form = NewNormalizedConstraintForm();
  NormalizedClause* clause = NewNormalizedClause();
  for (size_t i = 0; left != NULL && left->atoms != NULL &&
                     i < left->atoms->length; i++) {
    VectorAppend(clause->atoms,
                 CopyNormalizedAtomic(left->atoms->value.p[i]));
  }
  for (size_t i = 0; right != NULL && right->atoms != NULL &&
                     i < right->atoms->length; i++) {
    VectorAppend(clause->atoms,
                 CopyNormalizedAtomic(right->atoms->value.p[i]));
  }
  VectorAppend(form->clauses, clause);
  return form;
}

static NormalizedConstraintForm* NormalizedFormDnfConjoin(
    NormalizedConstraintForm* left, NormalizedConstraintForm* right) {
  if (left == NULL || right == NULL || left->clauses->length == 0 ||
      right->clauses->length == 0) {
    NormalizedConstraintFormDelete(left);
    NormalizedConstraintFormDelete(right);
    return NewNormalizedConstraintForm();
  }
  NormalizedConstraintForm* result = NewNormalizedConstraintForm();
  for (size_t i = 0; i < left->clauses->length; i++) {
    NormalizedClause* left_clause = left->clauses->value.p[i];
    for (size_t j = 0; j < right->clauses->length; j++) {
      NormalizedClause* right_clause = right->clauses->value.p[j];
      NormalizedConstraintForm* merged =
          NormalizedFormDnfMergeClauses(left_clause, right_clause);
      for (size_t k = 0; k < merged->clauses->length; k++) {
        VectorAppend(result->clauses, merged->clauses->value.p[k]);
      }
      merged->clauses->length = 0;
      NormalizedConstraintFormDelete(merged);
    }
  }
  NormalizedConstraintFormDelete(left);
  NormalizedConstraintFormDelete(right);
  return result;
}

static NormalizedConstraintForm* NormalizedFormDnfDisjoin(
    NormalizedConstraintForm* left, NormalizedConstraintForm* right) {
  if (left == NULL || left->clauses->length == 0) {
    NormalizedConstraintFormDelete(left);
    return right != NULL ? right : NewNormalizedConstraintForm();
  }
  if (right == NULL || right->clauses->length == 0) {
    NormalizedConstraintFormDelete(right);
    return left;
  }
  for (size_t i = 0; i < right->clauses->length; i++) {
    VectorAppend(left->clauses, right->clauses->value.p[i]);
  }
  right->clauses->length = 0;
  NormalizedConstraintFormDelete(right);
  return left;
}

static NormalizedConstraintForm* NormalizedFormCnfConjoin(
    NormalizedConstraintForm* left, NormalizedConstraintForm* right) {
  return NormalizedFormDnfDisjoin(left, right);
}

static NormalizedConstraintForm* NormalizedFormCnfDisjoin(
    NormalizedConstraintForm* left, NormalizedConstraintForm* right) {
  return NormalizedFormDnfConjoin(left, right);
}

static NormalizedConstraintForm* NormalizeConstraintToDnf(
    ConstraintExpr* constraint, Vector* outer_mapping, Set* expanding);
static NormalizedConstraintForm* NormalizeConstraintToCnf(
    ConstraintExpr* constraint, Vector* outer_mapping, Set* expanding);

static NormalizedConstraintForm* NormalizeConceptIdToDnf(
    Symbol* concept_symbol, Vector* arguments, SourceLocation location,
    Vector* outer_mapping, Set* expanding) {
  if (concept_symbol == NULL || concept_symbol->concept_definition == NULL) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        NULL, NULL, CopyParameterMapping(outer_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  if (SetContains(expanding, concept_symbol)) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        NULL, NULL, CopyParameterMapping(outer_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  SetInsert(expanding, concept_symbol);
  Vector* concept_mapping =
      ConceptParameterMapping(concept_symbol, arguments, outer_mapping);
  NormalizedConstraintForm* result = NormalizeConstraintToDnf(
      concept_symbol->concept_definition->constraint, concept_mapping,
      expanding);
  SetRemove(expanding, concept_symbol);
  VectorDeleteWithContents(concept_mapping,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return result;
}

static NormalizedConstraintForm* NormalizeConceptIdToCnf(
    Symbol* concept_symbol, Vector* arguments, SourceLocation location,
    Vector* outer_mapping, Set* expanding) {
  if (concept_symbol == NULL || concept_symbol->concept_definition == NULL) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        NULL, NULL, CopyParameterMapping(outer_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  if (SetContains(expanding, concept_symbol)) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        NULL, NULL, CopyParameterMapping(outer_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  SetInsert(expanding, concept_symbol);
  Vector* concept_mapping =
      ConceptParameterMapping(concept_symbol, arguments, outer_mapping);
  NormalizedConstraintForm* result = NormalizeConstraintToCnf(
      concept_symbol->concept_definition->constraint, concept_mapping,
      expanding);
  SetRemove(expanding, concept_symbol);
  VectorDeleteWithContents(concept_mapping,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return result;
}

static NormalizedConstraintForm* NormalizeAtomicExpressionToDnf(
    ASTNode* expr, SourceLocation location, Vector* parameter_mapping,
    Set* expanding) {
  if (expr == NULL || ExprIsFoldConstraint(expr)) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        expr, NULL, CopyParameterMapping(parameter_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  if (expr->op == AST_OP(logand)) {
    BinaryASTNode* binary = (BinaryASTNode*)expr;
    NormalizedConstraintForm* left = NormalizeAtomicExpressionToDnf(
        binary->left, binary->left != NULL ? binary->left->location : location,
        parameter_mapping, expanding);
    NormalizedConstraintForm* right = NormalizeAtomicExpressionToDnf(
        binary->right,
        binary->right != NULL ? binary->right->location : location,
        parameter_mapping, expanding);
    return NormalizedFormDnfConjoin(left, right);
  }
  if (expr->op == AST_OP(logor)) {
    BinaryASTNode* binary = (BinaryASTNode*)expr;
    NormalizedConstraintForm* left = NormalizeAtomicExpressionToDnf(
        binary->left, binary->left != NULL ? binary->left->location : location,
        parameter_mapping, expanding);
    NormalizedConstraintForm* right = NormalizeAtomicExpressionToDnf(
        binary->right,
        binary->right != NULL ? binary->right->location : location,
        parameter_mapping, expanding);
    return NormalizedFormDnfDisjoin(left, right);
  }
  Symbol* concept_symbol = NULL;
  Vector* arguments = NULL;
  if (ExprIsConceptIdentifier(expr, &concept_symbol, &arguments)) {
    NormalizedConstraintForm* result = NormalizeConceptIdToDnf(
        concept_symbol, arguments, location, parameter_mapping, expanding);
    if (arguments != NULL) {
      VectorDeleteWithContents(arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return result;
  }
  NormalizedAtomic* atom = NewNormalizedAtomic(
      expr, NULL, CopyParameterMapping(parameter_mapping), location);
  return NormalizedFormFromAtom(atom);
}

static NormalizedConstraintForm* NormalizeAtomicExpressionToCnf(
    ASTNode* expr, SourceLocation location, Vector* parameter_mapping,
    Set* expanding) {
  if (expr == NULL || ExprIsFoldConstraint(expr)) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        expr, NULL, CopyParameterMapping(parameter_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  if (expr->op == AST_OP(logand)) {
    BinaryASTNode* binary = (BinaryASTNode*)expr;
    NormalizedConstraintForm* left = NormalizeAtomicExpressionToCnf(
        binary->left, binary->left != NULL ? binary->left->location : location,
        parameter_mapping, expanding);
    NormalizedConstraintForm* right = NormalizeAtomicExpressionToCnf(
        binary->right,
        binary->right != NULL ? binary->right->location : location,
        parameter_mapping, expanding);
    return NormalizedFormCnfConjoin(left, right);
  }
  if (expr->op == AST_OP(logor)) {
    BinaryASTNode* binary = (BinaryASTNode*)expr;
    NormalizedConstraintForm* left = NormalizeAtomicExpressionToCnf(
        binary->left, binary->left != NULL ? binary->left->location : location,
        parameter_mapping, expanding);
    NormalizedConstraintForm* right = NormalizeAtomicExpressionToCnf(
        binary->right,
        binary->right != NULL ? binary->right->location : location,
        parameter_mapping, expanding);
    return NormalizedFormCnfDisjoin(left, right);
  }
  Symbol* concept_symbol = NULL;
  Vector* arguments = NULL;
  if (ExprIsConceptIdentifier(expr, &concept_symbol, &arguments)) {
    NormalizedConstraintForm* result = NormalizeConceptIdToCnf(
        concept_symbol, arguments, location, parameter_mapping, expanding);
    if (arguments != NULL) {
      VectorDeleteWithContents(arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
    return result;
  }
  NormalizedAtomic* atom = NewNormalizedAtomic(
      expr, NULL, CopyParameterMapping(parameter_mapping), location);
  return NormalizedFormFromAtom(atom);
}

static NormalizedConstraintForm* NormalizeConstraintToDnf(
    ConstraintExpr* constraint, Vector* parameter_mapping, Set* expanding) {
  if (constraint == NULL) {
    return NewNormalizedConstraintForm();
  }
  switch (constraint->kind) {
    case kConstraintConjunction:
      return NormalizedFormDnfConjoin(
          NormalizeConstraintToDnf(constraint->as.binary.left,
                                   parameter_mapping, expanding),
          NormalizeConstraintToDnf(constraint->as.binary.right,
                                   parameter_mapping, expanding));
    case kConstraintDisjunction:
      return NormalizedFormDnfDisjoin(
          NormalizeConstraintToDnf(constraint->as.binary.left,
                                   parameter_mapping, expanding),
          NormalizeConstraintToDnf(constraint->as.binary.right,
                                   parameter_mapping, expanding));
    case kConstraintConceptId:
      return NormalizeConceptIdToDnf(
          constraint->as.concept_id.concept_symbol,
          constraint->as.concept_id.arguments, constraint->location,
          parameter_mapping, expanding);
    case kConstraintRequires: {
      NormalizedAtomic* atom = NewNormalizedAtomic(
          NULL, constraint->as.requires_.requires_expr,
          CopyParameterMapping(parameter_mapping), constraint->location);
      return NormalizedFormFromAtom(atom);
    }
    case kConstraintAtomic:
      return NormalizeAtomicExpressionToDnf(constraint->as.atomic.expr,
                                          constraint->location,
                                          parameter_mapping, expanding);
  }
  return NewNormalizedConstraintForm();
}

static NormalizedConstraintForm* NormalizeConstraintToCnf(
    ConstraintExpr* constraint, Vector* parameter_mapping, Set* expanding) {
  if (constraint == NULL) {
    return NewNormalizedConstraintForm();
  }
  switch (constraint->kind) {
    case kConstraintConjunction:
      return NormalizedFormCnfConjoin(
          NormalizeConstraintToCnf(constraint->as.binary.left,
                                   parameter_mapping, expanding),
          NormalizeConstraintToCnf(constraint->as.binary.right,
                                   parameter_mapping, expanding));
    case kConstraintDisjunction:
      return NormalizedFormCnfDisjoin(
          NormalizeConstraintToCnf(constraint->as.binary.left,
                                   parameter_mapping, expanding),
          NormalizeConstraintToCnf(constraint->as.binary.right,
                                   parameter_mapping, expanding));
    case kConstraintConceptId:
      return NormalizeConceptIdToCnf(
          constraint->as.concept_id.concept_symbol,
          constraint->as.concept_id.arguments, constraint->location,
          parameter_mapping, expanding);
    case kConstraintRequires: {
      NormalizedAtomic* atom = NewNormalizedAtomic(
          NULL, constraint->as.requires_.requires_expr,
          CopyParameterMapping(parameter_mapping), constraint->location);
      return NormalizedFormFromAtom(atom);
    }
    case kConstraintAtomic:
      return NormalizeAtomicExpressionToCnf(constraint->as.atomic.expr,
                                          constraint->location,
                                          parameter_mapping, expanding);
  }
  return NewNormalizedConstraintForm();
}

static bool NormalizedClauseSubsumes(NormalizedClause* disjunctive,
                                     NormalizedClause* conjunctive) {
  if (conjunctive == NULL || conjunctive->atoms == NULL ||
      conjunctive->atoms->length == 0) {
    return true;
  }
  if (disjunctive == NULL || disjunctive->atoms == NULL) {
    return false;
  }
  for (size_t i = 0; i < disjunctive->atoms->length; i++) {
    for (size_t j = 0; j < conjunctive->atoms->length; j++) {
      if (NormalizedAtomsIdentical(disjunctive->atoms->value.p[i],
                                   conjunctive->atoms->value.p[j])) {
        return true;
      }
    }
  }
  return false;
}

static bool NormalizedFormSubsumes(NormalizedConstraintForm* stronger_dnf,
                                   NormalizedConstraintForm* weaker_cnf) {
  if (stronger_dnf == NULL || weaker_cnf == NULL) {
    return stronger_dnf == weaker_cnf;
  }
  if (weaker_cnf->clauses->length == 0) {
    return true;
  }
  if (stronger_dnf->clauses->length == 0) {
    return false;
  }
  for (size_t i = 0; i < weaker_cnf->clauses->length; i++) {
    NormalizedClause* weak_clause = weaker_cnf->clauses->value.p[i];
    for (size_t j = 0; j < stronger_dnf->clauses->length; j++) {
      if (!NormalizedClauseSubsumes(stronger_dnf->clauses->value.p[j],
                                    weak_clause)) {
        return false;
      }
    }
  }
  return true;
}

static bool ConstraintSubsumesWithMapping(ConstraintExpr* stronger,
                                            ConstraintExpr* weaker,
                                            Vector* parameter_mapping) {
  if (stronger == NULL || weaker == NULL) {
    return stronger == weaker;
  }
  Set* expanding = NewSet(SymbolPointerCompare);
  NormalizedConstraintForm* stronger_dnf =
      NormalizeConstraintToDnf(stronger, parameter_mapping, expanding);
  NormalizedConstraintForm* weaker_cnf =
      NormalizeConstraintToCnf(weaker, parameter_mapping, expanding);
  bool result = NormalizedFormSubsumes(stronger_dnf, weaker_cnf);
  NormalizedConstraintFormDelete(stronger_dnf);
  NormalizedConstraintFormDelete(weaker_cnf);
  SetDelete(expanding);
  return result;
}

static bool ConstraintSubsumesForTemplate(ConstraintExpr* stronger,
                                          ConstraintExpr* weaker,
                                          Symbol* templ) {
  Vector* mapping = NewIdentityParameterMapping(templ);
  bool result = ConstraintSubsumesWithMapping(stronger, weaker, mapping);
  VectorDeleteWithContents(mapping,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return result;
}

int ConceptsCompareAssociatedConstraints(
    ConstraintExpr* left, ConstraintExpr* right,
    Vector* left_parameter_mapping, Vector* right_parameter_mapping) {
  if (!ConceptsHasAssociatedConstraint(left) &&
      !ConceptsHasAssociatedConstraint(right)) {
    return 0;
  }
  if (!ConceptsHasAssociatedConstraint(left)) {
    return -1;
  }
  if (!ConceptsHasAssociatedConstraint(right)) {
    return 1;
  }
  bool left_subsumes_right = ConstraintSubsumesWithMapping(
      left, right, left_parameter_mapping);
  bool right_subsumes_left = ConstraintSubsumesWithMapping(
      right, left, right_parameter_mapping);
  if (left_subsumes_right == right_subsumes_left) {
    return 0;
  }
  return left_subsumes_right ? 1 : -1;
}

bool ConceptsFunctionTemplateConstraintsEquivalent(Symbol* left,
                                                   Symbol* right) {
  ConstraintExpr* left_constraint = FunctionTemplateConstraint(left);
  ConstraintExpr* right_constraint = FunctionTemplateConstraint(right);
  if (left_constraint == NULL || right_constraint == NULL) {
    return left_constraint == right_constraint;
  }
  return ConstraintSubsumesForTemplate(left_constraint, right_constraint,
                                       left) &&
         ConstraintSubsumesForTemplate(right_constraint, left_constraint,
                                       right);
}

int ConceptsCompareFunctionTemplateConstraints(Symbol* left, Symbol* right) {
  ConstraintExpr* left_constraint = FunctionTemplateConstraint(left);
  ConstraintExpr* right_constraint = FunctionTemplateConstraint(right);
  bool left_constrained = left_constraint != NULL;
  bool right_constrained = right_constraint != NULL;
  if (left_constrained != right_constrained) {
    return left_constrained ? 1 : -1;
  }
  if (!left_constrained) {
    return 0;
  }
  Vector* left_mapping = NewIdentityParameterMapping(left);
  Vector* right_mapping = NewIdentityParameterMapping(right);
  int result = ConceptsCompareAssociatedConstraints(
      left_constraint, right_constraint, left_mapping, right_mapping);
  VectorDeleteWithContents(left_mapping,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  VectorDeleteWithContents(right_mapping,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return result;
}

static void AppendTemplateArgumentDescription(String* out, Concept* concept,
                                              Vector* arguments) {
  if (concept == NULL || concept->template_parameters == NULL ||
      arguments == NULL) {
    return;
  }
  bool first = true;
  for (size_t i = 0; i < concept->template_parameters->length &&
                     i < arguments->length; i++) {
    TemplateParameter* param = concept->template_parameters->value.p[i];
    TemplateArgument* arg = arguments->value.p[i];
    if (param == NULL || arg == NULL) {
      continue;
    }
    if (first) {
      StringAppend(out, " [with ");
      first = false;
    } else {
      StringAppend(out, ", ");
    }
    StringAppend(out, param->name.value);
    StringAppend(out, " = ");
    if (arg->kind == kTemplateParameterType && arg->type != NULL) {
      TypeRecordToString(arg->type, out);
      while (out->length > 0 && out->value[out->length - 1] == ' ') {
        out->length--;
        out->value[out->length] = '\0';
      }
    } else {
      StringPrintf(out, "%lld", arg->int_value);
    }
  }
  if (!first) {
    while (out->length > 0 && out->value[out->length - 1] == ' ') {
      out->length--;
      out->value[out->length] = '\0';
    }
    StringAppend(out, "]");
  }
}

static void ReportConstraintFailure(ConstraintExpr* constraint,
                                    Vector* arguments) {
  if (constraint == NULL) {
    return;
  }
  switch (constraint->kind) {
    case kConstraintConjunction: {
      int64_t value = 0;
      if (!EvaluateConstraintInteger(constraint->as.binary.left, arguments,
                                     &value) || value == 0) {
        SemanticNoteAtLocation(
            constraint->location,
            "because the left operand of this conjunction constraint was not satisfied");
        ReportConstraintFailure(constraint->as.binary.left, arguments);
      } else {
        SemanticNoteAtLocation(
            constraint->location,
            "because the right operand of this conjunction constraint was not satisfied");
        ReportConstraintFailure(constraint->as.binary.right, arguments);
      }
      return;
    }
    case kConstraintDisjunction:
      SemanticNoteAtLocation(
          constraint->location,
          "because neither operand of this disjunction constraint was satisfied");
      ReportConstraintFailure(constraint->as.binary.left, arguments);
      ReportConstraintFailure(constraint->as.binary.right, arguments);
      return;
    case kConstraintConceptId: {
      Symbol* concept_symbol = constraint->as.concept_id.concept_symbol;
      Concept* concept = concept_symbol != NULL
                             ? concept_symbol->concept_definition
                             : NULL;
      Vector* concrete_args = TypeSubstituteTemplateArgumentVector(
          &compiler->syntax, constraint->as.concept_id.arguments, arguments);
      String note;
      StringInit(&note, "because concept ");
      StringAppend(&note,
                   concept_symbol != NULL ? concept_symbol->name.value : "<unknown>");
      StringAppend(&note, " was not satisfied");
      AppendTemplateArgumentDescription(&note, concept, concrete_args);
      SemanticNoteAtLocation(concept != NULL ? concept->location
                                             : constraint->location,
                             "%s", note.value);
      StringDestruct(&note);
      if (concept != NULL) {
        int64_t value = 0;
        if (concept->constraint != NULL &&
            (!EvaluateConstraintInteger(concept->constraint, concrete_args,
                                        &value) ||
             value == 0)) {
          ReportConstraintFailure(concept->constraint, concrete_args);
        }
      }
      if (concrete_args != NULL) {
        VectorDeleteWithContents(
            concrete_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      return;
    }
    case kConstraintAtomic: {
      int64_t dummy = 0;
      AtomicConstraintResult reason = EvaluateAtomicConstraint(
          constraint->as.atomic.expr, arguments, constraint->location, &dummy);
      SemanticNoteAtLocation(constraint->location, "%s",
                             AtomicConstraintFailureMessage(reason));
      return;
    }
    case kConstraintRequires: {
      SemanticNoteAtLocation(
          constraint->location,
          "because this requires-expression was not satisfied");
      RequiresExpr* requires_expr = constraint->as.requires_.requires_expr;
      if (requires_expr == NULL || requires_expr->requirements == NULL) {
        return;
      }
      for (size_t i = 0; i < requires_expr->requirements->length; i++) {
        Requirement* requirement = requires_expr->requirements->value.p[i];
        if (requirement == NULL) {
          continue;
        }
        switch (requirement->kind) {
          case kRequirementSimple:
          case kRequirementCompound:
          {
            RequirementFailureInfo failure_info;
            if (!EvaluateExpressionRequirement(requirement, requires_expr,
                                               arguments, &failure_info)) {
              SemanticNoteAtLocation(requirement->location,
                                     "%s",
                                     RequirementFailureReasonMessage(
                                         requirement->kind,
                                         failure_info.reason));
              ReportReturnTypeRequirementDetail(requirement, &failure_info,
                                                arguments);
              RequirementFailureInfoDestruct(&failure_info);
              return;
            }
            RequirementFailureInfoDestruct(&failure_info);
            break;
          }
          case kRequirementNested: {
            int64_t value = 0;
            if (!EvaluateConstraintInteger(requirement->nested, arguments,
                                           &value) ||
                value == 0) {
              SemanticNoteAtLocation(
                  requirement->location,
                  "because this nested requirement was not satisfied");
              ReportConstraintFailure(requirement->nested, arguments);
              return;
            }
            break;
          }
          case kRequirementType:
            if (!EvaluateTypeRequirement(requirement, arguments)) {
              SemanticNoteAtLocation(
                  requirement->location,
                  "because this type requirement was not satisfied");
              return;
            }
            break;
        }
      }
      return;
    }
  }
}

void ConceptsReportFunctionTemplateConstraintFailure(Symbol* templ,
                                                     Vector* arguments) {
  if (!ConceptsFunctionTemplateHasAssociatedConstraint(templ)) {
    return;
  }
  ConceptsReportAssociatedConstraintFailure(
      templ->type->info.function.associated_constraint, arguments, templ->location,
      "candidate template ignored: constraints not satisfied");
}

static Vector* MoveCurrentTemplateParameters(Syntax* syntax) {
  Vector* params = NewVector();
  if (syntax->current_template_parameters == NULL) {
    return params;
  }
  for (size_t i = 0; i < syntax->current_template_parameters->length; i++) {
    VectorAppend(params, syntax->current_template_parameters->value.p[i]);
  }
  syntax->current_template_parameters->length = 0;
  return params;
}

static bool AddConceptSymbolToEnclosingScope(Syntax* syntax, Symbol* symbol) {
  LocalSymbolTable* saved_symbols = syntax->local_symbol_stack;
  LocalSymbolTable* saved_tags = syntax->local_tag_stack;
  if (saved_symbols != NULL) {
    syntax->local_symbol_stack = saved_symbols->prev;
  }
  if (saved_tags != NULL) {
    syntax->local_tag_stack = saved_tags->prev;
  }
  bool added = SyntaxAddSymbol(syntax, symbol);
  syntax->local_symbol_stack = saved_symbols;
  syntax->local_tag_stack = saved_tags;
  return added;
}

static ConstraintExpr* ParseConceptConstraint(Syntax* syntax,
                                              SourceLocation location);
static ConstraintExpr* ParseConceptConstraintOr(Syntax* syntax,
                                                SourceLocation location);

static Requirement* ParseRequiresRequirement(Syntax* syntax) {
  Lex* lex = syntax->lex;
  SourceLocation requirement_location = lex->current_token_location;
  if (LexLookingAt(lex, TOK(requires))) {
    LexNextToken(lex);  // requires
    ConstraintExpr* nested =
        ParseConceptConstraintOr(syntax, requirement_location);
    SyntaxNeedSemicolon(syntax, TC(closebra) | TC(semicolon));
    return NewNestedRequirement(nested, requirement_location);
  }
  if (LexLookingAt(lex, TOK(typename))) {
    TypeParser parser;
    TypeParserInit(&parser, lex, syntax, STO(auto), kParsingPrototype);
    TypeRecord* type = TypeParserParseType(&parser, true);
    TypeParserDestruct(&parser);
    SyntaxNeedSemicolon(syntax, TC(closebra) | TC(semicolon));
    return NewTypeRequirement(type, requirement_location);
  }
  if (LexMatch(lex, TOK(lbrace))) {
    ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebra) | TC(semicolon));
    bool is_noexcept = LexMatch(lex, TOK(noexcept));
    ConstraintExpr* return_type_constraint = NULL;
    if (LexLookingAt(lex, TOK(arrow))) {
      SourceLocation arrow_location = lex->current_token_location;
      LexNextToken(lex);
      return_type_constraint = ParseConceptConstraintOr(syntax, arrow_location);
    }
    SyntaxNeedSemicolon(syntax, TC(closebra) | TC(semicolon));
    return NewCompoundRequirement(expr, is_noexcept,
                                  return_type_constraint,
                                  requirement_location);
  }

  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(semicolon));
  SyntaxNeedSemicolon(syntax, TC(closebra) | TC(semicolon));
  return NewSimpleRequirement(expr, requirement_location);
}

static Vector* ParseRequiresParameterList(Syntax* syntax) {
  Vector* parameters = NewVector();
  Lex* lex = syntax->lex;
  if (!LexMatch(lex, TOK(lparen))) {
    return parameters;
  }
  int arg_number = 0;
  while (!LexEof(lex) && !LexLookingAt(lex, TOK(rparen))) {
    if (LexLookingAt(lex, TOK(ellipsis))) {
      SyntaxError(syntax,
                  "requires-expression parameter list cannot end with an "
                  "ellipsis");
      SyntaxRecover(syntax, TC(closebra) | TC(semicolon));
      break;
    }
    TypeParser parser;
    TypeParserInit(&parser, lex, syntax, STO(auto), kParsingPrototype);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* param = TypeParserParseDeclarator(&parser, type);
    TypeParserDestruct(&parser);
    if (param != NULL) {
      if (param->flags.is_parameter_pack) {
        SyntaxError(syntax,
                    "requires-expression parameters cannot be parameter packs");
      }
      if (LexLookingAt(lex, TOK(equal))) {
        SyntaxError(syntax,
                    "requires-expression parameters cannot have default "
                    "arguments");
        SyntaxRecover(syntax, TC(closebra) | TC(semicolon));
      }
      if (param->type != NULL && TypeIsFunction(param->type)) {
        SyntaxError(syntax,
                    "requires-expression parameter has an invalid type");
      }
      param->flags.is_defined = true;
      param->flags.is_argument = true;
      param->value.arg_number = arg_number;
      if (!SyntaxAddSymbol(syntax, param)) {
        SyntaxError(syntax, "Duplicate requires-expression parameter '%s'",
                    param->name.value);
      }
      VectorAppend(parameters, param);
    }
    arg_number++;
    if (!LexMatch(lex, TOK(comma))) {
      if (LexLookingAt(lex, TOK(ellipsis))) {
        SyntaxError(syntax,
                    "requires-expression parameter list cannot end with an "
                    "ellipsis");
        SyntaxRecover(syntax, TC(closebra) | TC(semicolon));
      }
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra) | TC(semicolon));
  return parameters;
}

static ConstraintExpr* ParseRequiresExpressionConstraint(Syntax* syntax) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);  // requires

  SyntaxOpenScope(syntax);
  Vector* parameters = ParseRequiresParameterList(syntax);

  if (!LexMatch(lex, TOK(lbrace))) {
    SyntaxError(syntax, "Expected '{' in requires-expression");
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxCloseScope(syntax);
    return NewRequiresConstraint(NewRequiresExpr(parameters, NewVector(),
                                                location),
                                 location);
  }

  Vector* requirements = NewVector();
  while (!LexEof(lex) && !LexLookingAt(lex, TOK(rbrace))) {
    VectorAppend(requirements, ParseRequiresRequirement(syntax));
  }
  SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebra) | TC(semicolon));
  SyntaxCloseScope(syntax);
  RequiresExpr* requires_expr =
      NewRequiresExpr(parameters, requirements, location);
  return NewRequiresConstraint(requires_expr, location);
}

static ConstraintExpr* NewConstraintFromExpression(ASTNode* expr,
                                                   SourceLocation location) {
  if (expr == NULL) {
    return NewAtomicConstraint(expr, location);
  }
  if (expr->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)expr;
    if (id->symbol != NULL && id->symbol->flags.is_concept) {
      Vector* args = id->template_arguments != NULL
                         ? TemplateArgumentVectorCopy(id->template_arguments)
                         : NewVector();
      return NewConceptIdConstraint(id->symbol, args, expr->location);
    }
  }
  if (expr->op == AST_OP(logand) || expr->op == AST_OP(logor)) {
    BinaryASTNode* binary = (BinaryASTNode*)expr;
    ConstraintExpr* left =
        NewConstraintFromExpression(binary->left, binary->left != NULL
                                                      ? binary->left->location
                                                      : location);
    ConstraintExpr* right =
        NewConstraintFromExpression(binary->right, binary->right != NULL
                                                       ? binary->right->location
                                                       : location);
    if (expr->op == AST_OP(logand)) {
      return NewConjunctionConstraint(left, right, location);
    }
    return NewDisjunctionConstraint(left, right, location);
  }
  return NewAtomicConstraint(expr, location);
}

static ConstraintExpr* ParseConceptPrimaryConstraint(Syntax* syntax,
                                                     SourceLocation location) {
  if (LexLookingAt(syntax->lex, TOK(requires))) {
    return ParseRequiresExpressionConstraint(syntax);
  }
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    String name;
    StringInit(&name, syntax->lex->spelling.value);
    Symbol* symbol = SyntaxFindSymbol(syntax, &name);
    StringDestruct(&name);
    if (symbol != NULL && symbol->flags.is_concept) {
      SourceLocation concept_location = syntax->lex->current_token_location;
      LexNextToken(syntax->lex);
      Vector* args = NULL;
      if (LexLookingAt(syntax->lex, TOK(less))) {
        args = SyntaxParseTemplateArgumentList(syntax, TC(semicolon));
      } else {
        args = NewVector();
      }
      return NewConceptIdConstraint(symbol, args, concept_location);
    }
  }
  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(semicolon));
  return NewConstraintFromExpression(expr, location);
}

static ConstraintExpr* ParseConceptConstraintAnd(Syntax* syntax,
                                                 SourceLocation location) {
  ConstraintExpr* result = ParseConceptPrimaryConstraint(syntax, location);
  while (LexLookingAt(syntax->lex, TOK(ampamp))) {
    SourceLocation op_location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    ConstraintExpr* right =
        ParseConceptPrimaryConstraint(syntax, op_location);
    result = NewConjunctionConstraint(result, right, op_location);
  }
  return result;
}

static ConstraintExpr* ParseConceptConstraintOr(Syntax* syntax,
                                                SourceLocation location) {
  ConstraintExpr* result = ParseConceptConstraintAnd(syntax, location);
  while (LexLookingAt(syntax->lex, TOK(barbar))) {
    SourceLocation op_location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    ConstraintExpr* right = ParseConceptConstraintAnd(syntax, op_location);
    result = NewDisjunctionConstraint(result, right, op_location);
  }
  return result;
}

static ConstraintExpr* ParseConceptConstraint(Syntax* syntax,
                                              SourceLocation location) {
  return ParseConceptConstraintOr(syntax, location);
}

static bool LookingAtPreCXX20ConceptDefinition(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(lex, TOK(identifier)) ||
      !StringEqual(&lex->spelling, "concept")) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  LexNextToken(lex);
  bool looks_like_concept_definition = LexLookingAt(lex, TOK(identifier));
  if (looks_like_concept_definition) {
    LexNextToken(lex);
    looks_like_concept_definition = LexLookingAt(lex, TOK(equal));
  }
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return looks_like_concept_definition;
}

ConstraintExpr* ConceptsParseRequiresClause(Syntax* syntax) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(syntax->lex, TOK(requires))) {
    return NULL;
  }
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // requires
  return ParseConceptConstraint(syntax, location);
}

ConstraintExpr* ConceptsParseRequiresExpression(Syntax* syntax) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(syntax->lex, TOK(requires))) {
    return NULL;
  }
  return ParseRequiresExpressionConstraint(syntax);
}

ASTNode* ConceptsParseDefinition(Syntax* syntax,
                                 SourceLocation template_location) {
  Lex* lex = syntax->lex;
  if (!LexLookingAt(lex, TOK(concept)) &&
      !LookingAtPreCXX20ConceptDefinition(syntax)) {
    return NULL;
  }

  SourceLocation concept_location = lex->current_token_location;
  LexNextToken(lex);  // concept

  if (!CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    SyntaxError(syntax, "concept definitions require C++20");
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return NewDeclarationListASTNode(NewVector(), template_location);
  }
  if (syntax->context == kParsingStructOrUnion) {
    SyntaxError(syntax, "concept definitions are not permitted in classes");
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return NewDeclarationListASTNode(NewVector(), template_location);
  }
  if (syntax->parsing_template_specialization) {
    SyntaxError(syntax, "concept specialization is not permitted");
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return NewDeclarationListASTNode(NewVector(), template_location);
  }
  if (syntax->current_template_parameters == NULL ||
      syntax->current_template_parameters->length == 0) {
    SyntaxError(syntax, "concept definition requires a template parameter list");
  }
  if (!LexLookingAt(lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected concept name");
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return NewDeclarationListASTNode(NewVector(), template_location);
  }

  String name;
  StringInit(&name, lex->spelling.value);
  SourceLocation name_location = lex->current_token_location;
  LexNextToken(lex);

  if (!LexMatch(lex, TOK(equal))) {
    SyntaxError(syntax, "Expected '=' in concept definition");
    StringDestruct(&name);
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return NewDeclarationListASTNode(NewVector(), template_location);
  }

  ConstraintExpr* constraint = ParseConceptConstraint(syntax, concept_location);
  TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
  Symbol* symbol = NewSymbol(name.value, bool_type, STO(implicit));
  TypeRecordDelete(bool_type);
  symbol->location = name_location;
  symbol->flags.is_concept = true;
  symbol->flags.is_template = true;
  symbol->flags.is_defined = true;
  symbol->concept_definition = NewConcept(
      name.value, MoveCurrentTemplateParameters(syntax), constraint,
      concept_location);

  if (!AddConceptSymbolToEnclosingScope(syntax, symbol)) {
    SyntaxError(syntax, "Duplicate definition of concept %s", name.value);
    SymbolDelete(symbol);
  }

  StringDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return NewDeclarationListASTNode(NewVector(), template_location);
}
