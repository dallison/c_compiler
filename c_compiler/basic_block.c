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

// Compare variable defintions.  The pointers are Symbol**.
static int CompareVariable(const void* a, const void* b) {
  Symbol* s1 = *(Symbol**)a;
  Symbol* s2 = *(Symbol**)b;

  ptrdiff_t diff = s1 - s2;
  return (int)diff;
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
  b->idom = NULL;
  b->optimizer_data = NULL;
  b->num_calls = 0;
  return b;
}

void BasicBlockDelete(BasicBlock* b) {
  // NOTE: the IR nodes are not owned by the BasicBlock.  They belong to the
  // Generator and that is responsible for deleting them.
  VectorDestruct(&b->in_edges);
  VectorDestruct(&b->out_edges);
  BitSetDestruct(&b->dominators);
  BitSetDestruct(&b->dominance_frontier);
  VectorDestruct(&b->dominatees);
  MapDestruct(&b->defined_vars);
  free(b);
}

void BasicBlockAddInEdge(BasicBlock* from, BasicBlock* to) {
  VectorAppend(&from->in_edges, (void*)to->block_id);
}

void BasicBlockAddEdge(BasicBlock* from, BasicBlock* to) {
  VectorAppend(&from->out_edges, (void*)to->block_id);
  BasicBlockAddInEdge(to, from);
}

bool BasicBlockCalculateDominators(BasicBlock* b, Vector* blocks) {
  BitSet dominators;
  BitSetInit(&dominators);
  BitSetCopy(&dominators, &b->dominators);

  BitSet intersection;
  BitSetInit(&intersection);

  // Calculate intersection.
  for (size_t i = 0; i < b->in_edges.length; i++) {
    BlockId id = (BlockId)b->in_edges.value[i];
    BasicBlock* dom_block = blocks->value[id];

    BitSetClear(&intersection);
    BitSetIntersection(&dominators, &dom_block->dominators, &intersection);
    BitSetClear(&dominators);
    BitSetCopy(&dominators, &intersection);
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
  size_t maxndoms = 0;

  // It's hard to iterate through a BitSet, so expand it into a temporary
  // vector.
  Vector dominators;
  VectorInit(&dominators);
  BitSetExpand(&b->dominators, &dominators);

  for (size_t i = 0; i < dominators.length; i++) {
    BlockId id = (BlockId)dominators.value[i];
    if (id != b->block_id) {
      BasicBlock* block = VectorGet(blocks, id);
      if (block->num_dominators > maxndoms) {
        maxndoms = block->num_dominators;
        b->idom = block;
      }
    }
  }
  VectorDestruct(&dominators);
}

void BasicBlockCalculateDominanceFrontier(BasicBlock* b, Vector* blocks) {
  if (b->in_edges.length >= 2) {
    for (size_t i = 0; i < b->in_edges.length; i++) {
      BlockId id = (BlockId)b->in_edges.value[i];
      BasicBlock* block = VectorGet(blocks, id);
      BasicBlock* runner = block;
      while (runner != b->idom) { // TODO: this was NULL, which is right?
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
    for (size_t i = 0; i < num_nodes; ++i) {
      BitSetInsert(&b->dominators, i);
    }
  }
}

void BasicBlockAddToDF(BasicBlock* b, BlockId id) {
  BitSetInsert(&b->dominance_frontier, id);
}

void BasicBlockPrint(BasicBlock* b, BasicBlock* entry, BasicBlock* exit) {
  printf("*** Basic block #%zd%s\n", b->block_id,
         (b == entry ? " (ENTRY)" : (b == exit ? " (EXIT)" : "")));
  if (b != entry && b->in_edges.length == 0) {
    printf("** Unreachable **\n");
  }
  printf("  In:");
  for (size_t i = 0; i < b->in_edges.length; i++) {
    printf(" %zd", (BlockId)b->in_edges.value[i]);
  }
  printf("\n  Out:");
  for (size_t i = 0; i < b->out_edges.length; i++) {
    printf(" %zd", (BlockId)b->out_edges.value[i]);
  }
  printf("\n  Dominators: ");
  BitSetPrint(&b->dominators);

  printf("\n  Dominatees:");
  for (size_t i = 0; i < b->dominatees.length; i++) {
    printf(" %zd", (BlockId)b->dominatees.value[i]);
  }
  printf("\n  DF: ");
  BitSetPrint(&b->dominance_frontier);

  printf("\n  Immediate Dominator: ");
  if (b->idom == NULL) {
    printf("NIL\n");
  } else {
    printf("%zd\n", b->idom->block_id);
  }

  printf("  Defined variables:");
  for (size_t i = 0; i < b->defined_vars.length; i++) {
    Symbol* sym = b->defined_vars.values[i].key;
    printf(" %s", sym->name.value);
  }
  printf("\n");

  IRNode* inst = b->code;
  while (inst != b->end_code) {
    IRPrint(inst);
    inst = IRNext(inst);
  }
  if (b->end_code != NULL) {
    IRPrint(b->end_code);
  }
  printf("\n");
}

bool BasicBlockEndsInBranchOrReturn(BasicBlock* b) {
  IRNode* node = b->end_code;
  return node != NULL && (IRIsBranch(node) || IRIsReturn(node));
}

void BasicBlockInsertVar(struct Generator* gen, BasicBlock* block,
                         struct IRNode* inst) {
  // Move to end of phi nodes.
  IRNode* node = block->code;
  while (node != NULL && node->opcode == IR_OP(phi)) {
    node = IRNext(node);
  }
  BasicBlockEmitBefore(gen, block, inst, node);
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
    block->code = IRNext(inst);
  }
  if (inst == block->end_code) {
    block->end_code = IRPrev(inst);
  }
  GeneratorRemoveInstruction(gen, inst);
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
}

bool BasicBlockIsUnreachable(struct Generator* gen, BasicBlock* b) {
  return b != gen->entry_block && b->in_edges.length == 0;
}

// Remove all instructions from the block.
void BasicBlockClear(struct Generator* gen, BasicBlock* b) {
  if (b->code == NULL) {
    return;
  }
  IRNode* inst = b->code;
  while (inst != NULL && inst != b->end_code) {
    IRNode* next = IRNext(inst);
    GeneratorRemoveInstruction(gen, inst);
    inst = next;
  }
  GeneratorRemoveInstruction(gen, b->end_code);
  b->code = NULL;
  b->end_code = NULL;
}
