//
//  expr_semantics_inline.c
//  c_compiler
//
//  Function-call inlining and caller contract lowering.
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

// State used while cloning a callee body into its caller.
typedef struct {
  Map argument_map;     // Map of callee symbols to caller-local clones.
  ASTNode* end_label;   // End label for return conversion.
  Symbol* return_value; // Return value symbol.
  ASTNode* top_stmt;    // Top level compound statement.
  bool return_is_reference;
} Inliner;

static void AddInlineLocalSymbol(Inliner* inliner, Symbol* symbol);
static void CollectInlineLocalSymbols(ASTNode* node, void* data, int child_id, VisitorMode mode);
static ASTNode* InlineFunctionBodyStatement(ASTNode* node, void* data);
static ASTNode* CopyArguments(FunctionInfo* info, VectorASTNode* call, Inliner* inliner);
static Symbol* CallThisReceiverSymbol(VectorASTNode* call);
static Symbol* InlineMappedSymbol(Inliner* inliner, Symbol* original);
static ASTNode* NewCallerContractStatement(ContractAssertion* assertion, Inliner* inliner, VectorASTNode* call, TypeRecord* caller_func);
static void AppendCallerContractStatements(Vector* statements, TypeRecord* func, ContractAssertionKind kind, Inliner* inliner, VectorASTNode* call);
static bool VirtualCallerContractsRequireNontrivialCopy(TypeRecord* func);
static void ExamineBody(ASTNode* node, void* data, int child_id, VisitorMode mode);
static void FindNRVOLocal(ASTNode* node, void* data, int child_id, VisitorMode mode);
static void FindUnresolvedMemberAccess(ASTNode* node, void* data, int child_id, VisitorMode mode);
static bool FunctionTypeIsBeingAnalyzed(TypeRecord* func_type);
static bool InliningBlockedByCalleeOrCompilerFlags(FunctionInfo* func);
static bool InliningBlockedByCallerTemplateParameters(void);
static bool EnsureInlinedCalleeBodyIsAnalyzed(FunctionInfo* func);
static bool InliningBlockedByCalleeContractRules(FunctionInfo* func);
static bool InliningBlockedByLazyTemplateBodySharing(FunctionInfo* func);
static bool InliningBlockedByVirtualBaseMemberOwner(FunctionInfo* func);
static bool InliningBlockedByDefinitionAndLinkage(
    FunctionInfo* func, bool* force_inline_out, bool* unmarked_small_out);
static bool InliningBlockedByCalleeFormals(FunctionInfo* func);
static bool CalleeBodyEligibleByStructureAndSize(FunctionInfo* func,
                                                 bool force_inline,
                                                 bool unmarked_small);

// Contract: Clones local symbols into `inliner->argument_map` and global local list.
static void AddInlineLocalSymbol(Inliner* inliner, Symbol* symbol) {
  if (symbol == NULL || !symbol->flags.is_local ||
      StorageIs(symbol->storage, STO(static))) {
    return;
  }
  MapKeyType key = {.p = symbol};
  if (MapFind(&inliner->argument_map, key) != NULL) {
    return;
  }
  Symbol* clone = SymbolClone(symbol);
  clone->is_nrvo = false;
  VectorAppend(&compiler->syntax.all_local_symbols, clone);
  MapKeyValue kv = {.key.p = symbol, .value.p = clone};
  MapInsert(&inliner->argument_map, kv);
}

// Contract: Visitor; registers locals/catches/temps needing remap during clone.
static void CollectInlineLocalSymbols(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  Inliner* inliner = data;
  if (node->op == AST_OP(vardecl)) {
    AddInlineLocalSymbol(
        inliner, ((VariableDeclarationASTNode*)node)->symbol);
  } else if (node->op == AST_OP(identifier)) {
    // Lowered expressions can contain compiler-generated locals without a
    // VariableDeclarationASTNode (for example the temporary that carries a
    // placement-new result). They still need a caller-local clone when the
    // containing function is inlined.
    AddInlineLocalSymbol(inliner, ((IdentifierASTNode*)node)->symbol);
  } else if (node->op == AST_OP(catch)) {
    AddInlineLocalSymbol(inliner, ((CatchASTNode*)node)->symbol);
  }
}

// This is called while cloning the function body for inlining.  The
// data is a pointer to an Inliner.  The node is a cloned node.
// There are two conversion that need to happen:
// 1. A reference to an identifier that is a formal argument of the function
//    being inlined needs to refer to a new symbol that has been assigned
//    the actual value.
// 2. A return statement needs to be converted to a goto, after first
//    assigning the return value (if any) to the temporary symbol allocated
//    to hold the result.
//
// In addiiton, a cloned switch statement no longer has any case label
// or default information.  We perform a new semantic analysis on that
// after the clone of that node.
//
// Returns either the node passed or a new node (in the case of return
// converted to assignment and goto).
// Contract: Clone callback; may replace returns with compound+goto, remap symbols,
// and re-analyze synthesized statements/switches.
static ASTNode* InlineFunctionBodyStatement(ASTNode* node, void* data) {
  Inliner* inliner = data;
  // For an identifier
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id_node = (IdentifierASTNode*)node;
    MapKeyType key;
    key.p = id_node->symbol;
    void* new_sym = MapFind(&inliner->argument_map, key);
    if (new_sym != NULL) {
      id_node->symbol = new_sym;
      id_node->base.flags &= ~kASTNrvoMarker;
    }
    return node;
  }
  if (node->op == AST_OP(vardecl)) {
    VariableDeclarationASTNode* declaration =
        (VariableDeclarationASTNode*)node;
    MapKeyType key = {.p = declaration->symbol};
    Symbol* new_sym = MapFind(&inliner->argument_map, key);
    if (new_sym != NULL) {
      declaration->symbol = new_sym;
    }
    return node;
  }
  if (node->op == AST_OP(catch)) {
    CatchASTNode* catch_node = (CatchASTNode*)node;
    MapKeyType key = {.p = catch_node->symbol};
    Symbol* new_sym = MapFind(&inliner->argument_map, key);
    if (new_sym != NULL) {
      catch_node->symbol = new_sym;
    }
    return node;
  }
  if (node->op == AST_OP(return)) {
    CombinedStatementASTNode* ret_node = (CombinedStatementASTNode*)node;
    SourceLocation return_location = ret_node->base.location;
    Vector* new_ret = NewVector();
    if (ret_node->cond != NULL) {
      ASTNode* value = ASTNodeMove(ret_node->cond);
      // The cloned expression is no longer a return from the original
      // function: it initializes the inliner's local result temporary. An RVO
      // marker copied from the original return would incorrectly route a
      // nested aggregate-returning call to the caller's hidden struct-return
      // slot, which may not exist (for example when inlining into main).
      value->flags &= ~kASTRvoCall;
      ASTNode* statement;
      if (inliner->return_value != NULL) {
        if (inliner->return_is_reference) {
          value = NewUnaryASTNode(AST_OP(address), inliner->return_value->type,
                                  value->location, value);
        }
        ASTNode* ret_value =
            NewIdentifierASTNode(inliner->return_value, return_location);
        ASTNode* ret_assign = NewBinaryASTNode(AST_OP(assign),
                                               value->type,
                                               value->location,
                                               ret_value,
                                               value);
        statement =
            NewExpressionStatementASTNode(ret_assign, ret_assign->location);
      } else {
        // `return expression;` in a function returning void -- which a wrapper
        // forwarding to another void call is written as -- has no result to
        // store, but the expression is still evaluated for its side effects.
        statement = NewExpressionStatementASTNode(value, value->location);
      }
      AnalyzeStatement(statement);
      VectorAppend(new_ret, statement);
    }
    // Now make a goto node to the end_label.
    LabelASTNode* end_label = (LabelASTNode*)inliner->end_label;
    GotoStatementASTNode* goto_node = (GotoStatementASTNode*)
                    NewGotoStatementASTNode(
                                  NewString(end_label->name.value),
                                  return_location);
    goto_node->label = inliner->end_label;
    goto_node->lca = inliner->top_stmt;
    VectorAppend(new_ret, goto_node);

    // Don't need the return now.
    ASTNodeDelete(node);

    // Build a new Compound statement containing the assignment to the
    // return value (if necessary) and the goto.
    return NewCompoundStatementASTNode(new_ret, return_location);
  }

  // After cloning a switch statement we have lost the analysis of
  // case and default labels.  Need to do it again now.
  if (node->op == AST_OP(switch)) {
    AnalyzeStatement(node);
  }

  return node;
}

// Build declaration list for all formal args and initialize them with
// the actual args.  Since these are no longer arguments we need
// to copy the Symbols too.  The ownership of all actual expressions is
// changed to the variable declaration.  The inliner's argument_map
// will contains a mapping of old symbol to new symbol.
// Contract: Moves call actuals into param decls; fills `argument_map`; may allocate
// return temporary symbol.
static ASTNode* CopyArguments(FunctionInfo* info, VectorASTNode* call,
                              Inliner* inliner) {
  SourceLocation location = call->base.location;
  Vector* decls = NewVector();
  for (size_t i = 0; i < info->prototype.length; i++) {
    Symbol* original_formal = info->prototype.value.p[i];
    ASTNode* actual = ASTNodeMove(call->children->value.p[i]);
    Symbol* formal = SymbolClone(original_formal);
    formal->flags.is_argument = false;
    formal->flags.is_local = true;
    VectorAppend(&compiler->syntax.all_local_symbols, formal);

    // Insert old and new into inliner's argument map so we can translate
    // the argument references to the local variables.
    MapKeyValue kv;
    kv.key.p = original_formal;
    kv.value.p = formal;
    MapInsert(&inliner->argument_map, kv);

    // Initialize the local parameter object directly. Assignment would be
    // ill-formed for const parameters, would assign through references instead
    // of binding them, and would give class parameters the wrong copy semantics.
    ASTNode* formal_id = NewIdentifierASTNode(formal, location);
    formal_id->flags |= kASTNeedAddress | kASTIsDeclaration;
    ASTNode* initializer = NewBinaryASTNode(
        AST_OP(init), formal->type, actual->location, formal_id,
        NewExpressionInitializerASTNode(actual, actual->location));
    initializer = AnalyzeExpression(initializer);
    VectorAppend(
        decls,
        NewVariableDeclarationASTNode(formal, initializer, location));
  }

  // Allocate a temporary for the return value if it's not void.
  if (!TypeIsVoid(call->base.type)) {
    TypeRecord* temp_type =
        inliner->return_is_reference
            ? NewPointerTo(kQualPlain, call->base.type)
            : call->base.type;
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, temp_type);
    inliner->return_value = temp;
    VectorAppend(decls, NewVariableDeclarationASTNode(temp, NULL, location));
  } else {
    inliner->return_value = NULL;
  }
  return NewDeclarationListASTNode(decls, location);
}

// Contract: Read-only peel to `this`/object identifier for ctor/dtor inlining.
static Symbol* CallThisReceiverSymbol(VectorASTNode* call) {
  if (call == NULL || call->children == NULL || call->children->length == 0) {
    return NULL;
  }
  ASTNode* receiver = call->children->value.p[0];
  while (receiver != NULL) {
    if (receiver->op == AST_OP(address) || receiver->op == AST_OP(contents)) {
      receiver = ((UnaryASTNode*)receiver)->sub;
      continue;
    }
    if (receiver->op == AST_OP(subscript)) {
      receiver = ((BinaryASTNode*)receiver)->left;
      continue;
    }
    if (receiver->op == AST_OP(cast)) {
      receiver = ((CastASTNode*)receiver)->expr;
      continue;
    }
    break;
  }
  if (receiver != NULL && receiver->op == AST_OP(identifier)) {
    return ((IdentifierASTNode*)receiver)->symbol;
  }
  return NULL;
}

// Contract: Read-only shape check for mem-init prvalue elision (comma/inline ctor).
// A constructor mem-initializer call whose argument is a materialized prvalue
// `(ctor(&tmp, ...), tmp)` must stay a call so that
// ElideCXXMemberInitializerPrvalue can construct the member in place.
bool CXXMemberInitializerMayElidePrvalue(VectorASTNode* call) {
  if ((call->base.flags & kASTCXXMemberInitializer) == 0 ||
      call->children == NULL || call->children->length < 2 ||
      !TypeIsFunction(call->left->type) ||
      !call->left->type->info.function.is_constructor) {
    return false;
  }
  ASTNode* actual = call->children->value.p[call->children->length - 1];
  if (actual == NULL || actual->op != AST_OP(comma)) {
    return false;
  }
  ASTNode* inner = ((BinaryASTNode*)actual)->left;
  if (ASTIsInlinedConstructor(inner)) {
    return true;
  }
  if (inner == NULL || inner->op != AST_OP(call)) {
    return false;
  }
  ASTNode* callee = ((VectorASTNode*)inner)->left;
  return callee != NULL && callee->type != NULL &&
         TypeIsFunction(callee->type) &&
         callee->type->info.function.is_constructor;
}

// A copy/move constructor whose source is a materialized prvalue of the same
// class, `T(&dst, (T){prvalue})`, is elided by the backend
// (CXXElidableStructReturnInitializer), which needs to see the call.
// Contract: Read-only IR elision hint from compound-literal ctor actual shape.
bool CXXConstructorCallElidesPrvalueSource(VectorASTNode* call) {
  if (!CompilerIsCXX() || call->children == NULL ||
      !TypeIsFunction(call->left->type) ||
      !call->left->type->info.function.is_constructor) {
    return false;
  }
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual == NULL || actual->op != AST_OP(compound_literal)) {
      continue;
    }
    ASTNode* init = ((CompoundLiteralASTNode*)actual)->initializer;
    if (init == NULL || init->op != AST_OP(braced_init) ||
        ((BracedInitializerASTNode*)init)->initializers->length != 1) {
      continue;
    }
    ASTNode* only = ((BracedInitializerASTNode*)init)->initializers->value.p[0];
    if (only == NULL || only->op != AST_OP(designated_init)) {
      continue;
    }
    ASTNode* source = ((DesignatedInitializerASTNode*)only)->init;
    if (source != NULL && source->op == AST_OP(expr_init)) {
      source = ((ExpressionInitializerASTNode*)source)->expr;
    }
    if (source != NULL && source->type != NULL &&
        TypeIsStructOrUnion(source->type) &&
        TypeEqual(source->type, actual->type)) {
      return true;
    }
  }
  return false;
}

// Contract: Builds a new `inline_call` subtree owning cloned callee body;
// moves actuals into param init decls (`CopyArguments`). Temporarily sets
// `current_function` to the callee for access checks during clone callback.
// Caller replaces `call` with the returned node.
// Inline a function call.
// 1. Create new symbols for all formal args.
// 2. Assign all actual values to new symbols.
// 3. Allocate (but don't emit) end_label.
// 4. Clone body, replacing all returns by a goto to end_label and references
//    to the arguments with references to the new symbols.
// 5. Emit end_label.
ASTNode* InlineFunctionCall(FunctionInfo* info, VectorASTNode* call) {
  Vector* statements = NewVector();
  SourceLocation location = call->base.location;
  Inliner inliner;
  MapInitForPointerKeys(&inliner.argument_map);
  inliner.return_is_reference =
      info->symbol != NULL && info->symbol->type != NULL &&
      TypeIsReference(info->symbol->type->next);
  Symbol* cxx_receiver =
      (info->is_constructor || info->is_destructor) ? CallThisReceiverSymbol(call)
                                                    : NULL;

  VectorAppend(statements,
               CopyArguments(info, call, &inliner));
  ASTNode* inlined = NewCompoundStatementASTNode(statements, info->body->location);
  inliner.top_stmt = inlined;

  inliner.end_label = NewLabelASTNode(SyntaxFakeName(&compiler->syntax),
                                      NULL,
                                      false,
                                      location);

  // Clone function body replacing:
  // 1. Variable references to arguments with new symbols.
  // 2. return statements with goto statements to end_label.
  // Replacement statements analyzed by the clone callback are semantically
  // part of the inlined function's definition. Without this context, member
  // and friend access checks run as the caller and wrongly reject private or
  // protected members used by the inlined body.
  TypeRecord* saved_function = compiler->current_function;
  Struct* saved_access_context = compiler->current_class_access_context;
  compiler->current_function = info->symbol->type;
  compiler->current_class_access_context =
      info->symbol->type->info.function.cxx_member_owner;
  ASTNodeVisit(info->body, CollectInlineLocalSymbols, 0, &inliner);
  ASTNode* new_body = ASTNodeClone(info->body,
                                   InlineFunctionBodyStatement,
                                   &inliner, NULL);
  VectorAppend(statements, new_body);
  VectorAppend(statements, inliner.end_label);

  // Attach the new statements to the compound statement.
  for (size_t i = 0; i < statements->length; i++) {
    ASTNode* stmt = statements->value.p[i];
    stmt->parent = inlined;
    stmt->child_id = (int)i;
  }
  compiler->current_function = saved_function;
  compiler->current_class_access_context = saved_access_context;
  ASTNode* ret_node = NULL;
  if (inliner.return_value != NULL) {
    // Void function, no return value;
    ret_node = NewIdentifierASTNode(inliner.return_value, location);
  }
  MapDestruct(&inliner.argument_map);
  ASTNode* result =
      NewInlineCallASTNode(call->base.type, location, inlined, ret_node);
  result->value_category = call->base.value_category;
  if (info->is_constructor) {
    result->flags |= kASTInlinedConstructor;
  }
  if (info->is_destructor) {
    result->flags |= kASTInlinedDestructor;
  }
  ((InlineCallASTNode*)result)->cxx_receiver = cxx_receiver;
  return result;
}

// Contract: Looks up callee formal in inliner's map; read-only.
static Symbol* InlineMappedSymbol(Inliner* inliner, Symbol* original) {
  if (inliner == NULL || original == NULL) {
    return NULL;
  }
  MapKeyType key = {.p = original};
  return MapFind(&inliner->argument_map, key);
}

// Contract: Builds analyzed contract assert with cloned/mapped predicate; caller owns.
static ASTNode* NewCallerContractStatement(ContractAssertion* assertion,
                                           Inliner* inliner,
                                           VectorASTNode* call,
                                           TypeRecord* caller_func) {
  if (assertion == NULL || assertion->predicate == NULL) {
    return NULL;
  }
  ASTNode* predicate =
      ASTNodeClone(assertion->predicate, InlineFunctionBodyStatement,
                   inliner, NULL);
  FunctionInfo* info = &caller_func->info.function;
  bool has_runtime_dedup_guard = false;
  if (!info->is_pure_virtual && info->symbol != NULL &&
      call->left != NULL) {
    ASTNode* selected =
        ASTNodeClone(call->left, IdentityCloneNode, NULL, NULL);
    ASTNode* statically_chosen =
        AnalyzeExpression(NewIdentifierASTNode(info->symbol,
                                               assertion->location));
    ASTNode* same_function = NewBinaryASTNode(
        AST_OP(equal), NULL, assertion->location, selected, statically_chosen);
    same_function = AnalyzeExpression(same_function);
    predicate = AnalyzeExpression(NewBinaryASTNode(
        AST_OP(logor), NULL, assertion->location, same_function, predicate));
    has_runtime_dedup_guard = true;
  }
  Vector attributes = {0};
  AttributeListClone(&attributes, &assertion->attributes);
  ASTNode* statement = NewContractAssertASTNode(
      predicate, &attributes, assertion->location);
  ((ContractAssertASTNode*)statement)->kind = assertion->kind;
  statement->flags |= kASTAnalyzed;
  if (has_runtime_dedup_guard) {
    statement->flags |= kASTVirtualCallerContract;
  }
  return statement;
}

// Contract: Appends pre/post caller contract statements in kind order.
static void AppendCallerContractStatements(Vector* statements,
                                           TypeRecord* func,
                                           ContractAssertionKind kind,
                                           Inliner* inliner,
                                           VectorASTNode* call) {
  Vector* assertions = &func->info.function.contract_assertions;
  for (size_t i = 0; i < assertions->length; i++) {
    ContractAssertion* assertion = assertions->value.p[i];
    if (assertion == NULL || assertion->kind != kind) {
      continue;
    }
    ASTNode* statement =
        NewCallerContractStatement(assertion, inliner, call, func);
    if (statement != NULL) {
      VectorAppend(statements, statement);
    }
  }
}

// Contract: Read-only; rejects virtual caller-contract lowering when by-value types
// are not trivially copyable.
static bool VirtualCallerContractsRequireNontrivialCopy(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return false;
  }
  TypeRecord* return_type = func->next;
  if (return_type != NULL && !TypeIsReference(return_type) &&
      TypeIsStructOrUnion(return_type) &&
      !CXXTypeIsTriviallyCopyable(return_type)) {
    return true;
  }
  Vector* prototype = &func->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    if (formal != NULL && !TypeIsReference(formal->type) &&
        TypeIsStructOrUnion(formal->type) &&
        !CXXTypeIsTriviallyCopyable(formal->type)) {
      return true;
    }
  }
  return false;
}

// Contract: Replaces `call` with an `inline_call` compound wrapping pre/post
// caller contracts; detaches from parent during build. Mutates `call` children
// to mapped formals and deletes `caller_contract_function` type record.
ASTNode* LowerVirtualCallerContracts(VectorASTNode* call) {
  TypeRecord* caller_func = call->caller_contract_function;
  if (caller_func == NULL || !TypeIsFunction(caller_func) ||
      caller_func->info.function.contract_assertions.length == 0 ||
      call->children == NULL ||
      call->children->length !=
          caller_func->info.function.prototype.length) {
    return (ASTNode*)call;
  }
  if (VirtualCallerContractsRequireNontrivialCopy(caller_func)) {
    SemanticError(
        (ASTNode*)call,
        "caller-facing virtual contracts with non-trivially-copyable "
        "by-value parameters or results are not supported");
    TypeRecordDelete(call->caller_contract_function);
    call->caller_contract_function = NULL;
    return (ASTNode*)call;
  }

  ASTNode* outer_parent = call->base.parent;
  int outer_child_id = call->base.child_id;
  if (outer_parent != NULL) {
    ASTNodeReplaceChild(outer_parent, outer_child_id, NULL, false);
    call->base.parent = NULL;
  }

  call->caller_contract_function = NULL;
  SourceLocation location = call->base.location;
  Inliner inliner = {0};
  MapInitForPointerKeys(&inliner.argument_map);
  inliner.return_is_reference = TypeIsReference(caller_func->next);

  Vector* statements = NewVector();
  VectorAppend(statements,
               CopyArguments(&caller_func->info.function, call, &inliner));

  Symbol* this_formal =
      caller_func->info.function.prototype.length != 0
          ? caller_func->info.function.prototype.value.p[0]
          : NULL;
  Symbol* local_this = InlineMappedSymbol(&inliner, this_formal);
  if (local_this != NULL) {
    ASTNode* receiver = NewIdentifierASTNode(local_this, location);
    receiver = AnalyzeExpression(receiver);
    ASTNode* virtual_callee = NewVirtualCalleeFromFunction(
        receiver, caller_func, true, location);
    if (virtual_callee != NULL) {
      ASTNodeDelete(call->left);
      call->left = virtual_callee;
      call->left->parent = &call->base;
      call->left->child_id = 0;
    }
  }

  for (size_t i = 0;
       i < caller_func->info.function.prototype.length; i++) {
    Symbol* formal =
        caller_func->info.function.prototype.value.p[i];
    Symbol* local = InlineMappedSymbol(&inliner, formal);
    ASTNode* actual = NewIdentifierASTNode(local, location);
    actual = AnalyzeExpression(actual);
    VectorSet(call->children, i, actual);
    actual->parent = &call->base;
    actual->child_id = (int)i;
  }

  AppendCallerContractStatements(statements, caller_func,
                                 kContractPrecondition, &inliner, call);

  ASTNode* call_statement = NULL;
  if (inliner.return_value == NULL) {
    call_statement = NewExpressionStatementASTNode((ASTNode*)call, location);
  } else {
    ASTNode* value = (ASTNode*)call;
    if (inliner.return_is_reference) {
      value = NewUnaryASTNode(AST_OP(address), inliner.return_value->type,
                              location, value);
    }
    ASTNode* assignment = NewBinaryASTNode(
        AST_OP(assign), value->type, location,
        NewIdentifierASTNode(inliner.return_value, location), value);
    assignment = AnalyzeExpression(assignment);
    call_statement = NewExpressionStatementASTNode(assignment, location);
  }
  VectorAppend(statements, call_statement);

  Vector* assertions = &caller_func->info.function.contract_assertions;
  for (size_t i = 0; i < assertions->length; i++) {
    ContractAssertion* assertion = assertions->value.p[i];
    if (assertion != NULL &&
        assertion->kind == kContractPostcondition &&
        assertion->result_binding != NULL &&
        inliner.return_value != NULL) {
      Symbol* mapped_result = inliner.return_value;
      if (inliner.return_is_reference) {
        mapped_result = SymbolClone(assertion->result_binding);
        mapped_result->flags.is_argument = false;
        mapped_result->flags.is_local = true;
        VectorAppend(&compiler->syntax.all_local_symbols, mapped_result);

        ASTNode* returned_pointer =
            NewIdentifierASTNode(inliner.return_value, location);
        ASTNode* returned_reference = NewUnaryASTNode(
            AST_OP(contents), call->base.type, location, returned_pointer);
        returned_reference = AnalyzeExpression(returned_reference);
        ASTNode* result_id =
            NewIdentifierASTNode(mapped_result, assertion->location);
        result_id->flags |= kASTNeedAddress | kASTIsDeclaration;
        ASTNode* initializer = NewBinaryASTNode(
            AST_OP(init), mapped_result->type, assertion->location, result_id,
            NewExpressionInitializerASTNode(returned_reference,
                                            assertion->location));
        initializer = AnalyzeExpression(initializer);
        VectorAppend(
            statements,
            NewVariableDeclarationASTNode(
                mapped_result, initializer, assertion->location));
      }
      MapKeyValue mapping = {
          .key.p = assertion->result_binding,
          .value.p = mapped_result,
      };
      MapInsert(&inliner.argument_map, mapping);
    }
  }
  AppendCallerContractStatements(statements, caller_func,
                                 kContractPostcondition, &inliner, call);

  ASTNode* compound = NewCompoundStatementASTNode(statements, location);
  for (size_t i = 0; i < statements->length; i++) {
    ASTNode* statement = statements->value.p[i];
    statement->parent = compound;
    statement->child_id = (int)i;
  }
  ASTNode* result = NULL;
  if (inliner.return_value != NULL) {
    result = NewIdentifierASTNode(inliner.return_value, location);
  }
  ASTNode* lowered =
      NewInlineCallASTNode(call->base.type, location, compound, result);
  lowered->value_category = call->base.value_category;
  MapDestruct(&inliner.argument_map);
  TypeRecordDelete(caller_func);

  if (outer_parent != NULL) {
    ASTNodeReplaceChild(outer_parent, outer_child_id, lowered, false);
  }
  return lowered;
}

typedef struct {
  int node_count;
  bool found_goto;
} GotoFinder;

// Contract: Visitor; counts nodes and detects goto/inline_call/stmt_expr for inline eligibility.
static void ExamineBody(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  GotoFinder* finder = data;
  finder->node_count++;
  if (node->op == AST_OP(goto) || node->op == AST_OP(inline_call) ||
      node->op == AST_OP(stmt_expr)) {
    finder->found_goto = true;
  }
}

typedef struct {
  bool found_unresolved;
} UnresolvedMemberFinder;

typedef struct {
  bool found_nrvo;
} NRVOFinder;

// Contract: Visitor; sets flag when NRVO marker/local would break inlining.
static void FindNRVOLocal(ASTNode* node, void* data, int child_id,
                          VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren) {
    return;
  }
  NRVOFinder* finder = data;
  if ((node->flags & kASTNrvoMarker) != 0 ||
      (node->op == AST_OP(vardecl) &&
       ((VariableDeclarationASTNode*)node)->symbol != NULL &&
       ((VariableDeclarationASTNode*)node)->symbol->is_nrvo)) {
    finder->found_nrvo = true;
  }
}

// Contract: Visitor; detects unlowered init/member access/template callees in body.
static void FindUnresolvedMemberAccess(ASTNode* node, void* data, int child_id,
                                       VisitorMode mode) {
  if (mode != kVisitPreChildren) {
    return;
  }
  UnresolvedMemberFinder* finder = data;
  if ((node->op == AST_OP(dot) || node->op == AST_OP(arrow)) &&
      ((BinaryASTNode*)node)->right != NULL &&
      ((BinaryASTNode*)node)->right->op != AST_OP(structmember)) {
    finder->found_unresolved = true;
  }
  if (node->op == AST_OP(init) &&
      ((BinaryASTNode*)node)->right != NULL &&
      ((BinaryASTNode*)node)->right->op != AST_OP(braced_init)) {
    finder->found_unresolved = true;
  }
  // An analyzed braced initializer carries the type it initializes and holds a
  // designator per entry.  One with no type at all has not been analyzed, which
  // happens to a body reached from a caller that is analyzed first, and its
  // entries are still the plain expressions the parser left: code generation
  // has nowhere to place them.  The analysis that follows lowers the body the
  // function keeps and never sees a copy taken now.
  if (node->op == AST_OP(braced_init) && node->type == NULL) {
    finder->found_unresolved = true;
  }
  if (node->op == AST_OP(call)) {
    ASTNode* callee = ((VectorASTNode*)node)->left;
    if (callee != NULL && callee->op == AST_OP(identifier)) {
      Symbol* symbol = ((IdentifierASTNode*)callee)->symbol;
      // A callee still naming the template itself has not been resolved to the
      // specialization the call reaches.  Copying it emits a call to the
      // template's own name, which nothing defines.
      if (symbol != NULL && (symbol->flags.is_template_type_parameter ||
                             symbol->flags.is_template)) {
        finder->found_unresolved = true;
      }
    }
  }
}

// Does the callee expression name the function it is typed as, rather than hold
// a value that merely has that function type?  A function type record carries
// the body of the function it was declared for, and the lowering of a
// pointer-to-member-function call dereferences the loaded pointer using the
// member function's own type record.  What that call reaches is whatever the
// pointer holds, so inlining the body found on the type would call the wrong
// member.  Calls through a plain function pointer or a vtable slot never get
// here: their callee expression has pointer type.
// Contract: Read-only; distinguishes function designators from pointer values.
bool CalleeNamesItsFunction(ASTNode* callee) {
  if (callee == NULL || callee->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* symbol = ((IdentifierASTNode*)callee)->symbol;
  return symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type);
}

static bool NodeNamesFormalOf(ASTNode* node, void* data) {
  FunctionInfo* info = data;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  for (size_t i = 0; symbol != NULL && i < info->prototype.length; i++) {
    if (info->prototype.value.p[i] == symbol) {
      return true;
    }
  }
  return false;
}

// Contract: Returns body-bearing FunctionInfo (definition symbol if needed); read-only.
FunctionInfo* FunctionInfoForCallInlining(FunctionInfo* func) {
  if (func == NULL) {
    return NULL;
  }
  if (func->body != NULL) {
    // A TypeRecordCopy (e.g. an inherited special-member constructor) shares
    // the body but owns fresh prototype symbols the body does not reference.
    TypeRecord* canonical = func->symbol != NULL ? func->symbol->type : NULL;
    if (canonical != NULL && TypeIsFunction(canonical) &&
        &canonical->info.function != func &&
        canonical->info.function.body == func->body &&
        canonical->info.function.prototype.length == func->prototype.length &&
        !ASTNodeAny(func->body, NodeNamesFormalOf, func) &&
        ASTNodeAny(func->body, NodeNamesFormalOf, &canonical->info.function)) {
      return &canonical->info.function;
    }
    return func;
  }
  if (func->symbol != NULL && func->symbol->value.func_defn != NULL &&
      func->symbol->value.func_defn->type != NULL &&
      TypeIsFunction(func->symbol->value.func_defn->type) &&
      func->symbol->value.func_defn->type->info.function.body != NULL) {
    return &func->symbol->value.func_defn->type->info.function;
  }
  return func;
}

// Contract: Read-only re-entrancy guard against analyzing same function type twice.
static bool FunctionTypeIsBeingAnalyzed(TypeRecord* func_type) {
  if (func_type == NULL) {
    return false;
  }
  for (size_t i = 0; i < compiler->functions_being_analyzed.length; i++) {
    if (compiler->functions_being_analyzed.value.p[i] == func_type) {
      return true;
    }
  }
  return false;
}

// Contract: Read-only early gates (-O2, constexpr evaluation, consteval, noinline).
// Returns true when inlining must not proceed (caller should return false).
static bool InliningBlockedByCalleeOrCompilerFlags(FunctionInfo* func) {
  if (func == NULL || !OptLevel2()) {
    return true;
  }
  if (compiler->constant_evaluation_required_depth > 0) {
    return true;
  }
  if (func->is_consteval) {
    return true;
  }
  if (func->symbol != NULL && func->symbol->flags.noinline) {
    return true;
  }
  return false;
}

// Contract: Read-only; true when the caller's function type still carries unbound
// template parameters (inlining would substitute the wrong argument list).
static bool InliningBlockedByCallerTemplateParameters(void) {
  return compiler->current_function != NULL &&
         TypeIsFunction(compiler->current_function) &&
         TypeContainsTemplateParameter(compiler->current_function);
}

// Contract: Ensures `func->body` is analyzed when policy allows synchronous analysis
// (ctors/dtors/LTO). May mutate compiler analysis state and `func->body` flags.
// Returns false when the body cannot be inlined afterward.
static bool EnsureInlinedCalleeBodyIsAnalyzed(FunctionInfo* func) {
  if (func->body == NULL || (func->body->flags & kASTAnalyzed) != 0) {
    return true;
  }
  bool analyze_now = func->is_constructor || func->is_destructor ||
                     compiler->lto;
  if (func->template_origin != NULL) {
    analyze_now = false;
  }
  if (!analyze_now) {
    return false;
  }
  TypeRecord* func_type = func->symbol != NULL ? func->symbol->type : NULL;
  // Analyzing the body before its member initializers are inserted would
  // mark it analyzed and leave those initializers unanalyzed for codegen.
  if (FunctionTypeIsBeingAnalyzed(func_type) ||
      ConstructorPreambleIsPending(func_type)) {
    return false;
  }
  TypeRecord* saved_function = compiler->current_function;
  Struct* saved_access = compiler->current_class_access_context;
  compiler->current_function = func_type;
  compiler->current_class_access_context = func->cxx_member_owner;
  if (func_type != NULL) {
    VectorAppend(&compiler->functions_being_analyzed, func_type);
  }
  bool saved_udc = SemanticSuspendUserDefinedConversionRank();
  AnalyzeStatement(func->body);
  SemanticResumeUserDefinedConversionRank(saved_udc);
  if (func_type != NULL) {
    for (size_t i = compiler->functions_being_analyzed.length; i-- > 0;) {
      if (compiler->functions_being_analyzed.value.p[i] == func_type) {
        VectorDeleteElement(&compiler->functions_being_analyzed, i);
        break;
      }
    }
  }
  compiler->current_function = saved_function;
  compiler->current_class_access_context = saved_access;
  return (func->body->flags & kASTAnalyzed) != 0;
}

// Contract: Read-only; true when callee or caller contract context forbids inlining.
static bool InliningBlockedByCalleeContractRules(FunctionInfo* func) {
  if (func->contract_assertions.length != 0) {
    return true;
  }
  return compiler->contract_assertion_depth > 0;
}

// Contract: Read-only; true when the callee still shares its primary template's body AST.
static bool InliningBlockedByLazyTemplateBodySharing(FunctionInfo* func) {
  return func->template_origin != NULL &&
         func->template_origin->type != NULL &&
         TypeIsFunction(func->template_origin->type) &&
         func->body == func->template_origin->type->info.function.body;
}

// Contract: Read-only; true when inlining a member of a class with virtual bases is unsafe.
static bool InliningBlockedByVirtualBaseMemberOwner(FunctionInfo* func) {
  return func->cxx_member_owner != NULL &&
         func->cxx_member_owner->virtual_bases.length != 0;
}

// Contract: Read-only except for output flags; true when symbol/linkage/inline keyword
// rules reject inlining. Sets `*force_inline_out` and `*unmarked_small_out` when false
// would not be returned solely due to those checks (always set before returning true).
static bool InliningBlockedByDefinitionAndLinkage(
    FunctionInfo* func, bool* force_inline_out, bool* unmarked_small_out) {
  bool force_inline =
      func->symbol != NULL && func->symbol->flags.always_inline;
  bool unmarked_small = (OptLevel3() || compiler->lto) && !func->is_inline &&
                        !force_inline;
  *force_inline_out = force_inline;
  *unmarked_small_out = unmarked_small;
  if (func->symbol == NULL ||
      (!func->is_inline && !force_inline && !unmarked_small) ||
      !func->symbol->flags.is_defined || func->body == NULL ||
      compiler->current_function == NULL ||
      compiler->current_function->info.function.is_constexpr ||
      func->symbol == compiler->current_function->info.function.symbol ||
      func->unknown_args || func->varargs) {
    return true;
  }
  return false;
}

// Contract: Read-only scan of formals; true when parameter packs or initializer_list
// parameters forbid inlining.
static bool InliningBlockedByCalleeFormals(FunctionInfo* func) {
  for (size_t i = 0; i < func->prototype.length; i++) {
    Symbol* formal = func->prototype.value.p[i];
    if (formal == NULL) {
      continue;
    }
    if (formal->flags.is_parameter_pack) {
      return true;
    }
    if (TypeIsCXXInitializerList(formal->type) ||
        (formal->type != NULL && TypeIsReference(formal->type) &&
         TypeIsCXXInitializerList(formal->type->next))) {
      return true;
    }
  }
  return false;
}

// Contract: Read-only body inspection and size heuristic; false when goto/unresolved/NRVO
// or node count exceeds limits (unless `force_inline` bypasses size only).
static bool CalleeBodyEligibleByStructureAndSize(FunctionInfo* func,
                                                 bool force_inline,
                                                 bool unmarked_small) {
  const int kMaxInlineNodeCount = OptLevel3() ? 250 : 100;
  const int kMaxUnmarkedInlineNodeCount = 40;
  GotoFinder finder = {0, false};
  ASTNodeVisit(func->body, ExamineBody, 0, &finder);
  if (finder.found_goto) {
    return false;
  }
  UnresolvedMemberFinder unresolved = {false};
  ASTNodeVisit(func->body, FindUnresolvedMemberAccess, 0, &unresolved);
  if (unresolved.found_unresolved) {
    return false;
  }
  NRVOFinder nrvo = {false};
  ASTNodeVisit(func->body, FindNRVOLocal, 0, &nrvo);
  if (nrvo.found_nrvo) {
    return false;
  }
  if (force_inline) {
    return true;
  }
  int max_nodes =
      unmarked_small ? kMaxUnmarkedInlineNodeCount : kMaxInlineNodeCount;
  return finder.node_count < max_nodes;
}

// We can only inline a function if:
// 1. It is defined and has a body
// 2. It is not the current function.
// 3. It's not a varargs function or has unknown args.
// 4. It has no goto statements.
// 5. The number of AST nodes is reasonably small.
// 6. It has no unresolved member accesses or unlowered initializers. Template
//    bodies can retain either until their concrete instantiation is analyzed;
//    cloning such a body would leave nodes that code generation cannot handle.
// 7. It has no named-return-value object. NRVO binds that local directly to the
//    callee's hidden aggregate-result slot, which does not exist after inlining.
//
// Constructors and destructors may be inlined.  A constructor call that
// constructs a temporary is retargeted onto the enclosing object during
// codegen via current_struct_address.  Inlined destructor statements keep
// kASTInlinedDestructor so exception cleanup can still pair them with the
// object they destroy.
//
// Why the goto prohibition.  Well, the GotoStatementASTNode contains
// a resolved reference to its label.  We clone the body to
// inline it, so this reference is no longer valid after the
// copy.  It's really hard to find the new resolved label without
// traversing the whole function looking for the label.  Goto statements
// are pretty rare anyway so this isn't a big deal really.
//
// NOTE: a tail-recursive inline function will not be inlined because
// the tail recursion is converted into a goto statement.
// Contract: Read-only eligibility predicate; defers inlining for template-dependent bodies.
bool FunctionCanBeInlined(FunctionInfo* func) {
  if (InliningBlockedByCalleeOrCompilerFlags(func)) {
    return false;
  }
  if (InliningBlockedByCallerTemplateParameters()) {
    return false;
  }
  if (!EnsureInlinedCalleeBodyIsAnalyzed(func)) {
    return false;
  }
  if (InliningBlockedByCalleeContractRules(func)) {
    return false;
  }
  if (InliningBlockedByLazyTemplateBodySharing(func)) {
    return false;
  }
  if (InliningBlockedByVirtualBaseMemberOwner(func)) {
    return false;
  }
  bool force_inline = false;
  bool unmarked_small = false;
  if (InliningBlockedByDefinitionAndLinkage(func, &force_inline,
                                            &unmarked_small)) {
    return false;
  }
  if (InliningBlockedByCalleeFormals(func)) {
    return false;
  }
  return CalleeBodyEligibleByStructureAndSize(func, force_inline,
                                                unmarked_small);
}
