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
  StructMember* state;
  StructMember* done;
  StructMember* resume;
  StructMember* destroy;
  VariableDeclarationASTNode* decl;
} CoroutineFrame;

static ASTNode* NewCoroutinePromiseMemberCall(Symbol* promise,
                                              const char* member_name,
                                              Vector* actuals,
                                              SourceLocation location);
static ASTNode* NewNullPointerConstant(SourceLocation location);

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
  ASTNode* id = NewIdentifierASTNode(frame->symbol, location);
  TypeRecord* pointer_type =
      NewPointerTo(kQualPlain, TypeRecordCopy(frame->symbol->type));
  ASTNode* address = NewUnaryASTNode(AST_OP(address), pointer_type, location,
                                    id);
  id->flags |= kASTNeedAddress;
  return address;
}

static ASTNode* NewFrameMemberAccess(CoroutineFrame* frame,
                                     StructMember* member,
                                     SourceLocation location) {
  ASTNode* receiver = NewIdentifierASTNode(frame->symbol, location);
  receiver->flags |= kASTNeedAddress;
  ASTNode* member_node = NewStructMemberASTNode(member, location);
  ((StructMemberASTNode*)member_node)->byte_offset = member->byte_offset;
  ASTNode* access =
      NewBinaryASTNode(AST_OP(dot), TypeRecordCopy(member->symbol->type),
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

static ASTNode* NewSymbolAssignmentStatement(Symbol* symbol, ASTNode* value,
                                             SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), symbol->type, location,
                       NewIdentifierASTNode(symbol, location), value),
      location);
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

static Symbol* NewCoroutineStaticSymbol(TypeRecord* type,
                                        SourceLocation location,
                                        VariableDeclarationASTNode** decl_out) {
  Symbol* symbol = NewSymbol(SyntaxFakeName(&compiler->syntax), type,
                             STO(static));
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->flags.is_local = true;
  symbol->location = location;
  bool added = SyntaxAddSymbol(&compiler->syntax, symbol);
  assert(added);
  (void)added;
  VariableDeclarationASTNode* decl =
      (VariableDeclarationASTNode*)NewVariableDeclarationASTNode(symbol, NULL,
                                                                location);
  VectorAppend(&compiler->syntax.local_statics, decl);
  if (decl_out != NULL) {
    *decl_out = decl;
  }
  return symbol;
}

static bool LowerCoYieldStatement(SuspensionPoint* point, Symbol* promise,
                                  TypeRecord* yield_awaiter_type) {
  if (yield_awaiter_type == NULL) {
    SemanticError(point->co_yield,
                  "coroutine yield_value return type is invalid");
    return false;
  }
  SourceLocation location = point->co_yield->location;
  VariableDeclarationASTNode* awaiter_decl = NULL;
  Symbol* awaiter = NewCoroutineStaticSymbol(TypeRecordCopy(yield_awaiter_type),
                                             location, &awaiter_decl);
  point->awaiter = awaiter;
  point->awaiter_decl = awaiter_decl;

  Vector* actuals = NewVector();
  VectorAppend(actuals, ASTNodeMove(((UnaryASTNode*)point->co_yield)->sub));
  ASTNode* yield_call = NewCoroutinePromiseMemberCall(
      promise, "yield_value", actuals, location);
  ASTNodeSetType(yield_call, TypeRecordCopy(awaiter->type));
  ASTNode* assignment =
      NewSymbolAssignmentStatement(awaiter, yield_call, location);
  VectorSet(point->compound->statements, point->statement_index, assignment);
  assignment->parent = (ASTNode*)point->compound;
  assignment->child_id = (int)point->statement_index;
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

static TypeRecord* NewCoroutineResumePointerType(TypeRecord* return_type) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, TypeRecordCopy(return_type));
  return NewPointerTo(kQualPlain, func);
}

static TypeRecord* NewCoroutineResumeFunctionType(TypeRecord* return_type) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, TypeRecordCopy(return_type));
  func->info.function.definition = true;
  return func;
}

static TypeRecord* NewCoroutineDestroyPointerType(void) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return NewPointerTo(kQualPlain, func);
}

static TypeRecord* NewCoroutineDestroyFunctionType(void) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  func->info.function.definition = true;
  return func;
}

static StructMember* AddCoroutineFrameResumeMember(Struct* str,
                                                   TypeRecord* return_type) {
  Symbol* symbol = NewSymbol("__resume",
                             NewCoroutineResumePointerType(return_type),
                             STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

static StructMember* AddCoroutineFrameDestroyMember(Struct* str) {
  Symbol* symbol =
      NewSymbol("__destroy", NewCoroutineDestroyPointerType(), STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

static CoroutineFrame NewCoroutineFrame(TypeRecord* return_type,
                                        SourceLocation location) {
  TypeRecord* frame_type = NewTypeRecord(kTypeStruct, kQualPlain);
  Struct* str = NewStruct(false);
  frame_type->info.struct_info = str;

  CoroutineFrame frame = {0};
  frame.state = AddCoroutineFrameIntMember(str, "__state");
  frame.done = AddCoroutineFrameIntMember(str, "__done");
  frame.resume = AddCoroutineFrameResumeMember(str, return_type);
  frame.destroy = AddCoroutineFrameDestroyMember(str);
  frame_type = TypeRecordCalculateSize(frame_type);

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

  frame.symbol = NewCoroutineStaticSymbol(frame_type, location, &frame.decl);
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

static Symbol* NewCoroutineResumeFunction(ASTNode* node,
                                          CompoundStatementASTNode* body,
                                          TypeRecord* return_type,
                                          SourceLocation location) {
  char name[96];
  snprintf(name, sizeof(name), "%s_coroutine_resume",
           SyntaxFakeName(&compiler->syntax));
  TypeRecord* func = NewCoroutineResumeFunctionType(return_type);
  ASTNode* cloned = ASTNodeClone((ASTNode*)body, IdentityCloneNode, NULL, NULL);
  CompoundStatementASTNode* resume_body = (CompoundStatementASTNode*)cloned;
  if (resume_body->statements->length > 0) {
    ASTNode* first = resume_body->statements->value.p[0];
    if (first != NULL && first->op == AST_OP(decl_list)) {
      VectorDeleteElement(resume_body->statements, 0);
    }
  }
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
  TypeRecord* func = NewCoroutineDestroyFunctionType();
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewFrameIntAssignment(frame, frame->state, 0, location));
  VectorAppend(statements,
               NewFrameIntAssignment(frame, frame->done, 1, location));
  VectorAppend(statements,
               NewFrameAssignment(frame, frame->resume,
                                  NewNullPointerConstant(location), location));
  VectorAppend(statements,
               NewFrameAssignment(frame, frame->destroy,
                                  NewNullPointerConstant(location), location));
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

static void MarkDeclarationAsLocalStatic(VariableDeclarationASTNode* decl) {
  if (decl == NULL || decl->symbol == NULL ||
      StorageIs(decl->symbol->storage, STO(static))) {
    return;
  }
  decl->symbol->storage = STO(static);
  decl->symbol->flags.is_local = true;
  decl->symbol->flags.is_defined = true;
  VectorAppend(&compiler->syntax.local_statics, decl);
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
  VectorAppend(suspend_statements,
               NewExpressionStatementASTNode(
                   NewAwaiterMemberCallWithActuals(
                       NewIdentifierASTNode(awaiter, location),
                       "await_suspend", suspend_actuals, location),
                   location));
  VectorAppend(suspend_statements,
               NewCoroutineReturnObjectStatement(promise, return_type,
                                                 location));
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

static void ResetCompoundStatementParents(CompoundStatementASTNode* compound) {
  if (compound == NULL) {
    return;
  }
  for (size_t i = 0; i < compound->statements->length; i++) {
    ASTNode* stmt = compound->statements->value.p[i];
    if (stmt != NULL) {
      stmt->parent = (ASTNode*)compound;
      stmt->child_id = (int)i;
    }
  }
}

static void CollectTopLevelSuspensionPoints(CompoundStatementASTNode* body,
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

static bool ValidateSuspensionPoint(ASTNode* node, CompoundStatementASTNode* body,
                                    SuspensionPoint* point) {
  if (point->kind == kSuspensionCoYield) {
    if (point->co_yield == NULL || point->statement == NULL) {
      SemanticError(node, "unsupported coroutine suspension form");
      return false;
    }
    if (point->compound != body) {
      SemanticError(point->co_yield,
                    "nested coroutine suspension is not supported yet");
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
  if (point->compound != body) {
    SemanticError(point->co_await,
                  "nested coroutine suspension is not supported yet");
    return false;
  }

  Symbol* awaiter = CoAwaitIdentifierOperand(point->co_await);
  if (awaiter == NULL) {
    SemanticError(point->co_await,
                  "suspending co_await currently requires a named awaiter");
    return false;
  }
  point->awaiter_decl = FindAwaiterDeclarationBefore(point, awaiter);
  if (point->awaiter_decl == NULL) {
    SemanticError(point->co_await,
                  "suspending co_await currently requires a prior awaiter "
                  "declaration");
    return false;
  }
  MarkDeclarationAsLocalStatic(point->awaiter_decl);
  return true;
}

static bool LowerSuspendingCoAwaitFunction(ASTNode* node,
                                           CoroutineScan scan,
                                           TypeRecord* return_type,
                                           Symbol* promise,
                                           CoroutineFrame* frame,
                                           TypeRecord* yield_awaiter_type) {
  CompoundStatementASTNode* body =
      (CompoundStatementASTNode*)node->type->info.function.body;
  SuspensionPoints points = {0};
  points.capacity = scan.suspend_count;
  points.points = calloc((size_t)points.capacity, sizeof(SuspensionPoint));
  if (points.points == NULL) {
    SemanticError(node, "failed to allocate coroutine suspension points");
    return false;
  }
  CollectTopLevelSuspensionPoints(body, &points);
  if (points.count != scan.suspend_count) {
    SemanticError(node, "unsupported coroutine suspension form");
    free(points.points);
    return false;
  }
  for (int i = 0; i < points.count; i++) {
    if (!ValidateSuspensionPoint(node, body, &points.points[i])) {
      free(points.points);
      return false;
    }
  }

  LabelASTNode** labels =
      calloc((size_t)points.count, sizeof(LabelASTNode*));
  if (labels == NULL) {
    free(points.points);
    SemanticError(node, "failed to allocate coroutine resume labels");
    return false;
  }
  for (int i = 0; i < points.count; i++) {
    SourceLocation location = points.points[i].kind == kSuspensionCoYield
                                  ? points.points[i].co_yield->location
                                  : points.points[i].co_await->location;
    String label_name;
    StringInit(&label_name, SyntaxFakeName(&compiler->syntax));
    labels[i] =
        (LabelASTNode*)NewLabelASTNode(label_name.value, NULL, false, location);
    StringDestruct(&label_name);

    if (points.points[i].kind == kSuspensionCoYield) {
      if (!LowerCoYieldStatement(&points.points[i], promise,
                                 yield_awaiter_type)) {
        free(labels);
        free(points.points);
        return false;
      }
    } else {
      Symbol* awaiter = CoAwaitIdentifierOperand(points.points[i].co_await);
      points.points[i].awaiter = awaiter;
      ReplaceVariableInitializerExpression(
          points.points[i].value_decl,
          UnwrapExpressionInitializer(NewAwaitResumeInitializer(
              awaiter, points.points[i].value_decl->symbol->type, location)));
    }
  }

  for (int i = points.count - 1; i >= 0; i--) {
    SuspensionPoint* point = &points.points[i];
    SourceLocation location = point->kind == kSuspensionCoYield
                                  ? point->co_yield->location
                                  : point->co_await->location;
    Symbol* awaiter = point->awaiter;
    ASTNode* suspend_if =
        NewSuspendIf(frame, promise, awaiter, i + 1, return_type, location);
    ASTNode* reset_state =
        NewFrameIntAssignment(frame, frame->state, 0, location);
    if (point->kind == kSuspensionCoYield) {
      VectorInsertBefore(point->compound->statements, point->statement_index + 1,
                         suspend_if);
      VectorInsertBefore(point->compound->statements, point->statement_index + 2,
                         (ASTNode*)labels[i]);
      VectorInsertBefore(point->compound->statements, point->statement_index + 3,
                         reset_state);
      VectorInsertBefore(point->compound->statements, point->statement_index + 4,
                         NewAwaitResumeStatement(awaiter, location));
    } else {
      VectorInsertBefore(point->compound->statements, point->statement_index,
                         suspend_if);
      VectorInsertBefore(point->compound->statements, point->statement_index + 1,
                         (ASTNode*)labels[i]);
      VectorInsertBefore(point->compound->statements, point->statement_index + 2,
                         reset_state);
    }
  }

  for (int i = points.count - 1; i >= 0; i--) {
    SourceLocation location = points.points[i].kind == kSuspensionCoYield
                                  ? points.points[i].co_yield->location
                                  : points.points[i].co_await->location;
    ASTNode* entry_if = NewStateResumeIf(frame, i + 1, labels[i], location);
    CompoundASTNodeInsertStatement(body, entry_if, 1);
  }
  CompoundASTNodeInsertStatement(body,
                                 NewFrameIntAssignment(frame, frame->done, 0,
                                                       node->location),
                                 (size_t)points.count + 1);
  Symbol* resume_function =
      NewCoroutineResumeFunction(node, body, return_type, node->location);
  Symbol* destroy_function = NewCoroutineDestroyFunction(frame, node->location);
  if (resume_function != NULL && frame->resume != NULL) {
    CompoundASTNodeInsertStatement(
        body,
        NewFrameAssignment(
            frame, frame->resume,
            NewFunctionAddress(resume_function, frame->resume->symbol->type,
                               node->location),
            node->location),
        (size_t)points.count + 2);
  }
  if (destroy_function != NULL && frame->destroy != NULL) {
    CompoundASTNodeInsertStatement(
        body,
        NewFrameAssignment(
            frame, frame->destroy,
            NewFunctionAddress(destroy_function, frame->destroy->symbol->type,
                               node->location),
            node->location),
        (size_t)points.count + 3);
  }
  for (int i = 0; i < points.count; i++) {
    VectorDeleteElement(body->statements, 1);
  }
  ResetCompoundStatementParents(body);
  free(labels);
  free(points.points);
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

  Symbol* promise = NULL;
  VariableDeclarationASTNode* promise_decl = NULL;
  CoroutineFrame frame = {0};
  if (scan.suspend_count == 0) {
    promise =
        SyntaxNewTemporary(&compiler->syntax,
                           TypeRecordCopy(info->coroutine_promise_type));
    promise->flags.is_local = true;
    promise->flags.is_defined = true;
    promise->location = node->location;
    promise_decl = (VariableDeclarationASTNode*)NewVariableDeclarationASTNode(
        promise, NULL, node->location);
  } else {
    promise = NewCoroutineStaticSymbol(
        TypeRecordCopy(info->coroutine_promise_type), node->location,
        &promise_decl);
    frame = NewCoroutineFrame(node->type->next, node->location);
  }

  Vector* decls = NewVector();
  VectorAppend(decls, promise_decl);
  if (frame.decl != NULL) {
    VectorAppend(decls, frame.decl);
  }
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)info->body;
  CompoundASTNodeInsertStatement(
      body, NewDeclarationListASTNode(decls, node->location), 0);
  LowerCoReturnsInStatement(info->body, promise,
                            scan.suspend_count > 0 ? &frame : NULL,
                            node->type->next);
  TypeRecord* yield_awaiter_type =
      scan.has_co_yield
          ? CoroutinePromiseMemberReturnType(info->coroutine_promise_type,
                                             "yield_value")
          : NULL;
  if (scan.suspend_count > 0 &&
      !LowerSuspendingCoAwaitFunction(node, scan, node->type->next, promise,
                                      &frame, yield_awaiter_type)) {
    return false;
  }
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
