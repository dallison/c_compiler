//
//  syntax_namespace.c
//  c_compiler
//
//  Namespaces: definitions, aliases, using-directives, using-declarations,
//  using-enum and alias declarations.
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

static void ImportNamespace(Syntax* syntax, Namespace* ns);

// Allocates a using-alias symbol named `name` that refers to `target`.  Clones
// `target` when it names a function so overload sets stay distinct; otherwise
// copies storage from `target`.  Caller owns the returned symbol.
static Symbol* NewUsingAliasSymbol(const char* name, Symbol* target,
                                   SourceLocation location) {
  Symbol* alias =
      target != NULL && target->type != NULL && TypeIsFunction(target->type)
          ? SymbolClone(target)
          : NewSymbol(name, NULL,
                      target != NULL ? target->storage : STO(implicit));
  alias->flags.is_using_alias = true;
  SymbolMutableExtra(alias)->alias_target = target;
  alias->location = location;
  return alias;
}

// Introduce `alias` (a using-declaration alias) into the current scope.  When
// `current_scope_only` is set, the redeclaration check considers only the
// innermost declarative region: an explicit using-declaration such as
// `using ::foo;` legitimately introduces `foo` into the current namespace even
// though the same `foo` is visible from an enclosing scope, so consulting
// enclosing scopes here would wrongly treat it as a redundant redeclaration and
// silently drop it.  Using-directives (`using namespace N;`) keep the broader
// check so an already-visible name is not shadowed.
static bool AddUsingAlias(Syntax* syntax, Symbol* alias, bool is_tag,
                          bool current_scope_only) {
  Symbol* existing;
  if (current_scope_only) {
    existing = is_tag ? SyntaxFindTopScopeTag(syntax, &alias->name)
                      : SyntaxFindTopScopeSymbol(syntax, &alias->name);
  } else {
    existing = is_tag ? SyntaxFindTag(syntax, &alias->name)
                      : SyntaxFindSymbol(syntax, &alias->name);
  }
  if (existing != NULL) {
    Symbol* existing_target = FollowAlias(existing);
    Symbol* new_target = SymbolExtraOf(alias)->alias_target != NULL
                             ? FollowAlias(SymbolExtraOf(alias)->alias_target)
                             : NULL;
    if (existing_target != NULL && existing_target == new_target) {
      SymbolDelete(alias);
      return true;
    }
    // A using-directive does not hide an existing class or enumerator
    // (`using namespace std` inside namespace absl, which already has
    // `using std::weak_ordering`).  Functions do overload: an anonymous
    // namespace `template <EdgeType> Consume(...)` must join the
    // `void Consume(CordRep*, ...)` already declared in the enclosing
    // namespace, or `Consume<kBack>(...)` is parsed as a comparison.
    if (!current_scope_only) {
      if (!is_tag && CanOverloadFunctions(existing, alias)) {
        if (FindMatchingOverload(existing, alias->type, alias) != NULL) {
          SymbolDelete(alias);
          return true;
        }
        AppendOverload(existing, alias);
        return true;
      }
      SymbolDelete(alias);
      return true;
    }
    if (!is_tag && CanOverloadFunctions(existing, alias)) {
      if (FindMatchingOverload(existing, alias->type, alias) != NULL) {
        SymbolDelete(alias);
        return true;
      }
      AppendOverload(existing, alias);
      return true;
    }
    SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                alias->name.value);
    SymbolDelete(alias);
    return false;
  }
  bool added = is_tag ? SyntaxAddTag(syntax, alias) : SyntaxAddSymbol(syntax, alias);
  if (!added) {
    if (!current_scope_only) {
      SymbolDelete(alias);
      return true;
    }
    SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                alias->name.value);
    SymbolDelete(alias);
    return false;
  }
  return true;
}

// Binary-tree callback: introduces one namespace member symbol into the current
// scope as a using-alias (using-directive semantics).
static void ImportNamespaceSymbol(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  Syntax* syntax = data;
  Symbol* target = ((SymbolNode*)node)->symbol;
  AddUsingAlias(syntax, NewUsingAliasSymbol(target->name.value, target,
                                            syntax->lex->current_token_location),
                /*is_tag=*/false, /*current_scope_only=*/false);
}

// Binary-tree callback: introduces one namespace tag symbol into the current
// scope as a using-alias.
static void ImportNamespaceTag(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  Syntax* syntax = data;
  Symbol* target = ((SymbolNode*)node)->symbol;
  AddUsingAlias(syntax, NewUsingAliasSymbol(target->name.value, target,
                                            syntax->lex->current_token_location),
                /*is_tag=*/true, /*current_scope_only=*/false);
}

// Namespace iterator callback: recursively imports an inline namespace child.
static void ImportInlineNamespaceChild(Namespace* child, void* ctx) {
  ImportNamespace((Syntax*)ctx, child);
}

// Inserts namespace alias `name` -> `target` in the current declarative region.
// Returns kNamespaceAliasConflict when the name clashes with an existing symbol
// or tag; marks the alias exported when inside an export block.
static NamespaceAliasInsertResult InsertNamespaceAliasInCurrentScope(
    Syntax* syntax, String* name, Namespace* target) {
  if (syntax->local_symbol_stack != NULL) {
    if (FindTopLocalSymbol(syntax->local_symbol_stack, name) != NULL ||
        (syntax->local_tag_stack != NULL &&
         FindTopLocalSymbol(syntax->local_tag_stack, name) != NULL)) {
      return kNamespaceAliasConflict;
    }
    return InsertLocalNamespaceAlias(syntax->local_symbol_stack, name, target);
  }

  Namespace* scope = syntax->current_namespace != NULL
      ? syntax->current_namespace
      : compiler->global_namespace;
  if (scope == compiler->global_namespace) {
    if (FindGlobalSymbol(name) != NULL || FindGlobalTag(name) != NULL) {
      return kNamespaceAliasConflict;
    }
  } else if (NamespaceFindSymbol(scope, name) != NULL ||
             NamespaceFindTag(scope, name) != NULL) {
    return kNamespaceAliasConflict;
  }
  NamespaceAliasInsertResult result = NamespaceInsertAlias(scope, name, target);
  if (result == kNamespaceAliasInserted && syntax->export_depth > 0 &&
      scope->namespace_aliases.length > 0) {
    NamespaceAlias* alias = (NamespaceAlias*)VectorGet(
        &scope->namespace_aliases, scope->namespace_aliases.length - 1);
    alias->is_exported = true;
  }
  return result;
}

// Makes every namespace alias and named child namespace of `ns` visible in the
// current scope as if by individual using-declarations.
static void ImportNamespaceNames(Syntax* syntax, Namespace* ns) {
  for (size_t i = 0; i < ns->namespace_aliases.length; i++) {
    NamespaceAlias* alias =
        (NamespaceAlias*)VectorGet(&ns->namespace_aliases, i);
    if (alias != NULL) {
      InsertNamespaceAliasInCurrentScope(syntax, &alias->name, alias->target);
    }
  }
  for (size_t i = 0; i < ns->children.length; i++) {
    Namespace* child = (Namespace*)VectorGet(&ns->children, i);
    if (child != NULL && !child->is_anonymous) {
      InsertNamespaceAliasInCurrentScope(syntax, &child->name, child);
    }
  }
}

// Implements `using namespace ns;`: imports symbols, tags, aliases, anonymous
// and inline namespace members into the current scope.
static void ImportNamespace(Syntax* syntax, Namespace* ns) {
  BinaryTreeTraverse(&ns->symbol_table, ImportNamespaceSymbol, syntax);
  BinaryTreeTraverse(&ns->tag_table, ImportNamespaceTag, syntax);
  ImportNamespaceNames(syntax, ns);
  if (ns->anonymous_child != NULL) {
    ImportNamespace(syntax, ns->anonymous_child);
  }
  NamespaceForEachInlineChild(ns, ImportInlineNamespaceChild, syntax);
}

// True when the next tokens are `identifier` `=` (a namespace alias definition
// rather than a namespace body).  Consumes no tokens permanently.
bool LookingAtNamespaceAliasDefinition(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = LexLookingAt(syntax->lex, TOK(equal));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

// Parses `namespace alias = qualified-ns;`, registers the alias, and returns
// an empty declaration list.  Emits diagnostics and still requires `;` on error.
ASTNode* ParseNamespaceAliasDefinition(Syntax* syntax,
                                       SourceLocation location) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected namespace alias name");
    SyntaxNeedSemicolon(syntax, TC(decl) | TC(stmt));
    return EmptyDeclarationList(location);
  }

  String alias_name;
  StringInit(&alias_name, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  if (!LexMatch(syntax->lex, TOK(equal))) {
    SyntaxError(syntax, "Expected '=' in namespace alias declaration");
    StringDestruct(&alias_name);
    SyntaxNeedSemicolon(syntax, TC(decl) | TC(stmt));
    return EmptyDeclarationList(location);
  }

  FullyQualifiedIdentifier target_name;
  FullyQualifiedIdentifierInit(&target_name);
  if (!SyntaxParseFullyQualifiedIdentifier(syntax, &target_name)) {
    SyntaxError(syntax, "Expected namespace name after '='");
  } else {
    Namespace* target = SyntaxFindQualifiedNamespace(syntax, &target_name);
    if (target == NULL) {
      SyntaxError(syntax, "Unknown namespace %s", target_name.spelling.value);
    } else {
      NamespaceAliasInsertResult result =
          InsertNamespaceAliasInCurrentScope(syntax, &alias_name, target);
      if (result == kNamespaceAliasConflict) {
        SyntaxError(syntax, "Conflicting declaration of namespace alias '%s'",
                    alias_name.value);
      }
    }
  }

  FullyQualifiedIdentifierDestruct(&target_name);
  StringDestruct(&alias_name);
  SyntaxNeedSemicolon(syntax, TC(decl) | TC(stmt));
  return EmptyDeclarationList(location);
}

// Parses `using name = type;` (alias-declaration): builds the typedef symbol,
// attaches template parameters when inside a template, and consumes `;`.
static ASTNode* ParseUsingAliasDeclaration(Syntax* syntax,
                                           FullyQualifiedIdentifier* name,
                                           SourceLocation location,
                                           Vector* attributes) {
  Symbol* defining_template =
      LexLookingAt(syntax->lex, TOK(identifier))
          ? SyntaxFindSymbol(syntax, &syntax->lex->spelling)
          : NULL;
  bool ctad_names_template_template_parameter =
      defining_template != NULL &&
      (defining_template->flags.is_template_template_parameter ||
       (SymbolExtraOf(defining_template)->alias_template != NULL &&
        SymbolExtraOf(defining_template)
            ->alias_template->ctad_names_template_template_parameter));
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  parser.parsing_type_id = true;
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* parsed = TypeParserParseDeclarator(&parser, type);
  if (parsed != NULL) {
    SyntaxApplyDeclarationAttributes(syntax, parsed);
  }
  TypeParserDestruct(&parser);

  TypeRecord* alias_type = parsed != NULL ? parsed->type : type;
  for (TypeRecord* part = alias_type; part != NULL; part = part->next) {
    if (TypeIsFunction(part) &&
        FunctionExtraOf(&part->info.function)->contract_assertions.length !=
            0) {
      SyntaxError(syntax,
                  "function contract specifiers cannot be associated with a "
                  "function type alias");
      break;
    }
  }
  Symbol* alias =
      NewSymbol(FullyQualifiedIdentifierLast(name), alias_type, STO(typedef));
  alias->location = location;
  bool block_scope_alias = syntax->context == kParsingBlockScope;
  if (!block_scope_alias && syntax->parsing_template_declaration) {
    alias->flags.is_template = true;
  }
  LocalSymbolTable* template_parameter_scope = NULL;
  if (!block_scope_alias && syntax->parsing_template_declaration &&
      syntax->local_symbol_stack != NULL) {
    template_parameter_scope = syntax->local_symbol_stack;
    syntax->local_symbol_stack = template_parameter_scope->prev;
  }
  bool added = SyntaxAddSymbol(syntax, alias);
  if (template_parameter_scope != NULL) {
    syntax->local_symbol_stack = template_parameter_scope;
  }
  if (!added) {
    Symbol* existing = SyntaxFindSymbol(syntax, &alias->name);
    if (existing != NULL && existing->flags.is_template &&
        alias->flags.is_template) {
      // Identical alias-template redefinition: Abseil repeats
      // `using SizeType = typename AllocatorTraits<A>::size_type`.
      SymbolDelete(alias);
    } else {
      SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                  alias->name.value);
      SymbolDelete(alias);
    }
  }
  if (added && !block_scope_alias && syntax->parsing_template_declaration) {
    MoveTemplateParameterConstraints(
        syntax->current_template_parameters,
        &SymbolMutableExtra(alias)->associated_constraint);
    if (syntax->current_template_requires_clause != NULL) {
      AddOwnedAssociatedConstraint(
          &SymbolMutableExtra(alias)->associated_constraint,
          syntax->current_template_requires_clause);
      syntax->current_template_requires_clause = NULL;
    }
    if (SymbolExtraOf(alias)->alias_template == NULL) {
      SymbolMutableExtra(alias)->alias_template = malloc(sizeof(AliasTemplate));
      VectorInit(&SymbolExtraOf(alias)->alias_template->parameters);
      SymbolExtraOf(alias)
          ->alias_template->ctad_names_template_template_parameter = false;
    }
    SymbolExtraOf(alias)
        ->alias_template->ctad_names_template_template_parameter =
        ctad_names_template_template_parameter;
    if (syntax->current_template_parameters != NULL) {
      for (size_t p = 0; p < syntax->current_template_parameters->length; p++) {
        VectorAppend(&SymbolExtraOf(alias)->alias_template->parameters,
                     syntax->current_template_parameters->value.p[p]);
      }
      syntax->current_template_parameters->length = 0;
    }
  }
  if (parsed != NULL) {
    SymbolDelete(parsed);
  }
  if (added && attributes != NULL && attributes->length != 0) {
    SymbolAppendAttributes(alias, attributes);
    VectorClear(attributes);
    SyntaxApplyDeclarationAttributes(syntax, alias);
  }
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

// Resolves a using-enum-declarator (a possibly-qualified enum name or a
// typedef naming an enum) to its Enum info, or NULL if it is not an enum.
static Enum* ResolveUsingEnum(Syntax* syntax, FullyQualifiedIdentifier* name) {
  Symbol* tag = SyntaxFindQualifiedTag(syntax, name);
  if (tag != NULL && tag->type != NULL && TypeIsEnum(tag->type)) {
    return tag->type->info.enum_info;
  }
  // A typedef / alias-declaration can name an enumeration.
  Symbol* sym = SyntaxFindQualifiedSymbol(syntax, name);
  if (sym != NULL && sym->type != NULL && TypeIsEnum(sym->type)) {
    return sym->type->info.enum_info;
  }
  return NULL;
}

// C++20 `using enum E;`: introduce every enumerator of E into the current scope
// as if by an individual using-declaration.  For scoped enums this is the only
// way to name the enumerators unqualified; for unscoped enums it re-introduces
// the (already visible) enumerators.  Class-scope `using enum` (making the
// enumerators members) is not yet handled.
static ASTNode* ParseUsingEnumDeclaration(Syntax* syntax,
                                          SourceLocation location) {
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifier(syntax, &name)) {
    SyntaxError(syntax, "Expected enumeration name after 'using enum'");
    FullyQualifiedIdentifierDestruct(&name);
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

  Enum* e = ResolveUsingEnum(syntax, &name);
  if (e == NULL) {
    SyntaxError(syntax, "'%s' is not an enumeration type", name.spelling.value);
  } else {
    for (size_t i = 0; i < e->constants.length; i++) {
      Symbol* constant = e->constants.value.p[i];
      if (constant == NULL) {
        continue;
      }
      AddUsingAlias(syntax,
                    NewUsingAliasSymbol(constant->name.value, constant, location),
                    /*is_tag=*/false, /*current_scope_only=*/true);
    }
  }

  FullyQualifiedIdentifierDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

// Parses a `using` declaration: using-enum, using-directive, alias-declaration,
// or qualified using-declaration.  Consumes through the terminating `;`.
ASTNode* ParseUsingDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // using

  if (LexMatch(syntax->lex, TOK(enum))) {
    return ParseUsingEnumDeclaration(syntax, location);
  }

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

  // alias-declaration: `using Name [[attr]] = type;`
  Vector alias_attributes;
  VectorInit(&alias_attributes);
  if (!name.is_qualified) {
    while (SyntaxParseAnyAttribute(syntax, &alias_attributes)) {
    }
  }

  if (!name.is_qualified && LexMatch(syntax->lex, TOK(equal))) {
    ASTNode* result = ParseUsingAliasDeclaration(syntax, &name, location,
                                                 &alias_attributes);
    AttributeListDestruct(&alias_attributes);
    FullyQualifiedIdentifierDestruct(&name);
    return result;
  }
  AttributeListDestruct(&alias_attributes);

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
      TypoCorrectionErrorUnknownSymbol(syntax, &name);
    } else {
      AddUsingAlias(syntax, NewUsingAliasSymbol(FullyQualifiedIdentifierLast(&name),
                                                target, location),
                    /*is_tag=*/true, /*current_scope_only=*/true);
    }
  } else {
    AddUsingAlias(syntax, NewUsingAliasSymbol(FullyQualifiedIdentifierLast(&name),
                                              target, location),
                  /*is_tag=*/false, /*current_scope_only=*/true);
  }

  FullyQualifiedIdentifierDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

// Consumes a (possibly nested) namespace name after `namespace` and returns the
// Namespace node to open, creating or reopening children as needed.  Returns
// NULL when no identifier follows (caller keeps `parent`).  Emits diagnostics
// for alias extension and inline conflicts.
static Namespace* OpenNamespaceDefinition(Syntax* syntax, Namespace* parent,
                                            bool leading_inline) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return NULL;
  }

  String component;
  StringInit(&component, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);

  if (!LexLookingAt(syntax->lex, TOK(coloncolon))) {
    if (NamespaceFindDirectAlias(parent, &component) != NULL) {
      SyntaxError(syntax, "namespace alias '%s' cannot be extended",
                  component.value);
    }
    bool inline_conflict = false;
    Namespace* ns = NamespaceFindOrReopenChild(parent, &component, leading_inline,
                                               &inline_conflict);
    if (inline_conflict) {
      SyntaxError(syntax, "cannot reopen namespace '%s' as inline",
                  component.value);
    }
    StringDestruct(&component);
    return ns;
  }

  if (leading_inline) {
    SyntaxError(syntax,
                "'inline namespace' cannot specify a nested-namespace-definition");
  }

  Namespace* ns = parent;
  bool inline_conflict = false;
  if (NamespaceFindDirectAlias(ns, &component) != NULL) {
    SyntaxError(syntax, "namespace alias '%s' cannot be extended",
                component.value);
  }
  ns = NamespaceFindOrReopenChild(ns, &component, false, &inline_conflict);
  StringDestruct(&component);
  if (inline_conflict) {
    SyntaxError(syntax, "cannot reopen namespace as inline");
  }

  while (LexMatch(syntax->lex, TOK(coloncolon))) {
    bool component_inline = false;
    if (CompilerIsCXX() && CompilerCXXAtLeast(kLanguageStandardCXX20) &&
        LexMatch(syntax->lex, TOK(inline))) {
      component_inline = true;
    } else if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(inline))) {
      SyntaxError(syntax,
                  "'inline' in a nested namespace definition requires C++20");
      LexNextToken(syntax->lex);
    }

    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected namespace name after '::'");
      return ns;
    }

    StringInit(&component, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);

    inline_conflict = false;
    if (NamespaceFindDirectAlias(ns, &component) != NULL) {
      SyntaxError(syntax, "namespace alias '%s' cannot be extended",
                  component.value);
    }
    ns = NamespaceFindOrReopenChild(ns, &component, component_inline,
                                    &inline_conflict);
    if (inline_conflict) {
      SyntaxError(syntax, "cannot reopen namespace '%s' as inline",
                  component.value);
    }
    StringDestruct(&component);

    if (!LexLookingAt(syntax->lex, TOK(coloncolon))) {
      return ns;
    }
  }

  return ns;
}

// Parses `namespace [inline] name { ... }` or a namespace alias, sets
// `syntax->current_namespace` while parsing member declarations, and returns
// their declaration list.
ASTNode* ParseNamespaceDeclaration(Syntax* syntax, bool leading_inline) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // namespace

  if (!CompilerIsCXX()) {
    leading_inline = false;
  }

  if (CompilerIsCXX() && LookingAtNamespaceAliasDefinition(syntax)) {
    if (leading_inline) {
      SyntaxError(syntax, "namespace alias declaration cannot be inline");
    }
    return ParseNamespaceAliasDefinition(syntax, location);
  }

  Namespace* previous_namespace = syntax->current_namespace;
  Namespace* parent = previous_namespace != NULL ? previous_namespace
                                                 : compiler->global_namespace;
  Namespace* ns = NULL;
  if (LexLookingAt(syntax->lex, TOK(lbrace))) {
    bool inline_conflict = false;
    ns = NamespaceFindOrReopenAnonymousChild(parent, leading_inline,
                                             &inline_conflict);
    if (inline_conflict) {
      SyntaxError(syntax,
                  "cannot reopen anonymous namespace as inline");
    }
  } else {
    ns = OpenNamespaceDefinition(syntax, parent, leading_inline);
    if (ns == NULL) {
      ns = parent;
    }
  }

  SyntaxNeedBracket(syntax, TOK(lbrace), TC(openbra) | TC(decl));

  Vector* declarations = NewVector();
  syntax->current_namespace = ns;
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rbrace))) {
    ASTNode* node = SyntaxParseExternalDeclaration(syntax);
    if (node != NULL && node->op == AST_OP(consteval_block)) {
      SemanticAnalyzeConstevalBlock((ConstevalBlockASTNode*)node);
      ASTNode* injected = NULL;
      while ((injected = CompilerPopPendingInjectedDeclaration()) != NULL) {
        bool was_list = injected->op == AST_OP(decl_list);
        AppendDeclarationsFromNode(declarations, injected);
        if (was_list) {
          ASTNodeDelete(injected);
        }
      }
      ASTNodeDelete(node);
      continue;
    }
    AppendDeclarationsFromNode(declarations, node);
  }
  syntax->current_namespace = previous_namespace;

  SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
  return NewDeclarationListASTNode(declarations, location);
}
