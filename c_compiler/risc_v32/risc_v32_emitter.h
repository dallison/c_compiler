//
//  risc_v32_emitter.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v32_emitter_h
#define risc_v32_emitter_h

#include <stdio.h>
#include "risc_v32_codegen.h"
#include "risc_v32_reg_alloc.h"
#include "map.h"

typedef struct {
  RV32Generator* rv;
  RV32RegisterAllocator* regs;
  int saved_reg_offset;
  int spill_region_size;
  int first_spill_offset;
  void* current_block;
} RV32Emitter;

void RV32EmitterInit(RV32Emitter* emitter, RV32Generator* rv);
RV32Emitter* NewRV32Emitter(RV32Generator* rv);
void RV32EmitterDestruct(RV32Emitter* emitter);
void RV32EmitterDelete(RV32Emitter* emitter);

void RV32PrintFunction(RV32Emitter* emitter, FILE* fp);
void RV32PrintCXXAdjustorThunks(FILE* fp);

#endif /* risc_v32_emitter_h */
