#include "aarch64_encode.h"

#include <assert.h>

#include "aarch64_codegen.h"
#include "assembler.h"
#include "bitset.h"
#include "target_generator.h"

uint32_t AARCH64EncodeMoveWide(bool is_64bit, int opc, uint16_t immediate,
                               int halfword) {
  return ((uint32_t)is_64bit << 31) | ((uint32_t)opc << 29) |
         (0x25u << 23) | ((uint32_t)halfword << 21) |
         ((uint32_t)immediate << 5);
}

uint32_t AARCH64EncodeBranchRegister(int opc, int op3, int reg) {
  return (0x6bu << 25) | ((uint32_t)opc << 21) | (0x1fu << 16) |
         ((uint32_t)op3 << 10) | (0x5u << 26) | ((uint32_t)reg << 5);
}

uint32_t AARCH64EncodeReturn(int reg) {
  return AARCH64EncodeBranchRegister(2, 0, reg);
}

static bool HasEmptyStackFrame(AARCH64Generator* generator) {
  AARCH64RegisterAllocator* regs = &generator->register_allocator;
  return generator->base.stack_frame_size == 0 &&
         generator->base.num_calls == 0 && !generator->not_leaf &&
         !generator->base.varargs && generator->exception_ranges.length == 0 &&
         BitSetCount(&regs->used_int_regs) == 0 &&
         BitSetCount(&regs->used_float_regs) == 0 &&
         regs->max_spilled_region_size == 0;
}

bool AARCH64CanDirectEncodeFunction(AARCH64Generator* generator) {
  if (!HasEmptyStackFrame(generator)) {
    return false;
  }

  for (TargetInstruction* inst = TargetFirstInstruction(&generator->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst->block == NULL || inst->observable_checkpoint) {
      continue;
    }
    switch ((AARCH64Opcode)inst->opcode) {
      case AARCH64_OP(save):
      case AARCH64_OP(restore):
      case AARCH64_OP(loc):
        break;
      case AARCH64_OP(symbol):
      case AARCH64_OP(named_label):
        return false;
      case AARCH64_OP(label):
        if ((inst->flags &
             (AARCH64_EXPORTED_LABEL | TARGET_INST_EXCEPTION_LANDING)) != 0) {
          return false;
        }
        break;
      case AARCH64_OP(movz):
        if (inst->reg == NULL || inst->operand[0] == NULL ||
            !TargetIsConst(inst->operand[0]) || inst->operand[1] != NULL ||
            inst->operand[2] != NULL || inst->operand[3] != NULL ||
            TargetIntValue(inst->operand[0]) < 0 ||
            TargetIntValue(inst->operand[0]) > UINT16_MAX ||
            (GetRegisterSize(inst) != kSize32Bit &&
             GetRegisterSize(inst) != kSize64Bit)) {
          return false;
        }
        break;
      case AARCH64_OP(ret):
        if (inst->operand[0] != NULL || inst->operand[1] != NULL ||
            inst->operand[2] != NULL || inst->operand[3] != NULL) {
          return false;
        }
        break;
      default:
        if (AARCH64GeneratesOutput(inst)) {
          return false;
        }
        break;
    }
  }
  return true;
}

void AARCH64DirectEncodeFunction(AARCH64Generator* generator,
                                 Assembler* assembler) {
  assert(AARCH64CanDirectEncodeFunction(generator));
  for (TargetInstruction* inst = TargetFirstInstruction(&generator->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst->block == NULL || inst->observable_checkpoint) {
      continue;
    }
    switch ((AARCH64Opcode)inst->opcode) {
      case AARCH64_OP(movz): {
        bool is_64bit = GetRegisterSize(inst) == kSize64Bit;
        uint16_t immediate = (uint16_t)TargetIntValue(inst->operand[0]);
        AssemblerEmitWord(
            assembler, assembler->current_section,
            (int32_t)(AARCH64EncodeMoveWide(is_64bit, 2, immediate, 0) |
                      (uint32_t)inst->reg->num));
        break;
      }
      case AARCH64_OP(ret):
        AssemblerEmitWord(assembler, assembler->current_section,
                          (int32_t)AARCH64EncodeReturn(30));
        break;
      default:
        break;
    }
  }
}
