#include "memopt.h"

#include <stdlib.h>
#include <string.h>

#include "alias.h"
#include "basic_block.h"
#include "bitset.h"
#include "codegen.h"
#include "hashmap.h"
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
  // setbit used to be typed as the field; accept same-sized, same-signedness
  // integrals so a corrected word-typed setbit still forwards when types are
  // not pointer-equal without conflating sign- and zero-extended loads.
  return TypeIsIntegral(load->type) && TypeIsIntegral(value->type) &&
         load->type->size == value->type->size &&
         TypeIsUnsigned(load->type) == TypeIsUnsigned(value->type);
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
  if (inst != NULL && inst->opcode == IR_OP(phi) && location->base != NULL &&
      inst->var.def == location->base) {
    return true;
  }
  return IRAliasInstMayClobber(location, inst);
}

// How |inst| affects the loads tracked by cross-block forwarding.
typedef enum {
  kClobberNone,    // Cannot invalidate a load.
  kClobberAll,     // Invalidates every load (checkpoint, atomic).
  kClobberBase,    // Invalidates loads of |base| (a phi that defines it).
  kClobberEffect,  // A decoded store.  Alias-compare against |effect|.
  kClobberFull,    // Ask InstructionClobbersAvailability per load (call, asm).
} ClobberKind;

typedef struct {
  ClobberKind kind;
  Symbol* base;
  IRMemoryLocation effect;
} Clobber;

// A load that may be reused at a later load of the same location.
typedef struct {
  IRNode* load;
  IRMemoryLocation location;
  int linear;  // Position in the instruction list codegen walks.
} LoadInfo;

static bool IsAtomicInst(IRNode* inst) {
  return inst->opcode >= IR_OP(atomic_load) &&
         inst->opcode <= IR_OP(atomic_fence);
}

static Clobber ClassifyClobber(IRNode* inst) {
  Clobber clobber = {.kind = kClobberNone};
  if (inst == NULL) {
    return clobber;
  }
  if (IRIsObservableCheckpoint(inst) || IsAtomicInst(inst)) {
    clobber.kind = kClobberAll;
    return clobber;
  }
  if (inst->opcode == IR_OP(phi)) {
    clobber.kind = kClobberBase;
    clobber.base = inst->var.def;
    return clobber;
  }
  if (IRIsCall(inst) || IRAsmClobbersMemory(inst)) {
    clobber.kind = kClobberFull;
    return clobber;
  }
  if (!IRIsStore(inst) || inst->opcode == IR_OP(addressof) ||
      inst->opcode == IR_OP(cast)) {
    return clobber;
  }
  if (!IRAliasDecode(inst, &clobber.effect)) {
    clobber.kind = kClobberFull;
    return clobber;
  }
  clobber.kind = kClobberEffect;
  return clobber;
}

static bool ClobberHits(const Clobber* clobber, const LoadInfo* load,
                        IRNode* inst) {
  switch (clobber->kind) {
    case kClobberNone:
      return false;
    case kClobberAll:
      return true;
    case kClobberBase:
      return clobber->base != NULL && load->location.base == clobber->base;
    case kClobberEffect:
      return IRAliasClassify(&load->location, &clobber->effect) != kIRNoAlias;
    case kClobberFull:
      return InstructionClobbersAvailability(&load->location, load->load, inst);
  }
  return false;
}

// The two addresses are distinct SSA values, so the loads may not name the
// same object even when alias analysis gives them one root.
static bool SsaAddressesDiffer(IRNode* load, IRNode* candidate) {
  IRNode* load_addr =
      load->inputs.length >= 1 ? load->inputs.value.p[0] : NULL;
  IRNode* candidate_addr =
      candidate->inputs.length >= 1 ? candidate->inputs.value.p[0] : NULL;
  bool ssa_versioned =
      (load_addr != NULL && (load_addr->opcode == IR_OP(ssavar) ||
                             load_addr->opcode == IR_OP(phi) ||
                             load_addr->opcode == IR_OP(argument))) ||
      (candidate_addr != NULL &&
       (candidate_addr->opcode == IR_OP(ssavar) ||
        candidate_addr->opcode == IR_OP(phi) ||
        candidate_addr->opcode == IR_OP(argument)));
  return ssa_versioned && load_addr != candidate_addr;
}

// |src| can replace |use|: it is the same location, dominates the use, and
// codegen will have emitted it first.
static bool LoadCanFeed(const LoadInfo* src, const LoadInfo* use) {
  if (src->linear >= use->linear || src->load->block == NULL ||
      use->load->block == NULL) {
    return false;
  }
  if (src->load->block != use->load->block &&
      !BitSetContains(&use->load->block->dominators,
                      src->load->block->block_id)) {
    return false;
  }
  if (SsaAddressesDiffer(use->load, src->load) ||
      IRAliasClassify(&src->location, &use->location) != kIRMustAlias ||
      src->location.size != use->location.size ||
      !TypesMatchForForward(use->load, src->load)) {
    return false;
  }
  return true;
}

// Prefer the earliest dominating load, the one with the longest reuse.
static bool PreferLoad(const LoadInfo* candidate, const LoadInfo* best) {
  size_t candidate_dominators = candidate->load->block->num_dominators;
  size_t best_dominators = best->load->block->num_dominators;
  if (candidate_dominators != best_dominators) {
    return candidate_dominators < best_dominators;
  }
  return candidate->linear < best->linear;
}

static void IntersectAvailable(Generator* gen, BasicBlock* block, BitSet* outs,
                               BitSet* into) {
  BitSetClear(into);
  bool any = false;
  for (size_t e = 0; e < block->in_edges.length; e++) {
    BlockId id = block->in_edges.value.w[e];
    if (id >= gen->basic_blocks.length ||
        gen->basic_blocks.value.p[id] == NULL) {
      continue;
    }
    BitSet* pred = &outs[id];
    if (!any) {
      BitSetCopy(into, pred);
      any = true;
      continue;
    }
    size_t words = into->capacity < pred->capacity ? into->capacity
                                                   : pred->capacity;
    for (size_t w = 0; w < words; w++) {
      into->value[w] &= pred->value[w];
    }
    if (into->capacity > words) {
      memset(&into->value[words], 0,
             (into->capacity - words) * sizeof(uint64_t));
    }
  }
}

static void TransferAvailable(const BitSet* in, const BitSet* kill,
                              const BitSet* gen, BitSet* out) {
  size_t words = in->capacity;
  if (out->capacity < words) {
    BitSetFill(out, words * 64);
    BitSetClear(out);
  }
  for (size_t w = 0; w < words; w++) {
    uint64_t killed = w < kill->capacity ? kill->value[w] : 0;
    uint64_t generated = w < gen->capacity ? gen->value[w] : 0;
    out->value[w] = (in->value[w] & ~killed) | generated;
  }
  if (out->capacity > words) {
    memset(&out->value[words], 0, (out->capacity - words) * sizeof(uint64_t));
  }
}

static void KillAvailable(BitSet* live, const LoadInfo* loads, int num_loads,
                          const Clobber* clobber, IRNode* inst) {
  if (clobber->kind == kClobberNone) {
    return;
  }
  if (clobber->kind == kClobberAll) {
    BitSetClear(live);
    return;
  }
  for (int i = 0; i < num_loads; i++) {
    if (BitSetContains(live, (size_t)i) &&
        ClobberHits(clobber, &loads[i], inst)) {
      BitSetRemove(live, (size_t)i);
    }
  }
}

// Reuse a dominating load of the same MustAlias location when no path from
// that load to here clobbers it.  Local memopt only sees one block; field
// reloads after `if (p->x > 0)` / `p->x + n` therefore survived as duplicates
// (ChargeCycles turbo_owed).
//
// Availability is a dataflow over the CFG: a load is available at a point when
// every path from the entry computes it and no later instruction on those
// paths clobbers it.  Searching the CFG once per pair of loads is quadratic in
// a large function (a coroutine's main is one state machine) and does not
// finish inside the test timeout.
static void CrossBlockLoadCSE(Generator* gen) {
  size_t num_blocks = gen->basic_blocks.length;
  if (num_blocks == 0) {
    return;
  }

  LoadInfo* loads = NULL;
  int num_loads = 0;
  int loads_cap = 0;
  HashMap ids;
  HashMapInit(&ids, 64);
  int linear = 0;
  for (IRNode* node = GeneratorFirstInstruction(gen); node != NULL;
       node = IRNext(node)) {
    int here = linear++;
    IRMemoryLocation location;
    if (!CanReplaceLoad(node) || !IRAliasDecode(node, &location) ||
        location.volatile_access || location.atomic_access ||
        node->block == NULL) {
      continue;
    }
    if (num_loads == loads_cap) {
      loads_cap = loads_cap == 0 ? 32 : loads_cap * 2;
      loads = realloc(loads, (size_t)loads_cap * sizeof(LoadInfo));
    }
    loads[num_loads].load = node;
    loads[num_loads].location = location;
    loads[num_loads].linear = here;
    HashMapInsert(&ids, (uint64_t)(uintptr_t)node, num_loads);
    num_loads++;
  }
  if (num_loads == 0) {
    HashMapDestruct(&ids);
    free(loads);
    return;
  }

  BitSet* kill = calloc(num_blocks, sizeof(BitSet));
  BitSet* generated = calloc(num_blocks, sizeof(BitSet));
  BitSet* avail_out = calloc(num_blocks, sizeof(BitSet));
  for (size_t b = 0; b < num_blocks; b++) {
    BitSetFill(&kill[b], (size_t)num_loads);
    BitSetClear(&kill[b]);
    BitSetFill(&generated[b], (size_t)num_loads);
    BitSetClear(&generated[b]);
    BasicBlock* block = gen->basic_blocks.value.p[b];
    if (block == NULL) {
      continue;
    }
    BitSetFill(&avail_out[b], (size_t)num_loads);
    BitSet live;
    BitSetInit(&live);
    BitSetFill(&live, (size_t)num_loads);
    BitSetClear(&live);
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      Clobber clobber = ClassifyClobber(inst);
      if (clobber.kind == kClobberAll) {
        BitSetFill(&kill[b], (size_t)num_loads);
        BitSetClear(&live);
      } else if (clobber.kind != kClobberNone) {
        for (int i = 0; i < num_loads; i++) {
          if (!ClobberHits(&clobber, &loads[i], inst)) {
            continue;
          }
          BitSetInsert(&kill[b], (size_t)i);
          BitSetRemove(&live, (size_t)i);
        }
      }
      int id = (int)HashMapFind(&ids, (uint64_t)(uintptr_t)inst, -1);
      if (id >= 0) {
        BitSetInsert(&live, (size_t)id);
      }
    }
    BitSetCopy(&generated[b], &live);
    BitSetDestruct(&live);
  }

  BitSet in_set;
  BitSet next;
  BitSetInit(&in_set);
  BitSetInit(&next);
  BitSetFill(&in_set, (size_t)num_loads);
  BitSetFill(&next, (size_t)num_loads);
  BitSet in_queue;
  BitSetInit(&in_queue);
  size_t* queue = calloc(num_blocks + 1, sizeof(size_t));
  size_t qhead = 0;
  size_t qtail = 0;
  for (size_t b = 0; b < num_blocks; b++) {
    if (gen->basic_blocks.value.p[b] == NULL) {
      continue;
    }
    queue[qtail++] = b;
    BitSetInsert(&in_queue, b);
  }
  // Each block's available set only shrinks, so the worklist ends.  The cap
  // is that bound; exceeding it means the sets did not shrink and forwarding
  // would be unsafe.
  size_t steps = 0;
  size_t step_limit = num_blocks * ((size_t)num_loads + 2) + 1;
  bool failed = false;
  while (qhead != qtail) {
    if (++steps > step_limit) {
      failed = true;
      break;
    }
    size_t b = queue[qhead++];
    if (qhead == num_blocks + 1) {
      qhead = 0;
    }
    BitSetRemove(&in_queue, b);
    BasicBlock* block = gen->basic_blocks.value.p[b];
    IntersectAvailable(gen, block, avail_out, &in_set);
    TransferAvailable(&in_set, &kill[b], &generated[b], &next);
    if (BitSetEqual(&next, &avail_out[b])) {
      continue;
    }
    BitSetCopy(&avail_out[b], &next);
    for (size_t e = 0; e < block->out_edges.length; e++) {
      BlockId succ = block->out_edges.value.w[e];
      if (succ >= num_blocks || gen->basic_blocks.value.p[succ] == NULL ||
          BitSetContains(&in_queue, succ)) {
        continue;
      }
      queue[qtail++] = succ;
      if (qtail == num_blocks + 1) {
        qtail = 0;
      }
      BitSetInsert(&in_queue, succ);
    }
  }

  int* replacement = NULL;
  if (!failed) {
    replacement = malloc((size_t)num_loads * sizeof(int));
    for (int i = 0; i < num_loads; i++) {
      replacement[i] = -1;
    }
    for (size_t b = 0; b < num_blocks; b++) {
      BasicBlock* block = gen->basic_blocks.value.p[b];
      if (block == NULL) {
        continue;
      }
      IntersectAvailable(gen, block, avail_out, &in_set);
      for (IRNode* inst = BasicBlockBegin(block);
           !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
           inst = IRNext(inst)) {
        int id = (int)HashMapFind(&ids, (uint64_t)(uintptr_t)inst, -1);
        if (id >= 0) {
          int best = -1;
          BitSetIterator it;
          BitSetIteratorStart(&it, &in_set);
          while (!BitSetIteratorDone(&it)) {
            int candidate = (int)BitSetIteratorValue(&it);
            BitSetIteratorNext(&it);
            if (!LoadCanFeed(&loads[candidate], &loads[id])) {
              continue;
            }
            if (best < 0 ||
                PreferLoad(&loads[candidate], &loads[best])) {
              best = candidate;
            }
          }
          replacement[id] = best;
        }
        Clobber clobber = ClassifyClobber(inst);
        KillAvailable(&in_set, loads, num_loads, &clobber, inst);
        if (id >= 0) {
          BitSetInsert(&in_set, (size_t)id);
        }
      }
    }
    // Resolve chains before editing the IR.  Clearing a link or deleting a
    // load while another load still names it would forward to a removed node.
    int* final_id = malloc((size_t)num_loads * sizeof(int));
    for (int i = 0; i < num_loads; i++) {
      int final = i;
      int guard = num_loads;
      while (guard-- > 0 && replacement[final] >= 0) {
        final = replacement[final];
      }
      final_id[i] = (final == i || replacement[final] >= 0) ? -1 : final;
    }
    for (int i = 0; i < num_loads; i++) {
      if (final_id[i] < 0) {
        continue;
      }
      IRNode* load = loads[i].load;
      BasicBlock* block = load->block;
      GeneratorReplaceInstruction(gen, load, loads[final_id[i]].load);
      BasicBlockRemoveInstruction(gen, block, load);
    }
    free(final_id);
  }

  free(replacement);
  free(queue);
  BitSetDestruct(&in_queue);
  BitSetDestruct(&next);
  BitSetDestruct(&in_set);
  for (size_t b = 0; b < num_blocks; b++) {
    BitSetDestruct(&kill[b]);
    BitSetDestruct(&generated[b]);
    BitSetDestruct(&avail_out[b]);
  }
  free(kill);
  free(generated);
  free(avail_out);
  HashMapDestruct(&ids);
  free(loads);
}

void MemoryOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    OptimizeBlockMemory(gen, gen->basic_blocks.value.p[i]);
  }
  CrossBlockLoadCSE(gen);
}
