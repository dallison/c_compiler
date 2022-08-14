//
//  arm_assembler.c
//  c_compiler_library
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseARMed.
//

#include "arm_assembler.h"

void AssembleARMInstruction(Assembler* assembler, String* word) {
  
}

bool ARMAssemblerInit(ARMAssembler* assembler, String* infile, String* outfile) {
  return true;
}

ARMAssembler* NewARMAssembler(String* infile, String* outfile) {
  ARMAssembler* assembler = malloc(sizeof(ARMAssembler));
  ARMAssemblerInit(assembler, infile, outfile);
  return assembler;
}

// Destruct the assembler.
void ARMAssemblerDestruct(ARMAssembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void ARMAssemblerDelete(ARMAssembler* assembler) {
  ARMAssemblerDestruct(assembler);
  free(assembler);
}

