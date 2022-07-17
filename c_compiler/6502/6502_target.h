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

CompilerTarget* New6502Target(void);
CompilerTarget* New65c02Target(void);

#endif /* _502_target_h */
