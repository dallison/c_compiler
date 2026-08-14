//
//  reflection.h
//  c_compiler
//

#ifndef reflection_h
#define reflection_h

#include "type.h"
#include "symbol_table.h"

typedef enum {
  kReflectionInvalid,
  kReflectionGlobalNamespace,
  kReflectionNamespace,
  kReflectionType,
  kReflectionTypeAlias,
  kReflectionVariable,
  kReflectionFunction,
  kReflectionTemplate,
  kReflectionEnumerator,
  kReflectionDataMember,
  kReflectionBase,
} ReflectionEntityKind;

// A reflection value is compiler-owned and remains valid for the translation
// unit.  Entity pointers are borrowed; reflected_type is reference counted.
// Modules serialize these fields rather than the process-local pointer.
typedef struct ReflectionValue {
  ReflectionEntityKind kind;
  TypeRecord* reflected_type;
  Symbol* symbol;
  StructMember* member;
  Namespace* namespace_;
  Struct* parent_class;
  size_t base_index;
  SourceLocation location;
} ReflectionValue;

ReflectionValue* ReflectionCreate(ReflectionEntityKind kind,
                                  TypeRecord* reflected_type, Symbol* symbol,
                                  StructMember* member, Namespace* namespace_,
                                  Struct* parent_class, size_t base_index,
                                  SourceLocation location);
// Module reader hook: allocate a registry-owned shell whose entity references
// will be populated by the deserializer's reference-fixup pass.
ReflectionValue* ReflectionCreateDeserialized(ReflectionEntityKind kind,
                                              SourceLocation location);
ReflectionValue* ReflectionCreateInvalid(SourceLocation location);
ReflectionValue* ReflectionCreateType(TypeRecord* type, Symbol* alias,
                                      SourceLocation location);
ReflectionValue* ReflectionCreateSymbol(ReflectionEntityKind kind,
                                        Symbol* symbol,
                                        SourceLocation location);
ReflectionValue* ReflectionCreateMember(StructMember* member,
                                        Struct* parent_class,
                                        SourceLocation location);
ReflectionValue* ReflectionCreateNamespace(Namespace* namespace_,
                                           SourceLocation location);
ReflectionValue* ReflectionCreateBase(Struct* parent_class, size_t base_index,
                                      SourceLocation location);

bool ReflectionValueEqual(const ReflectionValue* left,
                          const ReflectionValue* right);
const char* ReflectionValueIdentifier(const ReflectionValue* value);
TypeRecord* ReflectionValueType(const ReflectionValue* value);

void ReflectionValueDelete(ReflectionValue* value);

#endif /* reflection_h */
