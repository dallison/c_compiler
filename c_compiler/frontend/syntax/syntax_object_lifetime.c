//
//  syntax_object_lifetime.c
//  c_compiler
//
//  Object lifetime: destructor calls, guarded local statics, thread_local
//  initialization and destruction, atexit registration, init/fini
//  attributes and array construction cleanup.
//

#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "concepts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "macro_expansion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_internal.h"
#include "type_member.h"
#include "type_special_member.h"
#include "errors.h"
#include "compiler.h"
#include "module_identity.h"
#include "module_syntax.h"
#include "reflection_semantics.h"
#include "syntax_internal.h"

// Builds an expression statement that calls the class destructor for
// `type` on `receiver`, or NULL when C++ is disabled, inputs are invalid,
// or the type has no usable destructor member.
static ASTNode* NewCXXDestructorCallOnReceiver(ASTNode* receiver,
                                               TypeRecord* type,
                                               SourceLocation location) {
  if (!CompilerIsCXX() || receiver == NULL || type == NULL ||
      !TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      !destructor->symbol->type->info.function.is_destructor) {
    StringDestruct(&destructor_name);
    return NULL;
  }
  ASTNode* member =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(type, actuals,
                                   /*complete_object=*/true, location);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

// Returns a destructor call statement for `sym`, or NULL when its type
// needs no destruction or the symbol is not a destructible class object.
static ASTNode* NewCXXDestructorCallIfNeeded(Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL ||
      StorageIs(sym->storage, STO(typedef)) ||
      !TypeIsStructOrUnion(sym->type) ||
      sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return NewCXXDestructorCallOnReceiver(NewIdentifierASTNode(sym, sym->location),
                                        sym->type, sym->location);
}

// Returns an assignment of integer `value` to `sym` at `location`.
static ASTNode* NewIntAssignment(Symbol* sym, int value,
                                 SourceLocation location) {
  return NewBinaryASTNode(
      AST_OP(assign), sym->type, location,
      NewIdentifierASTNode(sym, location),
      NewIntConstantASTNode(value,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
}

// True when `func` is an inline or weak-binding function whose local static
// objects may be merged across translation units and need unique asm names.
static bool FunctionHasSharedCXXLocalStatics(TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      func->info.function.symbol == NULL) {
    return false;
  }
  Symbol* symbol = func->info.function.symbol;
  return !StorageIs(symbol->storage, STO(static)) &&
         (func->info.function.is_inline || SymbolHasWeakBinding(symbol));
}

// Appends `component` to `out`, replacing non-alphanumeric characters with
// `_` plus two hex digits so the result is safe in linker symbol names.
static void AppendSanitizedAsmComponent(String* out, const char* component) {
  for (const unsigned char* p = (const unsigned char*)component; *p != '\0';
       p++) {
    if (isalnum(*p) || *p == '_') {
      StringAppendChar(out, (char)*p);
    } else {
      StringPrintf(out, "_%02x", *p);
    }
  }
}

// Sets the asm name of `sym` to a unique weak global name derived from the
// enclosing function, `local_name`, source location, and optional `suffix`.
static void SetCXXInlineLocalStaticAsmNameFor(TypeRecord* func, Symbol* sym,
                                              const char* local_name,
                                              SourceLocation location,
                                              const char* suffix) {
  if (!FunctionHasSharedCXXLocalStatics(func) || sym == NULL) {
    return;
  }

  Symbol* func_symbol = func->info.function.symbol;
  const char* func_name = SymbolAsmNameOrName(func_symbol);
  char fallback_func_name[64];
  if (func_name == NULL || func_name[0] == '\0') {
    const char* function_filename;
    int function_lineno;
    int function_start;
    int function_end;
    DecodeSourceLocation(func_symbol->location, &function_filename,
                         &function_lineno, &function_start, &function_end);
    (void)function_filename;
    (void)function_end;
    snprintf(fallback_func_name, sizeof(fallback_func_name),
             "anonymous_function_%d_%d", function_lineno, function_start);
    func_name = fallback_func_name;
  }
  if (local_name == NULL || local_name[0] == '\0') {
    local_name = "anonymous_local";
  }
  const char* filename;
  int lineno;
  int start;
  int end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);

  String asm_name = {0};
  StringInit(&asm_name, "__davecc_inline_static_");
  AppendSanitizedAsmComponent(&asm_name, func_name);
  StringAppendChar(&asm_name, '_');
  AppendSanitizedAsmComponent(&asm_name, local_name);
  StringPrintf(&asm_name, "_%d_%d", lineno, start);
  if (suffix != NULL && suffix[0] != '\0') {
    StringAppendChar(&asm_name, '_');
    AppendSanitizedAsmComponent(&asm_name, suffix);
  }

  SymbolSetAsmName(sym, &asm_name);
  StringDestruct(&asm_name);
  sym->flags.is_local = false;
  sym->flags.is_weak = true;
}

// Assigns a shared inline-local-static asm name to `sym` using the current
// function and the symbol's own name and location.
void SetCXXInlineLocalStaticAsmName(Symbol* sym, const char* suffix) {
  if (sym == NULL) {
    return;
  }
  SetCXXInlineLocalStaticAsmNameFor(compiler->current_function, sym,
                                    sym->name.value, sym->location, suffix);
}

// Names the guard symbol `guard` for the local static `guarded` within `func`.
static void SetCXXInlineLocalStaticGuardAsmName(TypeRecord* func, Symbol* guard,
                                                Symbol* guarded) {
  if (guard == NULL || guarded == NULL) {
    return;
  }
  SetCXXInlineLocalStaticAsmNameFor(func, guard, guarded->name.value,
                                    guarded->location, "guard");
}

// Queues a thread-local destructor for `sym` on `compiler`, run only when
// `guard` is non-zero, for non-local thread-local objects with destructors.
static void RegisterCXXThreadLocalDestructor(Symbol* sym, Symbol* guard) {
  ASTNode* destructor = NewCXXDestructorCallIfNeeded(sym);
  if (destructor == NULL) {
    return;
  }
  SourceLocation location = sym->location;
  if (sym->flags.is_local) {
    return;
  }
  ASTNode* condition = NewBinaryASTNode(
      AST_OP(noteq), NULL, location, NewIdentifierASTNode(guard, location),
      NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
  Vector* destructor_statements = NewVector();
  VectorAppend(destructor_statements, destructor);
  ASTNode* guarded_destructor = NewIfStatementASTNode(
      condition, NewCompoundStatementASTNode(destructor_statements, location),
      NULL, false, location);
  VectorAppend(&compiler->cxx_thread_destructor_calls, guarded_destructor);
}

// Defines a static thunk that destroys `sym` and records it in
// `compiler->cxx_tls_block_dtor_thunks`; returns the thunk symbol.
static Symbol* RegisterCXXTlsBlockDtorThunk(Syntax* syntax, Symbol* sym) {
  SourceLocation location = sym->location;
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_ptr = NewPointerTo(kQualPlain, void_type);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, void_type);
  Symbol* param = NewSymbol("__ignored", void_ptr, STO(auto));
  param->flags.is_argument = true;
  param->flags.invented = true;
  param->location = location;
  param->value.arg_number = 0;
  VectorAppend(&func_type->info.function.prototype, param);

  String name;
  StringInit(&name, "__davecc_tls_block_dtor_");
  char suffix[32];
  snprintf(suffix, sizeof(suffix), "%zu_",
           compiler->cxx_tls_block_dtor_thunks.length);
  StringAppend(&name, suffix);
  AppendSanitizedAsmComponent(&name, sym->name.value);

  Symbol* thunk = NewSymbol(name.value, func_type, STO(static));
  thunk->flags.invented = true;
  thunk->flags.is_defined = true;
  thunk->location = location;
  func_type->info.function.symbol = thunk;
  func_type->info.function.definition = true;
  StringDestruct(&name);

  Vector* body_statements = NewVector();
  ASTNode* destructor =
      NewCXXDestructorCallOnReceiver(NewIdentifierASTNode(sym, location),
                                     sym->type, location);
  if (destructor != NULL) {
    VectorAppend(body_statements, destructor);
  }
  func_type->info.function.body =
      NewCompoundStatementASTNode(body_statements, location);

  bool added = SyntaxAddSymbol(syntax, thunk);
  assert(added);
  (void)added;

  CXXTlsBlockDtorThunk* entry = malloc(sizeof(CXXTlsBlockDtorThunk));
  entry->thunk = thunk;
  VectorAppend(&compiler->cxx_tls_block_dtor_thunks, entry);
  return thunk;
}

// Returns a statement calling the runtime to register `thunk` as the block
// destructor for a thread-local object at `location`.
static ASTNode* NewCXXTlsRegisterBlockDtorCall(Symbol* thunk, Symbol* object,
                                               SourceLocation location) {
  (void)object;
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_ptr = NewPointerTo(kQualPlain, void_type);
  TypeRecordCalculateSize(void_ptr);

  TypeRecord* register_func = NewFunctionTypeRecord();
  TypeRecordChain(register_func, void_type);
  Symbol* fn_param = NewSymbol("__fn", void_ptr, STO(auto));
  fn_param->flags.is_argument = true;
  fn_param->flags.invented = true;
  fn_param->value.arg_number = 0;
  Symbol* obj_param = NewSymbol("__obj", void_ptr, STO(auto));
  obj_param->flags.is_argument = true;
  obj_param->flags.invented = true;
  obj_param->value.arg_number = 1;
  VectorAppend(&register_func->info.function.prototype, fn_param);
  VectorAppend(&register_func->info.function.prototype, obj_param);
  TypeRecordCalculateSize(register_func);

  ASTNode* thunk_id = NewIdentifierASTNode(thunk, location);
  thunk->flags.address_taken = true;
  TypeRecord* thunk_ptr_type = NewPointerTo(kQualPlain, thunk->type);
  TypeRecordCalculateSize(thunk_ptr_type);
  ASTNode* thunk_addr =
      NewUnaryASTNode(AST_OP(address), thunk_ptr_type, location, thunk_id);

  ASTNode* null_arg =
      NewIntConstantASTNode(0, void_ptr, location);

  Vector* args = NewVector();
  VectorAppend(args, thunk_addr);
  VectorAppend(args, null_arg);

  Symbol* register_sym =
      NewSymbol("__davecc_tls_register_block_dtor", register_func, STO(extern));
  ASTNode* call = NewVectorASTNode(
      AST_OP(call), NULL, location, NewIdentifierASTNode(register_sym, location),
      args);
  return NewExpressionStatementASTNode(call, location);
}

// Unwraps `init` and `expr_init` nodes to the underlying initializer
// expression, or returns `initializer` unchanged.
static ASTNode* CXXThreadLocalInitializerExpression(ASTNode* initializer) {
  if (initializer == NULL) {
    return NULL;
  }
  if (initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  return initializer;
}

// True when the thread-local initializer requires dynamic (non-constant)
// initialization at runtime.
bool CXXThreadLocalInitializerIsDynamic(ASTNode* initializer) {
  ASTNode* expr = CXXThreadLocalInitializerExpression(initializer);
  return expr != NULL && !IsConstantExpression(expr);
}

// Wraps one-time initialization of thread-local `sym` in a guard and optional
// TLS block-dtor registration; uses a default constructor when `init` is NULL.
ASTNode* NewCXXThreadLocalGuardedInit(Syntax* syntax, Symbol* sym,
                                      ASTNode* init) {
  if (init == NULL) {
    init = SyntaxNewCXXDefaultConstructorCallIfNeeded(syntax, sym);
    if (init == NULL) {
      return NULL;
    }
  }
  init = NewExpressionStatementASTNode(init, sym->location);

  SourceLocation location = sym->location;
  Symbol* guard = NewSymbol(
      SyntaxFakeName(syntax), NewTypeRecordWithSize(kTypeInt, kQualPlain),
      STO(static) | STO(thread));
  guard->flags.invented = true;
  guard->flags.is_defined = true;
  guard->flags.is_local = true;
  guard->location = location;
  SetCXXInlineLocalStaticGuardAsmName(compiler->current_function, guard, sym);
  bool added = SyntaxAddSymbol(syntax, guard);
  assert(added);
  (void)added;
  VectorAppend(&syntax->local_statics,
               NewVariableDeclarationASTNode(guard, NULL, location));

  Symbol* block_dtor_thunk = NULL;
  if (sym->flags.is_local && NewCXXDestructorCallIfNeeded(sym) != NULL) {
    block_dtor_thunk = RegisterCXXTlsBlockDtorThunk(syntax, sym);
  } else {
    RegisterCXXThreadLocalDestructor(sym, guard);
  }

  Vector* guarded_statements = NewVector();
  VectorAppend(guarded_statements, init);
  VectorAppend(guarded_statements,
               NewExpressionStatementASTNode(
                   NewIntAssignment(guard, 1, location), location));
  if (block_dtor_thunk != NULL) {
    VectorAppend(guarded_statements,
                 NewCXXTlsRegisterBlockDtorCall(block_dtor_thunk, sym,
                                                location));
  }

  ASTNode* condition = NewBinaryASTNode(
      AST_OP(equal), NULL, location, NewIdentifierASTNode(guard, location),
      NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
  ASTNode* guarded =
      NewIfStatementASTNode(condition,
                            NewCompoundStatementASTNode(guarded_statements,
                                                        location),
                            NULL, false, location);

  Vector* statements = NewVector();
  VectorAppend(statements, guarded);
  return NewUnaryASTNode(AST_OP(stmt_expr),
                         NewTypeRecordWithSize(kTypeVoid, kQualPlain),
                         location, NewCompoundStatementASTNode(statements,
                                                               location));
}

// Returns guarded dynamic initialization of thread-local `sym` via default
// construction when no explicit initializer is present.
ASTNode* NewCXXThreadLocalGuardedConstructor(Syntax* syntax,
                                             Symbol* sym) {
  return NewCXXThreadLocalGuardedInit(syntax, sym, NULL);
}

// Returns the non-trivial destructor member symbol for `type`, or NULL when
// none exists or the destructor is trivial.
static Symbol* FindCXXDestructorForType(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  StringDestruct(&destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      destructor->symbol == NULL ||
      !destructor->symbol->type->info.function.is_destructor ||
      destructor->symbol->type->info.function.is_trivial_special_member) {
    return NULL;
  }
  return destructor->symbol;
}

// Looks up a CXX runtime function `name`, or inserts an invented extern
// declaration with return type `return_kind` when it is not yet known.
static Symbol* GetCXXRuntimeFunction(const char* name, Type return_kind,
                                     SourceLocation location) {
  String runtime_name;
  StringInit(&runtime_name, name);
  Symbol* symbol = FindGlobalSymbol(&runtime_name);
  StringDestruct(&runtime_name);
  if (symbol != NULL) {
    return symbol;
  }
  TypeRecord* function = NewFunctionTypeRecord();
  TypeRecordChain(function,
                  NewTypeRecordWithSize(return_kind, kQualPlain));
  symbol = NewSymbol(name, function, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->location = location;
  bool added = SyntaxAddSymbol(&compiler->syntax, symbol);
  assert(added);
  (void)added;
  return symbol;
}

// Builds a call to runtime function `name` with arguments `actuals`.
static ASTNode* NewCXXRuntimeCall(const char* name, Type return_kind,
                                  Vector* actuals,
                                  SourceLocation location) {
  Symbol* function = GetCXXRuntimeFunction(name, return_kind, location);
  ASTNode* callee = NewIdentifierASTNode(function, location);
  callee->flags |= kASTNeedAddress;
  return NewVectorASTNode(AST_OP(call), TypeRecordCopy(function->type->next),
                          location, callee, actuals);
}

// Returns an address-of expression for `symbol` at `location`.
static ASTNode* NewSymbolAddress(Symbol* symbol, SourceLocation location) {
  TypeRecord* pointer = NewPointerTo(kQualPlain, TypeRecordCopy(symbol->type));
  ASTNode* identifier = NewIdentifierASTNode(symbol, location);
  identifier->flags |= kASTNeedAddress;
  return NewUnaryASTNode(AST_OP(address), pointer, location, identifier);
}

// Calls runtime function `name` passing the address of guard variable `guard`.
static ASTNode* NewGuardRuntimeCall(const char* name, Type return_kind,
                                    Symbol* guard,
                                    SourceLocation location) {
  Vector* actuals = NewVector();
  VectorAppend(actuals, NewSymbolAddress(guard, location));
  return NewCXXRuntimeCall(name, return_kind, actuals, location);
}

// Returns a `__davecc_cxa_atexit` registration for `sym`, or NULL when its
// (possibly array) type has no non-trivial destructor.
static ASTNode* NewCXXAtexitRegistration(Symbol* sym,
                                         SourceLocation location) {
  TypeRecord* object_type = sym->type;
  unsigned long count = 1;
  if (TypeIsFixedArray(object_type)) {
    object_type = CXXArrayBaseElementType(object_type, &count);
  }
  Symbol* destructor = FindCXXDestructorForType(object_type);
  if (destructor == NULL) {
    return NULL;
  }
  Vector* actuals = NewVector();
  ASTNode* destructor_address = NewIdentifierASTNode(destructor, location);
  destructor_address->flags |= kASTNeedAddress;
  VectorAppend(actuals, destructor_address);
  VectorAppend(actuals, NewSymbolAddress(sym, location));
  VectorAppend(actuals,
               NewIntConstantASTNode(
                   count,
                   NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain),
                   location));
  VectorAppend(actuals,
               NewIntConstantASTNode(
                   object_type->size,
                   NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain),
                   location));
  VectorAppend(actuals,
               NewIntConstantASTNode(
                   TypeNeedsCXXCompleteObjectArgument(object_type) ? 1 : 0,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
  return NewCXXRuntimeCall("__davecc_cxa_atexit", kTypeInt, actuals, location);
}

// Wraps at-exit registration for global `sym` in an expression statement, or
// NULL when no destructor registration is needed.
ASTNode* SyntaxNewCXXGlobalAtexitStatement(Symbol* sym,
                                           SourceLocation location) {
  ASTNode* registration = NewCXXAtexitRegistration(sym, location);
  if (registration == NULL) {
    return NULL;
  }
  return NewExpressionStatementASTNode(registration, location);
}

// Returns a statement (or loop) that destroys fixed-array `sym`'s elements in
// reverse construction order, or NULL when no element destructor applies.
ASTNode* SyntaxNewCXXArrayDestructorCalls(Symbol* sym) {
  if (sym == NULL || !TypeIsFixedArray(sym->type)) {
    return NULL;
  }
  unsigned long element_count = 0;
  TypeRecord* element_type = CXXArrayBaseElementType(sym->type, &element_count);
  Symbol* destructor = FindCXXDestructorForType(element_type);
  if (destructor == NULL || element_count == 0) {
    return NULL;
  }
  SourceLocation location = sym->location;
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, element_type->info.struct_info->tag_name);
  // Elements are destroyed in the reverse order of their construction.
  if (element_count > kCXXUnrolledArrayElementLimit) {
    ASTNode* loop = NewCXXArrayElementLoop(
        &compiler->syntax, NewCXXArrayElementExpression(sym, 0, location),
        element_type, element_count, destructor_name.value,
        /*reverse=*/true, location);
    StringDestruct(&destructor_name);
    return loop;
  }
  ASTNode* sequence = NULL;
  for (unsigned long i = element_count; i > 0; i--) {
    ASTNode* element = NewCXXArrayElementExpression(sym, i - 1, location);
    ASTNode* call = NewCXXElementSpecialMemberCall(
        element, element_type, destructor_name.value, location);
    if (sequence == NULL) {
      sequence = call;
    } else {
      sequence = NewBinaryASTNode(
          AST_OP(comma), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
          location, sequence, call);
    }
  }
  StringDestruct(&destructor_name);
  return NewExpressionStatementASTNode(sequence, location);
}

// True when `type` is a function type with void return and no parameters.
static bool FunctionTypeIsVoidVoid(TypeRecord* type) {
  if (type == NULL || !TypeIsFunction(type) || type->next == NULL ||
      !TypeIsVoid(type->next)) {
    return false;
  }
  return type->info.function.prototype.length == 0;
}

// Reads constructor/destructor attribute `attr` priority, emits diagnostics on
// invalid values, and returns -1 on error or the priority otherwise.
static int ParseInitFiniAttributePriority(Syntax* syntax, Attribute* attr,
                                          SourceLocation location) {
  int priority = kCXXInitFiniPriorityDefault;
  if (AttributeArgCount(attr) > 0) {
    long value = 0;
    if (!AttributeArgInt(attr, 0, &value) || value < 0 || value > 65535) {
      SyntaxErrorAtLocation(
          syntax, location,
          "constructor/destructor attribute priority must be an integer "
          "between 0 and 65535");
      return -1;
    }
    priority = (int)value;
    if (priority < 101) {
      SyntaxWarning(syntax, "attributes",
                    "priority %d is reserved for the implementation", priority);
    }
  }
  return priority;
}

// Appends `function` to the init/fini function list `functions`.
static void RegisterInitFiniArrayEntry(Vector* functions, Symbol* function) {
  VectorAppend(functions, function);
}

// Registers `sym` with GCC-style constructor/destructor init/fini arrays when
// it is a defined void function carrying those attributes; emits errors
// otherwise.
void SyntaxRegisterFunctionInitFiniAttributes(Syntax* syntax, Symbol* sym) {
  if (sym == NULL || !TypeIsFunction(sym->type) || !sym->flags.is_defined) {
    return;
  }
  Attribute* constructor =
      AttributeListFind(&sym->attributes, "constructor");
  Attribute* destructor = AttributeListFind(&sym->attributes, "destructor");
  if (constructor == NULL && destructor == NULL) {
    return;
  }
  if (!FunctionTypeIsVoidVoid(sym->type)) {
    SyntaxErrorAtLocation(
        syntax, sym->location,
        "constructor/destructor attribute applies only to void functions "
        "with no parameters");
    return;
  }
  if (sym->flags.is_template) {
    SyntaxErrorAtLocation(syntax, sym->location,
                          "constructor/destructor attribute cannot be applied "
                          "to a function template");
    return;
  }
  if (constructor != NULL) {
    int priority =
        ParseInitFiniAttributePriority(syntax, constructor, sym->location);
    if (priority >= 0) {
      RegisterInitFiniArrayEntry(CXXInitArrayFunctionsVector(), sym);
      sym->flags.used = true;
    }
  }
  if (destructor != NULL) {
    int priority =
        ParseInitFiniAttributePriority(syntax, destructor, sym->location);
    if (priority >= 0) {
      RegisterInitFiniArrayEntry(CXXFiniArrayFunctionsVector(), sym);
      sym->flags.used = true;
    }
  }
}

// Creates an invented local static guard symbol for local static `sym` in
// `func` and records it on `syntax->all_local_symbols`.
static Symbol* NewCXXLocalStaticGuard(Syntax* syntax, TypeRecord* func,
                                      Symbol* sym) {
  TypeRecord* guard_type =
      NewTypeRecordWithSize(kTypeLongLong | kTypeUnsigned, kQualPlain);
  Symbol* guard = NewSymbol(SyntaxFakeName(syntax), guard_type, STO(static));
  guard->flags.invented = true;
  guard->flags.is_defined = true;
  guard->flags.is_local = true;
  guard->alignment = 8;
  guard->location = sym->location;
  SetCXXInlineLocalStaticGuardAsmName(func, guard, sym);
  VectorAppend(&syntax->all_local_symbols, guard);
  return guard;
}

// Returns an assignment of `value` to `symbol` through its address.
static ASTNode* NewIntegerAssignment(Symbol* symbol, int value,
                                     SourceLocation location) {
  ASTNode* destination = NewIdentifierASTNode(symbol, location);
  destination->flags |= kASTNeedAddress;
  return NewBinaryASTNode(
      AST_OP(assign), TypeRecordCopy(symbol->type), location,
      destination,
      NewIntConstantASTNode(
          value, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
}

// Flattens a comma expression tree into individual expressions appended to
// `expressions`.
static void CollectCommaExpressions(ASTNode* expression, Vector* expressions) {
  if (expression != NULL && expression->op == AST_OP(comma)) {
    BinaryASTNode* comma = (BinaryASTNode*)expression;
    CollectCommaExpressions(comma->left, expressions);
    CollectCommaExpressions(comma->right, expressions);
    return;
  }
  if (expression != NULL) {
    VectorAppend(expressions, expression);
  }
}

// Returns the address of element `index` of array variable `array`.
static ASTNode* ArrayElementAddress(Symbol* array, int index,
                                    SourceLocation location) {
  TypeRecord* element_type = CXXArrayBaseElementType(array->type, NULL);
  ASTNode* element =
      NewCXXArrayElementExpression(array, (unsigned long)index, location);
  ASTNode* base = element;
  while (base != NULL && base->op == AST_OP(subscript)) {
    base->flags |= kASTNeedAddress;
    ASTNode* left = ((BinaryASTNode*)base)->left;
    if (left != NULL && left->op == AST_OP(identifier)) {
      left->flags |= kASTNeedAddress;
    }
    base = left;
  }
  element->flags |= kASTNeedAddress;
  return NewUnaryASTNode(
      AST_OP(address), NewPointerTo(kQualPlain, TypeRecordCopy(element_type)),
      location, element);
}

// Calls special member `function` on the object at `object_address`, including
// a complete-object argument when required by `object_type`.
static ASTNode* NewDirectCXXSpecialMemberCall(Symbol* function,
                                              ASTNode* object_address,
                                              TypeRecord* object_type,
                                              SourceLocation location) {
  ASTNode* callee = NewIdentifierASTNode(function, location);
  callee->flags |= kASTNeedAddress;
  Vector* actuals = NewVector();
  VectorAppend(actuals, object_address);
  if (TypeNeedsCXXCompleteObjectArgument(object_type)) {
    VectorAppend(actuals,
                 NewIntConstantASTNode(
                     1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                     location));
  }
  return NewVectorASTNode(
      AST_OP(call), TypeRecordCopy(function->type->next), location, callee,
      actuals);
}

// Introduces a local int counter tracking how many array elements were
// constructed during initialization.
static Symbol* NewArrayConstructionCount(Syntax* syntax,
                                         SourceLocation location) {
  Symbol* count =
      NewSymbol(SyntaxFakeName(syntax),
                NewTypeRecordWithSize(kTypeInt, kQualPlain), STO(auto));
  count->flags.invented = true;
  count->flags.is_defined = true;
  count->flags.is_local = true;
  count->location = location;
  VectorAppend(&syntax->all_local_symbols, count);
  return count;
}

// When `declaration` holds a comma-separated per-element constructor list,
// expands it into sequential statements on `success` and sets
// `*constructed_count`; returns false if the pattern does not apply.
static bool PrepareDefaultArrayInitialization(
    Syntax* syntax, VariableDeclarationASTNode* declaration, Vector* success,
    Symbol** constructed_count) {
  Symbol* array = declaration->symbol;
  ASTNode* initializer = declaration->initializer;
  if (!TypeIsFixedArray(array->type) || initializer == NULL ||
      initializer->op != AST_OP(stmt_expr)) {
    return false;
  }
  UnaryASTNode* statement_expression = (UnaryASTNode*)initializer;
  if (statement_expression->sub == NULL ||
      statement_expression->sub->op != AST_OP(compound)) {
    return false;
  }
  CompoundStatementASTNode* compound =
      (CompoundStatementASTNode*)statement_expression->sub;
  if (compound->statements->length != 1) {
    return false;
  }
  ASTNode* statement = VectorGet(compound->statements, 0);
  if (statement == NULL || statement->op != AST_OP(expr)) {
    return false;
  }

  Vector constructors;
  VectorInit(&constructors);
  CollectCommaExpressions(((ExpressionStatementASTNode*)statement)->expr,
                          &constructors);
  unsigned long element_count = 0;
  CXXArrayBaseElementType(array->type, &element_count);
  if (constructors.length != (size_t)element_count) {
    VectorDestruct(&constructors);
    return false;
  }

  Symbol* count = NewArrayConstructionCount(syntax, declaration->base.location);
  *constructed_count = count;
  VectorAppend(success,
               NewVariableDeclarationASTNode(
                   count,
                   NewIntegerAssignment(count, 0, declaration->base.location),
                   declaration->base.location));
  for (size_t i = 0; i < constructors.length; i++) {
    VectorAppend(success,
                 NewExpressionStatementASTNode(
                     VectorGet(&constructors, i), declaration->base.location));
    VectorAppend(success,
                 NewExpressionStatementASTNode(
                     NewIntegerAssignment(count, (int)i + 1,
                                          declaration->base.location),
                     declaration->base.location));
  }
  VectorDestruct(&constructors);
  return true;
}

// Appends guarded destructor calls for elements below `constructed_count` to
// `failure`, for exception cleanup during array default initialization.
static void AppendArrayInitializationCleanup(
    VariableDeclarationASTNode* declaration, Symbol* constructed_count,
    Vector* failure) {
  if (constructed_count == NULL) {
    return;
  }
  Symbol* array = declaration->symbol;
  unsigned long element_count = 0;
  TypeRecord* element_type =
      CXXArrayBaseElementType(array->type, &element_count);
  Symbol* destructor = FindCXXDestructorForType(element_type);
  if (destructor == NULL) {
    return;
  }
  SourceLocation location = declaration->base.location;
  for (int i = (int)element_count - 1; i >= 0; i--) {
    ASTNode* condition = NewBinaryASTNode(
        AST_OP(greater), NewTypeRecordWithSize(kTypeBool, kQualPlain), location,
        NewIdentifierASTNode(constructed_count, location),
        NewIntConstantASTNode(
            i, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
    ASTNode* destructor_call = NewDirectCXXSpecialMemberCall(
        destructor, ArrayElementAddress(array, i, location), element_type,
        location);
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatementASTNode(destructor_call, location));
    VectorAppend(
        failure,
        NewIfStatementASTNode(
            condition, NewCompoundStatementASTNode(statements, location), NULL,
            false, location));
  }
}

// Replaces or wraps `declaration`'s initializer with `__cxa_guard_*` guarded
// dynamic init, at-exit registration, and optional try/catch abort paths.
static ASTNode* NewCXXLocalStaticGuardedInitializer(
    Syntax* syntax, TypeRecord* func, VariableDeclarationASTNode* declaration,
    bool run_initializer) {
  Symbol* sym = declaration->symbol;
  SourceLocation location = declaration->base.location;
  ASTNode* original_initializer = declaration->initializer;
  Symbol* guard = NewCXXLocalStaticGuard(syntax, func, sym);
  declaration->local_static_guard = guard;

  Vector* success = NewVector();
  Symbol* constructed_count = NULL;
  if (!run_initializer) {
    // Constant initialization already resides in static storage.  The guard
    // exists only to register a non-trivial destructor on first passage.
  } else if (!PrepareDefaultArrayInitialization(syntax, declaration, success,
                                                &constructed_count)) {
    VectorAppend(success, NewExpressionStatementASTNode(
                              declaration->initializer, location));
  }
  ASTNode* registration = NewCXXAtexitRegistration(sym, location);
  if (registration != NULL) {
    VectorAppend(success,
                 NewExpressionStatementASTNode(registration, location));
  }
  VectorAppend(success,
               NewExpressionStatementASTNode(
                   NewGuardRuntimeCall("__cxa_guard_release", kTypeVoid, guard,
                                       location),
                   location));

  ASTNode* success_body = NewCompoundStatementASTNode(success, location);
  ASTNode* initialization = success_body;
  if (CompilerExceptionsEnabled() && compiler->target_name != NULL &&
      (StringEqual(compiler->target_name, "x86_64") ||
       StringEqual(compiler->target_name, "aarch64") ||
       StringEqual(compiler->target_name, "arm") ||
       StringEqual(compiler->target_name, "riscv") ||
       StringEqual(compiler->target_name, "p-code") ||
       StringEqual(compiler->target_name, "pcode"))) {
    Vector* catches = NewVector();
    Vector* failure = NewVector();
    AppendArrayInitializationCleanup(declaration, constructed_count, failure);
    VectorAppend(failure,
                 NewExpressionStatementASTNode(
                     NewGuardRuntimeCall("__cxa_guard_abort", kTypeVoid, guard,
                                         location),
                     location));
    VectorAppend(failure,
                 NewExpressionStatementASTNode(
                     NewThrowASTNode(NULL, location), location));
    VectorAppend(catches,
                 NewCatchASTNode(
                     NULL, true,
                     NewCompoundStatementASTNode(failure, location), location));
    initialization = NewTryASTNode(success_body, catches, location);
  }

  Vector* guarded_statements = NewVector();
  VectorAppend(guarded_statements, initialization);
  ASTNode* guarded_if = NewIfStatementASTNode(
      NewGuardRuntimeCall("__cxa_guard_acquire", kTypeInt, guard, location),
      NewCompoundStatementASTNode(guarded_statements, location), NULL, false,
      location);
  Vector* statements = NewVector();
  VectorAppend(statements, guarded_if);
  ASTNode* guarded = NewUnaryASTNode(
      AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain), location,
      NewCompoundStatementASTNode(statements, location));
  if (!run_initializer && original_initializer != NULL) {
    return NewBinaryASTNode(
        AST_OP(comma), TypeRecordCopy(original_initializer->type), location,
        guarded, original_initializer);
  }
  return guarded;
}

// An automatic array's cleanup range opens only once the whole array is built,
// so an exception from one element's constructor must destroy the elements
// already constructed before it propagates.
static void GuardCXXAutomaticArrayInitializer(
    Syntax* syntax, VariableDeclarationASTNode* declaration) {
  Symbol* array = declaration->symbol;
  if (!CompilerExceptionsEnabled() || !TypeIsFixedArray(array->type) ||
      FindCXXDestructorForType(CXXArrayBaseElementType(array->type, NULL)) ==
          NULL) {
    return;
  }
  SourceLocation location = declaration->base.location;
  Vector* success = NewVector();
  Symbol* constructed_count = NULL;
  if (!PrepareDefaultArrayInitialization(syntax, declaration, success,
                                         &constructed_count)) {
    VectorDelete(success);
    return;
  }
  Vector* failure = NewVector();
  AppendArrayInitializationCleanup(declaration, constructed_count, failure);
  VectorAppend(failure, NewExpressionStatementASTNode(
                            NewThrowASTNode(NULL, location), location));
  Vector* catches = NewVector();
  VectorAppend(catches,
               NewCatchASTNode(NULL, true,
                               NewCompoundStatementASTNode(failure, location),
                               location));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewTryASTNode(NewCompoundStatementASTNode(success, location),
                             catches, location));
  declaration->initializer = NewUnaryASTNode(
      AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain), location,
      NewCompoundStatementASTNode(statements, location));
  declaration->initializer->parent = &declaration->base;
}

typedef struct {
  Syntax* syntax;
  TypeRecord* function;
} CXXLocalStaticPreparation;

// Visitor callback: sets `*(bool*)data` when a static-init `init` node appears.
static void FindCXXLocalStaticConstantInitializer(
    ASTNode* node, void* data, int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL &&
      node->op == AST_OP(init) && (node->flags & kASTStaticInit) != 0) {
    *(bool*)data = true;
  }
}

// Visitor callback: prepares asm names, init classification, exception guards,
// and guarded initializers for local and function-local static declarations.
static void PrepareCXXLocalStaticVisitor(ASTNode* node, void* data, int child_id,
                                         VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL ||
      node->op != AST_OP(vardecl)) {
    return;
  }
  VariableDeclarationASTNode* declaration =
      (VariableDeclarationASTNode*)node;
  Symbol* symbol = declaration->symbol;
  CXXLocalStaticPreparation* preparation = data;
  if (symbol != NULL && (symbol->storage == STO(implicit) ||
                         StorageIs(symbol->storage, STO(auto)) ||
                         StorageIs(symbol->storage, STO(register)))) {
    GuardCXXAutomaticArrayInitializer(preparation->syntax, declaration);
    return;
  }
  if (symbol == NULL || !StorageIs(symbol->storage, STO(static)) ||
      StorageIs(symbol->storage, STO(thread))) {
    return;
  }

  SetCXXInlineLocalStaticAsmNameFor(
      preparation->function, symbol, symbol->name.value, symbol->location, "");
  if (declaration->local_static_init_kind == kLocalStaticInitUnclassified) {
    bool constant = declaration->initializer == NULL;
    ASTNodeVisit(declaration->initializer,
                 FindCXXLocalStaticConstantInitializer, 0, &constant);
    declaration->local_static_init_kind =
        constant ? kLocalStaticInitConstant : kLocalStaticInitDynamic;
  }
  bool dynamic =
      declaration->local_static_init_kind == kLocalStaticInitDynamic;
  TypeRecord* destructor_type = symbol->type;
  if (TypeIsFixedArray(destructor_type)) {
    destructor_type = CXXArrayBaseElementType(destructor_type, NULL);
  }
  bool needs_destructor_registration =
      FindCXXDestructorForType(destructor_type) != NULL;
  if ((!dynamic && !needs_destructor_registration) ||
      (dynamic && declaration->initializer == NULL) ||
      declaration->local_static_guard != NULL) {
    return;
  }
  declaration->initializer = NewCXXLocalStaticGuardedInitializer(
      preparation->syntax, preparation->function, declaration, dynamic);
  declaration->initializer->parent = node;
}

// Walks `function`'s body and transforms function-local static and related
// declarations for guarded initialization and destruction.
void SyntaxPrepareCXXLocalStatics(Syntax* syntax, TypeRecord* function) {
  if (!CompilerIsCXX() || syntax == NULL || function == NULL ||
      !TypeIsFunction(function) || function->info.function.body == NULL) {
    return;
  }
  CXXLocalStaticPreparation preparation = {syntax, function};
  ASTNodeVisit(function->info.function.body, PrepareCXXLocalStaticVisitor, 0,
               &preparation);
}
