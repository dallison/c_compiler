//
//  basic_block.c
//  c_compiler
//
//  Created by David Allison on 12/12/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "basic_block.h"
#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"
#include "ir.h"
#include "set.h"
#include "symbol.h"

// Compare variable defintions.  The pointers are Symbol**.  Order by address,
// but never by a truncated difference: two symbols far apart in memory can have
// a difference whose low 32 bits are zero (or of the wrong sign), which would
// make the map treat distinct symbols as equal and break its binary search.
static int CompareVariable(const void* a, const void* b) {
  uintptr_t s1 = (uintptr_t)*(Symbol**)a;
  uintptr_t s2 = (uintptr_t)*(Symbol**)b;

  if (s1 == s2) {
    return 0;
  }
  return s1 < s2 ? -1 : 1;
}

BasicBlock* NewBasicBlock(BlockId id) {
  BasicBlock* b = malloc(sizeof(BasicBlock));
  b->block_id = id;
  b->code = NULL;
  b->end_code = NULL;
  VectorInit(&b->in_edges);
  VectorInit(&b->out_edges);
  BitSetInit(&b->dominators);
  b->num_dominators = 0;
  VectorInit(&b->dominatees);
  BitSetInit(&b->dominance_frontier);
  MapInit(&b->defined_vars, CompareVariable);
  SetInitForPointers(&b->referenced_vars);
  b->idom = NULL;
  BitSetInit(&b->in_back_edges);
  b->optimizer_data = NULL;
  b->num_calls = 0;
  b->return_block = false;
  b->reachability_known = false;
  b->is_unreachable = false;
  b->innermost_loop = NULL;
  b->loop_nesting = 0;
  return b;
}

void BasicBlockDelete(BasicBlock* b) {
  // NOTE: the IR nodes are not owned by the BasicBlock.  They belong to the
  // Generator and that is responsible for deleting them.
  VectorDestruct(&b->in_edges);
  VectorDestruct(&b->out_edges);
  BitSetDestruct(&b->dominators);
  BitSetDestruct(&b->dominance_frontier);
  BitSetDestruct(&b->in_back_edges);
  VectorDestruct(&b->dominatees);
  MapDestruct(&b->defined_vars);
  SetDestruct(&b->referenced_vars);
  free(b);
}

void BasicBlockAddInEdge(BasicBlock* from, BasicBlock* to) {
  VectorAppend(&from->in_edges, (void*)to->block_id);
  to->reachability_known = false;
}

void BasicBlockAddEdge(BasicBlock* from, BasicBlock* to) {
  VectorAppend(&from->out_edges, (void*)to->block_id);
  BasicBlockAddInEdge(to, from);
  to->reachability_known = false;
}

void BasicBlockAddBackEdge(BasicBlock* from, BasicBlock* to) {
  BitSetInsert(&to->in_back_edges, from->block_id);
}

bool BasicBlockCalculateDominators(Generator* gen, BasicBlock* b, Vector* blocks) {
  BitSet dominators;
  BitSetInit(&dominators);
  BitSetCopy(&dominators, &b->dominators);

  BitSet intersection;
  BitSetInit(&intersection);

  int num_intersections = 0;
  // Calculate intersection.
  for (size_t i = 0; i < b->in_edges.length; i++) {
    BlockId id = (BlockId)b->in_edges.value.p[i];
    BasicBlock* dom_block = blocks->value.p[id];

    if (BasicBlockIsUnreachable(gen, dom_block)) {
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

void BasicBlockCalculateImmediateDominator(BasicBlock* b, Vector* blocks) {
  if (b->in_edges.length == 1) {
    BasicBlock* predecessor =
        VectorGet(blocks, (BlockId)b->in_edges.value.p[0]);
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
    BlockId id = BitSetIteratorValue(&it);
    if (id != b->block_id) {
       BasicBlock* block = VectorGet(blocks, id);
       if (block->num_dominators > maxndoms) {
         maxndoms = block->num_dominators;
         b->idom = block;
       }
     }
    BitSetIteratorNext(&it);
  }
}

bool BasicBlockDominatedBy(Generator* gen, BasicBlock* dom, BasicBlock* b) {
  for (size_t i = 0; i < dom->dominatees.length; i++) {
    BlockId child_id = dom->dominatees.value.w[i];
    BasicBlock* child = gen->basic_blocks.value.p[child_id];
    if (b == child) {
      return true;
    }
    if (BasicBlockDominatedBy(gen, child, b)) {
      return true;
    }
  }
  return false;
}


void BasicBlockCalculateDominanceFrontier(Generator* gen, BasicBlock* b, Vector* blocks) {
  if (b->in_edges.length >= 2) {
    for (size_t i = 0; i < b->in_edges.length; i++) {
      BlockId id = (BlockId)b->in_edges.value.p[i];
      BasicBlock* block = VectorGet(blocks, id);
      if (BasicBlockIsUnreachable(gen, block)) {
        continue;
      }
      BasicBlock* runner = block;
      while (runner != NULL && runner != b->idom) { // TODO: this was NULL, which is right?
        BasicBlockAddToDF(runner, b->block_id);
        runner = runner->idom;
      }
    }
  }
}

void BasicBlockInitDominators(BasicBlock* b, bool is_start, size_t num_nodes) {
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

void BasicBlockAddToDF(BasicBlock* b, BlockId id) {
  BitSetInsert(&b->dominance_frontier, id);
}

void BasicBlockPrint(Generator* gen, BasicBlock* b, BasicBlock* entry, BasicBlock* exit, FILE* fp) {
  fprintf(fp, "*** Basic block #%zd%s\n", b->block_id,
         (b == entry ? " (ENTRY)" : (b == exit ? " (EXIT)" : "")));
  if (BasicBlockIsUnreachable(gen, b)) {
    fprintf(fp, "** Unreachable **\n");
  }
  if (b->return_block) {
    fprintf(fp, "  [return]\n");
  }
  fprintf(fp, "  In:");
  for (size_t i = 0; i < b->in_edges.length; i++) {
    fprintf(fp, " %zd", (BlockId)b->in_edges.value.p[i]);
  }
  fprintf(fp, "\n  Out:");
  for (size_t i = 0; i < b->out_edges.length; i++) {
    fprintf(fp, " %zd", (BlockId)b->out_edges.value.p[i]);
  }
  fprintf(fp, "\n  Back: ");
  BitSetPrint(&b->in_back_edges, fp);
  
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
  fprintf(fp, "  Number of calls: %d\n", b->num_calls);

  fprintf(fp, "  Defined variables:");
  for (size_t i = 0; i < b->defined_vars.length; i++) {
    Symbol* sym = b->defined_vars.values[i].key.p;
    fprintf(fp, " %s", sym->name.value);
  }
  fprintf(fp, "\n");
  fprintf(fp, "  Referenced variables:");
  for (size_t i = 0; i < b->referenced_vars.vec.length; i++) {
    Symbol* sym = b->referenced_vars.vec.value.p[i];
    fprintf(fp, " %s", sym->name.value);
  }
  fprintf(fp, "\n");
  
  IRNode* inst = b->code;
  while (inst != b->end_code) {
    IRPrint(inst, fp);
    inst = IRNext(inst);
  }
  if (b->end_code != NULL) {
    IRPrint(b->end_code, fp);
  }
  fprintf(fp, "\n");
}

bool BasicBlockEndsInBranchReturnOrCall(BasicBlock* b) {
  IRNode* node = b->end_code;
  return node != NULL && (IRIsBranch(node) ||
                          IRIsReturn(node) || IRIsCall(node));
}

void BasicBlockInsertVar(struct Generator* gen, BasicBlock* block,
                         struct IRNode* inst) {
#if 0
  // Move to end of phi nodes.
  IRNode* node = block->code;
  while (node != NULL && node->opcode == IR_OP(phi)) {
    node = IRNext(node);
  }
  BasicBlockEmitBefore(gen, block, inst, node);
#else
  BasicBlockEmitBefore(gen, block, inst, block->code);
#endif
}

bool BasicBlockInsertPhi(Generator* gen, BasicBlock* b, Symbol* sym) {
  // If there is no code in this block there is no need to add a PHI
  // node.
  if (b->code == NULL) {
    return false;
  }

  // Check if we have already added a PHI node to this block for the
  // symbol.  PHI nodes are always at the very beginning of the block.
  IRNode* node = b->code;
  while (node->opcode == IR_OP(phi)) {
    IRVariable* var = (IRVariable*)node;
    if (var->symbol == sym) {
      // Already added, don't add it again.
      return false;
    }
    node = IRNext(node);
  }

  // We need to add a PHI to the beginning of the block.
  IRNode* phi = NewIRPhi(sym);

  // PHI node defines a variable.
  IRSetVarDef(phi, sym);

  b->code = GeneratorEmitBefore(gen, phi, b->code);
  phi->block = b;
  
  // Tell caller that we've added a new node.  This defines a new
  // variable so we need to iterate until there are no more PHI
  // nodes inserted.
  return true;
}

// Remove an instruction from a basic block and IR code.
void BasicBlockRemoveInstruction(Generator* gen, BasicBlock* block,
                                 IRNode* inst) {
  // If this instruction is the first in the basic block, move the
  // block's code on to the next instruction.
  if (inst == block->code) {
    if (inst == block->end_code) {
      // Block has no instructions now.
      block->code = block->end_code = NULL;
    } else {
      block->code = IRNext(inst);
    } 
  }
  if (inst == block->end_code) {
    block->end_code = IRPrev(inst);
  }
  GeneratorRemoveInstruction(gen, inst);
}

void BasicBlockMoveInstructionAfter(Generator* gen,
                              IRNode* inst, IRNode* pos) {
  BasicBlock* from = inst->block;
  if (inst == from->code) {
    if (inst == from->end_code) {
      // Block has no instructions now.
      from->code = from->end_code = NULL;
    } else {
      from->code = IRNext(inst);
    }
  }
  if (inst == from->end_code) {
    from->end_code = IRPrev(inst);
  }
  GeneratorMoveInstructionAfter(gen, inst, pos);
  inst->block = pos->block;
  if (pos->block->end_code == pos) {
    pos->block->end_code = inst;
  }
}

void BasicBlockMoveInstructionBefore(Generator* gen,
                              IRNode* inst, IRNode* pos) {
  BasicBlock* from = inst->block;
  if (inst == from->code) {
    if (inst == from->end_code) {
        // Block has no instructions now.
        from->code = from->end_code = NULL;
      } else {
        from->code = IRNext(inst);
      }
  }
  if (inst == from->end_code) {
    from->end_code = IRPrev(inst);
  }
  GeneratorMoveInstructionBefore(gen, inst, pos);
  inst->block = pos->block;
  if (pos->block->code == pos) {
    pos->block->code = inst;
  }
}


// Replace the instruction 'old' with 'new'.  Removes 'old' when
// the replacement is done.
void BasicBlockReplaceInstruction(Generator* gen, BasicBlock* block,
                                  IRNode* old, IRNode* new) {
  // If this instruction is the first in the basic block, move the
  // block's code on to the next instruction.
  if (old == block->code) {
    block->code = IRNext(old);
  }
  if (old == block->end_code) {
    block->end_code = IRPrev(old);
  }
  GeneratorReplaceInstruction(gen, old, new);
  BasicBlockRemoveInstruction(gen, block, old);
}

void BasicBlockEmitBefore(struct Generator* gen, BasicBlock* block,
                          struct IRNode* inst, struct IRNode* pos) {
  // If the position is the first in the block we need to move
  // it to the new instruction.
  if (pos == block->code) {
    block->code = inst;
  }
  GeneratorEmitBefore(gen, inst, pos);
  inst->block = block;
}

bool BasicBlockIsUnreachable(struct Generator* gen, BasicBlock* b) {
  if (b == gen->entry_block) {
    return false;
  }
  if (b->reachability_known) {
    return b->is_unreachable;
  }
  b->reachability_known = true;
  // Check if all the in edges are unreachable.
  for (size_t i = 0; i < b->in_edges.length; i++) {
    BlockId id = b->in_edges.value.w[i];
    BasicBlock* in = VectorGet(&gen->basic_blocks, id);
    if (!BasicBlockIsUnreachable(gen, in)) {
      return false;
    }
  }
  b->is_unreachable = true;
  return true;
}

void BasicBlockRemoveInput(BasicBlock* block, BlockId block_id) {
  for (size_t i = 0; i < block->in_edges.length; i++) {
    BlockId input = block->in_edges.value.w[i];
    if (input == block_id) {
      VectorDeleteElement(&block->in_edges, i);
      break;
    }
  }
}

void BasicBlockRemoveEdge(BasicBlock* from, BasicBlock* to) {
  // Remove in edge from 'to'.
  for (size_t i = 0; i < to->in_edges.length; i++) {
    BlockId input = to->in_edges.value.w[i];
    if (input == from->block_id) {
      VectorDeleteElement(&to->in_edges, i);
      break;
    }
  }
  
  // Remove out edge from 'from'.
  for (size_t i = 0; i < from->out_edges.length; i++) {
    BlockId output = from->out_edges.value.w[i];
    if (output == to->block_id) {
      VectorDeleteElement(&from->out_edges, i);
      break;
    }
  }
  to->reachability_known = false;
}

// Remove all instructions from the block.  Don't remove
// named labels since these are referenced from debug info.
void BasicBlockClear(struct Generator* gen, BasicBlock* b) {
  if (b->code == NULL) {
    return;
  }
  IRNode* inst = b->code;
  IRNode* first_named_label = NULL;
  IRNode* last_named_label = NULL;
  while (inst != NULL && inst != b->end_code) {
    IRNode* next = IRNext(inst);
    if (inst->opcode != IR_OP(named_label)) {
      GeneratorRemoveInstruction(gen, inst);
    } else {
      if (first_named_label == NULL) {
        first_named_label = inst;
        last_named_label = inst;
      } else {
        last_named_label = inst;
      }
    }
    inst = next;
  }
  if (b->end_code != NULL && b->end_code->opcode != IR_OP(named_label)) {
    GeneratorRemoveInstruction(gen, b->end_code);
  } else {
    if (first_named_label == NULL) {
      first_named_label = inst;
      last_named_label = inst;
    } else {
      last_named_label = inst;
    }
  }
  b->code = first_named_label;
  b->end_code = last_named_label;
  
  // If this block is unreachable then we need to remove it as
  // an input from all its outputs.
  for (size_t i = 0; i < b->out_edges.length; i++) {
    BlockId out = b->out_edges.value.w[i];
    BasicBlock* out_block = VectorGet(&gen->basic_blocks, out);
    BasicBlockRemoveInput(out_block, b->block_id);
  }
}

static void TraverseDomTree(Generator* gen,
                            BasicBlock* block, TraversalFunc func,
                            TraversalMode mode, void* data) {
  if (block == NULL) {
    return;
  }
  if (mode == kTraversePreOrder) {
    func(block, data);
  }
  for (size_t i = 0; i < block->dominatees.length; i++) {
    BlockId child_id = block->dominatees.value.w[i];
    BasicBlock* child = gen->basic_blocks.value.p[child_id];
    if (child == block) {
      continue;
    }
    TraverseDomTree(gen, child, func, mode, data);
  }
  if (mode == kTraversePostOrder) {
    func(block, data);
  }
}

void BasicBlockTraverseDominatorTree(Generator* gen, BasicBlock* block, TraversalFunc func,
                             TraversalMode mode,
                             void* data) {
  TraverseDomTree(gen, block, func, mode, data);
}

IRNode* BasicBlockBegin(BasicBlock* b) {
  return b->code;
}

IRNode* BasicBlockEnd(BasicBlock* b) {
  if (b->end_code == NULL) {
    return NULL;
  }
  return (IRNode*)b->end_code->header.next;
}

IRNode* BasicBlockRBegin(BasicBlock* b) {
  return b->end_code;
}

IRNode* BasicBlockREnd(BasicBlock* b) {
  if (b->code == NULL) {
    return NULL;
  }
  return (IRNode*)b->code->header.prev;
}

bool BasicBlockIsEmpty(BasicBlock* b) {
  return b->code == NULL || b->end_code == NULL;
}

