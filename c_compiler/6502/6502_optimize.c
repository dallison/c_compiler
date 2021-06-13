//
//  6502_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 5/11/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "6502_optimize.h"
#include "6502_codegen.h"
#include "target_basic_block.h"
#include <assert.h>

typedef enum {
  kRegUnknown,
  kRegConstant,
  kRegExpression,
} RegValueType;

typedef enum {
  kRegA,
  kRegX,
  kRegY,
} RegName;

// Tracks the value of a 6502 register.
typedef struct {
  RegValueType type;
  RegName reg;
  struct {
    int c;      // Constant value or offset for expression.
    TargetInstruction* expr;    // Expression value;
  } value;
} RegTracker;

struct OptimizerData {
  _6502Generator* g;
  bool modified;
  RegTracker A;
  RegTracker X;
  RegTracker Y;
};

static AddressingMode GetAddrMode(TargetInstruction* inst) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  assert(mode > kAddrModeUnknown && mode < kAddrModeInvalid);
  return mode;
}

static void SetAddrMode(TargetInstruction* inst, AddressingMode mode) {
  inst->flags |= (int)mode << 16;
}

static int ImmediateValue(TargetInstruction* inst) {
  assert(GetAddrMode(inst) == kAddrModeImmediate);
  int value = (int)TargetIntValue(inst->operand[0]);
  if (inst->operand[1] == NULL) {
    return value;
  }
  int byte = (int)TargetIntValue(inst->operand[1]);
  return (value >> (byte * 8) & 0xff);
}

static void TrackReg(RegTracker* tracker, TargetInstruction* inst) {
  AddressingMode mode = GetAddrMode(inst);
  switch (mode) {
    case kAddrModeImmediate:
      tracker->type = kRegConstant;
      tracker->value.c = ImmediateValue(inst);
      break;
    case kAddrModeZeroPage:
      tracker->type = kRegExpression;
      tracker->value.expr = inst->operand[0];
      tracker->value.c = (int)TargetIntValue(inst->operand[1]);
      break;
    default:
      tracker->type = kRegUnknown;
      break;
  }
}

static void IncrementReg(RegName reg, TargetInstruction* inst) {
  switch (reg) {
    case kRegA:
      inst->opcode = (TargetOpcode)_6502_OP(inc);
      SetAddrMode(inst, kAddrModeAccumulator);
      TargetReplaceOperand(inst, 0, NULL);
      break;
    case kRegX:
      inst->opcode = (TargetOpcode)_6502_OP(inx);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeImplied);
      break;
    case kRegY:
      inst->opcode = (TargetOpcode)_6502_OP(iny);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeImplied);
      break;
  }
}

static void DecrementReg(RegName reg, TargetInstruction* inst) {
  switch (reg) {
    case kRegA:
      inst->opcode = (TargetOpcode)_6502_OP(dec);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeAccumulator);
      break;
    case kRegX:
      inst->opcode = (TargetOpcode)_6502_OP(dex);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeImplied);
      break;
    case kRegY:
      inst->opcode = (TargetOpcode)_6502_OP(dey);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeImplied);
      break;
  }
}

static bool OptimizeInstruction(struct OptimizerData* opt_data,
                                          RegTracker* old,
                                           RegTracker* curr,
                                           TargetBasicBlock* block,
                                           TargetInstruction* inst) {
  if (curr->type == old->type) {
    switch (curr->type) {
      case kRegUnknown:
        break;
      case kRegConstant:
        if (curr->value.c == old->value.c) {
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
          opt_data->modified = true;
          return true;
        } else if (curr->value.c == old->value.c + 1) {
          // Increment.
          IncrementReg(curr->reg, inst);
          return true;
        } else if (curr->value.c == old->value.c - 1) {
          // Decrement.
          DecrementReg(curr->reg, inst);
          return true;
        }
        break;
      case kRegExpression:
        if (curr->value.expr == old->value.expr && curr->value.c == old->value.c) {
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
          opt_data->modified = true;
          return true;
        }
        break;
    }
  }
  return false;
}

static _6502Opcode TransferInstruction(RegName from, RegName to) {
  switch (from) {
    case kRegA:
      switch (to) {
        case kRegA:
          break;
        case kRegX:
          return _6502_OP(tax);
        case kRegY:
          return _6502_OP(tay);
      }
      break;

    case kRegX:
      switch (to) {
        case kRegA:
          return _6502_OP(txa);
        case kRegX:
          break;
        case kRegY:
          break;
      }
      break;
    case kRegY:
      switch (to) {
        case kRegA:
          return _6502_OP(tya);
        case kRegX:
          break;
        case kRegY:
          break;
      }
      break;
  }
  return _6502_OP(nop);
}

static bool OptimizeTransfer( struct OptimizerData* opt_data,
                                          RegTracker* old,
                                           RegTracker* curr,
                                           TargetBasicBlock* block,
                                           TargetInstruction* inst) {
  if (curr->type == old->type) {
    switch (curr->type) {
      case kRegUnknown:
        break;
      case kRegConstant:
        if (curr->value.c == old->value.c) {
          _6502Opcode op = TransferInstruction(old->reg, curr->reg);
          if (op != _6502_OP(nop)) {
            inst->opcode = (TargetOpcode)op;
            TargetReplaceOperand(inst, 0, NULL);
            opt_data->modified = true;
            return true;
          }
        }
        break;
      case kRegExpression:
        if (curr->value.expr == old->value.expr && curr->value.c == old->value.c) {
          _6502Opcode op = TransferInstruction(old->reg, curr->reg);
           if (op != _6502_OP(nop)) {
             inst->opcode = (TargetOpcode)op;
             TargetReplaceOperand(inst, 0, NULL);
             TargetReplaceOperand(inst, 1, NULL);
             opt_data->modified = true;
             return true;
           }
        }
        break;
    }
  }
  return false;
}

static int GetConstantByte(TargetInstruction* v, TargetInstruction* byte_num) {
  if (byte_num == NULL) {
    return (int)TargetIntValue(v);
  }
  return ((int)TargetIntValue(v) >> (int)TargetIntValue(byte_num) * 8) & 0xff;
}

static void OptimizeBlock(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  opt_data->A.type = kRegUnknown;
  opt_data->A.reg = kRegA;
  opt_data->X.type = kRegUnknown;
  opt_data->X.reg = kRegX;
  opt_data->Y.type = kRegUnknown;
  opt_data->Y.reg = kRegY;

  RegTracker current_A = {kRegUnknown, kRegA};
  RegTracker current_X = {kRegUnknown, kRegX};
  RegTracker current_Y = {kRegUnknown, kRegY};
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = block->code; inst != NULL &&
       block->end_code != NULL &&
       TargetPrev(inst) != block->end_code; inst = next) {
    next = block->end_code == NULL ? NULL : TargetNext(inst);

    switch ((_6502Opcode)inst->opcode) {
      case _6502_OP(lda): {
        TargetInstruction* prev = TargetPrev(inst);
        if (prev->opcode == (TargetOpcode)_6502_OP(lda)) {
          // LDA following an LDA, remove previous.
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, prev);
          opt_data->modified = true;
        } else if (prev->opcode == (TargetOpcode)_6502_OP(sta)) {
          // STA followed by LDA, remove LDA.
          if (prev->operand[0] == inst->operand[0] && prev->operand[1] == inst->operand[1] &&
              GetAddrMode(prev) == GetAddrMode(inst)) {
            TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
            opt_data->modified = true;
            break;
          }
        }
        TrackReg(&current_A, inst);
        OptimizeInstruction(opt_data, &opt_data->A, &current_A, block, inst) ||
          OptimizeTransfer(opt_data, &opt_data->Y, &current_A, block, inst) ||
                     OptimizeTransfer(opt_data, &opt_data->X, &current_A, block, inst);
        opt_data->A = current_A;
        break;
      }
      case _6502_OP(sta): {
        AddressingMode mode = GetAddrMode(inst);
        // If the expression being stored to has only one use and this
        // instruction is it, there's no need to actually store it.
        if (mode == kAddrModeZeroPage) {
          TargetInstruction* dest = inst->operand[0];
          if (dest->uses == 1 && dest->users.value.p[0] == inst) {
            // Only one use (this inst).  Remove instruction.
            TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
            opt_data->modified = true;
            break;
          }
        }
        if (opt_data->A.type == kRegConstant && opt_data->A.value.c == 0) {
          // Storing value 0 can be converted to STZ as long as the
          // addressing mode is OK.
          if (mode != kAddrModeIndirectIndexed && mode != kAddrModeIndirect) {
            inst->opcode = (TargetOpcode)_6502_OP(stz);
            TargetInstruction* prev = TargetPrev(inst);
            while (prev->opcode == (TargetOpcode)_6502_OP(lda)) {
               // STA following an LDA, remove previous.
               TargetInstruction* p = TargetPrev(prev);
               TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, prev);
               opt_data->modified = true;
               prev = p;
             }
          }
        }
        break;
      }
      case _6502_OP(ldx):
        TrackReg(&current_X, inst);
        OptimizeInstruction(opt_data, &opt_data->X, &current_X, block, inst) ||
          OptimizeTransfer(opt_data, &opt_data->A, &current_X, block, inst);
        opt_data->X = current_X;
        break;
      case _6502_OP(stx):
        TrackReg(&current_X, inst);
        opt_data->X = current_X;
        break;
      case _6502_OP(inx):
        if (opt_data->X.type == kRegConstant) {
          opt_data->X.value.c++;
        }
        break;
      case _6502_OP(dex):
        if (opt_data->X.type == kRegConstant) {
          opt_data->X.value.c-- ;
        }
        break;
      case _6502_OP(ldy):
        TrackReg(&current_Y, inst);
        OptimizeInstruction(opt_data, &opt_data->Y, &current_Y, block, inst) ||
          OptimizeTransfer(opt_data, &opt_data->A, &current_Y, block, inst);
        opt_data->Y = current_Y;
        break;
      case _6502_OP(sty):
        TrackReg(&current_Y, inst);
        opt_data->Y = current_Y;
        break;
      case _6502_OP(iny):
        if (opt_data->Y.type == kRegConstant) {
          opt_data->Y.value.c++;
        }
        break;
      case _6502_OP(dey):
        if (opt_data->Y.type == kRegConstant) {
          opt_data->Y.value.c-- ;
        }
        break;
        
      case _6502_OP(jsr):
        opt_data->A.type = kRegUnknown;
        opt_data->X.type = kRegUnknown;
        opt_data->Y.type = kRegUnknown;
        break;
        
      case _6502_OP(adc):
      case _6502_OP(sbc):
        // These modify the accumulator.
         opt_data->A.type = kRegUnknown;
         break;
      case _6502_OP(ora):
        opt_data->A.type = kRegUnknown;
        if (TargetIsConst(inst->operand[0])) {
          int v = GetConstantByte(inst->operand[0], inst->operand[1]);
          if (v == 255) {
            // ORA #255 sets A to 255.
            opt_data->A.type = kRegConstant;
            opt_data->A.value.c = 255;
            break;
          }
          if (opt_data->A.type == kRegConstant) {
            opt_data->A.value.c |= v;
          }
        }
        break;
      case _6502_OP(eor):
        opt_data->A.type = kRegUnknown;
        if (TargetIsConst(inst->operand[0])) {
          int v = GetConstantByte(inst->operand[0], inst->operand[1]);
          if (opt_data->A.type == kRegConstant) {
            opt_data->A.value.c ^= v;
          }
        }
        break;
      case _6502_OP(and):
        opt_data->A.type = kRegUnknown;
        if (TargetIsConst(inst->operand[0])) {
         int v = GetConstantByte(inst->operand[0], inst->operand[1]);
         if (v == 255) {
           // AND #255 keeps A the same.
           TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
           break;
         }
          if (v == 0) {
            // AND #0 sets A to zero.
            inst->opcode = (TargetOpcode)_6502_OP(lda);
            opt_data->A.type = kRegConstant;
            opt_data->A.value.c = 0;
            break;
          }
         if (opt_data->A.type == kRegConstant) {
           opt_data->A.value.c &= v;
         }
       }
        break;
        
        case _6502_OP(asl):
        case _6502_OP(lsr):
        case _6502_OP(rol):
        case _6502_OP(ror):
        // These might modify the accumulator:
        if (GetAddrMode(inst) == kAddrModeAccumulator) {
          opt_data->A.type = kRegUnknown;
        }
        break;
        
      case _6502_OP(expr1):
      case _6502_OP(expr2):
      case _6502_OP(expr4):
      case _6502_OP(expr8):
      case _6502_OP(clc):
      case _6502_OP(sec):
        break;
        
      default:
        // Not an instruction we can track, all values are unknown.
        opt_data->A.type = kRegUnknown;
        opt_data->X.type = kRegUnknown;
        opt_data->Y.type = kRegUnknown;
        break;
    }
    inst = next;
  }
}

void _6502Optimize(_6502Generator* g) {
  for (;;) {
    struct OptimizerData data = {g, false};
    TargetTraverseDominatorTree(&g->base, OptimizeBlock, kTraversePreOrder, &data);
    if (!data.modified) {
      break;
    }
  }
}

