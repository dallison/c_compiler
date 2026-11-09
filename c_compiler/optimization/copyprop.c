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

static bool InstructionPrecedesInFunction(IRNode* before, IRNode* after) {
  if (before == NULL || after == NULL) {
    return false;
  }
  for (IRNode* inst = before; inst != NULL; inst = IRNext(inst)) {
    if (inst == after) {
      return true;
    }
  }
  return false;
}

static bool BlockDominates(BasicBlock* dominator, BasicBlock* block) {
  return dominator != NULL && block != NULL &&
         BitSetContains(&block->dominators, dominator->block_id);
}

static bool CanPropagateMove(Generator* gen, IRNode* inst) {
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

  if (source->block == inst->block && user->block == inst->block) {
    return !IRCheckpointBetween(source, inst) &&
           !IRCheckpointBetween(inst, user);
  }

  // Cross-block copies: the source already reaches the move, and the move
  // already reaches its only user.  Replacing the move with the source does
  // not add a new live range, but exception landing pads can enter between
  // those points, so refuse the function if it has any.
  if (gen->exception_ranges.length != 0) {
    return false;
  }
  if (IRIsConst(source)) {
    return user->block != NULL;
  }
  if (source->block == NULL || inst->block == NULL || user->block == NULL) {
    return false;
  }
  if (!BlockDominates(source->block, inst->block) ||
      !BlockDominates(inst->block, user->block)) {
    return false;
  }
  return InstructionPrecedesInFunction(source, inst) &&
         InstructionPrecedesInFunction(inst, user);
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

static bool IsScalarVariableLoad(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(load8):
    case IR_OP(load16):
    case IR_OP(load32):
    case IR_OP(load64):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadu32):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      return node->inputs.length == 1 && node->dest == NULL &&
             node->type != NULL && !TypeIsVolatile(node->type);
    default:
      return false;
  }
}

// An incoming argument that is only ever read keeps its value throughout.
static bool IsReadOnlyArgument(IRNode* node) {
  if (node == NULL || node->opcode != IR_OP(argument)) {
    return false;
  }
  Symbol* symbol = ((IRVariable*)node)->symbol;
  if (symbol == NULL || symbol->flags.address_taken ||
      TypeIsVolatile(symbol->type) || TypeIsArray(symbol->type)) {
    return false;
  }
  for (size_t i = 0; i < node->outputs.length; i++) {
    IRNode* user = node->outputs.value.p[i];
    if (!IRIsLoadOnly(user) || user->inputs.value.p[0] != node) {
      return false;
    }
  }
  return true;
}

// The only definition of an SSA version, or NULL.
static IRNode* SoleDefinition(IRNode* variable) {
  IRNode* store = NULL;
  for (size_t i = 0; i < variable->outputs.length; i++) {
    IRNode* user = variable->outputs.value.p[i];
    if (user->inputs.length == 0 || user->inputs.value.p[0] != variable) {
      continue;
    }
    if (IRIsStoreOnly(user)) {
      if (store != NULL) {
        return NULL;
      }
      store = user;
    } else if (!IRIsLoadOnly(user)) {
      return NULL;
    }
  }
  return store;
}

// Removes phis whose value no instruction other than a dead phi reads.
static void RemoveDeadPhis(Generator* gen) {
  Vector phis = {0};
  BitSet live;
  BitSetInit(&live);
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (inst->opcode == IR_OP(phi)) {
      VectorAppend(&phis, inst);
    }
  }
  Vector work = {0};
  for (size_t i = 0; i < phis.length; i++) {
    IRNode* phi = phis.value.p[i];
    for (size_t j = 0; j < phi->outputs.length; j++) {
      IRNode* user = phi->outputs.value.p[j];
      if (user->opcode != IR_OP(phi)) {
        BitSetInsertGrow(&live, (size_t)phi->id);
        VectorAppend(&work, phi);
        break;
      }
    }
  }
  while (work.length > 0) {
    IRNode* phi = work.value.p[--work.length];
    for (size_t j = 0; j < phi->inputs.length; j++) {
      IRNode* input = phi->inputs.value.p[j];
      if (input != NULL && input->opcode == IR_OP(phi) &&
          !BitSetContains(&live, (size_t)input->id)) {
        BitSetInsertGrow(&live, (size_t)input->id);
        VectorAppend(&work, input);
      }
    }
  }
  for (size_t i = 0; i < phis.length; i++) {
    IRNode* phi = phis.value.p[i];
    if (!BitSetContains(&live, (size_t)phi->id) && phi->block != NULL) {
      BasicBlockRemoveInstruction(gen, phi->block, phi);
    }
  }
  BitSetDestruct(&live);
  VectorDestruct(&work);
  VectorDestruct(&phis);
}

// Leaving SSA folds every version of a symbol back into one variable, so a
// version can be read at a later point only while no other version of its
// symbol exists.
static int64_t SymbolVersionCount(Map* versions, Symbol* symbol) {
  MapKeyType key = {.p = symbol};
  MapValueType* count = MapSearch(versions, key);
  return count != NULL ? count->w : 0;
}

// True when code still names the symbol's pooled variable directly rather
// than one of its SSA versions.
static bool PooledVariableUsed(Map* pooled, Symbol* symbol) {
  MapKeyType key = {.p = symbol};
  IRNode* variable = MapFind(pooled, key);
  return variable != NULL && variable->outputs.length != 0;
}

// Drops the definitions of a symbol none of whose versions is read anywhere.
static void RemoveUnreadSymbolDefinitions(Generator* gen, Map* pooled) {
  Map read;
  MapInitForPointerKeys(&read);
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (inst->opcode != IR_OP(ssavar) && inst->opcode != IR_OP(phi)) {
      continue;
    }
    Symbol* symbol = ((IRVariable*)inst)->symbol;
    bool reads = inst->opcode == IR_OP(phi) || !IsSafeSSAVariable(inst) ||
                 PooledVariableUsed(pooled, symbol);
    for (size_t i = 0; i < inst->outputs.length && !reads; i++) {
      IRNode* user = inst->outputs.value.p[i];
      reads = !IRIsStoreOnly(user) || user->inputs.length != 2 ||
              user->inputs.value.p[0] != inst || user->dest != NULL;
    }
    MapKeyType key = {.p = symbol};
    MapValueType* found = MapSearch(&read, key);
    if (found != NULL) {
      found->w |= reads;
    } else {
      MapKeyValue kv = {.key = key, .value.w = reads};
      MapInsert(&read, kv);
    }
  }
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (inst->opcode != IR_OP(ssavar)) {
      continue;
    }
    MapKeyType key = {.p = ((IRVariable*)inst)->symbol};
    MapValueType* found = MapSearch(&read, key);
    if (found == NULL || found->w != 0) {
      continue;
    }
    for (size_t i = inst->outputs.length; i-- > 0;) {
      IRNode* store = inst->outputs.value.p[i];
      if (store->block == NULL || store->outputs.length != 0) {
        continue;
      }
      IRNode* value = store->inputs.value.p[1];
      BasicBlockRemoveInstruction(gen, store->block, store);
      if (value != NULL && value->outputs.length == 0 &&
          value->block != NULL && IsScalarVariableLoad(value)) {
        BasicBlockRemoveInstruction(gen, value->block, value);
      }
    }
  }
  MapDestruct(&read);
}

// An SSA version defined as a copy of a symbol's only version (or of an
// argument that is never reassigned) holds that value wherever it is read.
// Reading the source directly leaves one live value instead of two; each
// inlined call otherwise adds another copy of the caller's pointer.
void SSAVariableCopyPropagation(Generator* gen) {
  if (gen->exception_ranges.length != 0) {
    return;
  }
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (IRIsObservableCheckpoint(inst)) {
      return;
    }
  }
  RemoveDeadPhis(gen);
  Map pooled;
  MapInitForPointerKeys(&pooled);
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = gen->variable_pool.value.p[i];
    MapKeyValue kv = {.key.p = entry->value.symbol, .value.p = entry->pooled};
    MapInsert(&pooled, kv);
  }
  Map versions;
  MapInitForPointerKeys(&versions);
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (inst->opcode != IR_OP(ssavar) && inst->opcode != IR_OP(phi)) {
      continue;
    }
    MapKeyType key = {.p = ((IRVariable*)inst)->symbol};
    MapValueType* count = MapSearch(&versions, key);
    if (count != NULL) {
      count->w++;
    } else {
      MapKeyValue kv = {.key = key, .value.w = 1};
      MapInsert(&versions, kv);
    }
  }
  for (IRNode* copy = GeneratorFirstInstruction(gen); copy != NULL;
       copy = IRNext(copy)) {
    if (!IsSafeSSAVariable(copy)) {
      continue;
    }
    IRNode* store = SoleDefinition(copy);
    if (store == NULL || store->inputs.length != 2 || store->dest != NULL) {
      continue;
    }
    IRNode* value = store->inputs.value.p[1];
    if (value != NULL && IsScalarVariableLoad(value)) {
      IRNode* source = value->inputs.value.p[0];
      Symbol* symbol = source->opcode == IR_OP(ssavar) ||
                               source->opcode == IR_OP(argument)
                           ? ((IRVariable*)source)->symbol
                           : NULL;
      bool single_version =
          symbol != NULL &&
          ((IsSafeSSAVariable(source) && SoleDefinition(source) != NULL &&
            SymbolVersionCount(&versions, symbol) == 1 &&
            !PooledVariableUsed(&pooled, symbol)) ||
           (IsReadOnlyArgument(source) &&
            SymbolVersionCount(&versions, symbol) == 0));
      if (source != copy && single_version) {
        for (size_t i = copy->outputs.length; i-- > 0;) {
          IRNode* user = copy->outputs.value.p[i];
          if (user != store && user->opcode == value->opcode &&
              user->inputs.value.p[0] == copy && IsScalarVariableLoad(user)) {
            IRReplaceInput(user, 0, source);
          }
        }
      }
    }
  }
  RemoveUnreadSymbolDefinitions(gen, &pooled);
  MapDestruct(&pooled);
  MapDestruct(&versions);
}

void CopyPropagationOptimization(Generator* gen) {
  SSAVariableCopyPropagation(gen);
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
      if (!CanPropagateMove(gen, inst)) {
        continue;
      }
      IRNode* source = inst->inputs.value.p[0];
      GeneratorReplaceInstruction(gen, inst, source);
      BasicBlockRemoveInstruction(gen, block, inst);
    }
  }
}
