//
//  risc_v_basic_block.c
//  c_compiler_library
//
//  Created by David Allison on 5/27/20.
//  Copyright © 2020 David Allison. All rights resegened.
//

#include "target_basic_block.h"
#include "compiler.h"
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
    for (size_t i = 0; i < num_nodes; ++i) {
      BitSetInsert(&b->dominators, i);
    }
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
    if (gen->virtuals->is_label(inst)) {
      current->end_code = TargetPrev(inst);
      
      // A label marks the start of a block.
      TargetBasicBlock* b = TargetGeneratorNewTargetBasicBlock(gen);
      inst->block = b;
      b->code = inst;
      current = b;
    } else if (gen->virtuals->is_branch(inst) ||
               gen->virtuals->is_return(inst) ||
               gen->virtuals->is_call(inst)) {
      // Branch, return and call ends a block.
      current->end_code = inst;
      inst->block = current;
      VectorAppend(branches, inst);
      current->contains_call = gen->virtuals->is_call(inst);

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
       // Find all of them and link to this block.
       TargetInstruction* j = TargetNext(inst);
       while (gen->virtuals->is_table_entry(j)) {
         TargetBasicBlockAddEdge(block, j->block);
         j = TargetNext(j);
       }
    } else if (gen->virtuals->is_return(inst)) {
      // Return always links to the exit block.
      TargetBasicBlockAddEdge(block, gen->exit_block);
    } else if (gen->virtuals->is_call(inst)) {
      // Calls only link to their next block.
      TargetInstruction* fallthrough = TargetNext(inst);
      TargetBasicBlockAddEdge(block, fallthrough->block);
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
static void AddExceptionHandlerEdges(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->exception_edges.length; i++) {
    TargetExceptionEdge* edge = gen->exception_edges.value.p[i];
    TargetBasicBlock* region = edge->try_start->block;
    TargetBasicBlock* pad = edge->catch_label->block;
    if (region == NULL || pad == NULL || region == pad) {
      continue;
    }
    // A try with several handlers, or a handler naming several types, produces
    // one range per handler over the same region; the edge is wanted once.
    bool linked = false;
    for (size_t j = 0; j < region->out_edges.length && !linked; j++) {
      linked = region->out_edges.value.w[j] == (int64_t)pad->block_id;
    }
    if (!linked) {
      TargetBasicBlockAddEdge(region, pad);
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

// Calculate the dominators for all basic blocks.
static void CalculateDominators(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    TargetBasicBlockInitDominators(b, b == gen->entry_block,
                             gen->basic_blocks.length);
  }
  
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 0; i < gen->basic_blocks.length; i++) {
      TargetBasicBlock* b = gen->basic_blocks.value.p[i];
      changed |= TargetBasicBlockCalculateDominators(gen, b, &gen->basic_blocks);
    }
  }
  
  // Now check for isolated islands where the blocks form a loop
  // that cannot be accessed from outside the loop.  This is denoted
  // by the dominators of the block being all the blocks.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    if (BitSetCount(&b->dominators) == gen->basic_blocks.length) {
      BitSetClear(&b->dominators);
      b->is_unreachable = true;
      b->reachability_known = true;
    }
  }
}

static void CalculateImmediateDominator(TargetGenerator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[i];
    TargetBasicBlockCalculateImmediateDominator(b, &gen->basic_blocks);
  }
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

static void AddOutput(TargetGenerator* gen, TargetBasicBlock* block, TargetInstruction* inst, bool is_input) {
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
  // Unless this is an input inside a call block, check for
  // use count.  This allows all inputs to be propagated to the
  // output in a call block.
  if (!(is_input && block->contains_call)) {
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

static void BuildBlockOutputs(TargetGenerator* gen, TargetBasicBlock* block) {
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
    AddOutput(gen, block, inst, false);
  }
  AddOutput(gen, block, block->end_code, false);
  for (size_t i = 0; i < block->inputs.length; i++) {
    AddOutput(gen, block, block->inputs.value.p[i], true);
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
static void BuildInputsAndOutputs(TargetGenerator* gen, TargetBasicBlock* block) {
  if (block == NULL) {
    return;
  }
  BuildBlockInputs(block);
  BuildBlockOutputs(gen, block);
  Vector saved_uses = {0};
  SaveInstructionUses(block, &saved_uses);
  for (size_t i = 0; i < block->dominatees.length; i++) {
    TargetBlockId child_id = block->dominatees.value.w[i];
    TargetBasicBlock* child = gen->basic_blocks.value.p[child_id];
    BuildInputsAndOutputs(gen, child);
    RestoreInstructionUses(block, &saved_uses);
  }
  VectorDestruct(&saved_uses);
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
    Vector work;
    VectorInit(&work);
    for (size_t i = 0; i < gen->basic_blocks.length; i++) {
      TargetBasicBlock* latch = gen->basic_blocks.value.p[i];
      for (size_t j = 0; j < latch->out_edges.length; j++) {
        TargetBlockId out_id = latch->out_edges.value.w[j];
        if (out_id == header_id &&
            BitSetContains(&latch->dominators, header_id)) {
          VectorAppend(&work, latch);
        }
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
  
  // Phase 4b: immediate dominator.
  CalculateImmediateDominator(gen);
 
  // Phase 4c: dominance frontier.
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
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    VectorAppend(&work, gen->basic_blocks.value.p[i]);
  }
  while (work.length > 0) {
    TargetBasicBlock* block = VectorLast(&work);
    VectorPop(&work);
    for (size_t i = 0; i < block->in_edges.length; i++) {
      TargetBasicBlock* pred =
          VectorGet(&gen->basic_blocks, block->in_edges.value.w[i]);
      bool changed = false;
      for (size_t j = 0; j < block->inputs.length; j++) {
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
  }
  VectorDestruct(&work);
}

// Calculate the inputs and outputs for all basic blocks.  This
// information tells the register allocator the lifespan of
// registers.
void TargetBuildBasicBlockInputsAndOutputs(TargetGenerator* gen) {
   ResetAllBlockLiveness(gen);
   ResetAllInstructionUses(gen);
   BuildInputsAndOutputs(gen, gen->entry_block);
   PropagateLiveInToPredecessors(gen);

   // Reset the uses count for all instructions.
   ResetAllInstructionUses(gen);
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
  AddOutput(gen, from, inst, false);
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
