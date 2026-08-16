//
//  reflection.c
//  c_compiler
//

#include "reflection.h"

#include "ast.h"
#include "compiler.h"
#include "symbol.h"
#include "type_compare.h"
#include "type_core.h"
#include "type_internal.h"

#include <stdlib.h>
#include <string.h>

static ReflectionEntityKind ReflectionKindForSymbol(Symbol* symbol) {
  if (symbol == NULL) {
    return kReflectionInvalid;
  }
  if (symbol->flags.is_concept) {
    return kReflectionConcept;
  }
  if (symbol->alias_template != NULL) {
    return kReflectionAliasTemplate;
  }
  if (symbol->variable_template != NULL) {
    return kReflectionVariableTemplate;
  }
  if (symbol->flags.is_template && symbol->type != NULL &&
      TypeIsFunction(symbol->type)) {
    return kReflectionFunctionTemplate;
  }
  if (symbol->flags.is_template && symbol->type != NULL &&
      TypeIsStructOrUnion(symbol->type)) {
    return kReflectionClassTemplate;
  }
  if (symbol->flags.is_template) {
    return kReflectionTemplate;
  }
  if (symbol->flags.is_argument) {
    return kReflectionFunctionParameter;
  }
  if (StorageIs(symbol->storage, STO(typedef))) {
    return kReflectionTypeAlias;
  }
  if (symbol->type != NULL && TypeIsFunction(symbol->type)) {
    return kReflectionFunction;
  }
  return kReflectionVariable;
}

enum {
  kRefWire_data_member_spec = 33,
  kRefWire_constexpr_initializer = 34,
  kRefWire_promoted_symbol = 35,
  kRefWire_sequence = 36,
  kRefWire_substituted_template = 37,
  kRefWire_substituted_arguments = 38,
  kRefWire_extract_type = 39,
  kRefWire_spec_member_type = 40,
  kRefWire_spec_name = 41,
  kRefWire_spec_alignment = 42,
  kRefWire_spec_bit_width = 43,
  kRefWire_spec_no_unique_address = 44,
  kRefWire_spec_has_name = 45,
  kRefWire_spec_has_alignment = 46,
  kRefWire_spec_has_bit_width = 47,
  kRefWire_spec_annotations = 48,
  kRefWire_sequence_kind = 49,
  kRefWire_sequence_value = 50,
};

static void ReflectionDataMemberSpecDestructContents(
    ReflectionDataMemberSpec* spec) {
  if (spec == NULL) {
    return;
  }
  TypeRecordDelete(spec->member_type);
  StringDestruct(&spec->name);
  VectorDestruct(&spec->annotations);
}

ReflectionDataMemberSpec* ReflectionDataMemberSpecNew(TypeRecord* member_type) {
  ReflectionDataMemberSpec* spec = calloc(1, sizeof(*spec));
  spec->member_type =
      member_type != NULL ? TypeRecordCopy(member_type) : NULL;
  StringInit(&spec->name, NULL);
  VectorInit(&spec->annotations);
  return spec;
}

ReflectionDataMemberSpec* ReflectionDataMemberSpecCopy(
    const ReflectionDataMemberSpec* spec) {
  if (spec == NULL) {
    return NULL;
  }
  ReflectionDataMemberSpec* copy = ReflectionDataMemberSpecNew(spec->member_type);
  if (spec->name.value != NULL) {
    StringSetString(&copy->name, (String*)&spec->name);
  }
  copy->alignment = spec->alignment;
  copy->bit_width = spec->bit_width;
  copy->no_unique_address = spec->no_unique_address;
  copy->has_name = spec->has_name;
  copy->has_alignment = spec->has_alignment;
  copy->has_bit_width = spec->has_bit_width;
  for (size_t i = 0; i < spec->annotations.length; i++) {
    VectorAppend(&copy->annotations, spec->annotations.value.p[i]);
  }
  return copy;
}

void ReflectionDataMemberSpecDelete(ReflectionDataMemberSpec* spec) {
  if (spec == NULL) {
    return;
  }
  ReflectionDataMemberSpecDestructContents(spec);
  free(spec);
}

bool ReflectionDataMemberSpecEqual(const ReflectionDataMemberSpec* left,
                                   const ReflectionDataMemberSpec* right) {
  if (left == right) {
    return true;
  }
  if (left == NULL || right == NULL) {
    return false;
  }
  if (left->has_name != right->has_name || left->has_alignment != right->has_alignment ||
      left->has_bit_width != right->has_bit_width ||
      left->no_unique_address != right->no_unique_address ||
      left->alignment != right->alignment || left->bit_width != right->bit_width) {
    return false;
  }
  if (!TypeEqual(left->member_type, right->member_type)) {
    return false;
  }
  if (left->has_name &&
      !StringEqualString((String*)&left->name, (String*)&right->name)) {
    return false;
  }
  if (left->annotations.length != right->annotations.length) {
    return false;
  }
  for (size_t i = 0; i < left->annotations.length; i++) {
    ReflectionValue* l = left->annotations.value.p[i];
    ReflectionValue* r = right->annotations.value.p[i];
    if (!ReflectionValueEqual(l, r)) {
      return false;
    }
  }
  return true;
}

static void ReflectionValueDestructOwned(ReflectionValue* value) {
  if (value == NULL) {
    return;
  }
  ReflectionDataMemberSpecDelete(value->data_member_spec);
  value->data_member_spec = NULL;
  ASTNodeDelete(value->constexpr_initializer);
  value->constexpr_initializer = NULL;
  VectorDestruct(&value->sequence);
  VectorDestructWithContents(
      &value->substituted_arguments,
      (VectorElementDestructor)TemplateArgumentDelete, /*free_element=*/false);
  TypeRecordDelete(value->extract_type);
  value->extract_type = NULL;
}

static ReflectionValue ReflectionCandidate(ReflectionEntityKind kind,
                                           TypeRecord* reflected_type,
                                           Symbol* symbol, StructMember* member,
                                           Namespace* namespace_,
                                           Struct* parent_class,
                                           size_t base_index,
                                           size_t parameter_index,
                                           Attribute* annotation,
                                           int64_t scalar_ivalue,
                                           double scalar_fvalue,
                                           bool scalar_is_float,
                                           Namespace* namespace_alias_target,
                                           SourceLocation location) {
  return (ReflectionValue){
      .kind = kind,
      .reflected_type = reflected_type,
      .symbol = symbol,
      .member = member,
      .namespace_ = namespace_,
      .parent_class = parent_class,
      .base_index = base_index,
      .location = location,
      .parameter_index = parameter_index,
      .annotation = annotation,
      .scalar_ivalue = scalar_ivalue,
      .scalar_fvalue = scalar_fvalue,
      .scalar_is_float = scalar_is_float,
      .namespace_alias_target = namespace_alias_target,
  };
}

static bool ReflectionSequenceEqual(const Vector* left, const Vector* right) {
  if (left == right) {
    return true;
  }
  if (left == NULL || right == NULL ||
      left->length != right->length) {
    return false;
  }
  for (size_t i = 0; i < left->length; i++) {
    if (!ReflectionValueEqual(left->value.p[i], right->value.p[i])) {
      return false;
    }
  }
  return true;
}

static bool ReflectionTemplateArgumentsEqual(const Vector* left,
                                             const Vector* right) {
  if (left == right) {
    return true;
  }
  if (left == NULL || right == NULL ||
      left->length != right->length) {
    return false;
  }
  for (size_t i = 0; i < left->length; i++) {
    TemplateArgument* l = left->value.p[i];
    TemplateArgument* r = right->value.p[i];
    if (!TemplateArgumentEqual(l, r)) {
      return false;
    }
  }
  return true;
}

static void ReflectionCopyPayload(ReflectionValue* dest,
                                  const ReflectionValue* source) {
  dest->data_member_spec =
      source->data_member_spec != NULL
          ? ReflectionDataMemberSpecCopy(source->data_member_spec)
          : NULL;
  dest->constexpr_initializer =
      source->constexpr_initializer != NULL
          ? ASTNodeClone(source->constexpr_initializer, IdentityCloneNode,
                         NULL, NULL)
          : NULL;
  dest->promoted_symbol = source->promoted_symbol;
  dest->substituted_template = source->substituted_template;
  dest->extract_type =
      source->extract_type != NULL ? TypeRecordCopy(source->extract_type) : NULL;
  VectorInit(&dest->sequence);
  for (size_t i = 0; i < source->sequence.length; i++) {
    VectorAppend(&dest->sequence, source->sequence.value.p[i]);
  }
  VectorInit(&dest->substituted_arguments);
  for (size_t i = 0; i < source->substituted_arguments.length; i++) {
    VectorAppend(&dest->substituted_arguments,
                 TemplateArgumentCopy(source->substituted_arguments.value.p[i]));
  }
}

static ReflectionValue* ReflectionIntern(ReflectionValue candidate) {
  for (size_t i = 0; i < compiler->reflection_values.length; i++) {
    ReflectionValue* existing = compiler->reflection_values.value.p[i];
    if (ReflectionValueEqual(existing, &candidate)) {
      ReflectionValueDestructOwned(&candidate);
      return existing;
    }
  }
  ReflectionValue* value =
      ReflectionCreateDeserialized(candidate.kind, candidate.location);
  value->reflected_type = candidate.reflected_type != NULL
                              ? TypeRecordCopy(candidate.reflected_type)
                              : NULL;
  value->symbol = candidate.symbol;
  value->member = candidate.member;
  value->namespace_ = candidate.namespace_;
  value->parent_class = candidate.parent_class;
  value->base_index = candidate.base_index;
  value->parameter_index = candidate.parameter_index;
  value->annotation = candidate.annotation;
  value->scalar_ivalue = candidate.scalar_ivalue;
  value->scalar_fvalue = candidate.scalar_fvalue;
  value->scalar_is_float = candidate.scalar_is_float;
  value->namespace_alias_target = candidate.namespace_alias_target;
  ReflectionCopyPayload(value, &candidate);
  ReflectionValueDestructOwned(&candidate);
  return value;
}

ReflectionValue* ReflectionCreate(ReflectionEntityKind kind,
                                  TypeRecord* reflected_type, Symbol* symbol,
                                  StructMember* member, Namespace* namespace_,
                                  Struct* parent_class, size_t base_index,
                                  SourceLocation location) {
  return ReflectionCreateExtended(kind, reflected_type, symbol, member,
                                  namespace_, parent_class, base_index, 0, NULL,
                                  0, 0.0, false, NULL, location);
}

ReflectionValue* ReflectionCreateExtended(ReflectionEntityKind kind,
                                          TypeRecord* reflected_type,
                                          Symbol* symbol, StructMember* member,
                                          Namespace* namespace_,
                                          Struct* parent_class, size_t base_index,
                                          size_t parameter_index,
                                          Attribute* annotation,
                                          int64_t scalar_ivalue,
                                          double scalar_fvalue,
                                          bool scalar_is_float,
                                          Namespace* namespace_alias_target,
                                          SourceLocation location) {
  return ReflectionIntern(ReflectionCandidate(
      kind, reflected_type, symbol, member, namespace_, parent_class,
      base_index, parameter_index, annotation, scalar_ivalue, scalar_fvalue,
      scalar_is_float, namespace_alias_target, location));
}

ReflectionValue* ReflectionCreateDeserialized(ReflectionEntityKind kind,
                                              SourceLocation location) {
  ReflectionValue* value = calloc(1, sizeof(*value));
  value->kind = kind;
  value->location = location;
  VectorInit(&value->sequence);
  VectorInit(&value->substituted_arguments);
  VectorAppend(&compiler->reflection_values, value);
  return value;
}

ReflectionValue* ReflectionCreateInvalid(SourceLocation location) {
  return ReflectionCreate(kReflectionInvalid, NULL, NULL, NULL, NULL, NULL, 0,
                          location);
}

ReflectionValue* ReflectionCreateType(TypeRecord* type, Symbol* alias,
                                      SourceLocation location) {
  return ReflectionCreate(alias != NULL ? kReflectionTypeAlias : kReflectionType,
                          type, alias, NULL, NULL, NULL, 0, location);
}

ReflectionValue* ReflectionCreateSymbol(ReflectionEntityKind kind,
                                        Symbol* symbol,
                                        SourceLocation location) {
  return ReflectionCreate(kind, symbol != NULL ? symbol->type : NULL, symbol,
                          NULL, symbol != NULL ? symbol->namespace_ : NULL,
                          NULL, 0, location);
}

ReflectionValue* ReflectionCreateMember(StructMember* member,
                                        Struct* parent_class,
                                        SourceLocation location) {
  Symbol* symbol = member != NULL ? member->symbol : NULL;
  ReflectionEntityKind kind = kReflectionClassMember;
  if (member != NULL && member->bit_size > 0 &&
      (symbol == NULL || symbol->name.length == 0)) {
    kind = kReflectionUnnamedBitField;
  } else if (member != NULL && !member->is_static && symbol != NULL &&
             !TypeIsFunction(symbol->type)) {
    kind = kReflectionDataMember;
  } else if (symbol != NULL && TypeIsFunction(symbol->type)) {
    kind = kReflectionFunction;
  } else if (symbol != NULL) {
    kind = kReflectionVariable;
  }
  return ReflectionCreate(kind, symbol != NULL ? symbol->type : NULL, symbol,
                          member, symbol != NULL ? symbol->namespace_ : NULL,
                          parent_class, 0, location);
}

ReflectionValue* ReflectionCreateNamespace(Namespace* namespace_,
                                           SourceLocation location) {
  ReflectionEntityKind kind =
      namespace_ == compiler->global_namespace ? kReflectionGlobalNamespace
                                               : kReflectionNamespace;
  return ReflectionCreate(kind, NULL, NULL, NULL, namespace_, NULL, 0, location);
}

ReflectionValue* ReflectionCreateBase(Struct* parent_class, size_t base_index,
                                      SourceLocation location) {
  TypeRecord* type = NULL;
  if (parent_class != NULL && base_index < parent_class->bases.length) {
    CXXBaseSpecifier* base = parent_class->bases.value.p[base_index];
    type = base != NULL ? base->type : NULL;
  }
  return ReflectionCreate(kReflectionBase, type, NULL, NULL, NULL, parent_class,
                          base_index, location);
}

ReflectionValue* ReflectionCreateParameter(Symbol* symbol, size_t index,
                                           SourceLocation location) {
  return ReflectionCreateExtended(kReflectionFunctionParameter,
                                  symbol != NULL ? symbol->type : NULL, symbol,
                                  NULL, NULL, NULL, 0, index, NULL, 0, 0.0,
                                  false, NULL, location);
}

ReflectionValue* ReflectionCreateScalar(TypeRecord* type, int64_t ivalue,
                                        double fvalue, bool is_float,
                                        SourceLocation location) {
  return ReflectionCreateExtended(kReflectionValue, type, NULL, NULL, NULL,
                                  NULL, 0, 0, NULL, ivalue, fvalue, is_float,
                                  NULL, location);
}

ReflectionValue* ReflectionCreateAnnotation(Attribute* annotation,
                                            SourceLocation location) {
  return ReflectionCreateExtended(kReflectionAnnotation, NULL, NULL, NULL,
                                  NULL, NULL, 0, 0, annotation, 0, 0.0, false,
                                  NULL, location);
}

ReflectionValue* ReflectionCreateNamespaceAlias(Namespace* alias_target,
                                                SourceLocation location) {
  return ReflectionCreateExtended(kReflectionNamespaceAlias, NULL, NULL, NULL,
                                  NULL, NULL, 0, 0, NULL, 0, 0.0, false,
                                  alias_target, location);
}

ReflectionValue* ReflectionCreateDataMemberSpec(
    ReflectionDataMemberSpec* spec, SourceLocation location) {
  ReflectionValue candidate = ReflectionCandidate(
      kReflectionDataMemberDescription, spec != NULL ? spec->member_type : NULL,
      NULL, NULL, NULL, NULL, 0, 0, NULL, 0, 0.0, false, NULL, location);
  candidate.data_member_spec = spec;
  return ReflectionIntern(candidate);
}

ReflectionValue* ReflectionCreateSequence(ReflectionEntityKind kind,
                                          TypeRecord* element_type,
                                          Vector* values,
                                          SourceLocation location) {
  ReflectionValue candidate =
      ReflectionCandidate(kind, element_type, NULL, NULL, NULL, NULL, 0, 0,
                          NULL, 0, 0.0, false, NULL, location);
  if (values != NULL) {
    for (size_t i = 0; i < values->length; i++) {
      VectorAppend(&candidate.sequence, values->value.p[i]);
    }
  }
  return ReflectionIntern(candidate);
}

ReflectionValue* ReflectionCreateWithInitializer(TypeRecord* type,
                                                 ASTNode* initializer,
                                                 SourceLocation location) {
  ReflectionValue candidate = ReflectionCandidate(
      kReflectionValue, type, NULL, NULL, NULL, NULL, 0, 0, NULL, 0, 0.0,
      false, NULL, location);
  candidate.constexpr_initializer = initializer;
  return ReflectionIntern(candidate);
}

ReflectionValue* ReflectionCreateSubstituted(Symbol* template_symbol,
                                             Vector* arguments,
                                             SourceLocation location) {
  ReflectionEntityKind kind = kReflectionInvalid;
  if (template_symbol != NULL) {
    kind = ReflectionKindForSymbol(template_symbol);
  }
  ReflectionValue candidate =
      ReflectionCandidate(kind, template_symbol != NULL ? template_symbol->type
                                                        : NULL,
                          template_symbol, NULL, template_symbol != NULL
                                                   ? template_symbol->namespace_
                                                   : NULL,
                          NULL, 0, 0, NULL, 0, 0.0, false, NULL, location);
  candidate.substituted_template = template_symbol;
  if (arguments != NULL) {
    for (size_t i = 0; i < arguments->length; i++) {
      VectorAppend(&candidate.substituted_arguments,
                   TemplateArgumentCopy(arguments->value.p[i]));
    }
  }
  return ReflectionIntern(candidate);
}

bool ReflectionKindIsTemplate(ReflectionEntityKind kind) {
  return kind == kReflectionTemplate || kind == kReflectionClassTemplate ||
         kind == kReflectionFunctionTemplate ||
         kind == kReflectionVariableTemplate ||
         kind == kReflectionAliasTemplate;
}

bool ReflectionKindIsNamespace(ReflectionEntityKind kind) {
  return kind == kReflectionNamespace || kind == kReflectionGlobalNamespace ||
         kind == kReflectionNamespaceAlias;
}

bool ReflectionValueEqual(const ReflectionValue* left,
                          const ReflectionValue* right) {
  if (left == right) {
    return true;
  }
  if (left == NULL || right == NULL || left->kind != right->kind) {
    return false;
  }
  switch (left->kind) {
    case kReflectionInvalid:
      return true;
    case kReflectionGlobalNamespace:
    case kReflectionNamespace:
      return left->namespace_ == right->namespace_;
    case kReflectionNamespaceAlias:
      return left->namespace_alias_target == right->namespace_alias_target;
    case kReflectionType:
      return TypeEqual(left->reflected_type, right->reflected_type);
    case kReflectionTypeAlias:
    case kReflectionVariable:
    case kReflectionFunction:
    case kReflectionTemplate:
    case kReflectionClassTemplate:
    case kReflectionFunctionTemplate:
    case kReflectionVariableTemplate:
    case kReflectionAliasTemplate:
    case kReflectionConcept:
    case kReflectionEnumerator:
    case kReflectionObject:
    case kReflectionStructuredBinding:
    case kReflectionFunctionParameter:
      return left->symbol == right->symbol &&
             left->parameter_index == right->parameter_index &&
             left->substituted_template == right->substituted_template &&
             ReflectionTemplateArgumentsEqual(&left->substituted_arguments,
                                              &right->substituted_arguments);
    case kReflectionDataMember:
    case kReflectionClassMember:
    case kReflectionUnnamedBitField:
      return left->member == right->member &&
             left->parent_class == right->parent_class;
    case kReflectionDataMemberDescription:
      return ReflectionDataMemberSpecEqual(left->data_member_spec,
                                           right->data_member_spec);
    case kReflectionBase:
      return left->parent_class == right->parent_class &&
             left->base_index == right->base_index;
    case kReflectionValue:
      if (!TypeEqual(left->reflected_type, right->reflected_type)) {
        return false;
      }
      if (left->constexpr_initializer != NULL ||
          right->constexpr_initializer != NULL) {
        return left->constexpr_initializer == right->constexpr_initializer;
      }
      if (left->sequence.length != 0 || right->sequence.length != 0) {
        return ReflectionSequenceEqual(&left->sequence, &right->sequence);
      }
      return left->scalar_is_float == right->scalar_is_float &&
             (left->scalar_is_float
                  ? left->scalar_fvalue == right->scalar_fvalue
                  : left->scalar_ivalue == right->scalar_ivalue);
    case kReflectionAnnotation:
      return left->annotation == right->annotation;
  }
  return false;
}

const char* ReflectionValueIdentifier(const ReflectionValue* value) {
  if (value == NULL) {
    return NULL;
  }
  if (value->symbol != NULL && value->symbol->name.value != NULL) {
    return value->symbol->name.value;
  }
  if (value->data_member_spec != NULL && value->data_member_spec->has_name &&
      value->data_member_spec->name.value != NULL) {
    return value->data_member_spec->name.value;
  }
  if (value->namespace_ != NULL && value->namespace_->name.value != NULL) {
    return value->namespace_->name.value;
  }
  if (value->namespace_alias_target != NULL &&
      value->namespace_alias_target->name.value != NULL) {
    return value->namespace_alias_target->name.value;
  }
  if (value->reflected_type != NULL &&
      TypeIsStructOrUnion(value->reflected_type) &&
      value->reflected_type->info.struct_info != NULL &&
      value->reflected_type->info.struct_info->tag_name != NULL) {
    return value->reflected_type->info.struct_info->tag_name->value;
  }
  if (value->reflected_type != NULL && TypeIsEnum(value->reflected_type) &&
      value->reflected_type->info.enum_info != NULL &&
      value->reflected_type->info.enum_info->tag_name != NULL) {
    return value->reflected_type->info.enum_info->tag_name->value;
  }
  return NULL;
}

TypeRecord* ReflectionValueType(const ReflectionValue* value) {
  return value != NULL && value->reflected_type != NULL
             ? TypeRecordCopy(value->reflected_type)
             : NULL;
}

void ReflectionValueDelete(ReflectionValue* value) {
  if (value == NULL) {
    return;
  }
  ReflectionValueDestructOwned(value);
  TypeRecordDelete(value->reflected_type);
  free(value);
}
