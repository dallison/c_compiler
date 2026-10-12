//
//  expr_parser_literal.c
//  c_compiler
//
//  Integer, floating, character, and string literals, including user-defined literals.
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


typedef enum {
  kCXXNumericLiteralOperatorInvalid,
  kCXXNumericLiteralOperatorCooked,
  kCXXNumericLiteralOperatorRaw,
  kCXXNumericLiteralOperatorTemplate,
} CXXNumericLiteralOperatorKind;


typedef struct {
  CXXNumericLiteralOperatorKind kind;
  Symbol* symbol;
} CXXNumericLiteralOperator;


static void CXXUserDefinedLiteralOperatorName(String* name, String* suffix);
static ASTNode* NewCXXLiteralOperatorIdentifier(Syntax* syntax, String* name, SourceLocation location);
static Type WideLiteralElementType(void);
static Type UTF8LiteralElementType(void);
static ASTNode* NewStringLiteralArgument(String* contents, SourceLocation location, LiteralEncoding encoding);
static ASTNode* NewCXXUserDefinedLiteralCall(Syntax* syntax, String* suffix, Vector* actuals, SourceLocation location);
static ASTNode* NewCXXUserDefinedLiteralCallForSymbol( Syntax* syntax, String* suffix, Symbol* symbol, Vector* template_arguments, Vector* actuals, SourceLocation location);
static TypeRecord* FirstFunctionFormal(Symbol* symbol);
static bool IsCookedNumericLiteralOperator(Symbol* symbol, bool floating);
static bool IsRawNumericLiteralOperator(Symbol* symbol);
static CXXNumericLiteralOperator ResolveCXXNumericLiteralOperator( Syntax* syntax, String* suffix, bool floating);
static Vector* CXXNumericLiteralTemplateArguments(String* spelling);

// Parses an integer token, applies its base and suffix rules, and returns a
// typed integer constant.
ASTNode* ParseIntegerConstant(Syntax* syntax,
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
                       spelling[1] == 'b' || spelling[1] == 'B' ||
                       (spelling[1] >= '0' && spelling[1] <= '7'));
  bool has_u = StringContainsChar(&lex->suffix, 'U');
  bool has_wb = StringContainsString(&lex->suffix, "WB");
  bool has_z = StringContainsChar(&lex->suffix, 'Z');
  bool has_ll = StringContainsString(&lex->suffix, "LL");
  bool has_l = !has_ll && StringContainsChar(&lex->suffix, 'L');
  SourceLocation location = lex->current_token_location;
  if (has_wb) {
    uint64_t magnitude = (uint64_t)value;
    int value_bits = 0;
    for (uint64_t remaining = magnitude; remaining != 0; remaining >>= 1) {
      value_bits++;
    }
    int bit_width = has_u ? (value_bits > 0 ? value_bits : 1)
                          : (value_bits > 0 ? value_bits + 1 : 2);
    if (bit_width > DAVECC_BITINT_MAXWIDTH) {
      SyntaxError(syntax,
                  "wb integer literal requires more than BITINT_MAXWIDTH bits");
      bit_width = DAVECC_BITINT_MAXWIDTH;
    }
    TypeRecord* type =
        NewBitIntTypeRecord(bit_width, has_u, kQualPlain);
    LexNextToken(lex);
    return NewIntConstantASTNode(value, type, location);
  }
  if (has_z) {
    TypeRecord* type = NewSizeTypeRecord();
    if (!has_u) {
      // The plain z suffix denotes the signed integer type corresponding to
      // size_t; adding u selects size_t itself.
      type->type &= ~kTypeUnsigned;
    }
    LexNextToken(lex);
    return NewIntConstantASTNode(value, type, location);
  }
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
  return NewIntConstantASTNode(value, type, location);
}


// Recognizes the C++23 z and uz size-type literal suffixes.
bool IsCXXSizeLiteralSuffix(String* suffix) {
  if (suffix == NULL || (suffix->length != 1 && suffix->length != 2)) {
    return false;
  }
  char first = toupper((unsigned char)suffix->value[0]);
  if (suffix->length == 1) {
    return first == 'Z';
  }
  char second = toupper((unsigned char)suffix->value[1]);
  return (first == 'U' && second == 'Z') ||
         (first == 'Z' && second == 'U');
}


// Builds the operator-function name corresponding to a user-defined literal
// suffix.
static void CXXUserDefinedLiteralOperatorName(String* name, String* suffix) {
  StringInit(name, "operator\"\"");
  StringAppendString(name, suffix);
}


// Looks up a literal operator and creates the identifier used as the literal
// call’s callee.
static ASTNode* NewCXXLiteralOperatorIdentifier(Syntax* syntax, String* name,
                                                SourceLocation location) {
  Symbol* symbol = SyntaxFindSymbol(syntax, name);
  if (symbol == NULL) {
    SyntaxError(syntax, "No literal operator %s", name->value);
    TypeRecord* result_type =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    TypeRecord* func_type = NewFunctionTypeRecord();
    func_type->info.function.unknown_args = true;
    TypeRecordChain(func_type, result_type);
    symbol = NewSymbol(name->value, func_type, STO(implicit));
    symbol->flags.invented = true;
  }
  return NewIdentifierASTNode(symbol, location);
}


// C++ `wchar_t` is a distinct type (`L"x"` is `const wchar_t[2]`); C's is an
// `int` typedef.
static Type WideLiteralElementType(void) {
  return CompilerIsCXX() ? kTypeWchar : kTypeInt;
}


// Returns the language-version-appropriate element type for a UTF-8 string
// literal.
static Type UTF8LiteralElementType(void) {
  if (CompilerCAtLeast(kLanguageStandardC23)) {
    return kTypeChar | kTypeUnsigned;
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    return kTypeChar8;
  }
  return kTypeChar;
}


// Builds the correctly encoded and typed array constant passed to a literal
// operator.
static ASTNode* NewStringLiteralArgument(String* contents,
                                         SourceLocation location,
                                         LiteralEncoding encoding) {
  int element_size = 1;
  Type element_type = kTypeChar;
  if (encoding == kLiteralEncodingWide) {
    element_size = compiler->wchar_size;
    element_type = WideLiteralElementType();
  } else if (encoding == kLiteralEncodingUTF16) {
    element_size = 2;
    element_type = kTypeChar16;
  } else if (encoding == kLiteralEncodingUTF32) {
    element_size = 4;
    element_type = kTypeChar32;
  } else if (encoding == kLiteralEncodingUTF8) {
    element_type = UTF8LiteralElementType();
  }
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)(contents->length / element_size) + 1, false);
  TypeRecord* element = NewTypeRecordWithSize(
      element_type, CompilerIsCXX() ? kQualConst : kQualPlain);
  TypeRecordChain(array, element);
  TypeRecordCalculateSize(array);
  return element_size > 1
             ? NewWideStringConstantASTNode(contents, array, location)
             : NewStringConstantASTNode(contents, array, location);
}


// Builds a user-defined literal call whose operator will be resolved from its
// suffix.
static ASTNode* NewCXXUserDefinedLiteralCall(Syntax* syntax, String* suffix,
                                             Vector* actuals,
                                             SourceLocation location) {
  return NewCXXUserDefinedLiteralCallForSymbol(
      syntax, suffix, NULL, NULL, actuals, location);
}


// Builds a user-defined literal call for a selected operator symbol and
// optional template arguments.
static ASTNode* NewCXXUserDefinedLiteralCallForSymbol(
    Syntax* syntax, String* suffix, Symbol* symbol, Vector* template_arguments,
    Vector* actuals, SourceLocation location) {
  String name;
  CXXUserDefinedLiteralOperatorName(&name, suffix);
  ASTNode* callee =
      symbol != NULL ? NewIdentifierASTNode(symbol, location)
                     : NewCXXLiteralOperatorIdentifier(syntax, &name, location);
  ((IdentifierASTNode*)callee)->template_arguments = template_arguments;
  StringDestruct(&name);
  return NewVectorASTNode(AST_OP(call), NULL, location, callee, actuals);
}


// Returns the first formal parameter type of a function symbol, if it has one.
static TypeRecord* FirstFunctionFormal(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.prototype.length == 0) {
    return NULL;
  }
  Symbol* formal = symbol->type->info.function.prototype.value.p[0];
  return formal != NULL ? formal->type : NULL;
}


// Reports whether a literal-operator overload accepts the cooked numeric value
// type.
static bool IsCookedNumericLiteralOperator(Symbol* symbol, bool floating) {
  if (symbol == NULL || symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.prototype.length != 1) {
    return false;
  }
  TypeRecord* formal = FirstFunctionFormal(symbol);
  return floating ? TypeIsLongDouble(formal)
                  : TypeIsUnsignedLongLong(formal);
}


// Reports whether a literal-operator overload accepts the raw spelling as
// const char pointer.
static bool IsRawNumericLiteralOperator(Symbol* symbol) {
  if (symbol == NULL || symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.prototype.length != 1) {
    return false;
  }
  TypeRecord* formal = FirstFunctionFormal(symbol);
  return TypeIsPointer(formal) && formal->next != NULL &&
         formal->next->declarator == kDeclPrimitive &&
         formal->next->type == kTypeChar && TypeIsConst(formal->next);
}


// Selects the cooked, raw, or template literal-operator form for a numeric
// suffix.
static CXXNumericLiteralOperator ResolveCXXNumericLiteralOperator(
    Syntax* syntax, String* suffix, bool floating) {
  String name;
  CXXUserDefinedLiteralOperatorName(&name, suffix);
  Symbol* first = SyntaxFindSymbol(syntax, &name);
  Symbol* cooked = NULL;
  Symbol* raw = NULL;
  Symbol* templ = NULL;
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (IsCookedNumericLiteralOperator(candidate, floating)) {
      cooked = candidate;
    } else if (IsRawNumericLiteralOperator(candidate)) {
      raw = candidate;
    } else if (SyntaxIsCXXNumericLiteralOperatorTemplate(candidate)) {
      templ = candidate;
    }
  }
  CXXNumericLiteralOperator result = {
      .kind = kCXXNumericLiteralOperatorInvalid,
      .symbol = first,
  };
  if (cooked != NULL) {
    result.kind = kCXXNumericLiteralOperatorCooked;
    result.symbol = cooked;
  } else if (raw != NULL && templ != NULL) {
    SyntaxError(syntax,
                "Raw and numeric literal operator template cannot both be "
                "declared for %s",
                name.value);
    result.kind = kCXXNumericLiteralOperatorRaw;
    result.symbol = raw;
  } else if (raw != NULL) {
    result.kind = kCXXNumericLiteralOperatorRaw;
    result.symbol = raw;
  } else if (templ != NULL) {
    result.kind = kCXXNumericLiteralOperatorTemplate;
    result.symbol = templ;
  } else if (first != NULL) {
    SyntaxError(syntax, "No matching numeric literal operator %s", name.value);
  }
  StringDestruct(&name);
  return result;
}


// Converts the characters of a numeric token spelling into non-type template
// arguments.
static Vector* CXXNumericLiteralTemplateArguments(String* spelling) {
  Vector* arguments = NewVector();
  size_t length = spelling->length;
  if (length > 0 && spelling->value[length - 1] == '\0') {
    length--;
  }
  for (size_t i = 0; i < length; i++) {
    VectorAppend(arguments, NewIntegralTemplateArgument(
                                (unsigned char)spelling->value[i]));
  }
  return arguments;
}


// Parses an integer user-defined literal and builds the selected
// literal-operator call.
ASTNode* ParseCXXUserDefinedIntegerLiteral(Syntax* syntax) {
  Lex* lex = syntax->lex;
  int64_t value = lex->number;
  SourceLocation location = lex->current_token_location;
  String suffix;
  StringInit(&suffix, NULL);
  StringSetString(&suffix, &lex->ud_suffix);
  String spelling;
  StringInit(&spelling, NULL);
  StringSetString(&spelling, &lex->literal_spelling);
  CXXNumericLiteralOperator literal_operator =
      ResolveCXXNumericLiteralOperator(syntax, &suffix, /*floating=*/false);
  LexNextToken(lex);

  Vector* actuals = NewVector();
  Vector* template_arguments = NULL;
  if (literal_operator.kind == kCXXNumericLiteralOperatorRaw) {
    VectorAppend(actuals, NewStringLiteralArgument(
                              NewString(spelling.value), location,
                              kLiteralEncodingNone));
  } else if (literal_operator.kind == kCXXNumericLiteralOperatorTemplate) {
    template_arguments = CXXNumericLiteralTemplateArguments(&spelling);
  } else {
    // [lex.ext] fixes the cooked integer argument type regardless of the
    // ordinary type the unsuffixed token would otherwise have.
    TypeRecord* type =
        NewTypeRecordWithSize(kTypeLongLong | kTypeUnsigned, kQualPlain);
    VectorAppend(actuals, NewIntConstantASTNode(value, type, location));
  }
  StringDestruct(&spelling);
  ASTNode* call = NewCXXUserDefinedLiteralCallForSymbol(
      syntax, &suffix, literal_operator.symbol, template_arguments, actuals,
      location);
  StringDestruct(&suffix);
  return call;
}


// Parses a floating token and applies its suffix to select float, double, or
// long double.
ASTNode* ParseFloatingPointConstant(Syntax* syntax,
                                                 TokenClass followers) {
  Lex* lex = syntax->lex;
  double value = lex->fnumber;
  LexNextToken(lex);

  // Floating point numbers have the standard F/L suffixes and, in C++23,
  // fixed-width extended f32/f64 suffixes.
  Type type_specifier = kTypeDouble;
  if (StringEqual(&lex->suffix, "F32")) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(syntax, "The f32 floating-point suffix requires C++23");
    }
    type_specifier = kTypeFloat32;
  } else if (StringEqual(&lex->suffix, "F64")) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(syntax, "The f64 floating-point suffix requires C++23");
    }
    if (StringEqual(compiler->target_name, "6502") ||
        StringEqual(compiler->target_name, "65c02")) {
      SyntaxError(syntax,
                  "The binary64 extended floating-point type is not "
                  "supported on this target");
    }
    type_specifier = kTypeFloat64;
  } else if (StringContainsChar(&lex->suffix, 'F')) {
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


// Parses a floating user-defined literal and builds the selected
// literal-operator call.
ASTNode* ParseCXXUserDefinedFloatingLiteral(Syntax* syntax) {
  Lex* lex = syntax->lex;
  double value = lex->fnumber;
  SourceLocation location = lex->current_token_location;
  String suffix;
  StringInit(&suffix, NULL);
  StringSetString(&suffix, &lex->ud_suffix);
  String spelling;
  StringInit(&spelling, NULL);
  StringSetString(&spelling, &lex->literal_spelling);
  CXXNumericLiteralOperator literal_operator =
      ResolveCXXNumericLiteralOperator(syntax, &suffix, /*floating=*/true);
  LexNextToken(lex);

  Vector* actuals = NewVector();
  Vector* template_arguments = NULL;
  if (literal_operator.kind == kCXXNumericLiteralOperatorRaw) {
    VectorAppend(actuals, NewStringLiteralArgument(
                              NewString(spelling.value), location,
                              kLiteralEncodingNone));
  } else if (literal_operator.kind == kCXXNumericLiteralOperatorTemplate) {
    template_arguments = CXXNumericLiteralTemplateArguments(&spelling);
  } else {
    // [lex.ext] fixes the cooked floating argument type at long double.
    TypeRecord* type = NewTypeRecordWithSize(kTypeLongDouble, kQualPlain);
    VectorAppend(actuals, NewRealConstantASTNode(value, type, location));
  }
  StringDestruct(&spelling);
  ASTNode* call = NewCXXUserDefinedLiteralCallForSymbol(
      syntax, &suffix, literal_operator.symbol, template_arguments, actuals,
      location);
  StringDestruct(&suffix);
  return call;
}


// Concatenates adjacent narrow or UTF string tokens and handles a user-defined
// string suffix.
ASTNode* ParseStringLiteral(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  LiteralEncoding encoding = lex->literal_encoding;
  bool user_defined = lex->ud_suffix.length != 0;
  String suffix;
  StringInit(&suffix, NULL);
  if (user_defined) {
    StringSetString(&suffix, &lex->ud_suffix);
  }
  String* contents =
      NewStringWithLength(lex->spelling.value, lex->spelling.length);
  LexNextToken(lex);

  // Adjacent string literals are joined together.
  while (LexLookingAt(lex, TOK(string))) {
    LiteralEncoding next_encoding = lex->literal_encoding;
    if (next_encoding != encoding) {
      if (CompilerCXXAtLeast(kLanguageStandardCXX23) ||
          (encoding != kLiteralEncodingNone &&
           next_encoding != kLiteralEncodingNone)) {
        SyntaxError(syntax,
                    "Cannot concatenate string literals with different "
                    "encoding prefixes");
      } else if (encoding == kLiteralEncodingNone) {
        // Before C++23 an ordinary literal adjacent to a prefixed literal
        // adopts the prefixed literal's encoding.
        encoding = next_encoding;
      }
    }
    StringAppendSegment(contents, lex->spelling.value, lex->spelling.length);
    if (lex->ud_suffix.length != 0) {
      user_defined = true;
      StringSetString(&suffix, &lex->ud_suffix);
    }
    LexNextToken(lex);
  }
  if (user_defined) {
    Vector* actuals = NewVector();
    int element_size =
        encoding == kLiteralEncodingUTF16
            ? 2
            : encoding == kLiteralEncodingUTF32 ? 4 : 1;
    size_t length = contents->length / element_size;
    VectorAppend(actuals,
                 NewStringLiteralArgument(contents, location, encoding));
    VectorAppend(actuals,
                 NewIntConstantASTNode((int64_t)length, NewSizeTypeRecord(),
                                       location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  StringDestruct(&suffix);

  int element_size =
      encoding == kLiteralEncodingUTF16
          ? 2
          : encoding == kLiteralEncodingUTF32 ? 4 : 1;
  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)(contents->length / element_size) + 1, false);
  // In C++ a narrow string literal has type `const char[N]`; in C it is a
  // non-const `char[N]` (modifying it is undefined behavior, but the type is
  // not const-qualified).
  Type element_type =
      encoding == kLiteralEncodingUTF16
          ? kTypeChar16
          : encoding == kLiteralEncodingUTF32
                ? kTypeChar32
                : encoding == kLiteralEncodingUTF8
                      ? UTF8LiteralElementType()
                      : kTypeChar;
  TypeRecord* type = NewTypeRecordWithSize(
      element_type, CompilerIsCXX() ? kQualConst : kQualPlain);
  TypeRecordChain(array, type);
  TypeRecordCalculateSize(array);
  return element_size > 1
             ? NewWideStringConstantASTNode(contents, array, location)
             : NewStringConstantASTNode(contents, array, location);
}


// Concatenates adjacent wide string tokens and handles a user-defined string
// suffix.
ASTNode* ParseWideStringLiteral(Syntax* syntax,
                                               TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation location = lex->current_token_location;
  bool user_defined = lex->ud_suffix.length != 0;
  String suffix;
  StringInit(&suffix, NULL);
  if (user_defined) {
    StringSetString(&suffix, &lex->ud_suffix);
  }
  String* contents =
      NewStringWithLength(lex->spelling.value, lex->spelling.length);
  LexNextToken(lex);

  // Adjacent wide string literals are joined together.
  while (LexLookingAt(lex, TOK(string_wide))) {
    StringAppendSegment(contents, lex->spelling.value, lex->spelling.length);
    if (lex->ud_suffix.length != 0) {
      user_defined = true;
      StringSetString(&suffix, &lex->ud_suffix);
    }
    LexNextToken(lex);
  }
  if (user_defined) {
    Vector* actuals = NewVector();
    size_t length = contents->length / compiler->wchar_size;
    VectorAppend(actuals, NewStringLiteralArgument(
                              contents, location, kLiteralEncodingWide));
    VectorAppend(actuals,
                 NewIntConstantASTNode((int64_t)length, NewSizeTypeRecord(),
                                       location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  StringDestruct(&suffix);

  TypeRecord* array = NewBasicArrayTypeRecord(
      kQualPlain, (int)(contents->length / compiler->wchar_size) + 1, false);
  TypeRecord* type = NewTypeRecordWithSize(
      WideLiteralElementType(), CompilerIsCXX() ? kQualConst : kQualPlain);
  TypeRecordChain(array, type);
  TypeRecordCalculateSize(array);
  return NewWideStringConstantASTNode(contents, array,
                                 syntax->lex->current_token_location);
}


// Builds a typed character constant or its user-defined literal call.
ASTNode* ParseCharacterConstant(Syntax* syntax,
                                                   TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  SourceLocation location = lex->current_token_location;
  LiteralEncoding encoding = lex->literal_encoding;
  Type literal_type =
      encoding == kLiteralEncodingUTF16
          ? kTypeChar16
          : encoding == kLiteralEncodingUTF32
                ? kTypeChar32
                : encoding == kLiteralEncodingUTF8
                      ? UTF8LiteralElementType()
                      : kTypeChar;
  if (lex->ud_suffix.length != 0) {
    String suffix;
    StringInit(&suffix, NULL);
    StringSetString(&suffix, &lex->ud_suffix);
    LexNextToken(lex);
    Vector* actuals = NewVector();
    TypeRecord* type = NewTypeRecordWithSize(literal_type, kQualPlain);
    VectorAppend(actuals, NewCharConstantASTNode(value, type, location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecordWithSize(literal_type, kQualPlain);
  return NewCharConstantASTNode(value, type, location);
}


// Builds a typed wide-character constant or its user-defined literal call.
ASTNode* ParseWideCharacterConstant(Syntax* syntax,
                                       TokenClass followers) {
  Lex* lex = syntax->lex;
  int value = (int)lex->number;
  if (lex->ud_suffix.length != 0) {
    SourceLocation location = lex->current_token_location;
    String suffix;
    StringInit(&suffix, NULL);
    StringSetString(&suffix, &lex->ud_suffix);
    LexNextToken(lex);
    Vector* actuals = NewVector();
    TypeRecord* type =
        NewTypeRecordWithSize(WideLiteralElementType(), kQualPlain);
    VectorAppend(actuals, NewCharConstantASTNode(value, type, location));
    ASTNode* call =
        NewCXXUserDefinedLiteralCall(syntax, &suffix, actuals, location);
    StringDestruct(&suffix);
    return call;
  }
  LexNextToken(lex);
  TypeRecord* type =
      NewTypeRecordWithSize(WideLiteralElementType(), kQualPlain);
  return NewCharConstantASTNode(value, type,
                                syntax->lex->current_token_location);
}
