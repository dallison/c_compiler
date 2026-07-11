//
//  type_defs.h
//  c_compiler
//
//  Type data model and serialized layouts.
//

#ifndef type_defs_h
#define type_defs_h

#include <stdio.h>

#include "symbol.h"
#include "vector.h"
#include "map.h"

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
  struct Struct* lexical_parent;  // Enclosing class for nested C++ types. @wire -
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
  // For dependent qualified member type paths rooted at a template parameter.
  // `dependent_member_name` stores the `::`-joined components after the root;
  // this vector, when present, has one entry per component.  Each entry is
  // either NULL or a Vector<TemplateArgument*> for that component's template-id.
  Vector* dependent_member_template_arguments;
  // Unevaluated operand of a type-dependent decltype expression.  The AST is
  // arena-owned and re-cloned with concrete template arguments during
  // substitution.
  struct ASTNode* dependent_decltype_expr;                          // @wire 15
  struct TypeRecord* next;                                        // @wire 10
  union {                        // Discriminated by declarator/type:
    ArrayInfo array;             // @wire 11 (kDeclArray)
    FunctionInfo function;       // @wire 12 (kDeclFunction)
    Struct* struct_info;         // @wire 13 (kTypeStruct/kTypeUnion)
    Enum* enum_info;             // @wire 14 (kTypeEnum)
  } info;
} TypeRecord;

#endif /* type_defs_h */
