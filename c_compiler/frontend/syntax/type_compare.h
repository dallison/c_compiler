//
//  type_compare.h
//  c_compiler
//

#ifndef type_compare_h
#define type_compare_h

#include "type_defs.h"

struct ASTNode;
struct Syntax;

bool TypeContainsTemplateParameterSlow(TypeRecord* type);
static inline bool TypeContainsTemplateParameter(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  if (type->template_parameter_summary !=
      kTypeTemplateParameterSummaryUnknown) {
    return type->template_parameter_summary ==
           kTypeTemplateParameterSummaryPresent;
  }
  return TypeContainsTemplateParameterSlow(type);
}
void TypeCacheTemplateParameterSummary(TypeRecord* type);
bool SymbolIsInStdNamespace(Symbol* symbol);
bool TypeIsUninitializedFriendly(TypeRecord* type);
inline bool TypeIsPointer(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  return type->declarator == kDeclPointer;
}

bool TypeIsMemberPointer(TypeRecord* type);

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

inline bool TypeIsReflection(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeReflection) != 0;
}

inline bool TypeIsBitInt(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeBitInt) != 0;
}

// Reflection is a consteval-only type.  Compound declarators preserve that
// property so pointers, references, and arrays of reflection values cannot
// escape into runtime code.
inline bool TypeContainsReflection(TypeRecord* type) {
  for (TypeRecord* current = type; current != NULL; current = current->next) {
    if (TypeIsReflection(current)) {
      return true;
    }
  }
  return false;
}
bool TypeIsConstevalOnly(TypeRecord* type);

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

inline bool TypeIsAtomic(TypeRecord* type) {
  return type != NULL && (type->qualifiers & kQualAtomic) != 0;
}

inline bool TypeIsArray(TypeRecord* type) {
  return type != NULL && type->declarator == kDeclArray;
}
inline bool TypeIsFixedArray(TypeRecord* type) {
  return type != NULL && type->declarator == kDeclArray &&
      !type->info.array.is_vla && !type->info.array.is_dependent_bound;
}

// A VLA passed to a function is converted to a pointer but its array info
// remains intact.  A pointer will have all zeros in its array info.
inline bool TypeIsVLA(TypeRecord* type) {
  return type != NULL &&
      (type->declarator == kDeclArray || type->declarator == kDeclPointer) &&
      type->info.array.is_vla;
}

// Do two array declarators have the same bound?  A dependent or variable bound is
// an expression rather than a value, and the pointer to that expression shares
// storage with the fixed size, so neither can be read as a number.  Two variable
// bounds leave the arrays compatible: C requires only that the element types
// match and makes bounds that turn out to differ undefined at run time rather
// than a constraint violation.
bool TypeArrayBoundsEqual(ArrayInfo* a, ArrayInfo* b);

inline bool TypeIsIntegral(TypeRecord* type) {
  return TypeIsPrimitive(type) &&
         (type->type & (kTypeInt | kTypeShort | kTypeChar | kTypeChar8 |
                        kTypeChar16 | kTypeChar32 | kTypeLong |
                        kTypeLongLong | kTypeBool | kTypeEnum | kTypeUnsigned |
                        kTypeSigned | kTypeBitInt)) != 0;
}

inline bool TypeIsFloatingPoint(TypeRecord* type) {
  return TypeIsPrimitive(type) &&
         (type->type & (kTypeFloat | kTypeDouble | kTypeLongDouble |
                        kTypeFloat32 | kTypeFloat64)) != 0;
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

inline bool TypeIsChar8(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeChar8) != 0;
}

inline bool TypeIsChar16(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeChar16) != 0;
}

inline bool TypeIsChar32(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeChar32) != 0;
}

inline bool TypeIsCharFamily(TypeRecord* type) {
  return TypeIsChar(type) || TypeIsChar8(type) || TypeIsChar16(type) ||
         TypeIsChar32(type);
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
inline bool TypeIsFloat32(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeFloat32) != 0;
}
inline bool TypeIsDouble(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeDouble) != 0;
}
inline bool TypeIsFloat64(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeFloat64) != 0;
}
inline bool TypeIsLongDouble(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeLongDouble) != 0;
}
inline bool TypeUsesFloat32Representation(TypeRecord* type) {
  return TypeIsFloat(type) || TypeIsFloat32(type);
}
inline bool TypeUsesFloat64Representation(TypeRecord* type) {
  return TypeIsDouble(type) || TypeIsFloat64(type) || TypeIsLongDouble(type);
}
inline bool TypeIsBool(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeBool) != 0;
}

inline bool TypeIsEnum(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeEnum) != 0;
}

inline bool TypeChar8IdentityDiffers(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  Type unicode_mask = kTypeChar8 | kTypeChar16 | kTypeChar32;
  Type left_unicode = TypeIsPrimitive(left) && !TypeIsEnum(left)
                          ? left->type & unicode_mask
                          : kTypeImplicit;
  Type right_unicode = TypeIsPrimitive(right) && !TypeIsEnum(right)
                           ? right->type & unicode_mask
                           : kTypeImplicit;
  if (left_unicode != kTypeImplicit || right_unicode != kTypeImplicit) {
    return left_unicode != right_unicode;
  }
  if (left->next != NULL && right->next != NULL &&
      (TypeIsPointerOrArray(left) || TypeIsReference(left)) &&
      (TypeIsPointerOrArray(right) || TypeIsReference(right))) {
    return TypeChar8IdentityDiffers(left->next, right->next);
  }
  return false;
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
// Non-allocating variants for comparisons that intentionally ignore one
// top-level property while preserving strict comparison of the remaining type.
bool TypeEqualIgnoringTopLevelQualifierMask(TypeRecord* t1, TypeRecord* t2,
                                             Qualifiers ignored);
bool TypeEqualIgnoringFunctionNoexcept(TypeRecord* t1, TypeRecord* t2);
// Compare types for C++ virtual override matching after resolving alias and
// alias-template base spellings to their canonical class specializations.
bool TypeEqualForCXXOverride(struct Syntax* syntax, TypeRecord* t1,
                             TypeRecord* t2);
bool TypeAssignmentCompatible(TypeRecord* from, TypeRecord* to);
bool StructIsDerivedFrom(Struct* from, Struct* to, bool public_only);
int StructCountPublicDerivationPaths(Struct* from, Struct* to);
bool TypeIsDerivedFrom(TypeRecord* from, TypeRecord* to);
bool TypeBaseOffset(TypeRecord* from, TypeRecord* to, bool public_only,
                    int* offset);
bool TypeBaseAdjustment(TypeRecord* from, TypeRecord* to, bool public_only,
                        CXXBaseAdjustment* adjustment);
bool TypeIsAbstractClass(TypeRecord* type);
bool TypeContainsAuto(TypeRecord* type);
bool TypeFunctionReturnContainsAuto(TypeRecord* type);
TypeRecord* TypeDecayForByValueDeduction(TypeRecord* type);
TypeRecord* TypeDeduceAuto(TypeRecord* pattern, TypeRecord* initializer_type);
TypeRecord* TypeDeduceDecltypeAuto(struct ASTNode* expr);
bool TypeEqualIgnoringSign(TypeRecord* t1, TypeRecord* t2);
void TypeErrorDetails(SourceLocation location,
                      TypeRecord* t1, TypeRecord* t2);


#endif /* type_compare_h */
