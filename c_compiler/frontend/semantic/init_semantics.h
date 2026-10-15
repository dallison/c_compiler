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
// Whether `expr` names a C++ constexpr pointer or nullptr_t variable that has
// been evaluated: its value is a null pointer or an address constant.
bool CXXNamesConstexprPointerConstant(ASTNode* expr);

#endif /* init_semantics_h */
