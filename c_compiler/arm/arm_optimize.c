//
//  arm_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "arm_optimize.h"
#include <assert.h>
#include "arm_codegen.h"
#include "compiler.h"
#include "map.h"

static void Trap(){}

// Set breakpoints in these and change the instuction id to
// trap the instruction optimization passes.
static void TrapRemoveInstruction(TargetInstruction* inst) {
  if (inst->id == 99) {
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
// This file contains functions to optimize the instruction sequence for ARM32.
// Now that we have lowered the IR to actual ARM instructions we can look for
// sequences that can be made more optimal given the details of the instruction
// set.
//
// A requirement here is all the optimization be safe so that the output code is
// as correct as the input code.


struct OptimizerData {
  ARMGenerator* rv;
};

// Comparison instructions produce condition flags rather than a register
// result.  The flags are consumed by a following conditional branch / csel,
// but that dependency is not modelled as an operand, so dead-code elimination
// (which tracks register results) must never treat these as unused.
static bool ARMSetsConditionFlags(TargetInstruction* inst) {
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(cmp):
    case ARM_OP(cmn):
    case ARM_OP(tst):
    case ARM_OP(fcmp):
    case ARM_OP(ccmp):
    case ARM_OP(ccmn):
    // The flag-setting ('s' suffix) arithmetic/logical variants exist solely so
    // their condition codes can drive a later conditional branch -- e.g. a
    // 64-bit `x != 1` lowers to `eor/eor/orrs` and a 64-bit `<` to `subs/sbcs`,
    // where the register result is unused and only the flags matter.  They have
    // no operand users, so they must be treated as having a side effect or
    // RemoveUnusedExpressions deletes the whole comparison.
    case ARM_OP(adds):
    case ARM_OP(adcs):
    case ARM_OP(subs):
    case ARM_OP(sbcs):
    case ARM_OP(ands):
    case ARM_OP(bics):
    case ARM_OP(orrs):
      return true;
    default:
      return false;
  }
}


// Remove unused instructions from the basic block.
// The algorithm uses a filter to determine if the result
// of an expression is needed at the output of this basic block.  The
// filter is initialized with the block's outputs.
// Then we go backwards through the instructions.  If the instruction's
// result is not in the filter it means that the result will be ignored
// and it is removed.
// If the instruction is not removed, all of its operands are added to the
// filter.
static void RemoveBlockUnusedExpressions(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  ARMGenerator* rv = opt_data->rv;
  BitSet filter = {0};
  BitSetCopy(&filter, &block->output_ids);
  
  TrapRemoveInstructionBlock(block);

  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);

    ARMOpcode opcode = (ARMOpcode)inst->opcode;
    if (ARMIsExpression(inst) && !ARMSetsConditionFlags(inst) &&
        !ARMIsSymbol(inst) && !ARMIsConst(inst) && opcode != ARM_OP(tmp) &&
        opcode != ARM_OP(sp)) {
      // Instruction is an expression.  If its result (maybe in dest)
      // is not in the filter, remove it.
      TargetInstruction* dest = inst->dest;
      bool is_candidate = true;
      
      // Check if destination is not in the output filter.
      if (dest != NULL && (((int)dest->opcode == (int)ARM_OP(tmp)) ||
          ARMIsFixedRegister(dest) ||
          BitSetContains(&filter, dest->id) ||
          ARMIsResult(dest))) {
        is_candidate = false;
      }

      // If destination is not in the output filter then if the
      // expression is not used in this block it can be removed.
      if (is_candidate && ((int)inst->opcode != (int)ARM_OP(tmp)) &&
          !ARMIsFixedRegister(inst) &&
          !BitSetContains(&filter, inst->id)) {
        TrapRemoveInstruction(inst);
        TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        continue;
      }
#if 0
    } else if (((int)inst->opcode == (int)ARM_OP(rmov))) {
       TargetInstruction* result = inst->operand[0];
       if (inst->users.length > 0) {
         // Result of rmov is being used.  This overrides the destination
         // as the result
         result = inst;
       } else if (inst->dest != NULL) {
         result = inst->dest;
       } 
       if (ARMIsResult(result)) {
         goto dont_optimize;
       }
       if ((ARMOpcode)((int)result->opcode == (int)ARM_OP(sp))) {
         goto dont_optimize;
       }
      TargetInstruction* src = inst->operand[1];
      if (!BitSetContains(&filter, result->id)) {
         TrapRemoveInstruction(inst);
         TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
         continue;
      }
      // If the source for the rmov is an argument register and it is
      // not in the filter, and we are not moving to an output we can retarget 
      // all references to the destination to the source and eliminate
      // the rmov instruction
      if (ARMIsArgRegister(src) &&
          !BitSetContains(&filter, src->id) &&
          !BitSetContains(&block->output_ids, result->id)) {
#if 0
        if (ARMIsFixedRegister((ARMOpcode)result->opcode)) {
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
        TrapRemoveInstruction(inst);
        TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
      }
#endif
    }
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
static void RemoveUnusedExpressions(ARMGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, RemoveBlockUnusedExpressions, kTraversePostOrder, &data);
}

// The output from the codegen for loading and storing with a large offset is:
// add base, base, #page
// ldr dst, [base, #page_offset]
//
// When page_offset fits in the load/store immediate field, the add can be
// folded into the load/store offset if the add has no other users.

static bool IsAddWithImmediate(TargetInstruction* inst) {
  return inst != NULL &&
         inst->opcode == (TargetOpcode)ARM_OP(add) &&
         inst->operand[1] != NULL &&
         ARMIsIntConst(inst->operand[1]);
}

static void CombineLoadOrStoresInBlock(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  ARMGenerator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    TrapCombineLoadStore(inst);

    if (ARMIsLoad(inst)) {
      TargetInstruction* base = inst->operand[0];
      // Only fold a pure address-calculation add (no dest).  An add that also
      // writes a register variable (dest != NULL) updates that register in
      // place, so its source operand has already been overwritten by the time
      // the load runs -- folding the add's immediate into the load would then
      // address the post-update value (e.g. the load in `*(++p)`).
      if (IsAddWithImmediate(base) && base->dest == NULL) {
        int offset = ARMIntValue(inst->operand[1]);
        int immed = ARMIntValue(base->operand[1]);
        if (ARMIsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 0, base->operand[0]);
          TargetReplaceOperand(inst, 1, TargetGetIntConstant(
              &rv->base, NULL, kTargetType32Bit, offset + immed));
          // Only drop the address calculation if nothing else needs it.  A
          // dest means the add also assigns a (variable) register used
          // elsewhere, so it must be kept even with no remaining operand users.
          if (base->users.length == 0 && base->dest == NULL) {
            TrapRemoveInstruction(base);
            TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
        }
      }
    } else if (ARMIsStore(inst)) {
      TargetInstruction* base = inst->operand[1];
      // See the load case: only fold a pure address-calculation add.
      if (IsAddWithImmediate(base) && base->dest == NULL) {
        int offset = ARMIntValue(inst->operand[2]);
        int immed = ARMIntValue(base->operand[1]);
        if (ARMIsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 1, base->operand[0]);
          TargetReplaceOperand(inst, 2, TargetGetIntConstant(
              &rv->base, NULL, kTargetType32Bit, offset + immed));
          // See the load case: keep the add if it also defines a dest register.
          if (base->users.length == 0 && base->dest == NULL) {
            TrapRemoveInstruction(base);
            TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
        }
      }
    }
  }
}

static void CombineLoadOrStores(ARMGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, CombineLoadOrStoresInBlock, kTraversePreOrder, &data);
}

// Look for instructions that have an operand that is a single-use move from zr.
// For these, replace the operand with zr and eliminate the move.

static TargetInstruction* MoveSourceRegister(TargetInstruction* mv) {
  if (mv->opcode == (TargetOpcode)ARM_OP(mv) && mv->operand[1] != NULL) {
    return mv->operand[1];
  }
  if (mv->opcode == (TargetOpcode)ARM_OP(mov) ||
      mv->opcode == (TargetOpcode)ARM_OP(mv)) {
    return mv->operand[0];
  }
  return NULL;
}

static bool IsZeroRegister(TargetInstruction* reg) {
  return reg != NULL && reg->opcode == (TargetOpcode)ARM_OP(zr);
}

static void PropagateZeroesInBlock(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  ARMGenerator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    TrapPropagateZero(inst);
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (operand == NULL) {
        continue;
      }
      if ((operand->opcode != (TargetOpcode)ARM_OP(mv) &&
           operand->opcode != (TargetOpcode)ARM_OP(mov)) ||
          operand->users.length != 1) {
        continue;
      }
      TargetInstruction* src = MoveSourceRegister(operand);
      if (!IsZeroRegister(src)) {
        continue;
      }
      TargetReplaceOperand(inst, i, src);
      TrapRemoveInstruction(operand);
      TargetBasicBlockRemoveInstruction(&rv->base, block, operand);
    }
  }
}

static void PropagateZeroes(ARMGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, PropagateZeroesInBlock, kTraversePostOrder, &data);
}

#if 0
static bool CanPoolConstant(TargetInstruction* inst, int op) {
  ARMOpcode opcode = (ARMOpcode)inst->opcode;
  if (ARMIsLoad(opcode)) {
    // Loads can't use a pooled constant.
    return false;
  }
  if (ARMIsStore(opcode)) {
    // Store can only store a pooled constant (not as offset)
    return op == 0;
  }
  
  switch (opcode) {
    case ARM_OP(li):
      return false;
      // Instructions with immediate operands.  Can use pooled constant
      // only for first operand.
    case ARM_OP(addi):
    case ARM_OP(slti):
    case ARM_OP(sltiu):
    case ARM_OP(xori):
    case ARM_OP(ori):
    case ARM_OP(andi):
    case ARM_OP(slli):
    case ARM_OP(srli):
    case ARM_OP(srai):
    case  ARM_OP(addiw):
    case  ARM_OP(slliw):
    case  ARM_OP(srliw):
    case  ARM_OP(sraiw):
      return op != 1;
    default:
      return true;
  }
}
#endif

typedef struct {
  ARMGenerator* rv;
  Map pool;       // Key: int64_t constant, value: instruction.
} ConstantPooler;

static void PoolConstantsInBlock(TargetBasicBlock* block, void* data) {
  ConstantPooler* pooler = data;
  ARMGenerator* rv = pooler->rv;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    TrapPoolConstant(inst);

    if (inst->opcode == (TargetOpcode)ARM_OP(mov) && inst->dest == NULL &&
        inst->operand[0] != NULL && ARMIsIntConst(inst->operand[0])) {
      // Possible instruction to pool.
      int64_t value = ARMIntValue(inst->operand[0]);
      MapKeyType key = {.w = value};
      TargetInstruction* pooled = MapFind(&pooler->pool, key);
      if (pooled != NULL) {
        // Found a pooled li instruction for same constant.  Replace all
        // references to this instruction with the pooled one.
        TargetRetargetInstruction(inst, pooled);
        TrapRemoveInstruction(inst);
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

static void PoolConstants(ARMGenerator* rv) {
  ConstantPooler pooler = {rv};
  MapInitForInt64Keys(&pooler.pool);
  PoolConstantsInBlock(rv->base.entry_block, &pooler);
  MapDestruct(&pooler.pool);
}

static void EliminateMovesInBlock(TargetBasicBlock* block, void* data) {
  ARMGenerator* rv = data;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    if (inst->opcode == (TargetOpcode)ARM_OP(mv)) {
       TargetInstruction* inst_dest = inst->dest;
      if (inst_dest == NULL) {
        continue;
      }
      TargetInstruction* inst_src = inst->operand[0];
      TargetInstruction* prev_prev;
      for (TargetInstruction* prev = TargetPrev(inst);
           prev != NULL && TargetNext(prev) != block->code;
           prev = prev_prev) {
        prev_prev = TargetPrev(prev);
        if (prev->dest == inst_src || inst->dest == inst_dest) {
          // src or dest haveu been assigned to, not candidate.
          continue;
        }
        if (ARMIsVarRegister(inst_dest) &&
            ARMIsExpression(prev) && prev->users.length == 1) {
           // rmov an expression to a register, just retarget the
          // expression to the register.  We use rmov to assign to a
          // register variable so we don't eliminate that.
          prev->dest = inst_dest;
          TrapRemoveInstruction(inst);
          TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        } else if (prev->opcode == (TargetOpcode)ARM_OP(mv)) {
          TargetInstruction* prev_dest = prev->dest;
          TargetInstruction* prev_src = prev->operand[0];
          if (inst_dest == prev_src && inst_src == prev_dest) {
            // mov a,b
            // ...
            // mov b,a
            // Eliminate second rmov instructions.
            TrapRemoveInstruction(inst);
            TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
            break;
          }
        }
        prev = prev_prev;
      }
    }
  }
}

static void EliminateMoves(ARMGenerator* rv) {
  TargetTraverseDominatorTree(&rv->base, EliminateMovesInBlock, kTraversePostOrder, rv);
}

void ARMOptimize(ARMGenerator* rv) {  
  // Eliminate moves if we can.
  EliminateMoves(rv);

  // ARMPrintBasicBlocks(rv, stdout);

  // Remove all unused expressions.
  RemoveUnusedExpressions(rv);
  // ARMPrintBasicBlocks(rv, stdout);

  // A load or store from an address calculated using the 'la' instruction can
  // be combined with the 'la'.
  CombineLoadOrStores(rv);
  // ARMPrintBasicBlocks(rv, stdout);

  // x0 is always available as a source register.  Look for single-use mv
  // instructions from x0 and propagate x0 to the destination.
  PropagateZeroes(rv);
  // ARMPrintBasicBlocks(rv, stdout);

  // Pool multi-use constants as instructions.
  PoolConstants(rv);
  // ARMPrintBasicBlocks(rv, stdout);

  // ARMPrintBasicBlocks(rv, stdout);
  // Rebuild the basic block input and outputs.
  TargetBuildBasicBlockInputsAndOutputs(&rv->base);
}


