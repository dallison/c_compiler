//
//  arm_emitter.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseARMed.
//

#ifndef arm_emitter_h
#define arm_emitter_h

#include <stdio.h>
#include "arm_codegen.h"
#include "arm_reg_alloc.h"
#include "map.h"

typedef struct {
  ARMGenerator* g;
  ARMRegisterAllocator* regs;
  int saved_reg_offset;
  int spill_region_size;
  int first_spill_offset;
  void* current_block;
} ARMEmitter;

void ARMEmitterInit(ARMEmitter* emitter, ARMGenerator* ARM);
ARMEmitter* NewARMEmitter(ARMGenerator* ARM);
void ARMEmitterDestruct(ARMEmitter* emitter);
void ARMEmitterDelete(ARMEmitter* emitter);

void ARMPrintFunction(ARMEmitter* emitter, FILE* fp);
void ARMPrintCXXAdjustorThunks(FILE* fp);

#endif /* arm_emitter_h */
