//
//  expr_semantics_format.c
//  c_compiler
//
//  Printf/scanf format checking and call specialization.
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

// The class of argument a printf/scanf conversion expects.
typedef enum {
  kFmtNone,
  kFmtInvalid,
  kFmtInteger,
  kFmtDouble,
  kFmtString,
  kFmtPointer,
} FmtClass;

// Describes which optimized printf-family helper can handle a format string.
typedef enum {
  kPrintfProfileFull,
  kPrintfProfileLiteral,
  kPrintfProfileSimple,
  kPrintfProfileInt,
  kPrintfProfileLong,
  kPrintfProfileFP,
} PrintfProfile;

typedef enum {
  kFormatParseContinue,
  kFormatParseStopCall,
  kFormatParseStopLoop,
} FormatParseOutcome;

typedef struct {
  bool is_scanf;
  size_t arg_index;
} FormatCallScanState;

static bool IsValidFormatConversion(char conv, bool is_scanf);
static FmtClass FormatConversionClass(char conv, bool is_scanf);
static void CheckFormatArg(ASTNode* call, ASTNode* arg, FmtClass cls, int arg_number);
static int PrintfFormatArgument(const char* name);
static PrintfProfile ClassifyPrintfFormat(const char* format);
static Symbol* GetPrintfSpecializationSymbol(Symbol* original, const char* name);
static bool TryParseFormatAttributePositions(Attribute* fmt, const char** archetype_out,
                                             long* fmt_pos_out, long* first_pos_out,
                                             bool* is_scanf_out, bool* is_printf_out);
static bool FormatCallHasCheckableArguments(VectorASTNode* node, long fmt_pos,
                                            long first_pos, size_t* arg_index_out);
static const char* FormatLiteralStringOrWarn(VectorASTNode* node, ASTNode* fmt_arg,
                                             size_t arg_index, size_t nargs);
static void WarnZeroLengthFormatString(VectorASTNode* node, const char* format_start);
static void ParsePrintfDynamicWidth(VectorASTNode* node, FormatCallScanState* state,
                                    size_t nargs, int* conversions);
static void ParsePrintfDynamicPrecision(VectorASTNode* node, FormatCallScanState* state,
                                        const char** p, size_t nargs, int* conversions);
static void ParseFormatLengthModifiers(VectorASTNode* node, const char** p, bool is_scanf);
static FormatParseOutcome ParseOneFormatConversion(VectorASTNode* node,
                                                   FormatCallScanState* state,
                                                   const char** p, size_t nargs,
                                                   int* conversions);
static void ValidateFormatArgumentCount(VectorASTNode* node, size_t arg_index,
                                        size_t nargs);

// Contract: Read-only format conversion character validation.
static bool IsValidFormatConversion(char conv, bool is_scanf) {
  if (conv == '\0') {
    return false;
  }
  if (is_scanf) {
    return strchr("dioubxXaAeEfFgGsScCpn[%", conv) != NULL;
  }
  return strchr("dioubBxXcCaAeEfFgGsSpn%", conv) != NULL;
}

// Contract: Maps conversion specifier to coarse argument class; no side effects.
static FmtClass FormatConversionClass(char conv, bool is_scanf) {
  if (!IsValidFormatConversion(conv, is_scanf)) {
    return kFmtInvalid;
  }
  // For scanf every conversion takes a pointer to the destination.
  if (is_scanf) {
    return (conv == '%') ? kFmtNone : kFmtPointer;
  }
  switch (conv) {
    case 'd': case 'i': case 'u': case 'o': case 'b': case 'B':
    case 'x': case 'X': case 'c':
      return kFmtInteger;
    case 'f': case 'F': case 'e': case 'E':
    case 'g': case 'G': case 'a': case 'A':
      return kFmtDouble;
    case 's':
      return kFmtString;
    case 'p': case 'n':
      return kFmtPointer;
    default:
      return kFmtNone;
  }
}

// Checks one variadic argument against the class a conversion expects, emitting
// a (lenient) -Wformat warning only on clear mismatches.
// Contract: Warn-only; uses analyzed `arg->type`, does not mutate AST.
static void CheckFormatArg(ASTNode* call, ASTNode* arg, FmtClass cls,
                           int arg_number) {
  if (arg == NULL || arg->type == NULL || cls == kFmtNone) {
    return;
  }
  TypeRecord* t = arg->type;
  bool ok = true;
  const char* expected = NULL;
  switch (cls) {
    case kFmtInteger:
      // Accept any integer/enum; flag floating point and pointers.
      ok = TypeIsIntegral(t) || TypeIsEnum(t) || TypeIsBool(t);
      expected = "integer";
      break;
    case kFmtDouble:
      ok = TypeIsFloatingPoint(t);
      expected = "floating-point";
      break;
    case kFmtString:
      ok = TypeIsPointerOrArray(t);
      expected = "string (char *)";
      break;
    case kFmtPointer:
      ok = TypeIsPointerOrArray(t);
      expected = "pointer";
      break;
    case kFmtInvalid:
    case kFmtNone:
      return;
  }
  if (!ok) {
    SemanticWarning(call, "format",
                    "format argument %d has the wrong type (expected %s)",
                    arg_number, expected);
  }
}

// Contract: Read-only parse of format(archetype, fmt_index, first_vararg) attribute;
// returns false when the attribute is missing required integer args or archetype.
static bool TryParseFormatAttributePositions(Attribute* fmt, const char** archetype_out,
                                             long* fmt_pos_out, long* first_pos_out,
                                             bool* is_scanf_out, bool* is_printf_out) {
  const char* archetype = AttributeArgString(fmt, 0);
  long fmt_pos = 0;
  long first_pos = 0;
  if (archetype == NULL || !AttributeArgInt(fmt, 1, &fmt_pos) ||
      !AttributeArgInt(fmt, 2, &first_pos)) {
    return false;
  }
  *archetype_out = archetype;
  *fmt_pos_out = fmt_pos;
  *first_pos_out = first_pos;
  *is_scanf_out = strstr(archetype, "scanf") != NULL;
  *is_printf_out = strstr(archetype, "printf") != NULL;
  return true;
}

// Contract: Validates fmt/first indices against `node->children`; sets 0-based
// `*arg_index_out` for the first variadic argument. Returns false to skip checking.
static bool FormatCallHasCheckableArguments(VectorASTNode* node, long fmt_pos,
                                            long first_pos, size_t* arg_index_out) {
  if (first_pos == 0) {
    return false;
  }
  size_t nargs = node->children->length;
  if (fmt_pos < 1 || (size_t)fmt_pos > nargs) {
    return false;
  }
  *arg_index_out = (size_t)first_pos - 1;
  return true;
}

// Contract: Requires a string-literal format argument; emits non-literal warnings
// and returns NULL. Otherwise returns pointer into literal bytes (never NULL).
static const char* FormatLiteralStringOrWarn(VectorASTNode* node, ASTNode* fmt_arg,
                                             size_t arg_index, size_t nargs) {
  if (fmt_arg == NULL || fmt_arg->op != AST_OP(string)) {
    SemanticWarning((ASTNode*)node, "format-nonliteral",
                    "format string is not a string literal");
    if (arg_index >= nargs) {
      SemanticWarning((ASTNode*)node, "format-security",
                      "format string is not a string literal and has no format arguments");
    }
    return NULL;
  }
  String* format = ((ConstantASTNode*)fmt_arg)->value.string;
  return (format->value != NULL) ? format->value : "";
}

// Contract: Warn-only; flags empty format literals before conversion scanning.
static void WarnZeroLengthFormatString(VectorASTNode* node, const char* format_start) {
  if (*format_start == '\0') {
    SemanticWarning((ASTNode*)node, "format-zero-length",
                    "zero-length format string");
  }
}

// Contract: For printf, consumes a dynamic width (`*`) argument when present at `*p`.
static void ParsePrintfDynamicWidth(VectorASTNode* node, FormatCallScanState* state,
                                    size_t nargs, int* conversions) {
  if (state->is_scanf) {
    return;
  }
  CheckFormatArg((ASTNode*)node,
                 state->arg_index < nargs
                     ? (ASTNode*)node->children->value.p[state->arg_index]
                     : NULL,
                 kFmtInteger, (int)state->arg_index + 1);
  state->arg_index++;
  (*conversions)++;
}

// Contract: For printf, consumes a dynamic precision (`.*`) argument; advances `*p`
// past `*` when taken (caller already consumed '.').
static void ParsePrintfDynamicPrecision(VectorASTNode* node, FormatCallScanState* state,
                                        const char** p, size_t nargs, int* conversions) {
  if (**p != '*') {
    while (isdigit((unsigned char)**p)) {
      (*p)++;
    }
    return;
  }
  if (!state->is_scanf) {
    CheckFormatArg((ASTNode*)node,
                   state->arg_index < nargs
                       ? (ASTNode*)node->children->value.p[state->arg_index]
                       : NULL,
                   kFmtInteger, (int)state->arg_index + 1);
    state->arg_index++;
    (*conversions)++;
  }
  (*p)++;
}

// Contract: Advances `*p` over length modifiers; may emit width-specific warnings.
static void ParseFormatLengthModifiers(VectorASTNode* node, const char** p,
                                       bool is_scanf) {
  (void)is_scanf;
  if (**p == 'w') {
    (*p)++;
    if (**p == 'f') {
      (*p)++;
    }
    bool valid_width = **p >= '1' && **p <= '9';
    int width = 0;
    while (isdigit((unsigned char)**p)) {
      if (width <= 64) {
        width = width * 10 + **p - '0';
        if (width > 64) {
          width = 65;
        }
      }
      (*p)++;
    }
    if (!valid_width ||
        (width != 8 && width != 16 && width != 32 && width != 64)) {
      SemanticWarning((ASTNode*)node, "format-invalid-specifier",
                      "unsupported width-specific format modifier");
    }
  } else {
    while (**p == 'h' || **p == 'l' || **p == 'L' || **p == 'j' ||
           **p == 'z' || **p == 't') {
      (*p)++;
    }
  }
}

// Contract: Parses one conversion after `%`; updates `state->arg_index` and
// `*conversions`. Returns kFormatParseStopCall on too-few-args (caller exits),
// kFormatParseStopLoop on incomplete specifier, kFormatParseContinue otherwise.
static FormatParseOutcome ParseOneFormatConversion(VectorASTNode* node,
                                                   FormatCallScanState* state,
                                                   const char** p, size_t nargs,
                                                   int* conversions) {
  bool suppress = false;
  while (**p == '-' || **p == '+' || **p == ' ' || **p == '#' || **p == '0') {
    (*p)++;
  }
  if (state->is_scanf && **p == '*') {
    suppress = true;
    (*p)++;
  }
  if (**p == '*') {
    ParsePrintfDynamicWidth(node, state, nargs, conversions);
    (*p)++;
  } else {
    while (isdigit((unsigned char)**p)) {
      (*p)++;
    }
  }
  if (**p == '.') {
    (*p)++;
    ParsePrintfDynamicPrecision(node, state, p, nargs, conversions);
  }
  ParseFormatLengthModifiers(node, p, state->is_scanf);
  if (**p == '\0') {
    SemanticWarning((ASTNode*)node, "format-invalid-specifier",
                    "incomplete format specifier");
    return kFormatParseStopLoop;
  }
  char conv = **p;
  (*p)++;
  FmtClass cls = FormatConversionClass(conv, state->is_scanf);
  if (cls == kFmtInvalid) {
    SemanticWarning((ASTNode*)node, "format-invalid-specifier",
                    "invalid conversion specifier '%c' in format string", conv);
    return kFormatParseContinue;
  }
  if (cls == kFmtNone || suppress) {
    return kFormatParseContinue;
  }
  (*conversions)++;
  if (state->arg_index >= nargs) {
    SemanticWarning((ASTNode*)node, "format",
                    "too few arguments for format string");
    return kFormatParseStopCall;
  }
  CheckFormatArg((ASTNode*)node,
                 (ASTNode*)node->children->value.p[state->arg_index], cls,
                 (int)state->arg_index + 1);
  state->arg_index++;
  return kFormatParseContinue;
}

// Contract: Warn-only trailing-argument check after the format scan completes.
static void ValidateFormatArgumentCount(VectorASTNode* node, size_t arg_index,
                                        size_t nargs) {
  if (arg_index < nargs) {
    SemanticWarning((ASTNode*)node, "format",
                    "too many arguments for format string");
  }
}

// If the callee carries a format(printf/scanf, fmt, first) attribute and the
// format argument is a string literal, validate the variadic arguments against
// the conversions in the format string (count and rough types).
// Contract: Requires literal format string; read-only on AST aside from warnings.
void CheckFormatCall(VectorASTNode* node, Symbol* callee) {
  Attribute* fmt = SymbolFindAttribute(callee, "format");
  if (fmt == NULL) {
    return;
  }
  const char* archetype = NULL;
  long fmt_pos = 0;
  long first_pos = 0;
  bool is_scanf = false;
  bool is_printf = false;
  if (!TryParseFormatAttributePositions(fmt, &archetype, &fmt_pos, &first_pos,
                                        &is_scanf, &is_printf)) {
    return;
  }
  if (!is_scanf && !is_printf) {
    return;  // Unsupported archetype (e.g. strftime).
  }
  size_t arg_index = 0;
  if (!FormatCallHasCheckableArguments(node, fmt_pos, first_pos, &arg_index)) {
    return;
  }

  size_t nargs = node->children->length;
  ASTNode* fmt_arg = (ASTNode*)node->children->value.p[fmt_pos - 1];
  const char* p =
      FormatLiteralStringOrWarn(node, fmt_arg, arg_index, nargs);
  if (p == NULL) {
    return;
  }
  WarnZeroLengthFormatString(node, p);

  FormatCallScanState state = {
      .is_scanf = is_scanf,
      .arg_index = arg_index,
  };
  (void)is_printf;
  int conversions = 0;
  while (*p != '\0') {
    if (*p != '%') {
      p++;
      continue;
    }
    p++;
    if (*p == '%') {
      p++;
      continue;
    }
    FormatParseOutcome outcome =
        ParseOneFormatConversion(node, &state, &p, nargs, &conversions);
    if (outcome == kFormatParseStopCall) {
      return;
    }
    if (outcome == kFormatParseStopLoop) {
      break;
    }
  }
  ValidateFormatArgumentCount(node, state.arg_index, nargs);
  (void)conversions;
}

// Contract: Maps printf family name to format-string argument index; -1 if unknown.
static int PrintfFormatArgument(const char* name) {
  if (strcmp(name, "printf") == 0) {
    return 0;
  }
  if (strcmp(name, "fprintf") == 0 || strcmp(name, "sprintf") == 0) {
    return 1;
  }
  if (strcmp(name, "snprintf") == 0) {
    return 2;
  }
  return -1;
}

// Contract: Read-only scan of format literal for specialization profile.
static PrintfProfile ClassifyPrintfFormat(const char* format) {
  PrintfProfile profile = kPrintfProfileLiteral;
  // Every conversion is a bare %d, %i, %u, %c, %s or %%.
  bool simple = true;
  const char* p = format;
  while (*p != '\0') {
    if (*p++ != '%') {
      continue;
    }
    if (profile == kPrintfProfileLiteral) {
      profile = kPrintfProfileInt;
    }
    if (*p == '%') {
      p++;
      continue;
    }
    const char* spec = p;
    while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') {
      p++;
    }
    if (*p == '*') {
      p++;
    } else {
      while (isdigit((unsigned char)*p)) {
        p++;
      }
    }
    if (*p == '.') {
      p++;
      if (*p == '*') {
        p++;
      } else {
        while (isdigit((unsigned char)*p)) {
          p++;
        }
      }
    }
    bool long_value = false;
    if (*p == 'w') {
      return kPrintfProfileFull;
    } else if (*p == 'l') {
      long_value = true;
      p++;
      if (*p == 'l') {
        p++;
      }
    } else if (*p == 'L' || *p == 'j' || *p == 'z' || *p == 't') {
      long_value = true;
      p++;
    } else if (*p == 'h') {
      p++;
      if (*p == 'h') {
        p++;
      }
    }
    if (*p == '\0') {
      return kPrintfProfileFull;
    }
    if (p != spec || strchr("diucs", *p) == NULL) {
      simple = false;
    }
    char conversion = *p++;
    if (strchr("fFeEgGaA", conversion) != NULL) {
      profile = kPrintfProfileFP;
    } else if (long_value && profile != kPrintfProfileFP) {
      profile = kPrintfProfileLong;
    } else if (strchr("diuoxXpcsn", conversion) == NULL) {
      return kPrintfProfileFull;
    }
  }
  if (profile == kPrintfProfileInt && simple) {
    return kPrintfProfileSimple;
  }
  return profile;
}

// Contract: Finds or inserts global specialization stub symbol; may mutate symbol table.
static Symbol* GetPrintfSpecializationSymbol(Symbol* original,
                                             const char* name) {
  String symbol_name;
  StringInit(&symbol_name, name);
  Symbol* symbol = FindGlobalSymbol(&symbol_name);
  StringDestruct(&symbol_name);
  if (symbol != NULL) {
    return symbol;
  }
  symbol = NewSymbol(name, original->type, original->storage);
  symbol->flags.is_forward_declared = true;
  symbol->flags.is_c_linkage = true;
  symbol->flags.used = true;
  bool inserted = InsertGlobalSymbol(symbol);
  assert(inserted);
  (void)inserted;
  return symbol;
}

// Contract: When the format literal matches a known profile, mutates
// `node->left` identifier `symbol` to a synthesized `__name_*` helper; does
// not rewrite call arguments. No-op when specialization is disabled or format is
// not a string literal.
void SpecializePrintfCall(VectorASTNode* node, Symbol* callee) {
  if (!compiler->printf_specialize || callee == NULL ||
      callee->name.value == NULL) {
    return;
  }
  int format_arg = PrintfFormatArgument(callee->name.value);
  if (format_arg < 0 || (size_t)format_arg >= node->children->length) {
    return;
  }
  ASTNode* arg = node->children->value.p[format_arg];
  if (arg == NULL || arg->op != AST_OP(string)) {
    return;
  }
  String* value = ((ConstantASTNode*)arg)->value.string;
  PrintfProfile profile =
      ClassifyPrintfFormat(value->value == NULL ? "" : value->value);
  const char* suffix = NULL;
  switch (profile) {
    case kPrintfProfileLiteral:
      suffix = "literal";
      break;
    case kPrintfProfileSimple:
      suffix = "simple";
      break;
    case kPrintfProfileInt:
      suffix = "int";
      break;
    case kPrintfProfileLong:
      suffix = "long";
      break;
    case kPrintfProfileFP:
      suffix = "fp";
      break;
    case kPrintfProfileFull:
      return;
  }
  char specialized_name[64];
  snprintf(specialized_name, sizeof(specialized_name), "__%s_%s",
           callee->name.value, suffix);
  IdentifierASTNode* identifier = (IdentifierASTNode*)node->left;
  identifier->symbol =
      GetPrintfSpecializationSymbol(callee, specialized_name);
}
