//
//  basic_block.h
//  c_compiler
//
//  Created by David Allison on 12/12/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef basic_block_h
#define basic_block_h

#include <stdint.h>
#include "bitset.h"
#include "map.h"
#include "set.h"
#include "vector.h"

struct Generator;
struct IRnode;
struct Symbol;

typedef size_t BlockId;

// We use BlockId (integral) as a reference to a block because we
// need to perform set operations on them.  A BitSet is very efficient
// for storing integers but doesn't work for other types.  The
// Generator struct (the code generator) has a vector of BasicBlock
// pointers to we can translate a block id into a BasicBlock simply
// by indexing that vector.
typedef struct BasicBlock {
  BlockId block_id;
  Vector in_edges;   // Vector of BlockId.
  Vector out_edges;  // Vector of BlockId.
  struct IRNode* code;
  struct IRNode* end_code;

  BitSet dominators;          // Set of ids for blocks that dominate me.
  size_t num_dominators;      // Number of dominators.
  Vector dominatees;          // Vector of ids for blocks that I dominate.
  BitSet dominance_frontier;  // Set of BlockId.
  struct BasicBlock* idom;

  // This is a map of variables defined in this block vs the SSA variable node
  // last assigned to it.
  Map defined_vars;

  void* optimizer_data;  // Data for optimization pass.
  int num_calls;         // Number of function calls in block.
} BasicBlock;

BasicBlock* NewBasicBlock(BlockId id);
void BasicBlockDelete(BasicBlock* block);

void BasicBlockAddEdge(BasicBlock* from, BasicBlock* to);

void BasicBlockAddInEdge(BasicBlock* from, BasicBlock* to);

bool BasicBlockCalculateDominators(BasicBlock* b, Vector* blocks);
void BasicBlockCalculateImmediateDominator(BasicBlock* b, Vector* blocks);
void BasicBlockCalculateDominanceFrontier(BasicBlock* b, Vector* blocks);

void BasicBlockInitDominators(BasicBlock* b, bool is_start, size_t num_nodes);
void BasicBlockAddToDF(BasicBlock* b, BlockId id);

void BasicBlockPrint(BasicBlock* b, BasicBlock* entry, BasicBlock* exit);

bool BasicBlockEndsInBranchOrReturn(BasicBlock* b);

void BasicBlockInsertVar(struct Generator* gen, BasicBlock* block,
                         struct IRNode* inst);

bool BasicBlockInsertPhi(struct Generator* gen, BasicBlock* b,
                         struct Symbol* sym);
void BasicBlockRemoveInstruction(struct Generator* gen, BasicBlock* block,
                                 struct IRNode* inst);
void BasicBlockReplaceInstruction(struct Generator* gen, BasicBlock* block,
                                  struct IRNode* old, struct IRNode* new);

void BasicBlockEmitBefore(struct Generator* gen, BasicBlock* block,
                          struct IRNode* inst, struct IRNode* pos);

bool BasicBlockIsUnreachable(struct Generator* gen, BasicBlock* b);
void BasicBlockClear(struct Generator* gen, BasicBlock* b);

#endif /* basic_block_h */
