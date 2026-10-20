//
//  expr_parser_template.c
//  c_compiler
//
//  Dependent names, template-ids, and identifier expressions.
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


static bool SymbolHasFunctionTemplateOverload(Symbol* symbol);
static bool StructMemberHasFunctionTemplate(StructMember* member);
static bool IdentifierNamesFunctionTemplateOverload(Symbol* symbol);
static void AttachPendingMemberTemplateArguments(Syntax* syntax, ASTNode* member_access, TokenClass followers);
static bool ExpressionIdentifierNeedsTemplateIdParser(Syntax* syntax);
static ASTNode* BuildDependentQualifiedValueName(Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location);
static bool StructHasDependentBase(Struct* str);
static bool UnresolvedNameMayBeDependentMember(Syntax* syntax);
static Struct* FindEnclosingClassByName(Syntax* syntax, String* name);
static ASTNode* BuildDependentCurrentInstantiationMember( Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location);
static bool TemplateArgumentListIsDependent(Vector* args);
static ASTNode* BuildDependentMemberTemplateValueName( Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location);
static ASTNode* BuildDependentTemplateScopeValueName( Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location);

// Reports whether a symbol’s overload chain contains a function template.
static bool SymbolHasFunctionTemplateOverload(Symbol* symbol) {
  for (Symbol* candidate = symbol; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->flags.is_template && candidate->type != NULL &&
        TypeIsFunction(candidate->type)) {
      return true;
    }
  }
  return false;
}


// Reports whether a member overload chain contains a function template.
static bool StructMemberHasFunctionTemplate(StructMember* member) {
  for (; member != NULL; member = member->overload_next) {
    if (member->symbol != NULL && member->symbol->flags.is_template &&
        member->symbol->type != NULL && TypeIsFunction(member->symbol->type)) {
      return true;
    }
  }
  return false;
}


// Checks both a symbol overload chain and its owning class for a
// function-template overload.
static bool IdentifierNamesFunctionTemplateOverload(Symbol* symbol) {
  if (SymbolHasFunctionTemplateOverload(symbol)) {
    return true;
  }
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return false;
  }
  Struct* owner = symbol->type->info.function.cxx_member_owner;
  if (owner == NULL) {
    return false;
  }
  return StructMemberHasFunctionTemplate(
      FindStructMemberByName(owner, symbol->name.value));
}


// Unqualified `InitializeStorage<QualTRef>(args...)` is rewritten to
// `this->InitializeStorage` before `<` is seen.  The arrow form does not
// otherwise keep explicit template arguments, so `<` is parsed as less-than
// and the call's closing `)` is lost.  Attach a pending `<...>` to the
// member-name node; member analysis copies it onto the struct-member node.
static void AttachPendingMemberTemplateArguments(Syntax* syntax,
                                                 ASTNode* member_access,
                                                 TokenClass followers) {
  if (!CompilerIsCXX() || member_access == NULL ||
      member_access->op != AST_OP(arrow) ||
      !LexLookingAt(syntax->lex, TOK(less))) {
    return;
  }
  Vector* args = SyntaxParseTemplateArgumentList(syntax, followers);
  if (args == NULL) {
    return;
  }
  ASTNode* member_name = ((BinaryASTNode*)member_access)->right;
  if (member_name == NULL) {
    VectorDestructWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
    return;
  }
  ((ConstantASTNode*)member_name)->template_arguments = args;
}


// Looks ahead for template-ids in a qualified expression name without
// consuming input.
static bool ExpressionIdentifierNeedsTemplateIdParser(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool needs_template_ids = false;
  bool saw_coloncolon = false;
  if (LexMatch(syntax->lex, TOK(coloncolon))) {
    saw_coloncolon = true;
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      goto done;
    }
  }
  while (LexLookingAt(syntax->lex, TOK(identifier))) {
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(less))) {
      int depth = 0;
      int paren_depth = 0;
      int square_depth = 0;
      int brace_depth = 0;
      do {
        // A `;` or brace at the top level of this `<...>` cannot belong to a
        // template-argument list (`a < b;`, `a < b {`).  Braces nested inside
        // parentheses are part of the argument (`is_same<decltype(T{}), U>`),
        // so they must not abort the lookahead.  Stopping at a real top-level
        // terminator also keeps the scan from running to EOF, which frees the
        // current source out from under the checkpoint restored below.
        Token token = syntax->lex->current_token;
        bool at_top = paren_depth == 0 && square_depth == 0 &&
                      brace_depth == 0;
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
            depth -= LexClosingAngleCount(token);
          }
        }
        LexNextToken(syntax->lex);
      } while (!LexEof(syntax->lex) && depth > 0);
      if (depth == 0) {
        if (LexLookingAt(syntax->lex, TOK(coloncolon))) {
          needs_template_ids = true;
          goto done;
        }
        // `X::Flush<T>` as a complete id-expression (`&X::Flush<T>`, an
        // argument, a mem-initializer).  A comparison such as
        // `X::kMax < a && b > c` continues with another identifier after `>`.
        if (saw_coloncolon &&
            (LexLookingAt(syntax->lex, TOK(rparen)) ||
             LexLookingAt(syntax->lex, TOK(comma)) ||
             LexLookingAt(syntax->lex, TOK(semicolon)) ||
             LexLookingAt(syntax->lex, TOK(colon)) ||
             LexLookingAt(syntax->lex, TOK(rbrace)))) {
          needs_template_ids = true;
          goto done;
        }
      }
    }
    if (!LexMatch(syntax->lex, TOK(coloncolon))) {
      break;
    }
    saw_coloncolon = true;
    // `X::template Apply<T>::value` is a template-id even when the first
    // component (`X`) is not itself followed by `<`.
    if (LexMatch(syntax->lex, TOK(template))) {
      needs_template_ids = true;
    }
  }

done:
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return needs_template_ids;
}


// `typedef W<char> WC;` inside `W<T>`: while `W` is being defined, `WC` cannot
// be instantiated, and its members are not those of the class being defined.
static bool TypeNamesOtherSpecializationOfEnclosingClass(Syntax* syntax,
                                                         TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->template_arguments == NULL || type->info.struct_info == NULL) {
    return false;
  }
  Struct* scope = syntax->cxx_class_head;
  if (scope == NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    scope = compiler->current_function->info.function.cxx_member_owner;
  }
  for (; scope != NULL; scope = scope->lexical_parent) {
    if (scope == type->info.struct_info ||
        (scope->tag_symbol != NULL &&
         type->template_origin == scope->tag_symbol)) {
      return EnclosingClassTemplateForOtherSpecialization(
                 scope, type->template_arguments) != NULL;
    }
  }
  return false;
}

// A qualified name like `T::member` or `Alias<T>::member::value` whose leading
// nested-name-specifier is dependent cannot be resolved until the template is
// instantiated.  Builds a placeholder identifier carrying the dependent scope
// and the trailing member name (in dependent_member_name), flagged so the
// template-body cloner can resolve it against the concrete type argument.
// Returns NULL when `name` is not such a dependent qualified value name.
static ASTNode* BuildDependentQualifiedValueName(Syntax* syntax,
                                                  FullyQualifiedIdentifier* name,
                                                  SourceLocation location) {
  if (!CompilerIsCXX() || !name->is_qualified || name->absolute ||
      name->components.length < 2) {
    return NULL;
  }
  // A template-id on the scope itself (`Trait<T>::member`) is handled by the
  // scope builders.  Arguments on the final component (`T::template f<U>`)
  // belong to the member function template.  Arguments on a middle component
  // (`H::template is_hashable<T>::value`) stay on the dependent member path.
  Vector* member_template_arguments = NULL;
  bool intermediate_member_template_id = false;
  for (size_t i = 0; i < name->template_arguments.length; i++) {
    Vector* args = name->template_arguments.value.p[i];
    if (args == NULL) {
      continue;
    }
    if (i == 0) {
      return NULL;
    }
    if (i + 1 == name->components.length) {
      member_template_arguments = args;
    } else {
      intermediate_member_template_id = true;
    }
  }
  String* scope = name->components.value.p[0];
  Symbol* scope_symbol = SyntaxFindSymbol(syntax, scope);
  // The scope must name a dependent type: either a template type parameter (or
  // alias to one), or an alias to a dependent class-template specialization such
  // as `using traits_type = allocator_traits<Alloc>;`.
  if (scope_symbol == NULL || scope_symbol->type == NULL) {
    return NULL;
  }
  if (!scope_symbol->flags.is_template_type_parameter &&
      !StorageIs(scope_symbol->storage, STO(typedef))) {
    return NULL;
  }
  bool dependent_scope =
      scope_symbol->type->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(scope_symbol->type) ||
      (TypeDependentOf(scope_symbol->type)->dependent_member_name == NULL &&
       scope_symbol->type->template_origin != NULL &&
       TemplateArgumentListIsDependent(
           scope_symbol->type->template_arguments)) ||
      TypeNamesOtherSpecializationOfEnclosingClass(syntax, scope_symbol->type);
  if (!dependent_scope) {
    return NULL;
  }
  String* member =
      name->components.value.p[name->components.length - 1];
  TypeRecord* dependent_type = TypeRecordCopy(scope_symbol->type);
  if (TypeDependentOf(dependent_type)->dependent_member_name == NULL) {
    TypeMutableDependent(dependent_type)->dependent_member_name = NewString("");
  } else {
    // The scope alias is itself a dependent member access (e.g. `q` was
    // `typename W::period`).  Extend the existing member path rather than
    // discarding it, so `q::num` becomes the full `W::period::num` path instead
    // of collapsing to `W::num` (which would drop the intermediate member and
    // leave the name unresolvable).
    StringAppend(TypeDependentOf(dependent_type)->dependent_member_name, "::");
  }
  for (size_t i = 1; i < name->components.length; i++) {
    if (i != 1) {
      StringAppend(TypeDependentOf(dependent_type)->dependent_member_name,
                   "::");
    }
    StringAppendString(TypeDependentOf(dependent_type)->dependent_member_name,
                       name->components.value.p[i]);
  }
  if (intermediate_member_template_id &&
      TypeDependentOf(dependent_type)->dependent_member_template_arguments ==
          NULL) {
    Vector* path_args = NewVector();
    for (size_t i = 1; i < name->components.length; i++) {
      Vector* args = i < name->template_arguments.length
                         ? name->template_arguments.value.p[i]
                         : NULL;
      VectorAppend(path_args, TemplateArgumentVectorCopy(args));
    }
    TypeMutableDependent(dependent_type)->dependent_member_template_arguments =
        path_args;
  }
  Symbol* placeholder = NewSymbol(member->value, dependent_type, STO(implicit));
  placeholder->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(placeholder, location);
  node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
  if (member_template_arguments != NULL && !intermediate_member_template_id) {
    ((IdentifierASTNode*)node)->template_arguments =
        TemplateArgumentVectorCopy(member_template_arguments);
  }
  return node;
}


// `Derived::member` inside a class template names a member of the current
// instantiation.  If it is not declared in the class itself, it may still
// come from a dependent base and must not be diagnosed until instantiation
// ([temp.dep]).
static bool StructHasDependentBase(Struct* str) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->type == NULL) {
      continue;
    }
    if (TypeContainsTemplateParameter(base->type) ||
        TypeDependentOf(base->type)->dependent_member_name != NULL ||
        base->type->template_parameter_index >= 0) {
      return true;
    }
  }
  return false;
}


// An unqualified name that is not a member of the class being defined is
// still a member when it comes from a dependent base ([temp.dep]).  A
// concrete class has already been searched; inventing `this->name` there
// turns a free function such as `clock_gettime` into a missing member.
static bool UnresolvedNameMayBeDependentMember(Syntax* syntax) {
  Struct* owner = NULL;
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  if (StructHasDependentBase(owner) ||
      StructHasDependentBase(StructFromThisSymbol(lambda_enclosing_this)) ||
      (syntax != NULL && StructHasDependentBase(syntax->cxx_class_head))) {
    return true;
  }
  return false;
}


// Finds the lexically enclosing class whose injected class name matches the
// requested name.
static Struct* FindEnclosingClassByName(Syntax* syntax, String* name) {
  Struct* scope = syntax != NULL ? syntax->cxx_class_head : NULL;
  if (scope == NULL && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    scope = compiler->current_function->info.function.cxx_member_owner;
  }
  for (; scope != NULL; scope = scope->lexical_parent) {
    if (CurrentClassNameMatchesTypeName(scope, name)) {
      return scope;
    }
  }
  return NULL;
}


// Builds a deferred qualified member of the current instantiation when it may
// come from a dependent base.
static ASTNode* BuildDependentCurrentInstantiationMember(
    Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location) {
  if (!CompilerIsCXX() || syntax == NULL || name == NULL ||
      !name->is_qualified || name->components.length != 2) {
    return NULL;
  }
  Struct* owner =
      FindEnclosingClassByName(syntax, name->components.value.p[0]);
  if (owner == NULL || owner->tag_symbol == NULL ||
      !StructHasDependentBase(owner)) {
    return NULL;
  }
  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  StructMember* member = FindStructMember(owner, &member_name);
  StringDestruct(&member_name);
  if (member != NULL) {
    return NULL;
  }
  String* member_spelling =
      name->components.value.p[name->components.length - 1];
  TypeRecord* dependent_type =
      NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
  dependent_type->template_origin = owner->tag_symbol;
  TypeMutableDependent(dependent_type)->dependent_member_name =
      NewString(member_spelling->value);
  Symbol* placeholder =
      NewSymbol(member_spelling->value, dependent_type, STO(implicit));
  placeholder->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(placeholder, location);
  node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
  if (name->template_arguments.length == name->components.length) {
    Vector* member_args =
        name->template_arguments.value.p[name->components.length - 1];
    if (member_args != NULL) {
      ((IdentifierASTNode*)node)->template_arguments =
          TemplateArgumentVectorCopy(member_args);
    }
  }
  return node;
}


// True if any template argument in `args` mentions a template parameter, making
// the argument list (and thus a class template specialization using it) dependent.
static bool TemplateArgumentListIsDependent(Vector* args) {
  if (args == NULL) {
    return false;
  }
  for (size_t i = 0; i < args->length; i++) {
    TemplateArgument* a = args->value.p[i];
    if (a == NULL) {
      continue;
    }
    if (a->kind == kTemplateParameterType &&
        TypeContainsTemplateParameter(a->type)) {
      return true;
    }
    if (a->kind == kTemplateParameterNonType && a->template_parameter_index >= 0) {
      return true;
    }
    if (a->kind == kTemplateParameterTemplate &&
        a->template_parameter_index >= 0) {
      return true;
    }
  }
  return false;
}


// `Impl<Sig>::template CallIsValid<F>::value`: a dependent class-template
// specialization, then a member alias/class template, then a value member.
// The two-component helper below treats the second-to-last component as the
// scope template and would try to resolve `Impl::CallIsValid` against the
// (often empty) primary.  Keep the whole path dependent, rooted at the
// leftmost class/alias template.
static ASTNode* BuildDependentMemberTemplateValueName(
    Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location) {
  if (!CompilerIsCXX() || !name->is_qualified ||
      syntax->current_template_parameter_count <= 0 ||
      name->components.length < 3 ||
      name->template_arguments.length != name->components.length) {
    return NULL;
  }
  size_t last = name->components.length - 1;
  if (name->template_arguments.value.p[last] != NULL) {
    return NULL;
  }
  for (size_t origin_index = 0; origin_index + 2 < name->components.length;
       origin_index++) {
    Vector* origin_args = name->template_arguments.value.p[origin_index];
    if (origin_args == NULL ||
        (!TemplateArgumentVectorContainsTemplateParameter(origin_args) &&
         !TemplateArgumentListIsDependent(origin_args))) {
      continue;
    }
    Symbol* base = NULL;
    if (origin_index == 0 && !name->absolute) {
      String* origin_name = name->components.value.p[0];
      base = SyntaxFindSymbol(syntax, origin_name);
      if (base == NULL) {
        base = SyntaxFindTag(syntax, origin_name);
      }
    } else {
      FullyQualifiedIdentifier prefix;
      FullyQualifiedIdentifierInit(&prefix);
      prefix.absolute = name->absolute;
      prefix.is_qualified = true;
      for (size_t i = 0; i <= origin_index; i++) {
        String* component = name->components.value.p[i];
        if (prefix.spelling.length != 0 || prefix.absolute) {
          StringAppend(&prefix.spelling, "::");
        }
        StringAppendString(&prefix.spelling, component);
        VectorAppend(&prefix.components, NewString(component->value));
        VectorAppend(&prefix.template_arguments, NULL);
      }
      base = SyntaxFindQualifiedSymbol(syntax, &prefix);
      if (base == NULL) {
        base = SyntaxFindQualifiedTag(syntax, &prefix);
      }
      FullyQualifiedIdentifierDestruct(&prefix);
    }
    if (base == NULL || !base->flags.is_template || base->type == NULL ||
        (!TypeIsStructOrUnion(base->type) &&
         !StorageIs(base->storage, STO(typedef)))) {
      continue;
    }
    String path;
    StringInit(&path, NULL);
    Vector* path_args = NewVector();
    for (size_t i = origin_index + 1; i < name->components.length; i++) {
      if (path.length != 0) {
        StringAppend(&path, "::");
      }
      StringAppendString(&path, name->components.value.p[i]);
      VectorAppend(path_args, TemplateArgumentVectorCopy(
                                  name->template_arguments.value.p[i]));
    }
    TypeRecord* dependent_type =
        NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
    dependent_type->template_origin = base;
    dependent_type->template_arguments =
        TemplateArgumentVectorCopy(origin_args);
    TypeMutableDependent(dependent_type)->dependent_member_name =
        NewString(path.value);
    TypeMutableDependent(dependent_type)->dependent_member_template_arguments =
        path_args;
    StringDestruct(&path);
    String* member = name->components.value.p[last];
    Symbol* placeholder =
        NewSymbol(member->value, dependent_type, STO(implicit));
    placeholder->flags.invented = true;
    ASTNode* node = NewIdentifierASTNode(placeholder, location);
    node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
    return node;
  }
  return NULL;
}


// A qualified value name like `Trait<Deps...>::member` whose nested-name-specifier
// is a *dependent* class template specialization (its template arguments mention a
// template parameter) is a value-dependent name: the specialization chosen (primary
// vs. an explicit/partial specialization) is not known until the enclosing template
// is instantiated, so its `::member` must not be resolved (and folded) now.  Build a
// placeholder identifier carrying the dependent scope (template_origin +
// template_arguments) and the trailing member name (dependent_member_name), flagged
// so the template-body cloner resolves it against the substituted arguments.  Returns
// NULL when `name` is not such a dependent template-scope value name.
static ASTNode* BuildDependentTemplateScopeValueName(
    Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location) {
  // Class-template member functions are not themselves function templates, so
  // a deferred `noexcept(Trait<allocator_type>::value)` may be replayed with
  // `current_template_parameter_count == 0`.  Dependent arguments on the
  // scope template-id are enough to keep `::member` unresolved.
  if (!CompilerIsCXX() || !name->is_qualified ||
      name->components.length < 2 ||
      name->template_arguments.length != name->components.length) {
    return NULL;
  }
  size_t member_index = name->components.length - 1;
  size_t base_index = name->components.length - 2;
  // A member template-id (`Traits<Policy<K>>::template get<H, B>`) keeps its
  // arguments on the placeholder.  Looking the member up now would find the
  // primary's member template and lose the scope arguments.
  Vector* member_args = name->template_arguments.value.p[member_index];
  Vector* scope_args = name->template_arguments.value.p[base_index];
  // Inside `W<T>`, `W<char>::f` and `W<T*>::f` name members of other
  // specializations, not of the class being defined.
  Symbol* other_specialization = NULL;
  if (base_index == 0 && scope_args != NULL) {
    Struct* enclosing =
        FindEnclosingClassByName(syntax, name->components.value.p[0]);
    // `::W<char>::f` names the same class only when `W` is a global class.
    if (enclosing != NULL && name->absolute &&
        (enclosing->lexical_parent != NULL || enclosing->tag_symbol == NULL ||
         (enclosing->tag_symbol->namespace_ != NULL &&
          enclosing->tag_symbol->namespace_ != compiler->global_namespace))) {
      enclosing = NULL;
    }
    other_specialization =
        EnclosingClassTemplateForOtherSpecialization(enclosing, scope_args);
  }
  if (other_specialization == NULL &&
      !TemplateArgumentVectorContainsTemplateParameter(scope_args) &&
      !TemplateArgumentListIsDependent(scope_args)) {
    return NULL;
  }
  // Resolve the class or alias template that names the scope
  // (components[0..base_index]).  Alias templates such as
  // `conditional_t<B, T, F>` must stay dependent too: instantiating the
  // primary now would drop the arguments and look up `::member` on the
  // alias itself.
  FullyQualifiedIdentifier prefix;
  FullyQualifiedIdentifierInit(&prefix);
  prefix.absolute = name->absolute;
  prefix.is_qualified = prefix.absolute || base_index > 0;
  for (size_t i = 0; i <= base_index; i++) {
    String* component = name->components.value.p[i];
    if (prefix.spelling.length != 0 || prefix.absolute) {
      StringAppend(&prefix.spelling, "::");
    }
    StringAppendString(&prefix.spelling, component);
    VectorAppend(&prefix.components, NewString(component->value));
    // Look up the template itself.  Carrying the (still dependent) argument
    // list makes qualified lookup try to instantiate it and miss the primary,
    // so `Trait<T>::value` never stays dependent.
    VectorAppend(&prefix.template_arguments, NULL);
  }
  Symbol* base = other_specialization != NULL
                     ? other_specialization
                     : SyntaxFindQualifiedSymbol(syntax, &prefix);
  if (base == NULL) {
    base = SyntaxFindQualifiedTag(syntax, &prefix);
  }
  // `std::is_invocable_r` is a namespace tag.  Qualified tag lookup can miss
  // it when the prefix was built only to name the template; resolve the
  // leading namespace and the final component directly.
  if (base == NULL && base_index >= 1) {
    FullyQualifiedIdentifier ns_name;
    FullyQualifiedIdentifierInit(&ns_name);
    ns_name.absolute = name->absolute;
    ns_name.is_qualified = name->absolute || base_index > 1;
    for (size_t i = 0; i < base_index; i++) {
      String* component = name->components.value.p[i];
      if (ns_name.spelling.length != 0 || ns_name.absolute) {
        StringAppend(&ns_name.spelling, "::");
      }
      StringAppendString(&ns_name.spelling, component);
      VectorAppend(&ns_name.components, NewString(component->value));
      VectorAppend(&ns_name.template_arguments, NULL);
    }
    Namespace* ns = SyntaxFindQualifiedNamespace(syntax, &ns_name);
    FullyQualifiedIdentifierDestruct(&ns_name);
    if (ns != NULL) {
      String* trait_name = name->components.value.p[base_index];
      NamespaceInlineTagLookup found =
          NamespaceResolveTagInInlineSet(ns, trait_name);
      if (found.status == kInlineLookupUnique) {
        base = found.tag;
      } else {
        NamespaceInlineSymbolLookup symfound =
            NamespaceResolveSymbolInInlineSet(ns, trait_name);
        if (symfound.status == kInlineLookupUnique &&
            symfound.symbol != NULL && symfound.symbol->flags.is_template) {
          base = symfound.symbol;
        }
      }
    }
  }
  FullyQualifiedIdentifierDestruct(&prefix);
  if (base == NULL ||
      (!base->flags.is_template && base != other_specialization) ||
      base->type == NULL ||
      (!TypeIsStructOrUnion(base->type) &&
       !StorageIs(base->storage, STO(typedef)))) {
    return NULL;
  }
  String* member = name->components.value.p[member_index];
  TypeRecord* dependent_type = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
  dependent_type->template_origin = base;
  dependent_type->template_arguments = TemplateArgumentVectorCopy(scope_args);
  TypeMutableDependent(dependent_type)->dependent_member_name =
      NewString(member->value);
  Symbol* placeholder = NewSymbol(member->value, dependent_type, STO(implicit));
  placeholder->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(placeholder, location);
  node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
  if (member_args != NULL) {
    ((IdentifierASTNode*)node)->template_arguments =
        TemplateArgumentVectorCopy(member_args);
  }
  return node;
}


/* `Spy<A>::HashStateBase::combine` inside a class template, where the middle
 * name is the injected-class-name of a dependent base (`HashStateBase<Spy<T>>`).
 * Looking `combine` up now finds the base template's pattern member, whose own
 * parameters are never bound.  Keep the name dependent: on `Spy<A>` with the
 * member path `HashStateBase::combine`, or, for the current class
 * (`Spy::HashStateBase::combine`), on the base specialization itself. */
static ASTNode* BuildDependentInjectedBaseValueName(
    Syntax* syntax, FullyQualifiedIdentifier* name, SourceLocation location) {
  if (!CompilerIsCXX() || !name->is_qualified || name->absolute ||
      name->components.length != 3) {
    return NULL;
  }
  Vector* scope_args = name->template_arguments.length > 0
                           ? name->template_arguments.value.p[0]
                           : NULL;
  Vector* base_args = name->template_arguments.length > 1
                          ? name->template_arguments.value.p[1]
                          : NULL;
  Vector* member_args = name->template_arguments.length > 2
                            ? name->template_arguments.value.p[2]
                            : NULL;
  if (base_args != NULL) {
    return NULL;
  }
  Symbol* injected = SyntaxFindQualifiedPrefixSymbol(syntax, name, 2);
  if (injected == NULL || !injected->flags.invented ||
      !StorageIs(injected->storage, STO(typedef)) || injected->type == NULL ||
      !TypeIsStructOrUnion(injected->type)) {
    return NULL;
  }
  String* base_name = name->components.value.p[1];
  String* member = name->components.value.p[2];
  TypeRecord* dependent_type = NULL;
  if (scope_args == NULL) {
    dependent_type = TypeRecordCopy(injected->type);
    TypeMutableDependent(dependent_type)->dependent_member_name =
        NewString(member->value);
  } else {
    String* scope_name = name->components.value.p[0];
    // Inside the class template being defined, `Spy` is its injected-class-
    // name and the tag is not yet flagged as a template; `Spy<A>` names the
    // template whose tag that is.
    Struct* enclosing = FindEnclosingClassByName(syntax, scope_name);
    Symbol* scope = enclosing != NULL ? enclosing->tag_symbol : NULL;
    if (scope != NULL && scope->type != NULL &&
        scope->type->template_origin != NULL) {
      scope = scope->type->template_origin;
    }
    if (scope == NULL) {
      FullyQualifiedIdentifier prefix;
      FullyQualifiedIdentifierInit(&prefix);
      StringAppendString(&prefix.spelling, scope_name);
      VectorAppend(&prefix.components, NewString(scope_name->value));
      VectorAppend(&prefix.template_arguments, NULL);
      scope = SyntaxFindQualifiedSymbol(syntax, &prefix);
      if (scope == NULL) {
        scope = SyntaxFindQualifiedTag(syntax, &prefix);
      }
      FullyQualifiedIdentifierDestruct(&prefix);
      if (scope != NULL && !scope->flags.is_template) {
        return NULL;
      }
    }
    if (scope == NULL || scope->type == NULL ||
        !TypeIsStructOrUnion(scope->type)) {
      return NULL;
    }
    dependent_type = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
    dependent_type->template_origin = scope;
    dependent_type->template_arguments = TemplateArgumentVectorCopy(scope_args);
    TypeMutableDependent(dependent_type)->dependent_member_name =
        NewString(base_name->value);
    StringAppend(TypeDependentOf(dependent_type)->dependent_member_name, "::");
    StringAppendString(TypeDependentOf(dependent_type)->dependent_member_name,
                       member);
  }
  Symbol* placeholder = NewSymbol(member->value, dependent_type, STO(implicit));
  placeholder->flags.invented = true;
  ASTNode* node = NewIdentifierASTNode(placeholder, location);
  node->flags |= kASTQualifiedName | kASTDependentQualifiedName;
  if (member_args != NULL) {
    ((IdentifierASTNode*)node)->template_arguments =
        TemplateArgumentVectorCopy(member_args);
  }
  return node;
}

// An id-expression: a bare name, a qualified name, or an operator-function-id.
// Dependent qualified names stay unresolved; an unqualified class member is
// rewritten to `this->member` when lookup would otherwise hide it.
ASTNode* ParseIdentifier(Syntax* syntax,
                                            TokenClass followers) {
  Lex* lex = syntax->lex;
  SourceLocation identifier_location = lex->current_token_location;

  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  bool parsed = ExpressionIdentifierNeedsTemplateIdParser(syntax)
      ? SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                          followers)
      : SyntaxParseFullyQualifiedIdentifier(syntax, &name);
  if (!parsed) {
    SyntaxError(syntax, "Expected identifier");
    FullyQualifiedIdentifierDestruct(&name);
    return (ASTNode*)NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        syntax->lex->current_token_location);
  }

  // In preprocesor mode we have no symbols, everything is a macro name.
  if (lex->preprocessor_mode) {
    String macro_name;
    StringInit(&macro_name, FullyQualifiedIdentifierLast(&name));
    if (PreprocessorFindMacro(lex->preprocessor, &macro_name) == NULL) {
      PreprocessorWarning(lex->preprocessor, "undef",
                          "'%s' is not defined, evaluates to 0",
                          macro_name.value);
    }
    ASTNode* result = NewMacroNameASTNode(&macro_name, lex->current_token_location);
    StringDestruct(&macro_name);
    FullyQualifiedIdentifierDestruct(&name);
    return result;
  }

  // A value name qualified by a dependent class-template specialization
  // (`Trait<T>::member`) must stay dependent: resolving it now would silently
  // select the primary template and fold the wrong value.  Detect and defer it
  // before the ordinary lookup, which would otherwise instantiate the primary.
  if (name.is_qualified) {
    ASTNode* dependent_scope = BuildDependentMemberTemplateValueName(
        syntax, &name, lex->current_token_location);
    if (dependent_scope != NULL) {
      FullyQualifiedIdentifierDestruct(&name);
      return dependent_scope;
    }
    dependent_scope = BuildDependentInjectedBaseValueName(
        syntax, &name, lex->current_token_location);
    if (dependent_scope != NULL) {
      FullyQualifiedIdentifierDestruct(&name);
      return dependent_scope;
    }
    dependent_scope = BuildDependentTemplateScopeValueName(
        syntax, &name, lex->current_token_location);
    if (dependent_scope != NULL) {
      FullyQualifiedIdentifierDestruct(&name);
      return dependent_scope;
    }
    dependent_scope = BuildDependentQualifiedValueName(
        syntax, &name, lex->current_token_location);
    if (dependent_scope != NULL) {
      FullyQualifiedIdentifierDestruct(&name);
      return dependent_scope;
    }
  }

  // Find the symbol by searching all symbol tables.  It must exist.
  Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, &name);
  // In a nested-name-specifier, a member function can hide an enclosing class
  // with the same ordinary name (`ReflectionVisit::ExtensionSet()` versus the
  // namespace type `ExtensionSet`).  If value lookup cannot complete the
  // qualified name, retry tag lookup so `ExtensionSet::Prefetch{}` can name
  // the nested class.
  if (symbol == NULL && name.is_qualified) {
    symbol = SyntaxFindQualifiedTag(syntax, &name);
  }

  // An injected class name used in a functional construction expression
  // denotes the class type, even when ordinary member lookup found the
  // same-named constructor overload first.  Redirect source-level `T(...)`
  // through the enclosing class tag before constructing the identifier AST.
  if (symbol != NULL && symbol->type != NULL &&
      TypeIsFunction(symbol->type) &&
      symbol->type->info.function.is_constructor &&
      symbol->type->info.function.cxx_member_owner != NULL &&
      LexLookingAt(lex, TOK(lparen))) {
    Symbol* self_tag = CurrentClassSelfTagSymbol(syntax, &name);
    if (self_tag != NULL && self_tag->type != NULL &&
        TypeIsStructOrUnion(self_tag->type) &&
        self_tag->type->info.struct_info ==
            symbol->type->info.function.cxx_member_owner) {
      symbol = self_tag;
    }
  }

  // Name hiding ([basic.lookup.unqual], [class.member.lookup]): inside a member
  // function, an unqualified name that names a non-static member of the class
  // (or one of its bases) hides any entity of the same name declared in an
  // enclosing namespace.  Ordinary lookup above searches namespace scopes too,
  // so for a class defined in `namespace std`, an unqualified `end()` call can
  // wrongly bind to the free function template `std::end` instead of the
  // class's own `end` member (e.g. `map::rbegin`'s `reverse_iterator(end())`).
  // A block-scope declaration (local variable or parameter) is nearer than the
  // class and legitimately hides the member, so only redirect when the found
  // entity is at namespace/global scope.  Names carrying explicit template
  // arguments are left alone: the arrow member-access form built here does not
  // retain them.
  Vector* hiding_last_args =
      name.template_arguments.length > 0
          ? (Vector*)name.template_arguments.value
                .p[name.template_arguments.length - 1]
          : NULL;
  bool hiding_has_explicit_template_args =
      hiding_last_args != NULL && hiding_last_args->length > 0;
  // Member function templates found while parsing an inline definition
  // may be marked block-scope.  `return put(v)` must still become
  // `this->put(v)` so deduction sees a member call.  A true local or
  // parameter continues to hide.
  //
  // A block-scope using-declaration (`using std::swap;`) nominates a
  // namespace entity.  Ordinary lookup follows the alias, so `symbol`
  // above is that entity and is not itself marked block-scope.  The
  // using still hides class members of the same name.
  bool block_scope_using_hides_member = false;
  if (!name.is_qualified && name.components.length == 1 &&
      syntax->local_symbol_stack != NULL) {
    Symbol* local = FindLocalSymbol(syntax->local_symbol_stack,
                                    name.components.value.p[0]);
    block_scope_using_hides_member =
        local != NULL && local->flags.is_using_alias &&
        local->flags.is_block_scope;
  }
  // A local class member function does not see the enclosing function's
  // automatic variables.  `Sort`'s parameter `nodes` must not hide
  // `ByRank::nodes` inside `ByRank::operator()`.
  bool enclosing_automatic = false;
  if (symbol != NULL && symbol->flags.is_block_scope &&
      symbol->flags.is_argument && compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    enclosing_automatic = true;
    Vector* proto = &compiler->current_function->info.function.prototype;
    for (size_t i = 0; i < proto->length; i++) {
      if (proto->value.p[i] == symbol) {
        enclosing_automatic = false;
        break;
      }
    }
  }
  bool block_hides_member =
      !enclosing_automatic &&
      (block_scope_using_hides_member ||
       (symbol != NULL && symbol->flags.is_block_scope &&
        // Class-scope member functions are injected into the class-body
        // symbol table, which marks them block-scope.  They are not local
        // declarations and must not hide a non-static overload of the same
        // name (`Capacity()` vs `static Capacity(size_t)`).
        !(symbol->type != NULL && TypeIsFunction(symbol->type) &&
          symbol->type->info.function.cxx_member_owner != NULL)));
  // A default member initializer has no `this` yet, but a non-static data
  // member it names is still accessed through the object being initialized.
  if (CompilerIsCXX() && !name.is_qualified && symbol != NULL &&
      !block_hides_member && syntax->parsing_default_member_initializer &&
      syntax->cxx_class_head != NULL && FindThisSymbol(syntax) == NULL) {
    String member_name;
    StringInit(&member_name, FullyQualifiedIdentifierLast(&name));
    StructMember* member =
        FindStructMember(syntax->cxx_class_head, &member_name);
    StringDestruct(&member_name);
    if (member != NULL && member->symbol != NULL && !member->is_static &&
        !member->is_member_function &&
        !StorageIs(member->symbol->storage, STO(typedef)) &&
        !SymbolIsTagSymbol(member->symbol)) {
      ASTNode* member_access = NewMemberAccessFromEnclosingClass(syntax, &name);
      if (member_access != NULL) {
        FullyQualifiedIdentifierDestruct(&name);
        return member_access;
      }
    }
  }
  // `symbol == NULL` still counts: out-of-line member bodies do not put
  // class members in the ordinary symbol table, so `Init<kFront>` would
  // otherwise be parsed as a comparison.
  if (CompilerIsCXX() && !name.is_qualified && !block_hides_member &&
      !hiding_has_explicit_template_args &&
      (symbol != NULL || LexLookingAt(syntax->lex, TOK(less)) ||
       LexLookingAt(syntax->lex, TOK(lparen)))) {
    Symbol* this_symbol = ThisSymbolForUnqualifiedMember(
        syntax, &name, /*allow_unresolved_member=*/false);
    if (this_symbol != NULL && this_symbol->type != NULL &&
        TypeIsStructOrUnionPointer(this_symbol->type) &&
        this_symbol->type->next != NULL &&
        this_symbol->type->next->info.struct_info != NULL) {
      String member_name;
      StringInit(&member_name, FullyQualifiedIdentifierLast(&name));
      StructMember* member = FindStructMember(
          this_symbol->type->next->info.struct_info, &member_name);
      StringDestruct(&member_name);
      bool member_is_constructor =
          member != NULL && member->symbol != NULL &&
          member->symbol->type != NULL &&
          TypeIsFunction(member->symbol->type) &&
          member->symbol->type->info.function.is_constructor;
      // Nested type aliases and tags are class members, but they name types
      // (`using Policy = Tag; Policy{}`).  Rewriting them through `this`
      // turns a functional cast into leftover `{` after a member access.
      bool member_is_type_name =
          member != NULL && member->symbol != NULL &&
          (StorageIs(member->symbol->storage, STO(typedef)) ||
           SymbolIsTagSymbol(member->symbol));
      // The overload-chain head may be a static function declared first
      // (`static Capacity(size_t)` before `Capacity() const`).  Unqualified
      // lookup then binds the injected static symbol, but a later non-static
      // overload is still part of the set and needs `this->` so member
      // overload resolution can see both.
      bool member_has_nonstatic_function = false;
      for (StructMember* overload = member; overload != NULL;
           overload = overload->overload_next) {
        if (overload->is_member_function && !overload->is_static) {
          member_has_nonstatic_function = true;
          break;
        }
      }
      bool symbol_is_this_member_function =
          symbol != NULL && symbol->type != NULL &&
          TypeIsFunction(symbol->type) &&
          symbol->type->info.function.cxx_member_owner != NULL &&
          symbol->type->info.function.cxx_member_owner ==
              this_symbol->type->next->info.struct_info &&
          !symbol->type->info.function.is_constructor;
      if (((member != NULL && !member->is_static && !member_is_constructor &&
            !member_is_type_name) ||
           (member_has_nonstatic_function && !member_is_constructor &&
            !member_is_type_name)) ||
          (member == NULL && symbol_is_this_member_function)) {
        bool member_fn_template =
            (member != NULL &&
             StructMemberHasFunctionTemplate(member)) ||
            IdentifierNamesFunctionTemplateOverload(symbol);
        ASTNode* member_access = NewMemberAccessFromThis(
            syntax, &name,
            /*allow_unresolved_member=*/member == NULL);
        if (member_access != NULL) {
          if (member_fn_template) {
            AttachPendingMemberTemplateArguments(syntax, member_access,
                                                 followers);
          }
          FullyQualifiedIdentifierDestruct(&name);
          return member_access;
        }
      }
    }
  }
  if (symbol == NULL && CompilerIsCXX() && !name.is_qualified) {
    // Nested type aliases (`using Policy = Tag;`) are class members, not
    // namespace symbols.  Unqualified `Policy{}` in an out-of-line member
    // must resolve to that typedef so it is a functional cast, not a
    // missing-')' parse of `name` + `{`.
    Struct* owner = NULL;
    if (compiler->current_function != NULL &&
        TypeIsFunction(compiler->current_function)) {
      owner = compiler->current_function->info.function.cxx_member_owner;
    }
    if (owner != NULL) {
      String member_name;
      StringInit(&member_name, FullyQualifiedIdentifierLast(&name));
      StructMember* member = FindStructMember(owner, &member_name);
      StringDestruct(&member_name);
      if (member == NULL) {
        member = FindStructMemberByName(owner,
                                        FullyQualifiedIdentifierLast(&name));
      }
      if (member != NULL && member->symbol != NULL &&
          StorageIs(member->symbol->storage, STO(typedef))) {
        symbol = member->symbol;
      }
    }
  }
  if (symbol == NULL) {
    if (name.is_qualified) {
      ASTNode* member_access = NewQualifiedBaseMemberAccessFromThis(syntax, &name);
      if (member_access != NULL) {
        AttachPendingMemberTemplateArguments(syntax, member_access, followers);
        FullyQualifiedIdentifierDestruct(&name);
        return member_access;
      }
      ASTNode* dependent = BuildDependentQualifiedValueName(
          syntax, &name, lex->current_token_location);
      if (dependent == NULL) {
        dependent = BuildDependentCurrentInstantiationMember(
            syntax, &name, lex->current_token_location);
      }
      if (dependent != NULL) {
        FullyQualifiedIdentifierDestruct(&name);
        return dependent;
      }
      TypoCorrectionErrorUnknownSymbol(syntax, &name);
      TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type, STO(implicit));
      symbol->flags.invented = true;
    } else if (lex->assembler_mode) {
      // In assembler mode we have symbols but we pre-declare them
      // if they don't exist.  They are declared as variables with
      // type unsigned long.
      TypeRecord* type = NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain);
      symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type, STO(implicit));
      symbol->flags.is_forward_declared = true;
      // Register it so it is found on later references (and owned/freed by a
      // symbol table) rather than leaked.
      SyntaxAddSymbol(syntax, symbol);
    } else {
      const char* simple_name = FullyQualifiedIdentifierLast(&name);
      if (compiler->current_function != NULL &&
          TypeIsFunction(compiler->current_function) &&
          compiler->current_function->info.function.symbol != NULL &&
          (strcmp(simple_name, "__func__") == 0 ||
           strcmp(simple_name, "__FUNCTION__") == 0)) {
        String replacement;
        StringInit(&replacement, NULL);
        StringPrintf(
            &replacement, "\"%s\"",
            compiler->current_function->info.function.symbol->name.value);
        ASTNode* result = NewStringConstantASTNode(
            NewString(replacement.value), NULL, lex->current_token_location);
        StringDestruct(&replacement);
        FullyQualifiedIdentifierDestruct(&name);
        return result;
      }
      bool builtin_call =
          LexLookingAt(lex, TOK(lparen)) &&
          IsBuiltinCallName(simple_name);
      // A call that is not a member (`clock_gettime(...)` inside a concrete
      // member function) must stay a free function.  A bare name can still be
      // a data member that is absent from the ordinary symbol table
      // (`__len_` in an out-of-line `basic_string` method).
      if (!builtin_call &&
          (!LexLookingAt(lex, TOK(lparen)) ||
           UnresolvedNameMayBeDependentMember(syntax))) {
        // Before `this->name`, which accepts any name: a local class's
        // member can name a static member of the enclosing function's class.
        ASTNode* static_member = NewStaticMemberReference(syntax, &name);
        if (static_member != NULL) {
          FullyQualifiedIdentifierDestruct(&name);
          return static_member;
        }
        ASTNode* member_access = NewMemberAccessFromThis(
            syntax, &name, UnresolvedNameMayBeDependentMember(syntax));
        if (member_access != NULL) {
          FullyQualifiedIdentifierDestruct(&name);
          return member_access;
        }
        ASTNode* class_member =
            NewMemberAccessFromEnclosingClass(syntax, &name);
        if (class_member != NULL) {
          FullyQualifiedIdentifierDestruct(&name);
          return class_member;
        }
      }
      if (LexLookingAt(lex, TOK(lparen))) {
        if (!builtin_call) {
          ASTNode* static_member = NewStaticMemberReference(syntax, &name);
          if (static_member != NULL) {
            FullyQualifiedIdentifierDestruct(&name);
            return static_member;
          }
        }
        if (!builtin_call && UnresolvedNameMayBeDependentMember(syntax)) {
          ASTNode* deferred_member_access =
              NewMemberAccessFromThis(syntax, &name,
                                      /*allow_unresolved_member=*/true);
          if (deferred_member_access != NULL) {
            FullyQualifiedIdentifierDestruct(&name);
            return deferred_member_access;
          }
        }
        if (!CompilerIsCXX() &&
            GetIntrinsic(FullyQualifiedIdentifierLast(&name)) == NULL) {
          const char* callee = FullyQualifiedIdentifierLast(&name);
          const char* suggestion = TypoCorrectionFindVisibleName(syntax, callee);
          if (CompilerCAtLeast(kLanguageStandardC23)) {
            if (suggestion != NULL) {
              SyntaxError(syntax,
                          "Calling undeclared function %s; did you mean \"%s\"?",
                          callee, suggestion);
            } else {
              SyntaxError(syntax, "Calling undeclared function %s", callee);
            }
          } else if (suggestion != NULL) {
            SyntaxWarning(syntax, "implicit-function-declaration",
                          "Calling undeclared function %s; did you mean \"%s\"?",
                          callee, suggestion);
          } else {
            SyntaxWarning(syntax, "implicit-function-declaration",
                          "Calling undeclared function %s", callee);
          }
          ReportStdHeaderSuggestionForName(syntax, callee);
        }

        // Declare the function so we don't get more warnings for the same
        // function.
        TypeRecord* type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown,
                                                 kQualPlain);
        TypeRecord* func_type = NewFunctionTypeRecord();
        func_type->info.function.unknown_args =
            !CompilerCAtLeast(kLanguageStandardC23);
        TypeRecordChain(func_type, type);
        symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), func_type,
                           STO(implicit));
        symbol->flags.is_forward_declared = true;
        // Declare it so repeated calls find this symbol (no duplicate warnings)
        // and so it is owned/freed by a symbol table rather than leaked.
        SyntaxAddSymbol(syntax, symbol);
      } else {
        TypoCorrectionErrorUnknownSymbol(syntax, &name);
        TypeRecord* type =
            NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
        symbol = NewSymbol(FullyQualifiedIdentifierLast(&name), type,
                           STO(implicit));
        symbol->flags.invented = true;
        SyntaxAddSymbol(syntax, symbol);
      }
    }
  }
  if (symbol != NULL && !symbol->flags.is_template &&
      !symbol->flags.is_using_alias &&
      SymbolExtraOf(symbol)->alias_target != NULL &&
      SymbolExtraOf(symbol)->alias_target->flags.is_template &&
      name.template_arguments.length > 0) {
    Vector* parsed_args =
        name.template_arguments.value.p[name.template_arguments.length - 1];
    if (parsed_args != NULL && parsed_args->length > 0) {
      symbol = SymbolExtraOf(symbol)->alias_target;
    }
  }
  Vector* template_arguments = NULL;
  // Unqualified lookup inside a class body finds a scope-injected clone of a
  // static variable template.  The clone is marked as a template but does not
  // own the variable-template definition, so `<T>` would be parsed and dropped.
  // Use the class member, which holds the initializer.
  if (symbol != NULL && SymbolExtraOf(symbol)->variable_template == NULL &&
      symbol->flags.is_template &&
      SymbolExtraOf(symbol)->static_data_member_class != NULL) {
    StructMember* member = FindStructMember(
        SymbolExtraOf(symbol)->static_data_member_class, &symbol->name);
    if (member != NULL && member->symbol != NULL &&
        SymbolExtraOf(member->symbol)->variable_template != NULL) {
      symbol = member->symbol;
    }
  }
  if (symbol != NULL && name.template_arguments.length > 0 &&
      (TypeIsFunction(symbol->type) ||
       IdentifierNamesFunctionTemplateOverload(symbol) ||
       symbol->flags.is_template ||
       SymbolExtraOf(symbol)->variable_template != NULL)) {
    Vector* parsed_args =
        name.template_arguments.value.p[name.template_arguments.length - 1];
    template_arguments = TemplateArgumentVectorCopy(parsed_args);
  }
  // An explicit self-type template-id (`Box<T>`) inside the class's own member
  // body: the class is not marked as a template until its body is fully parsed,
  // so the resolved `symbol` is the injected-class-name (not a template).  Route
  // it through the class tag symbol and retain the arguments, mirroring an
  // ordinary `A<...>` functional cast.  The guard requires `symbol` to actually
  // denote the current class's own type (the injected-class-name), so a
  // shadowing value named like the class -- e.g. a local `Foo` followed by a
  // `<` less-than operator -- is left alone.
  if (template_arguments == NULL && symbol != NULL &&
      !symbol->flags.is_template && StorageIs(symbol->storage, STO(typedef)) &&
      symbol->type != NULL && TypeIsStructOrUnion(symbol->type) &&
      LexLookingAt(lex, TOK(less))) {
    Symbol* self_tag = CurrentClassSelfTagSymbol(syntax, &name);
    if (self_tag != NULL && self_tag->type != NULL &&
        TypeIsStructOrUnion(self_tag->type) &&
        self_tag->type->info.struct_info == symbol->type->info.struct_info) {
      Vector* args = SyntaxParseTemplateArgumentList(syntax, followers);
      if (args != NULL) {
        template_arguments = args;
        symbol = self_tag;
      }
    } else {
      Symbol* primary =
          CurrentClassSpecializationPrimary(&name, symbol->type->info.struct_info);
      if (primary != NULL) {
        symbol = primary;
      }
    }
  }
  if (template_arguments == NULL && symbol != NULL &&
      (symbol->flags.is_template ||
       IdentifierNamesFunctionTemplateOverload(symbol) ||
       SymbolExtraOf(symbol)->variable_template != NULL) &&
      LexLookingAt(lex, TOK(less))) {
    Vector* args = SyntaxParseTemplateArgumentList(syntax, followers);
    if (args != NULL) {
      if (TypeIsFunction(symbol->type) ||
          IdentifierNamesFunctionTemplateOverload(symbol)) {
        template_arguments = args;
        args = NULL;
      } else if (symbol->flags.is_concept) {
        template_arguments = args;
        args = NULL;
      } else if (symbol->flags.is_template && symbol->type != NULL &&
                 TypeIsStructOrUnion(symbol->type) &&
                 symbol->type->template_arguments == NULL &&
                 symbol->type->info.struct_info != NULL &&
                 symbol->type->info.struct_info->is_template) {
        // A class template-id used as an expression, e.g. the functional cast
        // `A<int>(...)`.  Retain the explicit arguments on the identifier node;
        // functional-construction analysis instantiates the specialization from
        // them (and the template-body cloner substitutes them first when the
        // construction appears inside another template).  Alias templates are
        // excluded here: their type carries the aliased `<...>` arguments, so
        // they still route through the placeholder/CTAD path.
        template_arguments = args;
        args = NULL;
      } else if (SymbolExtraOf(symbol)->variable_template != NULL) {
        // A variable template-id used as a value, e.g. `variant_size_v<T>`.
        // Retain the explicit arguments; the identifier is instantiated and
        // constant-folded during semantic analysis.
        template_arguments = args;
        args = NULL;
      } else if (symbol->flags.is_template &&
                 StorageIs(symbol->storage, STO(typedef))) {
        // Alias template-id used as an expression, e.g.
        // `make_index_sequence<N>()`.  Preserve its written arguments so a
        // dependent N can be substituted before functional construction
        // instantiates the alias.
        template_arguments = args;
        args = NULL;
      }
      if (args != NULL) {
        VectorDestructWithContents(args,
                                   (VectorElementDestructor)TemplateArgumentDelete,
                                   /*free_element=*/false);
      }
    }
  }
  bool is_qualified_name = name.is_qualified;
  Struct* functional_constructor_owner =
      symbol != NULL && symbol->type != NULL &&
              TypeIsFunction(symbol->type) &&
              symbol->type->info.function.is_constructor
          ? symbol->type->info.function.cxx_member_owner
          : NULL;
  bool is_alias_template = symbol != NULL && symbol->flags.is_template &&
                            StorageIs(symbol->storage, STO(typedef));
  bool is_functional_class_construction =
      CompilerIsCXX() && symbol != NULL && symbol->type != NULL &&
      ((TypeIsStructOrUnion(symbol->type) &&
        (StorageIs(symbol->storage, STO(typedef)) ||
         (symbol->flags.is_template &&
          SymbolExtraOf(symbol)->variable_template == NULL))) ||
       is_alias_template || functional_constructor_owner != NULL) &&
      LexLookingAt(lex, TOK(lparen));
  if (template_arguments == NULL && symbol != NULL &&
      SymbolExtraOf(symbol)->variable_template != NULL &&
      name.template_arguments.length > 0) {
    Vector* parsed_args =
        name.template_arguments.value.p[name.template_arguments.length - 1];
    if (parsed_args != NULL && parsed_args->length > 0) {
      template_arguments = TemplateArgumentVectorCopy(parsed_args);
    }
  }
  if (CompilerIsCXX() && is_qualified_name && template_arguments != NULL &&
      symbol != NULL && SymbolExtraOf(symbol)->variable_template != NULL &&
      name.components.length >= 2) {
    Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
        syntax, &name, name.components.length - 1);
    if (owner != NULL && owner->type != NULL &&
        TypeIsStructOrUnion(owner->type) &&
        owner->type->info.struct_info != NULL) {
      String member_name;
      StringInit(&member_name, FullyQualifiedIdentifierLast(&name));
      StructMember* member =
          FindStructMember(owner->type->info.struct_info, &member_name);
      StringDestruct(&member_name);
      if (member != NULL && member->is_static && member->symbol != NULL) {
        ASTNode* left =
            NewIdentifierASTNode(owner, lex->current_token_location);
        left->flags |= kASTQualifiedName;
        ASTNode* right =
            NewStructMemberASTNode(member, lex->current_token_location);
        right->flags |= kASTQualifiedName;
        ((StructMemberASTNode*)right)->template_arguments =
            TemplateArgumentVectorCopy(template_arguments);
        FullyQualifiedIdentifierDestruct(&name);
        return NewBinaryASTNode(AST_OP(dot), NULL,
                                lex->current_token_location, left, right);
      }
    }
  }
  FullyQualifiedIdentifierDestruct(&name);
  ASTNode* node = NewIdentifierASTNode(symbol, identifier_location);
  if (symbol->flags.name_independent_lookup_ambiguous) {
    node->flags |= kASTNameIndependentLookupAmbiguous;
  }
  if (is_qualified_name) {
    node->flags |= kASTQualifiedName;
  }
  if (is_functional_class_construction) {
    node->flags |= kASTCXXFunctionalConstruction;
    if (functional_constructor_owner != NULL) {
      TypeRecord* owner_type = NewTypeRecord(
          functional_constructor_owner->is_union ? kTypeUnion : kTypeStruct,
          kQualPlain);
      TypeRecordSetStructInfo(owner_type, functional_constructor_owner);
      TypeRecordCalculateSize(owner_type);
      ASTNodeSetType(node, owner_type);
    }
  }
  ((IdentifierASTNode*)node)->template_arguments = template_arguments;
  CXXPackExpressionSearch pack_search = {.syntax = syntax, .found = false};
  for (size_t i = 0;
       template_arguments != NULL && i < template_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            &pack_search, template_arguments->value.p[i])) {
      node->flags |= kASTReferencesParameterPack;
      break;
    }
  }
  return node;
}


// Reports whether an identifier expression names a template type parameter.
bool CXXExpressionNamesTemplateTypeParameter(ASTNode* node) {
  if (!CompilerIsCXX() || node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  return id->symbol != NULL && id->symbol->flags.is_template_parameter &&
         id->symbol->flags.is_template_type_parameter &&
         id->symbol->type != NULL;
}


// Parses T() for a dependent type parameter as value-initialization of that
// dependent type.
ASTNode* ParseCXXDependentValueInitialization(ASTNode* type_expr,
                                                     Syntax* syntax,
                                                     TokenClass followers) {
  // `T(args)` where T names a template type parameter is a dependent explicit
  // type conversion / value-initialization.  Represent it as an ordinary
  // functional-construction `call` node with the type-parameter identifier as
  // the callee (exactly as a non-dependent `Foo(args)` is represented).  Once
  // the enclosing template is instantiated and T is concrete,
  // AnalyzeCXXFunctionalClassConstruction resolves it correctly: a scalar T
  // yields a functional cast (`T()` -> value 0), while a class T is
  // constructed or value-initialized through its constructors.  (Previously
  // this produced a hardcoded `static_cast<T>(0)`, which was only valid when T
  // turned out to be scalar and made value-initialization of a class T --
  // e.g. an allocator default argument `Allocator()` -- an illegal cast.)
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, TOK(rparen))) {
    ASTNode* actual = LexMatch(syntax->lex, TOK(lbrace))
                          ? SyntaxParseBracedInitializer(syntax)
                          : SyntaxParseSingleExpression(
                                syntax, followers | TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  return NewVectorASTNode(AST_OP(call), NULL,
                          syntax->lex->current_token_location, type_expr,
                          actuals);
}
