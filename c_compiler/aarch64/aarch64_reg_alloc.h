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


// The RISC-V R32F and R32D instructions share floating point registers.

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
  
  int current_spilled_region_size;
  int max_spilled_region_size;
  BitSet preserved_instructions;    // Instructions needing preseAARCH64ed regs.
  BitSet short_lived_varregs;
  Map reassignable_spills;
} AARCH64RegisterAllocator;

void AARCH64RegisterAllocatorInit(AARCH64RegisterAllocator* alloc,
                             struct AARCH64Generator* AARCH64);
AARCH64RegisterAllocator* NewAARCH64RegisterAllocator(struct AARCH64Generator* pcode);
const char* AARCH64RegisterNameFromNum(int num, AARCH64RegisterType type, int size, char* buf,
                                  size_t len);

void AARCH64RegisterAllocatorDestruct(AARCH64RegisterAllocator* alloc);
void AARCH64RegisterAllocatorDelete(AARCH64RegisterAllocator* alloc);

void AARCH64AllocateRegisters(AARCH64RegisterAllocator* emitter);
const char* AARCH64RegisterName(AARCH64Register* reg, int size, char* buf, size_t len);

#endif /* arm_reg_alloc_h */
