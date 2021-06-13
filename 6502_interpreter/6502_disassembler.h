//
//  6502_disassembler.h
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef _6502_disassembler_h
#define _6502_disassembler_h

#include <stdio.h>
#include <stdint.h>
#include "6502_machine.h"
#include "loader.h"

void* Disassemble6502Instruction(SymbolScope* current_symbol, uint16_t addr, void* p, FILE* fp);

#endif /* _6502_disassembler_h */
