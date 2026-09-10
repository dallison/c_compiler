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
#include "typo_correction.h"
#include "type.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_internal.h"
#include "type_special_member.h"
#include "errors.h"
#include "compiler.h"
#include "module_identity.h"
#include "module_syntax.h"
#include "reflection_semantics.h"

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
static bool CXXConstructorSetHasInitializerList(StructMember* ctor);

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

static void MarkExportedDeclaration(Syntax* syntax, Symbol* symbol) {
  if (syntax->export_depth > 0) {
    symbol->flags.is_exported = true;
  }
  SymbolAttachModuleContext(symbol, symbol->storage);
}

static bool InsertFileScopeSymbol(Syntax* syntax, Symbol* symbol) {
  MarkExportedDeclaration(syntax, symbol);
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
  sym->flags.is_explicit_specialization = true;
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
  sym->flags.is_explicit_specialization = true;
}

static bool OverloadTypesEqual(TypeRecord* left, TypeRecord* right);

static bool OverloadParameterTypesEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  TypeRecord left_parameter = *left;
  TypeRecord right_parameter = *right;
  left_parameter.qualifiers &= ~(kQualConst | kQualVolatile);
  right_parameter.qualifiers &= ~(kQualConst | kQualVolatile);
  return OverloadTypesEqual(&left_parameter, &right_parameter);
}

static bool OverloadFunctionPrototypesEqual(FunctionInfo* left,
                                            FunctionInfo* right) {
  if (left->prototype.length != right->prototype.length ||
      left->varargs != right->varargs ||
      left->is_const_member != right->is_const_member ||
      left->is_volatile_member != right->is_volatile_member ||
      left->ref_qualifier != right->ref_qualifier) {
    return false;
  }
  size_t first_parameter = 0;
  if (left->prototype.length != 0) {
    Symbol* left_first = left->prototype.value.p[0];
    Symbol* right_first = right->prototype.value.p[0];
    bool left_has_this =
        left_first != NULL && StringEqual(&left_first->name, "this");
    bool right_has_this =
        right_first != NULL && StringEqual(&right_first->name, "this");
    if (left_has_this != right_has_this) {
      return false;
    }
    if (left_has_this) {
      first_parameter = 1;
    }
  }
  for (size_t i = first_parameter; i < left->prototype.length; i++) {
    Symbol* left_arg = left->prototype.value.p[i];
    Symbol* right_arg = right->prototype.value.p[i];
    if (left_arg == NULL || right_arg == NULL ||
        !OverloadParameterTypesEqual(left_arg->type, right_arg->type)) {
      return false;
    }
  }
  return true;
}

static bool OverloadTypesEqual(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator) {
    return false;
  }
  // Complex arithmetic types use a synthetic struct for storage, but their
  // identity is the element type rather than the per-declaration layout node.
  if (TypeIsComplex(left) || TypeIsComplex(right)) {
    return TypeEqual(left, right);
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
      return TypeArrayBoundsEqual(&left->info.array, &right->info.array) &&
             OverloadTypesEqual(left->next, right->next);
    case kDeclVector:
      return left->info.array.size.fixed == right->info.array.size.fixed &&
             OverloadTypesEqual(left->next, right->next);
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
      return OverloadTypesEqual(left->next, right->next);
    case kDeclMemberPointer:
      return TypeEqual(left, right);
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

static Symbol* FindMatchingOverload(Symbol* first, TypeRecord* type,
                                    Symbol* incoming) {
  for (Symbol* overload = first; overload != NULL;
       overload = overload->overload_next) {
    if (incoming != NULL &&
        !SymbolCompatibleModuleRedeclaration(overload, incoming)) {
      continue;
    }
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

// Two function templates whose signatures are identical are still distinct
// templates if their template parameter lists differ.  That is how the
// pre-C++20 constraint idiom works: the condition lives in the type of a
// defaulted non-type parameter, as in
// `template <class T, typename enable_if<C<T>, int>::type = 0> void f(T);`,
// so the two overloads differ only in that type.
static bool TemplateParameterListsDiffer(Vector* left, Vector* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  if (left->length != right->length) {
    return true;
  }
  for (size_t i = 0; i < left->length; i++) {
    TemplateParameter* left_param = left->value.p[i];
    TemplateParameter* right_param = right->value.p[i];
    if (left_param == NULL || right_param == NULL) {
      continue;
    }
    if (left_param->kind != right_param->kind ||
        left_param->is_parameter_pack != right_param->is_parameter_pack) {
      return true;
    }
    if (left_param->kind == kTemplateParameterNonType &&
        !TypeEqual(left_param->type, right_param->type)) {
      return true;
    }
  }
  return false;
}

// The parameters of the declaration being parsed have not reached its function
// type yet: MoveCurrentTemplateParametersToFunction hands them over later, so
// reading the type here yields an empty list and would make every redeclaration
// of a function template look like a new overload.  Until then they live in
// `syntax->current_template_parameters`.
static Vector* PendingTemplateParameterList(Syntax* syntax, Symbol* symbol) {
  if (symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type) &&
      symbol->type->info.function.template_parameters.length > 0) {
    return &symbol->type->info.function.template_parameters;
  }
  return syntax == NULL ? NULL : syntax->current_template_parameters;
}

static bool FunctionTemplateParameterListsDiffer(Symbol* candidate,
                                                 Vector* overload_params) {
  if (candidate == NULL || candidate->type == NULL ||
      overload_params == NULL || overload_params->length == 0) {
    return false;
  }
  return TemplateParameterListsDiffer(
      &candidate->type->info.function.template_parameters, overload_params);
}

static void AppendClonedConstraint(ConstraintExpr** target,
                                   ConstraintExpr* constraint) {
  if (target == NULL || constraint == NULL) {
    return;
  }
  ConstraintExpr* clone = ConceptsCloneConstraint(constraint);
  if (*target == NULL) {
    *target = clone;
  } else {
    *target = NewConjunctionConstraint(*target, clone, clone->location);
  }
}

static ConstraintExpr* PendingFunctionTemplateConstraint(
    Symbol* symbol, Vector* parameters, ConstraintExpr* trailing_constraint) {
  ConstraintExpr* result = NULL;
  if (symbol != NULL && symbol->type != NULL && TypeIsFunction(symbol->type)) {
    AppendClonedConstraint(
        &result, symbol->type->info.function.associated_constraint);
  }
  if (parameters != NULL) {
    for (size_t i = 0; i < parameters->length; i++) {
      TemplateParameter* param = parameters->value.p[i];
      if (param != NULL) {
        AppendClonedConstraint(&result, param->associated_constraint);
      }
    }
  }
  AppendClonedConstraint(&result, trailing_constraint);
  return result;
}

static bool TryAppendSameSignatureConstrainedTemplateOverload(
    Syntax* syntax, Symbol* first, Symbol* overload,
    ConstraintExpr* pending_constraint) {
  if (first == NULL || overload == NULL ||
      !SymbolLooksLikeFunctionTemplate(overload)) {
    return false;
  }
  Vector* overload_params = PendingTemplateParameterList(syntax, overload);
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
    if (((candidate_constrained || overload_constrained) &&
         !ConceptsFunctionTemplateConstraintsEquivalent(candidate, overload)) ||
        FunctionTemplateParameterListsDiffer(candidate, overload_params)) {
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
         symbol->alias_target != NULL && symbol->overload_next == NULL &&
         depth < 64) {
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
        } else if (current == TOK(lsquare)) {
          square_depth++;
        } else if (current == TOK(rsquare)) {
          square_depth--;
        } else if (current == TOK(lbrace)) {
          brace_depth++;
        } else if (current == TOK(rbrace)) {
          brace_depth--;
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
  if (arg->kind == kTemplateParameterNonType ||
      arg->kind == kTemplateParameterTemplate) {
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
    if (TypeIsArray(t) && t->info.array.is_dependent_bound) {
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
  // `Outer::Inner` names a nested class, not a namespace member.  Namespace
  // lookup above misses it; resolve the prefix as a class and look up Inner
  // among its nested types (stored as typedef members).
  if (symbol == NULL && name->components.length >= 2) {
    Symbol* owner = SyntaxFindQualifiedPrefixSymbolImpl(
        syntax, name, name->components.length - 1,
        /*allow_dependent_template_args=*/true);
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
    }
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
  syntax->parsing_friend_type_specifier = false;
  syntax->expression_nesting_depth = 0;
  syntax->struct_definition_depth = 0;
  syntax->parsing_lambda_body_depth = 0;
  syntax->parsing_consteval_block_depth = 0;
  syntax->parsing_enum_specifier_depth = 0;
  syntax->current_template_parameter_count = 0;
  syntax->current_template_parameters = NULL;
  syntax->current_template_requires_clause = NULL;
  syntax->pending_explicit_condition = NULL;
  syntax->pending_placeholder_variable_constraint = NULL;
  syntax->cxx_class_head = NULL;
  syntax->context = kParsingFileScope;
  syntax->c_linkage = false;
  syntax->explicit_cxx_linkage = false;
  syntax->export_depth = 0;
  syntax->eof_missing_bracket = TOK(eof);
  syntax->eof_expected_semicolon = false;
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
  StructMember* member = FindStructMember(owner, name);
  if (member != NULL &&
      (member->is_static ||
       (member->symbol != NULL &&
        (StorageIs(member->symbol->storage, STO(typedef)) ||
         member->symbol->flags.value_set)))) {
    return member->symbol;
  }
  return NULL;
}

Symbol* SyntaxFindSymbol(Syntax* syntax, String* name) {
  LocalSymbolTable* scope = syntax->local_symbol_stack;
  Symbol* symbol = FindLocalSymbol(scope, name);
  if (symbol != NULL) {
    return FollowAlias(symbol);
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

static void MarkCXX26AutomaticNameIndependent(Symbol* symbol) {
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

static void SymbolSetLocationFromLexIfMissing(Syntax* syntax, Symbol* symbol) {
  if (symbol != NULL && symbol->location == 0 && syntax != NULL &&
      syntax->lex != NULL) {
    symbol->location = syntax->lex->current_token_location;
  }
}

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

bool SyntaxAddBorrowedSymbol(Syntax* syntax, Symbol* symbol) {
  if (syntax == NULL || symbol == NULL || syntax->local_symbol_stack == NULL) {
    return false;
  }
  return InsertLocalSymbol(syntax->local_symbol_stack, symbol);
}

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
  StructMember* member = FindStructMember(owner, name);
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
    TemplateArgument* arg = args->value.p[i];
    // A materialized scalar argument such as `value<42>` has no parameter,
    // pack, dependent metadata, or nontrivial type spine. Its stored shape is
    // already a compact non-dependence summary, so avoid recursive checking.
    TypeRecord* type = arg != NULL ? arg->type : NULL;
    bool plain_type =
        type == NULL ||
        (type->next == NULL && type->declarator == kDeclPrimitive &&
         type->template_parameter_index < 0 &&
         type->template_origin == NULL && type->template_arguments == NULL &&
         type->dependent_member_name == NULL &&
         type->dependent_member_template_arguments == NULL &&
         type->dependent_decltype_expr == NULL &&
         type->dependent_splice_expr == NULL && !type->is_pack_index &&
         type->pack_index_expr == NULL && type->pack_index_pack == NULL &&
         !TypeIsStructOrUnion(type) && !TypeIsEnum(type));
    if (arg != NULL && arg->template_parameter_index < 0 &&
        arg->pack_arguments == NULL && plain_type) {
      continue;
    }
    if (StaticAssertTemplateArgumentContainsTemplateParameter(arg)) {
      return true;
    }
  }
  return false;
}

static bool ExpressionNodeIsTemplateDependent(ASTNode* node, void* data) {
  (void)data;
  if (TypeContainsTemplateParameter(node->type)) {
    return true;
  }
  if ((node->flags & kASTDependentQualifiedName) != 0 ||
      node->op == AST_OP(requires_expr)) {
    return true;
  }
  if (node->op == AST_OP(sizeof) || node->op == AST_OP(alignof)) {
    // `sizeof(T)` / `alignof(T)` on a dependent type is value-dependent even
    // though the node's own type is size_t; the operand type is retained only
    // when dependent (see SizeofASTNode::type_operand).
    SizeofASTNode* s = (SizeofASTNode*)node;
    if (TypeContainsTemplateParameter(s->type_operand)) {
      return true;
    }
  }
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    bool structured_binding_dependent =
        id->symbol != NULL &&
        id->symbol->structured_binding_pack_size >= -1;
    if (structured_binding_dependent &&
        id->symbol->flags.is_parameter_pack &&
        id->symbol->structured_binding_pack_size >= 0 &&
        node->parent != NULL && node->parent->op == AST_OP(sizeof) &&
        ((SizeofASTNode*)node->parent)->is_pack_size) {
      structured_binding_dependent = false;
    }
    if (id->symbol != NULL &&
        (structured_binding_dependent ||
         id->symbol->template_parameter_index >= 0 ||
         TypeContainsTemplateParameter(id->symbol->type) ||
         StaticAssertTemplateArgumentVectorContainsTemplateParameter(
             id->template_arguments))) {
      return true;
    }
  }
  return false;
}

bool ExpressionIsTemplateDependent(ASTNode* expr) {
  return ASTNodeAny(expr, ExpressionNodeIsTemplateDependent, NULL);
}

static bool ExpressionNodeNamesPendingStructuredBinding(ASTNode* node,
                                                        void* data) {
  (void)data;
  return node != NULL && node->op == AST_OP(identifier) &&
         ((IdentifierASTNode*)node)->symbol != NULL &&
         ((IdentifierASTNode*)node)->symbol->structured_binding_pack_size >= -1;
}

static bool ExpressionNodeNamesPendingConstexprObject(ASTNode* node,
                                                      void* data) {
  (void)data;
  if (node == NULL || node->op != AST_OP(identifier)) {
    return false;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol == NULL || symbol->type == NULL) {
    return false;
  }
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      TypeIsStructOrUnion(symbol->type) &&
      StructHasVirtualBases(symbol->type->info.struct_info)) {
    return true;
  }
  return symbol->flags.is_constexpr && !symbol->flags.value_set &&
         (TypeIsStructOrUnion(symbol->type) ||
          TypeIsFixedArray(symbol->type)) &&
         symbol->constexpr_initializer != NULL;
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
  // The operand is interpreted by the constant evaluator, which rejects the
  // body a call is replaced with once it is inlined.
  compiler->constant_evaluation_required_depth++;
  cloned = AnalyzeExpression(cloned);
  compiler->constant_evaluation_required_depth--;
  if (cloned == NULL) {
    ASTNodeDelete(cloned);
    return NULL;
  }
  return cloned;
}

static ASTNode* AnalyzeStaticAssertMessageMemberCall(ASTNode* message_expr,
                                                     const char* member_name) {
  ASTNode* receiver =
      ASTNodeClone(message_expr, IdentityCloneNode, NULL, NULL);
  if (receiver == NULL) {
    return NULL;
  }
  ASTNodeVisit(receiver, ClearStaticAssertExprAnalysis, 0, NULL);
  ASTNode* member = NewStringConstantASTNode(
      NewString(member_name), NULL, message_expr->location);
  ASTNode* access =
      NewBinaryASTNode(AST_OP(dot), NULL, message_expr->location, receiver,
                       member);
  ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, message_expr->location,
                                   access, NewVector());
  compiler->constant_evaluation_required_depth++;
  call = AnalyzeExpression(call);
  compiler->constant_evaluation_required_depth--;
  return call;
}

static ASTNode* ConvertStaticAssertMessageResult(ASTNode* expression,
                                                 TypeRecord* target) {
  if (expression == NULL || target == NULL) {
    ASTNodeDelete(expression);
    TypeRecordDelete(target);
    return NULL;
  }
  ASTNode* wrapper =
      NewExpressionStatementASTNode(expression, expression->location);
  NormalConversion(expression, target);
  expression = ((ExpressionStatementASTNode*)wrapper)->expr;
  bool converted =
      expression != NULL && expression->type != NULL &&
      TypeEqual(expression->type, target);
  expression = expression != NULL ? ASTNodeMove(expression) : NULL;
  ASTNodeDelete(wrapper);
  TypeRecordDelete(target);
  if (!converted) {
    ASTNodeDelete(expression);
    return NULL;
  }
  return expression;
}

bool SyntaxEvaluateStaticAssertMessage(ASTNode* message_expr, String* message) {
  if (message_expr == NULL || message == NULL) {
    return false;
  }
  ASTNode* size_call =
      AnalyzeStaticAssertMessageMemberCall(message_expr, "size");
  size_call =
      ConvertStaticAssertMessageResult(size_call, NewSizeTypeRecord());
  int64_t size = -1;
  bool size_ok = size_call != NULL &&
                 EvaluateIntegerExpression(size_call, &size) && size >= 0;
  ASTNodeDelete(size_call);
  if (!size_ok) {
    SemanticError(
        message_expr,
        "static_assert message size() must be an integral constant expression");
    return false;
  }

  ASTNode* data_call =
      AnalyzeStaticAssertMessageMemberCall(message_expr, "data");
  TypeRecord* char_type =
      NewTypeRecordWithSize(kTypeChar, kQualConst);
  data_call = ConvertStaticAssertMessageResult(
      data_call, NewPointerTo(kQualPlain, char_type));
  bool data_type_ok = data_call != NULL;
  if (!data_type_ok) {
    ASTNodeDelete(data_call);
    SemanticError(message_expr,
                  "static_assert message data() must be convertible to "
                  "'const char*'");
    return false;
  }
  bool data_ok =
      ConstexprEvaluateCharacterSequence(data_call, (size_t)size, message);
  ASTNodeDelete(data_call);
  if (!data_ok) {
    SemanticError(
        message_expr,
        "static_assert message data() must be a constant expression");
    return false;
  }
  return true;
}

ASTNode* SyntaxParseStaticAssert(Syntax* syntax) {
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);  // static_assert
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));

  ASTNode* expr = SyntaxParseSingleExpression(syntax, TC(exprsep));

  String message = {0};
  StringInit(&message, "static assertion failed");
  ASTNode* message_expr = NULL;
  if (LexMatch(syntax->lex, TOK(comma))) {
    if (LexLookingAtStringLiteral(syntax->lex)) {
      LexValidateUnevaluatedString(syntax->lex, "static_assert",
                                   /*allow_user_defined_suffix=*/false);
      StringSetString(&message, &syntax->lex->spelling);
      LexNextToken(syntax->lex);
    } else if (CompilerCXXAtLeast(kLanguageStandardCXX26)) {
      message_expr =
          SyntaxParseSingleExpression(syntax, TC(closebra));
    } else {
      SyntaxError(syntax, "static_assert message must be a string literal");
    }
  } else if (!CompilerIsCXX() &&
             !CompilerCAtLeast(kLanguageStandardC23)) {
    SyntaxError(syntax, "_Static_assert requires a message before C23");
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra));
  SyntaxNeedBracket(syntax, TOK(semicolon), TC(semicolon));

  bool dependent =
      syntax->parsing_lambda_body_depth > 0 ||
      (syntax->current_template_parameter_count > 0 &&
       (ExpressionIsTemplateDependent(expr) ||
        (message_expr != NULL &&
         ExpressionIsTemplateDependent(message_expr)))) ||
      ASTNodeAny(expr, ExpressionNodeNamesPendingStructuredBinding, NULL);
  dependent =
      dependent ||
      ASTNodeAny(expr, ExpressionNodeNamesPendingConstexprObject, NULL);
  if (dependent) {
    ASTNode* node =
        NewStaticAssertASTNode(expr, &message, message_expr, location);
    StringDestruct(&message);
    return node;
  }

  ASTNode* evaluated = EvaluateStaticAssertExpression(expr);
  ASTNodeDelete(expr);
  if (evaluated == NULL) {
    ASTNodeDelete(message_expr);
    StringDestruct(&message);
    SyntaxError(syntax, "static_assert expression is not an integer constant expression");
    return NULL;
  }

  int64_t value = 0;
  if (!EvaluateIntegerExpression(evaluated, &value)) {
    if (ExpressionIsTemplateDependent(evaluated)) {
      ASTNode* node =
          NewStaticAssertASTNode(evaluated, &message, message_expr, location);
      StringDestruct(&message);
      return node;
    }
    ASTNodeDelete(evaluated);
    ASTNodeDelete(message_expr);
    StringDestruct(&message);
    SyntaxError(syntax, "static_assert expression is not an integer constant expression");
    return NULL;
  }

  if (value == 0) {
    if (message_expr == NULL ||
        SyntaxEvaluateStaticAssertMessage(message_expr, &message)) {
      SyntaxError(syntax, "%s", message.value);
    }
  }
  ASTNodeDelete(message_expr);
  StringDestruct(&message);
  ASTNodeDelete(evaluated);
  return NULL;
}

static bool LookingAtContractSpecifier(Syntax* syntax,
                                       ContractAssertionKind* kind) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) ||
      !LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  if (strcmp(syntax->lex->spelling.value, "pre") == 0) {
    *kind = kContractPrecondition;
    return true;
  }
  if (strcmp(syntax->lex->spelling.value, "post") == 0) {
    *kind = kContractPostcondition;
    return true;
  }
  return false;
}

typedef struct {
  Symbol* formal;
  bool used;
} ContractFormalUse;

static void FindContractFormalUse(ASTNode* node, void* data, int child_id,
                                  VisitorMode mode) {
  (void)child_id;
  ContractFormalUse* use = data;
  if (mode == kVisitPreChildren && node->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node)->symbol == use->formal) {
    use->used = true;
  }
}

static void DiagnosePostconditionParameterConst(Syntax* syntax,
                                                TypeRecord* func,
                                                ASTNode* predicate) {
  Vector* formals = &func->info.function.prototype;
  for (size_t i = 0; i < formals->length; i++) {
    Symbol* formal = formals->value.p[i];
    if (formal == NULL || formal->name.length == 0 ||
        StringEqual(&formal->name, "this") ||
        TypeIsReference(formal->type) || TypeIsConst(formal->type)) {
      continue;
    }
    ContractFormalUse use = {
        .formal = formal,
    };
    ASTNodeVisit(predicate, FindContractFormalUse, 0, &use);
    if (use.used) {
      SyntaxError(syntax,
                  "non-reference parameter '%s' used by a postcondition "
                  "must have const type",
                  formal->name.value);
    }
  }
}

static Symbol* ParseContractResultBinding(Syntax* syntax) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    return NULL;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  String name = {0};
  StringInit(&name, syntax->lex->spelling.value);
  SourceLocation location = syntax->lex->current_token_location;
  LexNextToken(syntax->lex);

  Vector attributes = {0};
  VectorInit(&attributes);
  SyntaxParseCXXAttributes(syntax, &attributes);
  if (!LexMatch(syntax->lex, TOK(colon))) {
    AttributeListDestruct(&attributes);
    StringDestruct(&name);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  TypeRecord* unknown =
      NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualConst);
  Symbol* binding = NewSymbol(name.value, unknown, STO(auto));
  binding->location = location;
  binding->flags.is_local = true;
  binding->flags.is_block_scope = true;
  AttributeListDestruct(&binding->attributes);
  binding->attributes = attributes;
  StringDestruct(&name);
  return binding;
}

void SyntaxParseFunctionContracts(Syntax* syntax, TypeRecord* func,
                                  Struct* member_owner,
                                  bool add_implicit_this) {
  if (func == NULL || !TypeIsFunction(func)) {
    return;
  }
  ContractAssertionKind kind;
  if (LookingAtContractSpecifier(syntax, &kind) &&
      add_implicit_this && member_owner != NULL &&
      !func->info.function.has_explicit_object_parameter &&
      !FunctionHasImplicitThisParameter(func)) {
    TypeRecordAddCXXThisParameter(func, member_owner,
                                  syntax->lex->current_token_location);
  }
  while (LookingAtContractSpecifier(syntax, &kind)) {
    SourceLocation location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);

    Vector attributes = {0};
    VectorInit(&attributes);
    SyntaxParseCXXAttributes(syntax, &attributes);
    SyntaxNeedBracket(syntax, TOK(lparen), TC(closebra));

    SyntaxOpenScope(syntax);
    if (member_owner != NULL) {
      SyntaxInsertClassMembersForConstraint(syntax, member_owner);
    }
    for (size_t i = 0; i < func->info.function.prototype.length; i++) {
      Symbol* formal = func->info.function.prototype.value.p[i];
      if (formal != NULL && formal->name.length > 0) {
        InsertLocalSymbol(syntax->local_symbol_stack, formal);
      }
    }

    Symbol* result_binding = NULL;
    if (kind == kContractPostcondition) {
      result_binding = ParseContractResultBinding(syntax);
      if (result_binding != NULL &&
          !InsertLocalSymbol(syntax->local_symbol_stack, result_binding)) {
        SyntaxError(syntax,
                    "postcondition result name '%s' conflicts with a parameter",
                    result_binding->name.value);
      }
    }
    ASTNode* predicate =
        SyntaxParseSingleExpression(syntax, TC(closebra));
    SyntaxCloseScope(syntax);
    SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra));

    if (predicate == NULL) {
      SymbolDelete(result_binding);
      AttributeListDestruct(&attributes);
      continue;
    }
    if (kind == kContractPostcondition) {
      DiagnosePostconditionParameterConst(syntax, func, predicate);
    }
    VectorAppend(&func->info.function.contract_assertions,
                 NewContractAssertion(kind, predicate, result_binding,
                                      &attributes, location));
  }
}

void SyntaxDiagnoseInvalidFunctionContracts(Syntax* syntax, TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.contract_assertions.length == 0) {
    return;
  }
  FunctionInfo* info = &func->info.function;
  if ((info->is_virtual || info->is_override) &&
      !CompilerCXXAtLeast(kLanguageStandardCXX29)) {
    SyntaxError(syntax, "virtual functions cannot have contract assertions");
  }
  if (info->is_deleted || info->is_explicitly_deleted) {
    SyntaxError(syntax, "deleted functions cannot have contract assertions");
  }
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

typedef struct {
  uint64_t value;
  ContractAssertion* assertion;
} ContractExpressionHash;

static void HashContractBytes(ContractExpressionHash* hash,
                              const void* bytes, size_t length) {
  const unsigned char* p = bytes;
  for (size_t i = 0; i < length; i++) {
    hash->value ^= p[i];
    hash->value *= UINT64_C(1099511628211);
  }
}

static void HashContractString(ContractExpressionHash* hash,
                               const String* string) {
  if (string != NULL) {
    HashContractBytes(hash, string->value, string->length);
  }
}

static void HashContractExpressionNode(ASTNode* node, void* data,
                                       int child_id, VisitorMode mode) {
  if (mode != kVisitPreChildren) {
    return;
  }
  ContractExpressionHash* hash = data;
  HashContractBytes(hash, &node->op, sizeof(node->op));
  HashContractBytes(hash, &child_id, sizeof(child_id));
  if (node->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
    if (symbol == hash->assertion->result_binding) {
      const unsigned char result_marker = 0xf1;
      HashContractBytes(hash, &result_marker, sizeof(result_marker));
    } else if (symbol != NULL && symbol->flags.is_argument) {
      HashContractBytes(hash, &symbol->value.arg_number,
                        sizeof(symbol->value.arg_number));
    } else if (symbol != NULL) {
      HashContractString(hash, &symbol->name);
    }
  } else if (ASTNodeIsIntConstant(node)) {
    ConstantASTNode* constant = (ConstantASTNode*)node;
    HashContractBytes(hash, &constant->value.ivalue,
                      sizeof(constant->value.ivalue));
  } else if (node->op == AST_OP(fnumber)) {
    ConstantASTNode* constant = (ConstantASTNode*)node;
    HashContractBytes(hash, &constant->value.fvalue,
                      sizeof(constant->value.fvalue));
  } else if (node->op == AST_OP(string) ||
             node->op == AST_OP(string_wide)) {
    HashContractString(hash, ((ConstantASTNode*)node)->value.string);
  } else if (node->op == AST_OP(structmember)) {
    StructMember* member = ((StructMemberASTNode*)node)->member;
    if (member != NULL && member->symbol != NULL) {
      HashContractString(hash, &member->symbol->name);
    }
  }
}

static uint64_t ContractExpressionFingerprint(ContractAssertion* assertion) {
  ContractExpressionHash hash = {
      .value = UINT64_C(1469598103934665603),
      .assertion = assertion,
  };
  ASTNodeVisit(assertion->predicate, HashContractExpressionNode, 0, &hash);
  return hash.value;
}

static void DiagnoseOmittedPostconditionParameterConst(
    Syntax* syntax, TypeRecord* old_func, TypeRecord* new_func) {
  Vector* old_formals = &old_func->info.function.prototype;
  Vector* new_formals = &new_func->info.function.prototype;
  Vector* assertions = &old_func->info.function.contract_assertions;
  for (size_t i = 0; i < old_formals->length && i < new_formals->length; i++) {
    Symbol* old_formal = old_formals->value.p[i];
    Symbol* new_formal = new_formals->value.p[i];
    if (old_formal == NULL || new_formal == NULL ||
        old_formal->name.length == 0 ||
        StringEqual(&old_formal->name, "this") ||
        TypeIsReference(new_formal->type) ||
        TypeIsConst(new_formal->type)) {
      continue;
    }
    for (size_t j = 0; j < assertions->length; j++) {
      ContractAssertion* assertion = assertions->value.p[j];
      if (assertion->kind != kContractPostcondition) {
        continue;
      }
      ContractFormalUse use = {
          .formal = old_formal,
      };
      ASTNodeVisit(assertion->predicate, FindContractFormalUse, 0, &use);
      if (use.used) {
        SyntaxError(syntax,
                    "non-reference parameter '%s' used by a postcondition "
                    "must have const type",
                    new_formal->name.value);
        break;
      }
    }
  }
}

static void MergeCXXContractAssertions(Syntax* syntax, Symbol* old_sym,
                                       Symbol* new_sym) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) || old_sym == NULL ||
      new_sym == NULL || !TypeIsFunction(old_sym->type) ||
      !TypeIsFunction(new_sym->type)) {
    return;
  }
  Vector* old_parameters = &old_sym->type->info.function.prototype;
  Vector* new_parameters = &new_sym->type->info.function.prototype;
  for (size_t i = 0;
       i < old_parameters->length && i < new_parameters->length; i++) {
    Symbol* old_parameter = old_parameters->value.p[i];
    Symbol* new_parameter = new_parameters->value.p[i];
    if (old_parameter == NULL || new_parameter == NULL) {
      continue;
    }
    bool first_is_indeterminate =
        SymbolHasAttribute(old_parameter, "indeterminate");
    bool redeclaration_is_indeterminate =
        SymbolHasAttribute(new_parameter, "indeterminate");
    if (!first_is_indeterminate && redeclaration_is_indeterminate) {
      SyntaxError(syntax,
                  "'indeterminate' on a parameter must appear on the first "
                  "declaration of '%s'",
                  new_sym->name.value);
    } else if (first_is_indeterminate &&
               !redeclaration_is_indeterminate) {
      SymbolAddAttribute(new_parameter, NewAttribute("indeterminate"));
    }
  }
  Vector* old_assertions =
      &old_sym->type->info.function.contract_assertions;
  Vector* new_assertions =
      &new_sym->type->info.function.contract_assertions;
  // A redeclaration may omit the function-contract-specifier-seq.  When it
  // supplies one, however, it must correspond to the sequence on the reachable
  // first declaration.
  if (new_assertions->length == 0) {
    DiagnoseOmittedPostconditionParameterConst(
        syntax, old_sym->type, new_sym->type);
    TypeRecordCopyContractAssertions(new_sym->type, old_sym->type);
    return;
  }
  if (old_assertions->length != new_assertions->length) {
    SyntaxError(syntax,
                "redeclarations of '%s' have different contract assertions",
                new_sym->name.value);
    return;
  }
  for (size_t i = 0; i < old_assertions->length; i++) {
    ContractAssertion* old_assertion = old_assertions->value.p[i];
    ContractAssertion* new_assertion = new_assertions->value.p[i];
    if (old_assertion->kind != new_assertion->kind ||
        (old_assertion->result_binding != NULL) !=
            (new_assertion->result_binding != NULL) ||
        ContractExpressionFingerprint(old_assertion) !=
            ContractExpressionFingerprint(new_assertion)) {
      SyntaxError(syntax,
                  "redeclarations of '%s' have non-corresponding "
                  "contract assertions",
                  new_sym->name.value);
      return;
    }
  }
}

// A designator index is kept in an int, and the size of the array it
// designates is an int as well.  A value that does not fit would be truncated
// into an unrelated index, so it is rejected here and the initializer it
// carries is attached to element zero to keep parsing.
static int DesignatorArrayIndex(Syntax* syntax, int64_t value) {
  if (value < INT_MIN || value > INT_MAX) {
    SyntaxError(syntax, "Array designator index %lld is out of range",
                (long long)value);
    return 0;
  }
  return (int)value;
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
      int index = DesignatorArrayIndex(syntax, value);
      Designator* d = NewArrayDesignator(NULL, index);
      d->array_index_end =
          end_value == value ? index : DesignatorArrayIndex(syntax, end_value);
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
  // This is followed by either `= initializer-clause` or a direct braced
  // initializer.  C++ designated initializers permit both `.member = value`
  // and `.member { value }`.
  bool has_equal = LexMatch(syntax->lex, TOK(equal));
  bool has_direct_braces =
      !has_equal && LexMatch(syntax->lex, TOK(lbrace));
  if (!has_equal && !has_direct_braces) {
    SyntaxError(syntax, "Expected = or { in designated initializer");
    SyntaxRecover(syntax, TC(exprsep));
  } else {
    ASTNode* init;
    // The initialization expression is either a brace-enclosed
    // initializer or a single expression.
    if (has_direct_braces || LexMatch(syntax->lex, TOK(lbrace))) {
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
      // Expanding a [start ... end] range clones the initializer once per
      // index.  A range that would materialize millions of AST nodes is
      // rejected the same way a single huge designator used to be: diagnose
      // and keep a single slot so parsing can continue.
      if (end > start && (int64_t)end - (int64_t)start >= 65536) {
        SyntaxError(syntax,
                    "Array designator range [%d ... %d] is larger than this "
                    "compiler can lay out",
                    start, end);
        end = start;
      }
      for (int v = start; v <= end; v++) {
        Vector* desigs = CloneDesignators(designators, range_pos, v);
        ASTNode* init_for_index = (v == end) ? init : CloneInitializer(init);
        ASTNode* designated = NewDesignatedInitializerASTNode(
            desigs, init_for_index, location);
        designated->flags |= kASTSourceDesignatedInitializer;
        VectorAppend(initializers, designated);
      }
      VectorDelete(designators);
    } else {
      ASTNode* designated =
          NewDesignatedInitializerASTNode(designators, init, location);
      designated->flags |= kASTSourceDesignatedInitializer;
      VectorAppend(initializers, designated);
    }
  }

}

typedef struct {
  bool found;
} CXXPackExpressionSearch;

static bool CXXTemplateArgumentReferencesParameterPack(
    TemplateArgument* argument) {
  if (argument == NULL) {
    return false;
  }
  if (argument->references_parameter_pack) {
    return true;
  }
  for (size_t i = 0;
       argument->pack_arguments != NULL &&
       i < argument->pack_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            argument->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

static void FindCXXParameterPackExpression(ASTNode* node, void* data,
                                           int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  if ((node->flags & kASTReferencesParameterPack) != 0) {
    ((CXXPackExpressionSearch*)data)->found = true;
    return;
  }
  Vector* template_arguments = NULL;
  ASTNodeShape shape = ASTNodeGetShape(node);
  if (shape == kASTShapeIdentifier) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    if (id->symbol != NULL && id->symbol->flags.is_parameter_pack) {
      ((CXXPackExpressionSearch*)data)->found = true;
      return;
    }
    template_arguments = id->template_arguments;
  } else if (shape == kASTShapeStructMember) {
    template_arguments =
        ((StructMemberASTNode*)node)->template_arguments;
  } else if (shape == kASTShapeConstant) {
    template_arguments = ((ConstantASTNode*)node)->template_arguments;
  }
  for (size_t i = 0;
       template_arguments != NULL && i < template_arguments->length; i++) {
    if (CXXTemplateArgumentReferencesParameterPack(
            template_arguments->value.p[i])) {
      ((CXXPackExpressionSearch*)data)->found = true;
      return;
    }
  }
}

static bool CXXExpressionContainsParameterPack(ASTNode* node) {
  CXXPackExpressionSearch search = {0};
  ASTNodeVisit(node, FindCXXParameterPackExpression, 0, &search);
  return search.found;
}

static bool CXXTypeContainsParameterPack(Syntax* syntax, TypeRecord* type) {
  for (TypeRecord* current = type; current != NULL;
       current = current->next) {
    int parameter_index = -1;
    TypeIsTemplateParameterPlaceholder(current, &parameter_index);
    if (parameter_index < 0 && current->dependent_member_name != NULL) {
      parameter_index = current->template_parameter_index;
    }
    for (size_t i = 0;
         parameter_index >= 0 &&
         syntax->current_template_parameters != NULL &&
         i < syntax->current_template_parameters->length; i++) {
      TemplateParameter* parameter =
          syntax->current_template_parameters->value.p[i];
      if (parameter != NULL && parameter->index == parameter_index &&
          parameter->is_parameter_pack) {
        return true;
      }
    }
    Vector* function_parameters =
        compiler->current_function != NULL &&
                TypeIsFunction(compiler->current_function)
            ? &compiler->current_function->info.function.template_parameters
            : NULL;
    for (size_t i = 0;
         parameter_index >= 0 && function_parameters != NULL &&
         i < function_parameters->length; i++) {
      TemplateParameter* parameter = function_parameters->value.p[i];
      if (parameter != NULL && parameter->index == parameter_index &&
          parameter->is_parameter_pack) {
        return true;
      }
    }
    if (current->dependent_decltype_expr != NULL &&
        CXXExpressionContainsParameterPack(
            current->dependent_decltype_expr)) {
      return true;
    }
    for (size_t i = 0;
         current->template_arguments != NULL &&
         i < current->template_arguments->length; i++) {
      if (CXXTemplateArgumentReferencesParameterPack(
              current->template_arguments->value.p[i])) {
        return true;
      }
    }
  }
  return false;
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
    "constructor", "destructor", "meta_intrinsic", "indeterminate",
    // Accepted but not modelled (parsed cleanly, no effect).
    "stdcall", "cdecl", "fastcall", "thiscall", "regparm", "ms_abi",
    "sysv_abi", "may_alias", "gnu_inline", "nothrow", "leaf", "cold", "hot",
    "malloc", "pure", "const", "nonnull", "returns_nonnull", "sentinel",
    "weak", "alias", "section", "visibility", "used",
    "transparent_union", "vector_size", "ext_vector_type", "mode",
    "no_instrument_function", "cleanup", "returns_twice", "artificial",
    "designated_init", "fallthrough", "warning", "error", "alloc_size",
    "format_arg", "nonstring", "noclone", "noipa", "flatten", "naked",
    "weakref", "dllimport", "dllexport", "common", "nocommon", "tls_model",
    "selectany", "novtable", "uuid", "property", "allocate", "noalias",
    "aligned_alloc", "assume_aligned", "likely", "unlikely",
    "no_unique_address", "reproducible", "unsequenced",
  };
  for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
    if (strcmp(known[i], name) == 0) {
      return true;
    }
  }
  return false;
}

bool SyntaxAttributeIsSupported(const char* name) {
  static const char* supported[] = {
      "packed",          "aligned",          "format",
      "deprecated",      "unused",           "warn_unused_result",
      "noreturn",        "noinline",         "always_inline",
      "constructor",     "destructor",        "indeterminate",
      "vector_size",     "ext_vector_type",
  };
  for (size_t i = 0; i < sizeof(supported) / sizeof(supported[0]); i++) {
    if (strcmp(supported[i], name) == 0) {
      return true;
    }
  }
  return false;
}

static bool IsUserDefinedAttributeNamespace(const char* namespace_name) {
  return namespace_name != NULL && namespace_name[0] != '\0' &&
         strcmp(namespace_name, "gnu") != 0 &&
         strcmp(namespace_name, "std") != 0;
}

bool SyntaxAttributeIsReflectable(const Attribute* attr) {
  if (attr == NULL) {
    return false;
  }
  const char* token = AttributeIdentifier(attr);
  const char* namespace_name = attr->attribute_namespace.value;
  if (token == NULL || strcmp(token, "assume") == 0) {
    return false;
  }
  if (namespace_name == NULL || attr->attribute_namespace.length == 0) {
    static const char* standard[] = {
        "carries_dependency", "deprecated",        "fallthrough",
        "indeterminate",      "likely",            "maybe_unused",
        "nodiscard",          "noreturn",          "no_unique_address",
        "reproducible",       "unlikely",          "unsequenced",
    };
    for (size_t i = 0; i < sizeof(standard) / sizeof(standard[0]); i++) {
      if (strcmp(token, standard[i]) == 0) {
        return true;
      }
    }
    return false;
  }
  // P3385 permits implementation-defined support for vendor attributes.
  // DaveCC reflects the GNU attributes its parser already understands and
  // arbitrary namespaced user attributes as identity-only metadata.
  if (strcmp(namespace_name, "gnu") == 0) {
    return IsKnownAttribute(attr->name.value);
  }
  return IsUserDefinedAttributeNamespace(namespace_name);
}

static bool IsUnsupportedTypeAttribute(const char* name) {
  return strcmp(name, "mode") == 0;
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
    if (IsUnsupportedTypeAttribute(attr->name.value)) {
      SyntaxError(syntax, "'%s' type attribute is not supported",
                  attr->name.value);
    } else if (!IsKnownAttribute(attr->name.value)) {
      SyntaxWarning(syntax, "attributes", "'%s' attribute directive ignored",
                    attr->name.value);
    }
  }
  SyntaxNeedBracket(syntax, TOK(rparen), 0);
  StringDestruct(&attribute_list);
}

bool SyntaxLookingAtCXXAttribute(Syntax* syntax) {
  if ((!CompilerIsCXX() && !CompilerCAtLeast(kLanguageStandardC23)) ||
      !LexLookingAt(syntax->lex, TOK(lsquare))) {
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

// Parse `__declspec(name)` or `__declspec(name(args))`.  MSVC allows one
// attribute per `__declspec`; stack them for several.  `align(N)` is accepted
// as GNU `aligned(N)` so existing layout code applies.
static void SyntaxParseDeclspec(Syntax* syntax, Vector* attrs) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    SyntaxError(syntax, "Expected ( after __declspec");
    return;
  }
  String name = {0};
  if (!LexMatchIdentifier(syntax->lex, &name)) {
    SyntaxError(syntax, "Expected identifier in __declspec");
    int depth = 1;
    while (!LexEof(syntax->lex) && depth > 0) {
      if (LexMatch(syntax->lex, TOK(lparen))) {
        depth++;
      } else if (LexMatch(syntax->lex, TOK(rparen))) {
        depth--;
      } else {
        LexNextToken(syntax->lex);
      }
    }
    StringDestruct(&name);
    return;
  }
  const char* mapped = name.value;
  if (strcmp(mapped, "align") == 0) {
    mapped = "aligned";
  }
  Attribute* attr = NewAttribute(mapped);
  if (LexMatch(syntax->lex, TOK(lparen))) {
    int depth = 1;
    String arg = {0};
    StringInit(&arg, NULL);
    while (!LexEof(syntax->lex) && depth > 0) {
      if (LexLookingAt(syntax->lex, TOK(rparen))) {
        depth--;
        if (depth == 0) {
          AttributeAppendArg(attr, arg.value,
                             arg.value != NULL ? arg.value + arg.length
                                               : arg.value);
          break;
        }
      }
      if (depth == 1 && LexLookingAt(syntax->lex, TOK(comma))) {
        AttributeAppendArg(attr, arg.value,
                           arg.value != NULL ? arg.value + arg.length
                                             : arg.value);
        StringDestruct(&arg);
        StringInit(&arg, NULL);
        LexNextToken(syntax->lex);
        continue;
      }
      if (LexLookingAt(syntax->lex, TOK(lparen))) {
        depth++;
      }
      AppendCXXAttributeArgToken(syntax->lex, &arg);
      LexNextToken(syntax->lex);
    }
    SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra) | TC(semicolon));
    StringDestruct(&arg);
  }
  VectorAppend(attrs, attr);
  if (IsUnsupportedTypeAttribute(attr->name.value)) {
    SyntaxError(syntax, "'%s' type attribute is not supported",
                attr->name.value);
  } else if (!IsKnownAttribute(attr->name.value)) {
    SyntaxWarning(syntax, "attributes", "'%s' attribute directive ignored",
                  attr->name.value);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(closebra) | TC(semicolon));
  StringDestruct(&name);
}

bool SyntaxLookingAtAnyAttribute(Syntax* syntax) {
  return LexLookingAt(syntax->lex, TOK(attribute)) ||
         LexLookingAt(syntax->lex, TOK(declspec)) ||
         SyntaxLookingAtCXXAttribute(syntax);
}

bool SyntaxParseAnyAttribute(Syntax* syntax, Vector* attrs) {
  if (LexMatch(syntax->lex, TOK(attribute))) {
    SyntaxParseAttribute(syntax, attrs);
    return true;
  }
  if (LexMatch(syntax->lex, TOK(declspec))) {
    SyntaxParseDeclspec(syntax, attrs);
    return true;
  }
  if (SyntaxLookingAtCXXAttribute(syntax)) {
    SyntaxParseCXXAttributes(syntax, attrs);
    return true;
  }
  return false;
}

static void ParseCXXAttributeArguments(Syntax* syntax, Attribute* attr,
                                       bool unevaluated_string_argument) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return;
  }
  if (unevaluated_string_argument) {
    if (LexLookingAtStringLiteral(syntax->lex)) {
      LexValidateUnevaluatedString(syntax->lex, "attribute argument",
                                   /*allow_user_defined_suffix=*/false);
    } else {
      SyntaxError(syntax,
                  "attribute argument must be an unevaluated string");
    }
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
  if (strcmp(name, "__noreturn__") == 0) {
    return "noreturn";
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
  AttributeSetCXXIdentity(attr, namespace_name, attr_name);
  bool unevaluated_string_argument =
      namespace_name == NULL &&
      (strcmp(attr_name, "deprecated") == 0 ||
       strcmp(attr_name, "nodiscard") == 0);
  ParseCXXAttributeArguments(syntax, attr, unevaluated_string_argument);
  if (StringEqual(&attr->name, "aligned") && AttributeArgCount(attr) > 0) {
    long ignored = 0;
    if (!AttributeArgInt(attr, 0, &ignored)) {
      SyntaxError(syntax, "aligned attribute argument must be an integer");
    }
  }
  if (IsUnsupportedTypeAttribute(attr->name.value)) {
    SyntaxError(syntax, "'%s' type attribute is not supported",
                attr->name.value);
  } else if (!IsKnownAttribute(attr->name.value) &&
             !(CompilerCXXAtLeast(kLanguageStandardCXX29) &&
               IsUserDefinedAttributeNamespace(namespace_name))) {
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
      if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
          LexLookingAt(syntax->lex, TOK(equal))) {
        LexNextToken(syntax->lex);
        Attribute* attr = NewAttribute("annotation");
        attr->annotation_expr =
            SyntaxParseExpression(syntax, TC(closebra));
        VectorAppend(attrs, attr);
      } else {
        ParseCXXSingleAttribute(syntax, attrs, using_namespace);
      }
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
  if ((!CompilerIsCXX() && !CompilerCAtLeast(kLanguageStandardC11)) ||
      !LexMatch(syntax->lex, TOK(alignas))) {
    return false;
  }

  SyntaxNeedBracket(syntax, TOK(lparen), TC(decl) | TC(closebra));
  int64_t value = 0;
  Attribute* dependent = NULL;
  if (SyntaxLookingAtType(syntax)) {
    TypeParser parser;
    TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                   syntax->context);
    TypeRecord* type = TypeParserParseType(&parser, true);
    Symbol* sym = TypeParserParseDeclarator(&parser, type);
    if (sym == NULL || sym->type == NULL) {
      SyntaxError(syntax, "alignas type-id is invalid");
    } else if (TypeContainsTemplateParameter(sym->type)) {
      dependent = NewAttribute("aligned");
      dependent->dependent_alignas_type = TypeRecordCopy(sym->type);
    } else {
      TypeRecordCalculateSize(sym->type);
      value = TypeRecordAlignment(sym->type);
    }
    SymbolDelete(sym);
    TypeParserDestruct(&parser);
  } else {
    ASTNode* expr = SyntaxParseExpression(syntax, TC(closebra));
    if (syntax->parsing_template_declaration &&
        ExpressionIsTemplateDependent(expr)) {
      dependent = NewAttribute("aligned");
      dependent->dependent_alignas_expr = expr;
      expr = NULL;
    } else {
      expr = AnalyzeExpression(expr);
      bool ok = EvaluateIntegerExpression(expr, &value);
      if (!ok) {
        SyntaxError(syntax, "alignas specifier must be a constant expression");
      }
    }
    ASTNodeDelete(expr);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));

  if (dependent != NULL) {
    VectorAppend(attrs, dependent);
    return true;
  }
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
static void ApplyVectorTypeAttribute(Syntax* syntax, Symbol* sym) {
  Attribute* vector_size =
      AttributeListFind(&sym->attributes, "vector_size");
  Attribute* ext_vector =
      AttributeListFind(&sym->attributes, "ext_vector_type");
  Attribute* attr = vector_size != NULL ? vector_size : ext_vector;
  if (attr == NULL) {
    return;
  }
  if (vector_size != NULL && ext_vector != NULL) {
    SyntaxError(syntax,
                "'vector_size' and 'ext_vector_type' cannot be combined");
    return;
  }
  if (AttributeArgCount(attr) != 1) {
    SyntaxError(syntax, "'%s' attribute requires one argument",
                attr->name.value);
    return;
  }
  long argument = 0;
  if (!AttributeArgInt(attr, 0, &argument) || argument <= 0) {
    SyntaxError(syntax, "'%s' attribute argument must be a positive integer",
                attr->name.value);
    return;
  }
  TypeRecordCalculateSize(sym->type);
  TypeRecord* element = sym->type;
  if (TypeIsVector(element) || !TypeIsPrimitive(element) ||
      (!TypeIsIntegral(element) && !TypeIsFloatingPoint(element)) ||
      TypeIsBool(element) || TypeIsEnum(element) || TypeIsBitInt(element)) {
    SyntaxError(syntax,
                "'%s' attribute requires an integer or floating scalar type",
                attr->name.value);
    return;
  }
  long lanes = argument;
  if (vector_size != NULL) {
    if (element->size <= 0 || argument % element->size != 0) {
      SyntaxError(syntax,
                  "vector size must be a positive multiple of element size");
      return;
    }
    lanes = argument / element->size;
  }
  long bytes = lanes * element->size;
  if (lanes <= 0 || lanes > INT_MAX || bytes <= 0 ||
      (bytes & (bytes - 1)) != 0) {
    SyntaxError(syntax, "vector size must be a power of two");
    return;
  }
  SymbolSetType(sym, NewVectorTypeRecord(element, (int)lanes));
}

void SyntaxApplyDeclarationAttributes(Syntax* syntax, Symbol* sym) {
  if (sym == NULL) {
    return;
  }

  ApplyVectorTypeAttribute(syntax, sym);

  // Function-behavior flags (also meaningful on forward declarations).
  if (AttributeListHas(&sym->attributes, "noreturn")) {
    if (!TypeIsFunction(sym->type) ||
        StorageIs(sym->storage, STO(typedef))) {
      SyntaxError(syntax, "'noreturn' attribute applies only to functions");
    }
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
  Attribute* indeterminate =
      AttributeListFind(&sym->attributes, "indeterminate");
  if (indeterminate != NULL) {
    if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
      SyntaxError(syntax, "'indeterminate' attribute requires C++26");
    }
    if (AttributeArgCount(indeterminate) != 0) {
      SyntaxError(syntax, "'indeterminate' attribute takes no arguments");
    }
    bool automatic_block_variable =
        sym->flags.is_block_scope &&
            (sym->storage == STO(implicit) ||
             StorageIs(sym->storage, STO(auto)) ||
         StorageIs(sym->storage, STO(register)));
    if (!sym->flags.is_argument && !automatic_block_variable) {
      SyntaxError(syntax,
                  "'indeterminate' attribute applies only to parameters and "
                  "automatic block variables");
    }
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
    formal->flags.is_block_scope = true;
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
static bool CXXTypeHasDefaultConstructor(TypeRecord* type);
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

// Builds an lvalue expression naming the base subobject of `source` (the
// `other` reference parameter of a defaulted copy/move special member) as
// `*(B*)((char*)&source + byte_offset)`.  The nodes are emitted pre-analyzed
// with forced types so the later analysis pass does not re-run a derived-to-base
// conversion on them.  This is important because that conversion is access
// checked and would reject a *private/protected* base (e.g.
// `struct D : private B {}`), even though a class may always access its own base
// subobjects from within its special members.  Using the raw offset mirrors how
// the `this` receiver already reaches the base subobject and sidesteps the check
// entirely.
static ASTNode* NewCXXSourceBaseSubobject(Symbol* source,
                                          CXXBaseSpecifier* base,
                                          SourceLocation location) {
  if (source == NULL || source->type == NULL || base == NULL ||
      base->type == NULL) {
    return NULL;
  }
  if (!TypeIsReference(source->type)) {
    return NULL;
  }
  TypeRecord* referent = source->type->next;
  TypeRecord* source_pointer =
      NewPointerTo(kQualPlain, TypeRecordCopy(referent));
  // Reading the reference parameter yields the underlying pointer to the
  // referent (references are lowered to pointers), so treat the identifier as a
  // `owner*` directly -- exactly as the `this` pointer is used for the receiver.
  // kASTNeedAddress tells codegen to load the pointer held by the reference
  // parameter without then loading the referenced object.  The latter would
  // pass the source object's first word as the base address.
  ASTNode* source_ptr = NewIdentifierASTNode(source, location);
  source_ptr->flags |= kASTAnalyzed | kASTNeedAddress;
  ASTNodeSetType(source_ptr, source_pointer);
  TypeRecord* base_pointer =
      NewPointerTo(kQualPlain, TypeRecordCopy(base->type));
  ASTNode* offset = NewIntConstantASTNode(
      base->byte_offset, NewTypeRecordWithSize(kTypeInt, kQualPlain), location);
  ASTNode* base_ptr =
      NewBinaryASTNode(AST_OP(plus), base_pointer, location, source_ptr, offset);
  base_ptr->flags |= kASTAnalyzed;
  ASTNodeSetType(base_ptr, base_pointer);
  ASTNode* base_ref = NewUnaryASTNode(AST_OP(contents),
                                      TypeRecordCopy(base->type), location,
                                      base_ptr);
  base_ref->flags |= kASTAnalyzed;
  base_ref->value_category =
      source->type->declarator == kDeclRValueReference
          ? kValueCategoryXvalue
          : kValueCategoryLvalue;
  ASTNodeSetType(base_ref, TypeRecordCopy(base->type));
  return base_ref;
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
  TypeRecord* base_pointer =
      NewPointerTo(kQualPlain, TypeRecordCopy(base->type));
  if (base->byte_offset == 0 && !base->is_virtual) {
    // Member lookup must start from the base subobject even when no address
    // adjustment is required. Virtual-base special-member calls retain the
    // complete-object receiver used by their hidden construction/destruction
    // protocol.
    receiver->flags |= kASTAnalyzed;
    ASTNodeSetType(receiver, base_pointer);
  } else if (base->byte_offset != 0) {
    ASTNode* offset =
        NewIntConstantASTNode(base->byte_offset,
                              NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location);
    receiver = NewBinaryASTNode(AST_OP(plus), base_pointer, location,
                                receiver, offset);
    // base->byte_offset is already measured in bytes.  Preserve this forced
    // adjustment when a template special member body is cloned; re-analyzing
    // it as ordinary pointer arithmetic would scale the offset by sizeof(*this).
    receiver->flags |= kASTAnalyzed | kASTForcedTypeAdjustment;
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

// Emits the base-subobject copy/move assignments for a defaulted copy/move
// assignment operator.  The memberwise-copy helper only assigns the class's own
// members; a derived class must additionally forward to each direct base's
// assignment operator so the base subobject is assigned rather than left
// untouched.  The base assignment is expressed as
// `(B*)(this + offset)->operator=(*(B*)(&other + offset))`: reinterpreting
// `this` as `B*` (via a type-forced, already-analyzed pointer adjustment) makes
// name lookup resolve the *base's* operator= instead of the enclosing class's,
// and the argument is the source's base subobject extracted by the same offset
// trick (see NewCXXSourceBaseSubobject) so no access-checked derived-to-base
// conversion is needed -- which matters for private/protected bases.
void AppendCXXBaseAssignments(Syntax* syntax, TypeRecord* func, Vector* body,
                              SourceLocation location) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL ||
      func->info.function.prototype.length < 2) {
    return;
  }
  CXXSpecialMemberKind kind = func->info.function.cxx_special_member_kind;
  if (kind != kCXXSpecialMemberCopyAssignment &&
      kind != kCXXSpecialMemberMoveAssignment) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  Symbol* source =
      func->info.function.prototype.value
          .p[func->info.function.prototype.length - 1];
  if (this_symbol == NULL || source == NULL) {
    return;
  }
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    TypeRecord* base_pointer =
        NewPointerTo(kQualPlain, TypeRecordCopy(base->type));
    ASTNode* offset = NewIntConstantASTNode(
        base->byte_offset, NewTypeRecordWithSize(kTypeInt, kQualPlain),
        location);
    ASTNode* receiver =
        NewBinaryASTNode(AST_OP(plus), base_pointer, location,
                         NewIdentifierASTNode(this_symbol, location), offset);
    receiver->flags |= kASTAnalyzed;
    ASTNodeSetType(receiver, base_pointer);
    ASTNode* source_base = NewCXXSourceBaseSubobject(source, base, location);
    if (source_base == NULL) {
      continue;
    }
    ASTNode* member_node =
        NewStringConstantASTNode(NewString("operator="), NULL, location);
    ASTNode* member_access =
        NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver, member_node);
    Vector* actuals = NewVector();
    VectorAppend(actuals, source_base);
    ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, location,
                                     member_access, actuals);
    VectorAppend(body, NewExpressionStatementASTNode(call, location));
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
  init_list->delegating_statement = NULL;
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
  ASTNodeDelete(init_list->delegating_statement);
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

// Defined in type_template_substitute.c; declared here to avoid pulling the
// template-instantiation internal header into syntax.c.  Both subtract `base`
// from any template-parameter index that is >= base, leaving enclosing-template
// parameters (index < base) untouched.
void RebaseTemplateParameterIndices(TypeRecord* type, int base);
void RebaseTemplateArgumentParameterIndices(TemplateArgument* arg, int base);

typedef struct {
  TypeRecord* from_func;
  TypeRecord* to_func;
  int rebase_base;
} ConstructorInitFormalRemap;

/* Visitor: within a cloned member-initializer actual, (1) rewrite an identifier
 * that names one of `from_func`'s parameters to the parameter at the same
 * position in `to_func`, and (2) renumber template-parameter indices in any
 * explicit template arguments / cast types down by `rebase_base` so the
 * member's own parameters become zero-based (matching the class-level
 * constructor whose template_parameter_base was reset to 0). */
static void RemapConstructorInitFormalVisitor(ASTNode* node, void* data,
                                              int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL) {
    return;
  }
  ConstructorInitFormalRemap* remap = data;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node;
    Vector* from = &remap->from_func->info.function.prototype;
    Vector* to = &remap->to_func->info.function.prototype;
    for (size_t i = 0; i < from->length && i < to->length; i++) {
      if (from->value.p[i] == id->symbol) {
        Symbol* replacement = to->value.p[i];
        id->symbol = replacement;
        if (replacement != NULL) {
          ASTNodeSetType(node, replacement->type);
        }
        break;
      }
    }
    if (remap->rebase_base > 0 && id->template_arguments != NULL) {
      for (size_t i = 0; i < id->template_arguments->length; i++) {
        RebaseTemplateArgumentParameterIndices(
            id->template_arguments->value.p[i], remap->rebase_base);
      }
    }
  } else if (node->op == AST_OP(cast) && remap->rebase_base > 0) {
    CastASTNode* cast = (CastASTNode*)node;
    RebaseTemplateParameterIndices(cast->cast_type, remap->rebase_base);
  }
}

/* Rewrite every reference to one of `from_func`'s parameters inside the deferred
 * member-initializer actuals of `init_list` to the correspondingly-positioned
 * parameter of `to_func`, and rebase the member template's own template-
 * parameter indices by `rebase_base`.  Used when a class-template instantiation
 * clones a member function template constructor: the deferred init-list is
 * shared from the primary and still names the primary's parameters and numbers
 * the member's own template parameters relative to the enclosing class, but the
 * per-call preamble insertion keys its clone maps off the cloned (class-level)
 * prototype (whose parameters are fresh and whose template_parameter_base is 0).
 * Aligning both lets pack initializers such as
 * `value(std::forward<Args>(args)...)` expand against the concrete arguments
 * instead of dropping `forward`'s explicit template argument. */
void SyntaxCXXConstructorInitListRemapFormals(CXXConstructorInitList* init_list,
                                              TypeRecord* from_func,
                                              TypeRecord* to_func,
                                              int rebase_base) {
  if (init_list == NULL || from_func == NULL || to_func == NULL ||
      !TypeIsFunction(from_func) || !TypeIsFunction(to_func)) {
    return;
  }
  ConstructorInitFormalRemap remap = {from_func, to_func, rebase_base};
  for (size_t i = 0; i < init_list->deferred_initializers.length; i++) {
    CXXDeferredConstructorInitializer* init =
        init_list->deferred_initializers.value.p[i];
    if (init == NULL || init->actuals == NULL) {
      continue;
    }
    for (size_t j = 0; j < init->actuals->length; j++) {
      ASTNodeVisit(init->actuals->value.p[j],
                   RemapConstructorInitFormalVisitor, 0, &remap);
    }
  }
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

/* The primary-template name of a class type, or NULL if it is not a template
 * instantiation.  A template-id base is named in a mem-initializer by its bare
 * template name (`Base<T>()` records the base as "Base"), whereas the
 * instantiated base's tag_name is the specialized form ("Base<int>"), so base
 * matching against a mem-initializer name must also consider it. */
static String* CXXPrimaryTemplateName(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type)) {
    return NULL;
  }
  Symbol* origin = type->template_origin;
  if (origin == NULL && type->info.struct_info != NULL &&
      type->info.struct_info->tag_symbol != NULL &&
      type->info.struct_info->tag_symbol->type != NULL) {
    origin = type->info.struct_info->tag_symbol->type->template_origin;
  }
  return origin != NULL ? &origin->name : NULL;
}

static CXXBaseSpecifier* FindCXXDirectBaseByName(Struct* owner,
                                                 const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  StructMember* alias_member = FindStructMemberByName(owner, name);
  TypeRecord* alias_type =
      alias_member != NULL && alias_member->symbol != NULL &&
              StorageIs(alias_member->symbol->storage, STO(typedef))
          ? alias_member->symbol->type
          : NULL;
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (alias_type != NULL && TypeEqual(alias_type, base->type)) {
      return base;
    }
    if (base->type->info.struct_info->tag_name != NULL &&
        StringEqual(base->type->info.struct_info->tag_name, name)) {
      return base;
    }
    String* template_name = CXXPrimaryTemplateName(base->type);
    if (template_name != NULL && StringEqual(template_name, name)) {
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
  StructMember* alias_member = FindStructMemberByName(owner, name);
  TypeRecord* alias_type =
      alias_member != NULL && alias_member->symbol != NULL &&
              StorageIs(alias_member->symbol->storage, STO(typedef))
          ? alias_member->symbol->type
          : NULL;
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (alias_type != NULL && TypeEqual(alias_type, base->type)) {
      return base;
    }
    if (base->type->info.struct_info->tag_name != NULL &&
        StringEqual(base->type->info.struct_info->tag_name, name)) {
      return base;
    }
    String* template_name = CXXPrimaryTemplateName(base->type);
    if (template_name != NULL && StringEqual(template_name, name)) {
      return base;
    }
  }
  return NULL;
}

static bool CXXConstructorInitializerNamesOwner(Struct* owner,
                                                const char* name) {
  if (owner == NULL || name == NULL) {
    return false;
  }
  if (owner->tag_name != NULL && StringEqual(owner->tag_name, name)) {
    return true;
  }
  if (owner->tag_symbol != NULL && owner->tag_symbol->type != NULL) {
    String* template_name = CXXPrimaryTemplateName(owner->tag_symbol->type);
    if (template_name != NULL && StringEqual(template_name, name)) {
      return true;
    }
  }
  StructMember* alias = FindStructMemberByName(owner, name);
  return alias != NULL && alias->symbol != NULL &&
         StorageIs(alias->symbol->storage, STO(typedef)) &&
         alias->symbol->type != NULL && owner->tag_symbol != NULL &&
         owner->tag_symbol->type != NULL &&
         TypeEqual(alias->symbol->type, owner->tag_symbol->type);
}

static ASTNode* NewCXXDelegatingConstructorCall(
    Syntax* syntax, TypeRecord* func, Vector* actuals,
    SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL) {
    VectorDelete(actuals);
    return NULL;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  Symbol* this_symbol = FindThisSymbol(syntax);
  if (this_symbol == NULL && func->info.function.prototype.length > 0) {
    this_symbol = func->info.function.prototype.value.p[0];
  }
  if (this_symbol == NULL || owner->tag_name == NULL) {
    VectorDelete(actuals);
    return NULL;
  }
  if (StructHasVirtualBases(owner) &&
      func->info.function.prototype.length > 1) {
    Symbol* complete_object = func->info.function.prototype.value.p[1];
    ASTNode* argument = NewIdentifierASTNode(complete_object, location);
    if (actuals->length == 0) {
      VectorAppend(actuals, argument);
    } else {
      VectorInsertBefore(actuals, 0, argument);
    }
  }
  ASTNode* member = NewStringConstantASTNode(
      NewString(owner->tag_name->value), NULL, location);
  ASTNode* access = NewBinaryASTNode(
      AST_OP(arrow), NULL, location,
      NewIdentifierASTNode(this_symbol, location), member);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, access, actuals);
  return NewExpressionStatementASTNode(call, location);
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
  for (size_t i = 0; i < func->info.function.prototype.length; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL) {
      formal->value.arg_number = (int32_t)i;
    }
  }
  Symbol* source = CXXSourceObjectParameter(func);
  if (source == NULL) {
    return;
  }
  bool is_constructor_initializer =
      func->info.function.cxx_special_member_kind ==
      kCXXSpecialMemberCopyConstructor;
  Struct* owner = func->info.function.cxx_member_owner;
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    TypeRecord* member_type = member->symbol->type;
    if (is_constructor_initializer && !TypeIsFixedArray(member_type)) {
      continue;
    }
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
  // Mark as a forced (already-analyzed) byte-offset pointer adjustment so it
  // survives template-instantiation re-analysis with its forced `source*` type
  // intact.  Otherwise `this + source_offset` is re-analyzed as the receiver's
  // own (element-scaled) pointer type and the `->__vptr` binds to the wrong
  // subobject offset -- for a secondary base at a non-zero offset this collapses
  // every vptr store onto offset 0 (see NewCXXVBPtrReceiver for the same fix).
  adjusted->flags |= kASTAnalyzed | kASTForcedTypeAdjustment;
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

// Returns the subobject that physically owns the shared __vptr for `s`: the
// (unique) most-basic polymorphic class in `s`'s hierarchy that carries its own
// __vptr member.  Classes that derive from a polymorphic base (virtual or not)
// do not get their own vptr; they share the provider's.
static Struct* CXXFindVPtrProvider(Struct* s) {
  if (s == NULL) {
    return NULL;
  }
  if (s->vptr_member != NULL) {
    return s;
  }
  for (size_t i = 0; i < s->bases.length; i++) {
    CXXBaseSpecifier* base = s->bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* p = CXXFindVPtrProvider(base->type->info.struct_info);
    if (p != NULL) {
      return p;
    }
  }
  for (size_t i = 0; i < s->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = s->virtual_bases.value.p[i];
    if (base->type == NULL || !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    Struct* p = CXXFindVPtrProvider(base->type->info.struct_info);
    if (p != NULL) {
      return p;
    }
  }
  return NULL;
}

// Byte offset of the non-virtual base `to` within `from` (following only
// non-virtual base links), or false if `to` is not a non-virtual base.
static bool CXXNonVirtualBaseOffset(Struct* from, Struct* to, int inherited,
                                    int* out) {
  if (from == NULL || to == NULL) {
    return false;
  }
  if (from == to) {
    if (out != NULL) {
      *out = inherited;
    }
    return true;
  }
  for (size_t i = 0; i < from->bases.length; i++) {
    CXXBaseSpecifier* base = from->bases.value.p[i];
    if (base->is_virtual || base->type == NULL ||
        !TypeIsStructOrUnion(base->type) ||
        base->type->info.struct_info == NULL) {
      continue;
    }
    if (CXXNonVirtualBaseOffset(base->type->info.struct_info, to,
                                inherited + base->byte_offset, out)) {
      return true;
    }
  }
  return false;
}

// Computes the physical byte offset (within the complete object `owner`) of the
// __vptr slot that an initializer for `source` at `source_offset` would write.
// This lets us detect two initializers that target the same physical vptr, which
// happens when a polymorphic base is a (shared) virtual base: e.g. in a diamond
// `SS : IStream, OStream` where both virtually inherit polymorphic `Ios`, the
// SS/IStream/OStream vtables all target Ios's single shared vptr.  Only the
// first (most-derived, primary) initializer must survive; otherwise a secondary
// base's vtable overwrites the shared vptr and virtual dispatch (e.g. the
// destructor) receives a wrongly this-adjusted object.
static bool CXXVPtrPhysicalOffset(Struct* owner, Struct* source,
                                  int source_offset, int* out) {
  if (owner == NULL || source == NULL) {
    return false;
  }
  Struct* provider = CXXFindVPtrProvider(source);
  if (provider == NULL || provider->vptr_member == NULL) {
    return false;
  }
  int vptr_off = provider->vptr_member->byte_offset;
  // If the provider lives inside one of the complete object's virtual bases, the
  // vptr is shared and its physical offset is fixed by the complete-object
  // layout (independent of which base subobject we approached it through).
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* vb = owner->virtual_bases.value.p[i];
    if (vb->type == NULL || !TypeIsStructOrUnion(vb->type) ||
        vb->type->info.struct_info == NULL) {
      continue;
    }
    int within = 0;
    if (CXXNonVirtualBaseOffset(vb->type->info.struct_info, provider, 0,
                                &within)) {
      if (out != NULL) {
        *out = vb->byte_offset + within + vptr_off;
      }
      return true;
    }
  }
  // Otherwise the provider is reached from `source` through non-virtual bases.
  int within = 0;
  if (CXXNonVirtualBaseOffset(source, provider, 0, &within)) {
    if (out != NULL) {
      *out = source_offset + within + vptr_off;
    }
    return true;
  }
  return false;
}

static void AppendCXXVPtrInitializers(TypeRecord* func, Vector* body,
                                      SourceLocation location) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL || body == NULL) {
    return;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  int* seen_offsets = NULL;
  size_t seen_count = 0;
  if (owner->vtable_symbols.length > 0) {
    seen_offsets = malloc(owner->vtable_symbols.length * sizeof(int));
  }
  for (size_t i = 0; i < owner->vtable_symbols.length; i++) {
    CXXVTableInfo* info = owner->vtable_symbols.value.p[i];
    int phys = 0;
    if (seen_offsets != NULL &&
        CXXVPtrPhysicalOffset(owner, info->source, info->source_offset,
                              &phys)) {
      bool already = false;
      for (size_t j = 0; j < seen_count; j++) {
        if (seen_offsets[j] == phys) {
          already = true;
          break;
        }
      }
      if (already) {
        continue;
      }
      seen_offsets[seen_count++] = phys;
    }
    ASTNode* init = NewCXXVPtrInitializer(func, info, location);
    if (init != NULL) {
      VectorAppend(body, init);
    }
  }
  free(seen_offsets);
}

static TypeRecord* NewCXXStructType(Struct* str) {
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  TypeRecordSetStructInfo(type, str);
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
  adjusted->flags |= kASTAnalyzed | kASTForcedTypeAdjustment;
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

static bool CXXConstructorSetHasUserProvidedDefault(TypeRecord* record_type,
                                                    StructMember* constructor) {
  if (record_type == NULL || record_type->info.struct_info == NULL) {
    return false;
  }
  size_t first_user_formal =
      StructHasVirtualBases(record_type->info.struct_info) ? 2 : 1;
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor || info->is_deleted ||
        !info->is_user_provided ||
        info->prototype.length < first_user_formal) {
      continue;
    }
    bool all_defaulted = true;
    for (size_t i = first_user_formal; i < info->prototype.length; i++) {
      Symbol* formal = info->prototype.value.p[i];
      if (formal == NULL || formal->default_argument == NULL) {
        all_defaulted = false;
        break;
      }
    }
    if (all_defaulted) {
      return true;
    }
  }
  return false;
}

static ASTNode* NewCXXAggregateMemberZeroInitializer(
    Syntax* syntax, TypeRecord* func, StructMember* member,
    SourceLocation location) {
  ASTNode* target =
      NewCXXThisMemberAccess(func, member->symbol->name.value, location);
  if (target == NULL) {
    return NULL;
  }
  Symbol* storage = SyntaxNewTemporary(syntax, member->symbol->type);
  ASTNode* value = NewCompoundLiteralASTNode(
      NewIdentifierASTNode(storage, location), location,
      NewBracedInitializerASTNode(NewVector(), NULL, location));
  ASTNode* assign = NewBinaryASTNode(AST_OP(assign), member->symbol->type,
                                     location, target, value);
  assign->flags |= kASTCXXMemberInitializer;
  return NewExpressionStatementASTNode(assign, location);
}

static ASTNode* NewCXXMemberInitializerStatement(Syntax* syntax,
                                                TypeRecord* func,
                                                StructMember* member,
                                                Vector* actuals,
                                                bool value_initialize_empty,
                                                SourceLocation location) {
  if (member == NULL || member->symbol == NULL || actuals == NULL) {
    if (actuals != NULL) {
      VectorDelete(actuals);
    }
    return NULL;
  }

  TypeRecord* member_type = member->symbol->type;
  if (TypeIsFixedArray(member_type) && member_type->next != NULL) {
    size_t element_count = member_type->info.array.size.fixed;
    if (actuals->length > element_count) {
      SyntaxError(syntax, "too many initializers for array member %s",
                  member->symbol->name.value);
      VectorDeleteWithContents(actuals,
                               (VectorElementDestructor)ASTNodeDelete,
                               /*free_element=*/false);
      return NULL;
    }
    TypeRecord* element_type = member_type->next;
    StructMember* element_constructor =
        TypeIsStructOrUnion(element_type) ? FindCXXConstructor(element_type)
                                         : NULL;
    Vector* statements = NewVector();
    for (size_t i = 0; i < element_count; i++) {
      ASTNode* receiver =
          NewCXXThisMemberAccess(func, member->symbol->name.value, location);
      receiver = NewBinaryASTNode(
          AST_OP(subscript), NULL, location, receiver,
          NewIntConstantASTNode((int64_t)i,
                                NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                location));
      ASTNode* stmt = NULL;
      if (element_constructor != NULL) {
        Vector* element_actuals = NewVector();
        if (i < actuals->length) {
          VectorAppend(element_actuals, actuals->value.p[i]);
          actuals->value.p[i] = NULL;
        }
        CXXPrependCompleteObjectArgument(element_type, element_actuals,
                                         /*complete_object=*/true, location);
        ASTNode* constructor_name = NewStringConstantASTNode(
            NewString(CXXConstructorNameForType(element_type)), NULL, location);
        ASTNode* access = NewBinaryASTNode(AST_OP(dot), NULL, location,
                                           receiver, constructor_name);
        ASTNode* call =
            NewVectorASTNode(AST_OP(call), NULL, location, access,
                             element_actuals);
        call->flags |= kASTCXXMemberInitializer;
        stmt = NewExpressionStatementASTNode(call, location);
      } else {
        ASTNode* value =
            i < actuals->length
                ? actuals->value.p[i]
                : NewIntConstantASTNode(
                      0, TypeRecordCopy(element_type), location);
        if (i < actuals->length) {
          actuals->value.p[i] = NULL;
        }
        ASTNode* assign = NewBinaryASTNode(AST_OP(assign), element_type,
                                           location, receiver, value);
        assign->flags |= kASTCXXMemberInitializer;
        stmt = NewExpressionStatementASTNode(assign, location);
      }
      VectorAppend(statements, stmt);
      if (CompilerExceptionsEnabled() &&
          TypeHasNonTrivialDestructor(element_type)) {
        ASTNode* cleanup_receiver =
            NewCXXThisMemberAccess(func, member->symbol->name.value, location);
        cleanup_receiver = NewBinaryASTNode(
            AST_OP(subscript), NULL, location, cleanup_receiver,
            NewIntConstantASTNode((int64_t)i,
                                  NewTypeRecordWithSize(kTypeInt, kQualPlain),
                                  location));
        ASTNode* cleanup = NewCXXMemberDestructorCall(
            syntax, func, member, element_type, cleanup_receiver, location);
        if (cleanup != NULL) {
          cleanup->flags |= kASTEHCleanupOnly;
          VectorAppend(statements, cleanup);
        }
      }
    }
    VectorDeleteWithContents(actuals,
                             (VectorElementDestructor)ASTNodeDelete,
                             /*free_element=*/false);
    return NewCompoundStatementASTNode(statements, location);
  }
  StructMember* constructor = TypeIsStructOrUnion(member_type)
                                  ? FindCXXConstructor(member_type)
                                  : NULL;
  bool braced_aggregate =
      TypeIsStructOrUnion(member_type) &&
      member_type->info.struct_info != NULL &&
      member_type->info.struct_info->is_aggregate &&
      actuals->length == 1 &&
      actuals->value.p[0] != NULL &&
      ((ASTNode*)actuals->value.p[0])->op == AST_OP(braced_init);
  if (braced_aggregate) {
    ASTNode* initializer = actuals->value.p[0];
    actuals->value.p[0] = NULL;
    VectorDelete(actuals);
    ASTNode* target =
        NewCXXThisMemberAccess(func, member->symbol->name.value, location);
    if (target == NULL) {
      ASTNodeDelete(initializer);
      return NULL;
    }
    target->flags |= kASTNeedAddress | kASTIsDeclaration;
    ASTNode* init =
        NewBinaryASTNode(AST_OP(init), member_type, location, target,
                         initializer);
    init->flags |= kASTCXXMemberInitializer;
    return NewExpressionStatementASTNode(init, location);
  }
  if (constructor != NULL) {
    bool zero_before_default =
        value_initialize_empty && actuals->length == 0 &&
        !CXXConstructorSetHasUserProvidedDefault(member_type, constructor);
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
    ASTNode* call =
        NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
    call->flags |= kASTCXXMemberInitializer;
    ASTNode* constructor_call =
        NewExpressionStatementASTNode(call, location);
    if (!zero_before_default) {
      return constructor_call;
    }
    ASTNode* zero = NewCXXAggregateMemberZeroInitializer(
        syntax, func, member, location);
    if (zero == NULL) {
      return constructor_call;
    }
    Vector* statements = NewVector();
    VectorAppend(statements, zero);
    VectorAppend(statements, constructor_call);
    return NewCompoundStatementASTNode(statements, location);
  }

  if (actuals->length == 0) {
    VectorDelete(actuals);
    if (TypeIsStructOrUnion(member_type) || TypeIsArray(member_type)) {
      return NewCXXAggregateMemberZeroInitializer(syntax, func, member,
                                                  location);
    }
    ASTNode* target =
        NewCXXThisMemberAccess(func, member->symbol->name.value, location);
    if (target == NULL) {
      return NULL;
    }
    ASTNode* value =
        NewIntConstantASTNode(0, TypeRecordCopy(member_type), location);
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
  return NewCXXMemberInitializerStatement(syntax, func, member, actuals, true,
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
  if (member == NULL || member->symbol == NULL) {
    return NULL;
  }
  if (member->default_initializer != NULL) {
    Vector* actuals = CXXDefaultMemberInitializerActuals(
        member->default_initializer);
    return NewCXXMemberInitializerStatement(
        syntax, func, member, actuals, true,
        member->default_initializer->location);
  }
  // No default member initializer and no explicit mem-initializer: a class-type
  // member that has a default constructor must still be default-constructed by
  // the enclosing constructor.  A scalar member is left uninitialized, matching
  // C++ default initialization, so only synthesize a call for a record member
  // that has a constructor.  This does not apply to the memberwise copy/move
  // special members, whose members are copied/moved from the source object (by
  // AppendCXXMemberwiseAssignments) rather than default-constructed.
  if (func != NULL && TypeIsFunction(func)) {
    switch (func->info.function.cxx_special_member_kind) {
      case kCXXSpecialMemberCopyConstructor:
      case kCXXSpecialMemberMoveConstructor:
      case kCXXSpecialMemberCopyAssignment:
      case kCXXSpecialMemberMoveAssignment:
        return NULL;
      default:
        break;
    }
  }
  TypeRecord* member_type = member->symbol->type;
  if (!CXXTypeHasDefaultConstructor(member_type)) {
    return NULL;
  }
  return NewCXXMemberInitializerStatement(
      syntax, func, member, NewVector(), false, member->symbol->location);
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

static TypeRecord* CXXConstructorInitializerTargetType(Struct* owner,
                                                       const char* name) {
  if (owner == NULL || name == NULL) {
    return NULL;
  }
  if (CXXConstructorInitializerNamesOwner(owner, name)) {
    return owner->tag_symbol != NULL ? owner->tag_symbol->type : NULL;
  }
  CXXBaseSpecifier* base = FindCXXDirectBaseByName(owner, name);
  if (base != NULL) {
    return base->type;
  }
  CXXVirtualBaseInfo* virtual_base = FindCXXVirtualBaseByName(owner, name);
  if (virtual_base != NULL) {
    return virtual_base->type;
  }
  StructMember* member = FindCXXDirectDataMemberByName(owner, name);
  return member != NULL && member->symbol != NULL ? member->symbol->type : NULL;
}

static Vector* ResolveCXXBracedConstructorInitializerActuals(
    TypeRecord* target_type, Vector* actuals) {
  if (target_type == NULL || actuals == NULL || actuals->length != 1) {
    return actuals;
  }
  ASTNode* root = actuals->value.p[0];
  StructMember* constructor = TypeIsStructOrUnion(target_type)
                                  ? FindCXXConstructor(target_type)
                                  : NULL;
  if (root == NULL || root->op != AST_OP(braced_init) ||
      (TypeIsStructOrUnion(target_type) &&
       target_type->info.struct_info != NULL &&
       target_type->info.struct_info->is_aggregate) ||
      CXXConstructorSetHasInitializerList(constructor)) {
    return actuals;
  }

  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)root;
  Vector* expanded = NewVector();
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    ASTNode* expression = initializer;
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)initializer;
      expression = expr_init->expr;
      expr_init->expr = NULL;
    }
    braced->initializers->value.p[i] = NULL;
    if (expression != NULL) {
      expression->parent = NULL;
      VectorAppend(expanded, expression);
    }
    if (initializer != expression) {
      ASTNodeDelete(initializer);
    }
  }
  actuals->value.p[0] = NULL;
  ASTNodeDelete(root);
  VectorDelete(actuals);
  return expanded;
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
    // A base-class mem-initializer-id may be a template-id (`Base<T>(args)`), so
    // consume any template argument list here; the base is still matched by its
    // bare name (FullyQualifiedIdentifierLast strips the template arguments).
    // The following token is the initializer's opening `(` / `{`.
    if (!SyntaxParseFullyQualifiedIdentifierWithTemplateIds(syntax, &name,
                                                            TC(openbra))) {
      SyntaxError(syntax, "Expected constructor initializer name");
      FullyQualifiedIdentifierDestruct(&name);
      break;
    }

    const char* init_name = FullyQualifiedIdentifierLast(&name);
    Vector* actuals = NULL;
    if (LexMatch(syntax->lex, TOK(lparen))) {
      actuals = ParseCXXInitializerArgumentList(syntax, TOK(rparen));
    } else if (LexLookingAt(syntax->lex, TOK(lbrace))) {
      TypeRecord* target_type =
          CXXConstructorInitializerTargetType(owner, init_name);
      StructMember* constructor =
          TypeIsStructOrUnion(target_type)
              ? FindCXXConstructor(target_type)
              : NULL;
      LexNextToken(syntax->lex);
      if (target_type == NULL ||
          (TypeIsStructOrUnion(target_type) &&
           target_type->info.struct_info != NULL &&
           target_type->info.struct_info->is_aggregate) ||
          CXXConstructorSetHasInitializerList(constructor)) {
        actuals = NewVector();
        VectorAppend(actuals, SyntaxParseBracedInitializer(syntax));
      } else {
        actuals = ParseCXXInitializerArgumentList(syntax, TOK(rbrace));
      }
    } else {
      SyntaxError(syntax, "Expected constructor initializer argument list");
      actuals = NewVector();
    }

    VectorAppend(&init_list->raw_initializers,
                 NewCXXDeferredConstructorInitializer(
                     init_name, CloneCXXConstructorInitializerActuals(actuals),
                     location));
    if (CXXConstructorInitializerNamesOwner(owner, init_name)) {
      if (init_list->delegating_statement != NULL ||
          init_list->base_specs.length != 0 ||
          init_list->virtual_base_specs.length != 0 ||
          init_list->member_specs.length != 0) {
        SyntaxError(syntax,
                    "delegating constructor initializer must appear alone");
        VectorDelete(actuals);
      } else {
        init_list->delegating_statement =
            NewCXXDelegatingConstructorCall(syntax, func, actuals, location);
      }
      FullyQualifiedIdentifierDestruct(&name);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      continue;
    }
    if (init_list->delegating_statement != NULL) {
      SyntaxError(syntax,
                  "delegating constructor initializer must appear alone");
      VectorDelete(actuals);
      FullyQualifiedIdentifierDestruct(&name);
      if (!LexMatch(syntax->lex, TOK(comma))) {
        break;
      }
      continue;
    }
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
            syntax, func, member, actuals, true, location);
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
    actuals = ResolveCXXBracedConstructorInitializerActuals(
        CXXConstructorInitializerTargetType(owner, init_name), actuals);
    if (CXXConstructorInitializerNamesOwner(owner, init_name)) {
      if (init_list->delegating_statement != NULL ||
          init_list->base_specs.length != 0 ||
          init_list->virtual_base_specs.length != 0 ||
          init_list->member_specs.length != 0) {
        SyntaxError(syntax,
                    "delegating constructor initializer must appear alone");
        VectorDelete(actuals);
      } else {
        init_list->delegating_statement =
            NewCXXDelegatingConstructorCall(syntax, func, actuals, location);
      }
      continue;
    }
    if (init_list->delegating_statement != NULL) {
      SyntaxError(syntax,
                  "delegating constructor initializer must appear alone");
      VectorDelete(actuals);
      continue;
    }
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
        syntax, func, member, actuals, true, location);
    if (stmt != NULL) {
      VectorAppend(&init_list->member_specs, member);
      VectorAppend(&init_list->member_statements, stmt);
    }
  }
}

// Partial-construction cleanup: once a constructor has fully constructed a base
// or member subobject, an exception thrown by a later initializer or by the
// constructor body must destroy that subobject (but not the incomplete `*this`).
// This inserts, right after the subobject's construction statement, its
// destructor call marked kASTEHCleanupOnly, so the backend emits it only inside
// an exception cleanup pad -- never on the normal path -- with an EH range
// covering the remainder of the constructor.  Subobjects nest by construction
// order, so the runtime destroys them in reverse (see libc/eh_throw.c and
// GenerateCompoundStatement).  Returns the number of statements inserted.
static size_t InsertCXXPartialCleanupDestructor(Vector* body, size_t insert_at,
                                                ASTNode* dtor_stmt) {
  if (dtor_stmt == NULL) {
    return 0;
  }
  dtor_stmt->flags |= kASTEHCleanupOnly;
  VectorInsertOrAppend(body, insert_at, dtor_stmt);
  return 1;
}

static ASTNode* NewCXXDelegatingConstructorCleanup(TypeRecord* func,
                                                   Struct* owner,
                                                   SourceLocation location) {
  if (func == NULL || owner == NULL || owner->tag_name == NULL ||
      owner->tag_symbol == NULL || owner->tag_symbol->type == NULL ||
      func->info.function.prototype.length == 0) {
    return NULL;
  }
  Symbol* this_symbol = func->info.function.prototype.value.p[0];
  if (this_symbol == NULL) {
    return NULL;
  }
  String destructor_name;
  StringInit(&destructor_name, "~");
  StringAppendString(&destructor_name, owner->tag_name);
  ASTNode* destructor =
      NewStringConstantASTNode(NewString(destructor_name.value), NULL, location);
  StringDestruct(&destructor_name);
  ASTNode* member_access = NewBinaryASTNode(
      AST_OP(arrow), NULL, location,
      NewIdentifierASTNode(this_symbol, location), destructor);
  Vector* actuals = NewVector();
  CXXPrependCompleteObjectArgument(owner->tag_symbol->type, actuals,
                                   /*complete_object=*/true, location);
  ASTNode* call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  return NewExpressionStatementASTNode(call, location);
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
  if (init_list->delegating_statement != NULL) {
    VectorInsertOrAppend(body, 0, init_list->delegating_statement);
    init_list->delegating_statement = NULL;
    if (CompilerExceptionsEnabled()) {
      ASTNode* cleanup =
          NewCXXDelegatingConstructorCleanup(func, owner, location);
      InsertCXXPartialCleanupDestructor(body, 1, cleanup);
    }
    return;
  }
  size_t insert_at = 0;
  // A defaulted copy/move constructor must copy/move-construct each base
  // subobject (virtual or not) from the corresponding subobject of the source,
  // not default-construct it.  The source's base subobject is extracted directly
  // by offset (NewCXXSourceBaseSubobject) and passed as the base-constructor
  // argument, so the base's copy/move constructor binds to it without an
  // access-checked derived-to-base conversion (which would reject a
  // private/protected base).
  CXXSpecialMemberKind ctor_kind = func->info.function.cxx_special_member_kind;
  Symbol* base_copy_source =
      (ctor_kind == kCXXSpecialMemberCopyConstructor ||
       ctor_kind == kCXXSpecialMemberMoveConstructor) &&
              !func->info.function.is_user_provided &&
              func->info.function.prototype.length > 0
          ? func->info.function.prototype.value
                .p[func->info.function.prototype.length - 1]
          : NULL;
  Symbol* member_copy_source =
      ctor_kind == kCXXSpecialMemberCopyConstructor ? base_copy_source : NULL;
  Vector* complete_initializers = NewVector();
  AppendCXXVBPtrInitializers(func, complete_initializers, location);
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    ASTNode* call =
        FindCXXExplicitVirtualBaseInitializer(init_list, base);
    if (call == NULL &&
        !VectorContainsPointer(&init_list->virtual_base_specs, base)) {
      Vector* vbase_actuals = NULL;
      if (base_copy_source != NULL) {
        CXXBaseSpecifier vbase_spec;
        vbase_spec.type = base->type;
        vbase_spec.access = base->access;
        vbase_spec.byte_offset = base->byte_offset;
        vbase_spec.is_virtual = true;
        ASTNode* source_base =
            NewCXXSourceBaseSubobject(base_copy_source, &vbase_spec, location);
        if (source_base != NULL) {
          vbase_actuals = NewVector();
          VectorAppend(vbase_actuals, source_base);
        }
      }
      call = NewCXXVirtualBaseSpecialMemberCall(syntax, func, base, false,
                                                vbase_actuals, location);
    }
    if (call == NULL) {
      continue;
    }
    VectorAppend(complete_initializers, call);
  }
  InsertCXXCompleteObjectGuardedStatements(func, body, &insert_at,
                                           complete_initializers, location);
  // Partial-construction cleanup for the virtual bases just constructed: if a
  // later initializer or the constructor body throws, the complete object must
  // destroy its virtual bases (in reverse construction order) yet not run its
  // own destructor.  The complete-object guard restricts this to the complete-
  // object constructor variant -- the base-subobject variant never builds the
  // virtual bases -- and kASTEHCleanupOnly makes the backend emit it only inside
  // an exception cleanup pad.  Inserted before the direct-base initializers so
  // its EH range starts earliest and is therefore torn down last (matching the
  // reverse-of-construction order; see GenerateCompoundStatement).
  if (CompilerExceptionsEnabled() && owner->virtual_bases.length > 0) {
    Vector* vbase_dtors = NewVector();
    for (size_t i = owner->virtual_bases.length; i > 0; i--) {
      CXXVirtualBaseInfo* vbase = owner->virtual_bases.value.p[i - 1];
      if (!TypeHasNonTrivialDestructor(vbase->type)) {
        continue;
      }
      ASTNode* dtor = NewCXXVirtualBaseSpecialMemberCall(
          syntax, func, vbase, /*destructor=*/true, NULL, location);
      if (dtor != NULL) {
        VectorAppend(vbase_dtors, dtor);
      }
    }
    ASTNode* guarded =
        NewCXXCompleteObjectGuardedStatement(func, vbase_dtors, location);
    if (guarded != NULL) {
      guarded->flags |= kASTEHCleanupOnly;
      VectorInsertOrAppend(body, insert_at, guarded);
      insert_at++;
    }
  }
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
      Vector* base_actuals = NULL;
      if (base_copy_source != NULL) {
        ASTNode* source_base =
            NewCXXSourceBaseSubobject(base_copy_source, base, location);
        if (source_base != NULL) {
          base_actuals = NewVector();
          VectorAppend(base_actuals, source_base);
        }
      }
      call = NewCXXBaseSpecialMemberCall(syntax, func, base, false,
                                         /*complete_object=*/false, base_actuals,
                                         location);
    }
    if (call == NULL) {
      continue;
    }
    VectorInsertOrAppend(body, insert_at, call);
    insert_at++;
    if (CompilerExceptionsEnabled() && TypeHasNonTrivialDestructor(base->type)) {
      ASTNode* dtor = NewCXXBaseSpecialMemberCall(syntax, func, base,
                                                  /*destructor=*/true,
                                                  /*complete_object=*/false,
                                                  NULL, location);
      insert_at += InsertCXXPartialCleanupDestructor(body, insert_at, dtor);
    }
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
  // A union holds at most one active variant member, so a constructor may not
  // initialize every member the way a struct constructor does.  The
  // mem-initializer list names the member to activate; failing that the first
  // variant member carrying a default member initializer is activated.  Every
  // other variant member stays uninitialized -- running its default
  // constructor would both clobber the active member's storage and leave a
  // constructed object the destructor never tears down.
  StructMember* union_variant_member = NULL;
  if (owner->is_union && member_copy_source == NULL) {
    for (size_t i = 0; i < owner->members.length; i++) {
      StructMember* member = owner->members.value.p[i];
      if (member == NULL || member->symbol == NULL || member->is_static ||
          member->is_member_function ||
          StorageIs(member->symbol->storage, STO(typedef))) {
        continue;
      }
      if (VectorContainsPointer(&init_list->member_specs, member)) {
        union_variant_member = member;
        break;
      }
      if (union_variant_member == NULL && member->default_initializer != NULL) {
        union_variant_member = member;
      }
    }
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || StorageIs(member->symbol->storage,
                                                STO(typedef))) {
      continue;
    }
    if (owner->is_union && member_copy_source == NULL &&
        member != union_variant_member) {
      continue;
    }
    ASTNode* stmt = FindCXXExplicitMemberInitializer(init_list, member);
    if (stmt == NULL && member_copy_source != NULL &&
        !TypeIsFixedArray(member->symbol->type)) {
      ASTNode* source_member =
          NewCXXSourceMemberAccess(member_copy_source,
                                   member->symbol->name.value, location);
      if (TypeIsStructOrUnion(member->symbol->type)) {
        Vector* actuals = NewVector();
        VectorAppend(actuals, source_member);
        stmt = SyntaxNewCXXMemberInitializerStatement(
            syntax, func, member, actuals, location);
      } else {
        ASTNode* target_member =
            NewCXXThisMemberAccess(func, member->symbol->name.value, location);
        ASTNode* assign =
            NewBinaryASTNode(AST_OP(assign), member->symbol->type, location,
                             target_member, source_member);
        assign->flags |= kASTCXXMemberInitializer;
        stmt = NewExpressionStatementASTNode(assign, location);
      }
    }
    if (stmt == NULL) {
      stmt = NewCXXDefaultMemberInitializerStatement(syntax, func, member);
    }
    if (stmt == NULL) {
      continue;
    }
    VectorInsertOrAppend(body, insert_at, stmt);
    insert_at++;
    // Partial-construction cleanup for this member.  A fixed-array member yields
    // one destructor per element (reverse index order); the codegen pools the
    // consecutive kASTEHCleanupOnly statements into a single cleanup pad.  This
    // covers a throw after the member is fully constructed; a throw *during*
    // element construction of an array is not yet covered (it needs a runtime
    // element counter) and leaks the elements built so far.
    TypeRecord* member_type = member->symbol->type;
    if (CompilerExceptionsEnabled() &&
        TypeHasNonTrivialDestructor(member_type)) {
      Vector member_dtors;
      VectorInit(&member_dtors);
      AppendCXXSingleMemberDestructorCalls(func, member, &member_dtors,
                                           location);
      for (size_t k = 0; k < member_dtors.length; k++) {
        insert_at += InsertCXXPartialCleanupDestructor(
            body, insert_at, member_dtors.value.p[k]);
      }
      VectorDestruct(&member_dtors);
    }
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
      bool inserted_any = false;
      // __vbptr initializers share the same registration dependency as the
      // __vptr stores: when the preamble was built the class's vbtables had not
      // been registered, so AppendCXXVBPtrInitializers produced nothing and the
      // eager preamble emission dropped them.  Re-emit them here.  The first
      // batch must run before any virtual base is constructed, because the
      // derived-to-virtual-base pointer conversion reads __vbptr; insert it as a
      // complete-object-guarded block at the very front, ahead of the existing
      // virtual-base construction block (also at the front).
      size_t vptr_index = pending->index;
      Vector* vbptr_inits = NewVector();
      AppendCXXVBPtrInitializers(pending->func, vbptr_inits, pending->location);
      if (vbptr_inits->length > 0) {
        ASTNode* guarded = NewCXXCompleteObjectGuardedStatement(
            pending->func, vbptr_inits, pending->location);
        if (guarded != NULL) {
          VectorInsertOrAppend(compound->statements, 0, guarded);
          guarded->parent = &compound->base;
          vptr_index++;
          inserted_any = true;
        }
      } else {
        VectorDelete(vbptr_inits);
      }

      Vector* vptr_initializers = NewVector();
      AppendCXXVPtrInitializers(pending->func, vptr_initializers,
                                pending->location);
      // VectorInsertOrAppend handles index == length (e.g. an empty body whose
      // only statements are the deferred __vptr stores), which the strict
      // VectorInsertBefore inside CompoundASTNodeInsertStatement does not.
      size_t at = vptr_index;
      for (size_t j = 0; j < vptr_initializers->length; j++) {
        ASTNode* stmt = vptr_initializers->value.p[j];
        VectorInsertOrAppend(compound->statements, at, stmt);
        stmt->parent = &compound->base;
        at++;
      }
      if (vptr_initializers->length > 0) {
        inserted_any = true;
      }
      VectorDelete(vptr_initializers);

      // A base-class constructor may have overwritten __vbptr with its own
      // (base-subobject) vbtable, so restore the most-derived vbtables after the
      // base constructors and __vptr stores, matching the eager preamble path.
      Vector* vbptr_restores = NewVector();
      AppendCXXVBPtrInitializers(pending->func, vbptr_restores,
                                 pending->location);
      if (vbptr_restores->length > 0) {
        ASTNode* guarded = NewCXXCompleteObjectGuardedStatement(
            pending->func, vbptr_restores, pending->location);
        if (guarded != NULL) {
          VectorInsertOrAppend(compound->statements, at, guarded);
          guarded->parent = &compound->base;
          inserted_any = true;
        }
      } else {
        VectorDelete(vbptr_restores);
      }

      if (inserted_any) {
        for (size_t k = 0; k < compound->statements->length; k++) {
          ((ASTNode*)compound->statements->value.p[k])->child_id = (int)k;
        }
      }
    }
    free(pending);
  }
  pending_vptr_inits.length = out;
}

void SyntaxParseCXXDeletedFunctionReason(Syntax* syntax, TypeRecord* func) {
  if (!LexMatch(syntax->lex, TOK(lparen))) {
    return;
  }
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SyntaxError(syntax, "deleted function reasons require C++26");
  }
  if (!LexLookingAtStringLiteral(syntax->lex)) {
    SyntaxError(syntax,
                "deleted function reason must be an unevaluated string");
    if (!LexLookingAt(syntax->lex, TOK(rparen)) &&
        !LexLookingAt(syntax->lex, TOK(semicolon))) {
      LexNextToken(syntax->lex);
    }
  } else {
    if (LexValidateUnevaluatedString(
            syntax->lex, "deleted function reason",
            /*allow_user_defined_suffix=*/false)) {
      if (func->info.function.deleted_reason != NULL) {
        StringDelete(func->info.function.deleted_reason);
      }
      func->info.function.deleted_reason =
          NewStringWithLength(syntax->lex->spelling.value,
                              syntax->lex->spelling.length);
    }
    LexNextToken(syntax->lex);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(decl));
}

static void ParseCXXDefaultDeleteFunctionSpecifier(Syntax* syntax,
                                                   TypeRecord* func,
                                                   bool is_first_declaration) {
  if (!CompilerIsCXX() || func == NULL || !TypeIsFunction(func) ||
      !LexMatch(syntax->lex, TOK(equal))) {
    return;
  }
  if (LexMatch(syntax->lex, TOK(default))) {
    func->info.function.is_defaulted = true;
    func->info.function.is_explicitly_defaulted = true;
    func->info.function.is_constexpr_eligible = true;
    func->info.function.is_inline = true;
    if (is_first_declaration &&
        func->info.function.contract_assertions.length != 0) {
      SyntaxError(syntax,
                  "a function defaulted on its first declaration cannot "
                  "have contract assertions");
    }
    return;
  }
  if (LexMatch(syntax->lex, TOK(delete))) {
    func->info.function.is_deleted = true;
    func->info.function.is_explicitly_deleted = true;
    SyntaxParseCXXDeletedFunctionReason(syntax, func);
    SyntaxDiagnoseInvalidFunctionContracts(syntax, func);
    return;
  }
  SyntaxError(syntax, "function specifier must be '= default' or '= delete'");
}

static void CheckMatchingConstexprConsteval(Syntax* syntax,
                                            TypeRecord* previous,
                                            TypeRecord* current);

static ASTNode* DeclareOrDefineFunction(Syntax* syntax,
                                        Vector* declarations,
                             Symbol* sym, Symbol* old_sym) {
  if (sym->type->info.function.old_style) {
    // Old style functions have the types of their formal
    // arguments specified before the open brace for their
    // body.
    // Stop at end of input as well: the recovery below cannot move past it, so
    // an unparsable argument declaration would repeat forever.
    while (!LexLookingAt(syntax->lex, TOK(lbrace)) && !LexEof(syntax->lex)) {
      Token token_before = syntax->lex->current_token;
      SourceLocation location_before = syntax->lex->current_token_location;
      int errors_before = NumErrors();
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
      // Waiting for '{', so any other stuck token (including ')') must move.
      SyntaxEnsureProgress(syntax, token_before, location_before, errors_before,
                           TC(stmt));
    }
    // We need a function body after the argument declarations.
    if (!LexLookingAt(syntax->lex, TOK(lbrace))) {
      SyntaxError(syntax, "Function body expected");
    }
  }
  
  
  CXXConstructorInitList cxx_initializers;
  SyntaxCXXConstructorInitListInit(&cxx_initializers);
  bool constructor_scope_open =
      CompilerIsCXX() && TypeIsFunction(sym->type) &&
      sym->type->info.function.is_constructor;
  ParserContext constructor_old_context = syntax->context;
  TypeRecord* constructor_old_function = compiler->current_function;
  if (constructor_scope_open) {
    syntax->context = kParsingBlockScope;
    compiler->current_function = sym->type;
    SyntaxOpenScope(syntax);
    AddFunctionScopeSymbols(syntax, sym->type);
  }
  SyntaxParseCXXConstructorInitializerList(syntax, sym->type,
                                           &cxx_initializers);
  if (constructor_scope_open) {
    SyntaxCloseScope(syntax);
    compiler->current_function = constructor_old_function;
    syntax->context = constructor_old_context;
  }

  if (sym->type->info.function.is_defaulted) {
    if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
      old_sym->value.func_defn = sym;
      old_sym->type->info.function.is_defaulted = true;
    }
    bool defaulted_comparison =
        sym->type->info.function.cxx_member_owner != NULL &&
        (StringEqual(&sym->name, "operator==") ||
         StringEqual(&sym->name, "operator<=>"));
    bool defaulted_postfix =
        CXXFunctionIsDefaultedPostfixOperator(sym);
    if (defaulted_comparison || defaulted_postfix) {
      TypeParser parser;
      TypeParserInit(&parser, syntax->lex, syntax, STO(implicit),
                     kParsingFileScope);
      SynthesizeDefaultedMemberFunctionBody(&parser, sym);
      TypeParserDestruct(&parser);
      if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
        old_sym->type->info.function.is_deleted =
            sym->type->info.function.is_deleted;
        old_sym->type->info.function.is_implicitly_deleted =
            sym->type->info.function.is_implicitly_deleted;
      }
      SyntaxCXXConstructorInitListDestruct(&cxx_initializers);
      SyntaxNeedSemicolon(syntax, TC(decl));
      VectorAppend(declarations,
                   NewVariableDeclarationASTNode(sym, NULL, sym->location));
      return NewDeclarationListASTNode(declarations, sym->location);
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
      CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
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
        if (stmt->op == AST_OP(consteval_block) &&
            syntax->parsing_consteval_block_depth == 0) {
          SemanticAnalyzeFunctionConstevalBlockDuringParse(
              (ConstevalBlockASTNode*)stmt, body, body->length);
        }
      }
    }
    SyntaxInsertCXXConstructorPreamble(syntax, sym->type, body,
                                       &cxx_initializers, sym->location);
    // Destroy the function body's own block-scope locals on fall-through.  A
    // nested compound gets these appended by ParseCompoundStatement, but the
    // outermost function-body block is assembled here by hand, so append them
    // explicitly -- before the class member/base destructors so that, in a
    // destructor body, locals are destroyed before the enclosing object's
    // members and bases.  Coroutines are skipped: their locals live in the
    // coroutine frame and are destroyed by the lowered frame-cleanup logic.
    if (sym->type == NULL || !TypeIsFunction(sym->type) ||
        !sym->type->info.function.is_coroutine) {
      SyntaxAppendCXXBlockScopeDestructors(body);
    }
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
    CompilerQueuePendingTemplateInstantiation(definition);
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
  ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type, true);
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

static void AddFunctionAssociatedConstraint(TypeRecord* func,
                                            ConstraintExpr* constraint);
static void MoveTemplateParameterConstraints(Vector* parameters,
                                             ConstraintExpr** target);

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
  bool saved_friend_type_context = syntax->parsing_friend_type_specifier;
  bool used_friend_type_only_context =
      CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      LexLookingAt(syntax->lex, TOK(identifier)) &&
      SyntaxCurrentIdentifierFollowedByScopeOperator(syntax);
  syntax->parsing_friend_type_specifier =
      CompilerCXXAtLeast(kLanguageStandardCXX26);
  ParseDeclarationSpecifier(syntax, &storage, &is_inline, &is_constexpr,
                            &is_consteval, &is_constinit, &is_explicit, &type,
                            &attributes, kParsingFileScope);
  syntax->parsing_friend_type_specifier = saved_friend_type_context;

  TypeRecordIncRef(type);

  // C++26 friend-type-declaration:
  //   friend T;
  //   friend T, U;
  //   friend Ts...;
  // The first form predates C++26; comma-separated and expanded specifiers do
  // not.  Dependent type patterns are retained on the class template and
  // resolved for each specialization.
  if (LexLookingAt(syntax->lex, TOK(semicolon)) ||
      LexLookingAt(syntax->lex, TOK(comma)) ||
      LexLookingAt(syntax->lex, TOK(ellipsis))) {
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
      RecordFriendTypeSpecifier(syntax, befriending, type,
                                is_pack_expansion, location);
      TypeRecordDelete(type);
      type = NULL;
      AttributeListDestruct(&attributes);

      another = LexMatch(syntax->lex, TOK(comma));
      if (!another) {
        break;
      }
      VectorInit(&attributes);
      storage = STO(implicit);
      is_inline = false;
      is_constexpr = false;
      is_consteval = false;
      is_constinit = false;
      is_explicit = false;
      syntax->parsing_friend_type_specifier =
          CompilerCXXAtLeast(kLanguageStandardCXX26);
      ParseDeclarationSpecifier(
          syntax, &storage, &is_inline, &is_constexpr, &is_consteval,
          &is_constinit, &is_explicit, &type, &attributes,
          kParsingFileScope);
      syntax->parsing_friend_type_specifier = saved_friend_type_context;
      TypeRecordIncRef(type);
      if (type == NULL) {
        SyntaxError(syntax, "expected a friend type specifier");
        AttributeListDestruct(&attributes);
        break;
      }
    }
    SyntaxNeedSemicolon(syntax, TC(decl));
    if (type != NULL) {
      TypeRecordDelete(type);
      AttributeListDestruct(&attributes);
    }
    syntax->local_tag_stack = saved_tag_stack;
    return;
  }

  // Otherwise this is a friend function declaration or inline definition.  It
  // belongs to the nearest enclosing namespace scope, not to the class, so we
  // declare it there and merge with any existing overloads exactly as a normal
  // namespace-scope function declaration would.
  if (used_friend_type_only_context &&
      TypeContainsTemplateParameter(type)) {
    SyntaxError(syntax,
                "dependent friend function return type requires 'typename'");
  }
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, storage, kParsingFileScope);
  parser.is_inline = is_inline;
  parser.is_constexpr = is_constexpr;
  parser.is_consteval = is_consteval;
  parser.is_constinit = is_constinit;
  parser.parsing_friend_declaration = true;
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
  // A friend function defined in a non-local class definition is implicitly
  // inline even when the `inline` specifier is omitted ([dcl.fct.spec]).
  bool has_inline_friend_body = LexLookingAt(syntax->lex, TOK(lbrace));
  if (has_inline_friend_body) {
    sym->type->info.function.is_inline = true;
  }

  // A qualified friend declaration can name a namespace-scope function that
  // was declared before the befriending class. It does not redeclare that
  // function in the class's enclosing namespace.
  if (parser.cxx_qualified_friend_function != NULL) {
    Symbol* friend_function =
        FindMatchingOverload(parser.cxx_qualified_friend_function, sym->type,
                             /*incoming=*/NULL);
    if (friend_function == NULL) {
      SyntaxError(syntax,
                  "Qualified friend declaration does not match an existing "
                  "function");
    } else {
      StructAddFriendFunction(befriending, friend_function);
    }
    SyntaxNeedSemicolon(syntax, TC(decl));
    SyntaxCloseScope(syntax);
    TypeParserDestruct(&parser);
    TypeRecordDelete(type);
    AttributeListDestruct(&attributes);
    syntax->local_tag_stack = saved_tag_stack;
    return;
  }

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

  // Distinguish two "template friend" situations:
  //   (a) a friend declared inside a class *template* — its signature depends on
  //       the class's template parameters, so it is dependent and must be
  //       materialized per specialization (deferred below); versus
  //   (b) a friend function *template* declared inside a non-template class,
  //       e.g. `struct S { template<class I> friend bool operator==(S,const I&); };`
  //       — this is an ordinary namespace-scope function template, discoverable
  //       via ADL, and must be registered now (not deferred, since the class is
  //       never "instantiated").
  bool enclosing_class_is_template = false;
  for (Struct* c = befriending; c != NULL; c = c->lexical_parent) {
    if (c->is_template || c->template_parameter_count > 0 ||
        c->defining_template_scope_count > 0) {
      enclosing_class_is_template = true;
      break;
    }
  }

  // A friend function template's parameter indices start after the enclosing
  // class template's parameters. An ordinary dependent friend sees only the
  // enclosing parameters and remains a non-template function per
  // specialization.
  bool friend_has_own_template_head = false;
  if (syntax->parsing_template_declaration && TypeIsFunction(sym->type) &&
      syntax->current_template_parameters != NULL) {
    Vector* params = syntax->current_template_parameters;
    for (size_t i = 0; i < params->length; i++) {
      TemplateParameter* param = params->value.p[i];
      if (param != NULL &&
          param->index >= befriending->defining_template_scope_count) {
        friend_has_own_template_head = true;
        break;
      }
    }
  }

  // Case (b): give the friend its own template-parameter list and mark it a
  // function template so overload resolution / ADL treat it accordingly.  The
  // parameters currently live in `current_template_parameters`; copy them onto
  // the function type (the caller frees its own copy).
  if (friend_has_own_template_head &&
      sym->type->info.function.template_parameters.length == 0) {
    Vector* params = syntax->current_template_parameters;
    for (size_t i = 0; i < params->length; i++) {
      VectorAppend(&sym->type->info.function.template_parameters,
                   TemplateParameterCopy(params->value.p[i]));
    }
    sym->flags.is_template = true;
    sym->type->info.function.template_parameter_count = (int)params->length;
    TemplateParameter* first =
        params->length > 0 ? params->value.p[0] : NULL;
    sym->type->info.function.template_parameter_base =
        first != NULL ? first->index : 0;
    // Fold any concept-constrained parameters (e.g. `template <integral I>`) and
    // an explicit trailing requires-clause into the function's constraint.
    MoveTemplateParameterConstraints(
        &sym->type->info.function.template_parameters,
        &sym->type->info.function.associated_constraint);
    if (syntax->current_template_requires_clause != NULL) {
      AddFunctionAssociatedConstraint(sym->type,
                                      syntax->current_template_requires_clause);
      syntax->current_template_requires_clause = NULL;
    }
  }

  if (syntax->parsing_template_declaration && enclosing_class_is_template) {
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

  ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type,
                                         old_sym == NULL);
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

static void DiagnoseConstevalOnNonFunction(Syntax* syntax, TypeRecord* type,
                                           bool is_consteval) {
  if (is_consteval && (type == NULL || !TypeIsFunction(type))) {
    SyntaxError(syntax, "'consteval' can only be applied to functions");
  }
}

static void CheckMatchingConstexprConsteval(Syntax* syntax,
                                            TypeRecord* previous,
                                            TypeRecord* current) {
  if (!CompilerIsCXX() || previous == NULL || current == NULL ||
      !TypeIsFunction(previous) || !TypeIsFunction(current)) {
    return;
  }
  bool previous_immediate = previous->info.function.is_consteval;
  bool current_immediate = current->info.function.is_consteval;
  bool previous_constexpr =
      previous->info.function.is_constexpr && !previous_immediate;
  bool current_constexpr =
      current->info.function.is_constexpr && !current_immediate;
  if (previous_immediate != current_immediate ||
      previous_constexpr != current_constexpr) {
    SyntaxError(syntax,
                "'constexpr' and 'consteval' specifiers must match previous "
                "declaration");
  }
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
      Storage compatibility_mask = STO(thread);
      if (CompilerCAtLeast(kLanguageStandardC23) &&
          StorageIs(new | old, STO(auto))) {
        compatibility_mask |= STO(auto);
      }
      if (!IsPowerOf2OrZero((new | old) & ~compatibility_mask) ||
          (CompilerCAtLeast(kLanguageStandardC23) &&
           StorageIs(new | old, STO(auto)) &&
           StorageIs(new | old, STO(typedef)))) {
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
    } else if (LexMatch(syntax->lex, TOK(noreturn))) {
      if (AttributeListHas(attributes, "noreturn")) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate '_Noreturn' specifier");
      } else {
        VectorAppend(attributes, NewAttribute("noreturn"));
      }
      if (CompilerCAtLeast(kLanguageStandardC23)) {
        SyntaxWarning(syntax, "deprecated-declarations",
                      "'_Noreturn' is deprecated in C23; use [[noreturn]]");
      }
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
      if (*is_consteval) {
        SyntaxError(syntax,
                    "'constexpr' and 'consteval' cannot both be specified");
      }
      if (*is_constexpr) {
        SyntaxError(syntax, "Duplicate 'constexpr' specifier");
      }
      if (*is_constinit) {
        SyntaxError(syntax, "'constexpr' and 'constinit' cannot be combined");
      }
      *is_constexpr = true;
    } else if (LexMatch(syntax->lex, TOK(consteval))) {
      if (*is_consteval) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate 'consteval' specifier");
      } else if (*is_constexpr) {
        SyntaxError(syntax,
                    "'constexpr' and 'consteval' cannot both be specified");
      }
      if (*is_constinit) {
        SyntaxError(syntax, "'consteval' and 'constinit' cannot be combined");
      }
      *is_consteval = true;
      *is_constexpr = true;
    } else if (LexMatch(syntax->lex, TOK(constinit))) {
      if (*is_constinit) {
        SyntaxWarning(syntax, "duplicate-decl-specifier",
                      "Duplicate 'constinit' specifier");
      }
      if (*is_constexpr || *is_consteval) {
        SyntaxError(syntax, "'constexpr' and 'constinit' cannot be combined");
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
    } else if (SyntaxParseAnyAttribute(syntax, attributes)) {
      continue;
    } else {
      if (type_specifier.type == kTypeImplicit &&
          type_specifier.type_record == NULL) {
        if (CompilerCAtLeast(kLanguageStandardC23) &&
            StorageIs(*storage, STO(auto))) {
          *type = NewTypeRecordWithSize(kTypeAuto, type_specifier.quals);
          *storage &= ~STO(auto);
        } else if (CompilerCAtLeast(kLanguageStandardC23)) {
          SyntaxError(syntax, "type specifier missing in declaration");
          *type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
        } else {
          SyntaxWarning(syntax, "implicit-int",
                        "type specifier missing, defaults to int");
          *type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
        }
      } else {
        *type = TypeParserBuildTypeRecord(&parser, &type_specifier);
        if (CompilerCAtLeast(kLanguageStandardC23) &&
            StorageIs(*storage, STO(auto)) &&
            !IsPowerOf2OrZero(*storage & ~STO(thread))) {
          SyntaxError(syntax,
                      "Multiple incompatible storage specifiers");
        }
      }
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
static void MaterializeDeferredClassTemplateType(Syntax* syntax, Symbol* sym);
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
  if (syntax->current_template_requires_clause != NULL) {
    AddFunctionAssociatedConstraint(
        sym->type, syntax->current_template_requires_clause);
    syntax->current_template_requires_clause = NULL;
  }
  TypeAddCXXDeductionGuide(class_template, sym);
  return true;
}

static bool IsC23InferredAutoType(TypeRecord* type) {
  return !CompilerIsCXX() && CompilerCAtLeast(kLanguageStandardC23) &&
         type != NULL && TypeContainsAuto(type);
}

typedef struct {
  Symbol* symbol;
  bool found;
} C23AutoSelfReference;

static void FindC23AutoSelfReference(ASTNode* node, void* data, int child_id,
                                    VisitorMode mode) {
  (void)child_id;
  C23AutoSelfReference* search = data;
  if (mode == kVisitPreChildren && node != NULL &&
      node->op == AST_OP(identifier) &&
      ((IdentifierASTNode*)node)->symbol == search->symbol) {
    search->found = true;
  }
}

static void ValidateC23AutoDeclarator(Syntax* syntax, Symbol* sym) {
  if (sym == NULL || !IsC23InferredAutoType(sym->type)) {
    return;
  }
  if (sym->type->declarator != kDeclPrimitive ||
      (sym->type->type & kTypeAuto) == 0) {
    SyntaxError(syntax,
                "C23 inferred auto requires a simple object declarator");
  }
  if (TypeIsFunction(sym->type)) {
    SyntaxError(syntax, "C23 auto cannot infer a function type");
  }
}

static void ValidateC23AutoInitializer(Syntax* syntax, Symbol* sym,
                                       ASTNode* initializer) {
  if (sym == NULL || !IsC23InferredAutoType(sym->type)) {
    return;
  }
  if (initializer == NULL) {
    SyntaxError(syntax, "C23 inferred auto requires an initializer");
    return;
  }
  ASTNode* deduction_initializer = initializer;
  if (deduction_initializer->op == AST_OP(init)) {
    deduction_initializer = ((BinaryASTNode*)deduction_initializer)->right;
  }
  if (deduction_initializer == NULL ||
      deduction_initializer->op != AST_OP(expr_init)) {
    SyntaxError(syntax,
                "C23 inferred auto requires an assignment-expression initializer");
  }
  C23AutoSelfReference search = {.symbol = sym, .found = false};
  ASTNodeVisit(deduction_initializer, FindC23AutoSelfReference, 0, &search);
  if (search.found) {
    SyntaxError(syntax,
                "C23 inferred auto initializer cannot refer to itself");
  }
}

static bool C23ConstexprTypeContainsVLA(TypeRecord* type) {
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (TypeIsVLA(current)) {
      return true;
    }
  }
  return false;
}

static bool C23ConstexprObjectTypeIsAllowed(TypeRecord* type) {
  if (type == NULL || C23ConstexprTypeContainsVLA(type) ||
      (type->qualifiers &
       (kQualAtomic | kQualVolatile | kQualRestrict)) != 0) {
    return false;
  }
  if (TypeIsPointer(type)) {
    return true;
  }
  if (TypeIsArray(type)) {
    return C23ConstexprObjectTypeIsAllowed(type->next);
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* aggregate = type->info.struct_info;
    for (size_t i = 0; i < aggregate->members.length; i++) {
      StructMember* member = aggregate->members.value.p[i];
      if (member != NULL && member->symbol != NULL &&
          !member->is_static && !member->is_member_function &&
          !C23ConstexprObjectTypeIsAllowed(member->symbol->type)) {
        return false;
      }
    }
  }
  return true;
}

static bool C23InitializerIsNullPointer(ASTNode* initializer) {
  if (initializer == NULL) {
    return false;
  }
  if (initializer->op == AST_OP(expr_init)) {
    return C23InitializerIsNullPointer(
        ((ExpressionInitializerASTNode*)initializer)->expr);
  }
  if (initializer->op == AST_OP(designated_init)) {
    return C23InitializerIsNullPointer(
        ((DesignatedInitializerASTNode*)initializer)->init);
  }
  if (initializer->op == AST_OP(cast)) {
    return C23InitializerIsNullPointer(((CastASTNode*)initializer)->expr);
  }
  return TypeIsNullPointer(initializer->type) ||
         (initializer->op == AST_OP(number) &&
          ((ConstantASTNode*)initializer)->value.ivalue == 0);
}

static bool C23ConstexprTypeHasPointerSubobject(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsPointer(type)) {
    return true;
  }
  if (TypeIsArray(type)) {
    return C23ConstexprTypeHasPointerSubobject(type->next);
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* aggregate = type->info.struct_info;
    for (size_t i = 0; i < aggregate->members.length; i++) {
      StructMember* member = aggregate->members.value.p[i];
      if (member != NULL && member->symbol != NULL &&
          !member->is_static && !member->is_member_function &&
          C23ConstexprTypeHasPointerSubobject(member->symbol->type)) {
        return true;
      }
    }
  }
  return false;
}

static bool C23ConstexprPointerSubobjectsAreNull(TypeRecord* type,
                                                 ASTNode* initializer) {
  if (!C23ConstexprTypeHasPointerSubobject(type)) {
    return true;
  }
  if (initializer != NULL && initializer->op == AST_OP(designated_init)) {
    DesignatedInitializerASTNode* designated =
        (DesignatedInitializerASTNode*)initializer;
    TypeRecord* selected = type;
    for (size_t i = 0;
         selected != NULL && designated->designators != NULL &&
         i < designated->designators->length; i++) {
      Designator* designator = designated->designators->value.p[i];
      if (designator->designator_type == kDesignatorArray &&
          TypeIsArray(selected)) {
        selected = selected->next;
      } else if (designator->designator_type == kDesignatorStruct &&
                 TypeIsStructOrUnion(selected)) {
        StructMember* member =
            designator->is_resolved_member
                ? designator->value.struct_member
                : FindStructMemberByName(
                      selected->info.struct_info,
                      designator->value.struct_member_name->value);
        selected =
            member != NULL && member->symbol != NULL ? member->symbol->type
                                                      : NULL;
      } else {
        selected = NULL;
      }
    }
    return selected != NULL &&
           C23ConstexprPointerSubobjectsAreNull(selected, designated->init);
  }
  if (TypeIsPointer(type)) {
    return C23InitializerIsNullPointer(initializer);
  }
  if (initializer != NULL && initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer == NULL || initializer->op != AST_OP(braced_init)) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
  if (TypeIsArray(type)) {
    for (size_t i = 0; i < braced->initializers->length; i++) {
      if (!C23ConstexprPointerSubobjectsAreNull(
              type->next, braced->initializers->value.p[i])) {
        return false;
      }
    }
    return true;
  }
  Struct* aggregate = type->info.struct_info;
  size_t initializer_index = 0;
  for (size_t i = 0; i < aggregate->members.length; i++) {
    StructMember* member = aggregate->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function) {
      continue;
    }
    ASTNode* member_initializer =
        initializer_index < braced->initializers->length
            ? braced->initializers->value.p[initializer_index]
            : NULL;
    if (member_initializer == NULL) {
      // Omitted aggregate members are zero-initialized.
      initializer_index++;
      continue;
    }
    if (member_initializer->op == AST_OP(designated_init)) {
      if (!C23ConstexprPointerSubobjectsAreNull(type, member_initializer)) {
        return false;
      }
    } else if (!C23ConstexprPointerSubobjectsAreNull(member->symbol->type,
                                                     member_initializer)) {
      return false;
    }
    initializer_index++;
    if (aggregate->is_union) {
      break;
    }
  }
  return true;
}

static bool C23ConstexprIntegerConstantIsRepresentable(TypeRecord* type,
                                                       ASTNode* initializer) {
  if (type == NULL || !TypeIsIntegral(type) || initializer == NULL) {
    return true;
  }
  if (initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer != NULL && initializer->op == AST_OP(braced_init)) {
    BracedInitializerASTNode* braced = (BracedInitializerASTNode*)initializer;
    if (braced->initializers->length != 1) {
      return true;
    }
    initializer = braced->initializers->value.p[0];
    if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
      initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
    }
  }
  if (initializer == NULL) {
    return true;
  }
  initializer = AnalyzeExpression(initializer);
  int64_t value = 0;
  if (!EvaluateIntegerExpression(initializer, &value)) {
    return true;
  }
  if (TypeIsBool(type)) {
    return value == 0 || value == 1;
  }
  int bits = TypeIsBitInt(type) ? type->bit_width : type->size * 8;
  if (bits <= 0 || bits >= 64) {
    return !TypeIsUnsigned(type) || value >= 0;
  }
  if (TypeIsUnsigned(type)) {
    uint64_t maximum = (UINT64_C(1) << bits) - 1;
    return value >= 0 && (uint64_t)value <= maximum;
  }
  int64_t minimum = -(INT64_C(1) << (bits - 1));
  int64_t maximum = (INT64_C(1) << (bits - 1)) - 1;
  return value >= minimum && value <= maximum;
}

static bool C23ConstexprFloatingConstantIsRepresentable(TypeRecord* type,
                                                        ASTNode* initializer) {
  if (type == NULL || !TypeIsFloatingPoint(type) || initializer == NULL ||
      !TypeIsFloat(type)) {
    return true;
  }
  if (initializer->op == AST_OP(init)) {
    initializer = ((BinaryASTNode*)initializer)->right;
  }
  if (initializer != NULL && initializer->op == AST_OP(expr_init)) {
    initializer = ((ExpressionInitializerASTNode*)initializer)->expr;
  }
  if (initializer == NULL) {
    return true;
  }
  initializer = AnalyzeExpression(initializer);
  double value = 0;
  if (TypeIsIntegral(initializer->type)) {
    int64_t integer_value = 0;
    if (!EvaluateIntegerExpression(initializer, &integer_value)) {
      return true;
    }
    value = (double)integer_value;
  } else if (!EvaluateFloatingPointExpression(initializer, &value)) {
    return true;
  }
  float narrowed = (float)value;
  return (double)narrowed == value;
}

static void ValidateC23ConstexprObject(Syntax* syntax, Symbol* sym,
                                       ASTNode* initializer) {
  if (CompilerIsCXX() || !CompilerCAtLeast(kLanguageStandardC23) ||
      sym == NULL || !sym->flags.is_constexpr || TypeIsFunction(sym->type)) {
    return;
  }
  if (!C23ConstexprObjectTypeIsAllowed(sym->type)) {
    SyntaxError(
        syntax,
        "C constexpr object cannot have variably modified, atomic, volatile, "
        "or restrict-qualified type");
  }
  if (initializer != NULL &&
      !C23ConstexprPointerSubobjectsAreNull(sym->type, initializer)) {
    SyntaxError(syntax,
                "C constexpr pointer subobjects must be initialized to null");
  }
  if (!C23ConstexprIntegerConstantIsRepresentable(sym->type, initializer)) {
    SyntaxError(
        syntax,
        "C constexpr initializer is not exactly representable in its type");
  }
  if (!C23ConstexprFloatingConstantIsRepresentable(sym->type, initializer)) {
    SyntaxError(
        syntax,
        "C constexpr initializer is not exactly representable in its type");
  }
}

static bool IsGlobalMainSymbol(Symbol* sym) {
  if (sym == NULL || !StringEqual(&sym->name, "main") ||
      sym->namespace_ != NULL) {
    return false;
  }
  return !TypeIsFunction(sym->type) ||
         sym->type->info.function.cxx_member_owner == NULL;
}

static void ApplyExplicitCXXLinkageAttachment(Syntax* syntax, Symbol* sym,
                                              Storage storage) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX20) ||
      !syntax->explicit_cxx_linkage || sym == NULL) {
    return;
  }
  StringClear(&sym->owning_module_name);
  StringClear(&sym->owning_module_partition);
  sym->flags.is_module_private = false;
  sym->cxx_linkage = StorageIs(storage, STO(static)) ? kCXXLinkageInternal
                                                      : kCXXLinkageExternal;
}

static void ValidateCXX26MainDeclaration(Syntax* syntax, Symbol* sym,
                                         Storage storage) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX26) || sym == NULL ||
      !StringEqual(&sym->name, "main")) {
    return;
  }
  if (sym->flags.is_c_linkage) {
    SyntaxError(syntax, "an entity named 'main' cannot have C language linkage");
    return;
  }
  if (!IsGlobalMainSymbol(sym)) {
    return;
  }
  if (!TypeIsFunction(sym->type)) {
    SyntaxError(syntax, "a variable named 'main' cannot belong to global scope");
    return;
  }
  FunctionInfo* info = &sym->type->info.function;
  if (sym->flags.is_template) {
    SyntaxError(syntax, "'main' cannot be a function template");
  }
  if (StorageIs(storage, STO(static))) {
    SyntaxError(syntax, "'main' cannot be declared static");
  }
  if (info->is_inline) {
    SyntaxError(syntax, "'main' cannot be declared inline");
  }
  if (info->is_constexpr) {
    SyntaxError(syntax, "'main' cannot be declared constexpr");
  }
  if (info->is_consteval) {
    SyntaxError(syntax, "'main' cannot be declared consteval");
  }
  if (sym->owning_module_name.length != 0) {
    SyntaxError(syntax, "'main' cannot be attached to a named module");
  }
}

static void ValidateCXX26DeletedMain(Syntax* syntax, Symbol* sym) {
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      IsGlobalMainSymbol(sym) && TypeIsFunction(sym->type) &&
      sym->type->info.function.is_deleted) {
    SyntaxError(syntax, "'main' cannot be defined as deleted");
  }
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
    ValidateC23AutoDeclarator(syntax, sym);
    // A variable-template specialization: `template<...> T name<pattern> = ...`.
    // The declarator carried a template-argument list and an existing variable
    // template of the same name is in scope.  Register it as a partial/explicit
    // specialization rather than treating the reused name as a redefinition.
    if (CompilerIsCXX() && sym != NULL && !TypeIsFunction(sym->type) &&
        parser->declarator_template_arguments != NULL &&
        (syntax->parsing_template_declaration ||
         syntax->parsing_template_specialization)) {
      Symbol* primary = SyntaxFindSymbol(syntax, &sym->name);
      if (primary != NULL && primary->variable_template != NULL) {
        ASTNode* initializer = NULL;
        if (LexMatch(syntax->lex, TOK(equal))) {
          syntax->init_storage = storage;
          initializer = SyntaxParseInitializer(syntax, sym, storage);
        } else {
          SyntaxError(syntax,
                      "variable template specialization requires an initializer");
        }
        AddVariableTemplatePartialSpecialization(
            parser, primary, parser->declarator_template_arguments, initializer,
            sym->type);
        SymbolDelete(sym);
        TypeParserReset(parser);
        if (LexMatch(syntax->lex, TOK(comma))) {
          continue;
        }
        break;
      }
    }
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
      if (syntax->c_linkage) {
        sym->flags.is_c_linkage = true;
      }
      if (syntax->export_depth > 0) {
        sym->flags.is_exported = true;
      }
      SymbolAttachModuleContext(sym, storage);
      ApplyExplicitCXXLinkageAttachment(syntax, sym, storage);
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
      ValidateCXX26MainDeclaration(syntax, sym, storage);
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
          : NULL;
      if (parser->cxx_member_definition == NULL) {
        Symbol* raw_old = FindFileScopeSymbol(syntax, &sym->name);
        old_sym = SymbolFindModuleCompatibleOverload(raw_old, sym);
        if (raw_old != NULL && old_sym == NULL) {
          SyntaxError(syntax,
                      "Declaration of '%s' conflicts with an unrelated module "
                      "entity",
                      sym->name.value);
        }
      }
      if (IsC23InferredAutoType(sym->type) && old_sym != NULL) {
        SyntaxError(syntax,
                    "C23 inferred auto declaration cannot redeclare '%s'",
                    sym->name.value);
      }
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
        ConstraintExpr* saved_constraint =
            sym->type->info.function.associated_constraint;
        ConstraintExpr* comparison_constraint =
            PendingFunctionTemplateConstraint(
                sym, PendingTemplateParameterList(syntax, sym),
                syntax->current_template_requires_clause);
        if (comparison_constraint != NULL) {
          sym->type->info.function.associated_constraint =
              comparison_constraint;
        }
        if (TryAppendSameSignatureConstrainedTemplateOverload(
                syntax, old_sym, sym, NULL)) {
          old_sym = NULL;
          overload_was_appended = true;
        } else {
          Symbol* matching_overload = FindMatchingOverload(old_sym, sym->type, sym);
          if (matching_overload != NULL) {
            old_sym = matching_overload;
          } else {
          AppendOverload(old_sym, sym);
          old_sym = NULL;
          overload_was_appended = true;
          }
        }
        sym->type->info.function.associated_constraint = saved_constraint;
        ConstraintExprDelete(comparison_constraint);
      }
      if (parser->cxx_member_definition != NULL &&
          !syntax->parsing_template_specialization) {
        StructMember* matching_member = FindStructMemberOverload(
            parser->cxx_member_definition, sym->type);
        if (matching_member == NULL) {
          for (StructMember* candidate = parser->cxx_member_definition;
               candidate != NULL; candidate = candidate->overload_next) {
            if (candidate->symbol != NULL &&
                RedeclarationTypesEqual(candidate->symbol->type, sym->type)) {
              matching_member = candidate;
              break;
            }
          }
        }
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
          MergeCXXContractAssertions(syntax, old_sym, sym);
          CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
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
          if (!inserted) {
            SyntaxError(syntax, "Duplicate symbol %s",
                        sym->name.value);
          }
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

    // Parse common __attribute__ / __declspec / C++ attribute syntax.
    while (true) {
      if (SyntaxParseCXXAlignas(syntax, attributes)) {
        continue;
      }
      if (SyntaxParseAnyAttribute(syntax, attributes)) {
        continue;
      }
      break;
    }
    
    VectorAppendVector(&sym->attributes, attributes);
    VectorClear(attributes);
    SyntaxApplyDeclarationAttributes(syntax, sym);
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
      if (sym->flags.is_module_private && !old_sym->flags.is_exported) {
        old_sym->flags.is_module_private = true;
      }
      if (sym->flags.noreturn) {
        old_sym->flags.noreturn = true;
        if (!AttributeListHas(&old_sym->attributes, "noreturn")) {
          Attribute* noreturn =
              AttributeListFind(&sym->attributes, "noreturn");
          if (noreturn != NULL) {
            VectorAppend(&old_sym->attributes, AttributeClone(noreturn));
          }
        }
      }
      if (sym->asm_name.length != 0) {
        StringSetString(&old_sym->asm_name, &sym->asm_name);
      } else if (old_sym->asm_name.length != 0) {
        StringSetString(&sym->asm_name, &old_sym->asm_name);
      }
    }
    if (TypeIsFunction(sym->type)) {
      ParseCXXDefaultDeleteFunctionSpecifier(syntax, sym->type,
                                             old_sym == NULL);
      ValidateCXX26DeletedMain(syntax, sym);
      if (old_sym != NULL && TypeIsFunction(old_sym->type)) {
        if (sym->type->info.function.is_defaulted) {
          old_sym->type->info.function.is_defaulted = true;
        }
        if (sym->type->info.function.is_deleted) {
          old_sym->type->info.function.is_deleted = true;
          old_sym->type->info.function.is_explicitly_deleted =
              sym->type->info.function.is_explicitly_deleted;
          if (old_sym->type->info.function.deleted_reason != NULL) {
            StringDelete(old_sym->type->info.function.deleted_reason);
          }
          old_sym->type->info.function.deleted_reason =
              sym->type->info.function.deleted_reason != NULL
                  ? NewStringWithLength(
                        sym->type->info.function.deleted_reason->value,
                        sym->type->info.function.deleted_reason->length)
                  : NULL;
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
      if (!CompilerIsCXX() && parser->is_constexpr) {
        SyntaxError(syntax, "C constexpr functions are not supported");
      }
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
    // A type alias formed while its class template was only forward-declared
    // carries a *deferred* template-id (see the forward-declaration path in
    // InstantiateSimpleClassTemplateImpl): the primary's members were not yet
    // known, so the specialization was recorded but not materialized.  A
    // variable declaration requires a complete type, so materialize the
    // specialization now that the definition is available, instead of rejecting
    // the still-"template" type in the guard below.
    MaterializeDeferredClassTemplateType(syntax, sym);
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
    if (TypeIsFunction(sym->type)) {
      if (parser->is_constinit) {
        SyntaxError(syntax, "'constinit' cannot be applied to a function");
      }
    } else {
      sym->flags.is_constexpr = parser->is_constexpr;
      sym->flags.is_constinit = parser->is_constinit;
      DiagnoseConstevalOnNonFunction(syntax, sym->type, parser->is_consteval);
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
    ValidateC23AutoInitializer(syntax, sym, initializer);
    ValidateC23ConstexprObject(syntax, sym, initializer);
    if (TypeContainsAuto(sym->type) && initializer == NULL) {
      SyntaxError(syntax, "auto variable requires an initializer");
    } else if (TypeContainsAuto(sym->type)) {
      initializer = AnalyzeExpression(initializer);
      SemanticDeduceAutoType(sym, initializer, (ASTNode*)initializer);
      if (sym->associated_constraint != NULL && sym->type != NULL) {
        Vector* constraint_args = NewVector();
        TemplateArgument* type_arg = TemplateArgumentAlloc();
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
    if (CompilerCXXAtLeast(kLanguageStandardCXX17) &&
        !syntax->parsing_template_declaration &&
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

    if (IsC23InferredAutoType(type) &&
        LexLookingAt(syntax->lex, TOK(comma))) {
      SyntaxError(syntax,
                  "C23 inferred auto declaration must contain one declarator");
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
  Symbol* alias =
      target != NULL && target->type != NULL && TypeIsFunction(target->type)
          ? SymbolClone(target)
          : NewSymbol(name, NULL,
                      target != NULL ? target->storage : STO(implicit));
  alias->flags.is_using_alias = true;
  alias->alias_target = target;
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
    Symbol* new_target = alias->alias_target != NULL ? FollowAlias(alias->alias_target)
                                                     : NULL;
    if (existing_target != NULL && existing_target == new_target) {
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
                /*is_tag=*/false, /*current_scope_only=*/false);
}

static void ImportNamespaceTag(BinaryTreeNode* node, int depth, void* data) {
  (void)depth;
  Syntax* syntax = data;
  Symbol* target = ((SymbolNode*)node)->symbol;
  AddUsingAlias(syntax, NewUsingAliasSymbol(target->name.value, target,
                                            syntax->lex->current_token_location),
                /*is_tag=*/true, /*current_scope_only=*/false);
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
  NamespaceAliasInsertResult result = NamespaceInsertAlias(scope, name, target);
  if (result == kNamespaceAliasInserted && syntax->export_depth > 0 &&
      scope->namespace_aliases.length > 0) {
    NamespaceAlias* alias = (NamespaceAlias*)VectorGet(
        &scope->namespace_aliases, scope->namespace_aliases.length - 1);
    alias->is_exported = true;
  }
  return result;
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
    MergeCXXContractAssertions(syntax, old_sym, sym);
    CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
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
  Symbol* defining_template =
      LexLookingAt(syntax->lex, TOK(identifier))
          ? SyntaxFindSymbol(syntax, &syntax->lex->spelling)
          : NULL;
  bool ctad_names_template_template_parameter =
      defining_template != NULL &&
      (defining_template->flags.is_template_template_parameter ||
       (defining_template->alias_template != NULL &&
        defining_template->alias_template
            ->ctad_names_template_template_parameter));
  TypeParser parser;
  TypeParserInit(&parser, syntax->lex, syntax, STO(implicit), syntax->context);
  TypeRecord* type = TypeParserParseType(&parser, true);
  Symbol* parsed = TypeParserParseDeclarator(&parser, type);
  if (parsed != NULL) {
    SyntaxApplyDeclarationAttributes(syntax, parsed);
  }
  TypeParserDestruct(&parser);

  TypeRecord* alias_type = parsed != NULL ? parsed->type : type;
  for (TypeRecord* part = alias_type; part != NULL; part = part->next) {
    if (TypeIsFunction(part) &&
        part->info.function.contract_assertions.length != 0) {
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
      alias->alias_template->ctad_names_template_template_parameter = false;
    }
    alias->alias_template->ctad_names_template_template_parameter =
        ctad_names_template_template_parameter;
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
                    /*is_tag=*/false, /*current_scope_only=*/true);
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

static bool SymbolNamesTemplateArgument(Symbol* symbol);

static bool ExpressionContainsDependentTemplateParameter(ASTNode* node);
static bool ExpressionIsNonTypeTemplateParameter(ASTNode* node,
                                                 int* parameter_index);

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
    param->template_template_parameter_kind = template_kind;
    param->template_template_parameters =
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
  syntax->current_template_parameters = params;
  syntax->current_template_parameter_count = base;
  while (!LexEof(lex) && !LexLookingAtClosingAngle(lex)) {
    ParseTemplateParameter(syntax, params, base);
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
      TypeIsUnknown(node->type) ||
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
  bool followed_by_braced_initializer = LexLookingAt(syntax->lex, TOK(lbrace));
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
    return resolved && resolved_type && !followed_by_braced_initializer;
  }
  return !followed_by_braced_initializer;
}

static bool SymbolNamesTemplateArgument(Symbol* symbol) {
  if (symbol == NULL) {
    return false;
  }
  if (symbol->flags.is_template_template_parameter ||
      symbol->alias_template != NULL || symbol->variable_template != NULL ||
      symbol->flags.is_concept) {
    return true;
  }
  return symbol->flags.is_template && symbol->type != NULL &&
         TypeIsStructOrUnion(symbol->type) &&
         symbol->type->info.struct_info != NULL &&
         symbol->type->info.struct_info->is_template;
}

static Symbol* SyntaxBareTemplateArgumentSymbol(Syntax* syntax) {
  Token tok = syntax->lex->current_token;
  if (tok != TOK(identifier) && tok != TOK(coloncolon)) {
    return NULL;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  FullyQualifiedIdentifier name;
  FullyQualifiedIdentifierInit(&name);
  SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
      syntax, &name, TC(closebra) | TC(exprsep));
  bool has_template_id = false;
  for (size_t i = 0; i < name.template_arguments.length; i++) {
    if (name.template_arguments.value.p[i] != NULL) {
      has_template_id = true;
      break;
    }
  }
  Symbol* symbol = NULL;
  if (!has_template_id) {
    if (!name.is_qualified && !name.absolute) {
      symbol = FindLocalSymbol(syntax->local_symbol_stack, &name.spelling);
      if (symbol == NULL) {
        symbol = FindGlobalSymbol(&name.spelling);
      }
    }
    if (symbol == NULL) {
      symbol = SyntaxFindQualifiedSymbol(syntax, &name);
      if (!SymbolNamesTemplateArgument(symbol)) {
        symbol = SyntaxFindQualifiedTag(syntax, &name);
      }
    }
    if (!SymbolNamesTemplateArgument(symbol)) {
      symbol = NULL;
    }
  }
  FullyQualifiedIdentifierDestruct(&name);
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return symbol;
}

static void NormalizeTemplateNameArgument(TemplateArgument* arg,
                                          Symbol* bare_template) {
  if (arg == NULL || arg->kind != kTemplateParameterType || arg->type == NULL) {
    return;
  }
  if (!SymbolNamesTemplateArgument(bare_template)) {
    return;
  }
  Symbol* origin = bare_template;
  ASTNode* pack_index_expr =
      arg->type->is_pack_index
          ? ASTNodeClone(arg->type->pack_index_expr, IdentityCloneNode, NULL,
                         NULL)
          : NULL;
  TypeRecordDelete(arg->type);
  arg->type = NULL;
  arg->kind = kTemplateParameterTemplate;
  arg->template_symbol = origin;
  arg->pack_index_expr = pack_index_expr;
  arg->template_parameter_index =
      origin->flags.is_template_template_parameter
          ? origin->template_parameter_index : -1;
}

Vector* SyntaxParseTemplateArgumentList(Syntax* syntax, TokenClass followers) {
  Lex* lex = syntax->lex;
  if (!LexMatch(lex, TOK(less))) {
    return NULL;
  }
  Vector* args = NewVector();
  while (!LexEof(lex) && !LexLookingAtClosingAngle(lex)) {
    TemplateArgument* arg = TemplateArgumentAlloc();
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
    Symbol* bare_template = SyntaxBareTemplateArgumentSymbol(syntax);
    bool non_type_template_name =
        bare_template != NULL &&
        (bare_template->flags.is_concept ||
         bare_template->variable_template != NULL ||
         (bare_template->flags.is_template_template_parameter &&
          bare_template->template_template_parameter_kind !=
              kTemplateTemplateParameterType));
    if (non_type_template_name) {
      FullyQualifiedIdentifier template_name;
      FullyQualifiedIdentifierInit(&template_name);
      SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
          syntax, &template_name, TC(closebra) | TC(exprsep));
      FullyQualifiedIdentifierDestruct(&template_name);
      arg->kind = kTemplateParameterTemplate;
      arg->template_symbol = bare_template;
      arg->template_parameter_index =
          bare_template->flags.is_template_template_parameter
              ? bare_template->template_parameter_index
              : -1;
      if (LexMatch(lex, TOK(ellipsis))) {
        if (LexMatch(lex, TOK(lsquare))) {
          if (!CompilerCXXAtLeast(kLanguageStandardCXX29)) {
            SyntaxError(syntax,
                        "Template-name pack indexing requires C++29");
          }
          arg->pack_index_expr =
              SyntaxParseSingleExpression(syntax, TC(closebra));
          SyntaxNeedBracket(syntax, TOK(rsquare),
                            TC(closebra) | TC(exprsep));
        } else {
          arg->is_pack_expansion = true;
        }
      }
      arg->references_parameter_pack =
          arg->is_pack_expansion || arg->pack_index_expr != NULL ||
          bare_template->flags.is_parameter_pack;
    } else if (SyntaxTemplateArgumentLooksLikeType(syntax)) {
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
        if (CompilerIsCXX() && LexMatch(lex, TOK(ellipsis))) {
          arg->is_pack_expansion = true;
        }
      }
      arg->references_parameter_pack =
          CXXTypeContainsParameterPack(syntax, arg->type);
      NormalizeTemplateNameArgument(arg, bare_template);
    } else {
      bool old_parsing_template_argument = syntax->parsing_template_argument;
      syntax->parsing_template_argument = true;
      ASTNode* expr = SyntaxParseSingleExpression(syntax,
                                                  TC(closebra) | TC(exprsep));
      syntax->parsing_template_argument = old_parsing_template_argument;
      arg->kind = kTemplateParameterNonType;
      arg->references_parameter_pack =
          CXXExpressionContainsParameterPack(expr);
      int direct_template_parameter_index = -1;
      bool is_direct_nttp =
          ExpressionIsNonTypeTemplateParameter(expr,
                                               &direct_template_parameter_index);
      if (is_direct_nttp) {
        for (size_t i = 0;
             syntax->current_template_parameters != NULL &&
             i < syntax->current_template_parameters->length; i++) {
          TemplateParameter* parameter =
              syntax->current_template_parameters->value.p[i];
          if (parameter != NULL &&
              parameter->index == direct_template_parameter_index &&
              parameter->is_parameter_pack) {
            arg->references_parameter_pack = true;
            break;
          }
        }
      }
      // A template argument can only be value-dependent when template parameters
      // are in scope.  Outside one it must be a concrete constant expression, so
      // it has to be folded now.  The dependence probe below inspects the
      // still-unanalysed expression, where every interior node (a `+`, a call,
      // ...) has a null/unknown type and so is spuriously reported as dependent;
      // gating on the parse state avoids deferring -- and thus dropping to 0 --
      // such non-dependent arguments.  Parameters are also in scope for the rest
      // of their own list, which is where the pre-C++20 constraint idiom writes
      // its condition (`typename enable_if<C<T>, int>::type = 0`); the
      // declaration flag is not set until the list has been read, so ask the
      // parameter count as well.
      bool template_parameters_in_scope =
          syntax->parsing_template_declaration ||
          syntax->current_template_parameter_count > 0;
      if (template_parameters_in_scope && !is_direct_nttp &&
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
        compiler->constant_evaluation_required_depth++;
        expr = AnalyzeExpression(expr);
        compiler->constant_evaluation_required_depth--;
        bool value_ok = false;
        if (arg->template_parameter_index >= 0) {
          arg->type = expr != NULL && expr->type != NULL
                          ? TypeRecordCopy(expr->type) : NULL;
          value_ok = true;
        } else {
          value_ok = TemplateArgumentSetFromExpression(arg, expr);
        }
        if (!value_ok) {
          if (template_parameters_in_scope &&
              expr->op == AST_OP(identifier)) {
            IdentifierASTNode* id = (IdentifierASTNode*)expr;
            if (id->symbol != NULL && id->symbol->flags.is_template_parameter &&
                !id->symbol->flags.is_template_type_parameter) {
              arg->template_parameter_index =
                  id->symbol->template_parameter_index;
            }
          }
          if (arg->template_parameter_index < 0) {
            if (template_parameters_in_scope) {
              arg->dependent_expr = expr;
              arg->is_pack_expansion = LexMatch(lex, TOK(ellipsis));
              VectorAppend(args, arg);
              if (!LexMatch(lex, TOK(comma))) {
                break;
              }
              continue;
            } else {
              SyntaxError(syntax,
                          "Template non-type argument must be an integer constant "
                          "expression, or a pointer/member-pointer argument must "
                          "be a constant expression");
            }
          }
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

// A type alias formed while its class template was only forward-declared carries
// a *deferred* template-id (see the forward-declaration path in
// InstantiateSimpleClassTemplateImpl): the primary's members were not yet known,
// so the specialization was recorded but not materialized.  A variable
// declaration requires a complete type, so materialize the specialization now
// that the definition is available, instead of rejecting the still-"template"
// type in the guards that follow.
static void MaterializeDeferredClassTemplateType(Syntax* syntax, Symbol* sym) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      syntax->parsing_template_declaration ||
      !TypeContainsClassTemplate(sym->type) ||
      TypeIsClassTemplatePlaceholder(sym->type)) {
    return;
  }
  TypeRecord* materialized =
      TypeMaterializeClassTemplateSpecialization(syntax, sym->type);
  if (materialized != sym->type) {
    SymbolSetType(sym, materialized);
    TypeRecordDelete(materialized);
  }
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

static ASTNode* ParseTemplateDeclaration(Syntax* syntax) {
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
  if (!LexLookingAtStringLiteral(syntax->lex)) {
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return NULL;
  }
  LexCheckpointDestruct(&checkpoint);

  String linkage;
  StringInit(&linkage, "");
  while (LexLookingAtStringLiteral(syntax->lex)) {
    LexValidateUnevaluatedString(syntax->lex, "linkage specification",
                                 /*allow_user_defined_suffix=*/false);
    StringAppend(&linkage, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  bool is_c = StringEqual(&linkage, "C");
  bool is_cpp = StringEqual(&linkage, "C++");
  if (!is_c && !is_cpp) {
    SyntaxError(syntax, "Unknown linkage specification \"%s\"", linkage.value);
  }
  StringDestruct(&linkage);

  // A nested linkage specification overrides the surrounding linkage while
  // its declaration sequence is parsed.
  bool saved_c_linkage = syntax->c_linkage;
  bool saved_explicit_cxx_linkage = syntax->explicit_cxx_linkage;

  if (LexMatch(syntax->lex, TOK(lbrace))) {
    Vector* declarations = NewVector();
    syntax->c_linkage = is_c;
    syntax->explicit_cxx_linkage = is_cpp;
    while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rbrace))) {
      ASTNode* node = SyntaxParseExternalDeclaration(syntax);
      AppendDeclarationsFromNode(declarations, node);
    }
    syntax->c_linkage = saved_c_linkage;
    syntax->explicit_cxx_linkage = saved_explicit_cxx_linkage;
    SyntaxNeedBracket(syntax, TOK(rbrace), TC(closebrace) | TC(decl));
    return NewDeclarationListASTNode(declarations, location);
  }

  // Single-declaration form: `extern "C" <declaration>`.
  syntax->c_linkage = is_c;
  syntax->explicit_cxx_linkage = is_cpp;
  ASTNode* node = SyntaxParseExternalDeclaration(syntax);
  syntax->c_linkage = saved_c_linkage;
  syntax->explicit_cxx_linkage = saved_explicit_cxx_linkage;
  return node;
}

// Parses an external declaration (a global variable, etc.) and adds it
// to the symbol table.
static ASTNode* ParseExternalDeclarationBody(Syntax* syntax) {
  syntax->context = kParsingFileScope;
  if (syntax->current_namespace == NULL) {
    syntax->current_namespace = compiler->global_namespace;
  }

  if (CompilerCXXAtLeast(kLanguageStandardCXX20)) {
    if (LexLookingAt(syntax->lex, TOK(export))) {
      return ModuleSyntaxParseExportDeclaration(syntax);
    }
    ASTNode* module_node = ModuleSyntaxParseExternalDeclaration(syntax);
    if (module_node != NULL) {
      return module_node;
    }
  }

  ModuleSyntaxNoteNonImportDeclaration(syntax);

  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      LexLookingAt(syntax->lex, TOK(consteval))) {
    LexCheckpoint block_checkpoint;
    LexCheckpointSave(syntax->lex, &block_checkpoint);
    LexNextToken(syntax->lex);
    bool is_consteval_block = LexLookingAt(syntax->lex, TOK(lbrace));
    LexCheckpointRestore(syntax->lex, &block_checkpoint);
    LexCheckpointDestruct(&block_checkpoint);
    if (is_consteval_block) {
      return SyntaxParseConstevalBlock(syntax, TC(decl) | TC(closebrace));
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
  
  // Parse common __attribute__ / __declspec / C++ attribute syntax.
  while (SyntaxParseAnyAttribute(syntax, &attributes)) {
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

  if (!CompilerIsCXX() && is_constexpr) {
    if (StorageIs(storage, STO(extern))) {
      SyntaxError(syntax,
                  "file-scope C constexpr object cannot have external linkage");
    }
    storage &= ~STO(extern);
    storage |= STO(static);
  }

  if (StorageIs(storage, STO(auto)|STO(register))) {
    SyntaxError(syntax, "Illegal global storage specified: %s",
                StorageIs(storage, STO(register)) ? "register" : "auto");
    storage = STO(implicit);
  }

  // Parse common __attribute__ / __declspec / C++ attribute syntax.
  while (SyntaxParseAnyAttribute(syntax, &attributes)) {
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

ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax) {
  Token token_before = syntax->lex->current_token;
  SourceLocation location_before = syntax->lex->current_token_location;
  int errors_before = NumErrors();
  ASTNode* node = ParseExternalDeclarationBody(syntax);
  SyntaxEnsureProgress(syntax, token_before, location_before, errors_before,
                       TC(closebrace));
  return node;
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

ASTNode* SyntaxNewCXXConstructorCall(Syntax* syntax, Symbol* sym,
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

// True if `type` is a class type that has a (non-deleted) default constructor:
// one callable with no explicit arguments.  A constructor whose only parameters
// are the implicit object parameter (and, for a class with virtual bases, the
// most-derived flag) qualifies; a class whose every constructor takes explicit
// arguments (e.g. `Point(int, int)`) does not.  Used so a data member without
// an explicit initializer is default-constructed only when that is actually
// well-formed -- never for a non-default-constructible member of an aggregate,
// which is aggregate-initialized instead.
static bool CXXTypeHasDefaultConstructor(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  StructMember* ctor = FindCXXConstructor(type);
  if (ctor == NULL) {
    return false;
  }
  size_t expected = 1;
  if (StructHasVirtualBases(type->info.struct_info)) {
    expected++;
  }
  for (StructMember* candidate = ctor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL || candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type)) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (!info->is_constructor || info->is_deleted) {
      continue;
    }
    bool callable_without_arguments = info->prototype.length >= expected;
    for (size_t j = expected;
         callable_without_arguments && j < info->prototype.length; j++) {
      Symbol* parameter = info->prototype.value.p[j];
      if (parameter == NULL || parameter->default_argument == NULL) {
        callable_without_arguments = false;
      }
    }
    if (callable_without_arguments) {
      return true;
    }
  }
  return false;
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
    if (CXXConstructorIsInitializerListConstructor(info)) {
      return true;
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
    return SyntaxNewCXXConstructorCall(syntax, sym, actuals,
                                       initializer->location);
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
  return SyntaxNewCXXConstructorCall(syntax, sym, actuals,
                                     initializer->location);
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
  if (!CompilerIsCXX() || sym == NULL) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(sym->type) ||
      (!TypeIsClassTemplatePlaceholder(sym->type) &&
       (sym->type == NULL || sym->type->info.struct_info == NULL))) {
    if (!LexMatch(syntax->lex, TOK(lparen))) {
      return NULL;
    }
    SourceLocation location = syntax->lex->current_token_location;
    Vector* actuals = ParseCXXInitializerArgumentList(syntax, TOK(rparen));
    if (actuals->length == 1 &&
        (!TypeContainsTemplateParameter(sym->type) ||
         ((((ASTNode*)actuals->value.p[0])->flags & kASTPackExpansion) != 0))) {
      ASTNode* initializer = actuals->value.p[0];
      actuals->length = 0;
      VectorDelete(actuals);
      return initializer;
    }
    return NewCXXBracedInitializerFromActuals(actuals, location);
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
  // C++20 parenthesized aggregate initialization ([dcl.init]/16.6.2.5): a
  // parenthesized expression-list initializing an aggregate is treated as
  // aggregate initialization.  An aggregate has only implicit special-member
  // constructors, and aggregate init's single-element-of-same-type rule already
  // reproduces the copy/move case, so routing the whole parenthesized list
  // through aggregate init matches "consider constructors first, fall back to
  // aggregate init".  (The type is concrete here: any CTAD placeholder has been
  // resolved by ResolveCXXClassTemplateArgumentDeduction above.)
  //
  // A class with any user-declared (non-invented) constructor is never an
  // aggregate.  The `is_aggregate` flag on a class template's *primary* struct
  // is unreliable (it can be stale/true even though the class has constructors),
  // and the injected-class-name inside a member body can still refer to that
  // primary struct, so verify structurally rather than trusting the flag alone.
  bool has_user_declared_constructor = false;
  for (StructMember* c = constructor; c != NULL; c = c->overload_next) {
    if (c->is_member_function && c->symbol != NULL &&
        !c->symbol->flags.invented) {
      has_user_declared_constructor = true;
      break;
    }
  }
  bool paren_aggregate_init =
      !braced && !TypeIsClassTemplatePlaceholder(sym->type) &&
      sym->type->info.struct_info != NULL &&
      sym->type->info.struct_info->is_aggregate &&
      !sym->type->info.struct_info->is_template &&
      !has_user_declared_constructor;
  if (constructor == NULL ||
      (braced && sym->type->info.struct_info->is_aggregate) ||
      paren_aggregate_init) {
    ASTNode* braced_initializer =
        (braced || paren_aggregate_init)
            ? NewCXXBracedInitializerFromActuals(actuals, location)
            : NULL;
    if (braced_initializer == NULL) {
      VectorDeleteWithContents(actuals, (VectorElementDestructor)ASTNodeDelete,
                               /*free_element=*/false);
    }
    if (braced_initializer != NULL && wrap_aggregate_braces) {
      return NewVariableInitExpression(syntax, sym, braced_initializer);
    }
    return braced_initializer;
  }
  return SyntaxNewCXXConstructorCall(syntax, sym, actuals, location);
}

ASTNode* SyntaxNewCXXDefaultConstructorCallIfNeeded(Syntax* syntax,
                                                    Symbol* sym) {
  if (sym == NULL || StorageIs(sym->storage, STO(typedef))) {
    return NULL;
  }
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
  StructMember* ctor =
      FindStructMemberByName(sym->type->info.struct_info, constructor_name);
  if (ctor == NULL || !ctor->is_member_function ||
      !ctor->symbol->type->info.function.is_constructor) {
    return NULL;
  }
  // Calling a trivial implicit default constructor has no observable effect,
  // so aggregate default-initialization can omit it.  A non-trivial implicit
  // constructor must still run: in particular, aggregates may have default
  // member initializers or members with non-trivial default constructors.
  if (TypeIsStructOrUnion(sym->type) && sym->type->info.struct_info != NULL &&
      sym->type->info.struct_info->is_aggregate &&
      ctor->symbol->flags.invented &&
      (ctor->symbol->type->info.function.is_deleted ||
       ctor->symbol->type->info.function.is_trivial_special_member)) {
    return NULL;
  }
  return SyntaxNewCXXConstructorCall(syntax, sym, NewVector(), sym->location);
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
  if (!CompilerIsCXX() || sym == NULL ||
      StorageIs(sym->storage, STO(typedef)) ||
      !TypeIsStructOrUnion(sym->type) ||
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
  char fallback_func_name[64];
  if (func_name == NULL || func_name[0] == '\0') {
    const char* function_filename;
    int function_lineno;
    int function_start;
    int function_end;
    DecodeSourceLocation(func_symbol->location, &function_filename,
                         &function_lineno, &function_start, &function_end);
    (void)function_filename;
    (void)function_end;
    snprintf(fallback_func_name, sizeof(fallback_func_name),
             "anonymous_function_%d_%d", function_lineno, function_start);
    func_name = fallback_func_name;
  }
  if (local_name == NULL || local_name[0] == '\0') {
    local_name = "anonymous_local";
  }
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
    init = SyntaxNewCXXDefaultConstructorCallIfNeeded(syntax, sym);
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
      (StringEqual(compiler->target_name, "x86_64") ||
       StringEqual(compiler->target_name, "aarch64") ||
       StringEqual(compiler->target_name, "arm") ||
       StringEqual(compiler->target_name, "riscv"))) {
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

static Vector* ParseStructuredBindingNames(Syntax* syntax, Vector* symbols,
                                           int* pack_index) {
  if (!LexMatch(syntax->lex, TOK(lsquare))) {
    return NULL;
  }
  Vector* names = NewVector();
  *pack_index = -1;
  while (!LexEof(syntax->lex) && !LexLookingAt(syntax->lex, TOK(rsquare))) {
    bool is_pack = LexMatch(syntax->lex, TOK(ellipsis));
    if (is_pack) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax,
                    "Structured binding packs require C++26");
      }
      if (*pack_index >= 0) {
        SyntaxError(syntax,
                    "Structured binding declaration cannot contain multiple packs");
      } else {
        *pack_index = (int)names->length;
      }
    }
    if (!LexLookingAt(syntax->lex, TOK(identifier))) {
      SyntaxError(syntax, "Expected structured binding name");
      break;
    }
    String* name = NewString(syntax->lex->spelling.value);
    Symbol* sym = NewSymbol(name->value, NewTypeRecord(kTypeAuto, kQualPlain),
                            STO(implicit));
    sym->flags.is_local = true;
    sym->flags.is_defined = true;
    sym->flags.is_parameter_pack = is_pack;
    if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
        StringEqual(name, "_")) {
      // A structured binding outside namespace scope is name-independent
      // regardless of the backing object's storage duration.
      sym->flags.is_name_independent = true;
    }
    sym->structured_binding_pack_size = -1;
    sym->location = syntax->lex->current_token_location;
    LexNextToken(syntax->lex);
    if (SyntaxLookingAtCXXAttribute(syntax)) {
      if (!CompilerCXXAtLeast(kLanguageStandardCXX26)) {
        SyntaxError(syntax,
                    "Attributes on structured bindings require C++26");
      }
      SyntaxParseCXXAttributes(syntax, &sym->attributes);
      SyntaxApplyDeclarationAttributes(syntax, sym);
    }
    if (!SyntaxAddSymbol(syntax, sym)) {
      SyntaxError(syntax, "Duplicate structured binding name: %s",
                  name->value);
      StringDelete(name);
      SymbolDelete(sym);
    } else {
      VectorAppend(names, name);
      VectorAppend(symbols, sym);
    }
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
                                                 bool is_constexpr,
                                                 bool is_constinit,
                                                 Vector* declarations) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX17)) {
    return false;
  }
  // A NULL base type means the decl-specifier was invalid (e.g. an unknown type
  // name, with the error already reported).  A structured binding always has a
  // valid decl-specifier (`auto`/`const auto&`/...), so this can never be one;
  // bail out and let the normal declarator path recover -- copying a NULL type
  // below would dereference it.
  if (base_type == NULL) {
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
  Vector* symbols = NewVector();
  int pack_index = -1;
  Vector* names =
      ParseStructuredBindingNames(syntax, symbols, &pack_index);
  LexCheckpointDestruct(&checkpoint);
  if ((is_constexpr || is_constinit) &&
      !CompilerCXXAtLeast(kLanguageStandardCXX26)) {
    SyntaxError(syntax,
                "constexpr and constinit structured bindings require C++26");
  }
  for (size_t i = 0; i < symbols->length; i++) {
    Symbol* symbol = symbols->value.p[i];
    if (symbol != NULL) {
      symbol->flags.is_constexpr = is_constexpr;
      symbol->flags.is_constinit = is_constinit;
    }
  }
  if (names == NULL || names->length == 0) {
    SyntaxError(syntax, "Structured binding declaration requires at least one name");
  }
  if (pack_index >= 0 && syntax->current_template_parameter_count == 0) {
    SyntaxError(
        syntax,
        "Structured binding pack can only appear in a templated context");
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
  VectorAppend(declarations,
               NewStructuredBindingASTNode(declared_type, storage, names, symbols,
                                           pack_index,
                                           initializer,
                                           syntax->lex->current_token_location));
  return true;
}

// True if `type` has a nontrivial copy or move constructor.  This includes
// user-declared constructors and implicitly generated constructors that must
// copy or move a nontrivial subobject.
static bool CXXTypeHasNontrivialCopyOrMoveConstructor(TypeRecord* type) {
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  for (StructMember* c = FindCXXConstructor(type); c != NULL;
       c = c->overload_next) {
    if (c->is_member_function && c->symbol != NULL && c->symbol->type != NULL &&
        TypeIsFunction(c->symbol->type) &&
        (c->symbol->type->info.function.cxx_special_member_kind ==
             kCXXSpecialMemberCopyConstructor ||
         c->symbol->type->info.function.cxx_special_member_kind ==
             kCXXSpecialMemberMoveConstructor) &&
        (c->symbol->type->info.function.is_user_declared ||
         !c->symbol->type->info.function.is_trivial_special_member)) {
      return true;
    }
  }
  return false;
}

// True if copy-/move-initializing an object of `type` must run a copy or move
// constructor rather than degrading to a byte-wise copy: the class itself, a
// base, or a (possibly array) non-static data member -- recursively -- has a
// nontrivial copy or move constructor.  This is the recursive generalization
// of CXXTypeHasNontrivialCopyOrMoveConstructor: a class whose own copy/move
// constructor is implicit is still non-trivially copyable if any subobject is,
// and a byte copy would then alias whatever resource that subobject's
// user-defined copy constructor is responsible for duplicating (e.g. the bucket
// array owned by std::unordered_map's __hash_table member), causing shared state
// and later double-frees.  A class with no such subobject is left to the
// member-wise `init` path.
static bool CXXTypeRequiresCopyConstructorCallImpl(TypeRecord* type,
                                                   Vector* visited) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsFixedArray(type)) {
    return CXXTypeRequiresCopyConstructorCallImpl(type->next, visited);
  }
  if (!TypeIsStructOrUnion(type) || type->info.struct_info == NULL) {
    return false;
  }
  if (CXXTypeHasNontrivialCopyOrMoveConstructor(type)) {
    return true;
  }
  Struct* owner = type->info.struct_info;
  // Classes can contain themselves through compiler-internal or incomplete
  // representations.  Do not recurse forever when following such a cycle.
  if (VectorContainsPointer(visited, owner)) {
    return false;
  }
  VectorAppend(visited, owner);
  for (size_t i = 0; i < owner->bases.length; i++) {
    CXXBaseSpecifier* base = owner->bases.value.p[i];
    if (base != NULL &&
        CXXTypeRequiresCopyConstructorCallImpl(base->type, visited)) {
      return true;
    }
  }
  for (size_t i = 0; i < owner->virtual_bases.length; i++) {
    CXXVirtualBaseInfo* base = owner->virtual_bases.value.p[i];
    if (base != NULL &&
        CXXTypeRequiresCopyConstructorCallImpl(base->type, visited)) {
      return true;
    }
  }
  for (size_t i = 0; i < owner->members.length; i++) {
    StructMember* member = owner->members.value.p[i];
    if (member == NULL || member->symbol == NULL || member->is_static ||
        member->is_member_function || member->is_using_declaration ||
        StorageIs(member->symbol->storage, STO(typedef))) {
      continue;
    }
    if (CXXTypeRequiresCopyConstructorCallImpl(member->symbol->type, visited)) {
      return true;
    }
  }
  return false;
}

static bool CXXTypeRequiresCopyConstructorCall(TypeRecord* type) {
  Vector visited;
  VectorInit(&visited);
  bool requires_call =
      CXXTypeRequiresCopyConstructorCallImpl(type, &visited);
  VectorDestruct(&visited);
  return requires_call;
}

// Copy-initialization of a class object from a single expression, `T b = expr;`.
// Like direct-initialization `T b(expr);`, this must select and invoke a
// constructor (copy, move, or converting) rather than degrading to a shallow
// member-wise `init`.  A member-wise init merely copies the bytes of the source
// object, which for a class managing a resource (e.g. a string's heap pointer)
// aliases that resource instead of running the user-defined copy constructor and
// leads to double-frees / dangling pointers.
//
// Same-class initialization always uses constructor selection.  Initialization
// from a different class is rewritten only when the target has nontrivial copy
// semantics; otherwise the member-wise path is retained because it also handles
// conversion operators (for example the comparison-category types).
//
// `initializer` is the node produced by SyntaxParseInitializer after the braced
// list-constructor rewrite; only a plain expression initializer (`expr_init`)
// for a non-aggregate class type with at least one constructor is rewritten.
static ASTNode* NewCXXCopyInitConstructorInitializer(Syntax* syntax, Symbol* sym,
                                                     ASTNode* initializer) {
  if (!CompilerIsCXX() || sym == NULL || sym->type == NULL ||
      initializer == NULL || initializer->op != AST_OP(expr_init) ||
      !TypeIsStructOrUnion(sym->type) || sym->type->info.struct_info == NULL) {
    return initializer;
  }
  if (FindCXXConstructor(sym->type) == NULL) {
    return initializer;
  }
  ExpressionInitializerASTNode* expr_init =
      (ExpressionInitializerASTNode*)initializer;
  ASTNode* expr = expr_init->expr;
  if (expr == NULL) {
    return initializer;
  }
  bool same_class =
      TypeIsStructOrUnion(expr->type) &&
      expr->type->info.struct_info == sym->type->info.struct_info;
  if (!same_class && !CXXTypeRequiresCopyConstructorCall(sym->type)) {
    return initializer;
  }
  bool known_prvalue =
      (expr->flags & kASTAnalyzed) != 0 || expr->op == AST_OP(call) ||
      expr->op == AST_OP(compound_literal);
  if (known_prvalue && expr->value_category == kValueCategoryPrvalue &&
      same_class) {
    return initializer;
  }
  // Hand the operand to the constructor call and detach it from the wrapper so
  // the discarded `expr_init` node does not co-own it.
  expr_init->expr = NULL;
  Vector* actuals = NewVector();
  VectorAppend(actuals, expr);
  return SyntaxNewCXXConstructorCall(syntax, sym, actuals,
                                     initializer->location);
}

ASTNode* SyntaxRewriteCXXCopyInitConstructorIfNeeded(Syntax* syntax,
                                                     Symbol* sym,
                                                     ASTNode* initializer) {
  if (initializer == NULL) {
    return initializer;
  }
  if (initializer->op == AST_OP(expr_init)) {
    ASTNode* rewritten =
        NewCXXCopyInitConstructorInitializer(syntax, sym, initializer);
    if (rewritten != initializer) {
      ASTNodeDelete(initializer);
    }
    return rewritten;
  }
  if (initializer->op != AST_OP(init)) {
    return initializer;
  }
  BinaryASTNode* init = (BinaryASTNode*)initializer;
  ASTNode* expression_initializer = init->right;
  ASTNode* rewritten = NewCXXCopyInitConstructorInitializer(
      syntax, sym, expression_initializer);
  if (rewritten == expression_initializer) {
    return initializer;
  }

  init->right = NULL;
  ASTNodeDelete(initializer);
  ASTNodeDelete(expression_initializer);
  return rewritten;
}

// Emits -Wshadow when a newly declared block-scope variable `sym` hides a
// variable or parameter from an enclosing scope (or a file-scope object),
// mirroring clang/gcc -Wshadow.  Called for genuinely new local declarations,
// so the current innermost scope is skipped and only enclosing scopes are
// searched.
static void CheckLocalVariableShadow(Syntax* syntax, Symbol* sym) {
  if (!WarningIsEnabled("shadow")) {
    return;
  }
  if (sym == NULL || sym->name.length == 0) {
    return;
  }
  // Only ordinary objects shadow.  Functions, typedefs, and compiler-generated
  // symbols are not variables and are left alone.
  if (TypeIsFunction(sym->type) || StorageIs(sym->storage, STO(typedef)) ||
      sym->flags.is_temp || sym->flags.invented) {
    return;
  }
  if (syntax->local_symbol_stack == NULL) {
    return;
  }

  Symbol* shadowed = NULL;
  const char* what = NULL;
  // Enclosing local scopes first (the innermost scope is skipped via ->prev).
  Symbol* enclosing =
      FindLocalSymbol(syntax->local_symbol_stack->prev, &sym->name);
  if (enclosing != NULL) {
    if (TypeIsFunction(enclosing->type) ||
        StorageIs(enclosing->storage, STO(typedef))) {
      return;
    }
    shadowed = enclosing;
    what = enclosing->flags.is_argument ? "a parameter" : "a previous local";
  } else {
    Symbol* global = FindFileScopeSymbol(syntax, &sym->name);
    if (global == NULL || global == sym || TypeIsFunction(global->type) ||
        StorageIs(global->storage, STO(typedef))) {
      return;
    }
    shadowed = global;
    what = "a global declaration";
  }

  const char* filename;
  int lineno, start, end;
  DecodeSourceLocation(sym->location, &filename, &lineno, &start, &end);
  ReportWarning(filename, lineno, "shadow",
                "declaration of '%s' shadows %s", sym->name.value, what);

  const char* prev_filename;
  int prev_lineno, prev_start, prev_end;
  DecodeSourceLocation(shadowed->location, &prev_filename, &prev_lineno,
                       &prev_start, &prev_end);
  ReportNote(prev_filename, prev_lineno, "shadowed declaration is here");
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
    ValidateC23AutoDeclarator(syntax, sym);
    if (sym != NULL) {
      if (TypeIsFunction(sym->type)) {
        if (parser->is_constinit) {
          SyntaxError(syntax, "'constinit' cannot be applied to a function");
        }
      } else {
        sym->flags.is_constexpr = parser->is_constexpr;
        sym->flags.is_constinit = parser->is_constinit;
        DiagnoseConstevalOnNonFunction(syntax, sym->type, parser->is_consteval);
        if (parser->is_constinit &&
            !StorageIs(storage, STO(static) | STO(extern) | STO(thread))) {
          SyntaxError(
              syntax,
              "'constinit' variable must have static or thread storage duration");
        }
        if (sym->flags.is_constexpr) {
          sym->type->qualifiers |= kQualConst;
        }
      }
      if (!StorageIs(sym->storage, STO(extern))) {
        // Local declarators need this before duplicate-name handling so C++26
        // automatic `_` declarations can be recognized as name-independent.
        sym->flags.is_local = true;
      }
      MarkCXX26AutomaticNameIndependent(sym);
      Symbol* old_sym =
          FindTopLocalSymbol(syntax->local_symbol_stack, &sym->name);
      if (IsC23InferredAutoType(sym->type) && old_sym != NULL) {
        SyntaxError(syntax,
                    "C23 inferred auto declaration cannot redeclare '%s'",
                    sym->name.value);
      }
      bool ok = true;
      if (old_sym != NULL && !sym->flags.is_name_independent) {
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
           // Keep the declarator we just parsed.  The previous symbol has an
           // incompatible type, and for a name introduced by a using-declaration
           // or using-directive it has no type at all, which the rest of the
           // declaration processing cannot work with.
           ok = false;
        } else {
          MergeCXXDefaultArguments(syntax, old_sym, sym);
          MergeCXXContractAssertions(syntax, old_sym, sym);
          CheckMatchingConstexprConsteval(syntax, old_sym->type, sym->type);
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
            if (!added) {
              SyntaxError(syntax, "Duplicate symbol %s",
                          sym->name.value);
            }
          }
        } else {
          // This is the first declaration of this symbol, add to the symbol
          // table.
          CheckLocalVariableShadow(syntax, sym);
          bool added = SyntaxAddSymbol(syntax, sym);
          if (!added) {
            SyntaxError(syntax, "Duplicate symbol %s",
                        sym->name.value);
          }
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

    // Parse trailing __attribute__ / __declspec / C++ attribute syntax.
    while (SyntaxParseAnyAttribute(syntax, attributes)) {
    }

    // Symbol takes ownerhip of attribute strings.
    VectorAppendVector(&sym->attributes, attributes);
    VectorClear(attributes);
    SyntaxApplyDeclarationAttributes(syntax, sym);
    if (StorageIs(storage, STO(static)) && !TypeIsFunction(sym->type)) {
      SetCXXInlineLocalStaticAsmName(sym, "");
    }
    
    // Check for thread-local violations.
    SyntaxCheckThreadLocal(syntax, sym, kParsingBlockScope, false, false);

    // Declaring or defining a function?
    if (TypeIsFunction(sym->type)) {
      if (!CompilerIsCXX() && parser->is_constexpr) {
        SyntaxError(syntax, "C constexpr functions are not supported");
      }
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
      MaterializeDeferredClassTemplateType(syntax, sym);
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
            initializer =
                SyntaxNewCXXDefaultConstructorCallIfNeeded(syntax, sym);
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
      ValidateC23AutoInitializer(syntax, sym, initializer);
      ValidateC23ConstexprObject(syntax, sym, initializer);

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
          ((TypeContainsAuto(sym->type) ||
            (CompilerIsCXX() && sym->type->declarator == kDeclArray &&
             sym->type->info.array.is_flexible && initializer != NULL)) &&
           syntax->current_template_parameters == NULL)) {
        SemanticAnalyzeVariableDefinition(syntax,
                                        (VariableDeclarationASTNode*)decl);
      }
      
      if (!StorageIs(storage, STO(extern)) &&
          StorageIs(storage, STO(static) | STO(thread))) {
        VectorAppend(&syntax->local_statics, decl);
      }
    }

    if (IsC23InferredAutoType(type) &&
        LexLookingAt(syntax->lex, TOK(comma))) {
      SyntaxError(syntax,
                  "C23 inferred auto declaration must contain one declarator");
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
static ASTNode* ParseLocalDeclarationImpl(Syntax* syntax,
                                          bool require_semicolon) {
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
                                            parser.is_constexpr,
                                            parser.is_constinit,
                                            declarations)) {
    ParseLocalDeclarationList(&parser, type, storage, &attributes,
                              declarations);
  }
  TypeParserDestruct(&parser);
  TypeRecordDelete(type);

  if (require_semicolon) {
    // Stop at a closing brace as well: a missing ';' after a local declaration
    // must not skip the '}' that ends the function and swallow what follows.
    SyntaxNeedSemicolon(syntax, TC(type) | TC(closebrace));
  }

  AttributeListDestruct(&attributes);
  
  return NewDeclarationListASTNode(declarations,
                                   syntax->lex->current_token_location);
}

ASTNode* SyntaxParseLocalDeclaration(Syntax* syntax) {
  return ParseLocalDeclarationImpl(syntax, /*require_semicolon=*/true);
}

// Parses a C++ condition-declaration ("if (T x = init)", "while (T* p = q)",
// "switch (T c = get())").  Identical to a local declaration except that the
// terminating ')' takes the place of the usual ';', so no semicolon is
// consumed.  The declared symbol is added to the current (caller-opened) scope.
ASTNode* SyntaxParseConditionDeclaration(Syntax* syntax) {
  return ParseLocalDeclarationImpl(syntax, /*require_semicolon=*/false);
}

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

void SyntaxNeedTemplateClose(Syntax* syntax, TokenClass followers) {
  if (!LexConsumeClosingAngle(syntax->lex)) {
    SyntaxError(syntax, "Missing >");
    SyntaxRecover(syntax, followers);
  }
}

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
  if (SyntaxCurrentTokenStartsQualifiedName(syntax)) {
    SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
        syntax, &name, TC(openbra) | TC(stmt));
  } else {
    SyntaxParseFullyQualifiedIdentifier(syntax, &name);
  }
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
  // When the name positively resolves to a typedef, trust that: a type-name
  // followed by `=` is a defaulted parameter (`void f(T::x = v)`), not an
  // assignment.  The `followed_by_assignment` heuristic below is only meant to
  // steer an *unresolved* qualified name (e.g. a dependent `T::value = 5;`
  // statement) away from being parsed as a declaration, so apply it only when
  // lookup was inconclusive.
  if (decided) {
    return is_type;
  }
  if (followed_by_assignment) {
    return false;
  }
  return SyntaxCurrentTokenStartsQualifiedName(syntax);
}

bool SyntaxCurrentClassNameStartsType(Syntax* syntax) {
  if (!CompilerIsCXX() ||
      !LexLookingAt(syntax->lex, TOK(identifier))) {
    return false;
  }
  Struct* owner = syntax->cxx_class_head;
  if (owner == NULL && syntax->context == kParsingBlockScope &&
      compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    owner = compiler->current_function->info.function.cxx_member_owner;
  }
  while (owner != NULL) {
    if (CurrentClassNameMatchesTypeName(owner, &syntax->lex->spelling) &&
        owner->tag_symbol != NULL && owner->tag_symbol->type != NULL &&
        TypeIsStructOrUnion(owner->tag_symbol->type)) {
      return true;
    }
    owner = owner->lexical_parent;
  }
  return false;
}

bool SyntaxLookingAtType(Syntax* syntax) {
  switch (syntax->lex->current_token) {
    case TOK(atomic):
    case TOK(bitint):
    // Complex and imaginary types are not implemented, but the specifier still
    // starts a type: the type parser diagnoses it and consumes it, which no
    // other parser does.
    case TOK(complex):
    case TOK(imaginary):
    case TOK(char):
    case TOK(char8_t):
    case TOK(char16_t):
    case TOK(char32_t):
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
    case TOK(typeof):
    case TOK(typeof_unqual):
      return true;
    case TOK(typename):
      return CompilerIsCXX();
    case TOK(identifier): {
      if (CompilerIsCXX() && syntax->parsing_friend_type_specifier &&
          SyntaxCurrentIdentifierFollowedByScopeOperator(syntax) &&
          !SyntaxCurrentIdentifierFollowedByMemberPointerDeclarator(syntax)) {
        return true;
      }
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
        if (SyntaxCurrentClassNameStartsType(syntax)) {
          return true;
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
          int paren_depth = 0;
          int square_depth = 0;
          int brace_depth = 0;
          while (!LexEof(syntax->lex)) {
            Token t = syntax->lex->current_token;
            if (t == TOK(lparen)) {
              paren_depth++;
            } else if (t == TOK(rparen)) {
              paren_depth--;
            } else if (t == TOK(lsquare)) {
              square_depth++;
            } else if (t == TOK(rsquare)) {
              square_depth--;
            } else if (t == TOK(lbrace)) {
              brace_depth++;
            } else if (t == TOK(rbrace)) {
              brace_depth--;
            } else if (paren_depth == 0 && square_depth == 0 &&
                       brace_depth == 0) {
              if (t == TOK(less)) {
                depth++;
              } else {
                depth -= LexClosingAngleCount(t);
              }
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

// A functional-style temporary followed by member access is necessarily an
// expression statement, even when its leading name is also a type.  Without
// this lookahead, `T(value).member()` is sent to the declaration parser as the
// parenthesized declarator `T(value)`.
static bool CXXTypeStartsTemporaryMemberAccess(Syntax* syntax) {
  if (!CompilerIsCXX() ||
      (!LexLookingAt(syntax->lex, TOK(identifier)) &&
       !LexLookingAt(syntax->lex, TOK(coloncolon)))) {
    return false;
  }
  Symbol* initial_symbol =
      LexLookingAt(syntax->lex, TOK(identifier))
          ? SyntaxFindSymbol(syntax, &syntax->lex->spelling)
          : NULL;
  bool template_name =
      initial_symbol != NULL && initial_symbol->flags.is_template;
  if (LexLookingAt(syntax->lex, TOK(identifier))) {
    LexCheckpoint qualification;
    LexCheckpointSave(syntax->lex, &qualification);
    LexNextToken(syntax->lex);
    bool qualified = LexLookingAt(syntax->lex, TOK(coloncolon));
    LexCheckpointRestore(syntax->lex, &qualification);
    LexCheckpointDestruct(&qualification);
    bool qualified_type_name =
        qualified && SyntaxQualifiedNameLooksLikeType(syntax);
    bool type_name =
        initial_symbol != NULL &&
        (initial_symbol->flags.is_template ||
         StorageIs(initial_symbol->storage, STO(typedef)) ||
         (initial_symbol->type != NULL &&
          TypeIsStructOrUnion(initial_symbol->type)));
    if (!type_name &&
        SyntaxFindTag(syntax, &syntax->lex->spelling) == NULL &&
        !qualified_type_name) {
      return false;
    }
    template_name |= qualified_type_name;
  }
  LexCheckpoint checkpoint;
  LexCheckpointSave(syntax->lex, &checkpoint);
  bool is_member_access = false;

  // Keep this lookahead lexical. Running TypeParserParseType here is not
  // speculative: declarations and template instantiations it encounters can
  // mutate symbol tables even after the lexer checkpoint is restored.
  if (LexLookingAt(syntax->lex, TOK(coloncolon))) {
    LexNextToken(syntax->lex);
  }
  bool have_type_name = LexLookingAt(syntax->lex, TOK(identifier));
  while (have_type_name) {
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(less))) {
      if (!template_name) {
        have_type_name = false;
        break;
      }
      int angle_depth = 0;
      do {
        Token token = syntax->lex->current_token;
        if (token == TOK(semicolon)) {
          break;
        }
        if (token == TOK(less)) {
          angle_depth++;
        } else {
          angle_depth -= LexClosingAngleCount(token);
        }
        LexNextToken(syntax->lex);
      } while (!LexEof(syntax->lex) && angle_depth > 0);
      if (angle_depth != 0) {
        have_type_name = false;
        break;
      }
    }
    if (!LexLookingAt(syntax->lex, TOK(coloncolon))) {
      break;
    }
    LexNextToken(syntax->lex);
    if (LexLookingAt(syntax->lex, TOK(template))) {
      LexNextToken(syntax->lex);
    }
    have_type_name = LexLookingAt(syntax->lex, TOK(identifier));
  }

  if (have_type_name && LexLookingAt(syntax->lex, TOK(lparen))) {
    int depth = 0;
    do {
      if (LexLookingAt(syntax->lex, TOK(lparen))) {
        depth++;
      } else if (LexLookingAt(syntax->lex, TOK(rparen))) {
        depth--;
      }
      LexNextToken(syntax->lex);
    } while (!LexEof(syntax->lex) && depth > 0);
    is_member_access =
        depth == 0 &&
        (LexLookingAt(syntax->lex, TOK(dot)) ||
         LexLookingAt(syntax->lex, TOK(arrow)));
  }
  LexCheckpointRestore(syntax->lex, &checkpoint);
  LexCheckpointDestruct(&checkpoint);
  return is_member_access && SyntaxLookingAtType(syntax);
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
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) &&
      LexLookingAt(syntax->lex, TOK(consteval))) {
    LexCheckpoint block_checkpoint;
    LexCheckpointSave(syntax->lex, &block_checkpoint);
    LexNextToken(syntax->lex);
    bool is_consteval_block = LexLookingAt(syntax->lex, TOK(lbrace));
    LexCheckpointRestore(syntax->lex, &block_checkpoint);
    LexCheckpointDestruct(&block_checkpoint);
    if (is_consteval_block) {
      return false;
    }
  }
  if (SyntaxLookingAtAnyAttribute(syntax)) {
    LexCheckpoint checkpoint;
    LexCheckpointSave(syntax->lex, &checkpoint);
    Vector attrs = {0};
    VectorInit(&attrs);
    SyntaxParseAnyAttribute(syntax, &attrs);
    AttributeListDestruct(&attrs);
    bool result = SyntaxLookingAtDeclaration(syntax);
    LexCheckpointRestore(syntax->lex, &checkpoint);
    LexCheckpointDestruct(&checkpoint);
    return result;
  }
  if (CXXQualifiedNameLooksLikeCallExpression(syntax)) {
    return false;
  }
  if (CXXTypeStartsTemporaryMemberAccess(syntax)) {
    return false;
  }
  switch (syntax->lex->current_token) {
    // No expression can begin with an alignment specifier, so it always
    // introduces a declaration.
    case TOK(alignas):
      return true;
    case TOK(extern):
    case TOK(static):
    case TOK(auto):
    case TOK(register):
    case TOK(thread):
    case TOK(thread_local):
    case TOK(noreturn):
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
