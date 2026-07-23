#include "dce.h"

#include "basic_block.h"
#include "ir.h"

static bool IsDeadExpression(IRNode* inst) {
  if (!IRIsExpression(inst) || IRIsConstant(inst) || IRIsVariable(inst) ||
      inst->opcode == IR_OP(tmp)) {
    return false;
  }
  if (IRHasSideEffects(inst) || inst->dest != NULL) {
    return false;
  }
  // Some backends use the number and shape of a call's immediate IR users to
  // choose result materialization.  Until call results have an explicit value
  // node, retain otherwise-dead conversions/copies directly fed by a
  // side-effecting instruction.
  for (size_t i = 0; i < inst->inputs.length; i++) {
    if (IRHasSideEffects(inst->inputs.value.p[i])) {
      return false;
    }
  }
  return inst->outputs.length == 0;
}

void DeadCodeEliminationOptimization(Generator* gen) {
  // Removing one expression can make its inputs dead.  Iterating to a fixed
  // point is simple here and keeps the pass independent of block-local
  // liveness data because IR outputs already contain cross-block uses.
  bool changed;
  do {
    changed = false;
    for (size_t i = 0; i < gen->basic_blocks.length; i++) {
      BasicBlock* block = gen->basic_blocks.value.p[i];
      IRNode* next = NULL;
      for (IRNode* inst = BasicBlockBegin(block);
           !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
           inst = next) {
        next = IRNext(inst);
        if (IsDeadExpression(inst)) {
          BasicBlockRemoveInstruction(gen, block, inst);
          changed = true;
        }
      }
    }
  } while (changed);
}
