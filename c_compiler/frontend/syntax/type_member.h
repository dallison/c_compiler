//
//  type_member.h
//  c_compiler
//

#ifndef type_member_h
#define type_member_h

#include "type_core.h"

void StructAddSyntheticMember(Struct* str, StructMember* member);
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

#endif /* type_member_h */
