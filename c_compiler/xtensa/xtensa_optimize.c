//
//  xtensa_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "xtensa_optimize.h"
#include <assert.h>
#include "compiler.h"
#include "map.h"
#include "xtensa_codegen.h"

static void Trap() {}

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
// This file contains functions to optimize the instruction sequence for
// RISC-V32. Now that we have lowered the IR to actual RISC-V32 instructions we
// can look for sequences that can be made more optimal given the details of the
// instruction set.
//
// A requirement here is all the optimization be safe so that the output code is
// as correct as the input code.

struct OptimizerData {
  XTENSAGenerator* rv;
};

// An atomic read-modify-write updates memory as well as producing a value, and
// an atomic load carries the `fence` its ordering needs, so both must survive
// even when nobody reads the value they return.  The plain atomic store and the
// standalone fence are already not expressions; these are.
static bool XTENSAHasImplicitEffect(TargetInstruction* inst) {
  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(atomic_load):
    case XTENSA_OP(atomic_fetch_add):
    case XTENSA_OP(atomic_fetch_sub):
    case XTENSA_OP(atomic_add_fetch):
    case XTENSA_OP(atomic_sub_fetch):
    case XTENSA_OP(atomic_compare_exchange_bool):
    case XTENSA_OP(atomic_compare_exchange_val):
    case XTENSA_OP(atomic_compare_exchange_n):
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
  XTENSAGenerator* rv = opt_data->rv;
  BitSet filter = {0};
  BitSetCopy(&filter, &block->output_ids);

  TrapRemoveInstructionBlock(block);

  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);

    XTENSAOpcode opcode = (XTENSAOpcode)inst->opcode;
    if (!inst->observable_checkpoint && !XTENSAHasImplicitEffect(inst) &&
        XTENSAIsExpression(inst) && !XTENSAIsSymbol(inst) &&
        !XTENSAIsConst(inst) && opcode != XTENSA_OP(tmp) &&
        opcode != XTENSA_OP(sp)) {
      // Instruction is an expression.  If its result (maybe in dest)
      // is not in the filter, remove it.
      TargetInstruction* dest = inst->dest;

      // An rmov (`mv reg, src`, with no SSA dest) writes its target register
      // through operand[0] rather than producing an SSA value.  Consumers such
      // as a following regarg/call read that physical register, not the move
      // itself, so the move has no SSA users.  Treat the written register as
      // the result here; otherwise the move is dropped even when the register
      // is live (e.g. a float argument converted into an integer argument
      // register for a variadic call), leaving the argument undefined.
      if (dest == NULL && inst->operand[1] != NULL &&
          (opcode == XTENSA_OP(mv) || opcode == XTENSA_OP(fmv_s) ||
           opcode == XTENSA_OP(fmv_d))) {
        dest = inst->operand[0];
      }

      bool is_candidate = true;

      // Check if destination is not in the output filter.
      // An instruction writing sp adjusts the stack: the outgoing argument area
      // is popped by `addi sp, sp, n`, whose value nothing reads.  Dropping it
      // leaves sp low for the rest of the function, and the epilogue restores
      // the callee-saved registers at sp-relative offsets before it recovers sp
      // from fp, so it would reload them from the wrong place.
      if (dest != NULL &&
          (((int)dest->opcode == (int)XTENSA_OP(tmp)) ||
           ((int)dest->opcode == (int)XTENSA_OP(sp)) ||
           XTENSAIsFixedRegister(dest) || BitSetContains(&filter, dest->id) ||
           XTENSAIsResult(dest))) {
        is_candidate = false;
      }

      // If destination is not in the output filter then if the
      // expression is not used in this block it can be removed.
      if (is_candidate && ((int)inst->opcode != (int)XTENSA_OP(tmp)) &&
          !XTENSAIsFixedRegister(inst) && !BitSetContains(&filter, inst->id)) {
        TrapRemoveInstruction(inst);
        TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        continue;
      }
#if 0
    } else if (((int)inst->opcode == (int)XTENSA_OP(rmov))) {
       TargetInstruction* result = inst->operand[0];
       if (inst->users.length > 0) {
         // Result of rmov is being used.  This overrides the destination
         // as the result
         result = inst;
       } else if (inst->dest != NULL) {
         result = inst->dest;
      }
       if (XTENSAIsResult(result)) {
         goto dont_optimize;
       }
       if ((XTENSAOpcode)((int)result->opcode == (int)XTENSA_OP(sp))) {
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
      if (XTENSAIsArgRegister(src) &&
          !BitSetContains(&filter, src->id) &&
          !BitSetContains(&block->output_ids, result->id)) {
#if 0
        if (XTENSAIsFixedRegister((XTENSAOpcode)result->opcode)) {
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
static void RemoveUnusedExpressions(XTENSAGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, RemoveBlockUnusedExpressions,
                              kTraversePostOrder, &data);
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
  XTENSAGenerator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    TrapCombineLoadStore(inst);

    if (XTENSAIsLoad(inst)) {
      TargetInstruction* base = inst->operand[0];
      if (base->opcode == (TargetOpcode)XTENSA_OP(la) && !compiler->pic &&
          XTENSAIntValue(inst->operand[1]) == 0 && base->users.length == 1) {
        // Fold the la's %pcrel_lo into the load.  Only safe when the load
        // reads exactly the symbol (offset 0 -- %pcrel_lo cannot carry an
        // extra constant) and the la feeds nothing else.  A la shared by
        // several loads/stores (e.g. copying a multi-word struct) must keep
        // materializing the full address: folding would replace this access's
        // offset with the relocation and leave the other accesses reading from
        // the bare auipc (high bits only).
        // Insert label for auipc instruction.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)XTENSA_OP(label));
        label->flags = XTENSA_EXPORTED_LABEL;
        TargetBasicBlockEmitBefore(&rv->base, block, label, base);
        base->opcode = (TargetOpcode)XTENSA_OP(auipc);
        base->flags |= XTENSA_PCREL_HI_RELOC;
        TargetReplaceOperand(inst, 1, label);
        inst->flags |= XTENSA_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)XTENSA_OP(addi)) {
        // Load from an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = XTENSAIntValue(inst->operand[1]);
        int immed = XTENSAIntValue(base->operand[1]);
        if (XTENSAIsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 0, base->operand[0]);
          TargetReplaceOperand(
              inst, 1,
              TargetGetIntConstant(&rv->base, NULL, kTargetType32Bit,
                                   offset + immed));
          // Only drop the addi if nothing else needs it.  A non-NULL dest
          // means the addi also writes a variable register that may be read
          // elsewhere, so removing it would leave that register undefined.
          if (base->users.length == 0 && base->dest == NULL) {
            TrapRemoveInstruction(base);
            TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
        }
      }
    } else if (XTENSAIsStore(inst)) {
      TargetInstruction* base = inst->operand[1];
      if (base->opcode == (TargetOpcode)XTENSA_OP(la) && !compiler->pic &&
          XTENSAIntValue(inst->operand[2]) == 0 && base->users.length == 1) {
        // Fold only for a zero-offset store whose la feeds nothing else; see
        // the load case above for why a nonzero offset or a shared la base
        // (e.g. a multi-word struct copy) must not be folded.
        // Insert label for auipc instruction.  Use the basic-block-aware
        // helper (as in the load case): inserting via the raw list helper
        // fails to update block->code when the auipc is the first instruction
        // in its block, which orphans the label so it is never emitted and
        // leaves the paired %pcrel_lo relocation dangling.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)XTENSA_OP(label));
        label->flags = XTENSA_EXPORTED_LABEL;
        TargetBasicBlockEmitBefore(&rv->base, block, label, base);
        base->opcode = (TargetOpcode)XTENSA_OP(auipc);
        base->flags |= XTENSA_PCREL_HI_RELOC;
        TargetReplaceOperand(inst, 2, label);
        inst->flags |= XTENSA_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)XTENSA_OP(addi)) {
        // Store to an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = XTENSAIntValue(inst->operand[2]);
        int immed = XTENSAIntValue(base->operand[1]);
        if (XTENSAIsPossibleImmediate(offset + immed)) {
          TargetReplaceOperand(inst, 1, base->operand[0]);
          TargetReplaceOperand(
              inst, 2,
              TargetGetIntConstant(&rv->base, NULL, kTargetType32Bit,
                                   offset + immed));
          // See the load case above: keep the addi if it also defines a
          // variable register (non-NULL dest) that may be read elsewhere.
          if (base->users.length == 0 && base->dest == NULL) {
            TrapRemoveInstruction(base);
            TargetBasicBlockRemoveInstruction(&rv->base, base->block, base);
          }
        }
      }
    }
  }
}

static void CombineLoadOrStores(XTENSAGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, CombineLoadOrStoresInBlock,
                              kTraversePreOrder, &data);
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
  XTENSAGenerator* rv = opt_data->rv;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    TrapPropagateZero(inst);
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (operand != NULL) {
        if (((int)operand->opcode == (int)XTENSA_OP(mv)) &&
            operand->users.length == 1) {
          TargetInstruction* mv = operand;
          if (mv->operand[0]->opcode == (TargetOpcode)XTENSA_OP(x0)) {
            // Found mv xx, x0.  Replace instruction operand with x0.
            TargetReplaceOperand(inst, i, mv->operand[0]);

            // We can now eliminate the mv instruction.
            TrapRemoveInstruction(mv);
            TargetBasicBlockRemoveInstruction(&rv->base, block, mv);
          }
        }
      }
    }
  }
}

static void PropagateZeroes(XTENSAGenerator* rv) {
  struct OptimizerData data = {rv};
  TargetTraverseDominatorTree(&rv->base, PropagateZeroesInBlock,
                              kTraversePostOrder, &data);
}

#if 0
static bool CanPoolConstant(TargetInstruction* inst, int op) {
  XTENSAOpcode opcode = (XTENSAOpcode)inst->opcode;
  if (XTENSAIsLoad(opcode)) {
    // Loads can't use a pooled constant.
    return false;
  }
  if (XTENSAIsStore(opcode)) {
    // Store can only store a pooled constant (not as offset)
    return op == 0;
  }

  switch (opcode) {
    case XTENSA_OP(li):
      return false;
      // Instructions with immediate operands.  Can use pooled constant
      // only for first operand.
    case XTENSA_OP(addi):
    case XTENSA_OP(slti):
    case XTENSA_OP(sltiu):
    case XTENSA_OP(xori):
    case XTENSA_OP(ori):
    case XTENSA_OP(andi):
    case XTENSA_OP(slli):
    case XTENSA_OP(srli):
    case XTENSA_OP(srai):
    case  XTENSA_OP(addiw):
    case  XTENSA_OP(slliw):
    case  XTENSA_OP(srliw):
    case  XTENSA_OP(sraiw):
      return op != 1;
    default:
      return true;
  }
}
#endif

typedef struct {
  XTENSAGenerator* rv;
  Map pool;  // Key: int64_t constant, value: instruction.
} ConstantPooler;

static void PoolConstantsInBlock(TargetBasicBlock* block, void* data) {
  ConstantPooler* pooler = data;
  XTENSAGenerator* rv = pooler->rv;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    TrapPoolConstant(inst);

    if (inst->opcode == (TargetOpcode)XTENSA_OP(li) && inst->dest == NULL) {
      // Possible instruction to pool.
      int64_t value = XTENSAIntValue(inst->operand[0]);
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

static COMPILER_UNUSED void PoolConstants(XTENSAGenerator* rv) {
  ConstantPooler pooler = {rv};
  MapInitForInt64Keys(&pooler.pool);
  PoolConstantsInBlock(rv->base.entry_block, &pooler);
  MapDestruct(&pooler.pool);
}

static void EliminateMovesInBlock(TargetBasicBlock* block, void* data) {
  XTENSAGenerator* rv = data;
  TargetInstruction* next;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    if (inst->opcode == (TargetOpcode)XTENSA_OP(mv)) {
      TargetInstruction* inst_dest = inst->dest;
      if (inst_dest == NULL) {
        continue;
      }
      TargetInstruction* inst_src = inst->operand[0];
      TargetInstruction* prev_prev;
      for (TargetInstruction* prev = TargetPrev(inst);
           prev != NULL && TargetNext(prev) != block->code; prev = prev_prev) {
        prev_prev = TargetPrev(prev);
        if (prev->dest == inst_src || inst->dest == inst_dest) {
          // src or dest haveu been assigned to, not candidate.
          continue;
        }
        if (XTENSAIsVarRegister(inst_dest) && XTENSAIsExpression(prev) &&
            prev->users.length == 1) {
          // rmov an expression to a register, just retarget the
          // expression to the register.  We use rmov to assign to a
          // register variable so we don't eliminate that.
          prev->dest = inst_dest;
          TrapRemoveInstruction(inst);
          TargetBasicBlockRemoveInstruction(&rv->base, block, inst);
        } else if (prev->opcode == (TargetOpcode)XTENSA_OP(mv)) {
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

static void EliminateMoves(XTENSAGenerator* rv) {
  TargetTraverseDominatorTree(&rv->base, EliminateMovesInBlock,
                              kTraversePostOrder, rv);
}

void XTENSAOptimize(XTENSAGenerator* rv) {
  // Eliminate moves if we can.
  EliminateMoves(rv);

  // XTENSAPrintBasicBlocks(rv, stdout);

  // Remove all unused expressions.
  RemoveUnusedExpressions(rv);
  // XTENSAPrintBasicBlocks(rv, stdout);

  // A load or store from an address calculated using the 'la' instruction can
  // be combined with the 'la'.
  CombineLoadOrStores(rv);
  // XTENSAPrintBasicBlocks(rv, stdout);

  // x0 is always available as a source register.  Look for single-use mv
  // instructions from x0 and propagate x0 to the destination.
  PropagateZeroes(rv);
  // XTENSAPrintBasicBlocks(rv, stdout);

  // Pool multi-use constants as instructions.
  //
  // Disabled: pooling a constant forces it to live in a register across all
  // its uses.  When those uses straddle a loop and/or a call (e.g. the same
  // constants used to initialize two arrays around a loop), it keeps many
  // values live simultaneously and drives the allocator into spilling it
  // cannot always satisfy correctly, clobbering live values such as a loop
  // bound.  On RISC-V32 re-materializing a constant is cheap, so the pooling
  // pays for itself rarely and is not worth the correctness risk.
  // PoolConstants(rv);
  // XTENSAPrintBasicBlocks(rv, stdout);

  // XTENSAPrintBasicBlocks(rv, stdout);
  // Rebuild the basic block input and outputs.
  TargetBuildBasicBlockInputsAndOutputs(&rv->base);
}
