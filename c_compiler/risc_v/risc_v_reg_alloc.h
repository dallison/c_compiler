//
//  risc_v_reg_alloc.h
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_reg_alloc_h
#define risc_v_reg_alloc_h

// RISC-V register allocator.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "map.h"
#include "risc_v_machine.h"
#include "target_generator.h"

struct RVGenerator;
struct RVInstruction;

// Integer (64-bit), float (64-bit F/D), and vector (VLEN=128 RVV).
typedef enum {
  kRVRegTypeInt,
  kRVRegTypeFloat,
  kRVRegTypeVector,
} RVRegisterType;

typedef struct RVRegister {
  TargetRegister base;
  RVRegisterType type;
} RVRegister;


// The RISC-V R32F and R32D instructions share floating point registers.

#define RV_INT_RETURN_REG RV_INT_ARG_START  // Integer return value register.
#define RV_FLOAT_RETURN_REG \
  RV_FP_ARG_START  // Float/Double return value register.
#define RV_MAX_ALLOCATION_DEPTH 32

typedef struct {
  struct RVGenerator* rv;
  RVRegister int_regs[RV_NUM_INT_REGS];
  RVRegister float_regs[RV_NUM_FLOAT_REGS];
  RVRegister vector_regs[RV_NUM_VECTOR_REGS];

  // We need to save some registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;
  BitSet used_vector_regs;
  
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
  TargetInstruction* allocating[RV_MAX_ALLOCATION_DEPTH];
  bool allocating_reloaded[RV_MAX_ALLOCATION_DEPTH];
  size_t allocating_depth;
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
