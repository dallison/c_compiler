//
//  reflection.c
//  c_compiler
//

#include "reflection.h"

#include "compiler.h"
#include "type_compare.h"

#include <stdlib.h>

ReflectionValue* ReflectionCreate(ReflectionEntityKind kind,
                                  TypeRecord* reflected_type, Symbol* symbol,
                                  StructMember* member, Namespace* namespace_,
                                  Struct* parent_class, size_t base_index,
                                  SourceLocation location) {
  ReflectionValue candidate = {
      .kind = kind,
      .reflected_type = reflected_type,
      .symbol = symbol,
      .member = member,
      .namespace_ = namespace_,
      .parent_class = parent_class,
      .base_index = base_index,
      .location = location,
  };
  for (size_t i = 0; i < compiler->reflection_values.length; i++) {
    ReflectionValue* existing = compiler->reflection_values.value.p[i];
    if (ReflectionValueEqual(existing, &candidate)) {
      return existing;
    }
  }

  ReflectionValue* value = ReflectionCreateDeserialized(kind, location);
  value->reflected_type =
      reflected_type != NULL ? TypeRecordCopy(reflected_type) : NULL;
  value->symbol = symbol;
  value->member = member;
  value->namespace_ = namespace_;
  value->parent_class = parent_class;
  value->base_index = base_index;
  return value;
}

ReflectionValue* ReflectionCreateDeserialized(ReflectionEntityKind kind,
                                              SourceLocation location) {
  ReflectionValue* value = calloc(1, sizeof(*value));
  value->kind = kind;
  value->location = location;
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
  ReflectionEntityKind kind =
      member != NULL && !member->is_static && symbol != NULL &&
              !TypeIsFunction(symbol->type)
          ? kReflectionDataMember
          : symbol != NULL && TypeIsFunction(symbol->type)
                ? kReflectionFunction
                : kReflectionVariable;
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
    case kReflectionType:
      return TypeEqual(left->reflected_type, right->reflected_type);
    case kReflectionTypeAlias:
    case kReflectionVariable:
    case kReflectionFunction:
    case kReflectionTemplate:
    case kReflectionEnumerator:
      return left->symbol == right->symbol;
    case kReflectionDataMember:
      return left->member == right->member &&
             left->parent_class == right->parent_class;
    case kReflectionBase:
      return left->parent_class == right->parent_class &&
             left->base_index == right->base_index;
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
  if (value->namespace_ != NULL && value->namespace_->name.value != NULL) {
    return value->namespace_->name.value;
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
  TypeRecordDelete(value->reflected_type);
  free(value);
}
