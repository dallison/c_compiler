#include "aarch64_encode.h"

#include <assert.h>

#include "aarch64_codegen.h"
#include "aarch64_reg_alloc.h"
#include "aarch64_program.h"
#include "bitset.h"
#include "target_generator.h"

AARCH64AsmRegister AARCH64AsmZeroRegister(AARCH64AsmRegKind kind) {
  AARCH64AsmRegister reg = {.kind = kind, .num = 31};
  return reg;
}

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
    if (inst->block == NULL) {
      continue;
    }
    if (inst->dest != NULL) {
      return false;
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
        return false;
    }
  }
  return true;
}

void AARCH64DirectEncodeFunction(AARCH64Generator* generator,
                                 Assembler* assembler) {
  assert(AARCH64CanDirectEncodeFunction(generator));
  for (TargetInstruction* inst = TargetFirstInstruction(&generator->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst->block == NULL) {
      continue;
    }
    switch ((AARCH64Opcode)inst->opcode) {
      case AARCH64_OP(movz): {
        bool is_64bit = GetRegisterSize(inst) == kSize64Bit;
        uint16_t immediate = (uint16_t)TargetIntValue(inst->operand[0]);
        AssemblerEmitWord(
            assembler, assembler->object.current_section,
            (int32_t)(AARCH64EncodeMoveWide(is_64bit, 2, immediate, 0) |
                      (uint32_t)inst->reg->num));
        break;
      }
      case AARCH64_OP(ret):
        AssemblerEmitWord(assembler, assembler->object.current_section,
                          (int32_t)AARCH64EncodeReturn(30));
        break;
      default:
        break;
    }
  }
}

void AARCH64DirectEncodeFunctionToModule(AARCH64Generator* generator,
                                         AsmModule* module) {
  assert(AARCH64CanDirectEncodeFunction(generator));
  for (TargetInstruction* inst = TargetFirstInstruction(&generator->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst->block == NULL) {
      continue;
    }
    switch ((AARCH64Opcode)inst->opcode) {
      case AARCH64_OP(movz): {
        bool is_64bit = GetRegisterSize(inst) == kSize64Bit;
        uint16_t immediate = (uint16_t)TargetIntValue(inst->operand[0]);
        AARCH64ProgramEmitWord(
            module, AARCH64EncodeMoveWide(is_64bit, 2, immediate, 0) |
                        (uint32_t)inst->reg->num);
        break;
      }
      case AARCH64_OP(ret):
        AARCH64ProgramEmitWord(module, AARCH64EncodeReturn(30));
        break;
      default:
        break;
    }
  }
}

uint32_t AARCH64EncodeAddSubImmediate(const AARCH64AsmRegister* rd,
                                      const AARCH64AsmRegister* rn, int immed,
                                      bool subtract, bool set_flags,
                                      int shift) {
  int op = subtract ? 1 : 0;
  int s = set_flags ? 1 : 0;
  if (immed < 0) {
    immed = -immed;
    op ^= 1;
  }
  int sf = AARCH64AsmRegSf(rd);
  return (uint32_t)((sf << 31) | (op << 30) | (s << 29) | (0x22 << 23) |
                    (shift << 22) | ((immed & 0xfff) << 10) |
                    (rn->num << 5) | rd->num);
}

uint32_t AARCH64EncodeAddSubShiftedRegister(const AARCH64AsmRegister* rd,
                                            const AARCH64AsmRegister* rn,
                                            const AARCH64AsmOperand* rm,
                                            bool subtract, bool set_flags) {
  int sf = AARCH64AsmRegSf(rd);
  int op = subtract ? 1 : 0;
  int s = set_flags ? 1 : 0;
  return (uint32_t)((sf << 31) | (op << 30) | (s << 29) | (0xb << 24) |
                    (rm->shift.type << 22) | (rm->reg.num << 16) |
                    (rm->shift.amount << 10) | (rn->num << 5) | rd->num);
}

uint32_t AARCH64EncodeAddSubWithCarry(const AARCH64AsmRegister* rd,
                                      const AARCH64AsmRegister* rn,
                                      const AARCH64AsmRegister* rm,
                                      bool subtract, bool set_flags) {
  int sf = AARCH64AsmRegSf(rd);
  int op = subtract ? 1 : 0;
  int s = set_flags ? 1 : 0;
  return (uint32_t)((sf << 31) | (op << 30) | (s << 29) | (0xd0 << 21) |
                    (rm->num << 16) | (rn->num << 5) | rd->num);
}

static bool AARCH64IsMask64(uint64_t v) {
  return v != 0 && ((v + 1) & v) == 0;
}

static bool AARCH64IsShiftedMask64(uint64_t v) {
  return v != 0 && AARCH64IsMask64((v - 1) | v);
}

static unsigned AARCH64CountTrailingZeros64(uint64_t value) {
  unsigned count = 0;
  while ((value & 1) == 0) {
    ++count;
    value >>= 1;
  }
  return count;
}

static unsigned AARCH64CountLeadingZeros64(uint64_t value) {
  unsigned count = 0;
  for (uint64_t bit = 1ULL << 63; (value & bit) == 0; bit >>= 1) {
    ++count;
  }
  return count;
}

static unsigned AARCH64PopulationCount64(uint64_t value) {
  unsigned count = 0;
  while (value != 0) {
    value &= value - 1;
    ++count;
  }
  return count;
}

bool AARCH64EncodeLogicalImmediate(uint64_t imm, int sf, unsigned* encoding) {
  unsigned reg_size = sf ? 64 : 32;
  if (reg_size != 64) {
    if ((imm >> reg_size) != 0 && imm != (uint64_t)(int64_t)(int32_t)imm) {
      return false;
    }
    imm &= 0xffffffffULL;
  }
  uint64_t all_ones = reg_size == 64 ? ~0ULL : 0xffffffffULL;
  if (imm == 0 || imm == all_ones) {
    return false;
  }

  unsigned size = reg_size;
  do {
    size /= 2;
    uint64_t mask = (1ULL << size) - 1;
    if ((imm & mask) != ((imm >> size) & mask)) {
      size *= 2;
      break;
    }
  } while (size > 2);

  uint64_t mask = (~0ULL) >> (64 - size);
  imm &= mask;

  unsigned i;
  unsigned cto;
  if (AARCH64IsShiftedMask64(imm)) {
    i = AARCH64CountTrailingZeros64(imm);
    cto = AARCH64CountTrailingZeros64(~(imm >> i));
  } else {
    imm |= ~mask;
    if (!AARCH64IsShiftedMask64(~imm)) {
      return false;
    }
    unsigned clo = AARCH64CountLeadingZeros64(~imm);
    i = 64 - clo;
    cto = size - AARCH64PopulationCount64(~imm);
  }

  unsigned immr = (size - i) & (size - 1);
  unsigned nimms = (~(size - 1)) << 1;
  nimms |= (cto - 1);
  unsigned n = ((nimms >> 6) & 1) ^ 1;
  *encoding = ((n & 1) << 12) | (immr << 6) | (nimms & 0x3f);
  return true;
}

uint32_t AARCH64EncodeLogicalImmediateInst(const AARCH64AsmRegister* rd,
                                           const AARCH64AsmRegister* rn,
                                           int64_t immed, int sf, int opc) {
  unsigned encoding;
  if (!AARCH64EncodeLogicalImmediate((uint64_t)immed, sf, &encoding)) {
    return 0;
  }
  return (uint32_t)((sf << 31) | (opc << 29) | (0x24 << 23) |
                    (encoding << 10) | (rn->num << 5) | rd->num);
}

uint32_t AARCH64EncodeLogicalShiftedRegister(const AARCH64AsmRegister* rd,
                                             const AARCH64AsmRegister* rn,
                                             const AARCH64AsmOperand* rm, int sf,
                                             int opc, int invert_rn) {
  return (uint32_t)((sf << 31) | (opc << 29) | (0xa << 24) | (invert_rn << 21) |
                    (rm->shift.type << 22) | (rm->reg.num << 16) |
                    (rm->shift.amount << 10) | (rn->num << 5) | rd->num);
}

uint32_t AARCH64EncodeConditionalBranch(int32_t offset, AARCH64AsmCondition cond,
                                        bool consistent) {
  return (uint32_t)((0x2a << 25) | (((offset >> 2) & 0x7ffff) << 5) |
                    (consistent << 4) | cond);
}

uint32_t AARCH64EncodeUnconditionalBranchImmediate(int32_t offset, bool link) {
  return (uint32_t)((link ? 1u : 0u) << 31) | (0x5u << 26) |
         ((uint32_t)(offset >> 2) & 0x3ffffffu);
}

uint32_t AARCH64EncodeRotateRightImmediate(const AARCH64AsmRegister* rd,
                                           const AARCH64AsmRegister* rn,
                                           int64_t shift) {
  int sf = AARCH64AsmRegSf(rd);
  return (uint32_t)((sf << 31) | (0x13 << 24) | (1 << 23) | (sf << 22) |
                    (rn->num << 16) | ((int)shift << 10) | (rn->num << 5) |
                    rd->num);
}

uint32_t AARCH64EncodeSvc(uint16_t immediate) {
  return 0xd4000001u | ((uint32_t)immediate << 5);
}

uint32_t AARCH64EncodeDataProcessing3Source(bool is_64bit, int op54, int o0,
                                            int rd, int rn, int rm, int ra) {
  return ((uint32_t)is_64bit << 31) | (0x1bu << 24) |
         ((uint32_t)op54 << 21) | ((uint32_t)rm << 16) |
         ((uint32_t)o0 << 15) | ((uint32_t)ra << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeDivide(bool is_64bit, bool unsigned_divide, int rd,
                             int rn, int rm) {
  return ((uint32_t)is_64bit << 31) | (0xd6u << 21) |
         ((uint32_t)rm << 16) |
         ((uint32_t)(unsigned_divide ? 2 : 3) << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeVariableShift(bool is_64bit, int shift_op, int rd,
                                    int rn, int rm) {
  return ((uint32_t)is_64bit << 31) | (0xd6u << 21) |
         ((uint32_t)rm << 16) | ((uint32_t)shift_op << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeHighMultiply(bool unsigned_multiply, int rd, int rn,
                                   int rm) {
  return (1u << 31) | (0x1bu << 24) |
         ((uint32_t)(unsigned_multiply ? 6 : 2) << 21) |
         ((uint32_t)rm << 16) | (0x1fu << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeBitfield(bool is_64bit, int opc, int rd, int rn,
                               int immr, int imms) {
  return ((uint32_t)is_64bit << 31) | ((uint32_t)opc << 29) |
         (0x26u << 23) | ((uint32_t)is_64bit << 22) |
         (((uint32_t)immr & 0x3fu) << 16) |
         (((uint32_t)imms & 0x3fu) << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeDataProcessing1Source(bool is_64bit, int opcode, int rd,
                                            int rn) {
  return ((uint32_t)is_64bit << 31) | (0x2d6u << 21) |
         ((uint32_t)opcode << 10) | ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeExtract(bool is_64bit, int rd, int rn, int rm, int lsb) {
  return ((uint32_t)is_64bit << 31) | (0x27u << 23) |
         ((uint32_t)is_64bit << 22) | ((uint32_t)rm << 16) |
         (((uint32_t)lsb & 0x3fu) << 10) | ((uint32_t)rn << 5) |
         (uint32_t)rd;
}

uint32_t AARCH64EncodeCompareBranch(bool is_64bit, bool nonzero, int rt) {
  return ((uint32_t)is_64bit << 31) | (0x1au << 25) |
         ((uint32_t)nonzero << 24) | (uint32_t)rt;
}

uint32_t AARCH64EncodeTestBranch(int bit, bool nonzero, int rt) {
  return ((uint32_t)(bit >> 5) << 31) | (0x1bu << 25) |
         ((uint32_t)nonzero << 24) |
         (((uint32_t)bit & 0x1fu) << 19) | (uint32_t)rt;
}

uint32_t AARCH64EncodeConditionalCompare(bool is_64bit, bool negative,
                                         bool immediate, int rn, int op2,
                                         int nzcv,
                                         AARCH64AsmCondition cond) {
  return ((uint32_t)is_64bit << 31) | ((uint32_t)negative << 30) |
         (0x1d2u << 21) | ((uint32_t)op2 << 16) |
         ((uint32_t)cond << 12) | ((uint32_t)immediate << 11) |
         ((uint32_t)rn << 5) | ((uint32_t)nzcv & 15u);
}

uint32_t AARCH64EncodeConditionalSelect(bool is_64bit, int op, int op2,
                                        int rd, int rn, int rm,
                                        AARCH64AsmCondition cond) {
  return ((uint32_t)is_64bit << 31) | ((uint32_t)op << 30) |
         (0xd4u << 21) | ((uint32_t)rm << 16) |
         ((uint32_t)cond << 12) | ((uint32_t)op2 << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeLoadStoreUnsigned(int size, bool fp, int opc, int rt,
                                        int rn, int offset) {
  int scale = 1 << size;
  int immediate = offset / scale;
  return ((uint32_t)size << 30) | (0x39u << 24) |
         ((uint32_t)fp << 26) | ((uint32_t)opc << 22) |
         ((uint32_t)immediate << 10) | ((uint32_t)rn << 5) |
         (uint32_t)rt;
}

uint32_t AARCH64EncodeLoadStoreUnscaled(int size, bool fp, int opc, int rt,
                                        int rn, int offset, int mode) {
  return ((uint32_t)size << 30) | (0x7u << 27) |
         ((uint32_t)fp << 26) | ((uint32_t)opc << 22) |
         (((uint32_t)offset & 0x1ffu) << 12) |
         ((uint32_t)mode << 10) | ((uint32_t)rn << 5) | (uint32_t)rt;
}

uint32_t AARCH64EncodeLoadStoreRegister(int size, int opc, int rt, int rn,
                                        int rm, int option, bool scaled) {
  return ((uint32_t)size << 30) | (0x7u << 27) |
         ((uint32_t)opc << 22) | (1u << 21) |
         ((uint32_t)rm << 16) | ((uint32_t)option << 13) |
         ((uint32_t)scaled << 12) | (2u << 10) |
         ((uint32_t)rn << 5) | (uint32_t)rt;
}

uint32_t AARCH64EncodeLoadStorePair(int opc, bool fp, bool load, int mode,
                                    int rt, int rt2, int rn, int offset) {
  int scale = opc == 2 ? 8 : 4;
  int immediate = offset / scale;
  return ((uint32_t)opc << 30) | (0x28u << 24) |
         ((uint32_t)fp << 26) | ((uint32_t)mode << 23) |
         ((uint32_t)load << 22) |
         (((uint32_t)immediate & 0x7fu) << 15) |
         ((uint32_t)rt2 << 10) | ((uint32_t)rn << 5) | (uint32_t)rt;
}

uint32_t AARCH64EncodeExclusiveLoad(int size, bool acquire, int rt, int rn) {
  return ((uint32_t)size << 30) | 0x085f7c00u |
         (acquire ? 0x00008000u : 0) | ((uint32_t)rn << 5) |
         (uint32_t)rt;
}

uint32_t AARCH64EncodeExclusiveStore(int size, bool release, int status,
                                     int rt, int rn) {
  return ((uint32_t)size << 30) | 0x08007c00u |
         (release ? 0x00008000u : 0) | ((uint32_t)status << 16) |
         ((uint32_t)rn << 5) | (uint32_t)rt;
}

uint32_t AARCH64EncodeAcquireRelease(int size, bool load, int rt, int rn) {
  uint32_t base = load ? 0x08dffc00u : 0x089ffc00u;
  return ((uint32_t)size << 30) | base | ((uint32_t)rn << 5) |
         (uint32_t)rt;
}

uint32_t AARCH64EncodeDmb(int option) {
  return 0xd50330bfu | ((uint32_t)option << 8);
}

uint32_t AARCH64EncodeMrsTpidrEl0(int rt) {
  return 0xd53bd040u | (uint32_t)rt;
}

uint32_t AARCH64EncodeFPDataProcessing2(bool is_double, int opcode, int rd,
                                        int rn, int rm) {
  return 0x1e200000u | ((uint32_t)is_double << 22) |
         ((uint32_t)opcode << 10) | ((uint32_t)rm << 16) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeFPSqrt(bool is_double, int rd, int rn) {
  return 0x1e21c000u | ((uint32_t)is_double << 22) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeFPBitcast(bool destination_is_fp, bool is_64bit, int rd,
                                int rn) {
  uint32_t base = destination_is_fp
                      ? (is_64bit ? 0x9e670000u : 0x1e270000u)
                      : (is_64bit ? 0x9e660000u : 0x1e260000u);
  return base | ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeFPConvertPrecision(bool source_is_double,
                                         bool destination_is_double, int rd,
                                         int rn) {
  return 0x1e224000u | ((uint32_t)source_is_double << 22) |
         ((uint32_t)destination_is_double << 15) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeFPToInt(uint32_t base, bool int_is_64bit,
                              bool source_is_double, bool unsigned_convert,
                              int rd, int rn) {
  return base | ((uint32_t)int_is_64bit << 31) |
         ((uint32_t)source_is_double << 22) |
         ((uint32_t)unsigned_convert << 16) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeIntToFP(bool int_is_64bit, bool destination_is_double,
                              bool unsigned_convert, int rd, int rn) {
  return 0x1e220000u | ((uint32_t)int_is_64bit << 31) |
         ((uint32_t)destination_is_double << 22) |
         ((uint32_t)unsigned_convert << 16) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeFPMove(bool is_double, int rd, int rn) {
  return 0x1e204000u | ((uint32_t)is_double << 22) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

uint32_t AARCH64EncodeFPCompare(bool is_double, int rn, int rm) {
  return 0x1e202000u | ((uint32_t)is_double << 22) |
         ((uint32_t)rm << 16) | ((uint32_t)rn << 5);
}

uint32_t AARCH64EncodeFPNegate(bool is_double, int rd, int rn) {
  return 0x1e214000u | ((uint32_t)is_double << 22) |
         ((uint32_t)rn << 5) | (uint32_t)rd;
}

void AARCH64EmitInstruction(AsmObject* object, uint32_t word) {
  AARCH64EmitInstructionInSection(object, object->current_section, word);
}

void AARCH64EmitInstructionInSection(AsmObject* object, int32_t section,
                                     uint32_t word) {
  AsmObjectEmitWord(object, section, (int32_t)word);
}

void AARCH64EmitAddSubImmediate(AsmObject* object, const AARCH64AsmRegister* rd,
                                const AARCH64AsmRegister* rn, int immed,
                                bool subtract, bool set_flags, int shift) {
  AARCH64EmitInstruction(
      object, AARCH64EncodeAddSubImmediate(rd, rn, immed, subtract, set_flags,
                                           shift));
}

void AARCH64EmitAddSubShiftedRegister(AsmObject* object,
                                      const AARCH64AsmRegister* rd,
                                      const AARCH64AsmRegister* rn,
                                      const AARCH64AsmOperand* rm, bool subtract,
                                      bool set_flags) {
  AARCH64EmitInstruction(object, AARCH64EncodeAddSubShiftedRegister(
                                     rd, rn, rm, subtract, set_flags));
}

void AARCH64EmitAddSubWithCarry(AsmObject* object, const AARCH64AsmRegister* rd,
                                const AARCH64AsmRegister* rn,
                                const AARCH64AsmRegister* rm, bool subtract,
                                bool set_flags) {
  AARCH64EmitInstruction(object,
                         AARCH64EncodeAddSubWithCarry(rd, rn, rm, subtract,
                                                      set_flags));
}

void AARCH64EmitLogicalImmediate(AsmObject* object, const AARCH64AsmRegister* rd,
                                 const AARCH64AsmRegister* rn, int64_t immed,
                                 int sf, int opc) {
  AARCH64EmitInstruction(object, AARCH64EncodeLogicalImmediateInst(
                                     rd, rn, immed, sf, opc));
}

void AARCH64EmitLogicalShiftedRegister(AsmObject* object,
                                       const AARCH64AsmRegister* rd,
                                       const AARCH64AsmRegister* rn,
                                       const AARCH64AsmOperand* rm, int sf,
                                       int opc, int invert_rn) {
  AARCH64EmitInstruction(object, AARCH64EncodeLogicalShiftedRegister(
                                     rd, rn, rm, sf, opc, invert_rn));
}

void AARCH64EmitMoveWide(AsmObject* object, const AARCH64AsmRegister* rd, int sf,
                         int opc, int imm16, int hw) {
  AARCH64EmitInstruction(object, AARCH64EncodeMoveWide(sf != 0, opc, imm16, hw) |
                                    (uint32_t)rd->num);
}

void AARCH64EmitMoveImmediate(AsmObject* object, const AARCH64AsmRegister* rd,
                              int64_t immed) {
  bool inverted = false;
  if (immed < 0) {
    immed = ~immed;
    inverted = true;
  }
  int sf = AARCH64AsmRegSf(rd);
  if (immed == 0) {
    AARCH64EmitMoveWide(object, rd, sf, inverted ? 0 : 2, 0, 0);
    return;
  }
  int num_words = AARCH64AsmRegIs64Bit(rd) ? 4 : 2;
  if (inverted) {
    int num_non_zero_words = 0;
    int n = 0;
    for (int i = 0; i < num_words; i++) {
      if (((immed >> (i * 16)) & 0xffff) != 0) {
        num_non_zero_words++;
        n = i;
      }
    }
    if (num_non_zero_words == 1) {
      int imm16 = (int)((immed >> (n * 16)) & 0xffff);
      AARCH64EmitMoveWide(object, rd, sf, 0, imm16, n);
      return;
    }
    immed = ~immed;
  }

  bool keep = false;
  for (int i = 0; i < num_words; i++) {
    int imm16 = (int)((immed >> (i * 16)) & 0xffff);
    if (imm16 != 0) {
      int opc = keep ? 3 : 2;
      AARCH64EmitMoveWide(object, rd, sf, opc, imm16, i);
      keep = true;
    }
  }
}

void AARCH64EmitBranchRegister(AsmObject* object, int opc, int op3, int reg) {
  AARCH64EmitInstruction(object, AARCH64EncodeBranchRegister(opc, op3, reg));
}

void AARCH64EmitReturn(AsmObject* object, int reg) {
  AARCH64EmitInstruction(object, AARCH64EncodeReturn(reg));
}

void AARCH64EmitConditionalBranch(AsmObject* object, int32_t offset,
                                  AARCH64AsmCondition cond, bool consistent) {
  AARCH64EmitInstruction(object,
                         AARCH64EncodeConditionalBranch(offset, cond,
                                                        consistent));
}

void AARCH64EmitUnconditionalBranchImmediate(AsmObject* object, int32_t offset,
                                             bool link) {
  AARCH64EmitInstruction(
      object, AARCH64EncodeUnconditionalBranchImmediate(offset, link));
}

void AARCH64EmitUnconditionalBranchToSymbol(Assembler* assembler,
                                            AssemblerSymbol* sym, bool link,
                                            bool pic) {
  AsmObject* object = &assembler->object;
  int32_t instruction_offset = (int32_t)AsmObjectCurrentAddress(object);
  bool known = sym->defined && sym->section == object->current_section &&
               sym->binding != SYM_BIND(weak);
  int32_t offset = known ? (int32_t)(sym->value - instruction_offset) : 0;
  if (!known) {
    int reloc_type = link ? R_AARCH64_CALL26 : R_AARCH64_JUMP26;
    if (link && (sym->binding == SYM_BIND(global) ||
                 sym->binding == SYM_BIND(weak)) &&
        pic) {
      reloc_type = R_AARCH64_CALL_PLT;
    }
    AsmObjectAddRelocationForSymbol(object, sym, reloc_type,
                                    object->current_section,
                                    instruction_offset, 0);
  } else {
    int32_t off = offset < 0 ? -offset : offset;
    if (off > (1 << 28) || (off & 3) != 0) {
      AssemblerError(assembler, "Branch offset out of range");
      return;
    }
  }
  AARCH64EmitUnconditionalBranchImmediate(object, offset, link);
}
