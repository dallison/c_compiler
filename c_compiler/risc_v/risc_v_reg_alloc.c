//
//  risc_v_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include "risc_v_codegen.h"
#include "risc_v_machine.h"
#include "target_basic_block.h"
#include "compiler.h"


static void AllocateRegister(RVRegisterAllocator* allocator,
                             TargetInstruction* inst);

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

static void InitializeRegister(RVRegister* reg, int num, RVRegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void RVRegisterAllocatorInit(RVRegisterAllocator* allocator, RVGenerator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < RV_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kRVRegTypeInt);
  }

  for (int i = 0; i < RV_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kRVRegTypeFloat);
  }

  // Reserve some registers.
  allocator->int_regs[RV_INT_ZERO_REG].base.reserved = true;
  allocator->int_regs[RV_FP_REG].base.reserved = true;
  allocator->int_regs[RV_SP_REG].base.reserved = true;
  allocator->int_regs[RV_SPILL_ADDR].base.reserved = true;
  allocator->int_regs[RV_INT_TEMP_START_1].base.reserved = true;
  allocator->int_regs[RV_INT_TEMP_START_1 + 1].base.reserved = true;
  allocator->int_regs[RV_INT_TEMP_START_1 + 2].base.reserved = true;
  allocator->int_regs[RV_INT_TEMP_START_2].base.reserved = true;
  allocator->int_regs[RV_INT_TEMP_START_2 + 1].base.reserved = true;
  allocator->int_regs[RV_INT_TEMP_START_2 + 2].base.reserved = true;
  allocator->int_regs[RV_INT_TEMP_START_2 + 3].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
  MapInitForPointerKeys(&allocator->reassignable_spills);
}

RVRegisterAllocator* NewRVRegisterAllocator(RVGenerator* pcode) {
  RVRegisterAllocator* reg_alloc = malloc(sizeof(RVRegisterAllocator));
  RVRegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void RVRegisterAllocatorDestruct(RVRegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
  MapDestruct(&allocator->reassignable_spills);
}

void RVRegisterAllocatorDelete(RVRegisterAllocator* alloc) {
  RVRegisterAllocatorDestruct(alloc);
  free(alloc);
}

// RISC-V's ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  RVRegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  const char* prefix;   // Register name prefix.
  int base;
  bool temp;
} register_ranges[] = {
    {kRVRegTypeInt, RV_INT_TEMP_START_1, RV_INT_TEMP_END_1, "t", 0, true},
    {kRVRegTypeInt, RV_INT_TEMP_START_2, RV_INT_TEMP_END_2, "t", 3, true},
    {kRVRegTypeInt, RV_INT_ARG_START, RV_INT_ARG_END, "a", 0, true},
    {kRVRegTypeInt, RV_RET_REG, RV_RET_REG, "ra", 0, true},
    {kRVRegTypeInt, RV_INT_SAVED_START_1, RV_INT_SAVED_END_1, "s", 0, false},
    {kRVRegTypeInt, RV_INT_SAVED_START_2, RV_INT_SAVED_END_2, "s", 2, false},
    {kRVRegTypeFloat, RV_FP_TEMP_START_1, RV_FP_TEMP_END_1, "ft", 0, true},
    {kRVRegTypeFloat, RV_FP_TEMP_START_2, RV_FP_TEMP_END_2, "ft", 2, true},
    {kRVRegTypeFloat, RV_FP_ARG_START, RV_FP_ARG_END, "fa", 0, true},
    {kRVRegTypeFloat, RV_FP_SAVED_START_1, RV_FP_SAVED_END_1, "fs", 0, false},
    {kRVRegTypeFloat, RV_FP_SAVED_START_2, RV_FP_SAVED_END_2, "fs", 2, false},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(RVRegisterAllocator* allocator) {
  char buf[32];
  for (RVRegisterType type = kRVRegTypeInt; type <= kRVRegTypeFloat; type++) {
    RVRegister* regs =
        type == kRVRegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(x%d): free\n", RVRegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(x%d): owner: @%d %s\n", RVRegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num, owner->id,
                   RVOpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(RVRegister* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static RVRegister* FindFreeRegister(RVRegisterAllocator* allocator,
                                    RVRegisterType type, bool can_use_temp) {
  RVRegister* regs =
      type == kRVRegTypeInt ? allocator->int_regs : allocator->float_regs;
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
          if (regs[j].base.num == RV_RET_REG) {
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

static void FreeRegister(RVRegisterAllocator* allocator, RVRegister* reg) {
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
static void FreeRegisters(RVRegisterAllocator* allocator,
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
      if (RVIsFixedRegister(op)) {
        continue;
      }
      if (RVIsVarRegister(op)) {
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
      if (((int)op->opcode == (int)RV_OP(reload)) || ((int)op->opcode == (int)RV_OP(spill))) {
        continue;
      }
#endif
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL) {
        if (op->uses > 0) {
          op->uses--;
        }
        if (!HasUnprocessedUserOtherThan(op, inst) && reg->owner == op) {
          FreeRegister(allocator, (RVRegister*)reg);
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
      FreeRegister(allocator, (RVRegister*)inst->dest->reg);
    }
  }
#endif
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(RVRegister* reg) {
  int num = reg->base.num;
  if ((num >= RV_INT_SAVED_START_1 && num <= RV_INT_SAVED_END_1) ||
      (num >= RV_INT_SAVED_START_2 && num <= RV_INT_SAVED_END_2) ||
      (num >= RV_FP_SAVED_START_1 && num <= RV_FP_SAVED_END_1) ||
      (num >= RV_FP_SAVED_START_2 && num <= RV_FP_SAVED_END_2)) {
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
// RISC-V spill model stores an instruction once at its declaration point, so
// spilling such a tmp captures an undefined/stale value instead of the later
// branch definition.  Keep externally-defined values in registers until the
// allocator has a store-back model for every definition.
static bool IsReassignableDefinition(TargetInstruction* inst,
                                     TargetInstruction* target) {
  if (inst->dest == target) {
    return true;
  }
  RVOpcode opcode = (RVOpcode)inst->opcode;
  return (opcode == RV_OP(mv) || opcode == RV_OP(fmv_s) ||
          opcode == RV_OP(fmv_d)) &&
         inst->dest == NULL && inst->operand[0] == target &&
         inst->operand[1] != NULL;
}

static bool InstructionHasExternalDefs(RVRegisterAllocator* allocator,
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
static TargetInstruction* FindSpillVictim(RVRegisterAllocator* allocator,
                                   RVRegisterType type, bool can_use_temp) {
  RVRegister* regs =
      type == kRVRegTypeInt ? allocator->int_regs : allocator->float_regs;
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
              ((int)owner->opcode == (int)RV_OP(spill))) {
            // Already in memory, so it is not holding this register any more.
            regs[j].base.owner = NULL;
            continue;
          }
          if (RVIsVarRegister(owner)) {
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

static bool NotProcessed(TargetInstruction* inst, void* data) {
  return (inst->flags & TARGET_INST_PROCESSED) == 0;
}

static void InsertReassignableStoreBacks(
    RVRegisterAllocator* allocator, TargetInstruction* target,
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
            (TargetOpcode)RV_OP(spill), target, spill->operand[1]);
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

static void SyncReassignableSpill(RVRegisterAllocator* allocator,
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
      (TargetOpcode)RV_OP(spill), target, spill->operand[1]);
  store->reg = definition->reg;
  store->flags |= TARGET_INST_PROCESSED;
  TargetBasicBlockEmitAfter(&allocator->rv->base, definition->block, store,
                            definition);
}

static RVRegister* SpillInstruction(RVRegisterAllocator* allocator, TargetInstruction* inst) {
  RVRegister* reg = (RVRegister*)inst->reg;    // Current register.
  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)RV_OP(spill), NULL,
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
      if ((RVOpcode)current->opcode == RV_OP(save)) {
        save = current;
        break;
      }
    }
  }
  if (RVIsVarRegister(inst)) {
    // A varreg pseudo has no executable definition of its own. Its users are
    // not guaranteed to be in instruction order, and the first user need not
    // be an assignment, so keep this spill only as a slot handle and store
    // back after every real definition.
    TargetTrackOrphanInstruction(&allocator->rv->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    InsertReassignableStoreBacks(allocator, inst, spill, NULL);
  } else if (InstructionHasExternalDefs(allocator, inst)) {
    // The declaration of a merge tmp has no value to store.  Keep the spill
    // instruction only as a handle for its slot and write the slot after each
    // real definition instead.
    TargetTrackOrphanInstruction(&allocator->rv->base, spill);
    MapKeyValue kv = {.key.p = inst, .value.p = spill};
    MapInsert(&allocator->reassignable_spills, kv);
    InsertReassignableStoreBacks(allocator, inst, spill, NULL);
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
  TargetRetargetInstructionIf(inst, spill, NotProcessed, NULL);
  spill->uses = inst->uses;
  spill->operand[0] = inst;
  spill->reg = inst->reg;
  reg->base.owner = NULL;
  inst->flags |= TARGET_INST_SPILLED;
  return reg;
}

static void EvictPhysicalRegister(RVRegisterAllocator* allocator,
                                  RVRegisterType type, int num,
                                  TargetInstruction* keep) {
  RVRegister* regs =
      type == kRVRegTypeInt ? allocator->int_regs : allocator->float_regs;
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

static RVRegister* AllocateRegisterWithType(RVRegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            RVRegisterType type,
                                            bool can_use_temp) {
  RVRegister* reg = FindFreeRegister(allocator, type, can_use_temp);

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
    case kRVRegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kRVRegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

static RVRegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(constf):
    case RV_OP(constd):
    case RV_OP(fmv_s):
    case RV_OP(fmv_d):
    case RV_OP(fa0):
    case RV_OP(fa1):
    case RV_OP(fa2):
    case RV_OP(fa3):
    case RV_OP(fa4):
    case RV_OP(fa5):
    case RV_OP(fa6):
    case RV_OP(fa7):
    case RV_OP(fvarreg):
    case RV_OP(fmv_w_x):
    case RV_OP(fneg_d):
    case RV_OP(fneg_s):
    case RV_OP(fcvt_s_l):
    case RV_OP(fcvt_d_l):
      return kRVRegTypeFloat;

    case RV_OP(fcvt_w_d):
    case RV_OP(fcvt_wu_d):
    case RV_OP(fcvt_l_s):
    case RV_OP(fcvt_lu_s):
    case RV_OP(fcvt_l_d):
    case RV_OP(fcvt_lu_d):
    case RV_OP(fmv_x_d):
      return kRVRegTypeInt;

    case RV_OP(tmp):
      // A tmp placeholder is integer unless tagged as holding a float value.
      return (inst->flags & RV_INST_FLOAT_TMP) ? kRVRegTypeFloat : kRVRegTypeInt;

    default:
      if ((RVOpcode)inst->opcode >= RV_OP(flw) &&
          (RVOpcode)inst->opcode <= RV_OP(fmv_d_x)) {
        return kRVRegTypeFloat;
      }
      return kRVRegTypeInt;
  }
}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(RVRegisterAllocator* allocator, TargetInstruction* inst) {
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void AllocateVariableRegister(RVRegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                 !allocator->rv->not_leaf;

  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    if (var->inst != inst) {
      continue;
    }
    RVRegister* reg;
    if (var->is_fp) {
      int first = is_leaf ? RV_FIRST_LEAF_FP_REG_VAR : RV_FIRST_FP_REG_VAR;
      int last = is_leaf ? RV_LAST_LEAF_FP_REG_VAR : RV_LAST_FP_REG_VAR;
      if (var->varnum > last - first) {
        goto dynamic_alloc;
      }
      reg = &allocator->float_regs[first + var->varnum];
    } else {
      int first = is_leaf ? RV_FIRST_LEAF_INT_REG_VAR : RV_FIRST_INT_REG_VAR;
      int last = is_leaf ? RV_LAST_LEAF_INT_REG_VAR : RV_LAST_INT_REG_VAR;
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
    RVRegisterType reg_type = RegisterTypeFromInstruction(inst);
    RVRegister* reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                             reg_type, CanUseTemp(allocator, inst));
    AssignRegister(reg, inst);
    if (IsSavedReg(reg)) {
      if (reg_type == kRVRegTypeFloat) {
        BitSetInsert(&allocator->used_float_regs, reg->base.num);
      } else {
        BitSetInsert(&allocator->used_int_regs, reg->base.num);
      }
    }
  }
}

static COMPILER_UNUSED void AllocateForRmov(RVRegisterAllocator* allocator,
                            TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)RV_OP(mv)) || ((int)inst->opcode == (int)RV_OP(fmv_s)) ||
         ((int)inst->opcode == (int)RV_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  if (RVIsVarRegister(dest) && dest->reg == NULL) {
    // Delayed allocation of variable register.
    AllocateVariableRegister(allocator, dest);
  }
  RVRegister* reg = (RVRegister*)dest->reg;
  assert(reg != NULL);
  
  if (((int)src->opcode == (int)RV_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)RV_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    // It is no longer a register-to-register move, so it is no longer part of
    // the parallel copy that places the call's arguments.  Leaving the tag on
    // makes it look like a malformed member of that copy, and the resolver
    // gives up on the whole run -- emitting the remaining moves in their
    // original order, where an earlier one can overwrite an argument register
    // a later one still has to read.
    inst->flags &= ~RV_INST_ARG_MOVE;
    TrapReload(inst);
  } else {
    if (RVIsVarRegister(src) && src->reg == NULL) {
      // Delayed allocation of variable register.
      AllocateVariableRegister(allocator, src);
    }
    inst->operand[0]->uses++;
    FreeRegisters(allocator, inst);
  }
  
  inst->reg = &reg->base;
  inst->flags |= TARGET_INST_PROCESSED;
  SyncReassignableSpill(allocator, inst, dest);
}

static void ReloadSpills(RVRegisterAllocator* allocator,
                         TargetInstruction* inst);
static void EnsureOperandsAllocated(RVRegisterAllocator* allocator,
                                    TargetInstruction* inst);

static bool AllocateUsingDest(RVRegisterAllocator* allocator,
                              TargetInstruction* inst) {
  if (inst->dest == NULL) {
    return false;
  }
  if (inst->dest->reg == NULL) {
    if (RVIsVarRegister(inst->dest)) {
      AllocateVariableRegister(allocator, inst->dest);
    } else {
      AllocateRegister(allocator, inst->dest);
    }
  }
  assert(inst->dest->reg != NULL);
  RVRegisterType fixed_dest_type = kRVRegTypeInt;
  bool fixed_dest = true;
  switch ((RVOpcode)inst->dest->opcode) {
    case RV_OP(a0):
    case RV_OP(a1):
    case RV_OP(a2):
    case RV_OP(a3):
    case RV_OP(a4):
    case RV_OP(a5):
    case RV_OP(a6):
    case RV_OP(a7):
    case RV_OP(resulti):
      fixed_dest_type = kRVRegTypeInt;
      break;
    case RV_OP(fa0):
    case RV_OP(fa1):
    case RV_OP(fa2):
    case RV_OP(fa3):
    case RV_OP(fa4):
    case RV_OP(fa5):
    case RV_OP(fa6):
    case RV_OP(fa7):
    case RV_OP(resultf):
    case RV_OP(resultd):
      fixed_dest_type = kRVRegTypeFloat;
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


static void ReloadSpills(RVRegisterAllocator* allocator,
                         TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ((int)op->opcode == (int)RV_OP(spill))) {
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)RV_OP(reload),
                                                        op);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->rv->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      RVRegisterType reg_type = RegisterTypeFromInstruction(inst);
      RVRegister *reg = AllocateRegisterWithType(allocator, reload->block, reload,
                                     reg_type, CanUseTemp(allocator, reload));
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
    }
  }
}

static void EnsureOperandsAllocated(RVRegisterAllocator* allocator,
                                    TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op == NULL || op->reg != NULL || op->block == NULL ||
        TargetIsConst(op)) {
      continue;
    }
    if (RVIsFixedRegister(op)) {
      AllocateRegister(allocator, op);
      continue;
    }
    if (RVIsVarRegister(op)) {
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

static void AllocateRegister(RVRegisterAllocator* allocator,
                             TargetInstruction* inst) {
   bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
      !allocator->rv->not_leaf;

  RVOpcode opcode = (RVOpcode)inst->opcode;
  
  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }
  
  
  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == RV_OP(mv) || opcode == RV_OP(fmv_s) ||
      opcode == RV_OP(fmv_d)) && inst->dest == NULL &&
      inst->operand[1] != NULL) {
    AllocateForRmov(allocator, inst);
    return;
  }

  if (RVIsVarRegister(inst)) {
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

  RVRegister* reg;

  // Free up any registers we can.
  FreeRegisters(allocator, inst);
  
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(const8):
    case RV_OP(const16):
    case RV_OP(const32):
    case RV_OP(const64):
    case RV_OP(constf):
    case RV_OP(constd):
    case RV_OP(symbol):
    case RV_OP(beq):
    case RV_OP(bne):
    case RV_OP(blt):
    case RV_OP(bltu):
    case RV_OP(bge):
    case RV_OP(bgeu):
    case RV_OP(j):
    case RV_OP(label):
    case RV_OP(ret):
    case RV_OP(save):
    case RV_OP(restore):
    case RV_OP(literal):
    case RV_OP(asm):
    case RV_OP(loc):
    case RV_OP(atomic_store):
    case RV_OP(atomic_fence):
      // These instructions do not have registers allocated to them.
      return;

    case RV_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;
      
    case RV_OP(x0):
      reg = &allocator->int_regs[RV_INT_ZERO_REG];
      break;

    case RV_OP(fp):
      reg = &allocator->int_regs[RV_FP_REG];
      break;

    case RV_OP(sp):
      reg = &allocator->int_regs[RV_SP_REG];
      break;

    case RV_OP(t0):
      reg = &allocator->int_regs[RV_INT_TEMP_START_1];
      break;

    case RV_OP(t1):
      reg = &allocator->int_regs[RV_INT_TEMP_START_1 + 1];
      break;

    case RV_OP(t2):
      reg = &allocator->int_regs[RV_INT_TEMP_START_1 + 2];
      break;

    case RV_OP(a0):
    case RV_OP(a1):
    case RV_OP(a2):
    case RV_OP(a3):
    case RV_OP(a4):
    case RV_OP(a5):
    case RV_OP(a6):
    case RV_OP(a7):
      reg = &allocator->int_regs[(int)inst->opcode - RV_OP(a0) + RV_INT_ARG_START];
      break;

    case RV_OP(ivarreg):
    case RV_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();
      
    case RV_OP(fa0):
    case RV_OP(fa1):
    case RV_OP(fa2):
    case RV_OP(fa3):
    case RV_OP(fa4):
    case RV_OP(fa5):
    case RV_OP(fa6):
    case RV_OP(fa7):
      reg = &allocator
                 ->float_regs[(int)inst->opcode - RV_OP(fa0) + RV_FP_ARG_START];
      break;

    case RV_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? RV_FIRST_LEAF_INT_REG_VAR
                                          : RV_FIRST_INT_REG_VAR) +
                                 allocator->rv->struct_return_reg];
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;

    case RV_OP(resulti):
      reg = &allocator->int_regs[RV_INT_RETURN_REG];
      break;

    case RV_OP(resultf):
    case RV_OP(resultd):
      reg = &allocator->float_regs[RV_FLOAT_RETURN_REG];
      break;

    case RV_OP(call):
    case RV_OP(rcall):
      reg = &allocator->int_regs[RV_INT_RETURN_REG];
      break;
    case RV_OP(callf):
    case RV_OP(rcallf):
      reg = &allocator->float_regs[RV_FLOAT_RETURN_REG];
      break;
      
    case RV_OP(feq_s):
    case RV_OP(flt_s):
    case RV_OP(fle_s):
    case RV_OP(feq_d):
    case RV_OP(flt_d):
    case RV_OP(fle_d):
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     kRVRegTypeInt, CanUseTemp(allocator, inst));
      break;

    default: {
      RVRegisterType reg_type = RegisterTypeFromInstruction(inst);
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

static void InitializeBasicBlockRegisters(RVRegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < RV_NUM_INT_REGS; i++) {
    RVRegister* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < RV_NUM_FLOAT_REGS; i++) {
     RVRegister* reg = &allocator->float_regs[i];
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
    if (((int)inst->opcode == (int)RV_OP(spill)) ||
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
  RVRegisterAllocator* allocator = data;

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

static void ProcessBasicBlock(RVRegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->rv->base, ProcessBlock,
                          kTraversePreOrder, allocator);
}


// Build the preserved_instructions set, instructions that need their
// register to be preserved across calls.  If the block contains a call
// all outputs need to be preserved.
static void BuildPreservedInstructionsSet(TargetBasicBlock* block, void* data) {
  RVRegisterAllocator* allocator = data;
  if (block->contains_call) {
    BitSetUnionInPlace(&allocator->preserved_instructions,
                       &block->output_ids);
  }
}

// Variable registers are assigned a fixed physical register by index
// (FIRST_*_REG_VAR + varnum) and are allocated lazily at their first
// definition.  Those physical registers are drawn from the same range the
// dynamic allocator searches, so without reserving them the dynamic allocator
// can hand a variable's register to an unrelated value (e.g. a pooled
// loop-bound constant) before the variable is allocated, and the variable's
// later assignment then clobbers that still-live value.  Reserve them up front.
static void ReserveVariableRegisters(RVRegisterAllocator* allocator) {
  bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                 !allocator->rv->not_leaf;
  for (size_t i = 0; i < allocator->rv->var_regs.length; i++) {
    RegisterVariable* var = allocator->rv->var_regs.value.p[i];
    if (var->is_fp) {
      int first = is_leaf ? RV_FIRST_LEAF_FP_REG_VAR : RV_FIRST_FP_REG_VAR;
      int last = is_leaf ? RV_LAST_LEAF_FP_REG_VAR : RV_LAST_FP_REG_VAR;
      if (var->varnum > last - first) {
        continue;
      }
      allocator->float_regs[first + var->varnum].base.reserved = true;
    } else {
      int first = is_leaf ? RV_FIRST_LEAF_INT_REG_VAR : RV_FIRST_INT_REG_VAR;
      int last = is_leaf ? RV_LAST_LEAF_INT_REG_VAR : RV_LAST_INT_REG_VAR;
      if (var->varnum > last - first) {
        continue;
      }
      allocator->int_regs[first + var->varnum].base.reserved = true;
    }
  }
}

static bool HasAtomicInstructions(const RVRegisterAllocator* allocator) {
  for (size_t i = 0; i < allocator->rv->base.basic_blocks.length; i++) {
    TargetBasicBlock* block =
        allocator->rv->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = TargetNext(inst)) {
      if ((int)inst->opcode >= (int)RV_OP(atomic_load) &&
          (int)inst->opcode <= (int)RV_OP(atomic_fence)) {
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
// moves (tagged RV_INST_ARG_MOVE).  Conceptually they are a *parallel* copy:
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

#define RV_MAX_ARG_MOVES 32

typedef struct {
  int dst;                // Destination physical register number.
  int src;                // Source physical register number.
  RVRegisterType type;    // Register file (int or float).
  TargetOpcode opcode;    // Move opcode (mv / fmv.s / fmv.d).
  bool done;
} RVArgMove;

// Create a free-standing instruction that just carries a physical register so
// it can be used as a move operand.  It is never linked into the code list, so
// it is parked for teardown.
static TargetInstruction* RVRegHolder(RVRegisterAllocator* alloc,
                                      TargetBasicBlock* block, int num,
                                      RVRegisterType type) {
  TargetInstruction* h = TargetNewInstruction(TARGET_OP(tmp));
  RVRegister* reg = (type == kRVRegTypeInt) ? &alloc->int_regs[num]
                                            : &alloc->float_regs[num];
  h->reg = &reg->base;
  h->block = block;
  TargetTrackOrphanInstruction(&alloc->rv->base, h);
  return h;
}

static bool RVRegInMoves(RVArgMove* moves, int count, RVRegisterType type,
                         int num) {
  for (int i = 0; i < count; i++) {
    if (moves[i].type != type) {
      continue;
    }
    if (moves[i].dst == num || moves[i].src == num) {
      return true;
    }
  }
  return false;
}

// Find a caller-saved temporary register of the given type that is not used by
// any move in the copy (so clobbering it is safe -- the moves immediately
// precede a call, which clobbers temporaries anyway).  Returns -1 if none.
static int RVFindScratchTemp(RVArgMove* moves, int count, RVRegisterType type) {
  if (type == kRVRegTypeInt) {
    for (int n = RV_INT_TEMP_START_1; n <= RV_INT_TEMP_END_1; n++) {
      if (n == RV_INT_TEMP_START_1 + 2) continue;  // t2 stages indirect calls.
      if (!RVRegInMoves(moves, count, type, n)) return n;
    }
    for (int n = RV_INT_TEMP_START_2; n <= RV_INT_TEMP_END_2; n++) {
      if (!RVRegInMoves(moves, count, type, n)) return n;
    }
  } else {
    for (int n = RV_FP_TEMP_START_1; n <= RV_FP_TEMP_END_1; n++) {
      if (!RVRegInMoves(moves, count, type, n)) return n;
    }
    for (int n = RV_FP_TEMP_START_2; n <= RV_FP_TEMP_END_2; n++) {
      if (!RVRegInMoves(moves, count, type, n)) return n;
    }
  }
  return -1;
}

static void RVEmitMove(RVRegisterAllocator* alloc, TargetBasicBlock* block,
                       TargetInstruction* pos, int dst, int src,
                       RVRegisterType type, TargetOpcode opcode) {
  TargetInstruction* d = RVRegHolder(alloc, block, dst, type);
  TargetInstruction* s = RVRegHolder(alloc, block, src, type);
  TargetInstruction* move = TargetNewInstruction2(opcode, d, s);
  TargetBasicBlockEmitBefore(&alloc->rv->base, block, move, pos);
}

// Emit the parallel copy described by `moves` as correctly ordered move
// instructions inserted before `pos`.  All destinations are distinct.
static void RVResolveParallelCopy(RVRegisterAllocator* alloc,
                                  TargetBasicBlock* block,
                                  TargetInstruction* pos, RVArgMove* moves,
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
        if (moves[j].type == moves[i].type && moves[j].src == moves[i].dst) {
          blocked = true;
          break;
        }
      }
      if (blocked) {
        continue;
      }
      RVEmitMove(alloc, block, pos, moves[i].dst, moves[i].src, moves[i].type,
                 moves[i].opcode);
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
    RVRegisterType type = moves[pick].type;
    int scratch = RVFindScratchTemp(moves, count, type);
    if (scratch < 0) {
      // No scratch available (extremely unlikely for argument setup).  Emit the
      // remaining moves in order as a best effort rather than risk an overflow.
      for (int i = 0; i < count; i++) {
        if (moves[i].done) {
          continue;
        }
        RVEmitMove(alloc, block, pos, moves[i].dst, moves[i].src, moves[i].type,
                   moves[i].opcode);
        moves[i].done = true;
      }
      break;
    }
    RVEmitMove(alloc, block, pos, scratch, moves[pick].dst, type,
               moves[pick].opcode);
    for (int j = 0; j < count; j++) {
      if (!moves[j].done && moves[j].type == type &&
          moves[j].src == moves[pick].dst) {
        moves[j].src = scratch;
      }
    }
  }
}

// Rewrite each run of consecutive RV_INST_ARG_MOVE moves in `block` as a
// correctly ordered parallel copy.
static void RVResolveArgumentMovesInBlock(RVRegisterAllocator* alloc,
                                          TargetBasicBlock* block) {
  TargetInstruction* inst = TargetBasicBlockBegin(block);
  TargetInstruction* end = TargetBasicBlockEnd(block);
  while (inst != end && inst != NULL) {
    if ((inst->flags & RV_INST_ARG_MOVE) == 0) {
      inst = TargetNext(inst);
      continue;
    }

    // Gather the maximal run of consecutive argument moves.
    TargetInstruction* run[RV_MAX_ARG_MOVES];
    int run_count = 0;
    bool well_formed = true;
    TargetInstruction* scan = inst;
    while (scan != end && scan != NULL &&
           (scan->flags & RV_INST_ARG_MOVE) != 0) {
      if (run_count >= RV_MAX_ARG_MOVES) {
        well_formed = false;
        break;
      }
      if (scan->operand[0] == NULL || scan->operand[1] == NULL ||
          scan->operand[0]->reg == NULL || scan->operand[1]->reg == NULL ||
          scan->operand[1]->block == NULL) {
        well_formed = false;
      }
      run[run_count++] = scan;
      scan = TargetNext(scan);
    }
    TargetInstruction* after_run = scan;

    // Build the parallel-copy move set.
    RVArgMove moves[RV_MAX_ARG_MOVES];
    int count = 0;
    bool duplicate_dst = false;
    if (well_formed) {
      for (int i = 0; i < run_count; i++) {
        RVRegister* dreg = (RVRegister*)run[i]->operand[0]->reg;
        RVRegister* sreg = (RVRegister*)run[i]->operand[1]->reg;
        RVRegisterType type = (dreg->type == kRVRegTypeFloat ||
                               sreg->type == kRVRegTypeFloat)
                                  ? kRVRegTypeFloat
                                  : kRVRegTypeInt;
        for (int j = 0; j < count; j++) {
          if (moves[j].type == type && moves[j].dst == dreg->base.num) {
            duplicate_dst = true;
          }
        }
        moves[count].dst = dreg->base.num;
        moves[count].src = sreg->base.num;
        moves[count].type = type;
        moves[count].opcode = run[i]->opcode;
        moves[count].done = false;
        count++;
      }
    }

    // Only rewrite when it is safe to do so; otherwise leave the run untouched.
    if (well_formed && !duplicate_dst) {
      RVResolveParallelCopy(alloc, block, run[0], moves, count);
      // Neutralise the original moves rather than deleting them: the register
      // allocator has already consumed the use counts on their operands, so
      // deleting would underflow them.  Pointing both operands at the same
      // register makes the emitter skip them (it never emits `mv rx, rx`).
      for (int i = 0; i < run_count; i++) {
        run[i]->operand[1] = run[i]->operand[0];
      }
    }
    inst = after_run;
  }
}

static void RVResolveArgumentMoves(RVRegisterAllocator* alloc) {
  Vector* blocks = &alloc->rv->base.basic_blocks;
  for (size_t i = 0; i < blocks->length; i++) {
    TargetBasicBlock* block = blocks->value.p[i];
    if (block == NULL || TargetBasicBlockIsEmpty(block)) {
      continue;
    }
    RVResolveArgumentMovesInBlock(alloc, block);
  }
}

void RVAllocateRegisters(RVRegisterAllocator* allocator) {
  // t1 and t2 are required as untracked scratch registers by the atomic
  // emitter, while t2 also stages indirect call targets.  Functions without
  // those hazards can use them for ordinary short-lived values.
  bool has_atomics = HasAtomicInstructions(allocator);
  if (!has_atomics) {
    allocator->int_regs[RV_INT_TEMP_START_1 + 1].base.reserved = false;
  }
  if (!has_atomics && allocator->rv->base.num_calls == 0) {
    allocator->int_regs[RV_INT_TEMP_START_1 + 2].base.reserved = false;
  }

  if (allocator->rv->struct_return_reg >= 0) {
    bool is_leaf = allocator->rv->base.num_calls == 0 && OptLevel1() &&
                   !allocator->rv->not_leaf;
    int reg_num = (is_leaf ? RV_FIRST_LEAF_INT_REG_VAR
                           : RV_FIRST_INT_REG_VAR) +
                  allocator->rv->struct_return_reg;
    allocator->int_regs[reg_num].base.reserved = true;
  }

  TargetTraverseDominatorTree(&allocator->rv->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);

  ReserveVariableRegisters(allocator);

  // Process all basic blocks in the RV generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->rv->base.entry_block);

  // Fix up argument-register moves that the per-instruction allocation may have
  // left as a clobbering sequence (see RVResolveArgumentMoves).
  RVResolveArgumentMoves(allocator);
}

const char* RVRegisterName(RVRegister* reg, char* buf, size_t len) {
  return RVRegisterNameFromNum(reg->base.num, reg->type, buf, len);
}

const char* RVRegisterNameFromNum(int num, RVRegisterType type, char* buf,
                                  size_t len) {
  // See if the register is in one of the named ranges.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type &&
        register_ranges[i].start != RV_RET_REG) {
      if (num >= register_ranges[i].start && num <= register_ranges[i].end) {
        snprintf(buf, len, "%s%d", register_ranges[i].prefix,
                 num - register_ranges[i].start + register_ranges[i].base);
        return buf;
      }
    }
  }
  switch (type) {
    case kRVRegTypeInt:
      if (num == RV_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (num == RV_FP_REG) {
        snprintf(buf, len, "s0");
        break;
      }
      if (num == RV_RET_REG) {
        snprintf(buf, len, "ra");
        break;
      }

      snprintf(buf, len, "x%d", num);
      break;

    case kRVRegTypeFloat:
      snprintf(buf, len, "f%d", num);
      break;
  }
  return buf;
}

#undef NUM_REG_RANGES
