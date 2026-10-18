//
//  type_core.h
//  c_compiler
//
//  TypeRecord lifecycle, factories, and struct/enum shell APIs.
//

#ifndef type_core_h
#define type_core_h

#include <stdio.h>

#include "type_defs.h"

TypeRecord* NewTypeRecord(Type type, Qualifiers quals);
// A record with room for FunctionInfo, for callers that set the declarator
// to kDeclFunction afterwards.
TypeRecord* NewFunctionCapableTypeRecord(Type type, Qualifiers quals);
TypeRecord* NewTypeRecordWithSize(Type type, Qualifiers quals);
TypeRecord* NewComplexTypeRecord(Type element_type, Qualifiers quals);
TypeRecord* NewBitIntTypeRecord(int bit_width, bool is_unsigned,
                                Qualifiers quals);
void TypeRecordDeleteLastReference(TypeRecord* record);
// Most releases only drop one of several references. Keep that common path
// inline and reserve recursive destruction for the final owner.
static inline void TypeRecordDelete(TypeRecord* record) {
  if (record == NULL || record->shared_immutable) {
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
// Bytes retained by the type bump allocator.  Used by DAVECC_PROFILE.
size_t TypeArenaBytes(void);
// Allocation counters for DAVECC_PROFILE.  Full records hold a function type.
size_t TypeArenaFullAllocCount(void);
size_t TypeArenaShortAllocCount(void);
size_t TypeRecordCopyCount(void);
size_t TypeRecordShareCount(void);
// Count TypeRecordCopy calls per call site; the report lists the busiest.
void TypeRecordProfileCallers(void);
void TypeRecordProfileReport(FILE* out, size_t limit);
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
// The class name in `__davecc_vtbl_` / `__davecc_vbtbl_` symbols.
void AppendCXXVTableClassName(String* out, Struct* str);
void TypeRecordChain(TypeRecord* from, TypeRecord* to);
void TypeRecordInvalidateTemplateParameterSummary(TypeRecord* record);
void TypeRecordIncRef(TypeRecord* record);
void TypeRecordDecRef(TypeRecord* record);
TypeRecord* TypeRecordCopy(TypeRecord* record);
// True when template substitution cannot change `type`.  The record must
// still be reference-counted by its owner before a caller shares it.
// Class types are excluded; use TypeRecordCanShareConcreteClass when the
// caller checks that the class is not the one being instantiated.
bool TypeRecordCanShareThroughSubstitution(TypeRecord* type);
bool TypeRecordCanShareConcreteClass(TypeRecord* type);
// One more owner of an existing record.  Used when substitution would have
// copied a type it does not change.
TypeRecord* TypeRecordShareExisting(TypeRecord* type);
// `refs == 0` is updated in place.  A shared record is not modified; a
// qualified copy is returned and the caller keeps ownership of `type`.
TypeRecord* TypeRecordAddQualifiers(TypeRecord* type, Qualifiers quals);
// `type` if this caller may modify it in place, else a private copy.  The
// caller keeps ownership of `type` either way.
TypeRecord* TypeRecordForWrite(TypeRecord* type);
// The type table: each concrete type (TypeRecordCanShareConcreteClass) is
// stored once as an immutable canonical record.  Callers may hold the result
// without a reference and must copy it before modifying it.
bool TypeRecordCanIntern(TypeRecord* type);
// Requires TypeRecordCanIntern(type).
TypeRecord* TypeRecordIntern(TypeRecord* type);
// The canonical record for `type` with its top-level qualifiers replaced.
// Requires TypeRecordCanIntern(type).
TypeRecord* TypeRecordInternQualified(TypeRecord* type, Qualifiers quals);
// The canonical record when `type` is concrete, else TypeRecordCloneSpine.
TypeRecord* TypeRecordInternOrCloneSpine(TypeRecord* type);
// The canonical record when `type` is concrete, else TypeRecordCopy.
TypeRecord* TypeRecordInternOrCopy(TypeRecord* type);
// For template-argument storage: also shares a bare template parameter.
// Rebasing an argument clones its type first, so the shared record is never
// renumbered in place.
TypeRecord* TypeRecordInternArgumentOrCopy(TypeRecord* type);
size_t TypeRecordInternHitCount(void);
size_t TypeRecordInternMissCount(void);
// DAVECC_PROFILE_CENSUS: how many live records are structural duplicates.
void TypeRecordProfileCensus(FILE* out);
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
  overlay->value = TypeRecordValue(base);
  SetQualifiers(&overlay->value, qualifiers);
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
// The object with static storage that an unanalyzed argument names, which a
// reference parameter can bind to; NULL if there is none.
Symbol* TemplateArgumentLvalueSymbol(struct ASTNode* expr);
// Records that |arg| named |symbol|.  An argument that is not a constant value
// becomes a reference to it, unless |may_be_dependent| and |symbol| is a
// constant whose value is not known yet.  Returns whether |arg| is usable.
bool TemplateArgumentSetLvalue(TemplateArgument* arg, Symbol* symbol,
                               bool value_ok, bool may_be_dependent);
// Converts |arg| to a binding of the reference parameter type
// |parameter_type| if it names a compatible object.
bool TemplateArgumentBindReferenceParameter(TemplateArgument* arg,
                                            TypeRecord* parameter_type);
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
void TypeRecordRemoveImplicitThisParameter(TypeRecord* func);
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
