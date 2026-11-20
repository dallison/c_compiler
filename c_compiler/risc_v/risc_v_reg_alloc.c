//
//  risc_v_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_reg_alloc.h"
#include <stdio.h>
#include "risc_v_codegen.h"
#include "risc_v_machine.h"

static void InitializeRegister(RVRegister* reg, int num, RVRegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void RVRegisterAllocatorInit(RVRegisterAllocator* allocator, RVGenerator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < RV_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kRVRegTypeInt);
  }

  for (int i = 0; i < RV_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kRVRegTypeFloat);
  }

  for (int i = 0; i < RV_NUM_VECTOR_REGS; i++) {
    InitializeRegister(&allocator->vector_regs[i], i, kRVRegTypeVector);
  }

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  BitSetInit(&allocator->used_vector_regs);
  allocator->max_spilled_region_size = 0;
}

RVRegisterAllocator* NewRVRegisterAllocator(RVGenerator* pcode) {
  RVRegisterAllocator* reg_alloc = malloc(sizeof(RVRegisterAllocator));
  RVRegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void RVRegisterAllocatorDestruct(RVRegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->used_vector_regs);
}

void RVRegisterAllocatorDelete(RVRegisterAllocator* alloc) {
  RVRegisterAllocatorDestruct(alloc);
  free(alloc);
}

// The names of the ranges of the RISC-V ABI registers.
static struct {
  RVRegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  const char* prefix;   // Register name prefix.
  int base;
} register_ranges[] = {
    {kRVRegTypeInt, RV_INT_TEMP_START_1, RV_INT_TEMP_END_1, "t", 0},
    {kRVRegTypeInt, RV_INT_TEMP_START_2, RV_INT_TEMP_END_2, "t", 3},
    {kRVRegTypeInt, RV_INT_ARG_START, RV_INT_ARG_END, "a", 0},
    {kRVRegTypeInt, RV_RET_REG, RV_RET_REG, "ra", 0},
    {kRVRegTypeInt, RV_INT_SAVED_START_1, RV_INT_SAVED_END_1, "s", 0},
    {kRVRegTypeInt, RV_INT_SAVED_START_2, RV_INT_SAVED_END_2, "s", 2},
    {kRVRegTypeFloat, RV_FP_TEMP_START_1, RV_FP_TEMP_END_1, "ft", 0},
    {kRVRegTypeFloat, RV_FP_TEMP_START_2, RV_FP_TEMP_END_2, "ft", 2},
    {kRVRegTypeFloat, RV_FP_ARG_START, RV_FP_ARG_END, "fa", 0},
    {kRVRegTypeFloat, RV_FP_SAVED_START_1, RV_FP_SAVED_END_1, "fs", 0},
    {kRVRegTypeFloat, RV_FP_SAVED_START_2, RV_FP_SAVED_END_2, "fs", 2},
    {kRVRegTypeVector, RV_VECTOR_TEMP_START, RV_VECTOR_TEMP_END, "v",
     RV_VECTOR_TEMP_START},
    {kRVRegTypeVector, RV_VECTOR_SAVED_START, RV_VECTOR_SAVED_END, "v",
     RV_VECTOR_SAVED_START},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

RVRegisterType RVResultRegisterType(TargetInstruction* inst) {
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(constf):
    case RV_OP(constd):
    case RV_OP(fmv_s):
    case RV_OP(fmv_d):
    case RV_OP(fa0):
    case RV_OP(fa1):
    case RV_OP(fa2):
    case RV_OP(fa3):
    case RV_OP(fa4):
    case RV_OP(fa5):
    case RV_OP(fa6):
    case RV_OP(fa7):
    case RV_OP(fvarreg):
    case RV_OP(fmv_w_x):
    case RV_OP(fneg_d):
    case RV_OP(fneg_s):
    case RV_OP(fcvt_s_l):
    case RV_OP(fcvt_d_l):
    case RV_OP(fcvt_s_w):
    case RV_OP(fcvt_s_wu):
    case RV_OP(fcvt_d_w):
    case RV_OP(fcvt_d_wu):
      return kRVRegTypeFloat;

    case RV_OP(vle):
    case RV_OP(vadd):
    case RV_OP(vsub):
    case RV_OP(vmul):
    case RV_OP(vdiv):
    case RV_OP(vdivu):
    case RV_OP(vmod):
    case RV_OP(vmodu):
    case RV_OP(vlsl):
    case RV_OP(vlsr):
    case RV_OP(vasr):
    case RV_OP(vand):
    case RV_OP(vor):
    case RV_OP(vxor):
    case RV_OP(vfadd):
    case RV_OP(vfsub):
    case RV_OP(vfmul):
    case RV_OP(vfdiv):
    case RV_OP(vcmeq):
    case RV_OP(vcmne):
    case RV_OP(vcmlt):
    case RV_OP(vcmle):
    case RV_OP(vcmltu):
    case RV_OP(vcmleu):
      return kRVRegTypeVector;

    case RV_OP(fcvt_w_s):
    case RV_OP(fcvt_wu_s):
    case RV_OP(fmv_x_w):
    case RV_OP(fclass_s):
    case RV_OP(fclass_d):
    case RV_OP(fcvt_w_d):
    case RV_OP(fcvt_wu_d):
    case RV_OP(fcvt_l_s):
    case RV_OP(fcvt_lu_s):
    case RV_OP(fcvt_l_d):
    case RV_OP(fcvt_lu_d):
    case RV_OP(fmv_x_d):
      return kRVRegTypeInt;

    case RV_OP(tmp):
      // A tmp placeholder is integer unless tagged as holding a float value.
      return (inst->flags & RV_INST_FLOAT_TMP) ? kRVRegTypeFloat : kRVRegTypeInt;

    default:
      if ((RVOpcode)inst->opcode >= RV_OP(flw) &&
          (RVOpcode)inst->opcode <= RV_OP(fmv_d_x)) {
        return kRVRegTypeFloat;
      }
      return kRVRegTypeInt;
  }
}

void RVAllocateRegisters(RVRegisterAllocator* allocator) {
  RVAllocateRegistersByColoring(allocator);
  RVShrinkWrap(allocator);
}

const char* RVRegisterName(RVRegister* reg, char* buf, size_t len) {
  return RVRegisterNameFromNum(reg->base.num, reg->type, buf, len);
}

const char* RVRegisterNameFromNum(int num, RVRegisterType type, char* buf,
                                  size_t len) {
  // See if the register is in one of the named ranges.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type &&
        register_ranges[i].start != RV_RET_REG) {
      if (num >= register_ranges[i].start && num <= register_ranges[i].end) {
        snprintf(buf, len, "%s%d", register_ranges[i].prefix,
                 num - register_ranges[i].start + register_ranges[i].base);
        return buf;
      }
    }
  }
  switch (type) {
    case kRVRegTypeInt:
      if (num == RV_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (num == RV_FP_REG) {
        snprintf(buf, len, "s0");
        break;
      }
      if (num == RV_RET_REG) {
        snprintf(buf, len, "ra");
        break;
      }

      snprintf(buf, len, "x%d", num);
      break;

    case kRVRegTypeFloat:
      snprintf(buf, len, "f%d", num);
      break;

    case kRVRegTypeVector:
      snprintf(buf, len, "v%d", num);
      break;
  }
  return buf;
}

#undef NUM_REG_RANGES
