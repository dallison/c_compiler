#include "bpf_reg_alloc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bpf_codegen.h"
#include "compiler.h"

static int FixedRegNum(TargetInstruction* inst) {
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(resulti):
    case BPF_OP(r0):
    case BPF_OP(call):
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

static BPFRegister* FindFree(BPFRegisterAllocator* alloc, TargetInstruction* inst,
                             int exclude) {
  // Prefer callee-saved r6-r9 so values survive calls; r1-r5 are also usable
  // scratch when those four are full.
  static const int kOrder[] = {6, 7, 8, 9, 1, 2, 3, 4, 5};
  for (size_t i = 0; i < sizeof(kOrder) / sizeof(kOrder[0]); i++) {
    int num = kOrder[i];
    if (num == exclude) {
      continue;
    }
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

typedef struct {
  int target;
  int branch;
} BPFBackEdge;

// A value used in a loop and defined before that loop is read again when the
// branch is taken.  The users list only records each instruction once, so a
// purely forward scan would reuse its register before the back edge.
static bool LiveAcrossBackEdge(TargetInstruction* op, int here,
                               const BPFBackEdge* edges, int nedges) {
  for (int e = 0; e < nedges; e++) {
    int target = edges[e].target;
    int branch = edges[e].branch;
    if (here >= branch || op->addr > target) {
      continue;
    }
    for (size_t i = 0; i < op->users.length; i++) {
      int use = ((TargetInstruction*)op->users.value.p[i])->addr;
      if (use >= target && use <= branch) {
        return true;
      }
    }
  }
  return false;
}

static void FreeRegIfDead(BPFRegisterAllocator* alloc, TargetInstruction* op,
                          TargetInstruction* user, const BPFBackEdge* edges,
                          int nedges) {
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
  if (!still_live && LiveAcrossBackEdge(op, user->addr, edges, nedges)) {
    still_live = true;
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

static bool ValueInReg(TargetInstruction* op) {
  return op != NULL && op->reg != NULL && op->reg->owner == op;
}

static bool NeedsValueRegister(TargetInstruction* op) {
  if (op == NULL || BPFIsConst(op) || BPFIsSymbol(op) || BPFIsLabel(op) ||
      BPFIsFixedRegister(op) || BPFIsSpill(op)) {
    return false;
  }
  return true;
}

// Spill slots sit below the locals, addressed as [r10 - (frame + slot*8)].
// The slot number lives in the high 16 bits of the instruction flags.
static int EnsureSpillSlot(BPFRegisterAllocator* alloc, TargetInstruction* inst) {
  int slot = (inst->flags >> 16) & 0xffff;
  if (slot == 0) {
    slot = alloc->max_spill_size / 8 + 1;
    alloc->max_spill_size = slot * 8;
    inst->flags = (inst->flags & 0xffff) | (slot << 16);
  }
  inst->flags |= TARGET_INST_SPILLED;
  return slot;
}

static int SpillByteOffset(BPFRegisterAllocator* alloc, int slot) {
  return -(alloc->bpf->base.stack_frame_size + slot * 8);
}

static BPFRegister* ObtainReg(BPFRegisterAllocator* alloc,
                              TargetInstruction* inst, TargetInstruction* before,
                              int exclude, const BPFBackEdge* edges,
                              int nedges);

static void EmitSpill(BPFRegisterAllocator* alloc, TargetInstruction* value,
                      TargetInstruction* before) {
  int off = SpillByteOffset(alloc, EnsureSpillSlot(alloc, value));
  if (off < -32768 || off > 32767) {
    fprintf(stderr, "eBPF backend: spill offset %d does not fit in imm16\n",
            off);
  }
  TargetGenerator* gen = &alloc->bpf->base;
  TargetInstruction* spill = TargetNewInstruction3(
      (TargetOpcode)BPF_OP(spill), TargetFramePointer(gen),
      TargetGetIntConstant(gen, NULL, kTargetType64Bit, off), value);
  // Printing reads the source register later.  Keep the register that holds
  // the value here; the definition's own register stays put so the store and
  // the instruction that produced the value name the same register.
  spill->reg = value->reg;
  TargetEmitBefore(gen, spill, before);
}

// Reload `value` for `user` without changing value->reg.  The definition is
// printed from the register it was assigned; only this use reads the reload.
static void ReloadForUse(BPFRegisterAllocator* alloc, TargetInstruction* user,
                         TargetInstruction* value, int exclude,
                         const BPFBackEdge* edges, int nedges) {
  int slot = (value->flags >> 16) & 0xffff;
  int off = SpillByteOffset(alloc, slot);
  TargetGenerator* gen = &alloc->bpf->base;
  BPFRegister* reg = ObtainReg(alloc, user, user, exclude, edges, nedges);
  TargetInstruction* reload = TargetNewInstruction2(
      (TargetOpcode)BPF_OP(reload), TargetFramePointer(gen),
      TargetGetIntConstant(gen, NULL, kTargetType64Bit, off));
  reload->reg = &reg->base;
  reg->base.owner = reload;
  MarkSaved(alloc, reg->base.num);
  TargetEmitBefore(gen, reload, user);
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (user->operand[i] == value) {
      TargetReplaceOperand(user, i, reload);
    }
  }
}

// Write `value`'s spill slot back into the register its already-emitted users
// still name.  The reload sits behind the scan, so it is not allocated again.
static void ReloadIntoRegister(BPFRegisterAllocator* alloc,
                               TargetInstruction* value,
                               TargetInstruction* before) {
  int slot = (value->flags >> 16) & 0xffff;
  if (slot == 0 || before == NULL || value->reg == NULL) {
    return;
  }
  TargetGenerator* gen = &alloc->bpf->base;
  TargetInstruction* reload = TargetNewInstruction2(
      (TargetOpcode)BPF_OP(reload), TargetFramePointer(gen),
      TargetGetIntConstant(gen, NULL, kTargetType64Bit,
                           SpillByteOffset(alloc, slot)));
  reload->reg = value->reg;
  TargetEmitBefore(gen, reload, before);
}

// An in-place update (i = i + 1) shares dest with the value it reads.  The
// header sees the register the update just wrote; refilling the pre-update
// spill would undo the increment.
static bool UpdatedInLoop(BPFGenerator* bpf, TargetInstruction* owner,
                          int target, int branch) {
  for (TargetInstruction* inst = TargetFirstInstruction(&bpf->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst == owner || inst->addr <= target || inst->addr > branch ||
        inst->dest == NULL) {
      continue;
    }
    if (inst->dest == owner ||
        (owner->dest != NULL && inst->dest == owner->dest)) {
      return true;
    }
  }
  return false;
}

// `before` overwrites a register that a loop header already reads.  That read
// ran before this spill in the forward scan, so it keeps the original
// register number.  Store the value just after it is defined (a jump to the
// header must not skip the store) and refill that register before the
// earliest header use, which runs on every iteration.
static void RestoreAcrossBackEdge(BPFRegisterAllocator* alloc,
                                  TargetInstruction* owner,
                                  TargetInstruction* before,
                                  const BPFBackEdge* edges, int nedges) {
  if (owner == NULL || owner->reg == NULL || before == NULL || edges == NULL) {
    return;
  }
  bool spilled_at_def = false;
  for (int e = 0; e < nedges; e++) {
    int target = edges[e].target;
    int branch = edges[e].branch;
    if (owner->addr >= target || before->addr < target ||
        before->addr > branch ||
        UpdatedInLoop(alloc->bpf, owner, target, branch)) {
      continue;
    }
    TargetInstruction* earliest = NULL;
    for (size_t i = 0; i < owner->users.length; i++) {
      TargetInstruction* user = owner->users.value.p[i];
      int use = user->addr;
      if (use < target || use > branch || use >= before->addr) {
        continue;
      }
      if (earliest == NULL || use < earliest->addr) {
        earliest = user;
      }
    }
    if (earliest == NULL) {
      continue;
    }
    if (!spilled_at_def) {
      EmitSpill(alloc, owner, TargetNext(owner));
      spilled_at_def = true;
    }
    ReloadIntoRegister(alloc, owner, earliest);
  }
}

static void SpillOwner(BPFRegisterAllocator* alloc, BPFRegister* reg,
                       TargetInstruction* before, const BPFBackEdge* edges,
                       int nedges) {
  TargetInstruction* owner = reg->base.owner;
  if (owner != NULL && !BPFIsFixedRegister(owner)) {
    EmitSpill(alloc, owner, before);
    RestoreAcrossBackEdge(alloc, owner, before, edges, nedges);
    // If users name the coalesced home rather than the defining instruction,
    // that home has to reload the same slot.  Skip the home when the
    // instruction we are making room for writes it: those users want the
    // new value, which stays in the register.
    TargetInstruction* home = owner->dest;
    if (home != NULL && home != owner &&
        (before == NULL || home != before->dest)) {
      int slot = (owner->flags >> 16) & 0xffff;
      home->flags = (home->flags & 0xffff) | (slot << 16);
      home->flags |= TARGET_INST_SPILLED;
    }
  }
  reg->base.owner = NULL;
}

// A move into r1–r5 has no user: the call instruction does not mention the
// argument registers.  Spilling that move to make room for a later argument
// leaves the call with whatever leftover value now sits in the register.
static bool ArgRegHeldForCall(BPFRegister* reg) {
  TargetInstruction* owner = reg->base.owner;
  if (owner == NULL || owner->dest == NULL || owner->dest->reg != &reg->base) {
    return false;
  }
  int num = reg->base.num;
  return num >= BPF_REG_0 && num <= BPF_REG_5;
}

static BPFRegister* ObtainReg(BPFRegisterAllocator* alloc,
                              TargetInstruction* inst, TargetInstruction* before,
                              int exclude, const BPFBackEdge* edges,
                              int nedges) {
  BPFRegister* reg = FindFree(alloc, inst, exclude);
  if (reg != NULL) {
    return reg;
  }
  static const int kSpillOrder[] = {1, 2, 3, 4, 5, 9, 8, 7, 6};
  for (size_t i = 0; i < sizeof(kSpillOrder) / sizeof(kSpillOrder[0]); i++) {
    int num = kSpillOrder[i];
    if (num == exclude || OperandUsesReg(inst, num)) {
      continue;
    }
    reg = &alloc->regs[num];
    if (reg->base.reserved || ArgRegHeldForCall(reg)) {
      continue;
    }
    SpillOwner(alloc, reg, before, edges, nedges);
    return reg;
  }
  fprintf(stderr, "eBPF backend: no register to spill\n");
  return &alloc->regs[BPF_REG_9];
}

static bool LiveAfter(TargetInstruction* op, TargetInstruction* here,
                      const BPFBackEdge* edges, int nedges) {
  for (size_t i = 0; i < op->users.length; i++) {
    if (((TargetInstruction*)op->users.value.p[i])->addr > here->addr) {
      return true;
    }
  }
  return LiveAcrossBackEdge(op, here->addr, edges, nedges);
}

void BPFAllocateRegisters(BPFRegisterAllocator* alloc) {
  BPFGenerator* bpf = alloc->bpf;
  int count = 0;
  for (TargetInstruction* inst = TargetFirstInstruction(&bpf->base);
       inst != NULL; inst = TargetNext(inst)) {
    inst->addr = count++;
  }
  int edge_cap = 8;
  int nedges = 0;
  BPFBackEdge* edges = malloc((size_t)edge_cap * sizeof(BPFBackEdge));
  for (TargetInstruction* inst = TargetFirstInstruction(&bpf->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (!BPFIsBranch(inst)) {
      continue;
    }
    TargetInstruction* target = BPFGetBranchTarget(inst);
    if (target == NULL || target->addr > inst->addr) {
      continue;
    }
    if (nedges == edge_cap) {
      edge_cap *= 2;
      edges = realloc(edges, (size_t)edge_cap * sizeof(BPFBackEdge));
    }
    edges[nedges].target = target->addr;
    edges[nedges].branch = inst->addr;
    nedges++;
  }

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

  // The fixed-register pass points r1–r5 at their placeholder instructions.
  // Those placeholders are not live values; leaving them as owners would
  // lock the argument registers out of allocation for the whole function.
  for (int r = BPF_REG_1; r <= BPF_REG_9; r++) {
    TargetInstruction* owner = alloc->regs[r].base.owner;
    if (owner != NULL && BPFIsFixedRegister(owner)) {
      alloc->regs[r].base.owner = NULL;
    }
  }

  inst = TargetFirstInstruction(&bpf->base);
  while (inst != NULL) {
    if (BPFIsCall(inst)) {
      for (int r = BPF_REG_1; r <= BPF_REG_5; r++) {
        TargetInstruction* owner = alloc->regs[r].base.owner;
        if (owner == NULL || BPFIsFixedRegister(owner) ||
            OperandUsesReg(inst, r)) {
          continue;
        }
        if (LiveAfter(owner, inst, edges, nedges)) {
          SpillOwner(alloc, &alloc->regs[r], inst, edges, nedges);
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

    int result_reg = -1;
    if (BPFGeneratesOutput(inst) && inst->reg == NULL && inst->dest != NULL &&
        inst->dest->reg != NULL) {
      result_reg = inst->dest->reg->num;
    }
    // A later source sitting in the result register is clobbered by the
    // move of source 0 into the destination.  Park it elsewhere first.
    if (result_reg >= 0) {
      for (int i = 1; i < TARGET_MAX_OPERANDS; i++) {
        TargetInstruction* op = inst->operand[i];
        if (op == NULL || !ValueInReg(op) || op->reg->num != result_reg) {
          continue;
        }
        SpillOwner(alloc, (BPFRegister*)op->reg, inst, edges, nedges);
        ReloadForUse(alloc, inst, op, result_reg, edges, nedges);
      }
    }
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* op = inst->operand[i];
      if (!NeedsValueRegister(op) || ValueInReg(op)) {
        continue;
      }
      if ((op->flags & TARGET_INST_SPILLED) == 0) {
        continue;
      }
      ReloadForUse(alloc, inst, op, result_reg, edges, nedges);
    }

    if (BPFGeneratesOutput(inst) && inst->reg == NULL) {
      if (result_reg >= 0) {
        BPFRegister* reg = &alloc->regs[result_reg];
        if (reg->base.owner != NULL && reg->base.owner != inst) {
          TargetInstruction* prev = reg->base.owner;
          // An in-place update (i = i + 1) shares its destination with the
          // value it reads and must keep that register.  Reloading the
          // pre-update spill on the next iteration would spin forever.
          // Any other live occupant, including one that merely shares the
          // destination pointer, still has to move to the stack: a later
          // user of the old instruction would otherwise read this result.
          bool same_value =
              inst->dest != NULL &&
              (inst->dest == prev ||
               (prev->dest != NULL && inst->dest == prev->dest));
          if (!same_value &&
              (LiveAfter(prev, inst, edges, nedges) ||
               !OperandUsesReg(inst, result_reg))) {
            SpillOwner(alloc, reg, inst, edges, nedges);
          }
        }
        inst->reg = &reg->base;
        reg->base.owner = inst;
        MarkSaved(alloc, result_reg);
        // The destination already has a spill slot.  A later use reloads that
        // slot, so an update (flag = 0 after flag = 1) has to refresh it.
        // On the path that skips this instruction the previous store remains.
        if (inst->dest != NULL &&
            (inst->dest->flags & TARGET_INST_SPILLED) != 0) {
          int slot = (inst->dest->flags >> 16) & 0xffff;
          if (slot != 0) {
            inst->flags = (inst->flags & 0xffff) | (slot << 16);
            inst->flags |= TARGET_INST_SPILLED;
            EmitSpill(alloc, inst, TargetNext(inst));
          }
        }
      } else {
        Assign(alloc, inst, ObtainReg(alloc, inst, inst, -1, edges, nedges));
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
        FreeRegIfDead(alloc, inst->operand[i], inst, edges, nedges);
      }
    }
    if (BPFIsCall(inst)) {
      // The callee clobbers r0–r5.  r0 may hold the stack-argument pointer;
      // r1–r5 hold the register arguments.  Live values were spilled above.
      for (int r = BPF_REG_0; r <= BPF_REG_5; r++) {
        alloc->regs[r].base.owner = NULL;
      }
    }
    inst = TargetNext(inst);
  }
  free(edges);
}
