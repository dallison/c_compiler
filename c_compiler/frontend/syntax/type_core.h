//
//  type_core.h
//  c_compiler
//
//  TypeRecord lifecycle, factories, and struct/enum shell APIs.
//

#ifndef type_core_h
#define type_core_h

#include "type_defs.h"

TypeRecord* NewTypeRecord(Type type, Qualifiers quals);
TypeRecord* NewTypeRecordWithSize(Type type, Qualifiers quals);
TypeRecord* NewComplexTypeRecord(Type element_type, Qualifiers quals);
TypeRecord* NewBitIntTypeRecord(int bit_width, bool is_unsigned,
                                Qualifiers quals);
void TypeRecordDeleteLastReference(TypeRecord* record);
// Most releases only drop one of several references. Keep that common path
// inline and reserve recursive destruction for the final owner.
static inline void TypeRecordDelete(TypeRecord* record) {
  if (record == NULL) {
    return;
  }
  if (record->refs > 1) {
    record->refs--;
    return;
  }
  TypeRecordDeleteLastReference(record);
}
// Free every TypeRecord struct allocated from the type arena.  Call once, at
// CompilerDestruct, after all type-referencing structures are torn down.
void TypeRecordArenaRelease(void);
// Allocate/recycle TemplateArgument shells while preserving normal payload
// destruction. Release the backing blocks with the rest of frontend teardown.
TemplateArgument* TemplateArgumentAlloc(void);
void TemplateArgumentArenaRelease(void);
// Free every Struct info (and its members) in one pass.  Call once, at
// CompilerDestruct, after the AST/symbols/tags are gone but before
// TypeRecordArenaRelease.  Structs are freed here rather than by refcount
// because they can form reference cycles.
void StructRegistryRelease(void);
TypeRecord* TypeRecordCalculateSize(TypeRecord* record);
uint64_t TypeRecordSemanticIdentityHash(TypeRecord* record);
// Bind a type node to a class layout and register it for late size updates.
// This does not alter Struct reference counts.
void TypeRecordSetStructInfo(TypeRecord* record, Struct* str);
void TypeRecordSyncStructSizes(Struct* str);
void TypeRecordChain(TypeRecord* from, TypeRecord* to);
void TypeRecordInvalidateTemplateParameterSummary(TypeRecord* record);
void TypeRecordIncRef(TypeRecord* record);
void TypeRecordDecRef(TypeRecord* record);
TypeRecord* TypeRecordCopy(TypeRecord* record);
// Copies every node in a type's declarator spine so callers may safely mutate
// qualifiers, template indices, or next links without touching shared nodes.
TypeRecord* TypeRecordCloneSpine(TypeRecord* record);
// A scoped, non-owning top-level view.  It shares every payload and declarator
// tail with `base`, but lets read-only algorithms observe different qualifiers
// without allocation or deep copying.  The returned pointer must not escape the
// scope, be passed to TypeRecordDelete, or be structurally mutated.
typedef struct {
  TypeRecord value;
} TypeRecordQualifierOverlay;

static inline TypeRecord* TypeRecordOverlayQualifiers(
    TypeRecordQualifierOverlay* overlay, TypeRecord* base,
    Qualifiers qualifiers) {
  if (overlay == NULL || base == NULL) {
    return NULL;
  }
  overlay->value = *base;
  overlay->value.qualifiers = qualifiers;
  return &overlay->value;
}
int TypeRecordAlignment(TypeRecord* record);
void TemplateParameterDelete(TemplateParameter* param);
Vector* TemplateParameterVectorCopy(Vector* params);
void TemplateArgumentDelete(TemplateArgument* arg);
TemplateArgument* NewTypeTemplateArgument(TypeRecord* type);
TemplateArgument* NewIntegralTemplateArgument(long long value);
TemplateArgument* NewTemplateTemplateArgument(Symbol* symbol,
                                               int parameter_index);
TemplateValueKind TemplateArgumentConcreteValueKind(
    const TemplateArgument* arg);
bool TemplateArgumentSetFromExpression(TemplateArgument* arg,
                                       struct ASTNode* expr);
bool TemplateArgumentValuesEqual(const TemplateArgument* left,
                                 const TemplateArgument* right);
bool TemplateArgumentEqual(TemplateArgument* left, TemplateArgument* right);
struct ASTNode* TemplateArgumentMaterializeExpression(
    const TemplateArgument* arg, SourceLocation location);
Vector* TemplateArgumentVectorCopy(Vector* args);
TypeRecord* NewPointerTypeRecord(Qualifiers quals);
TypeRecord* NewMemberPointerTypeRecord(Struct* class_info, Qualifiers quals);
TypeRecord* TypeMemberPointerPointeeFromMember(StructMember* member);
TypeRecord* NewReferenceTypeRecord(Qualifiers quals, bool rvalue);
TypeRecord* NewArrayTypeRecord(Qualifiers quals, bool is_static);
TypeRecord* NewBasicArrayTypeRecord(Qualifiers quals, int size, bool is_flexible);
TypeRecord* NewVectorTypeRecord(TypeRecord* element_type, int lane_count);

TypeRecord* NewFunctionTypeRecord(void);
ContractAssertion* NewContractAssertion(ContractAssertionKind kind,
                                        struct ASTNode* predicate,
                                        Symbol* result_binding,
                                        Vector* attributes,
                                        SourceLocation location);
void ContractAssertionDelete(ContractAssertion* assertion);
void TypeRecordCopyContractAssertions(TypeRecord* to, TypeRecord* from);
TypeRecord* NewPointerTo(Qualifiers quals, TypeRecord* type);
Symbol* NewCXXThisSymbol(Struct* owner, bool is_const_member,
                         bool is_volatile_member, SourceLocation location);
bool FunctionHasImplicitThisParameter(TypeRecord* func);
bool FunctionHasExplicitObjectParameter(TypeRecord* func);
void TypeRecordAddCXXThisParameter(TypeRecord* func, Struct* owner,
                                   SourceLocation location);

StructMember* NewStructMember(Symbol* symbol);
Struct* NewStruct(bool is_union);
void StructDelete(Struct* s);
void StructMemberDelete(StructMember* member);

// Records a C++ 'friend class X;' relationship: members of friend_class may
// access the private and protected members of str.  Duplicates are ignored.
void StructAddFriendClass(Struct* str, Struct* friend_class);
CXXFriendTypeDeclaration* NewCXXFriendTypeDeclaration(
    TypeRecord* type, bool is_pack_expansion, SourceLocation location);
void CXXFriendTypeDeclarationDelete(CXXFriendTypeDeclaration* declaration);
// Records a C++ 'friend <function>;' relationship: the named function may
// access the private and protected members of str.  Duplicates are ignored.
void StructAddFriendFunction(Struct* str, Symbol* friend_function);
bool StructMemberIsBitField(StructMember* member);
// True when a class/struct/union type has a completed definition.  This is
// intentionally separate from size: complete empty C++ classes have a size,
// while a forward declaration is represented by its owning tag symbol.
bool TypeIsCompleteClass(TypeRecord* type);

Symbol* NewEnumConstant(const char* name, int64_t value);
Symbol* NewScopedEnumConstant(const char* name, int64_t value,
                              TypeRecord* enum_type);
Enum* NewEnum(void);
void EnumDelete(Enum* e);
Symbol* EnumFindConstant(Enum* e, String* name);
// What is the size of a given type in bytes?
int SizeofType(Type type);
int SizeofPointer(void);
int SizeofLongDouble(void);
TypeRecord* NewSizeTypeRecord(void);

#endif /* type_core_h */
