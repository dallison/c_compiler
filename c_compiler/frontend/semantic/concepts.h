//
//  concepts.h
//  c_compiler
//
//  C++20 concepts and constraints.
//
//  This module owns the representation of concept definitions, the normalized
//  constraint expression tree, constraint satisfaction checking, C++20
//  normalization/subsumption for partial ordering ([temp.constr.order]), and
//  the diagnostics emitted when a constraint is not satisfied.
//
//  Everything here is only meaningful in C++20 (or later) mode.  Below C++20
//  the `concept` and `requires` spellings lex as ordinary identifiers and none
//  of this machinery is reachable.
//

#ifndef concepts_h
#define concepts_h

#include <stdbool.h>

#include "dstring.h"
#include "source.h"
#include "vector.h"

struct ASTNode;
struct TypeRecord;
struct Symbol;
struct Syntax;
struct TemplateArgument;
struct RequiresExpr;

// Kinds of node in the normalized constraint expression tree ([temp.constr]).
// A constraint is either a conjunction/disjunction of constraints or one of the
// atomic forms (a bare boolean expression, a concept-id, or a requires-expr).
typedef enum {
  kConstraintAtomic,       // A single boolean primary constraint expression.
  kConstraintConjunction,  // left && right (short-circuits).
  kConstraintDisjunction,  // left || right (short-circuits).
  kConstraintConceptId,    // ConceptName or ConceptName<Args...>.
  kConstraintRequires,     // A requires-expression.
} ConstraintExprKind;

// A node in the constraint expression tree.  Constraint nodes are heap-owned and
// deleted explicitly; they reference AST nodes owned by the parser and
// reference-counted TypeRecords where needed.
typedef struct ConstraintExpr {
  ConstraintExprKind kind;
  SourceLocation location;  // Points directly at the sub-constraint's source.
  union {
    // kConstraintAtomic
    struct {
      struct ASTNode* expr;  // Unevaluated boolean expression.
    } atomic;
    // kConstraintConjunction / kConstraintDisjunction
    struct {
      struct ConstraintExpr* left;
      struct ConstraintExpr* right;
    } binary;
    // kConstraintConceptId
    struct {
      struct Symbol* concept_symbol;  // The named concept (flags.is_concept).
      Vector* arguments;              // TemplateArgument* (arena/owned).
    } concept_id;
    // kConstraintRequires
    struct {
      struct RequiresExpr* requires_expr;
    } requires_;
  } as;
} ConstraintExpr;

// The four requirement kinds inside a requires-expression ([expr.prim.req]).
typedef enum {
  kRequirementSimple,    // requires { expr; }
  kRequirementType,      // requires { typename T::member; }
  kRequirementCompound,  // requires { { expr } noexcept -> ConceptId; }
  kRequirementNested,    // requires { requires constraint-expression; }
} RequirementKind;

// A single requirement inside a requires-expression.
typedef struct Requirement {
  RequirementKind kind;
  SourceLocation location;
  struct ASTNode* expr;    // simple/compound: the tested expression.
  struct TypeRecord* type; // type requirement: the named type.
  bool is_noexcept;        // compound: `noexcept` was required.
  struct ConstraintExpr* return_type_constraint;  // compound: `-> ConceptId`.
  struct ConstraintExpr* nested;  // nested: the nested constraint expression.
} Requirement;

// A requires-expression: `requires (opt-params) { requirement-seq }`.
typedef struct RequiresExpr {
  SourceLocation location;
  Vector* parameters;    // Symbol* local parameters (may be NULL/empty).
  Vector* requirements;  // Requirement* entries.
} RequiresExpr;

// A concept definition: `template<params> concept Name = constraint-expr;`.
// Attached to the concept's Symbol via Symbol::concept_definition.
typedef struct Concept {
  String name;
  Vector* template_parameters;  // TemplateParameter* entries (owned).
  ConstraintExpr* constraint;   // The concept's constraint expression.
  SourceLocation location;
} Concept;

// ---------------------------------------------------------------------------
// Constructors (heap allocated).
// ---------------------------------------------------------------------------

ConstraintExpr* NewAtomicConstraint(struct ASTNode* expr,
                                    SourceLocation location);
ConstraintExpr* NewConjunctionConstraint(ConstraintExpr* left,
                                         ConstraintExpr* right,
                                         SourceLocation location);
ConstraintExpr* NewDisjunctionConstraint(ConstraintExpr* left,
                                         ConstraintExpr* right,
                                         SourceLocation location);
ConstraintExpr* NewConceptIdConstraint(struct Symbol* concept_symbol,
                                       Vector* arguments,
                                       SourceLocation location);
ConstraintExpr* NewRequiresConstraint(struct RequiresExpr* requires_expr,
                                      SourceLocation location);

Requirement* NewSimpleRequirement(struct ASTNode* expr, SourceLocation location);
Requirement* NewTypeRequirement(struct TypeRecord* type,
                                SourceLocation location);
Requirement* NewCompoundRequirement(struct ASTNode* expr, bool is_noexcept,
                                    ConstraintExpr* return_type_constraint,
                                    SourceLocation location);
Requirement* NewNestedRequirement(ConstraintExpr* nested,
                                  SourceLocation location);

RequiresExpr* NewRequiresExpr(Vector* parameters, Vector* requirements,
                              SourceLocation location);

Concept* NewConcept(const char* name, Vector* template_parameters,
                    ConstraintExpr* constraint, SourceLocation location);
void ConstraintExprDelete(ConstraintExpr* constraint);
void RequirementDelete(Requirement* requirement);
void RequiresExprDelete(RequiresExpr* expr);
void ConceptDelete(Concept* concept);
ConstraintExpr* ConceptsCloneConstraint(ConstraintExpr* constraint);
ConstraintExpr* ConceptsSubstituteConstraint(
    struct Syntax* syntax, ConstraintExpr* constraint, Vector* arguments,
    int rebase_base);
bool ConceptsConstraintContainsTemplateParameter(ConstraintExpr* constraint);

// Evaluates a concept-id expression represented by an identifier node.  This is
// the semantic bridge used by constant-expression contexts such as
// `static_assert(C<T>)`.
bool ConceptsEvaluateInteger(struct ASTNode* node, int64_t* result);
bool ConceptsEvaluateConstraint(ConstraintExpr* constraint, int64_t* result);
bool ConceptsEvaluateConstraintWithArguments(ConstraintExpr* constraint,
                                             Vector* arguments,
                                             int64_t* result);
bool ConceptsConstraintSatisfied(ConstraintExpr* constraint, Vector* arguments);
bool ConceptsHasAssociatedConstraint(ConstraintExpr* constraint);
void ConceptsReportAssociatedConstraintFailure(ConstraintExpr* constraint,
                                               Vector* arguments,
                                               SourceLocation location,
                                               const char* summary);
int ConceptsCompareAssociatedConstraints(
    ConstraintExpr* left, ConstraintExpr* right,
    Vector* left_parameter_mapping, Vector* right_parameter_mapping);
bool ConceptsFunctionTemplateConstraintsSatisfied(struct Symbol* templ,
                                                 Vector* arguments);
bool ConceptsFunctionTemplateHasAssociatedConstraint(struct Symbol* templ);
bool ConceptsFunctionTemplateConstraintsEquivalent(struct Symbol* left,
                                                  struct Symbol* right);
int ConceptsCompareFunctionTemplateConstraints(struct Symbol* left,
                                               struct Symbol* right);
void ConceptsReportFunctionTemplateConstraintFailure(struct Symbol* templ,
                                                     Vector* arguments);
ConstraintExpr* ConceptsParseRequiresClause(struct Syntax* syntax);
ConstraintExpr* ConceptsParseRequiresExpression(struct Syntax* syntax);

// Parses a C++20 concept definition after `template<...>` has already been
// consumed and the lexer is looking at `concept`.  Returns an empty declaration
// list on success or after recovery; returns NULL when the current token is not
// `concept`, so callers can continue with ordinary template declarations.
struct ASTNode* ConceptsParseDefinition(struct Syntax* syntax,
                                        SourceLocation template_location);

#endif /* concepts_h */
