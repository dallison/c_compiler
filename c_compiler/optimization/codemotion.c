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
#include "symbol.h"

typedef struct {
  bool writes_memory;
  bool calls;
} LoopMemoryEffects;

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
      effects.writes_memory |= IRIsStore(inst);
    }
    BitSetIteratorNext(&it);
  }
  return effects;
}

static bool LoopDefinesSymbol(Generator* gen, const LoopInfo* loop,
                              const Symbol* symbol) {
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (IRIsVarDef(inst) && inst->var.def == symbol) {
        return true;
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

// SSA-backed, non-address-taken locals cannot alias an unrelated store.  This
// lets a loop hoist invariant argument/local reads without pretending that an
// arbitrary pointer load is safe across stores or calls.
static bool IsSafeVariableLoad(Generator* gen, const LoopInfo* loop,
                               IRNode* inst) {
  if (!IRIsVarRef(inst) || inst->var.use == NULL ||
      IRHasSideEffects(inst)) {
    return false;
  }
  Symbol* symbol = inst->var.use;
  if (symbol->flags.address_taken ||
      (!symbol->flags.is_local && !symbol->flags.is_argument &&
       !symbol->flags.is_temp) ||
      LoopDefinesSymbol(gen, loop, symbol)) {
    return false;
  }
  return true;
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

static bool IsHoistCandidate(Generator* gen, const LoopInfo* loop,
                             LoopMemoryEffects effects, Set* moved,
                             IRNode* inst) {
  if (inst->dest != NULL || inst->inputs.length == 0 ||
      IRHasSideEffects(inst) || IRIsVariable(inst) ||
      inst->opcode == IR_OP(literalref) ||
      SetContains(moved, inst) ||
      (!IRIsLoad(inst) && !IRIsExpression(inst)) ||
      !IsLoopInvariant(loop, inst)) {
    return false;
  }
  if (IRIsLoadOnly(inst) && (effects.writes_memory || effects.calls) &&
      !IsSafeVariableLoad(gen, loop, inst)) {
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

static bool HoistLoopInvariants(Generator* gen, LoopInfo* loop, Set* moved) {
  if (loop->parent != NULL || loop->children.length != 0 ||
      loop->preheader == NULL || BasicBlockIsEmpty(loop->preheader)) {
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
          HoistToPreheader(gen, loop, inst);
          SetInsert(moved, inst);
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
  Set moved;
  SetInitForPointers(&moved);
  int max_depth = 0;
  for (size_t i = 0; i < gen->loops.length; i++) {
    LoopInfo* loop = gen->loops.value.p[i];
    if (loop->depth > max_depth) {
      max_depth = loop->depth;
    }
  }
  // Inner loops first.  An invariant moved to an inner preheader can then be
  // considered for movement out of its parent loop.
  for (int depth = max_depth; depth > 0; depth--) {
    for (size_t i = 0; i < gen->loops.length; i++) {
      LoopInfo* loop = gen->loops.value.p[i];
      if (loop->depth == depth) {
        HoistLoopInvariants(gen, loop, &moved);
      }
    }
  }
  SetDestruct(&moved);
}
