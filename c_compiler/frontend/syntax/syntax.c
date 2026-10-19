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

jmp_buf error_abort_state;       // Where to abort to.

bool abort_on_error;

static int next_pc_label_id = 0;

static String next_pc_label;

// Returns true when `syntax` is parsing inside a user-defined namespace rather
// than the global namespace.
bool InNamedNamespace(Syntax* syntax) {
  return syntax->current_namespace != NULL &&
         syntax->current_namespace != compiler->global_namespace;
}

// Initializes `syntax` for a translation unit or fragment parse using `lex`.
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
  VectorInit(&syntax->inline_static_member_definitions);
  VectorInit(&syntax->all_symbols);
  syntax->last_parsed_tag = NULL;
  syntax->parsing_template_declaration = false;
  syntax->parsing_template_specialization = false;
  syntax->parsing_explicit_instantiation = false;
  syntax->parsing_template_argument = false;
  syntax->parsing_friend_type_specifier = false;
  syntax->parsing_friend_declaration_specifier = false;
  syntax->case_label_chains_to_next_label = false;
  syntax->parsing_default_member_initializer = false;
  syntax->expression_nesting_depth = 0;
  syntax->struct_definition_depth = 0;
  syntax->enum_bodies_parsed = 0;
  syntax->parsing_lambda_body_depth = 0;
  syntax->parsing_consteval_block_depth = 0;
  syntax->parsing_enum_specifier_depth = 0;
  syntax->current_template_parameter_count = 0;
  syntax->current_template_parameters = NULL;
  syntax->current_template_requires_clause = NULL;
  syntax->pending_explicit_condition = NULL;
  syntax->pending_placeholder_variable_constraint = NULL;
  syntax->cxx_class_head = NULL;
  syntax->defer_template_ids_naming_class_head = false;
  VectorInit(&syntax->pending_member_return_types);
  syntax->template_substitution_source = NULL;
  syntax->template_substitution_target = NULL;
  syntax->enclosing_template_substitution_source = NULL;
  syntax->enclosing_template_substitution_target = NULL;
  syntax->deferred_friend_bodies = NULL;
  syntax->context = kParsingFileScope;
  syntax->c_linkage = false;
  syntax->explicit_cxx_linkage = false;
  syntax->export_depth = 0;
  syntax->eof_missing_bracket = TOK(eof);
  syntax->eof_expected_semicolon = false;
}

// Releases `syntax` state, including accumulated local symbols, pending
// constraints, and the parse AST root.
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
  VectorDestruct(&syntax->inline_static_member_definitions);
  VectorDestruct(&syntax->pending_member_return_types);
  ConstraintExprDelete(syntax->current_template_requires_clause);
  syntax->current_template_requires_clause = NULL;
  ConstraintExprDelete(syntax->pending_placeholder_variable_constraint);
  syntax->pending_placeholder_variable_constraint = NULL;
  ASTNodeDelete(syntax->ast);
}

// Clears per-declaration parse state in `syntax` while retaining local symbols
// from the finished declaration in `all_symbols` for later code generation.
void SyntaxResetForNewDeclaration(Syntax* syntax) {
  // Retain the just-finished declaration's local symbols until end of
  // compilation: code generation has run, but they may still be referenced by
  // the per-function target code that is emitted after the whole parse loop.
  // Accumulate (not overwrite - VectorCopy clears first) so symbols from every
  // declaration survive; SyntaxDestruct frees them all.
  VectorAppendVector(&syntax->all_symbols, &syntax->all_local_symbols);

  VectorDestruct(&syntax->all_local_symbols);
  VectorDestruct(&syntax->local_statics);
  VectorDestruct(&syntax->inline_static_member_definitions);
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
  syntax->parsing_template_specialization = false;
  syntax->parsing_explicit_instantiation = false;
  syntax->parsing_template_argument = false;
  syntax->parsing_consteval_block_depth = 0;
  syntax->parsing_enum_specifier_depth = 0;
  syntax->current_template_parameter_count = 0;
  syntax->current_template_parameters = NULL;
  ConstraintExprDelete(syntax->current_template_requires_clause);
  syntax->current_template_requires_clause = NULL;
  // Drop any deferred explicit(bool) condition that was parsed but not attached
  // to a function (e.g. after error recovery) so it cannot bleed into the next
  // declaration.  The node itself is arena-owned, so only the handle is cleared.
  syntax->pending_explicit_condition = NULL;
  ConstraintExprDelete(syntax->pending_placeholder_variable_constraint);
  syntax->pending_placeholder_variable_constraint = NULL;
  syntax->template_substitution_source = NULL;
  syntax->template_substitution_target = NULL;
  syntax->enclosing_template_substitution_source = NULL;
  syntax->enclosing_template_substitution_target = NULL;
  VectorInit(&syntax->all_local_symbols);
  VectorInit(&syntax->local_statics);
  VectorInit(&syntax->inline_static_member_definitions);
}

// Allocates a fresh internal label AST node at `location` for control-flow
// lowering; the returned node is owned by the AST arena.
ASTNode* SyntaxNewPCLabel(SourceLocation location) {
  StringInit(&next_pc_label, NULL);
  StringPrintf(&next_pc_label, ".PC.%d", next_pc_label_id++);
  ASTNode* node = NewLabelASTNode(next_pc_label.value, NULL, true, location);
  StringDestruct(&next_pc_label);
  return node;
}

// C++ unqualified lookup searches the current class scope -- including base
// classes -- before the enclosing namespace scopes.  A class's own members are
// injected into the local symbol stack while its body is parsed, but members
// *inherited* from base classes are not, so an unqualified reference to an
// inherited nested type (e.g. `iostate`) or inherited static constant (e.g.
// `goodbit`) is invisible to the ordinary lookup below even though the qualified
// forms (`Base::iostate`, `Base::goodbit`) already resolve via FindStructMember's
// base walk.  Recover such members here.
//
// Only members usable without an object are returned -- typedefs, static
// members, and members with a compile-time value (enum constants / constexpr) --
// exactly matching what the qualified `Owner::name` path accepts.  A non-static
// data member reached by unqualified name inside a member-function body is
// handled elsewhere (name hiding through `this`); returning it from here would
// change expression name resolution.
static Symbol* FindInheritedClassMember(Syntax* syntax, String* name) {
  if (!CompilerIsCXX()) {
    return NULL;
  }
  Struct* owner = syntax->cxx_class_head;
  if (owner == NULL) {
    // Not inside a class body (or its base clause).  We may still be parsing a
    // member-function body, in which case the enclosing class is recoverable
    // from the function in flight.  Require an open local scope so a stale
    // current_function left over between file-scope declarations cannot leak an
    // inherited name into namespace-scope lookups.
    if (syntax->local_symbol_stack != NULL &&
        compiler->current_function != NULL &&
        TypeIsFunction(compiler->current_function)) {
      owner = compiler->current_function->info.function.cxx_member_owner;
    }
  }
  if (owner == NULL) {
    return NULL;
  }
  // A nested class is in the scope of its enclosing class, so names such as
  // `size_type` written inside `Outer<T>::Nested` resolve to `Outer<T>`'s
  // members (including through further enclosing classes).
  for (Struct* scope = owner; scope != NULL; scope = scope->lexical_parent) {
    StructMember* member = FindStructMember(scope, name);
    if (member == NULL || member->symbol == NULL) {
      continue;
    }
    // Nested class / enum names are also usable without an object
    // (`store_by_value<T>::value` written in a sibling nested class).
    bool nested_type =
        member->symbol->type != NULL &&
        (TypeIsStructOrUnion(member->symbol->type) ||
         TypeIsEnum(member->symbol->type));
    if (member->is_static || nested_type ||
        StorageIs(member->symbol->storage, STO(typedef)) ||
        member->symbol->flags.value_set) {
      return member->symbol;
    }
  }
  return NULL;
}

/* Find an injected-class-name inherited by the current class without treating
 * the base's constructor overload (which has the same spelling) as the type. */
Symbol* FindCurrentInheritedInjectedClassName(Syntax* syntax,
                                              String* name) {
  if (!CompilerIsCXX() || syntax == NULL || name == NULL) {
    return NULL;
  }
  Struct* owner = syntax->cxx_class_head;
  if (owner == NULL && syntax->context == kParsingBlockScope &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  for (Struct* scope = owner; scope != NULL; scope = scope->lexical_parent) {
    StructMember* member = FindStructMember(scope, name);
    if (member != NULL && member->symbol != NULL &&
        (!member->is_member_function ||
         !TypeIsFunction(member->symbol->type) ||
         !member->symbol->type->info.function.is_constructor)) {
      return NULL;
    }
    Symbol* injected = FindInheritedInjectedClassName(scope, name);
    if (injected != NULL && injected->type != NULL) {
      return injected;
    }
  }
  return NULL;
}

// Returns the class that owns `symbol` when it is a member function or static
// data member, or NULL otherwise.
static Struct* SymbolMemberOwner(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL) {
    return NULL;
  }
  if (TypeIsFunction(symbol->type)) {
    return symbol->type->info.function.cxx_member_owner;
  }
  return symbol->static_data_member_class;
}

// The enclosing class's members stay in an open scope while a nested class's
// deferred member bodies are parsed, but the nested class's own members
// hide them.
static Symbol* FindNearerNestedClassMember(Syntax* syntax, Symbol* found,
                                           String* name) {
  Struct* found_owner = SymbolMemberOwner(found);
  if (!CompilerIsCXX()) {
    return NULL;
  }
  // A template parameter lives in the innermost declaration scope and always
  // hides members of enclosing classes.  In particular, the `Value` in
  // `template<class Value> ValueImpl(Value&&)` must not be replaced by an
  // outer `using Value = ValueImpl<true>`.
  if (found != NULL && found->flags.is_template_parameter) {
    return NULL;
  }
  Struct* owner = syntax->cxx_class_head;
  if (owner == NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  // `friend class Helper;` introduces a namespace-scope tag, but a later
  // nested `class Helper { ... };` in the same class hides it.  The namespace
  // tag has no member owner, so prefer a directly nested type while parsing
  // that class rather than qualifying through the stale friend declaration.
  if (found_owner == NULL &&
      (SymbolIsTagSymbol(found) ||
       (found != NULL && StorageIs(found->storage, STO(typedef))))) {
    for (Struct* scope = owner; scope != NULL; scope = scope->lexical_parent) {
      StructMember* member = FindStructMember(scope, name);
      if (member == NULL || member->symbol == NULL ||
          member->symbol->type == NULL ||
          !TypeIsStructOrUnion(member->symbol->type) ||
          member->symbol->type->info.struct_info == NULL ||
          member->symbol->type->info.struct_info->lexical_parent != scope) {
        continue;
      }
      return member->symbol;
    }
    return NULL;
  }
  if (found_owner == NULL) {
    return NULL;
  }
  bool enclosed = false;
  for (Struct* scope = owner; scope != NULL; scope = scope->lexical_parent) {
    if (scope == found_owner) {
      enclosed = true;
      break;
    }
  }
  if (!enclosed) {
    return NULL;
  }
  for (Struct* scope = owner; scope != found_owner;
       scope = scope->lexical_parent) {
    StructMember* member = FindStructMember(scope, name);
    if (member != NULL && member->symbol != NULL) {
      return member->symbol;
    }
  }
  return NULL;
}

// Performs unqualified name lookup for `name` in the current parse context,
// following typedef aliases.  Returns NULL when lookup is ambiguous or fails.
Symbol* SyntaxFindSymbol(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_symbol_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
  if (symbol != NULL) {
    Symbol* nearer = FindNearerNestedClassMember(syntax, symbol, name);
    return FollowAlias(nearer != NULL ? nearer : symbol);
  }
  symbol = FindInheritedClassMember(syntax, name);
  if (symbol != NULL) {
    return FollowAlias(symbol);
  }
  Namespace* ns = syntax->current_namespace != NULL ? syntax->current_namespace
                                                   : compiler->global_namespace;
  bool ambiguous = false;
  symbol = LookupSymbolInEnclosingScopesCheckingAmbiguity(
      syntax, ns, name, &ambiguous);
  if (symbol != NULL) {
    return FollowAlias(symbol);
  }
  if (ambiguous) {
    return NULL;
  }
  return FollowAlias(FindGlobalSymbol(name));
}

// Marks a block-scope `_` placeholder as a C++26 name-independent declaration
// when applicable; otherwise leaves `symbol` unchanged.
void MarkCXX26AutomaticNameIndependent(Symbol* symbol) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) || symbol == NULL ||
      !StringEqual(&symbol->name, "_") || !symbol->flags.is_local ||
      symbol->flags.is_argument || symbol->flags.is_template_parameter ||
      symbol->type == NULL || TypeIsFunction(symbol->type) ||
      StorageIs(symbol->storage,
                STO(static) | STO(extern) | STO(thread) | STO(typedef))) {
    return;
  }
  symbol->flags.is_name_independent = true;
}

// Records the current lexer token location on `symbol` when it has none yet.
static void SymbolSetLocationFromLexIfMissing(Syntax* syntax, Symbol* symbol) {
  if (symbol != NULL && symbol->location == 0 && syntax != NULL &&
      syntax->lex != NULL) {
    symbol->location = syntax->lex->current_token_location;
  }
}

// Inserts `symbol` into the current local, namespace, or global symbol table.
// Returns false on a name collision; does not take ownership of `symbol`.
bool SyntaxAddSymbol(Syntax* syntax, Symbol* symbol) {
  SymbolSetLocationFromLexIfMissing(syntax, symbol);
  if (syntax->local_symbol_stack == NULL) {
    MarkExportedDeclaration(syntax, symbol);
    if (InNamedNamespace(syntax)) {
      bool ok = NamespaceInsertSymbol(syntax->current_namespace, symbol);
      if (ok && LexIsTokenReplaying(syntax->lex)) {
        CompilerRecordInjectedSymbol(
            syntax->current_namespace, NULL, symbol, /*is_tag=*/false,
            /*is_global=*/false);
      }
      return ok;
    }
    if (NamespaceFindDirectAlias(compiler->global_namespace,
                                 &symbol->name) != NULL) {
      return false;
    }
    bool ok = InsertGlobalSymbol(symbol);
    if (ok && LexIsTokenReplaying(syntax->lex)) {
      CompilerRecordInjectedSymbol(NULL, NULL, symbol, /*is_tag=*/false,
                                   /*is_global=*/true);
    }
    return ok;
  }
  MarkCXX26AutomaticNameIndependent(symbol);
  bool ok = InsertLocalSymbol(syntax->local_symbol_stack, symbol);
  if (ok) {
    if (LexIsTokenReplaying(syntax->lex)) {
      CompilerRecordInjectedSymbol(NULL, syntax->local_symbol_stack, symbol,
                                   /*is_tag=*/false, /*is_global=*/false);
    }
    // Remember that this name was introduced at block scope.  Per
    // [basic.lookup.argdep], if ordinary lookup for a function call finds a
    // block-scope function declaration, argument-dependent lookup is
    // suppressed.  (The ADL gate additionally filters out the invented
    // placeholder symbols synthesized for calls to as-yet-undeclared functions
    // that are meant to be found by ADL.)
    symbol->flags.is_block_scope = true;
    VectorAppend(&syntax->all_local_symbols, symbol);
  }
  return ok;
}

// Inserts `symbol` into the innermost local symbol table only, without export
// or global handling.  Returns false when not in a local scope or on collision.
bool SyntaxAddBorrowedSymbol(Syntax* syntax, Symbol* symbol) {
  if (syntax == NULL || symbol == NULL || syntax->local_symbol_stack == NULL) {
    return false;
  }
  return InsertLocalSymbol(syntax->local_symbol_stack, symbol);
}

// Returns the tag symbol for a class or enum reached via a typedef member named
// `name` in the current class scope, or NULL when no such member exists.
static Symbol* FindCurrentClassMemberTag(Syntax* syntax, String* name) {
  if (!CompilerIsCXX()) {
    return NULL;
  }
  Struct* owner = NULL;
  if (syntax->context == kParsingBlockScope &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  if (owner == NULL) {
    owner = syntax->cxx_class_head;
  }
  if (owner == NULL) {
    return NULL;
  }
  StructMember* member = NULL;
  for (Struct* scope = owner; scope != NULL && member == NULL;
       scope = scope->lexical_parent) {
    member = FindStructMember(scope, name);
  }
  if (member == NULL || member->symbol == NULL ||
      !StorageIs(member->symbol->storage, STO(typedef)) ||
      member->symbol->type == NULL) {
    return NULL;
  }
  TypeRecord* type = member->symbol->type;
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    return type->info.struct_info->tag_symbol;
  }
  if (TypeIsEnum(type) && type->info.enum_info != NULL) {
    return type->info.enum_info->tag_symbol;
  }
  return NULL;
}

// Looks up a class, struct, union, or enum tag by `name`, following typedef
// aliases.  Returns NULL when lookup is ambiguous or the tag is not found.
Symbol* SyntaxFindTag(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_tag_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
  if (symbol != NULL) {
    return FollowAlias(symbol);
  }
  symbol = FindCurrentClassMemberTag(syntax, name);
  if (symbol != NULL) {
    return FollowAlias(symbol);
  }
  Namespace* ns = syntax->current_namespace != NULL ? syntax->current_namespace
                                                   : compiler->global_namespace;
  bool ambiguous = false;
  symbol =
      LookupTagInEnclosingScopesCheckingAmbiguity(syntax, ns, name, &ambiguous);
  if (symbol != NULL) {
    return FollowAlias(symbol);
  }
  if (ambiguous) {
    return NULL;
  }
  return FollowAlias(FindGlobalTag(name));
}

// Tag lookup restricted to the innermost declarative region (top local tag
// scope, current namespace, or global), without searching enclosing scopes.
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

// Ordinary-name lookup restricted to the innermost declarative region only
// (the top local scope, or the current namespace / global namespace at
// namespace scope) -- enclosing scopes are NOT consulted.  Used to decide
// whether a using-declaration collides with something already declared in the
// very region it targets, which is the only place a redeclaration check
// applies ([namespace.udecl]).
Symbol* SyntaxFindTopScopeSymbol(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_symbol_stack;
  if (scope != NULL) {
    return FindSymbol(&scope->table, name);
  }
  if (InNamedNamespace(syntax)) {
    return NamespaceFindSymbol(syntax->current_namespace, name);
  }
  return FindGlobalSymbol(name);
}

// Inserts tag symbol `symbol` into the current local tag stack, namespace, or
// global tag table.  Returns false on collision; does not take ownership.
bool SyntaxAddTag(Syntax* syntax, Symbol* symbol) {
  SymbolSetLocationFromLexIfMissing(syntax, symbol);
  if (syntax->local_tag_stack == NULL) {
    MarkExportedDeclaration(syntax, symbol);
    if (InNamedNamespace(syntax)) {
      bool ok = NamespaceInsertTag(syntax->current_namespace, symbol);
      if (ok && LexIsTokenReplaying(syntax->lex)) {
        CompilerRecordInjectedSymbol(
            syntax->current_namespace, NULL, symbol, /*is_tag=*/true,
            /*is_global=*/false);
      }
      return ok;
    }
    if (NamespaceFindDirectAlias(compiler->global_namespace,
                                 &symbol->name) != NULL) {
      return false;
    }
    bool ok = InsertGlobalTag(symbol);
    if (ok && LexIsTokenReplaying(syntax->lex)) {
      CompilerRecordInjectedSymbol(NULL, NULL, symbol, /*is_tag=*/true,
                                   /*is_global=*/true);
    }
    return ok;
  }
  if (syntax->local_symbol_stack != NULL &&
      FindDirectLocalNamespaceAlias(syntax->local_symbol_stack,
                                    &symbol->name) != NULL) {
    return false;
  }
  bool ok = InsertLocalSymbol(syntax->local_tag_stack, symbol);
  if (ok) {
    if (LexIsTokenReplaying(syntax->lex)) {
      CompilerRecordInjectedSymbol(NULL, syntax->local_tag_stack, symbol,
                                   /*is_tag=*/true, /*is_global=*/false);
    }
    // A local class (and a lambda closure) is a block-scope tag.  Substituting
    // an enclosing function template has to rebuild it; an unmarked tag is
    // copied as an unrelated complete class and keeps the template parameter
    // (`FactoryImpl(Factory)` never sees the concrete factory type).
    symbol->flags.is_block_scope = true;
    VectorAppend(&syntax->all_local_symbols, symbol);
  }
  return ok;
}

// Reports a parse error at the lexer's current position; longjmps when
// `abort_on_error` is set.
void SyntaxError(Syntax* syntax, const char* format, ...) {
  if (abort_on_error) {
    longjmp(error_abort_state, 1);
  }
  va_list ap;
  va_start(ap, format);
  VLexError(syntax->lex, format, ap);
  va_end(ap);
}

/* Report an error at a specific, previously recorded source location rather
 * than the lexer's current position.  Used when a diagnostic is discovered long
 * after the offending token was consumed (e.g. during template substitution),
 * so the error must point back at where the construct was actually written. */
void SyntaxErrorAtLocation(Syntax* syntax, SourceLocation location,
                           const char* format, ...) {
  if (abort_on_error) {
    longjmp(error_abort_state, 1);
  }
  va_list ap;
  va_start(ap, format);
  if (VReportErrorAtLocation(location, format, ap)) {
    ReportMacroExpansionNotes(location);
  }
  va_end(ap);
}

// Reports a warning at the lexer's current position using category `warn`.
void SyntaxWarning(Syntax* syntax, const char* warn, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VLexWarning(syntax->lex, warn, format, ap);
  va_end(ap);
}

// Consumes a semicolon or emits an error and runs recovery toward `followers`.
void SyntaxNeedSemicolon(Syntax* syntax, TokenClass followers) {
  if (!LexMatch(syntax->lex, TOK(semicolon))) {
    if (LexEof(syntax->lex) && syntax->eof_expected_semicolon) {
      return;
    }
    if (LexEof(syntax->lex)) {
      syntax->eof_expected_semicolon = true;
    }
    SyntaxError(syntax, "Expected semicolon");
    SyntaxRecover(syntax, followers | TC(semicolon));
  }
}

// Returns a unique invented identifier string stored in `syntax`'s buffer for
// the duration of the parse; reused on each call.
const char* SyntaxFakeName(Syntax* syntax) {
  snprintf(syntax->fake_name_buffer, sizeof(syntax->fake_name_buffer),
           "__invented__%d", syntax->fake_name_index);
  syntax->fake_name_index++;
  return syntax->fake_name_buffer;
}

// Sets `tag_name` to an invented tag spelling unused in the innermost tag scope.
void SyntaxFakeTagName(Syntax* syntax, String* tag_name) {
  Symbol* tag;
  do {
    StringSet(tag_name, SyntaxFakeName(syntax));
    tag = SyntaxFindTopScopeTag(syntax, tag_name);
  } while (tag != NULL);
}

// Parses and consumes a storage-class specifier at the current token, or
// returns implicit storage when none is present.
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
    case TOK(thread_local):
      LexNextToken(syntax->lex);
      return STO(thread);
   default:
      return STO(implicit);
  }
}

// Returns whether the declaration being parsed for `sym` with `storage` is a
// definition rather than a mere declaration.
bool IsDefinition(TypeParser* parser, Symbol* sym, Storage storage) {
  if (StorageIs(storage, STO(extern))) {
    return false;
  }
  if (sym->flags.is_argument) {
    return true;
  }
  if (CompilerIsCXX() && parser->is_inline && !TypeIsFunction(sym->type)) {
    return true;
  }
  return LexLookingAt(parser->lex, TOK(equal)) ||
      LexLookingAt(parser->lex, TOK(lbrace));
}

// Consumes matching delimiter `bracket` or reports an error and recovers
// toward `followers`.
void SyntaxNeedBracket(Syntax* syntax, Token bracket, TokenClass followers) {
  if (!LexMatch(syntax->lex, bracket)) {
    if (LexEof(syntax->lex) && syntax->eof_missing_bracket == bracket) {
      return;
    }
    if (LexEof(syntax->lex)) {
      syntax->eof_missing_bracket = bracket;
    }
    SyntaxError(syntax, "Missing %s", TokenName(bracket));
    SyntaxRecover(syntax, followers);
  }
}

// Consumes a closing `>` that ends a template argument list, or errors and
// recovers toward `followers`.
void SyntaxNeedTemplateClose(Syntax* syntax, TokenClass followers) {
  if (!LexConsumeClosingAngle(syntax->lex)) {
    SyntaxError(syntax, "Missing >");
    SyntaxRecover(syntax, followers);
  }
}

// Advances the lexer until a token in `tc` appears at the current nesting
// depth, without consuming unmatched closing delimiters.
void SyntaxRecover(Syntax* syntax, TokenClass tc) {
  // Skip tokens until a follower of `tc` is found at the current nesting
  // depth.  An unmatched closer is left in the stream: consuming it here
  // would desynchronize every enclosing skip-until-'}' / skip-until-')' loop
  // and is what turns a single syntax error into an infinite diagnostic
  // storm or a skipped later declaration.
  Lex* lex = syntax->lex;
  int paren = 0;
  int brace = 0;
  int square = 0;
  while (!LexEof(lex)) {
    Token tok = lex->current_token;
    if (brace == 0 && tok == TOK(rbrace)) {
      break;
    }
    if (paren == 0 && tok == TOK(rparen)) {
      break;
    }
    if (square == 0 && (tok == TOK(rsquare) || tok == TOK(splice_close))) {
      break;
    }
    TokenClass c = ClassifyToken(tok);
    if (paren == 0 && brace == 0 && square == 0 && (c & tc) != 0) {
      break;
    }
    if (tok == TOK(lparen) || tok == TOK(splice_open)) {
      paren++;
    } else if ((tok == TOK(rparen)) && paren > 0) {
      paren--;
    } else if (tok == TOK(lbrace)) {
      brace++;
    } else if (tok == TOK(rbrace) && brace > 0) {
      brace--;
    } else if (tok == TOK(lsquare)) {
      square++;
    } else if ((tok == TOK(rsquare) || tok == TOK(splice_close)) &&
               square > 0) {
      square--;
    }
    LexNextToken(lex);
  }
}

// After a diagnostic, advances past the token at `location_before` when the
// lexer is still stuck there and not already on a token in `leave`.
void SyntaxEnsureProgress(Syntax* syntax, Token token_before,
                          SourceLocation location_before, int errors_before,
                          TokenClass leave) {
  if (NumErrors() == errors_before || LexEof(syntax->lex)) {
    return;
  }
  TokenClass c = ClassifyToken(syntax->lex->current_token);
  if ((c & leave) != 0) {
    return;
  }
  if (syntax->lex->current_token == token_before &&
      syntax->lex->current_token_location == location_before) {
    LexNextToken(syntax->lex);
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

// Shadow non-function data members and typedefs of `owner` into the current
// local scope so a trailing requires-clause can name them during parsing.
void SyntaxInsertClassMembersForConstraint(Syntax* syntax, Struct* owner) {
  if (syntax == NULL || owner == NULL) {
    return;
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL ||
        member->symbol->name.length == 0 || member->is_anon) {
      continue;
    }
    // A trailing requires-clause may name earlier-declared data members and
    // member typedefs (they are reachable through the implicit object
    // parameter).  Member functions participate through normal (this-based)
    // member call syntax at satisfaction time and are intentionally not
    // shadowed here.  Skip statics and functions; only surface the names that
    // would otherwise fail bare lookup.
    if (member->is_member_function) {
      continue;
    }
    InsertLocalSymbol(syntax->local_symbol_stack, member->symbol);
  }
}

// Creates an invented temporary variable of `type`, registers it in the current
// scope, and returns the new symbol (owned by the symbol tables).
Symbol* SyntaxNewTemporary(Syntax* syntax, struct TypeRecord* type) {
  Symbol* sym = NewSymbol(SyntaxFakeName(syntax), type, STO(implicit));
  sym->flags.is_temp = true;
  sym->flags.invented = true;
  SyntaxAddSymbol(syntax, sym);
  return sym;
}

// Maps `tok` to the `TokenClass` bitset used for error-recovery followers.
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
    case TOK(true):
    case TOK(sizeof):
    case TOK(alignof):
    case TOK(tilde):
    case TOK(co_await):
    case TOK(co_yield):
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
      if (CompilerIsCXX() && tok == TOK(greater)) {
        return TC(exprsep) | TC(closebra);
      }
      return TC(exprsep);

    case TOK(greatergreatereq):
      return CompilerIsCXX() ? TC(exprsep) | TC(closebra) : TC(exprsep);

    case TOK(equal):
      return TC(stmt) | TC(exprsep);

    case TOK(auto):
    case TOK(break):
    case TOK(case):
    case TOK(continue):
    case TOK(default):
    case TOK(do):
    case TOK(else):
    case TOK(for):
    case TOK(if):
    case TOK(goto):
    case TOK(return ):
    case TOK(co_return):
    case TOK(switch):
    case TOK(while):
    case TOK(asm):
    case TOK(attribute):
    case TOK(declspec):
    case TOK(hash):
      return TC(stmt);

    case TOK(semicolon):
      return TC(stmt) | TC(semicolon);

    case TOK(extern):
    case TOK(inline):
    case TOK(noreturn):
    case TOK(register):
    case TOK(static):
    case TOK(typedef):
      return TC(stmt) | TC(decl);

    case TOK(atomic):
    case TOK(bitint):
    case TOK(bool):
    case TOK(char):
    case TOK(char8_t):
    case TOK(char16_t):
    case TOK(char32_t):
    case TOK(class):
    case TOK(complex):
    case TOK(const):
    case TOK(double):
    case TOK(float):
    case TOK(enum):
    case TOK(imaginary):
    case TOK(int):
    case TOK(int128):
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
    case TOK(typeof):
    case TOK(typeof_unqual):
    case TOK(decltype):
      return TC(type) | TC(stmt) | TC(decl);

    case TOK(colon):
      return TC(stmt) | TC(exprsep);

    case TOK(greatereq):
    case TOK(spaceship):
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
      if (CompilerIsCXX() && tok == TOK(greatergreater)) {
        return TC(exprsep) | TC(closebra);
      }
      return TC(exprsep);
    case TOK(slash):
    case TOK(slasheq):
    case TOK(star):
    case TOK(stareq):
    if (CompilerIsCXX() && tok == TOK(less)) {
      return TC(exprsep) | TC(closebra);
    }
    return TC(exprsep);

    case TOK(lbrace):
      return TC(stmt);
    case TOK(lparen):
    case TOK(lsquare):
    case TOK(splice_open):
      return TC(exprsep) | TC(openbra);
    case TOK(rbrace):
      return TC(closebrace);
    case TOK(rparen):
    case TOK(rsquare):
      return TC(closebra);
    case TOK(splice_close):
      return TC(closebra) | TC(spliceclose);

    default:
      return 0;
  }
}
