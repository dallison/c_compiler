//
//  optimizer.c
//  c_compiler_library
//
//  Created by David Allison on 6/12/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "optimizer.h"
#include "basic_block.h"

// Is the node an integer constant with the value given?
static bool IsIntConstantWithValue(IRNode* node, int value) {
  if (node == NULL) {
    return false;
  }
  if (!IRIsConst(node)) {
    return false;
  }
  if (node->type == NULL || !TypeIsIntegral(node->type)) {
    return false;
  }
  IRConstant* c = (IRConstant*)node;
  return c->value.ivalue == value;
}

// Is the node an integer constant power of 2 up to maxbits bits?
// A power of two only has one bit set to 1, all the others are zero.
static bool IsIntConstantPowerOf2(IRNode* node, int maxbits) {
  if (node == NULL) {
    return false;
  }
  if (!IRIsConst(node)) {
    return false;
  }
  if (node->type == NULL || !TypeIsIntegral(node->type)) {
    return false;
  }
  IRConstant* c = (IRConstant*)node;
  int mask = (1 << maxbits) - 1;
  int64_t value = c->value.ivalue & mask;
  return value != 0 &&
      c->value.ivalue >> maxbits == 0 &&
      (value & (value - 1)) == 0;
}

// The node is an integer power of 2.  What is its log?  This is the
// number of zero bits up to the first one bit.
// For example, the value 128 (0x80) is 7.
// This comes from:
// https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
static int LogBase2(IRNode* node) {
  IRConstant* c = (IRConstant*)node;
  int64_t value = c->value.ivalue;
  static const int MultiplyDeBruijnBitPosition2[32] =
  {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return MultiplyDeBruijnBitPosition2[(uint32_t)(value * 0x077CB531U) >> 27];

#if 0
  int zerocount = 0;
  int64_t value = c->value.ivalue;
  for (int i = 0; i < 64; i++) {
    if ((value & (1 << i)) == 0) {
      zerocount++;
    } else {
      break;
    }
  }
  return zerocount;
#endif
}


// The node is an integer power of 2, generate a bitmask suitable for
// ANDing.  This is simply the number minus 1.
static int64_t BitMask(IRNode* node) {
  IRConstant* c = (IRConstant*)node;
  int64_t value = c->value.ivalue;
  return value - 1;
}

// Perform strength reduction on the given node (in the given basic block). This
// looks at the operation and its operands.  If it can be simplified into
// something that is cheaper to execute, it is replaced by the better
// instruction.  For exmaple, multiplication is an expensive operation on most
// processors so if we can convert it to a shift the program might run faster.
static void ReduceNodeStrength(Generator* gen, BasicBlock* block,
                               IRNode* node) {
  // A non-NULL dest means this node's result must be written into a specific
  // merge temporary (e.g. the shared tmp for the arms of `a || b`, `a && b` or
  // a `?:`).  The simplifications below replace `node` with one of its inputs,
  // which silently drops that dest linkage -- the merge tmp would then never
  // receive the value (e.g. `x || (0 + f())` folds `0 + f()` to `f()` and loses
  // the assignment of f()'s result into the `||` result tmp).  Skip strength
  // reduction in that case; it only costs a minor, rare optimization.
  if (node->dest != NULL) {
    return;
  }
  switch (node->opcode) {
    case IR_OP(addi):
      // Adding constant 0 is a nop.
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      }
      break;
    case IR_OP(subi):
      // Subtracting zero is a nop.
      if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Subtracting from zero is a negation.
        if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
          node->opcode = IR_OP(negi);
          IRRemoveInput(node, 0);
        }
      }
      break;
    case IR_OP(muli):
      // Multiply by zero is zero.
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[0], 1)) {
        // Multiply by 1 is a nop.
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 1)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Multiply by a power of 2 less than the word width is a left shift.
        int maxbits = node->type->size * 8;
        if (IsIntConstantPowerOf2(node->inputs.value.p[0], maxbits)) {
          // Left is power of 2, convert to shift with left input moved to
          // the right and replaced by its log (base 2).
          node->opcode = IR_OP(lsli);
          IRNode* c = (IRNode*)node->inputs.value.p[0];
          IRNode* log2 = GeneratorGetIntConstant(
              gen, c->type, LogBase2(node->inputs.value.p[0]));
          IRRemoveInput(node, 0);
          IRAddInput(node, log2, false);
        } else if (IsIntConstantPowerOf2(node->inputs.value.p[1], maxbits)) {
          // Left is power of 2, convert to shift replaced by its log (base 2).
          node->opcode = IR_OP(lsli);
          IRNode* c = (IRNode*)node->inputs.value.p[1];
          IRNode* log2 = GeneratorGetIntConstant(
              gen, c->type, LogBase2(node->inputs.value.p[1]));
          IRReplaceInput(node, 1, log2);
        }
      }
      break;

    case IR_OP(divi):
      if (IsIntConstantWithValue(node->inputs.value.p[1], 1)) {
        // Division by 1 is nop.
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        // Division of zero by anything is zero.
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Division by a power of 2 less than the word width is a right shift.
        int maxbits = node->type->size * 8;
        if (IsIntConstantPowerOf2(node->inputs.value.p[1], maxbits)) {
          // Left is power of 2, convert to shift with left input moved to
          // the right and replaced by its log (base 2).
          node->opcode = TypeIsUnsigned(node->type) ? IR_OP(lsri) : IR_OP(asri);
          IRNode* c = (IRNode*)node->inputs.value.p[1];
          IRNode* log2 = GeneratorGetIntConstant(
              gen, c->type, LogBase2(node->inputs.value.p[1]));
          IRReplaceInput(node, 1, log2);
        }
      }
      break;

    case IR_OP(modi):
      // Modulus of zero by anything is zero.
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Modulus with a power of 2 is an AND (with the value - 1)
        // For example, x % 8 is the same as x & 0x7
        int maxbits = node->type->size * 8;
        if (IsIntConstantPowerOf2(node->inputs.value.p[1], maxbits)) {
          // Left is power of 2, convert to AND with mask.
          node->opcode = IR_OP(andi);
          IRNode* c = (IRNode*)node->inputs.value.p[1];
          IRNode* mask = GeneratorGetIntConstant(gen, c->type, BitMask(c));
          IRReplaceInput(node, 1, mask);
        }
      }
      break;

    default:
      break;
  }
}

// Go through all the code in all basic blocks seeing if we can make
// instructions cheaper to execute.
void StrengthReductionOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    IRNode* next;
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = next) {
      next = IRNext(inst);
      ReduceNodeStrength(gen, block, inst);
    }
  }
}


// Instructions that may legitimately appear after a call without preventing a
// tail call: they are just the return bookkeeping (labels, the procedure
// leave/return, branches to the exit) and do not run any user code.
static bool IsTrivialReturnInstruction(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(label):
    case IR_OP(leave):
    case IR_OP(ret):
    case IR_OP(bra):
    case IR_OP(nop):
    case IR_OP(enter):
      return true;
    default:
      return false;
  }
}

// The last IR_OP(calla) in a block whose only out edge goes
// to a block marked with return_block is a tail call.
// Also, if the block is a return block the last call is a
// tail call.
void FindTailCalls(BasicBlock* block, void* data) {
  Generator* gen = data;
  if (!block->return_block) {
    if (block->out_edges.length != 1) {
      return;
    }
    BlockId next_id = block->out_edges.value.w[0];
    BasicBlock* next = VectorGet(&gen->basic_blocks, next_id);
    if (!next->return_block) {
      return;
    }
    // The successor return block runs *after* this block's call.  If it does
    // any real work (e.g. `printf(); fred++;` where the post-increment of the
    // static `fred` lands in the return block) the call is not in tail position
    // and turning it into a jump would drop that work.
    for (IRNode* inst = BasicBlockBegin(next);
         !BasicBlockIsEmpty(next) && inst != BasicBlockEnd(next);
         inst = IRNext(inst)) {
      if (!IsTrivialReturnInstruction(inst)) {
        return;
      }
    }
  }
  // Look for the last call in the block.  If we see a result instruction
  // after the call it's not a tail call.
  for (IRNode* inst = BasicBlockRBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockREnd(block);
       inst = IRPrev(inst)) {
    if (IRIsResult(inst)) {
      // Scalar result instruction.
      IRNode* value = inst->inputs.value.p[0];
      if (value->opcode == IR_OP(calla)) {
        value->flags |= kIRTailCall;
      }
      return;
    }
    if (inst->opcode == IR_OP(memcpy)) {
      IRNode* dest = inst->inputs.value.p[0];
      if (dest->opcode == IR_OP(structreturn)) {
        // Struct result instruction.  Can't do a tail call for this.
        return;
      }
    }
    if (inst->opcode == IR_OP(calla)) {
      inst->flags |= kIRTailCall;
      return;
    }
    // Any other instruction sitting between the call and the return means there
    // is real work after the call, so it is not in tail position.
    if (!IsTrivialReturnInstruction(inst)) {
      return;
    }
  }
}

void TailCallOptimization(Generator* gen) {
  BasicBlockTraverseDominatorTree(gen, gen->entry_block,
                                  FindTailCalls, kTraversePostOrder, gen);
}

