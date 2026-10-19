//
//  syntax_template_param.c
//  c_compiler
//
//  Template parameter lists: type, non-type, template template and
//  constrained parameters with their defaults.
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

// Allocates a new `TemplateParameter` with the given kind, type defaults,
// and index.  Takes references to `type` and `default_type` when non-NULL.
TemplateParameter* NewTemplateParameter(const char* name,
                                        TemplateParameterKind kind,
                                        bool is_parameter_pack,
                                        TypeRecord* type,
                                        TypeRecord* default_type,
                                        bool has_default_int,
                                        long long default_int_value,
                                        int default_template_parameter_index,
                                        int index) {
  TemplateParameter* param = malloc(sizeof(TemplateParameter));
  StringInit(&param->name, name);
  param->kind = kind;
  param->is_parameter_pack = is_parameter_pack;
  param->type = type;
  param->default_type = default_type;
  param->has_default_int = has_default_int;
  param->default_int_value = default_int_value;
  param->default_template_parameter_index =
      default_template_parameter_index;
  param->associated_constraint = NULL;
  param->default_argument = NULL;
  param->template_parameters = NULL;
  param->template_template_kind = kTemplateTemplateParameterType;
  if (type != NULL) {
    TypeRecordIncRef(type);
  }
  if (default_type != NULL) {
    TypeRecordIncRef(default_type);
  }
  param->index = index;
  return param;
}

// Parses and constant-evaluates a default value for a non-type template
// parameter after `=`.  Returns a new `TemplateArgument`, or NULL after
// diagnosing when the expression is not a valid default.
static TemplateArgument* ParseTemplateNonTypeDefault(Syntax* syntax) {
  bool old_parsing_template_argument = syntax->parsing_template_argument;
  syntax->parsing_template_argument = true;
  ASTNode* expr = SyntaxParseSingleExpression(syntax,
                                              TC(closebra) | TC(exprsep));
  syntax->parsing_template_argument = old_parsing_template_argument;
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterNonType;
  arg->template_parameter_index = -1;
  arg->location = expr != NULL ? expr->location : SOURCE_LOCATION_MISSING;
  int direct_parameter_index = -1;
  bool is_direct_parameter =
      ExpressionIsNonTypeTemplateParameter(expr, &direct_parameter_index);
  if (!is_direct_parameter &&
      ExpressionContainsDependentTemplateParameter(expr)) {
    arg->dependent_expr = expr;
    return arg;
  }
  compiler->constant_evaluation_required_depth++;
  expr = AnalyzeExpression(expr);
  compiler->constant_evaluation_required_depth--;
  bool ok = TemplateArgumentSetFromExpression(arg, expr);
  if (!ok && is_direct_parameter) {
    arg->template_parameter_index = direct_parameter_index;
    arg->value_kind = kTemplateValueNone;
    TypeRecordDelete(arg->type);
    arg->type = expr != NULL ? TypeRecordCopy(expr->type) : NULL;
    ok = true;
  }
  if (!ok) {
    SyntaxError(syntax, "Template non-type default must be a constant expression");
    TemplateArgumentDelete(arg);
    arg = NULL;
  }
  ASTNodeDelete(expr);
  return arg;
}

// Parses a type-id used as a template type parameter default after `=`.
// Returns a type with reference count suitable for the caller to release.
static TypeRecord* ParseTemplateTypeDefault(Syntax* syntax) {
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* sym = TypeParserParseDeclarator(&parser, type);
  TypeParserDestruct(&parser);
  if (sym != NULL) {
    TypeRecord* result = TypeRecordCopy(sym->type);
    SymbolDelete(sym);
    return result;
  }
  return type;
}

// Builds a template template argument from parsed type `type`, which must
// name a template.  Consumes `type`; returns NULL after diagnosing when it
// does not.
static TemplateArgument* TemplateTemplateArgumentFromType(
    Syntax* syntax, TypeRecord* type, SourceLocation location) {
  Symbol* origin = type != NULL ? type->template_origin : NULL;
  if (origin == NULL || !origin->flags.is_template) {
    SyntaxError(syntax, "Template argument must name a template");
    TypeRecordDelete(type);
    return NULL;
  }
  int parameter_index =
      origin->flags.is_template_template_parameter
          ? origin->template_parameter_index : -1;
  TemplateArgument* arg =
      NewTemplateTemplateArgument(origin, parameter_index);
  arg->location = location;
  TypeRecordDelete(type);
  return arg;
}

// Allocates a type template argument that refers to template parameter `index`
// with type `type` (copied).
static TemplateArgument* NewTemplateParameterTypeArgument(int index,
                                                         TypeRecord* type) {
  TemplateArgument* arg = TemplateArgumentAlloc();
  arg->kind = kTemplateParameterType;
  arg->is_pack_expansion = false;
  arg->type = TypeRecordCopy(type);
  arg->int_value = 0;
  arg->template_parameter_index = index;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

// Wraps constrained placeholder type `placeholder` as the type argument for
// template parameter `index` in a concept-id.
static TemplateArgument* NewConstrainedPlaceholderTypeArgument(
    int index, TypeRecord* placeholder) {
  TemplateArgument* constrained_arg =
      NewTemplateParameterTypeArgument(index, placeholder);
  if (constrained_arg->type != NULL) {
    constrained_arg->type->template_parameter_index = index;
  }
  return constrained_arg;
}

// Parses a type-constraint (`Concept` or `ns::Concept<Args...>`) at the start
// of a template parameter.  On success sets `*concept_symbol` and returns a
// copy of the explicit concept arguments (possibly empty).  Returns NULL,
// with the lexer restored, when the tokens do not name a concept.
static Vector* ParseTemplateParameterTypeConstraint(Syntax* syntax,
                                                    Symbol** concept_symbol) {
  Lex* lex = syntax->lex;
  LexCheckpoint start_checkpoint;
  LexCheckpointSave(lex, &start_checkpoint);

  FullyQualifiedIdentifier concept_id;
  FullyQualifiedIdentifierInit(&concept_id);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &concept_id,
                                                          TC(closebra))) {
    FullyQualifiedIdentifierDestruct(&concept_id);
    LexCheckpointRestore(lex, &start_checkpoint);
    LexCheckpointDestruct(&start_checkpoint);
    return NULL;
  }

  *concept_symbol = SyntaxFindQualifiedSymbol(syntax, &concept_id);
  if (*concept_symbol == NULL || !(*concept_symbol)->flags.is_concept) {
    FullyQualifiedIdentifierDestruct(&concept_id);
    LexCheckpointRestore(lex, &start_checkpoint);
    LexCheckpointDestruct(&start_checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&start_checkpoint);

  Vector* concept_arguments = NewVector();
  if (concept_id.template_arguments.length > 0) {
    Vector* last_args = concept_id.template_arguments.value.p
        [concept_id.template_arguments.length - 1];
    if (last_args != NULL) {
      for (size_t i = 0; i < last_args->length; i++) {
        TemplateArgument* copy =
            TemplateArgumentCopy(last_args->value.p[i]);
        if (copy != NULL) {
          VectorAppend(concept_arguments, copy);
        }
      }
    }
  }
  FullyQualifiedIdentifierDestruct(&concept_id);
  return concept_arguments;
}

// Checks that the name of a constrained template parameter follows.  When it
// does not, diagnoses, frees `concept_arguments`, skips to the end of the
// parameter and returns false.
static bool ExpectConstrainedTemplateParameterName(Syntax* syntax,
                                                   Vector* concept_arguments) {
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    return true;
  }
  SyntaxError(syntax, "Expected constrained template parameter name");
  VectorDeleteWithContents(concept_arguments,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
  SyntaxRecover(syntax, TC(closebra));
  return false;
}

// Declares the constrained template parameter named by the current
// identifier token in the current scope, with type `symbol_type`, as a type
// parameter or (for `Concept auto V`) a non-type parameter.
static void DeclareConstrainedTemplateParameterSymbol(Syntax* syntax,
                                                      TypeRecord* symbol_type,
                                                      bool is_type_parameter,
                                                      bool is_parameter_pack,
                                                      int index) {
  Lex* lex = syntax->lex;
  Symbol* param = NewSymbol(lex->spelling.value, symbol_type, STO(typedef));
  param->flags.invented = true;
  param->flags.is_template_parameter = true;
  param->flags.is_template_type_parameter = is_type_parameter;
  param->flags.is_parameter_pack = is_parameter_pack;
  param->template_parameter_index = index;
  param->location = lex->current_token_location;
  bool added = SyntaxAddSymbol(syntax, param);
  if (!added) {
    SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
    SymbolDelete(param);
  }
}

// Makes the parameter itself (`placeholder`, at `index`) the first argument
// of its concept-id: `Concept<Args...> T` constrains `Concept<T, Args...>`.
static void PrependConstrainedPlaceholderArgument(Vector* concept_arguments,
                                                  int index,
                                                  TypeRecord* placeholder) {
  TemplateArgument* constrained_arg =
      NewConstrainedPlaceholderTypeArgument(index, placeholder);
  if (concept_arguments->length == 0) {
    VectorAppend(concept_arguments, constrained_arg);
  } else {
    VectorInsertBefore(concept_arguments, 0, constrained_arg);
  }
}

// Parses the rest of a constrained non-type parameter `Concept auto V [= v]`
// (the `auto` has been consumed) and appends it to `params`.  Takes
// ownership of `concept_arguments`.
static void ParseConstrainedAutoTemplateParameter(
    Syntax* syntax, Vector* params, Symbol* concept_symbol,
    Vector* concept_arguments, SourceLocation constraint_location, int index,
    bool is_parameter_pack) {
  Lex* lex = syntax->lex;
  if (!ExpectConstrainedTemplateParameterName(syntax, concept_arguments)) {
    return;
  }

  TypeRecord* placeholder_type =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  placeholder_type->template_parameter_index = index;
  placeholder_type->template_parameter_name = NewString("auto");
  DeclareConstrainedTemplateParameterSymbol(
      syntax, TypeRecordCopy(placeholder_type), /*is_type_parameter=*/false,
      is_parameter_pack, index);

  String param_name;
  StringInit(&param_name, lex->spelling.value);
  LexNextToken(lex);
  PrependConstrainedPlaceholderArgument(concept_arguments, index,
                                        placeholder_type);
  TemplateArgument* default_argument = NULL;
  if (LexMatch(lex, TOK(equal))) {
    if (is_parameter_pack) {
      SyntaxError(syntax, "Template parameter pack cannot have a default");
    }
    default_argument = ParseTemplateNonTypeDefault(syntax);
  }
  TemplateParameter* template_param =
      NewTemplateParameter(param_name.value, kTemplateParameterNonType,
                           is_parameter_pack, placeholder_type, NULL,
                           default_argument != NULL,
                           default_argument != NULL
                               ? default_argument->int_value : 0,
                           default_argument != NULL
                               ? default_argument->template_parameter_index : -1,
                           index);
  template_param->default_argument = default_argument;
  template_param->associated_constraint =
      NewConceptIdConstraint(concept_symbol, concept_arguments,
                             constraint_location);
  VectorAppend(params, template_param);
  StringDestruct(&param_name);
}

// Parses the rest of a constrained type parameter `Concept T [= type]` and
// appends it to `params`.  Takes ownership of `concept_arguments`.
static void ParseConstrainedTypeTemplateParameter(
    Syntax* syntax, Vector* params, Symbol* concept_symbol,
    Vector* concept_arguments, SourceLocation constraint_location, int index,
    bool is_parameter_pack) {
  Lex* lex = syntax->lex;
  if (!ExpectConstrainedTemplateParameterName(syntax, concept_arguments)) {
    return;
  }

  TypeRecord* placeholder = TypeRecordAdopt(
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
  placeholder->template_parameter_index = index;
  placeholder->template_parameter_name = NewString(lex->spelling.value);
  DeclareConstrainedTemplateParameterSymbol(syntax, placeholder,
                                            /*is_type_parameter=*/true,
                                            is_parameter_pack, index);

  String param_name;
  StringInit(&param_name, lex->spelling.value);
  LexNextToken(lex);
  PrependConstrainedPlaceholderArgument(concept_arguments, index, placeholder);
  TypeRecord* default_type = NULL;
  if (LexMatch(lex, TOK(equal))) {
    if (is_parameter_pack) {
      SyntaxError(syntax, "Template parameter pack cannot have a default");
    }
    default_type = TypeRecordAdopt(ParseTemplateTypeDefault(syntax));
  }

  TemplateParameter* template_param =
      NewTemplateParameter(param_name.value, kTemplateParameterType,
                           is_parameter_pack, NULL, default_type, false, 0,
                           -1, index);
  template_param->associated_constraint =
      NewConceptIdConstraint(concept_symbol, concept_arguments,
                             constraint_location);
  VectorAppend(params, template_param);
  StringDestruct(&param_name);
  TypeRecordDelete(default_type);
  TypeRecordDelete(placeholder);
}

// Parses a C++20 constrained template parameter (`Concept T`,
// `Concept<Args...>... Ts`, `Concept auto V`), appending it to `params` with
// index `base + params->length` and its concept-id as the associated
// constraint.  Returns false, consuming nothing, when the parameter does not
// start with a concept name.
static bool ParseConstrainedTemplateTypeParameter(Syntax* syntax,
                                                 Vector* params, int base) {
  Lex* lex = syntax->lex;
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(lex, TOK(identifier))) {
    return false;
  }

  SourceLocation constraint_location = lex->current_token_location;
  Symbol* concept_symbol = NULL;
  Vector* concept_arguments =
      ParseTemplateParameterTypeConstraint(syntax, &concept_symbol);
  if (concept_arguments == NULL) {
    return false;
  }

  int index = base + (int)params->length;
  bool is_parameter_pack = LexMatch(lex, TOK(ellipsis));
  if (LexLookingAt(lex, TOK(auto))) {
    LexNextToken(lex);
    ParseConstrainedAutoTemplateParameter(syntax, params, concept_symbol,
                                          concept_arguments,
                                          constraint_location, index,
                                          is_parameter_pack);
  } else {
    ParseConstrainedTypeTemplateParameter(syntax, params, concept_symbol,
                                          concept_arguments,
                                          constraint_location, index,
                                          is_parameter_pack);
  }
  return true;
}

// Parses a template template parameter (`template <parameter-list> class C`,
// `auto V`, or `concept C`).
static bool ParseTemplateTemplateParameter(Syntax* syntax, Vector* params,
                                           int base) {
  Lex* lex = syntax->lex;
  int index = base + (int)params->length;
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);  // template

  if (!LexLookingAt(lex, TOK(less))) {
    SyntaxError(syntax, "Expected '<' in template template parameter");
    return false;
  }
  // Inner parameter names have their own scope and are only visible while
  // parsing their parameter list.
  SyntaxOpenScope(syntax);
  Vector* inner_parameters =
      SyntaxParseTemplateParameterListWithBase(syntax, 0);
  SyntaxCloseScope(syntax);
  for (size_t i = 0; i < inner_parameters->length; i++) {
    TemplateParameter* inner = inner_parameters->value.p[i];
    if (inner != NULL &&
        ConceptsConstraintReferencesConceptTemplateParameter(
            inner->associated_constraint)) {
      SyntaxError(
          syntax,
          "template template parameter cannot have a concept-dependent "
          "constraint");
    }
  }

  TemplateTemplateParameterKind template_kind =
      kTemplateTemplateParameterType;
  if (LexMatch(lex, TOK(class)) || LexMatch(lex, TOK(typename))) {
    template_kind = kTemplateTemplateParameterType;
  } else if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
             LexMatch(lex, TOK(auto))) {
    template_kind = kTemplateTemplateParameterVariable;
  } else if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
             LexMatch(lex, TOK(concept))) {
    template_kind = kTemplateTemplateParameterConcept;
  } else {
    SyntaxError(syntax,
                "Expected 'class', 'typename', 'auto', or 'concept' in "
                "template template parameter");
    SyntaxRecover(syntax, TC(closebra));
    VectorDeleteWithContents(
        inner_parameters, (VectorElementDestructor)TemplateParameterDelete,
        /*free_element=*/false);
    return false;
  }
  bool is_parameter_pack = LexMatch(lex, TOK(ellipsis));

  String param_name;
  StringInit(&param_name, "");
  if (LexLookingAt(lex, TOK(identifier))) {
    StringSet(&param_name, lex->spelling.value);
    LexNextToken(lex);
  }

  if (param_name.length > 0) {
    TypeRecord* placeholder = NewTypeRecordWithSize(
        template_kind == kTemplateTemplateParameterConcept
            ? kTypeBool
            : kTypeInt | kTypeUnknown,
        kQualPlain);
    placeholder->template_parameter_index = index;
    placeholder->template_parameter_name = NewString(param_name.value);
    Symbol* param = NewSymbol(
        param_name.value, placeholder,
        template_kind == kTemplateTemplateParameterType ? STO(typedef)
                                                        : STO(implicit));
    param->flags.invented = true;
    param->flags.is_template = true;
    param->flags.is_template_parameter = true;
    param->flags.is_template_type_parameter = false;
    param->flags.is_template_template_parameter = true;
    param->flags.is_parameter_pack = is_parameter_pack;
    param->template_parameter_index = index;
    SymbolMutableExtra(param)->template_template_parameter_kind = template_kind;
    SymbolMutableExtra(param)->template_template_parameters =
        TemplateParameterVectorCopy(inner_parameters);
    if (template_kind == kTemplateTemplateParameterVariable) {
      param->variable_template = malloc(sizeof(VariableTemplate));
      param->variable_template->initializer = NULL;
      param->variable_template->associated_constraint = NULL;
      VectorInit(&param->variable_template->parameters);
      VectorInit(&param->variable_template->partial_specializations);
      for (size_t i = 0; i < inner_parameters->length; i++) {
        VectorAppend(&param->variable_template->parameters,
                     TemplateParameterCopy(inner_parameters->value.p[i]));
      }
    } else if (template_kind == kTemplateTemplateParameterConcept) {
      param->flags.is_concept = true;
    }
    param->location = location;
    if (!SyntaxAddSymbol(syntax, param)) {
      SymbolDelete(param);
    }
  }

  TemplateArgument* default_argument = NULL;
  if (LexMatch(lex, TOK(equal))) {
    if (is_parameter_pack) {
      SyntaxError(syntax, "Template parameter pack cannot have a default");
    }
    SourceLocation default_location = lex->current_token_location;
    if (template_kind == kTemplateTemplateParameterType) {
      TypeRecord* default_type = ParseTemplateTypeDefault(syntax);
      default_argument = TemplateTemplateArgumentFromType(
          syntax, default_type, default_location);
    } else {
      FullyQualifiedIdentifier default_name;
      FullyQualifiedIdentifierInit(&default_name);
      if (SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
              syntax, &default_name, TC(closebra) | TC(exprsep))) {
        Symbol* default_symbol =
            SyntaxFindQualifiedSymbol(syntax, &default_name);
        if (!SymbolNamesTemplateArgument(default_symbol)) {
          SyntaxError(syntax, "Template argument must name a template");
        } else {
          int parameter_index =
              default_symbol->flags.is_template_template_parameter
                  ? default_symbol->template_parameter_index
                  : -1;
          default_argument = NewTemplateTemplateArgument(
              default_symbol, parameter_index);
          default_argument->location = default_location;
        }
      }
      FullyQualifiedIdentifierDestruct(&default_name);
    }
  }

  TemplateParameter* template_param =
      NewTemplateParameter(param_name.value, kTemplateParameterTemplate,
                           is_parameter_pack, NULL, NULL, false, 0, -1, index);
  template_param->template_parameters = inner_parameters;
  template_param->template_template_kind = template_kind;
  template_param->default_argument = default_argument;
  VectorAppend(params, template_param);
  StringDestruct(&param_name);
  return true;
}

// `typename` opens a template parameter in two different roles: as the
// type-parameter keyword (`typename T`, `typename ...Ts`, `typename T = int`),
// or as the disambiguator in the type of a non-type parameter
// (`typename Dep<U>::type N`, the usual enable_if idiom).  Only the first is
// followed directly by the end of the parameter, so look past the keyword and
// the optional name to tell them apart.
static bool TypenameOpensTypeParameter(Syntax* syntax) {
  Lex* lex = syntax->lex;
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  LexNextToken(lex);
  LexMatch(lex, TOK(ellipsis));
  if (LexLookingAt(lex, TOK(identifier))) {
    LexNextToken(lex);
  }
  bool opens_type_parameter = LexLookingAt(lex, TOK(comma)) ||
                              LexLookingAt(lex, TOK(equal)) ||
                              LexLookingAtClosingAngle(lex);
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return opens_type_parameter;
}

// Parses one template parameter at the current token and appends it to
// `params` with index `base + params->length`.  Returns false when parsing
// fails after recovery.
static bool ParseTemplateParameter(Syntax* syntax, Vector* params, int base) {
  Lex* lex = syntax->lex;
  int index = base + (int)params->length;
  if (LexLookingAt(lex, TOK(template))) {
    return ParseTemplateTemplateParameter(syntax, params, base);
  }
  if (ParseConstrainedTemplateTypeParameter(syntax, params, base)) {
    return true;
  }
  if ((LexLookingAt(lex, TOK(typename)) && TypenameOpensTypeParameter(syntax) &&
       LexMatch(lex, TOK(typename))) ||
      LexMatch(lex, TOK(class))) {
    bool is_parameter_pack = LexMatch(lex, TOK(ellipsis));
    String param_name;
    StringInit(&param_name, "");
    if (LexLookingAt(lex, TOK(identifier))) {
      StringSet(&param_name, lex->spelling.value);
      TypeRecord* placeholder =
          NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
      placeholder->template_parameter_index = index;
      placeholder->template_parameter_name = NewString(lex->spelling.value);
      Symbol* param = NewSymbol(lex->spelling.value, placeholder, STO(typedef));
      param->flags.invented = true;
      param->flags.is_template_parameter = true;
      param->flags.is_template_type_parameter = true;
      param->flags.is_parameter_pack = is_parameter_pack;
      param->template_parameter_index = index;
      param->location = lex->current_token_location;
      bool added = SyntaxAddSymbol(syntax, param);
      if (!added) {
        SyntaxError(syntax, "Duplicate template parameter %s",
                    param->name.value);
        SymbolDelete(param);
      }
      LexNextToken(lex);
    } else if (!LexLookingAt(lex, TOK(equal)) &&
               !LexLookingAt(lex, TOK(comma)) &&
               !LexLookingAtClosingAngle(lex)) {
      SyntaxError(syntax, "Expected template parameter name or delimiter");
      SyntaxRecover(syntax, TC(closebra));
      StringDestruct(&param_name);
      return false;
    }
    TypeRecord* default_type = NULL;
    if (LexMatch(lex, TOK(equal))) {
      if (is_parameter_pack) {
        SyntaxError(syntax, "Template parameter pack cannot have a default");
      }
      default_type = ParseTemplateTypeDefault(syntax);
    }
    VectorAppend(params, NewTemplateParameter(param_name.value,
                                              kTemplateParameterType,
                                              is_parameter_pack, NULL,
                                              default_type, false, 0, -1,
                                              index));
    StringDestruct(&param_name);
    // NewTemplateParameter takes its own reference to `default_type`
    // (TypeRecordIncRef).  `default_type` came from ParseTemplateTypeDefault
    // with a zero reference count, so an additional TypeRecordDelete here would
    // drop the count back to zero and destruct the still-referenced record in
    // place -- freeing its dependent-member-name/template-argument data and
    // corrupting dependent defaults such as `class C = common_type<T,U>::type`.
    return true;
  }

  TypeParser parser;
  TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* param = TypeParserParseDeclarator(&parser, type);
  TypeParserDestruct(&parser);
  // Release `type` only when no declarator took it.  On success the symbol
  // holds the only reference, and `type` arrived here with a zero count, so
  // releasing it would destruct the still-referenced record in place and free
  // its dependent-member name and template arguments.  A primitive parameter
  // type survives that -- its bits live in the record itself -- which is why
  // this only showed up on a dependent one such as
  // `typename enable_if<C<T>, int>::type`, whose enable_if arguments and `type`
  // member name were silently erased.
  if (param == NULL) {
    TypeRecordDelete(type);
    SyntaxError(syntax, "Expected template parameter name");
    SyntaxRecover(syntax, TC(closebra));
    return false;
  }
  if (param->type != NULL && (param->type->type & kTypeAuto) != 0) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX17)) {
      SyntaxError(syntax,
                  "auto non-type template parameters require C++17");
    }
    TypeRecord* placeholder =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    placeholder->template_parameter_index = index;
    placeholder->template_parameter_name = NewString("auto");
    TypeRecordDelete(param->type);
    param->type = placeholder;
    TypeRecordIncRef(param->type);
  }
  param->flags.invented = true;
  param->flags.is_template_parameter = true;
  param->flags.is_template_type_parameter = false;
  param->template_parameter_index = index;
  bool is_parameter_pack = param->flags.is_parameter_pack;
  bool added = SyntaxAddSymbol(syntax, param);
  if (!added) {
    SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
    SymbolDelete(param);
  }
  TemplateArgument* default_argument = NULL;
  if (LexMatch(lex, TOK(equal))) {
    if (is_parameter_pack) {
      SyntaxError(syntax, "Template parameter pack cannot have a default");
    }
    default_argument = ParseTemplateNonTypeDefault(syntax);
  }
  TemplateParameter* template_param =
      NewTemplateParameter(param->name.value, kTemplateParameterNonType,
                           is_parameter_pack, param->type, NULL,
                           default_argument != NULL,
                           default_argument != NULL
                               ? default_argument->int_value : 0,
                           default_argument != NULL
                               ? default_argument->template_parameter_index : -1,
                           index);
  template_param->default_argument = default_argument;
  VectorAppend(params, template_param);
  return true;
}

// Parses `<` ... `>` as a template parameter list, assigning indices from
// `base`.  Temporarily exposes in-progress parameters so defaults can name
// earlier parameters.  Returns a new vector (possibly empty on error).
Vector* SyntaxParseTemplateParameterListWithBase(Syntax* syntax, int base) {
  Lex* lex = syntax->lex;
  if (!LexMatch(lex, TOK(less))) {
    SyntaxError(syntax, "Expected '<' after template");
    return NewVector();
  }
  Vector* params = NewVector();
  // Expose the in-progress parameter list while the list itself is parsed so a
  // later parameter's default argument that names earlier parameters (e.g.
  // `class C = typename common_type<T, U>::type`) is recognized as dependent and
  // kept unresolved.  The enclosing code only sets `current_template_parameters`
  // *after* the whole list is parsed, so without this such a default is eagerly
  // -- and wrongly -- resolved against the primary template (dropping the
  // `::type` member and yielding a bogus concrete type such as `int`).
  Vector* saved_template_parameters = syntax->current_template_parameters;
  int saved_template_parameter_count = syntax->current_template_parameter_count;
  // A member template's defaults can also expand the enclosing class's packs
  // (`class = decltype(f(std::declval<A>()...))`), so those stay visible.
  Vector visible_parameters;
  VectorInit(&visible_parameters);
  bool expose_enclosing = base > 0 && saved_template_parameters != NULL;
  for (size_t i = 0; expose_enclosing && i < saved_template_parameters->length;
       i++) {
    TemplateParameter* param = saved_template_parameters->value.p[i];
    if (param != NULL && param->index < base) {
      VectorAppend(&visible_parameters, param);
    }
  }
  syntax->current_template_parameters =
      expose_enclosing ? &visible_parameters : params;
  syntax->current_template_parameter_count = base;
  while (!LexEof(lex) && !LexLookingAtClosingAngle(lex)) {
    size_t first_new = params->length;
    ParseTemplateParameter(syntax, params, base);
    for (size_t i = first_new; expose_enclosing && i < params->length; i++) {
      VectorAppend(&visible_parameters, params->value.p[i]);
    }
    // Count the parameter now that it exists, so the next one's type sees a
    // non-empty enclosing template.  A qualified name is only treated as
    // dependent when parameters are in scope (see
    // BuildDependentTemplateScopeValueName), and the enclosing code does not
    // publish the count until the whole list has been read -- which left
    // `typename enable_if<C<T>, int>::type` resolving `C<T>::type` against C's
    // primary template right here, silently discarding the constraint.
    syntax->current_template_parameter_count = base + (int)params->length;
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  syntax->current_template_parameters = saved_template_parameters;
  syntax->current_template_parameter_count = saved_template_parameter_count;
  VectorDestruct(&visible_parameters);
  SyntaxNeedTemplateClose(syntax, TC(decl));
  return params;
}

// Parses a template parameter list with parameter indices starting at zero.
Vector* SyntaxParseTemplateParameterList(Syntax* syntax) {
  return SyntaxParseTemplateParameterListWithBase(syntax, 0);
}

// AST visitor callback that sets `*(bool*)data` when `node` makes an
// expression value-dependent on template parameters or class scope.
static void DependentTemplateExpressionVisitor(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL) {
    return;
  }
  if ((node->flags & kASTDependentQualifiedName) != 0 ||
      TypeIsUnknown(node->type) ||
      TypeContainsTemplateParameter(node->type)) {
    *(bool*)data = true;
    return;
  }
  if (node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) {
    // `sizeof(T)` is value-dependent.  The node's own type is size_t; the
    // operand type lives on the sizeof node, not in a child expression.
    SizeofASTNode* sizeof_node = (SizeofASTNode*)node;
    if (TypeContainsTemplateParameter(sizeof_node->type_operand)) {
      *(bool*)data = true;
      return;
    }
  }
  if (node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL) {
    return;
  }
  if ((id->symbol->flags.is_template_parameter &&
       id->symbol->template_parameter_index >= 0) ||
      id->symbol->dependent_value_template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(id->symbol->type) ||
      TemplateArgumentVectorIsDependent(id->template_arguments)) {
    *(bool*)data = true;
    return;
  }
  if (TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.cxx_member_owner != NULL &&
      StructContainsTemplateParameter(
          id->symbol->type->info.function.cxx_member_owner)) {
    // An unqualified call to a member of the current class-template
    // instantiation can depend on the class arguments even when its function
    // signature does not. For example, rank_dynamic() has a fixed size_t
    // signature but computes its value from the class's extent pack.
    *(bool*)data = true;
  }
}

// True if `node` reads a value through a dependent class-template scope
// (`Trait<T>::value`), making the whole expression value-dependent: it must be
// kept unevaluated and folded per-instantiation, not constant-folded now.
bool ExpressionContainsDependentTemplateParameter(ASTNode* node) {
  bool found = false;
  ASTNodeVisit(node, DependentTemplateExpressionVisitor, 0, &found);
  return found;
}

// Returns true when `node` is a bare identifier naming a non-type template
// parameter; stores its index in `*parameter_index` when that pointer is
// non-NULL.
bool ExpressionIsNonTypeTemplateParameter(ASTNode* node,
                                          int* parameter_index) {
  if (parameter_index != NULL) {
    *parameter_index = -1;
  }
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol == NULL || !id->symbol->flags.is_template_parameter ||
      id->symbol->flags.is_template_type_parameter ||
      id->symbol->template_parameter_index < 0) {
    return false;
  }
  if (parameter_index != NULL) {
    *parameter_index = id->symbol->template_parameter_index;
  }
  return true;
}
