//
//  risc_v32_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v32_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include "risc_v32_codegen.h"
#include "risc_v32_machine.h"
#include "target_basic_block.h"
#include "compiler.h"


static void AllocateRegister(RV32RegisterAllocator* allocator,
                             TargetInstruction* inst);
static RV32RegisterType RegisterTypeFromInstruction(TargetInstruction* inst);

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

static void InitializeRegister(RV32Register* reg, int num, RV32RegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void RV32RegisterAllocatorInit(RV32RegisterAllocator* allocator, RV32Generator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < RV32_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kRV32RegTypeInt);
  }

  for (int i = 0; i < RV32_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kRV32RegTypeFloat);
  }

  // Reserve some registers.
  allocator->int_regs[RV32_INT_ZERO_REG].base.reserved = true;
  allocator->int_regs[RV32_FP_REG].base.reserved = true;
  allocator->int_regs[RV32_SP_REG].base.reserved = true;
  allocator->int_regs[RV32_SPILL_ADDR].base.reserved = true;
  allocator->int_regs[RV32_INT_TEMP_START_1].base.reserved = true;
  allocator->int_regs[RV32_INT_TEMP_START_1 + 1].base.reserved = true;
  allocator->int_regs[RV32_INT_TEMP_START_1 + 2].base.reserved = true;
  allocator->int_regs[RV32_INT_TEMP_START_2].base.reserved = true;
  allocator->int_regs[RV32_INT_TEMP_START_2 + 1].base.reserved = true;
  allocator->int_regs[RV32_INT_TEMP_START_2 + 2].base.reserved = true;
  allocator->int_regs[RV32_INT_TEMP_START_2 + 3].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
  MapInitForPointerKeys(&allocator->reassignable_spills);
  allocator->allocating_depth = 0;
}

RV32RegisterAllocator* NewRV32RegisterAllocator(RV32Generator* pcode) {
  RV32RegisterAllocator* reg_alloc = malloc(sizeof(RV32RegisterAllocator));
  RV32RegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void RV32RegisterAllocatorDestruct(RV32RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
  MapDestruct(&allocator->reassignable_spills);
}

void RV32RegisterAllocatorDelete(RV32RegisterAllocator* alloc) {
  RV32RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// RISC-V32's ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  RV32RegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  const char* prefix;   // Register name prefix.
  int base;
  bool temp;
} register_ranges[] = {
    {kRV32RegTypeInt, RV32_INT_TEMP_START_1, RV32_INT_TEMP_END_1, "t", 0, true},
    {kRV32RegTypeInt, RV32_INT_TEMP_START_2, RV32_INT_TEMP_END_2, "t", 3, true},
    {kRV32RegTypeInt, RV32_INT_ARG_START, RV32_INT_ARG_END, "a", 0, true},
    {kRV32RegTypeInt, RV32_RET_REG, RV32_RET_REG, "ra", 0, true},
    {kRV32RegTypeInt, RV32_INT_SAVED_START_1, RV32_INT_SAVED_END_1, "s", 0, false},
    {kRV32RegTypeInt, RV32_INT_SAVED_START_2, RV32_INT_SAVED_END_2, "s", 2, false},
    {kRV32RegTypeFloat, RV32_FP_TEMP_START_1, RV32_FP_TEMP_END_1, "ft", 0, true},
    {kRV32RegTypeFloat, RV32_FP_TEMP_START_2, RV32_FP_TEMP_END_2, "ft", 2, true},
    {kRV32RegTypeFloat, RV32_FP_ARG_START, RV32_FP_ARG_END, "fa", 0, true},
    {kRV32RegTypeFloat, RV32_FP_SAVED_START_1, RV32_FP_SAVED_END_1, "fs", 0, false},
    {kRV32RegTypeFloat, RV32_FP_SAVED_START_2, RV32_FP_SAVED_END_2, "fs", 2, false},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(RV32RegisterAllocator* allocator) {
  char buf[32];
  for (RV32RegisterType type = kRV32RegTypeInt; type <= kRV32RegTypeFloat; type++) {
    RV32Register* regs =
        type == kRV32RegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(x%d): free\n", RV32RegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(x%d): owner: @%d %s\n", RV32RegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num, owner->id,
                   RV32OpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(RV32Register* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static RV32Register* FindFreeRegister(RV32RegisterAllocator* allocator,
                                    RV32RegisterType type, bool can_use_temp) {
  RV32Register* regs =
      type == kRV32RegTypeInt ? allocator->int_regs : allocator->float_regs;
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
          if (regs[j].base.num == RV32_RET_REG) {
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

static void FreeRegister(RV32RegisterAllocator* allocator, RV32Register* reg) {
  reg->base.owner = NULL;
}

static bool HasUnprocessedUserOtherThan(TargetInstruction* value,
                                        TargetInstruction* current) {
  for (size_t i = 0; i < value->users.length; i++) {
    TargetInstruction* user = value->users.value.p[i];
    if (user != current &&
        (user->flags & TARGET_INST_PROCESSED) == 0) {
      return true;
    }
  }
  return false;
}

// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(RV32RegisterAllocator* allocator,
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
      if (RV32IsFixedRegister(op)) {
        continue;
      }
      if (RV32IsVarRegister(op)) {
        continue;
      }
      // Inside a loop, a value that is live-out of the block is read again on a
      // later iteration through the back edge, so its linear "last use" in this
      // block is not really its last use.  Freeing its register here would let a
      // subsequent temp reuse it and clobber the still-live value (e.g. a pooled
      // loop-bound constant sharing a register with the loop variable).  Restrict
      // this to loop blocks so straight-line code keeps freeing registers
      // promptly.
      if (inst->block != NULL && inst->block->loop_nesting > 0 &&
          BitSetContains(&inst->block->output_ids, op->id)) {
        continue;
      }
#if 0
      if (((int)op->opcode == (int)RV32_OP(reload)) || ((int)op->opcode == (int)RV32_OP(spill))) {
        continue;
      }
#endif
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL) {
        if (op->uses > 0) {
          op->uses--;
        }
        if (!HasUnprocessedUserOtherThan(op, inst) && reg->owner == op) {
          FreeRegister(allocator, (RV32Register*)reg);
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
      FreeRegister(allocator, (RV32Register*)inst->dest->reg);
    }
  }
#endif
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(RV32Register* reg) {
  int num = reg->base.num;
  if ((num >= RV32_INT_SAVED_START_1 && num <= RV32_INT_SAVED_END_1) ||
      (num >= RV32_INT_SAVED_START_2 && num <= RV32_INT_SAVED_END_2) ||
      (num >= RV32_FP_SAVED_START_1 && num <= RV32_FP_SAVED_END_1) ||
      (num >= RV32_FP_SAVED_START_2 && num <= RV32_FP_SAVED_END_2)) {
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
  RV32Opcode opcode = (RV32Opcode)inst->opcode;
  return (opcode == RV32_OP(mv) || opcode == RV32_OP(fmv_s) ||
          opcode == RV32_OP(fmv_d)) &&
         inst->dest == NULL && inst->operand[0] == target &&
         inst->operand[1] != NULL;
}

static bool InstructionHasExternalDefs(RV32RegisterAllocator* allocator,
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
static TargetInstruction* FindSpillVictim(RV32RegisterAllocator* allocator,
                                   RV32RegisterType type, bool can_use_temp) {
  RV32Register* regs =
      type == kRV32RegTypeInt ? allocator->int_regs : allocator->float_regs;
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
              ((int)owner->opcode == (int)RV32_OP(spill))) {
            // Already in memory, so it is not holding this register any more.
            regs[j].base.owner = NULL;
            continue;
          }
          if (RV32IsVarRegister(owner)) {
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
static bool IsBeingAllocated(RV32RegisterAllocator* allocator,
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
static void NoteReloadsInserted(RV32RegisterAllocator* allocator,
                                TargetInstruction* inst) {
  for (size_t d = 0; d < allocator->allocating_depth; d++) {
    if (allocator->allocating[d] == inst) {
      allocator->allocating_reloaded[d] = true;
      return;
    }
  }
}

static bool NotProcessed(TargetInstruction* inst, void* data) {
  RV32RegisterAllocator* allocator = data;
  return (inst->flags & TARGET_INST_PROCESSED) == 0 &&
         !IsBeingAllocated(allocator, inst);
}

// A read that has already been allocated names the physical register the value
// was in, and spilling hands that register to something else.  Stage the slot
// through the dedicated scratch register immediately before the read instead.
// Returns false when the read cannot be repaired, which leaves it as it was.
static bool RepairProcessedRead(RV32RegisterAllocator* allocator,
                                TargetInstruction* user,
                                TargetInstruction* value,
                                TargetInstruction* spill) {
  if (user->block == NULL || value->reg == NULL) {
    return false;
  }
  if ((int)user->opcode == (int)RV32_OP(spill) ||
      (int)user->opcode == (int)RV32_OP(reload)) {
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
  if (((RV32Register*)value->reg)->type != kRV32RegTypeInt ||
      RegisterTypeFromInstruction(user) != kRV32RegTypeInt) {
    // There is no scratch register in the floating point file, and the read has
    // to want an integer register for the staged one to be usable.
    return false;
  }
  // t0 is the code generator's staging temporary and is never handed out by the
  // allocator, so it is free between the reload and the read that follows it.
  RV32Register* scratch = &allocator->int_regs[RV32_INT_TEMP_START_1];
  // There is one scratch register, so a read that already stages something else
  // through it cannot stage this slot too.
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = user->operand[i];
    if (op != NULL && op != value && op->reg == &scratch->base) {
      return false;
    }
  }
  TargetInstruction* reload =
      TargetNewInstruction1((TargetOpcode)RV32_OP(reload), spill);
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
    RV32RegisterAllocator* allocator, TargetInstruction* target,
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
            (TargetOpcode)RV32_OP(spill), target, spill->operand[1]);
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

static void SyncReassignableSpill(RV32RegisterAllocator* allocator,
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
      (TargetOpcode)RV32_OP(spill), target, spill->operand[1]);
  store->reg = definition->reg;
  store->flags |= TARGET_INST_PROCESSED;
  TargetBasicBlockEmitAfter(&allocator->rv->base, definition->block, store,
                            definition);
}

static RV32Register* SpillInstruction(RV32RegisterAllocator* allocator, TargetInstruction* inst) {
  RV32Register* reg = (RV32Register*)inst->reg;    // Current register.
  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)RV32_OP(spill), NULL,
                                                   TargetGetIntConstant(&allocator->rv->base,
                                                                        NULL,
                                                                        kTargetType32Bit,
                                                                        allocator->current_spilled_region_size));
  allocator->current_spilled_region_size += 8;    // Space for one register.
  if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
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
      if ((RV32Opcode)current->opcode == RV32_OP(save)) {
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
  if (RV32IsVarRegister(inst)) {
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

static void EvictPhysicalRegister(RV32RegisterAllocator* allocator,
                                  RV32RegisterType type, int num,
                                  TargetInstruction* keep) {
  RV32Register* regs =
      type == kRV32RegTypeInt ? allocator->int_regs : allocator->float_regs;
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

static RV32Register* AllocateRegisterWithType(RV32RegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            RV32RegisterType type,
                                            bool can_use_temp) {
  RV32Register* reg = FindFreeRegister(allocator, type, can_use_temp);

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
    case kRV32RegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kRV32RegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

static RV32RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(constf):
    case RV32_OP(constd):
    case RV32_OP(fmv_s):
    case RV32_OP(fmv_d):
    case RV32_OP(fa0):
    case RV32_OP(fa1):
    case RV32_OP(fa2):
    case RV32_OP(fa3):
    case RV32_OP(fa4):
    case RV32_OP(fa5):
    case RV32_OP(fa6):
    case RV32_OP(fa7):
    case RV32_OP(fvarreg):
    case RV32_OP(fmv_w_x):
    case RV32_OP(fneg_d):
    case RV32_OP(fneg_s):
    case RV32_OP(fcvt_s_l):
    case RV32_OP(fcvt_d_l):
      return kRV32RegTypeFloat;

    case RV32_OP(fcvt_w_d):
    case RV32_OP(fcvt_wu_d):
    case RV32_OP(fcvt_l_s):
    case RV32_OP(fcvt_lu_s):
    case RV32_OP(fcvt_l_d):
    case RV32_OP(fcvt_lu_d):
    case RV32_OP(fmv_x_d):
      return kRV32RegTypeInt;

    case RV32_OP(tmp):
      // A tmp placeholder is integer unless tagged as holding a float value.
      return (inst->flags & RV32_INST_FLOAT_TMP) ? kRV32RegTypeFloat : kRV32RegTypeInt;

    default:
      if ((RV32Opcode)inst->opcode >= RV32_OP(flw) &&
          (RV32Opcode)inst->opcode <= RV32_OP(fmv_d_x)) {
        return kRV32RegTypeFloat;
      }
      return kRV32RegTypeInt;
  }
}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(RV32RegisterAllocator* allocator, TargetInstruction* inst) {
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void AllocateVariableRegister(RV32RegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                 !allocator->rv->not_leaf;

  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    if (var->inst != inst) {
      continue;
    }
    RV32Register* reg;
    if (var->is_fp) {
      int first = is_leaf ? RV32_FIRST_LEAF_FP_REG_VAR : RV32_FIRST_FP_REG_VAR;
      int last = is_leaf ? RV32_LAST_LEAF_FP_REG_VAR : RV32_LAST_FP_REG_VAR;
      if (var->varnum > last - first) {
        goto dynamic_alloc;
      }
      reg = &allocator->float_regs[first + var->varnum];
    } else {
      int first = is_leaf ? RV32_FIRST_LEAF_INT_REG_VAR : RV32_FIRST_INT_REG_VAR;
      int last = is_leaf ? RV32_LAST_LEAF_INT_REG_VAR : RV32_LAST_INT_REG_VAR;
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

dynamic_alloc:
  {
    RV32RegisterType reg_type = RegisterTypeFromInstruction(inst);
    RV32Register* reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                             reg_type, CanUseTemp(allocator, inst));
    AssignRegister(reg, inst);
    if (IsSavedReg(reg)) {
      if (reg_type == kRV32RegTypeFloat) {
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
static void AllocateRmovEndpoint(RV32RegisterAllocator* allocator,
                                 TargetInstruction* inst) {
  if (inst == NULL || inst->reg != NULL) {
    return;
  }
  if (RV32IsVarRegister(inst)) {
    AllocateVariableRegister(allocator, inst);
  } else if (!TargetIsConst(inst)) {
    AllocateRegister(allocator, inst);
  }
}

static COMPILER_UNUSED void AllocateForRmov(RV32RegisterAllocator* allocator,
                            TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)RV32_OP(mv)) || ((int)inst->opcode == (int)RV32_OP(fmv_s)) ||
         ((int)inst->opcode == (int)RV32_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  AllocateRmovEndpoint(allocator, dest);
  RV32Register* reg = (RV32Register*)dest->reg;
  assert(reg != NULL);
  
  if (((int)src->opcode == (int)RV32_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)RV32_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    // It is no longer a register-to-register move, so it is no longer part of
    // the parallel copy that places the call's arguments.  Leaving the tag on
    // makes it look like a malformed member of that copy, and the resolver
    // gives up on the whole run -- emitting the remaining moves in their
    // original order, where an earlier one can overwrite an argument register
    // a later one still has to read.
    inst->flags &= ~RV32_INST_ARG_MOVE;
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

static void ReloadSpills(RV32RegisterAllocator* allocator,
                         TargetInstruction* inst);
static void EnsureOperandsAllocated(RV32RegisterAllocator* allocator,
                                    TargetInstruction* inst);

static bool AllocateUsingDest(RV32RegisterAllocator* allocator,
                              TargetInstruction* inst) {
  if (inst->dest == NULL) {
    return false;
  }
  if (inst->dest->reg == NULL) {
    if (RV32IsVarRegister(inst->dest)) {
      AllocateVariableRegister(allocator, inst->dest);
    } else {
      AllocateRegister(allocator, inst->dest);
    }
  }
  assert(inst->dest->reg != NULL);
  RV32RegisterType fixed_dest_type = kRV32RegTypeInt;
  bool fixed_dest = true;
  switch ((RV32Opcode)inst->dest->opcode) {
    case RV32_OP(a0):
    case RV32_OP(a1):
    case RV32_OP(a2):
    case RV32_OP(a3):
    case RV32_OP(a4):
    case RV32_OP(a5):
    case RV32_OP(a6):
    case RV32_OP(a7):
    case RV32_OP(resulti):
      fixed_dest_type = kRV32RegTypeInt;
      break;
    case RV32_OP(fa0):
    case RV32_OP(fa1):
    case RV32_OP(fa2):
    case RV32_OP(fa3):
    case RV32_OP(fa4):
    case RV32_OP(fa5):
    case RV32_OP(fa6):
    case RV32_OP(fa7):
    case RV32_OP(resultf):
    case RV32_OP(resultd):
      fixed_dest_type = kRV32RegTypeFloat;
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
  ReloadSpills(allocator, inst);
  EnsureOperandsAllocated(allocator, inst);
  inst->reg = inst->dest->reg;
  if (inst->operand[0] != NULL) {
    inst->operand[0]->uses++;
  }
  FreeRegisters(allocator, inst);
  inst->flags |= TARGET_INST_PROCESSED;
  SyncReassignableSpill(allocator, inst, inst->dest);
  return true;
}


static void ReloadSpills(RV32RegisterAllocator* allocator,
                         TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ((int)op->opcode == (int)RV32_OP(spill))) {
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)RV32_OP(reload),
                                                        op);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->rv->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      RV32RegisterType reg_type = RegisterTypeFromInstruction(inst);
      RV32Register *reg = AllocateRegisterWithType(allocator, reload->block, reload,
                                     reg_type, CanUseTemp(allocator, reload));
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
    }
  }
  NoteReloadsInserted(allocator, inst);
}

static void EnsureOperandsAllocated(RV32RegisterAllocator* allocator,
                                    TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op == NULL || op->reg != NULL || op->block == NULL ||
        TargetIsConst(op)) {
      continue;
    }
    if (RV32IsFixedRegister(op)) {
      AllocateRegister(allocator, op);
      continue;
    }
    if (RV32IsVarRegister(op)) {
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

static void AllocateRegisterOnce(RV32RegisterAllocator* allocator,
                                 TargetInstruction* inst);

// Record that this instruction is mid-allocation while it runs, so that a spill
// triggered from inside knows its reads have already been resolved.
static void AllocateRegister(RV32RegisterAllocator* allocator,
                             TargetInstruction* inst) {
  bool pushed = allocator->allocating_depth < RV32_MAX_ALLOCATION_DEPTH;
  if (pushed) {
    allocator->allocating_reloaded[allocator->allocating_depth] = false;
    allocator->allocating[allocator->allocating_depth++] = inst;
  }
  AllocateRegisterOnce(allocator, inst);
  if (pushed) {
    allocator->allocating_depth--;
  }
}

static void AllocateRegisterOnce(RV32RegisterAllocator* allocator,
                                 TargetInstruction* inst) {
   bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
      !allocator->rv->not_leaf;

  RV32Opcode opcode = (RV32Opcode)inst->opcode;
  
  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }
  
  
  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == RV32_OP(mv) || opcode == RV32_OP(fmv_s) ||
      opcode == RV32_OP(fmv_d)) && inst->dest == NULL &&
      inst->operand[1] != NULL) {
    AllocateForRmov(allocator, inst);
    return;
  }

  if (RV32IsVarRegister(inst)) {
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

  RV32Register* reg;

  // Free up any registers we can.
  FreeRegisters(allocator, inst);
  
  switch ((RV32Opcode)inst->opcode) {
    case RV32_OP(const8):
    case RV32_OP(const16):
    case RV32_OP(const32):
    case RV32_OP(const64):
    case RV32_OP(constf):
    case RV32_OP(constd):
    case RV32_OP(symbol):
    case RV32_OP(beq):
    case RV32_OP(bne):
    case RV32_OP(blt):
    case RV32_OP(bltu):
    case RV32_OP(bge):
    case RV32_OP(bgeu):
    case RV32_OP(j):
    case RV32_OP(label):
    case RV32_OP(ret):
    case RV32_OP(save):
    case RV32_OP(restore):
    case RV32_OP(literal):
    case RV32_OP(asm):
    case RV32_OP(loc):
    case RV32_OP(atomic_store):
    case RV32_OP(atomic_fence):
      // These instructions do not have registers allocated to them.  They still
      // have to be marked processed: their reads have had reloads inserted by
      // now, so a later spill of one of their operands must repair the read
      // rather than point it at a slot nothing will load from.
      inst->flags |= TARGET_INST_PROCESSED;
      return;

    case RV32_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;
      
    case RV32_OP(x0):
      reg = &allocator->int_regs[RV32_INT_ZERO_REG];
      break;

    case RV32_OP(fp):
      reg = &allocator->int_regs[RV32_FP_REG];
      break;

    case RV32_OP(sp):
      reg = &allocator->int_regs[RV32_SP_REG];
      break;

    case RV32_OP(t0):
      reg = &allocator->int_regs[RV32_INT_TEMP_START_1];
      break;

    case RV32_OP(t1):
      reg = &allocator->int_regs[RV32_INT_TEMP_START_1 + 1];
      break;

    case RV32_OP(t2):
      reg = &allocator->int_regs[RV32_INT_TEMP_START_1 + 2];
      break;

    case RV32_OP(a0):
    case RV32_OP(a1):
    case RV32_OP(a2):
    case RV32_OP(a3):
    case RV32_OP(a4):
    case RV32_OP(a5):
    case RV32_OP(a6):
    case RV32_OP(a7):
      reg = &allocator->int_regs[(int)inst->opcode - RV32_OP(a0) + RV32_INT_ARG_START];
      break;

    case RV32_OP(ivarreg):
    case RV32_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();
      
    case RV32_OP(fa0):
    case RV32_OP(fa1):
    case RV32_OP(fa2):
    case RV32_OP(fa3):
    case RV32_OP(fa4):
    case RV32_OP(fa5):
    case RV32_OP(fa6):
    case RV32_OP(fa7):
      reg = &allocator
                 ->float_regs[(int)inst->opcode - RV32_OP(fa0) + RV32_FP_ARG_START];
      break;

    case RV32_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? RV32_FIRST_LEAF_INT_REG_VAR
                                          : RV32_FIRST_INT_REG_VAR) +
                                 allocator->rv->struct_return_reg];
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;

    case RV32_OP(resulti):
      reg = &allocator->int_regs[RV32_INT_RETURN_REG];
      break;

    case RV32_OP(resultf):
    case RV32_OP(resultd):
      reg = &allocator->float_regs[RV32_FLOAT_RETURN_REG];
      break;

    case RV32_OP(call):
    case RV32_OP(rcall):
      reg = &allocator->int_regs[RV32_INT_RETURN_REG];
      break;
    case RV32_OP(callf):
    case RV32_OP(rcallf):
      reg = &allocator->float_regs[RV32_FLOAT_RETURN_REG];
      break;
      
    case RV32_OP(feq_s):
    case RV32_OP(flt_s):
    case RV32_OP(fle_s):
    case RV32_OP(feq_d):
    case RV32_OP(flt_d):
    case RV32_OP(fle_d):
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     kRV32RegTypeInt, CanUseTemp(allocator, inst));
      break;

    default: {
      RV32RegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     reg_type, CanUseTemp(allocator, inst));
    }
  }

  AssignRegister(reg, inst);

  // If nobody is using this register free it up immediately.
  // TODO: argument registers are not used explicitly but can't be freed here.
  if (inst->uses == 0 && !reg->base.reserved) {
    FreeRegister(allocator, reg);
  }
}

static void InitializeBasicBlockRegisters(RV32RegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < RV32_NUM_INT_REGS; i++) {
    RV32Register* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < RV32_NUM_FLOAT_REGS; i++) {
     RV32Register* reg = &allocator->float_regs[i];
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
    if (((int)inst->opcode == (int)RV32_OP(spill)) ||
        (inst->flags & TARGET_INST_SPILLED) != 0) {
      continue;
    }
    if (inst->uses == 0 &&
        !HasUnprocessedUserOtherThan(inst, NULL)) {
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
  RV32RegisterAllocator* allocator = data;

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
       inst != NULL && inst != block->end_code;
       inst = TargetNext(inst)) {
    AllocateRegister(allocator, inst);
  }
  if (block->end_code != NULL) {
    AllocateRegister(allocator, block->end_code);
  }
}

static void ProcessBasicBlock(RV32RegisterAllocator* allocator,
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
static void ReserveVariableRegisters(RV32RegisterAllocator* allocator) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                 !allocator->rv->not_leaf;
  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    if (var->is_fp) {
      int first = is_leaf ? RV32_FIRST_LEAF_FP_REG_VAR : RV32_FIRST_FP_REG_VAR;
      int last = is_leaf ? RV32_LAST_LEAF_FP_REG_VAR : RV32_LAST_FP_REG_VAR;
      if (var->varnum > last - first) {
        continue;
      }
      allocator->float_regs[first + var->varnum].base.reserved = true;
    } else {
      int first = is_leaf ? RV32_FIRST_LEAF_INT_REG_VAR : RV32_FIRST_INT_REG_VAR;
      int last = is_leaf ? RV32_LAST_LEAF_INT_REG_VAR : RV32_LAST_INT_REG_VAR;
      if (var->varnum > last - first) {
        continue;
      }
      allocator->int_regs[first + var->varnum].base.reserved = true;
    }
  }
}

static bool HasAtomicInstructions(const RV32RegisterAllocator* allocator) {
  for (size_t i = 0; i < allocator->rv->base.basic_blocks.length; i++) {
    TargetBasicBlock* block =
        allocator->rv->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      if ((int)inst->opcode >= (int)RV32_OP(atomic_load) &&
          (int)inst->opcode <= (int)RV32_OP(atomic_fence)) {
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
// moves (tagged RV32_INST_ARG_MOVE).  Conceptually they are a *parallel* copy:
// every source is the value live just before the call and every destination is
// a distinct argument register.  Emitting them as naive sequential moves is
// wrong whenever a value already sits in an argument register another argument
// needs: an earlier move overwrites a register a later move still has to read
// (e.g. `mv a3,t6; mv a0,a3` loses the original a3), and true cycles (`a1<->a2`)
// cannot be done with plain copies at all.
//
// After register allocation every operand is a physical register, so we can
// resolve each run of consecutive argument moves as a proper parallel copy:
// emit a move only once its destination is no longer needed as a source, and
// break cycles by saving one register into a scratch temporary.
// ---------------------------------------------------------------------------

#define RV32_MAX_ARG_MOVES 32

// A copy whose source is a spill slot rather than a register.  It clobbers its
// destination like any other copy but reads nothing, so it never blocks one.
#define RV32_ARG_MOVE_MEMORY (-1)

typedef struct {
  int dst;                // Destination physical register number.
  int src;                // Source physical register number.
  RV32RegisterType type;    // Register file (int or float).
  TargetOpcode opcode;    // Move opcode (mv / fmv.s / fmv.d).
  TargetInstruction* spill;  // Slot to reload from, for a memory source.
  bool done;
} RV32ArgMove;

// Create a free-standing instruction that just carries a physical register so
// it can be used as a move operand.  It is never linked into the code list, so
// it is parked for teardown.
static TargetInstruction* RV32RegHolder(RV32RegisterAllocator* alloc,
                                      TargetBasicBlock* block, int num,
                                      RV32RegisterType type) {
  TargetInstruction* h = TargetNewInstruction(TARGET_OP(tmp));
  RV32Register* reg = (type == kRV32RegTypeInt) ? &alloc->int_regs[num]
                                            : &alloc->float_regs[num];
  h->reg = &reg->base;
  h->block = block;
  TargetTrackOrphanInstruction(&alloc->rv->base, h);
  return h;
}

static bool RV32RegInMoves(RV32ArgMove* moves, int count, RV32RegisterType type,
                         int num) {
  for (int i = 0; i < count; i++) {
    if (moves[i].type != type) {
      continue;
    }
    if (moves[i].dst == num ||
        (moves[i].src != RV32_ARG_MOVE_MEMORY && moves[i].src == num)) {
      return true;
    }
  }
  return false;
}

// Find a caller-saved temporary register of the given type that is not used by
// any move in the copy (so clobbering it is safe -- the moves immediately
// precede a call, which clobbers temporaries anyway).  Returns -1 if none.
static int RV32FindScratchTemp(RV32ArgMove* moves, int count, RV32RegisterType type) {
  if (type == kRV32RegTypeInt) {
    for (int n = RV32_INT_TEMP_START_1; n <= RV32_INT_TEMP_END_1; n++) {
      if (n == RV32_INT_TEMP_START_1 + 2) continue;  // t2 stages indirect calls.
      if (!RV32RegInMoves(moves, count, type, n)) return n;
    }
    for (int n = RV32_INT_TEMP_START_2; n <= RV32_INT_TEMP_END_2; n++) {
      if (!RV32RegInMoves(moves, count, type, n)) return n;
    }
  } else {
    for (int n = RV32_FP_TEMP_START_1; n <= RV32_FP_TEMP_END_1; n++) {
      if (!RV32RegInMoves(moves, count, type, n)) return n;
    }
    for (int n = RV32_FP_TEMP_START_2; n <= RV32_FP_TEMP_END_2; n++) {
      if (!RV32RegInMoves(moves, count, type, n)) return n;
    }
  }
  return -1;
}

static void RV32EmitMove(RV32RegisterAllocator* alloc, TargetBasicBlock* block,
                       TargetInstruction* pos, const RV32ArgMove* move_desc) {
  if (move_desc->src == RV32_ARG_MOVE_MEMORY) {
    TargetInstruction* reload = TargetNewInstruction1(
        (TargetOpcode)RV32_OP(reload), move_desc->spill);
    RV32Register* regs = (move_desc->type == kRV32RegTypeInt) ? alloc->int_regs
                                                          : alloc->float_regs;
    reload->reg = &regs[move_desc->dst].base;
    reload->flags |= TARGET_INST_PROCESSED;
    reload->uses = 1;
    TargetBasicBlockEmitBefore(&alloc->rv->base, block, reload, pos);
    return;
  }
  TargetInstruction* d =
      RV32RegHolder(alloc, block, move_desc->dst, move_desc->type);
  TargetInstruction* s =
      RV32RegHolder(alloc, block, move_desc->src, move_desc->type);
  TargetInstruction* move = TargetNewInstruction2(move_desc->opcode, d, s);
  TargetBasicBlockEmitBefore(&alloc->rv->base, block, move, pos);
}

// Emit the parallel copy described by `moves` as correctly ordered move
// instructions inserted before `pos`.  All destinations are distinct.
static void RV32ResolveParallelCopy(RV32RegisterAllocator* alloc,
                                  TargetBasicBlock* block,
                                  TargetInstruction* pos, RV32ArgMove* moves,
                                  int count) {
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
            moves[j].src != RV32_ARG_MOVE_MEMORY &&
            moves[j].src == moves[i].dst) {
          blocked = true;
          break;
        }
      }
      if (blocked) {
        continue;
      }
      RV32EmitMove(alloc, block, pos, &moves[i]);
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
    RV32RegisterType type = moves[pick].type;
    int scratch = RV32FindScratchTemp(moves, count, type);
    if (scratch < 0) {
      // No scratch available (extremely unlikely for argument setup).  Emit the
      // remaining moves in order as a best effort rather than risk an overflow.
      for (int i = 0; i < count; i++) {
        if (moves[i].done) {
          continue;
        }
        RV32EmitMove(alloc, block, pos, &moves[i]);
        moves[i].done = true;
      }
      break;
    }
    RV32ArgMove save = {.dst = scratch,
                      .src = moves[pick].dst,
                      .type = type,
                      .opcode = moves[pick].opcode};
    RV32EmitMove(alloc, block, pos, &save);
    for (int j = 0; j < count; j++) {
      if (!moves[j].done && moves[j].type == type &&
          moves[j].src != RV32_ARG_MOVE_MEMORY &&
          moves[j].src == moves[pick].dst) {
        moves[j].src = scratch;
      }
    }
  }
}

// Rewrite each run of consecutive RV32_INST_ARG_MOVE moves in `block` as a
// correctly ordered parallel copy.
static void RV32ResolveArgumentMovesInBlock(RV32RegisterAllocator* alloc,
                                          TargetBasicBlock* block) {
  TargetInstruction* inst = TargetBasicBlockBegin(block);
  TargetInstruction* end = TargetBasicBlockEnd(block);
  while (inst != end && inst != NULL) {
    if ((inst->flags & RV32_INST_ARG_MOVE) == 0) {
      inst = TargetNext(inst);
      continue;
    }

    // Gather the maximal run of consecutive argument moves.  A reload for a
    // spilled argument is part of the copy too: it was given the argument's
    // register directly, so it clobbers that register and has to be ordered
    // with the moves rather than splitting the run in two.
    TargetInstruction* run[RV32_MAX_ARG_MOVES];
    int run_count = 0;
    int last_move;
    bool well_formed = true;
    TargetInstruction* scan = inst;
    while (scan != end && scan != NULL &&
           ((scan->flags & RV32_INST_ARG_MOVE) != 0 ||
            (int)scan->opcode == (int)RV32_OP(reload))) {
      if (run_count >= RV32_MAX_ARG_MOVES) {
        well_formed = false;
        break;
      }
      if ((int)scan->opcode == (int)RV32_OP(reload)) {
        if (scan->reg == NULL || scan->operand[0] == NULL ||
            ((int)scan->operand[0]->opcode != (int)RV32_OP(spill))) {
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
      if ((int)run[r]->opcode != (int)RV32_OP(reload)) {
        continue;
      }
      int readers = 0;
      bool as_destination = false;
      for (int j = r + 1; j < run_count; j++) {
        if ((int)run[j]->opcode == (int)RV32_OP(reload)) {
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
      if ((int)run[i]->opcode != (int)RV32_OP(reload)) {
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
    TargetInstruction* memory_source[RV32_MAX_ARG_MOVES];
    bool folded[RV32_MAX_ARG_MOVES];
    for (int i = 0; i < run_count; i++) {
      memory_source[i] = NULL;
      folded[i] = false;
    }
    for (int r = 0; r < run_count; r++) {
      if ((int)run[r]->opcode != (int)RV32_OP(reload)) {
        continue;
      }
      for (int j = r + 1; j < run_count; j++) {
        if ((int)run[j]->opcode == (int)RV32_OP(reload) ||
            run[j]->operand[1] != run[r]) {
          continue;
        }
        memory_source[j] = run[r]->operand[0];
        folded[r] = true;
        break;
      }
    }

    // Build the parallel-copy move set.
    RV32ArgMove moves[RV32_MAX_ARG_MOVES];
    int count = 0;
    bool duplicate_dst = false;
    if (well_formed) {
      for (int i = 0; i < run_count; i++) {
        RV32RegisterType type;
        int dst;
        if (folded[i]) {
          continue;
        }
        if ((int)run[i]->opcode == (int)RV32_OP(reload)) {
          RV32Register* reg = (RV32Register*)run[i]->reg;
          type = reg->type;
          dst = reg->base.num;
          moves[count].src = RV32_ARG_MOVE_MEMORY;
          moves[count].spill = run[i]->operand[0];
        } else if (memory_source[i] != NULL) {
          RV32Register* dreg = (RV32Register*)run[i]->operand[0]->reg;
          type = dreg->type;
          dst = dreg->base.num;
          moves[count].src = RV32_ARG_MOVE_MEMORY;
          moves[count].spill = memory_source[i];
        } else {
          RV32Register* dreg = (RV32Register*)run[i]->operand[0]->reg;
          RV32Register* sreg = (RV32Register*)run[i]->operand[1]->reg;
          type = (dreg->type == kRV32RegTypeFloat || sreg->type == kRV32RegTypeFloat)
                     ? kRV32RegTypeFloat
                     : kRV32RegTypeInt;
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
        moves[count].opcode = (TargetOpcode)RV32_OP(mv);
        if ((int)run[i]->opcode != (int)RV32_OP(reload)) {
          moves[count].opcode = run[i]->opcode;
        }
        moves[count].done = false;
        count++;
      }
    }

    // Only rewrite when it is safe to do so; otherwise leave the run untouched.
    if (well_formed && !duplicate_dst) {
      RV32ResolveParallelCopy(alloc, block, run[0], moves, count);
      // Neutralise the originals rather than deleting them: the register
      // allocator has already consumed the use counts on their operands, so
      // deleting would underflow them.  Pointing both operands at the same
      // register makes the emitter skip them (it never emits `mv rx, rx`).
      for (int i = 0; i < run_count; i++) {
        if ((int)run[i]->opcode == (int)RV32_OP(reload)) {
          TargetInstruction* holder = RV32RegHolder(
              alloc, block, ((RV32Register*)run[i]->reg)->base.num,
              ((RV32Register*)run[i]->reg)->type);
          run[i]->opcode = (TargetOpcode)RV32_OP(mv);
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

static void RV32ResolveArgumentMoves(RV32RegisterAllocator* alloc) {
  Vector* blocks = &alloc->rv->base.basic_blocks;
  for (size_t i = 0; i < blocks->length; i++) {
    TargetBasicBlock* block = blocks->value.p[i];
    if (block == NULL || TargetBasicBlockIsEmpty(block)) {
      continue;
    }
    RV32ResolveArgumentMovesInBlock(alloc, block);
  }
}

static TargetInstruction* CreateCallResultCopy(TargetInstruction* call) {
  RV32Opcode call_opcode = (RV32Opcode)call->opcode;
  TargetOpcode move_opcode =
      call_opcode == RV32_OP(callf) || call_opcode == RV32_OP(rcallf)
          ? (TargetOpcode)RV32_OP(fmv_d)
          : (TargetOpcode)RV32_OP(mv);
  return TargetNewInstruction1(move_opcode, call);
}

void RV32AllocateRegisters(RV32RegisterAllocator* allocator) {
  // t1 and t2 are required as untracked scratch registers by the atomic
  // emitter, while t2 also stages indirect call targets.  Functions without
  // those hazards can use them for ordinary short-lived values.
  bool has_atomics = HasAtomicInstructions(allocator);
  if (!has_atomics) {
    allocator->int_regs[RV32_INT_TEMP_START_1 + 1].base.reserved = false;
  }
  if (!has_atomics && allocator->rv->base.num_calls == 0) {
    allocator->int_regs[RV32_INT_TEMP_START_1 + 2].base.reserved = false;
  }

  if (allocator->rv->struct_return_reg >= 0) {
    bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                   !allocator->rv->not_leaf;
    int reg_num = (is_leaf ? RV32_FIRST_LEAF_INT_REG_VAR
                           : RV32_FIRST_INT_REG_VAR) +
                  allocator->rv->struct_return_reg;
    allocator->int_regs[reg_num].base.reserved = true;
  }

  TargetMarkCallPreservedInstructions(&allocator->rv->base,
                                      &allocator->preserved_instructions);
  if (TargetMaterializePreservedCallResults(
          &allocator->rv->base, &allocator->preserved_instructions,
          CreateCallResultCopy)) {
    TargetBuildBasicBlockInputsAndOutputs(&allocator->rv->base);
    BitSetClear(&allocator->preserved_instructions);
    TargetMarkCallPreservedInstructions(&allocator->rv->base,
                                        &allocator->preserved_instructions);
  }

  ReserveVariableRegisters(allocator);

  // Process all basic blocks in the RV32 generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->rv->base.entry_block);

  // Fix up argument-register moves that the per-instruction allocation may have
  // left as a clobbering sequence (see RV32ResolveArgumentMoves).
  RV32ResolveArgumentMoves(allocator);
}

const char* RV32RegisterName(RV32Register* reg, char* buf, size_t len) {
  return RV32RegisterNameFromNum(reg->base.num, reg->type, buf, len);
}

const char* RV32RegisterNameFromNum(int num, RV32RegisterType type, char* buf,
                                  size_t len) {
  // See if the register is in one of the named ranges.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type &&
        register_ranges[i].start != RV32_RET_REG) {
      if (num >= register_ranges[i].start && num <= register_ranges[i].end) {
        snprintf(buf, len, "%s%d", register_ranges[i].prefix,
                 num - register_ranges[i].start + register_ranges[i].base);
        return buf;
      }
    }
  }
  switch (type) {
    case kRV32RegTypeInt:
      if (num == RV32_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (num == RV32_FP_REG) {
        snprintf(buf, len, "s0");
        break;
      }
      if (num == RV32_RET_REG) {
        snprintf(buf, len, "ra");
        break;
      }

      snprintf(buf, len, "x%d", num);
      break;

    case kRV32RegTypeFloat:
      snprintf(buf, len, "f%d", num);
      break;
  }
  return buf;
}

#undef NUM_REG_RANGES
