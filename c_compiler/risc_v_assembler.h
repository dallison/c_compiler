//
//  risc_v_assembler.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_assembler_h
#define risc_v_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"
#include "risc_v_emitter.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
  int32_t next_la_label;
} RVAssembler;

bool RVAssemblerInit(RVAssembler* assembler, String* infile, String* outfile);
RVAssembler* NewRVAssembler(String* infile, String* outfile);
void RVAssemblerDestruct(RVAssembler* assembler);
void RVAssemblerDelete(RVAssembler* assembler);
void AssembleRVInstruction(Assembler* assembler, String* word);
#endif /* risc_v_assembler_h */
