//
//  6502_reg_alloc.h
//  c_compiler
//
//  Created by David Allison on 6/5/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef W65C02_reg_alloc_h
#define W65C02_reg_alloc_h

// P-Code register allocator.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "6502_machine.h"
#include "target_generator.h"

struct W65C02Generator;
struct W65C02Instruction;

// The 6502 really has only 3 accessible 8-bit machine registers.  Those
// are unsuitable for storing any variables or intemediate values so we
// have to use zero-page to hold "registers" of different sizes.  The 6502
// can also address memory directly so the address of a static variable
// is also a "register", as is the offset from the frame pointer or argument
// pointer "registers".

// We have 5 register types, 8, 16, 32, 64 bit int, 32 and 64 bit float.
typedef enum {
  k6502RegTypeB,
  k6502RegTypeI,
  k6502RegTypeL,
  k6502RegTypeX,
  k6502RegTypeF,
} W65C02RegisterType;


typedef struct W65C02Register {
  TargetRegister base;
  W65C02RegisterType type;
  bool locked;                      // Locked (do not free).
  bool temp;                        // Is temp (not preserved).
} W65C02Register;

#define W65C02_B_RETURN_REG 0     // 8-bit Integer return value register.
#define W65C02_I_RETURN_REG 0     // 16-bit Integer return value register.
#define W65C02_L_RETURN_REG 0     // 32-bit Integer return value register.
#define W65C02_X_RETURN_REG 0     // 64-bit Integer return value register.
#define W65C02_F_RETURN_REG 0     // Float return value register.

typedef struct {
  struct W65C02Generator* g;
  W65C02Register b_regs[W65C02_NUM_B_REGS];
  W65C02Register i_regs[W65C02_NUM_I_REGS];
  W65C02Register l_regs[W65C02_NUM_L_REGS];
  W65C02Register x_regs[W65C02_NUM_X_REGS];
  W65C02Register f_regs[W65C02_NUM_F_REGS];
  W65C02Register ap_reg;
  W65C02Register sp_reg;
  W65C02Register fp_reg;
  BitSet used_b_regs;
  BitSet used_i_regs;
  BitSet used_l_regs;
  BitSet used_x_regs;
  BitSet used_f_regs;
  int current_spilled_region_size;
  int max_spilled_region_size;
  Map spill_points;              // Map of inst id vs inst ptr for spill points.
  BitSet preserved_instructions;    // Instructions needing preserved regs.
} W65C02RegisterAllocator;

void W65C02RegisterAllocatorInit(W65C02RegisterAllocator* alloc,
                                struct W65C02Generator* g);
W65C02RegisterAllocator* New6502RegisterAllocator(struct W65C02Generator* g);

void W65C02RegisterAllocatorDestruct(W65C02RegisterAllocator* alloc);
void W65C02RegisterAllocatorDelete(W65C02RegisterAllocator* alloc);
uint32_t W65C02RegisterAllocatorBuildRegMask(W65C02RegisterAllocator* alloc);
const char* W65C02RegisterAllocatorPrintRegMask(uint32_t mask, char* buf);

void W65C02AllocateRegisters(W65C02RegisterAllocator* emitter);
const char* W65C02RegisterAsString(W65C02Register* reg, int byte, char* buf, size_t len);
void W65C02RegisterAllocatorAddSpillPoint(W65C02RegisterAllocator* alloc,
                                            int expr, TargetInstruction* spill_point);
void W65C02RegisterAllocatorRemoveSpillPoint(W65C02RegisterAllocator* alloc,
                                               int expr);

#endif /* W65C02_reg_alloc_h */
