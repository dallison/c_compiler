//
//  risc_v32_assembler.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v32_assembler_h
#define risc_v32_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
  int32_t next_la_label;
} RV32Assembler;

bool RV32AssemblerInit(RV32Assembler* assembler, String* infile, String* outfile);
RV32Assembler* NewRV32Assembler(String* infile, String* outfile);
void RV32AssemblerDestruct(RV32Assembler* assembler);
void RV32AssemblerDelete(RV32Assembler* assembler);
void AssembleRV32Instruction(Assembler* assembler, String* word);
#endif /* risc_v32_assembler_h */
