//
//  codemotion.c
//  c_compiler_library
//
//  Created by David Allison on 7/6/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "codemotion.h"
#include <assert.h>

static void Trap() {}
static void TrapInstructionMove(IRNode* node) {
  if (node->id == 561) {
    Trap();
  }
}

// Given an instruction that has no inputs in this block, move it
// to a dominator block.  The block chosen is the one closest to the
// current block (looking up the tree) that contains one of the inputs.
static void HoistInstruction(Generator* gen, IRNode* inst) {
  TrapInstructionMove(inst);
  BasicBlock* block = inst->block->idom;
  while (block != NULL) {
    IRNode* first_input = NULL;
    for (size_t i = 0; i < inst->inputs.length; i++) {
      IRNode* input = inst->inputs.value.p[i];
      if (block == input->block) {
        first_input = input;
        break;
      }
    }
    if (first_input != NULL) {
      IRNode* last_in_block = first_input->block->end_code;
      // If the block ends in a control-transfer instruction (any branch --
      // conditional bfalse/btrue, unconditional bra, computed cbra -- or a
      // return) the hoisted instruction must go BEFORE it: code placed after a
      // conditional branch lands on the fall-through path only, so a value used
      // on the taken path (e.g. the hoisted argument of a call further down the
      // dominator tree) is never computed there.  Otherwise append it at the
      // end of the block.
      if (IRIsBranch(last_in_block) || IRIsReturn(last_in_block)) {
        BasicBlockMoveInstructionBefore(gen, inst, last_in_block);
      } else {
        BasicBlockMoveInstructionAfter(gen, inst, last_in_block);
      }
      return;
    }
    block = block->idom;
  }
  assert(false);
}

typedef struct CodeMotionContext {
  Generator* gen;
  // True if any block inside a loop contains a call or a memory write.  When
  // set we must not hoist memory LOADS out of loops: a load is only loop
  // invariant if nothing in the loop can change the loaded location, and we
  // have no alias analysis to prove that.  (A call may modify any global, and a
  // store may alias the loaded address.)  Pure register expressions remain
  // hoistable.
  bool loop_writes_memory;
} CodeMotionContext;

// Pre-pass: detect whether any loop block performs a call or a memory write.
static void ScanLoopWrites(BasicBlock* block, void* data) {
  CodeMotionContext* ctx = data;
  if (block->loop_nesting == 0 || ctx->loop_writes_memory) {
    return;
  }
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (IRIsCall(inst) || IRIsStore(inst)) {
      ctx->loop_writes_memory = true;
      return;
    }
  }
}

// Given a basic block, check that it's in a loop and if so,
// look for instructions that have no side effects but have all their inputs
// coming from a dominator block (not this block).  For each of these,
// move them to the closest dominator block that satisfies all their
// inputs.
void PerformCodeMotion(BasicBlock* block, void* data) {
  CodeMotionContext* ctx = data;
  Generator* gen = ctx->gen;
  if (block->loop_nesting == 0) {
    return;
  }
  IRNode* next = NULL;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = next) {
    next = IRNext(inst);
    bool is_candidate = (IRIsLoad(inst) || IRIsExpression(inst)) &&
            !IRIsStore(inst) &&
            !IRIsCall(inst) &&
            !IRIsVariable(inst) && inst->opcode != IR_OP(literalref) &&
            inst->inputs.length > 0;
    // A memory load may only be hoisted out of a loop if nothing in the loop
    // writes memory; otherwise the hoisted value goes stale (e.g. a global read
    // in a loop that also calls a function which updates that global).
    if (is_candidate && IRIsLoad(inst) && ctx->loop_writes_memory) {
      is_candidate = false;
    }
    if (is_candidate) {
      for (size_t i = 0; i < inst->inputs.length; i++) {
        IRNode* input = inst->inputs.value.p[i];
        // If the input comes from this block then this is not a
        // code motion candidate.  If it comes from another block then
        // it can only come from a dominator block.
        if (input->block == block) {
          is_candidate = false;
          break;
        }
      }
      // Moving to another block with the same loop nesting isn't useful
      // and just increases register pressure, so eliminate that.
      if (is_candidate) {
        // If all of the inputs come from a block with a different
        // loop nesting it's worth moving it.
        is_candidate = true;
        for (size_t i = 0; i < inst->inputs.length; i++) {
           IRNode* input = inst->inputs.value.p[i];
          if (input->block->loop_nesting == block->loop_nesting) {
            is_candidate = false;
            break;
          }
        }
      }
      if (is_candidate) {
        // printf("Found loop invariant instruction $%d\n", inst->id);
        HoistInstruction(gen, inst);
      }
    }
  }
  
}

void CodeMotionOptimization(Generator* gen) {
  CodeMotionContext ctx = {gen, false};
  BasicBlockTraverseDominatorTree(gen, gen->entry_block, ScanLoopWrites,
                                  kTraversePreOrder, &ctx);
  BasicBlockTraverseDominatorTree(gen, gen->entry_block, PerformCodeMotion,
                                  kTraversePreOrder, &ctx);
}
