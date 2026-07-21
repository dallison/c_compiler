//
//  x86_64_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "x86_64_optimize.h"
#include <assert.h>
#include "x86_64_codegen.h"
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
// Instruction sequence optimizations for the x86-64 backend.
//
// A requirement here is all the optimization be safe so that the output code is
// as correct as the input code.


struct OptimizerData {
  X86_64Generator* rv;
};

// Some moves (mv/fmv_*) store into operand[0] when dest is NULL.
static TargetInstruction* InstructionResult(TargetInstruction* inst) {
  if (inst->dest != NULL) {
    return inst->dest;
  }
  if (inst->operand[0] != NULL && X86_64IsVarRegister(inst->operand[0])) {
    return inst->operand[0];
  }
  return inst;
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
  X86_64Generator* rv = opt_data->rv;
  BitSet filter = {0};
  BitSetCopy(&filter, &block->output_ids);
  
  TrapRemoveInstructionBlock(block);

  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);

    X86_64Opcode opcode = (X86_64Opcode)inst->opcode;
    if (X86_64IsExpression(inst) &&
        !X86_64IsSymbol(inst) && !X86_64IsConst(inst) && opcode != X86_64_OP(tmp) &&
        opcode != X86_64_OP(sp)) {
      // Instruction is an expression.  If its result (maybe in dest)
      // is not in the filter, remove it.
      TargetInstruction* dest = InstructionResult(inst);
      bool is_candidate = true;
      
      // Check if destination is not in the output filter.
      if (dest != NULL && (((int)dest->opcode == (int)X86_64_OP(tmp)) ||
          X86_64IsFixedRegister(dest) ||
          (X86_64Opcode)dest->opcode == X86_64_OP(sp) ||
          (X86_64Opcode)dest->opcode == X86_64_OP(fp) ||
          (X86_64Opcode)dest->opcode == X86_64_OP(tp) ||
          BitSetContains(&filter, dest->id) ||
          X86_64IsResult(dest))) {
        is_candidate = false;
      }

      // If destination is not in the output filter then if the
      // expression is not used in this block it can be removed.
      if (is_candidate && ((int)inst->opcode != (int)X86_64_OP(tmp)) &&
          !X86_64IsFixedRegister(inst) &&
          !BitSetContains(&filter, inst->id)) {
        TrapRemoveInstruction(inst);
        TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        continue;
      }
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
static void RemoveUnusedExpressions(X86_64Generator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, RemoveBlockUnusedExpressions, kTraversePostOrder, &data);
}

// Symbol address loads use lea (or lea_rip for PC-relative).  An add used
// to form an address can be folded into the load/store offset when the sum
// fits in an immediate field.

static void CombineLoadOrStoresInBlock(TargetBasicBlock* block, void* data) {
  struct OptimizerData* opt_data = data;
  X86_64Generator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    TrapCombineLoadStore(inst);
    
    if (X86_64IsLoad(inst)) {
      TargetInstruction* base = inst->operand[0];
      if (base->opcode == (TargetOpcode)X86_64_OP(lea) &&
          !compiler->pic) {
        // Insert label for lea_rip relocation.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)X86_64_OP(label));
        label->flags = X86_64_EXPORTED_LABEL;
        TargetBasicBlockEmitBefore(&rv->base, block, label, base);
        base->opcode = (TargetOpcode)X86_64_OP(lea_rip);
        base->flags |= X86_64_PCREL_HI_RELOC;
        TargetReplaceOperand(inst, 1, label);
        inst->flags |= X86_64_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)X86_64_OP(add)) {
        // Load from an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = X86_64IntValue(inst->operand[1]);
        int immed = X86_64IntValue(base->operand[1]);
        if (X86_64IsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 0, base->operand[0]);
          TargetReplaceOperand(inst, 1, TargetGetIntConstant(
                                                             &rv->base,
                                                             NULL,
                                                             kTargetType32Bit,
                                                             offset + immed));
          if (base->users.length == 0) {
            TrapRemoveInstruction(base);
           TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
         }
      }
    } else if (X86_64IsStore(inst)) {
      TargetInstruction* base = inst->operand[1];
      // TODO: rip-relative store folding needs emitter support before enabling.
      if (false && base->opcode == (TargetOpcode)X86_64_OP(lea) && !compiler->pic) {
        // Insert label for lea_rip relocation.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)X86_64_OP(label));
        label->flags = X86_64_EXPORTED_LABEL;
        TargetEmitBefore(&rv->base, label, base);
        base->opcode = (TargetOpcode)X86_64_OP(lea_rip);
        base->flags |= X86_64_PCREL_HI_RELOC;
        TargetReplaceOperand(inst, 2, label);
        inst->flags |= X86_64_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)X86_64_OP(add)) {
        // Store to an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = X86_64IntValue(inst->operand[2]);
        int immed = X86_64IntValue(base->operand[1]);
        if (X86_64IsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 1, base->operand[0]);
          TargetReplaceOperand(inst, 2, TargetGetIntConstant(
                                                             &rv->base,
                                                             NULL,
                                                             kTargetType32Bit,
                                                             offset + immed));
          if (base->users.length == 0) {
            TrapRemoveInstruction(base);
           TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
        }
      }
    }
  }
}

static void CombineLoadOrStores(X86_64Generator* rv) {
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
  X86_64Generator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
     prev = TargetPrev(inst);
    TrapPropagateZero(inst);
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (operand != NULL) {
        if (((int)operand->opcode == (int)X86_64_OP(mv)) && operand->users.length == 1) {
          TargetInstruction* mv = operand;
          if (mv->operand[0]->opcode == (TargetOpcode)X86_64_OP(x0)) {
            // Found mv xx, x0.  Replace instruction operand with x0.
            TargetReplaceOperand(inst, i,  mv->operand[0]);
            
            // We can now eliminate the mv instruction.
            TrapRemoveInstruction(mv);
            TargetBasicBlockRemoveInstruction(&rv->base, block, mv);
          }
        }
      }
    }
  }
}

static void PropagateZeroes(X86_64Generator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, PropagateZeroesInBlock, kTraversePostOrder, &data);
}

typedef struct {
  X86_64Generator* rv;
  Map pool;       // Key: int64_t constant, value: instruction.
} ConstantPooler;

static void PoolConstantsInBlock(TargetBasicBlock* block, void* data) {
  ConstantPooler* pooler = data;
  X86_64Generator* rv = pooler->rv;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    TrapPoolConstant(inst);

    if (inst->opcode == (TargetOpcode)X86_64_OP(mov) && inst->dest == NULL) {
      // Possible instruction to pool.
      int64_t value = X86_64IntValue(inst->operand[0]);
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

static void PoolConstants(X86_64Generator* rv) {
  ConstantPooler pooler = {rv};
  MapInitForInt64Keys(&pooler.pool);
  PoolConstantsInBlock(rv->base.entry_block, &pooler);
  MapDestruct(&pooler.pool);
}

static void EliminateMovesInBlock(TargetBasicBlock* block, void* data) {
  X86_64Generator* rv = data;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
      !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    if (inst->opcode == (TargetOpcode)X86_64_OP(mv)) {
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
        if (!X86_64IsVarRegister(inst_dest) &&
            X86_64IsExpression(prev) && prev->users.length == 1) {
           // rmov an expression to a register, just retarget the
          // expression to the register.  We use rmov to assign to a
          // register variable so we don't eliminate that.
          prev->dest = inst_dest;
          TrapRemoveInstruction(inst);
          TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        } else if (prev->opcode == (TargetOpcode)X86_64_OP(mv)) {
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

static void EliminateMoves(X86_64Generator* rv) {
  TargetTraverseDominatorTree(&rv->base, EliminateMovesInBlock, kTraversePostOrder, rv);
}

void X86_64Optimize(X86_64Generator* rv) {  
  // Eliminate moves if we can.
  EliminateMoves(rv);

  // RVPrintBasicBlocks(rv, stdout);

  // Remove all unused expressions.
  RemoveUnusedExpressions(rv);
  // RVPrintBasicBlocks(rv, stdout);

  CombineLoadOrStores(rv);
  // RVPrintBasicBlocks(rv, stdout);

  PropagateZeroes(rv);
  // RVPrintBasicBlocks(rv, stdout);

  // Pool multi-use constants as instructions.
  PoolConstants(rv);
  // RVPrintBasicBlocks(rv, stdout);

  // RVPrintBasicBlocks(rv, stdout);
  // Rebuild the basic block input and outputs.
  TargetBuildBasicBlockInputsAndOutputs(&rv->base);
}


