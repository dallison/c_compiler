//
//  ssa.h
//  c_compiler
//
//  Created by David Allison on 12/14/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef ssa_h
#define ssa_h

// Static Single Assignment conversions.
#include "codegen.h"

void GeneratorConvertToSSA(Generator* gen);
void GeneratorRemoveSSA(Generator* gen);

#endif /* ssa_h */
