//
//  target_shrink_wrap.c
//  c_compiler
//

#include "target_shrink_wrap.h"

#include <stdlib.h>
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


enum { kMaxSlow = 16, kMaxRegId = 64 };

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

static bool BlockFrameFree(const TargetShrinkWrapOps* sw, TargetBasicBlock* block,
                           bool allow_save, bool allow_reload, uint64_t* saved,
                           uint64_t* temp, uint64_t* arg, int* exec_count) {
  int cnt = 0;
  uint64_t sm = 0;
  uint64_t tm = 0;
  uint64_t am = 0;
  if (block == NULL || block->code == NULL) {
    return false;
  }
  for (TargetInstruction* inst = block->code; inst != NULL;
       inst = TargetNext(inst)) {
    TargetSWKind kind = sw->classify(sw->ctx, inst, allow_save, allow_reload,
                                    &sm, &tm, &am);
    if (kind == kTargetSWBad) {
      return false;
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

void TargetShrinkWrap(TargetGenerator* gen, const TargetShrinkWrapOps* sw) {
  if (sw == NULL || !sw->function_ok(sw->ctx, gen)) {
    return;
  }

  TargetInstruction* save_inst = NULL;
  TargetBasicBlock* epilogue = NULL;
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
          save_inst = p;
        }
      } else if (sw->is_restore(p)) {
        blk_has_restore = true;
        // A restore that does not fall into a separate return, such as a
        // 6502 leave that jumps to __leave, is the epilogue only when it
        // ends the block.  A leave placed before a tail call is not.
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
      epilogue = blk;
    } else if (blk_has_ret && !blk_has_restore) {
      return;
    }
  }
  if (save_count != 1 || save_inst == NULL || ret_epilogue_count != 1 ||
      epilogue == NULL || epilogue == gen->entry_block) {
    return;
  }

  // Code before the restore (a returned value, for example) stays in its
  // block.  The restore and the return become the pure epilogue.
  TargetInstruction* restore_inst = NULL;
  bool seen_restore = false;
  bool seen_ret = false;
  bool has_pre_restore_exec = false;
  for (TargetInstruction* p = epilogue->code; p != NULL; p = TargetNext(p)) {
    if (sw->is_restore(p) && !seen_restore && !seen_ret) {
      seen_restore = true;
      restore_inst = p;
      // Some targets return by the restore itself (6502 jumps to __leave).
      if (sw->is_return(p)) {
        seen_ret = true;
      }
    } else if (sw->is_return(p) && seen_restore && !seen_ret) {
      seen_ret = true;
    } else if (!sw->is_meta(sw->ctx, p)) {
      if (seen_restore) {
        return;
      }
      has_pre_restore_exec = true;
    }
    if (p == epilogue->end_code) {
      break;
    }
  }
  if (!seen_restore || !seen_ret || restore_inst == NULL) {
    return;
  }
  if (has_pre_restore_exec && restore_inst != epilogue->code) {
    TargetBasicBlock* pre_blk = epilogue;
    TargetInstruction* pre_end = TargetPrev(restore_inst);
    if (pre_end == NULL || pre_end->block != pre_blk) {
      return;
    }
    TargetBasicBlock* pure =
        NewTargetBasicBlock(gen->basic_blocks.length);
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
    epilogue = pure;
  }

  // Nothing is rewritten until every eligibility, fallthrough, spill, and
  // register-budget check has succeeded.
  size_t orig_blocks = gen->basic_blocks.length;
  size_t cap = orig_blocks + 32;
  BitSet eligible_pure, eligible_clone, can_reach_ret, sim_pure_fast, sim_clone,
      in_fast;
  Vector blk_saved, blk_temp, blk_arg, blk_exec, fall_succ, clone_of;
  BitSetInit(&eligible_pure);
  BitSetInit(&eligible_clone);
  BitSetInit(&can_reach_ret);
  BitSetInit(&sim_pure_fast);
  BitSetInit(&sim_clone);
  BitSetInit(&in_fast);
  Words(&blk_saved, cap);
  Words(&blk_temp, cap);
  Words(&blk_arg, cap);
  Words(&blk_exec, cap);
  Words(&fall_succ, cap);
  Words(&clone_of, cap);
  int* arg_coalesce = NULL;
  int* saved_reg_slot_idx = NULL;
  if (sw->num_saved_bits > 0) {
    arg_coalesce = malloc((size_t)sw->num_saved_bits * sizeof(int));
    for (int i = 0; i < sw->num_saved_bits; i++) {
      arg_coalesce[i] = -1;
    }
  }

  uint64_t incoming = sw->incoming_args(sw->ctx);
  for (size_t b = 0; b < orig_blocks; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk == epilogue || blk == gen->exit_block ||
        blk->code == NULL) {
      continue;
    }
    uint64_t sm = 0, tm = 0, am = 0;
    int ec = 0;
    // The reload-tolerant check overwrites the masks, and only runs when the
    // block is not already eligible without reloads.
    bool pure = BlockFrameFree(sw, blk, blk == gen->entry_block,
                               /*allow_reload=*/false, &sm, &tm, &am, &ec);
    if (pure) {
      BitSetInsert(&eligible_pure, b);
    }
    if (pure || (blk != gen->entry_block &&
                 BlockFrameFree(sw, blk, /*allow_save=*/false,
                                /*allow_reload=*/true, &sm, &tm, &am, &ec))) {
      BitSetInsert(&eligible_clone, b);
    }
    if (BitSetContains(&eligible_clone, b)) {
      SetWord(&blk_saved, b, sm);
      SetWord(&blk_temp, b, tm);
      SetWord(&blk_arg, b, am);
      SetWord(&blk_exec, b, (uint64_t)ec);
    }
    if (!sw->no_fallthrough(sw->ctx, blk->end_code)) {
      for (TargetInstruction* nxt = TargetNext(blk->end_code); nxt != NULL;
           nxt = TargetNext(nxt)) {
        if (nxt->block != NULL && nxt->block != blk) {
          VectorSet(&fall_succ, b, nxt->block);
          break;
        }
      }
    }
  }

  int coalesced_args = 0;
  if (sw->coalesce_copy != NULL && arg_coalesce != NULL) {
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
          bit >= 0 && bit < sw->num_saved_bits && arg_coalesce[bit] < 0) {
        arg_coalesce[bit] = k;
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
      if (arg_coalesce[i] >= 0) {
        coalesced_args++;
      }
    }
  }

  int max_scratch_pool = sw->scratch_base(sw->ctx) +
                         sw->extra_scratch(sw->ctx, incoming) + coalesced_args;

  bool reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      if (!BitSetContains(&eligible_clone, b) || BitSetContains(&can_reach_ret, b)) {
        continue;
      }
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == epilogue->block_id ||
            (succ_id < orig_blocks && BitSetContains(&can_reach_ret, succ_id))) {
          BitSetInsert(&can_reach_ret, b);
          reach_prog = true;
          break;
        }
      }
    }
  }

  bool has_fast_return = false;
  size_t num_slow_targets = 0;
  int num_sunk_slots = 0;
  int num_mapped = 0;
  size_t entry_id = gen->entry_block->block_id;
  if (!BitSetContains(&eligible_pure, entry_id) ||
      !BitSetContains(&can_reach_ret, entry_id)) {
    goto cleanup;
  }
  uint64_t total_saved = Word(&blk_saved, entry_id);
  uint64_t total_temp = Word(&blk_temp, entry_id);
  uint64_t total_arg = Word(&blk_arg, entry_id);
  if (PopCount(total_saved) > Room(max_scratch_pool, total_temp)) {
    goto cleanup;
  }
  BitSetInsert(&sim_pure_fast, entry_id);

  int cloned_insts = 0;
  int num_clones = 0;
  bool progress = true;
  while (progress) {
    progress = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      if (blk == NULL || BitSetContains(&sim_pure_fast, b) || BitSetContains(&sim_clone, b) ||
          !BitSetContains(&eligible_pure, b) || !BitSetContains(&can_reach_ret, b) || blk->in_edges.length == 0) {
        continue;
      }
      bool all_pure_fast = true;
      for (size_t e = 0; e < blk->in_edges.length; e++) {
        size_t pred_id = (size_t)blk->in_edges.value.w[e];
        if (pred_id >= orig_blocks || !BitSetContains(&sim_pure_fast, pred_id)) {
          all_pure_fast = false;
          break;
        }
      }
      if (!all_pure_fast) {
        continue;
      }
      if (b != entry_id) {
        TargetInstruction* prev = TargetPrev(blk->code);
        if (prev != NULL && prev->block != NULL &&
            (prev->block->block_id >= orig_blocks ||
             !BitSetContains(&sim_pure_fast, prev->block->block_id)) &&
            !sw->no_fallthrough(sw->ctx, prev)) {
          continue;
        }
      }
      uint64_t ns = total_saved | Word(&blk_saved, b);
      uint64_t nt = total_temp | Word(&blk_temp, b);
      if (PopCount(ns) <= Room(max_scratch_pool, nt)) {
        BitSetInsert(&sim_pure_fast, b);
        total_saved = ns;
        total_temp = nt;
        total_arg |= Word(&blk_arg, b);
        progress = true;
      }
    }
    if (progress) {
      continue;
    }
    // The cheapest block first, so the budget completes as many paths to the
    // epilogue as it can, and blocks inside loops last: cloning one only
    // peels the loop's first test.
    for (int pass = 0; pass < 2 && !progress; pass++) {
      size_t best = orig_blocks;
      for (size_t b = 0; b < orig_blocks && num_clones < 24; b++) {
        TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
        if (blk == NULL || BitSetContains(&sim_pure_fast, b) || BitSetContains(&sim_clone, b) ||
            (pass == 0 && blk->loop_nesting > 0) || !BitSetContains(&eligible_clone, b) ||
            !BitSetContains(&can_reach_ret, b) || (int)Word(&blk_exec, b) > 22 ||
            cloned_insts + (int)Word(&blk_exec, b) > 40 || blk->in_edges.length == 0) {
          continue;
        }
        bool has_fast_pred = false;
        bool has_slow_pred = false;
        bool forward_only = true;
        for (size_t e = 0; e < blk->in_edges.length; e++) {
          size_t pred_id = (size_t)blk->in_edges.value.w[e];
          if (pred_id < orig_blocks &&
              (BitSetContains(&sim_pure_fast, pred_id) || BitSetContains(&sim_clone, pred_id))) {
            has_fast_pred = true;
            if (pred_id >= b) {
              forward_only = false;
              break;
            }
            if (BitSetContains(&sim_clone, pred_id) || !BitSetContains(&eligible_pure, b)) {
              has_slow_pred = true;
            }
          } else {
            has_slow_pred = true;
          }
        }
        if (!has_fast_pred || !has_slow_pred || !forward_only) {
          continue;
        }
        uint64_t ns = total_saved | Word(&blk_saved, b);
        uint64_t nt = total_temp | Word(&blk_temp, b);
        if (PopCount(ns) > Room(max_scratch_pool, nt)) {
          continue;
        }
        if (best == orig_blocks || (int)Word(&blk_exec, b) < (int)Word(&blk_exec, best)) {
          best = b;
        }
      }
      if (best < orig_blocks) {
        BitSetInsert(&sim_clone, best);
        total_saved |= Word(&blk_saved, best);
        total_temp |= Word(&blk_temp, best);
        total_arg |= Word(&blk_arg, best);
        cloned_insts += (int)Word(&blk_exec, best);
        num_clones++;
        progress = true;
      }
    }
  }

  BitSet reaches_fast_ret;
  BitSetInit(&reaches_fast_ret);
  reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      if ((!BitSetContains(&sim_pure_fast, b) && !BitSetContains(&sim_clone, b)) || BitSetContains(&reaches_fast_ret, b)) {
        continue;
      }
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == epilogue->block_id ||
            (succ_id < orig_blocks && BitSetContains(&reaches_fast_ret, succ_id))) {
          BitSetInsert(&reaches_fast_ret, b);
          reach_prog = true;
          break;
        }
      }
    }
  }
  total_saved = 0;
  total_temp = 0;
  total_arg = 0;
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!BitSetContains(&reaches_fast_ret, b)) {
      BitSetRemove(&sim_pure_fast, b);
      BitSetRemove(&sim_clone, b);
    } else {
      total_saved |= Word(&blk_saved, b);
      total_temp |= Word(&blk_temp, b);
      total_arg |= Word(&blk_arg, b);
    }
  }
  BitSetDestruct(&reaches_fast_ret);
  if (!BitSetContains(&sim_pure_fast, entry_id)) {
    goto cleanup;
  }

  int slow_limit = sw->max_slow_targets > 0 ? sw->max_slow_targets : kMaxSlow;
  if (slow_limit > kMaxSlow) {
    slow_limit = kMaxSlow;
  }
  TargetBasicBlock* slow_targets[kMaxSlow];
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!BitSetContains(&sim_pure_fast, b) && !BitSetContains(&sim_clone, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (size_t o = 0; o < blk->out_edges.length; o++) {
      size_t succ_id = (size_t)blk->out_edges.value.w[o];
      TargetBasicBlock* succ = gen->basic_blocks.value.p[succ_id];
      if (succ == NULL || succ == gen->exit_block) {
        continue;
      }
      if (succ == epilogue) {
        has_fast_return = true;
      } else if (succ_id >= orig_blocks ||
                 (!BitSetContains(&sim_pure_fast, succ_id) && !BitSetContains(&sim_clone, succ_id))) {
        bool seen = false;
        for (size_t s = 0; s < num_slow_targets; s++) {
          if (slow_targets[s] == succ) {
            seen = true;
            break;
          }
        }
        if (!seen) {
          if ((int)num_slow_targets >= slow_limit) {
            goto cleanup;
          }
          slow_targets[num_slow_targets++] = succ;
        }
      }
    }
  }
  if (!has_fast_return || num_slow_targets == 0) {
    goto cleanup;
  }
  if (num_slow_targets > 1 &&
      (sw->emit_trampolines == NULL || sw->emit_dispatch == NULL)) {
    goto cleanup;
  }

  if (gen->entry_block->out_edges.length == 2) {
    size_t o0 = (size_t)gen->entry_block->out_edges.value.w[0];
    size_t o1 = (size_t)gen->entry_block->out_edges.value.w[1];
    bool f0 = o0 < orig_blocks && (BitSetContains(&sim_pure_fast, o0) || BitSetContains(&sim_clone, o0));
    bool f1 = o1 < orig_blocks && (BitSetContains(&sim_pure_fast, o1) || BitSetContains(&sim_clone, o1));
    if (f0 != f1 && sw->cold_entry_split(sw->ctx, gen->entry_block)) {
      goto cleanup;
    }
  }

  bool has_nontrivial_fast_work = false;
  for (size_t b = 0; b < orig_blocks && !has_nontrivial_fast_work; b++) {
    if (!BitSetContains(&sim_pure_fast, b) && !BitSetContains(&sim_clone, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (sw->is_nontrivial(sw->ctx, p)) {
        has_nontrivial_fast_work = true;
        break;
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  if (!has_nontrivial_fast_work) {
    goto cleanup;
  }

  for (size_t i = 0; i < num_slow_targets / 2; i++) {
    TargetBasicBlock* tmp = slow_targets[i];
    slow_targets[i] = slow_targets[num_slow_targets - 1 - i];
    slow_targets[num_slow_targets - 1 - i] = tmp;
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!BitSetContains(&sim_pure_fast, b) || b == entry_id) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* prev = TargetPrev(blk->code);
    if (prev != NULL && prev->block != NULL &&
        (prev->block->block_id >= orig_blocks ||
         !BitSetContains(&sim_pure_fast, prev->block->block_id)) &&
        !sw->no_fallthrough(sw->ctx, prev)) {
      goto cleanup;
    }
  }

  int64_t sunk_slot_off[kMaxSlow];
  int sunk_slot_reg[kMaxSlow];
  TargetInstruction* sunk_slot_proto[kMaxSlow];
  if (sw->num_saved_bits > 0) {
    saved_reg_slot_idx = malloc((size_t)sw->num_saved_bits * sizeof(int));
    for (int i = 0; i < sw->num_saved_bits; i++) {
      saved_reg_slot_idx[i] = -1;
    }
  }
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!BitSetContains(&sim_pure_fast, b) && !BitSetContains(&sim_clone, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (sw->is_spill(p)) {
        int sbit = -1;
        int reg_id = -1;
        int64_t soff = 0;
        if (sw->spill_info == NULL ||
            !sw->spill_info(sw->ctx, p, &sbit, &reg_id, &soff) ||
            saved_reg_slot_idx == NULL || sbit < 0 ||
            sbit >= sw->num_saved_bits) {
          goto cleanup;
        }
        int idx = -1;
        for (int s = 0; s < num_sunk_slots; s++) {
          if (sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx >= 0 || num_sunk_slots >= kMaxSlow ||
            saved_reg_slot_idx[sbit] >= 0) {
          goto cleanup;
        }
        idx = num_sunk_slots++;
        sunk_slot_off[idx] = soff;
        sunk_slot_reg[idx] = reg_id;
        sunk_slot_proto[idx] = p;
        saved_reg_slot_idx[sbit] = idx;
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  for (int idx = 0; idx < num_sunk_slots; idx++) {
    int64_t soff = sunk_slot_off[idx];
    TargetInstruction* proto = sunk_slot_proto[idx];
    bool clobbered = false;
    bool* reaches_reload = calloc(orig_blocks, sizeof(bool));
    for (size_t b = 0; b < orig_blocks && !clobbered; b++) {
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
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
        for (size_t b = 0; b < orig_blocks; b++) {
          if (reaches_reload[b] || b == proto->block->block_id) {
            continue;
          }
          TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
          if (blk == NULL) {
            continue;
          }
          for (size_t o = 0; o < blk->out_edges.length; o++) {
            size_t sid = (size_t)blk->out_edges.value.w[o];
            if (sid < orig_blocks && reaches_reload[sid]) {
              reaches_reload[b] = true;
              rprog = true;
              break;
            }
          }
        }
      }
      BitSet after_proto;
      BitSetInit(&after_proto);
      TargetBasicBlockReachableAfter(gen, proto->block, &after_proto);
      for (size_t b = 0; b < orig_blocks && !clobbered; b++) {
        if (!BitSetContains(&sim_pure_fast, b) && !BitSetContains(&sim_clone, b)) {
          continue;
        }
        bool is_proto_blk = (b == proto->block->block_id);
        if (!is_proto_blk &&
            (!BitSetContains(&after_proto, b) || !reaches_reload[b])) {
          continue;
        }
        TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
        bool past_proto = !is_proto_blk;
        for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
          if (p == proto) {
            past_proto = true;
          } else if (past_proto && !sw->is_spill(p) &&
                     !sw->is_meta(sw->ctx, p) &&
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
      goto cleanup;
    }
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!BitSetContains(&sim_clone, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (sw->is_reload(p)) {
        int64_t soff = 0;
        int idx = -1;
        if (sw->reload_offset == NULL ||
            !sw->reload_offset(sw->ctx, p, &soff)) {
          goto cleanup;
        }
        for (int s = 0; s < num_sunk_slots; s++) {
          if (sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx < 0 || !BitSetContains(&blk->dominators,
                                       sunk_slot_proto[idx]->block->block_id)) {
          goto cleanup;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  int saved_list[kMaxRegId];
  int temp_list[kMaxRegId];
  uint64_t taken_caller = 0;
  if (sw->take_temps != NULL) {
    sw->take_temps(sw->ctx, total_temp, &taken_caller);
  }
  if (arg_coalesce != NULL) {
    for (int bit = 0; bit < sw->num_saved_bits; bit++) {
      if ((total_saved & (1ull << bit)) == 0) {
        continue;
      }
      int k = arg_coalesce[bit];
      if (k < 0 || (taken_caller & (1ull << k)) != 0) {
        continue;
      }
      bool k_used_elsewhere = false;
      for (size_t b = 0; b < orig_blocks && !k_used_elsewhere; b++) {
        if (!BitSetContains(&sim_pure_fast, b) && !BitSetContains(&sim_clone, b)) {
          continue;
        }
        if (b != entry_id && (Word(&blk_arg, b) & (1ull << k)) != 0) {
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
            sw->record_reg(sw->ctx,
                           o->reg != NULL ? o->reg
                                          : (o->dest != NULL ? o->dest->reg : NULL),
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
        saved_list[num_mapped] = sw->saved_reg_id(sw->ctx, bit);
        temp_list[num_mapped] = k;
        taken_caller |= 1ull << k;
        num_mapped++;
      }
    }
  }

  int candidate_pool[kMaxRegId];
  int pool_len = 0;
  if (sw->scratch_pool != NULL) {
    pool_len = sw->scratch_pool(sw->ctx, candidate_pool, kMaxRegId, taken_caller,
                               total_arg, incoming);
  }
  int pool_pos = 0;
  for (int bit = 0; bit < sw->num_saved_bits; bit++) {
    if ((total_saved & (1ull << bit)) == 0) {
      continue;
    }
    int c = sw->saved_reg_id(sw->ctx, bit);
    bool already = false;
    for (int m = 0; m < num_mapped; m++) {
      if (saved_list[m] == c) {
        already = true;
        break;
      }
    }
    if (already) {
      continue;
    }
    if (pool_pos >= pool_len) {
      goto cleanup;
    }
    saved_list[num_mapped] = c;
    temp_list[num_mapped] = candidate_pool[pool_pos++];
    num_mapped++;
  }

  // Every check passed.  Clones first, so branches between them resolve.
  for (size_t b = 0; b < orig_blocks; b++) {
    if (BitSetContains(&sim_pure_fast, b)) {
      BitSetInsert(&in_fast, b);
    }
  }
  TargetInstruction* orig_clone_lbl[32] = {0};
  TargetInstruction* new_clone_lbl[32] = {0};
  int clone_idx = 0;
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!BitSetContains(&sim_clone, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* orig_lbl = EnsureLabel(sw, gen, blk);
    TargetBasicBlock* clone = NewTargetBasicBlock(gen->basic_blocks.length);
    size_t cid = clone->block_id;
    VectorAppend(&gen->basic_blocks, clone);
    TargetInstruction* clone_lbl = sw->new_label(sw->ctx);
    clone_lbl->block = clone;
    TargetEmit(gen, clone_lbl);
    clone->code = clone_lbl;
    clone->end_code = clone_lbl;
    TargetInstruction* cur = clone_lbl;
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != orig_lbl && !sw->omit_from_clone(sw->ctx, p)) {
        TargetInstruction* cp;
        if (sw->is_reload(p)) {
          int64_t soff = 0;
          int src_c = -1;
          sw->reload_offset(sw->ctx, p, &soff);
          for (int s = 0; s < num_sunk_slots; s++) {
            if (sunk_slot_off[s] == soff) {
              src_c = sunk_slot_reg[s];
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
        }
        cur = TargetShrinkWrapAppend(gen, clone, cp, cur);
      }
      if (p == blk->end_code) {
        break;
      }
    }
    VectorSet(&clone_of, b, clone);
    if (cid < cap) {
      BitSetInsert(&in_fast, cid);
      VectorSet(&fall_succ, cid, ((TargetBasicBlock*)VectorGet(&fall_succ, b)));
    }
    orig_clone_lbl[clone_idx] = orig_lbl;
    new_clone_lbl[clone_idx] = clone_lbl;
    clone_idx++;
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !BitSetContains(&in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = sw->branch_target(sw->ctx, p);
      if (slot != NULL && *slot != NULL) {
        for (int c = 0; c < clone_idx; c++) {
          if (*slot == orig_clone_lbl[c]) {
            *slot = new_clone_lbl[c];
            break;
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
    TargetBasicBlock* fall = (TargetBasicBlock*)VectorGet(&fall_succ, b);
    if (fall != NULL && fall->block_id < orig_blocks) {
      TargetBasicBlock* cloned =
          (TargetBasicBlock*)VectorGet(&clone_of, fall->block_id);
      if (cloned != NULL) {
        VectorSet(&fall_succ, b, cloned);
      }
    }
  }

  TargetInstruction* epilogue_lbl = EnsureLabel(sw, gen, epilogue);
  TargetInstruction* slow_lbls[kMaxSlow];
  for (size_t s = 0; s < num_slow_targets; s++) {
    slow_lbls[s] = EnsureLabel(sw, gen, slow_targets[s]);
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !BitSetContains(&in_fast, b) ||
        ((TargetBasicBlock*)VectorGet(&fall_succ, b)) == NULL) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (!sw->no_fallthrough(sw->ctx, blk->end_code)) {
      TargetInstruction* target_lbl = EnsureLabel(sw, gen, ((TargetBasicBlock*)VectorGet(&fall_succ, b)));
      TargetInstruction* br = sw->new_branch(sw->ctx, target_lbl);
      TargetShrinkWrapAppend(gen, blk, br, blk->end_code);
    }
  }

  TargetInstruction* fast_narrow[kMaxRegId] = {0};
  TargetInstruction* fast_wide[kMaxRegId] = {0};
  TargetInstruction* slow_narrow[kMaxRegId] = {0};
  TargetInstruction* slow_wide[kMaxRegId] = {0};
  for (int i = 0; i < num_mapped; i++) {
    int c = saved_list[i];
    int t = temp_list[i];
    if (sw->note_wrapped != NULL) {
      sw->note_wrapped(sw->ctx, c);
    }
    fast_narrow[c] = sw->proxy(sw->ctx, gen, t, 0);
    fast_wide[c] = sw->proxy(sw->ctx, gen, t, 1);
    slow_narrow[c] = sw->proxy(sw->ctx, gen, c, 0);
    slow_wide[c] = sw->proxy(sw->ctx, gen, c, 1);
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b < cap && BitSetContains(&in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    for (TargetInstruction* u = blk->code; u != NULL; u = TargetNext(u)) {
      if (u->dest != NULL && (sw->is_var == NULL || !sw->is_var(u->dest)) &&
          u->dest->block != NULL && u->dest->block->block_id < cap &&
          BitSetContains(&in_fast, u->dest->block->block_id) && u->dest->reg != NULL) {
        int id = 0;
        if (Remap(sw, u->dest->reg, &id) && slow_wide[id] != NULL) {
          u->dest = sw->value_width(sw->ctx, u->dest) == 0 ? slow_narrow[id]
                                                           : slow_wide[id];
        }
      }
      for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
        TargetInstruction* o = u->operand[k];
        if (o != NULL && (sw->is_var == NULL || !sw->is_var(o)) &&
            !sw->is_spill(o) && o->block != NULL && o->block->block_id < cap &&
            BitSetContains(&in_fast, o->block->block_id) && o->reg != NULL) {
          int id = 0;
          if (Remap(sw, o->reg, &id) && slow_wide[id] != NULL) {
            u->operand[k] = sw->value_width(sw->ctx, o) == 0 ? slow_narrow[id]
                                                             : slow_wide[id];
          }
        }
      }
      if (u == blk->end_code) {
        break;
      }
    }
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !BitSetContains(&in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != save_inst && !sw->is_spill(p) && !sw->is_meta(sw->ctx, p)) {
        int id = 0;
        if (Remap(sw, p->reg, &id) && fast_wide[id] != NULL) {
          p->reg = fast_wide[id]->reg;
        }
        if (p->dest != NULL && p->dest->reg != NULL &&
            Remap(sw, p->dest->reg, &id) && fast_wide[id] != NULL) {
          p->dest = sw->value_width(sw->ctx, p->dest) == 0 ? fast_narrow[id]
                                                           : fast_wide[id];
        }
        for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
          TargetInstruction* o = p->operand[k];
          if (o == NULL || sw->is_spill(o)) {
            continue;
          }
          TargetRegister* otr =
              o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL);
          if (Remap(sw, otr, &id) && fast_wide[id] != NULL) {
            p->operand[k] = sw->value_width(sw->ctx, o) == 0 ? fast_narrow[id]
                                                             : fast_wide[id];
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  if (gen->entry_block->code == save_inst) {
    gen->entry_block->code = TargetNext(save_inst);
  }
  if (gen->entry_block->end_code == save_inst) {
    gen->entry_block->end_code = TargetPrev(save_inst);
  }
  ListDeleteElement(&gen->code, &save_inst->header);

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!BitSetContains(&sim_pure_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
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

  // Fast blocks sit right after the entry, then the frameless return, then
  // one prologue block.  Keeping that prologue in one place is what an
  // unwind table for the function describes.
  TargetInstruction* cursor = gen->entry_block->end_code;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !BitSetContains(&in_fast, b) || b == entry_id) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
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
  if (num_slow_targets == 1) {
    tramp_lbls[0] = save_lbl;
  } else {
    cursor = sw->emit_trampolines(sw->ctx, gen, cursor, save_lbl,
                                  (int)num_slow_targets, tramp_lbls);
  }

  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !BitSetContains(&in_fast, b)) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = sw->branch_target(sw->ctx, p);
      if (slot != NULL && *slot != NULL) {
        if (*slot == epilogue_lbl) {
          *slot = fast_ret_lbl;
        } else {
          for (size_t s = 0; s < num_slow_targets; s++) {
            if (*slot == slow_lbls[s]) {
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
  cursor = TargetShrinkWrapAppend(gen, save_blk, save_inst, cursor);
  for (int i = 0; i < num_mapped; i++) {
    int c = saved_list[i];
    cursor = TargetShrinkWrapAppend(
        gen, save_blk,
        sw->create_move(sw->ctx, c, fast_wide[c], slow_wide[c]), cursor);
  }
  for (int idx = 0; idx < num_sunk_slots; idx++) {
    cursor = TargetShrinkWrapAppend(
        gen, save_blk,
        sw->clone_spill(sw->ctx, sunk_slot_proto[idx], sunk_slot_reg[idx]),
        cursor);
  }
  if (num_slow_targets == 1) {
    TargetShrinkWrapAppend(gen, save_blk, sw->new_branch(sw->ctx, slow_lbls[0]),
                           cursor);
  } else {
    sw->emit_dispatch(sw->ctx, gen, save_blk, cursor, slow_lbls,
                      (int)num_slow_targets);
  }

cleanup:
  BitSetDestruct(&eligible_pure);
  BitSetDestruct(&eligible_clone);
  BitSetDestruct(&can_reach_ret);
  BitSetDestruct(&sim_pure_fast);
  BitSetDestruct(&sim_clone);
  BitSetDestruct(&in_fast);
  VectorDestruct(&blk_saved);
  VectorDestruct(&blk_temp);
  VectorDestruct(&blk_arg);
  VectorDestruct(&blk_exec);
  VectorDestruct(&fall_succ);
  VectorDestruct(&clone_of);
  free(arg_coalesce);
  free(saved_reg_slot_idx);
}
