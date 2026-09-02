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

// Defined in type_parse.c (declared in the type subsystem's internal header,
// which cannot be included here without colliding with concepts.c's own
// file-local helpers).  Builds the `decltype((expr))` reference type.
TypeRecord* NewDecltypeReference(TypeRecord* expr_type, bool rvalue);

static void ReportConstraintFailure(ConstraintExpr* constraint, Vector* arguments);
static bool ConstraintSubsumesWithMapping(ConstraintExpr* stronger,
                                          ConstraintExpr* weaker,
                                          Vector* parameter_mapping);
static int FoldConstraintPackIndex(ASTNode* pattern);

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
                                    ASTNode* noexcept_condition,
                                    ConstraintExpr* return_type_constraint,
                                    SourceLocation location) {
  Requirement* r = NewRequirement(kRequirementCompound, location);
  r->expr = expr;
  r->is_noexcept = is_noexcept;
  r->noexcept_condition = noexcept_condition;
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

typedef struct ConceptSatisfactionCacheEntry {
  Vector* arguments;
  bool ok;
  int64_t result;
} ConceptSatisfactionCacheEntry;

static void ConceptSatisfactionCacheEntryDelete(
    ConceptSatisfactionCacheEntry* entry) {
  if (entry == NULL) {
    return;
  }
  VectorDeleteWithContents(
      entry->arguments, (VectorElementDestructor)TemplateArgumentDelete,
      /*free_element=*/false);
  free(entry);
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
  if (concept->satisfaction_cache != NULL) {
    VectorDeleteWithContents(
        concept->satisfaction_cache,
        (VectorElementDestructor)ConceptSatisfactionCacheEntryDelete,
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
          ASTNodeClone(requirement->noexcept_condition, IdentityCloneNode, NULL,
                       NULL),
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

static Requirement* RequirementSubstitute(Syntax* syntax,
                                          Requirement* requirement,
                                          Vector* arguments,
                                          int rebase_base);

static RequiresExpr* RequiresExprSubstitute(Syntax* syntax, RequiresExpr* expr,
                                            Vector* arguments,
                                            int rebase_base) {
  if (expr == NULL) {
    return NULL;
  }
  Vector* parameters = NewVector();
  for (size_t i = 0;
       expr->parameters != NULL && i < expr->parameters->length; i++) {
    Symbol* parameter = expr->parameters->value.p[i];
    if (parameter == NULL) {
      VectorAppend(parameters, NULL);
      continue;
    }
    TypeRecord* type = TypeSubstituteTemplateTypeAndRebase(
        syntax, parameter->type, arguments, rebase_base);
    Symbol* copy =
        NewSymbol(parameter->name.value, type, parameter->storage);
    copy->flags = parameter->flags;
    copy->location = parameter->location;
    copy->value = parameter->value;
    copy->template_parameter_index = parameter->template_parameter_index;
    if (copy->template_parameter_index >= rebase_base) {
      copy->template_parameter_index -= rebase_base;
    }
    copy->dependent_value_template_parameter_index =
        parameter->dependent_value_template_parameter_index;
    if (copy->dependent_value_template_parameter_index >= rebase_base) {
      copy->dependent_value_template_parameter_index -= rebase_base;
    }
    VectorAppend(parameters, copy);
  }
  Vector* requirements = NewVector();
  for (size_t i = 0;
       expr->requirements != NULL && i < expr->requirements->length; i++) {
    VectorAppend(requirements,
                 RequirementSubstitute(syntax,
                                       expr->requirements->value.p[i],
                                       arguments, rebase_base));
  }
  return NewRequiresExpr(parameters, requirements, expr->location);
}

static ConstraintExpr* ConceptsSubstituteConstraintImpl(
    Syntax* syntax, ConstraintExpr* constraint, Vector* arguments,
    int rebase_base, Struct* from_owner, Struct* to_owner) {
  if (constraint == NULL) {
    return NULL;
  }
  switch (constraint->kind) {
    case kConstraintAtomic:
      return NewAtomicConstraint(
          TypeSubstituteMemberTemplateExpressionAndRebase(
              syntax, constraint->as.atomic.expr, arguments, rebase_base,
              constraint->location, from_owner, to_owner),
          constraint->location);
    case kConstraintConjunction:
      return NewConjunctionConstraint(
          ConceptsSubstituteConstraintImpl(
              syntax, constraint->as.binary.left, arguments, rebase_base,
              from_owner, to_owner),
          ConceptsSubstituteConstraintImpl(
              syntax, constraint->as.binary.right, arguments, rebase_base,
              from_owner, to_owner),
          constraint->location);
    case kConstraintDisjunction:
      return NewDisjunctionConstraint(
          ConceptsSubstituteConstraintImpl(
              syntax, constraint->as.binary.left, arguments, rebase_base,
              from_owner, to_owner),
          ConceptsSubstituteConstraintImpl(
              syntax, constraint->as.binary.right, arguments, rebase_base,
              from_owner, to_owner),
          constraint->location);
    case kConstraintConceptId:
      return NewConceptIdConstraint(
          constraint->as.concept_id.concept_symbol,
          TypeSubstituteTemplateArgumentVectorAndRebase(
              syntax, constraint->as.concept_id.arguments, arguments,
              rebase_base),
          constraint->location);
    case kConstraintRequires:
      return NewRequiresConstraint(
          RequiresExprSubstitute(syntax,
                                 constraint->as.requires_.requires_expr,
                                 arguments, rebase_base),
          constraint->location);
  }
  return NULL;
}

ConstraintExpr* ConceptsSubstituteConstraint(Syntax* syntax,
                                             ConstraintExpr* constraint,
                                             Vector* arguments,
                                             int rebase_base) {
  return ConceptsSubstituteConstraintImpl(
      syntax, constraint, arguments, rebase_base, NULL, NULL);
}

ConstraintExpr* ConceptsSubstituteMemberConstraint(
    Syntax* syntax, ConstraintExpr* constraint, Vector* arguments,
    int rebase_base, Struct* from_owner, Struct* to_owner) {
  return ConceptsSubstituteConstraintImpl(
      syntax, constraint, arguments, rebase_base, from_owner, to_owner);
}

static Requirement* RequirementSubstitute(Syntax* syntax,
                                          Requirement* requirement,
                                          Vector* arguments,
                                          int rebase_base) {
  if (requirement == NULL) {
    return NULL;
  }
  switch (requirement->kind) {
    case kRequirementSimple:
      return NewSimpleRequirement(
          TypeSubstituteTemplateExpressionAndRebase(
              syntax, requirement->expr, arguments, rebase_base,
              requirement->location),
          requirement->location);
    case kRequirementType:
      return NewTypeRequirement(
          TypeSubstituteTemplateTypeAndRebase(
              syntax, requirement->type, arguments, rebase_base),
          requirement->location);
    case kRequirementCompound:
      return NewCompoundRequirement(
          TypeSubstituteTemplateExpressionAndRebase(
              syntax, requirement->expr, arguments, rebase_base,
              requirement->location),
          requirement->is_noexcept,
          TypeSubstituteTemplateExpressionAndRebase(
              syntax, requirement->noexcept_condition, arguments, rebase_base,
              requirement->location),
          ConceptsSubstituteConstraint(
              syntax, requirement->return_type_constraint, arguments,
              rebase_base),
          requirement->location);
    case kRequirementNested:
      return NewNestedRequirement(
          ConceptsSubstituteConstraint(syntax, requirement->nested, arguments,
                                       rebase_base),
          requirement->location);
  }
  return NULL;
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
             ExpressionContainsTemplateParameter(
                 requirement->noexcept_condition) ||
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
  // The return-type requirement checks `C<decltype((E))>`.  Build that
  // decltype type with the same helper the decltype operator uses so its
  // structure matches a written-out type (e.g. `int*&`) exactly.
  if (expr->value_category == kValueCategoryLvalue ||
      expr->value_category == kValueCategoryXvalue) {
    return NewDecltypeReference(
        expr->type, expr->value_category == kValueCategoryXvalue);
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
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      (expr->flags & kASTFoldExpression) != 0 &&
      (expr->op == AST_OP(logand) || expr->op == AST_OP(logor))) {
    BinaryASTNode* fold = (BinaryASTNode*)expr;
    ASTNode* pattern = (expr->flags & kASTFoldPackOnLeft) != 0
                           ? fold->left
                           : fold->right;
    int pack_index = FoldConstraintPackIndex(pattern);
    if (pack_index >= 0 && arguments != NULL &&
        (size_t)pack_index < arguments->length) {
      TemplateArgument* pack = arguments->value.p[pack_index];
      if (pack != NULL && pack->pack_arguments != NULL &&
          pack->pack_arguments->length == 0) {
        int64_t identity = expr->op == AST_OP(logand) ? 1 : 0;
        if (result != NULL) {
          *result = identity;
        }
        return identity != 0 ? kAtomicConstraintOk : kAtomicConstraintFalse;
      }
    }
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
  // The constraint is interpreted below, so a call inside it has to keep its
  // call boundary: inlining leaves a body the interpreter rejects, which would
  // read as an unsatisfied constraint rather than as the miscompile it is.
  compiler->constant_evaluation_required_depth++;
  evaluated = AnalyzeExpression(evaluated);
  compiler->constant_evaluation_required_depth--;
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

bool ExpressionPotentiallyThrows(ASTNode* node) {
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
  kRequirementNoexceptConditionInvalid,
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

// Pristine (never-substituted) type patterns for requires-expression
// parameters.  A requires-expression's parameter Symbols are shared across
// every satisfaction check of the owning concept, and EvaluateExpressionRequirement
// temporarily overwrites each parameter's `type` with the concrete, substituted
// type so the tested expression analyzes against real types.  When a concept is
// checked *re-entrantly* (e.g. `range<T>` -> `ranges::begin` overloading ->
// `__member_begin<T>` -> ... -> `range<U>` again), an inner evaluation would
// otherwise read the outer evaluation's already-substituted concrete type as its
// substitution pattern and re-instantiate it, which for a constrained class
// template (e.g. `ref_view`) re-checks the class's associated constraint and
// spins forever.  Capturing each parameter's original, template-parameter-bearing
// pattern the first time it is seen (always the outermost, pre-mutation call) and
// substituting from that pristine pattern keeps re-entrant checks correct.
typedef struct PristineParamPattern {
  Symbol* param;
  TypeRecord* pattern;  // Owned; kept alive for the whole compilation.
} PristineParamPattern;

static Vector g_pristine_param_patterns;
static bool g_pristine_param_patterns_init = false;

static TypeRecord* PristineParameterPattern(Symbol* param) {
  if (!g_pristine_param_patterns_init) {
    VectorInit(&g_pristine_param_patterns);
    g_pristine_param_patterns_init = true;
  }
  for (size_t i = 0; i < g_pristine_param_patterns.length; i++) {
    PristineParamPattern* entry = g_pristine_param_patterns.value.p[i];
    if (entry->param == param) {
      return entry->pattern;
    }
  }
  // First (outermost) sighting: param->type is still the pristine pattern.
  TypeRecord* copy = TypeRecordCopy(param->type);
  TypeRecordIncRef(copy);
  PristineParamPattern* entry = malloc(sizeof(*entry));
  entry->param = param;
  entry->pattern = copy;
  VectorAppend(&g_pristine_param_patterns, entry);
  return copy;
}

static bool EvaluateCompoundNoexceptCondition(Requirement* requirement,
                                              Vector* arguments,
                                              bool* require_noexcept) {
  *require_noexcept = requirement->is_noexcept;
  if (!requirement->is_noexcept || requirement->noexcept_condition == NULL) {
    return true;
  }
  ASTNode* condition = TypeSubstituteTemplateExpression(
      &compiler->syntax, requirement->noexcept_condition, arguments,
      requirement->location);
  if (condition == NULL || DiagnosticErrorTrapped()) {
    ASTNodeDelete(condition);
    return false;
  }
  ASTNodeVisit(condition, ClearRequirementExpressionAnalysis, 0, NULL);
  DiagnosticSuppressBegin();
  // Interpreted below, so the call boundaries within it have to survive.
  compiler->constant_evaluation_required_depth++;
  condition = AnalyzeExpression(condition);
  compiler->constant_evaluation_required_depth--;
  bool failed = condition == NULL || DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  if (failed) {
    ASTNodeDelete(condition);
    return false;
  }
  TypeRecord* type = ConstraintExpressionPrvalueType(condition);
  int64_t value = 0;
  bool valid = false;
  if (type != NULL && (TypeIsBool(type) || TypeIsIntegral(type))) {
    valid = EvaluateIntegerExpression(condition, &value) &&
            (TypeIsBool(type) || value == 0 || value == 1);
  } else if (type != NULL) {
    ASTNode* holder = NewExpressionStatementASTNode(
        condition, requirement->location);
    DiagnosticSuppressBegin();
    SemanticConvertType(condition,
                        NewTypeRecordWithSize(kTypeBool, kQualPlain),
                        kConvertContextualBool);
    condition = ASTNodeMove(((ExpressionStatementASTNode*)holder)->expr);
    failed = DiagnosticErrorTrapped() || condition == NULL ||
             !TypeIsBool(condition->type);
    DiagnosticSuppressEnd();
    ASTNodeDelete(holder);
    if (!failed) {
      valid = EvaluateIntegerExpression(condition, &value);
    }
  }
  ASTNodeDelete(condition);
  if (valid) {
    *require_noexcept = value != 0;
  }
  return valid;
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
      TypeRecord* parameter_pattern =
          TypeRecordCopy(PristineParameterPattern(param));
      TypeRecord* concrete =
          TypeSubstituteTemplateType(&compiler->syntax, parameter_pattern,
                                     arguments);
      TypeRecordDelete(parameter_pattern);
      // SymbolSetType takes a reference on `concrete`.  The substitution result
      // is a freshly created (refs == 0) type, so the symbol becomes its sole
      // owner; deleting it here would drop the count back to zero and free the
      // type out from under the parameter (nulling nested pointees).  The
      // original type is restored (and this one released) after evaluation.
      SymbolSetType(param, concrete);
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
  bool require_noexcept = requirement->is_noexcept;
  if (!failed && !EvaluateCompoundNoexceptCondition(
                     requirement, arguments, &require_noexcept)) {
    failed = true;
    if (failure_info != NULL) {
      failure_info->reason = kRequirementNoexceptConditionInvalid;
    }
  }
  if (!failed && require_noexcept &&
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

static bool EvaluateTypeRequirement(Requirement* requirement,
                                    Vector* arguments) {
  if (requirement == NULL || requirement->type == NULL) {
    return false;
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  TypeRecord* concrete =
      TypeSubstituteTemplateType(&compiler->syntax, requirement->type,
                                 arguments);
  bool trapped = DiagnosticErrorTrapped();
  bool failed = concrete == NULL || trapped ||
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
    case kRequirementNoexceptConditionInvalid:
      return "because this compound requirement noexcept condition is not a contextually converted constant expression of type bool";
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
      Symbol* concept_symbol = constraint->as.concept_id.concept_symbol;
      if (concept_symbol != NULL &&
          concept_symbol->flags.is_template_template_parameter &&
          concept_symbol->template_template_parameter_kind ==
              kTemplateTemplateParameterConcept &&
          concept_symbol->template_parameter_index >= 0 && arguments != NULL &&
          (size_t)concept_symbol->template_parameter_index <
              arguments->length) {
        TemplateArgument* actual =
            arguments->value.p[concept_symbol->template_parameter_index];
        if (actual != NULL && actual->kind == kTemplateParameterTemplate &&
            actual->template_symbol != NULL) {
          concept_symbol = actual->template_symbol;
        }
      }
      Vector* concrete_args = TypeSubstituteTemplateArgumentVector(
          &compiler->syntax, constraint->as.concept_id.arguments, arguments);
      bool ok = EvaluateConceptDefinitionInteger(
          concept_symbol, concrete_args,
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

static ConceptSatisfactionCacheEntry* FindConceptSatisfactionCacheEntry(
    Concept* concept, Vector* arguments) {
  if (concept == NULL || concept->satisfaction_cache == NULL ||
      arguments == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < concept->satisfaction_cache->length; i++) {
    ConceptSatisfactionCacheEntry* entry =
        concept->satisfaction_cache->value.p[i];
    if (entry != NULL &&
        TypeTemplateArgumentVectorEqual(entry->arguments, arguments)) {
      return entry;
    }
  }
  return NULL;
}

static void CacheConceptSatisfaction(Concept* concept, Vector* arguments,
                                     bool ok, int64_t result) {
  if (concept->satisfaction_cache == NULL) {
    concept->satisfaction_cache = NewVector();
  }
  ConceptSatisfactionCacheEntry* entry = malloc(sizeof(*entry));
  entry->arguments = TemplateArgumentVectorCopy(arguments);
  entry->ok = ok;
  entry->result = result;
  VectorAppend(concept->satisfaction_cache, entry);
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

  // Normalize the argument list to one entry per declared parameter. Besides
  // filling defaults, this gathers a flat trailing argument sequence into a
  // parameter pack. The latter is required even when the supplied argument
  // count exceeds the number of parameters, e.g. regular_invocable<F, A, B>
  // must evaluate its definition with Args bound to [A, B], not just A.
  Vector* completed =
      arguments != NULL && definition->template_parameters != NULL
          ? TypeCompleteConceptArguments(
                &compiler->syntax, definition->template_parameters, arguments)
          : NULL;
  Vector* eval_arguments = completed != NULL ? completed : arguments;
  bool cacheable =
      eval_arguments != NULL &&
      !TemplateArgumentVectorContainsTemplateParameter(eval_arguments);
  if (cacheable) {
    ConceptSatisfactionCacheEntry* cached =
        FindConceptSatisfactionCacheEntry(definition, eval_arguments);
    if (cached != NULL) {
      *result = cached->result;
      if (completed != NULL) {
        VectorDeleteWithContents(
            completed, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      return cached->ok;
    }
  }

  bool ok;
  if (eval_arguments == NULL || constraint->kind != kConstraintAtomic) {
    ok = EvaluateConstraintInteger(constraint, eval_arguments, result);
  } else {
    ok = EvaluateAtomicConstraintInteger(constraint->as.atomic.expr,
                                         eval_arguments, location, result);
  }
  // Failed checks can be observed during a re-entrant, speculative
  // substitution while requires-expression parameter symbols are temporarily
  // rebound.  A later check from the owning context may succeed, so only
  // memoize stable successful satisfactions.
  if (cacheable && ok && *result != 0) {
    CacheConceptSatisfaction(definition, eval_arguments, ok, *result);
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
  Symbol* definition =
      templ != NULL && templ->value.func_defn != NULL
          ? templ->value.func_defn
          : templ;
  int enclosing_count =
      definition != NULL && definition->type != NULL &&
              TypeIsFunction(definition->type)
          ? definition->type->info.function.template_parameter_base
          : 0;
  Vector* enclosing_args =
      templ != NULL && templ->type != NULL
          ? templ->type->template_arguments
          : NULL;
  if (definition != templ &&
      ConceptsFunctionTemplateHasAssociatedConstraint(definition)) {
    if (enclosing_count <= 0) {
      return ConceptsConstraintSatisfied(
          definition->type->info.function.associated_constraint, arguments);
    }
    if (enclosing_args == NULL ||
        enclosing_args->length < (size_t)enclosing_count) {
      return false;
    }
    Vector combined;
    VectorInit(&combined);
    for (int i = 0; i < enclosing_count; i++) {
      VectorAppend(&combined, enclosing_args->value.p[i]);
    }
    for (size_t i = 0; arguments != NULL && i < arguments->length; i++) {
      VectorAppend(&combined, arguments->value.p[i]);
    }
    bool satisfied = ConceptsConstraintSatisfied(
        definition->type->info.function.associated_constraint, &combined);
    VectorDestruct(&combined);
    return satisfied;
  }
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
  TemplateArgument* copy = TemplateArgumentAlloc();
  copy->kind = arg->kind;
  copy->is_pack_expansion = arg->is_pack_expansion;
  copy->references_parameter_pack = arg->references_parameter_pack;
  copy->type = arg->type != NULL ? TypeRecordCopy(arg->type) : NULL;
  copy->int_value = arg->int_value;
  copy->template_parameter_index = arg->template_parameter_index;
  copy->pack_arguments = TemplateArgumentVectorCopy(arg->pack_arguments);
  copy->dependent_expr = arg->dependent_expr;
  copy->location = arg->location;
  copy->value_kind = arg->value_kind;
  copy->value_symbol = arg->value_symbol;
  copy->value_offset = arg->value_offset;
  copy->value_adjustment = arg->value_adjustment;
  copy->member_function = arg->member_function;
  copy->template_symbol = arg->template_symbol;
  copy->reflection_value = arg->reflection_value;
  copy->pack_index_expr = arg->pack_index_expr;
  copy->object_initializer =
      ASTNodeClone(arg->object_initializer, IdentityCloneNode, NULL, NULL);
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
  if (left->kind == kTemplateParameterTemplate) {
    return TemplateArgumentEqual(left, right);
  }
  return TemplateArgumentValuesEqual(left, right);
}

typedef struct NormalizedAtomic {
  ASTNode* expr;
  RequiresExpr* requires_expr;
  ASTNode* fold_pattern;
  ASTOpcode fold_operator;
  int fold_pack_index;
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
  atom->fold_operator = AST_OP(bad);
  atom->fold_pack_index = -1;
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

static Vector* NewIdentityParameterMappingFromParameters(Vector* parameters) {
  Vector* mapping = NewVector();
  if (parameters == NULL) {
    return mapping;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    TemplateArgument* arg = TemplateArgumentAlloc();
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

static Vector* NewIdentityParameterMapping(Symbol* templ) {
  return NewIdentityParameterMappingFromParameters(
      FunctionTemplateParameters(templ));
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

static int FoldConstraintTemplateArgumentPackIndex(TemplateArgument* arg) {
  if (arg == NULL) {
    return -1;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      int index = FoldConstraintTemplateArgumentPackIndex(
          arg->pack_arguments->value.p[i]);
      if (index >= 0) {
        return index;
      }
    }
  }
  if (!arg->references_parameter_pack && !arg->is_pack_expansion) {
    return -1;
  }
  if (arg->template_parameter_index >= 0) {
    return arg->template_parameter_index;
  }
  for (TypeRecord* type = arg->type; type != NULL; type = type->next) {
    if (type->template_parameter_index >= 0) {
      return type->template_parameter_index;
    }
  }
  return -1;
}

typedef struct {
  int index;
} FoldConstraintPackSearch;

static void FindFoldConstraintPack(ASTNode* node, void* data, int child_id,
                                   VisitorMode mode) {
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren) {
    return;
  }
  FoldConstraintPackSearch* search = data;
  if (search->index >= 0) {
    return;
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
      search->index = id->symbol->template_parameter_index;
      return;
    }
  }
  if (node->op == AST_OP(sizeof)) {
    SizeofASTNode* sizeof_node = (SizeofASTNode*)node;
    for (TypeRecord* type = sizeof_node->type_operand; type != NULL;
         type = type->next) {
      if (type->template_parameter_index >= 0) {
        search->index = type->template_parameter_index;
        return;
      }
    }
  }
  Vector* arguments = NULL;
  switch (ASTNodeGetShape(node)) {
    case kASTShapeIdentifier:
      arguments = ((IdentifierASTNode*)node)->template_arguments;
      break;
    case kASTShapeStructMember:
      arguments = ((StructMemberASTNode*)node)->template_arguments;
      break;
    case kASTShapeConstant:
      arguments = ((ConstantASTNode*)node)->template_arguments;
      break;
    default:
      break;
  }
  for (size_t i = 0; arguments != NULL && i < arguments->length; i++) {
    int index =
        FoldConstraintTemplateArgumentPackIndex(arguments->value.p[i]);
    if (index >= 0) {
      search->index = index;
      return;
    }
  }
}

static int FoldConstraintPackIndex(ASTNode* pattern) {
  FoldConstraintPackSearch search = {.index = -1};
  ASTNodeVisit(pattern, FindFoldConstraintPack, 0, &search);
  return search.index;
}

static NormalizedAtomic* NewNormalizedFoldAtomic(
    ASTNode* fold, ASTNode* pattern, Vector* parameter_mapping,
    SourceLocation location) {
  NormalizedAtomic* atom = NewNormalizedAtomic(
      fold, NULL, CopyParameterMapping(parameter_mapping), location);
  atom->fold_pattern = pattern;
  atom->fold_operator = fold != NULL ? fold->op : AST_OP(bad);
  atom->fold_pack_index = FoldConstraintPackIndex(pattern);
  return atom;
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
  NormalizedAtomic* copy = NewNormalizedAtomic(
      atom->expr, atom->requires_expr,
      CopyParameterMapping(atom->parameter_mapping), atom->location);
  copy->fold_pattern = atom->fold_pattern;
  copy->fold_operator = atom->fold_operator;
  copy->fold_pack_index = atom->fold_pack_index;
  return copy;
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
  if (expr == NULL) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        expr, NULL, CopyParameterMapping(parameter_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  if (ExprIsFoldConstraint(expr)) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
        (expr->op != AST_OP(logand) && expr->op != AST_OP(logor))) {
      NormalizedAtomic* atom = NewNormalizedAtomic(
          expr, NULL, CopyParameterMapping(parameter_mapping), location);
      return NormalizedFormFromAtom(atom);
    }
    BinaryASTNode* fold = (BinaryASTNode*)expr;
    bool pack_on_left = (expr->flags & kASTFoldPackOnLeft) != 0;
    ASTNode* pattern = pack_on_left ? fold->left : fold->right;
    ASTNode* seed = pack_on_left ? fold->right : fold->left;
    NormalizedConstraintForm* folded = NormalizedFormFromAtom(
        NewNormalizedFoldAtomic(expr, pattern, parameter_mapping, location));
    if (seed == NULL) {
      return folded;
    }
    NormalizedConstraintForm* initial = NormalizeAtomicExpressionToDnf(
        seed, seed->location, parameter_mapping, expanding);
    return expr->op == AST_OP(logand)
               ? NormalizedFormDnfConjoin(folded, initial)
               : NormalizedFormDnfDisjoin(folded, initial);
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
  if (expr == NULL) {
    NormalizedAtomic* atom = NewNormalizedAtomic(
        expr, NULL, CopyParameterMapping(parameter_mapping), location);
    return NormalizedFormFromAtom(atom);
  }
  if (ExprIsFoldConstraint(expr)) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
        (expr->op != AST_OP(logand) && expr->op != AST_OP(logor))) {
      NormalizedAtomic* atom = NewNormalizedAtomic(
          expr, NULL, CopyParameterMapping(parameter_mapping), location);
      return NormalizedFormFromAtom(atom);
    }
    BinaryASTNode* fold = (BinaryASTNode*)expr;
    bool pack_on_left = (expr->flags & kASTFoldPackOnLeft) != 0;
    ASTNode* pattern = pack_on_left ? fold->left : fold->right;
    ASTNode* seed = pack_on_left ? fold->right : fold->left;
    NormalizedConstraintForm* folded = NormalizedFormFromAtom(
        NewNormalizedFoldAtomic(expr, pattern, parameter_mapping, location));
    if (seed == NULL) {
      return folded;
    }
    NormalizedConstraintForm* initial = NormalizeAtomicExpressionToCnf(
        seed, seed->location, parameter_mapping, expanding);
    return expr->op == AST_OP(logand)
               ? NormalizedFormCnfConjoin(folded, initial)
               : NormalizedFormCnfDisjoin(folded, initial);
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

static bool NormalizedFormSubsumes(NormalizedConstraintForm* stronger_dnf,
                                   NormalizedConstraintForm* weaker_cnf);

static bool NormalizedAtomSubsumes(NormalizedAtomic* stronger,
                                   NormalizedAtomic* weaker) {
  if (stronger == NULL || weaker == NULL) {
    return stronger == weaker;
  }
  bool stronger_is_fold = stronger->fold_pattern != NULL;
  bool weaker_is_fold = weaker->fold_pattern != NULL;
  if (!stronger_is_fold || !weaker_is_fold) {
    return !stronger_is_fold && !weaker_is_fold &&
           NormalizedAtomsIdentical(stronger, weaker);
  }
  if (stronger->fold_operator != weaker->fold_operator ||
      stronger->fold_pack_index < 0 || weaker->fold_pack_index < 0 ||
      stronger->fold_pack_index != weaker->fold_pack_index) {
    return false;
  }

  Set* expanding = NewSet(SymbolPointerCompare);
  NormalizedConstraintForm* stronger_dnf = NormalizeAtomicExpressionToDnf(
      stronger->fold_pattern, stronger->fold_pattern->location,
      stronger->parameter_mapping, expanding);
  NormalizedConstraintForm* weaker_cnf = NormalizeAtomicExpressionToCnf(
      weaker->fold_pattern, weaker->fold_pattern->location,
      weaker->parameter_mapping, expanding);
  bool result = NormalizedFormSubsumes(stronger_dnf, weaker_cnf);
  NormalizedConstraintFormDelete(stronger_dnf);
  NormalizedConstraintFormDelete(weaker_cnf);
  SetDelete(expanding);
  return result;
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
      if (NormalizedAtomSubsumes(disjunctive->atoms->value.p[i],
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

static void FindConceptTemplateParameterReference(ASTNode* node, void* data,
                                                  int child_id,
                                                  VisitorMode mode) {
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren || *(bool*)data ||
      node->op != AST_OP(identifier)) {
    return;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol != NULL && symbol->flags.is_template_template_parameter &&
      symbol->template_template_parameter_kind ==
          kTemplateTemplateParameterConcept) {
    *(bool*)data = true;
  }
}

bool ConceptsConstraintReferencesConceptTemplateParameter(
    ConstraintExpr* constraint) {
  if (constraint == NULL) {
    return false;
  }
  switch (constraint->kind) {
    case kConstraintConjunction:
    case kConstraintDisjunction:
      return ConceptsConstraintReferencesConceptTemplateParameter(
                 constraint->as.binary.left) ||
             ConceptsConstraintReferencesConceptTemplateParameter(
                 constraint->as.binary.right);
    case kConstraintConceptId: {
      Symbol* symbol = constraint->as.concept_id.concept_symbol;
      return symbol != NULL &&
             symbol->flags.is_template_template_parameter &&
             symbol->template_template_parameter_kind ==
                 kTemplateTemplateParameterConcept;
    }
    case kConstraintAtomic: {
      bool found = false;
      ASTNodeVisit(constraint->as.atomic.expr,
                   FindConceptTemplateParameterReference, 0, &found);
      return found;
    }
    case kConstraintRequires: {
      RequiresExpr* requires_expr = constraint->as.requires_.requires_expr;
      for (size_t i = 0;
           requires_expr != NULL && requires_expr->requirements != NULL &&
           i < requires_expr->requirements->length;
           i++) {
        Requirement* requirement = requires_expr->requirements->value.p[i];
        if (requirement == NULL) {
          continue;
        }
        bool found = false;
        ASTNodeVisit(requirement->expr, FindConceptTemplateParameterReference,
                     0, &found);
        if (found ||
            ConceptsConstraintReferencesConceptTemplateParameter(
                requirement->return_type_constraint) ||
            ConceptsConstraintReferencesConceptTemplateParameter(
                requirement->nested)) {
          return true;
        }
      }
      return false;
    }
  }
  return false;
}

static bool ConstraintSubsumesWithMapping(ConstraintExpr* stronger,
                                            ConstraintExpr* weaker,
                                            Vector* parameter_mapping) {
  if (stronger == NULL || weaker == NULL) {
    return stronger == weaker;
  }
  if (ConceptsConstraintReferencesConceptTemplateParameter(stronger) ||
      ConceptsConstraintReferencesConceptTemplateParameter(weaker)) {
    return false;
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

// Two associated constraints are equivalent iff each subsumes the other.  This
// is stricter than `ConceptsCompareAssociatedConstraints` returning 0, which
// also holds for merely *incomparable* constraints (neither subsumes).  Used to
// decide whether two same-pattern partial specializations are true redeclaration
// duplicates (equivalent) versus distinct constrained specializations.
bool ConceptsAssociatedConstraintsEquivalent(ConstraintExpr* left,
                                             Vector* left_parameters,
                                             ConstraintExpr* right,
                                             Vector* right_parameters) {
  bool left_constrained = ConceptsHasAssociatedConstraint(left);
  bool right_constrained = ConceptsHasAssociatedConstraint(right);
  if (!left_constrained && !right_constrained) {
    return true;
  }
  if (left_constrained != right_constrained) {
    return false;
  }
  Vector* left_mapping =
      NewIdentityParameterMappingFromParameters(left_parameters);
  Vector* right_mapping =
      NewIdentityParameterMappingFromParameters(right_parameters);
  bool equivalent =
      ConstraintSubsumesWithMapping(left, right, left_mapping) &&
      ConstraintSubsumesWithMapping(right, left, right_mapping);
  VectorDeleteWithContents(left_mapping,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  VectorDeleteWithContents(right_mapping,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return equivalent;
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
    parser.typename_allows_unqualified = true;
    TypeRecord* type = TypeParserParseType(&parser, true);
    TypeParserDestruct(&parser);
    SyntaxNeedSemicolon(syntax, TC(closebra) | TC(semicolon));
    return NewTypeRequirement(type, requirement_location);
  }
  if (LexMatch(lex, TOK(lbrace))) {
    ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebra) | TC(semicolon));
    bool is_noexcept = LexMatch(lex, TOK(noexcept));
    ASTNode* noexcept_condition = NULL;
    if (is_noexcept && LexMatch(lex, TOK(lparen))) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
        SyntaxError(syntax,
                    "Conditional noexcept compound requirements require "
                    "C++29");
      }
      noexcept_condition =
          SyntaxParseSingleExpression(syntax, TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rparen),
                        TC(semicolon) | TC(closebra));
    }
    ConstraintExpr* return_type_constraint = NULL;
    if (LexLookingAt(lex, TOK(arrow))) {
      SourceLocation arrow_location = lex->current_token_location;
      LexNextToken(lex);
      return_type_constraint = ParseConceptConstraintOr(syntax, arrow_location);
    }
    SyntaxNeedSemicolon(syntax, TC(closebra) | TC(semicolon));
    return NewCompoundRequirement(expr, is_noexcept, noexcept_condition,
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
  if ((expr->flags & kASTFoldExpression) == 0 &&
      (expr->op == AST_OP(logand) || expr->op == AST_OP(logor))) {
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
  // The parser's expression tree remains attached to transient syntax state
  // and can be destructed when that state is reset.  Constraints outlive that
  // tree (notably for deferred template instantiation and module emission), so
  // retain an independent AST with its own shape-specific storage.
  return NewAtomicConstraint(
      ASTNodeClone(expr, IdentityCloneNode, NULL, NULL), location);
}

// Parse a concept-id named by a nested-name-specifier, such as
// `std::default_initializable<V>` or `::my::pred<T>`.  A concept-id is not an
// expression, so the generic expression fallback cannot parse one; without this
// a qualified concept in a requires-clause is a syntax error.  Returns NULL with
// the lexer restored to its starting position when the tokens do not name a
// concept, letting the caller fall back to the expression form.
static ConstraintExpr* TryParseQualifiedConceptIdConstraint(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (!LexLookingAt(lex, TOK(identifier)) &&
      !LexLookingAt(lex, TOK(coloncolon))) {
    return NULL;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  SourceLocation concept_location = lex->current_token_location;

  FullyQualifiedIdentifier concept_id;
  FullyQualifiedIdentifierInit(&concept_id);
  Symbol* symbol = NULL;
  if (SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &concept_id,
                                                         TC(semicolon))) {
    symbol = SyntaxFindQualifiedSymbol(syntax, &concept_id);
  }
  if (symbol == NULL || !symbol->flags.is_concept ||
      (symbol->flags.is_template_template_parameter &&
       symbol->flags.is_parameter_pack)) {
    FullyQualifiedIdentifierDestruct(&concept_id);
    LexCheckpointRestore(lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  // Only the trailing component's template arguments belong to the concept; any
  // earlier ones qualify the enclosing class or namespace template.
  Vector* args = NULL;
  if (concept_id.template_arguments.length > 0) {
    Vector* concept_args = concept_id.template_arguments.value.p
        [concept_id.template_arguments.length - 1];
    if (concept_args != NULL) {
      args = TemplateArgumentVectorCopy(concept_args);
    }
  }
  if (args == NULL) {
    args = NewVector();
  }
  FullyQualifiedIdentifierDestruct(&concept_id);
  return NewConceptIdConstraint(symbol, args, concept_location);
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
    if (symbol != NULL && symbol->flags.is_concept &&
        !(symbol->flags.is_template_template_parameter &&
          symbol->flags.is_parameter_pack)) {
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
  ConstraintExpr* qualified = TryParseQualifiedConceptIdConstraint(syntax);
  if (qualified != NULL) {
    return qualified;
  }
  ASTNode* expr = SyntaxParseConstraintExpression(syntax, TC(semicolon));
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
