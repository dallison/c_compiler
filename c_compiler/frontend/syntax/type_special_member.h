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

// [basic.types.general] / [class.prop]: scalars, arrays, cv-qualified versions
// of those, and implicit-lifetime classes (an aggregate whose destructor is
// not user-provided, or a class with a trivial eligible constructor and a
// trivial non-deleted destructor).
bool CXXTypeIsImplicitLifetime(TypeRecord* type);

// Declares the implicit special members of `str` that its completion
// deferred.  Anything that looks for a constructor, destructor or assignment
// operator in the member list or lookup tables calls this first.
void CXXDeclarePendingSpecialMembersSlow(Struct* str);
static inline void CXXDeclarePendingSpecialMembers(Struct* str) {
  if (str != NULL && str->pending_special_members.kinds != 0) {
    CXXDeclarePendingSpecialMembersSlow(str);
  }
}
// True when `name` would find a constructor, the destructor or an assignment
// operator of `str` that is still pending.
bool CXXNameIsPendingSpecialMember(Struct* str, const char* name);
// True when the implicit member of `kind` is pending; it is then public, and
// a copy operation takes a const source.
bool CXXPendingSpecialMemberFlags(Struct* str, CXXSpecialMemberKind kind,
                                  bool* is_deleted, bool* is_noexcept);

bool CXXFunctionIsDefaultedPostfixOperator(Symbol* symbol);
void CXXFinalizeDefaultedPostfixFriendFunctions(TypeParser* parser,
                                                Struct* owner);

#endif /* type_special_member_h */
