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

typedef struct {
  RegTracker A;
  RegTracker X;
  RegTracker Y;
} RegTrackers;

struct OptimizerData {
  W65C02Generator* g;
  bool modified;
};

static AddressingMode GetAddrMode(TargetInstruction* inst) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  assert(mode > kAddrModeUnknown && mode < kAddrModeInvalid);
  return mode;
}

static void SetAddrMode(TargetInstruction* inst, AddressingMode mode) {
  // Clear top bits of flags.
  inst->flags &= 0xffff;
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

// This only works for 8-bit values (like an LDY instruction).
static void SetImmediateValue(struct OptimizerData* opt_data, TargetInstruction* inst, int value) {
  assert(GetAddrMode(inst) == kAddrModeImmediate);
  assert(inst->operand[1] == NULL);
  TargetInstruction* new_value = TargetGetIntConstant(&opt_data->g->base, NULL, kTargetType8Bit, value);
  inst->operand[0] = new_value;
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
      inst->opcode = (TargetOpcode)W65C02_OP(inc);
      SetAddrMode(inst, kAddrModeAccumulator);
      TargetReplaceOperand(inst, 0, NULL);
      break;
    case kRegX:
      inst->opcode = (TargetOpcode)W65C02_OP(inx);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeImplied);
      break;
    case kRegY:
      inst->opcode = (TargetOpcode)W65C02_OP(iny);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeImplied);
      break;
  }
}

static void DecrementReg(RegName reg, TargetInstruction* inst) {
  switch (reg) {
    case kRegA:
      inst->opcode = (TargetOpcode)W65C02_OP(dec);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeAccumulator);
      break;
    case kRegX:
      inst->opcode = (TargetOpcode)W65C02_OP(dex);
      TargetReplaceOperand(inst, 0, NULL);
      SetAddrMode(inst, kAddrModeImplied);
      break;
    case kRegY:
      inst->opcode = (TargetOpcode)W65C02_OP(dey);
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

static W65C02Opcode TransferInstruction(RegName from, RegName to) {
  switch (from) {
    case kRegA:
      switch (to) {
        case kRegA:
          break;
        case kRegX:
          return W65C02_OP(tax);
        case kRegY:
          return W65C02_OP(tay);
      }
      break;

    case kRegX:
      switch (to) {
        case kRegA:
          return W65C02_OP(txa);
        case kRegX:
          break;
        case kRegY:
          break;
      }
      break;
    case kRegY:
      switch (to) {
        case kRegA:
          return W65C02_OP(tya);
        case kRegX:
          break;
        case kRegY:
          break;
      }
      break;
  }
  return W65C02_OP(nop);
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
          W65C02Opcode op = TransferInstruction(old->reg, curr->reg);
          if (op != W65C02_OP(nop)) {
            inst->opcode = (TargetOpcode)op;
            TargetReplaceOperand(inst, 0, NULL);
            opt_data->modified = true;
            return true;
          }
        }
        break;
      case kRegExpression:
        if (curr->value.expr == old->value.expr && curr->value.c == old->value.c) {
          W65C02Opcode op = TransferInstruction(old->reg, curr->reg);
           if (op != W65C02_OP(nop)) {
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
  return (TargetIntValue(v) >> TargetIntValue(byte_num) * 8) & 0xff;
}

static RegTrackers* NewTrackers() {
  RegTrackers* t = malloc(sizeof(RegTrackers));
  t->A.type = kRegUnknown;
  t->A.reg = kRegA;
  t->X.type = kRegUnknown;
  t->X.reg = kRegX;
  t->Y.type = kRegUnknown;
  t->Y.reg = kRegY;
  return t;
}

static TargetInstruction* PrevInstruction(TargetInstruction* inst) {
  TargetInstruction* prev = inst;
  do {
    prev = TargetPrev(prev);
  } while (prev != NULL && !(((int)prev->opcode != (int)W65C02_OP(reloadpoint)) &&
           ((int)prev->opcode != (int)W65C02_OP(expr1)) &&
           ((int)prev->opcode != (int)W65C02_OP(expr2)) &&
           ((int)prev->opcode != (int)W65C02_OP(expr4)) &&
           ((int)prev->opcode != (int)W65C02_OP(expr8))));
  return prev;
}

// Does the given instruction change the value of A?
static bool ModifiesA(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(lda):
    case W65C02_OP(adc):
    case W65C02_OP(sbc):
    case W65C02_OP(ora):
    case W65C02_OP(and):
    case W65C02_OP(eor):
    case W65C02_OP(jsr):
    case W65C02_OP(pla):
    case W65C02_OP(tya):
    case W65C02_OP(txa):
    case W65C02_OP(jumptable):

    case W65C02_OP(var_addr):
    case W65C02_OP(var_addrb):
    case W65C02_OP(arg_addr):
    case  W65C02_OP(arg_addrb):

    case  W65C02_OP(var_addr_xy):
    case  W65C02_OP(var_addrb_xy):
    case   W65C02_OP(arg_addr_xy):
    case   W65C02_OP(arg_addrb_xy):

    case   W65C02_OP(var_value1):
    case   W65C02_OP(var_value1b):

    case   W65C02_OP(var_value2):
    case   W65C02_OP(var_value2b):

    case   W65C02_OP(var_value4):
    case   W65C02_OP(var_value4b):

    case   W65C02_OP(var_value8):
    case   W65C02_OP(var_value8b):

    case    W65C02_OP(arg_value1):
    case    W65C02_OP(arg_value1b):

    case    W65C02_OP(arg_value2):
    case   W65C02_OP(arg_value2b):

    case   W65C02_OP(arg_value4):
    case   W65C02_OP(arg_value4b):

    case   W65C02_OP(arg_value8):
    case   W65C02_OP(arg_value8b):
      
    case W65C02_OP(pushreg2):
    case W65C02_OP(pushreg4):
    case W65C02_OP(pushreg8):
      return true;
      
    case W65C02_OP(dec):
    case W65C02_OP(inc):
    case W65C02_OP(asl):
    case W65C02_OP(rol):
    case W65C02_OP(lsr):
    case W65C02_OP(ror):
      return GetAddrMode(inst) == kAddrModeAccumulator;
      
    default:
      return false;
  }
}

static bool ModifiesFlags(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(lda):
    case W65C02_OP(ldx):
    case W65C02_OP(ldy):
    case W65C02_OP(adc):
    case W65C02_OP(sbc):
    case W65C02_OP(ora):
    case W65C02_OP(and):
    case W65C02_OP(eor):
    case W65C02_OP(jsr):
    case W65C02_OP(pla):
    case W65C02_OP(ply):
    case W65C02_OP(plx):
    case W65C02_OP(tya):
    case W65C02_OP(txa):
    case W65C02_OP(tay):
    case W65C02_OP(tax):
    case W65C02_OP(dey):
    case W65C02_OP(iny):
    case W65C02_OP(dex):
    case W65C02_OP(inx):
    case W65C02_OP(jumptable):

    case W65C02_OP(var_addr):
    case W65C02_OP(var_addrb):
    case W65C02_OP(arg_addr):
    case  W65C02_OP(arg_addrb):

    case  W65C02_OP(var_addr_xy):
    case  W65C02_OP(var_addrb_xy):
    case   W65C02_OP(arg_addr_xy):
    case   W65C02_OP(arg_addrb_xy):

    case   W65C02_OP(var_value1):
    case   W65C02_OP(var_value1b):

    case   W65C02_OP(var_value2):
    case   W65C02_OP(var_value2b):

    case   W65C02_OP(var_value4):
    case   W65C02_OP(var_value4b):

    case   W65C02_OP(var_value8):
    case   W65C02_OP(var_value8b):

    case    W65C02_OP(arg_value1):
    case    W65C02_OP(arg_value1b):

    case    W65C02_OP(arg_value2):
    case   W65C02_OP(arg_value2b):

    case   W65C02_OP(arg_value4):
    case   W65C02_OP(arg_value4b):

    case   W65C02_OP(arg_value8):
    case   W65C02_OP(arg_value8b):
      
    case W65C02_OP(pushreg2):
    case W65C02_OP(pushreg4):
    case W65C02_OP(pushreg8):
      return true;
      
    case W65C02_OP(dec):
    case W65C02_OP(inc):
    case W65C02_OP(asl):
    case W65C02_OP(rol):
    case W65C02_OP(lsr):
    case W65C02_OP(ror):
      return GetAddrMode(inst) == kAddrModeAccumulator;
      
    default:
      return false;
  }
}
static bool UsesA(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(sta):
    case W65C02_OP(pha):
    case W65C02_OP(cmp):
    case W65C02_OP(adc):
    case W65C02_OP(sbc):
    case W65C02_OP(ora):
    case W65C02_OP(and):
    case W65C02_OP(eor):
    case W65C02_OP(tay):
    case W65C02_OP(tax):
    case W65C02_OP(jsr):
    case W65C02_OP(jumptable):

    case W65C02_OP(var_addr):
    case W65C02_OP(var_addrb):
    case W65C02_OP(arg_addr):
    case  W65C02_OP(arg_addrb):

    case  W65C02_OP(var_addr_xy):
    case  W65C02_OP(var_addrb_xy):
    case   W65C02_OP(arg_addr_xy):
    case   W65C02_OP(arg_addrb_xy):

    case   W65C02_OP(var_value1):
    case   W65C02_OP(var_value1b):

    case   W65C02_OP(var_value2):
    case   W65C02_OP(var_value2b):

    case   W65C02_OP(var_value4):
    case   W65C02_OP(var_value4b):

    case   W65C02_OP(var_value8):
    case   W65C02_OP(var_value8b):

    case    W65C02_OP(arg_value1):
    case    W65C02_OP(arg_value1b):

    case    W65C02_OP(arg_value2):
    case   W65C02_OP(arg_value2b):

    case   W65C02_OP(arg_value4):
    case   W65C02_OP(arg_value4b):

    case   W65C02_OP(arg_value8):
    case   W65C02_OP(arg_value8b):
      
    case W65C02_OP(pushreg2):
    case W65C02_OP(pushreg4):
    case W65C02_OP(pushreg8):
     return true;
      
    case W65C02_OP(dec):
    case W65C02_OP(inc):
    case W65C02_OP(asl):
    case W65C02_OP(rol):
    case W65C02_OP(lsr):
    case W65C02_OP(ror):
      return GetAddrMode(inst) == kAddrModeAccumulator;
      
    default:
      return false;
  }
  return false;
}

static TargetInstruction* PreviousModifierOfA(TargetInstruction* inst) {
  TargetInstruction* prev = inst;
  do {
    prev = TargetPrev(prev);
    if (prev == NULL || ((int)prev->opcode == (int)W65C02_OP(label))) {
      // Don't go past a label.
      return NULL;
    }
    // Don't cross basic blocks.
    if (prev->block != inst->block) {
      return NULL;
    }
    if (UsesA(prev)) {
      // LDA
      // ...
      // STA   <- don't go past this.
      // ...
      // LDA
      return NULL;
    }
  } while (prev != NULL && !ModifiesA(prev));
  return prev;
}

static TargetInstruction* PreviousModifierOfFlags(TargetInstruction* inst) {
  TargetInstruction* prev = inst;
  do {
    prev = TargetPrev(prev);
    if (prev == NULL || ((int)prev->opcode == (int)W65C02_OP(label))) {
      // Don't go past a label.
      return NULL;
    }
    // Don't cross basic blocks.
    if (prev->block != inst->block) {
      return NULL;
    }
  } while (prev != NULL && !ModifiesFlags(prev));
  return prev;
}

static TargetInstruction* PreviousUserOfA(TargetInstruction* inst) {
  // return NULL;
  TargetInstruction* prev = inst;
  do {
    prev = TargetPrev(prev);
    if (prev == NULL || ((int)prev->opcode == (int)W65C02_OP(label))) {
      // Don't go past a label.
      return NULL;
    }
    // Don't cross basic blocks.
    if (prev->block != inst->block) {
      return NULL;
    }
  } while (prev != NULL && !UsesA(prev));
  return prev;
}

// Do we pass over inst between start and end, going backwards.
static bool PassesOverBackwards(TargetInstruction* start, TargetInstruction* end, TargetInstruction* inst) {
  TargetInstruction* prev = TargetPrev(start);
  while (prev != NULL && prev != end) {
    // Don't cross basic blocks.
    if (prev->block != start->block) {
      return true;
    }
    if (prev == inst) {
      return true;
    }
    prev = TargetPrev(prev);
  }
  return false;
}

// An expression may be removed it if's unused.  However, stores to the
// expression count as uses so we have to count all uses that are not
// stores.
static bool IsUnusedExpression(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
      break;
    default:
      return false;
  }
  int uses = 0;
  int num_stores = 0;
  int64_t store_offset = -1;
  bool store_offset_set = false;
  bool all_store_offsets_same = true;
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* user = inst->users.value.p[i];
    W65C02Opcode opcode = (W65C02Opcode)user->opcode;
    switch (opcode) {
      case W65C02_OP(sta):
      case W65C02_OP(stx):
      case W65C02_OP(sty):
      case W65C02_OP(stz): {
        AddressingMode mode = GetAddrMode(user);
        if (mode == kAddrModeIndirect || mode == kAddrModeIndirectIndexed ||
            mode == kAddrModeIndexedIndirect) {
          // This is a read.
          uses++;
        } else {
          // This is store to a variable.
          uses++;
          num_stores++;
          // Get offset into variable and check against current.
          int64_t offset = TargetIntValue(user->operand[1]);
          if (store_offset_set) {
            if (store_offset != offset) {
              all_store_offsets_same = false;
            }
          } else {
            store_offset = offset;
            store_offset_set = true;
          }

        }
        break;
      }
      case W65C02_OP(var_addr):
      case W65C02_OP(reloadpoint):
        break;

      default:
        uses++;
        break;
    }
  }
  if (uses == 0) {
    return true;
  }
  if (uses == num_stores && all_store_offsets_same) {
    // All uses are stores.
    return true;
  }
  return false;
}


static void OptimizeBlock(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  // Propagate optimizar data down from idom.  All tracked register values
  // in the idom are valid in this block because it dominates us.
  
  RegTrackers* dom_trackers = NULL;
  if (block->idom != NULL) {
    // If we have an immediate dominator we propagate the
    // variables from it to this node.
    dom_trackers = block->idom->cookie;
  }

  RegTrackers* trackers = NULL;
  if (block->idom != NULL && block->idom->dominatees.length == 1) {
    // This block is the only one dominated by the dominator so we
    // can just reuse the trackers from the dominator.
    trackers = dom_trackers;
    block->idom->cookie = NULL;  // This is no longer valid.
    block->cookie = trackers;
  } else {
    // There is more than one block that is dominated by my dominator.
    // We need to copy the trackers from the dominator.
    trackers = NewTrackers();
    block->cookie = trackers;
    if (dom_trackers != NULL) {
      trackers->A = dom_trackers->A;
      trackers->X = dom_trackers->X;
      trackers->Y = dom_trackers->Y;
    }
  }

  RegTracker current_A = {kRegUnknown, kRegA};
  RegTracker current_X = {kRegUnknown, kRegX};
  RegTracker current_Y = {kRegUnknown, kRegY};
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = block->code; inst != NULL &&
       block->end_code != NULL &&
       TargetPrev(inst) != block->end_code; inst = next) {
    next = block->end_code == NULL ? NULL : TargetNext(inst);
    if ((inst->flags & k6502DontEmit) != 0) {
      // Don't look at pseudo-deleted instructions.
      continue;
    }
     switch ((W65C02Opcode)inst->opcode) {
      case W65C02_OP(lda): {
        TargetInstruction* prev_user = PreviousUserOfA(inst);
        TargetInstruction* prev_modifier = PreviousModifierOfA(inst);
        if (prev_modifier != NULL && prev_modifier->opcode == (TargetOpcode)W65C02_OP(lda)) {
          if (!PassesOverBackwards(inst, prev_modifier, prev_user)){
            // LDA following an LDA, remove previous.
            TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, prev_modifier);
            opt_data->modified = true;
            break;
          }
        }
        if (prev_modifier != NULL) {
          // If previous modifier is the same instruction, A hasn't changed
          // remove current instruction.
          // This covers:
          // LDA x
          // instructions that don't modify A
          // LDA x
          // STA followed by LDA, remove LDA.
          if (prev_modifier->opcode == inst->opcode &&
              prev_modifier->operand[0] == inst->operand[0] && prev_modifier->operand[1] == inst->operand[1] &&
              GetAddrMode(prev_modifier) == GetAddrMode(inst)) {
            TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
            opt_data->modified = true;
            break;
          }
        }
        if (prev_user != NULL && prev_user->opcode == (TargetOpcode)W65C02_OP(sta)) {
          // STA followed by LDA, remove LDA.
          // But we need to check that if the addressing mode is indirect (Y or X), the index
          // hasn't been modified.  We can't really know that, so prevent removal for indirect
          // addressing modes.
          if (prev_user->operand[0] == inst->operand[0] && prev_user->operand[1] == inst->operand[1] &&
              GetAddrMode(prev_user) == GetAddrMode(inst) &&
              GetAddrMode(inst) != kAddrModeIndirectIndexed && GetAddrMode(inst) != kAddrModeIndirect) {
            TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
            opt_data->modified = true;
            break;
          }
        }
 
        TrackReg(&current_A, inst);
        OptimizeInstruction(opt_data, &trackers->A, &current_A, block, inst) ||
          OptimizeTransfer(opt_data, &trackers->Y, &current_A, block, inst) ||
                     OptimizeTransfer(opt_data, &trackers->X, &current_A, block, inst);
        trackers->A = current_A;
        break;
      }
      case W65C02_OP(sta): {
        if (IsUnusedExpression(inst->operand[0])) {
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
          opt_data->modified = true;
          break;
        }
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
        if (trackers->A.type == kRegConstant && trackers->A.value.c == 0) {
          // Storing value 0 can be converted to STZ as long as the
          // addressing mode is OK.
          if (Is65c02() && mode != kAddrModeIndirectIndexed && mode != kAddrModeIndirect && mode != kAddrModeZeroPageIndexedX
              && mode != kAddrModeZeroPageIndexedY) {
            inst->opcode = (TargetOpcode)W65C02_OP(stz);
            TargetInstruction* prev = PrevInstruction(inst);
            while (prev->opcode == (TargetOpcode)W65C02_OP(lda)) {
               // STA following an LDA, remove previous.
               TargetInstruction* p = TargetPrev(prev);
               TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, prev);
               opt_data->modified = true;
               prev = p;
             }
            // We've removed the LDA #0 so we don't know what A is now.
            trackers->A.type = kRegUnknown;
          }
        }
        break;
      }
         
      case W65C02_OP(stz):
         // STZ has no effect on registers.
         break;

       case W65C02_OP(label):
         if (inst->users.length == 0) {
           // Unused label.
           TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
           opt_data->modified = true;
         } else {
           trackers->A.type = kRegUnknown;
           trackers->X.type = kRegUnknown;
           trackers->Y.type = kRegUnknown;
         }
         break;
         
      case W65C02_OP(ldx):
        TrackReg(&current_X, inst);
        OptimizeInstruction(opt_data, &trackers->X, &current_X, block, inst) ||
          OptimizeTransfer(opt_data, &trackers->A, &current_X, block, inst);
        trackers->X = current_X;
        break;
      case W65C02_OP(stx):
         if (IsUnusedExpression(inst->operand[0])) {
           TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
           opt_data->modified = true;
           break;
         }
        TrackReg(&current_X, inst);
        trackers->X = current_X;
        break;
      case W65C02_OP(inx):
        if (trackers->X.type == kRegConstant) {
          trackers->X.value.c++;
        }
        break;
      case W65C02_OP(dex):
        if (trackers->X.type == kRegConstant) {
          trackers->X.value.c-- ;
        }
        break;
      case W65C02_OP(ldy):
        TrackReg(&current_Y, inst);
        OptimizeInstruction(opt_data, &trackers->Y, &current_Y, block, inst) ||
          OptimizeTransfer(opt_data, &trackers->A, &current_Y, block, inst);
        trackers->Y = current_Y;
        break;
      case W65C02_OP(sty):
         if (IsUnusedExpression(inst->operand[0])) {
           TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
           opt_data->modified = true;
           break;
         }
         TrackReg(&current_Y, inst);
        trackers->Y = current_Y;
        break;
       case W65C02_OP(iny): {
        if (trackers->Y.type == kRegConstant) {
          trackers->Y.value.c++;
        }

         // An INY immediately preceded by LDY.  We can just increment the
         // LDY provided it's an immediate, and remove the INY.  This can
         // happen as we remove unused expressions and instructions.  We can
         // get a sequence of instructions like
         // LDY #2
         // INY
         // INY
         // INY
         //
         // We can remove the INY and set the LDY instruction to the newly
         // calculated value.
         TargetInstruction* prev = PrevInstruction(inst);
         if (((int)prev->opcode == (int)W65C02_OP(ldy)) && GetAddrMode(prev) == kAddrModeImmediate) {
           SetImmediateValue(opt_data, prev, trackers->Y.value.c);
           TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
           opt_data->modified = true;
        }
        break;
       }
      case W65C02_OP(dey):
        if (trackers->Y.type == kRegConstant) {
          trackers->Y.value.c-- ;
        }
        break;
        
      case W65C02_OP(jsr):
        trackers->A.type = kRegUnknown;
        trackers->X.type = kRegUnknown;
        trackers->Y.type = kRegUnknown;
        break;
        
      case W65C02_OP(rts): {
        // A JSR followed by an RTS can be replaced by a JMP.
        TargetInstruction* prev = PrevInstruction(inst);
        if (prev == NULL) {
          break;
        }
        if (((int)prev->opcode == (int)W65C02_OP(jsr))) {
          prev->opcode = (TargetOpcode)W65C02_OP(jmp);
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
          opt_data->modified = true;
        } else if (((int)prev->opcode == (int)W65C02_OP(leave)) ||
                   ((int)prev->opcode == (int)W65C02_OP(leave_leaf))) {
          // leave and leave_leaf can be told to use JMP instead of JSR.
          prev->flags |= k6502JmpForJSR;
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
          opt_data->modified = true;
        }
        break;
      }
        
      case W65C02_OP(adc):
      case W65C02_OP(sbc):
        // These modify the accumulator.
         trackers->A.type = kRegUnknown;
         break;
      case W65C02_OP(ora):
        trackers->A.type = kRegUnknown;
        if (TargetIsConst(inst->operand[0])) {
          int v = GetConstantByte(inst->operand[0], inst->operand[1]);
          if (v == 255) {
            // ORA #255 sets A to 255.
            trackers->A.type = kRegConstant;
            trackers->A.value.c = 255;
            break;
          }
          if (trackers->A.type == kRegConstant) {
            trackers->A.value.c |= v;
          }
        } else {
          // We can't remove the ORA because the flags aren't set.
          //
          // ORA preceeded by an STZ with the same operand can be eliminated.
          TargetInstruction* prev = PrevInstruction(inst);
          if (((int)prev->opcode == (int)W65C02_OP(stz))) {
            if (prev->operand[0] == inst->operand[0]) {
              TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
              opt_data->modified = true;
              break;
            }
          }
        }
        break;
      case W65C02_OP(eor):
        trackers->A.type = kRegUnknown;
        if (TargetIsConst(inst->operand[0])) {
          int v = GetConstantByte(inst->operand[0], inst->operand[1]);
          if (trackers->A.type == kRegConstant) {
            trackers->A.value.c ^= v;
          }
        }
        break;
      case W65C02_OP(and):
        trackers->A.type = kRegUnknown;
        if (TargetIsConst(inst->operand[0])) {
         int v = GetConstantByte(inst->operand[0], inst->operand[1]);
         if (v == 255) {
           // AND #255 keeps A the same.
           TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
           opt_data->modified = true;
           break;
         }
          if (v == 0) {
            // AND #0 sets A to zero.
            inst->opcode = (TargetOpcode)W65C02_OP(lda);
            trackers->A.type = kRegConstant;
            trackers->A.value.c = 0;
            break;
          }
         if (trackers->A.type == kRegConstant) {
           trackers->A.value.c &= v;
         }
       }
        break;
        
        case W65C02_OP(asl):
        case W65C02_OP(lsr):
        case W65C02_OP(rol):
        case W65C02_OP(ror):
        // These might modify the accumulator:
        if (GetAddrMode(inst) == kAddrModeAccumulator) {
          trackers->A.type = kRegUnknown;
        }
        break;
        
      case W65C02_OP(expr1):
      case W65C02_OP(expr2):
      case W65C02_OP(expr4):
      case W65C02_OP(expr8):
      case W65C02_OP(exprf):
      case W65C02_OP(exprd):
      case W65C02_OP(clc):
      case W65C02_OP(sec):
      case W65C02_OP(fake_bra):
       case W65C02_OP(reloadpoint):
       case W65C02_OP(cpy):
       case W65C02_OP(cpx):
         break;
         
      // If CMP #0 is preceeded by LDA, ORA, AND, EOR, INC or DEC we can
      // remove the CMP #0.
      case W65C02_OP(cmp):
         if ((inst->flags & k6502GeneratesFlags) == 0 && GetAddrMode(inst) == kAddrModeImmediate) {
           TargetInstruction* prev = PreviousModifierOfFlags(inst);
           if (prev != NULL && (((int)prev->opcode == (int)W65C02_OP(lda)) || ((int)prev->opcode == (int)W65C02_OP(ora)) ||
                                ((int)prev->opcode == (int)W65C02_OP(and)) || ((int)prev->opcode == (int)W65C02_OP(eor))
                                || ((int)prev->opcode == (int)W65C02_OP(inc)) || ((int)prev->opcode == (int)W65C02_OP(dec)))) {
             if (ImmediateValue(inst) == 0) {
               // We can't remove this instruction because it's a user of A
               // and it will make other optimizations incorrect.
               inst->flags |= k6502DontEmit;
               opt_data->modified = true;
               break;
             }
           }
         }
         break;
       
         // For branches, if they are a backward branch we don't know the
         // value, but if they are forward they can be ignored.
         // TODO: don't know if it's backward branch.
      case W65C02_OP(bra):
      case W65C02_OP(beq):
      case W65C02_OP(bne):
      case W65C02_OP(bpl):
      case W65C02_OP(bmi):
      case W65C02_OP(bvc):
      case W65C02_OP(bvs):
      case W65C02_OP(bcs):
      case W65C02_OP(bcc): {
        trackers->A.type = kRegUnknown;
        trackers->X.type = kRegUnknown;
        trackers->Y.type = kRegUnknown;
         TargetInstruction* branch_target = inst->operand[0];
         if (branch_target == next) {
           // Branch to next instruction, remove branch.
           TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
           break;
         }
 
        break;
        }
         
      default:
        // Not an instruction we can track, all values are unknown.
        trackers->A.type = kRegUnknown;
        trackers->X.type = kRegUnknown;
        trackers->Y.type = kRegUnknown;
        break;
    }
    inst = next;
  }
}

void W65C02Optimize(W65C02Generator* g) {
  // return;
  for (;;) {
    struct OptimizerData data = {g, false};
    TargetTraverseDominatorTree(&g->base, OptimizeBlock, kTraversePreOrder, &data);
    
    // Done with all the trackers, delete them all.
    for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
      TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
      if (block->cookie != NULL) {
        free(block->cookie);
        block->cookie = NULL;
      }
    }
    if (!data.modified) {
      break;
    }
  }
}

// Variable reference pooler.
struct PoolerData {
  W65C02Generator* g;
};

static bool IsVariable(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(var_addr):
    case W65C02_OP(var_addrb):
    case W65C02_OP(arg_addr):
    case W65C02_OP(arg_addrb):
#if 0
    case W65C02_OP(var_value1):
    case W65C02_OP(var_value2):
    case W65C02_OP(var_value4):
    case W65C02_OP(var_value8):
    case W65C02_OP(var_value1b):
    case W65C02_OP(var_value2b):
    case W65C02_OP(var_value4b):
    case W65C02_OP(var_value8b):
    case W65C02_OP(arg_value1):
    case W65C02_OP(arg_value2):
    case W65C02_OP(arg_value4):
    case W65C02_OP(arg_value8):
    case W65C02_OP(arg_value1b):
    case W65C02_OP(arg_value2b):
    case W65C02_OP(arg_value4b):
    case W65C02_OP(arg_value8b):
#endif
      return true;
    default:
      return false;
  }
}

// We can pool variables from our immediate dominator only if we are
// not a loop header.  A loop header is a block with an input edge that is
// dominated by this block itself.  In other words, there is a path
// from this block back to itself.
static COMPILER_UNUSED bool CanPoolFromDominator(TargetGenerator* g, TargetBasicBlock* block) {
  for (size_t i = 0; i < block->in_edges.length; i++) {
    TargetBlockId id = block->in_edges.value.w[i];
    TargetBasicBlock* in_block = g->basic_blocks.value.p[id];
    if (TargetBasicBlockDominatedBy(g, block, in_block)) {
      return false;
    }
  }
  return true;
}

static void PoolVariables(TargetBasicBlock* block, void* data) {
  struct PoolerData* pool_data = data;
    W65C02Generator* g = pool_data->g;
  Map* dominator_variables = NULL;
  bool can_pool_from_dominator = CanPoolFromDominator(&g->base, block);

  Map* vars = NULL;
  
  if (can_pool_from_dominator) {
    if (block->idom != NULL) {
      // If we have an immediate dominator we propagate the
      // variables from it to this node.
      dominator_variables = block->idom->cookie;
    }

    if (block->idom != NULL && block->idom->dominatees.length == 1) {
      // This block is the only one dominated by the dominator so we
      // can just reuse the value set from the dominator.
      vars = dominator_variables;
      block->idom->cookie = NULL;  // This is no longer valid.
      block->cookie = vars;
    } else {
      // There is more than one block that is dominated by my dominator.
      // We need to copy the variables from the dominator.
      vars = NewMapForInt64Keys();
      block->cookie = vars;
      if (dominator_variables != NULL) {
        MapCopy(vars, dominator_variables);
      }
    }
  } else {
    // We can't pool from our dominator, make a new variables map.
    vars = NewMapForInt64Keys();
    block->cookie = vars;
  }
  
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = block->code; inst != NULL &&
       block->end_code != NULL &&
       TargetPrev(inst) != block->end_code; inst = next) {
    next = block->end_code == NULL ? NULL : TargetNext(inst);

    if (IsVariable(inst)) {
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      
      // For the value in the map we use the opcode for the variable
      // reference instruction in the upper 32 bits and the id of the
      // variable instruction in the lower 32 bits.
      int64_t value = (int64_t)inst->opcode << 32 | (int64_t)var->id;
      MapKeyType key = {.w = value};
      TargetInstruction* pooled = MapFind(vars, key);
      if (pooled == NULL) {
        MapKeyValue kv = {.key.w = value, .value.p = result};
        MapInsert(vars, kv);
      } else {
        // Variable is in pool, replace the result instruction with the a
        // reference to the pooled one.  Then delete the variable instruction.
        TargetBasicBlockReplaceInstruction(&g->base, block, result, pooled);
        TargetBasicBlockRemoveInstruction(&g->base, block, inst);
      }
    }
    inst = next;
  }

}

void W65C02PoolVariables(W65C02Generator* g) {
  struct PoolerData data = {g};
  TargetTraverseDominatorTree(&g->base, PoolVariables, kTraversePreOrder, &data);
  
  // Done with all the variable maps, delete them all.
  for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
    if (block->cookie != NULL) {
      MapDelete((Map*)block->cookie);
      block->cookie = NULL;
    }
  }
  
  // Inputs and outputs might have changed, recalculate them.
  TargetBuildBasicBlockInputsAndOutputs(&g->base);
}

#if 0
static void PoolVariables(TargetBasicBlock* block, void* data) {
  struct PoolerData* pool_data = data;
  W65C02Generator* g = pool_data->g;
  
  Map vars;
  MapInitForInt64Keys(&vars);
  
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = block->code; inst != NULL &&
       block->end_code != NULL &&
       TargetPrev(inst) != block->end_code; inst = next) {
    next = block->end_code == NULL ? NULL : TargetNext(inst);

    if (IsVariable(inst)) {
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      
      // For the value in the map we use the opcode for the variable
      // reference instruction in the upper 32 bits and the id of the
      // variable instruction in the lower 32 bits.
      int64_t value = (int64_t)inst->opcode << 32 | (int64_t)var->id;
      MapKeyType key = {.w = value};
      TargetInstruction* pooled = MapFind(&vars, key);
      if (pooled == NULL) {
        MapKeyValue kv = {.key.w = value, .value.p = result};
        MapInsert(&vars, kv);
      } else {
        // Variable is in pool, replace the result instruction with the a
        // reference to the pooled one.  Then delete the variable instruction.
        TargetBasicBlockReplaceInstruction(&g->base, block, result, pooled);
        TargetBasicBlockRemoveInstruction(&g->base, block, inst);
      }
    }
    inst = next;
  }
  MapDestruct(&vars);
}

void W65C02PoolVariables(W65C02Generator* g) {
  struct PoolerData data = {g};
  TargetTraverseDominatorTree(&g->base, PoolVariables, kTraversePreOrder, &data);
}
#endif

