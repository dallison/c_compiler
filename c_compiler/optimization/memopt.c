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

static bool IncDecOpcodes(IROpcode op, IROpcode* load_op, IROpcode* store_op,
                          bool* is_inc) {
  switch (op) {
    case IR_OP(inc32):
      *load_op = IR_OP(load32);
      *store_op = IR_OP(store32);
      *is_inc = true;
      return true;
    case IR_OP(uinc32):
      *load_op = IR_OP(loadu32);
      *store_op = IR_OP(store32);
      *is_inc = true;
      return true;
    case IR_OP(dec32):
      *load_op = IR_OP(load32);
      *store_op = IR_OP(store32);
      *is_inc = false;
      return true;
    case IR_OP(udec32):
      *load_op = IR_OP(loadu32);
      *store_op = IR_OP(store32);
      *is_inc = false;
      return true;
    case IR_OP(inc64):
    case IR_OP(uinc64):
      *load_op = IR_OP(load64);
      *store_op = IR_OP(store64);
      *is_inc = true;
      return true;
    case IR_OP(dec64):
    case IR_OP(udec64):
      *load_op = IR_OP(load64);
      *store_op = IR_OP(store64);
      *is_inc = false;
      return true;
    default:
      return false;
  }
}

static bool HasNearbyFollowupLoad(IRNode* inst,
                                  const IRMemoryLocation* location) {
  int steps = 0;
  for (IRNode* scan = IRNext(inst); scan != NULL && steps < 48;
       scan = IRNext(scan), steps++) {
    if (IRIsObservableCheckpoint(scan) || IRIsCall(scan) ||
        IRAsmClobbersMemory(scan)) {
      return false;
    }
    if (IRIsLoadOnly(scan)) {
      IRMemoryLocation scan_loc;
      if (CanReplaceLoad(scan) && IRAliasDecode(scan, &scan_loc) &&
          !scan_loc.volatile_access && !scan_loc.atomic_access &&
          IRAliasClassify(location, &scan_loc) == kIRMustAlias &&
          location->size == scan_loc.size) {
        return true;
      }
    }
    if (IRIsStore(scan) && IRAliasInstMayClobber(location, scan)) {
      return false;
    }
  }
  return false;
}

static IRNode* TryExpandIncDecForForwarding(Generator* gen, BasicBlock* block,
                                            IRNode* inst) {
  IROpcode load_op, store_op;
  bool is_inc = false;
  if (!IncDecOpcodes(inst->opcode, &load_op, &store_op, &is_inc) ||
      inst->outputs.length != 0 || inst->dest != NULL ||
      inst->inputs.length != 2 || IRIsVarDef(inst) || inst->type == NULL ||
      TypeIsVolatile(inst->type) || TypeIsBitInt(inst->type)) {
    return NULL;
  }
  IRNode* addr = inst->inputs.value.p[0];
  IRNode* step_node = inst->inputs.value.p[1];
  if (addr == NULL || IRHasSideEffects(addr) || step_node == NULL ||
      !IRIsIntConst(step_node)) {
    return NULL;
  }
  IRMemoryLocation location;
  if (!IRAliasDecode(inst, &location) || location.volatile_access ||
      location.atomic_access ||
      (location.kind != kIRMemoryIndirect &&
       location.kind != kIRMemoryStatic &&
       location.kind != kIRMemoryExtern) ||
      !HasNearbyFollowupLoad(inst, &location)) {
    return NULL;
  }
  IRNode* ld = NewIR1(load_op, addr);
  IRSetType(ld, inst->type);
  ld->location = inst->location;
  if (IRIsVarRef(inst)) {
    ld->flags |= kIRVarUse;
    ld->var.use = inst->var.use;
  }
  // An IR node may only consume nodes that are already in the instruction
  // list (see IRAddInput), so each new node is emitted before the next one is
  // built from it.
  BasicBlockEmitBefore(gen, block, ld, inst);
  IRNode* step =
      GeneratorGetIntConstant(gen, inst->type, IRIntConstValue(step_node));
  IRNode* alu = NewIR2(is_inc ? IR_OP(addi) : IR_OP(subi), ld, step);
  IRSetType(alu, inst->type);
  alu->location = inst->location;
  BasicBlockEmitBefore(gen, block, alu, inst);
  IRNode* st = NewIR2(store_op, addr, alu);
  IRSetType(st, inst->type);
  st->location = inst->location;
  BasicBlockEmitBefore(gen, block, st, inst);
  BasicBlockRemoveInstruction(gen, block, inst);
  return ld;
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

    IRNode* expanded = TryExpandIncDecForForwarding(gen, block, inst);
    if (expanded != NULL) {
      next = expanded;
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

// A load or store whose value may be reused at a later load of the same
// location.
typedef struct {
  IRNode* load;      // The load or store instruction in gen->code.
  IRNode* value;     // Value produced (`load` for loads, StoredValue for stores).
  IRMemoryLocation location;
  int linear;  // Position in the instruction list codegen walks.
  bool is_store;
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
      return InstructionClobbersAvailability(&load->location, load->value, inst);
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
  if (use->is_store || src->value == NULL || src->linear >= use->linear ||
      src->load->block == NULL || use->load->block == NULL) {
    return false;
  }
  if (src->load->block != use->load->block &&
      !BitSetContains(&use->load->block->dominators,
                      src->load->block->block_id)) {
    return false;
  }
  if (src->is_store && !IRIsConstant(src->value)) {
    if (src->value->block == NULL) {
      return false;
    }
    if (src->value->block != use->load->block &&
        !BitSetContains(&use->load->block->dominators,
                        src->value->block->block_id)) {
      return false;
    }
  }
  if (SsaAddressesDiffer(use->load, src->load) ||
      IRAliasClassify(&src->location, &use->location) != kIRMustAlias ||
      src->location.size != use->location.size ||
      !TypesMatchForForward(use->load, src->value)) {
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

static void IntersectAvailable(Generator* gen, BasicBlock* block,
                               const Vector* outs, BitSet* into) {
  BitSetClear(into);
  bool any = false;
  for (size_t e = 0; e < block->in_edges.length; e++) {
    BlockId id = block->in_edges.value.w[e];
    if (id >= gen->basic_blocks.length ||
        gen->basic_blocks.value.p[id] == NULL) {
      continue;
    }
    BitSet* pred = outs->value.p[id];
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

static void KillAvailable(BitSet* live, const Vector* loads,
                          const Clobber* clobber, IRNode* inst) {
  if (clobber->kind == kClobberNone) {
    return;
  }
  if (clobber->kind == kClobberAll) {
    BitSetClear(live);
    return;
  }
  for (size_t i = 0; i < loads->length; i++) {
    if (BitSetContains(live, i) &&
        ClobberHits(clobber, loads->value.p[i], inst)) {
      BitSetRemove(live, i);
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

  Vector loads;
  VectorInit(&loads);
  HashMap ids;
  HashMapInit(&ids, 64);
  int linear = 0;
  for (IRNode* node = GeneratorFirstInstruction(gen); node != NULL;
       node = IRNext(node)) {
    int here = linear++;
    if (node->block == NULL) {
      continue;
    }
    IRMemoryLocation location;
    if (CanReplaceLoad(node) && IRAliasDecode(node, &location) &&
        !location.volatile_access && !location.atomic_access) {
      LoadInfo* info = calloc(1, sizeof(LoadInfo));
      info->load = node;
      info->value = node;
      info->location = location;
      info->linear = here;
      info->is_store = false;
      HashMapInsert(&ids, (uint64_t)(uintptr_t)node, (int64_t)loads.length);
      VectorAppend(&loads, info);
      continue;
    }
    if (IRIsStoreOnly(node) && node->dest == NULL &&
        IRAliasDecode(node, &location) && !location.volatile_access &&
        !location.atomic_access &&
        (location.kind == kIRMemoryIndirect ||
         location.kind == kIRMemoryStatic ||
         location.kind == kIRMemoryExtern)) {
      IRNode* val = StoredValue(node);
      if (val != NULL && val->type != NULL && !TypeIsVolatile(val->type) &&
          !IRIsVariable(val) && (IRIsConstant(val) || val->block != NULL)) {
        LoadInfo* info = calloc(1, sizeof(LoadInfo));
        info->load = node;
        info->value = val;
        info->location = location;
        info->linear = here;
        info->is_store = true;
        HashMapInsert(&ids, (uint64_t)(uintptr_t)node, (int64_t)loads.length);
        VectorAppend(&loads, info);
      }
    }
  }
  int num_loads = (int)loads.length;
  if (num_loads == 0) {
    HashMapDestruct(&ids);
    VectorDestruct(&loads);
    return;
  }

  Vector kill;
  Vector generated;
  Vector avail_out;
  VectorInit(&kill);
  VectorInit(&generated);
  VectorInit(&avail_out);
  VectorReserve(&kill, num_blocks);
  VectorReserve(&generated, num_blocks);
  VectorReserve(&avail_out, num_blocks);
  for (size_t b = 0; b < num_blocks; b++) {
    BitSet* k = NewBitSet();
    BitSet* g = NewBitSet();
    BitSet* ao = NewBitSet();
    VectorAppend(&kill, k);
    VectorAppend(&generated, g);
    VectorAppend(&avail_out, ao);
    BitSetFill(k, (size_t)num_loads);
    BitSetClear(k);
    BitSetFill(g, (size_t)num_loads);
    BitSetClear(g);
    BasicBlock* block = gen->basic_blocks.value.p[b];
    if (block == NULL) {
      continue;
    }
    BitSetFill(ao, (size_t)num_loads);
    BitSet live;
    BitSetInit(&live);
    BitSetFill(&live, (size_t)num_loads);
    BitSetClear(&live);
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      Clobber clobber = ClassifyClobber(inst);
      if (clobber.kind == kClobberAll) {
        BitSetFill(k, (size_t)num_loads);
        BitSetClear(&live);
      } else if (clobber.kind != kClobberNone) {
        for (int i = 0; i < num_loads; i++) {
          if (!ClobberHits(&clobber, loads.value.p[i], inst)) {
            continue;
          }
          BitSetInsert(k, (size_t)i);
          BitSetRemove(&live, (size_t)i);
        }
      }
      int id = (int)HashMapFind(&ids, (uint64_t)(uintptr_t)inst, -1);
      if (id >= 0) {
        BitSetInsert(&live, (size_t)id);
      }
    }
    BitSetCopy(g, &live);
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
  Vector queue;
  VectorInit(&queue);
  VectorResize(&queue, num_blocks + 1);
  size_t qhead = 0;
  size_t qtail = 0;
  for (size_t b = 0; b < num_blocks; b++) {
    if (gen->basic_blocks.value.p[b] == NULL) {
      continue;
    }
    queue.value.u[qtail++] = b;
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
    size_t b = queue.value.u[qhead++];
    if (qhead == num_blocks + 1) {
      qhead = 0;
    }
    BitSetRemove(&in_queue, b);
    BasicBlock* block = gen->basic_blocks.value.p[b];
    IntersectAvailable(gen, block, &avail_out, &in_set);
    TransferAvailable(&in_set, kill.value.p[b], generated.value.p[b], &next);
    if (BitSetEqual(&next, avail_out.value.p[b])) {
      continue;
    }
    BitSetCopy(avail_out.value.p[b], &next);
    for (size_t e = 0; e < block->out_edges.length; e++) {
      BlockId succ = block->out_edges.value.w[e];
      if (succ >= num_blocks || gen->basic_blocks.value.p[succ] == NULL ||
          BitSetContains(&in_queue, succ)) {
        continue;
      }
      queue.value.u[qtail++] = succ;
      if (qtail == num_blocks + 1) {
        qtail = 0;
      }
      BitSetInsert(&in_queue, succ);
    }
  }

  if (!failed) {
    Vector replacement;
    VectorInit(&replacement);
    VectorResize(&replacement, (size_t)num_loads);
    for (int i = 0; i < num_loads; i++) {
      replacement.value.w[i] = -1;
    }
    for (size_t b = 0; b < num_blocks; b++) {
      BasicBlock* block = gen->basic_blocks.value.p[b];
      if (block == NULL) {
        continue;
      }
      IntersectAvailable(gen, block, &avail_out, &in_set);
      for (IRNode* inst = BasicBlockBegin(block);
           !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
           inst = IRNext(inst)) {
        int id = (int)HashMapFind(&ids, (uint64_t)(uintptr_t)inst, -1);
        if (id >= 0 && !((LoadInfo*)loads.value.p[id])->is_store) {
          int best = -1;
          BitSetIterator it;
          BitSetIteratorStart(&it, &in_set);
          while (!BitSetIteratorDone(&it)) {
            int candidate = (int)BitSetIteratorValue(&it);
            BitSetIteratorNext(&it);
            if (!LoadCanFeed(loads.value.p[candidate], loads.value.p[id])) {
              continue;
            }
            if (best < 0 ||
                PreferLoad(loads.value.p[candidate], loads.value.p[best])) {
              best = candidate;
            }
          }
          replacement.value.w[id] = best;
        }
        Clobber clobber = ClassifyClobber(inst);
        KillAvailable(&in_set, &loads, &clobber, inst);
        if (id >= 0) {
          BitSetInsert(&in_set, (size_t)id);
        }
      }
    }
    // Resolve chains before editing the IR.  Clearing a link or deleting a
    // load while another load still names it would forward to a removed node.
    Vector final_id;
    VectorInit(&final_id);
    VectorResize(&final_id, (size_t)num_loads);
    for (int i = 0; i < num_loads; i++) {
      int final = i;
      int guard = num_loads;
      while (guard-- > 0 && replacement.value.w[final] >= 0) {
        final = (int)replacement.value.w[final];
      }
      final_id.value.w[i] =
          (final == i || replacement.value.w[final] >= 0) ? -1 : final;
    }
    for (int i = 0; i < num_loads; i++) {
      if (final_id.value.w[i] < 0) {
        continue;
      }
      LoadInfo* use_info = loads.value.p[i];
      LoadInfo* src_info = loads.value.p[final_id.value.w[i]];
      if (use_info->is_store || src_info->value == NULL ||
          !TypesMatchForForward(use_info->load, src_info->value)) {
        continue;
      }
      IRNode* load = use_info->load;
      BasicBlock* block = load->block;
      GeneratorReplaceInstruction(gen, load, src_info->value);
      BasicBlockRemoveInstruction(gen, block, load);
    }
    VectorDestruct(&final_id);
    VectorDestruct(&replacement);
  }

  VectorDestruct(&queue);
  BitSetDestruct(&in_queue);
  BitSetDestruct(&next);
  BitSetDestruct(&in_set);
  VectorDestructWithContents(&avail_out, (VectorElementDestructor)BitSetDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&generated, (VectorElementDestructor)BitSetDestruct,
                             /*free_element=*/true);
  VectorDestructWithContents(&kill, (VectorElementDestructor)BitSetDestruct,
                             /*free_element=*/true);
  HashMapDestruct(&ids);
  VectorDestructWithContents(&loads, NULL, /*free_element=*/true);
}

void MemoryOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    OptimizeBlockMemory(gen, gen->basic_blocks.value.p[i]);
  }
  CrossBlockLoadCSE(gen);
}
