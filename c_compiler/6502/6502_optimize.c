//
//  6502_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 5/11/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "6502_optimize.h"
#include "6502_codegen.h"
#include "compiler.h"
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
  } while (prev != NULL && !(TargetOpcodeNe(prev->opcode, W65C02_OP(reloadpoint)) &&
           TargetOpcodeNe(prev->opcode, W65C02_OP(expr1)) &&
           TargetOpcodeNe(prev->opcode, W65C02_OP(expr2)) &&
           TargetOpcodeNe(prev->opcode, W65C02_OP(expr4)) &&
           TargetOpcodeNe(prev->opcode, W65C02_OP(expr8))));
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
    case W65C02_OP(pushmem1):
    case W65C02_OP(pushmem2):
    case W65C02_OP(pushmem_xy1):
    case W65C02_OP(pushmem_xy2):
    case W65C02_OP(copymem1):
    case W65C02_OP(copymem2):
    case W65C02_OP(zeromem1):
    case W65C02_OP(zeromem2):
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
    case W65C02_OP(pushmem1):
    case W65C02_OP(pushmem2):
    case W65C02_OP(pushmem_xy1):
    case W65C02_OP(pushmem_xy2):
    case W65C02_OP(copymem1):
    case W65C02_OP(copymem2):
    case W65C02_OP(zeromem1):
    case W65C02_OP(zeromem2):
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
    if (prev == NULL || TargetOpcodeEq(prev->opcode, W65C02_OP(label))) {
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
    if (prev == NULL || TargetOpcodeEq(prev->opcode, W65C02_OP(label))) {
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
    if (prev == NULL || TargetOpcodeEq(prev->opcode, W65C02_OP(label))) {
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

static bool IsTrackingMetadata(TargetInstruction* inst) {
  return TargetOpcodeEq(inst->opcode, W65C02_OP(reloadpoint)) ||
         W65C02IsExpression(inst) || (inst->flags & k6502DontEmit) != 0;
}

static bool ByteValueLoadLeavesA(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(var_value1):
    case W65C02_OP(var_value1b):
    case W65C02_OP(arg_value1):
    case W65C02_OP(arg_value1b):
      return true;
    default:
      return false;
  }
}

static TargetInstruction* NextEffectiveInstruction(TargetInstruction* inst) {
  TargetInstruction* next = TargetNext(inst);
  while (next != NULL && IsTrackingMetadata(next)) {
    next = TargetNext(next);
  }
  return next;
}

// On the fallthrough edge of BNE following CMP/CPX/CPY, the compared register
// is equal to the immediate operand. Remove a matching load directly on that
// edge when there is no other entry to it. The following instruction must
// overwrite the load's N/Z flags, since the compare and load produce different
// Z values for a nonzero constant.
static bool RemoveCompareFallthroughLoad(struct OptimizerData* opt_data,
                                         TargetBasicBlock* block,
                                         TargetInstruction* branch) {
  if (!TargetOpcodeEq(branch->opcode, W65C02_OP(bne))) {
    return false;
  }

  TargetInstruction* compare = PrevInstruction(branch);
  if (compare == NULL) {
    return false;
  }

  W65C02Opcode load_opcode;
  if (TargetOpcodeEq(compare->opcode, W65C02_OP(cmp))) {
    load_opcode = W65C02_OP(lda);
  } else if (TargetOpcodeEq(compare->opcode, W65C02_OP(cpx))) {
    load_opcode = W65C02_OP(ldx);
  } else if (TargetOpcodeEq(compare->opcode, W65C02_OP(cpy))) {
    load_opcode = W65C02_OP(ldy);
  } else {
    return false;
  }
  if (GetAddrMode(compare) != kAddrModeImmediate) {
    return false;
  }

  TargetInstruction* fallthrough = TargetNext(branch);
  if (fallthrough == NULL) {
    return false;
  }
  TargetBasicBlock* fallthrough_block = fallthrough->block;
  if (fallthrough_block != block &&
      (fallthrough_block->in_edges.length != 1 ||
       fallthrough_block->in_edges.value.w[0] != block->block_id)) {
    return false;
  }

  TargetInstruction* load = NextEffectiveInstruction(branch);
  if (load == NULL || load->block != fallthrough_block ||
      !TargetOpcodeEq(load->opcode, load_opcode) ||
      GetAddrMode(load) != kAddrModeImmediate ||
      ImmediateValue(load) != ImmediateValue(compare)) {
    return false;
  }

  TargetInstruction* flags_overwrite = NextEffectiveInstruction(load);
  if (flags_overwrite == NULL || flags_overwrite->block != fallthrough_block ||
      !ModifiesFlags(flags_overwrite)) {
    return false;
  }

  TargetBasicBlockRemoveInstruction(&opt_data->g->base, fallthrough_block,
                                    load);
  opt_data->modified = true;
  return true;
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
        TargetInstruction* previous = TargetPrev(inst);
        while (previous != NULL && IsTrackingMetadata(previous)) {
          previous = TargetPrev(previous);
        }
        if (ByteValueLoadLeavesA(previous) &&
            previous->operand[0] == inst->operand[0]) {
          // The byte-load helpers store the loaded value in their destination
          // and return with that same value still in A. Avoid loading it back
          // from zero page when the next operation consumes the byte.
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
          opt_data->modified = true;
          break;
        }
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
         if (TargetOpcodeEq(prev->opcode, W65C02_OP(ldy)) &&
             GetAddrMode(prev) == kAddrModeImmediate) {
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
      case W65C02_OP(pushmem1):
      case W65C02_OP(pushmem2):
      case W65C02_OP(pushmem_xy1):
      case W65C02_OP(pushmem_xy2):
      case W65C02_OP(copymem1):
      case W65C02_OP(copymem2):
      case W65C02_OP(zeromem1):
      case W65C02_OP(zeromem2):
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
        if (TargetOpcodeEq(prev->opcode, W65C02_OP(jsr))) {
          prev->opcode = (TargetOpcode)W65C02_OP(jmp);
          TargetBasicBlockRemoveInstruction(&opt_data->g->base, block, inst);
          opt_data->modified = true;
        } else if (TargetOpcodeEq(prev->opcode, W65C02_OP(leave)) ||
                   TargetOpcodeEq(prev->opcode, W65C02_OP(leave_leaf))) {
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
          if (TargetOpcodeEq(prev->opcode, W65C02_OP(stz))) {
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
           if (prev != NULL && (TargetOpcodeEq(prev->opcode, W65C02_OP(lda)) ||
                                TargetOpcodeEq(prev->opcode, W65C02_OP(ora)) ||
                                TargetOpcodeEq(prev->opcode, W65C02_OP(and)) ||
                                TargetOpcodeEq(prev->opcode, W65C02_OP(eor)) ||
                                TargetOpcodeEq(prev->opcode, W65C02_OP(inc)) ||
                                TargetOpcodeEq(prev->opcode, W65C02_OP(dec)))) {
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
        if (RemoveCompareFallthroughLoad(opt_data, block, inst)) {
          next = TargetNext(inst);
        }
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

static bool FoldExpressionCopyChains(W65C02Generator* g);

void W65C02Optimize(W65C02Generator* g) {
  // return;
  for (;;) {
    struct OptimizerData data = {g, false};
    TargetTraverseDominatorTree(&g->base, OptimizeBlock, kTraversePreOrder, &data);
    if (FoldExpressionCopyChains(g)) {
      data.modified = true;
    }
    
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

static int RegisterByteAddress(TargetInstruction* value, int byte) {
  W65C02Register* reg = (W65C02Register*)value->reg;
  assert(reg != NULL);
  switch (reg->type) {
    case k6502RegTypeB:
      return W65C02_B_REG_START + reg->base.num + byte;
    case k6502RegTypeI:
      return W65C02_I_REG_START + reg->base.num * 2 + byte;
    case k6502RegTypeL:
      return W65C02_L_REG_START + reg->base.num * 4 + byte;
    case k6502RegTypeX:
      return W65C02_X_REG_START + reg->base.num * 8 + byte;
    case k6502RegTypeF:
      return W65C02_F_REG_START + reg->base.num * 4 + byte;
  }
  abort();
}

static int OperandByteOffset(TargetInstruction* inst) {
  return inst->operand[1] == NULL ? 0
                                  : (int)TargetIntValue(inst->operand[1]);
}

static int OperandByteAddress(TargetInstruction* inst) {
  return RegisterByteAddress(inst->operand[0], OperandByteOffset(inst));
}

static bool IsPostAllocationMetadata(TargetInstruction* inst) {
  if (TargetIsConst(inst) || W65C02IsExpression(inst) ||
      (inst->flags & k6502DontEmit) != 0) {
    return true;
  }
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(tmp):
    case W65C02_OP(fp):
    case W65C02_OP(sp):
    case W65C02_OP(ap):
    case W65C02_OP(tp):
    case W65C02_OP(literal):
    case W65C02_OP(resulti):
    case W65C02_OP(resultf):
    case W65C02_OP(resultd):
    case W65C02_OP(ret):
    case W65C02_OP(localvar):
    case W65C02_OP(argument):
    case W65C02_OP(symbol):
    case W65C02_OP(fake_bra):
    case W65C02_OP(ssavar):
    case W65C02_OP(phi):
    case W65C02_OP(reloadpoint):
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
    case W65C02_OP(literalrefX):
      return true;
    default:
      return false;
  }
}

static TargetInstruction* PreviousPostAllocationInstruction(
    TargetInstruction* inst) {
  TargetInstruction* prev = TargetPrev(inst);
  while (prev != NULL && IsPostAllocationMetadata(prev)) {
    prev = TargetPrev(prev);
  }
  return prev;
}

// Register allocation can assign consecutive temporary values to the same
// physical zero-page register.  Constant materialization then sometimes leaves
// runs such as:
//
//   sta __i0
//   sta __i0+1
//   sta __i0
//   sta __i0+1
//
// Since STA does not modify A and nothing observes memory between these
// contiguous stores, retain only the first write to each physical byte.
static void RemoveRepeatedZeroPageStores(W65C02Generator* g) {
  for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;) {
      TargetInstruction* next =
          inst == block->end_code ? NULL : TargetNext(inst);
      if (TargetOpcodeEq(inst->opcode, W65C02_OP(sta)) &&
          GetAddrMode(inst) == kAddrModeZeroPage &&
          inst->operand[0] != NULL && inst->operand[0]->reg != NULL) {
        int address = OperandByteAddress(inst);
        for (TargetInstruction* prev =
                 PreviousPostAllocationInstruction(inst);
             prev != NULL && TargetOpcodeEq(prev->opcode, W65C02_OP(sta)) &&
             GetAddrMode(prev) == kAddrModeZeroPage &&
             prev->operand[0] != NULL && prev->operand[0]->reg != NULL;
             prev = PreviousPostAllocationInstruction(prev)) {
          if (OperandByteAddress(prev) == address) {
            TargetBasicBlockRemoveInstruction(&g->base, block, inst);
            break;
          }
        }
      }
      inst = next;
    }
  }
}

// After register allocation, distinct byte temporaries can become the same
// physical zero-page register. Fold
//
//   lda source
//   sta temporary
//   lda temporary
//
// by removing the reload. The first LDA already leaves both A and its N/Z
// flags in exactly the state produced by the third instruction.
static void RemoveZeroPageStoreReloads(W65C02Generator* g) {
  for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;) {
      TargetInstruction* next =
          inst == block->end_code ? NULL : TargetNext(inst);
      if (TargetOpcodeEq(inst->opcode, W65C02_OP(lda)) &&
          GetAddrMode(inst) == kAddrModeZeroPage &&
          inst->operand[0] != NULL && inst->operand[0]->reg != NULL) {
        TargetInstruction* store =
            PreviousPostAllocationInstruction(inst);
        TargetInstruction* producer =
            store == NULL ? NULL
                          : PreviousPostAllocationInstruction(store);
        while (producer != NULL &&
               TargetOpcodeEq(producer->opcode, W65C02_OP(sta))) {
          producer = PreviousPostAllocationInstruction(producer);
        }
        if (store != NULL && producer != NULL &&
            TargetOpcodeEq(store->opcode, W65C02_OP(sta)) &&
            GetAddrMode(store) == kAddrModeZeroPage &&
            store->operand[0] != NULL && store->operand[0]->reg != NULL &&
            OperandByteAddress(store) == OperandByteAddress(inst) &&
            TargetOpcodeEq(producer->opcode, W65C02_OP(lda))) {
          TargetBasicBlockRemoveInstruction(&g->base, block, inst);
        }
      }
      inst = next;
    }
  }
}

static bool SamePhysicalPointerRegister(TargetInstruction* lhs,
                                        TargetInstruction* rhs) {
  return lhs != NULL && rhs != NULL && lhs->reg != NULL && rhs->reg != NULL &&
         RegisterByteAddress(lhs, 0) == RegisterByteAddress(rhs, 0) &&
         RegisterByteAddress(lhs, 1) == RegisterByteAddress(rhs, 1);
}

// Repeating a one-byte copy is idempotent even when its pointers alias.
// Repeating a small zero operation is always idempotent. Register allocation
// can expose these patterns by assigning distinct address temporaries to the
// same physical pointer registers.
static void RemoveRepeatedSmallMemoryOperations(W65C02Generator* g) {
  for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;) {
      TargetInstruction* next =
          inst == block->end_code ? NULL : TargetNext(inst);
      TargetInstruction* prev = PreviousPostAllocationInstruction(inst);
      bool repeated_copy =
          TargetOpcodeEq(inst->opcode, W65C02_OP(copymem1)) &&
          TargetIntValue(inst->operand[2]) == 1 && prev != NULL &&
          prev->block == block &&
          TargetOpcodeEq(prev->opcode, W65C02_OP(copymem1)) &&
          TargetIntValue(prev->operand[2]) == 1 &&
          SamePhysicalPointerRegister(prev->operand[0], inst->operand[0]) &&
          SamePhysicalPointerRegister(prev->operand[1], inst->operand[1]);
      bool repeated_zero =
          TargetOpcodeEq(inst->opcode, W65C02_OP(zeromem1)) &&
          TargetIntValue(inst->operand[1]) <= 2 && prev != NULL &&
          prev->block == block &&
          TargetOpcodeEq(prev->opcode, W65C02_OP(zeromem1)) &&
          TargetIntValue(prev->operand[1]) ==
              TargetIntValue(inst->operand[1]) &&
          SamePhysicalPointerRegister(prev->operand[0], inst->operand[0]);
      if (repeated_copy || repeated_zero) {
        TargetBasicBlockRemoveInstruction(&g->base, block, inst);
      }
      inst = next;
    }
  }
}

static TargetInstruction* SmallMemoryConstant(W65C02Generator* g,
                                               int value) {
  TargetInstruction* constant =
      TargetGetIntConstant(&g->base, NULL, kTargetType8Bit, value);
  SetAddrMode(constant, kAddrModeImmediate);
  return constant;
}

static void InsertSmallMemoryInstruction(W65C02Generator* g,
                                         TargetBasicBlock* block,
                                         TargetInstruction* before,
                                         W65C02Opcode opcode,
                                         TargetInstruction* operand,
                                         TargetInstruction* offset,
                                         AddressingMode mode) {
  TargetInstruction* inserted =
      operand == NULL
          ? TargetNewInstruction((TargetOpcode)opcode)
          : offset == NULL
                ? TargetNewInstruction1((TargetOpcode)opcode, operand)
                : TargetNewInstruction2((TargetOpcode)opcode, operand, offset);
  SetAddrMode(inserted, mode);
  TargetBasicBlockEmitBefore(&g->base, block, inserted, before);
}

// Inline one-byte operations when they are both smaller and faster than the
// descriptor helpers. At -O1 and above, also inline two-byte operations unless
// -Os requested the smaller shared helper. 65C02 can use (zp) for byte zero;
// NMOS 6502 uses the equivalent (zp),Y form.
static void InlineSmallMemoryOperations(W65C02Generator* g) {
  for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;) {
      TargetInstruction* next =
          inst == block->end_code ? NULL : TargetNext(inst);
      bool is_copy = TargetOpcodeEq(inst->opcode, W65C02_OP(copymem1));
      bool is_zero = TargetOpcodeEq(inst->opcode, W65C02_OP(zeromem1));
      int size = is_copy ? (int)TargetIntValue(inst->operand[2])
                         : is_zero ? (int)TargetIntValue(inst->operand[1]) : 0;
      bool inline_one =
          size == 1 &&
          (!is_zero || Is65c02() || !compiler->optimize_for_size);
      bool inline_two =
          size == 2 && OptLevel1() && !compiler->optimize_for_size;
      if ((is_copy || is_zero) && (inline_one || inline_two)) {
        TargetInstruction* zero = SmallMemoryConstant(g, 0);
        TargetInstruction* one =
            size == 2 ? SmallMemoryConstant(g, 1) : NULL;
        TargetInstruction* dest = inst->operand[0];
        TargetInstruction* src = is_copy ? inst->operand[1] : NULL;
        AddressingMode first_mode =
            Is65c02() ? kAddrModeIndirect : kAddrModeIndirectIndexed;

        if (is_zero) {
          InsertSmallMemoryInstruction(g, block, inst, W65C02_OP(lda), zero,
                                       NULL, kAddrModeImmediate);
        }
        if (!Is65c02()) {
          InsertSmallMemoryInstruction(g, block, inst, W65C02_OP(ldy), zero,
                                       NULL, kAddrModeImmediate);
        }
        if (is_copy) {
          InsertSmallMemoryInstruction(g, block, inst, W65C02_OP(lda), src,
                                       zero, first_mode);
        }
        InsertSmallMemoryInstruction(g, block, inst, W65C02_OP(sta), dest,
                                     zero, first_mode);

        if (size == 2) {
          InsertSmallMemoryInstruction(
              g, block, inst, Is65c02() ? W65C02_OP(ldy) : W65C02_OP(iny),
              Is65c02() ? one : NULL, NULL,
              Is65c02() ? kAddrModeImmediate : kAddrModeImplied);
          if (is_copy) {
            InsertSmallMemoryInstruction(g, block, inst, W65C02_OP(lda), src,
                                         one, kAddrModeIndirectIndexed);
          }
          InsertSmallMemoryInstruction(g, block, inst, W65C02_OP(sta), dest,
                                       one, kAddrModeIndirectIndexed);
        }
        TargetBasicBlockRemoveInstruction(&g->base, block, inst);
      }
      inst = next;
    }
  }
}

static bool IsImmediateY(TargetInstruction* inst, int value) {
  return inst != NULL && TargetOpcodeEq(inst->opcode, W65C02_OP(ldy)) &&
         GetAddrMode(inst) == kAddrModeImmediate &&
         ImmediateValue(inst) == value;
}

static bool IsIndirectCopyMetadata(TargetInstruction* inst) {
  return TargetOpcodeEq(inst->opcode, W65C02_OP(reloadpoint)) ||
         W65C02IsExpression(inst);
}

static TargetInstruction* NextInBlock(TargetInstruction* inst,
                                      TargetBasicBlock* block) {
  TargetInstruction* next = TargetNext(inst);
  while (next != NULL && next->block == block &&
         IsIndirectCopyMetadata(next)) {
    next = TargetNext(next);
  }
  return next != NULL && next->block == block ? next : NULL;
}

static bool IsLoadByte(TargetInstruction* inst, TargetInstruction* address,
                       int byte) {
  if (inst == NULL || !TargetOpcodeEq(inst->opcode, W65C02_OP(lda)) ||
      inst->operand[0] == NULL || inst->operand[0]->reg != address->reg ||
      OperandByteOffset(inst) != byte) {
    return false;
  }
  AddressingMode mode = GetAddrMode(inst);
  return byte == 0
             ? mode == kAddrModeIndirect ||
                   mode == kAddrModeIndirectIndexed
             : mode == kAddrModeIndirectIndexed;
}

static bool IsStoreRegisterByte(TargetInstruction* inst, int address) {
  return inst != NULL && TargetOpcodeEq(inst->opcode, W65C02_OP(sta)) &&
         GetAddrMode(inst) == kAddrModeZeroPage &&
         OperandByteAddress(inst) == address;
}

static TargetInstruction* MatchIndirectLoad(TargetInstruction* start, int size,
                                            TargetInstruction** address,
                                            TargetInstruction** dest) {
  TargetBasicBlock* block = start->block;
  TargetInstruction* inst = start;
  bool has_initial_y = IsImmediateY(inst, 0);
  if (has_initial_y) {
    inst = NextInBlock(inst, block);
  }
  if (inst == NULL || !TargetOpcodeEq(inst->opcode, W65C02_OP(lda)) ||
      inst->operand[0] == NULL || inst->operand[0]->reg == NULL ||
      OperandByteOffset(inst) != 0 ||
      (GetAddrMode(inst) != kAddrModeIndirect &&
       GetAddrMode(inst) != kAddrModeIndirectIndexed)) {
    return NULL;
  }
  if (GetAddrMode(inst) == kAddrModeIndirectIndexed && !has_initial_y) {
    return NULL;
  }

  *address = inst->operand[0];
  inst = NextInBlock(inst, block);
  if (inst == NULL || !TargetOpcodeEq(inst->opcode, W65C02_OP(sta)) ||
      GetAddrMode(inst) != kAddrModeZeroPage ||
      OperandByteOffset(inst) != 0 || inst->operand[0] == NULL ||
      inst->operand[0]->reg == NULL) {
    return NULL;
  }
  *dest = inst->operand[0];
  if (*address == *dest) {
    return NULL;
  }
  int first_dest_address = OperandByteAddress(inst);

  for (int byte = 1; byte < size; byte++) {
    inst = NextInBlock(inst, block);
    if (!IsImmediateY(inst, byte)) {
      return NULL;
    }
    inst = NextInBlock(inst, block);
    if (!IsLoadByte(inst, *address, byte)) {
      return NULL;
    }
    inst = NextInBlock(inst, block);
    if (!IsStoreRegisterByte(inst, first_dest_address + byte)) {
      return NULL;
    }
  }
  return inst;
}

static bool IsLoadRegisterByte(TargetInstruction* inst, int address) {
  return inst != NULL && TargetOpcodeEq(inst->opcode, W65C02_OP(lda)) &&
         GetAddrMode(inst) == kAddrModeZeroPage &&
         OperandByteAddress(inst) == address;
}

static bool IsStoreByte(TargetInstruction* inst, TargetInstruction* address,
                        int byte) {
  if (inst == NULL || !TargetOpcodeEq(inst->opcode, W65C02_OP(sta)) ||
      inst->operand[0] == NULL || inst->operand[0]->reg != address->reg ||
      OperandByteOffset(inst) != byte) {
    return false;
  }
  AddressingMode mode = GetAddrMode(inst);
  return byte == 0
             ? mode == kAddrModeIndirect ||
                   mode == kAddrModeIndirectIndexed
             : mode == kAddrModeIndirectIndexed;
}

static TargetInstruction* MatchIndirectStore(TargetInstruction* start, int size,
                                             TargetInstruction** src,
                                             TargetInstruction** address) {
  TargetBasicBlock* block = start->block;
  TargetInstruction* inst = start;
  if (inst == NULL || !TargetOpcodeEq(inst->opcode, W65C02_OP(lda)) ||
      GetAddrMode(inst) != kAddrModeZeroPage ||
      OperandByteOffset(inst) != 0) {
    return NULL;
  }
  *src = inst->operand[0];
  int first_src_address = OperandByteAddress(inst);

  inst = NextInBlock(inst, block);
  bool has_initial_y = IsImmediateY(inst, 0);
  if (has_initial_y) {
    inst = NextInBlock(inst, block);
  }
  if (inst == NULL || !TargetOpcodeEq(inst->opcode, W65C02_OP(sta)) ||
      inst->operand[0] == NULL || inst->operand[0]->reg == NULL ||
      OperandByteOffset(inst) != 0 ||
      (GetAddrMode(inst) != kAddrModeIndirect &&
       GetAddrMode(inst) != kAddrModeIndirectIndexed)) {
    return NULL;
  }
  if (GetAddrMode(inst) == kAddrModeIndirectIndexed && !has_initial_y) {
    return NULL;
  }
  *address = inst->operand[0];
  if (*src == *address) {
    return NULL;
  }

  for (int byte = 1; byte < size; byte++) {
    inst = NextInBlock(inst, block);
    if (!IsLoadRegisterByte(inst, first_src_address + byte)) {
      return NULL;
    }
    inst = NextInBlock(inst, block);
    if (!IsImmediateY(inst, byte)) {
      return NULL;
    }
    inst = NextInBlock(inst, block);
    if (!IsStoreByte(inst, *address, byte)) {
      return NULL;
    }
  }
  return inst;
}

static TargetInstruction* ReplaceIndirectCopy(
    W65C02Generator* g, TargetInstruction* start, TargetInstruction* end,
    W65C02Opcode opcode, TargetInstruction* x_value,
    TargetInstruction* y_value) {
  TargetBasicBlock* block = start->block;
  TargetInstruction* after = TargetNext(end);
  TargetInstruction* replacement =
      TargetNewInstruction2((TargetOpcode)opcode, x_value, y_value);
  SetAddrMode(replacement, kAddrModeImplied);
  TargetBasicBlockEmitBefore(&g->base, block, replacement, start);

  for (TargetInstruction* inst = start; inst != after;) {
    TargetInstruction* next = TargetNext(inst);
    if (!IsIndirectCopyMetadata(inst)) {
      TargetBasicBlockRemoveInstruction(&g->base, block, inst);
    }
    inst = next;
  }
  return replacement;
}

static bool IsZeroPageByteOp(TargetInstruction* inst, W65C02Opcode op,
                             int byte) {
  return inst != NULL && TargetOpcodeEq(inst->opcode, op) &&
         GetAddrMode(inst) == kAddrModeZeroPage && inst->operand[0] != NULL &&
         OperandByteOffset(inst) == byte;
}

// Fold chained two-byte register copies.  Copying a value through an
// intermediate temporary
//   lda E1 ; sta E2 ; lda E1+1 ; sta E2+1     (E1 -> E2)
//   lda E2 ; sta E3 ; lda E2+1 ; sta E3+1     (E2 -> E3)
// where E2 has no other use collapses to a single E1 -> E3 copy.  The
// register tracker can't see this because it only models the A register,
// not memory-to-memory equality.
static bool FoldExpressionCopyChains(W65C02Generator* g) {
  bool modified = false;
  for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;) {
      TargetInstruction* w[8];
      w[0] = inst;
      bool match = IsZeroPageByteOp(w[0], W65C02_OP(lda), 0);
      for (int j = 1; match && j < 8; j++) {
        w[j] = NextInBlock(w[j - 1], block);
        match = IsZeroPageByteOp(
            w[j], (j & 1) != 0 ? W65C02_OP(sta) : W65C02_OP(lda), (j >> 1) & 1);
      }
      if (match) {
        TargetInstruction* src = w[0]->operand[0];
        TargetInstruction* tmp = w[1]->operand[0];
        TargetInstruction* dest = w[5]->operand[0];
        match = TargetOpcodeEq(tmp->opcode, W65C02_OP(expr2)) && tmp != src &&
                dest != src && dest != tmp && w[2]->operand[0] == src &&
                w[3]->operand[0] == tmp && w[4]->operand[0] == tmp &&
                w[6]->operand[0] == tmp && w[7]->operand[0] == dest;
        // The temporary must be written and read only inside this window,
        // otherwise its stores are still needed.
        if (match) {
          for (size_t u = 0; match && u < tmp->users.length; u++) {
            TargetInstruction* user = tmp->users.value.p[u];
            match = user == w[1] || user == w[3] || user == w[4] ||
                    user == w[6];
          }
        }
        if (match) {
          // Read the source directly and drop the copy into the temporary.
          TargetReplaceOperand(w[4], 0, src);
          TargetReplaceOperand(w[6], 0, src);
          for (int j = 0; j < 4; j++) {
            TargetBasicBlockRemoveInstruction(&g->base, block, w[j]);
          }
          modified = true;
          inst = w[4];
          continue;
        }
      }
      if (inst == block->end_code) {
        break;
      }
      inst = NextInBlock(inst, block);
    }
  }
  return modified;
}

void W65C02CombineIndirectCopies(W65C02Generator* g) {
  // Register allocation repurposes `uses` as a countdown while assigning and
  // freeing registers. Restore the graph reference counts before deleting
  // instructions in this post-allocation peephole pass.
  for (TargetInstruction* inst = TargetFirstInstruction(&g->base);
       inst != NULL; inst = TargetNext(inst)) {
    inst->uses = (int)inst->users.length;
  }

  RemoveRepeatedSmallMemoryOperations(g);
  InlineSmallMemoryOperations(g);
  RemoveRepeatedZeroPageStores(g);
  RemoveZeroPageStoreReloads(g);

  for (size_t i = 0; i < g->base.basic_blocks.length; i++) {
    TargetBasicBlock* block = g->base.basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;) {
      TargetInstruction* address = NULL;
      TargetInstruction* value = NULL;
      TargetInstruction* end = NULL;
      for (int size = 8; size >= 4; size -= 4) {
        end = MatchIndirectLoad(inst, size, &address, &value);
        if (end != NULL) {
          W65C02Opcode opcode = size == 4 ? W65C02_OP(load_indirect4)
                                         : W65C02_OP(load_indirect8);
          TargetInstruction* replacement = ReplaceIndirectCopy(
              g, inst, end, opcode, address, value);
          inst = TargetNext(replacement);
          break;
        }
      }
      if (end != NULL) {
        continue;
      }

      for (int size = 8; size >= 4; size -= 4) {
        end = MatchIndirectStore(inst, size, &value, &address);
        if (end != NULL) {
          W65C02Opcode opcode = size == 4 ? W65C02_OP(store_indirect4)
                                         : W65C02_OP(store_indirect8);
          TargetInstruction* replacement = ReplaceIndirectCopy(
              g, inst, end, opcode, value, address);
          inst = TargetNext(replacement);
          break;
        }
      }
      if (end != NULL) {
        continue;
      }
      if (inst == block->end_code) {
        break;
      }
      inst = TargetNext(inst);
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
static bool CanPoolFromDominator(TargetGenerator* g, TargetBasicBlock* block) {
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

