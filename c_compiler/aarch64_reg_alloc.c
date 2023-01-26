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

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  BitSetInit(&allocator->preserved_instructions);
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
  {kAARCH64RegTypeInt, AARCH64_INT_ARG_START, AARCH64_INT_ARG_END, true},
  {kAARCH64RegTypeInt, AARCH64_INT_TEMP_START, AARCH64_INT_TEMP_END, true},
    {kAARCH64RegTypeInt, AARCH64_INT_ARG_START, AARCH64_INT_ARG_END, true},
    {kAARCH64RegTypeInt, AARCH64_INT_SAVED_START, AARCH64_INT_SAVED_END, false},
    {kAARCH64RegTypeFloat, AARCH64_FP_TEMP_START, AARCH64_FP_TEMP_END, true},
    {kAARCH64RegTypeFloat, AARCH64_FP_ARG_START, AARCH64_FP_ARG_END, true},
    {kAARCH64RegTypeFloat, AARCH64_FP_SAVED_START, AARCH64_FP_SAVED_END,false},
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


// Free up any registers that are no longer needed by the instruction.  This
// frees up all now-unused operands and destination.
static void FreeRegisters(AARCH64RegisterAllocator* allocator,
                          TargetInstruction* inst) {
  int dest_id = -1;
  if (inst->dest != NULL) {
    dest_id = inst->dest->id;
  }
  bool dest_in_operands = false;
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      if (op->id == dest_id) {
        dest_in_operands = true;
      }
      if (AARCH64IsFixedRegister(op)) {
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
  if ((num >= AARCH64_INT_SAVED_START && num <= AARCH64_INT_SAVED_END) ||
      (num >= AARCH64_FP_SAVED_START && num <= AARCH64_FP_SAVED_END)) {
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

static TargetInstruction* FindSpillVictim(AARCH64RegisterAllocator* allocator,
                                   AARCH64RegisterType type) {
  AARCH64Register* regs =
      type == kAARCH64RegTypeInt ? allocator->int_regs : allocator->float_regs;
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
  printf("Spilled @%d (reg %d) as @%d\n", inst->id, reg->base.num, spill->id);
 
  if (AARCH64IsVarRegister(inst)) {
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

static AARCH64Register* AllocateRegisterWithType(AARCH64RegisterAllocator* allocator,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            AARCH64RegisterType type,
                                            bool can_use_temp) {
  AARCH64Register* reg = FindFreeRegister(allocator, type, can_use_temp);

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
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void AllocateVariableRegister(AARCH64RegisterAllocator* allocator,
                                     TargetInstruction* inst) {
  AARCH64RegisterType reg_type = RegisterTypeFromInstruction(inst);
  
  AARCH64Register* reg = AllocateRegisterWithType(allocator, inst->block, inst,
                                 reg_type, CanUseTemp(allocator, inst));
  AssignRegister(reg, inst);
}

static void AllocateForRmov(AARCH64RegisterAllocator* allocator,
                            TargetInstruction* inst) {
  assert(inst->opcode == AARCH64_OP(mv) || inst->opcode == AARCH64_OP(fmv_s) ||
         inst->opcode == AARCH64_OP(fmv_d));
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];

  if (AARCH64IsVarRegister(dest) && dest->reg == NULL) {
    // Delayed allocation of variable register.
    AllocateVariableRegister(allocator, dest);
  }
  AARCH64Register* reg = (AARCH64Register*)dest->reg;
  assert(reg != NULL);
  
  if (src->opcode == AARCH64_OP(spill)) {
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
    if (op != NULL && op->opcode == AARCH64_OP(spill)) {
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)AARCH64_OP(reload),
                                                        op);
      TrapReload(reload);
      TargetBasicBlockEmitBefore(&allocator->g->base, inst->block, reload, inst);
      inst->operand[i] = reload;
      AARCH64RegisterType reg_type = RegisterTypeFromInstruction(inst);
      AARCH64Register *reg = AllocateRegisterWithType(allocator, reload->block, reload,
                                     reg_type, CanUseTemp(allocator, reload));
      AssignRegister(reg, reload);
      // This reload is for a single instruction.
      reload->uses = 1;
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
  
  
  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if ((opcode == AARCH64_OP(mv) || opcode == AARCH64_OP(fmv_s) ||
      opcode == AARCH64_OP(fmv_d)) && inst->dest != NULL) {
    AllocateForRmov(allocator, inst);
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

  AARCH64Register* reg;

  if (inst->dest != NULL) {
    if (inst->dest->reg == NULL) {
      if (AARCH64IsVarRegister(inst->dest)) {
        // Assignment to a variable register,
        AllocateVariableRegister(allocator, inst->dest);
      } else {
        AllocateRegister(allocator, inst->dest);
      }
    }
    assert(inst->dest->reg != NULL);
    reg = (AARCH64Register*)inst->dest->reg;
    inst->reg = inst->dest->reg;
    FreeRegisters(allocator, inst);
    inst->flags |= TARGET_INST_PROCESSED;
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
    case   AARCH64_OP(oplsl):
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
      break;
      
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
      BitSetInsert(&allocator->used_int_regs, reg->base.num);
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
      reg = &allocator->int_regs[AARCH64_INT_RETURN_REG];
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
    
  // Now allocate the registers to the inputs.
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* inst = block->inputs.value.p[i];
    if (inst->reg == NULL) {
      continue;
    }
    if (inst->opcode == AARCH64_OP(spill) ||
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
  AARCH64RegisterAllocator* allocator = data;

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

static void ProcessBasicBlock(AARCH64RegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->g->base, ProcessBlock,
                          kTraversePreOrder, allocator);
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

      snprintf(buf, len, "x%d", num);
      return buf;

    case kAARCH64RegTypeFloat:
      snprintf(buf, len, "d%d", num);
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

