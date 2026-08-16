//
//  type_special_member.h
//  c_compiler
//

#ifndef type_special_member_h
#define type_special_member_h

#include "type_core.h"

bool CXXTypeIsTriviallyCopyable(TypeRecord* type);

bool CXXTypeSpecialMemberIsTrivial(TypeRecord* type, CXXSpecialMemberKind kind);

#endif /* type_special_member_h */
