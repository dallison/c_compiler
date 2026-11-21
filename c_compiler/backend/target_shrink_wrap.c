//
//  target_shrink_wrap.c
//  c_compiler
//

#include "target_shrink_wrap.h"

#include <stdlib.h>
#include <string.h>
#include "target_basic_block.h"

// |n| zeroed slots.  Masks use the integer view; block pointers use VectorGet.
static void Words(Vector* v, size_t n) {
  VectorInit(v);
  if (n == 0) {
    return;
  }
  VectorReserve(v, n);
  v->length = n;
}

static uint64_t Word(Vector* v, size_t i) { return (uint64_t)v->value.w[i]; }

static void SetWord(Vector* v, size_t i, uint64_t value) {
  v->value.w[i] = (int64_t)value;
}


enum {
  kMaxSlow = 16,
  kMaxRegId = 64,
  kMaxClones = 24,
  kMaxCloneBlockInsts = 22,
  kMaxClonedInsts = 40,
};

static int PopCount(uint64_t x) {
  int c = 0;
  while (x != 0) {
    c += (int)(x & 1u);
    x >>= 1;
  }
  return c;
}

// A negative room means the temp registers already overflow the pool.  No
// callee-saved register fits then, and a fast region that saves nothing still
// does.
static int Room(int pool, uint64_t temps) {
  int room = pool - PopCount(temps);
  return room < 0 ? 0 : room;
}

TargetInstruction* TargetShrinkWrapAppend(TargetGenerator* gen,
                                          TargetBasicBlock* block,
                                          TargetInstruction* inst,
                                          TargetInstruction* cursor) {
  inst->block = block;
  ListInsertAfter(&gen->code, &inst->header, &cursor->header);
  if (block->code == NULL) {
    block->code = inst;
  }
  block->end_code = inst;
  return inst;
}

static bool RecordExec(const TargetShrinkWrapOps* sw, TargetInstruction* inst,
                       uint64_t* saved, uint64_t* temp, uint64_t* arg) {
  if (!sw->record_reg(sw->ctx, inst->reg, saved, temp, arg)) {
    return false;
  }
  if (inst->dest != NULL &&
      !sw->record_reg(sw->ctx, inst->dest->reg, saved, temp, arg)) {
    return false;
  }
  for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
    TargetInstruction* o = inst->operand[k];
    if (o == NULL) {
      continue;
    }
    TargetRegister* oreg =
        o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL);
    if (!sw->record_reg(sw->ctx, oreg, saved, temp, arg)) {
      return false;
    }
  }
  return true;
}

static bool IsTableJumpBlock(const TargetBasicBlock* block) {
  return block != NULL && block->end_code != NULL &&
         (block->end_code->flags & TARGET_INST_TABLE_JUMP) != 0;
}

static bool IsJumpTableEntryBlock(TargetGenerator* gen,
                                  const TargetBasicBlock* block) {
  if (block == NULL || block->end_code == NULL ||
      gen->virtuals->is_table_entry == NULL ||
      !gen->virtuals->is_table_entry(block->end_code)) {
    return false;
  }
  for (size_t i = 0; i < block->in_edges.length; i++) {
    size_t pred_id = (size_t)block->in_edges.value.w[i];
    if (pred_id < gen->basic_blocks.length &&
        IsTableJumpBlock(gen->basic_blocks.value.p[pred_id])) {
      return true;
    }
  }
  return false;
}

static bool BlockFrameFree(TargetGenerator* gen, const TargetShrinkWrapOps* sw,
                           TargetBasicBlock* block, bool allow_save,
                           bool allow_reload, uint64_t* saved, uint64_t* temp,
                           uint64_t* arg, int* exec_count) {
  int cnt = 0;
  uint64_t sm = 0;
  uint64_t tm = 0;
  uint64_t am = 0;
  if (block == NULL || block->code == NULL || IsTableJumpBlock(block) ||
      IsJumpTableEntryBlock(gen, block)) {
    return false;
  }
  for (TargetInstruction* inst = block->code; inst != NULL;
       inst = TargetNext(inst)) {
    if ((inst->flags & TARGET_INST_TABLE_JUMP) != 0) {
      return false;
    }
    TargetSWKind kind = sw->classify(sw->ctx, inst, allow_save, allow_reload,
                                    &sm, &tm, &am);
    if (kind == kTargetSWBad) {
      return false;
    }
    if (gen->virtuals->is_branch(inst) && !gen->virtuals->is_call(inst) &&
        !sw->is_return(inst)) {
      TargetInstruction** slot = sw->branch_target(sw->ctx, inst);
      if (slot == NULL || *slot == NULL || !sw->is_label(*slot)) {
        return false;
      }
    }
    if (kind == kTargetSWExec && !RecordExec(sw, inst, &sm, &tm, &am)) {
      return false;
    }
    if (kind == kTargetSWExec || kind == kTargetSWReload) {
      cnt++;
    }
    if (inst == block->end_code) {
      break;
    }
  }
  *saved = sm;
  *temp = tm;
  *arg = am;
  *exec_count = cnt;
  return true;
}

static TargetInstruction* EnsureLabel(const TargetShrinkWrapOps* sw,
                                     TargetGenerator* gen,
                                     TargetBasicBlock* block) {
  for (TargetInstruction* p = block->code; p != NULL; p = TargetNext(p)) {
    if (sw->is_label(p)) {
      return p;
    }
    if (!sw->is_meta(sw->ctx, p) || p == block->end_code) {
      break;
    }
  }
  TargetInstruction* lbl = sw->new_label(sw->ctx);
  lbl->block = block;
  TargetEmitBefore(gen, lbl, block->code);
  block->code = lbl;
  return lbl;
}

static bool Remap(const TargetShrinkWrapOps* sw, TargetRegister* reg, int* id) {
  return sw->remap_id != NULL && reg != NULL &&
         sw->remap_id(sw->ctx, reg, id) && *id >= 0 && *id < kMaxRegId;
}

// Working state for one function.  The bitsets and vectors cover the blocks
// that exist when analysis starts, plus room for the clones added later.
// A block id at or past |cap| is left out of the fast region.
typedef struct {
  TargetGenerator* gen;
  const TargetShrinkWrapOps* ops;

  TargetInstruction* save_inst;
  TargetBasicBlock* epilogue;
  TargetInstruction* epilogue_lbl;
  TargetInstruction* slow_lbls[kMaxSlow];

  size_t orig_blocks;
  size_t cap;
  size_t entry_id;

  BitSet eligible_pure;
  BitSet eligible_clone;
  BitSet can_reach_ret;
  BitSet sim_pure_fast;
  BitSet sim_clone;
  BitSet in_fast;
  Vector blk_saved;
  Vector blk_temp;
  Vector blk_arg;
  Vector blk_exec;
  Vector fall_succ;
  Vector clone_of;
  size_t* rpo_pos;

  int* arg_coalesce;
  int* saved_reg_slot_idx;
  uint64_t incoming;
  int max_scratch_pool;
  uint64_t total_saved;
  uint64_t total_temp;
  uint64_t total_arg;

  bool has_fast_return;
  size_t num_slow_targets;
  TargetBasicBlock* slow_targets[kMaxSlow];

  int num_sunk_slots;
  int64_t sunk_slot_off[kMaxSlow];
  int sunk_slot_reg[kMaxSlow];
  TargetInstruction* sunk_slot_proto[kMaxSlow];

  int num_mapped;
  int saved_list[kMaxRegId];
  int temp_list[kMaxRegId];
  // Scratch and callee-saved stand-ins, indexed by the callee-saved register.
  // The prologue copies one back to the other.
  TargetInstruction* fast_wide[kMaxRegId];
  TargetInstruction* slow_wide[kMaxRegId];
} ShrinkWrap;

static TargetBasicBlock* Block(ShrinkWrap* w, size_t id) {
  return w->gen->basic_blocks.value.p[id];
}

static bool Simulated(ShrinkWrap* w, size_t b) {
  return BitSetContains(&w->sim_pure_fast, b) || BitSetContains(&w->sim_clone, b);
}

static TargetBasicBlock* Fall(ShrinkWrap* w, size_t b) {
  return (TargetBasicBlock*)VectorGet(&w->fall_succ, b);
}

// The prologue is the one save in the entry block, and the epilogue is the
// one block that both restores and returns.  A return that never restores
// means some path already leaves without the prologue, so the function is
// left alone.  A restore that is itself the return counts only when it ends
// the block, so a restore placed in front of a tail call is not an epilogue.
static bool FindSinglePrologueAndEpilogue(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  int save_count = 0;
  int ret_epilogue_count = 0;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    bool blk_has_restore = false;
    bool blk_has_ret = false;
    bool restore_returns = false;
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (sw->is_save(p)) {
        save_count++;
        if (blk == gen->entry_block) {
          w->save_inst = p;
        }
      } else if (sw->is_restore(p)) {
        blk_has_restore = true;
        restore_returns = sw->is_return(p) && p == blk->end_code;
      } else if (sw->is_return(p)) {
        blk_has_ret = true;
      }
      if (p == blk->end_code) {
        break;
      }
    }
    if (blk_has_restore && (blk_has_ret || restore_returns)) {
      ret_epilogue_count++;
      w->epilogue = blk;
    } else if (blk_has_ret && !blk_has_restore) {
      return false;
    }
  }
  return save_count == 1 && w->save_inst != NULL && ret_epilogue_count == 1 &&
         w->epilogue != NULL && w->epilogue != gen->entry_block;
}

// Instructions before the restore, such as the code that puts a return value
// in its register, stay in the original block.  The restore and the return
// move into a new block, which becomes the epilogue.  This split is kept
// even when a later check declines to shrink-wrap.
static bool SplitEpilogueFromPrecedingCode(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  TargetBasicBlock* epilogue = w->epilogue;
  TargetInstruction* restore_inst = NULL;
  bool seen_restore = false;
  bool seen_ret = false;
  bool has_pre_restore_exec = false;
  for (TargetInstruction* p = epilogue->code; p != NULL; p = TargetNext(p)) {
    if (sw->is_restore(p) && !seen_restore && !seen_ret) {
      seen_restore = true;
      restore_inst = p;
      if (sw->is_return(p)) {
        seen_ret = true;
      }
    } else if (sw->is_return(p) && seen_restore && !seen_ret) {
      seen_ret = true;
    } else if (!sw->is_meta(sw->ctx, p)) {
      if (seen_restore) {
        return false;
      }
      has_pre_restore_exec = true;
    }
    if (p == epilogue->end_code) {
      break;
    }
  }
  if (!seen_restore || !seen_ret || restore_inst == NULL) {
    return false;
  }
  if (!has_pre_restore_exec || restore_inst == epilogue->code) {
    return true;
  }
  TargetBasicBlock* pre_blk = epilogue;
  TargetInstruction* pre_end = TargetPrev(restore_inst);
  if (pre_end == NULL || pre_end->block != pre_blk) {
    return false;
  }
  TargetBasicBlock* pure = NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, pure);
  pure->code = restore_inst;
  pure->end_code = pre_blk->end_code;
  pre_blk->end_code = pre_end;
  for (TargetInstruction* p = pure->code; p != NULL; p = TargetNext(p)) {
    p->block = pure;
    if (p == pure->end_code) {
      break;
    }
  }
  for (size_t o = 0; o < pre_blk->out_edges.length; o++) {
    size_t succ_id = (size_t)pre_blk->out_edges.value.w[o];
    VectorAppend(&pure->out_edges, (void*)(uintptr_t)succ_id);
    if (succ_id < gen->basic_blocks.length) {
      TargetBasicBlock* succ = gen->basic_blocks.value.p[succ_id];
      if (succ != NULL) {
        for (size_t e = 0; e < succ->in_edges.length; e++) {
          if ((size_t)succ->in_edges.value.w[e] == pre_blk->block_id) {
            succ->in_edges.value.w[e] = (intptr_t)pure->block_id;
          }
        }
      }
    }
  }
  pre_blk->out_edges.length = 0;
  VectorAppend(&pre_blk->out_edges, (void*)(uintptr_t)pure->block_id);
  VectorAppend(&pure->in_edges, (void*)(uintptr_t)pre_blk->block_id);
  BitSetCopy(&pure->dominators, &pre_blk->dominators);
  BitSetInsert(&pure->dominators, pure->block_id);
  pure->idom = pre_blk;
  w->epilogue = pure;
  return true;
}

static void ComputeRPO(ShrinkWrap* w) {
  size_t n = w->orig_blocks;
  w->rpo_pos = malloc((n > 0 ? n : 1) * sizeof(size_t));
  for (size_t i = 0; i < n; i++) {
    w->rpo_pos[i] = n;
  }
  if (w->entry_id >= n || Block(w, w->entry_id) == NULL) {
    return;
  }
  uint8_t* vis = calloc(n, sizeof(uint8_t));
  size_t* stk_blk = malloc(n * sizeof(size_t));
  size_t* stk_edge = malloc(n * sizeof(size_t));
  size_t* post = malloc(n * sizeof(size_t));
  size_t top = 0;
  size_t npost = 0;
  vis[w->entry_id] = 1;
  stk_blk[top] = w->entry_id;
  stk_edge[top] = 0;
  top++;
  while (top > 0) {
    size_t u = stk_blk[top - 1];
    size_t e = stk_edge[top - 1];
    TargetBasicBlock* blk = Block(w, u);
    if (blk != NULL && e < blk->out_edges.length) {
      stk_edge[top - 1] = e + 1;
      size_t v = (size_t)blk->out_edges.value.w[e];
      if (v < n && !vis[v] && Block(w, v) != NULL) {
        vis[v] = 1;
        stk_blk[top] = v;
        stk_edge[top] = 0;
        top++;
      }
    } else {
      top--;
      post[npost++] = u;
    }
  }
  for (size_t i = 0; i < npost; i++) {
    w->rpo_pos[post[i]] = npost - 1 - i;
  }
  free(post);
  free(stk_edge);
  free(stk_blk);
  free(vis);
}

// Allocates the per-block sets.  |cap| leaves room for the clones the fast
// region may add; a block id at or past |cap| is never treated as fast.
static void BeginAnalysis(ShrinkWrap* w) {
  w->orig_blocks = w->gen->basic_blocks.length;
  w->cap = w->orig_blocks + kMaxClones + 8;
  w->entry_id = w->gen->entry_block->block_id;
  ComputeRPO(w);
  BitSetInit(&w->eligible_pure);
  BitSetInit(&w->eligible_clone);
  BitSetInit(&w->can_reach_ret);
  BitSetInit(&w->sim_pure_fast);
  BitSetInit(&w->sim_clone);
  BitSetInit(&w->in_fast);
  Words(&w->blk_saved, w->cap);
  Words(&w->blk_temp, w->cap);
  Words(&w->blk_arg, w->cap);
  Words(&w->blk_exec, w->cap);
  Words(&w->fall_succ, w->cap);
  Words(&w->clone_of, w->cap);
  if (w->ops->num_saved_bits > 0) {
    w->arg_coalesce = malloc((size_t)w->ops->num_saved_bits * sizeof(int));
    for (int i = 0; i < w->ops->num_saved_bits; i++) {
      w->arg_coalesce[i] = -1;
    }
  }
}

static void EndAnalysis(ShrinkWrap* w) {
  BitSetDestruct(&w->eligible_pure);
  BitSetDestruct(&w->eligible_clone);
  BitSetDestruct(&w->can_reach_ret);
  BitSetDestruct(&w->sim_pure_fast);
  BitSetDestruct(&w->sim_clone);
  BitSetDestruct(&w->in_fast);
  VectorDestruct(&w->blk_saved);
  VectorDestruct(&w->blk_temp);
  VectorDestruct(&w->blk_arg);
  VectorDestruct(&w->blk_exec);
  VectorDestruct(&w->fall_succ);
  VectorDestruct(&w->clone_of);
  free(w->rpo_pos);
  free(w->arg_coalesce);
  free(w->saved_reg_slot_idx);
}

// Records which blocks can run with no frame, which of those can also run
// with reloads, and where each block falls through.  An argument register
// copied once into a callee-saved register is counted as an extra scratch
// register.  The entry starts the fast region when it needs no reload and
// can still reach the epilogue within the scratch budget.
static bool RecordBlockCosts(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  w->incoming = sw->incoming_args(sw->ctx);
  for (size_t b = 0; b < w->orig_blocks; b++) {
    TargetBasicBlock* blk = Block(w, b);
    if (blk == NULL || blk == w->epilogue || blk == gen->exit_block ||
        blk->code == NULL) {
      continue;
    }
    uint64_t sm = 0, tm = 0, am = 0;
    int ec = 0;
    // The reload-tolerant check overwrites the masks, and only runs when the
    // block is not already eligible without reloads.
    bool pure = BlockFrameFree(gen, sw, blk, blk == gen->entry_block,
                               /*allow_reload=*/false, &sm, &tm, &am, &ec);
    if (pure) {
      BitSetInsert(&w->eligible_pure, b);
    }
    if (pure || (blk != gen->entry_block &&
                 BlockFrameFree(gen, sw, blk, /*allow_save=*/false,
                                /*allow_reload=*/true, &sm, &tm, &am, &ec))) {
      BitSetInsert(&w->eligible_clone, b);
    }
    if (BitSetContains(&w->eligible_clone, b)) {
      SetWord(&w->blk_saved, b, sm);
      SetWord(&w->blk_temp, b, tm);
      SetWord(&w->blk_arg, b, am);
      SetWord(&w->blk_exec, b, (uint64_t)ec);
    }
    if (!sw->no_fallthrough(sw->ctx, blk->end_code)) {
      for (TargetInstruction* nxt = TargetNext(blk->end_code); nxt != NULL;
           nxt = TargetNext(nxt)) {
        if (nxt->block != NULL && nxt->block != blk) {
          VectorSet(&w->fall_succ, b, nxt->block);
          break;
        }
      }
    }
  }

  int coalesced_args = 0;
  if (sw->coalesce_copy != NULL && w->arg_coalesce != NULL) {
    uint64_t written_before = 0;
    for (TargetInstruction* p = gen->entry_block->code; p != NULL;
         p = TargetNext(p)) {
      if (sw->is_meta(sw->ctx, p)) {
        if (p == gen->entry_block->end_code) {
          break;
        }
        continue;
      }
      int bit = -1;
      int k = -1;
      if (sw->coalesce_copy(sw->ctx, p, written_before, &bit, &k) &&
          bit >= 0 && bit < sw->num_saved_bits && w->arg_coalesce[bit] < 0) {
        w->arg_coalesce[bit] = k;
      }
      uint64_t d1 = 0, d2 = 0, am = 0;
      sw->record_reg(sw->ctx, p->reg, &d1, &d2, &am);
      if (p->dest != NULL) {
        sw->record_reg(sw->ctx, p->dest->reg, &d1, &d2, &am);
      }
      written_before |= am;
      if (p == gen->entry_block->end_code) {
        break;
      }
    }
    for (int i = 0; i < sw->num_saved_bits; i++) {
      if (w->arg_coalesce[i] >= 0) {
        coalesced_args++;
      }
    }
  }
  w->max_scratch_pool = sw->scratch_base(sw->ctx) +
                        sw->extra_scratch(sw->ctx, w->incoming) + coalesced_args;

  bool reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < w->orig_blocks; b++) {
      if (!BitSetContains(&w->eligible_clone, b) ||
          BitSetContains(&w->can_reach_ret, b)) {
        continue;
      }
      TargetBasicBlock* blk = Block(w, b);
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == w->epilogue->block_id ||
            (succ_id < w->orig_blocks &&
             BitSetContains(&w->can_reach_ret, succ_id))) {
          BitSetInsert(&w->can_reach_ret, b);
          reach_prog = true;
          break;
        }
      }
    }
  }
  if (!BitSetContains(&w->eligible_pure, w->entry_id) ||
      !BitSetContains(&w->can_reach_ret, w->entry_id)) {
    return false;
  }
  w->total_saved = Word(&w->blk_saved, w->entry_id);
  w->total_temp = Word(&w->blk_temp, w->entry_id);
  w->total_arg = Word(&w->blk_arg, w->entry_id);
  if (PopCount(w->total_saved) > Room(w->max_scratch_pool, w->total_temp)) {
    return false;
  }
  BitSetInsert(&w->sim_pure_fast, w->entry_id);
  return true;
}

// The cheapest block the fast region may clone on this pass, or orig_blocks
// when none qualifies.  |skip_loops| leaves loop blocks for the second pass,
// so cloning one only peels the loop's first test.  A candidate needs a fast
// predecessor and a slow one, only forward edges in RPO, at most
// kMaxCloneBlockInsts real instructions, and room in the clone budget.
static size_t CheapestClone(ShrinkWrap* w, bool skip_loops, int num_clones,
                            int cloned_insts) {
  size_t best = w->orig_blocks;
  for (size_t b = 0; b < w->orig_blocks && num_clones < kMaxClones; b++) {
    TargetBasicBlock* blk = Block(w, b);
    if (blk == NULL || BitSetContains(&w->sim_pure_fast, b) ||
        BitSetContains(&w->sim_clone, b) ||
        (skip_loops && blk->loop_nesting > 0) ||
        !BitSetContains(&w->eligible_clone, b) ||
        !BitSetContains(&w->can_reach_ret, b) ||
        (int)Word(&w->blk_exec, b) > kMaxCloneBlockInsts ||
        cloned_insts + (int)Word(&w->blk_exec, b) > kMaxClonedInsts ||
        w->rpo_pos[b] >= w->orig_blocks ||
        blk->in_edges.length == 0) {
      continue;
    }
    bool has_fast_pred = false;
    bool has_slow_pred = false;
    bool forward_only = true;
    for (size_t e = 0; e < blk->in_edges.length; e++) {
      size_t pred_id = (size_t)blk->in_edges.value.w[e];
      if (pred_id < w->orig_blocks &&
          (BitSetContains(&w->sim_pure_fast, pred_id) ||
           BitSetContains(&w->sim_clone, pred_id))) {
        has_fast_pred = true;
        if (w->rpo_pos[pred_id] >= w->rpo_pos[b]) {
          forward_only = false;
          break;
        }
        if (BitSetContains(&w->sim_clone, pred_id) ||
            !BitSetContains(&w->eligible_pure, b)) {
          has_slow_pred = true;
        }
      } else {
        has_slow_pred = true;
      }
    }
    if (!has_fast_pred || !has_slow_pred || !forward_only) {
      continue;
    }
    uint64_t ns = w->total_saved | Word(&w->blk_saved, b);
    uint64_t nt = w->total_temp | Word(&w->blk_temp, b);
    if (PopCount(ns) > Room(w->max_scratch_pool, nt)) {
      continue;
    }
    if (best == w->orig_blocks ||
        (int)Word(&w->blk_exec, b) < (int)Word(&w->blk_exec, best)) {
      best = b;
    }
  }
  return best;
}

// Grows the fast region from the entry.  A block whose every predecessor is
// already fast, and that is reached by falling through from a fast block, is
// taken whole.  Otherwise the cheapest clonable block is taken, straight-line
// blocks before blocks inside loops.  The region stops when the scratch
// budget or the clone budget cannot take another block.
static void GrowFastRegion(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  int cloned_insts = 0;
  int num_clones = 0;
  bool progress = true;
  while (progress) {
    progress = false;
    for (size_t b = 0; b < w->orig_blocks; b++) {
      TargetBasicBlock* blk = Block(w, b);
      if (blk == NULL || BitSetContains(&w->sim_pure_fast, b) ||
          BitSetContains(&w->sim_clone, b) ||
          !BitSetContains(&w->eligible_pure, b) ||
          !BitSetContains(&w->can_reach_ret, b) || blk->in_edges.length == 0) {
        continue;
      }
      bool all_pure_fast = true;
      for (size_t e = 0; e < blk->in_edges.length; e++) {
        size_t pred_id = (size_t)blk->in_edges.value.w[e];
        if (pred_id >= w->orig_blocks ||
            !BitSetContains(&w->sim_pure_fast, pred_id)) {
          all_pure_fast = false;
          break;
        }
      }
      if (!all_pure_fast) {
        continue;
      }
      if (b != w->entry_id) {
        TargetInstruction* prev = TargetPrev(blk->code);
        if (prev != NULL && prev->block != NULL &&
            (prev->block->block_id >= w->orig_blocks ||
             !BitSetContains(&w->sim_pure_fast, prev->block->block_id)) &&
            !sw->no_fallthrough(sw->ctx, prev)) {
          continue;
        }
      }
      uint64_t ns = w->total_saved | Word(&w->blk_saved, b);
      uint64_t nt = w->total_temp | Word(&w->blk_temp, b);
      if (PopCount(ns) <= Room(w->max_scratch_pool, nt)) {
        BitSetInsert(&w->sim_pure_fast, b);
        w->total_saved = ns;
        w->total_temp = nt;
        w->total_arg |= Word(&w->blk_arg, b);
        progress = true;
      }
    }
    if (progress) {
      continue;
    }
    for (int pass = 0; pass < 2 && !progress; pass++) {
      size_t best = CheapestClone(w, pass == 0, num_clones, cloned_insts);
      if (best < w->orig_blocks) {
        BitSetInsert(&w->sim_clone, best);
        w->total_saved |= Word(&w->blk_saved, best);
        w->total_temp |= Word(&w->blk_temp, best);
        w->total_arg |= Word(&w->blk_arg, best);
        cloned_insts += (int)Word(&w->blk_exec, best);
        num_clones++;
        progress = true;
      }
    }
  }
}

// Drops fast blocks that cannot reach the epilogue through other fast blocks.
// The saved, temp, and argument masks are then rebuilt from what remains.
// The entry has to survive; otherwise there is no fast path.
static bool FastRegionStillReachesTheEpilogue(ShrinkWrap* w) {
  BitSet reaches;
  BitSetInit(&reaches);
  bool reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < w->orig_blocks; b++) {
      if (!Simulated(w, b) || BitSetContains(&reaches, b)) {
        continue;
      }
      TargetBasicBlock* blk = Block(w, b);
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == w->epilogue->block_id ||
            (succ_id < w->orig_blocks && BitSetContains(&reaches, succ_id))) {
          BitSetInsert(&reaches, b);
          reach_prog = true;
          break;
        }
      }
    }
  }
  w->total_saved = 0;
  w->total_temp = 0;
  w->total_arg = 0;
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!BitSetContains(&reaches, b)) {
      BitSetRemove(&w->sim_pure_fast, b);
      BitSetRemove(&w->sim_clone, b);
    } else {
      w->total_saved |= Word(&w->blk_saved, b);
      w->total_temp |= Word(&w->blk_temp, b);
      w->total_arg |= Word(&w->blk_arg, b);
    }
  }
  BitSetDestruct(&reaches);
  return BitSetContains(&w->sim_pure_fast, w->entry_id);
}

// The framed blocks the fast region branches to, other than the epilogue.
// There must be a path to the epilogue and at least one framed successor.
// More successors than the backend allows, or more than one when the backend
// has no trampoline and dispatch, declines.  The list is reversed so the
// last exit is the one that falls through the prologue.
static bool CollectFramedExits(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  int slow_limit = sw->max_slow_targets > 0 ? sw->max_slow_targets : kMaxSlow;
  if (slow_limit > kMaxSlow) {
    slow_limit = kMaxSlow;
  }
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!Simulated(w, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    for (size_t o = 0; o < blk->out_edges.length; o++) {
      size_t succ_id = (size_t)blk->out_edges.value.w[o];
      TargetBasicBlock* succ = Block(w, succ_id);
      if (succ == NULL || succ == w->gen->exit_block) {
        continue;
      }
      if (succ == w->epilogue) {
        w->has_fast_return = true;
      } else if (succ_id >= w->orig_blocks || !Simulated(w, succ_id)) {
        bool seen = false;
        for (size_t s = 0; s < w->num_slow_targets; s++) {
          if (w->slow_targets[s] == succ) {
            seen = true;
            break;
          }
        }
        if (!seen) {
          if ((int)w->num_slow_targets >= slow_limit) {
            return false;
          }
          w->slow_targets[w->num_slow_targets++] = succ;
        }
      }
    }
  }
  if (!w->has_fast_return || w->num_slow_targets == 0) {
    return false;
  }
  if (w->num_slow_targets > 1 &&
      (sw->emit_trampolines == NULL || sw->emit_dispatch == NULL)) {
    return false;
  }
  for (size_t i = 0; i < w->num_slow_targets / 2; i++) {
    TargetBasicBlock* tmp = w->slow_targets[i];
    w->slow_targets[i] = w->slow_targets[w->num_slow_targets - 1 - i];
    w->slow_targets[w->num_slow_targets - 1 - i] = tmp;
  }
  return true;
}

// A fast region that is only a null or zero test on the entry is left alone,
// and so is one whose blocks do no real work.  Moving the prologue there
// would add a branch on the common path and save it only on the cold path.
static bool FastRegionDoesRealWork(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetBasicBlock* entry = w->gen->entry_block;
  if (entry->out_edges.length == 2) {
    size_t o0 = (size_t)entry->out_edges.value.w[0];
    size_t o1 = (size_t)entry->out_edges.value.w[1];
    bool f0 = o0 == w->epilogue->block_id ||
              (o0 < w->orig_blocks && Simulated(w, o0));
    bool f1 = o1 == w->epilogue->block_id ||
              (o1 < w->orig_blocks && Simulated(w, o1));
    if (f0 != f1 && sw->cold_entry_split(sw->ctx, entry)) {
      return false;
    }
  }
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!Simulated(w, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (sw->is_nontrivial(sw->ctx, p)) {
        return true;
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  return false;
}

// A fast block other than the entry must not be reached by falling out of a
// block outside the fast region.  That edge would run the block without
// having executed the prologue.
static bool FastBlocksHaveNoSlowFallthrough(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!BitSetContains(&w->sim_pure_fast, b) || b == w->entry_id) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    TargetInstruction* prev = TargetPrev(blk->code);
    if (prev != NULL && prev->block != NULL &&
        (prev->block->block_id >= w->orig_blocks ||
         !BitSetContains(&w->sim_pure_fast, prev->block->block_id)) &&
        !sw->no_fallthrough(sw->ctx, prev)) {
      return false;
    }
  }
  return true;
}

// Each spill in the fast region must be the only spill of one callee-saved
// register into one slot.  The spill can move to the prologue when every
// reload is dominated by it and the fast region does not overwrite that
// register between the spill and a reload.  A reload in a block that will be
// cloned has to be one of those spills, dominated by the same block.
static bool SpillsCanMoveToThePrologue(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  if (sw->num_saved_bits > 0) {
    w->saved_reg_slot_idx = malloc((size_t)sw->num_saved_bits * sizeof(int));
    for (int i = 0; i < sw->num_saved_bits; i++) {
      w->saved_reg_slot_idx[i] = -1;
    }
  }
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!Simulated(w, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (sw->is_spill(p)) {
        int sbit = -1;
        int reg_id = -1;
        int64_t soff = 0;
        if (sw->spill_info == NULL ||
            !sw->spill_info(sw->ctx, p, &sbit, &reg_id, &soff) ||
            w->saved_reg_slot_idx == NULL || sbit < 0 ||
            sbit >= sw->num_saved_bits) {
          return false;
        }
        int idx = -1;
        for (int s = 0; s < w->num_sunk_slots; s++) {
          if (w->sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx >= 0 || w->num_sunk_slots >= kMaxSlow ||
            w->saved_reg_slot_idx[sbit] >= 0) {
          return false;
        }
        idx = w->num_sunk_slots++;
        w->sunk_slot_off[idx] = soff;
        w->sunk_slot_reg[idx] = reg_id;
        w->sunk_slot_proto[idx] = p;
        w->saved_reg_slot_idx[sbit] = idx;
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  for (int idx = 0; idx < w->num_sunk_slots; idx++) {
    int64_t soff = w->sunk_slot_off[idx];
    TargetInstruction* proto = w->sunk_slot_proto[idx];
    bool clobbered = false;
    bool* reaches_reload = calloc(w->orig_blocks, sizeof(bool));
    for (size_t b = 0; b < w->orig_blocks && !clobbered; b++) {
      TargetBasicBlock* blk = Block(w, b);
      if (blk == NULL || blk->code == NULL) {
        continue;
      }
      for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
        int64_t roff = 0;
        if (sw->is_reload(p) && sw->reload_offset != NULL &&
            sw->reload_offset(sw->ctx, p, &roff) && roff == soff) {
          if (!BitSetContains(&blk->dominators, proto->block->block_id)) {
            clobbered = true;
            break;
          }
          reaches_reload[b] = true;
          break;
        }
        if (p == blk->end_code) {
          break;
        }
      }
    }
    if (!clobbered) {
      bool rprog = true;
      while (rprog) {
        rprog = false;
        for (size_t b = 0; b < w->orig_blocks; b++) {
          if (reaches_reload[b] || b == proto->block->block_id) {
            continue;
          }
          TargetBasicBlock* blk = Block(w, b);
          if (blk == NULL) {
            continue;
          }
          for (size_t o = 0; o < blk->out_edges.length; o++) {
            size_t sid = (size_t)blk->out_edges.value.w[o];
            if (sid < w->orig_blocks && reaches_reload[sid]) {
              reaches_reload[b] = true;
              rprog = true;
              break;
            }
          }
        }
      }
      BitSet after_proto;
      BitSetInit(&after_proto);
      TargetBasicBlockReachableAfter(w->gen, proto->block, &after_proto);
      for (size_t b = 0; b < w->orig_blocks && !clobbered; b++) {
        if (!Simulated(w, b)) {
          continue;
        }
        bool is_proto_blk = (b == proto->block->block_id);
        if (!is_proto_blk &&
            (!BitSetContains(&after_proto, b) || !reaches_reload[b])) {
          continue;
        }
        TargetBasicBlock* blk = Block(w, b);
        bool past_proto = !is_proto_blk;
        for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
          if (p == proto) {
            past_proto = true;
          } else if (past_proto && !sw->is_spill(p) && !sw->is_meta(sw->ctx, p) &&
                     sw->clobbers_saved != NULL &&
                     sw->clobbers_saved(sw->ctx, proto, p)) {
            clobbered = true;
            break;
          }
          if (p == blk->end_code) {
            break;
          }
        }
      }
      BitSetDestruct(&after_proto);
    }
    free(reaches_reload);
    if (clobbered) {
      return false;
    }
  }

  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!BitSetContains(&w->sim_clone, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (sw->is_reload(p)) {
        int64_t soff = 0;
        int idx = -1;
        if (sw->reload_offset == NULL || !sw->reload_offset(sw->ctx, p, &soff)) {
          return false;
        }
        for (int s = 0; s < w->num_sunk_slots; s++) {
          if (w->sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx < 0 || !BitSetContains(&blk->dominators,
                                       w->sunk_slot_proto[idx]->block->block_id)) {
          return false;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  return true;
}

// An incoming argument that the entry copies once into a callee-saved
// register stays in that argument register.  Every other callee-saved
// register the fast region uses takes the next caller-saved register from
// the backend's pool.  Running out of scratch declines.
static bool MapCalleeSavedOntoScratch(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  uint64_t taken_caller = 0;
  if (sw->take_temps != NULL) {
    sw->take_temps(sw->ctx, w->total_temp, &taken_caller);
  }
  if (w->arg_coalesce != NULL) {
    for (int bit = 0; bit < sw->num_saved_bits; bit++) {
      if ((w->total_saved & (1ull << bit)) == 0) {
        continue;
      }
      int k = w->arg_coalesce[bit];
      if (k < 0 || (taken_caller & (1ull << k)) != 0) {
        continue;
      }
      bool k_used_elsewhere = false;
      for (size_t b = 0; b < w->orig_blocks && !k_used_elsewhere; b++) {
        if (!Simulated(w, b)) {
          continue;
        }
        if (b != w->entry_id && (Word(&w->blk_arg, b) & (1ull << k)) != 0) {
          k_used_elsewhere = true;
        }
      }
      if (k_used_elsewhere) {
        continue;
      }
      int count_k_in_entry = 0;
      for (TargetInstruction* p = gen->entry_block->code; p != NULL;
           p = TargetNext(p)) {
        if (sw->is_meta(sw->ctx, p) || sw->is_spill(p)) {
          if (p == gen->entry_block->end_code) {
            break;
          }
          continue;
        }
        uint64_t d1 = 0, d2 = 0, am = 0;
        sw->record_reg(sw->ctx, p->reg, &d1, &d2, &am);
        if (p->dest != NULL) {
          sw->record_reg(sw->ctx, p->dest->reg, &d1, &d2, &am);
        }
        for (int opi = 0; opi < TARGET_MAX_OPERANDS; opi++) {
          TargetInstruction* o = p->operand[opi];
          if (o != NULL) {
            sw->record_reg(
                sw->ctx,
                o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL),
                &d1, &d2, &am);
          }
        }
        if ((am & (1ull << k)) != 0) {
          count_k_in_entry++;
        }
        if (p == gen->entry_block->end_code) {
          break;
        }
      }
      if (count_k_in_entry == 1) {
        w->saved_list[w->num_mapped] = sw->saved_reg_id(sw->ctx, bit);
        w->temp_list[w->num_mapped] = k;
        taken_caller |= 1ull << k;
        w->num_mapped++;
      }
    }
  }

  int candidate_pool[kMaxRegId];
  int pool_len = 0;
  if (sw->scratch_pool != NULL) {
    pool_len = sw->scratch_pool(sw->ctx, candidate_pool, kMaxRegId, taken_caller,
                               w->total_arg, w->incoming);
  }
  int pool_pos = 0;
  for (int bit = 0; bit < sw->num_saved_bits; bit++) {
    if ((w->total_saved & (1ull << bit)) == 0) {
      continue;
    }
    int c = sw->saved_reg_id(sw->ctx, bit);
    bool already = false;
    for (int m = 0; m < w->num_mapped; m++) {
      if (w->saved_list[m] == c) {
        already = true;
        break;
      }
    }
    if (already) {
      continue;
    }
    if (pool_pos >= pool_len) {
      return false;
    }
    w->saved_list[w->num_mapped] = c;
    w->temp_list[w->num_mapped] = candidate_pool[pool_pos++];
    w->num_mapped++;
  }
  return true;
}

// Copies each block that is in the fast region only on the fast edge.  Spills,
// labels, and other instructions the backend omits are not copied.  A reload
// of a sunk slot becomes a copy from the scratch register that holds it.
// Branches in the fast region that targeted a cloned block are pointed at the
// clone, and fallthrough edges that landed on a cloned block follow the clone.
static void CloneBlocksOnTheFastEdge(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (BitSetContains(&w->sim_pure_fast, b)) {
      BitSetInsert(&w->in_fast, b);
    }
  }
  Vector orig_clone_lbl;
  Vector new_clone_lbl;
  Vector block_map_orig;
  Vector block_map_clone;
  VectorInit(&orig_clone_lbl);
  VectorInit(&new_clone_lbl);
  VectorInit(&block_map_orig);
  VectorInit(&block_map_clone);
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!BitSetContains(&w->sim_clone, b)) {
      continue;
    }
    block_map_orig.length = 0;
    block_map_clone.length = 0;
    TargetBasicBlock* blk = Block(w, b);
    TargetInstruction* orig_lbl = EnsureLabel(sw, gen, blk);
    TargetBasicBlock* clone = NewTargetBasicBlock(gen->basic_blocks.length);
    size_t cid = clone->block_id;
    VectorAppend(&gen->basic_blocks, clone);
    TargetInstruction* clone_lbl = sw->new_label(sw->ctx);
    clone_lbl->block = clone;
    TargetEmit(gen, clone_lbl);
    clone->code = clone_lbl;
    clone->end_code = clone_lbl;
    VectorAppend(&block_map_orig, orig_lbl);
    VectorAppend(&block_map_clone, clone_lbl);
    VectorAppend(&orig_clone_lbl, orig_lbl);
    VectorAppend(&new_clone_lbl, clone_lbl);
    TargetInstruction* cur = clone_lbl;
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != orig_lbl) {
        TargetInstruction* cp = NULL;
        if (sw->is_label(p)) {
          // Preserve intra-block labels (such as exported PC-relative anchor
          // labels attached to auipc/lea_rip) with fresh label IDs on the
          // clone so %pcrel_lo operands in the clone resolve to the cloned PC.
          cp = sw->new_label(sw->ctx);
          cp->flags |= (p->flags & ~TARGET_INST_EXCEPTION_LANDING);
          VectorAppend(&orig_clone_lbl, p);
          VectorAppend(&new_clone_lbl, cp);
        } else if (!sw->is_meta(sw->ctx, p) && !sw->omit_from_clone(sw->ctx, p)) {
          if (sw->is_reload(p)) {
            int64_t soff = 0;
            int src_c = -1;
            sw->reload_offset(sw->ctx, p, &soff);
            for (int s = 0; s < w->num_sunk_slots; s++) {
              if (w->sunk_slot_off[s] == soff) {
                src_c = w->sunk_slot_reg[s];
                break;
              }
            }
            cp = sw->clone_reload(sw->ctx, gen, p, src_c);
          } else {
            cp = TargetNewInstruction(p->opcode);
            cp->flags = p->flags;
            cp->reg = p->reg;
            cp->dest = p->dest;
            for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
              cp->operand[k] = p->operand[k];
            }
            for (size_t m = 0; m < block_map_orig.length; m++) {
              TargetInstruction* from =
                  (TargetInstruction*)VectorGet(&block_map_orig, m);
              TargetInstruction* to =
                  (TargetInstruction*)VectorGet(&block_map_clone, m);
              if (cp->dest == from) {
                cp->dest = to;
              }
              for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
                if (cp->operand[k] == from) {
                  cp->operand[k] = to;
                }
              }
            }
          }
        }
        if (cp != NULL) {
          VectorAppend(&block_map_orig, p);
          VectorAppend(&block_map_clone, cp);
          cur = TargetShrinkWrapAppend(gen, clone, cp, cur);
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
    VectorSet(&w->clone_of, b, clone);
    if (cid < w->cap) {
      BitSetInsert(&w->in_fast, cid);
      VectorSet(&w->fall_succ, cid, Fall(w, b));
    }
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= w->cap || !BitSetContains(&w->in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = sw->branch_target(sw->ctx, p);
      if (slot != NULL && *slot != NULL) {
        for (size_t c = 0; c < orig_clone_lbl.length; c++) {
          if (*slot == (TargetInstruction*)VectorGet(&orig_clone_lbl, c)) {
            *slot = (TargetInstruction*)VectorGet(&new_clone_lbl, c);
            break;
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
    TargetBasicBlock* fall = Fall(w, b);
    if (fall != NULL && fall->block_id < w->orig_blocks) {
      TargetBasicBlock* cloned =
          (TargetBasicBlock*)VectorGet(&w->clone_of, fall->block_id);
      if (cloned != NULL) {
        VectorSet(&w->fall_succ, b, cloned);
      }
    }
  }
  VectorDestruct(&block_map_orig);
  VectorDestruct(&block_map_clone);
  VectorDestruct(&orig_clone_lbl);
  VectorDestruct(&new_clone_lbl);
}

// Blocks that used to fall into the next instruction get an explicit branch.
// The fast blocks are about to be gathered together, so fallthrough would
// land in the wrong place.  Labels for the epilogue and the framed exits are
// recorded here, before anything is moved, so later branches can retarget them.
static void BranchAroundTheMovedPrologue(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  w->epilogue_lbl = EnsureLabel(sw, gen, w->epilogue);
  for (size_t s = 0; s < w->num_slow_targets; s++) {
    w->slow_lbls[s] = EnsureLabel(sw, gen, w->slow_targets[s]);
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= w->cap || !BitSetContains(&w->in_fast, b) || Fall(w, b) == NULL) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    if (!sw->no_fallthrough(sw->ctx, blk->end_code)) {
      TargetInstruction* target_lbl = EnsureLabel(sw, gen, Fall(w, b));
      TargetInstruction* br = sw->new_branch(sw->ctx, target_lbl);
      TargetShrinkWrapAppend(gen, blk, br, blk->end_code);
    }
  }
}

// Inside the fast region, a callee-saved register is renamed to the scratch
// register chosen for it.  Outside the fast region, a use of a value defined
// in the fast region names the callee-saved register the prologue restores.
static void RenameFastRegisters(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  TargetInstruction* fast_narrow[kMaxRegId] = {0};
  TargetInstruction* slow_narrow[kMaxRegId] = {0};
  for (int i = 0; i < w->num_mapped; i++) {
    int c = w->saved_list[i];
    int t = w->temp_list[i];
    if (sw->note_wrapped != NULL) {
      sw->note_wrapped(sw->ctx, c);
    }
    fast_narrow[c] = sw->proxy(sw->ctx, gen, t, 0);
    w->fast_wide[c] = sw->proxy(sw->ctx, gen, t, 1);
    slow_narrow[c] = sw->proxy(sw->ctx, gen, c, 0);
    w->slow_wide[c] = sw->proxy(sw->ctx, gen, c, 1);
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b < w->cap && BitSetContains(&w->in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    for (TargetInstruction* u = blk->code; u != NULL; u = TargetNext(u)) {
      if (u->dest != NULL && (sw->is_var == NULL || !sw->is_var(u->dest)) &&
          u->dest->block != NULL && u->dest->block->block_id < w->cap &&
          BitSetContains(&w->in_fast, u->dest->block->block_id) &&
          u->dest->reg != NULL) {
        int id = 0;
        if (Remap(sw, u->dest->reg, &id) && w->slow_wide[id] != NULL) {
          u->dest = sw->value_width(sw->ctx, u->dest) == 0 ? slow_narrow[id]
                                                           : w->slow_wide[id];
        }
      }
      for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
        TargetInstruction* o = u->operand[k];
        if (o != NULL && (sw->is_var == NULL || !sw->is_var(o)) &&
            !sw->is_spill(o) && o->block != NULL &&
            o->block->block_id < w->cap &&
            BitSetContains(&w->in_fast, o->block->block_id) && o->reg != NULL) {
          int id = 0;
          if (Remap(sw, o->reg, &id) && w->slow_wide[id] != NULL) {
            u->operand[k] = sw->value_width(sw->ctx, o) == 0 ? slow_narrow[id]
                                                             : w->slow_wide[id];
          }
        }
      }
      if (u == blk->end_code) {
        break;
      }
    }
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= w->cap || !BitSetContains(&w->in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != w->save_inst && !sw->is_spill(p) && !sw->is_meta(sw->ctx, p)) {
        int id = 0;
        if (Remap(sw, p->reg, &id) && w->fast_wide[id] != NULL) {
          p->reg = w->fast_wide[id]->reg;
        }
        if (p->dest != NULL && p->dest->reg != NULL &&
            Remap(sw, p->dest->reg, &id) && w->fast_wide[id] != NULL) {
          p->dest = sw->value_width(sw->ctx, p->dest) == 0 ? fast_narrow[id]
                                                           : w->fast_wide[id];
        }
        for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
          TargetInstruction* o = p->operand[k];
          if (o == NULL || sw->is_spill(o)) {
            continue;
          }
          TargetRegister* otr =
              o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL);
          if (Remap(sw, otr, &id) && w->fast_wide[id] != NULL) {
            p->operand[k] = sw->value_width(sw->ctx, o) == 0 ? fast_narrow[id]
                                                             : w->fast_wide[id];
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
}

// Unlinks the save from the entry.  The prologue block inserts it again
// after the fast path.
static void RemoveTheEntrySave(ShrinkWrap* w) {
  TargetGenerator* gen = w->gen;
  TargetInstruction* save_inst = w->save_inst;
  if (gen->entry_block->code == save_inst) {
    gen->entry_block->code = TargetNext(save_inst);
  }
  if (gen->entry_block->end_code == save_inst) {
    gen->entry_block->end_code = TargetPrev(save_inst);
  }
  ListDeleteElement(&gen->code, &save_inst->header);
}

// Deletes the original spills from blocks taken whole into the fast region.
// The prologue emits those stores after the save.  Clones never copied them.
static void RemoveSunkSpills(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  for (size_t b = 0; b < w->orig_blocks; b++) {
    if (!BitSetContains(&w->sim_pure_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    TargetInstruction* p = blk->code;
    while (p != NULL) {
      bool is_end = (p == blk->end_code);
      TargetInstruction* nxt = TargetNext(p);
      if (sw->is_spill(p)) {
        if (blk->code == p) {
          blk->code = nxt;
        }
        if (blk->end_code == p) {
          blk->end_code = TargetPrev(p);
        }
        ListDeleteElement(&gen->code, &p->header);
        TargetTrackOrphanInstruction(gen, p);
      }
      if (is_end) {
        break;
      }
      p = nxt;
    }
  }
}

// Places the fast blocks immediately after the entry, then a frameless
// return, then one prologue block.  Keeping the prologue in one place is
// what an unwind table for the function describes.  Branches that targeted
// the epilogue now target the frameless return, and branches that targeted a
// framed exit now target that exit's trampoline.  The prologue saves, copies
// scratch registers back to the callee-saved registers, re-emits the sunk
// spills, and continues into the framed region.
static void LayOutFastPathAndPrologue(ShrinkWrap* w) {
  const TargetShrinkWrapOps* sw = w->ops;
  TargetGenerator* gen = w->gen;
  TargetInstruction* cursor = gen->entry_block->end_code;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= w->cap || !BitSetContains(&w->in_fast, b) || b == w->entry_id) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    TargetInstruction* p = blk->code;
    while (p != NULL) {
      bool is_end = (p == blk->end_code);
      TargetInstruction* nxt = TargetNext(p);
      if (p != cursor) {
        ListDeleteElement(&gen->code, &p->header);
        ListInsertAfter(&gen->code, &p->header, &cursor->header);
        cursor = p;
      }
      if (is_end) {
        break;
      }
      p = nxt;
    }
  }

  TargetBasicBlock* fast_ret_blk = NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, fast_ret_blk);
  TargetInstruction* fast_ret_lbl = sw->new_label(sw->ctx);
  cursor = TargetShrinkWrapAppend(gen, fast_ret_blk, fast_ret_lbl, cursor);
  cursor = TargetShrinkWrapAppend(gen, fast_ret_blk, sw->new_return(sw->ctx),
                                  cursor);

  TargetInstruction* save_lbl = sw->new_label(sw->ctx);
  TargetInstruction* tramp_lbls[kMaxSlow] = {0};
  if (w->num_slow_targets == 1) {
    tramp_lbls[0] = save_lbl;
  } else {
    cursor = sw->emit_trampolines(sw->ctx, gen, cursor, save_lbl,
                                  (int)w->num_slow_targets, tramp_lbls);
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= w->cap || !BitSetContains(&w->in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = sw->branch_target(sw->ctx, p);
      if (slot != NULL && *slot != NULL) {
        if (*slot == w->epilogue_lbl) {
          *slot = fast_ret_lbl;
        } else {
          for (size_t s = 0; s < w->num_slow_targets; s++) {
            if (*slot == w->slow_lbls[s]) {
              *slot = tramp_lbls[s];
              break;
            }
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  TargetBasicBlock* save_blk = NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, save_blk);
  cursor = TargetShrinkWrapAppend(gen, save_blk, save_lbl, cursor);
  cursor = TargetShrinkWrapAppend(gen, save_blk, w->save_inst, cursor);
  for (int i = 0; i < w->num_mapped; i++) {
    int c = w->saved_list[i];
    cursor = TargetShrinkWrapAppend(
        gen, save_blk,
        sw->create_move(sw->ctx, c, w->fast_wide[c], w->slow_wide[c]), cursor);
  }
  for (int idx = 0; idx < w->num_sunk_slots; idx++) {
    cursor = TargetShrinkWrapAppend(
        gen, save_blk,
        sw->clone_spill(sw->ctx, w->sunk_slot_proto[idx], w->sunk_slot_reg[idx]),
        cursor);
  }
  if (w->num_slow_targets == 1) {
    TargetShrinkWrapAppend(gen, save_blk, sw->new_branch(sw->ctx, w->slow_lbls[0]),
                           cursor);
  } else {
    sw->emit_dispatch(sw->ctx, gen, save_blk, cursor, w->slow_lbls,
                      (int)w->num_slow_targets);
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= w->cap || !BitSetContains(&w->in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = Block(w, b);
    if (blk == NULL || blk->end_code == NULL) {
      continue;
    }
    TargetInstruction* end = blk->end_code;
    if (!sw->no_fallthrough(sw->ctx, end) || sw->is_return(end)) {
      continue;
    }
    TargetInstruction** slot = sw->branch_target(sw->ctx, end);
    if (slot == NULL || *slot == NULL) {
      continue;
    }
    bool redundant = false;
    for (TargetInstruction* nxt = TargetNext(end); nxt != NULL;
         nxt = TargetNext(nxt)) {
      if (nxt == *slot) {
        redundant = true;
        break;
      }
      if (!sw->is_label(nxt) && !sw->is_meta(sw->ctx, nxt)) {
        break;
      }
    }
    if (redundant) {
      TargetInstruction* prev = TargetPrev(end);
      TargetInstruction* after = TargetNext(end);
      if (blk->code == end) {
        blk->code = (after != NULL && after->block == blk) ? after : NULL;
      }
      blk->end_code = (prev != NULL && prev->block == blk) ? prev : NULL;
      ListDeleteElement(&gen->code, &end->header);
      TargetTrackOrphanInstruction(gen, end);
    }
  }
}

void TargetShrinkWrap(TargetGenerator* gen, const TargetShrinkWrapOps* sw) {
  if (sw == NULL || !sw->function_ok(sw->ctx, gen)) {
    return;
  }

  ShrinkWrap w;
  memset(&w, 0, sizeof(w));
  w.gen = gen;
  w.ops = sw;
  // The epilogue split is kept when a later check declines.  Nothing else is
  // rewritten until every check below has passed.
  if (!FindSinglePrologueAndEpilogue(&w) ||
      !SplitEpilogueFromPrecedingCode(&w)) {
    return;
  }
  BeginAnalysis(&w);
  if (RecordBlockCosts(&w)) {
    GrowFastRegion(&w);
    if (FastRegionStillReachesTheEpilogue(&w) && CollectFramedExits(&w) &&
        FastRegionDoesRealWork(&w) && FastBlocksHaveNoSlowFallthrough(&w) &&
        SpillsCanMoveToThePrologue(&w) && MapCalleeSavedOntoScratch(&w)) {
      CloneBlocksOnTheFastEdge(&w);
      BranchAroundTheMovedPrologue(&w);
      RenameFastRegisters(&w);
      RemoveTheEntrySave(&w);
      RemoveSunkSpills(&w);
      LayOutFastPathAndPrologue(&w);
    }
  }
  EndAnalysis(&w);
}
