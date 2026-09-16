//
//  xtensa_optimize.h
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef xtensa_optimize_h
#define xtensa_optimize_h

#include "target_basic_block.h"

struct XTENSAGenerator;

void XTENSAOptimize(struct XTENSAGenerator* rv);

#endif /* xtensa_optimize_h */
