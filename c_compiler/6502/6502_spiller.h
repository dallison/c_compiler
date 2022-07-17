//
//  6502_spiller.h
//  c_compiler
//
//  Created by David Allison on 3/29/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef W65C02_spiller_h
#define W65C02_spiller_h

#include "6502_codegen.h"
#include "6502_machine.h"

void W65C02SpillExpressions(W65C02Generator* g);
void W65C02PoolVariables(W65C02Generator* g);

#endif /* W65C02_spiller_h */
