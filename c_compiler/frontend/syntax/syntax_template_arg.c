//
//  syntax_template_arg.c
//  c_compiler
//
//  Template argument lists: classifying each argument as a type, a value
//  or a template name.
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

// Decide whether the template argument at the current position is a type-id
// (as opposed to a non-type / value expression).  This refines
// SyntaxLookingAtType for the one case that must be classified differently in a
// template-argument context: a *dependent qualified-id* that is not introduced
// by `typename`.  Per [temp.res] such a name does not denote a type, so
// `Trait<T, Types...>::value` is a value argument even though the shallow
// lookahead in SyntaxLookingAtType treats every `a::b` as a potential type.  (A
// leading `typename` is already handled: SyntaxLookingAtType returns true on
// the `typename` token, and a bare dependent name -- a type parameter `T` or a
// class-template-id `C<T>` -- stays a type because it is not qualified.)
bool SyntaxIdentifierStartsDaveCCTypeTraitBuiltin(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  const char* name = syntax->lex->spelling.value;
  return strncmp(name, "__davecc_is_", 12) == 0 ||
         strcmp(name, "__davecc_invoke_result_t") == 0 ||
         strcmp(name, "__davecc_common_type_t") == 0;
}

// True when `tok` is a keyword that can begin a builtin type-specifier sequence.
static bool SyntaxBuiltinTypeSpecifierToken(Token tok) {
  switch (tok) {
    case TOK(bool):
    case TOK(char):
    case TOK(const):
    case TOK(double):
    case TOK(float):
    case TOK(int):
    case TOK(long):
    case TOK(short):
    case TOK(signed):
    case TOK(unsigned):
    case TOK(void):
    case TOK(volatile):
      return true;
    default:
      return false;
  }
}

// True at `S::*` or `ns::S<T>::*`: a nested-name-specifier ending in `::*`
// begins a member-pointer declarator.  Leaves the lexer advanced.
static bool SyntaxSkipToMemberPointerDeclarator(Syntax* syntax) {
  LexMatch(syntax->lex, TOK(coloncolon));
  while (LexMatch(syntax->lex, TOK(identifier))) {
    if (LexLookingAt(syntax->lex, TOK(less))) {
      int depth = 0;
      do {
        if (LexLookingAt(syntax->lex, TOK(less))) {
          depth++;
        } else if (LexLookingAt(syntax->lex, TOK(greater))) {
          depth--;
        } else if (LexLookingAt(syntax->lex, TOK(greatergreater))) {
          depth -= 2;
        } else if (LexLookingAt(syntax->lex, TOK(eof)) ||
                   LexLookingAt(syntax->lex, TOK(semicolon))) {
          return false;
        }
        LexNextToken(syntax->lex);
      } while (depth > 0);
    }
    if (!LexMatch(syntax->lex, TOK(coloncolon))) {
      return false;
    }
    if (LexLookingAt(syntax->lex, TOK(star))) {
      return true;
    }
  }
  return false;
}

// Classifies the (possibly qualified) name at the current token without
// consuming it.  A qualified name is a type only when it resolves to a tag or
// typedef; a dependent qualified name without `typename` is a non-type.
static void SyntaxClassifyTemplateArgumentName(Syntax* syntax,
                                               bool* is_qualified,
                                               bool* names_type,
                                               bool* followed_by_brace) {
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                     TC(closebra) | TC(exprsep));
  *is_qualified = name.is_qualified;
  *followed_by_brace = LexLookingAt(syntax->lex, TOK(lbrace));
  *names_type = false;
  if (name.is_qualified) {
    if (SyntaxFindQualifiedTag(syntax, &name) != NULL) {
      *names_type = true;
    } else {
      Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, &name);
      *names_type =
          symbol != NULL && StorageIs(symbol->storage, STO(typedef));
    }
  }
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
}

// `integral_constant<bool, bool(!P::value)>`: a type-name followed by `(expr)`
// or `{expr}` is a functional cast, not a type-id.  `bool(int)`, `bool(*)()`
// and `int (S::*)` stay type-ids.
static bool SyntaxTemplateArgumentIsFunctionalCast(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool saw_type = false;
  while (SyntaxBuiltinTypeSpecifierToken(syntax->lex->current_token)) {
    saw_type = true;
    LexNextToken(syntax->lex);
  }
  bool cast = false;
  if (saw_type && LexLookingAt(syntax->lex, TOK(lbrace))) {
    cast = true;
  } else if (saw_type && LexLookingAt(syntax->lex, TOK(lparen))) {
    LexNextToken(syntax->lex);
    Token inner = syntax->lex->current_token;
    if (inner == TOK(bang) || inner == TOK(minus) || inner == TOK(plus) ||
        inner == TOK(tilde) || inner == TOK(number) || inner == TOK(sizeof) ||
        inner == TOK(alignof) || inner == TOK(true) || inner == TOK(false) ||
        inner == TOK(nullptr) ||
        ((inner == TOK(identifier) || inner == TOK(coloncolon)) &&
         !SyntaxLookingAtType(syntax))) {
      cast = true;
    } else if (inner == TOK(identifier) || inner == TOK(coloncolon)) {
      // `bool(P1::value)`: `P1` is a type, but `P1::value` is not.
      bool is_qualified = false;
      bool names_type = false;
      bool followed_by_brace = false;
      SyntaxClassifyTemplateArgumentName(syntax, &is_qualified, &names_type,
                                         &followed_by_brace);
      cast = is_qualified && !names_type;
    }
    if (cast && (inner == TOK(identifier) || inner == TOK(coloncolon)) &&
        SyntaxSkipToMemberPointerDeclarator(syntax)) {
      cast = false;
    }
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return cast;
}

// True when the template argument at the current token should be parsed as a
// type-id rather than a non-type expression, including dependent qualified-name
// rules and functional-cast disambiguation.
static bool SyntaxTemplateArgumentLooksLikeType(Syntax* syntax) {
  if (!SyntaxLookingAtType(syntax)) {
    return false;
  }
  if (SyntaxIdentifierStartsDaveCCTypeTraitBuiltin(syntax)) {
    return false;
  }
  if (SyntaxTemplateArgumentIsFunctionalCast(syntax)) {
    return false;
  }
  Token tok = syntax->lex->current_token;
  if (tok != TOK(identifier) && tok != TOK(coloncolon)) {
    return true;
  }
  bool is_qualified = false;
  bool names_type = false;
  bool followed_by_braced_initializer = false;
  SyntaxClassifyTemplateArgumentName(syntax, &is_qualified, &names_type,
                                     &followed_by_braced_initializer);
  if (is_qualified) {
    return names_type && !followed_by_braced_initializer;
  }
  return !followed_by_braced_initializer;
}

// True when `symbol` names something that can appear as a bare template
// template argument (class template, alias template, variable template, or
// concept).
bool SymbolNamesTemplateArgument(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->flags.is_template_template_parameter ||
      symbol->alias_template != NULL || symbol->variable_template != NULL ||
      symbol->flags.is_concept) {
    return true;
  }
  return symbol->flags.is_template && symbol->type != NULL &&
         TypeIsStructOrUnion(symbol->type) &&
         symbol->type->info.struct_info != NULL &&
         symbol->type->info.struct_info->is_template;
}

// Looks ahead at the current (possibly qualified) name without consuming tokens
// and returns the template it names when it is a bare template-name argument,
// else NULL.
static Symbol* SyntaxBareTemplateArgumentSymbol(Syntax* syntax) {
  Token tok = syntax->lex->current_token;
  if (tok != TOK(identifier) && tok != TOK(coloncolon)) {
    return NULL;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
      syntax, &name, TC(closebra) | TC(exprsep));
  bool has_template_id = false;
  for (size_t i = 0; i < name.template_arguments.length; i++) {
    if (name.template_arguments.value.p[i] != NULL) {
      has_template_id = true;
      break;
    }
  }
  Symbol* symbol = NULL;
  if (!has_template_id) {
    if (!name.is_qualified && !name.absolute) {
      symbol = FindLocalSymbol(syntax->local_symbol_stack, &name.spelling);
      if (symbol == NULL) {
        symbol = FindGlobalSymbol(&name.spelling);
      }
    }
    if (symbol == NULL) {
      symbol = SyntaxFindQualifiedSymbol(syntax, &name);
      // A member alias (`using M = const I<U>;`) names its type, not the tag
      // of the class template that type is a specialization of.
      bool names_alias = symbol != NULL &&
                         StorageIs(symbol->storage, STO(typedef)) &&
                         symbol->alias_template == NULL &&
                         !symbol->flags.is_template;
      if (!SymbolNamesTemplateArgument(symbol) && !names_alias) {
        symbol = SyntaxFindQualifiedTag(syntax, &name);
      }
    }
    if (!SymbolNamesTemplateArgument(symbol)) {
      symbol = NULL;
    }
  }
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return symbol;
}

// When a type argument is really the bare name of a class or alias template,
// rewrites `arg` to a template template argument, preserving pack index metadata
// unless the type is the injected-class-name of the class being defined.
static void NormalizeTemplateNameArgument(Syntax* syntax, TemplateArgument* arg,
                                          Symbol* bare_template) {
  if (arg == NULL || arg->kind != kTemplateParameterType || arg->type == NULL) {
    return;
  }
  if (!SymbolNamesTemplateArgument(bare_template)) {
    return;
  }
  // Inside a partial or explicit specialization's body the bare template name
  // is that specialization's injected-class-name (`is_same_v<I1, F>` within
  // `struct I1<R(A)>`).  The type parser has already resolved it to the
  // specialization being defined, which is not the primary's struct.
  if (TypeIsStructOrUnion(arg->type) && bare_template->type != NULL &&
      TypeIsStructOrUnion(bare_template->type) &&
      arg->type->info.struct_info != bare_template->type->info.struct_info) {
    for (Struct* s = syntax->cxx_class_head; s != NULL; s = s->lexical_parent) {
      if (s == arg->type->info.struct_info) {
        return;
      }
    }
  }
  Symbol* origin = bare_template;
  ASTNode* pack_index_expr =
      arg->type->is_pack_index
          ? ASTNodeClone(arg->type->pack_index_expr, IdentityCloneNode, NULL,
                         NULL)
          : NULL;
  TypeRecordDelete(arg->type);
  arg->type = NULL;
  arg->kind = kTemplateParameterTemplate;
  arg->template_symbol = origin;
  arg->pack_index_expr = pack_index_expr;
  arg->template_parameter_index =
      origin->flags.is_template_template_parameter
          ? origin->template_parameter_index : -1;
}

// Returns a new type template argument with no value yet, located at the
// current token.
static TemplateArgument* NewTemplateArgumentAtCurrentToken(Lex* lex) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  // Record where this argument is written so a diagnostic raised while it is
  // substituted (possibly in a far-removed instantiation) points back here.
  arg->location = lex->current_token_location;
  return arg;
}

// Returns true when the bare template name `bare_template` written as a
// template argument names a concept, a variable template, or a non-type
// template template parameter: something that is not a type.
static bool BareTemplateNameIsNotAType(Symbol* bare_template) {
  return bare_template != NULL &&
         (bare_template->flags.is_concept ||
          bare_template->variable_template != NULL ||
          (bare_template->flags.is_template_template_parameter &&
           bare_template->template_template_parameter_kind !=
               kTemplateTemplateParameterType));
}

// Parses a template argument that names the template `bare_template` itself
// (optionally followed by `...` or a C++29 pack index `...[i]`) into `arg`.
static void ParseTemplateNameArgument(Syntax* syntax, TemplateArgument* arg,
                                      Symbol* bare_template) {
  Lex* lex = syntax->lex;
  FullyQualifiedIdentifier template_name;
  FullyQualifiedIdentifierInit(&template_name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
      syntax, &template_name, TC(closebra) | TC(exprsep));
  FullyQualifiedIdentifierDestruct(&template_name);
  arg->kind = kTemplateParameterTemplate;
  arg->template_symbol = bare_template;
  arg->template_parameter_index =
      bare_template->flags.is_template_template_parameter
          ? bare_template->template_parameter_index
          : -1;
  if (LexMatch(lex, TOK(ellipsis))) {
    if (LexMatch(lex, TOK(lsquare))) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
        SyntaxError(syntax,
                    "Template-name pack indexing requires C++29");
      }
      arg->pack_index_expr =
          SyntaxParseSingleExpression(syntax, TC(closebra));
      SyntaxNeedBracket(syntax, TOK(rsquare),
                        TC(closebra) | TC(exprsep));
    } else {
      arg->is_pack_expansion = true;
    }
  }
  arg->references_parameter_pack =
      arg->is_pack_expansion || arg->pack_index_expr != NULL ||
      bare_template->flags.is_parameter_pack;
}

// Parses a type template argument (a type-id, possibly a pack expansion or a
// reference to one) into `arg`.  A type that is really the bare name of a
// class template (`bare_template`) becomes a template template argument.
static void ParseTypeTemplateArgument(Syntax* syntax, TemplateArgument* arg,
                                      Symbol* bare_template) {
  Lex* lex = syntax->lex;
  TypeParser parser;
  TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* sym = TypeParserParseDeclarator(&parser, type);
  TypeParserDestruct(&parser);
  if (sym != NULL) {
    arg->type = TypeRecordInternArgumentOrCopy(sym->type);
    arg->is_pack_expansion = sym->flags.is_parameter_pack;
    if (arg->is_pack_expansion && type != NULL) {
      arg->type = TypeRecordAddQualifiers(arg->type, GetQualifiers(type));
    }
    SymbolDelete(sym);
  } else {
    arg->type = type;
    arg->is_pack_expansion = CompilerIsCXX() && LexMatch(lex, TOK(ellipsis));
  }
  if (CompilerIsCXX() &&
      (LexLookingAt(lex, TOK(amp)) ||
       LexLookingAt(lex, TOK(ampamp)))) {
    bool rvalue = LexMatch(lex, TOK(ampamp));
    if (!rvalue) {
      LexMatch(lex, TOK(amp));
    }
    TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
    TypeRecordChain(ref, arg->type);
    ref->type = arg->type->type;
    TypeRecordCalculateSize(ref);
    TypeRecordDelete(arg->type);
    arg->type = ref;
    if (CompilerIsCXX() && LexMatch(lex, TOK(ellipsis))) {
      arg->is_pack_expansion = true;
    }
  }
  arg->references_parameter_pack =
      CXXTypeContainsParameterPack(syntax, arg->type);
  NormalizeTemplateNameArgument(syntax, arg, bare_template);
}

// Returns true when the template parameter in scope with index
// `parameter_index` is a parameter pack.
static bool TemplateParameterIndexIsPack(Syntax* syntax, int parameter_index) {
  for (size_t i = 0;
       syntax->current_template_parameters != NULL &&
       i < syntax->current_template_parameters->length; i++) {
    TemplateParameter* parameter =
        syntax->current_template_parameters->value.p[i];
    if (parameter != NULL &&
        parameter->index == parameter_index &&
        parameter->is_parameter_pack) {
      return true;
    }
  }
  return false;
}

// Evaluates the non-dependent non-type template argument `expr` (consumed)
// into `arg`: a direct reference to a template parameter, a constant value,
// or an lvalue (pointer / reference argument).  With template parameters in
// scope an argument that cannot be folded yet is kept as a dependent
// expression; otherwise it is diagnosed.
static void FoldNonTypeTemplateArgument(Syntax* syntax, TemplateArgument* arg,
                                        ASTNode* expr,
                                        int direct_template_parameter_index,
                                        bool template_parameters_in_scope) {
  Lex* lex = syntax->lex;
  if (direct_template_parameter_index >= 0) {
    arg->template_parameter_index = direct_template_parameter_index;
  }
  Symbol* lvalue_symbol = TemplateArgumentLvalueSymbol(expr);
  compiler->constant_evaluation_required_depth++;
  expr = AnalyzeExpression(expr);
  compiler->constant_evaluation_required_depth--;
  bool value_ok = false;
  if (arg->template_parameter_index >= 0) {
    arg->type = expr != NULL && expr->type != NULL
                    ? TypeRecordCopy(expr->type) : NULL;
    value_ok = true;
  } else {
    value_ok = TemplateArgumentSetFromExpression(arg, expr);
    if (lvalue_symbol != NULL) {
      value_ok = TemplateArgumentSetLvalue(arg, lvalue_symbol, value_ok,
                                           template_parameters_in_scope);
    }
  }
  if (!value_ok) {
    if (template_parameters_in_scope &&
        expr->op == AST_OP(identifier)) {
      IdentifierASTNode* id = (IdentifierASTNode*)expr;
      if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
          !id->symbol->flags.is_template_type_parameter) {
        arg->template_parameter_index =
            id->symbol->template_parameter_index;
      }
    }
    if (arg->template_parameter_index < 0) {
      if (template_parameters_in_scope) {
        arg->dependent_expr = expr;
        arg->is_pack_expansion = LexMatch(lex, TOK(ellipsis));
        return;
      }
      SyntaxError(syntax,
                  "Template non-type argument must be an integer constant "
                  "expression, or a pointer/member-pointer argument must "
                  "be a constant expression");
    }
  }
  ASTNodeDelete(expr);
  arg->is_pack_expansion = LexMatch(lex, TOK(ellipsis));
}

// Parses a non-type template argument expression into `arg`.  A
// value-dependent expression is kept for substitution; anything else is
// folded now.
static void ParseNonTypeTemplateArgument(Syntax* syntax,
                                         TemplateArgument* arg) {
  Lex* lex = syntax->lex;
  bool old_parsing_template_argument = syntax->parsing_template_argument;
  syntax->parsing_template_argument = true;
  ASTNode* expr = SyntaxParseSingleExpression(syntax,
                                              TC(closebra) | TC(exprsep));
  syntax->parsing_template_argument = old_parsing_template_argument;
  arg->kind = kTemplateParameterNonType;
  arg->references_parameter_pack =
      SyntaxExpressionContainsParameterPack(expr);
  int direct_template_parameter_index = -1;
  bool is_direct_nttp =
      ExpressionIsNonTypeTemplateParameter(expr,
                                           &direct_template_parameter_index);
  if (is_direct_nttp &&
      TemplateParameterIndexIsPack(syntax, direct_template_parameter_index)) {
    arg->references_parameter_pack = true;
  }
  // A template argument can only be value-dependent when template parameters
  // are in scope.  Outside one it must be a concrete constant expression, so
  // it has to be folded now.  The dependence probe below inspects the
  // still-unanalysed expression, where every interior node (a `+`, a call,
  // ...) has a null/unknown type and so is spuriously reported as dependent;
  // gating on the parse state avoids deferring -- and thus dropping to 0 --
  // such non-dependent arguments.  Parameters are also in scope for the rest
  // of their own list, which is where the pre-C++20 constraint idiom writes
  // its condition (`typename enable_if<C<T>, int>::type = 0`); the
  // declaration flag is not set until the list has been read, so ask the
  // parameter count as well.
  bool template_parameters_in_scope =
      syntax->parsing_template_declaration ||
      syntax->current_template_parameter_count > 0;
  if (template_parameters_in_scope && !is_direct_nttp &&
      ExpressionContainsDependentTemplateParameter(expr)) {
    // A value-dependent trait condition such as `!is_integral<It>::value`:
    // resolving it now would fold the primary template's value.  Keep the
    // expression and re-fold it once the parameters become concrete.
    arg->dependent_expr = expr;
    arg->is_pack_expansion = LexMatch(lex, TOK(ellipsis));
    return;
  }
  FoldNonTypeTemplateArgument(syntax, arg, expr,
                              direct_template_parameter_index,
                              template_parameters_in_scope);
}

// Parses a template argument list `<...>` if one follows, synchronizing on
// `followers` after the closing `>`.  Each argument is parsed as a template
// name, a type or an expression, whichever it looks like.  Returns the
// TemplateArgument vector, or NULL when no `<` follows.
Vector* SyntaxParseTemplateArgumentList(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  if (!LexMatch(lex, TOK(less))) {
    return NULL;
  }
  // Only the outermost template-id of a member's return type is deferred;
  // arguments such as `is_same_v<Self, T>` are evaluated as written.
  bool saved_defer_class_head_ids =
      syntax->defer_template_ids_naming_class_head;
  syntax->defer_template_ids_naming_class_head = false;
  Vector* args = NewVector();
  while (!LexEof(lex) && !LexLookingAtClosingAngle(lex)) {
    TemplateArgument* arg = NewTemplateArgumentAtCurrentToken(lex);
    Symbol* bare_template = SyntaxBareTemplateArgumentSymbol(syntax);
    if (BareTemplateNameIsNotAType(bare_template)) {
      ParseTemplateNameArgument(syntax, arg, bare_template);
    } else if (SyntaxTemplateArgumentLooksLikeType(syntax)) {
      ParseTypeTemplateArgument(syntax, arg, bare_template);
    } else {
      ParseNonTypeTemplateArgument(syntax, arg);
    }
    VectorAppend(args, arg);
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  syntax->defer_template_ids_naming_class_head = saved_defer_class_head_ids;
  SyntaxNeedTemplateClose(syntax, followers);
  return args;
}
