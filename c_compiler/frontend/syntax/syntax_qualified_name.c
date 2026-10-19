//
//  syntax_qualified_name.c
//  c_compiler
//
//  Qualified names: parsing `A::B<T>::c` and operator names, and looking
//  up namespaces, symbols and tags through nested-name-specifiers.
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

static bool QualifiedNameHasMemberTemplate(Syntax* syntax,
                                           FullyQualifiedIdentifier* name);
static bool TypeContainsTemplateParameterReference(TypeRecord* type);
static bool MemberCanBeQualifiedPrefix(StructMember* member,
                                       Struct* parent_struct);
static Symbol* SyntaxFindQualifiedPrefixSymbolImpl(
    Syntax* syntax, FullyQualifiedIdentifier* name, size_t component_count,
    bool allow_dependent_template_args);

// Initializes `name` to an empty unqualified identifier with empty component
// and template-argument vectors and an empty spelling string.
void FullyQualifiedIdentifierInit(FullyQualifiedIdentifier* name) {
  name->absolute = false;
  name->is_qualified = false;
  VectorInit(&name->components);
  VectorInit(&name->template_arguments);
  StringInit(&name->spelling, NULL);
}

// Frees every `TemplateArgument` in `args` and the vector itself.  No-op
// when `args` is NULL.
static void DeleteTemplateArgumentVector(Vector* args) {
  if (args == NULL) {
    return;
  }
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
}

// Releases the component strings, per-component template-argument vectors,
// and spelling held by `name`.
void FullyQualifiedIdentifierDestruct(FullyQualifiedIdentifier* name) {
  VectorDestructWithContents(&name->components,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&name->template_arguments,
                             (VectorElementDestructor)DeleteTemplateArgumentVector,
                             /*free_element=*/false);
  StringDestruct(&name->spelling);
}

// Returns the spelling of the last nested-name component of `name`, or `""`
// when there are no components.
const char* FullyQualifiedIdentifierLast(FullyQualifiedIdentifier* name) {
  if (name->components.length == 0) {
    return "";
  }
  String* last = name->components.value.p[name->components.length - 1];
  return last->value;
}

// Appends `component` to `name`, updating its spelling and adding a NULL
// placeholder for template arguments on the new component.
static void FullyQualifiedIdentifierAppend(FullyQualifiedIdentifier* name,
                                           String* component) {
  if (name->spelling.length != 0 || name->absolute) {
    StringAppend(&name->spelling, "::");
  }
  StringAppendString(&name->spelling, component);
  VectorAppend(&name->components, NewString(component->value));
  VectorAppend(&name->template_arguments, NULL);
}

// When the current token is `operator`, consumes the operator token(s) and
// builds the corresponding function name in `name`.  Returns false when not
// in C++ or not at `operator`; otherwise returns true and may emit diagnostics
// for invalid literal or unsupported operators.
bool SyntaxParseOperatorFunctionName(Syntax* syntax, String* name) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(operator))) {
    return false;
  }
  Token op = syntax->lex->current_token;
  switch (op) {
    case TOK(string): {
      LexValidateUnevaluatedString(syntax->lex, "literal operator name",
                                   /*allow_user_defined_suffix=*/true);
      if (syntax->lex->spelling.length != 0) {
        SyntaxError(syntax, "Expected empty string in literal operator name");
      }
      StringInit(name, "operator\"\"");
      if (syntax->lex->ud_suffix.length != 0) {
        StringAppendString(name, &syntax->lex->ud_suffix);
        LexNextToken(syntax->lex);
        return true;
      }
      LexNextToken(syntax->lex);
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        StringAppend(name, syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      } else {
        SyntaxError(syntax, "Expected suffix in literal operator name");
      }
      return true;
    }
    case TOK(plus):
    case TOK(minus):
    case TOK(star):
    case TOK(slash):
    case TOK(percent):
    case TOK(plusplus):
    case TOK(minusminus):
    case TOK(lessless):
    case TOK(greatergreater):
    case TOK(amp):
    case TOK(bar):
    case TOK(caret):
    case TOK(tilde):
    case TOK(bang):
    case TOK(ampamp):
    case TOK(barbar):
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
    case TOK(careteq):
    case TOK(equalequal):
    case TOK(bangeq):
    case TOK(less):
    case TOK(lesseq):
    case TOK(spaceship):
    case TOK(greater):
    case TOK(greatereq):
    case TOK(arrow):
    case TOK(arrowstar):
    case TOK(comma):
      StringInit(name, "operator");
      StringAppend(name, TokenName(op));
      LexNextToken(syntax->lex);
      return true;
    case TOK(new):
    case TOK(delete):
      StringInit(name, "operator ");
      StringAppend(name, TokenName(op));
      LexNextToken(syntax->lex);
      if (LexMatch(syntax->lex, TOK(lsquare))) {
        SyntaxNeedBracket(syntax, TOK(rsquare), TC(decl));
        StringAppend(name, "[]");
      }
      return true;
    case TOK(co_await):
      StringInit(name, "operator co_await");
      LexNextToken(syntax->lex);
      return true;
    case TOK(lsquare):
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rsquare), TC(decl));
      StringInit(name, "operator[]");
      return true;
    case TOK(lparen):
      LexNextToken(syntax->lex);
      SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));
      StringInit(name, "operator()");
      return true;
    default:
      SyntaxError(syntax, "Unsupported overloaded operator %s", TokenName(op));
      StringInit(name, "operator?");
      if (!LexEof(syntax->lex)) {
        LexNextToken(syntax->lex);
      }
      return true;
  }
}

// Returns true when `symbol` is a user-defined literal operator template of
// the form `operator""` with a single char pack non-type template parameter.
bool SyntaxIsCXXNumericLiteralOperatorTemplate(Symbol* symbol) {
  if (symbol == NULL || !symbol->flags.is_template || symbol->type == NULL ||
      !TypeIsFunction(symbol->type) ||
      !StringStartsWith(&symbol->name, "operator\"\"") ||
      symbol->type->info.function.prototype.length != 0 ||
      FunctionExtraOf(&symbol->type->info.function)
              ->template_parameters.length != 1) {
    return false;
  }
  TemplateParameter* parameter = FunctionExtraOf(&symbol->type->info.function)
                                     ->template_parameters.value.p[0];
  return parameter != NULL && parameter->kind == kTemplateParameterNonType &&
         parameter->is_parameter_pack && parameter->type != NULL &&
         parameter->type->declarator == kDeclPrimitive &&
         parameter->type->type == kTypeChar &&
         GetQualifiers(parameter->type) == kQualPlain;
}

// Parses a member `operator` name at the current token into `name`.  For a
// conversion operator, parses the target type; when it is dependent, stores a
// new reference in `*dependent_conversion_type` if non-NULL.  Returns false
// when not at `operator`.
bool SyntaxParseMemberOperatorName(Syntax* syntax, String* name,
                                   TypeRecord** dependent_conversion_type) {
  if (dependent_conversion_type != NULL) {
    *dependent_conversion_type = NULL;
  }
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(operator))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool is_conversion = SyntaxLookingAtType(syntax);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);

  if (!is_conversion) {
    return SyntaxParseOperatorFunctionName(syntax, name);
  }

  LexNextToken(syntax->lex);
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = ParseCXXConversionType(&parser);
  ConversionOperatorName(type, name);
  TypeParserDestruct(&parser);
  if (dependent_conversion_type != NULL &&
      TypeContainsTemplateParameter(type)) {
    TypeRecordIncRef(type);
    *dependent_conversion_type = type;
  }
  return true;
}

// `Class::*` is a pointer-to-member declarator, not another nested-name
// component.  The qualified-id parser must leave the `::` in place.
static bool QualifiedNameContinuesWithMemberPointer(Lex* lex) {
  if (!CompilerIsCXX() || !LexLookingAt(lex, TOK(coloncolon))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  LexNextToken(lex);
  bool member_pointer = LexLookingAt(lex, TOK(star));
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return member_pointer;
}

// Consumes one nested-name component (identifier or operator-function name)
// into `component`.  Returns false when the current token starts neither.
static bool ParseQualifiedIdentifierComponent(Syntax* syntax, String* component) {
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    StringInit(component, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
    return true;
  }
  return SyntaxParseOperatorFunctionName(syntax, component);
}

// Parses a nested-name-specifier and unqualified-id into `name`, stopping
// before `::` that begins a pointer-to-member.  Returns false when the first
// component is missing; may diagnose and still return true after errors.
bool SyntaxParseFullyQualifiedIdentifier(Syntax* syntax,
                                         FullyQualifiedIdentifier* name) {
  Lex* lex = syntax->lex;
  if (LexMatch(lex, TOK(coloncolon))) {
    name->absolute = true;
    name->is_qualified = true;
  }

  String component;
  if (!ParseQualifiedIdentifierComponent(syntax, &component)) {
    return false;
  }

  FullyQualifiedIdentifierAppend(name, &component);
  StringDestruct(&component);
  while (LexLookingAt(lex, TOK(coloncolon)) &&
         !QualifiedNameContinuesWithMemberPointer(lex)) {
    LexMatch(lex, TOK(coloncolon));
    name->is_qualified = true;
    bool is_destructor = LexMatch(lex, TOK(tilde));
    if (!is_destructor && CompilerIsCXX()) {
      LexMatch(lex, TOK(template));
    }
    if (!ParseQualifiedIdentifierComponent(syntax, &component)) {
      SyntaxError(syntax, "Expected identifier after '::'");
      return true;
    }
    if (is_destructor) {
      String destructor_name;
      StringInit(&destructor_name, "~");
      StringAppendString(&destructor_name, &component);
      FullyQualifiedIdentifierAppend(name, &destructor_name);
      StringDestruct(&destructor_name);
    } else {
      FullyQualifiedIdentifierAppend(name, &component);
    }
    StringDestruct(&component);
  }
  return true;
}

// True when the `<` at the current token introduces a template-argument list
// rather than a comparison.  Used for names that are not already known to be
// templates: `tuple_size<T>::value < 2` must leave the second `<` as an
// operator, while `Unknown<T>::type` is still a template-id.
static bool AngleBracketsLookLikeTemplateId(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(less))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  int depth = 0;
  int paren_depth = 0;
  int square_depth = 0;
  int brace_depth = 0;
  bool balanced = false;
  bool closed_by_nested_angle = false;
  do {
    Token token = syntax->lex->current_token;
    bool at_top = paren_depth == 0 && square_depth == 0 && brace_depth == 0;
    if (at_top &&
        (token == TOK(semicolon) || token == TOK(lbrace) ||
         token == TOK(rbrace))) {
      break;
    }
    if (token == TOK(lparen)) {
      paren_depth++;
    } else if (token == TOK(rparen)) {
      if (paren_depth > 0) {
        paren_depth--;
      }
    } else if (token == TOK(lsquare)) {
      square_depth++;
    } else if (token == TOK(rsquare)) {
      if (square_depth > 0) {
        square_depth--;
      }
    } else if (token == TOK(lbrace)) {
      brace_depth++;
    } else if (token == TOK(rbrace)) {
      if (brace_depth > 0) {
        brace_depth--;
      }
    } else if (at_top) {
      if (token == TOK(less)) {
        depth++;
      } else {
        int closes = LexClosingAngleCount(token);
        if (closes > 0) {
          if (closes > depth) {
            balanced = true;
            closed_by_nested_angle = true;
            break;
          }
          depth -= closes;
          if (depth == 0) {
            balanced = true;
            LexNextToken(syntax->lex);
            break;
          }
        }
      }
    }
    LexNextToken(syntax->lex);
  } while (!LexEof(syntax->lex) && depth > 0);

  bool result = false;
  if (balanced) {
    if (closed_by_nested_angle) {
      result = true;
    } else {
      Token next = syntax->lex->current_token;
      result = next == TOK(coloncolon) || next == TOK(lparen) ||
               next == TOK(rparen) || next == TOK(comma) ||
               next == TOK(semicolon) || next == TOK(lbrace) ||
               next == TOK(rbrace) || next == TOK(colon) ||
               next == TOK(eof) || next == TOK(ellipsis) ||
               next == TOK(amp) || next == TOK(ampamp) ||
               next == TOK(star) || next == TOK(lsquare) ||
               LexLookingAtClosingAngle(syntax->lex);
    }
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// A non-template can share a name with a function template (`operator<<(char)`
// beside `operator<<(const T&)`).  The first symbol is not enough: `<` after
// that name is still a template-argument list.
static bool SymbolOrOverloadIsTemplate(Symbol* symbol) {
  for (Symbol* candidate = symbol; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->flags.is_template ||
        candidate->flags.is_template_template_parameter ||
        SymbolExtraOf(candidate)->alias_template != NULL ||
        SymbolExtraOf(candidate)->variable_template != NULL) {
      return true;
    }
    if (candidate->type == NULL || !TypeIsFunction(candidate->type) ||
        candidate->type->info.function.cxx_member_owner == NULL) {
      continue;
    }
    StructMember* head = FindStructMember(
        candidate->type->info.function.cxx_member_owner, &candidate->name);
    for (StructMember* member = head; member != NULL;
         member = member->overload_next) {
      if (member->symbol != NULL && member->symbol->flags.is_template) {
        return true;
      }
    }
  }
  return false;
}

// A '<' after this name opens a template-argument list only when the name can
// actually be a template.  An NTTP (`k < sizeof...(Args)`) is a comparison;
// consuming the '<' as a template-id steals the closing '>' of the enclosing
// argument list (`enable_if<(k < sizeof...(Args))>`).
static bool NameMayBeTemplateId(Syntax* syntax, String* component) {
  if (component == NULL) {
    return true;
  }
  Symbol* symbol = SyntaxFindSymbol(syntax, component);
  if (symbol != NULL) {
    if (SymbolOrOverloadIsTemplate(symbol)) {
      return true;
    }
    // The injected-class-name of an explicit specialization denotes that
    // specialization, but `numeric_limits<double>` still names the primary.
    if (SymbolExtraOf(symbol)->alias_target != NULL &&
        SymbolExtraOf(symbol)->alias_target->flags.is_template &&
        !symbol->flags.is_using_alias) {
      return true;
    }
    // The injected-class-name of a class template is not flagged is_template
    // until the class body finishes, but `Class<Args>` is still a template-id.
    if (StorageIs(symbol->storage, STO(typedef)) && symbol->type != NULL &&
        TypeIsStructOrUnion(symbol->type) &&
        symbol->type->info.struct_info != NULL &&
        (symbol->type->info.struct_info->is_template ||
         symbol->type->info.struct_info->template_parameter_count > 0 ||
         symbol->type->info.struct_info->defining_template_scope_count > 0)) {
      return true;
    }
    return false;
  }
  Symbol* tag = SyntaxFindTag(syntax, component);
  if (tag != NULL) {
    return tag->flags.is_template;
  }
  return AngleBracketsLookLikeTemplateId(syntax);
}

// Returns true when qualified name `name` denotes something that can be
// followed by a template-argument list, including a hidden member template.
static bool QualifiedNameIsTemplate(Syntax* syntax,
                                   FullyQualifiedIdentifier* name) {
  if (name == NULL || (!name->is_qualified && !name->absolute)) {
    return false;
  }
  Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, name);
  if (symbol == NULL) {
    symbol = SyntaxFindQualifiedTag(syntax, name);
  }
  if (symbol != NULL) {
    if (SymbolOrOverloadIsTemplate(symbol)) {
      return true;
    }
    if (symbol->type != NULL && TypeIsStructOrUnion(symbol->type) &&
        symbol->type->info.struct_info != NULL &&
        (symbol->type->info.struct_info->is_template ||
         symbol->type->info.struct_info->template_parameter_count > 0)) {
      return true;
    }
  }
  // Qualified lookup drops non-static members, so `LogMessage::operator<<`
  // is invisible when the first overload is an ordinary member function.
  return QualifiedNameHasMemberTemplate(syntax, name);
}

// When `<` at the current token opens a template-id for `component` in the
// context of `name`, parses the argument list with `followers` and returns a
// new vector of arguments; otherwise returns NULL without consuming tokens.
static Vector* SyntaxConsumeOptionalTemplateId(Syntax* syntax,
                                               TokenClass followers,
                                               FullyQualifiedIdentifier* name,
                                               String* component) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(less))) {
    return NULL;
  }
  // A qualified component can be hidden by an unqualified non-template of the
  // same name (`template <class FieldMatcher> ... internal::FieldMatcher<T>`).
  // The qualifier decides.
  if (!NameMayBeTemplateId(syntax, component) &&
      !QualifiedNameIsTemplate(syntax, name)) {
    return NULL;
  }
  return SyntaxParseTemplateArgumentList(syntax, followers);
}

// Attaches parsed template arguments `args` to the last component of `name`,
// replacing and freeing any previous vector for that component.  Frees `args`
// when `name` has no components.
static void FullyQualifiedIdentifierSetLastTemplateArguments(
    FullyQualifiedIdentifier* name, Vector* args) {
  if (name->template_arguments.length == 0) {
    if (args != NULL) {
      DeleteTemplateArgumentVector(args);
    }
    return;
  }
  size_t index = name->template_arguments.length - 1;
  Vector* existing = name->template_arguments.value.p[index];
  if (existing != NULL) {
    DeleteTemplateArgumentVector(existing);
  }
  VectorSet(&name->template_arguments, index, args);
}

// Like SyntaxParseFullyQualifiedIdentifier, but also parses optional
// template-argument lists after each component when `followers` allows.
bool SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
    Syntax* syntax, FullyQualifiedIdentifier* name, TokenClass followers) {
  Lex* lex = syntax->lex;
  if (LexMatch(lex, TOK(coloncolon))) {
    name->absolute = true;
    name->is_qualified = true;
  }

  String component;
  if (!ParseQualifiedIdentifierComponent(syntax, &component)) {
    return false;
  }
  FullyQualifiedIdentifierAppend(name, &component);
  FullyQualifiedIdentifierSetLastTemplateArguments(
      name, SyntaxConsumeOptionalTemplateId(syntax, followers, name,
                                            &component));
  StringDestruct(&component);

  while (LexLookingAt(lex, TOK(coloncolon)) &&
         !QualifiedNameContinuesWithMemberPointer(lex)) {
    LexMatch(lex, TOK(coloncolon));
    name->is_qualified = true;
    bool is_destructor = LexMatch(lex, TOK(tilde));
    if (!is_destructor && CompilerIsCXX()) {
      LexMatch(lex, TOK(template));
    }
    if (!ParseQualifiedIdentifierComponent(syntax, &component)) {
      SyntaxError(syntax, "Expected identifier after '::'");
      return true;
    }
    if (is_destructor) {
      String destructor_name;
      StringInit(&destructor_name, "~");
      StringAppendString(&destructor_name, &component);
      FullyQualifiedIdentifierAppend(name, &destructor_name);
      StringDestruct(&destructor_name);
    } else {
      FullyQualifiedIdentifierAppend(name, &component);
    }
    FullyQualifiedIdentifierSetLastTemplateArguments(
        name, SyntaxConsumeOptionalTemplateId(syntax, followers, name,
                                              &component));
    StringDestruct(&component);
  }
  return true;
}

// Resolves child namespace `name` under `parent`, including inline namespaces.
// Sets `*ambiguous` and diagnoses when the name is ambiguous; returns NULL
// then or when the child is not found.
static Namespace* ResolveNamespaceChildCheckingAmbiguity(Syntax* syntax,
                                                         Namespace* parent,
                                                         String* name,
                                                         bool* ambiguous) {
  if (ambiguous != NULL) {
    *ambiguous = false;
  }
  NamespaceInlineChildLookup result =
      NamespaceResolveChildInInlineSet(parent, name);
  if (result.status == kInlineLookupAmbiguous) {
    if (ambiguous != NULL) {
      *ambiguous = true;
    }
    SyntaxError(syntax, "ambiguous namespace name '%s'", name->value);
    return NULL;
  }
  return result.child;
}

// Looks up ordinary symbol `name` at global scope, merging inline-namespace
// results with the global table.  Diagnoses ambiguity; returns NULL when
// ambiguous or not found.
static Symbol* ResolveGlobalSymbolCheckingAmbiguity(Syntax* syntax,
                                                    String* name,
                                                    bool* ambiguous) {
  if (ambiguous != NULL) {
    *ambiguous = false;
  }
  NamespaceInlineSymbolLookup inline_result =
      NamespaceResolveSymbolInInlineSet(compiler->global_namespace, name);
  if (inline_result.status == kInlineLookupAmbiguous) {
    if (ambiguous != NULL) {
      *ambiguous = true;
    }
    SyntaxError(syntax, "reference to '%s' is ambiguous", name->value);
    return NULL;
  }

  Symbol* global = FindGlobalSymbol(name);
  if (inline_result.status != kInlineLookupUnique) {
    return global;
  }
  if (global == NULL) {
    return inline_result.symbol;
  }

  Symbol* inline_symbol = FollowAlias(inline_result.symbol);
  Symbol* global_symbol = FollowAlias(global);
  if (inline_symbol == global_symbol) {
    return global;
  }
  if (inline_symbol != NULL && global_symbol != NULL &&
      TypeIsFunction(inline_symbol->type) && TypeIsFunction(global_symbol->type)) {
    // Call resolution gathers both independently owned overload chains.
    return global;
  }
  if (ambiguous != NULL) {
    *ambiguous = true;
  }
  SyntaxError(syntax, "reference to '%s' is ambiguous", name->value);
  return NULL;
}

// Looks up tag symbol `name` at global scope, merging inline-namespace
// results with the global tag table.  Diagnoses ambiguity; returns NULL when
// ambiguous or not found.
static Symbol* ResolveGlobalTagCheckingAmbiguity(Syntax* syntax, String* name,
                                                 bool* ambiguous) {
  if (ambiguous != NULL) {
    *ambiguous = false;
  }
  NamespaceInlineTagLookup inline_result =
      NamespaceResolveTagInInlineSet(compiler->global_namespace, name);
  if (inline_result.status == kInlineLookupAmbiguous) {
    if (ambiguous != NULL) {
      *ambiguous = true;
    }
    SyntaxError(syntax, "reference to tag '%s' is ambiguous", name->value);
    return NULL;
  }

  Symbol* global = FindGlobalTag(name);
  if (inline_result.status != kInlineLookupUnique) {
    return global;
  }
  if (global == NULL ||
      FollowAlias(global) == FollowAlias(inline_result.tag)) {
    return global != NULL ? global : inline_result.tag;
  }
  if (ambiguous != NULL) {
    *ambiguous = true;
  }
  SyntaxError(syntax, "reference to tag '%s' is ambiguous", name->value);
  return NULL;
}

// Walks from namespace `ns` toward the global namespace and returns the first
// ordinary symbol named `name`, using global ambiguity rules at the root.
// Diagnoses ambiguity; returns NULL when not found.
Symbol* LookupSymbolInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
                                                       Namespace* ns,
                                                       String* name,
                                                       bool* ambiguous) {
  if (ambiguous != NULL) {
    *ambiguous = false;
  }
  while (ns != NULL) {
    if (ns == compiler->global_namespace) {
      return ResolveGlobalSymbolCheckingAmbiguity(syntax, name, ambiguous);
    }
    NamespaceInlineSymbolLookup result =
        NamespaceResolveSymbolInInlineSet(ns, name);
    if (result.status == kInlineLookupAmbiguous) {
      if (ambiguous != NULL) {
        *ambiguous = true;
      }
      SyntaxError(syntax, "reference to '%s' is ambiguous", name->value);
      return NULL;
    }
    if (result.status == kInlineLookupUnique) {
      return result.symbol;
    }
    ns = ns->parent;
  }
  return NULL;
}

// Walks from namespace `ns` toward the global namespace and returns the first
// tag symbol named `name`, using global ambiguity rules at the root.  Diagnoses
// ambiguity; returns NULL when not found.
Symbol* LookupTagInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
                                                    Namespace* ns,
                                                    String* name,
                                                    bool* ambiguous) {
  if (ambiguous != NULL) {
    *ambiguous = false;
  }
  while (ns != NULL) {
    if (ns == compiler->global_namespace) {
      return ResolveGlobalTagCheckingAmbiguity(syntax, name, ambiguous);
    }
    NamespaceInlineTagLookup result = NamespaceResolveTagInInlineSet(ns, name);
    if (result.status == kInlineLookupAmbiguous) {
      if (ambiguous != NULL) {
        *ambiguous = true;
      }
      SyntaxError(syntax, "reference to tag '%s' is ambiguous", name->value);
      return NULL;
    }
    if (result.status == kInlineLookupUnique) {
      return result.tag;
    }
    ns = ns->parent;
  }
  return NULL;
}

// Finds a namespace named `name` visible for qualified lookup before `::`,
// ignoring local variables and tags that would hide a namespace.  Returns NULL
// when no such namespace exists or lookup is ambiguous.
Namespace* FindNamespaceChildInScope(Syntax* syntax, String* name) {
  LocalSymbolTable* symbols = syntax->local_symbol_stack;
  LocalSymbolTable* tags = syntax->local_tag_stack;
  while (symbols != NULL) {
    Namespace* alias = FindDirectLocalNamespaceAlias(symbols, name);
    if (alias != NULL) {
      return alias;
    }
    // Lookup of a name before `::` considers only namespaces, types, and
    // templates ([basic.lookup.qual]): a parameter `color` does not hide
    // namespace `color` in `color::kNormal`.
    Symbol* local = FindSymbol(&symbols->table, name);
    if ((local != NULL && (StorageIs(local->storage, STO(typedef)) ||
                           local->flags.is_template ||
                           local->flags.is_template_type_parameter)) ||
        (tags != NULL && FindSymbol(&tags->table, name) != NULL)) {
      return NULL;
    }
    symbols = symbols->prev;
    if (tags != NULL) {
      tags = tags->prev;
    }
  }

  Namespace* ns = syntax->current_namespace != NULL ? syntax->current_namespace
                                                    : compiler->global_namespace;
  while (ns != NULL) {
    bool ambiguous = false;
    Namespace* child =
        ResolveNamespaceChildCheckingAmbiguity(syntax, ns, name, &ambiguous);
    if (child != NULL) {
      return child;
    }
    if (ambiguous) {
      return NULL;
    }
    ns = ns->parent;
  }
  return ResolveNamespaceChildCheckingAmbiguity(syntax,
                                                compiler->global_namespace,
                                                name, NULL);
}

// Returns true when the current token begins a nested-name-specifier (leading
// `::`, a visible namespace, a type name before `::`, or a template-id prefix
// ending in `::`).
bool SyntaxCurrentTokenStartsQualifiedName(Syntax* syntax) {
  if (LexLookingAt(syntax->lex, TOK(coloncolon))) {
    return true;
  }
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  if (FindNamespaceChildInScope(syntax, &syntax->lex->spelling) != NULL) {
    return true;
  }
  if (!CompilerIsCXX()) {
    return false;
  }

  // A class/struct/union/enum name followed by `::` begins a qualified name
  // too (e.g. `Clock::duration`, `Outer::Inner`).  Namespaces were handled
  // above; recognising a leading *type* name here lets member types accessed
  // through their enclosing class (rather than a namespace) be parsed as
  // qualified type names instead of a bare type followed by a stray `::`.
  {
    Symbol* tag = SyntaxFindTag(syntax, &syntax->lex->spelling);
    Symbol* sym = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
    bool names_type =
        (tag != NULL && tag->type != NULL && TypeIsStructOrUnion(tag->type)) ||
        (sym != NULL && StorageIs(sym->storage, STO(typedef)) &&
         sym->type != NULL &&
         (TypeIsStructOrUnion(sym->type) || TypeIsEnum(sym->type)));
    if (names_type) {
      LexCheckpoint type_checkpoint;
      LexCheckpointSave(syntax->lex, &type_checkpoint);
      LexNextToken(syntax->lex);
      bool followed_by_scope = LexLookingAt(syntax->lex, TOK(coloncolon));
      LexCheckpointRestore(syntax->lex, &type_checkpoint);
      LexCheckpointDestruct(&type_checkpoint);
      if (followed_by_scope) {
        return true;
      }
    }
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool has_template_qualified_prefix = false;
  while (LexLookingAt(syntax->lex, TOK(identifier))) {
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(less))) {
      int depth = 0;
      int paren_depth = 0;
      int square_depth = 0;
      int brace_depth = 0;
      Token previous = TOK(identifier);
      do {
        Token current = syntax->lex->current_token;
        if (current == TOK(lparen)) {
          paren_depth++;
        } else if (current == TOK(rparen)) {
          paren_depth--;
          // `buffer_ < buffer_end_)` is a comparison, not a template-id.
          // Scanning until a matching `>` would read the rest of the include
          // and pop that file out from under the saved checkpoint.
          if (paren_depth < 0) {
            break;
          }
        } else if (current == TOK(lsquare)) {
          square_depth++;
        } else if (current == TOK(rsquare)) {
          square_depth--;
          if (square_depth < 0) {
            break;
          }
        } else if (current == TOK(lbrace)) {
          brace_depth++;
        } else if (current == TOK(rbrace)) {
          brace_depth--;
          if (brace_depth < 0) {
            break;
          }
        } else if (current == TOK(semicolon) && paren_depth == 0 &&
                   square_depth == 0 && brace_depth == 0) {
          break;
        } else if (paren_depth == 0 && square_depth == 0 &&
                   brace_depth == 0) {
          if (current == TOK(less)) {
            if (previous == TOK(identifier) || previous == TOK(greater) ||
                previous == TOK(greatergreater)) {
              depth++;
            }
          } else {
            depth -= LexClosingAngleCount(current);
          }
        }
        LexNextToken(syntax->lex);
        previous = current;
      } while (!LexEof(syntax->lex) && depth > 0);
      if (depth == 0 && LexLookingAt(syntax->lex, TOK(coloncolon))) {
        has_template_qualified_prefix = true;
        break;
      }
    }
    if (!LexMatch(syntax->lex, TOK(coloncolon))) {
      break;
    }
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return has_template_qualified_prefix;
}

// Returns the namespace named by all but the last component of `name`, or the
// global namespace for `::foo`.  Returns NULL when the prefix is unqualified
// with a single component or does not name a namespace path.
static Namespace* ResolveQualifiedNamespace(Syntax* syntax,
                                            FullyQualifiedIdentifier* name) {
  if (name->components.length <= 1 && !name->absolute) {
    return NULL;
  }

  size_t namespace_components =
      name->components.length == 0 ? 0 : name->components.length - 1;
  if (namespace_components == 0) {
    return compiler->global_namespace;
  }

  String* first = name->components.value.p[0];
  Namespace* ns = name->absolute
      ? ResolveNamespaceChildCheckingAmbiguity(syntax,
                                               compiler->global_namespace,
                                               first, NULL)
      : FindNamespaceChildInScope(syntax, first);
  for (size_t i = 1; ns != NULL && i < namespace_components; i++) {
    ns = ResolveNamespaceChildCheckingAmbiguity(
        syntax, ns, name->components.value.p[i], NULL);
  }
  return ns;
}

// Resolves `name` as a namespace (every component, not just the prefix).
// Returns NULL when `name` is empty or the path does not name a namespace.
Namespace* SyntaxFindQualifiedNamespace(Syntax* syntax,
                                        FullyQualifiedIdentifier* name) {
  if (name->components.length == 0) {
    return NULL;
  }

  String* first = name->components.value.p[0];
  Namespace* ns = name->absolute
      ? ResolveNamespaceChildCheckingAmbiguity(syntax,
                                               compiler->global_namespace,
                                               first, NULL)
      : FindNamespaceChildInScope(syntax, first);
  for (size_t i = 1; ns != NULL && i < name->components.length; i++) {
    ns = ResolveNamespaceChildCheckingAmbiguity(
        syntax, ns, name->components.value.p[i], NULL);
  }
  return ns;
}

// `using streambuf = basic_streambuf<char>` can still name the forward
// declaration.  Nested lookup (`streambuf::traits_type::eq_int_type`) has to
// see the completed specialization, which is where the members are declared.
static Symbol* QualifiedClassAfterMaterialize(Syntax* syntax, Symbol* symbol) {
  if (syntax == NULL || symbol == NULL || symbol->type == NULL) {
    return symbol;
  }
  TypeRecord* materialized =
      TypeMaterializeClassTemplateSpecialization(syntax, symbol->type);
  if (materialized == NULL || materialized == symbol->type) {
    return symbol;
  }
  Symbol* tag = NULL;
  if (TypeIsStructOrUnion(materialized) &&
      materialized->info.struct_info != NULL) {
    tag = materialized->info.struct_info->tag_symbol;
  }
  TypeRecordDelete(materialized);
  if (tag == NULL || tag->type == NULL) {
    return symbol;
  }
  return tag;
}

// Returns the tag symbol of a direct base class of the current class scope
// whose name matches `base_name`, or NULL when not in class scope or no base
// matches.
static Symbol* FindEnclosingBaseClassTagSymbol(Syntax* syntax, String* base_name) {
  if (!CompilerIsCXX() || base_name == NULL) {
    return NULL;
  }
  Struct* scope = syntax->cxx_class_head;
  if (scope == NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    scope = compiler->current_function->info.function.cxx_member_owner;
  }
  for (; scope != NULL; scope = scope->lexical_parent) {
    for (size_t i = 0; i < scope->bases.length; i++) {
      CXXBaseSpecifier* base = scope->bases.value.p[i];
      if (base == NULL || base->type == NULL ||
          !TypeIsStructOrUnion(base->type) ||
          base->type->info.struct_info == NULL ||
          base->type->info.struct_info->tag_name == NULL) {
        continue;
      }
      if (StringEqual(base_name, base->type->info.struct_info->tag_name->value)) {
        return base->type->info.struct_info->tag_symbol;
      }
    }
  }
  return NULL;
}

// Returns true when unqualified lookup finds a template type or template
// template parameter named `name`.
static bool NameIsTemplateParameter(Syntax* syntax, String* name) {
  Symbol* symbol = SyntaxFindSymbol(syntax, name);
  return symbol != NULL && (symbol->flags.is_template_type_parameter ||
                            symbol->flags.is_template_template_parameter);
}

// Looks up the last component `last` of qualified name `name` in the
// namespace its qualifier names (and that namespace's inline namespaces),
// diagnosing an ambiguous name.  Returns NULL when the qualifier is not a
// namespace or the name is not found there.
static Symbol* FindQualifiedNamespaceMember(Syntax* syntax,
                                            FullyQualifiedIdentifier* name,
                                            String* last) {
  Namespace* ns = ResolveQualifiedNamespace(syntax, name);
  if (ns != NULL && ns != compiler->global_namespace) {
    NamespaceInlineSymbolLookup result =
        NamespaceResolveSymbolInInlineSet(ns, last);
    if (result.status == kInlineLookupAmbiguous) {
      SyntaxError(syntax, "reference to '%s' is ambiguous", last->value);
    } else if (result.status == kInlineLookupUnique) {
      return result.symbol;
    }
  } else if (ns == compiler->global_namespace) {
    return ResolveGlobalSymbolCheckingAmbiguity(syntax, last, NULL);
  }
  return NULL;
}

// Finds `Enum::last` when the qualifier of `name` is an enumeration tag.
// C++ allows `Enum::enumerator` for unscoped enumerations as well as scoped
// ones ([dcl.enum]); C only has the scoped-enum form.  Returns NULL
// otherwise.
static Symbol* FindQualifiedEnumeratorOfTag(Syntax* syntax,
                                            FullyQualifiedIdentifier* name,
                                            String* last) {
  FullyQualifiedIdentifier prefix;
  FullyQualifiedIdentifierInit(&prefix);
  prefix.absolute = name->absolute;
  prefix.is_qualified = name->absolute || name->components.length > 2;
  for (size_t i = 0; i + 1 < name->components.length; i++) {
    FullyQualifiedIdentifierAppend(&prefix, name->components.value.p[i]);
  }
  Symbol* tag = SyntaxFindQualifiedTag(syntax, &prefix);
  Symbol* symbol = NULL;
  if (tag != NULL && tag->type != NULL && TypeIsEnum(tag->type) &&
      (CompilerIsCXX() || TypeIsScopedEnum(tag->type))) {
    symbol = EnumFindConstant(tag->type->info.enum_info, last);
  }
  FullyQualifiedIdentifierDestruct(&prefix);
  return symbol;
}

// Returns `owner` when it names a class or enumeration, else NULL.
static Symbol* ClassOrEnumOwner(Symbol* owner) {
  if (owner != NULL &&
      (owner->type == NULL ||
       (!TypeIsStructOrUnion(owner->type) && !TypeIsEnum(owner->type)))) {
    return NULL;
  }
  return owner;
}

// Resolves the class or enumeration that the qualifier of `name` names:
// through qualified-prefix lookup, as an enclosing class's base, or (for a
// single unqualified qualifier hidden by a class member) directly in the
// enclosing namespace scopes.  Returns NULL when it names neither.
static Symbol* FindQualifiedNameOwner(Syntax* syntax,
                                      FullyQualifiedIdentifier* name) {
  Symbol* owner =
      SyntaxFindQualifiedPrefixSymbolImpl(
          syntax, name, name->components.length - 1,
          /*allow_dependent_template_args=*/true);
  owner = ClassOrEnumOwner(QualifiedClassAfterMaterialize(syntax, owner));
  if (owner == NULL && name->components.length == 2 && !name->absolute) {
    owner = FindEnclosingBaseClassTagSymbol(syntax, name->components.value.p[0]);
    owner = QualifiedClassAfterMaterialize(syntax, owner);
  }
  if (owner == NULL && name->components.length == 2 && !name->absolute &&
      !NameIsTemplateParameter(syntax, name->components.value.p[0])) {
    // A class member function can hide a namespace-scope class used as the
    // first component of a nested type (`ExtensionSet::Prefetch`).  Resolve
    // that qualifier directly in namespace tag/ordinary tables, without
    // repeating class-member lookup.  A template parameter hides it too
    // (`typename Types::Head` with a parameter named like class `Types`).
    Namespace* ns = syntax->current_namespace != NULL
                        ? syntax->current_namespace
                        : compiler->global_namespace;
    bool ambiguous = false;
    owner = LookupTagInEnclosingScopesCheckingAmbiguity(
        syntax, ns, name->components.value.p[0], &ambiguous);
    if (owner == NULL && !ambiguous) {
      owner = LookupSymbolInEnclosingScopesCheckingAmbiguity(
          syntax, ns, name->components.value.p[0], &ambiguous);
    }
    owner = QualifiedClassAfterMaterialize(syntax, ClassOrEnumOwner(owner));
  }
  return owner;
}

// Looks up member `last` in the enclosing class being defined whose name
// matches `owner`.  `HashEq::ToPtr` inside `HashEq<T*>` names the
// specialization being defined.  Ordinary lookup finds the primary template,
// which does not have the member; the enclosing class with the same name
// does.  Updates `*owner` to that class and returns the member, or NULL.
static StructMember* FindMemberOfEnclosingSpecialization(
    Syntax* syntax, FullyQualifiedIdentifier* name, Symbol** owner,
    String* last) {
  StructMember* member = NULL;
  String prefix_name;
  StringInit(&prefix_name, (*owner)->name.value);
  Vector* prefix_args =
      name->template_arguments.length == name->components.length
          ? name->template_arguments.value.p[name->components.length - 2]
          : NULL;
  Struct* scope = syntax->cxx_class_head;
  if (scope == NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    scope = compiler->current_function->info.function.cxx_member_owner;
  }
  for (; scope != NULL && member == NULL;
       scope = scope->lexical_parent) {
    if (!CurrentClassNameMatchesTypeName(scope, &prefix_name)) {
      continue;
    }
    // `W<char>::type` inside `W<T>` is another specialization's member.
    if (EnclosingClassTemplateForOtherSpecialization(scope,
                                                     prefix_args) !=
        NULL) {
      break;
    }
    member = FindStructMember(scope, last);
    if (member != NULL) {
      *owner = scope->tag_symbol != NULL ? scope->tag_symbol : *owner;
    }
  }
  StringDestruct(&prefix_name);
  return member;
}

// Looks up `last` as a member of class `owner` named by a qualified name
// (`C::m`).  Non-static member functions are not found, except that a
// static overload of a non-static member is.  Returns NULL when there is no
// such member.
static Symbol* FindQualifiedClassMember(Syntax* syntax,
                                        FullyQualifiedIdentifier* name,
                                        Symbol* owner, String* last) {
  StructMember* member =
      FindStructMember(owner->type->info.struct_info, last);
  if (member == NULL && owner->name.value != NULL) {
    member = FindMemberOfEnclosingSpecialization(syntax, name, &owner, last);
  }
  // `Box::AddData` names the overload set.  The first declaration may be
  // the non-static member template; a later static overload is still a
  // valid qualified call (`Box::AddData<kFront>(tree, data)`).
  if (member != NULL && !member->is_static) {
    for (StructMember* overload = member->overload_next; overload != NULL;
         overload = overload->overload_next) {
      if (overload->is_static && overload->symbol != NULL) {
        member = overload;
        break;
      }
    }
  }
  if (MemberCanBeQualifiedPrefix(member, owner->type->info.struct_info)) {
    return member->symbol;
  }
  return NULL;
}

// Resolves a possibly qualified name to the symbol it denotes: a namespace
// member, an enumerator (`E::x`), or a static member, member typedef,
// enumerator or nested class of a class.  Returns NULL when not found.
Symbol* SyntaxFindQualifiedSymbol(Syntax* syntax,
                                  FullyQualifiedIdentifier* name) {
  if (!name->is_qualified && name->components.length == 1) {
    String* simple = name->components.value.p[0];
    return FollowAlias(SyntaxFindSymbol(syntax, simple));
  }

  const char* last_name = FullyQualifiedIdentifierLast(name);
  String last;
  StringInit(&last, last_name);
  Symbol* symbol = FindQualifiedNamespaceMember(syntax, name, &last);
  if (symbol == NULL && name->components.length >= 2) {
    symbol = FindQualifiedEnumeratorOfTag(syntax, name, &last);
  }
  if (symbol == NULL && name->components.length >= 2) {
    Symbol* owner = FindQualifiedNameOwner(syntax, name);
    if (owner != NULL && owner->type != NULL && TypeIsEnum(owner->type) &&
        owner->type->info.enum_info != NULL &&
        (CompilerIsCXX() || owner->type->info.enum_info->is_scoped)) {
      // `using MethodIdentifier = Tracker::Method;` then
      // `MethodIdentifier::kUnknown`.  The prefix is a typedef, not a tag.
      symbol = EnumFindConstant(owner->type->info.enum_info, &last);
    }
    if (symbol == NULL && owner != NULL && owner->type != NULL &&
        TypeIsStructOrUnion(owner->type) &&
        owner->type->info.struct_info != NULL) {
      symbol = FindQualifiedClassMember(syntax, name, owner, &last);
    }
  }
  StringDestruct(&last);
  return FollowAlias(symbol);
}

// Non-static member functions are omitted by SyntaxFindQualifiedSymbol.  The
// overload set can still contain a function template (`operator<<(char)` next
// to `template <typename T> operator<<(const T&)`).
static bool QualifiedNameHasMemberTemplate(Syntax* syntax,
                                           FullyQualifiedIdentifier* name) {
  if (syntax == NULL || name == NULL || name->components.length < 2) {
    return false;
  }
  Symbol* owner = SyntaxFindQualifiedPrefixSymbolImpl(
      syntax, name, name->components.length - 1,
      /*allow_dependent_template_args=*/true);
  owner = QualifiedClassAfterMaterialize(syntax, owner);
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    return false;
  }
  const char* last_name = FullyQualifiedIdentifierLast(name);
  String last;
  StringInit(&last, last_name);
  StructMember* member =
      FindStructMember(owner->type->info.struct_info, &last);
  StringDestruct(&last);
  if (member == NULL || member->symbol == NULL) {
    return false;
  }
  return SymbolOrOverloadIsTemplate(member->symbol);
}

// Returns true when template argument `arg` is dependent (unresolved
// expression, parameter reference, or dependent type).
static bool TemplateArgumentIsDependent(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->dependent_expr != NULL) {
    return true;
  }
  if (arg->kind == kTemplateParameterNonType ||
      arg->kind == kTemplateParameterTemplate) {
    return arg->template_parameter_index >= 0;
  }
  return TypeContainsTemplateParameterReference(arg->type);
}

// Returns true when any element of `args` is dependent.  Returns false when
// `args` is NULL.
bool TemplateArgumentVectorIsDependent(Vector* args) {
  if (args == NULL) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    if (TemplateArgumentIsDependent(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

// Returns true when the type chain `type` mentions a template parameter,
// dependent decltype or splice, pack index, dependent array bound, or
// dependent nested template arguments.
static bool TypeContainsTemplateParameterReference(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= 0) {
      return true;
    }
    if (TypeDependentOf(t)->dependent_decltype_expr != NULL ||
        TypeDependentOf(t)->dependent_splice_expr != NULL || t->is_pack_index) {
      return true;
    }
    if (TypeIsArray(t) && t->info.array.template_parameter_index >= 0) {
      return true;
    }
    if (TypeIsArray(t) && t->info.array.is_dependent_bound) {
      return true;
    }
    if (TemplateArgumentVectorIsDependent(t->template_arguments)) {
      return true;
    }
  }
  return false;
}

// Finds the injected-class-name symbol for `name` in `owner` or an inherited
// base, synthesizing an invented typedef alias for a dependent instantiated
// base when needed.  Returns NULL when no match exists.
static Symbol* FindInjectedClassNameSymbol(Struct* owner, String* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  if (CurrentClassNameMatchesTypeName(owner, name) &&
      owner->tag_symbol != NULL) {
    return owner->tag_symbol;
  }
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base == NULL || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Symbol* found =
        FindInjectedClassNameSymbol(base->type->info.struct_info, name);
    // The injected-class-name of a dependent base (`Spy<A>::HashStateBase`
    // naming `HashStateBase<Spy<T>>`) is that specialization, not the
    // template's pattern, whose tag the uninstantiated base still points at.
    // Returning an invented alias of the base type lets the parser treat
    // `::combine` as a dependent member of `HashStateBase<Spy<T>>`.
    if (found != NULL &&
        found == base->type->info.struct_info->tag_symbol &&
        base->type->info.struct_info->is_template &&
        base->type->template_arguments != NULL) {
      Symbol* alias =
          NewSymbol(name->value, TypeRecordCopy(base->type), STO(typedef));
      alias->flags.invented = true;
      return alias;
    }
    if (found != NULL) {
      return found;
    }
  }
  return NULL;
}

// `DefaultValue<T&>::address_` names the partial specialization, not the
// primary template.  A dependent argument list that matches exactly one
// partial-specialization pattern selects that specialization.
static Symbol* ClassTemplatePartialSpecializationTag(Symbol* primary,
                                                    Vector* template_args) {
  return ClassTemplatePartialSpecializationTagForArguments(primary,
                                                           template_args);
}

// Returns true when `symbol` is a class (or alias) template that the
// non-dependent template-id arguments `template_args` instantiate.
static bool QualifiedPrefixInstantiatesTemplate(Symbol* symbol,
                                                Vector* template_args) {
  return template_args != NULL &&
         !TemplateArgumentVectorIsDependent(template_args) &&
         symbol != NULL && symbol->flags.is_template &&
         symbol->type != NULL &&
         (TypeIsStructOrUnion(symbol->type) ||
          StorageIs(symbol->storage, STO(typedef)));
}

// Instantiates class (or alias) template `symbol` with `template_args` and
// returns the tag symbol of the resulting class, or NULL when it is not a
// class.
static Symbol* InstantiateQualifiedPrefixTag(Syntax* syntax, Symbol* symbol,
                                             Vector* template_args) {
  TypeRecord* type = TypeInstantiateClassTemplate(syntax, symbol,
                                                  template_args);
  Symbol* tag = type != NULL && TypeIsStructOrUnion(type) &&
                        type->info.struct_info != NULL
      ? type->info.struct_info->tag_symbol
      : NULL;
  TypeRecordDelete(type);
  return tag;
}

// For a typedef naming a dependent member type of a class template
// specialization (`typename X<A>::type`), instantiates the specialization and
// returns its member typedef.  Returns NULL when `symbol` is not such a
// typedef or the member is not a typedef.
static Symbol* DependentMemberTypedefOfPrefix(Syntax* syntax, Symbol* symbol) {
  if (symbol == NULL || !StorageIs(symbol->storage, STO(typedef)) ||
      symbol->type == NULL || symbol->type->template_origin == NULL ||
      TypeDependentOf(symbol->type)->dependent_member_name == NULL) {
    return NULL;
  }
  TypeRecord* owner =
      TypeInstantiateClassTemplate(syntax, symbol->type->template_origin,
                                   symbol->type->template_arguments);
  Symbol* nested = NULL;
  if (owner != NULL && TypeIsStructOrUnion(owner) &&
      owner->info.struct_info != NULL) {
    StructMember* member =
        FindStructMember(owner->info.struct_info,
                         TypeDependentOf(symbol->type)->dependent_member_name);
    if (member != NULL && member->symbol != NULL &&
        StorageIs(member->symbol->storage, STO(typedef))) {
      nested = member->symbol;
    }
  }
  TypeRecordDelete(owner);
  return nested;
}

// Resolves the first, unqualified component of a qualified-name prefix (with
// its template arguments `template_args`, if any) by ordinary lookup,
// instantiating a non-dependent template-id or selecting the partial
// specialization a dependent one matches.
static Symbol* FindUnqualifiedQualifiedPrefix(Syntax* syntax,
                                              FullyQualifiedIdentifier* name,
                                              Vector* template_args,
                                              bool allow_dependent_template_args) {
  Symbol* symbol =
      FollowAlias(SyntaxFindSymbol(syntax, name->components.value.p[0]));
  // The specialization's injected-class-name hides the primary.  A
  // template-id on that name (`numeric_limits<double>::infinity`) still
  // instantiates the primary.
  if (template_args != NULL && symbol != NULL && !symbol->flags.is_template &&
      !symbol->flags.is_using_alias &&
      SymbolExtraOf(symbol)->alias_target != NULL &&
      SymbolExtraOf(symbol)->alias_target->flags.is_template) {
    symbol = SymbolExtraOf(symbol)->alias_target;
  }
  if (allow_dependent_template_args) {
    Symbol* nested = DependentMemberTypedefOfPrefix(syntax, symbol);
    if (nested != NULL) {
      return nested;
    }
  }
  if (QualifiedPrefixInstantiatesTemplate(symbol, template_args)) {
    return InstantiateQualifiedPrefixTag(syntax, symbol, template_args);
  }
  Symbol* partial =
      ClassTemplatePartialSpecializationTag(symbol, template_args);
  if (partial != NULL) {
    return partial;
  }
  return symbol;
}

// Returns true when class member `member` of `parent_struct` can be a
// qualified-name prefix component or the named entity: a static member, a
// member typedef, a member with a value, or a nested class.
static bool MemberCanBeQualifiedPrefix(StructMember* member,
                                       Struct* parent_struct) {
  return member != NULL && member->symbol != NULL &&
         (member->is_static ||
          StorageIs(member->symbol->storage, STO(typedef)) ||
          member->symbol->flags.value_set ||
          (member->symbol->type != NULL &&
           TypeIsStructOrUnion(member->symbol->type) &&
           member->symbol->type->info.struct_info != NULL &&
           member->symbol->type->info.struct_info->lexical_parent ==
               parent_struct));
}

// Looks up `member_name` (with template arguments `template_args`) as a
// member, or the injected-class-name of a base, of class `parent`.  A member
// function inherited from a base is retargeted to `parent`, and a member
// class template named with non-dependent arguments is instantiated.
// Returns NULL when the class has no such member.
static Symbol* FindQualifiedPrefixClassMember(Syntax* syntax, Symbol* parent,
                                              String* member_name,
                                              Vector* template_args) {
  Struct* parent_struct = parent->type->info.struct_info;
  StructMember* member = FindStructMember(parent_struct, member_name);
  if (MemberCanBeQualifiedPrefix(member, parent_struct)) {
    if (member->symbol->type != NULL &&
        TypeIsFunction(member->symbol->type) &&
        !CXXRetargetDropsDeclaringTemplateArguments(
            member->symbol->type->info.function.cxx_member_owner,
            parent_struct)) {
      member->symbol->type->info.function.cxx_member_owner = parent_struct;
      StringClear(&member->symbol->asm_name);
      SymbolSetCXXMangledAsmName(member->symbol);
    }
    Symbol* found = FollowAlias(member->symbol);
    // `Base<C>::Alias<U>::value` must instantiate the member alias before
    // looking up `value`.  Returning the alias template itself looks the
    // member up on the unsubstituted pattern (the primary template).
    if (QualifiedPrefixInstantiatesTemplate(found, template_args)) {
      Symbol* tag = InstantiateQualifiedPrefixTag(syntax, found, template_args);
      if (tag != NULL) {
        return tag;
      }
    }
    return found;
  }
  // `Derived::Base` names the injected-class-name of an inherited base
  // (`using State::HashStateBase::combine_contiguous`).
  Symbol* injected = FindInjectedClassNameSymbol(parent_struct, member_name);
  if (injected != NULL) {
    return FollowAlias(injected);
  }
  return NULL;
}

// Resolves the first `namespace_components` components of `name` as a
// namespace path (the global namespace for none).  Returns NULL when they do
// not name a namespace.
static Namespace* ResolveQualifiedPrefixNamespace(Syntax* syntax,
                                                  FullyQualifiedIdentifier* name,
                                                  size_t namespace_components) {
  if (namespace_components == 0) {
    return compiler->global_namespace;
  }
  String* first = name->components.value.p[0];
  Namespace* ns = name->absolute
      ? ResolveNamespaceChildCheckingAmbiguity(syntax,
                                               compiler->global_namespace,
                                               first, NULL)
      : FindNamespaceChildInScope(syntax, first);
  for (size_t i = 1; ns != NULL && i < namespace_components; i++) {
    ns = ResolveNamespaceChildCheckingAmbiguity(
        syntax, ns, name->components.value.p[i], NULL);
  }
  return ns;
}

// Looks up `last` in namespace `ns` (and its inline namespaces), diagnosing
// an ambiguous name.  Returns NULL when it is not found.
static Symbol* FindQualifiedPrefixNamespaceMember(Syntax* syntax,
                                                  Namespace* ns,
                                                  String* last) {
  if (ns == compiler->global_namespace) {
    return ResolveGlobalSymbolCheckingAmbiguity(syntax, last, NULL);
  }
  NamespaceInlineSymbolLookup result =
      NamespaceResolveSymbolInInlineSet(ns, last);
  if (result.status == kInlineLookupAmbiguous) {
    SyntaxError(syntax, "reference to '%s' is ambiguous", last->value);
  } else if (result.status == kInlineLookupUnique) {
    return result.symbol;
  }
  return NULL;
}

// Resolves the first `component_count` components of qualified name `name`
// to the symbol they denote (a namespace member, class, class member or
// template specialization).  With `allow_dependent_template_args`, a class
// prefix is resolved first, so that members of dependent template-ids are
// found in the primary template.  Returns NULL when the prefix names nothing.
static Symbol* SyntaxFindQualifiedPrefixSymbolImpl(
    Syntax* syntax, FullyQualifiedIdentifier* name, size_t component_count,
    bool allow_dependent_template_args) {
  if (component_count == 0 || component_count > name->components.length) {
    return NULL;
  }
  if (component_count == name->components.length) {
    return SyntaxFindQualifiedSymbol(syntax, name);
  }

  Vector* template_args = NULL;
  if (component_count <= name->template_arguments.length) {
    template_args = name->template_arguments.value.p[component_count - 1];
  }

  if (component_count == 1 && !name->absolute) {
    return FindUnqualifiedQualifiedPrefix(syntax, name, template_args,
                                          allow_dependent_template_args);
  }

  Symbol* parent = allow_dependent_template_args
      ? SyntaxFindQualifiedPrefixSymbolImpl(
            syntax, name, component_count - 1, allow_dependent_template_args)
      : NULL;
  parent = QualifiedClassAfterMaterialize(syntax, parent);
  if (allow_dependent_template_args) {
    Symbol* nested = DependentMemberTypedefOfPrefix(syntax, parent);
    if (nested != NULL) {
      parent = nested;
    }
  }
  if (parent != NULL && parent->type != NULL &&
      TypeIsStructOrUnion(parent->type) &&
      parent->type->info.struct_info != NULL) {
    Symbol* member = FindQualifiedPrefixClassMember(
        syntax, parent, name->components.value.p[component_count - 1],
        template_args);
    if (member != NULL) {
      return member;
    }
  }

  Namespace* ns =
      ResolveQualifiedPrefixNamespace(syntax, name, component_count - 1);
  if (ns == NULL) {
    return NULL;
  }
  String* last = name->components.value.p[component_count - 1];
  Symbol* symbol =
      FollowAlias(FindQualifiedPrefixNamespaceMember(syntax, ns, last));
  if (QualifiedPrefixInstantiatesTemplate(symbol, template_args)) {
    return InstantiateQualifiedPrefixTag(syntax, symbol, template_args);
  }
  Symbol* partial = ClassTemplatePartialSpecializationTag(symbol, template_args);
  if (partial != NULL) {
    return partial;
  }
  return FollowAlias(symbol);
}

// Resolves the first `component_count` components of qualified name `name` to
// the symbol they denote, including dependent template-ids on class prefixes.
Symbol* SyntaxFindQualifiedPrefixSymbol(Syntax* syntax,
                                        FullyQualifiedIdentifier* name,
                                        size_t component_count) {
  // Dependent template-ids (`FixedArray<T, N, A>::Nested::method`) still
  // need the primary class as the prefix so nested types resolve.  The
  // implementation does not instantiate a dependent argument list; it
  // walks members of the primary template instead.
  return SyntaxFindQualifiedPrefixSymbolImpl(
      syntax, name, component_count,
      /*allow_dependent_template_args=*/true);
}

// Resolves qualified name `name` to a class, enumeration, or typedef tag
// symbol, including nested types and injected-class names.  Returns NULL when
// not found.
Symbol* SyntaxFindQualifiedTag(Syntax* syntax,
                               FullyQualifiedIdentifier* name) {
  if (!name->is_qualified && name->components.length == 1) {
    String* simple = name->components.value.p[0];
    return FollowAlias(SyntaxFindTag(syntax, simple));
  }

  const char* last_name = FullyQualifiedIdentifierLast(name);
  String last;
  StringInit(&last, last_name);
  Symbol* symbol = NULL;
  Namespace* ns = ResolveQualifiedNamespace(syntax, name);
  if (ns != NULL && ns != compiler->global_namespace) {
    NamespaceInlineTagLookup result =
        NamespaceResolveTagInInlineSet(ns, &last);
    if (result.status == kInlineLookupAmbiguous) {
      SyntaxError(syntax, "reference to tag '%s' is ambiguous", last.value);
    } else if (result.status == kInlineLookupUnique) {
      symbol = result.tag;
    }
  } else if (ns == compiler->global_namespace) {
    symbol = ResolveGlobalTagCheckingAmbiguity(syntax, &last, NULL);
  }
  // `Outer::Inner` names a nested class, not a namespace member.  Namespace
  // lookup above misses it; resolve the prefix as a class and look up Inner
  // among its nested types (stored as typedef members).
  if (symbol == NULL && name->components.length >= 2) {
    Symbol* owner = SyntaxFindQualifiedPrefixSymbolImpl(
        syntax, name, name->components.length - 1,
        /*allow_dependent_template_args=*/true);
    if (owner == NULL) {
      // Some class declarations have no ordinary injected-name symbol (for
      // example after an attribute-bearing nested class head), but their tag
      // still provides a valid qualifier.
      FullyQualifiedIdentifier prefix;
      FullyQualifiedIdentifierInit(&prefix);
      prefix.absolute = name->absolute;
      prefix.is_qualified = name->absolute || name->components.length > 2;
      for (size_t i = 0; i + 1 < name->components.length; i++) {
        FullyQualifiedIdentifierAppend(&prefix, name->components.value.p[i]);
      }
      owner = SyntaxFindQualifiedTag(syntax, &prefix);
      if (owner != NULL &&
          (owner->type == NULL ||
           (!TypeIsStructOrUnion(owner->type) &&
            !TypeIsEnum(owner->type)))) {
        owner = NULL;
      }
      // Inside a class, a member function can hide a namespace-scope class
      // with the same ordinary name.  Tag lookup may not find that class
      // because C++ class names are also represented in the namespace's
      // ordinary-symbol table.  Retry the one-component qualifier directly
      // in enclosing namespaces, bypassing class-member lookup.
      if (owner == NULL && !prefix.absolute &&
          prefix.components.length == 1) {
        Namespace* ns = syntax->current_namespace != NULL
                            ? syntax->current_namespace
                            : compiler->global_namespace;
        bool ambiguous = false;
        owner = LookupTagInEnclosingScopesCheckingAmbiguity(
            syntax, ns, prefix.components.value.p[0], &ambiguous);
        if (owner == NULL && !ambiguous) {
          owner = LookupSymbolInEnclosingScopesCheckingAmbiguity(
              syntax, ns, prefix.components.value.p[0], &ambiguous);
        }
        if (owner != NULL &&
            (owner->type == NULL ||
             (!TypeIsStructOrUnion(owner->type) &&
              !TypeIsEnum(owner->type)))) {
          owner = NULL;
        }
      }
      FullyQualifiedIdentifierDestruct(&prefix);
    }
    if (owner != NULL && owner->type != NULL &&
        TypeIsStructOrUnion(owner->type) &&
        owner->type->info.struct_info != NULL) {
      StructMember* member =
          FindStructMember(owner->type->info.struct_info, &last);
      if (member != NULL && member->symbol != NULL &&
          StorageIs(member->symbol->storage, STO(typedef)) &&
          member->symbol->type != NULL) {
        TypeRecord* nested = member->symbol->type;
        if (TypeIsStructOrUnion(nested) && nested->info.struct_info != NULL &&
            nested->info.struct_info->tag_symbol != NULL) {
          symbol = nested->info.struct_info->tag_symbol;
        } else if (TypeIsEnum(nested) && nested->info.enum_info != NULL &&
                   nested->info.enum_info->tag_symbol != NULL) {
          symbol = nested->info.enum_info->tag_symbol;
        }
      }
      if (symbol == NULL) {
        symbol = FindInjectedClassNameSymbol(owner->type->info.struct_info,
                                             &last);
      }
      if (symbol == NULL) {
        // A nested class is not inserted into its enclosing member table until
        // its definition closes.  While parsing that definition, a qualified
        // self-reference such as `friend class Outer::Inner;` must still name
        // the current class.
        for (Struct* current = syntax->cxx_class_head;
             current != NULL && symbol == NULL;
             current = current->lexical_parent) {
          Struct* parent = current->lexical_parent;
          bool parent_matches =
              parent == owner->type->info.struct_info ||
              (parent != NULL && parent->tag_symbol != NULL &&
               (parent->tag_symbol == owner ||
                TypeEqual(parent->tag_symbol->type, owner->type)));
          if (parent_matches &&
              CurrentClassNameMatchesTypeName(current, &last)) {
            symbol = current->tag_symbol;
          }
        }
      }
    }
  }
  StringDestruct(&last);
  return FollowAlias(symbol);
}
