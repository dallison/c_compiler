//
//  p_code_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 1/7/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "p_code_reg_alloc.h"
#include "p_code_codegen.h"
#include "p_code_machine.h"
#include "target_basic_block.h"

static void InitializeRegister(PCodeRegister* reg, int num,
                               PCodeRegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void PCodeRegisterAllocatorInit(PCodeRegisterAllocator* allocator,
                                PCodeGenerator* pcode) {
  allocator->pcode = pcode;

  for (int i = 0; i < PCODE_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kPCodeRegTypeInt);
  }

  for (int i = 0; i < PCODE_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kPCodeRegTypeFloat);
  }

  for (int i = 0; i < PCODE_NUM_DOUBLE_REGS; i++) {
    InitializeRegister(&allocator->double_regs[i], i, kPCodeRegTypeDouble);
  }

  // Reserve some registers.
  allocator->int_regs[PCODE_FP_REG].base.reserved = true;
  allocator->int_regs[PCODE_SP_REG].base.reserved = true;
  allocator->int_regs[PCODE_AP_REG].base.reserved = true;
  allocator->int_regs[PCODE_TP_REG].base.reserved = true;
  allocator->int_regs[PCODE_TMP1_REG].base.reserved = true;
  allocator->int_regs[PCODE_TMP2_REG].base.reserved = true;
  allocator->int_regs[PCODE_TMP3_REG].base.reserved = true;
  allocator->int_regs[PCODE_TMP4_REG].base.reserved = true;

  // We reserve the function return registers for simplicity.
  allocator->int_regs[PCODE_INT_RETURN_REG].base.reserved = true;
  allocator->float_regs[PCODE_FLOAT_RETURN_REG].base.reserved = true;
  allocator->double_regs[PCODE_DOUBLE_RETURN_REG].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  BitSetInit(&allocator->used_double_regs);
}

PCodeRegisterAllocator* NewPCodeRegisterAllocator(PCodeGenerator* pcode) {
  PCodeRegisterAllocator* reg_alloc = malloc(sizeof(PCodeRegisterAllocator));
  PCodeRegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void PCodeRegisterAllocatorDestruct(PCodeRegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->used_double_regs);
}

void PCodeRegisterAllocatorDelete(PCodeRegisterAllocator* alloc) {
  PCodeRegisterAllocatorDestruct(alloc);
  free(alloc);
}


static PCodeRegisterType RegisterTypeFromTmpUsers(TargetInstruction* inst) {
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* user = inst->users.value.p[i];
    for (size_t operand = 0; operand < TARGET_MAX_OPERANDS; operand++) {
      if (user->operand[operand] != inst) {
        continue;
      }
      switch ((PCodeOpcode)user->opcode) {
        case P_OP(movf):
        case P_OP(stf):
        case P_OP(addf):
        case P_OP(subf):
        case P_OP(mulf):
        case P_OP(divf):
        case P_OP(negf):
        case P_OP(f2i):
        case P_OP(f2ui):
        case P_OP(f2d):
        case P_OP(cmpeqf):
        case P_OP(cmpnef):
        case P_OP(cmpltf):
        case P_OP(cmplef):
        case P_OP(cmpgtf):
        case P_OP(cmpgef):
        case P_OP(cmp3wayf):
          return kPCodeRegTypeFloat;

        case P_OP(movd):
        case P_OP(std):
        case P_OP(addd):
        case P_OP(subd):
        case P_OP(muld):
        case P_OP(divd):
        case P_OP(negd):
        case P_OP(d2i):
        case P_OP(d2ui):
        case P_OP(d2f):
        case P_OP(cmpeqd):
        case P_OP(cmpned):
        case P_OP(cmpltd):
        case P_OP(cmpled):
        case P_OP(cmpgtd):
        case P_OP(cmpged):
        case P_OP(cmp3wayd):
          return kPCodeRegTypeDouble;

        default:
          break;
      }
    }
  }
  return kPCodeRegTypeInt;
}

static PCodeRegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(mov):
    case P_OP(movc):
    case P_OP(movxc):
    case P_OP(adr):
    case P_OP(adrs):
    case P_OP(adrtls):
    case P_OP(ldw):
    case P_OP(ldh):
    case P_OP(ldb):
    case P_OP(lduw):
    case P_OP(ldub):
    case P_OP(lduh):
    case P_OP(lda):
    case P_OP(ldx):
    case P_OP(stw):
    case P_OP(sth):
    case P_OP(stx):
    case P_OP(stb):
    case P_OP(add):
    case P_OP(addc):
    case P_OP(sub):
    case P_OP(mul):
    case P_OP(div):
    case P_OP(divu):
    case P_OP(mod):
    case P_OP(modu):
    case P_OP(lsr):
    case P_OP(asr):
    case P_OP(lsl):
    case P_OP(or):
    case P_OP(and):
    case P_OP(xor):
    case P_OP(not):
    case P_OP(inv):
    case P_OP(neg):
    case P_OP(cmpeq):
    case P_OP(cmpne):
    case P_OP(cmplt):
    case P_OP(cmple):
    case P_OP(cmpgt):
    case P_OP(cmpge):
    case P_OP(cmpltu):
    case P_OP(cmpleu):
    case P_OP(cmpgtu):
    case P_OP(cmpgeu):
    case P_OP(cmpeqf):
    case P_OP(cmpnef):
    case P_OP(cmpltf):
    case P_OP(cmplef):
    case P_OP(cmpgtf):
    case P_OP(cmpgef):
    case P_OP(cmpeqd):
    case P_OP(cmpned):
    case P_OP(cmpltd):
    case P_OP(cmpled):
    case P_OP(cmpgtd):
    case P_OP(cmpged):
    case P_OP(cmp3way):
    case P_OP(cmp3wayu):
    case P_OP(cmp3wayf):
    case P_OP(cmp3wayd):
    case P_OP(f2i):
    case P_OP(d2i):
    case P_OP(f2ui):
    case P_OP(d2ui):
    case P_OP(literal):
    case P_OP(fp):
    case P_OP(sp):
    case P_OP(ap):
    case P_OP(tp):
    case P_OP(resulti):
    case P_OP(call):
    case P_OP(rcall):
    case P_OP(structreturn):
      return kPCodeRegTypeInt;

    case P_OP(tmp):
      return RegisterTypeFromTmpUsers(inst);

    case P_OP(movf):
    case P_OP(movfc):
    case P_OP(ldf):
    case P_OP(stf):
    case P_OP(addf):
    case P_OP(subf):
    case P_OP(mulf):
    case P_OP(divf):
    case P_OP(negf):
    case P_OP(i2f):
    case P_OP(ui2f):
    case P_OP(d2f):
    case P_OP(resultf):
    case P_OP(callf):
    case P_OP(rcallf):
      return kPCodeRegTypeFloat;

    case P_OP(movd):
    case P_OP(movdc):
    case P_OP(ldd):
    case P_OP(std):
    case P_OP(addd):
    case P_OP(subd):
    case P_OP(muld):
    case P_OP(divd):
    case P_OP(negd):
    case P_OP(resultd):
    case P_OP(calld):
    case P_OP(rcalld):
    case P_OP(i2d):
    case P_OP(ui2d):
    case P_OP(f2d):
      return kPCodeRegTypeDouble;
    default:
      assert(false);
      return 0;
  }
}

PCodeRegisterType PCodeInstructionRegisterType(TargetInstruction* inst) {
  return RegisterTypeFromInstruction(inst);
}

const char* PCodeRegisterName(PCodeRegister* reg, char* buf, size_t len) {
  switch (reg->type) {
    case kPCodeRegTypeInt:
      if (reg->base.num == PCODE_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (reg->base.num == PCODE_FP_REG) {
        snprintf(buf, len, "fp");
        break;
      }
      if (reg->base.num == PCODE_AP_REG) {
        snprintf(buf, len, "ap");
        break;
      }
      if (reg->base.num == PCODE_TP_REG) {
        snprintf(buf, len, "tp");
        break;
      }
      snprintf(buf, len, "r%d", reg->base.num);
      break;
    case kPCodeRegTypeFloat:
      snprintf(buf, len, "f%d", reg->base.num);
      break;
    case kPCodeRegTypeDouble:
      snprintf(buf, len, "d%d", reg->base.num);
      break;
  }
  return buf;
}
void PCodeAllocateRegisters(PCodeRegisterAllocator* allocator) {
  TargetBuildBasicBlocks(&allocator->pcode->base);
  PCodeAllocateRegistersByColoring(allocator);
}
