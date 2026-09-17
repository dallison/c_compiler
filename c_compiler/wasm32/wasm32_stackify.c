//
//  wasm32_stackify.c
//  c_compiler
//
//  Wasm has no goto.  Control flow is a tree of block, loop and if scopes
//  and a branch names an enclosing scope by how many levels out it is, so an
//  arbitrary control flow graph cannot be encoded directly.
//
//  Where the graph is reducible its own shape can be used: a loop becomes a
//  loop scope its back edges branch to, and a forward branch becomes a block
//  scope whose end sits at the branch's target.  That is what the source
//  said in the first place and it is what an engine can optimize.
//
//  Not every graph is reducible, and even a reducible one can want scopes
//  that overlap without nesting.  For those there is a second shape that
//  works for anything: one loop containing a br_table that dispatches on a
//  selector local, with each basic block sitting between two of the nested
//  block ends so that dispatching to depth k lands at block k.
//
//      loop
//        block          ;; depth N-1 seen from the dispatch
//          ...
//            block      ;; depth 1
//              block    ;; depth 0
//                local.get $selector
//                br_table 0 1 .. N-1 (default 0)
//              end
//              <block 0>
//            end
//            <block 1>
//          ...
//        end
//        <block N-1>
//      end
//
//  A jump to block t becomes "set the selector to t and branch to the loop",
//  which re-runs the dispatch.  Falling off the end of block k lands in
//  block k+1 with no branch at all, so straight-line code stays cheap.
//
//  Because every value already lives in a local, nothing has to be threaded
//  through the operand stack across a dispatch, which is what makes either
//  transformation safe without any further analysis.
//

#include "wasm32_stackify.h"

#include <assert.h>
#include <stdlib.h>

#include "bitset.h"
#include "wasm32_optimize.h"

// A basic block, identified by the instruction it starts at.
typedef struct {
  TargetInstruction* anchor;  // First instruction of the block.
  TargetInstruction* label;   // Label instruction, if the block has one.
  Vector successors;          // Block indices this one can reach.
  Vector predecessors;        // Block indices that can reach this one.
  size_t rpo_number;          // Position in reverse postorder.
  int idom;                   // Immediate dominator, or -1 if not known.
  bool reachable;
} StackifyBlock;

static bool IsLabel(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W_OP(label) ||
         inst->opcode == (TargetOpcode)W_OP(named_label);
}

static bool IsTerminator(TargetInstruction* inst) {
  if ((inst->flags & TARGET_INST_DEAD) != 0) {
    return false;
  }
  switch ((Wasm32Opcode)inst->opcode) {
    case W_OP(br):
    case W_OP(br_if):
    case W_OP(return):
    case W_OP(throw):
    case W_OP(unreachable):
      return true;
    default:
      return false;
  }
}

// Split the instruction list into basic blocks.  A block starts at a label
// or immediately after a terminator.
static void CollectBlocks(Wasm32Generator* wasm, Vector* blocks) {
  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  bool start_new_block = true;
  while (inst != NULL) {
    if (IsLabel(inst) || start_new_block) {
      StackifyBlock* block = malloc(sizeof(StackifyBlock));
      block->anchor = inst;
      block->label = IsLabel(inst) ? inst : NULL;
      VectorInit(&block->successors);
      VectorInit(&block->predecessors);
      block->rpo_number = 0;
      block->idom = -1;
      block->reachable = false;
      VectorAppend(blocks, block);
      start_new_block = false;
    }
    if (IsTerminator(inst)) {
      start_new_block = true;
    }
    inst = TargetNext(inst);
  }
}

static void DestructBlocks(Vector* blocks) {
  for (size_t i = 0; i < blocks->length; i++) {
    StackifyBlock* block = blocks->value.p[i];
    VectorDestruct(&block->successors);
    VectorDestruct(&block->predecessors);
  }
  VectorDestructWithContents(blocks, NULL, /*free_element=*/true);
}

static int BlockIndexOfLabel(Vector* blocks, TargetInstruction* label) {
  for (size_t i = 0; i < blocks->length; i++) {
    StackifyBlock* block = blocks->value.p[i];
    if (block->label == label) {
      return (int)i;
    }
  }
  return -1;
}

// The instruction a block ends on, when that is a branch or a return.  A
// block that simply runs into the next one has none.
static TargetInstruction* BlockTerminator(Vector* blocks, size_t index) {
  StackifyBlock* block = blocks->value.p[index];
  TargetInstruction* limit =
      (index + 1 < blocks->length)
          ? ((StackifyBlock*)blocks->value.p[index + 1])->anchor
          : NULL;
  for (TargetInstruction* inst = block->anchor; inst != NULL && inst != limit;
       inst = TargetNext(inst)) {
    if (IsTerminator(inst)) {
      return inst;
    }
  }
  return NULL;
}

static void AddSuccessor(StackifyBlock* block, int index) {
  for (size_t i = 0; i < block->successors.length; i++) {
    if ((int)(intptr_t)block->successors.value.p[i] == index) {
      return;
    }
  }
  VectorAppend(&block->successors, (void*)(intptr_t)index);
}

// Wire up the graph.  A conditional branch also falls through, and a block
// with no terminator at all runs into the one after it.
static bool BuildEdges(Vector* blocks) {
  for (size_t i = 0; i < blocks->length; i++) {
    StackifyBlock* block = blocks->value.p[i];
    TargetInstruction* terminator = BlockTerminator(blocks, i);
    bool falls_through = true;
    if (terminator != NULL) {
      if (terminator->opcode == (TargetOpcode)W_OP(return)) {
        falls_through = false;
      } else {
        int target = BlockIndexOfLabel(blocks, terminator->operand[1]);
        if (target < 0) {
          // A branch whose target is not a block here cannot be given a
          // depth, so the structured shape is not available.
          return false;
        }
        AddSuccessor(block, target);
        falls_through = terminator->opcode == (TargetOpcode)W_OP(br_if);
      }
    }
    if (falls_through && i + 1 < blocks->length) {
      AddSuccessor(block, (int)(i + 1));
    }
  }
  return true;
}

static void MarkReachable(Vector* blocks) {
  Vector worklist;
  VectorInit(&worklist);
  ((StackifyBlock*)blocks->value.p[0])->reachable = true;
  VectorAppend(&worklist, (void*)(intptr_t)0);
  while (worklist.length > 0) {
    size_t index = (size_t)(intptr_t)VectorLast(&worklist);
    VectorPop(&worklist);
    StackifyBlock* block = blocks->value.p[index];
    for (size_t i = 0; i < block->successors.length; i++) {
      size_t next = (size_t)(intptr_t)block->successors.value.p[i];
      StackifyBlock* successor = blocks->value.p[next];
      if (!successor->reachable) {
        successor->reachable = true;
        VectorAppend(&worklist, (void*)(intptr_t)next);
      }
    }
  }
  VectorDestruct(&worklist);
}

static void BuildPredecessors(Vector* blocks) {
  for (size_t i = 0; i < blocks->length; i++) {
    StackifyBlock* block = blocks->value.p[i];
    if (!block->reachable) {
      continue;
    }
    for (size_t s = 0; s < block->successors.length; s++) {
      size_t next = (size_t)(intptr_t)block->successors.value.p[s];
      VectorAppend(&((StackifyBlock*)blocks->value.p[next])->predecessors,
                   (void*)(intptr_t)i);
    }
  }
}

// The nearest block that dominates both, found by climbing the tree built so
// far.  Reverse postorder numbers give a total order to climb by, since a
// block's immediate dominator always comes before it.
static int CommonDominator(Vector* blocks, int a, int b) {
  // Only blocks the entry reaches are ever predecessors, so both of these
  // have a reverse postorder number of their own and the climb terminates.
  assert(((StackifyBlock*)blocks->value.p[a])->reachable &&
         ((StackifyBlock*)blocks->value.p[b])->reachable);
  while (a != b) {
    while (((StackifyBlock*)blocks->value.p[a])->rpo_number >
           ((StackifyBlock*)blocks->value.p[b])->rpo_number) {
      a = ((StackifyBlock*)blocks->value.p[a])->idom;
    }
    while (((StackifyBlock*)blocks->value.p[b])->rpo_number >
           ((StackifyBlock*)blocks->value.p[a])->rpo_number) {
      b = ((StackifyBlock*)blocks->value.p[b])->idom;
    }
  }
  return a;
}

// Immediate dominators, by the usual fixed point over reverse postorder:
// a block's is the meet of its predecessors', and visiting in that order
// means most predecessors are already settled, so it converges in a couple
// of passes.  Only immediate dominators are kept, and the full relation is
// read off by climbing from the dominated block.
static void ComputeDominators(Vector* blocks, Vector* order) {
  ((StackifyBlock*)blocks->value.p[0])->idom = 0;
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 0; i < order->length; i++) {
      size_t index = (size_t)(intptr_t)order->value.p[i];
      if (index == 0) {
        continue;
      }
      StackifyBlock* block = blocks->value.p[index];
      int idom = -1;
      for (size_t p = 0; p < block->predecessors.length; p++) {
        int pred = (int)(intptr_t)block->predecessors.value.p[p];
        if (((StackifyBlock*)blocks->value.p[pred])->idom < 0) {
          continue;
        }
        idom = idom < 0 ? pred : CommonDominator(blocks, pred, idom);
      }
      if (idom >= 0 && block->idom != idom) {
        block->idom = idom;
        changed = true;
      }
    }
  }
}

// Does 'dominator' dominate 'block'?
static bool Dominates(Vector* blocks, size_t dominator, size_t block) {
  for (;;) {
    if (block == dominator) {
      return true;
    }
    int next = ((StackifyBlock*)blocks->value.p[block])->idom;
    if (next < 0 || (size_t)next == block) {
      return false;
    }
    block = (size_t)next;
  }
}

// Reverse postorder, which puts every block after at least one predecessor
// and so makes every edge that is not a back edge point forward.
static void ComputeReversePostorder(Vector* blocks, Vector* order) {
  size_t count = blocks->length;
  char* state = calloc(count, 1);  // 0 unvisited, 1 on stack, 2 done.
  Vector stack;                    // Block index, then successor cursor.
  VectorInit(&stack);
  Vector postorder;
  VectorInit(&postorder);

  VectorAppend(&stack, (void*)(intptr_t)0);
  VectorAppend(&stack, (void*)(intptr_t)0);
  state[0] = 1;
  while (stack.length > 0) {
    size_t cursor = (size_t)(intptr_t)stack.value.p[stack.length - 1];
    size_t index = (size_t)(intptr_t)stack.value.p[stack.length - 2];
    StackifyBlock* block = blocks->value.p[index];
    if (cursor < block->successors.length) {
      stack.value.p[stack.length - 1] = (void*)(intptr_t)(cursor + 1);
      size_t next = (size_t)(intptr_t)block->successors.value.p[cursor];
      if (state[next] == 0) {
        state[next] = 1;
        VectorAppend(&stack, (void*)(intptr_t)next);
        VectorAppend(&stack, (void*)(intptr_t)0);
      }
      continue;
    }
    state[index] = 2;
    VectorAppend(&postorder, (void*)(intptr_t)index);
    VectorPop(&stack);
    VectorPop(&stack);
  }

  for (size_t i = postorder.length; i > 0; i--) {
    size_t index = (size_t)(intptr_t)postorder.value.p[i - 1];
    ((StackifyBlock*)blocks->value.p[index])->rpo_number = order->length;
    VectorAppend(order, (void*)(intptr_t)index);
  }
  VectorDestruct(&postorder);
  VectorDestruct(&stack);
  free(state);
}

// The blocks of the natural loop a back edge closes: the header, the latch,
// and everything that reaches the latch without going through the header.
static void CollectLoopBody(Vector* blocks, size_t header, size_t latch,
                            BitSet* body) {
  BitSetInsert(body, header);
  if (latch == header) {
    return;
  }
  Vector worklist;
  VectorInit(&worklist);
  BitSetInsert(body, latch);
  VectorAppend(&worklist, (void*)(intptr_t)latch);
  while (worklist.length > 0) {
    size_t index = (size_t)(intptr_t)VectorLast(&worklist);
    VectorPop(&worklist);
    StackifyBlock* block = blocks->value.p[index];
    for (size_t p = 0; p < block->predecessors.length; p++) {
      size_t pred = (size_t)(intptr_t)block->predecessors.value.p[p];
      if (!BitSetContains(body, pred)) {
        BitSetInsert(body, pred);
        VectorAppend(&worklist, (void*)(intptr_t)pred);
      }
    }
  }
  VectorDestruct(&worklist);
}

// Find every natural loop.  A graph with a retreating edge whose target does
// not dominate its source is irreducible, and no arrangement of scopes can
// express it.
static bool FindLoops(Vector* blocks, BitSet** loops) {
  size_t count = blocks->length;
  for (size_t i = 0; i < count; i++) {
    loops[i] = NULL;
  }
  for (size_t i = 0; i < count; i++) {
    StackifyBlock* block = blocks->value.p[i];
    if (!block->reachable) {
      continue;
    }
    for (size_t s = 0; s < block->successors.length; s++) {
      size_t target = (size_t)(intptr_t)block->successors.value.p[s];
      // A retreating edge is one that goes back on itself in reverse
      // postorder, which is where a loop closes.
      if (((StackifyBlock*)blocks->value.p[target])->rpo_number >
          block->rpo_number) {
        continue;
      }
      if (!Dominates(blocks, target, i)) {
        return false;
      }
      if (loops[target] == NULL) {
        loops[target] = NewBitSet();
      }
      CollectLoopBody(blocks, target, i, loops[target]);
    }
  }
  return true;
}

// Lay the blocks out so that a loop's blocks are contiguous and its header
// comes first.  Reverse postorder alone does not give that: a loop written
// with its test at the bottom has the header last, which is why the order
// the instructions arrived in cannot be used as it stands.
static void PlaceBlockAndLoop(Vector* order, BitSet** loops, char* placed,
                              size_t index, Vector* result) {
  if (placed[index]) {
    return;
  }
  placed[index] = true;
  VectorAppend(result, (void*)(intptr_t)index);
  if (loops[index] == NULL) {
    return;
  }
  // The rest of this loop's blocks go out before anything after the loop.
  for (size_t i = 0; i < order->length; i++) {
    size_t candidate = (size_t)(intptr_t)order->value.p[i];
    if (BitSetContains(loops[index], candidate)) {
      PlaceBlockAndLoop(order, loops, placed, candidate, result);
    }
  }
}

static void OrderBlocks(Vector* blocks, Vector* order, BitSet** loops,
                        Vector* result) {
  char* placed = calloc(blocks->length, 1);
  for (size_t i = 0; i < order->length; i++) {
    PlaceBlockAndLoop(order, loops, placed,
                      (size_t)(intptr_t)order->value.p[i], result);
  }
  // Anything unreachable never made it into the ordering but still has to
  // appear somewhere, because its instructions are still in the list.
  for (size_t i = 0; i < blocks->length; i++) {
    if (!placed[i]) {
      placed[i] = true;
      VectorAppend(result, (void*)(intptr_t)i);
    }
  }
  free(placed);
}

// Moving blocks apart breaks the fallthroughs between them, so every one has
// to be spelled out as a branch first, and every block needs a label for
// those branches to name.
static void MakeEdgesExplicit(Wasm32Generator* wasm, Vector* blocks) {
  for (size_t i = 0; i < blocks->length; i++) {
    StackifyBlock* block = blocks->value.p[i];
    if (block->label == NULL) {
      TargetInstruction* label = TargetNewInstruction((TargetOpcode)W_OP(label));
      label->flags |= WASM32_FLAG_NO_RESULT;
      TargetEmitBefore(&wasm->base, label, block->anchor);
      block->label = label;
      block->anchor = label;
    }
  }
  for (size_t i = 0; i + 1 < blocks->length; i++) {
    TargetInstruction* terminator = BlockTerminator(blocks, i);
    // A conditional branch leaves by two edges and only one of them is
    // written down, so the other needs spelling out as well.
    if (terminator != NULL &&
        terminator->opcode != (TargetOpcode)W_OP(br_if)) {
      continue;
    }
    StackifyBlock* next = blocks->value.p[i + 1];
    TargetInstruction* branch = TargetNewInstruction((TargetOpcode)W_OP(br));
    branch->operand[1] = next->label;
    branch->flags |= WASM32_FLAG_NO_RESULT;
    TargetUpdateOperandUsers(branch);
    TargetEmitBefore(&wasm->base, branch, next->anchor);
  }
}

// Splice the instruction ranges into the chosen order.  Each instruction is
// taken out and put back exactly once, so the list ends up holding the same
// instructions in the new sequence.
static void ReorderInstructions(Wasm32Generator* wasm, Vector* blocks,
                                Vector* order) {
  Vector sequence;
  VectorInit(&sequence);
  for (size_t i = 0; i < order->length; i++) {
    size_t index = (size_t)(intptr_t)order->value.p[i];
    StackifyBlock* block = blocks->value.p[index];
    TargetInstruction* limit =
        (index + 1 < blocks->length)
            ? ((StackifyBlock*)blocks->value.p[index + 1])->anchor
            : NULL;
    for (TargetInstruction* inst = block->anchor; inst != NULL && inst != limit;
         inst = TargetNext(inst)) {
      VectorAppend(&sequence, inst);
    }
  }
  for (size_t i = 0; i < sequence.length; i++) {
    TargetInstruction* inst = sequence.value.p[i];
    ListDeleteElement(&wasm->base.code, &inst->header);
    // Deleting leaves the element pointing at its old neighbours.
    ListElementInit(&inst->header);
    ListAppend(&wasm->base.code, &inst->header);
  }
  VectorDestruct(&sequence);
}

// A scope to wrap around a run of blocks.  A block scope is branched to at
// its end and a loop scope at its start, which is what decides which edge of
// a scope may be moved when two of them overlap without nesting.
typedef struct {
  bool is_loop;
  size_t begin;  // First block inside the scope.
  size_t end;    // One past the last block inside the scope.
} StackifyScope;

// The scopes the graph asks for.  Every back edge needs a loop its source
// can branch to, and every forward branch a block whose end is the target.
// Returns false when a backward branch does not target a block that
// dominates it, which is exactly an irreducible graph.
static bool BuildScopes(Vector* blocks, Vector* scopes) {
  size_t count = blocks->length;
  // Widest extent of a scope of each kind for each block, or none.
  int* loop_end = malloc(count * sizeof(int));
  int* block_begin = malloc(count * sizeof(int));
  for (size_t i = 0; i < count; i++) {
    loop_end[i] = -1;
    block_begin[i] = -1;
  }

  // Only an explicit branch needs a scope.  Running off the end of a block
  // into the next one is already how wasm behaves.  Code nothing reaches is
  // still code the module has to validate, so its branches count too, and
  // since it sorts to the end its every branch runs backwards - which fails
  // the dominance test below and sends the function to the dispatch shape.
  bool reducible = true;
  for (size_t i = 0; i < count && reducible; i++) {
    TargetInstruction* terminator = BlockTerminator(blocks, i);
    if (terminator == NULL ||
        terminator->opcode == (TargetOpcode)W_OP(return)) {
      continue;
    }
    size_t target = (size_t)BlockIndexOfLabel(blocks, terminator->operand[1]);
    if (target > i) {
      if (terminator->opcode == (TargetOpcode)W_OP(br) && target == i + 1) {
        continue;
      }
      if (block_begin[target] < 0 || (size_t)block_begin[target] > i) {
        block_begin[target] = (int)i;
      }
    } else if (!Dominates(blocks, target, i)) {
      reducible = false;
    } else if (loop_end[target] < (int)i + 1) {
      loop_end[target] = (int)i + 1;
    }
  }

  if (reducible) {
    for (size_t i = 0; i < count; i++) {
      if (loop_end[i] >= 0) {
        StackifyScope* scope = malloc(sizeof(StackifyScope));
        scope->is_loop = true;
        scope->begin = i;
        scope->end = (size_t)loop_end[i];
        VectorAppend(scopes, scope);
      }
      if (block_begin[i] >= 0) {
        StackifyScope* scope = malloc(sizeof(StackifyScope));
        scope->is_loop = false;
        scope->begin = (size_t)block_begin[i];
        scope->end = i;
        VectorAppend(scopes, scope);
      }
    }
  }

  free(loop_end);
  free(block_begin);
  return reducible;
}

// Scopes have to nest.  Two that overlap only partly can often be made to
// nest by growing one of them, but only in the direction nothing branches
// to: a block scope may start earlier and a loop scope may end later.
static bool FixScopeNesting(Vector* scopes) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t a = 0; a < scopes->length; a++) {
      for (size_t b = 0; b < scopes->length; b++) {
        if (a == b) {
          continue;
        }
        StackifyScope* first = scopes->value.p[a];
        StackifyScope* second = scopes->value.p[b];
        // Overlapping with the first opening strictly earlier and closing
        // strictly inside the second.
        if (!(first->begin < second->begin && second->begin < first->end &&
              first->end < second->end)) {
          continue;
        }
        if (!second->is_loop) {
          second->begin = first->begin;
        } else if (first->is_loop) {
          first->end = second->end;
        } else {
          return false;
        }
        changed = true;
      }
    }
  }
  return true;
}

static TargetInstruction* NewScope(Wasm32Opcode opcode) {
  TargetInstruction* inst = TargetNewInstruction((TargetOpcode)opcode);
  inst->flags |= WASM32_FLAG_NO_RESULT;
  return inst;
}

// A scope marker waiting to be put in, before the block at 'at', or at the
// very end of the function when that is one past the last block.
typedef struct {
  size_t at;
  Wasm32Opcode opcode;
} StackifyMarker;

// What to do with the branch a block ends on.
#define STACKIFY_NO_BRANCH (-2)
#define STACKIFY_FALLS_THROUGH (-1)

// Walk the blocks in order keeping a stack of the scopes open at each one,
// working out where the markers go and what depth names the scope each
// branch is aiming at.  A branch to a block scope lands at its end, which is
// the target block; a branch to a loop scope lands back at its start.
//
// Nothing is written down here, so a graph that turns out to have no
// workable nesting after all can still be sent to the dispatch shape rather
// than leaving the function half rewritten.
static bool ResolveScopes(Vector* blocks, Vector* scopes, Vector* markers,
                          int* depths) {
  Vector open;  // Stack of StackifyScope*, outermost first.
  VectorInit(&open);
  bool ok = true;

  for (size_t i = 0; i <= blocks->length && ok; i++) {
    // Close everything that ends here, innermost first.
    while (open.length > 0 &&
           ((StackifyScope*)open.value.p[open.length - 1])->end == i) {
      VectorPop(&open);
      StackifyMarker* marker = malloc(sizeof(StackifyMarker));
      marker->at = i;
      marker->opcode = W_OP(end);
      VectorAppend(markers, marker);
    }
    if (i == blocks->length) {
      break;
    }
    // Open everything that starts here, widest first so it ends up outermost.
    // A loop and a block starting together nest with the loop outside, since
    // a block scope only ever reaches to where the loop already runs.
    for (;;) {
      StackifyScope* next = NULL;
      for (size_t s = 0; s < scopes->length; s++) {
        StackifyScope* scope = scopes->value.p[s];
        if (scope->begin != i) {
          continue;
        }
        bool already_open = false;
        for (size_t o = 0; o < open.length && !already_open; o++) {
          already_open = open.value.p[o] == scope;
        }
        if (already_open) {
          continue;
        }
        // Where a loop and a block cover the same run, the block goes
        // outside: a branch out of the loop has to land past its end, not on
        // it, or the loop would simply go round again.
        if (next == NULL || scope->end > next->end ||
            (scope->end == next->end && !scope->is_loop)) {
          next = scope;
        }
      }
      if (next == NULL) {
        break;
      }
      StackifyMarker* marker = malloc(sizeof(StackifyMarker));
      marker->at = i;
      marker->opcode = next->is_loop ? W_OP(loop) : W_OP(block);
      VectorAppend(markers, marker);
      VectorAppend(&open, next);
    }

    // Now that the stack is right for this block, give its branch a depth.
    TargetInstruction* terminator = BlockTerminator(blocks, i);
    if (terminator == NULL ||
        terminator->opcode == (TargetOpcode)W_OP(return)) {
      depths[i] = STACKIFY_NO_BRANCH;
      continue;
    }
    size_t target = (size_t)BlockIndexOfLabel(blocks, terminator->operand[1]);
    if (terminator->opcode == (TargetOpcode)W_OP(br) && target == i + 1) {
      // Branching to what comes next anyway, so no scope was made for it.
      depths[i] = STACKIFY_FALLS_THROUGH;
      continue;
    }
    depths[i] = -1;
    for (size_t o = 0; o < open.length; o++) {
      StackifyScope* scope = open.value.p[o];
      bool lands_on_target =
          scope->is_loop ? scope->begin == target : scope->end == target;
      if (lands_on_target) {
        depths[i] = (int)(open.length - 1 - o);
        break;
      }
    }
    ok = depths[i] >= 0;
  }

  // Properly nested scopes close in the order they opened, so anything still
  // open means the nesting was not what it was taken to be and the markers
  // would not balance.
  ok = ok && open.length == 0;
  VectorDestruct(&open);
  return ok;
}

static void EmitScopes(Wasm32Generator* wasm, Vector* blocks, Vector* markers,
                       int* depths) {
  // The branches first, while a block's extent is still just its own code
  // and the terminator in it is the one that was resolved.
  for (size_t i = 0; i < blocks->length; i++) {
    if (depths[i] == STACKIFY_NO_BRANCH) {
      continue;
    }
    TargetInstruction* terminator = BlockTerminator(blocks, i);
    terminator->operand[1] = NULL;
    if (depths[i] == STACKIFY_FALLS_THROUGH) {
      terminator->flags |= TARGET_INST_DEAD;
    } else {
      terminator->addr = depths[i];
    }
  }
  // In walk order, so markers sharing a position keep the order they were
  // decided in: TargetEmitBefore puts each one after the last.
  for (size_t i = 0; i < markers->length; i++) {
    StackifyMarker* marker = markers->value.p[i];
    if (marker->at < blocks->length) {
      TargetEmitBefore(&wasm->base, NewScope(marker->opcode),
                       ((StackifyBlock*)blocks->value.p[marker->at])->anchor);
    } else {
      TargetEmit(&wasm->base, NewScope(marker->opcode));
    }
  }
}

// ---------------------------------------------------------------------------
// The dispatch shape, for a graph the structured one cannot express.
// ---------------------------------------------------------------------------

static void Place(Wasm32Generator* wasm, TargetInstruction* inst,
                  TargetInstruction* position) {
  if (position != NULL) {
    TargetEmitBefore(&wasm->base, inst, position);
  } else {
    TargetEmit(&wasm->base, inst);
  }
}

static TargetInstruction* NewConstant(Wasm32Generator* wasm, int32_t value) {
  TargetInstruction* inst = TargetNewInstruction1(
      (TargetOpcode)W_OP(i32_const),
      TargetGetIntConstant(&wasm->base, NULL, kTargetType32Bit, value));
  Wasm32SetInstructionType(inst, kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  return inst;
}

static TargetInstruction* EmitConstAt(Wasm32Generator* wasm, int32_t value,
                                      TargetInstruction* position) {
  TargetInstruction* inst = NewConstant(wasm, value);
  Place(wasm, inst, position);
  return inst;
}

static TargetInstruction* EmitLoadAt(Wasm32Generator* wasm,
                                     TargetInstruction* address, int32_t offset,
                                     TargetInstruction* position) {
  TargetInstruction* inst =
      TargetNewInstruction1((TargetOpcode)W_OP(i32_load), address);
  inst->operand[1] =
      TargetGetIntConstant(&wasm->base, NULL, kTargetType32Bit, offset);
  Wasm32SetInstructionType(inst, kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  Place(wasm, inst, position);
  return inst;
}

static void EmitStoreAt(Wasm32Generator* wasm, TargetInstruction* address,
                        TargetInstruction* value, int32_t offset,
                        TargetInstruction* position) {
  TargetInstruction* inst =
      TargetNewInstruction2((TargetOpcode)W_OP(i32_store), address, value);
  inst->operand[2] =
      TargetGetIntConstant(&wasm->base, NULL, kTargetType32Bit, offset);
  inst->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(inst);
  Place(wasm, inst, position);
}

static void EmitLocalSetAt(Wasm32Generator* wasm, TargetInstruction* dest,
                           TargetInstruction* value,
                           TargetInstruction* position) {
  TargetInstruction* inst =
      TargetNewInstruction1((TargetOpcode)W_OP(local_set), value);
  inst->dest = dest;
  inst->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(inst);
  Place(wasm, inst, position);
}

static void PatchSetjmpContinuations(Vector* blocks) {
  for (size_t i = 0; i < blocks->length; i++) {
    StackifyBlock* block = blocks->value.p[i];
    TargetInstruction* limit =
        (i + 1 < blocks->length)
            ? ((StackifyBlock*)blocks->value.p[i + 1])->anchor
            : NULL;
    for (TargetInstruction* inst = block->anchor; inst != NULL && inst != limit;
         inst = TargetNext(inst)) {
      if (inst->opcode == (TargetOpcode)W_OP(setjmp_cont)) {
        inst->addr = (int)i;
      }
    }
  }
}

static void EmitSetjmpHandler(Wasm32Generator* wasm,
                              TargetInstruction* selector,
                              TargetInstruction* thrown,
                              TargetInstruction* position) {
  TargetInstruction* addr = EmitConstAt(wasm, WASM32_LONGJMP_PENDING, position);
  TargetInstruction* pending = EmitLoadAt(wasm, addr, 0, position);
  EmitLocalSetAt(wasm, thrown, pending, position);

  TargetInstruction* if_pending = NewScope(W_OP(if));
  if_pending->operand[0] = thrown;
  TargetUpdateOperandUsers(if_pending);
  Place(wasm, if_pending, position);

  TargetInstruction* saved_owner = EmitLoadAt(wasm, thrown, 8, position);
  TargetInstruction* ne = TargetNewInstruction2(
      (TargetOpcode)W_OP(i32_ne), saved_owner, wasm->setjmp_owner);
  Wasm32SetInstructionType(ne, kWasmTypeI32);
  TargetUpdateOperandUsers(ne);
  Place(wasm, ne, position);

  TargetInstruction* if_foreign = NewScope(W_OP(if));
  if_foreign->operand[0] = ne;
  TargetUpdateOperandUsers(if_foreign);
  Place(wasm, if_foreign, position);

  TargetInstruction* rethrow = TargetNewInstruction((TargetOpcode)W_OP(throw));
  rethrow->flags |= WASM32_FLAG_NO_RESULT;
  Place(wasm, rethrow, position);
  Place(wasm, NewScope(W_OP(end)), position);

  TargetInstruction* sp = EmitLoadAt(wasm, thrown, 0, position);
  TargetInstruction* set_sp =
      TargetNewInstruction1((TargetOpcode)W_OP(global_set), sp);
  set_sp->operand[1] = TargetGetIntConstant(&wasm->base, NULL, kTargetType32Bit,
                                            WASM32_STACK_POINTER_GLOBAL);
  set_sp->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(set_sp);
  Place(wasm, set_sp, position);

  TargetInstruction* value = EmitLoadAt(wasm, thrown, 12, position);
  EmitLocalSetAt(wasm, wasm->setjmp_result, value, position);
  TargetInstruction* cont = EmitLoadAt(wasm, thrown, 4, position);
  EmitLocalSetAt(wasm, selector, cont, position);

  TargetInstruction* zero = EmitConstAt(wasm, 0, position);
  TargetInstruction* clear_addr =
      EmitConstAt(wasm, WASM32_LONGJMP_PENDING, position);
  EmitStoreAt(wasm, clear_addr, zero, 0, position);

  Place(wasm, NewScope(W_OP(end)), position);
}

// Emit "selector = target; br depth" before 'position'.
static void EmitJumpToBlock(Wasm32Generator* wasm, TargetInstruction* selector,
                            int target, int depth,
                            TargetInstruction* position) {
  TargetInstruction* value = NewConstant(wasm, target);
  TargetEmitBefore(&wasm->base, value, position);

  TargetInstruction* set =
      TargetNewInstruction1((TargetOpcode)W_OP(local_set), value);
  set->dest = selector;
  set->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(set);
  TargetEmitBefore(&wasm->base, set, position);

  TargetInstruction* branch = TargetNewInstruction((TargetOpcode)W_OP(br));
  branch->addr = depth;
  branch->flags |= WASM32_FLAG_NO_RESULT;
  TargetEmitBefore(&wasm->base, branch, position);
}

static void StackifyByDispatch(Wasm32Generator* wasm, Vector* blocks) {
  size_t num_blocks = blocks->length;
  bool catch_longjmp = wasm->has_setjmp && wasm->setjmp_result != NULL &&
                       wasm->setjmp_owner != NULL;

  if (catch_longjmp) {
    PatchSetjmpContinuations(blocks);
  }

  // The selector local the dispatch reads.  Wasm zero-initializes locals, so
  // the first pass through the dispatch naturally selects block 0.
  TargetInstruction* selector = TargetNewInstruction((TargetOpcode)W_OP(slot));
  Wasm32SetInstructionType(selector, kWasmTypeI32);
  TargetInstruction* thrown = NULL;
  if (catch_longjmp) {
    thrown = TargetNewInstruction((TargetOpcode)W_OP(slot));
    Wasm32SetInstructionType(thrown, kWasmTypeI32);
  }

  TargetInstruction* first = TargetFirstInstruction(&wasm->base);

  // Put the scope markers in first.  Rewriting a terminator inserts code
  // just before it, and the last terminator of a block sits immediately
  // before the next block's first instruction, so if the boundary marker
  // were not already there that code would land in the wrong scope.
  //
  // setjmp wraps as:
  //   block $done
  //     loop
  //       block $caught
  //         try_table catch_all $caught
  //           ... dispatch ...
  //         end
  //         br $done
  //       end $caught
  //       handler
  //       br loop
  //     end loop
  //   end $done
  if (catch_longjmp) {
    TargetEmitBefore(&wasm->base, NewScope(W_OP(block)), first);
  }
  TargetEmitBefore(&wasm->base, NewScope(W_OP(loop)), first);
  TargetInstruction* try_table = NULL;
  if (catch_longjmp) {
    TargetEmitBefore(&wasm->base, NewScope(W_OP(block)), first);
    try_table = NewScope(W_OP(try_table));
    // Catch indices skip the try_table's own branch label, so 0 is $caught.
    try_table->addr = 0;
    TargetEmitBefore(&wasm->base, try_table, first);
  }
  for (size_t i = 0; i < num_blocks; i++) {
    TargetEmitBefore(&wasm->base, NewScope(W_OP(block)), first);
  }
  TargetInstruction* dispatch =
      TargetNewInstruction1((TargetOpcode)W_OP(br_table), selector);
  // The encoder reads the table size from addr: targets 0..N-1 sit at depths
  // 0..N-1 by construction, and the default is block 0.
  dispatch->addr = (int)num_blocks;
  dispatch->flags |= WASM32_FLAG_NO_RESULT;
  TargetUpdateOperandUsers(dispatch);
  TargetEmitBefore(&wasm->base, dispatch, first);
  TargetEmitBefore(&wasm->base, NewScope(W_OP(end)), first);

  // Close block k just before block k's code starts.
  for (size_t i = 1; i < num_blocks; i++) {
    StackifyBlock* block = blocks->value.p[i];
    TargetEmitBefore(&wasm->base, NewScope(W_OP(end)), block->anchor);
  }

  if (catch_longjmp) {
    TargetEmit(&wasm->base, NewScope(W_OP(end)));  // try_table
    TargetInstruction* leave = TargetNewInstruction((TargetOpcode)W_OP(br));
    leave->addr = 2;  // $done: 0=$caught, 1=loop, 2=$done
    leave->flags |= WASM32_FLAG_NO_RESULT;
    TargetEmit(&wasm->base, leave);
    TargetEmit(&wasm->base, NewScope(W_OP(end)));  // $caught
    EmitSetjmpHandler(wasm, selector, thrown, NULL);
    TargetInstruction* retry = TargetNewInstruction((TargetOpcode)W_OP(br));
    retry->addr = 0;  // loop
    retry->flags |= WASM32_FLAG_NO_RESULT;
    TargetEmit(&wasm->base, retry);
  }
  TargetEmit(&wasm->base, NewScope(W_OP(end)));  // loop
  if (catch_longjmp) {
    TargetEmit(&wasm->base, NewScope(W_OP(end)));  // $done
  }

  for (size_t i = 0; i < num_blocks; i++) {
    StackifyBlock* block = blocks->value.p[i];
    TargetInstruction* limit =
        (i + 1 < num_blocks)
            ? ((StackifyBlock*)blocks->value.p[i + 1])->anchor
            : NULL;
    // Distance from inside block i out to the loop.
    int loop_depth = (int)(num_blocks - 1 - i);
    if (catch_longjmp) {
      loop_depth += 2;  // try_table and $caught
    }

    for (TargetInstruction* inst = block->anchor; inst != NULL && inst != limit;
         inst = TargetNext(inst)) {
      if (IsTerminator(inst)) {
        TargetInstruction* label = inst->operand[1];
        int target = label != NULL ? BlockIndexOfLabel(blocks, label) : -1;
        if (inst->opcode == (TargetOpcode)W_OP(br)) {
          if (target < 0) {
            // A branch whose target was optimized away cannot happen; leave
            // it alone rather than emitting something wrong.
            break;
          }
          inst->operand[1] = NULL;
          if (target == (int)i + 1) {
            // Falls straight into the next block.
            inst->flags |= TARGET_INST_DEAD;
          } else {
            EmitJumpToBlock(wasm, selector, target, loop_depth, inst);
            inst->flags |= TARGET_INST_DEAD;
          }
        } else if (inst->opcode == (TargetOpcode)W_OP(br_if)) {
          // A conditional branch becomes an if whose body performs the
          // dispatch; the depth inside the if is one greater.
          TargetInstruction* condition = inst->operand[0];
          inst->operand[0] = NULL;
          inst->operand[1] = NULL;
          inst->flags |= TARGET_INST_DEAD;

          TargetInstruction* scope = NewScope(W_OP(if));
          scope->operand[0] = condition;
          TargetUpdateOperandUsers(scope);
          TargetEmitBefore(&wasm->base, scope, inst);
          if (target >= 0) {
            EmitJumpToBlock(wasm, selector, target, loop_depth + 1, inst);
          }
          TargetEmitBefore(&wasm->base, NewScope(W_OP(end)), inst);
        }
      }
    }
  }

  // The selector pseudo has to be in the list for the local allocator to
  // find it.  It encodes to nothing.
  TargetEmitBefore(&wasm->base, selector, TargetFirstInstruction(&wasm->base));
  if (thrown != NULL) {
    TargetEmitBefore(&wasm->base, thrown, TargetFirstInstruction(&wasm->base));
  }
}

// Work out an order the blocks can be laid out in for the structured shape,
// or report that this graph has none.
static bool PlanBlockOrder(Vector* blocks, Vector* order) {
  if (!BuildEdges(blocks)) {
    return false;
  }
  MarkReachable(blocks);
  BuildPredecessors(blocks);

  Vector reverse_postorder;
  VectorInit(&reverse_postorder);
  ComputeReversePostorder(blocks, &reverse_postorder);
  ComputeDominators(blocks, &reverse_postorder);

  BitSet** loops = malloc(blocks->length * sizeof(BitSet*));
  bool reducible = FindLoops(blocks, loops);
  if (reducible) {
    OrderBlocks(blocks, &reverse_postorder, loops, order);
  }

  for (size_t i = 0; i < blocks->length; i++) {
    if (loops[i] != NULL) {
      BitSetDelete(loops[i]);
    }
  }
  free(loops);
  VectorDestruct(&reverse_postorder);
  return reducible;
}

// Give the reordered blocks their scopes, or report that no nesting of them
// works.  Nothing has been emitted when this fails, so the dispatch shape is
// still available.
static bool ApplyScopes(Wasm32Generator* wasm, Vector* blocks) {
  if (!BuildEdges(blocks)) {
    return false;
  }
  MarkReachable(blocks);
  BuildPredecessors(blocks);

  Vector reverse_postorder;
  VectorInit(&reverse_postorder);
  ComputeReversePostorder(blocks, &reverse_postorder);
  ComputeDominators(blocks, &reverse_postorder);
  VectorDestruct(&reverse_postorder);

  Vector scopes;
  VectorInit(&scopes);
  Vector markers;
  VectorInit(&markers);
  int* depths = malloc(blocks->length * sizeof(int));

  bool ok = BuildScopes(blocks, &scopes) && FixScopeNesting(&scopes) &&
            ResolveScopes(blocks, &scopes, &markers, depths);
  if (ok) {
    EmitScopes(wasm, blocks, &markers, depths);
  }

  free(depths);
  VectorDestructWithContents(&markers, NULL, /*free_element=*/true);
  VectorDestructWithContents(&scopes, NULL, /*free_element=*/true);
  return ok;
}

void Wasm32Stackify(Wasm32Generator* wasm) {
  Vector blocks;
  VectorInit(&blocks);
  CollectBlocks(wasm, &blocks);

  if (blocks.length <= 1 && !wasm->has_setjmp) {
    // Straight-line code needs no scopes at all.
    DestructBlocks(&blocks);
    return;
  }

  Vector order;
  VectorInit(&order);
  bool structured = !wasm->has_setjmp && PlanBlockOrder(&blocks, &order);
  if (structured) {
    MakeEdgesExplicit(wasm, &blocks);
    ReorderInstructions(wasm, &blocks, &order);
    // The blocks have moved, so their positions - which is what a depth is
    // counted in - have to be read off the list again.
    DestructBlocks(&blocks);
    VectorInit(&blocks);
    CollectBlocks(wasm, &blocks);
    structured = ApplyScopes(wasm, &blocks);
  }
  if (!structured) {
    if (blocks.length == 0) {
      CollectBlocks(wasm, &blocks);
    }
    StackifyByDispatch(wasm, &blocks);
  }
  VectorDestruct(&order);
  DestructBlocks(&blocks);
}
