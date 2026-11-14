//
//  codemotion.c
//  c_compiler_library
//
//  Created by David Allison on 7/6/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "codemotion.h"

#include "alias.h"
#include "loop_info.h"

typedef struct {
  bool writes_memory;
  bool calls;
} LoopMemoryEffects;

static bool LoopContainsObservableCheckpoint(Generator* gen,
                                             const LoopInfo* loop) {
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (IRIsObservableCheckpoint(inst)) {
        return true;
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

static LoopMemoryEffects GetLoopMemoryEffects(Generator* gen,
                                              const LoopInfo* loop) {
  LoopMemoryEffects effects = {false, false};
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      effects.calls |= IRIsCall(inst);
      effects.writes_memory |=
          IRIsStore(inst) || IRAsmClobbersMemory(inst);
    }
    BitSetIteratorNext(&it);
  }
  return effects;
}

static bool IsLoopInvariant(const LoopInfo* loop, IRNode* inst) {
  for (size_t i = 0; i < inst->inputs.length; i++) {
    IRNode* input = inst->inputs.value.p[i];
    if (input->block != NULL && LoopInfoContainsBlock(loop, input->block)) {
      return false;
    }
  }
  return true;
}

static bool UsesVLAStorageImpl(IRNode* inst, Set* visited) {
  if (inst == NULL || SetContains(visited, inst)) {
    return false;
  }
  SetInsert(visited, inst);
  if (IRIsAutoVariable(inst) && TypeIsVLA(inst->type)) {
    return true;
  }
  for (size_t i = 0; i < inst->inputs.length; i++) {
    if (UsesVLAStorageImpl(inst->inputs.value.p[i], visited)) {
      return true;
    }
  }
  return false;
}

static bool UsesVLAStorage(IRNode* inst) {
  Set visited;
  SetInitForPointers(&visited);
  bool uses_vla = UsesVLAStorageImpl(inst, &visited);
  SetDestruct(&visited);
  return uses_vla;
}

// `base + small constant` used only as load/store addresses folds into those
// accesses; hoisting it would occupy a register for the whole loop instead.
static bool IsFoldedAddressOffset(IRNode* inst) {
  if (inst->opcode != IR_OP(adda) || inst->inputs.length != 2 ||
      !IRIsIntConst(inst->inputs.value.p[1])) {
    return false;
  }
  int64_t value = ((IRConstant*)inst->inputs.value.p[1])->value.ivalue;
  if (value < 0 || value >= 4096 || inst->outputs.length == 0) {
    return false;
  }
  for (size_t i = 0; i < inst->outputs.length; i++) {
    IRNode* user = inst->outputs.value.p[i];
    if ((!IRIsLoadOnly(user) && !IRIsStoreOnly(user)) ||
        user->inputs.length == 0 || user->inputs.value.p[0] != inst ||
        (user->inputs.length > 1 && user->inputs.value.p[1] == inst)) {
      return false;
    }
  }
  return true;
}

static bool IsHoistCandidate(Generator* gen, const LoopInfo* loop,
                             LoopMemoryEffects effects, Set* moved,
                             IRNode* inst) {
  if (IsFoldedAddressOffset(inst)) {
    return false;
  }
  if (inst->dest != NULL || inst->inputs.length == 0 ||
      IRHasSideEffects(inst) || IRIsVariable(inst) ||
      inst->opcode == IR_OP(literalref) ||
      SetContains(moved, inst) ||
      (!IRIsLoad(inst) && !IRIsExpression(inst)) ||
      !IsLoopInvariant(loop, inst) || UsesVLAStorage(inst)) {
    return false;
  }
  // A load is only invariant if nothing in the loop can write the location it
  // reads.  Ask the alias analysis about the loaded location itself: the
  // variable a load is annotated with describes where its address came from,
  // not what it reads, so a dereference of an unmodified pointer variable still
  // reads memory the loop may store to.
  if (IRIsLoadOnly(inst) && (effects.writes_memory || effects.calls)) {
    IRMemoryLocation location;
    if (!IRAliasDecode(inst, &location) ||
        location.volatile_access || location.atomic_access ||
        IRAliasLoopMayClobber(gen, loop, &location)) {
      return false;
    }
  }
  return true;
}

static void HoistToPreheader(Generator* gen, const LoopInfo* loop,
                             IRNode* inst) {
  BasicBlock* preheader = loop->preheader;
  BasicBlockMoveInstructionBefore(gen, inst, preheader->end_code);
}

// GVN keeps identical `base + small constant` nodes apart so that each can
// fold into the accesses that use it.  The ones hoisted here did not fold, so
// in the preheader they can share one register.
static IRNode* FindHoistedTwin(const LoopInfo* loop, Set* moved,
                               IRNode* inst) {
  if (inst->opcode != IR_OP(adda) || inst->inputs.length != 2) {
    return NULL;
  }
  BasicBlock* preheader = loop->preheader;
  for (IRNode* other = BasicBlockBegin(preheader);
       !BasicBlockIsEmpty(preheader) && other != BasicBlockEnd(preheader);
       other = IRNext(other)) {
    if (other->opcode == inst->opcode && other->inputs.length == 2 &&
        other->inputs.value.p[0] == inst->inputs.value.p[0] &&
        other->inputs.value.p[1] == inst->inputs.value.p[1] &&
        other->flags == inst->flags && TypeEqual(other->type, inst->type) &&
        SetContains(moved, other)) {
      return other;
    }
  }
  return NULL;
}

static bool HoistLoopInvariants(Generator* gen, LoopInfo* loop, Set* moved) {
  // Nested loops are eligible: inner preheaders sit inside the parent, so an
  // invariant moved inward can be considered again for the enclosing loop.
  // A per-loop `moved` set (not a function-wide one) is what makes that work.
  if (loop->preheader == NULL || BasicBlockIsEmpty(loop->preheader) ||
      LoopContainsObservableCheckpoint(gen, loop)) {
    return false;
  }
  LoopMemoryEffects effects = GetLoopMemoryEffects(gen, loop);
  bool changed = false;
  bool local_change;
  do {
    local_change = false;
    BitSetIterator it;
    BitSetIteratorStart(&it, &loop->blocks);
    while (!BitSetIteratorDone(&it)) {
      BasicBlock* block =
          VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
      IRNode* next = NULL;
      for (IRNode* inst = BasicBlockBegin(block);
           !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
           inst = next) {
        next = IRNext(inst);
        if (IsHoistCandidate(gen, loop, effects, moved, inst)) {
          IRNode* twin = FindHoistedTwin(loop, moved, inst);
          if (twin != NULL) {
            GeneratorReplaceInstruction(gen, inst, twin);
            BasicBlockRemoveInstruction(gen, block, inst);
          } else {
            HoistToPreheader(gen, loop, inst);
            SetInsert(moved, inst);
          }
          local_change = true;
          changed = true;
        }
      }
      BitSetIteratorNext(&it);
    }
  } while (local_change);
  return changed;
}

void CodeMotionOptimization(Generator* gen) {
  int max_depth = 0;
  for (size_t i = 0; i < gen->loops.length; i++) {
    LoopInfo* loop = gen->loops.value.p[i];
    if (loop->depth > max_depth) {
      max_depth = loop->depth;
    }
  }
  // Inner loops first.  An invariant moved to an inner preheader can then be
  // considered for movement out of its parent loop.  Each loop has its own
  // moved set so a hoist into an inner preheader is not frozen there.
  for (int depth = max_depth; depth > 0; depth--) {
    for (size_t i = 0; i < gen->loops.length; i++) {
      LoopInfo* loop = gen->loops.value.p[i];
      if (loop->depth == depth) {
        Set moved;
        SetInitForPointers(&moved);
        HoistLoopInvariants(gen, loop, &moved);
        SetDestruct(&moved);
      }
    }
  }
}

typedef struct {
  Generator* gen;
  Map ssa_versions;
  Map pooled_vars;
  Set incdec_symbols;
  bool changed;
} SinkContext;

static BasicBlock* NearestCommonDominator(BasicBlock* a, BasicBlock* b) {
  if (a == NULL) {
    return b;
  }
  if (b == NULL) {
    return a;
  }
  BasicBlock* cur = a;
  while (cur != NULL) {
    if (BitSetContains(&b->dominators, cur->block_id)) {
      return cur;
    }
    if (cur->idom == cur) {
      break;
    }
    cur = cur->idom;
  }
  return NULL;
}

static bool EntersNewLoop(Generator* gen, BasicBlock* from, BasicBlock* to) {
  if (to->loop_nesting > from->loop_nesting) {
    return true;
  }
  for (size_t i = 0; i < gen->loops.length; i++) {
    LoopInfo* loop = gen->loops.value.p[i];
    if (LoopInfoContainsBlock(loop, to) &&
        !LoopInfoContainsBlock(loop, from)) {
      return true;
    }
  }
  return false;
}

// True when some CFG path from `from` reaches function exit without visiting
// `cand` (i.e. `cand` is guarded by a branch that can bypass it).
static bool HasBypassPathToExit(Generator* gen, BasicBlock* from,
                                BasicBlock* cand) {
  BitSet visited;
  BitSetInit(&visited);
  Vector queue;
  VectorInit(&queue);
  BitSetInsert(&visited, from->block_id);
  VectorAppend(&queue, from);

  bool found = false;
  for (size_t head = 0; head < queue.length; head++) {
    BasicBlock* cur = queue.value.p[head];
    if (cur == gen->exit_block || cur->return_block ||
        cur->out_edges.length == 0) {
      found = true;
      break;
    }
    for (size_t i = 0; i < cur->out_edges.length; i++) {
      BlockId succ_id = (BlockId)cur->out_edges.value.w[i];
      if (succ_id == cand->block_id || BitSetContains(&visited, succ_id)) {
        continue;
      }
      BasicBlock* succ = VectorGet(&gen->basic_blocks, succ_id);
      if (succ == NULL || succ->is_unreachable) {
        continue;
      }
      BitSetInsert(&visited, succ_id);
      VectorAppend(&queue, succ);
    }
  }

  VectorDestruct(&queue);
  BitSetDestruct(&visited);
  return found;
}

// Collects all intermediate blocks on paths from `from` to `cand` without
// passing through `from`.  Returns false if `cand` can loop back to itself
// without passing through `from` (which could increase dynamic executions).
static bool CollectBlocksBetween(Generator* gen, BasicBlock* from,
                                 BasicBlock* cand, Vector* between) {
  BitSet visited;
  BitSetInit(&visited);
  Vector queue;
  VectorInit(&queue);

  for (size_t i = 0; i < cand->in_edges.length; i++) {
    BlockId pred_id = (BlockId)cand->in_edges.value.w[i];
    if (pred_id == cand->block_id) {
      VectorDestruct(&queue);
      BitSetDestruct(&visited);
      return false;
    }
    if (pred_id == from->block_id) {
      continue;
    }
    BasicBlock* pred = VectorGet(&gen->basic_blocks, pred_id);
    if (pred == NULL || pred->is_unreachable) {
      continue;
    }
    if (!BitSetContains(&pred->dominators, from->block_id)) {
      VectorDestruct(&queue);
      BitSetDestruct(&visited);
      return false;
    }
    if (!BitSetContains(&visited, pred_id)) {
      BitSetInsert(&visited, pred_id);
      VectorAppend(&queue, pred);
    }
  }

  for (size_t head = 0; head < queue.length; head++) {
    BasicBlock* cur = queue.value.p[head];
    VectorAppend(between, cur);
    for (size_t i = 0; i < cur->in_edges.length; i++) {
      BlockId pred_id = (BlockId)cur->in_edges.value.w[i];
      if (pred_id == cand->block_id) {
        VectorDestruct(&queue);
        BitSetDestruct(&visited);
        return false;
      }
      if (pred_id == from->block_id || BitSetContains(&visited, pred_id)) {
        continue;
      }
      BasicBlock* pred = VectorGet(&gen->basic_blocks, pred_id);
      if (pred == NULL || pred->is_unreachable) {
        continue;
      }
      if (!BitSetContains(&pred->dominators, from->block_id)) {
        VectorDestruct(&queue);
        BitSetDestruct(&visited);
        return false;
      }
      BitSetInsert(&visited, pred_id);
      VectorAppend(&queue, pred);
    }
  }

  VectorDestruct(&queue);
  BitSetDestruct(&visited);
  return true;
}

static IRNode* BlockSinkInsertPos(BasicBlock* block) {
  if (block == NULL || BasicBlockIsEmpty(block)) {
    return NULL;
  }
  for (IRNode* inst = block->code; inst != NULL; inst = IRNext(inst)) {
    if (inst->opcode != IR_OP(label) && inst->opcode != IR_OP(enter) &&
        inst->opcode != IR_OP(ssavar) && inst->opcode != IR_OP(phi)) {
      return inst;
    }
    if (inst == block->end_code) {
      break;
    }
  }
  return NULL;
}

static bool IsSinkablePureOpcode(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(movi):
    case IR_OP(mova):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(addi):
    case IR_OP(adda):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(subi):
    case IR_OP(suba):
    case IR_OP(subf):
    case IR_OP(subd):
    case IR_OP(muli):
    case IR_OP(mulf):
    case IR_OP(muld):
    case IR_OP(andi):
    case IR_OP(ori):
    case IR_OP(xori):
    case IR_OP(lsli):
    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(rotli):
    case IR_OP(rotri):
    case IR_OP(negi):
    case IR_OP(negf):
    case IR_OP(negd):
    case IR_OP(onescomp):
    case IR_OP(noti):
    case IR_OP(nota):
    case IR_OP(clzi):
    case IR_OP(ctzi):
    case IR_OP(popcounti):
    case IR_OP(zeroextendi):
    case IR_OP(signextendi):
    case IR_OP(cast):
    case IR_OP(addressof):
    case IR_OP(aligni):
    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2i):
    case IR_OP(d2i):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
    case IR_OP(cmpeqf):
    case IR_OP(cmpeqd):
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
    case IR_OP(cmpnef):
    case IR_OP(cmpned):
    case IR_OP(cmplti):
    case IR_OP(cmplta):
    case IR_OP(cmpltf):
    case IR_OP(cmpltd):
    case IR_OP(cmplei):
    case IR_OP(cmplea):
    case IR_OP(cmplef):
    case IR_OP(cmpled):
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgtd):
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
    case IR_OP(cmpgef):
    case IR_OP(cmpged):
      return true;
    default:
      return false;
  }
}

static bool IsSinkableSingleVersionSSAVar(SinkContext* ctx, IRNode* var) {
  if (var == NULL || var->opcode != IR_OP(ssavar)) {
    return false;
  }
  Symbol* sym = ((IRVariable*)var)->symbol;
  if (sym == NULL || sym->flags.address_taken || sym->flags.is_argument ||
      (!sym->flags.is_local && !sym->flags.is_temp) ||
      StorageIs(sym->storage, STO(static)) || sym->type == NULL ||
      TypeIsVolatile(sym->type) || TypeIsArray(sym->type) ||
      SymbolIsAnonymousUnionMemberView(sym) ||
      SetContains(&ctx->incdec_symbols, sym)) {
    return false;
  }
  MapKeyType key = {.p = sym};
  MapValueType* count = MapSearch(&ctx->ssa_versions, key);
  if (count == NULL || count->w != 1) {
    return false;
  }
  IRNode* pooled = MapFind(&ctx->pooled_vars, key);
  if (pooled != NULL && pooled->outputs.length != 0) {
    return false;
  }
  return true;
}

static bool LoadMayBeClobberedBeforeSink(IRNode* load, BasicBlock* from,
                                         const Vector* between) {
  IRMemoryLocation location;
  if (!IRAliasDecode(load, &location) || location.volatile_access ||
      location.atomic_access) {
    return true;
  }
  for (IRNode* cur = IRNext(load);
       cur != NULL && cur->block == from; cur = IRNext(cur)) {
    if (IRAliasInstMayClobber(&location, cur)) {
      return true;
    }
    if (cur == from->end_code) {
      break;
    }
  }
  for (size_t i = 0; i < between->length; i++) {
    BasicBlock* blk = between->value.p[i];
    if (blk == NULL || BasicBlockIsEmpty(blk)) {
      continue;
    }
    for (IRNode* cur = blk->code; cur != NULL; cur = IRNext(cur)) {
      if (IRAliasInstMayClobber(&location, cur)) {
        return true;
      }
      if (cur == blk->end_code) {
        break;
      }
    }
  }
  return false;
}

static void SinkInstructionsInBlock(BasicBlock* block, void* data) {
  SinkContext* ctx = data;
  Generator* gen = ctx->gen;
  if (block == NULL || block->is_unreachable || BasicBlockIsEmpty(block)) {
    return;
  }

  IRNode* prev = NULL;
  for (IRNode* inst = block->end_code;
       inst != NULL && inst->block == block; inst = prev) {
    prev = inst == block->code ? NULL : IRPrev(inst);

    if (inst == block->code && inst == block->end_code) {
      break;
    }
    if (inst->dest != NULL || UsesVLAStorage(inst)) {
      continue;
    }
    bool side_effect_input = false;
    for (size_t i = 0; i < inst->inputs.length; i++) {
      if (inst->inputs.value.p[i] != NULL &&
          IRHasSideEffects(inst->inputs.value.p[i])) {
        side_effect_input = true;
        break;
      }
    }
    if (side_effect_input) {
      continue;
    }

    bool is_pure = !IRHasSideEffects(inst) &&
                   IsSinkablePureOpcode(inst->opcode);
    bool is_load = !is_pure && IRIsLoadOnly(inst) && !IRHasSideEffects(inst);
    bool is_ssa_store = false;

    BasicBlock* ncd = NULL;
    if (is_pure || is_load) {
      if (inst->outputs.length == 0) {
        continue;
      }
      bool ok = true;
      for (size_t i = 0; i < inst->outputs.length; i++) {
        IRNode* user = inst->outputs.value.p[i];
        if (user != NULL && user->block != NULL && user->block->is_unreachable) {
          continue;
        }
        if (user == NULL || user->block == NULL ||
            user->opcode == IR_OP(phi) ||
            !BitSetContains(&user->block->dominators, block->block_id)) {
          ok = false;
          break;
        }
        ncd = NearestCommonDominator(ncd, user->block);
      }
      if (!ok || ncd == NULL || ncd == block) {
        continue;
      }
    } else if (IRIsStoreOnly(inst) && !IRIsObservableCheckpoint(inst) &&
               (inst->type == NULL || !TypeIsVolatile(inst->type)) &&
               inst->inputs.length == 2 && inst->outputs.length == 0 &&
               IsSinkableSingleVersionSSAVar(ctx, inst->inputs.value.p[0])) {
      IRNode* var = inst->inputs.value.p[0];
      bool ok = true;
      size_t num_loads = 0;
      for (size_t i = 0; i < var->outputs.length; i++) {
        IRNode* user = var->outputs.value.p[i];
        if (user == inst ||
            (user != NULL && user->block != NULL && user->block->is_unreachable)) {
          continue;
        }
        if (user == NULL || user->block == NULL ||
            !IRIsLoadOnly(user) || IRHasSideEffects(user) ||
            user->dest != NULL || user->inputs.length != 1 ||
            user->inputs.value.p[0] != var ||
            !BitSetContains(&user->block->dominators, block->block_id)) {
          ok = false;
          break;
        }
        num_loads++;
        ncd = NearestCommonDominator(ncd, user->block);
      }
      if (!ok || num_loads == 0 || ncd == NULL || ncd == block) {
        continue;
      }
      is_ssa_store = true;
    } else {
      continue;
    }
    (void)is_ssa_store;

    // Climb out of any loop that does not already contain `block`.
    BasicBlock* cand = ncd;
    while (cand != NULL && cand != block && EntersNewLoop(gen, block, cand)) {
      if (cand->idom == cand) {
        cand = NULL;
        break;
      }
      cand = cand->idom;
    }
    if (cand == NULL || cand == block) {
      continue;
    }

    // Climb to the highest ancestor below `block` if `cand` itself is not
    // bypassed on any exit path, or verify a valid non-cyclic region.
    while (cand != NULL && cand != block) {
      IRNode* insert_pos = BlockSinkInsertPos(cand);
      if (insert_pos != NULL && HasBypassPathToExit(gen, block, cand)) {
        Vector between;
        VectorInit(&between);
        bool region_ok = CollectBlocksBetween(gen, block, cand, &between);
        if (region_ok &&
            (!is_load || !LoadMayBeClobberedBeforeSink(inst, block, &between))) {
          VectorDestruct(&between);
          BasicBlockMoveInstructionBefore(gen, inst, insert_pos);
          ctx->changed = true;
          break;
        }
        VectorDestruct(&between);
      }
      if (cand->idom == cand) {
        break;
      }
      cand = cand->idom;
    }
  }
}

static void CollapseTrivialPhis(Generator* gen) {
  bool progress;
  do {
    progress = false;
    for (size_t b = 0; b < gen->basic_blocks.length; b++) {
      BasicBlock* block = gen->basic_blocks.value.p[b];
      if (block == NULL || block->is_unreachable || BasicBlockIsEmpty(block)) {
        continue;
      }
      IRNode* next = NULL;
      for (IRNode* inst = block->code; inst != NULL && inst->block == block;
           inst = next) {
        next = inst == block->end_code ? NULL : IRNext(inst);
        if (inst->opcode == IR_OP(ssavar) || inst->opcode == IR_OP(label) ||
            inst->opcode == IR_OP(enter)) {
          continue;
        }
        if (inst->opcode != IR_OP(phi)) {
          break;
        }
        if (inst->inputs.length == 0) {
          continue;
        }
        IRNode* same = NULL;
        bool trivial = true;
        for (size_t i = 0; i < inst->inputs.length; i++) {
          if (i < block->in_edges.length) {
            BasicBlock* pred =
                VectorGet(&gen->basic_blocks, block->in_edges.value.w[i]);
            if (pred == NULL || pred->is_unreachable) {
              continue;
            }
          }
          IRNode* in = inst->inputs.value.p[i];
          if (in == inst || in == NULL ||
              (in->block != NULL && in->block->is_unreachable)) {
            continue;
          }
          if (same == NULL) {
            same = in;
          } else if (same != in) {
            trivial = false;
            break;
          }
        }
        if (trivial && same != NULL) {
          GeneratorReplaceInstruction(gen, inst, same);
          BasicBlockRemoveInstruction(gen, block, inst);
          progress = true;
        }
      }
    }
  } while (progress);
}

bool CodeSinkingOptimization(Generator* gen) {
  if (gen->entry_block == NULL || gen->exception_ranges.length != 0) {
    return false;
  }
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (IRIsObservableCheckpoint(inst)) {
      return false;
    }
  }

  CollapseTrivialPhis(gen);

  SinkContext ctx;
  ctx.gen = gen;
  ctx.changed = false;
  MapInitForPointerKeys(&ctx.ssa_versions);
  MapInitForPointerKeys(&ctx.pooled_vars);
  SetInitForPointers(&ctx.incdec_symbols);

  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = gen->variable_pool.value.p[i];
    MapKeyValue kv = {.key.p = entry->value.symbol, .value.p = entry->pooled};
    MapInsert(&ctx.pooled_vars, kv);
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    BasicBlock* block = gen->basic_blocks.value.p[b];
    if (block == NULL || block->is_unreachable || BasicBlockIsEmpty(block)) {
      continue;
    }
    for (IRNode* inst = block->code; inst != NULL && inst->block == block;
         inst = IRNext(inst)) {
      if (IRIsVarDef(inst) && IRIsStore(inst) && !IRIsStoreOnly(inst) &&
          inst->var.def != NULL) {
        SetInsert(&ctx.incdec_symbols, inst->var.def);
      }
      if (inst->opcode == IR_OP(ssavar) || inst->opcode == IR_OP(phi)) {
        Symbol* sym = ((IRVariable*)inst)->symbol;
        if (sym != NULL) {
          MapKeyType key = {.p = sym};
          MapValueType* count = MapSearch(&ctx.ssa_versions, key);
          if (count != NULL) {
            count->w++;
          } else {
            MapKeyValue kv = {.key = key, .value.w = 1};
            MapInsert(&ctx.ssa_versions, kv);
          }
        }
      }
      if (inst == block->end_code) {
        break;
      }
    }
  }

  BasicBlockTraverseDominatorTree(gen, gen->entry_block,
                                  SinkInstructionsInBlock, kTraversePostOrder,
                                  &ctx);

  SetDestruct(&ctx.incdec_symbols);
  MapDestruct(&ctx.pooled_vars);
  MapDestruct(&ctx.ssa_versions);
  return ctx.changed;
}
