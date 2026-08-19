//
//  type_parse.c
//  c_compiler
//

#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "ast.h"
#include "compiler.h"
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"
#include "type_traits_semantics.h"

static TemplateArgument* NewTemplateParameterTypeArgumentForType(int index,
                                                                 TypeRecord* type);
static bool ParseMemberPointerDeclarator(TypeParser* parser);
static void DebugTypeParseLeavingToken(TypeParser* parser, const char* where);

void TypeParserInit(TypeParser* parser, Lex* lex, struct Syntax* syntax,
                    Storage storage, ParserContext context) {
  parser->lex = lex;
  parser->syntax = syntax;
  parser->storage = storage;
  VectorInit(&parser->stack);
  VectorInit(&parser->pending_declaration_attributes);
  parser->symbol = NULL;
  parser->found_void = false;
  parser->dimension_count = 0;
  parser->is_inline = false;
  parser->is_constexpr = false;
  parser->is_consteval = false;
  parser->is_constinit = false;
  parser->allow_constexpr_decl_specifier = false;
  parser->declarator_is_parameter_pack = false;
  parser->declarator_ellipsis_count = 0;
  parser->context = context;
  parser->cxx_member_owner = NULL;
  parser->template_substitution_source = NULL;
  parser->template_substitution_target = NULL;
  parser->enclosing_template_substitution_source = NULL;
  parser->enclosing_template_substitution_target = NULL;
  parser->cxx_member_definition = NULL;
  parser->declarator_template_arguments = NULL;
  parser->parsing_direct_class_template = false;
  parser->template_substitution_failed = false;
  parser->placeholder_variable_constraint = NULL;
  parser->typename_allows_unqualified = false;
  parser->deferred_inline_bodies = NULL;
  parser->deferred_noexcept_specifiers = NULL;
}

void TypeParserReset(TypeParser* parser) {
  parser->symbol = NULL;
  parser->storage = STO(implicit);
  parser->found_void = false;
  parser->dimension_count = 0;
  parser->is_consteval = false;
  parser->is_constinit = false;
  parser->declarator_is_parameter_pack = false;
  parser->declarator_ellipsis_count = 0;
  parser->cxx_member_owner = NULL;
  parser->template_substitution_source = NULL;
  parser->template_substitution_target = NULL;
  parser->enclosing_template_substitution_source = NULL;
  parser->enclosing_template_substitution_target = NULL;
  parser->cxx_member_definition = NULL;
  parser->parsing_direct_class_template = false;
  parser->placeholder_variable_constraint = NULL;
  if (parser->declarator_template_arguments != NULL) {
    VectorDeleteWithContents(parser->declarator_template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    parser->declarator_template_arguments = NULL;
  }
  VectorDestruct(&parser->stack);
  VectorInit(&parser->stack);
  AttributeListDestruct(&parser->pending_declaration_attributes);
  VectorInit(&parser->pending_declaration_attributes);
}

void TypeParserDestruct(TypeParser* parser) {
  // Only frees the stack's backing array.  Any TypeRecords still referenced by
  // the stack are owned elsewhere (the combined result type) and must not be
  // freed here.
  if (parser->declarator_template_arguments != NULL) {
    VectorDeleteWithContents(parser->declarator_template_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    parser->declarator_template_arguments = NULL;
  }
  VectorDestruct(&parser->stack);
  AttributeListDestruct(&parser->pending_declaration_attributes);
}

TypeSubstitutionScope TypeParserPushTemplateSubstitution(
    TypeParser* parser, Struct* source, Struct* target) {
  TypeSubstitutionScope scope = {
      parser,
      parser != NULL ? parser->template_substitution_source : NULL,
      parser != NULL ? parser->template_substitution_target : NULL,
  };
  if (parser != NULL) {
    parser->template_substitution_source = source;
    parser->template_substitution_target = target;
  }
  return scope;
}

void TypeParserPopTemplateSubstitution(TypeSubstitutionScope* scope) {
  if (scope == NULL || scope->parser == NULL) {
    return;
  }
  scope->parser->template_substitution_source = scope->source;
  scope->parser->template_substitution_target = scope->target;
  scope->parser = NULL;
}

// Mapping for token vs type for parsing a type specifier.
static struct {
  Token token;
  Type type;
} type_map[] = {
    {TOK(char), kTypeChar},         {TOK(char8_t), kTypeChar8},
    {TOK(char16_t), kTypeChar16},   {TOK(char32_t), kTypeChar32},
    {TOK(int), kTypeInt},
    {TOK(short), kTypeShort},       {TOK(long), kTypeLong},
    {TOK(float), kTypeFloat},       {TOK(double), kTypeDouble},
    {TOK(class), kTypeStruct},      {TOK(struct), kTypeStruct},
    {TOK(union), kTypeUnion},
    {TOK(enum), kTypeEnum},         {TOK(void), kTypeVoid},
    {TOK(bool), kTypeBool},         {TOK(signed), kTypeSigned},
    {TOK(unsigned), kTypeUnsigned}, {TOK(auto), kTypeAuto},
    // In C++ `wchar_t` is a distinct keyword, but this implementation defines
    // it to its underlying integer type (matching `__WCHAR_TYPE__` and the C
    // `typedef int wchar_t`), so a `wchar_t` type-specifier behaves like `int`.
    {TOK(wchar_t), kTypeInt},
    {TOK(bad), kTypeImplicit},
};

TypeRecord* NewDecltypeReference(TypeRecord* expr_type, bool rvalue) {
  TypeRecord* base = TypeIsReference(expr_type) ? expr_type->next : expr_type;
  TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
  TypeRecordChain(ref, base);
  ref->type = base->type;
  TypeRecordCalculateSize(ref);
  return ref;
}

static TypeRecord* UnparenthesizedDecltypeEntityType(ASTNode* expr) {
  if (expr == NULL || (expr->flags & kASTParenthesized) != 0) {
    return NULL;
  }
  if (expr->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expr)->symbol;
    return symbol != NULL ? symbol->type : NULL;
  }
  if (expr->op == AST_OP(dot) || expr->op == AST_OP(arrow)) {
    ASTNode* right = ((BinaryASTNode*)expr)->right;
    if (right != NULL && right->op == AST_OP(structmember)) {
      StructMember* member = ((StructMemberASTNode*)right)->member;
      if (member != NULL && member->symbol != NULL &&
          !member->is_member_function) {
        return member->symbol->type;
      }
    }
  }
  return NULL;
}

TypeRecord* TypeDeduceDecltypeAuto(ASTNode* expr) {
  if (expr == NULL) {
    return NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  }
  TypeRecord* entity_type = UnparenthesizedDecltypeEntityType(expr);
  if (entity_type != NULL) {
    return TypeRecordCopy(entity_type);
  }
  if (expr->type == NULL) {
    return NULL;
  }
  if (expr->value_category == kValueCategoryLvalue) {
    return NewDecltypeReference(expr->type, false);
  }
  if (expr->value_category == kValueCategoryXvalue) {
    return NewDecltypeReference(expr->type, true);
  }
  return TypeRecordCopy(expr->type);
}

static bool TypeVectorContainsTemplateParameter(Vector* types) {
  if (types == NULL) {
    return false;
  }
  for (size_t i = 0; i < types->length; i++) {
    if (TypeContainsTemplateParameter((TypeRecord*)types->value.p[i])) {
      return true;
    }
  }
  return false;
}

static TypeRecord* ParseDaveInvokeResultType(TypeParser* parser) {
  LexNextToken(parser->lex);
  SyntaxNeedBracket(parser->syntax, TOK(lparen), TC(type));
  Vector type_args;
  Vector pack_flags;
  VectorInit(&type_args);
  VectorInit(&pack_flags);
  while (!LexLookingAt(parser->lex, TOK(rparen))) {
    parser->declarator_is_parameter_pack = false;
    TypeRecord* arg_type = TypeParserParseType(parser, true);
    Symbol* sym = TypeParserParseDeclarator(parser, arg_type);
    bool is_pack = sym->flags.is_parameter_pack;
    if (CompilerIsCXX() && LexMatch(parser->lex, TOK(ellipsis))) {
      is_pack = true;
    }
    TypeRecordIncRef(sym->type);
    VectorAppend(&type_args, sym->type);
    VectorAppend(&pack_flags, (void*)(intptr_t)is_pack);
    SymbolDelete(sym);
    if (!LexMatch(parser->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type));
  bool defer = TypeVectorContainsTemplateParameter(&type_args) ||
               parser->syntax->parsing_template_declaration;
  TypeRecord* result = NULL;
  if (!defer) {
    result = CXXTypeTraitInvokeResultType(parser->syntax, &type_args);
  }
  if (result == NULL) {
    TypeRecord* placeholder =
        TypeRecordNewInvokeResultPlaceholderWithFlags(&type_args, &pack_flags);
    VectorDestructWithContents(
        &type_args, (VectorElementDestructor)TypeRecordDelete,
        /*free_element=*/false);
    VectorDestruct(&pack_flags);
    if (placeholder == NULL) {
      SyntaxError(parser->syntax, "Invalid invoke_result type");
      return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    }
    return placeholder;
  }
  VectorDestructWithContents(
      &type_args, (VectorElementDestructor)TypeRecordDelete,
      /*free_element=*/false);
  VectorDestruct(&pack_flags);
  return result;
}

static TypeRecord* ParseDaveCommonTypeType(TypeParser* parser) {
  LexNextToken(parser->lex);
  SyntaxNeedBracket(parser->syntax, TOK(lparen), TC(type));
  Vector type_args;
  VectorInit(&type_args);
  while (!LexLookingAt(parser->lex, TOK(rparen))) {
    TypeRecord* arg_type = TypeParserParseType(parser, true);
    Symbol* sym = TypeParserParseDeclarator(parser, arg_type);
    TypeRecordIncRef(sym->type);
    VectorAppend(&type_args, sym->type);
    SymbolDelete(sym);
    if (!LexMatch(parser->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type));
  if (type_args.length != 2) {
    SyntaxError(parser->syntax, "Invalid common_type type");
    VectorDestructWithContents(
        &type_args, (VectorElementDestructor)TypeRecordDelete,
        /*free_element=*/false);
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  bool defer = TypeVectorContainsTemplateParameter(&type_args) ||
               parser->syntax->parsing_template_declaration;
  TypeRecord* result = NULL;
  if (!defer) {
    result = CXXTypeTraitCommonType(parser->syntax, &type_args);
  }
  if (result == NULL) {
    TypeRecord* placeholder = TypeRecordNewCommonTypePlaceholder(&type_args);
    VectorDestructWithContents(
        &type_args, (VectorElementDestructor)TypeRecordDelete,
        /*free_element=*/false);
    if (placeholder == NULL) {
      SyntaxError(parser->syntax, "Invalid common_type type");
      return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    }
    return placeholder;
  }
  VectorDestructWithContents(
      &type_args, (VectorElementDestructor)TypeRecordDelete,
      /*free_element=*/false);
  return result;
}

static TypeRecord* ParseCXXDecltypeSpecifier(TypeParser* parser) {
  LexNextToken(parser->lex);
  SyntaxNeedBracket(parser->syntax, TOK(lparen), TC(type));

  // `decltype(auto)` is a placeholder type: its deduction follows decltype
  // (value-category preserving) rules rather than template-argument deduction.
  if (LexLookingAt(parser->lex, TOK(auto))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(parser->lex, &checkpoint);
    LexNextToken(parser->lex);
    if (LexLookingAt(parser->lex, TOK(rparen))) {
      SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type));
      return NewTypeRecord(kTypeDecltypeAuto | kTypeAuto, kQualPlain);
    }
    // In C++23, `decltype(auto(expr))` names the type of an auto cast rather
    // than the `decltype(auto)` placeholder.  Restore the `auto` token and
    // parse the complete operand as an expression below.
    LexCheckpointRestore(parser->lex, &checkpoint);
  }

  bool parenthesized_expression = LexLookingAt(parser->lex, TOK(lparen));
  bool unparenthesized_identifier =
      !parenthesized_expression &&
      (LexLookingAt(parser->lex, TOK(identifier)) ||
       LexLookingAt(parser->lex, TOK(coloncolon)));
  ASTNode* expr = SyntaxParseSingleExpression(parser->syntax, TC(closebra));
  Symbol* declared_symbol = NULL;
  if (unparenthesized_identifier && expr != NULL &&
      expr->op == AST_OP(identifier)) {
    declared_symbol = ((IdentifierASTNode*)expr)->symbol;
  }
  if (!parenthesized_expression && expr != NULL &&
      expr->op == AST_OP(pack_index)) {
    // [dcl.type.decltype]: an unparenthesized pack-index-expression names the
    // selected entity's declared type. Preserve that distinction while its
    // dependent operand is deferred and re-evaluated during instantiation.
    expr->flags |= kASTUnparenthesizedDecltypeEntity;
  }
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type));

  TypeRecord* result = NULL;
  if (declared_symbol != NULL) {
    result = TypeRecordCopy(declared_symbol->type);
  } else {
    expr = AnalyzeExpression(expr);
    if (expr == NULL || expr->type == NULL) {
      SyntaxError(parser->syntax, "Invalid expression in decltype");
      result = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    } else if (expr->value_category == kValueCategoryLvalue) {
      result = NewDecltypeReference(expr->type, false);
    } else if (expr->value_category == kValueCategoryXvalue) {
      result = NewDecltypeReference(expr->type, true);
    } else {
      result = TypeRecordCopy(expr->type);
    }
    if (result != NULL &&
        parser->syntax->current_template_parameters != NULL &&
        (TypeIsUnknown(expr->type) ||
         TypeContainsTemplateParameter(expr->type) ||
         DependentExpressionContainsTemplateParameter(expr))) {
      result->dependent_decltype_expr = expr;
      expr = NULL;
    }
  }
  if (expr != NULL) {
    ASTNodeDelete(expr);
  }
  if (result == NULL) {
    SyntaxError(parser->syntax, "Invalid decltype specifier");
    result = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  return result;
}

bool CurrentClassNameMatchesTypeName(Struct* owner, String* name) {
  if (!CompilerIsCXX() || owner == NULL || owner->tag_name == NULL ||
      name == NULL) {
    return false;
  }
  if (StringEqualString(owner->tag_name, name)) {
    return true;
  }
  const char* template_args = strchr(owner->tag_name->value, '<');
  if (template_args == NULL) {
    return false;
  }
  size_t base_length = (size_t)(template_args - owner->tag_name->value);
  return name->length == base_length &&
         strncmp(name->value, owner->tag_name->value, base_length) == 0;
}

// The class whose members are currently being parsed.  While a class body is
// parsed the owner lives on the TypeParser, but a member function *body* parses
// its local declarations through freshly-initialized TypeParsers that do not
// carry it; recover it from the member function then in flight (which does, via
// cxx_member_owner) so a self-type reference inside the body still resolves.
Struct* CurrentClassBeingParsed(TypeParser* parser) {
  if (parser != NULL && parser->cxx_member_owner != NULL) {
    return parser->cxx_member_owner;
  }
  // Only consult the current function while genuinely parsing statements inside
  // a function body (block scope): `compiler->current_function` is not cleared
  // between top-level declarations, so at file scope it may still point at the
  // last member function parsed, which would spuriously match a namespace-scope
  // use of that class's name (e.g. in an alias `using A = ThatClass<...>;`).
  if (parser != NULL && parser->context == kParsingBlockScope &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    return compiler->current_function->info.function.cxx_member_owner;
  }
  // Fall back to the class whose base-clause/body is currently being parsed.
  // Nested template-argument parsing (e.g. a self-template-id inside a base
  // specifier like `base<subrange<I, S> >`) spins up fresh TypeParsers that do
  // not carry cxx_member_owner, so recover the enclosing class from the syntax
  // state instead.
  if (parser != NULL && parser->syntax != NULL &&
      parser->syntax->cxx_class_head != NULL) {
    return parser->syntax->cxx_class_head;
  }
  return NULL;
}

TypeRecord* ParseCurrentClassTemplateType(TypeParser* parser,
                                                 String* name) {
  Struct* owner = CurrentClassBeingParsed(parser);
  while (owner != NULL) {
    if (CurrentClassNameMatchesTypeName(owner, name) &&
        owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
        TypeIsStructOrUnion(owner->tag_symbol->type)) {
      break;
    }
    owner = owner->lexical_parent;
  }
  if (owner == NULL) {
    return NULL;
  }
  LexNextToken(parser->lex);
  Vector* args = NULL;
  if (LexLookingAt(parser->lex, TOK(less))) {
    args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
  }
  TypeRecord* type_record = TypeRecordCopy(owner->tag_symbol->type);
  if (args != NULL) {
    // Inside a partial specialization, the owner's tag is the specialization
    // itself (whose struct is not marked is_template), so using it as the
    // template origin would prevent InstantiateSimpleClassTemplate from
    // choosing the right specialization for a *different* argument list (e.g.
    // the recursive member `storage<I + 1, Rest...>` inside
    // `storage<I, T, Rest...>`). Resolve the primary class template by name and
    // use it as the origin so such references instantiate correctly.
    Symbol* template_origin = owner->tag_symbol;
    Symbol* primary = SyntaxFindSymbol(parser->syntax, name);
    if (primary != NULL && primary->flags.is_template &&
        primary->type != NULL && TypeIsStructOrUnion(primary->type) &&
        primary->type->info.struct_info != NULL &&
        primary->type->info.struct_info->is_template) {
      template_origin = primary;
    }
    type_record->template_origin = template_origin;
    type_record->template_arguments = args;
    args = NULL;
  }
  if (args != NULL) {
    VectorDestructWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
  }
  return type_record;
}

static bool CurrentClassTemplateNameStartsType(TypeParser* parser) {
  if (!CompilerIsCXX() || parser == NULL ||
      !LexLookingAt(parser->lex, TOK(identifier))) {
    return false;
  }
  String name;
  StringInit(&name, parser->lex->spelling.value);
  Struct* owner = CurrentClassBeingParsed(parser);
  while (owner != NULL) {
    if (CurrentClassNameMatchesTypeName(owner, &name) &&
        owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
        TypeIsStructOrUnion(owner->tag_symbol->type)) {
      StringDestruct(&name);
      return true;
    }
    owner = owner->lexical_parent;
  }
  StringDestruct(&name);
  return false;
}

// Returns true if `type` (recursively, including template arguments, array
// bounds, and dependent-member paths) references a template parameter whose
// index is at or beyond `threshold`.  Used to detect a self-qualified type such
// as `EnclosingClass<..., MemberTemplateParam, ...>::member` inside a member
// function template: a parameter introduced by the *member* template has an
// index past the enclosing class's own parameters, which proves the prefix is a
// *different* specialization (a member of an unknown specialization) rather than
// the current instantiation.
static bool TypeReferencesTemplateParameterAtLeast(TypeRecord* type,
                                                   int threshold);

static bool TemplateArgumentReferencesTemplateParameterAtLeast(
    TemplateArgument* arg, int threshold) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= threshold) {
    return true;
  }
  if (arg->type != NULL &&
      TypeReferencesTemplateParameterAtLeast(arg->type, threshold)) {
    return true;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (TemplateArgumentReferencesTemplateParameterAtLeast(
              arg->pack_arguments->value.p[i], threshold)) {
        return true;
      }
    }
  }
  return false;
}

static bool TypeReferencesTemplateParameterAtLeast(TypeRecord* type,
                                                   int threshold) {
  if (type == NULL) {
    return false;
  }
  if (type->template_parameter_index >= threshold) {
    return true;
  }
  if (type->template_origin != NULL &&
      type->template_origin->flags.is_template_template_parameter &&
      type->template_origin->template_parameter_index >= threshold) {
    return true;
  }
  if (TypeIsArray(type) &&
      type->info.array.template_parameter_index >= threshold) {
    return true;
  }
  if (type->template_arguments != NULL) {
    for (size_t i = 0; i < type->template_arguments->length; i++) {
      if (TemplateArgumentReferencesTemplateParameterAtLeast(
              type->template_arguments->value.p[i], threshold)) {
        return true;
      }
    }
  }
  if (type->dependent_member_template_arguments != NULL) {
    for (size_t i = 0; i < type->dependent_member_template_arguments->length;
         i++) {
      Vector* component_args =
          type->dependent_member_template_arguments->value.p[i];
      if (component_args == NULL) {
        continue;
      }
      for (size_t j = 0; j < component_args->length; j++) {
        if (TemplateArgumentReferencesTemplateParameterAtLeast(
                component_args->value.p[j], threshold)) {
          return true;
        }
      }
    }
  }
  return TypeReferencesTemplateParameterAtLeast(type->next, threshold);
}

static TypeRecord* BuildDependentMemberTemplateTypename(
    TypeParser* parser, FullyQualifiedIdentifier* name,
    bool require_member_template_id) {
  if (parser == NULL || name == NULL ||
      parser->syntax->current_template_parameters == NULL ||
      name->components.length < 2 ||
      name->template_arguments.length != name->components.length) {
    return NULL;
  }

  String* base_name = name->components.value.p[0];
  Symbol* base = SyntaxFindSymbol(parser->syntax, base_name);
  TypeRecord* base_type = NULL;
  int base_template_parameter_index = -1;
  if (base != NULL && base->flags.is_template_parameter &&
      base->flags.is_template_type_parameter &&
      base->template_parameter_index >= 0) {
    base_template_parameter_index = base->template_parameter_index;
  } else if (base != NULL && StorageIs(base->storage, STO(typedef)) &&
             base->type != NULL && TypeContainsTemplateParameter(base->type)) {
    base_type = base->type;
  } else {
    return NULL;
  }

  bool has_member_template_id = false;
  for (size_t i = 1; i < name->components.length; i++) {
    if (name->template_arguments.value.p[i] != NULL) {
      has_member_template_id = true;
      break;
    }
  }
  if (require_member_template_id && !has_member_template_id) {
    return NULL;
  }

  String encoded_name;
  StringInit(&encoded_name, NULL);
  Vector* component_args = NewVector();
  for (size_t i = 1; i < name->components.length; i++) {
    if (i != 1) {
      StringAppend(&encoded_name, "::");
    }
    StringAppendString(&encoded_name, name->components.value.p[i]);
    VectorAppend(component_args, TemplateArgumentVectorCopy(
                                     name->template_arguments.value.p[i]));
  }

  TypeRecord* type = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
  if (base_type != NULL) {
    TypeRecordDelete(type);
    type = TypeRecordCopy(base_type);
  } else {
    type->template_parameter_index = base_template_parameter_index;
  }
  if (type->dependent_member_name != NULL) {
    StringDelete(type->dependent_member_name);
  }
  type->dependent_member_name = NewString(encoded_name.value);
  if (type->dependent_member_template_arguments != NULL) {
    VectorDeleteWithContents(
        type->dependent_member_template_arguments,
        (VectorElementDestructor)TemplateArgumentVectorDelete,
        /*free_element=*/false);
  }
  type->dependent_member_template_arguments = component_args;
  StringDestruct(&encoded_name);
  return type;
}

static bool LookingAtCXXPackIndexedTypename(TypeParser* parser) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !LexLookingAt(parser->lex, TOK(typename))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  bool result = LexLookingAt(parser->lex, TOK(identifier));
  if (result) {
    LexNextToken(parser->lex);
    result = LexLookingAt(parser->lex, TOK(ellipsis));
  }
  if (result) {
    LexNextToken(parser->lex);
    result = LexLookingAt(parser->lex, TOK(lsquare));
  }
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

/* Parse `typename Ts...[I]::member` (including a qualified member tail). */
static TypeRecord* ParseCXXPackIndexedTypename(TypeParser* parser) {
  LexMatch(parser->lex, TOK(typename));
  String pack_name;
  StringInit(&pack_name, parser->lex->spelling.value);
  Symbol* pack_symbol = SyntaxFindSymbol(parser->syntax, &pack_name);
  StringDestruct(&pack_name);
  LexNextToken(parser->lex);

  TypeRecord* type =
      pack_symbol != NULL && pack_symbol->type != NULL
          ? TypeRecordCopy(pack_symbol->type)
          : NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
  int parameter_index = -1;
  bool is_type_parameter =
      TypeIsTemplateParameterPlaceholder(type, &parameter_index);
  if (!is_type_parameter ||
      !CurrentTemplateParameterIsPack(parser->syntax, parameter_index)) {
    SyntaxError(parser->syntax,
                "pack indexing requires a type template parameter pack");
  }

  LexMatch(parser->lex, TOK(ellipsis));
  LexMatch(parser->lex, TOK(lsquare));
  type->is_pack_index = is_type_parameter;
  type->pack_index_expr =
      SyntaxParseSingleExpression(parser->syntax, TC(closebra));
  SyntaxNeedBracket(parser->syntax, TOK(rsquare), TC(decl));
  if (!LexMatch(parser->lex, TOK(coloncolon))) {
    SyntaxError(parser->syntax,
                "expected '::' after pack indexing specifier");
    return type;
  }

  FullyQualifiedIdentifier member;
  FullyQualifiedIdentifierInit(&member);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
          parser->syntax, &member, TC(decl))) {
    SyntaxError(parser->syntax,
                "expected member type after pack indexing specifier");
    FullyQualifiedIdentifierDestruct(&member);
    return type;
  }
  type->dependent_member_name = NewString(member.spelling.value);
  type->dependent_member_template_arguments = NewVector();
  for (size_t i = 0; i < member.template_arguments.length; i++) {
    VectorAppend(type->dependent_member_template_arguments,
                 TemplateArgumentVectorCopy(
                     member.template_arguments.value.p[i]));
  }
  FullyQualifiedIdentifierDestruct(&member);
  return type;
}

static bool LookingAtCXXSplicedType(TypeParser* parser) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !LexLookingAt(parser->lex, TOK(typename))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  bool result = LexLookingAt(parser->lex, TOK(splice_open));
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

static TypeRecord* ParseCXXSplicedType(TypeParser* parser) {
  SourceLocation location = parser->lex->current_token_location;
  LexMatch(parser->lex, TOK(typename));
  SyntaxNeedBracket(parser->syntax, TOK(splice_open), TC(type));
  ASTNode* reflection =
      SyntaxParseExpression(parser->syntax, TC(spliceclose));
  SyntaxNeedBracket(parser->syntax, TOK(splice_close), TC(type));

  if (reflection != NULL && reflection->op == AST_OP(reflect)) {
    ReflectionASTNode* reflected = (ReflectionASTNode*)reflection;
    if (reflected->operand_kind == kReflectionOperandType &&
        reflected->operand_type != NULL) {
      TypeRecord* result = TypeRecordCopy(reflected->operand_type);
      ASTNodeDelete(reflection);
      return result;
    }
  }

  TypeRecord* dependent =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  dependent->dependent_splice_expr =
      NewSpliceASTNode(reflection, kSpliceType, location);
  return dependent;
}

static bool CAtomicTypeIsSupported(TypeRecord* type) {
  return type != NULL &&
         (TypeIsIntegral(type) || TypeIsFloatingPoint(type) ||
          TypeIsPointer(type));
}

// Parse the atomic-type-specifier form `_Atomic(type-name)`.  The qualifier
// form (`_Atomic int`) is handled by ParseTypeSpecifier's qualifier path.
static TypeRecord* ParseCAtomicTypeSpecifier(TypeParser* parser) {
  TypeParser nested;
  TypeParserInit(&nested, parser->lex, parser->syntax, STO(implicit),
                 parser->context);
  TypeRecord* base = TypeParserParseType(&nested, true);
  Symbol* abstract = TypeParserParseDeclarator(&nested, base);
  TypeRecord* result =
      abstract != NULL ? TypeRecordCopy(abstract->type) : TypeRecordCopy(base);
  if (abstract != NULL) {
    SymbolDelete(abstract);
  }
  TypeParserDestruct(&nested);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type) | TC(decl));

  if (result == NULL) {
    return NewTypeRecordWithSize(kTypeInt, kQualAtomic);
  }
  if (TypeIsAtomic(result)) {
    SyntaxError(parser->syntax, "_Atomic cannot be applied to an atomic type");
  }
  if ((result->qualifiers &
       (kQualConst | kQualVolatile | kQualRestrict)) != 0) {
    SyntaxError(parser->syntax,
                "_Atomic(type-name) requires an unqualified type");
  }
  if (!CAtomicTypeIsSupported(result)) {
    SyntaxError(parser->syntax,
                "_Atomic currently supports only scalar and pointer types");
  }
  result->qualifiers |= kQualAtomic;
  return result;
}

static TypeRecord* ParseCBitIntTypeSpecifier(TypeParser* parser) {
  Lex* lex = parser->lex;
  LexNextToken(lex);
  if (!LexMatch(lex, TOK(lparen))) {
    SyntaxError(parser->syntax, "Expected '(' after _BitInt");
    return NewBitIntTypeRecord(2, false, kQualPlain);
  }

  ASTNode* width_expr =
      SyntaxParseSingleExpression(parser->syntax, TC(closebra));
  width_expr = AnalyzeExpression(width_expr);
  int64_t width = 0;
  if (!EvaluateIntegerExpression(width_expr, &width)) {
    SyntaxError(parser->syntax,
                "_BitInt width must be an integer constant expression");
    width = 2;
  } else if (width < 1 || width > DAVECC_BITINT_MAXWIDTH) {
    SyntaxError(parser->syntax,
                "_BitInt width must be between 1 and BITINT_MAXWIDTH (%d)",
                DAVECC_BITINT_MAXWIDTH);
    width = width < 1 ? 1 : DAVECC_BITINT_MAXWIDTH;
  }
  ASTNodeDelete(width_expr);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type) | TC(decl));
  return NewBitIntTypeRecord((int)width, false, kQualPlain);
}

static bool CTypeofOperandIsBitField(ASTNode* expr) {
  if (expr == NULL || (expr->op != AST_OP(dot) && expr->op != AST_OP(arrow))) {
    return false;
  }
  ASTNode* right = ((BinaryASTNode*)expr)->right;
  if (right == NULL || right->op != AST_OP(structmember)) {
    return false;
  }
  StructMember* member = ((StructMemberASTNode*)right)->member;
  return member != NULL && member->bit_size != 0;
}

static TypeRecord* ParseCTypeofSpecifier(TypeParser* parser, bool unqualified) {
  Lex* lex = parser->lex;
  LexNextToken(lex);
  if (!LexMatch(lex, TOK(lparen))) {
    SyntaxError(parser->syntax, "Expected '(' after %s",
                unqualified ? "typeof_unqual" : "typeof");
    return NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }

  TypeRecord* result = NULL;
  if (SyntaxLookingAtType(parser->syntax)) {
    TypeParser nested;
    TypeParserInit(&nested, lex, parser->syntax, STO(implicit),
                   parser->context);
    TypeRecord* base = TypeParserParseType(&nested, true);
    Symbol* abstract = TypeParserParseDeclarator(&nested, base);
    result = abstract != NULL ? TypeRecordCopy(abstract->type)
                              : TypeRecordCopy(base);
    if (abstract != NULL) {
      SymbolDelete(abstract);
    }
    TypeParserDestruct(&nested);
  } else {
    ASTNode* expr =
        SyntaxParseSingleExpression(parser->syntax, TC(closebra));
    expr = AnalyzeExpression(expr);
    if (CTypeofOperandIsBitField(expr)) {
      SyntaxError(parser->syntax,
                  "typeof cannot be applied to a bit-field");
    }
    if (expr != NULL && expr->type != NULL) {
      result = TypeRecordCopy(expr->type);
    }
    ASTNodeDelete(expr);
  }
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(type) | TC(decl));

  if (result == NULL) {
    result = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  if (unqualified) {
    result->qualifiers &=
        ~(kQualConst | kQualVolatile | kQualRestrict | kQualAtomic);
  }
  return result;
}

// Parse a type-specifier.  This might also be a typedef reference which
// contains a full TypeRecord.
static PartialTypeSpecifier ParseTypeSpecifier(TypeParser* parser, bool allow_typedef) {
  PartialTypeSpecifier result;
  result.error = false;
  
  Type type = kTypeImplicit;
  Qualifiers quals = kQualPlain;
  Lex* lex = parser->lex;
  TypeRecord* type_record = NULL;

  Token tok = lex->current_token;
  bool found = false;

  if (tok == TOK(bitint)) {
    type_record = ParseCBitIntTypeSpecifier(parser);
    type |= type_record->type;
    found = true;
  }

  if (!found && tok == TOK(atomic)) {
    LexNextToken(lex);
    if (LexMatch(lex, TOK(lparen))) {
      type_record = ParseCAtomicTypeSpecifier(parser);
      type |= type_record->type;
    } else {
      quals |= kQualAtomic;
    }
    found = true;
  }

  if (CompilerIsCXX() && allow_typedef && tok == TOK(identifier) &&
      strcmp(lex->spelling.value, "__davecc_invoke_result_t") == 0) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(lex, &checkpoint);
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(lparen))) {
      LexCheckpointRestore(lex, &checkpoint);
      type_record = ParseDaveInvokeResultType(parser);
      type |= type_record->type;
      found = true;
    } else {
      LexCheckpointRestore(lex, &checkpoint);
    }
    LexCheckpointDestruct(&checkpoint);
  }

  if (!found && CompilerIsCXX() && allow_typedef && tok == TOK(identifier) &&
      strcmp(lex->spelling.value, "__davecc_common_type_t") == 0) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(lex, &checkpoint);
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(lparen))) {
      LexCheckpointRestore(lex, &checkpoint);
      type_record = ParseDaveCommonTypeType(parser);
      type |= type_record->type;
      found = true;
    } else {
      LexCheckpointRestore(lex, &checkpoint);
    }
    LexCheckpointDestruct(&checkpoint);
  }
  
  if (parser->found_void) {
    // Special handling for already-consumed void type.  This can
    // happen inside a function prototype.
    type |= kTypeVoid;
    parser->found_void = false;
    found = true;
  } else {
    // Check for known type.
    for (int i = 0; type_map[i].token != TOK(bad); i++) {
      if (type_map[i].token == tok) {
        if (tok == TOK(auto) && !CompilerIsCXX()) {
          break;
        }
        // First check for a typedef name reference.
        LexNextToken(lex);
        
        type |= type_map[i].type;
        found = true;
        break;
      }
    }
  }

  // If we didn't find a known type, look for qualifiers and typedef
  // name.
  if (!found) {
    if (CompilerCXXAtLeast(kLanguageStandardCXX20) && allow_typedef &&
        LexLookingAt(lex, TOK(identifier))) {
      String concept_name;
      StringInit(&concept_name, lex->spelling.value);
      Symbol* concept_symbol = SyntaxFindSymbol(parser->syntax, &concept_name);
      if (concept_symbol != NULL && concept_symbol->flags.is_concept) {
        LexCheckpoint checkpoint;
        LexCheckpointSave(lex, &checkpoint);
        SourceLocation constraint_location = lex->current_token_location;
        LexNextToken(lex);
        Vector* concept_arguments = NULL;
        if (LexLookingAt(lex, TOK(less))) {
          concept_arguments =
              SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
        } else {
          concept_arguments = NewVector();
        }
        if (concept_arguments == NULL) {
          concept_arguments = NewVector();
        }
        if (LexLookingAt(lex, TOK(auto))) {
          LexNextToken(lex);
          TypeRecord* placeholder =
              NewTypeRecordWithSize(kTypeAuto, kQualPlain);
          TemplateArgument* constrained_arg =
              NewTemplateParameterTypeArgumentForType(0, placeholder);
          if (concept_arguments->length == 0) {
            VectorAppend(concept_arguments, constrained_arg);
          } else {
            VectorInsertBefore(concept_arguments, 0, constrained_arg);
          }
          parser->placeholder_variable_constraint =
              NewConceptIdConstraint(concept_symbol, concept_arguments,
                                     constraint_location);
          type_record = placeholder;
          type |= kTypeAuto;
          found = true;
          StringDestruct(&concept_name);
          LexCheckpointDestruct(&checkpoint);
        } else {
          if (concept_arguments != NULL) {
            VectorDeleteWithContents(
                concept_arguments,
                (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          }
          LexCheckpointRestore(lex, &checkpoint);
          LexCheckpointDestruct(&checkpoint);
        }
      }
      StringDestruct(&concept_name);
    }
    if (!found && LexMatch(lex, TOK(const))) {
      quals |= kQualConst;
    } else if (LexMatch(lex, TOK(volatile))) {
      quals |= kQualVolatile;
    } else if (LexMatch(lex, TOK(restrict))) {
      quals |= kQualRestrict;
    } else if (CompilerCAtLeast(kLanguageStandardC23) &&
               (LexLookingAt(lex, TOK(typeof)) ||
                LexLookingAt(lex, TOK(typeof_unqual)))) {
      bool unqualified = LexLookingAt(lex, TOK(typeof_unqual));
      type_record = ParseCTypeofSpecifier(parser, unqualified);
      type |= type_record->type;
    } else if (CompilerIsCXX() && LexLookingAt(lex, TOK(decltype))) {
      type_record = ParseCXXDecltypeSpecifier(parser);
      type |= type_record->type;
    } else if (CompilerIsCXX() && allow_typedef &&
               LookingAtCXXPackIndexedTypename(parser)) {
      type_record = ParseCXXPackIndexedTypename(parser);
      type |= type_record->type;
    } else if (CompilerIsCXX() && allow_typedef &&
               LookingAtCXXSplicedType(parser)) {
      type_record = ParseCXXSplicedType(parser);
      type |= type_record->type;
    } else if (CompilerIsCXX() && allow_typedef &&
               LexMatch(lex, TOK(typename))) {
      LexCheckpoint typename_name_start;
      LexCheckpointSave(lex, &typename_name_start);
      FullyQualifiedIdentifier typename_name;
      FullyQualifiedIdentifierInit(&typename_name);
      if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
              parser->syntax, &typename_name, TC(decl))) {
        SyntaxError(parser->syntax, "Expected qualified type name after typename");
      } else if (!typename_name.is_qualified &&
                 parser->typename_allows_unqualified) {
        // In a type-requirement the type-name after `typename` may be an
        // unqualified simple-template-id or type-name.  Reparse it through the
        // ordinary type-name path, which already resolves aliases, class
        // templates, and type parameters.
        LexCheckpointRestore(lex, &typename_name_start);
        LexCheckpointDestruct(&typename_name_start);
        FullyQualifiedIdentifierDestruct(&typename_name);
        return ParseTypeSpecifier(parser, allow_typedef);
      } else if (!typename_name.is_qualified) {
        SyntaxError(parser->syntax, "typename requires a qualified type name");
      } else {
        bool handled_dependent_template_member = false;
        if (parser->syntax->current_template_parameters != NULL &&
            typename_name.components.length >= 2 &&
            typename_name.template_arguments.length ==
                typename_name.components.length) {
          type_record =
              BuildDependentMemberTemplateTypename(
                  parser, &typename_name,
                  /*require_member_template_id=*/true);
          if (type_record != NULL) {
            type |= type_record->type;
            handled_dependent_template_member = true;
          }
          if (!handled_dependent_template_member) {
            size_t base_index = typename_name.components.length - 2;
            Vector* parsed_args =
                typename_name.template_arguments.value.p[base_index];
            if (parsed_args != NULL) {
            FullyQualifiedIdentifier prefix;
            FullyQualifiedIdentifierInit(&prefix);
            prefix.absolute = typename_name.absolute;
            prefix.is_qualified = prefix.absolute || base_index > 0;
            for (size_t i = 0; i <= base_index; i++) {
              String* component = typename_name.components.value.p[i];
              if (prefix.spelling.length != 0 || prefix.absolute) {
                StringAppend(&prefix.spelling, "::");
              }
              StringAppendString(&prefix.spelling, component);
              VectorAppend(&prefix.components, NewString(component->value));
              Vector* component_args = typename_name.template_arguments.value.p[i];
              VectorAppend(&prefix.template_arguments,
                           TemplateArgumentVectorCopy(component_args));
            }
            Symbol* base = SyntaxFindQualifiedSymbol(parser->syntax, &prefix);
            Symbol* dependent_member_origin = NULL;
            if (base != NULL && base->flags.is_template) {
              dependent_member_origin = base;
            } else if (base_index == 0) {
              // The prefix may name an enclosing class template by its
              // injected-class-name, which is not yet flagged is_template while
              // that class's own body is being parsed.  If any prefix argument
              // is introduced by a *member* template -- its parameter index sits
              // at or beyond the class's own parameters -- then `Prefix<...>` is
              // a different specialization, so `Prefix<...>::member` denotes a
              // member of an unknown specialization and must stay dependent
              // (resolved per specialization at instantiation) rather than
              // collapse to the current instantiation's member and silently drop
              // the differing argument.
              String* prefix_name = typename_name.components.value.p[0];
              for (Struct* owner = CurrentClassBeingParsed(parser);
                   owner != NULL; owner = owner->lexical_parent) {
                bool owner_is_template =
                    owner->is_template || owner->template_parameter_count > 0 ||
                    owner->defining_template_scope_count > 0;
                if (!owner_is_template || owner->tag_name == NULL ||
                    owner->tag_symbol == NULL ||
                    strcmp(owner->tag_name->value, prefix_name->value) != 0) {
                  continue;
                }
                int own_scope = owner->template_parameter_count > 0
                                    ? owner->template_parameter_count
                                    : owner->defining_template_scope_count;
                bool references_member_template_param = false;
                for (size_t i = 0; i < parsed_args->length; i++) {
                  if (TemplateArgumentReferencesTemplateParameterAtLeast(
                          parsed_args->value.p[i], own_scope)) {
                    references_member_template_param = true;
                    break;
                  }
                }
                if (references_member_template_param) {
                  Symbol* primary =
                      SyntaxFindSymbol(parser->syntax, prefix_name);
                  dependent_member_origin =
                      (primary != NULL && primary->flags.is_template)
                          ? primary
                          : owner->tag_symbol;
                }
                break;
              }
            }
            if (dependent_member_origin != NULL) {
              String* member_name =
                  typename_name.components.value.p[
                      typename_name.components.length - 1];
            type_record = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
            type_record->template_origin = dependent_member_origin;
            type_record->template_arguments =
                TemplateArgumentVectorCopy(parsed_args);
            type_record->dependent_member_name =
                NewString(member_name->value);
            type |= type_record->type;
            handled_dependent_template_member = true;
            }
            FullyQualifiedIdentifierDestruct(&prefix);
            }
          }
        }
        if (handled_dependent_template_member) {
          // Keep the dependent member lookup for template instantiation.
        } else {
        Symbol* symbol =
            SyntaxFindQualifiedSymbol(parser->syntax, &typename_name);
        // If the qualifier names a local alias whose type is a *dependent*
        // class-template specialization (e.g. `using tbl = HT<P<Key>, Key>;`
        // followed by `typename tbl::iterator`), resolving the member eagerly
        // against the qualified symbol would follow the alias to the primary
        // template `HT` and copy its member's type verbatim, dropping the
        // specialization's arguments `[P<Key>, Key]`.  The primary's member
        // type still references `HT`'s own parameters by index (e.g. `Value`
        // at index 0), which then aliases the enclosing template's parameter
        // at the same index (e.g. `Key`) once substituted.  Keep such a member
        // deferred as `HT<Args>::iterator` so it resolves against the correct
        // specialization at instantiation time.
        bool base_is_dependent_alias = false;
        if (typename_name.components.length == 2) {
          String* base_name0 = typename_name.components.value.p[0];
          Symbol* base0 = SyntaxFindSymbol(parser->syntax, base_name0);
          if (base0 != NULL && StorageIs(base0->storage, STO(typedef)) &&
              base0->type != NULL &&
              TypeIsStructOrUnion(base0->type) &&
              base0->type->template_origin != NULL &&
              TypeContainsTemplateParameter(base0->type)) {
            base_is_dependent_alias = true;
          }
        }
        if (symbol != NULL && StorageIs(symbol->storage, STO(typedef)) &&
            !base_is_dependent_alias) {
          symbol->flags.used = true;
          Vector* args = NULL;
          if (symbol->flags.is_template &&
              typename_name.template_arguments.length > 0) {
            Vector* parsed_args =
                typename_name.template_arguments.value.p[
                    typename_name.template_arguments.length - 1];
            args = TemplateArgumentVectorCopy(parsed_args);
          }
          if (symbol->flags.is_template && args != NULL &&
              !parser->syntax->parsing_template_declaration &&
              !TemplateArgumentVectorContainsTemplateParameter(args) &&
              TypeIsStructOrUnion(symbol->type)) {
            type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
          } else {
            type_record = TypeRecordCopy(symbol->type);
            if (symbol->flags.is_template && args != NULL) {
              if (type_record->template_arguments != NULL) {
                VectorDeleteWithContents(
                    type_record->template_arguments,
                    (VectorElementDestructor)TemplateArgumentDelete,
                    /*free_element=*/false);
              }
              type_record->template_origin = symbol;
              type_record->template_arguments = args;
              args = NULL;
            }
          }
          if (args != NULL) {
            VectorDeleteWithContents(
                args, (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          }
          type |= type_record->type;
        } else if (typename_name.components.length == 2) {
          String* base_name = typename_name.components.value.p[0];
          Symbol* base = SyntaxFindSymbol(parser->syntax, base_name);
          if (base != NULL && base->flags.is_template_parameter &&
              base->flags.is_template_type_parameter &&
              base->template_parameter_index >= 0) {
            String* member_name = typename_name.components.value.p[1];
            type_record =
                NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
            type_record->template_parameter_index =
                base->template_parameter_index;
            type_record->dependent_member_name =
                NewString(member_name->value);
            type |= type_record->type;
          } else if (base != NULL && StorageIs(base->storage, STO(typedef)) &&
                     base->type != NULL &&
                     TypeContainsTemplateParameter(base->type)) {
            String* member_name = typename_name.components.value.p[1];
            // `typename alias::member` where `alias` names a dependent
            // class-template specialization (e.g. `using tbl = HT<P<Key>,Key>;
            // typename tbl::iterator`).  Copying the alias's struct spine would
            // make `member` masquerade as the base class itself: a variable of
            // this type would be treated as an `HT` object and its constructor
            // baked under the base tag name (`found.HT(...)`), which then fails
            // once `member` resolves to a *different* nested type (e.g.
            // `__hash_iterator`) at instantiation.  Represent it instead as a
            // clean dependent-member placeholder that carries the base's
            // template-id (origin + arguments) plus the member name, so it
            // resolves against the correct specialization's member later
            // without dropping the specialization's arguments.
            if (base->type->template_origin != NULL &&
                base->type->template_arguments != NULL) {
              type_record = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
              type_record->template_origin = base->type->template_origin;
              type_record->template_arguments =
                  TemplateArgumentVectorCopy(base->type->template_arguments);
              type_record->dependent_member_name = NewString(member_name->value);
            } else {
              type_record = TypeRecordCopy(base->type);
              if (type_record->dependent_member_name != NULL) {
                StringDelete(type_record->dependent_member_name);
              }
              type_record->dependent_member_name = NewString(member_name->value);
            }
            type |= type_record->type;
          } else {
            SyntaxError(parser->syntax, "Unknown type name %s",
                        typename_name.spelling.value);
          }
        } else {
          SyntaxError(parser->syntax, "Unknown type name %s",
                      typename_name.spelling.value);
        }
        }
      }
      LexCheckpointDestruct(&typename_name_start);
      FullyQualifiedIdentifierDestruct(&typename_name);
    } else if (allow_typedef &&
               (SyntaxCurrentTokenStartsQualifiedName(parser->syntax) ||
                (parser->syntax->parsing_friend_type_specifier &&
                 SyntaxCurrentIdentifierFollowedByScopeOperator(
                     parser->syntax)))) {
      FullyQualifiedIdentifier typedef_name;
      FullyQualifiedIdentifierInit(&typedef_name);
      SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
          parser->syntax, &typedef_name, TC(decl));
      Symbol* symbol = SyntaxFindQualifiedSymbol(parser->syntax, &typedef_name);
      if (symbol != NULL && StorageIs(symbol->storage, STO(typedef))) {
        symbol->flags.used = true;
        Vector* args = NULL;
        if (symbol->flags.is_template) {
          if (typedef_name.template_arguments.length > 0) {
            Vector* parsed_args =
                typedef_name.template_arguments.value.p[
                    typedef_name.template_arguments.length - 1];
            args = TemplateArgumentVectorCopy(parsed_args);
          } else if (LexLookingAt(lex, TOK(less))) {
            args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
          }
        }
        if (symbol->flags.is_template &&
            !symbol->flags.is_template_template_parameter && args != NULL &&
            !TypeIsStructOrUnion(symbol->type) &&
            !CXXAliasTemplatePatternNamesClassTemplate(symbol) &&
            !TemplateArgumentVectorContainsTemplateParameter(args)) {
          Vector* completed_args = CompleteAliasTemplateArguments(symbol, args);
          if (completed_args != NULL) {
            if (!ConceptsConstraintSatisfied(symbol->associated_constraint,
                                             completed_args)) {
              SyntaxError(parser->syntax,
                          "constraints not satisfied for alias template %s",
                          symbol->name.value);
              ConceptsReportAssociatedConstraintFailure(
                  symbol->associated_constraint, completed_args, symbol->location,
                  NULL);
              type_record = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
            } else {
              type_record =
                  SubstituteTemplateParameters(parser, symbol->type,
                                               completed_args);
              type_record = TypeMaterializeClassTemplateSpecialization(
                  parser->syntax, type_record);
            }
            VectorDeleteWithContents(
                completed_args, (VectorElementDestructor)TemplateArgumentDelete,
                /*free_element=*/false);
          } else {
            type_record = TypeRecordCopy(symbol->type);
          }
        } else if (symbol->flags.is_template && args != NULL &&
            !parser->syntax->parsing_template_declaration &&
            TypeIsStructOrUnion(symbol->type)) {
          type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
        } else {
          type_record = TypeRecordCopy(symbol->type);
          if (symbol->flags.is_template_template_parameter) {
            type_record->template_origin = symbol;
          }
          if (symbol->flags.is_template && args == NULL &&
              !parser->syntax->parsing_template_declaration &&
              symbol->alias_template != NULL &&
              !CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
            SyntaxError(parser->syntax,
                        "Alias template %s does not name a deducible class "
                        "template",
                        symbol->name.value);
          }
          if (symbol->flags.is_template && args == NULL &&
              !parser->syntax->parsing_template_declaration &&
              TypeIsStructOrUnion(symbol->type)) {
            if (CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
              SetCXXAliasTemplatePlaceholderOrigin(symbol, type_record);
            } else {
              type_record->template_origin = symbol;
            }
          }
          if (symbol->flags.is_template && args != NULL &&
              (parser->syntax->current_template_parameters != NULL ||
               TemplateArgumentVectorContainsTemplateParameter(args))) {
            if (symbol->alias_template == NULL &&
                TypeIsStructOrUnion(symbol->type) &&
                symbol->type->info.struct_info != NULL) {
              Vector* completed = CompleteClassTemplateArguments(
                  parser, symbol->type->info.struct_info, args);
              if (completed != NULL) {
                VectorDeleteWithContents(
                    completed, (VectorElementDestructor)TemplateArgumentDelete,
                    /*free_element=*/false);
              }
            }
            type_record->template_origin = symbol;
            type_record->template_arguments = args;
            args = NULL;
          }
        }
        type |= type_record->type;
        if (args != NULL) {
          VectorDestructWithContents(args,
                                     (VectorElementDestructor)TemplateArgumentDelete,
                                     /*free_element=*/false);
        }
      } else if (parser->syntax->parsing_friend_type_specifier) {
        type_record = BuildDependentMemberTemplateTypename(
            parser, &typedef_name,
            /*require_member_template_id=*/false);
        if (type_record != NULL) {
          type |= type_record->type;
        } else {
          SyntaxError(parser->syntax, "Unknown type name %s",
                      typedef_name.spelling.value);
        }
      } else {
        SyntaxError(parser->syntax, "Unknown type name %s",
                    typedef_name.spelling.value);
      }
      FullyQualifiedIdentifierDestruct(&typedef_name);
    } else if (allow_typedef && tok == TOK(identifier)) {
      // Identifier.  If this is a known typedef name consume it
      // and keep the type.
      String typedef_name;
      StringInit(&typedef_name, lex->spelling.value);
      Symbol* symbol = SyntaxFindSymbol(parser->syntax, &typedef_name);
      if (symbol != NULL && !symbol->flags.is_template &&
          LexLookingAt(lex, TOK(less))) {
        Symbol* tag_symbol = SyntaxFindTag(parser->syntax, &typedef_name);
        if (tag_symbol != NULL && tag_symbol->flags.is_template &&
            tag_symbol->type != NULL && TypeIsStructOrUnion(tag_symbol->type)) {
          symbol = tag_symbol;
        }
      }
      if (symbol == NULL) {
        Symbol* tag_symbol = SyntaxFindTag(parser->syntax, &typedef_name);
        if (tag_symbol != NULL && tag_symbol->flags.is_template &&
            tag_symbol->type != NULL && TypeIsStructOrUnion(tag_symbol->type)) {
          symbol = tag_symbol;
        }
      }
      TypeRecord* current_class_type =
          ParseCurrentClassTemplateType(parser, &typedef_name);
      if (current_class_type != NULL) {
        type_record = current_class_type;
        type |= type_record->type;
      } else if (symbol != NULL) {
        // Reference to a typedef?
        if (StorageIs(symbol->storage, STO(typedef))) {
          // Naming a typedef as a type counts as a use of it
          // (-Wunused-local-typedef).
          symbol->flags.used = true;
          LexNextToken(lex);
          Vector* args = NULL;
          if (symbol->flags.is_template && LexLookingAt(lex, TOK(less))) {
            args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
          }
          if (symbol->flags.is_template &&
              !symbol->flags.is_template_template_parameter && args != NULL &&
              !TypeIsStructOrUnion(symbol->type) &&
              !CXXAliasTemplatePatternNamesClassTemplate(symbol) &&
              !TemplateArgumentVectorContainsTemplateParameter(args)) {
            Vector* completed_args = CompleteAliasTemplateArguments(symbol, args);
            if (completed_args != NULL) {
              if (!ConceptsConstraintSatisfied(symbol->associated_constraint,
                                               completed_args)) {
                SyntaxError(parser->syntax,
                            "constraints not satisfied for alias template %s",
                            symbol->name.value);
                ConceptsReportAssociatedConstraintFailure(
                    symbol->associated_constraint, completed_args,
                    symbol->location, NULL);
                type_record =
                    NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
              } else {
                type_record =
                    SubstituteTemplateParameters(parser, symbol->type,
                                                 completed_args);
                type_record = TypeMaterializeClassTemplateSpecialization(
                    parser->syntax, type_record);
              }
              VectorDeleteWithContents(
                  completed_args,
                  (VectorElementDestructor)TemplateArgumentDelete,
                  /*free_element=*/false);
            } else {
              type_record = TypeRecordCopy(symbol->type);
            }
          } else if (symbol->flags.is_template && args != NULL &&
              !parser->syntax->parsing_template_declaration &&
              TypeIsStructOrUnion(symbol->type)) {
            type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
          } else {
            type_record = TypeRecordCopy(symbol->type);
            if (symbol->flags.is_template_template_parameter) {
              type_record->template_origin = symbol;
            }
            if (symbol->flags.is_template && args == NULL &&
                !parser->syntax->parsing_template_declaration &&
                symbol->alias_template != NULL &&
                !CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
              SyntaxError(parser->syntax,
                          "Alias template %s does not name a deducible class "
                          "template",
                          symbol->name.value);
            }
            if (symbol->flags.is_template && args == NULL &&
                !parser->syntax->parsing_template_declaration &&
                TypeIsStructOrUnion(symbol->type)) {
              if (CXXAliasTemplatePatternNamesClassTemplate(symbol)) {
                SetCXXAliasTemplatePlaceholderOrigin(symbol, type_record);
              } else {
                type_record->template_origin = symbol;
              }
            }
            // A template alias used with arguments inside another template
            // declaration is a dependent alias template-id: record the alias as
            // origin and keep the supplied arguments so it is expanded (by
            // substituting the alias pattern) once they become concrete.  This
            // must also apply when the alias's pattern is not itself a
            // class/union (e.g. `using alt_t = typename alt<I, V>::type;`),
            // otherwise the arguments would be dropped and the alias pattern's
            // own parameters would leak into the enclosing template.
            if (symbol->flags.is_template && args != NULL &&
                parser->syntax->current_template_parameters != NULL) {
              type_record->template_origin = symbol;
              if (type_record->template_arguments != NULL) {
                VectorDeleteWithContents(
                    type_record->template_arguments,
                    (VectorElementDestructor)TemplateArgumentDelete,
                    /*free_element=*/false);
              }
              type_record->template_arguments = args;
              args = NULL;
            }
          }
          type |= type_record->type;
          if (args != NULL) {
            VectorDestructWithContents(args,
                                       (VectorElementDestructor)TemplateArgumentDelete,
                                       /*free_element=*/false);
          }
        } else if (symbol->flags.is_template && symbol->type != NULL &&
                   TypeIsStructOrUnion(symbol->type)) {
          symbol->flags.used = true;
          LexNextToken(lex);
          Vector* args = NULL;
          if (LexLookingAt(lex, TOK(less))) {
            args = SyntaxParseTemplateArgumentList(parser->syntax, TC(decl));
          }
          if (args != NULL && !parser->syntax->parsing_template_declaration &&
              !TemplateArgumentVectorContainsTemplateParameter(args)) {
            type_record = InstantiateSimpleClassTemplate(parser, symbol, args);
          } else {
            type_record = TypeRecordCopy(symbol->type);
            type_record->template_origin = symbol;
            if (args != NULL) {
              type_record->template_arguments = args;
              args = NULL;
            }
          }
          type |= type_record->type;
          if (args != NULL) {
            VectorDestructWithContents(args,
                                       (VectorElementDestructor)TemplateArgumentDelete,
                                       /*free_element=*/false);
          }
        }
      } else if (CompilerIsCXX()) {
        Symbol* tag = SyntaxFindTag(parser->syntax, &typedef_name);
        if (tag != NULL && tag->type != NULL &&
            TypeIsStructOrUnion(tag->type)) {
          LexNextToken(lex);
          type_record = TypeRecordCopy(tag->type);
          type |= type_record->type;
        }
      }
      StringDestruct(&typedef_name);
    }
  }

  // Check for struct, union or enum and parse it if necessary.
  if (type_record == NULL &&
      (type & (kTypeStruct | kTypeUnion | kTypeEnum)) != 0) {
    
    Symbol* tag;
    TypeParser composite_parser;
    TypeParserInit(&composite_parser, parser->lex, parser->syntax,
                   STO(implicit), kParsingStructOrUnion);
    composite_parser.cxx_member_owner = parser->cxx_member_owner;
    composite_parser.parsing_direct_class_template =
        parser->parsing_direct_class_template;
    
    if ((type & (kTypeStruct | kTypeUnion)) != 0) {
      bool is_union = (type & kTypeUnion) != 0;
      bool is_class = tok == TOK(class);
      tag = TypeParserParseStruct(&composite_parser, is_union, is_class);
    } else {
      tag = TypeParserParseEnum(&composite_parser);
    }
    if (tag != NULL) {
      // We need to copy the type record because it is held in the
      // struct tag and we need to apply our qualifiers to it for this
      // type definition.  For example, this might be:
      //   const struct Foo;
      // and the type record will be the one inside the symbol for 'Foo'
      type_record = TypeRecordCopy(tag->type);
      type |= type_record->type;
      
      if (LexMatch(lex, TOK(const))) {
        quals |= kQualConst;
      } else if (LexMatch(lex, TOK(volatile))) {
        quals |= kQualVolatile;
      } else if (LexMatch(lex, TOK(restrict))) {
        quals |= kQualRestrict;
      }
      type_record->qualifiers |= quals;
    }
    TypeParserDestruct(&composite_parser);
  }

  result.type = type;
  result.quals = quals;
  result.type_record = type_record;
  return result;
}

// This is a list of all the valid type combinations.
// These come from the C99 spec, section 6.7.2.
static Type valid_types[] = {
  kTypeVoid,
  
  kTypeChar,
  kTypeChar | kTypeSigned,
  kTypeChar | kTypeUnsigned,
  
  kTypeChar8,
  kTypeChar16,
  kTypeChar32,

  kTypeShort,
  kTypeShort | kTypeSigned,
  kTypeShort | kTypeInt,
  kTypeShort | kTypeSigned | kTypeInt,
  kTypeShort | kTypeUnsigned,
  kTypeShort | kTypeUnsigned | kTypeInt,
  
  kTypeInt,
  kTypeInt | kTypeSigned,
  kTypeSigned,
  kTypeUnsigned,
  kTypeUnsigned | kTypeInt,
  
  kTypeLong,
  kTypeLong | kTypeInt,
  kTypeLong | kTypeSigned,
  kTypeLong | kTypeSigned | kTypeInt,
  kTypeLong | kTypeUnsigned,
  kTypeLong | kTypeUnsigned | kTypeInt,
  
  kTypeLongLong,
  kTypeLongLong | kTypeSigned,
  kTypeLongLong | kTypeInt,
  kTypeLongLong | kTypeSigned | kTypeInt,
  kTypeLongLong | kTypeUnsigned,
  kTypeLongLong | kTypeUnsigned | kTypeInt,
  
  kTypeFloat,
  
  kTypeDouble,
  kTypeDouble | kTypeLong,
  
  kTypeBool,

  kTypeAuto,
  
  kTypeStruct,
  
  kTypeUnion,
  
  kTypeEnum,
};

#define NUM_VALID_TYPES (sizeof(valid_types)/sizeof(valid_types[0]))

static bool IsValidType(Type t) {
  for (size_t i = 0; i < NUM_VALID_TYPES; i++) {
    if (t == valid_types[i]) {
      return true;
    }
  }
  return false;
}

static bool IsValidQualiferCombo(Qualifiers q1, Qualifiers q2) {
  return (q1 & q2) == 0;
}

static void TypeComboError1(Syntax* syntax, Type t1, Type t2) {
  String error;
  StringInit(&error, "");
  TypeToString(t1, &error);
  if (t2 != kTypeImplicit) {
    StringAppend(&error, " and ");
    TypeToString(t2, &error);
  }
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void TypeComboError2(Syntax* syntax, TypeRecord* t1, Type t2) {
  String error;
  StringInit(&error, "defined type ");
  TypeRecordToString(t1, &error);
  StringAppend(&error, " and ");
  TypeToString(t2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void TypeComboError3(Syntax* syntax, TypeRecord* t1, TypeRecord* t2) {
  String error;
  StringInit(&error, "defined type ");
  TypeRecordToString(t1, &error);
  StringAppend(&error, " and defined type ");
  TypeRecordToString(t2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

static void QualifierComboError(Syntax* syntax, Qualifiers q1, Qualifiers q2) {
  String error;
  StringInit(&error, "");
  QualifiersToString(q1, &error);
  StringAppend(&error, " and ");
  QualifiersToString(q2, &error);
  SyntaxError(syntax, "Invalid type combination; can't combine %s",
              error.value);
  StringDestruct(&error);
}

// Type specifiers can be split into pieces.  For example you could
// have:
//
// int extern unsigned foo;
//
// Where the 'int' and 'unsigned' are split by a storage specifier.
// This function combines two type specifiers if it can and issues
// errors and warnings as necessary.
static PartialTypeSpecifier CombineTypeSpecifiers(Syntax* syntax,
                                                PartialTypeSpecifier* t1,
                                                PartialTypeSpecifier* t2) {
  PartialTypeSpecifier result = {0};
  // Check for a valid type.  You can't combine types that contain the
  // same bits:
  // e.g. short short
  // However, we need to handle 'long long'.
  if ((t1->type & kTypeLong) != 0 && (t2->type & kTypeLong) != 0) {
    t1->type &= ~kTypeLong;
    t1->type |= kTypeLongLong;
    t2->type = kTypeImplicit;
  }
  if (t1->type_record != NULL || t2->type_record != NULL) {
    // Either t1->type_record or t2->type_record is non-NULL. Put the non-NULL
    // one in t1 so qualifier-only combinations like `const T` and `T const`
    // can be handled before the primitive valid-type check sees T's internal
    // unknown marker.
    if (t1->type_record == NULL) {
      PartialTypeSpecifier* tmp = t1;
      t1 = t2;
      t2 = tmp;
    }
    if (TypeIsBitInt(t1->type_record) && t2->type_record == NULL &&
        (t2->type == kTypeSigned || t2->type == kTypeUnsigned)) {
      bool conflicting_sign =
          (t2->type == kTypeSigned && TypeIsUnsigned(t1->type_record));
      if (conflicting_sign) {
        TypeComboError2(syntax, t1->type_record, t2->type);
        result.error = true;
      } else if (t2->type == kTypeUnsigned) {
        t1->type_record->type &= ~kTypeSigned;
        t1->type_record->type |= kTypeUnsigned;
      }
      if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
        QualifierComboError(syntax, t1->quals, t2->quals);
        result.error = true;
      }
      result.type = t1->type_record->type;
      result.quals = t1->quals | t2->quals;
      result.type_record = t1->type_record;
      return result;
    }
    if (t2->type_record != NULL) {
      TypeComboError3(syntax, t1->type_record, t2->type_record);
      result.error = true;
    } else if (t2->type != kTypeImplicit) {
      TypeComboError2(syntax, t1->type_record, t2->type);
      result.error = true;
    }
    if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
      QualifierComboError(syntax, t1->quals, t2->quals);
      result.error = true;
    }
    result.type = t1->type;
    result.quals = t1->quals | t2->quals;
    result.type_record = t1->type_record;
    return result;
  }
  result.type = t1->type | t2->type;
  
  bool type_ok = result.type == kTypeImplicit ||
                 (t1->type & t2->type) == 0;
  if (type_ok && result.type != kTypeImplicit) {
    type_ok = IsValidType(result.type);
  }
  if (!type_ok) {
    TypeComboError1(syntax, t1->type, t2->type);
    result.error = true;
  }
  
  // Convert 'long double' to kTypeLongDouble.
  if ((result.type & (kTypeLong | kTypeDouble)) == (kTypeLong | kTypeDouble)) {
    result.type &= ~(kTypeLong | kTypeDouble);
    result.type |= kTypeLongDouble;
  }
  
  // Can't combine qualifiers if they are the same.
  // e.g. const const
  if (!IsValidQualiferCombo(t1->quals, t2->quals)) {
    QualifierComboError(syntax, t1->quals, t2->quals);
    result.error = true;
  }
  result.quals = t1->quals | t2->quals;
  result.type_record = NULL;

  if (result.error) {
    return result;
  }
  return result;
}

PartialTypeSpecifier TypeParserParseAndCombineTypes(TypeParser* parser,
                                                   PartialTypeSpecifier* prev) {
  PartialTypeSpecifier curr = ParseTypeSpecifier(parser, prev->type == kTypeImplicit);
  if (prev->type == kTypeImplicit && prev->quals == kQualPlain) {
    return curr;
  }
  
  return CombineTypeSpecifiers(parser->syntax, prev, &curr);
}

// Given a ParseTypeSpecifier, build a TypeRecord.
TypeRecord* TypeParserBuildTypeRecord(TypeParser* parser, PartialTypeSpecifier* type) {
  if (type->error) {
    return NewTypeRecordWithSize(kTypeInt, kQualPlain);
  }
  if (type->type_record == NULL) {
    if (type->type == kTypeImplicit) {
      return NULL;
    }
    return NewTypeRecordWithSize(type->type, type->quals);
  } else {
    // Add qualifiers to typedef copy.
    type->type_record->qualifiers |= type->quals;
    if (TypeIsBitInt(type->type_record) &&
        type->type_record->bit_width == 1 &&
        !TypeIsUnsigned(type->type_record)) {
      SyntaxError(parser->syntax, "signed _BitInt width must be at least 2");
    }
    return type->type_record;
  }
}

static bool LookingAtCXXTypePackIndex(TypeParser* parser);
static void ParseCXXTypePackIndex(TypeParser* parser, TypeRecord* base_type);

TypeRecord* TypeParserParseType(TypeParser* parser, bool needed) {
  Syntax* syntax = parser->syntax;
  
  PartialTypeSpecifier type_specifier = {
    .type = kTypeImplicit,
    .quals = kQualPlain,
    .type_record = NULL,
    .error = false };

  // Leading __attribute__((...)) specifiers (GCC extension) before the type.
  TypeParserSkipAttributes(parser);

  while (parser->found_void || SyntaxLookingAtType(syntax) ||
         CurrentClassTemplateNameStartsType(parser)) {
    if (parser->allow_constexpr_decl_specifier &&
        LexMatch(parser->lex, TOK(constexpr))) {
      if (parser->is_constexpr) {
        SyntaxError(parser->syntax, "Duplicate 'constexpr' specifier");
      }
      parser->is_constexpr = true;
      continue;
    }
    if (type_specifier.type != kTypeImplicit &&
        (SyntaxCurrentIdentifierFollowedByScopeOperator(syntax) ||
         SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax))) {
      // `int S::*` is a member-pointer declarator, not a second type specifier.
      break;
    }
    PartialTypeSpecifier new_type_specifier = ParseTypeSpecifier(parser, type_specifier.type == kTypeImplicit);
    if (new_type_specifier.type == kTypeImplicit &&
        new_type_specifier.quals == kQualPlain &&
        new_type_specifier.type_record == NULL) {
      break;
    }
    if (type_specifier.type == kTypeImplicit && type_specifier.quals == kQualPlain) {
      type_specifier = new_type_specifier;
    } else {
      type_specifier = CombineTypeSpecifiers(parser->syntax, &type_specifier, &new_type_specifier);
    }
  }
  // No type?
  if (type_specifier.type == kTypeImplicit &&
      type_specifier.type_record == NULL) {
    if (needed) {
      SyntaxError(parser->syntax, "Type expected");
      SyntaxRecover(parser->syntax, TC(semicolon) | TC(type));
      return NewTypeRecordWithSize(kTypeInt, kQualPlain);
    }
    return NULL;
  }
  TypeRecord* built = TypeParserBuildTypeRecord(parser, &type_specifier);
  if (LookingAtCXXTypePackIndex(parser)) {
    ParseCXXTypePackIndex(parser, built);
    while (LexLookingAt(parser->lex, TOK(const)) ||
           LexLookingAt(parser->lex, TOK(volatile)) ||
           LexLookingAt(parser->lex, TOK(restrict))) {
      if (LexMatch(parser->lex, TOK(const))) {
        built->qualifiers |= kQualConst;
      } else if (LexMatch(parser->lex, TOK(volatile))) {
        built->qualifiers |= kQualVolatile;
      } else {
        LexMatch(parser->lex, TOK(restrict));
        built->qualifiers |= kQualRestrict;
      }
    }
  }
  return built;
}

static bool LookingAtCXXTypePackIndex(TypeParser* parser) {
  if (!CompilerIsCXX() ||
      !CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !LexLookingAt(parser->lex, TOK(ellipsis))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  bool result = LexLookingAt(parser->lex, TOK(lsquare));
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

static void ParseCXXTypePackIndex(TypeParser* parser, TypeRecord* base_type) {
  LexMatch(parser->lex, TOK(ellipsis));
  LexMatch(parser->lex, TOK(lsquare));
  ASTNode* index =
      SyntaxParseSingleExpression(parser->syntax, TC(closebra));
  SyntaxNeedBracket(parser->syntax, TOK(rsquare), TC(decl) | TC(exprsep));

  int parameter_index = -1;
  bool is_type_parameter =
      TypeIsTemplateParameterPlaceholder(base_type, &parameter_index);
  if (!is_type_parameter ||
      !CurrentTemplateParameterIsPack(parser->syntax, parameter_index)) {
    SyntaxError(parser->syntax,
                "pack indexing requires a type template parameter pack");
  }
  if (is_type_parameter) {
    base_type->is_pack_index = true;
    base_type->pack_index_expr = index;
  } else {
    ASTNodeDelete(index);
  }
}

static void ValidateCAtomicDeclarator(TypeParser* parser, TypeRecord* type) {
  if (CompilerIsCXX()) {
    return;
  }
  bool target_supported = CompilerTargetSupportsC11Atomics();
  bool target_error_reported = false;
  for (TypeRecord* record = type; record != NULL; record = record->next) {
    if (!TypeIsAtomic(record)) {
      continue;
    }
    if (TypeIsArray(record) || TypeIsFunction(record)) {
      SyntaxError(parser->syntax,
                  "_Atomic cannot qualify an array or function type");
    } else if (!CAtomicTypeIsSupported(record)) {
      SyntaxError(parser->syntax,
                  "_Atomic currently supports only scalar and pointer types");
    } else if (target_supported &&
               !CompilerTargetSupportsAtomicSize(record->size)) {
      SyntaxError(parser->syntax,
                  "atomic object size is not supported on this target");
    }
    if (!target_error_reported && !target_supported) {
      SyntaxError(parser->syntax,
                  "atomic types are unavailable on this target");
      target_error_reported = true;
    }
  }
}

Symbol* TypeParserParseDeclarator(TypeParser* parser, TypeRecord* base_type) {
  if (base_type == NULL) {
    return NULL;
  }
  if (parser->context == kParsingPrototype) {
    parser->syntax->found_open_paren = false;
  }
  
  VectorClear(&parser->stack);
  parser->symbol = NULL;
  parser->base_type = base_type;
  parser->declarator_is_parameter_pack = false;
  parser->declarator_ellipsis_count = 0;
  if (LookingAtCXXTypePackIndex(parser)) {
    ParseCXXTypePackIndex(parser, base_type);
  } else {
    parser->declarator_is_parameter_pack =
        CompilerIsCXX() && LexMatch(parser->lex, TOK(ellipsis));
    if (parser->declarator_is_parameter_pack) {
      parser->declarator_ellipsis_count++;
    }
  }
  if (ParseMemberPointerDeclarator(parser)) {
    // Handled `T C::*` without going through the generic pointer path.
  } else {
    TypeParserParsePointer(parser);
  }
  if (CompilerIsCXX() && LexMatch(parser->lex, TOK(ellipsis))) {
    parser->declarator_is_parameter_pack = true;
    parser->declarator_ellipsis_count++;
  }

  // Join all the type records together in reverse order.
  size_t i = parser->stack.length;
  TypeRecord* t = parser->base_type;
  while (i > 0) {
    TypeRecord* record = (TypeRecord*)parser->stack.value.p[i - 1];
    if (TypeIsFunction(record) && record->next != NULL) {
      record->type = record->next->type;
    } else {
      TypeRecordChain(record, t);
      record->type = t->type;
    }
    t = record;
    i--;
  }

  // Calculate the size of t, now that we have the complete chain.
  TypeRecordCalculateSize(t);
  ValidateCAtomicDeclarator(parser, t);

  if (parser->symbol != NULL) {
    SymbolSetType(parser->symbol, t);
  } else {
    // Invent a fake symbol.
    parser->symbol = NewSymbol(SyntaxFakeName(parser->syntax), t, STO(auto));
    parser->symbol->flags.invented = true;
  }
  parser->symbol->flags.is_parameter_pack =
      parser->declarator_is_parameter_pack;
  for (size_t attr_index = 0;
       attr_index < parser->pending_declaration_attributes.length;
       attr_index++) {
    VectorAppend(&parser->symbol->attributes,
                 parser->pending_declaration_attributes.value.p[attr_index]);
  }
  VectorClear(&parser->pending_declaration_attributes);
  if (TypeIsFunction(parser->symbol->type) &&
      parser->declarator_template_arguments != NULL) {
    parser->symbol->type->template_arguments =
        parser->declarator_template_arguments;
    parser->declarator_template_arguments = NULL;
  }
  return parser->symbol;
}

// Skip any __attribute__((...)) specifiers (a GCC extension) that can appear
// in declarator positions (pointers, parenthesized declarators, type names).
// The attributes are parsed and discarded.  Returns true if at least one was
// seen.
bool TypeParserSkipAttributes(TypeParser* parser) {
  bool any = false;
  while (LexLookingAt(parser->lex, TOK(attribute)) ||
         SyntaxLookingAtCXXAttribute(parser->syntax)) {
    Vector attrs = {0};
    VectorInit(&attrs);
    if (LexMatch(parser->lex, TOK(attribute))) {
      SyntaxParseAttribute(parser->syntax, &attrs);
    } else {
      SyntaxParseCXXAttributes(parser->syntax, &attrs);
    }
    for (size_t i = 0; i < attrs.length; i++) {
      VectorAppend(&parser->pending_declaration_attributes, attrs.value.p[i]);
    }
    VectorClear(&attrs);
    VectorDestruct(&attrs);
    any = true;
  }
  return any;
}

static Qualifiers ParseQualifiers(TypeParser* parser) {
  Qualifiers quals = kQualPlain;
  int num_consts = 0;
  int num_volatiles = 0;
  int num_restricts = 0;
  int num_atomics = 0;
  while (!LexEof(parser->lex)) {
    if (LexMatch(parser->lex, TOK(const))) {
      quals |= kQualConst;
      num_consts++;
    } else if (LexMatch(parser->lex, TOK(volatile))) {
      num_volatiles++;
      quals |= kQualVolatile;
    } else if (LexMatch(parser->lex, TOK(restrict))) {
      num_restricts++;
      quals |= kQualRestrict;
    } else if (LexMatch(parser->lex, TOK(atomic))) {
      num_atomics++;
      quals |= kQualAtomic;
    } else if (LexLookingAt(parser->lex, TOK(attribute)) ||
               SyntaxLookingAtCXXAttribute(parser->syntax)) {
      TypeParserSkipAttributes(parser);
    } else {
      break;
    }
  }
  if (num_consts > 1 || num_volatiles > 1 || num_restricts > 1 ||
      num_atomics > 1) {
    SyntaxError(parser->syntax, "Invalid pointer qualifier declaration");
  }
  return quals;
}

void TypeParserParsePointer(TypeParser* parser) {
  TypeParserSkipAttributes(parser);
  if (CompilerIsCXX() && LexMatch(parser->lex, TOK(ellipsis))) {
    parser->declarator_is_parameter_pack = true;
    parser->declarator_ellipsis_count++;
    TypeParserParsePointer(parser);
  } else if (LexMatch(parser->lex, TOK(star))) {
    Qualifiers quals = ParseQualifiers(parser);
    TypeParserParsePointer(parser);
    TypeRecord* p = NewPointerTypeRecord(quals);
    VectorAppend(&parser->stack, p);
  } else if (CompilerIsCXX() &&
             (LexLookingAt(parser->lex, TOK(amp)) ||
              LexLookingAt(parser->lex, TOK(ampamp)))) {
    bool rvalue = LexMatch(parser->lex, TOK(ampamp));
    if (!rvalue) {
      LexMatch(parser->lex, TOK(amp));
    }
    TypeParserParsePointer(parser);
    TypeRecord* p = NewReferenceTypeRecord(kQualPlain, rvalue);
    VectorAppend(&parser->stack, p);
  } else {
    TypeParserParseFuncOrArray(parser);
  }
}

// Check that a formal argument name is not already in the list
// of formals.  Returns true if name is OK.
static bool CheckFormalName(Vector* formals, String* name) {
  if (name->length == 0) {
    // Empty name is OK.
    return true;
  }
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = (Symbol*)formals->value.p[i];
    if (StringEqualString(&formal->name, name)) {
      return false;
    }
  }
  return true;
}

static bool TemplateArgumentContainsCurrentParameterPack(TypeParser* parser,
                                                         TemplateArgument* arg);

/* True if `type` references the parameter pack of the template currently being
 * parsed. Used to decide whether a `pattern...` is a valid pack expansion in
 * the enclosing template declaration. */
static bool TypeContainsCurrentParameterPack(TypeParser* parser,
                                             TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    int index = -1;
    if (TypeIsTemplateParameterPlaceholder(t, &index) &&
        CurrentTemplateParameterIsPack(parser->syntax, index)) {
      return true;
    }
    if (t->declarator == kDeclArray &&
        CurrentTemplateParameterIsPack(
            parser->syntax, t->info.array.template_parameter_index)) {
      return true;
    }
    if (t->template_arguments != NULL) {
      for (size_t i = 0; i < t->template_arguments->length; i++) {
        if (TemplateArgumentContainsCurrentParameterPack(
                parser, t->template_arguments->value.p[i])) {
          return true;
        }
      }
    }
    if (TypeIsFunction(t)) {
      for (size_t i = 0; i < t->info.function.prototype.length; i++) {
        Symbol* formal = t->info.function.prototype.value.p[i];
        if (formal != NULL &&
            TypeContainsCurrentParameterPack(parser, formal->type)) {
          return true;
        }
      }
    }
  }
  return false;
}

/* True if a template argument references the parameter pack of the template
 * currently being parsed (see TypeContainsCurrentParameterPack). */
static bool TemplateArgumentContainsCurrentParameterPack(TypeParser* parser,
                                                         TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType &&
      CurrentTemplateParameterIsPack(parser->syntax,
                                     arg->template_parameter_index)) {
    return true;
  }
  if (arg->pack_arguments != NULL) {
    for (size_t i = 0; i < arg->pack_arguments->length; i++) {
      if (TemplateArgumentContainsCurrentParameterPack(
              parser, arg->pack_arguments->value.p[i])) {
        return true;
      }
    }
  }
  return TypeContainsCurrentParameterPack(parser, arg->type);
}

static void MarkNonCommaVariadicEllipsis(TypeParser* parser,
                                         TypeRecord* func) {
  func->info.function.varargs = true;
  if (CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SyntaxWarning(parser->syntax, "deprecated-declarations",
                  "non-comma-separated ellipsis parameters are deprecated "
                  "in C++26");
  }
}

// Parse a formal argument declaration.  Takes ownership of
// formal.
static void ParseFormalArgument(TypeParser* proto_parser,
                                TypeRecord* func,
                                Symbol* formal,
                                int arg_number,
                                bool abbreviated_parameter) {
  if (formal->flags.is_parameter_pack) {
    bool expands_template_pack =
        abbreviated_parameter ||
        TypeContainsCurrentParameterPack(proto_parser, formal->type);
    bool has_noncomma_ellipsis =
        (!expands_template_pack && formal->flags.invented) ||
        proto_parser->declarator_ellipsis_count > 1;
    if (has_noncomma_ellipsis && CompilerIsCXX()) {
      MarkNonCommaVariadicEllipsis(proto_parser, func);
      // With one ellipsis and no template pack this is an ordinary named
      // parameter followed by a C-style ellipsis parameter, not a function
      // parameter pack.  With two ellipses the first expands the template pack
      // and the second is the variadic ellipsis.
      if (!expands_template_pack) {
        formal->flags.is_parameter_pack = false;
      }
    } else if (!expands_template_pack) {
      SyntaxError(proto_parser->syntax,
                  "function parameter pack requires a template parameter pack");
    }
  }
  if (CheckFormalName(&func->info.function.prototype, &formal->name)) {
    // Function arguments are pointer to functions.
    if (TypeIsFunction(formal->type)) {
      TypeRecord* func_ptr = NewPointerTypeRecord(kQualPlain);
      TypeRecordChain(func_ptr, formal->type);
      SymbolSetType(formal, func_ptr);
    } else if (TypeIsArray(formal->type)) {
      // For an array, convert the type record to a pointer.
      TypeRecord* ptr = TypeRecordCopy(formal->type);
      ptr->declarator = kDeclPointer;
      ptr->size = compiler->pointer_size;
      SymbolSetType(formal, ptr);
    }
    VectorAppend(&func->info.function.prototype, formal);
    formal->flags.is_defined = true;
    formal->flags.is_argument = true;
    formal->value.arg_number = arg_number;
    SyntaxApplyDeclarationAttributes(proto_parser->syntax, formal);
    if (proto_parser->syntax->local_symbol_stack != NULL) {
      InsertLocalSymbol(proto_parser->syntax->local_symbol_stack,
                      formal);
    }

  } else {
    SyntaxError(proto_parser->syntax, "Duplicate function argument '%s'",
                formal->name.value);
    SymbolDelete(formal);
  }
}

static TemplateArgument* NewTemplateParameterTypeArgumentForType(int index,
                                                                 TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  memset(arg, 0, sizeof(*arg));
  arg->kind = kTemplateParameterType;
  arg->type = type != NULL ? TypeRecordCopy(type) : NULL;
  arg->template_parameter_index = index;
  arg->location = SOURCE_LOCATION_MISSING;
  return arg;
}

static TemplateParameter* NewAbbreviatedTypeTemplateParameter(TypeParser* parser,
                                                             int index) {
  TemplateParameter* param = malloc(sizeof(TemplateParameter));
  memset(param, 0, sizeof(*param));
  StringInit(&param->name, SyntaxFakeName(parser->syntax));
  param->kind = kTemplateParameterType;
  param->index = index;
  return param;
}

static void AddFunctionAssociatedConstraint(TypeRecord* func,
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

static void ParseCXXTrailingRequiresClause(TypeParser* parser,
                                           TypeRecord* func) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(parser->lex, TOK(requires))) {
    return;
  }
  SyntaxOpenScope(parser->syntax);
  if (parser->cxx_member_owner != NULL) {
    SyntaxInsertClassMembersForConstraint(parser->syntax,
                                          parser->cxx_member_owner);
  }
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->name.length > 0) {
      InsertLocalSymbol(parser->syntax->local_symbol_stack, formal);
    }
  }
  ConstraintExpr* constraint = ConceptsParseRequiresClause(parser->syntax);
  SyntaxCloseScope(parser->syntax);
  AddFunctionAssociatedConstraint(func, constraint);
}

static bool ParseAbbreviatedFunctionParameter(TypeParser* proto_parser,
                                              TypeRecord* func,
                                              int arg_number) {
  Syntax* syntax = proto_parser->syntax;
  Lex* lex = proto_parser->lex;
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    return false;
  }

  // A parameter-type-specifier of the form `const auto&` / `volatile auto`
  // begins with cv-qualifiers before the `auto` (or `Concept auto`) placeholder.
  // Consume them speculatively; if no placeholder follows, rewind so the normal
  // (non-abbreviated) parameter path re-parses from the qualifier.
  LexCheckpoint cv_checkpoint;
  LexCheckpointSave(lex, &cv_checkpoint);
  int placeholder_qualifiers = kQualPlain;
  while (true) {
    if (LexLookingAt(lex, TOK(const))) {
      placeholder_qualifiers |= kQualConst;
      LexNextToken(lex);
    } else if (LexLookingAt(lex, TOK(volatile))) {
      placeholder_qualifiers |= kQualVolatile;
      LexNextToken(lex);
    } else {
      break;
    }
  }

  Symbol* concept_symbol = NULL;
  SourceLocation constraint_location = lex->current_token_location;
  Vector* concept_arguments = NULL;
  if (LexLookingAt(lex, TOK(identifier))) {
    String concept_name;
    StringInit(&concept_name, lex->spelling.value);
    Symbol* found = SyntaxFindSymbol(syntax, &concept_name);
    StringDestruct(&concept_name);
    if (found != NULL && found->flags.is_concept) {
      LexCheckpoint checkpoint;
      LexCheckpointSave(lex, &checkpoint);
      LexNextToken(lex);
      if (LexLookingAt(lex, TOK(less))) {
        concept_arguments =
            SyntaxParseTemplateArgumentList(syntax, TC(closebra));
      } else {
        concept_arguments = NewVector();
      }
      if (LexLookingAt(lex, TOK(auto))) {
        concept_symbol = found;
      } else {
        if (concept_arguments != NULL) {
          VectorDeleteWithContents(
              concept_arguments, (VectorElementDestructor)TemplateArgumentDelete,
              /*free_element=*/false);
          concept_arguments = NULL;
        }
        LexCheckpointRestore(lex, &checkpoint);
      }
      LexCheckpointDestruct(&checkpoint);
    }
  }

  if (concept_symbol == NULL && !LexLookingAt(lex, TOK(auto))) {
    LexCheckpointRestore(lex, &cv_checkpoint);
    LexCheckpointDestruct(&cv_checkpoint);
    return false;
  }
  LexCheckpointDestruct(&cv_checkpoint);
  LexNextToken(lex);  // auto

  int index = syntax->current_template_parameter_count +
              (int)func->info.function.template_parameters.length;
  TypeRecord* placeholder =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, placeholder_qualifiers);
  placeholder->template_parameter_index = index;
  placeholder->template_parameter_name = NewString("auto");
  Symbol* formal = TypeParserParseDeclarator(proto_parser, placeholder);
  assert(formal != NULL);
  if (formal->name.length != 0) {
    StringSet(placeholder->template_parameter_name, formal->name.value);
  }
  ParseFormalArgument(proto_parser, func, formal, arg_number,
                      /*abbreviated_parameter=*/true);

  VectorAppend(&func->info.function.template_parameters,
               NewAbbreviatedTypeTemplateParameter(proto_parser, index));
  if (concept_symbol != NULL) {
    if (concept_arguments == NULL) {
      concept_arguments = NewVector();
    }
    TemplateArgument* constrained_arg =
        NewTemplateParameterTypeArgumentForType(index, placeholder);
    if (concept_arguments->length == 0) {
      VectorAppend(concept_arguments, constrained_arg);
    } else {
      VectorInsertBefore(concept_arguments, 0, constrained_arg);
    }
    AddFunctionAssociatedConstraint(
        func, NewConceptIdConstraint(concept_symbol, concept_arguments,
                                     constraint_location));
  }
  func->info.function.template_parameter_count =
      (int)func->info.function.template_parameters.length;
  TypeRecordDelete(placeholder);
  return true;
}


// Prototype style.  C still allows old-style K&R code.
typedef enum  {
  kStyleUnknown,
  kStyleOld,
  kStyleNew
} PrototypeStyle;

static PrototypeStyle ParseFunctionParameter(TypeParser* proto_parser,
                                             TypeRecord* func,
                                             PrototypeStyle style,
                                             int arg_number,
                                             bool* seen_default_argument) {
  TypeParserSkipAttributes(proto_parser);
  if (LexLookingAt(proto_parser->lex, TOK(thread)) ||
      LexLookingAt(proto_parser->lex, TOK(thread_local))) {
    const char* keyword =
        LexLookingAt(proto_parser->lex, TOK(thread_local)) ? "thread_local"
                                                           : "__thread";
    SyntaxError(proto_parser->syntax, "Illegal use of %s", keyword);
    LexNextToken(proto_parser->lex);
  }
  bool explicit_object_parameter =
      CompilerIsCXX() && LexMatch(proto_parser->lex, TOK(this));
  if (explicit_object_parameter) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX23)) {
      SyntaxError(proto_parser->syntax,
                  "explicit object parameters require C++23");
    }
    if (arg_number != 0) {
      SyntaxError(proto_parser->syntax,
                  "explicit object parameter must be the first parameter");
    }
    if (proto_parser->cxx_member_owner == NULL) {
      SyntaxError(proto_parser->syntax,
                  "explicit object parameter is only allowed in a member function");
    }
    func->info.function.has_explicit_object_parameter = true;
  }
  if (ParseAbbreviatedFunctionParameter(proto_parser, func, arg_number)) {
    if (style == kStyleUnknown) {
      style = kStyleNew;
    }
    if (style == kStyleOld) {
      SyntaxError(proto_parser->syntax,
                  "Cannot mix function prototype with old-style function args");
    }
    Symbol* formal = func->info.function.prototype.value.p[
        func->info.function.prototype.length - 1];
    if (explicit_object_parameter && formal->flags.is_parameter_pack) {
      SyntaxError(proto_parser->syntax,
                  "explicit object parameter cannot be a parameter pack");
    }
    if (CompilerIsCXX() && LexMatch(proto_parser->lex, TOK(equal))) {
      if (explicit_object_parameter) {
        SyntaxError(proto_parser->syntax,
                    "explicit object parameter cannot have a default argument");
      }
      if (LexMatch(proto_parser->lex, TOK(lbrace))) {
        formal->default_argument =
            SyntaxParseBracedInitializer(proto_parser->syntax);
      } else {
        formal->default_argument =
            SyntaxParseSingleExpression(proto_parser->syntax,
                                        TC(closebra) | TC(exprsep));
      }
      *seen_default_argument = true;
    }
  } else if (proto_parser->found_void ||
        SyntaxLookingAtType(proto_parser->syntax)) {
    TypeRecord* type = TypeParserParseType(proto_parser, true);
    if (style == kStyleUnknown) {
      style = kStyleNew;
    }
    if (style == kStyleOld) {
      SyntaxError(proto_parser->syntax,
                  "Cannot mix function prototype with old-style function args");
    }

    Symbol* formal = TypeParserParseDeclarator(proto_parser, type);
    assert(formal != NULL);
    TypeParserSkipAttributes(proto_parser);
    for (size_t attr_index = 0;
         attr_index < proto_parser->pending_declaration_attributes.length;
         attr_index++) {
      VectorAppend(
          &formal->attributes,
          proto_parser->pending_declaration_attributes.value.p[attr_index]);
    }
    VectorClear(&proto_parser->pending_declaration_attributes);
    ParseFormalArgument(proto_parser, func, formal, arg_number,
                        /*abbreviated_parameter=*/false);
    if (explicit_object_parameter && formal->flags.is_parameter_pack) {
      SyntaxError(proto_parser->syntax,
                  "explicit object parameter cannot be a parameter pack");
    }
    if (CompilerIsCXX() && LexMatch(proto_parser->lex, TOK(equal))) {
      if (explicit_object_parameter) {
        SyntaxError(proto_parser->syntax,
                    "explicit object parameter cannot have a default argument");
      }
      if (formal->flags.is_parameter_pack) {
        SyntaxError(proto_parser->syntax,
                    "function parameter pack cannot have a default argument");
      }
      if (LexMatch(proto_parser->lex, TOK(lbrace))) {
        formal->default_argument =
            SyntaxParseBracedInitializer(proto_parser->syntax);
      } else {
        formal->default_argument =
            SyntaxParseSingleExpression(proto_parser->syntax,
                                        TC(closebra) | TC(exprsep));
      }
      *seen_default_argument = true;
    } else if (*seen_default_argument && !formal->flags.is_parameter_pack) {
      SyntaxError(proto_parser->syntax,
                  "parameter after default argument must have a default argument");
    }
  } else {
    // Possible old-style function decl, identifiers only.
    if (LexLookingAt(proto_parser->lex, TOK(identifier))) {
      if (CompilerCAtLeast(kLanguageStandardC23)) {
        SyntaxError(proto_parser->syntax,
                    "identifier-list function declarators are not allowed "
                    "in C23");
      }
      if (style == kStyleUnknown) {
        style = kStyleOld;
      }
      if (style == kStyleNew) {
        SyntaxError(proto_parser->syntax, "Type expected for function arg");
        SyntaxRecover(proto_parser->syntax, TC(closebra));
      } else {
        TypeRecord* unknown = NewTypeRecordWithSize(kTypeInt, kQualPlain);
        Symbol* formal = NewSymbol(proto_parser->lex->spelling.value,
                                   unknown, STO(auto));
        LexNextToken(proto_parser->lex);
        ParseFormalArgument(proto_parser, func, formal, arg_number,
                            /*abbreviated_parameter=*/false);
      }
    } else {
      SyntaxError(proto_parser->syntax,
                  "Expected type or identifier in function prototype");
      SyntaxRecover(proto_parser->syntax, TC(closebra));
    }
  }
  return style;
}

// Parse a function prototype, old or new style.
void ParseFunctionPrototype(TypeParser* proto_parser, TypeRecord* func) {
  bool void_args = false;
  FunctionInfo* info = &func->info.function;
  PrototypeStyle style = kStyleUnknown;
  int arg_number = 0;
  bool seen_default_argument = false;
  
  info->old_style = false;

  while (!LexLookingAt(proto_parser->lex, TOK(rparen))) {
    if (LexMatch(proto_parser->lex, TOK(ellipsis))) {
      if (arg_number == 0 && !CompilerIsCXX() &&
          !CompilerCAtLeast(kLanguageStandardC23)) {
        SyntaxError(proto_parser->syntax,
                    "a parameter list consisting only of ... requires C23");
      }
      // ... must be the last argument in the prototype.
      info->varargs = true;
      if (!LexLookingAt(proto_parser->lex, TOK(rparen))) {
        SyntaxError(proto_parser->syntax,
                    "... must be at the end of a function prototype");
        SyntaxRecover(proto_parser->syntax, TC(closebra));
      }
      break;
    }
    // Check for (void).
    if (arg_number == 0 && LexMatch(proto_parser->lex, TOK(void))) {
      proto_parser->found_void = true;
      // Look for close paren; meaning (void).
      if (LexLookingAt(proto_parser->lex, TOK(rparen))) {
        // "void" means that there are no arguments.
        void_args = true;
        break;
      }
    }
    
    // The keyword 'register' is allowed here but we ignore it, except
    // to set the type as new style.
    if (LexMatch(proto_parser->lex, TOK(register))) {
      style = kStyleNew;
    }
    
    // Parse the formal argument's type, if it has one.
    // Otherwise it's a possible old-style function.
    style = ParseFunctionParameter(proto_parser, func, style, arg_number,
                                   &seen_default_argument);
    arg_number++;
    if (CompilerIsCXX() && LexMatch(proto_parser->lex, TOK(ellipsis))) {
      MarkNonCommaVariadicEllipsis(proto_parser, func);
      if (!LexLookingAt(proto_parser->lex, TOK(rparen))) {
        SyntaxError(proto_parser->syntax,
                    "... must be at the end of a function prototype");
        SyntaxRecover(proto_parser->syntax, TC(closebra));
      }
      break;
    }
    if (!LexMatch(proto_parser->lex, TOK(comma))) {
      break;
    }
  }
  if (style == kStyleOld) {
    info->old_style = true;
  }

  // If we were not told (void) and there are no formal args then the C
  // language says that this is a variable arguments function.
  if (!CompilerIsCXX() && !CompilerCAtLeast(kLanguageStandardC23) &&
      info->prototype.length == 0 && !void_args) {
    info->unknown_args = true;
    SyntaxWarning(proto_parser->syntax, "strict-prototypes",
                  "function declaration without a prototype");
  }
}

// Parse an optional C++ exception specification (`noexcept`, `noexcept(expr)`,
// `throw()` or `throw(types)`) and record whether the function is guaranteed
// non-throwing on `func`.  `noexcept` with no operand and the deprecated empty
// `throw()` mean non-throwing; `noexcept(expr)` evaluates `expr` as a constant
// boolean; a dynamic `throw(types)` specification is treated as throwing.
void ParseCXXExceptionSpecifier(TypeParser* parser, TypeRecord* func) {
  if (!CompilerIsCXX()) {
    return;
  }
  if (LexMatch(parser->lex, TOK(noexcept))) {
    bool is_noexcept = true;
    if (LexMatch(parser->lex, TOK(lparen))) {
      if (parser->deferred_noexcept_specifiers != NULL) {
        DeferredNoexceptSpecifier* deferred =
            malloc(sizeof(DeferredNoexceptSpecifier));
        deferred->member_symbol = parser->symbol;
        deferred->function_type = func;
        LexCheckpointSave(parser->lex, &deferred->expression_checkpoint);
        VectorAppend(parser->deferred_noexcept_specifiers, deferred);
        int depth = 0;
        while (!LexEof(parser->lex)) {
          if (LexLookingAt(parser->lex, TOK(rparen)) && depth == 0) {
            break;
          }
          if (LexLookingAt(parser->lex, TOK(lparen))) {
            depth++;
          } else if (LexLookingAt(parser->lex, TOK(rparen))) {
            depth--;
          }
          LexNextToken(parser->lex);
        }
        SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
        if (func != NULL) {
          func->info.function.is_noexcept = false;
        }
        return;
      }
      ASTNode* expr =
          SyntaxParseSingleExpression(parser->syntax, TC(exprsep));
      expr = AnalyzeExpression(expr);
      int64_t value = 1;
      if (!EvaluateIntegerExpression(expr, &value)) {
        // A dependent noexcept(expr) in a template can only be resolved at
        // instantiation time; conservatively treat it as possibly-throwing
        // rather than erroring or enforcing here.
        if (parser->syntax->parsing_template_declaration) {
          value = 0;
        } else {
          SyntaxError(parser->syntax,
                      "noexcept specifier is not a constant expression");
        }
      }
      ASTNodeDelete(expr);
      SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
      is_noexcept = value != 0;
    }
    if (func != NULL) {
      func->info.function.is_noexcept = is_noexcept;
    }
    return;
  }
  if (LexMatch(parser->lex, TOK(throw))) {
    if (LexLookingAt(parser->lex, TOK(lparen))) {
      // `throw()` is the deprecated non-throwing spec; `throw(types)` is a
      // dynamic specification that permits throwing the listed types.
      LexNextToken(parser->lex);  // Consume '('.
      bool is_empty = LexLookingAt(parser->lex, TOK(rparen));
      int depth = 1;
      while (depth > 0 && !LexEof(parser->lex)) {
        if (LexMatch(parser->lex, TOK(lparen))) {
          depth++;
        } else if (LexMatch(parser->lex, TOK(rparen))) {
          depth--;
        } else {
          LexNextToken(parser->lex);
        }
      }
      if (func != NULL) {
        func->info.function.is_noexcept = is_empty;
      }
    } else {
      SyntaxError(parser->syntax, "Expected exception specification");
    }
  }
}

static TypeRecord* ParseCXXTrailingReturnType(TypeParser* parser) {
  TypeParser return_parser;
  TypeParserInit(&return_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  TypeRecord* return_type = TypeParserParseType(&return_parser, true);
  Symbol* parsed = return_type != NULL
                       ? TypeParserParseDeclarator(&return_parser, return_type)
                       : NULL;
  TypeParserDestruct(&return_parser);
  if (parsed != NULL) {
    TypeRecord* result = TypeRecordCopy(parsed->type);
    SymbolDelete(parsed);
    TypeRecordDelete(return_type);
    return result;
  }
  if (return_type == NULL) {
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  return return_type;
}

CXXRefQualifier ParseCXXRefQualifier(TypeParser* parser) {
  if (!CompilerIsCXX()) {
    return kCXXRefQualifierNone;
  }
  if (LexMatch(parser->lex, TOK(ampamp))) {
    return kCXXRefQualifierRValue;
  }
  if (LexMatch(parser->lex, TOK(amp))) {
    return kCXXRefQualifierLValue;
  }
  return kCXXRefQualifierNone;
}

static void ParseFunctionDecl(TypeParser* parser) {
  TypeParser proto_parser;
  TypeParserInit(&proto_parser, parser->lex, parser->syntax, STO(auto), kParsingPrototype);
  // Propagate the enclosing class so a parameter type can name the class
  // currently being defined, including via its own template-id (e.g. a member
  // or friend declared as 'f(const Box<T>&)' inside 'template<class T> Box').
  proto_parser.cxx_member_owner = parser->cxx_member_owner;
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.is_inline =
      parser->is_inline ||
      (CompilerIsCXX() && (parser->is_constexpr || parser->is_consteval));
  func->info.function.is_constexpr = parser->is_constexpr;
  func->info.function.is_consteval = parser->is_consteval;
  if (parser->symbol != NULL) {
    func->info.function.symbol = parser->symbol;
  }

  // A function's parameters have function-prototype scope ([basic.scope.param]):
  // they are visible to later parameters' default arguments, a trailing return
  // type, and a trailing requires-clause, but must NOT leak into the enclosing
  // scope once the declarator is parsed.  Open a dedicated scope around the
  // prototype so the parameter names are torn down here.  Without it, a member
  // function's parameters stay visible in the surrounding class (or, for a
  // class template, the template-parameter) scope and can be found by unqualified
  // lookup inside a sibling member's body -- wrongly shadowing a data member of
  // the same name.  The definition body re-introduces the parameters through its
  // own scope (AddFunctionScopeSymbols / AddInlineFunctionScopeSymbols).
  SyntaxOpenScope(parser->syntax);
  ParseFunctionPrototype(&proto_parser, func);
  SyntaxNeedBracket(parser->syntax, TOK(rparen), TC(exprsep));
  while (LexLookingAt(parser->lex, TOK(const)) ||
         LexLookingAt(parser->lex, TOK(volatile))) {
    if (LexMatch(parser->lex, TOK(const))) {
      func->info.function.is_const_member = true;
    } else if (LexMatch(parser->lex, TOK(volatile))) {
      func->info.function.is_volatile_member = true;
    }
  }
  func->info.function.ref_qualifier = ParseCXXRefQualifier(parser);
  if (func->info.function.has_explicit_object_parameter &&
      (func->info.function.is_const_member ||
       func->info.function.is_volatile_member ||
       func->info.function.ref_qualifier != kCXXRefQualifierNone)) {
    SyntaxError(parser->syntax,
                "explicit object member function cannot have cv or ref qualifiers");
  }
  ParseCXXExceptionSpecifier(parser, func);
  if (CompilerIsCXX() && TypeContainsAuto(parser->base_type) &&
      LexMatch(parser->lex, TOK(arrow))) {
    TypeRecord* trailing_return = ParseCXXTrailingReturnType(parser);
    TypeRecordChain(func, trailing_return);
  }
  ParseCXXTrailingRequiresClause(parser, func);
  SyntaxParseFunctionContracts(parser->syntax, func,
                               parser->cxx_member_owner,
                               parser->cxx_member_owner != NULL &&
                                   !StorageIs(parser->storage, STO(static)));
  if (func->info.function.contract_assertions.length != 0 &&
      (parser->stack.length != 0 ||
       StorageIs(parser->storage, STO(typedef)))) {
    SyntaxError(parser->syntax,
                "function contract specifiers cannot be associated with a "
                "function pointer or function type alias");
  }
  if (parser->cxx_member_definition != NULL &&
      !parser->cxx_member_definition->is_static &&
      !func->info.function.has_explicit_object_parameter &&
      !FunctionHasImplicitThisParameter(func)) {
    TypeRecordAddCXXThisParameter(
        func, parser->cxx_member_owner, parser->symbol->location);
  } else if (parser->cxx_member_definition != NULL) {
    func->info.function.cxx_member_owner = parser->cxx_member_owner;
  }
  SyntaxCloseScope(parser->syntax);

  VectorAppend(&parser->stack, func);
  TypeParserDestruct(&proto_parser);
}
 
static void ParseArrayDecl(TypeParser* parser) {
  bool is_static = false;
  Qualifiers quals = kQualPlain;
  // An array decl can have:
  // [static quals ...]
  // [quals static ...];
  if (LexMatch(parser->lex, TOK(static))) {
    is_static = true;
    quals = ParseQualifiers(parser);
  } else {
    quals = ParseQualifiers(parser);
    is_static = LexMatch(parser->lex, TOK(static));
  }

  // These are only allowed inside a function prototype.
  if (parser->context != kParsingPrototype) {
    if (is_static || quals != kQualPlain) {
      SyntaxError(parser->syntax, "static or qualifiers used in array declarator outside function prototype");
    }
  }
  
  parser->dimension_count++;
  TypeRecord* p = NewArrayTypeRecord(quals, is_static);
  VectorAppend(&parser->stack, p);

  bool is_vla = false;
  bool found_star = false;
  SourceLocation location = parser->lex->current_token_location;
  if (LexMatch(parser->lex, TOK(rsquare))) {
    if (parser->dimension_count != 1) {
      SyntaxError(parser->syntax,
             "Array dimension required after first dimension");
    }
    // No size expression present.
    if (parser->context != kParsingPrototype) {
      p->info.array.is_flexible = true;
    }
    return;
  }
  
  // There is something in the [...]
  if (LexMatch(parser->lex, TOK(star))) {
    // Unfortunately * can be a unary operator and part of an expression
    // so we need to look at the next token to see if it's a close square
    // bracket.  We've already consumed the *.
    found_star = true;
  }
  if (found_star && LexLookingAt(parser->lex, TOK(rsquare))) {
    if (parser->context != kParsingPrototype) {
      SyntaxError(parser->syntax, "VLA placeholder '*' is only valid in a function prototype");
    } else {
      is_vla = true;
      p->info.array.is_placeholder_vla = true;
    }
  } else {
    // Size expression is present.  If it's constant we have a
    // regular fixed size array, otherwise it's a VLA.
    ASTNode* size_expr =
        SyntaxParseSingleExpression(parser->syntax, TC(closebra));
    if (found_star) {
      // There was a * before the expression, this means contents.
      size_expr = NewUnaryASTNode(AST_OP(contents), NULL, location, size_expr);
    }
    int sizeof_pack_index = -1;
    if (CompilerIsCXX() && ASTNodeGetShape(size_expr) == kASTShapeSizeof &&
        ((SizeofASTNode*)size_expr)->is_pack_size) {
      ASTNode* pack_expr = ((SizeofASTNode*)size_expr)->expr;
      if (pack_expr != NULL && pack_expr->op == AST_OP(identifier)) {
        Symbol* pack_symbol = ((IdentifierASTNode*)pack_expr)->symbol;
        if (pack_symbol != NULL && pack_symbol->flags.is_parameter_pack) {
          sizeof_pack_index = pack_symbol->template_parameter_index;
        }
      }
    }
    size_expr = AnalyzeExpression(size_expr);
    bool delete_expr = true;
    int64_t size;
    if (sizeof_pack_index >= 0) {
      p->info.array.template_parameter_index = sizeof_pack_index;
      // Keep one placeholder slot so the unexpanded pack initializer is
      // accepted while parsing the template definition. Substitution replaces
      // this with the concrete pack length before layout/code generation.
      p->info.array.size.fixed = 1;
      goto parsed_bound;
    }
    if (parser->syntax->parsing_template_declaration &&
        ExpressionIsTemplateDependent(size_expr)) {
      if (size_expr->op == AST_OP(identifier)) {
        IdentifierASTNode* id = (IdentifierASTNode*)size_expr;
        if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
            !id->symbol->flags.is_template_type_parameter) {
          p->info.array.template_parameter_index =
              id->symbol->template_parameter_index;
          p->info.array.size.fixed = 0;
          goto parsed_bound;
        }
      }
      // A dependent expression can carry a placeholder constant value after
      // semantic analysis (for example, sizeof(T) uses the template
      // parameter's provisional size).  Preserve the expression before
      // constant evaluation so instantiation can compute the real bound.
      p->info.array.size.vla.size = size_expr;
      p->info.array.is_dependent_bound = true;
      delete_expr = false;
      goto parsed_bound;
    }
    bool ok = EvaluateIntegerExpression(size_expr, &size);
    if (!ok) {
      // VLA.
      if (!TypeIsIntegral(size_expr->type)) {
        SyntaxError(parser->syntax, "Variable length array size must be integral");
      }
      if (parser->context != kParsingBlockScope && parser->context != kParsingPrototype) {
        SyntaxError(parser->syntax, "Variable length array is only allowed inside a function");
      } else if (StorageIs(parser->storage, STO(extern)|STO(static))) {
        SyntaxError(parser->syntax, "Variable length array cannot be static or extern");
      } else {
        p->info.array.size.vla.size = size_expr;
        delete_expr = false;      // Hold on to expression.
        is_vla = true;
      }
    } else {
      if (size < 0) {
        SyntaxError(parser->syntax, "Array with negative size");
        size = 1;
      }
      // A zero-length array (int r[0]) is a GCC extension, commonly used at
      // the end of a struct like a flexible array member.
      p->info.array.size.fixed = (int)size;
    }
parsed_bound:
    if (delete_expr) {
      ASTNodeDelete(size_expr);
    }
  }
  p->info.array.is_vla = is_vla;
  
  if (!LexMatch(parser->lex, TOK(rsquare))) {
    LexError(parser->lex, "Missing ]");
  }
}

static bool CXXDirectInitializerAfterDeclarator(TypeParser* parser) {
  if (!CompilerIsCXX() || parser->symbol == NULL ||
      parser->stack.length != 0 ||
      !LexLookingAt(parser->lex, TOK(lparen))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(parser->lex, &checkpoint);
  LexNextToken(parser->lex);
  bool starts_with_expression =
      !LexLookingAt(parser->lex, TOK(rparen)) &&
      !LexLookingAt(parser->lex, TOK(ellipsis)) &&
      !LexLookingAt(parser->lex, TOK(this)) &&
      !LexLookingAt(parser->lex, TOK(thread_local)) &&
      !SyntaxLookingAtCXXAttribute(parser->syntax) &&
      !SyntaxLookingAtType(parser->syntax);
  LexCheckpointRestore(parser->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  if (starts_with_expression) {
    return true;
  }
  if (!TypeIsStructOrUnion(parser->base_type)) {
    return false;
  }
  if (parser->context == kParsingBlockScope) {
    return true;
  }
  if (parser->context != kParsingFileScope) {
    return false;
  }
  LexCheckpoint file_checkpoint;
  LexCheckpointSave(parser->lex, &file_checkpoint);
  LexNextToken(parser->lex);
  bool parameter_attribute =
      SyntaxLookingAtCXXAttribute(parser->syntax);
  bool direct_initializer =
      !LexLookingAt(parser->lex, TOK(rparen)) &&
      !SyntaxLookingAtType(parser->syntax) && !parameter_attribute;
  LexCheckpointRestore(parser->lex, &file_checkpoint);
  LexCheckpointDestruct(&file_checkpoint);
  return direct_initializer;
}

void TypeParserParseFuncOrArray(TypeParser* parser) {
  TypeParserParseBase(parser);
  while (parser->syntax->found_open_paren ||
         LexLookingAt(parser->lex, TOK(lparen)) ||
         LexLookingAt(parser->lex, TOK(lsquare))) {
    if (SyntaxLookingAtCXXAttribute(parser->syntax)) {
      break;
    }
    if (CXXDirectInitializerAfterDeclarator(parser)) {
      // In `T obj(args);`, the parens are direct initialization of `obj`,
      // not a function declarator. At namespace scope this is limited to
      // constexpr/constinit objects so ordinary declarations keep the old path.
      break;
    }
    // Check for function prototype declaration.
    if (parser->syntax->found_open_paren || LexMatch(parser->lex, TOK(lparen))) {
      parser->syntax->found_open_paren = false;
      ParseFunctionDecl(parser);
    } else if (LexMatch(parser->lex, TOK(lsquare))) {
      ParseArrayDecl(parser);
    }
  }
}


static void ResolveQualifiedMemberDeclarator(TypeParser* parser,
                                             FullyQualifiedIdentifier* name) {
  if (!name->is_qualified || name->components.length < 2) {
    return;
  }

  Symbol* owner = SyntaxFindQualifiedPrefixSymbol(
      parser->syntax, name, name->components.length - 1);
  if (owner == NULL || owner->type == NULL ||
      !TypeIsStructOrUnion(owner->type) ||
      owner->type->info.struct_info == NULL) {
    SyntaxError(parser->syntax, "Qualified declarator %s does not name a class member",
                name->spelling.value);
    return;
  }

  parser->cxx_member_owner = owner->type->info.struct_info;
  String member_name;
  StringInit(&member_name, FullyQualifiedIdentifierLast(name));
  parser->cxx_member_definition =
      FindStructMember(parser->cxx_member_owner, &member_name);
  if (parser->cxx_member_definition == NULL) {
    SyntaxError(parser->syntax, "No class member named %s",
                name->spelling.value);
  }
  StringDestruct(&member_name);
}

void ConversionOperatorName(TypeRecord* type, String* name);
TypeRecord* ParseCXXConversionType(TypeParser* parser);

static void ValidateCXXLiteralOperatorDeclaration(TypeParser* parser,
                                                  Symbol* symbol,
                                                  bool in_system_header) {
  static const char prefix[] = "operator\"\"";
  size_t prefix_length = sizeof(prefix) - 1;
  if (symbol == NULL || symbol->name.length <= prefix_length ||
      !StringStartsWith(&symbol->name, prefix) ||
      symbol->name.value[prefix_length] == '_' || in_system_header) {
    return;
  }
  SyntaxError(parser->syntax,
              "Literal operator suffix \"%s\" must begin with '_' outside a "
              "system header",
              symbol->name.value + prefix_length);
}

static bool LookingAtMemberPointerDeclaratorSuffix(TypeParser* parser) {
  return SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(parser->syntax);
}

static TypeRecord* MemberPointerTypeForClassName(TypeParser* parser,
                                                 String* class_name) {
  Symbol* class_sym = SyntaxFindSymbol(parser->syntax, class_name);
  if (class_sym == NULL) {
    class_sym = SyntaxFindTag(parser->syntax, class_name);
  }
  if (class_sym != NULL && class_sym->type != NULL &&
      TypeIsStructOrUnion(class_sym->type) &&
      class_sym->type->info.struct_info != NULL) {
    return NewMemberPointerTypeRecord(class_sym->type->info.struct_info,
                                      kQualPlain);
  }
  if (class_sym != NULL && class_sym->flags.is_template_type_parameter &&
      class_sym->template_parameter_index >= 0) {
    TypeRecord* type =
        NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
    type->declarator = kDeclMemberPointer;
    type->template_parameter_index = class_sym->template_parameter_index;
    type->size = 0;
    return type;
  }
  return NULL;
}

static bool ParseMemberPointerDeclarator(TypeParser* parser) {
  if (!CompilerIsCXX()) {
    return false;
  }
  if (!LookingAtMemberPointerDeclaratorSuffix(parser)) {
    return false;
  }
  String class_name;
  StringInit(&class_name, parser->lex->spelling.value);
  LexNextToken(parser->lex);
  LexMatch(parser->lex, TOK(coloncolon));
  LexMatch(parser->lex, TOK(star));
  TypeRecord* mptr = MemberPointerTypeForClassName(parser, &class_name);
  StringDestruct(&class_name);
  if (mptr == NULL) {
    SyntaxError(parser->syntax, "Pointer-to-member requires a class type");
    return true;
  }
  VectorAppend(&parser->stack, mptr);
  if (LexLookingAt(parser->lex, TOK(identifier))) {
    SourceLocation location = parser->lex->current_token_location;
    parser->symbol =
        NewSymbol(parser->lex->spelling.value, parser->base_type, parser->storage);
    parser->symbol->location = location;
    LexNextToken(parser->lex);
  }
  return true;
}

void TypeParserParseBase(TypeParser* parser) {
  if (LexMatch(parser->lex, TOK(lparen))) {
    if (LookingAtMemberPointerDeclaratorSuffix(parser)) {
      if (ParseMemberPointerDeclarator(parser) &&
          !LexMatch(parser->lex, TOK(rparen))) {
        LexError(parser->lex, "Missing close parenthesis in declaration");
      }
      return;
    }
    if (!LookingAtMemberPointerDeclaratorSuffix(parser) &&
        (SyntaxLookingAtType(parser->syntax) ||
         LexLookingAt(parser->lex, TOK(rparen)))) {
      // Open paren followed by a type isn't a parenthesized decl, it's
      // a function prototype.
      parser->syntax->found_open_paren = true;
      return;
    }
    TypeParserParsePointer(parser);
    if (!LexMatch(parser->lex, TOK(rparen))) {
      LexError(parser->lex, "Missing close parenthesis in declaration");
    }
  } else {
    if (ParseMemberPointerDeclarator(parser)) {
      return;
    }
    if (LexLookingAt(parser->lex, TOK(identifier)) ||
        LexLookingAt(parser->lex, TOK(operator)) ||
        LexLookingAt(parser->lex, TOK(coloncolon))) {
      SourceLocation location = parser->lex->current_token_location;
      bool in_system_header = SourceIsSystemHeader(parser->lex->source);
      FullyQualifiedIdentifier name;
      FullyQualifiedIdentifierInit(&name);
      bool parsed = CompilerIsCXX()
          ? SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
                parser->syntax, &name, TC(decl))
          : SyntaxParseFullyQualifiedIdentifier(parser->syntax, &name);
      if (!parsed) {
        FullyQualifiedIdentifierDestruct(&name);
        return;
      }
      if (CompilerIsCXX() && LexMatch(parser->lex, TOK(coloncolon)) &&
          LexMatch(parser->lex, TOK(star))) {
        String class_name;
        StringInit(&class_name, FullyQualifiedIdentifierLast(&name));
        Symbol* class_sym = SyntaxFindSymbol(parser->syntax, &class_name);
        Struct* class_info = NULL;
        if (class_sym != NULL && class_sym->type != NULL &&
            TypeIsStructOrUnion(class_sym->type) &&
            class_sym->type->info.struct_info != NULL) {
          class_info = class_sym->type->info.struct_info;
        }
        StringDestruct(&class_name);
        if (class_info == NULL) {
          SyntaxError(parser->syntax,
                      "Pointer-to-member requires a class type");
        } else {
          TypeRecord* mptr =
              NewMemberPointerTypeRecord(class_info, kQualPlain);
          VectorAppend(&parser->stack, mptr);
          if (LexLookingAt(parser->lex, TOK(identifier))) {
            SourceLocation location = parser->lex->current_token_location;
            parser->symbol =
                NewSymbol(parser->lex->spelling.value, parser->base_type,
                          parser->storage);
            parser->symbol->location = location;
            LexNextToken(parser->lex);
          }
        }
        FullyQualifiedIdentifierDestruct(&name);
        return;
      }
      ResolveQualifiedMemberDeclarator(parser, &name);
      parser->symbol =
          NewSymbol(FullyQualifiedIdentifierLast(&name), parser->base_type,
                    parser->storage);
      parser->symbol->location = location;
      ValidateCXXLiteralOperatorDeclaration(parser, parser->symbol,
                                            in_system_header);
      if (name.template_arguments.length > 0) {
        Vector* args =
            name.template_arguments.value.p[name.template_arguments.length - 1];
        parser->declarator_template_arguments = TemplateArgumentVectorCopy(args);
      }
      FullyQualifiedIdentifierDestruct(&name);
    }
  }
}

void ConversionOperatorName(TypeRecord* type, String* name) {
  String type_name;
  StringInit(&type_name, "");
  TypeRecordToString(type, &type_name);
  StringInit(name, "operator ");
  for (size_t i = 0; i < type_name.length; i++) {
    char ch = type_name.value[i];
    if (ch == '*') {
      StringAppend(name, " pointer");
    } else if (ch == '&') {
      if (i + 1 < type_name.length && type_name.value[i + 1] == '&') {
        StringAppend(name, " rvalue_reference");
        i++;
      } else {
        StringAppend(name, " reference");
      }
    } else {
      StringAppendChar(name, ch);
    }
  }
  while (name->length > 0 && name->value[name->length - 1] == ' ') {
    name->value[name->length - 1] = '\0';
    name->length--;
  }
  StringDestruct(&type_name);
}

TypeRecord* ParseCXXConversionType(TypeParser* parser) {
  TypeParser return_parser;
  TypeParserInit(&return_parser, parser->lex, parser->syntax, STO(auto),
                 kParsingPrototype);
  TypeRecord* return_type = TypeParserParseType(&return_parser, true);
  TypeParserDestruct(&return_parser);
  if (return_type == NULL) {
    return_type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }

  Vector wrappers;
  VectorInit(&wrappers);
  while (!LexEof(parser->lex)) {
    if (LexMatch(parser->lex, TOK(star))) {
      VectorAppend(&wrappers, NewPointerTypeRecord(ParseQualifiers(parser)));
    } else if (LexLookingAt(parser->lex, TOK(amp)) ||
               LexLookingAt(parser->lex, TOK(ampamp))) {
      bool rvalue = LexMatch(parser->lex, TOK(ampamp));
      if (!rvalue) {
        LexMatch(parser->lex, TOK(amp));
      }
      VectorAppend(&wrappers, NewReferenceTypeRecord(kQualPlain, rvalue));
    } else {
      break;
    }
  }

  TypeRecord* result = return_type;
  for (size_t i = wrappers.length; i > 0; i--) {
    TypeRecord* wrapper = wrappers.value.p[i - 1];
    TypeRecordChain(wrapper, result);
    wrapper->type = result->type;
    result = wrapper;
  }
  VectorDestruct(&wrappers);
  TypeRecordCalculateSize(result);
  return result;
}
