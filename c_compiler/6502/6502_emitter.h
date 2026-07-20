//
//  6502_emitter.h
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef _502_emitter_h
#define _502_emitter_h

#include <stdio.h>
#include "6502_codegen.h"
#include "6502_reg_alloc.h"

typedef struct {
  W65C02Generator* g;
  W65C02RegisterAllocator* regs;
} W65C02Emitter;

void W65C02EmitterInit(W65C02Emitter* emitter, W65C02Generator* g);
W65C02Emitter* New6502Emitter(W65C02Generator* g);
void W65C02EmitterDestruct(W65C02Emitter* emitter);
void W65C02EmitterDelete(W65C02Emitter* emitter);

void W65C02PrintFunction(W65C02Emitter* emitter, FILE* fp);
void W65C02PrintCXXAdjustorThunks(FILE* fp);

#endif /* _502_emitter_h */
