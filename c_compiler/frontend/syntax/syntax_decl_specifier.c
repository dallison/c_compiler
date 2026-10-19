//
//  syntax_decl_specifier.c
//  c_compiler
//
//  Declaration specifiers: storage classes, thread_local, explicit,
//  constexpr/consteval, and class template deduction guides.
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

// Returns true when `v` is zero or has exactly one bit set in its value.
bool IsPowerOf2OrZero(int32_t v) {
  return (v & (v - 1)) == 0;
}

// Returns the spelling of the thread-local keyword for diagnostics in the
// current language (`thread_local` in C++, `__thread` in C).
static const char* ThreadLocalDiagnosticKeyword(void) {
  return CompilerIsCXX() ? "thread_local" : "__thread";
}

// thread_local / __thread imply static storage duration at block scope only.
static void NormalizeThreadLocalStorage(Storage* storage,
                                      ParserContext context) {
  if (!StorageIs(*storage, STO(thread)) || !CompilerIsCXX()) {
    return;
  }
  if (context == kParsingBlockScope &&
      !StorageIs(*storage, STO(extern))) {
    *storage |= STO(static);
  }
}

// Validate thread-local storage for the current language and scope.
void SyntaxCheckThreadLocal(Syntax* syntax, Symbol* symbol,
                            ParserContext context, bool is_static_member,
                            bool is_nonstatic_member) {
  if (!StorageIs(symbol->storage, STO(thread))) {
    return;
  }

  const char* keyword = ThreadLocalDiagnosticKeyword();
  if (TypeIsFunction(symbol->type) || symbol->flags.is_argument) {
    SyntaxError(syntax, "Illegal use of %s", keyword);
    return;
  }
  if (is_nonstatic_member) {
    SyntaxError(syntax, "Non-static data member cannot be %s", keyword);
    return;
  }
  if (CompilerIsCXX()) {
    return;
  }

  // C: __thread at block scope requires an explicit static or extern storage
  // class.  File-scope behavior is left unchanged.
  if (context == kParsingBlockScope &&
      !StorageIs(symbol->storage, STO(static) | STO(extern))) {
    SyntaxError(syntax, "Illegal use of %s", keyword);
  }
}

// Parses a C++ `explicit` or `explicit(condition)` specifier if present,
// setting `*saw_explicit`.  Returns whether the function is explicit: true
// for plain `explicit`, else the condition's value.  A value-dependent
// condition yields true and is kept in `syntax->pending_explicit_condition`
// to be folded for each instantiation.
static bool ParseCXXExplicitDeclarationSpecifier(Syntax* syntax,
                                                 bool* saw_explicit) {
  *saw_explicit = false;
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(explicit))) {
    return false;
  }
  *saw_explicit = true;
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return true;
  }
  ASTNode* expr =
      SyntaxParseExpression(syntax, TC(closebra) | TC(exprsep));
  // A value-dependent condition (e.g. `explicit(sizeof(T) > 4)`) is detected on
  // the parsed (pre-analysis) tree, before constant-folding could collapse it
  // to the placeholder's size.  The unanalyzed condition is stashed on the
  // function's FunctionInfo and re-folded per instantiation (see
  // InstantiateMemberFunctionType); mirrors static_assert deferral.
  if (syntax->parsing_template_declaration &&
      ExpressionIsTemplateDependent(expr)) {
    SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));
    syntax->pending_explicit_condition = expr;
    return true;
  }
  expr = AnalyzeExpression(expr);
  int64_t value = 0;
  bool ok = EvaluateIntegerExpression(expr, &value);
  if (!ok) {
    SyntaxError(syntax, "explicit specifier must be a constant expression");
  }
  ASTNodeDelete(expr);
  SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));
  return ok && value != 0;
}

// Emits an error when `is_consteval` is set but `type` is not a function
// type (including when `type` is NULL).
void DiagnoseConstevalOnNonFunction(Syntax* syntax, TypeRecord* type,
                                    bool is_consteval) {
  if (is_consteval && (type == NULL || !TypeIsFunction(type))) {
    SyntaxError(syntax, "'consteval' can only be applied to functions");
  }
}

// Diagnoses a mismatch between `previous` and `current` function types in
// their constexpr versus consteval (immediate) specifier pairing.
void CheckMatchingConstexprConsteval(Syntax* syntax,
                                     TypeRecord* previous,
                                     TypeRecord* current) {
  if (!CompilerIsCXX() || previous == NULL || current == NULL ||
      !TypeIsFunction(previous) || !TypeIsFunction(current)) {
    return;
  }
  bool previous_immediate = previous->info.function.is_consteval;
  bool current_immediate = current->info.function.is_consteval;
  bool previous_constexpr =
      previous->info.function.is_constexpr && !previous_immediate;
  bool current_constexpr =
      current->info.function.is_constexpr && !current_immediate;
  if (previous_immediate != current_immediate ||
      previous_constexpr != current_constexpr) {
    SyntaxError(syntax,
                "'constexpr' and 'consteval' specifiers must match previous "
                "declaration");
  }
}

// Adds storage-class specifier `s` to `*storage`, warning about a duplicate
// and diagnosing an incompatible combination (`__thread` combines with the
// others, and so does C23 `auto`, but not with `typedef`).
static void AddStorageSpecifier(Syntax* syntax, Storage* storage, Storage s) {
  Storage new = s;
  Storage old = *storage;

  // Check for duplicate storage.
  if ((new & old) != 0) {
    SyntaxWarning(syntax, "duplicate-decl-specifier",
                  "Duplicate storage specifier");
  }

  // Remove __thread from mask and check for multiple bits set.
  Storage compatibility_mask = STO(thread);
  if (CompilerCAtLeast(kLanguageStandardC23) &&
      StorageIs(new | old, STO(auto))) {
    compatibility_mask |= STO(auto);
  }
  if (!IsPowerOf2OrZero((new | old) & ~compatibility_mask) ||
      (CompilerCAtLeast(kLanguageStandardC23) &&
       StorageIs(new | old, STO(auto)) &&
       StorageIs(new | old, STO(typedef)))) {
    SyntaxError(syntax,
                "Multiple incompatible storage specifiers");
  }
  *storage |= s;
}

// Parses a function specifier (`inline`, `_Noreturn` or C++ `explicit`) if
// one is at the current token, recording it in `*is_inline`, `attributes`
// or `*is_explicit`.  Returns true when one was consumed.
static bool ParseFunctionSpecifier(Syntax* syntax, Vector* attributes,
                                   bool* is_inline, bool* is_explicit) {
  if (LexMatch(syntax->lex, TOK(inline))) {
    if (*is_inline) {
      SyntaxWarning(syntax, "duplicate-decl-specifier",
                    "Duplicate 'inline' specifier");
    }
    *is_inline = true;
    return true;
  }
  if (LexMatch(syntax->lex, TOK(noreturn))) {
    if (AttributeListHas(attributes, "noreturn")) {
      SyntaxWarning(syntax, "duplicate-decl-specifier",
                    "Duplicate '_Noreturn' specifier");
    } else {
      VectorAppend(attributes, NewAttribute("noreturn"));
    }
    if (CompilerCAtLeast(kLanguageStandardC23)) {
      SyntaxWarning(syntax, "deprecated-declarations",
                    "'_Noreturn' is deprecated in C23; use [[noreturn]]");
    }
    return true;
  }
  if (LexLookingAt(syntax->lex, TOK(explicit))) {
    bool saw_explicit = false;
    bool explicit_value =
        ParseCXXExplicitDeclarationSpecifier(syntax, &saw_explicit);
    if (saw_explicit && *is_explicit) {
      SyntaxWarning(syntax, "duplicate-decl-specifier",
                    "Duplicate 'explicit' specifier");
    }
    *is_explicit = *is_explicit || explicit_value;
    return true;
  }
  return false;
}

// Parses `constexpr`, `consteval` or `constinit` if one is at the current
// token, diagnosing duplicates and invalid combinations.  `consteval`
// implies constexpr.  Returns true when one was consumed.
static bool ParseConstexprFamilySpecifier(Syntax* syntax, bool* is_constexpr,
                                          bool* is_consteval,
                                          bool* is_constinit) {
  if (LexMatch(syntax->lex, TOK(constexpr))) {
    if (*is_consteval) {
      SyntaxError(syntax,
                  "'constexpr' and 'consteval' cannot both be specified");
    }
    if (*is_constexpr) {
      SyntaxError(syntax, "Duplicate 'constexpr' specifier");
    }
    if (*is_constinit) {
      SyntaxError(syntax, "'constexpr' and 'constinit' cannot be combined");
    }
    *is_constexpr = true;
    return true;
  }
  if (LexMatch(syntax->lex, TOK(consteval))) {
    if (*is_consteval) {
      SyntaxWarning(syntax, "duplicate-decl-specifier",
                    "Duplicate 'consteval' specifier");
    } else if (*is_constexpr) {
      SyntaxError(syntax,
                  "'constexpr' and 'consteval' cannot both be specified");
    }
    if (*is_constinit) {
      SyntaxError(syntax, "'consteval' and 'constinit' cannot be combined");
    }
    *is_consteval = true;
    *is_constexpr = true;
    return true;
  }
  if (LexMatch(syntax->lex, TOK(constinit))) {
    if (*is_constinit) {
      SyntaxWarning(syntax, "duplicate-decl-specifier",
                    "Duplicate 'constinit' specifier");
    }
    if (*is_constexpr || *is_consteval) {
      SyntaxError(syntax, "'constexpr' and 'constinit' cannot be combined");
    }
    *is_constinit = true;
    return true;
  }
  return false;
}

// Hands a constraint on a placeholder variable type (`Concept auto x`) parsed
// by `parser` over to the syntax, where the declarator picks it up.
static void AdoptPlaceholderVariableConstraint(Syntax* syntax,
                                               TypeParser* parser) {
  if (parser->placeholder_variable_constraint != NULL) {
    ConstraintExprDelete(syntax->pending_placeholder_variable_constraint);
    syntax->pending_placeholder_variable_constraint =
        parser->placeholder_variable_constraint;
    parser->placeholder_variable_constraint = NULL;
  }
}

// Builds the type of a declaration from the type specifiers collected in
// `type_specifier`.  Without any type specifier the type is C23 `auto`
// (with `auto` storage), an error in C23, or implicit `int`.
static TypeRecord* BuildDeclarationSpecifierType(
    Syntax* syntax, TypeParser* parser, PartialTypeSpecifier* type_specifier,
    Storage* storage) {
  if (type_specifier->type == kTypeImplicit &&
      type_specifier->type_record == NULL) {
    if (CompilerCAtLeast(kLanguageStandardC23) &&
        StorageIs(*storage, STO(auto))) {
      TypeRecord* type =
          NewTypeRecordWithSize(kTypeAuto, type_specifier->quals);
      *storage &= ~STO(auto);
      return type;
    }
    if (CompilerCAtLeast(kLanguageStandardC23)) {
      SyntaxError(syntax, "type specifier missing in declaration");
      return NewTypeRecordWithSize(kTypeInt, kQualPlain);
    }
    SyntaxWarning(syntax, "implicit-int",
                  "type specifier missing, defaults to int");
    return NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  TypeRecord* type = TypeParserBuildTypeRecord(parser, type_specifier);
  if (CompilerCAtLeast(kLanguageStandardC23) &&
      StorageIs(*storage, STO(auto)) &&
      !IsPowerOf2OrZero(*storage & ~STO(thread))) {
    SyntaxError(syntax,
                "Multiple incompatible storage specifiers");
  }
  return type;
}

// A declaration specifier is a set of:
// 1. storage specifier
// 2. type specifier
// 3. function specifier (inline).
// This collects them into the output variables.  `*type` receives the
// declared type; the attributes and alignas specifiers seen are appended to
// `attributes`.  Parsing stops at the first token that is no specifier.
void ParseDeclarationSpecifier(Syntax* syntax, Storage* storage,
                               bool* is_inline, bool* is_constexpr,
                               bool* is_consteval, bool* is_constinit,
                               bool* is_explicit, TypeRecord** type,
                               Vector* attributes,
                               ParserContext context) {
  PartialTypeSpecifier type_specifier = {
    .type = kTypeImplicit,
    .quals = kQualPlain,
    .type_record = NULL,
    .error = false
  };

  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), context);
  
  while (!LexEof(syntax->lex)) {
    Storage s = SyntaxParseStorage(syntax);
    if (s != STO(implicit)) {
      AddStorageSpecifier(syntax, storage, s);
    } else if (ParseFunctionSpecifier(syntax, attributes, is_inline,
                                      is_explicit)) {
      continue;
    } else if (ParseConstexprFamilySpecifier(syntax, is_constexpr,
                                             is_consteval, is_constinit)) {
      continue;
    } else if (!(type_specifier.type != kTypeImplicit &&
                 SyntaxCurrentIdentifierFollowedByScopeOperator(syntax)) &&
               SyntaxLookingAtType(syntax) &&
               (type_specifier.type & (kTypeStruct | kTypeUnion | kTypeEnum)) == 0) {
      // Guard against a type specifier that consumes no input (e.g. a stray
      // `::` that cannot begin a new specifier): if the lexer does not
      // advance, stop rather than spinning forever.
      size_t prev_pos = syntax->lex->pos;
      Token prev_token = syntax->lex->current_token;
      type_specifier = TypeParserParseAndCombineTypes(&parser, &type_specifier);
      if (syntax->lex->pos == prev_pos &&
          syntax->lex->current_token == prev_token) {
        *type = TypeParserBuildTypeRecord(&parser, &type_specifier);
        AdoptPlaceholderVariableConstraint(syntax, &parser);
        TypeParserDestruct(&parser);
        return;
      }
    } else if (SyntaxParseCXXAlignas(syntax, attributes)) {
      continue;
    } else if (SyntaxParseAnyAttribute(syntax, attributes)) {
      continue;
    } else {
      *type = BuildDeclarationSpecifierType(syntax, &parser, &type_specifier,
                                            storage);
      AdoptPlaceholderVariableConstraint(syntax, &parser);
      NormalizeThreadLocalStorage(storage, context);
      TypeParserDestruct(&parser);
      return;
    }
  }
  NormalizeThreadLocalStorage(storage, context);
  TypeParserDestruct(&parser);
}

// Returns the class template symbol `type` instantiates from, including
// through a class-template placeholder or nested record types, or NULL when
// `type` is not tied to a class template.
Symbol* CXXClassTemplateOrigin(TypeRecord* type) {
  Symbol* placeholder_origin = TypeClassTemplatePlaceholderOrigin(type);
  if (placeholder_origin != NULL) {
    return placeholder_origin;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL) {
      return t->template_origin;
    }
  }
  return NULL;
}

// If `->` follows declarator `sym`, parses a C++ deduction guide: sets the
// return type, marks the function, and registers it on the class template.
// Consumes tokens when matched; deletes `sym` on failure.  Returns false when
// no `->` is present (lexer unchanged).
bool TryParseCXXDeductionGuide(Syntax* syntax, Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      !TypeIsFunction(sym->type) || !LexLookingAt(syntax->lex, TOK(arrow))) {
    return false;
  }
  Symbol* declared_template = CXXClassTemplateOrigin(sym->type->next);

  LexNextToken(syntax->lex);
  TypeParser return_parser;
  TypeParserInit(&return_parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* guide_return = TypeParserParseType(&return_parser, true);
  TypeParserDestruct(&return_parser);
  Symbol* deduced_template = CXXClassTemplateOrigin(guide_return);
  if (deduced_template == NULL) {
    SyntaxError(syntax, "Deduction guide return type must be a class template-id");
  } else if (declared_template != NULL && deduced_template != declared_template) {
    SyntaxError(syntax, "Deduction guide return type does not match %s",
                declared_template->name.value);
  }
  Symbol* class_template =
      deduced_template != NULL ? deduced_template : declared_template;
  if (class_template == NULL) {
    TypeRecordDelete(guide_return);
    SymbolDelete(sym);
    return true;
  }

  if (sym->type->next != NULL) {
    TypeRecordDelete(sym->type->next);
  }
  TypeRecordIncRef(guide_return);
  sym->type->next = guide_return;
  sym->type->info.function.is_deduction_guide = true;
  if (syntax->parsing_template_declaration &&
      FunctionExtraOf(&sym->type->info.function)->template_parameters.length ==
          0) {
    MoveCurrentTemplateParametersToFunction(syntax, sym->type);
  }
  if (syntax->current_template_requires_clause != NULL) {
    AddFunctionAssociatedConstraint(
        sym->type, syntax->current_template_requires_clause);
    syntax->current_template_requires_clause = NULL;
  }
  TypeAddCXXDeductionGuide(class_template, sym);
  return true;
}
