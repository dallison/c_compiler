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

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  
  allocator->spilled_region_size = 0;
}

void RVRegisterAllocatorReserveRegisters(RVRegisterAllocator* allocator) {
  RVGenerator* rv = allocator->rv;

  // Reserve and mark register variables.
  bool is_leaf = allocator->rv->base.num_calls == 0 &&
      !rv->use_reg_vars;
  int first_int_reg_var =
      is_leaf ? RV_FIRST_LEAF_INT_REG_VAR : RV_FIRST_INT_REG_VAR;
  int first_fp_reg_var =
      is_leaf ? RV_FIRST_LEAF_FP_REG_VAR : RV_FIRST_FP_REG_VAR;

  for (int i = 0; i < rv->num_int_reg_vars; i++) {
    allocator->int_regs[first_int_reg_var + i].base.reserved = true;
    if (!is_leaf) {
      BitSetInsert(&allocator->used_int_regs, first_int_reg_var + i);
    }
  }

  for (int i = 0; i < rv->num_fp_reg_vars; i++) {
    allocator->int_regs[first_fp_reg_var + i].base.reserved = true;
    if (!is_leaf) {
      BitSetInsert(&allocator->used_float_regs, first_fp_reg_var + i);
    }
  }
}

RVRegisterAllocator* NewRVRegisterAllocator(RVGenerator* pcode) {
  RVRegisterAllocator* reg_alloc = malloc(sizeof(RVRegisterAllocator));
  RVRegisterAllocatorInit(reg_alloc, pcode);
  return reg_alloc;
}

void RVRegisterAllocatorDestruct(RVRegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
}

void RVRegisterAllocatorDelete(RVRegisterAllocator* alloc) {
  RVRegisterAllocatorDestruct(alloc);
  free(alloc);
}

// Find a register by searching for a free one in a set of non-overlapping
// ranges.
static struct {
  RVRegisterType type;  // Register type.
  int start;            // Start of range.
  int end;              // End of range.
  const char* prefix;   // Register name prefix
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

static RVRegister* FindSpillVictim(RVRegisterAllocator* allocator,
                                   RVRegisterType type) {
  RVRegister* regs =
      type == kRVRegTypeInt ? allocator->int_regs : allocator->float_regs;
  RVRegister* min_refs_reg = NULL;
  // Find the register with the minimum number of references
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type) {
      for (int j = register_ranges[i].start; j <= register_ranges[i].end; j++) {
        if (!regs[j].base.reserved && regs[j].base.owner != NULL) {
          TargetInstruction* owner = regs[j].base.owner;
          if (owner->opcode == RV_OP(rmov)) {
            // Not rmov instruction.
            continue;
          }
          if ((owner->flags & TARGET_INST_SPILLED) != 0) {
            // Not already spilled.
            continue;
          }
          if (min_refs_reg == NULL || owner->uses < min_refs_reg->base.owner->uses) {
            min_refs_reg = &regs[j];
          }
        }
      }
    }
  }
  assert(min_refs_reg != NULL);
  return min_refs_reg;
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
            // Increment call count so we can't treat this as a leaf proc.
            allocator->rv->base.num_calls++;
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

static void FreeRegisters(RVRegisterAllocator* allocator,
                          TargetInstruction* inst) {
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetInstruction* op = inst->operand[i];
      TargetRegister* reg = op->reg;
      if (reg != NULL && !reg->reserved && op->uses > 0) {
        op->uses--;
        if (op->uses == 0) {
          FreeRegister(allocator, (RVRegister*)reg);
        }
      }
    }
  }
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

static void SpillRegister(RVRegisterAllocator* allocator, RVRegister* reg) {
  TargetInstruction* inst = reg->base.owner;
  assert(inst != NULL);
  inst->flags |= TARGET_INST_SPILLED;
  reg->base.owner = NULL;
  allocator->spilled_region_size += 8;    // Register size.
}

static RVRegister* AllocateRegisterWithType(RVRegisterAllocator* allocator,
                                            RVRegisterType type,
                                            bool can_use_temp) {
  RVRegister* reg = FindFreeRegister(allocator, type, can_use_temp);

  if (reg == NULL) {
    reg = FindSpillVictim(allocator, type);
    SpillRegister(allocator, reg);
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
    case RV_OP(fv0):
    case RV_OP(fv1):
    case RV_OP(fv2):
    case RV_OP(fv3):
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
static bool CanUseTemp(TargetInstruction* start_inst) {
  TargetInstruction* inst = start_inst;
  int num_refs = inst->refs;
  if (num_refs == 0) {
    return true;
  }

  // Look for all reference to this instruction by traversing
  // the instructions forward.  If we encounter a call instruction
  // while looking for the references we know that we can't use a temp
  // register (call will not preserve them).
  TargetInstruction* this = inst;
  inst = TargetNext(inst);
  while (inst != NULL && num_refs > 0) {
    // If this is a call instruction we can't use a temp because the use of the
    // register spans the call.
    switch ((RVOpcode)inst->opcode) {
      case RV_OP(call):
      case RV_OP(callf):
      case RV_OP(rcall):
      case RV_OP(rcallf):
        return false;
      default:
        break;
    }

    // Look for a reference to the instruction in the operands.
    // If we find one, decrement the number of references we expect
    // to see and if that reaches zero we know there are no more
    // references and we can use a temp register.
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      if (inst->operand[i] == this) {
        num_refs--;
        if (num_refs == 0) {
          return true;
        }
      }
    }
    inst = TargetNext(inst);
  }

  // If we get here then the num_refs is still >0 and we've reached the end of
  // the code.
  assert(false);
}

// We want to change the register assigned to an instruction.  This function
// makes sure it's safe to do so.  This is used by the rmov instruction
// to remove unnecessary mv instructions.  It's only safe to reassign
// the register if:
// 1. The instruction being replaced isn't a fixed register (like an argument)
// 2. The number of references to the instruction is 1
// 3. If we are going to replace the register with a non-saved register then
//    it must be OK to use a temp register for the instruction.
static bool CanReassignRegister(TargetInstruction* src, RVRegister* reg) {
  if (RVIsFixedRegister((RVOpcode)src->opcode) || src->refs != 1) {
    return false;
  }
  // If the register we want to use (reg) is temporary make sure that the
  // src instruction can use a temp reg.
  if (!IsSavedReg(reg)) {
    if (!CanUseTemp(src)) {
      return false;
    }
  }
  return true;
}

static void AllocateForRmov(RVRegisterAllocator* allocator,
                            TargetInstruction* inst) {
  TargetInstruction* dest = inst->operand[0];
  TargetInstruction* src = inst->operand[1];
  // If the source is spilled then so is the target.
  if ((src->flags & TARGET_INST_SPILLED) != 0) {
    if (src->uses == 1) {
      dest->flags |= TARGET_INST_SPILLED;
    } else {
      // There is more than one use of the source.  This means we can't
      // propagate the spill to the destination since the source will
      // be used after this point.
    }
  }
  RVRegister* reg = (RVRegister*)dest->reg;
  if (CanReassignRegister(src, reg)) {
    // Safe to reassign register, merge the register into the source
    // and eliminate the rmov.
    FreeRegisters(allocator, inst);
    src->reg = &reg->base;
    src->uses++;
    reg->base.owner = src;
    inst->reg = src->reg;
    return;
  }
  inst->operand[0]->uses++;  // Prevent this from being freed.
  FreeRegisters(allocator, inst);
  inst->uses = inst->refs;
  inst->reg = &reg->base;
  reg->base.owner = inst;
}

static void AllocateRegister(RVRegisterAllocator* allocator,
                             TargetInstruction* inst) {

  bool is_leaf = allocator->rv->base.num_calls == 0 &&
      !allocator->rv->use_reg_vars;

  // rmov instructions use the register allocated to their first
  // operand as their own register.
  if (inst->opcode == RV_OP(rmov) || inst->opcode == RV_OP(rmovf) ||
      inst->opcode == RV_OP(rmovd)) {
    AllocateForRmov(allocator, inst);
    return;
  }

  // Free up any registers we can.
  FreeRegisters(allocator, inst);

  RVRegister* reg;
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(constb):
    case RV_OP(consth):
    case RV_OP(constw):
    case RV_OP(constx):
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

    case RV_OP(regarg): {
      // A register arg points to the argument register instruction
      // in its second operand.  The register allocated to this needs
      // to be freed.
      TargetInstruction* arg_reg = inst->operand[1];
      TargetRegister* reg = arg_reg->reg;
      assert(reg != NULL);
      TargetInstruction* owner = reg->owner;
      if (owner != NULL) {
        owner->uses--;
      }
      reg->owner = NULL;
      return;
    }
      
    case RV_OP(x0):
      reg = &allocator->int_regs[RV_INT_ZERO_REG];
      break;

    case RV_OP(fp):
      reg = &allocator->int_regs[RV_FP_REG];
      break;

    case RV_OP(sp):
      reg = &allocator->int_regs[RV_SP_REG];
      break;

    case RV_OP(a0):
    case RV_OP(a1):
    case RV_OP(a2):
    case RV_OP(a3):
    case RV_OP(a4):
    case RV_OP(a5):
    case RV_OP(a6):
    case RV_OP(a7):
      reg = &allocator
                 ->int_regs[(int)inst->opcode - RV_OP(a0) + RV_INT_ARG_START];
      break;

    case RV_OP(v0):
    case RV_OP(v1):
    case RV_OP(v2):
    case RV_OP(v3):
    case RV_OP(v4):
    case RV_OP(v5):
    case RV_OP(v6):
    case RV_OP(v7):
    case RV_OP(v8):
    case RV_OP(v9):
      reg = &allocator->int_regs[(int)inst->opcode - RV_OP(v0) +
                                 (is_leaf ? RV_FIRST_LEAF_INT_REG_VAR
                                          : RV_FIRST_INT_REG_VAR)];
      break;

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

    case RV_OP(fv0):
    case RV_OP(fv1):
    case RV_OP(fv2):
    case RV_OP(fv3):
    case RV_OP(fv4):
    case RV_OP(fv5):
    case RV_OP(fv6):
    case RV_OP(fv7):
    case RV_OP(fv8):
    case RV_OP(fv9):
      reg = &allocator->float_regs[(int)inst->opcode - RV_OP(fv0) +
                                   (is_leaf ? RV_FIRST_LEAF_FP_REG_VAR
                                            : RV_FIRST_FP_REG_VAR)];
      break;

    case RV_OP(structreturn):
      reg = &allocator->int_regs[(is_leaf ? RV_FIRST_LEAF_INT_REG_VAR
                                          : RV_FIRST_INT_REG_VAR) +
                                 allocator->rv->struct_return_reg];
      break;

    case RV_OP(resultx):
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
      reg = AllocateRegisterWithType(allocator, kRVRegTypeInt, CanUseTemp(inst));
      break;

    default: {
      RVRegisterType reg_type = RegisterTypeFromInstruction(inst);
      reg = AllocateRegisterWithType(allocator, reg_type, CanUseTemp(inst));
    }
  }

  inst->uses = inst->refs;
  inst->reg = &reg->base;
  reg->base.owner = inst;

  // If nobody is using this register free it up immediately.
  // TODO: argument registers are not used explicitly but can't be freed here.
  if (inst->uses == 0 && !reg->base.reserved) {
    FreeRegister(allocator, reg);
  }
}

void RVAllocateRegisters(RVRegisterAllocator* allocator) {
  RVRegisterAllocatorReserveRegisters(allocator);

  TargetInstruction* inst = TargetFirstInstruction(&allocator->rv->base);
  while (inst != NULL) {
    AllocateRegister(allocator, inst);
    inst = TargetNext(inst);
  }
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
