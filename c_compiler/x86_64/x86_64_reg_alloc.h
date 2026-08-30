//
//  x86_64_reg_alloc.h
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef x86_64_reg_alloc_h
#define x86_64_reg_alloc_h

// x86-64 register allocator.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "map.h"
#include "x86_64_machine.h"
#include "target_generator.h"

struct X86_64Generator;
struct X86_64Instruction;

// We have 2 register types, integer (64 bit), float int point (64 bit).
typedef enum {
  kX86_64RegTypeInt,
  kX86_64RegTypeFloat,
} X86_64RegisterType;

typedef struct X86_64Register {
  TargetRegister base;
  X86_64RegisterType type;
} X86_64Register;

// Allocating for one instruction can recurse into its destination register
// variable, and no further.
#define X86_64_MAX_ALLOCATION_DEPTH 4


// The x86-64 R32F and R32D instructions share floating point registers.

// Logical register 1 is rax; index 0 is the dedicated zero register.
#define X86_64_INT_RETURN_REG X86_64_RET_REG
#define X86_64_FLOAT_RETURN_REG X86_64_FP_RETURN_VALUE_0

typedef struct {
  struct X86_64Generator* rv;
  X86_64Register int_regs[X86_64_NUM_INT_REGS];
  X86_64Register float_regs[X86_64_NUM_FLOAT_REGS];

  // We need to save some registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;
  
  int current_spilled_region_size;
  int max_spilled_region_size;
  BitSet preserved_instructions;    // Instructions needing preserved regs.

  // Maps a spilled variable register (ivarreg/fvarreg) to the spill
  // instruction that owns its stack slot.  A variable register can be
  // reassigned (e.g. a loop induction variable), so each redefinition must
  // store the new value back into this slot to keep later reloads correct.
  Map varreg_spills;

  // Set when no register could be freed for the value being allocated, so it
  // was given the scratch register instead and has to be written to a spill
  // slot as soon as it is computed.  See AllocateRegisterWithType.
  bool spill_after_definition;

  // The physical registers pinned to a register variable for the length of the
  // function, by ReserveVariableRegisters.  Reserving the logical slot is not
  // enough on its own, because several slots name the same physical register.
  unsigned pinned_int_phys;
  unsigned pinned_float_phys;

  // The instructions currently being allocated for, outermost first.  Every
  // operand of each has to be readable where that instruction runs, so none of
  // them can be chosen as a spill victim in the meantime.  Allocation nests
  // when an instruction's destination is a register variable, hence a stack
  // rather than a single instruction.  See FindSpillVictim.
  TargetInstruction* allocating[X86_64_MAX_ALLOCATION_DEPTH];
  size_t allocating_depth;
} X86_64RegisterAllocator;

void X86_64RegisterAllocatorInit(X86_64RegisterAllocator* alloc,
                             struct X86_64Generator* rv);
X86_64RegisterAllocator* NewX86_64RegisterAllocator(struct X86_64Generator* pcode);
const char* X86_64RegisterNameFromNum(int num, X86_64RegisterType type, char* buf,
                                  size_t len);

void X86_64RegisterAllocatorDestruct(X86_64RegisterAllocator* alloc);
void X86_64RegisterAllocatorDelete(X86_64RegisterAllocator* alloc);

void X86_64AllocateRegisters(X86_64RegisterAllocator* emitter);
const char* X86_64RegisterName(X86_64Register* reg, char* buf, size_t len);

#endif /* x86_64_reg_alloc_h */
