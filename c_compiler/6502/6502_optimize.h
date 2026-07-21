//
//  6502_optimize.h
//  c_compiler_library
//
//  Created by David Allison on 5/11/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef W65C02_optimize_h
#define W65C02_optimize_h

#include "6502_codegen.h"
#include "6502_machine.h"

void W65C02Optimize(W65C02Generator* g);
void W65C02CombineIndirectCopies(W65C02Generator* g);

#endif /* W65C02_optimize_h */
