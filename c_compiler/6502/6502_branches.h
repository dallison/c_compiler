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

void W65C02CalculateInstructionAddresses(W65C02Generator* g);
void W65C02ProcessBranches(W65C02Generator* g);


#endif /* _502_branches_h */
