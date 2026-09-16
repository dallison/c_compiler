//
//  x86_emitter.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef x86_emitter_h
#define x86_emitter_h

#include <stdio.h>
#include "x86_codegen.h"
#include "x86_reg_alloc.h"
#include "map.h"

typedef struct {
  X86Generator* rv;
  X86RegisterAllocator* regs;
  int saved_reg_offset;
  int spill_region_size;
  int first_spill_offset;
  void* current_block;
} X86Emitter;

void X86EmitterInit(X86Emitter* emitter, X86Generator* rv);
X86Emitter* NewX86Emitter(X86Generator* rv);
void X86EmitterDestruct(X86Emitter* emitter);
void X86EmitterDelete(X86Emitter* emitter);

void X86PrintFunction(X86Emitter* emitter, FILE* fp);
void X86PrintCXXAdjustorThunks(FILE* fp);

#endif /* x86_emitter_h */
