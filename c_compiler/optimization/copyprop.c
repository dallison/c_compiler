#include "copyprop.h"

#include "basic_block.h"
#include "ir.h"
#include "type_compare.h"

static bool IsMoveOpcode(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
      return true;
    default:
      return false;
  }
}

static bool CanPropagateMove(IRNode* inst) {
  if (!IsMoveOpcode(inst->opcode) || inst->inputs.length != 1 ||
      inst->outputs.length != 1 || inst->dest != NULL || inst->flags != 0 ||
      inst->type == NULL || TypeIsVolatile(inst->type)) {
    return false;
  }

  IRNode* source = inst->inputs.value.p[0];
  IRNode* user = inst->outputs.value.p[0];
  if (source == NULL || user == NULL || source->type == NULL ||
      TypeIsVolatile(source->type) || !TypeEqual(source->type, inst->type) ||
      IRHasSideEffects(source)) {
    return false;
  }

  // Keep this first version pressure-neutral: it only removes a copy inside
  // one block and never lengthens the source's cross-block live range.
  return source->block == inst->block && user->block == inst->block;
}

void CopyPropagationOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    IRNode* next = NULL;
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = next) {
      next = IRNext(inst);
      if (!CanPropagateMove(inst)) {
        continue;
      }
      IRNode* source = inst->inputs.value.p[0];
      GeneratorReplaceInstruction(gen, inst, source);
      BasicBlockRemoveInstruction(gen, block, inst);
    }
  }
}
