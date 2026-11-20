//
//  arm_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "arm_reg_alloc.h"
#include <stdio.h>
#include "arm_codegen.h"
#include "arm_machine.h"

static const char* ARMRegisterNameFromNum1(int num, ARMRegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                           size_t len);

static void InitializeRegister(ARMRegister* reg, int num, ARMRegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void ARMRegisterAllocatorInit(ARMRegisterAllocator* allocator,
                             struct ARMGenerator* g) {
  allocator->g = g;

  for (int i = 0; i < ARM_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kARMRegTypeInt);
  }

  for (int i = 0; i < ARM_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kARMRegTypeFloat);
  }

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  allocator->max_spilled_region_size = 0;
}

ARMRegisterAllocator* NewARMRegisterAllocator(struct ARMGenerator* g) {
  ARMRegisterAllocator* reg_alloc = malloc(sizeof(ARMRegisterAllocator));
  ARMRegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void ARMRegisterAllocatorDestruct(ARMRegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
}

void ARMRegisterAllocatorDelete(ARMRegisterAllocator* alloc) {
  ARMRegisterAllocatorDestruct(alloc);
  free(alloc);
}

// Size for registers.
#define kSize32Bit 1
#define kSize64Bit 2
#define kSize128Bit 3

static ARMRegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(constf):
    case ARM_OP(constd):
    case ARM_OP(fmv_s):
    case ARM_OP(fmv_d):
    case ARM_OP(d0):
    case ARM_OP(d1):
    case ARM_OP(d2):
    case ARM_OP(d3):
    case ARM_OP(d4):
    case ARM_OP(d5):
    case ARM_OP(d6):
    case ARM_OP(d7):
    case ARM_OP(fldr):
    case  ARM_OP(fstr):
    case  ARM_OP(fadd):
    case  ARM_OP(fsub):
    case  ARM_OP(fmul):
    case  ARM_OP(fdiv):
    case  ARM_OP(fsqrt):
    case  ARM_OP(fmin):
    case  ARM_OP(fmax):
    case  ARM_OP(fcvtsd):     // Single to double
    case  ARM_OP(fcvtds):     // Double to single.
    case  ARM_OP(fcvt):     // Copy from int reg (no conversion)
    case  ARM_OP(fmov):
    case  ARM_OP(fcmp):
    case  ARM_OP(scvtf):
    case  ARM_OP(ucvtf):
    case  ARM_OP(fneg):
    case ARM_OP(vadd):
    case ARM_OP(vsub):
    case ARM_OP(vand):
    case ARM_OP(vorr):
    case ARM_OP(veor):
    case ARM_OP(vcmeq):
    case ARM_OP(vcmgt):
    case ARM_OP(vcmge):
    case ARM_OP(vcmhi):
    case ARM_OP(vcmhs):
    case ARM_OP(vfadd):
    case ARM_OP(vfsub):
    case ARM_OP(vfmul):
    case ARM_OP(fvarreg):
      return kARMRegTypeFloat;

    case  ARM_OP(fcvtns):
    case  ARM_OP(fcvtnu):
      return kARMRegTypeInt;

    case ARM_OP(tmp):
      // A bare temporary defaults to an integer register unless it was tagged
      // as holding a floating-point value (e.g. a `double` ?: merge slot).
      return (inst->flags & kARMFloatValue) ? kARMRegTypeFloat : kARMRegTypeInt;

    default:
      if ((ARMOpcode)inst->opcode >= ARM_OP(fldr) &&
          (ARMOpcode)inst->opcode <= ARM_OP(vfmul)) {
        return kARMRegTypeFloat;
      }
      return kARMRegTypeInt;
  }
}

bool ARMHasFloatResult(TargetInstruction* inst) {
  return RegisterTypeFromInstruction(inst) == kARMRegTypeFloat;
}

void ARMAllocateRegisters(ARMRegisterAllocator* allocator) {
  ARMAllocateRegistersByColoring(allocator);
}

const char* ARMRegisterName(ARMRegister* reg, int size, char* buf, size_t len) {
  return ARMRegisterNameFromNum(reg->base.num, reg->type, size, buf, len);
}

static const char* ARMRegisterNameFromNum1(int num, ARMRegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                           size_t len) {
  (void)allow_no_size;
  switch (type) {
    case kARMRegTypeInt:
      switch (num) {
        case ARM_FP_REG:
          snprintf(buf, len, "fp");
          return buf;
        case ARM_SP_REG:
          snprintf(buf, len, "sp");
          return buf;
        case ARM_LR_REG:
          snprintf(buf, len, "lr");
          return buf;
        case ARM_IP_REG:
          snprintf(buf, len, "ip");
          return buf;
        case ARM_PC_REG:
          snprintf(buf, len, "pc");
          return buf;
        default:
          snprintf(buf, len, "r%d", num);
          return buf;
      }

    case kARMRegTypeFloat:
      if (size == kSize32Bit || size == 0) {
        snprintf(buf, len, "s%d", num);
      } else if (size == kSize128Bit) {
        snprintf(buf, len, "q%d", num / 4);
      } else {
        snprintf(buf, len, "d%d", num / 2);
      }
      return buf;
  }

  return buf;
}

const char* ARMRegisterNameFromNum(int num, ARMRegisterType type, int size,
                                   char* buf,
                                   size_t len) {
  return ARMRegisterNameFromNum1(num, type, size, false, buf, len);
}
