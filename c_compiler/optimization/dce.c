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

static bool IsDeadWriteOnlyAddressOf(IRNode* addr, IRNode* var) {
  if (addr == NULL || addr->opcode != IR_OP(addressof) || addr->dest != NULL ||
      addr->inputs.length != 1 || addr->inputs.value.p[0] != var) {
    return false;
  }
  for (size_t i = 0; i < addr->outputs.length; i++) {
    IRNode* u = addr->outputs.value.p[i];
    if (!IRIsStoreOnly(u) || u->outputs.length != 0 || u->inputs.length != 2 ||
        u->inputs.value.p[0] != addr || u->inputs.value.p[1] == addr) {
      return false;
    }
  }
  return true;
}

static bool SymbolHasOtherReadOrRMW(Generator* gen, Symbol* symbol) {
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (IRIsVarRef(inst) && inst->var.use == symbol) {
      return true;
    }
    if (IRIsVarDef(inst) && !IRIsStoreOnly(inst) && inst->var.def == symbol) {
      return true;
    }
  }
  return false;
}

static bool IsDeadLocalStore(Generator* gen, IRNode* inst) {
  if (!IRIsStoreOnly(inst) || inst->inputs.length == 0 ||
      inst->outputs.length != 0) {
    return false;
  }
  IRNode* dest = inst->inputs.value.p[0];
  if (dest == NULL) {
    return false;
  }
  bool via_addressof = false;
  if (dest->opcode == IR_OP(addressof) && dest->inputs.length == 1) {
    dest = dest->inputs.value.p[0];
    via_addressof = true;
  }
  if (dest == NULL || !IRIsVariable(dest)) {
    return false;
  }
  if (via_addressof && dest->opcode != IR_OP(localvar) &&
      dest->opcode != IR_OP(tempvar)) {
    return false;
  }
  Symbol* symbol = IRGetVariableSymbol(dest);
  if (symbol == NULL || symbol->flags.is_argument ||
      (!symbol->flags.is_local && !symbol->flags.is_temp) ||
      SymbolIsAnonymousUnionMemberView(symbol) ||
      StorageIs(symbol->storage, STO(static) | STO(extern) | STO(thread)) ||
      TypeIsVolatile(symbol->type)) {
    return false;
  }
  if (!via_addressof && symbol->flags.address_taken) {
    return false;
  }
  if (via_addressof ? SymbolHasOtherReadOrRMW(gen, symbol)
                    : SymbolHasIncDec(gen, symbol)) {
    return false;
  }
  for (size_t i = 0; i < dest->outputs.length; i++) {
    IRNode* user = dest->outputs.value.p[i];
    if (IRIsStoreOnly(user) && user->outputs.length == 0 &&
        user->inputs.length == 2 && user->inputs.value.p[0] == dest &&
        user->inputs.value.p[1] != dest) {
      continue;
    }
    if (via_addressof && IsDeadWriteOnlyAddressOf(user, dest)) {
      continue;
    }
    return false;
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

