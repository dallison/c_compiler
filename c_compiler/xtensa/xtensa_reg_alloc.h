//
//  xtensa_reg_alloc.h
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef xtensa_reg_alloc_h
#define xtensa_reg_alloc_h

// RISC-V32 register allocator.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "map.h"
#include "target_generator.h"
#include "xtensa_machine.h"

struct XTENSAGenerator;
struct XTENSAInstruction;

// We have 2 register types, integer (32 bit), floating point.
typedef enum {
  kXTENSARegTypeInt,
  kXTENSARegTypeFloat,
} XTENSARegisterType;

typedef struct XTENSARegister {
  TargetRegister base;
  XTENSARegisterType type;
} XTENSARegister;

// The RISC-V32 R32F and R32D instructions share floating point registers.

#define XTENSA_INT_RETURN_REG \
  XTENSA_INT_ARG_START  // Integer return value register.
#define XTENSA_FLOAT_RETURN_REG \
  XTENSA_FP_ARG_START  // Float/Double return value register.
#define XTENSA_MAX_ALLOCATION_DEPTH 32

typedef struct {
  struct XTENSAGenerator* rv;
  XTENSARegister int_regs[XTENSA_NUM_INT_REGS];
  XTENSARegister float_regs[XTENSA_NUM_FLOAT_REGS];

  // We need to save some registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;

  int current_spilled_region_size;
  int max_spilled_region_size;
  BitSet preserved_instructions;  // Instructions needing preserved regs.
  // Reassignable values (variable registers and merge temporaries) need their
  // spill slot refreshed after every target instruction that defines them.
  Map reassignable_spills;
  // Instructions whose own allocation is in progress.  Allocating one can spill
  // another value, and once the reload pass for an in-flight instruction has
  // run, a read of that value has to be repaired rather than retargeted:
  // pointing the operand at the spill slot then leaves it naming a register the
  // spill has given away.
  TargetInstruction* allocating[XTENSA_MAX_ALLOCATION_DEPTH];
  bool allocating_reloaded[XTENSA_MAX_ALLOCATION_DEPTH];
  size_t allocating_depth;
} XTENSARegisterAllocator;

void XTENSARegisterAllocatorInit(XTENSARegisterAllocator* alloc,
                                 struct XTENSAGenerator* rv);
XTENSARegisterAllocator* NewXTENSARegisterAllocator(
    struct XTENSAGenerator* pcode);
const char* XTENSARegisterNameFromNum(int num, XTENSARegisterType type,
                                      char* buf, size_t len);

void XTENSARegisterAllocatorDestruct(XTENSARegisterAllocator* alloc);
void XTENSARegisterAllocatorDelete(XTENSARegisterAllocator* alloc);

void XTENSAAllocateRegisters(XTENSARegisterAllocator* emitter);
const char* XTENSARegisterName(XTENSARegister* reg, char* buf, size_t len);

#endif /* xtensa_reg_alloc_h */
