//
//  aarch64_emitter.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseAARCH64ed.
//

#ifndef aarch64_emitter_h
#define aarch64_emitter_h

#include <stdio.h>
#include "aarch64_codegen.h"
#include "aarch64_reg_alloc.h"
#include "asm_module.h"
#include "map.h"

typedef struct {
  AARCH64Generator* g;
  AARCH64RegisterAllocator* regs;
  int saved_reg_offset;
  int spill_region_size;
  int first_spill_offset;
  void* current_block;
} AARCH64Emitter;

void AARCH64EmitterInit(AARCH64Emitter* emitter, AARCH64Generator* AARCH64);
AARCH64Emitter* NewAARCH64Emitter(AARCH64Generator* AARCH64);
void AARCH64EmitterDestruct(AARCH64Emitter* emitter);
void AARCH64EmitterDelete(AARCH64Emitter* emitter);

void AARCH64PrintFunction(AARCH64Emitter* emitter, FILE* fp);
void AARCH64EmitFunctionToModule(AARCH64Emitter* emitter, AsmModule* module);
void AARCH64EmitCXXAdjustorThunksToModule(AsmModule* module);
void AARCH64EmitFunction(AARCH64Emitter* emitter, FILE* text_out);
void AARCH64PrintCXXAdjustorThunks(FILE* fp);

#endif /* aarch64_emitter_h */
