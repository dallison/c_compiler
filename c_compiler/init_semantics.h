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

ASTNode* AnalyzeInitializer(Syntax* syntax, TypeRecord* type, ASTNode* init);

#endif /* init_semantics_h */
