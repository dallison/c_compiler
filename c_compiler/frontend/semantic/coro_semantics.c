//
//  coro_semantics.c
//  c_compiler
//
//  C++ coroutine semantic analysis and early lowering.
//

#include "coro_semantics.h"
#include <assert.h>
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
  ASTNode* co_await;
  VariableDeclarationASTNode* value_decl;
  VariableDeclarationASTNode* awaiter_decl;
  DeclarationListASTNode* decl_list;
  CompoundStatementASTNode* compound;
  size_t statement_index;
  bool multiple;
} SuspensionPoint;

static ASTNode* NewCoroutinePromiseMemberCall(Symbol* promise,
                                              const char* member_name,
                                              Vector* actuals,
                                              SourceLocation location);

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

static ASTNode* NewSymbolAssignment(Symbol* symbol, ASTNode* value,
                                    SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), symbol->type, location,
                       NewIdentifierASTNode(symbol, location), value),
      location);
}

static ASTNode* NewIntAssignment(Symbol* symbol, int64_t value,
                                 SourceLocation location) {
  return NewSymbolAssignment(
      symbol,
      NewIntConstantASTNode(value, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location),
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

  ASTNode* return_object =
      NewCoroutineReturnObjectStatement(promise, coroutine_return_type,
                                        location);
  VectorAppend(statements, return_object);
  return NewCompoundStatementASTNode(statements, location);
}

static void LowerCoReturnsInStatement(ASTNode* node, Symbol* promise,
                                      TypeRecord* coroutine_return_type);

static void LowerCoReturnChild(ASTNode* parent, int child_id, Symbol* promise,
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
                                               promise,
                                               coroutine_return_type),
                        true);
    return;
  }
  LowerCoReturnsInStatement(child, promise, coroutine_return_type);
}

static void LowerCoReturnsInStatement(ASTNode* node, Symbol* promise,
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
                                     coroutine_return_type);
          VectorSet(compound->statements, i, lowered);
          lowered->parent = node;
          lowered->child_id = (int)i;
        } else {
          LowerCoReturnsInStatement(stmt, promise, coroutine_return_type);
        }
      }
      return;
    }
    case AST_OP(if):
      LowerCoReturnChild(node, 1, promise, coroutine_return_type);
      LowerCoReturnChild(node, 2, promise, coroutine_return_type);
      return;
    case AST_OP(while):
    case AST_OP(do):
    case AST_OP(switch):
      LowerCoReturnChild(node, 1, promise, coroutine_return_type);
      return;
    case AST_OP(for):
      LowerCoReturnChild(node, 3, promise, coroutine_return_type);
      return;
    case AST_OP(case):
      LowerCoReturnChild(node, 1, promise, coroutine_return_type);
      return;
    case AST_OP(label):
      LowerCoReturnChild(node, 0, promise, coroutine_return_type);
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

static ASTNode* NewStateResumeIf(Symbol* state, LabelASTNode* label,
                                 SourceLocation location) {
  ASTNode* condition =
      NewBinaryASTNode(AST_OP(equal), NULL, location,
                       NewIdentifierASTNode(state, location),
                       NewIntConstantASTNode(
                           1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  ASTNode* goto_stmt =
      NewGotoStatementASTNode(NewString(label->name.value), location);
  return NewIfStatementASTNode(condition, goto_stmt, NULL, false, location);
}

static ASTNode* NewSuspendIf(Symbol* state, Symbol* promise, Symbol* awaiter,
                             TypeRecord* return_type,
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
  VectorAppend(suspend_statements, NewIntAssignment(state, 1, location));
  Vector* suspend_actuals = NewVector();
  VectorAppend(suspend_actuals,
               NewIntConstantASTNode(0,
                                     NewTypeRecordWithSize(kTypeInt,
                                                           kQualPlain),
                                     location));
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

static bool LowerSingleSuspendingCoAwaitFunction(ASTNode* node,
                                                 TypeRecord* return_type,
                                                 Symbol* promise,
                                                 Symbol* state) {
  SuspensionPoint point = {0};
  CompoundStatementASTNode* body =
      (CompoundStatementASTNode*)node->type->info.function.body;
  for (size_t i = 0; i < body->statements->length; i++) {
    ASTNode* stmt = body->statements->value.p[i];
    if (stmt == NULL || stmt->op != AST_OP(decl_list)) {
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
      if (point.co_await != NULL) {
        point.multiple = true;
        break;
      }
      point.co_await = expr;
      point.value_decl = decl;
      point.decl_list = decl_list;
      point.compound = body;
      point.statement_index = i;
    }
  }
  if (point.multiple) {
    SemanticError(node, "coroutine suspension is not supported yet");
    return false;
  }
  if (point.co_await == NULL || point.value_decl == NULL ||
      point.decl_list == NULL) {
    SemanticError(node, "unsupported coroutine suspension form");
    return false;
  }
  if (point.decl_list->declarations->length != 1) {
    SemanticError(point.co_await,
                  "suspending co_await declaration must be isolated");
    return false;
  }
  if (point.compound != body) {
    SemanticError(point.co_await,
                  "nested coroutine suspension is not supported yet");
    return false;
  }

  Symbol* awaiter = CoAwaitIdentifierOperand(point.co_await);
  if (awaiter == NULL) {
    SemanticError(point.co_await,
                  "suspending co_await currently requires a named awaiter");
    return false;
  }
  point.awaiter_decl = FindAwaiterDeclarationBefore(&point, awaiter);
  if (point.awaiter_decl == NULL) {
    SemanticError(point.co_await,
                  "suspending co_await currently requires a prior awaiter "
                  "declaration");
    return false;
  }
  MarkDeclarationAsLocalStatic(point.awaiter_decl);

  SourceLocation location = point.co_await->location;
  String label_name;
  StringInit(&label_name, SyntaxFakeName(&compiler->syntax));
  LabelASTNode* resume_label =
      (LabelASTNode*)NewLabelASTNode(label_name.value, NULL, false, location);
  StringDestruct(&label_name);

  ASTNode* entry_if = NewStateResumeIf(state, resume_label, location);
  ASTNode* suspend_if = NewSuspendIf(state, promise, awaiter, return_type,
                                     location);
  ASTNode* reset_state = NewIntAssignment(state, 0, location);

  ReplaceVariableInitializerExpression(
      point.value_decl,
      UnwrapExpressionInitializer(
          NewAwaitResumeInitializer(awaiter, point.value_decl->symbol->type,
                                    location)));

  VectorInsertBefore(point.compound->statements, point.statement_index,
                     suspend_if);
  VectorInsertBefore(point.compound->statements, point.statement_index + 1,
                     (ASTNode*)resume_label);
  VectorInsertBefore(point.compound->statements, point.statement_index + 2,
                     reset_state);
  CompoundASTNodeInsertStatement(body, entry_if, 1);
  ResetCompoundStatementParents(body);
  return true;
}

static bool LowerCoroutineFunction(ASTNode* node, CoroutineScan scan) {
  if (!scan.is_coroutine || node == NULL || node->type == NULL ||
      !TypeIsFunction(node->type)) {
    return true;
  }
  FunctionInfo* info = &node->type->info.function;
  if (scan.suspend_count > 1 || scan.has_co_yield) {
    SemanticError(node, "coroutine suspension is not supported yet");
    return false;
  }
  if (info->coroutine_promise_type == NULL || info->body == NULL ||
      info->body->op != AST_OP(compound)) {
    return false;
  }

  Symbol* promise = NULL;
  VariableDeclarationASTNode* promise_decl = NULL;
  Symbol* state = NULL;
  VariableDeclarationASTNode* state_decl = NULL;
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
    state = NewCoroutineStaticSymbol(
        NewTypeRecordWithSize(kTypeInt, kQualPlain), node->location,
        &state_decl);
  }

  Vector* decls = NewVector();
  VectorAppend(decls, promise_decl);
  if (state_decl != NULL) {
    VectorAppend(decls, state_decl);
  }
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)info->body;
  CompoundASTNodeInsertStatement(
      body, NewDeclarationListASTNode(decls, node->location), 0);
  if (scan.suspend_count == 1 &&
      !LowerSingleSuspendingCoAwaitFunction(node, node->type->next, promise,
                                            state)) {
    return false;
  }
  LowerCoReturnsInStatement(info->body, promise, node->type->next);
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
  RequireCoroutinePromiseMember(node, promise, "final_suspend");
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
