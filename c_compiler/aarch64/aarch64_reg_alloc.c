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
#include "aarch64_codegen.h"
#include "aarch64_machine.h"
#include "target_basic_block.h"
#include "compiler.h"

static void AllocateRegister(AARCH64RegisterAllocator* allocator,
                             TargetInstruction* inst);

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
  allocator->spill_after_definition = false;
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
} register_ranges[] = {
  // General temporaries prefer non-argument registers.  Argument registers
  // (x0..x7 / d0..d7) are searched last so a long-lived temporary does not
  // grab one that is about to be needed for an outgoing call's register
  // argument (which would clobber the temporary when the argument is set up,
  // or vice versa).  x9 is intentionally excluded from the general pool: it is
  // the dedicated scratch used to stage indirect-call targets across argument
  // setup.
  {kAARCH64RegTypeInt, AARCH64_INT_TEMP_START + 1, AARCH64_INT_TEMP_END, true},
  {kAARCH64RegTypeInt, AARCH64_INT_SAVED_START, AARCH64_INT_SAVED_END, false},
  {kAARCH64RegTypeInt, AARCH64_INT_ARG_START, AARCH64_INT_ARG_END, true},
    {kAARCH64RegTypeFloat, AARCH64_FP_TEMP_START, AARCH64_FP_TEMP_END, true},
    {kAARCH64RegTypeFloat, AARCH64_FP_SAVED_START, AARCH64_FP_SAVED_END,false},
    {kAARCH64RegTypeFloat, AARCH64_FP_ARG_START, AARCH64_FP_ARG_END, true},
};

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

static AARCH64Register* FindFreeRegister(AARCH64RegisterAllocator* allocator,
                                    AARCH64RegisterType type, bool can_use_temp) {
  AARCH64Register* regs =
      type == kAARCH64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      // If we are told not to use a temp register, ignore any that are
      // marked as temp.
      if (!can_use_temp && register_ranges[i].temp) {
        continue;
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

static void FreeRegister(AARCH64RegisterAllocator* allocator, AARCH64Register* reg) {
  reg->base.owner = NULL;
}

static bool IsShortLivedVarReg(AARCH64RegisterAllocator* allocator,
                               TargetInstruction* inst) {
  return AARCH64IsVarRegister(inst) &&
         BitSetContains(&allocator->short_lived_varregs, inst->id);
}


// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(AARCH64RegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
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

// Calculate the cost of spilling the instruction.  This
// takes the number of uses and the loop nesting level into
// account.  The cost starts out as the number of
// uses left for the instruction.  It is then multiplied
// by the loop nesting level for all its users that have not yet
// got a register (+1) all the way to the top of the dominator tree.
//
// Blocks outside loops will have no effect on the score but
// blocks with a loop_nesting value of 1 will double the cost.  A
// loop_nesting value of 2 will quadruple the cost, and so on.
static int SpillCost(TargetInstruction* inst) {
  int cost = inst->uses;
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* user = inst->users.value.p[i];
    if (user->reg == NULL) {
      TargetBasicBlock* block = user->block;
      while (block != NULL) {
        cost *= block->loop_nesting + 1;
        if (cost > INT_MAX / 4) {
          return INT_MAX / 4;
        }
        block = block->idom;
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

// Returns the cheapest value whose register can be taken without breaking a
// read that has already been handed that register, or NULL when there is none.
// |unsafe|, when not NULL, receives the cheapest value that could be taken if
// there were no such reads, for a caller that has no way to proceed without
// taking a register from something.
static TargetInstruction* FindSpillVictim(AARCH64RegisterAllocator* allocator,
                                          AARCH64RegisterType type,
                                          bool can_use_temp,
                                          TargetBasicBlock* block,
                                          TargetInstruction** unsafe) {
  AARCH64Register* regs =
      type == kAARCH64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  BitSet reentered;
  BitSetInit(&reentered);
  TargetBasicBlockReachableAfter(&allocator->g->base, block, &reentered);
  int unsafe_cost = INT_MAX;
  TargetInstruction* unsafe_victim = NULL;
  int min_cost = INT_MAX;
  TargetInstruction* victim = NULL;
  int min_var_cost = INT_MAX;
  TargetInstruction* var_victim = NULL;
  // Find the instruction with the lowest spill cost.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      if (!can_use_temp && register_ranges[i].temp) {
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
          if (IsUnspillableFixedReg(owner)) {
            continue;
          }
          if (AARCH64IsVarRegister(owner) && owner->users.length == 0) {
            // A write-only variable has no valid first-use spill site and no
            // future read that requires its physical register.
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
              shares_register_with_a_value) {
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
  if (unsafe != NULL) {
    *unsafe = unsafe_victim;
    return victim;
  }
  // The caller cannot deal with there being nothing safe to take, so give it
  // the cheapest unsafe candidate as this always did.
  return victim != NULL ? victim : unsafe_victim;
}

static bool NotProcessed(TargetInstruction* inst, void* data) {
  return (inst->flags & TARGET_INST_PROCESSED) == 0;
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

static AARCH64Register* SpillInstruction(AARCH64RegisterAllocator* allocator, TargetInstruction* inst) {
  AARCH64Register* reg = (AARCH64Register*)inst->reg;    // Current register.
  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)AARCH64_OP(spill), NULL,
                                                   TargetGetIntConstant(&allocator->g->base,
                                                                        NULL,
                                                                        kTargetType32Bit,
                                                                        allocator->current_spilled_region_size));
  allocator->current_spilled_region_size += 8;    // Space for one register.
  if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size = allocator->current_spilled_region_size;
  }
 
  if (AARCH64IsVarRegister(inst) ||
      InstructionHasExternalDefs(allocator, inst)) {
    // Variable and merge pseudos have no executable definition of their own.
    // Keep the spill only as a slot handle and write that slot after every
    // real assignment, including assignments already allocated.
    TargetTrackOrphanInstruction(&allocator->g->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    InsertReassignableStoreBacks(allocator, inst, spill);
  } else {
    // Emit spill instruction just after spilled instruction.
    TargetBasicBlockEmitAfter(&allocator->g->base, inst->block, spill, inst);
  }
  
  // Retarget all uses of the original instruction to the spill.  If the
  // user has already been processed this will have no effect.
  // NOTE: this will transfer all uses of the inst to the spill, leaving
  // the users of inst empty and its uses count 0.
  TargetRetargetInstructionIf(inst, spill, NotProcessed, NULL);
  spill->uses = inst->uses;
  spill->operand[0] = inst;
  spill->reg = inst->reg;
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
                                            bool can_use_temp,
                                            bool may_spill_after_def) {
  AARCH64Register* reg = FindFreeRegister(allocator, type, can_use_temp);

  if (reg == NULL) {
    AARCH64Register* scratch = ScratchRegister(allocator, type);
    TargetInstruction* unsafe = NULL;
    TargetInstruction* victim = FindSpillVictim(
        allocator, type, can_use_temp, block,
        may_spill_after_def && scratch != NULL && CanSpillAfterDefinition(inst)
            ? &unsafe
            : NULL);
    // FindSpillVictim can release a stale/dead owner while scanning.
    reg = FindFreeRegister(allocator, type, can_use_temp);
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

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(AARCH64RegisterAllocator* allocator, TargetInstruction* inst) {
  if (IsShortLivedVarReg(allocator, inst)) {
    return true;
  }
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
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
                                   CanUseTemp(allocator, inst), false);
  }
  AssignRegister(reg, inst);
}

static COMPILER_UNUSED void AllocateForRmov(AARCH64RegisterAllocator* allocator,
                            TargetInstruction* inst) {
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
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ((int)op->opcode == (int)AARCH64_OP(spill))) {
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
          CanUseTemp(allocator, reload), false);
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
    }
  }
}

// Allocate registers for operand instructions (e.g. address calcs for ldr/str)
// that are not reached by the linear block scan.
static void EnsureOperandsAllocated(AARCH64RegisterAllocator* allocator,
                                    TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op == NULL || op->reg != NULL || op->block == NULL ||
        TargetIsConst(op)) {
      continue;
    }
    if (AARCH64IsFixedRegister(op)) {
      AllocateRegister(allocator, op);
      continue;
    }
    if (AARCH64IsVarRegister(op)) {
      if (op->reg == NULL) {
        AllocateVariableRegister(allocator, op);
      }
      continue;
    }
    if ((op->flags & TARGET_INST_PROCESSED) == 0) {
      AllocateRegister(allocator, op);
    }
  }
}

static void AllocateRegister(AARCH64RegisterAllocator* allocator,
                             TargetInstruction* inst) {
   bool is_leaf = allocator->g->base.num_calls == 0 &&
      compiler->optimize;

  AARCH64Opcode opcode = (AARCH64Opcode)inst->opcode;

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

  EnsureOperandsAllocated(allocator, inst);

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
                                   CanUseTemp(allocator, inst), false);
    AssignRegister(reg, inst);
    FreeRegisters(allocator, inst);
    inst->flags |= TARGET_INST_PROCESSED;
    return;
  }

  if (inst->dest != NULL) {
    if (inst->dest->reg == NULL) {
      if (AARCH64IsVarRegister(inst->dest)) {
        // Assignment to a variable register,
        AllocateVariableRegister(allocator, inst->dest);
      } else {
        AllocateRegister(allocator, inst->dest);
      }
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
    // A fixed argument-register pseudo (r0..r7 / d0..d7) is allocated once in
    // the symbol pre-pass, but InitializeBasicBlockRegisters clears every
    // physical register's owner at each block boundary.  Because the pseudo
    // already has a register, the allocation above is skipped and ownership is
    // never re-established, so inside a loop body the argument register looks
    // free and an intervening temporary can steal it, clobbering an argument.
    // Re-claim ownership here for the value we are routing into it.
    if (IsUnspillableFixedReg(inst->dest) && reg->base.owner == NULL) {
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
    case   AARCH64_OP(oplsl):
    case AARCH64_OP(spill):
    case AARCH64_OP(reload):
      // These instructions do not have registers allocated to them.
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
      AARCH64RegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     reg_type, CanUseTemp(allocator, inst),
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
  if (inst->uses == 0 && !reg->base.reserved) {
    FreeRegister(allocator, reg);
  }
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
      inst->reg->owner = inst;
    }
  }
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

static bool IsListedUser(TargetInstruction* value,
                         TargetInstruction* candidate) {
  for (size_t i = 0; i < value->users.length; i++) {
    if (value->users.value.p[i] == candidate) {
      return true;
    }
  }
  return false;
}

// Variable-register liveness is intentionally conservative because a C
// variable can be reassigned or carried around a loop. Some lowering-created
// pseudos, however, have exactly one definition and are consumed linearly in
// that same block before any call. They are ordinary temporaries in all but
// name and can safely use a caller-saved register.
static bool HasShortBlockLocalLifetime(AARCH64RegisterAllocator* allocator,
                                       TargetInstruction* value) {
  if (!AARCH64IsVarRegister(value) || value->users.length == 0) {
    return false;
  }

  TargetInstruction* definition = NULL;
  TargetBasicBlock* definition_block = NULL;
  int definitions = 0;
  TargetGenerator* gen = &allocator->g->base;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if (inst->dest == value) {
        definition = inst;
        definition_block = block;
        definitions++;
      }
      if (is_end) {
        break;
      }
    }
  }
  if (definitions != 1 || definition == NULL) {
    return false;
  }

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

  for (size_t i = 0; i < value->users.length; i++) {
    TargetInstruction* user = value->users.value.p[i];
    if (user->block != definition_block) {
      return false;
    }
  }

  size_t remaining_users = value->users.length;
  bool saw_definition = false;
  for (TargetInstruction* inst = definition_block->code; inst != NULL;
       inst = TargetNext(inst)) {
    bool is_end = inst == definition_block->end_code;
    if (inst == definition) {
      saw_definition = true;
    }
    if (IsListedUser(value, inst)) {
      if (!saw_definition) {
        return false;
      }
      remaining_users--;
      if (remaining_users == 0) {
        return true;
      }
    }
    if (saw_definition && AARCH64IsCall(inst)) {
      return false;
    }
    if (is_end) {
      break;
    }
  }
  return false;
}

static void BuildShortLivedVarRegSet(AARCH64RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if (inst->dest != NULL &&
          HasShortBlockLocalLifetime(allocator, inst->dest)) {
        BitSetInsert(&allocator->short_lived_varregs, inst->dest->id);
      }
      if (inst->dest != NULL && AARCH64IsVarRegister(inst->dest) &&
          inst->users.length != 0) {
        BitSetInsert(&allocator->shared_varregs, inst->dest->id);
      }
      if (is_end) {
        break;
      }
    }
  }
}


// Build the preserved_instructions set, instructions that need their
// register to be preserved across calls.  If the block contains a call
// all outputs need to be preserved.
static void BuildPreservedInstructionsSet(TargetBasicBlock* block, void* data) {
  AARCH64RegisterAllocator* allocator = data;
  if (!block->contains_call) {
    return;
  }
  // Preserve all outputs.
  BitSetUnionInPlace(&allocator->preserved_instructions, &block->output_ids);
}

void AARCH64AllocateRegisters(AARCH64RegisterAllocator* allocator) {
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
  BuildShortLivedVarRegSet(allocator);
  TargetTraverseDominatorTree(&allocator->g->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);

  // Process all basic blocks in the AARCH64 generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->g->base.entry_block);
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

