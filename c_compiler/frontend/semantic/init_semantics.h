//
//  init_semantics.h
//  c_compiler
//
//  Created by David Allison on 12/2/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef init_semantics_h
#define init_semantics_h

// Initialization semantics.  Initializations are complex enough to warrant
// their own file.

#include "semantics.h"

ASTNode* AnalyzeInitializer(TypeRecord* type, ASTNode* init,
                            bool constants_only);
bool InitializerIsLinkTimeConstant(ASTNode* init);
// The zero-initialized image of a static `type` object, which any dynamic
// initialization runs over ([basic.start.static]), when it is not all-zero
// bits (it holds a data member pointer); otherwise NULL.  Constructors and
// default member initializers do not contribute.  Virtual and non-public base
// subobjects are left all-zero.
ASTNode* CXXStaticZeroImage(TypeRecord* type, SourceLocation location);
// The null value of member pointer type `type` (not all-zero bits for data
// members).
ASTNode* NewNullMemberPointerConstant(TypeRecord* type,
                                      SourceLocation location);
// Whether `expr` names a C++ constexpr pointer or nullptr_t variable that has
// been evaluated: its value is a null pointer or an address constant.
bool CXXNamesConstexprPointerConstant(ASTNode* expr);

#endif /* init_semantics_h */
