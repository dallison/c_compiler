#include "memopt.h"

#include <stdlib.h>

#include "alias.h"
#include "basic_block.h"
#include "ir.h"
#include "type_compare.h"
#include "vector.h"

typedef struct {
  IRMemoryLocation location;
  IRNode* value;
  IRNode* producer;
  bool from_store;
  bool observed;
} AvailableAccess;

static void AvailableClear(Vector* available) {
  for (size_t i = 0; i < available->length; i++) {
    free(available->value.p[i]);
  }
  VectorClear(available);
}

static void AvailableAdd(Vector* available, const IRMemoryLocation* location,
                         IRNode* value, IRNode* producer, bool from_store) {
  AvailableAccess* access = malloc(sizeof(AvailableAccess));
  access->location = *location;
  access->value = value;
  access->producer = producer;
  access->from_store = from_store;
  access->observed = false;
  VectorAppend(available, access);
}

static void AvailableRemoveAt(Vector* available, size_t index) {
  free(available->value.p[index]);
  VectorDeleteElement(available, index);
}

static bool TypesMatchForForward(IRNode* load, IRNode* value) {
  return load != NULL && value != NULL && load->type != NULL &&
         value->type != NULL && TypeEqual(load->type, value->type) &&
         !TypeIsVolatile(load->type) && !TypeIsVolatile(value->type);
}

static bool CanReplaceLoad(IRNode* load) {
  return IRIsLoadOnly(load) && load->dest == NULL && load->type != NULL &&
         !TypeIsVolatile(load->type);
}

// Constants and addresses can be rematerialized after a call.  Call results
// and other computed values live in caller-saved registers, so forwarding a
// later load to them reuses a clobbered register instead of the stack slot
// the store was keeping live.
static bool ValueSurvivesCall(IRNode* value) {
  return value != NULL &&
         (IRIsConst(value) || value->opcode == IR_OP(addressof));
}

static void KillClobbered(Vector* available, IRNode* inst) {
  bool call = IRIsCall(inst);
  for (size_t i = available->length; i > 0; i--) {
    AvailableAccess* access = available->value.p[i - 1];
    if (IRAliasInstMayClobber(&access->location, inst) ||
        (call && !ValueSurvivesCall(access->value))) {
      AvailableRemoveAt(available, i - 1);
    }
  }
}

static void MarkObserved(Vector* available, const IRMemoryLocation* location) {
  for (size_t i = 0; i < available->length; i++) {
    AvailableAccess* access = available->value.p[i];
    if (IRAliasClassify(&access->location, location) != kIRNoAlias) {
      access->observed = true;
    }
  }
}

static AvailableAccess* FindMustAlias(Vector* available,
                                      const IRMemoryLocation* location,
                                      bool stores_only) {
  AvailableAccess* found = NULL;
  for (size_t i = 0; i < available->length; i++) {
    AvailableAccess* access = available->value.p[i];
    if (stores_only && !access->from_store) {
      continue;
    }
    if (IRAliasClassify(&access->location, location) == kIRMustAlias &&
        access->location.size == location->size) {
      found = access;
    }
  }
  return found;
}

static bool CallBetween(IRNode* earlier, IRNode* later) {
  if (earlier == NULL || later == NULL || earlier->block == NULL ||
      earlier->block != later->block) {
    return false;
  }
  for (IRNode* node = IRNext(earlier);
       node != NULL && node->block == earlier->block && node != later;
       node = IRNext(node)) {
    if (IRIsCall(node)) {
      return true;
    }
  }
  return false;
}

static bool ForwardLoad(Generator* gen, BasicBlock* block, Vector* available,
                        IRNode* load, const IRMemoryLocation* location) {
  if (!CanReplaceLoad(load) || location->volatile_access ||
      location->atomic_access) {
    return false;
  }
  AvailableAccess* access = FindMustAlias(available, location, false);
  if (access == NULL || access->value == NULL ||
      !TypesMatchForForward(load, access->value) ||
      IRCheckpointBetween(access->producer, load) ||
      (CallBetween(access->producer, load) &&
       !ValueSurvivesCall(access->value))) {
    return false;
  }
  GeneratorReplaceInstruction(gen, load, access->value);
  BasicBlockRemoveInstruction(gen, block, load);
  return true;
}

static void EliminateDeadStore(Generator* gen, BasicBlock* block,
                               Vector* available,
                               const IRMemoryLocation* location) {
  for (size_t i = available->length; i > 0; i--) {
    AvailableAccess* access = available->value.p[i - 1];
    if (!access->from_store || access->observed ||
        access->producer == NULL || access->producer->block != block ||
        !IRIsStoreOnly(access->producer) ||
        IRAliasClassify(&access->location, location) != kIRMustAlias ||
        access->location.size != location->size) {
      continue;
    }
    IRNode* store = access->producer;
    AvailableRemoveAt(available, i - 1);
    BasicBlockRemoveInstruction(gen, block, store);
  }
}

static IRNode* StoredValue(IRNode* store) {
  if (store == NULL || !IRIsStoreOnly(store) || store->inputs.length < 2) {
    return NULL;
  }
  return store->inputs.value.p[1];
}

static void OptimizeBlockMemory(Generator* gen, BasicBlock* block) {
  Vector available;
  VectorInit(&available);
  IRNode* next = NULL;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = next) {
    next = IRNext(inst);
    if (IRIsObservableCheckpoint(inst)) {
      AvailableClear(&available);
      continue;
    }

    IRMemoryLocation location;
    if (IRIsLoadOnly(inst) && IRAliasDecode(inst, &location)) {
      if (ForwardLoad(gen, block, &available, inst, &location)) {
        continue;
      }
      MarkObserved(&available, &location);
      continue;
    }

    if (IRIsStoreOnly(inst) && IRAliasDecode(inst, &location) &&
        inst->dest == NULL && !location.volatile_access &&
        !location.atomic_access) {
      EliminateDeadStore(gen, block, &available, &location);
      KillClobbered(&available, inst);
      IRNode* value = StoredValue(inst);
      if (value != NULL && value->type != NULL &&
          !TypeIsVolatile(value->type)) {
        AvailableAdd(&available, &location, value, inst, true);
      }
      continue;
    }

    KillClobbered(&available, inst);
  }
  AvailableClear(&available);
  VectorDestruct(&available);
}

void MemoryOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    OptimizeBlockMemory(gen, gen->basic_blocks.value.p[i]);
  }
}
