//
//  p_code_disassembler.h
//  p_code_interpreter
//
//  Created by David Allison on 1/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef p_code_disassembler_h
#define p_code_disassembler_h

#include <stdio.h>
#include "p_code_machine.h"
#include "p_code_interpreter.h"

void* DisassemblePCodeInstruction(Interpreter* interpreter, void* p, FILE* fp);

#endif /* p_code_disassembler_h */
