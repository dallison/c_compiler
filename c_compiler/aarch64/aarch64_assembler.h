//
//  aarch64_assembler.h
//  c_compiler_library
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseAARCH64ed.
//

#ifndef aarch64_assembler_h
#define aarch64_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"
#include "aarch64_emitter.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
} AARCH64Assembler;

bool AARCH64AssemblerInit(AARCH64Assembler* assembler, String* infile, String* outfile);
bool AARCH64AssemblerInitGenerated(AARCH64Assembler* assembler, String* infile,
                                   String* outfile);
bool AARCH64AssemblerInitFromGeneratedString(AARCH64Assembler* assembler,
                                             const char* name, String* input,
                                             String* outfile);
AARCH64Assembler* NewAARCH64Assembler(String* infile, String* outfile);
void AARCH64AssemblerDestruct(AARCH64Assembler* assembler);
void AARCH64AssemblerDelete(AARCH64Assembler* assembler);
void AssembleAARCH64Instruction(Assembler* assembler, String* word);

#endif /* aarch64_assembler_h */
