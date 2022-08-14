//
//  arm_codegen.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "arm_codegen.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "debug.h"

#include "target_basic_block.h"

const char* ARMOpcodeName(int op) {
  return NULL;
}

bool ARMIsExpression(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(label):
    case ARM_OP(asm):
    case ARM_OP(jal):
    case ARM_OP(jalr):
    case ARM_OP(beq):
    case ARM_OP(bne):
    case ARM_OP(blt):
    case ARM_OP(bge):
    case ARM_OP(bltu):
    case ARM_OP(bgeu):
    case ARM_OP(beqz):
    case ARM_OP(bnez):
    case ARM_OP(j):
    case ARM_OP(jr):
    case ARM_OP(call):
    case ARM_OP(rcall):
    case ARM_OP(callf):
    case ARM_OP(rcallf):
    case ARM_OP(ret):
    case ARM_OP(save):
    case ARM_OP(restore):
    case ARM_OP(rmov):
    case ARM_OP(rmovf):
    case ARM_OP(rmovd):
    case ARM_OP(sb):
    case ARM_OP(sw):
    case ARM_OP(sh):
    case ARM_OP(sd):
    case ARM_OP(fsw):
    case ARM_OP(fsd):
    case ARM_OP(loc):
    case ARM_OP(named_label):
    case ARM_OP(regarg):
    case ARM_OP(nrvoval):
    case ARM_OP(symbol):
    case ARM_OP(spill):
      return false;
    default:
      return !ARMIsFixedRegister(inst) && !ARMIsConst(inst);
  }
}

bool ARMGeneratesOutput(TargetInstruction* inst) {
  if (ARMIsFixedRegister(inst)) {
    return false;
  }
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(ivarreg):
    case ARM_OP(fvarreg):
    case ARM_OP(resulti):
    case ARM_OP(resultf):
    case ARM_OP(resultd):
      return false;
    default:
      return ARMIsExpression(inst) && !ARMIsSymbol(inst) && !ARMIsConst(inst);
  }
}

bool ARMIsLoad(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(lb):
    case ARM_OP(lw):
    case ARM_OP(ld):
    case ARM_OP(lbu):
    case ARM_OP(lwu):
    case ARM_OP(lh):
    case ARM_OP(lhu):
    case ARM_OP(flw):
    case ARM_OP(fld):
      return true;
    default:
      return false;
  }
}

bool ARMIsSignedLoad(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(lb):
    case ARM_OP(lw):
    case ARM_OP(lh):
    case ARM_OP(ld):
      return true;
    default:
      return false;
  }
}

bool ARMIsStore(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(sb):
    case ARM_OP(sw):
    case ARM_OP(sd):
    case ARM_OP(lh):
    case ARM_OP(fsw):
    case ARM_OP(fsd):
      return true;
    default:
      return false;
  }
}

bool ARMIsIntConst(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(const32):
    case ARM_OP(const8):
    case ARM_OP(const16):
    case ARM_OP(const64):
      return true;
    default:
      return false;
  }
}

bool ARMIsConst(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(const32):
    case ARM_OP(const8):
    case ARM_OP(const16):
    case ARM_OP(const64):
    case ARM_OP(constf):
    case ARM_OP(constd):
      return true;
    default:
      return false;
  }
}

bool ARMIsFloatingPoint(TargetInstruction* inst) {
  if ((ARMOpcode)inst->opcode < ARM_OP(flw) || (ARMOpcode)inst->opcode > ARM_OP(fmv_d_x)) {
    return false;
  }
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(fa0):
    case ARM_OP(fa1):
    case ARM_OP(fa2):
    case ARM_OP(fa3):
    case ARM_OP(fa4):
    case ARM_OP(fa5):
    case ARM_OP(fa6):
    case ARM_OP(fa7):
    case ARM_OP(fvarreg):
    case ARM_OP(constf):
    case ARM_OP(constd):
    case ARM_OP(fmv_s):
    case ARM_OP(fmv_d):
    case ARM_OP(rmovf):
    case ARM_OP(rmovd):
      return true;
    default:
      return false;
  }
}

bool ARMIsSymbol(TargetInstruction* inst) {
  return (ARMOpcode)inst->opcode == ARM_OP(symbol);
}

bool ARMIsCall(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(call):
    case ARM_OP(rcall):
    case ARM_OP(callf):
    case ARM_OP(rcallf):
       return true;
    default:
      return false;
  }
}

bool ARMIsArgRegister(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(a0):
    case ARM_OP(a1):
    case ARM_OP(a2):
    case ARM_OP(a3):
    case ARM_OP(a4):
    case ARM_OP(a5):
    case ARM_OP(a6):
    case ARM_OP(a7):
    case ARM_OP(fa0):
    case ARM_OP(fa1):
    case ARM_OP(fa2):
    case ARM_OP(fa3):
    case ARM_OP(fa4):
    case ARM_OP(fa5):
    case ARM_OP(fa6):
    case ARM_OP(fa7):
      return true;
    default:
      return false;
  }
}
 
bool ARMIsVarRegister(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
  case ARM_OP(ivarreg):
  case ARM_OP(fvarreg):
      return true;
  default:
    return false;
  }
}

bool ARMIsFixedRegister(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(a0):
    case ARM_OP(a1):
    case ARM_OP(a2):
    case ARM_OP(a3):
    case ARM_OP(a4):
    case ARM_OP(a5):
    case ARM_OP(a6):
    case ARM_OP(a7):
    case ARM_OP(fa0):
    case ARM_OP(fa1):
    case ARM_OP(fa2):
    case ARM_OP(fa3):
    case ARM_OP(fa4):
    case ARM_OP(fa5):
    case ARM_OP(fa6):
    case ARM_OP(fa7):
    case ARM_OP(x0):
    case ARM_OP(t0):
    case ARM_OP(fp):

      // Calls always return in a0 or fa0.
    case ARM_OP(call):
    case ARM_OP(rcall):
    case ARM_OP(callf):
    case ARM_OP(rcallf):
      return true;
    default:
      return false;
  }
}

bool ARMIsResult(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
     case ARM_OP(resulti):
      case ARM_OP(resultf):
      case ARM_OP(resultd):
      return true;
    default:
      return false;
  }
}

bool ARMIsBranch(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(j):
    case ARM_OP(jr):
    case ARM_OP(jal):
    case ARM_OP(jalr):
    case ARM_OP(beq):
    case ARM_OP(bne):
    case ARM_OP(blt):
    case ARM_OP(bge):
    case ARM_OP(bltu):
    case ARM_OP(bgeu):
    case  ARM_OP(beqz):
    case  ARM_OP(bnez):
      return true;
    default:
      return false;
  }
}

bool ARMIsConditionalBranch(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(beq):
    case ARM_OP(bne):
    case ARM_OP(blt):
    case ARM_OP(bge):
    case ARM_OP(bltu):
    case ARM_OP(bgeu):
    case  ARM_OP(beqz):
    case  ARM_OP(bnez):
      return true;
    default:
      return false;
  }
}

bool ARMIsSpill(TargetInstruction* inst) {
  return (ARMOpcode)inst->opcode == ARM_OP(spill);
}

bool ARMIsLabel(TargetInstruction* inst) {
  return (ARMOpcode)inst->opcode == ARM_OP(label);
}

bool ARMIsReturn(TargetInstruction* inst) {
  return (ARMOpcode)inst->opcode == ARM_OP(ret);
}

int ARMIntValue(TargetInstruction* inst) {
  if (inst->opcode == (TargetOpcode)ARM_OP(x0)) {
    return 0;
  }
  return (int)((TargetConstant*)inst)->value.ivalue;
}

// Is the value small enough to be encoded in an immediate field?
bool ARMIsPossibleImmediate(int64_t value) {
  // Check for 12 bit signed immediate.
  if (value < 0) {
    return value >= -2048;
  }
  return value < 2048;
}

TargetInstruction* ARMGetBranchTarget(TargetInstruction* inst) {
  ARMOpcode opcode = (ARMOpcode)inst->opcode;
  if (opcode == ARM_OP(bnez) || opcode == ARM_OP(beqz)) {
     return inst->operand[1];
   } else {
     return inst->operand[2];    // Operand 2.
   }
}

bool ARMIsJumpableEntry(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(j);
}

static TargetVirtuals virtuals = {
  .opcode_name = ARMOpcodeName,
  .is_branch = ARMIsBranch,
  .is_call = ARMIsCall,
  .is_return = ARMIsReturn,
  .is_spill = ARMIsSpill,
  .is_label = ARMIsLabel,
  .is_floating_point = ARMIsFloatingPoint,
  .is_conditional_branch = ARMIsConditionalBranch,
  .is_fixed_register = ARMIsFixedRegister,
  .is_const = ARMIsConst,
  .is_symbol = ARMIsSymbol,
  .is_expression = ARMIsExpression,
  .is_table_entry = ARMIsJumpableEntry,
  .get_branch_target = ARMGetBranchTarget,
};

void ARMGeneratorInit(ARMGenerator* arm, Generator* gen) {
  TargetGeneratorInit(&arm->base, gen, &virtuals);

  arm->num_int_arg_regs = 0;
  arm->num_fp_arg_regs = 0;
  arm->num_int_reg_vars = 0;
  arm->num_fp_reg_vars = 0;
  arm->struct_return_reg = -1;
  arm->not_leaf = false;
  memset(arm->int_argument_registers, 0, sizeof(arm->int_argument_registers));
  memset(arm->fp_argument_registers, 0, sizeof(arm->fp_argument_registers));
  VectorInit(&arm->var_regs);
  VectorInit(&arm->saved_regs);
  VectorInit(&arm->offsets);

  ARMRegisterAllocatorInit(&arm->register_allocator, arm);
}

ARMGenerator* NewARMGenerator(Generator* gen) {
  ARMGenerator* arm = malloc(sizeof(ARMGenerator));
  ARMGeneratorInit(arm, gen);
  return arm;
}

void ARMGeneratorDestruct(ARMGenerator* arm) {
  TargetGeneratorDestruct(&arm->base);
  VectorDestructWithContents(&arm->var_regs, NULL);
  VectorDestructWithContents(&arm->saved_regs, NULL);
  VectorDestructWithContents(&arm->offsets, NULL);
  ARMRegisterAllocatorDestruct(&arm->register_allocator);
}

void ARMGeneratorDelete(ARMGenerator* arm) {
  ARMGeneratorDestruct(arm);
  free(arm);
}

// Lower the IR to ARM.
void ARMLower(ARMGenerator* arm, Generator* gen) {
  
}

void ARMPrint(ARMGenerator* arm, FILE* fp) {
  
}
