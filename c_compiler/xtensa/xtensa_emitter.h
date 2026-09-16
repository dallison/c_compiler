//
//  xtensa_emitter.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef xtensa_emitter_h
#define xtensa_emitter_h

#include <stdio.h>
#include "map.h"
#include "xtensa_codegen.h"
#include "xtensa_reg_alloc.h"

typedef struct {
  XTENSAGenerator* rv;
  XTENSARegisterAllocator* regs;
  int saved_reg_offset;
  int spill_region_size;
  int first_spill_offset;
  int stack_pointer_adjustment;
  void* current_block;
} XTENSAEmitter;

void XTENSAEmitterInit(XTENSAEmitter* emitter, XTENSAGenerator* rv);
XTENSAEmitter* NewXTENSAEmitter(XTENSAGenerator* rv);
void XTENSAEmitterDestruct(XTENSAEmitter* emitter);
void XTENSAEmitterDelete(XTENSAEmitter* emitter);

void XTENSAPrintFunction(XTENSAEmitter* emitter, FILE* fp);
void XTENSAPrintCXXAdjustorThunks(FILE* fp);

#endif /* xtensa_emitter_h */
