//
//  syntax_attribute.c
//  c_compiler
//
//  Attributes: GNU __attribute__, __declspec, C++11 [[...]] and alignas.
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

// Appends the [start,end) text (trimmed of surrounding whitespace) to the
// attribute's argument list.  Empty arguments are ignored, so "foo()" yields
// no arguments.
static void AttributeAppendArg(Attribute* attr, const char* start,
                               const char* end) {
  while (start < end && isspace((unsigned char)*start)) {
    start++;
  }
  while (end > start && isspace((unsigned char)end[-1])) {
    end--;
  }
  if (end > start) {
    AttributeAddArg(attr, start, (size_t)(end - start));
  }
}

// Parses the raw text captured between the __attribute__ parentheses into a
// list of Attribute clauses.  Each clause is an identifier optionally followed
// by a balanced (...) argument list; clauses are separated by top-level commas.
// Inside the parentheses, arguments are split on top-level commas.  This
// correctly handles multi-argument attributes such as format(printf, 1, 2)
// and aligned(16) (a naive comma split would mangle them).
static void ParseAttributeText(String* text, Vector* attrs,
                               SourceLocation location, Source* source,
                               int lineno, int source_offset) {
  const char* p = (text->value != NULL) ? text->value : "";
  const char* text_start = p;
  while (*p != '\0') {
    while (*p != '\0' && (isspace((unsigned char)*p) || *p == ',')) {
      p++;
    }
    if (*p == '\0') {
      break;
    }
    const char* name_start = p;
    while (*p != '\0' && (isalnum((unsigned char)*p) || *p == '_')) {
      p++;
    }
    if (p == name_start) {
      // Not an identifier (stray punctuation); skip a char to make progress.
      p++;
      continue;
    }
    String* name = NewStringWithLength(name_start, (size_t)(p - name_start));
    Attribute* attr = NewAttribute(name->value);
    if (source != NULL && source_offset >= 0) {
      size_t offset = (size_t)source_offset +
                      (size_t)(name_start - text_start);
      attr->location =
          NewSourceLocation(source, lineno, offset, offset + name->length);
    } else {
      attr->location = location;
    }
    StringDelete(name);

    while (*p != '\0' && isspace((unsigned char)*p)) {
      p++;
    }
    if (*p == '(') {
      p++;  // Consume '('.
      int depth = 1;
      const char* arg_start = p;
      while (*p != '\0' && depth > 0) {
        char c = *p;
        if (c == '(') {
          depth++;
        } else if (c == ')') {
          depth--;
          if (depth == 0) {
            AttributeAppendArg(attr, arg_start, p);
            p++;  // Consume ')'.
            break;
          }
        } else if (c == ',' && depth == 1) {
          AttributeAppendArg(attr, arg_start, p);
          arg_start = p + 1;
        }
        p++;
      }
    }
    VectorAppend(attrs, attr);
  }
}

// Attributes the compiler either acts on or knowingly accepts and ignores.
// Anything not in this list triggers a (default-off) -Wattributes warning,
// matching GCC's "attribute directive ignored" diagnostic.
static bool IsKnownAttribute(const char* name) {
  static const char* known[] = {
    // Acted upon by davecc.
    "packed", "aligned", "format", "deprecated", "unused",
    "warn_unused_result", "noreturn", "noinline", "always_inline",
    "constructor", "destructor", "meta_intrinsic", "indeterminate",
    // Accepted but not modelled (parsed cleanly, no effect).
    "stdcall", "cdecl", "fastcall", "thiscall", "regparm", "ms_abi",
    "sysv_abi", "may_alias", "gnu_inline", "nothrow", "leaf", "cold", "hot",
    "malloc", "pure", "const", "nonnull", "returns_nonnull", "sentinel",
    "weak", "alias", "section", "visibility", "used",
    "transparent_union", "vector_size", "ext_vector_type", "mode",
    "no_instrument_function", "cleanup", "returns_twice", "artificial",
    "designated_init", "fallthrough", "warning", "error", "alloc_size",
    "format_arg", "nonstring", "noclone", "noipa", "flatten", "naked",
    "weakref", "dllimport", "dllexport", "common", "nocommon", "tls_model",
    "selectany", "novtable", "uuid", "property", "allocate", "noalias",
    "aligned_alloc", "assume_aligned", "likely", "unlikely",
    "no_unique_address", "reproducible", "unsequenced",
  };
  for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
    if (strcmp(known[i], name) == 0) {
      return true;
    }
  }
  return false;
}

// Returns true when `name` is an attribute DaveCC applies to declarations,
// not merely parses and ignores.
bool SyntaxAttributeIsSupported(const char* name) {
  static const char* supported[] = {
      "packed",          "aligned",          "format",
      "deprecated",      "unused",           "warn_unused_result",
      "noreturn",        "noinline",         "always_inline",
      "constructor",     "destructor",        "indeterminate",
      "vector_size",     "ext_vector_type",
  };
  for (size_t i = 0; i < sizeof(supported) / sizeof(supported[0]); i++) {
    if (strcmp(supported[i], name) == 0) {
      return true;
    }
  }
  return false;
}

// Returns true for a non-empty C++ attribute namespace other than gnu or std.
static bool IsUserDefinedAttributeNamespace(const char* namespace_name) {
  return namespace_name != NULL && namespace_name[0] != '\0' &&
         strcmp(namespace_name, "gnu") != 0 &&
         strcmp(namespace_name, "std") != 0;
}

// Returns true when reflection metadata may record `attr` (standard
// attributes, known GNU names, or user-defined namespaces).
bool SyntaxAttributeIsReflectable(const Attribute* attr) {
  if (attr == NULL) {
    return false;
  }
  const char* token = AttributeIdentifier(attr);
  const char* namespace_name = attr->attribute_namespace.value;
  if (token == NULL || strcmp(token, "assume") == 0) {
    return false;
  }
  if (namespace_name == NULL || attr->attribute_namespace.length == 0) {
    static const char* standard[] = {
        "carries_dependency", "deprecated",        "fallthrough",
        "indeterminate",      "likely",            "maybe_unused",
        "nodiscard",          "noreturn",          "no_unique_address",
        "reproducible",       "unlikely",          "unsequenced",
    };
    for (size_t i = 0; i < sizeof(standard) / sizeof(standard[0]); i++) {
      if (strcmp(token, standard[i]) == 0) {
        return true;
      }
    }
    return false;
  }
  // P3385 permits implementation-defined support for vendor attributes.
  // DaveCC reflects the GNU attributes its parser already understands and
  // arbitrary namespaced user attributes as identity-only metadata.
  if (strcmp(namespace_name, "gnu") == 0) {
    return IsKnownAttribute(attr->name.value);
  }
  return IsUserDefinedAttributeNamespace(namespace_name);
}

// Returns true for attributes that may appear on types but are not implemented.
static bool IsUnsupportedTypeAttribute(const char* name) {
  return strcmp(name, "mode") == 0;
}

// Parse an __attribute__((...)) clause list, appending parsed Attribute* to
// attrs.  Assumes TOK(attribute) was just consumed.
void SyntaxParseAttribute(Syntax* syntax, Vector* attrs) {
  SourceLocation location = syntax->lex->current_token_location;
  int lineno = syntax->lex->source->lineno;
  int location_start = 0;
  int ignored_end = 0;
  int ignored_line = 0;
  const char* ignored_filename = NULL;
  DecodeSourceLocation(location, &ignored_filename, &ignored_line,
                       &location_start, &ignored_end);
  String attribute_list = {0};
  LexReadAttributes(syntax->lex, &attribute_list);
  size_t attributes_start = attrs->length;
  int source_offset =
      syntax->lex->source->lineno == lineno ? location_start + 2 : -1;
  ParseAttributeText(&attribute_list, attrs, location, syntax->lex->source,
                     lineno, source_offset);
  for (size_t i = attributes_start; i < attrs->length; i++) {
    Attribute* attr = attrs->value.p[i];
    if (IsUnsupportedTypeAttribute(attr->name.value)) {
      SyntaxError(syntax, "'%s' type attribute is not supported",
                  attr->name.value);
    } else if (!IsKnownAttribute(attr->name.value)) {
      SyntaxWarning(syntax, "attributes", "'%s' attribute directive ignored",
                    attr->name.value);
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), 0);
  StringDestruct(&attribute_list);
}

// Returns true when the next tokens start a C++11 [[...]] attribute sequence
// (double left square bracket), without consuming input.
bool SyntaxLookingAtCXXAttribute(Syntax* syntax) {
  if ((!CompilerIsCXX() && !CompilerCAtLeast(kLanguageStandardC23)) ||
      !LexLookingAt(syntax->lex, TOK(lsquare))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = LexLookingAt(syntax->lex, TOK(lsquare));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// Appends a spaced token representation of the lexer's current token to `arg`
// for reconstructing attribute argument text.
static void AppendCXXAttributeArgToken(Lex* lex, String* arg) {
  if (arg->length > 0) {
    StringAppendChar(arg, ' ');
  }
  switch (lex->current_token) {
    case TOK(identifier):
    case TOK(string):
    case TOK(string_wide):
    case TOK(charconst):
    case TOK(charconst_wide):
      StringAppendString(arg, &lex->spelling);
      break;
    case TOK(number): {
      String buf;
      StringInit(&buf, NULL);
      StringPrintf(&buf, "%lld", (long long)lex->number);
      StringAppendString(arg, &buf);
      StringDestruct(&buf);
      break;
    }
    case TOK(fnumber): {
      String buf;
      StringInit(&buf, NULL);
      StringPrintf(&buf, "%g", lex->fnumber);
      StringAppendString(arg, &buf);
      StringDestruct(&buf);
      break;
    }
    default:
      StringAppend(arg, TokenName(lex->current_token));
      break;
  }
}

// Parse `__declspec(name)` or `__declspec(name(args))`.  MSVC allows one
// attribute per `__declspec`; stack them for several.  `align(N)` is accepted
// as GNU `aligned(N)` so existing layout code applies.
static void SyntaxParseDeclspec(Syntax* syntax, Vector* attrs) {
  SourceLocation location = syntax->lex->current_token_location;
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    SyntaxError(syntax, "Expected ( after __declspec");
    return;
  }
  String name = {0};
  if (!LexMatchIdentifier(syntax->lex, &name)) {
    SyntaxError(syntax, "Expected identifier in __declspec");
    int depth = 1;
    while (!LexEof(syntax->lex) && depth > 0) {
      if (LexMatch(syntax->lex, TOK(lparen))) {
        depth++;
      } else if (LexMatch(syntax->lex, TOK(rparen))) {
        depth--;
      } else {
        LexNextToken(syntax->lex);
      }
    }
    StringDestruct(&name);
    return;
  }
  const char* mapped = name.value;
  if (strcmp(mapped, "align") == 0) {
    mapped = "aligned";
  }
  Attribute* attr = NewAttribute(mapped);
  attr->location = location;
  if (LexMatch(syntax->lex, TOK(lparen))) {
    int depth = 1;
    String arg = {0};
    StringInit(&arg, NULL);
    while (!LexEof(syntax->lex) && depth > 0) {
      if (LexLookingAt(syntax->lex, TOK(rparen))) {
        depth--;
        if (depth == 0) {
          AttributeAppendArg(attr, arg.value,
                             arg.value != NULL ? arg.value + arg.length
                                               : arg.value);
          break;
        }
      }
      if (depth == 1 && LexLookingAt(syntax->lex, TOK(comma))) {
        AttributeAppendArg(attr, arg.value,
                           arg.value != NULL ? arg.value + arg.length
                                             : arg.value);
        StringDestruct(&arg);
        StringInit(&arg, NULL);
        LexNextToken(syntax->lex);
        continue;
      }
      if (LexLookingAt(syntax->lex, TOK(lparen))) {
        depth++;
      }
      AppendCXXAttributeArgToken(syntax->lex, &arg);
      LexNextToken(syntax->lex);
    }
    SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra) | TC(semicolon));
    StringDestruct(&arg);
  }
  VectorAppend(attrs, attr);
  if (IsUnsupportedTypeAttribute(attr->name.value)) {
    SyntaxError(syntax, "'%s' type attribute is not supported",
                attr->name.value);
  } else if (!IsKnownAttribute(attr->name.value)) {
    SyntaxWarning(syntax, "attributes", "'%s' attribute directive ignored",
                  attr->name.value);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra) | TC(semicolon));
  StringDestruct(&name);
}

// Returns true when the next token begins GNU __attribute__, __declspec, or
// C++ [[...]] attributes.
bool SyntaxLookingAtAnyAttribute(Syntax* syntax) {
  return LexLookingAt(syntax->lex, TOK(attribute)) ||
         LexLookingAt(syntax->lex, TOK(declspec)) ||
         SyntaxLookingAtCXXAttribute(syntax);
}

// Consumes one attribute specifier if present and appends parsed Attribute*
// entries to `attrs`. Returns true when an attribute was parsed.
bool SyntaxParseAnyAttribute(Syntax* syntax, Vector* attrs) {
  if (LexMatch(syntax->lex, TOK(attribute))) {
    SyntaxParseAttribute(syntax, attrs);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(declspec))) {
    SyntaxParseDeclspec(syntax, attrs);
    return true;
  }
  if (SyntaxLookingAtCXXAttribute(syntax)) {
    SyntaxParseCXXAttributes(syntax, attrs);
    return true;
  }
  return false;
}

// Parses optional (...) arguments after a C++ attribute name into `attr`,
// consuming tokens and emitting diagnostics for bad unevaluated-string args.
static void ParseCXXAttributeArguments(Syntax* syntax, Attribute* attr,
                                       bool unevaluated_string_argument) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return;
  }
  if (unevaluated_string_argument) {
    if (LexLookingAtStringLiteral(syntax->lex)) {
      LexValidateUnevaluatedString(syntax->lex, "attribute argument",
                                   /*allow_user_defined_suffix=*/false);
    } else {
      SyntaxError(syntax,
                  "attribute argument must be an unevaluated string");
    }
  }

  int depth = 1;
  String arg;
  StringInit(&arg, NULL);
  while (!LexEof(syntax->lex) && depth > 0) {
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      if (depth > 0) {
        AppendCXXAttributeArgToken(syntax->lex, &arg);
      }
      depth++;
      LexNextToken(syntax->lex);
    } else if (LexLookingAt(syntax->lex, TOK(rparen))) {
      depth--;
      if (depth == 0) {
        if (arg.length > 0) {
          AttributeAddArg(attr, arg.value, arg.length);
          StringClear(&arg);
        }
        LexNextToken(syntax->lex);
        break;
      }
      AppendCXXAttributeArgToken(syntax->lex, &arg);
      LexNextToken(syntax->lex);
    } else if (LexLookingAt(syntax->lex, TOK(comma)) && depth == 1) {
      AttributeAddArg(attr, arg.value != NULL ? arg.value : "", arg.length);
      StringClear(&arg);
      LexNextToken(syntax->lex);
    } else {
      AppendCXXAttributeArgToken(syntax->lex, &arg);
      LexNextToken(syntax->lex);
    }
  }
  if (depth != 0) {
    SyntaxError(syntax, "Unterminated attribute argument list");
  }
  StringDestruct(&arg);
}

// Consumes an identifier token into `out` when the lexer looks at one;
// returns false without advancing otherwise.
static bool ParseCXXAttributeIdentifier(Syntax* syntax, String* out) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  StringSetString(out, &syntax->lex->spelling);
  LexNextToken(syntax->lex);
  return true;
}

// Maps standard and GNU spellings of C++ attributes to the internal names
// used elsewhere in the compiler.
static const char* CanonicalCXXAttributeName(const char* ns,
                                             const char* name) {
  if (ns != NULL && strcmp(ns, "gnu") != 0) {
    return name;
  }
  if (strcmp(name, "maybe_unused") == 0) {
    return "unused";
  }
  if (strcmp(name, "nodiscard") == 0) {
    return "warn_unused_result";
  }
  if (strcmp(name, "__noreturn__") == 0) {
    return "noreturn";
  }
  return name;
}

// Parses one [[ns::name(args)]] clause (or unqualified name) and appends a
// new Attribute to `attrs`, using `using_namespace` from a prior using directive.
static void ParseCXXSingleAttribute(Syntax* syntax, Vector* attrs,
                                    const char* using_namespace) {
  SourceLocation location = syntax->lex->current_token_location;
  String first;
  String second;
  StringInit(&first, NULL);
  StringInit(&second, NULL);
  if (!ParseCXXAttributeIdentifier(syntax, &first)) {
    SyntaxError(syntax, "Expected attribute name");
    StringDestruct(&first);
    StringDestruct(&second);
    return;
  }

  const char* namespace_name = using_namespace;
  const char* attr_name = first.value;
  if (LexMatch(syntax->lex, TOK(coloncolon))) {
    namespace_name = first.value;
    if (!ParseCXXAttributeIdentifier(syntax, &second)) {
      SyntaxError(syntax, "Expected attribute name after ::");
      StringDestruct(&first);
      StringDestruct(&second);
      return;
    }
    attr_name = second.value;
  }

  Attribute* attr =
      NewAttribute(CanonicalCXXAttributeName(namespace_name, attr_name));
  attr->location = location;
  AttributeSetCXXIdentity(attr, namespace_name, attr_name);
  if (namespace_name == NULL && strcmp(attr_name, "deprecated") == 0 &&
      CompilerIsCXX() &&
      !CompilerCXXAtLeast(kLanguageStandardCXX14)) {
    SyntaxErrorAtLocation(syntax, attr->location,
                          "'deprecated' attribute requires C++14");
  }
  bool unevaluated_string_argument =
      namespace_name == NULL &&
      (strcmp(attr_name, "deprecated") == 0 ||
       strcmp(attr_name, "nodiscard") == 0);
  ParseCXXAttributeArguments(syntax, attr, unevaluated_string_argument);
  if (StringEqual(&attr->name, "aligned") && AttributeArgCount(attr) > 0) {
    long ignored = 0;
    if (!AttributeArgInt(attr, 0, &ignored)) {
      SyntaxError(syntax, "aligned attribute argument must be an integer");
    }
  }
  if (IsUnsupportedTypeAttribute(attr->name.value)) {
    SyntaxError(syntax, "'%s' type attribute is not supported",
                attr->name.value);
  } else if (!IsKnownAttribute(attr->name.value) &&
             !(CompilerCXXAtLeast(kLanguageStandardCXX29) &&
               IsUserDefinedAttributeNamespace(namespace_name))) {
    SyntaxWarning(syntax, "attributes", "'%s' attribute directive ignored",
                  attr->name.value);
  }
  VectorAppend(attrs, attr);
  StringDestruct(&first);
  StringDestruct(&second);
}

// Consumes one or more [[...]] blocks, including using namespaces and C++26
// annotation attributes, appending each clause to `attrs`. Returns true if any
// block was parsed.
bool SyntaxParseCXXAttributes(Syntax* syntax, Vector* attrs) {
  bool any = false;
  while (SyntaxLookingAtCXXAttribute(syntax)) {
    any = true;
    LexMatch(syntax->lex, TOK(lsquare));
    LexMatch(syntax->lex, TOK(lsquare));

    const char* using_namespace = NULL;
    String using_name;
    StringInit(&using_name, NULL);
    if (LexMatch(syntax->lex, TOK(using))) {
      if (ParseCXXAttributeIdentifier(syntax, &using_name)) {
        using_namespace = using_name.value;
      } else {
        SyntaxError(syntax, "Expected attribute namespace after using");
      }
      SyntaxNeedBracket(syntax, TOK(colon), TC(closebra));
    }

    while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
      if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
          LexLookingAt(syntax->lex, TOK(equal))) {
        LexNextToken(syntax->lex);
        Attribute* attr = NewAttribute("annotation");
        attr->annotation_expr =
            SyntaxParseExpression(syntax, TC(closebra));
        VectorAppend(attrs, attr);
      } else {
        ParseCXXSingleAttribute(syntax, attrs, using_namespace);
      }
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      if (LexLookingAt(syntax->lex, TOK(rsquare))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
    StringDestruct(&using_name);
  }
  return any;
}

// Appends an aligned(`alignment`) attribute to `attrs` when `alignment` is
// positive.
static void AppendAlignedAttribute(Vector* attrs, int alignment) {
  if (alignment <= 0) {
    return;
  }
  Attribute* attr = NewAttribute("aligned");
  String arg;
  StringInit(&arg, NULL);
  StringPrintf(&arg, "%d", alignment);
  AttributeAddArg(attr, arg.value, arg.length);
  StringDestruct(&arg);
  VectorAppend(attrs, attr);
}

// When alignas(...) is next, parses a type-id or constant expression (or a
// dependent form inside templates), validates power-of-two alignment, and
// appends aligned or dependent metadata to `attrs`. Returns false if alignas
// is not present.
bool SyntaxParseCXXAlignas(Syntax* syntax, Vector* attrs) {
  if ((!CompilerIsCXX() && !CompilerCAtLeast(kLanguageStandardC11)) ||
      !LexMatch(syntax->lex, TOK(alignas))) {
    return false;
  }

  SyntaxNeedBracket(syntax, TOK(lparen), TC(decl) | TC(closebra));
  int64_t value = 0;
  Attribute* dependent = NULL;
  if (SyntaxLookingAtType(syntax)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    bool expansion = parser.declarator_is_parameter_pack;
    if (sym == NULL || sym->type == NULL) {
      SyntaxError(syntax, "alignas type-id is invalid");
    } else if (TypeContainsTemplateParameter(sym->type)) {
      dependent = NewAttribute("aligned");
      dependent->dependent_alignas_type = TypeRecordCopy(sym->type);
      dependent->dependent_alignas_is_pack_expansion = expansion;
    } else if (expansion) {
      SyntaxError(syntax, "alignas pack expansion does not name a pack");
    } else {
      TypeRecordCalculateSize(sym->type);
      value = TypeRecordAlignment(sym->type);
    }
    SymbolDelete(sym);
    TypeParserDestruct(&parser);
  } else {
    ASTNode* expr = SyntaxParseExpression(syntax, TC(closebra));
    bool expansion = CompilerIsCXX() && LexMatch(syntax->lex, TOK(ellipsis));
    if (syntax->parsing_template_declaration &&
        ExpressionIsTemplateDependent(expr)) {
      dependent = NewAttribute("aligned");
      dependent->dependent_alignas_expr = expr;
      dependent->dependent_alignas_is_pack_expansion = expansion;
      expr = NULL;
    } else if (expansion) {
      SyntaxError(syntax, "alignas pack expansion does not name a pack");
    } else {
      expr = AnalyzeExpression(expr);
      bool ok = EvaluateIntegerExpression(expr, &value);
      if (!ok) {
        SyntaxError(syntax, "alignas specifier must be a constant expression");
      }
    }
    ASTNodeDelete(expr);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));

  if (dependent != NULL) {
    VectorAppend(attrs, dependent);
    return true;
  }
  if (value < 0 || value > 2147483647 ||
      !IsPowerOf2OrZero((int32_t)value)) {
    SyntaxError(syntax, "alignas specifier must name a power-of-two alignment");
    return true;
  }
  AppendAlignedAttribute(attrs, (int)value);
  return true;
}

// Interprets attributes that have just been attached to a declared symbol and
// affect the symbol/type directly (layout and function-behavior flags).
static void ApplyVectorTypeAttribute(Syntax* syntax, Symbol* sym) {
  Attribute* vector_size =
      AttributeListFind(&sym->attributes, "vector_size");
  Attribute* ext_vector =
      AttributeListFind(&sym->attributes, "ext_vector_type");
  Attribute* attr = vector_size != NULL ? vector_size : ext_vector;
  if (attr == NULL) {
    return;
  }
  if (vector_size != NULL && ext_vector != NULL) {
    SyntaxError(syntax,
                "'vector_size' and 'ext_vector_type' cannot be combined");
    return;
  }
  if (AttributeArgCount(attr) != 1) {
    SyntaxError(syntax, "'%s' attribute requires one argument",
                attr->name.value);
    return;
  }
  long argument = 0;
  if (!AttributeArgInt(attr, 0, &argument) || argument <= 0) {
    SyntaxError(syntax, "'%s' attribute argument must be a positive integer",
                attr->name.value);
    return;
  }
  TypeRecordCalculateSize(sym->type);
  TypeRecord* element = sym->type;
  if (TypeIsVector(element) || !TypeIsPrimitive(element) ||
      (!TypeIsIntegral(element) && !TypeIsFloatingPoint(element)) ||
      TypeIsBool(element) || TypeIsEnum(element) || TypeIsBitInt(element)) {
    SyntaxError(syntax,
                "'%s' attribute requires an integer or floating scalar type",
                attr->name.value);
    return;
  }
  long lanes = argument;
  if (vector_size != NULL) {
    if (element->size <= 0 || argument % element->size != 0) {
      SyntaxError(syntax,
                  "vector size must be a positive multiple of element size");
      return;
    }
    lanes = argument / element->size;
  }
  long bytes = lanes * element->size;
  if (lanes <= 0 || lanes > INT_MAX || bytes <= 0 ||
      (bytes & (bytes - 1)) != 0) {
    SyntaxError(syntax, "vector size must be a power of two");
    return;
  }
  SymbolSetType(sym, NewVectorTypeRecord(element, (int)lanes));
}

// Applies parsed attributes on `sym` to its type and flags: vector types,
// noreturn/inline/weak, indeterminate, typedef struct layout, and alignment.
void SyntaxApplyDeclarationAttributes(Syntax* syntax, Symbol* sym) {
  if (sym == NULL) {
    return;
  }

  ApplyVectorTypeAttribute(syntax, sym);

  // Function-behavior flags (also meaningful on forward declarations).
  if (AttributeListHas(&sym->attributes, "noreturn")) {
    if (!TypeIsFunction(sym->type) ||
        StorageIs(sym->storage, STO(typedef))) {
      SyntaxError(syntax, "'noreturn' attribute applies only to functions");
    }
    sym->flags.noreturn = true;
  }
  if (AttributeListHas(&sym->attributes, "always_inline")) {
    sym->flags.always_inline = true;
  }
  if (AttributeListHas(&sym->attributes, "noinline")) {
    sym->flags.noinline = true;
  }
  if (AttributeListHas(&sym->attributes, "weak")) {
    sym->flags.is_weak = true;
  }
  Attribute* indeterminate =
      AttributeListFind(&sym->attributes, "indeterminate");
  if (indeterminate != NULL) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
      SyntaxError(syntax, "'indeterminate' attribute requires C++26");
    }
    if (AttributeArgCount(indeterminate) != 0) {
      SyntaxError(syntax, "'indeterminate' attribute takes no arguments");
    }
    bool automatic_block_variable =
        sym->flags.is_block_scope &&
            (sym->storage == STO(implicit) ||
             StorageIs(sym->storage, STO(auto)) ||
         StorageIs(sym->storage, STO(register)));
    if (!sym->flags.is_argument && !automatic_block_variable) {
      SyntaxError(syntax,
                  "'indeterminate' attribute applies only to parameters and "
                  "automatic block variables");
    }
  }

  if (StorageIs(sym->storage, STO(typedef))) {
    // packed/aligned on a typedef applies to the (usually anonymous) struct or
    // union type it names.
    TypeApplyStructAttributesFromSymbol(sym);
    return;
  }
  // aligned(N) on a variable raises its alignment; the strictest of several
  // specifiers applies.
  int alignment = AttributeListAlignment(&sym->attributes, compiler->alignment);
  if (alignment > 0) {
    sym->alignment = alignment;
  }
}
