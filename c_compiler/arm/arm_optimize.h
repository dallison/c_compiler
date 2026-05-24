//
//  arm_optimize.h
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//


#ifndef arm_optimize_h
#define arm_optimize_h

#include "target_basic_block.h"

struct ARMGenerator;

void ARMOptimize(struct ARMGenerator* rv);

#endif /* arm_optimize_h */
