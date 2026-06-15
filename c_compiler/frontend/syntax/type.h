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
} Type;

// The last bit position in the type specifier that corresponds to a
// unique type (not including signed and unsigned).
//  This is used to test for a invalid combination of types.
#define TYPE_LAST_BIT 16

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

// Function info.
typedef struct {
  Symbol* symbol;       // Symbol for function (or NULL).
  Vector prototype;     // Formal arguments (owned Symbol*).
  bool varargs;         // True if varargs function.
  struct ASTNode* body; // Body AST.
  bool unknown_args;    // Old-style or invented function.
  bool definition;      // Function is a definition.
  bool old_style;       // Old-style arguments.
  bool is_inline;       // This is an inline function.
  bool is_constructor;  // Called before main.
  bool is_destructor;   // Called after exit.
  bool is_const_member; // C++ member function has trailing const qualifier.
  bool is_virtual;      // C++ virtual member function.
  bool is_override;     // C++ override virt-specifier.
  bool is_final;        // C++ final virt-specifier.
  bool is_pure_virtual; // C++ pure virtual function (`= 0`).
  int virtual_index;    // Vtable slot, or -1 for non-virtual functions.
  Struct* cxx_member_owner;  // Owning class for C++ member functions.
} FunctionInfo;

typedef enum {
  kAccessPublic,
  kAccessProtected,
  kAccessPrivate,
} CXXAccess;

typedef struct CXXBaseSpecifier {
  struct TypeRecord* type;  // Base class type.
  CXXAccess access;
  int byte_offset;
} CXXBaseSpecifier;

// A struct or union member.  Behaves like a Symbol with extra information.
typedef struct StructMember {
  Symbol* symbol;   // Embedded Symbol.
  int byte_offset;  // Byte offset into struct.
  int bit_offset;   // Bit offset into word.
  int bit_size;     // Bitfield size in bits.
  size_t index;     // Index into members vector.
  bool is_anon;     // This is an anonymous member.
  bool is_static;   // C++ static data/function member.
  bool is_member_function;
  CXXAccess access;
  struct StructMember* overload_next;  // Next C++ member overload by name.
} StructMember;

// A struct or union type.
struct Struct {
  int refs;
  String* tag_name;  // Tag name (not owned by this, owned by Symbol)
  Vector bases;      // Vector of CXXBaseSpecifier* (owns entries).
  Vector members;    // Vector of StructMember* (owns StructMembers)
  Vector virtual_members;  // Vector of StructMember* (not owned), by slot.
  StructMember* vptr_member;  // Hidden C++ vptr field, if owned by this class.
  Symbol* vtable_symbol;      // Hidden C++ vtable static symbol.
  Map symbol_table;  // Map of String* vs StructMember* (not owned).
  int next_offset;   // Byte offset of next member.
  int size;          // Size of struct in bytes.
  int alignment;     // Alignment of struct (max alignment of its members).
  bool is_union;     // True if this is a union.
  bool is_class;     // True if this is a C++ class.
  bool packed;       // __attribute__((packed)): no inter-member padding.
  bool is_abstract;  // C++ class has at least one unimplemented pure virtual.
  int explicit_alignment;  // __attribute__((aligned(N))) minimum; 0 = none.
  int pack;          // #pragma pack(n) member alignment cap; 0 = no cap.
  int next_bit_pos;  // Next bit position for bit fields.
  int current_offset;
};

// Applies layout-affecting attributes (packed, aligned) from an Attribute
// vector to a struct.  Must be called before the struct is laid out (or
// followed by a re-layout for the trailing/typedef form).
void StructApplyLayoutAttributes(struct Struct* str, Vector* attrs);
// Propagates packed/aligned attributes from a (typedef) symbol onto its
// struct/union type and re-lays out the struct.  No-op if the symbol carries
// no layout attribute or its type is not a struct/union.
void TypeApplyStructAttributesFromSymbol(Symbol* sym);

// An enum type.
typedef struct {
  int refs;
  String* tag_name;  // Tag name (owned by Symbol).
  Vector constants;  // Vector of Symbol* (not owned).
  int next_value;    // Value to give to next constant.
  bool is_scoped;    // C++ scoped enum: enum class / enum struct.
  bool has_fixed_underlying;
  Type fixed_underlying_type;
  int fixed_underlying_size;
} Enum;

typedef struct {
  union {
    struct {
      struct ASTNode* size;   // Variable Length Array size.
      void* codegen_info;     // Information for code generator.
    } vla;
    int fixed;             // Fixed array size.
  } size;
  bool is_flexible:1;           // Is a flexible array (inside struct).
  bool is_static:1;             // In call, actual and formal must match.
  bool is_vla:1;                // This is a variable length array.
  bool is_placeholder_vla:1;    // [*] used in function prototype.
} ArrayInfo;


// Type record.  This represents one part of a type.
// Each of these structs is chained into a full type by the 'next'
// pointer.  We keep a count of the number of things pointing to
// each record so we can delete them when the count goes to zero.
typedef struct TypeRecord {
  int id;     // Unique id for debugging.
  int refs;  // Reference count.
  Type type;
  Qualifiers qualifiers;
  Declarator declarator;
  int size;
  struct TypeRecord* next;
  union {
    ArrayInfo array;
    FunctionInfo function;
    Struct* struct_info;
    Enum* enum_info;
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
  enum ParserContext context;
  Struct* cxx_member_owner;
  StructMember* cxx_member_definition;
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
Struct* NewStruct(bool is_union);
void StructDelete(Struct* s);
void StructMemberDelete(StructMember* member);
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
StructMember* FindStructMemberWithAccess(Struct* str, String* name,
                                         CXXAccess* access,
                                         Struct** owner);
StructMember* FindStructMemberOverload(StructMember* first, TypeRecord* type);

void TypeRecordToString(TypeRecord* type, String* result);

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

Symbol* TypeParserParseStruct(TypeParser* parser, bool is_union, bool is_class);
Symbol* TypeParserParseEnum(TypeParser* parser);
Symbol* TypeParserParseCXXSpecialMemberDeclarator(TypeParser* parser);

TypeRecord* NewSizeTypeRecord(void);

//
// Type inference.  The inline defintiions for these is in type.c.
//

inline bool TypeIsPointer(TypeRecord* type) {
  return type->declarator == kDeclPointer;
}

bool TypeIsReference(TypeRecord* type);

inline bool TypeIsPrimitive(TypeRecord* type) {
  return type->declarator == kDeclPrimitive;
}
inline bool TypeIsPointerOrArray(TypeRecord* type) {
  return type->declarator == kDeclPointer ||
         type->declarator == kDeclReference ||
         type->declarator == kDeclRValueReference ||
         type->declarator == kDeclArray;
}

inline bool TypeIsFunction(TypeRecord* type) {
  return type->declarator == kDeclFunction;
}

inline bool TypeIsFunctionDefinition(TypeRecord* type) {
  if (!TypeIsFunction(type)) {
    return false;
  }
  return type->info.function.definition;
}

inline bool TypeIsFunctionPointer(TypeRecord* type) {
  if (TypeIsPointer(type)) {
    TypeRecord* subtype = type->next;
    return TypeIsFunction(subtype);
  }
  return TypeIsFunction(type);
}

inline bool TypeIsVoid(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeVoid) != 0;
}

inline bool TypeIsAuto(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeAuto) != 0;
}

inline bool TypeIsVoidFunction(TypeRecord* type) {
  return TypeIsFunction(type) && TypeIsVoid(type->next);
}

bool TypeIsUnsigned(TypeRecord* type);
bool TypeIsSigned(TypeRecord* type);


inline bool TypeIsConst(TypeRecord* type) {
  return (type->qualifiers & kQualConst) != 0;
}

inline bool TypeIsVolatile(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return (type->qualifiers & kQualVolatile) != 0;
}

inline bool TypeIsArray(TypeRecord* type) { return type->declarator == kDeclArray; }
inline bool TypeIsFixedArray(TypeRecord* type) {
  return type->declarator == kDeclArray &&
      !type->info.array.is_vla;
}

// A VLA passed to a function is converted to a pointer but its array info
// remains intact.  A pointer will have all zeros in its array info.
inline bool TypeIsVLA(TypeRecord* type) {
  return (type->declarator == kDeclArray || type->declarator == kDeclPointer) &&
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
  return (type->type & kTypeUnknown) != 0;
}

bool TypeEqual(TypeRecord* t1, TypeRecord* t2);
bool TypeAssignmentCompatible(TypeRecord* from, TypeRecord* to);
bool StructIsDerivedFrom(Struct* from, Struct* to, bool public_only);
bool TypeIsDerivedFrom(TypeRecord* from, TypeRecord* to);
bool TypeIsAbstractClass(TypeRecord* type);
bool TypeContainsAuto(TypeRecord* type);
TypeRecord* TypeDeduceAuto(TypeRecord* pattern, TypeRecord* initializer_type);
bool TypeEqualIgnoringSign(TypeRecord* t1, TypeRecord* t2);
void TypeErrorDetails(SourceLocation location,
                      TypeRecord* t1, TypeRecord* t2);

#endif /* type_h */
