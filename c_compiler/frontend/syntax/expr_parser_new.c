//
//  expr_parser_new.c
//  c_compiler
//
//  New-expressions and delete-expressions, including allocation and array loops.
//

#include <stdint.h>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "concepts.h"
#include "errors.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_parser_internal.h"
#include "expr_semantics.h"
#include "preprocessor.h"
#include "reflection.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "std_header_suggestion.h"
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_internal.h"
#include "type_member.h"
#include "compiler.h"
#include "type_traits_semantics.h"


// Walks an array new-initializer the way [dcl.init.aggr] consumes it,
// including brace elision, and initializes each base-type element of the
// allocation (viewed as a flat array at `base_ptr`) in place.  With
// `statements` NULL it only consumes, which counts the elements of an array
// whose bound is deduced; `syntax` is then NULL too, so it reports nothing.
typedef struct {
  Syntax* syntax;
  Vector* statements;
  Symbol* base_ptr;
  TypeRecord* base_type;
  bool constructed_class;
  Symbol* constructed;  // Count of completed elements, or NULL.
  SourceLocation location;
} CXXNewArrayInit;


typedef struct {
  Vector* items;  // Each an expression or a braced_init.
  size_t next;
} CXXNewInitCursor;


static Symbol* GetImplicitCXXAllocationFunction(const char* name, TypeRecord* return_type, TypeRecord* arg_type, SourceLocation location);
static Symbol* FindCXXAllocationFunctionByArgCount(Symbol* first, size_t arg_count);
static Symbol* FindCXXPlacementAllocationFunction(Symbol* first);
static Symbol* FindGlobalCXXAllocationFunction(const char* name, size_t arg_count);
static Symbol* GetImplicitCXXOperatorNew(SourceLocation location);
static Symbol* GetImplicitCXXOperatorNewArray(SourceLocation location);
static Symbol* GetImplicitCXXPlacementOperatorNew(SourceLocation location);
static Symbol* GetImplicitCXXOperatorDelete(SourceLocation location);
static Symbol* GetImplicitCXXOperatorDeleteArray(SourceLocation location);
static Symbol* GetCXXClassAllocationFunction(TypeRecord* type, const char* name, size_t arg_count);
static Symbol* GetCXXOperatorNewForType(TypeRecord* type, bool is_array, size_t arg_count, SourceLocation location, bool global_scope, bool standard_placement);
static Symbol* GetCXXOperatorDeleteForType(TypeRecord* type, bool is_array, SourceLocation location, bool global_scope);
static ASTNode* NewCallASTNode(Symbol* callee, SourceLocation location, Vector* actuals);
static bool CXXNewHasPlacementArguments(Syntax* syntax);
static bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type);
static void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals, SourceLocation location);
static ASTNode* NewCXXConstructorCallForPointer(TypeRecord* allocated_type, Symbol* ptr, Vector* actuals, SourceLocation location);
static ASTNode* NewCXXConstructorCallForReceiver(TypeRecord* allocated_type, ASTNode* receiver, Vector* actuals, SourceLocation location);
static StructMember* FindCXXDestructorForType(TypeRecord* type);
static ASTNode* NewCXXDestructorCallForPointer(TypeRecord* object_type, Symbol* ptr, SourceLocation location);
static ASTNode* NewExpressionStatement(ASTNode* expr, SourceLocation location);
static ASTNode* NewAssign(ASTNode* left, ASTNode* right, TypeRecord* type, SourceLocation location);
static ASTNode* NewTypedCast(TypeRecord* type, ASTNode* expr, SourceLocation location);
static ASTNode* NewPtrAdd(ASTNode* ptr, ASTNode* offset, SourceLocation location);
static ASTNode* NewPtrSub(ASTNode* ptr, ASTNode* offset, SourceLocation location);
static ASTNode* NewIntLiteral(int64_t value, SourceLocation location);
static ASTNode* NewPostIncrement(Symbol* sym, SourceLocation location);
static ASTNode* NewArrayConstructedCountUpdate(Symbol* constructed, ASTNode* index, SourceLocation location);
static ASTNode* NewArrayConstructionLoop(Syntax* syntax, TypeRecord* element_type, Symbol* ptr, Symbol* count, int64_t first, Symbol* constructed, SourceLocation location);
static ASTNode* NewArrayValueInitializationLoop( Syntax* syntax, TypeRecord* element_type, Symbol* ptr, Symbol* count, int64_t first, Symbol* constructed, SourceLocation location);
static ASTNode* NewArrayDestructionLoop(Syntax* syntax, TypeRecord* element_type, Symbol* ptr, Symbol* count, SourceLocation location);
static ASTNode* IdentityCloneNodeForCXXNew(ASTNode* node, void* data);
static Vector* CloneAndAnalyzeCXXNewDeductionActuals(Vector* actuals);
static TypeRecord* DeduceCXXNewAllocatedType(Syntax* syntax, TypeRecord* allocated_type, Vector* actuals);
static Vector* ParseCXXNewDeductionInitializerArguments(Syntax* syntax, TypeRecord* type, Token open, TokenClass followers);
static ASTNode* NewCXXNewCompoundLiteralInitializer(Syntax* syntax, TypeRecord* type, Vector* exprs, SourceLocation location);
static bool CXXNewTypeIsAggregateClass(TypeRecord* type);
static ASTNode* NewCXXNewBracedInitializer(Vector* items, SourceLocation location);
static ASTNode* CXXNewScalarTarget(ASTNode* target, TypeRecord* type, SourceLocation location);
static ASTNode* NewCXXNewInitialization(Syntax* syntax, TypeRecord* type, ASTNode* target, ASTNode* initializer, SourceLocation location);
static TypeRecord* CXXNewArrayBaseType(TypeRecord* type, int64_t* count);
static Vector* CXXNewBracedItems(Syntax* syntax, ASTNode* braced_node);
static void AppendCXXNewLeafInit(CXXNewArrayInit* init, int64_t index, ASTNode* initializer);
static void InitCXXNewSubobject(CXXNewArrayInit* init, CXXNewInitCursor* cursor, TypeRecord* type, int64_t index);
static size_t InitCXXNewArrayElements(CXXNewArrayInit* init, Vector* items, TypeRecord* element_type);
static TypeRecord* CXXMutablePointerType(TypeRecord* pointer_type);
static ASTNode* NewDeallocationPointer(ASTNode* expr, SourceLocation location);


// Returns an existing one-argument allocation function or synthesizes the
// required global declaration.
static Symbol* GetImplicitCXXAllocationFunction(const char* name,
                                                TypeRecord* return_type,
                                                TypeRecord* arg_type,
                                                SourceLocation location) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* symbol = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  Symbol* existing = FindCXXAllocationFunctionByArgCount(symbol, 1);
  if (existing != NULL) {
    TypeRecordDelete(return_type);
    TypeRecordDelete(arg_type);
    return existing;
  }

  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, return_type);
  Symbol* formal = NewSymbol(SyntaxFakeName(&compiler->syntax), arg_type,
                             STO(auto));
  formal->flags.is_argument = true;
  formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, formal);

  symbol = NewSymbol(name, func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->location = location;
  func_type->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);
  if (FindGlobalSymbol(&symbol->name) == NULL) {
    bool ok = InsertGlobalSymbol(symbol);
    assert(ok);
    (void)ok;
  } else {
    Symbol* first = FindGlobalSymbol(&symbol->name);
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


// Finds an allocation-function overload with the requested number of
// parameters.
static Symbol* FindCXXAllocationFunctionByArgCount(Symbol* first,
                                                   size_t arg_count) {
  for (Symbol* symbol = first; symbol != NULL; symbol = symbol->overload_next) {
    if (symbol->type != NULL && TypeIsFunction(symbol->type) &&
        symbol->type->info.function.prototype.length == arg_count) {
      return symbol;
    }
  }
  return NULL;
}


// Finds the standard two-argument placement allocation overload whose second
// parameter is void pointer.
static Symbol* FindCXXPlacementAllocationFunction(Symbol* first) {
  for (Symbol* symbol = first; symbol != NULL; symbol = symbol->overload_next) {
    if (symbol->type == NULL || !TypeIsFunction(symbol->type) ||
        symbol->type->info.function.prototype.length != 2) {
      continue;
    }
    Symbol* pointer_formal =
        symbol->type->info.function.prototype.value.p[1];
    if (pointer_formal != NULL && TypeIsPointer(pointer_formal->type) &&
        TypeIsVoid(pointer_formal->type->next)) {
      return symbol;
    }
  }
  return NULL;
}


// Looks up a global allocation-function overload by name and parameter count.
static Symbol* FindGlobalCXXAllocationFunction(const char* name,
                                               size_t arg_count) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* first = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  return FindCXXAllocationFunctionByArgCount(first, arg_count);
}


// Returns or synthesizes the usual global scalar operator new declaration.
static Symbol* GetImplicitCXXOperatorNew(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  return GetImplicitCXXAllocationFunction("operator new", return_type,
                                          NewSizeTypeRecord(), location);
}


// Returns or synthesizes the usual global array operator new declaration.
static Symbol* GetImplicitCXXOperatorNewArray(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  return GetImplicitCXXAllocationFunction("operator new[]", return_type,
                                          NewSizeTypeRecord(), location);
}


// Returns or synthesizes the standard global placement operator new(size_t,
// void*) declaration.
static Symbol* GetImplicitCXXPlacementOperatorNew(SourceLocation location) {
  String symbol_name;
  StringInit(&symbol_name, "operator new");
  Symbol* existing =
      FindCXXPlacementAllocationFunction(FindGlobalSymbol(&symbol_name));
  StringDestruct(&symbol_name);
  if (existing != NULL) {
    return existing;
  }

  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* return_type = NewPointerTo(kQualPlain, void_type);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, return_type);

  Symbol* size_formal = NewSymbol(SyntaxFakeName(&compiler->syntax),
                                  NewSizeTypeRecord(), STO(auto));
  size_formal->flags.is_argument = true;
  size_formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, size_formal);

  TypeRecord* ptr_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  Symbol* ptr_formal = NewSymbol(SyntaxFakeName(&compiler->syntax), ptr_type,
                                 STO(auto));
  ptr_formal->flags.is_argument = true;
  ptr_formal->flags.invented = true;
  VectorAppend(&func_type->info.function.prototype, ptr_formal);

  Symbol* symbol = NewSymbol("operator new", func_type, STO(extern));
  symbol->flags.invented = true;
  symbol->location = location;
  func_type->info.function.symbol = symbol;
  SymbolSetCXXMangledAsmName(symbol);

  Symbol* first = FindGlobalSymbol(&symbol->name);
  if (first == NULL) {
    bool ok = InsertGlobalSymbol(symbol);
    assert(ok);
    (void)ok;
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


// Returns or synthesizes the usual global scalar operator delete declaration.
static Symbol* GetImplicitCXXOperatorDelete(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* arg_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return GetImplicitCXXAllocationFunction("operator delete", void_type,
                                          arg_type, location);
}


// Returns or synthesizes the usual global array operator delete declaration.
static Symbol* GetImplicitCXXOperatorDeleteArray(SourceLocation location) {
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* arg_type =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  return GetImplicitCXXAllocationFunction("operator delete[]", void_type,
                                          arg_type, location);
}


// Finds a class-scoped allocation or deallocation overload by name and
// parameter count.
static Symbol* GetCXXClassAllocationFunction(TypeRecord* type,
                                             const char* name,
                                             size_t arg_count) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return NULL;
  }
  StructMember* member = FindStructMemberByName(type->info.struct_info, name);
  if (member == NULL || !member->is_member_function ||
      member->symbol == NULL || !TypeIsFunction(member->symbol->type)) {
    return NULL;
  }
  return FindCXXAllocationFunctionByArgCount(member->symbol, arg_count);
}


// Returns the class’s usual single-argument allocation or deallocation
// function.
Symbol* CXXClassUsualAllocationFunction(TypeRecord* type, const char* name) {
  return GetCXXClassAllocationFunction(type, name, 1);
}


// Selects class or global operator new for a scalar, array, or placement
// allocation.
static Symbol* GetCXXOperatorNewForType(TypeRecord* type,
                                        bool is_array,
                                        size_t arg_count,
                                        SourceLocation location,
                                        bool global_scope,
                                        bool standard_placement) {
  // `::new` names only the global allocation function, bypassing any
  // class-scoped operator new.
  Symbol* member = global_scope ? NULL : GetCXXClassAllocationFunction(
      type, is_array ? "operator new[]" : "operator new", arg_count);
  if (member != NULL && standard_placement) {
    member = FindCXXPlacementAllocationFunction(member);
  }
  if (member != NULL) {
    return member;
  }
  const char* name = is_array ? "operator new[]" : "operator new";
  Symbol* global = NULL;
  if (standard_placement) {
    String symbol_name;
    StringInit(&symbol_name, name);
    global =
        FindCXXPlacementAllocationFunction(FindGlobalSymbol(&symbol_name));
    StringDestruct(&symbol_name);
  } else {
    global = FindGlobalCXXAllocationFunction(name, arg_count);
  }
  if (global != NULL) {
    return global;
  }
  if (arg_count == 1) {
    return is_array ? GetImplicitCXXOperatorNewArray(location)
                    : GetImplicitCXXOperatorNew(location);
  }
  if (!is_array && standard_placement && arg_count == 2) {
    return GetImplicitCXXPlacementOperatorNew(location);
  }
  SyntaxError(&compiler->syntax, "No matching allocation function for placement new");
  return is_array ? GetImplicitCXXOperatorNewArray(location)
                  : GetImplicitCXXOperatorNew(location);
}


// Selects the matching class or global scalar or array deallocation function.
static Symbol* GetCXXOperatorDeleteForType(TypeRecord* type,
                                           bool is_array,
                                           SourceLocation location,
                                           bool global_scope) {
  // `::delete` names only the global deallocation function, bypassing any
  // class-scoped operator delete.
  Symbol* member = global_scope ? NULL : GetCXXClassAllocationFunction(
      type, is_array ? "operator delete[]" : "operator delete", 1);
  if (member != NULL) {
    return member;
  }
  return is_array ? GetImplicitCXXOperatorDeleteArray(location)
                  : GetImplicitCXXOperatorDelete(location);
}


// Builds a call AST for a known function symbol and argument vector.
static ASTNode* NewCallASTNode(Symbol* callee, SourceLocation location,
                               Vector* actuals) {
  return NewVectorASTNode(AST_OP(call), NULL, location,
                          NewIdentifierASTNode(callee, location), actuals);
}


// Returns the constructor overload set for a class type.
StructMember* FindCXXConstructorForType(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  StructMember* ctor =
      FindStructMember(type->info.struct_info, type->info.struct_info->tag_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}


// Reports whether any constructor overload accepts an initializer_list
// parameter.
bool CXXNewConstructorSetHasInitializerList(StructMember* ctor) {
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor) {
      continue;
    }
    for (size_t i = 0; i < info->prototype.length; i++) {
      Symbol* formal = info->prototype.value.p[i];
      TypeRecord* formal_type = formal->type;
      if (TypeIsReference(formal_type)) {
        formal_type = formal_type->next;
      }
      if (TypeIsCXXInitializerList(formal_type)) {
        return true;
      }
    }
  }
  return false;
}


// Parses the parenthesized or braced argument list following a new-type-id.
Vector* ParseCXXNewInitializerArguments(Syntax* syntax, Token open,
                                               TokenClass followers) {
  Token close = open == TOK(lparen) ? TOK(rparen) : TOK(rbrace);
  LexNextToken(syntax->lex);
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, close)) {
    ASTNode* actual =
        LexMatch(syntax->lex, TOK(lbrace))
            ? SyntaxParseBracedInitializer(syntax)
            : SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, close, followers | TC(exprsep));
  return actuals;
}

// Returns the new-initializer arguments, taking ownership of `*parsed` when
// they were already parsed (to deduce a placeholder type).
static Vector* TakeCXXNewInitializerArguments(Syntax* syntax, Vector** parsed,
                                              Token open,
                                              TokenClass followers) {
  if (*parsed != NULL) {
    Vector* actuals = *parsed;
    *parsed = NULL;
    return actuals;
  }
  return ParseCXXNewInitializerArguments(syntax, open, followers);
}


// Looks ahead to distinguish placement arguments from a parenthesized
// new-type-id.
static bool CXXNewHasPlacementArguments(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(lparen))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result =
      !LexLookingAt(syntax->lex, TOK(rparen)) && !SyntaxLookingAtType(syntax);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}


// Reports whether construction of a class with virtual bases needs the hidden
// complete-object argument.
static bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type) {
  return TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
         StructHasVirtualBases(type->info.struct_info);
}


// Prepends the hidden complete-object flag required by constructors of classes
// with virtual bases.
static void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals,
                                             SourceLocation location) {
  if (actuals == NULL || !TypeNeedsCXXCompleteObjectArgument(type)) {
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


// Builds a constructor call whose receiver is the object addressed by an
// allocation pointer.
static ASTNode* NewCXXConstructorCallForPointer(TypeRecord* allocated_type,
                                                Symbol* ptr,
                                                Vector* actuals,
                                                SourceLocation location) {
  ASTNode* receiver = NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                                      NewIdentifierASTNode(ptr, location));
  return NewCXXConstructorCallForReceiver(allocated_type, receiver, actuals,
                                          location);
}


// Builds a constructor member call for an explicit object receiver.
static ASTNode* NewCXXConstructorCallForReceiver(TypeRecord* allocated_type,
                                                 ASTNode* receiver,
                                                 Vector* actuals,
                                                 SourceLocation location) {
  CXXPrependCompleteObjectArgument(allocated_type, actuals, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(allocated_type->info.struct_info
                                             ->tag_name->value),
                               NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}


// Finds the destructor member of a class type.
static StructMember* FindCXXDestructorForType(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "~");
  StringAppendString(&name, type->info.struct_info->tag_name);
  StructMember* destructor = FindStructMember(type->info.struct_info, &name);
  StringDestruct(&name);
  if (destructor == NULL || !destructor->is_member_function ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}


// Builds a destructor or dependent pseudo-destructor call for the object
// addressed by a pointer.
static ASTNode* NewCXXDestructorCallForPointer(TypeRecord* object_type,
                                               Symbol* ptr,
                                               SourceLocation location) {
  String destructor_name;
  StringInit(&destructor_name, "~");
  // A dependent type's destructor is named once the type is known (or the
  // call becomes a pseudo-destructor no-op).
  if (TypeIsStructOrUnion(object_type) &&
      object_type->info.struct_info != NULL &&
      object_type->info.struct_info->tag_name != NULL) {
    StringAppendString(&destructor_name,
                       object_type->info.struct_info->tag_name);
  }
  ASTNode* receiver =
      NewUnaryASTNode(AST_OP(contents), object_type, location,
                      NewIdentifierASTNode(ptr, location));
  ASTNode* member =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  StringDestruct(&destructor_name);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(object_type, actuals, location);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                          actuals);
}


// Wraps an expression in an expression-statement AST node.
static ASTNode* NewExpressionStatement(ASTNode* expr, SourceLocation location) {
  return NewExpressionStatementASTNode(expr, location);
}


// Builds a typed assignment expression.
static ASTNode* NewAssign(ASTNode* left, ASTNode* right, TypeRecord* type,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(assign), type, location, left, right);
}


// Builds an explicit cast expression with a copied destination type.
static ASTNode* NewTypedCast(TypeRecord* type, ASTNode* expr,
                             SourceLocation location) {
  ASTNode* result = NewCastASTNode(type, location, expr);
  ((CastASTNode*)result)->kind = kCastStatic;
  return result;
}


// Builds pointer addition used by lowered array allocation code.
static ASTNode* NewPtrAdd(ASTNode* ptr, ASTNode* offset,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(plus), NULL, location, ptr, offset);
}


// Builds pointer subtraction used by lowered array deallocation code.
static ASTNode* NewPtrSub(ASTNode* ptr, ASTNode* offset,
                          SourceLocation location) {
  return NewBinaryASTNode(AST_OP(minus), NULL, location, ptr, offset);
}


// Builds an int constant used by lowered new and delete expressions.
static ASTNode* NewIntLiteral(int64_t value, SourceLocation location) {
  return NewIntConstantASTNode(value,
                               NewTypeRecordWithSize(kTypeInt, kQualConst),
                               location);
}


// Builds a post-increment expression used by generated initialization loops.
static ASTNode* NewPostIncrement(Symbol* sym, SourceLocation location) {
  return NewUnaryASTNode(AST_OP(postinc), NULL, location,
                         NewIdentifierASTNode(sym, location));
}


// `constructed = index + 1`: element `index` of a new-expression's array is
// now constructed, so a later exception must destroy it.
static ASTNode* NewArrayConstructedCountUpdate(Symbol* constructed,
                                               ASTNode* index,
                                               SourceLocation location) {
  return NewExpressionStatement(
      NewAssign(NewIdentifierASTNode(constructed, location),
                NewBinaryASTNode(AST_OP(plus), NULL, location, index,
                                 NewIntLiteral(1, location)),
                constructed->type, location),
      location);
}


// Default-constructs elements [first, count) of the array at `ptr`, recording
// each completed element in `constructed` when it is non-NULL.
static ASTNode* NewArrayConstructionLoop(Syntax* syntax, TypeRecord* element_type,
                                         Symbol* ptr, Symbol* count,
                                         int64_t first, Symbol* constructed,
                                         SourceLocation location) {
  Symbol* index =
      SyntaxNewTemporary(syntax, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  Symbol* element_ptr =
      SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, element_type));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewIntLiteral(first, location), index->type,
                             location),
                   location));

  Vector* body_statements = NewVector();
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(element_ptr, location),
                             NewPtrAdd(NewIdentifierASTNode(ptr, location),
                                       NewIdentifierASTNode(index, location),
                                       location),
                             element_ptr->type, location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewCXXConstructorCallForPointer(element_type, element_ptr,
                                                   NewVector(), location),
                   location));
  if (constructed != NULL) {
    VectorAppend(body_statements,
                 NewArrayConstructedCountUpdate(
                     constructed, NewIdentifierASTNode(index, location),
                     location));
  }
  VectorAppend(body_statements,
               NewExpressionStatement(NewPostIncrement(index, location),
                                      location));
  ASTNode* body = NewCompoundStatementASTNode(body_statements, location);
  ASTNode* cond =
      NewBinaryASTNode(AST_OP(less), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIdentifierASTNode(count, location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(AST_OP(while), cond, body,
                                           location));
  return NewCompoundStatementASTNode(statements, location);
}


// Value-initializes elements [first, count) of the array at `ptr`, recording
// each completed element in `constructed` when it is non-NULL.
static ASTNode* NewArrayValueInitializationLoop(
    Syntax* syntax, TypeRecord* element_type, Symbol* ptr, Symbol* count,
    int64_t first, Symbol* constructed, SourceLocation location) {
  Symbol* index =
      SyntaxNewTemporary(syntax, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewIntLiteral(first, location), index->type,
                             location),
                   location));
  ASTNode* element =
      NewUnaryASTNode(AST_OP(contents), element_type, location,
                      NewPtrAdd(NewIdentifierASTNode(ptr, location),
                                NewIdentifierASTNode(index, location),
                                location));
  Vector* body_statements = NewVector();
  ASTNode* init =
      NewCXXNewInitialization(syntax, element_type, element, NULL, location);
  if (TypeContainsTemplateParameter(element_type)) {
    // Instantiation rewrites this into the element type's value-init.
    init->flags |= kASTDependentNewInitializer | kASTDependentNewValueInit;
  }
  VectorAppend(body_statements, NewExpressionStatement(init, location));
  if (constructed != NULL) {
    VectorAppend(body_statements,
                 NewArrayConstructedCountUpdate(
                     constructed, NewIdentifierASTNode(index, location),
                     location));
  }
  VectorAppend(body_statements,
               NewExpressionStatement(NewPostIncrement(index, location),
                                      location));
  ASTNode* condition =
      NewBinaryASTNode(AST_OP(less), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIdentifierASTNode(count, location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(
                   AST_OP(while), condition,
                   NewCompoundStatementASTNode(body_statements, location),
                   location));
  return NewCompoundStatementASTNode(statements, location);
}


// Destroys elements [0, count) of the array at `ptr`, last element first.
static ASTNode* NewArrayDestructionLoop(Syntax* syntax, TypeRecord* element_type,
                                        Symbol* ptr, Symbol* count,
                                        SourceLocation location) {
  Symbol* index = SyntaxNewTemporary(syntax, NewSizeTypeRecord());
  Symbol* element_ptr =
      SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, element_type));
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewIdentifierASTNode(count, location),
                             index->type, location),
                   location));

  Vector* body_statements = NewVector();
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(index, location),
                             NewPtrSub(NewIdentifierASTNode(index, location),
                                       NewIntLiteral(1, location), location),
                             index->type, location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(element_ptr, location),
                             NewPtrAdd(NewIdentifierASTNode(ptr, location),
                                       NewIdentifierASTNode(index, location),
                                       location),
                             element_ptr->type, location),
                   location));
  VectorAppend(body_statements,
               NewExpressionStatement(
                   NewCXXDestructorCallForPointer(element_type, element_ptr,
                                                  location),
                   location));
  ASTNode* body = NewCompoundStatementASTNode(body_statements, location);
  ASTNode* cond =
      NewBinaryASTNode(AST_OP(noteq), NULL, location,
                       NewIdentifierASTNode(index, location),
                       NewIntLiteral(0, location));
  VectorAppend(statements,
               NewCombinedStatementASTNode(AST_OP(while), cond, body,
                                           location));
  return NewCompoundStatementASTNode(statements, location);
}


// Clone callback that preserves node identity semantics while copying
// deduction arguments.
static ASTNode* IdentityCloneNodeForCXXNew(ASTNode* node, void* data) {
  (void)data;
  return node;
}


// Clones and analyzes constructor arguments before class template argument
// deduction.
static Vector* CloneAndAnalyzeCXXNewDeductionActuals(Vector* actuals) {
  Vector* clones = NewVector();
  for (size_t i = 0; actuals != NULL && i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual == NULL) {
      continue;
    }
    ASTNode* clone =
        ASTNodeClone(actual, IdentityCloneNodeForCXXNew, NULL, NULL);
    VectorAppend(clones, AnalyzeExpression(clone));
  }
  return clones;
}


// Performs class template argument deduction for a placeholder new-type-id.
static TypeRecord* DeduceCXXNewAllocatedType(Syntax* syntax,
                                             TypeRecord* allocated_type,
                                             Vector* actuals) {
  if (!CompilerIsCXX() || syntax == NULL ||
      !TypeIsClassTemplatePlaceholder(allocated_type) || actuals == NULL) {
    return allocated_type;
  }
  Symbol* class_template = TypeClassTemplatePlaceholderOrigin(allocated_type);
  Vector* deduction_actuals = CloneAndAnalyzeCXXNewDeductionActuals(actuals);
  bool alias_rejected = false;
  TypeRecord* deduced =
      TypeDeduceClassTemplateFromPlaceholder(syntax, allocated_type,
                                             deduction_actuals,
                                             /*allow_explicit=*/true,
                                             &alias_rejected);
  VectorDeleteWithContents(deduction_actuals,
                           (VectorElementDestructor)ASTNodeDelete,
                           /*free_element=*/false);
  if (deduced != NULL &&
      !TypeClassTemplatePlaceholderAcceptsDeduced(allocated_type, deduced)) {
    SyntaxError(syntax,
                "Deduced template arguments do not match alias template");
    TypeRecordDelete(deduced);
    deduced = NULL;
  }
  if (deduced == NULL) {
    if (alias_rejected) {
      SyntaxError(syntax,
                  "Deduced template arguments do not match alias template");
    } else {
      SyntaxError(syntax, "Could not deduce template arguments for %s",
                  class_template != NULL ? class_template->name.value
                                         : "<class template>");
    }
    return allocated_type;
  }
  TypeRecordDelete(allocated_type);
  return deduced;
}


// Deduces a new-type-id containing `auto` from its sole initializer, as for
// `auto x(e);` ([expr.new]/2, [dcl.type.auto.deduct]).  Inside a template a
// type-dependent initializer allocates `decltype(auto(e))`, which yields the
// same decayed type once substitution makes `e` concrete.
static TypeRecord* DeduceCXXNewPlaceholderType(Syntax* syntax,
                                               TypeRecord* allocated_type,
                                               Vector* actuals,
                                               SourceLocation location) {
  ASTNode* actual =
      actuals != NULL && actuals->length == 1 ? actuals->value.p[0] : NULL;
  if (actual == NULL || actual->op == AST_OP(braced_init)) {
    SyntaxError(syntax, "new-expression with a placeholder type requires "
                        "exactly one initializer expression");
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  TypeRecord* leaf = allocated_type;
  while (leaf->next != NULL) {
    leaf = leaf->next;
  }
  if ((leaf->type & kTypeDecltypeAuto) != 0) {
    SyntaxError(syntax, "decltype(auto) in a new-expression is not supported");
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  Vector* analyzed_actuals = CloneAndAnalyzeCXXNewDeductionActuals(actuals);
  ASTNode* analyzed = analyzed_actuals->value.p[0];
  TypeRecord* init_type = analyzed != NULL ? analyzed->type : NULL;
  TypeRecord* function = compiler->current_function;
  bool dependent_context =
      syntax->current_template_parameters != NULL ||
      (function != NULL && TypeIsFunction(function) &&
       FunctionExtraOf(&function->info.function)->template_parameters.length >
           0);
  bool dependent =
      dependent_context &&
      (init_type == NULL || TypeIsUnknown(init_type) ||
       TypeContainsAuto(init_type) || TypeContainsTemplateParameter(init_type) ||
       ExpressionIsTemplateDependent(analyzed));
  TypeRecord* deduced = NULL;
  if (dependent) {
    ASTNode* operand = ASTNodeClone(actual, IdentityCloneNodeForCXXNew, NULL, NULL);
    ASTNode* decay_copy =
        NewCastASTNode(NewTypeRecord(kTypeAuto, kQualPlain), location, operand);
    ((CastASTNode*)decay_copy)->kind = kCastAutoParen;
    deduced =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, GetQualifiers(leaf));
    TypeMutableDependent(deduced)->dependent_decltype_expr = decay_copy;
  } else if (init_type != NULL && !TypeIsUnknown(init_type) &&
             !TypeContainsAuto(init_type)) {
    TypeRecord* decayed = TypeDecayForByValueDeduction(init_type);
    deduced = TypeDeduceAuto(allocated_type, decayed);
    TypeRecordDelete(decayed);
  }
  VectorDeleteWithContents(analyzed_actuals,
                           (VectorElementDestructor)ASTNodeDelete,
                           /*free_element=*/false);
  if (deduced == NULL) {
    SyntaxError(syntax, "Cannot deduce the allocated type of a new-expression");
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  return TypeRecordCalculateSize(deduced);
}


// Parses and retains initializer arguments needed for class template argument
// deduction in new.
static Vector* ParseCXXNewDeductionInitializerArguments(Syntax* syntax,
                                                        TypeRecord* type,
                                                        Token open,
                                                        TokenClass followers) {
  if (open == TOK(lbrace) &&
      CXXNewConstructorSetHasInitializerList(FindCXXConstructorForType(type))) {
    Vector* actuals = NewVector();
    LexNextToken(syntax->lex);
    VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    return actuals;
  }
  return ParseCXXNewInitializerArguments(syntax, open, followers);
}


// Build a `(type){ exprs... }` compound literal used to initialize a freshly
// `new`-allocated object that has no constructor (a scalar aggregate wrapper or
// a class aggregate).  With no expressions this performs value-initialization
// (zero-initialization); with expressions it performs aggregate initialization.
// `exprs`, when non-NULL, is consumed (its elements are re-wrapped and the
// vector is deleted).
static ASTNode* NewCXXNewCompoundLiteralInitializer(Syntax* syntax,
                                                    TypeRecord* type,
                                                    Vector* exprs,
                                                    SourceLocation location) {
  Symbol* storage = SyntaxNewTemporary(syntax, type);
  Vector* elements = NewVector();
  if (exprs != NULL) {
    for (size_t i = 0; i < exprs->length; i++) {
      ASTNode* expr = exprs->value.p[i];
      VectorAppend(elements,
                   expr->op == AST_OP(braced_init)
                       ? expr
                       : NewExpressionInitializerASTNode(expr, location));
    }
    VectorDelete(exprs);
  }
  ASTNode* braced = NewBracedInitializerASTNode(elements, NULL, location);
  return NewCompoundLiteralASTNode(NewIdentifierASTNode(storage, location),
                                   location, braced);
}


// A class that `new T{...}` and `new T(...)` aggregate-initialize.  Its
// implicit constructors do not make it a constructor call.  A class template's
// primary struct can carry a stale `is_aggregate`, so require that no
// constructor is user-declared as well.
static bool CXXNewTypeIsAggregateClass(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      !type->info.struct_info->is_aggregate ||
      type->info.struct_info->is_template) {
    return false;
  }
  for (StructMember* c = FindCXXConstructorForType(type); c != NULL;
       c = c->overload_next) {
    if (c->is_member_function && c->symbol != NULL &&
        !c->symbol->flags.invented) {
      return false;
    }
  }
  return true;
}


// A braced list holding `items` (each an expression or a braced list), which
// it consumes; NULL makes an empty list.
static ASTNode* NewCXXNewBracedInitializer(Vector* items,
                                           SourceLocation location) {
  Vector* elements = NewVector();
  for (size_t i = 0; items != NULL && i < items->length; i++) {
    ASTNode* item = items->value.p[i];
    VectorAppend(elements,
                 item->op == AST_OP(braced_init)
                     ? item
                     : NewExpressionInitializerASTNode(item, location));
  }
  if (items != NULL) {
    VectorDelete(items);
  }
  return NewBracedInitializerASTNode(elements, NULL, location);
}


// The scalar `*p` a new-expression stores its initial value through.  The
// object is being initialized, so a `const T` is stored through a `T*`.
static ASTNode* CXXNewScalarTarget(ASTNode* target, TypeRecord* type,
                                   SourceLocation location) {
  if (type == NULL || (GetQualifiers(type) & kQualConst) == 0 ||
      target == NULL || target->op != AST_OP(contents)) {
    return target;
  }
  TypeRecord* plain = TypeRecordCopy(type);
  SetQualifiers(plain, GetQualifiers(plain) & ~kQualConst);
  UnaryASTNode* contents = (UnaryASTNode*)target;
  contents->sub = NewTypedCast(NewPointerTo(kQualPlain, plain), contents->sub,
                               location);
  return target;
}


// Initializes the new object `target` of non-array `type` from `initializer`
// (an expression, a braced list, or NULL to value-initialize).  A class is
// initialized in place from a braced list -- assigning would run its
// assignment operator on storage that holds no object yet.  A scalar is
// assigned.
static ASTNode* NewCXXNewInitialization(Syntax* syntax, TypeRecord* type,
                                        ASTNode* target, ASTNode* initializer,
                                        SourceLocation location) {
  if (TypeIsStructOrUnion(type)) {
    ASTNode* braced = initializer;
    if (braced == NULL || braced->op != AST_OP(braced_init)) {
      Vector* items = NewVector();
      if (initializer != NULL) {
        VectorAppend(items, initializer);
      }
      braced = NewCXXNewBracedInitializer(items, location);
    }
    target->flags |= kASTNeedAddress;
    return NewBinaryASTNode(AST_OP(init), type, location, target, braced);
  }
  ASTNode* value = initializer;
  if (value == NULL) {
    value = NewIntLiteral(0, location);
  } else if (value->op == AST_OP(braced_init)) {
    // `new int[2]{{1}, {}}`: a braced scalar holds at most one expression.
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)value;
    ASTNode* element = braced->initializers->length == 1
                           ? braced->initializers->value.p[0]
                           : NULL;
    if (braced->initializers->length == 0) {
      value = NewIntLiteral(0, location);
    } else if (element == NULL || element->op != AST_OP(expr_init)) {
      SyntaxError(syntax, "new initializer for non-class type requires one "
                          "expression");
      value = NewIntLiteral(0, location);
    } else {
      value = ((ExpressionInitializerASTNode*)element)->expr;
    }
  }
  return NewAssign(CXXNewScalarTarget(target, type, location), value, type,
                   location);
}


// The non-array type at the bottom of `type`, and how many of it one `type`
// holds: int[2][3] is 6 ints.
static TypeRecord* CXXNewArrayBaseType(TypeRecord* type, int64_t* count) {
  *count = 1;
  while (TypeIsArray(type)) {
    *count *= (int64_t)type->info.array.size.fixed;
    type = type->next;
  }
  return type;
}


// Returns the elements of a braced new-initializer, diagnosing a malformed
// initializer node.
static Vector* CXXNewBracedItems(Syntax* syntax, ASTNode* braced_node) {
  Vector* elements = ((BracedInitializerASTNode*)braced_node)->initializers;
  Vector* items = NewVector();
  for (size_t i = 0; i < elements->length; i++) {
    ASTNode* element = elements->value.p[i];
    if (element->op == AST_OP(expr_init)) {
      element = ((ExpressionInitializerASTNode*)element)->expr;
    } else if (element->op != AST_OP(braced_init)) {
      if (syntax != NULL) {
        SyntaxError(syntax,
                    "designated initializers cannot initialize an array");
      }
      continue;
    }
    VectorAppend(items, element);
  }
  return items;
}


// Appends initialization code for one flattened array element and updates the
// constructed count.
static void AppendCXXNewLeafInit(CXXNewArrayInit* init, int64_t index,
                                 ASTNode* initializer) {
  SourceLocation location = init->location;
  TypeRecord* type = init->base_type;
  ASTNode* element = NewUnaryASTNode(
      AST_OP(contents), type, location,
      NewPtrAdd(NewIdentifierASTNode(init->base_ptr, location),
                NewIntLiteral(index, location), location));
  ASTNode* statement;
  if (init->constructed_class) {
    // Copy-initialize from the initializer; a braced list supplies the
    // constructor arguments, and no initializer default-constructs.
    Vector* actuals = NewVector();
    if (initializer != NULL && initializer->op == AST_OP(braced_init) &&
        !CXXNewConstructorSetHasInitializerList(
            FindCXXConstructorForType(type))) {
      Vector* elements = ((BracedInitializerASTNode*)initializer)->initializers;
      for (size_t i = 0; i < elements->length; i++) {
        ASTNode* actual = elements->value.p[i];
        VectorAppend(actuals,
                     actual->op == AST_OP(expr_init)
                         ? ((ExpressionInitializerASTNode*)actual)->expr
                         : actual);
      }
    } else if (initializer != NULL) {
      VectorAppend(actuals, initializer);
    }
    statement =
        NewCXXConstructorCallForReceiver(type, element, actuals, location);
  } else {
    statement = NewCXXNewInitialization(init->syntax, type, element,
                                        initializer, location);
  }
  VectorAppend(init->statements, NewExpressionStatement(statement, location));
  if (init->constructed != NULL) {
    VectorAppend(init->statements,
                 NewArrayConstructedCountUpdate(
                     init->constructed, NewIntLiteral(index, location),
                     location));
  }
}


// Recursively consumes brace elision and initializes one array or aggregate
// subobject.
static void InitCXXNewSubobject(CXXNewArrayInit* init,
                                CXXNewInitCursor* cursor, TypeRecord* type,
                                int64_t index) {
  if (TypeIsArray(type)) {
    int64_t stride = 0;
    CXXNewArrayBaseType(type->next, &stride);
    size_t bound = (size_t)type->info.array.size.fixed;
    ASTNode* item = cursor->next < cursor->items->length
                        ? cursor->items->value.p[cursor->next]
                        : NULL;
    if (item != NULL && item->op == AST_OP(braced_init)) {
      cursor->next++;
      CXXNewInitCursor inner = {CXXNewBracedItems(init->syntax, item), 0};
      for (size_t i = 0; i < bound; i++) {
        InitCXXNewSubobject(init, &inner, type->next,
                            index + (int64_t)i * stride);
      }
      if (inner.next < inner.items->length && init->statements != NULL) {
        SyntaxError(init->syntax, "Too many initializers");
      }
      VectorDelete(inner.items);
      return;
    }
    // Brace elision: the subarray takes its elements from the enclosing list.
    for (size_t i = 0; i < bound; i++) {
      InitCXXNewSubobject(init, cursor, type->next,
                          index + (int64_t)i * stride);
    }
    return;
  }
  ASTNode* item = cursor->next < cursor->items->length
                      ? cursor->items->value.p[cursor->next++]
                      : NULL;
  if (init->statements != NULL) {
    AppendCXXNewLeafInit(init, index, item);
  }
}


// Initializes the leading elements of the allocation from `items` and returns
// how many elements of type `element_type` they covered.
static size_t InitCXXNewArrayElements(CXXNewArrayInit* init, Vector* items,
                                      TypeRecord* element_type) {
  int64_t stride = 0;
  CXXNewArrayBaseType(element_type, &stride);
  CXXNewInitCursor cursor = {items, 0};
  size_t elements = 0;
  while (cursor.next < items->length) {
    InitCXXNewSubobject(init, &cursor, element_type,
                        (int64_t)elements * stride);
    elements++;
  }
  return elements;
}


// Lower an array new-expression to a statement-expression. It stores the
// element count (and, except for placement new, a cookie before the objects),
// initializes each element, and yields the pointer to the first object.
// `result` is the allocation call already cast to the array pointer type.
static ASTNode* BuildCXXArrayNew(
    Syntax* syntax, SourceLocation location, TypeRecord* allocated_type,
    ASTNode* array_size, Symbol* array_count, Vector* array_initializers,
    ASTNode* result, TypeRecord* result_type, bool standard_placement,
    bool value_init, bool allocated_type_dependent, bool eh_cleanup,
    bool deallocate_on_throw, bool global_scope) {
  TypeRecord* size_type = NewSizeTypeRecord();
  TypeRecord* size_ptr_type = NewPointerTo(kQualPlain, size_type);
  Symbol* object_ptr = SyntaxNewTemporary(syntax, result_type);
  Symbol* header = standard_placement
      ? NULL : SyntaxNewTemporary(syntax, size_ptr_type);
  Vector* statements = NewVector();
  if (standard_placement) {
    VectorAppend(statements,
                 NewVariableDeclarationASTNode(
                     array_count,
                     NewExpressionInitializerASTNode(
                         NewIntLiteral(0, location), location),
                     location));
    VectorAppend(statements,
                 NewVariableDeclarationASTNode(object_ptr, NULL, location));
  }
  VectorAppend(statements,
               NewExpressionStatement(
                   NewAssign(NewIdentifierASTNode(array_count, location),
                             array_size, array_count->type, location),
                   location));
  if (standard_placement) {
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(object_ptr, location),
                               result, object_ptr->type, location),
                     location));
    TypeRecordDelete(size_ptr_type);
  } else {
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(header, location),
                               NewTypedCast(size_ptr_type, result, location),
                               header->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewUnaryASTNode(AST_OP(contents), size_type,
                                              location,
                                              NewIdentifierASTNode(header,
                                                                   location)),
                               NewIdentifierASTNode(array_count, location),
                               size_type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(object_ptr, location),
                               NewTypedCast(
                                   result_type,
                                   NewPtrAdd(NewIdentifierASTNode(header,
                                                                 location),
                                             NewIntLiteral(1, location),
                                             location),
                                   location),
                               object_ptr->type, location),
                     location));
  }
  // Elements are initialized through a flat view of the allocation as an
  // array of its non-array base type: `new T[n][2]` holds 2n T objects.
  int64_t base_per_element = 1;
  TypeRecord* base_type =
      CXXNewArrayBaseType(allocated_type, &base_per_element);
  Symbol* base_ptr = object_ptr;
  Symbol* base_count = array_count;
  if (TypeIsArray(allocated_type)) {
    base_ptr =
        SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, base_type));
    base_count = SyntaxNewTemporary(syntax, NewSizeTypeRecord());
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(base_ptr, location),
                               NewTypedCast(base_ptr->type,
                                            NewIdentifierASTNode(object_ptr,
                                                                 location),
                                            location),
                               base_ptr->type, location),
                     location));
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(base_count, location),
                               NewBinaryASTNode(
                                   AST_OP(mult), NULL, location,
                                   NewIdentifierASTNode(array_count,
                                                        location),
                                   NewIntLiteral(base_per_element,
                                                 location)),
                               base_count->type, location),
                     location));
  }
  bool constructed_class =
      TypeIsStructOrUnion(base_type) &&
      FindCXXConstructorForType(base_type) != NULL &&
      (array_initializers == NULL || allocated_type_dependent ||
       !CXXNewTypeIsAggregateClass(base_type));
  // Element initialization goes in its own block, headed by a cleanup that
  // runs only if it throws: destroy the completed elements, last first,
  // then free the storage.
  bool init_may_throw = constructed_class || value_init ||
                        allocated_type_dependent;
  bool base_dependent = TypeContainsTemplateParameter(base_type);
  // A dependent element type with no initializer is initialized at
  // instantiation, where a class runs its default constructor.
  bool dependent_default_init =
      base_dependent && !constructed_class && !value_init &&
      array_initializers == NULL;
  bool destroy_on_throw = eh_cleanup && init_may_throw &&
                          (base_dependent ||
                           FindCXXDestructorForType(base_type) != NULL);
  bool free_on_throw = deallocate_on_throw && init_may_throw;
  Symbol* constructed = NULL;
  Vector* init_statements = statements;
  if (destroy_on_throw || free_on_throw) {
    init_statements = NewVector();
    VectorAppend(init_statements, NULL);  // The cleanup, built below.
  }
  if (destroy_on_throw) {
    constructed = SyntaxNewTemporary(syntax, NewSizeTypeRecord());
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(constructed, location),
                               NewIntLiteral(0, location),
                               constructed->type, location),
                     location));
  }
  size_t listed = 0;
  bool array_initializers_given = array_initializers != NULL;
  if (array_initializers != NULL) {
    CXXNewArrayInit init = {
        .syntax = syntax,
        .statements = init_statements,
        .base_ptr = base_ptr,
        .base_type = base_type,
        .constructed_class = constructed_class,
        .constructed = constructed,
        .location = location,
    };
    listed = InitCXXNewArrayElements(&init, array_initializers,
                                     allocated_type);
    VectorDelete(array_initializers);
  }
  int64_t first_unlisted = (int64_t)listed * base_per_element;
  // When the list covers a constant bound no element is left to default
  // construct, so the class need not have a default constructor.
  int64_t bound = 0;
  bool all_listed = array_initializers_given && array_size != NULL &&
                    array_size->op == AST_OP(number) &&
                    EvaluateIntegerExpression(array_size, &bound) &&
                    bound * base_per_element == first_unlisted;
  if (constructed_class) {
    if (!all_listed) {
      VectorAppend(init_statements,
                   NewArrayConstructionLoop(syntax, base_type, base_ptr,
                                            base_count, first_unlisted,
                                            constructed, location));
    }
  } else if (value_init || dependent_default_init) {
    // The elements without an initializer are value-initialized.
    VectorAppend(init_statements,
                 NewArrayValueInitializationLoop(
                     syntax, base_type, base_ptr, base_count,
                     first_unlisted, constructed, location));
  }
  if (init_statements != statements) {
    Vector* cleanup_statements = NewVector();
    if (destroy_on_throw) {
      VectorAppend(cleanup_statements,
                   NewArrayDestructionLoop(syntax, base_type, base_ptr,
                                           constructed, location));
    }
    if (free_on_throw) {
      Vector* deallocate_actuals = NewVector();
      VectorAppend(deallocate_actuals,
                   NewDeallocationPointer(
                       NewIdentifierASTNode(header, location), location));
      VectorAppend(cleanup_statements,
                   NewExpressionStatement(
                       NewCallASTNode(GetCXXOperatorDeleteForType(
                                          allocated_type, true, location,
                                          global_scope),
                                      location, deallocate_actuals),
                       location));
    }
    ASTNode* cleanup =
        NewCompoundStatementASTNode(cleanup_statements, location);
    cleanup->flags |= kASTEHCleanupOnly;
    init_statements->value.p[0] = cleanup;
    VectorAppend(statements,
                 NewCompoundStatementASTNode(init_statements, location));
  }
  VectorAppend(statements,
               NewExpressionStatement(NewIdentifierASTNode(object_ptr,
                                                           location),
                                      location));
  return NewUnaryASTNode(AST_OP(stmt_expr), result_type, location,
                           NewCompoundStatementASTNode(statements, location));
}

// Lower a non-array new-expression that has an initializer or a constructor
// call. The allocation is stored in a temporary, the object is initialized
// through that temporary, and the temporary is the value of the expression.
// If initialization can throw, a cleanup frees the storage.
static ASTNode* BuildCXXObjectNew(
    Syntax* syntax, SourceLocation location, TypeRecord* allocated_type,
    ASTNode* result, TypeRecord* result_type, Vector* ctor_actuals,
    ASTNode* scalar_initializer, ASTNode* aggregate_initializer,
    bool value_init, bool standard_placement, bool deallocate_on_throw,
    bool global_scope, bool allocated_type_dependent) {
  Symbol* temp = SyntaxNewTemporary(syntax, result_type);
  // The lowered comma expression assigns through this temporary and then
  // reuses it as the constructor receiver.  Keep it in addressable storage;
  // targets cannot represent that sequence with a register-only variable.
  temp->flags.address_taken = true;
  ASTNode* temp_lhs = NewIdentifierASTNode(temp, location);
  temp_lhs->flags |= kASTNeedAddress;
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), result_type, location,
                       temp_lhs, result);
  ASTNode* init;
  if (ctor_actuals != NULL) {
    if (TypeContainsTemplateParameter(allocated_type)) {
      init = NewAssign(
          NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                          NewIdentifierASTNode(temp, location)),
          NewBracedInitializerASTNode(ctor_actuals, NULL, location),
          allocated_type, location);
      init->flags |= kASTDependentNewInitializer;
    } else {
      init = NewCXXConstructorCallForPointer(allocated_type, temp, ctor_actuals,
                                             location);
    }
  } else if (aggregate_initializer != NULL) {
    init = NewCXXNewInitialization(
        syntax, allocated_type,
        NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                        NewIdentifierASTNode(temp, location)),
        aggregate_initializer, location);
  } else {
    init = NewAssign(
        CXXNewScalarTarget(
            NewUnaryASTNode(AST_OP(contents), allocated_type, location,
                            NewIdentifierASTNode(temp, location)),
            allocated_type, location),
        scalar_initializer, allocated_type, location);
  }
  bool defer_init_until_instantiation =
      TypeContainsTemplateParameter(allocated_type);
  if (!defer_init_until_instantiation && standard_placement &&
      scalar_initializer != NULL &&
      scalar_initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced =
        (BracedInitializerASTNode*)scalar_initializer;
    defer_init_until_instantiation = braced->initializers->length > 1;
  }
  if (ctor_actuals == NULL && defer_init_until_instantiation) {
    init->flags |= kASTDependentNewInitializer;
    if (value_init) {
      init->flags |= kASTDependentNewValueInit;
    }
  }
  Vector* statements = NewVector();
  VectorAppend(statements,
               NewVariableDeclarationASTNode(temp, NULL, location));
  VectorAppend(statements, NewExpressionStatement(assign, location));
  bool init_may_throw =
      ctor_actuals != NULL || aggregate_initializer != NULL ||
      allocated_type_dependent || TypeIsStructOrUnion(allocated_type) ||
      (scalar_initializer != NULL && scalar_initializer->op != AST_OP(number));
  if (deallocate_on_throw && init_may_throw) {
    // Free the storage if the initialization throws.
    Vector* init_statements = NewVector();
    VectorAppend(init_statements,
                 NewCXXNewDeallocationCleanup(temp, allocated_type,
                                              global_scope, location));
    VectorAppend(init_statements, NewExpressionStatement(init, location));
    VectorAppend(statements,
                 NewCompoundStatementASTNode(init_statements, location));
  } else {
    VectorAppend(statements, NewExpressionStatement(init, location));
  }
  VectorAppend(statements,
               NewExpressionStatement(NewIdentifierASTNode(temp, location),
                                      location));
  return NewUnaryASTNode(AST_OP(stmt_expr), result_type, location,
                          NewCompoundStatementASTNode(statements, location));
}

// new-expression. Parses placement arguments, the type, and the initializer,
// allocates with operator new, then either BuildCXXArrayNew or
// BuildCXXObjectNew finishes the initialization.
ASTNode* ParseCXXNewExpression(Syntax* syntax, TokenClass followers,
                                      bool global_scope) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // new

  Vector* placement_actuals = NULL;
  if (CXXNewHasPlacementArguments(syntax)) {
    placement_actuals =
        ParseCXXNewInitializerArguments(syntax, TOK(lparen), followers);
  }

  // `new (T)` names its type with a full type-id, whose declarator may hold
  // parentheses (`new (int (*)[3])`).
  bool parenthesized_type = LexLookingAt(syntax->lex, TOK(lparen));
  if (parenthesized_type) {
    LexNextToken(syntax->lex);
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* allocated_type = TypeParserParseType(&parser, true);
  if (parenthesized_type && allocated_type != NULL) {
    Symbol* sym = TypeParserParseDeclarator(&parser, allocated_type);
    if (sym != NULL) {
      allocated_type = sym->type;
    }
  }
  if (parenthesized_type) {
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
  }
  Token initializer_open = TOK(bad);
  ASTNode* array_size = NULL;
  if (LexLookingAt(syntax->lex, TOK(lparen)) ||
      LexLookingAt(syntax->lex, TOK(lbrace)) ||
      (!parenthesized_type && LexLookingAt(syntax->lex, TOK(lsquare)))) {
    initializer_open = syntax->lex->current_token;
  }
  // `new int*(&g)` initializes an int* rather than declaring `g`.
  parser.parsing_new_type_id = true;
  Symbol* sym = initializer_open == TOK(bad) && !parenthesized_type
      ? TypeParserParseDeclarator(&parser, allocated_type)
      : NULL;
  if (sym == NULL) {
    if (allocated_type == NULL) {
      SyntaxError(syntax, "Invalid type in new expression");
      allocated_type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    }
  } else {
    // Take the declarator symbol's reference: placeholder deduction below
    // releases the pattern, and `sym` is deleted at the end.
    allocated_type = sym->type;
    sym->type = NULL;
  }
  if (initializer_open == TOK(bad) && !parenthesized_type &&
      (LexLookingAt(syntax->lex, TOK(lparen)) ||
       LexLookingAt(syntax->lex, TOK(lbrace)) ||
       LexLookingAt(syntax->lex, TOK(lsquare)))) {
    initializer_open = syntax->lex->current_token;
  }
  TypeParserDestruct(&parser);
  Vector* ctor_actuals = NULL;
  if (TypeIsClassTemplatePlaceholder(allocated_type) &&
      (initializer_open == TOK(lparen) || initializer_open == TOK(lbrace))) {
    ctor_actuals = ParseCXXNewDeductionInitializerArguments(
        syntax, allocated_type, initializer_open, followers);
    allocated_type = DeduceCXXNewAllocatedType(syntax, allocated_type,
                                               ctor_actuals);
  }
  if (TypeIsClassTemplatePlaceholder(allocated_type)) {
    SyntaxError(syntax,
                "Class template argument deduction requires an initializer");
  }
  // The `auto` initializer, parsed for deduction and consumed below as the
  // new-initializer.
  Vector* placeholder_actuals = NULL;
  if (CompilerIsCXX() && TypeContainsAuto(allocated_type)) {
    if (initializer_open == TOK(lparen) || initializer_open == TOK(lbrace)) {
      placeholder_actuals =
          ParseCXXNewInitializerArguments(syntax, initializer_open, followers);
      TypeRecord* deduced = DeduceCXXNewPlaceholderType(
          syntax, allocated_type, placeholder_actuals, location);
      TypeRecordDelete(allocated_type);
      allocated_type = deduced;
      if (TypeIsUnknown(deduced) && !TypeContainsTemplateParameter(deduced)) {
        VectorDeleteWithContents(placeholder_actuals,
                                 (VectorElementDestructor)ASTNodeDelete,
                                 /*free_element=*/false);
        placeholder_actuals = NewVector();
      }
    } else {
      SyntaxError(syntax, "new-expression with a placeholder type requires "
                          "an initializer");
      allocated_type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    }
  }
  TypeRecordCalculateSize(allocated_type);
  bool allocated_type_dependent =
      TypeContainsTemplateParameter(allocated_type);
  if (TypeIsAbstractClass(allocated_type)) {
    SyntaxError(syntax, "Cannot allocate object of abstract class %s",
                allocated_type->info.struct_info->tag_name != NULL
                    ? allocated_type->info.struct_info->tag_name->value
                    : "<anonymous>");
  }
  SyntaxRejectIncompleteObjectType(syntax, allocated_type, false,
                                   "Allocation of incomplete type '%s'");

  ASTNode* scalar_initializer = NULL;
  // The braced list that initializes a class aggregate in place.
  ASTNode* aggregate_initializer = NULL;
  bool value_init = false;
  // The elements of an array new-initializer, each an expression or a braced
  // list; NULL when the array has no initializer.
  Vector* array_initializers = NULL;
  // `new int*[2]`, or `new A` for `typedef int A[2]`, allocates an array
  // whose element count is the outermost bound ([expr.new]).
  bool array_typed_new = initializer_open != TOK(lsquare) &&
                         TypeIsFixedArray(allocated_type) &&
                         allocated_type->next != NULL;
  if (array_typed_new) {
    array_size = NewIntLiteral(allocated_type->info.array.size.fixed, location);
    allocated_type = allocated_type->next;
  }
  if (initializer_open == TOK(lsquare) || array_typed_new) {
    if (!array_typed_new) {
      LexNextToken(syntax->lex);
      if (!LexLookingAt(syntax->lex, TOK(rsquare))) {
        array_size = SyntaxParseSingleExpression(syntax, TC(closebra));
      }
      SyntaxNeedBracket(syntax, TOK(rsquare), followers);
      // `new T[n][2][3]` allocates n objects of type T[2][3]; only the first
      // bound may be non-constant.
      Vector inner_bounds;
      VectorInit(&inner_bounds);
      while (LexMatch(syntax->lex, TOK(lsquare))) {
        ASTNode* bound = AnalyzeExpression(
            SyntaxParseSingleExpression(syntax, TC(closebra)));
        SyntaxNeedBracket(syntax, TOK(rsquare), followers);
        int64_t value = 0;
        if (!EvaluateIntegerExpression(bound, &value) || value <= 0) {
          SyntaxError(syntax, "array new bound after the first must be a "
                              "positive constant");
          value = 1;
        }
        VectorAppend(&inner_bounds, (void*)(intptr_t)value);
      }
      for (size_t i = inner_bounds.length; i > 0; i--) {
        TypeRecord* array_type = NewBasicArrayTypeRecord(
            kQualPlain, (int)(intptr_t)inner_bounds.value.p[i - 1], false);
        TypeRecordChain(array_type, allocated_type);
        allocated_type = TypeRecordCalculateSize(array_type);
      }
      VectorDestruct(&inner_bounds);
    }
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      array_initializers =
          CXXNewBracedItems(syntax, SyntaxParseBracedInitializer(syntax));
    } else if (LexLookingAt(syntax->lex, TOK(lparen))) {
      array_initializers =
          ParseCXXNewInitializerArguments(syntax, TOK(lparen), followers);
    }
    if (array_size == NULL) {
      size_t deduced = 0;
      if (array_initializers == NULL) {
        SyntaxError(syntax, "array new without a bound requires an "
                            "initializer");
      } else {
        CXXNewArrayInit counter = {.syntax = NULL};
        deduced = InitCXXNewArrayElements(&counter, array_initializers,
                                          allocated_type);
      }
      array_size = NewIntLiteral((int64_t)deduced, location);
    }
    if (array_initializers != NULL) {
      value_init = true;
    }
  } else if (ctor_actuals == NULL &&
             (initializer_open == TOK(lparen) || initializer_open == TOK(lbrace))) {
    if (!allocated_type_dependent &&
        CXXNewTypeIsAggregateClass(allocated_type)) {
      // `new T{...}` is aggregate initialization, as is the C++20
      // parenthesized form `new T(...)`.  A single element of type T copies
      // it ([dcl.init.list]/3.2); no element value-initializes.
      if (initializer_open == TOK(lbrace) && placeholder_actuals == NULL) {
        LexNextToken(syntax->lex);
        aggregate_initializer = SyntaxParseBracedInitializer(syntax);
      } else {
        aggregate_initializer = NewCXXNewBracedInitializer(
            TakeCXXNewInitializerArguments(syntax, &placeholder_actuals,
                                           initializer_open, followers),
            location);
      }
      value_init = ((BracedInitializerASTNode*)aggregate_initializer)
                       ->initializers->length == 0;
    } else if (FindCXXConstructorForType(allocated_type) == NULL) {
      Vector* initializers = TakeCXXNewInitializerArguments(
          syntax, &placeholder_actuals, initializer_open, followers);
      bool class_direct_init =
          !allocated_type_dependent && TypeIsStructOrUnion(allocated_type) &&
          allocated_type->info.struct_info != NULL &&
          allocated_type->info.struct_info->tag_name != NULL &&
          allocated_type->info.struct_info->is_class &&
          !allocated_type->info.struct_info->is_aggregate;
      if (initializers->length == 1) {
        if (class_direct_init) {
          // `new (p) T(arg)` on a class type performs direct/copy/move
          // construction, never assignment into uninitialized storage.
          ctor_actuals = initializers;
        } else {
          // `new T(x)` / `new T{x}`: direct/copy-initialization of a scalar (or a
          // same-typed class object).  For a dependent T this assignment is kept
          // and may be rewritten to `receiver.T(x)` once T resolves to a class.
          scalar_initializer = initializers->value.p[0];
          VectorDestruct(initializers);
        }
      } else if (initializers->length == 0) {
        // `new T()` / `new T{}`: value-initialization.  A scalar (or dependent
        // T, which is resolved during instantiation) is zero-initialized with a
        // plain assignment; a concrete aggregate is zero-initialized with an
        // empty compound literal so its members and padding are cleared.
        value_init = true;
        if (allocated_type_dependent ||
            (!TypeIsStructOrUnion(allocated_type) &&
                          !TypeIsArray(allocated_type))) {
          scalar_initializer = NewIntLiteral(0, location);
        } else {
          scalar_initializer = NewCXXNewCompoundLiteralInitializer(
              syntax, allocated_type, NULL, location);
        }
        VectorDelete(initializers);
      } else if (allocated_type_dependent ||
                 (placement_actuals != NULL &&
                  placement_actuals->length == 1 &&
                  initializers->length > 1)) {
        scalar_initializer =
            NewBracedInitializerASTNode(initializers, NULL, location);
      } else if (TypeIsStructOrUnion(allocated_type) ||
                 TypeIsArray(allocated_type)) {
        // `new T{a, b, ...}` (or the C++20 parenthesized aggregate form
        // `new T(a, b, ...)`): aggregate-initialize the object.
        scalar_initializer = NewCXXNewCompoundLiteralInitializer(
            syntax, allocated_type, initializers, location);
      } else {
        // A scalar type cannot take more than one new-initializer expression.
        SyntaxError(syntax, "new initializer for non-class type requires one expression");
        VectorDelete(initializers);
      }
    } else if (placeholder_actuals == NULL &&
               initializer_open == TOK(lbrace) &&
               CXXNewConstructorSetHasInitializerList(
                   FindCXXConstructorForType(allocated_type))) {
      // `new T{a, b}` list-initializes.  Keep the brace as one actual so an
      // initializer-list constructor is considered; overload resolution
      // expands the elements only when no such constructor is viable.
      LexNextToken(syntax->lex);
      ctor_actuals = NewVector();
      VectorAppend(ctor_actuals, SyntaxParseBracedInitializer(syntax));
    } else {
      // `new (p) T{a, b}` on a class with constructors is direct initialization
      // via the matching constructor, not aggregate/braced assignment into
      // storage (types such as ArenaBlock have const members and no assignment).
      ctor_actuals = TakeCXXNewInitializerArguments(
          syntax, &placeholder_actuals, initializer_open, followers);
    }
  } else if (ctor_actuals == NULL &&
             FindCXXConstructorForType(allocated_type) != NULL) {
    ctor_actuals = NewVector();
  }

  Vector* actuals = NewVector();
  ASTNode* sole_placement =
      placement_actuals != NULL && placement_actuals->length == 1
          ? placement_actuals->value.p[0] : NULL;
  TypeRecord* sole_placement_type =
      sole_placement != NULL ? sole_placement->type : NULL;
  if (sole_placement_type == NULL && sole_placement != NULL &&
      sole_placement->op == AST_OP(cast)) {
    sole_placement_type = ((CastASTNode*)sole_placement)->cast_type;
  }
  bool standard_placement =
      sole_placement_type != NULL && TypeIsPointer(sole_placement_type);
  ASTNode* allocation_size =
      allocated_type_dependent || TypeLayoutDependsOnTemplate(allocated_type)
          ? NewSizeofASTNodeWithType(allocated_type, location)
          : NewSizeofASTNodeWithKnownSize(allocated_type->size, location);
  Symbol* array_count = NULL;
  if (array_size != NULL) {
    array_count = SyntaxNewTemporary(syntax, NewSizeTypeRecord());
    allocation_size =
        NewBinaryASTNode(AST_OP(mult), NULL, location,
                         NewIdentifierASTNode(array_count, location),
                         allocation_size);
    if (!standard_placement) {
      allocation_size = NewBinaryASTNode(
          AST_OP(plus), NULL, location, allocation_size,
          NewSizeofASTNodeWithKnownSize(NewSizeTypeRecord()->size, location));
    }
  }
  VectorAppend(actuals, allocation_size);
  // If initialization throws, the storage is freed by the matching usual
  // deallocation function; placement forms (whose matching placement delete
  // would be needed) only destroy what was constructed.
  bool eh_cleanup = CompilerIsCXX() && CompilerExceptionsEnabled();
  bool deallocate_on_throw = eh_cleanup && placement_actuals == NULL;
  if (placement_actuals != NULL) {
    for (size_t i = 0; i < placement_actuals->length; i++) {
      VectorAppend(actuals, placement_actuals->value.p[i]);
    }
    VectorDelete(placement_actuals);
  }
  ASTNode* allocation =
      NewCallASTNode(GetCXXOperatorNewForType(allocated_type,
                                              array_size != NULL,
                                              actuals->length,
                                              location, global_scope,
                                              standard_placement),
                     location, actuals);
  TypeRecord* result_type = NewPointerTo(kQualPlain, allocated_type);
  ASTNode* result = NewCastASTNode(result_type, location, allocation);
  ((CastASTNode*)result)->kind = kCastStatic;
  ((CastASTNode*)result)->global_scope_new = global_scope;
  result->flags |= kASTCXXNewExpression;
  if (standard_placement) {
    result->flags |= kASTCXXPlacementNew;
  }
  if (array_size != NULL) {
    result->flags |= kASTCXXArrayNew;
  }
  if (allocated_type_dependent) {
    result->flags |= kASTDependentNewAllocation;
  }
  if (array_size != NULL) {
    result = BuildCXXArrayNew(
        syntax, location, allocated_type, array_size, array_count,
        array_initializers, result, result_type, standard_placement,
        value_init, allocated_type_dependent, eh_cleanup, deallocate_on_throw,
        global_scope);
  } else if (ctor_actuals != NULL || scalar_initializer != NULL ||
             aggregate_initializer != NULL) {
    result = BuildCXXObjectNew(
        syntax, location, allocated_type, result, result_type, ctor_actuals,
        scalar_initializer, aggregate_initializer, value_init,
        standard_placement, deallocate_on_throw, global_scope,
        allocated_type_dependent);
  }
  if (sym != NULL) {
    SymbolDelete(sym);
  }
  return result;
}


// `T* const` is a const pointer.  The delete temporary is assigned, so it
// cannot keep that top-level const.
static TypeRecord* CXXMutablePointerType(TypeRecord* pointer_type) {
  if (pointer_type == NULL || (GetQualifiers(pointer_type) & kQualConst) == 0) {
    return pointer_type;
  }
  TypeRecord* copy = TypeRecordCopy(pointer_type);
  SetQualifiers(copy, GetQualifiers(copy) & ~kQualConst);
  return copy;
}


// `delete` of `const T*` is valid.  The deallocation function takes `void*`,
// and a const object pointer does not convert to that implicitly.
static ASTNode* NewDeallocationPointer(ASTNode* expr, SourceLocation location) {
  TypeRecord* void_ptr =
      NewPointerTo(kQualPlain, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  ASTNode* result = NewCastASTNode(void_ptr, location, expr);
  ((CastASTNode*)result)->kind = kCastCStyle;
  return result;
}


// Builds the exception-only cleanup that frees a scalar allocation when
// initialization throws.
ASTNode* NewCXXNewDeallocationCleanup(Symbol* temp, TypeRecord* allocated_type,
                                      bool global_scope,
                                      SourceLocation location) {
  Vector* actuals = NewVector();
  VectorAppend(actuals,
               NewDeallocationPointer(NewIdentifierASTNode(temp, location),
                                      location));
  ASTNode* deallocate = NewExpressionStatement(
      NewCallASTNode(GetCXXOperatorDeleteForType(allocated_type, false,
                                                 location, global_scope),
                     location, actuals),
      location);
  deallocate->flags |= kASTEHCleanupOnly;
  return deallocate;
}


// `delete` / `delete[]` of a pointer expression. A class type runs its
// destructor first; an array delete reads the element-count cookie, destroys
// each element, then calls the array deallocation function.
ASTNode* NewCXXDeleteExpressionForPointer(Syntax* syntax, ASTNode* expr,
                                          bool is_array_delete,
                                          SourceLocation location,
                                          bool global_scope) {
  // An operand such as `s.d` is not typed until analyzed, and its type
  // decides the array header and the destructor.
  if (expr != NULL && expr->type == NULL &&
      !syntax->parsing_template_declaration) {
    expr = AnalyzeExpression(expr);
  }
  TypeRecord* pointer_type = expr->type;
  if ((pointer_type != NULL && TypeContainsTemplateParameter(pointer_type)) ||
      (syntax->parsing_template_declaration &&
       (pointer_type == NULL || !TypeIsPointerOrArray(pointer_type)))) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, NewDeallocationPointer(expr, location));
    ASTNode* node = NewCallASTNode(
        is_array_delete ? GetImplicitCXXOperatorDeleteArray(location)
                        : GetImplicitCXXOperatorDelete(location),
        location, actuals);
    node->flags |= kASTDependentDelete;
    if (is_array_delete) {
      node->flags |= kASTDependentArrayDelete;
    }
    return node;
  }
  TypeRecord* deleted_type =
      TypeIsPointerOrArray(pointer_type) ? pointer_type->next : NULL;
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      TypeIsStructOrUnion(deleted_type) &&
      !TypeIsCompleteClass(deleted_type)) {
    SyntaxError(syntax,
                "deleting a pointer to an incomplete class type is not "
                "allowed in C++26");
  }
  if (is_array_delete) {
    if (pointer_type == NULL || !TypeIsPointerOrArray(pointer_type)) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, expr);
    return NewCallASTNode(GetImplicitCXXOperatorDeleteArray(location),
                            location, actuals);
    }
    TypeRecord* size_type = NewSizeTypeRecord();
    TypeRecord* size_ptr_type = NewPointerTo(kQualPlain, size_type);
    Symbol* object_ptr =
        SyntaxNewTemporary(syntax, CXXMutablePointerType(pointer_type));
    Symbol* header = SyntaxNewTemporary(syntax, size_ptr_type);
    Symbol* count = SyntaxNewTemporary(syntax, size_type);
    Vector* statements = NewVector();
    Vector* guarded_statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(object_ptr, location),
                               expr, object_ptr->type, location),
                     location));
    VectorAppend(guarded_statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(header, location),
                               NewPtrSub(NewTypedCast(size_ptr_type,
                                                      NewIdentifierASTNode(
                                                          object_ptr,
                                                          location),
                                                      location),
                                         NewIntLiteral(1, location), location),
                               header->type, location),
                     location));
    VectorAppend(guarded_statements,
                 NewExpressionStatement(
                     NewAssign(NewIdentifierASTNode(count, location),
                               NewUnaryASTNode(AST_OP(contents), size_type,
                                                location,
                                                NewIdentifierASTNode(header,
                                                                     location)),
                               count->type, location),
                     location));
    int64_t base_per_element = 1;
    TypeRecord* base_type =
        CXXNewArrayBaseType(pointer_type->next, &base_per_element);
    if (TypeIsStructOrUnionPointer(pointer_type) &&
        FindCXXDestructorForType(pointer_type->next) != NULL) {
      VectorAppend(guarded_statements,
                   NewArrayDestructionLoop(syntax, pointer_type->next,
                                           object_ptr, count, location));
    } else if (TypeIsArray(pointer_type->next) &&
               FindCXXDestructorForType(base_type) != NULL) {
      // `delete[]` of `new T[n][2]` destroys all 2n T objects.
      Symbol* base_ptr =
          SyntaxNewTemporary(syntax, NewPointerTo(kQualPlain, base_type));
      Symbol* base_count = SyntaxNewTemporary(syntax, NewSizeTypeRecord());
      VectorAppend(guarded_statements,
                   NewExpressionStatement(
                       NewAssign(NewIdentifierASTNode(base_ptr, location),
                                 NewTypedCast(base_ptr->type,
                                              NewIdentifierASTNode(object_ptr,
                                                                   location),
                                              location),
                                 base_ptr->type, location),
                       location));
      VectorAppend(guarded_statements,
                   NewExpressionStatement(
                       NewAssign(NewIdentifierASTNode(base_count, location),
                                 NewBinaryASTNode(
                                     AST_OP(mult), NULL, location,
                                     NewIdentifierASTNode(count, location),
                                     NewIntLiteral(base_per_element,
                                                   location)),
                                 base_count->type, location),
                       location));
      VectorAppend(guarded_statements,
                   NewArrayDestructionLoop(syntax, base_type, base_ptr,
                                           base_count, location));
    }
    Vector* actuals = NewVector();
    VectorAppend(actuals, NewIdentifierASTNode(header, location));
    VectorAppend(guarded_statements,
                 NewExpressionStatement(
                     NewCallASTNode(GetCXXOperatorDeleteForType(
                                        pointer_type->next, true, location,
                                        global_scope),
                                    location, actuals),
                     location));
    ASTNode* nonnull = NewBinaryASTNode(
        AST_OP(noteq), NULL, location,
        NewIdentifierASTNode(object_ptr, location),
        NewIntLiteral(0, location));
    VectorAppend(statements,
                 NewIfStatementASTNode(
                     nonnull,
                     NewCompoundStatementASTNode(guarded_statements, location),
                     NULL, false, location));
    return NewUnaryASTNode(
        AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        location, NewCompoundStatementASTNode(statements, location));
  }
  if (pointer_type == NULL || !TypeIsStructOrUnionPointer(pointer_type) ||
      FindCXXDestructorForType(pointer_type->next) == NULL) {
    Vector* actuals = NewVector();
    VectorAppend(actuals, NewDeallocationPointer(expr, location));
    TypeRecord* object_type =
        TypeIsPointerOrArray(pointer_type) ? pointer_type->next : NULL;
    return NewCallASTNode(GetCXXOperatorDeleteForType(object_type, false,
                                                     location, global_scope),
                          location, actuals);
  }

  TypeRecord* mutable_pointer = CXXMutablePointerType(pointer_type);
  Symbol* temp = SyntaxNewTemporary(syntax, mutable_pointer);
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), mutable_pointer, location,
                       NewIdentifierASTNode(temp, location), expr);
  ASTNode* destructor =
      NewCXXDestructorCallForPointer(pointer_type->next, temp, location);
  Vector* actuals = NewVector();
  VectorAppend(actuals, NewDeallocationPointer(NewIdentifierASTNode(temp, location),
                                               location));
  ASTNode* deallocate =
      NewCallASTNode(GetCXXOperatorDeleteForType(pointer_type->next, false,
                                                 location, global_scope),
                     location, actuals);
  Vector* guarded_statements = NewVector();
  VectorAppend(guarded_statements,
               NewExpressionStatement(destructor, location));
  VectorAppend(guarded_statements,
               NewExpressionStatement(deallocate, location));
  ASTNode* nonnull = NewBinaryASTNode(
      AST_OP(noteq), NULL, location, NewIdentifierASTNode(temp, location),
      NewIntLiteral(0, location));
  ASTNode* guarded_delete = NewIfStatementASTNode(
      nonnull, NewCompoundStatementASTNode(guarded_statements, location), NULL,
      false, location);
  Vector* statements = NewVector();
  VectorAppend(statements, NewExpressionStatement(assign, location));
  VectorAppend(statements, guarded_delete);
  return NewUnaryASTNode(
      AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
      location, NewCompoundStatementASTNode(statements, location));
}


// Parses delete or delete[], parses its operand, and delegates destructor and
// deallocation lowering.
ASTNode* ParseCXXDeleteExpression(Syntax* syntax, TokenClass followers,
                                         bool global_scope) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // delete
  bool is_array_delete = false;
  if (LexMatch(syntax->lex, TOK(lsquare))) {
    SyntaxNeedBracket(syntax, TOK(rsquare), followers);
    is_array_delete = true;
  }
  ASTNode* expr = ParseCastExpression(syntax, followers);
  return NewCXXDeleteExpressionForPointer(syntax, expr, is_array_delete,
                                          location, global_scope);
}
