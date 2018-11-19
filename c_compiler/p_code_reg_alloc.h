//
//  p_code_reg_alloc.h
//  c_compiler
//
//  Created by David Allison on 1/7/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef p_code_reg_alloc_h
#define p_code_reg_alloc_h

// P-Code register allocator.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bitset.h"
#include "p_code_machine.h"
#include "target_generator.h"

struct PCodeGenerator;
struct PCodeInstruction;

// We have 3 register types, integer (64 bit), float (32 bit) and double (64
// bit).
typedef enum {
  kPCodeRegTypeInt,
  kPCodeRegTypeFloat,
  kPCodeRegTypeDouble,
} PCodeRegisterType;

typedef struct PCodeRegister {
  TargetRegister base;
  PCodeRegisterType type;
} PCodeRegister;

#define PCODE_INT_RETURN_REG 0     // Integer return value register.
#define PCODE_FLOAT_RETURN_REG 0   // Float return value register.
#define PCODE_DOUBLE_RETURN_REG 0  // Double return value register.

typedef struct {
  struct PCodeGenerator* pcode;
  PCodeRegister int_regs[PCODE_NUM_INT_REGS];
  PCodeRegister float_regs[PCODE_NUM_FLOAT_REGS];
  PCodeRegister double_regs[PCODE_NUM_DOUBLE_REGS];

  // We need to save registers on entry to a procedure and restore them on
  // exit.  These bit sets keep a record of the registers of each type that
  // we have used.
  BitSet used_int_regs;
  BitSet used_float_regs;
  BitSet used_double_regs;
} PCodeRegisterAllocator;

void PCodeRegisterAllocatorInit(PCodeRegisterAllocator* alloc,
                                struct PCodeGenerator* pcode);
PCodeRegisterAllocator* NewPCodeRegisterAllocator(struct PCodeGenerator* pcode);

void PCodeRegisterAllocatorDestruct(PCodeRegisterAllocator* alloc);
void PCodeRegisterAllocatorDelete(PCodeRegisterAllocator* alloc);

void PCodeAllocateRegisters(PCodeRegisterAllocator* emitter);
const char* PCodeRegisterName(PCodeRegister* reg, char* buf, size_t len);

#endif /* p_code_reg_alloc_h */
