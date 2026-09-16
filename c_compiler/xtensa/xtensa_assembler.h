//
//  xtensa_assembler.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef xtensa_assembler_h
#define xtensa_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
  int32_t next_la_label;
} XTENSAAssembler;

bool XTENSAAssemblerInit(XTENSAAssembler* assembler, String* infile,
                         String* outfile);
XTENSAAssembler* NewXTENSAAssembler(String* infile, String* outfile);
void XTENSAAssemblerDestruct(XTENSAAssembler* assembler);
void XTENSAAssemblerDelete(XTENSAAssembler* assembler);
void AssembleXTENSAInstruction(Assembler* assembler, String* word);
#endif /* xtensa_assembler_h */
