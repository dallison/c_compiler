//
//  x86_64_reg_alloc.c
//  c_compiler_library
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "x86_64_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include "map.h"
#include "x86_64_codegen.h"
#include "x86_64_machine.h"
#include "target_basic_block.h"
#include "compiler.h"


static void AllocateRegister(X86_64RegisterAllocator* allocator,
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

static void InitializeRegister(X86_64Register* reg, int num, X86_64RegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void X86_64RegisterAllocatorInit(X86_64RegisterAllocator* allocator, X86_64Generator* rv) {
  allocator->rv = rv;

  for (int i = 0; i < X86_64_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kX86_64RegTypeInt);
  }

  for (int i = 0; i < X86_64_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kX86_64RegTypeFloat);
  }

  // Reserve some registers.
  allocator->int_regs[X86_64_INT_ZERO_REG].base.reserved = true;
  allocator->int_regs[X86_64_FP_REG].base.reserved = true;
  allocator->int_regs[X86_64_SP_REG].base.reserved = true;
  allocator->int_regs[X86_64_SPILL_ADDR].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
}

X86_64RegisterAllocator* NewX86_64RegisterAllocator(X86_64Generator* pcode) {
  X86_64RegisterAllocator* reg_alloc = malloc(sizeof(X86_64RegisterAllocator));
  X86_64RegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void X86_64RegisterAllocatorDestruct(X86_64RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
}

void X86_64RegisterAllocatorDelete(X86_64RegisterAllocator* alloc) {
  X86_64RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// x86-64's ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  X86_64RegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  const char* prefix;   // Register name prefix.
  int base;
  bool temp;
} register_ranges[] = {
    {kX86_64RegTypeInt, X86_64_INT_TEMP_START_1, X86_64_INT_TEMP_END_1, "t", 0, true},
    {kX86_64RegTypeInt, X86_64_INT_TEMP_START_2, X86_64_INT_TEMP_END_2, "t", 3, true},
    {kX86_64RegTypeInt, X86_64_INT_ARG_START, X86_64_INT_ARG_END, "a", 0, true},
    {kX86_64RegTypeInt, X86_64_RET_REG, X86_64_RET_REG, "ra", 0, true},
    {kX86_64RegTypeInt, X86_64_INT_SAVED_START_1, X86_64_INT_SAVED_END_1, "s", 0, false},
    {kX86_64RegTypeInt, X86_64_INT_SAVED_START_2, X86_64_INT_SAVED_END_2, "s", 2, false},
    {kX86_64RegTypeFloat, X86_64_FP_TEMP_START_1, X86_64_FP_TEMP_END_1, "ft", 0, true},
    {kX86_64RegTypeFloat, X86_64_FP_TEMP_START_2, X86_64_FP_TEMP_END_2, "ft", 2, true},
    {kX86_64RegTypeFloat, X86_64_FP_ARG_START, X86_64_FP_ARG_END, "fa", 0, true},
    {kX86_64RegTypeFloat, X86_64_FP_SAVED_START_1, X86_64_FP_SAVED_END_1, "fs", 0, false},
    {kX86_64RegTypeFloat, X86_64_FP_SAVED_START_2, X86_64_FP_SAVED_END_2, "fs", 2, false},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(X86_64RegisterAllocator* allocator) {
  char buf[32];
  for (X86_64RegisterType type = kX86_64RegTypeInt; type <= kX86_64RegTypeFloat; type++) {
    X86_64Register* regs =
        type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(x%d): free\n", X86_64RegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(x%d): owner: @%d %s\n", X86_64RegisterName(&regs[j], buf, sizeof(buf)), regs[j].base.num, owner->id,
                   X86_64OpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(X86_64Register* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static X86_64Register* FindFreeRegister(X86_64RegisterAllocator* allocator,
                                    X86_64RegisterType type, bool can_use_temp) {
  X86_64Register* regs =
      type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
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
          if (regs[j].base.num == X86_64_RET_REG) {
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

static void FreeRegister(X86_64RegisterAllocator* allocator, X86_64Register* reg) {
  reg->base.owner = NULL;
}

// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(X86_64RegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      if (X86_64IsFixedRegister(op)) {
        continue;
      }
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL && op->uses > 0) {
        op->uses--;
        assert(op->uses >= 0);
        if (op->uses == 0) {
          if (reg->owner == op) {
            FreeRegister(allocator, (X86_64Register*)reg);
          }
        }
      }
    }
  }
  
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(X86_64Register* reg) {
  int num = reg->base.num;
  if ((num >= X86_64_INT_SAVED_START_1 && num <= X86_64_INT_SAVED_END_1) ||
      (num >= X86_64_INT_SAVED_START_2 && num <= X86_64_INT_SAVED_END_2) ||
      (num >= X86_64_FP_SAVED_START_1 && num <= X86_64_FP_SAVED_END_1) ||
      (num >= X86_64_FP_SAVED_START_2 && num <= X86_64_FP_SAVED_END_2)) {
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

static TargetInstruction* FindSpillVictim(X86_64RegisterAllocator* allocator,
                                   X86_64RegisterType type) {
  X86_64Register* regs =
      type == kX86_64RegTypeInt ? allocator->int_regs : allocator->float_regs;
  int min_cost = INT_MAX;
  TargetInstruction* victim = NULL;
  // Find the instruction with the lowest spill cost.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner != NULL) {
          TargetInstruction* owner = regs[j].base.owner;
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

static X86_64Register* SpillInstruction(X86_64RegisterAllocator* allocator, TargetInstruction* inst) {
  X86_64Register* reg = (X86_64Register*)inst->reg;    // Current register.
  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)X86_64_OP(spill), NULL,
                                                   TargetGetIntConstant(&allocator->rv->base,
                                                                        NULL,
                                                                        kTargetType32Bit,
                                                                        allocator->current_spilled_region_size));
  allocator->current_spilled_region_size += 8;    // Space for one register.
  if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size = allocator->current_spilled_region_size;
  }
  if (X86_64IsVarRegister(inst)) {
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

static X86_64Register* AllocateRegisterWithType(X86_64RegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            X86_64RegisterType type,
                                            bool can_use_temp) {
  X86_64Register* reg = FindFreeRegister(allocator, type, can_use_temp);

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
    case kX86_64RegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kX86_64RegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

static void ReserveIdivRegisters(X86_64RegisterAllocator* allocator) {
  static const int kIdivRegs[] = {X86_64_RET_REG, X86_64_INT_ARG_START + 2};
  for (size_t i = 0; i < sizeof(kIdivRegs) / sizeof(kIdivRegs[0]); i++) {
    X86_64Register* reg = &allocator->int_regs[kIdivRegs[i]];
    if (reg->base.owner != NULL) {
      SpillInstruction(allocator, reg->base.owner);
    }
  }
}

static X86_64RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(constf):
    case X86_64_OP(constd):
    case X86_64_OP(fmv_s):
    case X86_64_OP(fmv_d):
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
    case X86_64_OP(fvarreg):
    case X86_64_OP(loadss):
    case X86_64_OP(loadsd):
    case X86_64_OP(storess):
    case X86_64_OP(storesd):
    case X86_64_OP(addss):
    case X86_64_OP(addsd):
    case X86_64_OP(subss):
    case X86_64_OP(subsd):
    case X86_64_OP(mulss):
    case X86_64_OP(mulsd):
    case X86_64_OP(divss):
    case X86_64_OP(divsd):
    case X86_64_OP(sqrtss):
    case X86_64_OP(sqrtsd):
    case X86_64_OP(ucomiss):
    case X86_64_OP(ucomisd):
    case X86_64_OP(cvtsi2ss):
    case X86_64_OP(cvtsi2sd):
    case X86_64_OP(cvtss2sd):
    case X86_64_OP(cvtsd2ss):
    case X86_64_OP(movss):
    case X86_64_OP(movsd):
    case X86_64_OP(movd):
    case X86_64_OP(fneg_ss):
    case X86_64_OP(fneg_sd):
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
      return kX86_64RegTypeFloat;

    case X86_64_OP(cvttsd2si):
    case X86_64_OP(cvttss2si):
    case X86_64_OP(movq_xmm):
      return kX86_64RegTypeInt;

    default:
      return kX86_64RegTypeInt;
  }
}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(X86_64RegisterAllocator* allocator, TargetInstruction* inst) {
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void AllocateVariableRegister(X86_64RegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  X86_64RegisterType reg_type = RegisterTypeFromInstruction(inst);
  
  X86_64Register* reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                 reg_type, CanUseTemp(allocator, inst));
  AssignRegister(reg, inst);
}

static COMPILER_UNUSED void AllocateForRmov(X86_64RegisterAllocator* allocator,
                            TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)X86_64_OP(mv)) ||
         ((int)inst->opcode == (int)X86_64_OP(fmv_s)) ||
         ((int)inst->opcode == (int)X86_64_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  if (X86_64IsVarRegister(dest) && dest->reg == NULL) {
    // Delayed allocation of variable register.
    AllocateVariableRegister(allocator, dest);
  }
  X86_64Register* reg = (X86_64Register*)dest->reg;
  assert(reg != NULL);
  
  if (((int)src->opcode == (int)X86_64_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)X86_64_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    TrapReload(inst);
  } else {
    if (X86_64IsVarRegister(src) && src->reg == NULL) {
      // Delayed allocation of variable register.
      AllocateVariableRegister(allocator, src);
    }
    inst->operand[0]->uses++;
    FreeRegisters(allocator, inst);
  }
  
  inst->reg = &reg->base;
  inst->flags |= TARGET_INST_PROCESSED;
}


static void ReloadSpills(X86_64RegisterAllocator* allocator,
                         TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ((int)op->opcode == (int)X86_64_OP(spill))) {
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)X86_64_OP(reload),
                                                        op);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->rv->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      X86_64RegisterType reg_type = RegisterTypeFromInstruction(inst);
      X86_64Register *reg = AllocateRegisterWithType(allocator, reload->block, reload,
                                     reg_type, CanUseTemp(allocator, reload));
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
    }
  }
}

static bool AllocateUsingDest(X86_64RegisterAllocator* allocator,
                              TargetInstruction* inst) {
  if (inst->dest == NULL) {
    return false;
  }
  if (inst->dest->reg == NULL) {
    if (X86_64IsVarRegister(inst->dest)) {
      AllocateVariableRegister(allocator, inst->dest);
    } else {
      AllocateRegister(allocator, inst->dest);
    }
  }
  assert(inst->dest->reg != NULL);
  inst->reg = inst->dest->reg;
  if (inst->operand[0] != NULL) {
    inst->operand[0]->uses++;
  }
  FreeRegisters(allocator, inst);
  inst->flags |= TARGET_INST_PROCESSED;
  return true;
}

static void AllocateRegister(X86_64RegisterAllocator* allocator,
                             TargetInstruction* inst) {
   bool is_leaf = allocator->rv->base.num_calls == 0 &&
      compiler->optimize;

  X86_64Opcode opcode = (X86_64Opcode)inst->opcode;
  
  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }
  
  
  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == X86_64_OP(mv) || opcode == X86_64_OP(fmv_s) ||
      opcode == X86_64_OP(fmv_d)) && inst->dest == NULL &&
      inst->operand[1] != NULL) {
    AllocateForRmov(allocator, inst);
    return;
  }

  if (X86_64IsVarRegister(inst)) {
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

  X86_64Register* reg;

  // Free up any registers we can.
  FreeRegisters(allocator, inst);

  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(const8):
    case X86_64_OP(const16):
    case X86_64_OP(const32):
    case X86_64_OP(const64):
    case X86_64_OP(constf):
    case X86_64_OP(constd):
    case X86_64_OP(symbol):
    case X86_64_OP(cmp):
    case X86_64_OP(test):
    case X86_64_OP(je):
    case X86_64_OP(jne):
    case X86_64_OP(jl):
    case X86_64_OP(jb):
    case X86_64_OP(jge):
    case X86_64_OP(jae):
    case X86_64_OP(jmp):
    case X86_64_OP(label):
    case X86_64_OP(ret):
    case X86_64_OP(save):
    case X86_64_OP(restore):
    case X86_64_OP(literal):
    case X86_64_OP(asm):
    case X86_64_OP(loc):
      // These instructions do not have registers allocated to them.
      return;

    case X86_64_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;
      
    case X86_64_OP(x0):
      reg = &allocator->int_regs[X86_64_INT_ZERO_REG];
      break;

    case X86_64_OP(fp):
      reg = &allocator->int_regs[X86_64_FP_REG];
      break;

    case X86_64_OP(sp):
      reg = &allocator->int_regs[X86_64_SP_REG];
      break;

    case X86_64_OP(t0):
      reg = &allocator->int_regs[X86_64_INT_TEMP_START_1];
      break;

    case X86_64_OP(a0):
    case X86_64_OP(a1):
    case X86_64_OP(a2):
    case X86_64_OP(a3):
    case X86_64_OP(a4):
    case X86_64_OP(a5):
    case X86_64_OP(a6):
    case X86_64_OP(a7):
      reg = &allocator->int_regs[(int)inst->opcode - X86_64_OP(a0) + X86_64_INT_ARG_START];
      break;

    case X86_64_OP(ivarreg):
    case X86_64_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();
      
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
      reg = &allocator
                 ->float_regs[(int)inst->opcode - X86_64_OP(fa0) + X86_64_FP_ARG_START];
      break;

    case X86_64_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? X86_64_FIRST_LEAF_INT_REG_VAR
                                          : X86_64_FIRST_INT_REG_VAR) +
                                 allocator->rv->struct_return_reg];
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;

    case X86_64_OP(resulti):
      reg = &allocator->int_regs[X86_64_INT_RETURN_REG];
      break;

    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
      reg = &allocator->float_regs[X86_64_FLOAT_RETURN_REG];
      break;

    case X86_64_OP(call):
    case X86_64_OP(rcall):
      reg = &allocator->int_regs[X86_64_INT_RETURN_REG];
      break;
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
      reg = &allocator->float_regs[X86_64_FLOAT_RETURN_REG];
      break;
      
    case X86_64_OP(ucomiss):
    case X86_64_OP(ucomisd):
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     kX86_64RegTypeInt, CanUseTemp(allocator, inst));
      break;

    case X86_64_OP(idiv):
    case X86_64_OP(div):
    case X86_64_OP(mod):
      ReserveIdivRegisters(allocator);
      reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                     kX86_64RegTypeInt, CanUseTemp(allocator, inst));
      break;

    default: {
      X86_64RegisterType reg_type = RegisterTypeFromInstruction(inst);
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

static void InitializeBasicBlockRegisters(X86_64RegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < X86_64_NUM_INT_REGS; i++) {
    X86_64Register* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < X86_64_NUM_FLOAT_REGS; i++) {
     X86_64Register* reg = &allocator->float_regs[i];
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
    if (((int)inst->opcode == (int)X86_64_OP(spill)) ||
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
  X86_64RegisterAllocator* allocator = data;

  // For a basic block, the inputs specify what instructions are alive
  // on entry.  An alive instruction has a register allocated to it.  All
  // other registers should be free at this point.
  InitializeBasicBlockRegisters(allocator, block);
  
  for (TargetInstruction* inst = block->code;
       inst != NULL && inst != block->end_code;
       inst = TargetNext(inst)) {
    AllocateRegister(allocator, inst);
  }
  if (block->end_code != NULL) {
    AllocateRegister(allocator, block->end_code);
  }
}

static void ProcessBasicBlock(X86_64RegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->rv->base, ProcessBlock,
                          kTraversePreOrder, allocator);
}


// Build the preserved_instructions set, instructions that need their
// register to be preserved across calls.  If the block contains a call
// all outputs need to be preserved.
static void BuildPreservedInstructionsSet(TargetBasicBlock* block, void* data) {
  X86_64RegisterAllocator* allocator = data;
  if (!block->contains_call) {
    return;
  }
  // Preserve all outputs.
  BitSetUnionInPlace(&allocator->preserved_instructions, &block->output_ids);
}

void X86_64AllocateRegisters(X86_64RegisterAllocator* allocator) {
  TargetTraverseDominatorTree(&allocator->rv->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);

  // Process all basic blocks in the RV generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->rv->base.entry_block);
}

const char* X86_64RegisterName(X86_64Register* reg, char* buf, size_t len) {
  return X86_64RegisterNameFromNum(reg->base.num, reg->type, buf, len);
}

const char* X86_64RegisterNameFromNum(int num, X86_64RegisterType type, char* buf,
                                  size_t len) {
  static const char* kIntRegNames[X86_64_NUM_INT_REGS] = {
      "rax",  "rax",  "rsp",  "r11",  "r10",  "r10",  "r11",  "r12",
      "rbp",  "rbx",  "rdi",  "rsi",  "rdx",  "rcx",  "r8",   "r9",
      "r10",  "r11",  "r12",  "r13",  "r14",  "r15",  "r12",  "r13",
      "r14",  "r15",  "r10",  "r11",  "r8",   "r9",   "r10",  "r11",
  };
  static const char* kFloatRegNames[X86_64_NUM_FLOAT_REGS] = {
      "xmm0",  "xmm1",  "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
      "xmm8",  "xmm9",  "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
      "xmm0",  "xmm1",  "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
      "xmm8",  "xmm9",  "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
  };

  if (type == kX86_64RegTypeInt) {
    if (num >= 0 && num < X86_64_NUM_INT_REGS) {
      snprintf(buf, len, "%s", kIntRegNames[num]);
      return buf;
    }
    snprintf(buf, len, "r%d", num);
    return buf;
  }

  if (num >= 0 && num < X86_64_NUM_FLOAT_REGS) {
    snprintf(buf, len, "%s", kFloatRegNames[num]);
    return buf;
  }
  snprintf(buf, len, "xmm%d", num);
  return buf;
}

#undef NUM_REG_RANGES
