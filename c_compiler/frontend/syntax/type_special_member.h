//
//  type_special_member.h
//  c_compiler
//

#ifndef type_special_member_h
#define type_special_member_h

#include "type_core.h"

// A non-union class with no non-static data members (other than zero-width
// bit-fields), no vptr or virtual bases, and only empty bases.  Its one byte
// of storage holds no value and may be shared with an enclosing object's data
// (an empty base or a [[no_unique_address]] member), so copies must not write
// it.
bool TypeIsEmptyClass(TypeRecord* type);
bool CXXTypeIsTriviallyCopyable(TypeRecord* type);
bool CXXTypeBitCastableInConstantExpression(TypeRecord* type);

bool CXXTypeSpecialMemberIsTrivial(TypeRecord* type, CXXSpecialMemberKind kind);
bool CXXTypeSpecialMemberIsDeleted(TypeRecord* type, CXXSpecialMemberKind kind);
bool CXXTypeIsImplicitLifetimeAggregate(TypeRecord* type);

bool CXXFunctionIsDefaultedPostfixOperator(Symbol* symbol);
void CXXFinalizeDefaultedPostfixFriendFunctions(TypeParser* parser,
                                                Struct* owner);

#endif /* type_special_member_h */
