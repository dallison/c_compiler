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

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
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

// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(RVRegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      if (RVIsFixedRegister(op)) {
        continue;
      }
      if (RVIsVarRegister(op)) {
        continue;
      }
#if 0
      if (((int)op->opcode == (int)RV_OP(reload)) || ((int)op->opcode == (int)RV_OP(spill))) {
        continue;
      }
#endif
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL && op->uses > 0) {
        op->uses--;
        assert(op->uses >= 0);
        if (op->uses == 0) {
          if (reg->owner == op) {
            FreeRegister(allocator, (RVRegister*)reg);
          }
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

static TargetInstruction* FindSpillVictim(RVRegisterAllocator* allocator,
                                   RVRegisterType type) {
  RVRegister* regs =
      type == kRVRegTypeInt ? allocator->int_regs : allocator->float_regs;
  int min_cost = INT_MAX;
  TargetInstruction* victim = NULL;
  // Find the instruction with the lowest spill cost.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner != NULL) {
          TargetInstruction* owner = regs[j].base.owner;
#if 0
          if (((int)owner->opcode == (int)RV_OP(spill)) || ((int)owner->opcode == (int)RV_OP(reload))) {
            // Not spill or reload instruction.
            continue;
          }
#endif
          assert((owner->flags & TARGET_INST_SPILLED) == 0);
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
    DumpRegisters(allocator);
    abort();
  }
  return victim;
}

static bool NotProcessed(TargetInstruction* inst, void* data) {
  return (inst->flags & TARGET_INST_PROCESSED) == 0;
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
 
  if (RVIsVarRegister(inst)) {
    // Spilling a varreg.  This instruction is in the entry block but
    // it can't be spilled there.  It needs to be spilled at its first
    // use (the assignment to it).  This is going to be the first user
    // of the instruction.
    assert(inst->users.length > 0);
    TargetInstruction* first_use = inst->users.value.p[0];
    TargetBasicBlockEmitAfter(&allocator->rv->base, first_use->block, spill, first_use);
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

static RVRegister* AllocateRegisterWithType(RVRegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            RVRegisterType type,
                                            bool can_use_temp) {
  RVRegister* reg = FindFreeRegister(allocator, type, can_use_temp);

  if (reg == NULL) {
    TargetInstruction* victim = FindSpillVictim(allocator, type);
    reg = SpillInstruction(allocator, victim);
  }
  assert(reg != NULL);

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
}

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
  if (inst->operand[0] != NULL && inst->operand[0]->reg == NULL &&
      inst->operand[0]->block != NULL) {
    AllocateRegister(allocator, inst->operand[0]);
  }
  inst->reg = inst->dest->reg;
  if (inst->operand[0] != NULL) {
    inst->operand[0]->uses++;
  }
  FreeRegisters(allocator, inst);
  inst->flags |= TARGET_INST_PROCESSED;
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
    if (inst->uses == 0) {
      continue;
    }
    assert(inst->reg != NULL);
    inst->reg->owner = inst;
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
  if (!block->contains_call) {
    return;
  }
  // Preserve all outputs.
  BitSetUnionInPlace(&allocator->preserved_instructions, &block->output_ids);
}

void RVAllocateRegisters(RVRegisterAllocator* allocator) {
  TargetTraverseDominatorTree(&allocator->rv->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);

  // Process all basic blocks in the RV generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->rv->base.entry_block);
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
