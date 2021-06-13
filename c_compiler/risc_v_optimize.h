//
//  risc_v_optimize.h
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//


#ifndef risc_v_optimize_h
#define risc_v_optimize_h

#include "target_basic_block.h"

struct RVGenerator;

void RVOptimize(struct RVGenerator* rv);

#endif /* risc_v_optimize_h */
