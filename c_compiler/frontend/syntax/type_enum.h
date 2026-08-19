//
//  type_enum.h
//  c_compiler
//

#ifndef type_enum_h
#define type_enum_h

#include "type_parse.h"

typedef enum {
  kEnumValueAssignOk,
  kEnumValueAssignOverflow,
  kEnumValueAssignNotRepresentable,
} EnumValueAssignStatus;

Symbol* TypeParserParseEnum(TypeParser* parser);

bool EnumFixedValueIsRepresentable(const Enum* e, int64_t value);
EnumValueAssignStatus EnumAssignEnumeratorValue(Enum* e, bool has_explicit_value,
                                                int64_t explicit_value,
                                                bool* inout_implicit_overflow,
                                                int64_t* out_assigned_value);
Symbol* EnumAddScopedConstant(Enum* e, TypeRecord* enum_type, const char* name,
                              int64_t value);
void EnumRemoveConstantsFrom(Enum* e, size_t old_count);
void EnumCompleteDefinition(Enum* e, TypeRecord* enum_type, Symbol* tag);

#endif /* type_enum_h */
