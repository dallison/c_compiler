//
//  syntax.c
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdlib.h>

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type.h"
#include "errors.h"
#include "compiler.h"

jmp_buf error_abort_state;       // Where to abort to.
bool abort_on_error;

static int next_pc_label_id = 0;
static String next_pc_label;

static bool InNamedNamespace(Syntax* syntax) {
  return syntax->current_namespace != NULL &&
         syntax->current_namespace != compiler->global_namespace;
}

static bool CurrentIdentifierFollowedByScopeOperator(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (!LexLookingAt(lex, TOK(identifier))) {
    return false;
  }

  size_t pos = lex->pos;
  while (pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[pos])) {
    pos++;
  }
  if (CompilerIsCXX() && pos < lex->line.length && lex->line.value[pos] == '<') {
    int depth = 1;
    pos++;
    while (pos < lex->line.length && depth > 0) {
      if (lex->line.value[pos] == '<') {
        depth++;
      } else if (lex->line.value[pos] == '>') {
        depth--;
      }
      pos++;
    }
    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
  }
  return pos + 1 < lex->line.length &&
         lex->line.value[pos] == ':' &&
         lex->line.value[pos + 1] == ':';
}

static bool ReadLookaheadIdentifier(Lex* lex, size_t* pos, String* out) {
  while (*pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[*pos])) {
    (*pos)++;
  }
  size_t i = 0;
  while (*pos < lex->line.length) {
    char ch = lex->line.value[*pos];
    if (!(isalnum((unsigned char)ch) || ch == '_')) {
      break;
    }
    StringAppendChar(out, ch);
    i++;
    (*pos)++;
  }
  return i != 0;
}

static void SkipLookaheadTemplateId(Lex* lex, size_t* pos) {
  while (*pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[*pos])) {
    (*pos)++;
  }
  if (*pos >= lex->line.length || lex->line.value[*pos] != '<') {
    return;
  }
  int depth = 1;
  (*pos)++;
  while (*pos < lex->line.length && depth > 0) {
    if (lex->line.value[*pos] == '<') {
      depth++;
    } else if (lex->line.value[*pos] == '>') {
      depth--;
    }
    (*pos)++;
  }
  while (*pos < lex->line.length &&
         isspace((unsigned char)lex->line.value[*pos])) {
    (*pos)++;
  }
}

static bool CurrentLineLooksLikeSpecialMemberDefinition(Syntax* syntax) {
  Lex* lex = syntax->lex;
  String previous;
  StringInit(&previous, NULL);
  String current;
  StringInit(&current, NULL);
  size_t pos = lex->pos;
  bool result = false;

  if (LexLookingAt(lex, TOK(identifier))) {
    StringSetString(&previous, &lex->spelling);
    SkipLookaheadTemplateId(lex, &pos);
  } else if (LexLookingAt(lex, TOK(coloncolon))) {
    if (!ReadLookaheadIdentifier(lex, &pos, &previous)) {
      goto done;
    }
    SkipLookaheadTemplateId(lex, &pos);
  } else {
    goto done;
  }

  while (true) {
    StringClear(&current);
    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
    if (pos + 1 >= lex->line.length ||
        lex->line.value[pos] != ':' ||
        lex->line.value[pos + 1] != ':') {
      goto done;
    }
    pos += 2;

    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
    bool is_destructor = pos < lex->line.length && lex->line.value[pos] == '~';
    if (is_destructor) {
      pos++;
      StringAppendChar(&current, '~');
      if (!ReadLookaheadIdentifier(lex, &pos, &current)) {
        goto done;
      }
    } else if (!ReadLookaheadIdentifier(lex, &pos, &current)) {
      goto done;
    }
    SkipLookaheadTemplateId(lex, &pos);

    while (pos < lex->line.length &&
           isspace((unsigned char)lex->line.value[pos])) {
      pos++;
    }
    if (pos + 1 < lex->line.length &&
        lex->line.value[pos] == ':' &&
        lex->line.value[pos + 1] == ':') {
      StringSetString(&previous, &current);
      continue;
    }

    if (pos >= lex->line.length || lex->line.value[pos] != '(') {
      goto done;
    }
    if (current.length > 0 && current.value[0] == '~') {
      result = strcmp(current.value + 1, previous.value) == 0;
    } else {
      result = StringEqualString(&current, &previous);
    }
    break;
  }

done:
  StringDestruct(&current);
  StringDestruct(&previous);
  return result;
}

static Symbol* FindFileScopeSymbol(Syntax* syntax, String* name) {
  if (InNamedNamespace(syntax)) {
    Symbol* symbol = NamespaceFindSymbol(syntax->current_namespace, name);
    if (symbol != NULL) {
      return symbol;
    }
    return NULL;
  }
  return FindGlobalSymbol(name);
}

static bool InsertFileScopeSymbol(Syntax* syntax, Symbol* symbol) {
  if (InNamedNamespace(syntax)) {
    return NamespaceInsertSymbol(syntax->current_namespace, symbol);
  }
  return InsertGlobalSymbol(symbol);
}

static bool CanOverloadFunctions(Symbol* a, Symbol* b) {
  return CompilerIsCXX() && a != NULL && b != NULL &&
         TypeIsFunction(a->type) && TypeIsFunction(b->type);
}

static Symbol* FindMatchingOverload(Symbol* first, TypeRecord* type) {
  for (Symbol* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (TypeEqual(overload->type, type)) {
      return overload;
    }
  }
  return NULL;
}

static void SetOverloadAsmName(Symbol* first, Symbol* overload) {
  (void)first;
  SymbolSetCXXMangledAsmName(overload);
}

static void AppendOverload(Symbol* first, Symbol* overload) {
  Symbol* tail = first;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  overload->namespace_ = first->namespace_;
  tail->overload_next = overload;
  first->flags.is_overloaded = true;
  overload->flags.is_overloaded = true;
  SetOverloadAsmName(first, first);
  SetOverloadAsmName(first, overload);
}

static Symbol* FollowAlias(Symbol* symbol) {
  int depth = 0;
  while (symbol != NULL && symbol->flags.is_using_alias &&
         symbol->alias_target != NULL && depth < 64) {
    symbol = symbol->alias_target;
    depth++;
  }
  return symbol;
}

void FullyQualifiedIdentifierInit(FullyQualifiedIdentifier* name) {
  name->absolute = false;
  name->is_qualified = false;
  VectorInit(&name->components);
  StringInit(&name->spelling, NULL);
}

void FullyQualifiedIdentifierDestruct(FullyQualifiedIdentifier* name) {
  VectorDestructWithContents(&name->components,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
  StringDestruct(&name->spelling);
}

const char* FullyQualifiedIdentifierLast(FullyQualifiedIdentifier* name) {
  if (name->components.length == 0) {
    return "";
  }
  String* last = name->components.value.p[name->components.length - 1];
  return last->value;
}

static void FullyQualifiedIdentifierAppend(FullyQualifiedIdentifier* name,
                                           String* component) {
  if (name->spelling.length != 0 || name->absolute) {
    StringAppend(&name->spelling, "::");
  }
  StringAppendString(&name->spelling, component);
  VectorAppend(&name->components, NewString(component->value));
}

bool SyntaxParseOperatorFunctionName(Syntax* syntax, String* name) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(operator))) {
    return false;
  }
  Token op = syntax->lex->current_token;
  switch (op) {
    case TOK(plus):
    case TOK(minus):
    case TOK(star):
    case TOK(slash):
    case TOK(percent):
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
    case TOK(equalequal):
    case TOK(bangeq):
    case TOK(less):
    case TOK(lesseq):
    case TOK(greater):
    case TOK(greatereq):
      StringInit(name, "operator");
      StringAppend(name, TokenName(op));
      LexNextToken(syntax->lex);
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

static bool ParseQualifiedIdentifierComponent(Syntax* syntax, String* component) {
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    StringInit(component, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
    return true;
  }
  return SyntaxParseOperatorFunctionName(syntax, component);
}

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
  while (LexMatch(lex, TOK(coloncolon))) {
    name->is_qualified = true;
    bool is_destructor = LexMatch(lex, TOK(tilde));
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

static void SyntaxConsumeOptionalTemplateId(Syntax* syntax,
                                            TokenClass followers) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(less))) {
    return;
  }
  Vector* args = SyntaxParseTemplateArgumentList(syntax, followers);
  if (args != NULL) {
    VectorDestructWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
  }
}

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
  StringDestruct(&component);
  SyntaxConsumeOptionalTemplateId(syntax, followers);

  while (LexMatch(lex, TOK(coloncolon))) {
    name->is_qualified = true;
    bool is_destructor = LexMatch(lex, TOK(tilde));
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
    SyntaxConsumeOptionalTemplateId(syntax, followers);
  }
  return true;
}

static Namespace* FindNamespaceChildInScope(Syntax* syntax, String* name) {
  Namespace* ns = syntax->current_namespace != NULL ? syntax->current_namespace
                                                    : compiler->global_namespace;
  while (ns != NULL) {
    Namespace* child = NamespaceFindChild(ns, name);
    if (child != NULL) {
      return child;
    }
    ns = ns->parent;
  }
  return NamespaceFindChild(compiler->global_namespace, name);
}

bool SyntaxCurrentTokenStartsQualifiedName(Syntax* syntax) {
  if (LexLookingAt(syntax->lex, TOK(coloncolon))) {
    return true;
  }
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  return FindNamespaceChildInScope(syntax, &syntax->lex->spelling) != NULL;
}

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
      ? NamespaceFindChild(compiler->global_namespace, first)
      : FindNamespaceChildInScope(syntax, first);
  for (size_t i = 1; ns != NULL && i < namespace_components; i++) {
    ns = NamespaceFindChild(ns, name->components.value.p[i]);
  }
  return ns;
}

Namespace* SyntaxFindQualifiedNamespace(Syntax* syntax,
                                        FullyQualifiedIdentifier* name) {
  if (name->components.length == 0) {
    return NULL;
  }

  String* first = name->components.value.p[0];
  Namespace* ns = name->absolute
      ? NamespaceFindChild(compiler->global_namespace, first)
      : FindNamespaceChildInScope(syntax, first);
  for (size_t i = 1; ns != NULL && i < name->components.length; i++) {
    ns = NamespaceFindChild(ns, name->components.value.p[i]);
  }
  return ns;
}

Symbol* SyntaxFindQualifiedSymbol(Syntax* syntax,
                                  FullyQualifiedIdentifier* name) {
  if (!name->is_qualified && name->components.length == 1) {
    String* simple = name->components.value.p[0];
    return FollowAlias(SyntaxFindSymbol(syntax, simple));
  }

  const char* last_name = FullyQualifiedIdentifierLast(name);
  String last;
  StringInit(&last, last_name);
  Symbol* symbol = NULL;
  Namespace* ns = ResolveQualifiedNamespace(syntax, name);
  if (ns != NULL && ns != compiler->global_namespace) {
    symbol = NamespaceFindSymbol(ns, &last);
  } else if (ns == compiler->global_namespace) {
    symbol = FindGlobalSymbol(&last);
  }
  if (symbol == NULL && name->components.length >= 2) {
    FullyQualifiedIdentifier prefix;
    FullyQualifiedIdentifierInit(&prefix);
    prefix.absolute = name->absolute;
    prefix.is_qualified = name->absolute || name->components.length > 2;
    for (size_t i = 0; i + 1 < name->components.length; i++) {
      FullyQualifiedIdentifierAppend(&prefix, name->components.value.p[i]);
    }
    Symbol* tag = SyntaxFindQualifiedTag(syntax, &prefix);
    if (tag != NULL && tag->type != NULL && TypeIsScopedEnum(tag->type)) {
      symbol = EnumFindConstant(tag->type->info.enum_info, &last);
    }
    FullyQualifiedIdentifierDestruct(&prefix);
  }
  StringDestruct(&last);
  return FollowAlias(symbol);
}

Symbol* SyntaxFindQualifiedPrefixSymbol(Syntax* syntax,
                                        FullyQualifiedIdentifier* name,
                                        size_t component_count) {
  if (component_count == 0 || component_count > name->components.length) {
    return NULL;
  }
  if (component_count == name->components.length) {
    return SyntaxFindQualifiedSymbol(syntax, name);
  }
  if (component_count == 1 && !name->absolute) {
    return SyntaxFindSymbol(syntax, name->components.value.p[0]);
  }

  size_t namespace_components = component_count - 1;
  Namespace* ns = NULL;
  if (namespace_components == 0) {
    ns = compiler->global_namespace;
  } else {
    String* first = name->components.value.p[0];
    ns = name->absolute
        ? NamespaceFindChild(compiler->global_namespace, first)
        : FindNamespaceChildInScope(syntax, first);
    for (size_t i = 1; ns != NULL && i < namespace_components; i++) {
      ns = NamespaceFindChild(ns, name->components.value.p[i]);
    }
  }
  if (ns == NULL) {
    return NULL;
  }

  String* last = name->components.value.p[component_count - 1];
  Symbol* symbol = ns == compiler->global_namespace
      ? FindGlobalSymbol(last)
      : NamespaceFindSymbol(ns, last);
  return FollowAlias(symbol);
}

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
    symbol = NamespaceFindTag(ns, &last);
  } else if (ns == compiler->global_namespace) {
    symbol = FindGlobalTag(&last);
  }
  StringDestruct(&last);
  return FollowAlias(symbol);
}

void SyntaxInit(Syntax* syntax, Lex* lex) {
  syntax->ast = NULL;
  syntax->lex = lex;
  syntax->local_symbol_stack = NULL;
  syntax->local_tag_stack = NULL;
  syntax->current_namespace = compiler != NULL ? compiler->global_namespace : NULL;
  syntax->fake_name_index = 1;
  syntax->found_open_paren = false;
  syntax->compound_literal_type = NULL;
  syntax->loop_count = 0;
  syntax->switch_count = 0;
  VectorInit(&syntax->all_local_symbols);
  VectorInit(&syntax->local_statics);
  VectorInit(&syntax->all_symbols);
  syntax->last_parsed_tag = NULL;
  syntax->parsing_template_declaration = false;
  syntax->parsing_template_argument = false;
  syntax->current_template_parameter_count = 0;
  syntax->current_template_parameters = NULL;
  syntax->context = kParsingFileScope;
}


void SyntaxDestruct(Syntax* syntax) {
  // all_local_symbols holds the most recent declaration's local symbols;
  // all_symbols accumulates the local symbols of every prior declaration
  // (SyntaxResetForNewDeclaration moves them there so they outlive the per-
  // declaration reset but stay reachable for code emission).  Both own their
  // symbols, so free their contents here at end of compilation.  The two
  // vectors are disjoint and contain no symbols owned elsewhere (function
  // parameters live in their function type's prototype, globals in the global
  // table), so there is no double free.
  VectorDestructWithContents(&syntax->all_local_symbols,
                             (VectorElementDestructor)SymbolDestruct, /*free_element=*/true);
  VectorDestructWithContents(&syntax->all_symbols,
                             (VectorElementDestructor)SymbolDestruct, /*free_element=*/true);
  VectorDestruct(&syntax->local_statics);
  ASTNodeDelete(syntax->ast);
}

void SyntaxResetForNewDeclaration(Syntax* syntax) {
  // Retain the just-finished declaration's local symbols until end of
  // compilation: code generation has run, but they may still be referenced by
  // the per-function target code that is emitted after the whole parse loop.
  // Accumulate (not overwrite - VectorCopy clears first) so symbols from every
  // declaration survive; SyntaxDestruct frees them all.
  VectorAppendVector(&syntax->all_symbols, &syntax->all_local_symbols);

  VectorDestruct(&syntax->all_local_symbols);
  VectorDestruct(&syntax->local_statics);
  ASTNodeDelete(syntax->ast);
  
  syntax->ast = NULL;
  syntax->local_symbol_stack = NULL;
  syntax->local_tag_stack = NULL;
  syntax->found_open_paren = false;
  syntax->compound_literal_type = NULL;
  syntax->loop_count = 0;
  syntax->switch_count = 0;
  syntax->last_parsed_tag = NULL;
  syntax->parsing_template_declaration = false;
  syntax->parsing_template_argument = false;
  syntax->current_template_parameter_count = 0;
  syntax->current_template_parameters = NULL;
  VectorInit(&syntax->all_local_symbols);
  VectorInit(&syntax->local_statics);
}

ASTNode* SyntaxNewPCLabel(SourceLocation location) {
  StringInit(&next_pc_label, NULL);
  StringPrintf(&next_pc_label, ".PC.%d", next_pc_label_id++);
  ASTNode* node = NewLabelASTNode(next_pc_label.value, NULL, true, location);
  StringDestruct(&next_pc_label);
  return node;
}

Symbol* SyntaxFindSymbol(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_symbol_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
  if (symbol != NULL) {
    return FollowAlias(symbol);
  }
  if (InNamedNamespace(syntax)) {
    symbol = NamespaceFindSymbolInScope(syntax->current_namespace, name);
    if (symbol != NULL) {
      return FollowAlias(symbol);
    }
  }
  return FollowAlias(FindGlobalSymbol(name));
}

bool SyntaxAddSymbol(Syntax* syntax, Symbol* symbol) {
  if (syntax->local_symbol_stack == NULL) {
    if (InNamedNamespace(syntax)) {
      return NamespaceInsertSymbol(syntax->current_namespace, symbol);
    }
    return InsertGlobalSymbol(symbol);
  }
  bool ok = InsertLocalSymbol(syntax->local_symbol_stack, symbol);
  if (ok) {
    VectorAppend(&syntax->all_local_symbols, symbol);
  }
  return ok;
}

Symbol* SyntaxFindTag(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_tag_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
  if (symbol != NULL) {
    return FollowAlias(symbol);
  }
  if (InNamedNamespace(syntax)) {
    symbol = NamespaceFindTagInScope(syntax->current_namespace, name);
    if (symbol != NULL) {
      return FollowAlias(symbol);
    }
  }
  return FollowAlias(FindGlobalTag(name));
}

Symbol* SyntaxFindTopScopeTag(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_tag_stack;
  if (scope != NULL) {
    Symbol* symbol = FindSymbol(&scope->table, name);
    if (symbol != NULL) {
      return symbol;
    }
    return NULL;
  }
  if (InNamedNamespace(syntax)) {
    return NamespaceFindTag(syntax->current_namespace, name);
  }
  return FindGlobalTag(name);
}

bool SyntaxAddTag(Syntax* syntax, Symbol* symbol) {
  if (syntax->local_tag_stack == NULL) {
    if (InNamedNamespace(syntax)) {
      return NamespaceInsertTag(syntax->current_namespace, symbol);
    }
    return InsertGlobalTag(symbol);
  }
  bool ok = InsertLocalSymbol(syntax->local_tag_stack, symbol);
  if (ok) {
    VectorAppend(&syntax->all_local_symbols, symbol);
  }
  return ok;
}

void SyntaxError(Syntax* syntax, const char* format, ...) {
  if (abort_on_error) {
    longjmp(error_abort_state, 1);
  }
  va_list ap;
  va_start(ap, format);
  VLexError(syntax->lex, format, ap);
  va_end(ap);
}

void SyntaxWarning(Syntax* syntax, const char* warn, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VLexWarning(syntax->lex, warn, format, ap);
  va_end(ap);
}

void SyntaxNeedSemicolon(Syntax* syntax, TokenClass followers) {
  if (!LexMatch(syntax->lex, TOK(semicolon))) {
    SyntaxError(syntax, "Expected semicolon");
    SyntaxRecover(syntax, followers | TC(semicolon));
  }
}

const char* SyntaxFakeName(Syntax* syntax) {
  snprintf(syntax->fake_name_buffer, sizeof(syntax->fake_name_buffer),
           "__invented__%d", syntax->fake_name_index);
  syntax->fake_name_index++;
  return syntax->fake_name_buffer;
}

void SyntaxFakeTagName(Syntax* syntax, String* tag_name) {
  Symbol* tag;
  do {
    StringSet(tag_name, SyntaxFakeName(syntax));
    tag = SyntaxFindTopScopeTag(syntax, tag_name);
  } while (tag != NULL);
}

Storage SyntaxParseStorage(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(extern):
      LexNextToken(syntax->lex);
      return STO(extern);
    case TOK(typedef):
      LexNextToken(syntax->lex);
      return STO(typedef);
    case TOK(auto):
      if (CompilerIsCXX()) {
        return STO(implicit);
      }
      LexNextToken(syntax->lex);
      return STO(auto);
    case TOK(static):
      LexNextToken(syntax->lex);
      return STO(static);
    case TOK(register):
      LexNextToken(syntax->lex);
      return STO(register);
    case TOK(thread):
      LexNextToken(syntax->lex);
      return STO(thread);
   default:
      return STO(implicit);
  }
}

static bool IsDefinition(TypeParser* parser, Symbol* sym, Storage storage) {
  if (StorageIs(storage, STO(extern))) {
    return false;
  }
  if (sym->flags.is_argument) {
    return true;
  }
  return LexLookingAt(parser->lex, TOK(equal)) ||
      LexLookingAt(parser->lex, TOK(lbrace));
}

static ASTNode* ParseBracedInitializer(Syntax* syntax);
static ASTNode* ParseNamespaceDeclaration(Syntax* syntax);
static ASTNode* ParseUsingDeclaration(Syntax* syntax);

void SyntaxParseStaticAssert(Syntax* syntax) {
  LexNextToken(syntax->lex);  // static_assert
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));

  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(exprsep));
  expr = AnalyzeExpression(expr);
  int64_t value = 0;
  if (!EvaluateIntegerExpression(expr, &value)) {
    SyntaxError(syntax, "static_assert expression is not an integer constant expression");
  }

  const char* message = "static assertion failed";
  if (LexMatch(syntax->lex, TOK(comma))) {
    if (LexLookingAt(syntax->lex, TOK(string))) {
      message = syntax->lex->spelling.value;
      LexNextToken(syntax->lex);
    } else {
      SyntaxError(syntax, "static_assert message must be a string literal");
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra));
  SyntaxNeedBracket(syntax, TOK(semicolon), TC(semicolon));

  if (value == 0) {
    SyntaxError(syntax, "%s", message);
  }
  ASTNodeDelete(expr);
}

// Identity transform used when deep-cloning an AST node.
static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  return node;
}

// Deep-clone an initializer so that the same value can be used for each index
// of a GCC range designator.
static ASTNode* CloneInitializer(ASTNode* init) {
  if (init->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)init;
    return NewExpressionInitializerASTNode(
        ASTNodeClone(e->expr, IdentityCloneNode, NULL, NULL), init->location);
  }
  return ASTNodeClone(init, IdentityCloneNode, NULL, NULL);
}

// Clone a designator list, substituting a single array index at position
// range_pos (used to expand a [start ... end] range designator).
static Vector* CloneDesignators(Vector* src, int range_pos, int index_value) {
  Vector* dst = NewVector();
  for (size_t i = 0; i < src->length; i++) {
    Designator* d = src->value.p[i];
    if (d->designator_type == kDesignatorArray) {
      int v = ((int)i == range_pos) ? index_value : d->value.array_index;
      VectorAppend(dst, NewArrayDesignator(d->type, v));
    } else {
      VectorAppend(dst, NewStructDesignator(
                            NewString(d->value.struct_member_name->value)));
    }
  }
  return dst;
}

static void ParseDesignatedInitializer(Syntax* syntax,
                                           Vector* initializers) {
  Vector* designators = NewVector();
  SourceLocation location = syntax->lex->current_token_location;
  
  // Collect all designators.  Each is either an array or struct designator.
  // The array designator is a constant expression enclosed in square
  // brackets. The struct designator is a dot followed by a struct member
  // name.
  while (LexLookingAt(syntax->lex, TOK(lsquare)) ||
         LexLookingAt(syntax->lex, TOK(dot))) {
    if (LexMatch(syntax->lex, TOK(lsquare))) {
      // Array designator.
      ASTNode* index_expr = SyntaxParseExpression(syntax, TC(semicolon));
      index_expr = AnalyzeExpression(index_expr);
      int64_t value;
      bool ok = EvaluateIntegerExpression(index_expr, &value);
      if (!ok) {
        SyntaxError(syntax,
              "Need constant expression inside [] in designated initializer");
        value = 0;
      }
      // GCC range designator: [ start ... end ].
      int64_t end_value = value;
      if (LexMatch(syntax->lex, TOK(ellipsis))) {
        ASTNode* end_expr = SyntaxParseExpression(syntax, TC(semicolon));
        end_expr = AnalyzeExpression(end_expr);
        if (!EvaluateIntegerExpression(end_expr, &end_value)) {
          SyntaxError(syntax,
                "Need constant expression inside [] in designated initializer");
          end_value = value;
        }
      }
      SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
      Designator* d = NewArrayDesignator(NULL, (int)value);
      d->array_index_end = (int)end_value;
      VectorAppend(designators, d);
    } else if (LexMatch(syntax->lex, TOK(dot))) {
      // Struct designator.  The dot is followed by a struct member name.
      String* member_name;
      if (LexLookingAt(syntax->lex, TOK(identifier))) {
        member_name = NewString(syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      } else {
        SyntaxError(syntax,
                    "Need member name after . in designated initializer");
        member_name = NewString(SyntaxFakeName(syntax));
      }
      VectorAppend(designators, NewStructDesignator(member_name));
    }
  }
  // This is followed by an = sign and an initializer.
  if (!LexMatch(syntax->lex, TOK(equal))) {
    SyntaxError(syntax, "Expected = in designated initializer");
    SyntaxRecover(syntax, TC(exprsep));
  } else {
    ASTNode* init;
    // The initialization expression is either a brace-enclosed
    // initializer or a single expression.
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      init = ParseBracedInitializer(syntax);
    } else {
      init = NewExpressionInitializerASTNode(
                                             SyntaxParseSingleExpression(syntax, TC(exprsep)), location);
    }
    // If one of the designators is a [start ... end] range, expand it into one
    // designated initializer per index, cloning the value for each.
    int range_pos = -1;
    for (size_t i = 0; i < designators->length; i++) {
      Designator* d = designators->value.p[i];
      if (d->designator_type == kDesignatorArray &&
          d->array_index_end > d->value.array_index) {
        range_pos = (int)i;
        break;
      }
    }
    if (range_pos >= 0) {
      Designator* range = designators->value.p[range_pos];
      int start = range->value.array_index;
      int end = range->array_index_end;
      for (int v = start; v <= end; v++) {
        Vector* desigs = CloneDesignators(designators, range_pos, v);
        ASTNode* init_for_index = (v == end) ? init : CloneInitializer(init);
        VectorAppend(initializers, NewDesignatedInitializerASTNode(
                                       desigs, init_for_index, location));
      }
      VectorDelete(designators);
    } else {
      VectorAppend(initializers, NewDesignatedInitializerASTNode(
                                                               designators, init, location));
    }
  }

}

// Parse a brace-enclosed initializer.
static ASTNode* ParseBracedInitializer(Syntax* syntax) {
  Vector* initializers = NewVector();
  SourceLocation location = syntax->lex->current_token_location;
  while (!LexLookingAt(syntax->lex, TOK(rbrace))) {
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      // Braced initializer.
      VectorAppend(initializers, ParseBracedInitializer(syntax));
    } else if (LexLookingAt(syntax->lex, TOK(lsquare)) ||
               LexLookingAt(syntax->lex, TOK(dot))) {
      // Designated initializer.
      ParseDesignatedInitializer(syntax, initializers);
    } else {
      // Not a designated initializer, expression.
      ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(exprsep));
      VectorAppend(initializers,
                   NewExpressionInitializerASTNode(expr, location));
    }
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebra));
  return NewBracedInitializerASTNode(initializers, NULL, location);
}

// Parses a symbol initializer.
ASTNode* SyntaxParseInitializer(Syntax* syntax, Symbol* sym, Storage storage) {
  if (StorageIs(storage, STO(extern))) {
    SyntaxWarning(syntax, "extern-initializer", "extern with initializer");
  }
  if (StorageIs(storage, STO(typedef))) {
    SyntaxError(syntax, "typdefs can't have initializers");
  }
  if (!StorageIs(storage, STO(extern))) {
    sym->flags.is_defined = true;
  }
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    // Braced initializer.
    return ParseBracedInitializer(syntax);
  }

  SourceLocation location = syntax->lex->current_token_location;
  return NewExpressionInitializerASTNode(
      SyntaxParseSingleExpression(syntax, TC(semicolon) | TC(stmt)), location);
}

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
static void ParseAttributeText(String* text, Vector* attrs) {
  const char* p = (text->value != NULL) ? text->value : "";
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
    // Accepted but not modelled (parsed cleanly, no effect).
    "stdcall", "cdecl", "fastcall", "thiscall", "regparm", "ms_abi",
    "sysv_abi", "may_alias", "gnu_inline", "nothrow", "leaf", "cold", "hot",
    "malloc", "pure", "const", "nonnull", "returns_nonnull", "sentinel",
    "weak", "alias", "section", "visibility", "used", "constructor",
    "destructor", "transparent_union", "vector_size", "mode",
    "no_instrument_function", "cleanup", "returns_twice", "artificial",
    "designated_init", "fallthrough", "warning", "error", "alloc_size",
    "format_arg", "nonstring", "noclone", "noipa", "flatten", "naked",
    "weakref", "dllimport", "dllexport", "common", "nocommon", "tls_model",
    "aligned_alloc", "assume_aligned",
  };
  for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
    if (strcmp(known[i], name) == 0) {
      return true;
    }
  }
  return false;
}

// Parse an __attribute__((...)) clause list, appending parsed Attribute* to
// attrs.  Assumes TOK(attribute) was just consumed.
void SyntaxParseAttribute(Syntax* syntax, Vector* attrs) {
  String attribute_list = {0};
  LexReadAttributes(syntax->lex, &attribute_list);
  size_t start = attrs->length;
  ParseAttributeText(&attribute_list, attrs);
  for (size_t i = start; i < attrs->length; i++) {
    Attribute* attr = attrs->value.p[i];
    if (!IsKnownAttribute(attr->name.value)) {
      SyntaxWarning(syntax, "attributes", "'%s' attribute directive ignored",
                    attr->name.value);
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), 0);
  StringDestruct(&attribute_list);
}

// Interprets attributes that have just been attached to a declared symbol and
// affect the symbol/type directly (layout and function-behavior flags).
static void ApplyDeclarationAttributes(Symbol* sym) {
  if (sym == NULL) {
    return;
  }

  // Function-behavior flags (also meaningful on forward declarations).
  if (AttributeListHas(&sym->attributes, "noreturn")) {
    sym->flags.noreturn = true;
  }
  if (AttributeListHas(&sym->attributes, "always_inline")) {
    sym->flags.always_inline = true;
  }
  if (AttributeListHas(&sym->attributes, "noinline")) {
    sym->flags.noinline = true;
  }

  if (StorageIs(sym->storage, STO(typedef))) {
    // packed/aligned on a typedef applies to the (usually anonymous) struct or
    // union type it names.
    TypeApplyStructAttributesFromSymbol(sym);
    return;
  }
  // aligned(N) on a variable raises its alignment.
  Attribute* aligned = AttributeListFind(&sym->attributes, "aligned");
  if (aligned != NULL) {
    long n = 0;
    if (AttributeArgInt(aligned, 0, &n) && n > 0) {
      sym->alignment = (int)n;
    }
  }
}

// For an old-style C function we have the types for the arguments specified
// in the definition, before the body.  The previous formal arguments need
// to be resolved by the new formals with types.
static void ResolveOldStyleFormalArgument(Syntax* syntax, TypeRecord* func, Symbol* formal) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* prev_formal = (Symbol*)formals->value.p[i];
    if (StringEqualString(&prev_formal->name, &formal->name)) {
      formals->value.p[i] = formal;
      formal->flags.is_argument = true;
      formal->value.arg_number = prev_formal->value.arg_number;
      SymbolDelete(prev_formal);
      return;
    }
  }
  SyntaxError(syntax, "No such function parameter %s", formal->name.value);
}

static void AddFunctionScopeSymbols(Syntax* syntax, TypeRecord* func) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    InsertLocalSymbol(syntax->local_symbol_stack, formal);
  }
}

static Vector* ParseCXXInitializerArgumentList(Syntax* syntax, Token close);
static const char* CXXConstructorNameForType(TypeRecord* type);
static StructMember* FindCXXConstructor(TypeRecord* type);

static Symbol* FindThisSymbol(Syntax* syntax) {
  String this_name;
  StringInit(&this_name, "this");
  Symbol* symbol = SyntaxFindSymbol(syntax, &this_name);
  StringDestruct(&this_name);
  return symbol;
}

static StructMember* FindCXXBaseSpecialMember(CXXBaseSpecifier* base,
                                              bool destructor) {
  if (base == NULL || base->type == NULL || !TypeIsStructOrUnion(base->type) ||
      base->type->info.struct_info == NULL ||
      base->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String member_name;
  if (destructor) {
    StringInit(&member_name, "~");
    StringAppendString(&member_name, base->type->info.struct_info->tag_name);
  } else {
    StringInit(&member_name, base->type->info.struct_info->tag_name->value);
  }
  StructMember* member =
      FindStructMember(base->type->info.struct_info, &member_name);
  StringDestruct(&member_name);
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }
  TypeRecord* func = member->symbol->type;
  if (!TypeIsFunction(func)) {
    return NULL;
  }
  if (destructor && !func->info.function.is_destructor) {
    return NULL;
  }
  if (!destructor && !func->info.function.is_constructor) {
    return NULL;
  }
  return member;
}

static ASTNode* NewCXXBaseSpecialMemberCall(Syntax* syntax,
                                            TypeRecord* receiver_func,
                                            CXXBaseSpecifier* base,
                                            bool destructor,
                                            Vector* actuals,
                                            SourceLocation location) {
  StructMember* member = FindCXXBaseSpecialMember(base, destructor);
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL && receiver_func != NULL &&
      TypeIsFunction(receiver_func) &&
      receiver_func->info.function.prototype.length > 0) {
    this_symbol = receiver_func->info.function.prototype.value.p[0];
  }
  if (member == NULL || this_symbol == NULL) {
    if (actuals != NULL) {
      VectorDelete(actuals);
    }
    return NULL;
  }

  String member_name;
  if (destructor) {
    StringInit(&member_name, "~");
    StringAppendString(&member_name, base->type->info.struct_info->tag_name);
  } else {
    StringInit(&member_name, base->type->info.struct_info->tag_name->value);
  }
  ASTNode* receiver = NewIdentifierASTNode(this_symbol, location);
  ASTNode* member_node =
      NewStringConstantASTNode(NewString(member_name.value), NULL, location);
  StringDestruct(&member_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver, member_node);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                       actuals != NULL ? actuals : NewVector());
  return NewExpressionStatementASTNode(call, location);
}

static void AppendCXXBaseDestructorCalls(Syntax* syntax, TypeRecord* func,
                                        Vector* body,
                                        SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_destructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = owner->bases.length; i > 0; i--) {
    CXXBaseSpecifier* base = owner->bases.value.p[i - 1];
    ASTNode* call = NewCXXBaseSpecialMemberCall(syntax, func, base, true,
                                                NULL, location);
    if (call == NULL) {
      continue;
    }
    if (compiler->debug_output && body->length > 0) {
      VectorInsertBefore(body, body->length - 1, call);
    } else {
      VectorAppend(body, call);
    }
  }
}

void SyntaxCXXConstructorInitListInit(CXXConstructorInitList* init_list) {
  VectorInit(&init_list->base_specs);
  VectorInit(&init_list->base_statements);
  VectorInit(&init_list->member_statements);
}

void SyntaxCXXConstructorInitListDestruct(CXXConstructorInitList* init_list) {
  VectorDestruct(&init_list->base_specs);
  VectorDestruct(&init_list->base_statements);
  VectorDestruct(&init_list->member_statements);
}

static void VectorInsertOrAppend(Vector* vec, size_t index, void* value) {
  if (index >= vec->length) {
    VectorAppend(vec, value);
  } else {
    VectorInsertBefore(vec, index, value);
  }
}

static bool VectorContainsPointer(Vector* vec, void* value) {
  for (size_t i = 0; i < vec->length; i++) {
    if (vec->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

static CXXBaseSpecifier* FindCXXDirectBaseByName(Struct* owner,
                                                 const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base->type != NULL && TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        base->type->info.struct_info->tag_name != NULL &&
        StringEqual(base->type->info.struct_info->tag_name, name)) {
      return base;
    }
  }
  return NULL;
}

static StructMember* FindCXXDirectDataMemberByName(Struct* owner,
                                                   const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member->symbol != NULL && StringEqual(&member->symbol->name, name) &&
        !member->is_static && !member->is_member_function) {
      return member;
    }
  }
  return NULL;
}

static Symbol* CXXThisSymbolFromFunction(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length == 0) {
    return NULL;
  }
  return func->info.function.prototype.value.p[0];
}

static ASTNode* NewCXXThisMemberAccess(TypeRecord* func,
                                       const char* member_name,
                                       SourceLocation location) {
  Symbol* this_symbol = CXXThisSymbolFromFunction(func);
  if (this_symbol == NULL) {
    return NULL;
  }
  return NewBinaryASTNode(
      AST_OP(arrow), NULL, location,
      NewIdentifierASTNode(this_symbol, location),
      NewStringConstantASTNode(NewString(member_name), NULL, location));
}

static ASTNode* NewCXXVPtrInitializer(TypeRecord* func,
                                      SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL) {
    return NULL;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  if (owner->vtable_symbol == NULL) {
    return NULL;
  }
  String vptr_name;
  StringInit(&vptr_name, "__vptr");
  StructMember* vptr_member = FindStructMember(owner, &vptr_name);
  StringDestruct(&vptr_name);
  if (vptr_member == NULL) {
    return NULL;
  }
  ASTNode* target = NewCXXThisMemberAccess(func, "__vptr", location);
  if (target == NULL) {
    return NULL;
  }
  ASTNode* value = NewIdentifierASTNode(owner->vtable_symbol, location);
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), vptr_member->symbol->type,
                       location, target, value),
      location);
}

static ASTNode* NewCXXMemberInitializerStatement(Syntax* syntax,
                                                TypeRecord* func,
                                                StructMember* member,
                                                Vector* actuals,
                                                SourceLocation location) {
  if (member == NULL || member->symbol == NULL || actuals == NULL) {
    if (actuals != NULL) {
      VectorDelete(actuals);
    }
    return NULL;
  }

  TypeRecord* member_type = member->symbol->type;
  if (TypeIsStructOrUnion(member_type) && FindCXXConstructor(member_type) != NULL) {
    const char* constructor_name = CXXConstructorNameForType(member_type);
    ASTNode* receiver =
        NewCXXThisMemberAccess(func, member->symbol->name.value, location);
    if (receiver == NULL || constructor_name == NULL) {
      VectorDelete(actuals);
      return NULL;
    }
    ASTNode* member_name =
        NewStringConstantASTNode(NewString(constructor_name), NULL, location);
    ASTNode* member_access =
        NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member_name);
    return NewExpressionStatementASTNode(
        NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals),
        location);
  }

  if (actuals->length != 1) {
    SyntaxError(syntax,
                "member initializer for %s requires one expression",
                member->symbol->name.value);
    VectorDelete(actuals);
    return NULL;
  }
  ASTNode* value = actuals->value.p[0];
  VectorDelete(actuals);
  ASTNode* target =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  if (target == NULL) {
    return NULL;
  }
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), member_type, location, target, value),
      location);
}

void SyntaxParseCXXConstructorInitializerList(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_constructor ||
      func->info.function.cxx_member_owner == NULL ||
      !LexMatch(syntax->lex, TOK(colon))) {
    return;
  }

  Struct* owner = func->info.function.cxx_member_owner;
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(lbrace))) {
    SourceLocation location = syntax->lex->current_token_location;
    FullyQualifiedIdentifier name;
    FullyQualifiedIdentifierInit(&name);
    if (!SyntaxParseFullyQualifiedIdentifier(syntax, &name)) {
      SyntaxError(syntax, "Expected constructor initializer name");
      FullyQualifiedIdentifierDestruct(&name);
      break;
    }

    Vector* actuals = NULL;
    if (LexMatch(syntax->lex, TOK(lparen))) {
      actuals = ParseCXXInitializerArgumentList(syntax, TOK(rparen));
    } else if (LexMatch(syntax->lex, TOK(lbrace))) {
      actuals = ParseCXXInitializerArgumentList(syntax, TOK(rbrace));
    } else {
      SyntaxError(syntax, "Expected constructor initializer argument list");
      actuals = NewVector();
    }

    const char* init_name = FullyQualifiedIdentifierLast(&name);
    CXXBaseSpecifier* base = FindCXXDirectBaseByName(owner, init_name);
    if (base != NULL) {
      ASTNode* call = NewCXXBaseSpecialMemberCall(syntax, func, base, false,
                                                  actuals, location);
      if (call != NULL) {
        VectorAppend(&init_list->base_specs, base);
        VectorAppend(&init_list->base_statements, call);
      }
    } else {
      StructMember* member = FindCXXDirectDataMemberByName(owner, init_name);
      if (member == NULL) {
        SyntaxError(syntax, "%s is not a direct base or member of %s",
                    init_name,
                    owner->tag_name != NULL ? owner->tag_name->value
                                            : "<anonymous>");
        VectorDelete(actuals);
      } else {
        ASTNode* stmt = NewCXXMemberInitializerStatement(
            syntax, func, member, actuals, location);
        if (stmt != NULL) {
          VectorAppend(&init_list->member_statements, stmt);
        }
      }
    }
    FullyQualifiedIdentifierDestruct(&name);

    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
}

void SyntaxInsertCXXConstructorPreamble(Syntax* syntax, TypeRecord* func,
                                        Vector* body,
                                        CXXConstructorInitList* init_list,
                                        SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_constructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  size_t insert_at = 0;
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    ASTNode* call = NULL;
    for (size_t j = 0; j < init_list->base_specs.length; j++) {
      if (init_list->base_specs.value.p[j] == base) {
        call = init_list->base_statements.value.p[j];
        break;
      }
    }
    if (call == NULL && !VectorContainsPointer(&init_list->base_specs, base)) {
      call = NewCXXBaseSpecialMemberCall(syntax, func, base, false, NULL,
                                         location);
    }
    if (call == NULL) {
      continue;
    }
    VectorInsertOrAppend(body, insert_at, call);
    insert_at++;
  }
  ASTNode* vptr_init = NewCXXVPtrInitializer(func, location);
  if (vptr_init != NULL) {
    VectorInsertOrAppend(body, insert_at, vptr_init);
    insert_at++;
  }
  for (size_t i = 0; i < init_list->member_statements.length; i++) {
    ASTNode* stmt = init_list->member_statements.value.p[i];
    VectorInsertOrAppend(body, insert_at, stmt);
    insert_at++;
  }
}

static ASTNode* DeclareOrDefineFunction(Syntax* syntax,
                                        Vector* declarations,
                             Symbol* sym, Symbol* old_sym) {
  if (sym->type->info.function.old_style) {
    // Old style functions have the types of their formal
    // arguments specified before the open brace for their
    // body.
    while (!LexLookingAt(syntax->lex, TOK(lbrace))) {
      TypeParser arg_parser;
      TypeParserInit(&arg_parser, syntax->lex, syntax, STO(auto), kParsingBlockScope);
      TypeRecord* arg_type = TypeParserParseType(&arg_parser, true);
      if (arg_type == NULL) {
        SyntaxError(syntax, "Type expected");
        SyntaxRecover(syntax, TC(semicolon));
      } else {
        while (!LexLookingAt(syntax->lex, TOK(semicolon))) {
          Symbol* formal = TypeParserParseDeclarator(&arg_parser, arg_type);
          if (formal != NULL) {
            // Resolve the formal argument in the function prototype.
            ResolveOldStyleFormalArgument(syntax, sym->type, formal);
          }
          if (!LexMatch(syntax->lex, TOK(comma))) {
            break;
          }
        }
      }
      SyntaxNeedSemicolon(syntax, TC(openbra));
      TypeParserDestruct(&arg_parser);
    }
    // We need a function body after the argument declarations.
    if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
      SyntaxError(syntax, "Function body expected");
    }
  }
  
  
  CXXConstructorInitList cxx_initializers;
  SyntaxCXXConstructorInitListInit(&cxx_initializers);
  SyntaxParseCXXConstructorInitializerList(syntax, sym->type,
                                           &cxx_initializers);

  if (LexMatch(syntax->lex, TOK(lbrace))) {
    if (sym->type->info.function.old_style) {
      SyntaxWarning(syntax, "old-style-definition",
                    "old-style function definition");
    }
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->value.func_defn = sym;
    }
    ParserContext old_context = syntax->context;
    syntax->context = kParsingBlockScope;
    SyntaxOpenScope(syntax);
    AddFunctionScopeSymbols(syntax, sym->type);
    
    sym->flags.is_defined = true;
    sym->type->info.function.definition = true;
    
    // Parse the function body.
    Vector* body = NewVector();
    if (compiler->debug_output) {
      VectorAppend(body, SyntaxNewPCLabel(syntax->lex->current_token_location));
    }
    
    bool seen_statement = false;
    while (syntax->lex->current_token != TOK(rbrace) &&
           syntax->lex->current_token != TOK(eof)) {
      ASTNode* stmt;
      if (SyntaxLookingAtDeclaration(syntax)) {
        if (seen_statement) {
          SyntaxWarning(syntax, "declaration-after-statement",
                        "declaration after statement");
        }
        // Declaration.
        stmt = SyntaxParseLocalDeclaration(syntax);
      } else {
        seen_statement = true;
        stmt = SyntaxParseStatement(syntax,TC(semicolon));
      }
      if (stmt != NULL) {
        VectorAppend(body, stmt);
      }
    }
    SyntaxInsertCXXConstructorPreamble(syntax, sym->type, body,
                                       &cxx_initializers, sym->location);
    AppendCXXBaseDestructorCalls(syntax, sym->type, body, sym->location);
    SyntaxCloseScope(syntax);
    syntax->context = old_context;
    
    if (compiler->debug_output) {
      VectorAppend(body, SyntaxNewPCLabel(syntax->lex->current_token_location));
    }
    sym->type->info.function.body =
        NewCompoundStatementASTNode(body, syntax->lex->current_token_location);
    // The body is hung off the function type, not reachable from the
    // declaration AST, so register it as a teardown root of its own.
    VectorAppend(&compiler->declaration_asts, sym->type->info.function.body);
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(decl));
    ASTNode* decl = NewVariableDeclarationASTNode(sym, NULL,
                                                  syntax->lex->current_token_location);
    VectorAppend(declarations, decl);
    SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
    
    return NewDeclarationListASTNode(declarations,
                                     syntax->lex->current_token_location);
  } else {
    // Function is a declaration.  If the old symbol was inline, this is
    // an inline definition.
    if (old_sym != NULL && old_sym->type->info.function.is_inline) {
      old_sym->flags.is_inline_defn = true;
    }
  }
  SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
  return NULL;
}

static bool IsPowerOf2OrZero(int32_t v) {
  return (v & (v - 1)) == 0;
}

// Only static variables can be thread local.  This checks for invalid
// symbol storage or type.
static void CheckThreadLocal(Syntax* syntax, Symbol* symbol) {
  if (StorageIs(symbol->storage, STO(thread))) {
    bool error = TypeIsFunction(symbol->type);
    error |= !StorageIs(symbol->storage, STO(static) | STO(extern));
    if (error) {
      SyntaxError(syntax, "Illegal use of __thread");
    }
  }
}

// A declaration specifier is a set of:
// 1. storage specifier
// 2. type specifier
// 3. function specifier (inline).
// This collects them into the output variables.
static void ParseDeclarationSpecifier(Syntax* syntax, Storage* storage, bool* is_inline,
                                      TypeRecord** type, Vector* attributes, ParserContext context) {
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
      Storage new = s;
      Storage old = *storage;
    
      // Check for duplicate storage.
      if ((new & old) != 0) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate storage specifier");
      }
    
      // Remove __thread from mask and check for multiple bits set.
      if (!IsPowerOf2OrZero((new | old) & ~STO(thread))) {
        SyntaxError(syntax,
                    "Multiple incompatible storage specifiers");
      }
      *storage |= s;
    } else if (LexMatch(syntax->lex, TOK(inline))) {
      if (*is_inline) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate 'inline' specifier");
      }
      *is_inline = true;
    } else if (!(type_specifier.type != kTypeImplicit &&
                 CurrentIdentifierFollowedByScopeOperator(syntax)) &&
               SyntaxLookingAtType(syntax) &&
               (type_specifier.type & (kTypeStruct | kTypeUnion | kTypeEnum)) == 0) {
      type_specifier = TypeParserParseAndCombineTypes(&parser, &type_specifier);
    } else if (LexMatch(syntax->lex, TOK(attribute))) {
      SyntaxParseAttribute(syntax, attributes);
    } else {
      if (type_specifier.type == kTypeImplicit &&
          type_specifier.type_record == NULL) {
        SyntaxWarning(syntax, "implicit-int",
                      "type specifier missing, defaults to int");
      }
      *type = TypeParserBuildTypeRecord(&parser, &type_specifier);
      TypeParserDestruct(&parser);
      return;
    }
  }
  TypeParserDestruct(&parser);
}

static bool TypeContainsClassTemplate(TypeRecord* type);

static ASTNode* ParseExternalDeclarationList(TypeParser* parser,
                                         TypeRecord* type,
                                         Storage storage,
                                         Vector* attributes,
                                         Vector* declarations) {
  Syntax* syntax = parser->syntax;
  while (!LexEof(parser->syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // We don't need to have a variable declaration.
      break;
    }

    Symbol* sym = TypeParserParseDeclarator(parser, type);
    // The old_sym refers to a previous declaration if found.
    Symbol* old_sym = NULL;
    bool overload_was_appended = false;
    if (sym != NULL) {
      old_sym = parser->cxx_member_definition != NULL
          ? parser->cxx_member_definition->symbol
          : FindFileScopeSymbol(syntax, &sym->name);
      if (parser->cxx_member_definition == NULL &&
          CanOverloadFunctions(old_sym, sym)) {
        Symbol* matching_overload = FindMatchingOverload(old_sym, sym->type);
        if (matching_overload != NULL) {
          old_sym = matching_overload;
        } else {
          AppendOverload(old_sym, sym);
          old_sym = NULL;
          overload_was_appended = true;
        }
      }
      if (parser->cxx_member_definition != NULL) {
        StructMember* matching_member = FindStructMemberOverload(
            parser->cxx_member_definition, sym->type);
        if (matching_member != NULL) {
          parser->cxx_member_definition = matching_member;
          old_sym = matching_member->symbol;
        }
      }
      if (old_sym != NULL) {
        // We have this symbol already.  If it's a declaration then it's
        // OK to declare (and define) it now.  If it's a definition then
        // this must be a declaration.
        if (old_sym->flags.is_defined) {
          if (StorageIs(storage, STO(extern))) {
            // This might be a declaration, only if there is no initializer
            if (LexLookingAt(parser->lex, TOK(equal))) {
              // This is 'extern int foo = xxx', a definition
              SyntaxError(syntax, "Duplicate definition of symbol %s",
                          sym->name.value);
            }
          } else {
            if (IsDefinition(parser, old_sym, storage)) {
              // This is a declaration of a previously known definition.
              SyntaxError(syntax, "Duplicate definition of symbol %s",
                          sym->name.value);
            }
          }
        } else {
          // Old sym is a declaration.
          if (IsDefinition(parser, sym,  storage)) {
            old_sym->flags.is_defined = true;
          } else if (!StorageIs(storage, STO(extern))) {
            old_sym->flags.is_tentative_decl = true;
          }
        }
        if (!TypeEqual(sym->type, old_sym->type)) {
          SyntaxError(syntax, "Symbol %s redeclared with different type",
                      sym->name.value);
          TypeErrorDetails(syntax->lex->current_token_location,
                           sym->type, old_sym->type);
          const char* filename;
          int lineno, start, end;
          DecodeSourceLocation(old_sym->location, &filename, &lineno, &start, &end);
          ReportNote(filename, lineno, "Previously declared here");
        } else {
          // Symbol declaration is the same type as the definition, make sure
          // the linkage matches.
          Storage old_storage = old_sym->storage & ~STO(extern);
          Storage new_storage = sym->storage & ~STO(extern);

          if (old_storage != new_storage) {
            SyntaxError(syntax, "Symbol %s redeclared with different linkage",
                        sym->name.value);
          }
          sym->flags.is_defined = true;
        }
      } else {
        if (!overload_was_appended) {
          // This is the first declaration of this symbol, add to the symbol
          // table.
          bool inserted = InsertFileScopeSymbol(syntax, sym);
          assert(inserted);
          (void)inserted;
        }
        if (IsDefinition(parser, sym, storage)) {
          sym->flags.is_defined = true;
        } else if (!StorageIs(storage, STO(extern))) {
          sym->flags.is_tentative_decl = true;
        }
      }
    }

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsing.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(parser);
      continue;
    }

    // Parse common __attribute__ syntax.
    while (LexMatch(syntax->lex, TOK(attribute))) {
      SyntaxParseAttribute(syntax, attributes);
    }
    
    VectorCopy(&sym->attributes, attributes);
    VectorClear(attributes);
    ApplyDeclarationAttributes(sym);

    // Check for GCC-style assembler name after a declarator:
    //   int x asm("external_name");
    //   void f(void) asm("external_name");
    if (LexMatch(syntax->lex, TOK(asm))) {
      SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));
      String asm_name;
      StringInit(&asm_name, "");
      while (LexLookingAt(syntax->lex, TOK(string))) {
        StringAppend(&asm_name, syntax->lex->spelling.value);
        LexNextToken(syntax->lex);
      }
      SyntaxNeedBracket(syntax, TOK(rparen), TC(exprsep) | TC(decl));
      StringSetString(&sym->asm_name, &asm_name);
      StringDestruct(&asm_name);
    }

    if (old_sym != NULL) {
      if (sym->asm_name.length != 0) {
        StringSetString(&old_sym->asm_name, &sym->asm_name);
      } else if (old_sym->asm_name.length != 0) {
        StringSetString(&sym->asm_name, &old_sym->asm_name);
      }
    }
    if (TypeIsFunction(sym->type)) {
      SymbolSetCXXMangledAsmName(sym);
      if (old_sym != NULL && old_sym->asm_name.length == 0) {
        SymbolSetCXXMangledAsmName(old_sym);
      }
    }
  
    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      if (TypeContainsAuto(sym->type)) {
        SyntaxError(syntax, "auto function return type is not supported yet");
      }
      ASTNode *result = DeclareOrDefineFunction(syntax, declarations, sym, old_sym);
      if (result != NULL) {
        // A function definition whose name was already declared: `old_sym`
        // stays in the symbol table and `sym` (this definition, holding the
        // body and its own function type) is referenced only by the AST and
        // by old_sym->value.func_defn.  It never enters the table, so track it
        // for teardown instead of leaking the symbol<->type cycle.
        if (old_sym != NULL) {
          VectorAppend(&compiler->orphan_function_symbols, sym);
        }
        return result;
      }
    } else if (parser->is_inline) {
      SyntaxError(syntax, "inline can only be applied to functions");
    }
    if (TypeIsAbstractClass(sym->type)) {
      SyntaxError(syntax, "Cannot declare object of abstract class %s",
                  sym->type->info.struct_info->tag_name != NULL
                      ? sym->type->info.struct_info->tag_name->value
                      : "<anonymous>");
    }
    if (!syntax->parsing_template_declaration &&
        TypeContainsClassTemplate(sym->type)) {
      SyntaxError(syntax, "Class template instantiation is not supported yet");
    }
    
    if (old_sym != NULL) {
      // We now refer to the previously defined symbol rather than this new
      // one.
      if (sym->asm_name.length != 0) {
        StringSetString(&old_sym->asm_name, &sym->asm_name);
      }
      SymbolDelete(sym);
      sym = old_sym;
    }

    // Any initializer?
    ASTNode* initializer = NULL;
    if (LexMatch(syntax->lex, TOK(equal))) {
      syntax->init_storage = storage;
      initializer = SyntaxParseInitializer(syntax, sym, storage);
    } else if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(lbrace))) {
      initializer = ParseBracedInitializer(syntax);
      if (!StorageIs(storage, STO(extern))) {
        sym->flags.is_defined = true;
      }
    }
    if (TypeContainsAuto(sym->type) && initializer == NULL) {
      SyntaxError(syntax, "auto variable requires an initializer");
    } else if (TypeContainsAuto(sym->type)) {
      initializer = AnalyzeExpression(initializer);
      SemanticDeduceAutoType(sym, initializer, (ASTNode*)initializer);
    }
    ASTNode* decl = NewVariableDeclarationASTNode(
        sym, initializer, syntax->lex->current_token_location);
    VectorAppend(declarations, decl);

    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
    TypeParserReset(parser);
    
    // All declarations in the list share the same storage.
    parser->storage = storage;
  }
  if (type != NULL && (TypeIsEnum(type) || TypeIsStructOrUnion(type))) {
    // Declaring a struct/union/enum with no symbol still needs to
    // output debug information.
    if (compiler->debug_output) {
      BuildTypeDebugInfo(&compiler->debug_builder, type);
    }
  }
  return NULL;
}

static void AppendDeclarationsFromNode(Vector* declarations, ASTNode* node) {
  if (node == NULL) {
    return;
  }
  if (node->op != AST_OP(decl_list)) {
    VectorAppend(declarations, node);
    return;
  }

  DeclarationListASTNode* list = (DeclarationListASTNode*)node;
  VectorAppendVector(declarations, list->declarations);
  // The declarations have been transferred to the containing namespace list.
  // Leave the child list empty so its idempotent teardown does not delete them.
  VectorClear(list->declarations);
}

static Symbol* NewUsingAliasSymbol(const char* name, Symbol* target,
                                   SourceLocation location) {
  Symbol* alias = NewSymbol(name, NULL, target != NULL ? target->storage : STO(implicit));
  alias->flags.is_using_alias = true;
  alias->alias_target = target;
  alias->location = location;
  return alias;
}

static bool AddUsingAlias(Syntax* syntax, Symbol* alias, bool is_tag) {
  bool added = is_tag ? SyntaxAddTag(syntax, alias) : SyntaxAddSymbol(syntax, alias);
  if (!added) {
    SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                alias->name.value);
    SymbolDelete(alias);
    return false;
  }
  return true;
}

static void ImportNamespaceSymbol(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  Syntax* syntax = data;
  Symbol* target = ((SymbolNode*)node)->symbol;
  AddUsingAlias(syntax, NewUsingAliasSymbol(target->name.value, target,
                                            syntax->lex->current_token_location),
                /*is_tag=*/false);
}

static void ImportNamespaceTag(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  Syntax* syntax = data;
  Symbol* target = ((SymbolNode*)node)->symbol;
  AddUsingAlias(syntax, NewUsingAliasSymbol(target->name.value, target,
                                            syntax->lex->current_token_location),
                /*is_tag=*/true);
}

static void ImportNamespace(Syntax* syntax, Namespace* ns) {
  BinaryTreeTraverse(&ns->symbol_table, ImportNamespaceSymbol, syntax);
  BinaryTreeTraverse(&ns->tag_table, ImportNamespaceTag, syntax);
}

static ASTNode* EmptyDeclarationList(SourceLocation location) {
  return NewDeclarationListASTNode(NewVector(), location);
}

static ASTNode* ParseCXXSpecialMemberDefinition(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  Vector* declarations = NewVector();
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), kParsingFileScope);

  Symbol* sym = TypeParserParseCXXSpecialMemberDeclarator(&parser);
  Symbol* old_sym = parser.cxx_member_definition != NULL
      ? parser.cxx_member_definition->symbol
      : NULL;
  if (sym != NULL && parser.cxx_member_definition != NULL) {
    StructMember* matching_member =
        FindStructMemberOverload(parser.cxx_member_definition, sym->type);
    if (matching_member != NULL) {
      parser.cxx_member_definition = matching_member;
      old_sym = matching_member->symbol;
    }
  }
  if (sym == NULL || old_sym == NULL) {
    if (sym != NULL) {
      SymbolDelete(sym);
    }
    VectorDelete(declarations);
    TypeParserDestruct(&parser);
    return EmptyDeclarationList(location);
  }

  if (old_sym->flags.is_defined && LexLookingAt(syntax->lex, TOK(lbrace))) {
    SyntaxError(syntax, "Duplicate definition of symbol %s", sym->name.value);
  } else if (!TypeEqual(sym->type, old_sym->type)) {
    SyntaxError(syntax, "Symbol %s redeclared with different type",
                sym->name.value);
    TypeErrorDetails(syntax->lex->current_token_location,
                     sym->type, old_sym->type);
  } else if (LexLookingAt(syntax->lex, TOK(lbrace))) {
    old_sym->flags.is_defined = true;
  }

  ASTNode* result = DeclareOrDefineFunction(syntax, declarations, sym, old_sym);
  if (result != NULL) {
    VectorAppend(&compiler->orphan_function_symbols, sym);
    TypeParserDestruct(&parser);
    return result;
  }

  SyntaxError(syntax, "Special member definition requires a function body");
  SymbolDelete(sym);
  VectorDelete(declarations);
  TypeParserDestruct(&parser);
  return EmptyDeclarationList(location);
}

static ASTNode* ParseUsingAliasDeclaration(Syntax* syntax,
                                           FullyQualifiedIdentifier* name,
                                           SourceLocation location) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* parsed = TypeParserParseDeclarator(&parser, type);
  TypeParserDestruct(&parser);

  TypeRecord* alias_type = parsed != NULL ? parsed->type : type;
  Symbol* alias =
      NewSymbol(FullyQualifiedIdentifierLast(name), alias_type, STO(typedef));
  alias->location = location;
  bool added = SyntaxAddSymbol(syntax, alias);
  if (!added) {
    SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                alias->name.value);
    SymbolDelete(alias);
  }
  if (parsed != NULL) {
    SymbolDelete(parsed);
  }
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

static ASTNode* ParseUsingDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // using

  if (LexMatch(syntax->lex, TOK(namespace))) {
    FullyQualifiedIdentifier ns_name;
    FullyQualifiedIdentifierInit(&ns_name);
    if (!SyntaxParseFullyQualifiedIdentifier(syntax, &ns_name)) {
      SyntaxError(syntax, "Expected namespace name in using directive");
    } else {
      Namespace* ns = SyntaxFindQualifiedNamespace(syntax, &ns_name);
      if (ns == NULL) {
        SyntaxError(syntax, "Unknown namespace %s", ns_name.spelling.value);
      } else {
        ImportNamespace(syntax, ns);
      }
    }
    FullyQualifiedIdentifierDestruct(&ns_name);
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifier(syntax, &name)) {
    SyntaxError(syntax, "Expected identifier in using declaration");
    FullyQualifiedIdentifierDestruct(&name);
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

  if (!name.is_qualified && LexMatch(syntax->lex, TOK(equal))) {
    ASTNode* result = ParseUsingAliasDeclaration(syntax, &name, location);
    FullyQualifiedIdentifierDestruct(&name);
    return result;
  }

  if (!name.is_qualified) {
    SyntaxError(syntax, "Using declaration requires a qualified name");
    FullyQualifiedIdentifierDestruct(&name);
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

  Symbol* target = SyntaxFindQualifiedSymbol(syntax, &name);
  if (target == NULL) {
    target = SyntaxFindQualifiedTag(syntax, &name);
    if (target == NULL) {
      SyntaxError(syntax, "No such symbol \"%s\"", name.spelling.value);
    } else {
      AddUsingAlias(syntax, NewUsingAliasSymbol(FullyQualifiedIdentifierLast(&name),
                                                target, location),
                    /*is_tag=*/true);
    }
  } else {
    AddUsingAlias(syntax, NewUsingAliasSymbol(FullyQualifiedIdentifierLast(&name),
                                              target, location),
                  /*is_tag=*/false);
  }

  FullyQualifiedIdentifierDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

static Namespace* ParseNamespaceName(Syntax* syntax, Namespace* parent) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected namespace name");
    return parent;
  }

  Namespace* ns = parent;
  while (true) {
    String name;
    StringInit(&name, syntax->lex->spelling.value);
    ns = NamespaceFindOrCreateChild(ns, &name);
    StringDestruct(&name);
    LexNextToken(syntax->lex);

    if (!LexMatch(syntax->lex, TOK(coloncolon))) {
      break;
    }
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected namespace name after '::'");
      break;
    }
  }
  return ns;
}

static ASTNode* ParseNamespaceDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // namespace

  Namespace* previous_namespace = syntax->current_namespace;
  Namespace* parent = previous_namespace != NULL ? previous_namespace
                                                 : compiler->global_namespace;
  Namespace* ns = NULL;
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    ns = ParseNamespaceName(syntax, parent);
  } else {
    ns = NamespaceFindOrCreateAnonymousChild(parent);
  }

  SyntaxNeedBracket(syntax, TOK(lbrace), TC(openbra) | TC(decl));

  Vector* declarations = NewVector();
  syntax->current_namespace = ns;
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rbrace))) {
    ASTNode* node = SyntaxParseExternalDeclaration(syntax);
    AppendDeclarationsFromNode(declarations, node);
  }
  syntax->current_namespace = previous_namespace;

  SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
  return NewDeclarationListASTNode(declarations, location);
}

static TemplateParameter* NewTemplateParameter(const char* name,
                                               TemplateParameterKind kind,
                                               TypeRecord* type,
                                               int index) {
  TemplateParameter* param = malloc(sizeof(TemplateParameter));
  StringInit(&param->name, name);
  param->kind = kind;
  param->type = type;
  if (type != NULL) {
    TypeRecordIncRef(type);
  }
  param->index = index;
  return param;
}

static bool ParseTemplateParameter(Syntax* syntax, Vector* params) {
  Lex* lex = syntax->lex;
  int index = (int)params->length;
  if (LexMatch(lex, TOK(typename)) || LexMatch(lex, TOK(class))) {
    if (!LexLookingAt(lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected template parameter name");
      SyntaxRecover(syntax, TC(closebra));
      return false;
    }

    TypeRecord* placeholder =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    placeholder->template_parameter_index = index;
    Symbol* param = NewSymbol(lex->spelling.value, placeholder, STO(typedef));
    param->flags.invented = true;
    param->flags.is_template_parameter = true;
    param->flags.is_template_type_parameter = true;
    param->template_parameter_index = index;
    param->location = lex->current_token_location;
    bool added = SyntaxAddSymbol(syntax, param);
    if (!added) {
      SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
      SymbolDelete(param);
    }
    VectorAppend(params, NewTemplateParameter(lex->spelling.value,
                                              kTemplateParameterType, NULL,
                                              index));
    LexNextToken(lex);
    return true;
  }

  TypeParser parser;
  TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* param = TypeParserParseDeclarator(&parser, type);
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);
  if (param == NULL) {
    SyntaxError(syntax, "Expected template parameter name");
    SyntaxRecover(syntax, TC(closebra));
    return false;
  }
  param->flags.invented = true;
  param->flags.is_template_parameter = true;
  param->flags.is_template_type_parameter = false;
  param->template_parameter_index = index;
  bool added = SyntaxAddSymbol(syntax, param);
  if (!added) {
    SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
    SymbolDelete(param);
  }
  VectorAppend(params, NewTemplateParameter(param->name.value,
                                            kTemplateParameterNonType,
                                            param->type, index));
  return true;
}

static Vector* ParseTemplateParameterList(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (!LexMatch(lex, TOK(less))) {
    SyntaxError(syntax, "Expected '<' after template");
    return NewVector();
  }
  Vector* params = NewVector();
  while (!LexEof(lex) && !LexLookingAt(lex, TOK(greater))) {
    ParseTemplateParameter(syntax, params);
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(greater), TC(decl));
  return params;
}

Vector* SyntaxParseTemplateArgumentList(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  if (!LexMatch(lex, TOK(less))) {
    return NULL;
  }
  Vector* args = NewVector();
  while (!LexEof(lex) && !LexLookingAt(lex, TOK(greater))) {
    TemplateArgument* arg = malloc(sizeof(TemplateArgument));
    arg->kind = kTemplateParameterType;
    arg->type = NULL;
    arg->int_value = 0;
    arg->template_parameter_index = -1;
    if (SyntaxLookingAtType(syntax)) {
      TypeParser parser;
      TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
      TypeRecord* type = TypeParserParseType(&parser, true);
      Symbol* sym = TypeParserParseDeclarator(&parser, type);
      TypeParserDestruct(&parser);
      if (sym != NULL) {
        arg->type = TypeRecordCopy(sym->type);
        SymbolDelete(sym);
      } else {
        arg->type = type;
      }
    } else {
      bool old_parsing_template_argument = syntax->parsing_template_argument;
      syntax->parsing_template_argument = true;
      ASTNode* expr = SyntaxParseSingleExpression(syntax,
                                                  TC(closebra) | TC(exprsep));
      syntax->parsing_template_argument = old_parsing_template_argument;
      expr = AnalyzeExpression(expr);
      int64_t value = 0;
      if (!EvaluateIntegerExpression(expr, &value)) {
        if (syntax->parsing_template_declaration &&
            expr->op == AST_OP(identifier)) {
          IdentifierASTNode* id = (IdentifierASTNode*)expr;
          if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
              !id->symbol->flags.is_template_type_parameter) {
            arg->template_parameter_index =
                id->symbol->template_parameter_index;
          }
        }
        if (arg->template_parameter_index < 0) {
          SyntaxError(syntax,
                      "Template non-type argument must be an integer constant expression");
        }
      }
      arg->kind = kTemplateParameterNonType;
      arg->int_value = value;
      ASTNodeDelete(expr);
    }
    VectorAppend(args, arg);
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(greater), followers);
  return args;
}

static bool TypeContainsClassTemplate(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->info.struct_info != NULL &&
        t->info.struct_info->is_template) {
      return true;
    }
  }
  return false;
}

static void MarkTemplateDeclaration(Syntax* syntax, ASTNode* node) {
  if (node == NULL || node->op != AST_OP(decl_list)) {
    return;
  }
  DeclarationListASTNode* list = (DeclarationListASTNode*)node;
  bool marked_symbol = false;
  for (size_t i = 0; i < list->declarations->length; i++) {
    ASTNode* decl = list->declarations->value.p[i];
    if (decl != NULL && decl->op == AST_OP(vardecl)) {
      VariableDeclarationASTNode* var = (VariableDeclarationASTNode*)decl;
      if (var->symbol != NULL) {
        var->symbol->flags.is_template = true;
        marked_symbol = true;
      }
    }
  }
  if (!marked_symbol && syntax->last_parsed_tag != NULL &&
      syntax->last_parsed_tag->type != NULL &&
      TypeIsStructOrUnion(syntax->last_parsed_tag->type) &&
      syntax->last_parsed_tag->type->info.struct_info != NULL) {
    syntax->last_parsed_tag->flags.is_template = true;
    syntax->last_parsed_tag->type->info.struct_info->is_template = true;
    syntax->last_parsed_tag->type->info.struct_info->template_parameter_count =
        syntax->current_template_parameter_count;
    VectorDestructWithContents(
        &syntax->last_parsed_tag->type->info.struct_info->template_parameters,
        (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
    VectorInit(&syntax->last_parsed_tag->type->info.struct_info->template_parameters);
    if (syntax->current_template_parameters != NULL) {
      for (size_t i = 0; i < syntax->current_template_parameters->length; i++) {
        VectorAppend(&syntax->last_parsed_tag->type->info.struct_info->template_parameters,
                     syntax->current_template_parameters->value.p[i]);
      }
      syntax->current_template_parameters->length = 0;
    }
    Symbol* alias = NewSymbol(syntax->last_parsed_tag->name.value,
                              syntax->last_parsed_tag->type, STO(typedef));
    alias->namespace_ = syntax->last_parsed_tag->namespace_;
    alias->flags.is_template = true;
    if (!SyntaxAddSymbol(syntax, alias)) {
      SymbolDelete(alias);
      Symbol* existing_alias =
          SyntaxFindSymbol(syntax, &syntax->last_parsed_tag->name);
      if (existing_alias != NULL && StorageIs(existing_alias->storage, STO(typedef))) {
        existing_alias->flags.is_template = true;
      }
    }
  }
}

static ASTNode* ParseTemplateDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // template

  SyntaxOpenScope(syntax);
  LocalSymbolTable* template_tag_scope = syntax->local_tag_stack;
  syntax->local_tag_stack = template_tag_scope->prev;
  int old_template_parameter_count = syntax->current_template_parameter_count;
  Vector* old_template_parameters = syntax->current_template_parameters;
  syntax->current_template_parameters = ParseTemplateParameterList(syntax);
  syntax->current_template_parameter_count =
      (int)syntax->current_template_parameters->length;
  syntax->last_parsed_tag = NULL;
  bool old_parsing_template = syntax->parsing_template_declaration;
  syntax->parsing_template_declaration = true;
  ASTNode* declaration = SyntaxParseExternalDeclaration(syntax);
  syntax->parsing_template_declaration = old_parsing_template;
  syntax->local_tag_stack = template_tag_scope;
  SyntaxCloseScope(syntax);
  MarkTemplateDeclaration(syntax, declaration);
  VectorDestructWithContents(syntax->current_template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  syntax->current_template_parameter_count = old_template_parameter_count;
  syntax->current_template_parameters = old_template_parameters;
  return declaration != NULL ? declaration : EmptyDeclarationList(location);
}


// Parses an external declaration (a global variable, etc.) and adds it
// to the symbol table.
ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax) {
  syntax->context = kParsingFileScope;
  if (syntax->current_namespace == NULL) {
    syntax->current_namespace = compiler->global_namespace;
  }

  if (LexLookingAt(syntax->lex, TOK(static_assert))) {
    SourceLocation location = syntax->lex->current_token_location;
    SyntaxParseStaticAssert(syntax);
    return EmptyDeclarationList(location);
  }
  if (LexLookingAt(syntax->lex, TOK(namespace))) {
    return ParseNamespaceDeclaration(syntax);
  }
  if (LexLookingAt(syntax->lex, TOK(template))) {
    return ParseTemplateDeclaration(syntax);
  }
  if (LexLookingAt(syntax->lex, TOK(using))) {
    return ParseUsingDeclaration(syntax);
  }
  if (CurrentLineLooksLikeSpecialMemberDefinition(syntax)) {
    return ParseCXXSpecialMemberDefinition(syntax);
  }

  Vector* declarations = NewVector();
  Vector attributes = {0};
  
  // Parse common __attribute__ syntax.
  while (LexMatch(syntax->lex, TOK(attribute))) {
    SyntaxParseAttribute(syntax, &attributes);
  }

  Storage storage = STO(implicit);
  bool is_inline = false;
  TypeRecord* type = NULL;
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &type, &attributes, kParsingFileScope);

  if (StorageIs(storage, STO(auto)|STO(register))) {
    SyntaxError(syntax, "Illegal global storage specified: %s",
                StorageIs(storage, STO(register)) ? "register" : "auto");
    storage = STO(implicit);
  }

  // Parse common __attribute__ syntax.
  while (LexMatch(syntax->lex, TOK(attribute))) {
    SyntaxParseAttribute(syntax, &attributes);
  }

  // Create a type parser for the declarators.
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingFileScope);
  parser.is_inline = is_inline;
 
  // Open a scope (local symbol stack) for the symbols declared in the
  // type declaration list.
  SyntaxOpenScope(syntax);

  // Claim a reference on the freshly built base type for the duration of
  // declarator parsing.  Each declarator that adopts it takes its own
  // reference, so releasing ours afterwards frees the base type when no
  // declarator used it (e.g. a bare `struct S { ... };` or `enum E { ... };`).
  TypeRecordIncRef(type);

  // Now we get a sequence of declarations, separated by commas.
  ASTNode* result = ParseExternalDeclarationList(&parser,
                                                 type, storage,
                                                 &attributes,
                                                 declarations);
  SyntaxCloseScope(syntax);
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);
  if (result != NULL) {
    if (declarations->length != 1) {
      SyntaxError(syntax, "Cannot mix function definition with declaration");
    }
    // result is a DeclarationListASTNode that owns `declarations`; its teardown
    // frees the vector and its contents, so don't free them here.
    AttributeListDestruct(&attributes);
    return result;
  }

  // The declaration is followed by a semicolon.
  SyntaxNeedSemicolon(syntax, TC(type));

  AttributeListDestruct(&attributes);
  
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

// We have consumed the open brace, skip tokens until we find the
// matching close brace (or EOF).
static void SkipFunctionBody(Syntax* syntax) {
  Lex* lex = syntax->lex;
  int brace_count = 1;
  while (brace_count > 0 && !LexEof(lex)) {
    if (LexLookingAt(lex, TOK(lbrace))) {
      brace_count++;
    } else if (LexLookingAt(lex, TOK(rbrace))) {
      brace_count--;
    }
    LexNextToken(lex);
  }
}

static const char* CXXConstructorNameForType(TypeRecord* type) {
  if (!CompilerIsCXX() || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return type->info.struct_info->tag_name->value;
}

static ASTNode* NewCXXConstructorCall(Syntax* syntax, Symbol* sym,
                                      Vector* actuals,
                                      SourceLocation location) {
  (void)syntax;
  const char* constructor_name = CXXConstructorNameForType(sym->type);
  if (constructor_name == NULL) {
    VectorDelete(actuals);
    return NULL;
  }

  ASTNode* receiver = NewIdentifierASTNode(sym, location);
  ASTNode* member = NewStringConstantASTNode(NewString(constructor_name), NULL,
                                            location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
}

static StructMember* FindCXXConstructor(TypeRecord* type) {
  const char* constructor_name = CXXConstructorNameForType(type);
  if (constructor_name == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, constructor_name);
  StructMember* ctor = FindStructMember(type->info.struct_info, &name);
  StringDestruct(&name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}

static Vector* ParseCXXInitializerArgumentList(Syntax* syntax, Token close) {
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, close)) {
    ASTNode* actual =
        SyntaxParseSingleExpression(syntax, TC(closebra) | TC(exprsep));
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, close, TC(exprsep) | TC(decl));
  return actuals;
}

static ASTNode* ParseCXXDirectInitializer(Syntax* syntax, Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || !TypeIsStructOrUnion(sym->type) ||
      FindCXXConstructor(sym->type) == NULL) {
    return NULL;
  }

  SourceLocation location = syntax->lex->current_token_location;
  Vector* actuals = NULL;
  if (LexMatch(syntax->lex, TOK(lparen))) {
    actuals = ParseCXXInitializerArgumentList(syntax, TOK(rparen));
  } else if (LexMatch(syntax->lex, TOK(lbrace))) {
    actuals = ParseCXXInitializerArgumentList(syntax, TOK(rbrace));
  } else {
    return NULL;
  }
  return NewCXXConstructorCall(syntax, sym, actuals, location);
}

static ASTNode* NewCXXDefaultConstructorCallIfNeeded(Syntax* syntax,
                                                    Symbol* sym) {
  const char* constructor_name = CXXConstructorNameForType(sym->type);
  if (constructor_name == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, constructor_name);
  StructMember* ctor = FindStructMember(sym->type->info.struct_info, &name);
  StringDestruct(&name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return NewCXXConstructorCall(syntax, sym, NewVector(), sym->location);
}

static ASTNode* NewCXXDestructorCallIfNeeded(Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || !TypeIsStructOrUnion(sym->type) ||
      sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  SourceLocation location = sym->location;
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, sym->type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(sym->type->info.struct_info, &destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      !destructor->symbol->type->info.function.is_destructor) {
    StringDestruct(&destructor_name);
    return NULL;
  }
  ASTNode* member =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL,
                               location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location,
                       NewIdentifierASTNode(sym, location), member);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                       NewVector());
  return NewExpressionStatementASTNode(call, location);
}

static ASTNode* NewIntAssignment(Symbol* sym, int value,
                                 SourceLocation location) {
  return NewBinaryASTNode(
      AST_OP(assign), sym->type, location,
      NewIdentifierASTNode(sym, location),
      NewIntConstantASTNode(value,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
}

static void RegisterCXXLocalStaticDestructor(Symbol* sym, Symbol* guard) {
  ASTNode* destructor = NewCXXDestructorCallIfNeeded(sym);
  if (destructor == NULL) {
    return;
  }
  SourceLocation location = sym->location;
  ASTNode* condition = NewBinaryASTNode(
      AST_OP(noteq), NULL, location, NewIdentifierASTNode(guard, location),
      NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
  Vector* destructor_statements = NewVector();
  VectorAppend(destructor_statements, destructor);
  ASTNode* guarded_destructor = NewIfStatementASTNode(
      condition, NewCompoundStatementASTNode(destructor_statements, location),
      NULL, location);
  VectorAppend(&compiler->cxx_global_destructor_calls, guarded_destructor);
}

static ASTNode* NewCXXLocalStaticGuardedConstructor(Syntax* syntax,
                                                   Symbol* sym) {
  ASTNode* constructor = NewCXXDefaultConstructorCallIfNeeded(syntax, sym);
  if (constructor == NULL) {
    return NULL;
  }

  SourceLocation location = sym->location;
  Symbol* guard =
      NewSymbol(SyntaxFakeName(syntax),
                NewTypeRecordWithSize(kTypeInt, kQualPlain), STO(static));
  guard->flags.invented = true;
  guard->flags.is_defined = true;
  guard->flags.is_local = true;
  guard->location = location;
  bool added = SyntaxAddSymbol(syntax, guard);
  assert(added);
  (void)added;
  VectorAppend(&syntax->local_statics,
               NewVariableDeclarationASTNode(guard, NULL, location));
  RegisterCXXLocalStaticDestructor(sym, guard);

  Vector* guarded_statements = NewVector();
  VectorAppend(guarded_statements,
               NewExpressionStatementASTNode(constructor, location));
  VectorAppend(guarded_statements,
               NewExpressionStatementASTNode(
                   NewIntAssignment(guard, 1, location), location));

  ASTNode* condition = NewBinaryASTNode(
      AST_OP(equal), NULL, location, NewIdentifierASTNode(guard, location),
      NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
  ASTNode* guarded =
      NewIfStatementASTNode(condition,
                            NewCompoundStatementASTNode(guarded_statements,
                                                        location),
                            NULL, location);

  Vector* statements = NewVector();
  VectorAppend(statements, guarded);
  return NewUnaryASTNode(AST_OP(stmt_expr),
                         NewTypeRecordWithSize(kTypeVoid, kQualPlain),
                         location, NewCompoundStatementASTNode(statements,
                                                               location));
}

static ASTNode* NewVariableInitExpression(Syntax* syntax, Symbol* sym,
                                          ASTNode* initializer) {
  ASTNode* decl_id =
      NewIdentifierASTNode(sym, syntax->lex->current_token_location);
  decl_id->flags |= kASTNeedAddress;
  ASTNode* init = NewBinaryASTNode(AST_OP(init), sym->type,
                                  syntax->lex->current_token_location, decl_id,
                                  initializer);
  decl_id->flags |= kASTIsDeclaration;
  return init;
}

static void ParseLocalDeclarationList(TypeParser* parser,
                                      TypeRecord* type, Storage storage,
                                      Vector* attributes, Vector* declarations) {
  Syntax* syntax = parser->syntax;
  while (!LexEof(syntax->lex)) {
    if (LexLookingAt(syntax->lex, TOK(semicolon))) {
      // We don't need to have a variable declaration.
      break;
    }
    Symbol* sym = TypeParserParseDeclarator(parser, type);
    if (sym != NULL) {
      Symbol* old_sym =
          FindTopLocalSymbol(syntax->local_symbol_stack, &sym->name);
      bool ok = true;
      if (old_sym != NULL) {
        // We have this symbol already.  If it's a declaration then it's
        // OK to declare (and define) it now.  If it's a definition then
        // this must be a declaration.
        if (old_sym->flags.is_defined) {
          if (StorageIs(storage, STO(extern))) {
            // This might be a declaration, only if there is no initializer
            if (LexLookingAt(parser->lex, TOK(equal))) {
              SyntaxError(syntax,
                          "Extern variable can't have an initializer: %s",
                          sym->name.value);
              ok = false;
            }
          } else {
            if (IsDefinition(parser, old_sym, storage)) {
              // This is a declaration of a previously known definition.
              SyntaxError(syntax, "Duplicate definition of local symbol %s",
                          sym->name.value);
              ok = false;
            }
          }
        } else {
          // Old sym is a declaration.
          if (IsDefinition(parser, sym, storage)) {
            // This is a definition so the original symbol is now defined.
            old_sym->flags.is_defined = true;
          } else if (!StorageIs(storage, STO(extern))) {
            old_sym->flags.is_tentative_decl = true;
          }
        }

        if (!TypeEqual(sym->type, old_sym->type)) {
          SyntaxError(syntax, "Symbol %s redeclared with different type",
                      sym->name.value);
          TypeErrorDetails(syntax->lex->current_token_location,
                                    sym->type, old_sym->type);
           const char* filename;
           int lineno, start, end;
           DecodeSourceLocation(old_sym->location, &filename, &lineno, &start, &end);
           ReportNote(filename, lineno, "Previously declared here");
        } else {
          // Symbol declaration is the same type as the definition, make sure
          // the linkage matches.
          Storage old_storage = old_sym->storage & ~STO(extern);
          Storage new_storage = sym->storage & ~STO(extern);

          if (old_storage != new_storage) {
            SyntaxError(syntax, "Symbol %s redeclared with different linkage",
                        sym->name.value);
            ok = false;
          }
        }

        if (ok) {
          // We now refer to the previously defined symbol rather than this new
          // one.
          SymbolDelete(sym);
          sym = old_sym;
        }
      } else {
        // This is the first declaration of this symbol, add to the symbol
        // table.
        bool added = SyntaxAddSymbol(syntax, sym);
        assert(added);
        (void)added;
      }
    }

    // If we don't have a symbol in the type declarator we look for a comma and
    // continue parsring.
    if (sym == NULL) {
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      TypeParserReset(parser);
      continue;
    }

    if (!StorageIs(sym->storage, STO(extern))) {
      // This is a local variable.
      sym->flags.is_local = true;
    }

    // Parse trailing __attribute__ syntax (e.g. `int x __attribute__((...))`).
    while (LexMatch(syntax->lex, TOK(attribute))) {
      SyntaxParseAttribute(syntax, attributes);
    }

    // Symbol takes ownerhip of attribute strings.
    VectorCopy(&sym->attributes, attributes);
    VectorClear(attributes);
    ApplyDeclarationAttributes(sym);
    
    // Check for __thread violations.
    CheckThreadLocal(syntax, sym);

    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      if (TypeContainsAuto(sym->type)) {
        SyntaxError(syntax, "auto function return type is not supported yet");
      }
      if (LexMatch(syntax->lex, TOK(lbrace))) {
        // C does not supported nested functions.
        SyntaxError(syntax, "Function definition not allowed here");
        // Skip function body so that we can attempt to recover.
        SkipFunctionBody(syntax);
        // Continue on to check for comma, which probably won't exist
        // then we will exit the loop.
      }
    } else {
      sym->flags.is_defined = true;
      if (parser->is_inline) {
         SyntaxError(syntax, "inline can only be applied to functions");
      }
      if (TypeIsAbstractClass(sym->type)) {
        SyntaxError(syntax, "Cannot declare object of abstract class %s",
                    sym->type->info.struct_info->tag_name != NULL
                        ? sym->type->info.struct_info->tag_name->value
                        : "<anonymous>");
      }
      if (!syntax->parsing_template_declaration &&
          TypeContainsClassTemplate(sym->type)) {
        SyntaxError(syntax, "Class template instantiation is not supported yet");
      }
      // Any initializer?
      ASTNode* initializer = NULL;
      if (LexMatch(syntax->lex, TOK(equal))) {
        syntax->init_storage = storage;
        initializer = SyntaxParseInitializer(syntax, sym, storage);
        initializer = NewVariableInitExpression(syntax, sym, initializer);
      } else {
        initializer = ParseCXXDirectInitializer(syntax, sym);
        if (initializer == NULL && CompilerIsCXX() &&
            LexMatch(syntax->lex, TOK(lbrace))) {
          initializer =
              NewVariableInitExpression(syntax, sym, ParseBracedInitializer(syntax));
        }
        if (initializer == NULL) {
          if (TypeContainsAuto(sym->type)) {
            SyntaxError(syntax, "auto variable requires an initializer");
          } else if (StorageIs(storage, STO(static))) {
            initializer = NewCXXLocalStaticGuardedConstructor(syntax, sym);
          } else {
            initializer = NewCXXDefaultConstructorCallIfNeeded(syntax, sym);
          }
        }
      }

      ASTNode* decl = NewVariableDeclarationASTNode(
          sym, initializer, syntax->lex->current_token_location);
      VectorAppend(declarations, decl);
      if (TypeIsConst(sym->type) && !TypeIsStructOrUnion(sym->type)) {
        SemanticAnalyzeVariableDefinition(syntax,
                                        (VariableDeclarationASTNode*)decl);
      }
      
      if (StorageIs(storage, STO(static))) {
        VectorAppend(&syntax->local_statics, decl);
      }
    }

    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
    TypeParserReset(parser);
  }
}

// Parses a local symbol declaration or definition.  This occurs inside a
// function.  The symbol is added to the local scope (top symbol table in the
// local symbol stack).
ASTNode* SyntaxParseLocalDeclaration(Syntax* syntax) {
  if (LexLookingAt(syntax->lex, TOK(static_assert))) {
    SourceLocation location = syntax->lex->current_token_location;
    SyntaxParseStaticAssert(syntax);
    return EmptyDeclarationList(location);
  }
  if (LexLookingAt(syntax->lex, TOK(using))) {
    return ParseUsingDeclaration(syntax);
  }

  Vector* declarations = NewVector();

  Storage storage = STO(implicit);
  bool is_inline = false;
  TypeRecord* type = NULL;
  Vector attributes = {0};
  syntax->context = kParsingBlockScope;
  
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &type, &attributes, kParsingBlockScope);

  if (is_inline) {
    SyntaxError(syntax, "inline is not allowed here");
  }
  
  // Parse the type specifier (char, unsigned int, etc.)
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingBlockScope);

  // Claim a reference on the freshly built base type for the duration of
  // declarator parsing.  Each declarator that adopts it takes its own
  // reference, so releasing ours afterwards frees the base type when no
  // declarator used it (e.g. a bare `struct S { ... };`).
  TypeRecordIncRef(type);

  // Now we get a sequence of declarations, separated by commas.
  ParseLocalDeclarationList(&parser, type, storage, &attributes, declarations);
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);

  // The declaration is followed by a semicolon.
  SyntaxNeedSemicolon(syntax, TC(type));

  AttributeListDestruct(&attributes);
  
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

void SyntaxNeedBracket(Syntax* syntax, Token bracket, TokenClass followers) {
  if (!LexMatch(syntax->lex, bracket)) {
    SyntaxError(syntax, "Missing %s", TokenName(bracket));
    SyntaxRecover(syntax, followers);
  }
}

void SyntaxRecover(Syntax* syntax, TokenClass tc) {
  while (!LexEof(syntax->lex)) {
    TokenClass c = ClassifyToken(syntax->lex->current_token);
    if ((c & tc) != 0) {
      break;
    }
    LexNextToken(syntax->lex);
  }
}

bool SyntaxLookingAtType(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(char):
    case TOK(int):
    case TOK(short):
    case TOK(long):
    case TOK(float):
    case TOK(double):
    case TOK(class):
    case TOK(struct):
    case TOK(union):
    case TOK(enum):
    case TOK(bool):
    case TOK(signed):
    case TOK(unsigned):
    case TOK(const):
    case TOK(volatile):
    case TOK(restrict):
    case TOK(void):
      return true;
    case TOK(auto):
      return CompilerIsCXX();
    case TOK(decltype):
      return CompilerIsCXX();
    case TOK(identifier): {
      Symbol* sym = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
      if (sym == NULL) {
        return SyntaxCurrentTokenStartsQualifiedName(syntax);
      }
      if (StorageIs(sym->storage , STO(typedef))) {
        return true;
      }
      return false;
    }
    case TOK(coloncolon):
      return SyntaxCurrentTokenStartsQualifiedName(syntax);
    default:
      return false;
  }
}

bool SyntaxLookingAtDeclaration(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(extern):
    case TOK(static):
    case TOK(auto):
    case TOK(register):
    case TOK(typedef):
    case TOK(using):
    case TOK(static_assert):
      return true;
    default:
      return SyntaxLookingAtType(syntax);
  }
}

// Opens a new scope by pushing a new symbol table into the local symbol stack
// and local tag stack.
void SyntaxOpenScope(Syntax* syntax) {
  LocalSymbolTable* table = NewLocalSymbolTable();
  table->prev = syntax->local_symbol_stack;
  syntax->local_symbol_stack = table;

  LocalSymbolTable* tag_table = NewLocalSymbolTable();
  tag_table->prev = syntax->local_tag_stack;
  syntax->local_tag_stack = tag_table;
}

// Closes a local variable scope.  This does not free the symbols contained
// in the symbol table at the discarded scope.  These are held in the
// all_local_symbols vector in the Syntax object and are kept until the
// compilation of the function is complete.
void SyntaxCloseScope(Syntax* syntax) {
  LocalSymbolTable* prev = syntax->local_symbol_stack->prev;
  LocalSymbolTableDelete(syntax->local_symbol_stack);
  syntax->local_symbol_stack = prev;

  prev = syntax->local_tag_stack->prev;
  LocalSymbolTableDelete(syntax->local_tag_stack);
  syntax->local_tag_stack = prev;
}

Symbol* SyntaxNewTemporary(Syntax* syntax, struct TypeRecord* type) {
  Symbol* sym = NewSymbol(SyntaxFakeName(syntax), type, STO(implicit));
  sym->flags.is_temp = true;
  sym->flags.invented = true;
  SyntaxAddSymbol(syntax, sym);
  return sym;
}

TokenClass ClassifyToken(Token tok) {
  switch (tok) {
    case TOK(bad):
    case TOK(eof):
      return 0;

    case TOK(number):
    case TOK(string):
    case TOK(string_wide):
    case TOK(charconst):
    case TOK(charconst_wide):
    case TOK(fnumber):
    case TOK(caret):
    case TOK(ellipsis):
    case TOK(false):
    case TOK(sizeof):
    case TOK(tilde):
      return TC(expr);

    case TOK(identifier):
      return TC(expr) | TC(stmt);

    case TOK(amp):
    case TOK(ampeq):
    case TOK(arrow):
    case TOK(bang):
    case TOK(bar):
    case TOK(careteq):
    case TOK(dot):
    case TOK(comma):
    case TOK(equalequal):
    case TOK(greater):
      return TC(exprsep);


    case TOK(equal):
      return TC(stmt) | TC(exprsep);

    case TOK(auto):
    case TOK(break):
    case TOK(case):
    case TOK(continue):
    case TOK(default):
    case TOK(do):
    case TOK(for):
    case TOK(if):
    case TOK(goto):
    case TOK(return ):
    case TOK(switch):
    case TOK(while):
    case TOK(asm):
    case TOK(attribute):
    case TOK(hash):
      return TC(stmt);

    case TOK(semicolon):
      return TC(stmt) | TC(semicolon);

    case TOK(extern):
    case TOK(inline):
    case TOK(register):
    case TOK(static):
    case TOK(typedef):
      return TC(stmt) | TC(decl);

    case TOK(bool):
    case TOK(char):
    case TOK(class):
    case TOK(complex):
    case TOK(const):
    case TOK(double):
    case TOK(else):
    case TOK(float):
    case TOK(enum):
    case TOK(imaginary):
    case TOK(int):
    case TOK(long):
    case TOK(restrict):
    case TOK(short):
    case TOK(signed):
    case TOK(union):
    case TOK(unsigned):
    case TOK(void):
    case TOK(volatile):
    case TOK(wchar_t):
    case TOK(struct):
      return TC(type) | TC(stmt) | TC(decl);

    case TOK(colon):
      return TC(stmt) | TC(exprsep);

    case TOK(greatereq):
    case TOK(less):
    case TOK(lesseq):
    case TOK(ampamp):
    case TOK(barbar):
    case TOK(lessless):
    case TOK(lesslesseq):
    case TOK(minus):
    case TOK(minuseq):
    case TOK(minusminus):
    case TOK(bangeq):
    case TOK(bareq):
    case TOK(percent):
    case TOK(percenteq):
    case TOK(plus):
    case TOK(pluseq):
    case TOK(plusplus):
    case TOK(question):
    case TOK(greatergreater):
    case TOK(slash):
    case TOK(slasheq):
    case TOK(star):
    case TOK(stareq):
      return TC(exprsep);

    case TOK(lbrace):
      return TC(stmt);
    case TOK(lparen):
    case TOK(lsquare):
      return TC(exprsep) | TC(openbra);
    case TOK(rbrace):
      return TC(closebrace);
    case TOK(rparen):
    case TOK(rsquare):
      return TC(closebra);

    default:
      return 0;
  }
}
