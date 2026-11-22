//
//  aarch64_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "aarch64_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include "aarch64_codegen.h"
#include "aarch64_machine.h"
#include "aarch64_optimize.h"
#include "target_basic_block.h"
#include "compiler.h"

static void InitializeRegister(AARCH64Register* reg, int num, AARCH64RegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void AARCH64RegisterAllocatorInit(AARCH64RegisterAllocator* allocator,
                             struct AARCH64Generator* g) {
  allocator->g = g;

  for (int i = 0; i < AARCH64_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kAARCH64RegTypeInt);
  }

  for (int i = 0; i < AARCH64_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kAARCH64RegTypeFloat);
  }

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  allocator->max_spilled_region_size = 0;
  allocator->shrink_wrapped_regs = 0;
}

AARCH64RegisterAllocator* NewAARCH64RegisterAllocator(struct AARCH64Generator* g) {
  AARCH64RegisterAllocator* reg_alloc = malloc(sizeof(AARCH64RegisterAllocator));
  AARCH64RegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void AARCH64RegisterAllocatorDestruct(AARCH64RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
}

void AARCH64RegisterAllocatorDelete(AARCH64RegisterAllocator* alloc) {
  AARCH64RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// Size for registers.
#define kSize32Bit 1
#define kSize64Bit 2


static AARCH64RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  // A value whose opcode does not imply a register class (e.g. a `tmp` merge
  // slot for ?: / && / ||) records its floating-pointness via this flag so the
  // allocator places it in an FP register rather than defaulting to integer.
  if ((inst->flags & AARCH64_INST_FP) != 0) {
    return kAARCH64RegTypeFloat;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(constf):
    case AARCH64_OP(constd):
    case AARCH64_OP(fmv_s):
    case AARCH64_OP(fmv_d):
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
    case AARCH64_OP(fldr):
    case  AARCH64_OP(fstr):
    case  AARCH64_OP(fadd):
    case  AARCH64_OP(fsub):
    case  AARCH64_OP(fmul):
    case  AARCH64_OP(fdiv):
    case  AARCH64_OP(fsqrt):
    case  AARCH64_OP(fmin):
    case  AARCH64_OP(fmax):
    case  AARCH64_OP(fcvtsd):     // Single to double
    case  AARCH64_OP(fcvtds):     // Double to single.
    case  AARCH64_OP(fcvt):     // Copy from int reg (no conversion)
    case  AARCH64_OP(fmov):
    case  AARCH64_OP(fcmp):
    case  AARCH64_OP(scvtf):
    case  AARCH64_OP(ucvtf):
    case  AARCH64_OP(fneg):
    case AARCH64_OP(vadd):
    case AARCH64_OP(vsub):
    case AARCH64_OP(vand):
    case AARCH64_OP(vorr):
    case AARCH64_OP(veor):
    case AARCH64_OP(vcmeq):
    case AARCH64_OP(vcmgt):
    case AARCH64_OP(vcmge):
    case AARCH64_OP(vcmhi):
    case AARCH64_OP(vcmhs):
    case AARCH64_OP(vfadd):
    case AARCH64_OP(vfsub):
    case AARCH64_OP(vfmul):
    case AARCH64_OP(vfdiv):
    case AARCH64_OP(fvarreg):
      return kAARCH64RegTypeFloat;

    case  AARCH64_OP(fcvtns):
    case  AARCH64_OP(fcvtnu):
    case  AARCH64_OP(fcvtzs):
    case  AARCH64_OP(fcvtzu):
      return kAARCH64RegTypeInt;

    default:
      if ((AARCH64Opcode)inst->opcode >= AARCH64_OP(fldr) &&
          (AARCH64Opcode)inst->opcode <= AARCH64_OP(fneg)) {
        return kAARCH64RegTypeFloat;
      }
      return kAARCH64RegTypeInt;
  }
}

bool AARCH64HasFloatResult(TargetInstruction* inst) {
  return RegisterTypeFromInstruction(inst) == kAARCH64RegTypeFloat;
}

// Physical register touched by a post-allocation instruction.  SP is 32 and
// is reported so frame-pointer checks can see it; xzr (31) is included when
// the instruction actually names that register.
static int HoistIntReg(TargetInstruction* inst) {
  if (inst == NULL || inst->reg == NULL) {
    return -1;
  }
  AARCH64Register* reg = (AARCH64Register*)inst->reg;
  if (reg->type != kAARCH64RegTypeInt) {
    return -1;
  }
  int num = reg->base.num;
  if (num < 0 || num > 32) {
    return -1;
  }
  return num;
}

static void HoistNoteReg(uint64_t* bits, int num) {
  if (num >= 0 && num <= 32) {
    *bits |= UINT64_C(1) << num;
  }
}

// `call_clobbers` is for copy propagation, which must not forward a
// caller-saved register across a call.  Liveness for hoisting leaves it off
// so a value the allocator kept in a caller-saved register still looks live.
static void HoistInstEffects(TargetInstruction* inst, uint64_t* uses,
                             uint64_t* defs, bool call_clobbers) {
  *uses = 0;
  *defs = 0;
  if (inst == NULL) {
    return;
  }
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (op == AARCH64_OP(loc) || op == AARCH64_OP(label) ||
      op == AARCH64_OP(named_label) || op == AARCH64_OP(eq) ||
      op == AARCH64_OP(ne) || op == AARCH64_OP(al)) {
    return;
  }
  if (op == AARCH64_OP(asm)) {
    *uses = ~UINT64_C(0);
    *defs = ~UINT64_C(0);
    return;
  }
  if (op == AARCH64_OP(reload)) {
    HoistNoteReg(defs, HoistIntReg(inst));
    return;
  }
  if (op == AARCH64_OP(spill)) {
    HoistNoteReg(uses, HoistIntReg(inst));
    return;
  }
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    HoistNoteReg(uses, HoistIntReg(inst->operand[i]));
  }
  if (AARCH64GeneratesOutput(inst)) {
    HoistNoteReg(defs, HoistIntReg(inst));
  }
  if (call_clobbers && (op == AARCH64_OP(bl) || op == AARCH64_OP(blr))) {
    for (int r = 0; r <= 18; r++) {
      *defs |= UINT64_C(1) << r;
    }
  }
  if (call_clobbers && op == AARCH64_OP(restore)) {
    for (int r = AARCH64_INT_SAVED_START; r <= AARCH64_LR_REG; r++) {
      *defs |= UINT64_C(1) << r;
    }
  }
}

static bool HoistIsBranch(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(b):
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz):
    case AARCH64_OP(tbz):
    case AARCH64_OP(tbnz):
    case AARCH64_OP(br):
    case AARCH64_OP(ret):
      return true;
    default:
      return false;
  }
}

static TargetInstruction* HoistNextInBlock(TargetBasicBlock* block,
                                           TargetInstruction* inst) {
  if (inst == NULL || block == NULL || inst == block->end_code) {
    return NULL;
  }
  return TargetNext(inst);
}

static bool HoistLoopHasSideEffect(TargetGenerator* gen, BitSet* blocks) {
  BitSetIterator it;
  BitSetIteratorStart(&it, blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
        if (op == AARCH64_OP(bl) || op == AARCH64_OP(blr) ||
            op == AARCH64_OP(asm) || op == AARCH64_OP(save) ||
            op == AARCH64_OP(restore)) {
          return true;
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

// Slot index stored on the spill, before the emitter adds the frame base.
// Negative slots exist (the aggregate-return slot); failure is a separate
// result so a real offset of -1 is not treated as "no slot".
static bool HoistSpillSlot(TargetInstruction* spill, int* off) {
  if (spill == NULL || (AARCH64Opcode)spill->opcode != AARCH64_OP(spill) ||
      spill->operand[1] == NULL || !TargetIsConst(spill->operand[1])) {
    return false;
  }
  *off = (int)TargetIntValue(spill->operand[1]);
  return true;
}

static bool HoistRefsValue(TargetInstruction* inst, TargetInstruction* value) {
  if (inst == NULL || value == NULL || inst == value) {
    return false;
  }
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] == value) {
      return true;
    }
  }
  return inst->dest == value;
}

// True for a block whose terminator is after `save` in the instruction list.
// Shrink-wrapping moves `save` and retargets branches, but it does not add
// the new block to the predecessor edge lists, so a dominator or edge
// dataflow still sees the fast path falling into the slow path.  The list
// order matches emission: fast blocks, then `save`, then the framed body.
static void HoistFramedAtEnd(TargetGenerator* gen, BitSet* framed) {
  size_t n = gen->basic_blocks.length;
  BitSetInit(framed);
  bool seen_save = false;
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = TargetNext(inst)) {
    if ((AARCH64Opcode)inst->opcode == AARCH64_OP(save)) {
      seen_save = true;
    }
    if (!seen_save || inst->block == NULL) {
      continue;
    }
    size_t id = inst->block->block_id;
    if (id < n && gen->basic_blocks.value.p[id] == inst->block) {
      BitSetInsert(framed, id);
    }
  }
}

typedef struct {
  int header;
  int depth;
  Vector latches;
  Vector entries;
  BitSet blocks;
} HoistLoop;

static void HoistLoopDestruct(void* ptr) {
  HoistLoop* loop = ptr;
  VectorDestruct(&loop->latches);
  VectorDestruct(&loop->entries);
  BitSetDestruct(&loop->blocks);
}

typedef struct {
  TargetInstruction* spill;
  AARCH64Opcode imm_op;
  int size;
  int64_t imm;
  bool is_imm;
  TargetInstruction* members[48];
  int nmembers;
} HoistGroup;

static bool HoistDominatesLatches(TargetGenerator* gen, TargetBasicBlock* block,
                                  HoistLoop* loop) {
  if (block == NULL || loop->latches.length == 0) {
    return false;
  }
  for (size_t i = 0; i < loop->latches.length; i++) {
    TargetBasicBlock* latch =
        gen->basic_blocks.value.p[loop->latches.value.w[i]];
    if (latch == NULL ||
        !BitSetContains(&latch->dominators, block->block_id)) {
      return false;
    }
  }
  return true;
}

static uint64_t HoistLiveAtInsert(TargetBasicBlock* pred,
                                  const Vector* live_out,
                                  const Vector* extra_live) {
  uint64_t live = 0;
  if (pred->block_id < 0) {
    return ~UINT64_C(0);
  }
  live = live_out->value.u[pred->block_id] |
         extra_live->value.u[pred->block_id];
  if (pred->end_code != NULL && HoistIsBranch(pred->end_code)) {
    uint64_t uses = 0;
    uint64_t defs = 0;
    HoistInstEffects(pred->end_code, &uses, &defs, false);
    live = (live & ~defs) | uses;
  }
  return live;
}

static bool HoistEntriesReady(TargetGenerator* gen, HoistLoop* loop,
                              bool need_frame, BitSet* framed) {
  if (loop->entries.length == 0) {
    return false;
  }
  for (size_t i = 0; i < loop->entries.length; i++) {
    size_t pred_id = (size_t)loop->entries.value.w[i];
    TargetBasicBlock* pred = gen->basic_blocks.value.p[pred_id];
    if (pred == NULL || pred->code == NULL || pred->end_code == NULL) {
      return false;
    }
    for (size_t e = 0; e < pred->out_edges.length; e++) {
      size_t succ = pred->out_edges.value.w[e];
      if (!BitSetContains(&loop->blocks, succ)) {
        return false;
      }
    }
    if (need_frame && !BitSetContains(framed, pred_id)) {
      return false;
    }
  }
  return true;
}

static void HoistInsert(TargetGenerator* gen, TargetBasicBlock* pred,
                        TargetInstruction* inst) {
  if (HoistIsBranch(pred->end_code)) {
    TargetBasicBlockEmitBefore(gen, pred, inst, pred->end_code);
  } else {
    TargetBasicBlockEmitAfter(gen, pred, inst, pred->end_code);
  }
}

static TargetInstruction* HoistMakeReload(AARCH64RegisterAllocator* allocator,
                                          TargetInstruction* spill, int reg,
                                          int size) {
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)AARCH64_OP(reload), spill);
  SetInstructionSize(copy, size);
  copy->reg = &allocator->int_regs[reg].base;
  copy->flags |= TARGET_INST_PROCESSED;
  return copy;
}

static TargetInstruction* HoistMakeImmediate(AARCH64RegisterAllocator* allocator,
                                             HoistGroup* group, int reg) {
  TargetInstruction* proto = group->members[0];
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)group->imm_op, proto->operand[0]);
  SetInstructionSize(copy, group->size);
  copy->reg = &allocator->int_regs[reg].base;
  copy->flags |= TARGET_INST_PROCESSED;
  return copy;
}

static void HoistOneGroup(AARCH64RegisterAllocator* allocator, HoistLoop* loop,
                          HoistGroup* group, int reg, Vector* extra_live) {
  TargetGenerator* gen = &allocator->g->base;
  TargetBasicBlock* first_pred =
      gen->basic_blocks.value.p[loop->entries.value.w[0]];
  TargetInstruction* primary =
      group->is_imm ? HoistMakeImmediate(allocator, group, reg)
                    : HoistMakeReload(allocator, group->spill, reg, group->size);
  HoistInsert(gen, first_pred, primary);
  extra_live->value.u[first_pred->block_id] |= UINT64_C(1) << reg;
  for (size_t e = 1; e < loop->entries.length; e++) {
    TargetBasicBlock* pred =
        gen->basic_blocks.value.p[loop->entries.value.w[e]];
    TargetInstruction* copy =
        group->is_imm ? HoistMakeImmediate(allocator, group, reg)
                      : HoistMakeReload(allocator, group->spill, reg, group->size);
    HoistInsert(gen, pred, copy);
    extra_live->value.u[pred->block_id] |= UINT64_C(1) << reg;
  }
  for (int m = 0; m < group->nmembers; m++) {
    TargetInstruction* old = group->members[m];
    TargetBasicBlock* block = old->block;
    // ReloadSpills points the consumer at the reload without recording the
    // use.  Retarget the recorded users, then any operand that still names
    // the old instruction.  Remove deletes a user that still references the
    // instruction, so both updates have to happen first.
    TargetRetargetInstruction(old, primary);
    for (size_t b = 0; b < gen->basic_blocks.length; b++) {
      TargetBasicBlock* scan = gen->basic_blocks.value.p[b];
      if (scan == NULL || scan->code == NULL) {
        continue;
      }
      for (TargetInstruction* inst = scan->code; inst != NULL;
           inst = HoistNextInBlock(scan, inst)) {
        bool ref = false;
        for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
          if (inst->operand[i] == old) {
            inst->operand[i] = primary;
            ref = true;
          }
        }
        if (inst->dest == old) {
          inst->dest = primary;
          ref = true;
        }
        if (ref) {
          TargetAddUser(primary, inst);
        }
        if (inst == scan->end_code) {
          break;
        }
      }
    }
    if (block != NULL) {
      TargetBasicBlockRemoveInstruction(gen, block, old);
    }
  }
}

static int HoistPickReg(const uint64_t blocked) {
  static const int kRegs[] = {10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};
  for (size_t i = 0; i < sizeof(kRegs) / sizeof(kRegs[0]); i++) {
    if ((blocked & (UINT64_C(1) << kRegs[i])) == 0) {
      return kRegs[i];
    }
  }
  return -1;
}

// True when every real reference is inside the loop and there is at least
// one.  Recorded users that no longer name the value are stale (the spill
// node keeps consumers whose operands were rewritten to a reload) and do
// not count.
static bool HoistGroupStaysInLoop(TargetGenerator* gen, HoistGroup* group,
                                  BitSet* blocks) {
  bool any = false;
  for (int m = 0; m < group->nmembers; m++) {
    TargetInstruction* value = group->members[m];
    for (size_t u = 0; u < value->users.length; u++) {
      TargetInstruction* user = value->users.value.p[u];
      if (!HoistRefsValue(user, value)) {
        continue;
      }
      if (user->block == NULL ||
          !BitSetContains(blocks, user->block->block_id)) {
        return false;
      }
      any = true;
    }
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[b];
    if (block == NULL || block->code == NULL) {
      continue;
    }
    bool in_loop = BitSetContains(blocks, block->block_id);
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = HoistNextInBlock(block, inst)) {
      for (int m = 0; m < group->nmembers; m++) {
        if (!HoistRefsValue(inst, group->members[m])) {
          continue;
        }
        if (!in_loop) {
          return false;
        }
        any = true;
      }
      if (inst == block->end_code) {
        break;
      }
    }
  }
  return any;
}

static int HoistGroupWeight(const HoistGroup* group) {
  // A reload is a memory access; a materialized immediate is one alu op.
  return group->nmembers * 8 + (group->is_imm ? 0 : 4);
}

static void HoistGroupsInLoop(AARCH64RegisterAllocator* allocator,
                              HoistLoop* loop, const Vector* live_in,
                              const Vector* live_out, Vector* extra_live,
                              BitSet* framed) {
  TargetGenerator* gen = &allocator->g->base;
  if (HoistLoopHasSideEffect(gen, &loop->blocks)) {
    return;
  }
  bool spill_entries = HoistEntriesReady(gen, loop, true, framed);
  bool imm_entries = HoistEntriesReady(gen, loop, false, framed);
  if (!spill_entries && !imm_entries) {
    return;
  }

  uint64_t used = 0;
  bool frame_stable = true;
  BitSetIterator it;
  BitSetIteratorStart(&it, &loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        uint64_t uses = 0;
        uint64_t defs = 0;
        HoistInstEffects(inst, &uses, &defs, false);
        used |= uses | defs;
        if ((defs & ((UINT64_C(1) << AARCH64_SP_REG) |
                     (UINT64_C(1) << AARCH64_FP_REG))) != 0) {
          frame_stable = false;
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }
  if (!frame_stable) {
    spill_entries = false;
  }

  int killed_off[64];
  int nkilled = 0;
  BitSetIteratorStart(&it, &loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        int off = 0;
        if ((AARCH64Opcode)inst->opcode == AARCH64_OP(spill) &&
            nkilled < 64 && HoistSpillSlot(inst, &off)) {
          killed_off[nkilled++] = off;
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }

  HoistGroup groups[32];
  int ngroups = 0;
  BitSetIteratorStart(&it, &loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        bool take = false;
        TargetInstruction* spill = NULL;
        bool is_imm = false;
        int64_t imm = 0;
        AARCH64Opcode imm_op = AARCH64_OP(mov);
        if ((AARCH64Opcode)inst->opcode == AARCH64_OP(reload) &&
            inst->dest == NULL) {
          spill = inst->operand[0];
          int off = 0;
          int reg = HoistIntReg(inst);
          bool have_slot = spill != NULL && HoistSpillSlot(spill, &off);
          bool killed = false;
          for (int k = 0; k < nkilled; k++) {
            if (killed_off[k] == off) {
              killed = true;
              break;
            }
          }
          // An orphan spill handle has no block of its own.  Store-backs in
          // the loop share its slot and are caught by `killed`.
          bool spill_outside =
              spill != NULL &&
              (spill->block == NULL ||
               !BitSetContains(&loop->blocks, spill->block->block_id));
          bool dominates = HoistDominatesLatches(gen, block, loop);
          if (spill_entries && have_slot && reg >= 0 && !killed &&
              spill_outside && dominates) {
            take = true;
          }
        } else if (imm_entries && inst->dest == NULL &&
                   inst->operand[0] != NULL &&
                   TargetIsConst(inst->operand[0]) &&
                   ((AARCH64Opcode)inst->opcode == AARCH64_OP(mov) ||
                    (AARCH64Opcode)inst->opcode == AARCH64_OP(movz) ||
                    (AARCH64Opcode)inst->opcode == AARCH64_OP(movn)) &&
                   HoistIntReg(inst) >= 0 &&
                   HoistDominatesLatches(gen, block, loop)) {
          is_imm = true;
          imm_op = (AARCH64Opcode)inst->opcode;
          imm = TargetIntValue(inst->operand[0]);
          take = true;
        }
        if (take) {
          int found = -1;
          int size = GetRegisterSize(inst);
          for (int g = 0; g < ngroups; g++) {
            if (is_imm) {
              if (groups[g].is_imm && groups[g].imm_op == imm_op &&
                  groups[g].size == size && groups[g].imm == imm) {
                found = g;
                break;
              }
            } else if (!groups[g].is_imm && groups[g].spill == spill) {
              found = g;
              break;
            }
          }
          if (found < 0 && ngroups < 32) {
            found = ngroups++;
            groups[found].spill = spill;
            groups[found].is_imm = is_imm;
            groups[found].imm_op = imm_op;
            groups[found].size = size;
            groups[found].imm = imm;
            groups[found].nmembers = 0;
          }
          if (found >= 0 && groups[found].nmembers < 48) {
            groups[found].members[groups[found].nmembers++] = inst;
          }
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }

  uint64_t blocked = used | live_in->value.u[loop->header];
  for (size_t e = 0; e < loop->entries.length; e++) {
    TargetBasicBlock* pred =
        gen->basic_blocks.value.p[loop->entries.value.w[e]];
    blocked |= HoistLiveAtInsert(pred, live_out, extra_live);
  }
  // Heaviest groups first so a scarce register covers the hottest reload.
  for (int a = 0; a < ngroups; a++) {
    for (int b = a + 1; b < ngroups; b++) {
      if (HoistGroupWeight(&groups[b]) > HoistGroupWeight(&groups[a])) {
        HoistGroup tmp = groups[a];
        groups[a] = groups[b];
        groups[b] = tmp;
      }
    }
  }
  for (int g = 0; g < ngroups; g++) {
    if (!HoistGroupStaysInLoop(gen, &groups[g], &loop->blocks)) {
      continue;
    }
    int reg = HoistPickReg(blocked);
    if (reg < 0) {
      break;
    }
    HoistOneGroup(allocator, loop, &groups[g], reg, extra_live);
    blocked |= UINT64_C(1) << reg;
  }
}

static int HoistLoopDeeper(const void* lhs, const void* rhs) {
  const HoistLoop* a = *(HoistLoop* const*)lhs;
  const HoistLoop* b = *(HoistLoop* const*)rhs;
  if (b->depth != a->depth) {
    return b->depth - a->depth;
  }
  return a->header - b->header;
}

static void HoistLoopInvariantValues(AARCH64RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  size_t nblocks = gen->basic_blocks.length;
  if (!OptLevel2() || gen->entry_block == NULL || nblocks == 0) {
    return;
  }

  Vector live_in;
  Vector live_out;
  Vector extra_live;
  VectorInit(&live_in);
  VectorInit(&live_out);
  VectorInit(&extra_live);
  VectorResize(&live_in, nblocks);
  VectorResize(&live_out, nblocks);
  VectorResize(&extra_live, nblocks);

  // Live sets only grow, so a full reverse sweep converges.  Poison the
  // result if it does not: hoisting then sees every register as live and
  // leaves the function unchanged.
  int guard = 0;
  int guard_max = (int)nblocks * 33 + 2;
  bool grew = true;
  while (grew && guard++ < guard_max) {
    grew = false;
    for (size_t id = nblocks; id-- > 0;) {
      TargetBasicBlock* block = gen->basic_blocks.value.p[id];
      uint64_t out = 0;
      if (block != NULL) {
        for (size_t e = 0; e < block->out_edges.length; e++) {
          size_t succ = block->out_edges.value.w[e];
          if (succ < nblocks) {
            out |= live_in.value.u[succ];
          }
        }
      }
      live_out.value.u[id] = out;
      uint64_t cur = out;
      if (block != NULL && block->code != NULL && block->end_code != NULL) {
        for (TargetInstruction* inst = block->end_code; inst != NULL;
             inst = TargetPrev(inst)) {
          uint64_t uses = 0;
          uint64_t defs = 0;
          HoistInstEffects(inst, &uses, &defs, false);
          cur = (cur & ~defs) | uses;
          if (inst == block->code) {
            break;
          }
        }
      }
      if (cur != live_in.value.u[id]) {
        live_in.value.u[id] = cur;
        grew = true;
      }
    }
  }
  if (grew) {
    for (size_t i = 0; i < nblocks; i++) {
      live_in.value.u[i] = ~UINT64_C(0);
      live_out.value.u[i] = ~UINT64_C(0);
    }
  }

  Vector loops;
  VectorInit(&loops);
  Vector work;
  VectorInit(&work);
  for (size_t header_id = 0; header_id < nblocks; header_id++) {
    TargetBasicBlock* header = gen->basic_blocks.value.p[header_id];
    if (header == NULL || header->is_unreachable) {
      continue;
    }
    int nlatch = 0;
    for (size_t i = 0; i < header->in_edges.length; i++) {
      size_t pred = header->in_edges.value.w[i];
      TargetBasicBlock* latch = gen->basic_blocks.value.p[pred];
      if (latch != NULL &&
          BitSetContains(&latch->dominators, header_id)) {
        nlatch++;
      }
    }
    if (nlatch == 0 || loops.length >= nblocks) {
      continue;
    }
    HoistLoop* loop = calloc(1, sizeof(HoistLoop));
    if (loop == NULL) {
      break;
    }
    BitSetInit(&loop->blocks);
    VectorInit(&loop->latches);
    VectorInit(&loop->entries);
    BitSetInsert(&loop->blocks, header_id);
    VectorClear(&work);
    for (size_t i = 0; i < header->in_edges.length; i++) {
      size_t pred = header->in_edges.value.w[i];
      TargetBasicBlock* latch = gen->basic_blocks.value.p[pred];
      if (latch != NULL &&
          BitSetContains(&latch->dominators, header_id) &&
          !BitSetContains(&loop->blocks, pred)) {
        BitSetInsert(&loop->blocks, pred);
        VectorAppend(&work, (void*)(uintptr_t)pred);
      }
    }
    while (work.length > 0) {
      size_t id = (size_t)work.value.u[--work.length];
      TargetBasicBlock* block = gen->basic_blocks.value.p[id];
      if (block == NULL) {
        continue;
      }
      for (size_t i = 0; i < block->in_edges.length; i++) {
        size_t pred = block->in_edges.value.w[i];
        if (!BitSetContains(&loop->blocks, pred)) {
          BitSetInsert(&loop->blocks, pred);
          VectorAppend(&work, (void*)(uintptr_t)pred);
        }
      }
    }
    loop->header = (int)header_id;
    loop->depth = header->loop_nesting;
    for (size_t i = 0; i < header->in_edges.length; i++) {
      size_t pred = header->in_edges.value.w[i];
      if (BitSetContains(&loop->blocks, pred)) {
        VectorAppend(&loop->latches, (void*)(intptr_t)pred);
      } else {
        VectorAppend(&loop->entries, (void*)(intptr_t)pred);
      }
    }
    VectorAppend(&loops, loop);
  }
  VectorDestruct(&work);
  if (loops.length > 0) {
    VectorSortPointers(&loops, HoistLoopDeeper);
    BitSet framed;
    HoistFramedAtEnd(gen, &framed);
    for (size_t i = 0; i < loops.length; i++) {
      HoistGroupsInLoop(allocator, loops.value.p[i], &live_in, &live_out,
                        &extra_live, &framed);
    }
    BitSetDestruct(&framed);
  }
  VectorDestructWithContents(&loops, HoistLoopDestruct, /*free_element=*/true);
  VectorDestruct(&live_in);
  VectorDestruct(&live_out);
  VectorDestruct(&extra_live);
}

// `mov xD, xS` left by allocation.  Forward xS to same-block users when
// nothing in between overwrites either register.  32-bit moves zero the
// upper half and are left alone, as are moves that also write a dest, whose
// readers are not among the users.
static bool FoldPhysicalCopy(TargetGenerator* gen, TargetBasicBlock* block,
                             TargetInstruction* mov) {
  AARCH64Opcode op = (AARCH64Opcode)mov->opcode;
  if ((op != AARCH64_OP(mov) && op != AARCH64_OP(mv)) ||
      mov->operand[1] != NULL || mov->operand[0] == NULL ||
      mov->dest != NULL || TargetIsConst(mov->operand[0]) ||
      mov->users.length == 0 || mov->block != block) {
    return false;
  }
  int size = GetRegisterSize(mov);
  if (size != 0 && size != kSize64Bit) {
    return false;
  }
  int dst = HoistIntReg(mov);
  int src = HoistIntReg(mov->operand[0]);
  if (dst < 0 || src < 0 || dst > 30 || src > 30) {
    return false;
  }
  // Argument and scratch registers are read implicitly by calls and by
  // spill addressing.  Forwarding through them drops that write.
  if (dst <= 8 || dst == AARCH64_IP1_REG || dst == AARCH64_IP2_REG ||
      src == AARCH64_IP1_REG || src == AARCH64_IP2_REG) {
    return false;
  }
  uint64_t dst_bit = UINT64_C(1) << dst;
  uint64_t src_bit = UINT64_C(1) << src;
  size_t nusers = mov->users.length;
  size_t seen = 0;
  for (TargetInstruction* inst = HoistNextInBlock(block, mov); inst != NULL;
       inst = HoistNextInBlock(block, inst)) {
    bool is_user = false;
    for (size_t u = 0; u < nusers; u++) {
      TargetInstruction* user = mov->users.value.p[u];
      if (user == inst) {
        is_user = true;
        break;
      }
      if (user == NULL || user->block != block || user->dest == mov) {
        return false;
      }
    }
    uint64_t uses = 0;
    uint64_t defs = 0;
    HoistInstEffects(inst, &uses, &defs, true);
    if (!is_user) {
      if ((uses & dst_bit) != 0 || (defs & dst_bit) != 0 ||
          (defs & src_bit) != 0) {
        return false;
      }
    } else {
      seen++;
      if (((defs & dst_bit) != 0 || (defs & src_bit) != 0) && seen < nusers) {
        return false;
      }
      if (seen == nusers) {
        break;
      }
    }
    if (inst == block->end_code) {
      break;
    }
  }
  if (seen != nusers) {
    return false;
  }
  TargetInstruction* source = mov->operand[0];
  TargetRetargetInstruction(mov, source);
  TargetBasicBlockRemoveInstruction(gen, block, mov);
  return true;
}

static void FoldPhysicalCopies(AARCH64RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  if (!OptLevel2() || gen->entry_block == NULL) {
    return;
  }
  bool changed = true;
  for (int pass = 0; changed && pass < 8; pass++) {
    changed = false;
    for (size_t b = 0; b < gen->basic_blocks.length; b++) {
      TargetBasicBlock* block = gen->basic_blocks.value.p[b];
      if (block == NULL || block->code == NULL) {
        continue;
      }
      bool folded = true;
      while (folded) {
        folded = false;
        for (TargetInstruction* inst = block->code; inst != NULL;
             inst = HoistNextInBlock(block, inst)) {
          if (FoldPhysicalCopy(gen, block, inst)) {
            folded = true;
            changed = true;
            break;
          }
          if (inst == block->end_code) {
            break;
          }
        }
      }
    }
  }
}
// Shrink wrapping, hoisting and copy folding work on the physical registers
// coloring assigned.
void AARCH64AllocateRegisters(AARCH64RegisterAllocator* allocator) {
  AARCH64AllocateRegistersByColoring(allocator);
  AARCH64ShrinkWrap(allocator);
  HoistLoopInvariantValues(allocator);
  FoldPhysicalCopies(allocator);
  AARCH64SimplifyBranchesPostRA(allocator->g);
}


const char* AARCH64RegisterName(AARCH64Register* reg, int size, char* buf, size_t len) {
  return AARCH64RegisterNameFromNum(reg->base.num, reg->type, size, buf, len);
}

const char* AARCH64RegisterNameFromNum(int num, AARCH64RegisterType type, int size,
                                       char* buf, size_t len) {
  switch (type) {
    case kAARCH64RegTypeInt:
      if (num == AARCH64_FP_REG) {
        snprintf(buf, len, "x29");
        break;
      }
      if (num == AARCH64_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (num == AARCH64_LR_REG) {
        snprintf(buf, len, "x30");
        break;
      }
      if (num == AARCH64_INT_ZERO_REG) {
        snprintf(buf, len, "%s", size == kSize32Bit ? "wzr" : "xzr");
        break;
      }

      // Honor the requested operand width: a 32-bit operation must use the
      // "w" register so that, in particular, stores write 4 bytes (str w<n>)
      // rather than 8 (str x<n>), which would clobber the adjacent stack slot.
      // An unspecified size (0) keeps the historical 64-bit "x" name.
      if (size == kSize32Bit) {
        snprintf(buf, len, "w%d", num);
      } else {
        snprintf(buf, len, "x%d", num);
      }
      return buf;

    case kAARCH64RegTypeFloat:
      // Honor the requested operand width: a 32-bit (single precision) value
      // must use the "s" register so that, in particular, stores write 4 bytes
      // (fstr s<n>) rather than 8 (fstr d<n>), which would clobber the adjacent
      // stack slot, and arithmetic rounds at single precision.  An unspecified
      // size (0) keeps the 64-bit "d" name.
      if (size == kSize32Bit) {
        snprintf(buf, len, "s%d", num);
      } else if (size == kSize128Bit) {
        snprintf(buf, len, "q%d", num);
      } else {
        snprintf(buf, len, "d%d", num);
      }
      return buf;
  }
  return buf;
}
