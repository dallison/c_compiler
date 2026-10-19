//
//  syntax_friend.c
//  c_compiler
//
//  Friend declarations and deferred friend function bodies.
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

// Records a parsed friend function symbol, queuing it for code generation when
// it carries an inline body.  `definition` is the AST returned by
// DeclareOrDefineFunction (non-NULL for an inline definition), `sym` the parsed
// declarator symbol and `befriending` the class granting friendship.
static void RecordFriendFunction(Syntax* syntax, Struct* befriending,
                                 Symbol* sym, Symbol* in_scope_symbol,
                                 ASTNode* definition) {
  (void)syntax;
  // Match on the symbol that actually persists in the enclosing scope so the
  // friend record stays valid; access checks compare by name and signature so
  // either the merged declaration or the fresh symbol works.
  Symbol* friend_symbol = in_scope_symbol != NULL ? in_scope_symbol : sym;
  // Preserve the class whose lexical scope and access rights apply while an
  // inline hidden-friend body is analyzed later.  The function remains a
  // namespace-scope function, so cxx_member_owner cannot carry this context.
  if (sym != NULL) {
    sym->member_lookup_class = befriending;
  }
  if (friend_symbol != NULL) {
    friend_symbol->member_lookup_class = befriending;
  }
  StructAddFriendFunction(befriending, friend_symbol);
  if (definition != NULL) {
    // Inline friend definitions belong to the enclosing namespace; queue them
    // alongside the other deferred definitions so codegen emits the body.
    CompilerQueuePendingTemplateInstantiation(definition);
  }
}

// When `type` names a class whose tag matches a lexically enclosing class of
// `befriending` (ignoring template arguments), returns that enclosing class;
// otherwise NULL.
static Struct* ResolveFriendClassFromEnclosingClasses(Struct* befriending,
                                                      TypeRecord* type) {
  if (befriending == NULL || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  const char* friend_name = type->info.struct_info->tag_name->value;
  size_t friend_len = strcspn(friend_name, "<");
  for (Struct* parent = befriending->lexical_parent; parent != NULL;
       parent = parent->lexical_parent) {
    if (parent->tag_name == NULL) {
      continue;
    }
    size_t parent_len = strcspn(parent->tag_name->value, "<");
    if (parent_len == friend_len &&
        strncmp(parent->tag_name->value, friend_name, parent_len) == 0) {
      return parent;
    }
  }
  return NULL;
}

// Friend declared inside a class template: defer it.  We keep the parsed
// symbol (with its inline body, if any) on the template so each specialization
// can substitute the signature/body and register a concrete friend; we do NOT
// inject the dependent declaration into the enclosing namespace (which would
// otherwise create a spurious overload) nor emit any body now.
typedef struct {
  Symbol* symbol;
  String body_text;
  int body_lineno;
  uint32_t body_file_index;
  size_t body_path_index;
  bool body_is_system_header;
} DeferredFriendBody;

// Introduces symbols for each template parameter on function type `func` into
// the current scope while a deferred friend body is parsed.
static void AddDeferredFriendTemplateParameters(Syntax* syntax,
                                                TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return;
  }
  const Vector* parameters =
      &FunctionExtraOf(&func->info.function)->template_parameters;
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* parameter = parameters->value.p[i];
    if (parameter == NULL || parameter->name.length == 0) {
      continue;
    }
    TypeRecord* parameter_type = NULL;
    if (parameter->kind == kTemplateParameterType) {
      parameter_type =
          NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      parameter_type->template_parameter_index = parameter->index;
      TypeMutableDependent(parameter_type)->template_parameter_name =
          NewString(parameter->name.value);
    } else if (parameter->type != NULL) {
      parameter_type = TypeRecordCopy(parameter->type);
    } else {
      continue;
    }
    Storage storage = parameter->kind == kTemplateParameterType ? STO(typedef)
                                                                : STO(implicit);
    Symbol* symbol = NewSymbol(parameter->name.value, parameter_type, storage);
    symbol->flags.invented = true;
    symbol->flags.is_template_parameter = true;
    symbol->flags.is_template_type_parameter =
        parameter->kind == kTemplateParameterType;
    symbol->flags.is_parameter_pack = parameter->is_parameter_pack;
    symbol->template_parameter_index = parameter->index;
    SyntaxAddSymbol(syntax, symbol);
  }
}

// Consumes a braced function body from `lex`, including nested braces, when
// `{` is present; otherwise leaves the lexer unchanged.
static void SkipFriendFunctionBody(Lex* lex) {
  if (!LexMatch(lex, TOK(lbrace))) {
    return;
  }
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

// An inline friend defined inside a class is in complete-class scope
// ([class.mem]).  Capture the body and parse it after every member of this
// class is declared, so a later nested type (`FindElement`) is visible.
static void DeferFriendFunctionBody(Syntax* syntax, Symbol* sym) {
  Lex* lex = syntax->lex;
  DeferredFriendBody* deferred = malloc(sizeof(DeferredFriendBody));
  deferred->symbol = sym;
  deferred->body_lineno = lex->source->lineno;
  deferred->body_file_index = lex->source->file_index;
  deferred->body_path_index = lex->source->path_index;
  deferred->body_is_system_header = lex->source->is_system_header;
  StringInit(&deferred->body_text, NULL);
  LexBeginCapture(lex, &deferred->body_text);
  SkipFriendFunctionBody(lex);
  LexEndCapture(lex);
  VectorAppend(syntax->deferred_friend_bodies, deferred);
}

// True when `type` names `str` (the injected-class-name of the class being
// defined, or a reference/parameter of that type).
static bool TypeMentionsStruct(TypeRecord* type, Struct* str) {
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (str != NULL && TypeIsStructOrUnion(current) &&
        current->info.struct_info == str) {
      return true;
    }
    if (!TypeIsFunction(current)) {
      continue;
    }
    Vector* prototype = &current->info.function.prototype;
    for (size_t i = 0; i < prototype->length; i++) {
      Symbol* param = prototype->value.p[i];
      if (param != NULL && TypeMentionsStruct(param->type, str)) {
        return true;
      }
    }
  }
  return false;
}

// True when this friend must stay a pattern and be cloned per class
// instantiation, rather than compiled as one concrete function.
static bool FriendBodyIsClassTemplatePattern(Symbol* symbol,
                                             Struct* class_head) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->flags.is_template ||
      TypeContainsTemplateParameter(symbol->type)) {
    return true;
  }
  // is_template is set only after the class body is parsed.  During the
  // flush, defining_template_scope_count is the signal that this class is a
  // template, and an injected-class-name parameter (`civil_time a`) names
  // that pattern even though it carries no template-parameter index.
  if (class_head != NULL && class_head->defining_template_scope_count > 0 &&
      TypeMentionsStruct(symbol->type, class_head)) {
    return true;
  }
  if (symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return false;
  }
  Struct* owner = symbol->type->info.function.cxx_member_owner;
  return owner != NULL && owner->is_template;
}

// Re-parses each captured inline friend body after the befriending class is
// complete, defines the function with class access context, and queues codegen
// or retains the AST for template patterns.
void SyntaxFlushDeferredFriendBodies(Syntax* syntax) {
  Vector* deferred = syntax->deferred_friend_bodies;
  if (deferred == NULL || deferred->length == 0) {
    return;
  }
  Lex* lex = syntax->lex;
  LexCheckpoint end_checkpoint;
  LexCheckpointSave(lex, &end_checkpoint);
  for (size_t i = 0; i < deferred->length; i++) {
    DeferredFriendBody* entry = deferred->value.p[i];
    String* text = NewString(NULL);
    StringSetString(text, &entry->body_text);
    Source* replay = NewSourceFromString("<deferred-friend-body>", text);
    replay->file_index = entry->body_file_index;
    replay->lineno = entry->body_lineno - 1;
    replay->path_index = entry->body_path_index;
    replay->is_system_header = entry->body_is_system_header;
    lex->source = replay;
    lex->suppress_preprocessing = true;
    LexWillChangeLine(lex);
    StringClear(&lex->line);
    lex->pos = 0;
    LexNextToken(lex);
    SyntaxOpenScope(syntax);
    AddDeferredFriendTemplateParameters(syntax, entry->symbol->type);
    // The friend is a namespace function, but the body is in the scope of the
    // class that defined it, including private members declared after the
    // friend.
    Struct* saved_access = compiler->current_class_access_context;
    Struct* owner = syntax->cxx_class_head;
    if (entry->symbol->type != NULL && TypeIsFunction(entry->symbol->type) &&
        entry->symbol->type->info.function.cxx_member_owner != NULL) {
      owner = entry->symbol->type->info.function.cxx_member_owner;
    }
    compiler->current_class_access_context = owner;
    Vector* friend_decls = NewVector();
    ASTNode* definition =
        DeclareOrDefineFunction(syntax, friend_decls, entry->symbol, NULL);
    compiler->current_class_access_context = saved_access;
    SyntaxCloseScope(syntax);
    bool defer_friend =
        FriendBodyIsClassTemplatePattern(entry->symbol, syntax->cxx_class_head);
    if (definition == NULL) {
      VectorDelete(friend_decls);
    } else if (defer_friend) {
      // A friend defined in a class template is emitted with each
      // instantiation.  The pattern body is not a function of its own: its
      // injected-class-name parameters name the unspecialized template, and
      // compiling that body once the specializations exist treats every
      // specialization as an exact match (`civil_time + n`).
      VectorAppend(&compiler->declaration_asts, definition);
    } else {
      CompilerQueuePendingTemplateInstantiation(definition);
    }
    lex->suppress_preprocessing = false;
    lex->source = NULL;
    SourceDelete(replay);
    StringDestruct(&entry->body_text);
    free(entry);
  }
  LexCheckpointRestore(lex, &end_checkpoint);
  LexCheckpointDestruct(&end_checkpoint);
  VectorClear(deferred);
}

// Records a friend function declared inside a class template on `befriending`
// without injecting it into the enclosing namespace; parses or defers its body
// and handles `= default` / `= delete`.
static void SyntaxDeferTemplateFriendFunction(Syntax* syntax,
                                              Struct* befriending, Symbol* sym) {
  sym->namespace_ = syntax->current_namespace;
  ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type, true);
  if (LexLookingAt(syntax->lex, TOK(lbrace))) {
    if (syntax->deferred_friend_bodies != NULL) {
      DeferFriendFunctionBody(syntax, sym);
    } else {
      // Parse and retain the inline body on sym->type; intentionally do not
      // queue the returned definition for emission - that happens per
      // instantiation.
      Vector* friend_decls = NewVector();
      ASTNode* definition =
          DeclareOrDefineFunction(syntax, friend_decls, sym, NULL);
      if (definition == NULL) {
        VectorDelete(friend_decls);
      } else {
        // Retain as a teardown root so the template body outlives parsing.
        VectorAppend(&compiler->declaration_asts, definition);
      }
    }
  } else {
    SyntaxNeedSemicolon(syntax, TC(decl));
  }
  StructAddFriendFunction(befriending, sym);
}

// Registers a fully-substituted instantiated friend function `sym` in namespace
// `ns` (merging with existing overloads), returning the symbol that persists in
// scope (an existing matching declaration/definition if present, otherwise
// `sym`).  Used when instantiating a class template's friends.
Symbol* SyntaxRegisterInstantiatedFriendFunction(Syntax* syntax, Namespace* ns,
                                                 Symbol* sym) {
  Namespace* saved = syntax->current_namespace;
  syntax->current_namespace =
      ns != NULL ? ns : (compiler != NULL ? compiler->global_namespace : NULL);
  Symbol* old_sym = FindFileScopeSymbol(syntax, &sym->name);
  Symbol* in_scope = NULL;
  if (CanOverloadFunctions(old_sym, sym)) {
    Symbol* matching_overload = FindMatchingOverload(old_sym, sym->type, sym);
    if (matching_overload != NULL) {
      in_scope = matching_overload;
    } else {
      AppendOverload(old_sym, sym);
      in_scope = sym;
    }
  } else if (old_sym != NULL &&
             !RedeclarationTypesEqual(old_sym->type, sym->type)) {
    in_scope = NULL;
  }
  if (in_scope == NULL) {
    if (InsertFileScopeSymbol(syntax, sym)) {
      in_scope = sym;
    } else {
      in_scope = sym;
    }
  }
  syntax->current_namespace = saved;
  return in_scope;
}

// True when `type` or its template arguments mention a template parameter with
// index at or after `parameter_base` (the befriending class template's own
// parameters).
static bool FriendTypeReferencesTemplateParameterAtOrAfter(
    TypeRecord* type, int parameter_base) {
  for (TypeRecord* current = type; current != NULL;
       current = current->next) {
    if (current->template_parameter_index >= parameter_base) {
      return true;
    }
    for (size_t i = 0;
         current->template_arguments != NULL &&
         i < current->template_arguments->length; i++) {
      TemplateArgument* argument =
          current->template_arguments->value.p[i];
      if (argument == NULL) {
        continue;
      }
      if (argument->template_parameter_index >= parameter_base ||
          FriendTypeReferencesTemplateParameterAtOrAfter(
              argument->type, parameter_base)) {
        return true;
      }
      for (size_t j = 0;
           argument->pack_arguments != NULL &&
           j < argument->pack_arguments->length; j++) {
        TemplateArgument* element = argument->pack_arguments->value.p[j];
        if (element != NULL &&
            (element->template_parameter_index >= parameter_base ||
             FriendTypeReferencesTemplateParameterAtOrAfter(
                 element->type, parameter_base))) {
          return true;
        }
      }
    }
  }
  return false;
}

// Registers friendship for `type` on `befriending`: stores dependent friend-type
// patterns on the class, befriends a resolved class (including enclosing-name
// resolution), or ignores non-class types per the standard.
static void RecordFriendTypeSpecifier(Syntax* syntax, Struct* befriending,
                                      TypeRecord* type,
                                      bool is_pack_expansion,
                                      SourceLocation location) {
  if (syntax->parsing_template_declaration &&
      FriendTypeReferencesTemplateParameterAtOrAfter(
          type, befriending->defining_template_scope_count)) {
    SyntaxError(
        syntax,
        "friend type declaration cannot use its own template parameters");
    return;
  }
  bool dependent =
      type != NULL &&
      (TypeContainsTemplateParameter(type) || TypeIsUnknown(type));
  if (is_pack_expansion && !CXXTypeContainsParameterPack(syntax, type)) {
    SyntaxError(syntax,
                "friend type pack expansion requires a template parameter pack");
    return;
  }
  if (dependent || is_pack_expansion) {
    VectorAppend(&befriending->friend_type_declarations,
                 NewCXXFriendTypeDeclaration(type, is_pack_expansion,
                                             location));
    return;
  }
  if (type != NULL && TypeIsStructOrUnion(type) &&
      type->info.struct_info != NULL) {
    Struct* friend_class =
        ResolveFriendClassFromEnclosingClasses(befriending, type);
    StructAddFriendClass(
        befriending,
        friend_class != NULL ? friend_class : type->info.struct_info);
  }
  // [class.friend]: a friend-type-specifier that does not designate a class
  // type is ignored.
}

// Whether the qualified name at the current token has a nested-name-specifier
// that can be dependent: it does not start with a namespace, or one of its
// qualifying components is a template-id (`Outer<T>::Inner`).  A name such as
// `ns::Template<T>` only uses the template parameter as an argument and does
// not need `typename`.
static bool FriendQualifierMayBeDependent(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (FindNamespaceChildInScope(syntax, &lex->spelling) == NULL) {
    return true;
  }
  int depth = 0;
  for (size_t pos = lex->pos; pos < lex->line.length; pos++) {
    char c = lex->line.value[pos];
    if (c == '<') {
      depth++;
    } else if (c == '>' && depth > 0) {
      depth--;
      if (depth == 0) {
        size_t next = pos + 1;
        while (next < lex->line.length &&
               isspace((unsigned char)lex->line.value[next])) {
          next++;
        }
        if (next + 1 < lex->line.length && lex->line.value[next] == ':' &&
            lex->line.value[next + 1] == ':') {
          return true;
        }
      }
    } else if (depth == 0 && (c == ';' || c == ',')) {
      break;
    }
  }
  return false;
}

// The declaration specifiers of one friend declaration (or of one entry of a
// C++26 comma-separated friend-type list).
typedef struct FriendSpecifiers {
  Vector attributes;
  Storage storage;
  bool is_inline;
  bool is_constexpr;
  bool is_consteval;
  bool is_constinit;
  bool is_explicit;
  TypeRecord* type;  // Owned reference; NULL when no type was parsed.
} FriendSpecifiers;

// Parses the declaration specifiers following `friend` into `spec`, which
// must not hold a type or attributes.  The specifiers are parsed in the
// friend contexts (C++26 friend-type-specifier, friend declaration) and the
// resulting type is referenced by `spec`.
static void ParseFriendSpecifiers(Syntax* syntax, FriendSpecifiers* spec) {
  VectorInit(&spec->attributes);
  spec->storage = STO(implicit);
  spec->is_inline = false;
  spec->is_constexpr = false;
  spec->is_consteval = false;
  spec->is_constinit = false;
  spec->is_explicit = false;
  spec->type = NULL;
  bool saved_friend_type_context = syntax->parsing_friend_type_specifier;
  bool saved_friend_declaration_context =
      syntax->parsing_friend_declaration_specifier;
  syntax->parsing_friend_type_specifier =
      CompilerCXXAtLeast(kLanguageStandardCXX26);
  syntax->parsing_friend_declaration_specifier = true;
  ParseDeclarationSpecifier(syntax, &spec->storage, &spec->is_inline,
                            &spec->is_constexpr, &spec->is_consteval,
                            &spec->is_constinit, &spec->is_explicit,
                            &spec->type, &spec->attributes, kParsingFileScope);
  syntax->parsing_friend_type_specifier = saved_friend_type_context;
  syntax->parsing_friend_declaration_specifier =
      saved_friend_declaration_context;
  TypeRecordIncRef(spec->type);
}

// Parses the rest of a friend-type-declaration whose first specifier is in
// `spec`, recording each befriended type on `befriending`:
//   friend T;
//   friend T, U;
//   friend Ts...;
// The first form predates C++26; comma-separated and expanded specifiers do
// not.  Dependent type patterns are retained on the class template and
// resolved for each specialization.  Releases what `spec` holds.
static void ParseFriendTypeDeclaration(Syntax* syntax, Struct* befriending,
                                       FriendSpecifiers* spec,
                                       bool qualifier_may_be_dependent) {
  if (qualifier_may_be_dependent &&
      !CompilerCXXAtLeast(kLanguageStandardCXX26) && spec->type != NULL &&
      TypeContainsTemplateParameter(spec->type)) {
    SyntaxError(syntax, "dependent friend type requires 'typename' before C++26");
  }
  bool another = true;
  while (another) {
    SourceLocation location = syntax->lex->current_token_location;
    bool is_pack_expansion = LexMatch(syntax->lex, TOK(ellipsis));
    bool has_comma = LexLookingAt(syntax->lex, TOK(comma));
    if ((is_pack_expansion || has_comma) &&
        !CompilerCXXAtLeast(kLanguageStandardCXX26)) {
      SyntaxError(
          syntax,
          "variadic and comma-separated friend types require C++26");
    }
    RecordFriendTypeSpecifier(syntax, befriending, spec->type,
                              is_pack_expansion, location);
    TypeRecordDelete(spec->type);
    spec->type = NULL;
    AttributeListDestruct(&spec->attributes);

    another = LexMatch(syntax->lex, TOK(comma));
    if (!another) {
      break;
    }
    ParseFriendSpecifiers(syntax, spec);
    if (spec->type == NULL) {
      SyntaxError(syntax, "expected a friend type specifier");
      AttributeListDestruct(&spec->attributes);
      break;
    }
  }
  SyntaxNeedSemicolon(syntax, TC(decl));
  if (spec->type != NULL) {
    TypeRecordDelete(spec->type);
    AttributeListDestruct(&spec->attributes);
  }
}

// Handles a friend declarator that names an already declared function rather
// than introducing one: a qualified namespace-scope function
// (`friend void ns::f();`) or a member of another class
// (`friend void A::f();`).  Records the friendship and consumes the `;`.
// Returns false when the declarator names neither.
static bool BefriendExistingFunction(Syntax* syntax, TypeParser* parser,
                                     Struct* befriending, Symbol* sym) {
  // A qualified friend declaration can name a namespace-scope function that
  // was declared before the befriending class. It does not redeclare that
  // function in the class's enclosing namespace.
  if (parser->cxx_qualified_friend_function != NULL) {
    Symbol* friend_function =
        FindMatchingOverload(parser->cxx_qualified_friend_function, sym->type,
                             /*incoming=*/NULL);
    if (friend_function == NULL) {
      // A qualified friend names an existing function; it does not declare
      // a new one.  Dependent friend templates such as
      // `template <class H> friend H ns::f(H, T)` often fail a strict
      // signature match against the existing template, but lookup already
      // identified the intended function (or function template).
      friend_function = parser->cxx_qualified_friend_function;
    }
    if (friend_function == NULL) {
      SyntaxError(syntax,
                  "Qualified friend declaration does not match an existing "
                  "function");
    } else {
      StructAddFriendFunction(befriending, friend_function);
    }
    SyntaxNeedSemicolon(syntax, TC(decl));
    return true;
  }

  // 'friend void A::f();' names an existing member function of another class.
  // It refers to that member rather than introducing a namespace-scope function,
  // so record the friendship against the existing member and do not redeclare
  // or define it here.
  if (parser->cxx_member_definition != NULL &&
      parser->cxx_member_definition->symbol != NULL) {
    StructAddFriendFunction(befriending, parser->cxx_member_definition->symbol);
    SyntaxNeedSemicolon(syntax, TC(decl));
    return true;
  }
  return false;
}

// Returns true when `befriending` or a class lexically enclosing it is a
// class template (or a member of one being defined).
static bool FriendEnclosingClassIsTemplate(Struct* befriending) {
  for (Struct* c = befriending; c != NULL; c = c->lexical_parent) {
    if (c->is_template || c->template_parameter_count > 0 ||
        c->defining_template_scope_count > 0) {
      return true;
    }
  }
  return false;
}

// Returns true when the friend function `sym` has a template head of its own
// (a friend function template) rather than only seeing the template
// parameters of the enclosing class template.  A friend function template's
// parameter indices start after the enclosing class template's parameters.
static bool FriendHasOwnTemplateHead(Syntax* syntax, Struct* befriending,
                                     Symbol* sym) {
  if (!syntax->parsing_template_declaration || !TypeIsFunction(sym->type) ||
      syntax->current_template_parameters == NULL) {
    return false;
  }
  Vector* params = syntax->current_template_parameters;
  for (size_t i = 0; i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param != NULL &&
        param->index >= befriending->defining_template_scope_count) {
      return true;
    }
  }
  return false;
}

// Makes the friend `sym` a function template with its own template-parameter
// list, so overload resolution and ADL treat it accordingly.  The parameters
// currently live in `current_template_parameters`; copy only the friend's own
// parameters (indices at or after the enclosing class template).  Including
// the class parameters makes this a different template from the later
// namespace definition, so the specialization is not recognized as a friend.
// Their indices stay enclosing-relative; instantiation rebases them.
static void AdoptFriendTemplateHead(Syntax* syntax, Struct* befriending,
                                    Symbol* sym) {
  Vector* params = syntax->current_template_parameters;
  int parameter_base = befriending->defining_template_scope_count;
  int own_count = 0;
  TemplateParameter* first_own = NULL;
  for (size_t i = 0; i < params->length; i++) {
    TemplateParameter* param = params->value.p[i];
    if (param == NULL || param->index < parameter_base) {
      continue;
    }
    if (first_own == NULL) {
      first_own = param;
    }
    VectorAppend(
        &FunctionMutableExtra(&sym->type->info.function)->template_parameters,
        TemplateParameterCopy(param));
    own_count++;
  }
  sym->flags.is_template = true;
  FunctionMutableExtra(&sym->type->info.function)->template_parameter_count =
      own_count;
  FunctionMutableExtra(&sym->type->info.function)->template_parameter_base =
      first_own != NULL ? first_own->index : parameter_base;
  // Fold any concept-constrained parameters (e.g. `template <integral I>`) and
  // an explicit trailing requires-clause into the function's constraint.
  MoveTemplateParameterConstraints(
      &FunctionMutableExtra(&sym->type->info.function)->template_parameters,
      &FunctionMutableExtra(&sym->type->info.function)->associated_constraint);
  if (syntax->current_template_requires_clause != NULL) {
    AddFunctionAssociatedConstraint(sym->type,
                                    syntax->current_template_requires_clause);
    syntax->current_template_requires_clause = NULL;
  }
}

// Declares the friend function `sym` in the enclosing namespace scope,
// merging with an existing declaration or overload set exactly as a
// namespace-scope declaration would, and gives it its mangled name.  Sets
// `*old_sym` to the previous declaration it redeclares (or NULL) and returns
// the symbol now visible in the namespace scope for it (or NULL).
static Symbol* DeclareFriendAtNamespaceScope(Syntax* syntax, Symbol* sym,
                                             Symbol** old_sym_out) {
  Symbol* old_sym = FindFileScopeSymbol(syntax, &sym->name);
  bool overload_was_appended = false;
  if (CanOverloadFunctions(old_sym, sym)) {
    Symbol* matching_overload = FindMatchingOverload(old_sym, sym->type, sym);
    if (matching_overload != NULL) {
      old_sym = matching_overload;
    } else {
      AppendOverload(old_sym, sym);
      old_sym = NULL;
      overload_was_appended = true;
    }
  } else if (old_sym != NULL &&
             !RedeclarationTypesEqual(old_sym->type, sym->type)) {
    // A non-overloadable clash with an existing non-function symbol.
    old_sym = NULL;
  }

  Symbol* in_scope_symbol = old_sym;
  if (old_sym == NULL && !overload_was_appended) {
    if (InsertFileScopeSymbol(syntax, sym)) {
      in_scope_symbol = sym;
    }
  } else if (overload_was_appended) {
    in_scope_symbol = sym;
  }

  SymbolSetCXXMangledAsmName(sym);
  if (old_sym != NULL && old_sym->asm_name.length == 0) {
    SymbolSetCXXMangledAsmName(old_sym);
  }
  *old_sym_out = old_sym;
  return in_scope_symbol;
}

// Defers the inline body of friend function `sym` until the befriending
// class is complete: `friend operator<<` may name private members that are
// declared after the friend (`XMLElement::txt_`).
static void DeferInlineFriendDefinition(Syntax* syntax, Struct* befriending,
                                        Symbol* sym, Symbol* old_sym,
                                        Symbol* in_scope_symbol) {
  Symbol* body_sym = sym;
  if (body_sym->type->info.function.cxx_member_owner == NULL) {
    body_sym->type->info.function.cxx_member_owner = befriending;
  }
  if (old_sym != NULL && old_sym != sym && TypeIsFunction(old_sym->type)) {
    old_sym->value.func_defn = sym;
  }
  DeferFriendFunctionBody(syntax, body_sym);
  RecordFriendFunction(syntax, befriending, sym, in_scope_symbol, NULL);
}

// Declares (or defines, parsing its body now) the namespace-scope friend
// function `sym`, with access checked as from inside `befriending`, and
// records the friendship.  A hidden friend's body and a defaulted comparison
// are processed with `befriending` as the owning class.
static void DefineFriendFunction(Syntax* syntax, Struct* befriending,
                                 Symbol* sym, Symbol* old_sym,
                                 Symbol* in_scope_symbol,
                                 bool has_inline_friend_body) {
  Vector* friend_decls = NewVector();
  Struct* saved_access_context = compiler->current_class_access_context;
  Struct* saved_comparison_owner =
      sym->type->info.function.cxx_member_owner;
  bool defaulted_comparison =
      sym->type->info.function.is_defaulted &&
      (StringEqual(&sym->name, "operator==") ||
       StringEqual(&sym->name, "operator<=>"));
  if (saved_comparison_owner == NULL &&
      (has_inline_friend_body || defaulted_comparison)) {
    // A hidden friend is a namespace-scope function, but its body is parsed in
    // the scope of the befriending class. Supply that owner while parsing an
    // inline body (or synthesizing a defaulted comparison), then restore it to
    // preserve free-function lookup and mangling.
    sym->type->info.function.cxx_member_owner = befriending;
  }
  compiler->current_class_access_context = befriending;
  ASTNode* definition =
      DeclareOrDefineFunction(syntax, friend_decls, sym, old_sym);
  compiler->current_class_access_context = saved_access_context;
  sym->type->info.function.cxx_member_owner = saved_comparison_owner;
  RecordFriendFunction(syntax, befriending, sym, in_scope_symbol, definition);
  if (definition == NULL) {
    // Declaration only: DeclareOrDefineFunction did not adopt the vector.
    VectorDelete(friend_decls);
    SyntaxNeedSemicolon(syntax, TC(decl));
  }
}

// Processes the parsed friend function declarator `sym`: befriends an
// existing function it names, or declares it in the enclosing namespace
// scope.  A friend of a class template is deferred and materialized per
// specialization; a friend function template of an ordinary class is
// registered now.  An inline body is deferred until the class is complete.
static void DeclareFriendFunction(Syntax* syntax, TypeParser* parser,
                                  Struct* befriending, Symbol* sym) {
  sym->type->info.function.is_explicit = false;
  // A friend function defined in a non-local class definition is implicitly
  // inline even when the `inline` specifier is omitted ([dcl.fct.spec]).
  bool has_inline_friend_body = LexLookingAt(syntax->lex, TOK(lbrace));
  if (has_inline_friend_body) {
    sym->type->info.function.is_inline = true;
  }

  if (BefriendExistingFunction(syntax, parser, befriending, sym)) {
    return;
  }

  // Distinguish two "template friend" situations:
  //   (a) a friend declared inside a class *template* — its signature depends on
  //       the class's template parameters, so it is dependent and must be
  //       materialized per specialization (deferred below); versus
  //   (b) a friend function *template* declared inside a non-template class,
  //       e.g. `struct S { template<class I> friend bool operator==(S,const I&); };`
  //       — this is an ordinary namespace-scope function template, discoverable
  //       via ADL, and must be registered now (not deferred, since the class is
  //       never "instantiated").
  bool enclosing_class_is_template = FriendEnclosingClassIsTemplate(befriending);
  if (FriendHasOwnTemplateHead(syntax, befriending, sym) &&
      FunctionExtraOf(&sym->type->info.function)->template_parameters.length ==
          0) {
    AdoptFriendTemplateHead(syntax, befriending, sym);
  }

  if (syntax->parsing_template_declaration && enclosing_class_is_template) {
    SyntaxDeferTemplateFriendFunction(syntax, befriending, sym);
    return;
  }

  Symbol* old_sym = NULL;
  Symbol* in_scope_symbol = DeclareFriendAtNamespaceScope(syntax, sym, &old_sym);
  ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type,
                                         old_sym == NULL);
  // A non-template class still has to defer an inline friend body until the
  // class is complete.
  if (has_inline_friend_body && syntax->deferred_friend_bodies != NULL) {
    DeferInlineFriendDefinition(syntax, befriending, sym, old_sym,
                                in_scope_symbol);
    return;
  }
  DefineFriendFunction(syntax, befriending, sym, old_sym, in_scope_symbol,
                       has_inline_friend_body);
}

// Parses the declarator of a friend function declaration or inline
// definition whose specifiers are in `spec`.  It belongs to the nearest
// enclosing namespace scope, not to the class, so it is declared there and
// merged with any existing overloads exactly as a normal namespace-scope
// function declaration would.  Releases what `spec` holds.
static void ParseFriendFunctionDeclaration(Syntax* syntax, Struct* befriending,
                                           FriendSpecifiers* spec,
                                           bool used_friend_type_only_context) {
  if (used_friend_type_only_context &&
      TypeContainsTemplateParameter(spec->type)) {
    SyntaxError(syntax,
                "dependent friend function return type requires 'typename'");
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, spec->storage,
                 kParsingFileScope);
  parser.is_inline = spec->is_inline;
  parser.is_constexpr = spec->is_constexpr;
  parser.is_consteval = spec->is_consteval;
  parser.is_constinit = spec->is_constinit;
  parser.parsing_friend_declaration = true;
  // Let the friend's signature name the class currently being defined via its
  // own template-id (e.g. 'friend f(const Box<T>&)' inside 'template Box').
  // This only affects type resolution; the friend is still declared at the
  // enclosing namespace scope, not as a member of the class.
  parser.cxx_member_owner = befriending;
  SyntaxOpenScope(syntax);

  Symbol* sym = TypeParserParseDeclarator(&parser, spec->type);
  if (sym == NULL || !TypeIsFunction(sym->type)) {
    if (sym == NULL) {
      SyntaxError(syntax, "expected a friend function declaration");
    } else {
      SyntaxError(syntax, "a friend declaration must name a function or class");
    }
    SyntaxRecover(syntax, TC(semicolon));
    LexMatch(syntax->lex, TOK(semicolon));
  } else {
    DeclareFriendFunction(syntax, &parser, befriending, sym);
  }

  SyntaxCloseScope(syntax);
  TypeParserDestruct(&parser);
  TypeRecordDelete(spec->type);
  AttributeListDestruct(&spec->attributes);
}

// Parses a friend declaration in the member list of class `befriending`,
// starting at the `friend` keyword: either a friend-type-declaration
// (`friend class X;`, C++26 `friend Ts...;`) or a friend function
// declaration or inline definition.
void SyntaxParseFriendDeclaration(Syntax* syntax, Struct* befriending) {
  LexMatch(syntax->lex, TOK(friend));

  // A friend declaration introduces its name (a forward-declared friend class
  // tag, or the friend function) into the nearest enclosing namespace scope,
  // not into the class being defined.  Suspend the enclosing class's tag scope
  // so any newly created tag is routed to the namespace and unifies with a
  // later definition.
  LocalSymbolTable* saved_tag_stack = syntax->local_tag_stack;
  syntax->local_tag_stack = NULL;

  bool starts_with_qualified_name =
      LexLookingAt(syntax->lex, TOK(identifier)) &&
      SyntaxCurrentIdentifierFollowedByScopeOperator(syntax);
  bool qualifier_may_be_dependent =
      starts_with_qualified_name && FriendQualifierMayBeDependent(syntax);
  bool used_friend_type_only_context =
      CompilerCXXAtLeast(kLanguageStandardCXX26) && starts_with_qualified_name;
  FriendSpecifiers spec;
  ParseFriendSpecifiers(syntax, &spec);

  if (LexLookingAt(syntax->lex, TOK(semicolon)) ||
      LexLookingAt(syntax->lex, TOK(comma)) ||
      LexLookingAt(syntax->lex, TOK(ellipsis))) {
    ParseFriendTypeDeclaration(syntax, befriending, &spec,
                               qualifier_may_be_dependent);
  } else {
    ParseFriendFunctionDeclaration(syntax, befriending, &spec,
                                   used_friend_type_only_context);
  }
  syntax->local_tag_stack = saved_tag_stack;
}
