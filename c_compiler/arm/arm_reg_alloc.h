//
//  arm_reg_alloc.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseARMed.
//

#ifndef arm_reg_alloc_h
#define arm_reg_alloc_h

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "map.h"
#include "arm_machine.h"
#include "target_generator.h"

struct ARMGenerator;
struct ARMInstruction;

// We have 2 register types, integer (64 bit), float int point (64 bit).
typedef enum {
  kARMRegTypeInt,
  kARMRegTypeFloat,
} ARMRegisterType;

typedef struct ARMRegister {
  TargetRegister base;
  ARMRegisterType type;
} ARMRegister;


// The RISC-V R32F and R32D instructions share floating point registers.

#define ARM_INT_RETURN_REG ARM_INT_ARG_START  // Integer return value register.
#define ARM_FLOAT_RETURN_REG \
  ARM_FP_ARG_START  // Float/Double return value register.

typedef struct {
  struct ARMGenerator* g;
  ARMRegister int_regs[ARM_NUM_INT_REGS];
  ARMRegister float_regs[ARM_NUM_FLOAT_REGS];

  // We need to save some registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;
  
  int current_spilled_region_size;
  int max_spilled_region_size;
  BitSet preserved_instructions;    // Instructions needing preseARMed regs.
  Map reassignable_spills;
  // Instructions whose own allocation is in progress.  Allocating one can spill
  // another value, and if that value is one of these instructions' operands the
  // read has to be repaired rather than retargeted: the reload pass for it has
  // already run.
#define ARM_MAX_ALLOCATION_DEPTH 32
  TargetInstruction* allocating[ARM_MAX_ALLOCATION_DEPTH];
  size_t allocating_depth;
} ARMRegisterAllocator;

void ARMRegisterAllocatorInit(ARMRegisterAllocator* alloc,
                             struct ARMGenerator* ARM);
ARMRegisterAllocator* NewARMRegisterAllocator(struct ARMGenerator* pcode);
const char* ARMRegisterNameFromNum(int num, ARMRegisterType type, int size, char* buf,
                                  size_t len);

void ARMRegisterAllocatorDestruct(ARMRegisterAllocator* alloc);
void ARMRegisterAllocatorDelete(ARMRegisterAllocator* alloc);

void ARMAllocateRegisters(ARMRegisterAllocator* emitter);
const char* ARMRegisterName(ARMRegister* reg, int size, char* buf, size_t len);

#endif /* arm_reg_alloc_h */
