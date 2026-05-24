//
//  x86_64_assembler.h
//  c_compiler
//
//  x86_64 assembler built on the generic ELF assembler infrastructure.
//

#ifndef x86_64_assembler_h
#define x86_64_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;
} X86_64Assembler;

bool X86_64AssemblerInit(X86_64Assembler* assembler, String* infile,
                           String* outfile);
X86_64Assembler* NewX86_64Assembler(String* infile, String* outfile);
void X86_64AssemblerDestruct(X86_64Assembler* assembler);
void X86_64AssemblerDelete(X86_64Assembler* assembler);
void AssembleX86_64Instruction(Assembler* assembler, String* word);

#endif /* x86_64_assembler_h */
