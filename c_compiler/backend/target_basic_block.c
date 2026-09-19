//
//  risc_v_basic_block.c
//  c_compiler_library
//
//  Created by David Allison on 5/27/20.
//  Copyright © 2020 David Allison. All rights resegened.
//

#include "target_basic_block.h"
#include "compiler.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

TargetBasicBlock* NewTargetBasicBlock(TargetBlockId id) {
  TargetBasicBlock* b = malloc(sizeof(TargetBasicBlock));
  b->block_id = id;
  b->code = NULL;
  b->end_code = NULL;
  VectorInit(&b->in_edges);
  VectorInit(&b->out_edges);
  BitSetInit(&b->dominators);
  b->num_dominators = 0;
  VectorInit(&b->dominatees);
  BitSetInit(&b->dominance_frontier);
  b->idom = NULL;
  VectorInit(&b->inputs);
  BitSetInit(&b->input_ids);
  VectorInit(&b->outputs);
  BitSetInit(&b->output_ids);
  b->num_spills = 0;
  b->loop_nesting = 0;
  b->contains_call = false;
  b->reachability_known = false;
  b->is_unreachable = false;
  b->uses_floating_point = false;
  b->cookie = NULL;
  return b;
}

void TargetBasicBlockDelete(TargetBasicBlock* b) {
  // NOTE: the instructions are not owned by the TargetBasicBlock.  They belong to the
  // TargetGenerator and that is responsible for deleting them.
  VectorDestruct(&b->in_edges);
  VectorDestruct(&b->out_edges);
  BitSetDestruct(&b->dominators);
  BitSetDestruct(&b->dominance_frontier);
  VectorDestruct(&b->dominatees);
  VectorDestruct(&b->inputs);
  BitSetDestruct(&b->input_ids);
  VectorDestruct(&b->outputs);
  BitSetDestruct(&b->output_ids);
  free(b);
}

void TargetBasicBlockAddInEdge(TargetBasicBlock* from, TargetBasicBlock* to) {
  VectorAppend(&from->in_edges, (void*)to->block_id);
}

void TargetBasicBlockAddEdge(TargetBasicBlock* from, TargetBasicBlock* to) {
  assert(to->block_id != 0);
  VectorAppend(&from->out_edges, (void*)to->block_id);
  TargetBasicBlockAddInEdge(to, from);
}

bool TargetBasicBlockCalculateDominators(TargetGenerator* gen, TargetBasicBlock* b, Vector* blocks) {
  BitSet dominators;
  BitSetInit(&dominators);
  BitSetCopy(&dominators, &b->dominators);

  BitSet intersection;
  BitSetInit(&intersection);

  // Calculate intersection.
  int num_intersections = 0;
  for (size_t i = 0; i < b->in_edges.length; i++) {
    BlockId id = b->in_edges.value.w[i];
    TargetBasicBlock* dom_block = blocks->value.p[id];

    // Unreachable blocks do not count.
    if (TargetBasicBlockIsUnreachable(gen, dom_block)) {
      continue;
    }
    BitSetClear(&intersection);
    BitSetIntersection(&dominators, &dom_block->dominators, &intersection);
    BitSetClear(&dominators);
    BitSetCopy(&dominators, &intersection);
    num_intersections++;
  }
  if (num_intersections == 0) {
    // All in edges are unreachable, so this is unreachable.
    BitSetClear(&dominators);
  }

  // Insert this node.
  BitSetInsert(&dominators, b->block_id);

  // If we have changed, store the new dominators.
  bool changed = !BitSetEqual(&dominators, &b->dominators);
  if (changed) {
    BitSetClear(&b->dominators);
    BitSetCopy(&b->dominators, &dominators);
  }

  // Clean up.
  BitSetDestruct(&dominators);
  BitSetDestruct(&intersection);

  // Count the number of dominators so we don't have to do it on
  // every iteration when calculating the immediate dominator.
  b->num_dominators = BitSetCount(&b->dominators);
  return changed;
}

void TargetBasicBlockCalculateImmediateDominator(TargetBasicBlock* b, Vector* blocks) {
  if (b->in_edges.length == 1) {
    TargetBasicBlock* predecessor =
        VectorGet(blocks, b->in_edges.value.w[0]);
    if (predecessor != b &&
        BitSetContains(&b->dominators, predecessor->block_id)) {
      b->idom = predecessor;
      return;
    }
  }

  size_t maxndoms = 0;

  BitSetIterator it;
  BitSetIteratorStart(&it, &b->dominators);
  while (!BitSetIteratorDone(&it)) {
    BlockId id = (BlockId)BitSetIteratorValue(&it);
    if (id != b->block_id) {
      TargetBasicBlock* block = VectorGet(blocks, id);
      if (block->num_dominators > maxndoms) {
        maxndoms = block->num_dominators;
        b->idom = block;
      }
    }
    BitSetIteratorNext(&it);
  }
}

void TargetBasicBlockCalculateDominanceFrontier(TargetBasicBlock* b, Vector* blocks) {
  if (b->in_edges.length >= 2) {
    for (size_t i = 0; i < b->in_edges.length; i++) {
      BlockId id = (BlockId)b->in_edges.value.p[i];
      TargetBasicBlock* block = VectorGet(blocks, id);
      TargetBasicBlock* runner = block;
      while (runner != b->idom) { // TODO: this was NULL, which is right?
        TargetBasicBlockAddToDF(runner, b->block_id);
        runner = runner->idom;
      }
    }
  }
}

void TargetBasicBlockInitDominators(TargetBasicBlock* b, bool is_start, size_t num_nodes) {
  if (is_start) {
    BitSetInsert(&b->dominators, b->block_id);
  } else {
    if (b->in_edges.length == 0) {
      // Block is unreachable.
      return;
    }
    BitSetFill(&b->dominators, num_nodes);
  }
}

void TargetBasicBlockAddToDF(TargetBasicBlock* b, BlockId id) {
  BitSetInsert(&b->dominance_frontier, id);
}

void TargetBasicBlockPrint(TargetGenerator* gen, TargetBasicBlock* b, TargetBasicBlock* entry, TargetBasicBlock* exit, FILE* fp) {
  fprintf(fp, "*** Target Basic block #%zd%s\n", b->block_id,
         (b == entry ? " (ENTRY)" : (b == exit ? " (EXIT)" : "")));
  if (TargetBasicBlockIsUnreachable(gen, b)) {
    fprintf(fp, "** Unreachable **\n");
  }
  if (b->contains_call) {
    fprintf(fp, "  [call]\n");
  }
  fprintf(fp, "  In:");
  for (size_t i = 0; i < b->in_edges.length; i++) {
    fprintf(fp, " %zd", (BlockId)b->in_edges.value.p[i]);
  }
  fprintf(fp, "\n  Out:");
  for (size_t i = 0; i < b->out_edges.length; i++) {
    fprintf(fp, " %zd", (BlockId)b->out_edges.value.p[i]);
  }
  fprintf(fp, "\n  Dominators: ");
  BitSetPrint(&b->dominators, fp);

  fprintf(fp, "\n  Dominatees:");
  for (size_t i = 0; i < b->dominatees.length; i++) {
    fprintf(fp, " %zd", (BlockId)b->dominatees.value.p[i]);
  }
  fprintf(fp, "\n  DF: ");
  BitSetPrint(&b->dominance_frontier, fp);

  fprintf(fp, "\n  Immediate Dominator: ");
  if (b->idom == NULL) {
    fprintf(fp, "NIL\n");
  } else {
    fprintf(fp, "%zd\n", b->idom->block_id);
  }

  fprintf(fp, "  Loop nesting: %d\n", b->loop_nesting);

  fprintf(fp, "  Inputs: ");
  for (size_t i = 0; i < b->inputs.length; i++) {
    TargetInstruction* inst = b->inputs.value.p[i];
    fprintf(fp, "@%d ", inst->id);
  }
  fprintf(fp, "\n");

  fprintf(fp, "  Outputs: ");
  for (size_t i = 0; i < b->outputs.length; i++) {
    TargetInstruction* inst = b->outputs.value.p[i];
    fprintf(fp, "@%d ", inst->id);
  }
  fprintf(fp, "\n");
  
  TargetInstruction* inst = b->code;
  while (inst != NULL && inst != b->end_code) {
    TargetPrintInstruction(inst, gen->virtuals->opcode_name, fp);
    inst = TargetNext(inst);
  }
  if (b->end_code != NULL) {
    TargetPrintInstruction(b->end_code, gen->virtuals->opcode_name, fp);
  }
  fprintf(fp, "\n");
}

bool TargetBasicBlockEndsInBranchOrReturn(TargetGenerator* gen, TargetBasicBlock* b) {
  TargetInstruction* inst = b->end_code;
  return inst != NULL && (gen->virtuals->is_branch(inst) ||
                          gen->virtuals->is_return(inst));
}

// Remove an instruction from a basic block and IR code.
void TargetBasicBlockRemoveInstruction(TargetGenerator* gen, TargetBasicBlock* block,
                                 TargetInstruction* inst) {
  // If this instruction is the first in the basic block, move the
  // block's code on to the next instruction.
  if (inst == block->code) {
    if (inst == block->end_code) {
      // Block has no instructions now.
      block->code = block->end_code = NULL;
    } else {
      block->code = TargetNext(inst);
    }
  }
  if (inst == block->end_code) {
    block->end_code = TargetPrev(inst);
  }
  
  // If this instruction is referred to by another instruction then
  // that instruction is also dead.
  for (size_t i = 0; i < inst->users.length; i++) {
     TargetInstruction* user = inst->users.value.p[i];
     for (int j = 0; j < TARGET_MAX_OPERANDS; j++) {
       if (user->operand[j] == inst) {
         // Need to remove the user instruciton.
         TargetBasicBlockRemoveInstruction(gen, user->block, user);
         user = NULL;
         break;
       }
     }
     if (user != NULL && user->dest == inst) {
       user->dest = NULL;
     }
   }
  VectorClear(&inst->users);
  if (inst->user_index != NULL) {
    VectorClear(inst->user_index);
  }
  TargetDeleteInstruction(gen, inst);
}

// Replace the instruction 'old' with 'new'.  Removes 'old' when
// the replacement is done.
void TargetBasicBlockReplaceInstruction(TargetGenerator* gen, TargetBasicBlock* block,
                                  TargetInstruction* old, TargetInstruction* new) {
  // If this instruction is the first in the basic block, move the
  // block's code on to the next instruction.
  if (old == block->code) {
    block->code = TargetNext(old);
  }
  if (old == block->end_code) {
    block->end_code = TargetPrev(old);
  }
  TargetReplaceInstruction(gen, old, new);
  TargetBasicBlockRemoveInstruction(gen, block, old);
}

void TargetBasicBlockEmitBefore(struct TargetGenerator* gen, TargetBasicBlock* block,
                          struct TargetInstruction* inst, struct TargetInstruction* pos) {
  // If the position is the first in the block we need to move
  // it to the new instruction.
  if (pos == block->code) {
    block->code = inst;
  }
  TargetEmitBefore(gen, inst, pos);
  inst->block = block;
  if (gen->virtuals->is_spill(inst)) {
    block->num_spills++;
  }
}

void TargetBasicBlockEmitAfter(struct TargetGenerator* gen, TargetBasicBlock* block,
                          struct TargetInstruction* inst, struct TargetInstruction* pos) {
  // If the position is the last in the block we need to move
  // it to the new instruction.
  if (pos == block->end_code) {
    block->end_code = inst;
  }
  TargetEmitAfter(gen, inst, pos);
  inst->block = block;
  if (gen->virtuals->is_spill(inst)) {
    block->num_spills++;
  }
}

bool TargetBasicBlockIsUnreachable(struct TargetGenerator* gen, TargetBasicBlock* b) {
  if (b == gen->entry_block) {
    return false;
  }
  if (b->reachability_known) {
    return b->is_unreachable;
  }
  b->reachability_known = true;
  // A landing pad is reached from the protected region rather than by a branch,
  // and AddExceptionHandlerEdges gives it that in edge.  So a pad with no live
  // in edge belongs to a region that is itself dead, and answering anything but
  // "unreachable" here would keep it out of the dominator tree the register
  // allocator walks while the emitter still emits it.
  // Check if all the in edges are unreachable.
  for (size_t i = 0; i < b->in_edges.length; i++) {
    BlockId id = b->in_edges.value.w[i];
    TargetBasicBlock* in = VectorGet(&gen->basic_blocks, id);
    if (!TargetBasicBlockIsUnreachable(gen, in)) {
      return false;
    }
  }
  b->is_unreachable = true;
  return true;
}

void TargetBasicBlockClear(struct TargetGenerator* gen, TargetBasicBlock* b) {
  if (b->code == NULL) {
    return;
  }
  TargetInstruction* next;
  for (TargetInstruction* inst = b->code; inst != NULL && inst != b->end_code; inst = next) {
    next = TargetNext(inst);
    TargetDeleteInstruction(gen, inst);
  }
  if (b->end_code != NULL) {
    TargetDeleteInstruction(gen, b->end_code);
  }
  b->code = NULL;
  b->end_code = NULL;
}

static bool TargetBasicBlockHasKeptInstruction(TargetBasicBlock* b) {
  if (b->code == NULL) {
    return false;
  }
  for (TargetInstruction* inst = b->code; inst != NULL && inst != b->end_code;
       inst = TargetNext(inst)) {
    if ((inst->flags & TARGET_INST_KEEP_UNREACHABLE) != 0) {
      return true;
    }
  }
  return b->end_code != NULL &&
         (b->end_code->flags & TARGET_INST_KEEP_UNREACHABLE) != 0;
}

static TargetBasicBlock* TargetGeneratorNewTargetBasicBlock(TargetGenerator* gen) {
  TargetBasicBlock* b = NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, b);
  return b;
}

static TargetBasicBlock* FindTargetBasicBlock(TargetGenerator* gen, BlockId id) {
  return VectorGet(&gen->basic_blocks, id);
}

static void CreateTargetBasicBlocks(TargetGenerator* gen,
                              Vector* branches) {
  gen->entry_block = TargetGeneratorNewTargetBasicBlock(gen);
  TargetBasicBlock* current = gen->entry_block;

  // First instruction is in first block.
  current->code = TargetFirstInstruction(gen);
  
  // Find the boundary TargetInstructions (labels, branches and returns).  Each
  // one of these either ends a block or starts a new one.
  for (TargetInstruction* inst = current->code; inst != NULL; inst = TargetNext(inst)) {
    if (gen->virtuals->is_floating_point(inst)) {
      current->uses_floating_point = true;
    }
    bool is_call = gen->virtuals->is_call(inst);
    if (is_call) {
      current->contains_call = true;
    }
    if (gen->virtuals->is_label(inst)) {
      current->end_code = TargetPrev(inst);
      
      // A label marks the start of a block.
      TargetBasicBlock* b = TargetGeneratorNewTargetBasicBlock(gen);
      inst->block = b;
      b->code = inst;
      current = b;
    } else if (is_call && gen->virtuals->calls_may_stay_in_block &&
               gen->exception_edges.length == 0) {
      inst->block = current;
    } else if (gen->virtuals->is_branch(inst) ||
               gen->virtuals->is_return(inst) ||
               is_call) {
      // Branch, return and call ends a block.
      current->end_code = inst;
      inst->block = current;
      VectorAppend(branches, inst);

      // Allocate a new block starting at the next instruction provided it's
      // not a label (because that will be created in next iteration).
      TargetInstruction* next = TargetNext(inst);
      if (next != NULL && !gen->virtuals->is_label(next)) {
        TargetBasicBlock* b = TargetGeneratorNewTargetBasicBlock(gen);
        b->code = next;
        current = b;
      }
    } else {
      inst->block = current;
    }
  }
  current->end_code = TargetLastInstruction(gen);
  
  // Allocate exit block.
  gen->exit_block = TargetGeneratorNewTargetBasicBlock(gen);
}

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
int CopyFile(const char* from, const char* to) {
  int infd = open(from, O_RDONLY);
  if (infd == -1) {
    return errno;
  }
  int outfd = open(to, O_WRONLY|O_CREAT|O_TRUNC, 0444);
  if (outfd == -1) {
    close(infd);
    return errno;
  }
  char buf[256];
  for (;;) {
    ssize_t n = read(infd, buf, sizeof(buf));
    if (n < 0) {
      return errno;
    }
    if (n == 0) {
      break;
    }
    n = write(outfd, buf, n);
    if (n < 0) {
      return errno;
    }
  }
  close(infd);
  close(outfd);
  return 0;
}

// Process all branches and link their targets to the appropriate
// block.
static void BuildTargetBasicBlockGraph(TargetGenerator* gen,
                                  Vector* branches) {
  for (size_t i = 0; i < branches->length; i++) {
    TargetInstruction* inst = branches->value.p[i];
    TargetBasicBlock* block = inst->block;
    if (gen->virtuals->is_conditional_branch(inst)) {
      // Conditional branch links to both its taken and fallthrough blocks.
      TargetInstruction* fallthrough = TargetNext(inst);
      TargetInstruction* taken = gen->virtuals->get_branch_target(inst);
      TargetBasicBlockAddEdge(block, fallthrough->block);
      TargetBasicBlockAddEdge(block, taken->block);
    } else if ((inst->flags & TARGET_INST_TABLE_JUMP) != 0) {
       // Table jump is followed by a branch table.  These are j instructions.
       // Find all of them and link to this block.  x86-64 plants a label at
       // the table so the computed branch can lea it; skip that one label.
       TargetInstruction* j = TargetNext(inst);
       if (j != NULL && gen->virtuals->is_label(j)) {
         j = TargetNext(j);
       }
       while (j != NULL && gen->virtuals->is_table_entry(j)) {
         TargetBasicBlockAddEdge(block, j->block);
         j = TargetNext(j);
       }
    } else if (gen->virtuals->is_return(inst)) {
      // Return always links to the exit block.
      TargetBasicBlockAddEdge(block, gen->exit_block);
    } else if (gen->virtuals->is_call(inst)) {
      // Calls only link to their next block.
      TargetInstruction* fallthrough = TargetNext(inst);
      TargetBasicBlockAddEdge(
          block, fallthrough != NULL ? fallthrough->block : gen->exit_block);
    } else {
      // Unconditional branch only links to its target.
      TargetInstruction* target = gen->virtuals->get_branch_target(inst);
      if (gen->virtuals->is_label(target)) {
        // Due to tail calls we can have a jump to a symbol.  This
        // is not an edge.  The block will have an output edge
        // to the exit block.
        TargetBasicBlockAddEdge(block, target->block);
      }
    }
  }
}

// All blocks with no output edges link to exit block.  Also blocks
// that do not end in a branch or return fall through to next block.
static void AddMissingLinks(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    if (b == gen->exit_block) {
      continue;
    }
    if (!TargetBasicBlockEndsInBranchOrReturn(gen, b)) {
      // No branch or return, fall through to next block.
      TargetBasicBlockAddEdge(b, FindTargetBasicBlock(gen, b->block_id + 1));
    }
    if (b->out_edges.length == 0) {
      TargetBasicBlockAddEdge(b, gen->exit_block);
    }
  }
}

// Link each protected region to its landing pad, the same edge the IR-level CFG
// builds (see AddExceptionHandlerEdges in codegen.c).  The pad has no branch
// into it, and the fallback below would give it the function entry as its only
// predecessor.  That makes every block downstream of the handler dominated by
// the entry alone, so a value defined inside the try and read after the handler
// joins back is no longer dominated by its own definition.  Liveness is derived
// from the dominator tree, so it stops carrying that value partway through and
// the register allocator hands its register to something else.
//
// The edge starts at the block holding the region's start label rather than at
// the individual calls inside it, so definitions made before the region still
// dominate the pad while definitions made inside it correctly do not.
//
// The region's *end* label needs the same edge.  It marks where the protected
// range stops and normally falls through from the try body, but a try body that
// returns or throws on every path leaves it with no predecessor at all.
// AddKeptBlockLinks would then hang it off the function entry, and since it
// falls through to the pad the pad would inherit that entry predecessor too --
// which is exactly the claim this function exists to avoid.
static void AddExceptionHandlerEdges(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->exception_edges.length; i++) {
    TargetExceptionEdge* edge = gen->exception_edges.value.p[i];
    TargetBasicBlock* region = edge->try_start->block;
    if (region == NULL) {
      continue;
    }
    TargetBasicBlock* targets[2] = {
        edge->catch_label->block,
        edge->try_end != NULL ? edge->try_end->block : NULL};
    for (size_t t = 0; t < sizeof(targets) / sizeof(targets[0]); t++) {
      TargetBasicBlock* to = targets[t];
      if (to == NULL || to == region) {
        continue;
      }
      // A try with several handlers, or a handler naming several types,
      // produces one range per handler over the same region; the edge is
      // wanted once.
      bool linked = false;
      for (size_t j = 0; j < region->out_edges.length && !linked; j++) {
        linked = region->out_edges.value.w[j] == (int64_t)to->block_id;
      }
      if (!linked) {
        TargetBasicBlockAddEdge(region, to);
      }
    }
  }
}

static void AddKeptBlockLinks(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    if (b == gen->entry_block || b == gen->exit_block) {
      continue;
    }
    if (b->in_edges.length == 0 && TargetBasicBlockHasKeptInstruction(b)) {
      TargetBasicBlockAddEdge(gen->entry_block, b);
    }
  }
}

static bool HasForwardSinglePredecessorCFG(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    if (block == gen->entry_block || block->in_edges.length == 0) {
      continue;
    }
    if (block->in_edges.length != 1 ||
        block->in_edges.value.w[0] >= block->block_id) {
      return false;
    }
  }
  return true;
}

static void CalculateForwardSinglePredecessorDominators(
    TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    BitSetClear(&block->dominators);
    block->idom = NULL;
    block->num_dominators = 0;
    block->reachability_known = true;
    block->is_unreachable = block != gen->entry_block;
    if (block != gen->entry_block && block->in_edges.length == 1) {
      TargetBasicBlock* predecessor =
          VectorGet(&gen->basic_blocks, block->in_edges.value.w[0]);
      if (!predecessor->is_unreachable) {
        BitSetCopy(&block->dominators, &predecessor->dominators);
        block->idom = predecessor;
        block->is_unreachable = false;
      }
    }
    if (!block->is_unreachable) {
      BitSetInsert(&block->dominators, block->block_id);
      block->num_dominators = BitSetCount(&block->dominators);
    }
  }
}

static void TargetComputeReversePostorder(TargetGenerator* gen,
                                          Vector* postorder,
                                          BitSet* reachable) {
  Vector work;
  VectorInit(&work);
  VectorAppend(&work, (void*)(gen->entry_block->block_id << 1));
  while (work.length > 0) {
    uintptr_t item = (uintptr_t)VectorLast(&work);
    VectorPop(&work);
    TargetBlockId id = (TargetBlockId)(item >> 1);
    bool expanded = (item & 1) != 0;
    if (expanded) {
      VectorAppend(postorder, (void*)id);
      continue;
    }
    if (BitSetContains(reachable, id)) {
      continue;
    }
    BitSetInsert(reachable, id);
    VectorAppend(&work, (void*)((id << 1) | 1));
    TargetBasicBlock* block = VectorGet(&gen->basic_blocks, id);
    for (size_t i = block->out_edges.length; i > 0; i--) {
      TargetBlockId successor = block->out_edges.value.w[i - 1];
      if (!BitSetContains(reachable, successor)) {
        VectorAppend(&work, (void*)(successor << 1));
      }
    }
  }
  VectorDestruct(&work);
}

static TargetBasicBlock* TargetIntersectImmediateDominators(
    TargetBasicBlock* left, TargetBasicBlock* right, TargetBasicBlock** idoms,
    const size_t* rpo_number) {
  while (left != right) {
    while (rpo_number[left->block_id] > rpo_number[right->block_id]) {
      left = idoms[left->block_id];
    }
    while (rpo_number[right->block_id] > rpo_number[left->block_id]) {
      right = idoms[right->block_id];
    }
  }
  return left;
}

// Calculate the dominators for all basic blocks.
static void CalculateDominators(TargetGenerator* gen) {
  if (HasForwardSinglePredecessorCFG(gen)) {
    CalculateForwardSinglePredecessorDominators(gen);
    return;
  }

  size_t num_blocks = gen->basic_blocks.length;
  Vector postorder;
  VectorInit(&postorder);
  BitSet reachable;
  BitSetInit(&reachable);
  TargetComputeReversePostorder(gen, &postorder, &reachable);

  size_t* rpo_number = malloc(num_blocks * sizeof(*rpo_number));
  TargetBasicBlock** idoms = calloc(num_blocks, sizeof(*idoms));
  for (size_t i = 0; i < num_blocks; i++) {
    rpo_number[i] = SIZE_MAX;
  }
  for (size_t i = 0; i < postorder.length; i++) {
    TargetBlockId id = postorder.value.w[postorder.length - i - 1];
    rpo_number[id] = i;
  }

  TargetBlockId entry_id = gen->entry_block->block_id;
  idoms[entry_id] = gen->entry_block;
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 1; i < postorder.length; i++) {
      TargetBlockId id = postorder.value.w[postorder.length - i - 1];
      TargetBasicBlock* block = VectorGet(&gen->basic_blocks, id);
      TargetBasicBlock* new_idom = NULL;
      for (size_t j = 0; j < block->in_edges.length; j++) {
        TargetBlockId predecessor_id = block->in_edges.value.w[j];
        if (idoms[predecessor_id] == NULL) {
          continue;
        }
        TargetBasicBlock* predecessor =
            VectorGet(&gen->basic_blocks, predecessor_id);
        new_idom =
            new_idom == NULL
                ? predecessor
                : TargetIntersectImmediateDominators(
                      new_idom, predecessor, idoms, rpo_number);
      }
      if (new_idom != NULL && idoms[id] != new_idom) {
        idoms[id] = new_idom;
        changed = true;
      }
    }
  }

  for (size_t i = 0; i < num_blocks; i++) {
    TargetBasicBlock* block = VectorGet(&gen->basic_blocks, i);
    BitSetClear(&block->dominators);
    block->idom = NULL;
    block->num_dominators = 0;
    block->reachability_known = true;
    block->is_unreachable = !BitSetContains(&reachable, i);
    if (block->is_unreachable) {
      continue;
    }
    block->idom = i == entry_id ? NULL : idoms[i];
    for (TargetBasicBlock* dominator = block; dominator != NULL;
         dominator = dominator->block_id == entry_id
                         ? NULL
                         : idoms[dominator->block_id]) {
      BitSetInsert(&block->dominators, dominator->block_id);
      block->num_dominators++;
    }
  }

  free(idoms);
  free(rpo_number);
  BitSetDestruct(&reachable);
  VectorDestruct(&postorder);
}


// Build dominator tree.  If a block has an
// immediate dominator (idom) add the block to the idom's
// dominatees set.
static void BuildDominatorTree(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    if (b->idom != NULL) {
      VectorAppend(&b->idom->dominatees, (void*)b->block_id);
    }
  }
}

static void SaveInstructionUses(TargetBasicBlock* block, Vector* uses) {
  for (TargetInstruction* inst = block->code; inst != NULL &&
       inst != block->end_code; inst = TargetNext(inst)) {
    VectorAppend(uses, (void*)(int64_t)inst->uses);
  }
  if (block->end_code != NULL) {
    VectorAppend(uses, (void*)(int64_t)block->end_code->uses);
  }
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* inst = block->inputs.value.p[i];
    VectorAppend(uses, (void*)(int64_t)inst->uses);
  }
}

static void RestoreInstructionUses(TargetBasicBlock* block, Vector* uses) {
  size_t index = 0;
  for (TargetInstruction* inst = block->code; inst != NULL &&
       inst != block->end_code; inst = TargetNext(inst)) {
    assert(index < uses->length);
    inst->uses = (int)uses->value.w[index++];
  }
  if (block->end_code != NULL) {
    block->end_code->uses = (int)uses->value.w[index++];
  }
  for (size_t i = 0; i < block->inputs.length; i++) {
    TargetInstruction* inst = block->inputs.value.p[i];
    // We may have added an ourput by PropagateNewOutputUpwards, in which case it's not
    // part of the saved uses counts.
    if (index >= uses->length) {
      break;
    }
    assert(index < uses->length);
    inst->uses = (int)uses->value.w[index++];
  }
}


static void ResetInstructionUses(TargetBasicBlock* block) {
  for (TargetInstruction* inst = block->code;
       inst != NULL && inst != block->end_code;
       inst = TargetNext(inst)) {
    inst->uses = (int)inst->users.length;
  }
  if (block->end_code != NULL) {
    block->end_code->uses = (int)block->end_code->users.length;
  }
}

static void DecrementOperandUses(TargetInstruction* inst) {
  if (inst == NULL) {
    return;
  }
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      // The liveness use counter (ResetInstructionUses) is the number of
      // distinct user instructions (the users list is deduplicated), so an
      // instruction that references the same value in several operand slots
      // (e.g. `cmp a, a` from `a == a`) must only decrement it once.  Skip an
      // operand that already appeared in an earlier slot, otherwise the count
      // underflows to zero early and the value is wrongly dropped from the
      // block's live sets.
      bool duplicate = false;
      for (int j = 0; j < i; j++) {
        if (inst->operand[j] == inst->operand[i]) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        continue;
      }
      if (inst->operand[i]->uses > 0) {
        inst->operand[i]->uses--;
        assert(inst->operand[i]->uses >= 0);
      }
    }
  }
}

static void AddOutput(TargetGenerator* gen, TargetBasicBlock* block,
                      TargetInstruction* inst, bool is_input,
                      bool preserve_dead_call_inputs) {
  if (inst == NULL) {
    return;
  }
  // A call provides a fixed register but we need to propagate it
  // as output if its value is not used in this block.
  if (!gen->virtuals->is_call(inst)) {
    if (gen->virtuals->is_fixed_register(inst) || gen->virtuals->is_const(inst)
        || gen->virtuals->is_symbol(inst) || !gen->virtuals->is_expression(inst)) {
      return;
    }
  }
  // On CFGs with joins or backedges, retain the conservative rule that every
  // input to a call block is also an output.  On a dominator-tree-only CFG the
  // use count is path-exact, so dropping a dead input avoids carrying an
  // ever-growing live set through a long sequence of calls.
  if (!(preserve_dead_call_inputs && is_input && block->contains_call)) {
    assert(inst->uses >= 0);
    if (inst->uses == 0) {
      return;
    }
  }

  if (!BitSetContains(&block->output_ids, inst->id)) {
    VectorAppend(&block->outputs, inst);
    BitSetInsert(&block->output_ids, inst->id);
  }
}

static void PropagateNewOutputUpwards(TargetGenerator* gen,
                                      TargetBasicBlock* block,
                                      TargetInstruction* inst) {
  
  if (!BitSetContains(&block->output_ids, inst->id)) {
    VectorAppend(&block->outputs, inst);
    BitSetInsert(&block->output_ids, inst->id);
    
    // If this instruction is not in our input set we need to add it
    // there too.
    if (!BitSetContains(&block->input_ids, inst->id)) {
      VectorAppend(&block->inputs, inst);
      BitSetInsert(&block->input_ids, inst->id);
    }
    if (block->idom != NULL) {
      PropagateNewOutputUpwards(gen, block->idom, inst);
    }
  }
}

static void BuildBlockOutputs(TargetGenerator* gen, TargetBasicBlock* block,
                              bool preserve_dead_call_inputs) {
  // For each instruction, decrement the uses count for all its operands.
  for (TargetInstruction* inst = block->code; inst != NULL &&
       inst != block->end_code; inst = TargetNext(inst)) {
    DecrementOperandUses(inst);
  }
  DecrementOperandUses(block->end_code);

  VectorClear(&block->outputs);
  BitSetClear(&block->output_ids);
  
  // Now look for all instructions that have a uses count > 0.  Also
  // look at the inputs.  If we don't use an input it becomes an output.
  // Also, if the block is a call block we propagate all inputs
  // to outputs
  for (TargetInstruction* inst = block->code; inst != NULL &&
       inst != block->end_code; inst = TargetNext(inst)) {
    AddOutput(gen, block, inst, false, preserve_dead_call_inputs);
  }
  AddOutput(gen, block, block->end_code, false, preserve_dead_call_inputs);
  for (size_t i = 0; i < block->inputs.length; i++) {
    AddOutput(gen, block, block->inputs.value.p[i], true,
              preserve_dead_call_inputs);
  }
  
  // Now look at the block's out edges and add the inputs of those to
  // the output set of this block.  If a block can be reached by this one
  // then its inputs are part of our outputs.  Blocks dominated by this one
  // are already processed.
  for (size_t i = 0; i < block->out_edges.length; i++) {
    TargetBasicBlock* out = gen->basic_blocks.value.p[block->out_edges.value.w[i]];
    for (size_t j = 0; j < out->inputs.length; j++) {
      TargetInstruction* inst = out->inputs.value.p[j];
      PropagateNewOutputUpwards(gen, block, inst);
    }
  }
}

// Inputs to this block the outputs from the immediate dominator.
static void BuildBlockInputs(TargetBasicBlock* block) {
  if (block->idom != NULL) {
    VectorCopy(&block->inputs, &block->idom->outputs);
    for (size_t i = 0; i < block->inputs.length; i++) {
      TargetInstruction* inst = block->inputs.value.p[i];
      BitSetInsert(&block->input_ids, inst->id);
    }
  }
}

// Traverse the dominator tree building the inputs and outputs.
// Returns true if there are any changes.
static void BuildInputsAndOutputs(TargetGenerator* gen, TargetBasicBlock* block,
                                  bool preserve_dead_call_inputs) {
  while (block != NULL) {
    BuildBlockInputs(block);
    BuildBlockOutputs(gen, block, preserve_dead_call_inputs);

    // A single-child dominator path needs neither recursion nor a uses
    // snapshot: no sibling observes the mutations made by the child subtree.
    if (block->dominatees.length == 1) {
      TargetBlockId child_id = block->dominatees.value.w[0];
      block = gen->basic_blocks.value.p[child_id];
      continue;
    }

    if (block->dominatees.length > 1) {
      Vector saved_uses = {0};
      SaveInstructionUses(block, &saved_uses);
      for (size_t i = 0; i < block->dominatees.length; i++) {
        TargetBlockId child_id = block->dominatees.value.w[i];
        TargetBasicBlock* child = gen->basic_blocks.value.p[child_id];
        BuildInputsAndOutputs(gen, child, preserve_dead_call_inputs);
        RestoreInstructionUses(block, &saved_uses);
      }
      VectorDestruct(&saved_uses);
    }
    return;
  }
}


static void ResetAllInstructionUses(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    ResetInstructionUses(b);
  }
}

// Liveness is built once when the CFG is created and again after the target
// optimizer has rewritten instructions, so the second run has to start from
// nothing.  Clearing each block as the dominator tree walk reaches it is not
// enough: a block reads the live-in set of its successors, which the walk in
// general has not visited yet, and a block outside the dominator tree is never
// visited at all.  Either way the earlier run's set would be read as if it
// described the rewritten code, which spreads values whose defining
// instruction the optimizer has already deleted.
static void ResetAllBlockLiveness(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    VectorClear(&b->inputs);
    VectorClear(&b->outputs);
    BitSetClear(&b->input_ids);
    BitSetClear(&b->output_ids);
  }
}

static bool TargetValueNeedsLiveness(TargetGenerator* gen,
                                     TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  if (gen->virtuals->is_call(inst)) {
    return true;
  }
  return !gen->virtuals->is_fixed_register(inst) &&
         !gen->virtuals->is_const(inst) &&
         !gen->virtuals->is_symbol(inst) &&
         gen->virtuals->is_expression(inst);
}

static void BuildDataflowBlockUseAndDef(TargetGenerator* gen,
                                        TargetBasicBlock* block,
                                        BitSet* use, BitSet* def) {
  for (TargetInstruction* inst = block->code; inst != NULL;
       inst = inst == block->end_code ? NULL : TargetNext(inst)) {
    for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (TargetValueNeedsLiveness(gen, operand) &&
          !BitSetContains(def, operand->id)) {
        BitSetInsert(use, operand->id);
      }
    }
    if (TargetValueNeedsLiveness(gen, inst)) {
      BitSetInsert(def, inst->id);
    }
  }
}

static void BuildDataflowLiveIn(BitSet* use, BitSet* def, BitSet* live_out,
                                BitSet* live_in) {
  BitSetCopy(live_in, use);
  BitSetIterator it;
  BitSetIteratorStart(&it, live_out);
  while (!BitSetIteratorDone(&it)) {
    size_t id = BitSetIteratorValue(&it);
    if (!BitSetContains(def, id)) {
      BitSetInsert(live_in, id);
    }
    BitSetIteratorNext(&it);
  }
}

static void AppendLiveValues(Vector* values, BitSet* ids,
                             TargetInstruction** values_by_id,
                             size_t values_by_id_count) {
  BitSetIterator it;
  BitSetIteratorStart(&it, ids);
  while (!BitSetIteratorDone(&it)) {
    size_t id = BitSetIteratorValue(&it);
    assert(id < values_by_id_count);
    TargetInstruction* inst = values_by_id[id];
    assert(inst != NULL);
    VectorAppend(values, inst);
    BitSetIteratorNext(&it);
  }
}

static void TargetBuildDataflowLiveness(TargetGenerator* gen) {
  size_t block_count = gen->basic_blocks.length;
  BitSet* use = calloc(block_count, sizeof(*use));
  BitSet* def = calloc(block_count, sizeof(*def));
  BitSet* live_in = calloc(block_count, sizeof(*live_in));
  BitSet* live_out = calloc(block_count, sizeof(*live_out));
  for (size_t i = 0; i < block_count; i++) {
    BitSetInit(&use[i]);
    BitSetInit(&def[i]);
    BitSetInit(&live_in[i]);
    BitSetInit(&live_out[i]);
    BuildDataflowBlockUseAndDef(
        gen, gen->basic_blocks.value.p[i], &use[i], &def[i]);
  }

  BitSet new_out;
  BitSet new_in;
  BitSetInit(&new_out);
  BitSetInit(&new_in);
  bool changed;
  do {
    changed = false;
    for (size_t i = block_count; i > 0; i--) {
      TargetBasicBlock* block = gen->basic_blocks.value.p[i - 1];
      BitSetClear(&new_out);
      for (size_t j = 0; j < block->out_edges.length; j++) {
        TargetBlockId successor = block->out_edges.value.w[j];
        BitSetUnionInPlace(&new_out, &live_in[successor]);
      }
      BuildDataflowLiveIn(&use[i - 1], &def[i - 1], &new_out, &new_in);
      if (!BitSetEqual(&live_out[i - 1], &new_out) ||
          !BitSetEqual(&live_in[i - 1], &new_in)) {
        BitSetCopy(&live_out[i - 1], &new_out);
        BitSetCopy(&live_in[i - 1], &new_in);
        changed = true;
      }
    }
  } while (changed);
  BitSetDestruct(&new_in);
  BitSetDestruct(&new_out);

  size_t max_id = 0;
  for (size_t i = 0; i < block_count; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = inst == block->end_code ? NULL : TargetNext(inst)) {
      if (TargetValueNeedsLiveness(gen, inst) && (size_t)inst->id > max_id) {
        max_id = inst->id;
      }
      for (size_t j = 0; j < TARGET_MAX_OPERANDS; j++) {
        TargetInstruction* operand = inst->operand[j];
        if (TargetValueNeedsLiveness(gen, operand) &&
            (size_t)operand->id > max_id) {
          max_id = operand->id;
        }
      }
    }
  }
  size_t values_by_id_count = max_id + 1;
  TargetInstruction** values_by_id =
      calloc(values_by_id_count, sizeof(*values_by_id));
  for (size_t i = 0; i < block_count; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = inst == block->end_code ? NULL : TargetNext(inst)) {
      if (TargetValueNeedsLiveness(gen, inst)) {
        values_by_id[inst->id] = inst;
      }
      for (size_t j = 0; j < TARGET_MAX_OPERANDS; j++) {
        TargetInstruction* operand = inst->operand[j];
        if (TargetValueNeedsLiveness(gen, operand)) {
          values_by_id[operand->id] = operand;
        }
      }
    }
  }
  for (size_t i = 0; i < block_count; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    BitSetCopy(&block->input_ids, &live_in[i]);
    BitSetCopy(&block->output_ids, &live_out[i]);
    AppendLiveValues(&block->inputs, &block->input_ids, values_by_id,
                     values_by_id_count);
    AppendLiveValues(&block->outputs, &block->output_ids, values_by_id,
                     values_by_id_count);
    BitSetDestruct(&live_out[i]);
    BitSetDestruct(&live_in[i]);
    BitSetDestruct(&def[i]);
    BitSetDestruct(&use[i]);
  }
  free(values_by_id);
  free(live_out);
  free(live_in);
  free(def);
  free(use);
}

// An unreachable block may still hold a label the exception tables name, such
// as the bound of a try range that turned out to be dead.  Keep those labels so
// the tables still resolve and drop the rest of the block: the code is dead,
// and the passes that follow walk the dominator tree, which an unreachable
// block is not part of, so anything left here never gets a register assigned
// even though the emitter would still emit it.
static void ClearUnreachableBlockExceptKeptLabels(TargetGenerator* gen,
                                                  TargetBasicBlock* b) {
  if (b->code == NULL) {
    return;
  }
  TargetInstruction* first = NULL;
  TargetInstruction* last = NULL;
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = b->code; inst != NULL; inst = next) {
    next = inst == b->end_code ? NULL : TargetNext(inst);
    if ((inst->flags & TARGET_INST_KEEP_UNREACHABLE) != 0) {
      if (first == NULL) {
        first = inst;
      }
      last = inst;
      continue;
    }
    TargetDeleteInstruction(gen, inst);
  }
  b->code = first;
  b->end_code = last;
}

static void RemoveUnreachableBlocks(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    if (!TargetBasicBlockIsUnreachable(gen, b)) {
      continue;
    }
    if (TargetBasicBlockHasKeptInstruction(b)) {
      ClearUnreachableBlockExceptKeptLabels(gen, b);
    } else {
      TargetBasicBlockClear(gen, b);
    }
  }
}

// Detect natural loops from latch -> header back edges.  Build the union of
// every latch belonging to one header before incrementing loop_nesting so
// multiple latches do not look like nested loops.
static void DetectLoops(TargetGenerator* gen) {
  for (size_t header_id = 0; header_id < gen->basic_blocks.length;
       header_id++) {
    TargetBasicBlock* header = gen->basic_blocks.value.p[header_id];
    Vector work;
    VectorInit(&work);
    for (size_t i = 0; i < header->in_edges.length; i++) {
      TargetBasicBlock* latch =
          gen->basic_blocks.value.p[header->in_edges.value.w[i]];
      if (BitSetContains(&latch->dominators, header_id)) {
        VectorAppend(&work, latch);
      }
    }

    if (work.length == 0) {
      VectorDestruct(&work);
      continue;
    }

    BitSet loop_blocks;
    BitSetInit(&loop_blocks);
    BitSetInsert(&loop_blocks, header_id);
    while (work.length > 0) {
      TargetBasicBlock* block = VectorLast(&work);
      VectorPop(&work);
      if (BitSetContains(&loop_blocks, block->block_id)) {
        continue;
      }
      BitSetInsert(&loop_blocks, block->block_id);
      for (size_t i = 0; i < block->in_edges.length; i++) {
        TargetBasicBlock* predecessor =
            VectorGet(&gen->basic_blocks, block->in_edges.value.w[i]);
        VectorAppend(&work, predecessor);
      }
    }
    VectorDestruct(&work);

    BitSetIterator it;
    BitSetIteratorStart(&it, &loop_blocks);
    while (!BitSetIteratorDone(&it)) {
      TargetBasicBlock* block = VectorGet(
          &gen->basic_blocks, (TargetBlockId)BitSetIteratorValue(&it));
      block->loop_nesting++;
      BitSetIteratorNext(&it);
    }
    BitSetDestruct(&loop_blocks);
  }
}

void TargetBuildBasicBlocks(TargetGenerator* gen) {
  // The branches vector holds a list of all the branches we encounter in the
  // code.  Each branch adds an edge from its block to the block starting with
  // the label to which it is branching.
  Vector branches;
  VectorInit(&branches);

  // Phase 1: create all basic blocks.
  CreateTargetBasicBlocks(gen, &branches);
  
  // Phase 2: Link the blocks into a graph.
  BuildTargetBasicBlockGraph(gen, &branches);

  // Phase 3: all blocks with no output edges link to exit block.  Also blocks
  // that do not end in a branch or return fall through to next block.
  AddMissingLinks(gen);
  AddExceptionHandlerEdges(gen);
  AddKeptBlockLinks(gen);
  
  // RVPrintBasicBlocks(gen, stdout);
  
  // Phase 4: calculate dominators, dominance frontier and idom.
  // Phase 4a: dominators.
  CalculateDominators(gen);
 
  // TargetPrintBasicBlocks(gen, stdout);
  
  // Phase 4b: dominance frontier.
  //CalculateDominanceFrontier(gen);
  
  // Phase 5: build dominator tree.
  BuildDominatorTree(gen);
  
  // RVPrintBasicBlocks(gen, stdout);
  
  // Detect loops.
  DetectLoops(gen);
  
  // Tidy up.
  // Delete the branches vector.
  VectorDestruct(&branches);
  
  RemoveUnreachableBlocks(gen);
  
  TargetBuildBasicBlockInputsAndOutputs(gen);
}

// The dominator tree walk records a value as live in a block only if the block
// is on the dominator chain between the definition and the use.  A value that
// is live on entry to a join block is however also live along every other CFG
// path that reaches the join, and those paths need not dominate it (the sibling
// arm of an if/else is the common case).  The register allocator reserves a
// register only for the values a block records as live, so a gap in the middle
// of a live range lets an unrelated value claim the same register and clobber
// it.  Close the gaps by propagating each block's live-in set backwards over
// the CFG edges until the defining block is reached.
static void PropagateLiveInToPredecessors(TargetGenerator* gen) {
  Vector work;
  VectorInit(&work);
  size_t* propagated_inputs =
      calloc(gen->basic_blocks.length, sizeof(*propagated_inputs));
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    VectorAppend(&work, gen->basic_blocks.value.p[i]);
  }
  while (work.length > 0) {
    TargetBasicBlock* block = VectorLast(&work);
    VectorPop(&work);
    // Requeued blocks normally only need to propagate the inputs appended
    // since their previous visit.  A self-edge can grow this same vector while
    // it is being scanned, so retain the original full-rescan behavior there.
    bool has_self_edge = false;
    for (size_t i = 0; i < block->in_edges.length; i++) {
      if (block->in_edges.value.w[i] == block->block_id) {
        has_self_edge = true;
        break;
      }
    }
    size_t first_input =
        has_self_edge ? 0 : propagated_inputs[block->block_id];
    size_t input_count = block->inputs.length;
    for (size_t i = 0; i < block->in_edges.length; i++) {
      TargetBasicBlock* pred =
          VectorGet(&gen->basic_blocks, block->in_edges.value.w[i]);
      bool changed = false;
      for (size_t j = first_input; j < input_count; j++) {
        TargetInstruction* inst = block->inputs.value.p[j];
        if (inst->block == NULL || inst->block == block) {
          continue;
        }
        // Only values that are already defined when the predecessor runs are
        // live there.  The upward propagation also records values in blocks
        // that precede their definition; those must not spread any further.
        if (inst->block != pred &&
            !BitSetContains(&pred->dominators, inst->block->block_id)) {
          continue;
        }
        if (!BitSetContains(&pred->output_ids, inst->id)) {
          VectorAppend(&pred->outputs, inst);
          BitSetInsert(&pred->output_ids, inst->id);
          changed = true;
        }
        if (inst->block != pred &&
            !BitSetContains(&pred->input_ids, inst->id)) {
          VectorAppend(&pred->inputs, inst);
          BitSetInsert(&pred->input_ids, inst->id);
          changed = true;
        }
      }
      if (changed) {
        VectorAppend(&work, pred);
      }
    }
    if (!has_self_edge) {
      propagated_inputs[block->block_id] = input_count;
    }
  }
  free(propagated_inputs);
  VectorDestruct(&work);
}

static bool HasNonDominatorTreeEdge(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    if (block->is_unreachable) {
      continue;
    }
    for (size_t j = 0; j < block->out_edges.length; j++) {
      TargetBasicBlock* successor =
          VectorGet(&gen->basic_blocks, block->out_edges.value.w[j]);
      if (successor->is_unreachable) {
        continue;
      }
      if (successor->idom != block) {
        return true;
      }
    }
  }
  return false;
}

// Calculate the inputs and outputs for all basic blocks.  This
// information tells the register allocator the lifespan of
// registers.
void TargetBuildBasicBlockInputsAndOutputs(TargetGenerator* gen) {
  bool has_non_dominator_tree_edge = HasNonDominatorTreeEdge(gen);
  ResetAllBlockLiveness(gen);
  ResetAllInstructionUses(gen);
  // Dataflow liveness is the only walk that models loop-carried values
  // through back edges.  The uses-count walk under-counts a value that is
  // read once in a loop header, so the allocator reuses its register in the
  // body and the next iteration loads through a stale pointer (observed in
  // path::lexically_normal at -O2).  Exception edges are already linked into
  // out_edges by AddExceptionHandlerEdges, so they do not need a separate
  // fallback.
  if (gen->virtuals->calls_may_stay_in_block) {
    TargetBuildDataflowLiveness(gen);
  } else {
    BuildInputsAndOutputs(gen, gen->entry_block,
                         has_non_dominator_tree_edge);
    if (has_non_dominator_tree_edge) {
      PropagateLiveInToPredecessors(gen);
    }
  }

  // Reset the uses count for all instructions.
  ResetAllInstructionUses(gen);
}

static void MarkPreservedValue(BitSet* preserved, TargetInstruction* value) {
  if (value == NULL) {
    return;
  }
  BitSetInsert(preserved, value->id);
  if (value->dest != NULL) {
    BitSetInsert(preserved, value->dest->id);
  }
}

void TargetMarkCallPreservedInstructions(TargetGenerator* gen,
                                         BitSet* preserved) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    if (!block->contains_call || block->code == NULL) {
      continue;
    }

    int call_epoch = 0;
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = inst == block->end_code ? NULL : TargetNext(inst)) {
      for (size_t operand_index = 0; operand_index < TARGET_MAX_OPERANDS;
           operand_index++) {
        TargetInstruction* operand = inst->operand[operand_index];
        if (operand == NULL) {
          continue;
        }
        int definition_epoch =
            operand->block == block ? operand->call_epoch : 0;
        if (definition_epoch < call_epoch) {
          MarkPreservedValue(preserved, operand);
        }
      }

      if (gen->virtuals->is_call(inst)) {
        call_epoch++;
      }
      inst->call_epoch = call_epoch;
    }

    for (size_t output_index = 0; output_index < block->outputs.length;
         output_index++) {
      TargetInstruction* output = block->outputs.value.p[output_index];
      int definition_epoch =
          output->block == block ? output->call_epoch : 0;
      if (definition_epoch < call_epoch) {
        MarkPreservedValue(preserved, output);
      }
    }
  }
}

static bool KeepMaterializedCallResult(TargetInstruction* user, void* data) {
  return user != data;
}

bool TargetMaterializePreservedCallResults(TargetGenerator* gen,
                                           BitSet* preserved,
                                           TargetCreateCallResultCopyFunc
                                               create_copy) {
  assert(create_copy != NULL);
  bool changed = false;
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;) {
    TargetInstruction* next = TargetNext(inst);
    // A call result that is live across a later call must leave x0/d0
    // (or the equivalent return register) before that later call runs.
    // Exception edges force each call into its own block, so a single
    // user in a later block is the common case (mark_count() used after
    // constructing a local), not a reason to skip the copy.
    if (gen->virtuals->is_call(inst) &&
        BitSetContains(preserved, inst->id)) {
      TargetInstruction* copy = create_copy(inst);
      assert(copy != NULL);
      if (inst == inst->block->end_code && next != NULL &&
          next->block != inst->block) {
        TargetBasicBlockEmitBefore(gen, next->block, copy, next);
      } else {
        TargetBasicBlockEmitAfter(gen, inst->block, copy, inst);
      }
      TargetRetargetInstructionIf(inst, copy, KeepMaterializedCallResult, copy);
      changed = true;
      if (copy->block == inst->block) {
        next = TargetNext(copy);
      }
    }
    inst = next;
  }
  return changed;
}

void TargetPrintBasicBlocks(TargetGenerator* gen, FILE* fp) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    TargetBasicBlockPrint(gen, b, gen->entry_block, gen->exit_block, fp);
  }
}

static void TraverseDomTree(TargetGenerator* gen,
                            TargetBasicBlock* block, TargetTraversalFunc func,
                            TraversalMode mode, void* data) {
  if (block == NULL) {
    return;
  }
  if (mode == kTraversePreOrder) {
    func(block, data);
  }
  for (size_t i = 0; i < block->dominatees.length; i++) {
    TargetBlockId child_id = block->dominatees.value.w[i];
    TargetBasicBlock* child = gen->basic_blocks.value.p[child_id];
    if (child == block) {
      continue;
    }
    TraverseDomTree(gen, child, func, mode, data);
  }
  if (mode == kTraversePostOrder) {
    func(block, data);
  }
}

void TargetBasicBlockTraverseDominatorTree(TargetGenerator* gen, TargetBasicBlock* block,
                                       TargetTraversalFunc func,
                             TraversalMode mode,
                             void* data) {
  TraverseDomTree(gen, block, func, mode, data);
}

void TargetTraverseDominatorTree(TargetGenerator* gen, TargetTraversalFunc func,
                             TraversalMode mode,
                             void* data) {
  TraverseDomTree(gen, gen->entry_block, func, mode, data);
}

bool TargetBasicBlockOutputs(TargetBasicBlock* block, TargetInstruction* inst) {
  return BitSetContains(&block->output_ids, inst->id);
}

void TargetBasicBlockPropagateExpression(TargetGenerator* gen, TargetInstruction* inst, TargetBasicBlock* to) {
  TargetBasicBlock* from = inst->block;
  AddOutput(gen, from, inst, false, false);
  // Mark the value as input to the using block.
  if (!BitSetContains(&to->input_ids, inst->id)) {
    VectorAppend(&to->inputs, inst);
    BitSetInsert(&to->input_ids, inst->id);
  }
  // `from` dominates `to`, so the value is live along the dominator path
  // between them.  Mark every intermediate block as having the value both
  // live-in and live-out; otherwise the register allocator (which reserves
  // registers only for each block's recorded inputs) treats the value's
  // register as free in those blocks and reuses it -- clobbering a pooled
  // constant that is defined in `from` and consumed later in `to`.
  for (TargetBasicBlock* b = to->idom; b != NULL && b != from; b = b->idom) {
    if (!BitSetContains(&b->output_ids, inst->id)) {
      VectorAppend(&b->outputs, inst);
      BitSetInsert(&b->output_ids, inst->id);
    }
    if (!BitSetContains(&b->input_ids, inst->id)) {
      VectorAppend(&b->inputs, inst);
      BitSetInsert(&b->input_ids, inst->id);
    }
  }
}

TargetInstruction* TargetBasicBlockBegin(TargetBasicBlock* b) {
  return b->code;
}

TargetInstruction* TargetBasicBlockEnd(TargetBasicBlock* b) {
  if (b->end_code == NULL) {
    return NULL;
  }
  return (TargetInstruction*)b->end_code->header.next;
}

TargetInstruction* TargetBasicBlockRBegin(TargetBasicBlock* b) {
  return b->end_code;

}

TargetInstruction* TargetBasicBlockREnd(TargetBasicBlock* b) {
  if (b->code == NULL) {
    return NULL;
  }
  return (TargetInstruction*)b->code->header.prev;
}

bool TargetBasicBlockIsEmpty(TargetBasicBlock* b) {
  return b->code == NULL || b->end_code == NULL;
}

void TargetBasicBlockReachableAfter(TargetGenerator* gen,
                                    TargetBasicBlock* block,
                                    BitSet* reachable) {
  BitSetClear(reachable);
  if (block == NULL) {
    return;
  }
  Vector work;
  VectorInit(&work);
  for (size_t i = 0; i < block->out_edges.length; i++) {
    VectorAppend(&work, VectorGet(&gen->basic_blocks, block->out_edges.value.w[i]));
  }
  while (work.length > 0) {
    TargetBasicBlock* b = work.value.p[--work.length];
    if (b == NULL || BitSetContains(reachable, b->block_id)) {
      continue;
    }
    BitSetInsert(reachable, b->block_id);
    for (size_t i = 0; i < b->out_edges.length; i++) {
      VectorAppend(&work, VectorGet(&gen->basic_blocks, b->out_edges.value.w[i]));
    }
  }
  VectorDestruct(&work);
}

bool TargetBasicBlockDominatedBy(TargetGenerator* gen, TargetBasicBlock* dom, TargetBasicBlock* b) {
  for (size_t i = 0; i < dom->dominatees.length; i++) {
    TargetBlockId child_id = dom->dominatees.value.w[i];
    TargetBasicBlock* child = gen->basic_blocks.value.p[child_id];
    if (b == child) {
      return true;
    }
    if (TargetBasicBlockDominatedBy(gen, child, b)) {
      return true;
    }
  }
  return false;
}
