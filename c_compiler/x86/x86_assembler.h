//
//  x86_assembler.h
//  c_compiler
//
//  x86 assembler built on the generic ELF assembler infrastructure.
//

#ifndef x86_assembler_h
#define x86_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"
#include "x86_profile.h"

typedef struct {
  Assembler base;
  const X86Profile* profile;
  Map instructions;
  int32_t bss;
} X86Assembler;

bool X86AssemblerInit(X86Assembler* assembler, String* infile,
                           String* outfile);
bool X86AssemblerInitWithProfile(X86Assembler* assembler, String* infile,
                                 String* outfile, const X86Profile* profile);
X86Assembler* NewX86Assembler(String* infile, String* outfile);
void X86AssemblerDestruct(X86Assembler* assembler);
void X86AssemblerDelete(X86Assembler* assembler);
void AssembleX86Instruction(Assembler* assembler, String* word);

#endif /* x86_assembler_h */
