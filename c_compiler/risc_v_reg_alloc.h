//
//  risc_v_reg_alloc.h
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_reg_alloc_h
#define risc_v_reg_alloc_h

// P-Code register allocator.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "risc_v_machine.h"
#include "target_generator.h"

struct RVGenerator;
struct RVInstruction;

// We have 2 register types, integer (64 bit), float int point (64 bit).
typedef enum {
  kRVRegTypeInt,
  kRVRegTypeFloat,
} RVRegisterType;

typedef struct RVRegister {
  TargetRegister base;
  RVRegisterType type;
} RVRegister;


// The RISC-V R32F and R32D instructions share floating point registers.

#define RV_INT_RETURN_REG RV_INT_ARG_START  // Integer return value register.
#define RV_FLOAT_RETURN_REG \
  RV_FP_ARG_START  // Float/Double return value register.

typedef struct {
  struct RVGenerator* rv;
  RVRegister int_regs[RV_NUM_INT_REGS];
  RVRegister float_regs[RV_NUM_FLOAT_REGS];

  // We need to save some registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;
  
  int current_spilled_region_size;
  int max_spilled_region_size;
  BitSet preserved_instructions;    // Instructions needing preserved regs.
} RVRegisterAllocator;

void RVRegisterAllocatorInit(RVRegisterAllocator* alloc,
                             struct RVGenerator* rv);
RVRegisterAllocator* NewRVRegisterAllocator(struct RVGenerator* pcode);
const char* RVRegisterNameFromNum(int num, RVRegisterType type, char* buf,
                                  size_t len);

void RVRegisterAllocatorDestruct(RVRegisterAllocator* alloc);
void RVRegisterAllocatorDelete(RVRegisterAllocator* alloc);

void RVAllocateRegisters(RVRegisterAllocator* emitter);
const char* RVRegisterName(RVRegister* reg, char* buf, size_t len);

#endif /* risc_v_reg_alloc_h */
