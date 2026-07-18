//
//  type_class_internal.h
//  c_compiler
//
//  Shared implementation declarations among class/member modules.
//

#ifndef type_class_internal_h
#define type_class_internal_h

#include "type.h"
#include "type_template_internal.h"

struct Syntax;
struct CXXConstructorInitList;

bool StructMemberIsNestedType(StructMember* member);
bool StructHasMemberFunction(Struct* str);
bool TypeIsTemplateParameterPlaceholder(TypeRecord* type, int* index);
bool CurrentTemplateParameterIsPack(struct Syntax* syntax, int index);

void AlignNextOffset(Struct* str, TypeRecord* type);
void AlignNextOffsetForSymbol(Struct* str, Symbol* symbol);
void StructInsertMemberIntoTables(Struct* str, StructMember* member);
void AddStructMember(TypeParser* parser, Struct* str, StructMember* member);
void AppendStructMemberOverload(TypeParser* parser, Struct* str,
                                StructMember* first, StructMember* member);
void UpdateStructSize(Struct* str, TypeRecord* member_type, bool is_union);
void FinalizeStructAlignment(Struct* str);
void StructApplyLayoutAttributes(Struct* str, Vector* attrs);
bool RelayoutStruct(Struct* str);
void StructAddSyntheticMember(Struct* str, StructMember* member);

void LayoutCXXBaseSpecifiers(Struct* str);
void CollectCXXVirtualBases(Struct* str);
void CopyCXXBaseVirtualMembers(Struct* str);
bool StructHasVirtualBases(Struct* str);
void ParseCXXBaseSpecifiers(TypeParser* parser, Vector* bases, bool is_union,
                            bool is_class);
void RegisterCXXVirtualMember(TypeParser* parser, Struct* str,
                              StructMember* member);
void RegisterCXXVTable(TypeParser* parser, Struct* str);
void RegisterCXXVBTables(TypeParser* parser, Struct* str);
int CXXBaseOffsetForMember(Struct* str, StructMember* member);
void UpdateCXXAbstractStatus(Struct* str);
void AddCXXVPtrMember(TypeParser* parser, Struct* str);
void AddCXXVBPtrMember(TypeParser* parser, Struct* str);
void LayoutCXXVirtualBaseSpecifiers(Struct* str);
bool ParseCXXClassFinalSpecifier(TypeParser* parser);

void FinalizePendingInlineConstructorPreambles(TypeParser* parser,
                                               Struct* owner);

bool StructHasBaseStruct(Struct* str, Struct* target, int* offset);
bool StructHasBaseType(struct Syntax* syntax, Struct* str, TypeRecord* target,
                       int* offset);
void ParseBitField(TypeParser* parser, bool is_union, Struct* str,
                   Symbol* member_symbol, StructMember* member);

void AppendCXXMemberDestructorCalls(TypeRecord* func, Vector* body,
                                    SourceLocation location);
void AppendCXXSingleMemberDestructorCalls(TypeRecord* func,
                                          StructMember* member, Vector* body,
                                          SourceLocation location);
void SynthesizeDefaultedMemberFunctionBody(TypeParser* parser, Symbol* symbol);
Symbol* NewCXXConversionOperatorSymbol(TypeParser* parser, Struct* owner,
                                       TypeRecord* return_type,
                                       SourceLocation location, bool is_virtual);
bool SymbolIsCXXAllocationFunction(Symbol* symbol);
bool CXXClassNameMatchesUnqualifiedTemplateName(String* class_name,
                                                String* spelling);

void ComputeCXXAggregateStatus(Struct* str);
void AddImplicitCXXSpecialMembers(TypeParser* parser, Struct* str, Symbol* tag);
// After a class's layout is finalized (vptr and virtual-base pointers added),
// mark its special members non-trivial if the class is polymorphic or has
// virtual bases.  Such special members must run construction/destruction code
// (vptr/vbptr setup), so they must not be treated as trivial (which would let
// global static-init skip them, leaving those pointers uninitialized).
void CXXFixupSpecialMemberTrivialityAfterLayout(Struct* str);
void AddImplicitLambdaClosureSpecialMembers(struct Syntax* syntax, Struct* str,
                                            Symbol* tag, bool has_capture_fields,
                                            bool has_explicit_template_params);
void LambdaClosureRemoveEmptyPlaceholder(Struct* str);
void AddImplicitCXXDestructorIfNeeded(TypeParser* parser, Struct* str,
                                      Symbol* tag);
void AddImplicitCXXDeductionGuides(Struct* str, Symbol* tag);

void ApplyCXXMemberUsingDeclarations(TypeParser* parser, Struct* owner,
                                     Struct* template_struct, Vector* args);
struct CXXConstructorInitList* FindTemplateConstructorInitializers(Symbol* symbol);
void CopyTemplateConstructorInitializersKey(Symbol* from, Symbol* to);

// Insert and analyze the constructor member-initializer preamble for a freshly
// cloned constructor instantiation `symbol` (whose body was cloned from
// `template_definition` with template arguments `args`).  Shared by the
// class-instantiation queuing path and the per-call function-template
// instantiation path; the latter is required for member function *template*
// constructors, whose preamble is intentionally deferred from class
// instantiation until their own arguments are known.  No-op if `symbol` is not
// a constructor or has no cloned compound body.
void SyntaxInsertClonedTemplateConstructorPreamble(TypeParser* parser,
                                                    Symbol* template_definition,
                                                    Symbol* symbol,
                                                    Vector* args);

StructMember* FindStructMember(Struct* str, String* name);
StructMember* FindStructMemberByName(Struct* str, const char* name);
StructMember* FindStructMemberWithAccess(Struct* str, String* name,
                                         CXXAccess* access, Struct** owner);
StructMember* FindStructMemberOverload(StructMember* first, TypeRecord* type);
void CollectConversionOperators(Struct* str, Vector* out);

void ParseStructMembers(TypeParser* parser, Struct* str, bool is_union,
                        String* tag_name);
void CXXFinalizeSpecialMemberMetadata(Symbol* symbol, Struct* owner,
                                      bool user_declared);
void ParseCXXPureSpecifier(TypeParser* parser, TypeRecord* func);
void QueueInlineMemberFunctionDefinition(Symbol* symbol);

#endif /* type_class_internal_h */
