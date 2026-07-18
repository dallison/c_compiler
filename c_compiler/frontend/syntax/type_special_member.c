//
//  type_special_member.c
//  c_compiler
//
#include "type_class_internal.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "compiler.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"

static Symbol* CXXSourceObjectParameter(TypeRecord* func);
static bool CXXFunctionNeedsMemberwiseCopy(TypeRecord* func);
static ASTNode* NewCXXMemberReceiver(TypeRecord* func, StructMember* member,
                                     SourceLocation location);
static ASTNode* NewCXXSourceMemberReceiver(Symbol* source, StructMember* member,
                                           SourceLocation location);

bool CXXClassNameMatchesUnqualifiedTemplateName(String* class_name,
                                                String* spelling) {
  if (StringEqualString(spelling, class_name)) {
    return true;
  }
  const char* template_args = strchr(class_name->value, '<');
  if (template_args == NULL) {
    return false;
  }
  size_t base_length = (size_t)(template_args - class_name->value);
  return spelling->length == base_length &&
         strncmp(spelling->value, class_name->value, base_length) == 0;
}

static bool QualifiedNameIsSpecialMember(Symbol* owner,
                                         FullyQualifiedIdentifier* name,
                                         bool* is_destructor,
                                         String* special_member_name) {
  const char* member_name = FullyQualifiedIdentifierLast(name);
  *is_destructor = member_name[0] == '~';
  const char* source_class_name =
      *is_destructor ? member_name + 1 : member_name;
  String source;
  StringInit(&source, source_class_name);
  bool matches =
      CXXClassNameMatchesUnqualifiedTemplateName(&owner->name, &source);
  StringDestruct(&source);
  if (!matches) {
    return false;
  }
  StringSet(special_member_name, *is_destructor ? "~" : "");
  StringAppendString(special_member_name, &owner->name);
  return true;
}

Symbol* NewCXXConversionOperatorSymbol(TypeParser* parser,
                                              Struct* owner,
                                              TypeRecord* return_type,
                                              SourceLocation location,
                                              bool is_virtual) {
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = parser->is_constexpr;
  func->info.function.is_consteval = parser->is_consteval;
  func->info.function.is_virtual = is_virtual;
  TypeRecordChain(func, return_type);
  while (LexLookingAt(parser->lex, TOK(const)) ||
         LexLookingAt(parser->lex, TOK(volatile))) {
    if (LexMatch(parser->lex, TOK(const))) {
      func->info.function.is_const_member = true;
    } else if (LexMatch(parser->lex, TOK(volatile))) {
      func->info.function.is_volatile_member = true;
    }
  }
  func->info.function.ref_qualifier = ParseCXXRefQualifier(parser);
  ParseCXXExceptionSpecifier(parser, func);
  TypeRecordAddCXXThisParameter(func, owner, location);

  String member_name;
  ConversionOperatorName(return_type, &member_name);
  Symbol* member_symbol = NewSymbol(member_name.value, func, STO(implicit));
  member_symbol->location = location;
  func->info.function.symbol = member_symbol;
  StringDestruct(&member_name);
  return member_symbol;
}

static void AppendConversionOwnerComponent(FullyQualifiedIdentifier* name,
                                           String* component) {
  if (name->spelling.length != 0 || name->absolute) {
    StringAppend(&name->spelling, "::");
  }
  StringAppendString(&name->spelling, component);
  VectorAppend(&name->components, NewString(component->value));
  VectorAppend(&name->template_arguments, NULL);
}

static bool ParseCXXConversionOperatorOwner(TypeParser* parser,
                                            FullyQualifiedIdentifier* owner_name) {
  if (LexMatch(parser->lex, TOK(coloncolon))) {
    owner_name->absolute = true;
    owner_name->is_qualified = true;
  }
  if (!LexLookingAt(parser->lex, TOK(identifier))) {
    return false;
  }

  while (!LexEof(parser->lex)) {
    String component;
    StringInit(&component, parser->lex->spelling.value);
    LexNextToken(parser->lex);
    AppendConversionOwnerComponent(owner_name, &component);
    StringDestruct(&component);

    if (!LexMatch(parser->lex, TOK(coloncolon))) {
      return false;
    }
    owner_name->is_qualified = true;
    if (LexLookingAt(parser->lex, TOK(operator))) {
      return true;
    }
    if (!LexLookingAt(parser->lex, TOK(identifier))) {
      return false;
    }
  }
  return false;
}

static Symbol* TryParseCXXQualifiedConversionOperatorDeclarator(
    TypeParser* parser) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);

  SourceLocation location = parser->lex->current_token_location;
  FullyQualifiedIdentifier owner_name;
  FullyQualifiedIdentifierInit(&owner_name);
  if (!ParseCXXConversionOperatorOwner(parser, &owner_name) ||
      !LexMatch(parser->lex, TOK(operator))) {
    FullyQualifiedIdentifierDestruct(&owner_name);
    LexCheckpointRestore(parser->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  if (!owner_name.absolute && owner_name.components.length == 1) {
    owner_name.is_qualified = false;
  }
  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      parser->syntax, &owner_name, owner_name.components.length);
  if (owner == NULL) {
    owner = SyntaxFindQualifiedTag(parser->syntax, &owner_name);
  }
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    SyntaxError(parser->syntax,
                "Qualified conversion operator does not name a class member");
    FullyQualifiedIdentifierDestruct(&owner_name);
    return NULL;
  }

  parser->cxx_member_owner = owner->type->info.struct_info;
  TypeRecord* return_type = ParseCXXConversionType(parser);
  String member_name;
  ConversionOperatorName(return_type, &member_name);
  parser->cxx_member_definition =
      FindStructMember(parser->cxx_member_owner, &member_name);
  if (parser->cxx_member_definition == NULL) {
    SyntaxError(parser->syntax, "No class member named %s",
                member_name.value);
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&owner_name);
    return NULL;
  }

  if (!LexMatch(parser->lex, TOK(lparen))) {
    SyntaxError(parser->syntax,
                "Expected '(' in conversion operator definition");
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&owner_name);
    return NULL;
  }
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));

  Symbol* sym =
      NewCXXConversionOperatorSymbol(parser, parser->cxx_member_owner,
                                     return_type, location,
                                     /*is_virtual=*/false);
  SymbolSetCXXMangledAsmName(sym);
  StringDestruct(&member_name);
  FullyQualifiedIdentifierDestruct(&owner_name);
  return sym;
}

Symbol* TypeParserParseCXXSpecialMemberDeclarator(TypeParser* parser) {
  Symbol* conversion = TryParseCXXQualifiedConversionOperatorDeclarator(parser);
  if (conversion != NULL) {
    return conversion;
  }

  SourceLocation location = parser->lex->current_token_location;
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(parser->syntax, &name,
                                                         TC(decl))) {
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }
  if (!name.is_qualified || name.components.length < 2) {
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      parser->syntax, &name, name.components.length - 1);
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    SyntaxError(parser->syntax, "Qualified declarator %s does not name a class member",
                name.spelling.value);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  bool is_destructor = false;
  String member_name;
  StringInit(&member_name, NULL);
  if (!QualifiedNameIsSpecialMember(owner, &name, &is_destructor,
                                    &member_name)) {
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  parser->cxx_member_owner = owner->type->info.struct_info;
  parser->cxx_member_definition =
      FindStructMember(parser->cxx_member_owner, &member_name);
  if (parser->cxx_member_definition == NULL) {
    SyntaxError(parser->syntax, "No class member named %s", name.spelling.value);
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  if (!LexMatch(parser->lex, TOK(lparen))) {
    SyntaxError(parser->syntax, "Expected '(' in special member definition");
    StringDestruct(&member_name);
    FullyQualifiedIdentifierDestruct(&name);
    return NULL;
  }

  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  proto_parser.cxx_member_owner = parser->cxx_member_owner;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = parser->is_constexpr;
  func->info.function.is_consteval = parser->is_consteval;
  func->info.function.is_constructor = !is_destructor;
  func->info.function.is_destructor = is_destructor;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  if (LexMatch(parser->lex, TOK(const))) {
    SyntaxError(parser->syntax, "Constructors and destructors cannot be const");
  }
  ParseCXXExceptionSpecifier(parser, func);
  ParseCXXPureSpecifier(parser, func);
  TypeParserDestruct(&proto_parser);
  TypeRecordAddCXXThisParameter(func, parser->cxx_member_owner, location);

  Symbol* sym = NewSymbol(member_name.value, func, STO(implicit));
  sym->location = location;
  func->info.function.symbol = sym;
  CXXFinalizeSpecialMemberMetadata(sym, parser->cxx_member_owner, true);
  SymbolSetCXXMangledAsmName(sym);
  StringDestruct(&member_name);
  FullyQualifiedIdentifierDestruct(&name);
  return sym;
}

bool SymbolIsCXXAllocationFunction(Symbol* symbol) {
  if (!CompilerIsCXX() || symbol == NULL) {
    return false;
  }
  return strcmp(symbol->name.value, "operator new") == 0 ||
         strcmp(symbol->name.value, "operator new[]") == 0 ||
         strcmp(symbol->name.value, "operator delete") == 0 ||
         strcmp(symbol->name.value, "operator delete[]") == 0;
}

static bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type) {
  return TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
         StructHasVirtualBases(type->info.struct_info);
}

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

static StructMember* FindCXXDestructorForObjectType(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  StringDestruct(&destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      destructor->symbol == NULL || destructor->symbol->type == NULL ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}

static TypeRecord* CXXDestructibleElementType(TypeRecord* type) {
  if (TypeIsFixedArray(type) && type->next != NULL &&
      FindCXXDestructorForObjectType(type->next) != NULL) {
    return type->next;
  }
  if (FindCXXDestructorForObjectType(type) != NULL) {
    return type;
  }
  return NULL;
}

static ASTNode* NewCXXMemberReceiver(TypeRecord* func, StructMember* member,
                                     SourceLocation location) {
  if (func == NULL || func->info.function.prototype.length == 0 ||
      member == NULL || member->symbol == NULL) {
    return NULL;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               location);
  return NewBinaryASTNode(AST_OP(arrow), NULL, location, this_node,
                          member_name);
}

static ASTNode* NewCXXMemberDestructorCall(TypeRecord* func,
                                           TypeRecord* object_type,
                                           ASTNode* receiver,
                                           SourceLocation location) {
  if (func == NULL || object_type == NULL ||
      object_type->info.struct_info == NULL ||
      object_type->info.struct_info->tag_name == NULL) {
    ASTNodeDelete(receiver);
    return NULL;
  }
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(object_type, actuals, location);
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, object_type->info.struct_info->tag_name);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL, location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, destructor);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

// Appends the destructor call(s) for a single data member to `body`.  A fixed
// array member yields one call per element in reverse index order (matching the
// reverse-of-construction destruction order); a scalar/class member yields one
// call.  Members without a non-trivial destructor append nothing.  Unlike
// AppendCXXMemberDestructorCalls this does not require `func` to be a destructor,
// so it can also synthesize a constructor's partial-construction cleanup (the
// caller decides ordering and marks the statements kASTEHCleanupOnly).
void AppendCXXSingleMemberDestructorCalls(TypeRecord* func,
                                          StructMember* member, Vector* body,
                                          SourceLocation location) {
  if (member == NULL || member->symbol == NULL || member->is_static ||
      member->is_member_function || StructMemberIsNestedType(member)) {
    return;
  }
  TypeRecord* member_type = member->symbol->type;
  TypeRecord* object_type = CXXDestructibleElementType(member_type);
  if (object_type == NULL) {
    return;
  }
  if (TypeIsFixedArray(member_type)) {
    for (size_t index = member_type->info.array.size.fixed; index > 0;
         index--) {
      ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
      ASTNode* index_node = NewIntConstantASTNode(
          (int64_t)index - 1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
          location);
      receiver = NewBinaryASTNode(AST_OP(subscript), NULL, location, receiver,
                                  index_node);
      ASTNode* call =
          NewCXXMemberDestructorCall(func, object_type, receiver, location);
      if (call != NULL) {
        VectorAppend(body, call);
      }
    }
  } else {
    ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
    ASTNode* call =
        NewCXXMemberDestructorCall(func, object_type, receiver, location);
    if (call != NULL) {
      VectorAppend(body, call);
    }
  }
}

void AppendCXXMemberDestructorCalls(TypeRecord* func, Vector* body,
                                          SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_destructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = owner->members.length; i > 0; i--) {
    StructMember* member = owner->members.value.p[i - 1];
    AppendCXXSingleMemberDestructorCalls(func, member, body, location);
  }
}

// Applies `depth` constant subscripts (`indices[0..depth)`) to a freshly-built
// member receiver so that a multidimensional array member is addressed down to
// one of its innermost elements (e.g. `dst.g[i][j]`).
static ASTNode* ApplyConstantSubscripts(ASTNode* base, const size_t* indices,
                                        size_t depth, SourceLocation location) {
  ASTNode* node = base;
  for (size_t d = 0; d < depth; d++) {
    ASTNode* index = NewIntConstantASTNode(
        (int64_t)indices[d], NewTypeRecordWithSize(kTypeInt, kQualPlain),
        location);
    node = NewBinaryASTNode(AST_OP(subscript), NULL, location, node, index);
  }
  return node;
}

// Emits element-wise `dst.member[..] = src.member[..]` assignments for an array
// data member, recursing through every dimension of a multidimensional array so
// the assignment finally acts on the array's innermost (scalar or class)
// element.  Assigning a whole sub-array row (e.g. `int[2]` from an `int[2][2]`)
// is ill-formed, so a naive single-dimension loop breaks for such members.
static void AppendCXXArrayMemberwiseAssignments(
    TypeRecord* func, Symbol* source, StructMember* member,
    TypeRecord* array_type, size_t* indices, size_t depth,
    bool is_constructor_initializer, Vector* body, SourceLocation location) {
  TypeRecord* element_type = array_type->next;
  for (size_t index = 0; index < (size_t)array_type->info.array.size.fixed;
       index++) {
    indices[depth] = index;
    if (TypeIsFixedArray(element_type)) {
      AppendCXXArrayMemberwiseAssignments(func, source, member, element_type,
                                          indices, depth + 1,
                                          is_constructor_initializer, body,
                                          location);
      continue;
    }
    ASTNode* target =
        ApplyConstantSubscripts(NewCXXMemberReceiver(func, member, location),
                                indices, depth + 1, location);
    ASTNode* value = ApplyConstantSubscripts(
        NewCXXSourceMemberReceiver(source, member, location), indices,
        depth + 1, location);
    ASTNode* assign = NewBinaryASTNode(AST_OP(assign), element_type, location,
                                       target, value);
    if (is_constructor_initializer) {
      assign->flags |= kASTCXXMemberInitializer;
    }
    VectorAppend(body, NewExpressionStatementASTNode(assign, location));
  }
}

void AppendCXXMemberwiseAssignments(TypeParser* parser, TypeRecord* func,
                                    Vector* body, Struct* owner,
                                    SourceLocation location) {
  if (parser == NULL || func == NULL || body == NULL || owner == NULL) {
    return;
  }
  Symbol* source = CXXSourceObjectParameter(func);
  if (source == NULL) {
    return;
  }
  bool is_constructor_initializer =
      func->info.function.cxx_special_member_kind ==
          kCXXSpecialMemberCopyConstructor ||
      func->info.function.cxx_special_member_kind ==
          kCXXSpecialMemberMoveConstructor;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    if (TypeIsFixedArray(member_type)) {
      size_t ndims = 0;
      for (TypeRecord* t = member_type; TypeIsFixedArray(t); t = t->next) {
        ndims++;
      }
      size_t* indices = calloc(ndims, sizeof(size_t));
      AppendCXXArrayMemberwiseAssignments(func, source, member, member_type,
                                          indices, 0, is_constructor_initializer,
                                          body, location);
      free(indices);
    } else {
      ASTNode* target = NewCXXMemberReceiver(func, member, location);
      ASTNode* value = NewCXXSourceMemberReceiver(source, member, location);
      if (is_constructor_initializer && TypeIsStructOrUnion(member_type)) {
        Vector* actuals = NewVector();
        VectorAppend(actuals, value);
        ASTNode* init = SyntaxNewCXXMemberInitializerStatement(
            parser->syntax, func, member, actuals, location);
        if (init != NULL) {
          VectorAppend(body, init);
          continue;
        }
      }
      ASTNode* assign = NewBinaryASTNode(AST_OP(assign), member_type, location,
                                         target, value);
      if (is_constructor_initializer) {
        assign->flags |= kASTCXXMemberInitializer;
      }
      VectorAppend(body, NewExpressionStatementASTNode(assign, location));
    }
  }
}

static void AppendCXXAssignmentReturnThis(TypeRecord* func, Vector* body,
                                          SourceLocation location);
static bool CXXFunctionIsAssignmentOperator(TypeRecord* func);
static ASTNode* NewCXXSourceMemberReceiver(Symbol* source, StructMember* member,
                                           SourceLocation location);

static bool CXXReferenceTargetsStruct(TypeRecord* ref, Struct* owner) {
  return TypeIsReference(ref) && ref->next != NULL &&
         TypeIsStructOrUnion(ref->next) &&
         ref->next->info.struct_info == owner;
}

static bool CXXFunctionHasOnlyImplicitObjectParameters(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return false;
  }
  size_t implicit_count = 0;
  if (func->info.function.cxx_member_owner != NULL) {
    implicit_count++;
  }
  if ((func->info.function.is_constructor ||
       func->info.function.is_destructor) &&
      func->info.function.cxx_member_owner != NULL &&
      StructHasVirtualBases(func->info.function.cxx_member_owner)) {
    implicit_count++;
  }
  return func->info.function.prototype.length == implicit_count;
}

static CXXSpecialMemberKind CXXDetermineSpecialMemberKind(Symbol* symbol,
                                                         Struct* owner) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      owner == NULL) {
    return kCXXSpecialMemberNone;
  }
  TypeRecord* func = symbol->type;
  if (func->info.function.is_destructor) {
    return kCXXSpecialMemberDestructor;
  }
  if ((symbol->flags.is_template ||
       func->info.function.template_parameter_count > 0) &&
      func->info.function.is_constructor) {
    return kCXXSpecialMemberNone;
  }
  if (func->info.function.is_constructor) {
    if (CXXFunctionHasOnlyImplicitObjectParameters(func)) {
      return kCXXSpecialMemberDefaultConstructor;
    }
    Symbol* source = CXXSourceObjectParameter(func);
    if (source != NULL && CXXReferenceTargetsStruct(source->type, owner)) {
      return source->type->declarator == kDeclRValueReference
                 ? kCXXSpecialMemberMoveConstructor
                 : kCXXSpecialMemberCopyConstructor;
    }
    return kCXXSpecialMemberNone;
  }
  if (strcmp(symbol->name.value, "operator=") != 0 ||
      func->info.function.prototype.length < 2) {
    return kCXXSpecialMemberNone;
  }
  Symbol* source = CXXSourceObjectParameter(func);
  if (source == NULL || !CXXReferenceTargetsStruct(source->type, owner)) {
    return kCXXSpecialMemberNone;
  }
  return source->type->declarator == kDeclRValueReference
             ? kCXXSpecialMemberMoveAssignment
             : kCXXSpecialMemberCopyAssignment;
}

void CXXFinalizeSpecialMemberMetadata(Symbol* symbol, Struct* owner,
                                             bool user_declared) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return;
  }
  TypeRecord* func = symbol->type;
  func->info.function.cxx_special_member_kind =
      CXXDetermineSpecialMemberKind(symbol, owner);
  if (user_declared) {
    func->info.function.is_user_declared = true;
  }
  if (func->info.function.is_defaulted) {
    func->info.function.is_explicitly_defaulted =
        !func->info.function.is_implicitly_declared;
    func->info.function.is_constexpr_eligible = true;
  }
  if (func->info.function.is_deleted) {
    func->info.function.is_explicitly_deleted =
        !func->info.function.is_implicitly_declared;
    func->info.function.is_implicitly_deleted =
        func->info.function.is_implicitly_declared;
  }
}

static bool CXXFunctionIsThreeWayComparison(TypeRecord* func) {
  return func != NULL && TypeIsFunction(func) &&
         func->info.function.symbol != NULL &&
         strcmp(func->info.function.symbol->name.value, "operator<=>") == 0;
}

static bool CXXFunctionIsEqualityComparison(TypeRecord* func) {
  return func != NULL && TypeIsFunction(func) &&
         func->info.function.symbol != NULL &&
         strcmp(func->info.function.symbol->name.value, "operator==") == 0;
}

static void AppendCXXEqualityComparison(Symbol* member_symbol, Vector* body,
                                        SourceLocation location) {
  TypeRecord* func = member_symbol->type;
  Struct* owner = func->info.function.cxx_member_owner;
  Symbol* source = CXXSourceObjectParameter(func);
  if (owner == NULL || source == NULL) {
    return;
  }
  ASTNode* result = NULL;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    ASTNode* lhs = NewCXXMemberReceiver(func, member, location);
    ASTNode* rhs = NewCXXSourceMemberReceiver(source, member, location);
    ASTNode* eq = NewBinaryASTNode(AST_OP(equal), NULL, location, lhs, rhs);
    result = result == NULL
                 ? eq
                 : NewBinaryASTNode(AST_OP(logand), NULL, location, result, eq);
  }
  if (result == NULL) {
    result = NewIntConstantASTNode(
        1, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  VectorAppend(body,
               NewCombinedStatementASTNode(AST_OP(return), result, NULL,
                                           location));
}

static Symbol* CXXComparisonCategoryConstant(TypeRecord* category,
                                             const char* name) {
  if (category == NULL || category->info.struct_info == NULL) {
    return NULL;
  }
  Struct* str = category->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* m = str->members.value.p[i];
    if (m != NULL && m->symbol != NULL && m->is_static &&
        strcmp(m->symbol->name.value, name) == 0) {
      return m->symbol;
    }
  }
  return NULL;
}

static ASTNode* NewCXXReturnCategoryConstant(TypeRecord* category,
                                             const char* name,
                                             SourceLocation location) {
  Symbol* constant = CXXComparisonCategoryConstant(category, name);
  if (constant == NULL) {
    return NULL;
  }
  ASTNode* value = NewIdentifierASTNode(constant, location);
  return NewCombinedStatementASTNode(AST_OP(return), value, NULL, location);
}

static bool CXXCategoryIsPartial(TypeRecord* category) {
  return CXXComparisonCategoryConstant(category, "unordered") != NULL;
}

static TypeRecord* CXXDeduceComparisonCategory(Struct* owner) {
  bool has_floating = false;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* m = owner->members.value.p[i];
    if (m == NULL || m->symbol == NULL || m->is_static ||
        m->is_member_function) {
      continue;
    }
    if (TypeIsFloatingPoint(m->symbol->type)) {
      has_floating = true;
    }
  }
  return TypeFindCXXComparisonCategory(has_floating ? "partial_ordering"
                                                    : "strong_ordering");
}

static void AppendCXXThreeWayComparisons(TypeParser* parser,
                                         Symbol* member_symbol, Vector* body,
                                         SourceLocation location) {
  TypeRecord* func = member_symbol->type;
  Struct* owner = func->info.function.cxx_member_owner;
  Symbol* source = CXXSourceObjectParameter(func);
  if (owner == NULL || source == NULL) {
    return;
  }
  TypeRecord* category = func->next;
  if (category == NULL || (category->type & kTypeAuto) != 0) {
    TypeRecord* deduced = CXXDeduceComparisonCategory(owner);
    if (deduced == NULL) {
      SyntaxError(parser->syntax,
                  "Defaulted 'operator<=>' requires <compare> to be included");
      return;
    }
    category = TypeRecordCalculateSize(TypeRecordCopy(deduced));
    func->next = category;
  }
  bool is_partial = CXXCategoryIsPartial(category);

  struct {
    ASTOpcode op;
    const char* constant;
  } arms[3] = {
      {AST_OP(less), "less"},
      {AST_OP(greater), "greater"},
      {AST_OP(noteq), "unordered"},
  };
  int arm_count = is_partial ? 3 : 2;

  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    for (int a = 0; a < arm_count; a++) {
      ASTNode* ret =
          NewCXXReturnCategoryConstant(category, arms[a].constant, location);
      if (ret == NULL) {
        continue;
      }
      ASTNode* lhs = NewCXXMemberReceiver(func, member, location);
      ASTNode* rhs = NewCXXSourceMemberReceiver(source, member, location);
      ASTNode* cmp =
          NewBinaryASTNode(AST_OP(spaceship), NULL, location, lhs, rhs);
      ASTNode* zero = NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
      ASTNode* cond = NewBinaryASTNode(arms[a].op, NULL, location, cmp, zero);
      VectorAppend(body,
                   NewIfStatementASTNode(cond, ret, NULL, false, location));
    }
  }
  ASTNode* equal = NewCXXReturnCategoryConstant(
      category, is_partial ? "equivalent" : "equal", location);
  if (equal == NULL) {
    equal = NewCXXReturnCategoryConstant(category, "equivalent", location);
  }
  if (equal != NULL) {
    VectorAppend(body, equal);
  }
}

void SynthesizeDefaultedMemberFunctionBody(TypeParser* parser,
                                                  Symbol* member_symbol) {
  if (parser == NULL || member_symbol == NULL || member_symbol->type == NULL ||
      !TypeIsFunction(member_symbol->type) ||
      member_symbol->type->info.function.is_deleted) {
    return;
  }
  member_symbol->flags.is_defined = true;
  member_symbol->flags.is_inline_defn = true;
  if (!StorageIs(member_symbol->storage, STO(static))) {
    member_symbol->flags.is_weak = true;
  }
  member_symbol->value.func_defn = member_symbol;
  member_symbol->type->info.function.is_inline = true;
  member_symbol->type->info.function.definition = true;

  if (CXXFunctionIsThreeWayComparison(member_symbol->type) ||
      CXXFunctionIsEqualityComparison(member_symbol->type)) {
    // A defaulted comparison operator is implicitly constexpr when it satisfies
    // the requirements for a constexpr function ([class.compare.default]).  Mark
    // it so its synthesized body can participate in constant evaluation; if a
    // member subobject's comparison turns out not to be constant, the evaluator
    // simply fails the fold.
    member_symbol->type->info.function.is_constexpr = true;
    Vector* comparison_body = NewVector();
    if (CXXFunctionIsThreeWayComparison(member_symbol->type)) {
      AppendCXXThreeWayComparisons(parser, member_symbol, comparison_body,
                                   member_symbol->location);
    } else {
      AppendCXXEqualityComparison(member_symbol, comparison_body,
                                  member_symbol->location);
    }
    member_symbol->type->info.function.body =
        NewCompoundStatementASTNode(comparison_body, member_symbol->location);
    VectorAppend(&compiler->declaration_asts,
                 member_symbol->type->info.function.body);
    if (!parser->syntax->parsing_template_declaration) {
      QueueInlineMemberFunctionDefinition(member_symbol);
    }
    return;
  }

  Vector* body = NewVector();
  CXXConstructorInitList cxx_initializers;
  SyntaxCXXConstructorInitListInit(&cxx_initializers);
  SyntaxInsertCXXConstructorPreamble(parser->syntax, member_symbol->type, body,
                                     &cxx_initializers,
                                     member_symbol->location);
  if (CXXFunctionNeedsMemberwiseCopy(member_symbol->type)) {
    if (CXXFunctionIsAssignmentOperator(member_symbol->type)) {
      // A defaulted copy/move constructor initializes its base subobjects in
      // the constructor preamble above; a defaulted assignment operator has no
      // preamble, so its base subobjects must be assigned explicitly here
      // before the class's own members.
      AppendCXXBaseAssignments(parser->syntax, member_symbol->type, body,
                               member_symbol->location);
    }
    AppendCXXMemberwiseAssignments(
        parser, member_symbol->type, body,
        member_symbol->type->info.function.cxx_member_owner,
        member_symbol->location);
    if (CXXFunctionIsAssignmentOperator(member_symbol->type)) {
      AppendCXXAssignmentReturnThis(member_symbol->type, body,
                                    member_symbol->location);
    }
  }
  AppendCXXMemberDestructorCalls(member_symbol->type, body,
                                 member_symbol->location);
  AppendCXXBaseDestructorCalls(parser->syntax, member_symbol->type, body,
                               member_symbol->location);
  SyntaxCXXConstructorInitListDestruct(&cxx_initializers);

  member_symbol->type->info.function.body =
      NewCompoundStatementASTNode(body, member_symbol->location);
  VectorAppend(&compiler->declaration_asts,
               member_symbol->type->info.function.body);
  if (!parser->syntax->parsing_template_declaration) {
    QueueInlineMemberFunctionDefinition(member_symbol);
  }
}

static Symbol* CXXSourceObjectParameter(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length < 2) {
    return NULL;
  }
  return func->info.function.prototype.value.p[func->info.function.prototype.length - 1];
}

static ASTNode* NewCXXSourceMemberReceiver(Symbol* source,
                                           StructMember* member,
                                           SourceLocation location) {
  if (source == NULL || member == NULL || member->symbol == NULL) {
    return NULL;
  }
  ASTNode* source_node = NewIdentifierASTNode(source, location);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               location);
  return NewBinaryASTNode(AST_OP(dot), NULL, location, source_node, member_name);
}

static bool CXXFunctionIsAssignmentOperator(TypeRecord* func) {
  return func != NULL && TypeIsFunction(func) && func->info.function.symbol != NULL &&
         strcmp(func->info.function.symbol->name.value, "operator=") == 0;
}

static bool CXXFunctionNeedsMemberwiseCopy(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL ||
      func->info.function.is_destructor) {
    return false;
  }
  switch (func->info.function.cxx_special_member_kind) {
    case kCXXSpecialMemberCopyConstructor:
    case kCXXSpecialMemberMoveConstructor:
    case kCXXSpecialMemberCopyAssignment:
    case kCXXSpecialMemberMoveAssignment:
      return true;
    default:
      return false;
  }
}

static void AppendCXXAssignmentReturnThis(TypeRecord* func, Vector* body,
                                          SourceLocation location) {
  if (!CXXFunctionIsAssignmentOperator(func) || func->next == NULL ||
      TypeIsVoid(func->next) || func->info.function.prototype.length == 0) {
    return;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  TypeRecord* object_type =
      TypeIsPointer(this_symbol->type) ? this_symbol->type->next : NULL;
  ASTNode* object = NewUnaryASTNode(AST_OP(contents), object_type, location,
                                    this_node);
  VectorAppend(body, NewCombinedStatementASTNode(AST_OP(return), object, NULL,
                                                 location));
}

static bool CXXStructHasUserDeclaredConstructor(Struct* str) {
  if (str == NULL || str->tag_name == NULL) {
    return false;
  }
  StructMember* first = FindStructMember(str, str->tag_name);
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    TypeRecord* func = member->symbol != NULL ? member->symbol->type : NULL;
    if (member->is_member_function && func != NULL &&
        func->info.function.is_constructor &&
        func->info.function.is_user_declared) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasVirtualMemberFunction(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    TypeRecord* func = member != NULL && member->symbol != NULL
                           ? member->symbol->type
                           : NULL;
    if (member != NULL && member->is_member_function &&
        !member->is_using_declaration && func != NULL &&
        func->info.function.is_virtual) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasNonPublicDataMember(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    if (member->access != kAccessPublic) {
      return true;
    }
  }
  return false;
}

void ComputeCXXAggregateStatus(Struct* str) {
  if (!CompilerIsCXX() || str == NULL) {
    return;
  }
  bool aggregate = !str->is_union &&
                   !CXXStructHasUserDeclaredConstructor(str) &&
                   !CXXStructHasVirtualMemberFunction(str) &&
                   // A class that inherits virtual functions still "has virtual
                   // functions" and so is not an aggregate; virtual_members
                   // already includes slots copied from polymorphic bases.
                   str->virtual_members.length == 0 &&
                   !CXXStructHasNonPublicDataMember(str);
  for (size_t i = 0; aggregate && i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->access != kAccessPublic) {
      aggregate = false;
    }
  }
  if (aggregate && str->virtual_bases.length > 0) {
    aggregate = false;
  }
  str->is_aggregate = aggregate;
}

static bool StructHasDeclaredCXXDestructor(Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->tag_name == NULL) {
    return false;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, str->tag_name);
  StructMember* destructor = FindStructMember(str, &destructor_name);
  StringDestruct(&destructor_name);
  return destructor != NULL && destructor->is_member_function &&
         destructor->symbol != NULL && destructor->symbol->type != NULL &&
         destructor->symbol->type->info.function.is_destructor;
}

static bool StructNeedsImplicitCXXDestructor(Struct* str) {
  if (!CompilerIsCXX() || str == NULL || str->is_union) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    if (CXXDestructibleElementType(member->symbol->type) != NULL) {
      return true;
    }
  }
  // A base (or virtual base) class with a non-trivial destructor also requires
  // this class to have a destructor, so the base subobject gets destroyed.
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base != NULL && base->type != NULL &&
        FindCXXDestructorForObjectType(base->type) != NULL) {
      return true;
    }
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (base != NULL && base->type != NULL &&
        FindCXXDestructorForObjectType(base->type) != NULL) {
      return true;
    }
  }
  return false;
}

void AddImplicitCXXDestructorIfNeeded(TypeParser* parser, Struct* str,
                                             Symbol* tag) {
  if (!CompilerIsCXX() || parser == NULL || str == NULL || tag == NULL ||
      str->tag_name == NULL || StructHasDeclaredCXXDestructor(str) ||
      !StructNeedsImplicitCXXDestructor(str)) {
    return;
  }
  // While parsing a template *declaration* the primary's implicit special
  // members are intentionally deferred to instantiation time (see the matching
  // guard in AddImplicitCXXSpecialMembers).  Injecting a destructor into the
  // primary here would be cloned into every specialization and then collide
  // with the destructor the instantiation-time AddImplicitCXXSpecialMembers
  // synthesizes, leaving the class with two destructors.
  if (parser->syntax != NULL &&
      parser->syntax->parsing_template_declaration) {
    return;
  }

  SourceLocation location = tag->location;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_destructor = true;
  func->info.function.is_constexpr = true;
  func->info.function.is_inline = true;
  func->info.function.definition = true;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
  TypeRecordAddCXXThisParameter(func, str, location);

  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, str->tag_name);
  Symbol* symbol = NewSymbol(destructor_name.value, func, STO(implicit));
  StringDestruct(&destructor_name);
  symbol->flags.invented = true;
  symbol->flags.is_defined = true;
  symbol->flags.is_inline_defn = true;
  if (!StorageIs(symbol->storage, STO(static))) {
    symbol->flags.is_weak = true;
  }
  symbol->location = location;
  symbol->value.func_defn = symbol;
  func->info.function.symbol = symbol;

  StructMember* member = NewStructMember(symbol);
  member->is_member_function = true;
  member->access = str->is_class ? kAccessPublic : kAccessPublic;
  AddStructMember(parser, str, member);

  Vector* body = NewVector();
  AppendCXXMemberDestructorCalls(func, body, location);
  AppendCXXBaseDestructorCalls(parser->syntax, func, body, location);
  func->info.function.body = NewCompoundStatementASTNode(body, location);
  VectorAppend(&compiler->declaration_asts, func->info.function.body);
  if (!parser->syntax->parsing_template_declaration) {
    QueueInlineMemberFunctionDefinition(symbol);
  }
}

static TypeRecord* NewCXXClassTypeForStruct(Struct* str, Qualifiers quals) {
  TypeRecord* type = NewTypeRecordWithSize(str->is_union ? kTypeUnion
                                                         : kTypeStruct,
                                           quals);
  type->info.struct_info = str;
  TypeRecordCalculateSize(type);
  return type;
}

static TypeRecord* NewCXXClassReferenceType(Struct* str, bool is_const,
                                            bool rvalue) {
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, NewCXXClassTypeForStruct(str,
                                                is_const ? kQualConst
                                                         : kQualPlain));
  TypeRecordCalculateSize(ref);
  return ref;
}

static Symbol* NewCXXSyntheticFormal(const char* name, TypeRecord* type,
                                     SourceLocation location) {
  Symbol* formal = NewSymbol(name, type, STO(implicit));
  formal->flags.is_argument = true;
  formal->flags.invented = true;
  formal->flags.is_defined = true;
  formal->location = location;
  return formal;
}

static void AppendCXXSyntheticFormal(TypeRecord* func, Symbol* formal) {
  formal->value.arg_number = (int32_t)func->info.function.prototype.length;
  VectorAppend(&func->info.function.prototype, formal);
}

static bool CXXStructHasAnyConstructor(Struct* str) {
  if (str == NULL || str->tag_name == NULL) {
    return false;
  }
  StructMember* first = FindStructMember(str, str->tag_name);
  for (StructMember* member = first; member != NULL;
       member = member->overload_next) {
    if (member->is_member_function && member->symbol != NULL &&
        member->symbol->type != NULL &&
        member->symbol->type->info.function.is_constructor) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasSpecialMemberKind(Struct* str,
                                          CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    for (StructMember* overload = member; overload != NULL;
         overload = overload->overload_next) {
      TypeRecord* func = overload != NULL && overload->symbol != NULL
                             ? overload->symbol->type
                             : NULL;
      if (overload != NULL && overload->is_member_function && func != NULL &&
          func->info.function.cxx_special_member_kind == kind) {
        return true;
      }
    }
  }
  return false;
}

static bool CXXStructHasUserDeclaredSpecialMemberKind(
    Struct* str, CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    for (StructMember* overload = member; overload != NULL;
         overload = overload->overload_next) {
      TypeRecord* func = overload != NULL && overload->symbol != NULL
                             ? overload->symbol->type
                             : NULL;
      if (overload != NULL && overload->is_member_function && func != NULL &&
          func->info.function.cxx_special_member_kind == kind &&
          func->info.function.is_user_declared) {
        return true;
      }
    }
  }
  return false;
}

static bool CXXStructHasUserDeclaredDestructor(Struct* str) {
  return CXXStructHasUserDeclaredSpecialMemberKind(
      str, kCXXSpecialMemberDestructor);
}

static bool CXXStructHasUserDeclaredCopyOrMove(Struct* str) {
  return CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberCopyConstructor) ||
         CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberMoveConstructor) ||
         CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberCopyAssignment) ||
         CXXStructHasUserDeclaredSpecialMemberKind(
             str, kCXXSpecialMemberMoveAssignment);
}

static bool CXXStructHasUnassignableMember(Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    TypeRecord* type = member->symbol->type;
    if (TypeIsReference(type) || TypeIsConst(type)) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasDeletedSpecialMemberKind(Struct* str,
                                                 CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    for (StructMember* overload = member; overload != NULL;
         overload = overload->overload_next) {
      TypeRecord* func = overload != NULL && overload->symbol != NULL
                             ? overload->symbol->type
                             : NULL;
      if (overload != NULL && overload->is_member_function && func != NULL &&
          func->info.function.cxx_special_member_kind == kind &&
          func->info.function.is_deleted) {
        return true;
      }
    }
  }
  return false;
}

static bool CXXStructHasDeletedBaseSpecialMemberKind(
    Struct* str, CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (CXXStructHasDeletedSpecialMemberKind(base_struct, kind) ||
        CXXStructHasDeletedBaseSpecialMemberKind(base_struct, kind)) {
      return true;
    }
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* base_struct = base->type->info.struct_info;
    if (CXXStructHasDeletedSpecialMemberKind(base_struct, kind) ||
        CXXStructHasDeletedBaseSpecialMemberKind(base_struct, kind)) {
      return true;
    }
  }
  return false;
}

static bool CXXStructHasDeletedMemberSpecialMemberKind(
    Struct* str, CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    TypeRecord* type = member->symbol->type;
    if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
      continue;
    }
    Struct* member_struct = type->info.struct_info;
    if (CXXStructHasDeletedSpecialMemberKind(member_struct, kind) ||
        CXXStructHasDeletedBaseSpecialMemberKind(member_struct, kind) ||
        CXXStructHasDeletedMemberSpecialMemberKind(member_struct, kind)) {
      return true;
    }
  }
  return false;
}

// A subobject participates in the enclosing class's defaulted move
// constructor / move assignment by move-constructing / move-assigning that
// subobject.  Overload resolution selects the subobject type's move operation
// when one is declared; otherwise the corresponding copy operation is selected
// (a `const&` copy operation binds an rvalue).  The enclosing defaulted move
// operation is defined as deleted when the selected operation is deleted
// ([class.copy.ctor]/11, [class.copy.assign]/7).  The dedicated "deleted move
// operation" checks alone miss the common case where a type has *no* move
// operation and its copy operation is deleted (e.g. a member with a
// user-declared move constructor has an implicitly deleted copy assignment and
// no move assignment at all).
static bool CXXStructSelectedMoveSpecialMemberIsDeleted(Struct* member_struct,
                                                        bool assignment) {
  if (member_struct == NULL) {
    return false;
  }
  CXXSpecialMemberKind move_kind = assignment
                                       ? kCXXSpecialMemberMoveAssignment
                                       : kCXXSpecialMemberMoveConstructor;
  CXXSpecialMemberKind copy_kind = assignment
                                       ? kCXXSpecialMemberCopyAssignment
                                       : kCXXSpecialMemberCopyConstructor;
  CXXSpecialMemberKind selected =
      CXXStructHasSpecialMemberKind(member_struct, move_kind) ? move_kind
                                                              : copy_kind;
  return CXXStructHasDeletedSpecialMemberKind(member_struct, selected) ||
         CXXStructHasDeletedBaseSpecialMemberKind(member_struct, selected);
}

static bool CXXStructMoveSpecialMemberDeletedByMembers(Struct* str,
                                                       bool assignment) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    TypeRecord* type = member->symbol->type;
    while (type != NULL && TypeIsFixedArray(type)) {
      type = type->next;
    }
    if (type == NULL || !TypeIsStructOrUnion(type) ||
        type->info.struct_info == NULL) {
      continue;
    }
    if (CXXStructSelectedMoveSpecialMemberIsDeleted(type->info.struct_info,
                                                    assignment)) {
      return true;
    }
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (CXXStructSelectedMoveSpecialMemberIsDeleted(base->type->info.struct_info,
                                                    assignment)) {
      return true;
    }
  }
  return false;
}

static bool CXXTypeNeedsDefaultInitializer(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    return CXXTypeNeedsDefaultInitializer(type->next);
  }
  return TypeIsReference(type) || TypeIsConst(type);
}

static bool CXXStructHasMemberWithoutDefaultInitialization(Struct* str) {
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member) || member->default_initializer != NULL) {
      continue;
    }
    if (CXXTypeNeedsDefaultInitializer(member->symbol->type)) {
      return true;
    }
  }
  return false;
}

static void AddCXXSyntheticMemberFunction(TypeParser* parser, Struct* str,
                                          Symbol* symbol) {
  StructMember* member = NewStructMember(symbol);
  member->is_member_function = true;
  member->access = kAccessPublic;
  StructMember* existing = MapFindPointerKey(&str->symbol_table,
                                             &symbol->name);
  if (existing != NULL) {
    AppendStructMemberOverload(parser, str, existing, member);
  } else {
    AddStructMember(parser, str, member);
  }
  bool dependent_owner =
      StructContainsTemplateParameter(str) ||
      (str->lexical_parent != NULL &&
       (str->lexical_parent->is_template ||
        StructContainsTemplateParameter(str->lexical_parent)));
  // Previously the body of a base-having copy/move constructor or assignment
  // was left unsynthesized here ("inherited_copy_or_assign"), on the assumption
  // that some later pass would materialize it.  Nothing did, so an implicitly
  // declared copy/move of a derived class was emitted as a weak *declaration*
  // with no definition -- a call to it resolved to a null address at link time
  // and crashed.  SyntaxInsertCXXConstructorPreamble (invoked from the synthesis
  // below) already emits the base-subobject copy/move calls and defers the vptr
  // initializers until the vtables are registered, so synthesizing now is
  // correct.  Templates still defer to instantiation via dependent_owner.
  if (!dependent_owner) {
    SynthesizeDefaultedMemberFunctionBody(parser, symbol);
  }
}

static bool CXXStructHasMemberFunctionNamed(Struct* str, const char* name,
                                            bool defaulted_only) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    for (StructMember* m = str->members.value.p[i]; m != NULL;
         m = m->overload_next) {
      if (!m->is_member_function || m->symbol == NULL ||
          m->symbol->type == NULL ||
          strcmp(m->symbol->name.value, name) != 0) {
        continue;
      }
      if (!defaulted_only || m->symbol->type->info.function.is_defaulted) {
        return true;
      }
    }
  }
  return false;
}

static void AddImplicitCXXEqualityOperator(TypeParser* parser, Struct* str,
                                           Symbol* tag) {
  if (!CompilerIsCXX() || str == NULL || tag == NULL ||
      !CXXStructHasMemberFunctionNamed(str, "operator<=>", true) ||
      CXXStructHasMemberFunctionNamed(str, "operator==", false)) {
    return;
  }
  SourceLocation location = tag->location;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constexpr = true;
  func->info.function.is_inline = true;
  func->info.function.is_defaulted = true;
  func->info.function.is_implicitly_declared = true;
  func->info.function.is_const_member = true;
  func->info.function.is_constexpr_eligible = true;
  func->info.function.is_noexcept_eligible = true;
  TypeRecordChain(func, NewTypeRecordWithSize(kTypeBool, kQualPlain));
  TypeRecordAddCXXThisParameter(func, str, location);
  AppendCXXSyntheticFormal(
      func, NewCXXSyntheticFormal(
                "__other", NewCXXClassReferenceType(str, true, false),
                location));

  Symbol* symbol = NewSymbol("operator==", func, STO(implicit));
  symbol->flags.invented = true;
  symbol->location = location;
  symbol->namespace_ = tag->namespace_;
  func->info.function.symbol = symbol;
  symbol->flags.is_defined = true;
  symbol->flags.is_inline_defn = true;
  if (!StorageIs(symbol->storage, STO(static))) {
    symbol->flags.is_weak = true;
  }
  symbol->value.func_defn = symbol;
  AddCXXSyntheticMemberFunction(parser, str, symbol);
}

static TypeRecord* CXXFindSpecialMemberFunction(Struct* str,
                                                CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return NULL;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    for (StructMember* member = str->members.value.p[i]; member != NULL;
         member = member->overload_next) {
      TypeRecord* func =
          member->is_member_function && member->symbol != NULL
              ? member->symbol->type
              : NULL;
      if (func != NULL && TypeIsFunction(func) &&
          func->info.function.cxx_special_member_kind == kind) {
        return func;
      }
    }
  }
  return NULL;
}

static bool CXXTypeSpecialMemberIsNoexcept(TypeRecord* type,
                                          CXXSpecialMemberKind kind) {
  if (type == NULL) {
    return true;
  }
  if (TypeIsFixedArray(type)) {
    return CXXTypeSpecialMemberIsNoexcept(type->next, kind);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return true;
  }
  TypeRecord* func =
      CXXFindSpecialMemberFunction(type->info.struct_info, kind);
  if (func == NULL && kind == kCXXSpecialMemberMoveConstructor) {
    func = CXXFindSpecialMemberFunction(type->info.struct_info,
                                        kCXXSpecialMemberCopyConstructor);
  } else if (func == NULL && kind == kCXXSpecialMemberMoveAssignment) {
    func = CXXFindSpecialMemberFunction(type->info.struct_info,
                                        kCXXSpecialMemberCopyAssignment);
  }
  return func != NULL && !func->info.function.is_deleted &&
         func->info.function.is_noexcept;
}

static bool CXXImplicitSpecialMemberIsNoexcept(Struct* str,
                                              CXXSpecialMemberKind kind) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base != NULL &&
        !CXXTypeSpecialMemberIsNoexcept(base->type, kind)) {
      return false;
    }
  }
  for (size_t i = 0; i < str->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = str->virtual_bases.value.p[i];
    if (base != NULL &&
        !CXXTypeSpecialMemberIsNoexcept(base->type, kind)) {
      return false;
    }
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member)) {
      continue;
    }
    if (!CXXTypeSpecialMemberIsNoexcept(member->symbol->type, kind)) {
      return false;
    }
  }
  return true;
}

static Symbol* NewCXXSyntheticSpecialMember(TypeParser* parser, Struct* str,
                                            Symbol* tag,
                                            const char* name,
                                            TypeRecord* return_type,
                                            CXXSpecialMemberKind kind,
                                            bool is_constructor,
                                            bool is_destructor,
                                            bool source_is_const,
                                            bool source_is_rvalue,
                                            bool deleted) {
  SourceLocation location = tag->location;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_constructor = is_constructor;
  func->info.function.is_destructor = is_destructor;
  func->info.function.is_constexpr = true;
  func->info.function.is_inline = true;
  func->info.function.is_defaulted = true;
  func->info.function.is_explicitly_defaulted = false;
  func->info.function.is_implicitly_declared = true;
  func->info.function.is_deleted = deleted;
  func->info.function.is_implicitly_deleted = deleted;
  func->info.function.cxx_special_member_kind = kind;
  func->info.function.is_constexpr_eligible = true;
  func->info.function.is_noexcept_eligible = true;
  func->info.function.is_noexcept =
      !deleted && CXXImplicitSpecialMemberIsNoexcept(str, kind);
  func->info.function.is_trivial_special_member =
      !deleted &&
      (kind != kCXXSpecialMemberDestructor ||
       (!StructNeedsImplicitCXXDestructor(str) && str->bases.length == 0 &&
        str->virtual_bases.length == 0));
  TypeRecordChain(func, return_type);
  TypeRecordAddCXXThisParameter(func, str, location);
  if (!is_destructor && (source_is_const || source_is_rvalue)) {
    AppendCXXSyntheticFormal(
        func,
        NewCXXSyntheticFormal("__other",
                              NewCXXClassReferenceType(str, source_is_const,
                                                       source_is_rvalue),
                              location));
  }

  Symbol* symbol = NewSymbol(name, func, STO(implicit));
  symbol->flags.invented = true;
  symbol->location = location;
  symbol->namespace_ = tag->namespace_;
  func->info.function.symbol = symbol;
  if (!deleted) {
    symbol->flags.is_defined = true;
    symbol->flags.is_inline_defn = true;
    if (!StorageIs(symbol->storage, STO(static))) {
      symbol->flags.is_weak = true;
    }
    symbol->value.func_defn = symbol;
  }
  (void)parser;
  return symbol;
}

void AddImplicitCXXSpecialMembers(TypeParser* parser, Struct* str,
                                         Symbol* tag) {
  if (!CompilerIsCXX() || parser == NULL || str == NULL || tag == NULL ||
      str->tag_name == NULL || str->cxx_special_members_complete ||
      tag->flags.invented ||
      strcmp(tag->name.value, "__va_list_tag") == 0 ||
      parser->syntax->parsing_template_declaration) {
    return;
  }
  bool user_declared_move =
      CXXStructHasUserDeclaredSpecialMemberKind(
          str, kCXXSpecialMemberMoveConstructor) ||
      CXXStructHasUserDeclaredSpecialMemberKind(
          str, kCXXSpecialMemberMoveAssignment);
  bool user_declared_copy_or_move = CXXStructHasUserDeclaredCopyOrMove(str);
  bool user_declared_destructor = CXXStructHasUserDeclaredDestructor(str);
  AddImplicitCXXEqualityOperator(parser, str, tag);
  if (!CXXStructHasAnyConstructor(str)) {
    Symbol* ctor = NewCXXSyntheticSpecialMember(
        parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberDefaultConstructor, true, false, false, false,
        CXXStructHasMemberWithoutDefaultInitialization(str) ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberDefaultConstructor) ||
            CXXStructHasDeletedMemberSpecialMemberKind(
                str, kCXXSpecialMemberDefaultConstructor));
    AddCXXSyntheticMemberFunction(parser, str, ctor);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberDestructor)) {
    String destructor_name;
    StringInit(&destructor_name, "~");
    StringAppendString(&destructor_name, str->tag_name);
    Symbol* dtor = NewCXXSyntheticSpecialMember(
        parser, str, tag, destructor_name.value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberDestructor, false, true, false, false,
        CXXStructHasDeletedBaseSpecialMemberKind(
            str, kCXXSpecialMemberDestructor) ||
            CXXStructHasDeletedMemberSpecialMemberKind(
                str, kCXXSpecialMemberDestructor));
    StringDestruct(&destructor_name);
    AddCXXSyntheticMemberFunction(parser, str, dtor);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberCopyConstructor)) {
    Symbol* copy = NewCXXSyntheticSpecialMember(
        parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberCopyConstructor, true, false, true, false,
        user_declared_move ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberCopyConstructor) ||
            CXXStructHasDeletedMemberSpecialMemberKind(
                str, kCXXSpecialMemberCopyConstructor));
    AddCXXSyntheticMemberFunction(parser, str, copy);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberCopyAssignment)) {
    TypeRecord* assign_return = NewCXXClassReferenceType(str, false, false);
    Symbol* copy_assign = NewCXXSyntheticSpecialMember(
        parser, str, tag, "operator=", assign_return,
        kCXXSpecialMemberCopyAssignment, false, false, true, false,
        user_declared_move || CXXStructHasUnassignableMember(str) ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberCopyAssignment) ||
            CXXStructHasDeletedMemberSpecialMemberKind(
                str, kCXXSpecialMemberCopyAssignment));
    AddCXXSyntheticMemberFunction(parser, str, copy_assign);
  }

  if (!user_declared_copy_or_move && !user_declared_destructor &&
      !CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberMoveConstructor)) {
    Symbol* move = NewCXXSyntheticSpecialMember(
        parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberMoveConstructor, true, false, false, true,
        CXXStructHasDeletedBaseSpecialMemberKind(
            str, kCXXSpecialMemberMoveConstructor) ||
            CXXStructHasDeletedMemberSpecialMemberKind(
                str, kCXXSpecialMemberMoveConstructor) ||
            CXXStructMoveSpecialMemberDeletedByMembers(
                str, /*assignment=*/false));
    AddCXXSyntheticMemberFunction(parser, str, move);
  }

  if (!user_declared_copy_or_move && !user_declared_destructor &&
      !CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberMoveAssignment)) {
    TypeRecord* move_return = NewCXXClassReferenceType(str, false, false);
    Symbol* move_assign = NewCXXSyntheticSpecialMember(
        parser, str, tag, "operator=", move_return,
        kCXXSpecialMemberMoveAssignment, false, false, false, true,
        CXXStructHasUnassignableMember(str) ||
            CXXStructHasDeletedBaseSpecialMemberKind(
                str, kCXXSpecialMemberMoveAssignment) ||
            CXXStructHasDeletedMemberSpecialMemberKind(
                str, kCXXSpecialMemberMoveAssignment) ||
            CXXStructMoveSpecialMemberDeletedByMembers(
                str, /*assignment=*/true));
    AddCXXSyntheticMemberFunction(parser, str, move_assign);
  }
  str->cxx_special_members_complete = true;
}

static bool CXXLambdaCaptureMemberIsPlaceholder(StructMember* member) {
  return member != NULL && member->symbol != NULL &&
         strcmp(member->symbol->name.value, "__lambda_empty") == 0;
}

static bool CXXMemberTypePreventsSpecialMember(TypeRecord* type,
                                               CXXSpecialMemberKind kind) {
  if (type == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    return CXXMemberTypePreventsSpecialMember(type->next, kind);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  Struct* member_struct = type->info.struct_info;
  return CXXStructHasDeletedSpecialMemberKind(member_struct, kind) ||
         CXXStructHasDeletedBaseSpecialMemberKind(member_struct, kind) ||
         CXXStructHasDeletedMemberSpecialMemberKind(member_struct, kind);
}

static bool CXXLambdaClosureSpecialMemberIsTrivial(Struct* str,
                                                   CXXSpecialMemberKind kind) {
  if (str == NULL) {
    return true;
  }
  if (kind == kCXXSpecialMemberDestructor) {
    return !StructNeedsImplicitCXXDestructor(str);
  }
  if (kind != kCXXSpecialMemberCopyConstructor &&
      kind != kCXXSpecialMemberMoveConstructor &&
      kind != kCXXSpecialMemberDefaultConstructor) {
    return true;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member) ||
        CXXLambdaCaptureMemberIsPlaceholder(member)) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    if (kind == kCXXSpecialMemberMoveConstructor) {
      if (CXXMemberTypePreventsSpecialMember(member_type,
                                             kCXXSpecialMemberMoveConstructor)) {
        return false;
      }
    } else if (CXXMemberTypePreventsSpecialMember(
                   member_type, kCXXSpecialMemberCopyConstructor)) {
      return false;
    }
    if (CXXDestructibleElementType(member_type) != NULL) {
      return false;
    }
  }
  return true;
}

static void CXXFinalizeLambdaClosureSpecialMember(Symbol* symbol, Struct* str,
                                                  bool deleted) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      deleted) {
    return;
  }
  CXXSpecialMemberKind kind =
      symbol->type->info.function.cxx_special_member_kind;
  symbol->type->info.function.is_trivial_special_member =
      CXXLambdaClosureSpecialMemberIsTrivial(str, kind);
}

static bool CXXLambdaCaptureMemberPreventsSpecialMember(
    Struct* str, CXXSpecialMemberKind kind) {
  if (str == NULL || kind == kCXXSpecialMemberNone) {
    return false;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StructMemberIsNestedType(member) ||
        CXXLambdaCaptureMemberIsPlaceholder(member)) {
      continue;
    }
    if (CXXMemberTypePreventsSpecialMember(member->symbol->type, kind)) {
      return true;
    }
  }
  return false;
}

void LambdaClosureRemoveEmptyPlaceholder(Struct* str) {
  if (str == NULL) {
    return;
  }
  StructMember* empty = FindStructMemberByName(str, "__lambda_empty");
  if (empty == NULL) {
    return;
  }
  for (size_t i = 0; i < str->members.length; i++) {
    if (str->members.value.p[i] == empty) {
      VectorDeleteElement(&str->members, i);
      break;
    }
  }
  String name;
  StringInit(&name, "__lambda_empty");
  MapRemove(&str->symbol_table, (MapKeyType){.p = &name});
  StringDestruct(&name);
  RelayoutStruct(str);
}

void AddImplicitLambdaClosureSpecialMembers(Syntax* syntax, Struct* str,
                                            Symbol* tag, bool has_capture_fields,
                                            bool has_explicit_template_params) {
  if (!CompilerIsCXX() || syntax == NULL || str == NULL || tag == NULL ||
      !tag->flags.invented || str->tag_name == NULL ||
      str->cxx_special_members_complete) {
    return;
  }

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);

  str->is_aggregate = false;
  ComputeCXXAggregateStatus(str);

  bool captureless = !has_capture_fields;
  bool allow_default_ctor = captureless && !has_explicit_template_params;

  if (allow_default_ctor && !CXXStructHasAnyConstructor(str)) {
    Symbol* ctor = NewCXXSyntheticSpecialMember(
        &parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberDefaultConstructor, true, false, false, false,
        false);
    CXXFinalizeLambdaClosureSpecialMember(ctor, str, false);
    AddCXXSyntheticMemberFunction(&parser, str, ctor);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberDestructor)) {
    String destructor_name;
    StringInit(&destructor_name, "~");
    StringAppendString(&destructor_name, str->tag_name);
    bool dtor_deleted =
        CXXStructHasDeletedBaseSpecialMemberKind(
            str, kCXXSpecialMemberDestructor) ||
        CXXStructHasDeletedMemberSpecialMemberKind(
            str, kCXXSpecialMemberDestructor);
    Symbol* dtor = NewCXXSyntheticSpecialMember(
        &parser, str, tag, destructor_name.value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberDestructor, false, true, false, false,
        dtor_deleted);
    StringDestruct(&destructor_name);
    CXXFinalizeLambdaClosureSpecialMember(dtor, str, dtor_deleted);
    AddCXXSyntheticMemberFunction(&parser, str, dtor);
  }

  bool copy_deleted =
      CXXStructHasDeletedBaseSpecialMemberKind(
          str, kCXXSpecialMemberCopyConstructor) ||
      CXXStructHasDeletedMemberSpecialMemberKind(
          str, kCXXSpecialMemberCopyConstructor) ||
      CXXLambdaCaptureMemberPreventsSpecialMember(
          str, kCXXSpecialMemberCopyConstructor);
  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberCopyConstructor)) {
    Symbol* copy = NewCXXSyntheticSpecialMember(
        &parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberCopyConstructor, true, false, true, false,
        copy_deleted);
    CXXFinalizeLambdaClosureSpecialMember(copy, str, copy_deleted);
    AddCXXSyntheticMemberFunction(&parser, str, copy);
  }

  bool move_deleted =
      copy_deleted ||
      CXXStructHasDeletedBaseSpecialMemberKind(
          str, kCXXSpecialMemberMoveConstructor) ||
      CXXStructHasDeletedMemberSpecialMemberKind(
          str, kCXXSpecialMemberMoveConstructor) ||
      CXXLambdaCaptureMemberPreventsSpecialMember(
          str, kCXXSpecialMemberMoveConstructor);
  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberMoveConstructor)) {
    Symbol* move = NewCXXSyntheticSpecialMember(
        &parser, str, tag, str->tag_name->value,
        NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        kCXXSpecialMemberMoveConstructor, true, false, false, true,
        move_deleted);
    CXXFinalizeLambdaClosureSpecialMember(move, str, move_deleted);
    AddCXXSyntheticMemberFunction(&parser, str, move);
  }

  bool assignment_deleted = !captureless;
  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberCopyAssignment)) {
    TypeRecord* assign_return = NewCXXClassReferenceType(str, false, false);
    Symbol* copy_assign = NewCXXSyntheticSpecialMember(
        &parser, str, tag, "operator=", assign_return,
        kCXXSpecialMemberCopyAssignment, false, false, true, false,
        assignment_deleted);
    AddCXXSyntheticMemberFunction(&parser, str, copy_assign);
  }

  if (!CXXStructHasSpecialMemberKind(str, kCXXSpecialMemberMoveAssignment)) {
    TypeRecord* move_return = NewCXXClassReferenceType(str, false, false);
    Symbol* move_assign = NewCXXSyntheticSpecialMember(
        &parser, str, tag, "operator=", move_return,
        kCXXSpecialMemberMoveAssignment, false, false, false, true,
        assignment_deleted);
    AddCXXSyntheticMemberFunction(&parser, str, move_assign);
  }

  str->cxx_special_members_complete = true;
  TypeParserDestruct(&parser);
}
