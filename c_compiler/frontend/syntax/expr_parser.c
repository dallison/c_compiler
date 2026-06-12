//
//  expr_parser.c
//  c_compiler
//
//  Created by David Allison on 10/30/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdint.h>

#include <assert.h>
#include <string.h>
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "preprocessor.h"
#include "statement_parser.h"
#include "type.h"

static struct Intrinsic {
  const char* name;
  ASTOpcode opcode;
  int num_args;
} intrinsics[] = {
    {"__builtin_va_start", AST_OP(builtin_va_start), 2},
    {"__builtin_va_arg", AST_OP(builtin_va_arg), 2},
    {"__builtin_va_end", AST_OP(builtin_va_end), 1},
    {"__builtin_va_copy", AST_OP(builtin_va_copy), 2},
    {NULL, 0, 0},
};

// Returns -1 for not intrinsic.
static int GetIntrinsicIndex(const char* name) {
  for (int i = 0; intrinsics[i].name != NULL; i++) {
    if (strcmp(name, intrinsics[i].name) == 0) {
      return i;
    }
  }
  return -1;
}

// Forward declarations.
static ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers);
static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type);

static ASTNode* ParseIdentifier(Syntax* syntax,
                                            TokenClass followers) {
  Lex* lex = syntax->lex;

  String name;
  StringInit(&name, lex->spelling.value);
  LexNextToken(lex);
  
  // In preprocesor mode we have no symbols, everything is a macro name.
  if (lex->preprocessor_mode) {
    if (PreprocessorFindMacro(lex->preprocessor, &name) == NULL) {
      PreprocessorWarning(lex->preprocessor, "undef",
                          "'%s' is not defined, evaluates to 0",
                          name.value);
    }
    ASTNode* result = NewMacroNameASTNode(&name, lex->current_token_location);
    StringDestruct(&name);
    return result;
  }
  
  // Find the symbol by searching all symbol tables.  It must exist.
  Symbol* symbol = SyntaxFindSymbol(syntax, &name);
  if (symbol == NULL) {
    if (lex->assembler_mode) {
      // In assembler mode we have symbols but we pre-declare them
      // if they don't exist.  They are declared as variables with
      // type unsigned long.
      TypeRecord* type = NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
      symbol = NewSymbol(name.value, type, STO(implicit));
      symbol->flags.is_forward_declared = true;
      // Register it so it is found on later references (and owned/freed by a
      // symbol table) rather than leaked.
      SyntaxAddSymbol(syntax, symbol);
    } else if (LexLookingAt(lex, TOK(lparen))) {
      if (GetIntrinsicIndex(name.value) == -1) {
        // Calling an unknown function is a warning.
        SyntaxWarning(syntax, "implicit-function-declaration",
                      "Calling undeclared function %s", name.value);
      }
      
      // Declare the function so we don't get more warnings for the same
      // function.
      TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      TypeRecord* func_type = NewFunctionTypeRecord();
      func_type->info.function.unknown_args = true;
      TypeRecordChain(func_type, type);
      symbol = NewSymbol(name.value, func_type, STO(implicit));
      symbol->flags.is_forward_declared = true;
      // Declare it so repeated calls find this symbol (no duplicate warnings)
      // and so it is owned/freed by a symbol table rather than leaked.
      SyntaxAddSymbol(syntax, symbol);
    } else {
      SyntaxError(syntax, "No such symbol \"%s\"", name.value);
      TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      symbol = NewSymbol(name.value, type, STO(implicit));
      symbol->flags.invented = true;
      SyntaxAddSymbol(syntax, symbol);
    }
  }
  StringDestruct(&name);
  return NewIdentifierASTNode(symbol, lex->current_token_location);
}

static ASTNode* ParseIntegerConstant(Syntax* syntax,
                                                 TokenClass followers) {
  
  Lex* lex = syntax->lex;
  int64_t value = lex->number;

  // The type of an integer constant is the first type from a candidate list
  // (C11 6.4.4.1) that can represent its value.  Decimal constants only
  // consider signed types when there is no 'u' suffix; hexadecimal and octal
  // constants may also pick an unsigned type.  Capture the radix and suffix
  // flags before advancing the lexer, which overwrites them.
  const char* spelling = lex->spelling.value;
  bool octal_or_hex = spelling != NULL && spelling[0] == '0' &&
                      (spelling[1] == 'x' || spelling[1] == 'X' ||
                       (spelling[1] >= '0' && spelling[1] <= '7'));
  bool has_u = StringContainsChar(&lex->suffix, 'U');
  bool has_ll = StringContainsString(&lex->suffix, "LL");
  bool has_l = !has_ll && StringContainsChar(&lex->suffix, 'L');
  LexNextToken(lex);

  uint64_t uval = (uint64_t)value;
  // Whether an unsigned candidate may be selected when there is no 'u' suffix.
  bool allow_unsigned = octal_or_hex;

  // Candidate integer ranks, in increasing width.  The bit widths are taken
  // from the active target (e.g. int is 16 bits on the 6502 but 32 bits
  // elsewhere) so that a literal is given the narrowest standard type that can
  // actually represent it on that target.
  Type rank_spec[3] = { kTypeInt, kTypeLong, kTypeLongLong };
  int rank_bits[3] = { SizeofType(kTypeInt) * 8,
                       SizeofType(kTypeLong) * 8,
                       SizeofType(kTypeLongLong) * 8 };

  // An explicit L/LL suffix sets a minimum rank.  A 'u' suffix forbids signed
  // candidates; hex/octal constants (or a 'u' suffix) permit unsigned ones.
  int min_rank = has_ll ? 2 : (has_l ? 1 : 0);
  bool try_signed = !has_u;
  bool try_unsigned = has_u || allow_unsigned;

  Type type_specifier = kTypeLongLong | kTypeUnsigned;
  for (int r = min_rank; r < 3; r++) {
    int bits = rank_bits[r];
    uint64_t smax = (bits >= 64) ? 0x7fffffffffffffffULL
                                 : ((1ULL << (bits - 1)) - 1);
    uint64_t umax = (bits >= 64) ? 0xffffffffffffffffULL
                                 : ((1ULL << bits) - 1);
    if (try_signed && uval <= smax) {
      type_specifier = rank_spec[r];
      break;
    }
    if (try_unsigned && uval <= umax) {
      type_specifier = rank_spec[r] | kTypeUnsigned;
      break;
    }
  }

  TypeRecord* type = NewTypeRecordWithSize(type_specifier, kQualPlain);
  return NewIntConstantASTNode(value, type,
                               syntax->lex->current_token_location);
}

static ASTNode* ParseFloatingPointConstant(Syntax* syntax,
                                                 TokenClass followers) {
  Lex* lex = syntax->lex;
  double value = lex->fnumber;
  LexNextToken(lex);
  
  // Floating point numbers have a suffix: F or L, meaning
  // F: float, L: long double.
  Type type_specifier = kTypeDouble;
  if (StringContainsChar(&lex->suffix, 'F')) {
    type_specifier &= ~kTypeDouble;
    type_specifier |= kTypeFloat;
  } else if (StringContainsChar(&lex->suffix, 'L')) {
    type_specifier &= ~kTypeDouble;
    type_specifier |= kTypeLongDouble;
  }
  TypeRecord* type = NewTypeRecordWithSize(type_specifier, kQualPlain);
  return NewRealConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

static ASTNode* ParseStringLiteral(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  String* contents = NewString(lex->spelling.value);
  LexNextToken(lex);
  
  // Adjacent string literals are joined together.
  while (LexLookingAt(lex, TOK(string))) {
    StringAppend(contents, lex->spelling.value);
    LexNextToken(lex);
  }
  
  TypeRecord* array =
    NewBasicArrayTypeRecord(kQualPlain, (int)contents->length + 1, false);
  TypeRecord* type = NewTypeRecordWithSize(kTypeChar, kQualPlain);
  TypeRecordChain(array, type);
  TypeRecordCalculateSize(array);
  return NewStringConstantASTNode(contents, array,
                                  syntax->lex->current_token_location);
  
}

static ASTNode* ParseWideStringLiteral(Syntax* syntax,
                                               TokenClass followers) {
  Lex* lex = syntax->lex;
  String* contents = NewStringWithLength(lex->spelling.value,
                                         lex->spelling.length + 4);
  LexNextToken(lex);
  
  // Adjacent wide string literals are joined together.
  while (LexLookingAt(lex, TOK(string_wide))) {
    StringAppend(contents, lex->spelling.value);
    LexNextToken(lex);
  }
  
  TypeRecord* array =
  NewBasicArrayTypeRecord(kQualPlain, (int)contents->length + 4, false);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  TypeRecordChain(array, type);
  return NewWideStringConstantASTNode(contents, array,
                                 syntax->lex->current_token_location);
}

static ASTNode* ParseCharacterConstant(Syntax* syntax,
                                                   TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecordWithSize(kTypeChar, kQualPlain);
  return NewCharConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

static ASTNode* ParseWideCharacterConstant(Syntax* syntax,
                                       TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  return NewCharConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}

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
  copy->qualifiers = kQualPlain;
  return copy;
}

// Canonicalize a primitive type's specifier bits for _Generic matching so that
// equivalent spellings compare equal (e.g. "long" == "signed long int").  The
// char family (char/signed char/unsigned char) stays distinct, as does
// signedness for the other integer types.
static int CanonicalPrimitive(int t) {
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
      if (a->qualifiers != b->qualifiers) {
        return false;
      }
      if (TypeIsStructOrUnion(a) || TypeIsStructOrUnion(b)) {
        return TypeIsStructOrUnion(a) && TypeIsStructOrUnion(b) &&
               a->info.struct_info == b->info.struct_info;
      }
      if (TypeIsEnum(a) || TypeIsEnum(b)) {
        return TypeIsEnum(a) && TypeIsEnum(b) &&
               a->info.enum_info == b->info.enum_info;
      }
      return CanonicalPrimitive(a->type) == CanonicalPrimitive(b->type);
  }
}

// Parse a C11 _Generic selection:
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

// primary-expression:
//   identifier
//   constant
//   string-literal
//   ( expression )
static ASTNode* ParsePrimaryExpression(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;

  // Check for parenthesized expression.
  // This either looks at the flag 'found_open_paren' in the Syntax
  // struct or looks for an open paren.  The syntax is slightly ambiguous
  // and the open paren can be the start of a cast expression or postfix
  // expression.
  if (syntax->found_open_paren || LexMatch(lex, TOK(lparen))) {
    syntax->found_open_paren = false;
    // GCC statement expression: ( { statements } ).  The value is that of the
    // last statement if it is an expression statement.
    if (LexLookingAt(lex, TOK(lbrace))) {
      ASTNode* compound = SyntaxParseStatement(syntax, followers | TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewUnaryASTNode(AST_OP(stmt_expr), NULL,
                             syntax->lex->current_token_location, compound);
    }
    ASTNode* node = SyntaxParseExpression(syntax, followers | TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    return node;
  }

  // C11 _Generic selection (lexes as an identifier).
  if (LexLookingAt(lex, TOK(identifier)) &&
      StringEqual(&lex->spelling, "_Generic")) {
    return ParseGenericSelection(syntax, followers);
  }

  // Check for identifier.
  if (LexLookingAt(lex, TOK(identifier))) {
    return ParseIdentifier(syntax, followers);
  }

  // Check for integer constant.
  if (LexLookingAt(lex, TOK(number))) {
    return ParseIntegerConstant(syntax, followers);
 }

  // Check for floating point constant.
  if (LexLookingAt(lex, TOK(fnumber))) {
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

// Check if we have a varargs intrinsic.
static ASTNode* VarargsIntrinsic(Syntax* syntax, ASTNode* left,
                               TokenClass followers) {
  if (left->op == AST_OP(identifier)) {
    IdentifierASTNode* id_node = (IdentifierASTNode*)left;
    const char* name = id_node->symbol->name.value;

    int intrinsic_index = GetIntrinsicIndex(name);
    if (intrinsic_index == -1) {
      return NULL;
    }

    Vector* actuals = NewVector();
    while (!LexLookingAt(syntax->lex, TOK(rparen))) {
      ASTNode* actual;
      // Special case: __builtin_va_arg has a type as its second arg.
      if (intrinsics[intrinsic_index].opcode == AST_OP(builtin_va_arg) &&
          actuals->length == 1) {
        TypeParser parser;
        TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingBlockScope);
        TypeRecord* type = TypeParserParseType(&parser, true);
        Symbol* sym = TypeParserParseDeclarator(&parser, type);
        type = sym->type;
        actual =
            NewIntConstantASTNode(0, type, syntax->lex->current_token_location);
        SymbolDelete(sym);  // Don't need this.
        TypeParserDestruct(&parser);
      } else {
        actual = SyntaxParseSingleExpression(syntax, followers);
      }
      VectorAppend(actuals, actual);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    if (actuals->length != intrinsics[intrinsic_index].num_args) {
      SyntaxError(
          syntax,
          "Wrong number of args for varargs builtin; expected %d, got %zd",
          intrinsics[intrinsic_index].num_args, actuals->length);
    }
    return NewVectorASTNode(intrinsics[intrinsic_index].opcode, NULL,
                            syntax->lex->current_token_location, left, actuals);
  }
  return NULL;
}

// Parse an array subscript expression.
static ASTNode* ParseArraySubscript(ASTNode* left, Syntax* syntax,
                                TokenClass followers) {
  ASTNode* index = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rsquare), followers);
  return NewBinaryASTNode(AST_OP(subscript), NULL,
                   syntax->lex->current_token_location, left, index);
}

// Parse a function call or varargs builtin.
static ASTNode* ParseFunctionCall(ASTNode* left, Syntax* syntax,
                                  TokenClass followers) {
  // __builtin_expect(expr, hint) is a branch-prediction hint that evaluates to
  // its first argument; the hint is ignored.
  if (left->op == AST_OP(identifier) &&
      StringEqual(&((IdentifierASTNode*)left)->symbol->name,
                  "__builtin_expect")) {
    ASTNode* value = NULL;
    while (!LexLookingAt(syntax->lex, TOK(rparen))) {
      ASTNode* arg = SyntaxParseSingleExpression(syntax, followers);
      if (value == NULL) {
        value = arg;
      } else {
        ASTNodeDelete(arg);
      }
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rparen), followers);
    if (value == NULL) {
      value = (ASTNode*)NewIntConstantASTNode(
          0, NewTypeRecordWithSize(kTypeLong, kQualPlain),
          syntax->lex->current_token_location);
    }
    return value;
  }

  // Check for varargs intrinsic functions.
  ASTNode* varargs = VarargsIntrinsic(syntax, left, followers);
  if (varargs != NULL) {
    return varargs;
  }
  
  // Normal function call.
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    ASTNode* actual = SyntaxParseSingleExpression(syntax, followers);
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

static ASTNode* ParseStructMember(ASTNode* left, ASTOpcode op, Syntax* syntax,
                                  TokenClass followers) {
  String* member_name;
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    member_name = NewString(syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  } else {
    SyntaxError(syntax, "Expected struct or union member name");
    member_name = NewString(SyntaxFakeName(syntax));
  }
  ASTNode* member_node = NewStringConstantASTNode(member_name,
                                          NULL,
                                          syntax->lex->current_token_location);
  return NewBinaryASTNode(op, NULL,
                          syntax->lex->current_token_location, left,
                          member_node);
}

static ASTNode* ParseCompoundLiteral(Syntax* syntax, TypeRecord* type) {
  SourceLocation location = syntax->lex->current_token_location;
  Symbol* sym = SyntaxNewTemporary(syntax, type);
  ASTNode* initializer = SyntaxParseInitializer(syntax, sym, syntax->init_storage);
  return NewCompoundLiteralASTNode(NewIdentifierASTNode(sym, location),
                                   location, initializer);
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

static ASTNode* ParsePostfixExpression(Syntax* syntax, TokenClass followers) {
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
  while (!LexEof(syntax->lex)) {
    if (LexMatch(syntax->lex, TOK(lsquare))) {
      result = ParseArraySubscript(result, syntax, followers);
    } else if (LexMatch(syntax->lex, TOK(lparen))) {
      result = ParseFunctionCall(result, syntax, followers);
     } else if (LexMatch(syntax->lex, TOK(plusplus))) {
      result = NewUnaryASTNode(AST_OP(postinc), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(minusminus))) {
      result = NewUnaryASTNode(AST_OP(postdec), NULL,
                               syntax->lex->current_token_location, result);
    } else if (LexMatch(syntax->lex, TOK(dot))) {
      result = ParseStructMember(result, AST_OP(dot), syntax,
                                 followers);
    } else if (LexMatch(syntax->lex, TOK(arrow))) {
      result = ParseStructMember(result, AST_OP(arrow), syntax,
                                 followers);
    } else {
      break;
    }
  }
  return result;
}


static ASTNode* ParsePossiblePreprocessorFunction(Syntax* syntax,
                                                  TokenClass followers) {
  
  if (StringEqual(&syntax->lex->spelling, "defined")) {
    LexNextToken(syntax->lex);
    String macro_name;
    if (LexMatch(syntax->lex, TOK(lparen))) {
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        StringInit(&macro_name, syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      } else {
        StringInit(&macro_name, NULL);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
    } else if (LexLookingAt(syntax->lex, TOK(identifier))) {
      StringInit(&macro_name, syntax->lex->spelling.value);
      LexNextToken(syntax->lex);
    }
    Macro* macro =
    PreprocessorFindMacro(syntax->lex->preprocessor, &macro_name);
    TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
    return NewIntConstantASTNode(macro == NULL ? 0 : 1, int_type,
                                 syntax->lex->current_token_location);
  } else if (StringEqual(&syntax->lex->spelling, "__has_feature")) {
    LexNextToken(syntax->lex);
    if (LexMatch(syntax->lex, TOK(lparen))) {
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        // TODO: handle __has_feature?
        LexNextToken(syntax->lex);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  } else if (StringEqual(&syntax->lex->spelling, "__has_include") ||
             StringEqual(&syntax->lex->spelling, "__has_include_next")) {
    bool include_next =
    StringEqual(&syntax->lex->spelling, "__has_include_next");
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      String filename;
      bool system_include = false;
      int64_t value = 0;
      if (PreprocessorParseIncludeFilename(
                                           syntax->lex->preprocessor,
                                           &syntax->lex->line,
                                           &syntax->lex->pos,
                                           &filename, &system_include)) {
        if (include_next) {
          value = PreprocessorHasIncludeNext(syntax->lex->preprocessor,
                                             &filename, system_include);
        } else {
          value = PreprocessorHasInclude(syntax->lex->preprocessor,
                                         &filename, system_include);
        }
      }
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rparen), followers);
      return NewIntConstantASTNode(value,
                                   NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                   syntax->lex->current_token_location);
    }
  }
  return NULL;
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

static ASTNode* ParseSizeof(Syntax* syntax, TokenClass followers) {
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

// Parse a unary expression with syntax:
// unary-expression:
//    postfix-expression
//    ++ unary-expression
//    -- unary-expression
//    unary-operator cast-expression
//    sizeof unary-expression
//    sizeof ( type-name )

// unary-operator: one of
//    & * + - ~ !

static ASTNode* ParseUnaryExpression(Syntax* syntax, TokenClass followers) {
  if (syntax->lex->preprocessor_mode) {
    // In preprocessor mode we have a unary operator that is a the identifier
    // 'defined'.  This is followed by a possibly parenthesized macro name.
    // The result is an int constant with value 1 if the macro exists and 0
    // otherwise.
    if (LexLookingAt(syntax->lex, TOK(identifier))) {
      ASTNode* result = ParsePossiblePreprocessorFunction(syntax, followers);
      if (result != NULL) {
        return result;
      }
    }
  }

  if (LexMatch(syntax->lex, TOK(plus))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(uplus), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(minus))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(uminus), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(amp))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    // If we are taking the address of an identifier we need to
    // set a flag so that the later phases know that this has
    // to be in memory (can't be in a register, if that is supported).
    if (sub->op == AST_OP(identifier)) {
      IdentifierASTNode* ident = (IdentifierASTNode*)sub;
      ident->symbol->flags.address_taken = true;
    }
    return NewUnaryASTNode(AST_OP(address), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(star))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(contents), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(tilde))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(onescomp), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(bang))) {
    ASTNode* sub = ParseCastExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(not), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(plusplus))) {
    ASTNode* sub = ParseUnaryExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(preinc), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(minusminus))) {
    ASTNode* sub = ParseUnaryExpression(syntax, followers);
    return NewUnaryASTNode(AST_OP(predec), NULL,
                           syntax->lex->current_token_location, sub);
  }

  if (LexMatch(syntax->lex, TOK(sizeof))) {
    return ParseSizeof(syntax, followers);
  }

  return ParsePostfixExpression(syntax, followers);
}

// Parse cast expression with syntax:
// cast-expression:
//    unary-expression
//    ( type-name ) cast-expression
//
// This also handles compound literals, which are actually postfix expressions.
static ASTNode* ParseCastExpression(Syntax* syntax, TokenClass followers) {
  if (!syntax->lex->preprocessor_mode && !syntax->lex->assembler_mode &&
      LexMatch(syntax->lex, TOK(lparen))) {
    // A leading __attribute__ (GCC extension) only appears in type names, so
    // treat "( __attribute__((...)) type-name )" as a cast / compound literal.
    if (SyntaxLookingAtType(syntax) ||
        LexLookingAt(syntax->lex, TOK(attribute))) {
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
  } else {
    return ParseUnaryExpression(syntax, followers);
  }
}

static ASTNode* ParseMultiplicativeExpression(Syntax* syntax,
                                              TokenClass followers) {
  ASTNode* result = ParseCastExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(star))) {
      ASTNode* right = ParseCastExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(mult), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(slash))) {
      ASTNode* right = ParseCastExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(div), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(percent))) {
      ASTNode* right = ParseCastExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(mod), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseAdditiveExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseMultiplicativeExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(plus))) {
      ASTNode* right = ParseMultiplicativeExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(plus), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(minus))) {
      ASTNode* right = ParseMultiplicativeExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(minus), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseShiftExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseAdditiveExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(lessless))) {
      ASTNode* right = ParseAdditiveExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(lshift), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(greatergreater))) {
      ASTNode* right = ParseAdditiveExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(rshift), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseRelationalExpression(Syntax* syntax,
                                          TokenClass followers) {
  ASTNode* result = ParseShiftExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(less))) {
      ASTNode* right = ParseShiftExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(less), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(lesseq))) {
      ASTNode* right = ParseShiftExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(lesseq), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(greater))) {
      ASTNode* right = ParseShiftExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(greater), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(greatereq))) {
      ASTNode* right = ParseShiftExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(greatereq), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseEqualityExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseRelationalExpression(syntax, followers);
  for (;;) {
    if (LexMatch(syntax->lex, TOK(equalequal))) {
      ASTNode* right = ParseRelationalExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(equal), NULL,
                           syntax->lex->current_token_location, result, right);
    } else if (LexMatch(syntax->lex, TOK(bangeq))) {
      ASTNode* right = ParseRelationalExpression(syntax, followers);
      result =
          NewBinaryASTNode(AST_OP(noteq), NULL,
                           syntax->lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseAndExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseEqualityExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(amp))) {
    ASTNode* right = ParseEqualityExpression(syntax, followers);
    result = NewBinaryASTNode(
        AST_OP(and), NULL, syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseExclusiveOrExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result = ParseAndExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(caret))) {
    ASTNode* right = ParseAndExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(exor), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseInclusiveOrExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result = ParseExclusiveOrExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(bar))) {
    ASTNode* right = ParseExclusiveOrExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(bitor), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseLogicalAndExpression(Syntax* syntax,
                                          TokenClass followers) {
  ASTNode* result = ParseInclusiveOrExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(ampamp))) {
    ASTNode* right = ParseInclusiveOrExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(logand), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseLogicalOrExpression(Syntax* syntax, TokenClass followers) {
  ASTNode* result = ParseLogicalAndExpression(syntax, followers);
  while (LexMatch(syntax->lex, TOK(barbar))) {
    ASTNode* right = ParseLogicalAndExpression(syntax, followers);
    result =
        NewBinaryASTNode(AST_OP(logor), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTNode* ParseConditionalExpression(Syntax* syntax,
                                           TokenClass followers) {
  ASTNode* result = ParseLogicalOrExpression(syntax, followers);
  if (LexMatch(syntax->lex, TOK(question))) {
    ASTNode* left = SyntaxParseExpression(syntax, followers);
    ASTNode* right = NULL;
    if (LexMatch(syntax->lex, TOK(colon))) {
      right = ParseConditionalExpression(syntax, followers);
      right =
          NewBinaryASTNode(AST_OP(colon), NULL,
                           syntax->lex->current_token_location, left, right);
    } else {
      SyntaxError(syntax, "Missing : in conditional expression");
    }
    result =
        NewBinaryASTNode(AST_OP(question), NULL,
                         syntax->lex->current_token_location, result, right);
  }
  return result;
}

static ASTOpcode AssignASTOpcode(Syntax* syntax, Token tok) {
  switch (tok) {
    case TOK(equal):
      return AST_OP(assign);
    case TOK(pluseq):
      return AST_OP(pluseq);
    case TOK(minuseq):
      return AST_OP(minuseq);
    case TOK(stareq):
      return AST_OP(multeq);
    case TOK(slasheq):
      return AST_OP(diveq);
    case TOK(percenteq):
      return AST_OP(percenteq);
    case TOK(lesslesseq):
      return AST_OP(lshifteq);
    case TOK(greatergreatereq):
      return AST_OP(rshifteq);
    case TOK(ampeq):
      return AST_OP(andeq);
    case TOK(bareq):
      return AST_OP(oreq);
    case TOK(careteq):
      return AST_OP(exoreq);
    default:
      assert(false);
      return 0;
  }
}

static ASTNode* ParseAssignmentExpression(Syntax* syntax,
                                          TokenClass followers) {
  ASTNode* result = ParseConditionalExpression(syntax, followers);
  if (syntax->lex->preprocessor_mode || syntax->lex->assembler_mode) {
    return result;
  }
  switch (syntax->lex->current_token) {
    case TOK(equal):
    case TOK(pluseq):
    case TOK(minuseq):
    case TOK(stareq):
    case TOK(slasheq):
    case TOK(percenteq):
    case TOK(lesslesseq):
    case TOK(greatergreatereq):
    case TOK(ampeq):
    case TOK(bareq):
    case TOK(careteq): {
      Token tok = syntax->lex->current_token;
      LexNextToken(syntax->lex);
      ASTNode* right = ParseAssignmentExpression(syntax, followers);
      result = NewBinaryASTNode(AssignASTOpcode(syntax, tok), NULL,
                                syntax->lex->current_token_location, result,
                                right);
      break;
    }
    default:
      break;
  }
  return result;
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
