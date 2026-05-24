//
//  arm_assembler.h
//  c_compiler_library
//

#ifndef arm_assembler_h
#define arm_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;
} ARMAssembler;

bool ARMAssemblerInit(ARMAssembler* assembler, String* infile, String* outfile);
ARMAssembler* NewARMAssembler(String* infile, String* outfile);
void ARMAssemblerDestruct(ARMAssembler* assembler);
void ARMAssemblerDelete(ARMAssembler* assembler);
void AssembleARMInstruction(Assembler* assembler, String* word);

#endif /* arm_assembler_h */
