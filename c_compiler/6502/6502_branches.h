//
//  6502_branches.h
//  c_compiler
//
//  Created by David Allison on 3/29/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef _502_branches_h
#define _502_branches_h

#include "6502_codegen.h"
#include "6502_machine.h"

void _6502CalculateInstructionAddresses(_6502Generator* g);
void _6502ProcessBranches(_6502Generator* g);


#endif /* _502_branches_h */
