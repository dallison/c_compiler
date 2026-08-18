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
  return source->block == inst->block && user->block == inst->block &&
         !IRCheckpointBetween(source, inst) &&
         !IRCheckpointBetween(inst, user);
}

static bool IsSafeSSAVariable(IRNode* node) {
  if (node == NULL || node->opcode != IR_OP(ssavar)) {
    return false;
  }
  IRVariable* variable = (IRVariable*)node;
  Symbol* symbol = variable->symbol;
  return symbol != NULL && !symbol->flags.address_taken &&
         !TypeIsVolatile(symbol->type) && !TypeIsArray(symbol->type) &&
         (symbol->flags.is_argument || symbol->flags.is_temp ||
          (symbol->flags.is_local &&
           !StorageIs(symbol->storage, STO(static))));
}

static bool InstructionPrecedesInBlock(IRNode* before, IRNode* after) {
  for (IRNode* inst = before; inst != NULL && inst->block == before->block;
       inst = IRNext(inst)) {
    if (inst == after) {
      return true;
    }
  }
  return false;
}

// Inlined member functions can contain distinct Symbol objects named `this`
// whose non-canonical SSA versions still depend on an earlier assignment.
// Restrict memory-copy forwarding to an unambiguous single version until those
// field-definition phis are represented explicitly.
static bool HasSingleNamedSSAVersion(Generator* gen, IRNode* variable) {
  Symbol* symbol = ((IRVariable*)variable)->symbol;
  size_t versions = 0;
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (inst->opcode != IR_OP(ssavar) && inst->opcode != IR_OP(phi)) {
      continue;
    }
    Symbol* candidate = ((IRVariable*)inst)->symbol;
    if (candidate != NULL &&
        StringEqualString(&candidate->name, &symbol->name)) {
      versions++;
      if (versions > 1) {
        return false;
      }
    }
  }
  return versions == 1;
}

// SSA gives each scalar variable definition a distinct ssavar.  A store to
// that virtual name followed by a load from the same name is therefore a copy,
// not an observable memory round trip.  Keep this first form local to one block
// so forwarding does not lengthen a live range across control flow.
static bool PropagateSSALoad(Generator* gen, BasicBlock* block, IRNode* load) {
  if (load->opcode != IR_OP(loada) || load->inputs.length != 1 ||
      load->dest != NULL || load->type == NULL || TypeIsVolatile(load->type)) {
    return false;
  }

  IRNode* variable = load->inputs.value.p[0];
  if (!IsSafeSSAVariable(variable) ||
      !HasSingleNamedSSAVersion(gen, variable)) {
    return false;
  }

  IRNode* store = NULL;
  for (size_t i = 0; i < variable->outputs.length; i++) {
    IRNode* candidate = variable->outputs.value.p[i];
    if (candidate->opcode == IR_OP(storea) &&
        candidate->inputs.length == 2 &&
        candidate->inputs.value.p[0] == variable) {
      if (store != NULL) {
        return false;
      }
      store = candidate;
    }
  }
  if (store == NULL || store->block != block || load->block != block ||
      !InstructionPrecedesInBlock(store, load) ||
      IRCheckpointBetween(store, load)) {
    return false;
  }

  IRNode* source = store->inputs.value.p[1];
  if (source == NULL || source->block != block || source->type == NULL ||
      TypeIsVolatile(source->type) || !TypeEqual(source->type, load->type) ||
      IRHasSideEffects(source) || IRCheckpointBetween(source, load)) {
    return false;
  }

  GeneratorReplaceInstruction(gen, load, source);
  BasicBlockRemoveInstruction(gen, block, load);
  return true;
}

void CopyPropagationOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    IRNode* next = NULL;
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = next) {
      next = IRNext(inst);
      if (PropagateSSALoad(gen, block, inst)) {
        continue;
      }
      if (!CanPropagateMove(inst)) {
        continue;
      }
      IRNode* source = inst->inputs.value.p[0];
      GeneratorReplaceInstruction(gen, inst, source);
      BasicBlockRemoveInstruction(gen, block, inst);
    }
  }
}
