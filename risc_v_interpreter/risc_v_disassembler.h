//
//  risc_v_disassembler.h
//  risc_v_interpreter
//
//  Created by David Allison on 4/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_disassembler_h
#define risc_v_disassembler_h

#include <stdio.h>
#include "risc_v_machine.h"
#include "risc_v_interpreter.h"

typedef enum {
  kRegTypeInt,
  kRegTypeFloat,
} RegisterType;

void DisassembleRiscVInstruction(Interpreter* interpreter, void* p, FILE* fp);
void DisassemblePrintRegister(FILE* fp, int reg, RegisterType type, const char* sep);

#endif /* risc_v_disassembler_h */
