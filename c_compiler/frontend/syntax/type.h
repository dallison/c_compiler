//
//  type.h
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef type_h
#define type_h

#include "lex.h"
#include "symbol.h"
#include "vector.h"
#include "map.h"
#include "parser_context.h"

struct ASTNode;
struct ConstraintExpr;

// Basic type (int, long, etc.)
typedef enum {
  kTypeImplicit = 0,
  kTypeChar = 1 << 0,
  kTypeInt = 1 << 1,
  kTypeShort = 1 << 2,
  kTypeLong = 1 << 3,
  kTypeLongLong = 1 << 4,
  kTypeFloat = 1 << 5,
  kTypeDouble = 1 << 6,
  kTypeLongDouble = 1 << 7,
  kTypeBool = 1 << 8,
  kTypeVoid = 1 << 9,
  kTypeStruct = 1 << 10,
  kTypeUnion = 1 << 11,
  kTypeEnum = 1 << 12,
  kTypeSigned = 1 << 13,
  kTypeUnsigned = 1 << 14,
  kTypeUnknown = 1 << 15,
  kTypeAuto = 1 << 16,
  kTypeNullPointer = 1 << 17,
} Type;

// The last bit position in the type specifier that corresponds to a
// unique type (not including signed and unsigned).
//  This is used to test for a invalid combination of types.
#define TYPE_LAST_BIT 17

// Type qualifiers, multiple active at the same time.
typedef enum {
  kQualPlain = 0,
  kQualConst = 4,
  kQualVolatile = 8,
  kQualRestrict = 16,
} Qualifiers;

// Type declarator (pointer, array, ...)
typedef enum {
  kDeclPrimitive,
  kDeclPointer,
  kDeclReference,
  kDeclRValueReference,
  kDeclArray,
  kDeclFunction,
} Declarator;

typedef struct Struct Struct;

typedef enum {
  kTemplateParameterType,
  kTemplateParameterNonType,
} TemplateParameterKind;

// Serialized as an inline sub-message (see type_serialize.c); the field
// numbers below are local to that sub-message.
typedef struct TemplateParameter {
  String name;                      // @wire 1
  TemplateParameterKind kind;       // @wire 2
  bool is_parameter_pack;           // @wire 3
  struct TypeRecord* type;          // NULL for type params.       // @wire 4
  struct TypeRecord* default_type;  // Optional type-param default. // @wire 5
  bool has_default_int;  // Optional non-type integer default.      // @wire 6
  long long default_int_value;                                     // @wire 7
  int default_template_parameter_index;  // >=0 if default names a param. // @wire 8
  struct ConstraintExpr* associated_constraint;  // Optional C++20 constraint. // @wire 10
  int index;                        // @wire 9
} TemplateParameter;

// Serialized as an inline sub-message (see type_serialize.c); the field
// numbers below are local to that sub-message.
typedef struct TemplateArgument {
  TemplateParameterKind kind;    // @wire 1
  bool is_pack_expansion;        // @wire 2
  struct TypeRecord* type;  // Non-NULL for type arguments.        // @wire 3
  long long int_value;      // Valid for simple non-type int args. // @wire 4
  int template_parameter_index;  // >=0 when non-type arg is a param. // @wire 5
  Vector* pack_arguments;   // TemplateArgument* for bound packs.   // @wire 6
  // A value-dependent non-type argument expression (e.g. `!is_integral<It>::value`)
  // kept unevaluated at parse time.  It is re-cloned, substituted, and folded to a
  // concrete `int_value` when the referenced template parameters become concrete
  // (see NewSubstitutedTemplateArgument).  Arena-owned (never individually freed);
  // NULL for ordinary, already-evaluated arguments.
  struct ASTNode* dependent_expr;   // @wire 7
  // Source location of the argument as written in the template *definition*.
  // Substitution reuses (and copies) this node long after the lexer has moved
  // on, so diagnostics raised during substitution (e.g. an ill-formed pack
  // expansion) must report here rather than at the unrelated instantiation
  // point.  SOURCE_LOCATION_MISSING when unknown (synthesized arguments).
  SourceLocation location;   // @wire 8
} TemplateArgument;

typedef struct ClassTemplatePartialSpecialization {
  Symbol* tag_symbol;       // Parsed specialization body tag (not owned).
  Vector template_parameters;  // TemplateParameter* entries owned.
  Vector pattern_arguments;    // TemplateArgument* entries owned.
} ClassTemplatePartialSpecialization;

typedef enum {
  kCXXSpecialMemberNone,
  kCXXSpecialMemberDefaultConstructor,
  kCXXSpecialMemberDestructor,
  kCXXSpecialMemberCopyConstructor,
  kCXXSpecialMemberMoveConstructor,
  kCXXSpecialMemberCopyAssignment,
  kCXXSpecialMemberMoveAssignment,
} CXXSpecialMemberKind;

typedef enum {
  kCXXRefQualifierNone,
  kCXXRefQualifierLValue,
  kCXXRefQualifierRValue,
} CXXRefQualifier;

// Function info.  Serialized as an inline sub-message of TypeRecord (see
// type_serialize.c); the field numbers below are local to that sub-message.
typedef struct {
  Symbol* symbol;       // Symbol for function (or NULL).           // @wire 1
  Vector prototype;     // Formal arguments (owned Symbol*).        // @wire 2
  bool varargs;         // True if varargs function.                // @wire 3
  struct ASTNode* body; // Body AST.                                // @wire 4
  bool unknown_args;    // Old-style or invented function.          // @wire 5
  bool definition;      // Function is a definition.                // @wire 6
  bool old_style;       // Old-style arguments.                     // @wire 7
  bool is_inline;       // This is an inline function.              // @wire 8
  bool is_constexpr;    // C++ constexpr function.                  // @wire 9
  bool is_consteval;    // C++ consteval immediate function.        // @wire 10
  bool is_constructor;  // Called before main.                      // @wire 11
  bool is_destructor;   // Called after exit.                       // @wire 12
  bool is_const_member; // C++ trailing const qualifier.            // @wire 13
  CXXRefQualifier ref_qualifier;  // C++ trailing & / &&.           // @wire 14
  bool is_explicit;     // C++ explicit ctor/conv/deduction guide.  // @wire 15
  bool is_explicit_conversion;  // C++ explicit conversion op.      // @wire 16
  bool is_virtual;      // C++ virtual member function.             // @wire 17
  bool is_override;     // C++ override virt-specifier.             // @wire 18
  bool is_final;        // C++ final virt-specifier.                // @wire 19
  bool is_pure_virtual; // C++ pure virtual function (`= 0`).       // @wire 20
  bool is_defaulted;    // C++ explicitly defaulted (`= default`).  // @wire 21
  bool is_deleted;      // C++ deleted function (`= delete`).       // @wire 22
  CXXSpecialMemberKind cxx_special_member_kind;  // Special kind.   // @wire 23
  bool is_user_declared;      // C++ user-declared function.        // @wire 24
  bool is_user_provided;      // C++ user-provided function body.   // @wire 25
  bool is_explicitly_defaulted;  // C++ explicitly defaulted.       // @wire 26
  bool is_explicitly_deleted;    // C++ explicitly deleted.         // @wire 27
  bool is_implicitly_declared;   // C++ implicitly declared.        // @wire 28
  bool is_implicitly_deleted;    // C++ implicitly deleted.         // @wire 29
  bool is_trivial_special_member;  // C++ trivial special member.   // @wire 30
  bool is_constexpr_eligible;  // C++ constexpr-suitable.           // @wire 31
  bool is_noexcept_eligible;   // C++ nothrow special member.       // @wire 32
  bool is_noexcept;            // C++ declared non-throwing.        // @wire 33
  bool is_auto_return_deduced;  // C++ auto return deduced.         // @wire 34
  bool is_deduction_guide;  // C++ class template deduction guide.  // @wire 35
  bool is_coroutine;  // C++ coroutine function.                    // @wire 36
  struct TypeRecord* coroutine_promise_type;  // Promise type.      // @wire 37
  struct TypeRecord* coroutine_frame_type;  // Lowered frame.       // @wire 38
  int coroutine_suspend_count;  // Suspension points in body.       // @wire 39
  int virtual_index;    // Vtable slot, or -1 if non-virtual.       // @wire 40
  Struct* cxx_member_owner;  // Owning class for member functions.  // @wire 41
  Symbol* template_origin;  // Primary template for instantiations. // @wire 42
  int template_parameter_count;  // C++ function template arity.    // @wire 43
  int template_parameter_base;  // Param index base for nesting.    // @wire 44
  Vector template_parameters;  // TemplateParameter* for defaults.  // @wire 45
  Vector template_instantiations;  // Symbol* cache, not overload candidates. // @wire 46
  struct ConstraintExpr* associated_constraint;  // Optional C++20 requires-clause. // @wire 47
  struct ASTNode* explicit_condition;  // Deferred value-dependent explicit(bool). // @wire 48
} FunctionInfo;

typedef enum {
  kAccessPublic,
  kAccessProtected,
  kAccessPrivate,
} CXXAccess;

// Serialized as an inline sub-message (see type_serialize.c); field numbers
// below are local to that sub-message.
typedef struct CXXBaseSpecifier {
  struct TypeRecord* type;  // Base class type.   // @wire 1
  CXXAccess access;         // @wire 2
  int byte_offset;          // @wire 3
  bool is_virtual;          // @wire 4
  bool is_pack_expansion;   // @wire 5
} CXXBaseSpecifier;

typedef struct CXXMemberUsingDeclaration {
  struct TypeRecord* base_type;  // Nested-name-specifier base.
  String member_name;
  CXXAccess access;
  SourceLocation location;
  bool is_pack_expansion;
} CXXMemberUsingDeclaration;

typedef struct CXXVirtualBaseInfo {
  struct TypeRecord* type;  // Virtual base class type.
  CXXAccess access;
  int byte_offset;      // Complete-object offset.
  int vbtable_index;    // Index in this class's vbtable.
} CXXVirtualBaseInfo;

typedef enum {
  kCXXBaseAdjustmentNone,
  kCXXBaseAdjustmentStatic,
  kCXXBaseAdjustmentVirtual,
} CXXBaseAdjustmentKind;

typedef struct CXXBaseAdjustment {
  CXXBaseAdjustmentKind kind;
  int byte_offset;
  int vbtable_index;
} CXXBaseAdjustment;

typedef struct CXXVBTableInfo {
  struct Struct* source;
  int source_offset;
  Symbol* symbol;
} CXXVBTableInfo;

typedef struct CXXVTableInfo {
  struct Struct* source;
  int source_offset;
  Symbol* symbol;
} CXXVTableInfo;

// A struct or union member.  Behaves like a Symbol with extra information.
// Module-serialization field numbers (see type_serialize.c).
typedef struct StructMember {
  Symbol* symbol;   // Embedded Symbol.                            // @wire 1
  struct ASTNode* default_initializer;  // C++ default member init. // @wire 2
  int byte_offset;  // Byte offset into struct.                    // @wire 3
  int bit_offset;   // Bit offset into word.                       // @wire 4
  int bit_size;     // Bitfield size in bits.                      // @wire 5
  size_t index;     // Index into members vector.                  // @wire 6
  int cxx_vcall_offset;  // Subobject offset owning this vslot.     // @wire 7
  bool is_anon;     // This is an anonymous member.                // @wire 8
  bool is_static;   // C++ static data/function member.            // @wire 9
  bool is_mutable;  // C++ 'mutable' data member.                  // @wire 10
  bool is_member_function;                                         // @wire 11
  bool is_using_declaration;  // Imported by member using-decl.    // @wire 12
  CXXAccess access;                                                // @wire 13
  struct StructMember* overload_next;  // Next member overload.    // @wire 14
} StructMember;

// A struct or union type.  Module-serialization field numbers (see
// type_serialize.c).  refs is recomputed on load; the derived member maps,
// vtable/vbtable info, using-declarations, partial specializations and
// deduction guides carry no wire number (rebuilt or deferred).
struct Struct {
  int refs;          // @wire - (refcount, recomputed)
  String* tag_name;  // Tag name (owned by Symbol).               // @wire 1
  Symbol* tag_symbol;  // Owning tag symbol, if named.            // @wire 2
  Vector bases;      // CXXBaseSpecifier* (owns entries).         // @wire 3
  Vector friend_classes;    // Struct* granted friendship.        // @wire 28
  Vector friend_functions;  // Symbol* granted friendship.        // @wire 29
  Vector member_using_declarations;  // @wire - (not serialized)
  Vector virtual_bases;  // @wire - (recomputed on layout)
  Vector members;    // StructMember* (owns members).             // @wire 4
  Vector virtual_members;  // StructMember* by slot (not owned).  // @wire 27
  StructMember* vptr_member;  // Hidden C++ vptr field.           // @wire 23
  Symbol* vtable_symbol;      // Hidden C++ vtable static symbol.  // @wire 24
  StructMember* vbptr_member;  // Hidden virtual-base tbl ptr.    // @wire 25
  Symbol* vbtable_symbol;      // Hidden virtual-base table.       // @wire 26
  Vector vtable_symbols;  // @wire - (recomputed)
  Vector vbtable_symbols;  // @wire - (recomputed)
  Map symbol_table;  // @wire - (rebuilt from members)
  Map symbol_name_table;  // @wire - (rebuilt from members)
  int next_offset;   // Byte offset of next member.               // @wire 5
  int size;          // Size of struct in bytes.                  // @wire 6
  int non_virtual_size;  // Size excluding virtual base subobjs.   // @wire 7
  int alignment;     // Alignment (max member alignment).         // @wire 8
  bool is_union;     // True if this is a union.                  // @wire 9
  bool is_class;     // True if this is a C++ class.              // @wire 10
  bool is_final;     // True if declared 'final'.                 // @wire 11
  bool is_template;  // True if this is a C++ class template.     // @wire 12
  bool is_aggregate; // True if this is a C++ aggregate class.    // @wire 13
  bool cxx_special_members_complete;  // Special members declared. // @wire 14
  // True once RegisterCXXVTable has run for this class (i.e. vtable_symbols is
  // populated).  Constructor preambles built before this point defer their
  // __vptr initializers, since the vtables they reference do not exist yet.
  bool vtables_registered;  // @wire - (recomputed)
  Vector template_parameters;  // TemplateParameter* entries.     // @wire 15
  Vector partial_specializations;  // @wire - (deferred)
  Vector deduction_guides;  // @wire - (deferred)
  int template_parameter_count;  // Simple template arity.        // @wire 16
  bool packed;       // packed: no inter-member padding.          // @wire 17
  bool is_abstract;  // Has an unimplemented pure virtual.        // @wire 18
  int explicit_alignment;  // aligned(N) minimum; 0 = none.        // @wire 19
  int pack;          // #pragma pack(n) cap; 0 = no cap.          // @wire 20
  int next_bit_pos;  // Next bit position for bit fields.         // @wire 21
  int current_offset;  // @wire 22
};

// Applies layout-affecting attributes (packed, aligned) from an Attribute
// vector to a struct.  Must be called before the struct is laid out (or
// followed by a re-layout for the trailing/typedef form).
void StructApplyLayoutAttributes(struct Struct* str, Vector* attrs);
// Propagates packed/aligned attributes from a (typedef) symbol onto its
// struct/union type and re-lays out the struct.  No-op if the symbol carries
// no layout attribute or its type is not a struct/union.
void TypeApplyStructAttributesFromSymbol(Symbol* sym);

// An enum type.  Module-serialization field numbers (see type_serialize.c);
// refs is recomputed on load.
typedef struct {
  int refs;          // @wire - (refcount, recomputed)
  String* tag_name;  // Tag name (owned by Symbol).               // @wire 1
  Symbol* tag_symbol;  // Owning tag symbol, if named.            // @wire 2
  Vector constants;  // Symbol* constants (not owned).            // @wire 3
  int next_value;    // Value to give to next constant.           // @wire 4
  bool is_scoped;    // C++ scoped enum (enum class/struct).      // @wire 5
  bool has_fixed_underlying;                                      // @wire 6
  Type fixed_underlying_type;                                     // @wire 7
  int fixed_underlying_size;                                      // @wire 8
} Enum;

// Serialized as an inline sub-message of TypeRecord (see type_serialize.c);
// field numbers below are local to that sub-message.
typedef struct {
  union {
    struct {
      struct ASTNode* size;   // VLA size expression.       // @wire 7
      void* codegen_info;     // @wire - (codegen only)
    } vla;
    int fixed;             // Fixed array size.             // @wire 6
  } size;
  bool is_flexible:1;           // Flexible array (in struct).   // @wire 1
  bool is_static:1;             // Actual/formal must match.     // @wire 2
  bool is_vla:1;                // Variable length array.        // @wire 3
  bool is_placeholder_vla:1;    // [*] in function prototype.    // @wire 4
  int template_parameter_index;  // >=0 if bound is non-type param. // @wire 5
} ArrayInfo;


// Type record.  This represents one part of a type.
// Each of these structs is chained into a full type by the 'next'
// pointer.  We keep a count of the number of things pointing to
// each record so we can delete them when the count goes to zero.
// Module-serialization field numbers (see type_serialize.c).  refs is
// recomputed on load.  The info union is discriminated by declarator.
typedef struct TypeRecord {
  int id;     // Unique id for debugging.                         // @wire 1
  int refs;  // Reference count.                                  // @wire - (recomputed)
  Type type;                                                      // @wire 2
  Qualifiers qualifiers;                                          // @wire 3
  Declarator declarator;                                          // @wire 4
  int size;                                                       // @wire 5
  int template_parameter_index;  // >=0 for placeholder types.     // @wire 6
  String* dependent_member_name;  // For T::type-like types.       // @wire 7
  Symbol* template_origin;       // Primary template for template-ids. // @wire 8
  Vector* template_arguments;    // TemplateArgument* (owned).      // @wire 9
  struct TypeRecord* next;                                        // @wire 10
  union {                        // Discriminated by declarator/type:
    ArrayInfo array;             // @wire 11 (kDeclArray)
    FunctionInfo function;       // @wire 12 (kDeclFunction)
    Struct* struct_info;         // @wire 13 (kTypeStruct/kTypeUnion)
    Enum* enum_info;             // @wire 14 (kTypeEnum)
  } info;
} TypeRecord;

// Forward declaration of Syntax to avoid recursive loop in include
// files.
struct Syntax;

// State for parsing types.
typedef struct {
  Lex* lex;               // Lexical Analyzer.
  struct Syntax* syntax;  // Syntax Analyzer.
  Symbol* symbol;         // Symbol generated by declaration.
  Vector stack;           // Parsing stack.
  TypeRecord* base_type;  // Base type record.
  Storage storage;        // Storage for symbol.
  bool found_void;        // Flag: we've found 'void'.
  int dimension_count;    // Dimensions in array.
  bool is_inline;
  bool is_constexpr;
  bool is_consteval;
  bool is_constinit;
  bool declarator_is_parameter_pack;
  enum ParserContext context;
  Struct* cxx_member_owner;
  Struct* template_substitution_source;
  Struct* template_substitution_target;
  StructMember* cxx_member_definition;
  Vector* declarator_template_arguments;
  // Set when substituting template arguments into a type produces a hard
  // substitution failure in the immediate context (e.g. a dependent member
  // typedef like `enable_if<false, T>::type` that does not exist).  Callers
  // performing SFINAE-sensitive instantiation clear this before substitution
  // and, if it becomes set, discard the (ill-formed) instantiation instead of
  // emitting a diagnostic.
  bool template_substitution_failed;
} TypeParser;

// Struct to hold information from a partial type specifier.
typedef struct  {
  Type type;
  Qualifiers quals;
  TypeRecord* type_record;
  bool error;
} PartialTypeSpecifier;

TypeRecord* NewTypeRecord(Type type, Qualifiers quals);
TypeRecord* NewTypeRecordWithSize(Type type, Qualifiers quals);
void TypeRecordPrint(TypeRecord* record, FILE* fp);
void TypeRecordPrintDetails(TypeRecord* record, bool with_function_body, FILE* fp);
void TypeRecordDelete(TypeRecord* record);
// Free every TypeRecord struct allocated from the type arena.  Call once, at
// CompilerDestruct, after all type-referencing structures are torn down.
void TypeRecordArenaRelease(void);
// Free every Struct info (and its members) in one pass.  Call once, at
// CompilerDestruct, after the AST/symbols/tags are gone but before
// TypeRecordArenaRelease.  Structs are freed here rather than by refcount
// because they can form reference cycles.
void StructRegistryRelease(void);
TypeRecord* TypeRecordCalculateSize(TypeRecord* record);
void TypeRecordChain(TypeRecord* from, TypeRecord* to);
void TypeRecordIncRef(TypeRecord* record);
void TypeRecordDecRef(TypeRecord* record);
TypeRecord* TypeRecordCopy(TypeRecord* record);
int TypeRecordAlignment(TypeRecord* record);
bool TypeContainsTemplateParameter(TypeRecord* type);

// True if `symbol` is declared directly in namespace std.  Used to gate builtin
// recognition of standard-library types (e.g. std::source_location) so that a
// user type of the same name in another namespace is not mistaken for it.
bool SymbolIsInStdNamespace(Symbol* symbol);
void TemplateParameterDelete(TemplateParameter* param);
void TemplateArgumentDelete(TemplateArgument* arg);
TemplateArgument* NewTypeTemplateArgument(TypeRecord* type);
Vector* TemplateArgumentVectorCopy(Vector* args);
struct ASTNode* TypeSubstituteTemplateExpression(struct Syntax* syntax,
                                                struct ASTNode* expr,
                                                Vector* args,
                                                SourceLocation location);
TypeRecord* TypeSubstituteTemplateType(struct Syntax* syntax,
                                       TypeRecord* type,
                                       Vector* args);
Vector* TypeSubstituteTemplateArgumentVector(struct Syntax* syntax,
                                             Vector* template_args,
                                             Vector* args);

TypeRecord* NewPointerTypeRecord(Qualifiers quals);
TypeRecord* NewReferenceTypeRecord(Qualifiers quals, bool rvalue);
TypeRecord* NewArrayTypeRecord(Qualifiers quals, bool is_static);
TypeRecord* NewBasicArrayTypeRecord(Qualifiers quals, int size, bool is_flexible);

TypeRecord* NewFunctionTypeRecord(void);
TypeRecord* NewPointerTo(Qualifiers quals, TypeRecord* type);
Symbol* NewCXXThisSymbol(Struct* owner, bool is_const_member,
                         SourceLocation location);
void TypeRecordAddCXXThisParameter(TypeRecord* func, Struct* owner,
                                   SourceLocation location);

StructMember* NewStructMember(Symbol* symbol);
void StructAddSyntheticMember(Struct* str, StructMember* member);

// A generic lambda's call operator written inside another template numbers its
// invented `auto` parameters after the enclosing template's parameters.  When
// the closure captures nothing template-dependent it is never rebuilt per
// instantiation, so its operator keeps that enclosing-relative numbering and
// cannot be deduced or instantiated on its own.  Rebase such an operator to a
// standalone 0-based template in place (no-op for dependent-capture closures or
// operators that are already 0-based).
void TypeRebaseNonDependentLambdaCallOperator(Struct* closure, Symbol* op);
Struct* NewStruct(bool is_union);
void StructDelete(Struct* s);
void StructMemberDelete(StructMember* member);

// Records a C++ 'friend class X;' relationship: members of friend_class may
// access the private and protected members of str.  Duplicates are ignored.
void StructAddFriendClass(Struct* str, Struct* friend_class);
// Records a C++ 'friend <function>;' relationship: the named function may
// access the private and protected members of str.  Duplicates are ignored.
void StructAddFriendFunction(Struct* str, Symbol* friend_function);
bool StructMemberIsBitField(StructMember* member);

Symbol* NewEnumConstant(const char* name, int value);
Symbol* NewScopedEnumConstant(const char* name, int value, TypeRecord* enum_type);
Enum* NewEnum(void);
void EnumDelete(Enum* e);
Symbol* EnumFindConstant(Enum* e, String* name);

// What is the size of a given type in bytes?
int SizeofType(Type type);
int SizeofPointer(void);

StructMember* FindStructMember(Struct* str, String* name);
StructMember* FindStructMemberByName(Struct* str, const char* name);
StructMember* FindStructMemberWithAccess(Struct* str, String* name,
                                         CXXAccess* access,
                                         Struct** owner);
StructMember* FindStructMemberWithAccessByName(Struct* str, const char* name,
                                               CXXAccess* access,
                                               Struct** owner);
StructMember* FindStructMemberWithAccessAndOffset(Struct* str, String* name,
                                                  CXXAccess* access,
                                                  Struct** owner,
                                                  int* byte_offset);
StructMember* FindStructMemberWithAccessAndOffsetByName(
    Struct* str, const char* name, CXXAccess* access, Struct** owner,
    int* byte_offset);
StructMember* FindStructMemberOverload(StructMember* first, TypeRecord* type);
bool StructHasVirtualBases(Struct* str);
Symbol* StructFindVBTableSymbol(Struct* complete, Struct* source,
                                int source_offset);
Symbol* StructFindVTableSymbol(Struct* complete, Struct* source,
                               int source_offset);

void TypeRecordToString(TypeRecord* type, String* result);
void TypeRecordFunctionPrettyName(TypeRecord* func, String* result);
void SymbolFunctionPrettyName(Symbol* symbol, String* result);
void SymbolFunctionDiagnosticSuffix(Symbol* symbol, String* result);
void SymbolFunctionDiagnosticName(Symbol* symbol, String* result);

//
// TypeParser
//
// This provides functions for the syntax analysis of types and declarators.
//
void TypeParserInit(TypeParser* parser, Lex* lex, struct Syntax* syntax,
                    Storage storage, enum ParserContext context);
void TypeParserReset(TypeParser* parser);
// Release the parser's working storage (the declarator stack).  Does not free
// the TypeRecords the stack referenced; those are owned by the parsed type or
// already consumed.
void TypeParserDestruct(TypeParser* parser);

PartialTypeSpecifier TypeParserParseAndCombineTypes(TypeParser* parser,
                                                   PartialTypeSpecifier* prev);
TypeRecord* TypeParserBuildTypeRecord(TypeParser* parser, PartialTypeSpecifier* type);

TypeRecord* TypeParserParseType(TypeParser* parser, bool needed);
Symbol* TypeParserParseDeclarator(TypeParser* parser, TypeRecord* base_type);
bool TypeParserSkipAttributes(TypeParser* parser);
void TypeParserParseBase(TypeParser* parser);
void TypeParserParsePointer(TypeParser* parser);
void TypeParserParseFuncOrArray(TypeParser* parser);
Symbol* TypeInstantiateFunctionTemplate(struct Syntax* syntax, Symbol* templ,
                                        Vector* args);
void TypeEnsureTemplateMemberFunctionDefinition(struct Syntax* syntax,
                                                Symbol* symbol);
Symbol* TypeDeduceFunctionTemplateFromCall(struct Syntax* syntax, Symbol* templ,
                                           Vector* actuals);
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
    struct Syntax* syntax, Symbol* templ, Vector* explicit_args,
    Vector* actuals);
Symbol* TypeDeduceFunctionTemplateFromCallWithOffset(struct Syntax* syntax,
                                                     Symbol* templ,
                                                     Vector* actuals,
                                                     size_t first_formal_arg);
Symbol* TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    struct Syntax* syntax, Symbol* templ, Vector* explicit_args,
    Vector* actuals, size_t first_formal_arg);
bool TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
    Symbol* templ, Vector* explicit_args, Vector* actuals,
    size_t first_formal_arg);
bool TypeTemplateArgumentVectorEqual(Vector* left, Vector* right);
Symbol* TypeCreateFunctionTemplateCandidate(struct Syntax* syntax,
                                            Symbol* templ,
                                            Vector* explicit_args,
                                            Vector* actuals,
                                            size_t first_formal_arg);
Vector* TypeDeduceFunctionTemplateArgumentsFromCall(Symbol* templ,
                                                    Vector* actuals,
                                                    size_t first_formal_arg);
TypeRecord* TypeInstantiateClassTemplate(struct Syntax* syntax, Symbol* templ,
                                         Vector* args);
// If `type` (or a pointed-to/referenced type in its spine) is a class-template
// primary carrying concrete template arguments, replace that primary with the
// corresponding specialization.  Used when a type like `variant<int,long>` is
// still represented as the primary `variant` plus args (common inside function
// templates) and member lookup must see the instantiated members.
TypeRecord* TypeMaterializeClassTemplateSpecialization(struct Syntax* syntax,
                                                       TypeRecord* type);
// Instantiate a variable template's initializer with concrete template
// arguments and constant-fold it to an integer.  Returns true on success.
bool TypeInstantiateVariableTemplateConstant(struct Syntax* syntax,
                                             Symbol* var_template, Vector* args,
                                             int64_t* out);
// Instantiate the type of a variable template (e.g. `in_place_index<1>` ->
// `in_place_index_t<1>`) with concrete template arguments.  Used for variable
// templates whose value is a class-type tag object.  Returns NULL on failure.
TypeRecord* TypeInstantiateVariableTemplateType(struct Syntax* syntax,
                                                Symbol* var_template,
                                                Vector* args);
void TypeAddCXXDeductionGuide(Symbol* class_template, Symbol* guide);
void TypeEnsureCXXDeductionGuides(Symbol* class_template);
TypeRecord* TypeDeduceClassTemplateFromGuide(struct Syntax* syntax,
                                             Symbol* class_template,
                                             Vector* actuals,
                                             bool allow_explicit);
TypeRecord* TypeDeduceClassTemplateFromPlaceholder(struct Syntax* syntax,
                                                   TypeRecord* placeholder,
                                                   Vector* actuals,
                                                   bool allow_explicit,
                                                   bool* alias_rejected);
TypeRecord* TypeClassTemplatePlaceholderFromSymbol(Symbol* symbol);
bool TypeIsClassTemplatePlaceholder(TypeRecord* type);
Symbol* TypeClassTemplatePlaceholderOrigin(TypeRecord* type);
bool TypeClassTemplatePlaceholderAcceptsDeduced(TypeRecord* placeholder,
                                                TypeRecord* deduced);
bool TypeIsCXXInitializerList(TypeRecord* type);
TypeRecord* TypeCXXInitializerListElement(TypeRecord* type);
TypeRecord* TypeInstantiateCXXInitializerList(struct Syntax* syntax,
                                              TypeRecord* element_type);
TypeRecord* TypeFindCXXComparisonCategory(const char* category_name);

Symbol* TypeParserParseStruct(TypeParser* parser, bool is_union, bool is_class);
Symbol* TypeParserParseEnum(TypeParser* parser);
Symbol* TypeParserParseCXXSpecialMemberDeclarator(TypeParser* parser);

TypeRecord* NewSizeTypeRecord(void);

//
// Type inference.  The inline defintiions for these is in type.c.
//

inline bool TypeIsPointer(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return type->declarator == kDeclPointer;
}

bool TypeIsReference(TypeRecord* type);

inline bool TypeIsPrimitive(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return type->declarator == kDeclPrimitive;
}
inline bool TypeIsPointerOrArray(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return type->declarator == kDeclPointer ||
         type->declarator == kDeclReference ||
         type->declarator == kDeclRValueReference ||
         type->declarator == kDeclArray;
}

inline bool TypeIsFunction(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return type->declarator == kDeclFunction;
}

inline bool TypeIsFunctionDefinition(TypeRecord* type) {
  if (!TypeIsFunction(type)) {
    return false;
  }
  return type->info.function.definition;
}

inline bool TypeIsFunctionPointer(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (TypeIsPointer(type)) {
    TypeRecord* subtype = type->next;
    return subtype != NULL && TypeIsFunction(subtype);
  }
  return TypeIsFunction(type);
}

inline bool TypeIsVoid(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeVoid) != 0;
}

inline bool TypeIsAuto(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeAuto) != 0;
}

inline bool TypeIsNullPointer(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeNullPointer) != 0;
}

inline bool TypeIsVoidFunction(TypeRecord* type) {
  return TypeIsFunction(type) && TypeIsVoid(type->next);
}

bool TypeIsUnsigned(TypeRecord* type);
bool TypeIsSigned(TypeRecord* type);


inline bool TypeIsConst(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return (type->qualifiers & kQualConst) != 0;
}

inline bool TypeIsVolatile(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return (type->qualifiers & kQualVolatile) != 0;
}

inline bool TypeIsArray(TypeRecord* type) {
  return type != NULL && type->declarator == kDeclArray;
}
inline bool TypeIsFixedArray(TypeRecord* type) {
  return type != NULL && type->declarator == kDeclArray &&
      !type->info.array.is_vla;
}

// A VLA passed to a function is converted to a pointer but its array info
// remains intact.  A pointer will have all zeros in its array info.
inline bool TypeIsVLA(TypeRecord* type) {
  return type != NULL &&
      (type->declarator == kDeclArray || type->declarator == kDeclPointer) &&
      type->info.array.is_vla;
}

inline bool TypeIsIntegral(TypeRecord* type) {
  return TypeIsPrimitive(type) &&
         (type->type & (kTypeInt | kTypeShort | kTypeChar | kTypeLong |
                        kTypeLongLong | kTypeBool | kTypeEnum | kTypeUnsigned |
                        kTypeSigned)) != 0;
}

inline bool TypeIsFloatingPoint(TypeRecord* type) {
  return TypeIsPrimitive(type) &&
         (type->type & (kTypeFloat | kTypeDouble | kTypeLongDouble)) != 0;
}

inline bool TypeIsPointerToSameType(TypeRecord* ptr1, TypeRecord* ptr2) {
  return TypeIsPointer(ptr1) &&
         ptr1->declarator == ptr2->declarator &&
         ptr1->next->type == ptr2->next->type;
}

inline bool TypeIsStructOrUnion(TypeRecord* type) {
  return TypeIsPrimitive(type) &&
         (type->type & (kTypeStruct | kTypeUnion)) != 0;
}

inline bool TypeIsScalar(TypeRecord* type) { return !TypeIsStructOrUnion(type); }


inline bool TypeIsInt(TypeRecord* type) {
  if (!TypeIsPrimitive(type)) {
    return false;
  }
  if ((type->type & kTypeEnum) != 0) {
    if ((type->type & kTypeInt) != 0) {
      return true;
    }
    return false;
  }
  return (type->type & kTypeInt) != 0 &&
    (type->type & (kTypeShort | kTypeLong | kTypeLongLong)) == 0;
}

inline bool TypeIsChar(TypeRecord* type) {
  if (!TypeIsPrimitive(type)) {
    return false;
  }
  if ((type->type & kTypeEnum) != 0) {
    if ((type->type & kTypeChar) != 0) {
      return true;
    }
    return false;
  }
  return (type->type & kTypeChar) != 0;
}

inline bool TypeIsShort(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeShort) != 0;
}
inline bool TypeIsLong(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeLong) != 0;
}
inline bool TypeIsLongLong(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeLongLong) != 0;
}

inline bool TypeIsUnsignedInt(TypeRecord* type) {
  return TypeIsUnsigned(type) && TypeIsInt(type);
}

inline bool TypeIsUnsignedChar(TypeRecord* type) {
  return TypeIsUnsigned(type) && TypeIsChar(type);
}
inline bool TypeIsUnsignedShort(TypeRecord* type) {
  return TypeIsUnsigned(type) && TypeIsShort(type);
}
inline bool TypeIsUnsignedLong(TypeRecord* type) {
  return TypeIsUnsigned(type) && TypeIsLong(type);
}
inline bool TypeIsUnsignedLongLong(TypeRecord* type) {
  return TypeIsUnsigned(type) && TypeIsLongLong(type);
}

inline bool TypeIsFloat(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeFloat) != 0;
}
inline bool TypeIsDouble(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeDouble) != 0;
}
inline bool TypeIsLongDouble(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeLongDouble) != 0;
}
inline bool TypeIsBool(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeBool) != 0;
}

inline bool TypeIsEnum(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeEnum) != 0;
}

bool TypeIsScopedEnum(TypeRecord* type);

inline bool TypeIsVoidPointer(TypeRecord* type) {
  return TypeIsPointer(type) && type->next != NULL && TypeIsVoid(type->next);
}

inline bool TypeIsStructOrUnionPointer(TypeRecord* type) {
  return TypeIsPointerOrArray(type) && TypeIsStructOrUnion(type->next);
}


inline bool TypeIsIntConstant(TypeRecord* type) {
  return TypeIsIntegral(type) && ((type->qualifiers & kQualConst) != 0);
}

inline bool TypeIsFloatingPointConstant(TypeRecord* type) {
  return TypeIsFloatingPoint(type) && ((type->qualifiers & kQualConst) != 0);
}

inline bool TypeIsFunctionReturningStructOrUnion(TypeRecord* type) {
  if (TypeIsFunction(type)) {
    return TypeIsStructOrUnion(type->next);
  }
  if (TypeIsPointer(type) && TypeIsFunction(type->next)) {
    return TypeIsStructOrUnion(type->next->next);
  }
  return false;
}

inline bool TypeIsUnknown(TypeRecord* type) {
  if (type == NULL) {
    return true;
  }
  return (type->type & kTypeUnknown) != 0;
}

bool TypeEqual(TypeRecord* t1, TypeRecord* t2);
bool TypeAssignmentCompatible(TypeRecord* from, TypeRecord* to);
bool StructIsDerivedFrom(Struct* from, Struct* to, bool public_only);
bool TypeIsDerivedFrom(TypeRecord* from, TypeRecord* to);
bool TypeBaseOffset(TypeRecord* from, TypeRecord* to, bool public_only,
                    int* offset);
bool TypeBaseAdjustment(TypeRecord* from, TypeRecord* to, bool public_only,
                        CXXBaseAdjustment* adjustment);
bool TypeIsAbstractClass(TypeRecord* type);
bool TypeContainsAuto(TypeRecord* type);
bool TypeFunctionReturnContainsAuto(TypeRecord* type);
TypeRecord* TypeDeduceAuto(TypeRecord* pattern, TypeRecord* initializer_type);
bool TypeEqualIgnoringSign(TypeRecord* t1, TypeRecord* t2);
void TypeErrorDetails(SourceLocation location,
                      TypeRecord* t1, TypeRecord* t2);

#endif /* type_h */
