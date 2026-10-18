//
//  expr_semantics_cast.c
//  c_compiler
//
//  C and C++ casts, dynamic casts, and compound literals.
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

static void ValidateCXXConstCast(CastASTNode* node);
static void AnalyzeDynamicCast(CastASTNode* node);
static bool TryCastReferenceRelatedClass(CastASTNode* node);
static ASTNode* AnalyzeBitCast(CastASTNode* node);

// Contract: Pure type comparison; does not mutate AST or type records.
bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator) {
    return false;
  }
  switch (left->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
    case kDeclArray:
      return TypeEqualIgnoringQualifiers(left->next, right->next);
    case kDeclVector:
      return left->info.array.size.fixed == right->info.array.size.fixed &&
             TypeEqualIgnoringQualifiers(left->next, right->next);
    case kDeclMemberPointer:
      return TypeMemberPointerClass(left) == TypeMemberPointerClass(right) &&
             TypeEqualIgnoringQualifiers(
                 TypeMemberPointerPointeeType(left),
                 TypeMemberPointerPointeeType(right));
    case kDeclFunction:
      return TypeEqual(left, right);
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
        if (!TypeIsStructOrUnion(left) || !TypeIsStructOrUnion(right)) {
          return false;
        }
        TypeRecord unqualified_left = TypeRecordValue(left);
        TypeRecord unqualified_right = TypeRecordValue(right);
        SetQualifiers(&unqualified_left, kQualPlain);
        SetQualifiers(&unqualified_right, kQualPlain);
        return TypeEqual(&unqualified_left, &unqualified_right);
      }
      if (TypeIsEnum(left) || TypeIsEnum(right)) {
        return TypeIsEnum(left) && TypeIsEnum(right) &&
               left->info.enum_info == right->info.enum_info;
      }
      // `short` and `signed short` are the same type.  Template substitution
      // can spell one with the signed bit set and the other without it; a raw
      // bit compare then ranks that identity as a sign conversion, tying
      // `operator<<(short)` with `operator<<(int)`.
      TypeRecord unqualified_left = TypeRecordValue(left);
      TypeRecord unqualified_right = TypeRecordValue(right);
      SetQualifiers(&unqualified_left, kQualPlain);
      SetQualifiers(&unqualified_right, kQualPlain);
      return TypeEqual(&unqualified_left, &unqualified_right);
  }
  return false;
}

// A dependent alias or injected-class name in a cloned const_cast can retain
// its template pattern after the operand becomes concrete. Rebuild the target's
// underlying type from the operand while preserving the written cv-qualifiers
// and pointer/reference shape.
static TypeRecord* RebuildDependentConstCastTarget(TypeRecord* target,
                                                   TypeRecord* source) {
  if (target == NULL || source == NULL) {
    return NULL;
  }
  if (TypeIsReference(target)) {
    TypeRecord* referent =
        RebuildDependentConstCastTarget(target->next,
                                       TypeIsReference(source) ? source->next
                                                               : source);
    if (referent == NULL) {
      return NULL;
    }
    TypeRecord* result = NewReferenceTypeRecord(
        GetQualifiers(target), target->declarator == kDeclRValueReference);
    TypeRecordChain(result, referent);
    result->type = referent->type;
    return TypeRecordCalculateSize(result);
  }
  if (TypeIsPointer(target)) {
    if ((!TypeIsPointer(source) && !TypeIsArray(source)) ||
        target->next == NULL || source->next == NULL) {
      return NULL;
    }
    TypeRecord* pointee =
        RebuildDependentConstCastTarget(target->next, source->next);
    if (pointee == NULL) {
      return NULL;
    }
    TypeRecord* result = NewPointerTypeRecord(GetQualifiers(target));
    TypeRecordChain(result, pointee);
    result->type = pointee->type;
    return TypeRecordCalculateSize(result);
  }
  TypeRecord* result = TypeRecordCloneSpine(source);
  SetQualifiers(result, GetQualifiers(target));
  return TypeRecordCalculateSize(result);
}

// Contract: Validates the cast and may repair a stale dependent target;
// may skip errors when `node->expr` is template-dependent. Does not convert the
// operand or set the cast node's result type.
static void ValidateCXXConstCast(CastASTNode* node) {
  TypeRecord* to = node->cast_type;
  TypeRecord* from = node->expr->type;
  bool concrete_source =
      from != NULL && !TypeContainsTemplateParameter(from);
  bool stale_dependent_target =
      (node->base.flags & kASTDependentConstCastTarget) != 0;
  if (stale_dependent_target && concrete_source) {
    TypeRecord* repaired = RebuildDependentConstCastTarget(to, from);
    if (repaired != NULL) {
      TypeRecordDelete(node->cast_type);
      node->cast_type = repaired;
      TypeRecordIncRef(repaired);
      to = repaired;
    }
    node->base.flags &= ~kASTDependentConstCastTarget;
  }
  bool valid = false;
  if (TypeIsPointer(to) && to->next != NULL && !TypeIsFunction(to->next) &&
      from != NULL && from->next != NULL && !TypeIsFunction(from->next) &&
      (TypeIsPointer(from) || TypeIsArray(from))) {
    // An array operand decays to a pointer to its first element
    // (`const_cast<T*>(array)`).
    valid = TypeEqualIgnoringQualifiers(to->next, from->next);
  } else if (TypeIsReference(to) && to->next != NULL &&
             !TypeIsFunction(to->next)) {
    TypeRecord* from_object = TypeIsReference(from) ? from->next : from;
    bool compatible_value_category =
        to->declarator == kDeclRValueReference
            ? node->expr->value_category == kValueCategoryXvalue
            : node->expr->value_category == kValueCategoryLvalue;
    valid = compatible_value_category && from_object != NULL &&
            TypeEqualIgnoringQualifiers(to->next, from_object);
  }
  if (!valid) {
    // A member function template is cloned once when its class is instantiated,
    // while its own parameters are still placeholders.  The operand can be a
    // call whose result type collapsed to a concrete fallback even though the
    // argument still names that placeholder (`addressof(forward<Key>(k))`).
    // Checking const_cast then would reject a cast that is well-formed once
    // the member template is instantiated for a call.
    if (node->expr != NULL && ExpressionIsTemplateDependent(node->expr)) {
      return;
    }
    SemanticError((ASTNode*)node,
                  "const_cast requires pointer or reference to the same type");
    TypeErrorDetails(node->base.location, to, from);
  }
}

// Analyzes a dynamic_cast.  Identity and upcast (to an unambiguous public base)
// are resolved statically as before.  Polymorphic downcasts and sidecasts are
// marked for a run-time check lowered during codegen.  Handles both the pointer
// form (null on failure) and the reference form (throws std::bad_cast).
// Contract: Mutates operand via `SemanticConvertType` when static; sets
// `dynamic_runtime` and result type/category on the cast node.
static void AnalyzeDynamicCast(CastASTNode* node) {
  TypeRecord* to = node->cast_type;
  TypeRecord* from = node->expr->type;
  bool to_ref = TypeIsReference(to);
  TypeRecord* to_class = (to_ref || TypeIsPointer(to)) ? to->next : NULL;
  TypeRecord* from_class = to_ref ? from
                                  : (TypeIsPointer(from) ? from->next : NULL);

  if (to_class == NULL || from_class == NULL ||
      !TypeIsStructOrUnion(to_class) || !TypeIsStructOrUnion(from_class)) {
    SemanticError((ASTNode*)node,
                  "dynamic_cast requires pointer or reference to class type");
    ASTNodeSetType((ASTNode*)node, to);
    return;
  }

  bool identity = TypeEqual(to_class, from_class);
  bool upcast = !identity && TypeIsDerivedFrom(from_class, to_class);

  if (identity || upcast) {
    // Statically resolvable (no run-time check needed).
    node->dynamic_runtime = false;
    if (to_ref) {
      if (!TypeEqualIgnoringQualifiers(from_class, to_class)) {
        SemanticConvertType(node->expr, to_class, kConvertCast);
      }
      ASTNodeSetType((ASTNode*)node, to_class);
      node->base.value_category = to->declarator == kDeclRValueReference
                                      ? kValueCategoryXvalue
                                      : kValueCategoryLvalue;
    } else {
      SemanticConvertType(node->expr, to, kConvertCast);
      ASTNodeSetType((ASTNode*)node, to);
    }
    return;
  }

  // Downcast / sidecast: requires a polymorphic source.
  if (!TypeIsPolymorphicClass(from_class)) {
    SemanticError((ASTNode*)node,
                  "dynamic_cast of a non-polymorphic type");
    ASTNodeSetType((ASTNode*)node, to);
    return;
  }

  node->dynamic_runtime = true;
  if (to_ref) {
    ASTNodeSetType((ASTNode*)node, to_class);
    node->base.value_category = to->declarator == kDeclRValueReference
                                    ? kValueCategoryXvalue
                                    : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)node, to);
  }
}

// Contract: On success rewrites `node->expr` to a dereferenced pointer cast and
// may replace `node->cast_type->next` with a materialized specialization
// (caller owns the cast node). Returns false without mutating `expr` when the
// types are unrelated.
static bool TryCastReferenceRelatedClass(CastASTNode* node) {
  if (!CompilerIsCXX() || node->expr == NULL || node->expr->type == NULL) {
    return false;
  }
  TypeRecord* target = node->cast_type->next;
  if (target == NULL || node->expr->type == NULL) {
    return false;
  }
  // `static_cast<std::ostream&>(derived)` names the typedef, which may still
  // be the forward-declared primary of basic_ostream (not yet a class type).
  // The base subobject is the completed specialization.  Compare those.
  TypeRecord* from_type = MaterializeConversionClass(node->expr->type);
  TypeRecord* to_type = MaterializeConversionClass(target);
  bool own_from = from_type != node->expr->type;
  bool own_to = to_type != target;
  if (!TypeIsStructOrUnion(from_type) || !TypeIsStructOrUnion(to_type) ||
      from_type->info.struct_info == NULL ||
      to_type->info.struct_info == NULL) {
    if (own_from) {
      TypeRecordDelete(from_type);
    }
    if (own_to) {
      TypeRecordDelete(to_type);
    }
    return false;
  }
  CXXBaseAdjustment adjustment;
  bool related =
      TypeBaseAdjustment(from_type, to_type, /*public_only=*/false,
                         &adjustment) ||
      TypeBaseAdjustment(to_type, from_type, /*public_only=*/false,
                         &adjustment);
  if (!related) {
    if (own_from) {
      TypeRecordDelete(from_type);
    }
    if (own_to) {
      TypeRecordDelete(to_type);
    }
    return false;
  }
  if (own_to) {
    TypeRecord* stored = TypeRecordCopy(to_type);
    TypeRecordIncRef(stored);
    TypeRecordDelete(node->cast_type->next);
    node->cast_type->next = stored;
    target = stored;
  }
  if (own_from) {
    TypeRecordDelete(from_type);
  }
  if (own_to) {
    TypeRecordDelete(to_type);
  }
  SourceLocation loc = node->expr->location;
  ASTNode* addr = NewAnalyzedBuiltinAddressOf(node->expr, loc);
  TypeRecord* target_ptr = NewPointerTo(kQualPlain, TypeRecordCopy(target));
  ASTNode* ptr_cast = NewCastASTNode(target_ptr, loc, addr);
  ((CastASTNode*)ptr_cast)->kind = node->kind;
  ptr_cast = AnalyzeExpression(ptr_cast);
  ASTNode* deref = NewUnaryASTNode(AST_OP(contents), NULL, loc, ptr_cast);
  deref = AnalyzeExpression(deref);
  node->expr = deref;
  deref->parent = (ASTNode*)node;
  return true;
}

// Bind a class lvalue/xvalue `expr` to a reference whose referent type is a
// (possibly virtual, possibly offset) base class of `expr`'s class.  The base
// subobject may live at a non-zero offset -- and for a virtual base, only
// reachable through the run-time __vbptr -- so the reference must point at the
// adjusted subobject address, not the raw derived-object address.  Rewrites
// `expr` to `*static_cast<Base*>(&expr)`, reusing the pointer up-cast path that
// already emits the correct static/virtual adjustment.  Returns the new lvalue
// node (detached from any parent) when the rewrite applies, or NULL when `expr`
// is not a base-class subobject reference (leaving `expr` untouched).
// Contract: Moves `expr` into synthesized cast/deref tree; caller owns returned node.
// A typedef such as `using ostream = basic_ostream<char>` can be formed while
// the class template is still only forward-declared.  That stores a deferred
// template-id (the primary, flagged is_template, carrying the concrete
// arguments) rather than the specialization created once the definition is
// parsed.  Derived-to-base then fails because the base subobject uses the
// completed specialization.  Re-instantiate before comparing.
// Contract: May return materialized specialization (re-entrancy guarded); caller
// deletes when distinct from input.
TypeRecord* MaterializeConversionClass(TypeRecord* type) {
  static int depth = 0;
  if (depth > 0 || !CompilerIsCXX() || type == NULL || compiler == NULL) {
    return type;
  }
  depth++;
  TypeRecord* materialized =
      TypeMaterializeClassTemplateSpecialization(&compiler->syntax, type);
  depth--;
  return materialized != NULL ? materialized : type;
}

// Contract: On success returns a new analyzed glvalue for `referent`, consuming
// `expr` via ASTNodeMove; on failure returns NULL and leaves `expr` untouched.
// Materializes template class types internally and deletes any copies that differ
// from the inputs; caller owns the returned tree. Requires HasAddress(expr).
ASTNode* TryBindReferenceToBaseSubobject(ASTNode* expr,
                                                TypeRecord* referent) {
  if (!CompilerIsCXX() || expr == NULL || expr->type == NULL ||
      referent == NULL) {
    return NULL;
  }
  TypeRecord* from_type = MaterializeConversionClass(expr->type);
  TypeRecord* to_type = MaterializeConversionClass(referent);
  bool materialized_from = from_type != expr->type;
  bool materialized_to = to_type != referent;
  if (!TypeIsStructOrUnion(from_type) || !TypeIsStructOrUnion(to_type) ||
      TypeEqualIgnoringQualifiers(from_type, to_type)) {
    if (materialized_from) {
      TypeRecordDelete(from_type);
    }
    if (materialized_to) {
      TypeRecordDelete(to_type);
    }
    return NULL;
  }
  // Only meaningful for an object that already has an address; a materialized
  // temporary (no address) is handled separately by the caller.
  if (!HasAddress(expr)) {
    if (materialized_from) {
      TypeRecordDelete(from_type);
    }
    if (materialized_to) {
      TypeRecordDelete(to_type);
    }
    return NULL;
  }
  CXXBaseAdjustment adjustment;
  if (!TypeBaseAdjustment(from_type, to_type, /*public_only=*/true,
                          &adjustment)) {
    // Private and protected bases are accessible to the derived class and to
    // its friends.  `CIter::CIter(const Iter& other) : Gen(other)` binds the
    // private base of `Iter` because `CIter` is a friend.
    if (!TypeIsDerivedFromInCurrentClass(from_type, to_type) ||
        !TypeBaseAdjustment(from_type, to_type, /*public_only=*/false,
                            &adjustment)) {
      if (materialized_from) {
        TypeRecordDelete(from_type);
      }
      if (materialized_to) {
        TypeRecordDelete(to_type);
      }
      return NULL;
    }
  }
  if (materialized_from) {
    TypeRecordDelete(from_type);
  }
  // Keep the completed specialization as the pointer target.  The deferred
  // referent names the primary template and has no layout.  The pointer type
  // copies it, so the temporary can be released afterwards.
  if (materialized_to) {
    referent = to_type;
  }
  SourceLocation loc = expr->location;
  bool rvalue = expr->value_category != kValueCategoryLvalue;
  ASTNode* moved = ASTNodeMove(expr);
  ASTNode* addr = NewAnalyzedBuiltinAddressOf(moved, loc);
  TypeRecord* base_ptr = NewPointerTo(kQualPlain, TypeRecordCopy(referent));
  if (materialized_to) {
    TypeRecordDelete(to_type);
  }
  ASTNode* ptr_cast = NewCastASTNode(base_ptr, loc, addr);
  ((CastASTNode*)ptr_cast)->kind = kCastStatic;
  ptr_cast = AnalyzeExpression(ptr_cast);
  ASTNode* deref = NewUnaryASTNode(AST_OP(contents), NULL, loc, ptr_cast);
  deref = AnalyzeExpression(deref);
  // The base subobject of an xvalue (or of a materialized prvalue) is an
  // xvalue ([expr.static.cast]), so it still binds to `Base&&`.
  if (rvalue && deref != NULL) {
    deref->value_category = kValueCategoryXvalue;
  }
  return deref;
}

// [bit.cast]: both types are trivially copyable and of the same size, and the
// result is a prvalue of the target type holding the operand's bits.  The
// operand is left unconverted; code generation reads it through its address.
// Contract: Sets cast type/category; may materialize operand temporary and set
// kASTNeedAddress; does not convert operand to target type semantically.
static ASTNode* AnalyzeBitCast(CastASTNode* node) {
  TypeRecord* to = node->cast_type;
  TypeRecord* from = node->expr->type;
  ASTNodeSetType((ASTNode*)node, to);
  node->base.value_category = kValueCategoryPrvalue;
  if (to == NULL || from == NULL) {
    return (ASTNode*)node;
  }
  if (TypeIsReference(to) || TypeIsArray(to) || TypeIsFunction(to) ||
      TypeIsVoid(to)) {
    SemanticError((ASTNode*)node,
                  "__builtin_bit_cast cannot produce this type");
    return (ASTNode*)node;
  }
  TypeRecordCalculateSize(to);
  TypeRecordCalculateSize(from);
  if (TypeIsFunction(from) || TypeIsVoid(from) || to->size <= 0 ||
      to->size != from->size) {
    SemanticError((ASTNode*)node,
                  "__builtin_bit_cast requires types of the same size");
    return (ASTNode*)node;
  }
  if (CompilerIsCXX() && (!CXXTypeIsTriviallyCopyable(to) ||
                          !CXXTypeIsTriviallyCopyable(from))) {
    SemanticError((ASTNode*)node,
                  "__builtin_bit_cast requires trivially copyable types");
    return (ASTNode*)node;
  }
  if (!HasAddress(node->expr)) {
    ASTNode* materialized = MaterializeTemporary(node->expr, from);
    materialized->parent = (ASTNode*)node;
    materialized->child_id = 0;
    node->expr = materialized;
  }
  node->expr->flags |= kASTNeedAddress;
  return (ASTNode*)node;
}

// Contract: Analyzes and may rewrite `node` (operand conversions, braced-init
// lowering, dependent-type early exit). Returns the surviving root (often
// `node`, sometimes a replacement); caller must splice via parent when the
// returned pointer differs. May delete `node` when braced auto-cast lowers.
ASTNode* AnalyzeCastExpression(CastASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  bool auto_paren = node->kind == kCastAutoParen;
  bool auto_brace = node->kind == kCastAutoBrace;
  if (auto_paren || auto_brace) {
    TypeRecord* deduced =
        TypeDecayForByValueDeduction(node->expr != NULL ? node->expr->type
                                                        : NULL);
    if (deduced == NULL) {
      SemanticError((ASTNode*)node,
                    "Cannot deduce the target type of an auto cast");
      deduced = NewTypeRecordWithSize(kTypeInt, kQualPlain);
    }
    TypeRecordDelete(node->cast_type);
    node->cast_type = deduced;
    TypeRecordIncRef(node->cast_type);
    // Direct-list-initialization from a prvalue of the deduced type uses that
    // prvalue as the result object; it must not synthesize an extra move.
    if (auto_brace && !TypeIsVoid(node->cast_type) && node->expr != NULL &&
        node->expr->value_category == kValueCategoryPrvalue &&
        TypeEqualIgnoringQualifiers(node->expr->type, node->cast_type)) {
      node->kind = kCastAutoParen;
      auto_paren = true;
      auto_brace = false;
    }
  }
  if (node->kind == kCastDynamic) {
    AnalyzeDynamicCast(node);
    return (ASTNode*)node;
  }
  if (node->kind == kCastConst) {
    ValidateCXXConstCast(node);
  }
  // A cast whose target or operand type still mentions an unbound template
  // parameter is dependent: during the first (class-level) instantiation of a
  // member function template, the enclosing class parameters are concrete but
  // the member template's own parameters are not, so a cast such as a
  // constructor mem-initializer's `static_cast<T&&>(t)` cannot be checked yet.
  // Forcing a conversion now would either bake in the wrong one or, when a
  // dependent parameter is treated as an incomplete class, spuriously reject a
  // struct-to-struct cast.  Leave the operand unconverted and take the target
  // type; the member template's second-stage (per-call) instantiation re-clones
  // and re-analyzes the cast against the concrete argument type.
  if (CompilerIsCXX() &&
      (TypeContainsTemplateParameter(node->cast_type) ||
       (node->expr != NULL && node->expr->type != NULL &&
        TypeContainsTemplateParameter(node->expr->type)) ||
       (node->expr != NULL && ExpressionIsTemplateDependent(node->expr)))) {
    if (TypeIsReference(node->cast_type)) {
      ASTNodeSetType((ASTNode*)node, node->cast_type->next);
      node->base.value_category =
          node->cast_type->declarator == kDeclRValueReference
              ? kValueCategoryXvalue
              : kValueCategoryLvalue;
    } else {
      ASTNodeSetType((ASTNode*)node, node->cast_type);
      node->base.value_category = kValueCategoryPrvalue;
    }
    return (ASTNode*)node;
  }
  if (node->kind == kCastBit) {
    return AnalyzeBitCast(node);
  }
  if (auto_brace && TypeIsVoid(node->cast_type)) {
    SemanticError((ASTNode*)node,
                  "a braced auto cast cannot initialize void");
    ASTNodeSetType((ASTNode*)node, node->cast_type);
    node->base.value_category = kValueCategoryPrvalue;
    return (ASTNode*)node;
  }
  if (auto_brace) {
    Vector* elements = NewVector();
    VectorAppend(elements, ASTNodeMove(node->expr));
    ASTNode* braced =
        NewBracedInitializerASTNode(elements, NULL, node->base.location);
    ASTNode* lowered =
        LowerCXXBracedInitToTarget(braced, node->cast_type);
    ASTNode* parent = node->base.parent;
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, node->base.child_id, lowered, true);
    } else {
      ASTNodeDelete((ASTNode*)node);
    }
    return lowered;
  }
  if (TypeIsVoid(node->cast_type) && node->expr != NULL) {
    node->expr = SemanticMaterializeClassPrvalue(node->expr);
  }
  if (TypeIsReference(node->cast_type)) {
    if (!TypeEqualIgnoringQualifiers(node->expr->type, node->cast_type->next)) {
      // [expr.reinterpret.cast]/11: a glvalue of any object type can be
      // cast to a reference to another object type.  C-style casts may
      // perform that conversion.  Do not run a value conversion: the
      // result refers to the same object (`std::addressof`).
      bool reinterpret_reference =
          node->kind == kCastReinterpret || node->kind == kCastCStyle;
      if (!reinterpret_reference && !TryCastReferenceRelatedClass(node)) {
        SemanticConvertType(node->expr, node->cast_type->next, kConvertCast);
      }
    }
    ASTNodeSetType((ASTNode*)node, node->cast_type->next);
    node->base.value_category =
        node->cast_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    bool same_class =
        CompilerIsCXX() && TypeIsStructOrUnion(node->cast_type) &&
        TypeEqualIgnoringQualifiers(node->expr->type, node->cast_type);
    bool identity_same_class_prvalue =
        auto_paren && same_class &&
        node->expr->value_category == kValueCategoryPrvalue;
    bool materialized_same_class =
        identity_same_class_prvalue ||
        (same_class &&
         TryConvertWithConvertingConstructorImpl(
             node->expr, node->cast_type, kConvertCast,
             /*allow_same_class=*/true));
    if (auto_paren && same_class && !materialized_same_class) {
      SemanticError((ASTNode*)node,
                    "auto cast cannot construct its result from the operand");
    }
    if (!materialized_same_class) {
      SemanticConvertType(node->expr, node->cast_type, kConvertCast);
    }
    // Result is the requested type.
    ASTNodeSetType((ASTNode*)node, node->cast_type);
    node->base.value_category = kValueCategoryPrvalue;
  }
  return (ASTNode*)node;
}

// Contract: Mutates `node` and `node->sym` types when the literal names a
// forward-declared class template primary; instantiates layout in place. No-op
// when the type is already a complete specialization.
void MaterializeBracedClassTemplateTemporary(
    CompoundLiteralASTNode* node) {
  TypeRecord* type = node->base.type;
  if (type == NULL || type->template_origin == NULL ||
      type->template_arguments == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      !type->info.struct_info->is_template) {
    return;
  }
  TypeRecord* specialized = TypeInstantiateClassTemplate(
      &compiler->syntax, type->template_origin, type->template_arguments);
  if (specialized == NULL || !TypeIsStructOrUnion(specialized) ||
      specialized->info.struct_info == NULL ||
      specialized->info.struct_info->is_template) {
    TypeRecordDelete(specialized);
    return;
  }
  if (node->sym != NULL && node->sym->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->sym;
    if (id->symbol != NULL) {
      id->symbol->type = TypeRecordCopy(specialized);
      ASTNodeSetType(node->sym, id->symbol->type);
    }
  }
  ASTNodeSetType((ASTNode*)node, TypeRecordCopy(specialized));
  TypeRecordDelete(specialized);
}
