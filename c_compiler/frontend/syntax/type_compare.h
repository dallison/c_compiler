//
//  type_compare.h
//  c_compiler
//

#ifndef type_compare_h
#define type_compare_h

#include "type_defs.h"

struct ASTNode;

bool TypeContainsTemplateParameter(TypeRecord* type);
bool SymbolIsInStdNamespace(Symbol* symbol);
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
      !type->info.array.is_vla && !type->info.array.is_dependent_bound;
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
         (type->type & (kTypeInt | kTypeShort | kTypeChar | kTypeChar8 |
                        kTypeLong |
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

inline bool TypeIsChar8(TypeRecord* type) {
  return TypeIsPrimitive(type) && (type->type & kTypeChar8) != 0;
}

inline bool TypeIsCharFamily(TypeRecord* type) {
  return TypeIsChar(type) || TypeIsChar8(type);
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

inline bool TypeChar8IdentityDiffers(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL) {
    return false;
  }
  bool left_char8 = TypeIsChar8(left) && !TypeIsEnum(left);
  bool right_char8 = TypeIsChar8(right) && !TypeIsEnum(right);
  if (left_char8 || right_char8) {
    return left_char8 != right_char8;
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
TypeRecord* TypeDeduceAuto(TypeRecord* pattern, TypeRecord* initializer_type);
TypeRecord* TypeDeduceDecltypeAuto(struct ASTNode* expr);
bool TypeEqualIgnoringSign(TypeRecord* t1, TypeRecord* t2);
void TypeErrorDetails(SourceLocation location,
                      TypeRecord* t1, TypeRecord* t2);


#endif /* type_compare_h */
