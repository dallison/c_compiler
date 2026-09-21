#include "dce.h"

#include "basic_block.h"
#include "ir.h"
#include "type_compare.h"

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

// Store to a non-escaped local that is never loaded.  SROA and store-to-load
// forwarding leave these behind; they would otherwise force a stack slot.
// Increment/decrement is a read-modify-write of the same object, but SSA
// gives the RMW a fresh ssavar, so dest->outputs of the store's version may
// contain only stores.  Deleting that store leaves the increment operating on
// uninitialized storage (00032.c `p = &arr[0]; *(p++)`).
static bool SymbolHasIncDec(Generator* gen, Symbol* symbol) {
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (IRIsVarDef(inst) && IRIsStore(inst) && !IRIsStoreOnly(inst) &&
        inst->var.def == symbol) {
      return true;
    }
  }
  return false;
}

static bool IsDeadLocalStore(Generator* gen, IRNode* inst) {
  if (!IRIsStoreOnly(inst) || inst->inputs.length == 0) {
    return false;
  }
  IRNode* dest = inst->inputs.value.p[0];
  if (dest == NULL || !IRIsVariable(dest)) {
    return false;
  }
  Symbol* symbol = IRGetVariableSymbol(dest);
  if (symbol == NULL || symbol->flags.address_taken ||
      symbol->flags.is_argument ||
      (!symbol->flags.is_local && !symbol->flags.is_temp) ||
      StorageIs(symbol->storage, STO(static) | STO(extern) | STO(thread)) ||
      TypeIsVolatile(symbol->type)) {
    return false;
  }
  if (SymbolHasIncDec(gen, symbol)) {
    return false;
  }
  for (size_t i = 0; i < dest->outputs.length; i++) {
    IRNode* user = dest->outputs.value.p[i];
    if (!IRIsStoreOnly(user) || user->inputs.length == 0 ||
        user->inputs.value.p[0] != dest) {
      return false;
    }
  }
  return true;
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
        if (IsDeadExpression(inst) || IsDeadLocalStore(gen, inst)) {
          BasicBlockRemoveInstruction(gen, block, inst);
          changed = true;
        }
      }
    }
  } while (changed);
}

