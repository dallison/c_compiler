#include "memopt.h"

#include <stdlib.h>

#include "alias.h"
#include "basic_block.h"
#include "bitset.h"
#include "codegen.h"
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
  if (load == NULL || value == NULL || load->type == NULL || value->type == NULL ||
      TypeIsVolatile(load->type) || TypeIsVolatile(value->type)) {
    return false;
  }
  if (TypeEqual(load->type, value->type)) {
    return true;
  }
  // setbit used to be typed as the field; accept same-sized integrals so a
  // corrected word-typed setbit still forwards when types are not pointer-equal.
  return TypeIsIntegral(load->type) && TypeIsIntegral(value->type) &&
         load->type->size == value->type->size;
}

static bool CanReplaceLoad(IRNode* load) {
  return IRIsLoadOnly(load) && load->dest == NULL && load->type != NULL &&
         !TypeIsVolatile(load->type);
}

static void KillClobbered(Vector* available, IRNode* inst) {
  for (size_t i = available->length; i > 0; i--) {
    AvailableAccess* access = available->value.p[i - 1];
    // Rely on alias mod/ref alone.  SSA values are allowed to remain live
    // across calls that do not clobber the location; the register allocator
    // keeps them in callee-saved registers when needed.
    if (IRAliasInstMayClobber(&access->location, inst)) {
      AvailableRemoveAt(available, i - 1);
    }
  }
}

static void MarkObserved(Vector* available, const IRMemoryLocation* location) {
  for (size_t i = 0; i < available->length; i++) {
    AvailableAccess* access = available->value.p[i];
    // Only MustAlias proves this load reads that store.  MayAlias would
    // conservatively block dead-store elimination of consecutive bitfield
    // writes through a pointer.
    if (IRAliasClassify(&access->location, location) == kIRMustAlias) {
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

static bool ForwardLoad(Generator* gen, BasicBlock* block, Vector* available,
                        IRNode* load, const IRMemoryLocation* location) {
  if (!CanReplaceLoad(load) || location->volatile_access ||
      location->atomic_access) {
    return false;
  }
  AvailableAccess* access = FindMustAlias(available, location, false);
  if (access == NULL || access->value == NULL ||
      !TypesMatchForForward(load, access->value) ||
      IRCheckpointBetween(access->producer, load)) {
    return false;
  }
  // Refuse to forward when a call that may clobber this location sits between
  // the producer and the load.  KillClobbered normally drops those entries,
  // but a producer added earlier in this same scan can still be present.
  for (IRNode* node = IRNext(access->producer);
       node != NULL && node->block == access->producer->block && node != load;
       node = IRNext(node)) {
    if (IRAliasInstMayClobber(location, node)) {
      return false;
    }
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
      if (CanReplaceLoad(inst) && !location.volatile_access &&
          !location.atomic_access) {
        AvailableAdd(&available, &location, inst, inst, false);
      }
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

static bool InstructionClobbersAvailability(const IRMemoryLocation* location,
                                            IRNode* value, IRNode* inst) {
  (void)value;
  if (IRIsObservableCheckpoint(inst)) {
    return true;
  }
  return IRAliasInstMayClobber(location, inst);
}

// True when |earlier| precedes |later| in the same basic block.
static bool ComesBeforeInBlock(IRNode* earlier, IRNode* later) {
  if (earlier == NULL || later == NULL || earlier->block == NULL ||
      earlier->block != later->block || earlier == later) {
    return false;
  }
  for (IRNode* node = IRNext(earlier);
       node != NULL && node->block == earlier->block; node = IRNext(node)) {
    if (node == later) {
      return true;
    }
  }
  return false;
}

static void CollectForwardReachable(Generator* gen, BasicBlock* start,
                                    BitSet* reachable) {
  BitSetClear(reachable);
  Vector work;
  VectorInit(&work);
  BitSetInsert(reachable, start->block_id);
  VectorAppend(&work, start);
  while (work.length > 0) {
    BasicBlock* block = work.value.p[--work.length];
    for (size_t i = 0; i < block->out_edges.length; i++) {
      BlockId id = block->out_edges.value.w[i];
      if (BitSetContains(reachable, id)) {
        continue;
      }
      BitSetInsert(reachable, id);
      VectorAppend(&work, gen->basic_blocks.value.p[id]);
    }
  }
  VectorDestruct(&work);
}

static void CollectBackwardReachable(Generator* gen, BasicBlock* start,
                                     BitSet* reachable) {
  BitSetClear(reachable);
  Vector work;
  VectorInit(&work);
  BitSetInsert(reachable, start->block_id);
  VectorAppend(&work, start);
  while (work.length > 0) {
    BasicBlock* block = work.value.p[--work.length];
    for (size_t i = 0; i < block->in_edges.length; i++) {
      BlockId id = block->in_edges.value.w[i];
      if (BitSetContains(reachable, id)) {
        continue;
      }
      BitSetInsert(reachable, id);
      VectorAppend(&work, gen->basic_blocks.value.p[id]);
    }
  }
  VectorDestruct(&work);
}

// True when some path from after |from| to |to| may invalidate |value| as a
// forwarding source for |location|.  |from| is required to dominate |to|.
static bool PathMayClobber(Generator* gen, IRNode* from, IRNode* to,
                           const IRMemoryLocation* location, IRNode* value) {
  BasicBlock* from_bb = from->block;
  BasicBlock* to_bb = to->block;
  if (from_bb == to_bb) {
    for (IRNode* node = IRNext(from);
         node != NULL && node != to && node->block == from_bb;
         node = IRNext(node)) {
      if (InstructionClobbersAvailability(location, value, node)) {
        return true;
      }
    }
    return false;
  }

  BitSet forward;
  BitSet backward;
  BitSet on_path;
  BitSetInit(&forward);
  BitSetInit(&backward);
  BitSetInit(&on_path);
  CollectForwardReachable(gen, from_bb, &forward);
  CollectBackwardReachable(gen, to_bb, &backward);
  BitSetIntersection(&forward, &backward, &on_path);

  bool clobbered = false;
  BitSetIterator it;
  BitSetIteratorStart(&it, &on_path);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* node = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && node != BasicBlockEnd(block);
         node = IRNext(node)) {
      if (block == from_bb &&
          (node == from || ComesBeforeInBlock(node, from))) {
        continue;
      }
      if (block == to_bb &&
          (node == to || !ComesBeforeInBlock(node, to))) {
        break;
      }
      if (InstructionClobbersAvailability(location, value, node)) {
        clobbered = true;
        break;
      }
    }
    if (clobbered) {
      break;
    }
    BitSetIteratorNext(&it);
  }

  BitSetDestruct(&forward);
  BitSetDestruct(&backward);
  BitSetDestruct(&on_path);
  return clobbered;
}

static bool LoadDominatesLoad(IRNode* earlier, IRNode* later) {
  if (earlier == NULL || later == NULL || earlier->block == NULL ||
      later->block == NULL) {
    return false;
  }
  if (earlier->block == later->block) {
    return ComesBeforeInBlock(earlier, later);
  }
  return BitSetContains(&later->block->dominators, earlier->block->block_id);
}

// Codegen lowers IR in linked-list order, so a forwarded value must already
// appear earlier in that order (dominance alone is not enough when layout
// disagrees with the dominator tree).
static bool ComesBeforeLinearly(IRNode* earlier, IRNode* later) {
  if (earlier == NULL || later == NULL || earlier == later) {
    return false;
  }
  for (IRNode* node = IRNext(earlier); node != NULL; node = IRNext(node)) {
    if (node == later) {
      return true;
    }
  }
  return false;
}

// Reuse a dominating load of the same MustAlias location when no path from
// that load to here clobbers it.  Local memopt only sees one block; field
// reloads after `if (p->x > 0)` / `p->x + n` therefore survived as duplicates
// (ChargeCycles turbo_owed).
static void CrossBlockLoadCSE(Generator* gen) {
  bool changed;
  do {
    changed = false;
    for (size_t bi = 0; bi < gen->basic_blocks.length; bi++) {
      BasicBlock* block = gen->basic_blocks.value.p[bi];
      IRNode* next = NULL;
      for (IRNode* load = BasicBlockBegin(block);
           !BasicBlockIsEmpty(block) && load != BasicBlockEnd(block);
           load = next) {
        next = IRNext(load);
        IRMemoryLocation location;
        if (!CanReplaceLoad(load) || !IRAliasDecode(load, &location) ||
            location.volatile_access || location.atomic_access) {
          continue;
        }

        IRNode* best = NULL;
        for (size_t bj = 0; bj < gen->basic_blocks.length; bj++) {
          BasicBlock* other = gen->basic_blocks.value.p[bj];
          if (!BitSetContains(&block->dominators, other->block_id) &&
              other != block) {
            continue;
          }
          for (IRNode* candidate = BasicBlockBegin(other);
               !BasicBlockIsEmpty(other) &&
               candidate != BasicBlockEnd(other) && candidate != load;
               candidate = IRNext(candidate)) {
            IRMemoryLocation candidate_location;
            if (!IRIsLoadOnly(candidate) ||
                !IRAliasDecode(candidate, &candidate_location) ||
                IRAliasClassify(&candidate_location, &location) !=
                    kIRMustAlias ||
                candidate_location.size != location.size ||
                !TypesMatchForForward(load, candidate) ||
                !LoadDominatesLoad(candidate, load) ||
                !ComesBeforeLinearly(candidate, load) ||
                PathMayClobber(gen, candidate, load, &location, candidate)) {
              continue;
            }
            // Prefer the earliest dominating load (longest reuse).
            if (best == NULL || LoadDominatesLoad(candidate, best) ||
                (candidate->block == best->block &&
                 ComesBeforeInBlock(candidate, best))) {
              best = candidate;
            }
          }
        }

        if (best != NULL) {
          GeneratorReplaceInstruction(gen, load, best);
          BasicBlockRemoveInstruction(gen, block, load);
          changed = true;
          break;
        }
      }
      if (changed) {
        break;
      }
    }
  } while (changed);
}

void MemoryOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    OptimizeBlockMemory(gen, gen->basic_blocks.value.p[i]);
  }
  CrossBlockLoadCSE(gen);
}
