//
//  arm_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "arm_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include "arm_codegen.h"
#include "arm_machine.h"
#include "target_basic_block.h"
#include "compiler.h"

static void AllocateRegister(ARMRegisterAllocator* allocator,
                             TargetInstruction* inst);

static const char* ARMRegisterNameFromNum1(int num, ARMRegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                           size_t len);

static const char* ARMRegisterNameNoSize(ARMRegister* reg, int size, char* buf, size_t len);

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

static void InitializeRegister(ARMRegister* reg, int num, ARMRegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void ARMRegisterAllocatorInit(ARMRegisterAllocator* allocator,
                             struct ARMGenerator* g) {
  allocator->g = g;

  for (int i = 0; i < ARM_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kARMRegTypeInt);
  }

  for (int i = 0; i < ARM_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kARMRegTypeFloat);
  }

  // Reserve some registers.
  allocator->int_regs[ARM_FP_REG].base.reserved = true;
  allocator->int_regs[ARM_LR_REG].base.reserved = true;
  allocator->int_regs[ARM_SPILL_ADDR].base.reserved = true;

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
}

ARMRegisterAllocator* NewARMRegisterAllocator(struct ARMGenerator* g) {
  ARMRegisterAllocator* reg_alloc = malloc(sizeof(ARMRegisterAllocator));
  ARMRegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void ARMRegisterAllocatorDestruct(ARMRegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
  BitSetDestruct(&allocator->preserved_instructions);
}

void ARMRegisterAllocatorDelete(ARMRegisterAllocator* alloc) {
  ARMRegisterAllocatorDestruct(alloc);
  free(alloc);
}

// Size for registers.
#define kSize32Bit 1
#define kSize64Bit 2

// The arm ABI divides registers into various ranges, some of which are
// temporary and some preserved across calls.  We use this array to
// search for registers.
static struct {
  ARMRegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  bool temp;
} register_ranges[] = {
  {kARMRegTypeInt, ARM_INT_ARG_START, ARM_INT_ARG_END, true},
  {kARMRegTypeInt, ARM_INT_TEMP_START, ARM_INT_TEMP_END, true},
    {kARMRegTypeInt, ARM_INT_ARG_START, ARM_INT_ARG_END, true},
    {kARMRegTypeInt, ARM_INT_SAVED_START, ARM_INT_SAVED_END, false},
    {kARMRegTypeFloat, ARM_FP_TEMP_START, ARM_FP_TEMP_END, true},
    {kARMRegTypeFloat, ARM_FP_ARG_START, ARM_FP_ARG_END, true},
    {kARMRegTypeFloat, ARM_FP_SAVED_START, ARM_FP_SAVED_END,false},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static void DumpRegisters(ARMRegisterAllocator* allocator) {
  char buf[32];
  for (ARMRegisterType type = kARMRegTypeInt; type <= kARMRegTypeFloat; type++) {
    ARMRegister* regs =
        type == kARMRegTypeInt ? allocator->int_regs : allocator->float_regs;
    for (int i = 0; i < NUM_REG_RANGES; i++) {
      if (register_ranges[i].type == type) {
        for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
          if (regs[j].base.owner == NULL) {
            printf("%s(r%d): free\n", ARMRegisterNameNoSize(&regs[j], 0, buf, sizeof(buf)), regs[j].base.num);
          } else {
            TargetInstruction* owner = regs[j].base.owner;
            printf("%s(r%d): owner: @%d %s\n", ARMRegisterNameNoSize(&regs[j], 0, buf, sizeof(buf)), regs[j].base.num, owner->id,
                   ARMOpcodeName(owner->opcode));
          }
        }
      }
    }
  }
}

static void AssignRegister(ARMRegister* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static ARMRegister* FindFreeRegister(ARMRegisterAllocator* allocator,
                                    ARMRegisterType type, bool can_use_temp) {
  ARMRegister* regs =
      type == kARMRegTypeInt ? allocator->int_regs : allocator->float_regs;
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

static void FreeRegister(ARMRegisterAllocator* allocator, ARMRegister* reg) {
  reg->base.owner = NULL;
}


// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(ARMRegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      if (ARMIsFixedRegister(op)) {
        continue;
      }
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && reg->owner != NULL && op->uses > 0) {
        op->uses--;
        assert(op->uses >= 0);
        if (op->uses == 0) {
          if (reg->owner == op) {
            FreeRegister(allocator, (ARMRegister*)reg);
          }
        }
      }
    }
  }
}

// Is the register meant to be saved by the callee?
static bool IsSavedReg(ARMRegister* reg) {
  int num = reg->base.num;
  if ((num >= ARM_INT_SAVED_START && num <= ARM_INT_SAVED_END) ||
      (num >= ARM_FP_SAVED_START && num <= ARM_FP_SAVED_END)) {
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

static TargetInstruction* FindSpillVictim(ARMRegisterAllocator* allocator,
                                   ARMRegisterType type) {
  ARMRegister* regs =
      type == kARMRegTypeInt ? allocator->int_regs : allocator->float_regs;
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

static ARMRegister* SpillInstruction(ARMRegisterAllocator* allocator, TargetInstruction* inst) {
  ARMRegister* reg = (ARMRegister*)inst->reg;    // Current register.
  
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  TrapSpill(inst);
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)ARM_OP(spill), NULL,
                                                   TargetGetIntConstant(&allocator->g->base,
                                                                        NULL,
                                                                        kTargetType32Bit,
                                                                        allocator->current_spilled_region_size));
  allocator->current_spilled_region_size += 8;    // Space for one register.
  if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size = allocator->current_spilled_region_size;
  }
  printf("Spilled @%d (reg %d) as @%d\n", inst->id, reg->base.num, spill->id);
 
  if (ARMIsVarRegister(inst)) {
    // Spilling a varreg->base.  This instruction is in the entry block but
    // it can't be spilled there.  It needs to be spilled at its first
    // use (the assignment to it).  This is going to be the first user
    // of the instruction.
    assert(inst->users.length > 0);
    TargetInstruction* first_use = inst->users.value.p[0];
    TargetBasicBlockEmitAfter(&allocator->g->base, first_use->block, spill, first_use);
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

static ARMRegister* AllocateRegisterWithType(ARMRegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            ARMRegisterType type,
                                            bool can_use_temp) {
  ARMRegister* reg = FindFreeRegister(allocator, type, can_use_temp);

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
    case kARMRegTypeInt:
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;
    case kARMRegTypeFloat:
      BitSetInsert(&allocator->used_float_regs, reg->base.num);
      break;
  }
  return reg;
}

static ARMRegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(constf):
    case ARM_OP(constd):
    case ARM_OP(fmv_s):
    case ARM_OP(fmv_d):
    case ARM_OP(d0):
    case ARM_OP(d1):
    case ARM_OP(d2):
    case ARM_OP(d3):
    case ARM_OP(d4):
    case ARM_OP(d5):
    case ARM_OP(d6):
    case ARM_OP(d7):
    case ARM_OP(fldr):
    case  ARM_OP(fstr):
    case  ARM_OP(fadd):
    case  ARM_OP(fsub):
    case  ARM_OP(fmul):
    case  ARM_OP(fdiv):
    case  ARM_OP(fsqrt):
    case  ARM_OP(fmin):
    case  ARM_OP(fmax):
    case  ARM_OP(fcvtsd):     // Single to double
    case  ARM_OP(fcvtds):     // Double to single.
    case  ARM_OP(fcvt):     // Copy from int reg (no conversion)
    case  ARM_OP(fmov):
    case  ARM_OP(fcmp):
    case  ARM_OP(scvtf):
    case  ARM_OP(ucvtf):
    case  ARM_OP(fneg):
    case ARM_OP(fvarreg):
      return kARMRegTypeFloat;

    case  ARM_OP(fcvtns):
    case  ARM_OP(fcvtnu):
      return kARMRegTypeInt;

    default:
      if ((ARMOpcode)inst->opcode >= ARM_OP(fldr) &&
          (ARMOpcode)inst->opcode <= ARM_OP(fneg)) {
        return kARMRegTypeFloat;
      }
      return kARMRegTypeInt;
  }
}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(ARMRegisterAllocator* allocator, TargetInstruction* inst) {
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void AllocateVariableRegister(ARMRegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  ARMRegisterType reg_type = RegisterTypeFromInstruction(inst);
  
  ARMRegister* reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                 reg_type, CanUseTemp(allocator, inst));
  AssignRegister(reg, inst);
}

static COMPILER_UNUSED void AllocateForRmov(ARMRegisterAllocator* allocator,
                            TargetInstruction* inst) {
  assert(((int)inst->opcode == (int)ARM_OP(mv)) || ((int)inst->opcode == (int)ARM_OP(fmv_s)) ||
         ((int)inst->opcode == (int)ARM_OP(fmv_d)));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  if (ARMIsVarRegister(dest) && dest->reg == NULL) {
    // Delayed allocation of variable register.
    AllocateVariableRegister(allocator, dest);
  }
  ARMRegister* reg = (ARMRegister*)dest->reg;
  assert(reg != NULL);
  
  if (((int)src->opcode == (int)ARM_OP(spill))) {
    // If we are rmoving a spill we can just load it directly into the
    // destination register.  To do this, we convert the rmov
    // into a reload instruction
    inst->opcode = (TargetOpcode)ARM_OP(reload);
    inst->operand[0] = src;
    inst->operand[1] = NULL;
    TrapReload(inst);
  } else {
    if (ARMIsVarRegister(src) && src->reg == NULL) {
      // Delayed allocation of variable register.
      AllocateVariableRegister(allocator, src);
    }
    inst->operand[0]->uses++;
    FreeRegisters(allocator, inst);
  }
  
  inst->reg = &reg->base;
  inst->flags |= TARGET_INST_PROCESSED;
}


static void ReloadSpills(ARMRegisterAllocator* allocator,
                         TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && ((int)op->opcode == (int)ARM_OP(spill))) {
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)ARM_OP(reload),
                                                        op);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->g->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      ARMRegisterType reg_type = RegisterTypeFromInstruction(inst);
      ARMRegister *reg = AllocateRegisterWithType(allocator, reload->block, reload,
                                     reg_type, CanUseTemp(allocator, reload));
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
    }
  }
}

static void AllocateRegister(ARMRegisterAllocator* allocator,
                             TargetInstruction* inst) {
   bool is_leaf = allocator->g->base.num_calls == 0 &&
      compiler->optimize;

  ARMOpcode opcode = (ARMOpcode)inst->opcode;
  
  TrapInstruction(inst);

  // If we already have a register allocated (as can be the case
  // for an ivarreg that is the dest of another instruction) don't
  // reallocate register.
  if (inst->reg != NULL) {
    return;
  }
  
  
  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == ARM_OP(mv) || opcode == ARM_OP(fmv_s) ||
      opcode == ARM_OP(fmv_d)) && inst->dest != NULL) {
    AllocateForRmov(allocator, inst);
    return;
  }

  if (ARMIsVarRegister(inst)) {
    // Variable regsiter.  Delay allocation until it's assigned to.
    // It will be assigned to by an rmov or from a destination
    // assignemnt.
    return;
  }

  // Reload any spilled expressions.
  ReloadSpills(allocator, inst);

  ARMRegister* reg;

  if (inst->dest != NULL) {
    if (inst->dest->reg == NULL) {
      if (ARMIsVarRegister(inst->dest)) {
        // Assignment to a variable register,
        AllocateVariableRegister(allocator, inst->dest);
      } else {
        AllocateRegister(allocator, inst->dest);
      }
    }
    assert(inst->dest->reg != NULL);
    reg = (ARMRegister*)inst->dest->reg;
    inst->reg = inst->dest->reg;
    FreeRegisters(allocator, inst);
    inst->flags |= TARGET_INST_PROCESSED;
    return;
  }

  // Free up any registers we can.
  FreeRegisters(allocator, inst);
  
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(const8):
    case ARM_OP(const16):
    case ARM_OP(const32):
    case ARM_OP(const64):
    case ARM_OP(constf):
    case ARM_OP(constd):
    case ARM_OP(symbol):
    case ARM_OP(b):
    case ARM_OP(br):
    case ARM_OP(label):
    case ARM_OP(ret):
    case ARM_OP(save):
    case ARM_OP(restore):
    case ARM_OP(literal):
    case ARM_OP(asm):
    case ARM_OP(loc):
    case  ARM_OP(eq):
    case  ARM_OP(ne):
    case  ARM_OP(cs):
    case  ARM_OP(hs):
    case  ARM_OP(cc):
    case  ARM_OP(lo):
    case  ARM_OP(mi):
    case   ARM_OP(pl):
    case   ARM_OP(vs):
    case   ARM_OP(vc):
    case   ARM_OP(hi):
    case   ARM_OP(ls):
    case   ARM_OP(ge):
    case   ARM_OP(lt):
    case   ARM_OP(gt):
    case   ARM_OP(le):
    case   ARM_OP(al):
    case   ARM_OP(cmp):
    case   ARM_OP(fcmp):
    case   ARM_OP(oplsl):
      // These instructions do not have registers allocated to them.
      return;

    case ARM_OP(regarg):
      // Always refers to fixed register so no allocation necessry.
      return;
      
    case ARM_OP(fp):
      reg = &allocator->int_regs[ARM_FP_REG];
      break;

    case ARM_OP(sp):
      reg = &allocator->int_regs[ARM_SP_REG];
      break;

    case ARM_OP(lr):
      reg = &allocator->int_regs[ARM_LR_REG];
      break;

    case ARM_OP(xr):
      reg = &allocator->int_regs[ARM_XR_REG];
      break;

    case ARM_OP(r0):
    case ARM_OP(r1):
    case ARM_OP(r2):
    case ARM_OP(r3):
    case ARM_OP(r4):
    case ARM_OP(r5):
    case ARM_OP(r6):
    case ARM_OP(r7):
      reg = &allocator->int_regs[(int)inst->opcode - ARM_OP(r0) + ARM_INT_ARG_START];
      break;

    case ARM_OP(ivarreg):
    case ARM_OP(fvarreg):
      assert(false);
      COMPILER_UNREACHABLE();
      
    case ARM_OP(d0):
    case ARM_OP(d1):
    case ARM_OP(d2):
    case ARM_OP(d3):
    case ARM_OP(d4):
    case ARM_OP(d5):
    case ARM_OP(d6):
    case ARM_OP(d7):
      reg = &allocator
                 ->float_regs[(int)inst->opcode - ARM_OP(d0) + ARM_FP_ARG_START];
      break;

    case ARM_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? ARM_FIRST_LEAF_INT_REG_VAR
                                          : ARM_FIRST_INT_REG_VAR) +
                                 allocator->g->struct_return_reg];
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
      break;

    case ARM_OP(resulti):
      reg = &allocator->int_regs[ARM_INT_RETURN_REG];
      break;

    case ARM_OP(resultf):
    case ARM_OP(resultd):
      reg = &allocator->float_regs[ARM_FLOAT_RETURN_REG];
      break;

    case ARM_OP(bl):
    case ARM_OP(blr):
      reg = &allocator->int_regs[ARM_INT_RETURN_REG];
      break;
#if 0
    case ARM_OP(callf):
    case ARM_OP(rcallf):
      reg = &allocator->float_regs[ARM_FLOAT_RETURN_REG];
#endif
      break;
      

    default: {
      ARMRegisterType reg_type = RegisterTypeFromInstruction(inst);
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

static void InitializeBasicBlockRegisters(ARMRegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < ARM_NUM_INT_REGS; i++) {
    ARMRegister* reg = &allocator->int_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
  }
  for (int i = 0; i < ARM_NUM_FLOAT_REGS; i++) {
     ARMRegister* reg = &allocator->float_regs[i];
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
    if (((int)inst->opcode == (int)ARM_OP(spill)) ||
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
  ARMRegisterAllocator* allocator = data;

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

static void ProcessBasicBlock(ARMRegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->g->base, ProcessBlock,
                          kTraversePreOrder, allocator);
}


// Build the preserved_instructions set, instructions that need their
// register to be preserved across calls.  If the block contains a call
// all outputs need to be preserved.
static void BuildPreservedInstructionsSet(TargetBasicBlock* block, void* data) {
  ARMRegisterAllocator* allocator = data;
  if (!block->contains_call) {
    return;
  }
  // Preserve all outputs.
  BitSetUnionInPlace(&allocator->preserved_instructions, &block->output_ids);
}

void ARMAllocateRegisters(ARMRegisterAllocator* allocator) {
  TargetTraverseDominatorTree(&allocator->g->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);

  // Process all basic blocks in the ARM generator by traversing the
  // dominator tree.
  ProcessBasicBlock(allocator, allocator->g->base.entry_block);
}

static const char* ARMRegisterNameNoSize(ARMRegister* reg, int size, char* buf, size_t len) {
  return ARMRegisterNameFromNum1(reg->base.num, reg->type, size, true, buf, len);
}

const char* ARMRegisterName(ARMRegister* reg, int size, char* buf, size_t len) {
  return ARMRegisterNameFromNum(reg->base.num, reg->type, size, buf, len);
}

static const char* ARMRegisterNameFromNum1(int num, ARMRegisterType type, int size,
                                    bool allow_no_size,
                                   char* buf,
                                           size_t len) {
  (void)allow_no_size;
  switch (type) {
    case kARMRegTypeInt:
      switch (num) {
        case ARM_FP_REG:
          snprintf(buf, len, "fp");
          return buf;
        case ARM_SP_REG:
          snprintf(buf, len, "sp");
          return buf;
        case ARM_LR_REG:
          snprintf(buf, len, "lr");
          return buf;
        case ARM_IP_REG:
          snprintf(buf, len, "ip");
          return buf;
        case ARM_PC_REG:
          snprintf(buf, len, "pc");
          return buf;
        default:
          snprintf(buf, len, "r%d", num);
          return buf;
      }

    case kARMRegTypeFloat:
      if (size == kSize32Bit || size == 0) {
        snprintf(buf, len, "s%d", num);
      } else {
        snprintf(buf, len, "d%d", num / 2);
      }
      return buf;
  }

  return buf;
}

const char* ARMRegisterNameFromNum(int num, ARMRegisterType type, int size,
                                   char* buf,
                                   size_t len) {
  return ARMRegisterNameFromNum1(num, type, size, false, buf, len);
}

#undef NUM_REG_RANGES

