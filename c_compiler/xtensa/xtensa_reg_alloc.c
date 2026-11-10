//
//  xtensa_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "xtensa_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include "compiler.h"
#include "target_basic_block.h"
#include "xtensa_codegen.h"
#include "xtensa_machine.h"

static void AllocateRegister(XTENSARegisterAllocator* allocator,
                             TargetInstruction* inst);
static XTENSARegisterType RegisterTypeFromInstruction(TargetInstruction* inst);

static void Trap() {}

static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 63) {
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

static void InitializeRegister(XTENSARegister* reg, int num,
                               XTENSARegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void XTENSARegisterAllocatorInit(XTENSARegisterAllocator* allocator,
                                 XTENSAGenerator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < XTENSA_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kXTENSARegTypeInt);
  }

  for (int i = 0; i < XTENSA_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kXTENSARegTypeFloat);
  }

  // Reserve some registers.
  allocator->int_regs[XTENSA_INT_ZERO_REG].base.reserved = true;
  allocator->int_regs[XTENSA_FP_REG].base.reserved = true;
  allocator->int_regs[XTENSA_SP_REG].base.reserved = true;
  allocator->int_regs[XTENSA_SPILL_ADDR].base.reserved = true;
  allocator->int_regs[XTENSA_INT_TEMP_START_1].base.reserved = true;
  allocator->int_regs[XTENSA_INT_TEMP_START_1 + 1].base.reserved = true;
  allocator->int_regs[XTENSA_INT_TEMP_START_1 + 2].base.reserved = true;
  allocator->int_regs[XTENSA_INT_TEMP_START_2].base.reserved = true;
  allocator->int_regs[XTENSA_INT_TEMP_START_2 + 1].base.reserved = true;
  allocator->int_regs[XTENSA_INT_TEMP_START_2 + 2].base.reserved = true;
  allocator->int_regs[XTENSA_INT_TEMP_START_2 + 3].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);

  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
  MapInitForPointerKeys(&allocator->reassignable_spills);
  allocator->allocating_depth = 0;
}

XTENSARegisterAllocator* NewXTENSARegisterAllocator(XTENSAGenerator* pcode) {
  XTENSARegisterAllocator* reg_alloc = malloc(sizeof(XTENSARegisterAllocator));
  XTENSARegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void XTENSARegisterAllocatorDestruct(XTENSARegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
  MapDestruct(&allocator->reassignable_spills);
}

void XTENSARegisterAllocatorDelete(XTENSARegisterAllocator* alloc) {
  XTENSARegisterAllocatorDestruct(alloc);
  free(alloc);
}

// RISC-V32's ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  XTENSARegisterType type;  // Register type.
  int start;                // Start of range.
  int end;                  // End of range.
  const char* prefix;       // Register name prefix.
  int base;
  bool temp;
} register_ranges[] = {
    {kXTENSARegTypeInt, XTENSA_INT_TEMP_START_1, XTENSA_INT_TEMP_END_1, "t", 0,
     true},
    {kXTENSARegTypeInt, XTENSA_INT_TEMP_START_2, XTENSA_INT_TEMP_END_2, "t", 3,
     true},
    {kXTENSARegTypeInt, XTENSA_INT_ARG_START, XTENSA_INT_ARG_END, "a", 0, true},
    {kXTENSARegTypeInt, XTENSA_RET_REG, XTENSA_RET_REG, "ra", 0, true},
    {kXTENSARegTypeInt, XTENSA_INT_SAVED_START_1, XTENSA_INT_SAVED_END_1, "s",
     0, false},
    {kXTENSARegTypeInt, XTENSA_INT_SAVED_START_2, XTENSA_INT_SAVED_END_2, "s",
     2, false},
    {kXTENSARegTypeFloat, XTENSA_FP_TEMP_START_1, XTENSA_FP_TEMP_END_1, "ft", 0,
     true},
    {kXTENSARegTypeFloat, XTENSA_FP_TEMP_START_2, XTENSA_FP_TEMP_END_2, "ft", 2,
     true},
    {kXTENSARegTypeFloat, XTENSA_FP_ARG_START, XTENSA_FP_ARG_END, "fa", 0,
     true},
    {kXTENSARegTypeFloat, XTENSA_FP_SAVED_START_1, XTENSA_FP_SAVED_END_1, "fs",
     0, false},
    {kXTENSARegTypeFloat, XTENSA_FP_SAVED_START_2, XTENSA_FP_SAVED_END_2, "fs",
     2, false},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(XTENSARegisterAllocator* allocator) {
  char buf[32];
  for (XTENSARegisterType type = kXTENSARegTypeInt; type <= kXTENSARegTypeFloat;
       type++) {
    XTENSARegister* regs =
        type == kXTENSARegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end;
             j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(x%d): free\n",
                   XTENSARegisterName(&regs[j], buf, sizeof(buf)),
                   regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(x%d): owner: @%d %s\n",
                   XTENSARegisterName(&regs[j], buf, sizeof(buf)),
                   regs[j].base.num, owner->id,
                   XTENSAOpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(XTENSARegister* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static XTENSARegister* FindFreeRegister(XTENSARegisterAllocator* allocator,
                                        XTENSARegisterType type,
                                        bool can_use_temp) {
  XTENSARegister* regs =
      type == kXTENSARegTypeInt ? allocator->int_regs : allocator->float_regs;
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      // If we are told not to use a temp register, ignore any that are
      // marked as temp.
      if (!can_use_temp && register_ranges[i].temp) {
        continue;
      }
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner == NULL) {
          // If we use the return address register, we must save it and
          // therefore we are not a leaf procedure.
          if (regs[j].base.num == XTENSA_RET_REG) {
            allocator->rv->not_leaf = true;
          }
          return &regs[j];
        }
      }
    }
  }

  // No registers available.
  return NULL;
}

static void FreeRegister(XTENSARegisterAllocator* allocator,
                         XTENSARegister* reg) {
  reg->base.owner = NULL;
}

static bool HasUnprocessedUserOtherThan(TargetInstruction* value,
                                        TargetInstruction* current) {
  for (size_t i = 0; i < value->users.length; i++) {
    TargetInstruction* user = value->users.value.p[i];
    if (user != current && (user->flags & TARGET_INST_PROCESSED) == 0) {
      return true;
    }
  }
  return false;
}

// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(XTENSARegisterAllocator* allocator,
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
      if (XTENSAIsFixedRegister(op)) {
        continue;
      }
      if (XTENSAIsVarRegister(op)) {
        continue;
      }
      // Inside a loop, a value that is live-out of the block is read again on a
      // later iteration through the back edge, so its linear "last use" in this
      // block is not really its last use.  Freeing its register here would let
      // a subsequent temp reuse it and clobber the still-live value (e.g. a
      // pooled loop-bound constant sharing a register with the loop variable).
      // Restrict this to loop blocks so straight-line code keeps freeing
      // registers promptly.
      if (inst->block != NULL && inst->block->loop_nesting > 0 &&
          BitSetContains(&inst->block->output_ids, op->id)) {
        continue;
      }
#if 0
      if (((int)op->opcode == (int)XTENSA_OP(reload)) || ((int)op->opcode == (int)XTENSA_OP(spill))) {
        continue;
      }
#endif
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL) {
        if (op->uses > 0) {
          op->uses--;
        }
        if (!HasUnprocessedUserOtherThan(op, inst) && reg->owner == op) {
          FreeRegister(allocator, (XTENSARegister*)reg);
        }
      }
    }
  }

#if 0
  // A destination is a use of that instruction, decrement it too, as long
  // as it's not one of the operands.
  if (inst->dest != NULL && !dest_in_operands) {
    inst->dest->uses--;
    assert(inst->dest->uses >= 0);
    if (inst->dest->uses == 0) {
      FreeRegister(allocator, (XTENSARegister*)inst->dest->reg);
    }
  }
#endif
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(XTENSARegister* reg) {
  int num = reg->base.num;
  if ((num >= XTENSA_INT_SAVED_START_1 && num <= XTENSA_INT_SAVED_END_1) ||
      (num >= XTENSA_INT_SAVED_START_2 && num <= XTENSA_INT_SAVED_END_2) ||
      (num >= XTENSA_FP_SAVED_START_1 && num <= XTENSA_FP_SAVED_END_1) ||
      (num >= XTENSA_FP_SAVED_START_2 && num <= XTENSA_FP_SAVED_END_2)) {
    return true;
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
        block = block->idom;
      }
    }
  }

  return cost;
}

// A bare tmp can be defined by other instructions through ->dest (not through
// its users list), notably the merged value of a conditional expression.  The
// RISC-V32 spill model stores an instruction once at its declaration point, so
// spilling such a tmp captures an undefined/stale value instead of the later
// branch definition.  Keep externally-defined values in registers until the
// allocator has a store-back model for every definition.
static bool IsReassignableDefinition(TargetInstruction* inst,
                                     TargetInstruction* target) {
  if (inst->dest == target) {
    return true;
  }
  XTENSAOpcode opcode = (XTENSAOpcode)inst->opcode;
  return (opcode == XTENSA_OP(mv) || opcode == XTENSA_OP(fmv_s) ||
          opcode == XTENSA_OP(fmv_d)) &&
         inst->dest == NULL && inst->operand[0] == target &&
         inst->operand[1] != NULL;
}

static bool InstructionHasExternalDefs(XTENSARegisterAllocator* allocator,
                                       TargetInstruction* target) {
  TargetGenerator* gen = &allocator->rv->base;
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

// Find the cheapest value to evict so that its register can be reused.  When
// `can_use_temp` is false the value asking for the register has to survive a
// call, so only a callee-saved register will do for it: a victim holding a
// caller-saved temp is no use here, because handing that temp over would leave
// the new value to be clobbered by the very call it has to live across.
//
// Register variables are considered only when nothing else can be freed.  On a
// non-leaf function they own the whole of the second callee-saved range, so
// without them there is often no callee-saved register to reclaim at all.
static TargetInstruction* FindSpillVictim(XTENSARegisterAllocator* allocator,
                                          XTENSARegisterType type,
                                          bool can_use_temp) {
  XTENSARegister* regs =
      type == kXTENSARegTypeInt ? allocator->int_regs : allocator->float_regs;
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
              ((int)owner->opcode == (int)XTENSA_OP(spill))) {
            // Already in memory, so it is not holding this register any more.
            regs[j].base.owner = NULL;
            continue;
          }
          if (XTENSAIsVarRegister(owner)) {
            if (owner->users.length == 0) {
              regs[j].base.owner = NULL;
              continue;
            }
            int cost = SpillCost(owner);
            if (cost < min_var_cost) {
              min_var_cost = cost;
              var_victim = owner;
            }
            continue;
          }
          int cost = SpillCost(owner);
          if (cost < min_cost) {
            min_cost = cost;
            victim = owner;
          }
        }
      }
    }
  }
  if (victim == NULL) {
    victim = var_victim;
  }
  return victim;
}

// True if |inst|'s own allocation is in progress and its reload pass has been
// and gone.  Pointing one of its operands at a spill slot now would do nothing
// but leave the read naming the physical register the spill just gave away.
static bool IsBeingAllocated(XTENSARegisterAllocator* allocator,
                             TargetInstruction* inst) {
  for (size_t d = 0; d < allocator->allocating_depth; d++) {
    if (allocator->allocating[d] == inst) {
      return allocator->allocating_reloaded[d];
    }
  }
  return false;
}

// Record that reloads have been inserted for |inst|'s spilled operands, so that
// any spill from here on has to repair its reads in place.
static void NoteReloadsInserted(XTENSARegisterAllocator* allocator,
                                TargetInstruction* inst) {
  for (size_t d = 0; d < allocator->allocating_depth; d++) {
    if (allocator->allocating[d] == inst) {
      allocator->allocating_reloaded[d] = true;
      return;
    }
  }
}

static bool NotProcessed(TargetInstruction* inst, void* data) {
  XTENSARegisterAllocator* allocator = data;
  return (inst->flags & TARGET_INST_PROCESSED) == 0 &&
         !IsBeingAllocated(allocator, inst);
}

// A read that has already been allocated names the physical register the value
// was in, and spilling hands that register to something else.  Stage the slot
// through the dedicated scratch register immediately before the read instead.
// Returns false when the read cannot be repaired, which leaves it as it was.
static bool RepairProcessedRead(XTENSARegisterAllocator* allocator,
                                TargetInstruction* user,
                                TargetInstruction* value,
                                TargetInstruction* spill) {
  if (user->block == NULL || value->reg == NULL) {
    return false;
  }
  if ((int)user->opcode == (int)XTENSA_OP(spill) ||
      (int)user->opcode == (int)XTENSA_OP(reload)) {
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
  if (((XTENSARegister*)value->reg)->type != kXTENSARegTypeInt ||
      RegisterTypeFromInstruction(user) != kXTENSARegTypeInt) {
    // There is no scratch register in the floating point file, and the read has
    // to want an integer register for the staged one to be usable.
    return false;
  }
  // t0 is the code generator's staging temporary and is never handed out by the
  // allocator, so it is free between the reload and the read that follows it.
  XTENSARegister* scratch = &allocator->int_regs[XTENSA_INT_TEMP_START_1];
  // There is one scratch register, so a read that already stages something else
  // through it cannot stage this slot too.
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = user->operand[i];
    if (op != NULL && op != value && op->reg == &scratch->base) {
      return false;
    }
  }
  TargetInstruction* reload =
      TargetNewInstruction1((TargetOpcode)XTENSA_OP(reload), spill);
  TrapReload(reload);
  TargetBasicBlockEmitBefore(&allocator->rv->base, user->block, reload, user);
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

static void InsertReassignableStoreBacks(
    XTENSARegisterAllocator* allocator, TargetInstruction* target,
    TargetInstruction* spill, TargetInstruction* initial_definition) {
  TargetGenerator* gen = &allocator->rv->base;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      bool is_end = inst == block->end_code;
      if (inst != initial_definition &&
          (initial_definition == NULL || inst->id > initial_definition->id) &&
          (inst->flags & TARGET_INST_PROCESSED) != 0 && inst->reg != NULL &&
          IsReassignableDefinition(inst, target)) {
        TargetInstruction* store = TargetNewInstruction2(
            (TargetOpcode)XTENSA_OP(spill), target, spill->operand[1]);
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

static void SyncReassignableSpill(XTENSARegisterAllocator* allocator,
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
      (TargetOpcode)XTENSA_OP(spill), target, spill->operand[1]);
  store->reg = definition->reg;
  store->flags |= TARGET_INST_PROCESSED;
  TargetBasicBlockEmitAfter(&allocator->rv->base, definition->block, store,
                            definition);
}

static XTENSARegister* SpillInstruction(XTENSARegisterAllocator* allocator,
                                        TargetInstruction* inst) {
  XTENSARegister* reg = (XTENSARegister*)inst->reg;  // Current register.

  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2(
      (TargetOpcode)XTENSA_OP(spill), NULL,
      TargetGetIntConstant(&allocator->rv->base, NULL, kTargetType32Bit,
                           allocator->current_spilled_region_size));
  allocator->current_spilled_region_size += 8;  // Space for one register.
  if (allocator->current_spilled_region_size >
      allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size = allocator->current_spilled_region_size;
  }

  TargetInstruction* save = NULL;
  bool defined_before_save = false;
  if (inst->block == allocator->rv->base.entry_block) {
    for (TargetInstruction* current = inst->block->code; current != NULL;
         current = TargetNext(current)) {
      if (current == inst) {
        defined_before_save = true;
      }
      if ((XTENSAOpcode)current->opcode == XTENSA_OP(save)) {
        save = current;
        break;
      }
    }
  }
  // A reassignable spill is only a handle for its slot: the store happens after
  // whatever real definitions the value has, and a pseudo with none (an
  // incoming argument, say) never writes the slot at all.  A read of one of
  // those cannot be staged through the slot, so leave it naming the register.
  bool slot_always_written = true;
  if (XTENSAIsVarRegister(inst)) {
    // A varreg pseudo has no executable definition of its own. Its users are
    // not guaranteed to be in instruction order, and the first user need not
    // be an assignment, so keep this spill only as a slot handle and store
    // back after every real definition.
    TargetTrackOrphanInstruction(&allocator->rv->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    InsertReassignableStoreBacks(allocator, inst, spill, NULL);
    slot_always_written = false;
  } else if (InstructionHasExternalDefs(allocator, inst)) {
    // The declaration of a merge tmp has no value to store.  Keep the spill
    // instruction only as a handle for its slot and write the slot after each
    // real definition instead.
    TargetTrackOrphanInstruction(&allocator->rv->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    InsertReassignableStoreBacks(allocator, inst, spill, NULL);
    slot_always_written = false;
  } else if (defined_before_save && save != NULL) {
    // ABI argument and symbol pseudos are defined before the prologue.  Their
    // spill slot is frame-pointer-relative, so the store must execute only
    // after the save instruction has established the frame.
    TargetBasicBlockEmitAfter(&allocator->rv->base, save->block, spill, save);
  } else {
    // Emit spill instruction just after spilled instruction.
    TargetBasicBlockEmitAfter(&allocator->rv->base, inst->block, spill, inst);
  }

  // Retarget all uses of the original instruction to the spill.  If the
  // user has already been processed this will have no effect.
  // NOTE: this will transfer all uses of the inst to the spill, leaving
  // the users of inst empty and its uses count 0.
  TargetRetargetInstructionIf(inst, spill, NotProcessed, allocator);
  spill->uses = inst->uses;
  spill->operand[0] = inst;
  spill->reg = inst->reg;
  // Whatever is left over reads the register directly and has already been
  // allocated, so it has to be repaired in place.  The user list is left as it
  // is; nothing consults it once the value is marked spilled.
  if (slot_always_written) {
    for (size_t i = 0; i < inst->users.length; i++) {
      RepairProcessedRead(allocator, inst->users.value.p[i], inst, spill);
    }
  }
  reg->base.owner = NULL;
  inst->flags |= TARGET_INST_SPILLED;
  return reg;
}

static void EvictPhysicalRegister(XTENSARegisterAllocator* allocator,
                                  XTENSARegisterType type, int num,
                                  TargetInstruction* keep) {
  XTENSARegister* regs =
      type == kXTENSARegTypeInt ? allocator->int_regs : allocator->float_regs;
  TargetInstruction* owner = regs[num].base.owner;
  if (owner == NULL || owner == keep || regs[num].base.reserved) {
    return;
  }
  if ((owner->flags & TARGET_INST_SPILLED) != 0 ||
      !HasUnprocessedUserOtherThan(owner, NULL)) {
    regs[num].base.owner = NULL;
    return;
  }
  SpillInstruction(allocator, owner);
}

static XTENSARegister* AllocateRegisterWithType(
    XTENSARegisterAllocator* allocator, TargetBasicBlock* block,
    TargetInstruction* inst, XTENSARegisterType type, bool can_use_temp) {
  XTENSARegister* reg = FindFreeRegister(allocator, type, can_use_temp);

  if (reg == NULL) {
    TargetInstruction* victim = FindSpillVictim(allocator, type, can_use_temp);
    // FindSpillVictim releases the registers of any values it finds already
    // spilled, so a free one may have appeared.
    reg = FindFreeRegister(allocator, type, can_use_temp);
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
    case kXTENSARegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kXTENSARegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

static XTENSARegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(constf):
    case XTENSA_OP(constd):
    // ESP32 soft-float: the assembler has no FP register file / FP opcodes
    // (codegen still names RISC-V f* ops which the emitter nops).  Keep every
    // value — including float bit patterns — in the integer register file so
    // integer instructions never see an fN operand.
    case XTENSA_OP(tmp):
      return kXTENSARegTypeInt;

    default:
      return kXTENSARegTypeInt;
  }
}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(XTENSARegisterAllocator* allocator,
                       TargetInstruction* inst) {
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void AllocateVariableRegister(XTENSARegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                 !allocator->rv->not_leaf;

  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    if (var->inst != inst) {
      continue;
    }
    XTENSARegister* reg;
    if (var->is_fp) {
      int first =
          is_leaf ? XTENSA_FIRST_LEAF_FP_REG_VAR : XTENSA_FIRST_FP_REG_VAR;
      int last = is_leaf ? XTENSA_LAST_LEAF_FP_REG_VAR : XTENSA_LAST_FP_REG_VAR;
      if (var->varnum > last - first) {
        goto dynamic_alloc;
      }
      reg = &allocator->float_regs[first + var->varnum];
    } else {
      int first =
          is_leaf ? XTENSA_FIRST_LEAF_INT_REG_VAR : XTENSA_FIRST_INT_REG_VAR;
      int last =
          is_leaf ? XTENSA_LAST_LEAF_INT_REG_VAR : XTENSA_LAST_INT_REG_VAR;
      if (var->varnum > last - first) {
        goto dynamic_alloc;
      }
      reg = &allocator->int_regs[first + var->varnum];
    }
    AssignRegister(reg, inst);
    if (IsSavedReg(reg)) {
      if (var->is_fp) {
        BitSetInsert(&allocator->used_float_regs, reg->base.num);
      } else {
        BitSetInsert(&allocator->used_int_regs, reg->base.num);
      }
    }
    return;
  }

  assert(false);
  COMPILER_UNREACHABLE();

dynamic_alloc: {
  XTENSARegisterType reg_type = RegisterTypeFromInstruction(inst);
  XTENSARegister* reg = AllocateRegisterWithType(
      allocator, inst->block, inst, reg_type, CanUseTemp(allocator, inst));
  AssignRegister(reg, inst);
  if (IsSavedReg(reg)) {
    if (reg_type == kXTENSARegTypeFloat) {
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
    } else {
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
    }
  }
}
}

// Two-operand mv/fmv dests (argument copies, NRVO, staged t2) are often
// cached pseudos emitted in another block.  Allocate them here the same way
// AllocateUsingDest does, not only when the dest is a delayed varreg.
static void AllocateRmovEndpoint(XTENSARegisterAllocator* allocator,
                                 TargetInstruction* inst) {
  if (inst == NULL || inst->reg != NULL) {
    return;
  }
  if (XTENSAIsVarRegister(inst)) {
    AllocateVariableRegister(allocator, inst);
  } else if (!TargetIsConst(inst)) {
    AllocateRegister(allocator, inst);
  }
}

static COMPILER_UNUSED void AllocateForRmov(XTENSARegisterAllocator* allocator,
                                            TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)XTENSA_OP(mv)) ||
         ((int)inst->opcode == (int)XTENSA_OP(fmv_s)) ||
         ((int)inst->opcode == (int)XTENSA_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  AllocateRmovEndpoint(allocator, dest);
  XTENSARegister* reg = (XTENSARegister*)dest->reg;
  assert(reg != NULL);

  if (((int)src->opcode == (int)XTENSA_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)XTENSA_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    // It is no longer a register-to-register move, so it is no longer part of
    // the parallel copy that places the call's arguments.  Leaving the tag on
    // makes it look like a malformed member of that copy, and the resolver
    // gives up on the whole run -- emitting the remaining moves in their
    // original order, where an earlier one can overwrite an argument register
    // a later one still has to read.
    inst->flags &= ~XTENSA_INST_ARG_MOVE;
    TrapReload(inst);
  } else {
    AllocateRmovEndpoint(allocator, src);
    inst->operand[0]->uses++;
    FreeRegisters(allocator, inst);
  }

  inst->reg = &reg->base;
  inst->flags |= TARGET_INST_PROCESSED;
  SyncReassignableSpill(allocator, inst, dest);
}

static void ReloadSpills(XTENSARegisterAllocator* allocator,
                         TargetInstruction* inst);
static void EnsureOperandsAllocated(XTENSARegisterAllocator* allocator,
                                    TargetInstruction* inst);

// The single already-allocated definition of variable register `src`, or
// NULL when it has none or several.
static TargetInstruction* VariableRegisterDefinition(
    XTENSARegisterAllocator* allocator, TargetInstruction* src) {
  TargetInstruction* definition = NULL;
  int definitions = 0;
  for (size_t b = 0; b < allocator->rv->base.basic_blocks.length; b++) {
    TargetBasicBlock* block =
        VectorGet(&allocator->rv->base.basic_blocks, b);
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
static bool VariableRegisterDeadAfterCopy(XTENSARegisterAllocator* allocator,
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
static bool RegisterHasOtherLiveHolder(XTENSARegisterAllocator* allocator,
                                       TargetRegister* reg,
                                       TargetInstruction* keep1,
                                       TargetInstruction* keep2,
                                       TargetInstruction* keep3) {
  for (size_t b = 0; b < allocator->rv->base.basic_blocks.length; b++) {
    TargetBasicBlock* block =
        VectorGet(&allocator->rv->base.basic_blocks, b);
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
    XTENSARegisterAllocator* allocator, TargetInstruction* inst,
    TargetInstruction* varreg, TargetInstruction* src) {
  if (inst->block == NULL) {
    return false;
  }
  TargetInstruction* definition = src;
  if (XTENSAIsVarRegister(src)) {
    if (!VariableRegisterDeadAfterCopy(allocator, inst, src)) {
      return false;
    }
    definition = VariableRegisterDefinition(allocator, src);
    if (definition == NULL || definition->block != inst->block) {
      return false;
    }
  } else {
    if (src->block != inst->block || XTENSAIsFixedRegister(src) ||
        src->dest != NULL || src->uses != 1 ||
        (XTENSAOpcode)src->opcode == XTENSA_OP(tmp)) {
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
  switch ((XTENSAOpcode)definition->opcode) {
    case XTENSA_OP(call):
    case XTENSA_OP(rcall):
    case XTENSA_OP(callf):
    case XTENSA_OP(rcallf):
    case XTENSA_OP(resulti):
    case XTENSA_OP(resultf):
    case XTENSA_OP(resultd):
    case XTENSA_OP(atomic_load):
    case XTENSA_OP(atomic_fetch_add):
    case XTENSA_OP(atomic_fetch_sub):
    case XTENSA_OP(atomic_add_fetch):
    case XTENSA_OP(atomic_sub_fetch):
    case XTENSA_OP(atomic_compare_exchange_bool):
    case XTENSA_OP(atomic_compare_exchange_val):
    case XTENSA_OP(atomic_compare_exchange_n):
      return false;
    default:
      break;
  }
  if ((definition->flags & TARGET_INST_SPILLED) != 0 ||
      (src->flags & TARGET_INST_SPILLED) != 0 ||
      ((XTENSARegister*)varreg->reg)->type !=
          RegisterTypeFromInstruction(src)) {
    return false;
  }
  // A spilled variable's register may currently belong to another live
  // value (the copy would normally be routed through a scratch register
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
  // copy; writing the definition into the register here would clobber it
  // before the allocator's displaced-owner spill could preserve it.
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
  bool saved = IsSavedReg((XTENSARegister*)target);
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
      XTENSAOpcode opcode = (XTENSAOpcode)p->opcode;
      if (!saved && (opcode == XTENSA_OP(call) || opcode == XTENSA_OP(rcall) ||
                     opcode == XTENSA_OP(callf) ||
                     opcode == XTENSA_OP(rcallf))) {
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

// Record a callee-saved register taken over by a variable so the prologue
// saves and restores it, mirroring AllocateVariableRegister.
static void NoteUsedSavedRegister(XTENSARegisterAllocator* allocator,
                                  XTENSARegister* reg) {
  if (!IsSavedReg(reg)) {
    return;
  }
  if (reg->type == kXTENSARegTypeFloat) {
    BitSetInsert(&allocator->used_float_regs, reg->base.num);
  } else {
    BitSetInsert(&allocator->used_int_regs, reg->base.num);
  }
}

// A pure copy `mv src -> varreg` whose source dies at the copy need not
// move at all: either the variable takes over the source's register (when
// it has none yet), or the source's definition is retargeted to write the
// variable's register directly.  Both are only sound when the source has
// no other remaining use and is not live out of the block (so no successor,
// including one reached by a loop back edge, can still read it), when
// neither side is tied to another variable or a fixed register, and when
// a register the variable needs preserved across a call is callee-saved.
static bool TryCoalesceCopyIntoVariableRegister(
    XTENSARegisterAllocator* allocator, TargetInstruction* inst) {
  XTENSAOpcode opcode = (XTENSAOpcode)inst->opcode;
  if (opcode != XTENSA_OP(mv) && opcode != XTENSA_OP(fmv_s) &&
      opcode != XTENSA_OP(fmv_d)) {
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
  if (XTENSAIsFixedRegister(src)) {
    return false;
  }
  if (src->dest != NULL) {
    return false;
  }
  if (XTENSAIsVarRegister(src) &&
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
  XTENSARegister* reg = (XTENSARegister*)src->reg;
  if (reg->base.reserved ||
      reg->type != RegisterTypeFromInstruction(varreg)) {
    return false;
  }
  // The register must not be another variable's home, nor hold a value
  // live into this block: the variable keeps it for the rest of the
  // function, across regions where those owners are still read.
  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* variable = allocator->rv->var_regs.value.p[i];
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
  if (reg->type == kXTENSARegTypeInt &&
      allocator->rv->struct_return_reg >= 0) {
    for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
      RegisterVariable* variable = allocator->rv->var_regs.value.p[i];
      if (variable->inst == varreg &&
          variable->varnum == allocator->rv->struct_return_reg) {
        return false;
      }
    }
  }
  // The return-address register is in this allocator's temporary pool;
  // a variable must never inherit it (the first call overwrites it).
  if (reg->type == kXTENSARegTypeInt && reg->base.num == XTENSA_RET_REG) {
    return false;
  }
  // In a function that makes calls, variable registers are normally given
  // callee-saved registers outright (see AllocateVariableRegister); the
  // per-value preserved marking is only an approximation, so a variable in
  // such a function may only inherit a callee-saved register.
  if ((allocator->rv->base.num_calls > 0 || !CanUseTemp(allocator, inst) ||
       !CanUseTemp(allocator, varreg)) &&
      !IsSavedReg(reg)) {
    return false;
  }
  varreg->reg = &reg->base;
  varreg->uses = (int)varreg->users.length;
  varreg->flags |= TARGET_INST_PROCESSED;
  reg->base.owner = varreg;
  NoteUsedSavedRegister(allocator, reg);
  return true;
}

static bool AllocateUsingDest(XTENSARegisterAllocator* allocator,
                              TargetInstruction* inst) {
  if (inst->dest == NULL) {
    return false;
  }
  // A pure copy into a variable register may be coalesced away entirely;
  // that decision needs the source's register, so for copies the operands
  // are reloaded and allocated before the destination (below the usual
  // order is the other way around).
  bool operands_ready = false;
  if (XTENSAIsVarRegister(inst->dest) &&
      ((XTENSAOpcode)inst->opcode == XTENSA_OP(mv) ||
       (XTENSAOpcode)inst->opcode == XTENSA_OP(fmv_s) ||
       (XTENSAOpcode)inst->opcode == XTENSA_OP(fmv_d)) &&
      inst->operand[0] != NULL && inst->operand[1] == NULL) {
    ReloadSpills(allocator, inst);
    EnsureOperandsAllocated(allocator, inst);
    operands_ready = true;
    TryCoalesceCopyIntoVariableRegister(allocator, inst);
  }
  if (inst->dest->reg == NULL) {
    if (XTENSAIsVarRegister(inst->dest)) {
      AllocateVariableRegister(allocator, inst->dest);
    } else {
      AllocateRegister(allocator, inst->dest);
    }
  }
  assert(inst->dest->reg != NULL);
  XTENSARegisterType fixed_dest_type = kXTENSARegTypeInt;
  bool fixed_dest = true;
  switch ((XTENSAOpcode)inst->dest->opcode) {
    case XTENSA_OP(a0):
    case XTENSA_OP(a1):
    case XTENSA_OP(a2):
    case XTENSA_OP(a3):
    case XTENSA_OP(a4):
    case XTENSA_OP(a5):
    case XTENSA_OP(a6):
    case XTENSA_OP(a7):
    case XTENSA_OP(resulti):
      fixed_dest_type = kXTENSARegTypeInt;
      break;
    case XTENSA_OP(fa0):
    case XTENSA_OP(fa1):
    case XTENSA_OP(fa2):
    case XTENSA_OP(fa3):
    case XTENSA_OP(fa4):
    case XTENSA_OP(fa5):
    case XTENSA_OP(fa6):
    case XTENSA_OP(fa7):
    case XTENSA_OP(resultf):
    case XTENSA_OP(resultd):
      // Soft-float: float args/results share the integer arg/return regs.
      fixed_dest_type = kXTENSARegTypeInt;
      break;
    default:
      fixed_dest = false;
      break;
  }
  if (fixed_dest) {
    EvictPhysicalRegister(allocator, fixed_dest_type, inst->dest->reg->num,
                          inst->dest);
    inst->dest->reg->owner = inst->dest;
  }
  if (!operands_ready) {
    ReloadSpills(allocator, inst);
    EnsureOperandsAllocated(allocator, inst);
  }
  inst->reg = inst->dest->reg;
  if (inst->operand[0] != NULL) {
    inst->operand[0]->uses++;
  }
  FreeRegisters(allocator, inst);
  inst->flags |= TARGET_INST_PROCESSED;
  SyncReassignableSpill(allocator, inst, inst->dest);
  return true;
}

static void ReloadSpills(XTENSARegisterAllocator* allocator,
                         TargetInstruction* inst) {
  // Every operand is live when |inst| executes, so neither the registers they
  // already hold nor those reloads are given here may be picked for another
  // operand's reload or taken as its spill victim.  An operand can contribute
  // both the register it holds and its reload's.
  XTENSARegister* protect[2 * TARGET_MAX_OPERANDS];
  int num_protect = 0;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && op->reg != NULL && !op->reg->reserved) {
      op->reg->reserved = true;
      protect[num_protect++] = (XTENSARegister*)op->reg;
    }
  }
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ((int)op->opcode == (int)XTENSA_OP(spill))) {
      TargetInstruction* reload =
          TargetNewInstruction1((TargetOpcode)XTENSA_OP(reload), op);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->rv->base, inst->block, reload,
                                 inst);
      inst->operand[i] = reload;
      XTENSARegisterType reg_type = RegisterTypeFromInstruction(inst);
      XTENSARegister* reg =
          AllocateRegisterWithType(allocator, reload->block, reload, reg_type,
                                   CanUseTemp(allocator, reload));
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
  NoteReloadsInserted(allocator, inst);
}

static void EnsureOperandsAllocated(XTENSARegisterAllocator* allocator,
                                    TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op == NULL || op->reg != NULL || op->block == NULL ||
        TargetIsConst(op)) {
      continue;
    }
    if (XTENSAIsFixedRegister(op)) {
      AllocateRegister(allocator, op);
      continue;
    }
    if (XTENSAIsVarRegister(op)) {
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

static void AllocateRegisterOnce(XTENSARegisterAllocator* allocator,
                                 TargetInstruction* inst);

// Record that this instruction is mid-allocation while it runs, so that a spill
// triggered from inside knows its reads have already been resolved.
static void AllocateRegister(XTENSARegisterAllocator* allocator,
                             TargetInstruction* inst) {
  bool pushed = allocator->allocating_depth < XTENSA_MAX_ALLOCATION_DEPTH;
  if (pushed) {
    allocator->allocating_reloaded[allocator->allocating_depth] = false;
    allocator->allocating[allocator->allocating_depth++] = inst;
  }
  AllocateRegisterOnce(allocator, inst);
  if (pushed) {
    allocator->allocating_depth--;
  }
}

static void AllocateRegisterOnce(XTENSARegisterAllocator* allocator,
                                 TargetInstruction* inst) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                 !allocator->rv->not_leaf;

  XTENSAOpcode opcode = (XTENSAOpcode)inst->opcode;

  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }

  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == XTENSA_OP(mv) || opcode == XTENSA_OP(fmv_s) ||
       opcode == XTENSA_OP(fmv_d)) &&
      inst->dest == NULL && inst->operand[1] != NULL) {
    AllocateForRmov(allocator, inst);
    return;
  }

  if (XTENSAIsVarRegister(inst)) {
    // Variable regsiter.  Delay allocation until it's assigned to.
    // It will be assigned to by an rmov or from a destination
    // assignemnt.
    return;
  }

  if (AllocateUsingDest(allocator, inst)) {
    return;
  }

  // Reload any spilled expressions.
  ReloadSpills(allocator, inst);

  EnsureOperandsAllocated(allocator, inst);

  XTENSARegister* reg;

  // Free up any registers we can.
  FreeRegisters(allocator, inst);

  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(const8):
    case XTENSA_OP(const16):
    case XTENSA_OP(const32):
    case XTENSA_OP(const64):
    case XTENSA_OP(constf):
    case XTENSA_OP(constd):
    case XTENSA_OP(symbol):
    case XTENSA_OP(beq):
    case XTENSA_OP(bne):
    case XTENSA_OP(blt):
    case XTENSA_OP(bltu):
    case XTENSA_OP(bge):
    case XTENSA_OP(bgeu):
    case XTENSA_OP(j):
    case XTENSA_OP(label):
    case XTENSA_OP(ret):
    case XTENSA_OP(save):
    case XTENSA_OP(restore):
    case XTENSA_OP(literal):
    case XTENSA_OP(asm):
    case XTENSA_OP(loc):
    case XTENSA_OP(atomic_store):
    case XTENSA_OP(atomic_fence):
      // These instructions do not have registers allocated to them.  They still
      // have to be marked processed: their reads have had reloads inserted by
      // now, so a later spill of one of their operands must repair the read
      // rather than point it at a slot nothing will load from.
      inst->flags |= TARGET_INST_PROCESSED;
      return;

    case XTENSA_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;

    case XTENSA_OP(x0):
      reg = &allocator->int_regs[XTENSA_INT_ZERO_REG];
      break;

    case XTENSA_OP(fp):
      reg = &allocator->int_regs[XTENSA_FP_REG];
      break;

    case XTENSA_OP(sp):
      reg = &allocator->int_regs[XTENSA_SP_REG];
      break;

    case XTENSA_OP(t0):
      reg = &allocator->int_regs[XTENSA_INT_TEMP_START_1];
      break;

    case XTENSA_OP(t1):
      reg = &allocator->int_regs[XTENSA_INT_TEMP_START_1 + 1];
      break;

    case XTENSA_OP(t2):
      reg = &allocator->int_regs[XTENSA_INT_TEMP_START_1 + 2];
      break;

    case XTENSA_OP(a0):
    case XTENSA_OP(a1):
    case XTENSA_OP(a2):
    case XTENSA_OP(a3):
    case XTENSA_OP(a4):
    case XTENSA_OP(a5):
    case XTENSA_OP(a6):
    case XTENSA_OP(a7):
      reg = &allocator->int_regs[(int)inst->opcode - XTENSA_OP(a0) +
                                 XTENSA_INT_ARG_START];
      break;

    case XTENSA_OP(ivarreg):
    case XTENSA_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();

    case XTENSA_OP(fa0):
    case XTENSA_OP(fa1):
    case XTENSA_OP(fa2):
    case XTENSA_OP(fa3):
    case XTENSA_OP(fa4):
    case XTENSA_OP(fa5):
    case XTENSA_OP(fa6):
    case XTENSA_OP(fa7):
      // Soft-float: map faN onto the matching integer argument register.
      reg = &allocator->int_regs[(int)inst->opcode - XTENSA_OP(fa0) +
                                 XTENSA_INT_ARG_START];
      break;

    case XTENSA_OP(structreturn):
      // a9 is reloaded from the stack home before each use.  It is not an
      // argument register, so preparing the next call does not have to keep
      // the pointer live there across the call.
      (void)is_leaf;
      reg = &allocator->int_regs[XTENSA_INT_TEMP_START_1 + 1];
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;

    case XTENSA_OP(resulti):
      reg = &allocator->int_regs[XTENSA_INT_RETURN_REG];
      break;

    case XTENSA_OP(resultf):
    case XTENSA_OP(resultd):
      // Soft-float ABI: float results come back in the integer return reg.
      reg = &allocator->int_regs[XTENSA_INT_RETURN_REG];
      break;

    case XTENSA_OP(call):
    case XTENSA_OP(rcall):
    case XTENSA_OP(callf):
    case XTENSA_OP(rcallf):
      // CALL8 preserves a2-a7, but the emitter then copies the windowed
      // return (a10/a11) back into a2/a3.  A value still live in either
      // register does not survive that copy, so spill it before the result
      // takes the register.  Soft-float: float calls use the same int return.
      EvictPhysicalRegister(allocator, kXTENSARegTypeInt,
                            XTENSA_INT_RETURN_VALUE_0, inst);
      EvictPhysicalRegister(allocator, kXTENSARegTypeInt,
                            XTENSA_INT_RETURN_VALUE_1, inst);
      reg = &allocator->int_regs[XTENSA_INT_RETURN_REG];
      break;

    case XTENSA_OP(feq_s):
    case XTENSA_OP(flt_s):
    case XTENSA_OP(fle_s):
    case XTENSA_OP(feq_d):
    case XTENSA_OP(flt_d):
    case XTENSA_OP(fle_d):
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     kXTENSARegTypeInt,
                                     CanUseTemp(allocator, inst));
      break;

    default: {
      XTENSARegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, inst->block, inst, reg_type,
                                     CanUseTemp(allocator, inst));
    }
  }

  AssignRegister(reg, inst);

  // If nobody is using this register free it up immediately.
  // TODO: argument registers are not used explicitly but can't be freed here.
  if (inst->uses == 0 && !reg->base.reserved) {
    FreeRegister(allocator, reg);
  }
}

static void InitializeBasicBlockRegisters(XTENSARegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < XTENSA_NUM_INT_REGS; i++) {
    XTENSARegister* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < XTENSA_NUM_FLOAT_REGS; i++) {
    XTENSARegister* reg = &allocator->float_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }

  // Now allocate the registers to the inputs.
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* inst = block->inputs.value.p[i];
    if (inst->reg == NULL) {
      continue;
    }
    if (((int)inst->opcode == (int)XTENSA_OP(spill)) ||
        (inst->flags & TARGET_INST_SPILLED) != 0) {
      continue;
    }
    if (inst->uses == 0 && !HasUnprocessedUserOtherThan(inst, NULL)) {
      continue;
    }
    assert(inst->reg != NULL);
    if (inst->reg->owner == NULL || inst->id > inst->reg->owner->id) {
      inst->reg->owner = inst;
    }
  }
}

static void ProcessBlock(TargetBasicBlock* block, void* data) {
  TrapBlock(block);

  // printf("Allocating registers for block %zd\n", block->block_id);
  XTENSARegisterAllocator* allocator = data;

  // For a basic block, the inputs specify what instructions are alive
  // on entry.  An alive instruction has a register allocated to it.  All
  // other registers should be free at this point.
  InitializeBasicBlockRegisters(allocator, block);

#if 0
  // Unless we are the entry block, propagate the spill count from
  // the idom.  Use this to calculate the current spill region size
  // and thus offsets for spills in this block.
  if (block->idom != NULL) {
    block->num_spills = block->idom->num_spills;
    allocator->current_spilled_region_size = block->num_spills * 8;
  }
#endif

  for (TargetInstruction* inst = block->code;
       inst != NULL && inst != block->end_code; inst = TargetNext(inst)) {
    AllocateRegister(allocator, inst);
  }
  if (block->end_code != NULL) {
    AllocateRegister(allocator, block->end_code);
  }
}

static void ProcessBasicBlock(XTENSARegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->rv->base, ProcessBlock,
                              kTraversePreOrder, allocator);
}

// Variable registers are assigned a fixed physical register by index
// (FIRST_*_REG_VAR + varnum) and are allocated lazily at their first
// definition.  Those physical registers are drawn from the same range the
// dynamic allocator searches, so without reserving them the dynamic allocator
// can hand a variable's register to an unrelated value (e.g. a pooled
// loop-bound constant) before the variable is allocated, and the variable's
// later assignment then clobbers that still-live value.  Reserve them up front.
static void ReserveVariableRegisters(XTENSARegisterAllocator* allocator) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                 !allocator->rv->not_leaf;
  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    if (var->is_fp) {
      int first =
          is_leaf ? XTENSA_FIRST_LEAF_FP_REG_VAR : XTENSA_FIRST_FP_REG_VAR;
      int last = is_leaf ? XTENSA_LAST_LEAF_FP_REG_VAR : XTENSA_LAST_FP_REG_VAR;
      if (var->varnum > last - first) {
        continue;
      }
      allocator->float_regs[first + var->varnum].base.reserved = true;
    } else {
      int first =
          is_leaf ? XTENSA_FIRST_LEAF_INT_REG_VAR : XTENSA_FIRST_INT_REG_VAR;
      int last =
          is_leaf ? XTENSA_LAST_LEAF_INT_REG_VAR : XTENSA_LAST_INT_REG_VAR;
      if (var->varnum > last - first) {
        continue;
      }
      allocator->int_regs[first + var->varnum].base.reserved = true;
    }
  }
}

static bool HasAtomicInstructions(const XTENSARegisterAllocator* allocator) {
  for (size_t i = 0; i < allocator->rv->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = allocator->rv->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      if ((int)inst->opcode >= (int)XTENSA_OP(atomic_load) &&
          (int)inst->opcode <= (int)XTENSA_OP(atomic_fence)) {
        return true;
      }
      if (inst == block->end_code) {
        break;
      }
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// Parallel-copy resolution for call argument register moves.
//
// LowerCall emits the moves that place argument values into the physical
// argument registers (a0..a7 / fa0..fa7) as a sequence of independent register
// moves (tagged XTENSA_INST_ARG_MOVE).  Conceptually they are a *parallel*
// copy: every source is the value live just before the call and every
// destination is a distinct argument register.  Emitting them as naive
// sequential moves is wrong whenever a value already sits in an argument
// register another argument needs: an earlier move overwrites a register a
// later move still has to read (e.g. `mv a3,t6; mv a0,a3` loses the original
// a3), and true cycles (`a1<->a2`) cannot be done with plain copies at all.
//
// After register allocation every operand is a physical register, so we can
// resolve each run of consecutive argument moves as a proper parallel copy:
// emit a move only once its destination is no longer needed as a source, and
// break cycles by saving one register into a scratch temporary.
// ---------------------------------------------------------------------------

#define XTENSA_MAX_ARG_MOVES 32

// A copy whose source is a spill slot rather than a register.  It clobbers its
// destination like any other copy but reads nothing, so it never blocks one.
#define XTENSA_ARG_MOVE_MEMORY (-1)

typedef struct {
  int dst;                   // Destination physical register number.
  int src;                   // Source physical register number.
  XTENSARegisterType type;   // Register file (int or float).
  TargetOpcode opcode;       // Move opcode (mv / fmv.s / fmv.d).
  TargetInstruction* spill;  // Slot to reload from, for a memory source.
  bool done;
} XTENSAArgMove;

// Create a free-standing instruction that just carries a physical register so
// it can be used as a move operand.  It is never linked into the code list, so
// it is parked for teardown.
static TargetInstruction* XTENSARegHolder(XTENSARegisterAllocator* alloc,
                                          TargetBasicBlock* block, int num,
                                          XTENSARegisterType type) {
  TargetInstruction* h = TargetNewInstruction(TARGET_OP(tmp));
  XTENSARegister* reg = (type == kXTENSARegTypeInt) ? &alloc->int_regs[num]
                                                    : &alloc->float_regs[num];
  h->reg = &reg->base;
  h->block = block;
  TargetTrackOrphanInstruction(&alloc->rv->base, h);
  return h;
}

static bool XTENSARegInMoves(XTENSAArgMove* moves, int count,
                             XTENSARegisterType type, int num) {
  for (int i = 0; i < count; i++) {
    if (moves[i].type != type) {
      continue;
    }
    if (moves[i].dst == num ||
        (moves[i].src != XTENSA_ARG_MOVE_MEMORY && moves[i].src == num)) {
      return true;
    }
  }
  return false;
}

// Find a caller-saved temporary register of the given type that is not used by
// any move in the copy (so clobbering it is safe -- the moves immediately
// precede a call, which clobbers temporaries anyway).  Returns -1 if none.
static int XTENSAFindScratchTemp(XTENSAArgMove* moves, int count,
                                 XTENSARegisterType type) {
  if (type == kXTENSARegTypeInt) {
    for (int n = XTENSA_INT_TEMP_START_1; n <= XTENSA_INT_TEMP_END_1; n++) {
      if (n == XTENSA_INT_TEMP_START_1 + 2)
        continue;  // t2 stages indirect calls.
      if (!XTENSARegInMoves(moves, count, type, n)) return n;
    }
    for (int n = XTENSA_INT_TEMP_START_2; n <= XTENSA_INT_TEMP_END_2; n++) {
      if (!XTENSARegInMoves(moves, count, type, n)) return n;
    }
  } else {
    for (int n = XTENSA_FP_TEMP_START_1; n <= XTENSA_FP_TEMP_END_1; n++) {
      if (!XTENSARegInMoves(moves, count, type, n)) return n;
    }
    for (int n = XTENSA_FP_TEMP_START_2; n <= XTENSA_FP_TEMP_END_2; n++) {
      if (!XTENSARegInMoves(moves, count, type, n)) return n;
    }
  }
  return -1;
}

static void XTENSAEmitMove(XTENSARegisterAllocator* alloc,
                           TargetBasicBlock* block, TargetInstruction* pos,
                           const XTENSAArgMove* move_desc) {
  if (move_desc->src == XTENSA_ARG_MOVE_MEMORY) {
    TargetInstruction* reload = TargetNewInstruction1(
        (TargetOpcode)XTENSA_OP(reload), move_desc->spill);
    XTENSARegister* regs = (move_desc->type == kXTENSARegTypeInt)
                               ? alloc->int_regs
                               : alloc->float_regs;
    reload->reg = &regs[move_desc->dst].base;
    reload->flags |= TARGET_INST_PROCESSED;
    reload->uses = 1;
    TargetBasicBlockEmitBefore(&alloc->rv->base, block, reload, pos);
    return;
  }
  TargetInstruction* d =
      XTENSARegHolder(alloc, block, move_desc->dst, move_desc->type);
  TargetInstruction* s =
      XTENSARegHolder(alloc, block, move_desc->src, move_desc->type);
  TargetInstruction* move = TargetNewInstruction2(move_desc->opcode, d, s);
  TargetBasicBlockEmitBefore(&alloc->rv->base, block, move, pos);
}

// Emit the parallel copy described by `moves` as correctly ordered move
// instructions inserted before `pos`.  All destinations are distinct.
static void XTENSAResolveParallelCopy(XTENSARegisterAllocator* alloc,
                                      TargetBasicBlock* block,
                                      TargetInstruction* pos,
                                      XTENSAArgMove* moves, int count) {
  int remaining = 0;
  for (int i = 0; i < count; i++) {
    moves[i].done = (moves[i].dst == moves[i].src);  // Self-moves are no-ops.
    if (!moves[i].done) {
      remaining++;
    }
  }

  while (remaining > 0) {
    bool progressed = false;
    for (int i = 0; i < count; i++) {
      if (moves[i].done) {
        continue;
      }
      // Ready when no other unfinished move still needs to read this
      // destination's current contents.
      bool blocked = false;
      for (int j = 0; j < count; j++) {
        if (moves[j].done || j == i) {
          continue;
        }
        if (moves[j].type == moves[i].type &&
            moves[j].src != XTENSA_ARG_MOVE_MEMORY &&
            moves[j].src == moves[i].dst) {
          blocked = true;
          break;
        }
      }
      if (blocked) {
        continue;
      }
      XTENSAEmitMove(alloc, block, pos, &moves[i]);
      moves[i].done = true;
      remaining--;
      progressed = true;
    }
    if (progressed) {
      continue;
    }

    // Every remaining move is part of a cycle.  Break one by saving its
    // destination into a scratch temporary and redirecting its readers.
    int pick = -1;
    for (int i = 0; i < count; i++) {
      if (!moves[i].done) {
        pick = i;
        break;
      }
    }
    XTENSARegisterType type = moves[pick].type;
    int scratch = XTENSAFindScratchTemp(moves, count, type);
    if (scratch < 0) {
      // No scratch available (extremely unlikely for argument setup).  Emit the
      // remaining moves in order as a best effort rather than risk an overflow.
      for (int i = 0; i < count; i++) {
        if (moves[i].done) {
          continue;
        }
        XTENSAEmitMove(alloc, block, pos, &moves[i]);
        moves[i].done = true;
      }
      break;
    }
    XTENSAArgMove save = {.dst = scratch,
                          .src = moves[pick].dst,
                          .type = type,
                          .opcode = moves[pick].opcode};
    XTENSAEmitMove(alloc, block, pos, &save);
    for (int j = 0; j < count; j++) {
      if (!moves[j].done && moves[j].type == type &&
          moves[j].src != XTENSA_ARG_MOVE_MEMORY &&
          moves[j].src == moves[pick].dst) {
        moves[j].src = scratch;
      }
    }
  }
}

// Rewrite each run of consecutive XTENSA_INST_ARG_MOVE moves in `block` as a
// correctly ordered parallel copy.
static void XTENSAResolveArgumentMovesInBlock(XTENSARegisterAllocator* alloc,
                                              TargetBasicBlock* block) {
  TargetInstruction* inst = TargetBasicBlockBegin(block);
  TargetInstruction* end = TargetBasicBlockEnd(block);
  while (inst != end && inst != NULL) {
    if ((inst->flags & XTENSA_INST_ARG_MOVE) == 0) {
      inst = TargetNext(inst);
      continue;
    }

    // Gather the maximal run of consecutive argument moves.  A reload for a
    // spilled argument is part of the copy too: it was given the argument's
    // register directly, so it clobbers that register and has to be ordered
    // with the moves rather than splitting the run in two.
    TargetInstruction* run[XTENSA_MAX_ARG_MOVES];
    int run_count = 0;
    int last_move;
    bool well_formed = true;
    TargetInstruction* scan = inst;
    while (scan != end && scan != NULL &&
           ((scan->flags & XTENSA_INST_ARG_MOVE) != 0 ||
            (int)scan->opcode == (int)XTENSA_OP(reload))) {
      if (run_count >= XTENSA_MAX_ARG_MOVES) {
        well_formed = false;
        break;
      }
      if ((int)scan->opcode == (int)XTENSA_OP(reload)) {
        if (scan->reg == NULL || scan->operand[0] == NULL ||
            ((int)scan->operand[0]->opcode != (int)XTENSA_OP(spill))) {
          break;
        }
      } else if (scan->operand[0] == NULL || scan->operand[1] == NULL ||
                 scan->operand[0]->reg == NULL ||
                 scan->operand[1]->reg == NULL ||
                 scan->operand[1]->block == NULL) {
        well_formed = false;
      }
      run[run_count++] = scan;
      scan = TargetNext(scan);
    }
    // A reload that stages a spilled argument through a scratch register is
    // folded into the move that reads it, which becomes a memory-sourced copy.
    // That keeps the whole argument setup in one parallel copy.  A reload with
    // several readers, or one used as a move destination, cannot be folded and
    // has to stay ahead of the copy, so the run ends there.
    for (int r = 0; r < run_count; r++) {
      if ((int)run[r]->opcode != (int)XTENSA_OP(reload)) {
        continue;
      }
      int readers = 0;
      bool as_destination = false;
      for (int j = r + 1; j < run_count; j++) {
        if ((int)run[j]->opcode == (int)XTENSA_OP(reload)) {
          continue;
        }
        if (run[j]->operand[0] == run[r]) {
          as_destination = true;
        }
        if (run[j]->operand[1] == run[r]) {
          readers++;
        }
      }
      if (as_destination || readers > 1 ||
          (readers == 1 && run[r]->uses != 1)) {
        run_count = r;
        scan = run[r];
        break;
      }
    }
    // Trailing reloads belong to whatever follows the run, not to the copy.
    last_move = -1;
    for (int i = 0; i < run_count; i++) {
      if ((int)run[i]->opcode != (int)XTENSA_OP(reload)) {
        last_move = i;
      }
    }
    while (run_count > last_move + 1) {
      run_count--;
      scan = run[run_count];
    }
    if (run_count == 0) {
      // Nothing usable: step past the first move so the scan makes progress.
      inst = TargetNext(inst);
      continue;
    }
    TargetInstruction* after_run = scan;

    // Pair each foldable reload with the move that reads it.  The move takes
    // over the reload's spill slot as its source and the reload contributes no
    // copy of its own.
    TargetInstruction* memory_source[XTENSA_MAX_ARG_MOVES];
    bool folded[XTENSA_MAX_ARG_MOVES];
    for (int i = 0; i < run_count; i++) {
      memory_source[i] = NULL;
      folded[i] = false;
    }
    for (int r = 0; r < run_count; r++) {
      if ((int)run[r]->opcode != (int)XTENSA_OP(reload)) {
        continue;
      }
      for (int j = r + 1; j < run_count; j++) {
        if ((int)run[j]->opcode == (int)XTENSA_OP(reload) ||
            run[j]->operand[1] != run[r]) {
          continue;
        }
        memory_source[j] = run[r]->operand[0];
        folded[r] = true;
        break;
      }
    }

    // Build the parallel-copy move set.
    XTENSAArgMove moves[XTENSA_MAX_ARG_MOVES];
    int count = 0;
    bool duplicate_dst = false;
    if (well_formed) {
      for (int i = 0; i < run_count; i++) {
        XTENSARegisterType type;
        int dst;
        if (folded[i]) {
          continue;
        }
        if ((int)run[i]->opcode == (int)XTENSA_OP(reload)) {
          XTENSARegister* reg = (XTENSARegister*)run[i]->reg;
          type = reg->type;
          dst = reg->base.num;
          moves[count].src = XTENSA_ARG_MOVE_MEMORY;
          moves[count].spill = run[i]->operand[0];
        } else if (memory_source[i] != NULL) {
          XTENSARegister* dreg = (XTENSARegister*)run[i]->operand[0]->reg;
          type = dreg->type;
          dst = dreg->base.num;
          moves[count].src = XTENSA_ARG_MOVE_MEMORY;
          moves[count].spill = memory_source[i];
        } else {
          XTENSARegister* dreg = (XTENSARegister*)run[i]->operand[0]->reg;
          XTENSARegister* sreg = (XTENSARegister*)run[i]->operand[1]->reg;
          type = (dreg->type == kXTENSARegTypeFloat ||
                  sreg->type == kXTENSARegTypeFloat)
                     ? kXTENSARegTypeFloat
                     : kXTENSARegTypeInt;
          dst = dreg->base.num;
          moves[count].src = sreg->base.num;
          moves[count].spill = NULL;
        }
        for (int j = 0; j < count; j++) {
          if (moves[j].type == type && moves[j].dst == dst) {
            duplicate_dst = true;
          }
        }
        moves[count].dst = dst;
        moves[count].type = type;
        moves[count].opcode = (TargetOpcode)XTENSA_OP(mv);
        if ((int)run[i]->opcode != (int)XTENSA_OP(reload)) {
          moves[count].opcode = run[i]->opcode;
        }
        moves[count].done = false;
        count++;
      }
    }

    // Only rewrite when it is safe to do so; otherwise leave the run untouched.
    if (well_formed && !duplicate_dst) {
      XTENSAResolveParallelCopy(alloc, block, run[0], moves, count);
      // Neutralise the originals rather than deleting them: the register
      // allocator has already consumed the use counts on their operands, so
      // deleting would underflow them.  Pointing both operands at the same
      // register makes the emitter skip them (it never emits `mv rx, rx`).
      for (int i = 0; i < run_count; i++) {
        if ((int)run[i]->opcode == (int)XTENSA_OP(reload)) {
          TargetInstruction* holder = XTENSARegHolder(
              alloc, block, ((XTENSARegister*)run[i]->reg)->base.num,
              ((XTENSARegister*)run[i]->reg)->type);
          run[i]->opcode = (TargetOpcode)XTENSA_OP(mv);
          run[i]->operand[0] = holder;
          run[i]->operand[1] = holder;
          continue;
        }
        run[i]->operand[1] = run[i]->operand[0];
      }
    }
    inst = after_run;
  }
}

static void XTENSAResolveArgumentMoves(XTENSARegisterAllocator* alloc) {
  Vector* blocks = &alloc->rv->base.basic_blocks;
  for (size_t i = 0; i < blocks->length; i++) {
    TargetBasicBlock* block = blocks->value.p[i];
    if (block == NULL || TargetBasicBlockIsEmpty(block)) {
      continue;
    }
    XTENSAResolveArgumentMovesInBlock(alloc, block);
  }
}

static TargetInstruction* CreateCallResultCopy(TargetInstruction* call) {
  XTENSAOpcode call_opcode = (XTENSAOpcode)call->opcode;
  TargetOpcode move_opcode =
      call_opcode == XTENSA_OP(callf) || call_opcode == XTENSA_OP(rcallf)
          ? (TargetOpcode)XTENSA_OP(fmv_d)
          : (TargetOpcode)XTENSA_OP(mv);
  return TargetNewInstruction1(move_opcode, call);
}

void XTENSAAllocateRegisters(XTENSARegisterAllocator* allocator) {
  // t1 and t2 are required as untracked scratch registers by the atomic
  // emitter, while t2 also stages indirect call targets.  Functions without
  // those hazards can use them for ordinary short-lived values.
  bool has_atomics = HasAtomicInstructions(allocator);
  if (!has_atomics) {
    allocator->int_regs[XTENSA_INT_TEMP_START_1 + 1].base.reserved = false;
  }
  if (!has_atomics && allocator->rv->base.num_calls == 0 &&
      !allocator->rv->stages_tail_target) {
    allocator->int_regs[XTENSA_INT_TEMP_START_1 + 2].base.reserved = false;
  }

  if (allocator->rv->struct_return_reg >= 0) {
    bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                   !allocator->rv->not_leaf;
    int reg_num =
        (is_leaf ? XTENSA_FIRST_LEAF_INT_REG_VAR : XTENSA_FIRST_INT_REG_VAR) +
        allocator->rv->struct_return_reg;
    allocator->int_regs[reg_num].base.reserved = true;
  }

  TargetMarkCallPreservedInstructions(&allocator->rv->base,
                                      &allocator->preserved_instructions);
  if (TargetMaterializePreservedCallResults(&allocator->rv->base,
                                            &allocator->preserved_instructions,
                                            CreateCallResultCopy)) {
    TargetBuildBasicBlockInputsAndOutputs(&allocator->rv->base);
    BitSetClear(&allocator->preserved_instructions);
    TargetMarkCallPreservedInstructions(&allocator->rv->base,
                                        &allocator->preserved_instructions);
  }

  ReserveVariableRegisters(allocator);

  // Process all basic blocks in the XTENSA generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->rv->base.entry_block);

  // Fix up argument-register moves that the per-instruction allocation may have
  // left as a clobbering sequence (see XTENSAResolveArgumentMoves).
  XTENSAResolveArgumentMoves(allocator);
}

const char* XTENSARegisterName(XTENSARegister* reg, char* buf, size_t len) {
  return XTENSARegisterNameFromNum(reg->base.num, reg->type, buf, len);
}

const char* XTENSARegisterNameFromNum(int num, XTENSARegisterType type,
                                      char* buf, size_t len) {
  switch (type) {
    case kXTENSARegTypeInt:
      snprintf(buf, len, "a%d", num);
      break;

    case kXTENSARegTypeFloat:
      snprintf(buf, len, "f%d", num);
      break;
  }
  return buf;
}

#undef NUM_REG_RANGES
