//
//  expr_semantics_template.c
//  c_compiler
//
//  Template traits, variable-template folding, and identifiers.
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

static ASTNode* FoldConstantIdentifier(IdentifierASTNode* node);
static TypeRecord* CXXTypeFromLambdaInventedParameter(TypeRecord* type);
static Vector* CXXLambdaInventedTemplateArgumentValues(TypeRecord* func);
static TypeRecord* CXXResolveTypeForIsSameVArgument(TemplateArgument* arg);
static bool ParentSlotHoldsNode(ASTNode* parent, int child_id, ASTNode* node);
static ASTNode* CXXNewBoolConstantReplacing(ASTNode* node, int64_t same);
static ASTNode* CXXNewIsSameBoolConstant(IdentifierASTNode* node, int64_t same);
static bool CXXIsSameTemplateParameterType(TypeRecord* left, TypeRecord* right);
static int64_t CXXFoldIsSameSpecializationToBool(TypeRecord* materialized);
static bool CXXTypeIsStdArithmeticType(TypeRecord* type);
static int64_t CXXFoldIsBaseOfSpecializationToBool(TypeRecord* materialized);
static ASTNode* CXXTryFoldTraitValueDot(BinaryASTNode* dot);
static ASTNode* CXXTryFoldIsSameValueDot(BinaryASTNode* dot);
static ASTNode* CXXTryFoldIsSameVIdentifier(IdentifierASTNode* node);
static ASTNode* CXXTryFoldIsBaseOfVIdentifier(IdentifierASTNode* node);
static ASTNode* CXXTryFoldIsArithmeticVIdentifier(IdentifierASTNode* node);
static int CXXKUsesPointerConstant(Vector* template_arguments);
static ASTNode* CXXTryFoldKUsesPointerTemplate(IdentifierASTNode* node);
static bool CXXStructDerivesFromProtobufMessageLite(Struct* str);
static bool CXXTypeIsProtobufMessageLiteOrDerived(TypeRecord* type);
static size_t CXXTemplateArgumentPackLength(Vector* template_args, size_t pack_index);
static TypeRecord* CXXTemplateArgumentPackTypeAt(Vector* template_args, size_t pack_index, size_t element_index);
static int64_t CXXFoldProbeConstructTypeFromArgs(Vector* template_args);
static IdentifierASTNode* CXXCallCalleeIdentifier(ASTNode* callee);
static Vector* CXXSubstituteTemplateArgumentsForFold(Vector* template_arguments);
static bool CXXTypeNamesStdTuple(TypeRecord* type);
static BracedInitializerASTNode* CXXTupleArgumentBracedInit(ASTNode* arg);
static ASTNode* CXXBracedInitElementExpr(BracedInitializerASTNode* braced, size_t index);
static ASTNode* CXXTryFoldGetConstructTypeCall(VectorASTNode* call);
static TypeRecord* CXXVariableTemplateValueType(TypeRecord* type);
static ASTNode* CXXVariableTemplateExpressionSurface(ASTNode* expr);
static Symbol* CXXStdTupleClassTemplateSymbol(void);
static TypeRecord* CXXTupleTypeForSingleElementBracedInit( BracedInitializerASTNode* braced);
static void CXXPrepareStdGetVariableTemplateInitializer(ASTNode* expr);
static ASTNode* CXXInstantiateVariableTemplateValue(Symbol* var_sym, Vector* template_args, SourceLocation location);

static ASTNode* AnalyzeIdentifierHandleCXX26NameIndependentAmbiguity(
    IdentifierASTNode* node);
static void AnalyzeIdentifierDiagnoseMainInExpression(IdentifierASTNode* node);
static ASTNode* AnalyzeIdentifierHandleDependentQualifiedName(
    IdentifierASTNode* node);
static ASTNode* AnalyzeIdentifierFoldConceptId(IdentifierASTNode* node);
static ASTNode* AnalyzeIdentifierFoldVariableTemplateTraits(
    IdentifierASTNode* node);
static ASTNode* AnalyzeIdentifierMaterializeVariableTemplate(
    IdentifierASTNode* node);
static void AnalyzeIdentifierInstantiateUniqueFunctionTemplateId(
    IdentifierASTNode* node);
static void AnalyzeIdentifierTrackSymbolUseAndDeprecated(
    IdentifierASTNode* node);
static void AnalyzeIdentifierSetNonDeclarationType(IdentifierASTNode* node);
static ASTNode* AnalyzeIdentifierFinishValueCategoryAndFolding(
    IdentifierASTNode* node);


// Replaces a read of a scalar constant variable by its value.
// Contract: Replaces identifier in parent when value is known; deletes old node.
static ASTNode* FoldConstantIdentifier(IdentifierASTNode* node) {
  if (node->symbol == NULL || !node->symbol->flags.value_set ||
      (node->base.flags & (kASTNeedAddress | kASTIsDeclaration)) != 0 ||
      &node->base == compiler->decltype_operand) {
    return &node->base;
  }
  if (!CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC23) &&
      !node->symbol->flags.is_constexpr) {
    return &node->base;
  }
  ASTNode* const_node = NULL;
  if (TypeIsIntConstant(node->base.type)) {
    const_node = NewIntConstantASTNode(node->symbol->value.ivalue,
                                       node->symbol->type, node->base.location);
  } else if (TypeIsFloatingPointConstant(node->base.type)) {
    const_node = NewRealConstantASTNode(
        node->symbol->value.fvalue, node->symbol->type, node->base.location);
  } else {
    return &node->base;
  }
  ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node, true);
  const_node->flags |= kASTAnalyzed;
  return const_node;
}

// Contract: May replace `actual` in its parent with a scalar constant; otherwise
// returns `actual` unchanged.
ASTNode* FoldConstantArgument(ASTNode* actual) {
  if (actual == NULL || actual->op != AST_OP(identifier) ||
      actual->type == NULL || TypeIsStructOrUnion(actual->type) ||
      TypeIsArray(actual->type) || TypeIsFunction(actual->type)) {
    return actual;
  }
  return FoldConstantIdentifier((IdentifierASTNode*)actual);
}

// Contract: Maps template-parameter types to generic-lambda invented formals;
// read-only (returns borrowed formal type or newly substituted type).
static TypeRecord* CXXTypeFromLambdaInventedParameter(TypeRecord* type) {
  if (type == NULL || !TypeContainsTemplateParameter(type)) {
    return NULL;
  }
  TypeRecord* func = compiler->current_function;
  if (func == NULL || !TypeIsFunction(func)) {
    return NULL;
  }
  int idx = type->template_parameter_index;
  if (idx < 0) {
    for (TypeRecord* cur = type; cur != NULL; cur = cur->next) {
      if (cur->template_parameter_index >= 0) {
        idx = cur->template_parameter_index;
        break;
      }
    }
  }
  if (idx < 0) {
    return NULL;
  }
  int local = idx - func->info.function.template_parameter_base;
  if (local < 0) {
    return NULL;
  }
  size_t start = func->info.function.cxx_member_owner != NULL ? 1 : 0;
  size_t formal_index = start + (size_t)local;
  if (formal_index >= func->info.function.prototype.length) {
    return NULL;
  }
  Symbol* formal = func->info.function.prototype.value.p[formal_index];
  if (formal == NULL || formal->type == NULL ||
      TypeContainsAuto(formal->type) ||
      TypeContainsTemplateParameter(formal->type)) {
    return NULL;
  }
  return formal->type;
}

// Contract: Builds type template-arg vector from lambda `auto` parameters;
// caller owns vector or NULL on failure.
static Vector* CXXLambdaInventedTemplateArgumentValues(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.template_parameters.length == 0) {
    return NULL;
  }
  size_t start = func->info.function.cxx_member_owner != NULL ? 1 : 0;
  size_t param_count = func->info.function.prototype.length - start;
  if (param_count != func->info.function.template_parameters.length) {
    return NULL;
  }
  Vector* args = NewVector();
  for (size_t i = start; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal == NULL || formal->type == NULL ||
        TypeContainsAuto(formal->type) ||
        TypeContainsTemplateParameter(formal->type)) {
      VectorDeleteWithContents(
          args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      return NULL;
    }
    TemplateArgument* arg = TemplateArgumentAlloc();
    arg->kind = kTemplateParameterType;
    arg->type = TypeRecordCopy(formal->type);
    arg->location = formal->location;
    VectorAppend(args, arg);
  }
  return args;
}

// Contract: Resolves dependent type template args for trait folding; may allocate
// substituted type (caller must delete when distinct from arg->type).
static TypeRecord* CXXResolveTypeForIsSameVArgument(TemplateArgument* arg) {
  if (arg == NULL || arg->type == NULL || TypeIsUnknown(arg->type)) {
    return NULL;
  }
  TypeRecord* type = arg->type;
  if (!TypeContainsTemplateParameter(type)) {
    return type;
  }
  TypeRecord* from_formal = CXXTypeFromLambdaInventedParameter(type);
  if (from_formal != NULL && !TypeContainsTemplateParameter(from_formal)) {
    return from_formal;
  }
  TypeRecord* func = compiler->current_function;
  Vector* lambda_args = CXXLambdaInventedTemplateArgumentValues(func);
  if (lambda_args != NULL) {
    TypeRecord* substituted = TypeSubstituteTemplateTypeOrFail(
        &compiler->syntax, TypeRecordCopy(type), lambda_args);
    VectorDeleteWithContents(
        lambda_args, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    if (substituted != NULL && !TypeIsUnknown(substituted) &&
        !TypeContainsTemplateParameter(substituted)) {
      return substituted;
    }
    TypeRecordDelete(substituted);
  }
  if (func == NULL || func->template_arguments == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(func->template_arguments)) {
    return type;
  }
  TypeRecord* substituted = TypeSubstituteTemplateTypeOrFail(
      &compiler->syntax, TypeRecordCopy(type), func->template_arguments);
  if (substituted == NULL || TypeIsUnknown(substituted) ||
      TypeContainsTemplateParameter(substituted)) {
    TypeRecordDelete(substituted);
    return type;
  }
  return substituted;
}

// A clone copies the source node's parent pointer, and the clone callback runs
// before ASTNodeClone links the copy into its new parent.  Replacing
// `parent`'s child then deletes the template pattern (`is_same_v<T, Base>`
// becomes the first instantiation's constant, and every later instantiation
// clones that).  Splice only when this node is actually in the slot.
// Contract: Read-only parent/child consistency check before trait replacement.
static bool ParentSlotHoldsNode(ASTNode* parent, int child_id, ASTNode* node) {
  if (parent == NULL || node == NULL) {
    return false;
  }
  switch (ASTNodeGetShape(parent)) {
    case kASTShapeUnary:
      return child_id == 0 && ((UnaryASTNode*)parent)->sub == node;
    case kASTShapeBinary:
      if (child_id == 0) {
        return ((BinaryASTNode*)parent)->left == node;
      }
      if (child_id == 1) {
        return ((BinaryASTNode*)parent)->right == node;
      }
      return false;
    case kASTShapeCombined:
      if (child_id == 0) {
        return ((CombinedStatementASTNode*)parent)->cond == node;
      }
      if (child_id == 1) {
        return ((CombinedStatementASTNode*)parent)->stmt == node;
      }
      return false;
    case kASTShapeIf:
      if (child_id == 0) {
        return ((IfStatementASTNode*)parent)->cond == node;
      }
      if (child_id == 1) {
        return ((IfStatementASTNode*)parent)->if_part == node;
      }
      if (child_id == 2) {
        return ((IfStatementASTNode*)parent)->else_part == node;
      }
      return false;
    case kASTShapeExprStmt: {
      ExpressionStatementASTNode* stmt = (ExpressionStatementASTNode*)parent;
      return child_id == 0 && stmt->expr == node;
    }
    default:
      return true;
  }
}

// Contract: Replaces `node` in parent when slot matches; returns new constant.
static ASTNode* CXXNewBoolConstantReplacing(ASTNode* node, int64_t same) {
  TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
  ASTNode* const_node = NewIntConstantASTNode(same, bool_type, node->location);
  TypeRecordDelete(bool_type);
  if (node->parent != NULL &&
      ParentSlotHoldsNode(node->parent, node->child_id, node)) {
    ASTNodeReplaceChild(node->parent, node->child_id, const_node, true);
  }
  const_node->flags |= kASTAnalyzed;
  return const_node;
}

// Contract: Wrapper around `CXXNewBoolConstantReplacing` for trait identifiers.
static ASTNode* CXXNewIsSameBoolConstant(IdentifierASTNode* node, int64_t same) {
  return CXXNewBoolConstantReplacing(&node->base, same);
}

// Contract: Read-only; true when two types name the same template parameter.
static bool CXXIsSameTemplateParameterType(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (left->template_parameter_index >= 0 &&
      left->template_parameter_index == right->template_parameter_index &&
      left->template_parameter_name != NULL &&
      right->template_parameter_name != NULL &&
      StringEqual(left->template_parameter_name, right->template_parameter_name)) {
    return true;
  }
  return TypeEqualIgnoringQualifiers(left, right);
}

// Contract: Folds `is_same` class template to 0/1; -1 if still dependent.
static int64_t CXXFoldIsSameSpecializationToBool(TypeRecord* materialized) {
  if (materialized == NULL || materialized->template_arguments == NULL ||
      materialized->template_arguments->length != 2) {
    return -1;
  }
  TemplateArgument* left_arg = materialized->template_arguments->value.p[0];
  TemplateArgument* right_arg = materialized->template_arguments->value.p[1];
  if (left_arg == NULL || right_arg == NULL || left_arg->type == NULL ||
      right_arg->type == NULL) {
    return -1;
  }
  if (CXXIsSameTemplateParameterType(left_arg->type, right_arg->type)) {
    return 1;
  }
  TypeRecord* left_type = CXXResolveTypeForIsSameVArgument(left_arg);
  TypeRecord* right_type = CXXResolveTypeForIsSameVArgument(right_arg);
  bool copied_left = left_type != NULL && left_type != left_arg->type;
  bool copied_right = right_type != NULL && right_type != right_arg->type;
  int64_t result = -1;
  if (left_type != NULL && right_type != NULL &&
      !TypeContainsTemplateParameter(left_type) &&
      !TypeContainsTemplateParameter(right_type)) {
    Qualifiers cv_mask =
        kQualConst | kQualVolatile | kQualRestrict | kQualAtomic;
    result = TypeEqualIgnoringTopLevelQualifierMask(left_type, right_type,
                                                      cv_mask)
                   ? 1
                   : 0;
  }
  if (copied_left) {
    TypeRecordDelete(left_type);
  }
  if (copied_right) {
    TypeRecordDelete(right_type);
  }
  return result;
}

// Contract: Read-only type predicate for trait folding (excludes dependent types).
static bool CXXTypeIsStdArithmeticType(TypeRecord* type) {
  return type != NULL && !TypeContainsTemplateParameter(type) &&
         (TypeIsIntegral(type) || TypeIsFloatingPoint(type));
}

// Contract: Folds `is_base_of` specialization to 0/1; -1 when dependent.
static int64_t CXXFoldIsBaseOfSpecializationToBool(TypeRecord* materialized) {
  if (materialized == NULL || materialized->template_arguments == NULL ||
      materialized->template_arguments->length != 2) {
    return -1;
  }
  TemplateArgument* base_arg = materialized->template_arguments->value.p[0];
  TemplateArgument* derived_arg = materialized->template_arguments->value.p[1];
  if (base_arg == NULL || derived_arg == NULL || base_arg->type == NULL ||
      derived_arg->type == NULL) {
    return -1;
  }
  TypeRecord* base_type = CXXResolveTypeForIsSameVArgument(base_arg);
  TypeRecord* derived_type = CXXResolveTypeForIsSameVArgument(derived_arg);
  bool copied_base =
      base_type != NULL && base_type != base_arg->type;
  bool copied_derived =
      derived_type != NULL && derived_type != derived_arg->type;
  int64_t result = -1;
  if (base_type != NULL && derived_type != NULL &&
      !TypeContainsTemplateParameter(base_type) &&
      !TypeContainsTemplateParameter(derived_type)) {
    Qualifiers cv_mask =
        kQualConst | kQualVolatile | kQualRestrict | kQualAtomic;
    if (TypeEqualIgnoringTopLevelQualifierMask(base_type, derived_type,
                                                 cv_mask)) {
      result = base_type->size > 0 && derived_type->size > 0 ? 1 : 0;
    } else if (TypeIsStructOrUnion(base_type) &&
               TypeIsStructOrUnion(derived_type)) {
      result =
          StructIsDerivedFrom(derived_type->info.struct_info,
                              base_type->info.struct_info,
                              /*public_only=*/false)
              ? 1
              : 0;
    } else {
      result = 0;
    }
  }
  if (copied_base) {
    TypeRecordDelete(base_type);
  }
  if (copied_derived) {
    TypeRecordDelete(derived_type);
  }
  return result;
}

// Contract: Analyzes type operand under constant depth; may replace dot with bool
// constant via `CXXNewBoolConstantReplacing`. Deletes temporary type_expr.
static ASTNode* CXXTryFoldTraitValueDot(BinaryASTNode* dot) {
  if (dot == NULL || dot->right == NULL ||
      dot->right->op != AST_OP(structmember)) {
    return NULL;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)dot->right;
  if (member_node->member == NULL || member_node->member->symbol == NULL ||
      !StringEqual(&member_node->member->symbol->name, "value")) {
    return NULL;
  }
  dot->left->flags &= ~kASTAnalyzed;
  int saved_depth = compiler->constant_evaluation_required_depth;
  compiler->constant_evaluation_required_depth++;
  ASTNode* type_expr = AnalyzeExpression(dot->left);
  compiler->constant_evaluation_required_depth = saved_depth;
  if (type_expr == NULL || type_expr->type == NULL ||
      !TypeIsStructOrUnion(type_expr->type) ||
      type_expr->type->info.struct_info == NULL) {
    if (type_expr != dot->left) {
      ASTNodeDelete(type_expr);
    }
    return NULL;
  }
  Symbol* origin = CXXStructTemplateOrigin(type_expr->type->info.struct_info);
  if (origin == NULL) {
    if (type_expr != dot->left) {
      ASTNodeDelete(type_expr);
    }
    return NULL;
  }
  TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
      &compiler->syntax, type_expr->type);
  int64_t folded = -1;
  if (StringEqual(&origin->name, "is_same")) {
    folded = CXXFoldIsSameSpecializationToBool(materialized);
  } else if (StringEqual(&origin->name, "is_base_of")) {
    folded = CXXFoldIsBaseOfSpecializationToBool(materialized);
  } else if (StringEqual(&origin->name, "is_arithmetic") &&
             materialized->template_arguments != NULL &&
             materialized->template_arguments->length == 1) {
    TemplateArgument* arg = materialized->template_arguments->value.p[0];
    TypeRecord* type = arg != NULL ? CXXResolveTypeForIsSameVArgument(arg) : NULL;
    bool copied =
        type != NULL && arg != NULL && arg->type != NULL && type != arg->type;
    if (type != NULL && !TypeContainsTemplateParameter(type)) {
      folded = CXXTypeIsStdArithmeticType(type) ? 1 : 0;
    }
    if (copied) {
      TypeRecordDelete(type);
    }
  }
  if (type_expr != dot->left) {
    ASTNodeDelete(type_expr);
  }
  if (folded >= 0) {
    return CXXNewBoolConstantReplacing(&dot->base, folded);
  }
  return NULL;
}

// Contract: Alias for `CXXTryFoldTraitValueDot` on `::value` member access.
static ASTNode* CXXTryFoldIsSameValueDot(BinaryASTNode* dot) {
  return CXXTryFoldTraitValueDot(dot);
}

// `std::is_same_v<T, U>` is a variable template whose initializer is
// `is_same<T, U>::value`.  Instantiating that in a generic lambda body can fail
// even when both type arguments are already concrete (capturing closures are
// analyzed before `auto` parameters are deduced at the call site).  Fold the
// trait directly when both sides are known types.
// Contract: May replace identifier use with bool constant; defers when either type
// is still dependent (avoids wrong `if constexpr` branch deletion).
static ASTNode* CXXTryFoldIsSameVIdentifier(IdentifierASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->symbol == NULL ||
      node->template_arguments == NULL ||
      (node->base.flags & kASTIsDeclaration) != 0 ||
      node->template_arguments->length != 2 ||
      !StringEqual(&node->symbol->name, "is_same_v")) {
    return NULL;
  }
  TemplateArgument* left_arg = node->template_arguments->value.p[0];
  TemplateArgument* right_arg = node->template_arguments->value.p[1];
  TypeRecord* left_type = CXXResolveTypeForIsSameVArgument(left_arg);
  TypeRecord* right_type = CXXResolveTypeForIsSameVArgument(right_arg);
  bool copied_left = left_type != NULL && left_arg != NULL &&
                     left_type != left_arg->type;
  bool copied_right = right_type != NULL && right_arg != NULL &&
                      right_type != right_arg->type;
  if (left_arg != NULL && right_arg != NULL && left_arg->type != NULL &&
      right_arg->type != NULL &&
      CXXIsSameTemplateParameterType(left_arg->type, right_arg->type)) {
    if (copied_left) {
      TypeRecordDelete(left_type);
    }
    if (copied_right) {
      TypeRecordDelete(right_type);
    }
    return CXXNewIsSameBoolConstant(node, 1);
  }
  if (left_type != NULL && right_type != NULL &&
      !TypeContainsTemplateParameter(left_type) &&
      !TypeContainsTemplateParameter(right_type)) {
    Qualifiers cv_mask =
        kQualConst | kQualVolatile | kQualRestrict | kQualAtomic;
    int64_t same =
        TypeEqualIgnoringTopLevelQualifierMask(left_type, right_type, cv_mask)
            ? 1
            : 0;
    if (copied_left) {
      TypeRecordDelete(left_type);
    }
    if (copied_right) {
      TypeRecordDelete(right_type);
    }
    return CXXNewIsSameBoolConstant(node, same);
  }
  if (copied_left) {
    TypeRecordDelete(left_type);
  }
  if (copied_right) {
    TypeRecordDelete(right_type);
  }
  // One side is still a template parameter.  `is_same<T, U>::value` is the
  // primary template (`false`) until both arguments are concrete, and folding
  // that into `if constexpr` deletes a branch from the function template body.
  return NULL;
}

// `std::is_base_of_v<Base, Derived>` is a variable template whose initializer
// is `is_base_of<Base, Derived>::value`.  Instantiating the primary trait
// without folding yields `false` for a real derived class, so
// `HasDeprecatedArenaConstructor<RepeatedPtrField<T>>` takes the wrong
// constructor.
// Contract: Folds to bool constant when base/derived types are concrete; else NULL.
static ASTNode* CXXTryFoldIsBaseOfVIdentifier(IdentifierASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->symbol == NULL ||
      node->template_arguments == NULL ||
      (node->base.flags & kASTIsDeclaration) != 0 ||
      node->template_arguments->length != 2 ||
      !StringEqual(&node->symbol->name, "is_base_of_v")) {
    return NULL;
  }
  TemplateArgument* base_arg = node->template_arguments->value.p[0];
  TemplateArgument* derived_arg = node->template_arguments->value.p[1];
  TypeRecord* base_type = CXXResolveTypeForIsSameVArgument(base_arg);
  TypeRecord* derived_type = CXXResolveTypeForIsSameVArgument(derived_arg);
  bool copied_base = base_type != NULL && base_arg != NULL &&
                     base_type != base_arg->type;
  bool copied_derived = derived_type != NULL && derived_arg != NULL &&
                        derived_type != derived_arg->type;
  ASTNode* folded = NULL;
  if (base_type != NULL && derived_type != NULL &&
      !TypeContainsTemplateParameter(base_type) &&
      !TypeContainsTemplateParameter(derived_type)) {
    Qualifiers cv_mask =
        kQualConst | kQualVolatile | kQualRestrict | kQualAtomic;
    int64_t result = 0;
    if (TypeEqualIgnoringTopLevelQualifierMask(base_type, derived_type,
                                                cv_mask)) {
      result = base_type->size > 0 && derived_type->size > 0 ? 1 : 0;
    } else if (TypeIsStructOrUnion(base_type) &&
               TypeIsStructOrUnion(derived_type)) {
      result = StructIsDerivedFrom(derived_type->info.struct_info,
                                   base_type->info.struct_info,
                                   /*public_only=*/false)
                   ? 1
                   : 0;
    }
    folded = CXXNewIsSameBoolConstant(node, result);
  }
  if (copied_base) {
    TypeRecordDelete(base_type);
  }
  if (copied_derived) {
    TypeRecordDelete(derived_type);
  }
  return folded;
}

// Contract: Folds `is_arithmetic_v` when type arg is concrete; may replace id.
static ASTNode* CXXTryFoldIsArithmeticVIdentifier(IdentifierASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->symbol == NULL ||
      node->template_arguments == NULL ||
      (node->base.flags & kASTIsDeclaration) != 0 ||
      node->template_arguments->length != 1 ||
      !StringEqual(&node->symbol->name, "is_arithmetic_v")) {
    return NULL;
  }
  TemplateArgument* arg = node->template_arguments->value.p[0];
  TypeRecord* type = CXXResolveTypeForIsSameVArgument(arg);
  bool copied = type != NULL && arg != NULL && arg->type != NULL &&
                type != arg->type;
  if (type == NULL || TypeContainsTemplateParameter(type)) {
    if (copied) {
      TypeRecordDelete(type);
    }
    return NULL;
  }
  int64_t value = CXXTypeIsStdArithmeticType(type) ? 1 : 0;
  if (copied) {
    TypeRecordDelete(type);
  }
  return CXXNewIsSameBoolConstant(node, value);
}

// `kUsesPointer<T>` is `!std::is_arithmetic_v<T>`.  Returns 0 or 1, or -1 when
// the argument is still dependent.
// Contract: Read-only fold helper; returns -1 when type arg is still dependent.
static int CXXKUsesPointerConstant(Vector* template_arguments) {
  if (!CompilerIsCXX() || template_arguments == NULL ||
      template_arguments->length != 1) {
    return -1;
  }
  TemplateArgument* arg = template_arguments->value.p[0];
  TypeRecord* type = CXXResolveTypeForIsSameVArgument(arg);
  bool copied = type != NULL && arg != NULL && arg->type != NULL &&
                type != arg->type;
  if (type == NULL || TypeContainsTemplateParameter(type)) {
    if (copied) {
      TypeRecordDelete(type);
    }
    return -1;
  }
  int value = CXXTypeIsStdArithmeticType(type) ? 0 : 1;
  if (copied) {
    TypeRecordDelete(type);
  }
  return value;
}

// Contract: Folds Abseil `kUsesPointer` variable template to bool constant.
static ASTNode* CXXTryFoldKUsesPointerTemplate(IdentifierASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->symbol == NULL ||
      node->template_arguments == NULL ||
      (node->base.flags & kASTIsDeclaration) != 0 ||
      node->template_arguments->length != 1 ||
      !StringEqual(&node->symbol->name, "kUsesPointer")) {
    return NULL;
  }
  TemplateArgument* arg = node->template_arguments->value.p[0];
  TypeRecord* type = CXXResolveTypeForIsSameVArgument(arg);
  bool copied = type != NULL && arg != NULL && arg->type != NULL &&
                type != arg->type;
  if (type == NULL || TypeContainsTemplateParameter(type)) {
    if (copied) {
      TypeRecordDelete(type);
    }
    return NULL;
  }
  int64_t value = CXXTypeIsStdArithmeticType(type) ? 0 : 1;
  if (copied) {
    TypeRecordDelete(type);
  }
  return CXXNewIsSameBoolConstant(node, value);
}

// Contract: Read-only inheritance walk for protobuf Arena folding heuristics.
static bool CXXStructDerivesFromProtobufMessageLite(Struct* str) {
  if (str == NULL) {
    return false;
  }
  if (str->tag_name != NULL && StringEqual(str->tag_name, "MessageLite")) {
    return true;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base != NULL && base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        CXXStructDerivesFromProtobufMessageLite(base->type->info.struct_info)) {
      return true;
    }
  }
  return false;
}

// Contract: Type-level wrapper for `CXXStructDerivesFromProtobufMessageLite`.
static bool CXXTypeIsProtobufMessageLiteOrDerived(TypeRecord* type) {
  return type != NULL && TypeIsStructOrUnion(type) &&
         type->info.struct_info != NULL &&
         CXXStructDerivesFromProtobufMessageLite(type->info.struct_info);
}

// Contract: Read-only; length of template argument pack (or trailing pack expansion).
static size_t CXXTemplateArgumentPackLength(Vector* template_args,
                                            size_t pack_index) {
  if (template_args == NULL || pack_index >= template_args->length) {
    return 0;
  }
  TemplateArgument* arg = template_args->value.p[pack_index];
  if (arg != NULL && arg->pack_arguments != NULL) {
    return arg->pack_arguments->length;
  }
  return template_args->length - pack_index;
}

// Contract: Resolves pack element type; may allocate via `CXXResolveTypeForIsSameVArgument`.
static TypeRecord* CXXTemplateArgumentPackTypeAt(Vector* template_args,
                                                 size_t pack_index,
                                                 size_t element_index) {
  if (template_args == NULL || pack_index >= template_args->length) {
    return NULL;
  }
  TemplateArgument* arg = template_args->value.p[pack_index];
  if (arg != NULL && arg->pack_arguments != NULL) {
    if (element_index >= arg->pack_arguments->length) {
      return NULL;
    }
    TemplateArgument* elem = arg->pack_arguments->value.p[element_index];
    return elem != NULL ? CXXResolveTypeForIsSameVArgument(elem) : NULL;
  }
  size_t idx = pack_index + element_index;
  if (idx >= template_args->length) {
    return NULL;
  }
  return CXXResolveTypeForIsSameVArgument(template_args->value.p[idx]);
}

// `Arena::GetConstructType` uses `decltype(ProbeConstructType<T>(
// std::declval<Args>()...))::value`.
// Contract: Read-only Arena construct-type probe; -1 when args still dependent.
static int64_t CXXFoldProbeConstructTypeFromArgs(Vector* template_args) {
  size_t pack_len = CXXTemplateArgumentPackLength(template_args, 1);
  if (pack_len == 0) {
    return 1;  // ConstructType::kDefault
  }
  if (pack_len > 1) {
    return 0;  // ConstructType::kUnknown
  }
  TemplateArgument* pack_arg = template_args->value.p[1];
  TypeRecord* arg_type = CXXTemplateArgumentPackTypeAt(template_args, 1, 0);
  bool copied_arg = arg_type != NULL && pack_arg != NULL &&
                      pack_arg->type != NULL && arg_type != pack_arg->type;
  if (pack_arg != NULL && pack_arg->pack_arguments != NULL &&
      pack_arg->pack_arguments->length > 0) {
    pack_arg = pack_arg->pack_arguments->value.p[0];
    copied_arg = arg_type != NULL && pack_arg != NULL &&
                 pack_arg->type != NULL && arg_type != pack_arg->type;
  }
  if (arg_type == NULL || TypeContainsTemplateParameter(arg_type)) {
    if (copied_arg) {
      TypeRecordDelete(arg_type);
    }
    return -1;
  }
  int64_t result = 2;  // ConstructType::kCopy
  if (TypeIsReference(arg_type) &&
      arg_type->declarator == kDeclRValueReference &&
      (arg_type->qualifiers & kQualConst) == 0) {
    result = 3;  // ConstructType::kMove
  }
  if (copied_arg) {
    TypeRecordDelete(arg_type);
  }
  return result;
}

// Contract: Extracts identifier from call callee (incl. `obj.member`); read-only.
static IdentifierASTNode* CXXCallCalleeIdentifier(ASTNode* callee) {
  if (callee == NULL) {
    return NULL;
  }
  if (callee->op == AST_OP(identifier)) {
    return (IdentifierASTNode*)callee;
  }
  if (callee->op == AST_OP(dot)) {
    BinaryASTNode* dot = (BinaryASTNode*)callee;
    if (dot->right != NULL && dot->right->op == AST_OP(identifier)) {
      return (IdentifierASTNode*)dot->right;
    }
  }
  return NULL;
}

// Contract: Copies/substitutes template args for compile-time trait folding;
// caller owns returned vector.
static Vector* CXXSubstituteTemplateArgumentsForFold(Vector* template_arguments) {
  if (template_arguments == NULL) {
    return NULL;
  }
  Vector* args = TemplateArgumentVectorCopy(template_arguments);
  if (args == NULL) {
    return NULL;
  }
  Vector* lambda_args =
      CXXLambdaInventedTemplateArgumentValues(compiler->current_function);
  if (lambda_args != NULL) {
    bool failed = false;
    Vector* substituted = TypeSubstituteTemplateArgumentVectorOrFail(
        &compiler->syntax, args, lambda_args, &failed);
    VectorDeleteWithContents(
        lambda_args, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    if (substituted != NULL && !failed) {
      VectorDeleteWithContents(
          args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      args = substituted;
    } else if (substituted != NULL) {
      VectorDeleteWithContents(
          substituted, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
  }
  TypeRecord* func = compiler->current_function;
  if (func != NULL && func->template_arguments != NULL &&
      !TemplateArgumentVectorContainsTemplateParameter(
          func->template_arguments)) {
    bool failed = false;
    Vector* substituted = TypeSubstituteTemplateArgumentVectorOrFail(
        &compiler->syntax, args, func->template_arguments, &failed);
    if (substituted != NULL && !failed) {
      VectorDeleteWithContents(
          args, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      args = substituted;
    } else if (substituted != NULL) {
      VectorDeleteWithContents(
          substituted, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
  }
  return args;
}

// Contract: Read-only tuple type-name check (primary or `tuple<...>` origin).
static bool CXXTypeNamesStdTuple(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  if (type->info.struct_info->tag_name != NULL &&
      StringEqual(type->info.struct_info->tag_name, "tuple")) {
    return true;
  }
  if (type->template_origin != NULL &&
      StringEqual(&type->template_origin->name, "tuple")) {
    return true;
  }
  return false;
}

// Contract: Peels wrappers to braced-init tuple CTAD argument; no mutation.
static BracedInitializerASTNode* CXXTupleArgumentBracedInit(ASTNode* arg) {
  while (arg != NULL && arg->op == AST_OP(cast)) {
    arg = ((CastASTNode*)arg)->expr;
  }
  if (arg == NULL) {
    return NULL;
  }
  if (arg->op == AST_OP(expr_init)) {
    return CXXTupleArgumentBracedInit(
        ((ExpressionInitializerASTNode*)arg)->expr);
  }
  if (arg->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)arg;
    TypeRecord* ctor_type = call->base.type;
    if (ctor_type == NULL && call->left != NULL) {
      ctor_type = call->left->type;
    }
    if (CXXTypeNamesStdTuple(ctor_type) && call->children != NULL &&
        call->children->length == 1) {
      return CXXTupleArgumentBracedInit(call->children->value.p[0]);
    }
  }
  if (arg->op == AST_OP(braced_init)) {
    return (BracedInitializerASTNode*)arg;
  }
  if (arg->op == AST_OP(compound_literal)) {
    ASTNode* init = ((CompoundLiteralASTNode*)arg)->initializer;
    if (init != NULL && init->op == AST_OP(braced_init)) {
      return (BracedInitializerASTNode*)init;
    }
  }
  if (arg->op == AST_OP(comma)) {
    return CXXTupleArgumentBracedInit(((BinaryASTNode*)arg)->right);
  }
  return NULL;
}

// Contract: Returns braced-init element expression; read-only on braced node.
static ASTNode* CXXBracedInitElementExpr(BracedInitializerASTNode* braced,
                                        size_t index) {
  if (braced == NULL || braced->initializers == NULL ||
      index >= braced->initializers->length) {
    return NULL;
  }
  ASTNode* element = braced->initializers->value.p[index];
  if (element != NULL && element->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)element)->expr;
  }
  return element;
}

// Contract: Replaces `std::get` on tuple braced-init with element subtree;
// deletes call node. Returns analyzed element or NULL to defer.
ASTNode* CXXTryFoldStdGetFromTupleCall(VectorASTNode* call) {
  // AnalyzeFunctionCall may constant-fold the call into a non-call node
  // (a number, for example).  The second fold in AnalyzeExpression runs on
  // that result, so this must not treat an arbitrary node as a call.
  if (call == NULL || call->base.op != AST_OP(call)) {
    return NULL;
  }
  IdentifierASTNode* callee = CXXCallCalleeIdentifier(call->left);
  if (callee == NULL || callee->symbol == NULL ||
      !StringEqual(&callee->symbol->name, "get") ||
      callee->template_arguments == NULL ||
      callee->template_arguments->length != 1 ||
      call->children == NULL || call->children->length != 1) {
    return NULL;
  }
  Vector* resolved =
      CXXSubstituteTemplateArgumentsForFold(callee->template_arguments);
  if (resolved == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(resolved)) {
    VectorDeleteWithContents(
        resolved, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    return NULL;
  }
  TemplateArgument* t_arg = resolved->value.p[0];
  size_t index = (size_t)-1;
  TypeRecord* type_key = NULL;
  if (TemplateArgumentConcreteValueKind(t_arg) == kTemplateValueIntegral) {
    index = (size_t)t_arg->int_value;
  } else if (t_arg->dependent_expr != NULL) {
    int64_t iv = 0;
    if (EvaluateIntegerExpression(t_arg->dependent_expr, &iv)) {
      index = (size_t)iv;
    }
  } else if (t_arg->kind == kTemplateParameterNonType && t_arg->int_value >= 0) {
    index = (size_t)t_arg->int_value;
  } else if (t_arg->type != NULL) {
    type_key = CXXResolveTypeForIsSameVArgument(t_arg);
  }
  ASTNode* tuple_arg = (ASTNode*)call->children->value.p[0];
  if (tuple_arg != NULL && tuple_arg->op == AST_OP(expr_init)) {
    tuple_arg = ((ExpressionInitializerASTNode*)tuple_arg)->expr;
  }
  BracedInitializerASTNode* braced = CXXTupleArgumentBracedInit(tuple_arg);
  if (braced == NULL) {
    if (type_key != NULL) {
      TypeRecordDelete(type_key);
    }
    VectorDeleteWithContents(
        resolved, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    return NULL;
  }
  ASTNode* element = NULL;
  if (type_key != NULL) {
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* candidate = CXXBracedInitElementExpr(braced, i);
      if (candidate == NULL) {
        continue;
      }
      candidate = AnalyzeExpression(candidate);
      if (candidate->type != NULL &&
          (TypeEqual(candidate->type, type_key) ||
           (TypeIsMemberPointer(candidate->type) &&
            TypeIsMemberPointer(type_key) &&
            TypeEqual(TypeMemberPointerPointeeType(candidate->type),
                      TypeMemberPointerPointeeType(type_key))))) {
        element = candidate;
        break;
      }
    }
    TypeRecordDelete(type_key);
  } else if (index != (size_t)-1) {
    element = CXXBracedInitElementExpr(braced, index);
  }
  VectorDeleteWithContents(resolved,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  if (element == NULL) {
    return NULL;
  }
  ASTNode* moved = ASTNodeMove(element);
  moved = AnalyzeExpression(moved);
  // The callee may still be the primary `get` overload selected while the
  // tuple was an uninstantiated CTAD placeholder (`T get(integer_sequence)`).
  // That return type is the template parameter, not the element.  Keep the
  // element's type in that case; a concrete get<> return type still wins.
  if (call->base.type != NULL &&
      !TypeContainsTemplateParameter(call->base.type) &&
      !TypeIsUnknown(call->base.type)) {
    ASTNodeSetType(moved, TypeRecordCopy(call->base.type));
  }
  ASTNodeReplaceChild(call->base.parent, call->base.child_id, moved, true);
  moved->flags |= kASTAnalyzed;
  moved->value_category = kValueCategoryPrvalue;
  return moved;
}

// Contract: Folds protobuf `GetConstructType` to integer constant when args known.
static ASTNode* CXXTryFoldGetConstructTypeCall(VectorASTNode* call) {
  if (call == NULL) {
    return NULL;
  }
  IdentifierASTNode* callee = CXXCallCalleeIdentifier(call->left);
  if (callee == NULL || callee->symbol == NULL ||
      !StringEqual(&callee->symbol->name, "GetConstructType") ||
      callee->template_arguments == NULL ||
      callee->template_arguments->length < 1) {
    return NULL;
  }
  Vector* resolved =
      CXXSubstituteTemplateArgumentsForFold(callee->template_arguments);
  if (resolved == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(resolved)) {
    VectorDeleteWithContents(
        resolved, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    return NULL;
  }
  TemplateArgument* t_arg = resolved->value.p[0];
  TypeRecord* t_type = CXXResolveTypeForIsSameVArgument(t_arg);
  bool copied_t =
      t_type != NULL && t_arg != NULL && t_arg->type != NULL &&
      t_type != t_arg->type;
  if (t_type == NULL || TypeContainsTemplateParameter(t_type)) {
    if (copied_t) {
      TypeRecordDelete(t_type);
    }
    VectorDeleteWithContents(
        resolved, (VectorElementDestructor)TemplateArgumentDelete,
        /*free_element=*/false);
    return NULL;
  }
  int64_t value = 0;
  if (CXXTypeIsProtobufMessageLiteOrDerived(t_type)) {
    int64_t probed = CXXFoldProbeConstructTypeFromArgs(resolved);
    if (probed < 0) {
      if (copied_t) {
        TypeRecordDelete(t_type);
      }
      VectorDeleteWithContents(
          resolved, (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
      return NULL;
    }
    value = probed;
  }
  if (copied_t) {
    TypeRecordDelete(t_type);
  }
  VectorDeleteWithContents(resolved,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return CXXNewBoolConstantReplacing(&call->base, value);
}

// Contract: Normalizes variable-template value type (strip ref/cv); caller owns copy.
static TypeRecord* CXXVariableTemplateValueType(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  TypeRecord* normalized = type;
  while (normalized != NULL && TypeIsReference(normalized)) {
    normalized = normalized->next;
  }
  if (normalized == NULL) {
    return NULL;
  }
  TypeRecord* copy = TypeRecordCopy(normalized);
  copy->qualifiers &= ~(kQualConst | kQualVolatile);
  return copy;
}

// Contract: Peels init/cast wrappers; read-only pointer into existing AST.
static ASTNode* CXXVariableTemplateExpressionSurface(ASTNode* expr) {
  while (expr != NULL) {
    if (expr->op == AST_OP(init)) {
      expr = ((BinaryASTNode*)expr)->right;
      continue;
    }
    if (expr->op == AST_OP(expr_init)) {
      expr = ((ExpressionInitializerASTNode*)expr)->expr;
      continue;
    }
    if (expr->op == AST_OP(cast)) {
      expr = ((CastASTNode*)expr)->expr;
      continue;
    }
    break;
  }
  return expr;
}

// Contract: Lookup `tuple` class template symbol; read-only.
static Symbol* CXXStdTupleClassTemplateSymbol(void) {
  String name;
  StringInit(&name, "tuple");
  Symbol* sym = SyntaxFindSymbol(&compiler->syntax, &name);
  if (sym == NULL || !sym->flags.is_template) {
    sym = SyntaxFindTag(&compiler->syntax, &name);
  }
  StringDestruct(&name);
  if (sym != NULL && sym->flags.is_template) {
    return sym;
  }
  return NULL;
}

// Contract: CTAD/instantiate `tuple<T>` for single-element braced init; caller
// owns returned type or NULL.
static TypeRecord* CXXTupleTypeForSingleElementBracedInit(
    BracedInitializerASTNode* braced) {
  if (braced == NULL || braced->initializers == NULL ||
      braced->initializers->length != 1) {
    return NULL;
  }
  if (braced->base.type != NULL && CXXTypeNamesStdTuple(braced->base.type)) {
    return TypeRecordCopy(braced->base.type);
  }
  ASTNode* element = CXXBracedInitElementExpr(braced, 0);
  if (element == NULL) {
    return NULL;
  }
  DiagnosticSuppressBegin();
  element = AnalyzeExpression(element);
  DiagnosticSuppressEnd();
  if (element == NULL || element->type == NULL ||
      TypeContainsTemplateParameter(element->type)) {
    return NULL;
  }
  Symbol* tuple_template = CXXStdTupleClassTemplateSymbol();
  if (tuple_template == NULL || !tuple_template->flags.is_template) {
    return NULL;
  }
  Vector* args = NewVector();
  VectorAppend(args, NewTypeTemplateArgument(TypeRecordCopy(element->type)));
  TypeRecord* tuple_type =
      TypeInstantiateClassTemplate(&compiler->syntax, tuple_template, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  return tuple_type;
}

// Contract: Lowers braced tuple argument on `get` call before var-template eval;
// may replace call child via `LowerCXXBracedInitToTarget`.
static void CXXPrepareStdGetVariableTemplateInitializer(ASTNode* expr) {
  expr = CXXVariableTemplateExpressionSurface(expr);
  if (expr == NULL || expr->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)expr;
  IdentifierASTNode* callee = CXXCallCalleeIdentifier(call->left);
  if (callee == NULL || callee->symbol == NULL ||
      !StringEqual(&callee->symbol->name, "get") ||
      call->children == NULL || call->children->length != 1) {
    return;
  }
  ASTNode* arg = call->children->value.p[0];
  if (arg != NULL && arg->op == AST_OP(expr_init)) {
    arg = ((ExpressionInitializerASTNode*)arg)->expr;
  }
  if (arg == NULL || arg->op != AST_OP(braced_init)) {
    return;
  }
  TypeRecord* tuple_type =
      CXXTupleTypeForSingleElementBracedInit((BracedInitializerASTNode*)arg);
  if (tuple_type == NULL) {
    return;
  }
  ASTNode* lowered = LowerCXXBracedInitToTarget(arg, tuple_type);
  TypeRecordDelete(tuple_type);
  if (lowered != arg) {
    ASTNodeReplaceChild((ASTNode*)call, 0, lowered, false);
  }
}

// Contract: Materializes variable-template use as constant or analyzed expression;
// caller owns returned node. May analyze/instantiate and run tuple/get folds.
static ASTNode* CXXInstantiateVariableTemplateValue(Symbol* var_sym,
                                                    Vector* template_args,
                                                    SourceLocation location) {
  if (!CompilerIsCXX() || var_sym == NULL ||
      var_sym->variable_template == NULL || template_args == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(template_args)) {
    return NULL;
  }
  TypeRecord* concrete = TypeInstantiateVariableTemplateDeducedType(
      &compiler->syntax, var_sym, template_args);
  if (concrete != NULL && var_sym->type != NULL &&
      TypeContainsAuto(var_sym->type) && !TypeIsMemberPointer(concrete)) {
    TypeRecordDelete(concrete);
    concrete = NULL;
  }
  int64_t value = 0;
  if (concrete != NULL && TypeIsIntegral(concrete) &&
      TypeInstantiateVariableTemplateConstant(&compiler->syntax, var_sym,
                                              template_args, &value)) {
    return NewIntConstantASTNode(value, concrete, location);
  }
  if (StringEqual(&var_sym->name, "is_same_v") ||
      StringEqual(&var_sym->name, "is_base_of_v")) {
    IdentifierASTNode id = {0};
    id.base.location = location;
    id.symbol = var_sym;
    id.template_arguments = template_args;
    ASTNode* folded = CXXTryFoldIsSameVIdentifier(&id);
    if (folded == NULL) {
      folded = CXXTryFoldIsBaseOfVIdentifier(&id);
    }
    if (folded != NULL) {
      TypeRecordDelete(concrete);
      return folded;
    }
  }
  double floating_value = 0;
  if (concrete != NULL && TypeIsFloatingPoint(concrete) &&
      TypeInstantiateVariableTemplateFloatingConstant(
          &compiler->syntax, var_sym, template_args, &floating_value)) {
    return NewRealConstantASTNode(floating_value, concrete, location);
  }
  if (concrete != NULL && TypeUsesLongDoubleRepresentation(concrete) &&
      (concrete->qualifiers & kQualConst) != 0) {
    ASTNode* initializer = TypeInstantiateVariableTemplateInitializer(
        &compiler->syntax, var_sym, template_args);
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
    }
    if (initializer != NULL) {
      TypeRecord* value_type = CXXVariableTemplateValueType(concrete);
      ASTNode* cast = NewCastASTNode(value_type, location, initializer);
      TypeRecordDelete(value_type);
      TypeRecordDelete(concrete);
      return AnalyzeExpression(cast);
    }
  }
  if (concrete != NULL && TypeIsStructOrUnion(concrete)) {
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, concrete);
    temp->location = location;
    ASTNode* temp_id = NewIdentifierASTNode(temp, location);
    temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
    Vector* initializer_values = NewVector();
    Struct* value_class = concrete->info.struct_info;
    bool has_object_state = false;
    for (size_t i = 0; value_class != NULL && i < value_class->members.length;
         ++i) {
      StructMember* member = value_class->members.value.p[i];
      if (member != NULL && member->symbol != NULL && !member->is_static &&
          !member->is_member_function &&
          !StorageIs(member->symbol->storage, STO(typedef))) {
        has_object_state = true;
        break;
      }
    }
    if (has_object_state) {
      ASTNode* concrete_initializer = TypeInstantiateVariableTemplateInitializer(
          &compiler->syntax, var_sym, template_args);
      if (concrete_initializer != NULL) {
        VectorAppend(initializer_values, concrete_initializer);
      }
    }
    ASTNode* initializer =
        NewBracedInitializerASTNode(initializer_values, NULL, location);
    ASTNode* literal = NewCompoundLiteralASTNode(temp_id, location, initializer);
    TypeRecordDelete(concrete);
    ASTNode* analyzed = AnalyzeExpression(literal);
    analyzed->value_category = kValueCategoryLvalue;
    return analyzed;
  }
  if (concrete != NULL && TypeIsMemberPointer(concrete)) {
    ASTNode* initializer = TypeInstantiateVariableTemplateInitializer(
        &compiler->syntax, var_sym, template_args);
    TypeRecordDelete(concrete);
    if (initializer != NULL) {
      initializer = CXXVariableTemplateExpressionSurface(initializer);
      initializer = CXXFoldIsSameVExpression(initializer);
      CXXPrepareStdGetVariableTemplateInitializer(initializer);
      ASTNode* value = AnalyzeExpression(initializer);
      if (value != NULL && value->op == AST_OP(call)) {
        ASTNode* folded = CXXTryFoldStdGetFromTupleCall((VectorASTNode*)value);
        if (folded != NULL) {
          value = folded;
        }
      }
      if (value != NULL && value->type != NULL) {
        TypeRecord* value_type = CXXVariableTemplateValueType(value->type);
        if (value_type != NULL) {
          ASTNodeSetType(value, value_type);
        }
        value->value_category = kValueCategoryPrvalue;
        return value;
      }
    }
    return NULL;
  }
  if (concrete == NULL) {
    ASTNode* initializer = TypeInstantiateVariableTemplateInitializer(
        &compiler->syntax, var_sym, template_args);
    if (initializer != NULL) {
      initializer = CXXVariableTemplateExpressionSurface(initializer);
      initializer = CXXFoldIsSameVExpression(initializer);
      CXXPrepareStdGetVariableTemplateInitializer(initializer);
      ASTNode* value = AnalyzeExpression(initializer);
      if (value != NULL && value->op == AST_OP(call)) {
        ASTNode* folded = CXXTryFoldStdGetFromTupleCall((VectorASTNode*)value);
        if (folded != NULL) {
          value = folded;
        }
      }
      if (value != NULL && value->type != NULL &&
          !TypeContainsTemplateParameter(value->type) &&
          !TypeIsUnknown(value->type)) {
        TypeRecord* value_type = CXXVariableTemplateValueType(value->type);
        if (value_type != NULL) {
          ASTNodeSetType(value, value_type);
        }
        value->value_category = kValueCategoryPrvalue;
        return value;
      }
    }
  }
  TypeRecordDelete(concrete);
  return NULL;
}

// Contract: Returns a new analyzed constant/expression for a fully substituted
// variable template use, or NULL when args remain dependent. Does not splice;
// caller replaces the use site.
ASTNode* CXXMaterializeVariableTemplateExpression(ASTNode* expr) {
  if (expr == NULL) {
    return NULL;
  }
  expr = CXXVariableTemplateExpressionSurface(expr);
  if (expr == NULL) {
    return NULL;
  }
  if (expr->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)expr;
    if (id->symbol != NULL && id->symbol->variable_template != NULL &&
        id->template_arguments != NULL &&
        (expr->flags & kASTIsDeclaration) == 0 &&
        !TemplateArgumentVectorContainsTemplateParameter(
            id->template_arguments)) {
      return CXXInstantiateVariableTemplateValue(
          id->symbol, id->template_arguments, expr->location);
    }
    return NULL;
  }
  if (expr->op == AST_OP(structmember)) {
    StructMemberASTNode* member_node = (StructMemberASTNode*)expr;
    StructMember* member = member_node->member;
    if (member != NULL && member->is_static &&
        member->symbol != NULL && member->symbol->variable_template != NULL &&
        member_node->template_arguments != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            member_node->template_arguments)) {
      return CXXInstantiateVariableTemplateValue(
          member->symbol, member_node->template_arguments, expr->location);
    }
    return NULL;
  }
  if (expr->op == AST_OP(dot) || expr->op == AST_OP(arrow)) {
    BinaryASTNode* access = (BinaryASTNode*)expr;
    if (access->right == NULL || access->right->op != AST_OP(structmember)) {
      return NULL;
    }
    StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
    StructMember* member = member_node->member;
    if (member != NULL && member->is_static &&
        member->symbol != NULL && member->symbol->variable_template != NULL &&
        member_node->template_arguments != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            member_node->template_arguments)) {
      return CXXInstantiateVariableTemplateValue(
          member->symbol, member_node->template_arguments, expr->location);
    }
  }
  return NULL;
}

// Contract: Splices instantiated variable-template value over `use_site`; deletes old.
ASTNode* CXXTryReplaceWithVariableTemplateValue(
    ASTNode* use_site, Symbol* var_sym, Vector* template_args,
    SourceLocation location) {
  ASTNode* materialized =
      CXXInstantiateVariableTemplateValue(var_sym, template_args, location);
  if (materialized == NULL || use_site == NULL || use_site->parent == NULL) {
    return NULL;
  }
  ASTNodeReplaceChild(use_site->parent, use_site->child_id, materialized, true);
  materialized->flags |= kASTAnalyzed;
  return materialized;
}

// C++26 name-independent lookup: diagnose ambiguous references and stop with a
// typed lvalue placeholder.  Returns non-NULL when the pipeline should stop.
static ASTNode* AnalyzeIdentifierHandleCXX26NameIndependentAmbiguity(
    IdentifierASTNode* node) {
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) && node->symbol != NULL &&
      (node->base.flags & kASTNameIndependentLookupAmbiguous) != 0 &&
      (node->base.flags & kASTIsDeclaration) == 0) {
    SemanticError(&node->base,
                  "reference to name-independent declaration '%s' is ambiguous",
                  node->symbol->name.value);
    ASTNodeSetType(&node->base, TypeRecordCopy(node->symbol->type));
    node->base.value_category = kValueCategoryLvalue;
    return &node->base;
  }
  return NULL;
}

// `main` is not a valid expression name; emit the diagnostic but continue analysis.
static void AnalyzeIdentifierDiagnoseMainInExpression(IdentifierASTNode* node) {
  if (CompilerIsCXX() && node->symbol != NULL &&
      (node->base.flags & kASTIsDeclaration) == 0 &&
      StringEqual(&node->symbol->name, "main") &&
      node->symbol->namespace_ == NULL &&
      TypeIsFunction(node->symbol->type) &&
      node->symbol->type->info.function.cxx_member_owner == NULL) {
    SemanticError(&node->base,
                  "the function 'main' cannot be named by an expression");
  }
}

// A dependent qualified value name (`T::member`) that still carries its flag
// here was never resolved during template instantiation.  Returns non-NULL when
// typing/diagnostics are complete and the pipeline should stop.
static ASTNode* AnalyzeIdentifierHandleDependentQualifiedName(
    IdentifierASTNode* node) {
  if ((node->base.flags & kASTDependentQualifiedName) != 0 &&
      node->symbol != NULL && node->symbol->type != NULL &&
      node->symbol->type->dependent_member_name != NULL) {
    if (TypeContainsTemplateParameter(node->symbol->type)) {
      // The placeholder type is the dependent scope (`allocator_traits<A>`),
      // not the member (`::value`).  A constexpr bool cannot convert from that
      // class; keep an unknown value type until instantiation resolves it.
      ASTNodeSetType(&node->base,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
      return &node->base;
    }
    SemanticError(&node->base, "no member named '%s' in the dependent scope",
                  node->symbol->type->dependent_member_name->value);
    ASTNodeSetType(&node->base, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return &node->base;
  }
  return NULL;
}

// A concept-id used as a value is a prvalue of type bool; fold or diagnose and
// replace the identifier in its parent.  Returns non-NULL when done.
static ASTNode* AnalyzeIdentifierFoldConceptId(IdentifierASTNode* node) {
  if (!CompilerIsCXX() || node->symbol == NULL ||
      !node->symbol->flags.is_concept ||
      node->symbol->concept_definition == NULL ||
      (node->base.flags & kASTIsDeclaration) != 0 ||
      TemplateArgumentVectorContainsTemplateParameter(
          node->template_arguments)) {
    return NULL;
  }
  int64_t value = 0;
  if (ConceptsEvaluateInteger(&node->base, &value)) {
    TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
    ASTNode* const_node =
        NewIntConstantASTNode(value != 0, bool_type, node->base.location);
    TypeRecordDelete(bool_type);
    ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node,
                        true);
    const_node->flags |= kASTAnalyzed;
    return const_node;
  }
  if (DiagnosticsSuppressed()) {
    TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
    ASTNodeSetType(&node->base, bool_type);
    TypeRecordDelete(bool_type);
    node->base.value_category = kValueCategoryPrvalue;
    return &node->base;
  }
  SemanticError(&node->base,
                "concept-id is not a constant expression of type bool");
  TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
  ASTNode* const_node =
      NewIntConstantASTNode(0, bool_type, node->base.location);
  TypeRecordDelete(bool_type);
  ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node, true);
  const_node->flags |= kASTAnalyzed;
  return const_node;
}

// Compile-time folds for well-known variable templates (`is_same_v`, etc.)
// before generic variable-template instantiation.  Returns the replacement node
// or NULL to continue.
static ASTNode* AnalyzeIdentifierFoldVariableTemplateTraits(
    IdentifierASTNode* node) {
  ASTNode* folded = CXXTryFoldIsSameVIdentifier(node);
  if (folded == NULL) {
    folded = CXXTryFoldIsBaseOfVIdentifier(node);
  }
  if (folded == NULL) {
    folded = CXXTryFoldIsArithmeticVIdentifier(node);
  }
  if (folded == NULL) {
    folded = CXXTryFoldKUsesPointerTemplate(node);
  }
  return folded;
}

// Instantiate a fully substituted variable template-id and splice the value
// over the use site.  Returns non-NULL when materialization succeeded.
static ASTNode* AnalyzeIdentifierMaterializeVariableTemplate(
    IdentifierASTNode* node) {
  if (!CompilerIsCXX() || node->symbol == NULL ||
      node->symbol->variable_template == NULL ||
      node->template_arguments == NULL ||
      (node->base.flags & kASTIsDeclaration) != 0 ||
      TemplateArgumentVectorContainsTemplateParameter(
          node->template_arguments)) {
    return NULL;
  }
  ASTNode* materialized = CXXInstantiateVariableTemplateValue(
      node->symbol, node->template_arguments, node->base.location);
  if (materialized == NULL) {
    return NULL;
  }
  ASTNodeReplaceChild(node->base.parent, node->base.child_id, materialized,
                      true);
  materialized->flags |= kASTAnalyzed;
  return materialized;
}

// When a template-id names exactly one function specialization outside a call,
// bind `node->symbol` to that instantiation (non-call uses and `&fn<T>`).
static void AnalyzeIdentifierInstantiateUniqueFunctionTemplateId(
    IdentifierASTNode* node) {
  if (!CompilerIsCXX() || node->symbol == NULL ||
      !(node->symbol->flags.is_template || node->symbol->flags.is_overloaded) ||
      DiagnosticsSuppressed() || !TypeIsFunction(node->symbol->type) ||
      (compiler->current_function != NULL &&
       compiler->current_function->info.function.symbol != NULL &&
       compiler->current_function->info.function.symbol->flags.is_template) ||
      node->base.parent == NULL ||
      (node->base.parent->op == AST_OP(call) &&
       ((VectorASTNode*)node->base.parent)->left == &node->base) ||
      node->template_arguments == NULL ||
      TemplateArgumentVectorContainsTemplateParameter(
          node->template_arguments)) {
    return;
  }
  // A template-id naming a single specialization is that function, with or
  // without `&`: `ForEach(list, Delete<T>)` deduces a pointer to Delete<T>.
  // A direct call's callee is left to overload resolution with its actuals.
  // An overloaded set (`InvokeObject` has an object form and a nontype
  // form) cannot be instantiated from the head alone.  Explicit arguments
  // select at most one primary; a later conversion still checks the
  // destination function type when this is ambiguous.
  Symbol* winner = NULL;
  int matches = 0;
  for (Symbol* cand = node->symbol; cand != NULL; cand = cand->overload_next) {
    if (!cand->flags.is_template || cand->type == NULL ||
        !TypeIsFunction(cand->type)) {
      continue;
    }
    // Probe under suppression so a non-matching overload (a type argument
    // in a non-type parameter slot) does not diagnose.  Speculative depth
    // skips cloning the body; the unique winner is instantiated again below
    // so its definition is queued with diagnostics enabled.
    compiler->speculative_template_instantiation_depth++;
    DiagnosticSuppressBegin();
    Symbol* inst = TypeInstantiateFunctionTemplate(
        &compiler->syntax, cand, node->template_arguments);
    DiagnosticSuppressEnd();
    compiler->speculative_template_instantiation_depth--;
    if (inst == NULL || inst == cand || inst->flags.is_template) {
      continue;
    }
    winner = cand;
    matches++;
  }
  if (matches == 1 && winner != NULL) {
    Symbol* instantiated = TypeInstantiateFunctionTemplate(
        &compiler->syntax, winner, node->template_arguments);
    if (instantiated != NULL && instantiated != winner &&
        !instantiated->flags.is_template) {
      node->symbol = instantiated;
      CXXAnalyzeImmediateEscalationCandidate(instantiated);
    }
  }
}

// Mark the symbol used/read and emit deprecated warnings (skipped on init LHS).
static void AnalyzeIdentifierTrackSymbolUseAndDeprecated(
    IdentifierASTNode* node) {
  if (node->base.parent != NULL && node->base.parent->op == AST_OP(init)) {
    return;
  }
  // Symbol has now been used.
  node->symbol->flags.used = true;

  // Track whether the value is ever *read*, to distinguish variables that are
  // only ever written (-Wunused-but-set-variable).  Every reference counts as
  // a read except when the identifier is the direct target of a plain `=`
  // assignment (its old value is discarded).  Compound assignments (`+=`),
  // increments/decrements, address-of and any nested use all read the value
  // and are therefore left to mark it read.
  if (node->base.parent == NULL ||
      node->base.parent->op != AST_OP(assign) || node->base.child_id != 0) {
    node->symbol->is_read = true;
  }

  // Warn about uses of a symbol marked deprecated by either spelling.
  if ((node->base.flags & kASTIsDeclaration) == 0) {
    Attribute* dep = SymbolFindAttribute(node->symbol, "deprecated");
    if (dep != NULL) {
      const char* msg = AttributeArgString(dep, 0);
      bool emitted = false;
      if (msg != NULL) {
        // The argument token retains its surrounding double quotes; trim them
        // for a cleaner message.
        char trimmed[256];
        size_t len = strlen(msg);
        if (len >= 2 && msg[0] == '"' && msg[len - 1] == '"') {
          size_t inner = len - 2;
          if (inner >= sizeof(trimmed)) {
            inner = sizeof(trimmed) - 1;
          }
          memcpy(trimmed, msg + 1, inner);
          trimmed[inner] = '\0';
          msg = trimmed;
        }
        emitted = SemanticWarning(
            &node->base, "deprecated-declarations",
            "'%s' is deprecated: %s", node->symbol->name.value, msg);
      } else {
        emitted = SemanticWarning(
            &node->base, "deprecated-declarations",
            "'%s' is deprecated", node->symbol->name.value);
      }
      if (emitted) {
        SourceLocation location =
            dep->location != SOURCE_LOCATION_MISSING
                ? dep->location
                : node->symbol->location;
        SemanticNoteAtLocation(location,
                               "'%s' was marked deprecated here",
                               node->symbol->name.value);
      }
    }
  }
}

// Expression uses: lvalue category, reference decay, and contract-identifier views.
static void AnalyzeIdentifierSetNonDeclarationType(IdentifierASTNode* node) {
  if ((node->base.flags & kASTIsDeclaration) != 0) {
    return;
  }
  node->base.value_category = kValueCategoryLvalue;
  if (TypeIsReference(node->symbol->type)) {
    ASTNodeSetType(&node->base, node->symbol->type->next);
  } else {
    ASTNodeSetType(&node->base, node->symbol->type);
  }
  TypeRecord* contract_view =
      SemanticContractIdentifierViewType(node->symbol, node->base.type);
  if (contract_view != NULL) {
    ASTNodeSetType(&node->base, contract_view);
    TypeRecordDelete(contract_view);
  }
}

// Ordinary identifier path: glvalue address requirements, member template
// definition materialization, and scalar constant folding with the same early
// exits as before overload-resolution-sensitive contexts.
static ASTNode* AnalyzeIdentifierFinishValueCategoryAndFolding(
    IdentifierASTNode* node) {
  if (TypeIsStructOrUnion(node->base.type) || TypeIsArray(node->base.type) ||
      TypeIsFunction(node->base.type)) {
    if (CompilerIsCXX() && TypeIsFunction(node->base.type)) {
      bool has_implicit_this =
          node->symbol->type->info.function.prototype.length > 0 &&
          node->symbol->type->info.function.prototype.value.p[0] != NULL &&
          StringEqual(&((Symbol*)node->symbol->type->info.function.prototype
                            .value.p[0])
                           ->name,
                      "this");
      // Do not eagerly instantiate a member of an overload set here: the
      // correct overload is only known after overload resolution, which
      // instantiates the selected candidate itself. Instantiating an
      // arbitrary (e.g. first) overload can materialize an unused member of
      // a class template and trigger spurious errors (e.g. default-
      // constructing a non-default-constructible type).
      if (!has_implicit_this && !node->symbol->flags.is_overloaded &&
          !AnalyzingTemplatePatternBody()) {
        TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax,
                                                   node->symbol);
      }
    }
    node->base.flags |= kASTNeedAddress;
  } else {
    // If this is a declaration, don't try to fold it.
    if ((node->base.flags & kASTIsDeclaration) != 0) {
      return &node->base;
    }
    if ((node->base.flags & kASTNeedAddress) != 0) {
      return &node->base;
    }
    if (node->base.parent != NULL &&
        node->base.parent->op == AST_OP(address)) {
      return &node->base;
    }
    // A C++ argument is analyzed before overload resolution, and a reference
    // parameter must bind to the object itself.  AnalyzeFunctionCall folds
    // by-value arguments once the parameter types are known.
    if (CompilerIsCXX() && node->base.parent != NULL &&
        node->base.parent->op == AST_OP(call)) {
      return &node->base;
    }
    return FoldConstantIdentifier(node);
  }
  return &node->base;
}

// Contract: Types `node`, may fold to constants (except under address/call),
// materialize variable templates, and apply contract-identifier views. Returns
// `&node->base` (possibly after parent replacement from folding helpers).
ASTNode* AnalyzeIdentifier(IdentifierASTNode* node) {
  ASTNode* stopped = AnalyzeIdentifierHandleCXX26NameIndependentAmbiguity(node);
  if (stopped != NULL) {
    return stopped;
  }

  AnalyzeIdentifierDiagnoseMainInExpression(node);

  stopped = AnalyzeIdentifierHandleDependentQualifiedName(node);
  if (stopped != NULL) {
    return stopped;
  }

  stopped = AnalyzeIdentifierFoldConceptId(node);
  if (stopped != NULL) {
    return stopped;
  }

  stopped = AnalyzeIdentifierFoldVariableTemplateTraits(node);
  if (stopped != NULL) {
    return stopped;
  }

  stopped = AnalyzeIdentifierMaterializeVariableTemplate(node);
  if (stopped != NULL) {
    return stopped;
  }

  AnalyzeIdentifierInstantiateUniqueFunctionTemplateId(node);
  AnalyzeIdentifierTrackSymbolUseAndDeprecated(node);
  AnalyzeIdentifierSetNonDeclarationType(node);
  return AnalyzeIdentifierFinishValueCategoryAndFolding(node);
}

// Contract: Recursively folds trait/`is_same_v`-style subexpressions; may replace
// subtrees with bool constants via `ASTNodeReplaceChild`. Called before operator
// analysis so dependent folds do not run too late.
ASTNode* CXXFoldIsSameVExpression(ASTNode* expr) {
  if (expr == NULL) {
    return NULL;
  }
  if (expr->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* init = (ExpressionInitializerASTNode*)expr;
    init->expr = CXXFoldIsSameVExpression(init->expr);
    return expr;
  }
  if (expr->op == AST_OP(init)) {
    BinaryASTNode* init = (BinaryASTNode*)expr;
    init->right = CXXFoldIsSameVExpression(init->right);
    return expr;
  }
  if (expr->op == AST_OP(equal) || expr->op == AST_OP(noteq)) {
    BinaryASTNode* cmp = (BinaryASTNode*)expr;
    cmp->left = CXXFoldIsSameVExpression(cmp->left);
    cmp->right = CXXFoldIsSameVExpression(cmp->right);
    int64_t left_value = 0;
    int64_t right_value = 0;
    if (EvaluateIntegerExpression(cmp->left, &left_value) &&
        EvaluateIntegerExpression(cmp->right, &right_value)) {
      bool same = left_value == right_value;
      return CXXNewBoolConstantReplacing(
          expr, (expr->op == AST_OP(equal)) ? (same ? 1 : 0) : (same ? 0 : 1));
    }
    return expr;
  }
  if (expr->op == AST_OP(logand) || expr->op == AST_OP(logor)) {
    BinaryASTNode* logical = (BinaryASTNode*)expr;
    logical->left = CXXFoldIsSameVExpression(logical->left);
    logical->right = CXXFoldIsSameVExpression(logical->right);
    int64_t left_value = 0;
    int64_t right_value = 0;
    if (EvaluateIntegerExpression(logical->left, &left_value) &&
        EvaluateIntegerExpression(logical->right, &right_value)) {
      bool result = expr->op == AST_OP(logand)
                        ? (left_value != 0 && right_value != 0)
                        : (left_value != 0 || right_value != 0);
      return CXXNewBoolConstantReplacing(expr, result ? 1 : 0);
    }
    return expr;
  }
  if (expr->op == AST_OP(dot)) {
    BinaryASTNode* dot = (BinaryASTNode*)expr;
    if (dot->right != NULL && dot->right->op == AST_OP(identifier)) {
      ASTNode* folded =
          CXXTryFoldKUsesPointerTemplate((IdentifierASTNode*)dot->right);
      if (folded != NULL) {
        return folded;
      }
    }
    // `Extension::kUsesPointer<T>` is a static member access, so the template
    // arguments live on the structmember rather than an identifier.
    if (dot->right != NULL && dot->right->op == AST_OP(structmember)) {
      StructMemberASTNode* member = (StructMemberASTNode*)dot->right;
      if (member->member != NULL && member->member->symbol != NULL &&
          StringEqual(&member->member->symbol->name, "kUsesPointer")) {
        int value = CXXKUsesPointerConstant(member->template_arguments);
        if (value >= 0) {
          return CXXNewBoolConstantReplacing(expr, value);
        }
      }
    }
    ASTNode* folded = CXXTryFoldIsSameValueDot(dot);
    if (folded != NULL) {
      return folded;
    }
    return expr;
  }
  if (expr->op == AST_OP(call)) {
    ASTNode* folded = CXXTryFoldGetConstructTypeCall((VectorASTNode*)expr);
    if (folded != NULL) {
      return folded;
    }
    folded = CXXTryFoldStdGetFromTupleCall((VectorASTNode*)expr);
    if (folded != NULL) {
      return folded;
    }
    return expr;
  }
  if (expr->op != AST_OP(identifier)) {
    return expr;
  }
  expr->flags &= ~kASTAnalyzed;
  IdentifierASTNode* id = (IdentifierASTNode*)expr;
  ASTNode* folded = CXXTryFoldIsSameVIdentifier(id);
  if (folded == NULL) {
    folded = CXXTryFoldIsBaseOfVIdentifier(id);
  }
  if (folded == NULL) {
    folded = CXXTryFoldIsArithmeticVIdentifier(id);
  }
  if (folded == NULL) {
    folded = CXXTryFoldKUsesPointerTemplate(id);
  }
  return folded != NULL ? folded : expr;
}
