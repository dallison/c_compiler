//
//  aarch64_optimize.c
//  c_compiler
//
//  Conservative target-instruction peepholes run before register allocation.
//

#include "aarch64_optimize.h"

#include <assert.h>

#include "aarch64_codegen.h"
#include "target_basic_block.h"

static bool AARCH64HasImplicitEffect(TargetInstruction* inst) {
  if (inst->observable_checkpoint) {
    return true;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    // Condition flags are not represented by target-instruction operands.
    case AARCH64_OP(cmp):
    case AARCH64_OP(cmn):
    case AARCH64_OP(tst):
    case AARCH64_OP(fcmp):
    case AARCH64_OP(ccmp):
    case AARCH64_OP(ccmn):
    case AARCH64_OP(adds):
    case AARCH64_OP(adcs):
    case AARCH64_OP(subs):
    case AARCH64_OP(sbcs):
    case AARCH64_OP(negs):
    case AARCH64_OP(ngcs):
    case AARCH64_OP(ands):
    case AARCH64_OP(bics):
    case AARCH64_OP(eons):
      return true;

    // Stores and atomic operations are observable even when their register
    // result is unused.  Most stores are already non-expressions, but keep the
    // complete set here so new opcode classifications cannot make DCE unsafe.
    case AARCH64_OP(fstr):
    case AARCH64_OP(stxr):
    case AARCH64_OP(stlxr):
    case AARCH64_OP(ldxr):
    case AARCH64_OP(ldaxr):
    case AARCH64_OP(ldar):
    case AARCH64_OP(stlr):
    case AARCH64_OP(dmb):
    case AARCH64_OP(clrex):
    case AARCH64_OP(atomic_load):
    case AARCH64_OP(atomic_store):
    case AARCH64_OP(atomic_fetch_add):
    case AARCH64_OP(atomic_fetch_sub):
    case AARCH64_OP(atomic_add_fetch):
    case AARCH64_OP(atomic_sub_fetch):
    case AARCH64_OP(atomic_compare_exchange_bool):
    case AARCH64_OP(atomic_compare_exchange_val):
    case AARCH64_OP(atomic_compare_exchange_n):
    case AARCH64_OP(atomic_fence):
    case AARCH64_OP(nrvoval):
      return true;
    default:
      return false;
  }
}

static int InstructionSize(TargetInstruction* inst) {
  return (inst->flags >> 16) & 3;
}

static bool IsMove(TargetInstruction* inst) {
  return inst != NULL &&
         (inst->opcode == (TargetOpcode)AARCH64_OP(mov) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(mv));
}

static int MoveSourceIndex(TargetInstruction* move) {
  if (move->opcode == (TargetOpcode)AARCH64_OP(mv) &&
      move->operand[1] != NULL) {
    return 1;
  }
  return 0;
}

static TargetInstruction* MoveSource(TargetInstruction* move) {
  return move->operand[MoveSourceIndex(move)];
}

static bool WritesRegisterOperand(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(mv):
    case AARCH64_OP(mov):
    case AARCH64_OP(fmv_s):
    case AARCH64_OP(fmv_d):
    case AARCH64_OP(fmov):
      return inst->operand[1] != NULL;
    default:
      return false;
  }
}

static bool IsUserOf(TargetInstruction* value, TargetInstruction* inst) {
  for (size_t i = 0; i < value->users.length; i++) {
    if (value->users.value.p[i] == inst) {
      return true;
    }
  }
  return false;
}

static bool WritesFixedValue(TargetInstruction* inst,
                             TargetInstruction* value) {
  return inst->dest == value ||
         (WritesRegisterOperand(inst) && inst->operand[0] == value);
}

static bool IsCoalescibleIncomingVariable(TargetInstruction* inst) {
  if (!AARCH64IsVarRegister(inst)) {
    return false;
  }
  Symbol* symbol = ((TargetSymbol*)inst)->symbol;
  if (symbol == NULL || symbol->type == NULL) {
    return false;
  }
  TypeRecordCalculateSize(symbol->type);
  return symbol->type->size == 8 ||
         (symbol->type->size == 4 && TypeIsIntegral(symbol->type));
}

static bool HasUnusedVariableDestination(TargetInstruction* inst) {
  if (inst->dest == NULL ||
      !AARCH64IsVarRegister(inst->dest) ||
      inst->dest->users.length != 0) {
    return false;
  }
  Symbol* symbol = ((TargetSymbol*)inst->dest)->symbol;
  return symbol != NULL && symbol->type != NULL &&
         !symbol->flags.address_taken && !TypeIsVolatile(symbol->type) &&
         !TypeIsArray(symbol->type) &&
         (symbol->flags.is_local || symbol->flags.is_temp ||
          symbol->flags.is_argument);
}

static bool IsUnusedVariableMove(TargetInstruction* move) {
  return IsMove(move) && HasUnusedVariableDestination(move);
}

// A true leaf can consume an incoming ABI register directly when the value has
// one definition and remains in the entry block.  Argument registers are
// reserved throughout that block by the allocator, while x8 is outside the
// general allocation pool.  Reject any explicit overwrite before the final
// use so this remains a local, allocation-independent coalescing decision.
static bool EliminateLeafIncomingMove(AARCH64Generator* generator,
                                      TargetBasicBlock* block,
                                      TargetInstruction* move) {
  if (block != generator->base.entry_block ||
      generator->base.num_calls != 0 ||
      generator->exception_ranges.length != 0 || !IsMove(move) ||
      move->dest == NULL ||
      (!AARCH64IsVarRegister(move->dest) &&
       move->dest->opcode != (TargetOpcode)AARCH64_OP(structreturn))) {
    return false;
  }

  TargetInstruction* source = MoveSource(move);
  bool incoming_argument =
      source != NULL && (source->flags & TARGET_INST_INCOMING_ARG) != 0 &&
      IsCoalescibleIncomingVariable(move->dest);
  bool indirect_result =
      source != NULL &&
      source->opcode == (TargetOpcode)AARCH64_OP(xr) &&
      move->dest->opcode == (TargetOpcode)AARCH64_OP(structreturn);
  int source_size = source != NULL ? InstructionSize(source) : 0;
  int move_size = InstructionSize(move);
  if ((!incoming_argument && !indirect_result) ||
      (source_size != 0 && move_size != 0 && source_size != move_size) ||
      move->dest->users.length == 0) {
    return false;
  }

  // The destination must represent exactly this assignment.  A shared
  // variable-register pseudo with another definition cannot be coalesced.
  for (TargetInstruction* inst = TargetFirstInstruction(&generator->base);
       inst != NULL; inst = TargetNext(inst)) {
    if (inst != move && inst->dest == move->dest) {
      return false;
    }
  }

  size_t remaining_users = move->dest->users.length;
  for (TargetInstruction* inst = TargetNext(move);
       inst != NULL && inst->block == block; inst = TargetNext(inst)) {
    if (IsUserOf(move->dest, inst)) {
      remaining_users--;
      if (remaining_users == 0) {
        break;
      }
    }
    if (WritesFixedValue(inst, source) ||
        inst->opcode == (TargetOpcode)AARCH64_OP(asm)) {
      return false;
    }
  }
  if (remaining_users != 0) {
    return false;
  }

  while (move->dest->users.length != 0) {
    TargetInstruction* user = move->dest->users.value.p[0];
    for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
      if (user->operand[i] == move->dest) {
        TargetReplaceOperand(user, (int)i, source);
      }
    }
  }
  TargetBasicBlockRemoveInstruction(&generator->base, block, move);
  return true;
}

// Fold only adjacent, single-use move chains.  The adjacency and width checks
// avoid extending a value's live range or dropping a meaningful 32/64-bit
// truncation.
static bool EliminateMovesInBlock(AARCH64Generator* generator,
                                  TargetBasicBlock* block) {
  bool changed = false;
  bool local_change;
  do {
    local_change = false;
    for (TargetInstruction* inst = TargetBasicBlockBegin(block);
         !TargetBasicBlockIsEmpty(block) &&
         inst != TargetBasicBlockEnd(block);
         inst = TargetNext(inst)) {
      if (!IsMove(inst) || inst->dest == NULL) {
        continue;
      }
      if (IsUnusedVariableMove(inst)) {
        TargetBasicBlockRemoveInstruction(&generator->base, block, inst);
        local_change = true;
        changed = true;
        break;
      }
      if (EliminateLeafIncomingMove(generator, block, inst)) {
        local_change = true;
        changed = true;
        break;
      }
      TargetInstruction* prev = TargetPrev(inst);
      if (prev == NULL || prev->block != block ||
          InstructionSize(prev) != InstructionSize(inst)) {
        continue;
      }

      TargetInstruction* source = MoveSource(inst);
      if (IsMove(prev) && prev->dest == source &&
          source != NULL && source->users.length == 1) {
        TargetReplaceOperand(inst, MoveSourceIndex(inst), MoveSource(prev));
        TargetBasicBlockRemoveInstruction(&generator->base, block, prev);
        local_change = true;
        changed = true;
        break;
      }

      if (prev == source && prev->dest == NULL &&
          prev->users.length == 1 && AARCH64IsExpression(prev) &&
          !AARCH64HasImplicitEffect(prev) &&
          !AARCH64IsFixedRegister(prev) && !AARCH64IsConst(prev) &&
          !AARCH64IsSymbol(prev) && !AARCH64IsVarRegister(prev) &&
          !AARCH64IsResult(prev) &&
          prev->opcode != (TargetOpcode)AARCH64_OP(tmp) &&
          prev->opcode != (TargetOpcode)AARCH64_OP(reload) &&
          AARCH64IsVarRegister(inst->dest)) {
        prev->dest = inst->dest;
        TargetBasicBlockRemoveInstruction(&generator->base, block, inst);
        local_change = true;
        changed = true;
        break;
      }
    }
  } while (local_change);
  return changed;
}

static void EliminateMovesInBlockCallback(TargetBasicBlock* block, void* data) {
  EliminateMovesInBlock(data, block);
}

static void EliminateMoves(AARCH64Generator* generator) {
  TargetTraverseDominatorTree(&generator->base,
                              EliminateMovesInBlockCallback,
                              kTraversePostOrder, generator);
}

static void RemoveBlockUnusedExpressions(TargetBasicBlock* block, void* data) {
  AARCH64Generator* generator = data;
  bool changed;
  do {
    changed = false;
    TargetInstruction* prev = NULL;
    for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
         !TargetBasicBlockIsEmpty(block) &&
         inst != TargetBasicBlockREnd(block);
         inst = prev) {
      prev = TargetPrev(inst);
      AARCH64Opcode opcode = (AARCH64Opcode)inst->opcode;
      if ((inst->dest == NULL || HasUnusedVariableDestination(inst)) &&
          inst->users.length == 0 &&
          AARCH64IsExpression(inst) &&
          !AARCH64HasImplicitEffect(inst) &&
          !WritesRegisterOperand(inst) &&
          !AARCH64IsSymbol(inst) && !AARCH64IsConst(inst) &&
          !AARCH64IsFixedRegister(inst) &&
          opcode != AARCH64_OP(tmp) && opcode != AARCH64_OP(sp) &&
          (inst->flags & TARGET_INST_KEEP_UNREACHABLE) == 0) {
        TargetBasicBlockRemoveInstruction(&generator->base, block, inst);
        changed = true;
      }
    }
  } while (changed);
}

static void RemoveUnusedExpressions(AARCH64Generator* generator) {
  TargetTraverseDominatorTree(&generator->base,
                              RemoveBlockUnusedExpressions,
                              kTraversePostOrder, generator);
}

static bool IsAddressAdd(TargetInstruction* inst) {
  return inst != NULL &&
         inst->opcode == (TargetOpcode)AARCH64_OP(add) &&
         inst->dest == NULL && inst->operand[0] != NULL &&
         inst->operand[1] != NULL &&
         AARCH64IsIntConst(inst->operand[1]) &&
         inst->users.length == 1;
}

static bool IsFoldableLoad(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ldr):
    case AARCH64_OP(ldur):
    case AARCH64_OP(ldrb):
    case AARCH64_OP(ldrh):
    case AARCH64_OP(ldurb):
    case AARCH64_OP(ldurh):
    case AARCH64_OP(ldrsb):
    case AARCH64_OP(ldrsh):
    case AARCH64_OP(ldursb):
    case AARCH64_OP(ldursh):
    case AARCH64_OP(ldursw):
    case AARCH64_OP(fldr):
      return true;
    default:
      return false;
  }
}

static bool IsFoldableStore(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(str):
    case AARCH64_OP(stur):
    case AARCH64_OP(strb):
    case AARCH64_OP(strh):
    case AARCH64_OP(sturb):
    case AARCH64_OP(sturh):
    case AARCH64_OP(fstr):
      return true;
    default:
      return false;
  }
}

static bool LoadStoreOffsetInRange(int64_t offset) {
  return offset >= -256 && offset <= 255;
}

static void FoldAddressAdd(AARCH64Generator* generator,
                           TargetInstruction* memory,
                           int base_index, int offset_index) {
  TargetInstruction* base = memory->operand[base_index];
  TargetInstruction* offset = memory->operand[offset_index];
  if (!IsAddressAdd(base) || offset == NULL ||
      !AARCH64IsIntConst(offset)) {
    return;
  }
  int64_t combined =
      (int64_t)AARCH64IntValue(offset) +
      (int64_t)AARCH64IntValue(base->operand[1]);
  if (!LoadStoreOffsetInRange(combined)) {
    return;
  }
  TargetReplaceOperand(memory, base_index, base->operand[0]);
  TargetReplaceOperand(
      memory, offset_index,
      TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, combined));
  TargetBasicBlockRemoveInstruction(&generator->base, base->block, base);
}

static void CombineLoadOrStoresInBlock(TargetBasicBlock* block, void* data) {
  AARCH64Generator* generator = data;
  TargetInstruction* prev = NULL;
  for (TargetInstruction* inst = TargetBasicBlockRBegin(block);
       !TargetBasicBlockIsEmpty(block) &&
       inst != TargetBasicBlockREnd(block);
       inst = prev) {
    prev = TargetPrev(inst);
    if (IsFoldableLoad(inst)) {
      FoldAddressAdd(generator, inst, 0, 1);
    } else if (IsFoldableStore(inst)) {
      FoldAddressAdd(generator, inst, 1, 2);
    }
  }
}

static void CombineLoadOrStores(AARCH64Generator* generator) {
  TargetTraverseDominatorTree(&generator->base,
                              CombineLoadOrStoresInBlock,
                              kTraversePreOrder, generator);
}

static void AddOperand(TargetInstruction* inst, int index,
                       TargetInstruction* operand) {
  assert(inst->operand[index] == NULL);
  inst->operand[index] = operand;
  TargetAddUser(operand, inst);
}

static void CombineShiftedAddsInBlock(TargetBasicBlock* block, void* data) {
  AARCH64Generator* generator = data;
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) &&
       inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    if (inst->opcode != (TargetOpcode)AARCH64_OP(add) ||
        inst->operand[2] != NULL || inst->operand[3] != NULL) {
      continue;
    }
    int shift_index = -1;
    for (int i = 0; i < 2; i++) {
      if (inst->operand[i] != NULL &&
          inst->operand[i]->opcode == (TargetOpcode)AARCH64_OP(lsl)) {
        shift_index = i;
        break;
      }
    }
    if (shift_index < 0) {
      continue;
    }
    TargetInstruction* shift = inst->operand[shift_index];
    if (shift->dest != NULL || shift->users.length != 1 ||
        shift->block != block || shift->operand[0] == NULL ||
        shift->operand[1] == NULL ||
        !AARCH64IsIntConst(shift->operand[1]) ||
        (InstructionSize(shift) != 0 &&
         InstructionSize(shift) != InstructionSize(inst))) {
      continue;
    }
    int64_t amount = AARCH64IntValue(shift->operand[1]);
    int width = InstructionSize(inst) == kSize64Bit ? 64 : 32;
    if (amount < 0 || amount >= width) {
      continue;
    }

    TargetInstruction* base = inst->operand[1 - shift_index];
    // The shifted form is `add Rd, Rn, Rm, lsl #k`, so what the shift is added
    // to has to be a register.  Folding an immediate into Rn instead spells an
    // instruction that does not exist -- `add w0, #15, w1, lsl #2` -- and the
    // assembler rejects it.
    if (base == NULL || AARCH64IsIntConst(base)) {
      continue;
    }
    TargetInstruction* value = shift->operand[0];
    TargetInstruction* amount_inst = shift->operand[1];
    TargetReplaceOperand(inst, 0, base);
    TargetReplaceOperand(inst, 1, value);
    TargetInstruction* shift_marker =
        TargetNewInstruction((TargetOpcode)AARCH64_OP(oplsl));
    TargetBasicBlockEmitBefore(&generator->base, block, shift_marker, inst);
    AddOperand(inst, 2, shift_marker);
    AddOperand(inst, 3, amount_inst);
    TargetBasicBlockRemoveInstruction(&generator->base, block, shift);
  }
}

static void CombineShiftedAdds(AARCH64Generator* generator) {
  TargetTraverseDominatorTree(&generator->base,
                              CombineShiftedAddsInBlock,
                              kTraversePreOrder, generator);
}

void AARCH64Optimize(AARCH64Generator* generator) {
  EliminateMoves(generator);
  RemoveUnusedExpressions(generator);
  CombineLoadOrStores(generator);
  CombineShiftedAdds(generator);
  TargetBuildBasicBlockInputsAndOutputs(&generator->base);
}
