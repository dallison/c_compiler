//
//  type_class.c
//  c_compiler
//
#include "type_class_internal.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
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
#include "compiler.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"


typedef struct CXXAggregateDeductionElement {
  const char* name;
  TypeRecord* type;
  SourceLocation location;
  bool has_default;
} CXXAggregateDeductionElement;

static CXXAggregateDeductionElement* NewCXXAggregateDeductionElement(
    const char* name, TypeRecord* type, SourceLocation location,
    bool has_default) {
  CXXAggregateDeductionElement* element = malloc(sizeof(*element));
  element->name = name;
  element->type = type;
  element->location = location;
  element->has_default = has_default;
  return element;
}

bool TypeIsTemplateParameterPlaceholder(TypeRecord* type, int* index) {
  if (type == NULL || type->declarator != kDeclPrimitive ||
      type->template_parameter_index < 0 ||
      type->dependent_member_name != NULL ||
      type->template_origin != NULL ||
      type->template_arguments != NULL) {
    return false;
  }
  if (index != NULL) {
    *index = type->template_parameter_index;
  }
  return true;
}

bool CurrentTemplateParameterIsPack(Syntax* syntax, int index) {
  if (syntax == NULL || syntax->current_template_parameters == NULL ||
      index < 0) {
    return false;
  }
  for (size_t i = 0; i < syntax->current_template_parameters->length; i++) {
    TemplateParameter* param = syntax->current_template_parameters->value.p[i];
    if (param != NULL && param->index == index) {
      return param->is_parameter_pack;
    }
  }
  return false;
}

static TemplateArgument* NewTemplateParameterPatternArgument(
    TemplateParameter* param) {
  if (param == NULL) {
    return NULL;
  }
  TemplateArgument* arg = malloc(sizeof(TemplateArgument));
  arg->kind = param->kind;
  arg->is_pack_expansion = param->is_parameter_pack;
  arg->type = NULL;
  arg->int_value = 0;
  arg->template_parameter_index = -1;
  arg->pack_arguments = NULL;
  arg->dependent_expr = NULL;
  arg->location = SOURCE_LOCATION_MISSING;
  if (param->kind == kTemplateParameterType) {
    arg->type = NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
    arg->type->template_parameter_index = param->index;
    if (param->name.length != 0) {
      arg->type->template_parameter_name = NewString(param->name.value);
    }
  } else {
    arg->template_parameter_index = param->index;
  }
  return arg;
}

static TypeRecord* NewCXXDeductionGuideReturnType(Symbol* tag) {
  if (tag == NULL || tag->type == NULL ||
      !TypeIsStructOrUnion(tag->type) ||
      tag->type->info.struct_info == NULL) {
    return NULL;
  }
  TypeRecord* type = TypeRecordCopy(tag->type);
  type->template_origin = tag;
  type->template_arguments = NewVector();
  Struct* str = tag->type->info.struct_info;
  for (size_t i = 0; i < str->template_parameters.length; i++) {
    TemplateArgument* arg = NewTemplateParameterPatternArgument(
        str->template_parameters.value.p[i]);
    if (arg != NULL) {
      VectorAppend(type->template_arguments, arg);
    }
  }
  return type;
}

static void CopyClassTemplateParametersToGuide(Struct* str, TypeRecord* func) {
  if (str == NULL || func == NULL || !TypeIsFunction(func)) {
    return;
  }
  VectorDestructWithContents(&func->info.function.template_parameters,
                             (VectorElementDestructor)TemplateParameterDelete,
                             /*free_element=*/false);
  VectorInit(&func->info.function.template_parameters);
  for (size_t i = 0; i < str->template_parameters.length; i++) {
    VectorAppend(&func->info.function.template_parameters,
                 TemplateParameterCopy(str->template_parameters.value.p[i]));
  }
  func->info.function.template_parameter_count =
      (int)str->template_parameters.length;
  func->info.function.template_parameter_base = 0;
}

static size_t CXXConstructorImplicitParameterCount(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.cxx_member_owner == NULL) {
    return 0;
  }
  size_t count = 1;
  if (func->info.function.is_constructor &&
      StructHasVirtualBases(func->info.function.cxx_member_owner)) {
    count++;
  }
  return count;
}

static Symbol* NewCXXDeductionGuideSymbol(Struct* str, Symbol* tag,
                                          TypeRecord* pattern,
                                          size_t first_formal) {
  TypeRecord* guide_type = NewFunctionTypeRecord();
  guide_type->info.function.is_deduction_guide = true;
  guide_type->info.function.is_implicitly_declared = true;
  CopyClassTemplateParametersToGuide(str, guide_type);
  TypeRecord* return_type = NewCXXDeductionGuideReturnType(tag);
  if (return_type == NULL) {
    TypeRecordDelete(guide_type);
    return NULL;
  }
  TypeRecordChain(guide_type, return_type);
  if (pattern != NULL && TypeIsFunction(pattern)) {
    for (size_t i = first_formal; i < pattern->info.function.prototype.length; i++) {
      Symbol* formal = pattern->info.function.prototype.value.p[i];
      if (formal == NULL || formal->type == NULL) {
        continue;
      }
      Symbol* clone = NewSymbol(formal->name.value, formal->type, formal->storage);
      clone->flags = formal->flags;
      clone->flags.is_argument = true;
      clone->location = formal->location;
      clone->default_argument =
          ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
      clone->value.arg_number = (int32_t)guide_type->info.function.prototype.length;
      VectorAppend(&guide_type->info.function.prototype, clone);
    }
  }
  Symbol* guide = NewSymbol(tag->name.value, guide_type, STO(implicit));
  guide->flags.is_template = guide_type->info.function.template_parameter_count > 0;
  guide->location = tag->location;
  guide_type->info.function.symbol = guide;
  return guide;
}

static void AddImplicitCXXConstructorDeductionGuide(Struct* str, Symbol* tag,
                                                   StructMember* member) {
  if (member == NULL || !member->is_member_function ||
      member->symbol == NULL || member->symbol->type == NULL ||
      !member->symbol->type->info.function.is_constructor ||
      member->symbol->flags.is_template) {
    return;
  }
  TypeRecord* func = member->symbol->type;
  Symbol* guide = NewCXXDeductionGuideSymbol(
      str, tag, func, CXXConstructorImplicitParameterCount(func));
  TypeAddCXXDeductionGuide(tag, guide);
}

static bool CXXAggregateDeductionMember(StructMember* member) {
  return member != NULL && member->symbol != NULL && !member->is_static &&
         !member->is_member_function && !member->is_using_declaration &&
         !StructMemberIsNestedType(member);
}

static bool CollectCXXAggregateDeductionElements(Struct* str,
                                                 SourceLocation location,
                                                 Vector* elements) {
  if (str == NULL) {
    return false;
  }
  for (size_t i = 0; i < str->bases.length; i++) {
    CXXBaseSpecifier* base = str->bases.value.p[i];
    if (base == NULL || base->is_virtual || base->access != kAccessPublic ||
        base->type == NULL) {
      continue;
    }
    if (TypeIsStructOrUnion(base->type) &&
        base->type->info.struct_info != NULL &&
        base->type->info.struct_info->is_aggregate &&
        CollectCXXAggregateDeductionElements(base->type->info.struct_info,
                                             location, elements)) {
      continue;
    } else {
      VectorAppend(elements,
                   NewCXXAggregateDeductionElement(
                       str->tag_name != NULL ? str->tag_name->value
                                             : "__ctad_base",
                       base->type, location, false));
    }
  }
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (!CXXAggregateDeductionMember(member)) {
      continue;
    }
    VectorAppend(elements,
                 NewCXXAggregateDeductionElement(
                     member->symbol->name.value, member->symbol->type,
                     member->symbol->location,
                     member->default_initializer != NULL));
  }
  return true;
}

static void AddImplicitCXXAggregateDeductionGuideForPrefix(Struct* str,
                                                           Symbol* tag,
                                                           Vector* elements,
                                                           size_t prefix_count) {
  TypeRecord* guide_type = NewFunctionTypeRecord();
  guide_type->info.function.is_deduction_guide = true;
  guide_type->info.function.is_implicitly_declared = true;
  CopyClassTemplateParametersToGuide(str, guide_type);
  TypeRecord* return_type = NewCXXDeductionGuideReturnType(tag);
  if (return_type == NULL) {
    TypeRecordDelete(guide_type);
    return;
  }
  TypeRecordChain(guide_type, return_type);
  for (size_t i = 0; i < prefix_count; i++) {
    CXXAggregateDeductionElement* element = elements->value.p[i];
    Symbol* formal =
        NewSymbol(element->name, element->type, STO(auto));
    formal->flags.is_argument = true;
    formal->location = element->location;
    formal->value.arg_number = (int32_t)guide_type->info.function.prototype.length;
    VectorAppend(&guide_type->info.function.prototype, formal);
  }
  Symbol* guide = NewSymbol(tag->name.value, guide_type, STO(implicit));
  guide->flags.is_template = guide_type->info.function.template_parameter_count > 0;
  guide->location = tag->location;
  guide_type->info.function.symbol = guide;
  TypeAddCXXDeductionGuide(tag, guide);
}

static void AddImplicitCXXAggregateDeductionGuide(Struct* str, Symbol* tag) {
  if (!str->is_aggregate) {
    return;
  }
  Vector elements;
  VectorInit(&elements);
  CollectCXXAggregateDeductionElements(str, tag->location, &elements);
  size_t total = 0;
  size_t required = 0;
  for (size_t i = 0; i < elements.length; i++) {
    CXXAggregateDeductionElement* element = elements.value.p[i];
    total++;
    if (!element->has_default) {
      required = total;
    }
  }
  for (size_t prefix = required; prefix <= total; prefix++) {
    AddImplicitCXXAggregateDeductionGuideForPrefix(str, tag, &elements, prefix);
  }
  VectorDestructWithContents(&elements, NULL, /*free_element=*/true);
}

static void AddImplicitCXXCopyDeductionGuide(Struct* str, Symbol* tag) {
  TypeRecord* guide_type = NewFunctionTypeRecord();
  guide_type->info.function.is_deduction_guide = true;
  guide_type->info.function.is_implicitly_declared = true;
  CopyClassTemplateParametersToGuide(str, guide_type);
  TypeRecord* return_type = NewCXXDeductionGuideReturnType(tag);
  TypeRecord* source_type = NewCXXDeductionGuideReturnType(tag);
  if (return_type == NULL || source_type == NULL) {
    if (return_type != NULL) {
      TypeRecordDelete(return_type);
    }
    if (source_type != NULL) {
      TypeRecordDelete(source_type);
    }
    TypeRecordDelete(guide_type);
    return;
  }
  TypeRecordChain(guide_type, return_type);
  Symbol* formal = NewSymbol("__ctad_source", source_type, STO(auto));
  formal->flags.is_argument = true;
  formal->location = tag->location;
  formal->value.arg_number = 0;
  VectorAppend(&guide_type->info.function.prototype, formal);
  Symbol* guide = NewSymbol(tag->name.value, guide_type, STO(implicit));
  guide->flags.is_template = guide_type->info.function.template_parameter_count > 0;
  guide->location = tag->location;
  guide_type->info.function.symbol = guide;
  TypeAddCXXDeductionGuide(tag, guide);
}

void AddImplicitCXXDeductionGuides(Struct* str, Symbol* tag) {
  if (!CompilerIsCXX() || str == NULL || tag == NULL || !str->is_template ||
      str->template_parameters.length == 0 || str->deduction_guides.length > 0) {
    return;
  }
  if (str->tag_name != NULL) {
    StructMember* first = FindStructMember(str, str->tag_name);
    for (StructMember* member = first; member != NULL;
         member = member->overload_next) {
      AddImplicitCXXConstructorDeductionGuide(str, tag, member);
    }
  }
  AddImplicitCXXCopyDeductionGuide(str, tag);
  AddImplicitCXXAggregateDeductionGuide(str, tag);
}

void TypeEnsureCXXDeductionGuides(Symbol* class_template) {
  if (class_template == NULL || class_template->type == NULL ||
      !TypeIsStructOrUnion(class_template->type) ||
      class_template->type->info.struct_info == NULL) {
    return;
  }
  AddImplicitCXXDeductionGuides(class_template->type->info.struct_info,
                                class_template);
}

static void CheckFlexibleArrays(TypeParser* parser, Struct* str, bool is_union) {
  // Check the constraints for flexible arrays.
  // 1. Flexible array cannot be the only member
  // 2. Flexible array must be at the end of the struct.
  // 3. No flexible arrays in unions.
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* member = str->members.value.p[i];
    if (TypeIsArray(member->symbol->type)) {
      if (member->symbol->type->info.array.is_flexible) {
        if (is_union) {
          SyntaxError(parser->syntax, "No flexible arrays allowed in unions");
          break;
        }
        if (str->members.length == 1) {
          SyntaxError(parser->syntax,
                      "Flexible array '%s' cannot be the only "
                      "member in a struct",
                      member->symbol->name.value);
          break;
        }
        if (i != str->members.length - 1) {
          SyntaxError(parser->syntax,
                      "Flexible array '%s' needs to be "
                      "the last member in a struct",
                      member->symbol->name.value);
          break;
        }
      }
    }
  }
}

static void AddInjectedClassName(TypeParser* parser, Symbol* tag) {
  if (tag == NULL || tag->flags.invented || tag->type == NULL ||
      tag->type->info.struct_info == NULL) {
    return;
  }
  Struct* str = tag->type->info.struct_info;
  if (!str->is_class && (!CompilerIsCXX() || str->is_union)) {
    return;
  }
  if (SyntaxFindSymbol(parser->syntax, &tag->name) != NULL) {
    return;
  }

  Symbol* alias = NewSymbol(tag->name.value, tag->type, STO(typedef));
  alias->namespace_ = tag->namespace_;
  if (!SyntaxAddSymbol(parser->syntax, alias)) {
    SymbolDelete(alias);
  }
}

static Symbol* EnsureCXXClassHeadTagForBaseClause(TypeParser* parser,
                                                  String* tag_name,
                                                  bool is_union,
                                                  bool is_class) {
  if (!CompilerIsCXX() || tag_name == NULL || tag_name->length == 0) {
    return NULL;
  }
  Symbol* tag = SyntaxFindTopScopeTag(parser->syntax, tag_name);
  if (tag != NULL) {
    CheckTagType(parser, tag, is_union, false);
    if (tag->type != NULL && TypeIsStructOrUnion(tag->type) &&
        tag->type->info.struct_info != NULL) {
      tag->type->info.struct_info->is_class = is_class;
    }
    return tag;
  }
  Struct* str = NewStruct(is_union);
  str->is_class = is_class;
  TypeRecord* type = NewTypeRecord(is_union ? kTypeUnion : kTypeStruct,
                                    kQualPlain);
  type->info.struct_info = str;
  tag = NewSymbol(tag_name->value, type, STO(implicit));
  tag->flags.is_forward_declared = true;
  str->tag_name = &tag->name;
  str->tag_symbol = tag;
  SyntaxAddTag(parser->syntax, tag);
  return tag;
}

static Symbol* ParseStructBody(TypeParser* parser, String* tag_name,
                               bool is_union, bool is_class,
                               Vector* attributes, Vector* bases) {
  // We have a struct body.
  // First check that this is not a duplicate definition.
  Struct* str = NULL;
  bool empty_tag_name = tag_name->length == 0;
  if (empty_tag_name) {
    SyntaxFakeTagName(parser->syntax, tag_name);
  }
  Symbol* tag = SyntaxFindTopScopeTag(parser->syntax, tag_name);
  if (tag != NULL) {
    if (!tag->flags.is_forward_declared) {
      SyntaxError(parser->syntax, "Duplicate definition of struct/union %s",
                   tag_name->value);
    } else {
      CheckTagType(parser, tag, is_union, false);
    }
    str = tag->type->info.struct_info;
    if (str != NULL && str->tag_symbol == NULL) {
      str->tag_symbol = tag;
    }
    // The definition's class-key determines default member access, even when a
    // prior forward declaration (e.g. 'friend class X;' or 'class X;') used a
    // different key than the defining 'struct'/'class'.
    if (str != NULL) {
      str->is_class = is_class;
      str->is_union = is_union;
      if (tag->type != NULL) {
        tag->type->type &= ~(kTypeStruct | kTypeUnion);
        tag->type->type |= is_union ? kTypeUnion : kTypeStruct;
      }
    }
  } else {
    // Tag doesn't exist in the, create one.
    str = NewStruct(is_union);
    str->is_class = is_class;
    TypeRecord* type = NewTypeRecord(is_union ? kTypeUnion : kTypeStruct,
                                      kQualPlain);
    type->info.struct_info = str;
    tag = NewSymbol(tag_name->value, type, STO(implicit));
    str->tag_name = &tag->name;
    str->tag_symbol = tag;
    if (empty_tag_name) {
      tag->flags.invented = true;
    }
    SyntaxAddTag(parser->syntax, tag);
    AddInjectedEnumName(parser, tag);
  }

  // Note in the symbol that this tag is now defined and not
  // forward declared.
  tag->flags.is_forward_declared = false;
  tag->flags.is_defined = true;

  // Record how many template parameters are in scope as the body begins (this
  // class's own template head plus any enclosing templates).  The class's
  // is_template / template_parameter_count are only set once the body is fully
  // parsed, so this captured count is the reliable in-body signal that we are
  // inside a class template (needed for friend-declaration disambiguation).
  str->defining_template_scope_count =
      parser->syntax->current_template_parameter_count;

  // Now 'tag' will be the struct tag pointer
  // and 'str' will be a pointer to the Struct information.
  // Capture the #pragma pack(n) value in effect at the point of definition so
  // member offsets reflect it (and any later push/pop does not retroactively
  // change this struct).
  str->pack = compiler->pack_alignment;
  // Apply layout attributes (packed, aligned) before laying out members so the
  // member offsets reflect them in a single pass.
  StructApplyLayoutAttributes(str, attributes);
  for (size_t i = 0; i < bases->length; i++) {
    VectorAppend(&str->bases, bases->value.p[i]);
  }
  bases->length = 0;
  CollectCXXVirtualBases(str);
  CopyCXXBaseVirtualMembers(str);
  LayoutCXXBaseSpecifiers(str);

  LocalSymbolTable* class_tag_scope = NULL;
  LocalSymbolTable* class_symbol_scope = NULL;
  if (CompilerIsCXX()) {
    class_tag_scope = NewLocalSymbolTable();
    class_tag_scope->prev = parser->syntax->local_tag_stack;
    parser->syntax->local_tag_stack = class_tag_scope;
    // Open a symbol scope for the class body so that member type aliases
    // (`using`/`typedef`) become visible as type-names to subsequent member
    // declarations and inline member function bodies, as required by C++ class
    // scope rules.
    class_symbol_scope = NewLocalSymbolTable();
    class_symbol_scope->prev = parser->syntax->local_symbol_stack;
    parser->syntax->local_symbol_stack = class_symbol_scope;
  }
  Struct* saved_member_owner = parser->cxx_member_owner;
  Struct* saved_class_head = parser->syntax->cxx_class_head;
  if (str->lexical_parent == NULL && saved_member_owner != NULL) {
    str->lexical_parent = saved_member_owner;
  }
  parser->cxx_member_owner = str;
  parser->syntax->cxx_class_head = str;
  ParseStructMembers(parser, str, is_union, tag_name);
  parser->cxx_member_owner = saved_member_owner;
  parser->syntax->cxx_class_head = saved_class_head;
  if (class_symbol_scope != NULL) {
    assert(parser->syntax->local_symbol_stack == class_symbol_scope);
    parser->syntax->local_symbol_stack = class_symbol_scope->prev;
    LocalSymbolTableDelete(class_symbol_scope);
  }
  ComputeCXXAggregateStatus(str);
  AddImplicitCXXSpecialMembers(parser, str, tag);
  AddImplicitCXXDestructorIfNeeded(parser, str, tag);
  AddImplicitCXXDeductionGuides(str, tag);
  if (class_tag_scope != NULL) {
    assert(parser->syntax->local_tag_stack == class_tag_scope);
    parser->syntax->local_tag_stack = class_tag_scope->prev;
    LocalSymbolTableDelete(class_tag_scope);
  }
  UpdateCXXAbstractStatus(str);
  AddCXXVPtrMember(parser, str);
  AddCXXVBPtrMember(parser, str);
  str->non_virtual_size = str->size;
  LayoutCXXVirtualBaseSpecifiers(str);
  RegisterCXXVTable(parser, str);
  RegisterCXXVBTables(parser, str);
  // The class's vtables now exist, so any constructor preambles built earlier
  // during member parsing can have their deferred __vptr initializers emitted.
  str->vtables_registered = true;
  SyntaxFlushPendingVPtrInitializers(str);

  // Round the size of the struct up to its own alignment (the maximum
  // alignment of its members, or an explicit aligned(N)), as required by the
  // ABI.
  FinalizeStructAlignment(str);
  SyntaxNeedBracket(parser->syntax, TOK(rbrace), TC(exprsep));

  CheckFlexibleArrays(parser, str, is_union);
  AddInjectedClassName(parser, tag);
  // Record concrete (non-template) C++ classes so unused private data members
  // can be diagnosed at end of translation unit (-Wunused-private-field).
  // Templates (defining_template_scope_count > 0) are skipped because member
  // usage in a template body can be dependent; compiler-invented tags (lambda
  // closures, anonymous types) are skipped because their members are not
  // user-declared private fields.
  if (CompilerIsCXX() && str != NULL && !is_union &&
      str->defining_template_scope_count == 0 && tag != NULL &&
      !tag->flags.invented) {
    VectorAppend(&compiler->cxx_defined_classes, str);
  }
  return tag;
}

Symbol* TypeParserParseStruct(TypeParser* parser, bool is_union, bool is_class) {
  // Parse common __attribute__ syntax (e.g. struct __attribute__((packed)) ...).
  Vector attributes = {0};
  Vector bases = {0};
  VectorInit(&bases);
  while (true) {
    if (SyntaxParseCXXAlignas(parser->syntax, &attributes)) {
      continue;
    }
    if (LexMatch(parser->lex, TOK(attribute))) {
      SyntaxParseAttribute(parser->syntax, &attributes);
    } else if (SyntaxLookingAtCXXAttribute(parser->syntax)) {
      SyntaxParseCXXAttributes(parser->syntax, &attributes);
    } else {
      break;
    }
  }

  String tag_name = {0};
  FullyQualifiedIdentifier qualified_tag = {0};
  bool has_qualified_tag = false;
  Vector* specialization_args = NULL;
  Vector* completed_specialization_args = NULL;
  Symbol* tag = NULL;
  Symbol* specialization_template = NULL;
  String specialization_name;
  LocalSymbolTable* saved_specialization_tag_stack = NULL;
  Namespace* saved_specialization_namespace = NULL;
  bool using_specialization_namespace = false;
  bool is_full_specialization = false;
  bool is_partial_specialization = false;
  bool is_final = false;
  StringInit(&specialization_name, NULL);
  FullyQualifiedIdentifierInit(&qualified_tag);

  if (LexLookingAt(parser->lex, TOK(semicolon))) {
    // Don't consume the semicolon.
    goto done;
  }

  // Read the tag name if there is one.
  if (CompilerIsCXX() && (SyntaxCurrentTokenStartsQualifiedName(parser->syntax) ||
                          LexLookingAt(parser->lex, TOK(identifier)))) {
    SyntaxParseFullyQualifiedIdentifierWithTemplateIds(
        parser->syntax, &qualified_tag, TC(openbra) | TC(decl));
    has_qualified_tag = qualified_tag.is_qualified;
    if (!has_qualified_tag) {
      StringSet(&tag_name, FullyQualifiedIdentifierLast(&qualified_tag));
    }
    if (qualified_tag.template_arguments.length > 0) {
      Vector* args =
          qualified_tag.template_arguments.value.p[
              qualified_tag.template_arguments.length - 1];
      specialization_args = TemplateArgumentVectorCopy(args);
    }
  } else if (SyntaxCurrentTokenStartsQualifiedName(parser->syntax)) {
    SyntaxParseFullyQualifiedIdentifier(parser->syntax, &qualified_tag);
    has_qualified_tag = qualified_tag.is_qualified;
    if (!has_qualified_tag) {
      StringSet(&tag_name, FullyQualifiedIdentifierLast(&qualified_tag));
    }
  } else if (LexLookingAt(parser->lex, TOK(identifier))) {
    // Struct tag is present.
    StringSetString(&tag_name, &parser->lex->spelling);
    LexNextToken(parser->lex);
  }
  while (SyntaxParseCXXAlignas(parser->syntax, &attributes) ||
         SyntaxParseCXXAttributes(parser->syntax, &attributes)) {
  }
  is_final = ParseCXXClassFinalSpecifier(parser);
  while (SyntaxParseCXXAlignas(parser->syntax, &attributes) ||
         SyntaxParseCXXAttributes(parser->syntax, &attributes)) {
  }
  is_full_specialization =
      parser->syntax->parsing_template_specialization &&
      specialization_args != NULL;
  is_partial_specialization =
      parser->syntax->parsing_template_declaration &&
      specialization_args != NULL;
  if (is_full_specialization || is_partial_specialization) {
    specialization_template =
        has_qualified_tag ? SyntaxFindQualifiedSymbol(parser->syntax,
                                                      &qualified_tag)
                          : SyntaxFindSymbol(parser->syntax, &tag_name);
    if (specialization_template == NULL ||
        !specialization_template->flags.is_template ||
        specialization_template->type == NULL ||
        !TypeIsStructOrUnion(specialization_template->type) ||
        specialization_template->type->info.struct_info == NULL) {
      SyntaxError(parser->syntax, "%s is not a class template",
                  qualified_tag.spelling.value);
    } else {
      completed_specialization_args = CompleteClassTemplateArguments(
          parser, specialization_template->type->info.struct_info,
          specialization_args);
      if (completed_specialization_args != NULL) {
        AppendTemplateInstantiationName(&specialization_name,
                                        specialization_template,
                                        completed_specialization_args);
        StringSetString(&tag_name, &specialization_name);
        if (is_partial_specialization &&
            specialization_template->type->info.struct_info != NULL) {
          // Two constrained partial specializations may share an identical
          // pattern (differing only in their associated constraints), e.g.
          //   template <HasDiff T>  struct S<T> { ... };
          //   template <SubInt  T>  struct S<T> { ... };
          // Mangling the pattern alone would produce the same tag name and
          // collide as a "duplicate definition".  Disambiguate by appending the
          // index this specialization will occupy in the primary template's
          // partial-specialization list, so each gets a distinct body tag while
          // constraint-based selection still chooses the right one.
          size_t partial_index =
              specialization_template->type->info.struct_info
                  ->partial_specializations.length;
          StringPrintf(&tag_name, "#%zu", partial_index);
        }
        has_qualified_tag = false;
        saved_specialization_tag_stack = parser->syntax->local_tag_stack;
        saved_specialization_namespace = parser->syntax->current_namespace;
        parser->syntax->local_tag_stack = NULL;
        parser->syntax->current_namespace =
            specialization_template->namespace_ != NULL
                ? specialization_template->namespace_
                : compiler->global_namespace;
        using_specialization_namespace = true;
      }
    }
  }

  Symbol* class_head_tag = NULL;
  Struct* saved_base_member_owner = parser->cxx_member_owner;
  Struct* saved_base_class_head = parser->syntax->cxx_class_head;
  if (!has_qualified_tag && !is_full_specialization &&
      !is_partial_specialization && LexLookingAt(parser->lex, TOK(colon))) {
    class_head_tag =
        EnsureCXXClassHeadTagForBaseClause(parser, &tag_name, is_union, is_class);
    if (class_head_tag != NULL && class_head_tag->type != NULL &&
        TypeIsStructOrUnion(class_head_tag->type)) {
      parser->cxx_member_owner = class_head_tag->type->info.struct_info;
      parser->syntax->cxx_class_head = class_head_tag->type->info.struct_info;
    }
  }
  ParseCXXBaseSpecifiers(parser, &bases, is_union, is_class);
  parser->cxx_member_owner = saved_base_member_owner;
  parser->syntax->cxx_class_head = saved_base_class_head;
  if (LexMatch(parser->lex, TOK(lbrace))) {
    if (has_qualified_tag) {
      SyntaxError(parser->syntax, "Cannot define qualified struct tag %s",
                  qualified_tag.spelling.value);
    }
    tag = ParseStructBody(parser, &tag_name, is_union, is_class, &attributes,
                          &bases);
    if (is_final && tag != NULL && tag->type != NULL &&
        tag->type->info.struct_info != NULL) {
      tag->type->info.struct_info->is_final = true;
    }
    if (tag != NULL && tag->type != NULL &&
        tag->type->info.struct_info != NULL) {
      FinalizePendingInlineConstructorPreambles(parser,
                                                tag->type->info.struct_info);
    }
    if (is_partial_specialization && tag != NULL &&
        completed_specialization_args != NULL) {
      AddClassTemplatePartialSpecialization(parser, specialization_template,
                                            tag,
                                            completed_specialization_args);
    }
  } else {
    // No open brace, this is a reference to an existing struct or the
    // creation of a new one.
    if (tag_name.length == 0 && !has_qualified_tag) {
      // No tag name, nothing to do.
      goto done;
    }
    tag = has_qualified_tag ? SyntaxFindQualifiedTag(parser->syntax, &qualified_tag)
                            : SyntaxFindTag(parser->syntax, &tag_name);
    if (tag == NULL) {
      if (has_qualified_tag) {
        SyntaxError(parser->syntax, "Unknown struct tag %s",
                    qualified_tag.spelling.value);
        goto done;
      }
      // New tag.
      Struct* str = NewStruct(is_union);
      str->is_class = is_class;
      TypeRecord* type =
          NewTypeRecord(is_union ? kTypeUnion : kTypeStruct, kQualPlain);
      type->info.struct_info = str;
      tag = NewSymbol(tag_name.value, type, STO(implicit));
      tag->flags.is_forward_declared = true;
      str->tag_name = &tag->name;
      str->tag_symbol = tag;
      SyntaxAddTag(parser->syntax, tag);
      AddInjectedClassName(parser, tag);
    } else {
      // Tag already exists, make sure it's the same tag type.
      CheckTagType(parser, tag, is_union, false);
    }
  }

done:
  if (using_specialization_namespace) {
    parser->syntax->local_tag_stack = saved_specialization_tag_stack;
    parser->syntax->current_namespace = saved_specialization_namespace;
  }
  if (tag != NULL && !is_partial_specialization) {
    parser->syntax->last_parsed_tag = tag;
  }
  FullyQualifiedIdentifierDestruct(&qualified_tag);
  StringDestruct(&tag_name);
  StringDestruct(&specialization_name);
  if (specialization_args != NULL) {
    VectorDeleteWithContents(specialization_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  if (completed_specialization_args != NULL) {
    VectorDeleteWithContents(completed_specialization_args,
                             (VectorElementDestructor)TemplateArgumentDelete,
                             /*free_element=*/false);
  }
  VectorDestructWithContents(&bases,
                             (VectorElementDestructor)CXXBaseSpecifierDelete,
                             /*free_element=*/false);
  AttributeListDestruct(&attributes);
  return tag;
}
