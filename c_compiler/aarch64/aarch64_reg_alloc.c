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
#include "target_basic_block.h"
#include "compiler.h"

static void AllocateRegister(AARCH64RegisterAllocator* allocator,
                             TargetInstruction* inst);
static AARCH64Register* ScratchRegister(AARCH64RegisterAllocator* allocator,
                                        AARCH64RegisterType type);
static AARCH64RegisterType RegisterTypeFromInstruction(TargetInstruction* inst);

static const char* AARCH64RegisterNameFromNum1(int num, AARCH64RegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                           size_t len);

static const char* AARCH64RegisterNameNoSize(AARCH64Register* reg, int size, char* buf, size_t len);

static void Trap() {}

static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 16) {
    Trap();
  }
}

static void TrapSpill(TargetInstruction* inst) {
  if (inst->id == 63) {
    Trap();
  }
}

static void TrapReload(TargetInstruction* inst) {
  if (inst->id == 781) {
    Trap();
  }
}

static void TrapBlock(TargetBasicBlock* block) {
  if (block->block_id == 82) {
    Trap();
  }
}

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

  // Reserve some registers.
  allocator->int_regs[AARCH64_FP_REG].base.reserved = true;
  allocator->int_regs[AARCH64_LR_REG].base.reserved = true;
  allocator->int_regs[AARCH64_SPILL_ADDR].base.reserved = true;
  // The second intra-procedure-call register, which a value on its way to a
  // spill slot passes through.  A spill store addresses the slot through x16,
  // so the value cannot go there.
  allocator->int_regs[AARCH64_IP2_REG].base.reserved = true;
  allocator->int_regs[AARCH64_INT_ZERO_REG].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
  BitSetInit(&allocator->short_lived_varregs);
  BitSetInit(&allocator->shared_varregs);
  MapInitForPointerKeys(&allocator->reassignable_spills);
  MapInitForPointerKeys(&allocator->varreg_values);
  allocator->shrink_wrapped_regs = 0;
  allocator->spill_after_definition = false;
  allocator->allocating_depth = 0;
}

AARCH64RegisterAllocator* NewAARCH64RegisterAllocator(struct AARCH64Generator* g) {
  AARCH64RegisterAllocator* reg_alloc = malloc(sizeof(AARCH64RegisterAllocator));
  AARCH64RegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void AARCH64RegisterAllocatorDestruct(AARCH64RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
  BitSetDestruct(&allocator->short_lived_varregs);
  BitSetDestruct(&allocator->shared_varregs);
  MapDestruct(&allocator->reassignable_spills);
  MapDestruct(&allocator->varreg_values);
}

void AARCH64RegisterAllocatorDelete(AARCH64RegisterAllocator* alloc) {
  AARCH64RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// Size for registers.
#define kSize32Bit 1
#define kSize64Bit 2

// The aarch64 ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  AARCH64RegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  bool temp;
  bool arg;             // Argument registers.
} register_ranges[] = {
  // General temporaries prefer non-argument registers.  Argument registers
  // (x0..x7 / d0..d7) are searched last so a long-lived temporary does not
  // grab one that is about to be needed for an outgoing call's register
  // argument (which would clobber the temporary when the argument is set up,
  // or vice versa).  x9 is intentionally excluded from the general pool: it is
  // the dedicated scratch used to stage indirect-call targets across argument
  // setup.
  {kAARCH64RegTypeInt, AARCH64_INT_TEMP_START + 1, AARCH64_INT_TEMP_END, true,
   false},
  {kAARCH64RegTypeInt, AARCH64_INT_SAVED_START, AARCH64_INT_SAVED_END, false,
   false},
  {kAARCH64RegTypeInt, AARCH64_INT_ARG_START, AARCH64_INT_ARG_END, true, true},
    {kAARCH64RegTypeFloat, AARCH64_FP_TEMP_START, AARCH64_FP_TEMP_END, true,
     false},
    {kAARCH64RegTypeFloat, AARCH64_FP_SAVED_START, AARCH64_FP_SAVED_END, false,
     false},
    {kAARCH64RegTypeFloat, AARCH64_FP_ARG_START, AARCH64_FP_ARG_END, true,
     true},
};

// Which registers a value may be given.  A value live across a call needs a
// callee-saved register; a staged outgoing argument (AARCH64_INST_AVOID_ARG_REGS)
// may use any register the argument moves will not overwrite.
typedef enum {
  kRegPoolSaved,
  kRegPoolAny,
  kRegPoolNoArgs,
} AARCH64RegPool;

static bool PoolExcludesRange(AARCH64RegPool pool, size_t range) {
  return (pool == kRegPoolSaved && register_ranges[range].temp) ||
         (pool == kRegPoolNoArgs && register_ranges[range].arg);
}

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(AARCH64RegisterAllocator* allocator) {
  char buf[32];
  for (AARCH64RegisterType type = kAARCH64RegTypeInt; type <= kAARCH64RegTypeFloat; type++) {
    AARCH64Register* regs =
        type == kAARCH64RegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(x%d): free\n", AARCH64RegisterNameNoSize(&regs[j], 0, buf, sizeof(buf)), regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(x%d): owner: @%d %s\n", AARCH64RegisterNameNoSize(&regs[j], 0, buf, sizeof(buf)), regs[j].base.num, owner->id,
                   AARCH64OpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(AARCH64Register* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static bool BlockHasFixedArgOrRestore(TargetBasicBlock* block) {
  if (block == NULL || block->code == NULL) {
    return true;
  }
  for (TargetInstruction* p = block->code; p != NULL; p = TargetNext(p)) {
    AARCH64Opcode op = (AARCH64Opcode)p->opcode;
    if (op == AARCH64_OP(restore) || op == AARCH64_OP(bl) ||
        op == AARCH64_OP(blr) || op == AARCH64_OP(regarg) ||
        (op >= AARCH64_OP(r0) && op <= AARCH64_OP(r7)) ||
        (op >= AARCH64_OP(d0) && op <= AARCH64_OP(d7))) {
      return true;
    }
    if (p->dest != NULL) {
      AARCH64Opcode dop = (AARCH64Opcode)p->dest->opcode;
      if ((dop >= AARCH64_OP(r0) && dop <= AARCH64_OP(r7)) ||
          (dop >= AARCH64_OP(d0) && dop <= AARCH64_OP(d7))) {
        return true;
      }
    }
    if (p == block->end_code) {
      break;
    }
  }
  return false;
}

static bool RegisterHasOtherLiveHolder(
    AARCH64RegisterAllocator* allocator, TargetRegister* reg,
    TargetInstruction* keep1, TargetInstruction* keep2,
    TargetInstruction* keep3);

static bool FunctionHasAnyFixedArgOrTailCall(TargetGenerator* gen) {
  int restore_count = 0;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      AARCH64Opcode op = (AARCH64Opcode)p->opcode;
      if (op == AARCH64_OP(regarg) || op == AARCH64_OP(br)) {
        return true;
      }
      if (op == AARCH64_OP(b) && p->operand[1] != NULL &&
          p->operand[1]->opcode == (TargetOpcode)AARCH64_OP(symbol)) {
        return true;
      }
      if (op == AARCH64_OP(restore) && ++restore_count > 1) {
        return true;
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  return false;
}

static AARCH64Register* FindFreeRegisterForInst(
    AARCH64RegisterAllocator* allocator, TargetInstruction* inst,
    AARCH64RegisterType type, AARCH64RegPool pool) {
  AARCH64Register* regs =
      type == kAARCH64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  bool leaf_prefer_args =
      OptLevel1() && pool == kRegPoolAny &&
      allocator->g->base.num_calls == 0 &&
      !allocator->g->not_leaf && !allocator->g->base.varargs &&
      !allocator->g->moves_sp && allocator->g->saved_regs.length == 0 &&
      !FunctionHasAnyFixedArgOrTailCall(&allocator->g->base);
  bool local_callfree_prefer_args =
      OptLevel1() && !leaf_prefer_args && pool == kRegPoolAny && inst != NULL &&
      !AARCH64IsVarRegister(inst) && inst->dest == NULL &&
      inst->block != NULL && !inst->block->contains_call &&
      !BitSetContains(&inst->block->output_ids, inst->id) &&
      !BlockHasFixedArgOrRestore(inst->block);
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      if (PoolExcludesRange(pool, i)) {
        continue;
      }
      // In a true leaf function, or for a purely block-local temporary inside
      // a call-free block, use free caller-saved argument registers x7..x1 /
      // d7..d1 (skipping x0/d0 so return-value assembly never collides) before
      // touching callee-saved x19..x28 / d8..d15.
      if ((leaf_prefer_args || local_callfree_prefer_args) &&
          !register_ranges[i].temp) {
        int arg_start = type == kAARCH64RegTypeInt ? AARCH64_INT_ARG_START + 1
                                                   : AARCH64_FP_ARG_START + 1;
        int arg_end = type == kAARCH64RegTypeInt ? AARCH64_INT_ARG_END
                                                 : AARCH64_FP_ARG_END;
        for (int j = arg_end; j >= arg_start; j--) {
          if (!regs[j].base.reserved && regs[j].base.owner == NULL &&
              (!local_callfree_prefer_args ||
               !RegisterHasOtherLiveHolder(allocator, &regs[j].base, inst, NULL,
                                           NULL))) {
            return &regs[j];
          }
        }
      }
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner == NULL) {
          return &regs[j];
        }
      }
    }
  }

  // No registers available.
  return NULL;
}

static COMPILER_UNUSED AARCH64Register* FindFreeRegister(
    AARCH64RegisterAllocator* allocator, AARCH64RegisterType type,
    AARCH64RegPool pool) {
  return FindFreeRegisterForInst(allocator, NULL, type, pool);
}

static void FreeRegister(AARCH64RegisterAllocator* allocator, AARCH64Register* reg) {
  reg->base.owner = NULL;
}

static bool IsShortLivedVarReg(AARCH64RegisterAllocator* allocator,
                               TargetInstruction* inst) {
  return AARCH64IsVarRegister(inst) &&
         BitSetContains(&allocator->short_lived_varregs, inst->id);
}


// A value written straight into a variable register shares that register with
// the variable, and the variable's live range is not what the use counter
// tracks.  Handing the register on when the value's last read is done therefore
// takes it away from the variable as well.
static bool SharesRegisterWithVariable(AARCH64RegisterAllocator* allocator,
                                       TargetInstruction* inst) {
  return inst->dest != NULL && AARCH64IsVarRegister(inst->dest) &&
         !IsShortLivedVarReg(allocator, inst->dest);
}

// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(AARCH64RegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      // The use counter is the number of distinct user instructions (the users
      // list is deduplicated), so an instruction that references the same value
      // in several operand slots must only decrement it once.  Skip an operand
      // already seen in an earlier slot; otherwise the count underflows early
      // and the value's register is freed while still live.
      bool duplicate = false;
      for (size_t j = 0; j < i; j++) {
        if (inst->operand[j] == op) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        continue;
      }
      if (AARCH64IsFixedRegister(op)) {
        continue;
      }
      // A variable register is a dedicated callee-saved register pinned to a C
      // variable for its entire live range (which may span loop back edges that
      // the static use count cannot model).  It must not be freed by the use
      // counter, or it could be reassigned and clobber the variable.
      if (AARCH64IsVarRegister(op) &&
          !IsShortLivedVarReg(allocator, op)) {
        continue;
      }
      if (SharesRegisterWithVariable(allocator, op)) {
        continue;
      }
      // A live-out value is read in a successor (including a loop back edge).
      // The static use count hits zero after the last read in *this* block,
      // which is not its last read.  Freeing the register here lets a later
      // temp in the same block reuse it and clobber the successor's incoming
      // copy (path::lexically_normal at -O2 reused the `this` pointer's
      // register for a local string and then loaded size from that string).
      if (!IsShortLivedVarReg(allocator, op) && inst->block != NULL &&
          BitSetContains(&inst->block->output_ids, op->id)) {
        continue;
      }
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL && op->uses > 0) {
        op->uses--;
        assert(op->uses >= 0);
        if (op->uses == 0) {
          if (reg->owner == op) {
            FreeRegister(allocator, (AARCH64Register*)reg);
          }
        }
      }
    }
  }
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(AARCH64Register* reg) {
  int num = reg->base.num;
  switch (reg->type) {
    case kAARCH64RegTypeInt:
      return num >= AARCH64_INT_SAVED_START &&
             num <= AARCH64_INT_SAVED_END;
    case kAARCH64RegTypeFloat:
      return num >= AARCH64_FP_SAVED_START &&
             num <= AARCH64_FP_SAVED_END;
  }
  return false;
}

// Calculate the cost of spilling the instruction: the number of uses left
// plus, for each user that has not yet got a register, a reload weighted by
// 8 to the power of the loop nesting of the user's block.  Each user is
// weighted by its own nesting only; compounding along the dominator chain
// saturates every value in a large loop body and makes the choice arbitrary.
static int SpillCost(TargetInstruction* inst) {
  int cost = inst->uses;
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* user = inst->users.value.p[i];
    if ((user->flags & TARGET_INST_PROCESSED) == 0) {
      int nesting = user->block != NULL ? user->block->loop_nesting : 0;
      int shift = nesting < 8 ? 3 * nesting : 24;
      cost += 1 << shift;
      if (cost > INT_MAX / 4) {
        return INT_MAX / 4;
      }
    }
  }
  return cost;
}

// Outgoing-argument and other fixed-register pseudos must never be chosen as
// spill victims: their physical register is dictated by the ABI and is needed
// at the imminent call.  Spilling one (e.g. because its use-count happens to be
// the cheapest) would free the argument register for an unrelated temporary,
// clobbering an argument that was already set up.
static bool IsUnspillableFixedReg(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
    case AARCH64_OP(xr):
    case AARCH64_OP(structreturn):
      return true;
    default:
      return false;
  }
}

// Spilling leaves already-processed reads of the value pointing at the physical
// register rather than the slot, which only works while every such read happens
// before the register is handed to something else.  Down a straight line it
// does, because the allocator processes blocks in dominator order; around a
// loop it does not, so a read in any block control can come back to is a read
// of whatever the new owner left there.  |reentered| holds those blocks.
static bool IsUnsafeSpillVictim(TargetInstruction* owner, BitSet* reentered) {
  for (size_t i = 0; i < owner->users.length; i++) {
    TargetInstruction* user = owner->users.value.p[i];
    if ((user->flags & TARGET_INST_PROCESSED) != 0 && user->block != NULL &&
        BitSetContains(reentered, user->block->block_id)) {
      return true;
    }
  }
  return false;
}

static TargetInstruction* InstructionRegisterHolder(TargetInstruction* inst);
static bool BlockIsLoopHeader(TargetGenerator* gen, TargetBasicBlock* block);
static AARCH64Register* ScratchRegister(AARCH64RegisterAllocator* allocator,
                                        AARCH64RegisterType type);
static AARCH64RegisterType RegisterTypeFromInstruction(TargetInstruction* inst);

// RepairProcessedRead stages a spilled value through the one scratch register,
// so it cannot fix a read that already stages a different spilled value that
// way.  Taking the register of a value with such a read leaves the read naming
// whatever is put in the register next.
static bool HasUnrepairableProcessedRead(AARCH64RegisterAllocator* allocator,
                                         TargetInstruction* owner) {
  AARCH64Register* scratch =
      ScratchRegister(allocator, RegisterTypeFromInstruction(owner));
  for (size_t i = 0; i < owner->users.length; i++) {
    TargetInstruction* user = owner->users.value.p[i];
    if ((user->flags & TARGET_INST_PROCESSED) == 0 ||
        (int)user->opcode == (int)AARCH64_OP(spill) ||
        (int)user->opcode == (int)AARCH64_OP(reload)) {
      continue;
    }
    if (scratch == NULL) {
      return true;
    }
    for (size_t op = 0; op < TARGET_MAX_OPERANDS; op++) {
      TargetInstruction* other = user->operand[op];
      if (other != NULL && other != owner && other->reg == &scratch->base) {
        return true;
      }
    }
  }
  return false;
}

// Copies of the same pointer can share one physical register while only one
// of them is the owner.  A processed read in a block we can reenter may name
// the other copy, so spilling the owner still leaves that read pointing at
// whatever overwrites the register (path::lexically_normal at -O2).
static bool RegisterNamedByProcessedRead(TargetGenerator* gen,
                                         TargetRegister* reg,
                                         BitSet* reentered) {
  if (reg == NULL) {
    return false;
  }
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    if (block == NULL || !BitSetContains(reentered, block->block_id) ||
        TargetBasicBlockIsEmpty(block) || !BlockIsLoopHeader(gen, block)) {
      continue;
    }
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = inst == block->end_code ? NULL : TargetNext(inst)) {
      if ((inst->flags & TARGET_INST_PROCESSED) == 0) {
        continue;
      }
      for (size_t op = 0; op < TARGET_MAX_OPERANDS; op++) {
        TargetInstruction* holder = InstructionRegisterHolder(inst->operand[op]);
        if (holder != NULL && holder->reg == reg) {
          return true;
        }
      }
    }
  }
  return false;
}

// Returns the cheapest value whose register can be taken without breaking a
// read that has already been handed that register, or NULL when there is none.
// |unsafe|, when not NULL, receives the cheapest value that could be taken if
// there were no such reads, for a caller that has no way to proceed without
// taking a register from something.
static TargetInstruction* FindSpillVictim(AARCH64RegisterAllocator* allocator,
                                          AARCH64RegisterType type,
                                          AARCH64RegPool pool,
                                          TargetBasicBlock* block,
                                          TargetInstruction** unsafe) {
  AARCH64Register* regs =
      type == kAARCH64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  BitSet reentered;
  BitSetInit(&reentered);
  TargetBasicBlockReachableAfter(&allocator->g->base, block, &reentered);
  int unsafe_cost = INT_MAX;
  TargetInstruction* unsafe_victim = NULL;
  int unrepairable_cost = INT_MAX;
  TargetInstruction* unrepairable_victim = NULL;
  int min_cost = INT_MAX;
  TargetInstruction* victim = NULL;
  int min_var_cost = INT_MAX;
  TargetInstruction* var_victim = NULL;
  // Find the instruction with the lowest spill cost.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      if (PoolExcludesRange(pool, i)) {
        continue;
      }
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner != NULL) {
          TargetInstruction* owner = regs[j].base.owner;
          if ((owner->flags & TARGET_INST_SPILLED) != 0 ||
              ((int)owner->opcode == (int)AARCH64_OP(spill)) ||
              ((int)owner->opcode == (int)AARCH64_OP(reload))) {
            regs[j].base.owner = NULL;
            continue;
          }
          if (owner->reg != &regs[j].base || IsUnspillableFixedReg(owner)) {
            continue;
          }
          if (AARCH64IsVarRegister(owner) && owner->users.length == 0) {
            // Reads of a dest-shared value are recorded on the defining add
            // rather than the variable, so users.length == 0 does not mean
            // write-only.  Releasing a non-short-lived varreg here is what
            // handed `this`, `&part`, and a local bool slot the same
            // physical register in path::lexically_normal.
            if (!IsShortLivedVarReg(allocator, owner)) {
              continue;
            }
            regs[j].base.owner = NULL;
            continue;
          }
          int cost = SpillCost(owner);
          // Writing a variable register costs nothing when the value being
          // stored can simply be computed into it, but then the register holds
          // two things at once as far as this is concerned: the variable, whose
          // reads spilling redirects to the slot, and the value, whose reads
          // name the register and stay behind.
          bool shares_register_with_a_value =
              BitSetContains(&allocator->shared_varregs, owner->id);
          if (IsUnsafeSpillVictim(owner, &reentered) ||
              RegisterNamedByProcessedRead(&allocator->g->base, owner->reg,
                                           &reentered) ||
              shares_register_with_a_value) {
            if (HasUnrepairableProcessedRead(allocator, owner)) {
              if (cost < unrepairable_cost) {
                unrepairable_cost = cost;
                unrepairable_victim = owner;
              }
              continue;
            }
            if (cost < unsafe_cost) {
              unsafe_cost = cost;
              unsafe_victim = owner;
            }
            continue;
          }
          if (AARCH64IsVarRegister(owner)) {
            if (cost < min_var_cost) {
              min_var_cost = cost;
              var_victim = owner;
            }
            continue;
          }
          if (cost < min_cost) {
            min_cost = cost;
            victim = owner;
          }
        }
      }
    }
  }
  BitSetDestruct(&reentered);
  if (victim == NULL) {
    victim = var_victim;
  }
  if (unsafe_victim == NULL) {
    unsafe_victim = unrepairable_victim;
  }
  if (unsafe != NULL) {
    *unsafe = unsafe_victim;
    return victim;
  }
  // The caller cannot deal with there being nothing safe to take, so give it
  // the cheapest unsafe candidate as this always did.
  return victim != NULL ? victim : unsafe_victim;
}

// True if |inst|'s own allocation is in progress.  Its operands have already
// had reloads inserted for any of them that were spilled, so pointing one of
// them at a spill slot now would do nothing but leave it naming the physical
// register: the reload pass for this instruction has been and gone.
static bool IsBeingAllocated(AARCH64RegisterAllocator* allocator,
                             TargetInstruction* inst) {
  for (size_t d = 0; d < allocator->allocating_depth; d++) {
    if (allocator->allocating[d] == inst) {
      return true;
    }
  }
  return false;
}

static bool NotProcessed(TargetInstruction* inst, void* data) {
  AARCH64RegisterAllocator* allocator = data;
  return (inst->flags & TARGET_INST_PROCESSED) == 0 &&
         !IsBeingAllocated(allocator, inst);
}

static bool IsRematerializableAddSubShape(AARCH64RegisterAllocator* allocator,
                                          TargetInstruction* inst);

// A read that has already been allocated names the physical register the value
// was in, and spilling hands that register to something else.  Where the read
// runs before the handover that is fine, but the allocator walks blocks in
// dominator order, not layout order, so it is not always so.  Stage the slot
// through the scratch register immediately before the read instead.  Returns
// false when the read cannot be repaired, which leaves it as it was.
static bool RepairProcessedRead(AARCH64RegisterAllocator* allocator,
                                TargetInstruction* user,
                                TargetInstruction* value,
                                TargetInstruction* spill) {
  if (user->block == NULL || value->reg == NULL) {
    return false;
  }
  if ((int)user->opcode == (int)AARCH64_OP(spill) ||
      (int)user->opcode == (int)AARCH64_OP(reload)) {
    // Spill stores and reloads name their slot's register directly rather than
    // reading an operand's, so there is nothing to redirect.
    return true;
  }
  bool reads_value = false;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (user->operand[i] == value) {
      reads_value = true;
    }
  }
  if (!reads_value) {
    // A duplicate entry in the user list, already dealt with.
    return true;
  }
  AARCH64Register* scratch = ScratchRegister(
      allocator, RegisterTypeFromInstruction(value));
  if (scratch == NULL) {
    return false;
  }
  // There is one scratch register, so a read that already stages another slot
  // through it cannot stage this one too.
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = user->operand[i];
    if (op != NULL && op != value && op->reg == &scratch->base) {
      return false;
    }
  }
  TargetInstruction* reload;
  if (IsRematerializableAddSubShape(allocator, value) &&
      (value->operand[0]->flags & TARGET_INST_SPILLED) == 0) {
    reload = TargetNewInstruction2(value->opcode, value->operand[0],
                                   value->operand[1]);
    reload->flags =
        (reload->flags & ~(3 << 16)) | (value->flags & (3 << 16));
  } else {
    reload = TargetNewInstruction1((TargetOpcode)AARCH64_OP(reload), spill);
    TrapReload(reload);
  }
  TargetBasicBlockEmitBefore(&allocator->g->base, user->block, reload, user);
  reload->reg = &scratch->base;
  reload->flags |= TARGET_INST_PROCESSED;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (user->operand[i] == value) {
      user->operand[i] = reload;
    }
  }
  TargetAddUser(reload, user);
  // This reload is for a single instruction, however many of its operands read
  // it.
  reload->uses = 1;
  return true;
}

static bool IsReassignableDefinition(TargetInstruction* inst,
                                     TargetInstruction* target) {
  return inst->dest == target;
}

static bool InstructionHasExternalDefs(
    AARCH64RegisterAllocator* allocator, TargetInstruction* target) {
  TargetGenerator* gen = &allocator->g->base;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if (inst != target && IsReassignableDefinition(inst, target)) {
        return true;
      }
      if (is_end) {
        break;
      }
    }
  }
  return false;
}

static void InsertReassignableStoreBacks(
    AARCH64RegisterAllocator* allocator, TargetInstruction* target,
    TargetInstruction* spill) {
  TargetGenerator* gen = &allocator->g->base;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if ((inst->flags & TARGET_INST_PROCESSED) != 0 && inst->reg != NULL &&
          IsReassignableDefinition(inst, target)) {
        TargetInstruction* store = TargetNewInstruction2(
            (TargetOpcode)AARCH64_OP(spill), target, spill->operand[1]);
        store->reg = inst->reg;
        store->flags |= TARGET_INST_PROCESSED;
        TargetBasicBlockEmitAfter(gen, block, store, inst);
        inst = store;
      }
      if (is_end) {
        break;
      }
    }
  }
}

static void SyncReassignableSpill(AARCH64RegisterAllocator* allocator,
                                  TargetInstruction* definition,
                                  TargetInstruction* target) {
  if (target == NULL || definition->reg == NULL) {
    return;
  }
  TargetInstruction* spill =
      MapFindPointerKey(&allocator->reassignable_spills, target);
  if (spill == NULL) {
    return;
  }
  TargetInstruction* store = TargetNewInstruction2(
      (TargetOpcode)AARCH64_OP(spill), target, spill->operand[1]);
  store->reg = definition->reg;
  store->flags |= TARGET_INST_PROCESSED;
  TargetBasicBlockEmitAfter(&allocator->g->base, definition->block, store,
                            definition);
}

static bool IsRematerializableAddSubShape(AARCH64RegisterAllocator* allocator,
                                          TargetInstruction* inst) {
  if (inst == NULL || inst->dest != NULL ||
      AARCH64IsVarRegister(inst) ||
      InstructionHasExternalDefs(allocator, inst)) {
    return false;
  }
  if (inst->opcode != (TargetOpcode)AARCH64_OP(add) &&
      inst->opcode != (TargetOpcode)AARCH64_OP(sub)) {
    return false;
  }
  TargetInstruction* base = inst->operand[0];
  TargetInstruction* imm = inst->operand[1];
  if (base == NULL || imm == NULL || inst->operand[2] != NULL ||
      !TargetIsConst(imm)) {
    return false;
  }
  int64_t val = TargetIntValue(imm);
  if (val == INT64_MIN) {
    return false;
  }
  uint64_t mag = (uint64_t)(val < 0 ? -val : val);
  if (!(mag <= 0xfff || ((mag & 0xfff) == 0 && mag <= 0xfff000))) {
    return false;
  }
  if (base->opcode == (TargetOpcode)AARCH64_OP(fp)) {
    return true;
  }
  if (!AARCH64IsVarRegister(base) || base->reg == NULL ||
      IsShortLivedVarReg(allocator, base) ||
      base->reg->owner != base ||
      (allocator->g->base.num_calls != 0 &&
       (base->reg->num < AARCH64_INT_SAVED_START ||
        base->reg->num > AARCH64_INT_SAVED_END))) {
    return false;
  }
  TargetBasicBlock* entry = allocator->g->base.entry_block;
  if (entry == NULL) {
    return false;
  }
  int defs = 0;
  for (size_t b = 0; b < allocator->g->base.basic_blocks.length; b++) {
    TargetBasicBlock* blk = VectorGet(&allocator->g->base.basic_blocks, b);
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->dest == base) {
        if (blk != entry) {
          return false;
        }
        defs++;
        if (defs > 1) {
          return false;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  return defs == 1;
}

static bool CanRematAllProcessedUsers(AARCH64RegisterAllocator* allocator,
                                      TargetInstruction* inst) {
  AARCH64Register* scratch =
      ScratchRegister(allocator, RegisterTypeFromInstruction(inst));
  if (scratch == NULL) {
    return false;
  }
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* u = inst->users.value.p[i];
    if (NotProcessed(u, allocator)) {
      continue;
    }
    if (IsBeingAllocated(allocator, u) || u->block == NULL) {
      return false;
    }
    if ((int)u->opcode == (int)AARCH64_OP(spill) ||
        (int)u->opcode == (int)AARCH64_OP(reload)) {
      continue;
    }
    for (size_t k = 0; k < TARGET_MAX_OPERANDS; k++) {
      TargetInstruction* op = u->operand[k];
      if (op != NULL && op != inst && op->reg == &scratch->base) {
        return false;
      }
    }
  }
  return true;
}

static TargetInstruction* VariableRegisterDefinition(
    AARCH64RegisterAllocator* allocator, TargetInstruction* src);

// True when `user_block` lies in an acyclic CFG region dominated by
// `def_block` in which every predecessor path from `def_block` to `user_block`
// has already been completely register-allocated.  In that case no
// unprocessed block (including the block currently triggering a spill) can
// execute before `user_block` or re-enter `user_block`, so `user` is
// guaranteed to observe the original register value without a reload.
static bool ReadGuaranteedBeforeHandover(AARCH64RegisterAllocator* allocator,
                                         TargetInstruction* value,
                                         TargetInstruction* user) {
  if (user == NULL || user->block == NULL ||
      (user->flags & TARGET_INST_PROCESSED) == 0 ||
      IsBeingAllocated(allocator, user)) {
    return false;
  }
  TargetGenerator* gen = &allocator->g->base;
  TargetBasicBlock* def_block = value->block;
  if (AARCH64IsVarRegister(value)) {
    TargetInstruction* def = VariableRegisterDefinition(allocator, value);
    if (def == NULL || def->block == NULL) {
      return false;
    }
    def_block = def->block;
  } else if (def_block == NULL || InstructionHasExternalDefs(allocator, value)) {
    return false;
  }

  TargetBasicBlock* user_block = user->block;
  BitSet reachable_after_user;
  BitSetInit(&reachable_after_user);
  TargetBasicBlockReachableAfter(gen, user_block, &reachable_after_user);
  bool cyclic = BitSetContains(&reachable_after_user, user_block->block_id);
  BitSetDestruct(&reachable_after_user);
  if (cyclic) {
    return false;
  }
  if (user_block == def_block) {
    return true;
  }

  size_t nblocks = gen->basic_blocks.length;
  bool* visited = calloc(nblocks, sizeof(bool));
  size_t* queue = malloc(nblocks * sizeof(size_t));
  size_t head = 0, tail = 0;
  visited[user_block->block_id] = true;
  queue[tail++] = user_block->block_id;
  bool ok = true;
  while (head < tail) {
    size_t bid = queue[head++];
    if (bid == def_block->block_id) {
      if (def_block->end_code == NULL ||
          (def_block->end_code->flags & TARGET_INST_PROCESSED) == 0) {
        ok = false;
        break;
      }
      continue;
    }
    TargetBasicBlock* b = VectorGet(&gen->basic_blocks, bid);
    if (b == NULL || b->in_edges.length == 0) {
      ok = false;
      break;
    }
    if (bid != user_block->block_id) {
      if (b->end_code == NULL ||
          (b->end_code->flags & TARGET_INST_PROCESSED) == 0) {
        ok = false;
        break;
      }
      BitSet reach_b;
      BitSetInit(&reach_b);
      TargetBasicBlockReachableAfter(gen, b, &reach_b);
      bool b_cyclic = BitSetContains(&reach_b, bid);
      BitSetDestruct(&reach_b);
      if (b_cyclic) {
        ok = false;
        break;
      }
    }
    for (size_t e = 0; e < b->in_edges.length; e++) {
      size_t pid = (size_t)b->in_edges.value.w[e];
      if (pid >= nblocks) {
        ok = false;
        break;
      }
      if (!visited[pid]) {
        visited[pid] = true;
        queue[tail++] = pid;
      }
    }
    if (!ok) {
      break;
    }
  }
  free(queue);
  free(visited);
  return ok;
}

// Records that |inst|, if it is a register variable, is spilled to |slot|.
static void NoteSpilledVariable(AARCH64Generator* g, TargetInstruction* inst,
                                int slot) {
  for (size_t i = 0; i < g->var_regs.length; i++) {
    RegisterVariable* var = g->var_regs.value.p[i];
    if (var->inst == inst) {
      var->spill_slot = var->spilled && var->spill_slot != slot ? -1 : slot;
      var->spilled = true;
    }
  }
}

static AARCH64Register* SpillInstruction(AARCH64RegisterAllocator* allocator, TargetInstruction* inst) {
  AARCH64Register* reg = (AARCH64Register*)inst->reg;    // Current register.

  if (AARCH64IsVarRegister(inst)) {
    // A value assigned into a variable is left sharing the variable's register
    // rather than copied into it, so the register holds both.  The variable
    // survives losing it -- every assignment writes the slot as well -- but the
    // value's readers name the register directly, and they read it after the
    // spill has handed it to something else.  Spill the value too so those
    // readers reload it instead.
    MapKeyType key = {.p = inst};
    TargetInstruction* value = MapFind(&allocator->varreg_values, key);
    MapRemove(&allocator->varreg_values, key);
    if (value != NULL && value->reg == inst->reg && value->uses > 0 &&
        (value->flags & TARGET_INST_SPILLED) == 0) {
      SpillInstruction(allocator, value);
    }
  }

  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  bool remat_only = IsRematerializableAddSubShape(allocator, inst) &&
                    (inst->operand[0]->flags & TARGET_INST_SPILLED) == 0 &&
                    CanRematAllProcessedUsers(allocator, inst);
  int spill_bytes = GetRegisterSize(inst) == kSize128Bit ? 16 : 8;
  if (!remat_only && spill_bytes == 16) {
    allocator->current_spilled_region_size =
        (allocator->current_spilled_region_size + 15) & ~15;
  }
  // The emitter addresses slot |s| 8 + |s| bytes below the top of the spill
  // region, so a 16-byte slot at depth |d| is numbered |d| + 8; numbered |d|,
  // its upper half would overlap the previous slot or the locals.
  TargetInstruction* spill = TargetNewInstruction2(
      (TargetOpcode)AARCH64_OP(spill), NULL,
      remat_only ? NULL
                 : TargetGetIntConstant(&allocator->g->base, NULL,
                                        kTargetType32Bit,
                                        allocator->current_spilled_region_size +
                                            spill_bytes - 8));
  SetInstructionSize(spill, GetRegisterSize(inst) == 0 ? kSize64Bit
                                                       : GetRegisterSize(inst));
  if (!remat_only) {
    allocator->current_spilled_region_size += spill_bytes;
    if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
      allocator->max_spilled_region_size = allocator->current_spilled_region_size;
    }
  }
 
  if (remat_only) {
    TargetTrackOrphanInstruction(&allocator->g->base, spill);
  } else if (AARCH64IsVarRegister(inst) ||
      InstructionHasExternalDefs(allocator, inst)) {
    // Variable and merge pseudos have no executable definition of their own.
    // Keep the spill only as a slot handle and write that slot after every
    // real assignment, including assignments already allocated.
    TargetTrackOrphanInstruction(&allocator->g->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    InsertReassignableStoreBacks(allocator, inst, spill);
    NoteSpilledVariable(allocator->g, inst,
                        (int)TargetIntValue(spill->operand[1]));
  } else {
    // Emit spill instruction just after spilled instruction.
    TargetBasicBlockEmitAfter(&allocator->g->base, inst->block, spill, inst);
  }
  
  // Retarget all uses of the original instruction to the spill.  If the
  // user has already been processed this will have no effect.
  // NOTE: this will transfer all uses of the inst to the spill, leaving
  // the users of inst empty and its uses count 0.
  TargetRetargetInstructionIf(inst, spill, NotProcessed, allocator);
  spill->uses = inst->uses;
  spill->operand[0] = inst;
  spill->reg = inst->reg;
  bool kept_direct_reader = false;
  // Whatever is left over reads the register directly and has already been
  // allocated, so it has to be repaired in place unless it sits in an acyclic
  // already-allocated prefix that cannot be re-entered after this spill.
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* u = inst->users.value.p[i];
    if (ReadGuaranteedBeforeHandover(allocator, inst, u)) {
      kept_direct_reader = true;
      continue;
    }
    if (!RepairProcessedRead(allocator, u, inst, spill)) {
      kept_direct_reader = true;
    }
  }
  if (remat_only && !kept_direct_reader) {
    inst->observable_checkpoint = true;
  }
  reg->base.owner = NULL;
  inst->flags |= TARGET_INST_SPILLED;
  return reg;
}

// x17, which the allocator never hands out, for a value that is stored to its
// spill slot by the following instruction.  It cannot be x16: a spill store
// computes the slot's address into that.  There is no counterpart in the
// floating point file.
static AARCH64Register* ScratchRegister(AARCH64RegisterAllocator* allocator,
                                        AARCH64RegisterType type) {
  return type == kAARCH64RegTypeInt ? &allocator->int_regs[AARCH64_IP2_REG]
                                    : NULL;
}

// A value whose register a read has already been handed cannot be spilled: that
// read keeps naming the register, and once control comes back round a loop the
// register's next owner is what it finds there.  When every register holds such
// a value there is no victim to take, and the way out is to spill the value
// being defined here instead: it is computed into the scratch register and
// written straight to a slot, and every read of it is still ahead of us and so
// gets redirected to that slot.  A value written by other instructions than
// itself takes the same route: the node carries nothing at its own position, so
// the slot is only registered here and each definition stores into it as it is
// reached.
//
// A variable register is the one thing this cannot do, because its slot is
// written after every real assignment rather than at the node itself, which
// sits in the entry block; likewise a value bound to a fixed register, which
// its consumer reads by name.
static bool CanSpillAfterDefinition(TargetInstruction* inst) {
  return inst != NULL && inst->users.length > 0 && inst->block != NULL &&
         !AARCH64IsVarRegister(inst) && !IsUnspillableFixedReg(inst) &&
         (inst->dest == NULL || !AARCH64IsVarRegister(inst->dest));
}

static AARCH64Register* AllocateRegisterWithType(AARCH64RegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            AARCH64RegisterType type,
                                            AARCH64RegPool pool,
                                            bool may_spill_after_def) {
  AARCH64Register* reg = FindFreeRegisterForInst(allocator, inst, type, pool);

  if (reg == NULL) {
    AARCH64Register* scratch = ScratchRegister(allocator, type);
    TargetInstruction* unsafe = NULL;
    TargetInstruction* victim = FindSpillVictim(
        allocator, type, pool, block,
        may_spill_after_def && scratch != NULL && CanSpillAfterDefinition(inst)
            ? &unsafe
            : NULL);
    // FindSpillVictim can release a stale/dead owner while scanning.
    reg = FindFreeRegisterForInst(allocator, inst, type, pool);
    if (reg == NULL && victim == NULL && unsafe != NULL) {
      allocator->spill_after_definition = true;
      return scratch;
    }
    if (reg == NULL && victim != NULL) {
      reg = SpillInstruction(allocator, victim);
    }
  }
  if (reg == NULL) {
    DumpRegisters(allocator);
    abort();
  }

  if (reg->base.reserved) {
    return reg;
  }
  if (!IsSavedReg(reg)) {
    return reg;
  }
  switch (type) {
    case kAARCH64RegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kAARCH64RegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

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

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static AARCH64RegPool RegisterPool(AARCH64RegisterAllocator* allocator,
                                   TargetInstruction* inst) {
  if (IsShortLivedVarReg(allocator, inst)) {
    return kRegPoolAny;
  }
  if (BitSetContains(&allocator->preserved_instructions, inst->id)) {
    return kRegPoolSaved;
  }
  if ((inst->flags & AARCH64_INST_AVOID_ARG_REGS) != 0) {
    return kRegPoolNoArgs;
  }
  return kRegPoolAny;
}

static void AllocateVariableRegister(AARCH64RegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  AARCH64RegisterType reg_type = RegisterTypeFromInstruction(inst);
  
  AARCH64Register* reg = NULL;
  if (reg_type == kAARCH64RegTypeInt &&
      allocator->g->struct_return_reg >= 0) {
    for (size_t i = 0; i < allocator->g->var_regs.length; i++) {
      RegisterVariable* variable = allocator->g->var_regs.value.p[i];
      if (variable->inst == inst &&
          variable->varnum == allocator->g->struct_return_reg) {
        bool is_leaf = allocator->g->base.num_calls == 0 && compiler->optimize;
        int first = is_leaf ? AARCH64_FIRST_LEAF_INT_REG_VAR
                            : AARCH64_FIRST_INT_REG_VAR;
        reg = &allocator->int_regs[first + variable->varnum];
        break;
      }
    }
  }
  if (reg == NULL) {
    reg = AllocateRegisterWithType(allocator, inst->block, inst, reg_type,
                                   RegisterPool(allocator, inst), false);
  }
  AssignRegister(reg, inst);
}

static bool RegisterIsArgument(AARCH64Register* reg) {
  if (reg->type == kAARCH64RegTypeInt) {
    return reg->base.num >= AARCH64_INT_ARG_START &&
           reg->base.num <= AARCH64_INT_ARG_END;
  }
  return reg->base.num >= AARCH64_FP_ARG_START &&
         reg->base.num <= AARCH64_FP_ARG_END;
}

static bool PoolAllowsRegister(AARCH64RegPool pool, AARCH64Register* reg) {
  if (pool == kRegPoolSaved) {
    return IsSavedReg(reg);
  }
  if (pool == kRegPoolNoArgs) {
    return !RegisterIsArgument(reg);
  }
  return true;
}

// The single already-allocated definition of variable register `src`, or
// NULL when it has none or several.
static TargetInstruction* VariableRegisterDefinition(
    AARCH64RegisterAllocator* allocator, TargetInstruction* src) {
  TargetInstruction* definition = NULL;
  int definitions = 0;
  for (size_t b = 0; b < allocator->g->base.basic_blocks.length; b++) {
    TargetBasicBlock* block =
        VectorGet(&allocator->g->base.basic_blocks, b);
    for (TargetInstruction* p = block->code; p != NULL;
         p = TargetNext(p)) {
      if (p->dest == src) {
        definitions++;
        definition = p;
        if (!(p->flags & TARGET_INST_PROCESSED)) {
          return NULL;
        }
      }
      if (p == block->end_code) {
        break;
      }
    }
  }
  return definitions == 1 ? definition : NULL;
}

// True when variable register `src` is dead once `copy` has read it: it has
// exactly one definition in the function (already allocated, so the copy
// reads the register that definition wrote), every other user has already
// been allocated (a user in a successor or in this block later would still
// be unprocessed; a user that executes again via a back edge makes `src`
// live out of this block, which the caller rejects), and nothing later in
// this block reads or rewrites it.
static bool VariableRegisterDeadAfterCopy(AARCH64RegisterAllocator* allocator,
                                          TargetInstruction* copy,
                                          TargetInstruction* src) {
  TargetBasicBlock* copy_block = copy->block;
  TargetInstruction* definition =
      VariableRegisterDefinition(allocator, src);
  if (definition == NULL) {
    return false;
  }
  // The defining value shares the variable's register, so its own users
  // read that register too and must be just as dead.
  if (copy_block != NULL &&
      BitSetContains(&copy_block->output_ids, definition->id)) {
    return false;
  }
  for (size_t i = 0; i < src->users.length; i++) {
    TargetInstruction* user = src->users.value.p[i];
    if (user != copy && !(user->flags & TARGET_INST_PROCESSED)) {
      return false;
    }
  }
  for (size_t i = 0; i < definition->users.length; i++) {
    TargetInstruction* user = definition->users.value.p[i];
    if (!(user->flags & TARGET_INST_PROCESSED)) {
      return false;
    }
  }
  if (copy_block != NULL) {
    for (TargetInstruction* p = TargetNext(copy); p != NULL;
         p = TargetNext(p)) {
      if (p->dest == src) {
        return false;
      }
      for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
        if (p->operand[i] == src || p->operand[i] == definition) {
          return false;
        }
      }
      if (p == copy_block->end_code) {
        break;
      }
    }
  }
  return true;
}

// True when some instruction other than the excluded ones holds `reg`
// with a value that may still be read after this point: it has a user
// that has not been allocated yet, or it is live out of its own block
// (read in a successor, possibly across a loop back edge).  Register
// ownership in this allocator is per-block and approximate across
// regions, so a register that looks free in this block may still hold a
// live value on another path through the function.
static bool RegisterHasOtherLiveHolder(
    AARCH64RegisterAllocator* allocator, TargetRegister* reg,
    TargetInstruction* keep1, TargetInstruction* keep2,
    TargetInstruction* keep3) {
  for (size_t b = 0; b < allocator->g->base.basic_blocks.length; b++) {
    TargetBasicBlock* block =
        VectorGet(&allocator->g->base.basic_blocks, b);
    for (TargetInstruction* q = block->code; q != NULL; q = TargetNext(q)) {
      if (q != keep1 && q != keep2 && q != keep3 && q->reg == reg) {
        if (q->block != NULL &&
            BitSetContains(&q->block->output_ids, q->id)) {
          return true;
        }
        for (size_t i = 0; i < q->users.length; i++) {
          TargetInstruction* user = q->users.value.p[i];
          if (!(user->flags & TARGET_INST_PROCESSED)) {
            return true;
          }
        }
      }
      if (q == block->end_code) {
        break;
      }
    }
  }
  return false;
}

// The destination variable already has a register the copy cannot change,
// but when the copied value is computed in this same block and dies at the
// copy, its definition can write the variable's register directly instead.
// Nothing between the definition and the copy may read or write that
// register (other than the definition's own operands, e.g. `v = v + 1`
// routed through a temporary), and a call in between requires the register
// to be callee-saved.
static bool TryRetargetCopySourceDefinition(
    AARCH64RegisterAllocator* allocator, TargetInstruction* inst,
    TargetInstruction* varreg, TargetInstruction* src) {
  if (inst->block == NULL) {
    return false;
  }
  TargetInstruction* definition = src;
  if (AARCH64IsVarRegister(src)) {
    if (!IsShortLivedVarReg(allocator, src) &&
        !VariableRegisterDeadAfterCopy(allocator, inst, src)) {
      return false;
    }
    definition = VariableRegisterDefinition(allocator, src);
    if (definition == NULL || definition->block != inst->block) {
      return false;
    }
  } else {
    if (src->block != inst->block || AARCH64IsFixedRegister(src) ||
        src->dest != NULL || SharesRegisterWithVariable(allocator, src) ||
        src->uses != 1 ||
        (AARCH64Opcode)src->opcode == AARCH64_OP(tmp)) {
      return false;
    }
    // A holder whose value is written by another instruction in this block
    // (the `tmp` staging pattern) computes nothing itself; renaming it
    // would not rename the actual writer.
    for (TargetInstruction* p = inst->block->code; p != NULL;
         p = TargetNext(p)) {
      if (p->dest == src) {
        return false;
      }
      if (p == inst->block->end_code) {
        break;
      }
    }
  }
  // Some definitions produce their result in an architecturally fixed
  // register (call results) or expand later and need a register of their
  // own (atomics); renaming their destination register would lose it.
  switch ((AARCH64Opcode)definition->opcode) {
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
    case AARCH64_OP(resulti):
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
    case AARCH64_OP(tp):
    case AARCH64_OP(atomic_load):
    case AARCH64_OP(atomic_fetch_add):
    case AARCH64_OP(atomic_fetch_sub):
    case AARCH64_OP(atomic_add_fetch):
    case AARCH64_OP(atomic_sub_fetch):
    case AARCH64_OP(atomic_compare_exchange_bool):
    case AARCH64_OP(atomic_compare_exchange_val):
    case AARCH64_OP(atomic_compare_exchange_n):
      return false;
    default:
      break;
  }
  if ((definition->flags & TARGET_INST_SPILLED) != 0 ||
      (src->flags & TARGET_INST_SPILLED) != 0 ||
      ((AARCH64Register*)varreg->reg)->type !=
          RegisterTypeFromInstruction(src)) {
    return false;
  }
  // A spilled variable's register may currently belong to another live
  // value (the copy would normally be routed through the scratch register
  // and stored back to the slot).  More generally the register must be
  // owned by the variable itself, by the source, or by nobody: writing the
  // definition into a stranger's register would clobber its value.
  if ((varreg->flags & TARGET_INST_SPILLED) != 0) {
    return false;
  }
  TargetInstruction* target_owner = varreg->reg->owner;
  if (target_owner != NULL && target_owner != varreg &&
      target_owner != src && target_owner != definition) {
    return false;
  }
  // Another value live into this block may share the variable's register
  // (chosen in a sibling region where it was free) and be read after the
  // copy; writing the definition into the register here would clobber it.
  if (inst->block != NULL) {
    for (size_t i = 0; i < inst->block->inputs.length; i++) {
      TargetInstruction* input = inst->block->inputs.value.p[i];
      if (input != varreg && input != src && input != definition &&
          input->reg == varreg->reg) {
        return false;
      }
    }
  }
  if (inst->block != NULL &&
      BitSetContains(&inst->block->output_ids, src->id)) {
    return false;
  }
  if (RegisterHasOtherLiveHolder(allocator, varreg->reg, src, definition,
                                  varreg)) {
    return false;
  }
  TargetRegister* target = varreg->reg;
  bool saved = IsSavedReg((AARCH64Register*)target);
  bool found = false;
  for (TargetInstruction* p = inst->block->code; p != NULL;
       p = TargetNext(p)) {
    if (p == inst) {
      break;
    }
    if (found) {
      if (p->reg == target || p->dest == varreg ||
          (p->dest != NULL && p->dest->reg == target)) {
        return false;
      }
      for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
        TargetInstruction* operand = p->operand[i];
        if (operand != NULL && operand != src && operand->reg == target) {
          return false;
        }
      }
      AARCH64Opcode opcode = (AARCH64Opcode)p->opcode;
      if (!saved && (opcode == AARCH64_OP(bl) || opcode == AARCH64_OP(blr))) {
        return false;
      }
    }
    if (p == definition) {
      found = true;
    }
    if (p == inst->block->end_code) {
      break;
    }
  }
  if (!found) {
    return false;
  }
  // The register the definition used to write is abandoned; drop its stale
  // ownership so a later spill victim search cannot act on this definition
  // through it (the definition now names the variable's register).
  TargetRegister* old = src->reg;
  if (old != target && (old->owner == src || old->owner == definition)) {
    old->owner = NULL;
  }
  definition->reg = target;
  src->reg = target;
  target->owner = varreg;
  return true;
}

// A pure copy `mov src -> varreg` whose source dies at the copy need not
// move at all: either the variable takes over the source's register (when
// it has none yet), or the source's definition is retargeted to write the
// variable's register directly.  Both are only sound when the source has
// no other remaining use and is not live out of the block (so no successor,
// including one reached by a loop back edge, can still read it), when
// neither side is tied to another variable or a fixed register, and when
// the register satisfies the pools both the copy and the variable require
// (a variable live across a call may not inherit a caller-saved register).
static bool TryCoalesceCopyIntoVariableRegister(
    AARCH64RegisterAllocator* allocator, TargetInstruction* inst) {
  AARCH64Opcode opcode = (AARCH64Opcode)inst->opcode;
  if (opcode != AARCH64_OP(mv) && opcode != AARCH64_OP(mov) &&
      opcode != AARCH64_OP(fmov) && opcode != AARCH64_OP(fmv_s) &&
      opcode != AARCH64_OP(fmv_d)) {
    return false;
  }

  TargetInstruction* varreg = inst->dest;
  TargetInstruction* src = inst->operand[0];
  if (varreg == NULL || src == NULL || inst->operand[1] != NULL ||
      src == varreg) {
    return false;
  }
  if (src->reg == NULL || (src->flags & TARGET_INST_SPILLED) != 0) {
    return false;
  }
  if (varreg->reg != NULL) {
    if (varreg->reg == src->reg) {
      return true;
    }
    return TryRetargetCopySourceDefinition(allocator, inst, varreg, src);
  }
  if (AARCH64IsFixedRegister(src)) { return false; }
  if (src->dest != NULL) { return false; }
  if (SharesRegisterWithVariable(allocator, src)) { return false; }
  if (AARCH64IsVarRegister(src) && !IsShortLivedVarReg(allocator, src) &&
      !VariableRegisterDeadAfterCopy(allocator, inst, src)) {
    return false;
  }
  if (src->uses != 1) {
    return false;
  }
  if (inst->block != NULL &&
      BitSetContains(&inst->block->output_ids, src->id)) {
    return false;
  }
  AARCH64Register* reg = (AARCH64Register*)src->reg;
  if (reg->base.reserved) { return false; }
  if (reg->type != RegisterTypeFromInstruction(varreg)) {
    return false;
  }
  // The register must not be another variable's home, nor hold a value
  // live into this block: the variable keeps it for the rest of the
  // function, across regions where those owners are still read.
  for (size_t i = 0; i < allocator->g->var_regs.length; i++) {
    RegisterVariable* variable = allocator->g->var_regs.value.p[i];
    if (variable->inst != varreg && variable->inst != src &&
        variable->inst->reg == &reg->base) {
      return false;
    }
  }
  if (inst->block != NULL) {
    for (size_t i = 0; i < inst->block->inputs.length; i++) {
      TargetInstruction* input = inst->block->inputs.value.p[i];
      if (input != src && input->reg == &reg->base) {
        return false;
      }
    }
  }
  if (RegisterHasOtherLiveHolder(allocator, &reg->base, src, varreg,
                                  NULL)) {
    return false;
  }
  // The hidden aggregate-result variable has a dedicated register.
  if (reg->type == kAARCH64RegTypeInt &&
      allocator->g->struct_return_reg >= 0) {
    for (size_t i = 0; i < allocator->g->var_regs.length; i++) {
      RegisterVariable* variable = allocator->g->var_regs.value.p[i];
      if (variable->inst == varreg &&
          variable->varnum == allocator->g->struct_return_reg) {
        return false;
      }
    }
  }
  if (!PoolAllowsRegister(RegisterPool(allocator, inst), reg) ||
      !PoolAllowsRegister(RegisterPool(allocator, varreg), reg)) {
    return false;
  }

  varreg->reg = &reg->base;
  varreg->uses = (int)varreg->users.length;
  varreg->flags |= TARGET_INST_PROCESSED;
  reg->base.owner = varreg;
  return true;
}

static COMPILER_UNUSED void AllocateForRmov(
    AARCH64RegisterAllocator* allocator, TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)AARCH64_OP(mv)) || ((int)inst->opcode == (int)AARCH64_OP(fmv_s)) ||
         ((int)inst->opcode == (int)AARCH64_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  if (AARCH64IsVarRegister(dest) && dest->reg == NULL) {
    // Delayed allocation of variable register.
    AllocateVariableRegister(allocator, dest);
  }
  AARCH64Register* reg = (AARCH64Register*)dest->reg;
  assert(reg != NULL);
  
  if (((int)src->opcode == (int)AARCH64_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)AARCH64_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    TrapReload(inst);
  } else {
    if (AARCH64IsVarRegister(src) && src->reg == NULL) {
      // Delayed allocation of variable register.
      AllocateVariableRegister(allocator, src);
    }
    inst->operand[0]->uses++;
    FreeRegisters(allocator, inst);
  }
  
  inst->reg = &reg->base;
  inst->flags |= TARGET_INST_PROCESSED;
}

static void ReloadSpills(AARCH64RegisterAllocator* allocator,
                         TargetInstruction* inst) {
  // All operands of `inst` are simultaneously live when `inst` executes, so a
  // register allocated for reloading one operand must not be reused for another
  // operand's reload (nor stolen as a spill victim).  Temporarily reserve each
  // operand's register -- both those already holding a value and those we
  // allocate here for reloads -- so FindFreeRegister/FindSpillVictim skip them,
  // then release the reservations once every operand has its register.  Without
  // this, reloading a second spilled operand could spill the first operand and
  // reuse its register, leaving the first operand reading the wrong value
  // (e.g. `cmp w23, w23` where a class bound and `'\\'` collapsed onto the
  // same register in regex __class_match at -O2).  An operand can contribute
  // both the register it holds and its reload's.
  AARCH64Register* protect[2 * TARGET_MAX_OPERANDS];
  int num_protect = 0;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && op->reg != NULL) {
      AARCH64Register* r = (AARCH64Register*)op->reg;
      if (!r->base.reserved) {
        r->base.reserved = true;
        protect[num_protect++] = r;
      }
    }
  }
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ((int)op->opcode == (int)AARCH64_OP(spill))) {
      assert(op->operand[0] != NULL);
      TargetInstruction* spilled_val = op->operand[0];
      if (op->operand[1] == NULL ||
          (IsRematerializableAddSubShape(allocator, spilled_val) &&
           (spilled_val->operand[0]->flags & TARGET_INST_SPILLED) == 0)) {
        TargetInstruction* base_op = spilled_val->operand[0];
        if ((base_op->flags & TARGET_INST_SPILLED) != 0) {
          TargetInstruction* bspill =
              MapFindPointerKey(&allocator->reassignable_spills, base_op);
          assert(bspill != NULL);
          TargetInstruction* breload =
              TargetNewInstruction1((TargetOpcode)AARCH64_OP(reload), bspill);
          TrapReload(breload);
          TargetBasicBlockEmitBefore(&allocator->g->base, inst->block, breload,
                                     inst);
          AARCH64Register* breg = AllocateRegisterWithType(
              allocator, breload->block, breload, kAARCH64RegTypeInt,
              kRegPoolAny, false);
          AssignRegister(breg, breload);
          breload->uses = 1;
          base_op = breload;
        }
        TargetInstruction* remat = TargetNewInstruction(spilled_val->opcode);
        remat->operand[0] = base_op;
        remat->operand[1] = spilled_val->operand[1];
        remat->flags =
            (remat->flags & ~(3 << 16)) | (spilled_val->flags & (3 << 16));
        TargetBasicBlockEmitBefore(&allocator->g->base, inst->block, remat,
                                   inst);
        inst->operand[i] = remat;
        bool pinned_base = base_op->reg != NULL && !base_op->reg->reserved;
        if (pinned_base) {
          base_op->reg->reserved = true;
        }
        AARCH64Register* reg = AllocateRegisterWithType(
            allocator, remat->block, remat, kAARCH64RegTypeInt,
            RegisterPool(allocator, remat), false);
        if (pinned_base) {
          base_op->reg->reserved = false;
        }
        AssignRegister(reg, remat);
        remat->uses = 1;
        if (base_op->opcode == (TargetOpcode)AARCH64_OP(reload) &&
            base_op->reg != NULL && !base_op->reg->reserved) {
          FreeRegister(allocator, (AARCH64Register*)base_op->reg);
        }
        if (!reg->base.reserved) {
          reg->base.reserved = true;
          protect[num_protect++] = reg;
        }
        continue;
      }
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)AARCH64_OP(reload),
                                                        op);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->g->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      // A store/load can mix register classes (for example fstr has an FP
      // value and an integer address).  Reload in the class of the value that
      // was spilled, not the class implied by the consuming instruction.
      assert(op->operand[0] != NULL);
      AARCH64RegisterType reg_type =
          RegisterTypeFromInstruction(op->operand[0]);
      AARCH64Register *reg = AllocateRegisterWithType(
          allocator, reload->block, reload, reg_type,
          RegisterPool(allocator, reload), false);
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
      if (!reg->base.reserved) {
        reg->base.reserved = true;
        protect[num_protect++] = reg;
      }
    }
  }
  for (int i = 0; i < num_protect; i++) {
    protect[i]->base.reserved = false;
  }
}

// Allocate registers for operand instructions (e.g. address calcs for ldr/str)
// that are not reached by the linear block scan.
static void EnsureOperandsAllocated(AARCH64RegisterAllocator* allocator,
                                    TargetInstruction* inst) {
  // Operands of one instruction are live together.  Reserve each operand's
  // register as it is assigned so a later operand cannot steal it (the same
  // collapse ReloadSpills guards against when inserting reloads).
  AARCH64Register* protect[TARGET_MAX_OPERANDS];
  int num_protect = 0;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && op->reg != NULL && !op->reg->reserved) {
      op->reg->reserved = true;
      protect[num_protect++] = (AARCH64Register*)op->reg;
    }
  }
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op == NULL || op->reg != NULL || op->block == NULL ||
        TargetIsConst(op)) {
      continue;
    }
    if (AARCH64IsFixedRegister(op)) {
      AllocateRegister(allocator, op);
    } else if (AARCH64IsVarRegister(op)) {
      if (op->reg == NULL) {
        AllocateVariableRegister(allocator, op);
      }
    } else if ((op->flags & TARGET_INST_PROCESSED) == 0) {
      AllocateRegister(allocator, op);
    }
    if (op->reg != NULL && !op->reg->reserved) {
      op->reg->reserved = true;
      protect[num_protect++] = (AARCH64Register*)op->reg;
    }
  }
  for (int i = 0; i < num_protect; i++) {
    protect[i]->base.reserved = false;
  }
}

static void AllocateRegisterOnce(AARCH64RegisterAllocator* allocator,
                                 TargetInstruction* inst);

static void AllocateRegister(AARCH64RegisterAllocator* allocator,
                             TargetInstruction* inst) {
  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }

  if (AARCH64IsVarRegister(inst)) {
    // Variable regsiter.  Delay allocation until it's assigned to.
    // It will be assigned to by an rmov or from a destination
    // assignemnt.
    return;
  }

  // Reload any spilled expressions.
  ReloadSpills(allocator, inst);

  // Every read of this instruction is now committed to a physical register, so
  // a spill triggered by the rest of its allocation (finding a register for an
  // operand, for its destination variable, or for the result) can no longer
  // redirect those reads to the slot by retargeting: this instruction's reload
  // pass has already run.  Record it as in flight so such a spill repairs the
  // read in place instead.
  bool pushed = allocator->allocating_depth < AARCH64_MAX_ALLOCATION_DEPTH;
  if (pushed) {
    allocator->allocating[allocator->allocating_depth++] = inst;
  }
  AllocateRegisterOnce(allocator, inst);
  if (pushed) {
    allocator->allocating_depth--;
  }
}

static void AllocateRegisterOnce(AARCH64RegisterAllocator* allocator,
                                 TargetInstruction* inst) {
   bool is_leaf = allocator->g->base.num_calls == 0 &&
      compiler->optimize;

  AARCH64Opcode opcode = (AARCH64Opcode)inst->opcode;

  EnsureOperandsAllocated(allocator, inst);

  // Two-operand `mov rN, src` writes an outgoing argument register without
  // going through dest-assignment, so the pre-pass register has no owner after
  // InitializeBasicBlockRegisters.  Claim it before allocating a result or
  // reloading another value so the next argument cannot steal rN.
  if (inst->dest == NULL &&
      (opcode == AARCH64_OP(mov) || opcode == AARCH64_OP(fmov)) &&
      inst->operand[0] != NULL &&
      IsUnspillableFixedReg(inst->operand[0]) &&
      inst->operand[0]->reg != NULL) {
    inst->operand[0]->reg->owner = inst->operand[0];
  }

  AARCH64Register* reg;

  // Atomic pseudos expand to several instructions after allocation.  Their
  // result register must not alias an address/value operand: the exclusive
  // load writes the result before the later store-exclusive consumes all of
  // those operands.
  if (inst->dest == NULL &&
      (opcode == AARCH64_OP(atomic_load) ||
       opcode == AARCH64_OP(atomic_fetch_add) ||
       opcode == AARCH64_OP(atomic_fetch_sub) ||
       opcode == AARCH64_OP(atomic_add_fetch) ||
       opcode == AARCH64_OP(atomic_sub_fetch) ||
       opcode == AARCH64_OP(atomic_compare_exchange_bool) ||
       opcode == AARCH64_OP(atomic_compare_exchange_val) ||
       opcode == AARCH64_OP(atomic_compare_exchange_n))) {
    // An atomic pseudo expands into several instructions once allocation is
    // done, and that expansion needs the scratch register itself, so this
    // result has to have a register of its own.
    reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                   kAARCH64RegTypeInt,
                                   RegisterPool(allocator, inst), false);
    AssignRegister(reg, inst);
    FreeRegisters(allocator, inst);
    inst->flags |= TARGET_INST_PROCESSED;
    return;
  }

  if (inst->dest != NULL) {
    if (AARCH64IsVarRegister(inst->dest) &&
        TryCoalesceCopyIntoVariableRegister(allocator, inst)) {
      // Copy coalesced: the destination already names the right register.
    } else if (inst->dest->reg == NULL) {
      if (AARCH64IsVarRegister(inst->dest)) {
        // Assignment to a variable register,
        AllocateVariableRegister(allocator, inst->dest);
      } else {
        AllocateRegister(allocator, inst->dest);
      }
    }
    if (inst->dest->reg != NULL && inst->block != NULL &&
        AARCH64IsVarRegister(inst->dest) &&
        (opcode == AARCH64_OP(add) || opcode == AARCH64_OP(sub)) &&
        inst->operand[0] != NULL &&
        ((int)inst->operand[0]->opcode == (int)AARCH64_OP(fp) ||
         (inst->operand[0]->reg != NULL &&
          inst->operand[0]->reg->num == AARCH64_FP_REG))) {
      // Variable registers are pre-assigned a physical register.  Several
      // unrelated locals can share one (the pre-pass sees each as unused).
      // Writing `&part` (fp-relative) into a varreg that still holds the
      // `this` copy a processed inner-loop header will reread is what made
      // path::lexically_normal keep only the first component at -O2.
      BitSet reentered;
      BitSetInit(&reentered);
      TargetBasicBlockReachableAfter(&allocator->g->base, inst->block,
                                     &reentered);
      if (RegisterNamedByProcessedRead(&allocator->g->base, inst->dest->reg,
                                       &reentered)) {
        // Leave the physical register owned so the replacement allocation
        // cannot pick it again.  The dest instruction is detached from it.
        inst->dest->reg = NULL;
        AllocateVariableRegister(allocator, inst->dest);
      }
      BitSetDestruct(&reentered);
    }
    if (inst->dest->reg == NULL) {
      return;
    }
    reg = (AARCH64Register*)inst->dest->reg;
    if ((inst->dest->flags & TARGET_INST_SPILLED) != 0 &&
        reg->base.owner != NULL && reg->base.owner != inst->dest) {
      // The destination is in a spill slot and the register it used to hold has
      // since been handed to another value, so writing it here would clobber
      // that value.  Route this assignment through the scratch register; the
      // store-back below writes the slot from there.
      AARCH64Register* scratch =
          ScratchRegister(allocator, RegisterTypeFromInstruction(inst->dest));
      if (scratch != NULL) {
        reg = scratch;
      }
    }
    inst->reg = &reg->base;
    if (AARCH64IsVarRegister(inst->dest) && inst->users.length > 0) {
      // The value and the variable now share one physical register.  Note the
      // pairing so that taking the register away from the variable takes it
      // away from the value too.
      MapKeyValue kv = {.key.p = inst->dest, .value.p = inst};
      MapInsert(&allocator->varreg_values, kv);
    } else {
      MapKeyType key = {.p = inst->dest};
      MapRemove(&allocator->varreg_values, key);
    }
    // A fixed argument-register pseudo (r0..r7 / d0..d7) is allocated once in
    // the symbol pre-pass, but InitializeBasicBlockRegisters clears every
    // physical register's owner at each block boundary.  Because the pseudo
    // already has a register, the allocation above is skipped and ownership is
    // never re-established, so inside a loop body the argument register looks
    // free and an intervening temporary can steal it, clobbering an argument.
    // Always re-claim here: a stale owner (a previous unused mov result) still
    // leaves the register looking spillable, and the next outgoing argument's
    // reload then overwrites it.
    if (IsUnspillableFixedReg(inst->dest)) {
      reg->base.owner = inst->dest;
    }
    FreeRegisters(allocator, inst);
    inst->flags |= TARGET_INST_PROCESSED;
    SyncReassignableSpill(allocator, inst, inst->dest);
    return;
  }

  // Free up any registers we can.
  FreeRegisters(allocator, inst);
  
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(const8):
    case AARCH64_OP(const16):
    case AARCH64_OP(const32):
    case AARCH64_OP(const64):
    case AARCH64_OP(constf):
    case AARCH64_OP(constd):
    case AARCH64_OP(symbol):
    case AARCH64_OP(b):
    case AARCH64_OP(br):
    case AARCH64_OP(label):
    case AARCH64_OP(ret):
    case AARCH64_OP(save):
    case AARCH64_OP(restore):
    case AARCH64_OP(literal):
    case AARCH64_OP(asm):
    case AARCH64_OP(loc):
    case  AARCH64_OP(eq):
    case  AARCH64_OP(ne):
    case  AARCH64_OP(cs):
    case  AARCH64_OP(hs):
    case  AARCH64_OP(cc):
    case  AARCH64_OP(lo):
    case  AARCH64_OP(mi):
    case   AARCH64_OP(pl):
    case   AARCH64_OP(vs):
    case   AARCH64_OP(vc):
    case   AARCH64_OP(hi):
    case   AARCH64_OP(ls):
    case   AARCH64_OP(ge):
    case   AARCH64_OP(lt):
    case   AARCH64_OP(gt):
    case   AARCH64_OP(le):
    case   AARCH64_OP(al):
    case   AARCH64_OP(cmp):
    case   AARCH64_OP(fcmp):
    case AARCH64_OP(dmb):
    case AARCH64_OP(clrex):
    case AARCH64_OP(atomic_store):
    case AARCH64_OP(atomic_fence):
    case AARCH64_OP(stp):
    case   AARCH64_OP(oplsl):
    case AARCH64_OP(spill):
    case AARCH64_OP(reload):
      // These instructions do not have registers allocated to them.  They do
      // read their operands out of registers, though, so they still have to
      // count as processed: that is what tells a later spill that their reads
      // are already committed to a physical register and need repairing rather
      // than retargeting.
      inst->flags |= TARGET_INST_PROCESSED;
      return;

    case AARCH64_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;
      
    case AARCH64_OP(fp):
      reg = &allocator->int_regs[AARCH64_FP_REG];
      break;

    case AARCH64_OP(sp):
      reg = &allocator->int_regs[AARCH64_SP_REG];
      break;

    case AARCH64_OP(lr):
      reg = &allocator->int_regs[AARCH64_LR_REG];
      break;

    case AARCH64_OP(xr):
      reg = &allocator->int_regs[AARCH64_XR_REG];
      break;

    case AARCH64_OP(zr):
      reg = &allocator->int_regs[AARCH64_INT_ZERO_REG];
      break;

    case AARCH64_OP(r9):
      // Dedicated scratch register (x9) used for staging e.g. indirect call
      // targets so they survive argument-register setup.
      reg = &allocator->int_regs[9];
      break;

    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
      reg = &allocator->int_regs[(int)inst->opcode - AARCH64_OP(r0) + AARCH64_INT_ARG_START];
      break;

    case AARCH64_OP(ivarreg):
    case AARCH64_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();
      
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
      reg = &allocator
                 ->float_regs[(int)inst->opcode - AARCH64_OP(d0) + AARCH64_FP_ARG_START];
      break;

    case AARCH64_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? AARCH64_FIRST_LEAF_INT_REG_VAR
                                          : AARCH64_FIRST_INT_REG_VAR) +
                                 allocator->g->struct_return_reg];
      if (IsSavedReg(reg)) {
        BitSetInsert(&allocator->used_int_regs, reg->base.num);
      }
      break;

    case AARCH64_OP(resulti):
      reg = &allocator->int_regs[AARCH64_INT_RETURN_REG];
      break;

    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
      reg = &allocator->float_regs[AARCH64_FLOAT_RETURN_REG];
      break;

    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
      if ((inst->flags & AARCH64_INST_FP_RETURN) != 0) {
        reg = &allocator->float_regs[AARCH64_FLOAT_RETURN_REG];
      } else {
        reg = &allocator->int_regs[AARCH64_INT_RETURN_REG];
      }
      break;
#if 0
    case AARCH64_OP(callf):
    case AARCH64_OP(rcallf):
      reg = &allocator->float_regs[AARCH64_FLOAT_RETURN_REG];
#endif
      break;
      

    default: {
      if (inst->users.length == 0 &&
          (!AARCH64GeneratesOutput(inst) || opcode == AARCH64_OP(fstr))) {
        inst->flags |= TARGET_INST_PROCESSED;
        return;
      }
      AARCH64RegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     reg_type, RegisterPool(allocator, inst),
                                     true);
    }
  }

  AssignRegister(reg, inst);

  if (allocator->spill_after_definition) {
    // No register could be freed, so this value went into the scratch register
    // and has to leave it again before the next instruction.
    allocator->spill_after_definition = false;
    SpillInstruction(allocator, inst);
    return;
  }

  // If nobody is using this register free it up immediately.
  // TODO: argument registers are not used explicitly but can't be freed here.
  if (inst->uses == 0 && !reg->base.reserved &&
      !SharesRegisterWithVariable(allocator, inst) &&
      !(AARCH64IsVarRegister(inst) && !IsShortLivedVarReg(allocator, inst))) {
    FreeRegister(allocator, reg);
  }
}

static bool BlockIsLoopHeader(TargetGenerator* gen, TargetBasicBlock* block) {
  for (size_t i = 0; i < block->in_edges.length; i++) {
    TargetBasicBlock* predecessor =
        VectorGet(&gen->basic_blocks, block->in_edges.value.w[i]);
    if (predecessor != NULL &&
        BitSetContains(&predecessor->dominators, block->block_id)) {
      return true;
    }
  }
  return false;
}

static TargetInstruction* InstructionRegisterHolder(TargetInstruction* inst) {
  if (inst == NULL) {
    return NULL;
  }
  if (inst->reg != NULL) {
    return inst;
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return inst->dest;
  }
  return NULL;
}

static void ClaimOperandRegister(AARCH64RegisterAllocator* allocator,
                                 TargetInstruction* operand) {
  TargetInstruction* holder = InstructionRegisterHolder(operand);
  if (holder == NULL || holder->reg == NULL ||
      IsShortLivedVarReg(allocator, holder) ||
      ((int)holder->opcode == (int)AARCH64_OP(spill)) ||
      ((int)holder->opcode == (int)AARCH64_OP(reload)) ||
      (holder->flags & TARGET_INST_SPILLED) != 0) {
    return;
  }
  // The header operand must be the owner even when another copy that shares
  // the register still has a positive use count.  Otherwise FindSpillVictim
  // sees the other copy, decides it is safe to evict, and the header reload
  // still names the stolen register.
  holder->reg->owner = holder;
}

// A processed header read already names a physical register.  An inner
// loop header that does not dominate the outer latch still rereads that
// register on the next outer iteration, so pin every processed natural-loop
// header's operands, not only headers that dominate the current block.  A
// header that cannot be reached again from |block| belongs to a loop that has
// already finished, and its register may now hold one of |block|'s live-ins.
static void ClaimRegistersNamedByProcessedHeaders(
    AARCH64RegisterAllocator* allocator, TargetBasicBlock* block) {
  TargetGenerator* gen = &allocator->g->base;
  BitSet reachable;
  BitSetInit(&reachable);
  TargetBasicBlockReachableAfter(gen, block, &reachable);
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* header = gen->basic_blocks.value.p[i];
    if (header == NULL || TargetBasicBlockIsEmpty(header) ||
        !BlockIsLoopHeader(gen, header) ||
        !BitSetContains(&reachable, header->block_id)) {
      continue;
    }
    for (TargetInstruction* inst = header->code; inst != NULL;
         inst = inst == header->end_code ? NULL : TargetNext(inst)) {
      if ((inst->flags & TARGET_INST_PROCESSED) == 0) {
        continue;
      }
      for (size_t op = 0; op < TARGET_MAX_OPERANDS; op++) {
        ClaimOperandRegister(allocator, inst->operand[op]);
      }
    }
  }
  BitSetDestruct(&reachable);
}

static void InitializeBasicBlockRegisters(AARCH64RegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < AARCH64_NUM_INT_REGS; i++) {
    AARCH64Register* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < AARCH64_NUM_FLOAT_REGS; i++) {
     AARCH64Register* reg = &allocator->float_regs[i];
     if (reg->base.reserved) {
       continue;
     }
     reg->base.owner = NULL;
  }
    
  // Now allocate the registers to the inputs.  Values with reads still ahead of
  // them are claimed first: liveness is conservative, so the live-in set can
  // name several values that were handed the same physical register at
  // different points in the function, and whichever claims it last is the one
  // the block treats as holding it.  A value with a read left in this block
  // will actually load from that register, while one with no reads left is
  // claiming it only to stop an unrelated value taking it, so the read wins.
  for (int pass = 0; pass < 2; pass++) {
    bool want_used = pass == 0;
    for (size_t i = 0; i < block->inputs.length; i++) {
      TargetInstruction* inst = block->inputs.value.p[i];
      // Target liveness conservatively places variable-register pseudos in many
      // blocks. A proven block-local variable is defined and consumed entirely
      // within one block, so it must not be re-owned on unrelated block entries.
      if (IsShortLivedVarReg(allocator, inst)) {
        continue;
      }
      if (inst->reg == NULL) {
        continue;
      }
      if (((int)inst->opcode == (int)AARCH64_OP(spill)) ||
          (inst->flags & TARGET_INST_SPILLED) != 0) {
        continue;
      }
      if ((inst->uses > 0) != want_used) {
        continue;
      }
      // An input with no remaining uses inside this block is normally dead and
      // its physical register is free for reuse.  However, if the value is also
      // live-out of the block (it appears in the block's output set, e.g. a
      // read-only loop-invariant that is consumed again on a later loop
      // iteration), its register must stay reserved for the whole block.  The
      // static use count cannot model dynamic loop iterations, so without this
      // the register would be handed to an unrelated value and clobber the
      // loop-carried one.
      if (inst->uses == 0 &&
          !BitSetContains(&block->output_ids, inst->id)) {
        continue;
      }
      // Only the first pass may take a register from another live-in value.
      if (!want_used && inst->reg->owner != NULL) {
        continue;
      }
      // Two live-in values naming the same physical register is normally the
      // value/variable pair that deliberately shares one -- either order works
      // there, they hold the same thing.  Otherwise the register really can
      // only hold one of them, and the one that claimed it while walking this
      // block's dominators is the one whose definition is nearest, so leave it
      // alone rather than pointing the register at a value that lost it.
      TargetInstruction* claimed = inst->reg->owner;
      if (claimed != NULL && claimed != inst && claimed->uses > 0 &&
          claimed->dest != inst && inst->dest != claimed) {
        continue;
      }
      inst->reg->owner = inst;
    }
  }
  ClaimRegistersNamedByProcessedHeaders(allocator, block);
}

static void ProcessBlock(TargetBasicBlock* block, void* data) {
  TrapBlock(block);
  
  // printf("Allocating registers for block %zd\n", block->block_id);
  AARCH64RegisterAllocator* allocator = data;

  // For a basic block, the inputs specify what instructions are alive
  // on entry.  An alive instruction has a register allocated to it.  All
  // other registers should be free at this point.
  InitializeBasicBlockRegisters(allocator, block);

  // In the entry block the incoming argument registers (x0..x7 / d0..d7) are
  // live from function entry until they are copied into their home registers.
  // Reserve their physical registers for the duration of the entry block so
  // that a temporary materialized before the copy (e.g. an early use of a
  // *later* parameter) cannot steal a physical register that still holds a
  // not-yet-consumed incoming argument.  Without this, lowering could emit
  // "mov x0, x2" before "mov x19, x0", clobbering the first argument.  We use
  // the reserved flag (not just ownership) because pseudo-instructions that map
  // to fixed registers - e.g. resulti -> x0 - otherwise stomp the reservation.
  AARCH64Register* reserved_args[AARCH64_NUM_INT_ARGS + AARCH64_NUM_FP_ARGS];
  int num_reserved_args = 0;
  if (block == allocator->g->base.entry_block) {
    for (int i = 0; i < AARCH64_NUM_INT_ARGS; i++) {
      TargetInstruction* arg = allocator->g->int_argument_registers[i];
      if (arg != NULL && (arg->flags & TARGET_INST_INCOMING_ARG) != 0) {
        if (arg->reg == NULL) {
          AllocateRegister(allocator, arg);
        }
        if (arg->reg != NULL && !arg->reg->reserved) {
          arg->reg->reserved = true;
          reserved_args[num_reserved_args++] = (AARCH64Register*)arg->reg;
        }
      }
    }
    for (int i = 0; i < AARCH64_NUM_FP_ARGS; i++) {
      TargetInstruction* arg = allocator->g->fp_argument_registers[i];
      if (arg != NULL && (arg->flags & TARGET_INST_INCOMING_ARG) != 0) {
        if (arg->reg == NULL) {
          AllocateRegister(allocator, arg);
        }
        if (arg->reg != NULL && !arg->reg->reserved) {
          arg->reg->reserved = true;
          reserved_args[num_reserved_args++] = (AARCH64Register*)arg->reg;
        }
      }
    }
  }

  for (TargetInstruction* inst = block->code;
       inst != NULL && inst != block->end_code;
       inst = TargetNext(inst)) {
    AllocateRegister(allocator, inst);
  }
  if (block->end_code != NULL) {
    AllocateRegister(allocator, block->end_code);
  }

  // Release the incoming-argument reservations; by the end of the entry block
  // every argument has been copied to its home register.
  for (int i = 0; i < num_reserved_args; i++) {
    reserved_args[i]->base.reserved = false;
  }
}

static void ProcessBasicBlock(AARCH64RegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->g->base, ProcessBlock,
                          kTraversePreOrder, allocator);
}

typedef struct {
  TargetInstruction* definition;
  size_t position;
  size_t next_call_position;
  uint8_t definitions;
} ShortLifetimeInfo;

// Variable-register liveness is intentionally conservative because a C
// variable can be reassigned or carried around a loop. Some lowering-created
// pseudos, however, have exactly one definition and are consumed linearly in
// that same block before any call. They are ordinary temporaries in all but
// name and can safely use a caller-saved register.
static bool HasShortBlockLocalLifetime(const ShortLifetimeInfo* info,
                                       size_t info_count,
                                       TargetInstruction* value) {
  if (!AARCH64IsVarRegister(value) || value->users.length == 0) {
    return false;
  }
  if ((size_t)value->id >= info_count) {
    return false;
  }
  const ShortLifetimeInfo* value_info = &info[value->id];
  TargetInstruction* definition = value_info->definition;
  if (value_info->definitions != 1 || definition == NULL ||
      (size_t)definition->id >= info_count) {
    return false;
  }
  TargetBasicBlock* definition_block = definition->block;
  const ShortLifetimeInfo* definition_info = &info[definition->id];

  // The definition writes the variable's register but is also a value in its
  // own right, and reads of it are recorded against it rather than against the
  // variable.  Counting only the variable's own reads then declares the
  // register free while those reads are still ahead, and the next value handed
  // the register clobbers what they were going to find there.  This is what a
  // register variable holding an address the IR also uses directly looks like
  // (`this` inside an inlined assignment operator, whose `&object` is the
  // argument of the next call as well).
  if (definition->users.length != 0) {
    return false;
  }

  size_t last_user_position = 0;
  for (size_t i = 0; i < value->users.length; i++) {
    TargetInstruction* user = value->users.value.p[i];
    if (user->block != definition_block || (size_t)user->id >= info_count) {
      return false;
    }
    size_t user_position = info[user->id].position;
    if (user_position <= definition_info->position) {
      return false;
    }
    if (user_position > last_user_position) {
      last_user_position = user_position;
    }
  }
  return definition_info->next_call_position == 0 ||
         definition_info->next_call_position >= last_user_position;
}

static void BuildShortLivedVarRegSet(AARCH64RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  size_t max_id = 0;
  bool has_var_destination = false;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if ((size_t)inst->id > max_id) {
        max_id = inst->id;
      }
      if (inst->dest != NULL && (size_t)inst->dest->id > max_id) {
        max_id = inst->dest->id;
      }
      has_var_destination |=
          inst->dest != NULL && AARCH64IsVarRegister(inst->dest);
      if (is_end) {
        break;
      }
    }
  }
  if (!has_var_destination) {
    return;
  }

  size_t info_count = max_id + 1;
  ShortLifetimeInfo* info = calloc(info_count, sizeof(*info));
  Vector candidates;
  VectorInit(&candidates);
  size_t position = 0;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      info[inst->id].position = ++position;
      if (inst->dest != NULL && AARCH64IsVarRegister(inst->dest)) {
        ShortLifetimeInfo* value_info = &info[inst->dest->id];
        if (value_info->definitions == 0) {
          value_info->definition = inst;
          VectorAppend(&candidates, inst->dest);
        }
        if (value_info->definitions < 2) {
          value_info->definitions++;
        }
      }
      if (is_end) {
        break;
      }
    }
  }

  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    size_t next_call_position = 0;
    for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
         !TargetBasicBlockIsEmpty(block) &&
         inst != TargetBasicBlockREnd(block);
         inst = TargetPrev(inst)) {
      if (AARCH64IsCall(inst)) {
        next_call_position = info[inst->id].position;
      }
      info[inst->id].next_call_position = next_call_position;
    }
  }

  for (size_t i = 0; i < candidates.length; i++) {
    TargetInstruction* value = candidates.value.p[i];
    if (HasShortBlockLocalLifetime(info, info_count, value)) {
      BitSetInsert(&allocator->short_lived_varregs, value->id);
    }
    if (value->users.length != 0) {
      BitSetInsert(&allocator->shared_varregs, value->id);
    }
  }
  VectorDestruct(&candidates);
  free(info);
}

static TargetInstruction* CreateCallResultCopy(TargetInstruction* call) {
  TargetOpcode opcode =
      (call->flags & AARCH64_INST_FP_RETURN) != 0
          ? (TargetOpcode)AARCH64_OP(fmv_d)
          : (TargetOpcode)AARCH64_OP(mv);
  return TargetNewInstruction1(opcode, call);
}

static bool SWIsNoFallthroughTerminator(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (op == AARCH64_OP(ret) || op == AARCH64_OP(br)) {
    return true;
  }
  return op == AARCH64_OP(b) && inst->operand[0] != NULL &&
         inst->operand[0]->opcode == (TargetOpcode)AARCH64_OP(al);
}

static TargetInstruction** SWBranchTargetSlot(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(b):
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz):
      return &inst->operand[1];
    case AARCH64_OP(tbz):
    case AARCH64_OP(tbnz):
      return &inst->operand[2];
    default:
      return NULL;
  }
}

static bool SWRecordReg(TargetRegister* tr, uint32_t* saved_mask,
                        uint32_t* temp_mask, uint32_t* arg_mask) {
  if (tr == NULL) {
    return true;
  }
  AARCH64Register* r = (AARCH64Register*)tr;
  if (r->type == kAARCH64RegTypeFloat) {
    return !IsSavedReg(r);
  }
  int n = r->base.num;
  if (n == AARCH64_FP_REG || n == AARCH64_LR_REG || n == AARCH64_SP_REG ||
      n == AARCH64_IP1_REG) {
    return false;
  }
  if (n >= AARCH64_INT_SAVED_START && n <= AARCH64_INT_SAVED_END) {
    *saved_mask |= 1u << (n - AARCH64_INT_SAVED_START);
  } else if (n >= AARCH64_INT_TEMP_START && n <= AARCH64_INT_TEMP_END) {
    *temp_mask |= 1u << (n - AARCH64_INT_TEMP_START);
  } else if (n >= 0 && n <= 8) {
    *arg_mask |= 1u << n;
  } else if (n == AARCH64_IP2_REG) {
    *arg_mask |= 1u << 17;
  }
  return true;
}

static bool SWIsNonExecMeta(TargetInstruction* inst) {
  if (inst->observable_checkpoint || AARCH64IsConst(inst)) {
    return true;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(label):
    case AARCH64_OP(loc):
    case AARCH64_OP(ivarreg):
    case AARCH64_OP(fvarreg):
    case AARCH64_OP(symbol):
    case AARCH64_OP(literal):
    case AARCH64_OP(oplsl):
    case AARCH64_OP(zr):
    case AARCH64_OP(resulti):
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
    case AARCH64_OP(eq):
    case AARCH64_OP(ne):
    case AARCH64_OP(cs):
    case AARCH64_OP(hs):
    case AARCH64_OP(cc):
    case AARCH64_OP(lo):
    case AARCH64_OP(mi):
    case AARCH64_OP(pl):
    case AARCH64_OP(vs):
    case AARCH64_OP(vc):
    case AARCH64_OP(hi):
    case AARCH64_OP(ls):
    case AARCH64_OP(ge):
    case AARCH64_OP(lt):
    case AARCH64_OP(gt):
    case AARCH64_OP(le):
    case AARCH64_OP(al):
      return true;
    default:
      return false;
  }
}

static bool SWCheckBlockFrameFree(TargetBasicBlock* block, bool allow_save,
                                  bool allow_reload, uint32_t* saved_mask,
                                  uint32_t* temp_mask, uint32_t* arg_mask,
                                  int* exec_count) {
  int cnt = 0;
  uint32_t sm = 0;
  uint32_t tm = 0;
  uint32_t am = 0;
  if (block == NULL || block->code == NULL) {
    return false;
  }
  for (TargetInstruction* inst = block->code; inst != NULL;
       inst = TargetNext(inst)) {
    AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
    if (allow_save &&
        (op == AARCH64_OP(save) || op == AARCH64_OP(fp) ||
         op == AARCH64_OP(sp) || op == AARCH64_OP(lr))) {
      if (inst == block->end_code) {
        break;
      }
      continue;
    }
    if (op == AARCH64_OP(spill)) {
      if (inst->reg == NULL || inst->operand[1] == NULL ||
          !TargetIsConst(inst->operand[1]) ||
          GetRegisterSize(inst) == kSize128Bit) {
        return false;
      }
      AARCH64Register* sr = (AARCH64Register*)inst->reg;
      if (sr->type != kAARCH64RegTypeInt ||
          sr->base.num < AARCH64_INT_SAVED_START ||
          sr->base.num > AARCH64_INT_SAVED_END) {
        return false;
      }
      sm |= 1u << (sr->base.num - AARCH64_INT_SAVED_START);
      if (inst == block->end_code) {
        break;
      }
      continue;
    }
    if (op == AARCH64_OP(reload)) {
      if (!allow_reload || inst->reg == NULL || inst->operand[0] == NULL ||
          inst->operand[0]->opcode != (TargetOpcode)AARCH64_OP(spill) ||
          inst->operand[0]->operand[1] == NULL ||
          !TargetIsConst(inst->operand[0]->operand[1]) ||
          GetRegisterSize(inst->operand[0]) == kSize128Bit) {
        return false;
      }
      AARCH64Register* rr = (AARCH64Register*)inst->reg;
      if (rr->type != kAARCH64RegTypeInt || rr->base.num == AARCH64_IP1_REG ||
          rr->base.num == AARCH64_IP2_REG ||
          !SWRecordReg(inst->reg, &sm, &tm, &am)) {
        return false;
      }
      cnt++;
      if (inst == block->end_code) {
        break;
      }
      continue;
    }
    if (!SWIsNonExecMeta(inst)) {
      switch (op) {
        case AARCH64_OP(bl):
        case AARCH64_OP(blr):
        case AARCH64_OP(br):
        case AARCH64_OP(save):
        case AARCH64_OP(restore):
        case AARCH64_OP(ret):
        case AARCH64_OP(asm):
        case AARCH64_OP(named_label):
        case AARCH64_OP(tp):
        case AARCH64_OP(fp):
        case AARCH64_OP(sp):
        case AARCH64_OP(lr):
        case AARCH64_OP(xr):
        case AARCH64_OP(r9):
        case AARCH64_OP(structreturn):
        case AARCH64_OP(regarg):
        case AARCH64_OP(nrvoval):
        case AARCH64_OP(atomic_load):
        case AARCH64_OP(atomic_store):
        case AARCH64_OP(atomic_fetch_add):
        case AARCH64_OP(atomic_fetch_sub):
        case AARCH64_OP(atomic_add_fetch):
        case AARCH64_OP(atomic_sub_fetch):
        case AARCH64_OP(atomic_compare_exchange_bool):
        case AARCH64_OP(atomic_compare_exchange_val):
        case AARCH64_OP(atomic_compare_exchange_n):
        case AARCH64_OP(atomic_fence):
        case AARCH64_OP(ldxr):
        case AARCH64_OP(ldaxr):
        case AARCH64_OP(stxr):
        case AARCH64_OP(stlxr):
        case AARCH64_OP(clrex):
        case AARCH64_OP(dmb):
          return false;
        default:
          break;
      }
      cnt++;
      if (!SWRecordReg(inst->reg, &sm, &tm, &am)) {
        return false;
      }
      if (inst->dest != NULL && !SWRecordReg(inst->dest->reg, &sm, &tm, &am)) {
        return false;
      }
      for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
        TargetInstruction* o = inst->operand[k];
        if (o != NULL) {
          TargetRegister* oreg =
              o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL);
          if (!SWRecordReg(oreg, &sm, &tm, &am)) {
            return false;
          }
        }
      }
    }
    if (inst == block->end_code) {
      break;
    }
  }
  *saved_mask = sm;
  *temp_mask = tm;
  *arg_mask = am;
  *exec_count = cnt;
  return true;
}

static int SWPopCount(uint32_t x) {
  int c = 0;
  while (x != 0) {
    c += (int)(x & 1u);
    x >>= 1;
  }
  return c;
}

static TargetInstruction* SWEnsureBlockLabel(TargetGenerator* gen,
                                             TargetBasicBlock* block) {
  for (TargetInstruction* p = block->code; p != NULL; p = TargetNext(p)) {
    if (p->opcode == (TargetOpcode)AARCH64_OP(label)) {
      return p;
    }
    if (!SWIsNonExecMeta(p) || p == block->end_code) {
      break;
    }
  }
  TargetInstruction* lbl = TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
  lbl->block = block;
  TargetEmitBefore(gen, lbl, block->code);
  block->code = lbl;
  return lbl;
}

static TargetInstruction* SWAppendBlockInst(TargetGenerator* gen,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            TargetInstruction* cursor) {
  inst->block = block;
  ListInsertAfter(&gen->code, &inst->header, &cursor->header);
  if (block->code == NULL) {
    block->code = inst;
  }
  block->end_code = inst;
  return inst;
}

static TargetInstruction* SWNewRegProxy(TargetGenerator* gen,
                                        AARCH64Register* reg, int size) {
  TargetInstruction* p = TargetNewInstruction(TARGET_OP(ivarreg));
  p->reg = &reg->base;
  p->block = gen->entry_block;
  p->flags = TARGET_INST_PROCESSED | ((size & 3) << 16);
  TargetTrackOrphanInstruction(gen, p);
  return p;
}

static void ShrinkWrapFunction(AARCH64RegisterAllocator* allocator) {
  AARCH64Generator* g = allocator->g;
  TargetGenerator* gen = &g->base;
  if (!OptLevel2() || gen->varargs || g->moves_sp ||
      g->saved_regs.length != 0 || g->struct_return_reg >= 0 ||
      gen->exception_edges.length != 0 || gen->entry_block == NULL ||
      gen->entry_block->in_edges.length != 0) {
    return;
  }
  bool has_frame = gen->stack_frame_size > 0 || gen->num_calls > 0 ||
                   g->not_leaf ||
                   BitSetCount(&allocator->used_int_regs) > 0 ||
                   BitSetCount(&allocator->used_float_regs) > 0 ||
                   allocator->max_spilled_region_size > 0;
  if (!has_frame) {
    return;
  }

  // Locate the unique `save` in entry_block and unique `restore`+`ret`
  // epilogue block `E` (any additional `restore` blocks must be tail calls
  // that stay in the slow region).
  TargetInstruction* save_inst = NULL;
  TargetBasicBlock* epilogue = NULL;
  int save_count = 0;
  int ret_epilogue_count = 0;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    bool blk_has_restore = false;
    bool blk_has_ret = false;
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->opcode == (TargetOpcode)AARCH64_OP(save)) {
        save_count++;
        if (blk == gen->entry_block) {
          save_inst = p;
        }
      } else if (p->opcode == (TargetOpcode)AARCH64_OP(restore)) {
        blk_has_restore = true;
      } else if (p->opcode == (TargetOpcode)AARCH64_OP(ret)) {
        blk_has_ret = true;
      }
      if (p == blk->end_code) {
        break;
      }
    }
    if (blk_has_restore && blk_has_ret) {
      ret_epilogue_count++;
      epilogue = blk;
    } else if (blk_has_ret && !blk_has_restore) {
      return;
    }
  }
  if (save_count != 1 || save_inst == NULL || ret_epilogue_count != 1 ||
      epilogue == NULL || epilogue == gen->entry_block) {
    return;
  }

  // Verify epilogue ends with restore + ret; if epilogue also has frame-free
  // instructions before `restore` (e.g. `pc += 3` in void functions), split
  // `epilogue` right before `restore` into a fallthrough block + pure epilogue.
  TargetInstruction* restore_inst = NULL;
  bool seen_restore = false;
  bool seen_ret = false;
  bool has_pre_restore_exec = false;
  for (TargetInstruction* p = epilogue->code; p != NULL; p = TargetNext(p)) {
    AARCH64Opcode op = (AARCH64Opcode)p->opcode;
    if (op == AARCH64_OP(restore) && !seen_restore && !seen_ret) {
      seen_restore = true;
      restore_inst = p;
    } else if (op == AARCH64_OP(ret) && seen_restore && !seen_ret) {
      seen_ret = true;
    } else if (!SWIsNonExecMeta(p)) {
      if (seen_restore) {
        return;
      }
      has_pre_restore_exec = true;
    }
    if (p == epilogue->end_code) {
      break;
    }
  }
  if (!seen_restore || !seen_ret || restore_inst == NULL) {
    return;
  }
  if (has_pre_restore_exec && restore_inst != epilogue->code) {
    TargetBasicBlock* pre_blk = epilogue;
    TargetInstruction* pre_end = TargetPrev(restore_inst);
    if (pre_end == NULL || pre_end->block != pre_blk) {
      return;
    }
    TargetBasicBlock* pure_epilogue =
        NewTargetBasicBlock(gen->basic_blocks.length);
    VectorAppend(&gen->basic_blocks, pure_epilogue);
    pure_epilogue->code = restore_inst;
    pure_epilogue->end_code = pre_blk->end_code;
    pre_blk->end_code = pre_end;
    for (TargetInstruction* p = pure_epilogue->code; p != NULL;
         p = TargetNext(p)) {
      p->block = pure_epilogue;
      if (p == pure_epilogue->end_code) {
        break;
      }
    }
    for (size_t o = 0; o < pre_blk->out_edges.length; o++) {
      size_t succ_id = (size_t)pre_blk->out_edges.value.w[o];
      VectorAppend(&pure_epilogue->out_edges, (void*)(uintptr_t)succ_id);
      if (succ_id < gen->basic_blocks.length) {
        TargetBasicBlock* succ = gen->basic_blocks.value.p[succ_id];
        if (succ != NULL) {
          for (size_t e = 0; e < succ->in_edges.length; e++) {
            if ((size_t)succ->in_edges.value.w[e] == pre_blk->block_id) {
              succ->in_edges.value.w[e] = (intptr_t)pure_epilogue->block_id;
            }
          }
        }
      }
    }
    pre_blk->out_edges.length = 0;
    VectorAppend(&pre_blk->out_edges,
                 (void*)(uintptr_t)pure_epilogue->block_id);
    VectorAppend(&pure_epilogue->in_edges,
                 (void*)(uintptr_t)pre_blk->block_id);
    BitSetCopy(&pure_epilogue->dominators, &pre_blk->dominators);
    BitSetInsert(&pure_epilogue->dominators, pure_epilogue->block_id);
    pure_epilogue->idom = pre_blk;
    epilogue = pure_epilogue;
  }

  // Phase 1: 100% read-only region analysis.  Nothing in `gen` is mutated
  // until every eligibility, fallthrough, spill-sinking, exit-count, and
  // register-budget check has succeeded.
  size_t orig_blocks = gen->basic_blocks.length;
  size_t cap = orig_blocks + 32;
  bool* eligible_pure = calloc(cap, sizeof(bool));
  bool* eligible_clone = calloc(cap, sizeof(bool));
  bool* can_reach_ret = calloc(cap, sizeof(bool));
  bool* sim_pure_fast = calloc(cap, sizeof(bool));
  bool* sim_clone = calloc(cap, sizeof(bool));
  bool* in_fast = calloc(cap, sizeof(bool));
  uint32_t* blk_saved = calloc(cap, sizeof(uint32_t));
  uint32_t* blk_temp = calloc(cap, sizeof(uint32_t));
  uint32_t* blk_arg = calloc(cap, sizeof(uint32_t));
  int* blk_exec = calloc(cap, sizeof(int));
  TargetBasicBlock** fall_succ = calloc(cap, sizeof(TargetBasicBlock*));
  TargetBasicBlock** clone_of = calloc(cap, sizeof(TargetBasicBlock*));

  uint32_t incoming_arg_mask = 0;
  for (int k = 0; k < AARCH64_NUM_INT_ARGS; k++) {
    if (g->int_argument_registers[k] != NULL) {
      incoming_arg_mask |= 1u << k;
    }
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk == epilogue || blk == gen->exit_block ||
        blk->code == NULL) {
      continue;
    }
    uint32_t sm = 0, tm = 0, am = 0;
    int ec = 0;
    eligible_pure[b] = SWCheckBlockFrameFree(blk, blk == gen->entry_block,
                                             /*allow_reload=*/false, &sm, &tm,
                                             &am, &ec);
    eligible_clone[b] =
        eligible_pure[b] ||
        (blk != gen->entry_block &&
         SWCheckBlockFrameFree(blk, /*allow_save=*/false,
                               /*allow_reload=*/true, &sm, &tm, &am, &ec));
    if (eligible_clone[b]) {
      blk_saved[b] = sm;
      blk_temp[b] = tm;
      blk_arg[b] = am;
      blk_exec[b] = ec;
    }
    if (!SWIsNoFallthroughTerminator(blk->end_code)) {
      for (TargetInstruction* nxt = TargetNext(blk->end_code); nxt != NULL;
           nxt = TargetNext(nxt)) {
        if (nxt->block != NULL && nxt->block != blk) {
          fall_succ[b] = nxt->block;
          break;
        }
      }
    }
  }

  // Detect incoming-arg `mov r_k -> varreg_c` copies in `entry_block` that can
  // coalesce `c` onto `x_k` in `F`.
  int arg_coalesce_for_saved[10];
  for (int i = 0; i < 10; i++) {
    arg_coalesce_for_saved[i] = -1;
  }
  uint32_t written_before = 0;
  for (TargetInstruction* p = gen->entry_block->code; p != NULL;
       p = TargetNext(p)) {
    if (SWIsNonExecMeta(p)) {
      if (p == gen->entry_block->end_code) {
        break;
      }
      continue;
    }
    if ((p->opcode == (TargetOpcode)AARCH64_OP(mov) ||
         p->opcode == (TargetOpcode)AARCH64_OP(mv)) &&
        p->dest != NULL && p->dest->reg != NULL && p->operand[0] != NULL &&
        p->operand[1] == NULL &&
        p->operand[0]->opcode >= (TargetOpcode)AARCH64_OP(r0) &&
        p->operand[0]->opcode <= (TargetOpcode)AARCH64_OP(r7) &&
        p->operand[0]->users.length == 1) {
      int k = (int)p->operand[0]->opcode - (int)AARCH64_OP(r0);
      AARCH64Register* dr = (AARCH64Register*)p->dest->reg;
      if (dr->type == kAARCH64RegTypeInt &&
          dr->base.num >= AARCH64_INT_SAVED_START &&
          dr->base.num <= AARCH64_INT_SAVED_END &&
          (k > 0 || gen->is_void) && (written_before & (1u << k)) == 0) {
        int sbit = dr->base.num - AARCH64_INT_SAVED_START;
        if (arg_coalesce_for_saved[sbit] < 0) {
          arg_coalesce_for_saved[sbit] = k;
        }
      }
    }
    uint32_t d1 = 0, d2 = 0, am = 0;
    SWRecordReg(p->reg, &d1, &d2, &am);
    if (p->dest != NULL) {
      SWRecordReg(p->dest->reg, &d1, &d2, &am);
    }
    written_before |= am;
    if (p == gen->entry_block->end_code) {
      break;
    }
  }

  // Upper bound on caller-saved scratch registers available for `F`.
  int extra_scratch = 0;
  for (int k = 1; k <= 7; k++) {
    if ((incoming_arg_mask & (1u << k)) == 0) {
      extra_scratch++;
    }
  }
  if (gen->stack_frame_size < 0x100000) {
    extra_scratch++;
  }
  extra_scratch++;  // x17 when unused in F (checked via total_arg below).
  int coalesced_args = 0;
  for (int i = 0; i < 10; i++) {
    if (arg_coalesce_for_saved[i] >= 0) {
      coalesced_args++;
    }
  }
  int max_scratch_pool = 7 + extra_scratch + coalesced_args;

  // Only grow `F` into blocks that have a frame-free path to `epilogue`.
  bool reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      if (!eligible_clone[b] || can_reach_ret[b]) {
        continue;
      }
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == epilogue->block_id ||
            (succ_id < orig_blocks && can_reach_ret[succ_id])) {
          can_reach_ret[b] = true;
          reach_prog = true;
          break;
        }
      }
    }
  }

  bool has_fast_return = false;
  size_t num_slow_targets = 0;
  int num_sunk_slots = 0;
  int num_mapped = 0;
  int rcode = 0;
  size_t entry_id = gen->entry_block->block_id;
  if (!eligible_pure[entry_id] || !can_reach_ret[entry_id]) {
    rcode = 1;
    goto cleanup;
  }
  uint32_t total_saved = blk_saved[entry_id];
  uint32_t total_temp = blk_temp[entry_id];
  uint32_t total_arg = blk_arg[entry_id];
  if (SWPopCount(total_saved) > max_scratch_pool - SWPopCount(total_temp)) {
    goto cleanup;
  }
  sim_pure_fast[entry_id] = true;

  int cloned_insts = 0;
  int num_clones = 0;
  bool progress = true;
  while (progress) {
    progress = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      if (blk == NULL || sim_pure_fast[b] || sim_clone[b] ||
          !eligible_pure[b] || !can_reach_ret[b] ||
          blk->in_edges.length == 0) {
        continue;
      }
      bool all_pure_fast = true;
      for (size_t e = 0; e < blk->in_edges.length; e++) {
        size_t pred_id = (size_t)blk->in_edges.value.w[e];
        if (pred_id >= orig_blocks || !sim_pure_fast[pred_id]) {
          all_pure_fast = false;
          break;
        }
      }
      if (all_pure_fast) {
        if (b != entry_id) {
          TargetInstruction* prev = TargetPrev(blk->code);
          if (prev != NULL && prev->block != NULL &&
              (prev->block->block_id >= orig_blocks ||
               !sim_pure_fast[prev->block->block_id]) &&
              !SWIsNoFallthroughTerminator(prev)) {
            continue;
          }
        }
        uint32_t ns = total_saved | blk_saved[b];
        uint32_t nt = total_temp | blk_temp[b];
        if (SWPopCount(ns) <= max_scratch_pool - SWPopCount(nt)) {
          sim_pure_fast[b] = true;
          total_saved = ns;
          total_temp = nt;
          total_arg |= blk_arg[b];
          progress = true;
        }
      }
    }
    if (progress) {
      continue;
    }
    // The cheapest block first, so that the budget completes as many paths
    // to the epilogue as it can, and blocks inside loops last: cloning one
    // only peels the loop's first test.
    for (int pass = 0; pass < 2 && !progress; pass++) {
      size_t best = orig_blocks;
      for (size_t b = 0; b < orig_blocks && num_clones < 24; b++) {
        TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
        if (blk == NULL || sim_pure_fast[b] || sim_clone[b] ||
            (pass == 0 && blk->loop_nesting > 0) || !eligible_clone[b] ||
            !can_reach_ret[b] || blk_exec[b] > 22 ||
            cloned_insts + blk_exec[b] > 40 || blk->in_edges.length == 0) {
          continue;
        }
        bool has_fast_pred = false;
        bool has_slow_pred = false;
        bool forward_only = true;
        for (size_t e = 0; e < blk->in_edges.length; e++) {
          size_t pred_id = (size_t)blk->in_edges.value.w[e];
          if (pred_id < orig_blocks &&
              (sim_pure_fast[pred_id] || sim_clone[pred_id])) {
            has_fast_pred = true;
            if (pred_id >= b) {
              forward_only = false;
              break;
            }
            if (sim_clone[pred_id] || !eligible_pure[b]) {
              has_slow_pred = true;
            }
          } else {
            has_slow_pred = true;
          }
        }
        if (!has_fast_pred || !has_slow_pred || !forward_only) {
          continue;
        }
        uint32_t ns = total_saved | blk_saved[b];
        uint32_t nt = total_temp | blk_temp[b];
        if (SWPopCount(ns) > max_scratch_pool - SWPopCount(nt)) {
          continue;
        }
        if (best == orig_blocks || blk_exec[b] < blk_exec[best]) {
          best = b;
        }
      }
      if (best < orig_blocks) {
        sim_clone[best] = true;
        total_saved |= blk_saved[best];
        total_temp |= blk_temp[best];
        total_arg |= blk_arg[best];
        cloned_insts += blk_exec[best];
        num_clones++;
        progress = true;
      }
    }
  }

  // Prune any block in `sim_pure_fast` / `sim_clone` that cannot reach
  // `epilogue` solely through `sim_pure_fast` / `sim_clone` blocks.
  bool* reaches_fast_ret = calloc(orig_blocks, sizeof(bool));
  reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      if ((!sim_pure_fast[b] && !sim_clone[b]) || reaches_fast_ret[b]) {
        continue;
      }
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == epilogue->block_id ||
            (succ_id < orig_blocks && reaches_fast_ret[succ_id])) {
          reaches_fast_ret[b] = true;
          reach_prog = true;
          break;
        }
      }
    }
  }
  total_saved = 0;
  total_temp = 0;
  total_arg = 0;
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!reaches_fast_ret[b]) {
      sim_pure_fast[b] = false;
      sim_clone[b] = false;
    } else {
      total_saved |= blk_saved[b];
      total_temp |= blk_temp[b];
      total_arg |= blk_arg[b];
    }
  }
  free(reaches_fast_ret);
  if (!sim_pure_fast[entry_id]) {
    goto cleanup;
  }

  // Validate fast returns, slow boundary exits, fallthroughs, and temp regs.
  TargetBasicBlock* slow_targets[16];
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b] && !sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (size_t o = 0; o < blk->out_edges.length; o++) {
      size_t succ_id = (size_t)blk->out_edges.value.w[o];
      TargetBasicBlock* succ = gen->basic_blocks.value.p[succ_id];
      if (succ == NULL || succ == gen->exit_block) {
        continue;
      }
      if (succ == epilogue) {
        has_fast_return = true;
      } else if (succ_id >= orig_blocks ||
                 (!sim_pure_fast[succ_id] && !sim_clone[succ_id])) {
        bool seen = false;
        for (size_t s = 0; s < num_slow_targets; s++) {
          if (slow_targets[s] == succ) {
            seen = true;
            break;
          }
        }
        if (!seen) {
          if (num_slow_targets >= 16) {
            goto cleanup;
          }
          slow_targets[num_slow_targets++] = succ;
        }
      }
    }
  }
  if (!has_fast_return || num_slow_targets == 0) {
    goto cleanup;
  }

  // Reject when `entry_block` immediately exits to `Slow` unless a 64-bit
  // pointer is NULL (e.g. `ChargeCycles`: `if (bbc != NULL) { ... calls ... }
  // UpdatePace(...)`): the NULL path is cold, so the 100%-taken non-NULL path
  // would pay extra register-transfer instructions on every call.
  if (gen->entry_block->out_edges.length == 2) {
    size_t o0 = (size_t)gen->entry_block->out_edges.value.w[0];
    size_t o1 = (size_t)gen->entry_block->out_edges.value.w[1];
    bool f0 = o0 < orig_blocks && (sim_pure_fast[o0] || sim_clone[o0]);
    bool f1 = o1 < orig_blocks && (sim_pure_fast[o1] || sim_clone[o1]);
    if (f0 != f1) {
      TargetInstruction* last_cmp = NULL;
      for (TargetInstruction* p = gen->entry_block->code; p != NULL;
           p = TargetNext(p)) {
        if (p->opcode == (TargetOpcode)AARCH64_OP(cmp)) {
          last_cmp = p;
        }
        if (p == gen->entry_block->end_code) {
          break;
        }
      }
      TargetInstruction* end = gen->entry_block->end_code;
      bool ptr_zero_cmp =
          (end != NULL &&
           (end->opcode == (TargetOpcode)AARCH64_OP(cbz) ||
            end->opcode == (TargetOpcode)AARCH64_OP(cbnz)) &&
           GetRegisterSize(end) == kSize64Bit) ||
          (last_cmp != NULL && GetRegisterSize(last_cmp) == kSize64Bit &&
           last_cmp->operand[1] != NULL &&
           (last_cmp->operand[1]->opcode == (TargetOpcode)AARCH64_OP(zr) ||
            (TargetIsConst(last_cmp->operand[1]) &&
             TargetIntValue(last_cmp->operand[1]) == 0)));
      if (ptr_zero_cmp) {
        goto cleanup;
      }
    }
  }

  // Skip shrink-wrapping when the fast region only contains defensive null /
  // zero sanity guards (`ptr == NULL`, `n <= 0`, `!running`): those guards are
  // virtually never taken at runtime, so copying registers into and out of
  // scratch registers on the 100%-taken body would cost more than it saves.
  bool has_nontrivial_fast_work = false;
  for (size_t b = 0; b < orig_blocks && !has_nontrivial_fast_work; b++) {
    if (!sim_pure_fast[b] && !sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      AARCH64Opcode op = (AARCH64Opcode)p->opcode;
      if (AARCH64IsStore(p) || op == AARCH64_OP(tst) ||
          op == AARCH64_OP(and) || op == AARCH64_OP(ands) ||
          op == AARCH64_OP(sub) || op == AARCH64_OP(subs)) {
        has_nontrivial_fast_work = true;
        break;
      }
      if (op == AARCH64_OP(cmp) && p->operand[1] != NULL) {
        if (TargetIsConst(p->operand[1])) {
          if (TargetIntValue(p->operand[1]) != 0) {
            has_nontrivial_fast_work = true;
            break;
          }
        } else if (p->operand[1]->opcode != (TargetOpcode)AARCH64_OP(zr)) {
          has_nontrivial_fast_work = true;
          break;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  if (!has_nontrivial_fast_work) {
    goto cleanup;
  }

  for (size_t i = 0; i < num_slow_targets / 2; i++) {
    TargetBasicBlock* tmp = slow_targets[i];
    slow_targets[i] = slow_targets[num_slow_targets - 1 - i];
    slow_targets[num_slow_targets - 1 - i] = tmp;
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b] || b == entry_id) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* prev = TargetPrev(blk->code);
    if (prev != NULL && prev->block != NULL &&
        (prev->block->block_id >= orig_blocks ||
         !sim_pure_fast[prev->block->block_id]) &&
        !SWIsNoFallthroughTerminator(prev)) {
      goto cleanup;
    }
  }

  // Validate every `spill` and `reload` in `F`:
  // - At most 16 sunk spill slots, at most one distinct slot per saved reg `c`.
  // - Along every path in `F` from `sp` to any slow target that can reach a
  //   reload of `sp`'s slot (and to any `sim_clone` reload of `sp`'s slot),
  //   register `c` is never overwritten after `sp`.
  int64_t sunk_slot_off[16];
  int sunk_slot_reg[16];
  TargetInstruction* sunk_slot_proto[16];
  int saved_reg_slot_idx[10];
  for (int i = 0; i < 10; i++) {
    saved_reg_slot_idx[i] = -1;
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b] && !sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->opcode == (TargetOpcode)AARCH64_OP(spill)) {
        int c = p->reg->num;
        int sbit = c - AARCH64_INT_SAVED_START;
        int64_t soff = TargetIntValue(p->operand[1]);
        int idx = -1;
        for (int s = 0; s < num_sunk_slots; s++) {
          if (sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx >= 0) {
          goto cleanup;
        } else {
          if (num_sunk_slots >= 16 || saved_reg_slot_idx[sbit] >= 0) {
            goto cleanup;
          }
          idx = num_sunk_slots++;
          sunk_slot_off[idx] = soff;
          sunk_slot_reg[idx] = c;
          sunk_slot_proto[idx] = p;
          saved_reg_slot_idx[sbit] = idx;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  for (int idx = 0; idx < num_sunk_slots; idx++) {
    int64_t soff = sunk_slot_off[idx];
    int c = sunk_slot_reg[idx];
    TargetInstruction* proto = sunk_slot_proto[idx];
    // Every block containing a reload of `soff` must be dominated by `proto`'s
    // block, and along every block in `F` reachable from `proto` that can reach
    // such a reload, register `c` must not be clobbered after `proto`.
    bool clobbered = false;
    bool* reaches_reload = calloc(orig_blocks, sizeof(bool));
    for (size_t b = 0; b < orig_blocks && !clobbered; b++) {
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      if (blk == NULL || blk->code == NULL) {
        continue;
      }
      for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
        if (p->opcode == (TargetOpcode)AARCH64_OP(reload) &&
            p->operand[0] != NULL && p->operand[0]->operand[1] != NULL &&
            TargetIsConst(p->operand[0]->operand[1]) &&
            TargetIntValue(p->operand[0]->operand[1]) == soff) {
          if (!BitSetContains(&blk->dominators, proto->block->block_id)) {
            clobbered = true;
            break;
          }
          reaches_reload[b] = true;
          break;
        }
        if (p == blk->end_code) {
          break;
        }
      }
    }
    if (!clobbered) {
      bool rprog = true;
      while (rprog) {
        rprog = false;
        for (size_t b = 0; b < orig_blocks; b++) {
          if (reaches_reload[b] || b == proto->block->block_id) {
            continue;
          }
          TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
          if (blk == NULL) {
            continue;
          }
          for (size_t o = 0; o < blk->out_edges.length; o++) {
            size_t sid = (size_t)blk->out_edges.value.w[o];
            if (sid < orig_blocks && reaches_reload[sid]) {
              reaches_reload[b] = true;
              rprog = true;
              break;
            }
          }
        }
      }
      BitSet after_proto;
      BitSetInit(&after_proto);
      TargetBasicBlockReachableAfter(gen, proto->block, &after_proto);
      for (size_t b = 0; b < orig_blocks && !clobbered; b++) {
        if (!sim_pure_fast[b] && !sim_clone[b]) {
          continue;
        }
        bool is_proto_blk = (b == proto->block->block_id);
        if (!is_proto_blk &&
            (!BitSetContains(&after_proto, b) || !reaches_reload[b])) {
          continue;
        }
        TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
        bool past_proto = !is_proto_blk;
        for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
          if (p == proto) {
            past_proto = true;
          } else if (past_proto &&
                     p->opcode != (TargetOpcode)AARCH64_OP(spill) &&
                     !SWIsNonExecMeta(p)) {
            TargetRegister* wreg =
                p->dest != NULL ? p->dest->reg : p->reg;
            if (wreg == &allocator->int_regs[c].base) {
              if (!AARCH64IsVarRegister(proto->operand[0]) ||
                  p->dest != proto->operand[0]) {
                clobbered = true;
                break;
              }
            }
          }
          if (p == blk->end_code) {
            break;
          }
        }
      }
      BitSetDestruct(&after_proto);
    }
    free(reaches_reload);
    if (clobbered) {
      goto cleanup;
    }
  }

  // Verify every `reload` in `sim_clone` blocks maps to a validated sunk slot
  // whose `sp` dominates that block.
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->opcode == (TargetOpcode)AARCH64_OP(reload)) {
        int64_t soff = TargetIntValue(p->operand[0]->operand[1]);
        int idx = -1;
        for (int s = 0; s < num_sunk_slots; s++) {
          if (sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx < 0 ||
            !BitSetContains(&blk->dominators,
                            sunk_slot_proto[idx]->block->block_id)) {
          goto cleanup;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  // Assign each callee-saved int register used in `F` to a caller-saved
  // register: coalesce incoming-argument copies first (`x0..x7`), then free
  // `x15..x9`, then unused `x7..x1`, `x8`, `x17`.
  int saved_list[10];
  int temp_list[10];
  uint32_t taken_caller = 0;
  for (int t = AARCH64_INT_TEMP_START; t <= AARCH64_INT_TEMP_END; t++) {
    if ((total_temp & (1u << (t - AARCH64_INT_TEMP_START))) != 0) {
      taken_caller |= 1u << t;
    }
  }
  for (int bit = 0; bit <= AARCH64_INT_SAVED_END - AARCH64_INT_SAVED_START;
       bit++) {
    if ((total_saved & (1u << bit)) == 0) {
      continue;
    }
    int k = arg_coalesce_for_saved[bit];
    if (k >= 0 && (taken_caller & (1u << k)) == 0) {
      // Check that no instruction in `F` other than the single incoming-arg
      // move in `entry_block` uses `x_k`.
      bool k_used_elsewhere = false;
      for (size_t b = 0; b < orig_blocks && !k_used_elsewhere; b++) {
        if (!sim_pure_fast[b] && !sim_clone[b]) {
          continue;
        }
        if (b != entry_id && (blk_arg[b] & (1u << k)) != 0) {
          k_used_elsewhere = true;
          break;
        }
      }
      if (!k_used_elsewhere) {
        int count_k_in_entry = 0;
        for (TargetInstruction* p = gen->entry_block->code; p != NULL;
             p = TargetNext(p)) {
          if (SWIsNonExecMeta(p) ||
              p->opcode == (TargetOpcode)AARCH64_OP(spill)) {
            if (p == gen->entry_block->end_code) {
              break;
            }
            continue;
          }
          uint32_t d1 = 0, d2 = 0, am = 0;
          SWRecordReg(p->reg, &d1, &d2, &am);
          if (p->dest != NULL) {
            SWRecordReg(p->dest->reg, &d1, &d2, &am);
          }
          for (int opi = 0; opi < TARGET_MAX_OPERANDS; opi++) {
            TargetInstruction* o = p->operand[opi];
            if (o != NULL) {
              SWRecordReg(
                  o->reg != NULL ? o->reg
                                 : (o->dest != NULL ? o->dest->reg : NULL),
                  &d1, &d2, &am);
            }
          }
          if ((am & (1u << k)) != 0) {
            count_k_in_entry++;
          }
          if (p == gen->entry_block->end_code) {
            break;
          }
        }
        if (count_k_in_entry == 1) {
          saved_list[num_mapped] = AARCH64_INT_SAVED_START + bit;
          temp_list[num_mapped] = k;
          taken_caller |= 1u << k;
          num_mapped++;
        }
      }
    }
  }

  int candidate_pool[16];
  int pool_len = 0;
  for (int t = AARCH64_INT_TEMP_END; t >= AARCH64_INT_TEMP_START; t--) {
    if ((taken_caller & (1u << t)) == 0) {
      candidate_pool[pool_len++] = t;
    }
  }
  for (int k = 7; k >= 1; k--) {
    if ((incoming_arg_mask & (1u << k)) == 0 &&
        (total_arg & (1u << k)) == 0 && (taken_caller & (1u << k)) == 0) {
      candidate_pool[pool_len++] = k;
    }
  }
  if (gen->stack_frame_size < 0x100000 && (total_arg & (1u << 8)) == 0 &&
      (taken_caller & (1u << 8)) == 0) {
    candidate_pool[pool_len++] = 8;
  }
  if ((total_arg & (1u << 17)) == 0 && (taken_caller & (1u << 17)) == 0) {
    candidate_pool[pool_len++] = 17;
  }

  int pool_pos = 0;
  for (int bit = 0; bit <= AARCH64_INT_SAVED_END - AARCH64_INT_SAVED_START;
       bit++) {
    if ((total_saved & (1u << bit)) == 0) {
      continue;
    }
    int c = AARCH64_INT_SAVED_START + bit;
    bool already = false;
    for (int m = 0; m < num_mapped; m++) {
      if (saved_list[m] == c) {
        already = true;
        break;
      }
    }
    if (already) {
      continue;
    }
    if (pool_pos >= pool_len) {
      goto cleanup;
    }
    saved_list[num_mapped] = c;
    temp_list[num_mapped] = candidate_pool[pool_pos++];
    num_mapped++;
  }

  // Phase 2: All checks passed.  Materialize clones and apply shrink-wrap.
  TargetInstruction* al_cond = NULL;
  for (TargetInstruction* p = TargetFirstInstruction(gen); p != NULL;
       p = TargetNext(p)) {
    if (p->opcode == (TargetOpcode)AARCH64_OP(al)) {
      al_cond = p;
      break;
    }
  }
  if (al_cond == NULL) {
    al_cond = TargetNewInstruction((TargetOpcode)AARCH64_OP(al));
    al_cond->block = gen->entry_block;
    al_cond->flags = TARGET_INST_PROCESSED;
    TargetEmitBefore(gen, al_cond, gen->entry_block->code);
    gen->entry_block->code = al_cond;
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (sim_pure_fast[b]) {
      in_fast[b] = true;
    }
  }

  // Create clone blocks for every `sim_clone[b]` first so inter-clone branches
  // and fallthroughs can resolve to each other.
  TargetInstruction* orig_clone_lbl[32] = {0};
  TargetInstruction* new_clone_lbl[32] = {0};
  size_t clone_src_ids[32] = {0};
  int clone_idx = 0;
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* orig_lbl = SWEnsureBlockLabel(gen, blk);
    TargetBasicBlock* clone = NewTargetBasicBlock(gen->basic_blocks.length);
    size_t cid = clone->block_id;
    VectorAppend(&gen->basic_blocks, clone);
    TargetInstruction* clone_lbl =
        TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
    clone_lbl->block = clone;
    clone_lbl->flags = TARGET_INST_PROCESSED;
    TargetEmit(gen, clone_lbl);
    clone->code = clone_lbl;
    clone->end_code = clone_lbl;
    TargetInstruction* cur = clone_lbl;
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != orig_lbl && p->opcode != (TargetOpcode)AARCH64_OP(label) &&
          p->opcode != (TargetOpcode)AARCH64_OP(loc) &&
          p->opcode != (TargetOpcode)AARCH64_OP(spill) &&
          !p->observable_checkpoint) {
        TargetInstruction* cp;
        if (p->opcode == (TargetOpcode)AARCH64_OP(reload)) {
          int64_t soff = TargetIntValue(p->operand[0]->operand[1]);
          int src_c = -1;
          for (int s = 0; s < num_sunk_slots; s++) {
            if (sunk_slot_off[s] == soff) {
              src_c = sunk_slot_reg[s];
              break;
            }
          }
          assert(src_c >= 0);
          int rsz = GetRegisterSize(p) == kSize32Bit ? kSize32Bit : kSize64Bit;
          TargetInstruction* src_proxy =
              SWNewRegProxy(gen, &allocator->int_regs[src_c], rsz);
          cp = SetInstructionSize(
              TargetNewInstruction1((TargetOpcode)AARCH64_OP(mov), src_proxy),
              rsz);
          cp->reg = p->reg;
          cp->flags |= TARGET_INST_PROCESSED;
        } else {
          cp = TargetNewInstruction(p->opcode);
          cp->flags = p->flags;
          cp->reg = p->reg;
          cp->dest = p->dest;
          for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
            cp->operand[k] = p->operand[k];
          }
        }
        cur = SWAppendBlockInst(gen, clone, cp, cur);
      }
      if (p == blk->end_code) {
        break;
      }
    }
    clone_of[b] = clone;
    in_fast[cid] = true;
    fall_succ[cid] = fall_succ[b];
    orig_clone_lbl[clone_idx] = orig_lbl;
    new_clone_lbl[clone_idx] = clone_lbl;
    clone_src_ids[clone_idx] = b;
    clone_idx++;
  }

  // Redirect branches and fallthroughs among all fast blocks (both pure-fast
  // and cloned) from `orig_clone_lbl[i]` -> `new_clone_lbl[i]`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = SWBranchTargetSlot(p);
      if (slot != NULL && *slot != NULL) {
        for (int c = 0; c < clone_idx; c++) {
          if (*slot == orig_clone_lbl[c]) {
            *slot = new_clone_lbl[c];
            break;
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
    if (fall_succ[b] != NULL && fall_succ[b]->block_id < orig_blocks &&
        clone_of[fall_succ[b]->block_id] != NULL) {
      fall_succ[b] = clone_of[fall_succ[b]->block_id];
    }
  }

  // Materialize explicit labels and fallthrough branches on every fast block.
  TargetInstruction* epilogue_lbl = SWEnsureBlockLabel(gen, epilogue);
  TargetInstruction* slow_lbls[16];
  for (size_t s = 0; s < num_slow_targets; s++) {
    slow_lbls[s] = SWEnsureBlockLabel(gen, slow_targets[s]);
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b] || fall_succ[b] == NULL) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (!SWIsNoFallthroughTerminator(blk->end_code)) {
      TargetInstruction* target_lbl = SWEnsureBlockLabel(gen, fall_succ[b]);
      TargetInstruction* br =
          TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                                target_lbl);
      br->flags = TARGET_INST_PROCESSED;
      SWAppendBlockInst(gen, blk, br, blk->end_code);
    }
  }

  TargetInstruction* fast_p32[33] = {0};
  TargetInstruction* fast_p64[33] = {0};
  TargetInstruction* slow_p32[33] = {0};
  TargetInstruction* slow_p64[33] = {0};
  for (int i = 0; i < num_mapped; i++) {
    int c = saved_list[i];
    int t = temp_list[i];
    allocator->shrink_wrapped_regs |= UINT32_C(1) << c;
    fast_p32[c] = SWNewRegProxy(gen, &allocator->int_regs[t], kSize32Bit);
    fast_p64[c] = SWNewRegProxy(gen, &allocator->int_regs[t], kSize64Bit);
    slow_p32[c] = SWNewRegProxy(gen, &allocator->int_regs[c], kSize32Bit);
    slow_p64[c] = SWNewRegProxy(gen, &allocator->int_regs[c], kSize64Bit);
  }

  // First preserve `c` references in slow blocks whose operand was an
  // instruction defined in `F` whose `.reg` is about to change from `c` to `t`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b < cap && in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    for (TargetInstruction* u = blk->code; u != NULL; u = TargetNext(u)) {
      if (u->dest != NULL && !AARCH64IsVarRegister(u->dest) &&
          u->dest->block != NULL && u->dest->block->block_id < cap &&
          in_fast[u->dest->block->block_id] && u->dest->reg != NULL) {
        AARCH64Register* dr = (AARCH64Register*)u->dest->reg;
        if (dr->type == kAARCH64RegTypeInt && dr->base.num < 33 &&
            slow_p64[dr->base.num] != NULL) {
          u->dest = GetRegisterSize(u->dest) == kSize32Bit
                        ? slow_p32[dr->base.num]
                        : slow_p64[dr->base.num];
        }
      }
      for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
        TargetInstruction* o = u->operand[k];
        if (o != NULL && !AARCH64IsVarRegister(o) &&
            o->opcode != (TargetOpcode)AARCH64_OP(spill) &&
            o->block != NULL && o->block->block_id < cap &&
            in_fast[o->block->block_id] && o->reg != NULL) {
          AARCH64Register* orr = (AARCH64Register*)o->reg;
          if (orr->type == kAARCH64RegTypeInt && orr->base.num < 33 &&
              slow_p64[orr->base.num] != NULL) {
            u->operand[k] = GetRegisterSize(o) == kSize32Bit
                                ? slow_p32[orr->base.num]
                                : slow_p64[orr->base.num];
          }
        }
      }
      if (u == blk->end_code) {
        break;
      }
    }
  }

  // Rewrite `c -> t` across all instructions in `F`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != save_inst && p->opcode != (TargetOpcode)AARCH64_OP(spill) &&
          !SWIsNonExecMeta(p)) {
        if (p->reg != NULL) {
          AARCH64Register* pr = (AARCH64Register*)p->reg;
          if (pr->type == kAARCH64RegTypeInt && pr->base.num < 33 &&
              fast_p64[pr->base.num] != NULL) {
            p->reg = fast_p64[pr->base.num]->reg;
          }
        }
        if (p->dest != NULL) {
          TargetRegister* dtr = p->dest->reg;
          if (dtr != NULL) {
            AARCH64Register* dr = (AARCH64Register*)dtr;
            if (dr->type == kAARCH64RegTypeInt && dr->base.num < 33 &&
                fast_p64[dr->base.num] != NULL) {
              p->dest = GetRegisterSize(p->dest) == kSize32Bit
                            ? fast_p32[dr->base.num]
                            : fast_p64[dr->base.num];
            }
          }
        }
        for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
          TargetInstruction* o = p->operand[k];
          if (o == NULL || o->opcode == (TargetOpcode)AARCH64_OP(spill)) {
            continue;
          }
          TargetRegister* otr =
              o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL);
          if (otr != NULL) {
            AARCH64Register* orr = (AARCH64Register*)otr;
            if (orr->type == kAARCH64RegTypeInt && orr->base.num < 33 &&
                fast_p64[orr->base.num] != NULL) {
              p->operand[k] = GetRegisterSize(o) == kSize32Bit
                                  ? fast_p32[orr->base.num]
                                  : fast_p64[orr->base.num];
            }
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  // Unlink `save_inst` from `entry_block` and unlink all `spill` instructions
  // from pure-fast blocks so they sink into `CommonSave`.
  if (gen->entry_block->code == save_inst) {
    gen->entry_block->code = TargetNext(save_inst);
  }
  if (gen->entry_block->end_code == save_inst) {
    gen->entry_block->end_code = TargetPrev(save_inst);
  }
  ListDeleteElement(&gen->code, &save_inst->header);

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* p = blk->code;
    while (p != NULL) {
      bool is_end = (p == blk->end_code);
      TargetInstruction* nxt = TargetNext(p);
      if (p->opcode == (TargetOpcode)AARCH64_OP(spill)) {
        if (blk->code == p) {
          blk->code = nxt;
        }
        if (blk->end_code == p) {
          blk->end_code = TargetPrev(p);
        }
        ListDeleteElement(&gen->code, &p->header);
        TargetTrackOrphanInstruction(gen, p);
      }
      if (is_end) {
        break;
      }
      p = nxt;
    }
  }

  // Place all fast blocks sequentially right after `entry_block`, followed by
  // `E_fast: ret`, any multi-exit trampolines, and the single `CommonSave`
  // block right before `Slow` so `.eh_frame` stays contiguous and exact.
  TargetInstruction* cursor = gen->entry_block->end_code;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b] || b == entry_id) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* p = blk->code;
    while (p != NULL) {
      bool is_end = (p == blk->end_code);
      TargetInstruction* nxt = TargetNext(p);
      if (p != cursor) {
        ListDeleteElement(&gen->code, &p->header);
        ListInsertAfter(&gen->code, &p->header, &cursor->header);
        cursor = p;
      }
      if (is_end) {
        break;
      }
      p = nxt;
    }
  }

  // Fast frameless return block `E_fast`.
  TargetBasicBlock* fast_ret_blk =
      NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, fast_ret_blk);
  TargetInstruction* fast_ret_lbl =
      TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
  fast_ret_lbl->flags = TARGET_INST_PROCESSED;
  cursor = SWAppendBlockInst(gen, fast_ret_blk, fast_ret_lbl, cursor);
  TargetInstruction* fast_ret_inst =
      TargetNewInstruction((TargetOpcode)AARCH64_OP(ret));
  fast_ret_inst->flags = TARGET_INST_PROCESSED;
  cursor = SWAppendBlockInst(gen, fast_ret_blk, fast_ret_inst, cursor);

  TargetInstruction* save_lbl =
      TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
  save_lbl->flags = TARGET_INST_PROCESSED;

  TargetInstruction* tramp_lbls[16] = {0};
  if (num_slow_targets == 1) {
    tramp_lbls[0] = save_lbl;
  } else {
    for (size_t s = 0; s < num_slow_targets; s++) {
      TargetBasicBlock* tblk = NewTargetBasicBlock(gen->basic_blocks.length);
      VectorAppend(&gen->basic_blocks, tblk);
      TargetInstruction* tlbl =
          TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
      tlbl->flags = TARGET_INST_PROCESSED;
      cursor = SWAppendBlockInst(gen, tblk, tlbl, cursor);
      tramp_lbls[s] = tlbl;

      TargetInstruction* idx_const = SetInstructionSize(
          TargetGetIntConstant(gen, NULL, kTargetType32Bit, (int64_t)s),
          kSize32Bit);
      TargetInstruction* mov_idx = SetInstructionSize(
          TargetNewInstruction1((TargetOpcode)AARCH64_OP(movz), idx_const),
          kSize32Bit);
      mov_idx->reg = &allocator->int_regs[AARCH64_IP1_REG].base;
      mov_idx->flags |= TARGET_INST_PROCESSED;
      cursor = SWAppendBlockInst(gen, tblk, mov_idx, cursor);

      if (s + 1 < num_slow_targets) {
        TargetInstruction* jsave =
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                                  save_lbl);
        jsave->flags = TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, tblk, jsave, cursor);
      }
    }
  }

  // Redirect branches out of `F` to `fast_ret_lbl` and `tramp_lbls[s]`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = SWBranchTargetSlot(p);
      if (slot != NULL && *slot != NULL) {
        if (*slot == epilogue_lbl) {
          *slot = fast_ret_lbl;
        } else {
          for (size_t s = 0; s < num_slow_targets; s++) {
            if (*slot == slow_lbls[s]) {
              *slot = tramp_lbls[s];
              break;
            }
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  // Emit `CommonSave`: `save_lbl: save; mov c_i, t_i; dispatch -> slow_lbls`.
  TargetBasicBlock* save_blk = NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, save_blk);
  cursor = SWAppendBlockInst(gen, save_blk, save_lbl, cursor);
  cursor = SWAppendBlockInst(gen, save_blk, save_inst, cursor);
  for (int i = 0; i < num_mapped; i++) {
    int c = saved_list[i];
    TargetInstruction* mv = SetInstructionSize(
        TargetNewInstruction1((TargetOpcode)AARCH64_OP(mov), fast_p64[c]),
        kSize64Bit);
    mv->reg = &allocator->int_regs[c].base;
    mv->dest = slow_p64[c];
    mv->flags |= TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, mv, cursor);
  }
  for (int idx = 0; idx < num_sunk_slots; idx++) {
    TargetInstruction* proto = sunk_slot_proto[idx];
    TargetInstruction* sp_store = TargetNewInstruction2(
        (TargetOpcode)AARCH64_OP(spill), proto->operand[0], proto->operand[1]);
    sp_store->reg = &allocator->int_regs[sunk_slot_reg[idx]].base;
    sp_store->flags = proto->flags | TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, sp_store, cursor);
  }
  if (num_slow_targets == 1) {
    TargetInstruction* jslow =
        TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                              slow_lbls[0]);
    jslow->flags = TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, jslow, cursor);
  } else {
    TargetInstruction* ip1_proxy =
        SWNewRegProxy(gen, &allocator->int_regs[AARCH64_IP1_REG], kSize32Bit);
    TargetInstruction* eq_cond =
        TargetNewInstruction((TargetOpcode)AARCH64_OP(eq));
    eq_cond->block = save_blk;
    eq_cond->flags = TARGET_INST_PROCESSED;
    TargetTrackOrphanInstruction(gen, eq_cond);
    for (size_t s = 0; s + 1 < num_slow_targets; s++) {
      if (s == 0) {
        TargetInstruction* cbz_s0 = SetInstructionSize(
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(cbz), ip1_proxy,
                                  slow_lbls[0]),
            kSize32Bit);
        cbz_s0->flags |= TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, save_blk, cbz_s0, cursor);
      } else {
        TargetInstruction* c_s = SetInstructionSize(
            TargetGetIntConstant(gen, NULL, kTargetType32Bit, (int64_t)s),
            kSize32Bit);
        TargetInstruction* cmp_s = SetInstructionSize(
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(cmp), ip1_proxy,
                                  c_s),
            kSize32Bit);
        cmp_s->flags |= TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, save_blk, cmp_s, cursor);
        TargetInstruction* beq_s =
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), eq_cond,
                                  slow_lbls[s]);
        beq_s->flags = TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, save_blk, beq_s, cursor);
      }
    }
    TargetInstruction* jlast =
        TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                              slow_lbls[num_slow_targets - 1]);
    jlast->flags = TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, jlast, cursor);
  }

cleanup:
  (void)rcode;
  free(eligible_pure);
  free(eligible_clone);
  free(can_reach_ret);
  free(sim_pure_fast);
  free(sim_clone);
  free(in_fast);
  free(blk_saved);
  free(blk_temp);
  free(blk_arg);
  free(blk_exec);
  free(fall_succ);
  free(clone_of);
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
static bool* HoistFramedAtEnd(TargetGenerator* gen) {
  size_t n = gen->basic_blocks.length;
  bool* framed = calloc(n, sizeof(bool));
  if (framed == NULL) {
    return NULL;
  }
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
      framed[id] = true;
    }
  }
  return framed;
}

typedef struct {
  int header;
  int depth;
  int nlatches;
  int nentries;
  int* latches;
  int* entries;
  BitSet blocks;
} HoistLoop;

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
  if (block == NULL || loop->nlatches <= 0) {
    return false;
  }
  for (int i = 0; i < loop->nlatches; i++) {
    TargetBasicBlock* latch = gen->basic_blocks.value.p[loop->latches[i]];
    if (latch == NULL ||
        !BitSetContains(&latch->dominators, block->block_id)) {
      return false;
    }
  }
  return true;
}

static uint64_t HoistLiveAtInsert(TargetBasicBlock* pred, uint64_t* live_out,
                                  uint64_t* extra_live) {
  uint64_t live = 0;
  if (pred->block_id < 0) {
    return ~UINT64_C(0);
  }
  live = live_out[pred->block_id] | extra_live[pred->block_id];
  if (pred->end_code != NULL && HoistIsBranch(pred->end_code)) {
    uint64_t uses = 0;
    uint64_t defs = 0;
    HoistInstEffects(pred->end_code, &uses, &defs, false);
    live = (live & ~defs) | uses;
  }
  return live;
}

static bool HoistEntriesReady(TargetGenerator* gen, HoistLoop* loop,
                              bool need_frame, const bool* framed) {
  if (loop->nentries <= 0) {
    return false;
  }
  for (int i = 0; i < loop->nentries; i++) {
    size_t pred_id = (size_t)loop->entries[i];
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
    if (need_frame && (framed == NULL || !framed[pred_id])) {
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
                          HoistGroup* group, int reg, uint64_t* extra_live) {
  TargetGenerator* gen = &allocator->g->base;
  TargetBasicBlock* first_pred = gen->basic_blocks.value.p[loop->entries[0]];
  TargetInstruction* primary =
      group->is_imm ? HoistMakeImmediate(allocator, group, reg)
                    : HoistMakeReload(allocator, group->spill, reg, group->size);
  HoistInsert(gen, first_pred, primary);
  extra_live[first_pred->block_id] |= UINT64_C(1) << reg;
  for (int e = 1; e < loop->nentries; e++) {
    TargetBasicBlock* pred = gen->basic_blocks.value.p[loop->entries[e]];
    TargetInstruction* copy =
        group->is_imm ? HoistMakeImmediate(allocator, group, reg)
                      : HoistMakeReload(allocator, group->spill, reg, group->size);
    HoistInsert(gen, pred, copy);
    extra_live[pred->block_id] |= UINT64_C(1) << reg;
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
                              HoistLoop* loop, uint64_t* live_in,
                              uint64_t* live_out, uint64_t* extra_live,
                              const bool* framed) {
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

  uint64_t blocked = used | live_in[loop->header];
  for (int e = 0; e < loop->nentries; e++) {
    TargetBasicBlock* pred = gen->basic_blocks.value.p[loop->entries[e]];
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
  const HoistLoop* a = lhs;
  const HoistLoop* b = rhs;
  return b->depth - a->depth;
}

static void HoistLoopInvariantValues(AARCH64RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  size_t nblocks = gen->basic_blocks.length;
  if (!OptLevel2() || gen->entry_block == NULL || nblocks == 0) {
    return;
  }

  uint64_t* live_in = calloc(nblocks, sizeof(uint64_t));
  uint64_t* live_out = calloc(nblocks, sizeof(uint64_t));
  uint64_t* extra_live = calloc(nblocks, sizeof(uint64_t));
  if (live_in == NULL || live_out == NULL || extra_live == NULL) {
    free(live_in);
    free(live_out);
    free(extra_live);
    return;
  }
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
            out |= live_in[succ];
          }
        }
      }
      live_out[id] = out;
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
      if (cur != live_in[id]) {
        live_in[id] = cur;
        grew = true;
      }
    }
  }
  if (grew) {
    for (size_t i = 0; i < nblocks; i++) {
      live_in[i] = ~UINT64_C(0);
      live_out[i] = ~UINT64_C(0);
    }
  }

  HoistLoop* loops = calloc(nblocks, sizeof(HoistLoop));
  int nloops = 0;
  if (loops != NULL) {
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
      if (nlatch == 0 || nloops >= (int)nblocks) {
        continue;
      }
      HoistLoop* loop = &loops[nloops];
      BitSetInit(&loop->blocks);
      BitSetInsert(&loop->blocks, header_id);
      size_t* work = malloc(nblocks * sizeof(size_t));
      size_t nw = 0;
      if (work == NULL) {
        BitSetDestruct(&loop->blocks);
        break;
      }
      for (size_t i = 0; i < header->in_edges.length; i++) {
        size_t pred = header->in_edges.value.w[i];
        TargetBasicBlock* latch = gen->basic_blocks.value.p[pred];
        if (latch != NULL &&
            BitSetContains(&latch->dominators, header_id) &&
            !BitSetContains(&loop->blocks, pred)) {
          BitSetInsert(&loop->blocks, pred);
          work[nw++] = pred;
        }
      }
      while (nw > 0) {
        size_t id = work[--nw];
        TargetBasicBlock* block = gen->basic_blocks.value.p[id];
        if (block == NULL) {
          continue;
        }
        for (size_t i = 0; i < block->in_edges.length; i++) {
          size_t pred = block->in_edges.value.w[i];
          if (!BitSetContains(&loop->blocks, pred)) {
            BitSetInsert(&loop->blocks, pred);
            work[nw++] = pred;
          }
        }
      }
      free(work);
      loop->header = (int)header_id;
      loop->depth = header->loop_nesting;
      loop->latches = malloc((size_t)nlatch * sizeof(int));
      loop->entries = malloc(header->in_edges.length * sizeof(int));
      loop->nlatches = 0;
      loop->nentries = 0;
      if (loop->latches == NULL || loop->entries == NULL) {
        free(loop->latches);
        free(loop->entries);
        BitSetDestruct(&loop->blocks);
        break;
      }
      for (size_t i = 0; i < header->in_edges.length; i++) {
        size_t pred = header->in_edges.value.w[i];
        if (BitSetContains(&loop->blocks, pred)) {
          loop->latches[loop->nlatches++] = (int)pred;
        } else {
          loop->entries[loop->nentries++] = (int)pred;
        }
      }
      nloops++;
    }
    qsort(loops, (size_t)nloops, sizeof(HoistLoop), HoistLoopDeeper);
    bool* framed = HoistFramedAtEnd(gen);
    for (int i = 0; i < nloops; i++) {
      HoistGroupsInLoop(allocator, &loops[i], live_in, live_out, extra_live,
                        framed);
    }
    free(framed);
    for (int i = 0; i < nloops; i++) {
      BitSetDestruct(&loops[i].blocks);
      free(loops[i].latches);
      free(loops[i].entries);
    }
  }
  free(loops);
  free(live_in);
  free(live_out);
  free(extra_live);
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

// The coloring allocator is the default from -O1; at -O0 the dominator walk is
// faster and DAVECC_NEW_RA selects coloring anyway.  DAVECC_OLD_RA selects the
// dominator walk at every level.  For bisecting, DAVECC_NEW_RA_FUNC limits the
// coloring allocator to one function and DAVECC_NEW_RA_LIMIT to the first n
// functions compiled.
static bool UseColoringAllocator(AARCH64RegisterAllocator* allocator) {
  if (getenv("DAVECC_OLD_RA") != NULL ||
      (!OptLevel1() && getenv("DAVECC_NEW_RA") == NULL)) {
    return false;
  }
  const char* only = getenv("DAVECC_NEW_RA_FUNC");
  if (only != NULL &&
      strcmp(only, allocator->g->base.function_name.value) != 0) {
    return false;
  }
  const char* limit = getenv("DAVECC_NEW_RA_LIMIT");
  if (limit != NULL) {
    static int count;
    return count++ < atoi(limit);
  }
  return true;
}

static void AllocateByDominatorWalk(AARCH64RegisterAllocator* allocator) {
  // The hidden aggregate-result pointer has an ABI-selected dedicated
  // register. Reserve that physical register before allocating ordinary
  // variable registers; otherwise an incoming argument can be assigned the
  // same register and then be clobbered when x8 is saved in the prologue.
  if (allocator->g->struct_return_reg >= 0) {
    bool is_leaf = allocator->g->base.num_calls == 0 && compiler->optimize;
    int first = is_leaf ? AARCH64_FIRST_LEAF_INT_REG_VAR
                        : AARCH64_FIRST_INT_REG_VAR;
    allocator->int_regs[first + allocator->g->struct_return_reg]
        .base.reserved = true;
  }
  TargetMarkCallPreservedInstructions(&allocator->g->base,
                                      &allocator->preserved_instructions);
  if (TargetMaterializePreservedCallResults(
          &allocator->g->base, &allocator->preserved_instructions,
          CreateCallResultCopy)) {
    TargetBuildBasicBlockInputsAndOutputs(&allocator->g->base);
    BitSetClear(&allocator->preserved_instructions);
    TargetMarkCallPreservedInstructions(&allocator->g->base,
                                        &allocator->preserved_instructions);
  }
  BuildShortLivedVarRegSet(allocator);

  // Process all basic blocks in the AARCH64 generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->g->base.entry_block);
}

void AARCH64AllocateRegisters(AARCH64RegisterAllocator* allocator) {
  if (getenv("DAVECC_RA_CHECK") != NULL) {
    TargetRACheck(&allocator->g->base, AARCH64RAInfo(), NULL, stderr, 3);
  }
  if (!UseColoringAllocator(allocator) ||
      !AARCH64AllocateRegistersByColoring(allocator)) {
    AllocateByDominatorWalk(allocator);
  }
  ShrinkWrapFunction(allocator);
  HoistLoopInvariantValues(allocator);
  FoldPhysicalCopies(allocator);
}

static const char* AARCH64RegisterNameNoSize(AARCH64Register* reg, int size, char* buf, size_t len) {
  return AARCH64RegisterNameFromNum1(reg->base.num, reg->type, true, size, buf, len);
}

const char* AARCH64RegisterName(AARCH64Register* reg, int size, char* buf, size_t len) {
  return AARCH64RegisterNameFromNum(reg->base.num, reg->type, size, buf, len);
}

static const char* AARCH64RegisterNameFromNum1(int num, AARCH64RegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                   size_t len) {
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
  
  // See if the register is in one of the named ranges.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      if (num >= register_ranges[i].start && num <= register_ranges[i].end) {
        if (size == 0 && !allow_no_size) {
          printf("No size specfied for register %d\n", num);
          abort();
        }
        const char* prefix = type == kAARCH64RegTypeInt ? "r" : "d";
        switch (size) {
          case kSize32Bit:
            switch (type) {
              case kAARCH64RegTypeInt:
                prefix = "w";
                break;
              case kAARCH64RegTypeFloat:
                prefix = "s";
                break;
            }
            break;
          case kSize64Bit:
            switch (type) {
              case kAARCH64RegTypeInt:
                prefix = "x";
                break;
              case kAARCH64RegTypeFloat:
                prefix = "d";
                break;
            }
            break;
          case kSize128Bit:
            prefix = type == kAARCH64RegTypeFloat ? "q" : "x";
            break;
       }
        snprintf(buf, len, "%s%d", prefix,
                 num);
        return buf;
      }
    }
  }
 
  return buf;
}

const char* AARCH64RegisterNameFromNum(int num, AARCH64RegisterType type, int size,
                                   char* buf,
                                   size_t len) {
  return AARCH64RegisterNameFromNum1(num, type, size, false, buf, len);
}

#undef NUM_REG_RANGES

