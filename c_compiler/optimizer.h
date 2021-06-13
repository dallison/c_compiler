//
//  optimizer.h
//  c_compiler_library
//
//  Created by David Allison on 6/12/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef optimizer_h
#define optimizer_h

#include "codegen.h"

void StrengthReductionOptimization(Generator* gen);
void TailCallOptimization(Generator* gen);

#endif /* optimizer_h */
