//
//  expr_semantics_operator.c
//  c_compiler
//
//  Built-in and overloaded operators, assignment, and subscripting.
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

// Classifies conditional-expression arms that can form function pointers.
typedef enum {
  kConditionalFunctionArmOther,
  kConditionalFunctionArmNull,
  kConditionalFunctionArmFunction,
  kConditionalFunctionArmFunctionPointer,
} ConditionalFunctionArmKind;

static bool IsNullPointer(ASTNode* node);
static bool TypeHasDoubleConversionRank(TypeRecord* type);
static int GetRank(TypeRecord* type);
static int GetFloatingSubrank(TypeRecord* type);
static int IntRank(void);
static Type ComplexArithmeticElementType(TypeRecord* left, TypeRecord* right);
static TypeRecord* NewVectorComparisonResultType(TypeRecord* vector_type);
static bool PromoteBitFieldOperand(ASTNode* operand);
static bool IsIntConstant(ASTNode* node);
static void DiagnoseCXX26EnumArithmetic(BinaryASTNode* node);
static ASTNode* ReplaceBinaryWithCall(BinaryASTNode* node, ASTNode* call);
static ASTNode* ReplaceUnaryWithCall(UnaryASTNode* node, ASTNode* call);
static ASTNode* NewOperatorFreeCall(Symbol* function, ASTNode* first_actual, Vector* remaining_actuals, SourceLocation location);
static bool VectorContainsPointer(Vector* vec, void* value);
static void ADLAddAssociatedNamespaceClosure(Vector* namespaces, Namespace* ns);
static TypeRecord* ADLCanonicalType(TypeRecord* type);
static void ADLCollectNamespacesForType(TypeRecord* type, Vector* namespaces, int depth);
static void AddCollectedFunctionSymbols(Vector* candidates, Vector* functions);
static const char* IncDecOperatorFunctionName(ASTOpcode op);
static bool IsPostIncDec(ASTOpcode op);
static ASTNode* NewPostfixDummyArgument(SourceLocation location);
static ASTNode* TryAnalyzeOverloadedIncDecOperator(UnaryASTNode* node);
static TypeRecord* CXXOperatorOperandClassType(TypeRecord* type);
static bool BestBinaryMemberOperatorScore(BinaryASTNode* node, const char* op_name, int* out_best_score, bool* out_ambiguous, bool* out_has_template);
static int FreeBinaryOperatorSecondOperandScore(Symbol* function, ASTNode* right);
static ASTNode* BuildMemberOperatorCall(BinaryASTNode* node, const char* op_name);
static TypeRecord* PointerArithmeticResultType(TypeRecord* pointer_type);
static bool ASTNodeIsPointerScaleMultiply(ASTNode* node);
static void PromoteShiftOperand(ASTNode* operand);
static bool IsZeroIntegerConstant(ASTNode* node);
static bool BinaryOperatorViableForOperands(ASTNode* left, ASTNode* right, const char* op_name);
static ASTNode* TryReversedComparisonOperator(BinaryASTNode* node);
static void CheckComparisonOfPointerAndInteger(BinaryASTNode* node);
static bool DiagnoseCXX26ArrayComparison(BinaryASTNode* node);
static int BestFreeBinaryOperatorScore(const char* op_name, ASTNode* left, ASTNode* right);
static ConditionalFunctionArmKind GetConditionalFunctionArmKind( ASTNode* arm, TypeRecord** function_type);
static bool TryAnalyzeConditionalFunctionPointer(BinaryASTNode* node, BinaryASTNode* colon);
static bool TryAnalyzeCConditionalObjectPointers(BinaryASTNode* node, BinaryASTNode* colon);
static bool TryAnalyzeConditionalDerivedToBaseGlvalue(BinaryASTNode* node, BinaryASTNode* colon);
static bool InsertNumericConversionsPhaseVector(BinaryASTNode* node);
static bool InsertNumericConversionsPhaseMemberPointer(BinaryASTNode* node);
static bool InsertNumericConversionsPhaseComplex(BinaryASTNode* node);
static void InsertNumericConversionsPhaseStructPointerOrScalar(
    BinaryASTNode* node, bool promote_to_int);
static void InsertNumericConversionsApplyUsualArithmeticConversions(
    BinaryASTNode* node, bool promote_to_int);
static bool AnalyzeComparisonOperatorFinishMemberPointerLeft(
    BinaryASTNode* node, ASTNode** result);
static bool AnalyzeComparisonOperatorFinishMemberPointerRightNull(
    BinaryASTNode* node, ASTNode** result);
static bool AnalyzeComparisonOperatorFinishNullptrTComparison(
    BinaryASTNode* node);
static ASTNode* AnalyzeComparisonOperatorFinishScalarComparison(
    BinaryASTNode* node);
static bool AnalyzeConditionalExpressionPhaseCondition(
    BinaryASTNode* node, BinaryASTNode** colon);
static bool AnalyzeConditionalExpressionPhaseColonArms(
    BinaryASTNode* node, BinaryASTNode* colon);
static bool AnalyzeConditionalExpressionPhaseThrowAndVoid(
    BinaryASTNode* node, BinaryASTNode* colon);
static bool AnalyzeConditionalExpressionPhaseClassArmConversions(
    BinaryASTNode* node, BinaryASTNode* colon);
static bool AnalyzeConditionalExpressionPhaseMatchingGlvalueCategory(
    BinaryASTNode* node, BinaryASTNode* colon);
static bool AnalyzeConditionalExpressionPhaseSameClassPrvalueInit(
    BinaryASTNode* node, BinaryASTNode* colon);
static bool AnalyzeConditionalExpressionPhaseMemberPointerAndMixedPointer(
    BinaryASTNode* node, BinaryASTNode* colon);
static void AnalyzeConditionalExpressionPhaseUsualArithmetic(
    BinaryASTNode* node, BinaryASTNode* colon);
static ASTNode* AnalyzeAssignmentExpressionPhaseDependentBracedInit(
    BinaryASTNode* node);
static bool AnalyzeAssignmentExpressionPhaseLowerBracedInitBeforeRhs(
    BinaryASTNode* node);
static ASTNode* AnalyzeAssignmentExpressionPhaseReferenceMemberInitializer(
    BinaryASTNode* node);
static void AnalyzeAssignmentExpressionPhaseCompoundAssignmentOps(
    BinaryASTNode* node);

// Contract: Read-only shape check for null pointer constant forms (including
// `?:` with both null arms and casts); does not mutate the AST.
static bool IsNullPointer(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number): {
      ConstantASTNode* c = (ConstantASTNode*)node;
      return c->value.ivalue == 0;
      }
    case AST_OP(question): {      // Conditional expression:
      node = ((BinaryASTNode*)node)->right;     // Colon.
      if (node == NULL || node->op != AST_OP(colon)) {
        return false;
      }
      ASTNode* left = ((BinaryASTNode*)node)->left;
      ASTNode* right = ((BinaryASTNode*)node)->right;
      return left != NULL && right != NULL &&
             IsNullPointer(left) && IsNullPointer(right);
    }
    case AST_OP(cast): {      // cast
      CastASTNode* c = (CastASTNode*)node;
      return c->expr != NULL && IsNullPointer(c->expr);
    }
    default:
    return false;
  }
}

// General analysis of a binary expression.  Does a recursive analysis
// of the chilren and then makes sure the types are scalar.  No binary
// expressions use structs or unions as operands (well the AST_OP(dot)
// and AST_OP(arrow) ones do, but those don't call this).
// This also sets the type of the node to that of the left child.  This
// will most likely be overwritten by the caller but it's safe to do it.
// Contract: Analyzes children in place; provisional type is left type only.
// Caller overwrites type after operator-specific rules.
void AnalyzeBinaryExpression(BinaryASTNode* node) {
  if (node == NULL) {
    return;
  }
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  if (!TypeIsVector(node->left->type)) {
    SemanticCheckScalarType(node->left);
  }
  if (!TypeIsVector(node->right->type)) {
    SemanticCheckScalarType(node->right);
  }
}

// Ranks for types.  Larger ranks are closer to the end
// of the array.  These are pointers to functions that return true
// if the type is of the requested value.
// Contract: Predicate for `type_ranks`; no side effects.
static bool TypeHasDoubleConversionRank(TypeRecord* type) {
  return TypeIsDouble(type) || TypeIsFloat64(type);
}

bool (*type_ranks[])(TypeRecord*) = {
    TypeIsBool,       TypeIsCharFamily, TypeIsShort, TypeIsInt,
    TypeIsLong,       TypeIsLongLong, TypeIsInt128,
    TypeUsesFloat32Representation,
    TypeHasDoubleConversionRank, TypeIsLongDouble, TypeIsVoid, NULL,
};

// Given a type, what is its rank.  According to the standard, types with higher
// precision are higher in rank, with _Bool being the lowest rank.  Floating
// point types have the highest rank.
// Contract: Pure numeric conversion rank; read-only on type records.
static int GetRank(TypeRecord* type) {
  if (TypeIsBitInt(type)) {
    return type->bit_width * 16;
  }
  if (TypeIsWchar(type)) {
    TypeRecord as_int = {
        .type = kTypeInt,
        .declarator = kDeclPrimitive,
        .size = type->size,
    };
    return GetRank(&as_int);
  }
  for (int i = 0; type_ranks[i] != NULL; i++) {
    if (type_ranks[i](type)) {
      if (TypeIsIntegral(type)) {
        // A standard integer type outranks _BitInt(N) at the same width, while
        // _BitInt(N+1) outranks every standard type no wider than N bits.
        return type->size * 8 * 16 + i + 1;
      }
      return 4096 + i + 1;
    }
  }
  return -1;
}

// C++23 gives the standard floating-point type the greater conversion subrank
// when an extended type has the same representation and rank.
// Contract: C++23 floating subrank for extended types; no mutation.
static int GetFloatingSubrank(TypeRecord* type) {
  if (TypeIsFloat(type) || TypeIsDouble(type)) {
    return 2;
  }
  if (TypeIsFloat32(type) || TypeIsFloat64(type)) {
    return 1;
  }
  return 0;
}

// Contract: Returns `GetRank` for plain `int`; used as promotion baseline.
static int IntRank(void) {
  TypeRecord int_type = {
      .type = kTypeInt,
      .declarator = kDeclPrimitive,
      .size = SizeofType(kTypeInt),
  };
  return GetRank(&int_type);
}

// Contract: Selects complex/floating element type for binary complex ops;
// read-only on operands' types.
static Type ComplexArithmeticElementType(TypeRecord* left,
                                         TypeRecord* right) {
  Type result = kTypeFloat;
  TypeRecord* operands[] = {left, right};
  for (size_t i = 0; i < sizeof(operands) / sizeof(operands[0]); i++) {
    TypeRecord* operand = operands[i];
    Type element =
        TypeIsComplex(operand) ? TypeComplexElementType(operand)
                               : TypeIsLongDouble(operand) ? kTypeLongDouble
                               : TypeIsDouble(operand) || TypeIsFloat64(operand)
                                   ? kTypeDouble
                               : TypeIsFloat(operand) || TypeIsFloat32(operand)
                                   ? kTypeFloat
                                   : kTypeImplicit;
    if (element == kTypeLongDouble) {
      return kTypeLongDouble;
    }
    if (element == kTypeDouble) {
      result = kTypeDouble;
    }
  }
  return result;
}

// Contract: Allocates bool/int logical result type per language; caller owns.
TypeRecord* NewLogicalResultType(void) {
  return NewTypeRecordWithSize(CompilerIsCXX() ? kTypeBool : kTypeInt,
                               kQualPlain);
}

// Contract: Builds lane-wise integer vector type for vector comparisons; caller
// owns the returned type record.
static TypeRecord* NewVectorComparisonResultType(TypeRecord* vector_type) {
  TypeRecord* source = TypeVectorElement(vector_type);
  TypeRecord* element = NULL;
  if (TypeIsIntegral(source)) {
    element = TypeRecordCopy(source);
    element->type &= ~kTypeUnsigned;
    element->type |= kTypeSigned;
    element->qualifiers = kQualPlain;
  } else {
    Type type = source->size == 1 ? kTypeChar
                : source->size == 2 ? kTypeShort
                : source->size == 4 ? kTypeInt
                                    : kTypeLongLong;
    element = NewTypeRecordWithSize(type | kTypeSigned, kQualPlain);
  }
  return NewVectorTypeRecord(element, TypeVectorLaneCount(vector_type));
}

// The type a bit-field operand promotes to ([conv.prom]/5, C11 6.3.1.1p2):
// int when int holds every value of the field, unsigned int when that does,
// otherwise NULL.
// Contract: Read-only on `operand` shape; returns new type or NULL (caller
// deletes when applying promotion).
TypeRecord* BitFieldPromotionType(ASTNode* operand) {
  if (operand == NULL || !IsBitfieldReference(operand)) {
    return NULL;
  }
  StructMember* member =
      ((StructMemberASTNode*)((BinaryASTNode*)operand)->right)->member;
  if (member == NULL || !member->is_bit_field || member->bit_size <= 0 ||
      member->symbol == NULL || member->symbol->type == NULL ||
      !TypeIsIntegral(member->symbol->type) ||
      TypeIsEnum(member->symbol->type) ||
      TypeIsBitInt(member->symbol->type)) {
    return NULL;
  }
  int int_bits = SizeofType(kTypeInt) * 8;
  bool is_unsigned = TypeIsUnsigned(member->symbol->type);
  if (member->bit_size < int_bits ||
      (member->bit_size == int_bits && !is_unsigned)) {
    return NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  if (member->bit_size == int_bits) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualPlain);
  }
  return NULL;
}

// Applies bit-field promotion to |operand|; returns whether it promoted.
// Contract: May mutate `operand` via `NormalConversion` when promotion applies.
static bool PromoteBitFieldOperand(ASTNode* operand) {
  TypeRecord* promoted = BitFieldPromotionType(operand);
  if (promoted == NULL) {
    return false;
  }
  if (!TypeEqual(promoted, operand->type)) {
    NormalConversion(operand, promoted);
  }
  return true;
}

// Contract: Analyzes `sub` in place; may replace `sub` (lambda-to-pointer on
// uplus). Sets result type and value category for the unary op.
void AnalyzeUnaryExpression(UnaryASTNode* node) {
  if (node == NULL) {
    return;
  }
  node->sub = AnalyzeExpression(node->sub);
  if (node->base.op == AST_OP(uplus)) {
    ASTNode* function_pointer = LambdaToFunctionPointer(node->sub, NULL);
    if (function_pointer != NULL) {
      ASTNodeReplaceChild((ASTNode*)node, 0, function_pointer, true);
    }
  }
  if (node->base.op == AST_OP(not)) {
    SemanticConvertType(node->sub,
                        NewTypeRecordWithSize(kTypeBool, kQualPlain),
                        kConvertContextualBool);
  }
  if (!TypeIsVector(node->sub->type)) {
    SemanticCheckScalarType(node->sub);
  } else if (node->base.op == AST_OP(not)) {
    SemanticError((ASTNode*)node, "logical not is not valid for vector types");
  }
  switch (node->base.op) {
    case AST_OP(not):
      // Not operator is boolean.
      break;
    case AST_OP(uminus): {
      if (TypeIsComplex(node->sub->type) || TypeIsVector(node->sub->type)) {
        break;
      }
      if (PromoteBitFieldOperand(node->sub)) {
        break;
      }
      int rank = GetRank(node->sub->type);
      if (!TypeIsBitInt(node->sub->type) && rank < IntRank()) {
        NormalConversion(
            node->sub, NewTypeRecordWithSize(kTypeInt, kQualPlain));
      }
      break;
    }
    case AST_OP(uplus):
    case AST_OP(onescomp):
      PromoteBitFieldOperand(node->sub);
      break;
    default:
      break;

  }
  ASTNodeSetType((ASTNode*)node, node->base.op == AST_OP(not)
                                      ? NewLogicalResultType()
                                      : node->sub->type);
}

// Contract: Analyzes operand under `noexcept_operand_depth`; result is bool.
void AnalyzeNoexceptExpression(UnaryASTNode* node) {
  if (node->sub != NULL) {
    compiler->noexcept_operand_depth++;
    node->sub = AnalyzeExpression(node->sub);
    compiler->noexcept_operand_depth--;
  }
  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
}

// Contract: May rewrite to `operator co_await` member call; analyzes `sub`
// first. Mutates coroutine state on the enclosing function when valid.
void AnalyzeCoAwaitExpression(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  if (compiler->current_function == NULL ||
      !compiler->current_function->info.function.is_coroutine) {
    SemanticError((ASTNode*)node, "co_await used outside a coroutine");
  }
  if (CompilerIsCXX() && TypeIsStructOrUnion(node->sub->type) &&
      node->sub->type->info.struct_info != NULL) {
    StructMember* member =
        FindStructMemberByName(node->sub->type->info.struct_info,
                               "operator co_await");
    if (member != NULL && member->is_member_function) {
      ASTNode* receiver = ASTNodeMove(node->sub);
      ASTNode* call = NewOperatorMemberCall(receiver, "operator co_await", NULL,
                                            node->base.location);
      ASTNodeReplaceChild((ASTNode*)node, 0, AnalyzeExpression(call), true);
    }
  }
  bool already_awaiter = false;
  if (CompilerIsCXX() && node->sub != NULL &&
      TypeIsStructOrUnion(node->sub->type) &&
      node->sub->type->info.struct_info != NULL) {
    StructMember* await_ready =
        FindStructMemberByName(node->sub->type->info.struct_info,
                               "await_ready");
    already_awaiter = await_ready != NULL && await_ready->is_member_function;
  }
  if (CompilerIsCXX() && !already_awaiter && node->sub != NULL &&
      node->sub->type != NULL) {
    Symbol* function =
        ResolveFreeOperatorCoAwaitForActual(node->sub,
                                            /*diagnose_ambiguous=*/true);
    if (function != NULL) {
      ASTNode* actual = ASTNodeMove(node->sub);
      ASTNode* call = NewOperatorFreeCall(function, actual, NULL,
                                          node->base.location);
      ASTNodeReplaceChild((ASTNode*)node, 0, AnalyzeExpression(call), true);
    }
  }
  TypeRecord* awaitable_type = node->sub != NULL ? node->sub->type : NULL;
  StructMember* await_resume = NULL;
  if (TypeIsStructOrUnion(awaitable_type) &&
      awaitable_type->info.struct_info != NULL) {
    StructMember* await_ready =
        FindStructMemberByName(awaitable_type->info.struct_info,
                               "await_ready");
    StructMember* await_suspend =
        FindStructMemberByName(awaitable_type->info.struct_info,
                               "await_suspend");
    await_resume = FindStructMemberByName(awaitable_type->info.struct_info,
                                          "await_resume");
    if (await_ready == NULL || !await_ready->is_member_function) {
      SemanticError((ASTNode*)node, "awaiter is missing await_ready");
    }
    if (await_suspend == NULL || !await_suspend->is_member_function) {
      SemanticError((ASTNode*)node, "awaiter is missing await_suspend");
    }
    if (await_resume == NULL || !await_resume->is_member_function) {
      SemanticError((ASTNode*)node, "awaiter is missing await_resume");
    }
  } else {
    SemanticError((ASTNode*)node, "co_await operand must be an awaiter object");
  }
  if (await_resume != NULL && await_resume->symbol != NULL &&
      TypeIsFunction(await_resume->symbol->type) &&
      await_resume->symbol->type->next != NULL) {
    ASTNodeSetType((ASTNode*)node, await_resume->symbol->type->next);
    return;
  }
  ASTNodeSetType((ASTNode*)node,
                 node->sub != NULL && node->sub->type != NULL
                     ? node->sub->type
                     : NewTypeRecordWithSize(kTypeInt, kQualPlain));
}

// Contract: Analyzes operand; requires coroutine context. Result type follows
// promise `yield_value` / operand type.
void AnalyzeCoYieldExpression(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  if (compiler->current_function == NULL ||
      !compiler->current_function->info.function.is_coroutine) {
    SemanticError((ASTNode*)node, "co_yield used outside a coroutine");
  }
  TypeRecord* promise =
      compiler->current_function != NULL
          ? compiler->current_function->info.function.coroutine_promise_type
          : NULL;
  StructMember* yield_value = NULL;
  if (TypeIsStructOrUnion(promise) && promise->info.struct_info != NULL) {
    yield_value = FindStructMemberByName(promise->info.struct_info,
                                         "yield_value");
  }
  TypeRecord* awaiter_type =
      yield_value != NULL && yield_value->symbol != NULL &&
              TypeIsFunction(yield_value->symbol->type)
          ? yield_value->symbol->type->next
          : NULL;
  StructMember* await_resume = NULL;
  if (TypeIsStructOrUnion(awaiter_type) &&
      awaiter_type->info.struct_info != NULL) {
    await_resume = FindStructMemberByName(awaiter_type->info.struct_info,
                                          "await_resume");
  }
  if (await_resume != NULL && await_resume->symbol != NULL &&
      TypeIsFunction(await_resume->symbol->type) &&
      await_resume->symbol->type->next != NULL) {
    ASTNodeSetType((ASTNode*)node, await_resume->symbol->type->next);
    return;
  }
  if (yield_value != NULL) {
    SemanticError((ASTNode*)node,
                  "coroutine yield_value return type is invalid");
  }
  ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

// Contract: Read-only leaf check for integer literal ops (not full constant expr).
static bool IsIntConstant(ASTNode* node) {
  return node->op == AST_OP(number) || node->op == AST_OP(charconst);
}

// Contract: Emits diagnostics only; requires analyzed operand types on `node`.
static void DiagnoseCXX26EnumArithmetic(BinaryASTNode* node) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    return;
  }
  TypeRecord* left = node->left->type;
  TypeRecord* right = node->right->type;
  bool left_enum = TypeIsEnum(left);
  bool right_enum = TypeIsEnum(right);
  bool different_enums =
      left_enum && right_enum && left->info.enum_info != right->info.enum_info;
  bool enum_and_floating =
      (left_enum && TypeIsFloatingPoint(right)) ||
      (right_enum && TypeIsFloatingPoint(left));
  if (different_enums || enum_and_floating) {
    SemanticError((ASTNode*)node,
                  "usual arithmetic conversions between different enumeration "
                  "types or between an enumeration and floating-point type are "
                  "not allowed in C++26");
  }
}

// Contract: Matching vector operands share a cv-unqualified vector type on `node`.
// Returns true when either operand is vector (including mismatch diagnostics).
static bool InsertNumericConversionsPhaseVector(BinaryASTNode* node) {
  if (!TypeIsVector(node->left->type) && !TypeIsVector(node->right->type)) {
    return false;
  }
  if (!TypeIsVector(node->left->type) ||
      !TypeIsVector(node->right->type) ||
      !TypeEqual(node->left->type, node->right->type)) {
    SemanticTypeConversionError(node->left, node->right->type,
                                "Vector operand types '%s' and '%s' differ");
    return true;
  }
  TypeRecord* result = TypeRecordCopy(node->left->type);
  result->qualifiers = kQualPlain;
  ASTNodeSetType((ASTNode*)node, result);
  return true;
}

// Contract: Member-pointer operands skip numeric conversion entirely.
// Returns true when either side is a pointer-to-member.
static bool InsertNumericConversionsPhaseMemberPointer(BinaryASTNode* node) {
  if (TypeIsMemberPointer(node->left->type) ||
      TypeIsMemberPointer(node->right->type)) {
    return true;
  }
  return false;
}

// Contract: Complex usual arithmetic conversions on both operands; sets `node`
// type to the common complex type. Returns true when either operand is complex.
static bool InsertNumericConversionsPhaseComplex(BinaryASTNode* node) {
  if (!TypeIsComplex(node->left->type) && !TypeIsComplex(node->right->type)) {
    return false;
  }
  bool left_arithmetic = TypeIsComplex(node->left->type) ||
                         TypeIsIntegral(node->left->type) ||
                         TypeIsFloatingPoint(node->left->type);
  bool right_arithmetic = TypeIsComplex(node->right->type) ||
                          TypeIsIntegral(node->right->type) ||
                          TypeIsFloatingPoint(node->right->type);
  if (!left_arithmetic || !right_arithmetic) {
    SemanticTypeConversionError(node->left, node->right->type,
                                "Illegal numeric operand types "
                                "'%s' and '%s'");
    return true;
  }
  TypeRecord* target =
      NewComplexTypeRecord(
          ComplexArithmeticElementType(node->left->type, node->right->type),
          kQualPlain);
  NormalConversion(node->left, target);
  NormalConversion(node->right, target);
  ASTNodeSetType((ASTNode*)node, target);
  return true;
}

// Contract: Applies integer promotions, constant rank adjustment, rank/subrank
// widening, and cv-unqualification for scalar operands only.
static void InsertNumericConversionsApplyUsualArithmeticConversions(
    BinaryASTNode* node, bool promote_to_int) {
  int left_rank = GetRank(node->left->type);
  int right_rank = GetRank(node->right->type);
  if (promote_to_int) {
    // Integer promotions: operands of rank lower than int are promoted to
    // int.  This must happen on the actual operand types (including integer
    // constants such as `(short)1`) before any of the constant-adaption
    // below, otherwise small constants would skip promotion.
    if (PromoteBitFieldOperand(node->left)) {
      ASTNodeSetType((ASTNode*)node, node->left->type);
      left_rank = GetRank(node->left->type);
    }
    if (PromoteBitFieldOperand(node->right)) {
      ASTNodeSetType((ASTNode*)node, node->right->type);
      right_rank = GetRank(node->right->type);
    }
    if (!TypeIsBitInt(node->left->type) && left_rank > 0 &&
        left_rank < IntRank()) {
      NormalConversion(
          node->left, NewTypeRecordWithSize(kTypeInt, kQualPlain));
      ASTNodeSetType((ASTNode*)node, node->left->type);
      left_rank = GetRank(node->left->type);
    }
    if (!TypeIsBitInt(node->right->type) && right_rank > 0 &&
        right_rank < IntRank()) {
      NormalConversion(
          node->right, NewTypeRecordWithSize(kTypeInt, kQualPlain));
      ASTNodeSetType((ASTNode*)node, node->right->type);
      right_rank = GetRank(node->right->type);
    }
  }
  // After integer promotion, an integer constant no wider than int adapts to
  // the other operand's type (so e.g. `someUnsignedLong + 1` keeps its type)
  // by being treated as the lowest rank.  A constant with an explicit
  // long/long long type keeps its rank so the usual arithmetic conversions
  // widen the result correctly (e.g. `i + 2L` becomes long).
  if (IsIntConstant(node->left) && !TypeIsBitInt(node->left->type) &&
      !TypeIsBitInt(node->right->type) &&
      left_rank <= IntRank() &&
      !(TypeIsUnsigned(node->left->type) &&
        !TypeIsUnsigned(node->right->type) &&
        left_rank == right_rank)) {
    left_rank = 0;
  }
  if (IsIntConstant(node->right) && !TypeIsBitInt(node->right->type) &&
      !TypeIsBitInt(node->left->type) &&
      right_rank <= IntRank() &&
      !(TypeIsUnsigned(node->right->type) &&
        !TypeIsUnsigned(node->left->type) &&
        left_rank == right_rank)) {
    right_rank = 0;
  }
  // Convert smaller rank to larger.
  if (left_rank == -1 || right_rank == -1) {
    SemanticTypeConversionError(node->left, node->right->type,
                                "Illegal numeric operand types "
                                "'%s' and '%s'");
    return;
  }
  if (left_rank > right_rank) {
    // Convert right to left.
    NormalConversion(node->right, node->left->type);
    ASTNodeSetType((ASTNode*)node, node->left->type);
  } else if (left_rank < right_rank) {
    // Convert left to right.
    NormalConversion(node->left, node->right->type);
    ASTNodeSetType((ASTNode*)node, node->right->type);
  } else if (GetFloatingSubrank(node->left->type) >
             GetFloatingSubrank(node->right->type)) {
    NormalConversion(node->right, node->left->type);
    ASTNodeSetType((ASTNode*)node, node->left->type);
  } else if (GetFloatingSubrank(node->left->type) <
             GetFloatingSubrank(node->right->type)) {
    NormalConversion(node->left, node->right->type);
    ASTNodeSetType((ASTNode*)node, node->right->type);
  } else if (TypeIsUnsigned(node->left->type) !=
             TypeIsUnsigned(node->right->type)) {
    if (TypeIsUnsigned(node->left->type)) {
      NormalConversion(node->right, node->left->type);
      ASTNodeSetType((ASTNode*)node, node->left->type);
    } else {
      NormalConversion(node->left, node->right->type);
      ASTNodeSetType((ASTNode*)node, node->right->type);
    }
  } else {
    // Equal-rank operands still determine the expression type.  Leaving the
    // node's parser-default `int` type in place breaks expressions such as
    // `false ? declval<long long>() : declval<long long>()`, and therefore
    // `common_type_t<long long, long long>`.
    ASTNodeSetType((ASTNode*)node, node->left->type);
  }
  // The result of the usual arithmetic conversions is a prvalue of a cv-
  // unqualified type.  Operand types can still carry cv-qualifiers when they
  // arrive through the lvalue-to-rvalue conversion (e.g. via decltype /
  // declval, `declval<const int&>() + declval<long>()`), so normalize the
  // result; otherwise decltype would observe a spurious `const`.
  if (node->base.type != NULL &&
      (TypeIsIntegral(node->base.type) ||
       TypeIsFloatingPoint(node->base.type)) &&
      node->base.type->qualifiers != kQualPlain) {
    TypeRecord* unqualified = TypeRecordCopy(node->base.type);
    unqualified->qualifiers = kQualPlain;
    ASTNodeSetType((ASTNode*)node, unqualified);
    TypeRecordDelete(unqualified);
  }
}

// Contract: Struct operands diagnose only; pointer/nullptr mixing and scalar
// usual arithmetic conversions mutate operand subtrees and set `node` type.
static void InsertNumericConversionsPhaseStructPointerOrScalar(
    BinaryASTNode* node, bool promote_to_int) {
  if (TypeIsStructOrUnion(node->left->type) ||
      TypeIsStructOrUnion(node->right->type)) {
    SemanticTypeConversionError(node->left, node->right->type,
                                "Illegal binary operand types "
                                "'%s' and '%s'");
  } else if (TypeIsPointerOrArray(node->left->type) ||
             TypeIsPointerOrArray(node->right->type)) {
    // One of the types is a pointer or array, check for compatibility.
    if (TypeIsPointerOrArray(node->left->type) &&
        TypeIsPointerOrArray(node->right->type)) {
      // Both pointers
      if (!TypeEqual(node->left->type, node->left->type)) {
        SemanticTypeConversionWarning(node->left, node->right->type,
                                      "pointer-types",
                                    "Illegal pointer types "
                                    "'%s' and '%s'");
      }
      ASTNodeSetType((ASTNode*)node, node->right->type);
    } else {
      // One is a pointer, the other isn't.
      ASTNode* ptr = node->right;
      ASTNode* nonptr = node->left;
      if (TypeIsPointerOrArray(node->left->type)) {
        ASTNode* t = ptr;
        ptr = nonptr;
        nonptr = t;
      }
      if (!IsNullPointer(nonptr)) {
        SemanticTypeConversionWarning(ptr, nonptr->type,
                                      "pointer-types",
                                    "Illegal pointer types "
                                    "'%s' and '%s'");

      } else if (TypeIsNullPointer(nonptr->type)) {
        // A `nullptr` operand has type std::nullptr_t, which has no arithmetic
        // IR opcode of its own.  Convert it to the pointer operand's type so
        // the operation is generated as an ordinary pointer comparison instead
        // of tripping the backend's opcode lookup.  (A plain `0` operand keeps
        // its int type and is handled by the integral path, as before.)
        NormalConversion(nonptr, ptr->type);
      }
      ASTNodeSetType((ASTNode*)node, ptr->type);
    }
  } else {
    InsertNumericConversionsApplyUsualArithmeticConversions(node, promote_to_int);
  }
}

// Check that we have a valid operands for a numeric expression
// and insert conversions as necessary.
// Contract: Mutates operand subtrees via `NormalConversion` / promotions;
// sets `node` type. Requires analyzed children. `promote_to_int` enables
// integer promotions before usual arithmetic conversions.
void InsertNumericConversions(BinaryASTNode* node, bool promote_to_int) {
  DiagnoseCXX26EnumArithmetic(node);
  if (InsertNumericConversionsPhaseVector(node)) {
    return;
  }
  if (InsertNumericConversionsPhaseMemberPointer(node)) {
    return;
  }
  if (InsertNumericConversionsPhaseComplex(node)) {
    return;
  }
  InsertNumericConversionsPhaseStructPointerOrScalar(node, promote_to_int);
}

// Contract: Maps AST opcodes to `operator@` names for overload lookup; NULL if none.
const char* BinaryOperatorFunctionName(ASTOpcode op) {
  switch (op) {
    case AST_OP(plus):
      return "operator+";
    case AST_OP(minus):
      return "operator-";
    case AST_OP(mult):
      return "operator*";
    case AST_OP(div):
      return "operator/";
    case AST_OP(mod):
      return "operator%";
    case AST_OP(lshift):
      return "operator<<";
    case AST_OP(rshift):
    case AST_OP(rshiftl):
    case AST_OP(rshifta):
      return "operator>>";
    case AST_OP(and):
      return "operator&";
    case AST_OP(bitor):
      return "operator|";
    case AST_OP(exor):
      return "operator^";
    case AST_OP(logand):
      return "operator&&";
    case AST_OP(logor):
      return "operator||";
    case AST_OP(comma):
      return "operator,";
    case AST_OP(assign):
      return "operator=";
    case AST_OP(pluseq):
      return "operator+=";
    case AST_OP(minuseq):
      return "operator-=";
    case AST_OP(multeq):
      return "operator*=";
    case AST_OP(diveq):
      return "operator/=";
    case AST_OP(percenteq):
      return "operator%=";
    case AST_OP(lshifteq):
      return "operator<<=";
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
      return "operator>>=";
    case AST_OP(andeq):
      return "operator&=";
    case AST_OP(oreq):
      return "operator|=";
    case AST_OP(exoreq):
      return "operator^=";
    case AST_OP(equal):
      return "operator==";
    case AST_OP(noteq):
      return "operator!=";
    case AST_OP(less):
      return "operator<";
    case AST_OP(lesseq):
      return "operator<=";
    case AST_OP(greater):
      return "operator>";
    case AST_OP(greatereq):
      return "operator>=";
    case AST_OP(spaceship):
      return "operator<=>";
    case AST_OP(arrowstar):
      return "operator->*";
    default:
      return NULL;
  }
}

// Contract: Unary overload name mapping; NULL when not a user-overloadable unary op.
const char* UnaryOperatorFunctionName(ASTOpcode op) {
  switch (op) {
    case AST_OP(uplus):
      return "operator+";
    case AST_OP(uminus):
      return "operator-";
    case AST_OP(address):
      return "operator&";
    case AST_OP(contents):
      return "operator*";
    case AST_OP(not):
      return "operator!";
    case AST_OP(onescomp):
      return "operator~";
    default:
      return NULL;
  }
}

// Contract: Splices `call` over `node` (deletes operator node), then analyzes
// and returns the analyzed call root.
static ASTNode* ReplaceBinaryWithCall(BinaryASTNode* node, ASTNode* call) {
  ASTNode* parent = node->base.parent;
  int child_id = node->base.child_id;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  return AnalyzeExpression(call);
}

// Contract: Same splice/analyze pattern as `ReplaceBinaryWithCall` for unary ops.
static ASTNode* ReplaceUnaryWithCall(UnaryASTNode* node, ASTNode* call) {
  ASTNode* parent = node->base.parent;
  int child_id = node->base.child_id;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  return AnalyzeExpression(call);
}

// Contract: Splices `call` over vector-shaped node (e.g. subscript), deletes old.
ASTNode* ReplaceVectorWithCall(VectorASTNode* node, ASTNode* call) {
  ASTNode* parent = node->base.parent;
  int child_id = node->base.child_id;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  return AnalyzeExpression(call);
}

// Contract: Builds unattached `call` AST; ownership passes to caller (often
// followed by Replace*WithCall). Does not analyze children.
ASTNode* NewOperatorMemberCall(ASTNode* receiver, const char* op_name,
                                      Vector* actuals,
                                      SourceLocation location) {
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(op_name), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member_name);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                          actuals != NULL ? actuals : NewVector());
}

// Contract: Builds unattached free-operator call; `first_actual` moved into
// actuals vector; caller owns returned node.
static ASTNode* NewOperatorFreeCall(Symbol* function, ASTNode* first_actual,
                                    Vector* remaining_actuals,
                                    SourceLocation location) {
  Vector* actuals = NewVector();
  if (first_actual != NULL) {
    VectorAppend(actuals, first_actual);
  }
  if (remaining_actuals != NULL) {
    VectorAppendVector(actuals, remaining_actuals);
  }
  return NewVectorASTNode(AST_OP(call), NULL, location,
                          NewIdentifierASTNode(function, location), actuals);
}

// Contract: Read-only alias chain walk for ADL/overload gathering; bounded depth.
Symbol* FollowUsingAliasForADL(Symbol* symbol) {
  int depth = 0;
  while (symbol != NULL && symbol->flags.is_using_alias &&
         symbol->alias_target != NULL && depth < 64) {
    symbol = symbol->alias_target;
    depth++;
  }
  return symbol;
}

// Contract: Dedup helper for candidate vectors; no mutation.
static bool VectorContainsPointer(Vector* vec, void* value) {
  for (size_t i = 0; i < vec->length; i++) {
    if (vec->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

// Contract: Appends associated namespaces for ADL; mutates `namespaces` only.
static void ADLAddAssociatedNamespaceClosure(Vector* namespaces, Namespace* ns) {
  NamespaceCollectADLAssociatedNamespaces(ns, namespaces);
}

// Contract: Strips pointer/array/reference declarators for ADL type walk; no alloc.
static TypeRecord* ADLCanonicalType(TypeRecord* type) {
  while (type != NULL &&
         (TypeIsReference(type) || TypeIsPointer(type) || TypeIsArray(type))) {
    type = type->next;
  }
  return type;
}

// Contract: Recursively collects associated namespaces into `namespaces`;
// depth-limited; no AST mutation.
static void ADLCollectNamespacesForType(TypeRecord* type, Vector* namespaces,
                                        int depth) {
  if (type == NULL || depth > 8) {
    return;
  }
  type = ADLCanonicalType(type);
  if (type == NULL) {
    return;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    Namespace* ns =
        str->tag_symbol != NULL ? str->tag_symbol->namespace_ : NULL;
    if (ns == NULL && str->tag_symbol != NULL &&
        str->tag_symbol->type != NULL &&
        str->tag_symbol->type->template_origin != NULL) {
      ns = str->tag_symbol->type->template_origin->namespace_;
    }
    if (ns == NULL && type->template_origin != NULL) {
      ns = type->template_origin->namespace_;
    }
    bool has_associated_class = str->tag_symbol != NULL ||
                                type->template_origin != NULL;
    // A nested class (e.g. an instantiated view's __iterator) carries no
    // namespace on its own tag and is not itself a template instantiation, so
    // walk out through enclosing classes to the nearest namespace.  Without
    // this, ADL over such a type finds none of its hidden-friend operators.
    if (ns == NULL) {
      for (Struct* parent = str->lexical_parent; parent != NULL;
           parent = parent->lexical_parent) {
        if (parent->tag_symbol == NULL) {
          continue;
        }
        has_associated_class = true;
        if (parent->tag_symbol->namespace_ != NULL) {
          ns = parent->tag_symbol->namespace_;
          break;
        }
        if (parent->tag_symbol->type != NULL &&
            parent->tag_symbol->type->template_origin != NULL) {
          ns = parent->tag_symbol->type->template_origin->namespace_;
          break;
        }
      }
    }
    if (has_associated_class) {
      ADLAddAssociatedNamespaceClosure(namespaces,
                                       ns != NULL ? ns : compiler->global_namespace);
    }
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base != NULL) {
        ADLCollectNamespacesForType(base->type, namespaces, depth + 1);
      }
    }
  } else if (TypeIsEnum(type) && type->info.enum_info != NULL &&
             type->info.enum_info->tag_symbol != NULL) {
    Symbol* tag = type->info.enum_info->tag_symbol;
    ADLAddAssociatedNamespaceClosure(namespaces, tag->namespace_ != NULL
                                                    ? tag->namespace_
                                                    : compiler->global_namespace);
  }
  if (type->template_arguments != NULL) {
    for (size_t i = 0; i < type->template_arguments->length; i++) {
      TemplateArgument* arg = type->template_arguments->value.p[i];
      if (arg != NULL && arg->kind == kTemplateParameterType) {
        ADLCollectNamespacesForType(arg->type, namespaces, depth + 1);
      }
    }
  }
}

// Contract: Appends viable overload symbols to `candidates` (no AST mutation).
void AddFunctionOverloadCandidates(Vector* candidates, Symbol* first) {
  Symbol* alias_extensions =
      first != NULL && first->flags.is_using_alias ? first->overload_next : NULL;
  first = FollowUsingAliasForADL(first);
  // Static member functions link their overloads through the StructMember chain
  // rather than the symbol-level overload_next chain that ordinary
  // (namespace-scope) functions use.  When a qualified name resolves to a static
  // member function (e.g. `Class::f(...)` or `traits_type::assign(...)`), only
  // the single member the lookup returned is reachable via overload_next, so
  // gather the rest of the overload set from the owning struct's member list.
  //
  // This must be limited to *static* members.  Non-static member functions are
  // resolved through ResolveMemberFunctionOverload, which applies the
  // receiver-const tie-breaker; gathering their overloads here and rescoring via
  // the ordinary (penalty-free) path would make const/non-const pairs ambiguous.
  // A non-static member carries an implicit `this` as its first parameter, so a
  // member function with an owner but no leading `this` is the static case.
  bool first_is_static_member =
      first != NULL && first->type != NULL && TypeIsFunction(first->type) &&
      first->type->info.function.cxx_member_owner != NULL &&
      (first->type->info.function.prototype.length == 0 ||
       !StringEqual(
           &((Symbol*)first->type->info.function.prototype.value.p[0])->name,
           "this"));
  if (first_is_static_member && first->overload_next == NULL) {
    // Prefer the class that published this overload set.  A using-declaration
    // of a base static function shares the base function type, so
    // cxx_member_owner is still the base and would hide overloads added by
    // the derived class.
    Struct* owner = first->member_lookup_class != NULL
                        ? first->member_lookup_class
                        : first->type->info.function.cxx_member_owner;
    StructMember* head = FindStructMember(owner, &first->name);
    if (head != NULL) {
      for (StructMember* m = head; m != NULL; m = m->overload_next) {
        // This path is the qualified `Class::f(...)` lookup, which can only
        // call static members.  A non-static overload of the same name has an
        // implicit `this` and would otherwise compete with the static
        // signature (`AddData(tree, data)` vs `AddData(this, data)`).
        if (!m->is_static) {
          continue;
        }
        if (m->symbol != NULL && m->symbol->type != NULL &&
            TypeIsFunction(m->symbol->type) &&
            !VectorContainsPointer(candidates, m->symbol)) {
          VectorAppend(candidates, m->symbol);
        }
      }
      if (candidates->length > 0) {
        return;
      }
    }
  }
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    Symbol* effective = FollowUsingAliasForADL(candidate);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !VectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
  for (Symbol* candidate = alias_extensions; candidate != NULL;
       candidate = candidate->overload_next) {
    Symbol* effective = FollowUsingAliasForADL(candidate);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !VectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
}

// Contract: Merges `functions` into overload `candidates` with alias resolution;
// no AST changes.
static void AddCollectedFunctionSymbols(Vector* candidates, Vector* functions) {
  for (size_t i = 0; i < functions->length; i++) {
    Symbol* sym = (Symbol*)functions->value.p[i];
    Symbol* effective = FollowUsingAliasForADL(sym);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !VectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
}

// Contract: Populates `candidates` from namespace/global lookup; read-only on AST.
void AddNamedFunctionCandidates(String* name, Namespace* ns,
                                       Vector* candidates) {
  Vector functions;
  VectorInit(&functions);
  if (ns == NULL || ns == compiler->global_namespace) {
    NamespaceCollectFunctionSymbolsInInlineSet(compiler->global_namespace, name,
                                               &functions);
    AddCollectedFunctionSymbols(candidates, &functions);
    AddFunctionOverloadCandidates(candidates, FindGlobalSymbol(name));
  } else {
    NamespaceCollectFunctionSymbolsInInlineSet(ns, name, &functions);
    AddCollectedFunctionSymbols(candidates, &functions);
  }
  VectorDestruct(&functions);
}

// Contract: ADL namespace discovery from argument types, then named lookup;
// mutates `candidates` only.
void AddADLFunctionCandidates(String* name, Vector* actuals,
                                     Vector* candidates) {
  if (!CompilerIsCXX() || actuals == NULL) {
    return;
  }
  Vector namespaces;
  VectorInit(&namespaces);
  for (size_t i = 0; i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual != NULL) {
      ADLCollectNamespacesForType(actual->type, &namespaces, 0);
    }
  }
  for (size_t i = 0; i < namespaces.length; i++) {
    AddNamedFunctionCandidates(name, namespaces.value.p[i], candidates);
  }
  VectorDestruct(&namespaces);
}

// Contract: Analyzes operand, then may replace the unary node with an
// analyzed call via `ReplaceUnaryWithCall`. Returns NULL to keep built-in
// semantics.
ASTNode* TryAnalyzeOverloadedUnaryOperator(UnaryASTNode* node) {
  const char* op_name = UnaryOperatorFunctionName(node->base.op);
  if (!CompilerIsCXX() || op_name == NULL) {
    return NULL;
  }

  node->sub = AnalyzeExpression(node->sub);
  if (node->sub->type != NULL) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->sub->type);
    if (materialized != node->sub->type) {
      ASTNodeSetType(node->sub, materialized);
    }
  }
  if (!TypeIsStructOrUnion(node->sub->type)) {
    return NULL;
  }

  StructMember* member =
      FindStructMemberByName(node->sub->type->info.struct_info, op_name);
  if (member != NULL && member->is_member_function) {
    ASTNode* receiver = ASTNodeMove(node->sub);
    ASTNode* call = NewOperatorMemberCall(receiver, op_name, NULL,
                                          node->base.location);
    return ReplaceUnaryWithCall(node, call);
  }

  String name;
  StringInit(&name, op_name);
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, node->sub);
  Symbol* function = ResolveFreeFunctionWithADL(&name, &actuals,
                                                /*diagnose_ambiguous=*/true);
  VectorDestruct(&actuals);
  if (function != NULL && TypeIsFunction(function->type)) {
    ASTNode* actual = ASTNodeMove(node->sub);
    ASTNode* call = NewOperatorFreeCall(function, actual, NULL,
                                        node->base.location);
    StringDestruct(&name);
    return ReplaceUnaryWithCall(node, call);
  }
  StringDestruct(&name);
  return NULL;
}

// Contract: Maps inc/dec opcodes to `operator++`/`operator--` spellings; no side effects.
static const char* IncDecOperatorFunctionName(ASTOpcode op) {
  switch (op) {
    case AST_OP(preinc):
    case AST_OP(postinc):
      return "operator++";
    case AST_OP(predec):
    case AST_OP(postdec):
      return "operator--";
    default:
      return NULL;
  }
}

// Contract: Distinguishes postfix inc/dec for dummy-int overload argument.
static bool IsPostIncDec(ASTOpcode op) {
  return op == AST_OP(postinc) || op == AST_OP(postdec);
}

// Contract: Returns unattached int constant `0` for postfix operator signature.
static ASTNode* NewPostfixDummyArgument(SourceLocation location) {
  return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

// Contract: Analyzes operand; may replace unary node with member/free inc/dec
// call (moves `sub` into call). Returns analyzed call or NULL.
static ASTNode* TryAnalyzeOverloadedIncDecOperator(UnaryASTNode* node) {
  const char* op_name = IncDecOperatorFunctionName(node->base.op);
  if (!CompilerIsCXX() || op_name == NULL) {
    return NULL;
  }

  node->sub = AnalyzeExpression(node->sub);
  if (node->sub->type != NULL) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->sub->type);
    if (materialized != node->sub->type) {
      ASTNodeSetType(node->sub, materialized);
    }
  }
  if (!TypeIsStructOrUnion(node->sub->type)) {
    return NULL;
  }

  bool postfix = IsPostIncDec(node->base.op);
  StructMember* member =
      FindStructMemberByName(node->sub->type->info.struct_info, op_name);
  bool has_matching_member = false;
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->symbol != NULL &&
        candidate->symbol->type != NULL &&
        TypeIsFunction(candidate->symbol->type)) {
      size_t expected_parameters = postfix ? 2 : 1;
      if (candidate->symbol->type->info.function.prototype.length ==
          expected_parameters) {
        has_matching_member = true;
        break;
      }
    }
  }
  if (has_matching_member) {
    Vector* actuals = NewVector();
    if (postfix) {
      VectorAppend(actuals, NewPostfixDummyArgument(node->base.location));
    }
    ASTNode* call =
        NewOperatorMemberCall(ASTNodeMove(node->sub), op_name, actuals,
                              node->base.location);
    return ReplaceUnaryWithCall(node, call);
  }

  String name;
  StringInit(&name, op_name);
  Vector lookup_actuals;
  VectorInit(&lookup_actuals);
  VectorAppend(&lookup_actuals, node->sub);
  if (postfix) {
    VectorAppend(&lookup_actuals, NewPostfixDummyArgument(node->base.location));
  }
  Symbol* function = ResolveFreeFunctionWithADL(&name, &lookup_actuals,
                                                /*diagnose_ambiguous=*/true);
  VectorDestruct(&lookup_actuals);
  if (function != NULL && TypeIsFunction(function->type)) {
    Vector* actuals = NULL;
    if (postfix) {
      actuals = NewVector();
      VectorAppend(actuals, NewPostfixDummyArgument(node->base.location));
    }
    ASTNode* call = NewOperatorFreeCall(function, ASTNodeMove(node->sub),
                                        actuals, node->base.location);
    StringDestruct(&name);
    return ReplaceUnaryWithCall(node, call);
  }
  StringDestruct(&name);
  return NULL;
}

// Contract: Strips references; returns class type for member operator lookup.
static TypeRecord* CXXOperatorOperandClassType(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  if (TypeIsReference(type)) {
    type = type->next;
  }
  return TypeIsStructOrUnion(type) ? type : NULL;
}

// Scores the built-in-vs-member competition for a binary operator's member
// candidate set.  Returns true when at least one member operator overload is
// viable for `node`'s operands, setting *out_best_score to the best (lowest)
// candidate score, *out_ambiguous when two or more distinct overloads tie at
// that best score, and *out_has_template when a template member overload is
// present (whose score is not computed here).  A template member overload is
// treated as viable conservatively (deducing it here would be a premature side
// effect).  Used so a genuinely ambiguous member set does not shadow a better
// non-member (free) operator ([over.match.oper]).
static bool BestBinaryMemberOperatorScore(BinaryASTNode* node,
                                          const char* op_name,
                                          int* out_best_score,
                                          bool* out_ambiguous,
                                          bool* out_has_template) {
  if (out_best_score != NULL) {
    *out_best_score = -1;
  }
  if (out_ambiguous != NULL) {
    *out_ambiguous = false;
  }
  if (out_has_template != NULL) {
    *out_has_template = false;
  }
  ASTNode* left = node->left;
  TypeRecord* class_type = CXXOperatorOperandClassType(left->type);
  if (class_type == NULL || class_type->info.struct_info == NULL) {
    return false;
  }
  StructMember* first =
      FindStructMemberByName(class_type->info.struct_info, op_name);
  if (first == NULL || !first->is_member_function) {
    return false;
  }
  Vector children;
  VectorInit(&children);
  VectorAppend(&children, node->right);
  VectorASTNode call = {0};
  call.children = &children;
  bool receiver_const = MemberReceiverIsConst(node);
  bool receiver_volatile = MemberReceiverIsVolatile(node);
  bool viable = false;
  int best_score = -1;
  int best_count = 0;
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    if (candidate->symbol->flags.is_template) {
      viable = true;
      if (out_has_template != NULL) {
        *out_has_template = true;
      }
      continue;
    }
    int score = FunctionCallScore(candidate->symbol->type, &call,
                                  /*first_formal_arg=*/1);
    if (score < 0) {
      continue;
    }
    if (receiver_const &&
        !candidate->symbol->type->info.function.is_const_member &&
        !candidate->symbol->type->info.function.is_constructor &&
        !candidate->symbol->type->info.function.is_destructor) {
      continue;
    }
    if (receiver_volatile &&
        !candidate->symbol->type->info.function.is_volatile_member &&
        !candidate->symbol->type->info.function.is_constructor &&
        !candidate->symbol->type->info.function.is_destructor) {
      continue;
    }
    if (!MemberReceiverMatchesRefQualifier(candidate->symbol->type, node)) {
      continue;
    }
    viable = true;
    if (best_score < 0 || score < best_score) {
      best_score = score;
      best_count = 1;
    } else if (score == best_score) {
      best_count++;
    }
  }
  VectorDestruct(&children);
  if (out_best_score != NULL) {
    *out_best_score = best_score;
  }
  if (out_ambiguous != NULL) {
    *out_ambiguous = best_count >= 2;
  }
  return viable;
}

// Conversion rank of a free binary operator's *second* parameter against the
// binary node's right operand.  A member operator is scored over its second
// operand only (its first operand is the implicit object argument), so this is
// the free-operator score on the same scale, letting a member and a non-member
// operator compete on the operand that distinguishes them.  Returns -1 when the
// candidate is not a viable binary free operator.
static int FreeBinaryOperatorSecondOperandScore(Symbol* function,
                                                ASTNode* right) {
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type)) {
    return -1;
  }
  Vector* prototype = &function->type->info.function.prototype;
  if (prototype->length < 2) {
    return -1;
  }
  Symbol* second = prototype->value.p[1];
  if (second == NULL || second->type == NULL) {
    return -1;
  }
  return OverloadConversionRank(right, second->type);
}

// Contract: Moves operands into member `op_name` call and splices via
// `ReplaceBinaryWithCall` (deletes operator node).
static ASTNode* BuildMemberOperatorCall(BinaryASTNode* node,
                                        const char* op_name) {
  ASTNode* left = ASTNodeMove(node->left);
  ASTNode* right = ASTNodeMove(node->right);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(op_name), NULL, node->base.location);
  ASTNode* member_access = NewBinaryASTNode(AST_OP(dot), NULL,
                                            node->base.location, left,
                                            member_name);
  Vector* actuals = NewVector();
  VectorAppend(actuals, right);
  ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, node->base.location,
                                   member_access, actuals);
  return ReplaceBinaryWithCall(node, call);
}

// Contract: Assumes operands are analyzed; may replace `node` with a member or
// free operator call. Returns analyzed call root or NULL for built-in lowering.
ASTNode* TryAnalyzeOverloadedBinaryOperator(BinaryASTNode* node) {
  const char* op_name = BinaryOperatorFunctionName(node->base.op);
  if (!CompilerIsCXX() || op_name == NULL ||
      node->left == NULL || node->right == NULL ||
      node->left->type == NULL || node->right->type == NULL) {
    return NULL;
  }
  // A constructor member-initializer that targets a reference data member binds
  // the reference; it is not an assignment through the referent.  Do not resolve
  // it as `referent.operator=(...)` -- leave it for AnalyzeAssignmentExpression's
  // reference-binding path.  (Without this, a class-type referent's operator=
  // would be selected here, before the assignment analysis runs.)
  if (node->base.op == AST_OP(assign) &&
      (node->base.flags & kASTCXXMemberInitializer) != 0 &&
      node->left != NULL && node->left->type != NULL &&
      TypeIsReference(node->left->type)) {
    return NULL;
  }
  TypeRecord* left_class = CXXOperatorOperandClassType(node->left->type);
  TypeRecord* right_class = CXXOperatorOperandClassType(node->right->type);
  if (left_class == NULL && right_class == NULL) {
    return NULL;
  }

  bool member_present = false;
  if (left_class != NULL && left_class->info.struct_info != NULL) {
    StructMember* member = FindStructMemberByName(left_class->info.struct_info,
                                                  op_name);
    member_present = member != NULL && member->is_member_function;
  }

  // Member and non-member (free) operators of the same name compete together
  // ([over.match.oper]); a member operator does not hide the free ones.  Score
  // the best member candidate over its second operand and, when a free operator
  // converts that same operand strictly better -- or the member set is ambiguous
  // by itself and the free operator is no worse -- choose the free operator.
  // This is what makes `ostream << "..."` (and `<< 'c'`) select the exact-match
  // free `operator<<(basic_ostream&, const CharT*)` (resp. `CharT`) instead of a
  // member `operator<<(bool)` / `operator<<(const void*)` that only matches via a
  // worse conversion.  A template member overload's conversion cost is not
  // modeled here, so keep the historical member preference when one is viable.
  bool member_has_template = false;
  int member_best_score = -1;
  bool member_ambiguous = false;
  bool member_viable =
      member_present &&
      BestBinaryMemberOperatorScore(node, op_name, &member_best_score,
                                    &member_ambiguous, &member_has_template);

  String name;
  StringInit(&name, op_name);
  Vector lookup_actuals;
  VectorInit(&lookup_actuals);
  VectorAppend(&lookup_actuals, node->left);
  VectorAppend(&lookup_actuals, node->right);
  // Only diagnose an ambiguous free set when there is no member fallback: with a
  // viable member operator, a silently-ambiguous free set must not turn a
  // previously valid member call into a hard error.
  Symbol* function = ResolveFreeFunctionWithADL(
      &name, &lookup_actuals, /*diagnose_ambiguous=*/!member_viable);
  VectorDestruct(&lookup_actuals);

  bool use_free = false;
  if (function != NULL && TypeIsFunction(function->type)) {
    if (!member_viable) {
      use_free = true;
    } else if (!member_has_template) {
      int free_score = FreeBinaryOperatorSecondOperandScore(function,
                                                            node->right);
      if (free_score >= 0 &&
          (free_score < member_best_score ||
           (member_ambiguous && free_score <= member_best_score))) {
        use_free = true;
      }
    }
  }
  if (use_free) {
    ASTNode* left = ASTNodeMove(node->left);
    ASTNode* right = ASTNodeMove(node->right);
    Vector* actuals = NewVector();
    VectorAppend(actuals, left);
    VectorAppend(actuals, right);
    ASTNode* call = NewVectorASTNode(
        AST_OP(call), NULL, node->base.location,
        NewIdentifierASTNode(function, node->base.location), actuals);
    StringDestruct(&name);
    return ReplaceBinaryWithCall(node, call);
  }
  StringDestruct(&name);

  // The free operator was not preferred.  If a member operator exists, build the
  // member call: the existing member-overload machinery either performs the call
  // or emits a precise diagnostic (including for an ambiguous member set).
  if (member_present) {
    return BuildMemberOperatorCall(node, op_name);
  }
  return NULL;
}

// Contract: Analyzes operands (short-circuit may replace right arm with constant),
// then delegates to `TryAnalyzeOverloadedBinaryOperator`. Defers when braced-init
// present (assignment lowering must run first).
ASTNode* TryAnalyzeOverloadedBinaryOperatorWithAnalyzedOperands(
    BinaryASTNode* node) {
  if (BinaryOperatorFunctionName(node->base.op) == NULL) {
    return NULL;
  }
  if (node->left == NULL || node->right == NULL) {
    return NULL;
  }
  // A braced-init-list operand (e.g. the right-hand side of `x = {...}`) is not
  // an expression and has no type, so it cannot participate in
  // overloaded-operator resolution here.  Defer to the operator's dedicated
  // analysis, which lowers the braced-init-list to a temporary of the
  // appropriate type first.
  if ((node->left != NULL && node->left->op == AST_OP(braced_init)) ||
      (node->right != NULL && node->right->op == AST_OP(braced_init))) {
    return NULL;
  }
  ASTNode* analyzed_left = AnalyzeExpression(node->left);
  if (analyzed_left != node->left) {
    ASTNodeReplaceChild((ASTNode*)node, 0, analyzed_left,
                        /*delete_old_child=*/false);
  }
  if ((node->base.op == AST_OP(logand) || node->base.op == AST_OP(logor)) &&
      !ExpressionIsTemplateDependent(analyzed_left)) {
    int64_t left_truth = 0;
    if (EvaluateTruthExpression(analyzed_left, &left_truth)) {
      if ((node->base.op == AST_OP(logand) && left_truth == 0) ||
          (node->base.op == AST_OP(logor) && left_truth != 0)) {
        TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
        ASTNode* discarded = NewIntConstantASTNode(
            left_truth != 0 ? 1 : 0, bool_type, node->base.location);
        TypeRecordDelete(bool_type);
        ASTNodeReplaceChild((ASTNode*)node, 1, discarded, true);
        return NULL;
      }
    }
  }
  ASTNode* analyzed_right = AnalyzeExpression(node->right);
  if (analyzed_right != node->right) {
    ASTNodeReplaceChild((ASTNode*)node, 1, analyzed_right,
                        /*delete_old_child=*/false);
  }
  ASTNodeSetType((ASTNode*)node, node->left->type);
  return TryAnalyzeOverloadedBinaryOperator(node);
}

// A binary plus operator allows an integer to be added to a pointer (or array).
// The integer is scaled (multiplied) by the size of the thing pointed to.
// The result of pointer arithmetic (p + n, n + p, p - n) is a prvalue whose
// type is the cv-unqualified pointer type: a top-level const/volatile on the
// operand (e.g. `int* const`) does not carry over to the computed value.  Only
// the top-level qualifiers are dropped; the pointee's qualifiers are retained.
// Contract: May allocate cv-unqualified pointer copy; caller owns result.
static TypeRecord* PointerArithmeticResultType(TypeRecord* pointer_type) {
  if (pointer_type == NULL) {
    return pointer_type;
  }
  // An array operand decays to a (cv-unqualified) prvalue pointer to its
  // element type: `decltype(arr + 0)` is `T*`, not the array type `T[N]`.
  if (TypeIsArray(pointer_type)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, pointer_type->next);
    return ptr;
  }
  if (pointer_type->qualifiers == kQualPlain) {
    return pointer_type;
  }
  TypeRecord* plain = TypeRecordCopy(pointer_type);
  plain->qualifiers = kQualPlain;
  return plain;
}

// True if `node` is the multiply-by-element-size node that pointer arithmetic
// inserts, so a re-analyzed `p + n` does not scale `n` twice.  A pointer
// *difference* is also a ptr_scale node, but a dividing one, and it is an
// ordinary integer operand that still needs scaling: without this distinction
// `p + (q - r)` would advance `p` by bytes instead of elements.
// Contract: Read-only; guards against double-scaling in re-analysis.
static bool ASTNodeIsPointerScaleMultiply(ASTNode* node) {
  return node != NULL && node->op == AST_OP(ptr_scale) &&
         ((PtrScaleASTNode*)node)->scale_op == AST_OP(mult);
}

// Contract: Analyzes operands, tries overload resolution, then applies pointer/
// arithmetic or `InsertNumericConversions`. May return a replacement call node.
ASTNode* AnalyzePlusOperator(BinaryASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  if (!TypeIsVector(node->left->type)) {
    SemanticCheckScalarType(node->left);
  }
  if (!TypeIsVector(node->right->type)) {
    SemanticCheckScalarType(node->right);
  }
  if (TypeIsPointerOrArray(node->left->type)) {
    ASTNodeSetType((ASTNode*)node,
                   PointerArithmeticResultType(node->left->type));
    if (ASTNodeIsPointerScaleMultiply(node->right)) {
      return &node->base;
    }
    if (TypeIsIntegral(node->right->type)) {
      // Scale right side by size of left.
      // If the right node is a constant we can do the multiplication now.
      if (ASTNodeIsIntConstant(node->right)) {
        int64_t size = node->left->type->next->size;
        int64_t value = ASTNodeConstantValue(node->right);
        ASTNode* scale = NewIntConstantASTNode(value * size, node->right->type,
                                               node->base.location);
        node->right = scale;
      } else {
        ASTNode* scale = NewPtrScaleASTNode(node->left->type->next, AST_OP(mult),
                                            node->right, node->right->location);
        node->right = scale;
        scale->parent = (ASTNode*)node;
        ASTNodeSetType(scale, node->left->type);
      }
    } else {
      SemanticError((ASTNode*)node, "Can only add an integer to a pointer");
    }
  } else if (TypeIsPointerOrArray(node->right->type)) {
    ASTNodeSetType((ASTNode*)node,
                   PointerArithmeticResultType(node->right->type));
    if (ASTNodeIsPointerScaleMultiply(node->left)) {
      return &node->base;
    }
    if (TypeIsIntegral(node->left->type)) {
      // Scale left side by size of right.
      if (ASTNodeIsIntConstant(node->left)) {
         int64_t size = node->right->type->next->size;
         int64_t value = ASTNodeConstantValue(node->left);
         ASTNode* scale = NewIntConstantASTNode(value * size, node->left->type,
                                                node->base.location);
         node->left = scale;
       } else {
         ASTNode* scale = NewPtrScaleASTNode(
           node->right->type->next, AST_OP(mult), node->left, node->left->location);
         node->left = scale;
         scale->parent = (ASTNode*)node;
         ASTNodeSetType(scale, node->right->type);
       }
    } else {
      SemanticError((ASTNode*)node, "Can only add an integer to a pointer");
    }
  } else {
    InsertNumericConversions(node, true);
  }
  return &node->base;
}

// Like binary plus, a binary minus can subtract integers from pointers,
// but not the other way around.  It can also subtract two pointers.
// Contract: Overload attempt, pointer diff, or numeric conversions; may return
// replacement call node.
ASTNode* AnalyzeMinusOperator(BinaryASTNode* node) {
  if (node == NULL) {
    return &node->base;
  }
  AnalyzeBinaryExpression(node);
  if (TypeIsPointerOrArray(node->left->type)) {
    if (TypeIsIntegral(node->right->type)) {
      ASTNodeSetType((ASTNode*)node,
                     PointerArithmeticResultType(node->left->type));
      // Scale right side by size of left.
      // If the right node is a constant we can do the multiplication now.  The
      // scaled operand is a count of bytes, so it keeps the integer type it had
      // rather than taking the pointer's, as in binary plus above.
      if (ASTNodeIsIntConstant(node->right)) {
        int64_t size = node->left->type->next->size;
        int64_t value = ASTNodeConstantValue(node->right);
        ASTNode* scale = NewIntConstantASTNode(value * size, node->right->type,
                                               node->base.location);
        node->right = scale;
      } else {
        ASTNode* scale = NewPtrScaleASTNode(
            node->left->type->next, AST_OP(mult), node->right, node->right->location);
        node->right = scale;
        scale->parent = (ASTNode*)node;
        ASTNodeSetType(scale, node->left->type);
      }
    } else if (TypeIsPointerOrArray(node->right->type)) {
      // Pointer - pointer: divide the result by the size of the type
      // pointed to.  Both operands are pointer-or-array here, so an array
      // operand decays to a pointer to its element type; compare the pointed-to
      // element types rather than requiring identical declarators.
      if (node->left->type->next == NULL || node->right->type->next == NULL ||
          node->left->type->next->type != node->right->type->next->type) {
        SemanticTypeConversionError(
            node->left, node->right->type,
            "Illegal pointer subtraction; "
            "pointers are not the same type: '%s' and '%s'");
      } else {
        // Scale the pointer difference by the size of the type.
        // Tree goes from this:
        //                minus
        //                 / \
        //               x     y
        //
        // To:
        //             ptr_scale
        //              /
        //           minus
        //            / \
        //           x   y
        //

        ASTNode* parent = node->base.parent;
        ASTNode* scale = NewPtrScaleASTNode(node->right->type->next,
                                             AST_OP(div),
                                             ASTNodeMove(&node->base),
                                             node->right->location);
        ASTNodeReplaceChild(parent, node->base.child_id, scale, false);

        // The result of subtracting two pointers is the signed type
        // ptrdiff_t (`long` on this target), not an unsigned type.
        ASTNodeSetType(scale,
                       NewTypeRecordWithSize(kTypeLong, kQualPlain));
        return scale;
      }
    } else {
      SemanticError((ASTNode*)node, "Illegal pointer subtraction operation");
    }
  } else {
    InsertNumericConversions(node, true);
  }
  return &node->base;
}

// Integer-promote one operand of a shift in place.  A type narrower than int
// becomes int and not merely a wider version of itself, because int can hold
// every one of its values; the exception is an unsigned type int is no wider
// than, which promotes to unsigned int instead (C11 6.3.1.1p2).  Getting this
// wrong leaves `(unsigned short)x << 1` unsigned, and the whole point of
// 6.5.7p3 giving the result the promoted left type is that it is signed.
// Contract: Mutates `operand` via bit-field or integer promotion in place.
static void PromoteShiftOperand(ASTNode* operand) {
  if (PromoteBitFieldOperand(operand)) {
    return;
  }
  int rank = GetRank(operand->type);
  if (TypeIsBitInt(operand->type) || rank <= 0 || rank >= IntRank()) {
    return;
  }
  Type promoted = kTypeInt;
  if (TypeIsUnsigned(operand->type) &&
      operand->type->size >= SizeofType(kTypeInt)) {
    promoted |= kTypeUnsigned;
  }
  NormalConversion(operand, NewTypeRecordWithSize(promoted, kQualPlain));
}

// Both sides of a shift operator needs to be an integral type.
// Contract: Promotes operands, tries overloads, then integral conversions.
ASTNode* AnalyzeShift(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  bool vector_shift =
      TypeIsVector(node->left->type) && TypeIsVector(node->right->type) &&
      TypeEqual(node->left->type, node->right->type);
  if (!vector_shift) {
    SemanticCheckScalarType(node->left);
    SemanticCheckScalarType(node->right);
  }
  if (vector_shift) {
    if (!TypeIsIntegral(TypeVectorElement(node->left->type))) {
      SemanticError((ASTNode*)node, "Shift operator needs integral types");
    }
  } else if (!TypeIsIntegral(node->left->type) ||
             !TypeIsIntegral(node->right->type)) {
    SemanticError((ASTNode*)node, "Shift operator needs integral types");
  }

  // Each operand of a shift is integer-promoted independently and the type of
  // the result is the promoted type of the LEFT operand (C11 6.5.7p3).  The
  // usual arithmetic conversions are NOT applied, so the right operand's type
  // (e.g. a `long long` shift count) must not widen the result.
  if (!vector_shift) {
    PromoteShiftOperand(node->left);
    PromoteShiftOperand(node->right);
  }
  ASTNodeSetType((ASTNode*)node, node->left->type);

  // Convert node opcode to correct shift type.   An unsigned type uses a
  // logical shift an a signed type uses an arithmetic (sign extension) shift.
  if (node->base.op == AST_OP(rshift)) {
    // Right shift only.
    TypeRecord* signedness_type =
        TypeIsVector(node->base.type) ? TypeVectorElement(node->base.type)
                                     : node->base.type;
    if (TypeIsUnsigned(signedness_type)) {
      node->base.op = AST_OP(rshiftl);
    } else {
      node->base.op = AST_OP(rshifta);
    }
  }
  return (ASTNode*)node;
}

// Bitwise operators need integers.
// Contract: Overloads then `InsertNumericConversions` on integral operands.
ASTNode* AnalyzeBitwiseOperator(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  bool vector_bitwise =
      TypeIsVector(node->left->type) && TypeIsVector(node->right->type) &&
      TypeEqual(node->left->type, node->right->type);
  if (!vector_bitwise) {
    SemanticCheckScalarType(node->left);
    SemanticCheckScalarType(node->right);
  }
  if (vector_bitwise) {
    if (!TypeIsIntegral(TypeVectorElement(node->left->type))) {
      SemanticError((ASTNode*)node, "Bitwise operator needs integral types");
    } else {
      InsertNumericConversions(node, true);
    }
  } else if (!TypeIsIntegral(node->left->type) ||
             !TypeIsIntegral(node->right->type)) {
    SemanticError((ASTNode*)node, "Bitwise operator needs integral types");
  } else {
    InsertNumericConversions(node, true);
  }
  return (ASTNode*)node;
}

// Contract: Read-only literal zero test for null-pointer-constant rules.
static bool IsZeroIntegerConstant(ASTNode* node) {
  return node != NULL &&
         (node->op == AST_OP(number) || node->op == AST_OP(charconst)) &&
         ((ConstantASTNode*)node)->value.ivalue == 0;
}

// [conv.ptr]: in C++ a null pointer constant is an integer literal with value
// zero, not a folded const variable.  `const CharT nul = 0; s.find(nul)` must
// select `find(CharT)` rather than tying `find(const CharT*)`.
// Contract: Read-only shape/type check for [conv.ptr] null pointer constants.
bool IsNullPointerConstant(ASTNode* node) {
  if (!IsZeroIntegerConstant(node)) {
    return false;
  }
  if (!CompilerIsCXX()) {
    return true;
  }
  if (node->op == AST_OP(charconst)) {
    return false;
  }
  if (node->type != NULL && (node->type->qualifiers & kQualConst) != 0) {
    return false;
  }
  return true;
}

// Returns true if `op_name` is a viable binary operator for the ordered operand
// pair `(left, right)`: either a member operator on `left`'s class callable with
// `right`, or a free operator found by ordinary lookup / ADL over `(left,
// right)`.  Used to decide the orientation of a C++20 rewritten comparison
// without committing to (and possibly mis-diagnosing) a non-viable rewrite.
// Contract: Speculative overload viability only; no AST mutation (diagnostics
// suppressed for ADL lookup).
static bool BinaryOperatorViableForOperands(ASTNode* left, ASTNode* right,
                                            const char* op_name) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (TypeIsStructOrUnion(left->type) && left->type->info.struct_info != NULL) {
    StructMember* first =
        FindStructMemberByName(left->type->info.struct_info, op_name);
    if (first != NULL && first->is_member_function) {
      Vector children;
      VectorInit(&children);
      VectorAppend(&children, right);
      VectorASTNode call = {0};
      call.children = &children;
      bool viable = false;
      for (StructMember* candidate = first; candidate != NULL;
           candidate = candidate->overload_next) {
        if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
            !TypeIsFunction(candidate->symbol->type)) {
          continue;
        }
        if (candidate->symbol->flags.is_template ||
            FunctionCallScore(candidate->symbol->type, &call,
                              /*first_formal_arg=*/1) >= 0) {
          viable = true;
          break;
        }
      }
      VectorDestruct(&children);
      if (viable) {
        return true;
      }
    }
  }
  String name;
  StringInit(&name, op_name);
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, left);
  VectorAppend(&actuals, right);
  Symbol* function =
      ResolveFreeFunctionWithADL(&name, &actuals, /*diagnose_ambiguous=*/false);
  VectorDestruct(&actuals);
  StringDestruct(&name);
  return function != NULL && TypeIsFunction(function->type);
}

// C++20 rewritten comparison candidates ([over.match.oper]): when a relational
// or `!=` operator has a class operand with no directly-usable overload, rewrite
// it in terms of `operator<=>` / `operator==`:
//   a @ b   (@ in < > <= >=)  ->  (a <=> b) @ 0   or   0 @ (b <=> a) reversed
//   a != b                    ->  !(a == b)       (the `==` may reverse below)
// `==` itself is left to operator== (see TryReversedComparisonOperator for the
// reversed `b == a` candidate), matching the standard.
// Contract: May replace comparison node with rewritten subtree via
// `ReplaceBinaryWithCall`; moves original operands into new tree.
ASTNode* TryRewriteComparisonOperator(BinaryASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL || node->right == NULL) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(node->left->type) &&
      !TypeIsStructOrUnion(node->right->type)) {
    return NULL;
  }
  ASTOpcode op = node->base.op;
  SourceLocation loc = node->base.location;

  if (op == AST_OP(noteq)) {
    ASTNode* left = ASTNodeMove(node->left);
    ASTNode* right = ASTNodeMove(node->right);
    ASTNode* eq = NewBinaryASTNode(AST_OP(equal), NULL, loc, left, right);
    ASTNode* negated = NewUnaryASTNode(AST_OP(not), NULL, loc, eq);
    return ReplaceBinaryWithCall(node, negated);
  }
  if (op == AST_OP(less) || op == AST_OP(greater) || op == AST_OP(lesseq) ||
      op == AST_OP(greatereq)) {
    // Prefer the as-written rewrite `(a <=> b) @ 0`.  Only when that spaceship
    // is not viable but the reversed one is, synthesize `0 @ (b <=> a)`.
    bool forward_viable =
        BinaryOperatorViableForOperands(node->left, node->right, "operator<=>");
    bool reverse_viable =
        !forward_viable &&
        BinaryOperatorViableForOperands(node->right, node->left, "operator<=>");
    ASTNode* left = ASTNodeMove(node->left);
    ASTNode* right = ASTNodeMove(node->right);
    ASTNode* zero = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), loc);
    ASTNode* rewritten;
    if (reverse_viable) {
      ASTNode* cmp =
          NewBinaryASTNode(AST_OP(spaceship), NULL, loc, right, left);
      rewritten = NewBinaryASTNode(op, NULL, loc, zero, cmp);
    } else {
      ASTNode* cmp =
          NewBinaryASTNode(AST_OP(spaceship), NULL, loc, left, right);
      rewritten = NewBinaryASTNode(op, NULL, loc, cmp, zero);
    }
    return ReplaceBinaryWithCall(node, rewritten);
  }
  return NULL;
}

// C++20 reversed `==` candidate ([over.match.oper]): when `a == b` has no viable
// as-written `operator==`, try the reversed `b == a` (member on `b`'s class or a
// free operator matching the swapped operands).  The reversed node is flagged so
// it is not itself reversed again.  Reversed `!=` is handled by the `==` rewrite
// above whose inner `a == b` reaches this same path.
// Contract: Replaces node with swapped `==` and sets `kASTReversedComparison`.
static ASTNode* TryReversedComparisonOperator(BinaryASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL || node->right == NULL) {
    return NULL;
  }
  if ((node->base.flags & kASTReversedComparison) != 0) {
    return NULL;
  }
  if (node->base.op != AST_OP(equal)) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(node->left->type) &&
      !TypeIsStructOrUnion(node->right->type)) {
    return NULL;
  }
  if (!BinaryOperatorViableForOperands(node->right, node->left, "operator==")) {
    return NULL;
  }
  SourceLocation loc = node->base.location;
  ASTNode* left = ASTNodeMove(node->left);
  ASTNode* right = ASTNodeMove(node->right);
  ASTNode* reversed = NewBinaryASTNode(AST_OP(equal), NULL, loc, right, left);
  reversed->flags |= kASTReversedComparison;
  return ReplaceBinaryWithCall(node, reversed);
}

// A pointer only compares against another pointer or a null pointer constant.
// C++ has no implicit conversion between pointers and integers, so an operand
// pair like `int* != long` makes the comparison ill-formed -- which is what
// makes `sentinel_for<long, int*>` false and therefore what keeps overload sets
// such as `ranges::advance(i, n)` versus `ranges::advance(i, bound)`
// unambiguous.  C only makes it a constraint violation, so warn there.
// Contract: Diagnoses/warns only; does not convert operands.
static void CheckComparisonOfPointerAndInteger(BinaryASTNode* node) {
  ASTNode* pointer = NULL;
  ASTNode* integer = NULL;
  if (TypeIsPointer(node->left->type) && TypeIsIntegral(node->right->type)) {
    pointer = node->left;
    integer = node->right;
  } else if (TypeIsIntegral(node->left->type) &&
             TypeIsPointer(node->right->type)) {
    pointer = node->right;
    integer = node->left;
  }
  if (pointer == NULL || IsNullPointer(integer)) {
    return;
  }
  if (CompilerIsCXX()) {
    SemanticError((ASTNode*)node,
                  "Comparison between a pointer and an integer");
  } else {
    SemanticWarning((ASTNode*)node, "int-conversion",
                    "comparison between a pointer and an integer");
  }
}

// Contract: Emits C++26 array-array comparison error; returns true when diagnosed.
static bool DiagnoseCXX26ArrayComparison(BinaryASTNode* node) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !TypeIsArray(node->left->type) || !TypeIsArray(node->right->type)) {
    return false;
  }
  SemanticError((ASTNode*)node,
                "comparison between two arrays is not allowed in C++26");
  return true;
}

// Score of the best free `op_name` for `(left, right)`, or -1 if none is
// viable.  Ambiguous sets still yield the tied score so a rewritten `==` can
// beat every `!=` candidate without diagnosing the worse tie.
// Contract: Speculative ADL/lookup scoring; no AST mutation.
static int BestFreeBinaryOperatorScore(const char* op_name, ASTNode* left,
                                       ASTNode* right) {
  if (left == NULL || right == NULL || op_name == NULL) {
    return -1;
  }
  String name;
  StringInit(&name, op_name);
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, left);
  VectorAppend(&actuals, right);
  Symbol* function = ResolveFreeFunctionWithADL(
      &name, &actuals, /*diagnose_ambiguous=*/false);
  VectorDestruct(&actuals);
  StringDestruct(&name);
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type)) {
    return -1;
  }
  Vector children;
  VectorInit(&children);
  VectorAppend(&children, left);
  VectorAppend(&children, right);
  VectorASTNode call = {0};
  call.children = &children;
  int score = FunctionCallScore(function->type, &call, /*first_formal_arg=*/0);
  VectorDestruct(&children);
  return score;
}

// C++20 [over.match.oper]: `a != b` considers `!(a == b)` in the same set as
// `operator!=`.  A non-rewritten candidate wins a tie; a strictly better `==`
// (string_view has `==` and `<=>` but no `!=`) wins over conversion-ranked
// `operator!=` overloads such as `optional`'s, which would otherwise be an
// ambiguous tie among themselves.
// Contract: Read-only overload scoring to prefer rewritten `==`; no AST changes.
bool RewrittenEqualityBeatsInequality(BinaryASTNode* node) {
  if (node->base.op != AST_OP(noteq)) {
    return false;
  }
  if (CXXOperatorOperandClassType(node->left->type) == NULL &&
      CXXOperatorOperandClassType(node->right->type) == NULL) {
    return false;
  }
  int eq_score =
      BestFreeBinaryOperatorScore("operator==", node->left, node->right);
  if (eq_score < 0) {
    eq_score =
        BestFreeBinaryOperatorScore("operator==", node->right, node->left);
  }
  if (eq_score < 0) {
    return false;
  }
  int member_score = -1;
  bool member_ambiguous = false;
  bool member_template = false;
  bool member_viable = BestBinaryMemberOperatorScore(
      node, "operator!=", &member_score, &member_ambiguous, &member_template);
  // A template member's conversion cost is not scored here.  Keep it instead
  // of guessing that a rewritten `==` is better.
  if (member_template) {
    return false;
  }
  // Member scores cover only the explicit parameter, so an exact member `!=`
  // ranks at or below a two-argument free `==`.  Prefer that non-rewritten
  // candidate.
  if (member_viable && member_score >= 0 && member_score <= eq_score) {
    return false;
  }
  int ne_score = BestFreeBinaryOperatorScore("operator!=", node->left,
                                             node->right);
  if (ne_score >= 0 && ne_score <= eq_score) {
    return false;
  }
  return true;
}

// Contract: Member-pointer lhs with member-pointer or null rhs; may replace rhs
// with encoded null, fold to a constant, or set logical result type. When true,
// `*result` is the analyzed root (constant or `node`).
static bool AnalyzeComparisonOperatorFinishMemberPointerLeft(
    BinaryASTNode* node, ASTNode** result) {
  if (!(TypeIsMemberPointer(node->left->type) &&
        (TypeIsMemberPointer(node->right->type) ||
         (node->right != NULL &&
          (TypeIsNullPointer(node->right->type) ||
           (IsIntConstant(node->right) &&
            ((ConstantASTNode*)node->right)->value.ivalue == 0)))))) {
    return false;
  }
  if (node->base.op != AST_OP(equal) &&
      node->base.op != AST_OP(noteq)) {
    SemanticError((ASTNode*)node,
                  "Only == and != are valid for pointers to members");
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    *result = (ASTNode*)node;
    return true;
  }
  TypeRecord* comparison_type = node->left->type;
  if (TypeIsMemberPointer(node->right->type) &&
      !TypeEqual(node->left->type, node->right->type)) {
    if (MemberPointerCanConvert(node->left->type, node->right->type,
                                /*is_cast=*/false, NULL)) {
      SemanticConvertType(node->left, node->right->type,
                          kConvertNormal);
      comparison_type = node->right->type;
    } else if (MemberPointerCanConvert(node->right->type, node->left->type,
                                       /*is_cast=*/false, NULL)) {
      SemanticConvertType(node->right, node->left->type,
                          kConvertNormal);
    } else {
      SemanticError((ASTNode*)node,
                    "Pointers to members of unrelated classes cannot be compared");
    }
  }
  SemanticCheckScalarType(node->left);
  if (TypeIsMemberPointer(node->right->type)) {
    SemanticCheckScalarType(node->right);
  } else {
    MemberPointerValue null_value;
    MemberPointerEncodeNull(comparison_type, &null_value);
    ASTNode* converted_null = NewIntConstantASTNode(
        null_value.ptr, comparison_type, node->right->location);
    ASTNodeReplaceChild((ASTNode*)node, 1, converted_null, true);
  }
  MemberPointerValue left_value;
  MemberPointerValue right_value;
  bool left_constant = MemberPointerTryEvaluateConstant(
      node->left, comparison_type, &left_value);
  bool right_constant = false;
  right_constant = MemberPointerTryEvaluateConstant(
      node->right, comparison_type, &right_value);
  if (left_constant && right_constant) {
    bool equal = MemberPointerValuesEqual(comparison_type, &left_value,
                                          &right_value);
    if (node->base.op == AST_OP(noteq)) {
      equal = !equal;
    }
    *result = NewIntConstantASTNode(
        equal ? 1 : 0, NewLogicalResultType(),
        node->base.location);
    return true;
  }
  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
  *result = (ASTNode*)node;
  return true;
}

// Contract: Member-pointer rhs with null/zero lhs; may fold to constant or set
// logical result type. When true, `*result` is the analyzed root.
static bool AnalyzeComparisonOperatorFinishMemberPointerRightNull(
    BinaryASTNode* node, ASTNode** result) {
  if (!(TypeIsMemberPointer(node->right->type) &&
        node->left != NULL &&
        (TypeIsNullPointer(node->left->type) ||
         (IsIntConstant(node->left) &&
          ((ConstantASTNode*)node->left)->value.ivalue == 0)))) {
    return false;
  }
  if (node->base.op != AST_OP(equal) &&
      node->base.op != AST_OP(noteq)) {
    SemanticError((ASTNode*)node,
                  "Only == and != are valid for pointers to members");
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    *result = (ASTNode*)node;
    return true;
  }
  SemanticCheckScalarType(node->right);
  MemberPointerValue null_value;
  MemberPointerEncodeNull(node->right->type, &null_value);
  ASTNode* converted_null = NewIntConstantASTNode(
      null_value.ptr, node->right->type, node->left->location);
  ASTNodeReplaceChild((ASTNode*)node, 0, converted_null, true);
  MemberPointerValue left_value;
  MemberPointerValue right_value;
  if (MemberPointerEncodeNull(node->right->type, &left_value) &&
      MemberPointerTryEvaluateConstant(node->right, node->right->type,
                                       &right_value)) {
    bool equal = MemberPointerValuesEqual(node->right->type, &left_value,
                                          &right_value);
    if (node->base.op == AST_OP(noteq)) {
      equal = !equal;
    }
    *result = NewIntConstantASTNode(
        equal ? 1 : 0, NewLogicalResultType(),
        node->base.location);
    return true;
  }
  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
  *result = (ASTNode*)node;
  return true;
}

// Contract: Both operands are `nullptr_t`; diagnoses ordering comparisons and
// sets logical result type. Returns true when this case applies.
static bool AnalyzeComparisonOperatorFinishNullptrTComparison(
    BinaryASTNode* node) {
  if (!(TypeIsNullPointer(node->left->type) &&
        TypeIsNullPointer(node->right->type))) {
    return false;
  }
  if (node->base.op != AST_OP(equal) &&
      node->base.op != AST_OP(noteq)) {
    SemanticError((ASTNode*)node,
                  "Only == and != are valid for %s",
                  CompilerIsCXX() ? "std::nullptr_t" : "nullptr_t");
  }
  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
  return true;
}

// Contract: Scalar/ pointer usual comparison path after rewrites: function
// decay, diagnostics, numeric conversions, logical result type.
static ASTNode* AnalyzeComparisonOperatorFinishScalarComparison(
    BinaryASTNode* node) {
  // A function designator compared with a null pointer constant (`f != 0`)
  // decays to a function pointer.
  for (int child = 0; child < 2; child++) {
    ASTNode* operand = child == 0 ? node->left : node->right;
    ASTNode* other = child == 0 ? node->right : node->left;
    if (operand != NULL && other != NULL && TypeIsFunction(operand->type) &&
        IsNullPointer(other)) {
      SourceLocation location = operand->location;
      ASTNode* address =
          NewAnalyzedBuiltinAddressOf(ASTNodeMove(operand), location);
      ASTNodeReplaceChild((ASTNode*)node, child, address, false);
    }
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
  CheckComparisonOfPointerAndInteger(node);
  if (TypeIsIntegral(node->left->type) && TypeIsIntegral(node->right->type) &&
      TypeIsUnsigned(node->left->type) != TypeIsUnsigned(node->right->type) &&
      !IsZeroIntegerConstant(node->left) && !IsZeroIntegerConstant(node->right)) {
    SemanticWarning((ASTNode*)node, "sign-compare",
                    "comparison of integers of different signs");
  }
  InsertNumericConversions(node, true);

  // Comparisons produce bool in C++ and int in C.
  ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
  return (ASTNode*)node;
}

// Contract: Analyzes operands, overload/rewrite paths for `==`/`!=`, vector
// and pointer rules; may replace node with a call. Sets bool/logical result type.
ASTNode* AnalyzeComparisonOperator(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  // `for (auto it = src.begin(), e = src.end(); it != e;)` inside a member
  // function template keeps undeduced `auto` until the member template's own
  // parameters are bound.  Comparing those placeholders is not a numeric
  // operation.
  if (ExpressionIsTemplateDependent((ASTNode*)node->left) ||
      ExpressionIsTemplateDependent((ASTNode*)node->right)) {
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  if ((TypeIsComplex(node->left->type) ||
       TypeIsComplex(node->right->type)) &&
      node->base.op != AST_OP(equal) &&
      node->base.op != AST_OP(noteq)) {
    SemanticError((ASTNode*)node,
                  "Only == and != are valid for complex operands");
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  if (DiagnoseCXX26ArrayComparison(node)) {
    ASTNodeSetType((ASTNode*)node, NewLogicalResultType());
    return (ASTNode*)node;
  }
  ASTNode* member_pointer_result = NULL;
  if (AnalyzeComparisonOperatorFinishMemberPointerLeft(node,
                                                       &member_pointer_result)) {
    return member_pointer_result;
  }
  if (AnalyzeComparisonOperatorFinishMemberPointerRightNull(
          node, &member_pointer_result)) {
    return member_pointer_result;
  }
  if (AnalyzeComparisonOperatorFinishNullptrTComparison(node)) {
    return (ASTNode*)node;
  }
  ASTNode* rewritten = TryRewriteComparisonOperator(node);
  if (rewritten != NULL) {
    return rewritten;
  }
  ASTNode* reversed = TryReversedComparisonOperator(node);
  if (reversed != NULL) {
    return reversed;
  }
  if (TypeIsVector(node->left->type) || TypeIsVector(node->right->type)) {
    InsertNumericConversions(node, true);
    if (TypeIsVector(node->left->type) &&
        TypeIsVector(node->right->type) &&
        TypeEqual(node->left->type, node->right->type)) {
      ASTNodeSetType((ASTNode*)node,
                     NewVectorComparisonResultType(node->left->type));
    }
    return (ASTNode*)node;
  }
  return AnalyzeComparisonOperatorFinishScalarComparison(node);
}

// C++20 three-way comparison `a <=> b`.  For class operands this resolves a
// user-declared/defaulted operator<=>; for scalar operands it yields a value of
// the appropriate comparison-category type from <compare>.
// Contract: May replace with overloaded `<=>` call; sets comparison category type.
ASTNode* AnalyzeThreeWayComparison(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  if (TypeIsComplex(node->left->type) ||
      TypeIsComplex(node->right->type)) {
    SemanticError((ASTNode*)node,
                  "Three-way comparison is not valid for complex operands");
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  if (DiagnoseCXX26ArrayComparison(node)) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
  // Apply the usual arithmetic / pointer conversions so both operands end up
  // with a common type that the cmp3way IR op can compare directly.
  InsertNumericConversions(node, true);
  // Built-in operands compare with strong ordering, except floating-point
  // operands (which may be unordered) that compare with partial ordering.
  const char* category_name = "strong_ordering";
  if (TypeIsFloatingPoint(node->left->type) ||
      TypeIsFloatingPoint(node->right->type)) {
    category_name = "partial_ordering";
  }
  TypeRecord* category_type = TypeFindCXXComparisonCategory(category_name);
  if (category_type == NULL) {
    SemanticError((ASTNode*)node,
                  "include <compare> to use the three-way comparison operator");
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  // The canonical tag type stores its size in struct_info; codegen reads
  // TypeRecord::size directly (e.g. when laying out the result temporary), so
  // hand the node a copy with the size materialized.
  ASTNodeSetType((ASTNode*)node,
                 TypeRecordCalculateSize(TypeRecordCopy(category_type)));
  return (ASTNode*)node;
}

// Contract: Classifies a `?:` arm for function/pointer/null rules; read-only.
static ConditionalFunctionArmKind GetConditionalFunctionArmKind(
    ASTNode* arm, TypeRecord** function_type) {
  *function_type = NULL;
  if (TypeIsFunction(arm->type)) {
    *function_type = arm->type;
    return kConditionalFunctionArmFunction;
  }
  if (TypeIsFunctionPointer(arm->type)) {
    *function_type = arm->type->next;
    return kConditionalFunctionArmFunctionPointer;
  }
  return IsNullPointer(arm) ? kConditionalFunctionArmNull
                            : kConditionalFunctionArmOther;
}

// Contract: Sets common function-pointer type on colon arms; may set
// `kASTNeedAddress` on function lvalues. Returns false to fall through.
static bool TryAnalyzeConditionalFunctionPointer(BinaryASTNode* node,
                                                 BinaryASTNode* colon) {
  TypeRecord* left_function = NULL;
  TypeRecord* right_function = NULL;
  ConditionalFunctionArmKind left_kind =
      GetConditionalFunctionArmKind(colon->left, &left_function);
  ConditionalFunctionArmKind right_kind =
      GetConditionalFunctionArmKind(colon->right, &right_function);

  if (left_kind == kConditionalFunctionArmOther ||
      right_kind == kConditionalFunctionArmOther ||
      (left_kind == kConditionalFunctionArmNull &&
       right_kind == kConditionalFunctionArmNull)) {
    return false;
  }

  TypeRecord* common_function =
      left_function != NULL ? left_function : right_function;
  if (common_function == NULL ||
      (left_function != NULL && right_function != NULL &&
       !TypeEqual(left_function, right_function))) {
    return false;
  }

  TypeRecord* common_type = NULL;
  bool owns_common_type = false;
  if (left_kind == kConditionalFunctionArmFunctionPointer) {
    common_type = colon->left->type;
  } else if (right_kind == kConditionalFunctionArmFunctionPointer) {
    common_type = colon->right->type;
  } else {
    common_type = NewPointerTo(kQualPlain, common_function);
    owns_common_type = true;
  }

  if (left_kind == kConditionalFunctionArmFunction) {
    colon->left->flags |= kASTNeedAddress;
  }
  if (right_kind == kConditionalFunctionArmFunction) {
    colon->right->flags |= kASTNeedAddress;
  }
  ASTNodeSetType(colon->left, common_type);
  ASTNodeSetType(colon->right, common_type);
  ASTNodeSetType((ASTNode*)colon, common_type);
  ASTNodeSetType((ASTNode*)node, colon->base.type);
  if (owns_common_type) {
    TypeRecordDelete(common_type);
  }
  return true;
}

// Contract: C-only composite pointer type for object pointers; sets prvalue types
// on colon/`?` nodes. Allocates/deletes temporary result type record.
static bool TryAnalyzeCConditionalObjectPointers(BinaryASTNode* node,
                                                 BinaryASTNode* colon) {
  if (CompilerIsCXX() ||
      !TypeIsPointerOrArray(colon->left->type) ||
      !TypeIsPointerOrArray(colon->right->type) ||
      colon->left->type->next == NULL || colon->right->type->next == NULL ||
      TypeIsFunction(colon->left->type->next) ||
      TypeIsFunction(colon->right->type->next)) {
    return false;
  }
  TypeRecord* left_pointee = colon->left->type->next;
  TypeRecord* right_pointee = colon->right->type->next;
  bool has_void =
      TypeIsVoid(left_pointee) || TypeIsVoid(right_pointee);
  Qualifiers cv_mask =
      kQualConst | kQualVolatile | kQualRestrict | kQualAtomic;
  if (!has_void &&
      !TypeEqualIgnoringTopLevelQualifierMask(left_pointee, right_pointee,
                                               cv_mask)) {
    return false;
  }
  TypeRecord* pointee =
      TypeRecordCopy(TypeIsVoid(left_pointee) ? left_pointee :
                     TypeIsVoid(right_pointee) ? right_pointee :
                     left_pointee);
  pointee->qualifiers =
      (left_pointee->qualifiers | right_pointee->qualifiers) & cv_mask;
  // The pointer owns the pointee from here on, so only the pointer is released.
  TypeRecord* result = NewPointerTo(kQualPlain, pointee);
  ASTNodeSetType((ASTNode*)colon, result);
  ASTNodeSetType((ASTNode*)node, result);
  colon->base.value_category = kValueCategoryPrvalue;
  node->base.value_category = kValueCategoryPrvalue;
  TypeRecordDelete(result);
  return true;
}

/* [expr.cond]/4.1-4.2: glvalue arms of the same category where one class is a
 * base of the other (`c ? derived : base`).  The derived arm binds directly to
 * a reference to the base when that is at least as cv-qualified, and the
 * result is a glvalue of the base. */
// Contract: May replace a colon arm with `TryBindReferenceToBaseSubobject` tree;
// updates types and glvalue categories on success.
static bool TryAnalyzeConditionalDerivedToBaseGlvalue(BinaryASTNode* node,
                                                      BinaryASTNode* colon) {
  ASTNode* left = colon->left;
  ASTNode* right = colon->right;
  if (!CompilerIsCXX() || left->value_category != right->value_category ||
      left->value_category == kValueCategoryPrvalue ||
      !TypeIsStructOrUnion(left->type) || !TypeIsStructOrUnion(right->type) ||
      TypeEqualIgnoringQualifiers(left->type, right->type)) {
    return false;
  }
  Qualifiers cv_mask = kQualConst | kQualVolatile;
  int arm = -1;
  if (TypeIsDerivedFrom(left->type, right->type) &&
      (left->type->qualifiers & cv_mask & ~right->type->qualifiers) == 0) {
    arm = 0;
  } else if (TypeIsDerivedFrom(right->type, left->type) &&
             (right->type->qualifiers & cv_mask & ~left->type->qualifiers) ==
                 0) {
    arm = 1;
  }
  if (arm < 0) {
    return false;
  }
  ASTValueCategory category = left->value_category;
  ASTNode* derived = arm == 0 ? left : right;
  TypeRecord* base = arm == 0 ? right->type : left->type;
  int child_id = derived->child_id;
  ASTNode* bound = TryBindReferenceToBaseSubobject(derived, base);
  if (bound == NULL) {
    return false;
  }
  ASTNodeReplaceChild((ASTNode*)colon, child_id, bound, false);
  if (arm == 0) {
    colon->left = bound;
  } else {
    colon->right = bound;
  }
  ASTNodeSetType((ASTNode*)colon, base);
  ASTNodeSetType((ASTNode*)node, base);
  colon->base.value_category = category;
  node->base.value_category = category;
  return true;
}

// Contract: Analyzes and contextualizes the condition; locates the colon node.
// Returns true when analysis must stop (missing/invalid condition or colon).
static bool AnalyzeConditionalExpressionPhaseCondition(
    BinaryASTNode* node, BinaryASTNode** colon) {
  node->left = AnalyzeExpression(node->left);
  if (node->left == NULL) {
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return true;
  }
  SemanticConvertType(node->left, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  if (!TypeIsScalar(node->left->type)) {
    SemanticError((ASTNode*)node, "Condition for ? operator must be scalar");
    ASTNodeSetType((ASTNode*)node, node->left->type);
    return true;
  }
  *colon = (BinaryASTNode*)node->right;
  if (*colon == NULL || (*colon)->base.op != AST_OP(colon)) {
    ASTNodeSetType((ASTNode*)node, node->left->type);
    return true;
  }
  return false;
}

// Contract: Analyzes colon arms; handles a single surviving arm or missing types.
// Returns true when colon/`node` types are fully assigned.
static bool AnalyzeConditionalExpressionPhaseColonArms(
    BinaryASTNode* node, BinaryASTNode* colon) {
  colon->left = AnalyzeExpression(colon->left);
  colon->right = AnalyzeExpression(colon->right);
  if (colon->left == NULL || colon->right == NULL) {
    ASTNode* arm = colon->left != NULL ? colon->left : colon->right;
    TypeRecord* type = NULL;
    if (arm != NULL && arm->type != NULL) {
      type = arm->type;
    } else if (node->left->type != NULL) {
      type = node->left->type;
    } else {
      type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
      ASTNodeSetType((ASTNode*)colon, type);
      ASTNodeSetType((ASTNode*)node, type);
      TypeRecordDelete(type);
      return true;
    }
    ASTNodeSetType((ASTNode*)colon, type);
    ASTNodeSetType((ASTNode*)node, type);
    return true;
  }
  return false;
}

// Contract: throw-expression and void-arm rules from [expr.cond]; sets types and
// value categories. Returns true when a throw/void case finishes analysis.
static bool AnalyzeConditionalExpressionPhaseThrowAndVoid(
    BinaryASTNode* node, BinaryASTNode* colon) {
  if (colon->left->op == AST_OP(throw) && colon->right->op == AST_OP(throw)) {
    ASTNodeSetType((ASTNode*)colon, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    return true;
  }
  if (colon->left->op == AST_OP(throw)) {
    ASTNodeSetType((ASTNode*)colon, colon->right->type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    node->base.value_category = colon->right->value_category;
    return true;
  }
  if (colon->right->op == AST_OP(throw)) {
    ASTNodeSetType((ASTNode*)colon, colon->left->type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    node->base.value_category = colon->left->value_category;
    return true;
  }
  if (TypeIsVoid(colon->left->type) || TypeIsVoid(colon->right->type)) {
    // In C++ [expr.cond], if exactly one arm has void type -- and neither is a
    // throw-expression (handled above) -- the conditional is ill-formed.  In C,
    // GCC accepts a mixed void/non-void conditional as an extension and gives
    // the whole expression type void (this arises, for instance, when one arm
    // is a statement expression whose last statement is not an expression), so
    // we mirror that and do not diagnose.
    if (CompilerIsCXX() &&
        (!TypeIsVoid(colon->left->type) || !TypeIsVoid(colon->right->type))) {
      SemanticError((ASTNode*)node,
                    "Conditional operator with void expression requires both arms to be void");
    }
    ASTNodeSetType((ASTNode*)colon, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    return true;
  }
  return false;
}

// Contract: [expr.cond] class-arm converting constructors and conversion
// operators; may materialize prvalues on the colon node. Returns true when done.
static bool AnalyzeConditionalExpressionPhaseClassArmConversions(
    BinaryASTNode* node, BinaryASTNode* colon) {
  // [expr.cond]: when the arms differ and at least one has class type, convert
  // the other arm to that class if exactly one direction has a converting
  // constructor.  The class arm may already be a prvalue
  // (`p ? str : std::string(str, comma)`) or a glvalue
  // (`p ? *name : "UNKNOWN"`).  A glvalue class arm is copy-initialized so
  // both arms are prvalues; leaving it as an lvalue makes codegen memcpy the
  // object and return a pointer into the callee's frame.
  if (!(CompilerIsCXX() &&
        !TypeEqualIgnoringQualifiers(colon->left->type, colon->right->type) &&
        (TypeIsStructOrUnion(colon->left->type) ||
         TypeIsStructOrUnion(colon->right->type)))) {
    return false;
  }
  bool left_to_right =
      FindConvertingConstructorCandidate(
          colon->right->type, colon->left, /*allow_explicit=*/false,
          /*allow_same_class=*/false) != NULL;
  bool right_to_left =
      FindConvertingConstructorCandidate(
          colon->left->type, colon->right, /*allow_explicit=*/false,
          /*allow_same_class=*/false) != NULL;
  bool convert_left = left_to_right && !right_to_left;
  bool convert_right = right_to_left && !left_to_right;
  ASTNode* source = convert_left ? colon->left :
                    convert_right ? colon->right : NULL;
  TypeRecord* target = convert_left ? colon->right->type :
                       convert_right ? colon->left->type : NULL;
  ASTNode* other = convert_left ? colon->right :
                   convert_right ? colon->left : NULL;
  if (source != NULL &&
      TryConvertWithConvertingConstructorImpl(
          source, target, kConvertNormal, /*allow_same_class=*/false)) {
    if (other != NULL && other->type != NULL &&
        other->value_category != kValueCategoryPrvalue &&
        TypeIsStructOrUnion(other->type) &&
        TypeEqualIgnoringQualifiers(other->type, target)) {
      TryConvertWithConvertingConstructorImpl(
          other, target, kConvertNormal, /*allow_same_class=*/true);
    }
    TypeRecord* result_type = TypeRecordCopy(target);
    result_type->qualifiers = kQualPlain;
    ASTNodeSetType((ASTNode*)colon, result_type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    colon->base.value_category = kValueCategoryPrvalue;
    node->base.value_category = kValueCategoryPrvalue;
    TypeRecordDelete(result_type);
    return true;
  }
  // Conversion functions, not only converting constructors:
  // `cond ? string : string_view()` uses basic_string::operator string_view.
  bool left_op = ClassHasConversionOperatorTo(colon->left, colon->right->type);
  bool right_op = ClassHasConversionOperatorTo(colon->right, colon->left->type);
  if (left_op != right_op) {
    ASTNode* conv_source = left_op ? colon->left : colon->right;
    TypeRecord* conv_target =
        left_op ? colon->right->type : colon->left->type;
    SemanticConvertType(conv_source, conv_target, kConvertNormal);
    TypeRecord* result_type = TypeRecordCopy(conv_target);
    result_type->qualifiers = kQualPlain;
    ASTNodeSetType((ASTNode*)colon, result_type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    colon->base.value_category = kValueCategoryPrvalue;
    node->base.value_category = kValueCategoryPrvalue;
    TypeRecordDelete(result_type);
    return true;
  }
  return false;
}

// Contract: Matching glvalue category with identical arm types; propagates
// category to the conditional. Returns true when this case applies.
static bool AnalyzeConditionalExpressionPhaseMatchingGlvalueCategory(
    BinaryASTNode* node, BinaryASTNode* colon) {
  if (!(CompilerIsCXX() &&
        colon->left->value_category == colon->right->value_category &&
        colon->left->value_category != kValueCategoryPrvalue &&
        TypeEqual(colon->left->type, colon->right->type))) {
    return false;
  }
  ASTNodeSetType((ASTNode*)colon, colon->left->type);
  ASTNodeSetType((ASTNode*)node, colon->base.type);
  colon->base.value_category = colon->left->value_category;
  node->base.value_category = colon->left->value_category;
  return true;
}

// Contract: Same-class arms with mixed prvalue/glvalue; copy-initializes the
// lvalue arm when needed. Returns true when this case applies.
static bool AnalyzeConditionalExpressionPhaseSameClassPrvalueInit(
    BinaryASTNode* node, BinaryASTNode* colon) {
  if (!(CompilerIsCXX() &&
        TypeIsStructOrUnion(colon->left->type) &&
        TypeIsStructOrUnion(colon->right->type) &&
        TypeEqualIgnoringQualifiers(colon->left->type, colon->right->type))) {
    return false;
  }
  // [expr.cond]: when same-class operands do not already share a glvalue
  // category, an lvalue arm opposite a prvalue arm is copy-initialized into
  // a prvalue.  Merely assigning the common class type here leaves codegen
  // selecting between object addresses and can return a pointer into the
  // callee's dead stack frame.
  if (colon->left->value_category == kValueCategoryPrvalue &&
      colon->right->value_category != kValueCategoryPrvalue) {
    TryConvertWithConvertingConstructorImpl(
        colon->right, colon->left->type, kConvertNormal,
        /*allow_same_class=*/true);
  } else if (colon->right->value_category == kValueCategoryPrvalue &&
             colon->left->value_category != kValueCategoryPrvalue) {
    TryConvertWithConvertingConstructorImpl(
        colon->left, colon->right->type, kConvertNormal,
        /*allow_same_class=*/true);
  }
  TypeRecord* result_type = TypeRecordCopy(colon->left->type);
  result_type->qualifiers = kQualPlain;
  ASTNodeSetType((ASTNode*)colon, result_type);
  ASTNodeSetType((ASTNode*)node, colon->base.type);
  TypeRecordDelete(result_type);
  return true;
}

// Contract: Equal member pointers and compatible pointer/array arms; sets prvalue
// pointer result on the colon node. Returns true when either case applies.
static bool AnalyzeConditionalExpressionPhaseMemberPointerAndMixedPointer(
    BinaryASTNode* node, BinaryASTNode* colon) {
  if (CompilerIsCXX() && TypeIsMemberPointer(colon->left->type) &&
      TypeIsMemberPointer(colon->right->type) &&
      TypeEqual(colon->left->type, colon->right->type)) {
    ASTNodeSetType((ASTNode*)colon, colon->left->type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    colon->base.value_category = kValueCategoryPrvalue;
    node->base.value_category = kValueCategoryPrvalue;
    return true;
  }
  if (!(CompilerIsCXX() &&
        (TypeIsPointer(colon->left->type) || TypeIsArray(colon->left->type)) &&
        (TypeIsPointer(colon->right->type) || TypeIsArray(colon->right->type)))) {
    return false;
  }
  // [expr.cond]: mixed pointer/array arms decay to a prvalue pointer.
  // Leaving `p ? ptr_member : ""` as an lvalue of pointer type makes a
  // later by-value `const char*` argument take the address of the member
  // instead of loading the stored pointer.
  TypeRecord* left_pointee =
      TypeIsArray(colon->left->type) || TypeIsPointer(colon->left->type)
          ? colon->left->type->next
          : NULL;
  TypeRecord* right_pointee =
      TypeIsArray(colon->right->type) || TypeIsPointer(colon->right->type)
          ? colon->right->type->next
          : NULL;
  if (left_pointee != NULL && right_pointee != NULL) {
    Qualifiers cv_mask =
        kQualConst | kQualVolatile | kQualRestrict | kQualAtomic;
    if (TypeEqualIgnoringTopLevelQualifierMask(left_pointee, right_pointee,
                                               cv_mask) ||
        TypeIsVoid(left_pointee) || TypeIsVoid(right_pointee)) {
      TypeRecord* pointee = TypeRecordCopy(
          TypeIsVoid(left_pointee) ? left_pointee :
          TypeIsVoid(right_pointee) ? right_pointee : left_pointee);
      pointee->qualifiers =
          (left_pointee->qualifiers | right_pointee->qualifiers) & cv_mask;
      TypeRecord* result = NewPointerTo(kQualPlain, pointee);
      if (TypeIsArray(colon->left->type)) {
        colon->left->flags |= kASTNeedAddress;
      }
      if (TypeIsArray(colon->right->type)) {
        colon->right->flags |= kASTNeedAddress;
      }
      ASTNodeSetType(colon->left, result);
      ASTNodeSetType(colon->right, result);
      ASTNodeSetType((ASTNode*)colon, result);
      ASTNodeSetType((ASTNode*)node, result);
      colon->base.value_category = kValueCategoryPrvalue;
      node->base.value_category = kValueCategoryPrvalue;
      TypeRecordDelete(result);
      return true;
    }
  }
  return false;
}

// Contract: Default path — usual arithmetic conversions on the colon node,
// complex cast wrappers, and propagation of colon type/category to `node`.
static void AnalyzeConditionalExpressionPhaseUsualArithmetic(
    BinaryASTNode* node, BinaryASTNode* colon) {
  InsertNumericConversions(colon, false);
  if (colon->base.type != NULL && TypeIsComplex(colon->base.type)) {
    ASTNode* operands[] = {colon->left, colon->right};
    for (size_t i = 0; i < sizeof(operands) / sizeof(operands[0]); i++) {
      ASTNode* operand = operands[i];
      int child_id = operand->child_id;
      TypeRecord* target = TypeRecordCopy(colon->base.type);
      ASTNode* converted =
          NewCastASTNode(target, operand->location, operand);
      ASTNodeSetType(converted, target);
      converted->value_category = kValueCategoryPrvalue;
      converted->flags |= kASTAnalyzed;
      ASTNodeReplaceChild((ASTNode*)colon, child_id, converted, false);
    }
    colon->base.value_category = kValueCategoryPrvalue;
  }
  // [expr.cond]/7: when the operands have arithmetic (or unscoped enumeration)
  // type, the usual arithmetic conversions apply and the result is a prvalue of
  // the cv-unqualified common type (InsertNumericConversions strips the cv).
  if (colon->base.type != NULL &&
      (TypeIsIntegral(colon->base.type) ||
       TypeIsFloatingPoint(colon->base.type))) {
    colon->base.value_category = kValueCategoryPrvalue;
  }
  ASTNodeSetType((ASTNode*)node, colon->base.type);
  node->base.value_category = colon->base.value_category;
}

// Contract: Analyzes condition then colon arms; may rewrite arms for glvalue
// compatibility and run `InsertNumericConversions` on the colon node. Sets
// result type/category from the colon node (dependent cases may defer).
void AnalyzeConditionalExpression(BinaryASTNode* node) {
  BinaryASTNode* colon = NULL;
  if (AnalyzeConditionalExpressionPhaseCondition(node, &colon)) {
    return;
  }
  if (AnalyzeConditionalExpressionPhaseColonArms(node, colon)) {
    return;
  }
  if (AnalyzeConditionalExpressionPhaseThrowAndVoid(node, colon)) {
    return;
  }
  if (TryAnalyzeConditionalFunctionPointer(node, colon)) {
    return;
  }
  if (TryAnalyzeCConditionalObjectPointers(node, colon)) {
    return;
  }
  if (TryAnalyzeConditionalDerivedToBaseGlvalue(node, colon)) {
    return;
  }
  if (AnalyzeConditionalExpressionPhaseClassArmConversions(node, colon)) {
    return;
  }
  if (AnalyzeConditionalExpressionPhaseMatchingGlvalueCategory(node, colon)) {
    return;
  }
  if (AnalyzeConditionalExpressionPhaseSameClassPrvalueInit(node, colon)) {
    return;
  }
  if (AnalyzeConditionalExpressionPhaseMemberPointerAndMixedPointer(node,
                                                                      colon)) {
    return;
  }
  AnalyzeConditionalExpressionPhaseUsualArithmetic(node, colon);
}

// Contract: Dependent `new` braced-init assignment before lhs analysis; marks
// analyzed and returns the assignment node when deferral applies.
static ASTNode* AnalyzeAssignmentExpressionPhaseDependentBracedInit(
    BinaryASTNode* node) {
  if (CompilerIsCXX() && node->base.op == AST_OP(assign) &&
      node->right != NULL && node->right->op == AST_OP(braced_init) &&
      (node->base.flags & kASTDependentNewInitializer) != 0) {
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }
  return NULL;
}

// Contract: After lhs analysis, lowers `= { ... }` to a target temporary when
// appropriate; may defer dependent `new` or replace the rhs child (no delete).
// Returns true when rhs analysis must be skipped (dependent `new` deferral).
static bool AnalyzeAssignmentExpressionPhaseLowerBracedInitBeforeRhs(
    BinaryASTNode* node) {
  // `x = {...}` assigns a braced-init-list, which is not itself an expression.
  // Lower it to a temporary of the left-hand side's type so it is analyzed and
  // generated like `x = T{...}`.  (The parser only produces this for simple
  // `=`.)
  if (!(CompilerIsCXX() && node->base.op == AST_OP(assign) &&
        node->right != NULL && node->right->op == AST_OP(braced_init) &&
        node->left->type != NULL)) {
    return false;
  }
  // A dependent `new T(args...)` is stored as `*temp = {args...}` and the
  // template body is analyzed before instantiation.  The left-hand pointee
  // collapses to an unknown int, so list-initializing it reports "too many
  // initializers".  Leave the braced list for
  // RewriteClonedDependentNewInitializer once T is concrete.
  bool dependent_new =
      node->base.type != NULL &&
      (TypeContainsTemplateParameter(node->base.type) ||
       node->base.type->dependent_member_name != NULL);
  if (dependent_new) {
    node->base.flags |= kASTAnalyzed;
    return true;
  }
  TypeRecord* braced_target = node->left->type;
  // Symbol substitution can collapse the pointee to an unknown int and drop
  // the template-id.  Recover a now-concrete class from the assignment's own
  // type before list-initialization.
  if ((node->base.flags & kASTDependentNewInitializer) != 0 &&
      node->base.type != NULL &&
      node->base.type->template_origin != NULL) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->base.type);
    if (materialized != NULL && TypeIsStructOrUnion(materialized) &&
        (materialized->type & kTypeUnknown) == 0 &&
        !TypeContainsTemplateParameter(materialized)) {
      braced_target = materialized;
    }
  }
  ASTNode* lowered =
      LowerCXXBracedInitToTarget(node->right, braced_target);
  if (lowered != node->right) {
    ASTNodeReplaceChild((ASTNode*)node, 1, lowered, false);
    node->right = lowered;
  }
  return false;
}

// Contract: Constructor mem-initializer binding a reference data member; may
// replace rhs with materialized temporary. Returns non-NULL when finished.
static ASTNode* AnalyzeAssignmentExpressionPhaseReferenceMemberInitializer(
    BinaryASTNode* node) {
  // A constructor member-initializer that targets a reference data member BINDS
  // the reference -- it stores the address of the initializer into the member's
  // pointer slot -- rather than assigning through it.  A reference member access
  // keeps its declared reference type (T&) here, so without this the assignment
  // would resolve `this->ref = x` as `ref.operator=(x)` (for a class referent)
  // or a bogus `T&`-from-`T` conversion (for a scalar referent).  Mirror the
  // reference-initialization / reference-return contract: convert the
  // initializer to the referent type, take its address, and store into the slot
  // (node type T& selects `storea`).
  if (!(CompilerIsCXX() &&
        (((ASTNode*)node)->flags & kASTCXXMemberInitializer) != 0 &&
        node->base.op == AST_OP(assign) && node->left != NULL &&
        node->left->type != NULL && TypeIsReference(node->left->type))) {
    return NULL;
  }
  TypeRecord* reference_type = node->left->type;
  TypeRecord* initializer_type =
      TypeIsReference(node->right->type) ? node->right->type->next
                                         : node->right->type;
  if (!TypeEqualIgnoringQualifiers(initializer_type, reference_type->next)) {
    NormalConversion(node->right,
                     ReferenceConversionTarget(node->right, reference_type));
  }
  if (ReferenceCanBind(node->right, reference_type) &&
      !HasAddress(node->right)) {
    ASTNode* materialized =
        MaterializeTemporary(node->right, reference_type->next);
    ASTNodeReplaceChild((ASTNode*)node, 1, materialized, false);
    node->right = materialized;
  }
  node->left->flags |= kASTNeedAddress;
  node->right->flags |= kASTNeedAddress;
  ASTNodeSetType((ASTNode*)node, reference_type);
  return (ASTNode*)node;
}

// Contract: Built-in compound/simple assignment conversions after assignability
// checks; sets lhs need-address, atomic diagnostics, and opcode-specific rhs
// transforms. Does not handle overload rewrite.
static void AnalyzeAssignmentExpressionPhaseCompoundAssignmentOps(
    BinaryASTNode* node) {
  bool is_initializer = ((ASTNode*)node)->flags & kASTCXXMemberInitializer;
  if (!IsAssignable(node->left, is_initializer)) {
    SemanticError(node->left, "Cannot assign to this expression");
  }

  // We need the address of this node, not its value.
  node->left->flags |= kASTNeedAddress;

  if (TypeIsAtomic(node->left->type)) {
    bool supported_rmw =
        node->base.op == AST_OP(pluseq) || node->base.op == AST_OP(minuseq);
    if (node->base.op != AST_OP(assign) && !supported_rmw) {
      SemanticError(
          (ASTNode*)node,
          "atomic compound assignment is not supported for this operator");
    }
    if (supported_rmw && !TypeIsIntegral(node->left->type) &&
        !TypeIsPointer(node->left->type)) {
      SemanticError((ASTNode*)node,
                    "atomic arithmetic requires integral or pointer type");
    }
  }

  switch (node->base.op) {
    case AST_OP(assign):
      // During the first (class-level) instantiation of a member function
      // template, the enclosing template parameters are concrete but the
      // member template's own parameters are not, so an assignment such as a
      // constructor mem-initializer `__rep_ = value` may have a still-dependent
      // right-hand side (`value` has type `const Rep2&`).  Choosing an
      // arithmetic conversion now would bake in the wrong one (a dependent type
      // is treated as `int`, yielding a bogus `i2d`/`cvtsi2sd` once the real
      // argument is a floating type).  Leave the operand unconverted; the
      // member template's second-stage instantiation re-analyzes the
      // mem-initializer with the concrete argument type and inserts the correct
      // conversion.
      if (TypeContainsTemplateParameter(node->right->type) ||
          TypeContainsTemplateParameter(node->left->type)) {
        ((ASTNode*)node)->flags |= kASTDeferredDependentAssign;
      } else {
        NormalConversion(node->right, node->left->type);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);

      break;
    case AST_OP(pluseq):
    case AST_OP(minuseq):
      // Pointers are incremented or decremented by a scaled value.
      if (TypeIsPointer(node->left->type)) {
        if (!TypeIsIntegral(node->right->type)) {
          SemanticError((ASTNode*)node,
                        "Cannot add or subtract non-integers from pointers");
        }
        ASTNode* scale = NewPtrScaleASTNode(node->left->type->next,
                                            AST_OP(mult),
                                            node->right,
                                            node->right->location);
        node->right = scale;
        scale->parent = (ASTNode*)node;
        ASTNodeSetType(scale, node->left->type);
      } else {
        ConvertCompoundAssignmentOperand(node);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;
    case AST_OP(multeq):
    case AST_OP(diveq):
      ConvertCompoundAssignmentOperand(node);
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;
    case AST_OP(percenteq):
    case AST_OP(lshifteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
      if (!TypeIsIntegral(node->left->type) ||
          !TypeIsIntegral(node->right->type)) {
        SemanticError((ASTNode*)node, "Integer type expected");
      } else {
        NormalConversion(node->right, node->left->type);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;

    // For right shift we need to use either arithmetic or logical
    // shift depending on the type.
    case AST_OP(rshifteq):
      if (!TypeIsIntegral(node->left->type) ||
          !TypeIsIntegral(node->right->type)) {
        SemanticError((ASTNode*)node, "Integer type expected");
      } else {
        NormalConversion(node->right, node->left->type);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      if (TypeIsUnsigned(node->base.type)) {
        node->base.op = AST_OP(rshifteql);
      } else {
        node->base.op = AST_OP(rshifteqa);
      }
      break;

    default:
      assert(false);
  }
}

// Contract: Analyzes lhs then rhs; may lower braced-init on rhs, rewrite to
// overloaded operator call, or insert conversions. Returns root (often a call
// replacing the assignment node).
ASTNode* AnalyzeAssignmentExpression(BinaryASTNode* node) {
  ASTNode* early = AnalyzeAssignmentExpressionPhaseDependentBracedInit(node);
  if (early != NULL) {
    return early;
  }
  node->left = AnalyzeExpression(node->left);
  if (AnalyzeAssignmentExpressionPhaseLowerBracedInitBeforeRhs(node)) {
    return (ASTNode*)node;
  }
  node->right = AnalyzeExpression(node->right);

  ASTNode* reference_init =
      AnalyzeAssignmentExpressionPhaseReferenceMemberInitializer(node);
  if (reference_init != NULL) {
    return reference_init;
  }

  if (BinaryOperatorFunctionName(node->base.op) != NULL) {
    ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
    if (overloaded != NULL) {
      return overloaded;
    }
  }
  AnalyzeAssignmentExpressionPhaseCompoundAssignmentOps(node);
  if (CompilerIsCXX()) {
    node->base.value_category = kValueCategoryLvalue;
  }
  return (ASTNode*)node;
}

// Increment and decrement operators, both pre and post.
// Contract: Overloaded inc/dec rewrite or built-in lvalue update; may return call.
ASTNode* AnalyzeIncDec(UnaryASTNode* node) {
  ASTNode* overloaded = TryAnalyzeOverloadedIncDecOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  AnalyzeUnaryExpression(node);
  if (!IsAssignable(node->sub, false)) {
    SemanticError(node->sub, "Cannot increment or decrement this value");
  }
  if (TypeIsComplex(node->sub->type)) {
    SemanticError(node->sub,
                  "Cannot increment or decrement a complex value");
  }
  if (TypeIsAtomic(node->sub->type) &&
      !TypeIsIntegral(node->sub->type) &&
      !TypeIsPointer(node->sub->type)) {
    SemanticError(node->sub,
                  "atomic increment and decrement require integral or pointer type");
  }
  // NOTE: the scaling by the size of the pointer is handled by code
  // generation, not here.

  // Here we mark the sub node as needing the address, not value.
  node->sub->flags |= kASTNeedAddress;
  if (CompilerIsCXX() &&
      (node->base.op == AST_OP(preinc) || node->base.op == AST_OP(predec))) {
    node->base.value_category = kValueCategoryLvalue;
  }
  return (ASTNode*)node;
}

// Array subscripting operator.
// Contract: Analyzes base/index; overload, member pointer, or decay rules.
ASTNode* AnalyzeArraySubscript(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  if (CompilerIsCXX() && node->left != NULL &&
      (TypeIsUnknown(node->left->type) ||
       TypeContainsTemplateParameter(node->left->type))) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    node->base.value_category = kValueCategoryLvalue;
    return (ASTNode*)node;
  }
  if (CompilerIsCXX() && TypeIsStructOrUnion(node->left->type)) {
    StructMember* member =
        FindStructMemberByName(node->left->type->info.struct_info,
                               "operator[]");
    if (member != NULL && member->is_member_function) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, ASTNodeMove(node->right));
      ASTNode* call = NewOperatorMemberCall(ASTNodeMove(node->left),
                                            "operator[]", actuals,
                                            node->base.location);
      return ReplaceBinaryWithCall(node, call);
    }
    String name;
    StringInit(&name, "operator[]");
    Vector lookup_actuals;
    VectorInit(&lookup_actuals);
    VectorAppend(&lookup_actuals, node->left);
    VectorAppend(&lookup_actuals, node->right);
    Symbol* function = ResolveFreeFunctionWithADL(&name, &lookup_actuals,
                                                  /*diagnose_ambiguous=*/true);
    VectorDestruct(&lookup_actuals);
    if (function != NULL && TypeIsFunction(function->type)) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, ASTNodeMove(node->right));
      ASTNode* call = NewOperatorFreeCall(function, ASTNodeMove(node->left),
                                          actuals, node->base.location);
      StringDestruct(&name);
      return ReplaceBinaryWithCall(node, call);
    }
    StringDestruct(&name);
  }
  if (node->right != NULL && !TypeIsIntegral(node->right->type)) {
    SemanticError(node->right, "Subscripts must be integral types");
  }
  if (node->left != NULL && !TypeIsPointerOrArray(node->left->type) &&
      !TypeIsVector(node->left->type)) {
    SemanticError(node->left, "Can only subscript arrays, pointers, and vectors");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  // Dereference the array type.
  TypeRecord* subtype = node->left->type->next;
  ASTNodeSetType((ASTNode*)node, subtype);
  node->base.value_category = kValueCategoryLvalue;
  if (TypeIsVector(node->left->type)) {
    node->left->flags |= kASTNeedAddress;
  }
  return (ASTNode*)node;
}

// Contract: Chains `AnalyzeArraySubscript` over multiple index children; mutates
// `node->children` and final type/category on the vector node.
ASTNode* AnalyzeMultidimensionalSubscript(VectorASTNode* node) {
  // Analyze the receiver while detached.  VectorASTNode's legacy child
  // replacer indexes only the argument vector, so an in-place receiver rewrite
  // would otherwise replace argument zero.
  ASTNode* receiver = node->left;
  node->left = NULL;
  if (receiver != NULL) {
    receiver->parent = NULL;
  }
  ASTNode* analyzed_receiver = AnalyzeExpression(receiver);
  if (analyzed_receiver != receiver) {
    ASTNodeDelete(receiver);
  }
  node->left = analyzed_receiver;
  if (analyzed_receiver != NULL) {
    analyzed_receiver->parent = &node->base;
    analyzed_receiver->child_id = 0;
  }

  TypeRecord* receiver_type =
      analyzed_receiver != NULL ? analyzed_receiver->type : NULL;
  bool dependent_receiver =
      receiver_type == NULL || TypeIsUnknown(receiver_type) ||
      TypeContainsTemplateParameter(receiver_type);
  if (dependent_receiver || TypeIsStructOrUnion(receiver_type)) {
    Vector* actuals = NewVector();
    for (size_t i = 0; i < node->children->length; i++) {
      ASTNode* actual = (ASTNode*)VectorGet(node->children, i);
      VectorSet(node->children, i, NULL);
      if (actual != NULL) {
        actual->parent = NULL;
      }
      VectorAppend(actuals, actual);
    }
    node->left = NULL;
    if (analyzed_receiver != NULL) {
      analyzed_receiver->parent = NULL;
    }
    ASTNode* call =
        NewOperatorMemberCall(analyzed_receiver, "operator[]", actuals,
                              node->base.location);
    return ReplaceVectorWithCall(node, call);
  }

  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* old_index = (ASTNode*)VectorGet(node->children, i);
    ASTNode* index = AnalyzeExpression(old_index);
    if (index != old_index) {
      VectorSet(node->children, i, index);
      if (index != NULL) {
        index->parent = &node->base;
        index->child_id = (int)i;
      }
      ASTNodeDelete(old_index);
    }
  }
  SemanticError((ASTNode*)node,
                "Built-in subscripting requires exactly one index");
  ASTNodeSetType((ASTNode*)node,
                 NewTypeRecordWithSize(kTypeInt, kQualPlain));
  return (ASTNode*)node;
}
