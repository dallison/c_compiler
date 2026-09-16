#include "bpf_reg_alloc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bpf_codegen.h"
#include "compiler.h"

static int FixedRegNum(TargetInstruction* inst) {
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(resulti):
    case BPF_OP(r0):
      return BPF_REG_0;
    case BPF_OP(r1):
      return BPF_REG_1;
    case BPF_OP(r2):
      return BPF_REG_2;
    case BPF_OP(r3):
      return BPF_REG_3;
    case BPF_OP(r4):
      return BPF_REG_4;
    case BPF_OP(r5):
      return BPF_REG_5;
    case BPF_OP(fp):
    case BPF_OP(sp):
      return BPF_REG_FP;
    default:
      return -1;
  }
}

void BPFRegisterAllocatorInit(BPFRegisterAllocator* alloc, BPFGenerator* bpf) {
  alloc->bpf = bpf;
  for (int i = 0; i < BPF_NUM_REGS; i++) {
    TargetRegisterInit(&alloc->regs[i].base, i);
  }
  alloc->regs[BPF_REG_FP].base.reserved = true;
  BitSetInit(&alloc->used_saved_regs);
  alloc->current_spill_size = 0;
  alloc->max_spill_size = 0;
}

void BPFRegisterAllocatorDestruct(BPFRegisterAllocator* alloc) {
  BitSetDestruct(&alloc->used_saved_regs);
}

const char* BPFRegisterNameFromNum(int num, char* buf, size_t len) {
  if (num == BPF_REG_FP) {
    snprintf(buf, len, "r10");
    return buf;
  }
  snprintf(buf, len, "r%d", num);
  return buf;
}

static void MarkSaved(BPFRegisterAllocator* alloc, int num) {
  if (num >= BPF_FIRST_SAVED_REG && num <= BPF_LAST_SAVED_REG) {
    BitSetInsert(&alloc->used_saved_regs, (size_t)num);
  }
}

static bool OperandUsesReg(TargetInstruction* inst, int num) {
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && op->reg != NULL && op->reg->num == num) {
      return true;
    }
    if (op != NULL && BPFIsFixedRegister(op) && FixedRegNum(op) == num) {
      return true;
    }
  }
  if (inst->dest != NULL && inst->dest->reg != NULL &&
      inst->dest->reg->num == num) {
    return true;
  }
  return false;
}

static BPFRegister* FindFree(BPFRegisterAllocator* alloc, TargetInstruction* inst) {
  static const int kOrder[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  for (size_t i = 0; i < sizeof(kOrder) / sizeof(kOrder[0]); i++) {
    int num = kOrder[i];
    BPFRegister* reg = &alloc->regs[num];
    if (reg->base.reserved) {
      continue;
    }
    if (reg->base.owner != NULL) {
      continue;
    }
    if (OperandUsesReg(inst, num)) {
      continue;
    }
    return reg;
  }
  return NULL;
}

static void FreeRegIfDead(BPFRegisterAllocator* alloc, TargetInstruction* op,
                          TargetInstruction* user) {
  if (op == NULL || op->reg == NULL || BPFIsFixedRegister(op) ||
      BPFIsConst(op) || BPFIsSymbol(op) || BPFIsLabel(op)) {
    return;
  }
  TargetRegister* reg = op->reg;
  if (reg->reserved || reg->owner != op) {
    return;
  }
  bool still_live = false;
  for (size_t i = 0; i < op->users.length; i++) {
    TargetInstruction* u = op->users.value.p[i];
    if (u == user) {
      continue;
    }
    still_live = true;
    break;
  }
  if (!still_live) {
    reg->owner = NULL;
  }
}

static void Assign(BPFRegisterAllocator* alloc, TargetInstruction* inst,
                   BPFRegister* reg) {
  inst->reg = &reg->base;
  reg->base.owner = inst;
  MarkSaved(alloc, reg->base.num);
}

static void SpillAroundCall(BPFRegisterAllocator* alloc,
                            TargetInstruction* call) {
  (void)alloc;
  (void)call;
}

void BPFAllocateRegisters(BPFRegisterAllocator* alloc) {
  BPFGenerator* bpf = alloc->bpf;
  TargetInstruction* inst = TargetFirstInstruction(&bpf->base);
  while (inst != NULL) {
    int fixed = FixedRegNum(inst);
    if (fixed >= 0) {
      inst->reg = &alloc->regs[fixed].base;
      alloc->regs[fixed].base.owner = inst;
      MarkSaved(alloc, fixed);
    }
    inst = TargetNext(inst);
  }

  inst = TargetFirstInstruction(&bpf->base);
  while (inst != NULL) {
    if (BPFIsCall(inst)) {
      SpillAroundCall(alloc, inst);
      for (int r = BPF_REG_1; r <= BPF_REG_5; r++) {
        if (alloc->regs[r].base.owner != NULL &&
            !alloc->regs[r].base.reserved) {
          alloc->regs[r].base.owner = NULL;
        }
      }
    }

    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* op = inst->operand[i];
      if (op == NULL) {
        continue;
      }
      if (op->reg == NULL && BPFIsFixedRegister(op)) {
        int num = FixedRegNum(op);
        if (num >= 0) {
          op->reg = &alloc->regs[num].base;
        }
      }
    }

    if (BPFGeneratesOutput(inst) && inst->reg == NULL) {
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        inst->reg = inst->dest->reg;
        inst->reg->owner = inst;
        MarkSaved(alloc, inst->reg->num);
      } else {
        BPFRegister* reg = FindFree(alloc, inst);
        if (reg == NULL) {
          alloc->max_spill_size += 8;
          reg = &alloc->regs[BPF_REG_9];
        }
        Assign(alloc, inst, reg);
      }
    }

    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      bool seen = false;
      for (int j = 0; j < i; j++) {
        if (inst->operand[j] == inst->operand[i]) {
          seen = true;
          break;
        }
      }
      if (!seen) {
        FreeRegIfDead(alloc, inst->operand[i], inst);
      }
    }
    inst = TargetNext(inst);
  }
}
