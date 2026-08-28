//
//  loop_info.c
//  c_compiler
//

#include "loop_info.h"

#include <assert.h>
#include <stdlib.h>

#include "basic_block.h"
#include "codegen.h"
#include "ir.h"

static LoopInfo* NewLoopInfo(size_t loop_id, BasicBlock* header) {
  LoopInfo* loop = malloc(sizeof(LoopInfo));
  loop->loop_id = loop_id;
  loop->header = header;
  loop->preheader = NULL;
  BitSetInit(&loop->blocks);
  BitSetInit(&loop->latches);
  BitSetInit(&loop->exits);
  loop->parent = NULL;
  VectorInit(&loop->children);
  loop->depth = 1;
  return loop;
}

static void DeleteLoopInfo(LoopInfo* loop) {
  BitSetDestruct(&loop->blocks);
  BitSetDestruct(&loop->latches);
  BitSetDestruct(&loop->exits);
  VectorDestruct(&loop->children);
  free(loop);
}

bool LoopInfoContainsBlock(const LoopInfo* loop, const BasicBlock* block) {
  return loop != NULL && block != NULL &&
         BitSetContains((BitSet*)&loop->blocks, block->block_id);
}

void LoopInfoClear(Generator* gen) {
  for (size_t i = 0; i < gen->loops.length; i++) {
    DeleteLoopInfo(gen->loops.value.p[i]);
  }
  VectorClear(&gen->loops);
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    block->innermost_loop = NULL;
    block->loop_nesting = 0;
    BitSetClear(&block->in_back_edges);
  }
}

static bool LoopContainsLoop(const LoopInfo* outer, const LoopInfo* inner) {
  if (BitSetCount((BitSet*)&outer->blocks) <=
      BitSetCount((BitSet*)&inner->blocks)) {
    return false;
  }
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&inner->blocks);
  while (!BitSetIteratorDone(&it)) {
    if (!BitSetContains((BitSet*)&outer->blocks,
                        BitSetIteratorValue(&it))) {
      return false;
    }
    BitSetIteratorNext(&it);
  }
  return true;
}

static int CalculateLoopDepth(LoopInfo* loop) {
  if (loop->parent == NULL) {
    return 1;
  }
  if (loop->depth > 1) {
    return loop->depth;
  }
  loop->depth = CalculateLoopDepth(loop->parent) + 1;
  return loop->depth;
}

static void FindPreheader(Generator* gen, LoopInfo* loop) {
  BasicBlock* outside = NULL;
  for (size_t i = 0; i < loop->header->in_edges.length; i++) {
    BasicBlock* predecessor =
        VectorGet(&gen->basic_blocks, loop->header->in_edges.value.w[i]);
    if (LoopInfoContainsBlock(loop, predecessor)) {
      continue;
    }
    if (outside != NULL && outside != predecessor) {
      return;
    }
    outside = predecessor;
  }
  if (outside == NULL || outside->out_edges.length != 1 ||
      outside->out_edges.value.w[0] != loop->header->block_id) {
    return;
  }
  loop->preheader = outside;
}

void LoopInfoBuild(Generator* gen) {
  LoopInfoClear(gen);

  // A back edge is latch -> header where the header dominates the latch.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* latch = gen->basic_blocks.value.p[i];
    for (size_t j = 0; j < latch->out_edges.length; j++) {
      BlockId header_id = latch->out_edges.value.w[j];
      if (BitSetContains(&latch->dominators, header_id)) {
        BasicBlock* header = VectorGet(&gen->basic_blocks, header_id);
        BasicBlockAddBackEdge(latch, header);
      }
    }
  }

  // Union all latches targeting one header into one natural loop.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* header = gen->basic_blocks.value.p[i];
    if (BitSetCount(&header->in_back_edges) == 0) {
      continue;
    }
    LoopInfo* loop = NewLoopInfo(gen->loops.length, header);
    BitSetCopy(&loop->latches, &header->in_back_edges);
    BitSetInsert(&loop->blocks, header->block_id);

    Vector work;
    VectorInit(&work);
    BitSetIterator latch_it;
    BitSetIteratorStart(&latch_it, &loop->latches);
    while (!BitSetIteratorDone(&latch_it)) {
      VectorAppend(&work,
                   VectorGet(&gen->basic_blocks,
                             (BlockId)BitSetIteratorValue(&latch_it)));
      BitSetIteratorNext(&latch_it);
    }
    while (work.length > 0) {
      BasicBlock* block = VectorLast(&work);
      VectorPop(&work);
      if (BitSetContains(&loop->blocks, block->block_id)) {
        continue;
      }
      BitSetInsert(&loop->blocks, block->block_id);
      for (size_t j = 0; j < block->in_edges.length; j++) {
        VectorAppend(
            &work,
            VectorGet(&gen->basic_blocks, block->in_edges.value.w[j]));
      }
    }
    VectorDestruct(&work);

    BitSetIterator block_it;
    BitSetIteratorStart(&block_it, &loop->blocks);
    while (!BitSetIteratorDone(&block_it)) {
      BasicBlock* block =
          VectorGet(&gen->basic_blocks, BitSetIteratorValue(&block_it));
      for (size_t j = 0; j < block->out_edges.length; j++) {
        if (!BitSetContains(&loop->blocks, block->out_edges.value.w[j])) {
          BitSetInsert(&loop->exits, block->block_id);
          break;
        }
      }
      BitSetIteratorNext(&block_it);
    }
    VectorAppend(&gen->loops, loop);
  }

  // The parent is the smallest strict natural-loop superset.
  for (size_t i = 0; i < gen->loops.length; i++) {
    LoopInfo* loop = gen->loops.value.p[i];
    size_t parent_size = SIZE_MAX;
    for (size_t j = 0; j < gen->loops.length; j++) {
      LoopInfo* candidate = gen->loops.value.p[j];
      if (candidate == loop || !LoopContainsLoop(candidate, loop)) {
        continue;
      }
      size_t candidate_size = BitSetCount(&candidate->blocks);
      if (candidate_size < parent_size) {
        loop->parent = candidate;
        parent_size = candidate_size;
      }
    }
    if (loop->parent != NULL) {
      VectorAppend(&loop->parent->children, loop);
    }
  }

  for (size_t i = 0; i < gen->loops.length; i++) {
    LoopInfo* loop = gen->loops.value.p[i];
    loop->depth = CalculateLoopDepth(loop);
    FindPreheader(gen, loop);

    BitSetIterator block_it;
    BitSetIteratorStart(&block_it, &loop->blocks);
    while (!BitSetIteratorDone(&block_it)) {
      BasicBlock* block =
          VectorGet(&gen->basic_blocks, BitSetIteratorValue(&block_it));
      if (block->innermost_loop == NULL ||
          loop->depth > block->innermost_loop->depth) {
        block->innermost_loop = loop;
        block->loop_nesting = loop->depth;
      }
      BitSetIteratorNext(&block_it);
    }
  }
}

static bool HeaderIsExceptionBoundary(Generator* gen, BasicBlock* header) {
  for (size_t i = 0; i < gen->exception_ranges.length; i++) {
    ExceptionHandlerRange* range = gen->exception_ranges.value.p[i];
    if ((range->try_start != NULL && range->try_start->block == header) ||
        (range->try_end != NULL && range->try_end->block == header) ||
        (range->catch_label != NULL && range->catch_label->block == header)) {
      return true;
    }
  }
  return false;
}

static bool IsFallthroughEdge(BasicBlock* from, BasicBlock* to) {
  if (from->end_code == NULL || IRIsReturn(from->end_code) ||
      from->end_code->opcode == IR_OP(bra) ||
      from->end_code->opcode == IR_OP(cbra)) {
    return false;
  }
  return IRNext(from->end_code) != NULL &&
         IRNext(from->end_code)->block == to;
}

static bool CanRedirectEdge(BasicBlock* from, BasicBlock* header) {
  if (IsFallthroughEdge(from, header)) {
    return true;
  }
  IRNode* terminator = from->end_code;
  if (terminator == NULL || terminator->opcode == IR_OP(cbra)) {
    return false;
  }
  if (IRIsConditionalBranch(terminator)) {
    return terminator->inputs.length > 1 &&
           ((IRNode*)terminator->inputs.value.p[1])->block == header;
  }
  return terminator->opcode == IR_OP(bra) && terminator->inputs.length > 0 &&
         ((IRNode*)terminator->inputs.value.p[0])->block == header;
}

static void RedirectExplicitEdge(BasicBlock* from, BasicBlock* header,
                                 IRNode* preheader_label,
                                 bool was_fallthrough) {
  IRNode* terminator = from->end_code;
  if (IRIsConditionalBranch(terminator) &&
      ((IRNode*)terminator->inputs.value.p[1])->block == header) {
    IRReplaceInput(terminator, 1, preheader_label);
  } else if (terminator->opcode == IR_OP(bra) &&
             ((IRNode*)terminator->inputs.value.p[0])->block == header) {
    IRReplaceInput(terminator, 0, preheader_label);
  } else {
    assert(was_fallthrough);
  }
}

// The instruction the preheader's code goes in front of.  A loop whose test
// sits at the bottom keeps its header below its body, and code placed in front
// of the header would then sit inside the loop: the body would fall into it on
// every iteration, and the lowering, which walks the instruction list in order,
// would meet the body's uses of the hoisted values before their definitions.
// Going in front of the loop's first instruction instead puts the preheader
// ahead of every block it feeds.
static IRNode* PreheaderAnchor(Generator* gen, LoopInfo* loop) {
  for (IRNode* scan = GeneratorFirstInstruction(gen); scan != NULL;
       scan = IRNext(scan)) {
    if (scan->block != NULL && LoopInfoContainsBlock(loop, scan->block)) {
      return scan;
    }
  }
  return loop->header->code;
}

static BasicBlock* NewPreheaderBlock(Generator* gen, LoopInfo* loop,
                                     BitSet* outside_predecessors) {
  // Record fallthroughs before inserting any code: the insertion itself changes
  // IRNext(predecessor->end_code).
  BitSet fallthrough_predecessors;
  BitSetInit(&fallthrough_predecessors);
  BitSetIterator it;
  BitSetIteratorStart(&it, outside_predecessors);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* predecessor =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    if (IsFallthroughEdge(predecessor, loop->header)) {
      BitSetInsert(&fallthrough_predecessors, predecessor->block_id);
    }
    BitSetIteratorNext(&it);
  }

  IRNode* header_label = loop->header->code;
  assert(header_label != NULL && header_label->opcode == IR_OP(label));
  IRNode* anchor = PreheaderAnchor(gen, loop);
  // A predecessor that reaches the header by falling into it only keeps that
  // edge if the preheader takes the header's place in the instruction list.
  // Leave such a loop alone rather than move the code out from under the edge.
  if (anchor != header_label && BitSetCount(&fallthrough_predecessors) > 0) {
    BitSetDestruct(&fallthrough_predecessors);
    return NULL;
  }

  BasicBlock* preheader = NewBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, preheader);

  // The branch goes to the header wherever the code itself lands.
  IRNode* label = GeneratorEmitBefore(gen, NewIR(IR_OP(label)), anchor);
  IRNode* branch =
      GeneratorEmitBefore(gen, NewIR1(IR_OP(bra), header_label), anchor);
  label->block = preheader;
  branch->block = preheader;
  preheader->code = label;
  preheader->end_code = branch;

  BitSetIteratorStart(&it, outside_predecessors);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* predecessor =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    RedirectExplicitEdge(
        predecessor, loop->header, label,
        BitSetContains(&fallthrough_predecessors, predecessor->block_id));
    BasicBlockRemoveEdge(predecessor, loop->header);
    BasicBlockAddEdge(predecessor, preheader);
    BitSetIteratorNext(&it);
  }
  BasicBlockAddEdge(preheader, loop->header);
  BitSetDestruct(&fallthrough_predecessors);
  return preheader;
}

bool LoopInfoCreatePreheaders(Generator* gen) {
  for (size_t i = 0; i < gen->loops.length; i++) {
    LoopInfo* loop = gen->loops.value.p[i];
    // Keep nested-loop CFGs unchanged for the first milestone.  Extra
    // preheaders there lengthen several simultaneously-live loop values and
    // expose allocator aliasing on current backends.
    if (loop->preheader != NULL || loop->parent != NULL ||
        loop->children.length != 0 || loop->header == gen->entry_block ||
        HeaderIsExceptionBoundary(gen, loop->header) ||
        loop->header->code == NULL ||
        loop->header->code->opcode != IR_OP(label)) {
      continue;
    }

    BitSet outside;
    BitSetInit(&outside);
    bool can_redirect = true;
    for (size_t j = 0; j < loop->header->in_edges.length; j++) {
      BasicBlock* predecessor =
          VectorGet(&gen->basic_blocks, loop->header->in_edges.value.w[j]);
      if (LoopInfoContainsBlock(loop, predecessor) ||
          BitSetContains(&outside, predecessor->block_id)) {
        continue;
      }
      BitSetInsert(&outside, predecessor->block_id);
      if (!CanRedirectEdge(predecessor, loop->header)) {
        can_redirect = false;
      }
    }
    if (BitSetCount(&outside) > 0 && can_redirect &&
        NewPreheaderBlock(gen, loop, &outside) != NULL) {
      BitSetDestruct(&outside);
      return true;
    }
    BitSetDestruct(&outside);
  }
  return false;
}
