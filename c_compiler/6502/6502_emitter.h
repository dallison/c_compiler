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
  _6502Generator* g;
  _6502RegisterAllocator* regs;
} _6502Emitter;

void _6502EmitterInit(_6502Emitter* emitter, _6502Generator* g);
_6502Emitter* New6502Emitter(_6502Generator* g);
void _6502EmitterDestruct(_6502Emitter* emitter);
void _6502EmitterDelete(_6502Emitter* emitter);

void _6502PrintFunction(_6502Emitter* emitter, FILE* fp);

#endif /* _502_emitter_h */
