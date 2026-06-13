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
  W65C02Generator* g;
};

static void Load(W65C02Generator* g, TargetOpcode op, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) {
  TargetInstruction* inst = TargetNewInstruction2(
                                           op, src, TargetGetIntConstant(&g->base, NULL, kTargetType8Bit, index));
  inst->flags |= (int)kAddrModeZeroPage << 16;
  TargetBasicBlockEmitBefore(&g->base, block, inst, pos);
}

// Operation instructions
#define INST(op)                                                         \
  static void op(W65C02Generator* g, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) { \
    Load(g, (TargetOpcode)W65C02_OP(op), src, block, pos, index);                                \
  }

INST(lda)
INST(ldx)
INST(ldy)

#undef INST

static void jsr(W65C02Generator* g, TargetBasicBlock* block, TargetInstruction* pos, Symbol* func) {
  TargetInstruction* inst = TargetNewInstruction1(
                                           (TargetOpcode)W65C02_OP(jsr),  TargetGetSymbol(&g->base, NULL, func));
  inst->flags |= (int)kAddrModeAbsolute << 16;
  TargetBasicBlockEmitBefore(&g->base, block, inst, pos);
}

static TargetInstruction* jsr2(W65C02Generator* g, TargetBasicBlock* block, TargetInstruction* pos, Symbol* func) {
  TargetInstruction* inst = TargetNewInstruction1(
                                           (TargetOpcode)W65C02_OP(jsr),  TargetGetSymbol(&g->base, NULL, func));
  inst->flags |= (int)kAddrModeAbsolute << 16;
  TargetBasicBlockEmitAfter(&g->base, block, inst, pos);
  return inst;
}

#define INST(op)                                                              \
  static void op##zi(W65C02Generator* g, TargetInstruction* reg, TargetBasicBlock* block, TargetInstruction* pos, int offset) { \
    TargetInstruction* inst = TargetNewInstruction2( \
                                             (TargetOpcode)W65C02_OP(op), reg, TargetGetIntConstant(&g->base, NULL, kTargetType8Bit, offset)); \
    inst->flags |= (int)kAddrModeZeroPageImmediate << 16; \
    TargetBasicBlockEmitBefore(&g->base, block, inst, pos); \
  }

INST(ldx)

#undef INST

static TargetInstruction* Store(W65C02Generator* g, TargetOpcode op, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) {
  TargetInstruction* inst = TargetNewInstruction2(
                                           op, src, TargetGetIntConstant(&g->base, NULL, kTargetType8Bit, index));
  inst->flags |= (int)kAddrModeZeroPage << 16;
  TargetBasicBlockEmitAfter(&g->base, block, inst, pos);
  return inst;
}

// Operation instructions
#define INST(op)                                                         \
  static TargetInstruction* op(W65C02Generator* g, TargetInstruction* src, TargetBasicBlock* block, TargetInstruction* pos, int index) { \
    return Store(g, (TargetOpcode)W65C02_OP(op), src, block, pos, index);                                \
  }

INST(sta)
INST(stx)
INST(sty)

#undef INST

#define INST(op)                                                              \
  static TargetInstruction* op##zi(W65C02Generator* g, TargetInstruction* reg, TargetBasicBlock* block, TargetInstruction* pos, int offset) { \
    TargetInstruction* inst = TargetNewInstruction2( \
                                             (TargetOpcode)W65C02_OP(op), reg, TargetGetIntConstant(&g->base, NULL, kTargetType8Bit, offset)); \
    inst->flags |= (int)kAddrModeZeroPageImmediate << 16; \
    TargetBasicBlockEmitAfter(&g->base, block, inst, pos); \
    return inst; \
  }

INST(stx)

#undef INST
static void PushExpression(W65C02Generator* g, TargetInstruction* inst, TargetBasicBlock* block, TargetInstruction* pos) {
  switch ((W65C02Opcode)inst->opcode) {
  case W65C02_OP(expr1):
    lda(g, inst, block, pos, 0);
    jsr(g, block, pos, g->pusha);
    break;
  case W65C02_OP(expr2):
    ldx(g, inst, block, pos, 0);
    ldy(g, inst, block, pos, 1);
    jsr(g, block, pos, g->pushxy);
    break;

  case W65C02_OP(expr4):
    ldxzi(g, inst, block, pos, 0);
    jsr(g, block, pos, g->push4);
    break;

  case W65C02_OP(expr8):
    ldxzi(g, inst, block, pos, 0);
    jsr(g, block, pos, g->push8);
    break;
  default:
    abort();
  }
}

static TargetInstruction* PullExpression(W65C02Generator* g, TargetInstruction* inst, TargetBasicBlock* block, TargetInstruction* pos) {
  switch ((W65C02Opcode)inst->opcode) {
  case W65C02_OP(expr1):
    pos = jsr2(g, block, pos, g->pulla);
    return sta(g, inst, block, pos, 0);
  case W65C02_OP(expr2):
    pos = jsr2(g, block, pos, g->pullxy);
    pos = stx(g, inst, block, pos, 0);
    return sty(g, inst, block, pos, 1);

  case W65C02_OP(expr4):
  case W65C02_OP(exprf):
    pos = jsr2(g, block, pos, g->pull4);
    return stxzi(g, inst, block, pos, 0);

  case W65C02_OP(expr8):
  case W65C02_OP(exprd):
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
static COMPILER_UNUSED void SpillExpressions(TargetBasicBlock* block, void* data) {
   if (!block->contains_call) {
    return;
  }
  struct SpillerData* spill_data = data;
   W65C02Generator* g = spill_data->g;
  
  // Get set of nodes written to (used as destination) in this block.  If
  // these appear in the inputs or outputs we don't save them as they are
  // generated by the block.
  BitSet writes = {0};                // Nodes written to in this block.
  TargetInstruction* inst = block->code;
  while (inst != NULL && TargetPrev(inst) != block->end_code) {
    if (inst->dest != NULL) {
      BitSetInsert(&writes, inst->dest->id);
    }
    inst = TargetNext(inst);
  }
  
  Vector pushed_instructions = {0};   // Pushed instructions.
  BitSet pushes = {0};                // Set to detect already pushed.
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* input = block->inputs.value.p[i];
    if (BitSetContains(&block->output_ids, input->id)) {
      if (!BitSetContains(&pushes, input->id) &&
          !BitSetContains(&writes, input->id)) {
        VectorAppend(&pushed_instructions, input);
        BitSetInsert(&pushes, input->id);
      }
    }
  }
  
  // Push all instructions before the first instruction in the block.
  TargetInstruction* first = block->code;
  while (first != block->end_code && W65C02IsLabel(first)) {
    first = TargetNext(first);
  }
  
  for (size_t i = 0; i < pushed_instructions.length; i++) {
    TargetInstruction* inst = pushed_instructions.value.p[i];
    PushExpression(g, inst, block, first);
  }
  
  // Pull all instructions after the last instruction in the block.
  TargetInstruction* last = block->end_code;
  while (last != block->code && W65C02IsBranch(last)) {
    last = TargetPrev(last);
  }
  
  for (size_t i = pushed_instructions.length; i > 0; i--) {
    TargetInstruction* inst = pushed_instructions.value.p[i-1];
    last = PullExpression(g, inst, block, last);
  }
  VectorDestruct(&pushed_instructions);
  BitSetDestruct(&pushes);
  BitSetDestruct(&writes);
}


void W65C02SpillExpressions(W65C02Generator* g) {
  // struct SpillerData data = {g};
  //TargetTraverseDominatorTree(&g->base, SpillExpressions, kTraversePreOrder, &data);
}

