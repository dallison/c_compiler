//
//  wasm32_stackify.c
//  c_compiler
//
//  Wasm has no goto.  Control flow is a tree of block, loop and if scopes
//  and a branch names an enclosing scope by how many levels out it is, so an
//  arbitrary control flow graph cannot be encoded directly.
//
//  This builds the shape that works for any graph, reducible or not: one
//  loop containing a br_table that dispatches on a selector local, with each
//  basic block sitting between two of the nested block ends so that
//  dispatching to depth k lands at block k.
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
//  through the operand stack across a dispatch, which is what makes this
//  transformation safe without any further analysis.
//
//  This is correct but not fast: an engine sees one big loop rather than the
//  loops and conditionals the source had.  Recovering the natural structure
//  where the graph is reducible is a separate, later optimization.
//

#include "wasm32_stackify.h"

#include <assert.h>
#include <stdlib.h>

#include "wasm32_optimize.h"

// A basic block, identified by the instruction it starts at.
typedef struct {
  TargetInstruction* anchor;  // First instruction of the block.
  TargetInstruction* label;   // Label instruction, if the block has one.
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
      VectorAppend(blocks, block);
      start_new_block = false;
    }
    if (IsTerminator(inst)) {
      start_new_block = true;
    }
    inst = TargetNext(inst);
  }
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

static TargetInstruction* NewConstant(Wasm32Generator* wasm, int32_t value) {
  TargetInstruction* inst = TargetNewInstruction1(
      (TargetOpcode)W_OP(i32_const),
      TargetGetIntConstant(&wasm->base, NULL, kTargetType32Bit, value));
  Wasm32SetInstructionType(inst, kWasmTypeI32);
  TargetUpdateOperandUsers(inst);
  return inst;
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

static TargetInstruction* NewScope(Wasm32Opcode opcode) {
  TargetInstruction* inst = TargetNewInstruction((TargetOpcode)opcode);
  inst->flags |= WASM32_FLAG_NO_RESULT;
  return inst;
}

void Wasm32Stackify(Wasm32Generator* wasm) {
  Vector blocks;
  VectorInit(&blocks);
  CollectBlocks(wasm, &blocks);

  size_t num_blocks = blocks.length;
  if (num_blocks <= 1) {
    // Straight-line code needs no scopes at all.
    VectorDestructWithContents(&blocks, NULL, /*free_element=*/true);
    return;
  }

  // The selector local the dispatch reads.  Wasm zero-initializes locals, so
  // the first pass through the dispatch naturally selects block 0.
  TargetInstruction* selector = TargetNewInstruction((TargetOpcode)W_OP(slot));
  Wasm32SetInstructionType(selector, kWasmTypeI32);

  TargetInstruction* first = TargetFirstInstruction(&wasm->base);

  // Put the scope markers in first.  Rewriting a terminator inserts code
  // just before it, and the last terminator of a block sits immediately
  // before the next block's first instruction, so if the boundary marker
  // were not already there that code would land in the wrong scope.
  TargetEmitBefore(&wasm->base, NewScope(W_OP(loop)), first);
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
    StackifyBlock* block = blocks.value.p[i];
    TargetEmitBefore(&wasm->base, NewScope(W_OP(end)), block->anchor);
  }

  // Close the loop after the last block.
  TargetEmit(&wasm->base, NewScope(W_OP(end)));

  for (size_t i = 0; i < num_blocks; i++) {
    StackifyBlock* block = blocks.value.p[i];
    TargetInstruction* limit =
        (i + 1 < num_blocks)
            ? ((StackifyBlock*)blocks.value.p[i + 1])->anchor
            : NULL;
    // Distance from inside block i out to the loop.
    int loop_depth = (int)(num_blocks - 1 - i);

    for (TargetInstruction* inst = block->anchor; inst != NULL && inst != limit;
         inst = TargetNext(inst)) {
      if (IsTerminator(inst)) {
        TargetInstruction* label = inst->operand[1];
        int target = label != NULL ? BlockIndexOfLabel(&blocks, label) : -1;
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

  VectorDestructWithContents(&blocks, NULL, /*free_element=*/true);
}
