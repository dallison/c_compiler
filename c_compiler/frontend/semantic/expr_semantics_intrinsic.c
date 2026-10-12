//
//  expr_semantics_intrinsic.c
//  c_compiler
//
//  Atomic, source-location, type-trait, and other builtins.
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

static TypeRecord* AtomicPointerPointee(VectorASTNode* node);
static TypeRecord* AtomicOperationValueType(TypeRecord* object_type);
static void ValidateAtomicTarget(VectorASTNode* node);
static bool AtomicFailureOrderAllowed(int success, int failure);
static TypeRecord* NewConstCharPointerType(void);

// Contract: Re-analyzes each child in place, sets `kASTNeedAddress` on all,
// and types the call as void.
void AnalyzeVarargsBuiltin1(VectorASTNode* args) {
  for (size_t i = 0; i < args->children->length; i++) {
    ASTNode* child = args->children->value.p[i];
    args->children->value.p[i] = AnalyzeExpression(child);
    child = args->children->value.p[i];
    SetNeedAddress(child);     // Need address of all of these.
  }
  ASTNodeSetType(&args->base, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

// Contract: Sets need-address on first arg; result type is second arg's type
// (second child not re-analyzed here).
void AnalyzeVarargsBuiltin2(VectorASTNode* args) {
  // Second arg is a constant whose type is set to the type of the arg.
  ASTNode* ap = args->children->value.p[0];
  SetNeedAddress(ap);  // Need address of ap arg.
  ASTNode* type_node = args->children->value.p[1];
  ASTNodeSetType(&args->base, type_node->type);  // Type is type of second arg.
}

// Contract: Validates first arg as object pointer; may error and return fallback type.
static TypeRecord* AtomicPointerPointee(VectorASTNode* node) {
  if (node->children->length == 0) {
    return NULL;
  }
  ASTNode* ptr = node->children->value.p[0];
  if (ptr == NULL || !TypeIsPointer(ptr->type) || ptr->type->next == NULL ||
      TypeIsVoid(ptr->type->next)) {
    SemanticError((ASTNode*)node,
                  "atomic builtin requires pointer to object type");
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  if (CompilerTargetSupportsAtomics() &&
      !CompilerTargetSupportsAtomicSize(ptr->type->next->size)) {
    SemanticError((ASTNode*)node,
                  "atomic object size is not supported on this target");
  }
  if (!TypeIsIntegral(ptr->type->next) &&
      !TypeIsFloatingPoint(ptr->type->next) &&
      !TypeIsPointer(ptr->type->next)) {
    SemanticError((ASTNode*)node,
                  "atomic builtin requires scalar or pointer object type");
  }
  return ptr->type->next;
}

// Contract: Returns unqualified copy of atomic pointee type; caller owns.
static TypeRecord* AtomicOperationValueType(TypeRecord* object_type) {
  TypeRecord* value_type = TypeRecordCopy(object_type);
  if (value_type != NULL) {
    value_type->qualifiers &=
        ~(kQualAtomic | kQualConst | kQualVolatile | kQualRestrict);
  }
  return value_type;
}

// Contract: Target capability check; emits errors only.
static void ValidateAtomicTarget(VectorASTNode* node) {
  if (!CompilerTargetSupportsAtomics()) {
    if (compiler != NULL && compiler->target_name != NULL &&
        (StringEqual(compiler->target_name, "6502") ||
         StringEqual(compiler->target_name, "65c02"))) {
      SemanticError((ASTNode*)node,
                    "atomic operations are unavailable in the single-threaded "
                    "65(C)02 profile");
    } else {
      SemanticError((ASTNode*)node,
                    "atomic operations are unavailable on this target");
    }
  }
}

// Contract: Validates target support, then analyzes every child in place.
void AnalyzeAtomicBuiltinChildren(VectorASTNode* node) {
  ValidateAtomicTarget(node);
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression((ASTNode*)node->children->value.p[i]);
  }
}

// Contract: Diagnoses invalid constant orders; does not mutate `order`.
int ValidateAtomicMemoryOrder(ASTNode* order, AtomicOrderUse use) {
  int64_t value = -1;
  if (order == NULL || !TypeIsIntegral(order->type)) {
    SemanticError(order, "atomic memory order must have integral type");
    return 5;
  }
  // GCC permits a runtime memory-model expression and conservatively maps it
  // to sequential consistency.  Validate all constants precisely; dynamic
  // values retain that documented seq_cst fallback in target lowering.
  if (!EvaluateIntegerExpression(order, &value)) {
    return 5;
  }
  if (value < 0 || value > 5) {
    SemanticError(order, "atomic memory order must be a constant from 0 to 5");
    return 5;
  }
  if (use == kAtomicOrderLoad && (value == 3 || value == 4)) {
    SemanticError(order, "atomic load cannot use release or acq_rel order");
  } else if (use == kAtomicOrderStore &&
             (value == 1 || value == 2 || value == 4)) {
    SemanticError(order,
                  "atomic store cannot use consume, acquire, or acq_rel order");
  } else if (use == kAtomicOrderFailure && (value == 3 || value == 4)) {
    SemanticError(order,
                  "atomic compare-exchange failure order cannot be release or acq_rel");
  }
  return (int)value;
}

// Contract: Read-only C++ memory-order compatibility table for cmpxchg failure order.
static bool AtomicFailureOrderAllowed(int success, int failure) {
  if (failure == 0) {
    return true;
  }
  if (failure == 1) {
    return success == 1 || success == 2 || success == 4 || success == 5;
  }
  if (failure == 2) {
    return success == 2 || success == 4 || success == 5;
  }
  return failure == 5 && success == 5;
}

// Contract: Analyzes children, then sets prvalue result type to the pointee
// (non-atomic, unqualified copy of the atomic object type).
void AnalyzeAtomicLoadBuiltin(VectorASTNode* node) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
  if (node->children->length > 1) {
    ValidateAtomicMemoryOrder(node->children->value.p[1], kAtomicOrderLoad);
  }
  ASTNodeSetType(&node->base, value_type);
}

// Contract: Converts value operand to pointee type; result type void. Deletes
// temporary `value_type` after use.
void AnalyzeAtomicStoreBuiltin(VectorASTNode* node) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
  if (node->children->length > 1) {
    NormalConversion(node->children->value.p[1], value_type);
  }
  if (node->children->length > 2) {
    ValidateAtomicMemoryOrder(node->children->value.p[2], kAtomicOrderStore);
  }
  ASTNodeSetType(&node->base, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  TypeRecordDelete(value_type);
}

// Contract: May replace pointer arithmetic operand with `kASTPtrScale` node;
// result type is the unqualified pointee type (`returns_new` unused).
void AnalyzeAtomicFetchBuiltin(VectorASTNode* node, bool returns_new) {
  (void)returns_new;
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
  if (!TypeIsIntegral(value_type) && !TypeIsPointer(value_type)) {
    SemanticError((ASTNode*)node,
                  "atomic arithmetic builtin requires integral or pointer type");
  }
  if (node->children->length > 1) {
    ASTNode* operand = node->children->value.p[1];
    if (TypeIsPointer(value_type)) {
      if (!TypeIsIntegral(operand->type)) {
        SemanticError(operand,
                      "atomic pointer arithmetic requires an integer operand");
      } else {
        ASTNode* scale =
            NewPtrScaleASTNode(value_type->next, AST_OP(mult), operand,
                               operand->location);
        scale->parent = (ASTNode*)node;
        node->children->value.p[1] = scale;
        ASTNodeSetType(scale, value_type);
      }
    } else {
      NormalConversion(operand, value_type);
    }
  }
  if (node->children->length > 2) {
    ValidateAtomicMemoryOrder(node->children->value.p[2],
                              kAtomicOrderReadModifyWrite);
  }
  ASTNodeSetType(&node->base, value_type);
}

// Contract: Normalizes expected/desired operands; validates weak/order args when
// `expected_is_pointer`. Result bool or pointee per `returns_bool`.
void AnalyzeAtomicCompareExchangeBuiltin(VectorASTNode* node,
                                                bool expected_is_pointer,
                                                bool returns_bool) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type =
      AtomicOperationValueType(AtomicPointerPointee(node));
  if (node->children->length > 1) {
    ASTNode* expected = node->children->value.p[1];
    if (expected_is_pointer) {
      if (expected == NULL || !TypeIsPointer(expected->type) ||
          expected->type->next == NULL ||
          !TypeEqual(expected->type->next, value_type)) {
        SemanticError(expected != NULL ? expected : (ASTNode*)node,
                      "atomic compare exchange expected argument has incompatible type");
      }
    } else {
      NormalConversion(expected, value_type);
    }
  }
  if (node->children->length > 2) {
    NormalConversion(node->children->value.p[2], value_type);
  }
  if (expected_is_pointer && node->children->length > 5) {
    int64_t weak = -1;
    ASTNode* weak_node = node->children->value.p[3];
    if (!EvaluateIntegerExpression(weak_node, &weak) ||
        (weak != 0 && weak != 1)) {
      SemanticError(weak_node,
                    "atomic compare-exchange weak argument must be constant 0 or 1");
    }
    int success = ValidateAtomicMemoryOrder(
        node->children->value.p[4], kAtomicOrderReadModifyWrite);
    int failure = ValidateAtomicMemoryOrder(
        node->children->value.p[5], kAtomicOrderFailure);
    if (!AtomicFailureOrderAllowed(success, failure)) {
      SemanticError(node->children->value.p[5],
                    "atomic compare-exchange failure order is stronger than success order");
    }
  }
  if (returns_bool) {
    ASTNodeSetType(&node->base,
                   NewTypeRecordWithSize(kTypeBool, kQualPlain));
    TypeRecordDelete(value_type);
  } else {
    ASTNodeSetType(&node->base, value_type);
  }
}

// Contract: Allocates const char* type for source-location builtins.
static TypeRecord* NewConstCharPointerType(void) {
  TypeRecord* char_type = NewTypeRecordWithSize(kTypeChar, kQualConst);
  TypeRecord* pointer_type = NewPointerTo(kQualPlain, char_type);
  return TypeRecordCalculateSize(pointer_type);
}

// Contract: Sets result type only; children unchanged (source location builtins).
void AnalyzeSourceStringBuiltin(VectorASTNode* node) {
  ASTNodeSetType(&node->base, NewConstCharPointerType());
}

// Contract: Sets result type only; children unchanged.
void AnalyzeSourceIntegerBuiltin(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualPlain));
}

// Contract: Analyzes the single operand when present; result typed as int
// (constant folding of the answer happens elsewhere).
void AnalyzeBuiltinConstantP(VectorASTNode* node) {
  if (node->children != NULL && node->children->length == 1) {
    node->children->value.p[0] =
        AnalyzeExpression(node->children->value.p[0]);
  } else {
    SemanticError((ASTNode*)node,
                  "__builtin_constant_p requires one argument");
  }
  // The result is an int constant: 1 when the operand is an integer constant
  // expression, otherwise 0.  Zero is a correct answer when the compiler
  // cannot prove the operand is constant.
  ASTNodeSetType(&node->base, NewTypeRecordWithSize(kTypeInt, kQualPlain));
}

// Contract: Analyzes and promotes both operands to long; result type long.
void AnalyzeBuiltinExpect(VectorASTNode* node) {
  TypeRecord* result_type = NewTypeRecordWithSize(kTypeLong, kQualPlain);
  if (node->children->length != 2) {
    ASTNodeSetType(&node->base, result_type);
    return;
  }
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* child = node->children->value.p[i];
    child = AnalyzeExpression(child);
    node->children->value.p[i] = child;
    NormalConversion(child, NewTypeRecordWithSize(kTypeLong, kQualPlain));
  }
  ASTNodeSetType(&node->base, result_type);
}

// Contract: Analyzes children in place; result type follows first operand for
// rotate/bswap, otherwise int.
void AnalyzeBuiltinBitOperation(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression(node->children->value.p[i]);
  }
  if (node->children->length == 0) {
    ASTNodeSetType(&node->base,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  ASTNode* value = node->children->value.p[0];
  bool allow_signed = node->base.op == AST_OP(builtin_bswap);
  if (!TypeIsIntegral(value->type) ||
      (!allow_signed && !TypeIsUnsigned(value->type))) {
    SemanticError(value, "bit builtin requires an unsigned integer operand");
  }
  bool rotate = node->base.op == AST_OP(builtin_rotl) ||
                node->base.op == AST_OP(builtin_rotr) ||
                node->base.op == AST_OP(builtin_bswap);
  if (node->children->length == 2 &&
      !TypeIsIntegral(((ASTNode*)node->children->value.p[1])->type)) {
    SemanticError(node->children->value.p[1],
                  "bit builtin count or width must be an integer");
  }
  ASTNodeSetType(&node->base,
                 rotate ? TypeRecordCopy(value->type)
                        : NewTypeRecordWithSize(kTypeInt, kQualPlain));
}

// Contract: Analyzes children; optional args must be compile-time integers.
// Result type void.
void AnalyzeBuiltinPrefetch(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression(node->children->value.p[i]);
  }
  if (node->children->length >= 1) {
    ASTNode* address = node->children->value.p[0];
    if (!TypeIsPointerOrArray(address->type)) {
      SemanticError(address, "__builtin_prefetch address must be a pointer");
    }
  }
  for (size_t i = 1; i < node->children->length; i++) {
    ASTNode* option = node->children->value.p[i];
    int64_t value = -1;
    if (!TypeIsIntegral(option->type) ||
        !EvaluateIntegerExpression(option, &value)) {
      SemanticError(option,
                    "__builtin_prefetch optional arguments must be constant integers");
      continue;
    }
    if ((i == 1 && (value < 0 || value > 1)) ||
        (i == 2 && (value < 0 || value > 3))) {
      SemanticError(option,
                    "__builtin_prefetch argument is outside its valid range");
    }
  }
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

// Contract: Analyzes pointer operand; diagnoses complete implicit-lifetime
// aggregate pointee. Result type void.
void AnalyzeBuiltinStartLifetime(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression(node->children->value.p[i]);
  }
  ASTNode* operand =
      node->children->length == 1 ? node->children->value.p[0] : NULL;
  TypeRecord* target =
      operand != NULL && operand->type != NULL &&
              TypeIsPointer(operand->type)
          ? operand->type->next
          : NULL;
  if (target == NULL) {
    SemanticError(&node->base,
                  "__builtin_start_lifetime requires a pointer operand");
  } else if (target->size <= 0) {
    SemanticError(&node->base,
                  "__builtin_start_lifetime requires a complete type");
  } else if (!CXXTypeIsImplicitLifetimeAggregate(target)) {
    SemanticError(
        &node->base,
        "__builtin_start_lifetime requires an implicit-lifetime aggregate");
  }
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

// Contract: Result type void only; no operand analysis.
void AnalyzeBuiltinObservableCheckpoint(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

// Contract: Result type void only; no operand analysis.
void AnalyzeBuiltinTerminator(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

// Contract: Result bool prvalue; folding uses enclosing constant context elsewhere.
void AnalyzeBuiltinIsConstantEvaluated(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeBool, kQualPlain));
  node->base.value_category = kValueCategoryPrvalue;
}

// Contract: Dependent type arguments leave the builtin in place; otherwise may
// replace the call in its parent with an int constant (returns detached constant
// when parent is NULL). Frees substituted type-arg vector on all paths.
ASTNode* AnalyzeTypeTraitBuiltin(VectorASTNode* node) {
  SourceLocation location = node->base.location;
  if (node->children == NULL || node->children->length == 0) {
    return (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  ASTNode* kind_node = node->children->value.p[0];
  int64_t kind_value = 0;
  if (!EvaluateIntegerExpression(kind_node, &kind_value)) {
    SemanticError(&node->base, "Invalid type trait builtin");
    return (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  Vector type_args;
  VectorInit(&type_args);
  bool dependent = false;
  for (size_t i = 1; i < node->children->length; i++) {
    ASTNode* arg = node->children->value.p[i];
    if (arg == NULL || arg->type == NULL) {
      continue;
    }
    TypeRecord* type = TypeRecordCopy(arg->type);
    if (type != NULL) {
      dependent = dependent || TypeIsUnknown(type) ||
                  TypeContainsTemplateParameter(type);
      VectorAppend(&type_args, type);
    }
  }
  if (dependent) {
    VectorDestructWithContents(
        &type_args, (VectorElementDestructor)TypeRecordDelete,
        /*free_element=*/false);
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  bool value = CXXTypeTraitEvaluateBool(&compiler->syntax,
                                        (CXXTypeTraitKind)kind_value,
                                        &type_args);
  VectorDestructWithContents(&type_args, (VectorElementDestructor)TypeRecordDelete,
                             /*free_element=*/false);
  ASTNode* constant = (ASTNode*)NewIntConstantASTNode(
      value ? 1 : 0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  if (node->base.parent != NULL) {
    ASTNodeReplaceChild(node->base.parent, node->base.child_id, constant, false);
  }
  return constant;
}
