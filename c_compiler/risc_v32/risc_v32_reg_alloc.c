//
//  risc_v32_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v32_reg_alloc.h"
#include <stdio.h>
#include "risc_v32_codegen.h"
#include "risc_v32_machine.h"

static void InitializeRegister(RV32Register* reg, int num, RV32RegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void RV32RegisterAllocatorInit(RV32RegisterAllocator* allocator, RV32Generator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < RV32_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kRV32RegTypeInt);
  }

  for (int i = 0; i < RV32_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kRV32RegTypeFloat);
  }

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  allocator->max_spilled_region_size = 0;
}

RV32RegisterAllocator* NewRV32RegisterAllocator(RV32Generator* pcode) {
  RV32RegisterAllocator* reg_alloc = malloc(sizeof(RV32RegisterAllocator));
  RV32RegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void RV32RegisterAllocatorDestruct(RV32RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
}

void RV32RegisterAllocatorDelete(RV32RegisterAllocator* alloc) {
  RV32RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// The names of the ranges of the RISC-V32 ABI registers.
static struct {
  RV32RegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  const char* prefix;   // Register name prefix.
  int base;
} register_ranges[] = {
    {kRV32RegTypeInt, RV32_INT_TEMP_START_1, RV32_INT_TEMP_END_1, "t", 0},
    {kRV32RegTypeInt, RV32_INT_TEMP_START_2, RV32_INT_TEMP_END_2, "t", 3},
    {kRV32RegTypeInt, RV32_INT_ARG_START, RV32_INT_ARG_END, "a", 0},
    {kRV32RegTypeInt, RV32_RET_REG, RV32_RET_REG, "ra", 0},
    {kRV32RegTypeInt, RV32_INT_SAVED_START_1, RV32_INT_SAVED_END_1, "s", 0},
    {kRV32RegTypeInt, RV32_INT_SAVED_START_2, RV32_INT_SAVED_END_2, "s", 2},
    {kRV32RegTypeFloat, RV32_FP_TEMP_START_1, RV32_FP_TEMP_END_1, "ft", 0},
    {kRV32RegTypeFloat, RV32_FP_TEMP_START_2, RV32_FP_TEMP_END_2, "ft", 2},
    {kRV32RegTypeFloat, RV32_FP_ARG_START, RV32_FP_ARG_END, "fa", 0},
    {kRV32RegTypeFloat, RV32_FP_SAVED_START_1, RV32_FP_SAVED_END_1, "fs", 0},
    {kRV32RegTypeFloat, RV32_FP_SAVED_START_2, RV32_FP_SAVED_END_2, "fs", 2},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

RV32RegisterType RV32ResultRegisterType(TargetInstruction* inst) {
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(constf):
    case RV32_OP(constd):
    case RV32_OP(fmv_s):
    case RV32_OP(fmv_d):
    case RV32_OP(fa0):
    case RV32_OP(fa1):
    case RV32_OP(fa2):
    case RV32_OP(fa3):
    case RV32_OP(fa4):
    case RV32_OP(fa5):
    case RV32_OP(fa6):
    case RV32_OP(fa7):
    case RV32_OP(fvarreg):
    case RV32_OP(fmv_w_x):
    case RV32_OP(fneg_d):
    case RV32_OP(fneg_s):
    case RV32_OP(fcvt_s_l):
    case RV32_OP(fcvt_d_l):
      return kRV32RegTypeFloat;

    case RV32_OP(fcvt_w_s):
    case RV32_OP(fcvt_wu_s):
    case RV32_OP(fmv_x_w):
    case RV32_OP(fclass_s):
    case RV32_OP(fclass_d):
    case RV32_OP(fcvt_w_d):
    case RV32_OP(fcvt_wu_d):
    case RV32_OP(fcvt_l_s):
    case RV32_OP(fcvt_lu_s):
    case RV32_OP(fcvt_l_d):
    case RV32_OP(fcvt_lu_d):
    case RV32_OP(fmv_x_d):
      return kRV32RegTypeInt;

    case RV32_OP(tmp):
      // A tmp placeholder is integer unless tagged as holding a float value.
      return (inst->flags & RV32_INST_FLOAT_TMP) ? kRV32RegTypeFloat : kRV32RegTypeInt;

    default:
      if ((RV32Opcode)inst->opcode >= RV32_OP(flw) &&
          (RV32Opcode)inst->opcode <= RV32_OP(fmv_d_x)) {
        return kRV32RegTypeFloat;
      }
      return kRV32RegTypeInt;
  }
}

void RV32AllocateRegisters(RV32RegisterAllocator* allocator) {
  RV32AllocateRegistersByColoring(allocator);
  RV32ShrinkWrap(allocator);
}

const char* RV32RegisterName(RV32Register* reg, char* buf, size_t len) {
  return RV32RegisterNameFromNum(reg->base.num, reg->type, buf, len);
}

const char* RV32RegisterNameFromNum(int num, RV32RegisterType type, char* buf,
                                  size_t len) {
  // See if the register is in one of the named ranges.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type &&
        register_ranges[i].start != RV32_RET_REG) {
      if (num >= register_ranges[i].start && num <= register_ranges[i].end) {
        snprintf(buf, len, "%s%d", register_ranges[i].prefix,
                 num - register_ranges[i].start + register_ranges[i].base);
        return buf;
      }
    }
  }
  switch (type) {
    case kRV32RegTypeInt:
      if (num == RV32_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (num == RV32_FP_REG) {
        snprintf(buf, len, "s0");
        break;
      }
      if (num == RV32_RET_REG) {
        snprintf(buf, len, "ra");
        break;
      }

      snprintf(buf, len, "x%d", num);
      break;

    case kRV32RegTypeFloat:
      snprintf(buf, len, "f%d", num);
      break;
  }
  return buf;
}

#undef NUM_REG_RANGES
