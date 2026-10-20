//
//  expr_semantics_call.c
//  c_compiler
//
//  Function calls, member-call lowering, and class construction.
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

static void RenumberVectorChildren(VectorASTNode* node);
static bool CurrentFunctionIsCXXCtorOrDtor(void);
static bool ReceiverIsThisObject(ASTNode* receiver);
static bool ExpressionHasExactDynamicClassType(ASTNode* expr);
static bool FunctionIsSourceLocationCurrent(TypeRecord* func);
static ASTNode* NewSourceLocationDefaultArgument(Symbol* formal, SourceLocation location);
static bool AppendDefaultCallArguments(VectorASTNode* call, TypeRecord* func);
static ASTNode* CloneReceiverForVirtualLookup(ASTNode* receiver);
static TypeRecord* CopyFunctionTypeForVirtualCall(TypeRecord* function_type);
static ASTNode* NewVirtualCalleeFromReceiver(ASTNode* receiver, StructMember* member, bool receiver_is_pointer, SourceLocation location);
static Symbol* CXXStructTemplateFamilyOrigin(Struct* owner);
static bool CXXStructsShareTemplateFamily(Struct* left, Struct* right);
typedef struct MemberCallLowerPlan MemberCallLowerPlan;
static bool MemberCallLowerValidateAccess(VectorASTNode* node,
                                          BinaryASTNode** member_access_out,
                                          StructMemberASTNode** member_node_out,
                                          StructMember** member_out);
static void MemberCallLowerRetargetMemberSymbol(
    BinaryASTNode* member_access, StructMemberASTNode* member_node,
    StructMember** member_out, Struct** concrete_receiver_out,
    bool* qualified_base_member_out);
static void MemberCallLowerRebindConstructorMember(
    BinaryASTNode* member_access, StructMemberASTNode* member_node,
    StructMember** member_out, Struct* concrete_receiver);
static void MemberCallLowerDiagnoseMemberCall(BinaryASTNode* member_access,
                                                StructMember* member);
static void MemberCallLowerPlanDispatch(
    VectorASTNode* node, BinaryASTNode* member_access,
    StructMemberASTNode* member_node, StructMember* member,
    Struct* concrete_receiver, bool qualified_base_member,
    MemberCallLowerPlan* plan);
static void MemberCallLowerRewriteToFunctionCall(
    VectorASTNode* node, BinaryASTNode* member_access,
    StructMemberASTNode* member_node, StructMember* member,
    MemberCallLowerPlan* plan);
static Struct* FunctionMemberOwner(TypeRecord* current);
static Struct* CXXLambdaEnclosingClassAccessContext(TypeRecord* func);
bool CXXSameAccessClass(Struct* a, Struct* b);
static bool FunctionIsFriendOf(TypeRecord* current, Struct* owner);
static bool CurrentFunctionIsFriendOf(Struct* owner);
static bool EnclosingFunctionIsFriendOf(TypeRecord* current, Struct* owner);
static TypeRecord* NewStructTypeForAdjustment(Struct* str);
static ASTNode* NewVirtualBaseOffsetLoad(ASTNode* receiver, int vbtable_index, SourceLocation location);
static ASTNode* AddStaticOffsetToRuntimeOffset(ASTNode* offset, int byte_offset, SourceLocation location);
static ASTNode* AnalyzeCXXFunctionalClassConstruction(VectorASTNode* node);
static ASTNode* MaterializeCXXByValueClassArgument(ASTNode* actual, TypeRecord* formal_type);
static ASTNode* TryAnalyzeOverloadedCallOperator(VectorASTNode* node);
static bool CallActualsContainTemplateParameter(VectorASTNode* call);
static bool CallActualsContainUndeducedAuto(VectorASTNode* call);
static bool CallActualsContainDependentFunctorCall(VectorASTNode* call);
static bool IsDependentMemberTemplateCall(VectorASTNode* node);
static bool IsMemberTemplateCallWithDependentExplicitArgs(VectorASTNode* node);
static void SetDependentMemberTemplateCallType(VectorASTNode* node);
static ASTNode* TryAnalyzeCXXExplicitDestructorCall(VectorASTNode* node);
static ASTNode* CXXFunctionalConstructionInstantiateCalleeTypes(VectorASTNode* node);
static bool CXXFunctionalConstructionTryTypedefScalarRoute(VectorASTNode* node,
                                                            ASTNode** result);
static bool CXXFunctionalConstructionResolveTargetType(VectorASTNode* node,
                                                         TypeRecord** out_type,
                                                         bool* ctad_failed);
static ASTNode* CXXFunctionalConstructionFinishWithType(VectorASTNode* node,
                                                        TypeRecord* type);
static bool CXXFunctionalConstructionHasUserDeclaredConstructor(
    StructMember* constructor_head);
static ASTNode* CXXFunctionalConstructionSpliceCastAndAnalyze(
    VectorASTNode* node, TypeRecord* type, ASTNode* arg);
static ASTNode* CXXFunctionalConstructionSpliceCompoundLiteralAndAnalyze(
    VectorASTNode* node, TypeRecord* type, ASTNode* initializer);
static ASTNode* CXXFunctionalConstructionTryConversionOperatorCast(
    VectorASTNode* node, TypeRecord* type);
static ASTNode* CXXFunctionalConstructionTryAggregateFunctionalInit(
    VectorASTNode* node, TypeRecord* type,
    bool has_user_declared_constructor);
static ASTNode* CXXFunctionalConstructionTryCastWithoutViableConstructor(
    VectorASTNode* node, TypeRecord* type, StructMember* constructor);
static void CXXFunctionalConstructionRetargetConstructorMemberOwners(
    TypeRecord* type, StructMember* constructor);
static bool CXXFunctionalConstructionHasNonInventedConstructor(
    StructMember* constructor);
static ASTNode* CXXFunctionalConstructionLowerToConstructorComma(
    VectorASTNode* node, TypeRecord* type, StructMember* constructor);
static bool FunctionCallValidateFixedArity(VectorASTNode* node,
                                           TypeRecord* subtype,
                                           size_t* num_actual_args,
                                           bool has_pack_expansion_actual);
static void FunctionCallConvertOneFixedArgument(VectorASTNode* node,
                                                TypeRecord* subtype,
                                                size_t index);
static ASTNode* FunctionCallTryExplicitDestructor(VectorASTNode* node);
static void FunctionCallAnalyzeAndAttachCallee(VectorASTNode* node,
                                               bool* syntactic_class_construction);
static bool FunctionCallDeferDependentQualifiedCallee(VectorASTNode* node);
static bool FunctionCallDeferMemberTemplateExplicitArgs(VectorASTNode* node);
static ASTNode* FunctionCallTryMetaSynthesisEarly(VectorASTNode* node);
static void FunctionCallAnalyzeActualArguments(VectorASTNode* node);
static ASTNode* FunctionCallTryMetaCallsAfterActuals(VectorASTNode* node);
static bool FunctionCallDeferDependentFunctorActuals(VectorASTNode* node);
static ASTNode* FunctionCallTrySyntacticConstruction(VectorASTNode* node,
                                                     bool syntactic_class_construction);
static bool FunctionCallDeferDependentMemberTemplate(VectorASTNode* node,
                                                     bool has_pack_expansion_actual);
static bool FunctionCallDeferUndeducedAutoActuals(VectorASTNode* node);
static ASTNode* FunctionCallTryDeduceIdentifierFunctionTemplate(
    VectorASTNode* node, bool has_pack_expansion_actual);
static bool FunctionCallDeferTypeDependentOverloadedActuals(VectorASTNode* node);
static void FunctionCallResolveAndLowerMemberAccess(VectorASTNode* node,
                                                      bool* lowered_member_call);
static bool FunctionCallDeferDependentMemberReceiver(VectorASTNode* node,
                                                     bool lowered_member_call);
static bool FunctionCallIsConcreteCtadConstruction(VectorASTNode* node,
                                                     bool has_pack_expansion_actual);
static bool FunctionCallHasConcreteOverloadSet(VectorASTNode* node);
static bool FunctionCallTryAssignDependentTemplateCalleeType(VectorASTNode* node,
                                                               bool is_concrete_ctad_construction,
                                                               bool concrete_overload_set);
static ASTNode* FunctionCallResolveCalleeAndTemplates(VectorASTNode* node,
                                                      bool has_pack_expansion_actual);
static ASTNode* FunctionCallValidateCallableAndArguments(VectorASTNode* node);
static void FunctionCallClearStaleDependentFlag(VectorASTNode* node);
static void FunctionCallEnsureTemplateDefinitionsAndEscalation(VectorASTNode* node);
static TypeRecord* FunctionCallAssignReturnTypeFromCallee(VectorASTNode* node);
static bool FunctionCallConvertFixedArguments(VectorASTNode* node,
                                              TypeRecord* subtype,
                                              size_t* num_actual_args,
                                              bool has_pack_expansion_actual);
static void FunctionCallPromoteVarargsActuals(TypeRecord* subtype,
                                              VectorASTNode* node,
                                              size_t num_formal_args,
                                              size_t num_actual_args);
static ASTNode* FunctionCallCheckConstexprImmediate(VectorASTNode* node,
                                                    TypeRecord* subtype,
                                                    TypeRecord* return_type,
                                                    bool call_ok);
static void FunctionCallCheckFormatAndPrintf(VectorASTNode* node, bool call_ok);
static ASTNode* FunctionCallTryLowerContractsOrInline(VectorASTNode* node,
                                                      TypeRecord* subtype,
                                                      TypeRecord* return_type,
                                                      bool call_ok);


// Reassigns parent/child_id on each argument child after inserting or reordering.
static void RenumberVectorChildren(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* child = node->children->value.p[i];
    if (child == NULL) {
      continue;
    }
    child->parent = &node->base;
    child->child_id = (int)i;
  }
}

// Diagnoses uses of deleted or implicitly-deleted functions once per use site.
void CheckDeletedFunctionUse(Symbol* function, ASTNode* use) {
  if (!CompilerIsCXX() || function == NULL || function->type == NULL ||
      use == NULL ||
      !TypeIsFunction(function->type) ||
      (use->flags & kASTDeletedFunctionDiagnosed) != 0) {
    return;
  }
  TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, function);
  TypeRecord* deleted_type = function->type;
  if (!deleted_type->info.function.is_deleted &&
      function->value.func_defn != NULL &&
      function->value.func_defn->type != NULL &&
      TypeIsFunction(function->value.func_defn->type) &&
      function->value.func_defn->type->info.function.is_deleted) {
    deleted_type = function->value.func_defn->type;
  }
  if (!deleted_type->info.function.is_deleted &&
      function->type->info.function.template_origin != NULL) {
    Symbol* origin = function->type->info.function.template_origin;
    if (origin->type != NULL && TypeIsFunction(origin->type) &&
        origin->type->info.function.is_deleted) {
      deleted_type = origin->type;
    } else if (origin->value.func_defn != NULL &&
               origin->value.func_defn->type != NULL &&
               TypeIsFunction(origin->value.func_defn->type) &&
               origin->value.func_defn->type->info.function.is_deleted) {
      deleted_type = origin->value.func_defn->type;
    }
  }
  if (!deleted_type->info.function.is_deleted) {
    return;
  }
  use->flags |= kASTDeletedFunctionDiagnosed;
  String function_name;
  StringInit(&function_name, NULL);
  SymbolFunctionDiagnosticName(function, &function_name);
  if (deleted_type->info.function.is_implicitly_deleted) {
    SemanticError(use, "Use of implicitly deleted function %s",
                  function_name.value);
  } else if (FunctionExtraOf(&deleted_type->info.function)->deleted_reason !=
             NULL) {
    SemanticError(
        use, "Use of deleted function %s: %s", function_name.value,
        FunctionExtraOf(&deleted_type->info.function)->deleted_reason->value);
  } else {
    SemanticError(use, "Use of deleted function %s", function_name.value);
  }
  StringDestruct(&function_name);
}

// True while semantic analysis runs inside a constructor or destructor body.
static bool CurrentFunctionIsCXXCtorOrDtor(void) {
  return compiler->current_function != NULL &&
         TypeIsFunction(compiler->current_function) &&
         (compiler->current_function->info.function.is_constructor ||
          compiler->current_function->info.function.is_destructor);
}

// True if `receiver` designates the object `this` points at, or one of its base
// subobjects.  Accepts `this`, `(*this)`, and the fixed base-subobject
// adjustments that base constructor and destructor calls are synthesized with.
//
// The `this` symbol is matched by name rather than by identity with the current
// function's parameter, because synthesized and template-cloned special member
// bodies do not always share the symbol.  `this` is a keyword, so no user
// variable can collide with it.
static bool ReceiverIsThisObject(ASTNode* receiver) {
  while (receiver != NULL) {
    if (receiver->op == AST_OP(contents)) {
      receiver = ((UnaryASTNode*)receiver)->sub;
      continue;
    }
    // A base subobject call adjusts `this` by the base's byte offset.  Only the
    // compiler synthesizes these; user pointer arithmetic on `this` lacks the
    // flag and names a different array element, not a base.
    if (receiver->op == AST_OP(plus) &&
        (receiver->flags & kASTForcedTypeAdjustment) != 0) {
      receiver = ((BinaryASTNode*)receiver)->left;
      continue;
    }
    break;
  }
  if (receiver == NULL || receiver->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* symbol = ((IdentifierASTNode*)receiver)->symbol;
  return symbol != NULL && StringEqual(&symbol->name, "this");
}

// True when `expr` names a complete object of its static class type, so a
// virtual call on it can be bound to that class's overrider.  Pointers,
// references, and dereferences are excluded: they can designate a more-derived
// object.  A non-reference class data member is included because that slot is
// always a complete object of the member's type.
static bool ExpressionHasExactDynamicClassType(ASTNode* expr) {
  while (expr != NULL && expr->op == AST_OP(comma)) {
    expr = ((BinaryASTNode*)expr)->right;
  }
  if (expr == NULL) {
    return false;
  }
  if (expr->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expr)->symbol;
    return symbol != NULL && symbol->type != NULL &&
           !TypeIsReference(symbol->type) &&
           TypeIsStructOrUnion(symbol->type);
  }
  if (expr->op == AST_OP(compound_literal)) {
    return TypeIsStructOrUnion(expr->type);
  }
  if ((expr->op == AST_OP(dot) || expr->op == AST_OP(arrow))) {
    BinaryASTNode* access = (BinaryASTNode*)expr;
    if (access->right == NULL || access->right->op != AST_OP(structmember)) {
      return false;
    }
    StructMember* field = ((StructMemberASTNode*)access->right)->member;
    return field != NULL && field->symbol != NULL && field->symbol->type != NULL &&
           !field->is_static && !field->is_member_function &&
           !TypeIsReference(field->symbol->type) &&
           TypeIsStructOrUnion(field->symbol->type);
  }
  if (expr->op == AST_OP(subscript)) {
    BinaryASTNode* sub = (BinaryASTNode*)expr;
    TypeRecord* base_type = sub->left != NULL ? sub->left->type : NULL;
    while (base_type != NULL && TypeIsReference(base_type)) {
      base_type = base_type->next;
    }
    return base_type != NULL && TypeIsArray(base_type) &&
           TypeIsStructOrUnion(base_type->next);
  }
  return false;
}

// True for `std::source_location::current` static member signatures.
static bool FunctionIsSourceLocationCurrent(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.symbol == NULL ||
      !StringEqual(&func->info.function.symbol->name, "current")) {
    return false;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  return owner != NULL && owner->tag_name != NULL &&
         StringEqual(owner->tag_name, "source_location") &&
         SymbolIsInStdNamespace(owner->tag_symbol);
}

// Builds a compiler builtin default for a `source_location::current` formal.
static ASTNode* NewSourceLocationDefaultArgument(Symbol* formal,
                                                 SourceLocation location) {
  if (formal == NULL) {
    return NULL;
  }
  ASTOpcode opcode = AST_OP(bad);
  if (StringEqual(&formal->name, "line")) {
    opcode = AST_OP(builtin_source_line);
  } else if (StringEqual(&formal->name, "column")) {
    opcode = AST_OP(builtin_source_column);
  } else if (StringEqual(&formal->name, "file")) {
    opcode = AST_OP(builtin_source_file);
  } else if (StringEqual(&formal->name, "function")) {
    opcode = AST_OP(builtin_source_pretty_function);
  }
  if (opcode == AST_OP(bad)) {
    return NULL;
  }
  return NewVectorASTNode(opcode, NULL, location,
                          NewRawIdentifierASTNode(NULL, location), NewVector());
}

// Appends analyzed default-argument expressions; returns false if any formal lacks a default.
static bool AppendDefaultCallArguments(VectorASTNode* call, TypeRecord* func) {
  if (call == NULL || call->children == NULL || func == NULL ||
      !TypeIsFunction(func)) {
    return true;
  }
  size_t num_actual_args = call->children->length;
  size_t num_formal_args = func->info.function.prototype.length;
  if (num_actual_args >= num_formal_args) {
    return true;
  }
  for (size_t i = num_actual_args; i < num_formal_args; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal == NULL || SymbolExtraOf(formal)->default_argument == NULL) {
      return false;
    }
  }
  for (size_t i = num_actual_args; i < num_formal_args; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    ASTNode* default_arg =
        FunctionIsSourceLocationCurrent(func)
            ? NewSourceLocationDefaultArgument(formal, call->base.location)
            : NULL;
    if (default_arg == NULL) {
      default_arg = ASTNodeClone(SymbolExtraOf(formal)->default_argument,
                                 IdentityCloneNode, NULL, NULL);
    }
    default_arg->flags |= kASTDefaultArgument;
    default_arg->location = call->base.location;
    default_arg = AnalyzeExpression(default_arg);
    default_arg->parent = (ASTNode*)call;
    // A stored child_id on a call argument is its index in `children`, not the
    // number visitors pass down (those reserve 0 for the callee in `left`).
    default_arg->child_id = (int)i;
    VectorAppend(call->children, default_arg);
  }
  return true;
}

// Shallow-clones a receiver for vptr lookup without mutating the original lvalue.
static ASTNode* CloneReceiverForVirtualLookup(ASTNode* receiver) {
  if (receiver->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)receiver;
    ASTNode* clone = NewIdentifierASTNode(id->symbol, receiver->location);
    ASTNodeSetType(clone, receiver->type);
    clone->flags |= receiver->flags & kASTNeedAddress;
    return clone;
  }
  return ASTNodeClone(receiver, IdentityCloneNode, NULL, NULL);
}

// Copies a virtual member's function type and formal symbols for vtable slot typing.
static TypeRecord* CopyFunctionTypeForVirtualCall(TypeRecord* function_type) {
  TypeRecord* copy = TypeRecordCopy(function_type);
  VectorInit(&copy->info.function.prototype);
  for (size_t i = 0; i < function_type->info.function.prototype.length; i++) {
    Symbol* formal = function_type->info.function.prototype.value.p[i];
    VectorAppend(&copy->info.function.prototype, SymbolClone(formal));
  }
  return copy;
}

// Lowers `&` on an already-analyzed operand; folds `&*p` to `p`. Mutates/substitutes `sub`.
ASTNode* NewAnalyzedBuiltinAddressOf(ASTNode* sub,
                                            SourceLocation location) {
  if (sub != NULL && sub->op == AST_OP(contents)) {
    ASTNode* pointer = ASTNodeMove(((UnaryASTNode*)sub)->sub);
    ASTNodeDelete(sub);
    return pointer;
  }
  TypeRecord* pointer_type = NewPointerTo(kQualPlain, sub->type);
  ASTNode* address =
      NewUnaryASTNode(AST_OP(address), pointer_type, location, sub);
  ASTNodeSetType(address, pointer_type);
  sub->flags |= kASTNeedAddress;
  address->flags |= kASTAnalyzed;
  return address;
}

// Builds the vtable slot load used as the callee for a virtual call: takes the
// address of a class lvalue when needed, reads `__vptr[index]`, and types the
// slot as a pointer to a copy of `function_type` suitable for indirect call.
// Returns NULL when the member is not virtual or inputs are invalid.
ASTNode* NewVirtualCalleeFromFunction(ASTNode* receiver,
                                            TypeRecord* function_type,
                                            bool receiver_is_pointer,
                                            SourceLocation location) {
  if (receiver == NULL || function_type == NULL ||
      !TypeIsFunction(function_type) ||
      function_type->info.function.virtual_index < 0) {
    return NULL;
  }
  ASTNode* receiver_clone = CloneReceiverForVirtualLookup(receiver);
  if (!receiver_is_pointer) {
    receiver_clone = NewAnalyzedBuiltinAddressOf(receiver_clone, location);
  }
  ASTNode* vptr_name =
      NewStringConstantASTNode(NewString("__vptr"), NULL, location);
  ASTNode* vptr =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver_clone,
                       vptr_name);
  ASTNode* index =
      NewIntConstantASTNode(function_type->info.function.virtual_index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* slot =
      NewBinaryASTNode(AST_OP(subscript), NULL, location, vptr, index);
  TypeRecord* callable_type =
      CopyFunctionTypeForVirtualCall(function_type);
  TypeRecord* function_pointer = NewPointerTo(kQualPlain, callable_type);
  slot = AnalyzeExpression(slot);
  ASTNodeSetType(slot, function_pointer);
  return slot;
}

// Contract: NULL unless `member` carries a virtual function type; otherwise
// delegates to NewVirtualCalleeFromFunction. Caller owns the returned callee tree;
// `receiver_is_pointer` must match how `receiver` was obtained (`.` vs `->`).
static ASTNode* NewVirtualCalleeFromReceiver(ASTNode* receiver,
                                            StructMember* member,
                                            bool receiver_is_pointer,
                                            SourceLocation location) {
  return member != NULL && member->symbol != NULL
             ? NewVirtualCalleeFromFunction(receiver, member->symbol->type,
                                            receiver_is_pointer, location)
             : NULL;
}

// Primary template symbol shared by a class template specialization family.
static Symbol* CXXStructTemplateFamilyOrigin(Struct* owner) {
  if (owner == NULL || owner->tag_symbol == NULL) {
    return NULL;
  }
  TypeRecord* tag_type = owner->tag_symbol->type;
  return tag_type != NULL && tag_type->template_origin != NULL
             ? tag_type->template_origin
             : owner->tag_symbol;
}

// True when two class types are specializations of the same class template.
static bool CXXStructsShareTemplateFamily(Struct* left, Struct* right) {
  if (left == right) {
    return left != NULL;
  }
  Symbol* left_origin = CXXStructTemplateFamilyOrigin(left);
  Symbol* right_origin = CXXStructTemplateFamilyOrigin(right);
  return left_origin != NULL && left_origin == right_origin;
}

struct MemberCallLowerPlan {
  bool use_virtual_dispatch;
  bool polymorphic_special_member;
  ASTNode* receiver;
  ASTNode* discarded_static_receiver;
};

// Returns false when `node->left` is not a `.`/`->` member access to a callable
// member. On success outputs the access nodes and initial `member` symbol.
static bool MemberCallLowerValidateAccess(VectorASTNode* node,
                                          BinaryASTNode** member_access_out,
                                          StructMemberASTNode** member_node_out,
                                          StructMember** member_out) {
  if (node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return false;
  }

  BinaryASTNode* member_access = (BinaryASTNode*)node->left;
  if (member_access->right == NULL ||
      member_access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node =
      (StructMemberASTNode*)member_access->right;
  StructMember* member = member_node->member;
  if (!member->is_member_function &&
      (member->symbol == NULL || member->symbol->type == NULL ||
       !TypeIsFunction(member->symbol->type))) {
    return false;
  }
  *member_access_out = member_access;
  *member_node_out = member_node;
  *member_out = member;
  return true;
}

// Retargets the selected overload head to the concrete receiver or qualified
// base member before overload resolution runs. Mutates `member_node` and
// *member_out; sets *concrete_receiver_out and *qualified_base_member_out.
static void MemberCallLowerRetargetMemberSymbol(
    BinaryASTNode* member_access, StructMemberASTNode* member_node,
    StructMember** member_out, Struct** concrete_receiver_out,
    bool* qualified_base_member_out) {
  StructMember* member = *member_out;
  TypeRecord* concrete_receiver_type = member_access->left->type;
  if (member_access->base.op == AST_OP(arrow) &&
      TypeIsStructOrUnionPointer(concrete_receiver_type)) {
    concrete_receiver_type = concrete_receiver_type->next;
  }
  Struct* concrete_receiver =
      concrete_receiver_type != NULL &&
              TypeIsStructOrUnion(concrete_receiver_type)
          ? concrete_receiver_type->info.struct_info
          : NULL;
  Struct* member_owner =
      member->symbol != NULL && member->symbol->type != NULL &&
              TypeIsFunction(member->symbol->type)
          ? member->symbol->type->info.function.cxx_member_owner
          : NULL;
  TypeRecord* qualified_owner_type = member_node->owner_type;
  bool qualified_base_member =
      member_access->right != NULL &&
      (member_access->right->flags & kASTQualifiedName) != 0;
  if (qualified_base_member && qualified_owner_type != NULL &&
      TypeIsStructOrUnion(qualified_owner_type) &&
      qualified_owner_type->info.struct_info != NULL &&
      member->symbol != NULL) {
    StructMember* base_member = FindStructMemberWithAccessAndOffsetByName(
        qualified_owner_type->info.struct_info, member->symbol->name.value,
        NULL, NULL, NULL);
    if (base_member != NULL && base_member->symbol != NULL) {
      member = base_member;
      StructMemberASTNodeSetMember(member_node, base_member);
      member_owner =
          base_member->symbol->type != NULL &&
                  TypeIsFunction(base_member->symbol->type)
              ? base_member->symbol->type->info.function.cxx_member_owner
              : member_owner;
    }
  }
  // A member template specialization (`(s.*&S::Get<1>)()`) is already the
  // selected function; the name would find the template again.
  bool template_specialization =
      member->symbol != NULL && member->symbol->type != NULL &&
      TypeIsFunction(member->symbol->type) &&
      !member->symbol->flags.is_template &&
      member->symbol->type->info.function.template_origin != NULL &&
      member_owner != NULL && concrete_receiver != NULL &&
      (concrete_receiver == member_owner ||
       StructIsDerivedFrom(concrete_receiver, member_owner,
                           /*public_only=*/false));
  if (!qualified_base_member && !template_specialization &&
      concrete_receiver != NULL && member->symbol != NULL) {
    StructMember* concrete_member =
        FindStructMember(concrete_receiver, &member->symbol->name);
    if (concrete_member != NULL && concrete_member->symbol != NULL &&
        concrete_member->symbol->type != NULL &&
        TypeIsFunction(concrete_member->symbol->type) &&
        (concrete_receiver != member_owner ||
         concrete_member->symbol != member->symbol)) {
      member = concrete_member;
      StructMemberASTNodeSetMember(member_node, concrete_member);
    }
  }
  *member_out = member;
  *concrete_receiver_out = concrete_receiver;
  *qualified_base_member_out = qualified_base_member;
}

// After overload resolution, rebinds constructor members to the receiver's
// concrete owner when template families match. Mutates `member`, `member_node`,
// and constructor symbols' cxx_member_owner / asm names.
static void MemberCallLowerRebindConstructorMember(
    BinaryASTNode* member_access, StructMemberASTNode* member_node,
    StructMember** member_out, Struct* concrete_receiver) {
  StructMember* member = *member_out;
  if (member == NULL || member->symbol == NULL || member->symbol->type == NULL ||
      !TypeIsFunction(member->symbol->type) ||
      !member->symbol->type->info.function.is_constructor ||
      member_access->left == NULL) {
    return;
  }
  Struct* concrete_owner = NULL;
  if (TypeIsStructOrUnion(member_access->left->type)) {
    concrete_owner = member_access->left->type->info.struct_info;
  } else if (TypeIsStructOrUnionPointer(member_access->left->type)) {
    concrete_owner = member_access->left->type->next->info.struct_info;
  }
  Struct* selected_owner =
      member->symbol->type->info.function.cxx_member_owner;
  bool same_constructor_owner =
      CXXStructsShareTemplateFamily(concrete_owner, selected_owner);
  if (same_constructor_owner &&
      (compiler->current_class_access_context != NULL ||
       (compiler->current_function != NULL &&
        TypeIsFunction(compiler->current_function) &&
        compiler->current_function->info.function.cxx_member_owner != NULL))) {
    Struct* current_owner = compiler->current_class_access_context != NULL
                                ? compiler->current_class_access_context
                                : compiler->current_function->info.function
                                      .cxx_member_owner;
    if (concrete_owner->lexical_parent != NULL &&
        CXXStructsShareTemplateFamily(concrete_owner->lexical_parent,
                                      current_owner)) {
      StructMember* canonical =
          FindStructMember(current_owner, concrete_owner->tag_name);
      if (canonical != NULL && canonical->symbol != NULL &&
          StorageIs(canonical->symbol->storage, STO(typedef)) &&
          TypeIsStructOrUnion(canonical->symbol->type)) {
        concrete_owner = canonical->symbol->type->info.struct_info;
      }
    }
  }
  bool selected_member_template_specialization =
      member->symbol->type->info.function.template_origin != NULL;
  if (same_constructor_owner && concrete_owner != NULL &&
      selected_owner != concrete_owner &&
      !selected_member_template_specialization &&
      concrete_owner->tag_name != NULL) {
    StructMember* concrete_head =
        FindCXXMemberOverloadHead(concrete_owner, concrete_owner->tag_name);
    size_t selected_arity =
        member->symbol->type->info.function.prototype.length;
    StructMember* arity_fallback = NULL;
    for (StructMember* candidate = concrete_head; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->is_member_function && candidate->symbol != NULL &&
          candidate->symbol->type != NULL &&
          TypeIsFunction(candidate->symbol->type) &&
          candidate->symbol->type->info.function.is_constructor &&
          candidate->symbol->type->info.function.prototype.length ==
              selected_arity) {
        if (arity_fallback == NULL) {
          arity_fallback = candidate;
        }
        bool same_formals = true;
        for (size_t i = 1; i < selected_arity; i++) {
          Symbol* selected_formal =
              member->symbol->type->info.function.prototype.value.p[i];
          Symbol* candidate_formal =
              candidate->symbol->type->info.function.prototype.value.p[i];
          if (selected_formal == NULL || candidate_formal == NULL ||
              !TypeEqualIgnoringQualifiers(selected_formal->type,
                                           candidate_formal->type)) {
            same_formals = false;
            break;
          }
        }
        if (same_formals) {
          member = candidate;
          StructMemberASTNodeSetMember(member_node, candidate);
          break;
        }
      }
    }
    if (member_node->member != arity_fallback && arity_fallback != NULL &&
        member->symbol->type->info.function.cxx_member_owner !=
            concrete_owner &&
        member->symbol->type->info.function.cxx_special_member_kind ==
            kCXXSpecialMemberNone) {
      member = arity_fallback;
      StructMemberASTNodeSetMember(member_node, arity_fallback);
    }
  }
  if (same_constructor_owner && concrete_owner != NULL) {
    member->symbol->type->info.function.cxx_member_owner = concrete_owner;
    SymbolClearAsmName(member->symbol);
    SymbolSetCXXMangledAsmName(member->symbol);
  }
  *member_out = member;
}

// Access, cv-qualification, and ref-qualifier diagnostics for a resolved member.
// Does not alter the call shape; errors are reported on `member_access`.
static bool MemberChainHasUsingDeclaration(StructMember* head) {
  for (StructMember* m = head; m != NULL; m = m->overload_next) {
    if (m->is_using_declaration) {
      return true;
    }
  }
  return false;
}

static void MemberCallLowerDiagnoseMemberCall(
                                              BinaryASTNode* member_access,
                                              StructMember* member) {
  Struct* owner = member->symbol->type->info.function.cxx_member_owner;
  CXXAccess access = member->access;
  Struct* lookup_context = NULL;
  if (member_access->base.op == AST_OP(arrow) &&
      TypeIsStructOrUnionPointer(member_access->left->type)) {
    lookup_context = member_access->left->type->next->info.struct_info;
  } else if (TypeIsStructOrUnion(member_access->left->type)) {
    lookup_context = member_access->left->type->info.struct_info;
  }
  // A member named through a using-declaration is a member of the class
  // containing that declaration for access checking ([namespace.udecl]).
  if (lookup_context != NULL && member->symbol->name.value != NULL) {
    CXXAccess found_access = kAccessPublic;
    Struct* found_owner = NULL;
    int found_offset = 0;
    StructMember* found = FindStructMemberWithAccessAndOffsetByName(
        lookup_context, member->symbol->name.value, &found_access,
        &found_owner, &found_offset);
    if (found != NULL && found_owner != NULL && found_owner != owner &&
        MemberChainHasUsingDeclaration(found)) {
      owner = found_owner;
    }
  }
  if (!CurrentFunctionCanAccessMember(lookup_context, owner, member->access,
                                      access)) {
    const char* owner_name =
        owner != NULL && owner->tag_name != NULL
            ? owner->tag_name->value
            : "<anonymous>";
    SemanticError((ASTNode*)member_access, "%s is a %s member of %s",
                  member->symbol->name.value, CXXAccessName(access),
                  owner_name);
  }

  bool explicit_object =
      FunctionHasExplicitObjectParameter(member->symbol->type);
  if (!member->is_static) {
    if (!explicit_object &&
        !member->symbol->type->info.function.is_const_member &&
        !member->symbol->type->info.function.is_constructor &&
        !member->symbol->type->info.function.is_destructor &&
        MemberReceiverIsConst(member_access)) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticSuffix(member->symbol, &function_name);
      SemanticError((ASTNode*)member_access,
                    "Cannot call non-const member function %s on const object%s",
                    member->symbol->name.value, function_name.value);
      StringDestruct(&function_name);
    }
    if (!explicit_object &&
        !member->symbol->type->info.function.is_volatile_member &&
        !member->symbol->type->info.function.is_constructor &&
        !member->symbol->type->info.function.is_destructor &&
        MemberReceiverIsVolatile(member_access)) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticSuffix(member->symbol, &function_name);
      SemanticError(
          (ASTNode*)member_access,
          "Cannot call non-volatile member function %s on volatile object%s",
          member->symbol->name.value, function_name.value);
      StringDestruct(&function_name);
    }
    if (!explicit_object &&
        !MemberReceiverMatchesRefQualifier(member->symbol->type, member_access)) {
      SemanticError((ASTNode*)member_access,
                    "Cannot call ref-qualified member function %s on this "
                    "object value category",
                    member->symbol->name.value);
    }
  }
}

// A member found in a base class still needs `this` converted to that base,
// which may be private (`using Base::f;`): member access was already checked,
// so convert as a cast does.  A base at offset 0 is retyped in place, as any
// offset-0 derived-to-base conversion is, so constant evaluation keeps binding
// `this` to the complete object that a CRTP `static_cast<D&>(*this)` returns
// to.
static ASTNode* ConvertReceiverToMemberBase(ASTNode* receiver,
                                            StructMember* member) {
  if (member->symbol->type->info.function.prototype.length == 0 ||
      receiver->type == NULL || !TypeIsStructOrUnionPointer(receiver->type)) {
    return receiver;
  }
  Symbol* this_arg = member->symbol->type->info.function.prototype.value.p[0];
  if (this_arg == NULL || this_arg->type == NULL ||
      !TypeIsStructOrUnionPointer(this_arg->type)) {
    return receiver;
  }
  Struct* from = receiver->type->next->info.struct_info;
  Struct* base = this_arg->type->next->info.struct_info;
  if (from == NULL || base == NULL || from == base ||
      !StructIsDerivedFrom(from, base, /*public_only=*/false)) {
    return receiver;
  }
  TypeRecord* base_type = NewStructTypeForAdjustment(base);
  SetQualifiers(base_type, GetQualifiers(receiver->type->next));
  int base_offset = -1;
  if (TypeBaseOffset(receiver->type->next, base_type, /*public_only=*/false,
                     &base_offset) &&
      base_offset == 0) {
    ASTNodeSetType(receiver, NewPointerTo(kQualPlain, base_type));
    return receiver;
  }
  ASTNode* cast = NewCastASTNode(NewPointerTo(kQualPlain, base_type),
                                 receiver->location, receiver);
  return AnalyzeExpression(cast);
}

// Computes virtual vs static dispatch and prepares the implicit `this` argument
// (or static-receiver side-effect preservation). May set caller_contract_function
// on `node` for C++29 contract assertions on virtual calls.
static void MemberCallLowerPlanDispatch(
    VectorASTNode* node, BinaryASTNode* member_access,
    StructMemberASTNode* member_node, StructMember* member,
    Struct* concrete_receiver, bool qualified_base_member,
    MemberCallLowerPlan* plan) {
  plan->receiver = NULL;
  plan->discarded_static_receiver = NULL;
  plan->use_virtual_dispatch = false;
  plan->polymorphic_special_member = false;

  bool explicit_object =
      FunctionHasExplicitObjectParameter(member->symbol->type);
  bool bound_by_ctor_or_dtor =
      CurrentFunctionIsCXXCtorOrDtor() &&
      ReceiverIsThisObject(member_access->left);
  bool bound_by_final =
      member->symbol->type->info.function.is_final ||
      (concrete_receiver != NULL && concrete_receiver->is_final);
  bool bound_by_exact_type =
      ExpressionHasExactDynamicClassType(member_access->left);
  plan->use_virtual_dispatch =
      member->symbol->type->info.function.is_virtual &&
      !member->is_static && !explicit_object &&
      !qualified_base_member &&
      !bound_by_ctor_or_dtor &&
      !bound_by_final &&
      !bound_by_exact_type;
  if (plan->use_virtual_dispatch &&
      CompilerCXXAtLeast(kLanguageStandardCXX29) &&
      compiler->contract_semantic != kContractSemanticIgnore &&
      FunctionExtraOf(&member->symbol->type->info.function)
              ->contract_assertions.length != 0) {
    TypeRecordDelete(node->caller_contract_function);
    node->caller_contract_function =
        TypeRecordCopy(member->symbol->type);
  }
  Struct* owner = member->symbol->type->info.function.cxx_member_owner;
  plan->polymorphic_special_member =
      (member->symbol->type->info.function.is_constructor ||
       member->symbol->type->info.function.is_destructor) &&
      owner != NULL && owner->virtual_members.length > 0;
  if (!member->is_static) {
    plan->receiver = ASTNodeMove(member_access->left);
    if (explicit_object && member_access->base.op == AST_OP(arrow)) {
      TypeRecord* object_type =
          plan->receiver->type != NULL ? plan->receiver->type->next : NULL;
      plan->receiver = NewUnaryASTNode(AST_OP(contents), object_type,
                                 plan->receiver->location, plan->receiver);
      plan->receiver = AnalyzeExpression(plan->receiver);
    } else if (!explicit_object && member_access->base.op == AST_OP(dot)) {
      plan->receiver = NewAnalyzedBuiltinAddressOf(plan->receiver,
                                                   plan->receiver->location);
    }
    int this_adjustment = explicit_object ? 0 : member_node->byte_offset;
    if (plan->use_virtual_dispatch && member->cxx_vcall_offset != 0) {
      this_adjustment = member->cxx_vcall_offset;
    }
    if (!plan->polymorphic_special_member && this_adjustment != 0) {
      TypeRecord* receiver_type = plan->receiver->type;
      if (member->symbol->type->info.function.prototype.length > 0) {
        Symbol* this_arg =
            member->symbol->type->info.function.prototype.value.p[0];
        receiver_type = this_arg->type;
      }
      ASTNode* offset_node = NewIntConstantASTNode(
          this_adjustment,
          NewTypeRecordWithSize(kTypeInt, kQualPlain),
          plan->receiver->location);
      plan->receiver = NewBinaryASTNode(AST_OP(plus), receiver_type,
                                  plan->receiver->location, plan->receiver,
                                  offset_node);
      ASTNodeSetType(plan->receiver, receiver_type);
      plan->receiver->flags |= kASTAnalyzed;
    } else if (!explicit_object && !plan->polymorphic_special_member) {
      plan->receiver = ConvertReceiverToMemberBase(plan->receiver, member);
    }
  } else {
    plan->discarded_static_receiver = ASTNodeMove(member_access->left);
  }
}

// Replaces the member access callee with an identifier or vtable slot, inserts
// `this`, and handles static-member comma preservation. Deletes the old
// member-access subtree.
static void MemberCallLowerRewriteToFunctionCall(
    VectorASTNode* node, BinaryASTNode* member_access,
    StructMemberASTNode* member_node, StructMember* member,
    MemberCallLowerPlan* plan) {
  ASTNode* old_left = node->left;
  ASTNode* virtual_callee = NULL;
  if (plan->use_virtual_dispatch) {
    virtual_callee =
        NewVirtualCalleeFromReceiver(plan->receiver, member,
                                     true,
                                     old_left->location);
  }
  node->left = virtual_callee != NULL
                   ? virtual_callee
                   : NewIdentifierASTNode(member->symbol, old_left->location);
  if (node->left->op == AST_OP(identifier) &&
      member_node->template_arguments != NULL) {
    ((IdentifierASTNode*)node->left)->template_arguments =
        TemplateArgumentVectorCopy(member_node->template_arguments);
  }
  node->left->parent = &node->base;
  node->left->child_id = 0;
  if (virtual_callee == NULL) {
    ASTNode* analyzed_left = AnalyzeExpression(node->left);
    if (analyzed_left != node->left && analyzed_left != NULL &&
        analyzed_left->op == AST_OP(identifier) &&
        ((IdentifierASTNode*)analyzed_left)->template_arguments == NULL &&
        node->left->op == AST_OP(identifier)) {
      ((IdentifierASTNode*)analyzed_left)->template_arguments =
          ((IdentifierASTNode*)node->left)->template_arguments;
      ((IdentifierASTNode*)node->left)->template_arguments = NULL;
    }
    node->left = analyzed_left;
  }
  ASTNodeDelete(old_left);

  if (plan->receiver != NULL) {
    if (node->children->length == 0) {
      VectorAppend(node->children, plan->receiver);
    } else {
      VectorInsertBefore(node->children, 0, plan->receiver);
    }
    RenumberVectorChildren(node);
  } else if (plan->discarded_static_receiver != NULL) {
    if (node->children->length > 0) {
      ASTNode* first_actual = ASTNodeMove(node->children->value.p[0]);
      ASTNode* comma = NewBinaryASTNode(
          AST_OP(comma), NULL, plan->discarded_static_receiver->location,
          plan->discarded_static_receiver, first_actual);
      comma = AnalyzeExpression(comma);
      VectorSet(node->children, 0, comma);
      comma->parent = &node->base;
      comma->child_id = 0;
    } else {
      ASTNode* callee = node->left;
      node->left = NULL;
      callee->parent = NULL;
      ASTNode* comma = NewBinaryASTNode(
          AST_OP(comma), NULL, plan->discarded_static_receiver->location,
          plan->discarded_static_receiver, callee);
      node->left = AnalyzeExpression(comma);
      node->left->parent = &node->base;
      node->left->child_id = 0;
    }
  }
}

// Lowers `obj.member(args)` / `obj->member(args)` to a plain call with an
// explicit `this` argument (or vtable dispatch). Mutates `node` in place; returns
// false when the callee is not a member-function access shape.
bool LowerMemberFunctionCall(VectorASTNode* node) {
  BinaryASTNode* member_access = NULL;
  StructMemberASTNode* member_node = NULL;
  StructMember* member = NULL;
  if (!MemberCallLowerValidateAccess(node, &member_access, &member_node,
                                     &member)) {
    return false;
  }

  Struct* concrete_receiver = NULL;
  bool qualified_base_member = false;
  MemberCallLowerRetargetMemberSymbol(member_access, member_node, &member,
                                      &concrete_receiver,
                                      &qualified_base_member);

  member = ResolveMemberFunctionOverload(member, node, member_access);
  if (member != NULL) {
    StructMemberASTNodeSetMember(member_node, member);
  }
  if (member != NULL) {
    MemberCallLowerRebindConstructorMember(member_access, member_node, &member,
                                           concrete_receiver);
    CheckDeletedFunctionUse(member->symbol, (ASTNode*)node);
  }
  if (member == NULL) {
    return false;
  }

  MemberCallLowerDiagnoseMemberCall(member_access, member);

  MemberCallLowerPlan plan;
  MemberCallLowerPlanDispatch(node, member_access, member_node, member,
                              concrete_receiver, qualified_base_member, &plan);
  MemberCallLowerRewriteToFunctionCall(node, member_access, member_node, member,
                                       &plan);
  return true;
}


// Returns the spelling of a C++ access specifier for diagnostics.
const char* CXXAccessName(CXXAccess access) {
  switch (access) {
    case kAccessPublic:
      return "public";
    case kAccessProtected:
      return "protected";
    case kAccessPrivate:
      return "private";
  }
  return "unknown";
}

// Recover the class owner from a member function type or its `this` parameter.
static Struct* FunctionMemberOwner(TypeRecord* current) {
  if (current == NULL || !TypeIsFunction(current)) {
    return NULL;
  }
  if (current->info.function.cxx_member_owner != NULL) {
    return current->info.function.cxx_member_owner;
  }
  if (current->info.function.prototype.length == 0) {
    return NULL;
  }
  Symbol* this_sym = current->info.function.prototype.value.p[0];
  if (this_sym == NULL || this_sym->type == NULL ||
      !StringEqual(&this_sym->name, "this") || this_sym->type->next == NULL ||
      !TypeIsStructOrUnion(this_sym->type->next)) {
    return NULL;
  }
  return this_sym->type->next->info.struct_info;
}

// A lambda closure's operator() is a member of the closure class, but the
// lambda body has the access of the function that contains it ([class.local]).
static Struct* CXXLambdaEnclosingClassAccessContext(TypeRecord* func) {
  Struct* owner = FunctionMemberOwner(func);
  if (owner == NULL || owner->tag_symbol == NULL ||
      !owner->tag_symbol->flags.invented) {
    return owner;
  }
  Struct* context = owner;
  if (owner->access_enclosing_function != NULL &&
      owner->access_enclosing_function->type != NULL &&
      TypeIsFunction(owner->access_enclosing_function->type)) {
    Struct* enclosing =
        owner->access_enclosing_function->type->info.function.cxx_member_owner;
    if (enclosing != NULL) {
      context = enclosing;
    }
  }
  if (context == owner && owner->lexical_parent != NULL) {
    // [class.access.nest]: a lambda is a nested class of its enclosing class.
    context = owner->lexical_parent;
  }
  return context;
}

// Contract: Read-only; yields the primary template symbol for a class or
// specialization (tag `template_origin`, else the template pattern symbol).
// NULL for ordinary non-template classes; do not free the result.
Symbol* CXXStructTemplateOrigin(Struct* str) {
  if (str == NULL || str->tag_symbol == NULL) {
    return NULL;
  }
  if (str->tag_symbol->type != NULL &&
      str->tag_symbol->type->template_origin != NULL) {
    return str->tag_symbol->type->template_origin;
  }
  if (str->is_template || str->tag_symbol->flags.is_template) {
    return str->tag_symbol;
  }
  return NULL;
}

// True when two classes share access (same type, nested, or template family).
bool CXXSameAccessClass(Struct* a, Struct* b) {
  if (a == b) {
    return true;
  }
  Symbol* a_origin = CXXStructTemplateOrigin(a);
  Symbol* b_origin = CXXStructTemplateOrigin(b);
  if (a_origin != NULL && b_origin != NULL) {
    if (a_origin == b_origin ||
        StringEqualString(&a_origin->name, &b_origin->name)) {
      return true;
    }
  }
  if (a == NULL || b == NULL || a->tag_name == NULL || b->tag_name == NULL) {
    return false;
  }
  const char* a_name = a->tag_name->value;
  const char* b_name = b->tag_name->value;
  size_t a_len = strcspn(a_name, "<");
  size_t b_len = strcspn(b_name, "<");
  return a_len == b_len && strncmp(a_name, b_name, a_len) == 0;
}

// Returns true when the function currently being analyzed has been granted
// friendship by class `owner` (via a 'friend class' or 'friend function'
// declaration), and may therefore access its private and protected members.
static bool FunctionIsFriendOf(TypeRecord* current, Struct* owner) {
  if (owner == NULL) {
    return false;
  }
  if (current == NULL || !TypeIsFunction(current)) {
    return false;
  }
  // 'friend class C;': any member function of C is a friend.  A nested
  // class is a member of its enclosing class and therefore has that class's
  // access, including friendships ([class.access.nest]).  `Cord::ChunkIterator`
  // may call `InlineRep::inline_size` because `Cord` is a friend of `InlineRep`.
  Struct* current_owner = FunctionMemberOwner(current);
  for (Struct* accessor = current_owner; accessor != NULL;
       accessor = accessor->lexical_parent) {
    for (size_t i = 0; i < owner->friend_classes.length; i++) {
      Struct* friend_class = owner->friend_classes.value.p[i];
      if (friend_class == accessor ||
          CXXSameAccessClass(friend_class, accessor)) {
        return true;
      }
    }
  }
  // 'friend <function>;': match the befriended declaration either by symbol
  // identity or, since a friend declaration and the later definition may be
  // separate symbols, by name and signature.
  Symbol* current_symbol = current->info.function.symbol;
  Symbol* current_template = current->info.function.template_origin;
  for (size_t i = 0; i < owner->friend_functions.length; i++) {
    Symbol* friend_symbol = owner->friend_functions.value.p[i];
    if (friend_symbol == NULL) {
      continue;
    }
    if (friend_symbol == current_symbol) {
      return true;
    }
    if (current_template != NULL &&
        (friend_symbol == current_template ||
         (friend_symbol->type != NULL &&
          TypeIsFunction(friend_symbol->type) &&
          friend_symbol->type->info.function.template_origin ==
              current_template))) {
      return true;
    }
    if (current_template != NULL && friend_symbol->type != NULL &&
        current_template->type != NULL &&
        StringEqualString(&friend_symbol->name, &current_template->name) &&
        TypeEqual(friend_symbol->type, current_template->type)) {
      return true;
    }
    if (current_symbol != NULL && SymbolHasAsmName(friend_symbol) &&
        SymbolHasAsmName(current_symbol) &&
        StringEqualString(SymbolAsmName(friend_symbol),
                          SymbolAsmName(current_symbol))) {
      return true;
    }
    if (current_symbol != NULL &&
        StringEqualString(&friend_symbol->name, &current_symbol->name) &&
        TypeEqual(friend_symbol->type, current_symbol->type)) {
      return true;
    }
  }
  return false;
}

// Friend check for the function currently being analyzed.
static bool CurrentFunctionIsFriendOf(Struct* owner) {
  return FunctionIsFriendOf(compiler->current_function, owner);
}

// A local class, including a lambda closure, has the same access as the
// function it was defined in ([class.local]).  Nested lambdas walk out to
// that function, so a lambda in a friend can use the friend's private access.
static bool EnclosingFunctionIsFriendOf(TypeRecord* current, Struct* owner) {
  for (int depth = 0; depth < 32; depth++) {
    Struct* member_owner = FunctionMemberOwner(current);
    Symbol* enclosing = member_owner != NULL
                            ? member_owner->access_enclosing_function
                            : NULL;
    if (enclosing == NULL || enclosing->type == NULL ||
        !TypeIsFunction(enclosing->type) || enclosing->type == current) {
      return false;
    }
    if (FunctionIsFriendOf(enclosing->type, owner)) {
      return true;
    }
    current = enclosing->type;
  }
  return false;
}

// A named local class has the same member-access context as the function in
// which it is declared, just like a lambda closure. Walk nested local classes
// and accept an enclosing member function (or one of its nested classes).
static bool EnclosingFunctionHasClassAccess(TypeRecord* current,
                                            Struct* owner) {
  for (int depth = 0; depth < 32; depth++) {
    Struct* member_owner = FunctionMemberOwner(current);
    Symbol* enclosing = member_owner != NULL
                            ? member_owner->access_enclosing_function
                            : NULL;
    if (enclosing == NULL || enclosing->type == NULL ||
        !TypeIsFunction(enclosing->type) || enclosing->type == current) {
      return false;
    }
    Struct* enclosing_owner = FunctionMemberOwner(enclosing->type);
    for (Struct* accessor = enclosing_owner; accessor != NULL;
         accessor = accessor->lexical_parent) {
      if (CXXSameAccessClass(accessor, owner)) {
        return true;
      }
    }
    current = enclosing->type;
  }
  return false;
}

// Implements [class.access] for the current semantic context: resolves the
// effective accessor (member function owner, lambda enclosing class, or
// current_class_access_context), then applies friendship, nesting, protected
// derivation, and lambda-capture privacy before comparing against
// `original_access` / `effective_access`.
bool CurrentFunctionCanAccessMember(Struct* lookup_context,
                                           Struct* owner,
                                           CXXAccess original_access,
                                           CXXAccess effective_access) {
  if (effective_access == kAccessPublic) {
    return true;
  }
  TypeRecord* func = compiler->current_function;
  if (func != NULL && TypeIsFunction(func) && owner != NULL) {
    Struct* member_owner = FunctionMemberOwner(func);
    if (member_owner != NULL &&
        CXXSameAccessClass(member_owner, owner)) {
      return true;
    }
  }
  Struct* current_owner = NULL;
  if (func != NULL && TypeIsFunction(func)) {
    Struct* closure_owner = FunctionMemberOwner(func);
    Struct* lambda_context = CXXLambdaEnclosingClassAccessContext(func);
    if (closure_owner != NULL && closure_owner->tag_symbol != NULL &&
        closure_owner->tag_symbol->flags.invented &&
        lambda_context != closure_owner) {
      // A lambda body uses the access of its enclosing function ([class.local]),
      // not the closure type that owns operator().
      current_owner = lambda_context;
    }
  }
  if (current_owner == NULL &&
      compiler->current_class_access_context != NULL) {
    // A static data member initializer is in the scope of its class and may
    // name the class's private and protected members. Template body cloning also
    // re-analyzes member expressions before current_function has a recoverable
    // owner, but still within the instantiated class context.
    current_owner = compiler->current_class_access_context;
  } else if (current_owner == NULL && func != NULL &&
             TypeIsFunction(func)) {
    current_owner = CXXLambdaEnclosingClassAccessContext(func);
  }
  if (CurrentFunctionIsFriendOf(owner) ||
      (lookup_context != owner && CurrentFunctionIsFriendOf(lookup_context)) ||
      EnclosingFunctionHasClassAccess(compiler->current_function, owner) ||
      EnclosingFunctionIsFriendOf(compiler->current_function, owner) ||
      (lookup_context != owner &&
       EnclosingFunctionIsFriendOf(compiler->current_function,
                                   lookup_context))) {
    return true;
  }
  if (current_owner == NULL) {
    return false;
  }
  if (owner != NULL) {
    for (size_t i = 0; i < owner->friend_classes.length; i++) {
      Struct* friend_class = owner->friend_classes.value.p[i];
      for (Struct* accessor = current_owner; accessor != NULL;
           accessor = accessor->lexical_parent) {
        if (CXXSameAccessClass(friend_class, accessor)) {
          // A nested class is a member of its enclosing friend class and uses
          // that class's access rights.
          return true;
        }
      }
    }
  }
  if (lookup_context != owner && lookup_context != NULL) {
    for (size_t i = 0; i < lookup_context->friend_classes.length; i++) {
      Struct* friend_class = lookup_context->friend_classes.value.p[i];
      for (Struct* accessor = current_owner; accessor != NULL;
           accessor = accessor->lexical_parent) {
        if (CXXSameAccessClass(friend_class, accessor)) {
          return true;
        }
      }
    }
  }
  if (CXXSameAccessClass(current_owner, owner)) {
    return true;
  }
  // A nested class is a member of each enclosing class and has the same access
  // rights as any other member, including access to private members of those
  // enclosing classes ([class.access.nest]).
  for (Struct* enclosing = current_owner->lexical_parent; enclosing != NULL;
       enclosing = enclosing->lexical_parent) {
    if (CXXSameAccessClass(enclosing, owner)) {
      return true;
    }
  }
  // Nested lambda closures: while instantiating an outer generic lambda, an
  // inner lambda's body may still be re-analyzed with the outer operator() as
  // current_function.  Capture fields are private to the inner closure; allow
  // access when both the accessor and the member owner are invented lambda
  // closures (the rewritten `this->capture` form is only produced for the
  // owning operator()'s body).
  if (current_owner->tag_symbol != NULL &&
      current_owner->tag_symbol->flags.invented && owner->tag_symbol != NULL &&
      owner->tag_symbol->flags.invented) {
    return true;
  }
  if (original_access == kAccessPrivate) {
    return false;
  }
  if (effective_access == kAccessPrivate) {
    return lookup_context != NULL &&
           CXXSameAccessClass(current_owner, lookup_context);
  }
  return effective_access == kAccessProtected &&
         (lookup_context != NULL &&
          (CXXSameAccessClass(current_owner, lookup_context) ||
           StructIsDerivedFrom(current_owner, lookup_context,
                               /*public_only=*/false)));
}

// Contract: Read-only const-qualification of the object expression after
// member-access analysis; `->` inspects the pointee, `.` the left operand type.
bool MemberReceiverIsConst(BinaryASTNode* node) {
  if (node->base.op == AST_OP(arrow)) {
    return TypeIsPointerOrArray(node->left->type) &&
           TypeIsConst(node->left->type->next);
  }
  return TypeIsConst(node->left->type);
}

// True when the object expression of a member access is volatile-qualified.
bool MemberReceiverIsVolatile(BinaryASTNode* node) {
  if (node->base.op == AST_OP(arrow)) {
    return TypeIsPointerOrArray(node->left->type) &&
           TypeIsVolatile(node->left->type->next);
  }
  return TypeIsVolatile(node->left->type);
}

// A braced temporary (`Find{key}`) is lowered to a compound literal, which
// keeps C's lvalue category.  In C++ it is a prvalue: it binds to `T&&`.
bool ASTNodeIsCXXBracedTemporary(ASTNode* node) {
  return CompilerIsCXX() && node != NULL &&
         node->op == AST_OP(compound_literal);
}

// Contract: Read-only lvalue category for ref-qualifier matching; arrow receivers
// are always lvalues; dot excludes C++ braced temporaries lowered as compound literals.
bool MemberReceiverIsLValue(BinaryASTNode* node) {
  if (node->base.op == AST_OP(arrow)) {
    return true;
  }
  return node->left != NULL &&
         node->left->value_category == kValueCategoryLvalue &&
         !ASTNodeIsCXXBracedTemporary(node->left);
}

// Checks `&`/`&&` ref-qualifier compatibility with the receiver value category.
bool MemberReceiverMatchesRefQualifier(TypeRecord* func,
                                              BinaryASTNode* member_access) {
  if (func == NULL || !TypeIsFunction(func)) {
    return true;
  }
  switch (func->info.function.ref_qualifier) {
    case kCXXRefQualifierNone:
      return true;
    case kCXXRefQualifierLValue:
      // The implicit object parameter of `f() const&` is `const X&`, which
      // binds an rvalue as well ([over.match.funcs]).
      return MemberReceiverIsLValue(member_access) ||
             (func->info.function.is_const_member &&
              !func->info.function.is_volatile_member);
    case kCXXRefQualifierRValue:
      return !MemberReceiverIsLValue(member_access);
  }
  return true;
}

// Allocates a plain struct type record for base-offset calculations.
static TypeRecord* NewStructTypeForAdjustment(Struct* str) {
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(type, str);
  TypeRecordCalculateSize(type);
  return type;
}

// Loads a virtual-base offset from `__vbptr[index]` for the given receiver.
static ASTNode* NewVirtualBaseOffsetLoad(ASTNode* receiver,
                                         int vbtable_index,
                                         SourceLocation location) {
  ASTNode* receiver_clone = CloneReceiverForVirtualLookup(receiver);
  ASTNode* vbptr_name =
      NewStringConstantASTNode(NewString("__vbptr"), NULL, location);
  ASTNode* vbptr =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver_clone,
                       vbptr_name);
  ASTNode* index =
      NewIntConstantASTNode(vbtable_index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* load =
      NewBinaryASTNode(AST_OP(subscript), NULL, location, vbptr, index);
  return AnalyzeExpression(load);
}

// Adds a compile-time byte offset to a runtime offset expression.
static ASTNode* AddStaticOffsetToRuntimeOffset(ASTNode* offset, int byte_offset,
                                               SourceLocation location) {
  if (byte_offset == 0) {
    return offset;
  }
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  ASTNode* tail = NewIntConstantASTNode(byte_offset, int_type, location);
  ASTNode* combined =
      NewBinaryASTNode(AST_OP(plus), int_type, location, offset, tail);
  combined->flags |= kASTAnalyzed;
  return combined;
}

// Rewrites a `.`/`->` member access on a derived subobject so it names a member
// declared on a virtual base: loads the vbptr offset, adjusts the receiver to
// `(byte*)obj + dynamic_offset`, switches to `->`, and clears a fixed byte offset
// when the remaining access must use the dynamic address only.
void ApplyVirtualBaseAdjustmentToMemberReference(BinaryASTNode* node,
                                                        Struct* from,
                                                        Struct* owner,
                                                        int* member_offset) {
  if (node == NULL || from == NULL || owner == NULL || from == owner) {
    return;
  }
  TypeRecord* from_type = NewStructTypeForAdjustment(from);
  TypeRecord* owner_type = NewStructTypeForAdjustment(owner);
  CXXBaseAdjustment adjustment;
  bool found = TypeBaseAdjustment(from_type, owner_type, /*public_only=*/true,
                                  &adjustment);
  TypeRecordDelete(from_type);
  TypeRecordDelete(owner_type);
  if (!found || adjustment.kind != kCXXBaseAdjustmentVirtual) {
    return;
  }

  ASTNode* receiver = ASTNodeMove(node->left);
  if (node->base.op == AST_OP(dot)) {
    receiver = NewAnalyzedBuiltinAddressOf(receiver, receiver->location);
  }
  // The receiver expression is referenced twice below: once to load the
  // virtual-base offset (`receiver->__vbptr[index]`) and once as the base of
  // the pointer adjustment (`receiver + offset`).  If it is anything other than
  // a plain identifier it may have side effects (e.g. a function call whose
  // result yields the object), so evaluate it exactly once into a temporary and
  // reference the temporary in both positions.  Otherwise the receiver would be
  // cloned and evaluated twice.
  ASTNode* seed_assign = NULL;
  if (receiver->op != AST_OP(identifier)) {
    SourceLocation rloc = receiver->location;
    Symbol* temp =
        SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(receiver->type));
    temp->location = rloc;
    ASTNode* temp_lhs = NewIdentifierASTNode(temp, rloc);
    temp_lhs->flags |= kASTNeedAddress;
    seed_assign = NewBinaryASTNode(AST_OP(assign), receiver->type, rloc,
                                   temp_lhs, receiver);
    seed_assign->flags |= kASTAnalyzed;
    receiver = NewIdentifierASTNode(temp, rloc);
  }
  ASTNode* offset =
      NewVirtualBaseOffsetLoad(receiver, adjustment.vbtable_index,
                               node->base.location);
  offset = AddStaticOffsetToRuntimeOffset(offset, adjustment.byte_offset,
                                          node->base.location);
  TypeRecord* adjusted_type =
      NewPointerTo(kQualPlain, NewStructTypeForAdjustment(owner));
  ASTNode* adjusted =
      NewBinaryASTNode(AST_OP(plus), adjusted_type, node->base.location,
                       receiver, offset);
  adjusted->flags |= kASTAnalyzed;
  if (seed_assign != NULL) {
    adjusted = NewBinaryASTNode(AST_OP(comma), adjusted_type,
                                node->base.location, seed_assign, adjusted);
    adjusted->flags |= kASTAnalyzed;
  }
  node->base.op = AST_OP(arrow);
  node->left = adjusted;
  node->left->parent = (ASTNode*)node;
  node->left->child_id = 0;
  if (member_offset != NULL) {
    *member_offset = -1;
  }
}

// True when the type or any array element type is const-qualified.
bool TypeIsEffectivelyConst(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsConst(t)) {
      return true;
    }
    if (t->declarator != kDeclArray) {
      break;
    }
  }
  return false;
}

// Reference binding and conversion for initialization/reference formal matching.
ASTNode* SemanticBindReferenceInitializer(ASTNode* expr,
                                         TypeRecord* reference_type) {
  if (expr == NULL || reference_type == NULL ||
      !TypeIsReference(reference_type) || reference_type->next == NULL ||
      expr->type == NULL) {
    return expr;
  }
  bool rvalue_ref = reference_type->declarator == kDeclRValueReference;
  bool discards_qualifiers =
      TypeIsEffectivelyConst(expr->type) &&
      !TypeIsEffectivelyConst(reference_type->next);
  ASTNode* parent = expr->parent;
  int child_id = expr->child_id;
  if (!TypeEqualIgnoringQualifiers(expr->type, reference_type->next)) {
    ASTNode* base_bound =
        TryBindReferenceToBaseSubobject(expr, reference_type->next);
    if (base_bound != NULL) {
      if (parent != NULL) {
        ASTNodeReplaceChild(parent, child_id, base_bound, false);
      }
      expr = base_bound;
    } else {
      NormalConversion(expr, ReferenceConversionTarget(expr, reference_type));
      if (parent != NULL && parent->op == AST_OP(vardecl)) {
        expr = ((VariableDeclarationASTNode*)parent)->initializer;
      }
    }
  }
  if (discards_qualifiers) {
    SemanticError(expr, "Reference initializer discards qualifiers");
  } else if (!ReferenceCanBind(expr, reference_type)) {
    if (rvalue_ref) {
      SemanticError(expr, "Rvalue reference initializer must not be an lvalue");
    } else if (TypeIsConst(reference_type->next)) {
      SemanticError(expr, "Const reference initializer has incompatible type");
    } else {
      SemanticError(expr, "Reference initializer must be an lvalue");
    }
  }
  if (ReferenceCanBind(expr, reference_type) && !HasAddress(expr)) {
    ASTNode* materialized = MaterializeTemporary(expr, reference_type->next);
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, child_id, materialized, false);
    }
    expr = materialized;
  }
  if (expr != NULL) {
    expr->flags |= kASTNeedAddress;
  }
  return expr;
}

// Returns the overload chain head for a named member function, preferring a true overload set.
StructMember* FindCXXMemberOverloadHead(Struct* owner, String* name) {
  StructMember* fallback = FindStructMember(owner, name);
  if (owner == NULL || name == NULL) {
    return fallback;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        !StringEqualString(&member->symbol->name, name)) {
      continue;
    }
    if (member->overload_next != NULL) {
      return member;
    }
    if (fallback == NULL) {
      fallback = member;
    }
  }
  return fallback;
}


// Mutates `node->left->type` (and sometimes `node` type) from concrete template
// aliases and placeholder origins. Returns `node` only for the enable_if_t failure
// shortcut; otherwise NULL to continue construction analysis. Does not replace `node`
// in its parent.
static ASTNode* CXXFunctionalConstructionInstantiateCalleeTypes(VectorASTNode* node) {
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && id->symbol->flags.is_template &&
        StorageIs(id->symbol->storage, STO(typedef)) &&
        id->template_arguments != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            id->template_arguments)) {
      TypeRecord* alias_type = TypeInstantiateClassTemplate(
          &compiler->syntax, id->symbol, id->template_arguments);
      if (alias_type == NULL) {
        // `enable_if_t<false, int>()` is ill-formed.  Value-initializing the
        // unresolved alias as `int` made every SFINAE overload viable.
        ASTNodeSetType((ASTNode*)node,
                       NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                             kQualPlain));
        return (ASTNode*)node;
      }
      ASTNodeSetType(node->left, alias_type);
    }
  }
  // `using Period = typename T::period` can stay a placeholder
  // (`kTypeInt|kTypeUnknown` plus `ratio<1, 1000000>` arguments) after the
  // function template is instantiated.  `const auto v = Period{}` then
  // value-initializes that placeholder as a scalar.  Instantiate the class
  // once the arguments are concrete.
  if (node->left->type != NULL && !TypeIsStructOrUnion(node->left->type) &&
      node->left->type->template_origin != NULL &&
      node->left->type->template_arguments != NULL &&
      node->left->type->template_arguments->length > 0 &&
      !TemplateArgumentVectorContainsTemplateParameter(
          node->left->type->template_arguments)) {
    TypeRecord* concrete = TypeInstantiateClassTemplate(
        &compiler->syntax, node->left->type->template_origin,
        node->left->type->template_arguments);
    bool concrete_class =
        concrete != NULL && TypeIsStructOrUnion(concrete) &&
        concrete->info.struct_info != NULL &&
        !concrete->info.struct_info->is_template;
    bool concrete_scalar =
        concrete != NULL && !TypeIsStructOrUnion(concrete) &&
        (concrete->type & kTypeUnknown) == 0 &&
        !TypeContainsTemplateParameter(concrete);
    if (concrete_class || concrete_scalar) {
      ASTNodeSetType(node->left, concrete);
    } else {
      TypeRecordDelete(concrete);
    }
  }
  return NULL;
}

// Handles non-class typedef functional casts (`typedef T(...)`). On success sets
// *result to the analyzed replacement (or dependent `node`) and returns true.
// Returns false when the callee is already a class type (caller continues).
// May replace `node` in its parent with a cast and move argument children.
static bool CXXFunctionalConstructionTryTypedefScalarRoute(VectorASTNode* node,
                                                            ASTNode** result) {
  if (TypeIsStructOrUnion(node->left->type)) {
    return false;
  }
  if (node->left->op != AST_OP(identifier)) {
    *result = NULL;
    return true;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  if (id->symbol == NULL ||
      !StorageIs(id->symbol->storage, STO(typedef)) ||
      node->children->length > 1) {
    *result = NULL;
    return true;
  }
  // A typedef that is still an unresolved placeholder (`typename T::period`
  // before the member alias is a concrete class) must not be value-initialized
  // as a scalar.  Casting 0 to that placeholder becomes a cast to the class
  // once the alias is completed (`const auto v = Period{}`).
  if ((node->left->type->type & kTypeUnknown) != 0) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node, TypeRecordCopy(node->left->type));
    *result = (ASTNode*)node;
    return true;
  }
  SourceLocation location = node->base.location;
  ASTNode* initializer =
      node->children->length == 0
          ? NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                  location)
          : ASTNodeMove(node->children->value.p[0]);
  ASTNode* cast =
      NewCastASTNode(TypeRecordCopy(node->left->type), location, initializer);
  ASTNode* parent = node->base.parent;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, node->base.child_id, cast, true);
  }
  *result = AnalyzeExpression(cast);
  return true;
}

// Resolves CTAD / explicit template-ids into a heap `TypeRecord*` owned by the
// caller on success. Returns true when resolution aborts construction: sets
// *ctad_failed when deduction fails (caller returns `&node->base`), otherwise
// leaves *out_type NULL. Deletes temporary placeholder/deduced types on success.
static bool CXXFunctionalConstructionResolveTargetType(VectorASTNode* node,
                                                         TypeRecord** out_type,
                                                         bool* ctad_failed) {
  *out_type = NULL;
  *ctad_failed = false;
  // An explicit template-id used as a functional cast (`A<int>(...)`) names the
  // concrete specialization directly; instantiate it from those arguments
  // rather than attempting class template argument deduction from the
  // constructor call.  When the construction appears inside another template,
  // the body cloner substitutes the arguments to concrete types before this
  // runs, so this handles the dependent case (e.g. `allocator<CharT>()`) too.
  TypeRecord* explicit_type = NULL;
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    Symbol* template_symbol = id->symbol;
    // Cloning a member body maps the injected-class-name to the current
    // specialization, but `SP<int>(...)` inside `SP<char>` still names the
    // specialization its own arguments select.
    if (template_symbol != NULL && !template_symbol->flags.is_template &&
        id->template_arguments != NULL && template_symbol->type != NULL &&
        TypeIsStructOrUnion(template_symbol->type) &&
        template_symbol->type->template_arguments != NULL &&
        template_symbol->type->template_origin != NULL &&
        template_symbol->type->template_origin->flags.is_template) {
      template_symbol = template_symbol->type->template_origin;
    }
    if (template_symbol != NULL && template_symbol->flags.is_template &&
        template_symbol->type != NULL &&
        id->template_arguments != NULL &&
        !TemplateArgumentVectorContainsTemplateParameter(
            id->template_arguments)) {
      bool class_template =
          TypeIsStructOrUnion(template_symbol->type) &&
          template_symbol->type->template_arguments == NULL &&
          template_symbol->type->info.struct_info != NULL &&
          template_symbol->type->info.struct_info->is_template;
      bool alias_template = StorageIs(template_symbol->storage, STO(typedef));
      if (class_template || alias_template) {
        explicit_type = TypeInstantiateClassTemplate(
            &compiler->syntax, template_symbol, id->template_arguments);
      }
    }
  }

  TypeRecord* placeholder_type = NULL;
  TypeRecord* construction_type =
      explicit_type != NULL ? explicit_type : node->left->type;
  bool dependent_explicit_args = false;
  if (explicit_type == NULL && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* args_id = (IdentifierASTNode*)node->left;
    dependent_explicit_args =
        args_id->template_arguments != NULL &&
        TemplateArgumentVectorContainsTemplateParameter(
            args_id->template_arguments);
  }
  if (explicit_type == NULL && !dependent_explicit_args &&
      !TypeIsClassTemplatePlaceholder(construction_type) &&
      node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    /* For a plain class template the callee type carries no template
     * arguments. For an alias template (`using Alias = Foo<T>;`) the callee
     * type is the aliased `Foo<T>` and already has template arguments, but it
     * still needs the alias placeholder origin so CTAD deduces and instantiates
     * the underlying class template. Derive the placeholder from the symbol in
     * both cases. */
    bool is_alias_template =
        id->symbol != NULL && id->symbol->flags.is_template &&
        StorageIs(id->symbol->storage, STO(typedef));
    if (construction_type->template_arguments == NULL || is_alias_template) {
      placeholder_type = TypeClassTemplatePlaceholderFromSymbol(id->symbol);
      if (placeholder_type != NULL) {
        construction_type = placeholder_type;
      }
    }
  }
  TypeRecord* deduced_type = explicit_type;
  if (explicit_type == NULL && !dependent_explicit_args &&
      TypeIsClassTemplatePlaceholder(construction_type)) {
    bool alias_rejected = false;
    Symbol* class_template =
        TypeClassTemplatePlaceholderOrigin(construction_type);
    deduced_type = TypeDeduceClassTemplateFromPlaceholder(
        &compiler->syntax, construction_type, node->children,
        /*allow_explicit=*/true, &alias_rejected);
    if (deduced_type == NULL) {
      if (alias_rejected) {
        SemanticError((ASTNode*)node,
                      "Deduced template arguments do not match alias template");
      } else {
        SemanticError((ASTNode*)node,
                      "Could not deduce template arguments for %s",
                      class_template != NULL ? class_template->name.value
                                             : "<class template>");
      }
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
      if (placeholder_type != NULL) {
        TypeRecordDelete(placeholder_type);
      }
      *ctad_failed = true;
      return true;
    }
    construction_type = deduced_type;
  }

  TypeRecord* type = TypeRecordCopy(construction_type);
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    TypeRecordDelete(type);
    return true;
  }
  // A template-id with dependent arguments (`integral_constant<bool, C<T>>`)
  // cannot be instantiated yet.  Keep the written arguments on the result
  // type so later overload resolution still sees a dependent class type
  // instead of the primary template (which would match every specialization).
  if (dependent_explicit_args && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* args_id = (IdentifierASTNode*)node->left;
    // An alias template's pattern type already carries the pattern's own
    // arguments (`using HashAlias = Hash<X>` stores `Hash<X>`).  A use
    // `HashAlias<const T*>{}` must keep those written arguments.  Leaving the
    // pattern arguments in place substitutes `X` from the enclosing template
    // (index 0), so `Hash<const T*>` collapses to `Hash<T>`.
    bool alias_template =
        args_id->symbol != NULL && args_id->symbol->flags.is_template &&
        StorageIs(args_id->symbol->storage, STO(typedef));
    if (args_id->symbol != NULL &&
        (type->template_arguments == NULL || alias_template)) {
      if (type->template_arguments != NULL) {
        VectorDeleteWithContents(
            type->template_arguments,
            (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
        type->template_arguments = NULL;
      }
      type->template_origin = args_id->symbol;
      type->template_arguments =
          TemplateArgumentVectorCopy(args_id->template_arguments);
      type->template_parameter_summary = kTypeTemplateParameterSummaryUnknown;
    }
  }
  if (placeholder_type != NULL) {
    TypeRecordDelete(placeholder_type);
  }
  if (deduced_type != NULL) {
    TypeRecordDelete(deduced_type);
  }
  TypeRecordCalculateSize(type);
  if (dependent_explicit_args) {
    // A construction such as `Info<decltype(ext)>{...}` inside a generic
    // lambda still names the primary class while `ext` has its invented
    // `auto&` type.  Resolving constructors now would compare arguments
    // against the primary's unsubstituted `ExtensionT&`.  Keep the call intact;
    // cloning the lambda call operator will substitute `decltype(ext)` and
    // re-run construction against the concrete specialization.
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node, type);
    *ctad_failed = true;
    return true;
  }
  *out_type = type;
  return false;
}

// True when any overload of the class-named constructor head is user-declared.
static bool CXXFunctionalConstructionHasUserDeclaredConstructor(
    StructMember* constructor_head) {
  for (StructMember* candidate = constructor_head; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->symbol != NULL &&
        !candidate->symbol->flags.invented) {
      return true;
    }
  }
  return false;
}

// Splices `cast` over the functional-construction call and analyzes it.
static ASTNode* CXXFunctionalConstructionSpliceCastAndAnalyze(
    VectorASTNode* node, TypeRecord* type, ASTNode* arg) {
  ASTNode* cast = NewCastASTNode(type, node->base.location, arg);
  ASTNode* parent = node->base.parent;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, node->base.child_id, cast, true);
  }
  return AnalyzeExpression(cast);
}

// Splices a compound literal temporary over the call, analyzes it, and marks prvalue.
static ASTNode* CXXFunctionalConstructionSpliceCompoundLiteralAndAnalyze(
    VectorASTNode* node, TypeRecord* type, ASTNode* initializer) {
  SourceLocation location = node->base.location;
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* temp_id = NewIdentifierASTNode(temp, location);
  temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* literal =
      NewCompoundLiteralASTNode(temp_id, location, initializer);
  ASTNode* parent = node->base.parent;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, node->base.child_id, literal, true);
  }
  ASTNode* analyzed = AnalyzeExpression(literal);
  analyzed->value_category = kValueCategoryPrvalue;
  return analyzed;
}

// Single-argument `T(arg)` when the argument converts to `T` via a user-defined
// conversion operator ([over.ics.user]); returns NULL to continue constructor routing.
static ASTNode* CXXFunctionalConstructionTryConversionOperatorCast(
    VectorASTNode* node, TypeRecord* type) {
  if (node->children->length != 1 ||
      !ClassHasConversionOperatorTo(node->children->value.p[0], type)) {
    return NULL;
  }
  ASTNode* arg = ASTNodeMove(node->children->value.p[0]);
  return CXXFunctionalConstructionSpliceCastAndAnalyze(node, type, arg);
}

// Aggregate `T(...)` / `T({...})` when the class is still an aggregate without a
// user-declared constructor; NULL continues toward constructor/cast lowering.
static ASTNode* CXXFunctionalConstructionTryAggregateFunctionalInit(
    VectorASTNode* node, TypeRecord* type,
    bool has_user_declared_constructor) {
  if (!type->info.struct_info->is_aggregate ||
      type->info.struct_info->is_template ||
      has_user_declared_constructor) {
    return NULL;
  }
  SourceLocation location = node->base.location;
  ASTNode* initializer = NULL;
  bool source_used_braces =
      (node->base.flags & kASTCXXBracedTemporary) != 0;
  // `A(a)` runs A's copy or move constructor, not aggregate initialization of
  // A's first member from `a`.
  ASTNode* only = node->children->length == 1 ? node->children->value.p[0]
                                               : NULL;
  if (!source_used_braces && only != NULL && only->op != AST_OP(braced_init) &&
      only->type != NULL && TypeIsStructOrUnion(only->type) &&
      TypeEqualIgnoringQualifiers(only->type, type)) {
    return CXXFunctionalConstructionSpliceCastAndAnalyze(
        node, type, ASTNodeMove(node->children->value.p[0]));
  }
  if (!source_used_braces && node->children->length == 1 &&
      node->children->value.p[0] != NULL &&
      ((ASTNode*)node->children->value.p[0])->op == AST_OP(braced_init)) {
    initializer = ASTNodeMove(node->children->value.p[0]);
  } else {
    Vector* elements = NewVector();
    for (size_t i = 0; i < node->children->length; i++) {
      ASTNode* child = node->children->value.p[i];
      if (child != NULL) {
        VectorAppend(elements, ASTNodeMove(child));
      }
    }
    initializer = NewBracedInitializerASTNode(elements, NULL, location);
  }
  return CXXFunctionalConstructionSpliceCompoundLiteralAndAnalyze(node, type,
                                                                  initializer);
}

// No viable constructor head: explicit cast for one argument, empty compound
// literal for `T()`, or NULL when construction cannot complete.
static ASTNode* CXXFunctionalConstructionTryCastWithoutViableConstructor(
    VectorASTNode* node, TypeRecord* type, StructMember* constructor) {
  if (constructor != NULL && constructor->is_member_function &&
      constructor->symbol != NULL && constructor->symbol->type != NULL &&
      TypeIsFunction(constructor->symbol->type) &&
      constructor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  if (node->children->length == 1) {
    ASTNode* arg = ASTNodeMove(node->children->value.p[0]);
    return CXXFunctionalConstructionSpliceCastAndAnalyze(node, type, arg);
  }
  if (node->children->length == 0) {
    ASTNode* initializer =
        NewBracedInitializerASTNode(NewVector(), NULL, node->base.location);
    return CXXFunctionalConstructionSpliceCompoundLiteralAndAnalyze(node, type,
                                                                  initializer);
  }
  return NULL;
}

// Retargets constructor overload owners to the constructed class unless the
// overload is an inherited base constructor ([dcl.init.list] / inherited ctors).
static void CXXFunctionalConstructionRetargetConstructorMemberOwners(
    TypeRecord* type, StructMember* constructor) {
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!(candidate->is_member_function && candidate->symbol != NULL &&
          candidate->symbol->type != NULL &&
          TypeIsFunction(candidate->symbol->type))) {
      continue;
    }
    Struct* recorded_owner =
        candidate->symbol->type->info.function.cxx_member_owner;
    int base_offset = 0;
    bool inherited_base_constructor =
        recorded_owner != NULL &&
        recorded_owner != type->info.struct_info &&
        StructHasBaseStruct(type->info.struct_info, recorded_owner,
                            &base_offset);
    if (!inherited_base_constructor) {
      candidate->symbol->type->info.function.cxx_member_owner =
          type->info.struct_info;
      SymbolClearAsmName(candidate->symbol);
      SymbolSetCXXMangledAsmName(candidate->symbol);
    }
  }
}

static bool CXXFunctionalConstructionHasNonInventedConstructor(
    StructMember* constructor) {
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->symbol != NULL &&
        !candidate->symbol->flags.invented) {
      return true;
    }
  }
  return false;
}

// `T(args...)` as `(T temp = T::T(args...), temp)` with member-call lowering.
static ASTNode* CXXFunctionalConstructionLowerToConstructorComma(
    VectorASTNode* node, TypeRecord* type, StructMember* constructor) {
  SourceLocation location = node->base.location;
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member = NewStructMemberASTNode(constructor, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);

  Vector* actuals = NewVector();
  for (size_t i = 0; i < node->children->length; i++) {
    VectorAppend(actuals, ASTNodeMove(node->children->value.p[i]));
  }
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  LowerMemberFunctionCall((VectorASTNode*)constructor_call);
  ASTNode* result = NewIdentifierASTNode(temp, location);
  ASTNode* comma =
      NewBinaryASTNode(AST_OP(comma), type, location, constructor_call, result);

  ASTNode* parent = node->base.parent;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, node->base.child_id, comma, true);
  }
  ASTNode* analyzed = AnalyzeExpression(comma);
  analyzed->value_category = kValueCategoryPrvalue;
  return analyzed;
}

// Lowers class functional construction to casts, aggregate/compound literals, or
// a constructor member call. Takes ownership of `type` (freed on NULL return paths
// that abandon construction). Replaces `node` in its parent when lowering.
static ASTNode* CXXFunctionalConstructionFinishWithType(VectorASTNode* node,
                                                        TypeRecord* type) {
  String* constructor_name = type->info.struct_info->tag_name;
  StructMember* constructor =
      FindCXXMemberOverloadHead(type->info.struct_info, constructor_name);
  bool has_user_declared_constructor =
      CXXFunctionalConstructionHasUserDeclaredConstructor(constructor);

  ASTNode* finished =
      CXXFunctionalConstructionTryConversionOperatorCast(node, type);
  if (finished != NULL) {
    return finished;
  }

  finished = CXXFunctionalConstructionTryAggregateFunctionalInit(
      node, type, has_user_declared_constructor);
  if (finished != NULL) {
    return finished;
  }

  finished =
      CXXFunctionalConstructionTryCastWithoutViableConstructor(node, type,
                                                               constructor);
  if (finished != NULL) {
    return finished;
  }

  CXXFunctionalConstructionRetargetConstructorMemberOwners(type, constructor);
  bool has_non_invented_constructor =
      CXXFunctionalConstructionHasNonInventedConstructor(constructor);

  if (!has_non_invented_constructor && node->children->length == 0) {
    ASTNode* initializer =
        NewBracedInitializerASTNode(NewVector(), NULL, node->base.location);
    return CXXFunctionalConstructionSpliceCompoundLiteralAndAnalyze(node, type,
                                                                    initializer);
  }
  if (!has_non_invented_constructor && node->children->length > 1) {
    return NULL;
  }

  return CXXFunctionalConstructionLowerToConstructorComma(node, type, constructor);
}

// Dispatcher for `T(...)` / CTAD functional class construction on a call node.
// May replace the entire call in its parent; caller must use the returned AST.
// Preserves legacy ordering: instantiate callee type, typedef scalar routes, then
// class target resolution, then constructor lowering.
static ASTNode* AnalyzeCXXFunctionalClassConstruction(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL) {
    return NULL;
  }
  // Functional construction is spelled with a type-name.  A class-typed
  // expression in the callee position (`object()`) is instead a call to
  // `operator()`; if no such member exists it must be rejected as non-callable,
  // not reinterpreted as value-initialization of the object's class.
  if (node->left->op != AST_OP(identifier)) {
    return NULL;
  }
  Symbol* callee_symbol = ((IdentifierASTNode*)node->left)->symbol;
  if (callee_symbol == NULL ||
      (!StorageIs(callee_symbol->storage, STO(typedef)) &&
       !SymbolIsTagSymbol(callee_symbol))) {
    return NULL;
  }
  // An alias formed while its class template was only forward-declared
  // (`using ValueView = ValueImpl<false>;` before ValueImpl's definition)
  // still names the deferred specialization; constructing through it must
  // see that specialization's constructors, not the primary's.
  if (StorageIs(callee_symbol->storage, STO(typedef)) &&
      !compiler->syntax.parsing_template_declaration &&
      node->left->type != NULL && TypeIsStructOrUnion(node->left->type) &&
      node->left->type->info.struct_info != NULL &&
      node->left->type->info.struct_info->is_template &&
      node->left->type->template_arguments != NULL &&
      !TemplateArgumentVectorContainsTemplateParameter(
          node->left->type->template_arguments)) {
    TypeRecord* materialized = TypeMaterializeClassTemplateSpecialization(
        &compiler->syntax, node->left->type);
    if (materialized != node->left->type) {
      ASTNodeSetType(node->left, materialized);
    }
  }
  ASTNode* early = CXXFunctionalConstructionInstantiateCalleeTypes(node);
  if (early != NULL) {
    return early;
  }
  ASTNode* typedef_route = NULL;
  if (CXXFunctionalConstructionTryTypedefScalarRoute(node, &typedef_route)) {
    return typedef_route;
  }
  if (node->left->type->info.struct_info == NULL ||
      node->left->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  TypeRecord* target_type = NULL;
  bool ctad_failed = false;
  if (CXXFunctionalConstructionResolveTargetType(node, &target_type,
                                                 &ctad_failed)) {
    if (ctad_failed) {
      return &node->base;
    }
    return NULL;
  }
  return CXXFunctionalConstructionFinishWithType(node, target_type);
}


// Materializes a class lvalue actual as a prvalue temporary when passed by value
// to a matching class formal. Does not mutate the call node; returns `actual` when
// no materialization applies. Caller owns the returned subtree.
static ASTNode* MaterializeCXXByValueClassArgument(ASTNode* actual,
                                                   TypeRecord* formal_type) {
  // An argument of an already-analyzed call (a cloned pack expansion) is our
  // own `(T(&tmp, actual), tmp)`.  Re-analysis makes the comma an lvalue, and
  // copying `tmp` again would need a copy constructor (`unique_ptr`).
  if (actual != NULL && actual->op == AST_OP(comma) &&
      (actual->flags & kASTFunctionParameterTemporary) != 0) {
    return actual;
  }
  if (!CompilerIsCXX() || actual == NULL || formal_type == NULL ||
      !TypeIsStructOrUnion(formal_type) ||
      !TypeIsStructOrUnion(actual->type) ||
      !TypeEqualIgnoringQualifiers(actual->type, formal_type) ||
      formal_type->info.struct_info == NULL ||
      formal_type->info.struct_info->tag_name == NULL) {
    return actual;
  }

  String* constructor_name = formal_type->info.struct_info->tag_name;
  StructMember* constructor =
      FindStructMember(formal_type->info.struct_info, constructor_name);
  if (constructor == NULL || !constructor->is_member_function ||
      constructor->symbol == NULL || constructor->symbol->type == NULL ||
      !constructor->symbol->type->info.function.is_constructor) {
    return actual;
  }
  bool has_nontrivial_unary_constructor = false;
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        !candidate->symbol->type->info.function.is_constructor) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (info->varargs ||
        (info->prototype.length >= 2 && !info->is_trivial_special_member)) {
      has_nontrivial_unary_constructor = true;
      break;
    }
  }
  if (!has_nontrivial_unary_constructor) {
    return actual;
  }

  SourceLocation location = actual->location;
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, formal_type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(constructor_name->value), NULL,
                               location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);

  Vector* actuals = NewVector();
  VectorAppend(actuals, ASTNodeMove(actual));
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* result = NewIdentifierASTNode(temp, location);
  ASTNode* comma =
      NewBinaryASTNode(AST_OP(comma), formal_type, location, constructor_call,
                       result);
  ASTNode* analyzed = AnalyzeExpression(comma);
  analyzed->value_category = kValueCategoryPrvalue;
  analyzed->flags |= kASTFunctionParameterTemporary;
  return analyzed;
}

// Handle calling an object that has an `operator()` member, i.e. a lambda's
// closure object or any other functor: `obj(args)` is rewritten into the member
// call `obj.operator()(args)`.  The receiver and argument nodes are detached
// from the original call (leaving NULL holes the caller no longer owns) and
// re-parented under the new member call.  Returns NULL when the callee is not a
// class/union with a callable `operator()`, leaving `node` untouched.
static ASTNode* TryAnalyzeOverloadedCallOperator(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      !TypeIsStructOrUnion(node->left->type)) {
    return NULL;
  }
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && StorageIs(id->symbol->storage, STO(typedef))) {
      return NULL;
    }
  }

  StructMember* member =
      FindStructMemberByName(node->left->type->info.struct_info, "operator()");
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }

  // A generic lambda captured inside a template whose closure does not depend
  // on the enclosing template keeps its call operator numbered relative to that
  // template; rebase it to a standalone 0-based template now so deduction and
  // instantiation of `operator()` can proceed (no-op in the common case).
  Struct* closure = node->left->type->info.struct_info;
  for (Symbol* overload = member->symbol; overload != NULL;
       overload = overload->overload_next) {
    TypeRebaseNonDependentLambdaCallOperator(closure, overload);
  }

  // Move the actuals out of the original call node into the member call.
  Vector* actuals = NewVector();
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* actual = node->children->value.p[i];
    VectorSet(node->children, i, NULL);
    if (actual != NULL) {
      actual->parent = NULL;
    }
    VectorAppend(actuals, actual);
  }
  ASTNode* receiver = node->left;
  node->left = NULL;
  if (receiver != NULL) {
    receiver->parent = NULL;
  }
  ASTNode* call =
      NewOperatorMemberCall(receiver, "operator()", actuals,
                            node->base.location);
  return ReplaceVectorWithCall(node, call);
}

// True when any call operand is still template-dependent for deduction purposes.
// A bare function-template name (`std::endl`, `&h`) as an argument: its
// pattern type mentions template parameters, but the call is not dependent;
// a function-pointer formal supplies the target type ([over.over]).
static bool ActualIsFunctionTemplateName(ASTNode* actual) {
  if (actual != NULL && actual->op == AST_OP(address)) {
    actual = ((UnaryASTNode*)actual)->sub;
  }
  if (actual == NULL || actual->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)actual;
  return id->symbol != NULL && actual->type != NULL &&
         TypeIsFunction(actual->type) && SymbolIsTemplateFunction(id->symbol) &&
         !TemplateArgumentVectorContainsTemplateParameter(
             id->template_arguments);
}

static bool CallActualsContainTemplateParameter(VectorASTNode* call) {
  for (size_t i = 0; call != NULL && i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual == NULL) {
      continue;
    }
    // A generic lambda's closure type and a function template's pattern type
    // carry template parameters, but a function-pointer formal can still
    // convert them ([over.over] / funcaddr).
    if ((actual->flags & kASTLambdaExpression) != 0 ||
        ActualIsFunctionTemplateName(actual)) {
      continue;
    }
    // Unknown is the placeholder given to a dependent operand (for example
    // `*this->field` while the capture's type is still a template parameter).
    // Instantiating a function template against it caches a specialization on
    // that placeholder, and the later concrete argument no longer matches.
    if (TypeContainsTemplateParameter(actual->type) ||
        (actual->type != NULL && TypeIsUnknown(actual->type)) ||
        ExpressionIsTemplateDependent(actual)) {
      return true;
    }
  }
  return false;
}

// `const auto p = dependent(); duration_cast<D>(p.second)` is parsed while
// `p` is still an auto placeholder.  Deduction against that placeholder
// fails; the call has to wait until instantiation replaces `auto`.
static bool CallActualsContainUndeducedAuto(VectorASTNode* call) {
  for (size_t i = 0; call != NULL && i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual != NULL && TypeContainsAuto(actual->type)) {
      return true;
    }
  }
  return false;
}

// True when an actual is a call flagged `kASTDependentFunctorCall`.
static bool CallActualsContainDependentFunctorCall(VectorASTNode* call) {
  for (size_t i = 0; call != NULL && i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    // A braced-init-list is resolved against the selected formal type. In a
    // concrete function, a dependent nested call such as `std::move(x)` must
    // not defer the enclosing overload set: selecting `insert(init_type&&)`
    // provides the target that makes the brace elements analyzable.
    if (actual != NULL && actual->op == AST_OP(braced_init) &&
        compiler->current_function != NULL &&
        !TypeContainsTemplateParameter(compiler->current_function)) {
      continue;
    }
    if (actual != NULL && (actual->flags & kASTDependentFunctorCall) != 0) {
      return true;
    }
  }
  return false;
}
// True when `node` is a call through a member-function access whose overload
// cannot yet be resolved because it is still value/type-dependent: either the
// explicit template arguments or an actual argument mention a template
// parameter.  This arises when partially instantiating an enclosing member
// template (its own parameters remain generic while the class arguments are
// baked in), e.g. `this->emplace<index_of<remove_cvref_t<T>, Types...>::value>(
// forward<T>(value))` inside a variant's `operator=`.  Such a call must be
// deferred (left unresolved) until the enclosing member template is
// instantiated with concrete arguments, rather than run through overload
// resolution now (which would fail to deduce and diagnose a spurious error).
static bool IsDependentMemberTemplateCall(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return false;
  }
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  if (access->right == NULL || access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
  if (member_node->member == NULL || !member_node->member->is_member_function) {
    return false;
  }
  // Dependence in an argument only requires deferral when the member overload
  // set itself contains a function template whose deduction must wait.  An
  // ordinary member of an already-concrete class can accept a function
  // template name contextually through a concrete function-pointer parameter
  // ([temp.deduct.funcaddr]); treating that template name as an unresolved
  // call dependency skips overload resolution and leaves the primary template
  // address in the generated code (e.g. ostream::operator<<(std::endl)).
  bool has_member_template = false;
  for (StructMember* candidate = member_node->member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol != NULL &&
        SymbolIsTemplateFunction(candidate->symbol)) {
      has_member_template = true;
      break;
    }
  }
  if (TemplateArgumentVectorContainsTemplateParameter(
          member_node->template_arguments)) {
    return true;
  }
  if (!CallActualsContainTemplateParameter(node)) {
    return false;
  }
  bool dependence_only_inside_braces = true;
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* actual = node->children->value.p[i];
    if (actual != NULL && TypeContainsTemplateParameter(actual->type) &&
        actual->op != AST_OP(braced_init)) {
      dependence_only_inside_braces = false;
      break;
    }
  }
  // A concrete overload set must see an untyped braced-init-list before a
  // speculative member-template selection can make the nested expressions
  // look dependent. The selected formal supplies the brace target type.
  if (dependence_only_inside_braces &&
      compiler->current_function != NULL &&
      !TypeContainsTemplateParameter(compiler->current_function)) {
    return false;
  }
  if (has_member_template) {
    return true;
  }
  // An ordinary member call still has to wait for genuinely dependent values
  // (for example, a member-template constructor initializing a concrete member
  // from its U&& argument), but not for a bare function-template name.
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* actual = node->children->value.p[i];
    if (actual == NULL || !TypeContainsTemplateParameter(actual->type)) {
      continue;
    }
    if ((actual->flags & kASTLambdaExpression) != 0 ||
        ActualIsFunctionTemplateName(actual)) {
      continue;
    }
    return true;
  }
  return false;
}

/* True when a member-function-template call names its member template with
 * explicit template arguments that are still template-dependent (e.g.
 * `rest.template ctor<Target>(...)` where `Target` is an unresolved parameter
 * of the enclosing member template). Such a call cannot be resolved to a
 * concrete member overload no matter what its actuals look like -- even when an
 * actual is a pack expansion -- so it must be deferred until the member
 * template is instantiated with concrete arguments. */
// Member call whose explicit template-args are still dependent.
static bool IsMemberTemplateCallWithDependentExplicitArgs(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return false;
  }
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  if (access->right == NULL || access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
  if (member_node->member == NULL || !member_node->member->is_member_function) {
    return false;
  }
  return TemplateArgumentVectorContainsTemplateParameter(
      member_node->template_arguments);
}

// Assigns a dependent placeholder result type to a deferred member-template call.
static void SetDependentMemberTemplateCallType(VectorASTNode* node) {
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  StructMemberASTNode* member_node = (StructMemberASTNode*)access->right;
  TypeRecord* return_type =
      TypeRecordAdopt(TypeSubstituteFunctionTemplateReturnType(
          &compiler->syntax, member_node->member->symbol,
          member_node->template_arguments));
  if (return_type == NULL) {
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                         kQualPlain));
    return;
  }
  if (TypeIsReference(return_type)) {
    ASTNodeSetType((ASTNode*)node, return_type->next);
    node->base.value_category =
        return_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)node, return_type);
  }
  TypeRecordDelete(return_type);
}

// A compiler-generated base-subobject destructor cleanup call spells the base
// by the name recorded when the enclosing template was *defined* -- the base's
// primary template name (e.g. "A" for a base `A<T>").  After instantiation the
// base subobject `A<int>` registers its destructor under the *specialized* tag
// ("~A<int>"), so the spelled "~A" no longer resolves by exact name.  Search the
// (direct or indirect) base subobjects of `str` for one whose tag name or
// primary-template name equals `base_name`, returning that base's actual tag
// name so the spelling can be rewritten to a resolvable destructor name.  Only
// bases are searched: a spelling that names the object's own type is handled by
// the own-destructor rewrite instead.
String* CXXFindBaseTagNameForDestructorSpelling(Struct* str,
                                                       const char* base_name) {
  if (str == NULL || base_name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (base_struct->tag_name != NULL &&
        strcmp(base_struct->tag_name->value, base_name) == 0) {
      return base_struct->tag_name;
    }
    Symbol* origin = base->type->template_origin;
    if (origin == NULL && base_struct->tag_symbol != NULL &&
        base_struct->tag_symbol->type != NULL) {
      origin = base_struct->tag_symbol->type->template_origin;
    }
    if (origin != NULL && strcmp(origin->name.value, base_name) == 0) {
      return base_struct->tag_name;
    }
    // A template-id base ("O<int>") whose destructor is spelled by its primary
    // name ("~O").  template_origin is not always propagated onto the base type
    // (notably for instantiations that carry virtual bases), so also match the
    // primary-template prefix textually: the tag name begins with `base_name`
    // immediately followed by the template-argument list opener '<'.
    if (base_struct->tag_name != NULL) {
      size_t n = strlen(base_name);
      const char* tag = base_struct->tag_name->value;
      if (strncmp(tag, base_name, n) == 0 && tag[n] == '<') {
        return base_struct->tag_name;
      }
    }
    String* nested =
        CXXFindBaseTagNameForDestructorSpelling(base_struct, base_name);
    if (nested != NULL) {
      return nested;
    }
  }
  return NULL;
}

// Handles an explicit destructor or pseudo-destructor call written through a
// member-access callee: `obj.~T()`, `p->~T()`, or `p->~int()`.  This is needed
// for generic code (e.g. containers destroying their elements) where the named
// type may resolve to a class or to a scalar, and where the spelled name (such
// as a template parameter `~T`) does not textually match the class's own
// destructor name.
//
//   * If the object is a class type with a materialized (user-declared or
//     synthesized non-trivial) destructor, the spelled name is rewritten to the
//     object type's real destructor name and NULL is returned so the ordinary
//     member-call path resolves and invokes it.
//   * Otherwise -- a scalar type, or a class whose destructor is trivial and
//     therefore not materialized as a member -- the call is a well-formed no-op
//     of type void that still evaluates the object expression for its side
//     effects, and that replacement expression is returned.
//
// Returns NULL when this is not an explicit destructor call, or when the object
// type is still dependent (so analysis is deferred to instantiation).
static ASTNode* TryAnalyzeCXXExplicitDestructorCall(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return NULL;
  }
  // A user-written destructor call takes no explicit arguments, but a
  // compiler-generated base-subobject cleanup call carries the implicit
  // `__complete_object` flag argument when the base has virtual bases.  We must
  // still rewrite a dependent/primary-name spelling ("~O" -> "~O<int>") in that
  // case, so do not bail here; instead only the trivial/pseudo-destructor no-op
  // replacement below is suppressed when arguments are present (that path would
  // otherwise drop the argument).
  bool has_arguments = node->children != NULL && node->children->length != 0;
  BinaryASTNode* access = (BinaryASTNode*)node->left;
  if (access->right == NULL || access->right->op != AST_OP(string)) {
    return NULL;
  }
  String* spelled = ((ConstantASTNode*)access->right)->value.string;
  if (spelled == NULL || spelled->length == 0 || spelled->value[0] != '~') {
    return NULL;
  }

  // Learn the (possibly concrete) type of the object being destroyed.
  access->left = AnalyzeExpression(access->left);
  if (access->left != NULL) {
    access->left->parent = (ASTNode*)access;
    access->left->child_id = 0;
  }
  TypeRecord* object_type = access->left != NULL ? access->left->type : NULL;
  if (access->base.op == AST_OP(arrow)) {
    if (!TypeIsPointer(object_type)) {
      // Not a raw pointer (e.g. dependent, or an overloaded operator->); let
      // the ordinary member-reference path handle or diagnose it.
      return NULL;
    }
    object_type = object_type->next;
  }
  if (object_type == NULL || TypeContainsTemplateParameter(object_type)) {
    // Dependent: defer to instantiation, where the type becomes concrete.
    return NULL;
  }

  if (TypeIsStructOrUnion(object_type) &&
      object_type->info.struct_info != NULL &&
      object_type->info.struct_info->tag_name != NULL) {
    Struct* struct_info = object_type->info.struct_info;
    // If the spelled name already names a destructor reachable from the object
    // -- its own destructor (`p->~Der()`) or an inherited base-subobject
    // destructor (`derived_ptr->~Base()`, as generated for base cleanup) --
    // leave it entirely to the ordinary member-call path.  Only names that do
    // NOT resolve (e.g. an unsubstituted template parameter `~T`) are rewritten.
    CXXAccess spelled_access = kAccessPublic;
    Struct* spelled_owner = NULL;
    int spelled_offset = 0;
    StructMember* spelled_member = FindStructMemberWithAccessAndOffset(
        struct_info, spelled, &spelled_access, &spelled_owner, &spelled_offset);
    if (spelled_member != NULL && spelled_member->is_member_function &&
        spelled_member->symbol != NULL &&
        TypeIsFunction(spelled_member->symbol->type) &&
        spelled_member->symbol->type->info.function.is_destructor) {
      return NULL;
    }
    // A base-subobject cleanup call `this->~A()` generated inside a class
    // template spells the base by its primary name ("~A"); after instantiation
    // the base subobject is `A<int>` with destructor "~A<int>".  If the spelled
    // name names a base by tag or primary-template name, rewrite it to that
    // base's specialized destructor name so the ordinary member-call path binds
    // to the base's destructor -- rather than falling through to the own-type
    // rewrite below, which would bind to the derived class's own destructor and
    // recurse forever.
    String* base_tag = CXXFindBaseTagNameForDestructorSpelling(
        struct_info, spelled->value + 1);
    if (base_tag != NULL) {
      String base_destructor_name;
      StringInit(&base_destructor_name, "~");
      StringAppendString(&base_destructor_name, base_tag);
      if (!StringEqualString(spelled, &base_destructor_name)) {
        ConstantASTNode* name_node = (ConstantASTNode*)access->right;
        StringSet(name_node->value.string, base_destructor_name.value);
      }
      StringDestruct(&base_destructor_name);
      return NULL;
    }
    String destructor_name;
    StringInit(&destructor_name, "~");
    StringAppendString(&destructor_name, struct_info->tag_name);
    StructMember* destructor = FindStructMember(struct_info, &destructor_name);
    if (destructor != NULL && destructor->is_member_function &&
        destructor->symbol != NULL &&
        TypeIsFunction(destructor->symbol->type) &&
        destructor->symbol->type->info.function.is_destructor) {
      // Rewrite the unresolved spelled name (e.g. "~T") to the object type's
      // real destructor name so the ordinary member-call path resolves it.
      if (!StringEqualString(spelled, &destructor_name)) {
        ConstantASTNode* name_node = (ConstantASTNode*)access->right;
        StringSet(name_node->value.string, destructor_name.value);
      }
      StringDestruct(&destructor_name);
      return NULL;
    }
    StringDestruct(&destructor_name);
    // Class with a trivial (unmaterialized) destructor: fall through to no-op.
  }

  // If the call carries arguments (the implicit `__complete_object` flag of a
  // generated base cleanup, or ill-formed user arguments) we must not collapse
  // it into an argument-dropping no-op; hand it to the ordinary member-call path
  // which either binds a materialized destructor or diagnoses the arguments.
  if (has_arguments) {
    return NULL;
  }

  // Scalar pseudo-destructor, or trivial-destructor class: evaluate the object
  // expression for its side effects and yield void.
  ASTNode* object_expr = access->left;
  access->left = NULL;
  if (object_expr != NULL) {
    object_expr->parent = NULL;
    object_expr->child_id = 0;
  }
  ASTNode* discard =
      NewCastASTNode(NewTypeRecordWithSize(kTypeVoid, kQualPlain),
                     node->base.location, object_expr);
  ((CastASTNode*)discard)->kind = kCastStatic;
  return ReplaceVectorWithCall(node, discard);
}


// Explicit destructor call before callee analysis. Returns replacement AST or NULL.
static ASTNode* FunctionCallTryExplicitDestructor(VectorASTNode* node) {
  return TryAnalyzeCXXExplicitDestructorCall(node);
}

// An undeclared function named `name` in `ns`, as the parser invents for a
// call that only ADL can resolve.  It is not added to `ns`, so a later
// declaration does not see it; one symbol per (ns, name) is kept.
static Symbol* AdlOnlyFunctionSymbol(Namespace* ns, String* name) {
  static Vector symbols;
  static bool symbols_ready;
  if (!symbols_ready) {
    VectorInit(&symbols);
    symbols_ready = true;
  }
  for (size_t i = 0; i < symbols.length; i++) {
    Symbol* existing = symbols.value.p[i];
    if (existing->namespace_ == ns && StringEqualString(&existing->name, name)) {
      return existing;
    }
  }
  TypeRecord* result = NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                             kQualPlain);
  TypeRecord* func_type = NewFunctionTypeRecord();
  func_type->info.function.unknown_args = true;
  TypeRecordChain(func_type, result);
  Symbol* function = NewSymbol(name->value, func_type, STO(implicit));
  function->flags.is_forward_declared = true;
  function->namespace_ = ns;
  VectorAppend(&symbols, function);
  return function;
}

// A call to an unqualified name in a class template with a dependent base is
// parsed as `this->name`, since the base may declare it.  When the
// instantiated class has no such member, the name is an ordinary function
// found by unqualified lookup and ADL ([temp.dep]), e.g. gmock's `DoAll`
// declared after `DoAllAction`.  Returns the replacement callee or NULL.
static ASTNode* UnresolvedImplicitMemberCallee(ASTNode* callee) {
  if (!CompilerIsCXX() || callee == NULL || callee->op != AST_OP(arrow) ||
      (callee->flags & kASTUnresolvedImplicitMember) == 0) {
    return NULL;
  }
  BinaryASTNode* access = (BinaryASTNode*)callee;
  if (access->left == NULL || access->left->op != AST_OP(identifier) ||
      access->right == NULL || access->right->op != AST_OP(string)) {
    return NULL;
  }
  Symbol* this_symbol = ((IdentifierASTNode*)access->left)->symbol;
  TypeRecord* this_type = this_symbol != NULL ? this_symbol->type : NULL;
  if (this_type == NULL || !TypeIsStructOrUnionPointer(this_type) ||
      this_type->next->info.struct_info == NULL) {
    return NULL;
  }
  Struct* owner = this_type->next->info.struct_info;
  if (owner->is_template || TypeContainsTemplateParameter(this_type->next)) {
    return NULL;
  }
  String* name = ((ConstantASTNode*)access->right)->value.string;
  CXXAccess member_access = kAccessPublic;
  Struct* member_owner = NULL;
  int member_offset = 0;
  if (name == NULL ||
      FindStructMemberWithAccessAndOffsetByName(owner, name->value,
                                                &member_access, &member_owner,
                                                &member_offset) != NULL) {
    return NULL;
  }
  Struct* outermost = owner;
  while (outermost->lexical_parent != NULL) {
    outermost = outermost->lexical_parent;
  }
  Namespace* ns = outermost->tag_symbol != NULL &&
                          outermost->tag_symbol->namespace_ != NULL
                      ? outermost->tag_symbol->namespace_
                      : compiler->global_namespace;
  Symbol* function = NamespaceLookupSymbolInEnclosingScopes(ns, name);
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type) ||
      function->type->info.function.cxx_member_owner != NULL) {
    function = AdlOnlyFunctionSymbol(ns, name);
  }
  return NewIdentifierASTNode(function, callee->location);
}

// Detaches callee from the call node while analyzing to protect argument child
// indices from parent replacement. Reattaches with child_id 0; may delete the
// old callee subtree. Sets *syntactic_class_construction from callee flags.
static void FunctionCallAnalyzeAndAttachCallee(VectorASTNode* node,
                                               bool* syntactic_class_construction) {
  ASTNode* old_callee = node->left;
  node->left = NULL;
  if (old_callee != NULL) {
    old_callee->parent = NULL;
  }
  ASTNode* free_callee = UnresolvedImplicitMemberCallee(old_callee);
  if (free_callee != NULL) {
    ASTNodeDelete(old_callee);
    old_callee = free_callee;
  }
  *syntactic_class_construction =
      old_callee != NULL &&
      (old_callee->flags & kASTCXXFunctionalConstruction) != 0;
  ASTNode* analyzed_callee =
      *syntactic_class_construction ? old_callee : AnalyzeExpression(old_callee);
  if (analyzed_callee != old_callee) {
    if (old_callee != NULL && analyzed_callee != NULL) {
      analyzed_callee->flags |=
          old_callee->flags & kASTReferencesParameterPack;
    }
    ASTNodeDelete(old_callee);
  }
  node->left = analyzed_callee;
  if (analyzed_callee != NULL) {
    analyzed_callee->parent = &node->base;
    analyzed_callee->child_id = 0;
  }
}

// Dependent qualified name callee: flags call and assigns placeholder type.
// Returns true when the caller must return `(ASTNode*)node` immediately.
static bool FunctionCallDeferDependentQualifiedCallee(VectorASTNode* node) {
  ASTNode* analyzed_callee = node->left;
  if (analyzed_callee != NULL &&
      (analyzed_callee->flags & kASTDependentQualifiedName) != 0 &&
      analyzed_callee->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)analyzed_callee;
    if (id->symbol != NULL && id->symbol->type != NULL &&
        TypeDependentOf(id->symbol->type)->dependent_member_name != NULL &&
        TypeContainsTemplateParameter(id->symbol->type)) {
      node->base.flags |= kASTDependentFunctorCall;
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
      return true;
    }
  }
  return false;
}

// Member template with dependent explicit template args — defer before actuals.
static bool FunctionCallDeferMemberTemplateExplicitArgs(VectorASTNode* node) {
  if (IsMemberTemplateCallWithDependentExplicitArgs(node)) {
    node->base.flags |= kASTDependentFunctorCall;
    SetDependentMemberTemplateCallType(node);
    return true;
  }
  return false;
}

// Meta-synthesis calls that must run before actuals are analyzed. Sets constexpr flag
// on the replacement subtree; returns NULL to continue ordinary call analysis.
static ASTNode* FunctionCallTryMetaSynthesisEarly(VectorASTNode* node) {
  ASTNode* early_meta_synthesis =
      SemanticTryAnalyzeMetaSynthesisCallEarly(node);
  if (early_meta_synthesis != NULL) {
    early_meta_synthesis->flags |= kASTRequiresASTConstexpr;
  }
  return early_meta_synthesis;
}

// Mutates argument child pointers in place; ordering must follow callee analysis.
// Clears deferred calls nested in a braced argument once the enclosing
// function specialization is concrete, so target-typed brace conversion sees
// `std::move(local)` and similar expressions with their real result types.
static void ClearConcreteBracedDependentCall(ASTNode* node, void* data,
                                             int child_id,
                                             VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(call) ||
      (node->flags & kASTDependentFunctorCall) == 0) {
    return;
  }
  node->flags &= ~(kASTDependentFunctorCall | kASTAnalyzed);
  ASTNodeClearType(node);
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left != NULL) {
    call->left->flags &= ~kASTAnalyzed;
  }
}

static void FunctionCallAnalyzeActualArguments(VectorASTNode* node) {
  size_t num_actual_args = node->children->length;
  for (size_t i = 0; i < num_actual_args; i++) {
    ASTNode* actual = node->children->value.p[i];
    if (actual != NULL && actual->op == AST_OP(braced_init) &&
        compiler->current_function != NULL &&
        !TypeContainsTemplateParameter(compiler->current_function)) {
      actual->flags &= ~kASTAnalyzed;
      ASTNodeVisit(actual, ClearConcreteBracedDependentCall, 0, NULL);
    }
    node->children->value.p[i] =
        AnalyzeExpression(actual);
  }
}

// Meta/reflection call forms that require analyzed actuals. First match wins.
static ASTNode* FunctionCallTryMetaCallsAfterActuals(VectorASTNode* node) {
  ASTNode* meta_call = SemanticTryAnalyzeMetaCall(node);
  if (meta_call != NULL) {
    meta_call->flags |= kASTRequiresASTConstexpr;
    return meta_call;
  }
  ASTNode* meta_trait_call = SemanticTryAnalyzeMetaTraitCall(node);
  if (meta_trait_call != NULL) {
    meta_trait_call->flags |= kASTRequiresASTConstexpr;
    return meta_trait_call;
  }
  ASTNode* meta_synthesis_call = SemanticTryAnalyzeMetaSynthesisCall(node);
  if (meta_synthesis_call != NULL) {
    meta_synthesis_call->flags |= kASTRequiresASTConstexpr;
    return meta_synthesis_call;
  }
  return NULL;
}

// Defers when an argument is itself a dependent functor call; mutates call flags/type.
static bool FunctionCallDeferDependentFunctorActuals(VectorASTNode* node) {
  if (CallActualsContainDependentFunctorCall(node)) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    return true;
  }
  return false;
}

// Routes marked syntactic `T(...)` functional casts before overload resolution.
static ASTNode* FunctionCallTrySyntacticConstruction(VectorASTNode* node,
                                                     bool syntactic_class_construction) {
  if (!syntactic_class_construction) {
    return NULL;
  }
  return AnalyzeCXXFunctionalClassConstruction(node);
}

// Defers member-function-template calls whose overload set is still dependent.
static bool FunctionCallDeferDependentMemberTemplate(VectorASTNode* node,
                                                     bool has_pack_expansion_actual) {
  if (!has_pack_expansion_actual && IsDependentMemberTemplateCall(node)) {
    node->base.flags |= kASTDependentFunctorCall;
    SetDependentMemberTemplateCallType(node);
    return true;
  }
  return false;
}

// Defers when an actual still carries undeduced `auto` from a surrounding declaration.
static bool FunctionCallDeferUndeducedAutoActuals(VectorASTNode* node) {
  if (CallActualsContainUndeducedAuto(node)) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    return true;
  }
  return false;
}

// Early function-template deduction on a single identifier callee. Returns a
// finished call node when deduction fails or CopyConstruct arity mismatch; NULL
// to continue resolution.
static ASTNode* FunctionCallTryDeduceIdentifierFunctionTemplate(
    VectorASTNode* node, bool has_pack_expansion_actual) {
  if (node->left == NULL || node->left->op != AST_OP(identifier)) {
    return NULL;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  bool static_member_has_overloads = false;
  if (id->symbol != NULL && id->symbol->overload_next == NULL &&
      id->symbol->type != NULL && TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.cxx_member_owner != NULL) {
    StructMember* head = FindStructMember(
        id->symbol->type->info.function.cxx_member_owner, &id->symbol->name);
    static_member_has_overloads =
        head != NULL && head->overload_next != NULL;
  }
  if (id->symbol != NULL && id->symbol->flags.is_template &&
      !id->symbol->flags.is_overloaded &&
      id->symbol->overload_next == NULL &&
      !static_member_has_overloads &&
      TypeIsFunction(id->symbol->type) &&
      !has_pack_expansion_actual &&
      !TemplateArgumentVectorContainsTemplateParameter(id->template_arguments) &&
      !CallActualsContainTemplateParameter(node)) {
    Symbol* instantiated =
        TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
            &compiler->syntax, id->symbol, id->template_arguments,
            node->children);
    if (instantiated != NULL && instantiated != id->symbol) {
      id->symbol = instantiated;
      ASTNodeSetInstantiatedCalleeType(node->left, instantiated->type);
    }
    if (id->symbol->flags.is_template &&
        !TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
            id->symbol, id->template_arguments, node->children, 0)) {
      if (StringEqual(&id->symbol->name, "CopyConstruct") &&
          node->children != NULL && node->children->length != 2) {
        ASTNodeSetType((ASTNode*)node,
                       NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                             kQualPlain));
        return (ASTNode*)node;
      }
      if (TemplateArgumentVectorContainsErroneous(id->template_arguments)) {
        // An explicit argument that failed to parse was diagnosed already.
        ASTNodeSetType(
            (ASTNode*)node,
            NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
        return (ASTNode*)node;
      }
      if (ConceptsFunctionTemplateHasAssociatedConstraint(id->symbol)) {
        ReportUnsatisfiedFunctionTemplateConstraints(node, id->symbol,
                                                     id->template_arguments,
                                                     0);
      } else {
        SemanticError((ASTNode*)node, "Template argument deduction failed");
      }
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
      return (ASTNode*)node;
    }
  }
  return NULL;
}

// Defers overload resolution when actual types are dependent but the callee is an overload set.
static bool FunctionCallDeferTypeDependentOverloadedActuals(VectorASTNode* node) {
  if (node->left == NULL || node->left->op != AST_OP(identifier) ||
      !CallActualsContainTemplateParameter(node)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  bool actuals_type_dependent = false;
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* actual = node->children->value.p[i];
    if (actual != NULL && ((actual->flags & kASTLambdaExpression) != 0 ||
                           ActualIsFunctionTemplateName(actual))) {
      continue;
    }
    if (actual != NULL && actual->type != NULL &&
        (TypeContainsTemplateParameter(actual->type) ||
         TypeContainsAuto(actual->type) ||
         TypeIsUnknown(actual->type))) {
      actuals_type_dependent = true;
      break;
    }
  }
  if (actuals_type_dependent && id->symbol != NULL &&
      (id->symbol->flags.is_overloaded ||
       id->symbol->flags.is_template ||
       id->symbol->overload_next != NULL)) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                         kQualPlain));
    return true;
  }
  return false;
}

// First overload-resolution pass and member-call lowering before dependent
// callee typing. *lowered_member_call reports whether member lowering ran.
static void FunctionCallResolveAndLowerMemberAccess(VectorASTNode* node,
                                                      bool* lowered_member_call) {
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    ResolveOverloadedFunctionCall(node);
  }
  *lowered_member_call = LowerMemberFunctionCall(node);
  if (!*lowered_member_call) {
    *lowered_member_call = LowerMemberPointerFunctionCall(node);
  }
}

// Defers member calls whose receiver type is still template- or auto-dependent.
static bool FunctionCallDeferDependentMemberReceiver(VectorASTNode* node,
                                                     bool lowered_member_call) {
  if (!lowered_member_call && node->left != NULL &&
      (node->left->op == AST_OP(dot) || node->left->op == AST_OP(arrow))) {
    BinaryASTNode* member_access = (BinaryASTNode*)node->left;
    TypeRecord* receiver_type =
        member_access->left != NULL ? member_access->left->type : NULL;
    if (receiver_type != NULL && node->left->op == AST_OP(arrow) &&
        TypeIsPointerOrArray(receiver_type)) {
      receiver_type = receiver_type->next;
    }
    if (receiver_type != NULL &&
        (TypeContainsTemplateParameter(receiver_type) ||
         TypeContainsAuto(receiver_type))) {
      node->base.flags |= kASTDependentFunctorCall;
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                           kQualPlain));
      return true;
    }
  }
  return false;
}

// True when the callee names a class/alias template with concrete args and actuals (CTAD path).
static bool FunctionCallIsConcreteCtadConstruction(VectorASTNode* node,
                                                     bool has_pack_expansion_actual) {
  if (node->left == NULL || node->left->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* callee_id = (IdentifierASTNode*)node->left;
  if (callee_id->symbol != NULL && callee_id->symbol->flags.is_template &&
      callee_id->symbol->type != NULL &&
      (TypeIsStructOrUnion(callee_id->symbol->type) ||
       StorageIs(callee_id->symbol->storage, STO(typedef))) &&
      !has_pack_expansion_actual &&
      !CallActualsContainTemplateParameter(node) &&
      !TemplateArgumentVectorContainsTemplateParameter(
          callee_id->template_arguments)) {
    return true;
  }
  return false;
}

// True when an overload set must be resolved now despite template parameters in candidates.
static bool FunctionCallHasConcreteOverloadSet(VectorASTNode* node) {
  if (node->left == NULL || node->left->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  if (id->symbol != NULL &&
      (id->symbol->flags.is_overloaded ||
       id->symbol->overload_next != NULL) &&
      !TemplateArgumentVectorContainsTemplateParameter(
          id->template_arguments) &&
      !CallActualsContainDependentFunctorCall(node)) {
    bool actuals_type_dependent = false;
    for (size_t i = 0; i < node->children->length; i++) {
      ASTNode* actual = node->children->value.p[i];
      if (actual != NULL && actual->type != NULL &&
          (TypeContainsTemplateParameter(actual->type) ||
           TypeIsUnknown(actual->type))) {
        actuals_type_dependent = true;
        break;
      }
    }
    if (!actuals_type_dependent) {
      return true;
    }
  }
  return false;
}

// Dependent template callee typing (declval, functors). Returns true when the
// call node is fully typed/deferred and the caller must return it.
static bool FunctionCallTryAssignDependentTemplateCalleeType(
    VectorASTNode* node, bool is_concrete_ctad_construction,
    bool concrete_overload_set) {
  if (!CompilerIsCXX() || node->left == NULL || node->left->type == NULL ||
      is_concrete_ctad_construction || concrete_overload_set ||
      (!TypeContainsTemplateParameter(node->left->type) &&
       !TypeIsUnknown(node->left->type))) {
    return false;
  }
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    TypeRecord* return_type = TypeRecordAdopt(TypeSubstituteFunctionTemplateReturnType(
        &compiler->syntax, id->symbol, id->template_arguments));
    if (return_type != NULL) {
      bool result_is_concrete =
          !TypeContainsTemplateParameter(return_type) &&
          !TypeIsUnknown(return_type) &&
          !TemplateArgumentVectorContainsTemplateParameter(
              id->template_arguments) &&
          !CallActualsContainTemplateParameter(node) &&
          !CallActualsContainDependentFunctorCall(node);
      if (!result_is_concrete) {
        node->base.flags |= kASTDependentFunctorCall;
      }
      if (TypeIsReference(return_type)) {
        ASTNodeSetType((ASTNode*)node, return_type->next);
        node->base.value_category =
            return_type->declarator == kDeclRValueReference
                ? kValueCategoryXvalue
                : kValueCategoryLvalue;
      } else {
        ASTNodeSetType((ASTNode*)node, return_type);
      }
      if (result_is_concrete) {
        CheckDeletedFunctionUse(id->symbol, (ASTNode*)node);
      }
      TypeRecordDelete(return_type);
      return true;
    }
    if (id->symbol != NULL && !id->symbol->flags.is_overloaded &&
        id->symbol->overload_next == NULL) {
      CheckDeletedFunctionUse(id->symbol, (ASTNode*)node);
    }
  }
  node->base.flags |= kASTDependentFunctorCall;
  ASTNodeSetType((ASTNode*)node,
                 NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
  return true;
}

// Second resolution pass after CTAD/operator() paths. Returns finished node when
// template callee must stay dependent; NULL to continue toward arity checking.
static ASTNode* FunctionCallResolveCalleeAndTemplates(VectorASTNode* node,
                                                      bool has_pack_expansion_actual) {
  LowerMemberFunctionCall(node);
  ResolveOverloadedFunctionCall(node);
  InstantiateResolvedFunctionTemplateCall(node);
  if (node->left == NULL || node->left->op != AST_OP(identifier)) {
    return NULL;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  CheckDeletedFunctionUse(id->symbol, (ASTNode*)node);
  if (id->symbol == NULL || !id->symbol->flags.is_template ||
      has_pack_expansion_actual) {
    return NULL;
  }
  bool dependent_signature = false;
  if (TypeIsFunction(id->symbol->type)) {
    Vector* formals = &id->symbol->type->info.function.prototype;
    for (size_t i = 0; i < formals->length; i++) {
      Symbol* formal = formals->value.p[i];
      if (formal != NULL && TypeHasDependentMemberName(formal->type)) {
        dependent_signature = true;
        break;
      }
    }
    if (id->symbol->type->next != NULL &&
        TypeHasDependentMemberName(id->symbol->type->next)) {
      dependent_signature = true;
    }
  }
  if (TypeIsFunction(id->symbol->type) &&
      (TemplateArgumentVectorContainsTemplateParameter(
           id->template_arguments) ||
       dependent_signature)) {
    node->base.flags |= kASTDependentFunctorCall;
    TypeRecord* return_type = TypeRecordAdopt(TypeSubstituteFunctionTemplateReturnType(
        &compiler->syntax, id->symbol, id->template_arguments));
    if (return_type == NULL) {
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                           kQualPlain));
    } else if (TypeIsReference(return_type)) {
      ASTNodeSetType((ASTNode*)node, return_type->next);
      node->base.value_category =
          return_type->declarator == kDeclRValueReference
              ? kValueCategoryXvalue
              : kValueCategoryLvalue;
    } else {
      ASTNodeSetType((ASTNode*)node, return_type);
    }
    TypeRecordDelete(return_type);
    return (ASTNode*)node;
  }
  if (TemplateArgumentVectorContainsErroneous(id->template_arguments)) {
    // An explicit argument that failed to parse was diagnosed already.
    return NULL;
  }
  if (TypeIsFunction(id->symbol->type) &&
      !TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
          id->symbol, id->template_arguments, node->children, 0)) {
    SemanticError((ASTNode*)node, "Template argument deduction failed");
  } else {
    SemanticError((ASTNode*)node,
                  "Template instantiation is not supported yet");
  }
  return NULL;
}

// Callable check, null callee, and not-ready template actual deferral.
static ASTNode* FunctionCallValidateCallableAndArguments(VectorASTNode* node) {
  if (node->left != NULL && !TypeIsFunctionPointer(node->left->type)) {
    SemanticError(node->left, "Cannot call a non-function");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return &node->base;
  }
  if (node->left == NULL) {
    return &node->base;
  }
  if (CompilerIsCXX() && node->children != NULL) {
    bool argument_not_ready = false;
    for (size_t i = 0; i < node->children->length; i++) {
      ASTNode* actual = node->children->value.p[i];
      if (actual != NULL && (actual->flags & kASTLambdaExpression) != 0) {
        continue;
      }
      if (actual != NULL && actual->type != NULL &&
          (TypeIsUnknown(actual->type) ||
           TypeContainsTemplateParameter(actual->type) ||
           TypeContainsAuto(actual->type))) {
        if (ActualIsFunctionTemplateName(actual)) {
          continue;
        }
        argument_not_ready = true;
        break;
      }
    }
    if (argument_not_ready) {
      node->base.flags |= kASTDependentFunctorCall;
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                           kQualPlain));
      return (ASTNode*)node;
    }
  }
  return NULL;
}

// Clears a stale dependent-functor flag once the call is fully concrete and resolved.
static void FunctionCallClearStaleDependentFlag(VectorASTNode* node) {
  node->base.flags &= ~kASTDependentFunctorCall;
}

// Materializes member template bodies and records immediate-function escalation candidates.
static void FunctionCallEnsureTemplateDefinitionsAndEscalation(VectorASTNode* node) {
  if (!CompilerIsCXX()) {
    return;
  }
  if (node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, id->symbol);
    CXXAnalyzeImmediateEscalationCandidate(id->symbol);
  } else if (node->left->op == AST_OP(dot) ||
             node->left->op == AST_OP(arrow)) {
    BinaryASTNode* access = (BinaryASTNode*)node->left;
    if (access->right != NULL &&
        access->right->op == AST_OP(structmember)) {
      StructMemberASTNode* member =
          (StructMemberASTNode*)access->right;
      if (member->member != NULL && member->member->symbol != NULL) {
        TypeEnsureTemplateMemberFunctionDefinition(
            &compiler->syntax, member->member->symbol);
        CXXAnalyzeImmediateEscalationCandidate(member->member->symbol);
      }
    }
  }
}

// Sets call result type and value category from the resolved function type.
static TypeRecord* FunctionCallAssignReturnTypeFromCallee(VectorASTNode* node) {
  TypeRecord* subtype = node->left->type;
  if (TypeIsPointer(node->left->type)) {
    subtype = subtype->next;
  }
  SemanticEnsureAutoReturnTypeDeduced(subtype);
  TypeMaterializePendingMemberReturnType(&compiler->syntax, subtype);
  TypeRecord* return_type = subtype->next;
  if (TypeIsReference(return_type)) {
    ASTNodeSetType((ASTNode*)node, return_type->next);
    node->base.value_category =
        return_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)node, return_type);
  }
  return subtype;
}

// Appends default arguments when allowed and diagnoses fixed-arity mismatch.
// Updates *num_actual_args when defaults are inserted; does not convert actuals.
static bool FunctionCallValidateFixedArity(VectorASTNode* node,
                                           TypeRecord* subtype,
                                           size_t* num_actual_args,
                                           bool has_pack_expansion_actual) {
  if (subtype->info.function.unknown_args || has_pack_expansion_actual) {
    return true;
  }
  bool call_ok = true;
  size_t num_formal_args = subtype->info.function.prototype.length;
  if (AppendDefaultCallArguments(node, subtype)) {
    *num_actual_args = node->children->length;
  }
  bool already_diagnosed = (node->base.flags & kASTOverloadDiagnosed) != 0;
  if (subtype->info.function.varargs) {
    if (*num_actual_args < num_formal_args) {
      if (!already_diagnosed) {
        SemanticError((ASTNode*)node,
                      "Too few arguments supplied to varargs function call; "
                      "need %zd, got %zd",
                      num_formal_args, *num_actual_args);
      }
      call_ok = false;
    }
  } else if (*num_actual_args != num_formal_args) {
    if (!already_diagnosed) {
      SemanticError((ASTNode*)node,
                    "Incorrect number of arguments supplied to function call; "
                    "need %zd, got %zd",
                    num_formal_args, *num_actual_args);
    }
    call_ok = false;
  }
  return call_ok;
}

// Converts one call actual to its formal, including reference binding, braced-init
// lowering, lambda-to-function-pointer, and polymorphic ctor/dtor `this` rules.
static void FunctionCallConvertOneFixedArgument(VectorASTNode* node,
                                                TypeRecord* subtype,
                                                size_t index) {
  ASTNode* actual = (ASTNode*)node->children->value.p[index];
  Symbol* formal =
      (Symbol*)subtype->info.function.prototype.value.p[index];
  ASTNode* converted_initializer_list =
      ConvertCXXInitializerListArgument(actual, formal->type);
  if (converted_initializer_list != actual) {
    ASTNodeReplaceChild((ASTNode*)node, (int)index, converted_initializer_list,
                        true);
    actual = converted_initializer_list;
  }
  if (actual != NULL && actual->op == AST_OP(braced_init)) {
    TypeRecord* braced_target = TypeIsReference(formal->type)
                                    ? formal->type->next
                                    : formal->type;
    ASTNode* lowered = LowerCXXBracedInitToTarget(actual, braced_target);
    if (lowered != actual) {
      ASTNodeReplaceChild((ASTNode*)node, (int)index, lowered, false);
      actual = lowered;
      if (TypeIsReference(formal->type) &&
          actual->op == AST_OP(compound_literal)) {
        actual->value_category = kValueCategoryXvalue;
      }
    }
  }
  if (actual != NULL && actual->op == AST_OP(compound_literal) &&
      TypeIsReference(formal->type) &&
      formal->type->declarator == kDeclRValueReference) {
    actual->value_category = kValueCategoryXvalue;
  }
  bool polymorphic_special_this =
      index == 0 &&
      (subtype->info.function.is_constructor ||
       subtype->info.function.is_destructor) &&
      subtype->info.function.cxx_member_owner != NULL &&
      subtype->info.function.cxx_member_owner->virtual_members.length > 0;
  if (TypeIsReference(formal->type)) {
    TypeRecord* reference_type = formal->type;
    if (actual->op == AST_OP(identifier)) {
      Symbol* named = ((IdentifierASTNode*)actual)->symbol;
      if (named != NULL && named->flags.value_set &&
          !named->flags.is_local && !named->flags.is_defined) {
        actual = FoldConstantArgument(actual);
      }
    }
    bool discards_qualifiers =
        TypeIsEffectivelyConst(actual->type) &&
        !TypeIsEffectivelyConst(reference_type->next);
    if (!polymorphic_special_this &&
        !TypeEqualIgnoringQualifiers(actual->type, reference_type->next)) {
      ASTNode* base_bound =
          TryBindReferenceToBaseSubobject(actual, reference_type->next);
      if (base_bound != NULL) {
        ASTNodeReplaceChild((ASTNode*)node, (int)index, base_bound, false);
        actual = base_bound;
      } else {
        if (TypeIsScalar(actual->type)) {
          actual->flags &= ~kASTNeedAddress;
        }
        NormalConversion(actual,
                         ReferenceConversionTarget(actual, reference_type));
        actual = node->children->value.p[index];
      }
    }
    if (discards_qualifiers) {
      SemanticError(actual, "Reference argument discards qualifiers");
    } else if (!ReferenceCanBind(actual, reference_type)) {
      if (reference_type->declarator == kDeclRValueReference) {
        SemanticError(actual,
                      "Rvalue reference argument must not be an lvalue");
      } else if (TypeIsConst(reference_type->next)) {
        SemanticError(actual, "Const reference argument has incompatible type");
      } else {
        SemanticError(actual, "Reference argument must be an lvalue");
      }
    }
    bool materialize_scalar_lvalue_reference =
        reference_type->declarator == kDeclReference &&
        actual->value_category == kValueCategoryPrvalue &&
        !TypeIsStructOrUnion(actual->type);
    if (ReferenceCanBind(actual, reference_type) && !HasAddress(actual) &&
        (TypeIsStructOrUnion(actual->type) ||
         materialize_scalar_lvalue_reference)) {
      ASTNode* materialized =
          MaterializeTemporary(actual, reference_type->next);
      ASTNodeReplaceChild((ASTNode*)node, (int)index, materialized, false);
      actual = materialized;
    }
    SetNeedAddress(actual);
  } else {
    actual = FoldConstantArgument(actual);
    if (!polymorphic_special_this && TypeIsPointer(formal->type) &&
        formal->type->next != NULL &&
        TypeIsFunction(formal->type->next)) {
      ASTNode* lambda_expr = CXXLambdaConversionExpression(actual);
      if (lambda_expr != NULL &&
          (lambda_expr->flags & kASTLambdaExpression) != 0 &&
          LambdaCallMatchesFunctionPointer(lambda_expr, formal->type)) {
        ASTNode* converted = LambdaToFunctionPointer(
            lambda_expr, formal->type->next);
        if (converted != NULL) {
          ASTNodeReplaceChild((ASTNode*)node, (int)index, converted, true);
          actual = converted;
        }
      }
    }
    ASTNode* materialized =
        actual->value_category != kValueCategoryPrvalue
            ? MaterializeCXXByValueClassArgument(actual, formal->type)
            : SemanticMaterializeClassPrvalue(actual);
    if (materialized != actual) {
      ASTNodeReplaceChild((ASTNode*)node, (int)index, materialized, false);
      actual = materialized;
    }
    if (TypeIsStructOrUnion(formal->type) &&
        TypeIsStructOrUnion(actual->type)) {
      actual->flags |= kASTFunctionParameterTemporary;
    }
    if (!polymorphic_special_this) {
      NormalConversion(actual, formal->type);
    }
  }

  if (TypeIsStructOrUnion(actual->type) || TypeIsArray(actual->type) ||
      TypeIsFunction(actual->type)) {
    actual->flags |= kASTNeedAddress;
  }
}

// Per-formal argument conversion loop; mutates argument children. Returns whether
// arity and conversions succeeded (*num_actual_args updated for defaults).
static bool FunctionCallConvertFixedArguments(VectorASTNode* node,
                                              TypeRecord* subtype,
                                              size_t* num_actual_args,
                                              bool has_pack_expansion_actual) {
  bool call_ok = FunctionCallValidateFixedArity(
      node, subtype, num_actual_args, has_pack_expansion_actual);
  if (subtype->info.function.unknown_args || has_pack_expansion_actual) {
    return call_ok;
  }
  size_t num_formal_args = subtype->info.function.prototype.length;
  for (size_t i = 0; i < num_formal_args && i < *num_actual_args; i++) {
    FunctionCallConvertOneFixedArgument(node, subtype, i);
  }
  return call_ok;
}

static void FunctionCallPromoteVarargsActuals(TypeRecord* subtype,
                                              VectorASTNode* node,
                                              size_t num_formal_args,
                                              size_t num_actual_args) {
  bool unknown = subtype->info.function.unknown_args;
  bool varargs = subtype->info.function.varargs;
  if (unknown || varargs) {
    size_t start = unknown ? 0 : num_formal_args;
    for (size_t i = start; i < num_actual_args; i++) {
      ASTNode* actual =
          FoldConstantArgument((ASTNode*)node->children->value.p[i]);
      if (TypeIsFloat(actual->type)) {
        NormalConversion(actual, NewTypeRecordWithSize(kTypeDouble, kQualPlain));
      } else if (TypeIsIntegral(actual->type) && !TypeIsBitInt(actual->type) &&
                 actual->type->size < compiler->int_size) {
        // The callee reads an int, but a narrower value's register is not
        // extended past its own width.  The cast context admits scoped enums,
        // which are passed as their underlying type.
        SemanticConvertType(actual, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            kConvertCast);
      }
    }
  }
}

// An immediate function is never emitted, so a class or array result is
// replaced by a temporary initialized from its constant image.
static ASTNode* MaterializeConstevalObjectResult(VectorASTNode* node,
                                                 TypeRecord* return_type,
                                                 bool* ok) {
  SourceLocation location = node->base.location;
  ASTNode* parent = node->base.parent;
  TypeRecord* temporary_type = TypeRecordCopy(return_type);
  SetQualifiers(temporary_type, kQualPlain);
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, temporary_type);
  temp->location = location;
  ASTNode* init = NewExpressionInitializerASTNode((ASTNode*)node, location);
  *ok = ConstexprEvaluateObjectConstantForSymbol(temp, init);
  ASTNode* image =
      *ok ? ConstexprObjectInitializerForSymbol(temp, location) : NULL;
  node->base.parent = parent;
  if (image == NULL) {
    if (*ok) {
      SemanticError((ASTNode*)node,
                    "consteval function call result cannot be emitted as a "
                    "constant object");
    }
    return NULL;
  }
  ASTNode* temp_id = NewIdentifierASTNode(temp, location);
  temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* replacement =
      AnalyzeExpression(NewCompoundLiteralASTNode(temp_id, location, image));
  replacement->value_category = kValueCategoryPrvalue;
  replacement->parent = parent;
  return replacement;
}

// Likewise a consteval constructor call (`obj.T(&obj, args)`) becomes an
// initialization of `obj` from the constant it constructs.
static ASTNode* MaterializeConstevalConstructorCall(VectorASTNode* node,
                                                    bool* ok) {
  ASTNode* receiver = node->children != NULL && node->children->length > 0
                          ? node->children->value.p[0]
                          : NULL;
  if (receiver != NULL && receiver->op == AST_OP(address)) {
    receiver = ((UnaryASTNode*)receiver)->sub;
  }
  Symbol* object = receiver != NULL && receiver->op == AST_OP(identifier)
                       ? ((IdentifierASTNode*)receiver)->symbol
                       : NULL;
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  if (object == NULL || object->type == NULL) {
    *ok = ConstexprEvaluateCall(&ctx, (ASTNode*)node);
    ConstEvalContextDestruct(&ctx);
    return NULL;
  }
  bool saved_value_set = object->flags.value_set;
  void* saved_value = object->value.other;
  SourceLocation location = node->base.location;
  *ok = ConstexprEvaluateConstructorCallForSymbol(&ctx, (ASTNode*)node, object);
  ASTNode* image =
      *ok ? ConstexprObjectInitializerForSymbol(object, location) : NULL;
  object->flags.value_set = saved_value_set;
  object->value.other = saved_value;
  ConstEvalContextDestruct(&ctx);
  if (image == NULL) {
    if (*ok) {
      SemanticError((ASTNode*)node,
                    "consteval constructor result cannot be emitted as a "
                    "constant object");
    }
    return NULL;
  }
  ASTNode* destination = NewIdentifierASTNode(object, location);
  destination->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* replacement = NewBinaryASTNode(
      AST_OP(init), TypeRecordCopy(object->type), location, destination,
      AnalyzeInitializer(object->type, image, true));
  replacement->parent = node->base.parent;
  return replacement;
}

// Diagnoses `consteval` calls that cannot be constant-evaluated when the call
// occurs outside an immediate function context (P2564). Uses the resolved
// identifier callee type when present; may escalate the enclosing function
// instead of diagnosing when escalation applies.  Returns the constant that
// replaces a class or array result, since the immediate function itself is
// never emitted.
static ASTNode* FunctionCallCheckConstexprImmediate(VectorASTNode* node,
                                                    TypeRecord* subtype,
                                                    TypeRecord* return_type,
                                                    bool call_ok) {
  ASTNode* replacement = NULL;
  TypeRecord* immediate_function_type = subtype;
  if (node->left->op == AST_OP(identifier)) {
    Symbol* resolved = ((IdentifierASTNode*)node->left)->symbol;
    if (resolved != NULL && resolved->type != NULL &&
        TypeIsFunction(resolved->type)) {
      immediate_function_type = resolved->type;
    }
  }
  if (call_ok && immediate_function_type->info.function.is_consteval &&
      !CXXInImmediateFunctionContext()) {
    bool ok = false;
    if (TypeIsFloatingPoint(return_type)) {
      double value;
      ok = EvaluateFloatingPointExpression((ASTNode*)node, &value);
    } else if (TypeIsIntegral(return_type)) {
      int64_t value;
      ok = EvaluateIntegerExpression((ASTNode*)node, &value);
    } else if (TypeIsFixedArray(return_type) || TypeIsStructOrUnion(return_type)) {
      replacement = MaterializeConstevalObjectResult(node, return_type, &ok);
    } else if (TypeIsVoid(return_type) &&
               immediate_function_type->info.function.is_constructor) {
      replacement = MaterializeConstevalConstructorCall(node, &ok);
    } else if (TypeIsVoid(return_type)) {
      ConstEvalContext ctx;
      ConstEvalContextInit(&ctx);
      ok = ConstexprEvaluateCall(&ctx, (ASTNode*)node);
      ConstEvalContextDestruct(&ctx);
    }
    if (!ok) {
      if (!CXXEscalateCurrentFunction()) {
        SemanticError((ASTNode*)node,
                      "consteval function call is not a constant expression");
      }
    }
  }
  return replacement;
}

// Validates `format` attributes and specializes printf-family calls when arity checks passed.
static void FunctionCallCheckFormatAndPrintf(VectorASTNode* node, bool call_ok) {
  if (call_ok && node->left->op == AST_OP(identifier)) {
    Symbol* callee = ((IdentifierASTNode*)node->left)->symbol;
    if (callee != NULL) {
      CheckFormatCall(node, callee);
      SpecializePrintfCall(node, callee);
    }
  }
}

// Caller contracts and inlining run last; may replace the call in its parent.
static ASTNode* FunctionCallTryLowerContractsOrInline(VectorASTNode* node,
                                                      TypeRecord* subtype,
                                                      TypeRecord* return_type,
                                                      bool call_ok) {
  if (call_ok && node->caller_contract_function != NULL &&
      compiler->constant_evaluation_required_depth == 0) {
    return LowerVirtualCallerContracts(node);
  }
  if (call_ok && TypeIsFunction(node->left->type) &&
      CalleeNamesItsFunction(node->left)) {
    FunctionInfo* func =
        FunctionInfoForCallInlining(&node->left->type->info.function);
    bool has_initializer_list_actual = false;
    if (node->children != NULL) {
      for (size_t i = 0; i < node->children->length; i++) {
        ASTNode* actual = node->children->value.p[i];
        if (actual == NULL) {
          continue;
        }
        if (actual->op == AST_OP(braced_init) ||
            (actual->flags & kASTCXXBracedTemporary) != 0 ||
            TypeIsCXXInitializerList(actual->type) ||
            (actual->type != NULL && TypeIsReference(actual->type) &&
             TypeIsCXXInitializerList(actual->type->next))) {
          has_initializer_list_actual = true;
          break;
        }
      }
    }
    if (!TypeIsStructOrUnion(return_type) && !has_initializer_list_actual &&
        !CXXMemberInitializerMayElidePrvalue(node) &&
        !CXXConstructorCallElidesPrvalueSource(node) &&
        FunctionCanBeInlined(func)) {
      ASTNode* inline_call = InlineFunctionCall(func, node);
      ASTNodeReplaceChild(node->base.parent,
                          node->base.child_id, inline_call, true);
      return inline_call;
    }
  }
  return NULL;
}

// Top-level call semantic analysis dispatcher; preserves legacy phase ordering.
ASTNode* AnalyzeFunctionCall(VectorASTNode* node) {
  ASTNode* finished = FunctionCallTryExplicitDestructor(node);
  if (finished != NULL) {
    return finished;
  }

  bool syntactic_class_construction = false;
  FunctionCallAnalyzeAndAttachCallee(node, &syntactic_class_construction);

  if (FunctionCallDeferDependentQualifiedCallee(node)) {
    return (ASTNode*)node;
  }

  finished = FunctionCallTryExplicitDestructor(node);
  if (finished != NULL) {
    return finished;
  }

  if (FunctionCallDeferMemberTemplateExplicitArgs(node)) {
    return (ASTNode*)node;
  }

  finished = FunctionCallTryMetaSynthesisEarly(node);
  if (finished != NULL) {
    return finished;
  }

  FunctionCallAnalyzeActualArguments(node);

  finished = FunctionCallTryMetaCallsAfterActuals(node);
  if (finished != NULL) {
    return finished;
  }

  bool has_pack_expansion_actual = CallHasPackExpansionActual(node);

  if (FunctionCallDeferDependentFunctorActuals(node)) {
    return (ASTNode*)node;
  }

  finished = FunctionCallTrySyntacticConstruction(node,
                                                  syntactic_class_construction);
  if (finished != NULL) {
    return finished;
  }

  if (FunctionCallDeferDependentMemberTemplate(node, has_pack_expansion_actual)) {
    return (ASTNode*)node;
  }

  if (FunctionCallDeferUndeducedAutoActuals(node)) {
    return (ASTNode*)node;
  }

  finished = FunctionCallTryDeduceIdentifierFunctionTemplate(
      node, has_pack_expansion_actual);
  if (finished != NULL) {
    return finished;
  }

  if (FunctionCallDeferTypeDependentOverloadedActuals(node)) {
    return (ASTNode*)node;
  }

  bool lowered_member_call = false;
  FunctionCallResolveAndLowerMemberAccess(node, &lowered_member_call);

  if (FunctionCallDeferDependentMemberReceiver(node, lowered_member_call)) {
    return (ASTNode*)node;
  }

  bool is_concrete_ctad_construction =
      FunctionCallIsConcreteCtadConstruction(node, has_pack_expansion_actual);
  bool concrete_overload_set = FunctionCallHasConcreteOverloadSet(node);

  if (FunctionCallTryAssignDependentTemplateCalleeType(
          node, is_concrete_ctad_construction, concrete_overload_set)) {
    return (ASTNode*)node;
  }

  finished = TryAnalyzeOverloadedCallOperator(node);
  if (finished != NULL) {
    return finished;
  }

  finished = AnalyzeCXXFunctionalClassConstruction(node);
  if (finished != NULL) {
    return finished;
  }

  finished = FunctionCallResolveCalleeAndTemplates(node,
                                                   has_pack_expansion_actual);
  if (finished != NULL) {
    return finished;
  }

  finished = FunctionCallValidateCallableAndArguments(node);
  if (finished != NULL) {
    return finished;
  }

  FunctionCallClearStaleDependentFlag(node);
  FunctionCallEnsureTemplateDefinitionsAndEscalation(node);

  TypeRecord* subtype = FunctionCallAssignReturnTypeFromCallee(node);
  TypeRecord* return_type = subtype->next;

  size_t num_actual_args = node->children->length;
  bool call_ok = FunctionCallConvertFixedArguments(
      node, subtype, &num_actual_args, has_pack_expansion_actual);

  FunctionCallPromoteVarargsActuals(subtype, node,
                                    subtype->info.function.prototype.length,
                                    num_actual_args);

  finished =
      FunctionCallCheckConstexprImmediate(node, subtype, return_type, call_ok);
  if (finished != NULL) {
    return finished;
  }
  FunctionCallCheckFormatAndPrintf(node, call_ok);

  finished = FunctionCallTryLowerContractsOrInline(node, subtype, return_type,
                                                   call_ok);
  if (finished != NULL) {
    return finished;
  }

  return &node->base;
}
