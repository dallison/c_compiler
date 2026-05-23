//
//  pcode_emitter.h
//  c_compiler
//
//  Created by David Allison on 12/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef pcode_emitter_h
#define pcode_emitter_h

#include <stdio.h>
#include "p_code_codegen.h"
#include "p_code_reg_alloc.h"

typedef struct {
  PCodeGenerator* pcode;
  PCodeRegisterAllocator* regs;
} PCodeEmitter;

void PCodeEmitterInit(PCodeEmitter* emitter, PCodeGenerator* pcode);
PCodeEmitter* NewPCodeEmitter(PCodeGenerator* pcode);
void PCodeEmitterDestruct(PCodeEmitter* emitter);
void PCodeEmitterDelete(PCodeEmitter* emitter);

void PCodePrintFunction(PCodeEmitter* emitter, FILE* fp);

#endif /* pcode_emitter_h */
