//
//  6502_target.h
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef _502_target_h
#define _502_target_h

#include "compiler.h"

#define k65c02Target 1       // Target is 65c02.

// Target specific options.
#define k6502OptionRegStart 0     // Address of start of zero page regs.

CompilerTarget* New6502Target(void);
CompilerTarget* New65c02Target(void);

typedef struct {
  CompilerTarget base;
  int regs_start;
} W65C02Target;

#endif /* _502_target_h */
