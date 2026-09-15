//
//  risc_v32_optimize.h
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//


#ifndef risc_v32_optimize_h
#define risc_v32_optimize_h

#include "target_basic_block.h"

struct RV32Generator;

void RV32Optimize(struct RV32Generator* rv);

#endif /* risc_v32_optimize_h */
