//
//  syntax_template_decl.c
//  c_compiler
//
//  Template declarations: attaching parameter lists and constraints to the
//  declared entity, and explicit instantiations.
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

// True when any record on `type`'s declarator spine is a class or union
// template specialization.
static bool TypeContainsClassTemplate(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->info.struct_info != NULL &&
        t->info.struct_info->is_template) {
      return true;
    }
  }
  return false;
}

// True when a declaration names an object of class-template type that still
// needs a concrete specialization.  Function types and pointer/reference
// wrappers do not: C++ allows incomplete class types there, including the
// deferred `using ostream = basic_ostream<char>` alias from <iosfwd>.
bool TypeIsClassTemplateObject(TypeRecord* type) {
  if (type == NULL || TypeIsFunction(type) || TypeIsPointer(type) ||
      TypeIsReference(type)) {
    return false;
  }
  if (TypeIsArray(type)) {
    return TypeIsClassTemplateObject(type->next);
  }
  return TypeContainsClassTemplate(type) &&
         !TypeIsClassTemplatePlaceholder(type);
}

// Returns the tag name of the innermost class template on `type`'s spine, or
// `"<anonymous>"` when no such tag name is available.
const char* ClassTemplateObjectName(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->info.struct_info != NULL &&
        t->info.struct_info->is_template &&
        t->info.struct_info->tag_name != NULL &&
        t->info.struct_info->tag_name->value != NULL) {
      return t->info.struct_info->tag_name->value;
    }
  }
  return "<anonymous>";
}

// A type alias formed while its class template was only forward-declared carries
// a *deferred* template-id (see the forward-declaration path in
// InstantiateSimpleClassTemplateImpl): the primary's members were not yet known,
// so the specialization was recorded but not materialized.  A variable
// declaration requires a complete type, so materialize the specialization now
// that the definition is available, instead of rejecting the still-"template"
// type in the guards that follow.
void MaterializeDeferredClassTemplateType(Syntax* syntax, Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      syntax->parsing_template_declaration ||
      !TypeContainsClassTemplate(sym->type) ||
      TypeIsClassTemplatePlaceholder(sym->type)) {
    return;
  }
  TypeRecord* materialized =
      TypeMaterializeClassTemplateSpecialization(syntax, sym->type);
  if (materialized != sym->type) {
    // SymbolSetType retains the record.  Dropping it here frees the function
    // type the symbol now points at, which clears a just-copied parameter list
    // (`ostream& operator<<(ostream&, int128)`).
    SymbolSetType(sym, materialized);
  }
}

// Returns the number of template parameters in the current `template<>` head
// being parsed, from the parameter vector or the saved count.
int CurrentTemplateParameterListLength(Syntax* syntax) {
  return syntax->current_template_parameters != NULL
             ? (int)syntax->current_template_parameters->length
             : syntax->current_template_parameter_count;
}

// Returns the index assigned to the first parameter in the current template
// head within the nested template-parameter scope stack.
int CurrentTemplateParameterBase(Syntax* syntax) {
  return syntax->current_template_parameter_count -
         CurrentTemplateParameterListLength(syntax);
}

// Transfers ownership of `syntax->current_template_parameters` into `func`'s
// template parameter list and clears the syntax vector length.
void MoveCurrentTemplateParametersToFunction(Syntax* syntax,
                                             TypeRecord* func) {
  if (syntax->current_template_parameters == NULL || func == NULL ||
      !TypeIsFunction(func)) {
    return;
  }
  if (func->info.function.symbol != NULL &&
      func->info.function.symbol->is_imported_module_symbol &&
      func->info.function.template_parameters.length > 0) {
    return;
  }
  VectorDestructWithContents(&func->info.function.template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&func->info.function.template_parameters);
  for (size_t i = 0; i < syntax->current_template_parameters->length; i++) {
    VectorAppend(&func->info.function.template_parameters,
                 syntax->current_template_parameters->value.p[i]);
  }
  syntax->current_template_parameters->length = 0;
}

// Replaces `str`'s template parameters with the current head, preserving
// default arguments from a prior declaration when the new list omits them.
static void MoveCurrentTemplateParametersToStruct(Syntax* syntax, Struct* str) {
  if (syntax->current_template_parameters == NULL || str == NULL) {
    return;
  }
  Vector* incoming = syntax->current_template_parameters;
  // A class template's default template arguments accumulate across its
  // declarations ([temp.param]): a forward declaration may supply a default
  // that the later definition omits.  The definition re-parses the parameter
  // list without those defaults, so carry any default that the incoming list is
  // missing over from the prior parameter list before it is discarded.
  // Ownership of a transferred default_type moves to the incoming parameter --
  // the old parameter's pointer is cleared so the TemplateParameterDelete below
  // does not free the type now owned by the surviving parameter.
  for (size_t i = 0;
       i < incoming->length && i < str->template_parameters.length; i++) {
    TemplateParameter* prev = str->template_parameters.value.p[i];
    TemplateParameter* next = incoming->value.p[i];
    if (prev == NULL || next == NULL || prev->kind != next->kind) {
      continue;
    }
    bool next_has_default = next->default_type != NULL ||
                            next->default_argument != NULL ||
                            next->has_default_int ||
                            next->default_template_parameter_index >= 0;
    if (next_has_default) {
      continue;
    }
    if (prev->default_type != NULL) {
      next->default_type = prev->default_type;
      prev->default_type = NULL;
    }
    next->has_default_int = prev->has_default_int;
    next->default_int_value = prev->default_int_value;
    next->default_template_parameter_index =
        prev->default_template_parameter_index;
    next->default_argument = prev->default_argument;
    prev->default_argument = NULL;
  }
  VectorDestructWithContents(&str->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&str->template_parameters);
  for (size_t i = 0; i < incoming->length; i++) {
    VectorAppend(&str->template_parameters, incoming->value.p[i]);
  }
  incoming->length = 0;
}

// Conjoins `constraint` with any existing associated constraint on `func`;
// takes ownership of `constraint` when it becomes the sole constraint.
void AddFunctionAssociatedConstraint(TypeRecord* func,
                                     ConstraintExpr* constraint) {
  if (func == NULL || !TypeIsFunction(func) || constraint == NULL) {
    return;
  }
  ConstraintExpr* current = func->info.function.associated_constraint;
  if (current == NULL) {
    func->info.function.associated_constraint = constraint;
    return;
  }
  func->info.function.associated_constraint =
      NewConjunctionConstraint(current, constraint, constraint->location);
}

// Stores `constraint` in `*target`, conjoining with the prior constraint and
// taking ownership of the incoming expression.
void AddOwnedAssociatedConstraint(ConstraintExpr** target,
                                  ConstraintExpr* constraint) {
  if (target == NULL || constraint == NULL) {
    return;
  }
  ConstraintExpr* current = *target;
  if (current == NULL) {
    *target = constraint;
    return;
  }
  *target = NewConjunctionConstraint(current, constraint, constraint->location);
}

// Moves each template parameter's `associated_constraint` into `*target` and
// clears the parameter pointers so they are not freed twice.
void MoveTemplateParameterConstraints(Vector* parameters,
                                      ConstraintExpr** target) {
  if (parameters == NULL || target == NULL) {
    return;
  }
  for (size_t i = 0; i < parameters->length; i++) {
    TemplateParameter* param = parameters->value.p[i];
    if (param == NULL || param->associated_constraint == NULL) {
      continue;
    }
    ConstraintExpr* constraint = param->associated_constraint;
    param->associated_constraint = NULL;
    AddOwnedAssociatedConstraint(target, constraint);
  }
}

// Hoists per-parameter constraints from `func`'s template parameter list onto
// the function's associated constraint.
static void MoveTemplateParameterConstraintsToFunction(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return;
  }
  MoveTemplateParameterConstraints(&func->info.function.template_parameters,
                                   &func->info.function.associated_constraint);
}

// Hoists per-parameter constraints from `str`'s template parameter list onto
// the class template's associated constraint.
static void MoveTemplateParameterConstraintsToStruct(Struct* str) {
  if (str == NULL) {
    return;
  }
  MoveTemplateParameterConstraints(&str->template_parameters,
                                   &str->associated_constraint);
}

// Hoists per-parameter constraints from `vt`'s parameter list onto the
// variable template's associated constraint.
static void MoveTemplateParameterConstraintsToVariableTemplate(
    VariableTemplate* vt) {
  if (vt == NULL) {
    return;
  }
  MoveTemplateParameterConstraints(&vt->parameters, &vt->associated_constraint);
}

// Emits a diagnostic when `symbol` names a user-defined literal operator
// template that is not the required `template<char...>` form.
static void ValidateLiteralOperatorTemplate(Syntax* syntax, Symbol* symbol) {
  if (symbol == NULL || !StringStartsWith(&symbol->name, "operator\"\"")) {
    return;
  }
  if (!SyntaxIsCXXNumericLiteralOperatorTemplate(symbol)) {
    SyntaxError(
        syntax,
        "Numeric literal operator template must have the form "
        "template<char...> operator\"\"suffix()");
  }
}

// Marks entities in a parsed `template` declaration as templates and attaches
// the current parameter list, constraints, and variable-template state.
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
        bool already_template = var->symbol->flags.is_template;
        var->symbol->flags.is_template = true;
        if (var->symbol->type != NULL && TypeIsFunction(var->symbol->type)) {
          if (!already_template) {
            var->symbol->type->info.function.template_parameter_count =
                CurrentTemplateParameterListLength(syntax);
            var->symbol->type->info.function.template_parameter_base =
                CurrentTemplateParameterBase(syntax);
          }
          if (var->symbol->type->info.function.template_parameters.length == 0) {
            MoveCurrentTemplateParametersToFunction(syntax, var->symbol->type);
          }
          MoveTemplateParameterConstraintsToFunction(var->symbol->type);
          ValidateLiteralOperatorTemplate(syntax, var->symbol);
          if (syntax->current_template_requires_clause != NULL) {
            AddFunctionAssociatedConstraint(
                var->symbol->type, syntax->current_template_requires_clause);
            syntax->current_template_requires_clause = NULL;
          }
        } else if (var->symbol->variable_template == NULL) {
          // A C++ variable template: capture its (unanalyzed) initializer and
          // template parameters for per-use instantiation.  The initializer is
          // transferred off the declaration node so the template itself emits
          // no definition; only concrete instantiations produce values.
          VariableTemplate* vt = malloc(sizeof(VariableTemplate));
          vt->initializer = var->initializer;
          var->initializer = NULL;
          vt->associated_constraint = NULL;
          VectorInit(&vt->parameters);
          VectorInit(&vt->partial_specializations);
          if (syntax->current_template_parameters != NULL) {
            for (size_t p = 0;
                 p < syntax->current_template_parameters->length; p++) {
              VectorAppend(&vt->parameters,
                           syntax->current_template_parameters->value.p[p]);
            }
            syntax->current_template_parameters->length = 0;
          }
          MoveTemplateParameterConstraintsToVariableTemplate(vt);
          if (syntax->current_template_requires_clause != NULL) {
            AddOwnedAssociatedConstraint(&vt->associated_constraint,
                                         syntax->current_template_requires_clause);
            syntax->current_template_requires_clause = NULL;
          }
          var->symbol->variable_template = vt;
        }
        marked_symbol = true;
      }
    }
  }
  if (!marked_symbol && syntax->last_parsed_tag != NULL &&
      syntax->last_parsed_tag->type != NULL &&
      TypeIsStructOrUnion(syntax->last_parsed_tag->type) &&
      syntax->last_parsed_tag->type->info.struct_info != NULL) {
    for (size_t i = 0; syntax->current_template_parameters != NULL &&
                       i < syntax->current_template_parameters->length; i++) {
      TemplateParameter* param = syntax->current_template_parameters->value.p[i];
      if (param != NULL && param->is_parameter_pack &&
          i + 1 < syntax->current_template_parameters->length) {
        SyntaxError(syntax, "Template parameter pack must be last");
        break;
      }
    }
    bool already_class_template =
        syntax->last_parsed_tag->flags.is_template;
    syntax->last_parsed_tag->flags.is_template = true;
    syntax->last_parsed_tag->type->info.struct_info->is_template = true;
    syntax->last_parsed_tag->type->info.struct_info->template_parameter_count =
        CurrentTemplateParameterListLength(syntax);
    MoveCurrentTemplateParametersToStruct(
        syntax, syntax->last_parsed_tag->type->info.struct_info);
    Struct* class_template = syntax->last_parsed_tag->type->info.struct_info;
    MoveTemplateParameterConstraintsToStruct(class_template);
    if (syntax->current_template_requires_clause != NULL) {
      AddOwnedAssociatedConstraint(&class_template->associated_constraint,
                                   syntax->current_template_requires_clause);
      syntax->current_template_requires_clause = NULL;
    }
    if (class_template->lexical_parent == NULL ||
        !already_class_template) {
      Symbol* alias = NewSymbol(syntax->last_parsed_tag->name.value,
                                syntax->last_parsed_tag->type, STO(typedef));
      alias->namespace_ = syntax->last_parsed_tag->namespace_;
      alias->flags.is_template = true;
      if (!SyntaxAddSymbol(syntax, alias)) {
        SymbolDelete(alias);
        Symbol* existing_alias =
            SyntaxFindSymbol(syntax, &syntax->last_parsed_tag->name);
        if (existing_alias != NULL &&
            StorageIs(existing_alias->storage, STO(typedef))) {
          existing_alias->flags.is_template = true;
        }
      }
    } else {
      Symbol* existing_alias =
          SyntaxFindTopScopeSymbol(syntax, &syntax->last_parsed_tag->name);
      if (existing_alias != NULL &&
          StorageIs(existing_alias->storage, STO(typedef)) &&
          existing_alias->type != NULL &&
          TypeIsStructOrUnion(existing_alias->type) &&
          existing_alias->type->info.struct_info == class_template) {
        existing_alias->flags.is_template = true;
      }
    }
    TypeEnsureCXXDeductionGuides(syntax->last_parsed_tag);
  }
}

// Parses an `extern template` declaration by consuming tokens until the
// top-level semicolon and returns an empty declaration list node.
ASTNode* SyntaxParseExplicitInstantiationDeclaration(Syntax* syntax,
                                                     SourceLocation location) {
  // Function/variable explicit instantiation, including qualified members:
  // `extern template bool C::Dispatch<int>(Data, ...);`
  // Consume the declaration; `extern template` does not instantiate.
  int paren = 0;
  int brace = 0;
  int square = 0;
  for (;;) {
    if (paren == 0 && brace == 0 && square == 0 &&
        LexLookingAt(syntax->lex, TOK(semicolon))) {
      break;
    }
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      paren++;
    } else if (LexLookingAt(syntax->lex, TOK(rparen)) && paren > 0) {
      paren--;
    } else if (LexLookingAt(syntax->lex, TOK(lbrace))) {
      brace++;
    } else if (LexLookingAt(syntax->lex, TOK(rbrace)) && brace > 0) {
      brace--;
    } else if (LexLookingAt(syntax->lex, TOK(lsquare))) {
      square++;
    } else if (LexLookingAt(syntax->lex, TOK(rsquare)) && square > 0) {
      square--;
    }
    Token before = syntax->lex->current_token;
    SourceLocation before_loc = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    if (syntax->lex->current_token == before &&
        syntax->lex->current_token_location == before_loc) {
      // A finished token-replay looks like EOF/no-progress.  Drop back to
      // the enclosing source so `)` / `;` after a macro replacement are seen.
      if (LexIsTokenReplaying(syntax->lex)) {
        LexEndTokenReplay(syntax->lex);
        LexNextToken(syntax->lex);
        if (syntax->lex->current_token != before ||
            syntax->lex->current_token_location != before_loc) {
          continue;
        }
      }
      break;
    }
  }
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

// An explicit instantiation definition of a class template specialization
// defines every member function it has a definition for.
static void DefineExplicitClassInstantiationMembers(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->is_template) {
    return;
  }
  Struct* str = type->info.struct_info;
  for (size_t i = 0; i < str->members.length; i++) {
    for (StructMember* member = str->members.value.p[i]; member != NULL;
         member = member->overload_next) {
      Symbol* function = member->symbol;
      if (function == NULL || function->flags.is_template ||
          function->type == NULL || !TypeIsFunction(function->type) ||
          function->type->info.function.is_deleted ||
          function->type->info.function.is_implicitly_declared) {
        continue;
      }
      CompilerMarkFunctionReferenced(function);
    }
  }
}

// Parses `template` followed by an explicit instantiation: a class
// specialization (instantiating members when not `extern`) or a function or
// variable declaration consumed as an external declaration.
ASTNode* ParseExplicitTemplateInstantiation(Syntax* syntax,
                                            SourceLocation location,
                                            bool is_extern) {
  if (!(LexLookingAt(syntax->lex, TOK(struct)) ||
        LexLookingAt(syntax->lex, TOK(class)) ||
        LexLookingAt(syntax->lex, TOK(union)))) {
    if (is_extern) {
      return SyntaxParseExplicitInstantiationDeclaration(syntax, location);
    }
    bool saved = syntax->parsing_explicit_instantiation;
    syntax->parsing_explicit_instantiation = true;
    ASTNode* declaration = SyntaxParseExternalDeclaration(syntax);
    syntax->parsing_explicit_instantiation = saved;
    return declaration != NULL ? declaration : EmptyDeclarationList(location);
  }
  LexNextToken(syntax->lex);

  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  if (!SyntaxParseFullyQualifiedIdentifier(syntax, &name)) {
    SyntaxError(syntax, "Expected class template name");
    FullyQualifiedIdentifierDestruct(&name);
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

  Symbol* templ = SyntaxFindQualifiedSymbol(syntax, &name);
  if (templ == NULL || !templ->flags.is_template ||
      templ->type == NULL || !TypeIsStructOrUnion(templ->type)) {
    SyntaxError(syntax, "%s is not a class template", name.spelling.value);
    FullyQualifiedIdentifierDestruct(&name);
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

  Vector* args = SyntaxParseTemplateArgumentList(syntax, TC(decl));
  if (args == NULL) {
    SyntaxError(syntax, "Expected template argument list");
  } else {
    TypeRecord* type = TypeInstantiateClassTemplate(syntax, templ, args);
    if (!is_extern) {
      DefineExplicitClassInstantiationMembers(type);
    }
    TypeRecordDelete(type);
    VectorDestructWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
  }
  FullyQualifiedIdentifierDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

// Introduces a scope symbol for a cloned template parameter so the spliced
// or nested head can refer to it during parsing.
static void SyntaxRegisterClonedTemplateParameter(Syntax* syntax,
                                                  TemplateParameter* param) {
  if (param == NULL || param->name.length == 0) {
    return;
  }
  if (param->kind == kTemplateParameterType) {
    TypeRecord* placeholder =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    placeholder->template_parameter_index = param->index;
    placeholder->template_parameter_name = NewString(param->name.value);
    Symbol* sym = NewSymbol(param->name.value, placeholder, STO(typedef));
    sym->flags.invented = true;
    sym->flags.is_template_parameter = true;
    sym->flags.is_template_type_parameter = true;
    sym->flags.is_parameter_pack = param->is_parameter_pack;
    sym->template_parameter_index = param->index;
    SyntaxAddSymbol(syntax, sym);
    return;
  }
  if (param->kind == kTemplateParameterNonType && param->type != NULL) {
    Symbol* sym =
        NewSymbol(param->name.value, TypeRecordCopy(param->type), STO(implicit));
    sym->flags.invented = true;
    sym->flags.is_template_parameter = true;
    sym->flags.is_parameter_pack = param->is_parameter_pack;
    sym->template_parameter_index = param->index;
    SyntaxAddSymbol(syntax, sym);
  }
}

// Returns a new vector of template parameters copied from class or function
// template `templ`, re-indexed from `base`; emits an error when `templ` is not
// a template. Caller owns the returned vector.
static Vector* SyntaxCloneTemplateParametersFromSymbol(Syntax* syntax,
                                                       Symbol* templ,
                                                       int base) {
  Vector* params = NewVector();
  Vector* source = NULL;
  if (templ != NULL && templ->type != NULL && TypeIsStructOrUnion(templ->type) &&
      templ->type->info.struct_info != NULL) {
    source = &templ->type->info.struct_info->template_parameters;
  } else if (templ != NULL && templ->type != NULL &&
             TypeIsFunction(templ->type)) {
    source = &templ->type->info.function.template_parameters;
  }
  if (source == NULL) {
    SyntaxError(syntax, "Template splice operand does not reflect a template");
    return params;
  }
  for (size_t i = 0; i < source->length; i++) {
    TemplateParameter* old_param = source->value.p[i];
    if (old_param == NULL) {
      continue;
    }
    TemplateParameter* copy = NewTemplateParameter(
        old_param->name.value, old_param->kind, old_param->is_parameter_pack,
        old_param->type, old_param->default_type, old_param->has_default_int,
        old_param->default_int_value,
        old_param->default_template_parameter_index, base + (int)i);
    VectorAppend(params, copy);
    SyntaxRegisterClonedTemplateParameter(syntax, copy);
  }
  return params;
}

// Parses a C++26 template-head splice (`template[:]` reflection `]`) and
// returns cloned parameters from the reflected template; caller owns the
// vector. Consumes tokens through the splice close bracket.
static Vector* SyntaxParseTemplateHeadSpliceParameterList(Syntax* syntax,
                                                          int base) {
  LexNextToken(syntax->lex);
  ASTNode* reflection =
      SyntaxParseExpression(syntax, TC(spliceclose));
  SyntaxNeedBracket(syntax, TOK(splice_close), TC(decl));
  Symbol* templ = NULL;
  if (reflection != NULL && reflection->op == AST_OP(reflect)) {
    ReflectionASTNode* reflect = (ReflectionASTNode*)reflection;
    if (reflect->operand_kind == kReflectionOperandExpression &&
        reflect->operand != NULL &&
        reflect->operand->op == AST_OP(identifier)) {
      templ = ((IdentifierASTNode*)reflect->operand)->symbol;
    }
  }
  ASTNodeDelete(reflection);
  if (templ == NULL || !templ->flags.is_template) {
    SyntaxError(syntax, "Template splice operand does not reflect a template");
    return NewVector();
  }
  return SyntaxCloneTemplateParametersFromSymbol(syntax, templ, base);
}

// Parses a `template` declaration: explicit instantiation, head splice,
// specialization (`template<>`), or a normal template head followed by the
// member or external declaration. Consumes leading `template` token.
ASTNode* ParseTemplateDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // template

  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      LexLookingAt(syntax->lex, TOK(splice_open))) {
    SyntaxOpenScope(syntax);
    LocalSymbolTable* template_tag_scope = syntax->local_tag_stack;
    syntax->local_tag_stack = template_tag_scope->prev;
    int old_template_parameter_count = syntax->current_template_parameter_count;
    Vector* old_template_parameters = syntax->current_template_parameters;
    ConstraintExpr* old_requires_clause =
        syntax->current_template_requires_clause;
    syntax->current_template_parameters =
        SyntaxParseTemplateHeadSpliceParameterList(
            syntax, old_template_parameter_count);
    syntax->current_template_parameter_count =
        old_template_parameter_count +
        (int)syntax->current_template_parameters->length;
    syntax->last_parsed_tag = NULL;
    bool old_parsing_template = syntax->parsing_template_declaration;
    bool old_parsing_specialization = syntax->parsing_template_specialization;
    syntax->parsing_template_declaration = true;
    syntax->parsing_template_specialization = false;
    syntax->current_template_requires_clause =
        ConceptsParseRequiresClause(syntax);
    ASTNode* declaration = ConceptsParseDefinition(syntax, location);
    if (declaration == NULL) {
      declaration = SyntaxParseExternalDeclaration(syntax);
    }
    syntax->parsing_template_declaration = old_parsing_template;
    syntax->parsing_template_specialization = old_parsing_specialization;
    syntax->local_tag_stack = template_tag_scope;
    SyntaxCloseScope(syntax);
    MarkTemplateDeclaration(syntax, declaration);
    VectorDestructWithContents(syntax->current_template_parameters,
                               (VectorElementDestructor)TemplateParameterDelete,
                               /*free_element=*/false);
    ConstraintExprDelete(syntax->current_template_requires_clause);
    syntax->current_template_parameter_count = old_template_parameter_count;
    syntax->current_template_parameters = old_template_parameters;
    syntax->current_template_requires_clause = old_requires_clause;
    return declaration != NULL ? declaration : EmptyDeclarationList(location);
  }

  if (!LexLookingAt(syntax->lex, TOK(less))) {
    return ParseExplicitTemplateInstantiation(syntax, location,
                                              /*is_extern=*/false);
  }

  SyntaxOpenScope(syntax);
  LocalSymbolTable* template_tag_scope = syntax->local_tag_stack;
  syntax->local_tag_stack = template_tag_scope->prev;
  int old_template_parameter_count = syntax->current_template_parameter_count;
  Vector* old_template_parameters = syntax->current_template_parameters;
  ConstraintExpr* old_requires_clause =
      syntax->current_template_requires_clause;
  syntax->current_template_parameters =
      SyntaxParseTemplateParameterListWithBase(
          syntax, old_template_parameter_count);
  syntax->current_template_parameter_count =
      old_template_parameter_count +
      (int)syntax->current_template_parameters->length;
  syntax->last_parsed_tag = NULL;
  bool old_parsing_template = syntax->parsing_template_declaration;
  bool old_parsing_specialization = syntax->parsing_template_specialization;
  bool is_specialization = syntax->current_template_parameters->length == 0;
  syntax->parsing_template_declaration = !is_specialization;
  syntax->parsing_template_specialization = is_specialization;
  if (is_specialization && LexLookingAt(syntax->lex, TOK(concept))) {
    SyntaxError(syntax, "concept specialization is not permitted");
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    syntax->parsing_template_declaration = old_parsing_template;
    syntax->parsing_template_specialization = old_parsing_specialization;
    syntax->local_tag_stack = template_tag_scope;
    SyntaxCloseScope(syntax);
    syntax->current_template_parameter_count = old_template_parameter_count;
    syntax->current_template_parameters = old_template_parameters;
    syntax->current_template_requires_clause = old_requires_clause;
    return EmptyDeclarationList(location);
  }
  syntax->current_template_requires_clause =
      ConceptsParseRequiresClause(syntax);
  ASTNode* declaration = ConceptsParseDefinition(syntax, location);
  if (declaration == NULL) {
    declaration = SyntaxParseExternalDeclaration(syntax);
  }
  syntax->parsing_template_declaration = old_parsing_template;
  syntax->parsing_template_specialization = old_parsing_specialization;
  syntax->local_tag_stack = template_tag_scope;
  SyntaxCloseScope(syntax);
  if (!is_specialization) {
    MarkTemplateDeclaration(syntax, declaration);
  }
  VectorDestructWithContents(syntax->current_template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  ConstraintExprDelete(syntax->current_template_requires_clause);
  syntax->current_template_parameter_count = old_template_parameter_count;
  syntax->current_template_parameters = old_template_parameters;
  syntax->current_template_requires_clause = old_requires_clause;
  return declaration != NULL ? declaration : EmptyDeclarationList(location);
}
