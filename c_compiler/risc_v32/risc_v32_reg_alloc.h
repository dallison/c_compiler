//
//  risc_v32_reg_alloc.h
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v32_reg_alloc_h
#define risc_v32_reg_alloc_h

// RISC-V32 register allocator.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "map.h"
#include "risc_v32_machine.h"
#include "target_generator.h"

struct RV32Generator;
struct RV32Instruction;

// We have 2 register types, integer (32 bit), floating point.
typedef enum {
  kRV32RegTypeInt,
  kRV32RegTypeFloat,
} RV32RegisterType;

typedef struct RV32Register {
  TargetRegister base;
  RV32RegisterType type;
} RV32Register;


// The RISC-V32 R32F and R32D instructions share floating point registers.

#define RV32_INT_RETURN_REG RV32_INT_ARG_START  // Integer return value register.
#define RV32_FLOAT_RETURN_REG \
  RV32_FP_ARG_START  // Float/Double return value register.
#define RV32_MAX_ALLOCATION_DEPTH 32

typedef struct {
  struct RV32Generator* rv;
  RV32Register int_regs[RV32_NUM_INT_REGS];
  RV32Register float_regs[RV32_NUM_FLOAT_REGS];

  // We need to save some registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;
  
  int current_spilled_region_size;
  int max_spilled_region_size;
  BitSet preserved_instructions;    // Instructions needing preserved regs.
  // Reassignable values (variable registers and merge temporaries) need their
  // spill slot refreshed after every target instruction that defines them.
  Map reassignable_spills;
  // Instructions whose own allocation is in progress.  Allocating one can spill
  // another value, and once the reload pass for an in-flight instruction has
  // run, a read of that value has to be repaired rather than retargeted:
  // pointing the operand at the spill slot then leaves it naming a register the
  // spill has given away.
  TargetInstruction* allocating[RV32_MAX_ALLOCATION_DEPTH];
  bool allocating_reloaded[RV32_MAX_ALLOCATION_DEPTH];
  size_t allocating_depth;
} RV32RegisterAllocator;

void RV32RegisterAllocatorInit(RV32RegisterAllocator* alloc,
                             struct RV32Generator* rv);
RV32RegisterAllocator* NewRV32RegisterAllocator(struct RV32Generator* pcode);
const char* RV32RegisterNameFromNum(int num, RV32RegisterType type, char* buf,
                                  size_t len);

void RV32RegisterAllocatorDestruct(RV32RegisterAllocator* alloc);
void RV32RegisterAllocatorDelete(RV32RegisterAllocator* alloc);

void RV32AllocateRegisters(RV32RegisterAllocator* emitter);
const char* RV32RegisterName(RV32Register* reg, char* buf, size_t len);

#endif /* risc_v32_reg_alloc_h */
