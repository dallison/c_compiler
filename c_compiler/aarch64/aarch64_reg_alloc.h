//
//  arm_reg_alloc.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseAARCH64ed.
//

#ifndef arm_reg_alloc_h
#define arm_reg_alloc_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "map.h"
#include "aarch64_machine.h"
#include "target_generator.h"
#include "target_ra_ir.h"

struct AARCH64Generator;
struct AARCH64Instruction;

// We have 2 register types, integer (64 bit), float int point (64 bit).
typedef enum {
  kAARCH64RegTypeInt,
  kAARCH64RegTypeFloat,
} AARCH64RegisterType;

typedef struct AARCH64Register {
  TargetRegister base;
  AARCH64RegisterType type;
} AARCH64Register;

#define AARCH64_INT_RETURN_REG AARCH64_INT_ARG_START  // Integer return value register.
#define AARCH64_FLOAT_RETURN_REG \
  AARCH64_FP_ARG_START  // Float/Double return value register.

typedef struct {
  struct AARCH64Generator* g;
  AARCH64Register int_regs[AARCH64_NUM_INT_REGS];
  AARCH64Register float_regs[AARCH64_NUM_FLOAT_REGS];

  // We need to save some registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;

  int max_spilled_region_size;

  // The saved registers shrink wrapping replaces on the fast path, one bit
  // each.
  uint32_t shrink_wrapped_regs;
} AARCH64RegisterAllocator;

void AARCH64RegisterAllocatorInit(AARCH64RegisterAllocator* alloc,
                             struct AARCH64Generator* AARCH64);
AARCH64RegisterAllocator* NewAARCH64RegisterAllocator(struct AARCH64Generator* pcode);
const char* AARCH64RegisterNameFromNum(int num, AARCH64RegisterType type, int size, char* buf,
                                  size_t len);

void AARCH64RegisterAllocatorDestruct(AARCH64RegisterAllocator* alloc);
void AARCH64RegisterAllocatorDelete(AARCH64RegisterAllocator* alloc);

void AARCH64AllocateRegisters(AARCH64RegisterAllocator* emitter);
void AARCH64ShrinkWrap(AARCH64RegisterAllocator* allocator);
const char* AARCH64RegisterName(AARCH64Register* reg, int size, char* buf, size_t len);

// True if the allocator gives |inst|'s result a floating point register.
bool AARCH64HasFloatResult(TargetInstruction* inst);

// How the shared allocator code reads AArch64 instructions.
const TargetRAInfo* AARCH64RAInfo(void);

// Allocates the current function with the shared coloring allocator.
void AARCH64AllocateRegistersByColoring(AARCH64RegisterAllocator* allocator);

#endif /* arm_reg_alloc_h */
