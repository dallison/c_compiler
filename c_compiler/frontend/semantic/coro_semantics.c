//
//  coro_semantics.c
//  c_compiler
//
//  C++ coroutine semantic analysis and early lowering.
//

#include "coro_semantics.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "semantics.h"

typedef struct {
  bool is_coroutine;
  int suspend_count;
  bool has_co_return;
  bool has_co_return_value;
  bool has_co_yield;
} CoroutineScan;

typedef struct {
  enum {
    kSuspensionCoAwait,
    kSuspensionCoYield,
  } kind;
  ASTNode* co_await;
  ASTNode* co_yield;
  ASTNode* statement;
  VariableDeclarationASTNode* value_decl;
  VariableDeclarationASTNode* awaiter_decl;
  Symbol* awaiter;
  ASTNode* awaiter_init;
  Symbol* awaitable_temp;
  StructMember* frame_member;
  StructMember* frame_constructed_member;
  DeclarationListASTNode* decl_list;
  CompoundStatementASTNode* compound;
  size_t statement_index;
  bool multiple;
} SuspensionPoint;

typedef struct {
  SuspensionPoint* points;
  int count;
  int capacity;
} SuspensionPoints;

typedef struct {
  Symbol* symbol;
  StructMember* member;
  StructMember* constructed_member;
  bool is_parameter;
  bool move_parameter;
} CoroutinePersistedLocal;

typedef struct {
  Symbol* symbol;
  TypeRecord* type;
  TypeRecord* promise_type;
  StructMember* state;
  StructMember* done;
  StructMember* resume;
  StructMember* destroy;
  StructMember* promise;
  StructMember* promise_constructed;
  StructMember* initial_awaiter;
  StructMember* initial_awaiter_constructed;
  Vector owned_symbols;
  VariableDeclarationASTNode* decl;
} CoroutineFrame;

typedef struct {
  Symbol* symbol;
  StructMember* member;
  StructMember* constructed_member;
  bool construct_at_start;
} FrameOwnedSymbol;

static ASTNode* NewCoroutinePromiseMemberCall(Symbol* promise,
                                              const char* member_name,
                                              Vector* actuals,
                                              SourceLocation location);
static ASTNode* NewNullPointerConstant(SourceLocation location);
static ASTNode* NewAwaiterMemberCall(ASTNode* awaiter,
                                     const char* member_name,
                                     SourceLocation location);
static ASTNode* NewAwaitResumeStatement(Symbol* awaiter,
                                        SourceLocation location);
static ASTNode* NewCoroutineMoveExpression(Symbol* symbol,
                                           SourceLocation location);
static const char* CXXConstructorNameForType(TypeRecord* type);
static Vector* TakeCXXTemporaryConstructionActuals(ASTNode* expr,
                                                   TypeRecord* type);
static void ResetCompoundStatementParents(CompoundStatementASTNode* compound);
static void CoroutineFrameAddOwnedSymbol(CoroutineFrame* frame,
                                         Symbol* symbol,
                                         StructMember* member,
                                         StructMember* constructed_member,
                                         bool construct_at_start);

static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static StructMember* FindAwaiterMember(TypeRecord* awaiter_type,
                                       const char* name) {
  if (awaiter_type == NULL || !TypeIsStructOrUnion(awaiter_type) ||
      awaiter_type->info.struct_info == NULL) {
    return NULL;
  }
  String member_name;
  StringInit(&member_name, name);
  StructMember* member =
      FindStructMember(awaiter_type->info.struct_info, &member_name);
  StringDestruct(&member_name);
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function) {
      return candidate;
    }
  }
  return NULL;
}

static bool FunctionBodyIsReturnTrue(ASTNode* body) {
  if (body == NULL || body->op != AST_OP(compound)) {
    return false;
  }
  CompoundStatementASTNode* compound = (CompoundStatementASTNode*)body;
  if (compound->statements->length != 1) {
    return false;
  }
  ASTNode* stmt = compound->statements->value.p[0];
  if (stmt == NULL || stmt->op != AST_OP(return)) {
    return false;
  }
  ASTNode* value = ((CombinedStatementASTNode*)stmt)->cond;
  return value != NULL && value->op == AST_OP(number) &&
         ((ConstantASTNode*)value)->value.ivalue != 0;
}

static bool CoAwaitIsAlwaysReady(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(co_await)) {
    return false;
  }
  UnaryASTNode* co_await = (UnaryASTNode*)node;
  TypeRecord* awaiter_type = co_await->sub != NULL ? co_await->sub->type : NULL;
  StructMember* await_ready = FindAwaiterMember(awaiter_type, "await_ready");
  return await_ready != NULL && await_ready->symbol != NULL &&
         await_ready->symbol->type != NULL &&
         TypeIsFunction(await_ready->symbol->type) &&
         FunctionBodyIsReturnTrue(await_ready->symbol->type->info.function.body);
}

static StructMember* FindMemberOperatorCoAwait(TypeRecord* type) {
  return FindAwaiterMember(type, "operator co_await");
}

static bool ValidateCoAwaiterType(ASTNode* node, TypeRecord* awaiter_type) {
  if (!TypeIsStructOrUnion(awaiter_type) ||
      awaiter_type->info.struct_info == NULL) {
    SemanticError(node, "co_await operand must be an awaiter object");
    return false;
  }
  if (FindAwaiterMember(awaiter_type, "await_ready") == NULL) {
    SemanticError(node, "awaiter is missing await_ready");
    return false;
  }
  StructMember* await_suspend = FindAwaiterMember(awaiter_type, "await_suspend");
  if (await_suspend == NULL) {
    SemanticError(node, "awaiter is missing await_suspend");
    return false;
  }
  TypeRecord* await_suspend_return = NULL;
  if (await_suspend->symbol != NULL && await_suspend->symbol->type != NULL &&
      TypeIsFunction(await_suspend->symbol->type)) {
    await_suspend_return = await_suspend->symbol->type->next;
  }
  if (await_suspend_return != NULL && !TypeIsVoid(await_suspend_return) &&
      !TypeIsBool(await_suspend_return)) {
    SemanticError(node, "await_suspend return type is not supported yet");
    return false;
  }
  if (FindAwaiterMember(awaiter_type, "await_resume") == NULL) {
    SemanticError(node, "awaiter is missing await_resume");
    return false;
  }
  return true;
}

static TypeRecord* CoAwaitOperandMemberLookupType(ASTNode* operand) {
  if (operand == NULL) {
    return NULL;
  }
  if (operand->type != NULL) {
    return operand->type;
  }
  if (operand->op == AST_OP(compound_literal)) {
    CompoundLiteralASTNode* literal = (CompoundLiteralASTNode*)operand;
    return literal->sym != NULL ? literal->sym->type : NULL;
  }
  if (operand->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)operand;
    return call->left != NULL && TypeIsStructOrUnion(call->left->type)
               ? call->left->type
               : NULL;
  }
  return NULL;
}

static bool ApplyMemberOperatorCoAwait(UnaryASTNode* co_await,
                                       Symbol** awaitable_temp_out) {
  if (!CompilerIsCXX() || co_await == NULL || co_await->sub == NULL) {
    return false;
  }
  TypeRecord* operand_type = CoAwaitOperandMemberLookupType(co_await->sub);
  StructMember* member = FindMemberOperatorCoAwait(operand_type);
  if (member == NULL || member->symbol == NULL ||
      !TypeIsFunction(member->symbol->type) ||
      member->symbol->type->next == NULL) {
    return false;
  }
  Vector* constructor_actuals =
      TakeCXXTemporaryConstructionActuals(co_await->sub, operand_type);
  if (constructor_actuals != NULL) {
    TypeRecord* temp_type = TypeRecordCopy(operand_type);
    temp_type = TypeRecordCalculateSize(temp_type);
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, temp_type);
    temp->flags.is_local = true;
    temp->flags.is_defined = true;
    temp->location = co_await->base.location;
    if (awaitable_temp_out != NULL) {
      *awaitable_temp_out = temp;
    }

    const char* constructor_name = CXXConstructorNameForType(operand_type);
    ASTNode* constructor_member =
        NewStringConstantASTNode(NewString(constructor_name), NULL,
                                 co_await->base.location);
    ASTNode* constructor_access =
        NewBinaryASTNode(AST_OP(dot), NULL, co_await->base.location,
                         NewIdentifierASTNode(temp, co_await->base.location),
                         constructor_member);
    ASTNode* constructor_call =
        NewVectorASTNode(AST_OP(call), NULL, co_await->base.location,
                         constructor_access, constructor_actuals);

    ASTNode* operator_call =
        NewAwaiterMemberCall(NewIdentifierASTNode(temp, co_await->base.location),
                             "operator co_await", co_await->base.location);
    ASTNodeSetType(operator_call, TypeRecordCopy(member->symbol->type->next));
    ASTNode* comma =
        NewBinaryASTNode(AST_OP(comma), TypeRecordCopy(member->symbol->type->next),
                         co_await->base.location, constructor_call,
                         operator_call);
    ASTNodeSetType(comma, TypeRecordCopy(member->symbol->type->next));
    ASTNodeReplaceChild((ASTNode*)co_await, 0, comma, true);
    return true;
  }
  ASTNode* receiver = ASTNodeMove(co_await->sub);
  ASTNode* call =
      NewAwaiterMemberCall(receiver, "operator co_await",
                           co_await->base.location);
  ASTNodeSetType(call, TypeRecordCopy(member->symbol->type->next));
  ASTNodeReplaceChild((ASTNode*)co_await, 0, call, true);
  return true;
}

static bool CoAwaitOperandIsLValue(ASTNode* operand) {
  return operand != NULL &&
         (operand->value_category == kValueCategoryLvalue ||
          (operand->op == AST_OP(identifier) && operand->type != NULL &&
           !TypeIsFunction(operand->type)));
}

static bool CoroVectorContainsPointer(Vector* vec, void* value) {
  for (size_t i = 0; i < vec->length; i++) {
    if (vec->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

static Symbol* CoroFollowUsingAlias(Symbol* symbol) {
  int depth = 0;
  while (symbol != NULL && symbol->flags.is_using_alias &&
         symbol->alias_target != NULL && depth < 64) {
    symbol = symbol->alias_target;
    depth++;
  }
  return symbol;
}

static void CoroADLAddNamespace(Vector* namespaces, Namespace* ns) {
  while (ns != NULL) {
    if (!CoroVectorContainsPointer(namespaces, ns)) {
      VectorAppend(namespaces, ns);
    }
    if (ns == compiler->global_namespace) {
      break;
    }
    ns = ns->parent;
  }
}

static TypeRecord* CoroADLCanonicalType(TypeRecord* type) {
  while (type != NULL &&
         (TypeIsReference(type) || TypeIsPointer(type) || TypeIsArray(type))) {
    type = type->next;
  }
  return type;
}

static void CoroADLCollectNamespacesForType(TypeRecord* type,
                                            Vector* namespaces, int depth) {
  if (type == NULL || depth > 8) {
    return;
  }
  type = CoroADLCanonicalType(type);
  if (type == NULL) {
    return;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    if (str->tag_symbol != NULL) {
      CoroADLAddNamespace(namespaces, str->tag_symbol->namespace_ != NULL
                                          ? str->tag_symbol->namespace_
                                          : compiler->global_namespace);
    }
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base != NULL) {
        CoroADLCollectNamespacesForType(base->type, namespaces, depth + 1);
      }
    }
  } else if (TypeIsEnum(type) && type->info.enum_info != NULL &&
             type->info.enum_info->tag_symbol != NULL) {
    Symbol* tag = type->info.enum_info->tag_symbol;
    CoroADLAddNamespace(namespaces, tag->namespace_ != NULL
                                        ? tag->namespace_
                                        : compiler->global_namespace);
  }
  if (type->template_arguments != NULL) {
    for (size_t i = 0; i < type->template_arguments->length; i++) {
      TemplateArgument* arg = type->template_arguments->value.p[i];
      if (arg != NULL && arg->kind == kTemplateParameterType) {
        CoroADLCollectNamespacesForType(arg->type, namespaces, depth + 1);
      }
    }
  }
}

static void CoroADLAddFunctionOverloadCandidates(Vector* candidates,
                                                 Symbol* first) {
  first = CoroFollowUsingAlias(first);
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    Symbol* effective = CoroFollowUsingAlias(candidate);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !CoroVectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
}

static void CoroADLAddNamedFunctionCandidates(String* name, Namespace* ns,
                                              Vector* candidates) {
  Symbol* found = (ns == NULL || ns == compiler->global_namespace)
                      ? FindGlobalSymbol(name)
                      : NamespaceFindSymbol(ns, name);
  CoroADLAddFunctionOverloadCandidates(candidates, found);
}

static void CoroADLAddCandidatesForType(String* name, TypeRecord* type,
                                        Vector* candidates) {
  Vector namespaces;
  VectorInit(&namespaces);
  CoroADLCollectNamespacesForType(type, &namespaces, 0);
  for (size_t i = 0; i < namespaces.length; i++) {
    CoroADLAddNamedFunctionCandidates(name, namespaces.value.p[i], candidates);
  }
  VectorDestruct(&namespaces);
}

static int CoAwaitOperatorFormalScore(TypeRecord* formal, ASTNode* operand,
                                      TypeRecord* operand_type) {
  if (formal == NULL || operand == NULL || operand_type == NULL) {
    return -1;
  }
  TypeRecord* target = TypeIsReference(formal) ? formal->next : formal;
  if (TypeIsStructOrUnion(target) || TypeIsStructOrUnion(operand_type)) {
    if (!TypeIsStructOrUnion(target) || !TypeIsStructOrUnion(operand_type) ||
        target->info.struct_info != operand_type->info.struct_info) {
      return -1;
    }
  } else if (!TypeEqual(target, operand_type)) {
    return -1;
  }
  if (TypeIsReference(formal)) {
    if (TypeIsConst(operand_type) && !TypeIsConst(target)) {
      return -1;
    }
    if (formal->declarator == kDeclRValueReference) {
      return CoAwaitOperandIsLValue(operand) ? -1 : 0;
    }
    if (CoAwaitOperandIsLValue(operand)) {
      return TypeIsConst(target) ? 1 : 0;
    }
    return TypeIsConst(target) ? 2 : -1;
  }
  return 5;
}

static Symbol* FindFreeOperatorCoAwait(ASTNode* operand,
                                       TypeRecord* operand_type) {
  if (!CompilerIsCXX() || operand == NULL || operand_type == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "operator co_await");
  Vector candidates;
  VectorInit(&candidates);
  CoroADLAddNamedFunctionCandidates(&name, compiler->global_namespace,
                                    &candidates);
  CoroADLAddCandidatesForType(&name, operand_type, &candidates);
  Symbol* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  for (size_t i = 0; i < candidates.length; i++) {
    Symbol* candidate = candidates.value.p[i];
    if (candidate->type == NULL || !TypeIsFunction(candidate->type) ||
        candidate->type->info.function.prototype.length != 1 ||
        candidate->type->next == NULL) {
      continue;
    }
    Symbol* formal = candidate->type->info.function.prototype.value.p[0];
    if (formal == NULL) {
      continue;
    }
    int score = CoAwaitOperatorFormalScore(formal->type, operand, operand_type);
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
  if (ambiguous) {
    SemanticError(operand, "Ambiguous overload for operator co_await");
  }
  VectorDestruct(&candidates);
  StringDestruct(&name);
  return best;
}

static bool FreeOperatorCoAwaitTakesRValueReference(Symbol* function) {
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type) ||
      function->type->info.function.prototype.length != 1) {
    return false;
  }
  Symbol* formal = function->type->info.function.prototype.value.p[0];
  return formal != NULL && formal->type != NULL &&
         formal->type->declarator == kDeclRValueReference;
}

static bool ApplyFreeOperatorCoAwait(UnaryASTNode* co_await,
                                     Symbol** awaitable_temp_out) {
  if (!CompilerIsCXX() || co_await == NULL || co_await->sub == NULL) {
    return false;
  }
  TypeRecord* operand_type = CoAwaitOperandMemberLookupType(co_await->sub);
  Symbol* function = FindFreeOperatorCoAwait(co_await->sub, operand_type);
  if (function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type) || function->type->next == NULL) {
    return false;
  }
  Vector* constructor_actuals =
      TakeCXXTemporaryConstructionActuals(co_await->sub, operand_type);
  if (constructor_actuals != NULL) {
    TypeRecord* temp_type = TypeRecordCopy(operand_type);
    temp_type = TypeRecordCalculateSize(temp_type);
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, temp_type);
    temp->flags.is_local = true;
    temp->flags.is_defined = true;
    temp->location = co_await->base.location;
    if (awaitable_temp_out != NULL) {
      *awaitable_temp_out = temp;
    }

    const char* constructor_name = CXXConstructorNameForType(operand_type);
    ASTNode* constructor_member =
        NewStringConstantASTNode(NewString(constructor_name), NULL,
                                 co_await->base.location);
    ASTNode* constructor_access =
        NewBinaryASTNode(AST_OP(dot), NULL, co_await->base.location,
                         NewIdentifierASTNode(temp, co_await->base.location),
                         constructor_member);
    ASTNode* constructor_call =
        NewVectorASTNode(AST_OP(call), NULL, co_await->base.location,
                         constructor_access, constructor_actuals);

    Vector* operator_actuals = NewVector();
    ASTNode* operator_actual =
        FreeOperatorCoAwaitTakesRValueReference(function)
            ? NewCoroutineMoveExpression(temp, co_await->base.location)
            : NewIdentifierASTNode(temp, co_await->base.location);
    VectorAppend(operator_actuals, operator_actual);
    ASTNode* operator_call =
        NewVectorASTNode(AST_OP(call), TypeRecordCopy(function->type->next),
                         co_await->base.location,
                         NewIdentifierASTNode(function, co_await->base.location),
                         operator_actuals);
    ASTNodeSetType(operator_call, TypeRecordCopy(function->type->next));
    ASTNode* comma =
        NewBinaryASTNode(AST_OP(comma), TypeRecordCopy(function->type->next),
                         co_await->base.location, constructor_call,
                         operator_call);
    ASTNodeSetType(comma, TypeRecordCopy(function->type->next));
    ASTNodeReplaceChild((ASTNode*)co_await, 0, comma, true);
    return true;
  }
  Vector* actuals = NewVector();
  VectorAppend(actuals, ASTNodeMove(co_await->sub));
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), TypeRecordCopy(function->type->next),
                       co_await->base.location,
                       NewIdentifierASTNode(function, co_await->base.location),
                       actuals);
  ASTNodeSetType(call, TypeRecordCopy(function->type->next));
  ASTNodeReplaceChild((ASTNode*)co_await, 0, call, true);
  return true;
}

static ASTNode* NewAwaiterMemberCallWithActuals(ASTNode* awaiter,
                                                const char* member_name,
                                                Vector* actuals,
                                                SourceLocation location) {
  ASTNode* member =
      NewStringConstantASTNode(NewString(member_name), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, awaiter, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

static ASTNode* NewAwaiterMemberCall(ASTNode* awaiter,
                                     const char* member_name,
                                     SourceLocation location) {
  return NewAwaiterMemberCallWithActuals(awaiter, member_name, NewVector(),
                                         location);
}

static ASTNode* NewFrameAddress(CoroutineFrame* frame,
                                SourceLocation location) {
  return NewIdentifierASTNode(frame->symbol, location);
}

static ASTNode* NewFrameMemberAccess(CoroutineFrame* frame,
                                     StructMember* member,
                                     SourceLocation location) {
  ASTNode* receiver = NewIdentifierASTNode(frame->symbol, location);
  ASTNode* member_node = NewStructMemberASTNode(member, location);
  ((StructMemberASTNode*)member_node)->byte_offset = member->byte_offset;
  ASTNode* access =
      NewBinaryASTNode(AST_OP(arrow), TypeRecordCopy(member->symbol->type),
                       location, receiver, member_node);
  access->value_category = kValueCategoryLvalue;
  return access;
}

static ASTNode* NewFrameIntAssignment(CoroutineFrame* frame,
                                      StructMember* member,
                                      int64_t value,
                                      SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), member->symbol->type, location,
                       NewFrameMemberAccess(frame, member, location),
                       NewIntConstantASTNode(
                           value,
                           NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location)),
      location);
}

static ASTNode* NewFrameAssignment(CoroutineFrame* frame,
                                   StructMember* member,
                                   ASTNode* value,
                                   SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), member->symbol->type, location,
                       NewFrameMemberAccess(frame, member, location), value),
      location);
}

static const char* CXXConstructorNameForType(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return type->info.struct_info->tag_name->value;
}

static void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals,
                                             SourceLocation location) {
  if (actuals == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      !StructHasVirtualBases(type->info.struct_info)) {
    return;
  }
  ASTNode* arg =
      NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  if (actuals->length == 0) {
    VectorAppend(actuals, arg);
  } else {
    VectorInsertBefore(actuals, 0, arg);
  }
}

static StructMember* FindCXXConstructorForType(TypeRecord* type) {
  const char* constructor_name = CXXConstructorNameForType(type);
  if (constructor_name == NULL || type->info.struct_info == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, constructor_name);
  StructMember* ctor = FindStructMember(type->info.struct_info, &name);
  StringDestruct(&name);
  if (ctor == NULL || !ctor->is_member_function ||
      ctor->symbol == NULL || ctor->symbol->type == NULL ||
      !TypeIsFunction(ctor->symbol->type) ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}

static bool CXXConstructorCandidateIsDefault(StructMember* candidate,
                                             TypeRecord* type) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type)) {
    return false;
  }
  FunctionInfo* info = &candidate->symbol->type->info.function;
  if (!info->is_constructor || info->is_deleted) {
    return false;
  }
  size_t expected = 1;
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      StructHasVirtualBases(type->info.struct_info)) {
    expected++;
  }
  return info->prototype.length == expected;
}

static StructMember* FindCXXDefaultConstructorForType(TypeRecord* type) {
  StructMember* ctor = FindCXXConstructorForType(type);
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (CXXConstructorCandidateIsDefault(candidate, type)) {
      return candidate;
    }
  }
  return NULL;
}

static bool CXXConstructorCandidateIsCopyOrMove(StructMember* candidate,
                                                TypeRecord* type,
                                                bool move) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type)) {
    return false;
  }
  FunctionInfo* info = &candidate->symbol->type->info.function;
  if (!info->is_constructor || info->is_deleted) {
    return false;
  }
  size_t source_index = 1;
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
      StructHasVirtualBases(type->info.struct_info)) {
    source_index++;
  }
  if (info->prototype.length != source_index + 1) {
    return false;
  }
  Symbol* source = info->prototype.value.p[source_index];
  return source != NULL && TypeIsReference(source->type) &&
         source->type->next != NULL && TypeIsStructOrUnion(source->type->next) &&
         source->type->next->info.struct_info == type->info.struct_info &&
         (source->type->declarator == kDeclRValueReference) == move;
}

static bool CXXConstructorCandidateIsCopy(StructMember* candidate,
                                          TypeRecord* type) {
  return CXXConstructorCandidateIsCopyOrMove(candidate, type, false);
}

static bool CXXConstructorCandidateIsMove(StructMember* candidate,
                                          TypeRecord* type) {
  return CXXConstructorCandidateIsCopyOrMove(candidate, type, true);
}

static StructMember* FindCXXCopyConstructorForType(TypeRecord* type) {
  StructMember* ctor = FindCXXConstructorForType(type);
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (CXXConstructorCandidateIsCopy(candidate, type)) {
      return candidate;
    }
  }
  return NULL;
}

static StructMember* FindCXXMoveConstructorForType(TypeRecord* type) {
  StructMember* ctor = FindCXXConstructorForType(type);
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (CXXConstructorCandidateIsMove(candidate, type)) {
      return candidate;
    }
  }
  return NULL;
}

static StructMember* FindCXXDestructorForType(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "~");
  StringAppendString(&name, type->info.struct_info->tag_name);
  StructMember* destructor = FindStructMember(type->info.struct_info, &name);
  StringDestruct(&name);
  if (destructor == NULL || !destructor->is_member_function ||
      destructor->symbol == NULL || destructor->symbol->type == NULL ||
      !TypeIsFunction(destructor->symbol->type) ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}

static bool CoroutineFrameOwnedTypeNeedsCXXLifetime(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  return FindCXXConstructorForType(type) != NULL ||
         FindCXXDestructorForType(type) != NULL;
}

static bool ValidateCoroutineFrameOwnedAwaiterConstructible(ASTNode* node,
                                                            TypeRecord* type) {
  if (!CoroutineFrameOwnedTypeNeedsCXXLifetime(type) ||
      FindCXXCopyConstructorForType(type) != NULL) {
    return true;
  }
  SemanticError(node,
                "coroutine frame-owned awaiter requires a copy constructor");
  return false;
}

static bool CoroutineTypeCanBeCopyConstructed(TypeRecord* type) {
  return !CoroutineFrameOwnedTypeNeedsCXXLifetime(type) ||
         FindCXXCopyConstructorForType(type) != NULL;
}

static bool CoroutineTypeCanBeMoveConstructed(TypeRecord* type) {
  return CoroutineFrameOwnedTypeNeedsCXXLifetime(type) &&
         FindCXXMoveConstructorForType(type) != NULL;
}

static ASTNode* NewCoroutineMoveExpression(Symbol* symbol,
                                           SourceLocation location) {
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, true);
  TypeRecordChain(ref, TypeRecordCopy(symbol->type));
  ref = TypeRecordCalculateSize(ref);
  ASTNode* cast =
      NewCastASTNode(ref, location, NewIdentifierASTNode(symbol, location));
  ((CastASTNode*)cast)->kind = kCastStatic;
  cast->value_category = kValueCategoryXvalue;
  return cast;
}

static ASTNode* NewCoroutineFrameMemberConstructorCallWithActuals(
    CoroutineFrame* frame, StructMember* member, Vector* actuals,
    SourceLocation location) {
  TypeRecord* type = member != NULL && member->symbol != NULL
                         ? member->symbol->type
                         : NULL;
  const char* constructor_name = CXXConstructorNameForType(type);
  if (constructor_name == NULL || FindCXXConstructorForType(type) == NULL) {
    return NULL;
  }
  CXXPrependCompleteObjectArgument(type, actuals, location);
  ASTNode* receiver = NewFrameMemberAccess(frame, member, location);
  ASTNode* ctor =
      NewStringConstantASTNode(NewString(constructor_name), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, ctor);
  return NewExpressionStatementASTNode(
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals),
      location);
}

static ASTNode* NewCoroutineFrameMemberDefaultConstructorCall(
    CoroutineFrame* frame, StructMember* member, SourceLocation location) {
  TypeRecord* type = member != NULL && member->symbol != NULL
                         ? member->symbol->type
                         : NULL;
  if (FindCXXDefaultConstructorForType(type) == NULL) {
    return NULL;
  }
  return NewCoroutineFrameMemberConstructorCallWithActuals(
      frame, member, NewVector(), location);
}

static ASTNode* NewFrameMemberInitialization(CoroutineFrame* frame,
                                             StructMember* member,
                                             StructMember* constructed_member,
                                             ASTNode* value,
                                             SourceLocation location) {
  Vector* statements = NewVector();
  ASTNode* init = NULL;
  if (value != NULL && FindCXXConstructorForType(member->symbol->type) != NULL) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, value);
    init = NewCoroutineFrameMemberConstructorCallWithActuals(
        frame, member, actuals, location);
  } else if (value != NULL) {
    init = NewFrameAssignment(frame, member, value, location);
  } else {
    init = NewCoroutineFrameMemberDefaultConstructorCall(frame, member,
                                                        location);
  }
  if (init != NULL) {
    VectorAppend(statements, init);
  }
  if (constructed_member != NULL) {
    VectorAppend(statements,
                 NewFrameIntAssignment(frame, constructed_member, 1,
                                       location));
  }
  if (statements->length == 1) {
    return statements->value.p[0];
  }
  return NewCompoundStatementASTNode(statements, location);
}

static ASTNode* NewFrameMemberConstructorInitialization(
    CoroutineFrame* frame, StructMember* member,
    StructMember* constructed_member, Vector* actuals,
    SourceLocation location) {
  Vector* statements = NewVector();
  ASTNode* init = NewCoroutineFrameMemberConstructorCallWithActuals(
      frame, member, actuals, location);
  if (init != NULL) {
    VectorAppend(statements, init);
  }
  if (constructed_member != NULL) {
    VectorAppend(statements,
                 NewFrameIntAssignment(frame, constructed_member, 1,
                                       location));
  }
  if (statements->length == 1) {
    return statements->value.p[0];
  }
  return NewCompoundStatementASTNode(statements, location);
}

static Vector* TakeCXXTemporaryConstructionActuals(ASTNode* expr,
                                                   TypeRecord* type) {
  if (expr == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return NULL;
  }
  if (expr->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)expr;
    if (call->left == NULL || !TypeIsStructOrUnion(call->left->type) ||
        call->left->type->info.struct_info != type->info.struct_info) {
      return NULL;
    }
    Vector* actuals = NewVector();
    for (size_t i = 0; i < call->children->length; i++) {
      VectorAppend(actuals, ASTNodeMove(call->children->value.p[i]));
    }
    call->children->length = 0;
    return actuals;
  }
  if (expr->op != AST_OP(comma)) {
    return NULL;
  }
  BinaryASTNode* comma = (BinaryASTNode*)expr;
  if (comma->right == NULL || !TypeIsStructOrUnion(comma->right->type) ||
      comma->right->type->info.struct_info != type->info.struct_info ||
      comma->left == NULL || comma->left->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)comma->left;
  size_t first_actual = 0;
  if (call->left == NULL) {
    return NULL;
  }
  if (call->left->op == AST_OP(identifier)) {
    Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
    if (callee == NULL || callee->type == NULL ||
        !TypeIsFunction(callee->type) ||
        !callee->type->info.function.is_constructor ||
        callee->type->info.function.cxx_member_owner !=
            type->info.struct_info) {
      return NULL;
    }
    first_actual = 1;
  } else if (call->left->op == AST_OP(dot)) {
    BinaryASTNode* member_access = (BinaryASTNode*)call->left;
    TypeRecord* receiver_type =
        member_access->left != NULL ? member_access->left->type : NULL;
    if (!TypeIsStructOrUnion(receiver_type) ||
        receiver_type->info.struct_info != type->info.struct_info) {
      return NULL;
    }
  } else {
    return NULL;
  }
  if (call->children->length < first_actual) {
    return NULL;
  }
  Vector* actuals = NewVector();
  for (size_t i = first_actual; i < call->children->length; i++) {
    VectorAppend(actuals, ASTNodeMove(call->children->value.p[i]));
  }
  call->children->length = 0;
  return actuals;
}

static ASTNode* NewCoroutineFrameMemberDestructorCall(
    CoroutineFrame* frame, StructMember* member, SourceLocation location) {
  TypeRecord* type = member != NULL && member->symbol != NULL
                         ? member->symbol->type
                         : NULL;
  if (FindCXXDestructorForType(type) == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  ASTNode* receiver = NewFrameMemberAccess(frame, member, location);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, destructor);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(type, actuals, location);
  return NewExpressionStatementASTNode(
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals),
      location);
}

static ASTNode* NewCoroutineLocalDestructorCall(Symbol* symbol,
                                               SourceLocation location) {
  TypeRecord* type = symbol != NULL ? symbol->type : NULL;
  if (FindCXXDestructorForType(type) == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  ASTNode* receiver = NewIdentifierASTNode(symbol, location);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, destructor);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(type, actuals, location);
  return NewExpressionStatementASTNode(
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals),
      location);
}

static ASTNode* NewFunctionAddress(Symbol* function,
                                   TypeRecord* pointer_type,
                                   SourceLocation location) {
  ASTNode* id = NewIdentifierASTNode(function, location);
  ASTNode* address = NewUnaryASTNode(AST_OP(address),
                                    TypeRecordCopy(pointer_type),
                                    location, id);
  id->flags |= kASTNeedAddress;
  return address;
}

static ASTNode* NewCoroutineReturnObjectStatement(Symbol* promise,
                                                  TypeRecord* return_type,
                                                  SourceLocation location) {
  ASTNode* return_object = NewCoroutinePromiseMemberCall(
      promise, "get_return_object", NewVector(), location);
  Symbol* result =
      SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(return_type));
  result->flags.is_local = true;
  result->flags.is_defined = true;
  result->location = location;
  Vector* decls = NewVector();
  VectorAppend(decls,
               NewVariableDeclarationASTNode(
                   result, NewExpressionInitializerASTNode(return_object,
                                                           location),
                   location));
  Vector* statements = NewVector();
  VectorAppend(statements, NewDeclarationListASTNode(decls, location));
  ASTNode* return_stmt =
      NewCombinedStatementASTNode(AST_OP(return),
                                  NewIdentifierASTNode(result, location),
                                  NULL, location);
  return_stmt->flags |= kASTCoroutineLoweredReturn;
  VectorAppend(statements, return_stmt);
  return NewCompoundStatementASTNode(statements, location);
}

static ASTNode* LowerReadyCoAwaitExpression(UnaryASTNode* co_await) {
  SourceLocation location = co_await->base.location;
  ASTNode* awaiter = ASTNodeMove(co_await->sub);
  ASTNode* await_resume =
      NewAwaiterMemberCall(awaiter, "await_resume", location);
  ASTNodeSetType(await_resume, co_await->base.type);
  return await_resume;
}

static void LowerReadyCoAwaitNode(ASTNode* node, void* data, int child_id,
                                  VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL ||
      node->op != AST_OP(co_await) || !CoAwaitIsAlwaysReady(node)) {
    return;
  }
  ASTNodeReplaceChild(node->parent, node->child_id,
                      LowerReadyCoAwaitExpression((UnaryASTNode*)node),
                      true);
}

static void ScanCoroutineNode(ASTNode* node, void* data, int child_id,
                              VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  CoroutineScan* scan = data;
  switch (node->op) {
    case AST_OP(co_return):
      scan->is_coroutine = true;
      scan->has_co_return = true;
      scan->has_co_return_value = ((CombinedStatementASTNode*)node)->cond != NULL;
      return;
    case AST_OP(co_await):
      scan->is_coroutine = true;
      if (!CoAwaitIsAlwaysReady(node)) {
        scan->suspend_count++;
      }
      return;
    case AST_OP(co_yield):
      scan->is_coroutine = true;
      scan->has_co_yield = true;
      scan->suspend_count++;
      return;
    default:
      return;
  }
}

static TypeRecord* ResolveDirectCoroutinePromiseType(TypeRecord* return_type) {
  if (return_type == NULL || !TypeIsStructOrUnion(return_type) ||
      return_type->info.struct_info == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "promise_type");
  StructMember* member = FindStructMember(return_type->info.struct_info, &name);
  StringDestruct(&name);
  if (member == NULL || member->symbol == NULL ||
      !StorageIs(member->symbol->storage, STO(typedef)) ||
      member->symbol->type == NULL) {
    return NULL;
  }
  return TypeRecordCalculateSize(TypeRecordCopy(member->symbol->type));
}

static StructMember* FindCoroutinePromiseMember(TypeRecord* promise_type,
                                                const char* name) {
  if (promise_type == NULL || !TypeIsStructOrUnion(promise_type) ||
      promise_type->info.struct_info == NULL) {
    return NULL;
  }
  String member_name;
  StringInit(&member_name, name);
  StructMember* member =
      FindStructMember(promise_type->info.struct_info, &member_name);
  StringDestruct(&member_name);
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function) {
      return candidate;
    }
  }
  return NULL;
}

static bool RequireCoroutinePromiseMember(ASTNode* node, TypeRecord* promise,
                                          const char* name) {
  if (FindCoroutinePromiseMember(promise, name) != NULL) {
    return true;
  }
  SemanticError(node, "coroutine promise_type is missing %s", name);
  return false;
}

static TypeRecord* CoroutinePromiseMemberReturnType(TypeRecord* promise,
                                                    const char* name) {
  StructMember* member = FindCoroutinePromiseMember(promise, name);
  if (member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL ||
      !TypeIsFunction(member->symbol->type)) {
    return NULL;
  }
  return member->symbol->type->next;
}

static bool AwaiterTypeIsAlwaysReady(TypeRecord* awaiter_type) {
  StructMember* await_ready = FindAwaiterMember(awaiter_type, "await_ready");
  return await_ready != NULL && await_ready->symbol != NULL &&
         await_ready->symbol->type != NULL &&
         TypeIsFunction(await_ready->symbol->type) &&
         FunctionBodyIsReturnTrue(await_ready->symbol->type->info.function.body);
}

static TypeRecord* AwaiterAwaitSuspendReturnType(Symbol* awaiter) {
  StructMember* await_suspend =
      awaiter != NULL ? FindAwaiterMember(awaiter->type, "await_suspend") : NULL;
  if (await_suspend == NULL || await_suspend->symbol == NULL ||
      await_suspend->symbol->type == NULL ||
      !TypeIsFunction(await_suspend->symbol->type)) {
    return NULL;
  }
  return await_suspend->symbol->type->next;
}

static ASTNode* NewCoroutinePromiseMemberCall(Symbol* promise,
                                              const char* member_name,
                                              Vector* actuals,
                                              SourceLocation location) {
  ASTNode* receiver = NewIdentifierASTNode(promise, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(member_name), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

static ASTNode* LowerCoReturnStatement(CombinedStatementASTNode* co_return,
                                       Symbol* promise,
                                       CoroutineFrame* frame,
                                       TypeRecord* coroutine_return_type) {
  SourceLocation location = co_return->base.location;
  Vector* statements = NewVector();
  if (co_return->cond != NULL) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, ASTNodeMove(co_return->cond));
    VectorAppend(statements,
                 NewExpressionStatementASTNode(
                     NewCoroutinePromiseMemberCall(promise, "return_value",
                                                   actuals, location),
                     location));
  } else {
    VectorAppend(statements,
                 NewExpressionStatementASTNode(
                     NewCoroutinePromiseMemberCall(promise, "return_void",
                                                   NewVector(), location),
                     location));
  }
  if (frame != NULL) {
    VectorAppend(statements,
                 NewFrameIntAssignment(frame, frame->state, 0, location));
  }
  if (frame != NULL) {
    VectorAppend(statements,
                 NewFrameIntAssignment(frame, frame->done, 1, location));
  }
  if (frame != NULL && frame->resume != NULL) {
    VectorAppend(statements,
                 NewFrameAssignment(frame, frame->resume,
                                    NewNullPointerConstant(location),
                                    location));
  }
  VectorAppend(statements,
               NewExpressionStatementASTNode(
                   NewCoroutinePromiseMemberCall(promise, "final_suspend",
                                                 NewVector(), location),
                   location));

  ASTNode* return_object =
      NewCoroutineReturnObjectStatement(promise, coroutine_return_type,
                                        location);
  VectorAppend(statements, return_object);
  return NewCompoundStatementASTNode(statements, location);
}

static void LowerCoReturnsInStatement(ASTNode* node, Symbol* promise,
                                      CoroutineFrame* frame,
                                      TypeRecord* coroutine_return_type);

static void LowerCoReturnChild(ASTNode* parent, int child_id, Symbol* promise,
                               CoroutineFrame* frame,
                               TypeRecord* coroutine_return_type) {
  ASTNode* child = NULL;
  switch (parent->op) {
    case AST_OP(if): {
      IfStatementASTNode* if_node = (IfStatementASTNode*)parent;
      child = child_id == 1 ? if_node->if_part : if_node->else_part;
      break;
    }
    case AST_OP(while):
    case AST_OP(do): {
      child = ((CombinedStatementASTNode*)parent)->stmt;
      break;
    }
    case AST_OP(for): {
      if (child_id != 3) {
        return;
      }
      child = ((ForStatementASTNode*)parent)->stmt;
      break;
    }
    case AST_OP(switch): {
      if (child_id != 1) {
        return;
      }
      child = ((SwitchStatementASTNode*)parent)->stmt;
      break;
    }
    case AST_OP(case): {
      if (child_id != 1) {
        return;
      }
      child = ((CaseLabelASTNode*)parent)->stmt;
      break;
    }
    case AST_OP(label): {
      if (child_id != 0) {
        return;
      }
      child = ((LabelASTNode*)parent)->stmt;
      break;
    }
    default:
      return;
  }
  if (child == NULL) {
    return;
  }
  if (child->op == AST_OP(co_return)) {
    ASTNodeReplaceChild(parent, child_id,
                        LowerCoReturnStatement((CombinedStatementASTNode*)child,
                                               promise, frame,
                                               coroutine_return_type),
                        true);
    return;
  }
  LowerCoReturnsInStatement(child, promise, frame, coroutine_return_type);
}

static void LowerCoReturnsInStatement(ASTNode* node, Symbol* promise,
                                      CoroutineFrame* frame,
                                      TypeRecord* coroutine_return_type) {
  if (node == NULL) {
    return;
  }
  switch (node->op) {
    case AST_OP(compound): {
      CompoundStatementASTNode* compound = (CompoundStatementASTNode*)node;
      for (size_t i = 0; i < compound->statements->length; i++) {
        ASTNode* stmt = compound->statements->value.p[i];
        if (stmt != NULL && stmt->op == AST_OP(co_return)) {
          ASTNode* lowered =
              LowerCoReturnStatement((CombinedStatementASTNode*)stmt, promise,
                                     frame, coroutine_return_type);
          VectorSet(compound->statements, i, lowered);
          lowered->parent = node;
          lowered->child_id = (int)i;
        } else {
          LowerCoReturnsInStatement(stmt, promise, frame,
                                    coroutine_return_type);
        }
      }
      return;
    }
    case AST_OP(if):
      LowerCoReturnChild(node, 1, promise, frame, coroutine_return_type);
      LowerCoReturnChild(node, 2, promise, frame, coroutine_return_type);
      return;
    case AST_OP(while):
    case AST_OP(do):
    case AST_OP(switch):
      LowerCoReturnChild(node, 1, promise, frame, coroutine_return_type);
      return;
    case AST_OP(for):
      LowerCoReturnChild(node, 3, promise, frame, coroutine_return_type);
      return;
    case AST_OP(case):
      LowerCoReturnChild(node, 1, promise, frame, coroutine_return_type);
      return;
    case AST_OP(label):
      LowerCoReturnChild(node, 0, promise, frame, coroutine_return_type);
      return;
    default:
      return;
  }
}

static ASTNode* UnwrapExpressionInitializer(ASTNode* initializer) {
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  return initializer;
}

static ASTNode* VariableInitializerExpression(ASTNode* initializer) {
  if (initializer != NULL && initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  return UnwrapExpressionInitializer(initializer);
}

static void ReplaceVariableInitializerExpression(VariableDeclarationASTNode* decl,
                                                 ASTNode* expr) {
  ASTNode* initializer = decl->initializer;
  if (initializer != NULL && initializer->op == AST_OP(init)) {
    BinaryASTNode* init = (BinaryASTNode*)initializer;
    if (init->right != NULL && init->right->op == AST_OP(expr_init)) {
      ASTNodeReplaceChild(init->right, 0, expr, true);
    } else {
      ASTNodeReplaceChild(initializer, 1,
                          NewExpressionInitializerASTNode(
                              expr, initializer->location),
                          true);
    }
    return;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    ASTNodeReplaceChild(initializer, 0, expr, true);
    return;
  }
  ASTNodeReplaceChild((ASTNode*)decl, 0,
                      NewExpressionInitializerASTNode(expr,
                                                      decl->base.location),
                      true);
}

static Symbol* CoAwaitIdentifierOperand(ASTNode* co_await) {
  if (co_await == NULL || co_await->op != AST_OP(co_await)) {
    return NULL;
  }
  ASTNode* operand = ((UnaryASTNode*)co_await)->sub;
  if (operand == NULL || operand->op != AST_OP(identifier)) {
    return NULL;
  }
  return ((IdentifierASTNode*)operand)->symbol;
}

static TypeRecord* CoAwaitOperandType(ASTNode* co_await) {
  if (co_await == NULL || co_await->op != AST_OP(co_await)) {
    return NULL;
  }
  ASTNode* operand = ((UnaryASTNode*)co_await)->sub;
  if (operand == NULL) {
    return NULL;
  }
  if (operand->type != NULL) {
    return operand->type;
  }
  if (operand->op == AST_OP(comma)) {
    BinaryASTNode* comma = (BinaryASTNode*)operand;
    return comma->right != NULL ? comma->right->type : NULL;
  }
  if (operand->op == AST_OP(inline_call)) {
    InlineCallASTNode* inline_call = (InlineCallASTNode*)operand;
    return inline_call->ret_value != NULL ? inline_call->ret_value->type : NULL;
  }
  if (operand->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)operand;
    if (call->left != NULL && TypeIsStructOrUnion(call->left->type)) {
      return call->left->type;
    }
    TypeRecord* callee_type = call->left != NULL ? call->left->type : NULL;
    if (TypeIsPointer(callee_type)) {
      callee_type = callee_type->next;
    }
    if (TypeIsFunction(callee_type) && callee_type->next != NULL) {
      ASTNodeSetType(operand, TypeRecordCopy(callee_type->next));
      return operand->type;
    }
    return NULL;
  }
  if (operand->op == AST_OP(compound_literal)) {
    CompoundLiteralASTNode* literal = (CompoundLiteralASTNode*)operand;
    return literal->sym != NULL ? literal->sym->type : NULL;
  }
  return NULL;
}

static Symbol* NewCoroutineAwaiterTemporary(TypeRecord* type,
                                            SourceLocation location) {
  if (type == NULL) {
    return NULL;
  }
  TypeRecord* temp_type = TypeRecordCopy(type);
  temp_type = TypeRecordCalculateSize(temp_type);
  Symbol* awaiter =
      SyntaxNewTemporary(&compiler->syntax, temp_type);
  awaiter->flags.is_local = true;
  awaiter->flags.is_defined = true;
  awaiter->location = location;
  return awaiter;
}

static VariableDeclarationASTNode* FindAwaiterDeclarationBefore(
    SuspensionPoint* point, Symbol* awaiter) {
  if (point == NULL || point->compound == NULL || awaiter == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < point->statement_index; i++) {
    ASTNode* stmt = point->compound->statements->value.p[i];
    if (stmt == NULL || stmt->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decls = (DeclarationListASTNode*)stmt;
    for (size_t j = 0; j < decls->declarations->length; j++) {
      VariableDeclarationASTNode* decl = decls->declarations->value.p[j];
      if (decl != NULL && decl->symbol == awaiter) {
        return decl;
      }
    }
  }
  return NULL;
}

static bool LowerCoYieldStatement(SuspensionPoint* point, Symbol* promise,
                                  CoroutineFrame* frame,
                                  TypeRecord* yield_awaiter_type) {
  if (yield_awaiter_type == NULL) {
    SemanticError(point->co_yield,
                  "coroutine yield_value return type is invalid");
    return false;
  }
  SourceLocation location = point->co_yield->location;
  Symbol* awaiter =
      SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(yield_awaiter_type));
  awaiter->flags.is_local = true;
  awaiter->flags.is_defined = true;
  awaiter->location = location;
  point->awaiter = awaiter;
  CoroutineFrameAddOwnedSymbol(frame, awaiter, point->frame_member,
                               point->frame_constructed_member,
                               false);

  Vector* actuals = NewVector();
  VectorAppend(actuals, ASTNodeMove(((UnaryASTNode*)point->co_yield)->sub));
  ASTNode* yield_call = NewCoroutinePromiseMemberCall(
      promise, "yield_value", actuals, location);
  ASTNodeSetType(yield_call, TypeRecordCopy(awaiter->type));
  ASTNode* init = NewFrameMemberInitialization(
      frame, point->frame_member, point->frame_constructed_member,
      yield_call, location);
  VectorSet(point->compound->statements, point->statement_index, init);
  init->parent = (ASTNode*)point->compound;
  init->child_id = (int)point->statement_index;
  return true;
}

static StructMember* AddCoroutineFrameIntMember(Struct* str,
                                                const char* name) {
  Symbol* symbol = NewSymbol(name,
                             NewTypeRecordWithSize(kTypeInt, kQualPlain),
                             STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

static StructMember* AddCoroutineFrameTypedMember(Struct* str,
                                                  const char* name,
                                                  TypeRecord* type) {
  TypeRecord* member_type = TypeRecordCopy(type);
  member_type = TypeRecordCalculateSize(member_type);
  Symbol* symbol = NewSymbol(name, member_type, STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

static void CoroutineFrameAddOwnedSymbol(CoroutineFrame* frame,
                                         Symbol* symbol,
                                         StructMember* member,
                                         StructMember* constructed_member,
                                         bool construct_at_start) {
  if (frame == NULL || symbol == NULL || member == NULL) {
    return;
  }
  FrameOwnedSymbol* owned = malloc(sizeof(FrameOwnedSymbol));
  assert(owned != NULL);
  owned->symbol = symbol;
  owned->member = member;
  owned->constructed_member = constructed_member;
  owned->construct_at_start = construct_at_start;
  VectorAppend(&frame->owned_symbols, owned);
}

static StructMember* CoroutineFrameMemberForSymbol(CoroutineFrame* frame,
                                                   Symbol* symbol) {
  if (frame == NULL || symbol == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < frame->owned_symbols.length; i++) {
    FrameOwnedSymbol* owned = frame->owned_symbols.value.p[i];
    if (owned != NULL && owned->symbol == symbol) {
      return owned->member;
    }
  }
  return NULL;
}

static void CoroutineFrameOwnedSymbolsDestruct(CoroutineFrame* frame) {
  if (frame == NULL) {
    return;
  }
  for (size_t i = 0; i < frame->owned_symbols.length; i++) {
    free(frame->owned_symbols.value.p[i]);
  }
  VectorDestruct(&frame->owned_symbols);
}

static void InsertCoroutineFrameStarterInitializers(
    CompoundStatementASTNode* body,
    CoroutineFrame* frame,
    SourceLocation location) {
  if (body == NULL || frame == NULL) {
    return;
  }
  size_t index = 1;
  CompoundASTNodeInsertStatement(
      body, NewFrameIntAssignment(frame, frame->state, 0, location), index++);
  for (size_t i = 0; i < frame->owned_symbols.length; i++) {
    FrameOwnedSymbol* owned = frame->owned_symbols.value.p[i];
    if (owned == NULL || owned->constructed_member == NULL) {
      continue;
    }
    CompoundASTNodeInsertStatement(
        body, NewFrameIntAssignment(frame, owned->constructed_member, 0,
                                    location), index++);
  }
  for (size_t i = 0; i < frame->owned_symbols.length; i++) {
    FrameOwnedSymbol* owned = frame->owned_symbols.value.p[i];
    if (owned == NULL || !owned->construct_at_start) {
      continue;
    }
    CompoundASTNodeInsertStatement(
        body,
        NewFrameMemberInitialization(frame, owned->member,
                                     owned->constructed_member, NULL,
                                     location),
        index++);
  }
}

static ASTNode* NewCoroutineFrameGuardedDestructor(CoroutineFrame* frame,
                                                   FrameOwnedSymbol* owned,
                                                   SourceLocation location) {
  ASTNode* dtor = NewCoroutineFrameMemberDestructorCall(
      frame, owned->member, location);
  if (dtor == NULL) {
    return NULL;
  }
  if (owned->constructed_member == NULL) {
    return dtor;
  }
  ASTNode* condition =
      NewBinaryASTNode(AST_OP(noteq),
                       NewTypeRecordWithSize(kTypeBool, kQualPlain),
                       location,
                       NewFrameMemberAccess(frame, owned->constructed_member,
                                            location),
                       NewIntConstantASTNode(
                           0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  Vector* statements = NewVector();
  VectorAppend(statements, dtor);
  VectorAppend(statements,
               NewFrameIntAssignment(frame, owned->constructed_member, 0,
                                     location));
  return NewIfStatementASTNode(
      condition, NewCompoundStatementASTNode(statements, location),
      NULL, false, location);
}

static void AppendCoroutineFrameDestructors(Vector* statements,
                                            CoroutineFrame* frame,
                                            SourceLocation location) {
  if (statements == NULL || frame == NULL) {
    return;
  }
  for (size_t i = frame->owned_symbols.length; i > 0; i--) {
    FrameOwnedSymbol* owned = frame->owned_symbols.value.p[i - 1];
    if (owned == NULL || owned->member == NULL ||
        owned->member->symbol == NULL) {
      continue;
    }
    ASTNode* dtor = NewCoroutineFrameGuardedDestructor(frame, owned,
                                                      location);
    if (dtor != NULL) {
      VectorAppend(statements, dtor);
    }
  }
}

static TypeRecord* NewCoroutineFramePointerType(TypeRecord* frame_type) {
  return NewPointerTo(kQualPlain, TypeRecordCopy(frame_type));
}

static Symbol* NewCoroutineFrameParameter(TypeRecord* frame_type,
                                          SourceLocation location) {
  Symbol* symbol =
      NewSymbol("__frame", NewCoroutineFramePointerType(frame_type),
                STO(implicit));
  symbol->flags.is_argument = true;
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = location;
  symbol->value.arg_number = 0;
  return symbol;
}

static TypeRecord* NewCoroutineResumePointerType(TypeRecord* return_type,
                                                 TypeRecord* frame_type,
                                                 SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, TypeRecordCopy(return_type));
  VectorAppend(&func->info.function.prototype,
               NewCoroutineFrameParameter(frame_type, location));
  return NewPointerTo(kQualPlain, func);
}

static TypeRecord* NewCoroutineResumeFunctionType(TypeRecord* return_type,
                                                  TypeRecord* frame_type,
                                                  Symbol** frame_param_out,
                                                  SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, TypeRecordCopy(return_type));
  Symbol* frame_param = NewCoroutineFrameParameter(frame_type, location);
  VectorAppend(&func->info.function.prototype, frame_param);
  if (frame_param_out != NULL) {
    *frame_param_out = frame_param;
  }
  func->info.function.definition = true;
  return func;
}

static TypeRecord* NewCoroutineDestroyPointerType(TypeRecord* frame_type,
                                                  SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  VectorAppend(&func->info.function.prototype,
               NewCoroutineFrameParameter(frame_type, location));
  return NewPointerTo(kQualPlain, func);
}

static TypeRecord* NewCoroutineDestroyFunctionType(TypeRecord* frame_type,
                                                   Symbol** frame_param_out,
                                                   SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  Symbol* frame_param = NewCoroutineFrameParameter(frame_type, location);
  VectorAppend(&func->info.function.prototype, frame_param);
  if (frame_param_out != NULL) {
    *frame_param_out = frame_param;
  }
  func->info.function.definition = true;
  return func;
}

static StructMember* AddCoroutineFrameResumeMember(Struct* str,
                                                   TypeRecord* return_type,
                                                   TypeRecord* frame_type,
                                                   SourceLocation location) {
  Symbol* symbol = NewSymbol("__resume",
                             NewCoroutineResumePointerType(return_type,
                                                           frame_type,
                                                           location),
                             STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

static StructMember* AddCoroutineFrameDestroyMember(Struct* str,
                                                    TypeRecord* frame_type,
                                                    SourceLocation location) {
  Symbol* symbol = NewSymbol("__destroy",
                             NewCoroutineDestroyPointerType(frame_type,
                                                           location),
                             STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

static Symbol* FindCoroutineAllocationFunctionByArgCount(Symbol* first,
                                                         size_t arg_count) {
  for (Symbol* symbol = first; symbol != NULL; symbol = symbol->overload_next) {
    if (symbol->type != NULL && TypeIsFunction(symbol->type) &&
        symbol->type->info.function.prototype.length == arg_count) {
      return symbol;
    }
  }
  return NULL;
}

static Symbol* GetCoroutineAllocationFunction(const char* name,
                                              TypeRecord* return_type,
                                              TypeRecord* arg_type,
                                              SourceLocation location) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* first = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  Symbol* existing = FindCoroutineAllocationFunctionByArgCount(first, 1);
  if (existing != NULL) {
    return existing;
  }
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, return_type);
  Symbol* formal = NewSymbol(SyntaxFakeName(&compiler->syntax), arg_type,
                             STO(auto));
  formal->flags.is_argument = true;
  formal->flags.invented = true;
  VectorAppend(&func->info.function.prototype, formal);

  Symbol* symbol = NewSymbol(name, func, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_defined = false;
  symbol->location = location;
  func->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);
  if (first == NULL) {
    bool added = InsertGlobalSymbol(symbol);
    assert(added);
    (void)added;
  } else {
    Symbol* tail = first;
    while (tail->overload_next != NULL) {
      tail = tail->overload_next;
    }
    symbol->namespace_ = first->namespace_;
    tail->overload_next = symbol;
    first->flags.is_overloaded = true;
    symbol->flags.is_overloaded = true;
    SymbolSetCXXMangledAsmName(first);
    SymbolSetCXXMangledAsmName(symbol);
  }
  return symbol;
}

static Symbol* GetCoroutineClassAllocationFunction(TypeRecord* promise_type,
                                                   const char* name,
                                                   size_t arg_count) {
  if (!TypeIsStructOrUnion(promise_type) ||
      promise_type->info.struct_info == NULL) {
    return NULL;
  }
  String member_name;
  StringInit(&member_name, name);
  StructMember* member =
      FindStructMember(promise_type->info.struct_info, &member_name);
  StringDestruct(&member_name);
  if (member == NULL || !member->is_member_function ||
      member->symbol == NULL || !TypeIsFunction(member->symbol->type)) {
    return NULL;
  }
  return FindCoroutineAllocationFunctionByArgCount(member->symbol, arg_count);
}

static Symbol* GetCoroutineOperatorNew(TypeRecord* promise_type,
                                       SourceLocation location) {
  Symbol* member = GetCoroutineClassAllocationFunction(
      promise_type, "operator new", 1);
  if (member != NULL) {
    return member;
  }
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  return GetCoroutineAllocationFunction("operator new", return_type,
                                        NewSizeTypeRecord(), location);
}

static Symbol* GetCoroutineOperatorDelete(TypeRecord* promise_type,
                                          SourceLocation location) {
  Symbol* member = GetCoroutineClassAllocationFunction(
      promise_type, "operator delete", 1);
  if (member != NULL) {
    return member;
  }
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* arg_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return GetCoroutineAllocationFunction("operator delete", void_type, arg_type,
                                        location);
}

static ASTNode* NewCoroutineFrameAllocation(TypeRecord* frame_type,
                                            TypeRecord* frame_pointer_type,
                                            TypeRecord* promise_type,
                                            SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_pointer = NewPointerTo(kQualPlain, void_type);
  Vector* actuals = NewVector();
  VectorAppend(actuals, NewSizeofASTNodeWithKnownSize(frame_type->size,
                                                      location));
  ASTNode* call = NewVectorASTNode(AST_OP(call), TypeRecordCopy(void_pointer),
                                   location,
                                   NewIdentifierASTNode(GetCoroutineOperatorNew(
                                                            promise_type,
                                                            location),
                                                        location),
                                   actuals);
  ASTNode* cast =
      NewCastASTNode(TypeRecordCopy(frame_pointer_type), location, call);
  ((CastASTNode*)cast)->kind = kCastStatic;
  return cast;
}

static ASTNode* NewCoroutineFrameDeallocation(CoroutineFrame* frame,
                                              SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_pointer = NewPointerTo(kQualPlain, void_type);
  ASTNode* frame_as_void =
      NewCastASTNode(TypeRecordCopy(void_pointer), location,
                     NewFrameAddress(frame, location));
  ((CastASTNode*)frame_as_void)->kind = kCastStatic;
  Vector* actuals = NewVector();
  VectorAppend(actuals, frame_as_void);
  ASTNode* call = NewVectorASTNode(
      AST_OP(call), TypeRecordCopy(void_type), location,
      NewIdentifierASTNode(GetCoroutineOperatorDelete(frame->promise_type,
                                                      location),
                           location),
      actuals);
  return NewExpressionStatementASTNode(call, location);
}

static ASTNode* NewVariableInitExpression(Symbol* symbol, ASTNode* initializer,
                                          SourceLocation location) {
  ASTNode* decl_id = NewIdentifierASTNode(symbol, location);
  decl_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  return NewBinaryASTNode(AST_OP(init), symbol->type, location, decl_id,
                          NewExpressionInitializerASTNode(initializer,
                                                          location));
}

static CoroutineFrame NewCoroutineFrame(TypeRecord* return_type,
                                        TypeRecord* promise_type,
                                        TypeRecord* initial_awaiter_type,
                                        SuspensionPoints* points,
                                        Vector* persisted_locals,
                                        TypeRecord* yield_awaiter_type,
                                        SourceLocation location) {
  TypeRecord* frame_type = NewTypeRecord(kTypeStruct, kQualPlain);
  Struct* str = NewStruct(false);
  frame_type->info.struct_info = str;

  CoroutineFrame frame = {0};
  VectorInit(&frame.owned_symbols);
  frame.state = AddCoroutineFrameIntMember(str, "__state");
  frame.done = AddCoroutineFrameIntMember(str, "__done");
  frame.resume = AddCoroutineFrameResumeMember(str, return_type, frame_type,
                                               location);
  frame.destroy = AddCoroutineFrameDestroyMember(str, frame_type, location);
  frame.promise = AddCoroutineFrameTypedMember(str, "__promise",
                                               promise_type);
  frame.promise_constructed =
      AddCoroutineFrameIntMember(str, "__promise_constructed");
  if (initial_awaiter_type != NULL) {
    frame.initial_awaiter =
        AddCoroutineFrameTypedMember(str, "__initial_awaiter",
                                     initial_awaiter_type);
    frame.initial_awaiter_constructed =
        AddCoroutineFrameIntMember(str, "__initial_awaiter_constructed");
  }
  for (size_t i = 0; persisted_locals != NULL &&
                     i < persisted_locals->length; i++) {
    CoroutinePersistedLocal* local = persisted_locals->value.p[i];
    if (local == NULL || local->symbol == NULL ||
        local->symbol->type == NULL) {
      continue;
    }
    char member_name[32];
    snprintf(member_name, sizeof(member_name), "__local%zu", i);
    local->member =
        AddCoroutineFrameTypedMember(str, member_name, local->symbol->type);
    if (TypeIsStructOrUnion(local->symbol->type)) {
      snprintf(member_name, sizeof(member_name), "__local%zu_constructed", i);
      local->constructed_member = AddCoroutineFrameIntMember(str, member_name);
    }
  }
  for (int i = 0; points != NULL && i < points->count; i++) {
    SuspensionPoint* point = &points->points[i];
    TypeRecord* awaiter_type = point->kind == kSuspensionCoYield
                                   ? yield_awaiter_type
                                   : point->awaiter->type;
    if (awaiter_type == NULL) {
      continue;
    }
    char member_name[32];
    snprintf(member_name, sizeof(member_name), "__awaiter%d", i);
    point->frame_member =
        AddCoroutineFrameTypedMember(str, member_name, awaiter_type);
    snprintf(member_name, sizeof(member_name), "__awaiter%d_constructed", i);
    point->frame_constructed_member =
        AddCoroutineFrameIntMember(str, member_name);
  }
  frame_type = TypeRecordCalculateSize(frame_type);
  frame.type = frame_type;
  frame.promise_type = promise_type;

  Symbol* tag = NewSymbol(SyntaxFakeName(&compiler->syntax),
                          TypeRecordCopy(frame_type), STO(implicit));
  tag->flags.invented = true;
  tag->flags.is_defined = true;
  tag->location = location;
  str->tag_symbol = tag;
  str->tag_name = &tag->name;
  bool added = SyntaxAddTag(&compiler->syntax, tag);
  assert(added);
  (void)added;

  TypeRecord* frame_pointer_type = NewCoroutineFramePointerType(frame_type);
  frame.symbol = SyntaxNewTemporary(&compiler->syntax, frame_pointer_type);
  frame.symbol->flags.is_local = true;
  frame.symbol->flags.is_defined = true;
  frame.symbol->location = location;
  frame.decl = (VariableDeclarationASTNode*)NewVariableDeclarationASTNode(
      frame.symbol,
      NewVariableInitExpression(
          frame.symbol,
          NewCoroutineFrameAllocation(frame_type, frame_pointer_type,
                                      promise_type, location),
          location),
      location);
  return frame;
}

static void QueueCoroutineGeneratedFunction(Symbol* symbol) {
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  VectorAppend(&compiler->pending_template_instantiations,
               NewDeclarationListASTNode(declarations, symbol->location));
  VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
}

typedef struct {
  Symbol* old_symbol;
  Symbol* new_symbol;
} SymbolReplacement;

static void ReplaceIdentifierSymbol(ASTNode* node, void* data, int child_id,
                                    VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  SymbolReplacement* replacement = data;
  IdentifierASTNode* identifier = (IdentifierASTNode*)node;
  if (identifier->symbol == replacement->old_symbol) {
    identifier->symbol = replacement->new_symbol;
    ASTNodeSetType(node, TypeRecordCopy(replacement->new_symbol->type));
  }
}

static bool IsFrameMemberAccessFor(ASTNode* node, StructMember* member) {
  if (node == NULL || node->op != AST_OP(arrow) || member == NULL) {
    return false;
  }
  BinaryASTNode* access = (BinaryASTNode*)node;
  if (access->right == NULL || access->right->op != AST_OP(structmember)) {
    return false;
  }
  return ((StructMemberASTNode*)access->right)->member == member;
}

static bool IsFrameCopyAssignmentRhs(ASTNode* node, StructMember* member) {
  if (node == NULL || node->parent == NULL || node->child_id != 1 ||
      node->parent->op != AST_OP(assign)) {
    return false;
  }
  BinaryASTNode* assign = (BinaryASTNode*)node->parent;
  return IsFrameMemberAccessFor(assign->left, member);
}

static bool ASTNodeIsAncestorOf(ASTNode* ancestor, ASTNode* node) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if (current == ancestor) {
      return true;
    }
  }
  return false;
}

static bool IsFrameConstructionActual(ASTNode* node, StructMember* member) {
  ASTNode* call_node = NULL;
  for (ASTNode* ancestor = node != NULL ? node->parent : NULL;
       ancestor != NULL; ancestor = ancestor->parent) {
    if (ancestor->op == AST_OP(call)) {
      call_node = ancestor;
      break;
    }
  }
  if (call_node == NULL) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)call_node;
  if (call->left == NULL || call->left->op != AST_OP(dot)) {
    return false;
  }
  if (ASTNodeIsAncestorOf(call->left, node)) {
    return false;
  }
  BinaryASTNode* member_call = (BinaryASTNode*)call->left;
  return IsFrameMemberAccessFor(member_call->left, member);
}

static bool IsInsideDeclarationOfSymbol(ASTNode* node, Symbol* symbol) {
  for (ASTNode* ancestor = node != NULL ? node->parent : NULL;
       ancestor != NULL; ancestor = ancestor->parent) {
    if (ancestor->op == AST_OP(vardecl) &&
        ((VariableDeclarationASTNode*)ancestor)->symbol == symbol) {
      return true;
    }
  }
  return false;
}

static void ReplaceFrameOwnedIdentifier(ASTNode* node, void* data,
                                        int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier) || node->parent == NULL ||
      (node->flags & kASTIsDeclaration)) {
    return;
  }
  CoroutineFrame* frame = data;
  IdentifierASTNode* identifier = (IdentifierASTNode*)node;
  if (IsInsideDeclarationOfSymbol(node, identifier->symbol)) {
    return;
  }
  StructMember* member =
      CoroutineFrameMemberForSymbol(frame, identifier->symbol);
  if (member == NULL || IsFrameCopyAssignmentRhs(node, member) ||
      IsFrameConstructionActual(node, member)) {
    return;
  }
  ASTNode* access = NewFrameMemberAccess(frame, member, node->location);
  ASTNodeReplaceChild(node->parent, node->child_id, access, true);
}

static void RewriteFrameOwnedSymbols(ASTNode* node, CoroutineFrame* frame) {
  if (node == NULL || frame == NULL || frame->owned_symbols.length == 0) {
    return;
  }
  ASTNodeVisit(node, ReplaceFrameOwnedIdentifier, 0, frame);
}

static void RemoveCoroutineFrameStores(CompoundStatementASTNode* compound) {
  if (compound == NULL || compound->statements == NULL) {
    return;
  }
  for (size_t i = compound->statements->length; i > 0; i--) {
    ASTNode* stmt = compound->statements->value.p[i - 1];
    if (stmt == NULL) {
      continue;
    }
    if ((stmt->flags & kASTCoroutineFrameStore) != 0) {
      VectorDeleteElement(compound->statements, i - 1);
      continue;
    }
    if (stmt->op == AST_OP(compound)) {
      RemoveCoroutineFrameStores((CompoundStatementASTNode*)stmt);
    } else if (stmt->op == AST_OP(if)) {
      IfStatementASTNode* if_stmt = (IfStatementASTNode*)stmt;
      if (if_stmt->if_part != NULL &&
          if_stmt->if_part->op == AST_OP(compound)) {
        RemoveCoroutineFrameStores((CompoundStatementASTNode*)if_stmt->if_part);
      }
      if (if_stmt->else_part != NULL &&
          if_stmt->else_part->op == AST_OP(compound)) {
        RemoveCoroutineFrameStores(
            (CompoundStatementASTNode*)if_stmt->else_part);
      }
    } else if (stmt->op == AST_OP(while) || stmt->op == AST_OP(do)) {
      CombinedStatementASTNode* loop = (CombinedStatementASTNode*)stmt;
      if (loop->stmt != NULL && loop->stmt->op == AST_OP(compound)) {
        RemoveCoroutineFrameStores((CompoundStatementASTNode*)loop->stmt);
      }
    } else if (stmt->op == AST_OP(for)) {
      ForStatementASTNode* loop = (ForStatementASTNode*)stmt;
      if (loop->stmt != NULL && loop->stmt->op == AST_OP(compound)) {
        RemoveCoroutineFrameStores((CompoundStatementASTNode*)loop->stmt);
      }
    }
  }
  ResetCompoundStatementParents(compound);
}

static Symbol* NewCoroutineResumeFunction(ASTNode* node,
                                          CoroutineFrame* frame,
                                          CompoundStatementASTNode* body,
                                          TypeRecord* return_type,
                                          int suspend_count,
                                          Symbol* initial_awaiter,
                                          SourceLocation location) {
  char name[96];
  snprintf(name, sizeof(name), "%s_coroutine_resume",
           SyntaxFakeName(&compiler->syntax));
  Symbol* frame_param = NULL;
  TypeRecord* func = NewCoroutineResumeFunctionType(return_type, frame->type,
                                                    &frame_param, location);
  ASTNode* cloned = ASTNodeClone((ASTNode*)body, IdentityCloneNode, NULL, NULL);
  CompoundStatementASTNode* resume_body = (CompoundStatementASTNode*)cloned;
  if (resume_body->statements->length > 0) {
    ASTNode* first = resume_body->statements->value.p[0];
    if (first != NULL && first->op == AST_OP(decl_list)) {
      VectorDeleteElement(resume_body->statements, 0);
    }
  }
  RemoveCoroutineFrameStores(resume_body);
  if (initial_awaiter != NULL) {
    VectorInsertBefore(
        resume_body->statements, (size_t)suspend_count + 1,
        NewAwaitResumeStatement(initial_awaiter, location));
    ResetCompoundStatementParents(resume_body);
  }
  SymbolReplacement replacement = {frame->symbol, frame_param};
  ASTNodeVisit((ASTNode*)resume_body, ReplaceIdentifierSymbol, 0,
               &replacement);
  CoroutineFrame resume_frame = *frame;
  resume_frame.symbol = frame_param;
  RewriteFrameOwnedSymbols((ASTNode*)resume_body, &resume_frame);
  func->info.function.body = (ASTNode*)resume_body;
  Symbol* symbol = NewSymbol(name, func, STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = location;
  symbol->value.func_defn = symbol;
  func->info.function.symbol = symbol;
  bool added = InsertGlobalSymbol(symbol);
  assert(added);
  (void)added;
  QueueCoroutineGeneratedFunction(symbol);
  (void)node;
  return symbol;
}

static ASTNode* NewNullPointerConstant(SourceLocation location) {
  return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

static Symbol* NewCoroutineDestroyFunction(CoroutineFrame* frame,
                                           SourceLocation location) {
  char name[96];
  snprintf(name, sizeof(name), "%s_coroutine_destroy",
           SyntaxFakeName(&compiler->syntax));
  Symbol* frame_param = NULL;
  TypeRecord* func = NewCoroutineDestroyFunctionType(frame->type, &frame_param,
                                                     location);
  CoroutineFrame destroy_frame = *frame;
  destroy_frame.symbol = frame_param;
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewFrameIntAssignment(&destroy_frame, frame->state, 0,
                                     location));
  VectorAppend(statements,
               NewFrameIntAssignment(&destroy_frame, frame->done, 1,
                                     location));
  VectorAppend(statements,
               NewFrameAssignment(&destroy_frame, frame->resume,
                                  NewNullPointerConstant(location), location));
  VectorAppend(statements,
               NewFrameAssignment(&destroy_frame, frame->destroy,
                                  NewNullPointerConstant(location), location));
  AppendCoroutineFrameDestructors(statements, &destroy_frame, location);
  VectorAppend(statements,
               NewCoroutineFrameDeallocation(&destroy_frame, location));
  func->info.function.body = NewCompoundStatementASTNode(statements, location);
  Symbol* symbol = NewSymbol(name, func, STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->location = location;
  symbol->value.func_defn = symbol;
  func->info.function.symbol = symbol;
  bool added = InsertGlobalSymbol(symbol);
  assert(added);
  (void)added;
  QueueCoroutineGeneratedFunction(symbol);
  return symbol;
}

static ASTNode* NewStateResumeIf(CoroutineFrame* frame, int state_value,
                                 LabelASTNode* label,
                                 SourceLocation location) {
  ASTNode* condition =
      NewBinaryASTNode(AST_OP(equal), NULL, location,
                       NewFrameMemberAccess(frame, frame->state, location),
                       NewIntConstantASTNode(
                           state_value,
                           NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  ASTNode* goto_stmt =
      NewGotoStatementASTNode(NewString(label->name.value), location);
  return NewIfStatementASTNode(condition, goto_stmt, NULL, false, location);
}

static ASTNode* NewSuspendIf(CoroutineFrame* frame, Symbol* promise,
                             Symbol* awaiter,
                             int state_value, TypeRecord* return_type,
                             SourceLocation location) {
  ASTNode* ready_call =
      NewAwaiterMemberCall(NewIdentifierASTNode(awaiter, location),
                           "await_ready", location);
  ASTNode* not_ready =
      NewBinaryASTNode(AST_OP(equal), NewTypeRecordWithSize(kTypeBool,
                                                            kQualPlain),
                       location, ready_call,
                       NewIntConstantASTNode(
                           0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  Vector* suspend_statements = NewVector();
  VectorAppend(suspend_statements,
               NewFrameIntAssignment(frame, frame->state, state_value,
                                     location));
  Vector* suspend_actuals = NewVector();
  VectorAppend(suspend_actuals, NewFrameAddress(frame, location));
  ASTNode* await_suspend = NewAwaiterMemberCallWithActuals(
      NewIdentifierASTNode(awaiter, location), "await_suspend",
      suspend_actuals, location);
  TypeRecord* await_suspend_return = AwaiterAwaitSuspendReturnType(awaiter);
  if (await_suspend_return != NULL && TypeIsBool(await_suspend_return)) {
    VectorAppend(suspend_statements,
                 NewIfStatementASTNode(
                     await_suspend,
                     NewCoroutineReturnObjectStatement(promise, return_type,
                                                       location),
                     NULL, false, location));
  } else {
    VectorAppend(suspend_statements,
                 NewExpressionStatementASTNode(await_suspend, location));
    VectorAppend(suspend_statements,
                 NewCoroutineReturnObjectStatement(promise, return_type,
                                                   location));
  }
  return NewIfStatementASTNode(
      not_ready, NewCompoundStatementASTNode(suspend_statements, location),
      NULL, false, location);
}

static ASTNode* NewAwaitResumeInitializer(Symbol* awaiter,
                                          TypeRecord* result_type,
                                          SourceLocation location) {
  ASTNode* await_resume =
      NewAwaiterMemberCall(NewIdentifierASTNode(awaiter, location),
                           "await_resume", location);
  ASTNodeSetType(await_resume, result_type);
  return NewExpressionInitializerASTNode(await_resume, location);
}

static ASTNode* NewAwaitResumeStatement(Symbol* awaiter,
                                        SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewAwaiterMemberCall(NewIdentifierASTNode(awaiter, location),
                           "await_resume", location),
      location);
}

static ASTNode* NewInitialSuspendInitialization(CoroutineFrame* frame,
                                                Symbol* promise,
                                                SourceLocation location) {
  ASTNode* initial_suspend = NewCoroutinePromiseMemberCall(
      promise, "initial_suspend", NewVector(), location);
  ASTNodeSetType(initial_suspend,
                 TypeRecordCopy(frame->initial_awaiter->symbol->type));
  return NewFrameMemberInitialization(
      frame, frame->initial_awaiter, frame->initial_awaiter_constructed,
      initial_suspend, location);
}

static ASTNode* NewInitialSuspendIf(CoroutineFrame* frame, Symbol* promise,
                                    Symbol* awaiter, TypeRecord* return_type,
                                    SourceLocation location) {
  ASTNode* ready_call =
      NewAwaiterMemberCall(NewIdentifierASTNode(awaiter, location),
                           "await_ready", location);
  ASTNode* not_ready =
      NewBinaryASTNode(AST_OP(equal),
                       NewTypeRecordWithSize(kTypeBool, kQualPlain), location,
                       ready_call,
                       NewIntConstantASTNode(
                           0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  Vector* suspend_statements = NewVector();
  VectorAppend(suspend_statements,
               NewFrameIntAssignment(frame, frame->state, 0, location));
  Vector* suspend_actuals = NewVector();
  VectorAppend(suspend_actuals, NewFrameAddress(frame, location));
  ASTNode* await_suspend = NewAwaiterMemberCallWithActuals(
      NewIdentifierASTNode(awaiter, location), "await_suspend",
      suspend_actuals, location);
  TypeRecord* await_suspend_return = AwaiterAwaitSuspendReturnType(awaiter);
  if (await_suspend_return != NULL && TypeIsBool(await_suspend_return)) {
    VectorAppend(suspend_statements,
                 NewIfStatementASTNode(
                     await_suspend,
                     NewCoroutineReturnObjectStatement(promise, return_type,
                                                       location),
                     NULL, false, location));
  } else {
    VectorAppend(suspend_statements,
                 NewExpressionStatementASTNode(await_suspend, location));
    VectorAppend(suspend_statements,
                 NewCoroutineReturnObjectStatement(promise, return_type,
                                                   location));
  }
  return NewIfStatementASTNode(
      not_ready, NewCompoundStatementASTNode(suspend_statements, location),
      NULL, false, location);
}

static void ResetStatementParents(ASTNode* node);

static void CoroutineCompoundInsertStatement(CompoundStatementASTNode* compound,
                                             ASTNode* stmt,
                                             size_t at_index) {
  if (compound == NULL || stmt == NULL || compound->statements == NULL) {
    return;
  }
  if (at_index >= compound->statements->length) {
    VectorAppend(compound->statements, stmt);
    stmt->parent = (ASTNode*)compound;
    stmt->child_id = (int)(compound->statements->length - 1);
    return;
  }
  CompoundASTNodeInsertStatement(compound, stmt, at_index);
}

static void ResetCompoundStatementParents(CompoundStatementASTNode* compound) {
  if (compound == NULL) {
    return;
  }
  for (size_t i = 0; i < compound->statements->length; i++) {
    ASTNode* stmt = compound->statements->value.p[i];
    if (stmt != NULL) {
      stmt->parent = (ASTNode*)compound;
      stmt->child_id = (int)i;
      ResetStatementParents(stmt);
    }
  }
}

static void ResetStatementParents(ASTNode* node) {
  if (node == NULL) {
    return;
  }
  if (node->op == AST_OP(compound)) {
    ResetCompoundStatementParents((CompoundStatementASTNode*)node);
    return;
  }
  if (node->op == AST_OP(if)) {
    IfStatementASTNode* if_stmt = (IfStatementASTNode*)node;
    if (if_stmt->if_part != NULL) {
      if_stmt->if_part->parent = node;
      if_stmt->if_part->child_id = 1;
      ResetStatementParents(if_stmt->if_part);
    }
    if (if_stmt->else_part != NULL) {
      if_stmt->else_part->parent = node;
      if_stmt->else_part->child_id = 2;
      ResetStatementParents(if_stmt->else_part);
    }
    return;
  }
  if (node->op == AST_OP(while) || node->op == AST_OP(do)) {
    CombinedStatementASTNode* loop = (CombinedStatementASTNode*)node;
    if (loop->stmt != NULL) {
      loop->stmt->parent = node;
      loop->stmt->child_id = 1;
      ResetStatementParents(loop->stmt);
    }
    return;
  }
  if (node->op == AST_OP(for)) {
    ForStatementASTNode* loop = (ForStatementASTNode*)node;
    if (loop->stmt != NULL) {
      loop->stmt->parent = node;
      loop->stmt->child_id = 3;
      ResetStatementParents(loop->stmt);
    }
  }
}

static void CollectSuspensionPointsInCompound(CompoundStatementASTNode* body,
                                              SuspensionPoints* points) {
  for (size_t i = 0; i < body->statements->length; i++) {
    ASTNode* stmt = body->statements->value.p[i];
    if (stmt == NULL) {
      continue;
    }
    if (stmt->op == AST_OP(expr)) {
      ASTNode* expr = ((ExpressionStatementASTNode*)stmt)->expr;
      if (expr != NULL && expr->op == AST_OP(co_yield)) {
        if (points->count >= points->capacity) {
          continue;
        }
        SuspensionPoint* point = &points->points[points->count++];
        point->kind = kSuspensionCoYield;
        point->co_yield = expr;
        point->statement = stmt;
        point->compound = body;
        point->statement_index = i;
      }
      continue;
    }
    if (stmt->op == AST_OP(compound)) {
      CollectSuspensionPointsInCompound((CompoundStatementASTNode*)stmt,
                                        points);
      continue;
    }
    if (stmt->op == AST_OP(if)) {
      IfStatementASTNode* if_stmt = (IfStatementASTNode*)stmt;
      if (if_stmt->if_part != NULL &&
          if_stmt->if_part->op == AST_OP(compound)) {
        CollectSuspensionPointsInCompound(
            (CompoundStatementASTNode*)if_stmt->if_part, points);
      }
      if (if_stmt->else_part != NULL &&
          if_stmt->else_part->op == AST_OP(compound)) {
        CollectSuspensionPointsInCompound(
            (CompoundStatementASTNode*)if_stmt->else_part, points);
      }
      continue;
    }
    if (stmt->op == AST_OP(while) || stmt->op == AST_OP(do)) {
      CombinedStatementASTNode* loop = (CombinedStatementASTNode*)stmt;
      if (loop->stmt != NULL && loop->stmt->op == AST_OP(compound)) {
        CollectSuspensionPointsInCompound(
            (CompoundStatementASTNode*)loop->stmt, points);
      }
      continue;
    }
    if (stmt->op == AST_OP(for)) {
      ForStatementASTNode* loop = (ForStatementASTNode*)stmt;
      if (loop->stmt != NULL && loop->stmt->op == AST_OP(compound)) {
        CollectSuspensionPointsInCompound(
            (CompoundStatementASTNode*)loop->stmt, points);
      }
      continue;
    }
    if (stmt->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decl_list = (DeclarationListASTNode*)stmt;
    for (size_t j = 0; j < decl_list->declarations->length; j++) {
      VariableDeclarationASTNode* decl = decl_list->declarations->value.p[j];
      if (decl == NULL) {
        continue;
      }
      ASTNode* expr = VariableInitializerExpression(decl->initializer);
      if (expr == NULL || expr->op != AST_OP(co_await) ||
          CoAwaitIsAlwaysReady(expr)) {
        continue;
      }
      if (points->count >= points->capacity) {
        continue;
      }
      SuspensionPoint* point = &points->points[points->count++];
      point->kind = kSuspensionCoAwait;
      point->co_await = expr;
      point->value_decl = decl;
      point->decl_list = decl_list;
      point->compound = body;
      point->statement_index = i;
    }
  }
}

typedef struct {
  Symbol* symbol;
  bool found;
} SymbolUseSearch;

static void FindSymbolUse(ASTNode* node, void* data, int child_id,
                          VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  SymbolUseSearch* search = data;
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if ((node->flags & kASTIsDeclaration) == 0 && id->symbol == search->symbol) {
    search->found = true;
  }
}

static bool ASTUsesSymbol(ASTNode* node, Symbol* symbol) {
  if (node == NULL || symbol == NULL) {
    return false;
  }
  SymbolUseSearch search = {symbol, false};
  ASTNodeVisit(node, FindSymbolUse, 0, &search);
  return search.found;
}

static bool SymbolVectorContains(Vector* symbols, Symbol* symbol) {
  if (symbols == NULL || symbol == NULL) {
    return false;
  }
  for (size_t i = 0; i < symbols->length; i++) {
    if (symbols->value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

static bool PersistedLocalVectorContains(Vector* locals, Symbol* symbol) {
  if (locals == NULL || symbol == NULL) {
    return false;
  }
  for (size_t i = 0; i < locals->length; i++) {
    CoroutinePersistedLocal* local = locals->value.p[i];
    if (local != NULL && local->symbol == symbol) {
      return true;
    }
  }
  return false;
}

static bool CoroutineScalarCanBePersisted(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL ||
      TypeIsStructOrUnion(symbol->type) || TypeIsArray(symbol->type) ||
      TypeIsFunction(symbol->type) || TypeIsReference(symbol->type)) {
    return false;
  }
  return true;
}

static bool CoroutineParameterCanBePersisted(Symbol* symbol,
                                             bool* move_parameter) {
  if (move_parameter != NULL) {
    *move_parameter = false;
  }
  if (CoroutineScalarCanBePersisted(symbol)) {
    return true;
  }
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsStructOrUnion(symbol->type)) {
    return false;
  }
  if (CoroutineTypeCanBeMoveConstructed(symbol->type)) {
    if (move_parameter != NULL) {
      *move_parameter = true;
    }
    return true;
  }
  return CoroutineTypeCanBeCopyConstructed(symbol->type);
}

static bool CoroutineLocalCanBePersisted(Symbol* symbol) {
  if (CoroutineScalarCanBePersisted(symbol)) {
    return true;
  }
  return symbol != NULL && symbol->type != NULL &&
         TypeIsStructOrUnion(symbol->type) &&
         CoroutineTypeCanBeCopyConstructed(symbol->type);
}

static bool StatementRangeUsesSymbol(CompoundStatementASTNode* body,
                                     size_t first_index,
                                     Symbol* symbol) {
  if (body == NULL || body->statements == NULL) {
    return false;
  }
  for (size_t i = first_index; i < body->statements->length; i++) {
    if (ASTUsesSymbol(body->statements->value.p[i], symbol)) {
      return true;
    }
  }
  return false;
}

static bool SuspensionPointUsesSymbolAfter(CompoundStatementASTNode* root,
                                           SuspensionPoint* point,
                                           Symbol* symbol) {
  if (root == NULL || point == NULL || point->compound == NULL) {
    return false;
  }
  if (StatementRangeUsesSymbol(point->compound, point->statement_index + 1,
                               symbol)) {
    return true;
  }
  ASTNode* current = (ASTNode*)point->compound;
  while (current != NULL && current != (ASTNode*)root) {
    ASTNode* parent = current->parent;
    if (parent == NULL) {
      break;
    }
    if ((parent->op == AST_OP(while) || parent->op == AST_OP(do)) &&
        ((CombinedStatementASTNode*)parent)->stmt == current &&
        ASTUsesSymbol(((CombinedStatementASTNode*)parent)->cond, symbol)) {
      return true;
    }
    if (parent->op == AST_OP(for) &&
        ((ForStatementASTNode*)parent)->stmt == current) {
      ForStatementASTNode* loop = (ForStatementASTNode*)parent;
      if (ASTUsesSymbol(loop->c2, symbol) ||
          ASTUsesSymbol(loop->c3, symbol)) {
        return true;
      }
    }
    if (parent->op == AST_OP(compound) &&
        StatementRangeUsesSymbol((CompoundStatementASTNode*)parent,
                                 (size_t)current->child_id + 1, symbol)) {
      return true;
    }
    current = parent;
  }
  return false;
}

static void CollectCoroutineAwaiterSymbols(SuspensionPoints* points,
                                           Vector* awaiters) {
  VectorInit(awaiters);
  for (int i = 0; points != NULL && i < points->count; i++) {
    if (points->points[i].awaiter != NULL &&
        !SymbolVectorContains(awaiters, points->points[i].awaiter)) {
      VectorAppend(awaiters, points->points[i].awaiter);
    }
  }
}

static void AddPersistedCoroutineLocal(Vector* persisted_locals,
                                       Symbol* symbol,
                                       bool is_parameter,
                                       bool move_parameter) {
  if (PersistedLocalVectorContains(persisted_locals, symbol)) {
    return;
  }
  CoroutinePersistedLocal* local = malloc(sizeof(CoroutinePersistedLocal));
  assert(local != NULL);
  local->symbol = symbol;
  local->member = NULL;
  local->constructed_member = NULL;
  local->is_parameter = is_parameter;
  local->move_parameter = move_parameter;
  VectorAppend(persisted_locals, local);
}

static void CollectPersistedCoroutineLocalsBeforeIndex(
    CompoundStatementASTNode* root,
    CompoundStatementASTNode* compound,
    size_t limit,
    SuspensionPoint* point,
    Vector* awaiters,
    Vector* persisted_locals,
    bool* ok) {
  if (compound == NULL || point == NULL || persisted_locals == NULL ||
      ok == NULL) {
    return;
  }
  for (size_t stmt_index = 0; stmt_index < limit; stmt_index++) {
    ASTNode* stmt = compound->statements->value.p[stmt_index];
    if (stmt == NULL || stmt->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decls = (DeclarationListASTNode*)stmt;
    for (size_t decl_index = 0; decl_index < decls->declarations->length;
         decl_index++) {
      VariableDeclarationASTNode* decl =
          decls->declarations->value.p[decl_index];
      Symbol* symbol = decl != NULL ? decl->symbol : NULL;
      if (SymbolVectorContains(awaiters, symbol)) {
        continue;
      }
      if (!SuspensionPointUsesSymbolAfter(root, point, symbol)) {
        continue;
      }
      if (!CoroutineLocalCanBePersisted(symbol)) {
        SemanticError(
            (ASTNode*)decl,
            "coroutine local live across suspension is not supported yet");
        *ok = false;
        continue;
      }
      AddPersistedCoroutineLocal(persisted_locals, symbol, false, false);
    }
  }
}

static void CollectPersistedCoroutineLocalsForPoint(
    CompoundStatementASTNode* root,
    SuspensionPoint* point,
    Vector* awaiters,
    Vector* persisted_locals,
    bool* ok) {
  if (root == NULL || point == NULL || point->compound == NULL) {
    return;
  }
  CollectPersistedCoroutineLocalsBeforeIndex(
      root, point->compound, point->statement_index, point, awaiters,
      persisted_locals, ok);
  ASTNode* current = (ASTNode*)point->compound;
  while (current != NULL && current != (ASTNode*)root) {
    ASTNode* parent = current->parent;
    if (parent == NULL) {
      break;
    }
    if (parent->op == AST_OP(compound)) {
      CollectPersistedCoroutineLocalsBeforeIndex(
          root, (CompoundStatementASTNode*)parent, (size_t)current->child_id,
          point, awaiters, persisted_locals, ok);
    }
    current = parent;
  }
}

static bool CollectPersistedCoroutineLocals(
    CompoundStatementASTNode* body,
    SuspensionPoints* points,
    Vector* persisted_locals) {
  VectorInit(persisted_locals);
  if (body == NULL || points == NULL || points->count == 0) {
    return true;
  }
  Vector awaiters;
  CollectCoroutineAwaiterSymbols(points, &awaiters);
  bool ok = true;
  for (int i = 0; i < points->count; i++) {
    CollectPersistedCoroutineLocalsForPoint(body, &points->points[i],
                                            &awaiters, persisted_locals, &ok);
  }
  VectorDestruct(&awaiters);
  return ok;
}

static bool CollectPersistedCoroutineParameters(
    FunctionInfo* info,
    CompoundStatementASTNode* body,
    SuspensionPoints* points,
    Vector* persisted_locals) {
  if (info == NULL || body == NULL || points == NULL ||
      points->count == 0 || persisted_locals == NULL) {
    return true;
  }
  bool ok = true;
  for (size_t i = 0; i < info->prototype.length; i++) {
    Symbol* symbol = info->prototype.value.p[i];
    if (!ASTUsesSymbol((ASTNode*)body, symbol)) {
      continue;
    }
    bool move_parameter = false;
    if (!CoroutineParameterCanBePersisted(symbol, &move_parameter)) {
      SemanticError(
          (ASTNode*)body,
          "coroutine parameter live across suspension is not supported yet");
      ok = false;
      continue;
    }
    AddPersistedCoroutineLocal(persisted_locals, symbol, true,
                               move_parameter);
  }
  return ok;
}

static void PersistedCoroutineLocalsDestruct(Vector* persisted_locals) {
  if (persisted_locals == NULL) {
    return;
  }
  for (size_t i = 0; i < persisted_locals->length; i++) {
    free(persisted_locals->value.p[i]);
  }
  VectorDestruct(persisted_locals);
}

static CoroutinePersistedLocal* FindPersistedCoroutineLocal(Vector* locals,
                                                            Symbol* symbol) {
  if (locals == NULL || symbol == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < locals->length; i++) {
    CoroutinePersistedLocal* local = locals->value.p[i];
    if (local != NULL && local->symbol == symbol) {
      return local;
    }
  }
  return NULL;
}

static void AdjustSuspensionPointIndicesAfterInsert(SuspensionPoints* points,
                                                    CompoundStatementASTNode* compound,
                                                    size_t insert_index) {
  for (int i = 0; points != NULL && i < points->count; i++) {
    if (points->points[i].compound == compound &&
        points->points[i].statement_index >= insert_index) {
      points->points[i].statement_index++;
    }
  }
}

static void InsertPersistedCoroutineLocalStoresInCompound(
    CompoundStatementASTNode* compound,
    CoroutineFrame* frame,
    Vector* persisted_locals,
    SuspensionPoints* points) {
  if (compound == NULL || frame == NULL || persisted_locals == NULL ||
      persisted_locals->length == 0) {
    return;
  }
  for (size_t i = 0; i < compound->statements->length; i++) {
    ASTNode* stmt = compound->statements->value.p[i];
    if (stmt != NULL && stmt->op == AST_OP(compound)) {
      InsertPersistedCoroutineLocalStoresInCompound(
          (CompoundStatementASTNode*)stmt, frame, persisted_locals, points);
      continue;
    }
    if (stmt != NULL && stmt->op == AST_OP(if)) {
      IfStatementASTNode* if_stmt = (IfStatementASTNode*)stmt;
      if (if_stmt->if_part != NULL &&
          if_stmt->if_part->op == AST_OP(compound)) {
        InsertPersistedCoroutineLocalStoresInCompound(
            (CompoundStatementASTNode*)if_stmt->if_part, frame,
            persisted_locals, points);
      }
      if (if_stmt->else_part != NULL &&
          if_stmt->else_part->op == AST_OP(compound)) {
        InsertPersistedCoroutineLocalStoresInCompound(
            (CompoundStatementASTNode*)if_stmt->else_part, frame,
            persisted_locals, points);
      }
      continue;
    }
    if (stmt != NULL && (stmt->op == AST_OP(while) ||
                         stmt->op == AST_OP(do))) {
      CombinedStatementASTNode* loop = (CombinedStatementASTNode*)stmt;
      if (loop->stmt != NULL && loop->stmt->op == AST_OP(compound)) {
        InsertPersistedCoroutineLocalStoresInCompound(
            (CompoundStatementASTNode*)loop->stmt, frame, persisted_locals,
            points);
      }
      continue;
    }
    if (stmt != NULL && stmt->op == AST_OP(for)) {
      ForStatementASTNode* loop = (ForStatementASTNode*)stmt;
      if (loop->stmt != NULL && loop->stmt->op == AST_OP(compound)) {
        InsertPersistedCoroutineLocalStoresInCompound(
            (CompoundStatementASTNode*)loop->stmt, frame, persisted_locals,
            points);
      }
      continue;
    }
    if (stmt == NULL || stmt->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* decls = (DeclarationListASTNode*)stmt;
    size_t inserted = 0;
    for (size_t decl_index = 0; decl_index < decls->declarations->length;
         decl_index++) {
      VariableDeclarationASTNode* decl =
          decls->declarations->value.p[decl_index];
      CoroutinePersistedLocal* local = FindPersistedCoroutineLocal(
          persisted_locals, decl != NULL ? decl->symbol : NULL);
      if (local == NULL || local->member == NULL || local->is_parameter) {
        continue;
      }
      size_t insert_index = i + 1 + inserted;
      ASTNode* value = NewIdentifierASTNode(local->symbol, decl->base.location);
      ASTNode* store = TypeIsStructOrUnion(local->symbol->type)
                           ? NewFrameMemberInitialization(
                                 frame, local->member,
                                 local->constructed_member, value,
                                 decl->base.location)
                           : NewFrameAssignment(frame, local->member, value,
                                                decl->base.location);
      CompoundASTNodeInsertStatement(
          compound, store, insert_index);
      AdjustSuspensionPointIndicesAfterInsert(points, compound, insert_index);
      inserted++;
    }
    i += inserted;
  }
}

static void InsertPersistedCoroutineLocalStores(
    CompoundStatementASTNode* body,
    CoroutineFrame* frame,
    Vector* persisted_locals,
    SuspensionPoints* points) {
  InsertPersistedCoroutineLocalStoresInCompound(body, frame, persisted_locals,
                                               points);
}

static void InsertPersistedCoroutineParameterStores(
    CompoundStatementASTNode* body,
    CoroutineFrame* frame,
    Vector* persisted_locals,
    SuspensionPoints* points) {
  if (body == NULL || frame == NULL || persisted_locals == NULL ||
      persisted_locals->length == 0) {
    return;
  }
  size_t insert_index = 1;
  for (size_t i = 0; i < persisted_locals->length; i++) {
    CoroutinePersistedLocal* local = persisted_locals->value.p[i];
    if (local == NULL || local->member == NULL || !local->is_parameter) {
      continue;
    }
    ASTNode* value = local->move_parameter
                         ? NewCoroutineMoveExpression(local->symbol,
                                                      local->symbol->location)
                         : NewIdentifierASTNode(local->symbol,
                                                local->symbol->location);
    ASTNode* store = TypeIsStructOrUnion(local->symbol->type)
                         ? NewFrameMemberInitialization(
                               frame, local->member,
                               local->constructed_member, value,
                               local->symbol->location)
                         : NewFrameAssignment(frame, local->member, value,
                                              local->symbol->location);
    store->flags |= kASTCoroutineFrameStore;
    CompoundASTNodeInsertStatement(
        body, store, insert_index);
    AdjustSuspensionPointIndicesAfterInsert(points, body, insert_index);
    insert_index++;
  }
}

static bool ValidateSuspensionPoint(ASTNode* node, CompoundStatementASTNode* body,
                                    SuspensionPoint* point) {
  (void)body;
  if (point->kind == kSuspensionCoYield) {
    if (point->co_yield == NULL || point->statement == NULL) {
      SemanticError(node, "unsupported coroutine suspension form");
      return false;
    }
    return true;
  }

  if (point->co_await == NULL || point->value_decl == NULL ||
      point->decl_list == NULL) {
    SemanticError(node, "unsupported coroutine suspension form");
    return false;
  }
  if (point->decl_list->declarations->length != 1) {
    SemanticError(point->co_await,
                  "suspending co_await declaration must be isolated");
    return false;
  }
  Symbol* awaitable_temp = NULL;
  if (!ApplyMemberOperatorCoAwait((UnaryASTNode*)point->co_await,
                                  &awaitable_temp)) {
    ApplyFreeOperatorCoAwait((UnaryASTNode*)point->co_await,
                             &awaitable_temp);
  }
  point->awaitable_temp = awaitable_temp;
  Symbol* awaiter = CoAwaitIdentifierOperand(point->co_await);
  if (awaiter == NULL) {
    TypeRecord* awaiter_type = CoAwaitOperandType(point->co_await);
    if (awaiter_type == NULL) {
      SemanticError(point->co_await,
                    "coroutine co_await operand type is invalid");
      return false;
    }
    if (!ValidateCoAwaiterType(point->co_await, awaiter_type)) {
      return false;
    }
    awaiter = NewCoroutineAwaiterTemporary(awaiter_type,
                                           point->co_await->location);
    if (awaiter == NULL) {
      SemanticError(point->co_await,
                    "coroutine co_await operand type is invalid");
      return false;
    }
    point->awaiter = awaiter;
    point->awaiter_init =
        ASTNodeMove(((UnaryASTNode*)point->co_await)->sub);
    return true;
  }
  point->awaiter = awaiter;
  if (!ValidateCoAwaiterType(point->co_await, awaiter->type)) {
    return false;
  }
  point->awaiter_decl = FindAwaiterDeclarationBefore(point, awaiter);
  if (point->awaiter_decl == NULL) {
    SemanticError(point->co_await,
                  "suspending co_await currently requires a prior awaiter "
                  "declaration");
    return false;
  }
  return true;
}

static bool LowerSuspendingCoAwaitFunction(ASTNode* node,
                                           CoroutineScan scan,
                                           SuspensionPoints* points,
                                           TypeRecord* return_type,
                                           Symbol* promise,
                                           CoroutineFrame* frame,
                                           Symbol* initial_awaiter,
                                           TypeRecord* yield_awaiter_type) {
  CompoundStatementASTNode* body =
      (CompoundStatementASTNode*)node->type->info.function.body;

  LabelASTNode** labels = NULL;
  if (points->count > 0) {
    labels = calloc((size_t)points->count, sizeof(LabelASTNode*));
  }
  if (points->count > 0 && labels == NULL) {
    SemanticError(node, "failed to allocate coroutine resume labels");
    return false;
  }
  for (int i = 0; i < points->count; i++) {
    SourceLocation location = points->points[i].kind == kSuspensionCoYield
                                  ? points->points[i].co_yield->location
                                  : points->points[i].co_await->location;
    String label_name;
    StringInit(&label_name, SyntaxFakeName(&compiler->syntax));
    labels[i] =
        (LabelASTNode*)NewLabelASTNode(label_name.value, NULL, false, location);
    StringDestruct(&label_name);

    if (points->points[i].kind == kSuspensionCoYield) {
      if (!LowerCoYieldStatement(&points->points[i], promise, frame,
                                 yield_awaiter_type)) {
        free(labels);
        return false;
      }
    } else {
      ReplaceVariableInitializerExpression(
          points->points[i].value_decl,
          UnwrapExpressionInitializer(NewAwaitResumeInitializer(
              points->points[i].awaiter,
              points->points[i].value_decl->symbol->type, location)));
    }
  }

  for (int i = points->count - 1; i >= 0; i--) {
    SuspensionPoint* point = &points->points[i];
    SourceLocation location = point->kind == kSuspensionCoYield
                                  ? point->co_yield->location
                                  : point->co_await->location;
    Symbol* awaiter = point->awaiter;
    ASTNode* suspend_if =
        NewSuspendIf(frame, promise, awaiter, i + 1, return_type, location);
    ASTNode* reset_state =
        NewFrameIntAssignment(frame, frame->state, 0, location);
    if (point->kind == kSuspensionCoYield) {
      CoroutineCompoundInsertStatement(point->compound, suspend_if,
                                       point->statement_index + 1);
      CoroutineCompoundInsertStatement(point->compound, (ASTNode*)labels[i],
                                       point->statement_index + 2);
      CoroutineCompoundInsertStatement(point->compound, reset_state,
                                       point->statement_index + 3);
      CoroutineCompoundInsertStatement(
          point->compound, NewAwaitResumeStatement(awaiter, location),
          point->statement_index + 4);
    } else {
      ASTNode* awaiter_init = NULL;
      if (point->awaiter_init != NULL) {
        Vector* constructor_actuals = TakeCXXTemporaryConstructionActuals(
            point->awaiter_init, point->frame_member->symbol->type);
        if (constructor_actuals != NULL) {
          awaiter_init = NewFrameMemberConstructorInitialization(
              frame, point->frame_member, point->frame_constructed_member,
              constructor_actuals, location);
        }
      }
      ASTNode* awaiter_value =
          point->awaiter_init != NULL ? point->awaiter_init
                                      : NewIdentifierASTNode(awaiter, location);
      if (awaiter_init == NULL) {
        awaiter_init = NewFrameMemberInitialization(
            frame, point->frame_member, point->frame_constructed_member,
            awaiter_value, location);
      }
      CoroutineCompoundInsertStatement(
          point->compound, awaiter_init, point->statement_index);
      size_t next_index = point->statement_index + 1;
      ASTNode* awaitable_dtor = NewCoroutineLocalDestructorCall(
          point->awaitable_temp, location);
      if (awaitable_dtor != NULL) {
        CoroutineCompoundInsertStatement(point->compound, awaitable_dtor,
                                         next_index);
        next_index++;
      }
      CoroutineCompoundInsertStatement(point->compound, suspend_if,
                                       next_index);
      CoroutineCompoundInsertStatement(point->compound, (ASTNode*)labels[i],
                                       next_index + 1);
      CoroutineCompoundInsertStatement(point->compound, reset_state,
                                       next_index + 2);
    }
  }

  for (int i = points->count - 1; i >= 0; i--) {
    SourceLocation location = points->points[i].kind == kSuspensionCoYield
                                  ? points->points[i].co_yield->location
                                  : points->points[i].co_await->location;
    ASTNode* entry_if = NewStateResumeIf(frame, i + 1, labels[i], location);
    CompoundASTNodeInsertStatement(body, entry_if, 1);
  }
  CompoundASTNodeInsertStatement(body,
                                 NewFrameIntAssignment(frame, frame->done, 0,
                                                       node->location),
                                 (size_t)points->count + 1);
  RewriteFrameOwnedSymbols((ASTNode*)body, frame);
  Symbol* resume_function =
      NewCoroutineResumeFunction(node, frame, body, return_type,
                                 scan.suspend_count, initial_awaiter,
                                 node->location);
  Symbol* destroy_function = NewCoroutineDestroyFunction(frame, node->location);
  if (resume_function != NULL && frame->resume != NULL) {
    CompoundASTNodeInsertStatement(
        body,
        NewFrameAssignment(
            frame, frame->resume,
            NewFunctionAddress(resume_function, frame->resume->symbol->type,
                               node->location),
            node->location),
        (size_t)points->count + 2);
  }
  if (destroy_function != NULL && frame->destroy != NULL) {
    CompoundASTNodeInsertStatement(
        body,
        NewFrameAssignment(
            frame, frame->destroy,
            NewFunctionAddress(destroy_function, frame->destroy->symbol->type,
                               node->location),
            node->location),
        (size_t)points->count + 3);
  }
  if (initial_awaiter != NULL) {
    size_t initial_index = (size_t)points->count + 4;
    CompoundASTNodeInsertStatement(
        body, NewInitialSuspendInitialization(frame, promise, node->location),
        initial_index++);
    CompoundASTNodeInsertStatement(
        body, NewInitialSuspendIf(frame, promise, initial_awaiter, return_type,
                                  node->location),
        initial_index++);
    CompoundASTNodeInsertStatement(
        body, NewAwaitResumeStatement(initial_awaiter, node->location),
        initial_index++);
    RewriteFrameOwnedSymbols((ASTNode*)body, frame);
  }
  for (int i = 0; i < points->count; i++) {
    VectorDeleteElement(body->statements, 1);
  }
  InsertCoroutineFrameStarterInitializers(body, frame, node->location);
  ResetCompoundStatementParents(body);
  free(labels);
  return true;
}

static bool LowerCoroutineFunction(ASTNode* node, CoroutineScan scan) {
  if (!scan.is_coroutine || node == NULL || node->type == NULL ||
      !TypeIsFunction(node->type)) {
    return true;
  }
  FunctionInfo* info = &node->type->info.function;
  if (info->coroutine_promise_type == NULL || info->body == NULL ||
      info->body->op != AST_OP(compound)) {
    return false;
  }

  TypeRecord* initial_awaiter_type =
      CoroutinePromiseMemberReturnType(info->coroutine_promise_type,
                                       "initial_suspend");
  bool initial_can_suspend = initial_awaiter_type != NULL &&
                             !AwaiterTypeIsAlwaysReady(initial_awaiter_type);
  bool needs_frame = scan.suspend_count > 0 || initial_can_suspend;
  TypeRecord* yield_awaiter_type =
      scan.has_co_yield
          ? CoroutinePromiseMemberReturnType(info->coroutine_promise_type,
                                             "yield_value")
          : NULL;
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)info->body;
  SuspensionPoints points = {0};
  Vector persisted_locals;
  VectorInit(&persisted_locals);
  points.capacity = scan.suspend_count;
  if (needs_frame && points.capacity > 0) {
    points.points = calloc((size_t)points.capacity, sizeof(SuspensionPoint));
    if (points.points == NULL) {
      SemanticError(node, "failed to allocate coroutine suspension points");
      PersistedCoroutineLocalsDestruct(&persisted_locals);
      return false;
    }
    CollectSuspensionPointsInCompound(body, &points);
    if (points.count != scan.suspend_count) {
      SemanticError(node, "unsupported coroutine suspension form");
      PersistedCoroutineLocalsDestruct(&persisted_locals);
      free(points.points);
      return false;
    }
    for (int i = 0; i < points.count; i++) {
      if (!ValidateSuspensionPoint(node, body, &points.points[i])) {
        PersistedCoroutineLocalsDestruct(&persisted_locals);
        free(points.points);
        return false;
      }
    }
    for (int i = 0; i < points.count; i++) {
      if (points.points[i].kind == kSuspensionCoAwait &&
          !ValidateCoroutineFrameOwnedAwaiterConstructible(
              points.points[i].co_await, points.points[i].awaiter->type)) {
        PersistedCoroutineLocalsDestruct(&persisted_locals);
        free(points.points);
        return false;
      }
    }
    if (!CollectPersistedCoroutineLocals(body, &points, &persisted_locals) ||
        !CollectPersistedCoroutineParameters(info, body, &points,
                                             &persisted_locals)) {
      PersistedCoroutineLocalsDestruct(&persisted_locals);
      free(points.points);
      return false;
    }
  }
  if (needs_frame && initial_awaiter_type != NULL &&
      !ValidateCoroutineFrameOwnedAwaiterConstructible(node,
                                                       initial_awaiter_type)) {
    PersistedCoroutineLocalsDestruct(&persisted_locals);
    free(points.points);
    return false;
  }
  if (needs_frame && yield_awaiter_type != NULL &&
      !ValidateCoroutineFrameOwnedAwaiterConstructible(node,
                                                       yield_awaiter_type)) {
    PersistedCoroutineLocalsDestruct(&persisted_locals);
    free(points.points);
    return false;
  }

  Symbol* promise = NULL;
  VariableDeclarationASTNode* promise_decl = NULL;
  Symbol* initial_awaiter = NULL;
  VariableDeclarationASTNode* initial_awaiter_decl = NULL;
  CoroutineFrame frame = {0};
  if (!needs_frame) {
    promise =
        SyntaxNewTemporary(&compiler->syntax,
                           TypeRecordCopy(info->coroutine_promise_type));
    promise->flags.is_local = true;
    promise->flags.is_defined = true;
    promise->location = node->location;
    promise_decl = (VariableDeclarationASTNode*)NewVariableDeclarationASTNode(
        promise, NULL, node->location);
  } else {
    promise =
        SyntaxNewTemporary(&compiler->syntax,
                           TypeRecordCopy(info->coroutine_promise_type));
    promise->flags.is_local = true;
    promise->flags.is_defined = true;
    promise->location = node->location;
    frame = NewCoroutineFrame(node->type->next, info->coroutine_promise_type,
                              initial_awaiter_type, &points,
                              &persisted_locals,
                              yield_awaiter_type,
                              node->location);
    CoroutineFrameAddOwnedSymbol(&frame, promise, frame.promise,
                                 frame.promise_constructed, true);
    for (size_t i = 0; i < persisted_locals.length; i++) {
      CoroutinePersistedLocal* local = persisted_locals.value.p[i];
      if (local != NULL && local->member != NULL) {
        CoroutineFrameAddOwnedSymbol(&frame, local->symbol, local->member,
                                     local->constructed_member, false);
      }
    }
    if (initial_awaiter_type != NULL) {
      initial_awaiter =
          SyntaxNewTemporary(&compiler->syntax,
                             TypeRecordCopy(initial_awaiter_type));
      initial_awaiter->flags.is_local = true;
      initial_awaiter->flags.is_defined = true;
      initial_awaiter->location = node->location;
      CoroutineFrameAddOwnedSymbol(&frame, initial_awaiter,
                                   frame.initial_awaiter,
                                   frame.initial_awaiter_constructed,
                                   false);
    }
    for (int i = 0; i < points.count; i++) {
      if (points.points[i].kind == kSuspensionCoAwait) {
        CoroutineFrameAddOwnedSymbol(
            &frame, points.points[i].awaiter,
            points.points[i].frame_member,
            points.points[i].frame_constructed_member, false);
      }
    }
  }

  Vector* decls = NewVector();
  if (promise_decl != NULL) {
    VectorAppend(decls, promise_decl);
  }
  if (frame.decl != NULL) {
    VectorAppend(decls, frame.decl);
  }
  if (initial_awaiter_decl != NULL) {
    VectorAppend(decls, initial_awaiter_decl);
  }
  CompoundASTNodeInsertStatement(
      body, NewDeclarationListASTNode(decls, node->location), 0);
  for (int i = 0; i < points.count; i++) {
    if (points.points[i].compound == body) {
      points.points[i].statement_index++;
    }
  }
  if (needs_frame) {
    InsertPersistedCoroutineParameterStores(body, &frame, &persisted_locals,
                                            &points);
    InsertPersistedCoroutineLocalStores(body, &frame, &persisted_locals,
                                        &points);
  }
  if (!needs_frame) {
    CompoundASTNodeInsertStatement(
        body,
        NewExpressionStatementASTNode(
            NewCoroutinePromiseMemberCall(promise, "initial_suspend",
                                          NewVector(), node->location),
            node->location),
        1);
  }
  LowerCoReturnsInStatement(info->body, promise, needs_frame ? &frame : NULL,
                            node->type->next);
  if (needs_frame &&
      !LowerSuspendingCoAwaitFunction(node, scan, &points, node->type->next,
                                      promise, &frame, initial_awaiter,
                                      yield_awaiter_type)) {
    CoroutineFrameOwnedSymbolsDestruct(&frame);
    PersistedCoroutineLocalsDestruct(&persisted_locals);
    free(points.points);
    return false;
  }
  if (needs_frame) {
    CoroutineFrameOwnedSymbolsDestruct(&frame);
  }
  PersistedCoroutineLocalsDestruct(&persisted_locals);
  free(points.points);
  return true;
}

static CoroutineScan MarkAndValidateCoroutineFunction(ASTNode* node) {
  CoroutineScan scan = {0};
  if (node == NULL || node->type == NULL || !TypeIsFunction(node->type) ||
      node->type->info.function.body == NULL) {
    return scan;
  }
  ASTNodeVisit(node->type->info.function.body, ScanCoroutineNode, 0, &scan);
  if (!scan.is_coroutine) {
    return scan;
  }

  FunctionInfo* info = &node->type->info.function;
  info->is_coroutine = true;
  info->coroutine_suspend_count = scan.suspend_count;
  if (info->is_constexpr || info->is_consteval) {
    SemanticError(node, "coroutine function cannot be constexpr or consteval");
  }
  if (info->is_constructor || info->is_destructor) {
    SemanticError(node, "constructors and destructors cannot be coroutines");
  }
  if (info->symbol != NULL && strcmp(info->symbol->name.value, "main") == 0) {
    SemanticError(node, "main cannot be a coroutine");
  }
  if (info->varargs || info->unknown_args) {
    SemanticError(node, "coroutines cannot use varargs or old-style parameters");
  }
  if (TypeFunctionReturnContainsAuto(node->type)) {
    SemanticError(node, "coroutine function return type cannot be deduced");
  }
  return scan;
}

static void ValidateCoroutinePromise(ASTNode* node, CoroutineScan scan) {
  if (!scan.is_coroutine || node == NULL || node->type == NULL ||
      !TypeIsFunction(node->type)) {
    return;
  }
  FunctionInfo* info = &node->type->info.function;
  if (info->coroutine_promise_type == NULL) {
    info->coroutine_promise_type =
        ResolveDirectCoroutinePromiseType(node->type->next);
  }
  if (info->coroutine_promise_type == NULL) {
    SemanticError(node, "coroutine return type must provide promise_type");
    return;
  }

  TypeRecord* promise = info->coroutine_promise_type;
  RequireCoroutinePromiseMember(node, promise, "get_return_object");
  RequireCoroutinePromiseMember(node, promise, "initial_suspend");
  if (RequireCoroutinePromiseMember(node, promise, "final_suspend")) {
    TypeRecord* final_awaiter =
        CoroutinePromiseMemberReturnType(promise, "final_suspend");
    if (!AwaiterTypeIsAlwaysReady(final_awaiter)) {
      SemanticError(node,
                    "coroutine final_suspend suspension is not supported yet");
    }
  }
  if (scan.has_co_yield) {
    RequireCoroutinePromiseMember(node, promise, "yield_value");
  }
  if (scan.has_co_return_value) {
    RequireCoroutinePromiseMember(node, promise, "return_value");
  } else if (scan.has_co_return) {
    RequireCoroutinePromiseMember(node, promise, "return_void");
  }
  RequireCoroutinePromiseMember(node, promise, "unhandled_exception");
}

void SemanticAnalyzeCoroutineFunction(ASTNode* node) {
  CoroutineScan coroutine_scan = MarkAndValidateCoroutineFunction(node);
  ValidateCoroutinePromise(node, coroutine_scan);
  if (coroutine_scan.is_coroutine && node != NULL && node->type != NULL &&
      TypeIsFunction(node->type) && node->type->info.function.body != NULL) {
    ASTNodeVisit(node->type->info.function.body, LowerReadyCoAwaitNode, 0,
                 NULL);
  }
  LowerCoroutineFunction(node, coroutine_scan);
}
