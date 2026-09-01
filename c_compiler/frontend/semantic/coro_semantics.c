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
#include "expr_semantics.h"
#include "semantics.h"
#include "statement_semantics.h"

/* Result of walking a function body to decide whether it is a coroutine
 * (contains co_await/co_yield/co_return) and to count its suspension points. */
typedef struct {
  bool is_coroutine;
  int suspend_count;
  bool has_co_return;
  bool has_co_return_value;
  bool has_co_yield;
} CoroutineScan;

/* A single suspension point (one co_await or co_yield) discovered in the body,
 * together with the synthesized declarations/temporaries used to lower it: the
 * awaiter object, its initializer, the frame slot it is stored in, and the
 * statement/compound it lives in so it can be spliced into the state machine. */
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
  StructMember* yielded_value_member;
  StructMember* yielded_value_constructed_member;
  DeclarationListASTNode* decl_list;
  CompoundStatementASTNode* compound;
  size_t statement_index;
  bool multiple;
} SuspensionPoint;

/* Growable collection of the suspension points found in a coroutine body. */
typedef struct {
  SuspensionPoint* points;
  int count;
  int capacity;
} SuspensionPoints;

/* A local variable (or parameter) whose lifetime crosses a suspension point and
 * must therefore be promoted from the stack into the heap-allocated coroutine
 * frame. Records the frame slot, whether it came from a parameter (and so needs
 * copy/move-construction at frame setup), and the compound where its store
 * statement is emitted. */
typedef struct {
  Symbol* symbol;
  StructMember* member;
  StructMember* constructed_member;
  bool is_parameter;
  bool is_catch_parameter;
  bool move_parameter;
  CompoundStatementASTNode* store_compound;
} CoroutinePersistedLocal;

/* The heap-allocated coroutine frame: the synthesized struct that holds the
 * resume/destroy state, the promise object, the initial/final awaiters, and all
 * persisted locals. `*_constructed` members are flags tracking whether the
 * corresponding object has been constructed yet (for correct teardown). */
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
  StructMember* final_awaiter;
  StructMember* final_awaiter_constructed;
  Vector owned_symbols;
  VariableDeclarationASTNode* decl;
} CoroutineFrame;

/* An object owned by the coroutine frame (promise, awaiters, persisted locals)
 * that has C++ lifetime: it has a frame slot, an optional "constructed" flag
 * slot, and a note on whether it should be constructed eagerly at frame start. */
typedef struct {
  Symbol* symbol;
  StructMember* member;
  StructMember* constructed_member;
  bool construct_at_start;
  bool body_lifetime;
} FrameOwnedSymbol;

static ASTNode* NewCoroutinePromiseMemberCall(Symbol* promise,
                                              const char* member_name,
                                              Vector* actuals,
                                              SourceLocation location);
static StructMember* FindCoroutinePromiseMember(TypeRecord* promise_type,
                                                const char* name);
static ASTNode* NewNullPointerConstant(SourceLocation location);
static ASTNode* NewAwaiterMemberCall(ASTNode* awaiter,
                                     const char* member_name,
                                     SourceLocation location);
static bool TypeIsCoroutineHandle(TypeRecord* type);
static ASTNode* NewAwaitResumeStatement(Symbol* awaiter,
                                        SourceLocation location);
static ASTNode* NewCoroutineMoveExpression(Symbol* symbol,
                                           SourceLocation location);
static TypeRecord* CoAwaitOperandType(ASTNode* co_await);
static TypeRecord* CoAwaitResultType(ASTNode* co_await);
static TypeRecord* CoYieldResultType(ASTNode* co_yield);
static const char* CXXConstructorNameForType(TypeRecord* type);
static Vector* TakeCXXTemporaryConstructionActuals(ASTNode* expr,
                                                   TypeRecord* type);
static void ResetCompoundStatementParents(CompoundStatementASTNode* compound);
static void CoroutineCompoundInsertStatement(CompoundStatementASTNode* compound,
                                             ASTNode* stmt,
                                             size_t at_index);
static void NormalizeNestedSuspensionsInCompound(
    CompoundStatementASTNode* compound);
static void CoroutineFrameAddOwnedSymbol(CoroutineFrame* frame,
                                         Symbol* symbol,
                                         StructMember* member,
                                         StructMember* constructed_member,
                                         bool construct_at_start,
                                         bool body_lifetime);
static CoroutinePersistedLocal* FindPersistedCoroutineLocal(Vector* locals,
                                                            Symbol* symbol);
static void AppendCoroutineFrameBodyDestructors(Vector* statements,
                                                CoroutineFrame* frame,
                                                SourceLocation location);
static void AppendPersistedCoroutineLocalDestructors(
    Vector* statements, CoroutineFrame* frame, Vector* persisted_locals,
    ASTNode* return_statement, SourceLocation location);
static void AppendCoroutineAwaitSuspendReturn(Vector* suspend_statements,
                                              CoroutineFrame* frame,
                                              ASTNode* await_suspend,
                                              TypeRecord* await_suspend_return,
                                              Symbol* return_object,
                                              SourceLocation location);

/* Clone callback that returns the node unchanged (shallow sharing). */
static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

/* Look up a member function named `name` on an awaiter struct (e.g.
 * await_ready/await_suspend/await_resume), skipping non-function members. */
static StructMember* FindAwaiterMember(TypeRecord* awaiter_type,
                                       const char* name) {
  if (awaiter_type == NULL || !TypeIsStructOrUnion(awaiter_type) ||
      awaiter_type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(awaiter_type->info.struct_info, name);
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function) {
      return candidate;
    }
  }
  return NULL;
}

/* True if a function body is exactly `{ return <non-zero constant>; }`. Used to
 * detect awaiters that are statically always-ready (await_ready returns true). */
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

/* True if this co_await's awaiter has a trivially-true await_ready, meaning the
 * suspension can be elided and the co_await lowered to just await_resume(). */
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

/* Find a member `operator co_await` on `type`, if present. */
static StructMember* FindMemberOperatorCoAwait(TypeRecord* type) {
  return FindAwaiterMember(type, "operator co_await");
}

/* Verify a type satisfies the Awaiter interface (await_ready/await_suspend/
 * await_resume present, with await_suspend returning a permitted type),
 * emitting a semantic error and returning false otherwise. */
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
      !TypeIsBool(await_suspend_return) &&
      !TypeIsVoidPointer(await_suspend_return) &&
      !TypeIsCoroutineHandle(await_suspend_return)) {
    SemanticError(node,
                  "await_suspend must return void, bool, void*, or a "
                  "coroutine handle");
    return false;
  }
  if (FindAwaiterMember(awaiter_type, "await_resume") == NULL) {
    SemanticError(node, "awaiter is missing await_resume");
    return false;
  }
  return true;
}

/* Determine the struct type to use for member lookup on a co_await operand,
 * handling member accesses, compound literals, and calls whose type is carried
 * on a child rather than the node itself. */
static TypeRecord* CoAwaitOperandMemberLookupType(ASTNode* operand) {
  if (operand == NULL) {
    return NULL;
  }
  if (operand->type != NULL) {
    return operand->type;
  }
  if (operand->op == AST_OP(dot) || operand->op == AST_OP(arrow)) {
    ASTNode* member = ((BinaryASTNode*)operand)->right;
    if (member != NULL && member->type != NULL) {
      return member->type;
    }
    if (member != NULL && member->op == AST_OP(structmember)) {
      StructMember* resolved = ((StructMemberASTNode*)member)->member;
      if (resolved != NULL && resolved->symbol != NULL) {
        return resolved->symbol->type;
      }
    }
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

static void SetCoroutineCallResultType(ASTNode* call,
                                       TypeRecord* return_type) {
  if (TypeIsReference(return_type)) {
    ASTNodeSetType(call, TypeRecordCopy(return_type->next));
    call->value_category =
        return_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType(call, TypeRecordCopy(return_type));
    call->value_category = kValueCategoryPrvalue;
  }
}

/* If the co_await operand's type provides a member `operator co_await`, rewrite
 * the operand so the awaiter is the result of calling it. When the operand is a
 * prvalue temporary, materialize it into a named temporary first (constructing
 * it, then calling the operator via a comma expression) so the awaiter binds to
 * a stable object. Returns true if a rewrite happened. */
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
    /* Operand is a temporary: bind it to a named temp, construct it, then
     * (temp.ctor(...), temp.operator co_await()) yields the awaiter. */
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
    SetCoroutineCallResultType(operator_call, member->symbol->type->next);
    ASTNode* comma =
        NewBinaryASTNode(AST_OP(comma), TypeRecordCopy(member->symbol->type->next),
                         co_await->base.location, constructor_call,
                         operator_call);
    ASTNodeSetType(comma, TypeRecordCopy(member->symbol->type->next));
    ASTNodeReplaceChild((ASTNode*)co_await, 0, comma, true);
    return true;
  }
  /* Operand is already an lvalue: call operator co_await on it directly. */
  ASTNode* receiver = ASTNodeMove(co_await->sub);
  ASTNode* call =
      NewAwaiterMemberCall(receiver, "operator co_await",
                           co_await->base.location);
  SetCoroutineCallResultType(call, member->symbol->type->next);
  ASTNodeReplaceChild((ASTNode*)co_await, 0, call, true);
  return true;
}

/* True if the co_await operand is an lvalue (affects rvalue-ref overload
 * selection for a free operator co_await). */
static bool CoAwaitOperandIsLValue(ASTNode* operand) {
  return operand != NULL &&
         (operand->value_category == kValueCategoryLvalue ||
          (operand->op == AST_OP(identifier) && operand->type != NULL &&
           !TypeIsFunction(operand->type)));
}

/* Linear membership test used to de-duplicate namespaces/candidates below. */
static bool CoroVectorContainsPointer(Vector* vec, void* value) {
  for (size_t i = 0; i < vec->length; i++) {
    if (vec->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

/* Resolve a chain of `using`-declaration aliases to the underlying symbol
 * (depth-capped to avoid cycles). */
static Symbol* CoroFollowUsingAlias(Symbol* symbol) {
  int depth = 0;
  while (symbol != NULL && symbol->flags.is_using_alias &&
         symbol->alias_target != NULL && depth < 64) {
    symbol = symbol->alias_target;
    depth++;
  }
  return symbol;
}

/* Add associated namespaces for operator co_await ADL, including inline and
 * anonymous namespace closure in both directions. */
static void CoroADLAddAssociatedNamespaceClosure(Vector* namespaces,
                                                 Namespace* ns) {
  NamespaceCollectADLAssociatedNamespaces(ns, namespaces);
}

/* Strip references/pointers/arrays to reach the underlying type for ADL. */
static TypeRecord* CoroADLCanonicalType(TypeRecord* type) {
  while (type != NULL &&
         (TypeIsReference(type) || TypeIsPointer(type) || TypeIsArray(type))) {
    type = type->next;
  }
  return type;
}

/* Collect the associated namespaces for `type` (its own namespace plus those of
 * base classes and template arguments) for argument-dependent lookup. */
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
      CoroADLAddAssociatedNamespaceClosure(
          namespaces, str->tag_symbol->namespace_ != NULL
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
    CoroADLAddAssociatedNamespaceClosure(namespaces, tag->namespace_ != NULL
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

/* Append every function in an overload chain (following using-aliases) to the
 * candidate set, skipping non-functions and duplicates. */
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

/* Look up `name` in namespace `ns` (or global) and add its overloads. */
static void CoroAddCollectedFunctionSymbols(Vector* candidates, Vector* functions) {
  for (size_t i = 0; i < functions->length; i++) {
    Symbol* sym = (Symbol*)functions->value.p[i];
    Symbol* effective = CoroFollowUsingAlias(sym);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !CoroVectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
}

static void CoroADLAddNamedFunctionCandidates(String* name, Namespace* ns,
                                              Vector* candidates) {
  Vector functions;
  VectorInit(&functions);
  if (ns == NULL || ns == compiler->global_namespace) {
    NamespaceCollectFunctionSymbolsInInlineSet(compiler->global_namespace, name,
                                               &functions);
    CoroAddCollectedFunctionSymbols(candidates, &functions);
    CoroADLAddFunctionOverloadCandidates(candidates, FindGlobalSymbol(name));
  } else {
    NamespaceCollectFunctionSymbolsInInlineSet(ns, name, &functions);
    CoroAddCollectedFunctionSymbols(candidates, &functions);
  }
  VectorDestruct(&functions);
}

/* Gather `name` overloads from all namespaces associated with `type` (ADL). */
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

/* Rank how well a free operator co_await's single parameter `formal` matches the
 * operand (lower is better; -1 means non-viable). Encodes the reference-binding
 * preferences: exact value match beats reference binding, const/rvalue rules are
 * applied so the best overload can be chosen. */
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

/* Find the best non-member `operator co_await` for `operand` via ADL + global
 * lookup, scoring each viable single-parameter overload and reporting ambiguity. */
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

/* True if the free operator co_await takes its argument by rvalue reference, in
 * which case the operand must be moved into the call. */
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

/* Like ApplyMemberOperatorCoAwait but for a non-member operator co_await: rewrite
 * the operand to the result of calling the chosen free function, materializing a
 * temporary (and moving it in if the parameter is an rvalue reference) when the
 * operand is a prvalue. Returns true if a rewrite happened. */
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
    /* Operand is a temporary: bind to a named temp, then
     * (temp.ctor(...), operator co_await(temp-or-move(temp))). */
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

/* Build an AST call node `awaiter.member_name(actuals...)`. */
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

/* Build an AST call node `awaiter.member_name()` with no arguments. */
static ASTNode* NewAwaiterMemberCall(ASTNode* awaiter,
                                     const char* member_name,
                                     SourceLocation location) {
  return NewAwaiterMemberCallWithActuals(awaiter, member_name, NewVector(),
                                         location);
}

/* Reference the coroutine frame pointer (the `frame->symbol` local). */
static ASTNode* NewFrameAddress(CoroutineFrame* frame,
                                SourceLocation location) {
  return NewIdentifierASTNode(frame->symbol, location);
}

/* Return the first user-visible parameter of an awaiter's await_suspend (i.e.
 * the coroutine-handle parameter), skipping the implicit `this`. */
static Symbol* AwaitSuspendFirstUserFormal(Symbol* awaiter) {
  StructMember* await_suspend =
      awaiter != NULL ? FindAwaiterMember(awaiter->type, "await_suspend") : NULL;
  if (await_suspend == NULL || await_suspend->symbol == NULL ||
      await_suspend->symbol->type == NULL ||
      !TypeIsFunction(await_suspend->symbol->type)) {
    return NULL;
  }
  FunctionInfo* info = &await_suspend->symbol->type->info.function;
  size_t formal_index = await_suspend->is_member_function ? 1 : 0;
  if (formal_index >= info->prototype.length) {
    return NULL;
  }
  return info->prototype.value.p[formal_index];
}

/* Find a coroutine handle's static `from_address(void*)` factory member, used to
 * reconstruct a typed handle from the raw frame pointer. */
static Symbol* FindCoroutineHandleFromAddress(TypeRecord* handle_type) {
  if (handle_type == NULL || !TypeIsStructOrUnion(handle_type) ||
      handle_type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(handle_type->info.struct_info, "from_address");
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || !candidate->is_static ||
        candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        candidate->symbol->type->info.function.prototype.length != 1 ||
        candidate->symbol->type->next == NULL) {
      continue;
    }
    Symbol* formal = candidate->symbol->type->info.function.prototype.value.p[0];
    if (formal != NULL && TypeIsVoidPointer(formal->type)) {
      return candidate->symbol;
    }
  }
  return NULL;
}

/* Find a coroutine handle's `address()` member returning the raw void* frame
 * pointer (the inverse of from_address). */
static Symbol* FindCoroutineHandleAddress(TypeRecord* handle_type) {
  if (handle_type == NULL || !TypeIsStructOrUnion(handle_type) ||
      handle_type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(handle_type->info.struct_info, "address");
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->is_static ||
        candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        candidate->symbol->type->next == NULL) {
      continue;
    }
    if (TypeIsVoidPointer(candidate->symbol->type->next)) {
      return candidate->symbol;
    }
  }
  return NULL;
}

/* Duck-typed check: a type is a coroutine handle if it has both from_address and
 * address members. */
static bool TypeIsCoroutineHandle(TypeRecord* type) {
  return FindCoroutineHandleFromAddress(type) != NULL &&
         FindCoroutineHandleAddress(type) != NULL;
}

/* Build the argument passed to await_suspend: if its parameter is a coroutine
 * handle type, wrap the frame pointer in handle::from_address(frame); otherwise
 * pass the raw frame pointer. */
static ASTNode* NewAwaitSuspendHandleArgument(CoroutineFrame* frame,
                                             Symbol* awaiter,
                                             SourceLocation location) {
  Symbol* formal = AwaitSuspendFirstUserFormal(awaiter);
  TypeRecord* formal_type = formal != NULL ? formal->type : NULL;
  if (TypeIsReference(formal_type)) {
    formal_type = formal_type->next;
  }
  if (TypeIsStructOrUnion(formal_type)) {
    Symbol* from_address = FindCoroutineHandleFromAddress(formal_type);
    if (from_address != NULL) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, NewFrameAddress(frame, location));
      ASTNode* call = NewVectorASTNode(
          AST_OP(call), TypeRecordCopy(from_address->type->next), location,
          NewIdentifierASTNode(from_address, location), actuals);
      ASTNodeSetType(call, TypeRecordCopy(from_address->type->next));
      return call;
    }
  }
  return NewFrameAddress(frame, location);
}

/* Build an lvalue access `frame->member` to a slot in the coroutine frame. */
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

/* Build a statement `frame->member = <scalar value>;` using the member's type
 * for the constant. State is an int, while done/lifetime flags are bool. */
static ASTNode* NewFrameIntAssignment(CoroutineFrame* frame,
                                      StructMember* member,
                                      int64_t value,
                                      SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), member->symbol->type, location,
                       NewFrameMemberAccess(frame, member, location),
                       NewIntConstantASTNode(
                           value, TypeRecordCopy(member->symbol->type),
                           location)),
      location);
}

/* Build a statement `frame->member = <value expr>;`. */
static ASTNode* NewFrameAssignment(CoroutineFrame* frame,
                                   StructMember* member,
                                   ASTNode* value,
                                   SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), member->symbol->type, location,
                       NewFrameMemberAccess(frame, member, location), value),
      location);
}

/* The constructor name for a class type is its tag name (constructors are stored
 * as members named after the class). Returns NULL for non-class types. */
static const char* CXXConstructorNameForType(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return type->info.struct_info->tag_name->value;
}

/* For classes with virtual bases, constructors/destructors take a leading
 * is-complete-object flag; prepend that implicit `1` argument to `actuals`. */
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

/* Find the (head of the) constructor overload set for a class type. */
static StructMember* FindCXXConstructorForType(TypeRecord* type) {
  const char* constructor_name = CXXConstructorNameForType(type);
  if (constructor_name == NULL || type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* ctor =
      FindStructMemberByName(type->info.struct_info, constructor_name);
  if (ctor == NULL || !ctor->is_member_function ||
      ctor->symbol == NULL || ctor->symbol->type == NULL ||
      !TypeIsFunction(ctor->symbol->type) ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}

/* True if a constructor overload is the default constructor (no user parameters
 * beyond the implicit object/complete-object args) and not deleted. */
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

/* Find a usable default constructor among a class's constructor overloads. */
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

/* True if a constructor overload is the copy (move==false) or move (move==true)
 * constructor: a single source parameter that is an lvalue/rvalue reference to
 * the same class type. */
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

/* True if a constructor overload is the copy constructor. */
static bool CXXConstructorCandidateIsCopy(StructMember* candidate,
                                          TypeRecord* type) {
  return CXXConstructorCandidateIsCopyOrMove(candidate, type, false);
}

/* True if a constructor overload is the move constructor. */
static bool CXXConstructorCandidateIsMove(StructMember* candidate,
                                          TypeRecord* type) {
  return CXXConstructorCandidateIsCopyOrMove(candidate, type, true);
}

/* Find the copy constructor among a class's constructor overloads, if any. */
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

/* Find the move constructor among a class's constructor overloads, if any. */
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

/* Find the destructor (`~Tag`) member of a class type, if declared. */
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

/* True if a frame-owned object of this type has non-trivial C++ lifetime (a user
 * constructor or destructor), so the frame must explicitly construct/destroy it
 * rather than treating it as plain memory. */
static bool CoroutineFrameOwnedTypeNeedsCXXLifetime(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  return FindCXXConstructorForType(type) != NULL ||
         FindCXXDestructorForType(type) != NULL;
}

/* Ensure a frame-owned awaiter that needs C++ lifetime is copy-constructible;
 * emit an error otherwise. */
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

/* True if `expr` is a temporary construction of `type` (a constructor call, or a
 * `(ctor(...), object)` comma) whose constructor arguments can be lifted out and
 * re-applied to construct a frame slot directly. Mirror of
 * TakeCXXTemporaryConstructionActuals without mutating the AST. */
static bool CXXTemporaryConstructionActualsCanBeTaken(ASTNode* expr,
                                                      TypeRecord* type) {
  if (expr == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return false;
  }
  if (expr->op == AST_OP(call)) {
    VectorASTNode* call = (VectorASTNode*)expr;
    return call->left != NULL && TypeIsStructOrUnion(call->left->type) &&
           call->left->type->info.struct_info == type->info.struct_info;
  }
  if (expr->op != AST_OP(comma)) {
    return false;
  }
  BinaryASTNode* comma = (BinaryASTNode*)expr;
  if (comma->right == NULL || !TypeIsStructOrUnion(comma->right->type) ||
      comma->right->type->info.struct_info != type->info.struct_info ||
      comma->left == NULL || comma->left->op != AST_OP(call)) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)comma->left;
  if (call->left == NULL) {
    return false;
  }
  if (call->left->op == AST_OP(identifier)) {
    Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
    return callee != NULL && callee->type != NULL &&
           TypeIsFunction(callee->type) &&
           callee->type->info.function.is_constructor &&
           callee->type->info.function.cxx_member_owner ==
               type->info.struct_info;
  }
  if (call->left->op == AST_OP(dot)) {
    BinaryASTNode* member_access = (BinaryASTNode*)call->left;
    TypeRecord* receiver_type =
        member_access->left != NULL ? member_access->left->type : NULL;
    return TypeIsStructOrUnion(receiver_type) &&
           receiver_type->info.struct_info == type->info.struct_info;
  }
  return false;
}

/* True if the awaiter at this suspension point can be constructed in place in its
 * frame slot from the original initializer's constructor arguments (avoiding a
 * separate temporary + copy/move). */
static bool SuspensionPointCanDirectConstructFrameAwaiter(
    SuspensionPoint* point) {
  return point != NULL && point->kind == kSuspensionCoAwait &&
         point->awaiter != NULL && point->awaiter->type != NULL &&
         point->awaiter_init != NULL &&
         FindCXXConstructorForType(point->awaiter->type) != NULL &&
         CXXTemporaryConstructionActualsCanBeTaken(point->awaiter_init,
                                                  point->awaiter->type);
}

/* Ensure the awaiter at a suspension point can be placed in the frame: either it
 * has trivial lifetime, or it is copy/move-constructible, or it can be directly
 * constructed in place. Emit an error otherwise. */
static bool ValidateCoroutineSuspensionPointFrameAwaiterConstructible(
    SuspensionPoint* point) {
  if (point == NULL || point->kind != kSuspensionCoAwait ||
      point->awaiter == NULL) {
    return true;
  }
  TypeRecord* type = point->awaiter->type;
  if (!CoroutineFrameOwnedTypeNeedsCXXLifetime(type) ||
      FindCXXCopyConstructorForType(type) != NULL ||
      (point->awaiter_init != NULL &&
       FindCXXMoveConstructorForType(type) != NULL) ||
      SuspensionPointCanDirectConstructFrameAwaiter(point)) {
    return true;
  }
  SemanticError(point->co_await,
                "coroutine frame-owned awaiter requires a copy or move "
                "constructor");
  return false;
}

/* True if a type can be copy-constructed (trivial, or has a copy constructor). */
static bool CoroutineTypeCanBeCopyConstructed(TypeRecord* type) {
  return !CoroutineFrameOwnedTypeNeedsCXXLifetime(type) ||
         FindCXXCopyConstructorForType(type) != NULL;
}

/* True if a non-trivial type has a move constructor. */
static bool CoroutineTypeCanBeMoveConstructed(TypeRecord* type) {
  return CoroutineFrameOwnedTypeNeedsCXXLifetime(type) &&
         FindCXXMoveConstructorForType(type) != NULL;
}

/* True if storing into the frame should move rather than copy: the type is
 * non-trivial, has no copy constructor, but does have a move constructor. */
static bool CoroutineFrameAwaiterShouldMove(TypeRecord* type) {
  return CoroutineFrameOwnedTypeNeedsCXXLifetime(type) &&
         FindCXXCopyConstructorForType(type) == NULL &&
         FindCXXMoveConstructorForType(type) != NULL;
}

/* Build `static_cast<T&&>(symbol)`: an xvalue expression that moves `symbol`. */
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

/* Turn an expression into an xvalue without moving it out of its storage.
 * Coroutine yield temporaries use this after being materialized in the frame so
 * an rvalue-reference yield_value overload binds to the frame-resident object. */
static ASTNode* NewCoroutineMoveValue(ASTNode* value,
                                      SourceLocation location) {
  if (value == NULL || value->type == NULL) {
    return value;
  }
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, true);
  TypeRecordChain(ref, TypeRecordCopy(value->type));
  ref = TypeRecordCalculateSize(ref);
  ASTNode* cast = NewCastASTNode(ref, location, value);
  ((CastASTNode*)cast)->kind = kCastStatic;
  cast->value_category = kValueCategoryXvalue;
  return cast;
}

/* Produce the initializer expression used to store `symbol` into its frame slot:
 * a move when the type is move-only, otherwise a plain reference (copy). */
static ASTNode* NewCoroutineFrameAwaiterInitializer(Symbol* symbol,
                                                   SourceLocation location) {
  if (symbol == NULL) {
    return NULL;
  }
  if (CoroutineFrameAwaiterShouldMove(symbol->type)) {
    return NewCoroutineMoveExpression(symbol, location);
  }
  return NewIdentifierASTNode(symbol, location);
}

/* Build a statement that constructs a frame member in place:
 * `frame->member.Ctor(actuals...);`. Returns NULL if the type has no
 * constructor. */
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

/* Build a default-construction statement for a frame member, or NULL if the type
 * has no default constructor. */
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

/* Initialize a frame member from `value`: construct it (if the type has a
 * constructor), else assign it, else default-construct it; then set its
 * "constructed" flag slot if present. Returns a single statement or a compound. */
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

/* Initialize a frame member by direct in-place construction from `actuals`, then
 * set its "constructed" flag slot if present. */
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

/* Steal the constructor arguments out of a temporary-construction expression of
 * `type` (a constructor call, or `(ctor(...), object)` comma), leaving the call
 * argument-less, and return them so they can construct a frame slot directly.
 * Returns NULL if `expr` is not such a construction. */
static Vector* TakeCXXTemporaryConstructionActuals(ASTNode* expr,
                                                   TypeRecord* type) {
  if (expr == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return NULL;
  }
  /* An aggregate awaiter (`T{...}`) has no user-declared constructor: its
   * `T(args)`-shaped construction node is aggregate initialization, not a
   * constructor call whose arguments can be replayed as an in-place
   * `frame->slot.T(args)` construction (that would fail overload resolution
   * against the implicit special members).  Leave the whole node intact so the
   * frame slot is copy-initialized from the aggregate temporary instead. */
  if (type->info.struct_info->is_aggregate) {
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

/* Build a statement `frame->member.~Tag();` to destroy a frame member, or NULL if
 * the type has no destructor. */
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

/* Build a statement `symbol.~Tag();` to destroy a (non-frame) local, or NULL if
 * the type has no destructor. */
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

/* Build `&function` typed as `pointer_type` (used to fill in the frame's resume/
 * destroy function-pointer slots). */
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

/* Declare the ramp's return object, initialized exactly once from
 * `promise.get_return_object()`. */
static ASTNode* NewCoroutineReturnObjectDeclaration(Symbol* promise,
                                                    Symbol* return_object,
                                                    SourceLocation location) {
  Vector* decls = NewVector();
  VectorAppend(decls,
               NewVariableDeclarationASTNode(
                   return_object,
                   NewExpressionInitializerASTNode(
                       NewCoroutinePromiseMemberCall(
                           promise, "get_return_object", NewVector(), location),
                       location),
                   location));
  return NewDeclarationListASTNode(decls, location);
}

/* Return the ramp's previously-created return object. A null symbol is used
 * while building the void-returning resume function. */
static ASTNode* NewCoroutineReturnObjectStatement(Symbol* return_object,
                                                  SourceLocation location) {
  Vector* statements = NewVector();
  ASTNode* return_stmt =
      NewCombinedStatementASTNode(AST_OP(return),
                                  return_object != NULL
                                      ? NewCoroutineMoveExpression(return_object,
                                                                   location)
                                      : NULL,
                                  NULL, location);
  return_stmt->flags |= kASTCoroutineLoweredReturn;
  VectorAppend(statements, return_stmt);
  return NewCompoundStatementASTNode(statements, location);
}

/* Find the promise's static `get_return_object_on_allocation_failure()` member,
 * if defined (enables the nothrow allocation path). */
static StructMember* FindCoroutineAllocationFailureMember(
    TypeRecord* promise_type) {
  StructMember* member =
      FindCoroutinePromiseMember(promise_type,
                                 "get_return_object_on_allocation_failure");
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->is_static &&
        candidate->symbol != NULL && candidate->symbol->type != NULL &&
        TypeIsFunction(candidate->symbol->type) &&
        candidate->symbol->type->info.function.prototype.length == 0 &&
        candidate->symbol->type->next != NULL) {
      return candidate;
    }
  }
  return NULL;
}

/* Build the early return used when frame allocation fails: declare a temporary
 * initialized from the promise's on-allocation-failure factory and return it. */
static ASTNode* NewCoroutineAllocationFailureReturnStatement(
    StructMember* failure_member,
    TypeRecord* return_type,
    SourceLocation location) {
  ASTNode* return_object = NewVectorASTNode(
      AST_OP(call), TypeRecordCopy(return_type), location,
      NewIdentifierASTNode(failure_member->symbol, location), NewVector());
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

/* Build `if (frame == nullptr) return on-allocation-failure-object;` to guard the
 * coroutine body when the frame allocation returned null. */
static ASTNode* NewCoroutineAllocationFailureIf(CoroutineFrame* frame,
                                                StructMember* failure_member,
                                                TypeRecord* return_type,
                                                SourceLocation location) {
  ASTNode* condition =
      NewBinaryASTNode(AST_OP(equal),
                       NewTypeRecordWithSize(kTypeBool, kQualPlain),
                       location,
                       NewIdentifierASTNode(frame->symbol, location),
                       NewNullPointerConstant(location));
  return NewIfStatementASTNode(
      condition,
      NewCoroutineAllocationFailureReturnStatement(failure_member, return_type,
                                                  location),
      NULL, false, location);
}

/* Lower an always-ready co_await to just `awaiter.await_resume()` (no suspend). */
static ASTNode* LowerReadyCoAwaitExpression(UnaryASTNode* co_await) {
  SourceLocation location = co_await->base.location;
  ASTNode* awaiter = ASTNodeMove(co_await->sub);
  ASTNode* await_resume =
      NewAwaiterMemberCall(awaiter, "await_resume", location);
  ASTNodeSetType(await_resume, co_await->base.type);
  return await_resume;
}

/* AST visitor that replaces every always-ready co_await with its await_resume
 * call in place. */
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

/* AST visitor that records coroutine-defining keywords (co_return/co_await/
 * co_yield) and counts the suspension points into the CoroutineScan in `data`. */
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

/* Resolve the promise type as the nested `return_type::promise_type` typedef, if
 * the coroutine's return type defines one directly. */
static TypeRecord* ResolveDirectCoroutinePromiseType(TypeRecord* return_type) {
  if (return_type == NULL || !TypeIsStructOrUnion(return_type) ||
      return_type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(return_type->info.struct_info, "promise_type");
  if (member == NULL || member->symbol == NULL ||
      !StorageIs(member->symbol->storage, STO(typedef)) ||
      member->symbol->type == NULL) {
    return NULL;
  }
  return TypeRecordCalculateSize(TypeRecordCopy(member->symbol->type));
}

/* Wrap a type as a template type-argument (used to instantiate
 * std::coroutine_traits<Return, Args...>). */
static TemplateArgument* NewCoroutineTypeTemplateArgument(TypeRecord* type) {
  if (type == NULL) {
    return NULL;
  }
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  assert(arg != NULL);
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCopy(type);
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

/* Resolve the promise type via std::coroutine_traits: instantiate
 * coroutine_traits<ReturnType, ParamTypes...> and read its ::promise_type. This
 * is the standard customization point and takes priority over the direct
 * return-type::promise_type lookup. Returns NULL if std::coroutine_traits is not
 * available or doesn't apply. */
static TypeRecord* ResolveCoroutineTraitsPromiseType(TypeRecord* function_type) {
  if (!CompilerIsCXX() || function_type == NULL ||
      !TypeIsFunction(function_type) || function_type->next == NULL) {
    return NULL;
  }
  Namespace* std_ns = NamespaceFindStdNamespace();
  if (std_ns == NULL) {
    return NULL;
  }

  String traits_name;
  StringInit(&traits_name, "coroutine_traits");
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(std_ns, &traits_name);
  Symbol* traits = result.status == kInlineLookupUnique ? result.symbol : NULL;
  StringDestruct(&traits_name);
  if (traits == NULL || !traits->flags.is_template ||
      traits->type == NULL || !TypeIsStructOrUnion(traits->type)) {
    return NULL;
  }

  Vector* args = NewVector();
  VectorAppend(args, NewCoroutineTypeTemplateArgument(function_type->next));
  FunctionInfo* info = &function_type->info.function;
  for (size_t i = 0; i < info->prototype.length; i++) {
    Symbol* formal = info->prototype.value.p[i];
    if (formal == NULL || formal->type == NULL) {
      continue;
    }
    VectorAppend(args, NewCoroutineTypeTemplateArgument(formal->type));
  }
  Struct* traits_struct = traits->type->info.struct_info;
  bool traits_has_pack = false;
  for (size_t i = 0; traits_struct != NULL &&
                     i < traits_struct->template_parameters.length; i++) {
    TemplateParameter* param = traits_struct->template_parameters.value.p[i];
    if (param != NULL && param->is_parameter_pack) {
      traits_has_pack = true;
      break;
    }
  }
  if (traits_struct == NULL ||
      (!traits_has_pack &&
       args->length > traits_struct->template_parameters.length)) {
    VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    return NULL;
  }
  TypeRecord* traits_type =
      TypeInstantiateClassTemplate(&compiler->syntax, traits, args);
  VectorDeleteWithContents(args, (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  TypeRecord* promise_type = ResolveDirectCoroutinePromiseType(traits_type);
  TypeRecordDelete(traits_type);
  return promise_type;
}

/* Resolve a coroutine's promise type: prefer std::coroutine_traits, then fall
 * back to the return type's own nested promise_type. */
static TypeRecord* ResolveCoroutinePromiseType(TypeRecord* function_type) {
  TypeRecord* promise = ResolveCoroutineTraitsPromiseType(function_type);
  if (promise != NULL) {
    return promise;
  }
  return function_type != NULL && TypeIsFunction(function_type)
             ? ResolveDirectCoroutinePromiseType(function_type->next)
             : NULL;
}

/* Look up a member function `name` on the promise type (e.g. initial_suspend,
 * return_value, unhandled_exception), skipping non-functions. */
static StructMember* FindCoroutinePromiseMember(TypeRecord* promise_type,
                                                const char* name) {
  if (promise_type == NULL || !TypeIsStructOrUnion(promise_type) ||
      promise_type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(promise_type->info.struct_info, name);
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function) {
      return candidate;
    }
  }
  return NULL;
}

/* Require a promise member to exist, emitting an error if it is missing. */
static bool RequireCoroutinePromiseMember(ASTNode* node, TypeRecord* promise,
                                          const char* name) {
  if (FindCoroutinePromiseMember(promise, name) != NULL) {
    return true;
  }
  SemanticError(node, "coroutine promise_type is missing %s", name);
  return false;
}

/* Return type of a promise member function `name`, or NULL if absent. */
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

/* Validate that a promise member `name` (initial_suspend/final_suspend) returns a
 * valid awaiter type. */
static bool ValidateCoroutinePromiseAwaiterReturn(ASTNode* node,
                                                  TypeRecord* promise,
                                                  const char* name) {
  TypeRecord* awaiter_type = CoroutinePromiseMemberReturnType(promise, name);
  if (awaiter_type == NULL) {
    SemanticError(node, "coroutine %s return type is invalid", name);
    return false;
  }
  if (!TypeIsStructOrUnion(awaiter_type) ||
      awaiter_type->info.struct_info == NULL) {
    SemanticError(node, "coroutine %s return type is invalid", name);
    return false;
  }
  return ValidateCoAwaiterType(node, awaiter_type);
}

/* Return the `index`-th user parameter of a promise member function, skipping
 * the implicit `this` for non-static members. */
static Symbol* CoroutinePromiseMemberUserFormal(StructMember* member,
                                                size_t index) {
  if (member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL ||
      !TypeIsFunction(member->symbol->type)) {
    return NULL;
  }
  FunctionInfo* info = &member->symbol->type->info.function;
  size_t formal_index = member->is_member_function ? index + 1 : index;
  if (formal_index >= info->prototype.length) {
    return NULL;
  }
  return info->prototype.value.p[formal_index];
}

/* Select the best single-parameter `promise.await_transform` overload for the
 * co_await operand. Sets *has_transform if any await_transform exists (so the
 * caller can distinguish "no transform" from "no viable transform"), and reports
 * ambiguity. */
static StructMember* FindCoroutinePromiseAwaitTransform(TypeRecord* promise,
                                                        ASTNode* operand,
                                                        bool* has_transform) {
  StructMember* member = FindCoroutinePromiseMember(promise, "await_transform");
  if (member != NULL && has_transform != NULL) {
    *has_transform = true;
  }
  StructMember* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  for (StructMember* candidate = member; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        candidate->symbol->type->next == NULL) {
      continue;
    }
    Symbol* formal = CoroutinePromiseMemberUserFormal(candidate, 0);
    if (formal == NULL ||
        CoroutinePromiseMemberUserFormal(candidate, 1) != NULL) {
      continue;
    }
    int score = CoAwaitOperatorFormalScore(formal->type, operand,
                                           operand != NULL ? operand->type
                                                           : NULL);
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
    SemanticError(operand, "Ambiguous overload for await_transform");
    return NULL;
  }
  return best;
}

/* If the promise defines await_transform, rewrite the co_await operand to
 * `promise.await_transform(operand)`. Sets *error and returns false if a
 * transform exists but none is viable. */
static bool ApplyPromiseAwaitTransform(UnaryASTNode* co_await,
                                       Symbol* promise,
                                       bool* error) {
  if (!CompilerIsCXX() || co_await == NULL || co_await->sub == NULL ||
      promise == NULL || promise->type == NULL) {
    return false;
  }
  bool has_transform = false;
  StructMember* member = FindCoroutinePromiseAwaitTransform(
      promise->type, co_await->sub, &has_transform);
  if (member == NULL || member->symbol == NULL ||
      member->symbol->type == NULL || !TypeIsFunction(member->symbol->type) ||
      member->symbol->type->next == NULL) {
    if (has_transform) {
      SemanticError(co_await->sub, "No viable overload for await_transform");
      if (error != NULL) {
        *error = true;
      }
    }
    return false;
  }
  Vector* actuals = NewVector();
  VectorAppend(actuals, ASTNodeMove(co_await->sub));
  ASTNode* transformed = NewCoroutinePromiseMemberCall(
      promise, "await_transform", actuals, co_await->base.location);
  ASTNodeSetType(transformed, TypeRecordCopy(member->symbol->type->next));
  ASTNodeReplaceChild((ASTNode*)co_await, 0, transformed, true);
  return true;
}

/* True if an awaiter type's await_ready is statically always-true (used to elide
 * suspension for initial/final suspends like suspend_never). */
static bool AwaiterTypeIsAlwaysReady(TypeRecord* awaiter_type) {
  StructMember* await_ready = FindAwaiterMember(awaiter_type, "await_ready");
  return await_ready != NULL && await_ready->symbol != NULL &&
         await_ready->symbol->type != NULL &&
         TypeIsFunction(await_ready->symbol->type) &&
         FunctionBodyIsReturnTrue(await_ready->symbol->type->info.function.body);
}

/* Return type of an awaiter's await_suspend (void/bool/handle), which determines
 * how the suspend is lowered. */
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

/* Build a call to a promise member function: `promise.member_name(actuals...)`. */
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

/* Build the final-suspend sequence: evaluate `promise.final_suspend()`, and
 * unless the awaiter is always-ready, store it in the frame's final-awaiter slot
 * and emit `if (!awaiter.await_ready()) { await_suspend(...); return; }`. */
static ASTNode* NewFinalSuspendStatement(Symbol* promise,
                                         CoroutineFrame* frame,
                                         Symbol* final_awaiter,
                                         Symbol* return_object,
                                         SourceLocation location) {
  ASTNode* final_suspend = NewCoroutinePromiseMemberCall(
      promise, "final_suspend", NewVector(), location);
  TypeRecord* final_awaiter_type =
      promise != NULL ? CoroutinePromiseMemberReturnType(promise->type,
                                                         "final_suspend")
                      : NULL;
  if (frame == NULL || final_awaiter_type == NULL ||
      AwaiterTypeIsAlwaysReady(final_awaiter_type)) {
    return NewExpressionStatementASTNode(final_suspend, location);
  }
  if (frame->final_awaiter == NULL || final_awaiter == NULL) {
    return NewExpressionStatementASTNode(final_suspend, location);
  }

  ASTNode* ready_call =
      NewAwaiterMemberCall(NewIdentifierASTNode(final_awaiter, location),
                           "await_ready", location);
  ASTNode* not_ready =
      NewBinaryASTNode(AST_OP(equal), NewTypeRecordWithSize(kTypeBool,
                                                            kQualPlain),
                       location, ready_call,
                       NewIntConstantASTNode(
                           0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  Vector* suspend_actuals = NewVector();
  VectorAppend(suspend_actuals,
               NewAwaitSuspendHandleArgument(frame, final_awaiter, location));
  ASTNode* await_suspend = NewAwaiterMemberCallWithActuals(
      NewIdentifierASTNode(final_awaiter, location), "await_suspend",
      suspend_actuals, location);
  TypeRecord* await_suspend_return = AwaiterAwaitSuspendReturnType(final_awaiter);
  if (await_suspend_return != NULL) {
    ASTNodeSetType(await_suspend, TypeRecordCopy(await_suspend_return));
  }
  Vector* suspend_statements = NewVector();
  AppendCoroutineAwaitSuspendReturn(suspend_statements, frame, await_suspend,
                                    await_suspend_return, return_object,
                                    location);

  Vector* statements = NewVector();
  VectorAppend(statements,
               NewFrameMemberInitialization(frame, frame->final_awaiter,
                                            frame->final_awaiter_constructed,
                                            final_suspend, location));
  VectorAppend(statements,
               NewIfStatementASTNode(
                   not_ready,
                   NewCompoundStatementASTNode(suspend_statements, location),
                   NULL, false, location));
  return NewCompoundStatementASTNode(statements, location);
}

/* Lower a `co_return [expr]` into: call promise.return_value(expr) (or
 * return_void()), mark the frame done, null the resume pointer, run the final
 * suspend, and produce the return object. */
static ASTNode* LowerCoReturnStatement(CombinedStatementASTNode* co_return,
                                       Symbol* promise,
                                       CoroutineFrame* frame,
                                       Vector* persisted_locals,
                                       Symbol* final_awaiter,
                                       Symbol* return_object) {
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
  AppendPersistedCoroutineLocalDestructors(
      statements, frame, persisted_locals, (ASTNode*)co_return, location);
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
               NewFinalSuspendStatement(promise, frame, final_awaiter,
                                        return_object, location));
  VectorAppend(statements,
               NewCoroutineReturnObjectStatement(return_object, location));
  return NewCompoundStatementASTNode(statements, location);
}

/* Build the handler body for an escaping exception: destroy frame-owned objects,
 * call promise.unhandled_exception(), mark the frame done, run final suspend, and
 * return the return object. */
static ASTNode* NewCoroutineUnhandledExceptionStatement(
    Symbol* promise,
    CoroutineFrame* frame,
    Symbol* final_awaiter,
    Symbol* return_object,
    SourceLocation location) {
  Vector* statements = NewVector();
  AppendCoroutineFrameBodyDestructors(statements, frame, location);
  VectorAppend(statements,
               NewExpressionStatementASTNode(
                   NewCoroutinePromiseMemberCall(promise,
                                                 "unhandled_exception",
                                                 NewVector(), location),
                   location));
  if (frame != NULL) {
    VectorAppend(statements,
                 NewFrameIntAssignment(frame, frame->state, 0, location));
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
               NewFinalSuspendStatement(promise, frame, final_awaiter,
                                        return_object, location));
  VectorAppend(statements,
               NewCoroutineReturnObjectStatement(return_object, location));
  return NewCompoundStatementASTNode(statements, location);
}

/* Context carried through the co_return/throw lowering transforms: the objects
 * needed to build promise calls and final-suspend code. */
typedef struct {
  Symbol* promise;
  CoroutineFrame* frame;
  Vector* persisted_locals;
  Symbol* final_awaiter;
  Symbol* return_object;
} CoroutineStatementLowering;

/* Transform callback: rewrite each co_return statement via
 * LowerCoReturnStatement. */
static ASTNode* LowerCoReturnTransform(ASTNode* node, void* data,
                                       ASTNodeTransformAction* action) {
  (void)action;
  if (node == NULL || node->op != AST_OP(co_return)) {
    return node;
  }
  CoroutineStatementLowering* lowering = data;
  return LowerCoReturnStatement((CombinedStatementASTNode*)node,
                                lowering->promise, lowering->frame,
                                lowering->persisted_locals,
                                lowering->final_awaiter,
                                lowering->return_object);
}

/* Lower all co_return statements within `node` to their promise/final-suspend
 * sequences. */
static void LowerCoReturnsInStatement(ASTNode* node, Symbol* promise,
                                      CoroutineFrame* frame,
                                      Vector* persisted_locals,
                                      Symbol* final_awaiter,
                                      Symbol* return_object) {
  CoroutineStatementLowering lowering = {
      promise, frame, persisted_locals, final_awaiter,
      return_object};
  ASTNodeVisitAndTransform(node, LowerCoReturnTransform, &lowering);
}

/* True if a statement is a bare `throw ...;` expression statement. */
static bool IsThrowExpressionStatement(ASTNode* node) {
  return node != NULL && node->op == AST_OP(expr) &&
         ((ExpressionStatementASTNode*)node)->expr != NULL &&
         ((ExpressionStatementASTNode*)node)->expr->op == AST_OP(throw);
}

/* Replace a top-level `throw;` (one not caught inside the body) with the
 * unhandled-exception sequence, since an uncaught throw in a coroutine routes
 * through promise.unhandled_exception(). */
static ASTNode* LowerCoroutineThrowStatement(ASTNode* throw_stmt,
                                             Symbol* promise,
                                             CoroutineFrame* frame,
                                             Symbol* final_awaiter,
                                             Symbol* return_object) {
  return NewCoroutineUnhandledExceptionStatement(
      promise, frame, final_awaiter, return_object,
      throw_stmt != NULL ? throw_stmt->location : promise->location);
}

/* Transform callback for top-level throws. Skips into try statements (their
 * throws are handled by the body's own catch clauses, not the coroutine). */
static ASTNode* LowerCoroutineThrowTransform(ASTNode* node, void* data,
                                             ASTNodeTransformAction* action) {
  if (node != NULL && node->op == AST_OP(try)) {
    *action = kASTTransformSkipChildren;
    return node;
  }
  if (!IsThrowExpressionStatement(node)) {
    return node;
  }
  CoroutineStatementLowering* lowering = data;
  return LowerCoroutineThrowStatement(node, lowering->promise,
                                      lowering->frame,
                                      lowering->final_awaiter,
                                      lowering->return_object);
}

/* Lower all uncaught top-level throw statements within `node`. */
static void LowerCoroutineThrowsInStatement(ASTNode* node, Symbol* promise,
                                            CoroutineFrame* frame,
                                            Symbol* final_awaiter,
                                            Symbol* return_object) {
  CoroutineStatementLowering lowering = {
      promise, frame, NULL, final_awaiter, return_object};
  ASTNodeVisitAndTransform(node, LowerCoroutineThrowTransform, &lowering);
}

/* Peel initializer wrappers (expression/braced/designated) down to the single
 * underlying initializing expression. */
static ASTNode* UnwrapExpressionInitializer(ASTNode* initializer) {
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer != NULL && initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    if (braced->initializers != NULL && braced->initializers->length == 1) {
      return UnwrapExpressionInitializer(braced->initializers->value.p[0]);
    }
  }
  if (initializer != NULL && initializer->op == AST_OP(designated_init)) {
    return ((DesignatedInitializerASTNode*)initializer)->init;
  }
  return initializer;
}

/* Extract the initializing expression from a variable declaration's initializer
 * node (handling the `name = init` wrapper). */
static ASTNode* VariableInitializerExpression(ASTNode* initializer) {
  if (initializer != NULL && initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  return UnwrapExpressionInitializer(initializer);
}

/* State for searching a subtree for nested suspending co_await/co_yield (other
 * than `root` itself): the first one found, how many, and whether any yields a
 * void result. */
typedef struct {
  ASTNode* root;
  ASTNode* found;
  int count;
  bool void_result;
} SuspendedCoroutineExpressionSearch;

/* Visitor that records nested suspending co_await expressions (skipping the
 * search root and always-ready awaits). */
static void FindNestedSuspendingCoAwait(ASTNode* node, void* data,
                                        int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(co_await)) {
    return;
  }
  SuspendedCoroutineExpressionSearch* search = data;
  if (node == search->root || CoAwaitIsAlwaysReady(node)) {
    return;
  }
  search->count++;
  if (search->found == NULL) {
    search->found = node;
  }
  if (TypeIsVoid(node->type)) {
    search->void_result = true;
  }
}

/* Visitor that records nested co_yield expressions (skipping the search root). */
static void FindNestedSuspendingCoYield(ASTNode* node, void* data,
                                        int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(co_yield)) {
    return;
  }
  SuspendedCoroutineExpressionSearch* search = data;
  if (node == search->root) {
    return;
  }
  search->count++;
  if (search->found == NULL) {
    search->found = node;
  }
  if (TypeIsVoid(node->type)) {
    search->void_result = true;
  }
}

/* State for checking whether a suspension sits under an operator with
 * conditional/sequenced evaluation (&&, ||, ?:, comma) between it and `root`. */
typedef struct {
  ASTNode* root;
  ASTNode* stop_parent;
  bool unsafe;
} CoroutineUnsafeSplitSearch;

/* Upward visitor: flags `unsafe` if an ancestor is a short-circuit/sequencing
 * operator, meaning the suspension can't be naively hoisted out of the
 * expression without changing evaluation order. */
static bool FindUnsafeSplitAncestor(ASTNode* node, void* data) {
  CoroutineUnsafeSplitSearch* search = data;
  if (node == NULL || node == search->stop_parent) {
    return false;
  }
  if (node->op == AST_OP(logand) || node->op == AST_OP(logor) ||
      node->op == AST_OP(question) || node->op == AST_OP(colon) ||
      node->op == AST_OP(comma)) {
    search->unsafe = true;
    return false;
  }
  return node != search->root;
}

/* True if `suspension` is nested under a short-circuit/sequencing operator within
 * the expression rooted at `root`. */
static bool SuspensionHasUnsafeSplitAncestor(ASTNode* root, ASTNode* suspension) {
  CoroutineUnsafeSplitSearch search = {
      .root = root,
      .stop_parent = root != NULL ? root->parent : NULL,
  };
  ASTNodeVisitUpwards(suspension != NULL ? suspension->parent : NULL,
                      FindUnsafeSplitAncestor, &search);
  return search.unsafe;
}

/* Context for rewriting a short-circuit/conditional expression that contains a
 * suspension into a temporary-backed form whose arms can be split across the
 * suspension. */
typedef struct {
  Symbol* result;
  ASTNode* lowering;
  SourceLocation location;
  ASTOpcode op;
} ShortCircuitSuspensionTransform;

/* True for operators with conditional/sequenced evaluation (&&, ||, ?:, comma). */
static bool ASTNodeIsShortCircuitExpression(ASTNode* node) {
  return node != NULL &&
         (node->op == AST_OP(logand) || node->op == AST_OP(logor) ||
          node->op == AST_OP(question) || node->op == AST_OP(comma));
}

/* True if `expr` contains a suspending co_await anywhere within it. */
static bool ASTContainsSuspendingCoAwait(ASTNode* expr) {
  if (expr == NULL) {
    return false;
  }
  SuspendedCoroutineExpressionSearch search = {NULL, NULL, 0, false};
  ASTNodeVisit(expr, FindNestedSuspendingCoAwait, 0, &search);
  return search.count > 0;
}

/* True if `expr` contains a co_yield anywhere within it. */
static bool ASTContainsSuspendingCoYield(ASTNode* expr) {
  if (expr == NULL) {
    return false;
  }
  SuspendedCoroutineExpressionSearch search = {NULL, NULL, 0, false};
  ASTNodeVisit(expr, FindNestedSuspendingCoYield, 0, &search);
  return search.count > 0;
}

/* True if `expr` contains any suspending coroutine subexpression. */
static bool ASTContainsSuspendingCoroutineExpression(ASTNode* expr) {
  return ASTContainsSuspendingCoAwait(expr) || ASTContainsSuspendingCoYield(expr);
}

/* True if a scalar temporary of `type` can be used to hold a short-circuit
 * sub-result (excludes void/class/array/function/reference types). */
static bool CoroutineShortCircuitTemporaryTypeIsSupported(TypeRecord* type) {
  return type != NULL && !TypeIsVoid(type) && !TypeIsStructOrUnion(type) &&
         !TypeIsArray(type) && !TypeIsFunction(type) &&
         !TypeIsReference(type);
}

/* The type of the value an expression produces, accounting for co_await/co_yield
 * result types rather than the node's own type. */
static TypeRecord* CoroutineExpressionTemporaryType(ASTNode* expr) {
  if (expr == NULL) {
    return NULL;
  }
  if (expr->op == AST_OP(co_await)) {
    return CoAwaitResultType(expr);
  }
  if (expr->op == AST_OP(co_yield)) {
    return CoYieldResultType(expr);
  }
  return expr->type;
}

/* The result type of a short-circuit/conditional expression (bool for &&/||, the
 * common arm type for ?:, the right operand type for comma). */
static TypeRecord* CoroutineShortCircuitTemporaryType(ASTNode* expr) {
  if (expr == NULL) {
    return NULL;
  }
  if (expr->op == AST_OP(logand) || expr->op == AST_OP(logor)) {
    return NewTypeRecordWithSize(kTypeBool, kQualPlain);
  }
  if (expr->op == AST_OP(comma)) {
    return CoroutineExpressionTemporaryType(((BinaryASTNode*)expr)->right);
  }
  if (expr->op == AST_OP(question)) {
    BinaryASTNode* question = (BinaryASTNode*)expr;
    BinaryASTNode* colon = (BinaryASTNode*)question->right;
    if (colon == NULL || colon->base.op != AST_OP(colon)) {
      return expr->type;
    }
    TypeRecord* true_type = CoroutineExpressionTemporaryType(colon->left);
    TypeRecord* false_type = CoroutineExpressionTemporaryType(colon->right);
    if (true_type != NULL && false_type != NULL &&
        TypeEqual(true_type, false_type)) {
      return true_type;
    }
  }
  return expr->type;
}

/* Create a fresh local temporary symbol of `type` (used to hold suspension or
 * short-circuit sub-results that must persist across statements). */
static Symbol* NewCoroutineExpressionTemporary(TypeRecord* type,
                                               SourceLocation location) {
  TypeRecord* temp_type = TypeRecordCopy(type);
  temp_type = TypeRecordCalculateSize(temp_type);
  Symbol* result = SyntaxNewTemporary(&compiler->syntax, temp_type);
  result->flags.is_local = true;
  result->flags.is_defined = true;
  result->location = location;
  return result;
}

/* Build a declaration-list statement declaring `result` with optional
 * initializer `init`. */
static ASTNode* NewCoroutineTemporaryDeclaration(Symbol* result, ASTNode* init,
                                                 SourceLocation location) {
  Vector* declarations = NewVector();
  VectorAppend(
      declarations,
      NewVariableDeclarationASTNode(
          result,
          init != NULL ? NewExpressionInitializerASTNode(init, location) : NULL,
          location));
  return NewDeclarationListASTNode(declarations, location);
}

/* Build a statement `result = value;`. */
static ASTNode* NewCoroutineTemporaryAssignment(Symbol* result, ASTNode* value,
                                                SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), TypeRecordCopy(result->type), location,
                       NewIdentifierASTNode(result, location), value),
      location);
}

/* Build a bool literal node. */
static ASTNode* NewCoroutineBoolConstant(bool value, SourceLocation location) {
  return NewIntConstantASTNode(value ? 1 : 0,
                               NewTypeRecordWithSize(kTypeBool, kQualPlain),
                               location);
}

/* Wrap a single statement in a `{ stmt }` compound. */
static ASTNode* NewCoroutineSingleStatementCompound(ASTNode* stmt,
                                                    SourceLocation location) {
  Vector* statements = NewVector();
  VectorAppend(statements, stmt);
  return NewCompoundStatementASTNode(statements, location);
}

/* Detach and return a binary node's left/right child, clearing the parent link
 * so it can be re-parented elsewhere. */
static ASTNode* TakeCoroutineBinaryChild(BinaryASTNode* node, bool right) {
  ASTNode** child = right ? &node->right : &node->left;
  ASTNode* result = *child;
  *child = NULL;
  if (result != NULL) {
    result->parent = NULL;
    result->child_id = -1;
  }
  return result;
}

/* Rewrite a short-circuit/conditional expression `expr` into statement form that
 * assigns its result into `result`, preserving evaluation order so an embedded
 * suspension can later be split out: comma -> sequence, && / || / ?: -> if/else
 * that assigns the appropriate arm. */
static ASTNode* NewCoroutineShortCircuitLowering(Symbol* result,
                                                 ASTNode* expr) {
  SourceLocation location = expr->location;
  BinaryASTNode* binary = (BinaryASTNode*)expr;
  if (expr->op == AST_OP(comma)) {
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatementASTNode(
                                               TakeCoroutineBinaryChild(binary,
                                                                        false),
                                               location));
    VectorAppend(statements,
                 NewCoroutineTemporaryAssignment(result,
                                                 TakeCoroutineBinaryChild(
                                                     binary, true),
                                                 location));
    return NewCompoundStatementASTNode(statements, location);
  }

  if (expr->op == AST_OP(logand)) {
    ASTNode* left = TakeCoroutineBinaryChild(binary, false);
    ASTNode* right = TakeCoroutineBinaryChild(binary, true);
    ASTNode* then_stmt = NewCoroutineTemporaryAssignment(result, right,
                                                        location);
    ASTNode* else_stmt =
        NewCoroutineTemporaryAssignment(result,
                                        NewCoroutineBoolConstant(false,
                                                                 location),
                                        location);
    return NewIfStatementASTNode(
        left, NewCoroutineSingleStatementCompound(then_stmt, location),
        NewCoroutineSingleStatementCompound(else_stmt, location), false,
        location);
  }

  if (expr->op == AST_OP(logor)) {
    ASTNode* left = TakeCoroutineBinaryChild(binary, false);
    ASTNode* right = TakeCoroutineBinaryChild(binary, true);
    ASTNode* then_stmt =
        NewCoroutineTemporaryAssignment(result,
                                        NewCoroutineBoolConstant(true,
                                                                 location),
                                        location);
    ASTNode* else_stmt = NewCoroutineTemporaryAssignment(result, right,
                                                        location);
    return NewIfStatementASTNode(
        left, NewCoroutineSingleStatementCompound(then_stmt, location),
        NewCoroutineSingleStatementCompound(else_stmt, location), false,
        location);
  }

  if (expr->op == AST_OP(question)) {
    BinaryASTNode* colon = (BinaryASTNode*)binary->right;
    if (colon == NULL || colon->base.op != AST_OP(colon)) {
      return NULL;
    }
    ASTNode* cond = TakeCoroutineBinaryChild(binary, false);
    ASTNode* true_arm = TakeCoroutineBinaryChild(colon, false);
    ASTNode* false_arm = TakeCoroutineBinaryChild(colon, true);
    binary->right = NULL;
    ASTNode* then_stmt = NewCoroutineTemporaryAssignment(result, true_arm,
                                                        location);
    ASTNode* else_stmt = NewCoroutineTemporaryAssignment(result, false_arm,
                                                        location);
    return NewIfStatementASTNode(
        cond, NewCoroutineSingleStatementCompound(then_stmt, location),
        NewCoroutineSingleStatementCompound(else_stmt, location), false,
        location);
  }

  return NULL;
}

/* Transform callback: when it finds the first short-circuit expression that both
 * supports a scalar temporary and contains a suspension, replace it with a
 * reference to a new temporary and remember the statement form to insert. */
static ASTNode* LowerShortCircuitSuspensionTransform(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  ShortCircuitSuspensionTransform* transform = data;
  if (node == NULL || transform->result != NULL) {
    return node;
  }
  TypeRecord* temp_type = CoroutineShortCircuitTemporaryType(node);
  if (!ASTNodeIsShortCircuitExpression(node) ||
      !CoroutineShortCircuitTemporaryTypeIsSupported(temp_type) ||
      !ASTContainsSuspendingCoroutineExpression(node)) {
    return node;
  }
  transform->location = node->location;
  transform->op = node->op;
  transform->result = NewCoroutineExpressionTemporary(temp_type,
                                                      node->location);
  transform->lowering = NewCoroutineShortCircuitLowering(transform->result,
                                                         node);
  *action = kASTTransformSkipChildren;
  return NewIdentifierASTNode(transform->result, node->location);
}

/* Hoist a suspension-containing short-circuit expression out of `expr`: replace
 * it with a temporary, then insert the temporary's declaration and the
 * if/else/sequence lowering before the current statement. Returns true if a
 * rewrite happened. */
static bool LowerShortCircuitSuspensionExpression(
    CompoundStatementASTNode* compound, size_t statement_index, ASTNode* expr) {
  if (compound == NULL || expr == NULL) {
    return false;
  }
  ASTNode* parent = expr->parent;
  int child_id = expr->child_id;
  ShortCircuitSuspensionTransform transform = {0};
  ASTNode* transformed = ASTNodeVisitAndTransform(
      expr, LowerShortCircuitSuspensionTransform, &transform);
  if (transform.result == NULL || transform.lowering == NULL) {
    return false;
  }
  if (transformed != expr && parent != NULL && child_id >= 0) {
    ASTNodeReplaceChild(parent, child_id, transformed, true);
  }

  CoroutineCompoundInsertStatement(
      compound, NewCoroutineTemporaryDeclaration(transform.result, NULL,
                                                transform.location),
      statement_index);
  CoroutineCompoundInsertStatement(compound, transform.lowering,
                                   statement_index + 1);
  ResetCompoundStatementParents(compound);
  return true;
}

/* Build `T result{ = suspension };` declaring the temporary that will hold a
 * suspension's await_resume()/yield result, using a braced/designated
 * initializer so the suspension is the initializing expression. */
static ASTNode* NewSuspensionResultTemporaryDeclaration(
    Symbol* result, ASTNode* suspension, SourceLocation location) {
  ASTNode* decl_id = NewIdentifierASTNode(result, location);
  decl_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* designated =
      NewDesignatedInitializerASTNode(NewVector(), suspension, location);
  ASTNodeSetType(designated, TypeRecordCopy(result->type));
  Vector* initializers = NewVector();
  VectorAppend(initializers, designated);
  ASTNode* braced = NewBracedInitializerASTNode(
      initializers, TypeRecordCopy(result->type), location);
  ASTNode* init =
      NewBinaryASTNode(AST_OP(init), TypeRecordCopy(result->type), location,
                       decl_id, braced);
  Vector* declarations = NewVector();
  VectorAppend(declarations, NewVariableDeclarationASTNode(result, init,
                                                           location));
  return NewDeclarationListASTNode(declarations, location);
}

/* The type produced by a co_await: its node type if known, else the awaiter's
 * await_resume() return type. */
static TypeRecord* CoAwaitResultType(ASTNode* co_await) {
  if (co_await == NULL || co_await->op != AST_OP(co_await)) {
    return NULL;
  }
  if (co_await->type != NULL) {
    return co_await->type;
  }
  TypeRecord* awaiter_type = CoAwaitOperandType(co_await);
  StructMember* await_resume =
      FindAwaiterMember(awaiter_type, "await_resume");
  if (await_resume == NULL || await_resume->symbol == NULL ||
      await_resume->symbol->type == NULL ||
      !TypeIsFunction(await_resume->symbol->type)) {
    return NULL;
  }
  return await_resume->symbol->type->next;
}

/* The type produced by a co_yield: its node type if known, else the
 * await_resume() return type of the awaiter from promise.yield_value(). */
static TypeRecord* CoYieldResultType(ASTNode* co_yield) {
  if (co_yield == NULL || co_yield->op != AST_OP(co_yield)) {
    return NULL;
  }
  if (co_yield->type != NULL) {
    return co_yield->type;
  }
  TypeRecord* promise_type =
      compiler->current_function != NULL
          ? compiler->current_function->info.function.coroutine_promise_type
          : NULL;
  TypeRecord* awaiter_type =
      CoroutinePromiseMemberReturnType(promise_type, "yield_value");
  StructMember* await_resume = FindAwaiterMember(awaiter_type, "await_resume");
  if (await_resume == NULL || await_resume->symbol == NULL ||
      await_resume->symbol->type == NULL ||
      !TypeIsFunction(await_resume->symbol->type)) {
    return NULL;
  }
  return await_resume->symbol->type->next;
}

/* Pull a non-void suspending co_await out of `expr`: replace it with a temporary,
 * and insert `T tmp{ = co_await ...};` before the statement so the value is
 * available after resumption. `allow_root_co_await` permits treating `expr`
 * itself as the suspension. Bails if the await sits under a sequencing operator
 * where splitting would change evaluation order. */
static bool SplitSuspendingCoAwaitExpression(CompoundStatementASTNode* compound,
                                             size_t statement_index,
                                             ASTNode* expr,
                                             bool allow_root_co_await) {
  if (compound == NULL || expr == NULL) {
    return false;
  }

  SuspendedCoroutineExpressionSearch search = {expr, NULL, 0, false};
  if (expr->op == AST_OP(co_await)) {
    if (!allow_root_co_await || CoAwaitIsAlwaysReady(expr)) {
      return false;
    }
    search.found = expr;
    search.count = 1;
    search.void_result = TypeIsVoid(expr->type);
  } else {
    ASTNodeVisit(expr, FindNestedSuspendingCoAwait, 0, &search);
  }
  TypeRecord* result_type = CoAwaitResultType(search.found);
  if (search.found == NULL || search.void_result || result_type == NULL ||
      TypeIsVoid(result_type) ||
      SuspensionHasUnsafeSplitAncestor(expr, search.found)) {
    return false;
  }

  SourceLocation location = search.found->location;
  TypeRecord* temp_type = TypeRecordCopy(result_type);
  temp_type = TypeRecordCalculateSize(temp_type);
  Symbol* result = SyntaxNewTemporary(&compiler->syntax, temp_type);
  result->flags.is_local = true;
  result->flags.is_defined = true;
  result->location = location;

  ASTNode* parent = search.found->parent;
  int child_id = search.found->child_id;
  ASTNode* co_await = ASTNodeMove(search.found);
  ASTNodeReplaceChild(parent, child_id, NewIdentifierASTNode(result, location),
                      false);
  CoroutineCompoundInsertStatement(
      compound,
      NewSuspensionResultTemporaryDeclaration(result, co_await, location),
      statement_index);
  ResetCompoundStatementParents(compound);
  return true;
}

/* Split only a co_await that is nested inside `expr` (not `expr` itself). */
static bool SplitNestedSuspendingCoAwaitExpression(
    CompoundStatementASTNode* compound, size_t statement_index, ASTNode* expr) {
  return SplitSuspendingCoAwaitExpression(compound, statement_index, expr,
                                          false);
}

/* Same as SplitSuspendingCoAwaitExpression but for co_yield. */
static bool SplitSuspendingCoYieldExpression(CompoundStatementASTNode* compound,
                                             size_t statement_index,
                                             ASTNode* expr,
                                             bool allow_root_co_yield) {
  if (compound == NULL || expr == NULL) {
    return false;
  }
  SuspendedCoroutineExpressionSearch search = {expr, NULL, 0, false};
  if (expr->op == AST_OP(co_yield)) {
    if (!allow_root_co_yield) {
      return false;
    }
    search.found = expr;
    search.count = 1;
    search.void_result = TypeIsVoid(expr->type);
  } else {
    ASTNodeVisit(expr, FindNestedSuspendingCoYield, 0, &search);
  }
  TypeRecord* result_type = CoYieldResultType(search.found);
  if (search.found == NULL || search.void_result || result_type == NULL ||
      TypeIsVoid(result_type) ||
      SuspensionHasUnsafeSplitAncestor(expr, search.found)) {
    return false;
  }

  SourceLocation location = search.found->location;
  TypeRecord* temp_type = TypeRecordCopy(result_type);
  temp_type = TypeRecordCalculateSize(temp_type);
  Symbol* result = SyntaxNewTemporary(&compiler->syntax, temp_type);
  result->flags.is_local = true;
  result->flags.is_defined = true;
  result->location = location;

  ASTNode* parent = search.found->parent;
  int child_id = search.found->child_id;
  ASTNode* co_yield = ASTNodeMove(search.found);
  ASTNodeReplaceChild(parent, child_id, NewIdentifierASTNode(result, location),
                      false);
  CoroutineCompoundInsertStatement(
      compound,
      NewSuspensionResultTemporaryDeclaration(result, co_yield, location),
      statement_index);
  ResetCompoundStatementParents(compound);
  return true;
}

/* Split only a co_yield that is nested inside `expr` (not `expr` itself). */
static bool SplitNestedSuspendingCoYieldExpression(
    CompoundStatementASTNode* compound, size_t statement_index, ASTNode* expr) {
  return SplitSuspendingCoYieldExpression(compound, statement_index, expr,
                                          false);
}

/* Split a single suspending co_await or co_yield out of `expr`. */
static bool SplitSuspendingCoroutineExpression(CompoundStatementASTNode* compound,
                                               size_t statement_index,
                                               ASTNode* expr,
                                               bool allow_root) {
  return SplitSuspendingCoAwaitExpression(compound, statement_index, expr,
                                          allow_root) ||
         SplitSuspendingCoYieldExpression(compound, statement_index, expr,
                                          allow_root);
}

/* True if `expr` contains exactly one suspending co_await/co_yield whose result
 * is non-void (the case that can be cleanly factored into a single temporary). */
static bool ExpressionHasSingleNonVoidSuspendingCoroutineExpression(
    ASTNode* expr, bool allow_root) {
  if (expr == NULL) {
    return false;
  }
  SuspendedCoroutineExpressionSearch search = {expr, NULL, 0, false};
  if (expr->op == AST_OP(co_await)) {
    if (!allow_root || CoAwaitIsAlwaysReady(expr)) {
      return false;
    }
    search.found = expr;
    search.count = 1;
    search.void_result = TypeIsVoid(expr->type);
  } else {
    ASTNodeVisit(expr, FindNestedSuspendingCoAwait, 0, &search);
  }
  TypeRecord* result_type = CoAwaitResultType(search.found);
  if (search.count == 1 && search.found != NULL && !search.void_result &&
      result_type != NULL && !TypeIsVoid(result_type)) {
    return true;
  }

  search = (SuspendedCoroutineExpressionSearch){expr, NULL, 0, false};
  if (expr->op == AST_OP(co_yield)) {
    if (!allow_root) {
      return false;
    }
    search.found = expr;
    search.count = 1;
    search.void_result = TypeIsVoid(expr->type);
  } else {
    ASTNodeVisit(expr, FindNestedSuspendingCoYield, 0, &search);
  }
  result_type = CoYieldResultType(search.found);
  return search.count == 1 && search.found != NULL && !search.void_result &&
         result_type != NULL && !TypeIsVoid(result_type);
}

/* Split a nested suspension out of a single-variable declaration's initializer. */
static bool SplitNestedSuspensionInDeclaration(
    CompoundStatementASTNode* compound, size_t statement_index,
    DeclarationListASTNode* decl_list) {
  if (decl_list == NULL || decl_list->declarations == NULL ||
      decl_list->declarations->length != 1) {
    return false;
  }
  VariableDeclarationASTNode* decl = decl_list->declarations->value.p[0];
  ASTNode* expr = decl != NULL ? VariableInitializerExpression(decl->initializer)
                               : NULL;
  return SplitNestedSuspendingCoAwaitExpression(compound, statement_index,
                                               expr) ||
         SplitNestedSuspendingCoYieldExpression(compound, statement_index,
                                                expr);
}

/* Split a nested suspension out of a statement's relevant subexpression
 * (declaration initializer, assignment RHS, co_return value, switch selector). */
static bool SplitNestedSuspensionInStatement(
    CompoundStatementASTNode* compound, size_t statement_index, ASTNode* stmt) {
  if (stmt == NULL) {
    return false;
  }
  if (stmt->op == AST_OP(decl_list)) {
    return SplitNestedSuspensionInDeclaration(
        compound, statement_index, (DeclarationListASTNode*)stmt);
  }
  if (stmt->op == AST_OP(expr)) {
    ASTNode* expr = ((ExpressionStatementASTNode*)stmt)->expr;
    if (expr != NULL && expr->op == AST_OP(assign)) {
      ASTNode* rhs = ((BinaryASTNode*)expr)->right;
      if (rhs != NULL) {
        return SplitSuspendingCoAwaitExpression(compound, statement_index, rhs,
                                                true) ||
               SplitNestedSuspendingCoYieldExpression(compound, statement_index,
                                                      rhs);
      }
      return false;
    }
    return SplitNestedSuspendingCoAwaitExpression(compound, statement_index,
                                                 expr) ||
           SplitNestedSuspendingCoYieldExpression(compound, statement_index,
                                                  expr);
  }
  if (stmt->op == AST_OP(co_return)) {
    ASTNode* expr = ((CombinedStatementASTNode*)stmt)->cond;
    return SplitNestedSuspendingCoAwaitExpression(compound, statement_index,
                                                 expr) ||
           SplitNestedSuspendingCoYieldExpression(compound, statement_index,
                                                  expr);
  }
  if (stmt->op == AST_OP(switch)) {
    return SplitSuspendingCoroutineExpression(
        compound, statement_index, ((SwitchStatementASTNode*)stmt)->expr, true);
  }
  return false;
}

/* Apply short-circuit-suspension lowering to a statement's relevant
 * subexpression (declaration, assignment RHS, co_return, if/switch condition). */
static bool LowerShortCircuitSuspensionInStatement(
    CompoundStatementASTNode* compound, size_t statement_index, ASTNode* stmt) {
  if (stmt == NULL) {
    return false;
  }
  if (stmt->op == AST_OP(decl_list)) {
    return LowerShortCircuitSuspensionExpression(compound, statement_index,
                                                 stmt);
  }
  if (stmt->op == AST_OP(expr)) {
    ASTNode* expr = ((ExpressionStatementASTNode*)stmt)->expr;
    if (expr != NULL && expr->op == AST_OP(assign)) {
      return LowerShortCircuitSuspensionExpression(
          compound, statement_index, ((BinaryASTNode*)expr)->right);
    }
    return LowerShortCircuitSuspensionExpression(compound, statement_index,
                                                expr);
  }
  if (stmt->op == AST_OP(co_return)) {
    return LowerShortCircuitSuspensionExpression(compound, statement_index,
                                                stmt);
  }
  if (stmt->op == AST_OP(if)) {
    return LowerShortCircuitSuspensionExpression(
        compound, statement_index, ((IfStatementASTNode*)stmt)->cond);
  }
  if (stmt->op == AST_OP(switch)) {
    return LowerShortCircuitSuspensionExpression(
        compound, statement_index, ((SwitchStatementASTNode*)stmt)->expr);
  }
  return false;
}

/* Build the constant `1` used to replace a loop condition after the real
 * condition has been moved into the loop body as a guard. */
static ASTNode* NewCoroutineTrueCondition(SourceLocation location) {
  return NewIntConstantASTNode(1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

/* Build `if (!cond) break;`, the guard used inside a loop body after a
 * suspending loop condition is hoisted into the body. */
static ASTNode* NewCoroutineBreakIfFalse(ASTNode* cond,
                                         SourceLocation location) {
  ASTNode* not_cond =
      NewUnaryASTNode(AST_OP(not),
                      NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      location, cond);
  return NewIfStatementASTNode(
      not_cond, NewASTNode(AST_OP(break), NULL, location), NULL, false,
      location);
}

/* Ensure a loop body is a compound statement (wrapping a single statement in
 * braces if needed) so guard/suspension statements can be inserted into it. */
static CompoundStatementASTNode* EnsureCoroutineCompoundLoopBody(
    ASTNode* loop, int stmt_child_id, ASTNode* stmt, SourceLocation location) {
  if (stmt != NULL && stmt->op == AST_OP(compound)) {
    return (CompoundStatementASTNode*)stmt;
  }
  Vector* statements = NewVector();
  if (stmt != NULL) {
    VectorAppend(statements, ASTNodeMove(stmt));
  }
  ASTNode* compound = NewCompoundStatementASTNode(statements, location);
  ASTNodeReplaceChild(loop, stmt_child_id, compound, false);
  return (CompoundStatementASTNode*)compound;
}

/* Context for redirecting `continue` in a do/while body to the synthesized label
 * placed before the (hoisted) loop condition. */
typedef struct {
  String* condition_label;
  LabelASTNode* label;
} DoWhileConditionRewrite;

/* Transform callback: rewrite `continue` in a do/while body into a goto to the
 * condition label (so continue still evaluates the condition), without
 * descending into nested loops which own their own continue target. */
static ASTNode* RewriteDoWhileConditionContinue(ASTNode* node, void* data,
                                                ASTNodeTransformAction* action) {
  DoWhileConditionRewrite* rewrite = data;
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(while) || node->op == AST_OP(do) ||
      node->op == AST_OP(for) || node->op == AST_OP(expansion_for)) {
    *action = kASTTransformSkipChildren;
    return node;
  }
  if (node->op != AST_OP(continue)) {
    return node;
  }
  ASTNode* goto_stmt =
      NewGotoStatementASTNode(NewString(rewrite->condition_label->value),
                              node->location);
  goto_stmt->flags |= kASTCompilerGeneratedGoto;
  ((GotoStatementASTNode*)goto_stmt)->label = (ASTNode*)rewrite->label;
  return goto_stmt;
}

/* Rewrite `while (cond-with-suspension) body` into `while (1) { <cond eval>;
 * if (!cond) break; body }` so the suspension in the condition occurs in
 * statement position where it can be split. */
static bool NormalizeWhileConditionSuspension(ASTNode* stmt) {
  CombinedStatementASTNode* loop = (CombinedStatementASTNode*)stmt;
  CompoundStatementASTNode* body =
      EnsureCoroutineCompoundLoopBody(stmt, 1, loop->stmt, stmt->location);
  SourceLocation location = loop->cond->location;
  size_t guard_index = 1;
  if (LowerShortCircuitSuspensionExpression(body, 0, loop->cond)) {
    guard_index = 2;
  } else if (ExpressionHasSingleNonVoidSuspendingCoroutineExpression(loop->cond,
                                                                     true)) {
    if (!SplitSuspendingCoroutineExpression(body, 0, loop->cond, true)) {
      return false;
    }
  } else {
    return false;
  }

  ASTNode* guard_cond = ASTNodeMove(loop->cond);
  ASTNodeReplaceChild(stmt, 0, NewCoroutineTrueCondition(location), false);
  CoroutineCompoundInsertStatement(
      body, NewCoroutineBreakIfFalse(guard_cond, location), guard_index);
  ResetCompoundStatementParents(body);
  return true;
}

/* Like NormalizeWhileConditionSuspension but for do/while: append a continue
 * label and the condition evaluation at the end of the body, redirect `continue`
 * to that label, and turn the loop condition into a `break` guard. */
static bool NormalizeDoConditionSuspension(ASTNode* stmt) {
  CombinedStatementASTNode* loop = (CombinedStatementASTNode*)stmt;
  bool has_condition_suspension =
      ASTContainsSuspendingCoroutineExpression(loop->cond);
  if (!has_condition_suspension) {
    return false;
  }
  CompoundStatementASTNode* body =
      EnsureCoroutineCompoundLoopBody(stmt, 1, loop->stmt, stmt->location);

  SourceLocation location = loop->cond->location;
  String label_name;
  StringInit(&label_name, SyntaxFakeName(&compiler->syntax));
  ASTNode* empty_label_stmt =
      NewCompoundStatementASTNode(NewVector(), location);
  LabelASTNode* condition_label =
      (LabelASTNode*)NewLabelASTNode(label_name.value, empty_label_stmt,
                                     false, location);
  DoWhileConditionRewrite rewrite = {&label_name, condition_label};
  ASTNodeVisitAndTransform((ASTNode*)body, RewriteDoWhileConditionContinue,
                           &rewrite);

  size_t condition_index = body->statements->length;
  CoroutineCompoundInsertStatement(body, (ASTNode*)condition_label,
                                   condition_index);
  size_t guard_index = condition_index + 2;
  if (LowerShortCircuitSuspensionExpression(body, condition_index + 1,
                                           loop->cond)) {
    guard_index = condition_index + 3;
  } else if (ExpressionHasSingleNonVoidSuspendingCoroutineExpression(loop->cond,
                                                                     true)) {
    if (!SplitSuspendingCoroutineExpression(body, condition_index + 1,
                                           loop->cond, true)) {
      StringDestruct(&label_name);
      return false;
    }
  } else {
    StringDestruct(&label_name);
    return false;
  }

  ASTNode* guard_cond = ASTNodeMove(loop->cond);
  CoroutineCompoundInsertStatement(
      body, NewCoroutineBreakIfFalse(guard_cond, location), guard_index);
  ASTNodeReplaceChild(stmt, 0, NewCoroutineTrueCondition(location), false);
  StringDestruct(&label_name);
  ResetCompoundStatementParents(body);
  return true;
}

/* Like NormalizeWhileConditionSuspension but for the `for` loop's middle
 * (condition) expression. */
static bool NormalizeForConditionSuspension(ASTNode* stmt) {
  ForStatementASTNode* loop = (ForStatementASTNode*)stmt;
  if (loop->c2 == NULL) {
    return false;
  }
  CompoundStatementASTNode* body =
      EnsureCoroutineCompoundLoopBody(stmt, 3, loop->stmt, stmt->location);
  SourceLocation location = loop->c2->location;
  size_t guard_index = 1;
  if (LowerShortCircuitSuspensionExpression(body, 0, loop->c2)) {
    guard_index = 2;
  } else if (ExpressionHasSingleNonVoidSuspendingCoroutineExpression(loop->c2,
                                                                     true)) {
    if (!SplitSuspendingCoroutineExpression(body, 0, loop->c2, true)) {
      return false;
    }
  } else {
    return false;
  }

  ASTNode* guard_cond = ASTNodeMove(loop->c2);
  CoroutineCompoundInsertStatement(
      body, NewCoroutineBreakIfFalse(guard_cond, location), guard_index);
  ResetCompoundStatementParents(body);
  return true;
}

/* Normalize suspensions inside a sub-statement, wrapping it in a compound first
 * so newly split statements have somewhere to live. */
static void NormalizeNestedSuspensionsInStatementChild(
    ASTNode* parent, int child_id, ASTNode* stmt, SourceLocation location) {
  if (parent == NULL || stmt == NULL) {
    return;
  }
  CompoundStatementASTNode* compound = NULL;
  if (stmt->op == AST_OP(compound)) {
    compound = (CompoundStatementASTNode*)stmt;
  } else {
    Vector* statements = NewVector();
    VectorAppend(statements, ASTNodeMove(stmt));
    ASTNode* compound_node = NewCompoundStatementASTNode(statements, location);
    ASTNodeReplaceChild(parent, child_id, compound_node, false);
    compound = (CompoundStatementASTNode*)compound_node;
  }
  NormalizeNestedSuspensionsInCompound(compound);
}

/* Collector for the immediate sub-statements of a statement (the children that
 * are themselves statements but not already inside a compound). */
typedef struct {
  ASTNode* root;
  Vector children;
} CoroutineStatementChildCollector;

/* Transform callback that gathers the nearest statement children of `root`,
 * skipping into expressions and stopping at compounds (which are normalized
 * separately). */
static ASTNode* CollectNestedStatementChildTransform(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  CoroutineStatementChildCollector* collector = data;
  if (node == NULL || node == collector->root) {
    return node;
  }
  if (!ASTNodeIsStatement(node)) {
    return node;
  }
  if (!ASTNodeChildIsStatement(node->parent, node->child_id)) {
    *action = kASTTransformSkipChildren;
    return node;
  }
  if (node->parent != NULL && node->parent->op == AST_OP(compound)) {
    return node;
  }
  VectorAppend(&collector->children, node);
  *action = kASTTransformSkipChildren;
  return node;
}

/* Recurse into each statement child of `stmt`, normalizing nested suspensions
 * there (e.g. the then/else of an if, the body of a loop). */
static void NormalizeNestedSuspensionsInStatementChildren(ASTNode* stmt) {
  CoroutineStatementChildCollector collector = {
      .root = stmt,
  };
  VectorInit(&collector.children);
  ASTNodeVisitAndTransform(stmt, CollectNestedStatementChildTransform,
                           &collector);
  for (size_t i = 0; i < collector.children.length; i++) {
    ASTNode* child = collector.children.value.p[i];
    if (child != NULL && child->parent != NULL) {
      NormalizeNestedSuspensionsInStatementChild(
          child->parent, child->child_id, child, child->location);
    }
  }
  VectorDestruct(&collector.children);
}

/* Core normalization pass over a compound: for each statement, hoist embedded
 * suspensions out of subexpressions and conditions into standalone statements,
 * rewrite loops whose conditions suspend, and recurse into nested blocks. This
 * leaves every suspension at statement granularity so the state machine can be
 * built around them. */
static void NormalizeNestedSuspensionsInCompound(
    CompoundStatementASTNode* compound) {
  if (compound == NULL || compound->statements == NULL) {
    return;
  }
  ResetCompoundStatementParents(compound);
  for (size_t i = 0; i < compound->statements->length; i++) {
    ASTNode* stmt = compound->statements->value.p[i];
    if (LowerShortCircuitSuspensionInStatement(compound, i, stmt)) {
      continue;
    }
    if (SplitNestedSuspensionInStatement(compound, i, stmt)) {
      continue;
    }
    if (stmt == NULL) {
      continue;
    }
    if (stmt->op == AST_OP(compound)) {
      NormalizeNestedSuspensionsInCompound(
          (CompoundStatementASTNode*)stmt);
      continue;
    }
    if (stmt->op == AST_OP(if)) {
      IfStatementASTNode* if_stmt = (IfStatementASTNode*)stmt;
      if (SplitSuspendingCoroutineExpression(compound, i, if_stmt->cond,
                                             true)) {
        i++;
        stmt = compound->statements->value.p[i];
      }
    }
    if (stmt->op == AST_OP(while)) {
      NormalizeWhileConditionSuspension(stmt);
    } else if (stmt->op == AST_OP(do)) {
      NormalizeDoConditionSuspension(stmt);
    } else if (stmt->op == AST_OP(for)) {
      NormalizeForConditionSuspension(stmt);
    }
    NormalizeNestedSuspensionsInStatementChildren(stmt);
  }
}

/* Replace the initializing expression of a variable declaration in place,
 * navigating the various initializer wrapper shapes (init/expr_init/braced/
 * designated) to substitute `expr`. */
static void ReplaceVariableInitializerExpression(VariableDeclarationASTNode* decl,
                                                 ASTNode* expr) {
  ASTNode* initializer = decl->initializer;
  if (initializer != NULL && initializer->op == AST_OP(init)) {
    BinaryASTNode* init = (BinaryASTNode*)initializer;
    if (init->right != NULL && init->right->op == AST_OP(expr_init)) {
      ASTNodeReplaceChild(init->right, 0, expr, false);
    } else if (init->right != NULL &&
               init->right->op == AST_OP(braced_init)) {
      BracedInitializerASTNode* braced =
          (BracedInitializerASTNode*)init->right;
      if (braced->initializers != NULL &&
          braced->initializers->length == 1 &&
          ((ASTNode*)braced->initializers->value.p[0])->op ==
              AST_OP(designated_init)) {
        ASTNode* designated = braced->initializers->value.p[0];
        ASTNodeReplaceChild(designated, 0, expr, false);
      } else {
        ASTNodeReplaceChild(initializer, 1,
                            NewExpressionInitializerASTNode(
                                expr, initializer->location),
                            false);
      }
    } else {
      ASTNodeReplaceChild(initializer, 1,
                          NewExpressionInitializerASTNode(
                              expr, initializer->location),
                          false);
    }
    return;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    ASTNodeReplaceChild(initializer, 0, expr, false);
    return;
  }
  ASTNodeReplaceChild((ASTNode*)decl, 0,
                      NewExpressionInitializerASTNode(expr,
                                                      decl->base.location),
                      false);
}

/* If a co_await's operand is a plain identifier, return its symbol (used to reuse
 * an existing local as the awaiter rather than creating a temporary). */
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

/* Determine the awaiter type of a co_await's operand, digging through comma,
 * inline-call, call (member result or function return), and compound-literal
 * forms when the node itself has no type yet. */
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

/* Create a local temporary symbol to hold an awaiter object of `type`. */
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

/* Find the declaration of `awaiter` among the statements preceding the suspension
 * point in its compound (so the awaiter's init can be redirected into the frame). */
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

/* Lower a co_yield at a suspension point: create the awaiter from
 * `promise.yield_value(operand)`, register it as a frame-owned object, and
 * initialize its frame slot in place of (or before) the original statement. The
 * subsequent suspend machinery is emitted later like a co_await. */
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
                               false, true);

  ASTNode* operand = ASTNodeMove(((UnaryASTNode*)point->co_yield)->sub);
  ASTNode* yielded_value_init = NULL;
  if (point->yielded_value_member != NULL) {
    CoroutineFrameAddOwnedSymbol(
        frame, point->yielded_value_member->symbol,
        point->yielded_value_member,
        point->yielded_value_constructed_member, false, true);
    yielded_value_init = NewFrameMemberInitialization(
        frame, point->yielded_value_member,
        point->yielded_value_constructed_member, operand, location);
    operand = NewCoroutineMoveValue(
        NewFrameMemberAccess(frame, point->yielded_value_member, location),
        location);
  }
  Vector* actuals = NewVector();
  VectorAppend(actuals, operand);
  ASTNode* yield_call = NewCoroutinePromiseMemberCall(
      promise, "yield_value", actuals, location);
  ASTNodeSetType(yield_call, TypeRecordCopy(awaiter->type));
  ASTNode* init = NewFrameMemberInitialization(
      frame, point->frame_member, point->frame_constructed_member,
      yield_call, location);
  if (yielded_value_init != NULL) {
    Vector* init_statements = NewVector();
    VectorAppend(init_statements, yielded_value_init);
    VectorAppend(init_statements, init);
    init = NewCompoundStatementASTNode(init_statements, location);
  }
  bool statement_yield =
      point->statement != NULL && point->statement->op == AST_OP(expr) &&
      ((ExpressionStatementASTNode*)point->statement)->expr == point->co_yield;
  if (statement_yield) {
    VectorSet(point->compound->statements, point->statement_index, init);
    init->parent = (ASTNode*)point->compound;
    init->child_id = (int)point->statement_index;
    return true;
  }

  Vector* statements = NewVector();
  VectorAppend(statements, init);
  VectorAppend(statements, ASTNodeMove(point->statement));
  ASTNode* compound = NewCompoundStatementASTNode(statements, location);
  VectorSet(point->compound->statements, point->statement_index, compound);
  compound->parent = (ASTNode*)point->compound;
  compound->child_id = (int)point->statement_index;
  point->compound = (CompoundStatementASTNode*)compound;
  point->statement_index = 0;
  return true;
}

/* Add the synthetic multi-valued state member to the frame struct. */
static StructMember* AddCoroutineFrameIntMember(Struct* str,
                                                const char* name) {
  Symbol* symbol = NewSymbol(name,
                             NewTypeRecordWithSize(kTypeInt, kQualPlain),
                             STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

/* Add a synthetic done/lifetime flag to the frame struct. */
static StructMember* AddCoroutineFrameBoolMember(Struct* str,
                                                 const char* name) {
  Symbol* symbol = NewSymbol(name,
                             NewTypeRecordWithSize(kTypeBool, kQualPlain),
                             STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

/* Add a synthetic member of a given type to the frame struct (promise, awaiter,
 * persisted-local, or function-pointer slots). */
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

/* Register a symbol whose object lives in the frame (so it can be constructed and
 * later destroyed in the right order). */
static void CoroutineFrameAddOwnedSymbol(CoroutineFrame* frame,
                                         Symbol* symbol,
                                         StructMember* member,
                                         StructMember* constructed_member,
                                         bool construct_at_start,
                                         bool body_lifetime) {
  if (frame == NULL || symbol == NULL || member == NULL) {
    return;
  }
  FrameOwnedSymbol* owned = malloc(sizeof(FrameOwnedSymbol));
  assert(owned != NULL);
  owned->symbol = symbol;
  owned->member = member;
  owned->constructed_member = constructed_member;
  owned->construct_at_start = construct_at_start;
  owned->body_lifetime = body_lifetime;
  VectorAppend(&frame->owned_symbols, owned);
}

/* Return the frame slot associated with a frame-owned symbol, or NULL. */
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

/* Free the bookkeeping list of frame-owned symbols. */
static void CoroutineFrameOwnedSymbolsDestruct(CoroutineFrame* frame) {
  if (frame == NULL) {
    return;
  }
  for (size_t i = 0; i < frame->owned_symbols.length; i++) {
    free(frame->owned_symbols.value.p[i]);
  }
  VectorDestruct(&frame->owned_symbols);
}

/* At the start of the resume function, initialize the frame's state to 0, clear
 * every owned object's "constructed" flag, and eagerly construct the objects
 * marked construct-at-start (e.g. the promise). */
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
  /* Parameter copies are ramp-only frame stores. They are initialized before
   * the promise object, as required by coroutine-state construction order. */
  while (index < body->statements->length) {
    ASTNode* stmt = body->statements->value.p[index];
    if (stmt == NULL || (stmt->flags & kASTCoroutineFrameStore) == 0) {
      break;
    }
    index++;
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

/* Build the destructor call for a frame-owned object, guarded by its
 * "constructed" flag when present: `if (constructed) { obj.~T(); constructed=0; }`
 * so partially-constructed frames tear down correctly. */
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
                           0,
                           TypeRecordCopy(
                               owned->constructed_member->symbol->type),
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

/* Guarded destructor for a frame member that isn't tracked in owned_symbols
 * (builds a throwaway FrameOwnedSymbol descriptor). */
static ASTNode* NewCoroutineFrameMemberGuardedDestructor(
    CoroutineFrame* frame, StructMember* member,
    StructMember* constructed_member, SourceLocation location) {
  FrameOwnedSymbol owned = {
      .symbol = member != NULL ? member->symbol : NULL,
      .member = member,
      .constructed_member = constructed_member,
      .construct_at_start = false,
      .body_lifetime = true,
  };
  return NewCoroutineFrameGuardedDestructor(frame, &owned, location);
}

/* Append guarded destructor calls for all frame-owned objects in reverse
 * construction order (used by the destroy function for full teardown). */
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

/* True if a frame-owned object has body lifetime. Promise/initial/final awaiters
 * and function-parameter copies outlive the body until the frame is destroyed. */
static bool CoroutineFrameOwnedSymbolIsBodyLifetime(CoroutineFrame* frame,
                                                    FrameOwnedSymbol* owned) {
  if (frame == NULL || owned == NULL) {
    return false;
  }
  return owned->body_lifetime;
}

/* Append guarded destructors for only the body-lifetime frame objects, in
 * reverse construction order (used when the body completes or throws). */
static void AppendCoroutineFrameBodyDestructors(Vector* statements,
                                                CoroutineFrame* frame,
                                                SourceLocation location) {
  if (statements == NULL || frame == NULL) {
    return;
  }
  for (size_t i = frame->owned_symbols.length; i > 0; i--) {
    FrameOwnedSymbol* owned = frame->owned_symbols.value.p[i - 1];
    if (owned == NULL || owned->member == NULL ||
        owned->member->symbol == NULL ||
        !CoroutineFrameOwnedSymbolIsBodyLifetime(frame, owned)) {
      continue;
    }
    ASTNode* dtor = NewCoroutineFrameGuardedDestructor(frame, owned,
                                                      location);
    if (dtor != NULL) {
      VectorAppend(statements, dtor);
    }
  }
}

/* Append guarded destruction for one persisted local, if any. */
static void AppendPersistedCoroutineLocalDestructor(
    Vector* statements, CoroutineFrame* frame, Vector* persisted_locals,
    Symbol* symbol, SourceLocation location) {
  CoroutinePersistedLocal* local =
      FindPersistedCoroutineLocal(persisted_locals, symbol);
  if (local == NULL || local->is_parameter || local->member == NULL) {
    return;
  }
  ASTNode* dtor = NewCoroutineFrameMemberGuardedDestructor(
      frame, local->member, local->constructed_member, location);
  if (dtor != NULL) {
    VectorAppend(statements, dtor);
  }
}

static void AppendPersistedCoroutineRangeForTemporaryDestructors(
    Vector* statements, CoroutineFrame* frame, Vector* persisted_locals,
    CompoundStatementASTNode* compound, SourceLocation location) {
  for (size_t i = persisted_locals->length; i > 0; i--) {
    CoroutinePersistedLocal* local = persisted_locals->value.p[i - 1];
    if (local == NULL || local->symbol == NULL ||
        !local->symbol->flags.is_temp ||
        local->store_compound != compound || local->member == NULL) {
      continue;
    }
    ASTNode* destructor = NewCoroutineFrameMemberGuardedDestructor(
        frame, local->member, local->constructed_member, location);
    if (destructor != NULL) {
      VectorAppend(statements, destructor);
    }
  }
}

/* Destroy frame-backed locals when the coroutine body exits normally.  Walk
 * from the co_return out through its enclosing scopes so active locals are
 * destroyed in exact reverse declaration/nesting order.  Frame copies of
 * function parameters intentionally outlive the body until the coroutine state
 * itself is destroyed. */
static void AppendPersistedCoroutineLocalDestructors(
    Vector* statements, CoroutineFrame* frame, Vector* persisted_locals,
    ASTNode* return_statement, SourceLocation location) {
  if (statements == NULL || frame == NULL || persisted_locals == NULL ||
      return_statement == NULL) {
    return;
  }

  ASTNode* current = return_statement;
  while (current != NULL && current->parent != NULL) {
    ASTNode* parent = current->parent;
    if (parent->op == AST_OP(compound)) {
      CompoundStatementASTNode* compound =
          (CompoundStatementASTNode*)parent;
      size_t limit = current->child_id >= 0 ? (size_t)current->child_id : 0;
      if (limit > compound->statements->length) {
        limit = compound->statements->length;
      }
      for (size_t i = limit; i > 0; i--) {
        ASTNode* stmt = compound->statements->value.p[i - 1];
        if (stmt == NULL || stmt->op != AST_OP(decl_list)) {
          continue;
        }
        DeclarationListASTNode* declarations =
            (DeclarationListASTNode*)stmt;
        for (size_t j = declarations->declarations->length; j > 0; j--) {
          ASTNode* declaration = declarations->declarations->value.p[j - 1];
          if (declaration == NULL || declaration->op != AST_OP(vardecl)) {
            continue;
          }
          AppendPersistedCoroutineLocalDestructor(
              statements, frame, persisted_locals,
              ((VariableDeclarationASTNode*)declaration)->symbol, location);
        }
      }
      if ((compound->base.flags & kASTRangeForInitializer) != 0 &&
          limit > 0) {
        AppendPersistedCoroutineRangeForTemporaryDestructors(
            statements, frame, persisted_locals, compound, location);
      }
    } else if (parent->op == AST_OP(catch)) {
      AppendPersistedCoroutineLocalDestructor(
          statements, frame, persisted_locals,
          ((CatchASTNode*)parent)->symbol, location);
    }
    current = parent;
  }
}

static void AppendPersistedCoroutineCompoundRangeDestructors(
    Vector* statements, CoroutineFrame* frame, Vector* persisted_locals,
    CompoundStatementASTNode* compound, size_t first, size_t limit,
    SourceLocation location) {
  if (compound == NULL || compound->statements == NULL) {
    return;
  }
  if (limit > compound->statements->length) {
    limit = compound->statements->length;
  }
  for (size_t i = limit; i > first; i--) {
    ASTNode* stmt = compound->statements->value.p[i - 1];
    if (stmt == NULL || stmt->op != AST_OP(decl_list)) {
      continue;
    }
    DeclarationListASTNode* declarations = (DeclarationListASTNode*)stmt;
    for (size_t j = declarations->declarations->length; j > 0; j--) {
      ASTNode* declaration = declarations->declarations->value.p[j - 1];
      if (declaration == NULL || declaration->op != AST_OP(vardecl)) {
        continue;
      }
      AppendPersistedCoroutineLocalDestructor(
          statements, frame, persisted_locals,
          ((VariableDeclarationASTNode*)declaration)->symbol, location);
    }
  }
}

static ASTNode* EnclosingCoroutineLoopOrSwitch(ASTNode* node,
                                               bool loop_only) {
  for (ASTNode* parent = node != NULL ? node->parent : NULL;
       parent != NULL; parent = parent->parent) {
    if (parent->op == AST_OP(for) || parent->op == AST_OP(expansion_for) ||
        parent->op == AST_OP(while) ||
        parent->op == AST_OP(do) ||
        (!loop_only && parent->op == AST_OP(switch))) {
      return parent;
    }
  }
  return NULL;
}

static void CollectCoroutineBreakContinue(ASTNode* node, void* data,
                                          int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL &&
      (node->op == AST_OP(break) || node->op == AST_OP(continue))) {
    VectorAppend((Vector*)data, node);
  }
}

/* Coroutines bypass the ordinary scope-exit insertion pass because persisted
 * objects must be destroyed through frame slots. Add guarded frame destructors
 * before break/continue for every lexical compound exited by the jump. */
static void InsertPersistedCoroutineJumpDestructors(
    ASTNode* body, CoroutineFrame* frame, Vector* persisted_locals) {
  if (body == NULL || frame == NULL || persisted_locals == NULL ||
      persisted_locals->length == 0) {
    return;
  }
  Vector jumps;
  VectorInit(&jumps);
  ASTNodeVisit(body, CollectCoroutineBreakContinue, 0, &jumps);
  for (size_t i = 0; i < jumps.length; i++) {
    ASTNode* jump = jumps.value.p[i];
    if (jump == NULL || (jump->flags & kASTScopeExitCleanup) != 0) {
      continue;
    }
    jump->flags |= kASTScopeExitCleanup;
    ASTNode* limit = EnclosingCoroutineLoopOrSwitch(
        jump, jump->op == AST_OP(continue));
    if (limit == NULL) {
      continue;
    }
    Vector* destructors = NewVector();
    ASTNode* child = jump;
    ASTNode* parent = jump->parent;
    while (parent != NULL) {
      if (parent->op == AST_OP(compound)) {
        size_t child_index =
            child->child_id >= 0 ? (size_t)child->child_id : 0;
        AppendPersistedCoroutineCompoundRangeDestructors(
            destructors, frame, persisted_locals,
            (CompoundStatementASTNode*)parent, 0, child_index, jump->location);
      }
      if (parent == limit) {
        break;
      }
      child = parent;
      parent = parent->parent;
    }
    if (destructors->length == 0) {
      VectorDelete(destructors);
      continue;
    }
    ASTNode* old_parent = jump->parent;
    int old_child_id = jump->child_id;
    VectorAppend(destructors, jump);
    ASTNode* compound =
        NewCompoundStatementASTNode(destructors, jump->location);
    ASTNodeReplaceChild(old_parent, old_child_id, compound, false);
  }
  VectorDestruct(&jumps);
}

/* Build the `FrameType*` pointer type. */
static TypeRecord* NewCoroutineFramePointerType(TypeRecord* frame_type) {
  return NewPointerTo(kQualPlain, TypeRecordCopy(frame_type));
}

/* Build the implicit `__frame` pointer parameter shared by the resume and
 * destroy functions. */
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

/* Build the function-pointer type for the resume slot: `void (*)(FrameType*)`. */
static TypeRecord* NewCoroutineResumePointerType(TypeRecord* frame_type,
                                                 SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  VectorAppend(&func->info.function.prototype,
               NewCoroutineFrameParameter(frame_type, location));
  return NewPointerTo(kQualPlain, func);
}

/* Build the resume function's type `void(FrameType*)` (a definition, not a
 * pointer), returning its frame parameter so the body can reference the frame. */
static TypeRecord* NewCoroutineResumeFunctionType(TypeRecord* frame_type,
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
  func->info.function.coroutine_frame_type = TypeRecordCopy(frame_type);
  return func;
}

/* Build the function-pointer type for the destroy slot: `void (*)(FrameType*)`. */
static TypeRecord* NewCoroutineDestroyPointerType(TypeRecord* frame_type,
                                                  SourceLocation location) {
  TypeRecord* func = NewFunctionTypeRecord();
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  VectorAppend(&func->info.function.prototype,
               NewCoroutineFrameParameter(frame_type, location));
  return NewPointerTo(kQualPlain, func);
}

/* Build the destroy function's type `void(FrameType*)` (a definition), returning
 * its frame parameter. */
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
  func->info.function.coroutine_frame_type = TypeRecordCopy(frame_type);
  return func;
}

/* Add the frame's `__resume` function-pointer slot. */
static StructMember* AddCoroutineFrameResumeMember(Struct* str,
                                                   TypeRecord* frame_type,
                                                   SourceLocation location) {
  Symbol* symbol = NewSymbol("__resume",
                             NewCoroutineResumePointerType(frame_type,
                                                           location),
                             STO(auto));
  StructMember* member = NewStructMember(symbol);
  StructAddSyntheticMember(str, member);
  return member;
}

/* Add the frame's `__destroy` function-pointer slot. */
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

/* Find an overload in a chain with the given parameter count (used to pick the
 * right operator new/delete). */
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

/* Find, or synthesize and globally declare, a single-argument global allocation
 * function `name` (the global ::operator new / ::operator delete used to obtain
 * and release the coroutine frame). */
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

  Symbol* symbol = NewSymbol(name, func, STO(static));
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

/* Look up a promise class's own `operator new`/`operator delete` member (the
 * class-scoped allocation override), if one with `arg_count` parameters exists. */
static Symbol* GetCoroutineClassAllocationFunction(TypeRecord* promise_type,
                                                   const char* name,
                                                   size_t arg_count) {
  if (!TypeIsStructOrUnion(promise_type) ||
      promise_type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(promise_type->info.struct_info, name);
  if (member == NULL || !member->is_member_function ||
      member->symbol == NULL || !TypeIsFunction(member->symbol->type)) {
    return NULL;
  }
  return FindCoroutineAllocationFunctionByArgCount(member->symbol, arg_count);
}

/* Resolve the operator new used for frame allocation: the promise's class
 * override if present, else the global ::operator new(size_t). */
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

/* Resolve the operator delete used for frame deallocation: the promise's class
 * override if present, else the global ::operator delete(void*). */
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

/* Build `(FrameType*)operator new(sizeof(FrameType))`: the expression that
 * allocates and types the coroutine frame in the ramp function. */
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

/* Build `operator delete((void*)frame);` to release the coroutine frame. */
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

/* Build a `symbol = initializer` declaration-initializer expression. */
static ASTNode* NewVariableInitExpression(Symbol* symbol, ASTNode* initializer,
                                          SourceLocation location) {
  ASTNode* decl_id = NewIdentifierASTNode(symbol, location);
  decl_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  return NewBinaryASTNode(AST_OP(init), symbol->type, location, decl_id,
                          NewExpressionInitializerASTNode(initializer,
                                                          location));
}

/* Construct the coroutine frame: synthesize the frame struct with slots for
 * state/done, the resume/destroy function pointers, the promise, the
 * initial/final awaiters, each persisted local, and each suspension point's
 * awaiter (plus "constructed" flags for objects with C++ lifetime). Registers
 * the struct as a tag, allocates the frame-pointer temporary, and builds its
 * `frame = (FrameType*)operator new(...)` declaration. Returns the populated
 * CoroutineFrame. */
static CoroutineFrame NewCoroutineFrame(TypeRecord* promise_type,
                                        TypeRecord* initial_awaiter_type,
                                        TypeRecord* final_awaiter_type,
                                        SuspensionPoints* points,
                                        Vector* persisted_locals,
                                        TypeRecord* yield_awaiter_type,
                                        SourceLocation location) {
  TypeRecord* frame_type = NewTypeRecord(kTypeStruct, kQualPlain);
  Struct* str = NewStruct(false);
  TypeRecordSetStructInfo(frame_type, str);

  CoroutineFrame frame = {0};
  VectorInit(&frame.owned_symbols);
  frame.state = AddCoroutineFrameIntMember(str, "__state");
  frame.done = AddCoroutineFrameBoolMember(str, "__done");
  frame.resume = AddCoroutineFrameResumeMember(str, frame_type, location);
  frame.destroy = AddCoroutineFrameDestroyMember(str, frame_type, location);
  frame.promise = AddCoroutineFrameTypedMember(str, "__promise",
                                               promise_type);
  frame.promise_constructed =
      AddCoroutineFrameBoolMember(str, "__promise_constructed");
  if (initial_awaiter_type != NULL) {
    frame.initial_awaiter =
        AddCoroutineFrameTypedMember(str, "__initial_awaiter",
                                     initial_awaiter_type);
    frame.initial_awaiter_constructed =
        AddCoroutineFrameBoolMember(str, "__initial_awaiter_constructed");
  }
  if (final_awaiter_type != NULL &&
      !AwaiterTypeIsAlwaysReady(final_awaiter_type)) {
    frame.final_awaiter =
        AddCoroutineFrameTypedMember(str, "__final_awaiter",
                                     final_awaiter_type);
    frame.final_awaiter_constructed =
        AddCoroutineFrameBoolMember(str, "__final_awaiter_constructed");
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
    TypeRecord* member_type =
        TypeIsReference(local->symbol->type)
            ? NewPointerTo(kQualPlain,
                           TypeRecordCopy(local->symbol->type->next))
            : TypeRecordCopy(local->symbol->type);
    local->member =
        AddCoroutineFrameTypedMember(str, member_name, member_type);
    TypeRecordDelete(member_type);
    if (TypeIsStructOrUnion(local->symbol->type)) {
      snprintf(member_name, sizeof(member_name), "__local%zu_constructed", i);
      local->constructed_member = AddCoroutineFrameBoolMember(str, member_name);
    }
  }
  for (int i = 0; points != NULL && i < points->count; i++) {
    SuspensionPoint* point = &points->points[i];
    if (point->kind == kSuspensionCoYield && point->co_yield != NULL) {
      ASTNode* operand = ((UnaryASTNode*)point->co_yield)->sub;
      if (operand != NULL && operand->type != NULL &&
          operand->value_category == kValueCategoryPrvalue) {
        char value_member_name[32];
        snprintf(value_member_name, sizeof(value_member_name),
                 "__yielded_value%d", i);
        point->yielded_value_member = AddCoroutineFrameTypedMember(
            str, value_member_name, operand->type);
        if (TypeIsStructOrUnion(operand->type)) {
          snprintf(value_member_name, sizeof(value_member_name),
                   "__yielded_value%d_constructed", i);
          point->yielded_value_constructed_member =
              AddCoroutineFrameBoolMember(str, value_member_name);
        }
      }
    }
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
        AddCoroutineFrameBoolMember(str, member_name);
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

/* Enqueue a synthesized coroutine function (resume/destroy) for later codegen by
 * adding it to the compiler's pending-instantiation and declaration lists. */
static void QueueCoroutineGeneratedFunction(Symbol* symbol) {
  Vector* declarations = NewVector();
  VectorAppend(declarations,
               NewVariableDeclarationASTNode(symbol, NULL, symbol->location));
  CompilerQueuePendingTemplateInstantiation(
      NewDeclarationListASTNode(declarations, symbol->location));
  VectorAppend(&compiler->declaration_asts, symbol->type->info.function.body);
}

/* Old/new symbol pair for rewriting identifier references during lowering. */
typedef struct {
  Symbol* old_symbol;
  Symbol* new_symbol;
} SymbolReplacement;

/* Visitor that retargets identifier nodes referring to `old_symbol` to
 * `new_symbol` (e.g. remapping the original function parameter to the resume
 * function's frame parameter). */
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

/* True if `node` is exactly `frame->member` for the given frame slot. */
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

/* True if `node` is the RHS of an assignment whose LHS is `frame->member` (i.e.
 * a self-copy that should be left alone when promoting locals into the frame). */
static bool IsFrameCopyAssignmentRhs(ASTNode* node, StructMember* member) {
  if (node == NULL || node->parent == NULL || node->child_id != 1 ||
      node->parent->op != AST_OP(assign)) {
    return false;
  }
  BinaryASTNode* assign = (BinaryASTNode*)node->parent;
  return IsFrameMemberAccessFor(assign->left, member);
}

/* Scratch state for upward (parent-chain) AST searches. */
typedef struct {
  ASTNode* target;
  ASTNode* found;
} CoroutineUpwardSearch;

/* Upward visitor: stops when it reaches the target ancestor node. */
static bool FindMatchingAncestor(ASTNode* node, void* data) {
  CoroutineUpwardSearch* search = data;
  if (node == search->target) {
    search->found = node;
    return false;
  }
  return true;
}

/* True if `ancestor` is on the parent chain of `node`. */
static bool ASTNodeIsAncestorOf(ASTNode* ancestor, ASTNode* node) {
  CoroutineUpwardSearch search = {
      .target = ancestor,
  };
  ASTNodeVisitUpwards(node, FindMatchingAncestor, &search);
  return search.found != NULL;
}

/* Upward visitor: stops at the nearest enclosing call node. */
static bool FindEnclosingCall(ASTNode* node, void* data) {
  CoroutineUpwardSearch* search = data;
  if (node != NULL && node->op == AST_OP(call)) {
    search->found = node;
    return false;
  }
  return true;
}

/* True if `node` is an argument to a `frame->member.ctor(...)` construction call
 * (not part of the callee/receiver), so that identifier-to-frame rewriting can
 * skip the receiver itself. */
static bool IsFrameConstructionActual(ASTNode* node, StructMember* member) {
  CoroutineUpwardSearch search = {0};
  ASTNodeVisitUpwards(node != NULL ? node->parent : NULL, FindEnclosingCall,
                      &search);
  ASTNode* call_node = search.found;
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

/* Scratch state for finding a symbol's own declaration node above a reference. */
typedef struct {
  Symbol* symbol;
  bool found;
} CoroutineDeclarationAncestorSearch;

/* Upward visitor: detects whether an ancestor is the variable declaration of the
 * searched-for symbol. */
static bool FindDeclarationAncestorForSymbol(ASTNode* node, void* data) {
  CoroutineDeclarationAncestorSearch* search = data;
  if (node != NULL && node->op == AST_OP(vardecl) &&
      ((VariableDeclarationASTNode*)node)->symbol == search->symbol) {
    search->found = true;
    return false;
  }
  return true;
}

/* True if `node` sits inside the declaration of `symbol` (so its declaring
 * identifier isn't rewritten to a frame access). */
static bool IsInsideDeclarationOfSymbol(ASTNode* node, Symbol* symbol) {
  CoroutineDeclarationAncestorSearch search = {
      .symbol = symbol,
  };
  ASTNodeVisitUpwards(node != NULL ? node->parent : NULL,
                      FindDeclarationAncestorForSymbol, &search);
  return search.found;
}

static bool IsInsideCoroutineFrameStore(ASTNode* node) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if ((current->flags & kASTCoroutineFrameStore) != 0) {
      return true;
    }
    if (current->op == AST_OP(compound)) {
      return false;
    }
  }
  return false;
}

/* Visitor that rewrites references to frame-owned locals into `frame->slot`
 * accesses, skipping the declaration itself, self-copy assignment RHSs, and
 * in-place construction arguments to avoid clobbering construction semantics. */
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
  if (IsInsideDeclarationOfSymbol(node, identifier->symbol) ||
      IsInsideCoroutineFrameStore(node)) {
    return;
  }
  StructMember* member =
      CoroutineFrameMemberForSymbol(frame, identifier->symbol);
  if (member == NULL || IsFrameCopyAssignmentRhs(node, member) ||
      IsFrameConstructionActual(node, member)) {
    return;
  }
  ASTNode* access = NewFrameMemberAccess(frame, member, node->location);
  if (TypeIsReference(identifier->symbol->type)) {
    access = NewUnaryASTNode(
        AST_OP(contents), TypeRecordCopy(identifier->symbol->type->next),
        node->location, access);
  }
  ASTNodeReplaceChild(node->parent, node->child_id, access, true);
}

/* Rewrite all references to frame-owned locals within `node` into frame
 * accesses. */
static void RewriteFrameOwnedSymbols(ASTNode* node, CoroutineFrame* frame) {
  if (node == NULL || frame == NULL || frame->owned_symbols.length == 0) {
    return;
  }
  ASTNodeVisit(node, ReplaceFrameOwnedIdentifier, 0, frame);
}

/* Transform callback: drop statements flagged as frame-parameter stores (the
 * ramp-only copies of incoming arguments into the frame), which must not run
 * again inside the resume function. */
static ASTNode* RemoveCoroutineFrameStoreTransform(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  (void)action;
  if (node != NULL && (node->flags & kASTCoroutineFrameStore) != 0) {
    return NULL;
  }
  return node;
}

/* Remove all frame-store statements from a (cloned) body. */
static void RemoveCoroutineFrameStores(CompoundStatementASTNode* compound) {
  if (compound == NULL) {
    return;
  }
  ASTNodeVisitAndTransform((ASTNode*)compound,
                           RemoveCoroutineFrameStoreTransform, NULL);
  ResetCompoundStatementParents(compound);
}

/* Resume functions return void. Replace each synthesized ramp return with a
 * plain `return;`; the ramp itself retains the return-object expression. */
static ASTNode* ReplaceCoroutineResumeReturnTransform(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)data;
  if (node == NULL || node->op != AST_OP(compound)) {
    return node;
  }
  CompoundStatementASTNode* compound = (CompoundStatementASTNode*)node;
  if (compound->statements == NULL || compound->statements->length != 1) {
    return node;
  }
  ASTNode* return_stmt = compound->statements->value.p[0];
  if (return_stmt == NULL || return_stmt->op != AST_OP(return) ||
      (return_stmt->flags & kASTCoroutineLoweredReturn) == 0) {
    return node;
  }
  ASTNode* void_return =
      NewCombinedStatementASTNode(AST_OP(return), NULL, NULL, node->location);
  void_return->flags |= kASTCoroutineLoweredReturn;
  *action = kASTTransformSkipChildren;
  return void_return;
}

/* Build the coroutine resume function `void resume(FrameType*)`: clone the
 * normalized body, drop the frame-allocation declaration and ramp-only frame
 * stores, splice in the initial-suspend await_resume, conditionally wrap the
 * body in a try/catch that routes uncaught exceptions to unhandled_exception,
 * retarget the original frame symbol and owned locals to the frame parameter,
 * and register and queue the function for codegen. Returns the new function
 * symbol. */
static Symbol* NewCoroutineResumeFunction(ASTNode* node,
                                          CoroutineFrame* frame,
                                          CompoundStatementASTNode* body,
                                          int suspend_count,
                                          Symbol* promise,
                                          Symbol* initial_awaiter,
                                          Symbol* final_awaiter,
                                          SourceLocation location) {
  char name[96];
  snprintf(name, sizeof(name), "%s_coroutine_resume",
           SyntaxFakeName(&compiler->syntax));
  Symbol* frame_param = NULL;
  TypeRecord* func =
      NewCoroutineResumeFunctionType(frame->type, &frame_param, location);
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
  ASTNode* wrapped_body = (ASTNode*)resume_body;
  if (CompilerExceptionsEnabled()) {
    Vector* catches = NewVector();
    VectorAppend(catches,
                 NewCatchASTNode(
                     NULL, true,
                     NewCoroutineUnhandledExceptionStatement(
                         promise, frame, final_awaiter, NULL,
                         location),
                     location));
    ASTNode* try_stmt =
        NewTryASTNode((ASTNode*)resume_body, catches, location);
    Vector* wrapped_statements = NewVector();
    VectorAppend(wrapped_statements, try_stmt);
    wrapped_body = NewCompoundStatementASTNode(wrapped_statements, location);
  }
  wrapped_body = ASTNodeVisitAndTransform(
      wrapped_body, ReplaceCoroutineResumeReturnTransform, NULL);
  SymbolReplacement replacement = {frame->symbol, frame_param};
  ASTNodeVisit(wrapped_body, ReplaceIdentifierSymbol, 0, &replacement);
  CoroutineFrame resume_frame = *frame;
  resume_frame.symbol = frame_param;
  RewriteFrameOwnedSymbols(wrapped_body, &resume_frame);
  func->info.function.body = wrapped_body;
  Symbol* symbol = NewSymbol(name, func, STO(extern));
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

/* Build a null pointer constant (integer 0). */
static ASTNode* NewNullPointerConstant(SourceLocation location) {
  return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

/* Build the coroutine destroy function `void destroy(FrameType*)`: mark the
 * frame done, null the resume/destroy pointers, run all frame-object destructors
 * in reverse order, and free the frame. Registers and queues it for codegen. */
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

/* Build `if (frame->__state == state_value) goto label;`, one arm of the resume
 * dispatch that jumps to the code following the matching suspension point. */
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
  goto_stmt->flags |= kASTCompilerGeneratedGoto;
  return NewIfStatementASTNode(condition, goto_stmt, NULL, false, location);
}

/* Create a `void*` temporary that holds the raw frame address of a coroutine to
 * which control is being symmetrically transferred. */
static Symbol* NewCoroutineTransferHandleTemporary(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_pointer = NewPointerTo(kQualPlain, void_type);
  Symbol* handle = SyntaxNewTemporary(&compiler->syntax, void_pointer);
  handle->flags.is_local = true;
  handle->flags.is_defined = true;
  handle->location = location;
  return handle;
}

/* Create a temporary to hold the coroutine_handle returned by await_suspend (the
 * next coroutine to resume). */
static Symbol* NewCoroutineHandleReturnTemporary(TypeRecord* handle_type,
                                                 SourceLocation location) {
  if (handle_type == NULL) {
    return NULL;
  }
  Symbol* handle =
      SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(handle_type));
  handle->flags.is_local = true;
  handle->flags.is_defined = true;
  handle->location = location;
  return handle;
}

/* Cast a stored `void*` transfer handle back to the typed `FrameType*`. */
static ASTNode* NewCoroutineTransferFramePointer(CoroutineFrame* frame,
                                                 Symbol* handle,
                                                 SourceLocation location) {
  ASTNode* cast =
      NewCastASTNode(NewCoroutineFramePointerType(frame->type), location,
                     NewIdentifierASTNode(handle, location));
  ((CastASTNode*)cast)->kind = kCastStatic;
  return cast;
}

/* Access `((FrameType*)handle)->member` for the coroutine being transferred to. */
static ASTNode* NewCoroutineTransferFrameMemberAccess(CoroutineFrame* frame,
                                                      Symbol* handle,
                                                      StructMember* member,
                                                      SourceLocation location) {
  ASTNode* member_node = NewStructMemberASTNode(member, location);
  ((StructMemberASTNode*)member_node)->byte_offset = member->byte_offset;
  ASTNode* access =
      NewBinaryASTNode(AST_OP(arrow), TypeRecordCopy(member->symbol->type),
                       location,
                       NewCoroutineTransferFramePointer(frame, handle, location),
                       member_node);
  access->value_category = kValueCategoryLvalue;
  return access;
}

/* Build `if (next->__resume != null) next->__resume(next);`, the symmetric
 * transfer that drives the next coroutine to its next suspension before this
 * frame's resume function returns. */
static ASTNode* NewCoroutineTransferResumeIf(CoroutineFrame* frame,
                                             Symbol* handle,
                                             SourceLocation location) {
  ASTNode* condition = NewBinaryASTNode(
      AST_OP(noteq), NewTypeRecordWithSize(kTypeBool, kQualPlain), location,
      NewCoroutineTransferFrameMemberAccess(frame, handle, frame->resume,
                                            location),
      NewNullPointerConstant(location));
  Vector* actuals = NewVector();
  VectorAppend(actuals,
               NewCoroutineTransferFramePointer(frame, handle, location));
  ASTNode* resume_call = NewVectorASTNode(
      AST_OP(call), NewTypeRecordWithSize(kTypeVoid, kQualPlain), location,
      NewCoroutineTransferFrameMemberAccess(frame, handle, frame->resume,
                                            location),
      actuals);
  return NewIfStatementASTNode(
      condition, NewExpressionStatementASTNode(resume_call, location),
      NULL, false, location);
}

/* Emit the statements that run after calling await_suspend, dispatching on its
 * return type:
 *   - bool: if it returns true, return from the ramp/resume function (suspend);
 *           otherwise fall through (resume immediately).
 *   - coroutine_handle / void*: symmetric transfer to the returned handle, then
 *           return.
 *   - void: just return (unconditional suspend). */
static void AppendCoroutineAwaitSuspendReturn(Vector* suspend_statements,
                                              CoroutineFrame* frame,
                                              ASTNode* await_suspend,
                                              TypeRecord* await_suspend_return,
                                              Symbol* return_object,
                                              SourceLocation location) {
  if (await_suspend_return != NULL && TypeIsBool(await_suspend_return)) {
    VectorAppend(suspend_statements,
                 NewIfStatementASTNode(
                     await_suspend,
                     NewCoroutineReturnObjectStatement(return_object,
                                                       location),
                     NULL, false, location));
    return;
  }
  if (await_suspend_return != NULL &&
      TypeIsCoroutineHandle(await_suspend_return)) {
    Symbol* address_member = FindCoroutineHandleAddress(await_suspend_return);
    Symbol* handle_object =
        NewCoroutineHandleReturnTemporary(await_suspend_return, location);
    if (address_member != NULL && handle_object != NULL) {
      ASTNode* address_call = NewAwaiterMemberCall(
          NewIdentifierASTNode(handle_object, location), "address", location);
      ASTNodeSetType(address_call, TypeRecordCopy(address_member->type->next));
      Symbol* transfer_handle = NewCoroutineTransferHandleTemporary(location);
      Vector* transfer_decls = NewVector();
      VectorAppend(
          transfer_decls,
          NewVariableDeclarationASTNode(
              handle_object,
              NewExpressionInitializerASTNode(await_suspend, location),
              location));
      VectorAppend(
          transfer_decls,
          NewVariableDeclarationASTNode(
              transfer_handle,
              NewVariableInitExpression(transfer_handle, address_call,
                                        location),
              location));
      VectorAppend(suspend_statements,
                   NewDeclarationListASTNode(transfer_decls, location));
      VectorAppend(suspend_statements,
                   NewCoroutineTransferResumeIf(frame, transfer_handle,
                                                location));
      VectorAppend(suspend_statements,
                   NewCoroutineReturnObjectStatement(return_object, location));
      return;
    }
  }
  if (await_suspend_return != NULL &&
      TypeIsVoidPointer(await_suspend_return)) {
    Symbol* transfer_handle = NewCoroutineTransferHandleTemporary(location);
    Vector* transfer_decls = NewVector();
    VectorAppend(
        transfer_decls,
        NewVariableDeclarationASTNode(
            transfer_handle,
            NewVariableInitExpression(transfer_handle, await_suspend,
                                      location),
            location));
    VectorAppend(suspend_statements,
                 NewDeclarationListASTNode(transfer_decls, location));
    VectorAppend(suspend_statements,
                 NewCoroutineTransferResumeIf(frame, transfer_handle,
                                              location));
    VectorAppend(suspend_statements,
                 NewCoroutineReturnObjectStatement(return_object, location));
    return;
  }
  VectorAppend(suspend_statements,
               NewExpressionStatementASTNode(await_suspend, location));
  VectorAppend(suspend_statements,
               NewCoroutineReturnObjectStatement(return_object, location));
}

/* Build the per-suspension-point suspend block: `if (!awaiter.await_ready()) {
 * frame->__state = N; <await_suspend dispatch>; }`, where the dispatch suspends
 * (returns) or continues based on await_suspend's result. */
static ASTNode* NewSuspendIf(CoroutineFrame* frame, Symbol* awaiter,
                             int state_value, Symbol* return_object,
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
  VectorAppend(suspend_actuals,
               NewAwaitSuspendHandleArgument(frame, awaiter, location));
  ASTNode* await_suspend = NewAwaiterMemberCallWithActuals(
      NewIdentifierASTNode(awaiter, location), "await_suspend",
      suspend_actuals, location);
  TypeRecord* await_suspend_return = AwaiterAwaitSuspendReturnType(awaiter);
  if (await_suspend_return != NULL) {
    ASTNodeSetType(await_suspend, TypeRecordCopy(await_suspend_return));
  }
  AppendCoroutineAwaitSuspendReturn(suspend_statements, frame, await_suspend,
                                    await_suspend_return, return_object,
                                    location);
  return NewIfStatementASTNode(
      not_ready, NewCompoundStatementASTNode(suspend_statements, location),
      NULL, false, location);
}

static void CoroutineCompoundInsertStatement(CompoundStatementASTNode* compound,
                                             ASTNode* stmt,
                                             size_t at_index);

/* Insert guarded destructors for a suspension point's awaiter and any
 * frame-materialized co_yield prvalue right after await_resume consumes them.
 * The awaiter was constructed last, so destroy it before the yielded value. */
static void InsertCoroutineAwaiterDestructorAfterUse(
    SuspensionPoint* point, CoroutineFrame* frame, size_t statement_index,
    SourceLocation location) {
  ASTNode* awaiter_dtor = NewCoroutineFrameMemberGuardedDestructor(
      frame, point->frame_member, point->frame_constructed_member, location);
  ASTNode* yielded_value_dtor = NewCoroutineFrameMemberGuardedDestructor(
      frame, point->yielded_value_member,
      point->yielded_value_constructed_member, location);
  if (awaiter_dtor == NULL && yielded_value_dtor == NULL) {
    return;
  }
  ASTNode* use_stmt = point->compound->statements->value.p[statement_index];
  if (use_stmt != NULL && use_stmt->op == AST_OP(compound)) {
    size_t index = 1;
    if (awaiter_dtor != NULL) {
      CompoundASTNodeInsertStatement((CompoundStatementASTNode*)use_stmt,
                                     awaiter_dtor, index++);
    }
    if (yielded_value_dtor != NULL) {
      CompoundASTNodeInsertStatement((CompoundStatementASTNode*)use_stmt,
                                     yielded_value_dtor, index);
    }
    return;
  }
  size_t index = statement_index + 1;
  if (awaiter_dtor != NULL) {
    CoroutineCompoundInsertStatement(point->compound, awaiter_dtor, index++);
  }
  if (yielded_value_dtor != NULL) {
    CoroutineCompoundInsertStatement(point->compound, yielded_value_dtor,
                                     index);
  }
}

/* Build an initializer expression `awaiter.await_resume()` of `result_type` (used
 * to initialize the temporary that holds a co_await/co_yield's result). */
static ASTNode* NewAwaitResumeInitializer(Symbol* awaiter,
                                          TypeRecord* result_type,
                                          SourceLocation location) {
  ASTNode* await_resume =
      NewAwaiterMemberCall(NewIdentifierASTNode(awaiter, location),
                           "await_resume", location);
  ASTNodeSetType(await_resume, result_type);
  return NewExpressionInitializerASTNode(await_resume, location);
}

/* Build a statement `awaiter.await_resume();` for a void-result suspension. */
static ASTNode* NewAwaitResumeStatement(Symbol* awaiter,
                                        SourceLocation location) {
  return NewExpressionStatementASTNode(
      NewAwaiterMemberCall(NewIdentifierASTNode(awaiter, location),
                           "await_resume", location),
      location);
}

/* Initialize the frame's initial-awaiter slot from `promise.initial_suspend()`. */
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

/* Build the initial-suspend block, analogous to NewSuspendIf but for the initial
 * awaiter (state 0): `if (!awaiter.await_ready()) { <await_suspend dispatch> }`. */
static ASTNode* NewInitialSuspendIf(CoroutineFrame* frame, Symbol* awaiter,
                                    Symbol* return_object,
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
  VectorAppend(suspend_actuals,
               NewAwaitSuspendHandleArgument(frame, awaiter, location));
  ASTNode* await_suspend = NewAwaiterMemberCallWithActuals(
      NewIdentifierASTNode(awaiter, location), "await_suspend",
      suspend_actuals, location);
  TypeRecord* await_suspend_return = AwaiterAwaitSuspendReturnType(awaiter);
  if (await_suspend_return != NULL) {
    ASTNodeSetType(await_suspend, TypeRecordCopy(await_suspend_return));
  }
  AppendCoroutineAwaitSuspendReturn(suspend_statements, frame, await_suspend,
                                    await_suspend_return, return_object,
                                    location);
  return NewIfStatementASTNode(
      not_ready, NewCompoundStatementASTNode(suspend_statements, location),
      NULL, false, location);
}

/* No-op transform used only for its side effect of re-walking the tree (so
 * parent/child_id links get refreshed). */
static ASTNode* PreserveASTNodeTransform(ASTNode* node, void* data,
                                         ASTNodeTransformAction* action) {
  (void)data;
  (void)action;
  return node;
}

/* Insert `stmt` into `compound` at `at_index` (appending if the index is past
 * the end), fixing up parent links. */
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

/* Refresh parent and child_id links across a compound after structural edits. */
static void ResetCompoundStatementParents(CompoundStatementASTNode* compound) {
  if (compound == NULL) {
    return;
  }
  ASTNodeVisitAndTransform((ASTNode*)compound, PreserveASTNodeTransform, NULL);
}

/* Locate the compound and index where `stmt` lives (only if it is a direct
 * statement child of a compound). */
static bool CoroutineStatementLocation(ASTNode* stmt,
                                       CompoundStatementASTNode** compound,
                                       size_t* statement_index) {
  if (stmt == NULL || stmt->parent == NULL ||
      stmt->parent->op != AST_OP(compound) || stmt->child_id < 0) {
    return false;
  }
  *compound = (CompoundStatementASTNode*)stmt->parent;
  *statement_index = (size_t)stmt->child_id;
  return true;
}

/* Reserve and return the next slot in the suspension-points array, or NULL when
 * full. */
static SuspensionPoint* NextSuspensionPoint(SuspensionPoints* points) {
  if (points == NULL || points->count >= points->capacity) {
    return NULL;
  }
  return &points->points[points->count++];
}

/* Record a suspension point for a (non-always-ready) co_await found directly in a
 * statement. */
static void AddCoAwaitSuspensionPoint(SuspensionPoints* points,
                                      ASTNode* stmt,
                                      ASTNode* co_await) {
  if (co_await == NULL || co_await->op != AST_OP(co_await) ||
      CoAwaitIsAlwaysReady(co_await)) {
    return;
  }
  CompoundStatementASTNode* compound = NULL;
  size_t statement_index = 0;
  if (!CoroutineStatementLocation(stmt, &compound, &statement_index)) {
    return;
  }
  SuspensionPoint* point = NextSuspensionPoint(points);
  if (point == NULL) {
    return;
  }
  point->kind = kSuspensionCoAwait;
  point->co_await = co_await;
  point->statement = stmt;
  point->compound = compound;
  point->statement_index = statement_index;
}

/* Record a suspension point for a co_yield found directly in a statement. */
static void AddCoYieldSuspensionPoint(SuspensionPoints* points,
                                      ASTNode* stmt,
                                      ASTNode* co_yield) {
  if (co_yield == NULL || co_yield->op != AST_OP(co_yield)) {
    return;
  }
  CompoundStatementASTNode* compound = NULL;
  size_t statement_index = 0;
  if (!CoroutineStatementLocation(stmt, &compound, &statement_index)) {
    return;
  }
  SuspensionPoint* point = NextSuspensionPoint(points);
  if (point == NULL) {
    return;
  }
  point->kind = kSuspensionCoYield;
  point->co_yield = co_yield;
  point->statement = stmt;
  point->compound = compound;
  point->statement_index = statement_index;
}

/* Record suspension points for co_await initializers in a declaration list (e.g.
 * `auto x = co_await ...;`), tracking the declared variable for each. */
static void AddDeclarationCoAwaitSuspensionPoints(
    SuspensionPoints* points,
    DeclarationListASTNode* decl_list,
    CompoundStatementASTNode* compound,
    size_t statement_index) {
  for (size_t i = 0; i < decl_list->declarations->length; i++) {
    VariableDeclarationASTNode* decl = decl_list->declarations->value.p[i];
    if (decl == NULL) {
      continue;
    }
    ASTNode* expr = VariableInitializerExpression(decl->initializer);
    if (expr == NULL || expr->op != AST_OP(co_await) ||
        CoAwaitIsAlwaysReady(expr)) {
      continue;
    }
    SuspensionPoint* point = NextSuspensionPoint(points);
    if (point == NULL) {
      continue;
    }
    point->kind = kSuspensionCoAwait;
    point->co_await = expr;
    point->value_decl = decl;
    point->decl_list = decl_list;
    point->compound = compound;
    point->statement_index = statement_index;
  }
}

/* Record suspension points for co_yield initializers in a declaration list. */
static void AddDeclarationCoYieldSuspensionPoints(
    SuspensionPoints* points,
    DeclarationListASTNode* decl_list,
    CompoundStatementASTNode* compound,
    size_t statement_index) {
  for (size_t i = 0; decl_list != NULL && i < decl_list->declarations->length;
       i++) {
    VariableDeclarationASTNode* decl = decl_list->declarations->value.p[i];
    if (decl == NULL) {
      continue;
    }
    ASTNode* expr = VariableInitializerExpression(decl->initializer);
    if (expr == NULL || expr->op != AST_OP(co_yield)) {
      continue;
    }
    SuspensionPoint* point = NextSuspensionPoint(points);
    if (point == NULL) {
      continue;
    }
    point->kind = kSuspensionCoYield;
    point->co_yield = expr;
    point->statement = (ASTNode*)decl_list;
    point->value_decl = decl;
    point->decl_list = decl_list;
    point->compound = compound;
    point->statement_index = statement_index;
  }
}

/* True if a suspension point is a bare `co_yield expr;` expression statement (no
 * surrounding assignment/declaration consuming its result). */
static bool SuspensionPointIsStatementCoYield(SuspensionPoint* point) {
  return point != NULL && point->kind == kSuspensionCoYield &&
         point->statement != NULL && point->statement->op == AST_OP(expr) &&
         ((ExpressionStatementASTNode*)point->statement)->expr ==
             point->co_yield;
}

/* Visitor that records every statement-level suspension point in the (already
 * normalized) body: co_return operands, expression-statement co_await/co_yield,
 * assignment RHSs, and declaration initializers. */
static void CollectSuspensionPointNode(ASTNode* node, void* data,
                                       int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  SuspensionPoints* points = data;
  if (node->op == AST_OP(co_return)) {
    AddCoAwaitSuspensionPoint(points, node,
                              ((CombinedStatementASTNode*)node)->cond);
    return;
  }
  if (node->op == AST_OP(expr)) {
    ASTNode* expr = ((ExpressionStatementASTNode*)node)->expr;
    AddCoYieldSuspensionPoint(points, node, expr);
    AddCoAwaitSuspensionPoint(points, node, expr);
    if (expr != NULL && expr->op == AST_OP(assign)) {
      AddCoYieldSuspensionPoint(points, node, ((BinaryASTNode*)expr)->right);
      AddCoAwaitSuspensionPoint(points, node, ((BinaryASTNode*)expr)->right);
    }
    return;
  }
  if (node->op == AST_OP(decl_list)) {
    CompoundStatementASTNode* compound = NULL;
    size_t statement_index = 0;
    if (CoroutineStatementLocation(node, &compound, &statement_index)) {
      AddDeclarationCoAwaitSuspensionPoints(
          points, (DeclarationListASTNode*)node, compound, statement_index);
      AddDeclarationCoYieldSuspensionPoints(
          points, (DeclarationListASTNode*)node, compound, statement_index);
    }
  }
}

/* Collect all suspension points in a coroutine body into `points`. */
static void CollectSuspensionPointsInCompound(CompoundStatementASTNode* body,
                                              SuspensionPoints* points) {
  ASTNodeVisit((ASTNode*)body, CollectSuspensionPointNode, 0, points);
}

/* Scratch state for detecting whether a symbol is referenced in a subtree. */
typedef struct {
  Symbol* symbol;
  bool found;
} SymbolUseSearch;

/* Visitor that flags `found` when a non-declaration reference to the target
 * symbol is seen. */
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

/* True if `node` references `symbol` anywhere (outside its own declaration). */
static bool ASTUsesSymbol(ASTNode* node, Symbol* symbol) {
  if (node == NULL || symbol == NULL) {
    return false;
  }
  SymbolUseSearch search = {symbol, false};
  ASTNodeVisit(node, FindSymbolUse, 0, &search);
  return search.found;
}

/* True if `symbol` is already in the list of locals to persist into the frame. */
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

/* True if a symbol holds a plain scalar value that can be trivially copied into
 * and out of a frame slot. */
static bool CoroutineScalarCanBePersisted(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL ||
      TypeIsStructOrUnion(symbol->type) || TypeIsArray(symbol->type) ||
      TypeIsFunction(symbol->type) || TypeIsReference(symbol->type)) {
    return false;
  }
  return true;
}

/* True if a parameter can be persisted into the frame; for class types, prefer
 * moving (sets *move_parameter) when a move constructor exists, else require a
 * copy constructor. */
static bool CoroutineParameterCanBePersisted(Symbol* symbol,
                                             bool* move_parameter) {
  if (move_parameter != NULL) {
    *move_parameter = false;
  }
  if (symbol != NULL && TypeIsReference(symbol->type)) {
    return true;
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

/* True if a local can be represented directly in the coroutine frame.  Unlike
 * parameters, locals are initialized in their frame slots at their declaration
 * sites, so class types do not need a copy or move constructor. */
static bool CoroutineLocalCanBePersisted(Symbol* symbol) {
  if (CoroutineScalarCanBePersisted(symbol)) {
    return true;
  }
  return symbol != NULL && symbol->type != NULL &&
         TypeIsStructOrUnion(symbol->type);
}

/* Catch parameters cannot be initialized directly in a frame slot because the
 * exception machinery creates them.  Preserve the old copy-first, move-second
 * transfer into the frame. */
static bool CoroutineCatchParameterCanBePersisted(Symbol* symbol,
                                                  bool* move_parameter) {
  if (move_parameter != NULL) {
    *move_parameter = false;
  }
  if (symbol != NULL && TypeIsReference(symbol->type)) {
    return true;
  }
  if (CoroutineScalarCanBePersisted(symbol)) {
    return true;
  }
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsStructOrUnion(symbol->type)) {
    return false;
  }
  if (CoroutineTypeCanBeCopyConstructed(symbol->type)) {
    return true;
  }
  if (CoroutineTypeCanBeMoveConstructed(symbol->type)) {
    if (move_parameter != NULL) {
      *move_parameter = true;
    }
    return true;
  }
  return false;
}

/* True if any statement at or after `first_index` in `body` uses `symbol`. */
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

/* Scratch state for checking whether a symbol is used after a suspension point,
 * including via enclosing-loop conditions/updates and later sibling statements. */
typedef struct {
  CompoundStatementASTNode* root;
  Symbol* symbol;
  bool found;
} SuspensionUseAfterSearch;

/* Upward visitor: detects a use of the symbol that would execute after the
 * suspension — a loop condition/increment of an enclosing while/do/for, or a
 * later statement in an enclosing compound — meaning the symbol's value must
 * survive the suspension and so be persisted in the frame. */
static bool FindSuspensionUseAfterInAncestors(ASTNode* current, void* data) {
  SuspensionUseAfterSearch* search = data;
  if (current == NULL || current == (ASTNode*)search->root) {
    return false;
  }
  ASTNode* parent = current->parent;
  if (parent == NULL) {
    return false;
  }
  if ((parent->op == AST_OP(while) || parent->op == AST_OP(do)) &&
      ((CombinedStatementASTNode*)parent)->stmt == current &&
      ASTUsesSymbol(((CombinedStatementASTNode*)parent)->cond,
                    search->symbol)) {
    search->found = true;
    return false;
  }
  if (parent->op == AST_OP(for) &&
      ((ForStatementASTNode*)parent)->stmt == current) {
    ForStatementASTNode* loop = (ForStatementASTNode*)parent;
    if (ASTUsesSymbol(loop->c2, search->symbol) ||
        ASTUsesSymbol(loop->c3, search->symbol)) {
      search->found = true;
      return false;
    }
  }
  if (parent->op == AST_OP(compound) &&
      StatementRangeUsesSymbol((CompoundStatementASTNode*)parent,
                               (size_t)current->child_id + 1,
                               search->symbol)) {
    search->found = true;
    return false;
  }
  return true;
}

/* True if `symbol` is read after the given suspension point (in later statements
 * of the same compound or in enclosing loop conditions/updates and blocks),
 * which is the condition for needing to persist it into the frame. */
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
  SuspensionUseAfterSearch search = {
      .root = root,
      .symbol = symbol,
  };
  ASTNodeVisitUpwards((ASTNode*)point->compound,
                      FindSuspensionUseAfterInAncestors, &search);
  return search.found;
}

/* Add a symbol to the persisted-locals list (deduplicated), recording how it
 * should be stored into the frame. */
static void AddPersistedCoroutineLocal(Vector* persisted_locals,
                                       Symbol* symbol,
                                       bool is_parameter,
                                       bool is_catch_parameter,
                                       bool move_parameter,
                                       CompoundStatementASTNode* store_compound) {
  if (PersistedLocalVectorContains(persisted_locals, symbol)) {
    return;
  }
  CoroutinePersistedLocal* local = malloc(sizeof(CoroutinePersistedLocal));
  assert(local != NULL);
  local->symbol = symbol;
  local->member = NULL;
  local->constructed_member = NULL;
  local->is_parameter = is_parameter;
  local->is_catch_parameter = is_catch_parameter;
  local->move_parameter = move_parameter;
  local->store_compound = store_compound;
  VectorAppend(persisted_locals, local);
}

static void CollectPersistedCoroutineDeclarationList(
    CompoundStatementASTNode* root,
    DeclarationListASTNode* decls,
    SuspensionPoint* point,
    Vector* persisted_locals,
    bool is_for_initializer,
    bool* ok) {
  if (decls == NULL || decls->declarations == NULL) {
    return;
  }
  for (size_t decl_index = 0; decl_index < decls->declarations->length;
       decl_index++) {
    VariableDeclarationASTNode* decl =
        decls->declarations->value.p[decl_index];
    Symbol* symbol = decl != NULL ? decl->symbol : NULL;
    if (!SuspensionPointUsesSymbolAfter(root, point, symbol)) {
      continue;
    }
    (void)is_for_initializer;
    if (!CoroutineLocalCanBePersisted(symbol)) {
      SemanticError(
          (ASTNode*)decl,
          "coroutine local live across suspension has unsupported type");
      *ok = false;
      continue;
    }
    AddPersistedCoroutineLocal(persisted_locals, symbol, false, false,
                               false, NULL);
  }
}

/* Examine declarations in `compound` before `limit` and, for any local that is
 * used after `point`, mark it for direct construction in the frame. Clears *ok
 * when the local has a representation that cannot be persisted. */
static void CollectPersistedCoroutineLocalsBeforeIndex(
    CompoundStatementASTNode* root,
    CompoundStatementASTNode* compound,
    size_t limit,
    SuspensionPoint* point,
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
    CollectPersistedCoroutineDeclarationList(
        root, (DeclarationListASTNode*)stmt, point, persisted_locals, false,
        ok);
  }
}

/* Context for collecting persisted locals/catch-parameters while walking up the
 * ancestors of a suspension point. */
typedef struct {
  CompoundStatementASTNode* root;
  SuspensionPoint* point;
  Vector* persisted_locals;
  bool* ok;
} PersistedLocalCollectionSearch;

/* Recurse to the outermost enclosing scope first, then collect declarations as
 * the recursion unwinds toward the suspension.  This keeps persisted locals in
 * construction order, which is also needed for reverse-order frame teardown. */
static void CollectPersistedCoroutineLocalsFromAncestors(
    ASTNode* current, PersistedLocalCollectionSearch* search) {
  if (current == NULL || current == (ASTNode*)search->root) {
    return;
  }
  ASTNode* parent = current->parent;
  if (parent == NULL) {
    return;
  }
  CollectPersistedCoroutineLocalsFromAncestors(parent, search);
  if (parent->op == AST_OP(compound)) {
    CollectPersistedCoroutineLocalsBeforeIndex(
        search->root, (CompoundStatementASTNode*)parent,
        (size_t)current->child_id, search->point, search->persisted_locals,
        search->ok);
  } else if (parent->op == AST_OP(for)) {
    ForStatementASTNode* loop = (ForStatementASTNode*)parent;
    if (loop->c1 != NULL && loop->c1->op == AST_OP(decl_list)) {
      CollectPersistedCoroutineDeclarationList(
          search->root, (DeclarationListASTNode*)loop->c1, search->point,
          search->persisted_locals, true, search->ok);
    }
  } else if (parent->op == AST_OP(expansion_for)) {
    ExpansionStatementASTNode* expansion = (ExpansionStatementASTNode*)parent;
    if (expansion->init_stmt != NULL &&
        expansion->init_stmt->op == AST_OP(decl_list)) {
      CollectPersistedCoroutineDeclarationList(
          search->root, (DeclarationListASTNode*)expansion->init_stmt,
          search->point, search->persisted_locals, true, search->ok);
    }
  }
}

/* Collect all locals that must be persisted for a single suspension point: those
 * declared earlier in the same compound and those in enclosing compounds. */
static void CollectPersistedCoroutineLocalsForPoint(
    CompoundStatementASTNode* root,
    SuspensionPoint* point,
    Vector* persisted_locals,
    bool* ok) {
  if (root == NULL || point == NULL || point->compound == NULL) {
    return;
  }
  PersistedLocalCollectionSearch search = {
      .root = root,
      .point = point,
      .persisted_locals = persisted_locals,
      .ok = ok,
  };
  CollectPersistedCoroutineLocalsFromAncestors(
      (ASTNode*)point->compound, &search);
  CollectPersistedCoroutineLocalsBeforeIndex(
      root, point->compound, point->statement_index, point, persisted_locals,
      ok);
}

/* Upward visitor: persist an enclosing catch clause's exception parameter if it
 * is used after the suspension (it would otherwise be lost across resumption). */
static bool CollectPersistedCoroutineCatchParameterFromAncestor(
    ASTNode* current, void* data) {
  PersistedLocalCollectionSearch* search = data;
  if (current == NULL || current == (ASTNode*)search->root) {
    return false;
  }
  ASTNode* parent = current->parent;
  if (parent == NULL) {
    return false;
  }
  if (parent->op != AST_OP(catch)) {
    return true;
  }
  CatchASTNode* catch_stmt = (CatchASTNode*)parent;
  Symbol* symbol = catch_stmt->symbol;
  if (symbol == NULL ||
      !SuspensionPointUsesSymbolAfter(search->root, search->point, symbol)) {
    return true;
  }
  bool move_parameter = false;
  if (!CoroutineCatchParameterCanBePersisted(symbol, &move_parameter)) {
    SemanticError(
        (ASTNode*)catch_stmt,
        "coroutine catch parameter live across suspension requires a copy or "
        "move constructor");
    *search->ok = false;
    return true;
  }
  CompoundStatementASTNode* store_compound =
      catch_stmt->stmt != NULL && catch_stmt->stmt->op == AST_OP(compound)
          ? (CompoundStatementASTNode*)catch_stmt->stmt
          : NULL;
  AddPersistedCoroutineLocal(search->persisted_locals, symbol, false, true,
                             move_parameter, store_compound);
  return true;
}

/* Collect catch-clause parameters that must be persisted for a suspension point. */
static void CollectPersistedCoroutineCatchParametersForPoint(
    CompoundStatementASTNode* root,
    SuspensionPoint* point,
    Vector* persisted_locals,
    bool* ok) {
  if (root == NULL || point == NULL || point->compound == NULL ||
      persisted_locals == NULL || ok == NULL) {
    return;
  }
  PersistedLocalCollectionSearch search = {
      .root = root,
      .point = point,
      .persisted_locals = persisted_locals,
      .ok = ok,
  };
  ASTNodeVisitUpwards(
      (ASTNode*)point->compound,
      CollectPersistedCoroutineCatchParameterFromAncestor, &search);
}

typedef struct {
  SuspensionPoints* points;
  Vector* persisted_locals;
  bool* ok;
} PersistedRangeForTemporaryCollection;

static bool CoroutineNodeIsWithin(ASTNode* node, ASTNode* ancestor) {
  for (ASTNode* current = node; current != NULL; current = current->parent) {
    if (current == ancestor) {
      return true;
    }
  }
  return false;
}

static bool CoroutineRangeForContainsSuspension(
    CompoundStatementASTNode* compound, SuspensionPoints* points) {
  for (int i = 0; points != NULL && i < points->count; i++) {
    ASTNode* suspension =
        points->points[i].kind == kSuspensionCoAwait
            ? points->points[i].co_await
            : points->points[i].co_yield;
    if (CoroutineNodeIsWithin(suspension, (ASTNode*)compound)) {
      return true;
    }
  }
  return false;
}

static void AnalyzeCoroutineRangeForDeclarations(
    ASTNode* node, void* data, int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(compound) ||
      (node->flags & kASTRangeForInitializer) == 0) {
    return;
  }
  CompoundStatementASTNode* range = (CompoundStatementASTNode*)node;
  if (range->statements == NULL || range->statements->length < 2) {
    return;
  }
  AnalyzeStatement(range->statements->value.p[0]);
  ASTNode* iteration = range->statements->value.p[1];
  if (iteration == NULL || iteration->op != AST_OP(compound)) {
    return;
  }
  CompoundStatementASTNode* declarations =
      (CompoundStatementASTNode*)iteration;
  for (size_t i = 0; i < declarations->statements->length; i++) {
    ASTNode* statement = declarations->statements->value.p[i];
    if (statement == NULL || statement->op != AST_OP(decl_list)) {
      break;
    }
    AnalyzeStatement(statement);
  }
}

static void CollectPersistedCoroutineRangeForTemporaries(
    ASTNode* node, void* data, int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(compound) ||
      (node->flags & kASTRangeForInitializer) == 0) {
    return;
  }
  PersistedRangeForTemporaryCollection* collection = data;
  CompoundStatementASTNode* compound = (CompoundStatementASTNode*)node;
  if (!CoroutineRangeForContainsSuspension(compound, collection->points) ||
      compound->statements == NULL || compound->statements->length == 0) {
    return;
  }
  Vector temporaries;
  VectorInit(&temporaries);
  CXXCollectRangeForInitializerTemporaries(
      compound->statements->value.p[0], &temporaries);
  for (size_t i = temporaries.length; i > 0; i--) {
    Symbol* symbol = temporaries.value.p[i - 1];
    if (!CoroutineLocalCanBePersisted(symbol)) {
      SemanticError(node,
                    "coroutine range-for temporary has unsupported type");
      *collection->ok = false;
      continue;
    }
    AddPersistedCoroutineLocal(collection->persisted_locals, symbol, false,
                               false, false, compound);
  }
  VectorDestruct(&temporaries);
}

/* Build the full set of locals and catch-parameters that live across any
 * suspension point and so must be promoted into the frame. Returns false if any
 * such object cannot be represented there (catch parameters still require a
 * copy or move). */
static bool CollectPersistedCoroutineLocals(
    CompoundStatementASTNode* body,
    SuspensionPoints* points,
    Vector* persisted_locals) {
  VectorInit(persisted_locals);
  if (body == NULL || points == NULL || points->count == 0) {
    return true;
  }
  bool ok = true;
  PersistedRangeForTemporaryCollection range_collection = {
      .points = points,
      .persisted_locals = persisted_locals,
      .ok = &ok,
  };
  ASTNodeVisit((ASTNode*)body,
               CollectPersistedCoroutineRangeForTemporaries, 0,
               &range_collection);
  for (int i = 0; i < points->count; i++) {
    CollectPersistedCoroutineLocalsForPoint(body, &points->points[i],
                                            persisted_locals, &ok);
    CollectPersistedCoroutineCatchParametersForPoint(
        body, &points->points[i], persisted_locals, &ok);
  }
  return ok;
}

/* Persist every referenced function parameter, plus every class parameter even
 * when unused because its copy/move construction and destruction are observable.
 * Parameters outlive the ramp and so are copied/moved into the frame. */
static bool CollectPersistedCoroutineParameters(
    FunctionInfo* info,
    CompoundStatementASTNode* body,
    SuspensionPoints* points,
    Vector* persisted_locals) {
  if (info == NULL || body == NULL || points == NULL ||
      persisted_locals == NULL) {
    return true;
  }
  bool ok = true;
  for (size_t i = 0; i < info->prototype.length; i++) {
    Symbol* symbol = info->prototype.value.p[i];
    if (!ASTUsesSymbol((ASTNode*)body, symbol) &&
        !TypeIsStructOrUnion(symbol->type)) {
      continue;
    }
    bool move_parameter = false;
    if (!CoroutineParameterCanBePersisted(symbol, &move_parameter)) {
      SemanticError(
          (ASTNode*)body,
          "coroutine parameter live across suspension requires a copy or move "
          "constructor");
      ok = false;
      continue;
    }
    AddPersistedCoroutineLocal(persisted_locals, symbol, true, false,
                               move_parameter, NULL);
  }
  return ok;
}

/* Free the persisted-locals bookkeeping list. */
static void PersistedCoroutineLocalsDestruct(Vector* persisted_locals) {
  if (persisted_locals == NULL) {
    return;
  }
  for (size_t i = 0; i < persisted_locals->length; i++) {
    free(persisted_locals->value.p[i]);
  }
  VectorDestruct(persisted_locals);
}

/* Look up a persisted-local record by its symbol. */
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

/* After inserting a statement at `insert_index`, shift the recorded statement
 * indices of any suspension points in the same compound that come at or after
 * it, keeping their positions accurate. */
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
    SuspensionPoints* points);

typedef struct {
  Symbol* symbol;
  CoroutineFrame* frame;
  StructMember* member;
} PersistedLocalInitializerRewrite;

/* Retarget every reference to the declared local within its initializer to the
 * corresponding frame member.  This turns the parser's `local.Ctor(args...)`
 * or `init(local, value)` directly into initialization of `frame->__localN`,
 * without introducing an intermediate local object. */
static ASTNode* RewritePersistedLocalInitializerTransform(
    ASTNode* node, void* data, ASTNodeTransformAction* action) {
  (void)action;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return node;
  }
  PersistedLocalInitializerRewrite* rewrite = data;
  if (((IdentifierASTNode*)node)->symbol != rewrite->symbol) {
    return node;
  }
  ASTNode* access = NewFrameMemberAccess(rewrite->frame, rewrite->member,
                                         node->location);
  access->flags |= node->flags & (kASTNeedAddress | kASTIsDeclaration);
  return access;
}

/* Retarget a persisted local's original initializer to the frame slot and mark
 * the slot constructed afterwards.  Ordinary initializers move into the
 * inserted statement so variable codegen cannot redirect class-return elision
 * back to stack storage.  Initializers containing a tracked co_await/co_yield
 * stay attached until suspension lowering rewrites them. */
static ASTNode* NewPersistedCoroutineLocalFrameInitialization(
    VariableDeclarationASTNode* decl,
    CoroutinePersistedLocal* local,
    CoroutineFrame* frame,
    bool keep_initializer_on_declaration) {
  if (decl == NULL || local == NULL || local->member == NULL ||
      frame == NULL) {
    return NULL;
  }

  Vector* statements = NewVector();
  if (decl->initializer != NULL) {
    PersistedLocalInitializerRewrite rewrite = {
        .symbol = local->symbol,
        .frame = frame,
        .member = local->member,
    };
    decl->initializer = ASTNodeVisitAndTransform(
        decl->initializer, RewritePersistedLocalInitializerTransform, &rewrite);
    if (!keep_initializer_on_declaration) {
      VectorAppend(
          statements,
          NewExpressionStatementASTNode(ASTNodeMove(decl->initializer),
                                        decl->base.location));
    }
  }
  if (local->constructed_member != NULL) {
    VectorAppend(statements,
                 NewFrameIntAssignment(frame, local->constructed_member, 1,
                                       decl->base.location));
  }
  if (statements->length == 0) {
    VectorDelete(statements);
    return NULL;
  }
  if (statements->length == 1) {
    ASTNode* statement = statements->value.p[0];
    VectorDelete(statements);
    return statement;
  }
  return NewCompoundStatementASTNode(statements, decl->base.location);
}

/* Suspension lowering keeps a pointer to declarations initialized by co_await
 * or co_yield, so those initializers must remain attached until that lowering
 * has replaced the suspended expression. */
static bool CoroutineSuspensionUsesValueDeclaration(
    SuspensionPoints* points, VariableDeclarationASTNode* decl) {
  if (points == NULL || decl == NULL) {
    return false;
  }
  for (int i = 0; i < points->count; i++) {
    if (points->points[i].value_decl == decl) {
      return true;
    }
  }
  return false;
}

/* If `stmt` is a direct destructor call on a persisted local, return its frame
 * descriptor.  Parser-generated scope-exit destructors and explicit destructor
 * calls share this shape and both end the object's current lifetime. */
static CoroutinePersistedLocal* PersistedLocalDestroyedByStatement(
    ASTNode* stmt, Vector* persisted_locals) {
  if (stmt == NULL || stmt->op != AST_OP(expr) ||
      persisted_locals == NULL) {
    return NULL;
  }
  ASTNode* expr = ((ExpressionStatementASTNode*)stmt)->expr;
  if (expr == NULL || expr->op != AST_OP(call)) {
    return NULL;
  }
  ASTNode* callee = ((VectorASTNode*)expr)->left;
  if (callee == NULL || callee->op != AST_OP(dot)) {
    return NULL;
  }
  BinaryASTNode* member_call = (BinaryASTNode*)callee;
  if (member_call->left == NULL ||
      member_call->left->op != AST_OP(identifier) ||
      member_call->right == NULL ||
      member_call->right->op != AST_OP(string)) {
    return NULL;
  }
  String* member_name = ((ConstantASTNode*)member_call->right)->value.string;
  if (member_name == NULL || member_name->value == NULL ||
      member_name->value[0] != '~') {
    return NULL;
  }
  CoroutinePersistedLocal* local = FindPersistedCoroutineLocal(
      persisted_locals,
      ((IdentifierASTNode*)member_call->left)->symbol);
  if (local == NULL || local->member == NULL || local->is_parameter ||
      local->is_catch_parameter) {
    return NULL;
  }
  return local;
}

typedef struct {
  CoroutineFrame* frame;
  Vector* persisted_locals;
  Vector loops;
} PersistedForInitializerHoist;

static bool PersistedCoroutineForInitializerNeedsHoist(
    ForStatementASTNode* loop, Vector* persisted_locals) {
  if (loop == NULL || loop->c1 == NULL ||
      loop->c1->op != AST_OP(decl_list)) {
    return false;
  }
  DeclarationListASTNode* decls = (DeclarationListASTNode*)loop->c1;
  for (size_t i = 0; decls->declarations != NULL &&
                     i < decls->declarations->length; i++) {
    VariableDeclarationASTNode* decl = decls->declarations->value.p[i];
    CoroutinePersistedLocal* local = FindPersistedCoroutineLocal(
        persisted_locals, decl != NULL ? decl->symbol : NULL);
    if (local != NULL && local->member != NULL && !local->is_parameter &&
        !local->is_catch_parameter && local->constructed_member != NULL) {
      return true;
    }
  }
  return false;
}

static void CollectPersistedCoroutineForInitializers(
    ASTNode* node, void* data, int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(for)) {
    return;
  }
  PersistedForInitializerHoist* hoist = data;
  if (PersistedCoroutineForInitializerNeedsHoist(
          (ForStatementASTNode*)node, hoist->persisted_locals)) {
    VectorAppend(&hoist->loops, node);
  }
}

/* A class object declared in a for-init statement has the lifetime of the
 * entire loop. Hoist an initializer containing a persisted class object into a
 * synthetic enclosing compound:
 *
 *   for (T value = init; cond; step) body
 * becomes
 *   { T value = init; for (; cond; step) body; guarded-destroy(value); }
 *
 * This gives frame construction and every exit path a real lexical scope while
 * preserving continue semantics (the increment remains on the inner for). */
static void HoistPersistedCoroutineForInitializer(
    ForStatementASTNode* loop, PersistedForInitializerHoist* hoist) {
  if (!PersistedCoroutineForInitializerNeedsHoist(
          loop, hoist->persisted_locals) ||
      loop->base.parent == NULL) {
    return;
  }
  DeclarationListASTNode* decls = (DeclarationListASTNode*)loop->c1;
  ASTNode* old_parent = loop->base.parent;
  int old_child_id = loop->base.child_id;
  Vector* statements = NewVector();
  VectorAppend(statements, loop->c1);
  loop->c1 = NULL;
  VectorAppend(statements, (ASTNode*)loop);
  for (size_t i = decls->declarations->length; i > 0; i--) {
    VariableDeclarationASTNode* decl = decls->declarations->value.p[i - 1];
    CoroutinePersistedLocal* local = FindPersistedCoroutineLocal(
        hoist->persisted_locals, decl != NULL ? decl->symbol : NULL);
    if (local == NULL || local->member == NULL || local->is_parameter ||
        local->is_catch_parameter || local->constructed_member == NULL) {
      continue;
    }
    ASTNode* dtor = NewCoroutineFrameMemberGuardedDestructor(
        hoist->frame, local->member, local->constructed_member,
        loop->base.location);
    if (dtor != NULL) {
      VectorAppend(statements, dtor);
    }
  }
  ASTNode* compound =
      NewCompoundStatementASTNode(statements, loop->base.location);
  ASTNodeReplaceChild(old_parent, old_child_id, compound, false);
}

static void HoistPersistedCoroutineForInitializers(
    CompoundStatementASTNode* body, CoroutineFrame* frame,
    Vector* persisted_locals) {
  if (body == NULL || frame == NULL || persisted_locals == NULL) {
    return;
  }
  PersistedForInitializerHoist hoist = {
      .frame = frame,
      .persisted_locals = persisted_locals,
  };
  VectorInit(&hoist.loops);
  ASTNodeVisit((ASTNode*)body, CollectPersistedCoroutineForInitializers, 0,
               &hoist);
  for (size_t i = hoist.loops.length; i > 0; i--) {
    HoistPersistedCoroutineForInitializer(hoist.loops.value.p[i - 1], &hoist);
  }
  VectorDestruct(&hoist.loops);
}

/* Retarget persisted scalar declarations in a `for` initializer directly to
 * their frame slots.  Unlike a declaration in a compound, the initializer
 * cannot be followed by a separately inserted statement. Persisted class
 * initializers have already been hoisted into a surrounding compound. */
static void RewritePersistedCoroutineForInitializer(
    ForStatementASTNode* loop,
    CoroutineFrame* frame,
    Vector* persisted_locals) {
  if (loop == NULL || loop->c1 == NULL ||
      loop->c1->op != AST_OP(decl_list)) {
    return;
  }
  DeclarationListASTNode* decls = (DeclarationListASTNode*)loop->c1;
  for (size_t i = 0; decls->declarations != NULL &&
                     i < decls->declarations->length; i++) {
    VariableDeclarationASTNode* decl = decls->declarations->value.p[i];
    CoroutinePersistedLocal* local = FindPersistedCoroutineLocal(
        persisted_locals, decl != NULL ? decl->symbol : NULL);
    if (local == NULL || local->member == NULL || local->is_parameter ||
        local->is_catch_parameter) {
      continue;
    }
    ASTNode* trailing = NewPersistedCoroutineLocalFrameInitialization(
        decl, local, frame, true);
    /* Only scalar declarations remain in the header, so no separate
     * constructed-flag statement is expected. */
    if (trailing != NULL) {
      SemanticError(
          (ASTNode*)decl,
          "coroutine class local in for initializer cannot cross suspension");
      ASTNodeDelete(trailing);
    }
  }
}

/* Recurse into a statement's sub-statements to insert direct frame-local
 * initializations within nested blocks. */
static void InsertPersistedCoroutineLocalStoresInStatementChildren(
    ASTNode* stmt,
    CoroutineFrame* frame,
    Vector* persisted_locals,
    SuspensionPoints* points) {
  if (stmt != NULL && stmt->op == AST_OP(for)) {
    RewritePersistedCoroutineForInitializer(
        (ForStatementASTNode*)stmt, frame, persisted_locals);
  }
  CoroutineStatementChildCollector collector = {
      .root = stmt,
  };
  VectorInit(&collector.children);
  ASTNodeVisitAndTransform(stmt, CollectNestedStatementChildTransform,
                           &collector);
  for (size_t i = 0; i < collector.children.length; i++) {
    ASTNode* child = collector.children.value.p[i];
    if (child == NULL) {
      continue;
    }
    if (child->op == AST_OP(compound)) {
      InsertPersistedCoroutineLocalStoresInCompound(
          (CompoundStatementASTNode*)child, frame, persisted_locals, points);
    } else {
      InsertPersistedCoroutineLocalStoresInStatementChildren(
          child, frame, persisted_locals, points);
    }
  }
  VectorDestruct(&collector.children);
}

/* After each declaration of a persisted (non-parameter) local, move its
 * initializer to the corresponding frame slot.  Scope-exit destructor calls
 * are replaced with guarded frame-member destruction so teardown remains
 * idempotent.  Suspension-point indices are kept in sync as statements are
 * inserted, and nested blocks are processed recursively. */
static void InsertPersistedCoroutineLocalStoresInCompound(
    CompoundStatementASTNode* compound,
    CoroutineFrame* frame,
    Vector* persisted_locals,
    SuspensionPoints* points) {
  if (compound == NULL || frame == NULL || persisted_locals == NULL ||
      persisted_locals->length == 0) {
    return;
  }
  Vector range_temporaries;
  VectorInit(&range_temporaries);
  if ((compound->base.flags & kASTRangeForInitializer) != 0 &&
      compound->statements != NULL && compound->statements->length != 0) {
    ASTNode* range_decl = compound->statements->value.p[0];
    CXXCollectRangeForInitializerTemporaries(range_decl, &range_temporaries);
    size_t inserted = 0;
    for (size_t i = range_temporaries.length; i > 0; i--) {
      Symbol* symbol = range_temporaries.value.p[i - 1];
      CoroutinePersistedLocal* local =
          FindPersistedCoroutineLocal(persisted_locals, symbol);
      if (local == NULL || local->member == NULL) {
        continue;
      }
      PersistedLocalInitializerRewrite rewrite = {
          .symbol = symbol,
          .frame = frame,
          .member = local->member,
      };
      range_decl = ASTNodeVisitAndTransform(
          range_decl, RewritePersistedLocalInitializerTransform, &rewrite);
      compound->statements->value.p[0] = range_decl;
      if (local->constructed_member != NULL) {
        size_t insert_index = 1 + inserted;
        CompoundASTNodeInsertStatement(
            compound,
            NewFrameIntAssignment(frame, local->constructed_member, 1,
                                  range_decl->location),
            insert_index);
        AdjustSuspensionPointIndicesAfterInsert(points, compound, insert_index);
        inserted++;
      }
    }
  }
  for (size_t i = 0; i < compound->statements->length; i++) {
    ASTNode* stmt = compound->statements->value.p[i];
    CoroutinePersistedLocal* destroyed =
        PersistedLocalDestroyedByStatement(stmt, persisted_locals);
    if (destroyed != NULL) {
      ASTNode* replacement = NewCoroutineFrameMemberGuardedDestructor(
          frame, destroyed->member, destroyed->constructed_member,
          stmt->location);
      if (replacement != NULL) {
        ASTNodeReplaceChild((ASTNode*)compound, (int)i, replacement, true);
      }
      continue;
    }
    if (stmt != NULL && stmt->op == AST_OP(compound)) {
      InsertPersistedCoroutineLocalStoresInCompound(
          (CompoundStatementASTNode*)stmt, frame, persisted_locals, points);
      continue;
    }
    if (stmt == NULL || stmt->op != AST_OP(decl_list)) {
      InsertPersistedCoroutineLocalStoresInStatementChildren(
          stmt, frame, persisted_locals, points);
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
      if (local == NULL || local->member == NULL || local->is_parameter ||
          local->is_catch_parameter) {
        continue;
      }
      size_t insert_index = i + 1 + inserted;
      ASTNode* initialization =
          NewPersistedCoroutineLocalFrameInitialization(
              decl, local, frame,
              CoroutineSuspensionUsesValueDeclaration(points, decl));
      if (initialization == NULL) {
        continue;
      }
      CompoundASTNodeInsertStatement(compound, initialization, insert_index);
      AdjustSuspensionPointIndicesAfterInsert(points, compound, insert_index);
      inserted++;
    }
    i += inserted;
  }
  for (size_t i = 0; i < range_temporaries.length; i++) {
    CoroutinePersistedLocal* local = FindPersistedCoroutineLocal(
        persisted_locals, range_temporaries.value.p[i]);
    if (local == NULL || local->member == NULL ||
        local->constructed_member == NULL) {
      continue;
    }
    ASTNode* destructor = NewCoroutineFrameMemberGuardedDestructor(
        frame, local->member, local->constructed_member,
        compound->base.location);
    if (destructor != NULL) {
      VectorAppend(compound->statements, destructor);
      destructor->parent = (ASTNode*)compound;
      destructor->child_id = (int)compound->statements->length - 1;
    }
  }
  VectorDestruct(&range_temporaries);
}

/* Entry point: redirect persisted-local initialization into the frame. */
static void InsertPersistedCoroutineLocalStores(
    CompoundStatementASTNode* body,
    CoroutineFrame* frame,
    Vector* persisted_locals,
    SuspensionPoints* points) {
  HoistPersistedCoroutineForInitializers(body, frame, persisted_locals);
  InsertPersistedCoroutineLocalStoresInCompound(body, frame, persisted_locals,
                                               points);
}

static ASTNode* NewPersistedCoroutineTransferValue(
    CoroutinePersistedLocal* local) {
  if (local == NULL || local->symbol == NULL) {
    return NULL;
  }
  ASTNode* value =
      local->move_parameter
          ? NewCoroutineMoveExpression(local->symbol, local->symbol->location)
          : NewIdentifierASTNode(local->symbol, local->symbol->location);
  if (TypeIsReference(local->symbol->type)) {
    value = NewUnaryASTNode(
        AST_OP(address), TypeRecordCopy(local->member->symbol->type),
        local->symbol->location, value);
  }
  return value;
}

/* For persisted catch-clause parameters, insert a frame store at the top of the
 * catch body (flagged as a frame store so it doesn't re-run in the resume fn). */
static void InsertPersistedCoroutineCatchParameterStores(
    CoroutineFrame* frame,
    Vector* persisted_locals,
    SuspensionPoints* points) {
  if (frame == NULL || persisted_locals == NULL ||
      persisted_locals->length == 0) {
    return;
  }
  for (size_t i = 0; i < persisted_locals->length; i++) {
    CoroutinePersistedLocal* local = persisted_locals->value.p[i];
    if (local == NULL || local->member == NULL ||
        !local->is_catch_parameter || local->store_compound == NULL) {
      continue;
    }
    ASTNode* value = NewPersistedCoroutineTransferValue(local);
    ASTNode* store = TypeIsStructOrUnion(local->symbol->type)
                         ? NewFrameMemberInitialization(
                               frame, local->member,
                               local->constructed_member, value,
                               local->symbol->location)
                         : NewFrameAssignment(frame, local->member, value,
                                              local->symbol->location);
    store->flags |= kASTCoroutineFrameStore;
    CompoundASTNodeInsertStatement(local->store_compound, store, 0);
    AdjustSuspensionPointIndicesAfterInsert(points, local->store_compound, 0);
  }
}

/* At the top of the ramp body, store each persisted parameter into its frame
 * slot (copying or moving as decided), flagged as a frame store so the resume
 * function strips it out. */
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
    ASTNode* value = NewPersistedCoroutineTransferValue(local);
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

/* Validate and prepare one suspension point. For co_await, this applies
 * await_transform and member/free operator co_await to obtain the real awaiter,
 * checks the awaiter satisfies the interface, and decides where the awaiter
 * object comes from: an existing local declaration, a fresh frame temporary, or
 * a moved copy. Records the awaiter, its initializer, and any awaitable
 * temporary back into `point`. Returns false on a semantic error. */
static bool ValidateSuspensionPoint(ASTNode* node, CompoundStatementASTNode* body,
                                    Symbol* promise,
                                    SuspensionPoint* point) {
  (void)body;
  if (point->kind == kSuspensionCoYield) {
    if (point->co_yield == NULL || point->statement == NULL) {
      SemanticError(node, "unsupported coroutine suspension form");
      return false;
    }
    return true;
  }

  bool is_statement_co_await =
      point->statement != NULL && point->value_decl == NULL;
  if (point->co_await == NULL ||
      (!is_statement_co_await &&
       (point->value_decl == NULL || point->decl_list == NULL))) {
    SemanticError(node, "unsupported coroutine suspension form");
    return false;
  }
  if (point->decl_list != NULL &&
      point->decl_list->declarations->length != 1) {
    SemanticError(point->co_await,
                  "suspending co_await declaration must be isolated");
    return false;
  }
  UnaryASTNode* co_await = (UnaryASTNode*)point->co_await;
  co_await->sub = AnalyzeExpression(co_await->sub);
  if (co_await->sub != NULL) {
    co_await->sub->parent = point->co_await;
    co_await->sub->child_id = 0;
  }
  Symbol* awaitable_temp = NULL;
  bool await_transform_error = false;
  ApplyPromiseAwaitTransform((UnaryASTNode*)point->co_await, promise,
                             &await_transform_error);
  if (await_transform_error) {
    return false;
  }
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
    point->awaiter = NewCoroutineAwaiterTemporary(awaiter->type,
                                                  point->co_await->location);
    if (point->awaiter == NULL) {
      SemanticError(point->co_await,
                    "coroutine co_await operand type is invalid");
      return false;
    }
    point->awaiter_init =
        NewCoroutineFrameAwaiterInitializer(awaiter,
                                            point->co_await->location);
  } else if (CoroutineFrameAwaiterShouldMove(awaiter->type)) {
    point->awaiter_init =
        NewCoroutineFrameAwaiterInitializer(awaiter,
                                            point->co_await->location);
  }
  return true;
}

/* Build the coroutine state machine in the (ramp) function body. Phases:
 *   1. Allocate a resume label per suspension point, lower each co_yield, and
 *      replace each co_await/co_yield expression with its await_resume() result
 *      (or redirect the declared variable's initializer to it).
 *   2. Walking points in reverse so indices stay valid, splice in for each
 *      point: the awaiter's frame initialization, the `if (!await_ready) {
 *      state=N; suspend... }` block, the resume label, a state reset, and the
 *      awaiter destructor after its use.
 *   3. At the body top, emit the resume dispatch `if (state==N) goto labelN;`
 *      chain, then wire up the frame's done flag, resume/destroy function
 *      pointers (generating those functions), and the initial-suspend handling.
 *   4. Remove the original (now-relocated) suspension statements and prepend the
 *      frame starter initializers.
 * Returns false on allocation failure. */
static bool LowerSuspendingCoAwaitFunction(ASTNode* node,
                                           CoroutineScan scan,
                                           SuspensionPoints* points,
                                           Symbol* promise,
                                           Symbol* return_object,
                                           CoroutineFrame* frame,
                                           Symbol* initial_awaiter,
                                           Symbol* final_awaiter,
                                           TypeRecord* yield_awaiter_type) {
  CompoundStatementASTNode* body =
      (CompoundStatementASTNode*)node->type->info.function.body;

  /* Phase 1: per-point resume labels + replace suspension expressions with their
   * await_resume() results. */
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
      if (points->points[i].value_decl != NULL) {
        ReplaceVariableInitializerExpression(
            points->points[i].value_decl,
            UnwrapExpressionInitializer(NewAwaitResumeInitializer(
                points->points[i].awaiter,
                points->points[i].value_decl->symbol->type, location)));
      } else if (!SuspensionPointIsStatementCoYield(&points->points[i])) {
        ASTNode* await_resume =
            NewAwaiterMemberCall(
                NewIdentifierASTNode(points->points[i].awaiter, location),
                "await_resume", location);
        ASTNodeSetType(await_resume, points->points[i].co_yield->type);
        ASTNodeReplaceChild(points->points[i].co_yield->parent,
                            points->points[i].co_yield->child_id,
                            await_resume, true);
      }
    } else if (points->points[i].value_decl == NULL) {
      ASTNode* await_resume =
          NewAwaiterMemberCall(NewIdentifierASTNode(points->points[i].awaiter,
                                                    location),
                               "await_resume", location);
      ASTNodeSetType(await_resume, points->points[i].co_await->type);
      ASTNodeReplaceChild(points->points[i].co_await->parent,
                          points->points[i].co_await->child_id,
                          await_resume, true);
    } else {
      ReplaceVariableInitializerExpression(
          points->points[i].value_decl,
          UnwrapExpressionInitializer(NewAwaitResumeInitializer(
              points->points[i].awaiter,
              points->points[i].value_decl->symbol->type, location)));
    }
  }

  /* Phase 2: splice the awaiter init, suspend block, resume label, and destructor
   * around each point (reverse order keeps earlier indices valid). */
  for (int i = points->count - 1; i >= 0; i--) {
    SuspensionPoint* point = &points->points[i];
    SourceLocation location = point->kind == kSuspensionCoYield
                                  ? point->co_yield->location
                                  : point->co_await->location;
    Symbol* awaiter = point->awaiter;
    ASTNode* suspend_if =
        NewSuspendIf(frame, awaiter, i + 1, return_object, location);
    ASTNode* reset_state =
        NewFrameIntAssignment(frame, frame->state, 0, location);
    if (point->kind == kSuspensionCoYield) {
      CoroutineCompoundInsertStatement(point->compound, suspend_if,
                                       point->statement_index + 1);
      CoroutineCompoundInsertStatement(point->compound, (ASTNode*)labels[i],
                                       point->statement_index + 2);
      CoroutineCompoundInsertStatement(point->compound, reset_state,
                                       point->statement_index + 3);
      if (SuspensionPointIsStatementCoYield(point)) {
        CoroutineCompoundInsertStatement(
            point->compound, NewAwaitResumeStatement(awaiter, location),
            point->statement_index + 4);
      }
      InsertCoroutineAwaiterDestructorAfterUse(point, frame,
                                               point->statement_index + 4,
                                               location);
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
      InsertCoroutineAwaiterDestructorAfterUse(point, frame, next_index + 3,
                                               location);
    }
  }

  /* Phase 3: prepend the resume dispatch (`if (state==N) goto labelN;`). */
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
      NewCoroutineResumeFunction(node, frame, body, scan.suspend_count, promise,
                                 initial_awaiter, final_awaiter,
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
    while (initial_index < body->statements->length) {
      ASTNode* statement = body->statements->value.p[initial_index];
      if (statement == NULL ||
          (statement->flags & kASTCoroutineFrameStore) == 0) {
        break;
      }
      initial_index++;
    }
    CompoundASTNodeInsertStatement(
        body,
        NewCoroutineReturnObjectDeclaration(promise, return_object,
                                            node->location),
        initial_index++);
    CompoundASTNodeInsertStatement(
        body, NewInitialSuspendInitialization(frame, promise, node->location),
        initial_index++);
    CompoundASTNodeInsertStatement(
        body, NewInitialSuspendIf(frame, initial_awaiter, return_object,
                                  node->location),
        initial_index++);
    CompoundASTNodeInsertStatement(
        body, NewAwaitResumeStatement(initial_awaiter, node->location),
        initial_index++);
    RewriteFrameOwnedSymbols((ASTNode*)body, frame);
  }
  /* Phase 4: drop the now-relocated original suspension statements (each sat at
   * index 1 after the dispatch chain was prepended) and add frame starters. */
  for (int i = 0; i < points->count; i++) {
    VectorDeleteElement(body->statements, 1);
  }
  InsertCoroutineFrameStarterInitializers(body, frame, node->location);
  ResetCompoundStatementParents(body);
  free(labels);
  return true;
}

/* Top-level coroutine lowering driver. Decides whether a heap frame is needed
 * (any real suspension at body/initial/final), allocates the promise, gathers
 * suspension points and the locals/parameters that must outlive them, validates
 * that all frame-owned objects can be constructed, builds the frame, registers
 * its owned objects, prepends frame/promise declarations and parameter stores,
 * lowers co_return/throw into promise calls + final suspend, and finally builds
 * the suspend/resume state machine via LowerSuspendingCoAwaitFunction (plus the
 * allocation-failure guard). For the trivial no-suspend case it just inserts the
 * initial_suspend call and co_return lowering. Returns false on error. */
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
  TypeRecord* final_awaiter_type =
      CoroutinePromiseMemberReturnType(info->coroutine_promise_type,
                                       "final_suspend");
  bool final_can_suspend = final_awaiter_type != NULL &&
                           !AwaiterTypeIsAlwaysReady(final_awaiter_type);
  bool needs_frame = scan.suspend_count > 0 || initial_can_suspend ||
                     final_can_suspend;
  TypeRecord* yield_awaiter_type =
      scan.has_co_yield
          ? CoroutinePromiseMemberReturnType(info->coroutine_promise_type,
                                             "yield_value")
          : NULL;
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)info->body;
  SuspensionPoints points = {0};
  Vector persisted_locals;
  VectorInit(&persisted_locals);
  Symbol* promise =
      SyntaxNewTemporary(&compiler->syntax,
                         TypeRecordCopy(info->coroutine_promise_type));
  promise->flags.is_local = true;
  promise->flags.is_defined = true;
  promise->location = node->location;
  Symbol* return_object =
      SyntaxNewTemporary(&compiler->syntax, TypeRecordCopy(node->type->next));
  return_object->flags.is_local = true;
  return_object->flags.is_defined = true;
  return_object->location = node->location;
  points.capacity = scan.suspend_count;
  if (needs_frame && points.capacity > 0) {
    ASTNodeVisit((ASTNode*)body, AnalyzeCoroutineRangeForDeclarations, 0,
                 NULL);
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
      if (!ValidateSuspensionPoint(node, body, promise, &points.points[i])) {
        PersistedCoroutineLocalsDestruct(&persisted_locals);
        free(points.points);
        return false;
      }
    }
    for (int i = 0; i < points.count; i++) {
      if (points.points[i].kind == kSuspensionCoAwait &&
          !ValidateCoroutineSuspensionPointFrameAwaiterConstructible(
              &points.points[i])) {
        PersistedCoroutineLocalsDestruct(&persisted_locals);
        free(points.points);
        return false;
      }
    }
    if (!CollectPersistedCoroutineLocals(body, &points, &persisted_locals)) {
      PersistedCoroutineLocalsDestruct(&persisted_locals);
      free(points.points);
      return false;
    }
  }
  if (needs_frame &&
      !CollectPersistedCoroutineParameters(info, body, &points,
                                           &persisted_locals)) {
    PersistedCoroutineLocalsDestruct(&persisted_locals);
    free(points.points);
    return false;
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
  if (final_awaiter_type != NULL &&
      !ValidateCoAwaiterType(node, final_awaiter_type)) {
    PersistedCoroutineLocalsDestruct(&persisted_locals);
    free(points.points);
    return false;
  }

  VariableDeclarationASTNode* promise_decl = NULL;
  Symbol* initial_awaiter = NULL;
  VariableDeclarationASTNode* initial_awaiter_decl = NULL;
  Symbol* final_awaiter = NULL;
  CoroutineFrame frame = {0};
  if (!needs_frame) {
    promise_decl = (VariableDeclarationASTNode*)NewVariableDeclarationASTNode(
        promise, NULL, node->location);
  } else {
    frame = NewCoroutineFrame(info->coroutine_promise_type,
                              initial_awaiter_type, final_awaiter_type, &points,
                              &persisted_locals,
                              yield_awaiter_type,
                              node->location);
    /* Register parameter copies before the promise. Reverse teardown then
     * destroys active body locals first, followed by the promise and finally
     * parameter copies, matching coroutine-state destruction order. */
    for (size_t i = 0; i < persisted_locals.length; i++) {
      CoroutinePersistedLocal* local = persisted_locals.value.p[i];
      if (local != NULL && local->member != NULL && local->is_parameter) {
        CoroutineFrameAddOwnedSymbol(&frame, local->symbol, local->member,
                                     local->constructed_member, false, false);
      }
    }
    CoroutineFrameAddOwnedSymbol(&frame, promise, frame.promise,
                                 frame.promise_constructed, true, false);
    for (size_t i = 0; i < persisted_locals.length; i++) {
      CoroutinePersistedLocal* local = persisted_locals.value.p[i];
      if (local != NULL && local->member != NULL && !local->is_parameter) {
        CoroutineFrameAddOwnedSymbol(&frame, local->symbol, local->member,
                                     local->constructed_member, false, true);
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
                                   false, false);
    }
    if (frame.final_awaiter != NULL) {
      final_awaiter =
          SyntaxNewTemporary(&compiler->syntax,
                             TypeRecordCopy(final_awaiter_type));
      final_awaiter->flags.is_local = true;
      final_awaiter->flags.is_defined = true;
      final_awaiter->location = node->location;
      CoroutineFrameAddOwnedSymbol(&frame, final_awaiter,
                                   frame.final_awaiter,
                                   frame.final_awaiter_constructed, false,
                                   false);
    }
    for (int i = 0; i < points.count; i++) {
      if (points.points[i].kind == kSuspensionCoAwait) {
        CoroutineFrameAddOwnedSymbol(
            &frame, points.points[i].awaiter,
            points.points[i].frame_member,
            points.points[i].frame_constructed_member, false, true);
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
    InsertPersistedCoroutineCatchParameterStores(&frame, &persisted_locals,
                                                 &points);
    InsertPersistedCoroutineLocalStores(body, &frame, &persisted_locals,
                                        &points);
    InsertPersistedCoroutineJumpDestructors(
        (ASTNode*)body, &frame, &persisted_locals);
  }
  if (!needs_frame) {
    CompoundASTNodeInsertStatement(
        body,
        NewCoroutineReturnObjectDeclaration(promise, return_object,
                                            node->location),
        1);
    CompoundASTNodeInsertStatement(
        body,
        NewExpressionStatementASTNode(
            NewCoroutinePromiseMemberCall(promise, "initial_suspend",
                                          NewVector(), node->location),
            node->location),
        2);
  }
  /* Flowing off the end of a coroutine is equivalent to `co_return;`.  Add the
   * implicit statement before the co_return lowering pass so return_void(),
   * frame completion, and final_suspend all run on the fallthrough path. */
  if (!scan.has_co_return_value) {
    VectorAppend(body->statements,
                 NewCombinedStatementASTNode(AST_OP(co_return), NULL, NULL,
                                             node->location));
    ResetCompoundStatementParents(body);
  }
  LowerCoReturnsInStatement(info->body, promise, needs_frame ? &frame : NULL,
                            &persisted_locals, final_awaiter,
                            return_object);
  /* A suspending coroutine's generated resume function has a catch-all that
   * invokes unhandled_exception() while the active exception is established.
   * Keep explicit throws intact so that handler performs the standard
   * propagation path.  The no-frame path has no generated resume wrapper and
   * still needs direct lowering. */
  if (!needs_frame) {
    LowerCoroutineThrowsInStatement(info->body, promise, NULL, final_awaiter,
                                    return_object);
  }
  if (needs_frame &&
      !LowerSuspendingCoAwaitFunction(node, scan, &points, promise,
                                      return_object, &frame, initial_awaiter,
                                      final_awaiter,
                                      yield_awaiter_type)) {
    CoroutineFrameOwnedSymbolsDestruct(&frame);
    PersistedCoroutineLocalsDestruct(&persisted_locals);
    free(points.points);
    return false;
  }
  if (needs_frame) {
    StructMember* allocation_failure =
        FindCoroutineAllocationFailureMember(info->coroutine_promise_type);
    if (allocation_failure != NULL) {
      CompoundASTNodeInsertStatement(
          body,
          NewCoroutineAllocationFailureIf(&frame, allocation_failure,
                                          node->type->next, node->location),
          1);
    }
  }
  if (needs_frame) {
    CoroutineFrameOwnedSymbolsDestruct(&frame);
  }
  PersistedCoroutineLocalsDestruct(&persisted_locals);
  free(points.points);
  return true;
}

/* Scan a function body for coroutine keywords; if it is a coroutine, mark the
 * FunctionInfo and reject illegal coroutine forms (constexpr/consteval, ctor/
 * dtor, main, varargs, deduced return type). Returns the scan result. */
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

/* Resolve the promise type and verify it provides all required members for this
 * coroutine: get_return_object, valid initial/final_suspend awaiters,
 * yield_value (if co_yield is used), return_value/return_void (matching the
 * co_return form), and unhandled_exception. Returns true only if all present. */
static bool ValidateCoroutinePromise(ASTNode* node, CoroutineScan scan) {
  if (!scan.is_coroutine || node == NULL || node->type == NULL ||
      !TypeIsFunction(node->type)) {
    return false;
  }
  FunctionInfo* info = &node->type->info.function;
  if (info->coroutine_promise_type == NULL) {
    info->coroutine_promise_type = ResolveCoroutinePromiseType(node->type);
  }
  if (info->coroutine_promise_type == NULL) {
    SemanticError(node, "coroutine return type must provide promise_type");
    return false;
  }

  TypeRecord* promise = info->coroutine_promise_type;
  bool ok = true;
  ok &= RequireCoroutinePromiseMember(node, promise, "get_return_object");
  if (RequireCoroutinePromiseMember(node, promise, "initial_suspend")) {
    ok &= ValidateCoroutinePromiseAwaiterReturn(node, promise,
                                                "initial_suspend");
  } else {
    ok = false;
  }
  if (RequireCoroutinePromiseMember(node, promise, "final_suspend")) {
    ok &= ValidateCoroutinePromiseAwaiterReturn(node, promise,
                                                "final_suspend");
  } else {
    ok = false;
  }
  if (scan.has_co_yield) {
    if (RequireCoroutinePromiseMember(node, promise, "yield_value")) {
      ok &= ValidateCoroutinePromiseAwaiterReturn(node, promise, "yield_value");
    } else {
      ok = false;
    }
  }
  if (scan.has_co_return_value) {
    ok &= RequireCoroutinePromiseMember(node, promise, "return_value");
  } else {
    ok &= RequireCoroutinePromiseMember(node, promise, "return_void");
  }
  ok &= RequireCoroutinePromiseMember(node, promise, "unhandled_exception");
  return ok;
}

/* Error-recovery transform: when the promise is invalid, replace co_await/
 * co_yield expressions with `0` so analysis can continue without crashing on a
 * malformed coroutine. */
static ASTNode* RecoverInvalidCoroutineExpression(ASTNode* node, void* data,
                                                  ASTNodeTransformAction* action) {
  (void)data;
  if (node == NULL) {
    return NULL;
  }
  if (node->op == AST_OP(co_await) || node->op == AST_OP(co_yield)) {
    *action = kASTTransformSkipChildren;
    return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt,
                                                          kQualPlain),
                                 node->location);
  }
  return node;
}

/* Public entry point: analyze and lower a (possibly) coroutine function. Detects
 * and validates the coroutine, validates the promise type, lowers always-ready
 * co_awaits, normalizes nested suspensions to statement granularity, re-scans to
 * recount suspensions, and runs the full frame/state-machine lowering. On an
 * invalid promise it performs error recovery instead. */
void SemanticAnalyzeCoroutineFunction(ASTNode* node) {
  if (node == NULL || node->type == NULL || !TypeIsFunction(node->type) ||
      node->type->info.function.body == NULL ||
      (!node->type->info.function.has_coroutine_syntax &&
       !node->type->info.function.is_coroutine)) {
    return;
  }
  CoroutineScan coroutine_scan = MarkAndValidateCoroutineFunction(node);
  if (coroutine_scan.is_coroutine && node != NULL && node->type != NULL &&
      TypeIsFunction(node->type) &&
      (node->type->info.function.is_constexpr ||
       node->type->info.function.is_consteval)) {
    // The declaration is already ill-formed. Keep the coroutine marker so
    // co_return/co_await/co_yield receive normal body analysis, but do not
    // cascade into promise validation or state-machine lowering.
    return;
  }
  bool promise_ok = ValidateCoroutinePromise(node, coroutine_scan);
  CoroutineScan lowering_scan = coroutine_scan;
  if (!promise_ok && coroutine_scan.is_coroutine && node != NULL &&
      node->type != NULL && TypeIsFunction(node->type) &&
      node->type->info.function.body != NULL) {
    node->type->info.function.body = ASTNodeVisitAndTransform(
        node->type->info.function.body, RecoverInvalidCoroutineExpression,
        NULL);
  }
  if (promise_ok && coroutine_scan.is_coroutine && node != NULL &&
      node->type != NULL &&
      TypeIsFunction(node->type) && node->type->info.function.body != NULL) {
    ASTNodeVisit(node->type->info.function.body, LowerReadyCoAwaitNode, 0,
                 NULL);
    if (node->type->info.function.body->op == AST_OP(compound)) {
      NormalizeNestedSuspensionsInCompound(
          (CompoundStatementASTNode*)node->type->info.function.body);
    }
    lowering_scan = (CoroutineScan){0};
    ASTNodeVisit(node->type->info.function.body, ScanCoroutineNode, 0,
                 &lowering_scan);
    node->type->info.function.coroutine_suspend_count =
        lowering_scan.suspend_count;
  }
  LowerCoroutineFunction(node, lowering_scan);
}
