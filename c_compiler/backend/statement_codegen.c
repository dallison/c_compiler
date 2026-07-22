//
//  statement_codegen.c
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "statement_codegen.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "expr_codegen.h"

static void GenerateDeclarationList(Generator* gen,
                                    DeclarationListASTNode* node) {
  size_t num_decls = node->declarations->length;
  for (size_t i = 0; i < num_decls; i++) {
    GenerateStatement(gen, node->declarations->value.p[i]);
  }
}

static bool IsSupportedTypedCatch(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL) {
    return false;
  }
  TypeRecord* type = symbol->type;
  if (TypeIsReference(type)) {
    type = type->next;
  }
  return TypeIsIntegral(type) || TypeIsPointer(type) || TypeIsStructOrUnion(type);
}

static bool IsSupportedCatchHandler(CatchASTNode* handler) {
  return handler != NULL &&
         (handler->is_catch_all || IsSupportedTypedCatch(handler->symbol));
}

static bool TryStatementHasSupportedHandler(TryASTNode* node) {
  for (size_t i = 0; i < node->catches->length; i++) {
    CatchASTNode* handler = node->catches->value.p[i];
    if (IsSupportedCatchHandler(handler)) {
      return true;
    }
  }
  return false;
}

static const char* CurrentExceptionAccessorName(TypeRecord* type) {
  if (TypeIsReference(type)) {
    type = type->next;
  }
  if (TypeIsStructOrUnion(type)) {
    return "__davecc_current_exception_object";
  }
  if (TypeIsPointer(type)) {
    return "__davecc_current_exception_ptr";
  }
  if (TypeIsFloatingPoint(type)) {
    return type->size == 4 ? "__davecc_current_exception_f4"
                           : "__davecc_current_exception_f8";
  }
  switch (type->size) {
    case 1:
      return "__davecc_current_exception_i1";
    case 2:
      return "__davecc_current_exception_i2";
    case 4:
      return "__davecc_current_exception_i4";
    case 8:
      return "__davecc_current_exception_i8";
    default:
      return "__davecc_current_exception_i4";
  }
}

static TypeRecord* CurrentExceptionAccessorReturnType(TypeRecord* catch_type) {
  if (TypeIsReference(catch_type)) {
    catch_type = catch_type->next;
  }
  if (TypeIsStructOrUnion(catch_type)) {
    TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
    return NewPointerTo(kQualPlain, void_type);
  }
  if (TypeIsPointer(catch_type)) {
    TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
    return NewPointerTo(kQualPlain, void_type);
  }
  return TypeRecordCopy(catch_type);
}

static Symbol* GetCurrentExceptionFunction(TypeRecord* catch_type,
                                           SourceLocation location) {
  const char* name = CurrentExceptionAccessorName(catch_type);
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* symbol = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  if (symbol != NULL) {
    return symbol;
  }

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, CurrentExceptionAccessorReturnType(catch_type));
  symbol = NewSymbol(name, func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->location = location;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

static Symbol* GetCurrentExceptionAddressFunction(SourceLocation location) {
  String symbol_name;
  StringInit(&symbol_name, "__davecc_current_exception_addr");
  Symbol* symbol = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  if (symbol != NULL) {
    return symbol;
  }

  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, NewPointerTo(kQualPlain, void_type));
  symbol = NewSymbol("__davecc_current_exception_addr", func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->location = location;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

static IRNode* GenerateNoArgRuntimeCall(Generator* gen, Symbol* symbol) {
  IRNode* func = GeneratorGetVariable(gen, symbol);
  IRNode* call = NewIR1(IR_OP(calla), func);
  TypeRecord* return_type = symbol->type->next;
  return IRSetType(GeneratorEmit(gen, call), return_type);
}

static void GenerateReferenceCatchBinding(Generator* gen, CatchASTNode* handler) {
  SourceLocation location = handler->base.location;
  TypeRecord* referred_type = handler->symbol->type->next;
  Symbol* accessor = TypeIsStructOrUnion(referred_type)
                         ? GetCurrentExceptionFunction(handler->symbol->type,
                                                       location)
                         : GetCurrentExceptionAddressFunction(location);
  IRNode* ref_storage = GeneratorGetVariable(gen, handler->symbol);
  IRNode* address = GenerateNoArgRuntimeCall(gen, accessor);
  IRSetType(address, NewPointerTo(kQualPlain, referred_type));
  IRNode* store =
      IRSetType(GeneratorEmit(gen, NewIR2(IR_OP(storea), ref_storage, address)),
                handler->symbol->type);
  IRSetVarDef(store, handler->symbol);
}

static void GenerateStructCatchBinding(Generator* gen, CatchASTNode* handler) {
  SourceLocation location = handler->base.location;
  Symbol* accessor = GetCurrentExceptionFunction(handler->symbol->type,
                                                 location);
  IRNode* dest = GeneratorGetVariable(gen, handler->symbol);
  IRNode* source = GenerateNoArgRuntimeCall(gen, accessor);
  IRSetType(source, NewPointerTo(kQualPlain, handler->symbol->type));
  IRNode* copy = GeneratorEmit(
      gen, NewIR3(IR_OP(memcpy), dest, source,
                  GeneratorGetIntConstant(gen, NULL,
                                          handler->symbol->type->size)));
  IRSetVarDef(copy, handler->symbol);
}

static void GenerateCatchBinding(Generator* gen, CatchASTNode* handler) {
  if (handler == NULL || handler->is_catch_all || handler->symbol == NULL ||
      !IsSupportedTypedCatch(handler->symbol)) {
    return;
  }

  if (TypeIsReference(handler->symbol->type)) {
    GenerateReferenceCatchBinding(gen, handler);
    return;
  }
  if (TypeIsStructOrUnion(handler->symbol->type)) {
    GenerateStructCatchBinding(gen, handler);
    return;
  }

  SourceLocation location = handler->base.location;
  Symbol* current_exception =
      GetCurrentExceptionFunction(handler->symbol->type, location);
  ASTNode* lhs = NewIdentifierASTNode(handler->symbol, location);
  ASTNode* callee = NewIdentifierASTNode(current_exception, location);
  lhs->flags |= kASTNeedAddress;
  callee->flags |= kASTNeedAddress;
  ASTNode* rhs = NewVectorASTNode(AST_OP(call), handler->symbol->type,
                                  location, callee, NewVector());
  ASTNode* assignment = NewBinaryASTNode(AST_OP(assign), handler->symbol->type,
                                        location, lhs, rhs);
  GenerateExpression(gen, assignment);
}

static ASTNode* UnwrapExpressionInitializer(ASTNode* node) {
  if (node != NULL && node->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)node)->expr;
  }
  return node;
}

static ASTNode* CXXSingleCompoundLiteralInitializer(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(compound_literal)) {
    return NULL;
  }
  CompoundLiteralASTNode* literal = (CompoundLiteralASTNode*)node;
  if (literal->initializer == NULL ||
      literal->initializer->op != AST_OP(braced_init)) {
    return NULL;
  }
  BracedInitializerASTNode* braced =
      (BracedInitializerASTNode*)literal->initializer;
  if (braced->initializers->length != 1) {
    return NULL;
  }
  ASTNode* only = braced->initializers->value.p[0];
  if (only == NULL || only->op != AST_OP(designated_init)) {
    return NULL;
  }
  return UnwrapExpressionInitializer(((DesignatedInitializerASTNode*)only)->init);
}

static ASTNode* CXXElidableStructReturnInitializer(ASTNode* initializer,
                                                   TypeRecord* target) {
  ASTNode* expr = UnwrapExpressionInitializer(initializer);
  if (!CompilerIsCXX() || expr == NULL || target == NULL ||
      !TypeIsStructOrUnion(target)) {
    return NULL;
  }
  if ((expr->op == AST_OP(call) || expr->op == AST_OP(inline_call) ||
       expr->op == AST_OP(compound_literal) || expr->op == AST_OP(comma)) &&
      TypeEqual(expr->type, target)) {
    return expr;
  }
  if (expr->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)expr;
  if (call->left == NULL || call->left->op != AST_OP(identifier)) {
    return NULL;
  }
  Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
  if (callee == NULL || !TypeIsFunction(callee->type) ||
      !callee->type->info.function.is_constructor) {
    return NULL;
  }
  for (size_t i = 0; i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    ASTNode* inner = CXXSingleCompoundLiteralInitializer(actual);
    if (inner != NULL && TypeEqual(inner->type, target)) {
      return inner;
    }
  }
  return NULL;
}

static EHTypeInfo* CatchHandlerTypeInfo(Generator* gen, CatchASTNode* handler) {
  if (handler == NULL || handler->is_catch_all || handler->symbol == NULL) {
    return NULL;
  }
  return GeneratorGetExceptionTypeInfo(gen, handler->symbol->type);
}

static void RecordExceptionRange(Generator* gen, IRNode* try_start,
                                 IRNode* try_end, IRNode* catch_label,
                                 EHTypeInfo* catch_typeinfo) {
  ExceptionHandlerRange* range = malloc(sizeof(ExceptionHandlerRange));
  range->try_start = try_start;
  range->try_end = try_end;
  range->catch_label = catch_label;
  range->catch_typeinfo = catch_typeinfo;
  range->is_cleanup = false;
  VectorAppend(&gen->exception_ranges, range);
}

// A cleanup range: while the exception unwinder's pc is in [region_start,
// region_end], `pad` (which destroys one automatic object and resumes) is run
// before any enclosing handler.  Nested/enclosing objects chain automatically
// via the runtime's scope-containment walk (see libc/eh_throw.c).
static void RecordCleanupRange(Generator* gen, IRNode* region_start,
                               IRNode* region_end, IRNode* pad) {
  ExceptionHandlerRange* range = malloc(sizeof(ExceptionHandlerRange));
  range->try_start = region_start;
  range->try_end = region_end;
  range->catch_label = pad;
  range->catch_typeinfo = NULL;
  range->is_cleanup = true;
  VectorAppend(&gen->exception_ranges, range);
}

// Look up (or lazily declare) the runtime entry point that implements
// std::terminate.  It never returns, so callers don't emit any follow-on code.
static Symbol* GetDaveCCTerminateFunction(SourceLocation location) {
  String name;
  StringInit(&name, "__davecc_terminate");
  Symbol* symbol = FindGlobalSymbol(&name);
  StringDestruct(&name);
  if (symbol != NULL) {
    symbol->flags.noreturn = true;
    return symbol;
  }

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  symbol = NewSymbol("__davecc_terminate", func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->flags.noreturn = true;
  symbol->location = location;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

// Look up (or lazily declare) the runtime entry point that continues unwinding
// after a cleanup landing pad has run its destructor.  It never returns.
static Symbol* GetDaveCCResumeFunction(SourceLocation location) {
  String name;
  StringInit(&name, "__davecc_resume");
  Symbol* symbol = FindGlobalSymbol(&name);
  StringDestruct(&name);
  if (symbol != NULL) {
    symbol->flags.noreturn = true;
    return symbol;
  }

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  symbol = NewSymbol("__davecc_resume", func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->flags.noreturn = true;
  symbol->location = location;
  SyntaxAddSymbol(&compiler->syntax, symbol);
  return symbol;
}

static bool IsDestructorStatement(ASTNode* stmt);

// A cleanup landing pad scheduled during body codegen and emitted after the
// function's return path (so it is only reached via the unwinder).  Running the
// pad destroys the object(s) `dtor_stmts` and then resumes unwinding.
typedef struct {
  IRNode* pad_label;
  Vector dtor_stmts;  // ASTNode* destructor statements, run in order.
} PendingCleanupPad;

// The automatic object a compiler-inserted destructor statement acts on, or
// NULL if the statement is not of the recognized `receiver.~T()` shape.  Used to
// pair a block's trailing destructor statements with the declarations above
// them so each object gets a precise EH cleanup range.
static Symbol* CleanupReceiverSymbol(ASTNode* dtor_stmt) {
  if (!IsDestructorStatement(dtor_stmt)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)((ExpressionStatementASTNode*)dtor_stmt)->expr;
  ASTNode* receiver = NULL;
  if (call->left->op == AST_OP(dot) || call->left->op == AST_OP(arrow)) {
    // Pre-lowering shape: receiver.~T().
    receiver = ((BinaryASTNode*)call->left)->left;
  } else if (call->children != NULL && call->children->length > 0) {
    // Analyzed shape: the member call is lowered to ~T(&receiver), so the object
    // is the (address-of) `this` argument.
    receiver = call->children->value.p[0];
  }
  while (receiver != NULL &&
         (receiver->op == AST_OP(address) || receiver->op == AST_OP(contents))) {
    receiver = ((UnaryASTNode*)receiver)->sub;
  }
  if (receiver != NULL && receiver->op == AST_OP(subscript)) {
    receiver = ((BinaryASTNode*)receiver)->left;  // array element: arr[k].~T()
  }
  if (receiver != NULL && receiver->op == AST_OP(identifier)) {
    return ((IdentifierASTNode*)receiver)->symbol;
  }
  return NULL;
}

// Visitor that flags whether a subtree can raise an exception: only a `throw`
// or a call (which may itself throw) can do so.  Used to decide whether a
// noexcept function needs a runtime terminate guard at all.
static void NoexceptMightThrowVisitor(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  bool* might_throw = data;
  if (*might_throw) {
    return;
  }
  if (node->op == AST_OP(throw) || ASTIsCallNode(node)) {
    *might_throw = true;
  }
}

static bool FunctionBodyMightThrow(ASTNode* body) {
  bool might_throw = false;
  ASTNodeVisit(body, NoexceptMightThrowVisitor, 0, &might_throw);
  return might_throw;
}

// A noexcept function needs the terminate guard only when exceptions are
// enabled and its body can actually raise one; a body with no throw or call
// can never violate the specification, so no guard is emitted for it.
static bool FunctionNeedsNoexceptGuard(Generator* gen) {
  TypeRecord* func = gen->func;
  if (gen->for_constant_evaluation || !CompilerIsCXX() ||
      !CompilerExceptionsEnabled() || func == NULL || !TypeIsFunction(func) ||
      !func->info.function.is_noexcept) {
    return false;
  }
  return FunctionBodyMightThrow(func->info.function.body);
}

// The terminate guard implements [except.spec]: an exception that escapes a
// noexcept function must call std::terminate.  It is modelled as a function-
// wide catch-all exception range whose landing pad calls the terminate runtime.
// During unwinding the runtime selects the innermost matching range, so a
// local try/catch that handles the exception still takes precedence; only an
// exception that would truly leave the function reaches this catch-all.
//
// GenerateNoexceptGuardEnter emits the label that opens the guarded region (the
// whole function body) and decides whether a guard is needed at all.
void GenerateNoexceptGuardEnter(Generator* gen, NoexceptTerminateGuard* guard) {
  guard->active = FunctionNeedsNoexceptGuard(gen);
  guard->try_start = NULL;
  guard->try_end = NULL;
  if (!guard->active) {
    return;
  }
  guard->try_start = GeneratorEmit(gen, NewIR(IR_OP(label)));
}

// Closes the guarded region after the function body has been generated.
void GenerateNoexceptGuardLeave(Generator* gen, NoexceptTerminateGuard* guard) {
  if (!guard->active) {
    return;
  }
  guard->try_end = GeneratorEmit(gen, NewIR(IR_OP(label)));
}

// Emits the terminate landing pad (placed after the normal return path so it is
// reached only via the unwinder) and records the function-wide catch-all range.
void GenerateNoexceptGuardTerminate(Generator* gen,
                                    NoexceptTerminateGuard* guard) {
  if (!guard->active) {
    return;
  }
  SourceLocation location = gen->func->info.function.symbol->location;
  IRNode* landing_pad = GeneratorEmit(gen, NewIR(IR_OP(label)));
  Symbol* terminate = GetDaveCCTerminateFunction(location);
  GenerateNoArgRuntimeCall(gen, terminate);
  // __davecc_terminate never returns, but the block still needs a terminator so
  // the CFG builder does not treat it as falling through to a (nonexistent)
  // successor.  These instructions are unreachable.
  GeneratorEmit(gen, NewIR(IR_OP(leave)));
  GeneratorEmit(gen, NewIR(IR_OP(ret)));
  RecordExceptionRange(gen, guard->try_start, guard->try_end, landing_pad,
                       /*catch_typeinfo=*/NULL);
}

// Emits the scope-exit cleanup landing pads scheduled during body codegen, after
// the function's return path so they are reached only via the unwinder.  Each
// pad destroys its automatic object(s) and then resumes unwinding; the runtime
// chains enclosing scopes' pads so the whole live set is destroyed in reverse
// construction order (see libc/eh_throw.c).
void GenerateCleanupLandingPads(Generator* gen) {
  if (gen->cleanup_pads.length == 0) {
    return;
  }
  SourceLocation location = gen->func->info.function.symbol->location;
  Symbol* resume = GetDaveCCResumeFunction(location);
  for (size_t i = 0; i < gen->cleanup_pads.length; i++) {
    PendingCleanupPad* pad = gen->cleanup_pads.value.p[i];
    GeneratorEmit(gen, pad->pad_label);
    for (size_t j = 0; j < pad->dtor_stmts.length; j++) {
      GenerateStatement(gen, pad->dtor_stmts.value.p[j]);
    }
    GenerateNoArgRuntimeCall(gen, resume);
    // __davecc_resume never returns, but the block still needs a terminator so
    // the CFG builder does not treat it as falling through.  Unreachable.
    GeneratorEmit(gen, NewIR(IR_OP(leave)));
    GeneratorEmit(gen, NewIR(IR_OP(ret)));
  }
}

void FreeCleanupPads(Generator* gen) {
  for (size_t i = 0; i < gen->cleanup_pads.length; i++) {
    PendingCleanupPad* pad = gen->cleanup_pads.value.p[i];
    VectorDestruct(&pad->dtor_stmts);
    free(pad);
  }
  VectorDestruct(&gen->cleanup_pads);
}

static bool IsDestructorStatement(ASTNode* stmt) {
  if (stmt == NULL || stmt->op != AST_OP(expr)) {
    return false;
  }
  ExpressionStatementASTNode* expr_stmt = (ExpressionStatementASTNode*)stmt;
  if (expr_stmt->expr == NULL || expr_stmt->expr->op != AST_OP(call)) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)expr_stmt->expr;
  if (call->left == NULL) {
    return false;
  }
  if (call->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)call->left;
    return id->symbol != NULL && id->symbol->type != NULL &&
           id->symbol->type->info.function.is_destructor;
  }
  if (call->left->op != AST_OP(dot) && call->left->op != AST_OP(arrow)) {
    return false;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)call->left;
  if (member_access->right == NULL) {
    return false;
  }
  if (member_access->right->op == AST_OP(structmember)) {
    StructMemberASTNode* member = (StructMemberASTNode*)member_access->right;
    return member->member != NULL && member->member->is_member_function &&
           member->member->symbol != NULL &&
           member->member->symbol->type != NULL &&
           member->member->symbol->type->info.function.is_destructor;
  }
  if (member_access->right->op == AST_OP(string)) {
    ConstantASTNode* name = (ConstantASTNode*)member_access->right;
    return name->value.string != NULL && name->value.string->value[0] == '~';
  }
  return false;
}

static bool StatementMayFallThrough(ASTNode* stmt) {
  if (stmt == NULL) {
    return true;
  }
  switch (stmt->op) {
    case AST_OP(return):
    case AST_OP(throw):
      return false;
    case AST_OP(compound): {
      CompoundStatementASTNode* compound = (CompoundStatementASTNode*)stmt;
      if (compound->statements->length == 0) {
        return true;
      }
      return StatementMayFallThrough(VectorLast(compound->statements));
    }
    case AST_OP(if): {
      IfStatementASTNode* if_stmt = (IfStatementASTNode*)stmt;
      return if_stmt->else_part == NULL ||
             StatementMayFallThrough(if_stmt->if_part) ||
             StatementMayFallThrough(if_stmt->else_part);
    }
    default:
      return true;
  }
}

static ASTNode* FindEnclosingLoop(ASTNode* stmt) {
  while (stmt != NULL) {
    switch (stmt->op) {
      case AST_OP(for): {
        ForStatementASTNode* f = (ForStatementASTNode*)stmt;
        return f->stmt;
      }
      case AST_OP(while):
      case AST_OP(do): {
        CombinedStatementASTNode* c = (CombinedStatementASTNode*)stmt;
        return c->stmt;
      }
      default:
        break;
    }
    stmt = stmt->parent;
  }
  return NULL;
}

static ASTNode* FindEnclosingLoopOrSwitch(ASTNode* stmt) {
  while (stmt != NULL) {
    switch (stmt->op) {
       case AST_OP(for): {
        ForStatementASTNode* f = (ForStatementASTNode*)stmt;
        return f->stmt;
      }
      case AST_OP(while):
      case AST_OP(do): {
        CombinedStatementASTNode* c = (CombinedStatementASTNode*)stmt;
        return c->stmt;
      }
      case AST_OP(switch): {
        SwitchStatementASTNode *s = (SwitchStatementASTNode*)stmt;
        return s->stmt;
      }
      default:
        break;
    }
    stmt = stmt->parent;
  }
  return NULL;
}

static IRNode* ContainsVLA(DeclarationListASTNode* decl_list) {
  for (size_t j = 0; j < decl_list->declarations->length; j++) {
     VariableDeclarationASTNode* decl = decl_list->declarations->value.p[j];
     if (TypeIsVLA(decl->base.type)) {
       return decl->saved_sp;
     }
  }
  return NULL;
}

static IRNode* FindTopVLAForJump(ASTNode* jump, ASTNode* dest) {
  ASTNode* node = jump;
  IRNode* top_vla = NULL;
  while (node != dest) {
    if (node->parent != NULL && node->parent->op == AST_OP(compound)) {
      CompoundStatementASTNode* c = (CompoundStatementASTNode*)node->parent;
      bool found_vla = false;
      for (size_t i = 0; !found_vla && i < c->statements->length; i++) {
        ASTNode* stmt = c->statements->value.p[i];
        if (stmt == node) {
          break;
        }
        if (stmt->op == AST_OP(decl_list)) {
          DeclarationListASTNode* decl_list = (DeclarationListASTNode*)stmt;
          IRNode* vla = ContainsVLA(decl_list);
          if (vla != NULL) {
            top_vla = vla;
            found_vla = true;
          }
        }
      }
    }
    node = node->parent;
  }
  return top_vla;
}

IRNode* GenerateVLASize(Generator* gen, TypeRecord* type) {
  IRNode* size_expr = GenerateExpression(gen,
                                         type->info.array.size.vla.size);
  IRNode* sub_size;
  if (TypeIsVLA(type->next)) {
    sub_size = GenerateVLASize(gen, type->next);
  } else {
    sub_size = GeneratorGetIntConstant(gen,
                                       NULL,
                                       type->next->size);
  }
  IRNode* result = GeneratorEmit(gen, NewIR2(IR_OP(muli), size_expr, sub_size));
  type->info.array.size.vla.codegen_info = result;
  return result;
}

// VLA definition.  The size of the array is an expression held in the
// array info in the type.  This needs to be multiplied by the array's
// subtype's size, which might also be a VLA.
// Also writes the saved SP address into the decl node's saved_sp.
static IRNode* GenerateVLADefinition(Generator* gen, TypeRecord* type,
                                     VariableDeclarationASTNode* decl) {
  // Size of array.
  IRNode* size = GenerateVLASize(gen, type);
  
  // Saved stack pointer.
  IRNode* saved_sp = GeneratorEmit(gen, NewIR(IR_OP(tmp)));
  IRSetType(saved_sp, NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain)));
  GeneratorEmit(gen, NewIR1(IR_OP(savesp), saved_sp));
  decl->saved_sp = saved_sp;
  
  // Address of VLA (current stack pointer after decrement).
  IRNode* array_addr = GeneratorEmit(gen, NewIR(IR_OP(tmp)));
  IRSetType(array_addr, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  
  // Make space for aligned array on stack.
  IRNode* aligned = GeneratorEmit(gen, NewIR2(IR_OP(aligni),
                                             size,
                                             GeneratorGetIntConstant(gen,
                                                                     NULL,
                                                                     compiler->target->stack_alignment)));
  GeneratorEmit(gen, NewIR1(IR_OP(decsp), aligned));
  GeneratorEmit(gen, NewIR1(IR_OP(savesp), array_addr));
  return array_addr;
}

static void GenerateVariableDeclaration(Generator* gen,
                                        VariableDeclarationASTNode* node) {
  if (TypeIsVLA(node->symbol->type)) {
    // Variable Length Array.
    IRNode* addr = GenerateVLADefinition(gen, node->symbol->type, node);
    node->symbol->value.other = addr;
  }
  if (node->initializer != NULL) {
    ASTNode* elidable = CXXElidableStructReturnInitializer(
        node->initializer, node->symbol->type);
    if (elidable != NULL) {
      IRNode* old_struct_address = gen->current_struct_address;
      IRNode* var = GeneratorGetVariable(gen, node->symbol);
      IRNode* ref = IRSetType(GeneratorEmit(gen, NewIR1(IR_OP(addressof), var)),
                              NewPointerTo(kQualPlain, node->symbol->type));
      gen->current_struct_address = ref;
      GenerateExpression(gen, elidable);
      gen->current_struct_address = old_struct_address;
    } else {
      GenerateExpression(gen, node->initializer);
    }
  }
}


static void GenerateExpressionStatement(Generator* gen,
                                        ExpressionStatementASTNode* node) {
  GenerateExpression(gen, node->expr);
}

// Schedules a cleanup landing pad (emitted after the return path) that destroys
// `dtor_stmts` and resumes unwinding, and records its EH range [start, end].
static void ScheduleCleanupPad(Generator* gen, IRNode* region_start,
                               IRNode* region_end, Vector* dtor_stmts) {
  IRNode* pad = NewIR(IR_OP(label));
  RecordCleanupRange(gen, region_start, region_end, pad);
  PendingCleanupPad* pending = malloc(sizeof(PendingCleanupPad));
  pending->pad_label = pad;
  VectorInit(&pending->dtor_stmts);
  for (size_t i = 0; i < dtor_stmts->length; i++) {
    VectorAppend(&pending->dtor_stmts, dtor_stmts->value.p[i]);
  }
  VectorAppend(&gen->cleanup_pads, pending);
}

// The index of the first of the block's trailing destructor statements (the
// scope-exit destructors the front end appends for locals), or the statement
// count if there are none.  Statements flagged kASTEHCleanupOnly (a
// constructor's partial-construction subobject destructors) are not scope-exit
// local destructors and stop the scan.
static size_t TrailingDestructorStart(CompoundStatementASTNode* node) {
  size_t start = node->statements->length;
  while (start > 0) {
    ASTNode* stmt = node->statements->value.p[start - 1];
    if (!IsDestructorStatement(stmt) || (stmt->flags & kASTEHCleanupOnly)) {
      break;
    }
    start--;
  }
  return start;
}

static bool SymbolHasTrailingDestructor(CompoundStatementASTNode* node,
                                        size_t trailing_start, Symbol* sym) {
  for (size_t j = trailing_start; j < node->statements->length; j++) {
    if (CleanupReceiverSymbol(node->statements->value.p[j]) == sym) {
      return true;
    }
  }
  return false;
}

typedef struct {
  IRNode* start_label;
  Symbol* symbol;
} OpenCleanup;

static bool GenerateCompoundExceptionCleanup(Generator* gen) {
  return CompilerIsCXX() && CompilerExceptionsEnabled() &&
         !gen->for_constant_evaluation;
}

// Schedules a cleanup pad for every still-open local automatic object, pairing
// it with the block's trailing destructor statements that act on it, over the
// range [object construction, scope_end].
static void ScheduleOpenLocalCleanups(Generator* gen,
                                      CompoundStatementASTNode* node,
                                      size_t trailing_start, Vector* open,
                                      IRNode* scope_end) {
  size_t num_statements = node->statements->length;
  for (size_t k = 0; k < open->length; k++) {
    OpenCleanup* oc = open->value.p[k];
    Vector dtors;
    VectorInit(&dtors);
    for (size_t j = trailing_start; j < num_statements; j++) {
      ASTNode* dtor = node->statements->value.p[j];
      if (CleanupReceiverSymbol(dtor) == oc->symbol) {
        VectorAppend(&dtors, dtor);
      }
    }
    ScheduleCleanupPad(gen, oc->start_label, scope_end, &dtors);
    VectorDestruct(&dtors);
  }
}

static void GenerateCompoundStatement(Generator* gen,
                                      CompoundStatementASTNode* node) {
  size_t num_statements = node->statements->length;

  // For C++ with exceptions on, give every automatic object with a destructor a
  // precise EH cleanup range covering [after its construction, block-scope end],
  // so an exception propagating through the block destroys exactly the objects
  // that are live -- including through frames that have no try/catch of their
  // own.  Enclosing and nested objects are ordered by the runtime's
  // scope-containment walk (see libc/eh_throw.c).
  //
  // The same machinery also handles a constructor's partial-construction
  // cleanup: the front end inserts each fully-constructed subobject's destructor
  // right after its initializer, flagged kASTEHCleanupOnly.  Those statements
  // are never emitted on the normal path; each gets a cleanup range covering the
  // rest of the constructor, so a throw destroys exactly the subobjects built so
  // far, in reverse construction order.
  bool eh = GenerateCompoundExceptionCleanup(gen);
  size_t trailing_start = num_statements;
  bool local_cleanups = false;
  bool member_cleanups = false;
  if (eh) {
    trailing_start = TrailingDestructorStart(node);
    bool has_decls = false;
    for (size_t i = 0; i < num_statements; i++) {
      ASTNode* stmt = node->statements->value.p[i];
      if (stmt->flags & kASTEHCleanupOnly) {
        member_cleanups = true;
      } else if (i < trailing_start && stmt->op == AST_OP(decl_list)) {
        has_decls = true;
      }
    }
    local_cleanups = has_decls && trailing_start < num_statements;
  }
  bool do_cleanup = eh && (local_cleanups || member_cleanups);

  // A single scope-end label bounds every cleanup range in this block.  It is
  // positioned just before the trailing local destructors (or at the block end
  // when there are none) so member/subobject cleanups also cover local
  // constructions and the user body.
  IRNode* scope_end = do_cleanup ? NewIR(IR_OP(label)) : NULL;
  bool scope_end_emitted = false;

  Vector open;  // OpenCleanup* for objects whose scope has not yet ended.
  VectorInit(&open);

  for (size_t i = 0; i < num_statements; i++) {
    ASTNode* stmt = node->statements->value.p[i];
    if (do_cleanup && !scope_end_emitted && i == trailing_start) {
      // Reached the trailing destructors: the block scope ends here.  Close the
      // live range of every object declared above and schedule its cleanup pad.
      GeneratorEmit(gen, scope_end);
      scope_end_emitted = true;
      ScheduleOpenLocalCleanups(gen, node, trailing_start, &open, scope_end);
    }
    if (stmt->flags & kASTEHCleanupOnly) {
      // Constructor subobject destructors: emitted only inside a cleanup pad,
      // never on the normal path (including constant evaluation, where the
      // object stays alive).  A run of consecutive kASTEHCleanupOnly statements
      // belongs to one subobject (e.g. the per-element destructors of an array
      // member) and is pooled into a single cleanup pad so it forms one EH range
      // run in one pass, in emitted (reverse-element) order.  The range starts
      // here -- right after the subobject's initializer -- and runs to scope_end.
      size_t run_end = i;
      while (run_end + 1 < num_statements &&
             (((ASTNode*)node->statements->value.p[run_end + 1])->flags &
              kASTEHCleanupOnly)) {
        run_end++;
      }
      if (do_cleanup) {
        IRNode* start_label = GeneratorEmit(gen, NewIR(IR_OP(label)));
        Vector dtors;
        VectorInit(&dtors);
        for (size_t j = i; j <= run_end; j++) {
          VectorAppend(&dtors, node->statements->value.p[j]);
        }
        ScheduleCleanupPad(gen, start_label, scope_end, &dtors);
        VectorDestruct(&dtors);
      }
      i = run_end;  // the loop's i++ advances past the whole run
      continue;
    }
    GenerateStatement(gen, stmt);
    if (do_cleanup && !scope_end_emitted && stmt->op == AST_OP(decl_list)) {
      // Objects constructed by this declaration become live from here.
      DeclarationListASTNode* decls = (DeclarationListASTNode*)stmt;
      IRNode* start_label = NULL;
      for (size_t d = 0; d < decls->declarations->length; d++) {
        ASTNode* decl = decls->declarations->value.p[d];
        if (decl->op != AST_OP(vardecl)) {
          continue;
        }
        Symbol* sym = ((VariableDeclarationASTNode*)decl)->symbol;
        if (!SymbolHasTrailingDestructor(node, trailing_start, sym)) {
          continue;
        }
        if (start_label == NULL) {
          start_label = GeneratorEmit(gen, NewIR(IR_OP(label)));
        }
        OpenCleanup* oc = malloc(sizeof(OpenCleanup));
        oc->start_label = start_label;
        oc->symbol = sym;
        VectorAppend(&open, oc);
      }
    }
  }
  // No trailing local destructors closed the scope (e.g. a constructor with only
  // member cleanups): position scope_end at the block end.
  if (do_cleanup && !scope_end_emitted) {
    GeneratorEmit(gen, scope_end);
    scope_end_emitted = true;
    ScheduleOpenLocalCleanups(gen, node, trailing_start, &open, scope_end);
  }
  VectorDestructWithContents(&open, NULL, /*free_element=*/true);

  // Restore stack pointer to value saved before topmost VLA was allocated.
  for (size_t i = 0; i < num_statements; i++) {
    ASTNode* stmt = node->statements->value.p[i];
    if (stmt->op == AST_OP(decl_list)) {
      DeclarationListASTNode* decl_list = (DeclarationListASTNode*)stmt;
      IRNode* vla = ContainsVLA(decl_list);
      if (vla != NULL) {
        GeneratorEmit(gen, NewIR1(IR_OP(restoresp), vla));
        break;
      }
    }
  }
}

// Is the statement just a branch (possibly enclosed in a compound).
// Return the branch if it is, NULL otherwise;
static ASTNode* CheckSingleBranch(ASTNode* stmt, ASTOpcode opcode) {
  if (stmt == NULL) {
    return NULL;
  }
  if (OptLevel0()) {
    return NULL;
  }
  if (stmt->op == AST_OP(compound)) {
    CompoundStatementASTNode* c = (CompoundStatementASTNode*)stmt;
    if (c->statements->length >= 1) {
      stmt = c->statements->value.p[0];
    }
  }
  if (stmt->op == opcode) {
    return stmt;
  }
  
  return NULL;
}


// Does the statement subtree contain a label, case or default?  Such labels
// are valid goto/switch targets, so a branch that looks dead (e.g. the body of
// `if (0)`) is actually reachable and its code must still be generated.
static bool StatementContainsLabel(ASTNode* node) {
  if (node == NULL) {
    return false;
  }
  switch (node->op) {
    case AST_OP(label):
    case AST_OP(case):
      return true;
    case AST_OP(compound): {
      CompoundStatementASTNode* c = (CompoundStatementASTNode*)node;
      for (size_t i = 0; i < c->statements->length; i++) {
        if (StatementContainsLabel(c->statements->value.p[i])) {
          return true;
        }
      }
      return false;
    }
    case AST_OP(if): {
      IfStatementASTNode* n = (IfStatementASTNode*)node;
      return StatementContainsLabel(n->if_part) ||
             StatementContainsLabel(n->else_part);
    }
    case AST_OP(while):
    case AST_OP(do):
      return StatementContainsLabel(((CombinedStatementASTNode*)node)->stmt);
    case AST_OP(for):
      return StatementContainsLabel(((ForStatementASTNode*)node)->stmt);
    case AST_OP(switch):
      return StatementContainsLabel(((SwitchStatementASTNode*)node)->stmt);
    case AST_OP(try): {
      TryASTNode* t = (TryASTNode*)node;
      if (StatementContainsLabel(t->try_stmt)) {
        return true;
      }
      for (size_t i = 0; i < t->catches->length; i++) {
        if (StatementContainsLabel(t->catches->value.p[i])) {
          return true;
        }
      }
      return false;
    }
    case AST_OP(catch):
      return StatementContainsLabel(((CatchASTNode*)node)->stmt);
    default:
      return false;
  }
}

static void GenerateIfStatement(Generator* gen, IfStatementASTNode* node) {
  // If the condition is a constant we can omit the expression,
  // comparison and the statement as appropriate -- but only when the dead arm
  // contains no labels.  A label inside the dead arm is a valid goto/switch
  // target, so that code is reachable and must still be generated (otherwise
  // the label is dropped while branches to it survive, dangling the target).
  if (ASTNodeIsIntConstant(node->cond) &&
      !StatementContainsLabel(node->if_part) &&
      !StatementContainsLabel(node->else_part)) {
    ConstantASTNode* c = (ConstantASTNode*)node->cond;
    if (c->value.ivalue != 0) {
      GenerateStatement(gen, node->if_part);
    } else {
      if (node->else_part != NULL) {
        GenerateStatement(gen, node->else_part);
      }
    }
    return;
  }
  
  // cond
  IRNode* cond = GenerateExpression(gen, node->cond);
  IRNode* else_label = NewIR(IR_OP(label));
  IRNode* end_label = NULL;
  if (node->else_part != NULL) {
     end_label = NewIR(IR_OP(label));
  }
  
  // Optimize branches over single break, continue, goto
  // instructions.
  // We want to avoid a condition branch over a branch.
  //
  // Say we have (as is common):
  // if (cond) {
  //   break;
  // }
  // The naive way to do this is:
  // cond
  // bfalse end_label
  // bra break_label
  // end_label:
  //
  // But it's more efficient to generate:
  // cond
  // btrue break_label
  //
  // Likewise for continue and goto.
  ASTNode* break_stmt = CheckSingleBranch(node->if_part, AST_OP(break));
  ASTNode* continue_stmt = CheckSingleBranch(node->if_part, AST_OP(continue));
  ASTNode* goto_stmt = CheckSingleBranch(node->if_part, AST_OP(goto));
  if (break_stmt != NULL) {
    // if (cond) { break; } -> if(cond) goto break_label;
    // btrue cond, break_label
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, gen->break_label));
  } else if (continue_stmt != NULL) {
      // if (cond) { continue; } -> if(cond) goto continue_label;
      // btrue cond, continue_label
      GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, gen->continue_label));
  } else if (goto_stmt != NULL) {
    GotoStatementASTNode* go = (GotoStatementASTNode*)goto_stmt;
    LabelASTNode* label_node = (LabelASTNode*)go->label;
    if (label_node->label == NULL) {
      label_node->label = NewIR(IR_OP(label));
    }
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, label_node->label));
  } else {
    // Regular if (cond) stmt;
    // bfalse cond, else_label
    GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, else_label));

    // if_part
    GenerateStatement(gen, node->if_part);
    
    if (end_label != NULL) {
      // bra end_label
      GeneratorEmit(gen, NewIR1(IR_OP(bra), end_label));
    }
  }

  if (node->else_part != NULL) {
    // else_label:
    GeneratorEmit(gen, else_label);

    // else_part
    GenerateStatement(gen, node->else_part);

    // end:
    GeneratorEmit(gen, end_label);
  } else {
    // else_label:
    GeneratorEmit(gen, else_label);
  }
}

static void GenerateTryStatement(Generator* gen, TryASTNode* node) {
  if (!TryStatementHasSupportedHandler(node)) {
    GenerateStatement(gen, node->try_stmt);
    return;
  }

  IRNode* try_start = NewIR(IR_OP(label));
  IRNode* try_end = NewIR(IR_OP(label));
  IRNode* after_try = NewIR(IR_OP(label));
  Vector catch_labels;
  Vector catch_typeinfos;
  VectorInit(&catch_labels);
  VectorInit(&catch_typeinfos);

  for (size_t i = 0; i < node->catches->length; i++) {
    CatchASTNode* handler = node->catches->value.p[i];
    if (!IsSupportedCatchHandler(handler)) {
      continue;
    }
    VectorAppend(&catch_labels, NewIR(IR_OP(label)));
    VectorAppend(&catch_typeinfos, CatchHandlerTypeInfo(gen, handler));
  }

  GeneratorEmit(gen, try_start);
  GenerateStatement(gen, node->try_stmt);
  GeneratorEmit(gen, try_end);
  // Automatic objects declared in the try body are destroyed on the exception
  // path by the generic per-object cleanup ranges (see GenerateCompoundStatement
  // and libc/eh_throw.c): the runtime runs those inner cleanups before it enters
  // the handler recorded here, and only for the objects actually constructed.
  for (size_t i = 0; i < catch_labels.length; i++) {
    RecordExceptionRange(gen, try_start, try_end, catch_labels.value.p[i],
                         catch_typeinfos.value.p[i]);
  }
  if (StatementMayFallThrough(node->try_stmt)) {
    GeneratorEmit(gen, NewIR1(IR_OP(bra), after_try));
  }

  size_t catch_index = 0;
  for (size_t i = 0; i < node->catches->length; i++) {
    CatchASTNode* handler = node->catches->value.p[i];
    if (!IsSupportedCatchHandler(handler)) {
      continue;
    }
    IRNode* catch_label = catch_labels.value.p[catch_index++];
    GeneratorEmit(gen, catch_label);
    GenerateCatchBinding(gen, handler);
    GenerateStatement(gen, handler->stmt);
    if (StatementMayFallThrough(handler->stmt)) {
      GeneratorEmit(gen, NewIR1(IR_OP(bra), after_try));
    }
  }
  GeneratorEmit(gen, after_try);
  VectorDestruct(&catch_labels);
  VectorDestruct(&catch_typeinfos);
}

// Unless we are generating code for size:
// Most processors predict that a conditional branch will be
// taken.  A while loop has a conditional branch at the start
// that will be be predicted as taken but will not be taken
// on every loop iteration.  It is better to convert the
// while loop into:
//
// if (cond) {
//   do {
//   ...
//   } while (cond);
// }
static void GenerateWhileStatement(Generator* gen,
                                   CombinedStatementASTNode* node) {
  // Check for constant loop condition.
  ConstantASTNode* const_cond = NULL;
  if (OptLevel1() && ASTNodeIsIntConstant(node->cond)) {
    const_cond = (ConstantASTNode*)node->cond;
    if (const_cond->value.ivalue == 0) {
      // This is while(false), omit the whole statement.
      return;
    }
  }
  IRNode* old_break = gen->break_label;
  IRNode* old_continue = gen->continue_label;

  gen->break_label = NewIR(IR_OP(label));
  gen->continue_label = NewIR(IR_OP(label));
 
  switch (compiler->code_preference) {
    case kCodeForSize: {
      // Preference is for size.  Generate the traditional loop:
      // continue_label:
      // bfalse cond, break_label
      // ...
      // bra continue_label
      // break_label:
      GeneratorEmit(gen, gen->continue_label);
      IRNode* cond = GenerateExpression(gen, node->cond);

      // bfalse cond, break_label
      GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));

      // stmt
      GenerateStatement(gen, node->stmt);
      GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));
      break;
    }
    case kCodeForSpeed: {
      // If we have a constant condition at this point it is non-zero
      // so this is a forever loop.
      if (const_cond == NULL) {
        IRNode* cond = GenerateExpression(gen, node->cond);

        // bfalse cond, break_label
        GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
      }
     
      // continue_label:
      GeneratorEmit(gen, gen->continue_label);

      // stmt
      GenerateStatement(gen, node->stmt);

      if (const_cond == NULL) {
        // if (cond) goto continue_label.
        IRNode* cond = GenerateExpression(gen, node->cond);
        GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, gen->continue_label));
      } else {
        // bra continue_label
        GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));
      }
      break;
    }
  }
  
  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
  gen->continue_label = old_continue;
}

static void GenerateDoStatement(Generator* gen,
                                CombinedStatementASTNode* node) {
  IRNode* old_break = gen->break_label;
  IRNode* old_continue = gen->continue_label;

  gen->break_label = NewIR(IR_OP(label));
  gen->continue_label = NewIR(IR_OP(label));

  // loop_label:
  IRNode* loop_label = GeneratorEmit(gen, NewIR(IR_OP(label)));
 
  // stmt
  GenerateStatement(gen, node->stmt);

  // continue_label:
  GeneratorEmit(gen, gen->continue_label);

  if (OptLevel1() && ASTNodeIsIntConstant(node->cond)) {
    ConstantASTNode* c = (ConstantASTNode*)node->cond;
    // do ... while(constant);
    if (c->value.ivalue != 0) {
      // do .. while(true) - always loop.
      GeneratorEmit(gen, NewIR1(IR_OP(bra), loop_label));
    } else {
      // do ... while(false) - no loop back
    }
  } else {
    // cond
    IRNode* cond = GenerateExpression(gen, node->cond);
    
    // btrue cond, loop_label
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, loop_label));
  }

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
  gen->continue_label = old_continue;
}

// Generate IR for a dense switch statement.  This is generated as a branch
// table.  This is used when the case values are in a range that are suitable
// for coding as a branch table; that is, they have a decent density.
static void GenerateDenseSwitch(Generator* gen, SwitchStatementASTNode* node) {
  IRNode* old_break = gen->break_label;

  gen->break_label = NewIR(IR_OP(label));

  IRNode* expr = GenerateExpression(gen, node->expr);

  // Set default label as either the break label or the defined default label.
  IRNode* default_label =
      node->default_node != NULL ? node->default_node->label : gen->break_label;

  IRNode* min =
      GeneratorGetIntConstant(gen, node->expr->type, node->min_case_value);
  IRNode* max =
      GeneratorGetIntConstant(gen, node->expr->type, node->max_case_value);

  if (!node->all_cases_covered) {
    // If all cases are positive we can generate a negative comparison and
    // branch to default if true.  We can then use an unsigned comparison
    // for the values, which is faster on some processors.
    if (TypeIsSigned(node->expr->type) && node->all_cases_positive) {
      IRNode* cmp0 = GeneratorEmit(gen, NewIR2(IR_OP(cmplti), expr,
                                               GeneratorGetIntConstant(gen, node->expr->type, 0)));
      GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmp0, default_label));
      
      // Use an unsigned comparison by resetting the node's type.
      TypeRecord* unsigned_type = TypeRecordCopy(expr->type);
      unsigned_type->type |= kTypeUnsigned;
      IRSetType(expr, unsigned_type);
      expr->flags |= kIRFakeUnsigned;     // This isn't really unsigned.
    }
    // Compare expr to min and branch to default if less.
    IRNode* cmplo = GeneratorEmit(gen, NewIR2(IR_OP(cmplti), expr, min));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmplo, default_label));

    // Compare expr to max and branch to default if greater.
    IRNode* cmphi = GeneratorEmit(gen, NewIR2(IR_OP(cmpgti), expr, max));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), cmphi, default_label));
  }
  
  // Subtract min from expr to get branch offset, unless min is zero (no
  // point in subtracting zero).
  IRNode* zeroed = expr;
  if (node->min_case_value != 0) {
    zeroed = GeneratorEmit(gen, NewIR2(IR_OP(subi), expr, min));
  }

  // Computed branch via jump table.
  GeneratorEmit(gen, NewIR1(IR_OP(cbra), zeroed));

  // Generate dense branch table with branches to default filling in empty slots
  // and branches to the case labels for those with case values.
  int64_t next_value = node->min_case_value;
  for (size_t i = 0; i < node->cases.length; i++) {
    CaseLabelASTNode* case_node = node->cases.value.p[i];
    if (next_value != case_node->value) {
      // Fill gap in branch table with branches to the default label.
      do {
        IRNode* bra =GeneratorEmit(gen, NewIR1(IR_OP(bra), default_label));
        bra->flags |= kIRJumpTableBranch;
        next_value++;
      } while (next_value != case_node->value);
    }
    IRNode* bra = GeneratorEmit(gen, NewIR1(IR_OP(bra), case_node->label));
    bra->flags |= kIRJumpTableBranch;
    next_value++;
  }

  // Switch statement body.  This includes all the case labels and default (if
  // present). These will emit their labels when they are generated.
  GenerateStatement(gen, node->stmt);

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
}

static void GenerateBinaryCaseSearch(Generator* gen,
                                     SwitchStatementASTNode* node,
                                     IRNode* expr,
                                     IRNode* default_label,
                                     size_t start,
                                     size_t end) {
  size_t length = end - start;
  if (length > 15) {
    // More than 15 cases, split search into lower and upper halfs.
    IRNode* lower_half = NewIR(IR_OP(label));
    size_t mid = start + length/2;
    CaseLabelASTNode* mid_case_node = node->cases.value.p[mid];
    IRNode* compare =
          GeneratorEmit(gen, NewIR2(IR_OP(cmplti), expr,
                              GeneratorGetIntConstant(gen, node->expr->type,
                                                      mid_case_node->value)));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), compare, lower_half));
    
    // Search upper half
    GenerateBinaryCaseSearch(gen, node, expr, default_label, mid, end);
    
    // Search lower half.
    GeneratorEmit(gen, lower_half);
    GenerateBinaryCaseSearch(gen, node, expr, default_label, start, mid);
    return;
  }
  // 15 cases or less, use linear search.
  for (size_t i = start; i < end; i++) {
    CaseLabelASTNode* case_node = node->cases.value.p[i];
    IRNode* compare =
    GeneratorEmit(gen, NewIR2(IR_OP(cmpeqi), expr,
                              GeneratorGetIntConstant(gen, node->expr->type,
                                                      case_node->value)));
    GeneratorEmit(gen, NewIR2(IR_OP(btrue), compare, case_node->label));
  }
  if (!node->all_cases_covered) {
    GeneratorEmit(gen, NewIR1(IR_OP(bra), default_label));
  }
}

// Generate IR for a switch statement using a sparse comparison coding.  This
// compares each case value in turn and branches to the appropriate label.
static void GenerateSparseSwitch(Generator* gen, SwitchStatementASTNode* node) {
  IRNode* old_break = gen->break_label;

  gen->break_label = NewIR(IR_OP(label));

  IRNode* expr = GenerateExpression(gen, node->expr);

  // Set default label as either the break label or the defined default label.
  IRNode* default_label =
      node->default_node != NULL ? node->default_node->label : gen->break_label;

  // Generate sequence of comparisons using a binary search through the
  // cases.
  GenerateBinaryCaseSearch(gen, node, expr,
                          default_label, 0, node->cases.length);
  
  // Switch statement body.  This includes all the case labels and default (if
  // present). These will emit their labels when they are generated.
  GenerateStatement(gen, node->stmt);

  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
}

// A constant switch is generated as a branch to a label inside
// the switch body.  What we really want is to simply generate
// the code for the located case label but that is inside a
// statement that is difficult to traverse.  So instead we
// generate a single branch to a label and rely on the
// basic block analysis to remove any unreachable code.
static void GenerateConstantSwitch(Generator* gen,
                                   SwitchStatementASTNode* node) {
  ConstantASTNode* value_node = (ConstantASTNode*)node->expr;
  int64_t value = value_node->value.ivalue;
  
  // Look for the case statement that matches the value and create
  // a label for it.
  CaseLabelASTNode* found_case = NULL;
  for (size_t i = 0; i < node->cases.length; i++) {
    CaseLabelASTNode* case_node = (CaseLabelASTNode*)node->cases.value.p[i];
    if (case_node->value == value) {
      found_case = case_node;
      break;
    }
  }
  
  if (found_case != NULL) {
    IRNode* label = NewIR(IR_OP(label));
    found_case->label = label;
    // Generate an unconditional branch to the label.
    GeneratorEmit(gen, NewIR1(IR_OP(bra), label));
  } else {
    // No case found, look for default.
    if (node->default_node == NULL) {
      // No default, no statement is possible.
      return;
    }
    IRNode* label = NewIR(IR_OP(label));
    node->default_node->label = label;
    // Generate an unconditional branch to the label.
    GeneratorEmit(gen, NewIR1(IR_OP(bra), label));
  }
  
  // There will be one label defined in the switch statement
  // body.  We now generate the code for all the cases and
  // anything that is unreachable will be eliminated later.
  IRNode* old_break = gen->break_label;
  
  gen->break_label = NewIR(IR_OP(label));

  GenerateStatement(gen, node->stmt);
  
  // break_label:
  GeneratorEmit(gen, gen->break_label);
  
  gen->break_label = old_break;
}
  
// Switch statement IR generation.
static void GenerateSwitchStatement(Generator* gen,
                                    SwitchStatementASTNode* node) {
  if (OptLevel1() && ASTNodeIsIntConstant(node->expr)) {
    // Constant switch expression.  Only generate code for the case
    // that matches.
    GenerateConstantSwitch(gen, node);
    return;
  }

  // Create case labels.
  for (size_t i = 0; i < node->cases.length; i++) {
    IRNode* label = NewIR(IR_OP(label));
    CaseLabelASTNode* case_node = (CaseLabelASTNode*)node->cases.value.p[i];
    case_node->label = label;
  }

  // Default label.
  if (node->default_node != NULL) {
    IRNode* label = NewIR(IR_OP(label));
    CaseLabelASTNode* default_node = (CaseLabelASTNode*)node->default_node;
    default_node->label = label;
  }

  // A dense switch is only good if the comparisons to set it up do
  // not exceed the advantage of the jump table.
  int min_dense_cases = 4;     // TODO: configure this per target.
  if (!node->all_cases_covered) {
    min_dense_cases += 2;     // Two extra comparisons.
    if (TypeIsSigned(node->expr->type) && node->all_cases_positive) {
      min_dense_cases += 1;     // One extra comparison.
    }
  }
  // Use density calculated from semantic analysis to determine what
  // type of switch to generate.
  //
  // The x86_64 back-end does not yet implement the computed-branch jump table
  // (its instructions are variable length, so the fixed-stride inline jump
  // table used by the RISC-V/AArch64 back-ends does not apply).  Always use a
  // sparse comparison search there instead.
  bool target_has_jump_table =
      !StringEqual(&compiler->target->name, "x86-64");
  if (target_has_jump_table && node->cases.length > min_dense_cases &&
      node->density > 0.5) {
    GenerateDenseSwitch(gen, node);
  } else {
    GenerateSparseSwitch(gen, node);
  }
}

// Like a while loop, convert it into a tail condition loop with
// an enclosing if statement.
static void GenerateForStatement(Generator* gen, ForStatementASTNode* node) {
  IRNode* old_break = gen->break_label;
  IRNode* old_continue = gen->continue_label;

  gen->break_label = NewIR(IR_OP(label));
  gen->continue_label = NewIR(IR_OP(label));
  IRNode* loop_label = NewIR(IR_OP(label));

  // Initial expression (e1).
  if (node->c1 != NULL) {
    if (node->c1->op == AST_OP(decl_list)) {
      GenerateStatement(gen, node->c1);
    } else {
      GenerateExpression(gen, node->c1);
    }
  }

  // Check for constant condition.
  bool constant_condition = false;
  if (node->c2 != NULL) {
    if (OptLevel1() && ASTNodeIsIntConstant(node->c2)) {
      ConstantASTNode* c = (ConstantASTNode*)node->c2;
      if (c->value.ivalue == 0) {
        // Condition is false, omit whole statement as it will never
        // be executed
        gen->break_label = old_break;
        gen->continue_label = old_continue;
        return;
      }
      constant_condition = true;
    }
  }
  
  switch (compiler->code_preference) {
    case kCodeForSize:
      // loop_label:
      GeneratorEmit(gen, loop_label);
      
      if (node->c2 != NULL && !constant_condition) {
        IRNode* cond = GenerateExpression(gen, node->c2);
        // bfalse cond, break_label
        GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
      }
    
      // stmt
      GenerateStatement(gen, node->stmt);

      // Continue label.
      GeneratorEmit(gen, gen->continue_label);

      if (node->c3 != NULL) {
        GenerateExpression(gen, node->c3);
      }
      GeneratorEmit(gen, NewIR1(IR_OP(bra), loop_label));
      break;
      
    case kCodeForSpeed:
      // if (!cond) goto break_label
      if (node->c2 != NULL && !constant_condition) {
        IRNode* cond = GenerateExpression(gen, node->c2);
        // bfalse cond, break_label
        GeneratorEmit(gen, NewIR2(IR_OP(bfalse), cond, gen->break_label));
      }
      
      // loop_label:
      GeneratorEmit(gen, loop_label);


      // stmt
      GenerateStatement(gen, node->stmt);

      // Continue label.
      GeneratorEmit(gen, gen->continue_label);

      if (node->c3 != NULL) {
        GenerateExpression(gen, node->c3);
      }

      if (node->c2 != NULL && !constant_condition) {
        IRNode* cond = GenerateExpression(gen, node->c2);
        // btrue cond, loop_label
        GeneratorEmit(gen, NewIR2(IR_OP(btrue), cond, loop_label));
      } else {
        // bra loop_label
        GeneratorEmit(gen, NewIR1(IR_OP(bra), loop_label));
      }
      break;
    
  }
  
  // break_label:
  GeneratorEmit(gen, gen->break_label);

  gen->break_label = old_break;
  gen->continue_label = old_continue;
}

static void GenerateReturnStatement(Generator* gen,
                                    CombinedStatementASTNode* node) {
  IRNode* nrvo_expr = NULL;
  // For a scalar/reference return the value must survive the scope-exit
  // destructors (a destructor call would otherwise clobber the return
  // register), so the `result` IR that places it is deferred until after the
  // destructors run.  A struct return is copied into the return slot (memory)
  // first, which is safe across the destructor calls.
  IROpcode deferred_result_op = 0;
  IRNode* deferred_result_value = NULL;
  bool have_deferred_result = false;
  bool deferred_result_needs_reload = false;
  TypeRecord* deferred_reload_type = NULL;
  if (node->cond != NULL) {
    if (node->cond->op == AST_OP(asm)) {
      // Extension: return asm(..)
      GenerateStatement(gen, node->cond);
    } else {
      ASTNode* elidable =
          CXXElidableStructReturnInitializer(node->cond, gen->func->next);
      if (elidable != NULL) {
        IRNode* old_struct_address = gen->current_struct_address;
        gen->current_struct_address = gen->struct_return_value;
        IRNode* direct_result = GenerateExpression(gen, elidable);
        direct_result->flags |= kIRRvoCall;
        gen->current_struct_address = old_struct_address;
        goto emit_return_branch;
      }
      bool returns_reference = TypeIsReference(gen->func->next);
      int old_cond_flags = node->cond->flags;
      if (returns_reference) {
        node->cond->flags |= kASTNeedAddress;
      }
      IRNode* expr = GenerateExpression(gen, node->cond);
      node->cond->flags = old_cond_flags;
      // When scope-exit destructors run between here and the branch, the
      // scalar/reference return value must survive those calls.  The register
      // allocator does not keep it live across the `result` opcode, so spill it
      // to a stack temporary now and reload it after the destructors.
      bool spill_for_cleanup = node->stmt != NULL;
      if (returns_reference) {
        if (spill_for_cleanup) {
          TypeRecord* addr_type = NewPointerTo(kQualPlain, gen->func->next);
          deferred_result_value =
              GeneratorSpillValueToTemp(gen, expr, addr_type);
          deferred_result_needs_reload = true;
          deferred_reload_type = addr_type;
        } else {
          deferred_result_value = expr;
        }
        deferred_result_op = IR_OP(resulta);
        have_deferred_result = true;
      } else if (TypeIsStructOrUnion(node->cond->type)) {
        if ((node->cond->flags & kASTRvoCall) != 0) {
          // An RVO call is passed the structresult directly from the
          // current function so there's no need to copy the result.
        } else if ((node->cond->flags & kASTNrvoMarker) != 0) {
          // Named RVO, nothing to do.
          nrvo_expr = expr;
          // GeneratorEmit(gen, NewIR1(IR_OP(nrvoval), nrvo_expr));
        } else {
          // Returning a struct, copy result to return value.
          expr = GeneratorEmit(gen, NewIR1(IR_OP(addressof), expr));
          CheckForVarUse(expr, node->cond);
          IRNode* result = GeneratorEmit(gen, NewIR3(IR_OP(memcpy), gen->struct_return_value, expr,
                                    GeneratorGetIntConstant(
                                        gen, NULL, node->cond->type->size)));
          CheckForVarDef(result, &node->base);
        }
      } else {
        IROpcode result;
        if (TypeIsIntegral(node->cond->type)) {
          result = IR_OP(resulti);
        } else if (TypeIsFloat(node->cond->type)) {
          result = IR_OP(resultf);
        } else if (TypeIsDouble(node->cond->type) ||
                   TypeIsLongDouble(node->cond->type)) {
          result = IR_OP(resultd);
        } else {
          result = IR_OP(resulta);
        }
        deferred_result_op = result;
        if (spill_for_cleanup) {
          deferred_result_value =
              GeneratorSpillValueToTemp(gen, expr, node->cond->type);
          deferred_result_needs_reload = true;
          deferred_reload_type = node->cond->type;
        } else {
          deferred_result_value = expr;
        }
        have_deferred_result = true;
      }
    }
  }

emit_return_branch:
  // Run the C++ scope-exit destructors for automatic objects going out of
  // scope (attached to the return's `stmt` child by the semantic analyzer).
  // They run after the return value has been materialised but before the
  // deferred result register write and the branch to the epilogue.
  if (node->stmt != NULL) {
    GenerateStatement(gen, node->stmt);
  }
  if (have_deferred_result) {
    IRNode* result_value = deferred_result_value;
    if (deferred_result_needs_reload) {
      result_value = GeneratorReloadSpilledValue(gen, deferred_result_value,
                                                 deferred_reload_type);
    }
    GeneratorEmit(gen, NewIR1(deferred_result_op, result_value));
  }
  // We don't explictly do the return here because the code
  // sequence can be large (restoring saved registers, etc).
  // So instead, we branch to the first return in the function.
  IRNode* return_label = GeneratorGetReturnLabel(gen);
  IRNode* branch = GeneratorEmit(gen, NewIR1(IR_OP(bra), return_label));
  
  // Help out the lower code generators.
  branch->flags |= kIRReturnJump;
  if (nrvo_expr != NULL) {
    branch->flags |= kIRNrvoMarker;
  }
}

static void GenerateCaseLabel(Generator* gen, CaseLabelASTNode* node) {
  if (node->label == NULL) {
    return;
  }
  GeneratorEmit(gen, node->label);
  GenerateStatement(gen, node->stmt);
}

// The semantic analyzer sets the 'stmt' field of the CombinedStatementASTNode
// to the AST node of the referenced label.  This may be before or after the
// goto statement has been generated.  If the label node has an IRNode already
// assigned we generate a branch to it, otherwise we create one for it.
static void GenerateGotoStatement(Generator* gen,
                                  GotoStatementASTNode* node) {
  LabelASTNode* label_node = (LabelASTNode*)node->label;
  if (label_node->label == NULL) {
    label_node->label = NewIR(IR_OP(label));
  }
  assert(node->lca != NULL);      // Need a Lowest Common Ancestor set.
  IRNode* top_vla = FindTopVLAForJump(&node->base, node->lca);
  if (top_vla != NULL) {
    GeneratorEmit(gen, NewIR1(IR_OP(restoresp), top_vla));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), label_node->label));
}

static void GenerateLabel(Generator* gen, LabelASTNode* node) {
  // If the label has not already been generated (by the goto) generate
  // one now.
  if (node->label == NULL) {
    if (node->named) {
      node->label = NewIRNamedLabel(node->name.value);
    } else {
      node->label = NewIR(IR_OP(label));
    }
  }

  // Emit label.
  GeneratorEmit(gen, node->label);
  
  // Emit label statement.
  GenerateStatement(gen, node->stmt);
}

static void GenerateBreak(Generator* gen, ASTNode* node) {
  ASTNode* loop_or_switch = FindEnclosingLoopOrSwitch(node);
  assert(loop_or_switch != NULL);
  IRNode* top_vla = FindTopVLAForJump(node, loop_or_switch);
  if (top_vla != NULL) {
    GeneratorEmit(gen, NewIR1(IR_OP(restoresp), top_vla));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->break_label));
}

static void GenerateContinue(Generator* gen, ASTNode* node) {
  ASTNode* loop = FindEnclosingLoop(node);
  assert(loop != NULL);
  IRNode* top_vla = FindTopVLAForJump(node, loop);
  if (top_vla != NULL) {
    GeneratorEmit(gen, NewIR1(IR_OP(restoresp), top_vla));
  }
  GeneratorEmit(gen, NewIR1(IR_OP(bra), gen->continue_label));
}

// Assembly language IR node.  This refers to a string literal.
static void GenerateAsm(Generator* gen, AsmASTNode* node) {
  int literal_id = CompilerAddStringLiteral(node->text, false);
  IRNode* literal = GeneratorGetIntConstant(gen, NULL, literal_id);
  Vector inputs;
  VectorInit(&inputs);
  for (size_t i = 0; i < node->outputs.length; i++) {
    AsmOperand* operand = node->outputs.value.p[i];
    int old_flags = operand->expr->flags;
    operand->expr->flags |= kASTNeedAddress;
    VectorAppend(&inputs, GenerateExpression(gen, operand->expr));
    operand->expr->flags = old_flags;
  }
  for (size_t i = 0; i < node->inputs.length; i++) {
    AsmOperand* operand = node->inputs.value.p[i];
    VectorAppend(&inputs, GenerateExpression(gen, operand->expr));
  }
  IRNode* asm_ir = GeneratorEmit(gen, NewIR(IR_OP(asm)));
  asm_ir->aux = node;
  VectorAppend(&asm_ir->inputs, literal);
  VectorAppend(&literal->outputs, asm_ir);
  for (size_t i = 0; i < inputs.length; i++) {
    IRNode* input = inputs.value.p[i];
    VectorAppend(&asm_ir->inputs, input);
    VectorAppend(&input->outputs, asm_ir);
  }
  VectorDestruct(&inputs);
  if (node->is_goto && node->label_nodes.length != 0) {
    LabelASTNode* label = node->label_nodes.value.p[0];
    if (label->label == NULL) {
      label->label = label->named ? NewIRNamedLabel(label->name.value)
                                  : NewIR(IR_OP(label));
    }
    IRNode* branch = GeneratorEmit(gen, NewIR(IR_OP(bra)));
    VectorAppend(&branch->inputs, label->label);
    VectorAppend(&label->label->outputs, branch);
  }
}

void GenerateStatement(Generator* gen, ASTNode* node) {
  if (node == NULL) {
    return;
  }

  IRSetLocation(node->location);
  
  // Emit location instructions if the statement will generate code.
  if (compiler->debug_output) {
    bool emit_loc = true;
    switch (node->op) {
      case AST_OP(decl_list):
        emit_loc = false;
        break;
      case AST_OP(vardecl): {
        // If the variable declaration doesn't have an initializer there is
        // no location,
        VariableDeclarationASTNode* decl = (VariableDeclarationASTNode*)node;
        if (decl->initializer == NULL) {
          emit_loc = false;
        }
        break;
      }
      case AST_OP(compound):
      case AST_OP(try):
      case AST_OP(catch):
        // A compound statement contains statements, so there is no code for
        // itself.
        emit_loc = false;
        break;
      case AST_OP(label):
        emit_loc = false;
        break;
      default:
        break;
    }

    if (emit_loc) {
      GeneratorEmit(gen, NewIRLocation(node->location));
    }
  }

  switch (node->op) {
  case AST_OP(decl_list):
    GenerateDeclarationList(gen, (DeclarationListASTNode*)node);
    break;
  case AST_OP(vardecl):
    GenerateVariableDeclaration(gen, (VariableDeclarationASTNode*)node);
    break;
  case AST_OP(expr):
    GenerateExpressionStatement(gen, (ExpressionStatementASTNode*)node);
    break;
  case AST_OP(compound):
    GenerateCompoundStatement(gen, (CompoundStatementASTNode*)node);
    break;
  case AST_OP(try):
    GenerateTryStatement(gen, (TryASTNode*)node);
    break;
  case AST_OP(catch):
    break;
  case AST_OP(if):
    GenerateIfStatement(gen, (IfStatementASTNode*)node);
    break;
  case AST_OP(while):
    GenerateWhileStatement(gen, (CombinedStatementASTNode*)node);
    break;
  case AST_OP(do):
    GenerateDoStatement(gen, (CombinedStatementASTNode*)node);
    break;
  case AST_OP(switch):
    GenerateSwitchStatement(gen, (SwitchStatementASTNode*)node);
    break;
  case AST_OP(for):
    GenerateForStatement(gen, (ForStatementASTNode*)node);
    break;
  case AST_OP(return ):
    GenerateReturnStatement(gen, (CombinedStatementASTNode*)node);
    break;
  case AST_OP(case):
    GenerateCaseLabel(gen, (CaseLabelASTNode*)node);
    break;
  case AST_OP(goto):
    GenerateGotoStatement(gen, (GotoStatementASTNode*)node);
    break;
  case AST_OP(label):
    GenerateLabel(gen, (LabelASTNode*)node);
    break;
  case AST_OP(break):
    GenerateBreak(gen, node);
    break;
  case AST_OP(continue):
    GenerateContinue(gen, node);
    break;
  case AST_OP(asm):
    GenerateAsm(gen, (AsmASTNode*)node);
    break;
  case AST_OP(static_assert):
    // Checked during semantic analysis; no runtime code is emitted.
    break;
  default:
    assert(false);
  }
}
