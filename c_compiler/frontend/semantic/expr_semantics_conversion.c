//
//  expr_semantics_conversion.c
//  c_compiler
//
//  Implicit conversions, reference binding, and conversion ranking.
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

static int ReferenceBindingRank(ASTNode* actual, TypeRecord* reference_type);
static int OverloadBaseConversionRank(TypeRecord* actual, TypeRecord* target);
static int OverloadBaseConversionRankMaterialized(TypeRecord* actual, TypeRecord* target);
static int FuncAddrBaseRank(ASTNode* actual, TypeRecord* target);
static int ConvertingConstructorRank(StructMember* member, ASTNode* from, bool allow_explicit, bool allow_invented);
static bool TypeIsNamedClass(TypeRecord* type, const char* name);
static bool TypeIsStdVectorType(TypeRecord* type);
static TypeRecord* StdVectorElementType(TypeRecord* vector_type);
static ASTNode* CXXStdVectorMemberCall(ASTNode* vector, const char* member_name, SourceLocation location);

static int CXXBracedInitTargetRankScalar(BracedInitializerASTNode* braced,
                                         TypeRecord* target);
static int CXXBracedInitTargetRankIlConstructors(StructMember* ctor,
                                                 ASTNode* actual);
static void CXXBracedInitTargetRankOrdinaryConstructors(StructMember* ctor,
                                                        Vector* elements,
                                                        int* best);
static void CXXBracedInitTargetRankAggregateExpressionElements(Struct* str,
                                                               Vector* elements,
                                                               int* best);
static void CXXBracedInitTargetRankDesignatedAggregate(Struct* str,
                                                       BracedInitializerASTNode* braced,
                                                       int* best);
static void CXXBracedInitTargetRankAggregateMixedElements(Struct* str,
                                                          BracedInitializerASTNode* braced,
                                                          int* best);

static int OverloadConversionRankReference(ASTNode* actual,
                                           TypeRecord* formal_type,
                                           TypeRecord* target);
static int OverloadConversionRankValue(ASTNode* actual, TypeRecord* target);
static int OverloadConversionRankBracedToReference(ASTNode* actual,
                                                   TypeRecord* formal_type,
                                                   TypeRecord* target);
static int OverloadConversionRankBracedToValue(ASTNode* actual,
                                               TypeRecord* target);
static int OverloadConversionRankStandardValue(ASTNode* actual,
                                               TypeRecord* target,
                                               int base_rank);
static int OverloadConversionRankReferenceFallback(ASTNode* actual,
                                                   TypeRecord* formal_type,
                                                   TypeRecord* target,
                                                   int binding_rank);
static int OverloadConversionRankValueFallback(ASTNode* actual,
                                               TypeRecord* target);

// Contract: Read-only; uses value category and effective constness of `actual`.
bool ReferenceCanBind(ASTNode* actual, TypeRecord* reference_type) {
  if (!TypeIsReference(reference_type)) {
    return false;
  }
  if (TypeIsEffectivelyConst(actual->type) &&
      !TypeIsEffectivelyConst(reference_type->next)) {
    return false;
  }
  if (reference_type->declarator == kDeclRValueReference) {
    return !ASTNodeIsLValue(actual) || ASTNodeIsCXXBracedTemporary(actual);
  }
  if (ASTNodeIsLValue(actual)) {
    return true;
  }
  return TypeIsEffectivelyConst(reference_type->next);
}

// Contract: Read-only ranking helper for `ReferenceCanBind`; -1 when binding is
// ill-formed.
static int ReferenceBindingRank(ASTNode* actual, TypeRecord* reference_type) {
  if (!ReferenceCanBind(actual, reference_type)) {
    return -1;
  }
  bool target_const = TypeIsEffectivelyConst(reference_type->next);
  if (reference_type->declarator == kDeclRValueReference) {
    return target_const && !TypeIsConst(actual->type) ? 1 : 0;
  }
  if (ASTNodeIsCXXBracedTemporary(actual)) {
    // Still accepted for a non-const `T&`, but never preferred over `T&&` or
    // `const T&`.
    return target_const ? 2 : 3;
  }
  if (ASTNodeIsLValue(actual)) {
    return target_const ? 1 : 0;
  }
  return 2;
}

/* [class.access.base]: a member of `D` may convert `D*` to a private or
 * protected base.  TypeIsDerivedFrom is public-only, which is correct for
 * implicit conversions in general but rejects `return StorageT<I>::get()`
 * when `Storage` is a private base and `get` is overloaded (ranking uses
 * this helper; a single candidate later converts via TryConvertDerivedPointer). */
bool TypeIsDerivedFromInCurrentClass(TypeRecord* from, TypeRecord* to) {
  if (TypeIsDerivedFrom(from, to)) {
    return true;
  }
  if (from == NULL || to == NULL || !TypeIsStructOrUnion(from) ||
      !TypeIsStructOrUnion(to) || from->info.struct_info == NULL ||
      to->info.struct_info == NULL) {
    return false;
  }
  Struct* current = compiler->current_class_access_context;
  if (current == NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    current = compiler->current_function->info.function.cxx_member_owner;
  }
  if (current == NULL) {
    return false;
  }
  Struct* from_struct = from->info.struct_info;
  if (!CXXSameAccessClass(current, from_struct)) {
    bool nested = false;
    for (Struct* enclosing = current->lexical_parent; enclosing != NULL;
         enclosing = enclosing->lexical_parent) {
      if (CXXSameAccessClass(enclosing, from_struct)) {
        nested = true;
        break;
      }
    }
    // `friend const_iterator` inside `iterator` lets the other specialization
    // bind a private base (`btree_iterator_generation_info(other)`).
    bool friend_of_derived = false;
    if (!nested) {
      for (size_t i = 0; i < from_struct->friend_classes.length; i++) {
        if (CXXSameAccessClass(from_struct->friend_classes.value.p[i],
                               current)) {
          friend_of_derived = true;
          break;
        }
      }
    }
    if (!nested && !friend_of_derived) {
      return false;
    }
  }
  return TypeBaseOffset(from, to, /*public_only=*/false, NULL);
}

// Contract: Materializes class types then ranks; may delete temporary materializations.
static int OverloadBaseConversionRank(TypeRecord* actual, TypeRecord* target) {
  TypeRecord* actual_m = MaterializeConversionClass(actual);
  TypeRecord* target_m = MaterializeConversionClass(target);
  bool drop_actual = actual_m != actual;
  bool drop_target = target_m != target;
  actual = actual_m;
  target = target_m;
  int rank = OverloadBaseConversionRankMaterialized(actual, target);
  if (drop_actual) {
    TypeRecordDelete(actual_m);
  }
  if (drop_target) {
    TypeRecordDelete(target_m);
  }
  return rank;
}

// Contract: Pure standard conversion rank on materialized types; read-only.
static int OverloadBaseConversionRankMaterialized(TypeRecord* actual,
                                                  TypeRecord* target) {
  // Array-to-pointer decay produces a pointer to the element.  A const or
  // volatile array cannot convert to a pointer that drops that qualifier,
  // just as `const T*` cannot convert to `void*`.
  if (actual != NULL && TypeIsPointer(target) && target->next != NULL &&
      (TypeIsPointer(actual) || actual->declarator == kDeclArray) &&
      actual->next != NULL) {
    Qualifiers source_quals =
        GetQualifiers(actual->next) & (kQualConst | kQualVolatile);
    if (actual->declarator == kDeclArray) {
      source_quals |= GetQualifiers(actual) & (kQualConst | kQualVolatile);
    }
    Qualifiers discarded =
        source_quals &
        ~(GetQualifiers(target->next) & (kQualConst | kQualVolatile));
    if (discarded != 0) {
      return -1;
    }
  }
  if (TypeEqual(actual, target) || TypeEqualIgnoringQualifiers(actual, target)) {
    return 0;
  }
  // [conv.ptr]: a prvalue of type std::nullptr_t converts to any pointer or
  // pointer-to-member type.
  if (TypeIsNullPointer(actual) &&
      (TypeIsPointer(target) || TypeIsMemberPointer(target))) {
    return 2;
  }
  if (TypeIsStructOrUnion(actual) || TypeIsStructOrUnion(target)) {
    // Two distinct class types convert only via a derived-to-base relationship.
    // TypeEqual above handles separately materialized records for the same
    // template specialization by comparing their origins and arguments.
    if (TypeIsStructOrUnion(actual) && TypeIsStructOrUnion(target)) {
      if (TypeIsDerivedFromInCurrentClass(actual, target)) {
        return 2;
      }
    }
    return -1;
  }
  // A same-width signedness change (`unsigned char` to `char`, `int` to
  // `unsigned int`) is an integral conversion, not a promotion.  Ranking it
  // with promotions made `unsigned char` ambiguous between `Arg(char)` and
  // `Arg(int)`, so `absl::Substitute` rejected a descriptor type field.
  if (TypeEqualIgnoringSign(actual, target)) {
    return 2;
  }
  if (TypeIsEnum(actual) && TypeIsEnum(target)) {
    return actual->info.enum_info == target->info.enum_info ? 0 : -1;
  }
  if (TypeIsIntegral(actual) && TypeIsEnum(target)) {
    // C++ does not implicitly convert an integer to an enumeration.  Allowing
    // it makes `Edge(uint8_t)` ambiguous between `Edge(size_t)` and
    // `Edge(EdgeType)`.  C still treats enumerations as integers.
    return CompilerIsCXX() ? -1 : 2;
  }
  if (TypeIsEnum(actual) && TypeIsIntegral(target)) {
    // Integral promotion of an unscoped enumeration to int ([conv.prom])
    // outranks a conversion to a wider integer.  Scoped enumerations do not
    // promote.
    if (!TypeIsEnum(target) && CompilerIsCXX() &&
        actual->info.enum_info != NULL && !actual->info.enum_info->is_scoped &&
        TypeIsInt(target) && !TypeIsUnsigned(target) && !TypeIsLong(target) &&
        !TypeIsLongLong(target) && !TypeIsShort(target) &&
        !TypeIsCharFamily(target)) {
      return 1;
    }
    return 2;
  }
  if (TypeIsInt(target) && !TypeIsUnsigned(target) &&
      (TypeIsCharFamily(actual) || TypeIsShort(actual) ||
       TypeIsBool(actual))) {
    return 1;
  }
  if (TypeIsIntegral(actual) && TypeIsIntegral(target)) {
    return 2;
  }
  // Standard arithmetic conversion sequences.  In particular, a converted
  // prvalue can bind to a const reference parameter after this conversion
  // (for example, float -> const double&).
  if (TypeIsFloat(actual) && TypeIsDouble(target)) {
    return 1;
  }
  if ((TypeIsFloatingPoint(actual) && TypeIsFloatingPoint(target)) ||
      (TypeIsIntegral(actual) && TypeIsFloatingPoint(target)) ||
      (TypeIsFloatingPoint(actual) && TypeIsIntegral(target))) {
    return 2;
  }
  if (TypeIsPointerOrArray(actual) && TypeIsPointerOrArray(target)) {
    if (TypeChar8IdentityDiffers(actual, target)) {
      return -1;
    }
    if (CompilerIsCXX() && TypeIsVoidPointer(actual) &&
        TypeIsPointer(target) && !TypeIsVoidPointer(target)) {
      return -1;
    }
    if (CompilerIsCXX() && actual->next != NULL && target->next != NULL) {
      // A parameter typed as `std::ostream*` may still be the primary
      // `basic_ostream` plus concrete arguments (the typedef was formed while
      // the class template was only forward-declared).  Materialize both
      // pointees so a `stringstream*` argument converts to that base.
      TypeRecord* actual_pointee = TypeMaterializeClassTemplateSpecialization(
          &compiler->syntax, actual->next);
      TypeRecord* target_pointee = TypeMaterializeClassTemplateSpecialization(
          &compiler->syntax, target->next);
      bool derived = actual_pointee != NULL && target_pointee != NULL &&
                     TypeIsStructOrUnion(actual_pointee) &&
                     TypeIsStructOrUnion(target_pointee) &&
                     TypeIsDerivedFromInCurrentClass(actual_pointee,
                                                     target_pointee);
      if (actual_pointee != actual->next) {
        TypeRecordDelete(actual_pointee);
      }
      if (target_pointee != target->next) {
        TypeRecordDelete(target_pointee);
      }
      if (derived) {
        return 2;
      }
    }
    if (CompilerIsCXX() && actual->next != NULL && target->next != NULL &&
        !TypeIsVoid(actual->next) && !TypeIsVoid(target->next) &&
        !TypeEqualIgnoringQualifiers(actual->next, target->next) &&
        !(TypeIsStructOrUnion(actual->next) &&
          TypeIsStructOrUnion(target->next) &&
          TypeIsDerivedFromInCurrentClass(actual->next, target->next))) {
      return -1;
    }
    if (TypeIsVoidPointer(actual) || TypeIsVoidPointer(target)) {
      return 2;
    }
    if (TypeAssignmentCompatible(actual, target)) {
      return 1;
    }
  }
  // Function-to-pointer conversion ([conv.func]): a function lvalue argument
  // binds to a matching function-pointer parameter.  This is an lvalue
  // transformation (exact-match rank), so a function argument such as a free
  // function passed to a `Pred`-deduced `R(*)(Args...)` parameter is viable.
  if (TypeIsFunction(actual) && TypeIsPointer(target) && target->next != NULL &&
      TypeIsFunction(target->next) &&
      (TypeEqual(actual, target->next) ||
       TypeEqualIgnoringQualifiers(actual, target->next))) {
    return 0;
  }
  return -1;
}

// Overload-resolution rank for list-initializing a *non-`initializer_list`*
// target (a class or scalar) from the braced-init-list `actual`.  Returns a
// non-negative base rank if the braces can initialize `target`, or -1.  This
// mirrors the list-initialization that LowerCXXBracedInitToTarget performs at
// call time, so that an overloaded function taking a class/scalar (by value or
// reference) is viable for a braced-init argument -- e.g. `insert({k, v})`
// where the parameter is `pair<const K,V>` (or a reference to it).  Without this
// the scoring path treats the braced list as `<unknown>` and rejects every
// candidate whenever more than one overload exists.
// Contract: Recursion guard for initializer-list ctor scoring on the same braced
// list; only CXXBracedInitTargetRankIlConstructors increments it.
static int cxx_braced_init_target_rank_depth = 0;

// Contract: Scalar list-init ({}) or single expression element; analyzes the
// element and ranks with OverloadBaseConversionRank only.
static int CXXBracedInitTargetRankScalar(BracedInitializerASTNode* braced,
                                         TypeRecord* target) {
  if (braced->initializers->length == 0) {
    return 0;
  }
  if (braced->initializers->length != 1) {
    return -1;
  }
  ASTNode* init = braced->initializers->value.p[0];
  if (init == NULL || init->op != AST_OP(expr_init)) {
    return -1;
  }
  ExpressionInitializerASTNode* ei = (ExpressionInitializerASTNode*)init;
  ei->expr = AnalyzeExpression(ei->expr);
  int r = OverloadBaseConversionRank(ei->expr->type, target);
  return r < 0 ? -1 : r;
}

// Ranks the braced elements against the initializer_list parameter of a
// constructor that CXXConstructorIsInitializerListConstructor has recognized.
static int CXXInitializerListConstructorElementRank(TypeRecord* func,
                                                    ASTNode* actual) {
  if (func == NULL || !TypeIsFunction(func)) {
    return -1;
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal == NULL || StringEqual(&formal->name, "this") ||
        StringEqual(&formal->name, "__complete_object")) {
      continue;
    }
    TypeRecord* target =
        TypeIsReference(formal->type) ? formal->type->next : formal->type;
    return CXXInitializerListBracedInitRank(actual, target);
  }
  return -1;
}

// Contract: [over.match.list] initializer-list constructors; speculative template
// deduction with diagnostics suppressed; bumps cxx_braced_init_target_rank_depth.
static int CXXBracedInitTargetRankIlConstructors(StructMember* ctor,
                                                 ASTNode* actual) {
  if (ctor == NULL) {
    return -1;
  }
  Vector one_actual;
  VectorInit(&one_actual);
  VectorAppend(&one_actual, actual);
  int best = -1;
  cxx_braced_init_target_rank_depth++;
  for (StructMember* c = ctor; c != NULL; c = c->overload_next) {
    if (!c->is_member_function || c->symbol == NULL ||
        c->symbol->type == NULL || !TypeIsFunction(c->symbol->type) ||
        !c->symbol->type->info.function.is_constructor ||
        c->symbol->type->info.function.is_deleted ||
        !CXXConstructorIsInitializerListConstructor(
            &c->symbol->type->info.function)) {
      continue;
    }
    int s = -1;
    if (c->symbol->flags.is_template) {
      DiagnosticSuppressBegin();
      Symbol* temporary = TypeCreateFunctionTemplateCandidate(
          &compiler->syntax, c->symbol, NULL, &one_actual,
          /*first_formal_arg=*/1);
      DiagnosticSuppressEnd();
      if (temporary == NULL || temporary->type == NULL ||
          !TypeIsFunction(temporary->type)) {
        if (temporary != NULL) {
          SymbolDelete(temporary);
        }
        continue;
      }
      s = CXXInitializerListConstructorElementRank(temporary->type, actual);
      SymbolDelete(temporary);
    } else {
      s = CXXInitializerListConstructorElementRank(c->symbol->type, actual);
    }
    if (s >= 0 && (best < 0 || s < best)) {
      best = s;
    }
  }
  cxx_braced_init_target_rank_depth--;
  VectorDestruct(&one_actual);
  return best;
}

// True when a non-deleted, non-template constructor in `ctor` is callable
// without explicit arguments.
static bool CXXConstructorSetHasDefaultConstructor(StructMember* ctor,
                                                   Struct* owner) {
  size_t expected = StructHasVirtualBases(owner) ? 2 : 1;
  for (StructMember* c = ctor; c != NULL; c = c->overload_next) {
    if (!c->is_member_function || c->symbol == NULL ||
        c->symbol->type == NULL || !TypeIsFunction(c->symbol->type) ||
        c->symbol->flags.is_template) {
      continue;
    }
    FunctionInfo* info = &c->symbol->type->info.function;
    if (!info->is_constructor || info->is_deleted ||
        info->prototype.length < expected) {
      continue;
    }
    bool callable_without_arguments = true;
    for (size_t j = expected; j < info->prototype.length; j++) {
      Symbol* parameter = info->prototype.value.p[j];
      if (parameter == NULL ||
          SymbolExtraOf(parameter)->default_argument == NULL) {
        callable_without_arguments = false;
        break;
      }
    }
    if (callable_without_arguments) {
      return true;
    }
  }
  return false;
}

// Reports whether [over.match.list]'s first phase has a viable
// initializer-list constructor.  Lowering uses this to decide whether the
// braces stay one argument or are unpacked for ordinary constructors.
bool CXXBracedInitSelectsInitializerListConstructor(ASTNode* actual,
                                                    TypeRecord* target) {
  if (actual == NULL || actual->op != AST_OP(braced_init) || target == NULL ||
      !TypeIsStructOrUnion(target) || target->info.struct_info == NULL ||
      target->info.struct_info->tag_name == NULL) {
    return false;
  }
  StructMember* ctor = FindStructMember(target->info.struct_info,
                                        target->info.struct_info->tag_name);
  // An empty list value-initializes a class with a default constructor
  // ([dcl.init.list]/3.4); initializer-list constructors are not considered.
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if ((braced->initializers == NULL || braced->initializers->length == 0) &&
      CXXConstructorSetHasDefaultConstructor(ctor, target->info.struct_info)) {
    return false;
  }
  return CXXBracedInitTargetRankIlConstructors(ctor, actual) >= 0;
}

// Contract: Non-initializer-list constructors for expression-only element lists;
// updates `*best` with the minimum FunctionCallScore when viable.  Member
// template constructors are deduced against the elements; skipping them made
// `vector<Sub>{{"k", value}}` non-viable when `Sub`'s only constructor is
// `template<class V> Sub(string, V&&)`.
static void CXXBracedInitTargetRankOrdinaryConstructors(StructMember* ctor,
                                                          Vector* elements,
                                                          int* best) {
  VectorASTNode call = {0};
  call.children = elements;
  for (StructMember* c = ctor; c != NULL; c = c->overload_next) {
    if (!c->is_member_function || c->symbol == NULL ||
        c->symbol->type == NULL || !TypeIsFunction(c->symbol->type) ||
        !c->symbol->type->info.function.is_constructor ||
        c->symbol->type->info.function.is_deleted ||
        CXXConstructorIsInitializerListConstructor(
            &c->symbol->type->info.function)) {
      continue;
    }
    Symbol* scored = c->symbol;
    Symbol* temporary = NULL;
    if (c->symbol->flags.is_template) {
      DiagnosticSuppressBegin();
      temporary = TypeCreateFunctionTemplateCandidate(
          &compiler->syntax, c->symbol, NULL, elements,
          /*first_formal_arg=*/1);
      DiagnosticSuppressEnd();
      if (temporary == NULL || temporary->type == NULL ||
          !TypeIsFunction(temporary->type)) {
        if (temporary != NULL) {
          SymbolDelete(temporary);
        }
        continue;
      }
      scored = temporary;
    }
    int s = FunctionCallScore(scored->type, &call, /*first_formal_arg=*/1);
    if (temporary != NULL) {
      SymbolDelete(temporary);
    }
    if (s >= 0 && (*best < 0 || s < *best)) {
      *best = s;
    }
  }
}

// Contract: Positional aggregate init when every brace element is a analyzed
// expression; sets `*best` to 0 when each maps to a data member via
// OverloadConversionRank.
static bool CXXBracedInitAggregateObjectMember(StructMember* member) {
  return member != NULL && member->symbol != NULL && !member->is_static &&
         !member->is_member_function &&
         !StorageIs(member->symbol->storage, STO(typedef));
}

static void CXXBracedInitTargetRankAggregateExpressionElements(Struct* str,
                                                               Vector* elements,
                                                               int* best) {
  if (!str->is_aggregate) {
    return;
  }
  size_t member_index = 0;
  bool ok = true;
  for (size_t i = 0; i < elements->length; i++) {
    while (member_index < str->members.length) {
      StructMember* m = str->members.value.p[member_index];
      if (CXXBracedInitAggregateObjectMember(m)) {
        break;
      }
      member_index++;
    }
    if (member_index >= str->members.length) {
      ok = false;
      break;
    }
    StructMember* m = str->members.value.p[member_index++];
    ASTNode* element = elements->value.p[i];
    if (element == NULL || m->symbol == NULL ||
        OverloadConversionRank(element, m->symbol->type) < 0) {
      ok = false;
      break;
    }
  }
  if (ok) {
    *best = 0;
  }
}

// Contract: Designated-initializer aggregate (`{.a = 1}`); analyzes init
// expressions and ranks with OverloadConversionRank; sets `*best` to 0 when ok.
static void CXXBracedInitTargetRankDesignatedAggregate(Struct* str,
                                                       BracedInitializerASTNode* braced,
                                                       int* best) {
  if (!str->is_aggregate || braced->initializers == NULL) {
    return;
  }
  bool ok = braced->initializers->length > 0;
  for (size_t i = 0; ok && i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL || init->op != AST_OP(designated_init)) {
      ok = false;
      break;
    }
    DesignatedInitializerASTNode* designated =
        (DesignatedInitializerASTNode*)init;
    if (designated->designators == NULL ||
        designated->designators->length != 1) {
      ok = false;
      break;
    }
    Designator* designator = designated->designators->value.p[0];
    if (designator == NULL ||
        designator->designator_type != kDesignatorStruct) {
      ok = false;
      break;
    }
    StructMember* member = designator->is_resolved_member
                               ? designator->value.struct_member
                               : NULL;
    if (member == NULL && designator->value.struct_member_name != NULL) {
      member = FindStructMember(str, designator->value.struct_member_name);
    }
    if (member == NULL || member->symbol == NULL ||
        member->symbol->type == NULL) {
      ok = false;
      break;
    }
    ASTNode* expr = designated->init;
    if (expr != NULL && expr->op == AST_OP(expr_init)) {
      expr = ((ExpressionInitializerASTNode*)expr)->expr;
    }
    expr = AnalyzeExpression(expr);
    if (expr == NULL ||
        OverloadConversionRank(expr, member->symbol->type) < 0) {
      ok = false;
      break;
    }
  }
  if (ok) {
    *best = 0;
  }
}

// Contract: Positional aggregate with nested braces or expr_init wrappers;
// recurses CXXBracedInitTargetRank for braced elements; sets `*best` to 0 when ok.
static void CXXBracedInitTargetRankAggregateMixedElements(Struct* str,
                                                          BracedInitializerASTNode* braced,
                                                          int* best) {
  if (!str->is_aggregate || braced->initializers == NULL ||
      braced->initializers->length == 0) {
    return;
  }
  bool positional = true;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init != NULL && init->op == AST_OP(designated_init)) {
      positional = false;
      break;
    }
  }
  if (!positional) {
    return;
  }
  size_t member_index = 0;
  bool ok = true;
  for (size_t i = 0; ok && i < braced->initializers->length; i++) {
    while (member_index < str->members.length) {
      StructMember* m = str->members.value.p[member_index];
      if (CXXBracedInitAggregateObjectMember(m)) {
        break;
      }
      member_index++;
    }
    if (member_index >= str->members.length) {
      ok = false;
      break;
    }
    StructMember* m = str->members.value.p[member_index++];
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL || m->symbol == NULL || m->symbol->type == NULL) {
      ok = false;
      break;
    }
    int rank = -1;
    if (init->op == AST_OP(braced_init)) {
      rank = CXXBracedInitTargetRank(init, m->symbol->type);
    } else if (init->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* ei = (ExpressionInitializerASTNode*)init;
      ei->expr = AnalyzeExpression(ei->expr);
      if (ei->expr != NULL && ei->expr->op == AST_OP(braced_init)) {
        rank = CXXBracedInitTargetRank(ei->expr, m->symbol->type);
      } else {
        rank = OverloadConversionRank(ei->expr, m->symbol->type);
      }
    }
    if (rank < 0) {
      ok = false;
    }
  }
  if (ok) {
    *best = 0;
  }
}

// Contract: May analyze braced-init elements and run speculative template ctor
// scoring; mutates child expressions under `actual`.
int CXXBracedInitTargetRank(ASTNode* actual, TypeRecord* target) {
  if (actual == NULL || actual->op != AST_OP(braced_init) || target == NULL) {
    return -1;
  }
  // Ranking an initializer-list constructor re-enters conversion ranking on
  // the same braced list.  A constructor whose parameter is not recognized as
  // `initializer_list` (or whose element initialization constructs the same
  // class) would recurse without bound.
  if (cxx_braced_init_target_rank_depth > 4) {
    return -1;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  if (TypeIsScalar(target)) {
    return CXXBracedInitTargetRankScalar(braced, target);
  }
  if (!TypeIsStructOrUnion(target) || target->info.struct_info == NULL) {
    return -1;
  }
  // `std::initializer_list<E>` is ranked by CXXInitializerListBracedInitRank.
  // Falling through here would treat its copy constructor (first parameter is
  // `const initializer_list<E>&`) as an initializer-list constructor and
  // recurse through FunctionCallScore forever.
  if (TypeIsCXXInitializerList(target)) {
    return -1;
  }
  Struct* str = target->info.struct_info;
  // Analyze expression elements once and retain nested braced elements as
  // constructor arguments.  Designated initializers are aggregate-only.
  Vector elements;
  VectorInit(&elements);
  bool simple_elements = true;
  bool constructor_elements = true;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init == NULL) {
      simple_elements = false;
      constructor_elements = false;
      break;
    }
    if (init->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* ei = (ExpressionInitializerASTNode*)init;
      ei->expr = AnalyzeExpression(ei->expr);
      VectorAppend(&elements, ei->expr);
    } else {
      simple_elements = false;
      if (init->op == AST_OP(braced_init)) {
        VectorAppend(&elements, init);
      } else {
        constructor_elements = false;
        break;
      }
    }
  }
  int best = -1;
  StructMember* ctor =
      str->tag_name != NULL ? FindStructMember(str, str->tag_name) : NULL;
  // [over.match.list] first tries initializer-list constructors, which take
  // the entire braced-init-list as one `std::initializer_list<E>` argument.
  // Those constructors are often member templates (Abseil `Span<const T>`'s
  // `EnableIfValueIsConst` overload); skipping templates here rejected every
  // candidate for `Streamable(fmt, {FormatArgImpl(args)...})`.
  if (ctor != NULL) {
    int il_best = CXXBracedInitTargetRankIlConstructors(ctor, actual);
    if (il_best >= 0) {
      best = il_best;
    }
  }
  if (best < 0 && constructor_elements) {
    CXXBracedInitTargetRankOrdinaryConstructors(ctor, &elements, &best);
  }
  if (best < 0 && simple_elements) {
    // Aggregate initialization: element-wise conversion to each data member.
    CXXBracedInitTargetRankAggregateExpressionElements(str, &elements, &best);
  }
  // Designated-initializer aggregate initialization (`f({.a = 1, .b = 2})`).
  // Positional ranking above only accepts expression elements.
  if (best < 0 && !simple_elements) {
    CXXBracedInitTargetRankDesignatedAggregate(str, braced, &best);
  }
  // Positional aggregate initialization with a nested brace (`{false, n, {}}`).
  // The expression-only path above rejects the list as soon as one element is
  // itself a braced-init, so the call argument stays `<unknown>`.
  if (best < 0 && !simple_elements) {
    CXXBracedInitTargetRankAggregateMixedElements(str, braced, &best);
  }
  VectorDestruct(&elements);
  return best;
}

// [over.over] viability during overload resolution: `actual` names a function
// template or overload set and `target` is a pointer-to-function.  Returns a
// base rank (0, an exact function-to-pointer conversion) when a unique matching
// function can be formed, or -1.  Deduction/instantiation diagnostics are
// suppressed since this is a speculative probe over every candidate parameter.
// Contract: Speculative function-address resolution rank; no AST mutation.
static int FuncAddrBaseRank(ASTNode* actual, TypeRecord* target) {
  if (actual != NULL && actual->op == AST_OP(address)) {
    actual = ((UnaryASTNode*)actual)->sub;
  }
  if (!CompilerIsCXX() || actual == NULL || actual->op != AST_OP(identifier)) {
    return -1;
  }
  if (!TypeIsPointer(target) || target->next == NULL ||
      !TypeIsFunction(target->next)) {
    return -1;
  }
  if (actual->type == NULL || !TypeIsFunction(actual->type)) {
    return -1;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)actual;
  if (id->symbol == NULL) {
    return -1;
  }
  DiagnosticSuppressBegin();
  Symbol* resolved = CXXResolveFunctionAddressForTargetType(
      id->symbol, id->template_arguments, target->next);
  DiagnosticSuppressEnd();
  return resolved != NULL ? 0 : -1;
}

/* True when `type` names a member of a still-dependent type (`T::slot_type`,
 * `remove_reference<T>::type`). */
bool TypeHasDependentMemberName(TypeRecord* type) {
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (TypeDependentOf(current)->dependent_member_name != NULL) {
      return true;
    }
    if (current->template_arguments != NULL) {
      for (size_t i = 0; i < current->template_arguments->length; i++) {
        TemplateArgument* arg = current->template_arguments->value.p[i];
        if (arg != NULL && arg->type != NULL &&
            TypeHasDependentMemberName(arg->type)) {
          return true;
        }
      }
    }
  }
  return false;
}

/* The dependent member is a member of the enclosing class template (index below
 * `template_parameter_base`), not of this function template's own parameters.
 * `Policy::slot_type` qualifies; `remove_reference<T>::type` on `forward` does
 * not. */
bool TypeDependsOnEnclosingTemplateParameter(TypeRecord* type, int base) {
  if (base <= 0) {
    return false;
  }
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (TypeDependentOf(current)->dependent_member_name != NULL &&
        current->template_parameter_index >= 0 &&
        current->template_parameter_index < base) {
      return true;
    }
    if (current->template_arguments != NULL) {
      for (size_t i = 0; i < current->template_arguments->length; i++) {
        TemplateArgument* arg = current->template_arguments->value.p[i];
        if (arg != NULL && arg->type != NULL &&
            TypeDependsOnEnclosingTemplateParameter(arg->type, base)) {
          return true;
        }
      }
    }
  }
  return false;
}

// Contract: Braced-init to reference parameter; list-init rank then reference
// binding tier (rvalue ref vs const lvalue ref); -1 when non-const lvalue ref.
static int OverloadConversionRankBracedToReference(ASTNode* actual,
                                                   TypeRecord* formal_type,
                                                   TypeRecord* target) {
  int il_rank = CXXInitializerListBracedInitRank(actual, target);
  if (il_rank >= 0) {
    return il_rank;
  }
  int base = CXXBracedInitTargetRank(actual, target);
  if (base < 0) {
    return -1;
  }
  // A braced-init-list yields a prvalue temporary: it binds to an rvalue
  // reference or a const lvalue reference, but never a non-const lvalue
  // reference.  Prefer the rvalue reference so, e.g., `insert(value_type&&)`
  // wins over `insert(const value_type&)` for `insert({k, v})`.
  if (formal_type->declarator == kDeclRValueReference) {
    return base * 10;
  }
  if (TypeIsConst(target)) {
    return base * 10 + 1;
  }
  return -1;
}

// Contract: Braced-init to non-reference parameter; initializer_list rank first,
// then class/scalar list-init base rank scaled by 10.
static int OverloadConversionRankBracedToValue(ASTNode* actual,
                                               TypeRecord* target) {
  int il_rank = CXXInitializerListBracedInitRank(actual, target);
  if (il_rank >= 0) {
    return il_rank;
  }
  int base = CXXBracedInitTargetRank(actual, target);
  return base < 0 ? -1 : base * 10;
}

// Contract: After a viable standard conversion to a value parameter, applies
// [over.ics.rank] pointer qualification sub-tiers on top of `base_rank`.
static int OverloadConversionRankStandardValue(ASTNode* actual,
                                               TypeRecord* target,
                                               int base_rank) {
  // [over.ics.rank]: when two standard conversion sequences are otherwise
  // indistinguishable, the one that is not a qualification conversion is
  // better.  Binding a `T*` argument to a `const T*` parameter differs from an
  // exact `T*` parameter only by adding pointee cv-qualifiers; give it a small
  // penalty so the exact `T*` overload wins instead of being ambiguous (e.g.
  // `get_if<0>(variant<...>*)` between the `variant<Types...>*` and
  // `const variant<Types...>*` overloads).  The same tie-break applies when
  // both sequences also perform a derived-to-base or object-to-void pointer
  // conversion, including after an array-to-pointer conversion: `unsigned
  // char[N]` to `void*` beats `const void*`.  This applies to either const or
  // volatile qualification; otherwise the standard volatile overloads of the
  // atomic free functions are ambiguous for non-volatile objects.  Each added
  // qualifier contributes one sub-tier so adding const is better than adding
  // both const and volatile, without outweighing a genuine conversion in
  // another argument.
  TypeRecord* source_pointee = NULL;
  Qualifiers source_quals = 0;
  if (actual->type != NULL && TypeIsPointer(target) && target->next != NULL) {
    if (TypeIsPointer(actual->type) ||
        actual->type->declarator == kDeclArray) {
      source_pointee = actual->type->next;
      if (source_pointee != NULL) {
        source_quals =
            GetQualifiers(source_pointee) & (kQualConst | kQualVolatile);
      }
      if (actual->type->declarator == kDeclArray) {
        source_quals |=
            GetQualifiers(actual->type) & (kQualConst | kQualVolatile);
      }
    }
  }
  Qualifiers added_pointer_qualifiers =
      source_pointee != NULL
          ? ((GetQualifiers(target->next) & (kQualConst | kQualVolatile)) &
             ~source_quals)
          : 0;
  bool pointer_qualification_conversion = source_pointee != NULL &&
                                          added_pointer_qualifiers != 0;
  int qualification_penalty =
      pointer_qualification_conversion
          ? ((added_pointer_qualifiers & kQualConst) != 0) +
                ((added_pointer_qualifiers & kQualVolatile) != 0)
          : 0;
  return base_rank * 10 + 5 + qualification_penalty;
}

// A conversion function of a class argument reaching a non-class parameter:
// `operator int&()` binds `int&` or `const int&`, and a scalar result converts
// to a value parameter or initializes the temporary a `const long&` binds.
// Ranked after any standard conversion, by the trailing standard conversion.
static int OverloadConversionFunctionRank(ASTNode* actual,
                                          TypeRecord* parameter_type) {
  TypeRecord* target = TypeIsReference(parameter_type) ? parameter_type->next
                                                       : parameter_type;
  if (!CompilerIsCXX() || g_suppress_user_defined_conversion_rank ||
      actual == NULL || actual->type == NULL ||
      !TypeIsStructOrUnion(actual->type) || target == NULL ||
      TypeIsStructOrUnion(target)) {
    return -1;
  }
  int rank = SemanticConversionFunctionRank(actual, parameter_type);
  return rank < 0 ? -1 : 100 + rank;
}

// Contract: Reference parameter after standard conversion failed; function
// address and lambda exact-match paths, then UDC to temporary (100/101).
// Respects g_suppress_user_defined_conversion_rank.
static int OverloadConversionRankReferenceFallback(ASTNode* actual,
                                                   TypeRecord* formal_type,
                                                   TypeRecord* target,
                                                   int binding_rank) {
  if (FuncAddrBaseRank(actual, target) >= 0) {
    return binding_rank;
  }
  if (TypeIsPointer(target) && target->next != NULL &&
      TypeIsFunction(target->next)) {
    ASTNode* lambda_expr = CXXLambdaConversionExpression(actual);
    if (lambda_expr != NULL &&
        (lambda_expr->flags & kASTLambdaExpression) != 0 &&
        LambdaCallMatchesFunctionPointer(lambda_expr, target)) {
      return binding_rank;
    }
  }
  // A user-defined conversion produces a temporary. It can satisfy an rvalue
  // reference or a const lvalue reference, but never a non-const lvalue
  // reference.  Both directions are eligible: a converting constructor of the
  // target class, or a conversion operator on the source class yielding the
  // target (e.g. binding a `std::string` to `const std::string_view&` via
  // `std::string::operator string_view`).
  if (CompilerIsCXX() && !g_suppress_user_defined_conversion_rank &&
      (formal_type->declarator == kDeclRValueReference ||
       TypeIsConst(target)) &&
      TypeIsStructOrUnion(target) &&
      (FindConvertingConstructorCandidate(target, actual,
                                          /*allow_explicit=*/false,
                                          /*allow_same_class=*/false) != NULL ||
       ClassHasConversionOperatorTo(actual, target))) {
    // The conversion produces a temporary. Binding that prvalue to an rvalue
    // reference is a better second standard conversion than binding it to a
    // const lvalue reference, so `operator=(vector&&)` beats
    // `operator=(const vector&)`.
    if (formal_type->declarator == kDeclRValueReference) {
      return 100;
    }
    return 101;
  }
  return OverloadConversionFunctionRank(actual, formal_type);
}

// Contract: Value parameter after standard conversion failed; function/lambda
// (rank 5), null pointer constant (30), then UDC (100). Respects UDC suppression.
static int OverloadConversionRankValueFallback(ASTNode* actual,
                                               TypeRecord* target) {
  if (FuncAddrBaseRank(actual, target) >= 0) {
    // Exact function-to-pointer conversion (an lvalue transformation).
    return 5;
  }
  if (TypeIsPointer(target) && target->next != NULL &&
      TypeIsFunction(target->next)) {
    ASTNode* lambda_expr = CXXLambdaConversionExpression(actual);
    if (lambda_expr != NULL &&
        (lambda_expr->flags & kASTLambdaExpression) != 0 &&
        LambdaCallMatchesFunctionPointer(lambda_expr, target)) {
      return 5;
    }
  }
  // A null pointer constant is an integer literal with value zero
  // ([conv.ptr]).  An enumerator is not an integer literal, even when its
  // value is zero and it has been folded to a constant node.
  if (IsNullPointerConstant(actual) &&
      (TypeIsPointer(target) || TypeIsMemberPointer(target)) &&
      !(CompilerIsCXX() && actual->type != NULL && TypeIsEnum(actual->type))) {
    // Worse than an integral conversion (rank 25).  A zero integer that is
    // also a character or a promoted literal then prefers `find(CharT)` over
    // `find(const CharT*)`.
    return 30;
  }
  // As a last resort consider a user-defined conversion through a converting
  // constructor of a class target or a conversion operator on the source.
  // This ranks worse than any standard conversion sequence above, matching
  // the standard's ordering.
  if (CompilerIsCXX() && !g_suppress_user_defined_conversion_rank &&
      ((TypeIsStructOrUnion(target) &&
        FindConvertingConstructorCandidate(target, actual,
                                           /*allow_explicit=*/false,
                                           /*allow_same_class=*/false) != NULL) ||
       ClassHasConversionOperatorTo(actual, target))) {
    return 100;
  }
  return OverloadConversionFunctionRank(actual, target);
}

// Contract: Reference formal parameter dispatch; may mutate braced-init children.
static int OverloadConversionRankReference(ASTNode* actual,
                                           TypeRecord* formal_type,
                                           TypeRecord* target) {
  if (actual != NULL && actual->op == AST_OP(braced_init)) {
    return OverloadConversionRankBracedToReference(actual, formal_type, target);
  }
  int binding_rank = ReferenceBindingRank(actual, formal_type);
  if (binding_rank < 0) {
    return OverloadConversionFunctionRank(actual, formal_type);
  }
  int base_rank = OverloadBaseConversionRank(actual->type, target);
  if (base_rank >= 0) {
    return base_rank * 10 + binding_rank;
  }
  return OverloadConversionRankReferenceFallback(actual, formal_type, target,
                                                 binding_rank);
}

// Contract: Non-reference formal parameter dispatch; may mutate braced-init children.
static int OverloadConversionRankValue(ASTNode* actual, TypeRecord* target) {
  if (actual != NULL && actual->op == AST_OP(braced_init)) {
    return OverloadConversionRankBracedToValue(actual, target);
  }
  int base_rank = OverloadBaseConversionRank(actual->type, target);
  if (base_rank >= 0) {
    return OverloadConversionRankStandardValue(actual, target, base_rank);
  }
  return OverloadConversionRankValueFallback(actual, target);
}

// Contract: Read-only rank for overload resolution; may analyze braced-init
// elements and run speculative template deduction (diagnostics suppressed).
// Mutates braced-init child expressions via `AnalyzeExpression` when ranking
// list initialization.
int OverloadConversionRank(ASTNode* actual, TypeRecord* formal_type) {
  /* nullptr converts to any object pointer.  While `slot_type*` is still
   * `Policy::slot_type*`, that conversion is not a typed pointer yet; treat it
   * as viable so the other arguments can rank the overload. */
  if (actual != NULL && actual->type != NULL && TypeIsNullPointer(actual->type) &&
      formal_type != NULL && formal_type->declarator == kDeclPointer &&
      TypeHasDependentMemberName(formal_type)) {
    return 0;
  }
  if (TypeIsReference(formal_type)) {
    return OverloadConversionRankReference(actual, formal_type,
                                           formal_type->next);
  }
  return OverloadConversionRankValue(actual, formal_type);
}

// Contract: Speculative constructor viability rank; temporarily sets
// `g_suppress_user_defined_conversion_rank` while ranking the first parameter.
static int ConvertingConstructorRank(StructMember* member, ASTNode* from,
                                     bool allow_explicit,
                                     bool allow_invented) {
  Symbol* ctor_symbol = member->symbol;
  Symbol* temporary = NULL;
  if (ctor_symbol->flags.is_template) {
    Vector actuals;
    VectorInit(&actuals);
    VectorAppend(&actuals, from);
    DiagnosticSuppressBegin();
    temporary = TypeCreateFunctionTemplateCandidate(
        &compiler->syntax, ctor_symbol, NULL, &actuals,
        /*first_formal_arg=*/1);
    DiagnosticSuppressEnd();
    VectorDestruct(&actuals);
    if (temporary == NULL || temporary->type == NULL ||
        !TypeIsFunction(temporary->type)) {
      if (temporary != NULL) {
        SymbolDelete(temporary);
      }
      return -1;
    }
    ctor_symbol = temporary;
  }

  int rank = -1;
  FunctionInfo* fi = &ctor_symbol->type->info.function;
  if (!fi->is_constructor || fi->is_deleted ||
      (fi->is_explicit && !allow_explicit) ||
      (member->symbol->flags.invented && !allow_invented) ||
      fi->prototype.length < 1) {
    goto done;
  }
  if (fi->prototype.length == 1) {
    // `T(...)`: an ellipsis conversion sequence, worse than any other
    // ([over.ics.rank]); the same weight overload scoring gives it.
    if (fi->varargs) {
      rank = 1000;
    }
    goto done;
  }
  for (size_t i = 2; i < fi->prototype.length; i++) {
    Symbol* param = fi->prototype.value.p[i];
    if (param == NULL || SymbolExtraOf(param)->default_argument == NULL) {
      goto done;
    }
  }
  Symbol* param = fi->prototype.value.p[1];
  if (param == NULL || param->type == NULL) {
    goto done;
  }
  bool saved = g_suppress_user_defined_conversion_rank;
  g_suppress_user_defined_conversion_rank = true;
  rank = OverloadConversionRank(from, param->type);
  g_suppress_user_defined_conversion_rank = saved;

done:
  if (temporary != NULL) {
    SymbolDelete(temporary);
  }
  return rank;
}

// Contract: Read-only overload search; does not mutate AST. Returns NULL when
// ambiguous or no viable converting constructor exists.
StructMember* FindConvertingConstructorCandidate(TypeRecord* to,
                                                        ASTNode* from,
                                                        bool allow_explicit,
                                                        bool allow_same_class) {
  if (!CompilerIsCXX() || to == NULL || from == NULL || from->type == NULL ||
      !TypeIsStructOrUnion(to) || to->info.struct_info == NULL ||
      to->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  bool same_class_source =
      TypeIsStructOrUnion(from->type) &&
      TypeEqualIgnoringQualifiers(from->type, to);
  // Same-class sources are only considered when an explicit class cast needs
  // to materialize a copy or move. Other conversion contexts already have
  // their own copy-initialization path. Derived sources use the standard
  // derived-to-base conversion.
  if (same_class_source && !allow_same_class) {
    return NULL;
  }
  if (TypeIsStructOrUnion(from->type) && !same_class_source &&
      TypeIsDerivedFrom(from->type, to)) {
    return NULL;
  }
  Struct* str = to->info.struct_info;
  StructMember* ctor = FindStructMember(str, str->tag_name);
  StructMember* best = NULL;
  int best_rank = -1;
  bool ambiguous = false;
  for (StructMember* c = ctor; c != NULL; c = c->overload_next) {
    if (!c->is_member_function || c->symbol == NULL ||
        c->symbol->type == NULL || !TypeIsFunction(c->symbol->type)) {
      continue;
    }
    int rank = ConvertingConstructorRank(
        c, from, allow_explicit, allow_same_class && same_class_source);
    if (rank < 0) {
      continue;
    }
    if (best == NULL || rank < best_rank) {
      best = c;
      best_rank = rank;
      ambiguous = false;
    } else if (rank == best_rank) {
      bool best_is_template =
          best->symbol != NULL && best->symbol->flags.is_template;
      bool candidate_is_template = c->symbol->flags.is_template;
      if (best_is_template != candidate_is_template) {
        // [over.match.best.general]: for otherwise indistinguishable
        // candidates, a non-template function is better than a function
        // template specialization.  This is common for a class that has both
        // `T(const T&)` and `template<class U> T(const T<U>&)`.
        if (best_is_template) {
          best = c;
        }
        ambiguous = false;
      } else {
        ambiguous = true;
      }
    }
  }
  return ambiguous ? NULL : best;
}

// Contract: Wrapper excluding same-class sources from converting ctor search.
StructMember* CXXFindConvertingConstructorCandidate(TypeRecord* to, ASTNode* from,
                                                    bool allow_explicit) {
  return FindConvertingConstructorCandidate(
      to, from, allow_explicit, /*allow_same_class=*/false);
}

// Contract: Diagnoses only; does not convert `from`.
void CXXValidateReturnInitialization(TypeRecord* to, ASTNode* from) {
  StructMember* constructor = FindConvertingConstructorCandidate(
      to, from, /*allow_explicit=*/false, /*allow_same_class=*/true);
  if (constructor == NULL || constructor->symbol == NULL) {
    SemanticError(from, "No viable constructor for return value");
    return;
  }
  CheckDeletedFunctionUse(constructor->symbol, from);
}

// Converts `from` to the class type `to` by constructing a temporary through a
// viable converting constructor (`to(from)`), splicing the resulting
// prvalue temporary in place of `from`.  Returns true if such a conversion was
// performed.  `ctx == kConvertCast` additionally allows explicit constructors
// (matching the explicit-conversion semantics of a cast).
// Contract: Splices comma/constructor tree over `from` in its parent, analyzes
// with UDC ranking suppressed; may further rewrite for consteval temps. Caller
// must have `from->parent` set unless analyzing a detached subtree.
bool TryConvertWithConvertingConstructorImpl(
    ASTNode* from, TypeRecord* to, ConversionContext ctx,
    bool allow_same_class) {
  if (!CompilerIsCXX() || from == NULL || to == NULL ||
      !TypeIsStructOrUnion(to)) {
    return false;
  }
  // [over.best.ics]/4: an implicit conversion sequence contains at most one
  // user-defined conversion.  While we are already materializing one converting
  // constructor, a nested argument conversion must not invoke another, otherwise
  // resolving the constructor call can pick a constructor whose own parameter
  // requires the very same user-defined conversion, recursing without bound
  // (e.g. a copy constructor `T(const T&)` selected for a non-`T` argument).
  if (g_suppress_user_defined_conversion_rank) {
    return false;
  }
  StructMember* ctor = FindConvertingConstructorCandidate(
      to, from, /*allow_explicit=*/ctx == kConvertCast, allow_same_class);
  if (ctor == NULL) {
    return false;
  }
  Struct* str = to->info.struct_info;
  SourceLocation location = from->location;
  TypeRecord* type = TypeRecordCopy(to);
  TypeRecordCalculateSize(type);

  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  ASTNode* arg = ASTNodeMove(from);

  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member = NewStringConstantASTNode(NewString(str->tag_name->value),
                                             NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  Vector* actuals = NewVector();
  VectorAppend(actuals, arg);
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* result = NewIdentifierASTNode(temp, location);
  ASTNode* comma = NewBinaryASTNode(AST_OP(comma), type, location,
                                    constructor_call, result);
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, comma, true);
  }
  // Analyze the synthesized constructor call with user-defined conversions
  // suppressed: the argument must reach the chosen constructor's parameter by a
  // standard conversion only (the one user-defined conversion in this sequence
  // is the constructor itself), which prevents unbounded re-entry through
  // another converting constructor.
  bool saved_suppress = g_suppress_user_defined_conversion_rank;
  g_suppress_user_defined_conversion_rank = true;
  int errors_before_analysis = compiler->num_errors;
  ASTNode* analyzed = AnalyzeExpression(comma);
  g_suppress_user_defined_conversion_rank = saved_suppress;
  if (ctor->symbol != NULL && ctor->symbol->type != NULL &&
      TypeIsFunction(ctor->symbol->type) &&
      ctor->symbol->type->info.function.is_consteval &&
      compiler->num_errors == errors_before_analysis &&
      !CXXInImmediateFunctionContext() &&
      analyzed != NULL && analyzed->op == AST_OP(comma) &&
      ((BinaryASTNode*)analyzed)->left != NULL &&
      ((BinaryASTNode*)analyzed)->left->op != AST_OP(init)) {
    ASTNode* converted_call = ((BinaryASTNode*)analyzed)->left;
    if (converted_call != NULL && converted_call->op == AST_OP(call)) {
      VectorASTNode* call = (VectorASTNode*)converted_call;
      if (call->left != NULL && call->left->op == AST_OP(identifier)) {
        IdentifierASTNode* id = (IdentifierASTNode*)call->left;
        if (id->symbol != NULL && id->symbol->flags.is_template) {
          Symbol* instantiated =
              TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
                  &compiler->syntax, id->symbol, id->template_arguments,
                  call->children);
          if (instantiated != NULL && instantiated != id->symbol) {
            id->symbol = instantiated;
            ASTNodeSetInstantiatedCalleeType(call->left, instantiated->type);
          }
        }
      }
    }
    ConstEvalContext context;
    ConstEvalContextInit(&context);
    bool constant =
        converted_call != NULL && converted_call->op == AST_OP(call) &&
        ConstexprEvaluateConstructorCallForSymbol(&context, converted_call,
                                                  temp);
    ASTNode* initializer =
        constant ? ConstexprObjectInitializerForSymbol(temp, location) : NULL;
    temp->flags.value_set = false;
    temp->value.other = NULL;
    ConstEvalContextDestruct(&context);
    if (!constant) {
      if (!CXXEscalateCurrentFunction()) {
        SemanticError(converted_call != NULL ? converted_call : analyzed,
                      "consteval function call is not a constant expression");
      }
    } else if (initializer != NULL) {
      ASTNode* simplified =
          AnalyzeInitializer(temp->type, initializer, true);
      ASTNode* destination = NewIdentifierASTNode(temp, location);
      destination->flags |= kASTNeedAddress | kASTIsDeclaration;
      ASTNode* materialized =
          NewBinaryASTNode(AST_OP(init), TypeRecordCopy(temp->type), location,
                           destination, simplified);
      ASTNodeReplaceChild(analyzed, 0, materialized, true);
    }
  }
  analyzed->value_category = kValueCategoryPrvalue;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, analyzed, false);
  }
  return true;
}

// Contract: Thin wrapper; same AST mutation/ownership as
// `TryConvertWithConvertingConstructorImpl` with `allow_same_class=false`.
bool TryConvertWithConvertingConstructor(ASTNode* from, TypeRecord* to,
                                         ConversionContext ctx) {
  return TryConvertWithConvertingConstructorImpl(
      from, to, ctx, /*allow_same_class=*/false);
}

// Contract: Read-only tag-name comparison for libstdc++-style helpers.
static bool TypeIsNamedClass(TypeRecord* type, const char* name) {
  return type != NULL && TypeIsStructOrUnion(type) &&
         type->info.struct_info != NULL &&
         type->info.struct_info->tag_name != NULL &&
         strcmp(type->info.struct_info->tag_name->value, name) == 0;
}

// Contract: Recognizes `vector` class or template origin; read-only.
static bool TypeIsStdVectorType(TypeRecord* type) {
  if (TypeIsNamedClass(type, "vector")) {
    return true;
  }
  return type != NULL && type->template_origin != NULL &&
         strcmp(type->template_origin->name.value, "vector") == 0;
}

// Contract: Extracts element type from vector type record; read-only/borrowed.
static TypeRecord* StdVectorElementType(TypeRecord* vector_type) {
  if (vector_type == NULL) {
    return NULL;
  }
  if (vector_type->next != NULL) {
    return vector_type->next;
  }
  if (vector_type->template_arguments != NULL &&
      vector_type->template_arguments->length > 0) {
    TemplateArgument* arg = vector_type->template_arguments->value.p[0];
    if (arg != NULL && arg->kind == kTemplateParameterType) {
      return arg->type;
    }
  }
  return NULL;
}

// Contract: On success replaces `from` in its parent with a synthesized
// `.data()` call (and conversion); moves `from` into the new subtree.
bool CXXTryConvertStdVectorToPointeePointer(ASTNode* from, TypeRecord* to) {
  if (!CompilerIsCXX() || from == NULL || from->type == NULL || to == NULL ||
      !TypeIsPointer(to) || !TypeIsStdVectorType(from->type)) {
    return false;
  }
  TypeRecord* element = StdVectorElementType(from->type);
  if (element == NULL || to->next == NULL ||
      (!TypeEqualIgnoringQualifiers(element, to->next) &&
       !TypeEqualIgnoringSign(element, to->next))) {
    return false;
  }
  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  SourceLocation location = from->location;
  ASTNode* object = ASTNodeMove(from);
  ASTNode* data_member =
      NewStringConstantASTNode(NewString("data"), NULL, location);
  ASTNode* data_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, object, data_member);
  Vector* args = NewVector();
  ASTNode* data_call =
      NewVectorASTNode(AST_OP(call), NULL, location, data_access, args);
  ASTNode* analyzed = AnalyzeExpression(data_call);
  if (analyzed == NULL) {
    return false;
  }
  SemanticConvertType(analyzed, to, kConvertNormal);
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, analyzed, false);
  }
  return true;
}

// Contract: Synthesizes and analyzes `.member()` on `vector`; returns analyzed call.
static ASTNode* CXXStdVectorMemberCall(ASTNode* vector,
                                       const char* member_name,
                                       SourceLocation location) {
  ASTNode* member =
      NewStringConstantASTNode(NewString(member_name), NULL, location);
  ASTNode* access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, vector, member);
  return AnalyzeExpression(
      NewVectorASTNode(AST_OP(call), NULL, location, access, NewVector()));
}

// Contract: May delegate to converting-constructor materialization or replace
// `from` with braced-init lowering for `{data(), size()}`; parent splice required.
bool CXXTryConvertStdVectorToSpan(ASTNode* from, TypeRecord* to) {
  if (!CompilerIsCXX() || from == NULL || from->type == NULL || to == NULL ||
      !TypeIsNamedClass(to, "Span") || !TypeIsStdVectorType(from->type)) {
    return false;
  }
  if (TryConvertWithConvertingConstructorImpl(from, to, kConvertNormal,
                                              /*allow_same_class=*/false)) {
    return true;
  }
  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  SourceLocation location = from->location;
  ASTNode* vector = ASTNodeMove(from);
  ASTNode* data = CXXStdVectorMemberCall(vector, "data", location);
  ASTNode* size = CXXStdVectorMemberCall(vector, "size", location);
  if (data == NULL || size == NULL) {
    return false;
  }
  TypeRecord* span_type = TypeRecordCopy(to);
  TypeRecordCalculateSize(span_type);
  Vector* elements = NewVector();
  VectorAppend(elements,
               NewExpressionInitializerASTNode(data, location));
  VectorAppend(elements,
               NewExpressionInitializerASTNode(size, location));
  ASTNode* braced =
      NewBracedInitializerASTNode(elements, span_type, location);
  ASTNode* lowered = LowerCXXBracedInitToTarget(braced, span_type);
  if (lowered == NULL || lowered == braced) {
    return false;
  }
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, lowered, false);
  }
  return true;
}
