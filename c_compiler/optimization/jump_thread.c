#include "jump_thread.h"

#include <stdint.h>
#include <stdlib.h>

#include "basic_block.h"
#include "bitset.h"
#include "codegen.h"
#include "compiler.h"
#include "dce.h"
#include "hashmap.h"
#include "ir.h"
#include "loop_info.h"
#include "map.h"
#include "set.h"
#include "symbol.h"
#include "type.h"
#include "vector.h"

typedef enum {
  kJoinNone = 0,
  kJoinForwarder,
  kJoinVarBranch,
} JoinKind;

typedef struct {
  JoinKind kind;
  BasicBlock* dest;
  IRNode* var;
  Symbol* sym;
  IRNode* load;
  bool branch_on_true;
  BasicBlock* taken;
  BasicBlock* fall;
} JoinBlockInfo;

static bool IsAnchoredDebugLoc(const Generator* gen, const IRNode* inst) {
  return gen->debug_range_anchors != NULL && inst->opcode == IR_OP(loc) &&
         HashMapContains(gen->debug_range_anchors, (uint64_t)(uintptr_t)inst);
}

static bool LabelHasOnlyBranchUsers(const Generator* gen, const IRNode* label) {
  if (label == NULL || label == gen->return_label ||
      (label->flags & kIRNrvoMarker) != 0) {
    return false;
  }
  for (size_t i = 0; i < label->outputs.length; i++) {
    const IRNode* user = label->outputs.value.p[i];
    if (user == NULL || (user->flags & kIRNrvoMarker) != 0) {
      return false;
    }
    if (user->opcode != IR_OP(bra) && user->opcode != IR_OP(btrue) &&
        user->opcode != IR_OP(bfalse)) {
      return false;
    }
  }
  return true;
}

static bool IsSafeThreadableSymbol(Symbol* sym) {
  if (sym == NULL || sym->type == NULL || sym->flags.is_argument ||
      (!sym->flags.is_local && !sym->flags.is_temp) ||
      sym->flags.address_taken || SymbolIsAnonymousUnionMemberView(sym) ||
      StorageIs(sym->storage, STO(static) | STO(extern) | STO(thread)) ||
      TypeIsVolatile(sym->type)) {
    return false;
  }
  return TypeIsIntegral(sym->type) || TypeIsPointer(sym->type);
}

static void CollectJumpTableBlocks(Generator* gen, BitSet* jump_table_blocks) {
  for (IRNode* inst = GeneratorFirstInstruction(gen); inst != NULL;
       inst = IRNext(inst)) {
    if (inst->block != NULL && (inst->flags & kIRJumpTableBranch) != 0) {
      BitSetInsert(jump_table_blocks, inst->block->block_id);
    }
    if (inst->opcode == IR_OP(cbra)) {
      for (IRNode* b = IRNext(inst); b != NULL && b->opcode == IR_OP(bra);
           b = IRNext(b)) {
        if (b->block != NULL) {
          BitSetInsert(jump_table_blocks, b->block->block_id);
        }
      }
    }
  }
}

static bool ClassifyJoinBlock(Generator* gen, BasicBlock* block,
                              const BitSet* jump_table_blocks,
                              JoinBlockInfo* info) {
  info->kind = kJoinNone;
  if (block == NULL || block == gen->entry_block || block == gen->exit_block ||
      block->return_block || BasicBlockIsEmpty(block) ||
      BasicBlockIsUnreachable(gen, block) ||
      BitSetContains(jump_table_blocks, block->block_id)) {
    return false;
  }

  IRNode* seen_load = NULL;
  IRNode* seen_var = NULL;
  Symbol* seen_sym = NULL;

  for (IRNode* inst = BasicBlockBegin(block); inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (IRIsObservableCheckpoint(inst)) {
      return false;
    }
    if (inst->opcode == IR_OP(label) &&
        (inst == block->code || inst->outputs.length == 0)) {
      if (!LabelHasOnlyBranchUsers(gen, inst)) {
        return false;
      }
      continue;
    }
    if (inst->opcode == IR_OP(loc)) {
      if (IsAnchoredDebugLoc(gen, inst)) {
        return false;
      }
      continue;
    }
    if (inst == block->end_code && inst->opcode == IR_OP(bra)) {
      if (seen_load == NULL &&
          (inst->flags & (kIRJumpTableBranch | kIRNrvoMarker)) == 0 &&
          block->out_edges.length == 1) {
        BasicBlock* dest =
            VectorGet(&gen->basic_blocks, block->out_edges.value.w[0]);
        if (dest != NULL && dest != block && dest != gen->exit_block &&
            !BasicBlockIsEmpty(dest)) {
          info->kind = kJoinForwarder;
          info->dest = dest;
          return true;
        }
      }
      return false;
    }
    if (IRIsConditionalBranch(inst)) {
      if (seen_load == NULL || (inst->flags & kIRNrvoMarker) != 0 ||
          inst->inputs.length < 2 || inst->inputs.value.p[0] != seen_load ||
          block->out_edges.length != 2) {
        return false;
      }
      BasicBlock* fall = NULL;
      IRNode* target_lbl = inst->inputs.value.p[1];
      if (target_lbl == NULL || target_lbl->block == NULL) {
        return false;
      }
      BasicBlock* taken = target_lbl->block;
      if (inst == block->end_code) {
        BasicBlock* b0 =
            VectorGet(&gen->basic_blocks, block->out_edges.value.w[0]);
        BasicBlock* b1 =
            VectorGet(&gen->basic_blocks, block->out_edges.value.w[1]);
        fall = (taken == b0) ? b1 : ((taken == b1) ? b0 : NULL);
      } else {
        IRNode* tail = IRNext(inst);
        while (tail != NULL && tail != block->end_code &&
               tail->opcode == IR_OP(loc) && !IRIsObservableCheckpoint(tail) &&
               !IsAnchoredDebugLoc(gen, tail)) {
          tail = IRNext(tail);
        }
        if (tail == block->end_code && tail->opcode == IR_OP(bra) &&
            (tail->flags & (kIRJumpTableBranch | kIRNrvoMarker)) == 0 &&
            tail->inputs.length >= 1 && tail->inputs.value.p[0] != NULL) {
          fall = ((IRNode*)tail->inputs.value.p[0])->block;
        }
      }
      if (taken == NULL || fall == NULL || taken == fall || taken == block ||
          fall == block || taken == gen->exit_block ||
          fall == gen->exit_block || BasicBlockIsEmpty(taken) ||
          BasicBlockIsEmpty(fall)) {
        return false;
      }
      info->kind = kJoinVarBranch;
      info->var = seen_var;
      info->sym = seen_sym;
      info->load = seen_load;
      info->branch_on_true = (inst->opcode == IR_OP(btrue));
      info->taken = taken;
      info->fall = fall;
      return true;
    }
    if (seen_load == NULL && inst != block->end_code && IRIsLoadOnly(inst) &&
        inst->dest == NULL && inst->inputs.length == 1 &&
        inst->outputs.length == 1 && inst->type != NULL &&
        (TypeIsIntegral(inst->type) || TypeIsPointer(inst->type)) &&
        !TypeIsVolatile(inst->type)) {
      IRNode* var = inst->inputs.value.p[0];
      if (var != NULL &&
          (var->opcode == IR_OP(localvar) || var->opcode == IR_OP(tempvar))) {
        Symbol* sym = IRGetVariableSymbol(var);
        if (IsSafeThreadableSymbol(sym)) {
          seen_load = inst;
          seen_var = var;
          seen_sym = sym;
          continue;
        }
      }
    }
    return false;
  }

  if (seen_load == NULL && !IRIsBranch(block->end_code) &&
      !IRIsReturn(block->end_code) && block->out_edges.length == 1) {
    BasicBlock* dest =
        VectorGet(&gen->basic_blocks, block->out_edges.value.w[0]);
    if (dest != NULL && dest != block && dest != gen->exit_block &&
        !BasicBlockIsEmpty(dest)) {
      info->kind = kJoinForwarder;
      info->dest = dest;
      return true;
    }
  }
  return false;
}

static bool CanThreadEdgeTo(Generator* gen, BasicBlock* pred,
                            BasicBlock* dest) {
  if (dest == NULL || dest == pred || dest == gen->exit_block ||
      BasicBlockIsEmpty(dest) || BasicBlockIsUnreachable(gen, dest)) {
    return false;
  }
  // Preserve loop reducibility: never jump from outside a natural loop to a
  // block inside that loop other than its header.
  for (LoopInfo* loop = dest->innermost_loop; loop != NULL;
       loop = loop->parent) {
    if (dest != loop->header && !BitSetContains(&loop->blocks, pred->block_id)) {
      return false;
    }
  }
  return true;
}

static bool BlockHasOutEdge(const BasicBlock* block, const BasicBlock* target) {
  for (size_t i = 0; i < block->out_edges.length; i++) {
    if ((BlockId)block->out_edges.value.w[i] == target->block_id) {
      return true;
    }
  }
  return false;
}

static bool BlockHasBranchBeforeEnd(const BasicBlock* block) {
  if (block == NULL || BasicBlockIsEmpty((BasicBlock*)block)) {
    return false;
  }
  for (IRNode* inst = BasicBlockBegin((BasicBlock*)block);
       inst != block->end_code; inst = IRNext(inst)) {
    if (IRIsBranch(inst) || IRIsReturn(inst)) {
      return true;
    }
  }
  return false;
}

static IRNode* EnsureBlockLabel(Generator* gen, BasicBlock* block) {
  if (block->code->opcode != IR_OP(label)) {
    IRNode* label =
        GeneratorEmitBefore(gen, NewIR(IR_OP(label)), block->code);
    label->block = block;
    block->code = label;
  }
  return block->code;
}

static BasicBlock* ResolveForwarderChain(Generator* gen, BasicBlock* start,
                                         const JoinBlockInfo* infos,
                                         size_t num_infos) {
  BasicBlock* cur = start;
  BasicBlock* visited[8];
  int nvis = 0;
  for (int hop = 0; hop < 8; hop++) {
    if (cur == NULL || cur->block_id >= num_infos ||
        infos[cur->block_id].kind != kJoinForwarder) {
      break;
    }
    BasicBlock* next = infos[cur->block_id].dest;
    bool cycle = false;
    for (int i = 0; i < nvis; i++) {
      if (visited[i] == next) {
        cycle = true;
        break;
      }
    }
    if (cycle) {
      break;
    }
    visited[nvis++] = cur;
    cur = next;
  }
  return cur;
}

static BasicBlock* ResolveConstantVarChain(Generator* gen, BasicBlock* start,
                                           Symbol* sym, bool is_nonzero,
                                           const JoinBlockInfo* infos,
                                           size_t num_infos) {
  BasicBlock* cur = start;
  BasicBlock* visited[8];
  int nvis = 0;
  for (int hop = 0; hop < 8; hop++) {
    if (cur == NULL || cur->block_id >= num_infos) {
      break;
    }
    const JoinBlockInfo* jb = &infos[cur->block_id];
    BasicBlock* next = NULL;
    if (jb->kind == kJoinForwarder) {
      next = jb->dest;
    } else if (jb->kind == kJoinVarBranch && jb->sym == sym) {
      bool take = jb->branch_on_true ? is_nonzero : !is_nonzero;
      next = take ? jb->taken : jb->fall;
    } else {
      break;
    }
    bool cycle = false;
    for (int i = 0; i < nvis; i++) {
      if (visited[i] == next) {
        cycle = true;
        break;
      }
    }
    if (cycle) {
      break;
    }
    visited[nvis++] = cur;
    cur = next;
  }
  return cur;
}

static bool ValueWidthMatchesBranchLoad(IRNode* val, IRNode* load) {
  if (val == NULL || load == NULL || load->type == NULL) {
    return false;
  }
  if (IRIsComparison(val)) {
    return true;
  }
  if (val->opcode == IR_OP(zeroextendi) && val->inputs.length >= 1) {
    IRNode* inner = val->inputs.value.p[0];
    if (inner != NULL && inner->type != NULL &&
        TypeIsIntegral(inner->type) && inner->type->size > 0 &&
        inner->type->size <= load->type->size) {
      return true;
    }
  }
  if (val->type == NULL ||
      (!TypeIsIntegral(val->type) && !TypeIsPointer(val->type))) {
    return false;
  }
  if (TypeIsBitInt(load->type) || TypeIsBitInt(val->type)) {
    return false;
  }
  if (load->type->type == kTypeBool && load->type->size == 1 &&
      val->type->type != kTypeBool && val->type->size > 1) {
    return false;
  }
  return val->type->size > 0 && val->type->size <= load->type->size;
}

static bool SafeToEvaluateAtBlockEnd(BasicBlock* pred, IRNode* val,
                                     IRNode* store) {
  if (val == NULL || val->block != pred || IRIsVariable(val) ||
      IRHasSideEffects(val)) {
    return false;
  }
  // Ensure nothing with side effects or memory writes sits between `val` and
  // the end of `pred`, so deferred comparison lowering evaluates the same
  // inputs at the branch.
  bool seen_val = false;
  bool seen_store = false;
  for (IRNode* inst = BasicBlockBegin(pred); inst != BasicBlockEnd(pred);
       inst = IRNext(inst)) {
    if (inst == val) {
      seen_val = true;
      continue;
    }
    if (!seen_val) {
      continue;
    }
    if (inst == store) {
      seen_store = true;
      continue;
    }
    if (inst == pred->end_code && inst->opcode == IR_OP(bra)) {
      continue;
    }
    if (inst->opcode == IR_OP(loc) && !IRIsObservableCheckpoint(inst)) {
      continue;
    }
    if (!seen_store) {
      if (IRIsStore(inst) || IRIsCall(inst) || IRHasSideEffects(inst)) {
        return false;
      }
      continue;
    }
    return false;
  }
  return seen_val && seen_store;
}

static void RedirectSingleOutBlock(Generator* gen, BasicBlock* pred,
                                   BasicBlock* old_succ, BasicBlock* dest) {
  IRNode* lbl = EnsureBlockLabel(gen, dest);
  if (pred->end_code->opcode == IR_OP(bra)) {
    IRReplaceInput(pred->end_code, 0, lbl);
  } else {
    IRNode* bra =
        GeneratorEmitAfter(gen, NewIR1(IR_OP(bra), lbl), pred->end_code);
    bra->block = pred;
    pred->end_code = bra;
  }
  BasicBlockRemoveEdge(pred, old_succ);
  BasicBlockAddEdge(pred, dest);
}

static bool RunJumpThreadingPass(Generator* gen) {
  size_t orig_blocks = gen->basic_blocks.length;
  if (orig_blocks == 0) {
    return false;
  }

  BitSet jump_table_blocks;
  BitSetInit(&jump_table_blocks);
  CollectJumpTableBlocks(gen, &jump_table_blocks);

  JoinBlockInfo* infos = calloc(orig_blocks, sizeof(JoinBlockInfo));
  for (size_t i = 0; i < orig_blocks; i++) {
    ClassifyJoinBlock(gen, gen->basic_blocks.value.p[i], &jump_table_blocks,
                      &infos[i]);
  }

  bool changed = false;

  // 1. Thread branches that land on empty forwarder blocks.
  for (size_t i = 0; i < orig_blocks; i++) {
    BasicBlock* pred = gen->basic_blocks.value.p[i];
    if (pred == NULL || pred == gen->exit_block || BasicBlockIsEmpty(pred) ||
        BasicBlockIsUnreachable(gen, pred) ||
        BitSetContains(&jump_table_blocks, pred->block_id) ||
        infos[i].kind == kJoinForwarder) {
      continue;
    }
    IRNode* term = pred->end_code;
    if (term->opcode == IR_OP(bra) &&
        (term->flags & (kIRJumpTableBranch | kIRNrvoMarker)) == 0 &&
        pred->out_edges.length == 1 && !BlockHasBranchBeforeEnd(pred)) {
      IRNode* target_lbl = term->inputs.value.p[0];
      BasicBlock* orig_succ = target_lbl != NULL ? target_lbl->block : NULL;
      BasicBlock* dest =
          ResolveForwarderChain(gen, orig_succ, infos, orig_blocks);
      if (orig_succ != NULL && dest != orig_succ &&
          CanThreadEdgeTo(gen, pred, dest)) {
        IRReplaceInput(term, 0, EnsureBlockLabel(gen, dest));
        BasicBlockRemoveEdge(pred, orig_succ);
        BasicBlockAddEdge(pred, dest);
        changed = true;
      }
    } else if (IRIsConditionalBranch(term) &&
               (term->flags & kIRNrvoMarker) == 0 &&
               term->inputs.length >= 2) {
      IRNode* target_lbl = term->inputs.value.p[1];
      BasicBlock* orig_taken = target_lbl != NULL ? target_lbl->block : NULL;
      BasicBlock* dest =
          ResolveForwarderChain(gen, orig_taken, infos, orig_blocks);
      if (orig_taken != NULL && dest != orig_taken &&
          !BlockHasOutEdge(pred, dest) && CanThreadEdgeTo(gen, pred, dest)) {
        IRReplaceInput(term, 1, EnsureBlockLabel(gen, dest));
        BasicBlockRemoveEdge(pred, orig_taken);
        BasicBlockAddEdge(pred, dest);
        changed = true;
      }
    }
  }

  // 2. Thread blocks that store to a local/temp variable and enter a join
  //    block that immediately branches on that variable.
  for (size_t i = 0; i < orig_blocks; i++) {
    BasicBlock* pred = gen->basic_blocks.value.p[i];
    if (pred == NULL || pred == gen->exit_block || BasicBlockIsEmpty(pred) ||
        BasicBlockIsUnreachable(gen, pred) ||
        BitSetContains(&jump_table_blocks, pred->block_id) ||
        pred->out_edges.length != 1 || BlockHasBranchBeforeEnd(pred)) {
      continue;
    }
    IRNode* term = pred->end_code;
    if (term->opcode == IR_OP(bra)) {
      if ((term->flags & (kIRJumpTableBranch | kIRNrvoMarker)) != 0) {
        continue;
      }
    } else if (IRIsBranch(term) || IRIsReturn(term)) {
      continue;
    }

    BlockId join_id = (BlockId)pred->out_edges.value.w[0];
    if (join_id >= orig_blocks || infos[join_id].kind != kJoinVarBranch) {
      continue;
    }
    BasicBlock* join_blk = gen->basic_blocks.value.p[join_id];
    if (join_blk == pred) {
      continue;
    }
    const JoinBlockInfo* jb = &infos[join_id];

    // Find the last definition of `jb->sym` in `pred`.
    IRNode* store = NULL;
    for (IRNode* inst = pred->end_code; inst != NULL; inst = IRPrev(inst)) {
      if (inst == term && inst->opcode == IR_OP(bra)) {
        if (inst == pred->code) {
          break;
        }
        continue;
      }
      if (IRIsObservableCheckpoint(inst)) {
        break;
      }
      if (IRIsVarDef(inst) && inst->var.def == jb->sym) {
        if (IRIsStoreOnly(inst) && inst->inputs.length == 2 &&
            inst->inputs.value.p[0] == jb->var && inst->outputs.length == 0 &&
            !TypeIsVolatile(inst->type)) {
          store = inst;
        }
        break;
      }
      if (inst == pred->code) {
        break;
      }
    }
    if (store == NULL) {
      continue;
    }

    IRNode* val = store->inputs.value.p[1];
    if (val != NULL && IRIsIntConst(val)) {
      int bits = TypeIsBitInt(jb->sym->type) ? jb->sym->type->bit_width
                                             : jb->sym->type->size * 8;
      uint64_t mask = (bits <= 0 || bits >= 64)
                          ? ~UINT64_C(0)
                          : ((UINT64_C(1) << bits) - 1);
      bool is_nonzero = ((uint64_t)IRIntConstValue(val) & mask) != 0;
      BasicBlock* dest = ResolveConstantVarChain(
          gen, join_blk, jb->sym, is_nonzero, infos, orig_blocks);
      if (dest != join_blk && CanThreadEdgeTo(gen, pred, dest)) {
        RedirectSingleOutBlock(gen, pred, join_blk, dest);
        changed = true;
      }
      continue;
    }

    if (ValueWidthMatchesBranchLoad(val, jb->load) &&
        SafeToEvaluateAtBlockEnd(pred, val, store)) {
      BasicBlock* b_nz = ResolveConstantVarChain(gen, join_blk, jb->sym, true,
                                                 infos, orig_blocks);
      BasicBlock* b_z = ResolveConstantVarChain(gen, join_blk, jb->sym, false,
                                                infos, orig_blocks);
      if (!CanThreadEdgeTo(gen, pred, b_nz) ||
          !CanThreadEdgeTo(gen, pred, b_z)) {
        continue;
      }
      if (b_nz == b_z) {
        if (b_z != join_blk) {
          RedirectSingleOutBlock(gen, pred, join_blk, b_z);
          changed = true;
        }
        continue;
      }

      IRNode* lbl_nz = EnsureBlockLabel(gen, b_nz);
      IRNode* lbl_z = EnsureBlockLabel(gen, b_z);
      if (term->opcode == IR_OP(bra)) {
        BasicBlockRemoveInstruction(gen, pred, term);
      }
      IRNode* next_inst = IRNext(pred->end_code);
      if (next_inst == b_z->code) {
        IRNode* cond_br = GeneratorEmitAfter(
            gen, NewIR2(IR_OP(btrue), val, lbl_nz), pred->end_code);
        cond_br->block = pred;
        pred->end_code = cond_br;
        BasicBlockRemoveEdge(pred, join_blk);
        BasicBlockAddEdge(pred, b_z);
        BasicBlockAddEdge(pred, b_nz);
      } else if (next_inst == b_nz->code) {
        IRNode* cond_br = GeneratorEmitAfter(
            gen, NewIR2(IR_OP(bfalse), val, lbl_z), pred->end_code);
        cond_br->block = pred;
        pred->end_code = cond_br;
        BasicBlockRemoveEdge(pred, join_blk);
        BasicBlockAddEdge(pred, b_nz);
        BasicBlockAddEdge(pred, b_z);
      } else {
        IRNode* cond_br = GeneratorEmitAfter(
            gen, NewIR2(IR_OP(btrue), val, lbl_nz), pred->end_code);
        cond_br->block = pred;
        pred->end_code = cond_br;

        BasicBlock* ft = NewBasicBlock(gen->basic_blocks.length);
        VectorAppend(&gen->basic_blocks, ft);
        IRNode* ft_bra =
            GeneratorEmitAfter(gen, NewIR1(IR_OP(bra), lbl_z), cond_br);
        ft_bra->block = ft;
        ft->code = ft_bra;
        ft->end_code = ft_bra;
        ft->innermost_loop = pred->innermost_loop;
        ft->loop_nesting = pred->loop_nesting;

        BasicBlockRemoveEdge(pred, join_blk);
        BasicBlockAddEdge(pred, ft);
        BasicBlockAddEdge(pred, b_nz);
        BasicBlockAddEdge(ft, b_z);
      }
      changed = true;
    }
  }

  free(infos);
  BitSetDestruct(&jump_table_blocks);
  return changed;
}

static bool BlockHasCallOrHazard(const Generator* gen,
                                 const BasicBlock* block) {
  if (block->num_calls > 0) {
    return true;
  }
  for (IRNode* inst = BasicBlockBegin((BasicBlock*)block);
       !BasicBlockIsEmpty((BasicBlock*)block) &&
       inst != BasicBlockEnd((BasicBlock*)block);
       inst = IRNext(inst)) {
    if (IRIsCall(inst) || IRAsmClobbersMemory(inst) ||
        inst->opcode == IR_OP(asm) || IRIsObservableCheckpoint(inst) ||
        IsAnchoredDebugLoc(gen, inst)) {
      return true;
    }
  }
  return false;
}

static void ComputePureCallFreeBlocks(Generator* gen, BitSet* pure_fast) {
  BitSetClear(pure_fast);
  size_t n = gen->basic_blocks.length;
  for (size_t i = 0; i < n; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    if (b == NULL || b == gen->exit_block || BasicBlockIsEmpty(b) ||
        BasicBlockIsUnreachable(gen, b) || BlockHasCallOrHazard(gen, b)) {
      continue;
    }
    BitSetInsert(pure_fast, b->block_id);
  }
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 0; i < n; i++) {
      BasicBlock* b = gen->basic_blocks.value.p[i];
      if (b == NULL || b == gen->entry_block ||
          !BitSetContains(pure_fast, b->block_id)) {
        continue;
      }
      bool all_fast = false;
      for (size_t e = 0; e < b->in_edges.length; e++) {
        BasicBlock* p =
            VectorGet(&gen->basic_blocks, b->in_edges.value.w[e]);
        if (p == NULL || BasicBlockIsUnreachable(gen, p)) {
          continue;
        }
        if (!BitSetContains(pure_fast, p->block_id)) {
          all_fast = false;
          break;
        }
        all_fast = true;
      }
      if (!all_fast) {
        BitSetRemove(pure_fast, b->block_id);
        changed = true;
      }
    }
  }
}

static bool CanRedirectPredToClone(const BasicBlock* pred,
                                   const BasicBlock* tail,
                                   const BitSet* jump_table_blocks) {
  if (pred == NULL || pred == tail ||
      BitSetContains(jump_table_blocks, pred->block_id)) {
    return false;
  }
  IRNode* term = pred->end_code;
  if (term == NULL || (term->flags & (kIRJumpTableBranch | kIRNrvoMarker)) != 0) {
    return false;
  }
  if (pred->out_edges.length == 1) {
    if (BlockHasBranchBeforeEnd(pred)) {
      return false;
    }
    if (term->opcode == IR_OP(bra)) {
      return term->inputs.length >= 1 && term->inputs.value.p[0] != NULL &&
             ((IRNode*)term->inputs.value.p[0])->block == tail;
    }
    return !IRIsBranch(term) && !IRIsReturn(term);
  }
  if (pred->out_edges.length == 2) {
    if (IRIsConditionalBranch(term)) {
      if (term->inputs.length < 2 || term->inputs.value.p[1] == NULL ||
          BlockHasBranchBeforeEnd(pred)) {
        return false;
      }
      if (((IRNode*)term->inputs.value.p[1])->block == tail) {
        return true;
      }
      return IRNext(term) == tail->code;
    }
    if (term->opcode == IR_OP(bra) && term->inputs.length >= 1 &&
        term->inputs.value.p[0] != NULL) {
      IRNode* prev = IRPrev(term);
      while (prev != NULL && prev->block == pred &&
             prev->opcode == IR_OP(loc)) {
        prev = IRPrev(prev);
      }
      for (IRNode* scan = BasicBlockBegin((BasicBlock*)pred);
           scan != term; scan = IRNext(scan)) {
        if (scan != prev && (IRIsBranch(scan) || IRIsReturn(scan))) {
          return false;
        }
      }
      if (((IRNode*)term->inputs.value.p[0])->block == tail) {
        return prev == NULL || prev->block != pred || !IRIsBranch(prev) ||
               (IRIsConditionalBranch(prev) && prev->inputs.length >= 2 &&
                prev->inputs.value.p[1] != NULL &&
                ((IRNode*)prev->inputs.value.p[1])->block != tail);
      }
      if (prev != NULL && prev->block == pred && IRIsConditionalBranch(prev) &&
          (prev->flags & kIRNrvoMarker) == 0 && prev->inputs.length >= 2 &&
          prev->inputs.value.p[1] != NULL &&
          ((IRNode*)prev->inputs.value.p[1])->block == tail) {
        return true;
      }
    }
  }
  return false;
}

static size_t CloneableTailSize(const Generator* gen, BasicBlock* tail) {
  size_t count = 0;
  for (IRNode* inst = BasicBlockBegin(tail); inst != BasicBlockEnd(tail);
       inst = IRNext(inst)) {
    if (inst->opcode == IR_OP(label)) {
      if ((inst != tail->code && inst->outputs.length != 0) ||
          !LabelHasOnlyBranchUsers(gen, inst)) {
        return 0;
      }
      continue;
    }
    if (inst->opcode == IR_OP(loc)) {
      if (IRIsObservableCheckpoint(inst) || IsAnchoredDebugLoc(gen, inst)) {
        return 0;
      }
      continue;
    }
    if (inst == tail->end_code && inst->opcode == IR_OP(bra)) {
      if ((inst->flags & (kIRJumpTableBranch | kIRNrvoMarker)) != 0) {
        return 0;
      }
      continue;
    }
    if (IRIsBranch(inst) || IRIsReturn(inst) || IRIsCall(inst) ||
        IRAsmClobbersMemory(inst) || inst->opcode == IR_OP(asm) ||
        inst->opcode == IR_OP(named_label) || inst->opcode == IR_OP(enter) ||
        inst->opcode == IR_OP(leave) || inst->opcode == IR_OP(decsp) ||
        inst->opcode == IR_OP(savesp) || inst->opcode == IR_OP(restoresp) ||
        inst->opcode == IR_OP(nrvoval) || inst->opcode == IR_OP(capturev) ||
        inst->opcode == IR_OP(memzero) || inst->opcode == IR_OP(memcpy) ||
        (inst->opcode >= IR_OP(builtin_va_start) &&
         inst->opcode <= IR_OP(atomic_fence)) ||
        IRIsConst(inst) || IRIsVariable(inst) || inst->dest != NULL ||
        (inst->flags & (kIRJumpTableBranch | kIRNrvoMarker)) != 0 ||
        (inst->type != NULL && TypeIsVolatile(inst->type))) {
      return 0;
    }
    for (size_t u = 0; u < inst->outputs.length; u++) {
      IRNode* user = inst->outputs.value.p[u];
      if (user == NULL || user->block != tail) {
        return 0;
      }
    }
    for (size_t j = 0; j < inst->inputs.length; j++) {
      IRNode* inp = inst->inputs.value.p[j];
      if (inp == NULL || inp->block == tail || IRIsConst(inp) ||
          IRIsVariable(inp)) {
        continue;
      }
      if (inp->block == NULL ||
          !BitSetContains(&tail->dominators, inp->block->block_id)) {
        return 0;
      }
    }
    count++;
    if (count > 36) {
      return 0;
    }
  }
  return count;
}

static bool DuplicateCallFreeFastTails(Generator* gen) {
  BitSet jump_table_blocks;
  BitSetInit(&jump_table_blocks);
  CollectJumpTableBlocks(gen, &jump_table_blocks);

  BitSet pure_fast;
  BitSetInit(&pure_fast);

  size_t total_cloned_blocks = 0;
  size_t total_cloned_insts = 0;
  bool any_changed = false;

  for (int pass = 0; pass < 4 && total_cloned_blocks < 4; pass++) {
    ComputePureCallFreeBlocks(gen, &pure_fast);
    bool pass_changed = false;
    size_t orig_blocks = gen->basic_blocks.length;

    for (size_t i = 0; i < orig_blocks && total_cloned_blocks < 4; i++) {
      BasicBlock* tail = gen->basic_blocks.value.p[i];
      if (tail == NULL || tail == gen->entry_block || tail == gen->exit_block ||
          tail->return_block || BasicBlockIsEmpty(tail) ||
          BasicBlockIsUnreachable(gen, tail) || tail->innermost_loop != NULL ||
          BitSetContains(&jump_table_blocks, tail->block_id) ||
          tail->out_edges.length != 1 || BlockHasCallOrHazard(gen, tail)) {
        continue;
      }
      BasicBlock* succ =
          VectorGet(&gen->basic_blocks, tail->out_edges.value.w[0]);
      if (succ == NULL || succ == tail || succ == gen->exit_block ||
          BasicBlockIsEmpty(succ) || BasicBlockIsUnreachable(gen, succ)) {
        continue;
      }
      // Preserve loop reducibility when `fast_blk` branches to `succ`.
      bool loop_hazard = false;
      for (LoopInfo* loop = succ->innermost_loop; loop != NULL;
           loop = loop->parent) {
        if (succ != loop->header) {
          loop_hazard = true;
          break;
        }
      }
      if (loop_hazard) {
        continue;
      }

      Vector fast_preds;
      VectorInit(&fast_preds);
      size_t slow_preds = 0;
      bool preds_ok = true;
      for (size_t e = 0; e < tail->in_edges.length; e++) {
        BasicBlock* p =
            VectorGet(&gen->basic_blocks, tail->in_edges.value.w[e]);
        if (p == NULL || BasicBlockIsUnreachable(gen, p)) {
          continue;
        }
        if (BitSetContains(&pure_fast, p->block_id)) {
          if (!CanRedirectPredToClone(p, tail, &jump_table_blocks)) {
            preds_ok = false;
            break;
          }
          VectorAppend(&fast_preds, p);
        } else {
          slow_preds++;
        }
      }
      if (!preds_ok || fast_preds.length == 0 || slow_preds == 0) {
        VectorDestruct(&fast_preds);
        continue;
      }

      size_t tail_insts = CloneableTailSize(gen, tail);
      if (tail_insts == 0 || total_cloned_insts + tail_insts > 80) {
        VectorDestruct(&fast_preds);
        continue;
      }

      IRNode* succ_lbl = EnsureBlockLabel(gen, succ);
      EnsureBlockLabel(gen, tail);

      BasicBlock* fall_pred = NULL;
      IRNode* prev_node = IRPrev(tail->code);
      if (prev_node != NULL && prev_node->block != NULL &&
          prev_node->block != tail &&
          BlockHasOutEdge(prev_node->block, tail)) {
        IRNode* pterm = prev_node->block->end_code;
        if (pterm != NULL && !IRIsUnconditionalBranch(pterm) &&
            !(IRIsConditionalBranch(pterm) && pterm->inputs.length >= 2 &&
              pterm->inputs.value.p[1] != NULL &&
              ((IRNode*)pterm->inputs.value.p[1])->block == tail)) {
          fall_pred = prev_node->block;
        }
      }

      bool place_before_tail =
          fall_pred != NULL && BitSetContains(&pure_fast, fall_pred->block_id);
      if (!place_before_tail && !IRIsBranch(tail->end_code)) {
        IRNode* tbra = GeneratorEmitAfter(gen, NewIR1(IR_OP(bra), succ_lbl),
                                          tail->end_code);
        tbra->block = tail;
        tail->end_code = tbra;
      }

      BasicBlock* fast_blk = NewBasicBlock(gen->basic_blocks.length);
      VectorAppend(&gen->basic_blocks, fast_blk);

      IRNode* fast_lbl = NewIR(IR_OP(label));
      if (place_before_tail) {
        GeneratorEmitBefore(gen, fast_lbl, tail->code);
      } else {
        GeneratorEmitAfter(gen, fast_lbl, tail->end_code);
      }
      fast_lbl->block = fast_blk;
      fast_blk->code = fast_lbl;
      fast_blk->end_code = fast_lbl;

      Map remap;
      MapInitForPointerKeys(&remap);
      for (IRNode* inst = BasicBlockBegin(tail); inst != BasicBlockEnd(tail);
           inst = IRNext(inst)) {
        if (inst->opcode == IR_OP(label) || inst->opcode == IR_OP(loc)) {
          continue;
        }
        if (inst == tail->end_code && inst->opcode == IR_OP(bra)) {
          continue;
        }
        IRNode* copy = NewIR(inst->opcode);
        IRSetType(copy, inst->type);
        copy->flags = inst->flags;
        copy->var = inst->var;
        copy->location = inst->location;
        copy->value_state = inst->value_state;
        copy->data = inst->data;
        copy->aux = inst->aux;
        for (size_t j = 0; j < inst->inputs.length; j++) {
          IRNode* inp = inst->inputs.value.p[j];
          IRNode* mapped = inp != NULL ? MapFindPointerKey(&remap, inp) : NULL;
          IRAddInput(copy, mapped != NULL ? mapped : inp, false);
        }
        GeneratorEmitAfter(gen, copy, fast_blk->end_code);
        copy->block = fast_blk;
        fast_blk->end_code = copy;
        MapKeyValue kv = {.key.p = inst, .value.p = copy};
        MapInsert(&remap, kv);

        if (IRIsVarDef(copy)) {
          MapKeyValue dkv = {.key.p = copy->var.def, .value.p = NULL};
          MapInsert(&fast_blk->defined_vars, dkv);
        }
        if (IRIsVarRef(copy)) {
          SetInsert(&fast_blk->referenced_vars, copy->var.use);
        }
      }
      MapDestruct(&remap);

      IRNode* end_bra = GeneratorEmitAfter(gen, NewIR1(IR_OP(bra), succ_lbl),
                                           fast_blk->end_code);
      end_bra->block = fast_blk;
      fast_blk->end_code = end_bra;
      BasicBlockAddEdge(fast_blk, succ);

      for (size_t p_idx = 0; p_idx < fast_preds.length; p_idx++) {
        BasicBlock* p = fast_preds.value.p[p_idx];
        if (p == fall_pred) {
          BasicBlockRemoveEdge(p, tail);
          BasicBlockAddEdge(p, fast_blk);
        } else if (p->out_edges.length == 1) {
          RedirectSingleOutBlock(gen, p, tail, fast_blk);
        } else {
          IRNode* pterm = p->end_code;
          if (IRIsConditionalBranch(pterm) &&
              ((IRNode*)pterm->inputs.value.p[1])->block == tail) {
            IRReplaceInput(pterm, 1, fast_lbl);
          } else if (pterm->opcode == IR_OP(bra) &&
                     ((IRNode*)pterm->inputs.value.p[0])->block == tail) {
            IRReplaceInput(pterm, 0, fast_lbl);
          } else if (pterm->opcode == IR_OP(bra)) {
            IRNode* pprev = IRPrev(pterm);
            while (pprev != NULL && pprev->block == p &&
                   pprev->opcode == IR_OP(loc)) {
              pprev = IRPrev(pprev);
            }
            if (pprev != NULL && pprev->block == p &&
                IRIsConditionalBranch(pprev) &&
                ((IRNode*)pprev->inputs.value.p[1])->block == tail) {
              IRReplaceInput(pprev, 1, fast_lbl);
            }
          }
          BasicBlockRemoveEdge(p, tail);
          BasicBlockAddEdge(p, fast_blk);
        }
      }
      VectorDestruct(&fast_preds);

      total_cloned_blocks++;
      total_cloned_insts += tail_insts;
      pass_changed = true;
      any_changed = true;
      break;
    }

    if (!pass_changed) {
      break;
    }
    GeneratorCleanupCFG(gen);
  }

  BitSetDestruct(&pure_fast);
  BitSetDestruct(&jump_table_blocks);
  return any_changed;
}

bool JumpThreadingOptimization(Generator* gen) {
  if (gen == NULL || gen->entry_block == NULL || gen->exit_block == NULL ||
      gen->exception_ranges.length != 0 ||
      gen->exception_keep_labels.length != 0) {
    return false;
  }

  bool any_changed = false;
  for (int iter = 0; iter < 6; iter++) {
    if (!RunJumpThreadingPass(gen)) {
      break;
    }
    any_changed = true;
    GeneratorCleanupCFG(gen);
    if (compiler->ir_optimizations.dce) {
      DeadCodeEliminationOptimization(gen);
    }
  }
  if (DuplicateCallFreeFastTails(gen)) {
    any_changed = true;
  }
  return any_changed;
}
