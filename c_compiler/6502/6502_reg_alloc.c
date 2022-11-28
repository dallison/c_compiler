//
//  6502_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 6/5/19.
//  Copyright © 2019 David Allison. All rights reserved.
//
#include <limits.h>

#include "6502_reg_alloc.h"
#include <assert.h>
#include "6502_codegen.h"
#include "6502_machine.h"
#include "target_basic_block.h"

static void DumpRegisters(W65C02RegisterAllocator* allocator);

static void Trap() {}
static void TrapInstruction(W65C02RegisterAllocator* allocator, TargetInstruction* inst) {
  if (inst->id == 795) {
    // Set breakpoint here to trap on a certain ianstruction id.
    Trap();
  }
}

static void TrapRegister(W65C02RegisterAllocator* allocator, W65C02Register* reg) {
  W65C02RegisterType type = k6502RegTypeI;
  int num = 1;
  if (reg->type == type && reg->base.num == num) {
    Trap();
  }
}

static void TrapAllocRegister(W65C02RegisterAllocator* allocator, W65C02Register* reg) {
  W65C02RegisterType type = k6502RegTypeB;
  int num = 6;
  if (reg->type == type && reg->base.num == num) {
  //  DumpRegisters(allocator);
    Trap();
  }
}

static void TrapFreeRegister(W65C02RegisterAllocator* allocator, W65C02Register* reg) {
  W65C02RegisterType type = k6502RegTypeI;
  int num = 4;
  if (reg->type == type && reg->base.num == num) {
    Trap();
  }
}


static void InitializeZeroPageRegister(W65C02Register* reg, int num,
                                       W65C02RegisterType type, bool temp) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
  reg->temp = temp;
}

#define INIT_REGS(set, type) \
for (int i = 0; i < W65C02_NUM_TEMP_##type##_REGS; i++) { \
  InitializeZeroPageRegister(&allocator->set[i], i, k6502RegType##type, true); \
} \
for (int i = W65C02_NUM_TEMP_##type##_REGS; i < W65C02_NUM_##type##_REGS; i++) { \
  InitializeZeroPageRegister(&allocator->set[i], i, k6502RegType##type, false); \
}

void W65C02RegisterAllocatorInit(W65C02RegisterAllocator* allocator,
                                W65C02Generator* g) {
  allocator->g = g;

  INIT_REGS(b_regs, B);
  INIT_REGS(i_regs, I);
  INIT_REGS(l_regs, L);
  INIT_REGS(x_regs, X);
  INIT_REGS(f_regs, F);

#undef INIT_REGS

  InitializeZeroPageRegister(&allocator->sp_reg, W65C02_SP_REG, k6502RegTypeI, false);
  InitializeZeroPageRegister(&allocator->fp_reg, W65C02_FP_REG, k6502RegTypeI, false);
  BitSetInit(&allocator->used_b_regs);
  BitSetInit(&allocator->used_i_regs);
  BitSetInit(&allocator->used_l_regs);
  BitSetInit(&allocator->used_x_regs);
  BitSetInit(&allocator->used_f_regs);
  allocator->current_spilled_region_size = 0;
  allocator->max_spilled_region_size = 0;
  MapInitForInt64Keys(&allocator->spill_points);
  BitSetInit(&allocator->preserved_instructions);
}

W65C02RegisterAllocator* New6502RegisterAllocator(W65C02Generator* g) {
  W65C02RegisterAllocator* reg_alloc = malloc(sizeof(W65C02RegisterAllocator));
  W65C02RegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void W65C02RegisterAllocatorDestruct(W65C02RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_b_regs);
  BitSetDestruct(&allocator->used_i_regs);
  BitSetDestruct(&allocator->used_l_regs);
  BitSetDestruct(&allocator->used_x_regs);
  BitSetDestruct(&allocator->used_f_regs);
  MapDestruct(&allocator->spill_points);
  BitSetDestruct(&allocator->preserved_instructions);
}

void W65C02RegisterAllocatorDelete(W65C02RegisterAllocator* alloc) {
  W65C02RegisterAllocatorDestruct(alloc);
  free(alloc);
}

void W65C02RegisterAllocatorAddSpillPoint(W65C02RegisterAllocator* alloc,
                                            int expr, TargetInstruction* spill_point) {
  MapKeyValue kv = {.key.w = expr, .value.p = spill_point};
  MapInsert(&alloc->spill_points, kv);
}

void W65C02RegisterAllocatorRemoveSpillPoint(W65C02RegisterAllocator* alloc,
                                               int expr) {
  MapKeyType key = {.w = expr};
  MapRemove(&alloc->spill_points, key);
}


// Calculate the cost of spilling the instruction.
static int SpillCost(TargetInstruction* inst) {
  int max_diff = 0;
  for (int i = 0; i < inst->users.length; i++) {
    TargetInstruction* user = inst->users.value.p[i];
    int diff = user->addr - inst->addr;
    if (diff > max_diff) {
      max_diff = diff;
    }
  }
  // printf("possible victim @%d has spill cost %d\n", inst->id, -max_diff);
  return -max_diff;
}

static bool IsSpillInstruction(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(spill1):
    case W65C02_OP(spill2):
    case W65C02_OP(spill4):
    case W65C02_OP(spill8):
    case W65C02_OP(reload1):
    case W65C02_OP(reload2):
    case W65C02_OP(reload4):
    case W65C02_OP(reload8):
      return true;
    default:
      return false;
  }
}

static bool IsRegVar(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
      return true;
    default:
      return false;
  }
}

static bool IsSpillOnly(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(spill1):
    case W65C02_OP(spill2):
    case W65C02_OP(spill4):
    case W65C02_OP(spill8):
       return true;
    default:
      return false;
  }
}
static int RegisterSize(W65C02Register* reg) {
  switch (reg->type) {
      case k6502RegTypeB:
      return 1;
      case k6502RegTypeI:
        return 2;
      case k6502RegTypeL:
        return 4;
      case k6502RegTypeX:
       return 8;
      case k6502RegTypeF:
        return 4;
   }
}

static void DumpReg(W65C02Register* reg, int num, W65C02RegisterType type) {
  if (reg->base.owner == NULL) {
    return;
  }
  char t;
  switch (type) {
      case k6502RegTypeB:
      t = 'b';
      break;
      case k6502RegTypeI:
        t = 'i';
        break;
      case k6502RegTypeL:
        t = 'l';
        break;
      case k6502RegTypeX:
       t = 'x';
       break;
      case k6502RegTypeF:
        t = 'f';
        break;
  }
  bool is_spill = IsSpillInstruction(reg->base.owner);
  bool is_reg_var = IsRegVar(reg->base.owner);
    
  int cost = SpillCost(reg->base.owner);
  printf("reg %c%d: owner: @%d, locked: %d: IsSpill: %d, IsRegVar: %d, cost: %d\n", t, num, reg->base.owner->id, reg->locked,
         is_spill, is_reg_var, cost);
}

static void DumpRegisters(W65C02RegisterAllocator* allocator) {
  for (int i = 0; i < W65C02_NUM_B_REGS; i++) {
    DumpReg(&allocator->b_regs[i], i, k6502RegTypeB);
  }

  for (int i = 0; i < W65C02_NUM_I_REGS; i++) {
    DumpReg(&allocator->i_regs[i], i, k6502RegTypeI);
  }

  for (int i = 0; i < W65C02_NUM_L_REGS; i++) {
    DumpReg(&allocator->l_regs[i], i, k6502RegTypeL);
  }

  for (int i = 0; i < W65C02_NUM_X_REGS; i++) {
    DumpReg(&allocator->x_regs[i], i, k6502RegTypeX);
  }

  for (int i = 0; i < W65C02_NUM_F_REGS; i++) {
    DumpReg(&allocator->f_regs[i], i, k6502RegTypeF);
  }

}

// Can we use a temp register?  If not we will have to use a saved one and
// those are more expensive since they need to be saved on entry and reloaded
// on exit.
static bool CanUseTemp(W65C02RegisterAllocator* allocator, TargetInstruction* inst) {
  if ((inst->flags & k6502ExprIsCallResult) != 0) {
    // A call result can be in a temp.
    return true;
  }
  return !BitSetContains(&allocator->preserved_instructions, inst->id);
}

static void FindSpillVictim(W65C02RegisterAllocator* allocator,
                                   W65C02RegisterType type, TargetInstruction** victim, TargetInstruction** spill_point) {
  W65C02Register* regs;
  int num_regs;
  switch (type) {
      case k6502RegTypeB:
        regs = allocator->b_regs;
        num_regs = W65C02_NUM_B_REGS;
        break;
      case k6502RegTypeI:
        regs = allocator->i_regs;
        num_regs = W65C02_NUM_I_REGS;
        break;
      case k6502RegTypeL:
        regs = allocator->l_regs;
        num_regs = W65C02_NUM_L_REGS;
        break;
      case k6502RegTypeX:
        regs = allocator->x_regs;
        num_regs = W65C02_NUM_X_REGS;
        break;
      case k6502RegTypeF:
        regs = allocator->f_regs;
        num_regs = W65C02_NUM_F_REGS;
        break;

  }
  int min_cost = INT_MAX;
  *victim = NULL;
  // Find the instruction with the lowest spill cost.
  for (int i = 0; i < num_regs; i++) {
    if (regs[i].base.owner != NULL) {
      TargetInstruction* owner = regs[i].base.owner;
      assert(owner != NULL);
      if (IsSpillInstruction(owner) || IsRegVar(owner)) {
        continue;
      }
      assert((owner->flags & TARGET_INST_SPILLED) == 0);
      
      int cost = SpillCost(owner);
      if (cost < min_cost) {
        min_cost = cost;
        *victim = owner;
      }
    }
  }
 
  if (*victim == NULL) {
    DumpRegisters(allocator);
    fprintf(stderr, "Unable to find spill victim (out of registers)\n");
    abort();
  }
#if 0
  printf("Spilling victim @%d\n", (*victim)->id);
  DumpRegisters(allocator);
#endif
  
  if (IsSpillInstruction(*victim)) {
    // Spilling a spill is a NOP.
    *spill_point = NULL;
    return;
  }
  // Now find the spill point for the victim.  It must exist.
  // Spill points are needed for 6502 because writes to "registers" are split
  // into multiple instructions.
  MapKeyType key = {.w = (*victim)->id};
  *spill_point = MapFind(&allocator->spill_points, key);
  if (*spill_point == NULL) {
    printf("Can't find spill point for @%d\n", (*victim)->id);
    abort();
  }
}

static W65C02Opcode SpillOpFromReg(W65C02Register* reg) {
  switch (reg->type) {
      case k6502RegTypeB:
      return W65C02_OP(spill1);
      case k6502RegTypeI:
        return W65C02_OP(spill2);
      case k6502RegTypeL:
        return W65C02_OP(spill4);
      case k6502RegTypeX:
       return W65C02_OP(spill8);
      case k6502RegTypeF:
        return W65C02_OP(spill4);
  }
}

static W65C02Opcode ReloadOpFromReg(W65C02Register* reg) {
  switch (reg->type) {
      case k6502RegTypeB:
      return W65C02_OP(reload1);
      case k6502RegTypeI:
        return W65C02_OP(reload2);
      case k6502RegTypeL:
        return W65C02_OP(reload4);
      case k6502RegTypeX:
       return W65C02_OP(reload8);
      case k6502RegTypeF:
        return W65C02_OP(reload4);
  }
}


static bool IsAfterSpillPoint(TargetInstruction* inst, void* data) {
  TargetInstruction* spill_point = data;
  return (inst->flags & TARGET_INST_PROCESSED) == 0 &&
      inst->addr > spill_point->addr;
}

static W65C02Register* SpillInstruction(W65C02RegisterAllocator* allocator,
                                          TargetInstruction* victim, TargetInstruction* spill_point) {
  W65C02Register* reg = (W65C02Register*)victim->reg;    // Current register.
  
  if (spill_point == NULL) {
    return reg;
  }
  // Generate a spill instruction with 2 operands:
  // 1. Instruction to spill (not set yet)
  // 2. Offset into spill region.
  // We don't set the spilled instruction yet because TargetRetargetInstruction
  // will see it and retarget it to the spill.
  allocator->current_spilled_region_size += RegisterSize(reg);    // Space for one register.
  if (allocator->current_spilled_region_size > allocator->max_spilled_region_size) {
    allocator->max_spilled_region_size = allocator->current_spilled_region_size;
  }
  TargetInstruction* spill = TargetNewInstruction2((TargetOpcode)SpillOpFromReg(reg), NULL,
                                                   TargetGetIntConstant(&allocator->g->base,
                                                                        NULL,
                                                                        kTargetType32Bit,
                                                                        allocator->current_spilled_region_size));

  // printf("Spilled @%d (reg %d) as @%d\n", victim->id, reg->base.num, spill->id);
 
  // Emit spill instruction just after spill point.
  TargetBasicBlockEmitAfter(&allocator->g->base, victim->block, spill, spill_point);
  
  // Retarget all uses of the original instruction to the spill.  If the
  // user has already been processed this will have no effect.
  // NOTE: this will transfer all uses of the inst to the spill, leaving
  // the users of inst empty and its uses count 0.
  TargetRetargetInstructionIf(victim, spill, IsAfterSpillPoint, spill_point);
  spill->operand[0] = victim;
  spill->reg = victim->reg;
  reg->base.owner = NULL;
  victim->flags |= TARGET_INST_SPILLED;
  return reg;
}

static void AssignRegister(W65C02Register* reg, TargetInstruction* inst) {
  assert(inst->reg == NULL);
  inst->reg = &reg->base;
  reg->base.owner = inst;
  inst->uses = (int)inst->users.length;
  inst->flags |= TARGET_INST_PROCESSED;
}

static W65C02Register* FindFreeRegister(W65C02RegisterAllocator* allocator,
                                       W65C02RegisterType type, bool can_use_temp) {
  int num_regs;
  W65C02Register* regs;
  switch (type) {
    case k6502RegTypeB:
      regs = allocator->b_regs;
      num_regs = W65C02_NUM_B_REGS;
      break;
    case k6502RegTypeI:
      regs = allocator->i_regs;
      num_regs = W65C02_NUM_I_REGS;
      break;
    case k6502RegTypeL:
      regs = allocator->l_regs;
      num_regs = W65C02_NUM_L_REGS;
      break;
    case k6502RegTypeX:
      regs = allocator->x_regs;
      num_regs = W65C02_NUM_X_REGS;
      break;
    case k6502RegTypeF:
      regs = allocator->f_regs;
      num_regs = W65C02_NUM_F_REGS;
      break;
  }

  for (int i = 0; i < num_regs; i++) {
    if (!regs[i].base.reserved && regs[i].base.owner == NULL) {
      if (!can_use_temp && regs[i].temp) {
        continue;
      }
      return &regs[i];
    }
  }
  return NULL;
}

static void FreeRegister(W65C02RegisterAllocator* allocator,
                         W65C02Register* reg) {
  TrapRegister(allocator, reg);
  TrapFreeRegister(allocator, reg);
  reg->base.owner = NULL;
}

static W65C02RegisterType RegisterTypeFromTypeRecord(TypeRecord* type) {
  if (TypeIsPointerOrArray(type) || TypeIsStructOrUnion(type) ||
      TypeIsFunction(type)) {
    return k6502RegTypeI;
  } else if (TypeIsFloat(type) || TypeIsDouble(type)) {
    return k6502RegTypeF;
  } else if (TypeIsBool(type)) {
    return k6502RegTypeB;
  } else if (TypeIsChar(type)) {
    return k6502RegTypeB;
  } else if (TypeIsLong(type)) {
    return k6502RegTypeL;
  } else if (TypeIsLongLong(type)) {
    return k6502RegTypeX;
  } else if (TypeIsShort(type)) {
    return k6502RegTypeI;
  }
  return k6502RegTypeI;
}

static W65C02RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(symbol):
    case W65C02_OP(localvar):
    case W65C02_OP(argument): {
      // For a symbol we need to get the type of the symbol:
      TargetSymbol* target_sym = (TargetSymbol*)inst;
      Symbol* sym = target_sym->symbol;
      TypeRecord* type = sym->type;
      return RegisterTypeFromTypeRecord(type);
    }

    case W65C02_OP(ssavar):
    case W65C02_OP(phi): {
      TargetSymbol* target_sym = (TargetSymbol*)inst->operand[0];
      Symbol* sym = target_sym->symbol;
      TypeRecord* type = sym->type;
      return RegisterTypeFromTypeRecord(type);
      break;
    }

    case W65C02_OP(expr1):
    case W65C02_OP(bvarreg):
      return k6502RegTypeB;

    case W65C02_OP(expr2):
    case W65C02_OP(ivarreg):
      return k6502RegTypeI;

    case W65C02_OP(structreturn):
      return k6502RegTypeI;

    case W65C02_OP(expr4):
    case W65C02_OP(lvarreg):
      return k6502RegTypeL;

    case W65C02_OP(expr8):
    case W65C02_OP(xvarreg):
      return k6502RegTypeX;

    case W65C02_OP(fvarreg):
    case W65C02_OP(exprf):
      return k6502RegTypeF;
      
    case W65C02_OP(dvarreg):
    case W65C02_OP(exprd):
      return k6502RegTypeF;

    default:
      printf("opcode: %s\n", W65C02OpcodeName(inst->opcode));
      assert(false);
      return 0;
  }
}

static void FreeRegisters(W65C02RegisterAllocator* allocator,
                          TargetInstruction* inst) {
  if (IsSpillOnly(inst)) {
    return;
  }

  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      W65C02Register* reg = (W65C02Register*)op->reg;
      if (IsSpillInstruction(op)) {
        continue;
      }
      if (reg != NULL && !reg->base.reserved && !reg->locked) {
        op->uses--;
        // printf("inst @%d, op: @%d, uses: %d: output: %d\n", inst->id, op->id, op->uses, TargetBasicBlockOutputs(inst->block, op));
        assert(op->uses >= 0);
        if (op->uses == 0 && !TargetBasicBlockOutputs(inst->block, op)) {
          FreeRegister(allocator, reg);
        }
      }
    }
  }
}

static W65C02Register* AllocateRegisterWithType(
    W65C02RegisterAllocator* allocator, W65C02RegisterType type, bool can_use_temp) {
  W65C02Register* reg = FindFreeRegister(allocator, type, can_use_temp);
  if (reg == NULL) {
    TargetInstruction* victim, *spill_point;
    FindSpillVictim(allocator, type, &victim, &spill_point);
    reg = SpillInstruction(allocator, victim, spill_point);
  }
  TrapRegister(allocator, reg);
  TrapAllocRegister(allocator, reg);
  assert(reg != NULL);
  if (reg->temp) {
    // A temp register isn't recorded as being used.
    return reg;
  }
  switch (type) {
    case k6502RegTypeB:
      BitSetInsert(&allocator->used_b_regs, reg->base.num);
      break;
    case k6502RegTypeI:
      BitSetInsert(&allocator->used_i_regs, reg->base.num);
      break;
    case k6502RegTypeL:
      BitSetInsert(&allocator->used_l_regs, reg->base.num);
      break;
    case k6502RegTypeX:
      BitSetInsert(&allocator->used_x_regs, reg->base.num);
      break;
    case k6502RegTypeF:
      BitSetInsert(&allocator->used_f_regs, reg->base.num);
      break;
  }
  return reg;
}

struct ReloadData {
  TargetInstruction* spilled;
  TargetInstruction* reload;
};

static void RetargetToReload(TargetBasicBlock* block, void* data) {
  struct ReloadData* rdata = data;
  for (TargetInstruction* inst = block->code;
       inst != NULL && TargetPrev(inst) != block->end_code;
       inst = TargetNext(inst)) {
    if (inst == rdata->reload) {
      continue;
    }
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* op = inst->operand[i];
      if (op == rdata->spilled) {
        assert(op->uses > 0);
        TargetReplaceOperand(inst, i, rdata->reload);
      }
    }
  }
  // Add the reload instruction to the inputs of the block if the block has
  // the spilled instruction as an input.
  if (BitSetContains(&block->input_ids, rdata->spilled->operand[0]->id)) {
    if (!BitSetContains(&block->input_ids, rdata->reload->id)) {
      VectorAppend(&block->inputs, rdata->reload);
      BitSetInsert(&block->input_ids, rdata->reload->id);
    }
  }
  // Same for outputs.
  if (BitSetContains(&block->output_ids, rdata->spilled->operand[0]->id)) {
    if (!BitSetContains(&block->output_ids, rdata->reload->id)) {
      VectorAppend(&block->outputs, rdata->reload);
      BitSetInsert(&block->output_ids, rdata->reload->id);
    }
  }
}

static TargetInstruction* FindReloadPoint(TargetInstruction* inst, TargetInstruction* spill) {
  void* block = inst->block;
  TargetInstruction* p = TargetPrev(inst);
  while (p != NULL && p->block == block) {
    if (p->opcode == (TargetOpcode)W65C02_OP(reloadpoint) && p->operand[0] == spill) {
      return p;
    }
    p = TargetPrev(p);
  }
  printf("Cannot find reload point for instruction @%d\n", inst->id);
  abort();
}

static void ReloadSpills(W65C02RegisterAllocator* allocator,
                         TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL && IsSpillOnly(op)) {
      W65C02Register* reg = (W65C02Register*)op->reg;
      TargetInstruction* reloadpoint = FindReloadPoint(inst, op);
      TargetInstruction* reload = TargetNewInstruction1((TargetOpcode)ReloadOpFromReg(reg),
                                                        op);

      reload->uses = op->uses;
      TargetBasicBlockEmitAfter(&allocator->g->base, inst->block, reload, reloadpoint);
      reg = AllocateRegisterWithType(allocator,
                                     reg->type, CanUseTemp(allocator, inst));
      assert(reg != NULL);
      AssignRegister(reg, reload);
      
      // In any blocks dominated by this one, we now need to retarget
      // all references to the spilled instruction to the reload instruction.
      struct ReloadData rdata = {.spilled = op, .reload = reload};
      TargetBasicBlockTraverseDominatorTree(&allocator->g->base,
                                            inst->block,
                                            RetargetToReload,
                                            kTraversePreOrder,
                                            &rdata);
      
    }
  }
}

static bool UsesFixedRegister(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(ap):
    case W65C02_OP(sp):
    case W65C02_OP(fp):
    case W65C02_OP(tp):
      return true;
    default:
      return false;
  }
}

// rmov instructions use the register allocated to their first
// operand as their own register.
static void AllocateForRmov(W65C02RegisterAllocator* allocator,
                            TargetInstruction* inst) {
  W65C02Register* reg = (W65C02Register*)inst->operand[0]->reg;
  TargetInstruction* src = inst->operand[1];

  // See if we can reassign the src operand's register.  We can do
  // this if this is the only reference to it.  It can't be a fixed
  // register though.
  if (src->users.length == 1 && !UsesFixedRegister(src)) {
    FreeRegisters(allocator, inst);
    src->reg = &reg->base;
    src->uses++;
    reg->base.owner = src;
    inst->reg = src->reg;
    return;
  }

  // Use the register assigned to the first operand as the
  // register for this instruction.
  inst->operand[0]->uses++;  // Prevent this from being freed.
  FreeRegisters(allocator, inst);
  inst->uses = (int)inst->users.length;
  inst->reg = &reg->base;
  reg->base.owner = inst;
}

// Does the instruction need a register allocated for it?
static bool NeedsRegister(TargetInstruction* inst) {
  if ((inst->flags & k6502DontEmit) != 0) {
    return false;
  }
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
    case W65C02_OP(structreturn):
    case W65C02_OP(const8):
    case W65C02_OP(const16):
    case W65C02_OP(const32):
    case W65C02_OP(const64):
    case W65C02_OP(constf):
    case W65C02_OP(constd):
    case W65C02_OP(symbol):
    case W65C02_OP(ssavar):
    case W65C02_OP(phi):
    case W65C02_OP(fp):
    case W65C02_OP(sp):
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
      return true;
    default:
      return false;
  }
}

static void AllocateRegister(W65C02RegisterAllocator* allocator,
                             TargetInstruction* inst) {
  // Treat rmov instructions specially.
  W65C02Opcode opcode = (W65C02Opcode)inst->opcode;
#if 0
  if (opcode == W65C02_OP(rmova) ||
      opcode == W65C02_OP(rmov) ||
      opcode == W65C02_OP(rmovx) ||
      opcode == W65C02_OP(rmovf) ||
      opcode == W65C02_OP(rmovd)) {
    AllocateForRmov(allocator, inst);
    return;
  }
#endif
  TrapInstruction(allocator, inst);

  if (inst->opcode == (TargetOpcode)W65C02_OP(reloadpoint)) {
    FreeRegisters(allocator, inst);
    return;
  }

  ReloadSpills(allocator, inst);
 
  W65C02Register* reg;

  if (inst->dest != NULL) {
     if (inst->dest->reg == NULL) {
         AllocateRegister(allocator, inst->dest);
     }
     assert(inst->dest->reg != NULL);
     reg = (W65C02Register*)inst->dest->reg;
     inst->reg = inst->dest->reg;
     FreeRegisters(allocator, inst);
     return;
   }
  
  // Free up any registers we can.
  FreeRegisters(allocator, inst);

  if (!NeedsRegister(inst)) {
    return;
  }

  switch (opcode) {
    case W65C02_OP(const8):
    case W65C02_OP(const16):
    case W65C02_OP(const32):
    case W65C02_OP(const64):
     case W65C02_OP(constf):
     case W65C02_OP(constd):
    case W65C02_OP(symbol):
     case W65C02_OP(localvar):
    case W65C02_OP(argument):
      return;

    case W65C02_OP(fp):
      reg = &allocator->fp_reg;
      break;

    case W65C02_OP(sp):
      reg = &allocator->sp_reg;
      break;

    case W65C02_OP(ap):
      reg = &allocator->ap_reg;
      break;

    case W65C02_OP(resulti):
      reg = &allocator->x_regs[0];
      break;
      
    case W65C02_OP(structreturn): {
      W65C02RegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, reg_type, CanUseTemp(allocator, inst));
      reg->base.reserved = true;
      break;
    }
      
    default: {
      W65C02RegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, reg_type, CanUseTemp(allocator, inst));
      if (IsRegVar(inst)) {
        reg->locked = true;
      }
    }
  }

  inst->uses = (int)inst->users.length;
  inst->reg = &reg->base;
  reg->base.owner = inst;

  // If nobody is using this register free it up immediately.
  if (inst->uses == 0 && !reg->locked) {
    FreeRegister(allocator, reg);
  }
}

const char* W65C02RegisterAsString(W65C02Register* reg, int byte, char* buf,
                                  size_t len) {
  char bytebuf[16] = {0};
  if (byte > 0) {
    snprintf(bytebuf, sizeof(bytebuf), "+%d", byte);
  }
  switch (reg->type) {
    case k6502RegTypeB:
      snprintf(buf, len, "__b%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeI:
      if (reg->base.num == W65C02_SP_REG) {
        snprintf(buf, len, "__sp%s", bytebuf);
        break;
      }
      if (reg->base.num == W65C02_FP_REG) {
        snprintf(buf, len, "__fp%s", bytebuf);
        break;
      }
      snprintf(buf, len, "__i%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeL:
      snprintf(buf, len, "__l%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeX:
      snprintf(buf, len, "__x%d%s", reg->base.num, bytebuf);
      break;
    case k6502RegTypeF:
      snprintf(buf, len, "__f%d%s", reg->base.num, bytebuf);
      break;
  }

  return buf;
}

static void InitializeBasicBlockRegisters(W65C02RegisterAllocator* allocator,
                                          TargetBasicBlock* block) {
  for (int i = 0; i < W65C02_NUM_B_REGS; i++) {
    W65C02Register* reg = &allocator->b_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
    reg->locked = false;
  }
 for (int i = 0; i < W65C02_NUM_I_REGS; i++) {
    W65C02Register* reg = &allocator->i_regs[i];
    if (reg->base.reserved) {
      continue;
    }
    reg->base.owner = NULL;
    reg->locked = false;
  }
  for (int i = 0; i < W65C02_NUM_L_REGS; i++) {
     W65C02Register* reg = &allocator->l_regs[i];
     if (reg->base.reserved) {
       continue;
     }
     reg->base.owner = NULL;
     reg->locked = false;
   }
  for (int i = 0; i < W65C02_NUM_X_REGS; i++) {
     W65C02Register* reg = &allocator->x_regs[i];
     if (reg->base.reserved) {
       continue;
     }
     reg->base.owner = NULL;
     reg->locked = false;
   }
  for (int i = 0; i < W65C02_NUM_F_REGS; i++) {
     W65C02Register* reg = &allocator->f_regs[i];
     if (reg->base.reserved) {
       continue;
     }
     reg->base.owner = NULL;
     reg->locked = false;
   }
  
  // Now allocate the registers to the inputs.
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* inst = block->inputs.value.p[i];
    W65C02Register* reg = (W65C02Register*)inst->reg;
    if (reg == NULL) {
      continue;
    }
    if (inst->uses == 0 && !TargetBasicBlockOutputs(block, inst)) {
      continue;
    }
    // Don't allocate spilled instructions.
    if ((inst->flags & TARGET_INST_SPILLED) != 0) {
      continue;
    }
    assert(inst->reg != NULL);
    reg->base.owner = inst;
  }
}

// Build the preserved_instructions set, instructions that need their
// register to be preserved across calls.  If the block contains a call
// all outputs need to be preserved.
static void BuildPreservedInstructionsSet(TargetBasicBlock* block, void* data) {
  W65C02RegisterAllocator* allocator = data;
  if (!block->contains_call) {
    return;
  }
  // Preserve all outputs.
  BitSetUnionInPlace(&allocator->preserved_instructions, &block->output_ids);
}

static void ProcessBlock(TargetBasicBlock* block, void* data) {
  
  // printf("Allocating registers for block %zd\n", block->block_id);
  W65C02RegisterAllocator* allocator = data;

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


static void ProcessBasicBlock(W65C02RegisterAllocator* allocator,
                              TargetBasicBlock* block) {
  TargetTraverseDominatorTree(&allocator->g->base, ProcessBlock,
                          kTraversePreOrder, allocator);
}

void W65C02AllocateRegisters(W65C02RegisterAllocator* allocator) {
  TargetTraverseDominatorTree(&allocator->g->base, BuildPreservedInstructionsSet,
                          kTraversePreOrder, allocator);
  
  ProcessBasicBlock(allocator, allocator->g->base.entry_block);
}

uint32_t W65C02RegisterAllocatorBuildRegMask(W65C02RegisterAllocator* alloc) {
  uint32_t result= 0;
  static int shifts[] = {0, 5, 4, 4, 3};
  BitSet* reg_sets[5];
  reg_sets[0] = &alloc->used_i_regs;
  reg_sets[1] = &alloc->used_b_regs;
  reg_sets[2] = &alloc->used_l_regs;
  reg_sets[3] = &alloc->used_x_regs;
  reg_sets[4] = &alloc->used_f_regs;
  for (int i = 4; i >= 0; i--) {
    BitSetIterator it;
    BitSetIteratorStart(&it, reg_sets[i]);
    int num_regs = 0;
     while (!BitSetIteratorDone(&it)) {
       num_regs++;
       BitSetIteratorNext(&it);
     }
    result |= num_regs;
    result <<= shifts[i];
  }
  return result;
}

const char* W65C02RegisterAllocatorPrintRegMask(uint32_t mask, char* buf) {
  static int shifts[] = {5, 4, 4, 3, 3};
  static int masks[] = {31,15,15,7,7};
  static const char* reg_types[] = {
    "i",
    "b",
    "l",
    "x",
    "f",
  };
  const char* r = buf;
  for (int i = 0; i < 5; i++) {
    int n = mask & masks[i];
    buf += snprintf(buf, 6, "%s:%d ", reg_types[i], n);
    mask >>= shifts[i];
  }
  return r;
}
