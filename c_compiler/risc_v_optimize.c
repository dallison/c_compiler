//
//  risc_v_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_optimize.h"
#include <assert.h>
#include "risc_v_codegen.h"
#include "compiler.h"
#include "map.h"

static void Trap(){}

// Set breakpoints in these and change the instuction id to
// trap the instruction optimization passes.
static void TrapRemoveInstruction(TargetInstruction* inst) {
  if (inst->id == 53) {
    Trap();
  }
}

static void TrapRemoveInstructionBlock(TargetBasicBlock* b) {
  if (b->block_id == 53) {
    Trap();
  }
}

static void TrapCombineLoadStore(TargetInstruction* inst) {
  if (inst->id == 480) {
    Trap();
  }
}


static void TrapPropagateZero(TargetInstruction* inst) {
  if (inst->id == 480) {
    Trap();
  }
}


static void TrapPoolConstant(TargetInstruction* inst) {
  if (inst->id == 480) {
    Trap();
  }
}
// This file contains functions to optimize the instruction sequence for RISC-V.
// Now that we have lowered the IR to actual RISC-V instructions we can look for
// sequences that can be made more optimal given the details of the instruction
// set.
//
// A requirement here is all the optimization be safe so that the output code is
// as correct as the input code.


struct OptimizerData {
  RVGenerator* rv;
};


// Remove unused instructions from the basic block.
// The algorithm uses a filter to determine if the result
// of an expression is needed at the output of this basic block.  The
// filter is initialized with the block's outputs.
// Then we go backwards through the instrucitons.  If the instruction's
// result is not in the filter it means that the result will be ignored
// and it is removed.
// If the instruction is not removed, all of its operands are added to the
// filter.
static void RemoveBlockUnusedExpressions(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  RVGenerator* rv = opt_data->rv;
  BitSet filter = {0};
  BitSetCopy(&filter, &block->output_ids);
  
  TrapRemoveInstructionBlock(block);

  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    TrapRemoveInstruction(inst);

    RVOpcode opcode = (RVOpcode)inst->opcode;
    if (RVIsExpression(inst) &&
        !RVIsSymbol(inst) && !RVIsConst(inst) && opcode != RV_OP(tmp) &&
        opcode != RV_OP(sp)) {
      // Instruction is an expression.  If its result (maybe in dest)
      // is not in the filter, remove it.
      TargetInstruction* dest = inst->dest;
      bool is_candidate = true;
      
      // Check if destination is not in the output filter.
      if (dest != NULL && (dest->opcode == RV_OP(tmp) ||
          RVIsFixedRegister(dest) ||
          BitSetContains(&filter, dest->id) ||
          RVIsResult(dest))) {
        is_candidate = false;
      }

      // If destination is not in the output filter than if the
      // expression is not used in this block it can be removed.
      if (is_candidate && inst->opcode != RV_OP(tmp) &&
          !RVIsFixedRegister(inst) &&
          !BitSetContains(&filter, inst->id)) {
        TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        continue;
      }
      } else if (inst->opcode == RV_OP(rmov)) {
       TargetInstruction* result = inst->operand[0];
       if (inst->dest != NULL) {
         result = inst->dest;
       }
       if (RVIsResult(result)) {
         goto dont_optimize;
       }
       if ((RVOpcode)result->opcode == RV_OP(sp)) {
         goto dont_optimize;
       }
      TargetInstruction* src = inst->operand[1];
      if (!BitSetContains(&filter, result->id)) {
         TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
         continue;
      }
      // If the source for the rmov is an argument register and it is
      // not in the filter, and we are not moving to an output we can retarget 
      // all references to the destination to the source and eliminate
      // the rmov instruction
      if (RVIsArgRegister(src) &&
          !BitSetContains(&filter, src->id) &&
          !BitSetContains(&block->output_ids, result->id)) {
#if 0
        if (RVIsFixedRegister((RVOpcode)result->opcode)) {
          // Moving to a fixed register.  See if we can set it as the
          // destination of the input.
          if (src->dest != NULL) {
            // Already has a destination, no joy, leave the rmov as is.
            BitSetInsert(&filter, src->id);
            continue;
          }
          src->dest = result;
          BitSetInsert(&filter, result->id);
          TargetBasicBlockRemoveInstruction(rv, block, inst);
          continue;
        }
#endif
        TargetRetargetInstruction(result, src);
        BitSetInsert(&filter, result->id);
        TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
      }
    }
dont_optimize:;
    // Add all of the instruction's operands to the filter.
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      if (inst->operand[i] != NULL) {
        BitSetInsert(&filter, inst->operand[i]->id);
      }
    }
  }
  BitSetDestruct(&filter);
}

// Remove any expressions that have no references.
static void RemoveUnusedExpressions(RVGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, RemoveBlockUnusedExpressions, kTraversePostOrder, &data);
}

// The output from the codegen for loading and storing symbols is this:
// la x, symbol_name
// lw y, 0(x)  (any other load or store)
//
// The 'la' instruction is a macro for loading the symbol address
// via a relocation and expands to:
// auipc x, %pcrel_hi(symbol_name)
// addi x, x, %pcrel_lo(symbol_name)
// lw y, 0(x)
//
// The addi can be combined with the load(store):
// auipc x, %pcrel_hi(symbol_name)
// lw y, %pcrel_lo(symbol_name), x
//
//
// Also, the sequence:
// addi x1, x2, n
// ld x, a(x1)  - any load or store
//
// can become:
// ld x, (n+a)(x2)
// as long as n+a is a possible immediate value.

static void CombineLoadOrStoresInBlock(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  RVGenerator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    TrapCombineLoadStore(inst);
    
    if (RVIsLoad(inst)) {
      TargetInstruction* base = inst->operand[0];
      if (base->opcode == (TargetOpcode)RV_OP(la) &&
          !compiler->pic) {
        // Insert label for auipc instruction.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)RV_OP(label));
        label->flags = RV_EXPORTED_LABEL;
        TargetBasicBlockEmitBefore(&rv->base, block, label, base);
        base->opcode = (TargetOpcode)RV_OP(auipc);
        base->flags |= RV_PCREL_HI_RELOC;
        TargetRetargetInstruction(inst->operand[1], label);
        TargetReplaceOperand(inst, 1, label);
        inst->flags |= RV_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)RV_OP(addi)) {
        // Load from an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = RVIntValue(inst->operand[1]);
        int immed = RVIntValue(base->operand[1]);
        if (RVIsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 0, base->operand[0]);
          TargetReplaceOperand(inst, 1, TargetGetIntConstant(
                                                             &rv->base,
                                                             NULL,
                                                             kTargetTypeWord,
                                                             offset + immed));
          if (base->users.length == 0) {
            TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
         }
      }
    } else if (RVIsStore(inst)) {
      TargetInstruction* base = inst->operand[1];
      if (base->opcode == (TargetOpcode)RV_OP(la) && !compiler->pic) {
        // Insert label for auipc instruction.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)RV_OP(label));
        label->flags = RV_EXPORTED_LABEL;
        TargetEmitBefore(&rv->base, label, base);
        base->opcode = (TargetOpcode)RV_OP(auipc);
        base->flags |= RV_PCREL_HI_RELOC;
        TargetReplaceOperand(inst, 2, label);
        inst->flags |= RV_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)RV_OP(addi)) {
        // Store to an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = RVIntValue(inst->operand[2]);
        int immed = RVIntValue(base->operand[1]);
        if (RVIsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 1, base->operand[0]);
          TargetReplaceOperand(inst, 2, TargetGetIntConstant(
                                                             &rv->base,
                                                             NULL,
                                                             kTargetTypeWord,
                                                             offset + immed));
          if (base->users.length == 0) {
            TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
        }
      }
    }
  }
}

static void CombineLoadOrStores(RVGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, CombineLoadOrStoresInBlock, kTraversePreOrder, &data);
}

// Look for instructions that have an operand that is a single-use mv from x0.
// For these, replace the operand with x0.
//
// For example:
// mv t0, x0
// sb t0, 0(t2)
//
// is replaced by:
// sb x0, 0(t2)
// (the mv instruction is eliminated).

static void PropagateZeroesInBlock(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  RVGenerator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
     prev = TargetPrev(inst);
    TrapPropagateZero(inst);
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (operand != NULL) {
        if (operand->opcode == RV_OP(mv) && operand->users.length == 1) {
          TargetInstruction* mv = operand;
          if (mv->operand[0]->opcode == (TargetOpcode)RV_OP(x0)) {
            // Found mv xx, x0.  Replace instruction operand with x0.
            TargetReplaceOperand(inst, i,  mv->operand[0]);
            
            // We can now eliminate the mv instruction.
            TargetBasicBlockRemoveInstruction(&rv->base, block, mv);
          }
        }
      }
    }
  }
}

static void PropagateZeroes(RVGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, PropagateZeroesInBlock, kTraversePostOrder, &data);
}

#if 0
static bool CanPoolConstant(TargetInstruction* inst, int op) {
  RVOpcode opcode = (RVOpcode)inst->opcode;
  if (RVIsLoad(opcode)) {
    // Loads can't use a pooled constant.
    return false;
  }
  if (RVIsStore(opcode)) {
    // Store can only store a pooled constant (not as offset)
    return op == 0;
  }
  
  switch (opcode) {
    case RV_OP(li):
      return false;
      // Instructions with immediate operands.  Can use pooled constant
      // only for first operand.
    case RV_OP(addi):
    case RV_OP(slti):
    case RV_OP(sltiu):
    case RV_OP(xori):
    case RV_OP(ori):
    case RV_OP(andi):
    case RV_OP(slli):
    case RV_OP(srli):
    case RV_OP(srai):
    case  RV_OP(addiw):
    case  RV_OP(slliw):
    case  RV_OP(srliw):
    case  RV_OP(sraiw):
      return op != 1;
    default:
      return true;
  }
}
#endif

typedef struct {
  RVGenerator* rv;
  Map pool;       // Key: int64_t constant, value: instruction.
} ConstantPooler;

static void PoolConstantsInBlock(TargetBasicBlock* block, void* data) {
  ConstantPooler* pooler = data;
  RVGenerator* rv = pooler->rv;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    TrapPoolConstant(inst);

    if (inst->opcode == (TargetOpcode)RV_OP(li) && inst->dest == NULL) {
      // Possible instruction to pool.
      int64_t value = RVIntValue(inst->operand[0]);
      MapKeyType key = {.w = value};
      TargetInstruction* pooled = MapFind(&pooler->pool, key);
      if (pooled != NULL) {
        // Found a pooled li instruction for same constant.  Replace all
        // references to this instruction with the pooled one.
        TargetRetargetInstruction(inst, pooled);
        TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        if (pooled->block != block) {
          // In dominator block, add as input to this block and output
          // from dominator block.
          TargetBasicBlockPropagateExpression(&rv->base, pooled, block);
        }
        continue;
      }
      MapKeyValue kv = {.key.w = value, .value.p = inst};
      MapInsert(&pooler->pool, kv);
    }
  }
  
  // Now look in all blocks dominated by this one.  We copy the
  // pool for each one so they only see constants in their
  // own dominator tree (dominating blocks can contain pooled
  // constants).
  for (size_t i = 0; i < block->dominatees.length; i++) {
    TargetBlockId child_id = block->dominatees.value.w[i];
    TargetBasicBlock* child = rv->base.basic_blocks.value.p[child_id];
    ConstantPooler sub_pooler = {rv};
    MapClone(&sub_pooler.pool, &pooler->pool);
    PoolConstantsInBlock(child, &sub_pooler);
    MapDestruct(&sub_pooler.pool);
  }
}

static void PoolConstants(RVGenerator* rv) {
  ConstantPooler pooler = {rv};
  MapInitForInt64Keys(&pooler.pool);
  PoolConstantsInBlock(rv->base.entry_block, &pooler);
  MapDestruct(&pooler.pool);
}

static void EliminateMovesInBlock(TargetBasicBlock* block, void* data) {
  RVGenerator* rv = data;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    if (inst->opcode == (TargetOpcode)RV_OP(rmov)) {
      TargetInstruction* inst_dest = inst->operand[0];
      TargetInstruction* inst_src = inst->operand[1];
      TargetInstruction* prev_prev;
      for (TargetInstruction* prev = TargetPrev(inst);
           prev != NULL && TargetNext(prev) != block->code;
           prev = prev_prev) {
        prev_prev = TargetPrev(prev);
        if (prev->dest == inst_src || inst->dest == inst_dest) {
          // src or dest haveu been assigned to, not candidate.
          continue;
        }
        if (prev->opcode == (TargetOpcode)RV_OP(rmov)) {
          TargetInstruction* prev_dest = prev->operand[0];
          TargetInstruction* prev_src = prev->operand[1];
          if (inst_dest == prev_src && inst_src == prev_dest) {
            // rmov a,b
            // ...
            // rmov b,a
            // Eliminate second rmov instructions.
            TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
            break;
          }
        }
        prev = prev_prev;
      }
    }
  }
}

static void EliminateMoves(RVGenerator* rv) {
  EliminateMovesInBlock(rv->base.entry_block, rv);
}

void RVOptimize(RVGenerator* rv) {  
  // Eliminate moves if we can.
  EliminateMoves(rv);

  // RVPrintBasicBlocks(rv, stdout);

  // Remove all unused expressions.
  RemoveUnusedExpressions(rv);
  // RVPrintBasicBlocks(rv, stdout);

  // A load or store from an address calculated using the 'la' instruction can
  // be combined with the 'la'.
  CombineLoadOrStores(rv);
  // RVPrintBasicBlocks(rv, stdout);

  // x0 is always available as a source register.  Look for single-use mv
  // instructions from x0 and propagate x0 to the destination.
  PropagateZeroes(rv);
  // RVPrintBasicBlocks(rv, stdout);

  // Pool multi-use constants as instructions.
  PoolConstants(rv);
  // RVPrintBasicBlocks(rv, stdout);

  // RVPrintBasicBlocks(rv, stdout);
  // Rebuild the basic block input and outputs.
  TargetBuildBasicBlockInputsAndOutputs(&rv->base);
}


