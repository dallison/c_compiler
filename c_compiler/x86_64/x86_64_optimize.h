//
//  x86_64_optimize.h
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//


#ifndef x86_64_optimize_h
#define x86_64_optimize_h

#include "target_basic_block.h"

struct X86_64Generator;

void X86_64Optimize(struct X86_64Generator* rv);

#endif /* x86_64_optimize_h */
