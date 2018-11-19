//
//  risc_v_emitter.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_emitter_h
#define risc_v_emitter_h

#include <stdio.h>
#include "risc_v_codegen.h"
#include "risc_v_reg_alloc.h"

typedef struct {
  RVGenerator* rv;
  RVRegisterAllocator* regs;
  int saved_reg_offset;
} RVEmitter;

void RVEmitterInit(RVEmitter* emitter, RVGenerator* rv);
RVEmitter* NewRVEmitter(RVGenerator* rv);
void RVEmitterDestruct(RVEmitter* emitter);
void RVEmitterDelete(RVEmitter* emitter);

void RVPrintFunction(RVEmitter* emitter, FILE* fp);

#endif /* risc_v_emitter_h */
