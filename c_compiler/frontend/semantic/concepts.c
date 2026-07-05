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

#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "compiler.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "syntax.h"
#include "type.h"

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

static bool EvaluateConceptDefinitionInteger(Symbol* concept_symbol,
                                             Vector* arguments,
                                             SourceLocation location,
                                             int64_t* result);
static void AppendTemplateArgumentDescription(String* out, Concept* concept,
                                              Vector* arguments);

static bool EvaluateAtomicConstraintInteger(ASTNode* expr, Vector* arguments,
                                            SourceLocation location,
                                            int64_t* result) {
  if (expr == NULL) {
    return false;
  }
  if (arguments == NULL) {
    return EvaluateIntegerExpression(expr, result);
  }
  ASTNode* cloned = TypeSubstituteTemplateExpression(&compiler->syntax, expr,
                                                     arguments, location);
  if (cloned == NULL) {
    return false;
  }
  DiagnosticSuppressBegin();
  cloned = AnalyzeExpression(cloned);
  bool ok = EvaluateIntegerExpression(cloned, result);
  DiagnosticSuppressEnd();
  ASTNodeDelete(cloned);
  return ok;
}

static void FindPotentiallyThrowingExpression(ASTNode* node, void* data,
                                              int child_id,
                                              VisitorMode mode) {
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren) {
    return;
  }
  bool* potentially_throwing = data;
  if (*potentially_throwing) {
    return;
  }
  if (node->op == AST_OP(throw)) {
    *potentially_throwing = true;
    return;
  }
  if (node->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  TypeRecord* callee_type = call->left != NULL ? call->left->type : NULL;
  if (callee_type != NULL && TypeIsPointer(callee_type)) {
    callee_type = callee_type->next;
  }
  if (callee_type == NULL || !TypeIsFunction(callee_type) ||
      !callee_type->info.function.is_noexcept) {
    *potentially_throwing = true;
  }
}

static bool ExpressionRequirementNoexceptSatisfied(ASTNode* expr) {
  bool potentially_throwing = false;
  ASTNodeVisit(expr, FindPotentiallyThrowingExpression, 0,
               &potentially_throwing);
  return !potentially_throwing;
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
      TypeRecord* concrete =
          TypeSubstituteTemplateType(&compiler->syntax, param->type, arguments);
      SymbolSetType(param, concrete);
      TypeRecordDelete(concrete);
    }
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  ASTNode* cloned = TypeSubstituteTemplateExpression(
      &compiler->syntax, requirement->expr, arguments,
      requirement->location);
  if (cloned != NULL) {
    cloned = AnalyzeExpression(cloned);
  }
  bool failed = cloned == NULL || DiagnosticErrorTrapped();
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
  if (!failed && !EvaluateReturnTypeRequirement(
                     requirement->return_type_constraint, cloned->type,
                     arguments)) {
    failed = true;
    if (failure_info != NULL) {
      failure_info->reason = kRequirementReturnTypeUnsatisfied;
      failure_info->expression_type =
          cloned->type != NULL ? TypeRecordCopy(cloned->type) : NULL;
    }
  }
  DiagnosticErrorTrapEnd(saved_trap);
  ASTNodeDelete(cloned);
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
  ConstraintExpr* constraint = concept_symbol->concept_definition->constraint;
  if (constraint == NULL) {
    return false;
  }
  if (arguments == NULL || constraint->kind != kConstraintAtomic) {
    return EvaluateConstraintInteger(constraint, arguments, result);
  }

  return EvaluateAtomicConstraintInteger(constraint->as.atomic.expr, arguments,
                                         location, result);
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
  return EvaluateConceptDefinitionInteger(id->symbol, id->template_arguments,
                                          node->location, result);
}

bool ConceptsFunctionTemplateConstraintsSatisfied(Symbol* templ,
                                                 Vector* arguments) {
  if (!ConceptsFunctionTemplateHasAssociatedConstraint(templ)) {
    return true;
  }
  ConstraintExpr* constraint =
      templ->type->info.function.associated_constraint;
  int64_t value = 0;
  return EvaluateConstraintInteger(constraint, arguments, &value) && value != 0;
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

static bool TemplateArgumentTypeEqual(TemplateArgument* left,
                                      TemplateArgument* right) {
  if (left == NULL || right == NULL || left->kind != right->kind) {
    return left == right;
  }
  if (left->kind == kTemplateParameterType) {
    return TypeEqual(left->type, right->type);
  }
  return left->int_value == right->int_value &&
         left->template_parameter_index == right->template_parameter_index;
}

static bool TemplateArgumentVectorShallowEqual(Vector* left, Vector* right) {
  if (left == NULL || right == NULL || left->length != right->length) {
    return left == right;
  }
  for (size_t i = 0; i < left->length; i++) {
    if (!TemplateArgumentTypeEqual(left->value.p[i], right->value.p[i])) {
      return false;
    }
  }
  return true;
}

static bool ConstraintExprEqual(ConstraintExpr* left, ConstraintExpr* right) {
  if (left == NULL || right == NULL || left->kind != right->kind) {
    return left == right;
  }
  switch (left->kind) {
    case kConstraintConjunction:
    case kConstraintDisjunction:
      return ConstraintExprEqual(left->as.binary.left, right->as.binary.left) &&
             ConstraintExprEqual(left->as.binary.right,
                                 right->as.binary.right);
    case kConstraintConceptId:
      return left->as.concept_id.concept_symbol ==
                 right->as.concept_id.concept_symbol &&
             TemplateArgumentVectorShallowEqual(left->as.concept_id.arguments,
                                                right->as.concept_id.arguments);
    case kConstraintAtomic:
      return left->as.atomic.expr == right->as.atomic.expr;
    case kConstraintRequires:
      return left->as.requires_.requires_expr ==
             right->as.requires_.requires_expr;
  }
  return false;
}

bool ConceptsFunctionTemplateConstraintsEquivalent(Symbol* left,
                                                   Symbol* right) {
  return ConstraintExprEqual(FunctionTemplateConstraint(left),
                             FunctionTemplateConstraint(right));
}

static bool ConstraintSubsumes(ConstraintExpr* stronger,
                               ConstraintExpr* weaker) {
  if (ConstraintExprEqual(stronger, weaker)) {
    return true;
  }
  if (stronger == NULL || weaker == NULL) {
    return false;
  }
  if (stronger->kind == kConstraintConjunction) {
    return ConstraintSubsumes(stronger->as.binary.left, weaker) ||
           ConstraintSubsumes(stronger->as.binary.right, weaker);
  }
  return false;
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
  bool left_subsumes_right =
      ConstraintSubsumes(left_constraint, right_constraint);
  bool right_subsumes_left =
      ConstraintSubsumes(right_constraint, left_constraint);
  if (left_subsumes_right == right_subsumes_left) {
    return 0;
  }
  return left_subsumes_right ? 1 : -1;
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
        ReportConstraintFailure(constraint->as.binary.left, arguments);
      } else {
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
            EvaluateConstraintInteger(concept->constraint, concrete_args,
                                      &value) &&
            value == 0) {
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
    case kConstraintAtomic:
      SemanticNoteAtLocation(constraint->location,
                             "because this constraint expression evaluated to false");
      return;
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
  SemanticNoteAtLocation(templ->location,
                         "candidate template ignored: constraints not satisfied");
  ReportConstraintFailure(templ->type->info.function.associated_constraint,
                          arguments);
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
    TypeParser parser;
    TypeParserInit(&parser, lex, syntax, STO(auto), kParsingPrototype);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* param = TypeParserParseDeclarator(&parser, type);
    TypeParserDestruct(&parser);
    if (param != NULL) {
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
