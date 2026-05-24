//
//  x86_64_emitter.h
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef x86_64_emitter_h
#define x86_64_emitter_h

#include <stdio.h>
#include "x86_64_codegen.h"
#include "x86_64_reg_alloc.h"
#include "map.h"

typedef struct {
  X86_64Generator* rv;
  X86_64RegisterAllocator* regs;
  int saved_reg_offset;
  int spill_region_size;
  int first_spill_offset;
  void* current_block;
} X86_64Emitter;

void X86_64EmitterInit(X86_64Emitter* emitter, X86_64Generator* rv);
X86_64Emitter* NewX86_64Emitter(X86_64Generator* rv);
void X86_64EmitterDestruct(X86_64Emitter* emitter);
void X86_64EmitterDelete(X86_64Emitter* emitter);

void X86_64PrintFunction(X86_64Emitter* emitter, FILE* fp);

#endif /* x86_64_emitter_h */
