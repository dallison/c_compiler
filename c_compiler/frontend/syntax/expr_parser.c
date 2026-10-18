//
//  expr_parser.c
//  c_compiler
//
//  Expression grammar: primary, postfix, unary, cast, and the public parse entry points.
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
#include "type_template_internal.h"
#include "compiler.h"
#include "type_traits_semantics.h"


// Every operand that is itself an expression -- the body of a parenthesized
// subexpression, an argument, a subscript -- is parsed by recursive descent, so
// each level of nesting costs stack whose amount the parser cannot observe.
// Bound the nesting instead: the limit is far above both what the language
// requires (63 levels in C, 256 in C++) and what any real program contains, and
// past it the input is rejected rather than the compiler crashing.
#define kMaxExpressionNesting 512


static TypeRecord* GenericControllingType(TypeRecord* ctype);
static int CanonicalPrimitive(int t);
static bool GenericTypeMatch(TypeRecord* a, TypeRecord* b);
static ASTNode* ParseGenericSelection(Syntax* syntax, TokenClass followers);
static bool TokenStartsFundamentalTypeSpecifier(Token token);
static ASTNode* ParseNestedPrimaryExpression(Syntax* syntax, TokenClass followers);
static void SkipOverdeepExpression(Syntax* syntax);
static ASTNode* ParsePrimaryExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseArraySubscript(ASTNode* left, Syntax* syntax, TokenClass followers);
static ASTNode* ParseFunctionCall(ASTNode* left, Syntax* syntax, TokenClass followers);
static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type);
static bool CXXPostfixExpressionNamesType(ASTNode* node);
static Vector* ParseCXXBracedTemporaryActuals(Syntax* syntax, TypeRecord* type, TokenClass followers);
static ASTNode* ParseCXXBracedTemporaryExpression(ASTNode* type_expr, Syntax* syntax, TokenClass followers);
static ASTNode* ParsePostfixOperators(Syntax* syntax, ASTNode* result, TokenClass followers);
static ASTNode* CloneVLASize(ASTNode* node, void* data);
static ASTNode* GetSizeofVLA(TypeRecord* type, SourceLocation location);
static ASTNode* TryParseSizeofNestedMember(Syntax* syntax, SourceLocation location);
static ASTNode* ParseSizeof(Syntax* syntax, TokenClass followers);
static ASTNode* ParseAlignof(Syntax* syntax, TokenClass followers);
static ASTNode* ParseTypeid(Syntax* syntax, TokenClass followers);
static ASTNode* ParseCXXThrowExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseCXXCoAwaitExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers);
static CastKind CXXNamedCastKind(Token token);
static bool TokenIsCXXNamedCast(Token token);
static ASTNode* ParseCXXNamedCastExpression(Syntax* syntax, TokenClass followers);
static bool LookingAtDefiningTagSpecifier(Syntax* syntax);
static bool CastOrCompoundLiteralFollows(Lex* lex);











// Parse the highest priority expression - a primary expression.
// This consists of one of:
// 1. A parenthesized expression.
// 2. An identifier
// 3. An integer or floating point constant
// 4. A string literal
// 5. A character constant
//
// We create an Abstract Syntax Tree node for it.  At this point we know
// the type of the AST node because it's either well defined (an integer
// constant for example) or can be read from the symbol table.
//
// The syntax for this is:

// Build the type used to match a _Generic controlling expression against the
// association type names.  This is the type of the controlling expression
// after lvalue conversion: array and function types decay to pointers and any
// top-level qualifiers are removed (C11 6.5.1.1).
static TypeRecord* GenericControllingType(TypeRecord* ctype) {
  if (TypeIsArray(ctype)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, ctype->next);
    TypeRecordCalculateSize(ptr);
    return ptr;
  }
  if (TypeIsFunction(ctype)) {
    TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
    TypeRecordChain(ptr, ctype);
    TypeRecordCalculateSize(ptr);
    return ptr;
  }
  TypeRecord* copy = TypeRecordCopy(ctype);
  SetQualifiers(copy, kQualPlain);
  return copy;
}


// Canonicalize a primitive type's specifier bits for _Generic matching so that
// equivalent spellings compare equal (e.g. "long" == "signed long int").  The
// Character types (char/signed char/unsigned char/char8_t) stay distinct, as
// does signedness for the other integer types.
static int CanonicalPrimitive(int t) {
  if (t & kTypeChar32) {
    return kTypeChar32;
  }
  if (t & kTypeChar16) {
    return kTypeChar16;
  }
  if (t & kTypeChar8) {
    return kTypeChar8;
  }
  if (t & kTypeChar) {
    return t & (kTypeChar | kTypeSigned | kTypeUnsigned);
  }
  if (t & (kTypeFloat | kTypeDouble | kTypeLongDouble | kTypeVoid | kTypeBool)) {
    return t & (kTypeFloat | kTypeDouble | kTypeLongDouble | kTypeVoid |
                kTypeBool);
  }
  int size = t & (kTypeShort | kTypeLong | kTypeLongLong);
  int sign = (t & kTypeUnsigned) ? kTypeUnsigned : kTypeSigned;
  return size | kTypeInt | sign;
}


// Type matching for _Generic associations.  Unlike TypeEqual this distinguishes
// distinct struct/union/enum tags (which must select different associations)
// and treats equivalent integer spellings as identical.
static bool GenericTypeMatch(TypeRecord* a, TypeRecord* b) {
  if ((a->type & kTypeUnknown) != 0 || (b->type & kTypeUnknown) != 0) {
    return true;
  }
  if (a->declarator != b->declarator) {
    return false;
  }
  switch (a->declarator) {
    case kDeclArray:
      return GenericTypeMatch(a->next, b->next) &&
             a->info.array.size.fixed == b->info.array.size.fixed;
    case kDeclPointer:
      return GenericTypeMatch(a->next, b->next);
    case kDeclFunction:
      return TypeEqual(a, b);
    case kDeclPrimitive:
    default:
      if (GetQualifiers(a) != GetQualifiers(b)) {
        return false;
      }
      if (TypeIsComplex(a) || TypeIsComplex(b)) {
        return TypeIsComplex(a) && TypeIsComplex(b) && TypeEqual(a, b);
      }
      if (TypeIsStructOrUnion(a) || TypeIsStructOrUnion(b)) {
        return TypeIsStructOrUnion(a) && TypeIsStructOrUnion(b) &&
               a->info.struct_info == b->info.struct_info;
      }
      if (TypeIsEnum(a) || TypeIsEnum(b)) {
        return TypeIsEnum(a) && TypeIsEnum(b) &&
               a->info.enum_info == b->info.enum_info;
      }
      if (TypeIsBitInt(a) || TypeIsBitInt(b)) {
        return TypeIsBitInt(a) && TypeIsBitInt(b) &&
               a->bit_width == b->bit_width &&
               TypeIsUnsigned(a) == TypeIsUnsigned(b);
      }
      return CanonicalPrimitive(a->type) == CanonicalPrimitive(b->type);
  }
}


// Parse a _Generic selection.  _Generic became a standard language construct
// in C11, but accepting its globally reserved spelling as an extension in
// earlier C modes lets the C99-mandated <tgmath.h> implement type-generic
// dispatch without evaluating its controlling arguments.
//   _Generic ( assignment-expression , generic-assoc-list )
//   generic-association:
//     type-name : assignment-expression
//     default : assignment-expression
// The controlling expression is an unevaluated operand: only its type is used
// to pick the matching association.  We return the selected expression (or the
// default), which is then analyzed normally as part of the surrounding AST.
static ASTNode* ParseGenericSelection(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  LexNextToken(lex);  // Consume "_Generic".
  SyntaxNeedBracket(syntax, TOK(lparen), followers);

  // Determine the controlling expression's type without keeping the node: it
  // is unevaluated, so we analyze it only to obtain its type.
  ASTNode* controlling =
      SyntaxParseSingleExpression(syntax, followers | TC(exprsep));
  controlling = AnalyzeExpression(controlling);
  TypeRecord* match_type =
      controlling->type != NULL ? GenericControllingType(controlling->type)
                                : NewTypeRecordWithSize(kTypeInt, kQualPlain);

  SyntaxNeedBracket(syntax, TOK(comma), followers);

  ASTNode* selected = NULL;
  ASTNode* default_expr = NULL;
  while (!LexEof(lex)) {
    bool is_default = false;
    TypeRecord* assoc_type = NULL;
    Symbol* assoc_sym = NULL;
    if (LexMatch(lex, TOK(default))) {
      is_default = true;
    } else {
      TypeParser parser;
      TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
      TypeRecord* type = TypeParserParseType(&parser, true);
      assoc_sym = TypeParserParseDeclarator(&parser, type);
      assoc_type = assoc_sym != NULL ? assoc_sym->type : type;
      TypeParserDestruct(&parser);
    }
    SyntaxNeedBracket(syntax, TOK(colon), followers);
    ASTNode* expr = SyntaxParseSingleExpression(syntax, followers | TC(exprsep));

    if (is_default) {
      default_expr = expr;
    } else if (selected == NULL && assoc_type != NULL &&
               GenericTypeMatch(match_type, assoc_type)) {
      selected = expr;
    } else {
      ASTNodeDelete(expr);
    }
    if (assoc_sym != NULL) {
      SymbolDelete(assoc_sym);
    }
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  ASTNode* result = selected != NULL ? selected : default_expr;
  if (result == NULL) {
    SyntaxError(syntax, "No matching association in _Generic selection");
    result = (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        syntax->lex->current_token_location);
  } else if (selected != NULL && default_expr != NULL) {
    ASTNodeDelete(default_expr);
  }
  return result;
}


// Allocate a LambdaCapture; the closure-class `field` is filled in later by
// AddLambdaCaptureFields.

// primary-expression:
//   identifier
//   constant
//   string-literal
//   ( expression )
// True if `token` begins a fundamental (built-in) simple-type-specifier that
// can introduce a functional-style cast `T(...)` / `T{...}`.  Class/enum tags,
// cv-qualifiers, and `auto` are deliberately excluded: they never start a
// functional cast in an expression.
static bool TokenStartsFundamentalTypeSpecifier(Token token) {
  switch (token) {
    case TOK(char):
    case TOK(char8_t):
    case TOK(char16_t):
    case TOK(char32_t):
    case TOK(short):
    case TOK(int):
    case TOK(int128):
    case TOK(long):
    case TOK(float):
    case TOK(double):
    case TOK(signed):
    case TOK(unsigned):
    case TOK(bool):
    case TOK(void):
    case TOK(wchar_t):
      return true;
    default:
      return false;
  }
}



// A reflection splice `[:r:]` or `[:r:]::name` used as a primary expression.
static ASTNode* ParseSplicePrimary(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);
  ASTNode* reflection =
      SyntaxParseExpression(syntax, TC(spliceclose));
  SyntaxNeedBracket(syntax, TOK(splice_close), followers);
  if (LexMatch(lex, TOK(coloncolon))) {
    if (!LexLookingAt(lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected identifier after namespace splice");
      return NewSpliceQualifiedASTNode(
          reflection,
          NewIdentifierASTNode(
              NewSymbol(SyntaxFakeName(syntax),
                        NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                              kQualPlain),
                        STO(implicit)),
              location),
          location);
    }
    String* suffix_name = NewString(lex->spelling.value);
    LexNextToken(lex);
    TypeRecord* unknown =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    Symbol* suffix_symbol = NewSymbol(suffix_name->value, unknown, STO(implicit));
    suffix_symbol->flags.invented = true;
    suffix_symbol->flags.is_forward_declared = true;
    ASTNode* suffix = NewIdentifierASTNode(suffix_symbol, location);
    return NewSpliceQualifiedASTNode(reflection, suffix, location);
  }
  return NewSpliceASTNode(reflection, kSpliceExpression, location);
}

// `^{ ... }` is a token-sequence literal. A bare caret is not one; restore
// the token and let a later production parse it.
static ASTNode* ParseCaretTokenSequencePrimary(Syntax* syntax,
                                               TokenClass followers) {
  Lex* lex = syntax->lex;
  LexCheckpoint caret_checkpoint;
  LexCheckpointSave(lex, &caret_checkpoint);
  LexNextToken(lex);
  if (LexLookingAt(lex, TOK(lbrace))) {
    LexCheckpointDestruct(&caret_checkpoint);
    SourceLocation location = caret_checkpoint.current_token_location;
    return ParseTokenSequenceLiteral(syntax, followers, location);
  }
  LexCheckpointRestore(lex, &caret_checkpoint);
  LexCheckpointDestruct(&caret_checkpoint);
  return NULL;
}

// A parenthesized expression, a GCC statement-expression `({ ... })`, or a
// fold expression. The opening '(' may already have been consumed by the cast
// lookahead (`found_open_paren`).
static ASTNode* ParseParenthesizedPrimary(Syntax* syntax,
                                          TokenClass followers) {
  Lex* lex = syntax->lex;
  // `found_open_paren` means the cast lookahead already consumed the '('.
  if (!syntax->found_open_paren) {
    LexMatch(lex, TOK(lparen));
  }
  syntax->found_open_paren = false;
  // A parenthesized sub-expression is a fresh top level: any '>' / '>>'
  // inside it is a relational/shift operator, not a template-list closer, so
  // suppress the template-argument context until the matching ')'.
  bool saved_parsing_template_argument = syntax->parsing_template_argument;
  syntax->parsing_template_argument = false;
  // GCC statement expression: ( { statements } ).  The value is that of the
  // last statement if it is an expression statement.
  if (LexLookingAt(lex, TOK(lbrace))) {
    ASTNode* compound = SyntaxParseStatement(syntax, followers | TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    syntax->parsing_template_argument = saved_parsing_template_argument;
    return NewUnaryASTNode(AST_OP(stmt_expr), NULL,
                           syntax->lex->current_token_location, compound);
  }
  ASTNode* fold = TryParseCXXFoldExpression(syntax, followers);
  if (fold != NULL) {
    syntax->parsing_template_argument = saved_parsing_template_argument;
    return fold;
  }
  ASTNode* node = SyntaxParseExpression(syntax, followers | TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  syntax->parsing_template_argument = saved_parsing_template_argument;
  if (node != NULL) {
    node->flags |= kASTParenthesized;
  }
  return node;
}

// `typename T::member` followed by `()` or `{}`. The identifier is a
// functional cast; postfix parsing builds the call or braced temporary.
static ASTNode* ParseTypenamePrimary(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  TypeParser type_parser;
  TypeParserInit(&type_parser, lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&type_parser, true);
  TypeParserDestruct(&type_parser);
  if (type == NULL) {
    type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  if (!LexLookingAt(lex, TOK(lparen)) && !LexLookingAt(lex, TOK(lbrace))) {
    SyntaxError(syntax,
                "expected '(' or '{' after typename-specifier in expression");
    SyntaxRecover(syntax, followers);
    return NewIntConstantASTNode(0, type, location);
  }
  Symbol* tag = NewSymbol("<typename>", type, STO(typedef));
  tag->flags.invented = true;
  return NewIdentifierASTNode(tag, location);
}

// `decltype(expr)`, optionally followed by `::member`, as a primary.
// `()` / `{}` after it is a functional cast of the decltype type.
static ASTNode* ParseDecltypePrimary(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  TypeParser type_parser;
  TypeParserInit(&type_parser, lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&type_parser, true);
  TypeParserDestruct(&type_parser);
  if (type == NULL) {
    type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  // TypeParserParseType also accepts `decltype(expr)::member` as a nested
  // type suffix.  In expression context that names a value (often still
  // dependent), so turn the already-consumed suffix into a qualified-id.
  if (type->dependent_member_name != NULL) {
    Symbol* placeholder = NewSymbol(type->dependent_member_name->value,
                                    TypeRecordCopy(type), STO(implicit));
    placeholder->flags.invented = true;
    ASTNode* node = NewIdentifierASTNode(placeholder, location);
    node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
    return node;
  }
  // `decltype(expr)::member` is a qualified-id whose nested-name-specifier
  // is a decltype-specifier (Abseil `decltype(Test<Hash>(0))::value`).
  if (LexMatch(lex, TOK(coloncolon))) {
    if (!LexLookingAt(lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected identifier after '::'");
      SyntaxRecover(syntax, followers);
      return NewIntConstantASTNode(0, type, location);
    }
    String member_name;
    StringInit(&member_name, lex->spelling.value);
    LexNextToken(lex);
    TypeRecord* scope = type;
    if (TypeIsReference(scope)) {
      scope = scope->next;
    }
    if (scope != NULL && TypeIsStructOrUnion(scope) &&
        scope->info.struct_info != NULL &&
        !TypeContainsTemplateParameter(scope)) {
      StructMember* member =
          FindStructMember(scope->info.struct_info, &member_name);
      if (member != NULL && member->symbol != NULL &&
          (member->is_static || member->symbol->flags.value_set ||
           StorageIs(member->symbol->storage, STO(typedef)))) {
        StringDestruct(&member_name);
        ASTNode* id = NewIdentifierASTNode(member->symbol, location);
        id->flags |= kASTQualifiedName;
        return id;
      }
    }
    TypeRecord* dependent =
        TypeRecordCopy(scope != NULL ? scope : type);
    if (dependent->dependent_member_name == NULL) {
      dependent->dependent_member_name = NewString(member_name.value);
    } else {
      StringAppend(dependent->dependent_member_name, "::");
      StringAppend(dependent->dependent_member_name, member_name.value);
    }
    Symbol* placeholder =
        NewSymbol(member_name.value, dependent, STO(implicit));
    placeholder->flags.invented = true;
    StringDestruct(&member_name);
    ASTNode* node = NewIdentifierASTNode(placeholder, location);
    node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
    return node;
  }
  if (!LexLookingAt(lex, TOK(lparen)) && !LexLookingAt(lex, TOK(lbrace))) {
    SyntaxError(syntax,
                "expected '(' or '{' after decltype-specifier in expression");
    SyntaxRecover(syntax, followers);
    return NewIntConstantASTNode(0, type, location);
  }
  Symbol* tag = NewSymbol("<decltype>", type, STO(typedef));
  tag->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(tag, location);
  // Mark the invented typedef as functional construction so `decltype(T)()`
  // and `decltype(T){}` skip value-analysis of the typedef name itself and
  // go through AnalyzeCXXFunctionalClassConstruction (scalar -> cast-from-0,
  // class -> constructor).
  node->flags |= kASTCXXFunctionalConstruction;
  return node;
}

// Functional-style cast of a fundamental type: `int(x)`, `bool{z}`,
// `double()`. Named class and typedef casts arrive as identifiers instead.
static ASTNode* ParseFundamentalFunctionalCast(Syntax* syntax,
                                               TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  TypeParser type_parser;
  TypeParserInit(&type_parser, lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&type_parser, true);
  TypeParserDestruct(&type_parser);
  bool brace_init = LexLookingAt(lex, TOK(lbrace));
  if (!brace_init && !LexLookingAt(lex, TOK(lparen))) {
    SyntaxError(syntax,
                "expected '(' or '{' after type in functional-style cast");
    SyntaxRecover(syntax, followers);
    return NewIntConstantASTNode(0, type, location);
  }
  Token close = brace_init ? TOK(rbrace) : TOK(rparen);
  LexNextToken(lex);
  ASTNode* argument = NULL;
  if (!LexLookingAt(lex, close)) {
    argument = SyntaxParseSingleExpression(syntax, followers | TC(exprsep) |
                                                       TC(closebra));
    if (LexLookingAt(lex, TOK(comma))) {
      SyntaxError(syntax,
                  "a functional-style cast to a fundamental type takes at "
                  "most one argument");
      while (LexMatch(lex, TOK(comma))) {
        ASTNodeDelete(SyntaxParseSingleExpression(
            syntax, followers | TC(exprsep) | TC(closebra)));
      }
    }
  }
  SyntaxNeedBracket(syntax, close, followers);
  if (argument == NULL) {
    // Value-initialization of a fundamental type yields a zero-valued prvalue
    // of that type.
    return NewCastASTNode(
        type, location,
        NewIntConstantASTNode(
            0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
  }
  return NewCastASTNode(type, location, argument);
}

// C++23 `auto(expr)` / `auto{expr}`. The target type is deduced later,
// after the operand has a type.
static ASTNode* ParseAutoFunctionalCast(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);
  bool brace_init = LexLookingAt(lex, TOK(lbrace));
  if (!brace_init && !LexLookingAt(lex, TOK(lparen))) {
    SyntaxError(syntax,
                "expected '(' or '{' after auto in functional-style cast");
    SyntaxRecover(syntax, followers);
    return NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
    SyntaxError(syntax, "auto(x) and auto{x} require C++23");
  }
  Token close = brace_init ? TOK(rbrace) : TOK(rparen);
  LexNextToken(lex);
  ASTNode* argument = NULL;
  if (!LexLookingAt(lex, close)) {
    argument = SyntaxParseSingleExpression(syntax, followers | TC(exprsep) |
                                                       TC(closebra));
    if (LexLookingAt(lex, TOK(comma))) {
      SyntaxError(syntax,
                  "a functional-style cast to auto requires exactly one "
                  "argument");
      while (LexMatch(lex, TOK(comma))) {
        ASTNodeDelete(SyntaxParseSingleExpression(
            syntax, followers | TC(exprsep) | TC(closebra)));
      }
    }
  }
  SyntaxNeedBracket(syntax, close, followers);
  if (argument == NULL) {
    SyntaxError(syntax,
                "a functional-style cast to auto requires exactly one "
                "argument");
    argument = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  ASTNode* result =
      NewCastASTNode(NewTypeRecord(kTypeAuto, kQualPlain), location, argument);
  ((CastASTNode*)result)->kind =
      brace_init ? kCastAutoBrace : kCastAutoParen;
  return result;
}

// A value injected by token replay (`injected_value`), cloned so later
// parsing does not alias the original tree.
static ASTNode* ParseInjectedValuePrimary(Syntax* syntax) {
  Lex* lex = syntax->lex;
  ASTNode* injected = LexCurrentInjectedValue(lex);
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);
  if (injected == NULL) {
    SyntaxError(syntax, "missing injected value during token replay");
    return NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  return ASTNodeClone(injected, IdentityCloneNode, NULL, NULL);
}

// `nullptr`, `true`, and `false`.
static ASTNode* ParseBooleanOrNullptrPrimary(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (LexLookingAt(lex, TOK(nullptr))) {
    LexNextToken(lex);
    TypeRecord* type = NewTypeRecordWithSize(kTypeNullPointer, kQualPlain);
    return NewIntConstantASTNode(0, type, syntax->lex->current_token_location);
  }
  bool value = LexLookingAt(lex, TOK(true));
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);
  return NewIntConstantASTNode(value ? 1 : 0,
                               NewTypeRecordWithSize(kTypeBool, kQualPlain),
                               location);
}

// A requires-expression. A dependent constraint stays in the tree; a
// concrete one folds to a bool constant.
static ASTNode* ParseRequiresPrimary(Syntax* syntax) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  ConstraintExpr* constraint = ConceptsParseRequiresExpression(syntax);
  if (ConceptsConstraintContainsTemplateParameter(constraint)) {
    return NewRequiresExpressionASTNode(constraint, location);
  }
  int64_t value = 0;
  bool ok = constraint != NULL &&
            ConceptsEvaluateConstraint(constraint, &value);
  ConstraintExprDelete(constraint);
  return NewIntConstantASTNode(ok && value != 0 ? 1 : 0,
                               NewTypeRecordWithSize(kTypeBool, kQualPlain),
                               location);
}

// An integer, floating, string, or character literal, including
// user-defined literal suffixes. Anything else is a syntax error and
// becomes a zero so parsing can continue.
static ASTNode* ParsePrimaryLiteral(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  // Check for integer constant.
if (LexLookingAt(lex, TOK(number))) {
  if (lex->ud_suffix.length != 0) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23) &&
        IsCXXSizeLiteralSuffix(&lex->ud_suffix)) {
      SyntaxError(syntax, "size_t literal suffix requires C++23");
    } else if (CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      char first =
          toupper((unsigned char)lex->ud_suffix.value[0]);
      char second =
          lex->ud_suffix.length > 1
              ? toupper((unsigned char)lex->ud_suffix.value[1])
              : '\0';
      if (first == 'Z' || (first == 'U' && second == 'Z')) {
        SyntaxError(syntax, "invalid size_t literal suffix");
      }
    }
    return ParseCXXUserDefinedIntegerLiteral(syntax);
  }
  return ParseIntegerConstant(syntax, followers);
 }

// Check for floating point constant.
if (LexLookingAt(lex, TOK(fnumber))) {
  if (lex->ud_suffix.length != 0) {
    return ParseCXXUserDefinedFloatingLiteral(syntax);
  }
  return ParseFloatingPointConstant(syntax, followers);
}

// Check for string literal.
if (LexLookingAt(lex, TOK(string))) {
  return ParseStringLiteral(syntax, followers);
}

// Wide string literal
if (LexLookingAt(lex, TOK(string_wide))) {
  return ParseWideStringLiteral(syntax, followers);
 }

// Character constant.
if (LexLookingAt(lex, TOK(charconst))) {
  return ParseCharacterConstant(syntax, followers);
}

// Wide character constant.
if (LexLookingAt(lex, TOK(charconst_wide))) {
  return ParseWideCharacterConstant(syntax, followers);
}

// Invalid primary expression, error out, recover and return 0.
SyntaxError(syntax, "Expression syntax error; primary expression expected");
SyntaxRecover(syntax, followers);
TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
return (ASTNode*)NewIntConstantASTNode(0, type,
                                       syntax->lex->current_token_location);
}

// primary-expression:
//   literal
//   this
//   ( expression )
//   id-expression
//   lambda-expression
//   fold-expression
//   requires-expression
// Each alternative is parsed by its own function. This one only decides
// which alternative the current token begins.
static ASTNode* ParseNestedPrimaryExpression(Syntax* syntax,
                                             TokenClass followers) {
  Lex* lex = syntax->lex;
  if (LexLookingAt(lex, TOK(splice_open))) {
    return ParseSplicePrimary(syntax, followers);
  }
  if (LexLookingAt(lex, TOK(caret))) {
    ASTNode* token_sequence =
        ParseCaretTokenSequencePrimary(syntax, followers);
    if (token_sequence != NULL) {
      return token_sequence;
    }
  }
  // The opening parenthesis may already have been consumed while deciding
  // this was not a cast.
  if (syntax->found_open_paren || LexLookingAt(lex, TOK(lparen))) {
    return ParseParenthesizedPrimary(syntax, followers);
  }
  // Before C11 `_Generic` is an ordinary identifier, so a program that
  // declares its own `_Generic` keeps that meaning.
  if (!CompilerIsCXX() && LexLookingAt(lex, TOK(identifier)) &&
      StringEqual(&lex->spelling, "_Generic") &&
      (CompilerCAtLeast(kLanguageStandardC11) ||
       SyntaxFindSymbol(syntax, &lex->spelling) == NULL)) {
    return ParseGenericSelection(syntax, followers);
  }
  if (CompilerIsCXX() && LexLookingAt(lex, TOK(typename))) {
    return ParseTypenamePrimary(syntax, followers);
  }
  if (CompilerIsCXX() && LexLookingAt(lex, TOK(decltype))) {
    return ParseDecltypePrimary(syntax, followers);
  }
  if (CompilerIsCXX() &&
      TokenStartsFundamentalTypeSpecifier(lex->current_token)) {
    return ParseFundamentalFunctionalCast(syntax, followers);
  }
  if (CompilerIsCXX() && LexLookingAt(lex, TOK(auto))) {
    return ParseAutoFunctionalCast(syntax, followers);
  }
  if (LexLookingAt(lex, TOK(injected_value))) {
    return ParseInjectedValuePrimary(syntax);
  }
  // `operator+` is an id-expression. The identifier parser reads the
  // operator-function-id.
  if (LexLookingAt(lex, TOK(identifier)) ||
      LexLookingAt(lex, TOK(coloncolon)) ||
      (CompilerIsCXX() && LexLookingAt(lex, TOK(operator)))) {
    return ParseIdentifier(syntax, followers);
  }
  if (LexLookingAt(lex, TOK(this))) {
    return ParseThisExpression(syntax);
  }
  if (LexLookingAt(lex, TOK(nullptr)) || LexLookingAt(lex, TOK(true)) ||
      LexLookingAt(lex, TOK(false))) {
    return ParseBooleanOrNullptrPrimary(syntax);
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX20) &&
      LexLookingAt(lex, TOK(requires))) {
    return ParseRequiresPrimary(syntax);
  }
  ASTNode* lambda = ParseCXXLambdaExpression(syntax, followers);
  if (lambda != NULL) {
    return lambda;
  }
  return ParsePrimaryLiteral(syntax, followers);
}



// Discard the expression that is nested too deeply to parse, keeping the
// brackets balanced.  Ordinary recovery would leave the rest of it in front of
// the levels already on the stack, and each would report and rescan the same
// input, so the cost of refusing one expression grew with the square of its
// length.  Stopping before a closing bracket this scan did not open, or before a
// separator that cannot appear inside an operand, hands each enclosing level the
// token it was waiting for instead.
static void SkipOverdeepExpression(Syntax* syntax) {
  Lex* lex = syntax->lex;
  int depth = 0;
  while (!LexEof(lex)) {
    switch (lex->current_token) {
      case TOK(lparen):
      case TOK(lsquare):
      case TOK(lbrace):
        depth++;
        break;
      case TOK(rparen):
      case TOK(rsquare):
      case TOK(rbrace):
        if (depth == 0) {
          return;
        }
        depth--;
        break;
      case TOK(comma):
      case TOK(semicolon):
      case TOK(colon):
        if (depth == 0) {
          return;
        }
        break;
      default:
        break;
    }
    LexNextToken(lex);
  }
}


static ASTNode* ParsePrimaryExpression(Syntax* syntax, TokenClass followers) {
  if (syntax->expression_nesting_depth >= kMaxExpressionNesting) {
    SyntaxError(syntax, "Expression nests more than %d levels deep",
                kMaxExpressionNesting);
    SkipOverdeepExpression(syntax);
    return NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain),
        syntax->lex->current_token_location);
  }
  syntax->expression_nesting_depth++;
  ASTNode* result = ParseNestedPrimaryExpression(syntax, followers);
  syntax->expression_nesting_depth--;
  return result;
}



// Check if we have a varargs intrinsic.

// Parse an array subscript expression.
static ASTNode* ParseArraySubscript(ASTNode* left, Syntax* syntax,
                                    TokenClass followers) {
  if (CompilerIsCXX() &&
      CompilerCXXAtLeast(kLanguageStandardCXX23)) {
    Vector* indices = NewVector();
    while (!LexLookingAt(syntax->lex, TOK(rsquare))) {
      ASTNode* index =
          LexMatch(syntax->lex, TOK(lbrace))
              ? SyntaxParseBracedInitializer(syntax)
              : SyntaxParseSingleExpression(
                    syntax, followers | TC(exprsep) | TC(closebra));
      MarkCXXPackExpansionIfPresent(syntax, index);
      VectorAppend(indices, index);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rsquare), followers);
    ASTNode* only_index =
        indices->length == 1 ? (ASTNode*)VectorGet(indices, 0) : NULL;
    if (only_index != NULL &&
        (only_index->flags & kASTPackExpansion) == 0) {
      ASTNode* index = only_index;
      VectorDelete(indices);
      return NewBinaryASTNode(AST_OP(subscript), NULL,
                              syntax->lex->current_token_location, left, index);
    }
    return NewVectorASTNode(AST_OP(subscript), NULL,
                            syntax->lex->current_token_location, left, indices);
  }
  ASTNode* index =
      CompilerIsCXX() && LexMatch(syntax->lex, TOK(lbrace))
          ? SyntaxParseBracedInitializer(syntax)
          : SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);
  return NewBinaryASTNode(AST_OP(subscript), NULL,
                          syntax->lex->current_token_location, left, index);
}


// __builtin_bit_cast(type, expression), with the opening bracket consumed.

// Parse a function call or varargs builtin.
static ASTNode* ParseFunctionCall(ASTNode* left, Syntax* syntax,
                                  TokenClass followers) {
  // Check for varargs intrinsic functions.
  ASTNode* varargs = VarargsIntrinsic(syntax, left, followers);
  if (varargs != NULL) {
    return varargs;
  }

  ASTNode* bit_cast = BitCastIntrinsic(syntax, left, followers);
  if (bit_cast != NULL) {
    return bit_cast;
  }

  ASTNode* type_trait = TypeTraitIntrinsic(syntax, left, followers);
  if (type_trait != NULL) {
    return type_trait;
  }

  // Normal function call.
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    ASTNode* actual = LexMatch(syntax->lex, TOK(lbrace))
                          ? SyntaxParseBracedInitializer(syntax)
                          : SyntaxParseSingleExpression(syntax,
                                                        followers | TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return NewVectorASTNode(AST_OP(call), NULL,
                            syntax->lex->current_token_location, left,
                            actuals);
}



// Whether the object expression of a member access is type-dependent (i.e.
// involves a template parameter), in which case a member named with a trailing
// template-argument list needs the `template` disambiguator under the standard.

static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type) {
  SourceLocation location = syntax->lex->current_token_location;
  type = TypeRecordCalculateSize(type);
  Symbol* sym = SyntaxNewTemporary(syntax, type);
  // The temporary is not the enclosing declarator.  Passing leftover
  // `typedef` / `extern` storage here rejects `return (T){...}` after a
  // typedef, which is how CMPLX() is spelled.
  ASTNode* initializer = SyntaxParseInitializer(syntax, sym, sym->storage);
  return NewCompoundLiteralASTNode(NewIdentifierASTNode(sym, location),
                                   location, initializer);
}


static bool CXXPostfixExpressionNamesType(ASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL) {
    return false;
  }
  // Concepts, variable templates and function templates are not types, so
  // `C<T>{}` / `trait_v<T>{}` is not a functional cast.  Treating them as
  // types also steals the function body after a trailing requires-clause
  // such as `requires (C<T>) { ... }` or `requires trait_v<T> { ... }`.
  if (id->symbol->flags.is_concept) {
    return false;
  }
  if (id->template_arguments != NULL) {
    if (StorageIs(id->symbol->storage, STO(typedef))) {
      return true;
    }
    if (id->symbol->type == NULL) {
      return false;
    }
    if (TypeIsFunction(id->symbol->type)) {
      return false;
    }
    return TypeIsStructOrUnion(id->symbol->type) ||
           TypeIsEnum(id->symbol->type);
  }
  if (id->symbol->type == NULL) {
    return false;
  }
  if (StorageIs(id->symbol->storage, STO(typedef))) {
    return true;
  }
  return TypeIsStructOrUnion(id->symbol->type);
}


static Vector* ParseCXXBracedTemporaryActuals(Syntax* syntax, TypeRecord* type,
                                              TokenClass followers) {
  StructMember* ctor = FindCXXConstructorForType(type);
  if (CXXNewConstructorSetHasInitializerList(ctor)) {
    Vector* actuals = NewVector();
    LexNextToken(syntax->lex);
    VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    return actuals;
  }
  return ParseCXXNewInitializerArguments(syntax, TOK(lbrace), followers);
}


static ASTNode* ParseCXXBracedTemporaryExpression(ASTNode* type_expr,
                                                  Syntax* syntax,
                                                  TokenClass followers) {
  bool dependent_type =
      CXXExpressionNamesTemplateTypeParameter(type_expr);
  if (!CXXPostfixExpressionNamesType(type_expr) && !dependent_type) {
    return NULL;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)type_expr;
  TypeRecord* type = id->symbol->type;
  SourceLocation location = type_expr->location;
  if (dependent_type) {
    Vector* actuals =
        ParseCXXNewInitializerArguments(syntax, TOK(lbrace), followers);
    ASTNode* call =
        NewVectorASTNode(AST_OP(call), NULL, location, type_expr, actuals);
    // Preserve the outer braces separately from a braced first argument.
    // Instantiating `T{{{a, b}}}` must rebuild all three brace levels, while
    // `T({{a, b}})` intentionally passes the inner list as one argument.
    call->flags |= kASTCXXBracedTemporary;
    return call;
  }
  // [dcl.init.list]: an aggregate is initialized memberwise, even when an
  // implicit (possibly deleted) default constructor exists.  `FindElement{set}`
  // binds the reference member; calling the deleted default constructor is
  // ill-formed.
  bool aggregate = TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
                   type->info.struct_info->is_aggregate;
  // A class template named without arguments needs deduction from its
  // guides ([over.match.class.deduct]) before it can be initialized.
  bool needs_deduction =
      id->template_arguments == NULL && id->symbol->flags.is_template &&
      TypeIsStructOrUnion(type) && type->template_arguments == NULL &&
      type->info.struct_info != NULL && type->info.struct_info->is_template;
  if (needs_deduction ||
      (!aggregate &&
       (id->template_arguments != NULL ||
        TypeIsClassTemplatePlaceholder(type) ||
        FindCXXConstructorForType(type) != NULL))) {
    type_expr->flags |= kASTCXXFunctionalConstruction;
    Vector* actuals =
        ParseCXXBracedTemporaryActuals(syntax, type, followers);
    return NewVectorASTNode(AST_OP(call), NULL, location, type_expr, actuals);
  }
  // C++ `Typedef{}` of a non-class type (including invented
  // `decltype(0){}` and `using I = int; I{}`) is a functional-style
  // cast, not a C compound literal.  Compound literals of scalars
  // generate unusable addresses at runtime.
  if (CompilerIsCXX() && StorageIs(id->symbol->storage, STO(typedef)) &&
      !TypeIsStructOrUnion(type)) {
    type_expr->flags |= kASTCXXFunctionalConstruction;
    Vector* actuals =
        ParseCXXNewInitializerArguments(syntax, TOK(lbrace), followers);
    return NewVectorASTNode(AST_OP(call), NULL, location, type_expr, actuals);
  }
  // The identifier's symbol type is the primary template.  Instantiating here
  // re-enters class completion while the surrounding declaration is still
  // being parsed and overflows in triviality checks.  Record the arguments on
  // the temporary and instantiate when the expression is analyzed.
  // The injected-class-name of a class template whose body is still open
  // (`Spy<void>{}` in a member of `Spy<T>::Combiner`) is its tag symbol, which
  // is not yet marked as a template but becomes one.
  TypeRecord* literal_type = TypeRecordCopy(type);
  if (id->template_arguments != NULL && TypeIsStructOrUnion(literal_type) &&
      literal_type->info.struct_info != NULL &&
      ((id->symbol->flags.is_template &&
        literal_type->info.struct_info->is_template) ||
       literal_type->info.struct_info->tag_symbol == id->symbol)) {
    if (literal_type->template_arguments != NULL) {
      VectorDeleteWithContents(
          literal_type->template_arguments,
          (VectorElementDestructor)TemplateArgumentDelete,
          /*free_element=*/false);
    }
    literal_type->template_origin = id->symbol;
    literal_type->template_arguments =
        TemplateArgumentVectorCopy(id->template_arguments);
  }
  // `T{...}` is a prvalue, not the C compound literal it is built from.
  ASTNode* literal = ParseCompoundLiteral(syntax, literal_type);
  if (literal != NULL && CompilerIsCXX()) {
    literal->flags |= kASTCXXBracedTemporary;
  }
  return literal;
}


// Parse a postfix-expression.  This is a primary expression with a postfixed
// operator.  The syntax is:

// postfix-expression:
//   primary-expression
//   postfix-expression [ expression ]
//   postfix-expression ( argument-expression-listopt )
//   postfix-expression . identifier
//   postfix-expression -> identifier
//   postfix-expression ++
//   postfix-expression --
//   ( type-name ) { initializer-list }
//   ( type-name ) { initializer-list , }

// argument-expression-list:
//   assignment-expression
//   argument-expression-list , assignment-expression

// Parse the trailing postfix operators ([], (), {}, ++, --, ., ->) of a
// postfix-expression, given an already-parsed primary `result`.  Shared by the
// ordinary postfix-expression parser and by named-cast parsing (a named cast
// such as `static_cast<T>(x)` is itself a postfix-expression and may be
// directly followed by `.member`, `[i]`, etc.).

static ASTNode* ParsePostfixOperators(Syntax* syntax, ASTNode* result,
                                      TokenClass followers) {
  while (!LexEof(syntax->lex)) {
    if (LookingAtCXXPackIndexSuffix(syntax)) {
      result = ParseCXXPackIndexExpression(syntax, result, followers);
    } else if (LexMatch(syntax->lex, TOK(lsquare))) {
      result = ParseArraySubscript(result, syntax, followers);
    } else if (LexMatch(syntax->lex, TOK(lparen))) {
      result = CXXExpressionNamesTemplateTypeParameter(result)
                   ? ParseCXXDependentValueInitialization(result, syntax,
                                                         followers)
                   : ParseFunctionCall(result, syntax, followers);
    } else if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(lbrace))) {
      ASTNode* braced =
          ParseCXXBracedTemporaryExpression(result, syntax, followers);
      if (braced == NULL) {
        break;
      }
      result = braced;
     } else if (LexMatch(syntax->lex, TOK(plusplus))) {
      result = NewUnaryASTNode(AST_OP(postinc), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(minusminus))) {
      result = NewUnaryASTNode(AST_OP(postdec), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(dot))) {
      result = ParseStructMember(result, AST_OP(dot), syntax,
                                 followers);
    } else if (CompilerIsCXX() &&
               LexLookingAt(syntax->lex, TOK(dotstar))) {
      if (MemberPointerOperatorPrecedesFoldEllipsis(syntax)) {
        break;
      }
      LexMatch(syntax->lex, TOK(dotstar));
      ASTNode* pm = ParseCastExpression(syntax, followers);
      result = NewBinaryASTNode(AST_OP(dotstar), NULL,
                                syntax->lex->current_token_location, result,
                                pm);
    } else if (LexMatch(syntax->lex, TOK(arrow))) {
      result = ParseStructMember(result, AST_OP(arrow), syntax,
                                 followers);
    } else if (CompilerIsCXX() &&
               LexLookingAt(syntax->lex, TOK(arrowstar))) {
      if (MemberPointerOperatorPrecedesFoldEllipsis(syntax)) {
        break;
      }
      LexMatch(syntax->lex, TOK(arrowstar));
      ASTNode* pm = ParseCastExpression(syntax, followers);
      result = NewBinaryASTNode(AST_OP(arrowstar), NULL,
                                syntax->lex->current_token_location, result,
                                pm);
    } else {
      break;
    }
  }
  return result;
}


ASTNode* ParsePostfixExpression(Syntax* syntax, TokenClass followers) {
  // A compound literal looks exactly like a cast except it is followed
  // by an initializer (in braces).  We've already consumed the ( type-name )
  // and determined it's not a cast, so we can parse the
  if (syntax->compound_literal_type != NULL && LexLookingAt(syntax->lex, TOK(lbrace))) {
    TypeRecord* type = syntax->compound_literal_type;
    syntax->compound_literal_type = NULL;
    return ParseCompoundLiteral(syntax, type);
  }
  ASTNode* result = ParsePrimaryExpression(syntax, followers);
  if (syntax->lex->assembler_mode) {
    // No postfix expressions in assembler mode.
    return result;
  }
  return ParsePostfixOperators(syntax, result, followers);
}




static ASTNode* CloneVLASize(ASTNode* node, void* data) {
  return node;
}


// Expression that is the size of an array for a VLAThis is the
// size expression of the VLA multiplied by the size of its type.
// sizeof(t) * GetSizeofVLA(t->next)
static ASTNode* GetSizeofVLA(TypeRecord* type, SourceLocation location) {
  ASTNode* size = ASTNodeClone(type->info.array.size.vla.size,
                               CloneVLASize, NULL, NULL);
  size->location = location;
  TypeRecord* t = type->next;
  do {
    ASTNode* next_size;
    if (TypeIsVLA(t)) {
      next_size = ASTNodeClone(t->info.array.size.vla.size,
                               CloneVLASize, NULL, NULL);
    } else {
      next_size = NewIntConstantASTNode(t->size,
                                   NewTypeRecordWithSize(
                                                 kTypeLong |
                                                 kTypeUnsigned,
                                                 kQualPlain), location);
    }
    size = NewBinaryASTNode(AST_OP(mult), t, location, size, next_size);
    t = t->next;
  } while (t != NULL && TypeIsArray(t));
  return size;
}


// `sizeof(Nested::member.submember)` names a non-static data member in
// unevaluated context (common in generated layout calculations).  Parse the
// qualified member and any following member chain directly so the first member
// is not resolved through the current member-function's injected scope.
static ASTNode* TryParseSizeofNestedMember(Syntax* syntax,
                                           SourceLocation location) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(identifier))) {
    return NULL;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  String nested_name;
  StringInit(&nested_name, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  if (!LexMatch(syntax->lex, TOK(coloncolon)) ||
      !LexLookingAt(syntax->lex, TOK(identifier))) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    StringDestruct(&nested_name);
    return NULL;
  }
  String member_name;
  StringInit(&member_name, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);

  Symbol* nested = SyntaxFindSymbol(syntax, &nested_name);
  if (nested == NULL || nested->type == NULL ||
      !TypeIsStructOrUnion(nested->type) ||
      nested->type->info.struct_info == NULL) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    StringDestruct(&nested_name);
    StringDestruct(&member_name);
    return NULL;
  }
  StructMember* member =
      FindStructMemberByName(nested->type->info.struct_info, member_name.value);
  StringDestruct(&nested_name);
  StringDestruct(&member_name);
  if (member == NULL || member->symbol == NULL || member->symbol->type == NULL) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  TypeRecord* member_type = member->symbol->type;
  while (LexMatch(syntax->lex, TOK(dot))) {
    if (!LexLookingAt(syntax->lex, TOK(identifier)) ||
        !TypeIsStructOrUnion(member_type) ||
        member_type->info.struct_info == NULL) {
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
      return NULL;
    }
    StructMember* nested_member = FindStructMemberByName(
        member_type->info.struct_info, syntax->lex->spelling.value);
    if (nested_member == NULL || nested_member->symbol == NULL ||
        nested_member->symbol->type == NULL) {
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
      return NULL;
    }
    member_type = nested_member->symbol->type;
    LexNextToken(syntax->lex);
  }
  // `sizeof(C::m + 0)` and `sizeof(C::f())` are ordinary expressions.
  if (!LexLookingAt(syntax->lex, TOK(rparen))) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);
  if (TypeLayoutDependsOnTemplate(member_type)) {
    return NewSizeofASTNodeWithType(member_type, location);
  }
  TypeRecordCalculateSize(member_type);
  return NewSizeofASTNodeWithKnownSize(member_type->size, location);
}
// Speculatively parses a type-name and abstract-or-named declarator with
// diagnostics trapped.  Returns true with `*sym_out` set (NULL when no type
// was parsed) when the parse finished without error; the caller then owns the
// symbol and must destruct `parser`.  On false nothing is owned: a longjmp
// leaves the parser partial, so it is abandoned.
static bool TrialParseTypeName(Syntax* syntax, TypeParser* parser,
                               Symbol** sym_out) {
  volatile bool parse_completed = false;
  Symbol* volatile sym = NULL;
  // `abort_on_error` makes a SyntaxError longjmp out immediately (so we stop
  // at the first one), while the trap additionally catches LexErrors (e.g.
  // "Missing close parenthesis"), which do not honor `abort_on_error`.
  volatile bool saved_trap = DiagnosticErrorTrapBegin();
  bool prev_abort_on_error = abort_on_error;
  // Save the global abort target: parsing the type-name can recurse through
  // the expression parser (e.g. an array bound) and set up its own trial,
  // which would otherwise clobber our jmp_buf.
  jmp_buf saved_abort_state;
  memcpy(saved_abort_state, error_abort_state, sizeof(error_abort_state));
  // A longjmp out of the trial parse skips the bookkeeping of every level
  // it entered, so restore the nesting depths by hand.  A type-name can
  // define a class (`(struct { int x; }){0}`), so both counters are at risk.
  // The class head is reset the same way: a stale head makes a later
  // namespace-scope consteval injection target that class.
  int saved_nesting_depth = syntax->expression_nesting_depth;
  int saved_struct_depth = syntax->struct_definition_depth;
  int saved_instantiation_depth = compiler->template_instantiation_depth;
  Struct* volatile saved_class_head = syntax->cxx_class_head;
  abort_on_error = true;
  if (setjmp(error_abort_state) == 0) {
    TypeParserInit(parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(parser, false);
    if (type != NULL) {
      sym = TypeParserParseDeclarator(parser, type);
    }
    parse_completed = true;
  }
  abort_on_error = prev_abort_on_error;
  memcpy(error_abort_state, saved_abort_state, sizeof(error_abort_state));
  syntax->expression_nesting_depth = saved_nesting_depth;
  syntax->struct_definition_depth = saved_struct_depth;
  compiler->template_instantiation_depth = saved_instantiation_depth;
  if (!parse_completed) {
    syntax->cxx_class_head = saved_class_head;
  }
  bool trapped = DiagnosticErrorTrapped();
  DiagnosticErrorTrapEnd(saved_trap);
  if (parse_completed && trapped) {
    if (sym != NULL) {
      SymbolDelete(sym);
    }
    TypeParserDestruct(parser);
    return false;
  }
  *sym_out = sym;
  return parse_completed;
}

static ASTNode* ParseSizeof(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(ellipsis))) {
    SyntaxNeedBracket(syntax, TOK(lparen), followers);
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected parameter pack name");
      SyntaxRecover(syntax, TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewSizeofPackASTNode(
          NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location),
          location);
    }
    Symbol* symbol = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
    if (symbol == NULL || !symbol->flags.is_parameter_pack) {
      SyntaxError(syntax, "sizeof... requires a parameter pack");
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewSizeofPackASTNode(
          NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location),
          location);
    }
    ASTNode* id = NewIdentifierASTNode(symbol, syntax->lex->current_token_location);
    if (symbol->flags.name_independent_lookup_ambiguous) {
      id->flags |= kASTNameIndependentLookupAmbiguous;
    }
    LexNextToken(syntax->lex);
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    return NewSizeofPackASTNode(id, location);
  }
  bool has_brackets = LexMatch(syntax->lex, TOK(lparen));
  ASTNode* result = NULL;
  bool sizeof_type_name = false;
  if (SyntaxLookingAtType(syntax)) {
    if (!has_brackets) {
      SyntaxError(syntax,
                  "Parentheses expected around type name "
                  " in sizeof operator");
    }
    sizeof_type_name = true;
  }
  // `sizeof(W<C>{c})` and `sizeof(T(x))` start with a type but are
  // expressions: the type-id does not end at the ')'.
  if (CompilerIsCXX() && has_brackets && sizeof_type_name) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    TypeParser trial;
    Symbol* sym = NULL;
    if (TrialParseTypeName(syntax, &trial, &sym)) {
      if (sym != NULL &&
          (!LexLookingAt(syntax->lex, TOK(rparen)) || !sym->flags.invented)) {
        sizeof_type_name = false;
      }
      if (sym != NULL) {
        SymbolDelete(sym);
      }
      TypeParserDestruct(&trial);
    }
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
  }
  if (has_brackets && !sizeof_type_name) {
    ASTNode* nested_member = TryParseSizeofNestedMember(syntax, location);
    if (nested_member != NULL) {
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return nested_member;
    }
  }
  if (sizeof_type_name) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    int size = -1;
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL) {
      if (TypeIsVLA(sym->type)) {
        // The size of a VLA is its size expression.
        result = GetSizeofVLA(sym->type,
                              syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      if (CompilerIsCXX() && TypeLayoutDependsOnTemplate(sym->type)) {
        // The size depends on a template parameter, so it cannot be computed
        // until the enclosing template is instantiated.  Retain the operand
        // type so it can be substituted and re-measured then.
        result = NewSizeofASTNodeWithType(
            sym->type, syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      size = sym->type->size;
      SymbolDelete(sym);
    }
    TypeParserDestruct(&parser);
    assert(size != -1);
    result = NewSizeofASTNodeWithKnownSize(size,
                                           syntax->lex->current_token_location);
  } else {
    if (has_brackets) {
      syntax->found_open_paren = true;
      has_brackets = false;
    }
    ASTNode* expr = ParseUnaryExpression(syntax, followers);
    result = NewSizeofASTNodeWithExpression(expr,
                                            syntax->lex->current_token_location);
  }
done:
  if (has_brackets) {
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
  }
  return result;
}


static ASTNode* ParseAlignof(Syntax* syntax, TokenClass followers) {
  bool has_brackets = LexMatch(syntax->lex, TOK(lparen));
  ASTNode* result = NULL;
  bool alignof_type_name = false;
  if (SyntaxLookingAtType(syntax)) {
    if (!has_brackets) {
      SyntaxError(syntax,
                  "Parentheses expected around type name "
                  " in alignof operator");
    }
    alignof_type_name = true;
  }
  if (alignof_type_name) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    int alignment = -1;
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL) {
      if (CompilerIsCXX() && TypeLayoutDependsOnTemplate(sym->type)) {
        result = NewAlignofASTNodeWithType(
            sym->type, syntax->lex->current_token_location);
        SymbolDelete(sym);
        TypeParserDestruct(&parser);
        goto done;
      }
      TypeRecordCalculateSize(sym->type);
      alignment = TypeRecordAlignment(sym->type);
      SymbolDelete(sym);
    }
    TypeParserDestruct(&parser);
    assert(alignment != -1);
    result = NewAlignofASTNodeWithKnownAlignment(
        alignment, syntax->lex->current_token_location);
  } else {
    if (has_brackets) {
      syntax->found_open_paren = true;
      has_brackets = false;
    }
    ASTNode* expr = ParseUnaryExpression(syntax, followers);
    result = NewAlignofASTNodeWithExpression(
        expr, syntax->lex->current_token_location);
  }
done:
  if (has_brackets) {
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
  }
  return result;
}


// Parses a typeid operator: `typeid ( type-id )` or `typeid ( expression )`.
// Disambiguates via SyntaxLookingAtType, mirroring the sizeof type/expression
// split.  The resulting node is rewritten into a type_info access during
// semantic analysis.
static ASTNode* ParseTypeid(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* result;
  if (SyntaxLookingAtType(syntax)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym != NULL && sym->type != NULL) {
      // Retain the declarator's type (sym is intentionally not deleted, like
      // the cast-expression type-name path).
      type = sym->type;
    }
    TypeParserDestruct(&parser);
    result = NewTypeidASTNodeWithType(type, location);
  } else {
    ASTNode* expr = SyntaxParseExpression(syntax, followers | TC(closebra));
    result = NewTypeidASTNodeWithExpression(expr, location);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return result;
}




static ASTNode* ParseCXXThrowExpression(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // throw
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon)) &&
      !LexLookingAt(syntax->lex, TOK(rparen)) &&
      !LexLookingAt(syntax->lex, TOK(comma))) {
    expr = ParseAssignmentExpression(syntax, followers);
  }
  return NewThrowASTNode(expr, location);
}


static ASTNode* ParseCXXCoAwaitExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    compiler->current_function->info.function.has_coroutine_syntax = true;
  }
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // co_await
  ASTNode* expr = ParseCastExpression(syntax, followers);
  return NewUnaryASTNode(AST_OP(co_await), NULL, location, expr);
}


ASTNode* ParseCXXCoYieldExpression(Syntax* syntax,
                                          TokenClass followers) {
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    compiler->current_function->info.function.has_coroutine_syntax = true;
  }
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // co_yield
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon)) &&
      !LexLookingAt(syntax->lex, TOK(rparen)) &&
      !LexLookingAt(syntax->lex, TOK(comma))) {
    expr = ParseAssignmentExpression(syntax, followers);
  } else {
    SyntaxError(syntax, "Expected expression after co_yield");
    expr = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  }
  return NewUnaryASTNode(AST_OP(co_yield), NULL, location, expr);
}


// `+ - * ~ !` take a cast-expression. `++` and `--` take a unary-expression
// so `++++x` parses. Returns NULL when the current token is not one of these.
static ASTNode* ParseSimpleUnaryOperator(Syntax* syntax, TokenClass followers) {
  ASTOpcode op;
  bool operand_is_unary = false;
  switch (syntax->lex->current_token) {
    case TOK(plus):
      op = AST_OP(uplus);
      break;
    case TOK(minus):
      op = AST_OP(uminus);
      break;
    case TOK(star):
      op = AST_OP(contents);
      break;
    case TOK(tilde):
      op = AST_OP(onescomp);
      break;
    case TOK(bang):
      op = AST_OP(not);
      break;
    case TOK(plusplus):
      op = AST_OP(preinc);
      operand_is_unary = true;
      break;
    case TOK(minusminus):
      op = AST_OP(predec);
      operand_is_unary = true;
      break;
    default:
      return NULL;
  }
  LexNextToken(syntax->lex);
  ASTNode* sub = operand_is_unary ? ParseUnaryExpression(syntax, followers)
                                  : ParseCastExpression(syntax, followers);
  return NewUnaryASTNode(op, NULL, syntax->lex->current_token_location, sub);
}

// `&` is either a pointer to member (`&C::m`) or the address operator.
// An addressed identifier has to live in memory, not a register.
static ASTNode* ParseAddressOperator(Syntax* syntax, TokenClass followers) {
  if (CompilerIsCXX()) {
    ASTNode* member_ptr = ParsePointerToMember(syntax, followers);
    if (member_ptr != NULL) {
      return member_ptr;
    }
  }
  ASTNode* sub = ParseCastExpression(syntax, followers);
  if (sub->op == AST_OP(identifier)) {
    IdentifierASTNode* ident = (IdentifierASTNode*)sub;
    ident->symbol->flags.address_taken = true;
  } else if (sub->op == AST_OP(splice)) {
    ((SpliceASTNode*)sub)->context = kSpliceAddressed;
  }
  return NewUnaryASTNode(AST_OP(address), NULL,
                         syntax->lex->current_token_location, sub);
}

// `noexcept(expression)` is a bool prvalue.
static ASTNode* ParseNoexceptExpression(Syntax* syntax, TokenClass followers) {
  SourceLocation location = syntax->lex->current_token_location;
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    SyntaxError(syntax, "Expected ( after noexcept");
    TypeRecord* type =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    return NewUnaryASTNode(AST_OP(noexcept_expr), NULL, location,
                           (ASTNode*)NewIntConstantASTNode(0, type, location));
  }
  ASTNode* expression = SyntaxParseSingleExpression(syntax, TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return NewUnaryASTNode(AST_OP(noexcept_expr), NULL, location, expression);
}

// `::new` / `::delete` name the global allocation functions. Returns NULL
// when the `::` introduces a qualified name instead, restoring the token.
static ASTNode* ParseGlobalNewOrDelete(Syntax* syntax, TokenClass followers) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);  // consume ::
  if (LexLookingAt(syntax->lex, TOK(new))) {
    LexCheckpointDestruct(&checkpoint);
    return ParseCXXNewExpression(syntax, followers, /*global_scope=*/true);
  }
  if (LexLookingAt(syntax->lex, TOK(delete))) {
    LexCheckpointDestruct(&checkpoint);
    return ParseCXXDeleteExpression(syntax, followers, /*global_scope=*/true);
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return NULL;
}

// unary-expression. Simple operators share ParseSimpleUnaryOperator; each
// keyword production (`sizeof`, `new`, `throw`, ...) has its own parser.
static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers) {
  if (syntax->lex->preprocessor_mode &&
      LexLookingAt(syntax->lex, TOK(identifier))) {
    // `defined name` / `defined(name)` is a preprocessor unary operator.
    ASTNode* result = ParsePossiblePreprocessorFunction(syntax, followers);
    if (result != NULL) {
      return result;
    }
  }

  ASTNode* simple = ParseSimpleUnaryOperator(syntax, followers);
  if (simple != NULL) {
    return simple;
  }
  if (LexMatch(syntax->lex, TOK(amp))) {
    return ParseAddressOperator(syntax, followers);
  }
  if (LexMatch(syntax->lex, TOK(sizeof))) {
    return ParseSizeof(syntax, followers);
  }
  if (LexMatch(syntax->lex, TOK(alignof))) {
    return ParseAlignof(syntax, followers);
  }
  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(noexcept))) {
    return ParseNoexceptExpression(syntax, followers);
  }
  if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(typeid))) {
    // typeid(...) is a postfix-expression, so `.name()` may follow.
    ASTNode* result = ParseTypeid(syntax, followers);
    return ParsePostfixOperators(syntax, result, followers);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(reflect))) {
    SourceLocation location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    ASTNode* result = ParseReflectionExpression(syntax, followers, location);
    return ParsePostfixOperators(syntax, result, followers);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(new))) {
    return ParseCXXNewExpression(syntax, followers, /*global_scope=*/false);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(delete))) {
    return ParseCXXDeleteExpression(syntax, followers, /*global_scope=*/false);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(coloncolon))) {
    ASTNode* global_new = ParseGlobalNewOrDelete(syntax, followers);
    if (global_new != NULL) {
      return global_new;
    }
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(throw))) {
    return ParseCXXThrowExpression(syntax, followers);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(co_await))) {
    return ParseCXXCoAwaitExpression(syntax, followers);
  }
  return ParsePostfixExpression(syntax, followers);
}


static CastKind CXXNamedCastKind(Token token) {
  switch (token) {
    case TOK(static_cast):
      return kCastStatic;
    case TOK(reinterpret_cast):
      return kCastReinterpret;
    case TOK(const_cast):
      return kCastConst;
    case TOK(dynamic_cast):
      return kCastDynamic;
    default:
      assert(false);
      return kCastCStyle;
  }
}


static bool TokenIsCXXNamedCast(Token token) {
  return token == TOK(static_cast) || token == TOK(reinterpret_cast) ||
         token == TOK(const_cast) || token == TOK(dynamic_cast);
}


static ASTNode* ParseCXXNamedCastExpression(Syntax* syntax,
                                            TokenClass followers) {
  Token cast_token = syntax->lex->current_token;
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);
  SyntaxNeedBracket(syntax, TOK(less), followers);

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, false);
  Symbol* sym = NULL;
  if (type == NULL) {
    SyntaxError(syntax, "Invalid type name");
    type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  } else {
    sym = TypeParserParseDeclarator(&parser, type);
    type = sym->type;
  }
  TypeParserDestruct(&parser);

  SyntaxNeedTemplateClose(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(closebra));
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  ASTNode* result = NewCastASTNode(type, location, expr);
  ((CastASTNode*)result)->kind = CXXNamedCastKind(cast_token);
  if (sym != NULL) {
    SymbolDelete(sym);
  }
  // A named cast is a postfix-expression, so it may be directly followed by
  // postfix operators, e.g. `static_cast<T>(x).member` or `static_cast<T>(p)->m`.
  return ParsePostfixOperators(syntax, result, followers);
}



// Parse cast expression with syntax:
// cast-expression:
//    unary-expression
//    ( type-name ) cast-expression
//
// This also handles compound literals, which are actually postfix expressions.

// True when the current token starts a class/enum/union *definition*
// (`enum {`, `struct S {`, ...), as opposed to a mere type-name (`enum E`).
static bool LookingAtDefiningTagSpecifier(Syntax* syntax) {
  Token tok = syntax->lex->current_token;
  if (tok != TOK(enum) && tok != TOK(struct) && tok != TOK(class) &&
      tok != TOK(union)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  int paren_depth = 0;
  bool found_brace = false;
  LexNextToken(syntax->lex);
  while (!LexEof(syntax->lex)) {
    if (paren_depth == 0 && LexLookingAt(syntax->lex, TOK(lbrace))) {
      found_brace = true;
      break;
    }
    if (paren_depth == 0 &&
        (LexLookingAt(syntax->lex, TOK(rparen)) ||
         LexLookingAt(syntax->lex, TOK(semicolon)) ||
         LexLookingAt(syntax->lex, TOK(rbrace)))) {
      break;
    }
    if (LexMatch(syntax->lex, TOK(lparen))) {
      paren_depth++;
      continue;
    }
    if (paren_depth > 0 && LexLookingAt(syntax->lex, TOK(rparen))) {
      paren_depth--;
      LexNextToken(syntax->lex);
      continue;
    }
    LexNextToken(syntax->lex);
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return found_brace;
}


// A cast or compound literal is `( type-name )` followed by an operand or a
// brace initializer.  `(TypeIdHelper<T>::dummy_)` is a parenthesized value;
// the `)` is followed by `;` or another closer, not by an expression.
static bool CastOrCompoundLiteralFollows(Lex* lex) {
  switch (lex->current_token) {
    case TOK(identifier):
    case TOK(coloncolon):
    case TOK(operator):
    case TOK(number):
    case TOK(fnumber):
    case TOK(string):
    case TOK(string_wide):
    case TOK(charconst):
    case TOK(charconst_wide):
    case TOK(lparen):
    case TOK(lbrace):
    case TOK(plus):
    case TOK(minus):
    case TOK(star):
    case TOK(amp):
    case TOK(plusplus):
    case TOK(minusminus):
    case TOK(bang):
    case TOK(tilde):
    case TOK(sizeof):
    case TOK(alignof):
    case TOK(new):
    case TOK(delete):
    case TOK(this):
    case TOK(nullptr):
    case TOK(true):
    case TOK(false):
    case TOK(throw):
    case TOK(requires):
    case TOK(lsquare):
    case TOK(static_cast):
    case TOK(reinterpret_cast):
    case TOK(const_cast):
    case TOK(dynamic_cast):
    case TOK(typeid):
    case TOK(noexcept):
    case TOK(co_await):
    case TOK(decltype):
    case TOK(typename):
    case TOK(bool):
    case TOK(char):
    case TOK(char8_t):
    case TOK(char16_t):
    case TOK(char32_t):
    case TOK(wchar_t):
    case TOK(short):
    case TOK(int):
    case TOK(long):
    case TOK(signed):
    case TOK(unsigned):
    case TOK(float):
    case TOK(double):
    case TOK(auto):
    case TOK(int128):
    case TOK(bitint):
    case TOK(caret):
    case TOK(reflect):
    case TOK(splice_open):
    case TOK(injected_value):
      return true;
    default:
      return false;
  }
}


// The '(' of a possible cast has already been consumed. A type-name makes
// this a cast or a compound literal; anything else is a parenthesized
// expression, with `found_open_paren` set so the primary parser does not
// consume another '('.
static ASTNode* ParseParenthesizedCast(Syntax* syntax, TokenClass followers) {

  // A leading __attribute__ (GCC extension) only appears in type names, so
  // treat "( __attribute__((...)) type-name )" as a cast / compound literal.
  if (SyntaxLookingAtType(syntax) ||
      SyntaxLookingAtAnyAttribute(syntax)) {
    // Nested type definitions in enumerator initializers are ill-formed.
    // Diagnose before the speculative `(type-name)` parse so the error is
    // not swallowed by abort_on_error, and recover without skipping the
    // enclosing enumerator-list `}`.
    if (CompilerIsCXX() && syntax->parsing_enum_specifier_depth > 0 &&
        LookingAtDefiningTagSpecifier(syntax)) {
      SourceLocation location = syntax->lex->current_token_location;
      SyntaxError(syntax, "Type cannot be defined in an enumeration");
      SyntaxRecover(syntax, TC(closebrace) | TC(closebra) | TC(semicolon));
      LexMatch(syntax->lex, TOK(rbrace));
      LexMatch(syntax->lex, TOK(rparen));
      return NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
    }
    // The leading '(' is followed by something that begins a type, so this is
    // potentially a cast or compound literal `( type-name ) ...`.  It can,
    // however, also be a parenthesized functional-cast expression such as
    // `(T(args))`, where `T(args)` is not a type-id (e.g. the arguments are
    // values rather than parameter declarations).  Tentatively parse the
    // type-name; if it does not form a well-formed `( type-name )` (i.e. the
    // type-name is not immediately followed by ')', or the parse hits an
    // error), backtrack and reparse the parenthesized construct as an
    // expression.  Valid casts always succeed here, so their behavior is
    // unchanged; only inputs that previously failed as casts are rerouted.
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    TypeParser parser;
    bool is_type_name = false;
    TypeRecord* type = NULL;
    Symbol* sym = NULL;
    bool parse_completed = TrialParseTypeName(syntax, &parser, &sym);
    if (parse_completed && sym != NULL) {
      type = sym->type;
      // A well-formed cast / compound-literal type-name is an abstract
      // declarator ending exactly at the closing ')'.  Two shapes of the
      // parenthesized functional cast `(T(arg))` slip past a naive check and
      // must be rejected so we backtrack to expression parsing:
      //   * a value argument is (mis)parsed as a *named* declarator, e.g.
      //     `(Fahrenheit(c))` -> "Fahrenheit c"; a real type-name names
      //     nothing, so require the declarator to be abstract (invented),
      //   * a type argument is (mis)parsed as a *function* type, e.g.
      //     `(weak_ordering(strong_ordering::x))`; a bare function type is
      //     never a valid cast target, so reject it (function pointers,
      //     which are valid, are not function types).
      is_type_name = LexLookingAt(syntax->lex, TOK(rparen)) &&
                     sym->flags.invented && !TypeIsFunction(type);
      // `(std::numeric_limits<T>::max)()` is a parenthesized call, not a
      // cast to a dependent nested type followed by `()`.
      if (is_type_name) {
        LexCheckpoint peek;
        LexCheckpointSave(syntax->lex, &peek);
        LexMatch(syntax->lex, TOK(rparen));
        bool followed_by_call =
            type != NULL && type->dependent_member_name != NULL &&
            LexLookingAt(syntax->lex, TOK(lparen));
        bool operand = CastOrCompoundLiteralFollows(syntax->lex);
        LexCheckpointRestore(syntax->lex, &peek);
        LexCheckpointDestruct(&peek);
        if (followed_by_call || !operand) {
          is_type_name = false;
        }
      }
    }
    if (!is_type_name) {
      // Not a well-formed `( type-name )`; abandon the trial parse (its
      // TypeParser is only released when it completed cleanly, since a
      // longjmp leaves it in a partial state) and reparse as an expression.
      if (parse_completed) {
        if (sym != NULL) {
          SymbolDelete(sym);
        }
        TypeParserDestruct(&parser);
      }
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
      syntax->found_open_paren = true;
      return ParsePostfixExpression(syntax, followers);
    }
    LexCheckpointDestruct(&checkpoint);
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    // If the (type-name) is followed by an initializer list we have
    // a compound literal.  This is actually a postfix expression so we
    // save the parsed type and move forward.  The initializer will be
    // parsed by the postfix expression parser.
    if (LexLookingAt(syntax->lex, TOK(lbrace))) {
      TypeRecord* saved_type = syntax->compound_literal_type;
      syntax->compound_literal_type = type;
      ASTNode* result = ParseCastExpression(syntax, followers);
      syntax->compound_literal_type = saved_type;
      if (sym != NULL) {
        // The compound literal will create its own symbol.
        SymbolDelete(sym);
      }
      TypeParserDestruct(&parser);
      return result;
    }
    ASTNode* expr = ParseCastExpression(syntax, followers);
    // The cast AST node will take ownership of the TypeRecord pointer.
    ASTNode* result =
        NewCastASTNode(type, syntax->lex->current_token_location, expr);
    if (sym != NULL) {
      SymbolDelete(sym);
    }
    TypeParserDestruct(&parser);
    return result;
  } else {
    // Not a type name.  We have consumed the open paren so we
    // know that it is present.  Set a flag to tell all downstream
    // parsers that we've found and consumed it.
    syntax->found_open_paren = true;
    return ParsePostfixExpression(syntax, followers);
  }
}

ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers) {
  if (CompilerIsCXX() && TokenIsCXXNamedCast(syntax->lex->current_token)) {
    return ParseCXXNamedCastExpression(syntax, followers);
  }
  if (!syntax->lex->preprocessor_mode && !syntax->lex->assembler_mode &&
      LexMatch(syntax->lex, TOK(lparen))) {
    return ParseParenthesizedCast(syntax, followers);
  }
  return ParseUnaryExpression(syntax, followers);
}




ASTNode* SyntaxParseExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseAssignmentExpression(syntax, followers | TC(exprsep));
  while (LexMatch(syntax->lex, TOK(comma))) {
    ASTNode* right = ParseAssignmentExpression(syntax, followers | TC(exprsep));
    result =
        NewBinaryASTNode(AST_OP(comma), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}


ASTNode* SyntaxParseSingleExpression(Syntax* syntax, TokenClass followers) {
  return ParseAssignmentExpression(syntax, followers);
}


ASTNode* SyntaxParseConditionalExpression(Syntax* syntax,
                                          TokenClass followers) {
  return ParseConditionalExpression(syntax, followers);
}


ASTNode* SyntaxParseConstraintExpression(Syntax* syntax,
                                         TokenClass followers) {
  return ParseBinaryExpressionOperands(syntax, followers,
                                       kBinaryPrecedenceLogicalOr, true);
}
