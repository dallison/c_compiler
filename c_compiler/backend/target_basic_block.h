//
//  risc_v_basic_block.h
//  c_compiler_library
//
//  Created by David Allison on 5/27/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef risc_v_basic_block_h
#define risc_v_basic_block_h

#include <stdint.h>
#include "bitset.h"
#include "map.h"
#include "set.h"
#include "vector.h"
#include "target_generator.h"

typedef size_t TargetBlockId;

typedef struct TargetBasicBlock {
  TargetBlockId block_id;
  Vector in_edges;   // Vector of TargetBlockId.
  Vector out_edges;  // Vector of TargetBlockId.
  struct TargetInstruction* code;
  struct TargetInstruction* end_code;

  BitSet dominators;          // Set of ids for blocks that dominate me.
  size_t num_dominators;      // Number of dominators.
  Vector dominatees;          // Vector of ids for blocks that I dominate.
  BitSet dominance_frontier;  // Set of TargetBlockId.
  struct TargetBasicBlock* idom;
  
  // Instructions that are alive on input to this block.
  Vector inputs;
  BitSet input_ids;
  
  // Instructions alive at exit of this block.
  Vector outputs;
  BitSet output_ids;      // Ids of instructions in the output.
  int num_spills;         // Number of spills in this block.
  int loop_nesting;       // Loop nesting level.
  
  bool contains_call;
  bool reachability_known;
  bool is_unreachable;
  bool uses_floating_point;
  void* cookie;  // Cookie for general traversal use.
} TargetBasicBlock;

TargetBasicBlock* NewTargetBasicBlock(TargetBlockId id);
void TargetBasicBlockDelete(TargetBasicBlock* block);

void TargetBasicBlockAddEdge(TargetBasicBlock* from, TargetBasicBlock* to);

void TargetBasicBlockAddInEdge(TargetBasicBlock* from, TargetBasicBlock* to);

bool TargetBasicBlockCalculateDominators(TargetGenerator* rv, TargetBasicBlock* b, Vector* blocks);
void TargetBasicBlockCalculateImmediateDominator(TargetBasicBlock* b, Vector* blocks);
void TargetBasicBlockCalculateDominanceFrontier(TargetBasicBlock* b, Vector* blocks);

void TargetBasicBlockInitDominators(TargetBasicBlock* b, bool is_start, size_t num_nodes);
void TargetBasicBlockAddToDF(TargetBasicBlock* b, TargetBlockId id);

void TargetBasicBlockPrint(struct TargetGenerator* rv,
                       TargetBasicBlock* b,
                       TargetBasicBlock* entry,
                       TargetBasicBlock* exit,
                       FILE* fp);

bool TargetBasicBlockEndsInBranchOrReturn(TargetGenerator* gen, TargetBasicBlock* b);


void TargetBasicBlockRemoveInstruction(struct TargetGenerator* rv, TargetBasicBlock* block,
                                 struct TargetInstruction* inst);
void TargetBasicBlockReplaceInstruction(struct TargetGenerator* rv, TargetBasicBlock* block,
                                  struct TargetInstruction* old, struct TargetInstruction* new);

void TargetBasicBlockEmitBefore(struct TargetGenerator* rv, TargetBasicBlock* block,
                          struct TargetInstruction* inst, struct TargetInstruction* pos);
void TargetBasicBlockEmitAfter(struct TargetGenerator* rv, TargetBasicBlock* block,
                           struct TargetInstruction* inst, struct TargetInstruction* pos);
  
bool TargetBasicBlockIsUnreachable(struct TargetGenerator* rv, TargetBasicBlock* b);
void TargetBasicBlockClear(struct TargetGenerator* rv, TargetBasicBlock* b);

void TargetBuildBasicBlocks(struct TargetGenerator* rv);
void TargetPrintBasicBlocks(TargetGenerator* rv, FILE* fp);
void TargetBuildBasicBlockInputsAndOutputs(TargetGenerator* rv);

// TraverseMode is in basic_block.h

typedef void (*TargetTraversalFunc)(TargetBasicBlock* block, void* data);

void TargetTraverseDominatorTree(TargetGenerator* rv, TargetTraversalFunc func,
                             TraversalMode mode,
                             void* data);

void TargetBasicBlockTraverseDominatorTree(TargetGenerator* rv, TargetBasicBlock* block,
                                       TargetTraversalFunc func,
                                       TraversalMode mode,
                                       void* data);

// Does the basic block output the given instruction?
bool TargetBasicBlockOutputs(TargetBasicBlock* block, TargetInstruction* inst);
void TargetBasicBlockPropagateExpression(TargetGenerator* gen, TargetInstruction* inst, TargetBasicBlock* to);

// Iterators
TargetInstruction* TargetBasicBlockBegin(TargetBasicBlock* b);
TargetInstruction* TargetBasicBlockEnd(TargetBasicBlock* b);
TargetInstruction* TargetBasicBlockRBegin(TargetBasicBlock* b);
TargetInstruction* TargetBasicBlockREnd(TargetBasicBlock* b);

bool TargetBasicBlockIsEmpty(TargetBasicBlock* b);

// Is b dominated by dom (in its dominiator tree)?
bool TargetBasicBlockDominatedBy(TargetGenerator* gen, TargetBasicBlock* dom, TargetBasicBlock* b);

#endif /* risc_v_basic_block_h */
