//
//  arm_assembler.h
//  c_compiler_library
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseARMed.
//

#ifndef arm_assembler_h
#define arm_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"
#include "arm_emitter.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
  int32_t next_la_label;
} ARMAssembler;

bool ARMAssemblerInit(ARMAssembler* assembler, String* infile, String* outfile);
ARMAssembler* NewARMAssembler(String* infile, String* outfile);
void ARMAssemblerDestruct(ARMAssembler* assembler);
void ARMAssemblerDelete(ARMAssembler* assembler);
void AssembleARMInstruction(Assembler* assembler, String* word);

#endif /* arm_assembler_h */
