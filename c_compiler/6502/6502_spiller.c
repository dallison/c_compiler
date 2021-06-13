//
//  6502_spiller.c
//  c_compiler
//
//  Created by David Allison on 3/29/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "6502_spiller.h"
#include "6502_codegen.h"
#include "target_basic_block.h"
#include "map.h"
#include <assert.h>

struct SpillerData {
  _6502Generator* g;
};

static void Load(_6502Generator* g, TargetOpcode op, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) {
  TargetInstruction* inst = TargetNewInstruction2(
                                           op, src, TargetGetIntConstant(&g->base, NULL, kTargetTypeByte, index));
  inst->flags |= (int)kAddrModeZeroPage << 16;
  TargetBasicBlockEmitBefore(&g->base, block, inst, pos);
}

// Operation instructions
#define INST(op)                                                         \
  static void op(_6502Generator* g, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) { \
    Load(g, (TargetOpcode)_6502_OP(op), src, block, pos, index);                                \
  }

INST(lda)
INST(ldx)
INST(ldy)

#undef INST

static void jsr(_6502Generator* g, TargetBasicBlock* block, TargetInstruction* pos, Symbol* func) {
  TargetInstruction* inst = TargetNewInstruction1(
                                           (TargetOpcode)_6502_OP(jsr),  TargetGetSymbol(&g->base, NULL, func));
  inst->flags |= (int)kAddrModeAbsolute << 16;
  TargetBasicBlockEmitBefore(&g->base, block, inst, pos);
}

static TargetInstruction* jsr2(_6502Generator* g, TargetBasicBlock* block, TargetInstruction* pos, Symbol* func) {
  TargetInstruction* inst = TargetNewInstruction1(
                                           (TargetOpcode)_6502_OP(jsr),  TargetGetSymbol(&g->base, NULL, func));
  inst->flags |= (int)kAddrModeAbsolute << 16;
  TargetBasicBlockEmitAfter(&g->base, block, inst, pos);
  return inst;
}

#define INST(op)                                                              \
  static void op##zi(_6502Generator* g, TargetInstruction* reg, TargetBasicBlock* block, TargetInstruction* pos, int offset) { \
    TargetInstruction* inst = TargetNewInstruction2( \
                                             (TargetOpcode)_6502_OP(op), reg, TargetGetIntConstant(&g->base, NULL, kTargetTypeByte, offset)); \
    inst->flags |= (int)kAddrModeZeroPageImmediate << 16; \
    TargetBasicBlockEmitBefore(&g->base, block, inst, pos); \
  }

INST(ldx)

#undef INST

static TargetInstruction* Store(_6502Generator* g, TargetOpcode op, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) {
  TargetInstruction* inst = TargetNewInstruction2(
                                           op, src, TargetGetIntConstant(&g->base, NULL, kTargetTypeByte, index));
  inst->flags |= (int)kAddrModeZeroPage << 16;
  TargetBasicBlockEmitAfter(&g->base, block, inst, pos);
  return inst;
}

// Operation instructions
#define INST(op)                                                         \
  static TargetInstruction* op(_6502Generator* g, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) { \
    return Store(g, (TargetOpcode)_6502_OP(op), src, block, pos, index);                                \
  }

INST(sta)
INST(stx)
INST(sty)

#undef INST

#define INST(op)                                                              \
  static TargetInstruction* op##zi(_6502Generator* g, TargetInstruction* reg, TargetBasicBlock* block, TargetInstruction* pos, int offset) { \
    TargetInstruction* inst = TargetNewInstruction2( \
                                             (TargetOpcode)_6502_OP(op), reg, TargetGetIntConstant(&g->base, NULL, kTargetTypeByte, offset)); \
    inst->flags |= (int)kAddrModeZeroPageImmediate << 16; \
    TargetBasicBlockEmitAfter(&g->base, block, inst, pos); \
    return inst; \
  }

INST(stx)

#undef INST
static void PushExpression(_6502Generator* g, TargetInstruction* inst, TargetBasicBlock* block, TargetInstruction* pos) {
  switch ((_6502Opcode)inst->opcode) {
  case _6502_OP(expr1):
    lda(g, inst, block, pos, 0);
    jsr(g, block, pos, g->pusha);
    break;
  case _6502_OP(expr2):
    ldx(g, inst, block, pos, 0);
    ldy(g, inst, block, pos, 1);
    jsr(g, block, pos, g->pushxy);
    break;

  case _6502_OP(expr4):
    ldxzi(g, inst, block, pos, 0);
    jsr(g, block, pos, g->push4);
    break;

  case _6502_OP(expr8):
    ldxzi(g, inst, block, pos, 0);
    jsr(g, block, pos, g->push8);
    break;
  default:
    abort();
  }
}

static TargetInstruction* PullExpression(_6502Generator* g, TargetInstruction* inst, TargetBasicBlock* block, TargetInstruction* pos) {
  switch ((_6502Opcode)inst->opcode) {
  case _6502_OP(expr1):
    pos = jsr2(g, block, pos, g->pulla);
    return sta(g, inst, block, pos, 0);
  case _6502_OP(expr2):
    pos = jsr2(g, block, pos, g->pullxy);
    pos = stx(g, inst, block, pos, 0);
    return sty(g, inst, block, pos, 1);

  case _6502_OP(expr4):
    pos = jsr2(g, block, pos, g->pull4);
    return stxzi(g, inst, block, pos, 0);

  case _6502_OP(expr8):
    pos = jsr2(g, block, pos, g->pull8);
    return stxzi(g, inst, block, pos, 0);
  default:
    abort();
  }
  return NULL;
}

// If the basic block contains a call, look for all inputs that are also
// in the output set and push each one onto the stack at the start of
// the block.  Pop them in reverse order at the end of the block.
// This saves the values of expressions held in 'registers' over calls.
static void SpillExpressions(TargetBasicBlock* block, void* data) {
   if (!block->contains_call) {
    return;
  }
  struct SpillerData* spill_data = data;
   _6502Generator* g = spill_data->g;
  
  Vector pushed_instructions = {0};   // Pushed instructions.
  BitSet pushes = {0};                // Set to detect already pushed.
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* input = block->inputs.value.p[i];
    if (BitSetContains(&block->output_ids, input->id)) {
      if (!BitSetContains(&pushes, input->id)) {
        VectorAppend(&pushed_instructions, input);
        BitSetInsert(&pushes, input->id);
      }
    }
  }
  
  // Push all instructions before the first instruction in the block.
  TargetInstruction* first = block->code;
  while (first != block->end_code && _6502IsLabel(first)) {
    first = TargetNext(first);
  }
  
  for (size_t i = 0; i < pushed_instructions.length; i++) {
    TargetInstruction* inst = pushed_instructions.value.p[i];
    PushExpression(g, inst, block, first);
  }
  
  // Pull all instructions after the last instruction in the block.
  TargetInstruction* last = block->end_code;
  while (last != block->code && _6502IsBranch(last)) {
    last = TargetPrev(last);
  }
  
  for (size_t i = pushed_instructions.length; i > 0; i--) {
    TargetInstruction* inst = pushed_instructions.value.p[i-1];
    last = PullExpression(g, inst, block, last);
  }
  VectorDestruct(&pushed_instructions);
  BitSetDestruct(&pushes);
}


void _6502SpillExpressions(_6502Generator* g) {
  struct SpillerData data = {g};
  TargetTraverseDominatorTree(&g->base, SpillExpressions, kTraversePreOrder, &data);
}

struct PoolerData {
  _6502Generator* g;
};

static bool IsVariable(TargetInstruction* inst) {
  switch ((_6502Opcode)inst->opcode) {
    case _6502_OP(var_addr):
    case _6502_OP(var_addrb):
    case _6502_OP(arg_addr):
    case _6502_OP(arg_addrb):
    case _6502_OP(var_value1):
    case _6502_OP(var_value2):
    case _6502_OP(var_value4):
    case _6502_OP(var_value8):
    case _6502_OP(var_value1b):
    case _6502_OP(var_value2b):
    case _6502_OP(var_value4b):
    case _6502_OP(var_value8b):
    case _6502_OP(arg_value1):
    case _6502_OP(arg_value2):
    case _6502_OP(arg_value4):
    case _6502_OP(arg_value8):
    case _6502_OP(arg_value1b):
    case _6502_OP(arg_value2b):
    case _6502_OP(arg_value4b):
    case _6502_OP(arg_value8b):
      return true;
    default:
      return false;
  }
}

static void PoolVariables(TargetBasicBlock* block, void* data) {
  struct PoolerData* pool_data = data;
    _6502Generator* g = pool_data->g;
  Map* dominator_variables = NULL;
  if (block->idom != NULL) {
    // If we have an immediate dominator we propagate the
    // variables from it to this node.
    dominator_variables = block->idom->cookie;
  }

  Map* vars = NULL;
  if (block->idom != NULL && block->idom->dominatees.length == 1) {
    // This block is the only one dominated by the dominator so we
    // can just reuse the value set from the dominator.
    vars = dominator_variables;
    block->idom->cookie = NULL;  // This is no longer valid.
    block->cookie = vars;
  } else {
    // There is more than one block that is dominated by my dominator.
    // We need to copy the variables from the dominator.
    vars = NewMapForPointerKeys();
    block->cookie = vars;
    if (dominator_variables != NULL) {
      MapCopy(vars, dominator_variables);
    }
  }
  
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = block->code; inst != NULL &&
       block->end_code != NULL &&
       TargetPrev(inst) != block->end_code; inst = next) {
    next = block->end_code == NULL ? NULL : TargetNext(inst);

    if (IsVariable(inst)) {
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      MapKeyType key = {.p = var};
      TargetInstruction* pooled = MapFind(vars, key);
      if (pooled == NULL) {
        MapKeyValue kv = {.key.p = var, .value.p = result};
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

void _6502PoolVariables(_6502Generator* g) {
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
