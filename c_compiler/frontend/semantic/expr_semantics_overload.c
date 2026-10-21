//
//  expr_semantics_overload.c
//  c_compiler
//
//  Free, member, and function-template overload resolution.
//

#include "expr_semantics.h"
#include "expr_semantics_internal.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "concepts.h"
#include "contracts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "statement_semantics.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "typo_correction.h"
#include "rtti.h"
#include "reflection.h"
#include "reflection_meta_synthesis.h"
#include "reflection_meta_traits.h"
#include "reflection_semantics.h"
#include "type_traits_semantics.h"
#include "type_compare.h"
#include "member_pointer.h"
#include "statement_parser.h"
#include "type_parse.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_special_member.h"
#include "type_template.h"

static bool CallActualIsPackExpansion(ASTNode* actual);
static size_t FunctionRequiredArgumentCount(TypeRecord* func, size_t first_formal_arg);
static bool ConstructorThisPointerMatches(TypeRecord* actual, TypeRecord* formal);
static bool SymbolIsOperatorCoAwait(Symbol* symbol);
static bool FunctionNameSkipsADL(String* name);
static bool CoAwaitActualIsLValue(ASTNode* actual);
static int CoAwaitOperatorActualScore(Symbol* candidate, ASTNode* actual);
static int OperatorCoAwaitCallScore(Symbol* candidate, VectorASTNode* node);
static int OverloadCallScore(Symbol* candidate, VectorASTNode* node);
static void AppendReadableType(TypeRecord* type, String* result);
static Symbol* TemplateOriginForConstraintOrdering(Symbol* candidate);
static int ConstraintTieBreak(Symbol* best, Symbol* candidate);
static bool DescribeFunctionNonViability(TypeRecord* func, VectorASTNode* node, size_t first_formal_arg, String* reason);
static const char* ImplicitSpecialMemberDescription(Symbol* candidate);
static void EmitCandidateNote(Symbol* candidate, const char* reason);
static void EmitFreeCandidateNotes(Vector* candidates, VectorASTNode* call, bool ambiguous, int best_score);
static void DeleteTemporaryFunctionTemplateCandidates(Vector* candidates, Symbol* keep);
static bool SameFunctionSignature(Symbol* left, Symbol* right);
static Symbol* ResolveFunctionCandidateVector(String* name, Vector* candidates, Vector* actuals, Vector* explicit_args, bool diagnose_no_match, bool diagnose_ambiguous, ASTNode* diagnostic_node);
static void AddOrdinaryUnqualifiedFunctionCandidates(String* name, Namespace* start, Vector* candidates);
static Symbol* FunctionTemplateOverloadCandidate(Symbol* candidate, VectorASTNode* node, Vector* explicit_args, Vector* temporary_candidates);
static Symbol* InstantiateSelectedFunctionTemplateCandidate(Symbol* selected);
static void EmitFunctionTemplateCandidateRejectionNote( Symbol* candidate, VectorASTNode* call, Vector* explicit_args, size_t first_formal_arg);
static int FunctionTemplatePatternTypeSpecificity(TypeRecord* type);
static int FunctionTemplatePatternSpecificity(Symbol* instantiated);
static Vector* ClassTemplateArgumentsFromMemberAccess( BinaryASTNode* member_access);
static bool MemberFunctionConstraintsSatisfied(StructMember* candidate, BinaryASTNode* member_access);
static ASTNode* ExplicitObjectReceiverForCall(BinaryASTNode* member_access, ASTNode* scratch);
static void BuildExplicitObjectCallActuals(VectorASTNode* call, BinaryASTNode* member_access, ASTNode* receiver_scratch, Vector* actuals);
static int MemberOverloadCallScore(StructMember* candidate, VectorASTNode* node, BinaryASTNode* member_access, bool check_receiver_const);
static int MemberCvTiePenalty(StructMember* candidate, BinaryASTNode* member_access);
static void EmitMemberCandidateNotes(StructMember* first, VectorASTNode* node, BinaryASTNode* member_access, Vector* explicit_args, bool ambiguous, int best_score);
static StructMember* MemberTemplateOverloadCandidate(StructMember* candidate, VectorASTNode* node, BinaryASTNode* member_access, Vector* explicit_args, Vector* temporary_members);
static void DeleteTemporaryMemberTemplateCandidates(Vector* members, StructMember* keep);
static StructMember* InstantiateSelectedMemberTemplateCandidate( StructMember* selected);
static bool MemberExplicitParametersEqual(StructMember* left, StructMember* right);
static bool DerivedConstructorHidesInherited(StructMember* derived, StructMember* inherited);
static bool CallHasBracedInitActual(VectorASTNode* node);
static bool CandidateIsInitializerListConstructor(StructMember* candidate);
static bool CallActualsContainUndeducedAuto(Vector* actuals);
static int FreeExactMatchScoreCeiling(Vector* actuals);
static void RankNonTemplateFreeFunctionCandidates(Vector* candidates,
                                                  VectorASTNode* call,
                                                  Symbol** best, int* best_score,
                                                  bool* ambiguous);
static void ApplyFreeFunctionEqualScoreTie(Symbol* effective, Symbol** best,
                                           bool* ambiguous);
static bool FunctionTemplateConversionSequencesTie(
    Symbol* left, Symbol* right, VectorASTNode* call);
static void RankFunctionTemplateCandidates(Vector* candidates,
                                           VectorASTNode* call,
                                           Vector* explicit_args,
                                           Vector* temporary_candidates,
                                           Vector* actuals, Symbol** best,
                                           int* best_score, bool* ambiguous);
static Symbol* DiagnoseFailedFreeFunctionResolution(
    String* name, Vector* candidates, Vector* actuals, Vector* explicit_args,
    bool diagnose_no_match, ASTNode* diagnostic_node,
    Vector* temporary_candidates);
static bool MemberCallOnlyInitializerListConstructors(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    bool explicit_args_present);
static void RankNonTemplateMemberCandidates(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    bool only_initializer_list_constructors, StructMember** best,
    int* best_score, bool* ambiguous, bool* receiver_const_rejected);
static void ApplyMemberNonTemplateEqualScoreTie(
    StructMember* candidate, StructMember** best, bool* ambiguous,
    BinaryASTNode* member_access);
static void RankMemberTemplateCandidates(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    Vector* explicit_args, bool only_initializer_list_constructors,
    Vector* temporary_members, StructMember** best, int* best_score,
    bool* ambiguous, bool* receiver_const_rejected);
static void ApplyMemberTemplateEqualScoreTie(StructMember* effective,
                                             StructMember** best,
                                             bool* ambiguous, int score,
                                             int best_score,
                                             BinaryASTNode* member_access);
static StructMember* DiagnoseFailedMemberOverload(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    Vector* explicit_args, bool receiver_const_rejected,
    Vector* temporary_members);
static bool TryReuseSelectedFunctionTemplateCallee(VectorASTNode* node,
                                                   IdentifierASTNode* id);
static void ResolveLoweredConstructorOverload(VectorASTNode* node,
                                              IdentifierASTNode* id);
static void GatherOverloadedCallCandidates(IdentifierASTNode* id,
                                             VectorASTNode* node,
                                             Vector* candidates,
                                             size_t* ordinary_count,
                                             bool* gathered_inline_overloads,
                                             bool* gathered_member_overloads,
                                             bool* has_adl_candidates);
static bool ShouldSkipOverloadedCallResolution(
    IdentifierASTNode* id, bool gathered_inline_overloads,
    bool gathered_member_overloads, bool has_adl_candidates);

// Contract: Read-only; true when an actual (possibly under unary/cast/member
// wrappers) is a pack expansion or names an unexpanded parameter-pack member.
static bool CallActualIsPackExpansion(ASTNode* actual) {
  if (actual == NULL) {
    return false;
  }
  if ((actual->flags & kASTPackExpansion) != 0) {
    return true;
  }
  if (actual->op == AST_OP(contents) || actual->op == AST_OP(address) ||
      actual->op == AST_OP(cast)) {
    ASTNode* sub = actual->op == AST_OP(cast)
                       ? ((CastASTNode*)actual)->expr
                       : ((UnaryASTNode*)actual)->sub;
    return CallActualIsPackExpansion(sub);
  }
  if (actual->op != AST_OP(dot) && actual->op != AST_OP(arrow)) {
    return false;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)actual;
  if (member_access->right == NULL ||
      member_access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member = (StructMemberASTNode*)member_access->right;
  return member->member != NULL && member->member->symbol != NULL &&
         member->member->symbol->flags.is_parameter_pack;
}

// Contract: Read-only scan of call actuals; any true CallActualIsPackExpansion
// suppresses strict arity scoring in FunctionCallScore.
bool CallHasPackExpansionActual(VectorASTNode* node) {
  if (node == NULL || node->children == NULL) {
    return false;
  }
  for (size_t i = 0; i < node->children->length; i++) {
    if (CallActualIsPackExpansion(node->children->value.p[i])) {
      return true;
    }
  }
  return false;
}

/* Contract: count of trailing formals (from `first_formal_arg`) without defaults;
 * used with arity checks in FunctionCallScore and diagnostics. */
static size_t FunctionRequiredArgumentCount(TypeRecord* func,
                                            size_t first_formal_arg) {
  size_t required = 0;
  if (func == NULL || !TypeIsFunction(func) ||
      first_formal_arg > func->info.function.prototype.length) {
    return required;
  }
  for (size_t i = first_formal_arg; i < func->info.function.prototype.length;
       i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && SymbolExtraOf(formal)->default_argument == NULL) {
      required = i - first_formal_arg + 1;
    }
  }
  return required;
}

/* Constructor `this` may name a private/protected base subobject of the
 * class currently being constructed (`class D : private B { D() : B(args) {} }`).
 * OverloadConversionRank uses public-only derived-to-base and would reject it. */
static bool ConstructorThisPointerMatches(TypeRecord* actual,
                                          TypeRecord* formal) {
  if (TypeEqualIgnoringQualifiers(actual, formal)) {
    return true;
  }
  if (actual == NULL || formal == NULL || !TypeIsPointer(actual) ||
      !TypeIsPointer(formal) || actual->next == NULL || formal->next == NULL ||
      !TypeIsStructOrUnion(actual->next) ||
      !TypeIsStructOrUnion(formal->next)) {
    return false;
  }
  return TypeBaseOffset(actual->next, formal->next, /*public_only=*/false,
                        NULL);
}

/* Contract: conversion-sequence rank sum for matching `node` to `func`, or -1 if
 * not viable.  `first_formal_arg` skips implicit object parameters. */
int FunctionCallScore(TypeRecord* func, VectorASTNode* node,
                             size_t first_formal_arg) {
  if (!TypeIsFunction(func) || func->info.function.unknown_args) {
    return -1;
  }
  if (CallHasPackExpansionActual(node)) {
    return 0;
  }
  size_t num_actual_args = node->children->length;
  size_t num_formal_args = func->info.function.prototype.length;
  if (first_formal_arg > num_formal_args) {
    return -1;
  }
  size_t num_user_formal_args = num_formal_args - first_formal_arg;
  size_t required_args =
      FunctionRequiredArgumentCount(func, first_formal_arg);
  if (num_actual_args < required_args) {
    return -1;
  }
  if (!func->info.function.varargs &&
      num_actual_args > num_user_formal_args) {
    return -1;
  }

  int score = 0;
  size_t num_checked_args =
      num_actual_args < num_user_formal_args ? num_actual_args
                                             : num_user_formal_args;
  for (size_t i = 0; i < num_checked_args; i++) {
    ASTNode* actual = (ASTNode*)node->children->value.p[i];
    Symbol* formal =
        (Symbol*)func->info.function.prototype.value.p[i + first_formal_arg];
    bool constructor_this =
        func->info.function.is_constructor &&
        i + first_formal_arg == 0 &&
        formal != NULL && strcmp(formal->name.value, "this") == 0;
    int rank =
        constructor_this &&
                ConstructorThisPointerMatches(actual->type, formal->type)
            ? 0
            : OverloadConversionRank(actual, formal->type);
    if (rank < 0) {
      return -1;
    }
    score += rank;
  }
  if (func->info.function.varargs && num_actual_args > num_user_formal_args) {
    // [over.ics.rank]: an ellipsis conversion sequence is worse than any
    // standard or user-defined conversion sequence.  Keep it viable for true
    // varargs calls, but do not let ignored tail arguments make `f(T, ...)`
    // look better than an overload with declared parameters for those arguments.
    score += 1000 * (int)(num_actual_args - num_user_formal_args);
  }
  return score;
}

// Contract: Read-only name test; used to route `operator co_await` out of
// ordinary overload conversion scoring (handled by coroutine rules instead).
static bool SymbolIsOperatorCoAwait(Symbol* symbol) {
  return symbol != NULL && strcmp(symbol->name.value, "operator co_await") == 0;
}

static bool FunctionNameSkipsADL(String* name) {
  return name != NULL &&
         (strcmp(name->value, "operator new") == 0 ||
          strcmp(name->value, "operator new[]") == 0 ||
          strcmp(name->value, "operator delete") == 0 ||
          strcmp(name->value, "operator delete[]") == 0);
}

static bool CoAwaitActualIsLValue(ASTNode* actual) {
  if (ASTNodeIsLValue(actual)) {
    return true;
  }
  return actual != NULL && actual->op == AST_OP(identifier) &&
         actual->type != NULL && !TypeIsFunction(actual->type);
}

/* Contract: ranks a unary `operator co_await` against one operand; -1 if ill-formed. */
static int CoAwaitOperatorActualScore(Symbol* candidate, ASTNode* actual) {
  if (candidate == NULL || candidate->type == NULL ||
      !TypeIsFunction(candidate->type) ||
      candidate->type->info.function.prototype.length != 1) {
    return -1;
  }
  Symbol* formal = candidate->type->info.function.prototype.value.p[0];
  if (actual == NULL || formal == NULL || formal->type == NULL) {
    return -1;
  }
  TypeRecord* target = TypeIsReference(formal->type) ? formal->type->next
                                                     : formal->type;
  if (TypeIsStructOrUnion(actual->type) || TypeIsStructOrUnion(target)) {
    if (!TypeIsStructOrUnion(actual->type) || !TypeIsStructOrUnion(target) ||
        actual->type->info.struct_info != target->info.struct_info) {
      return -1;
    }
  }
  if (TypeIsReference(formal->type)) {
    if (TypeIsConst(actual->type) && !TypeIsConst(target)) {
      return -1;
    }
    if (formal->type->declarator == kDeclRValueReference) {
      return CoAwaitActualIsLValue(actual) ? -1 : 0;
    }
    if (CoAwaitActualIsLValue(actual)) {
      return TypeIsConst(target) ? 1 : 0;
    }
    return TypeIsConst(target) ? 2 : -1;
  }
  return OverloadConversionRank(actual, formal->type);
}

/* Contract: ADL + ordinary lookup for `operator co_await` on `actual`; NULL if none. */
Symbol* ResolveFreeOperatorCoAwaitForActual(ASTNode* actual,
                                                   bool diagnose_ambiguous) {
  if (!CompilerIsCXX() || actual == NULL || actual->type == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "operator co_await");
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, actual);
  Vector candidates;
  VectorInit(&candidates);
  AddNamedFunctionCandidates(&name, compiler->global_namespace, &candidates);
  AddADLFunctionCandidates(&name, &actuals, &candidates);
  Symbol* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  VectorASTNode call = {0};
  call.children = &actuals;
  for (size_t i = 0; i < candidates.length; i++) {
    Symbol* candidate = candidates.value.p[i];
    if (candidate->type == NULL || !TypeIsFunction(candidate->type) ||
        candidate->type->next == NULL) {
      continue;
    }
    int score = OperatorCoAwaitCallScore(candidate, &call);
    if (score < 0) {
      continue;
    }
    if (best == NULL || score < best_score) {
      best = candidate;
      best_score = score;
      ambiguous = false;
    } else if (score == best_score) {
      ambiguous = true;
    }
  }
  if (ambiguous && diagnose_ambiguous) {
    SemanticError(actual, "Ambiguous overload for operator co_await");
  }
  VectorDestruct(&candidates);
  VectorDestruct(&actuals);
  StringDestruct(&name);
  return best;
}

static int OperatorCoAwaitCallScore(Symbol* candidate, VectorASTNode* node) {
  if (node == NULL || node->children == NULL || node->children->length != 1) {
    return -1;
  }
  return CoAwaitOperatorActualScore(candidate, node->children->value.p[0]);
}

/* Contract: dispatches to co_await or ordinary FunctionCallScore for `candidate`. */
static int OverloadCallScore(Symbol* candidate, VectorASTNode* node) {
  if (SymbolIsOperatorCoAwait(candidate)) {
    return OperatorCoAwaitCallScore(candidate, node);
  }
  return FunctionCallScore(candidate->type, node, 0);
}

// Appends a human-readable spelling of `type` to `result` for diagnostics.
static void AppendReadableType(TypeRecord* type, String* result) {
  if (type == NULL) {
    StringAppend(result, "<unknown>");
    return;
  }
  TypeRecordToString(type, result);
}

// Returns true if `candidate` is a function template (either an uninstantiated
// template or a specialization produced by instantiating one).
bool SymbolIsTemplateFunction(Symbol* candidate) {
  if (candidate == NULL) {
    return false;
  }
  if (candidate->flags.is_template) {
    return true;
  }
  return candidate->type != NULL && TypeIsFunction(candidate->type) &&
         candidate->type->info.function.template_origin != NULL;
}

/* Contract: primary template symbol used for [temp.func.order] constraint ties. */
static Symbol* TemplateOriginForConstraintOrdering(Symbol* candidate) {
  if (candidate == NULL) {
    return NULL;
  }
  if (candidate->flags.is_template) {
    return candidate;
  }
  if (candidate->type != NULL && TypeIsFunction(candidate->type)) {
    return candidate->type->info.function.template_origin;
  }
  return NULL;
}

/* Contract: ConceptsCompareFunctionTemplateConstraints(candidate, best). */
static int ConstraintTieBreak(Symbol* best, Symbol* candidate) {
  Symbol* best_origin = TemplateOriginForConstraintOrdering(best);
  Symbol* candidate_origin = TemplateOriginForConstraintOrdering(candidate);
  return ConceptsCompareFunctionTemplateConstraints(candidate_origin,
                                                   best_origin);
}

// Fills `reason` with an explanation of why the function type `func` is not a
// viable candidate for the call `node`, where `first_formal_arg` is the index of
// the first formal parameter matched against an explicit argument (1 for a
// non-static member function, whose leading parameter is the implicit object).
// The checks mirror FunctionCallScore so the explanation matches the rejection.
// Returns true if a concrete reason was produced.
static bool DescribeFunctionNonViability(TypeRecord* func, VectorASTNode* node,
                                         size_t first_formal_arg,
                                         String* reason) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.unknown_args) {
    return false;
  }
  size_t num_actual = node->children->length;
  size_t num_formal = func->info.function.prototype.length;
  if (first_formal_arg > num_formal) {
    return false;
  }
  size_t num_user_formal = num_formal - first_formal_arg;
  size_t required = FunctionRequiredArgumentCount(func, first_formal_arg);

  if (num_actual < required) {
    if (required == num_user_formal) {
      StringPrintf(reason, "requires %zu argument%s, but %zu %s provided",
                   num_user_formal, num_user_formal == 1 ? "" : "s", num_actual,
                   num_actual == 1 ? "was" : "were");
    } else {
      StringPrintf(reason,
                   "requires at least %zu argument%s, but %zu %s provided",
                   required, required == 1 ? "" : "s", num_actual,
                   num_actual == 1 ? "was" : "were");
    }
    return true;
  }
  if (!func->info.function.varargs && num_actual > num_user_formal) {
    StringPrintf(reason, "requires %zu argument%s, but %zu %s provided",
                 num_user_formal, num_user_formal == 1 ? "" : "s", num_actual,
                 num_actual == 1 ? "was" : "were");
    return true;
  }

  size_t num_checked =
      num_actual < num_user_formal ? num_actual : num_user_formal;
  for (size_t i = 0; i < num_checked; i++) {
    ASTNode* actual = (ASTNode*)node->children->value.p[i];
    Symbol* formal =
        (Symbol*)func->info.function.prototype.value.p[i + first_formal_arg];
    if (OverloadConversionRank(actual, formal->type) >= 0) {
      continue;
    }
    String from;
    StringInit(&from, NULL);
    String to;
    StringInit(&to, NULL);
    AppendReadableType(actual != NULL ? actual->type : NULL, &from);
    AppendReadableType(formal->type, &to);
    StringPrintf(reason, "no known conversion from '%s' to '%s' for argument %zu",
                 from.value, to.value, i + 1);
    StringDestruct(&from);
    StringDestruct(&to);
    return true;
  }
  return false;
}

// For a compiler-synthesized special member (one the user never wrote), returns
// a short description such as "implicit copy constructor".  Returns NULL for an
// ordinary user-declared function so the caller falls back to the signature.
static const char* ImplicitSpecialMemberDescription(Symbol* candidate) {
  if (candidate == NULL || candidate->type == NULL ||
      !TypeIsFunction(candidate->type) ||
      !candidate->type->info.function.is_implicitly_declared) {
    return NULL;
  }
  if (candidate->type->info.function.is_constructor) {
    Vector* proto = &candidate->type->info.function.prototype;
    // prototype[0] is the implicit object parameter; the source operand of a
    // copy/move constructor, if any, is prototype[1].
    if (proto->length >= 2) {
      Symbol* source = proto->value.p[1];
      if (source != NULL && source->type != NULL &&
          TypeIsReference(source->type)) {
        return source->type->declarator == kDeclRValueReference
                   ? "implicit move constructor"
                   : "implicit copy constructor";
      }
    }
    return "implicit default constructor";
  }
  if (candidate->type->info.function.is_destructor) {
    return "implicit destructor";
  }
  return "implicitly-declared special member";
}

// Emits a "candidate ... not viable" note pointing at `candidate`'s declaration.
// `reason` (may be empty) explains why it was rejected; an empty reason produces
// a bare "candidate" note used for the equally-ranked candidates of an
// ambiguous call.  Compiler-synthesized members have no meaningful source
// location, so they are described in prose without one.
static void EmitCandidateNote(Symbol* candidate, const char* reason) {
  if (candidate == NULL) {
    return;
  }
  bool has_reason = reason != NULL && reason[0] != '\0';
  const char* implicit_desc = ImplicitSpecialMemberDescription(candidate);
  if (implicit_desc != NULL) {
    if (has_reason) {
      ReportNote(NULL, 0, "candidate (%s) not viable: %s", implicit_desc,
                 reason);
    } else {
      ReportNote(NULL, 0, "candidate (%s)", implicit_desc);
    }
    return;
  }
  String signature;
  StringInit(&signature, NULL);
  SymbolFunctionPrettyName(candidate, &signature);
  if (has_reason) {
    SemanticNoteAtLocation(candidate->location, "candidate '%s' not viable: %s",
                           signature.value, reason);
  } else {
    SemanticNoteAtLocation(candidate->location, "candidate '%s'",
                           signature.value);
  }
  StringDestruct(&signature);
}

// Emits one note per overload in `candidates` after a failed free-function /
// operator resolution.  When `ambiguous` is false (no viable candidate) each
// note explains the rejection; when true, the equally-best candidates (those
// whose score ties `best_score`) are listed as the competing matches.
static void EmitFreeCandidateNotes(Vector* candidates, VectorASTNode* call,
                                   bool ambiguous, int best_score) {
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    if (candidate == NULL) {
      continue;
    }
    bool is_template = SymbolIsTemplateFunction(candidate);
    if (ambiguous) {
      // The competing matches are the candidates whose conversion score ties the
      // winner.  Template candidates were resolved to specializations that are
      // not represented in this vector, so they are left out rather than
      // mislabeled.
      if (!is_template && OverloadCallScore(candidate, call) == best_score) {
        EmitCandidateNote(candidate, NULL);
      }
      continue;
    }
    if (is_template) {
      EmitFunctionTemplateCandidateRejectionNote(candidate, call, NULL, 0);
      continue;
    }
    String reason;
    StringInit(&reason, NULL);
    DescribeFunctionNonViability(candidate->type, call, 0, &reason);
    EmitCandidateNote(candidate, reason.value);
    StringDestruct(&reason);
  }
}

/* Contract: SymbolDelete every temporary in `candidates` except `keep`; destroys vector. */
static void DeleteTemporaryFunctionTemplateCandidates(Vector* candidates,
                                                      Symbol* keep) {
  if (candidates == NULL) {
    return;
  }
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    if (candidate != NULL && candidate != keep) {
      SymbolDelete(candidate);
    }
  }
  VectorDestruct(candidates);
}

// Two instantiations of the same class-template friend can both be exact
// matches when they are the same function type.  That is one candidate.
static bool SameFunctionSignature(Symbol* left, Symbol* right) {
  return left != NULL && right != NULL && left->type != NULL &&
         right->type != NULL && TypeIsFunction(left->type) &&
         TypeIsFunction(right->type) && TypeEqual(left->type, right->type);
}

// Lookup can reach one free function more than once (ordinary lookup and
// ADL, a using-declaration, a friend in several instantiations).  Functions of
// the same type declared in different namespaces are still distinct
// candidates, and a call that ties them is ambiguous.
static bool SameFreeFunction(Symbol* left, Symbol* right) {
  if (!SameFunctionSignature(left, right)) {
    return false;
  }
  if (SymbolHasAsmName(left) && SymbolHasAsmName(right)) {
    return strcmp(SymbolAsmName(left), SymbolAsmName(right)) == 0;
  }
  return left->namespace_ == right->namespace_;
}

/* Contract: true if any element of `actuals` has auto or unknown type.
 * Overload resolution must defer diagnostics until those types are known. */
static bool CallActualsContainUndeducedAuto(Vector* actuals) {
  if (actuals == NULL) {
    return false;
  }
  for (size_t i = 0; i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual != NULL && actual->type != NULL &&
        (TypeContainsAuto(actual->type) || TypeIsUnknown(actual->type))) {
      return true;
    }
  }
  return false;
}

/* Contract: upper bound on a non-template score when every argument is an
 * exact match ([over.match.best] non-template preference threshold). */
static int FreeExactMatchScoreCeiling(Vector* actuals) {
  size_t scored_actuals = actuals != NULL ? actuals->length : 0;
  return scored_actuals == 0 ? 0 : (int)(5 * scored_actuals);
}

/* A prvalue binding to `const T&` is viable but worse than binding to `T&&`.
 * Do not let the exact-match shortcut hide a forwarding-reference template
 * that can therefore win ([over.ics.rank]). */
static bool FunctionHasRvalueToConstLvalueReferenceBinding(
    TypeRecord* func, VectorASTNode* call, size_t first_formal_arg) {
  if (func == NULL || !TypeIsFunction(func) || call == NULL ||
      call->children == NULL) {
    return false;
  }
  Vector* prototype = &func->info.function.prototype;
  for (size_t i = 0; i < call->children->length &&
                     i + first_formal_arg < prototype->length;
       i++) {
    ASTNode* actual = call->children->value.p[i];
    Symbol* formal = prototype->value.p[i + first_formal_arg];
    if (actual != NULL && formal != NULL && formal->type != NULL &&
        TypeIsReference(formal->type) &&
        formal->type->declarator != kDeclRValueReference &&
        formal->type->next != NULL && TypeIsConst(formal->type->next) &&
        !ASTNodeIsLValue(actual)) {
      return true;
    }
  }
  return false;
}

/* Compare equally ranked pointer conversions whose destination classes lie on
 * the same inheritance path.  Converting Generated* to Message* is better than
 * converting it to MessageLite* when Message derives from MessageLite.  The
 * same holds for a class argument converted or bound to a base class
 * (`align(minute_tag)` over `align(second_tag)` for a `minute_tag` subclass),
 * and converting a class pointer to a base pointer is better than converting
 * it to `void*` ([over.ics.rank]/4.4). */
static int CompareDerivedPointerTargets(ASTNode* actual, TypeRecord* left,
                                        TypeRecord* right) {
  if (TypeIsReference(left)) {
    left = left->next;
  }
  if (TypeIsReference(right)) {
    right = right->next;
  }
  if (actual == NULL || actual->type == NULL || left == NULL ||
      right == NULL) {
    return 0;
  }
  TypeRecord* from = actual->type;
  if (TypeIsPointer(from) && TypeIsPointer(left) && TypeIsPointer(right)) {
    from = from->next;
    left = left->next;
    right = right->next;
    if (from != NULL && left != NULL && right != NULL &&
        TypeIsStructOrUnion(from) && TypeIsVoid(left) != TypeIsVoid(right)) {
      TypeRecord* base = TypeIsVoid(left) ? right : left;
      bool reaches_base = TypeIsStructOrUnion(base) &&
                          (TypeEqualIgnoringQualifiers(from, base) ||
                           TypeIsDerivedFromInCurrentClass(from, base));
      if (!reaches_base) {
        return 0;
      }
      return TypeIsVoid(left) ? -1 : 1;
    }
  } else if (TypeIsPointer(from) || TypeIsPointer(left) ||
             TypeIsPointer(right)) {
    return 0;
  }
  if (from == NULL || left == NULL || right == NULL ||
      !TypeIsStructOrUnion(from) || !TypeIsStructOrUnion(left) ||
      !TypeIsStructOrUnion(right)) {
    return 0;
  }
  bool actual_reaches_left = TypeEqualIgnoringQualifiers(from, left) ||
                             TypeIsDerivedFromInCurrentClass(from, left);
  bool actual_reaches_right = TypeEqualIgnoringQualifiers(from, right) ||
                              TypeIsDerivedFromInCurrentClass(from, right);
  if (!actual_reaches_left || !actual_reaches_right) {
    return 0;
  }
  if (TypeIsDerivedFrom(left, right)) {
    return 1;
  }
  if (TypeIsDerivedFrom(right, left)) {
    return -1;
  }
  return 0;
}

/* Apply the per-argument "no worse, and better for at least one" rule when two
 * non-template candidates have the same aggregate score.  The numeric rank
 * intentionally groups all derived-to-base pointer conversions together, so
 * CompareDerivedPointerTargets supplies the required same-path tie-break. */
static int CompareEqualScoreConversionSequences(Symbol* left, Symbol* right,
                                                VectorASTNode* call) {
  if (left == NULL || right == NULL || left->type == NULL ||
      right->type == NULL || !TypeIsFunction(left->type) ||
      !TypeIsFunction(right->type) || call == NULL ||
      call->children == NULL) {
    return 0;
  }
  Vector* left_params = &left->type->info.function.prototype;
  Vector* right_params = &right->type->info.function.prototype;
  if (call->children->length > left_params->length ||
      call->children->length > right_params->length) {
    return 0;
  }
  int preference = 0;
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    Symbol* left_param = left_params->value.p[i];
    Symbol* right_param = right_params->value.p[i];
    if (actual == NULL || left_param == NULL || right_param == NULL ||
        left_param->type == NULL || right_param->type == NULL) {
      return 0;
    }
    int left_rank = OverloadConversionRank(actual, left_param->type);
    int right_rank = OverloadConversionRank(actual, right_param->type);
    if (left_rank < 0 || right_rank < 0) {
      return 0;
    }
    int argument_preference =
        left_rank < right_rank
            ? 1
            : (right_rank < left_rank
                   ? -1
                   : CompareDerivedPointerTargets(actual, left_param->type,
                                                  right_param->type));
    if (argument_preference != 0 && preference != 0 &&
        argument_preference != preference) {
      return 0;
    }
    if (argument_preference != 0) {
      preference = argument_preference;
    }
  }
  return preference;
}

/* Contract: scores non-template, non-instantiated overloads in `candidates`;
 * updates `*best`, `*best_score`, and `*ambiguous`.  Skips template symbols. */
static void RankNonTemplateFreeFunctionCandidates(Vector* candidates,
                                                  VectorASTNode* call,
                                                  Symbol** best, int* best_score,
                                                  bool* ambiguous) {
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    if (candidate == NULL || candidate->flags.is_template ||
        (candidate->type != NULL && TypeIsFunction(candidate->type) &&
         candidate->type->info.function.template_origin != NULL)) {
      continue;
    }
    int score = OverloadCallScore(candidate, call);
    if (score < 0) {
      continue;
    }
    if (*best == NULL || score < *best_score) {
      *best = candidate;
      *best_score = score;
      *ambiguous = false;
    } else if (score == *best_score && !SameFreeFunction(*best, candidate)) {
      *ambiguous = true;
    }
  }
  if (*best == NULL || !*ambiguous) {
    return;
  }
  // A pairwise replacement can lose an earlier tied candidate.  Search the
  // complete minimum-score set for one candidate that is better than every
  // distinct peer; if none exists, the call remains ambiguous.
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    if (candidate == NULL || candidate->flags.is_template ||
        (candidate->type != NULL && TypeIsFunction(candidate->type) &&
         candidate->type->info.function.template_origin != NULL) ||
        OverloadCallScore(candidate, call) != *best_score) {
      continue;
    }
    bool dominates = true;
    for (size_t j = 0; j < candidates->length; j++) {
      Symbol* peer = candidates->value.p[j];
      if (peer == NULL || peer == candidate || peer->flags.is_template ||
          (peer->type != NULL && TypeIsFunction(peer->type) &&
           peer->type->info.function.template_origin != NULL) ||
          OverloadCallScore(peer, call) != *best_score ||
          SameFreeFunction(candidate, peer)) {
        continue;
      }
      if (CompareEqualScoreConversionSequences(candidate, peer, call) <= 0) {
        dominates = false;
        break;
      }
    }
    if (dominates) {
      *best = candidate;
      *ambiguous = false;
      return;
    }
  }
}

/* Contract: when `effective` ties the current `*best` conversion score, applies
 * [over.match.best] / [temp.func.order] template vs non-template and constraint
 * ordering.  May replace `*best` and clears or sets `*ambiguous`. */
static void ApplyFreeFunctionEqualScoreTie(Symbol* effective, Symbol** best,
                                           bool* ambiguous) {
  bool best_is_template =
      (*best)->type != NULL && TypeIsFunction((*best)->type) &&
      (*best)->type->info.function.template_origin != NULL;
  bool effective_is_template =
      effective->type != NULL && TypeIsFunction(effective->type) &&
      effective->type->info.function.template_origin != NULL;
  if (best_is_template && !effective_is_template) {
    *best = effective;
    *ambiguous = false;
  } else if (best_is_template && effective_is_template) {
    int constraint_tie_break = ConstraintTieBreak(*best, effective);
    if (constraint_tie_break > 0) {
      *best = effective;
      *ambiguous = false;
    } else if (constraint_tie_break < 0) {
      *ambiguous = false;
    } else if (*best != effective) {
      int template_order =
          CompareFunctionTemplateSpecificity(effective, *best);
      if (template_order > 0) {
        *best = effective;
        *ambiguous = false;
      } else if (template_order < 0) {
        *ambiguous = false;
      } else {
        *ambiguous = true;
      }
    }
  } else if (*best != effective && best_is_template == effective_is_template) {
    int template_order = CompareFunctionTemplateSpecificity(effective, *best);
    if (template_order > 0) {
      *best = effective;
      *ambiguous = false;
    } else if (template_order < 0) {
      *ambiguous = false;
    } else {
      *ambiguous = true;
    }
  }
}

/* Compares template candidates per argument instead of by the aggregate numeric
 * score. A by-value exact match and a direct reference binding are both exact
 * conversion sequences; the score's internal +5 value marker must not prevent
 * template partial ordering from selecting a fixed overload over `Args&&...`. */
static bool FunctionTemplateConversionSequencesTie(
    Symbol* left, Symbol* right, VectorASTNode* call) {
  if (!SymbolIsTemplateFunction(left) || !SymbolIsTemplateFunction(right) ||
      left->type == NULL || right->type == NULL ||
      !TypeIsFunction(left->type) || !TypeIsFunction(right->type) ||
      call == NULL || call->children == NULL) {
    return false;
  }
  Vector* left_params = &left->type->info.function.prototype;
  Vector* right_params = &right->type->info.function.prototype;
  if (call->children->length > left_params->length ||
      call->children->length > right_params->length) {
    return false;
  }
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    Symbol* left_param = left_params->value.p[i];
    Symbol* right_param = right_params->value.p[i];
    if (actual == NULL || left_param == NULL || right_param == NULL ||
        left_param->type == NULL || right_param->type == NULL) {
      return false;
    }
    int left_rank = OverloadConversionRank(actual, left_param->type);
    int right_rank = OverloadConversionRank(actual, right_param->type);
    if (left_rank < 0 || right_rank < 0) {
      return false;
    }
    bool left_reference = TypeIsReference(left_param->type);
    bool right_reference = TypeIsReference(right_param->type);
    if (left_reference != right_reference) {
      if (left_rank / 10 != right_rank / 10) {
        return false;
      }
    } else if (left_rank != right_rank) {
      return false;
    }
  }
  return true;
}

/* Contract: instantiates viable function templates from `candidates`, scores
 * them, and merges into `*best`.  Appends temporaries to `temporary_candidates`.
 * Runs only when non-template ranking did not already yield an exact match. */
static void RankFunctionTemplateCandidates(Vector* candidates,
                                           VectorASTNode* call,
                                           Vector* explicit_args,
                                           Vector* temporary_candidates,
                                           Vector* actuals, Symbol** best,
                                           int* best_score, bool* ambiguous) {
  if (*best_score >= 0 && *best_score <= FreeExactMatchScoreCeiling(actuals) &&
      *best != NULL &&
      !FunctionHasRvalueToConstLvalueReferenceBinding((*best)->type, call, 0)) {
    return;
  }
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    // Non-template candidates were ranked in the first pass.  Reprocessing
    // them here would feed equal-score ordinary overloads into template
    // partial ordering and incorrectly mark the selected overload ambiguous.
    if (candidate == NULL || !candidate->flags.is_template) {
      continue;
    }
    Symbol* effective =
        FunctionTemplateOverloadCandidate(candidate, call, explicit_args,
                                          temporary_candidates);
    if (effective == NULL) {
      continue;
    }
    int score = OverloadCallScore(effective, call);
    if (score < 0) {
      continue;
    }
    if (*best != NULL &&
        FunctionTemplateConversionSequencesTie(effective, *best, call)) {
      Symbol* previous_best = *best;
      ApplyFreeFunctionEqualScoreTie(effective, best, ambiguous);
      if (*best != previous_best) {
        *best_score = score;
      }
      continue;
    }
    if (*best == NULL || score < *best_score) {
      *best = effective;
      *best_score = score;
      *ambiguous = false;
    } else if (score == *best_score) {
      ApplyFreeFunctionEqualScoreTie(effective, best, ambiguous);
    }
  }
}

/* Contract: on failed resolution, deletes `temporary_candidates` and returns
 * NULL.  Emits diagnostics only when `diagnose_no_match` and not yet diagnosed.
 * Defers when actuals still contain undeduced auto. */
static Symbol* DiagnoseFailedFreeFunctionResolution(
    String* name, Vector* candidates, Vector* actuals, Vector* explicit_args,
    bool diagnose_no_match, ASTNode* diagnostic_node,
    Vector* temporary_candidates) {
  bool already_diagnosed =
      diagnostic_node != NULL &&
      (diagnostic_node->flags & kASTOverloadDiagnosed) != 0;
  if (CallActualsContainUndeducedAuto(actuals)) {
    DeleteTemporaryFunctionTemplateCandidates(temporary_candidates, NULL);
    return NULL;
  }
  if (!diagnose_no_match || already_diagnosed) {
    DeleteTemporaryFunctionTemplateCandidates(temporary_candidates, NULL);
    return NULL;
  }
  Symbol* constraint_rejected = NULL;
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    if (candidate == NULL || !SymbolIsTemplateFunction(candidate)) {
      continue;
    }
    if (TypeClassifyFunctionTemplateCandidate(&compiler->syntax, candidate,
                                              explicit_args, actuals, 0) ==
        kFunctionTemplateCandidateConstraintsNotSatisfied) {
      constraint_rejected = candidate;
      break;
    }
  }
  if (constraint_rejected != NULL &&
      ConceptsFunctionTemplateHasAssociatedConstraint(constraint_rejected)) {
    VectorASTNode call_node = {0};
    call_node.children = actuals;
    ReportUnsatisfiedFunctionTemplateConstraints(
        &call_node, constraint_rejected, explicit_args, 0);
    diagnostic_node->flags |= kASTOverloadDiagnosed;
    DeleteTemporaryFunctionTemplateCandidates(temporary_candidates, NULL);
    return NULL;
  }
  String function_name;
  StringInit(&function_name, NULL);
  Symbol* first_candidate =
      candidates->length > 0 ? candidates->value.p[0] : NULL;
  if (first_candidate != NULL) {
    SymbolFunctionDiagnosticName(first_candidate, &function_name);
  } else {
    StringAppendString(&function_name, name);
  }
  SemanticError(diagnostic_node, "No matching overload for %s",
                function_name.value);
  StringDestruct(&function_name);
  VectorASTNode call = {0};
  call.children = actuals;
  EmitFreeCandidateNotes(candidates, &call, /*ambiguous=*/false, -1);
  diagnostic_node->flags |= kASTOverloadDiagnosed;
  DeleteTemporaryFunctionTemplateCandidates(temporary_candidates, NULL);
  return NULL;
}

/* Contract: picks the best viable overload from `candidates` for `actuals`,
 * optionally diagnosing failure or ambiguity at `diagnostic_node`.  Owns and
 * deletes template instantiations in `temporary_candidates` except the winner. */
static Symbol* ResolveFunctionCandidateVector(String* name, Vector* candidates,
                                              Vector* actuals,
                                              Vector* explicit_args,
                                              bool diagnose_no_match,
                                              bool diagnose_ambiguous,
                                              ASTNode* diagnostic_node) {
  VectorASTNode call = {0};
  call.children = actuals;
  Symbol* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  Vector temporary_candidates;
  VectorInit(&temporary_candidates);
  if (explicit_args == NULL) {
    RankNonTemplateFreeFunctionCandidates(candidates, &call, &best, &best_score,
                                          &ambiguous);
  }

  // An exact match scores 5 per argument (`base_rank * 10 + 5`).  A
  // non-template that matches every argument exactly is better than any
  // function template ([over.match.best]); do not let a reference-binding
  // template, which this ranker scores below 5, replace it.  `pred(const
  // char*, const char*)` must beat `pred(const StringType&, ...)`.
  RankFunctionTemplateCandidates(candidates, &call, explicit_args,
                                 &temporary_candidates, actuals, &best,
                                 &best_score, &ambiguous);

  bool already_diagnosed =
      diagnostic_node != NULL &&
      (diagnostic_node->flags & kASTOverloadDiagnosed) != 0;
  if (best == NULL) {
    return DiagnoseFailedFreeFunctionResolution(
        name, candidates, actuals, explicit_args, diagnose_no_match,
        diagnostic_node, &temporary_candidates);
  }
  if (ambiguous) {
    if (diagnose_ambiguous && !already_diagnosed) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticName(best, &function_name);
      SemanticError(diagnostic_node, "Ambiguous overload for %s",
                    function_name.value);
      StringDestruct(&function_name);
      VectorASTNode call = {0};
      call.children = actuals;
      EmitFreeCandidateNotes(candidates, &call, /*ambiguous=*/true, best_score);
      diagnostic_node->flags |= kASTOverloadDiagnosed;
    }
    DeleteTemporaryFunctionTemplateCandidates(&temporary_candidates, best);
    return best;
  }
  Symbol* resolved = InstantiateSelectedFunctionTemplateCandidate(best);
  DeleteTemporaryFunctionTemplateCandidates(
      &temporary_candidates, resolved == best ? best : NULL);
  return resolved;
}

// Unqualified lookup for a free operator stops at the first enclosing
// namespace that declares the name ([basic.lookup.unqual]).  Global scope is
// collected separately by the caller.  Without this walk, an operator declared
// in the function's own namespace is invisible unless ADL happens to associate
// that namespace.
static void AddOrdinaryUnqualifiedFunctionCandidates(String* name,
                                                     Namespace* start,
                                                     Vector* candidates) {
  for (int depth = 0; start != NULL && depth < 64; depth++, start = start->parent) {
    if (start == compiler->global_namespace) {
      return;
    }
    Vector functions;
    VectorInit(&functions);
    NamespaceCollectFunctionSymbolsInInlineSet(start, name, &functions);
    bool found = functions.length > 0;
    VectorDestruct(&functions);
    if (found) {
      AddNamedFunctionCandidates(name, start, candidates);
      return;
    }
  }
}

/* Contract: collects global + unqualified + ADL overloads for `name`; best match. */
Symbol* ResolveFreeFunctionWithADL(String* name, Vector* actuals,
                                          bool diagnose_ambiguous) {
  Vector candidates;
  VectorInit(&candidates);
  AddNamedFunctionCandidates(name, compiler->global_namespace, &candidates);
  if (CompilerIsCXX()) {
    Namespace* from_function = NULL;
    if (compiler->current_function != NULL &&
        compiler->current_function->info.function.symbol != NULL) {
      from_function =
          compiler->current_function->info.function.symbol->namespace_;
    }
    AddOrdinaryUnqualifiedFunctionCandidates(name, from_function, &candidates);
    AddOrdinaryUnqualifiedFunctionCandidates(
        name, compiler->syntax.current_namespace, &candidates);
  }
  AddADLFunctionCandidates(name, actuals, &candidates);
  ASTNode* diagnostic_node =
      actuals != NULL && actuals->length > 0 ? actuals->value.p[0] : NULL;
  Symbol* best = ResolveFunctionCandidateVector(
      name, &candidates, actuals, NULL,
      /*diagnose_no_match=*/false, diagnose_ambiguous, diagnostic_node);
  VectorDestruct(&candidates);
  // ResolveFunctionCandidateVector already turns the selected temporary
  // template candidate into its concrete specialization.  Instantiating that
  // specialization a second time reuses its argument vector against the
  // primary template and can permute dependent operands in operator templates.
  return best;
}

/* Public entry used when instantiating a cloned template body: a call whose
 * callee names an *overloaded* function template must undergo full overload
 * resolution once the arguments are concrete, not be bound to whichever single
 * overload the identifier happened to reference during the first (dependent)
 * pass.  Gathers the whole overload set reachable from `callee` (plus ADL over
 * the now-concrete actuals), resolves it, and returns the best concrete
 * instantiation, or NULL when none is viable. */
Symbol* CXXResolveOverloadedFunctionTemplateCall(Symbol* callee,
                                                 Vector* explicit_args,
                                                 Vector* actuals) {
  if (!CompilerIsCXX() || callee == NULL || actuals == NULL) {
    return NULL;
  }
  Vector candidates;
  VectorInit(&candidates);
  AddFunctionOverloadCandidates(&candidates, callee);
  AddADLFunctionCandidates(&callee->name, actuals, &candidates);
  ASTNode* diagnostic_node =
      actuals->length > 0 ? actuals->value.p[0] : NULL;
  Symbol* best = ResolveFunctionCandidateVector(
      &callee->name, &candidates, actuals, explicit_args,
      /*diagnose_no_match=*/false, /*diagnose_ambiguous=*/false,
      diagnostic_node);
  VectorDestruct(&candidates);
  return best;
}

// A braced-init-list is a non-deduced context, but the parameter it initializes
// may have its template arguments deduced from another argument
// (`SetFlag(Flag<T>*, const T&)` called as `SetFlag(&f, {})`).
static bool FunctionTemplateDeducesWithoutBracedArguments(
    Symbol* templ, Vector* actuals, size_t first_formal_arg) {
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  bool viable = TypeClassifyFunctionTemplateCandidate(
                    &compiler->syntax, templ, NULL, actuals,
                    first_formal_arg) == kFunctionTemplateCandidateViable &&
                !DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  return viable;
}

// True when `templ` cannot accept this many explicit template arguments, even
// though some of them are still dependent.  `Ov<U, char, short>()` must not
// keep the unary `template <class U> Ov()` as a viable primary.  An explicit
// pack expansion may stand for any number of arguments.
static bool ExplicitArgumentsExceedTemplateParameters(Symbol* templ,
                                                      Vector* explicit_args) {
  const Vector* params =
      &FunctionExtraOf(&templ->type->info.function)->template_parameters;
  if (params->length == 0) {
    return false;
  }
  for (size_t i = 0; i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      return false;
    }
  }
  for (size_t i = 0; i < explicit_args->length; i++) {
    TemplateArgument* arg = explicit_args->value.p[i];
    if (arg != NULL && (arg->is_pack_expansion ||
                        arg->references_parameter_pack ||
                        arg->pack_arguments != NULL)) {
      return false;
    }
  }
  return explicit_args->length > params->length;
}

/* Contract: returns `candidate`, a dependent primary, or a deduced specialization;
 * NULL when substitution/arity pre-check fails.  Registers temps when provided. */
static Symbol* FunctionTemplateOverloadCandidate(Symbol* candidate,
                                                 VectorASTNode* node,
                                                 Vector* explicit_args,
                                                 Vector* temporary_candidates) {
  if (candidate == NULL || candidate->type == NULL ||
      !TypeIsFunction(candidate->type)) {
    return candidate;
  }
  if (candidate->type->info.function.template_origin != NULL) {
    return NULL;
  }
  if (!candidate->flags.is_template) {
    return explicit_args == NULL ? candidate : NULL;
  }
  // `f<T>()` inside a class template, where T is still a template parameter
  // and does not appear in f's parameter list (`IsNoThrowSwappable<hasher>()`).
  // The specialization cannot be built yet.  Keep the primary so a dependent
  // noexcept specifier is not rejected as a deduction failure.
  if (explicit_args != NULL) {
    bool dependent_explicit = false;
    for (size_t i = 0; i < explicit_args->length; i++) {
      TemplateArgument* arg = explicit_args->value.p[i];
      if (arg != NULL && arg->type != NULL &&
          TypeContainsTemplateParameter(arg->type)) {
        dependent_explicit = true;
        break;
      }
      if (arg != NULL && arg->dependent_expr != NULL) {
        dependent_explicit = true;
        break;
      }
    }
    if (dependent_explicit) {
      return ExplicitArgumentsExceedTemplateParameters(candidate, explicit_args)
                 ? NULL
                 : candidate;
    }
  }
  // A braced-init-list has no type and therefore cannot deduce an ordinary
  // template parameter such as `T&&`. Reject this before the enclosing-class
  // deferral below can preserve the unresolved primary template. Only
  // initializer_list parameters (and array-reference parameters) are deduced
  // element-wise from braces.
  Vector* prototype = &candidate->type->info.function.prototype;
  bool braced_parameters_deduced_elsewhere = false;
  bool braced_parameters_checked = false;
  for (size_t i = 0; explicit_args == NULL && node->children != NULL &&
                     i < node->children->length && i < prototype->length;
       i++) {
    Symbol* formal = prototype->value.p[i];
    ASTNode* actual = node->children->value.p[i];
    TypeRecord* braced_formal_target =
        formal != NULL && TypeIsReference(formal->type)
            ? formal->type->next
            : NULL;
    if (formal != NULL && actual != NULL &&
        actual->op == AST_OP(braced_init) &&
        TypeContainsTemplateParameter(formal->type) &&
        !TypeIsCXXInitializerList(formal->type) &&
        (braced_formal_target == NULL ||
         !TypeIsArray(braced_formal_target))) {
      if (!braced_parameters_checked) {
        braced_parameters_checked = true;
        braced_parameters_deduced_elsewhere =
            FunctionTemplateDeducesWithoutBracedArguments(candidate,
                                                          node->children, 0);
      }
      if (!braced_parameters_deduced_elsewhere) {
        return NULL;
      }
    }
  }
  // A member function template whose signature still names a member of the
  // enclosing class template (`slot_type` = `typename Policy::slot_type`)
  // cannot be instantiated until that class is.  Keep the primary; the call is
  // resolved again when the enclosing class is instantiated.
  {
    int base = FunctionExtraOf(&candidate->type->info.function)
                   ->template_parameter_base;
    Vector* formals = &candidate->type->info.function.prototype;
    for (size_t i = 0; i < formals->length; i++) {
      Symbol* formal = formals->value.p[i];
      if (formal != NULL &&
          TypeDependsOnEnclosingTemplateParameter(formal->type, base)) {
        return candidate;
      }
    }
  }
  for (size_t i = 0; node->children != NULL && i < node->children->length &&
                     i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    ASTNode* actual = node->children->value.p[i];
    if (formal != NULL && actual != NULL &&
        TypeIsCXXInitializerList(formal->type) &&
        actual->op != AST_OP(braced_init)) {
      return NULL;
    }
    if (formal != NULL && actual != NULL &&
        !TypeContainsTemplateParameter(formal->type) &&
        OverloadConversionRank(actual, formal->type) < 0) {
      return NULL;
    }
  }
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  Symbol* instantiated = TypeCreateFunctionTemplateCandidate(
      &compiler->syntax, candidate, explicit_args, node->children, 0);
  bool substitution_failed = DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  if (substitution_failed && instantiated != NULL) {
    SymbolDelete(instantiated);
    instantiated = NULL;
  }
  if (instantiated != NULL && temporary_candidates != NULL) {
    VectorAppend(temporary_candidates, instantiated);
  }
  return instantiated;
}

/* Contract: materializes the body of a scored template specialization; else `selected`. */
static Symbol* InstantiateSelectedFunctionTemplateCandidate(Symbol* selected) {
  if (selected == NULL || selected->type == NULL ||
      !TypeIsFunction(selected->type) ||
      selected->type->info.function.template_origin == NULL ||
      selected->type->template_arguments == NULL) {
    return selected;
  }
  Symbol* instantiated =
      TypeInstantiateFunctionTemplateWithCompletedArguments(
          &compiler->syntax, selected->type->info.function.template_origin,
          selected->type->template_arguments);
  CXXAnalyzeImmediateEscalationCandidate(instantiated);
  return instantiated != NULL ? instantiated : selected;
}

// Overloads of a class member are chained on its StructMember, not on the
// symbol: `&S::f<B>` must also consider the `f` declared after the one the
// name found.
static void CollectFunctionOverloads(Symbol* head, Vector* out) {
  Struct* owner = head->overload_next == NULL && head->type != NULL &&
                          TypeIsFunction(head->type)
                      ? head->type->info.function.cxx_member_owner
                      : NULL;
  StructMember* member =
      owner != NULL ? FindStructMember(owner, &head->name) : NULL;
  if (member != NULL && member->overload_next != NULL) {
    for (; member != NULL; member = member->overload_next) {
      if (member->symbol != NULL) {
        VectorAppend(out, member->symbol);
      }
    }
    return;
  }
  for (Symbol* cand = head; cand != NULL; cand = cand->overload_next) {
    VectorAppend(out, cand);
  }
}

// Substitutes speculatively, so a failed trailing `decltype` is not diagnosed.
static TypeRecord* SubstituteFunctionTemplateSignatureQuietly(
    Symbol* templ, Vector* completed_args) {
  bool saved_trap = DiagnosticErrorTrapBegin();
  DiagnosticSuppressBegin();
  TypeRecord* signature = TypeSubstituteFunctionTemplateSignature(
      &compiler->syntax, templ, completed_args);
  bool substitution_failed = DiagnosticErrorTrapped();
  DiagnosticSuppressEnd();
  DiagnosticErrorTrapEnd(saved_trap);
  if (substitution_failed) {
    TypeRecordDelete(signature);
    return NULL;
  }
  return signature;
}

// [over.over] / [temp.deduct.funcaddr]: given a (possibly overloaded) function
// name -- the head |head| of an overload chain, optionally with explicitly
// written template arguments |explicit_args| -- used where a specific function
// type |target_fn| is required (the pointee of a destination function pointer),
// return the unique matching concrete function: a non-template overload whose
// type matches |target_fn| exactly, or a function-template specialization
// deduced/instantiated from |target_fn|.  Non-template matches are preferred
// over template specializations.  Returns NULL when there is no unique match.
Symbol* CXXResolveFunctionAddressForTargetType(Symbol* head,
                                               Vector* explicit_args,
                                               TypeRecord* target_fn) {
  if (!CompilerIsCXX() || head == NULL || target_fn == NULL ||
      !TypeIsFunction(target_fn)) {
    return NULL;
  }
  Symbol* non_template_match = NULL;
  bool non_template_ambiguous = false;
  Symbol* template_match = NULL;
  bool template_ambiguous = false;
  Vector candidates;
  VectorInit(&candidates);
  CollectFunctionOverloads(head, &candidates);
  for (size_t ci = 0; ci < candidates.length; ci++) {
    Symbol* cand = candidates.value.p[ci];
    if (cand->type == NULL || !TypeIsFunction(cand->type)) {
      continue;
    }
    if (SymbolIsTemplateFunction(cand)) {
      Symbol* inst = NULL;
      if (!cand->flags.is_template &&
          cand->type->info.function.template_origin != NULL &&
          cand->type->template_arguments != NULL) {
        // Overload resolution may already have replaced the identifier's
        // primary template with a non-emitting specialization candidate.  Its
        // signature and arguments are concrete, but the body has not been
        // instantiated yet.  Re-deducing from this candidate cannot work (it
        // is no longer marked as a primary template), so materialize the
        // selected specialization directly.
        if (!(TypeEqual(cand->type, target_fn) ||
              TypeEqualIgnoringQualifiers(cand->type, target_fn))) {
          continue;
        }
        inst = InstantiateSelectedFunctionTemplateCandidate(cand);
      } else {
        Vector* args = TypeDeduceFunctionTemplateArgumentsFromFunctionType(
            &compiler->syntax, cand, explicit_args, target_fn);
        if (args == NULL) {
          continue;
        }
        // The return type is not deduced from the target; substituting it
        // discards the overload whose trailing `decltype` fails, before an
        // instantiation could diagnose it.
        TypeRecord* signature =
            SubstituteFunctionTemplateSignatureQuietly(cand, args);
        bool signature_matches =
            signature != NULL &&
            (TypeEqual(signature, target_fn) ||
             TypeEqualIgnoringQualifiers(signature, target_fn));
        TypeRecordDelete(signature);
        if (signature_matches) {
          inst = TypeInstantiateFunctionTemplateWithCompletedArguments(
              &compiler->syntax, cand, args);
        }
        VectorDeleteWithContents(
            args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
      if (inst == NULL || inst->type == NULL || !TypeIsFunction(inst->type) ||
          !(TypeEqual(inst->type, target_fn) ||
            TypeEqualIgnoringQualifiers(inst->type, target_fn))) {
        continue;
      }
      if (template_match != NULL && template_match != inst) {
        template_ambiguous = true;
      } else {
        template_match = inst;
      }
    } else {
      // Explicit template arguments cannot apply to a non-template function.
      if (explicit_args != NULL && explicit_args->length > 0) {
        continue;
      }
      if (TypeEqual(cand->type, target_fn) ||
          TypeEqualIgnoringQualifiers(cand->type, target_fn)) {
        if (non_template_match != NULL && non_template_match != cand) {
          non_template_ambiguous = true;
        } else {
          non_template_match = cand;
        }
      }
    }
  }
  VectorDestruct(&candidates);
  if (non_template_match != NULL) {
    return non_template_ambiguous ? NULL : non_template_match;
  }
  if (template_match != NULL) {
    return template_ambiguous ? NULL : template_match;
  }
  return NULL;
}

// [over.over]: when `from` names a (possibly overloaded / templated) function
// and the required type `to` is a pointer-to-function (or a function type in a
// reference-binding context), resolve the unique matching function and rewrite
// `from` in place to refer to the concrete specialization.  Returns true on a
// successful rewrite.  A no-op (returns false) for non-C++, non-identifier
// operands, or when no unique function matches.
/* [over.over] for `&C::f` converted to a pointer to member function: the
 * parser picked the first overload; select the one whose type is the target's
 * pointee and rewrite the member_ptr node to it.  A template-id (`&C::f<T>`)
 * names only specializations, which the parser already selected; their
 * members are not in the class's overload chain. */
static bool ResolveMemberFunctionPointerNode(ASTNode* from, TypeRecord* to) {
  ASTNode* sub = ((UnaryASTNode*)from)->sub;
  if (sub == NULL || sub->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)sub;
  StructMember* current = member_node->member;
  Struct* owner = TypeMemberPointerClass(from->type);
  if (current == NULL || current->symbol == NULL ||
      !current->is_member_function || owner == NULL) {
    return false;
  }
  StructMember* head = FindStructMember(owner, &current->symbol->name);
  if (head == NULL || head->overload_next == NULL) {
    return false;
  }
  bool declared_overload = false;
  for (StructMember* cand = head; cand != NULL; cand = cand->overload_next) {
    declared_overload |= cand == current;
  }
  if (!declared_overload) {
    return false;
  }
  StructMember* match = NULL;
  bool ambiguous = false;
  for (StructMember* cand = head; cand != NULL; cand = cand->overload_next) {
    if (cand->is_static || !cand->is_member_function || cand->symbol == NULL ||
        cand->symbol->type == NULL || cand->symbol->flags.is_template) {
      continue;
    }
    TypeRecord* pointee = TypeMemberPointerPointeeFromMember(cand);
    bool same = pointee != NULL && (TypeEqual(pointee, to->next) ||
                                    TypeEqualIgnoringQualifiers(pointee, to->next));
    TypeRecordDelete(pointee);
    if (!same) {
      continue;
    }
    if (match != NULL && match != cand) {
      ambiguous = true;
    }
    match = cand;
  }
  if (match == NULL || ambiguous || match == current) {
    return false;
  }
  member_node->member = match;
  member_node->access = match->access;
  ASTNodeSetType(sub, match->symbol->type);
  match->symbol->flags.used = true;
  TypeRecord* mptr = NewMemberPointerTypeRecord(owner, kQualPlain);
  TypeRecordChain(mptr, TypeMemberPointerPointeeFromMember(match));
  TypeRecordCalculateSize(mptr);
  ASTNodeSetType(from, mptr);
  return true;
}

bool CXXTryResolveFunctionAddressNode(ASTNode* from, TypeRecord* to) {
  if (!CompilerIsCXX() || from == NULL || to == NULL) {
    return false;
  }
  if (from->op == AST_OP(member_ptr) && TypeIsMemberPointer(to) &&
      to->next != NULL && TypeIsFunction(to->next) &&
      TypeIsMemberPointer(from->type)) {
    return ResolveMemberFunctionPointerNode(from, to);
  }
  TypeRecord* target_fn = NULL;
  if (TypeIsPointer(to) && to->next != NULL && TypeIsFunction(to->next)) {
    target_fn = to->next;
  } else if (TypeIsReference(to) && to->next != NULL && TypeIsPointer(to->next) &&
             to->next->next != NULL && TypeIsFunction(to->next->next)) {
    target_fn = to->next->next;
  } else if (TypeIsFunction(to)) {
    target_fn = to;
  } else {
    return false;
  }
  // `&InvokeObject<F&, R, Args...>` is an address node whose operand names the
  // overload set.  The operand is analyzed before the destination type is
  // known, so an overloaded function template is still the primary.  Resolve
  // it now that the target signature is available.
  ASTNode* function_node = from;
  if (from->op == AST_OP(address)) {
    ASTNode* sub = ((UnaryASTNode*)from)->sub;
    if (sub != NULL && sub->op == AST_OP(identifier)) {
      function_node = sub;
    }
  }
  if (function_node->type == NULL || !TypeIsFunction(function_node->type) ||
      function_node->op != AST_OP(identifier)) {
    return false;
  }
  // Only the un-substituted template pattern (or an unresolved overload set)
  // needs help here; a concrete function whose type already matches is handled
  // by the ordinary function-to-pointer conversion.
  IdentifierASTNode* id = (IdentifierASTNode*)function_node;
  if (id->symbol == NULL) {
    return false;
  }
  Symbol* resolved = CXXResolveFunctionAddressForTargetType(
      id->symbol, id->template_arguments, target_fn);
  if (resolved == NULL || resolved->type == NULL) {
    return false;
  }
  id->symbol = resolved;
  resolved->flags.used = true;
  // An unanalyzed identifier (a constexpr brace element) still shares the
  // primary template's declaration type.
  ASTNodeSetInstantiatedCalleeType(function_node, resolved->type);
  function_node->flags |= kASTNeedAddress;
  TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, resolved);
  if (function_node != from) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, function_node->type);
    ASTNodeSetType(from, ptr);
  }
  return true;
}

static void EmitFunctionTemplateCandidateRejectionNote(
    Symbol* candidate, VectorASTNode* call, Vector* explicit_args,
    size_t first_formal_arg) {
  if (candidate == NULL || !SymbolIsTemplateFunction(candidate)) {
    return;
  }
  FunctionTemplateCandidateStatus status =
      TypeClassifyFunctionTemplateCandidate(&compiler->syntax, candidate,
                                            explicit_args, call->children,
                                            first_formal_arg);
  if (status == kFunctionTemplateCandidateConstraintsNotSatisfied) {
    EmitCandidateNote(candidate, NULL);
    Vector* args = TypeDeduceFunctionTemplateArgumentsFromCall(
        candidate, call->children, first_formal_arg);
    if (args == NULL && explicit_args != NULL) {
      args = TemplateArgumentVectorCopy(explicit_args);
    }
    if (args != NULL) {
      ConceptsReportFunctionTemplateConstraintFailure(candidate, args);
      VectorDeleteWithContents(
          args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    return;
  }
  EmitCandidateNote(candidate,
                    "could not deduce template arguments for this call");
}

void ReportUnsatisfiedFunctionTemplateConstraints(VectorASTNode* node,
                                                         Symbol* templ,
                                                         Vector* explicit_args,
                                                         size_t first_formal_arg) {
  String function_name;
  StringInit(&function_name, NULL);
  SymbolFunctionDiagnosticName(templ, &function_name);
  SemanticError((ASTNode*)node, "constraints not satisfied for function template %s",
                function_name.value);
  StringDestruct(&function_name);
  if (templ != NULL && TypeIsFunction(templ->type)) {
    Vector* args =
        TypeDeduceFunctionTemplateArgumentsFromCall(templ, node->children,
                                                    first_formal_arg);
    if (args == NULL && explicit_args != NULL) {
      args = TemplateArgumentVectorCopy(explicit_args);
    }
    ConceptsReportFunctionTemplateConstraintFailure(templ, args);
    if (args != NULL) {
      VectorDeleteWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    }
  }
}

static int FunctionTemplatePatternTypeSpecificity(TypeRecord* type) {
  if (type == NULL) {
    return 0;
  }
  int score = 0;
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= 0) {
      continue;
    }
    if (TypeIsReference(t)) {
      // A forwarding-reference pattern `T&&` accepts essentially any type and
      // is less specialized than structural patterns such as `T*`.
      score += 0;
    } else if (TypeIsPointer(t) || TypeIsArray(t)) {
      score += 1;
    } else {
      score += 2;
    }
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL &&
        t->template_arguments != NULL) {
      score += 4;
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        TemplateArgument* arg = t->template_arguments->value.p[i];
        if (arg != NULL && arg->kind == kTemplateParameterType) {
          score += FunctionTemplatePatternTypeSpecificity(arg->type);
        }
      }
    }
  }
  return score;
}

static int FunctionTemplatePatternSpecificity(Symbol* instantiated) {
  if (instantiated == NULL || instantiated->type == NULL ||
      !TypeIsFunction(instantiated->type)) {
    return 0;
  }
  Symbol* origin = instantiated->type->info.function.template_origin;
  if (origin == NULL || origin->type == NULL || !TypeIsFunction(origin->type)) {
    return 0;
  }
  int score = 0;
  Vector* prototype = &origin->type->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    if (formal != NULL) {
      score += FunctionTemplatePatternTypeSpecificity(formal->type);
    }
  }
  return score;
}

int CompareFunctionTemplateSpecificity(Symbol* left, Symbol* right) {
  bool left_is_template = left != NULL && left->type != NULL &&
                          TypeIsFunction(left->type) &&
                          left->type->info.function.template_origin != NULL;
  bool right_is_template = right != NULL && right->type != NULL &&
                           TypeIsFunction(right->type) &&
                           right->type->info.function.template_origin != NULL;
  if (!left_is_template || !right_is_template) {
    return 0;
  }
  int left_score = FunctionTemplatePatternSpecificity(left);
  int right_score = FunctionTemplatePatternSpecificity(right);
  if (left_score > right_score) {
    return 1;
  }
  if (right_score > left_score) {
    return -1;
  }
  // Identical structural scores still leave `equal(It, It, It, It)` tied with
  // `equal(It, It, It, Pred)`.  Partial ordering distinguishes a repeated
  // parameter from an independent one.
  return TypeFunctionTemplateMoreSpecialized(left, right);
}

static Vector* ClassTemplateArgumentsFromMemberAccess(
    BinaryASTNode* member_access) {
  if (member_access == NULL || member_access->left == NULL) {
    return NULL;
  }
  TypeRecord* receiver_type = member_access->left->type;
  if (receiver_type == NULL) {
    return NULL;
  }
  if (TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_type == NULL) {
    return NULL;
  }
  if (receiver_type->template_arguments != NULL) {
    return receiver_type->template_arguments;
  }
  if (TypeIsStructOrUnion(receiver_type) &&
      receiver_type->info.struct_info != NULL) {
    if (receiver_type->template_arguments != NULL) {
      return receiver_type->template_arguments;
    }
    if (receiver_type->info.struct_info->tag_symbol != NULL &&
        receiver_type->info.struct_info->tag_symbol->type != NULL &&
        receiver_type->info.struct_info->tag_symbol->type->template_arguments !=
            NULL) {
      return receiver_type->info.struct_info->tag_symbol->type
          ->template_arguments;
    }
  }
  return NULL;
}

/* Contract: evaluates associated constraints for a member template in context. */
static bool MemberFunctionConstraintsSatisfied(StructMember* candidate,
                                               BinaryASTNode* member_access) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type) ||
      FunctionExtraOf(&candidate->symbol->type->info.function)
              ->associated_constraint == NULL) {
    return true;
  }
  ConstraintExpr* constraint =
      FunctionExtraOf(&candidate->symbol->type->info.function)
          ->associated_constraint;
  Vector* class_args = ClassTemplateArgumentsFromMemberAccess(member_access);
  if (class_args == NULL) {
    Struct* owner =
        candidate->symbol->type->info.function.cxx_member_owner;
    if (owner != NULL && owner->tag_symbol != NULL &&
        owner->tag_symbol->type != NULL) {
      class_args = owner->tag_symbol->type->template_arguments;
    }
  }
  if (class_args == NULL &&
      ConceptsConstraintContainsTemplateParameter(constraint)) {
    return true;
  }
  return ConceptsConstraintSatisfied(constraint, class_args);
}

// Contract: First actual for explicit-object member calls. Returns `left` for `.`;
// for `->` writes a dereference into caller-owned `scratch` (must outlive the
// actuals vector). NULL when the access form cannot yield an object parameter.
static ASTNode* ExplicitObjectReceiverForCall(BinaryASTNode* member_access,
                                              ASTNode* scratch) {
  if (member_access == NULL || member_access->left == NULL) {
    return NULL;
  }
  if (member_access->base.op == AST_OP(dot)) {
    return member_access->left;
  }
  if (!TypeIsPointer(member_access->left->type) ||
      member_access->left->type->next == NULL) {
    return NULL;
  }
  *scratch = *member_access->left;
  scratch->op = AST_OP(contents);
  scratch->type = member_access->left->type->next;
  scratch->value_category = kValueCategoryLvalue;
  return scratch;
}

/* Contract: builds actuals for an explicit-object member call (receiver first). */
static void BuildExplicitObjectCallActuals(VectorASTNode* call,
                                           BinaryASTNode* member_access,
                                           ASTNode* receiver_scratch,
                                           Vector* actuals) {
  VectorInit(actuals);
  ASTNode* receiver =
      ExplicitObjectReceiverForCall(member_access, receiver_scratch);
  if (receiver != NULL) {
    VectorAppend(actuals, receiver);
  }
  for (size_t i = 0; call->children != NULL &&
                     i < call->children->length; i++) {
    VectorAppend(actuals, call->children->value.p[i]);
  }
}

/* Contract: FunctionCallScore plus member cv/ref/constraint checks; -1 if rejected. */
static int MemberOverloadCallScore(StructMember* candidate,
                                   VectorASTNode* node,
                                   BinaryASTNode* member_access,
                                   bool check_receiver_const) {
  TypeRecord* function_type = candidate->symbol->type;
  if (FunctionHasExplicitObjectParameter(function_type)) {
    ASTNode receiver_scratch;
    Vector actuals;
    BuildExplicitObjectCallActuals(node, member_access, &receiver_scratch,
                                   &actuals);
    VectorASTNode explicit_call = *node;
    explicit_call.children = &actuals;
    int score = FunctionCallScore(function_type, &explicit_call, 0);
    VectorDestruct(&actuals);
    if (score >= 0 &&
        !MemberFunctionConstraintsSatisfied(candidate, member_access)) {
      return -1;
    }
    return score;
  }
  size_t first_formal_arg = candidate->is_static ? 0 : 1;
  if (check_receiver_const && !candidate->is_static &&
      !candidate->symbol->type->info.function.is_const_member &&
      !candidate->symbol->type->info.function.is_constructor &&
      !candidate->symbol->type->info.function.is_destructor &&
      MemberReceiverIsConst(member_access)) {
    return -1;
  }
  if (check_receiver_const && !candidate->is_static &&
      !candidate->symbol->type->info.function.is_volatile_member &&
      !candidate->symbol->type->info.function.is_constructor &&
      !candidate->symbol->type->info.function.is_destructor &&
      MemberReceiverIsVolatile(member_access)) {
    return -1;
  }
  if (!candidate->is_static &&
      !MemberReceiverMatchesRefQualifier(candidate->symbol->type,
                                         member_access)) {
    return -1;
  }
  int score = FunctionCallScore(candidate->symbol->type, node, first_formal_arg);
  if (score >= 0 &&
      !MemberFunctionConstraintsSatisfied(candidate, member_access)) {
    return -1;
  }
  return score;
}

/* Calling a const member on a non-const object is a worse implicit-object
 * binding than the non-const overload, and binding an rvalue object to
 * `f() const&` is worse than to `f() &&` / `f() const&&`, which ranks first
 * ([over.ics.rank]).  This is a tie-break only: folding it into the
 * conversion score made an exact `slot_type*` match look worse than 5 and let
 * `hash_of(const K&)` steal a call that passes the slot pointer. */
static int MemberCvTiePenalty(StructMember* candidate,
                              BinaryASTNode* member_access) {
  if (candidate == NULL || candidate->is_static || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type)) {
    return 0;
  }
  int penalty = 0;
  if (candidate->symbol->type->info.function.ref_qualifier ==
          kCXXRefQualifierLValue &&
      !MemberReceiverIsLValue(member_access)) {
    penalty += 2;
  }
  if (candidate->symbol->type->info.function.is_const_member &&
      !MemberReceiverIsConst(member_access)) {
    penalty++;
  }
  if (candidate->symbol->type->info.function.is_volatile_member &&
      !MemberReceiverIsVolatile(member_access)) {
    penalty++;
  }
  return penalty;
}

// Emits one note per member-function overload after a failed member-call /
// member-operator resolution.  When `ambiguous` is false each note explains why
// the candidate was rejected (argument mismatch, or a const object passed to a
// non-const method); when true, the equally-best candidates are listed.
static void EmitMemberCandidateNotes(StructMember* first, VectorASTNode* node,
                                     BinaryASTNode* member_access,
                                     Vector* explicit_args, bool ambiguous,
                                     int best_score) {
  bool receiver_const = MemberReceiverIsConst(member_access);
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL) {
      continue;
    }
    bool is_template = SymbolIsTemplateFunction(candidate->symbol);
    if (ambiguous) {
      if (!is_template &&
          MemberOverloadCallScore(candidate, node, member_access, true) ==
              best_score) {
        EmitCandidateNote(candidate->symbol, NULL);
      }
      continue;
    }
    if (is_template) {
      size_t first_formal_arg = candidate->is_static ? 0 : 1;
      EmitFunctionTemplateCandidateRejectionNote(candidate->symbol, node,
                                                 explicit_args,
                                                 first_formal_arg);
      continue;
    }
    // A non-static method rejected only because the object is const (it would
    // be viable on a non-const object) gets a dedicated explanation.
    if (receiver_const && !candidate->is_static &&
        MemberOverloadCallScore(candidate, node, member_access, true) < 0 &&
        MemberOverloadCallScore(candidate, node, member_access, false) >= 0) {
      EmitCandidateNote(candidate->symbol,
                        "'this' argument has a const-qualified type, but the "
                        "method is not declared const");
      continue;
    }
    size_t first_formal_arg = candidate->is_static ? 0 : 1;
    String reason;
    StringInit(&reason, NULL);
    DescribeFunctionNonViability(candidate->symbol->type, node, first_formal_arg,
                                 &reason);
    EmitCandidateNote(candidate->symbol, reason.value);
    StringDestruct(&reason);
  }
}

/* Contract: like FunctionTemplateOverloadCandidate for a StructMember overload. */
static StructMember* MemberTemplateOverloadCandidate(StructMember* candidate,
                                                     VectorASTNode* node,
                                                     BinaryASTNode* member_access,
                                                     Vector* explicit_args,
                                                     Vector* temporary_members) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type)) {
    return candidate;
  }
  if (candidate->symbol->type->info.function.template_origin != NULL) {
    return NULL;
  }
  if (!candidate->symbol->flags.is_template) {
    return explicit_args == NULL ? candidate : NULL;
  }
  bool explicit_object =
      FunctionHasExplicitObjectParameter(candidate->symbol->type);
  size_t first_formal_arg = candidate->is_static || explicit_object ? 0 : 1;
  ASTNode receiver_scratch;
  Vector explicit_object_actuals;
  Vector* deduction_actuals = node->children;
  if (explicit_object) {
    BuildExplicitObjectCallActuals(node, member_access, &receiver_scratch,
                                   &explicit_object_actuals);
    deduction_actuals = &explicit_object_actuals;
  }
  Vector* prototype = &candidate->symbol->type->info.function.prototype;
  bool braced_parameters_deduced_elsewhere = false;
  bool braced_parameters_checked = false;
  for (size_t i = 0; deduction_actuals != NULL && i < deduction_actuals->length &&
                     i + first_formal_arg < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i + first_formal_arg];
    ASTNode* actual = deduction_actuals->value.p[i];
    TypeRecord* target =
        formal != NULL && TypeIsReference(formal->type) ? formal->type->next
                                                        : formal->type;
    // An `initializer_list<U>` parameter is satisfied either by a braced-init
    // argument (`{1, 2, 3}`) or by an argument that is already an
    // `initializer_list<V>` (e.g. forwarding a bound `init` parameter through a
    // second member template); reject only arguments that are neither.
    if (formal != NULL && actual != NULL && actual->type != NULL &&
        TypeIsCXXInitializerList(target) &&
        actual->op != AST_OP(braced_init) &&
        !TypeIsCXXInitializerList(TypeIsReference(actual->type)
                                      ? actual->type->next
                                      : actual->type)) {
      if (explicit_object) {
        VectorDestruct(&explicit_object_actuals);
      }
      return NULL;
    }
    if (explicit_args == NULL && formal != NULL && actual != NULL &&
        actual->op == AST_OP(braced_init) &&
        TypeContainsTemplateParameter(formal->type) &&
        !TypeIsCXXInitializerList(target) &&
        (target == NULL || !TypeIsArray(target))) {
      if (!braced_parameters_checked) {
        braced_parameters_checked = true;
        braced_parameters_deduced_elsewhere =
            FunctionTemplateDeducesWithoutBracedArguments(
                candidate->symbol, deduction_actuals, first_formal_arg);
      }
      if (braced_parameters_deduced_elsewhere) {
        continue;
      }
      if (explicit_object) {
        VectorDestruct(&explicit_object_actuals);
      }
      return NULL;
    }
  }
  DiagnosticSuppressBegin();
  Symbol* instantiated = TypeCreateFunctionTemplateCandidate(
      &compiler->syntax, candidate->symbol, explicit_args, deduction_actuals,
      first_formal_arg);
  DiagnosticSuppressEnd();
  if (explicit_object) {
    VectorDestruct(&explicit_object_actuals);
  }
  if (instantiated == NULL) {
    return NULL;
  }
  StructMember* member = NewStructMember(instantiated);
  member->is_member_function = true;
  member->is_static = candidate->is_static;
  member->access = candidate->access;
  if (temporary_members != NULL) {
    VectorAppend(temporary_members, member);
  }
  return member;
}

static void DeleteTemporaryMemberTemplateCandidates(Vector* members,
                                                    StructMember* keep) {
  if (members == NULL) {
    return;
  }
  // Candidate members can be installed into member-access ASTs while an
  // enclosing expression is still being re-analysed (notably auto-return and
  // requires-expression instantiation).  A later pass may retain one that was
  // not the final `keep` value observed by this invocation.  Reclaiming the
  // rejected wrappers here leaves that AST with a dangling StructMember and
  // eventually corrupts code generation.  Their symbols/types are already
  // compilation-lifetime template candidates, so keep these tiny wrappers for
  // the same lifetime.
  (void)keep;
  VectorDestruct(members);
}

static StructMember* InstantiateSelectedMemberTemplateCandidate(
    StructMember* selected) {
  if (selected == NULL || selected->symbol == NULL ||
      selected->symbol->type == NULL ||
      !TypeIsFunction(selected->symbol->type) ||
      selected->symbol->type->info.function.template_origin == NULL ||
      selected->symbol->type->template_arguments == NULL) {
    return selected;
  }
  Symbol* instantiated =
      TypeInstantiateFunctionTemplateWithCompletedArguments(
          &compiler->syntax,
          selected->symbol->type->info.function.template_origin,
          selected->symbol->type->template_arguments);
  if (instantiated == NULL) {
    return selected;
  }
  CXXAnalyzeImmediateEscalationCandidate(instantiated);
  StructMember* member = NewStructMember(instantiated);
  member->is_member_function = true;
  member->is_static = selected->is_static;
  member->access = selected->access;
  return member;
}

/* A derived-class constructor hides an inherited constructor with the same
 * parameter list (`flat_hash_set() {}` plus `using Base::Base`). */
static bool MemberExplicitParametersEqual(StructMember* left,
                                          StructMember* right) {
  if (left == NULL || right == NULL || left->symbol == NULL ||
      right->symbol == NULL || left->symbol->type == NULL ||
      right->symbol->type == NULL || !TypeIsFunction(left->symbol->type) ||
      !TypeIsFunction(right->symbol->type)) {
    return false;
  }
  Vector* left_params = &left->symbol->type->info.function.prototype;
  Vector* right_params = &right->symbol->type->info.function.prototype;
  size_t left_start = left->is_static ? 0 : 1;
  size_t right_start = right->is_static ? 0 : 1;
  if (left_params->length < left_start || right_params->length < right_start ||
      left_params->length - left_start !=
          right_params->length - right_start) {
    return false;
  }
  for (size_t i = 0; i < left_params->length - left_start; i++) {
    Symbol* left_param = left_params->value.p[left_start + i];
    Symbol* right_param = right_params->value.p[right_start + i];
    if (left_param == NULL || right_param == NULL ||
        !TypeEqual(left_param->type, right_param->type)) {
      return false;
    }
  }
  return true;
}

static bool DerivedConstructorHidesInherited(StructMember* derived,
                                             StructMember* inherited) {
  if (derived == NULL || inherited == NULL || derived->is_using_declaration ||
      !inherited->is_using_declaration || derived->symbol == NULL ||
      inherited->symbol == NULL || derived->symbol->type == NULL ||
      inherited->symbol->type == NULL ||
      !TypeIsFunction(derived->symbol->type) ||
      !TypeIsFunction(inherited->symbol->type) ||
      !derived->symbol->type->info.function.is_constructor ||
      !inherited->symbol->type->info.function.is_constructor) {
    return false;
  }
  return MemberExplicitParametersEqual(derived, inherited);
}

/* [over.match.funcs]/9: a constructor inherited from C whose first parameter
 * is a reference to C (its copy or move constructor) is not a candidate for a
 * single argument.  `Checked(Cmp)` must build the derived object instead of
 * an inherited `Cmp(Cmp&&)` initializing only the base subobject. */
static bool InheritedConstructorExcludedForSingleArgument(
    StructMember* candidate, VectorASTNode* node) {
  if (!candidate->is_using_declaration || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type) || node->children == NULL ||
      node->children->length != 1) {
    return false;
  }
  FunctionInfo* info = &candidate->symbol->type->info.function;
  if (!info->is_constructor || info->prototype.length < 2) {
    return false;
  }
  Symbol* first = info->prototype.value.p[1];
  if (first == NULL || first->type == NULL || !TypeIsReference(first->type)) {
    return false;
  }
  TypeRecord* referenced = first->type->next;
  return referenced != NULL && TypeIsStructOrUnion(referenced) &&
         referenced->info.struct_info != NULL &&
         referenced->info.struct_info == info->cxx_member_owner;
}

static bool CallHasBracedInitActual(VectorASTNode* node) {
  if (node == NULL || node->children == NULL) {
    return false;
  }
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* actual = node->children->value.p[i];
    if (actual != NULL && actual->op == AST_OP(braced_init)) {
      return true;
    }
  }
  return false;
}

static bool CandidateIsInitializerListConstructor(StructMember* candidate);

/* Implements [over.match.list]'s second phase for direct-list construction.
 * Once no initializer-list constructor is viable, the clauses in the sole
 * braced actual become the ordinary constructor arguments.  The whole list is
 * only passed as one braced actual when the class has initializer-list
 * constructors; otherwise that actual is a genuine argument (`Entry{{a, b}}`
 * or a member initializer `base{v}`) and must stay intact. */
static void ExpandBracedConstructorCallActuals(StructMember* first,
                                               VectorASTNode* node) {
  if (first == NULL || first->symbol == NULL || first->symbol->type == NULL ||
      !TypeIsFunction(first->symbol->type) ||
      !first->symbol->type->info.function.is_constructor ||
      node == NULL || node->children == NULL || node->children->length != 1) {
    return;
  }
  bool has_initializer_list_constructor = false;
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (CandidateIsInitializerListConstructor(candidate)) {
      has_initializer_list_constructor = true;
      break;
    }
  }
  if (!has_initializer_list_constructor) {
    return;
  }
  ASTNode* actual = node->children->value.p[0];
  if (actual == NULL || actual->op != AST_OP(braced_init)) {
    return;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  Vector* expanded = NewVector();
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    if (initializer == NULL) {
      continue;
    }
    initializer = ASTNodeMove(initializer);
    ASTNode* element = initializer;
    if (initializer->op == AST_OP(expr_init)) {
      element =
          ASTNodeMove(((ExpressionInitializerASTNode*)initializer)->expr);
      ASTNodeDelete(initializer);
    }
    if (element != NULL && (element->flags & kASTAnalyzed) == 0 &&
        element->op != AST_OP(braced_init)) {
      element = AnalyzeExpression(element);
    }
    if (element != NULL) {
      VectorAppend(expanded, element);
    }
  }
  actual = ASTNodeMove(actual);
  ASTNodeDelete(actual);
  VectorDeleteWithContents(
      node->children, (VectorElementDestructor)ASTNodeDelete,
      /*free_element=*/false);
  node->children = expanded;
  for (size_t i = 0; i < expanded->length; i++) {
    ASTNode* element = expanded->value.p[i];
    element->parent = (ASTNode*)node;
    element->child_id = (int)i;
  }
}

static bool CandidateIsInitializerListConstructor(StructMember* candidate) {
  return candidate != NULL && candidate->symbol != NULL &&
         candidate->symbol->type != NULL &&
         TypeIsFunction(candidate->symbol->type) &&
         CXXConstructorIsInitializerListConstructor(
             &candidate->symbol->type->info.function);
}

/* Contract: [over.match.list] — true when a braced-init call must consider only
 * initializer-list constructors (no explicit template args on the member). */
static bool MemberCallOnlyInitializerListConstructors(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    bool explicit_args_present) {
  (void)member_access;
  if (explicit_args_present || first == NULL || first->symbol == NULL ||
      first->symbol->type == NULL ||
      !TypeIsFunction(first->symbol->type) ||
      !first->symbol->type->info.function.is_constructor ||
      node == NULL || node->children == NULL || node->children->length != 1 ||
      !CallHasBracedInitActual(node)) {
    return false;
  }
  Struct* owner = first->symbol->type->info.function.cxx_member_owner;
  TypeRecord* target =
      owner != NULL && owner->tag_symbol != NULL ? owner->tag_symbol->type
                                                 : NULL;
  ASTNode* actual = node->children->value.p[0];
  if (target != NULL &&
      CXXBracedInitSelectsInitializerListConstructor(actual, target)) {
    return true;
  }
  return false;
}

/* Contract: skips template symbols; updates ranking state and tracks const
 * receiver rejections for later diagnostics. */
static void RankNonTemplateMemberCandidates(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    bool only_initializer_list_constructors, StructMember** best,
    int* best_score, bool* ambiguous, bool* receiver_const_rejected) {
  bool receiver_const = MemberReceiverIsConst(member_access);
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol != NULL &&
        (candidate->symbol->flags.is_template ||
         (candidate->symbol->type != NULL &&
          TypeIsFunction(candidate->symbol->type) &&
          candidate->symbol->type->info.function.template_origin != NULL))) {
      continue;
    }
    if (only_initializer_list_constructors &&
        !CandidateIsInitializerListConstructor(candidate)) {
      continue;
    }
    if (InheritedConstructorExcludedForSingleArgument(candidate, node)) {
      continue;
    }
    int score = MemberOverloadCallScore(candidate, node, member_access, true);
    if (score < 0) {
      if (receiver_const &&
          MemberOverloadCallScore(candidate, node, member_access, false) >= 0) {
        *receiver_const_rejected = true;
      }
      continue;
    }
    if (*best == NULL || score < *best_score) {
      *best = candidate;
      *best_score = score;
      *ambiguous = false;
    } else if (score == *best_score) {
      ApplyMemberNonTemplateEqualScoreTie(candidate, best, ambiguous,
                                          member_access);
    }
  }
}

/* Contract: member overload tie-break for equal conversion scores (cv-qual,
 * derived constructor hiding, duplicate signatures). */
static void ApplyMemberNonTemplateEqualScoreTie(
    StructMember* candidate, StructMember** best, bool* ambiguous,
    BinaryASTNode* member_access) {
  int best_penalty = MemberCvTiePenalty(*best, member_access);
  int penalty = MemberCvTiePenalty(candidate, member_access);
  if (penalty < best_penalty) {
    *best = candidate;
    *ambiguous = false;
  } else if (penalty > best_penalty) {
    // The non-const (or less cv-qualified) overload stays preferred.
  } else if (DerivedConstructorHidesInherited(*best, candidate)) {
    // The derived constructor hides this inherited one.
  } else if (DerivedConstructorHidesInherited(candidate, *best)) {
    *best = candidate;
    *ambiguous = false;
  } else if ((*best)->symbol == NULL || candidate->symbol == NULL ||
             !SameFunctionSignature((*best)->symbol, candidate->symbol)) {
    *ambiguous = true;
  }
}

/* Contract: template member tie when conversion scores are equal. */
static void ApplyMemberTemplateEqualScoreTie(StructMember* effective,
                                             StructMember** best,
                                             bool* ambiguous, int score,
                                             int best_score,
                                             BinaryASTNode* member_access) {
  if (score != best_score) {
    return;
  }
  if (MemberCvTiePenalty(effective, member_access) <
      MemberCvTiePenalty(*best, member_access)) {
    *best = effective;
    *ambiguous = false;
    return;
  }
  if (MemberCvTiePenalty(effective, member_access) >
      MemberCvTiePenalty(*best, member_access)) {
    // The less cv-qualified overload stays preferred.
    return;
  }
  if (DerivedConstructorHidesInherited(*best, effective)) {
    // The derived constructor hides this inherited one.
    return;
  }
  if (DerivedConstructorHidesInherited(effective, *best)) {
    *best = effective;
    *ambiguous = false;
    return;
  }
  bool best_is_template =
      (*best)->symbol != NULL && (*best)->symbol->type != NULL &&
      TypeIsFunction((*best)->symbol->type) &&
      (*best)->symbol->type->info.function.template_origin != NULL;
  bool effective_is_template =
      effective->symbol != NULL && effective->symbol->type != NULL &&
      TypeIsFunction(effective->symbol->type) &&
      effective->symbol->type->info.function.template_origin != NULL;
  if (best_is_template && !effective_is_template) {
    *best = effective;
    *ambiguous = false;
  } else if (best_is_template && effective_is_template && *best != effective) {
    int template_order = CompareFunctionTemplateSpecificity(
        effective->symbol, (*best)->symbol);
    if (template_order > 0) {
      *best = effective;
      *ambiguous = false;
    } else if (template_order < 0) {
      *ambiguous = false;
    } else {
      *ambiguous = true;
    }
  } else if (*best != effective && best_is_template == effective_is_template &&
             ((*best)->symbol == NULL || effective->symbol == NULL ||
              !SameFunctionSignature((*best)->symbol, effective->symbol))) {
    *ambiguous = true;
  }
}

/* Contract: scores member function templates when no exact non-template match
 * (score threshold > 5).  Appends wrapper StructMembers to `temporary_members`. */
static void RankMemberTemplateCandidates(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    Vector* explicit_args, bool only_initializer_list_constructors,
    Vector* temporary_members, StructMember** best, int* best_score,
    bool* ambiguous, bool* receiver_const_rejected) {
  size_t first_formal_arg =
      *best != NULL && (*best)->is_static ? 0 : 1;
  if (*best_score >= 0 && *best_score <= 5 && *best != NULL &&
      !FunctionHasRvalueToConstLvalueReferenceBinding(
          (*best)->symbol != NULL ? (*best)->symbol->type : NULL, node,
          first_formal_arg)) {
    return;
  }
  bool receiver_const = MemberReceiverIsConst(member_access);
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (only_initializer_list_constructors &&
        !CandidateIsInitializerListConstructor(candidate)) {
      continue;
    }
    StructMember* effective =
        MemberTemplateOverloadCandidate(candidate, node, member_access,
                                        explicit_args, temporary_members);
    if (effective == NULL) {
      continue;
    }
    int score = MemberOverloadCallScore(effective, node, member_access, true);
    if (score < 0) {
      if (receiver_const &&
          MemberOverloadCallScore(effective, node, member_access, false) >= 0) {
        *receiver_const_rejected = true;
      }
      continue;
    }
    if (*best == NULL || score < *best_score) {
      *best = effective;
      *best_score = score;
      *ambiguous = false;
    } else {
      ApplyMemberTemplateEqualScoreTie(effective, best, ambiguous, score,
                                       *best_score, member_access);
    }
  }
}

/* Contract: failure-path diagnostics for member overload resolution.  Always
 * returns `first` (overload head) except when undeduced-auto deferral returns
 * NULL from the caller.  Destroys `temporary_members`. */
static StructMember* DiagnoseFailedMemberOverload(
    StructMember* first, VectorASTNode* node, BinaryASTNode* member_access,
    Vector* explicit_args, bool receiver_const_rejected,
    Vector* temporary_members) {
  bool already_diagnosed = ((ASTNode*)node)->flags & kASTOverloadDiagnosed;
  if (already_diagnosed) {
    DeleteTemporaryMemberTemplateCandidates(temporary_members, NULL);
    return first;
  }
  StructMember* constraint_rejected = NULL;
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL ||
        !ConceptsFunctionTemplateHasAssociatedConstraint(candidate->symbol)) {
      if (candidate->symbol != NULL && candidate->symbol->type != NULL &&
          TypeIsFunction(candidate->symbol->type) &&
          FunctionExtraOf(&candidate->symbol->type->info.function)
                  ->associated_constraint != NULL &&
          !MemberFunctionConstraintsSatisfied(candidate, member_access)) {
        constraint_rejected = candidate;
        break;
      }
      continue;
    }
    size_t first_formal_arg = candidate->is_static ? 0 : 1;
    Vector* args = TypeDeduceFunctionTemplateArgumentsFromCall(
        candidate->symbol, node->children, first_formal_arg);
    bool rejected = args != NULL &&
                    !ConceptsFunctionTemplateConstraintsSatisfied(
                        candidate->symbol, args);
    if (args != NULL) {
      VectorDeleteWithContents(
          args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    if (rejected) {
      constraint_rejected = candidate;
      break;
    }
  }
  if (constraint_rejected != NULL) {
    if (ConceptsFunctionTemplateHasAssociatedConstraint(
            constraint_rejected->symbol)) {
      size_t first_formal_arg = constraint_rejected->is_static ? 0 : 1;
      ReportUnsatisfiedFunctionTemplateConstraints(
          node, constraint_rejected->symbol, explicit_args, first_formal_arg);
    } else {
      SemanticError((ASTNode*)node, "constraints not satisfied");
      if (constraint_rejected->symbol != NULL &&
          TypeIsFunction(constraint_rejected->symbol->type)) {
        ConceptsReportAssociatedConstraintFailure(
            FunctionExtraOf(&constraint_rejected->symbol->type->info.function)
                ->associated_constraint,
            ClassTemplateArgumentsFromMemberAccess(member_access),
            constraint_rejected->symbol->location, NULL);
      }
    }
    ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
  } else if (receiver_const_rejected) {
    String function_name;
    StringInit(&function_name, NULL);
    SymbolFunctionDiagnosticSuffix(first->symbol, &function_name);
    SemanticError((ASTNode*)member_access,
                  "Cannot call non-const member function %s on const object%s",
                  first->symbol->name.value, function_name.value);
    StringDestruct(&function_name);
    EmitMemberCandidateNotes(first, node, member_access, explicit_args,
                             /*ambiguous=*/false, -1);
    ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
  } else if (first->overload_next != NULL ||
             first->symbol->type->info.function.is_constructor) {
    String function_name;
    StringInit(&function_name, NULL);
    SymbolFunctionDiagnosticName(first->symbol, &function_name);
    SemanticError((ASTNode*)node, "No matching overload for %s",
                  function_name.value);
    StringDestruct(&function_name);
    EmitMemberCandidateNotes(first, node, member_access, explicit_args,
                             /*ambiguous=*/false, -1);
    ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
  } else if (first != NULL &&
             MemberOverloadCallScore(first, node, member_access, true) < 0 &&
             first->symbol != NULL && first->symbol->type != NULL &&
             TypeIsFunction(first->symbol->type) &&
             FunctionExtraOf(&first->symbol->type->info.function)
                     ->associated_constraint != NULL) {
    SemanticError((ASTNode*)node, "constraints not satisfied");
    ConceptsReportAssociatedConstraintFailure(
        FunctionExtraOf(&first->symbol->type->info.function)
            ->associated_constraint,
        ClassTemplateArgumentsFromMemberAccess(member_access),
        first->symbol->location, NULL);
    ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
  }
  DeleteTemporaryMemberTemplateCandidates(temporary_members, NULL);
  return first;
}

/* Contract: resolves a member-function overload set for `node` on
 * `member_access`; may diagnose and always materializes the winning template. */
StructMember* ResolveMemberFunctionOverload(StructMember* first,
                                                   VectorASTNode* node,
                                                   BinaryASTNode* member_access) {
  StructMember* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  bool receiver_const_rejected = false;
  Vector temporary_members;
  VectorInit(&temporary_members);
  Vector* explicit_args = NULL;
  if (member_access->right != NULL &&
      member_access->right->op == AST_OP(structmember)) {
    explicit_args =
        ((StructMemberASTNode*)member_access->right)->template_arguments;
  }

  bool only_initializer_list_constructors =
      MemberCallOnlyInitializerListConstructors(
          first, node, member_access, explicit_args != NULL);
  if (!only_initializer_list_constructors && explicit_args == NULL) {
    ExpandBracedConstructorCallActuals(first, node);
  }

  if (explicit_args == NULL) {
    RankNonTemplateMemberCandidates(
        first, node, member_access, only_initializer_list_constructors, &best,
        &best_score, &ambiguous, &receiver_const_rejected);
  }

  RankMemberTemplateCandidates(first, node, member_access, explicit_args,
                               only_initializer_list_constructors,
                               &temporary_members, &best, &best_score,
                               &ambiguous, &receiver_const_rejected);

  bool already_diagnosed =
      ((ASTNode*)node)->flags & kASTOverloadDiagnosed;
  if (best == NULL) {
    if (CallActualsContainUndeducedAuto(node->children)) {
      DeleteTemporaryMemberTemplateCandidates(&temporary_members, NULL);
      return NULL;
    }
    return DiagnoseFailedMemberOverload(first, node, member_access, explicit_args,
                                        receiver_const_rejected,
                                        &temporary_members);
  }
  if (ambiguous) {
    if (already_diagnosed) {
      DeleteTemporaryMemberTemplateCandidates(&temporary_members, best);
      return best;
    }
    String function_name;
    StringInit(&function_name, NULL);
    SymbolFunctionDiagnosticName(first->symbol, &function_name);
    SemanticError((ASTNode*)node, "Ambiguous overload for %s",
                  function_name.value);
    StringDestruct(&function_name);
    EmitMemberCandidateNotes(first, node, member_access, explicit_args,
                             /*ambiguous=*/true, best_score);
    ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    DeleteTemporaryMemberTemplateCandidates(&temporary_members, best);
    return best;
  }
  if (best != NULL && !MemberFunctionConstraintsSatisfied(best, member_access)) {
    if (!already_diagnosed) {
      SemanticError((ASTNode*)node, "constraints not satisfied");
      if (best->symbol != NULL && TypeIsFunction(best->symbol->type)) {
        ConceptsReportAssociatedConstraintFailure(
            FunctionExtraOf(&best->symbol->type->info.function)
                ->associated_constraint,
            ClassTemplateArgumentsFromMemberAccess(member_access),
            best->symbol->location, NULL);
      }
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    }
    DeleteTemporaryMemberTemplateCandidates(
        &temporary_members, best);
    return best;
  }
  StructMember* resolved = InstantiateSelectedMemberTemplateCandidate(best);
  if (resolved != NULL) {
    CXXAnalyzeImmediateEscalationCandidate(resolved->symbol);
  }
  DeleteTemporaryMemberTemplateCandidates(
      &temporary_members, resolved == best ? best : NULL);
  return resolved;
}

/* Contract: when the callee is already a function-template specialization,
 * instantiates it if viable; otherwise resets to the primary template.
 * Returns true if resolution for this call is complete. */
static bool TryReuseSelectedFunctionTemplateCallee(VectorASTNode* node,
                                                     IdentifierASTNode* id) {
  if (id->symbol == NULL || id->symbol->type == NULL ||
      !TypeIsFunction(id->symbol->type) ||
      id->symbol->type->info.function.template_origin == NULL) {
    return false;
  }
  Symbol* origin = id->symbol->type->info.function.template_origin;
  Vector* prototype = &id->symbol->type->info.function.prototype;
  Symbol* first_formal =
      prototype->length > 0 ? prototype->value.p[0] : NULL;
  bool implicit_object_call =
      id->symbol->type->info.function.cxx_member_owner != NULL &&
      first_formal != NULL && first_formal->name.value != NULL &&
      strcmp(first_formal->name.value, "this") == 0;
  bool selected_matches =
      implicit_object_call ||
      FunctionCallScore(id->symbol->type, node, /*first_formal_arg=*/0) >= 0;
  if (selected_matches) {
    Symbol* instantiated =
        InstantiateSelectedFunctionTemplateCandidate(id->symbol);
    if (instantiated != NULL) {
      id->symbol = instantiated;
      ASTNodeSetInstantiatedCalleeType(node->left, instantiated->type);
      CheckDeletedFunctionUse(instantiated, (ASTNode*)node);
    }
    return true;
  }
  id->symbol = origin;
  return false;
}

/* Contract: picks the best non-template constructor when prototype arity differs
 * from the lowered call (defaults and [over.match.list]). */
static StructMember* RankLoweredConstructorNonTemplateMatch(
    StructMember* head, VectorASTNode* node) {
  bool only_initializer_list = false;
  for (StructMember* candidate = head; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->flags.is_template ||
        !CandidateIsInitializerListConstructor(candidate)) {
      continue;
    }
    if (FunctionCallScore(candidate->symbol->type, node,
                          /*first_formal_arg=*/0) >= 0) {
      only_initializer_list = true;
      break;
    }
  }
  StructMember* match = NULL;
  int best_score = -1;
  for (StructMember* candidate = head; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        candidate->symbol->flags.is_template ||
        !candidate->symbol->type->info.function.is_constructor) {
      continue;
    }
    if (only_initializer_list &&
        !CandidateIsInitializerListConstructor(candidate)) {
      continue;
    }
    int score = FunctionCallScore(candidate->symbol->type, node,
                                  /*first_formal_arg=*/0);
    if (score >= 0 && (match == NULL || score < best_score)) {
      match = candidate;
      best_score = score;
    }
  }
  return match;
}

/* Contract: deduces constructor templates against user args (receiver at [0]). */
static Symbol* DeduceBestLoweredConstructorTemplate(StructMember* head,
                                                    VectorASTNode* node,
                                                    size_t num_actual) {
  Symbol* best_inst = NULL;
  int best_inst_score = -1;
  Vector user_actuals;
  VectorInit(&user_actuals);
  for (size_t i = 1; node->children != NULL && i < node->children->length;
       i++) {
    VectorAppend(&user_actuals, node->children->value.p[i]);
  }
  for (StructMember* candidate = head; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || !candidate->symbol->flags.is_template ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        !candidate->symbol->type->info.function.is_constructor) {
      continue;
    }
    DiagnosticSuppressBegin();
    Symbol* inst = TypeDeduceFunctionTemplateFromCallWithOffset(
        &compiler->syntax, candidate->symbol, &user_actuals,
        /*first_formal_arg=*/1);
    DiagnosticSuppressEnd();
    if (inst == NULL || inst == candidate->symbol || inst->type == NULL ||
        !TypeIsFunction(inst->type) ||
        inst->type->info.function.prototype.length != num_actual) {
      continue;
    }
    bool compatible = true;
    int score = 0;
    for (size_t i = 1; i < num_actual; i++) {
      ASTNode* actual = node->children->value.p[i];
      Symbol* formal = inst->type->info.function.prototype.value.p[i];
      if (actual == NULL || formal == NULL || formal->type == NULL) {
        compatible = false;
        break;
      }
      if (actual->type == NULL ||
          TypeContainsTemplateParameter(formal->type)) {
        continue;
      }
      int rank = OverloadConversionRank(actual, formal->type);
      if (rank < 0) {
        compatible = false;
        break;
      }
      score += rank;
    }
    if (compatible && (best_inst == NULL || score < best_inst_score)) {
      best_inst = inst;
      best_inst_score = score;
    }
  }
  VectorDestruct(&user_actuals);
  return best_inst;
}

/* Contract: rebinds a lowered constructor call to the correct overload/template. */
static void ResolveLoweredConstructorOverload(VectorASTNode* node,
                                              IdentifierASTNode* id) {
  if (id->symbol == NULL || id->symbol->type == NULL ||
      !TypeIsFunction(id->symbol->type) ||
      !id->symbol->type->info.function.is_constructor) {
    return;
  }
  size_t num_actual = node->children != NULL ? node->children->length : 0;
  if (id->symbol->type->info.function.prototype.length == num_actual &&
      !id->symbol->flags.is_template) {
    return;
  }
  Struct* owner = id->symbol->type->info.function.cxx_member_owner;
  StructMember* head =
      owner != NULL && owner->tag_name != NULL
          ? FindCXXMemberOverloadHead(owner, owner->tag_name)
          : NULL;
  StructMember* match = NULL;
  if (id->symbol->type->info.function.prototype.length != num_actual) {
    match = RankLoweredConstructorNonTemplateMatch(head, node);
  }
  if (match == NULL) {
    Symbol* best_inst =
        DeduceBestLoweredConstructorTemplate(head, node, num_actual);
    if (best_inst != NULL) {
      id->symbol = best_inst;
      ASTNodeSetInstantiatedCalleeType(node->left, best_inst->type);
    }
  } else {
    id->symbol = match->symbol;
    ASTNodeSetType(node->left, match->symbol->type);
  }
}

/* Contract: fills `candidates` via ordinary lookup and optional ADL; reports
 * whether inline/member/ADL sets were expanded beyond a single symbol. */
static void GatherOverloadedCallCandidates(IdentifierASTNode* id,
                                           VectorASTNode* node,
                                           Vector* candidates,
                                           size_t* ordinary_count,
                                           bool* gathered_inline_overloads,
                                           bool* gathered_member_overloads,
                                           bool* has_adl_candidates) {
  *gathered_inline_overloads = false;
  bool is_namespace_scope_function =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.cxx_member_owner == NULL &&
      !id->symbol->flags.is_block_scope;
  if (is_namespace_scope_function) {
    Namespace* lookup_ns =
        NamespaceParentForInlineTransparentLookup(id->symbol->namespace_);
    AddNamedFunctionCandidates(&id->symbol->name, lookup_ns, candidates);
    *gathered_inline_overloads = candidates->length > 1;
  } else {
    AddFunctionOverloadCandidates(candidates, id->symbol);
  }
  *ordinary_count = candidates->length;
  Symbol* ordinary = FollowUsingAliasForADL(id->symbol);
  bool ordinary_is_function = ordinary != NULL && ordinary->type != NULL &&
                              TypeIsFunction(ordinary->type);
  Vector* ordinary_prototype =
      ordinary_is_function ? &ordinary->type->info.function.prototype : NULL;
  Symbol* ordinary_first_formal =
      ordinary_prototype != NULL && ordinary_prototype->length > 0
          ? ordinary_prototype->value.p[0]
          : NULL;
  bool ordinary_is_member_call =
      ordinary_is_function &&
      ordinary->type->info.function.cxx_member_owner != NULL &&
      ordinary_first_formal != NULL &&
      ordinary_first_formal->name.value != NULL &&
      strcmp(ordinary_first_formal->name.value, "this") == 0;
  bool ordinary_suppresses_adl =
      !ordinary_is_function || ordinary_is_member_call ||
      (ordinary->flags.is_block_scope &&
       !ordinary->type->info.function.unknown_args);
  bool allow_adl = CompilerIsCXX() &&
                   (node->left->flags & kASTQualifiedName) == 0 &&
                   !ordinary_suppresses_adl &&
                   !FunctionNameSkipsADL(&id->symbol->name);
  if (allow_adl) {
    AddADLFunctionCandidates(&id->symbol->name, node->children, candidates);
  }
  *has_adl_candidates = candidates->length > *ordinary_count;
  *gathered_member_overloads = candidates->length > 1;
}

/* Contract: true when full overload resolution can be skipped (single
 * non-overloaded callee with no ADL expansion). */
static bool ShouldSkipOverloadedCallResolution(
    IdentifierASTNode* id, bool gathered_inline_overloads,
    bool gathered_member_overloads, bool has_adl_candidates) {
  bool ordinary_unknown =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.unknown_args;
  return !gathered_inline_overloads && !gathered_member_overloads &&
         !id->symbol->flags.is_overloaded && !has_adl_candidates &&
         !ordinary_unknown &&
         !(id->symbol->flags.is_template && id->symbol->type != NULL &&
           TypeIsFunction(id->symbol->type) &&
           id->symbol->type->info.function.template_origin == NULL);
}

static bool CallActualsAreConcrete(VectorASTNode* node) {
  for (size_t i = 0; node->children != NULL && i < node->children->length;
       i++) {
    ASTNode* actual = node->children->value.p[i];
    if (actual == NULL || actual->type == NULL ||
        CallActualIsPackExpansion(actual) ||
        (actual->flags & kASTDependentFunctorCall) != 0 ||
        TypeContainsTemplateParameter(actual->type) ||
        TypeContainsAuto(actual->type) || TypeIsUnknown(actual->type)) {
      return false;
    }
  }
  return true;
}

/* A lone function template called with concrete explicit template arguments
 * (`test<T, U>(0)` in a detection idiom) is not viable when substituting those
 * arguments into its signature fails; that must be diagnosed so the enclosing
 * SFINAE context sees the failure. */
static bool ExplicitSingleTemplateCallMustMatch(IdentifierASTNode* id,
                                                VectorASTNode* node,
                                                Vector* candidates) {
  return candidates->length == 1 && id->symbol->flags.is_template &&
         id->template_arguments != NULL &&
         !TemplateArgumentVectorContainsTemplateParameter(
             id->template_arguments) &&
         CallActualsAreConcrete(node);
}

/* Contract: binds an identifier callee to the best overload (lookup + ADL +
 * template instantiation) and checks deleted functions. */
void ResolveOverloadedFunctionCall(VectorASTNode* node) {
  if (node->left == NULL || node->left->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  if (TryReuseSelectedFunctionTemplateCallee(node, id)) {
    return;
  }
  ResolveLoweredConstructorOverload(node, id);
  if (id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.is_constructor) {
    return;
  }
  if (!CompilerIsCXX() && !id->symbol->flags.is_overloaded) {
    return;
  }
  Vector candidates;
  VectorInit(&candidates);
  size_t ordinary_count = 0;
  bool gathered_inline_overloads = false;
  bool gathered_member_overloads = false;
  bool has_adl_candidates = false;
  GatherOverloadedCallCandidates(id, node, &candidates, &ordinary_count,
                                 &gathered_inline_overloads,
                                 &gathered_member_overloads,
                                 &has_adl_candidates);
  if (ShouldSkipOverloadedCallResolution(id, gathered_inline_overloads,
                                         gathered_member_overloads,
                                         has_adl_candidates)) {
    VectorDestruct(&candidates);
    return;
  }
  bool ordinary_unknown =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.unknown_args;
  bool diagnose_overload_set =
      id->symbol->flags.is_overloaded || has_adl_candidates ||
      ordinary_unknown || gathered_inline_overloads ||
      gathered_member_overloads ||
      ExplicitSingleTemplateCallMustMatch(id, node, &candidates);
  Symbol* best = ResolveFunctionCandidateVector(
      &id->symbol->name, &candidates, node->children, id->template_arguments,
      /*diagnose_no_match=*/diagnose_overload_set,
      /*diagnose_ambiguous=*/diagnose_overload_set, (ASTNode*)node);
  VectorDestruct(&candidates);
  if (best == NULL) {
    return;
  }

  best = InstantiateSelectedFunctionTemplateCandidate(best);
  id->symbol = best;
  ASTNodeSetInstantiatedCalleeType(node->left, best->type);
  CheckDeletedFunctionUse(best, (ASTNode*)node);
}
/* Contract: late instantiation when a call retained a template specialization stub. */
void InstantiateResolvedFunctionTemplateCall(VectorASTNode* node) {
  if (node == NULL || node->left == NULL || node->left->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  if (id->symbol == NULL || id->symbol->type == NULL ||
      !TypeIsFunction(id->symbol->type) ||
      id->symbol->type->info.function.template_origin == NULL) {
    return;
  }
  Symbol* instantiated =
      id->symbol->type->template_arguments != NULL
          ? InstantiateSelectedFunctionTemplateCandidate(id->symbol)
          : TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
                &compiler->syntax,
                id->symbol->type->info.function.template_origin,
                /*explicit_args=*/NULL, node->children);
  if (instantiated != NULL) {
    id->symbol = instantiated;
    ASTNodeSetInstantiatedCalleeType(node->left, instantiated->type);
    CheckDeletedFunctionUse(instantiated, (ASTNode*)node);
  }
}
