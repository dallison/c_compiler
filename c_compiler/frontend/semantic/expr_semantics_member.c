//
//  expr_semantics_member.c
//  c_compiler
//
//  Member, member-pointer, address, sizeof, and typeid expressions.
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

static ASTNode* TryAnalyzeOverloadedArrowOperator(ASTNode* receiver, SourceLocation location);
static StructMember* MemberPointerExpressionMember(ASTNode* node);
static bool CurrentFunctionCountsMemberUses(void);
static bool MemberReferenceIsInitializerTarget(BinaryASTNode* node);
static bool ShouldCountMemberReferenceUse(BinaryASTNode* node);
static ASTValueCategory DataMemberAccessValueCategory(BinaryASTNode* node, StructMember* member);
static TypeRecord* SizeofOperandType(TypeRecord* type);
static TypeRecord* FindStdClassType(const char* name);
static ASTNode* MemberReferenceWhenTypeAlreadySet(BinaryASTNode* node);
static void PrepareMemberReferenceReceiver(BinaryASTNode* node);
static TypeRecord* MemberReferenceUnqualifiedReceiverType(BinaryASTNode* node);
static Struct* MemberReferenceReceiverStruct(TypeRecord* receiver_type);
static ASTNode* TryFinalizeQualifiedMemberReference(BinaryASTNode* node);
static ASTNode* TryFinalizeKnownMemberReference(BinaryASTNode* node);
static ASTNode* TryHandleDependentMemberReference(BinaryASTNode* node,
                                                  TypeRecord* receiver_type);
static Struct* ResolveMemberReferenceStructInfo(BinaryASTNode* node,
                                                TypeRecord* receiver_type);
static StructMember* LookupStructMemberForReference(
    BinaryASTNode* node, Struct* struct_info, TypeRecord* receiver_type,
    String* member_name, CXXAccess* access, Struct** member_owner,
    int* member_offset);
static ASTNode* FinalizeResolvedMemberReference(BinaryASTNode* node,
                                                StructMemberASTNode* member_node,
                                                StructMember* member);
static ASTNode* FinalizeLookupMemberReference(
    BinaryASTNode* node, Struct* struct_info, TypeRecord* receiver_type,
    StructMember* member, String* member_name, CXXAccess access,
    Struct* member_owner, int member_offset,
    Vector* explicit_template_arguments);


// Contract: C++ only. If `receiver` is a class type with `operator->`, builds and
// analyzes a call and returns the call result; otherwise returns NULL without
// mutating `receiver`.
static ASTNode* TryAnalyzeOverloadedArrowOperator(ASTNode* receiver,
                                                  SourceLocation location) {
  if (!CompilerIsCXX() || receiver == NULL ||
      !TypeIsStructOrUnion(receiver->type)) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(receiver->type->info.struct_info, "operator->");
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }

  ASTNode* call = NewOperatorMemberCall(ASTNodeMove(receiver), "operator->",
                                        NULL, location);
  return AnalyzeExpression(call);
}

// Contract: Walks casts to find the structmember under a member_ptr; does not
// consult symbol values (unlike MemberPointerExpressionMember).
StructMember* MemberPointerMemberFromExpression(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(member_ptr)) {
    UnaryASTNode* unary = (UnaryASTNode*)node;
    if (unary->sub != NULL && unary->sub->op == AST_OP(structmember)) {
      return ((StructMemberASTNode*)unary->sub)->member;
    }
    return NULL;
  }
  if (node->op == AST_OP(cast)) {
    return MemberPointerMemberFromExpression(((CastASTNode*)node)->expr);
  }
  return NULL;
}

// Contract: Resolves a structmember or a constant-folded member pointer identifier
// to its target StructMember; NULL if the expression is not a concrete pmf/pmd.
static StructMember* MemberPointerExpressionMember(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(member_ptr)) {
    UnaryASTNode* unary = (UnaryASTNode*)node;
    if (unary->sub != NULL && unary->sub->op == AST_OP(structmember)) {
      return ((StructMemberASTNode*)unary->sub)->member;
    }
    return NULL;
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    // Only a variable that cannot be reassigned still holds its initializer.
    if (id->symbol != NULL && TypeIsMemberPointer(id->symbol->type) &&
        (id->symbol->flags.is_constexpr || TypeIsConst(id->symbol->type)) &&
        id->symbol->flags.value_set &&
        id->symbol->flags.value_is_pointed_member) {
      return (StructMember*)id->symbol->value.other;
    }
  }
  return NULL;
}

// Contract: Type-checks `.*` / `->*`; may rewrite to dot/arrow member access or
// runtime lowering. Mutates `node` and may replace it entirely; caller owns result.
ASTNode* AnalyzeMemberPointerReference(BinaryASTNode* node) {
  if (node->right != NULL) {
    ASTNode* materialized =
        CXXMaterializeVariableTemplateExpression(node->right);
    if (materialized != NULL) {
      ASTNodeReplaceChild((ASTNode*)node, 1, materialized, true);
      node->right = materialized;
    }
  }
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  bool right_is_nttp =
      node->right != NULL && node->right->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node->right)->symbol != NULL &&
      ((IdentifierASTNode*)node->right)->symbol->flags.is_template_parameter &&
      !((IdentifierASTNode*)node->right)->symbol->flags.is_template_type_parameter;
  bool concrete_member_pointer =
      node->right != NULL && node->right->type != NULL &&
      TypeIsMemberPointer(node->right->type) &&
      !TypeContainsTemplateParameter(node->right->type) &&
      !TypeIsUnknown(node->right->type);
  if (!concrete_member_pointer &&
      (right_is_nttp || ExpressionIsTemplateDependent(node->right) ||
       (node->right != NULL && node->right->type != NULL &&
        (TypeContainsTemplateParameter(node->right->type) ||
         TypeIsUnknown(node->right->type))) ||
       (node->left != NULL && node->left->type != NULL &&
        TypeContainsTemplateParameter(node->left->type)))) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    return (ASTNode*)node;
  }
  if (node->right == NULL || !TypeIsMemberPointer(node->right->type)) {
    SemanticError((ASTNode*)node, "Right operand of .* / ->* is not a pointer to member");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  bool receiver_is_pointer = node->base.op == AST_OP(arrowstar);
  TypeRecord* receiver_type = node->left != NULL ? node->left->type : NULL;
  while (receiver_type != NULL && TypeIsReference(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  if (receiver_is_pointer) {
    if (receiver_type == NULL || !TypeIsPointer(receiver_type)) {
      SemanticError((ASTNode*)node, "Left operand of ->* is not a pointer");
      ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
      return (ASTNode*)node;
    }
    receiver_type = receiver_type->next;
  }

  Struct* pm_class = TypeMemberPointerClass(node->right->type);
  if (receiver_type == NULL || pm_class == NULL ||
      !TypeIsStructOrUnion(receiver_type)) {
    SemanticError((ASTNode*)node,
                  "Pointer-to-member object expression has incompatible class type");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  TypeRecord* receiver_class_type =
      NewTypeRecord(receiver_type->info.struct_info->is_union ? kTypeUnion
                                                                : kTypeStruct,
                    kQualPlain);
  TypeRecordSetStructInfo(receiver_class_type, receiver_type->info.struct_info);
  TypeRecord* pm_class_type =
      NewTypeRecord(pm_class->is_union ? kTypeUnion : kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(pm_class_type, pm_class);
  CXXBaseAdjustment member_base_adjustment;
  bool compatible_class =
      TypeEqual(receiver_class_type, pm_class_type) ||
      TypeBaseAdjustment(receiver_class_type, pm_class_type,
                         /*public_only=*/true, &member_base_adjustment);
  if (!compatible_class) {
    String receiver_name;
    String member_class_name;
    StringInit(&receiver_name, NULL);
    StringInit(&member_class_name, NULL);
    TypeRecordToString(receiver_type, &receiver_name);
    TypeRecordToString(pm_class_type, &member_class_name);
    SemanticError((ASTNode*)node,
                  "Pointer-to-member object type '%s' is incompatible with "
                  "member class '%s'",
                  receiver_name.value, member_class_name.value);
    StringDestruct(&receiver_name);
    StringDestruct(&member_class_name);
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  TypeRecord* pointee = TypeMemberPointerPointeeType(node->right->type);
  if (pointee == NULL) {
    SemanticError((ASTNode*)node, "Invalid pointer-to-member type");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  StructMember* known_member = MemberPointerExpressionMember(node->right);
  if (known_member != NULL && node->right->op != AST_OP(identifier) &&
      known_member->symbol != NULL &&
      known_member->symbol->type != NULL && !known_member->is_member_function) {
    ASTOpcode access_op =
        node->base.op == AST_OP(arrowstar) ? AST_OP(arrow) : AST_OP(dot);
    ASTNode* old_right = node->right;
    ASTNode* old_left = node->left;
    node->base.op = access_op;
    node->right = NewStructMemberASTNode(known_member, node->base.location);
    ((StructMemberASTNode*)node->right)->byte_offset = known_member->byte_offset;
    ASTNodeDelete(old_right);
    // The ->* node may already carry the receiver's type.  Member access
    // returns immediately when a type is set, which would keep that receiver
    // type instead of the member's type.
    ASTNodeClearType((ASTNode*)node);
    ASTNode* analyzed = AnalyzeMemberReference(node);
    if (analyzed != (ASTNode*)node) {
      return analyzed;
    }
    node->left = old_left;
    return (ASTNode*)node;
  }

  if (TypeIsFunction(pointee)) {
    ASTNodeSetType((ASTNode*)node, TypeRecordCopy(pointee));
    node->base.value_category = kValueCategoryLvalue;
    return (ASTNode*)node;
  }

  ASTNode* member_ptr = node->right;
  MemberPointerValue pm_value;
  if (MemberPointerTryEvaluateConstant(member_ptr, member_ptr->type, &pm_value)) {
    member_ptr = NewIntConstantASTNode(pm_value.ptr, member_ptr->type,
                                        node->base.location);
    member_ptr = AnalyzeExpression(member_ptr);
  }

  ASTNode* result = MemberPointerApplyDataAccess(&compiler->syntax, node->base.location,
                                        node->left, receiver_is_pointer,
                                        member_ptr, TypeRecordCopy(pointee));
  result->value_category = kValueCategoryLvalue;
  return result;
}

// Contract: Validates `&Class::member` syntax and records the member offset; does
// not set the pm type (done by the caller of expression analysis).
void AnalyzePointerToMember(UnaryASTNode* node) {
  if (node->sub == NULL || node->sub->op != AST_OP(structmember)) {
    SemanticError((ASTNode*)node, "Invalid pointer-to-member expression");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)node->sub;
  if (member_node->member == NULL || member_node->member->symbol == NULL) {
    SemanticError((ASTNode*)node, "Invalid pointer-to-member expression");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  if ((member_node->base.flags & kASTNameIndependentLookupAmbiguous) != 0) {
    SemanticError((ASTNode*)node,
                  "reference to name-independent declaration '%s' is ambiguous",
                  member_node->member->symbol->name.value);
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  member_node->byte_offset = member_node->member->byte_offset;
  node->base.value_category = kValueCategoryPrvalue;
}

// Contract: Rewrites a pmf call `(obj.*pmf)(...)` into an ordinary member call when
// the pmf is syntactically known; otherwise delegates to runtime lowering.
bool LowerMemberPointerFunctionCall(VectorASTNode* node) {
  if (node->left == NULL ||
      (node->left->op != AST_OP(dotstar) &&
       node->left->op != AST_OP(arrowstar))) {
    return false;
  }
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  StructMember* member = MemberPointerExpressionMember(access->right);
  if (member != NULL && member->is_member_function &&
      member->symbol != NULL && member->symbol->type != NULL) {
    ASTOpcode access_op =
        access->base.op == AST_OP(arrowstar) ? AST_OP(arrow) : AST_OP(dot);
    ASTNode* member_node = NewStructMemberASTNode(member, access->base.location);
    ((StructMemberASTNode*)member_node)->byte_offset = member->byte_offset;
    node->left = NewBinaryASTNode(access_op, NULL, access->base.location,
                                  access->left, member_node);
    return LowerMemberFunctionCall(node);
  }

  return MemberPointerLowerRuntimeFunctionCall(
      node, access->left, access->base.op == AST_OP(arrowstar), access->right,
      access->right->type);
}

// A data member named while analyzing a compiler-synthesized special member
// (implicit or `= default` copy/move constructor, assignment operator, or
// destructor) is not a real use for -Wunused-private-field: those functions
// touch every member mechanically.  Only references in user-provided code
// count, matching clang's behavior.
static bool CurrentFunctionCountsMemberUses(void) {
  TypeRecord* function = compiler->current_function;
  if (function == NULL || !TypeIsFunction(function)) {
    // A reference outside any function (e.g. a namespace-scope initializer) is
    // genuine user code.
    return true;
  }
  FunctionInfo* info = &function->info.function;
  if (info->cxx_special_member_kind != kCXXSpecialMemberNone &&
      !info->is_user_provided) {
    return false;
  }
  return true;
}

// A member reference that is the target of a constructor member-initializer
// (the synthetic `this->member = ...` assignment) does not count as a use for
// -Wunused-private-field: clang reports fields that are initialized but never
// otherwise referenced.
static bool MemberReferenceIsInitializerTarget(BinaryASTNode* node) {
  ASTNode* parent = node->base.parent;
  // A class member's initializer is a constructor call whose child 0 is the
  // first argument; the member it initializes is inside the callee.
  return parent != NULL &&
         (parent->flags & kASTCXXMemberInitializer) != 0 &&
         parent->op != AST_OP(call) && node->base.child_id == 0;
}

// Whether a data-member reference at `node` should mark the member as used.
static bool ShouldCountMemberReferenceUse(BinaryASTNode* node) {
  return CurrentFunctionCountsMemberUses() &&
         !MemberReferenceIsInitializerTarget(node);
}

// Contract: For non-static data members, maps receiver value category to the
// access result category (lvalue vs xvalue); static members and `->` are lvalues.
static ASTValueCategory DataMemberAccessValueCategory(BinaryASTNode* node,
                                                       StructMember* member) {
  if (node == NULL || member == NULL || member->is_static ||
      node->base.op == AST_OP(arrow) || node->left == NULL ||
      node->left->value_category == kValueCategoryLvalue) {
    return kValueCategoryLvalue;
  }
  return kValueCategoryXvalue;
}

// `haystack.npos` reads the same constant as `std::string_view::npos`, which
// AnalyzeIdentifier folds; an in-class-initialized static const member has no
// out-of-class definition to load from.  Only a plain object name is folded
// away, since any other object expression must still be evaluated.
ASTNode* FoldStaticDataMemberReference(BinaryASTNode* node) {
  /* An instantiated template analyzes cloned expressions before attaching
   * them (`field->CPPTYPE_ENUM` in a function template body); the caller then
   * takes the returned constant. */
  if (node->right == NULL || node->right->op != AST_OP(structmember) ||
      node->left == NULL || node->left->op != AST_OP(identifier) ||
      node->base.type == NULL ||
      (node->base.flags & (kASTNeedAddress | kASTIsDeclaration)) != 0 ||
      (node->base.parent != NULL &&
       node->base.parent->op == AST_OP(address))) {
    return &node->base;
  }
  StructMember* member = ((StructMemberASTNode*)node->right)->member;
  if (member == NULL || !member->is_static || member->is_member_function ||
      member->symbol == NULL || !member->symbol->flags.value_set ||
      !TypeIsIntConstant(node->base.type)) {
    return &node->base;
  }
  ASTNode* const_node = NewIntConstantASTNode(
      member->symbol->value.ivalue, member->symbol->type, node->base.location);
  ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node, true);
  const_node->flags |= kASTAnalyzed;
  return const_node;
}

// Contract: If `node` already has a type (e.g. cloned dependent expr), refreshes
// data-member value category when needed and returns `node`; otherwise NULL.
static ASTNode* MemberReferenceWhenTypeAlreadySet(BinaryASTNode* node) {
  if (node->base.type == NULL) {
    return NULL;
  }
  // A cloned dependent expression can retain its resolved member and type
  // while its receiver changes from an lvalue to an xvalue. Recompute the
  // category because data-member access propagates that distinction.
  if (CompilerIsCXX() && node->right != NULL &&
      node->right->op == AST_OP(structmember)) {
    StructMember* member = ((StructMemberASTNode*)node->right)->member;
    if (member != NULL && !member->is_member_function &&
        member->symbol != NULL) {
      node->base.value_category =
          TypeIsReference(member->symbol->type)
              ? kValueCategoryLvalue
              : DataMemberAccessValueCategory(node, member);
    }
  }
  return (ASTNode*)node;
}

// Contract: Analyzes and normalizes `node->left` only (prvalue materialization,
// overloaded `operator->`, class-template materialization on the receiver type).
static void PrepareMemberReferenceReceiver(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  if (CompilerIsCXX() && node->base.op == AST_OP(dot) &&
      node->left != NULL && node->left->type != NULL &&
      TypeIsStructOrUnion(node->left->type) &&
      node->left->value_category == kValueCategoryPrvalue &&
      !HasAddress(node->left)) {
    node->left = MaterializeTemporary(node->left, node->left->type);
    node->left->parent = (ASTNode*)node;
    node->left->child_id = 0;
  }
  int overloaded_arrow_depth = 0;
  while (node->base.op == AST_OP(arrow) && node->left != NULL &&
         !TypeIsStructOrUnionPointer(node->left->type) &&
         overloaded_arrow_depth++ < 64) {
    ASTNode* overloaded_arrow =
        TryAnalyzeOverloadedArrowOperator(node->left, node->base.location);
    if (overloaded_arrow == NULL) {
      break;
    }
    node->left = overloaded_arrow;
    node->left->parent = (ASTNode*)node;
    node->left->child_id = 0;
  }
  // A class-template primary still carrying concrete template arguments (e.g.
  // `variant` + `<int,long>` on a lambda capture field) must be materialized
  // before member lookup, or we resolve against the primary's unrebased
  // member templates.
  if (node->left != NULL && node->left->type != NULL) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->left->type);
    if (materialized != node->left->type) {
      ASTNodeSetType(node->left, materialized);
    }
  }
}

// Contract: Receiver expression type for member lookup with references stripped.
// Arrow validation still needs the pointer wrapper, so callers peel it only
// when they specifically need the receiver class.
static TypeRecord* MemberReferenceUnqualifiedReceiverType(BinaryASTNode* node) {
  TypeRecord* receiver_type = node->left != NULL ? node->left->type : NULL;
  if (TypeIsReference(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  return receiver_type;
}

static Struct* MemberReferenceReceiverStruct(TypeRecord* receiver_type) {
  return receiver_type != NULL && TypeIsStructOrUnion(receiver_type)
             ? receiver_type->info.struct_info
             : NULL;
}

// Contract: Applies reference-member typing, const-on-const-object, static
// variable-template replacement, and data-member value category. Requires
// `member->symbol`. Returns a replacement AST for variable templates, else `node`.
static ASTNode* FinalizeResolvedMemberReference(BinaryASTNode* node,
                                                StructMemberASTNode* member_node,
                                                StructMember* member) {
  TypeRecord* member_type = member->symbol->type;
  if (!member->is_static && !member->is_member_function &&
      TypeIsReference(member_type) &&
      !MemberReferenceIsInitializerTarget(node)) {
    ASTNodeSetType((ASTNode*)node, member_type->next);
    node->base.value_category = kValueCategoryLvalue;
    return (ASTNode*)node;
  }
  if (!member->is_static && !member->is_member_function && !member->is_mutable &&
      MemberReceiverIsConst(node)) {
    member_type = TypeRecordCopy(member_type);
    SetQualifiers(member_type, GetQualifiers(member_type) | kQualConst);
  }
  if (member->is_static &&
      SymbolExtraOf(member->symbol)->variable_template != NULL &&
      member_node->template_arguments != NULL) {
    ASTNode* replaced = CXXTryReplaceWithVariableTemplateValue(
        (ASTNode*)node, member->symbol, member_node->template_arguments,
        node->base.location);
    if (replaced != NULL) {
      return replaced;
    }
  }
  ASTNodeSetType((ASTNode*)node, member_type);
  if (!member->is_member_function) {
    node->base.value_category = DataMemberAccessValueCategory(node, member);
  }
  return (ASTNode*)node;
}

// How a `receiver.Qualifier::member` access reaches the Qualifier subobject.
// Access is checked separately, so private and protected bases count.
static bool QualifierSubobjectAdjustment(Struct* receiver, Struct* qualifier,
                                         CXXBaseAdjustment* adjustment) {
  TypeRecord* from = NewTypeRecord(kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(from, receiver);
  TypeRecord* to = NewTypeRecord(kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(to, qualifier);
  bool found =
      TypeBaseAdjustment(from, to, /*public_only=*/false, adjustment);
  TypeRecordDelete(from);
  TypeRecordDelete(to);
  return found;
}

// Contract: Handles `Base::member` when the right child is already a structmember.
// Returns `node` when qualified lookup completes or errors; NULL to fall through.
static ASTNode* TryFinalizeQualifiedMemberReference(BinaryASTNode* node) {
  if (node->right == NULL || node->right->op != AST_OP(structmember)) {
    return NULL;
  }
  if ((node->right->flags & kASTQualifiedName) == 0) {
    return NULL;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)node->right;
  StructMember* member = member_node->member;
  TypeRecord* receiver_type = MemberReferenceUnqualifiedReceiverType(node);
  if (node->base.op == AST_OP(arrow) && TypeIsPointer(receiver_type)) {
    receiver_type = receiver_type->next;
  }
  Struct* receiver_struct = MemberReferenceReceiverStruct(receiver_type);
  if (member != NULL && member->symbol != NULL &&
      (node->right->flags & kASTNameIndependentLookupAmbiguous) != 0) {
    SemanticError((ASTNode*)node,
                  "reference to name-independent declaration '%s' is ambiguous",
                  member->symbol->name.value);
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  // `d.B<2>::value` names a concrete specialization.
  if (member_node->owner_type != NULL &&
      !TypeIsStructOrUnion(member_node->owner_type) &&
      member_node->owner_type->template_origin != NULL &&
      !TypeContainsTemplateParameter(member_node->owner_type)) {
    TypeRecord* concrete = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, member_node->owner_type);
    if (concrete != NULL && concrete != member_node->owner_type &&
        TypeIsStructOrUnion(concrete) && concrete->info.struct_info != NULL) {
      TypeRecordIncRef(concrete);
      TypeRecordDelete(member_node->owner_type);
      member_node->owner_type = concrete;
    } else if (concrete != NULL && concrete != member_node->owner_type) {
      TypeRecordDelete(concrete);
    }
  }
  // `expr.StorageT<I>::get` stores the alias template-id as owner_type.
  // Until that id materializes to a class, do not resolve `get` against
  // the primary pattern (that would bind the `&` overload and reject an
  // xvalue return of `T&&`).
  if (member_node->owner_type != NULL &&
      !TypeIsStructOrUnion(member_node->owner_type) &&
      (TypeContainsTemplateParameter(member_node->owner_type) ||
       member_node->owner_type->template_origin != NULL)) {
    TypeRecord* placeholder =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    ASTNodeSetType((ASTNode*)node, placeholder);
    return (ASTNode*)node;
  }
  Struct* qualified_owner =
      member_node->owner_type != NULL &&
              member_node->owner_type->info.struct_info != NULL
          ? member_node->owner_type->info.struct_info
          : NULL;
  if (qualified_owner == NULL && member != NULL && member->symbol != NULL &&
      member->symbol->type != NULL && TypeIsFunction(member->symbol->type) &&
      member->symbol->type->info.function.cxx_member_owner != NULL) {
    qualified_owner = member->symbol->type->info.function.cxx_member_owner;
  }
  if (member == NULL || member->symbol == NULL || qualified_owner == NULL) {
    return NULL;
  }
  if ((node->right->flags & kASTQualifiedName) == 0) {
    node->right->flags |= kASTQualifiedName;
  }
  CXXAccess access = kAccessPublic;
  Struct* member_owner = NULL;
  int member_offset = 0;
  StructMember* resolved = FindStructMemberWithAccessAndOffsetByName(
      qualified_owner, member->symbol->name.value, &access, &member_owner,
      &member_offset);
  if (resolved != NULL) {
    member = resolved;
    StructMemberASTNodeSetMember(member_node, resolved);
  }
  // `member_offset` is relative to the qualifier's subobject; `d.B<2>::value`
  // must not read `value` from the first base.
  CXXBaseAdjustment qualifier_path = {.kind = kCXXBaseAdjustmentNone};
  if (resolved != NULL && member_offset >= 0 && receiver_struct != NULL &&
      receiver_struct != qualified_owner &&
      QualifierSubobjectAdjustment(receiver_struct, qualified_owner,
                                   &qualifier_path) &&
      qualifier_path.kind == kCXXBaseAdjustmentVirtual) {
    int qualifier_relative_offset = member_offset;
    ApplyVirtualBaseAdjustmentToMemberReference(node, receiver_struct,
                                                qualified_owner, &member_offset);
    member_offset = qualifier_relative_offset;
  } else {
    if (resolved != NULL && member_offset >= 0 &&
        qualifier_path.kind == kCXXBaseAdjustmentStatic) {
      member_offset += qualifier_path.byte_offset;
    }
    if (receiver_struct != NULL) {
      ApplyVirtualBaseAdjustmentToMemberReference(node, receiver_struct,
                                                  member_owner, &member_offset);
    }
  }
  if (member_offset < 0) {
    member_offset =
        CXXBaseOffsetForMember(receiver_struct, member) + member->byte_offset;
  }
  member_node->access = access;
  member_node->byte_offset = member_offset;
  if (!member->is_member_function &&
      !CurrentFunctionCanAccessMember(receiver_struct, member_owner,
                                      member->access, access)) {
    const char* owner_name =
        member_owner != NULL && member_owner->tag_name != NULL
            ? member_owner->tag_name->value
            : "<anonymous>";
    SemanticError((ASTNode*)node, "%s is a %s member of %s",
                  member->symbol->name.value, CXXAccessName(access),
                  owner_name);
  }
  if (!member->is_member_function && member->symbol != NULL &&
      ShouldCountMemberReferenceUse(node)) {
    member->symbol->flags.used = true;
  }
  return FinalizeResolvedMemberReference(node, member_node, member);
}

// Contract: Completes analysis when the parser already bound a structmember on
// the right (unqualified). Returns `node` or a replacement; NULL to continue.
static ASTNode* TryFinalizeKnownMemberReference(BinaryASTNode* node) {
  if (node->right == NULL || node->right->op != AST_OP(structmember)) {
    return NULL;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)node->right;
  StructMember* member = member_node->member;
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  // A data member named in a member-access expression counts as used for
  // -Wunused-private-field (constructor member-initializers do not reach
  // this path, matching clang's "initialized but never used" reporting).
  if (!member->is_member_function && ShouldCountMemberReferenceUse(node)) {
    member->symbol->flags.used = true;
  }
  return FinalizeResolvedMemberReference(node, member_node, member);
}

// Contract: Defers typing for dependent receivers and scalar pseudo-destructors.
// Returns `node` when handled; NULL to proceed with concrete lookup.
static ASTNode* TryHandleDependentMemberReference(BinaryASTNode* node,
                                                   TypeRecord* receiver_type) {
  if (CompilerIsCXX() && receiver_type != NULL &&
      TypeContainsTemplateParameter(receiver_type)) {
    TypeRecord* placeholder = TypeRecordCopy(receiver_type);
    placeholder->type |= kTypeUnknown;
    ASTNodeSetType((ASTNode*)node, placeholder);
    return (ASTNode*)node;
  }
  if (CompilerIsCXX() && receiver_type != NULL &&
      (TypeIsUnknown(receiver_type) || TypeContainsAuto(receiver_type))) {
    TypeRecord* placeholder = TypeRecordCopy(receiver_type);
    placeholder->type |= kTypeUnknown;
    ASTNodeSetType((ASTNode*)node, placeholder);
    return (ASTNode*)node;
  }
  if (CompilerIsCXX() && node->base.op == AST_OP(arrow) &&
      TypeIsPointer(receiver_type) &&
      !TypeIsStructOrUnion(receiver_type->next) && node->right != NULL &&
      node->right->op == AST_OP(string)) {
    String* member_name = ((ConstantASTNode*)node->right)->value.string;
    if (member_name != NULL && member_name->length != 0 &&
        member_name->value[0] == '~') {
      // Leave a scalar pseudo-destructor member access deferred to its
      // enclosing call.  AnalyzeFunctionCall will replace the complete call
      // with the required object-evaluating void expression.
      TypeRecord* placeholder =
          NewTypeRecordWithSize(kTypeUnknown, kQualPlain);
      ASTNodeSetType((ASTNode*)node, placeholder);
      return (ASTNode*)node;
    }
  }
  return NULL;
}

// Contract: Validates `.` / `->` against `receiver_type`. On failure sets int
// type on `node` and returns NULL; otherwise returns the Struct* for lookup.
static Struct* ResolveMemberReferenceStructInfo(BinaryASTNode* node,
                                                TypeRecord* receiver_type) {
  Struct* struct_info = NULL;
  if (node->base.op == AST_OP(arrow)) {
    // Op is ->, needs to be a pointer to a struct/union.
    if (!TypeIsStructOrUnionPointer(receiver_type)) {
      SemanticError((ASTNode*)node,
                    "Left of -> is not a pointer to a "
                    "struct/union; did you mean to use '.'");
    } else {
      // Dereference the pointer to get the struct info.
      struct_info = receiver_type->next->info.struct_info;
    }
  } else if (!TypeIsStructOrUnion(receiver_type)) {
    if (TypeIsStructOrUnionPointer(receiver_type)) {
      SemanticError((ASTNode*)node,
                    "Left of '.' is a pointer; did you mean to use ->?");
    } else {
      SemanticError((ASTNode*)node, "Left of '.' is not a struct/union");
    }
  } else {
    // Node is AST_OP(dot) and left is a struct/union.
    struct_info = receiver_type->info.struct_info;
  }
  if (struct_info == NULL) {
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  }
  return struct_info;
}

// Contract: Name lookup on `struct_info`, including C++ constructor/destructor
// spellings. May rewrite `member_name`. On failure diagnoses and returns NULL.
static StructMember* LookupStructMemberForReference(
    BinaryASTNode* node, Struct* struct_info, TypeRecord* receiver_type,
    String* member_name, CXXAccess* access, Struct** member_owner,
    int* member_offset) {
  StructMember* member = FindStructMemberWithAccessAndOffsetByName(
      struct_info, member_name->value, access, member_owner, member_offset);
  if (member == NULL && CompilerIsCXX() && struct_info->tag_name != NULL) {
    TypeRecord* class_type = receiver_type;
    if (node->base.op == AST_OP(arrow) && TypeIsPointer(class_type)) {
      class_type = class_type->next;
    }
    const char* origin_name =
        class_type != NULL && class_type->template_origin != NULL
            ? class_type->template_origin->name.value
            : NULL;
    size_t origin_length = origin_name != NULL ? strlen(origin_name) : 0;
    const char* source_name = member_name->value;
    bool names_injected_constructor =
        origin_name != NULL &&
        strncmp(source_name, origin_name, origin_length) == 0 &&
        (source_name[origin_length] == '\0' ||
         source_name[origin_length] == '<' ||
         source_name[origin_length] == '#');
    // An explicit specialization is tagged `CordRepRef<I0>` and does not
    // record template_origin.  A call spelled `CordRepRef(...)` is still that
    // specialization's constructor.
    if (!names_injected_constructor && struct_info->tag_name != NULL) {
      const char* tag = struct_info->tag_name->value;
      const char* angle = strchr(tag, '<');
      size_t prefix = angle != NULL ? (size_t)(angle - tag) : 0;
      if (prefix > 0 && strlen(source_name) == prefix &&
          strncmp(source_name, tag, prefix) == 0) {
        names_injected_constructor = true;
      }
    }
    if (names_injected_constructor &&
        !StringEqual(member_name, struct_info->tag_name->value)) {
      member = FindStructMemberWithAccessAndOffsetByName(
          struct_info, struct_info->tag_name->value, access, member_owner,
          member_offset);
      if (member != NULL && member->symbol != NULL &&
          member->symbol->type != NULL &&
          TypeIsFunction(member->symbol->type) &&
          member->symbol->type->info.function.is_constructor) {
        StringSet(member_name, struct_info->tag_name->value);
      } else {
        member = NULL;
      }
    }
  }
  if (member == NULL && CompilerIsCXX() && member_name->length > 1 &&
      member_name->value[0] == '~') {
    String* base_tag = CXXFindBaseTagNameForDestructorSpelling(
        struct_info, member_name->value + 1);
    if (base_tag != NULL) {
      String concrete_name;
      StringInit(&concrete_name, "~");
      StringAppendString(&concrete_name, base_tag);
      StringSet(member_name, concrete_name.value);
      StringDestruct(&concrete_name);
      member = FindStructMemberWithAccessAndOffsetByName(
          struct_info, member_name->value, access, member_owner, member_offset);
    }
  }
  if (member == NULL && CompilerIsCXX() && member_name->length > 1 &&
      member_name->value[0] == '~' && struct_info->tag_name != NULL) {
    // A generated cleanup spells the primary template (`~basic_stringstream`).
    // The instantiation registers `~basic_stringstream<char, ...>`.
    const char* spelled = member_name->value + 1;
    const char* tag = struct_info->tag_name->value;
    size_t spelled_len = strlen(spelled);
    Symbol* origin = struct_info->tag_symbol != NULL &&
                             struct_info->tag_symbol->type != NULL
                         ? struct_info->tag_symbol->type->template_origin
                         : NULL;
    bool names_this_specialization =
        (strncmp(tag, spelled, spelled_len) == 0 && tag[spelled_len] == '<') ||
        (origin != NULL && strcmp(origin->name.value, spelled) == 0);
    if (names_this_specialization) {
      String concrete_name;
      StringInit(&concrete_name, "~");
      StringAppendString(&concrete_name, struct_info->tag_name);
      member = FindStructMemberWithAccessAndOffsetByName(
          struct_info, concrete_name.value, access, member_owner, member_offset);
      if (member != NULL) {
        StringSet(member_name, concrete_name.value);
      }
      StringDestruct(&concrete_name);
    }
  }
  if (member == NULL) {
    const char* suggestion =
        TypoCorrectionFindMemberName(struct_info, member_name->value);
    const char* tag =
        struct_info->tag_name != NULL ? struct_info->tag_name->value : "";
    if (suggestion != NULL) {
      SemanticError((ASTNode*)node,
                    "%s is not a member of struct/union %s; did you mean \"%s\"?",
                    member_name->value, tag, suggestion);
    } else {
      SemanticError((ASTNode*)node, "%s is not a member of struct/union %s",
                    member_name->value, tag);
    }
  }
  return member;
}

// Contract: After string-name lookup, replaces the right child with structmember,
// checks access/ambiguity, and completes typing. Returns `node` or replacement.
static ASTNode* FinalizeLookupMemberReference(
    BinaryASTNode* node, Struct* struct_info, TypeRecord* receiver_type,
    StructMember* member, String* member_name, CXXAccess access,
    Struct* member_owner, int member_offset,
    Vector* explicit_template_arguments) {
  if (member->symbol != NULL &&
      (node->right->flags & kASTNameIndependentLookupAmbiguous) != 0) {
    SemanticError((ASTNode*)node,
                  "reference to name-independent declaration '%s' is ambiguous",
                  member->symbol->name.value);
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  ApplyVirtualBaseAdjustmentToMemberReference(node, struct_info, member_owner,
                                              &member_offset);
  if (member_offset < 0) {
    member_offset = member->byte_offset;
  }
  // The vtable pointers are read by compiler-lowered virtual calls and
  // virtual-base adjustments, whatever the inheritance access of their base.
  bool vtable_pointer =
      member->symbol != NULL && member->symbol->flags.invented &&
      (StringStartsWith(&member->symbol->name, "__vptr") ||
       StringStartsWith(&member->symbol->name, "__vbptr"));
  if (!member->is_member_function && !vtable_pointer &&
      !CurrentFunctionCanAccessMember(struct_info, member_owner, member->access,
                                      access)) {
    const char* owner_name =
        member_owner != NULL && member_owner->tag_name != NULL
            ? member_owner->tag_name->value
            : "<anonymous>";
    SemanticError((ASTNode*)node, "%s is a %s member of %s",
                  member->symbol->name.value, CXXAccessName(access),
                  owner_name);
  }

  ASTNode* old_right = node->right;
  node->right = NewStructMemberASTNode(member, node->right->location);
  StructMemberASTNode* member_node = (StructMemberASTNode*)node->right;
  member_node->access = access;
  member_node->byte_offset = member_offset;
  member_node->template_arguments =
      TemplateArgumentVectorCopy(explicit_template_arguments);
  member_node->owner_type =
      member->symbol != NULL && member->symbol->type != NULL &&
              TypeIsFunction(member->symbol->type) &&
              member->symbol->type->info.function.cxx_member_owner != NULL &&
              member->symbol->type->info.function.cxx_member_owner->tag_symbol !=
                  NULL
          ? member->symbol->type->info.function.cxx_member_owner->tag_symbol->type
          : receiver_type;
  TypeRecordIncRef(member_node->owner_type);
  ASTNodeDelete(old_right);
  if (!member->is_member_function && member->symbol != NULL &&
      ShouldCountMemberReferenceUse(node)) {
    member->symbol->flags.used = true;
  }
  return FinalizeResolvedMemberReference(node, member_node, member);
}

// Contract: Types `.` / `->` member access. May replace `node`, rewrite children,
// or return early with dependent/unknown types; caller owns the returned AST.
ASTNode* AnalyzeMemberReference(BinaryASTNode* node) {
  ASTNode* already_typed = MemberReferenceWhenTypeAlreadySet(node);
  if (already_typed != NULL) {
    return already_typed;
  }

  PrepareMemberReferenceReceiver(node);
  node->right = AnalyzeExpression(node->right);

  ASTNode* qualified = TryFinalizeQualifiedMemberReference(node);
  if (qualified != NULL) {
    return qualified;
  }

  ASTNode* known = TryFinalizeKnownMemberReference(node);
  if (known != NULL) {
    return known;
  }

  TypeRecord* receiver_type = MemberReferenceUnqualifiedReceiverType(node);

  ASTNode* dependent = TryHandleDependentMemberReference(node, receiver_type);
  if (dependent != NULL) {
    return dependent;
  }

  Struct* struct_info =
      ResolveMemberReferenceStructInfo(node, receiver_type);
  if (struct_info == NULL) {
    return (ASTNode*)node;
  }

  if (TypeIsStructOrUnion(receiver_type)) {
    // Left is a struct, only need address.
    node->left->flags |= kASTNeedAddress;
  }

  String* member_name = ((ConstantASTNode*)node->right)->value.string;
  CXXAccess access = kAccessPublic;
  Struct* member_owner = NULL;
  int member_offset = 0;
  StructMember* member = LookupStructMemberForReference(
      node, struct_info, receiver_type, member_name, &access, &member_owner,
      &member_offset);
  if (member == NULL) {
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  Vector* explicit_template_arguments =
      ((ConstantASTNode*)node->right)->template_arguments;
  return FinalizeLookupMemberReference(node, struct_info, receiver_type, member,
                                       member_name, access, member_owner,
                                       member_offset, explicit_template_arguments);
}

// Contract: Forms `T*` from a glvalue operand; sets kASTNeedAddress on the subexpr
// and may deduce template member function types for `&fn`.
void AnalyzeAddressOperator(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  node->base.value_category = kValueCategoryPrvalue;
  // Forming a pointer to a function bakes its signature into the result type,
  // so a placeholder return type has to be resolved first.  This is how a
  // closure's `&C::operator()` reaches deduction guides such as std::function's.
  if (node->sub != NULL) {
    SemanticEnsureAutoReturnTypeDeduced(node->sub->type);
  }
  TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
  if (!HasAddress(node->sub)) {
    SemanticError(node->sub, "Cannot take the address of this expression");
    // Make a void* pointer type for this node.
    TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
    TypeRecordChain(ptr, void_type);
    ASTNodeSetType((ASTNode*)node, ptr);
    return;
  }
  TypeRecordChain(ptr, node->sub->type);
  // `*begin` of a still-unbound template parameter is an unknown type.  The
  // pointer formed by `&*begin` has to stay unknown too; otherwise overload
  // resolution sees a plain `int*` and rejects `Span<uint32_t>(&*begin, n)`
  // while the member function template is only being rebased onto its class.
  if (TypeIsUnknown(node->sub->type)) {
    ptr->type |= kTypeUnknown;
  }
  ASTNodeSetType((ASTNode*)node, ptr);

  // Tell downstream that we need the address of this node, not its
  // contents.
  node->sub->flags |= kASTNeedAddress;
  if (CompilerIsCXX() && node->sub->op == AST_OP(identifier) &&
      TypeIsFunction(node->sub->type)) {
    if (!AnalyzingTemplatePatternBody()) {
      TypeEnsureTemplateMemberFunctionDefinition(
          &compiler->syntax, ((IdentifierASTNode*)node->sub)->symbol);
    }
    IdentifierASTNode* identifier = (IdentifierASTNode*)node->sub;
    if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
        identifier->symbol != NULL && identifier->symbol->type != NULL &&
        TypeIsFunction(identifier->symbol->type) &&
        identifier->symbol->type->info.function.is_consteval &&
        !CXXInImmediateFunctionContext() &&
        (!CompilerCXXAtLeast(kLanguageStandardCXX23) ||
         !CXXEscalateCurrentFunction())) {
      SemanticError(node->sub,
                    "immediate function may only be named in an immediate "
                    "function context");
    }
  }
}

// Contract: Dereferences pointer/array (or applies function-to-pointer for `*f`);
// leaves dependent operands unknown; materializes class-template pointees on `*p`.
void AnalyzeContentsOperator(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  // A dependent operand (a template parameter or otherwise unknown type, such
  // as `*declval<T&>()` inside a decltype) has no known pointee yet.  Its
  // built-in or overloaded meaning is resolved after substitution, so leave the
  // result dependent rather than diagnosing it here.
  if (CompilerIsCXX() && node->sub->type != NULL &&
      (TypeIsUnknown(node->sub->type) ||
       TypeContainsAuto(node->sub->type) ||
       TypeContainsTemplateParameter(node->sub->type))) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    node->base.value_category = kValueCategoryLvalue;
    return;
  }
  // `*f` applies the function-to-pointer conversion, designating f itself.
  if (node->sub->type != NULL && TypeIsFunction(node->sub->type)) {
    ASTNode* function = node->sub;
    if (function->op == AST_OP(contents)) {
      node->sub = ASTNodeMove(((UnaryASTNode*)function)->sub);
      node->sub->parent = (ASTNode*)node;
      ASTNodeDelete(function);
    } else {
      TypeRecord* pointer_type =
          NewPointerTo(kQualPlain, TypeRecordCopy(function->type));
      function->flags |= kASTNeedAddress;
      node->sub = NewUnaryASTNode(AST_OP(address), NULL, function->location,
                                  function);
      ASTNodeSetType(node->sub, pointer_type);
      node->sub->parent = (ASTNode*)node;
      node->sub->flags |= kASTAnalyzed;
      node->sub->value_category = kValueCategoryPrvalue;
    }
  }
  if (!TypeIsPointerOrArray(node->sub->type)) {
    SemanticError(node->sub, "Cannot take contents of this expression");
    // Fake an integer type for the result.
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }

  TypeRecord* pointee = node->sub->type->next;
  if (pointee == NULL) {
    if (CompilerIsCXX() && node->sub->type != NULL &&
        (TypeIsUnknown(node->sub->type) ||
         TypeContainsAuto(node->sub->type) ||
         TypeContainsTemplateParameter(node->sub->type))) {
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
      node->base.value_category = kValueCategoryLvalue;
      return;
    }
    SemanticError(node->sub, "Cannot take contents of this expression");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  TypeRecord* materialized =
      TypeMaterializeClassTemplateSpecialization(&compiler->syntax, pointee);
  if (materialized != pointee) {
    // Keep the pointer spine consistent with the materialized pointee so
    // subsequent member access sees the concrete specialization.
    TypeRecord* ptr = TypeRecordCopy(node->sub->type);
    TypeRecordIncRef(materialized);
    TypeRecordDelete(ptr->next);
    ptr->next = materialized;
    ptr->type = materialized->type;
    ASTNodeSetType(node->sub, TypeRecordCalculateSize(ptr));
    pointee = materialized;
  }
  ASTNodeSetType((ASTNode*)node, pointee);
  node->base.value_category = kValueCategoryLvalue;
}

// Contract: Returns the type measured by sizeof/alignof without allocating;
// in C++ peels references to the referent; ownership stays with the type chain.
static TypeRecord* SizeofOperandType(TypeRecord* type) {
  return CompilerIsCXX() && TypeIsReference(type) ? type->next : type;
}

// Contract: Validates `pack...[index]`; index must be a constant when non-dependent.
void AnalyzePackIndexExpression(BinaryASTNode* node) {
  bool indexes_template_name =
      node->left != NULL && node->left->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node->left)->symbol != NULL &&
      ((IdentifierASTNode*)node->left)
          ->symbol->flags.is_template_template_parameter;
  if (!indexes_template_name) {
    node->left = AnalyzeExpression(node->left);
  }
  node->right = AnalyzeExpression(node->right);

  if (node->left == NULL || node->left->op != AST_OP(identifier) ||
      ((IdentifierASTNode*)node->left)->symbol == NULL ||
      !((IdentifierASTNode*)node->left)->symbol->flags.is_parameter_pack) {
    SemanticError((ASTNode*)node,
                  "pack indexing requires an unexpanded parameter pack name");
  }

  int64_t index = 0;
  if (!ExpressionIsTemplateDependent(node->right)) {
    if (!EvaluateIntegerExpression(node->right, &index)) {
      SemanticError(node->right,
                    "pack index must be a constant expression");
    } else if (index < 0) {
      SemanticError(node->right, "pack index cannot be negative");
    }
  }

  if (node->left != NULL && node->left->type != NULL) {
    ASTNodeSetType((ASTNode*)node, node->left->type);
    node->base.value_category = node->left->value_category;
  }
}

// Contract: Computes sizeof/alignof/sizeof... constant when the operand type is
// known; VLAs and dependent type operands stay runtime-sized or unset.
// [expr.sizeof]/1, [expr.alignof]/1: a function type has no size or
// alignment.  C accepts `sizeof` of a function as a GNU extension.
static void DiagnoseSizeofFunctionType(SizeofASTNode* node,
                                       TypeRecord* operand_type,
                                       bool is_alignof) {
  if (CompilerIsCXX() && operand_type != NULL && TypeIsFunction(operand_type)) {
    SemanticError((ASTNode*)node, "invalid application of '%s' to a function type",
                  is_alignof ? "alignof" : "sizeof");
  }
}

void AnalyzeSizeofExpression(SizeofASTNode* node) {
  bool is_alignof = node->base.base.op == AST_OP(alignof);
  if (node->is_pack_size) {
    bool valid_pack = (node->expr != NULL &&
                       (node->expr->flags & kASTPackExpansion) != 0);
    if (node->expr != NULL && node->expr->op == AST_OP(identifier)) {
      IdentifierASTNode* id = (IdentifierASTNode*)node->expr;
      valid_pack =
          id->symbol != NULL && id->symbol->flags.is_parameter_pack;
    }
    if (!valid_pack) {
      SemanticError((ASTNode*)node, "sizeof... requires a parameter pack");
    }
    if (node->expr != NULL && node->expr->op == AST_OP(identifier)) {
      Symbol* pack = ((IdentifierASTNode*)node->expr)->symbol;
      if (pack != NULL && pack->structured_binding_pack_size >= 0) {
        node->base.value.ivalue = pack->structured_binding_pack_size;
      }
    }
    ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
    return;
  }
  if (node->expr != NULL) {
    node->expr = AnalyzeExpression(node->expr);
    TypeRecord* operand_type = SizeofOperandType(node->expr->type);
    DiagnoseSizeofFunctionType(node, operand_type, is_alignof);
    if (TypeIsVLA(operand_type)) {
      // sizeof(vla) is calculated at runtime.
    } else {
      node->base.value.ivalue =
          is_alignof ? TypeRecordAlignment(operand_type) : operand_type->size;
    }
  } else if (node->type_operand != NULL &&
             !TypeContainsTemplateParameter(node->type_operand)) {
    // A `sizeof(type-id)` / `alignof(type-id)` whose operand has become
    // concrete (e.g. after template instantiation): re-measure now.
    TypeRecord* operand_type = SizeofOperandType(node->type_operand);
    DiagnoseSizeofFunctionType(node, operand_type, is_alignof);
    TypeRecordCalculateSize(operand_type);
    node->base.value.ivalue =
        is_alignof ? TypeRecordAlignment(operand_type) : operand_type->size;
  }
  ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
}

// Finds the std::<name> class type, or NULL if it is not declared (e.g. the
// relevant standard header has not been included).
static TypeRecord* FindStdClassType(const char* name) {
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }
  String class_name;
  StringInit(&class_name, name);
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &class_name);
  StringDestruct(&class_name);
  if (result.status != kInlineLookupUnique || result.symbol == NULL ||
      result.symbol->type == NULL || !TypeIsStructOrUnion(result.symbol->type)) {
    return NULL;
  }
  return result.symbol->type;
}

// Contract: True when the class layout includes a vptr member (`__vptr`).
bool TypeIsPolymorphicClass(TypeRecord* type) {
  return type != NULL && TypeIsStructOrUnion(type) &&
         type->info.struct_info != NULL &&
         FindStructMemberByName(type->info.struct_info, "__vptr") != NULL;
}

// Lowers a typeid operator into the underlying std::type_info access.  The
// static form yields the address of the type's emitted type_info object; the
// polymorphic form (a glvalue of polymorphic class type) reads the most-derived
// type_info out of the object's vtable header at runtime.  Returns a fully
// analyzed replacement expression.
ASTNode* AnalyzeTypeidExpression(TypeidASTNode* node) {
  SourceLocation location = node->base.location;
  TypeRecord* type_info_type = FindStdClassType("type_info");
  if (type_info_type == NULL) {
    SemanticError((ASTNode*)node,
                  "typeid requires <typeinfo> to be included");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }

  bool polymorphic = false;
  TypeRecord* static_type = node->operand_type;
  ASTNode* operand = node->expr;
  if (operand != NULL) {
    operand = AnalyzeExpression(operand);
    node->expr = operand;
    static_type = operand->type;
    bool glvalue = operand->value_category == kValueCategoryLvalue ||
                   operand->value_category == kValueCategoryXvalue;
    if (glvalue && TypeIsPolymorphicClass(static_type)) {
      polymorphic = true;
    }
  }
  if (static_type != NULL && TypeIsReference(static_type)) {
    static_type = static_type->next;
  }

  TypeRecord* type_info_ptr =
      NewPointerTo(kQualPlain, TypeRecordCopy(type_info_type));
  SetQualifiers(type_info_ptr->next,
                GetQualifiers(type_info_ptr->next) | kQualConst);

  ASTNode* result;
  if (polymorphic) {
    // The Itanium address point is the first virtual function entry, with the
    // type_info pointer exactly one pointer-width before it. Do the byte
    // adjustment explicitly: applying the ordinary subscript lowering to the
    // compiler-invented void** member can inherit the vtable aggregate stride.
    ASTNode* address =
        NewUnaryASTNode(AST_OP(address), NULL, location, operand);
    ASTNode* vptr = NewBinaryASTNode(
        AST_OP(arrow), NULL, location, address,
        NewStringConstantASTNode(NewString("__vptr"), NULL, location));
    TypeRecord* byte_ptr = NewPointerTo(
        kQualPlain, NewTypeRecordWithSize(kTypeChar, kQualPlain));
    ASTNode* vptr_bytes = NewCastASTNode(byte_ptr, location, vptr);
    ASTNode* slot_address = NewBinaryASTNode(
        AST_OP(minus), NULL, location, vptr_bytes,
        NewIntConstantASTNode(SizeofPointer(),
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
    TypeRecord* type_info_ptr_ptr =
        NewPointerTo(kQualPlain, TypeRecordCopy(type_info_ptr));
    ASTNode* slot = NewUnaryASTNode(
        AST_OP(contents), NULL, location,
        NewCastASTNode(type_info_ptr_ptr, location, slot_address));
    result = NewUnaryASTNode(AST_OP(contents), NULL, location, slot);
  } else {
    Symbol* type_info_symbol = RttiGetTypeInfoSymbol(static_type);
    if (type_info_symbol == NULL) {
      SemanticError((ASTNode*)node,
                    "cannot form typeid for this type");
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
      node->base.flags |= kASTAnalyzed;
      return (ASTNode*)node;
    }
    // The RTTI symbol is emitted as raw storage, but expressions observe it as
    // a const type_info lvalue. Keep the storage symbol's byte-array type
    // private to the emitter instead of round-tripping through a synthetic
    // cast whose pointee type can be discarded with the replaced typeid node.
    result = NewIdentifierASTNode(type_info_symbol, location);
    ASTNodeSetType(result, type_info_ptr->next);
    result->value_category = kValueCategoryLvalue;
    result->flags |= kASTAnalyzed;
    TypeRecordDelete(type_info_ptr);
  }
  return AnalyzeExpression(result);
}
