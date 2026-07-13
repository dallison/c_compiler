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
#include <stdio.h>
#include <string.h>
#include "concepts.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "type.h"
#include "type_internal.h"
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

static Symbol* LookupSymbolInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
                                                              Namespace* ns,
                                                              String* name,
                                                              bool* ambiguous);
static Symbol* LookupTagInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
                                                           Namespace* ns,
                                                           String* name,
                                                           bool* ambiguous);
static Namespace* ResolveNamespaceChildCheckingAmbiguity(Syntax* syntax,
                                                         Namespace* parent,
                                                         String* name,
                                                         bool* ambiguous);

bool SyntaxCurrentIdentifierFollowedByScopeOperator(Syntax* syntax) {
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

bool SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(Syntax* syntax) {
  Lex* lex = syntax->lex;
  if (!CompilerIsCXX() || !LexLookingAt(lex, TOK(identifier))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(lex, &checkpoint);
  LexNextToken(lex);
  bool result = false;
  if (LexMatch(lex, TOK(coloncolon))) {
    result = LexLookingAt(lex, TOK(star));
  }
  LexCheckpointRestore(lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
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
    if (strncmp(lex->line.value + pos, "operator", 8) == 0 &&
        (pos + 8 == lex->line.length ||
         !isalnum((unsigned char)lex->line.value[pos + 8]))) {
      result = true;
      break;
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
  if (syntax->export_depth > 0) {
    symbol->flags.is_exported = true;
  }
  if (InNamedNamespace(syntax)) {
    return NamespaceInsertSymbol(syntax->current_namespace, symbol);
  }
  return InsertGlobalSymbol(symbol);
}

static bool CanOverloadFunctions(Symbol* a, Symbol* b) {
  return CompilerIsCXX() && a != NULL && b != NULL &&
         TypeIsFunction(a->type) && TypeIsFunction(b->type);
}

static Symbol* FindFunctionTemplateOverload(Symbol* first) {
  for (Symbol* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (overload->flags.is_template && overload->type != NULL &&
        TypeIsFunction(overload->type)) {
      return overload;
    }
  }
  return NULL;
}

static Symbol* FindMemberFunctionTemplateOverload(StructMember* first) {
  for (StructMember* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (overload->symbol != NULL && overload->symbol->flags.is_template &&
        overload->symbol->type != NULL && TypeIsFunction(overload->symbol->type)) {
      return overload->symbol;
    }
  }
  return NULL;
}

static StructMember* FindMemberFunctionTemplateSpecialization(
    StructMember* first, TypeRecord* type) {
  for (StructMember* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (overload->symbol == NULL || overload->symbol->flags.is_template ||
        overload->symbol->type == NULL ||
        !TypeIsFunction(overload->symbol->type) ||
        overload->symbol->type->info.function.template_origin == NULL) {
      continue;
    }
    if (TypeEqual(overload->symbol->type, type)) {
      return overload;
    }
  }
  return NULL;
}

static void AppendMemberFunctionSpecialization(Syntax* syntax,
                                               StructMember* first,
                                               StructMember* specialization) {
  StructMember* tail = first;
  while (tail->overload_next != NULL) {
    tail = tail->overload_next;
  }
  tail->overload_next = specialization;
  specialization->symbol->flags.is_overloaded = true;
  first->symbol->flags.is_overloaded = true;
  Symbol* templ = specialization->symbol->type->info.function.template_origin;
  if (templ != NULL) {
    Symbol* tail = templ;
    while (tail->overload_next != NULL) {
      tail = tail->overload_next;
    }
    tail->overload_next = specialization->symbol;
    templ->flags.is_overloaded = true;
  }
  (void)syntax;
}

static void MarkFunctionTemplateSpecialization(Syntax* syntax, Symbol* sym,
                                               Symbol* first_overload) {
  if (!syntax->parsing_template_specialization || sym == NULL ||
      sym->type == NULL || !TypeIsFunction(sym->type) ||
      sym->type->template_arguments == NULL) {
    return;
  }
  Symbol* templ = FindFunctionTemplateOverload(first_overload);
  if (templ == NULL) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(sym, &suffix);
    SyntaxError(syntax, "%s is not a function template%s",
                sym->name.value, suffix.value);
    StringDestruct(&suffix);
    return;
  }
  sym->type->info.function.template_origin = templ;
}

static void MarkMemberFunctionTemplateSpecialization(
    Syntax* syntax, Symbol* sym, StructMember* first_overload) {
  if (!syntax->parsing_template_specialization || sym == NULL ||
      sym->type == NULL || !TypeIsFunction(sym->type) ||
      sym->type->template_arguments == NULL) {
    return;
  }
  Symbol* templ = FindMemberFunctionTemplateOverload(first_overload);
  if (templ == NULL) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(sym, &suffix);
    SyntaxError(syntax, "%s is not a function template%s",
                sym->name.value, suffix.value);
    StringDestruct(&suffix);
    return;
  }
  sym->type->info.function.template_origin = templ;
}

static bool OverloadTypesEqual(TypeRecord* left, TypeRecord* right);

static bool OverloadFunctionPrototypesEqual(FunctionInfo* left,
                                            FunctionInfo* right) {
  if (left->prototype.length != right->prototype.length ||
      left->varargs != right->varargs ||
      left->is_const_member != right->is_const_member ||
      left->ref_qualifier != right->ref_qualifier) {
    return false;
  }
  for (size_t i = 0; i < left->prototype.length; i++) {
    Symbol* left_arg = left->prototype.value.p[i];
    Symbol* right_arg = right->prototype.value.p[i];
    if (left_arg == NULL || right_arg == NULL ||
        !OverloadTypesEqual(left_arg->type, right_arg->type)) {
      return false;
    }
  }
  return true;
}

static bool OverloadTypesEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator) {
    return false;
  }
  if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
    if (!TypeIsStructOrUnion(left) || !TypeIsStructOrUnion(right) ||
        left->type != right->type || left->qualifiers != right->qualifiers) {
      return false;
    }
    if (left->template_origin != NULL || right->template_origin != NULL) {
      if (left->template_origin == right->template_origin &&
          TypeTemplateArgumentVectorEqual(left->template_arguments,
                                          right->template_arguments)) {
        return true;
      }
      return left->info.struct_info == right->info.struct_info &&
             TypeTemplateArgumentVectorEqual(left->template_arguments,
                                             right->template_arguments);
    }
    return left->info.struct_info == right->info.struct_info;
  }
  switch (left->declarator) {
    case kDeclArray:
      return left->info.array.size.fixed == right->info.array.size.fixed &&
             OverloadTypesEqual(left->next, right->next);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return OverloadTypesEqual(left->next, right->next);
    case kDeclFunction:
      return OverloadTypesEqual(left->next, right->next) &&
             OverloadFunctionPrototypesEqual(&left->info.function,
                                             &right->info.function);
    case kDeclPrimitive:
      return TypeEqual(left, right);
  }
  return false;
}

static bool OverloadFunctionTypesEqual(TypeRecord* left, TypeRecord* right) {
  return TypeIsFunction(left) && TypeIsFunction(right) &&
         OverloadTypesEqual(left, right);
}

static bool RedeclarationTypesEqual(TypeRecord* left, TypeRecord* right) {
  if (TypeIsFunction(left) || TypeIsFunction(right)) {
    return OverloadFunctionTypesEqual(left, right);
  }
  return TypeEqual(left, right);
}

static bool SameSignatureTemplateConstraintsAreEquivalent(Symbol* overload,
                                                          TypeRecord* type) {
  if (overload == NULL || overload->type == NULL || type == NULL ||
      !overload->flags.is_template || !TypeIsFunction(overload->type) ||
      !TypeIsFunction(type) || type->info.function.template_parameter_count <= 0) {
    return true;
  }
  bool overload_constrained =
      ConceptsFunctionTemplateHasAssociatedConstraint(overload);
  bool type_constrained = type->info.function.associated_constraint != NULL;
  if (!overload_constrained && !type_constrained) {
    return true;
  }
  Symbol scratch = {0};
  scratch.type = type;
  return ConceptsFunctionTemplateConstraintsEquivalent(overload, &scratch);
}

static Symbol* FindMatchingOverload(Symbol* first, TypeRecord* type) {
  for (Symbol* overload = first; overload != NULL;
       overload = overload->overload_next) {
    bool overload_is_template = overload->flags.is_template;
    bool type_is_template =
        TypeIsFunction(type) && type->info.function.template_parameter_count > 0;
    if (overload_is_template != type_is_template) {
      continue;
    }
    if (OverloadFunctionTypesEqual(overload->type, type) &&
        SameSignatureTemplateConstraintsAreEquivalent(overload, type)) {
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

static bool SymbolLooksLikeFunctionTemplate(Symbol* symbol) {
  return symbol != NULL && symbol->type != NULL &&
         TypeIsFunction(symbol->type) &&
         (symbol->flags.is_template ||
          symbol->type->info.function.template_parameter_count > 0 ||
          symbol->type->info.function.template_parameters.length > 0);
}

static bool TryAppendSameSignatureConstrainedTemplateOverload(
    Symbol* first, Symbol* overload, ConstraintExpr* pending_constraint) {
  if (first == NULL || overload == NULL ||
      !SymbolLooksLikeFunctionTemplate(overload)) {
    return false;
  }
  ConstraintExpr* saved_constraint =
      overload->type->info.function.associated_constraint;
  if (saved_constraint == NULL && pending_constraint != NULL) {
    overload->type->info.function.associated_constraint = pending_constraint;
  }
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!SymbolLooksLikeFunctionTemplate(candidate) ||
        !OverloadFunctionTypesEqual(candidate->type, overload->type)) {
      continue;
    }
    bool candidate_constrained =
        ConceptsFunctionTemplateHasAssociatedConstraint(candidate);
    bool overload_constrained =
        ConceptsFunctionTemplateHasAssociatedConstraint(overload);
    if ((candidate_constrained || overload_constrained) &&
        !ConceptsFunctionTemplateConstraintsEquivalent(candidate, overload)) {
      overload->type->info.function.associated_constraint = saved_constraint;
      AppendOverload(first, overload);
      return true;
    }
  }
  overload->type->info.function.associated_constraint = saved_constraint;
  return false;
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
  VectorInit(&name->template_arguments);
  StringInit(&name->spelling, NULL);
}

static void DeleteTemplateArgumentVector(Vector* args) {
  if (args == NULL) {
    return;
  }
  VectorDeleteWithContents(args,
                           (VectorElementDestructor)TemplateArgumentDelete,
                           /*free_element=*/false);
}

void FullyQualifiedIdentifierDestruct(FullyQualifiedIdentifier* name) {
  VectorDestructWithContents(&name->components,
                             (VectorElementDestructor)StringDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&name->template_arguments,
                             (VectorElementDestructor)DeleteTemplateArgumentVector,
                             /*free_element=*/false);
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
  VectorAppend(&name->template_arguments, NULL);
}

bool SyntaxParseOperatorFunctionName(Syntax* syntax, String* name) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(operator))) {
    return false;
  }
  Token op = syntax->lex->current_token;
  switch (op) {
    case TOK(string): {
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

bool SyntaxIsCXXNumericLiteralOperatorTemplate(Symbol* symbol) {
  if (symbol == NULL || !symbol->flags.is_template ||
      symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      !StringStartsWith(&symbol->name, "operator\"\"") ||
      symbol->type->info.function.prototype.length != 0 ||
      symbol->type->info.function.template_parameters.length != 1) {
    return false;
  }
  TemplateParameter* parameter =
      symbol->type->info.function.template_parameters.value.p[0];
  return parameter != NULL &&
         parameter->kind == kTemplateParameterNonType &&
         parameter->is_parameter_pack && parameter->type != NULL &&
         parameter->type->declarator == kDeclPrimitive &&
         parameter->type->type == kTypeChar &&
         parameter->type->qualifiers == kQualPlain;
}

bool SyntaxParseMemberOperatorName(Syntax* syntax, String* name) {
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
  return true;
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

static Vector* SyntaxConsumeOptionalTemplateId(Syntax* syntax,
                                               TokenClass followers) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(less))) {
    return NULL;
  }
  return SyntaxParseTemplateArgumentList(syntax, followers);
}

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
  FullyQualifiedIdentifierSetLastTemplateArguments(
      name, SyntaxConsumeOptionalTemplateId(syntax, followers));

  while (LexMatch(lex, TOK(coloncolon))) {
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
    FullyQualifiedIdentifierSetLastTemplateArguments(
        name, SyntaxConsumeOptionalTemplateId(syntax, followers));
  }
  return true;
}

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

static Symbol* LookupSymbolInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
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

static Symbol* LookupTagInEnclosingScopesCheckingAmbiguity(Syntax* syntax,
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

static Namespace* FindNamespaceChildInScope(Syntax* syntax, String* name) {
  LocalSymbolTable* symbols = syntax->local_symbol_stack;
  LocalSymbolTable* tags = syntax->local_tag_stack;
  while (symbols != NULL) {
    Namespace* alias = FindDirectLocalNamespaceAlias(symbols, name);
    if (alias != NULL) {
      return alias;
    }
    if (FindSymbol(&symbols->table, name) != NULL ||
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
      Token previous = TOK(identifier);
      do {
        Token current = syntax->lex->current_token;
        if (LexLookingAt(syntax->lex, TOK(less))) {
          if (previous == TOK(identifier) || previous == TOK(greater) ||
              previous == TOK(greatergreater)) {
            depth++;
          }
        } else if (LexLookingAt(syntax->lex, TOK(greater))) {
          depth--;
        } else if (LexLookingAt(syntax->lex, TOK(greatergreater))) {
          depth -= 2;
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

static bool TypeContainsTemplateParameterReference(TypeRecord* type);
static Symbol* SyntaxFindQualifiedPrefixSymbolImpl(
    Syntax* syntax, FullyQualifiedIdentifier* name, size_t component_count,
    bool allow_dependent_template_args);

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
    NamespaceInlineSymbolLookup result =
        NamespaceResolveSymbolInInlineSet(ns, &last);
    if (result.status == kInlineLookupAmbiguous) {
      SyntaxError(syntax, "reference to '%s' is ambiguous", last.value);
    } else if (result.status == kInlineLookupUnique) {
      symbol = result.symbol;
    }
  } else if (ns == compiler->global_namespace) {
    symbol = ResolveGlobalSymbolCheckingAmbiguity(syntax, &last, NULL);
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
  if (symbol == NULL && name->components.length >= 2) {
    Symbol* owner =
        SyntaxFindQualifiedPrefixSymbolImpl(
            syntax, name, name->components.length - 1,
            /*allow_dependent_template_args=*/true);
    if (owner != NULL && owner->type != NULL &&
        TypeIsStructOrUnion(owner->type) &&
        owner->type->info.struct_info != NULL) {
      StructMember* member =
          FindStructMember(owner->type->info.struct_info, &last);
      if (member != NULL &&
          (member->is_static ||
           (member->symbol != NULL &&
            (StorageIs(member->symbol->storage, STO(typedef)) ||
             member->symbol->flags.value_set)))) {
        symbol = member->symbol;
      }
    }
  }
  StringDestruct(&last);
  return FollowAlias(symbol);
}

static bool TemplateArgumentIsDependent(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->kind == kTemplateParameterNonType) {
    return arg->template_parameter_index >= 0;
  }
  return TypeContainsTemplateParameterReference(arg->type);
}

static bool TemplateArgumentVectorIsDependent(Vector* args) {
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

static bool TypeContainsTemplateParameterReference(TypeRecord* type) {
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (t->template_parameter_index >= 0) {
      return true;
    }
    if (TypeIsArray(t) && t->info.array.template_parameter_index >= 0) {
      return true;
    }
    if (TemplateArgumentVectorIsDependent(t->template_arguments)) {
      return true;
    }
  }
  return false;
}

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
    Symbol* symbol =
        FollowAlias(SyntaxFindSymbol(syntax, name->components.value.p[0]));
    if (allow_dependent_template_args && symbol != NULL &&
        StorageIs(symbol->storage, STO(typedef)) && symbol->type != NULL &&
        symbol->type->template_origin != NULL &&
        symbol->type->dependent_member_name != NULL) {
      TypeRecord* owner =
          TypeInstantiateClassTemplate(syntax, symbol->type->template_origin,
                                       symbol->type->template_arguments);
      Symbol* nested = NULL;
      if (owner != NULL && TypeIsStructOrUnion(owner) &&
          owner->info.struct_info != NULL) {
        StructMember* member =
            FindStructMember(owner->info.struct_info,
                             symbol->type->dependent_member_name);
        if (member != NULL && member->symbol != NULL &&
            StorageIs(member->symbol->storage, STO(typedef))) {
          nested = member->symbol;
        }
      }
      TypeRecordDelete(owner);
      if (nested != NULL) {
        return nested;
      }
    }
    if (template_args != NULL &&
        (allow_dependent_template_args ||
         !TemplateArgumentVectorIsDependent(template_args)) &&
        symbol != NULL && symbol->flags.is_template &&
        symbol->type != NULL && TypeIsStructOrUnion(symbol->type)) {
      TypeRecord* type = TypeInstantiateClassTemplate(syntax, symbol,
                                                      template_args);
      Symbol* tag = type != NULL && type->info.struct_info != NULL
          ? type->info.struct_info->tag_symbol
          : NULL;
      TypeRecordDelete(type);
      return tag;
    }
    return symbol;
  }

  Symbol* parent = allow_dependent_template_args
      ? SyntaxFindQualifiedPrefixSymbolImpl(
            syntax, name, component_count - 1, allow_dependent_template_args)
      : NULL;
  if (allow_dependent_template_args && parent != NULL &&
      StorageIs(parent->storage, STO(typedef)) && parent->type != NULL &&
      parent->type->template_origin != NULL &&
      parent->type->dependent_member_name != NULL) {
    TypeRecord* owner =
        TypeInstantiateClassTemplate(syntax, parent->type->template_origin,
                                     parent->type->template_arguments);
    Symbol* nested = NULL;
    if (owner != NULL && TypeIsStructOrUnion(owner) &&
        owner->info.struct_info != NULL) {
      StructMember* member =
          FindStructMember(owner->info.struct_info,
                           parent->type->dependent_member_name);
      if (member != NULL && member->symbol != NULL &&
          StorageIs(member->symbol->storage, STO(typedef))) {
        nested = member->symbol;
      }
    }
    TypeRecordDelete(owner);
    if (nested != NULL) {
      parent = nested;
    }
  }
  if (parent != NULL && parent->type != NULL &&
      TypeIsStructOrUnion(parent->type) &&
      parent->type->info.struct_info != NULL) {
    String* member_name = name->components.value.p[component_count - 1];
    StructMember* member =
        FindStructMember(parent->type->info.struct_info, member_name);
    if (member != NULL &&
        (member->is_static ||
         (member->symbol != NULL &&
          (StorageIs(member->symbol->storage, STO(typedef)) ||
           member->symbol->flags.value_set)))) {
      if (member->symbol != NULL && member->symbol->type != NULL &&
          TypeIsFunction(member->symbol->type)) {
        member->symbol->type->info.function.cxx_member_owner =
            parent->type->info.struct_info;
        StringClear(&member->symbol->asm_name);
        SymbolSetCXXMangledAsmName(member->symbol);
      }
      return FollowAlias(member->symbol);
    }
  }

  size_t namespace_components = component_count - 1;
  Namespace* ns = NULL;
  if (namespace_components == 0) {
    ns = compiler->global_namespace;
  } else {
    String* first = name->components.value.p[0];
    ns = name->absolute
        ? ResolveNamespaceChildCheckingAmbiguity(syntax,
                                                 compiler->global_namespace,
                                                 first, NULL)
        : FindNamespaceChildInScope(syntax, first);
    for (size_t i = 1; ns != NULL && i < namespace_components; i++) {
      ns = ResolveNamespaceChildCheckingAmbiguity(
          syntax, ns, name->components.value.p[i], NULL);
    }
  }
  if (ns == NULL) {
    return NULL;
  }

  String* last = name->components.value.p[component_count - 1];
  Symbol* symbol = NULL;
  if (ns == compiler->global_namespace) {
    symbol = ResolveGlobalSymbolCheckingAmbiguity(syntax, last, NULL);
  } else {
    NamespaceInlineSymbolLookup result =
        NamespaceResolveSymbolInInlineSet(ns, last);
    if (result.status == kInlineLookupAmbiguous) {
      SyntaxError(syntax, "reference to '%s' is ambiguous", last->value);
    } else if (result.status == kInlineLookupUnique) {
      symbol = result.symbol;
    }
  }
  symbol = FollowAlias(symbol);
  if (template_args != NULL &&
      (allow_dependent_template_args ||
       !TemplateArgumentVectorIsDependent(template_args)) &&
      symbol != NULL && symbol->flags.is_template &&
      symbol->type != NULL && TypeIsStructOrUnion(symbol->type)) {
    TypeRecord* type = TypeInstantiateClassTemplate(syntax, symbol,
                                                    template_args);
    Symbol* tag = type != NULL && type->info.struct_info != NULL
        ? type->info.struct_info->tag_symbol
        : NULL;
    TypeRecordDelete(type);
    return tag;
  }
  return FollowAlias(symbol);
}

Symbol* SyntaxFindQualifiedPrefixSymbol(Syntax* syntax,
                                        FullyQualifiedIdentifier* name,
                                        size_t component_count) {
  return SyntaxFindQualifiedPrefixSymbolImpl(
      syntax, name, component_count,
      /*allow_dependent_template_args=*/false);
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
  VectorInit(&syntax->inline_static_member_definitions);
  VectorInit(&syntax->all_symbols);
  syntax->last_parsed_tag = NULL;
  syntax->parsing_template_declaration = false;
  syntax->parsing_template_specialization = false;
  syntax->parsing_template_argument = false;
  syntax->current_template_parameter_count = 0;
  syntax->current_template_parameters = NULL;
  syntax->current_template_requires_clause = NULL;
  syntax->pending_explicit_condition = NULL;
  syntax->pending_placeholder_variable_constraint = NULL;
  syntax->context = kParsingFileScope;
  syntax->extern_c_depth = 0;
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
  VectorDestruct(&syntax->inline_static_member_definitions);
  ConstraintExprDelete(syntax->current_template_requires_clause);
  syntax->current_template_requires_clause = NULL;
  ConstraintExprDelete(syntax->pending_placeholder_variable_constraint);
  syntax->pending_placeholder_variable_constraint = NULL;
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
  syntax->parsing_template_argument = false;
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
  VectorInit(&syntax->all_local_symbols);
  VectorInit(&syntax->local_statics);
  VectorInit(&syntax->inline_static_member_definitions);
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

bool SyntaxAddSymbol(Syntax* syntax, Symbol* symbol) {
  if (syntax->local_symbol_stack == NULL) {
    if (InNamedNamespace(syntax)) {
      return NamespaceInsertSymbol(syntax->current_namespace, symbol);
    }
    if (NamespaceFindDirectAlias(compiler->global_namespace,
                                 &symbol->name) != NULL) {
      return false;
    }
    return InsertGlobalSymbol(symbol);
  }
  bool ok = InsertLocalSymbol(syntax->local_symbol_stack, symbol);
  if (ok) {
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

bool SyntaxAddBorrowedSymbol(Syntax* syntax, Symbol* symbol) {
  if (syntax == NULL || symbol == NULL || syntax->local_symbol_stack == NULL) {
    return false;
  }
  return InsertLocalSymbol(syntax->local_symbol_stack, symbol);
}

Symbol* SyntaxFindTag(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_tag_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
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
    if (NamespaceFindDirectAlias(compiler->global_namespace,
                                 &symbol->name) != NULL) {
      return false;
    }
    return InsertGlobalTag(symbol);
  }
  if (syntax->local_symbol_stack != NULL &&
      FindDirectLocalNamespaceAlias(syntax->local_symbol_stack,
                                    &symbol->name) != NULL) {
    return false;
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

/* Report an error at a specific, previously recorded source location rather
 * than the lexer's current position.  Used when a diagnostic is discovered long
 * after the offending token was consumed (e.g. during template substitution),
 * so the error must point back at where the construct was actually written. */
void SyntaxErrorAtLocation(Syntax* syntax, SourceLocation location,
                           const char* format, ...) {
  if (abort_on_error) {
    longjmp(error_abort_state, 1);
  }
  const char* filename;
  int lineno, start, end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);
  va_list ap;
  va_start(ap, format);
  VReportError(filename, lineno, format, ap);
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
    case TOK(thread_local):
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
  if (CompilerIsCXX() && parser->is_inline && !TypeIsFunction(sym->type)) {
    return true;
  }
  return LexLookingAt(parser->lex, TOK(equal)) ||
      LexLookingAt(parser->lex, TOK(lbrace));
}

static ASTNode* ParseBracedInitializer(Syntax* syntax);
static ASTNode* ParseNamespaceDeclaration(Syntax* syntax, bool leading_inline);
static ASTNode* ParseUsingDeclaration(Syntax* syntax);
static void AddOwnedAssociatedConstraint(ConstraintExpr** target,
                                         ConstraintExpr* constraint);
static void MoveTemplateParameterConstraints(Vector* parameters,
                                             ConstraintExpr** target);

static bool StaticAssertTemplateArgumentContainsTemplateParameter(
    TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type)) {
    return true;
  }
  for (size_t i = 0; arg->pack_arguments != NULL &&
                     i < arg->pack_arguments->length; i++) {
    if (StaticAssertTemplateArgumentContainsTemplateParameter(
            arg->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

static bool StaticAssertTemplateArgumentVectorContainsTemplateParameter(
    Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (StaticAssertTemplateArgumentContainsTemplateParameter(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

static void ExpressionDependencyVisitor(ASTNode* node, void* data,
                                        int child_id, VisitorMode mode) {
  (void)child_id;
  if (node == NULL || mode != kVisitPreChildren || *(bool*)data) {
    return;
  }
  if (TypeContainsTemplateParameter(node->type)) {
    *(bool*)data = true;
    return;
  }
  if ((node->flags & kASTDependentQualifiedName) != 0 ||
      node->op == AST_OP(requires_expr)) {
    *(bool*)data = true;
    return;
  }
  if (node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) {
    // `sizeof(T)` / `alignof(T)` on a dependent type is value-dependent even
    // though the node's own type is size_t; the operand type is retained only
    // when dependent (see SizeofASTNode::type_operand).
    SizeofASTNode* s = (SizeofASTNode*)node;
    if (TypeContainsTemplateParameter(s->type_operand)) {
      *(bool*)data = true;
      return;
    }
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL &&
        (id->symbol->template_parameter_index >= 0 ||
         TypeContainsTemplateParameter(id->symbol->type) ||
         StaticAssertTemplateArgumentVectorContainsTemplateParameter(
             id->template_arguments))) {
      *(bool*)data = true;
    }
  }
}

bool ExpressionIsTemplateDependent(ASTNode* expr) {
  bool dependent = false;
  ASTNodeVisit(expr, ExpressionDependencyVisitor, 0, &dependent);
  return dependent;
}

static void ClearStaticAssertExprAnalysis(ASTNode* node, void* data, int child_id,
                                          VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

static ASTNode* EvaluateStaticAssertExpression(ASTNode* expr) {
  ASTNode* cloned = ASTNodeClone(expr, IdentityCloneNode, NULL, NULL);
  if (cloned == NULL) {
    return NULL;
  }
  ASTNodeVisit(cloned, ClearStaticAssertExprAnalysis, 0, NULL);
  cloned = AnalyzeExpression(cloned);
  if (cloned == NULL) {
    ASTNodeDelete(cloned);
    return NULL;
  }
  return cloned;
}

ASTNode* SyntaxParseStaticAssert(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // static_assert
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));

  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(exprsep));
  bool dependent = syntax->current_template_parameter_count > 0 &&
                   ExpressionIsTemplateDependent(expr);

  String message = {0};
  StringInit(&message, "static assertion failed");
  if (LexMatch(syntax->lex, TOK(comma))) {
    if (LexLookingAt(syntax->lex, TOK(string))) {
      StringSetString(&message, &syntax->lex->spelling);
      LexNextToken(syntax->lex);
    } else {
      SyntaxError(syntax, "static_assert message must be a string literal");
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra));
  SyntaxNeedBracket(syntax, TOK(semicolon), TC(semicolon));

  if (dependent) {
    ASTNode* node = NewStaticAssertASTNode(expr, &message, location);
    StringDestruct(&message);
    return node;
  }

  ASTNode* evaluated = EvaluateStaticAssertExpression(expr);
  ASTNodeDelete(expr);
  if (evaluated == NULL) {
    StringDestruct(&message);
    SyntaxError(syntax, "static_assert expression is not an integer constant expression");
    return NULL;
  }

  int64_t value = 0;
  if (!EvaluateIntegerExpression(evaluated, &value)) {
    if (ExpressionIsTemplateDependent(evaluated)) {
      ASTNode* node = NewStaticAssertASTNode(evaluated, &message, location);
      StringDestruct(&message);
      return node;
    }
    ASTNodeDelete(evaluated);
    StringDestruct(&message);
    SyntaxError(syntax, "static_assert expression is not an integer constant expression");
    return NULL;
  }

  if (value == 0) {
    SyntaxError(syntax, "%s", message.value);
  }
  StringDestruct(&message);
  ASTNodeDelete(evaluated);
  return NULL;
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

static ASTNode* CloneCXXDefaultArgument(ASTNode* arg) {
  return ASTNodeClone(arg, IdentityCloneNode, NULL, NULL);
}

static void MergeCXXDefaultArguments(Syntax* syntax, Symbol* old_sym,
                                     Symbol* new_sym) {
  if (!CompilerIsCXX() || old_sym == NULL || new_sym == NULL ||
      old_sym->type == NULL || new_sym->type == NULL ||
      !TypeIsFunction(old_sym->type) || !TypeIsFunction(new_sym->type) ||
      old_sym->type->info.function.prototype.length !=
          new_sym->type->info.function.prototype.length) {
    return;
  }
  for (size_t i = 0; i < old_sym->type->info.function.prototype.length; i++) {
    Symbol* old_formal = old_sym->type->info.function.prototype.value.p[i];
    Symbol* new_formal = new_sym->type->info.function.prototype.value.p[i];
    if (old_formal == NULL || new_formal == NULL) {
      continue;
    }
    if (old_formal->default_argument != NULL &&
        new_formal->default_argument != NULL) {
      SyntaxError(syntax, "default argument already specified");
      continue;
    }
    if (old_formal->default_argument == NULL &&
        new_formal->default_argument != NULL) {
      old_formal->default_argument =
          CloneCXXDefaultArgument(new_formal->default_argument);
    } else if (old_formal->default_argument != NULL &&
               new_formal->default_argument == NULL) {
      new_formal->default_argument =
          CloneCXXDefaultArgument(old_formal->default_argument);
    }
  }
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

typedef struct {
  bool found;
} CXXPackExpressionSearch;

static void FindCXXParameterPackExpression(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
    ((CXXPackExpressionSearch*)data)->found = true;
  }
}

static bool CXXExpressionContainsParameterPack(ASTNode* node) {
  CXXPackExpressionSearch search = {0};
  ASTNodeVisit(node, FindCXXParameterPackExpression, 0, &search);
  return search.found;
}

static void MarkCXXPackExpansionIfPresent(Syntax* syntax, ASTNode* expr) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(ellipsis))) {
    return;
  }
  if (!CXXExpressionContainsParameterPack(expr)) {
    SyntaxError(syntax, "pack expansion requires a function parameter pack");
  }
  expr->flags |= kASTPackExpansion;
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
      MarkCXXPackExpansionIfPresent(syntax, expr);
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

ASTNode* SyntaxParseBracedInitializer(Syntax* syntax) {
  return ParseBracedInitializer(syntax);
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

ASTNode* SyntaxParseCXXDefaultMemberInitializer(Syntax* syntax) {
  if (!CompilerIsCXX()) {
    return NULL;
  }
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    return ParseBracedInitializer(syntax);
  }
  if (!LexMatch(syntax->lex, TOK(equal))) {
    return NULL;
  }
  SourceLocation location = syntax->lex->current_token_location;
  return NewExpressionInitializerASTNode(
      SyntaxParseSingleExpression(syntax, TC(semicolon) | TC(exprsep)),
      location);
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
    "constructor", "destructor",
    // Accepted but not modelled (parsed cleanly, no effect).
    "stdcall", "cdecl", "fastcall", "thiscall", "regparm", "ms_abi",
    "sysv_abi", "may_alias", "gnu_inline", "nothrow", "leaf", "cold", "hot",
    "malloc", "pure", "const", "nonnull", "returns_nonnull", "sentinel",
    "weak", "alias", "section", "visibility", "used",
    "transparent_union", "vector_size", "mode",
    "no_instrument_function", "cleanup", "returns_twice", "artificial",
    "designated_init", "fallthrough", "warning", "error", "alloc_size",
    "format_arg", "nonstring", "noclone", "noipa", "flatten", "naked",
    "weakref", "dllimport", "dllexport", "common", "nocommon", "tls_model",
    "aligned_alloc", "assume_aligned", "likely", "unlikely",
    "no_unique_address",
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

bool SyntaxLookingAtCXXAttribute(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(lsquare))) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  LexNextToken(syntax->lex);
  bool result = LexLookingAt(syntax->lex, TOK(lsquare));
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

static void AppendCXXAttributeArgToken(Lex* lex, String* arg) {
  if (arg->length > 0) {
    StringAppendChar(arg, ' ');
  }
  switch (lex->current_token) {
    case TOK(identifier):
    case TOK(string):
    case TOK(string_wide):
    case TOK(charconst):
    case TOK(charconst_wide):
      StringAppendString(arg, &lex->spelling);
      break;
    case TOK(number): {
      String buf;
      StringInit(&buf, NULL);
      StringPrintf(&buf, "%lld", (long long)lex->number);
      StringAppendString(arg, &buf);
      StringDestruct(&buf);
      break;
    }
    case TOK(fnumber): {
      String buf;
      StringInit(&buf, NULL);
      StringPrintf(&buf, "%g", lex->fnumber);
      StringAppendString(arg, &buf);
      StringDestruct(&buf);
      break;
    }
    default:
      StringAppend(arg, TokenName(lex->current_token));
      break;
  }
}

static void ParseCXXAttributeArguments(Syntax* syntax, Attribute* attr) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return;
  }

  int depth = 1;
  String arg;
  StringInit(&arg, NULL);
  while (!LexEof(syntax->lex) && depth > 0) {
    if (LexLookingAt(syntax->lex, TOK(lparen))) {
      if (depth > 0) {
        AppendCXXAttributeArgToken(syntax->lex, &arg);
      }
      depth++;
      LexNextToken(syntax->lex);
    } else if (LexLookingAt(syntax->lex, TOK(rparen))) {
      depth--;
      if (depth == 0) {
        if (arg.length > 0) {
          AttributeAddArg(attr, arg.value, arg.length);
          StringClear(&arg);
        }
        LexNextToken(syntax->lex);
        break;
      }
      AppendCXXAttributeArgToken(syntax->lex, &arg);
      LexNextToken(syntax->lex);
    } else if (LexLookingAt(syntax->lex, TOK(comma)) && depth == 1) {
      AttributeAddArg(attr, arg.value != NULL ? arg.value : "", arg.length);
      StringClear(&arg);
      LexNextToken(syntax->lex);
    } else {
      AppendCXXAttributeArgToken(syntax->lex, &arg);
      LexNextToken(syntax->lex);
    }
  }
  if (depth != 0) {
    SyntaxError(syntax, "Unterminated attribute argument list");
  }
  StringDestruct(&arg);
}

static bool ParseCXXAttributeIdentifier(Syntax* syntax, String* out) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  StringSetString(out, &syntax->lex->spelling);
  LexNextToken(syntax->lex);
  return true;
}

static const char* CanonicalCXXAttributeName(const char* ns,
                                             const char* name) {
  if (ns != NULL && strcmp(ns, "gnu") != 0) {
    return name;
  }
  if (strcmp(name, "maybe_unused") == 0) {
    return "unused";
  }
  if (strcmp(name, "nodiscard") == 0) {
    return "warn_unused_result";
  }
  return name;
}

static void ParseCXXSingleAttribute(Syntax* syntax, Vector* attrs,
                                    const char* using_namespace) {
  String first;
  String second;
  StringInit(&first, NULL);
  StringInit(&second, NULL);
  if (!ParseCXXAttributeIdentifier(syntax, &first)) {
    SyntaxError(syntax, "Expected attribute name");
    StringDestruct(&first);
    StringDestruct(&second);
    return;
  }

  const char* namespace_name = using_namespace;
  const char* attr_name = first.value;
  if (LexMatch(syntax->lex, TOK(coloncolon))) {
    namespace_name = first.value;
    if (!ParseCXXAttributeIdentifier(syntax, &second)) {
      SyntaxError(syntax, "Expected attribute name after ::");
      StringDestruct(&first);
      StringDestruct(&second);
      return;
    }
    attr_name = second.value;
  }

  Attribute* attr =
      NewAttribute(CanonicalCXXAttributeName(namespace_name, attr_name));
  ParseCXXAttributeArguments(syntax, attr);
  if (StringEqual(&attr->name, "aligned") && AttributeArgCount(attr) > 0) {
    long ignored = 0;
    if (!AttributeArgInt(attr, 0, &ignored)) {
      SyntaxError(syntax, "aligned attribute argument must be an integer");
    }
  }
  if (!IsKnownAttribute(attr->name.value)) {
    SyntaxWarning(syntax, "attributes", "'%s' attribute directive ignored",
                  attr->name.value);
  }
  VectorAppend(attrs, attr);
  StringDestruct(&first);
  StringDestruct(&second);
}

bool SyntaxParseCXXAttributes(Syntax* syntax, Vector* attrs) {
  bool any = false;
  while (SyntaxLookingAtCXXAttribute(syntax)) {
    any = true;
    LexMatch(syntax->lex, TOK(lsquare));
    LexMatch(syntax->lex, TOK(lsquare));

    const char* using_namespace = NULL;
    String using_name;
    StringInit(&using_name, NULL);
    if (LexMatch(syntax->lex, TOK(using))) {
      if (ParseCXXAttributeIdentifier(syntax, &using_name)) {
        using_namespace = using_name.value;
      } else {
        SyntaxError(syntax, "Expected attribute namespace after using");
      }
      SyntaxNeedBracket(syntax, TOK(colon), TC(closebra));
    }

    while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
      ParseCXXSingleAttribute(syntax, attrs, using_namespace);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      if (LexLookingAt(syntax->lex, TOK(rsquare))) {
        break;
      }
    }
    SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
    SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
    StringDestruct(&using_name);
  }
  return any;
}

static bool IsPowerOf2OrZero(int32_t v);

static void AppendAlignedAttribute(Vector* attrs, int alignment) {
  if (alignment <= 0) {
    return;
  }
  Attribute* attr = NewAttribute("aligned");
  String arg;
  StringInit(&arg, NULL);
  StringPrintf(&arg, "%d", alignment);
  AttributeAddArg(attr, arg.value, arg.length);
  StringDestruct(&arg);
  VectorAppend(attrs, attr);
}

bool SyntaxParseCXXAlignas(Syntax* syntax, Vector* attrs) {
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(alignas))) {
    return false;
  }

  SyntaxNeedBracket(syntax, TOK(lparen), TC(decl) | TC(closebra));
  int64_t value = 0;
  if (SyntaxLookingAtType(syntax)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym == NULL || sym->type == NULL) {
      SyntaxError(syntax, "alignas type-id is invalid");
    } else if (TypeContainsTemplateParameter(sym->type)) {
      SyntaxError(syntax, "dependent alignas type-id is not supported");
    } else {
      TypeRecordCalculateSize(sym->type);
      value = TypeRecordAlignment(sym->type);
    }
    SymbolDelete(sym);
    TypeParserDestruct(&parser);
  } else {
    ASTNode* expr = SyntaxParseExpression(syntax, TC(closebra));
    expr = AnalyzeExpression(expr);
    bool ok = EvaluateIntegerExpression(expr, &value);
    if (!ok) {
      SyntaxError(syntax, "alignas specifier must be a constant expression");
    }
    ASTNodeDelete(expr);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));

  if (value < 0 || value > 2147483647 ||
      !IsPowerOf2OrZero((int32_t)value)) {
    SyntaxError(syntax, "alignas specifier must name a power-of-two alignment");
    return true;
  }
  AppendAlignedAttribute(attrs, (int)value);
  return true;
}

// Interprets attributes that have just been attached to a declared symbol and
// affect the symbol/type directly (layout and function-behavior flags).
void SyntaxApplyDeclarationAttributes(Symbol* sym) {
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
  if (AttributeListHas(&sym->attributes, "weak")) {
    sym->flags.is_weak = true;
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
static ASTNode* ParseCXXDirectInitializer(Syntax* syntax, Symbol* sym,
                                          bool wrap_aggregate_braces);
static bool ResolveCXXClassTemplateArgumentDeductionFromInitializer(
    Syntax* syntax, Symbol* sym, ASTNode* initializer);
static const char* CXXConstructorNameForType(TypeRecord* type);
static StructMember* FindCXXConstructor(TypeRecord* type);
static ASTNode* NewVariableInitExpression(Syntax* syntax, Symbol* sym,
                                          ASTNode* initializer);
static ASTNode* NewCXXCompleteObjectGuardedStatement(TypeRecord* func,
                                                    Vector* statements,
                                                    SourceLocation location);

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

static bool TypeNeedsCXXCompleteObjectArgument(TypeRecord* type) {
  return TypeIsStructOrUnion(type) && type->info.struct_info != NULL &&
         StructHasVirtualBases(type->info.struct_info);
}

static ASTNode* NewCXXCompleteObjectArgument(bool complete_object,
                                            SourceLocation location) {
  return NewIntConstantASTNode(complete_object ? 1 : 0,
                               NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

static void CXXPrependCompleteObjectArgument(TypeRecord* type, Vector* actuals,
                                             bool complete_object,
                                             SourceLocation location) {
  if (actuals == NULL || !TypeNeedsCXXCompleteObjectArgument(type)) {
    return;
  }
  ASTNode* arg = NewCXXCompleteObjectArgument(complete_object, location);
  if (actuals->length == 0) {
    VectorAppend(actuals, arg);
  } else {
    VectorInsertBefore(actuals, 0, arg);
  }
}

static ASTNode* NewCXXBaseSpecialMemberCall(Syntax* syntax,
                                            TypeRecord* receiver_func,
                                            CXXBaseSpecifier* base,
                                            bool destructor,
                                            bool complete_object,
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
  if (actuals == NULL) {
    actuals = NewVector();
  }
  CXXPrependCompleteObjectArgument(base->type, actuals, complete_object,
                                   location);

  String member_name;
  if (destructor) {
    StringInit(&member_name, "~");
    StringAppendString(&member_name, base->type->info.struct_info->tag_name);
  } else {
    StringInit(&member_name, base->type->info.struct_info->tag_name->value);
  }
  ASTNode* receiver = NewIdentifierASTNode(this_symbol, location);
  if (base->byte_offset != 0) {
    TypeRecord* base_pointer =
        NewPointerTo(kQualPlain, TypeRecordCopy(base->type));
    ASTNode* offset =
        NewIntConstantASTNode(base->byte_offset,
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location);
    receiver = NewBinaryASTNode(AST_OP(plus), base_pointer, location,
                                receiver, offset);
    receiver->flags |= kASTAnalyzed;
    ASTNodeSetType(receiver, base_pointer);
  }
  ASTNode* member_node =
      NewStringConstantASTNode(NewString(member_name.value), NULL, location);
  StringDestruct(&member_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver, member_node);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                       actuals);
  return NewExpressionStatementASTNode(call, location);
}

static ASTNode* NewCXXVirtualBaseSpecialMemberCall(Syntax* syntax,
                                                   TypeRecord* receiver_func,
                                                   CXXVirtualBaseInfo* vbase,
                                                   bool destructor,
                                                   Vector* actuals,
                                                   SourceLocation location) {
  if (vbase == NULL) {
    if (actuals != NULL) {
      VectorDelete(actuals);
    }
    return NULL;
  }
  CXXBaseSpecifier base;
  base.type = vbase->type;
  base.access = vbase->access;
  base.byte_offset = vbase->byte_offset;
  base.is_virtual = true;
  return NewCXXBaseSpecialMemberCall(syntax, receiver_func, &base, destructor,
                                     /*complete_object=*/false, actuals,
                                     location);
}

void AppendCXXBaseDestructorCalls(Syntax* syntax, TypeRecord* func,
                                  Vector* body, SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_destructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = owner->bases.length; i > 0; i--) {
    CXXBaseSpecifier* base = owner->bases.value.p[i - 1];
    if (base->is_virtual) {
      continue;
    }
    ASTNode* call = NewCXXBaseSpecialMemberCall(syntax, func, base, true,
                                                /*complete_object=*/false,
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
  Vector* virtual_base_destructors = NewVector();
  for (size_t i = owner->virtual_bases.length; i > 0; i--) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i - 1];
    ASTNode* call = NewCXXVirtualBaseSpecialMemberCall(syntax, func, base,
                                                       true, NULL, location);
    if (call == NULL) {
      continue;
    }
    VectorAppend(virtual_base_destructors, call);
  }
  ASTNode* guarded = NewCXXCompleteObjectGuardedStatement(
      func, virtual_base_destructors, location);
  if (guarded != NULL) {
    if (compiler->debug_output && body->length > 0) {
      VectorInsertBefore(body, body->length - 1, guarded);
    } else {
      VectorAppend(body, guarded);
    }
  }
}

static StructMember* FindCXXDestructorForObjectType(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  StringDestruct(&destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      destructor->symbol == NULL || destructor->symbol->type == NULL ||
      !destructor->symbol->type->info.function.is_destructor) {
    return NULL;
  }
  return destructor;
}

static TypeRecord* CXXDestructibleElementType(TypeRecord* type) {
  if (TypeIsFixedArray(type) && type->next != NULL &&
      FindCXXDestructorForObjectType(type->next) != NULL) {
    return type->next;
  }
  if (FindCXXDestructorForObjectType(type) != NULL) {
    return type;
  }
  return NULL;
}

static ASTNode* NewCXXMemberDestructorCall(Syntax* syntax, TypeRecord* func,
                                           StructMember* member,
                                           TypeRecord* object_type,
                                           ASTNode* receiver,
                                           SourceLocation location) {
  (void)syntax;
  if (func == NULL || func->info.function.prototype.length == 0 ||
      member == NULL || object_type == NULL ||
      object_type->info.struct_info == NULL ||
      object_type->info.struct_info->tag_name == NULL) {
    ASTNodeDelete(receiver);
    return NULL;
  }
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(object_type, actuals,
                                   /*complete_object=*/true, location);
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, object_type->info.struct_info->tag_name);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL, location);
  StringDestruct(&destructor_name);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, destructor);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

static ASTNode* NewCXXMemberReceiver(TypeRecord* func, StructMember* member,
                                     SourceLocation location) {
  if (func == NULL || func->info.function.prototype.length == 0 ||
      member == NULL || member->symbol == NULL) {
    return NULL;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  ASTNode* this_node = NewIdentifierASTNode(this_symbol, location);
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(member->symbol->name.value), NULL,
                               location);
  return NewBinaryASTNode(AST_OP(arrow), NULL, location, this_node,
                          member_name);
}

static void SyntaxAppendCXXMemberDestructorCalls(Syntax* syntax, TypeRecord* func,
                                           Vector* body,
                                           SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_destructor ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = owner->members.length; i > 0; i--) {
    StructMember* member = owner->members.value.p[i - 1];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    TypeRecord* object_type = CXXDestructibleElementType(member_type);
    if (object_type == NULL) {
      continue;
    }
    if (TypeIsFixedArray(member_type)) {
      for (size_t index = member_type->info.array.size.fixed; index > 0;
           index--) {
        ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
        ASTNode* index_node = NewIntConstantASTNode(
            (int64_t)index - 1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
            location);
        receiver = NewBinaryASTNode(AST_OP(subscript), NULL, location,
                                    receiver, index_node);
        ASTNode* call = NewCXXMemberDestructorCall(
            syntax, func, member, object_type, receiver, location);
        if (call != NULL) {
          VectorAppend(body, call);
        }
      }
    } else {
      ASTNode* receiver = NewCXXMemberReceiver(func, member, location);
      ASTNode* call = NewCXXMemberDestructorCall(
          syntax, func, member, object_type, receiver, location);
      if (call != NULL) {
        VectorAppend(body, call);
      }
    }
  }
}

typedef struct {
  String name;
  Vector* actuals;
  SourceLocation location;
} CXXDeferredConstructorInitializer;

static CXXDeferredConstructorInitializer* NewCXXDeferredConstructorInitializer(
    const char* name, Vector* actuals, SourceLocation location) {
  CXXDeferredConstructorInitializer* init =
      malloc(sizeof(CXXDeferredConstructorInitializer));
  StringInit(&init->name, name);
  init->actuals = actuals;
  init->location = location;
  return init;
}

static Vector* CloneCXXConstructorInitializerActuals(Vector* actuals) {
  Vector* clone = NewVector();
  if (actuals == NULL) {
    return clone;
  }
  for (size_t i = 0; i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    VectorAppend(clone, ASTNodeClone(actual, IdentityCloneNode, NULL, NULL));
  }
  return clone;
}

static CXXDeferredConstructorInitializer* CloneCXXDeferredConstructorInitializer(
    CXXDeferredConstructorInitializer* init) {
  if (init == NULL) {
    return NULL;
  }
  return NewCXXDeferredConstructorInitializer(
      init->name.value, CloneCXXConstructorInitializerActuals(init->actuals),
      init->location);
}

static void CXXDeferredConstructorInitializerDelete(
    CXXDeferredConstructorInitializer* init) {
  if (init == NULL) {
    return;
  }
  StringDestruct(&init->name);
  if (init->actuals != NULL) {
    VectorDelete(init->actuals);
  }
  free(init);
}

void SyntaxCXXConstructorInitListInit(CXXConstructorInitList* init_list) {
  VectorInit(&init_list->virtual_base_specs);
  VectorInit(&init_list->virtual_base_statements);
  VectorInit(&init_list->base_specs);
  VectorInit(&init_list->base_statements);
  VectorInit(&init_list->member_specs);
  VectorInit(&init_list->member_statements);
  VectorInit(&init_list->raw_initializers);
  VectorInit(&init_list->deferred_initializers);
  init_list->last_initializer_order = -1;
}

void SyntaxCXXConstructorInitListDestruct(CXXConstructorInitList* init_list) {
  VectorDestruct(&init_list->virtual_base_specs);
  VectorDestruct(&init_list->virtual_base_statements);
  VectorDestruct(&init_list->base_specs);
  VectorDestruct(&init_list->base_statements);
  VectorDestruct(&init_list->member_specs);
  VectorDestruct(&init_list->member_statements);
  VectorDestructWithContents(
      &init_list->raw_initializers,
      (VectorElementDestructor)CXXDeferredConstructorInitializerDelete,
      /*free_element=*/false);
  VectorDestructWithContents(
      &init_list->deferred_initializers,
      (VectorElementDestructor)CXXDeferredConstructorInitializerDelete,
      /*free_element=*/false);
}

static CXXConstructorInitList* SyntaxCXXConstructorInitListCloneVector(
    Vector* initializers) {
  CXXConstructorInitList* clone = malloc(sizeof(CXXConstructorInitList));
  SyntaxCXXConstructorInitListInit(clone);
  if (initializers == NULL) {
    return clone;
  }
  for (size_t i = 0; i < initializers->length; i++) {
    CXXDeferredConstructorInitializer* init = initializers->value.p[i];
    CXXDeferredConstructorInitializer* cloned =
        CloneCXXDeferredConstructorInitializer(init);
    if (cloned != NULL) {
      VectorAppend(&clone->deferred_initializers, cloned);
    }
  }
  return clone;
}

CXXConstructorInitList* SyntaxCXXConstructorInitListCloneRaw(
    CXXConstructorInitList* init_list) {
  if (init_list == NULL) {
    return NULL;
  }
  return SyntaxCXXConstructorInitListCloneVector(&init_list->raw_initializers);
}

CXXConstructorInitList* SyntaxCXXConstructorInitListCloneDeferred(
    CXXConstructorInitList* init_list) {
  if (init_list == NULL) {
    return NULL;
  }
  return SyntaxCXXConstructorInitListCloneVector(
      &init_list->deferred_initializers);
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

static int CXXDirectBaseOrder(Struct* owner, CXXBaseSpecifier* base) {
  if (owner == NULL || base == NULL) {
    return -1;
  }
  for (size_t i = 0; i < owner->bases.length; i++) {
    if (owner->bases.value.p[i] == base) {
      return (int)owner->virtual_bases.length + (int)i;
    }
  }
  return -1;
}

static int CXXVirtualBaseOrder(Struct* owner, CXXVirtualBaseInfo* base) {
  if (owner == NULL || base == NULL) {
    return -1;
  }
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    if (owner->virtual_bases.value.p[i] == base) {
      return (int)i;
    }
  }
  return -1;
}

static int CXXDirectMemberOrder(Struct* owner, StructMember* member) {
  if (owner == NULL || member == NULL) {
    return -1;
  }
  return (int)owner->virtual_bases.length + (int)owner->bases.length +
         (int)member->index;
}

static void CheckCXXConstructorInitializerOrder(
    Syntax* syntax, CXXConstructorInitList* init_list, const char* name,
    int order) {
  if (order < 0) {
    return;
  }
  if (init_list->last_initializer_order > order) {
    SyntaxWarning(syntax, "reorder-ctor-init",
                  "constructor initializer for %s does not match declaration order",
                  name);
  }
  init_list->last_initializer_order = order;
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

static CXXVirtualBaseInfo* FindCXXVirtualBaseByName(Struct* owner,
                                                    const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
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

static Symbol* CXXSourceObjectParameter(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length < 2) {
    return NULL;
  }
  return func->info.function.prototype.value.p[func->info.function.prototype.length - 1];
}

static ASTNode* NewCXXSourceMemberAccess(Symbol* source,
                                         const char* member_name,
                                         SourceLocation location) {
  if (source == NULL) {
    return NULL;
  }
  return NewBinaryASTNode(
      AST_OP(dot), NULL, location, NewIdentifierASTNode(source, location),
      NewStringConstantASTNode(NewString(member_name), NULL, location));
}

static bool CXXFunctionNeedsMemberwiseDefaultedBody(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return false;
  }
  switch (func->info.function.cxx_special_member_kind) {
    case kCXXSpecialMemberCopyConstructor:
    case kCXXSpecialMemberMoveConstructor:
    case kCXXSpecialMemberCopyAssignment:
    case kCXXSpecialMemberMoveAssignment:
      return true;
    default:
      return false;
  }
}

static void AppendCXXDefaultedMemberwiseAssignments(TypeRecord* func,
                                                    Vector* body,
                                                    SourceLocation location) {
  if (!CXXFunctionNeedsMemberwiseDefaultedBody(func) ||
      func->info.function.cxx_member_owner == NULL) {
    return;
  }
  Symbol* source = CXXSourceObjectParameter(func);
  if (source == NULL) {
    return;
  }
  bool is_constructor_initializer =
      func->info.function.cxx_special_member_kind ==
          kCXXSpecialMemberCopyConstructor ||
      func->info.function.cxx_special_member_kind ==
          kCXXSpecialMemberMoveConstructor;
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    if (TypeIsFixedArray(member_type)) {
      for (size_t index = 0; index < member_type->info.array.size.fixed;
           index++) {
        ASTNode* target =
            NewCXXThisMemberAccess(func, member->symbol->name.value, location);
        ASTNode* value =
            NewCXXSourceMemberAccess(source, member->symbol->name.value,
                                     location);
        target = NewBinaryASTNode(
            AST_OP(subscript), NULL, location, target,
            NewIntConstantASTNode((int64_t)index,
                                  NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                  location));
        value = NewBinaryASTNode(
            AST_OP(subscript), NULL, location, value,
            NewIntConstantASTNode((int64_t)index,
                                  NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                  location));
        ASTNode* assign = NewBinaryASTNode(AST_OP(assign), member_type->next,
                                           location, target, value);
        if (is_constructor_initializer) {
          assign->flags |= kASTCXXMemberInitializer;
        }
        VectorAppend(body, NewExpressionStatementASTNode(assign, location));
      }
    } else {
      ASTNode* target =
          NewCXXThisMemberAccess(func, member->symbol->name.value, location);
      ASTNode* value =
          NewCXXSourceMemberAccess(source, member->symbol->name.value,
                                   location);
      if (is_constructor_initializer && TypeIsStructOrUnion(member_type)) {
        Vector* actuals = NewVector();
        VectorAppend(actuals, value);
        ASTNode* init = SyntaxNewCXXMemberInitializerStatement(
            &compiler->syntax, func, member, actuals, location);
        if (init != NULL) {
          VectorAppend(body, init);
        }
        continue;
      }
      ASTNode* assign =
          NewBinaryASTNode(AST_OP(assign), member_type, location, target, value);
      if (is_constructor_initializer) {
        assign->flags |= kASTCXXMemberInitializer;
      }
      VectorAppend(body, NewExpressionStatementASTNode(assign, location));
    }
  }
}

static void AppendCXXDefaultedAssignmentReturnThis(TypeRecord* func,
                                                   Vector* body,
                                                   SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) || func->next == NULL ||
      TypeIsVoid(func->next) || func->info.function.prototype.length == 0 ||
      (func->info.function.cxx_special_member_kind !=
           kCXXSpecialMemberCopyAssignment &&
       func->info.function.cxx_special_member_kind !=
           kCXXSpecialMemberMoveAssignment)) {
    return;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  TypeRecord* object_type =
      TypeIsPointer(this_symbol->type) ? this_symbol->type->next : NULL;
  ASTNode* object =
      NewUnaryASTNode(AST_OP(contents), object_type, location,
                      NewIdentifierASTNode(this_symbol, location));
  VectorAppend(body,
               NewCombinedStatementASTNode(AST_OP(return), object, NULL,
                                           location));
}

static TypeRecord* NewCXXStructType(Struct* str);

static ASTNode* NewCXXVPtrReceiver(TypeRecord* func, Struct* source,
                                   int source_offset,
                                   SourceLocation location) {
  Symbol* this_symbol = CXXThisSymbolFromFunction(func);
  if (this_symbol == NULL || source == NULL) {
    return NULL;
  }
  ASTNode* receiver = NewIdentifierASTNode(this_symbol, location);
  TypeRecord* source_pointer =
      NewPointerTo(kQualPlain, NewCXXStructType(source));
  ASTNode* offset =
      NewIntConstantASTNode(source_offset,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* adjusted =
      NewBinaryASTNode(AST_OP(plus), source_pointer, location, receiver,
                       offset);
  adjusted->flags |= kASTAnalyzed;
  return adjusted;
}

// A constructor's __vptr initialization that could not be emitted when its
// preamble was built (because the class's vtables had not been registered yet),
// to be inserted into `body` at `index` once SyntaxFlushPendingVPtrInitializers
// runs for `owner`.
typedef struct {
  Struct* owner;
  TypeRecord* func;
  size_t index;
  SourceLocation location;
} PendingVPtrInit;

static Vector pending_vptr_inits;
static bool pending_vptr_inits_initialized = false;

static ASTNode* NewCXXVPtrInitializer(TypeRecord* func, CXXVTableInfo* info,
                                      SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL || info == NULL ||
      info->source == NULL || info->symbol == NULL) {
    return NULL;
  }
  StructMember* vptr_member = FindStructMemberByName(info->source, "__vptr");
  if (vptr_member == NULL) {
    return NULL;
  }
  ASTNode* receiver =
      NewCXXVPtrReceiver(func, info->source, info->source_offset, location);
  if (receiver == NULL) {
    return NULL;
  }
  ASTNode* target =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver,
                       NewStringConstantASTNode(NewString("__vptr"), NULL,
                                                location));
  // The vtable begins with two RTTI header entries (offset_to_top and
  // &type_info); __vptr must point at the first function pointer, i.e. two
  // entries past the start of the table.
  ASTNode* value =
      NewBinaryASTNode(AST_OP(plus), NULL, location,
                       NewIdentifierASTNode(info->symbol, location),
                       NewIntConstantASTNode(
                           2, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                           location));
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), vptr_member->symbol->type,
                       location, target, value),
      location);
}

static void AppendCXXVPtrInitializers(TypeRecord* func, Vector* body,
                                      SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL || body == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < owner->vtable_symbols.length; i++) {
    CXXVTableInfo* info = owner->vtable_symbols.value.p[i];
    ASTNode* init = NewCXXVPtrInitializer(func, info, location);
    if (init != NULL) {
      VectorAppend(body, init);
    }
  }
}

static TypeRecord* NewCXXStructType(Struct* str) {
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  type->info.struct_info = str;
  TypeRecordCalculateSize(type);
  return type;
}

static ASTNode* NewCXXVBPtrReceiver(TypeRecord* func,
                                    CXXVBTableInfo* info,
                                    SourceLocation location) {
  Symbol* this_symbol = CXXThisSymbolFromFunction(func);
  if (this_symbol == NULL || info == NULL || info->source == NULL) {
    return NULL;
  }
  ASTNode* receiver = NewIdentifierASTNode(this_symbol, location);
  TypeRecord* source_pointer =
      NewPointerTo(kQualPlain, NewCXXStructType(info->source));
  ASTNode* offset =
      NewIntConstantASTNode(info->source_offset,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* adjusted =
      NewBinaryASTNode(AST_OP(plus), source_pointer, location, receiver,
                       offset);
  adjusted->flags |= kASTAnalyzed;
  return adjusted;
}

static ASTNode* NewCXXVBPtrInitializer(TypeRecord* func,
                                       CXXVBTableInfo* info,
                                       SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) || info == NULL ||
      info->symbol == NULL) {
    return NULL;
  }
  ASTNode* receiver = NewCXXVBPtrReceiver(func, info, location);
  if (receiver == NULL) {
    return NULL;
  }
  ASTNode* target =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver,
                       NewStringConstantASTNode(NewString("__vbptr"), NULL,
                                                location));
  ASTNode* value = NewIdentifierASTNode(info->symbol, location);
  return NewExpressionStatementASTNode(
      NewBinaryASTNode(AST_OP(assign), info->symbol->type, location, target,
                       value),
      location);
}

static void AppendCXXVBPtrInitializers(TypeRecord* func, Vector* body,
                                       SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL || body == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < owner->vbtable_symbols.length; i++) {
    CXXVBTableInfo* info = owner->vbtable_symbols.value.p[i];
    ASTNode* init = NewCXXVBPtrInitializer(func, info, location);
    if (init != NULL) {
      VectorAppend(body, init);
    }
  }
}

static Symbol* CXXCompleteObjectSymbolFromFunction(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.prototype.length < 2 ||
      func->info.function.cxx_member_owner == NULL ||
      !StructHasVirtualBases(func->info.function.cxx_member_owner) ||
      (!func->info.function.is_constructor &&
       !func->info.function.is_destructor)) {
    return NULL;
  }
  return func->info.function.prototype.value.p[1];
}

static ASTNode* NewCXXCompleteObjectGuardedStatement(TypeRecord* func,
                                                    Vector* statements,
                                                    SourceLocation location) {
  if (statements == NULL || statements->length == 0) {
    if (statements != NULL) {
      VectorDelete(statements);
    }
    return NULL;
  }
  ASTNode* body = NewCompoundStatementASTNode(statements, location);
  Symbol* complete_object = CXXCompleteObjectSymbolFromFunction(func);
  if (complete_object == NULL) {
    return body;
  }
  return NewIfStatementASTNode(NewIdentifierASTNode(complete_object, location),
                               body, NULL, false, location);
}

static void InsertCXXCompleteObjectGuardedStatements(TypeRecord* func,
                                                    Vector* body,
                                                    size_t* insert_at,
                                                    Vector* statements,
                                                    SourceLocation location) {
  ASTNode* guarded =
      NewCXXCompleteObjectGuardedStatement(func, statements, location);
  if (guarded == NULL) {
    return;
  }
  VectorInsertOrAppend(body, *insert_at, guarded);
  (*insert_at)++;
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
    CXXPrependCompleteObjectArgument(member_type, actuals,
                                     /*complete_object=*/true, location);
    ASTNode* member_name =
        NewStringConstantASTNode(NewString(constructor_name), NULL, location);
    ASTNode* member_access =
        NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member_name);
    return NewExpressionStatementASTNode(
        NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals),
        location);
  }

  if (actuals->length == 0) {
    VectorDelete(actuals);
    if (TypeIsStructOrUnion(member_type)) {
      return NULL;
    }
    ASTNode* target =
        NewCXXThisMemberAccess(func, member->symbol->name.value, location);
    if (target == NULL) {
      return NULL;
    }
    ASTNode* value = NewIntConstantASTNode(
        0, TypeRecordCopy(member_type), location);
    ASTNode* assign =
        NewBinaryASTNode(AST_OP(assign), member_type, location, target, value);
    assign->flags |= kASTCXXMemberInitializer;
    return NewExpressionStatementASTNode(assign, location);
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
  ASTNode* assign =
      NewBinaryASTNode(AST_OP(assign), member_type, location, target, value);
  assign->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(assign, location);
}

ASTNode* SyntaxNewCXXMemberInitializerStatement(
    Syntax* syntax, TypeRecord* func, StructMember* member, Vector* actuals,
    SourceLocation location) {
  return NewCXXMemberInitializerStatement(syntax, func, member, actuals,
                                          location);
}

static Vector* CXXDefaultMemberInitializerActuals(ASTNode* initializer) {
  if (initializer == NULL) {
    return NULL;
  }
  Vector* actuals = NewVector();
  if (initializer->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)initializer;
    VectorAppend(actuals,
                 ASTNodeClone(expr_init->expr, IdentityCloneNode, NULL, NULL));
    return actuals;
  }
  if (initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* init = braced->initializers->value.p[i];
      if (init != NULL && init->op == AST_OP(expr_init)) {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)init;
        VectorAppend(actuals,
                     ASTNodeClone(expr_init->expr, IdentityCloneNode, NULL,
                                  NULL));
      } else {
        VectorAppend(actuals, CloneInitializer(init));
      }
    }
    return actuals;
  }
  VectorAppend(actuals, ASTNodeClone(initializer, IdentityCloneNode, NULL, NULL));
  return actuals;
}

static ASTNode* NewCXXDefaultMemberInitializerStatement(Syntax* syntax,
                                                       TypeRecord* func,
                                                       StructMember* member) {
  if (member == NULL || member->default_initializer == NULL) {
    return NULL;
  }
  Vector* actuals = CXXDefaultMemberInitializerActuals(
      member->default_initializer);
  return NewCXXMemberInitializerStatement(syntax, func, member, actuals,
                                          member->default_initializer->location);
}

static ASTNode* FindCXXExplicitMemberInitializer(CXXConstructorInitList* init_list,
                                                StructMember* member) {
  for (size_t i = 0; i < init_list->member_specs.length; i++) {
    if (init_list->member_specs.value.p[i] == member) {
      return init_list->member_statements.value.p[i];
    }
  }
  return NULL;
}

static ASTNode* FindCXXExplicitVirtualBaseInitializer(
    CXXConstructorInitList* init_list, CXXVirtualBaseInfo* base) {
  for (size_t i = 0; i < init_list->virtual_base_specs.length; i++) {
    if (init_list->virtual_base_specs.value.p[i] == base) {
      return init_list->virtual_base_statements.value.p[i];
    }
  }
  return NULL;
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
    VectorAppend(&init_list->raw_initializers,
                 NewCXXDeferredConstructorInitializer(
                     init_name, CloneCXXConstructorInitializerActuals(actuals),
                     location));
    CXXBaseSpecifier* base = FindCXXDirectBaseByName(owner, init_name);
    CXXVirtualBaseInfo* virtual_base = NULL;
    if (base != NULL && base->is_virtual) {
      virtual_base = FindCXXVirtualBaseByName(owner, init_name);
      base = NULL;
    }
    if (base != NULL) {
      if (VectorContainsPointer(&init_list->base_specs, base)) {
        SyntaxError(syntax, "Duplicate initializer for base %s", init_name);
        VectorDelete(actuals);
        FullyQualifiedIdentifierDestruct(&name);
        if (!LexMatch(syntax->lex, TOK(comma))) {
          break;
        }
        continue;
      }
      CheckCXXConstructorInitializerOrder(
          syntax, init_list, init_name, CXXDirectBaseOrder(owner, base));
      ASTNode* call = NewCXXBaseSpecialMemberCall(syntax, func, base, false,
                                                  /*complete_object=*/false,
                                                  actuals, location);
      if (call != NULL) {
        VectorAppend(&init_list->base_specs, base);
        VectorAppend(&init_list->base_statements, call);
      }
    } else if ((virtual_base = virtual_base != NULL
                                   ? virtual_base
                                   : FindCXXVirtualBaseByName(owner,
                                                              init_name)) != NULL) {
      if (VectorContainsPointer(&init_list->virtual_base_specs, virtual_base)) {
        SyntaxError(syntax, "Duplicate initializer for virtual base %s",
                    init_name);
        VectorDelete(actuals);
        FullyQualifiedIdentifierDestruct(&name);
        if (!LexMatch(syntax->lex, TOK(comma))) {
          break;
        }
        continue;
      }
      CheckCXXConstructorInitializerOrder(
          syntax, init_list, init_name,
          CXXVirtualBaseOrder(owner, virtual_base));
      ASTNode* call = NewCXXVirtualBaseSpecialMemberCall(
          syntax, func, virtual_base, false, actuals, location);
      if (call != NULL) {
        VectorAppend(&init_list->virtual_base_specs, virtual_base);
        VectorAppend(&init_list->virtual_base_statements, call);
      }
    } else {
      StructMember* member = FindCXXDirectDataMemberByName(owner, init_name);
      if (member == NULL) {
        VectorAppend(&init_list->deferred_initializers,
                     NewCXXDeferredConstructorInitializer(init_name, actuals,
                                                          location));
      } else if (VectorContainsPointer(&init_list->member_specs, member)) {
        SyntaxError(syntax, "Duplicate initializer for member %s", init_name);
        VectorDelete(actuals);
      } else {
        CheckCXXConstructorInitializerOrder(
            syntax, init_list, init_name,
            CXXDirectMemberOrder(owner, member));
        ASTNode* stmt = NewCXXMemberInitializerStatement(
            syntax, func, member, actuals, location);
        if (stmt != NULL) {
          VectorAppend(&init_list->member_specs, member);
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

void SyntaxResolveCXXConstructorInitializerList(
    Syntax* syntax, TypeRecord* func, CXXConstructorInitList* init_list) {
  if (!CompilerIsCXX() || func == NULL || !func->info.function.is_constructor ||
      func->info.function.cxx_member_owner == NULL || init_list == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < init_list->deferred_initializers.length; i++) {
    CXXDeferredConstructorInitializer* deferred =
        init_list->deferred_initializers.value.p[i];
    if (deferred == NULL || deferred->actuals == NULL) {
      continue;
    }
    const char* init_name = deferred->name.value;
    Vector* actuals = deferred->actuals;
    deferred->actuals = NULL;
    SourceLocation location = deferred->location;
    CXXBaseSpecifier* base = FindCXXDirectBaseByName(owner, init_name);
    CXXVirtualBaseInfo* virtual_base = NULL;
    if (base != NULL && base->is_virtual) {
      virtual_base = FindCXXVirtualBaseByName(owner, init_name);
      base = NULL;
    }
    if (base != NULL) {
      if (VectorContainsPointer(&init_list->base_specs, base)) {
        SyntaxError(syntax, "Duplicate initializer for base %s", init_name);
        VectorDelete(actuals);
        continue;
      }
      CheckCXXConstructorInitializerOrder(
          syntax, init_list, init_name, CXXDirectBaseOrder(owner, base));
      ASTNode* call = NewCXXBaseSpecialMemberCall(syntax, func, base, false,
                                                  /*complete_object=*/false,
                                                  actuals, location);
      if (call != NULL) {
        VectorAppend(&init_list->base_specs, base);
        VectorAppend(&init_list->base_statements, call);
      }
      continue;
    }
    virtual_base = virtual_base != NULL
                       ? virtual_base
                       : FindCXXVirtualBaseByName(owner, init_name);
    if (virtual_base != NULL) {
      if (VectorContainsPointer(&init_list->virtual_base_specs, virtual_base)) {
        SyntaxError(syntax, "Duplicate initializer for virtual base %s",
                    init_name);
        VectorDelete(actuals);
        continue;
      }
      CheckCXXConstructorInitializerOrder(
          syntax, init_list, init_name,
          CXXVirtualBaseOrder(owner, virtual_base));
      ASTNode* call = NewCXXVirtualBaseSpecialMemberCall(
          syntax, func, virtual_base, false, actuals, location);
      if (call != NULL) {
        VectorAppend(&init_list->virtual_base_specs, virtual_base);
        VectorAppend(&init_list->virtual_base_statements, call);
      }
      continue;
    }
    StructMember* member = FindCXXDirectDataMemberByName(owner, init_name);
    if (member == NULL) {
      SyntaxError(syntax, "%s is not a direct base or member of %s",
                  init_name,
                  owner->tag_name != NULL ? owner->tag_name->value
                                          : "<anonymous>");
      VectorDelete(actuals);
      continue;
    }
    if (VectorContainsPointer(&init_list->member_specs, member)) {
      SyntaxError(syntax, "Duplicate initializer for member %s", init_name);
      VectorDelete(actuals);
      continue;
    }
    CheckCXXConstructorInitializerOrder(
        syntax, init_list, init_name, CXXDirectMemberOrder(owner, member));
    ASTNode* stmt = NewCXXMemberInitializerStatement(
        syntax, func, member, actuals, location);
    if (stmt != NULL) {
      VectorAppend(&init_list->member_specs, member);
      VectorAppend(&init_list->member_statements, stmt);
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
  SyntaxResolveCXXConstructorInitializerList(syntax, func, init_list);
  size_t insert_at = 0;
  Vector* complete_initializers = NewVector();
  AppendCXXVBPtrInitializers(func, complete_initializers, location);
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    ASTNode* call =
        FindCXXExplicitVirtualBaseInitializer(init_list, base);
    if (call == NULL &&
        !VectorContainsPointer(&init_list->virtual_base_specs, base)) {
      call = NewCXXVirtualBaseSpecialMemberCall(syntax, func, base, false,
                                                NULL, location);
    }
    if (call == NULL) {
      continue;
    }
    VectorAppend(complete_initializers, call);
  }
  InsertCXXCompleteObjectGuardedStatements(func, body, &insert_at,
                                           complete_initializers, location);
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base->is_virtual) {
      continue;
    }
    ASTNode* call = NULL;
    for (size_t j = 0; j < init_list->base_specs.length; j++) {
      if (init_list->base_specs.value.p[j] == base) {
        call = init_list->base_statements.value.p[j];
        break;
      }
    }
    if (call == NULL && !VectorContainsPointer(&init_list->base_specs, base)) {
      call = NewCXXBaseSpecialMemberCall(syntax, func, base, false,
                                         /*complete_object=*/false, NULL,
                                         location);
    }
    if (call == NULL) {
      continue;
    }
    VectorInsertOrAppend(body, insert_at, call);
    insert_at++;
  }
  if (owner->vtables_registered) {
    Vector* vptr_initializers = NewVector();
    AppendCXXVPtrInitializers(func, vptr_initializers, location);
    for (size_t i = 0; i < vptr_initializers->length; i++) {
      VectorInsertOrAppend(body, insert_at, vptr_initializers->value.p[i]);
      insert_at++;
    }
    VectorDelete(vptr_initializers);
  } else {
    // The class is still being parsed, so its vtables (and the symbols the
    // __vptr initializers must reference) do not exist yet.  Defer the
    // insertion; SyntaxFlushPendingVPtrInitializers fills it in at `insert_at`
    // -- right after the base-class initializers -- once the vtables are
    // registered.  Member initializers and the user body, inserted after this
    // point, sit at higher indices and remain correctly ordered.
    if (!pending_vptr_inits_initialized) {
      VectorInit(&pending_vptr_inits);
      pending_vptr_inits_initialized = true;
    }
    PendingVPtrInit* pending = malloc(sizeof(PendingVPtrInit));
    pending->owner = owner;
    pending->func = func;
    pending->index = insert_at;
    pending->location = location;
    VectorAppend(&pending_vptr_inits, pending);
  }
  Vector* complete_restores = NewVector();
  AppendCXXVBPtrInitializers(func, complete_restores, location);
  InsertCXXCompleteObjectGuardedStatements(func, body, &insert_at,
                                           complete_restores, location);
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || StorageIs(member->symbol->storage,
                                                STO(typedef))) {
      continue;
    }
    ASTNode* stmt = FindCXXExplicitMemberInitializer(init_list, member);
    if (stmt == NULL) {
      stmt = NewCXXDefaultMemberInitializerStatement(syntax, func, member);
    }
    if (stmt == NULL) {
      continue;
    }
    VectorInsertOrAppend(body, insert_at, stmt);
    insert_at++;
  }
}

void SyntaxFlushPendingVPtrInitializers(Struct* owner) {
  if (!pending_vptr_inits_initialized || owner == NULL) {
    return;
  }
  size_t out = 0;
  for (size_t i = 0; i < pending_vptr_inits.length; i++) {
    PendingVPtrInit* pending = pending_vptr_inits.value.p[i];
    if (pending->owner != owner) {
      // Keep entries for other (e.g. enclosing) classes still being parsed.
      pending_vptr_inits.value.p[out++] = pending;
      continue;
    }
    ASTNode* body = pending->func->info.function.body;
    if (body != NULL && body->op == AST_OP(compound)) {
      CompoundStatementASTNode* compound = (CompoundStatementASTNode*)body;
      Vector* vptr_initializers = NewVector();
      AppendCXXVPtrInitializers(pending->func, vptr_initializers,
                                pending->location);
      // VectorInsertOrAppend handles index == length (e.g. an empty body whose
      // only statements are the deferred __vptr stores), which the strict
      // VectorInsertBefore inside CompoundASTNodeInsertStatement does not.
      size_t at = pending->index;
      for (size_t j = 0; j < vptr_initializers->length; j++) {
        ASTNode* stmt = vptr_initializers->value.p[j];
        VectorInsertOrAppend(compound->statements, at, stmt);
        stmt->parent = &compound->base;
        at++;
      }
      if (vptr_initializers->length > 0) {
        for (size_t k = 0; k < compound->statements->length; k++) {
          ((ASTNode*)compound->statements->value.p[k])->child_id = (int)k;
        }
      }
      VectorDelete(vptr_initializers);
    }
    free(pending);
  }
  pending_vptr_inits.length = out;
}

static void ParseCXXDefaultDeleteFunctionSpecifier(Syntax* syntax,
                                                   TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      !LexMatch(syntax->lex, TOK(equal))) {
    return;
  }
  if (LexMatch(syntax->lex, TOK(default))) {
    func->info.function.is_defaulted = true;
    func->info.function.is_explicitly_defaulted = true;
    func->info.function.is_constexpr_eligible = true;
    func->info.function.is_inline = true;
    return;
  }
  if (LexMatch(syntax->lex, TOK(delete))) {
    func->info.function.is_deleted = true;
    func->info.function.is_explicitly_deleted = true;
    return;
  }
  SyntaxError(syntax, "function specifier must be '= default' or '= delete'");
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

  if (sym->type->info.function.is_defaulted) {
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->value.func_defn = sym;
      old_sym->type->info.function.is_defaulted = true;
    }
    ParserContext old_context = syntax->context;
    syntax->context = kParsingBlockScope;
    SyntaxOpenScope(syntax);
    AddFunctionScopeSymbols(syntax, sym->type);
    sym->flags.is_defined = true;
    sym->type->info.function.definition = true;
    sym->type->info.function.is_user_provided = true;
    Vector* body = NewVector();
    SyntaxInsertCXXConstructorPreamble(syntax, sym->type, body,
                                       &cxx_initializers, sym->location);
    AppendCXXDefaultedMemberwiseAssignments(sym->type, body, sym->location);
    AppendCXXDefaultedAssignmentReturnThis(sym->type, body, sym->location);
    SyntaxAppendCXXMemberDestructorCalls(syntax, sym->type, body, sym->location);
    AppendCXXBaseDestructorCalls(syntax, sym->type, body, sym->location);
    SyntaxCloseScope(syntax);
    syntax->context = old_context;
    sym->type->info.function.body =
        NewCompoundStatementASTNode(body, sym->location);
    VectorAppend(&compiler->declaration_asts, sym->type->info.function.body);
    SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
    SyntaxNeedSemicolon(syntax, TC(decl));
    return NewVariableDeclarationASTNode(sym, NULL, sym->location);
  }

  if (sym->type->info.function.is_deleted) {
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->type->info.function.is_deleted = true;
    }
    SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
    return NULL;
  }

  if (LexMatch(syntax->lex, TOK(lbrace))) {
    if (sym->type->info.function.old_style) {
      SyntaxWarning(syntax, "old-style-definition",
                    "old-style function definition");
    }
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->value.func_defn = sym;
      if (old_sym->type->info.function.is_constexpr) {
        sym->type->info.function.is_constexpr = true;
      }
      if (old_sym->type->info.function.is_consteval) {
        sym->type->info.function.is_consteval = true;
        sym->type->info.function.is_constexpr = true;
      }
      if (old_sym->flags.is_weak) {
        sym->flags.is_weak = true;
      }
      if (CompilerIsCXX() && old_sym->type->info.function.is_inline) {
        sym->type->info.function.is_inline = true;
      }
    }
    ParserContext old_context = syntax->context;
    syntax->context = kParsingBlockScope;
    SyntaxOpenScope(syntax);
    AddFunctionScopeSymbols(syntax, sym->type);
    
    sym->flags.is_defined = true;
    sym->type->info.function.definition = true;
    if (CompilerIsCXX() && sym->type->info.function.is_inline) {
      sym->flags.is_inline_defn = true;
      if (!StorageIs(sym->storage, STO(static))) {
        sym->flags.is_weak = true;
      }
      if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
        old_sym->type->info.function.is_inline = true;
      }
    }
    
    // Parse the function body.
    TypeRecord* old_current_function = compiler->current_function;
    compiler->current_function = sym->type;
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
    SyntaxAppendCXXMemberDestructorCalls(syntax, sym->type, body, sym->location);
    AppendCXXBaseDestructorCalls(syntax, sym->type, body, sym->location);
    SyntaxCloseScope(syntax);
    syntax->context = old_context;
    compiler->current_function = old_current_function;
    
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
    // an inline definition in C. C++ emits inline definitions directly.
    if (!CompilerIsCXX() && old_sym != NULL &&
        old_sym->type->info.function.is_inline) {
      old_sym->flags.is_inline_defn = true;
    }
  }
  SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
  return NULL;
}

static void ParseDeclarationSpecifier(Syntax* syntax, Storage* storage,
                                      bool* is_inline, bool* is_constexpr,
                                      bool* is_consteval, bool* is_constinit,
                                      bool* is_explicit, TypeRecord** type,
                                      Vector* attributes,
                                      ParserContext context);

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
  StructAddFriendFunction(befriending, friend_symbol);
  if (definition != NULL) {
    // Inline friend definitions belong to the enclosing namespace; queue them
    // alongside the other deferred definitions so codegen emits the body.
    VectorAppend(&compiler->pending_template_instantiations, definition);
  }
}

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
static void SyntaxDeferTemplateFriendFunction(Syntax* syntax,
                                              Struct* befriending, Symbol* sym) {
  sym->namespace_ = syntax->current_namespace;
  ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type);
  if (LexLookingAt(syntax->lex, TOK(lbrace))) {
    // Parse and retain the inline body on sym->type; intentionally do not queue
    // the returned definition for emission - that happens per instantiation.
    Vector* friend_decls = NewVector();
    ASTNode* definition =
        DeclareOrDefineFunction(syntax, friend_decls, sym, NULL);
    if (definition == NULL) {
      VectorDelete(friend_decls);
    } else {
      // Retain as a teardown root so the template body outlives parsing.
      VectorAppend(&compiler->declaration_asts, definition);
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
    Symbol* matching_overload = FindMatchingOverload(old_sym, sym->type);
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

void SyntaxParseFriendDeclaration(Syntax* syntax, Struct* befriending) {
  LexMatch(syntax->lex, TOK(friend));

  // A friend declaration introduces its name (a forward-declared friend class
  // tag, or the friend function) into the nearest enclosing namespace scope,
  // not into the class being defined.  Suspend the enclosing class's tag scope
  // so any newly created tag is routed to the namespace and unifies with a
  // later definition.
  LocalSymbolTable* saved_tag_stack = syntax->local_tag_stack;
  syntax->local_tag_stack = NULL;

  Vector attributes = {0};
  VectorInit(&attributes);
  Storage storage = STO(implicit);
  bool is_inline = false;
  bool is_constexpr = false;
  bool is_consteval = false;
  bool is_constinit = false;
  bool is_explicit = false;
  TypeRecord* type = NULL;
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &is_constexpr,
                            &is_consteval, &is_constinit, &is_explicit, &type,
                            &attributes, kParsingFileScope);

  TypeRecordIncRef(type);

  // 'friend class X;' / 'friend struct X;' / 'friend X;' — a type-specifier
  // with no declarator names a class to befriend.
  if (LexLookingAt(syntax->lex, TOK(semicolon))) {
    if (type != NULL && TypeIsStructOrUnion(type) &&
        type->info.struct_info != NULL) {
      Struct* friend_class =
          ResolveFriendClassFromEnclosingClasses(befriending, type);
      StructAddFriendClass(
          befriending,
          friend_class != NULL ? friend_class : type->info.struct_info);
    } else {
      SyntaxError(syntax,
                  "friend declaration does not name a class or a function");
    }
    LexMatch(syntax->lex, TOK(semicolon));
    TypeRecordDelete(type);
    AttributeListDestruct(&attributes);
    syntax->local_tag_stack = saved_tag_stack;
    return;
  }

  // Otherwise this is a friend function declaration or inline definition.  It
  // belongs to the nearest enclosing namespace scope, not to the class, so we
  // declare it there and merge with any existing overloads exactly as a normal
  // namespace-scope function declaration would.
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingFileScope);
  parser.is_inline = is_inline;
  parser.is_constexpr = is_constexpr;
  parser.is_consteval = is_consteval;
  parser.is_constinit = is_constinit;
  // Let the friend's signature name the class currently being defined via its
  // own template-id (e.g. 'friend f(const Box<T>&)' inside 'template Box').
  // This only affects type resolution; the friend is still declared at the
  // enclosing namespace scope, not as a member of the class.
  parser.cxx_member_owner = befriending;
  SyntaxOpenScope(syntax);

  Symbol* sym = TypeParserParseDeclarator(&parser, type);
  if (sym == NULL || !TypeIsFunction(sym->type)) {
    if (sym == NULL) {
      SyntaxError(syntax, "expected a friend function declaration");
    } else {
      SyntaxError(syntax, "a friend declaration must name a function or class");
    }
    SyntaxRecover(syntax, TC(semicolon));
    LexMatch(syntax->lex, TOK(semicolon));
    SyntaxCloseScope(syntax);
    TypeParserDestruct(&parser);
    TypeRecordDelete(type);
    AttributeListDestruct(&attributes);
    syntax->local_tag_stack = saved_tag_stack;
    return;
  }

  sym->type->info.function.is_explicit = false;

  // 'friend void A::f();' names an existing member function of another class.
  // It refers to that member rather than introducing a namespace-scope function,
  // so record the friendship against the existing member and do not redeclare
  // or define it here.
  if (parser.cxx_member_definition != NULL &&
      parser.cxx_member_definition->symbol != NULL) {
    StructAddFriendFunction(befriending, parser.cxx_member_definition->symbol);
    SyntaxNeedSemicolon(syntax, TC(decl));
    SyntaxCloseScope(syntax);
    TypeParserDestruct(&parser);
    TypeRecordDelete(type);
    AttributeListDestruct(&attributes);
    syntax->local_tag_stack = saved_tag_stack;
    return;
  }

  // A friend declared inside a class template is dependent: defer its
  // registration and code emission until each specialization is instantiated,
  // where the signature (and any inline body) is substituted with the template
  // arguments.  Friend *classes* are handled separately above and are unchanged.
  if (syntax->parsing_template_declaration) {
    SyntaxDeferTemplateFriendFunction(syntax, befriending, sym);
    SyntaxCloseScope(syntax);
    TypeParserDestruct(&parser);
    TypeRecordDelete(type);
    AttributeListDestruct(&attributes);
    syntax->local_tag_stack = saved_tag_stack;
    return;
  }

  Symbol* old_sym = FindFileScopeSymbol(syntax, &sym->name);
  bool overload_was_appended = false;
  if (CanOverloadFunctions(old_sym, sym)) {
    Symbol* matching_overload = FindMatchingOverload(old_sym, sym->type);
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

  ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type);
  Vector* friend_decls = NewVector();
  Struct* saved_access_context = compiler->current_class_access_context;
  compiler->current_class_access_context = befriending;
  ASTNode* definition =
      DeclareOrDefineFunction(syntax, friend_decls, sym, old_sym);
  compiler->current_class_access_context = saved_access_context;
  RecordFriendFunction(syntax, befriending, sym, in_scope_symbol, definition);
  if (definition == NULL) {
    // Declaration only: DeclareOrDefineFunction did not adopt the vector.
    VectorDelete(friend_decls);
    SyntaxNeedSemicolon(syntax, TC(decl));
  }

  SyntaxCloseScope(syntax);
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);
  AttributeListDestruct(&attributes);
  syntax->local_tag_stack = saved_tag_stack;
}

static bool IsPowerOf2OrZero(int32_t v) {
  return (v & (v - 1)) == 0;
}

static const char* ThreadLocalDiagnosticKeyword(void) {
  return CompilerIsCXX() ? "thread_local" : "__thread";
}

// thread_local / __thread imply static storage duration at block scope only.
static void NormalizeThreadLocalStorage(Storage* storage,
                                      ParserContext context) {
  if (!StorageIs(*storage, STO(thread)) || !CompilerIsCXX()) {
    return;
  }
  if (context == kParsingBlockScope &&
      !StorageIs(*storage, STO(extern))) {
    *storage |= STO(static);
  }
}

// Validate thread-local storage for the current language and scope.
void SyntaxCheckThreadLocal(Syntax* syntax, Symbol* symbol,
                            ParserContext context, bool is_static_member,
                            bool is_nonstatic_member) {
  if (!StorageIs(symbol->storage, STO(thread))) {
    return;
  }

  const char* keyword = ThreadLocalDiagnosticKeyword();
  if (TypeIsFunction(symbol->type) || symbol->flags.is_argument) {
    SyntaxError(syntax, "Illegal use of %s", keyword);
    return;
  }
  if (is_nonstatic_member) {
    SyntaxError(syntax, "Non-static data member cannot be %s", keyword);
    return;
  }
  if (CompilerIsCXX()) {
    return;
  }

  // C: __thread at block scope requires an explicit static or extern storage
  // class.  File-scope behavior is left unchanged.
  if (context == kParsingBlockScope &&
      !StorageIs(symbol->storage, STO(static) | STO(extern))) {
    SyntaxError(syntax, "Illegal use of %s", keyword);
  }
}

// A declaration specifier is a set of:
// 1. storage specifier
// 2. type specifier
// 3. function specifier (inline).
// This collects them into the output variables.
static bool ParseCXXExplicitDeclarationSpecifier(Syntax* syntax,
                                                 bool* saw_explicit) {
  *saw_explicit = false;
  if (!CompilerIsCXX() || !LexMatch(syntax->lex, TOK(explicit))) {
    return false;
  }
  *saw_explicit = true;
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return true;
  }
  ASTNode* expr =
      SyntaxParseExpression(syntax, TC(closebra) | TC(exprsep));
  // A value-dependent condition (e.g. `explicit(sizeof(T) > 4)`) is detected on
  // the parsed (pre-analysis) tree, before constant-folding could collapse it
  // to the placeholder's size.  The unanalyzed condition is stashed on the
  // function's FunctionInfo and re-folded per instantiation (see
  // InstantiateMemberFunctionType); mirrors static_assert deferral.
  if (syntax->parsing_template_declaration &&
      ExpressionIsTemplateDependent(expr)) {
    SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));
    syntax->pending_explicit_condition = expr;
    return true;
  }
  expr = AnalyzeExpression(expr);
  int64_t value = 0;
  bool ok = EvaluateIntegerExpression(expr, &value);
  if (!ok) {
    SyntaxError(syntax, "explicit specifier must be a constant expression");
  }
  ASTNodeDelete(expr);
  SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));
  return ok && value != 0;
}

static void ParseDeclarationSpecifier(Syntax* syntax, Storage* storage,
                                      bool* is_inline, bool* is_constexpr,
                                      bool* is_consteval, bool* is_constinit,
                                      bool* is_explicit, TypeRecord** type,
                                      Vector* attributes,
                                      ParserContext context) {
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
    } else if (LexLookingAt(syntax->lex, TOK(explicit))) {
      bool saw_explicit = false;
      bool explicit_value =
          ParseCXXExplicitDeclarationSpecifier(syntax, &saw_explicit);
      if (saw_explicit && *is_explicit) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate 'explicit' specifier");
      }
      *is_explicit = *is_explicit || explicit_value;
    } else if (LexMatch(syntax->lex, TOK(constexpr))) {
      if (*is_constexpr) {
        SyntaxError(syntax, "Duplicate 'constexpr' specifier");
      }
      *is_constexpr = true;
    } else if (LexMatch(syntax->lex, TOK(consteval))) {
      if (*is_consteval) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate 'consteval' specifier");
      }
      *is_consteval = true;
      *is_constexpr = true;
    } else if (LexMatch(syntax->lex, TOK(constinit))) {
      if (*is_constinit) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate 'constinit' specifier");
      }
      *is_constinit = true;
    } else if (!(type_specifier.type != kTypeImplicit &&
                 SyntaxCurrentIdentifierFollowedByScopeOperator(syntax)) &&
               SyntaxLookingAtType(syntax) &&
               (type_specifier.type & (kTypeStruct | kTypeUnion | kTypeEnum)) == 0) {
      // Guard against a type specifier that consumes no input (e.g. a stray
      // `::` that cannot begin a new specifier): if the lexer does not
      // advance, stop rather than spinning forever.
      size_t prev_pos = syntax->lex->pos;
      Token prev_token = syntax->lex->current_token;
      type_specifier = TypeParserParseAndCombineTypes(&parser, &type_specifier);
      if (syntax->lex->pos == prev_pos &&
          syntax->lex->current_token == prev_token) {
        *type = TypeParserBuildTypeRecord(&parser, &type_specifier);
        if (parser.placeholder_variable_constraint != NULL) {
          ConstraintExprDelete(syntax->pending_placeholder_variable_constraint);
          syntax->pending_placeholder_variable_constraint =
              parser.placeholder_variable_constraint;
          parser.placeholder_variable_constraint = NULL;
        }
        TypeParserDestruct(&parser);
        return;
      }
    } else if (SyntaxParseCXXAlignas(syntax, attributes)) {
      continue;
    } else if (LexLookingAt(syntax->lex, TOK(attribute)) ||
               SyntaxLookingAtCXXAttribute(syntax)) {
      if (LexMatch(syntax->lex, TOK(attribute))) {
        SyntaxParseAttribute(syntax, attributes);
      } else {
        SyntaxParseCXXAttributes(syntax, attributes);
      }
    } else {
      if (type_specifier.type == kTypeImplicit &&
          type_specifier.type_record == NULL) {
        SyntaxWarning(syntax, "implicit-int",
                      "type specifier missing, defaults to int");
      }
      *type = TypeParserBuildTypeRecord(&parser, &type_specifier);
      if (parser.placeholder_variable_constraint != NULL) {
        ConstraintExprDelete(syntax->pending_placeholder_variable_constraint);
        syntax->pending_placeholder_variable_constraint =
            parser.placeholder_variable_constraint;
        parser.placeholder_variable_constraint = NULL;
      }
      NormalizeThreadLocalStorage(storage, context);
      TypeParserDestruct(&parser);
      return;
    }
  }
  NormalizeThreadLocalStorage(storage, context);
  TypeParserDestruct(&parser);
}

static bool TypeContainsClassTemplate(TypeRecord* type);
static int CurrentTemplateParameterListLength(Syntax* syntax);
static int CurrentTemplateParameterBase(Syntax* syntax);
static void MoveCurrentTemplateParametersToFunction(Syntax* syntax,
                                                    TypeRecord* func);
static void AddFunctionAssociatedConstraint(TypeRecord* func,
                                            ConstraintExpr* constraint);

static Symbol* CXXClassTemplateOrigin(TypeRecord* type) {
  Symbol* placeholder_origin = TypeClassTemplatePlaceholderOrigin(type);
  if (placeholder_origin != NULL) {
    return placeholder_origin;
  }
  for (TypeRecord* t = type; t != NULL; t = t->next) {
    if (TypeIsStructOrUnion(t) && t->template_origin != NULL) {
      return t->template_origin;
    }
  }
  return NULL;
}

static bool TryParseCXXDeductionGuide(Syntax* syntax, Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      !TypeIsFunction(sym->type) || !LexLookingAt(syntax->lex, TOK(arrow))) {
    return false;
  }
  Symbol* declared_template = CXXClassTemplateOrigin(sym->type->next);

  LexNextToken(syntax->lex);
  TypeParser return_parser;
  TypeParserInit(&return_parser, syntax->lex, syntax, STO(implicit),
                 syntax->context);
  TypeRecord* guide_return = TypeParserParseType(&return_parser, true);
  TypeParserDestruct(&return_parser);
  Symbol* deduced_template = CXXClassTemplateOrigin(guide_return);
  if (deduced_template == NULL) {
    SyntaxError(syntax, "Deduction guide return type must be a class template-id");
  } else if (declared_template != NULL && deduced_template != declared_template) {
    SyntaxError(syntax, "Deduction guide return type does not match %s",
                declared_template->name.value);
  }
  Symbol* class_template =
      deduced_template != NULL ? deduced_template : declared_template;
  if (class_template == NULL) {
    TypeRecordDelete(guide_return);
    SymbolDelete(sym);
    return true;
  }

  if (sym->type->next != NULL) {
    TypeRecordDelete(sym->type->next);
  }
  TypeRecordIncRef(guide_return);
  sym->type->next = guide_return;
  sym->type->info.function.is_deduction_guide = true;
  if (syntax->parsing_template_declaration &&
      sym->type->info.function.template_parameters.length == 0) {
    MoveCurrentTemplateParametersToFunction(syntax, sym->type);
  }
  TypeAddCXXDeductionGuide(class_template, sym);
  return true;
}

static ASTNode* ParseExternalDeclarationList(TypeParser* parser,
                                         TypeRecord* type,
                                         Storage storage,
                                         bool is_explicit,
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
    // True once we discover this declarator is an out-of-class definition of a
    // static data member that was already defined in-class (a C++17 inline /
    // constexpr static data member).  Such an out-of-class `T C::m;` is a
    // deprecated non-defining redeclaration and must not emit a second symbol.
    bool redundant_static_member_redefinition = false;
    if (sym != NULL) {
      // A declaration appearing inside an `extern "C"` linkage specification
      // has C language linkage, so its name is not mangled.
      if (syntax->extern_c_depth > 0) {
        sym->flags.is_c_linkage = true;
      }
      // `constexpr` on an object implies `const` on its type.  Apply it before
      // matching against any previous declaration so that an out-of-class
      // definition (`constexpr T C::x;`) compares equal to the in-class
      // `static constexpr` member, whose type is already const-qualified.
      if (!TypeIsFunction(sym->type) && parser->is_constexpr) {
        sym->type->qualifiers |= kQualConst;
      }
      if (TypeIsFunction(sym->type) &&
          sym->type->info.function.template_parameters.length > 0 &&
          !sym->flags.is_template) {
        sym->flags.is_template = true;
        sym->type->info.function.template_parameter_count =
            (int)sym->type->info.function.template_parameters.length;
        sym->type->info.function.template_parameter_base = 0;
      }
      if (syntax->parsing_template_declaration && TypeIsFunction(sym->type)) {
        sym->flags.is_template = true;
        sym->type->info.function.template_parameter_count =
            CurrentTemplateParameterListLength(syntax);
        sym->type->info.function.template_parameter_base =
            CurrentTemplateParameterBase(syntax);
      }
      if (TypeIsFunction(sym->type)) {
        sym->type->info.function.is_explicit = is_explicit;
        if (syntax->pending_explicit_condition != NULL) {
          sym->type->info.function.explicit_condition =
              syntax->pending_explicit_condition;
          syntax->pending_explicit_condition = NULL;
        }
      }
      if (TryParseCXXDeductionGuide(syntax, sym)) {
        TypeParserReset(parser);
        continue;
      }
      if (is_explicit) {
        SyntaxError(syntax,
                    "explicit is only supported on deduction guides");
      }
      old_sym = parser->cxx_member_definition != NULL
          ? parser->cxx_member_definition->symbol
          : FindFileScopeSymbol(syntax, &sym->name);
      if (parser->cxx_member_definition != NULL &&
          syntax->parsing_template_specialization) {
        MarkMemberFunctionTemplateSpecialization(
            syntax, sym, parser->cxx_member_definition);
        StructMember* matching_specialization =
            FindMemberFunctionTemplateSpecialization(
                parser->cxx_member_definition, sym->type);
        if (matching_specialization != NULL) {
          parser->cxx_member_definition = matching_specialization;
          old_sym = matching_specialization->symbol;
        } else if (sym->type->info.function.template_origin != NULL) {
          StructMember* specialization = NewStructMember(sym);
          specialization->is_member_function = true;
          specialization->is_static = parser->cxx_member_definition->is_static;
          specialization->access = parser->cxx_member_definition->access;
          AppendMemberFunctionSpecialization(
              syntax, parser->cxx_member_definition, specialization);
          parser->cxx_member_definition = specialization;
          old_sym = NULL;
          overload_was_appended = true;
        }
      } else {
        MarkFunctionTemplateSpecialization(syntax, sym, old_sym);
      }
      if (parser->cxx_member_definition == NULL &&
          CanOverloadFunctions(old_sym, sym)) {
        if (TryAppendSameSignatureConstrainedTemplateOverload(
                old_sym, sym, syntax->current_template_requires_clause)) {
          old_sym = NULL;
          overload_was_appended = true;
        } else {
          Symbol* matching_overload = FindMatchingOverload(old_sym, sym->type);
          if (matching_overload != NULL) {
            old_sym = matching_overload;
          } else {
          AppendOverload(old_sym, sym);
          old_sym = NULL;
          overload_was_appended = true;
          }
        }
      }
      if (parser->cxx_member_definition != NULL &&
          !syntax->parsing_template_specialization) {
        StructMember* matching_member = FindStructMemberOverload(
            parser->cxx_member_definition, sym->type);
        if (matching_member != NULL) {
          parser->cxx_member_definition = matching_member;
          old_sym = matching_member->symbol;
        }
      }
      // Capture whether the in-class declaration already defined this static
      // data member (only inline / constexpr members are defined in-class).
      // This must be read before the block below can set `is_defined`, so that
      // a genuine out-of-class definition of a non-inline member is not skipped.
      if (CompilerIsCXX() && parser->cxx_member_definition != NULL &&
          old_sym != NULL && !TypeIsFunction(old_sym->type) &&
          old_sym->flags.is_defined) {
        redundant_static_member_redefinition = true;
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
              String symbol_name;
              StringInit(&symbol_name, NULL);
              SymbolFunctionDiagnosticName(sym, &symbol_name);
              SyntaxError(syntax, "Duplicate definition of symbol %s",
                          symbol_name.value);
              StringDestruct(&symbol_name);
            }
          } else {
            if (IsDefinition(parser, old_sym, storage)) {
              // This is a declaration of a previously known definition.
              String symbol_name;
              StringInit(&symbol_name, NULL);
              SymbolFunctionDiagnosticName(sym, &symbol_name);
              SyntaxError(syntax, "Duplicate definition of symbol %s",
                          symbol_name.value);
              StringDestruct(&symbol_name);
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
        if (!RedeclarationTypesEqual(sym->type, old_sym->type)) {
          String suffix;
          StringInit(&suffix, NULL);
          SymbolFunctionDiagnosticSuffix(sym, &suffix);
          SyntaxError(syntax, "Symbol %s redeclared with different type%s",
                      sym->name.value, suffix.value);
          StringDestruct(&suffix);
          TypeErrorDetails(syntax->lex->current_token_location,
                           sym->type, old_sym->type);
          const char* filename;
          int lineno, start, end;
          DecodeSourceLocation(old_sym->location, &filename, &lineno, &start, &end);
          ReportNote(filename, lineno, "Previously declared here");
        } else {
          MergeCXXDefaultArguments(syntax, old_sym, sym);
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
        if (TypeIsFunction(sym->type) && sym->type->info.function.is_constexpr) {
          old_sym->type->info.function.is_constexpr = true;
        }
        if (TypeIsFunction(sym->type) && sym->type->info.function.is_consteval) {
          old_sym->type->info.function.is_consteval = true;
          old_sym->type->info.function.is_constexpr = true;
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

    // Parse common __attribute__ / C++ attribute syntax.
    while (true) {
      if (SyntaxParseCXXAlignas(syntax, attributes)) {
        continue;
      }
      if (LexMatch(syntax->lex, TOK(attribute))) {
        SyntaxParseAttribute(syntax, attributes);
      } else if (SyntaxLookingAtCXXAttribute(syntax)) {
        SyntaxParseCXXAttributes(syntax, attributes);
      } else {
        break;
      }
    }
    
    VectorCopy(&sym->attributes, attributes);
    VectorClear(attributes);
    SyntaxApplyDeclarationAttributes(sym);
    if (parser->placeholder_variable_constraint != NULL) {
      sym->associated_constraint = parser->placeholder_variable_constraint;
      parser->placeholder_variable_constraint = NULL;
    } else if (syntax->pending_placeholder_variable_constraint != NULL) {
      sym->associated_constraint = syntax->pending_placeholder_variable_constraint;
      syntax->pending_placeholder_variable_constraint = NULL;
    }

    SyntaxCheckThreadLocal(syntax, sym, kParsingFileScope,
                     parser->cxx_member_definition != NULL &&
                         parser->cxx_member_definition->is_static,
                     parser->cxx_member_definition != NULL &&
                         !parser->cxx_member_definition->is_static &&
                         !TypeIsFunction(sym->type));

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
      ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type);
      if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
        if (sym->type->info.function.is_defaulted) {
          old_sym->type->info.function.is_defaulted = true;
        }
        if (sym->type->info.function.is_deleted) {
          old_sym->type->info.function.is_deleted = true;
        }
        if (sym->flags.is_weak) {
          old_sym->flags.is_weak = true;
        }
      }
    }
    if (TypeIsFunction(sym->type)) {
      if (old_sym != NULL && sym->flags.is_c_linkage) {
        old_sym->flags.is_c_linkage = true;
      }
      SymbolSetCXXMangledAsmName(sym);
      if (old_sym != NULL && old_sym->asm_name.length == 0) {
        SymbolSetCXXMangledAsmName(old_sym);
      }
    }
  
    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      if (TypeContainsAuto(sym->type) &&
          !TypeFunctionReturnContainsAuto(sym->type)) {
        SyntaxError(syntax, "auto function parameter type is not supported yet");
      }
      if (TypeFunctionReturnContainsAuto(sym->type) &&
          !LexLookingAt(syntax->lex, TOK(lbrace))) {
        SyntaxError(syntax,
                    "auto function return type requires a function body");
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
    } else if (parser->is_inline && !CompilerIsCXX()) {
      SyntaxError(syntax, "inline can only be applied to functions");
    }
    if (!TypeIsClassTemplatePlaceholder(sym->type) &&
        TypeIsAbstractClass(sym->type)) {
      SyntaxError(syntax, "Cannot declare object of abstract class %s",
                  sym->type->info.struct_info->tag_name != NULL
                      ? sym->type->info.struct_info->tag_name->value
                      : "<anonymous>");
    }
    if (!syntax->parsing_template_declaration &&
        TypeContainsClassTemplate(sym->type) &&
        !TypeIsClassTemplatePlaceholder(sym->type)) {
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
    bool skip_cxx_function_redeclaration =
        CompilerIsCXX() && old_sym != NULL && TypeIsFunction(sym->type) &&
        sym->flags.is_defined && sym->type->info.function.definition;
    if (!TypeIsFunction(sym->type)) {
      sym->flags.is_constexpr = parser->is_constexpr;
      sym->flags.is_constinit = parser->is_constinit;
      if (CompilerIsCXX() && parser->is_inline) {
        sym->flags.is_defined = true;
        if (!StorageIs(sym->storage, STO(static))) {
          sym->flags.is_weak = true;
          SymbolSetCXXDataAsmName(sym, NULL);
        }
      }
      if (sym->flags.is_constexpr) {
        sym->type->qualifiers |= kQualConst;
      }
    }

    // Any initializer?
    ASTNode* initializer = NULL;
    if (LexMatch(syntax->lex, TOK(equal))) {
      syntax->init_storage = storage;
      initializer = SyntaxParseInitializer(syntax, sym, storage);
      ResolveCXXClassTemplateArgumentDeductionFromInitializer(
          syntax, sym, initializer);
    } else {
      initializer = ParseCXXDirectInitializer(syntax, sym, false);
      if (initializer != NULL) {
        if (!StorageIs(storage, STO(extern))) {
          sym->flags.is_defined = true;
        }
      } else if (CompilerIsCXX() && LexMatch(syntax->lex, TOK(lbrace))) {
        initializer = ParseBracedInitializer(syntax);
        if (!StorageIs(storage, STO(extern))) {
          sym->flags.is_defined = true;
        }
      }
    }
    if (TypeContainsAuto(sym->type) && initializer == NULL) {
      SyntaxError(syntax, "auto variable requires an initializer");
    } else if (TypeContainsAuto(sym->type)) {
      initializer = AnalyzeExpression(initializer);
      SemanticDeduceAutoType(sym, initializer, (ASTNode*)initializer);
      if (sym->associated_constraint != NULL && sym->type != NULL) {
        Vector* constraint_args = NewVector();
        TemplateArgument* type_arg = malloc(sizeof(TemplateArgument));
        memset(type_arg, 0, sizeof(*type_arg));
        type_arg->kind = kTemplateParameterType;
        type_arg->type = TypeRecordCopy(sym->type);
        VectorAppend(constraint_args, type_arg);
        if (!ConceptsConstraintSatisfied(sym->associated_constraint,
                                         constraint_args)) {
          SyntaxError(syntax, "constraints not satisfied");
          ConceptsReportAssociatedConstraintFailure(sym->associated_constraint,
                                                    constraint_args,
                                                    sym->location, NULL);
        }
        VectorDeleteWithContents(
            constraint_args, (VectorElementDestructor)TemplateArgumentDelete,
            /*free_element=*/false);
      }
    }
    if ((sym->flags.is_constexpr || sym->flags.is_constinit) &&
        initializer == NULL && parser->cxx_member_definition == NULL) {
      // An out-of-class definition of a static data member
      // (`constexpr T C::x;`) needs no initializer: the required initializer is
      // supplied by the in-class `static constexpr` declaration.
      SyntaxError(syntax, sym->flags.is_constinit
                              ? "constinit variable requires an initializer"
                              : "constexpr variable requires an initializer");
    }
    if (!syntax->parsing_template_declaration &&
        TypeIsClassTemplatePlaceholder(sym->type)) {
      SyntaxError(syntax, "Class template argument deduction requires an initializer");
    }
    // A `T C::m;` out-of-class redeclaration of an already-in-class-defined
    // inline/constexpr static data member (no initializer of its own) is not a
    // definition: emitting it would duplicate the symbol produced by the
    // in-class initializer.  Drop it from code generation.
    bool skip_redundant_static_member_definition =
        redundant_static_member_redefinition && initializer == NULL;
    if (!skip_cxx_function_redeclaration &&
        !skip_redundant_static_member_definition) {
      ASTNode* decl = NewVariableDeclarationASTNode(
          sym, initializer, syntax->lex->current_token_location);
      VectorAppend(declarations, decl);
      // A variable template's initializer is value-dependent on its template
      // parameters; it is analyzed and folded per use (see
      // MarkTemplateDeclaration / TypeInstantiateVariableTemplateConstant),
      // never eagerly at the point of definition.
      if (!syntax->parsing_template_declaration &&
          (sym->flags.is_constexpr || sym->flags.is_constinit ||
           (TypeIsConst(sym->type) && !TypeIsStructOrUnion(sym->type)))) {
        SemanticAnalyzeVariableDefinition(syntax,
                                          (VariableDeclarationASTNode*)decl);
      }
    }

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
  Symbol* existing = is_tag ? SyntaxFindTag(syntax, &alias->name)
                            : SyntaxFindSymbol(syntax, &alias->name);
  if (existing != NULL) {
    Symbol* existing_target = FollowAlias(existing);
    Symbol* new_target = alias->alias_target != NULL ? FollowAlias(alias->alias_target)
                                                     : NULL;
    if (existing_target != NULL && existing_target == new_target) {
      SymbolDelete(alias);
      return true;
    }
    SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                alias->name.value);
    SymbolDelete(alias);
    return false;
  }
  bool added = is_tag ? SyntaxAddTag(syntax, alias) : SyntaxAddSymbol(syntax, alias);
  if (!added) {
    SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                alias->name.value);
    SymbolDelete(alias);
    return false;
  }
  return true;
}

static void ImportNamespace(Syntax* syntax, Namespace* ns);

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

static void ImportInlineNamespaceChild(Namespace* child, void* ctx) {
  ImportNamespace((Syntax*)ctx, child);
}

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
  return NamespaceInsertAlias(scope, name, target);
}

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

static void ImportNamespace(Syntax* syntax, Namespace* ns) {
  BinaryTreeTraverse(&ns->symbol_table, ImportNamespaceSymbol, syntax);
  BinaryTreeTraverse(&ns->tag_table, ImportNamespaceTag, syntax);
  ImportNamespaceNames(syntax, ns);
  if (ns->anonymous_child != NULL) {
    ImportNamespace(syntax, ns->anonymous_child);
  }
  NamespaceForEachInlineChild(ns, ImportInlineNamespaceChild, syntax);
}

static ASTNode* EmptyDeclarationList(SourceLocation location) {
  return NewDeclarationListASTNode(NewVector(), location);
}

static bool LookingAtNamespaceAliasDefinition(Syntax* syntax) {
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

static ASTNode* ParseNamespaceAliasDefinition(Syntax* syntax,
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
    String symbol_name;
    StringInit(&symbol_name, NULL);
    SymbolFunctionDiagnosticName(sym, &symbol_name);
    SyntaxError(syntax, "Duplicate definition of symbol %s",
                symbol_name.value);
    StringDestruct(&symbol_name);
  } else if (!RedeclarationTypesEqual(sym->type, old_sym->type)) {
    String suffix;
    StringInit(&suffix, NULL);
    SymbolFunctionDiagnosticSuffix(sym, &suffix);
    SyntaxError(syntax, "Symbol %s redeclared with different type%s",
                sym->name.value, suffix.value);
    StringDestruct(&suffix);
    TypeErrorDetails(syntax->lex->current_token_location,
                     sym->type, old_sym->type);
  } else if (LexLookingAt(syntax->lex, TOK(lbrace))) {
    MergeCXXDefaultArguments(syntax, old_sym, sym);
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
    SyntaxError(syntax, "Duplicate symbol from using declaration: %s",
                alias->name.value);
    SymbolDelete(alias);
  }
  if (added && !block_scope_alias && syntax->parsing_template_declaration) {
    MoveTemplateParameterConstraints(syntax->current_template_parameters,
                                     &alias->associated_constraint);
    if (syntax->current_template_requires_clause != NULL) {
      AddOwnedAssociatedConstraint(&alias->associated_constraint,
                                   syntax->current_template_requires_clause);
      syntax->current_template_requires_clause = NULL;
    }
    if (alias->alias_template == NULL) {
      alias->alias_template = malloc(sizeof(AliasTemplate));
      VectorInit(&alias->alias_template->parameters);
    }
    if (syntax->current_template_parameters != NULL) {
      for (size_t p = 0; p < syntax->current_template_parameters->length; p++) {
        VectorAppend(&alias->alias_template->parameters,
                     syntax->current_template_parameters->value.p[p]);
      }
      syntax->current_template_parameters->length = 0;
    }
  }
  if (parsed != NULL) {
    SymbolDelete(parsed);
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
                    /*is_tag=*/false);
    }
  }

  FullyQualifiedIdentifierDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

static ASTNode* ParseUsingDeclaration(Syntax* syntax) {
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

static ASTNode* ParseNamespaceDeclaration(Syntax* syntax, bool leading_inline) {
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
    AppendDeclarationsFromNode(declarations, node);
  }
  syntax->current_namespace = previous_namespace;

  SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
  return NewDeclarationListASTNode(declarations, location);
}

static TemplateParameter* NewTemplateParameter(const char* name,
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
  if (type != NULL) {
    TypeRecordIncRef(type);
  }
  if (default_type != NULL) {
    TypeRecordIncRef(default_type);
  }
  param->index = index;
  return param;
}

static bool ParseTemplateNonTypeDefault(Syntax* syntax,
                                        long long* default_value,
                                        int* default_parameter_index) {
  bool old_parsing_template_argument = syntax->parsing_template_argument;
  syntax->parsing_template_argument = true;
  ASTNode* expr = SyntaxParseSingleExpression(syntax,
                                              TC(closebra) | TC(exprsep));
  syntax->parsing_template_argument = old_parsing_template_argument;
  expr = AnalyzeExpression(expr);
  bool ok = EvaluateIntegerExpression(expr, default_value);
  if (!ok && expr->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)expr;
    if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
        !id->symbol->flags.is_template_type_parameter) {
      *default_parameter_index = id->symbol->template_parameter_index;
      ok = true;
    }
  }
  if (!ok) {
    SyntaxError(syntax,
                "Template non-type default must be an integer constant expression");
  }
  ASTNodeDelete(expr);
  return ok;
}

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
    TypeRecordDelete(type);
    return result;
  }
  return type;
}

static TemplateArgument* NewTemplateParameterTypeArgument(int index,
                                                         TypeRecord* type) {
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
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

static TemplateArgument* NewConstrainedPlaceholderTypeArgument(
    int index, TypeRecord* placeholder) {
  TemplateArgument* constrained_arg =
      NewTemplateParameterTypeArgument(index, placeholder);
  if (constrained_arg->type != NULL) {
    constrained_arg->type->template_parameter_index = index;
  }
  return constrained_arg;
}

static bool ParseConstrainedTemplateTypeParameter(Syntax* syntax,
                                                 Vector* params, int base) {
  Lex* lex = syntax->lex;
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !LexLookingAt(lex, TOK(identifier))) {
    return false;
  }

  LexCheckpoint start_checkpoint;
  LexCheckpointSave(lex, &start_checkpoint);
  SourceLocation constraint_location = lex->current_token_location;

  FullyQualifiedIdentifier concept_id;
  FullyQualifiedIdentifierInit(&concept_id);
  if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &concept_id,
                                                          TC(closebra))) {
    FullyQualifiedIdentifierDestruct(&concept_id);
    LexCheckpointRestore(lex, &start_checkpoint);
    LexCheckpointDestruct(&start_checkpoint);
    return false;
  }

  Symbol* concept_symbol = SyntaxFindQualifiedSymbol(syntax, &concept_id);
  if (concept_symbol == NULL || !concept_symbol->flags.is_concept) {
    FullyQualifiedIdentifierDestruct(&concept_id);
    LexCheckpointRestore(lex, &start_checkpoint);
    LexCheckpointDestruct(&start_checkpoint);
    return false;
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
  if (concept_arguments == NULL) {
    concept_arguments = NewVector();
  }

  int index = base + (int)params->length;
  bool is_parameter_pack = LexMatch(lex, TOK(ellipsis));
  if (LexLookingAt(lex, TOK(auto))) {
    LexNextToken(lex);
    if (!LexLookingAt(lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected constrained template parameter name");
      VectorDeleteWithContents(concept_arguments,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
      SyntaxRecover(syntax, TC(closebra));
      return true;
    }

    TypeRecord* placeholder_type =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    placeholder_type->template_parameter_index = index;
    placeholder_type->template_parameter_name = NewString("auto");
    Symbol* param =
        NewSymbol(lex->spelling.value, TypeRecordCopy(placeholder_type),
                  STO(typedef));
    param->flags.invented = true;
    param->flags.is_template_parameter = true;
    param->flags.is_template_type_parameter = false;
    param->flags.is_parameter_pack = is_parameter_pack;
    param->template_parameter_index = index;
    param->location = lex->current_token_location;
    bool added = SyntaxAddSymbol(syntax, param);
    if (!added) {
      SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
      SymbolDelete(param);
    }

    String param_name;
    StringInit(&param_name, lex->spelling.value);
    LexNextToken(lex);
    TemplateArgument* constrained_arg =
        NewConstrainedPlaceholderTypeArgument(index, placeholder_type);
    if (concept_arguments->length == 0) {
      VectorAppend(concept_arguments, constrained_arg);
    } else {
      VectorInsertBefore(concept_arguments, 0, constrained_arg);
    }
    bool has_default_int = false;
    long long default_int_value = 0;
    int default_template_parameter_index = -1;
    if (LexMatch(lex, TOK(equal))) {
      if (is_parameter_pack) {
        SyntaxError(syntax, "Template parameter pack cannot have a default");
      }
      has_default_int =
          ParseTemplateNonTypeDefault(syntax, &default_int_value,
                                      &default_template_parameter_index);
    }
    TemplateParameter* template_param =
        NewTemplateParameter(param_name.value, kTemplateParameterNonType,
                             is_parameter_pack, placeholder_type, NULL,
                             has_default_int, default_int_value,
                             default_template_parameter_index, index);
    template_param->associated_constraint =
        NewConceptIdConstraint(concept_symbol, concept_arguments,
                               constraint_location);
    VectorAppend(params, template_param);
    StringDestruct(&param_name);
    return true;
  }

  if (!LexLookingAt(lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected constrained template parameter name");
    VectorDeleteWithContents(concept_arguments,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
    SyntaxRecover(syntax, TC(closebra));
    return true;
  }

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
    SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
    SymbolDelete(param);
  }

  String param_name;
  StringInit(&param_name, lex->spelling.value);
  LexNextToken(lex);
  TemplateArgument* constrained_arg =
      NewConstrainedPlaceholderTypeArgument(index, placeholder);
  if (concept_arguments->length == 0) {
    VectorAppend(concept_arguments, constrained_arg);
  } else {
    VectorInsertBefore(concept_arguments, 0, constrained_arg);
  }
  TypeRecord* default_type = NULL;
  if (LexMatch(lex, TOK(equal))) {
    if (is_parameter_pack) {
      SyntaxError(syntax, "Template parameter pack cannot have a default");
    }
    default_type = ParseTemplateTypeDefault(syntax);
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
  return true;
}

// Parses a template template parameter (`template <parameter-list> class C`).
// The construct is consumed cleanly and `C` is introduced as a type-parameter
// placeholder so later references resolve, but full support -- using `C<...>`
// as a template-id and binding a class-template argument -- is not implemented
// yet, so a clear diagnostic is reported instead of the parser crashing.
static bool ParseTemplateTemplateParameter(Syntax* syntax, Vector* params,
                                           int base) {
  Lex* lex = syntax->lex;
  int index = base + (int)params->length;
  SourceLocation location = lex->current_token_location;
  LexNextToken(lex);  // template

  // Skip the inner template-parameter-list by balancing angle brackets.  A full
  // parse is avoided because it is discarded anyway and because the inner list
  // legitimately contains unnamed parameters (`template <class> class C`) that
  // the ordinary parameter parser would reject with a misleading diagnostic.
  if (LexLookingAt(lex, TOK(less))) {
    int depth = 0;
    while (!LexEof(lex)) {
      Token t = lex->current_token;
      if (t == TOK(less)) {
        depth++;
      } else if (t == TOK(lessless)) {
        depth += 2;
      } else if (t == TOK(greater)) {
        depth--;
      } else if (t == TOK(greatergreater) || t == TOK(greatergreatereq)) {
        depth -= 2;
      } else if (t == TOK(greatereq)) {
        depth -= 1;
      }
      LexNextToken(lex);
      if (depth <= 0) {
        break;
      }
    }
  } else {
    SyntaxError(syntax, "Expected '<' in template template parameter");
  }

  if (!LexMatch(lex, TOK(class)) && !LexMatch(lex, TOK(typename))) {
    SyntaxError(syntax,
                "Expected 'class' or 'typename' in template template parameter");
    SyntaxRecover(syntax, TC(closebra));
    return false;
  }
  bool is_parameter_pack = LexMatch(lex, TOK(ellipsis));

  String param_name;
  StringInit(&param_name, "");
  if (LexLookingAt(lex, TOK(identifier))) {
    StringSet(&param_name, lex->spelling.value);
    LexNextToken(lex);
  }

  SyntaxError(syntax, "template template parameters are not yet supported");

  // Register the name as a type-parameter placeholder so downstream references
  // resolve and index bookkeeping stays consistent.  The TU already carries an
  // error, so no code is generated regardless.
  if (param_name.length > 0) {
    TypeRecord* placeholder =
        NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    placeholder->template_parameter_index = index;
    placeholder->template_parameter_name = NewString(param_name.value);
    Symbol* param = NewSymbol(param_name.value, placeholder, STO(typedef));
    param->flags.invented = true;
    param->flags.is_template_parameter = true;
    param->flags.is_template_type_parameter = true;
    param->flags.is_parameter_pack = is_parameter_pack;
    param->template_parameter_index = index;
    param->location = location;
    if (!SyntaxAddSymbol(syntax, param)) {
      SymbolDelete(param);
    }
  }

  // Consume an optional default template argument in this error-recovery path.
  if (LexMatch(lex, TOK(equal))) {
    while (!LexEof(lex) && !LexLookingAt(lex, TOK(comma)) &&
           !LexLookingAt(lex, TOK(greater)) &&
           !LexLookingAt(lex, TOK(greatergreater)) &&
           !LexLookingAt(lex, TOK(greatergreatereq))) {
      LexNextToken(lex);
    }
  }

  VectorAppend(params,
               NewTemplateParameter(param_name.value, kTemplateParameterType,
                                    is_parameter_pack, NULL, NULL, false, 0, -1,
                                    index));
  StringDestruct(&param_name);
  return true;
}

static bool ParseTemplateParameter(Syntax* syntax, Vector* params, int base) {
  Lex* lex = syntax->lex;
  int index = base + (int)params->length;
  if (LexLookingAt(lex, TOK(template))) {
    return ParseTemplateTemplateParameter(syntax, params, base);
  }
  if (ParseConstrainedTemplateTypeParameter(syntax, params, base)) {
    return true;
  }
  if (LexMatch(lex, TOK(typename)) || LexMatch(lex, TOK(class))) {
    bool is_parameter_pack = LexMatch(lex, TOK(ellipsis));
    if (!LexLookingAt(lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected template parameter name");
      SyntaxRecover(syntax, TC(closebra));
      return false;
    }

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
      SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
      SymbolDelete(param);
    }
    String param_name;
    StringInit(&param_name, lex->spelling.value);
    LexNextToken(lex);
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
    TypeRecordDelete(default_type);
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
  bool is_parameter_pack = param->flags.is_parameter_pack;
  bool added = SyntaxAddSymbol(syntax, param);
  if (!added) {
    SyntaxError(syntax, "Duplicate template parameter %s", param->name.value);
    SymbolDelete(param);
  }
  bool has_default_int = false;
  long long default_int_value = 0;
  int default_template_parameter_index = -1;
  if (LexMatch(lex, TOK(equal))) {
    if (is_parameter_pack) {
      SyntaxError(syntax, "Template parameter pack cannot have a default");
    }
    has_default_int =
        ParseTemplateNonTypeDefault(syntax, &default_int_value,
                                    &default_template_parameter_index);
  }
  VectorAppend(params, NewTemplateParameter(param->name.value,
                                            kTemplateParameterNonType,
                                            is_parameter_pack,
                                            param->type, NULL,
                                            has_default_int,
                                            default_int_value,
                                            default_template_parameter_index,
                                            index));
  return true;
}

Vector* SyntaxParseTemplateParameterListWithBase(Syntax* syntax, int base) {
  Lex* lex = syntax->lex;
  if (!LexMatch(lex, TOK(less))) {
    SyntaxError(syntax, "Expected '<' after template");
    return NewVector();
  }
  Vector* params = NewVector();
  while (!LexEof(lex) && !LexLookingAt(lex, TOK(greater))) {
    ParseTemplateParameter(syntax, params, base);
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedTemplateClose(syntax, TC(decl));
  return params;
}

Vector* SyntaxParseTemplateParameterList(Syntax* syntax) {
  return SyntaxParseTemplateParameterListWithBase(syntax, 0);
}

static void DependentTemplateExpressionVisitor(ASTNode* node, void* data,
                                               int child_id,
                                               VisitorMode mode) {
  (void)child_id;
  (void)mode;
  if (node == NULL) {
    return;
  }
  if ((node->flags & kASTDependentQualifiedName) != 0 ||
      TypeContainsTemplateParameter(node->type)) {
    *(bool*)data = true;
    return;
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
  }
}

// True if `node` reads a value through a dependent class-template scope
// (`Trait<T>::value`), making the whole expression value-dependent: it must be
// kept unevaluated and folded per-instantiation, not constant-folded now.
static bool ExpressionContainsDependentTemplateParameter(ASTNode* node) {
  bool found = false;
  ASTNodeVisit(node, DependentTemplateExpressionVisitor, 0, &found);
  return found;
}

static bool ExpressionIsNonTypeTemplateParameter(ASTNode* node,
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
static bool SyntaxIdentifierStartsDaveCCTypeTraitBuiltin(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  const char* name = syntax->lex->spelling.value;
  return strncmp(name, "__davecc_is_", 12) == 0 ||
         strcmp(name, "__davecc_invoke_result_t") == 0 ||
         strcmp(name, "__davecc_common_type_t") == 0;
}

static bool SyntaxTemplateArgumentLooksLikeType(Syntax* syntax) {
  if (!SyntaxLookingAtType(syntax)) {
    return false;
  }
  if (SyntaxIdentifierStartsDaveCCTypeTraitBuiltin(syntax)) {
    return false;
  }
  Token tok = syntax->lex->current_token;
  if (tok != TOK(identifier) && tok != TOK(coloncolon)) {
    return true;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                     TC(closebra) | TC(exprsep));
  bool is_qualified = name.is_qualified;
  bool resolved = false;
  bool resolved_type = false;
  if (is_qualified) {
    if (SyntaxFindQualifiedTag(syntax, &name) != NULL) {
      resolved = true;
      resolved_type = true;
    } else {
      Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, &name);
      if (symbol != NULL) {
        resolved = true;
        resolved_type = StorageIs(symbol->storage, STO(typedef));
      }
    }
  }
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  if (is_qualified) {
    // A resolvable qualified name is a type only when it names a tag/typedef; an
    // unresolvable (dependent) qualified name without `typename` is a non-type.
    return resolved && resolved_type;
  }
  return true;
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
    arg->is_pack_expansion = false;
    arg->type = NULL;
    arg->int_value = 0;
    arg->template_parameter_index = -1;
    arg->pack_arguments = NULL;
    arg->dependent_expr = NULL;
    // Record where this argument is written so a diagnostic raised while it is
    // substituted (possibly in a far-removed instantiation) points back here.
    arg->location = lex->current_token_location;
    if (SyntaxTemplateArgumentLooksLikeType(syntax)) {
      TypeParser parser;
      TypeParserInit(&parser, lex, syntax, STO(implicit), syntax->context);
      TypeRecord* type = TypeParserParseType(&parser, true);
      Symbol* sym = TypeParserParseDeclarator(&parser, type);
      TypeParserDestruct(&parser);
      if (sym != NULL) {
        arg->type = TypeRecordCopy(sym->type);
        arg->is_pack_expansion = sym->flags.is_parameter_pack;
        if (arg->is_pack_expansion && type != NULL) {
          arg->type->qualifiers |= type->qualifiers;
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
      }
    } else {
      bool old_parsing_template_argument = syntax->parsing_template_argument;
      syntax->parsing_template_argument = true;
      ASTNode* expr = SyntaxParseSingleExpression(syntax,
                                                  TC(closebra) | TC(exprsep));
      syntax->parsing_template_argument = old_parsing_template_argument;
      arg->kind = kTemplateParameterNonType;
      int direct_template_parameter_index = -1;
      if (!ExpressionIsNonTypeTemplateParameter(
              expr, &direct_template_parameter_index) &&
          ExpressionContainsDependentTemplateParameter(expr)) {
        // A value-dependent trait condition such as `!is_integral<It>::value`:
        // resolving it now would fold the primary template's value.  Keep the
        // expression and re-fold it once the parameters become concrete.
        arg->dependent_expr = expr;
        arg->is_pack_expansion = LexMatch(lex, TOK(ellipsis));
      } else {
        if (direct_template_parameter_index >= 0) {
          arg->template_parameter_index = direct_template_parameter_index;
        }
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
            if (syntax->parsing_template_declaration) {
              arg->dependent_expr = expr;
              arg->is_pack_expansion = LexMatch(lex, TOK(ellipsis));
              VectorAppend(args, arg);
              if (!LexMatch(lex, TOK(comma))) {
                break;
              }
              continue;
            } else {
              SyntaxError(syntax,
                          "Template non-type argument must be an integer constant expression");
            }
          }
        }
        arg->int_value = value;
        if (expr->type != NULL) {
          arg->type = TypeRecordCopy(expr->type);
        }
        ASTNodeDelete(expr);
        arg->is_pack_expansion = LexMatch(lex, TOK(ellipsis));
      }
    }
    VectorAppend(args, arg);
    if (!LexMatch(lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedTemplateClose(syntax, followers);
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

static int CurrentTemplateParameterListLength(Syntax* syntax) {
  return syntax->current_template_parameters != NULL
             ? (int)syntax->current_template_parameters->length
             : syntax->current_template_parameter_count;
}

static int CurrentTemplateParameterBase(Syntax* syntax) {
  return syntax->current_template_parameter_count -
         CurrentTemplateParameterListLength(syntax);
}

static void MoveCurrentTemplateParametersToFunction(Syntax* syntax,
                                                    TypeRecord* func) {
  if (syntax->current_template_parameters == NULL || func == NULL ||
      !TypeIsFunction(func)) {
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

static void MoveCurrentTemplateParametersToStruct(Syntax* syntax, Struct* str) {
  if (syntax->current_template_parameters == NULL || str == NULL) {
    return;
  }
  VectorDestructWithContents(&str->template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&str->template_parameters);
  for (size_t i = 0; i < syntax->current_template_parameters->length; i++) {
    VectorAppend(&str->template_parameters,
                 syntax->current_template_parameters->value.p[i]);
  }
  syntax->current_template_parameters->length = 0;
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

static void AddOwnedAssociatedConstraint(ConstraintExpr** target,
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

static void MoveTemplateParameterConstraints(Vector* parameters,
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

static void MoveTemplateParameterConstraintsToFunction(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func)) {
    return;
  }
  MoveTemplateParameterConstraints(&func->info.function.template_parameters,
                                   &func->info.function.associated_constraint);
}

static void MoveTemplateParameterConstraintsToStruct(Struct* str) {
  if (str == NULL) {
    return;
  }
  MoveTemplateParameterConstraints(&str->template_parameters,
                                   &str->associated_constraint);
}

static void MoveTemplateParameterConstraintsToVariableTemplate(
    VariableTemplate* vt) {
  if (vt == NULL) {
    return;
  }
  MoveTemplateParameterConstraints(&vt->parameters, &vt->associated_constraint);
}

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
    TypeEnsureCXXDeductionGuides(syntax->last_parsed_tag);
  }
}

static ASTNode* ParseExplicitTemplateInstantiation(Syntax* syntax,
                                                   SourceLocation location) {
  if (!(LexMatch(syntax->lex, TOK(struct)) ||
        LexMatch(syntax->lex, TOK(class)) ||
        LexMatch(syntax->lex, TOK(union)))) {
    SyntaxError(syntax, "Expected class template name after template");
    SyntaxRecover(syntax, TC(semicolon));
    SyntaxNeedSemicolon(syntax, TC(decl));
    return EmptyDeclarationList(location);
  }

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
    TypeRecordDelete(type);
    VectorDestructWithContents(args,
                               (VectorElementDestructor)TemplateArgumentDelete,
                               /*free_element=*/false);
  }
  FullyQualifiedIdentifierDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

static ASTNode* ParseTemplateDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // template

  if (!LexLookingAt(syntax->lex, TOK(less))) {
    return ParseExplicitTemplateInstantiation(syntax, location);
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


// If we are looking at a C++ linkage specification (`extern "C"` or
// `extern "C++"`, optionally with a brace-enclosed declaration sequence),
// parse it and return the resulting declaration(s).  Returns NULL (leaving the
// lexer untouched) when the current tokens are not a linkage specification, so
// the caller can treat a bare `extern` as an ordinary storage-class specifier.
static ASTNode* ParseCXXLinkageSpecification(Syntax* syntax) {
  if (!CompilerIsCXX() || !LexLookingAt(syntax->lex, TOK(extern))) {
    return NULL;
  }

  // Peek past `extern` to see whether a string-literal linkage name follows.
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // extern
  if (!LexLookingAt(syntax->lex, TOK(string))) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  String linkage;
  StringInit(&linkage, "");
  while (LexLookingAt(syntax->lex, TOK(string))) {
    StringAppend(&linkage, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  bool is_c = StringEqual(&linkage, "C");
  if (!is_c && !StringEqual(&linkage, "C++")) {
    SyntaxError(syntax, "Unknown linkage specification \"%s\"", linkage.value);
  }
  StringDestruct(&linkage);

  // C linkage suppresses name mangling for the enclosed declarations; C++
  // linkage is the default, so it only needs to parse the declarations.
  bool apply_c_linkage = is_c;

  if (LexMatch(syntax->lex, TOK(lbrace))) {
    Vector* declarations = NewVector();
    if (apply_c_linkage) {
      syntax->extern_c_depth++;
    }
    while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rbrace))) {
      ASTNode* node = SyntaxParseExternalDeclaration(syntax);
      AppendDeclarationsFromNode(declarations, node);
    }
    if (apply_c_linkage) {
      syntax->extern_c_depth--;
    }
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
    return NewDeclarationListASTNode(declarations, location);
  }

  // Single-declaration form: `extern "C" <declaration>`.
  if (apply_c_linkage) {
    syntax->extern_c_depth++;
  }
  ASTNode* node = SyntaxParseExternalDeclaration(syntax);
  if (apply_c_linkage) {
    syntax->extern_c_depth--;
  }
  return node;
}

// Parses an external declaration (a global variable, etc.) and adds it
// to the symbol table.
// ---------------------------------------------------------------------------
// C++20 modules (MVP: named interface units, export declarations, import).
//
// `module` and `import` are context-sensitive: they lex as ordinary
// identifiers (see cxx_reserved_words in lex.c) and only act as keywords when
// they begin a module directive at the start of an external declaration.
// `export` is a real keyword (TOK(export)).
// ---------------------------------------------------------------------------

// True if the current token is an identifier spelled `spelling`, in C++20 mode.
static bool SyntaxAtContextualKeyword(Syntax* syntax, const char* spelling) {
  return CompilerCXXAtLeast(kLanguageStandardCXX20) &&
         LexLookingAt(syntax->lex, TOK(identifier)) &&
         StringEqual(&syntax->lex->spelling, spelling);
}

// True if we're at a contextual `module`/`import` keyword that actually begins
// a module directive (followed by a name), as opposed to an identifier that
// merely happens to be spelled "module"/"import".
static bool SyntaxAtModuleDirective(Syntax* syntax, const char* spelling) {
  if (!SyntaxAtContextualKeyword(syntax, spelling)) {
    return false;
  }
  LexCheckpoint cp;
  LexCheckpointSave(syntax->lex, &cp);
  LexNextToken(syntax->lex);
  bool followed_by_name = LexLookingAt(syntax->lex, TOK(identifier));
  LexCheckpointRestore(syntax->lex, &cp);
  LexCheckpointDestruct(&cp);
  return followed_by_name;
}

// Parse a (possibly dotted) module name like `foo` or `foo.bar` into `out`
// (which this initializes).  Partitions (`foo:part`) are not yet supported.
static bool ParseModuleName(Syntax* syntax, String* out) {
  StringInit(out, "");
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Expected module name");
    return false;
  }
  StringAppend(out, syntax->lex->spelling.value);
  LexNextToken(syntax->lex);
  while (LexMatch(syntax->lex, TOK(dot))) {
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected identifier after '.' in module name");
      return false;
    }
    StringAppendChar(out, '.');
    StringAppend(out, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  return true;
}

// Parse `module NAME ;` (implementation unit) or, when `exported`, the
// `module NAME ;` tail of `export module NAME ;` (interface unit).  The
// contextual `module` keyword is the current token.
static ASTNode* ParseModuleDeclaration(Syntax* syntax, bool exported) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // consume contextual `module`
  String name;
  if (ParseModuleName(syntax, &name)) {
    if (compiler->module_name.length != 0) {
      SyntaxError(syntax,
                  "Multiple module declarations in one translation unit");
    } else {
      StringSetString(&compiler->module_name, &name);
      compiler->is_module_interface = exported;
    }
  }
  StringDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

// Parse `import NAME ;`, triggering the driver's module import hook.  The
// contextual `import` keyword is the current token.
static ASTNode* ParseImportDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // consume contextual `import`
  String name;
  if (ParseModuleName(syntax, &name)) {
    if (!CompilerImportModule(name.value)) {
      SyntaxError(syntax, "Cannot import module '%s'", name.value);
    }
  }
  StringDestruct(&name);
  SyntaxNeedSemicolon(syntax, TC(decl));
  return EmptyDeclarationList(location);
}

// Parse an `export` region: `export module ...`, `export import ...`,
// `export { declaration-seq }`, or `export declaration`.  The `export` keyword
// is the current token.
static ASTNode* ParseExportDeclaration(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // consume `export`

  if (SyntaxAtContextualKeyword(syntax, "module")) {
    return ParseModuleDeclaration(syntax, /*exported=*/true);
  }
  if (SyntaxAtContextualKeyword(syntax, "import")) {
    SyntaxError(syntax, "'export import' is not yet supported");
    return ParseImportDeclaration(syntax);
  }

  syntax->export_depth++;
  ASTNode* result;
  if (LexMatch(syntax->lex, TOK(lbrace))) {
    Vector* declarations = NewVector();
    while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rbrace))) {
      ASTNode* node = SyntaxParseExternalDeclaration(syntax);
      AppendDeclarationsFromNode(declarations, node);
    }
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
    result = NewDeclarationListASTNode(declarations, location);
  } else {
    result = SyntaxParseExternalDeclaration(syntax);
  }
  syntax->export_depth--;
  return result;
}

ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax) {
  syntax->context = kParsingFileScope;
  if (syntax->current_namespace == NULL) {
    syntax->current_namespace = compiler->global_namespace;
  }

  if (CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    if (LexLookingAt(syntax->lex, TOK(export))) {
      return ParseExportDeclaration(syntax);
    }
    if (SyntaxAtModuleDirective(syntax, "module")) {
      return ParseModuleDeclaration(syntax, /*exported=*/false);
    }
    if (SyntaxAtModuleDirective(syntax, "import")) {
      return ParseImportDeclaration(syntax);
    }
  }

  if (LexLookingAt(syntax->lex, TOK(static_assert))) {
    SourceLocation location = syntax->lex->current_token_location;
    ASTNode* node = SyntaxParseStaticAssert(syntax);
    ASTNodeDelete(node);
    return EmptyDeclarationList(location);
  }
  if (LexLookingAt(syntax->lex, TOK(namespace))) {
    return ParseNamespaceDeclaration(syntax, /*leading_inline=*/false);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(inline))) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(namespace))) {
      LexCheckpointDestruct(&checkpoint);
      return ParseNamespaceDeclaration(syntax, /*leading_inline=*/true);
    }
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
  }
  if (LexLookingAt(syntax->lex, TOK(template))) {
    return ParseTemplateDeclaration(syntax);
  }
  if (LexLookingAt(syntax->lex, TOK(using))) {
    return ParseUsingDeclaration(syntax);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(extern))) {
    ASTNode* linkage = ParseCXXLinkageSpecification(syntax);
    if (linkage != NULL) {
      return linkage;
    }
  }
  if (CurrentLineLooksLikeSpecialMemberDefinition(syntax)) {
    return ParseCXXSpecialMemberDefinition(syntax);
  }

  Vector* declarations = NewVector();
  Vector attributes = {0};
  
  // Parse common __attribute__ / C++ attribute syntax.
  while (LexLookingAt(syntax->lex, TOK(attribute)) ||
         SyntaxLookingAtCXXAttribute(syntax)) {
    if (LexMatch(syntax->lex, TOK(attribute))) {
      SyntaxParseAttribute(syntax, &attributes);
    } else {
      SyntaxParseCXXAttributes(syntax, &attributes);
    }
  }

  Storage storage = STO(implicit);
  bool is_inline = false;
  bool is_constexpr = false;
  bool is_consteval = false;
  bool is_constinit = false;
  bool is_explicit = false;
  TypeRecord* type = NULL;
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &is_constexpr,
                            &is_consteval, &is_constinit,
                            &is_explicit, &type, &attributes,
                            kParsingFileScope);

  if (StorageIs(storage, STO(auto)|STO(register))) {
    SyntaxError(syntax, "Illegal global storage specified: %s",
                StorageIs(storage, STO(register)) ? "register" : "auto");
    storage = STO(implicit);
  }

  // Parse common __attribute__ / C++ attribute syntax.
  while (LexLookingAt(syntax->lex, TOK(attribute)) ||
         SyntaxLookingAtCXXAttribute(syntax)) {
    if (LexMatch(syntax->lex, TOK(attribute))) {
      SyntaxParseAttribute(syntax, &attributes);
    } else {
      SyntaxParseCXXAttributes(syntax, &attributes);
    }
  }

  // Create a type parser for the declarators.
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingFileScope);
  parser.is_inline = is_inline;
  parser.is_constexpr = is_constexpr;
  parser.is_consteval = is_consteval;
  parser.is_constinit = is_constinit;
 
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
                                                 is_explicit,
                                                 &attributes,
                                                 declarations);
  SyntaxCloseScope(syntax);
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);
  if (result != NULL) {
    if (declarations->length != 1) {
      SyntaxError(syntax, "Cannot mix function definition with declaration");
    }
    VectorAppendVector(declarations, &syntax->inline_static_member_definitions);
    // Ownership of the queued inline static data member definitions now belongs
    // to `declarations`; clear the queue so later declarations don't re-emit
    // (and double-free) them.
    VectorClear(&syntax->inline_static_member_definitions);
    // result is a DeclarationListASTNode that owns `declarations`; its teardown
    // frees the vector and its contents, so don't free them here.
    AttributeListDestruct(&attributes);
    return result;
  }

  VectorAppendVector(declarations, &syntax->inline_static_member_definitions);
  VectorClear(&syntax->inline_static_member_definitions);

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

  CXXPrependCompleteObjectArgument(sym->type, actuals,
                                   /*complete_object=*/true, location);
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
  StructMember* ctor =
      FindStructMemberByName(type->info.struct_info, constructor_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return ctor;
}

static TypeRecord* CXXArrayBaseElementType(TypeRecord* type,
                                           unsigned long* count) {
  unsigned long elements = 1;
  while (TypeIsFixedArray(type)) {
    elements *= (unsigned long)type->info.array.size.fixed;
    type = type->next;
  }
  if (count != NULL) {
    *count = elements;
  }
  return type;
}

static ASTNode* NewCXXArrayElementExpression(Symbol* array,
                                             unsigned long flat_index,
                                             SourceLocation location) {
  ASTNode* expression = NewIdentifierASTNode(array, location);
  TypeRecord* array_type = array->type;
  while (TypeIsFixedArray(array_type)) {
    unsigned long lower_elements = 1;
    for (TypeRecord* lower = array_type->next; TypeIsFixedArray(lower);
         lower = lower->next) {
      lower_elements *= (unsigned long)lower->info.array.size.fixed;
    }
    unsigned long index = flat_index / lower_elements;
    flat_index %= lower_elements;
    expression = NewBinaryASTNode(
        AST_OP(subscript), TypeRecordCopy(array_type->next), location,
        expression,
        NewIntConstantASTNode(
            index, NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain),
            location));
    array_type = array_type->next;
  }
  return expression;
}

static bool CXXConstructorSetHasInitializerList(StructMember* ctor) {
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor) {
      continue;
    }
    for (size_t i = 0; i < info->prototype.length; i++) {
      Symbol* formal = info->prototype.value.p[i];
      TypeRecord* formal_type = formal->type;
      if (TypeIsReference(formal_type)) {
        formal_type = formal_type->next;
      }
      if (TypeIsCXXInitializerList(formal_type)) {
        return true;
      }
    }
  }
  return false;
}

static bool TypeHasCXXInitializerListConstructor(TypeRecord* type) {
  return CXXConstructorSetHasInitializerList(FindCXXConstructor(type));
}

static ASTNode* NewCXXCopyListConstructorInitializer(Syntax* syntax,
                                                    Symbol* sym,
                                                    ASTNode* initializer) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      initializer == NULL || initializer->op != AST_OP(braced_init) ||
      !TypeIsStructOrUnion(sym->type) || sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->is_aggregate) {
    return initializer;
  }
  StructMember* constructor = FindCXXConstructor(sym->type);
  if (constructor == NULL) {
    return initializer;
  }
  Vector* actuals = NewVector();
  if (CXXConstructorSetHasInitializerList(constructor)) {
    VectorAppend(actuals, initializer);
    return NewCXXConstructorCall(syntax, sym, actuals, initializer->location);
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* init = braced->initializers->value.p[i];
    if (init != NULL && init->op == AST_OP(expr_init)) {
      VectorAppend(actuals, ((ExpressionInitializerASTNode*)init)->expr);
    } else if (init != NULL) {
      VectorAppend(actuals, init);
    }
  }
  braced->initializers->length = 0;
  return NewCXXConstructorCall(syntax, sym, actuals, initializer->location);
}

static Vector* ParseCXXInitializerArgumentList(Syntax* syntax, Token close) {
  Vector* actuals = NewVector();
  while (!LexLookingAt(syntax->lex, close)) {
    ASTNode* actual = LexMatch(syntax->lex, TOK(lbrace))
                          ? SyntaxParseBracedInitializer(syntax)
                          : SyntaxParseSingleExpression(syntax,
                                                        TC(closebra) |
                                                            TC(exprsep));
    MarkCXXPackExpansionIfPresent(syntax, actual);
    VectorAppend(actuals, actual);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, close, TC(exprsep) | TC(decl));
  return actuals;
}

static void AnalyzeCXXInitializerArgumentTypes(Vector* actuals) {
  for (size_t i = 0; actuals != NULL && i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual != NULL) {
      VectorSet(actuals, i, AnalyzeExpression(actual));
    }
  }
}

static bool ResolveCXXClassTemplateArgumentDeduction(Syntax* syntax,
                                                     Symbol* sym,
                                                     Vector* actuals,
                                                     bool allow_explicit) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      !TypeIsClassTemplatePlaceholder(sym->type)) {
    return true;
  }
  Symbol* class_template = CXXClassTemplateOrigin(sym->type);
  AnalyzeCXXInitializerArgumentTypes(actuals);
  bool alias_rejected = false;
  TypeRecord* deduced = TypeDeduceClassTemplateFromPlaceholder(
      syntax, sym->type, actuals, allow_explicit, &alias_rejected);
  if (deduced == NULL) {
    if (alias_rejected) {
      SyntaxError(syntax,
                  "Deduced template arguments do not match alias template");
    } else {
      SyntaxError(syntax, "Could not deduce template arguments for %s",
                  class_template != NULL ? class_template->name.value
                                         : "<class template>");
    }
    return false;
  }
  if (!TypeClassTemplatePlaceholderAcceptsDeduced(sym->type, deduced)) {
    SyntaxError(syntax,
                "Deduced template arguments do not match alias template");
    TypeRecordDelete(deduced);
    return false;
  }
  SymbolSetType(sym, deduced);
  TypeRecordDelete(deduced);
  return true;
}

static Vector* CXXDeductionActualsFromInitializer(ASTNode* initializer,
                                                  bool preserve_braced) {
  if (initializer == NULL) {
    return NULL;
  }
  Vector* actuals = NewVector();
  if (initializer->op == AST_OP(expr_init)) {
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)initializer;
    expr_init->expr = AnalyzeExpression(expr_init->expr);
    VectorAppend(actuals, expr_init->expr);
    return actuals;
  }
  if (initializer->op == AST_OP(braced_init)) {
    if (preserve_braced) {
      VectorAppend(actuals, initializer);
      return actuals;
    }
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    for (size_t i = 0; i < braced->initializers->length; i++) {
      ASTNode* init = braced->initializers->value.p[i];
      if (init != NULL && init->op == AST_OP(expr_init)) {
        ExpressionInitializerASTNode* expr_init =
            (ExpressionInitializerASTNode*)init;
        expr_init->expr = AnalyzeExpression(expr_init->expr);
        VectorAppend(actuals, expr_init->expr);
      } else if (init != NULL) {
        VectorAppend(actuals, init);
      }
    }
    return actuals;
  }
  VectorAppend(actuals, AnalyzeExpression(initializer));
  return actuals;
}

static bool ResolveCXXClassTemplateArgumentDeductionFromInitializer(
    Syntax* syntax, Symbol* sym, ASTNode* initializer) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      !TypeIsClassTemplatePlaceholder(sym->type)) {
    return true;
  }
  bool preserve_braced = initializer != NULL &&
                         initializer->op == AST_OP(braced_init) &&
                         TypeHasCXXInitializerListConstructor(sym->type);
  Vector* actuals =
      CXXDeductionActualsFromInitializer(initializer, preserve_braced);
  bool ok = ResolveCXXClassTemplateArgumentDeduction(
      syntax, sym, actuals, /*allow_explicit=*/false);
  VectorDelete(actuals);
  return ok;
}

static ASTNode* NewCXXBracedInitializerFromActuals(Vector* actuals,
                                                   SourceLocation location) {
  Vector* initializers = NewVector();
  for (size_t i = 0; actuals != NULL && i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual != NULL && actual->op == AST_OP(braced_init)) {
      VectorAppend(initializers, actual);
    } else {
      VectorAppend(initializers,
                   NewExpressionInitializerASTNode(actual, location));
    }
  }
  if (actuals != NULL) {
    actuals->length = 0;
    VectorDelete(actuals);
  }
  return NewBracedInitializerASTNode(initializers, NULL, location);
}

static ASTNode* ParseCXXDirectInitializer(Syntax* syntax, Symbol* sym,
                                          bool wrap_aggregate_braces) {
  if (!CompilerIsCXX() || sym == NULL || !TypeIsStructOrUnion(sym->type)) {
    return NULL;
  }
  if (!TypeIsClassTemplatePlaceholder(sym->type) &&
      sym->type->info.struct_info->is_aggregate &&
      LexLookingAt(syntax->lex, TOK(lbrace))) {
    return NULL;
  }
  SourceLocation location = syntax->lex->current_token_location;
  Vector* actuals = NULL;
  bool braced = false;
  if (LexMatch(syntax->lex, TOK(lparen))) {
    actuals = ParseCXXInitializerArgumentList(syntax, TOK(rparen));
  } else if (LexMatch(syntax->lex, TOK(lbrace))) {
    braced = true;
    StructMember* constructor = FindCXXConstructor(sym->type);
    if (CXXConstructorSetHasInitializerList(constructor)) {
      actuals = NewVector();
      VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
    } else {
      actuals = ParseCXXInitializerArgumentList(syntax, TOK(rbrace));
    }
  } else {
    return NULL;
  }
  if (!ResolveCXXClassTemplateArgumentDeduction(
          syntax, sym, actuals, /*allow_explicit=*/true)) {
    VectorDeleteWithContents(actuals, (VectorElementDestructor)ASTNodeDelete,
                             /*free_element=*/false);
    return NULL;
  }
  StructMember* constructor = FindCXXConstructor(sym->type);
  if (constructor == NULL ||
      (braced && sym->type->info.struct_info->is_aggregate)) {
    ASTNode* braced_initializer =
        braced ? NewCXXBracedInitializerFromActuals(actuals, location) : NULL;
    if (braced_initializer == NULL) {
      VectorDeleteWithContents(actuals, (VectorElementDestructor)ASTNodeDelete,
                               /*free_element=*/false);
    }
    if (braced_initializer != NULL && wrap_aggregate_braces) {
      return NewVariableInitExpression(syntax, sym, braced_initializer);
    }
    return braced_initializer;
  }
  return NewCXXConstructorCall(syntax, sym, actuals, location);
}

static ASTNode* NewCXXDefaultConstructorCallIfNeeded(Syntax* syntax,
                                                    Symbol* sym) {
  if (TypeIsFixedArray(sym->type)) {
    unsigned long element_count = 0;
    TypeRecord* element_type =
        CXXArrayBaseElementType(sym->type, &element_count);
    const char* constructor_name =
        CXXConstructorNameForType(element_type);
    if (constructor_name == NULL || element_type->info.struct_info == NULL ||
        element_type->info.struct_info->is_aggregate) {
      return NULL;
    }
    StructMember* ctor = FindStructMemberByName(
        element_type->info.struct_info, constructor_name);
    if (ctor == NULL || !ctor->is_member_function ||
        !ctor->symbol->type->info.function.is_constructor) {
      return NULL;
    }
    ASTNode* sequence = NULL;
    for (unsigned long i = 0; i < element_count; i++) {
      SourceLocation location = sym->location;
      ASTNode* element =
          NewCXXArrayElementExpression(sym, i, location);
      ASTNode* member = NewStringConstantASTNode(
          NewString(constructor_name), NULL, location);
      ASTNode* member_access = NewBinaryASTNode(
          AST_OP(dot), NULL, location, element, member);
      Vector* actuals = NewVector();
      CXXPrependCompleteObjectArgument(element_type, actuals,
                                       /*complete_object=*/true, location);
      ASTNode* call = NewVectorASTNode(
          AST_OP(call), NULL, location, member_access, actuals);
      if (sequence == NULL) {
        sequence = call;
      } else {
        sequence = NewBinaryASTNode(
            AST_OP(comma), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
            location, sequence, call);
      }
    }
    if (sequence == NULL) {
      return NULL;
    }
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatementASTNode(sequence, sym->location));
    return NewUnaryASTNode(
        AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain),
        sym->location,
        NewCompoundStatementASTNode(statements, sym->location));
  }
  const char* constructor_name = CXXConstructorNameForType(sym->type);
  if (constructor_name == NULL) {
    return NULL;
  }
  if (TypeIsStructOrUnion(sym->type) && sym->type->info.struct_info != NULL &&
      sym->type->info.struct_info->is_aggregate) {
    return NULL;
  }
  StructMember* ctor =
      FindStructMemberByName(sym->type->info.struct_info, constructor_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  return NewCXXConstructorCall(syntax, sym, NewVector(), sym->location);
}

static ASTNode* NewCXXDestructorCallOnReceiver(ASTNode* receiver,
                                               TypeRecord* type,
                                               SourceLocation location) {
  if (!CompilerIsCXX() || receiver == NULL || type == NULL ||
      !TypeIsStructOrUnion(type) || type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
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
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(type, actuals,
                                   /*complete_object=*/true, location);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
}

static ASTNode* NewCXXDestructorCallIfNeeded(Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || !TypeIsStructOrUnion(sym->type) ||
      sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  return NewCXXDestructorCallOnReceiver(NewIdentifierASTNode(sym, sym->location),
                                        sym->type, sym->location);
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

static bool FunctionHasSharedCXXLocalStatics(TypeRecord* func) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      func->info.function.symbol == NULL) {
    return false;
  }
  Symbol* symbol = func->info.function.symbol;
  return !StorageIs(symbol->storage, STO(static)) &&
         (func->info.function.is_inline || SymbolHasWeakBinding(symbol));
}

static void AppendSanitizedAsmComponent(String* out, const char* component) {
  for (const unsigned char* p = (const unsigned char*)component; *p != '\0';
       p++) {
    if (isalnum(*p) || *p == '_') {
      StringAppendChar(out, (char)*p);
    } else {
      StringPrintf(out, "_%02x", *p);
    }
  }
}

static void SetCXXInlineLocalStaticAsmNameFor(TypeRecord* func, Symbol* sym,
                                              const char* local_name,
                                              SourceLocation location,
                                              const char* suffix) {
  if (!FunctionHasSharedCXXLocalStatics(func) || sym == NULL) {
    return;
  }

  Symbol* func_symbol = func->info.function.symbol;
  const char* func_name = func_symbol->asm_name.length != 0
                              ? func_symbol->asm_name.value
                              : func_symbol->name.value;
  const char* filename;
  int lineno;
  int start;
  int end;
  DecodeSourceLocation(location, &filename, &lineno, &start, &end);

  String asm_name = {0};
  StringInit(&asm_name, "__davecc_inline_static_");
  AppendSanitizedAsmComponent(&asm_name, func_name);
  StringAppendChar(&asm_name, '_');
  AppendSanitizedAsmComponent(&asm_name, local_name);
  StringPrintf(&asm_name, "_%d_%d", lineno, start);
  if (suffix != NULL && suffix[0] != '\0') {
    StringAppendChar(&asm_name, '_');
    AppendSanitizedAsmComponent(&asm_name, suffix);
  }

  StringSetString(&sym->asm_name, &asm_name);
  StringDestruct(&asm_name);
  sym->flags.is_local = false;
  sym->flags.is_weak = true;
}

static void SetCXXInlineLocalStaticAsmName(Symbol* sym, const char* suffix) {
  if (sym == NULL) {
    return;
  }
  SetCXXInlineLocalStaticAsmNameFor(compiler->current_function, sym,
                                    sym->name.value, sym->location, suffix);
}

static void SetCXXInlineLocalStaticGuardAsmName(TypeRecord* func, Symbol* guard,
                                                Symbol* guarded) {
  if (guard == NULL || guarded == NULL) {
    return;
  }
  SetCXXInlineLocalStaticAsmNameFor(func, guard, guarded->name.value,
                                    guarded->location, "guard");
}

static void RegisterCXXThreadLocalDestructor(Symbol* sym, Symbol* guard) {
  ASTNode* destructor = NewCXXDestructorCallIfNeeded(sym);
  if (destructor == NULL) {
    return;
  }
  SourceLocation location = sym->location;
  if (sym->flags.is_local) {
    return;
  }
  ASTNode* condition = NewBinaryASTNode(
      AST_OP(noteq), NULL, location, NewIdentifierASTNode(guard, location),
      NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
  Vector* destructor_statements = NewVector();
  VectorAppend(destructor_statements, destructor);
  ASTNode* guarded_destructor = NewIfStatementASTNode(
      condition, NewCompoundStatementASTNode(destructor_statements, location),
      NULL, false, location);
  VectorAppend(&compiler->cxx_thread_destructor_calls, guarded_destructor);
}

static Symbol* RegisterCXXTlsBlockDtorThunk(Syntax* syntax, Symbol* sym) {
  SourceLocation location = sym->location;
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_ptr = NewPointerTo(kQualPlain, void_type);
  TypeRecord* func_type = NewFunctionTypeRecord();
  TypeRecordChain(func_type, void_type);
  Symbol* param = NewSymbol("__ignored", void_ptr, STO(auto));
  param->flags.is_argument = true;
  param->flags.invented = true;
  param->location = location;
  param->value.arg_number = 0;
  VectorAppend(&func_type->info.function.prototype, param);

  String name;
  StringInit(&name, "__davecc_tls_block_dtor_");
  char suffix[32];
  snprintf(suffix, sizeof(suffix), "%zu_",
           compiler->cxx_tls_block_dtor_thunks.length);
  StringAppend(&name, suffix);
  AppendSanitizedAsmComponent(&name, sym->name.value);

  Symbol* thunk = NewSymbol(name.value, func_type, STO(static));
  thunk->flags.invented = true;
  thunk->flags.is_defined = true;
  thunk->location = location;
  func_type->info.function.symbol = thunk;
  func_type->info.function.definition = true;
  StringDestruct(&name);

  Vector* body_statements = NewVector();
  ASTNode* destructor =
      NewCXXDestructorCallOnReceiver(NewIdentifierASTNode(sym, location),
                                     sym->type, location);
  if (destructor != NULL) {
    VectorAppend(body_statements, destructor);
  }
  func_type->info.function.body =
      NewCompoundStatementASTNode(body_statements, location);

  bool added = SyntaxAddSymbol(syntax, thunk);
  assert(added);
  (void)added;

  CXXTlsBlockDtorThunk* entry = malloc(sizeof(CXXTlsBlockDtorThunk));
  entry->thunk = thunk;
  VectorAppend(&compiler->cxx_tls_block_dtor_thunks, entry);
  return thunk;
}

static ASTNode* NewCXXTlsRegisterBlockDtorCall(Symbol* thunk, Symbol* object,
                                               SourceLocation location) {
  (void)object;
  TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
  TypeRecord* void_ptr = NewPointerTo(kQualPlain, void_type);
  TypeRecordCalculateSize(void_ptr);

  TypeRecord* register_func = NewFunctionTypeRecord();
  TypeRecordChain(register_func, void_type);
  Symbol* fn_param = NewSymbol("__fn", void_ptr, STO(auto));
  fn_param->flags.is_argument = true;
  fn_param->flags.invented = true;
  fn_param->value.arg_number = 0;
  Symbol* obj_param = NewSymbol("__obj", void_ptr, STO(auto));
  obj_param->flags.is_argument = true;
  obj_param->flags.invented = true;
  obj_param->value.arg_number = 1;
  VectorAppend(&register_func->info.function.prototype, fn_param);
  VectorAppend(&register_func->info.function.prototype, obj_param);
  TypeRecordCalculateSize(register_func);

  ASTNode* thunk_id = NewIdentifierASTNode(thunk, location);
  thunk->flags.address_taken = true;
  TypeRecord* thunk_ptr_type = NewPointerTo(kQualPlain, thunk->type);
  TypeRecordCalculateSize(thunk_ptr_type);
  ASTNode* thunk_addr =
      NewUnaryASTNode(AST_OP(address), thunk_ptr_type, location, thunk_id);

  ASTNode* null_arg =
      NewIntConstantASTNode(0, void_ptr, location);

  Vector* args = NewVector();
  VectorAppend(args, thunk_addr);
  VectorAppend(args, null_arg);

  Symbol* register_sym =
      NewSymbol("__davecc_tls_register_block_dtor", register_func, STO(extern));
  ASTNode* call = NewVectorASTNode(
      AST_OP(call), NULL, location, NewIdentifierASTNode(register_sym, location),
      args);
  return NewExpressionStatementASTNode(call, location);
}

static ASTNode* CXXThreadLocalInitializerExpression(ASTNode* initializer) {
  if (initializer == NULL) {
    return NULL;
  }
  if (initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer->op == AST_OP(expr_init)) {
    return ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  return initializer;
}

static bool CXXThreadLocalInitializerIsDynamic(ASTNode* initializer) {
  ASTNode* expr = CXXThreadLocalInitializerExpression(initializer);
  return expr != NULL && !IsConstantExpression(expr);
}

static ASTNode* NewCXXThreadLocalGuardedInit(Syntax* syntax, Symbol* sym,
                                             ASTNode* init) {
  if (init == NULL) {
    init = NewCXXDefaultConstructorCallIfNeeded(syntax, sym);
    if (init == NULL) {
      return NULL;
    }
  }
  init = NewExpressionStatementASTNode(init, sym->location);

  SourceLocation location = sym->location;
  Symbol* guard = NewSymbol(
      SyntaxFakeName(syntax), NewTypeRecordWithSize(kTypeInt, kQualPlain),
      STO(static) | STO(thread));
  guard->flags.invented = true;
  guard->flags.is_defined = true;
  guard->flags.is_local = true;
  guard->location = location;
  SetCXXInlineLocalStaticGuardAsmName(compiler->current_function, guard, sym);
  bool added = SyntaxAddSymbol(syntax, guard);
  assert(added);
  (void)added;
  VectorAppend(&syntax->local_statics,
               NewVariableDeclarationASTNode(guard, NULL, location));

  Symbol* block_dtor_thunk = NULL;
  if (sym->flags.is_local && NewCXXDestructorCallIfNeeded(sym) != NULL) {
    block_dtor_thunk = RegisterCXXTlsBlockDtorThunk(syntax, sym);
  } else {
    RegisterCXXThreadLocalDestructor(sym, guard);
  }

  Vector* guarded_statements = NewVector();
  VectorAppend(guarded_statements, init);
  VectorAppend(guarded_statements,
               NewExpressionStatementASTNode(
                   NewIntAssignment(guard, 1, location), location));
  if (block_dtor_thunk != NULL) {
    VectorAppend(guarded_statements,
                 NewCXXTlsRegisterBlockDtorCall(block_dtor_thunk, sym,
                                                location));
  }

  ASTNode* condition = NewBinaryASTNode(
      AST_OP(equal), NULL, location, NewIdentifierASTNode(guard, location),
      NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location));
  ASTNode* guarded =
      NewIfStatementASTNode(condition,
                            NewCompoundStatementASTNode(guarded_statements,
                                                        location),
                            NULL, false, location);

  Vector* statements = NewVector();
  VectorAppend(statements, guarded);
  return NewUnaryASTNode(AST_OP(stmt_expr),
                         NewTypeRecordWithSize(kTypeVoid, kQualPlain),
                         location, NewCompoundStatementASTNode(statements,
                                                               location));
}

static ASTNode* NewCXXThreadLocalGuardedConstructor(Syntax* syntax,
                                                    Symbol* sym) {
  return NewCXXThreadLocalGuardedInit(syntax, sym, NULL);
}

static Symbol* FindCXXDestructorForType(TypeRecord* type) {
  if (!CompilerIsCXX() || type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL ||
      type->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, type->info.struct_info->tag_name);
  StructMember* destructor =
      FindStructMember(type->info.struct_info, &destructor_name);
  StringDestruct(&destructor_name);
  if (destructor == NULL || !destructor->is_member_function ||
      destructor->symbol == NULL ||
      !destructor->symbol->type->info.function.is_destructor ||
      destructor->symbol->type->info.function.is_trivial_special_member) {
    return NULL;
  }
  return destructor->symbol;
}

static Symbol* GetCXXRuntimeFunction(const char* name, Type return_kind,
                                     SourceLocation location) {
  String runtime_name;
  StringInit(&runtime_name, name);
  Symbol* symbol = FindGlobalSymbol(&runtime_name);
  StringDestruct(&runtime_name);
  if (symbol != NULL) {
    return symbol;
  }
  TypeRecord* function = NewFunctionTypeRecord();
  TypeRecordChain(function,
                  NewTypeRecordWithSize(return_kind, kQualPlain));
  symbol = NewSymbol(name, function, STO(extern));
  symbol->flags.invented = true;
  symbol->flags.is_forward_declared = true;
  symbol->location = location;
  bool added = SyntaxAddSymbol(&compiler->syntax, symbol);
  assert(added);
  (void)added;
  return symbol;
}

static ASTNode* NewCXXRuntimeCall(const char* name, Type return_kind,
                                  Vector* actuals,
                                  SourceLocation location) {
  Symbol* function = GetCXXRuntimeFunction(name, return_kind, location);
  ASTNode* callee = NewIdentifierASTNode(function, location);
  callee->flags |= kASTNeedAddress;
  return NewVectorASTNode(AST_OP(call), TypeRecordCopy(function->type->next),
                          location, callee, actuals);
}

static ASTNode* NewSymbolAddress(Symbol* symbol, SourceLocation location) {
  TypeRecord* pointer = NewPointerTo(kQualPlain, TypeRecordCopy(symbol->type));
  ASTNode* identifier = NewIdentifierASTNode(symbol, location);
  identifier->flags |= kASTNeedAddress;
  return NewUnaryASTNode(AST_OP(address), pointer, location, identifier);
}

static ASTNode* NewGuardRuntimeCall(const char* name, Type return_kind,
                                    Symbol* guard,
                                    SourceLocation location) {
  Vector* actuals = NewVector();
  VectorAppend(actuals, NewSymbolAddress(guard, location));
  return NewCXXRuntimeCall(name, return_kind, actuals, location);
}

static ASTNode* NewCXXAtexitRegistration(Symbol* sym,
                                         SourceLocation location) {
  TypeRecord* object_type = sym->type;
  unsigned long count = 1;
  if (TypeIsFixedArray(object_type)) {
    object_type = CXXArrayBaseElementType(object_type, &count);
  }
  Symbol* destructor = FindCXXDestructorForType(object_type);
  if (destructor == NULL) {
    return NULL;
  }
  Vector* actuals = NewVector();
  ASTNode* destructor_address = NewIdentifierASTNode(destructor, location);
  destructor_address->flags |= kASTNeedAddress;
  VectorAppend(actuals, destructor_address);
  VectorAppend(actuals, NewSymbolAddress(sym, location));
  VectorAppend(actuals,
               NewIntConstantASTNode(
                   count,
                   NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain),
                   location));
  VectorAppend(actuals,
               NewIntConstantASTNode(
                   object_type->size,
                   NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain),
                   location));
  VectorAppend(actuals,
               NewIntConstantASTNode(
                   TypeNeedsCXXCompleteObjectArgument(object_type) ? 1 : 0,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
  return NewCXXRuntimeCall("__davecc_cxa_atexit", kTypeInt, actuals, location);
}

ASTNode* SyntaxNewCXXGlobalAtexitStatement(Symbol* sym,
                                           SourceLocation location) {
  ASTNode* registration = NewCXXAtexitRegistration(sym, location);
  if (registration == NULL) {
    return NULL;
  }
  return NewExpressionStatementASTNode(registration, location);
}

static bool FunctionTypeIsVoidVoid(TypeRecord* type) {
  if (type == NULL || !TypeIsFunction(type) || type->next == NULL ||
      !TypeIsVoid(type->next)) {
    return false;
  }
  return type->info.function.prototype.length == 0;
}

static int ParseInitFiniAttributePriority(Syntax* syntax, Attribute* attr,
                                          SourceLocation location) {
  int priority = kCXXInitFiniPriorityDefault;
  if (AttributeArgCount(attr) > 0) {
    long value = 0;
    if (!AttributeArgInt(attr, 0, &value) || value < 0 || value > 65535) {
      SyntaxErrorAtLocation(
          syntax, location,
          "constructor/destructor attribute priority must be an integer "
          "between 0 and 65535");
      return -1;
    }
    priority = (int)value;
    if (priority < 101) {
      SyntaxWarning(syntax, "attributes",
                    "priority %d is reserved for the implementation", priority);
    }
  }
  return priority;
}

static void RegisterInitFiniArrayEntry(Vector* functions, Symbol* function) {
  VectorAppend(functions, function);
}

void SyntaxRegisterFunctionInitFiniAttributes(Syntax* syntax, Symbol* sym) {
  if (sym == NULL || !TypeIsFunction(sym->type) || !sym->flags.is_defined) {
    return;
  }
  Attribute* constructor =
      AttributeListFind(&sym->attributes, "constructor");
  Attribute* destructor = AttributeListFind(&sym->attributes, "destructor");
  if (constructor == NULL && destructor == NULL) {
    return;
  }
  if (!FunctionTypeIsVoidVoid(sym->type)) {
    SyntaxErrorAtLocation(
        syntax, sym->location,
        "constructor/destructor attribute applies only to void functions "
        "with no parameters");
    return;
  }
  if (sym->flags.is_template) {
    SyntaxErrorAtLocation(syntax, sym->location,
                          "constructor/destructor attribute cannot be applied "
                          "to a function template");
    return;
  }
  if (constructor != NULL) {
    int priority =
        ParseInitFiniAttributePriority(syntax, constructor, sym->location);
    if (priority >= 0) {
      RegisterInitFiniArrayEntry(CXXInitArrayFunctionsVector(), sym);
      sym->flags.used = true;
    }
  }
  if (destructor != NULL) {
    int priority =
        ParseInitFiniAttributePriority(syntax, destructor, sym->location);
    if (priority >= 0) {
      RegisterInitFiniArrayEntry(CXXFiniArrayFunctionsVector(), sym);
      sym->flags.used = true;
    }
  }
}

static Symbol* NewCXXLocalStaticGuard(Syntax* syntax, TypeRecord* func,
                                      Symbol* sym) {
  TypeRecord* guard_type =
      NewTypeRecordWithSize(kTypeLongLong | kTypeUnsigned, kQualPlain);
  Symbol* guard = NewSymbol(SyntaxFakeName(syntax), guard_type, STO(static));
  guard->flags.invented = true;
  guard->flags.is_defined = true;
  guard->flags.is_local = true;
  guard->alignment = 8;
  guard->location = sym->location;
  SetCXXInlineLocalStaticGuardAsmName(func, guard, sym);
  VectorAppend(&syntax->all_local_symbols, guard);
  return guard;
}

static ASTNode* NewIntegerAssignment(Symbol* symbol, int value,
                                     SourceLocation location) {
  ASTNode* destination = NewIdentifierASTNode(symbol, location);
  destination->flags |= kASTNeedAddress;
  return NewBinaryASTNode(
      AST_OP(assign), TypeRecordCopy(symbol->type), location,
      destination,
      NewIntConstantASTNode(
          value, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
}

static void CollectCommaExpressions(ASTNode* expression, Vector* expressions) {
  if (expression != NULL && expression->op == AST_OP(comma)) {
    BinaryASTNode* comma = (BinaryASTNode*)expression;
    CollectCommaExpressions(comma->left, expressions);
    CollectCommaExpressions(comma->right, expressions);
    return;
  }
  if (expression != NULL) {
    VectorAppend(expressions, expression);
  }
}

static ASTNode* ArrayElementAddress(Symbol* array, int index,
                                    SourceLocation location) {
  TypeRecord* element_type = CXXArrayBaseElementType(array->type, NULL);
  ASTNode* element =
      NewCXXArrayElementExpression(array, (unsigned long)index, location);
  ASTNode* base = element;
  while (base != NULL && base->op == AST_OP(subscript)) {
    base->flags |= kASTNeedAddress;
    ASTNode* left = ((BinaryASTNode*)base)->left;
    if (left != NULL && left->op == AST_OP(identifier)) {
      left->flags |= kASTNeedAddress;
    }
    base = left;
  }
  element->flags |= kASTNeedAddress;
  return NewUnaryASTNode(
      AST_OP(address), NewPointerTo(kQualPlain, TypeRecordCopy(element_type)),
      location, element);
}

static ASTNode* NewDirectCXXSpecialMemberCall(Symbol* function,
                                              ASTNode* object_address,
                                              TypeRecord* object_type,
                                              SourceLocation location) {
  ASTNode* callee = NewIdentifierASTNode(function, location);
  callee->flags |= kASTNeedAddress;
  Vector* actuals = NewVector();
  VectorAppend(actuals, object_address);
  if (TypeNeedsCXXCompleteObjectArgument(object_type)) {
    VectorAppend(actuals,
                 NewIntConstantASTNode(
                     1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                     location));
  }
  return NewVectorASTNode(
      AST_OP(call), TypeRecordCopy(function->type->next), location, callee,
      actuals);
}

static Symbol* NewArrayConstructionCount(Syntax* syntax,
                                         SourceLocation location) {
  Symbol* count =
      NewSymbol(SyntaxFakeName(syntax),
                NewTypeRecordWithSize(kTypeInt, kQualPlain), STO(auto));
  count->flags.invented = true;
  count->flags.is_defined = true;
  count->flags.is_local = true;
  count->location = location;
  VectorAppend(&syntax->all_local_symbols, count);
  return count;
}

static bool PrepareDefaultArrayInitialization(
    Syntax* syntax, VariableDeclarationASTNode* declaration, Vector* success,
    Symbol** constructed_count) {
  Symbol* array = declaration->symbol;
  ASTNode* initializer = declaration->initializer;
  if (!TypeIsFixedArray(array->type) || initializer == NULL ||
      initializer->op != AST_OP(stmt_expr)) {
    return false;
  }
  UnaryASTNode* statement_expression = (UnaryASTNode*)initializer;
  if (statement_expression->sub == NULL ||
      statement_expression->sub->op != AST_OP(compound)) {
    return false;
  }
  CompoundStatementASTNode* compound =
      (CompoundStatementASTNode*)statement_expression->sub;
  if (compound->statements->length != 1) {
    return false;
  }
  ASTNode* statement = VectorGet(compound->statements, 0);
  if (statement == NULL || statement->op != AST_OP(expr)) {
    return false;
  }

  Vector constructors;
  VectorInit(&constructors);
  CollectCommaExpressions(((ExpressionStatementASTNode*)statement)->expr,
                          &constructors);
  unsigned long element_count = 0;
  CXXArrayBaseElementType(array->type, &element_count);
  if (constructors.length != (size_t)element_count) {
    VectorDestruct(&constructors);
    return false;
  }

  Symbol* count = NewArrayConstructionCount(syntax, declaration->base.location);
  *constructed_count = count;
  VectorAppend(success,
               NewVariableDeclarationASTNode(
                   count,
                   NewIntegerAssignment(count, 0, declaration->base.location),
                   declaration->base.location));
  for (size_t i = 0; i < constructors.length; i++) {
    VectorAppend(success,
                 NewExpressionStatementASTNode(
                     VectorGet(&constructors, i), declaration->base.location));
    VectorAppend(success,
                 NewExpressionStatementASTNode(
                     NewIntegerAssignment(count, (int)i + 1,
                                          declaration->base.location),
                     declaration->base.location));
  }
  VectorDestruct(&constructors);
  return true;
}

static void AppendArrayInitializationCleanup(
    VariableDeclarationASTNode* declaration, Symbol* constructed_count,
    Vector* failure) {
  if (constructed_count == NULL) {
    return;
  }
  Symbol* array = declaration->symbol;
  unsigned long element_count = 0;
  TypeRecord* element_type =
      CXXArrayBaseElementType(array->type, &element_count);
  Symbol* destructor = FindCXXDestructorForType(element_type);
  if (destructor == NULL) {
    return;
  }
  SourceLocation location = declaration->base.location;
  for (int i = (int)element_count - 1; i >= 0; i--) {
    ASTNode* condition = NewBinaryASTNode(
        AST_OP(greater), NewTypeRecordWithSize(kTypeBool, kQualPlain), location,
        NewIdentifierASTNode(constructed_count, location),
        NewIntConstantASTNode(
            i, NewTypeRecordWithSize(kTypeInt, kQualPlain), location));
    ASTNode* destructor_call = NewDirectCXXSpecialMemberCall(
        destructor, ArrayElementAddress(array, i, location), element_type,
        location);
    Vector* statements = NewVector();
    VectorAppend(statements,
                 NewExpressionStatementASTNode(destructor_call, location));
    VectorAppend(
        failure,
        NewIfStatementASTNode(
            condition, NewCompoundStatementASTNode(statements, location), NULL,
            false, location));
  }
}

static ASTNode* NewCXXLocalStaticGuardedInitializer(
    Syntax* syntax, TypeRecord* func, VariableDeclarationASTNode* declaration,
    bool run_initializer) {
  Symbol* sym = declaration->symbol;
  SourceLocation location = declaration->base.location;
  ASTNode* original_initializer = declaration->initializer;
  Symbol* guard = NewCXXLocalStaticGuard(syntax, func, sym);
  declaration->local_static_guard = guard;

  Vector* success = NewVector();
  Symbol* constructed_count = NULL;
  if (!run_initializer) {
    // Constant initialization already resides in static storage.  The guard
    // exists only to register a non-trivial destructor on first passage.
  } else if (!PrepareDefaultArrayInitialization(syntax, declaration, success,
                                                &constructed_count)) {
    VectorAppend(success, NewExpressionStatementASTNode(
                              declaration->initializer, location));
  }
  ASTNode* registration = NewCXXAtexitRegistration(sym, location);
  if (registration != NULL) {
    VectorAppend(success,
                 NewExpressionStatementASTNode(registration, location));
  }
  VectorAppend(success,
               NewExpressionStatementASTNode(
                   NewGuardRuntimeCall("__cxa_guard_release", kTypeVoid, guard,
                                       location),
                   location));

  ASTNode* success_body = NewCompoundStatementASTNode(success, location);
  ASTNode* initialization = success_body;
  if (CompilerExceptionsEnabled() && compiler->target_name != NULL &&
      StringEqual(compiler->target_name, "x86_64")) {
    Vector* catches = NewVector();
    Vector* failure = NewVector();
    AppendArrayInitializationCleanup(declaration, constructed_count, failure);
    VectorAppend(failure,
                 NewExpressionStatementASTNode(
                     NewGuardRuntimeCall("__cxa_guard_abort", kTypeVoid, guard,
                                         location),
                     location));
    VectorAppend(failure,
                 NewExpressionStatementASTNode(
                     NewThrowASTNode(NULL, location), location));
    VectorAppend(catches,
                 NewCatchASTNode(
                     NULL, true,
                     NewCompoundStatementASTNode(failure, location), location));
    initialization = NewTryASTNode(success_body, catches, location);
  }

  Vector* guarded_statements = NewVector();
  VectorAppend(guarded_statements, initialization);
  ASTNode* guarded_if = NewIfStatementASTNode(
      NewGuardRuntimeCall("__cxa_guard_acquire", kTypeInt, guard, location),
      NewCompoundStatementASTNode(guarded_statements, location), NULL, false,
      location);
  Vector* statements = NewVector();
  VectorAppend(statements, guarded_if);
  ASTNode* guarded = NewUnaryASTNode(
      AST_OP(stmt_expr), NewTypeRecordWithSize(kTypeVoid, kQualPlain), location,
      NewCompoundStatementASTNode(statements, location));
  if (!run_initializer && original_initializer != NULL) {
    return NewBinaryASTNode(
        AST_OP(comma), TypeRecordCopy(original_initializer->type), location,
        guarded, original_initializer);
  }
  return guarded;
}

typedef struct {
  Syntax* syntax;
  TypeRecord* function;
} CXXLocalStaticPreparation;

static void FindCXXLocalStaticConstantInitializer(
    ASTNode* node, void* data, int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL &&
      node->op == AST_OP(init) && (node->flags & kASTStaticInit) != 0) {
    *(bool*)data = true;
  }
}

static void PrepareCXXLocalStaticVisitor(ASTNode* node, void* data, int child_id,
                                         VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL ||
      node->op != AST_OP(vardecl)) {
    return;
  }
  VariableDeclarationASTNode* declaration =
      (VariableDeclarationASTNode*)node;
  Symbol* symbol = declaration->symbol;
  if (symbol == NULL || !StorageIs(symbol->storage, STO(static)) ||
      StorageIs(symbol->storage, STO(thread))) {
    return;
  }

  CXXLocalStaticPreparation* preparation = data;
  SetCXXInlineLocalStaticAsmNameFor(
      preparation->function, symbol, symbol->name.value, symbol->location, "");
  if (declaration->local_static_init_kind == kLocalStaticInitUnclassified) {
    bool constant = declaration->initializer == NULL;
    ASTNodeVisit(declaration->initializer,
                 FindCXXLocalStaticConstantInitializer, 0, &constant);
    declaration->local_static_init_kind =
        constant ? kLocalStaticInitConstant : kLocalStaticInitDynamic;
  }
  bool dynamic =
      declaration->local_static_init_kind == kLocalStaticInitDynamic;
  TypeRecord* destructor_type = symbol->type;
  if (TypeIsFixedArray(destructor_type)) {
    destructor_type = CXXArrayBaseElementType(destructor_type, NULL);
  }
  bool needs_destructor_registration =
      FindCXXDestructorForType(destructor_type) != NULL;
  if ((!dynamic && !needs_destructor_registration) ||
      (dynamic && declaration->initializer == NULL) ||
      declaration->local_static_guard != NULL) {
    return;
  }
  declaration->initializer = NewCXXLocalStaticGuardedInitializer(
      preparation->syntax, preparation->function, declaration, dynamic);
  declaration->initializer->parent = node;
}

void SyntaxPrepareCXXLocalStatics(Syntax* syntax, TypeRecord* function) {
  if (!CompilerIsCXX() || syntax == NULL || function == NULL ||
      !TypeIsFunction(function) || function->info.function.body == NULL) {
    return;
  }
  CXXLocalStaticPreparation preparation = {syntax, function};
  ASTNodeVisit(function->info.function.body, PrepareCXXLocalStaticVisitor, 0,
               &preparation);
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

static Vector* ParseStructuredBindingNames(Syntax* syntax) {
  if (!LexMatch(syntax->lex, TOK(lsquare))) {
    return NULL;
  }
  Vector* names = NewVector();
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected structured binding name");
      break;
    }
    VectorAppend(names, NewString(syntax->lex->spelling.value));
    LexNextToken(syntax->lex);
    if (!LexMatch(syntax->lex, TOK(comma))) {
      break;
    }
  }
  SyntaxNeedBracket(syntax, TOK(rsquare), TC(closebra));
  return names;
}

static TypeRecord* ParseStructuredBindingDeclaredType(Syntax* syntax,
                                                      TypeRecord* base_type) {
  TypeRecord* declared = TypeRecordCopy(base_type);
  if (CompilerIsCXX() &&
      (LexLookingAt(syntax->lex, TOK(amp)) ||
       LexLookingAt(syntax->lex, TOK(ampamp)))) {
    bool rvalue = LexMatch(syntax->lex, TOK(ampamp));
    if (!rvalue) {
      LexMatch(syntax->lex, TOK(amp));
    }
    TypeRecord* ref = NewReferenceTypeRecord(kQualPlain, rvalue);
    TypeRecordChain(ref, declared);
    TypeRecordCalculateSize(ref);
    declared = ref;
  }
  return declared;
}

static bool TryParseStructuredBindingDeclaration(Syntax* syntax,
                                                 TypeRecord* base_type,
                                                 Storage storage,
                                                 Vector* declarations) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX17)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  TypeRecord* declared_type =
      ParseStructuredBindingDeclaredType(syntax, base_type);
  if (!LexLookingAt(syntax->lex, TOK(lsquare))) {
    TypeRecordDelete(declared_type);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return false;
  }
  Vector* names = ParseStructuredBindingNames(syntax);
  LexCheckpointDestruct(&checkpoint);
  if (names == NULL || names->length == 0) {
    SyntaxError(syntax, "Structured binding declaration requires at least one name");
  }
  if (StorageIs(storage, STO(extern))) {
    SyntaxError(syntax, "Structured binding declaration cannot be extern");
  }
  ASTNode* initializer = NULL;
  if (LexMatch(syntax->lex, TOK(equal))) {
    if (LexMatch(syntax->lex, TOK(lbrace))) {
      initializer = ParseBracedInitializer(syntax);
    } else {
      initializer = NewExpressionInitializerASTNode(
          SyntaxParseSingleExpression(syntax, TC(semicolon) | TC(stmt)),
          syntax->lex->current_token_location);
    }
  } else if (LexMatch(syntax->lex, TOK(lbrace))) {
    initializer = ParseBracedInitializer(syntax);
  } else {
    SyntaxError(syntax, "Structured binding declaration requires an initializer");
  }
  Vector* symbols = NewVector();
  for (size_t i = 0; names != NULL && i < names->length; i++) {
    String* name = names->value.p[i];
    Symbol* sym = NewSymbol(name->value, NewTypeRecord(kTypeAuto, kQualPlain),
                            STO(implicit));
    sym->flags.is_local = true;
    sym->flags.is_defined = true;
    sym->location = syntax->lex->current_token_location;
    if (!SyntaxAddSymbol(syntax, sym)) {
      SyntaxError(syntax, "Duplicate structured binding name: %s",
                  name->value);
      SymbolDelete(sym);
    } else {
      VectorAppend(symbols, sym);
    }
  }
  VectorAppend(declarations,
               NewStructuredBindingASTNode(declared_type, names, symbols,
                                           initializer,
                                           syntax->lex->current_token_location));
  return true;
}

// True if `type` is a class with a user-declared copy or move constructor.  Such
// a class manages its own copy semantics (e.g. owns a resource), so a member-wise
// byte copy of one of its objects is wrong; copy-initialization must run the
// constructor.  Classes without one are trivially/implicitly copyable, where the
// member-wise `init` path (which also handles user-defined conversion operators)
// is both correct and necessary -- e.g. the comparison categories convert via
// `operator T()` and would otherwise hit their private value constructor.
static bool CXXTypeHasUserDeclaredCopyOrMoveConstructor(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  for (StructMember* c = FindCXXConstructor(type); c != NULL;
       c = c->overload_next) {
    if (c->is_member_function && c->symbol != NULL && c->symbol->type != NULL &&
        TypeIsFunction(c->symbol->type) &&
        c->symbol->type->info.function.is_user_declared &&
        (c->symbol->type->info.function.cxx_special_member_kind ==
             kCXXSpecialMemberCopyConstructor ||
         c->symbol->type->info.function.cxx_special_member_kind ==
             kCXXSpecialMemberMoveConstructor)) {
      return true;
    }
  }
  return false;
}

// Copy-initialization of a class object from a single expression, `T b = expr;`.
// Like direct-initialization `T b(expr);`, this must select and invoke a
// constructor (copy, move, or converting) rather than degrading to a shallow
// member-wise `init`.  A member-wise init merely copies the bytes of the source
// object, which for a class managing a resource (e.g. a string's heap pointer)
// aliases that resource instead of running the user-defined copy constructor and
// leads to double-frees / dangling pointers.
//
// This rewrite is limited to classes with a user-declared copy/move constructor:
// only those need a constructor invocation here.  Trivially-copyable classes are
// left to the member-wise `init` path, which also performs user-defined
// conversions (constructing one type from another via `operator T()`); rewriting
// those into a constructor call would bypass the conversion operator and select
// a possibly-inaccessible value constructor instead.
//
// `initializer` is the node produced by SyntaxParseInitializer after the braced
// list-constructor rewrite; only a plain expression initializer (`expr_init`)
// for a non-aggregate class type with at least one constructor is rewritten.
static ASTNode* NewCXXCopyInitConstructorInitializer(Syntax* syntax, Symbol* sym,
                                                     ASTNode* initializer) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      initializer == NULL || initializer->op != AST_OP(expr_init) ||
      !TypeIsStructOrUnion(sym->type) || sym->type->info.struct_info == NULL ||
      sym->type->info.struct_info->is_aggregate) {
    return initializer;
  }
  if (FindCXXConstructor(sym->type) == NULL ||
      !CXXTypeHasUserDeclaredCopyOrMoveConstructor(sym->type)) {
    return initializer;
  }
  ExpressionInitializerASTNode* expr_init =
      (ExpressionInitializerASTNode*)initializer;
  ASTNode* expr = expr_init->expr;
  if (expr == NULL) {
    return initializer;
  }
  // Hand the operand to the constructor call and detach it from the wrapper so
  // the discarded `expr_init` node does not co-own it.
  expr_init->expr = NULL;
  Vector* actuals = NewVector();
  VectorAppend(actuals, expr);
  return NewCXXConstructorCall(syntax, sym, actuals, initializer->location);
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
      if (!TypeIsFunction(sym->type)) {
        sym->flags.is_constexpr = parser->is_constexpr;
        sym->flags.is_constinit = parser->is_constinit;
        if (sym->flags.is_constexpr) {
          sym->type->qualifiers |= kQualConst;
        }
      }
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
              String symbol_name;
              StringInit(&symbol_name, NULL);
              SymbolFunctionDiagnosticName(sym, &symbol_name);
              SyntaxError(syntax, "Duplicate definition of local symbol %s",
                          symbol_name.value);
              StringDestruct(&symbol_name);
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

        if (!RedeclarationTypesEqual(sym->type, old_sym->type)) {
          String suffix;
          StringInit(&suffix, NULL);
          SymbolFunctionDiagnosticSuffix(sym, &suffix);
          SyntaxError(syntax, "Symbol %s redeclared with different type%s",
                      sym->name.value, suffix.value);
          StringDestruct(&suffix);
          TypeErrorDetails(syntax->lex->current_token_location,
                                    sym->type, old_sym->type);
           const char* filename;
           int lineno, start, end;
           DecodeSourceLocation(old_sym->location, &filename, &lineno, &start, &end);
           ReportNote(filename, lineno, "Previously declared here");
        } else {
          MergeCXXDefaultArguments(syntax, old_sym, sym);
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
          if (!TypeIsFunction(sym->type) && parser->is_constexpr) {
            sym->flags.is_constexpr = true;
            sym->type->qualifiers |= kQualConst;
          }
          if (!TypeIsFunction(sym->type) && parser->is_constinit) {
            sym->flags.is_constinit = true;
          }
        }
      } else {
        if (StorageIs(storage, STO(extern))) {
          Symbol* link = FindFileScopeSymbol(syntax, &sym->name);
          if (link != NULL) {
            SymbolDelete(sym);
            sym = link;
          } else {
            bool added = SyntaxAddSymbol(syntax, sym);
            assert(added);
            (void)added;
          }
        } else {
          // This is the first declaration of this symbol, add to the symbol
          // table.
          bool added = SyntaxAddSymbol(syntax, sym);
          assert(added);
          (void)added;
        }
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

    // Parse trailing __attribute__ / C++ attribute syntax.
    while (LexLookingAt(syntax->lex, TOK(attribute)) ||
           SyntaxLookingAtCXXAttribute(syntax)) {
      if (LexMatch(syntax->lex, TOK(attribute))) {
        SyntaxParseAttribute(syntax, attributes);
      } else {
        SyntaxParseCXXAttributes(syntax, attributes);
      }
    }

    // Symbol takes ownerhip of attribute strings.
    VectorCopy(&sym->attributes, attributes);
    VectorClear(attributes);
    SyntaxApplyDeclarationAttributes(sym);
    if (StorageIs(storage, STO(static)) && !TypeIsFunction(sym->type)) {
      SetCXXInlineLocalStaticAsmName(sym, "");
    }
    
    // Check for thread-local violations.
    SyntaxCheckThreadLocal(syntax, sym, kParsingBlockScope, false, false);

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
      if (!StorageIs(sym->storage, STO(extern))) {
        sym->flags.is_defined = true;
      }
      if (parser->is_inline) {
         SyntaxError(syntax, "inline can only be applied to functions");
      }
      if (!TypeIsClassTemplatePlaceholder(sym->type) &&
          TypeIsAbstractClass(sym->type)) {
        SyntaxError(syntax, "Cannot declare object of abstract class %s",
                    sym->type->info.struct_info->tag_name != NULL
                        ? sym->type->info.struct_info->tag_name->value
                        : "<anonymous>");
      }
      if (!syntax->parsing_template_declaration &&
          TypeContainsClassTemplate(sym->type) &&
          !TypeIsClassTemplatePlaceholder(sym->type)) {
        SyntaxError(syntax, "Class template instantiation is not supported yet");
      }
      // Any initializer?
      ASTNode* initializer = NULL;
      if (LexMatch(syntax->lex, TOK(equal))) {
        syntax->init_storage = storage;
        initializer = SyntaxParseInitializer(syntax, sym, storage);
        ResolveCXXClassTemplateArgumentDeductionFromInitializer(
            syntax, sym, initializer);
        initializer =
            NewCXXCopyListConstructorInitializer(syntax, sym, initializer);
        initializer =
            NewCXXCopyInitConstructorInitializer(syntax, sym, initializer);
        if (initializer == NULL || initializer->op != AST_OP(call)) {
          initializer = NewVariableInitExpression(syntax, sym, initializer);
        }
        if (!StorageIs(sym->storage, STO(extern)) &&
            StorageIs(sym->storage, STO(thread)) &&
            CXXThreadLocalInitializerIsDynamic(initializer)) {
          initializer = NewCXXThreadLocalGuardedInit(syntax, sym, initializer);
        }
      } else {
        initializer = ParseCXXDirectInitializer(syntax, sym, true);
        if (initializer == NULL && CompilerIsCXX() &&
            LexMatch(syntax->lex, TOK(lbrace))) {
          initializer =
              NewVariableInitExpression(syntax, sym, ParseBracedInitializer(syntax));
        }
        if (initializer == NULL) {
          if (TypeContainsAuto(sym->type)) {
            SyntaxError(syntax, "auto variable requires an initializer");
          } else if (sym->flags.is_constexpr || sym->flags.is_constinit) {
            SyntaxError(syntax, sym->flags.is_constinit
                                    ? "constinit variable requires an initializer"
                                    : "constexpr variable requires an initializer");
          } else if (!StorageIs(storage, STO(extern)) &&
                     StorageIs(storage, STO(thread))) {
            initializer = NewCXXThreadLocalGuardedConstructor(syntax, sym);
          } else {
            initializer = NewCXXDefaultConstructorCallIfNeeded(syntax, sym);
          }
        } else if (!StorageIs(storage, STO(extern)) &&
                   StorageIs(sym->storage, STO(thread)) &&
                   initializer->op == AST_OP(call) &&
                   CXXThreadLocalInitializerIsDynamic(initializer)) {
          initializer = NewCXXThreadLocalGuardedInit(syntax, sym, initializer);
        }
        if (!syntax->parsing_template_declaration &&
            TypeIsClassTemplatePlaceholder(sym->type)) {
          SyntaxError(syntax,
                      "Class template argument deduction requires an initializer");
        }
      }

      ASTNode* decl = NewVariableDeclarationASTNode(
          sym, initializer, syntax->lex->current_token_location);
      if (StorageIs(storage, STO(static)) &&
          !StorageIs(storage, STO(thread))) {
        ((VariableDeclarationASTNode*)decl)->local_static_init_kind =
            CompilerIsCXX() ? kLocalStaticInitUnclassified
                            : kLocalStaticInitConstant;
      }
      VectorAppend(declarations, decl);
      if (sym->flags.is_constexpr || sym->flags.is_constinit ||
          (TypeIsConst(sym->type) && !TypeIsStructOrUnion(sym->type)) ||
          // Deduce `auto` eagerly at parse time. Block-scope declarations are
          // otherwise analyzed only after the whole function body is parsed, so
          // a later `decltype(var)` in the same block (resolved during parsing)
          // would still see the undeduced `auto` type. Analyzing here fixes the
          // symbol's type up front; the definition is idempotent under the
          // later body-analysis pass. Skip this inside a template, where the
          // initializer may be dependent and unanalyzable until instantiation
          // (decltype defers via dependent_decltype_expr there anyway).
          (CompilerIsCXX() && TypeContainsAuto(sym->type) &&
           syntax->current_template_parameters == NULL)) {
        SemanticAnalyzeVariableDefinition(syntax,
                                        (VariableDeclarationASTNode*)decl);
      }
      
      if (!StorageIs(storage, STO(extern)) &&
          StorageIs(storage, STO(static) | STO(thread))) {
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
    return SyntaxParseStaticAssert(syntax);
  }
  if (LexLookingAt(syntax->lex, TOK(using))) {
    return ParseUsingDeclaration(syntax);
  }
  if (CompilerIsCXX() && LexLookingAt(syntax->lex, TOK(namespace))) {
    SourceLocation location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    if (!LookingAtNamespaceAliasDefinition(syntax)) {
      SyntaxError(syntax, "namespace definition is not allowed at block scope");
    }
    return ParseNamespaceAliasDefinition(syntax, location);
  }

  Vector* declarations = NewVector();

  Storage storage = STO(implicit);
  bool is_inline = false;
  bool is_constexpr = false;
  bool is_consteval = false;
  bool is_constinit = false;
  bool is_explicit = false;
  TypeRecord* type = NULL;
  Vector attributes = {0};
  syntax->context = kParsingBlockScope;
  
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &is_constexpr,
                            &is_consteval, &is_constinit,
                            &is_explicit, &type, &attributes,
                            kParsingBlockScope);

  if (is_inline) {
    SyntaxError(syntax, "inline is not allowed here");
  }
  if (is_explicit) {
    SyntaxError(syntax, "explicit is not allowed here");
  }
  
  // Parse the type specifier (char, unsigned int, etc.)
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingBlockScope);
  parser.is_constexpr = is_constexpr;
  parser.is_consteval = is_consteval;
  parser.is_constinit = is_constinit;

  // Claim a reference on the freshly built base type for the duration of
  // declarator parsing.  Each declarator that adopts it takes its own
  // reference, so releasing ours afterwards frees the base type when no
  // declarator used it (e.g. a bare `struct S { ... };`).
  TypeRecordIncRef(type);

  // Now we get a sequence of declarations, separated by commas.
  if (!TryParseStructuredBindingDeclaration(syntax, type, storage,
                                            declarations)) {
    ParseLocalDeclarationList(&parser, type, storage, &attributes,
                              declarations);
  }
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

void SyntaxNeedTemplateClose(Syntax* syntax, TokenClass followers) {
  if (!LexConsumeClosingAngle(syntax->lex)) {
    SyntaxError(syntax, "Missing >");
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

// Determines whether the qualified-name at the current position actually names
// a type.  The shallow `starts-a-qualified-name` test treats every `a::b` as a
// potential type, which misclassifies a qualified *value* such as
// `std::nothrow` as a type and, e.g., breaks `new (std::nothrow) T`.  Resolve
// the name: if it denotes a class/enum tag or a typedef it is a type; if it
// denotes a variable/function it is not.  When the name cannot be resolved
// (a dependent name, an incomplete template-id, etc.) fall back to the
// conservative "could be a type" answer so dependent code keeps parsing.
static bool SyntaxQualifiedNameLooksLikeType(Syntax* syntax) {
  if (SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
    return false;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                     TC(openbra) | TC(stmt));
  bool followed_by_assignment = LexLookingAt(syntax->lex, TOK(equal));
  bool decided = false;
  bool is_type = false;
  if (name.is_qualified) {
    if (SyntaxFindQualifiedTag(syntax, &name) != NULL) {
      decided = true;
      is_type = true;
    } else {
      Symbol* symbol = SyntaxFindQualifiedSymbol(syntax, &name);
      if (symbol != NULL) {
        decided = true;
        is_type = StorageIs(symbol->storage, STO(typedef));
      } else if (name.components.length == 2) {
        String* owner_name = name.components.value.p[0];
        String* member_name = name.components.value.p[1];
        Symbol* owner = SyntaxFindTag(syntax, owner_name);
        if (owner != NULL && owner->type != NULL &&
            TypeIsStructOrUnion(owner->type) &&
            owner->type->info.struct_info != NULL) {
          StructMember* member =
              FindStructMember(owner->type->info.struct_info, member_name);
          if (member != NULL && member->symbol != NULL) {
            decided = true;
            is_type = StorageIs(member->symbol->storage, STO(typedef));
          }
        }
      }
    }
  }
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  if (followed_by_assignment) {
    return false;
  }
  if (decided) {
    return is_type;
  }
  return SyntaxCurrentTokenStartsQualifiedName(syntax);
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
    case TOK(wchar_t):
    case TOK(consteval):
    case TOK(constexpr):
    case TOK(constinit):
      return true;
    case TOK(auto):
      return CompilerIsCXX();
    case TOK(decltype):
      return CompilerIsCXX();
    case TOK(typename):
      return CompilerIsCXX();
    case TOK(identifier): {
      if (CompilerIsCXX() &&
          SyntaxIdentifierStartsDaveCCTypeTraitBuiltin(syntax)) {
        LexCheckpoint checkpoint;
        LexCheckpointSave(syntax->lex, &checkpoint);
        LexNextToken(syntax->lex);
        bool is_type_trait = LexLookingAt(syntax->lex, TOK(lparen));
        LexCheckpointRestore(syntax->lex, &checkpoint);
        LexCheckpointDestruct(&checkpoint);
        if (is_type_trait) {
          return true;
        }
      }
      Symbol* sym = SyntaxFindSymbol(syntax, &syntax->lex->spelling);
      if (sym == NULL) {
        if (CompilerIsCXX() &&
            SyntaxFindTag(syntax, &syntax->lex->spelling) != NULL) {
          LexCheckpoint checkpoint;
          LexCheckpointSave(syntax->lex, &checkpoint);
          LexNextToken(syntax->lex);
          bool qualified = LexLookingAt(syntax->lex, TOK(coloncolon));
          LexCheckpointRestore(syntax->lex, &checkpoint);
          LexCheckpointDestruct(&checkpoint);
          if (qualified) {
            if (SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
              return false;
            }
            return SyntaxQualifiedNameLooksLikeType(syntax);
          }
          return true;
        }
        if (SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
          return false;
        }
        return SyntaxQualifiedNameLooksLikeType(syntax);
      }
      if (StorageIs(sym->storage , STO(typedef))) {
        if (CompilerIsCXX() && sym->type != NULL &&
            TypeIsStructOrUnion(sym->type)) {
          LexCheckpoint checkpoint;
          LexCheckpointSave(syntax->lex, &checkpoint);
          LexNextToken(syntax->lex);
          bool qualified = LexLookingAt(syntax->lex, TOK(coloncolon));
          LexCheckpointRestore(syntax->lex, &checkpoint);
          LexCheckpointDestruct(&checkpoint);
          if (qualified) {
            if (SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
              return false;
            }
            return SyntaxQualifiedNameLooksLikeType(syntax);
          }
        }
        return true;
      }
      if (CompilerCXXAtLeast(kLanguageStandardCXX20) && sym->flags.is_concept) {
        LexCheckpoint checkpoint;
        LexCheckpointSave(syntax->lex, &checkpoint);
        LexNextToken(syntax->lex);
        if (LexLookingAt(syntax->lex, TOK(less))) {
          int depth = 0;
          while (!LexEof(syntax->lex)) {
            Token t = syntax->lex->current_token;
            if (t == TOK(less)) {
              depth++;
            } else if (t == TOK(lessless)) {
              depth += 2;
            } else if (t == TOK(greater)) {
              depth--;
            } else if (t == TOK(greatergreater) ||
                       t == TOK(greatergreatereq)) {
              depth -= 2;
            } else if (t == TOK(greatereq)) {
              depth -= 1;
            }
            LexNextToken(syntax->lex);
            if (depth <= 0) {
              break;
            }
          }
        }
        bool constrained_auto = LexLookingAt(syntax->lex, TOK(auto));
        LexCheckpointRestore(syntax->lex, &checkpoint);
        LexCheckpointDestruct(&checkpoint);
        if (constrained_auto) {
          return true;
        }
      }
      return false;
    }
    case TOK(coloncolon): {
      // `::new` and `::delete` are expressions naming the global allocation
      // functions, never the start of a declaration's type.
      LexCheckpoint checkpoint;
      LexCheckpointSave(syntax->lex, &checkpoint);
      LexNextToken(syntax->lex);
      bool new_or_delete = LexLookingAt(syntax->lex, TOK(new)) ||
                           LexLookingAt(syntax->lex, TOK(delete));
      LexCheckpointRestore(syntax->lex, &checkpoint);
      LexCheckpointDestruct(&checkpoint);
      if (new_or_delete) {
        return false;
      }
      return SyntaxQualifiedNameLooksLikeType(syntax);
    }
    default:
      return false;
  }
}

static bool CXXQualifiedNameLooksLikeCallExpression(Syntax* syntax) {
  // Recognize any qualified-name start: a leading `::`, a name that a
  // namespace/template prefix makes look qualified, or a plain `ident::`
  // sequence (e.g. a dependent `T::member` where `T` is a template
  // parameter or a concrete `Class::member`).  Without the plain `ident::`
  // case, a dependent qualified call used as an expression statement would
  // be misclassified as a declaration and send the declaration parser into
  // an infinite loop.
  if (!CompilerIsCXX() ||
      !(SyntaxCurrentTokenStartsQualifiedName(syntax) ||
        SyntaxCurrentIdentifierFollowedByScopeOperator(syntax))) {
    return false;
  }

  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                     TC(openbra) | TC(stmt));
  bool result = name.is_qualified && LexLookingAt(syntax->lex, TOK(lparen));
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return result;
}

bool SyntaxLookingAtDeclaration(Syntax* syntax) {
  if (SyntaxLookingAtCXXAttribute(syntax)) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    Vector attrs = {0};
    VectorInit(&attrs);
    SyntaxParseCXXAttributes(syntax, &attrs);
    AttributeListDestruct(&attrs);
    bool result = SyntaxLookingAtDeclaration(syntax);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return result;
  }
  if (CXXQualifiedNameLooksLikeCallExpression(syntax)) {
    return false;
  }
  switch (syntax->lex->current_token) {
    case TOK(extern):
    case TOK(static):
    case TOK(auto):
    case TOK(register):
    case TOK(thread):
    case TOK(thread_local):
    case TOK(typedef):
    case TOK(using):
    case TOK(namespace):
    case TOK(static_assert):
    case TOK(explicit):
    case TOK(consteval):
    case TOK(constexpr):
    case TOK(constinit):
      return syntax->lex->current_token != TOK(namespace) ||
             (CompilerIsCXX() && syntax->context == kParsingBlockScope);
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
    case TOK(co_return):
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
