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

static bool InstructionWritesOpcode(TargetInstruction* inst,
                                    AARCH64Opcode opcode) {
  if (inst->dest != NULL && inst->dest->opcode == (TargetOpcode)opcode) {
    return true;
  }
  return WritesRegisterOperand(inst) && inst->operand[0] != NULL &&
         inst->operand[0]->opcode == (TargetOpcode)opcode;
}

// Incoming x0 is the AAPCS64 integer/pointer result register; incoming v0
// is the FP result register.  The peephole treats those as distinct
// TargetInstructions from resulti/resultf/resultd, so a later store of the
// return value looks like a different write and would otherwise clobber a
// still-live arg0 (`this` in ios_base::setf, for example).
static bool WritesAliasedReturnRegister(TargetInstruction* incoming,
                                        TargetInstruction* inst) {
  if (incoming == NULL) {
    return false;
  }
  if (incoming->opcode == (TargetOpcode)AARCH64_OP(r0)) {
    return InstructionWritesOpcode(inst, AARCH64_OP(resulti));
  }
  if (incoming->opcode == (TargetOpcode)AARCH64_OP(d0)) {
    return InstructionWritesOpcode(inst, AARCH64_OP(resultf)) ||
           InstructionWritesOpcode(inst, AARCH64_OP(resultd));
  }
  return false;
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
        inst->opcode == (TargetOpcode)AARCH64_OP(asm) ||
        WritesAliasedReturnRegister(source, inst)) {
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
// truncation.  An unspecified size (0) is treated as compatible with either
// width so a correctly-sized staging copy can still collapse into a later
// move whose destination was left unsized (fixed arg-register pseudos).
static bool MoveSizesCompatible(TargetInstruction* a, TargetInstruction* b) {
  int size_a = InstructionSize(a);
  int size_b = InstructionSize(b);
  return size_a == 0 || size_b == 0 || size_a == size_b;
}

static void InheritMoveSize(TargetInstruction* dest_move,
                            TargetInstruction* src_move) {
  if (InstructionSize(dest_move) == 0 && InstructionSize(src_move) != 0) {
    SetInstructionSize(dest_move, InstructionSize(src_move));
  }
}

// Incoming and outgoing ABI register pseudos are distinct TargetInstructions
// that share an opcode (separate r0() for the parameter vs. the call arg).
// Treat equal fixed-register opcodes as the same physical register so
// staging round-trips can collapse to a true identity and be deleted.
static bool SameFixedRegister(TargetInstruction* a, TargetInstruction* b) {
  if (a == b) {
    return true;
  }
  return a != NULL && b != NULL && AARCH64IsFixedRegister(a) &&
         AARCH64IsFixedRegister(b) && a->opcode == b->opcode;
}

// Debug locs and abandoned staging temps sit between a defining copy and its
// single user after other folds; they do not change register dataflow.
static bool IsTransparentMoveFoldGap(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  AARCH64Opcode opcode = (AARCH64Opcode)inst->opcode;
  if (opcode == AARCH64_OP(loc)) {
    return true;
  }
  return opcode == AARCH64_OP(tmp) && inst->users.length == 0;
}

static TargetInstruction* PreviousSignificantInstruction(
    TargetBasicBlock* block, TargetInstruction* inst) {
  for (TargetInstruction* prev = TargetPrev(inst);
       prev != NULL && prev->block == block; prev = TargetPrev(prev)) {
    if (!IsTransparentMoveFoldGap(prev)) {
      return prev;
    }
  }
  return NULL;
}

static void RemoveInstructionAndDeadTempDest(AARCH64Generator* generator,
                                             TargetBasicBlock* block,
                                             TargetInstruction* inst) {
  TargetInstruction* dest = inst->dest;
  TargetBasicBlockRemoveInstruction(&generator->base, block, inst);
  if (dest != NULL && dest->opcode == (TargetOpcode)AARCH64_OP(tmp) &&
      dest->users.length == 0 && dest->block == block) {
    TargetBasicBlockRemoveInstruction(&generator->base, block, dest);
  }
}

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
        RemoveInstructionAndDeadTempDest(generator, block, inst);
        local_change = true;
        changed = true;
        break;
      }
      if (EliminateLeafIncomingMove(generator, block, inst)) {
        local_change = true;
        changed = true;
        break;
      }
      TargetInstruction* source = MoveSource(inst);
      // Identity copies left after forwarding a call-arg staging chain
      // (mov rN, rN), including when the source and dest are distinct
      // incoming/outgoing fixed-register pseudos for the same ABI register.
      if (SameFixedRegister(source, inst->dest)) {
        TargetBasicBlockRemoveInstruction(&generator->base, block, inst);
        local_change = true;
        changed = true;
        break;
      }
      TargetInstruction* prev =
          PreviousSignificantInstruction(block, inst);
      if (prev != NULL && MoveSizesCompatible(prev, inst)) {
        if (IsMove(prev) && prev->dest == source &&
            source != NULL && source->users.length == 1) {
          TargetReplaceOperand(inst, MoveSourceIndex(inst), MoveSource(prev));
          InheritMoveSize(inst, prev);
          RemoveInstructionAndDeadTempDest(generator, block, prev);
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

      // Call-arg staging emits all temps first, then the ABI moves, so a
      // temp's defining copy is often separated from its only user by an
      // unrelated argument move.  Forward when the forwarded source is not
      // written between the definition and this use.
      if (source != NULL &&
          source->opcode == (TargetOpcode)AARCH64_OP(tmp) &&
          source->users.length == 1 && source->block == block) {
        TargetInstruction* def = NULL;
        for (TargetInstruction* p = TargetBasicBlockBegin(block);
             p != NULL && p != inst; p = TargetNext(p)) {
          if (p->dest == source) {
            def = IsMove(p) ? p : NULL;
          }
        }
        if (def != NULL && MoveSizesCompatible(def, inst)) {
          TargetInstruction* forwarded = MoveSource(def);
          bool clobbered = false;
          for (TargetInstruction* p = TargetNext(def); p != NULL && p != inst;
               p = TargetNext(p)) {
            if (WritesFixedValue(p, forwarded) ||
                SameFixedRegister(p->dest, forwarded) ||
                WritesAliasedReturnRegister(forwarded, p) ||
                (WritesRegisterOperand(p) &&
                 SameFixedRegister(p->operand[0], forwarded)) ||
                (AARCH64IsFixedRegister(forwarded) &&
                 (AARCH64IsCall(p) ||
                  p->opcode == (TargetOpcode)AARCH64_OP(asm))) ||
                (AARCH64IsCall(forwarded) &&
                 (AARCH64IsCall(p) ||
                  p->opcode == (TargetOpcode)AARCH64_OP(asm) ||
                  (p->dest != NULL &&
                   (AARCH64IsFixedRegister(p->dest) ||
                    AARCH64IsResult(p->dest)))))) {
              clobbered = true;
              break;
            }
          }
          if (!clobbered) {
            TargetReplaceOperand(inst, MoveSourceIndex(inst), forwarded);
            InheritMoveSize(inst, def);
            RemoveInstructionAndDeadTempDest(generator, block, def);
            local_change = true;
            changed = true;
            break;
          }
        }
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
  // Do not require a single user: RMW (`s->x += n`) shares one address add
  // between a load and a store.  Folding each memory op independently still
  // leaves a correct [base, #imm] form, and the add is removed once its last
  // user is rewritten.
  return inst != NULL &&
         inst->opcode == (TargetOpcode)AARCH64_OP(add) &&
         inst->dest == NULL && inst->operand[0] != NULL &&
         inst->operand[1] != NULL &&
         AARCH64IsIntConst(inst->operand[1]);
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

// Bytes moved by a load or store, which scales its unsigned offset field.
// Mirrors the size the program emitter derives for the encoding.
static int AccessBytes(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ldrb):
    case AARCH64_OP(ldurb):
    case AARCH64_OP(ldrsb):
    case AARCH64_OP(ldursb):
    case AARCH64_OP(strb):
    case AARCH64_OP(sturb):
      return 1;
    case AARCH64_OP(ldrh):
    case AARCH64_OP(ldurh):
    case AARCH64_OP(ldrsh):
    case AARCH64_OP(ldursh):
    case AARCH64_OP(strh):
    case AARCH64_OP(sturh):
      return 2;
    case AARCH64_OP(ldursw):
      return 4;
    default:
      break;
  }
  switch (InstructionSize(inst)) {
    case kSize128Bit:
      return 16;
    case kSize32Bit:
      return 4;
    default:
      return 8;
  }
}

static bool IsUnscaledOnly(TargetInstruction* inst) {
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ldur):
    case AARCH64_OP(ldurb):
    case AARCH64_OP(ldurh):
    case AARCH64_OP(ldursb):
    case AARCH64_OP(ldursh):
    case AARCH64_OP(ldursw):
    case AARCH64_OP(stur):
    case AARCH64_OP(sturb):
    case AARCH64_OP(sturh):
      return true;
    default:
      return false;
  }
}

// The signed 9-bit unscaled form, or the unsigned 12-bit form scaled by the
// access size.
static bool LoadStoreOffsetInRange(TargetInstruction* memory, int64_t offset) {
  if (offset >= -256 && offset <= 255) {
    return true;
  }
  if (IsUnscaledOnly(memory)) {
    return false;
  }
  int bytes = AccessBytes(memory);
  return offset >= 0 && offset % bytes == 0 && offset / bytes <= 0xfff;
}

static bool IsRedefinableValue(TargetInstruction* value) {
  if (value == NULL || value->opcode == (TargetOpcode)AARCH64_OP(fp) ||
      value->opcode == (TargetOpcode)AARCH64_OP(zr)) {
    return false;
  }
  if (value->opcode >= (TargetOpcode)AARCH64_OP(d0) &&
      value->opcode <= (TargetOpcode)AARCH64_OP(d7)) {
    return true;
  }
  return AARCH64IsVarRegister(value) || AARCH64IsFixedRegister(value) ||
         AARCH64IsResult(value) || AARCH64IsCall(value) ||
         value->opcode == (TargetOpcode)AARCH64_OP(tmp) ||
         value->dest != NULL;
}

static bool ClobbersAARCH64ForwardedValue(TargetInstruction* p,
                                          TargetInstruction* forwarded) {
  return WritesFixedValue(p, forwarded) ||
         SameFixedRegister(p->dest, forwarded) ||
         WritesAliasedReturnRegister(forwarded, p) ||
         (WritesRegisterOperand(p) &&
          SameFixedRegister(p->operand[0], forwarded)) ||
         ((AARCH64IsFixedRegister(forwarded) ||
           (forwarded->opcode >= (TargetOpcode)AARCH64_OP(d0) &&
            forwarded->opcode <= (TargetOpcode)AARCH64_OP(d7))) &&
          (AARCH64IsCall(p) || p->opcode == (TargetOpcode)AARCH64_OP(asm))) ||
         (AARCH64IsCall(forwarded) &&
          (AARCH64IsCall(p) || p->opcode == (TargetOpcode)AARCH64_OP(asm) ||
           (p->dest != NULL &&
            (AARCH64IsFixedRegister(p->dest) || AARCH64IsResult(p->dest)))));
}

static bool ValueUnclobberedBetweenGen(TargetGenerator* gen,
                                       TargetInstruction* def,
                                       TargetInstruction* use,
                                       TargetInstruction* forwarded) {
  if (def == NULL || use == NULL || forwarded == NULL) {
    return false;
  }
  if (!IsRedefinableValue(forwarded)) {
    return true;
  }
  return TargetPathUnclobberedBetween(gen, def, use, forwarded,
                                      ClobbersAARCH64ForwardedValue);
}

static bool ValueUnclobberedBetween(TargetInstruction* def,
                                    TargetInstruction* use,
                                    TargetInstruction* forwarded) {
  return ValueUnclobberedBetweenGen(NULL, def, use, forwarded);
}

static bool FoldAddressAdd(AARCH64Generator* generator,
                           TargetInstruction* memory,
                           int base_index, int offset_index) {
  TargetInstruction* base = memory->operand[base_index];
  TargetInstruction* offset = memory->operand[offset_index];
  if (!IsAddressAdd(base) || offset == NULL ||
      !AARCH64IsIntConst(offset) ||
      !ValueUnclobberedBetweenGen(&generator->base, base, memory,
                                  base->operand[0])) {
    return false;
  }
  int64_t combined =
      (int64_t)AARCH64IntValue(offset) +
      (int64_t)AARCH64IntValue(base->operand[1]);
  if (!LoadStoreOffsetInRange(memory, combined)) {
    return false;
  }
  TargetReplaceOperand(memory, base_index, base->operand[0]);
  TargetReplaceOperand(
      memory, offset_index,
      TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, combined));
  if (base->users.length == 0) {
    TargetBasicBlockRemoveInstruction(&generator->base, base->block, base);
  }
  return true;
}

// add base, index [, lsl #n] used only to form [tmp, #0].  Fold into the
// register-offset form [base, index] (or scaled when n matches the access
// size).  Unscaled (ldur*) opcodes have no register-offset encoding.
static bool IsRegisterAddressAddOperand(TargetInstruction* op) {
  return op != NULL && !AARCH64IsConst(op) && !AARCH64IsSymbol(op) &&
         op->opcode != (TargetOpcode)AARCH64_OP(literal);
}

static bool IsRegisterAddressAdd(TargetInstruction* inst) {
  return inst != NULL &&
         inst->opcode == (TargetOpcode)AARCH64_OP(add) &&
         inst->dest == NULL &&
         IsRegisterAddressAddOperand(inst->operand[0]) &&
         IsRegisterAddressAddOperand(inst->operand[1]);
}

static int RegisterAddressAddShift(TargetInstruction* add) {
  if (add->operand[2] == NULL && add->operand[3] == NULL) {
    return 0;
  }
  if (add->operand[2] != NULL &&
      add->operand[2]->opcode == (TargetOpcode)AARCH64_OP(oplsl) &&
      add->operand[3] != NULL && AARCH64IsIntConst(add->operand[3])) {
    return (int)AARCH64IntValue(add->operand[3]);
  }
  return -1;
}

static int AccessSizeLog2(TargetInstruction* memory) {
  int bytes = AccessBytes(memory);
  int log2 = 0;
  while ((1 << log2) < bytes) {
    log2++;
  }
  return (1 << log2) == bytes ? log2 : -1;
}

static bool FoldAddressIndex(AARCH64Generator* generator,
                             TargetInstruction* memory,
                             int base_index, int offset_index) {
  TargetInstruction* base = memory->operand[base_index];
  TargetInstruction* offset = memory->operand[offset_index];
  if (!IsRegisterAddressAdd(base) || offset == NULL ||
      !AARCH64IsIntConst(offset) || AARCH64IntValue(offset) != 0 ||
      IsUnscaledOnly(memory) ||
      memory->opcode == (TargetOpcode)AARCH64_OP(fldr) ||
      memory->opcode == (TargetOpcode)AARCH64_OP(fstr) ||
      AccessBytes(memory) > 8) {
    return false;
  }
  int shift = RegisterAddressAddShift(base);
  int access_log2 = AccessSizeLog2(memory);
  if (shift < 0 || access_log2 < 0 ||
      (shift != 0 && shift != access_log2)) {
    return false;
  }
  // Keep only the unshifted form for now: scaled register offsets need an
  // extra shift marker on the load/store, and stores already use operand[2]
  // for the index.  shift==0 covers mem[i] / Rd-style byte accesses.
  if (shift != 0 ||
      !ValueUnclobberedBetweenGen(&generator->base, base, memory,
                                  base->operand[0]) ||
      !ValueUnclobberedBetweenGen(&generator->base, base, memory,
                                  base->operand[1])) {
    return false;
  }
  TargetReplaceOperand(memory, base_index, base->operand[0]);
  TargetReplaceOperand(memory, offset_index, base->operand[1]);
  if (base->users.length == 0) {
    TargetBasicBlockRemoveInstruction(&generator->base, base->block, base);
  }
  return true;
}

// strb/strh already truncates; ubfx/and masking the value first is dead when
// that mask is only feeding the store (BBC PC updates: add; uxth; strh).
static bool FoldTruncBeforeNarrowStore(AARCH64Generator* generator,
                                       TargetInstruction* store) {
  int bytes = AccessBytes(store);
  if (bytes != 1 && bytes != 2) {
    return false;
  }
  TargetInstruction* value = store->operand[0];
  if (value == NULL || value->users.length != 1) {
    return false;
  }
  TargetInstruction* src = NULL;
  if (value->opcode == (TargetOpcode)AARCH64_OP(ubfx) &&
      value->operand[0] != NULL && value->operand[1] != NULL &&
      value->operand[2] != NULL && AARCH64IsIntConst(value->operand[1]) &&
      AARCH64IsIntConst(value->operand[2]) &&
      AARCH64IntValue(value->operand[1]) == 0 &&
      AARCH64IntValue(value->operand[2]) == bytes * 8) {
    src = value->operand[0];
  } else if (value->opcode == (TargetOpcode)AARCH64_OP(and) &&
             value->operand[0] != NULL && value->operand[1] != NULL &&
             AARCH64IsIntConst(value->operand[1])) {
    int64_t mask = AARCH64IntValue(value->operand[1]);
    if ((bytes == 1 && mask == 0xff) || (bytes == 2 && mask == 0xffff)) {
      src = value->operand[0];
    }
  }
  if (src == NULL ||
      !ValueUnclobberedBetweenGen(&generator->base, value, store, src)) {
    return false;
  }
  TargetReplaceOperand(store, 0, src);
  if (value->users.length == 0) {
    RemoveInstructionAndDeadTempDest(generator, value->block, value);
  }
  return true;
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
      while (FoldAddressAdd(generator, inst, 0, 1) ||
             FoldAddressIndex(generator, inst, 0, 1)) {
      }
    } else if (IsFoldableStore(inst)) {
      while (FoldAddressAdd(generator, inst, 1, 2) ||
             FoldAddressIndex(generator, inst, 1, 2) ||
             FoldTruncBeforeNarrowStore(generator, inst)) {
      }
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
    // The shifted form of add names two registers, so it has nowhere to put
    // the immediate that the plain form would have taken.
    if (base == NULL || AARCH64IsIntConst(base)) {
      continue;
    }
    TargetInstruction* value = shift->operand[0];
    if (!ValueUnclobberedBetweenGen(&generator->base, shift, inst, value)) {
      continue;
    }
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

// cset does not clobber NZCV, so `cmp; cset; cbz` can become `cmp; cset; b.cc`
// when nothing between the cset and the cbz updates flags (or calls).
static bool ClobbersConditionFlags(TargetInstruction* inst) {
  if (inst == NULL) {
    return true;
  }
  if (AARCH64HasImplicitEffect(inst)) {
    return true;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
    case AARCH64_OP(b):
    case AARCH64_OP(br):
    case AARCH64_OP(ret):
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz):
    case AARCH64_OP(tbz):
    case AARCH64_OP(tbnz):
    case AARCH64_OP(asm):
      return true;
    default:
      return false;
  }
}

static AARCH64Opcode InvertCond(AARCH64Opcode cond) {
  switch (cond) {
    case AARCH64_OP(eq):
      return AARCH64_OP(ne);
    case AARCH64_OP(ne):
      return AARCH64_OP(eq);
    case AARCH64_OP(cs):
    case AARCH64_OP(hs):
      return AARCH64_OP(cc);
    case AARCH64_OP(cc):
    case AARCH64_OP(lo):
      return AARCH64_OP(cs);
    case AARCH64_OP(mi):
      return AARCH64_OP(pl);
    case AARCH64_OP(pl):
      return AARCH64_OP(mi);
    case AARCH64_OP(vs):
      return AARCH64_OP(vc);
    case AARCH64_OP(vc):
      return AARCH64_OP(vs);
    case AARCH64_OP(hi):
      return AARCH64_OP(ls);
    case AARCH64_OP(ls):
      return AARCH64_OP(hi);
    case AARCH64_OP(ge):
      return AARCH64_OP(lt);
    case AARCH64_OP(lt):
      return AARCH64_OP(ge);
    case AARCH64_OP(gt):
      return AARCH64_OP(le);
    case AARCH64_OP(le):
      return AARCH64_OP(gt);
    default:
      return AARCH64_OP(al);
  }
}

static TargetInstruction* PeelMovSource(TargetInstruction* value,
                                        TargetBasicBlock* block,
                                        TargetInstruction* use) {
  while (value != NULL && IsMove(value) && value->block == block) {
    TargetInstruction* src = MoveSource(value);
    if (!ValueUnclobberedBetween(value, use, src)) {
      break;
    }
    value = src;
  }
  return value;
}

// Resolve a cbz/cbnz operand to the cset that produced it (expression-style
// cset, or cset that wrote a tmp the branch reads).
static TargetInstruction* FindCsetProducer(TargetInstruction* value,
                                           TargetBasicBlock* block,
                                           TargetInstruction* use) {
  value = PeelMovSource(value, block, use);
  if (value == NULL || value->block != block) {
    return NULL;
  }
  if (value->opcode == (TargetOpcode)AARCH64_OP(cset)) {
    return value;
  }
  // ubfx (objdump: uxtb/uxth) of a cset — peel one layer.
  if (value->opcode == (TargetOpcode)AARCH64_OP(ubfx) &&
      value->operand[0] != NULL) {
    TargetInstruction* inner = PeelMovSource(value->operand[0], block, value);
    if (inner != NULL && inner->opcode == (TargetOpcode)AARCH64_OP(cset) &&
        inner->block == block) {
      return inner;
    }
  }
  // Branch operand is a tmp defined by cset in this block.
  if (value->opcode == (TargetOpcode)AARCH64_OP(tmp)) {
    for (TargetInstruction* inst = TargetBasicBlockBegin(block);
         inst != NULL && inst->block == block; inst = TargetNext(inst)) {
      if (inst->opcode == (TargetOpcode)AARCH64_OP(cset) &&
          inst->dest == value && ValueUnclobberedBetween(inst, use, value)) {
        return inst;
      }
    }
  }
  return NULL;
}

// cbz(ubfx(x,0,8/16)) → cbz(x): zero-extend does not change "is zero".
static bool FoldRedundantZextBeforeCompareZeroBranch(
    AARCH64Generator* generator, TargetBasicBlock* block,
    TargetInstruction* branch) {
  if (branch->opcode != (TargetOpcode)AARCH64_OP(cbz) &&
      branch->opcode != (TargetOpcode)AARCH64_OP(cbnz)) {
    return false;
  }
  TargetInstruction* value = PeelMovSource(branch->operand[0], block, branch);
  if (value == NULL || value->opcode != (TargetOpcode)AARCH64_OP(ubfx) ||
      value->operand[0] == NULL || value->operand[1] == NULL ||
      value->operand[2] == NULL || !AARCH64IsIntConst(value->operand[1]) ||
      !AARCH64IsIntConst(value->operand[2]) ||
      AARCH64IntValue(value->operand[1]) != 0 ||
      !ValueUnclobberedBetween(value, branch, value->operand[0])) {
    return false;
  }
  int64_t width = AARCH64IntValue(value->operand[2]);
  if (width != 8 && width != 16 && width != 32) {
    return false;
  }
  TargetReplaceOperand(branch, 0, value->operand[0]);
  if (value->users.length == 0) {
    RemoveInstructionAndDeadTempDest(generator, block, value);
  }
  return true;
}

static bool FoldCsetCompareZeroBranch(AARCH64Generator* generator,
                                      TargetBasicBlock* block,
                                      TargetInstruction* branch) {
  bool is_cbz = branch->opcode == (TargetOpcode)AARCH64_OP(cbz);
  bool is_cbnz = branch->opcode == (TargetOpcode)AARCH64_OP(cbnz);
  if (!is_cbz && !is_cbnz) {
    return false;
  }
  // Drop a dead zext on the predicate before looking for a cset producer.
  FoldRedundantZextBeforeCompareZeroBranch(generator, block, branch);
  TargetInstruction* value = FindCsetProducer(branch->operand[0], block, branch);
  if (value == NULL || value->operand[0] == NULL) {
    return false;
  }
  AARCH64Opcode cond = (AARCH64Opcode)value->operand[0]->opcode;
  if (InvertCond(cond) == AARCH64_OP(al) && cond != AARCH64_OP(al)) {
    return false;
  }
  // Flags must still match those consumed by the cset.
  for (TargetInstruction* inst = TargetNext(value);
       inst != NULL && inst != branch && inst->block == block;
       inst = TargetNext(inst)) {
    if (ClobbersConditionFlags(inst)) {
      return false;
    }
  }
  AARCH64Opcode branch_cond = is_cbz ? InvertCond(cond) : cond;
  if (branch_cond == AARCH64_OP(al)) {
    return false;
  }
  // Rebuild as a conditional b; keep the existing target operand.
  TargetInstruction* target = branch->operand[1];
  TargetInstruction* cond_inst =
      TargetNewInstruction((TargetOpcode)branch_cond);
  TargetBasicBlockEmitBefore(&generator->base, block, cond_inst, branch);
  branch->opcode = (TargetOpcode)AARCH64_OP(b);
  TargetReplaceOperand(branch, 0, cond_inst);
  if (target != NULL) {
    TargetReplaceOperand(branch, 1, target);
  }
  // Drop the cset when this branch (via moves) was its only consumer.
  if (value->users.length == 0) {
    RemoveInstructionAndDeadTempDest(generator, block, value);
  }
  return true;
}

static bool ReadsConditionFlags(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(cset):
    case AARCH64_OP(csetm):
    case AARCH64_OP(csel):
    case AARCH64_OP(csinc):
    case AARCH64_OP(csinv):
    case AARCH64_OP(csneg):
    case AARCH64_OP(cinc):
    case AARCH64_OP(cinv):
    case AARCH64_OP(cneg):
    case AARCH64_OP(ccmp):
    case AARCH64_OP(ccmn):
    case AARCH64_OP(adc):
    case AARCH64_OP(adcs):
    case AARCH64_OP(sbc):
    case AARCH64_OP(sbcs):
    case AARCH64_OP(ngc):
    case AARCH64_OP(ngcs):
    case AARCH64_OP(asm):
      return true;
    case AARCH64_OP(b):
      return inst->operand[0] != NULL &&
             inst->operand[0]->opcode != (TargetOpcode)AARCH64_OP(al);
    default:
      return false;
  }
}

static bool OverwritesConditionFlags(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(cmp):
    case AARCH64_OP(cmn):
    case AARCH64_OP(tst):
    case AARCH64_OP(fcmp):
    case AARCH64_OP(adds):
    case AARCH64_OP(subs):
    case AARCH64_OP(negs):
    case AARCH64_OP(ands):
    case AARCH64_OP(bics):
    case AARCH64_OP(eons):
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
    case AARCH64_OP(ret):
      return true;
    default:
      return false;
  }
}

static bool FlagsDeadAfter(TargetBasicBlock* block, TargetInstruction* pos) {
  for (TargetInstruction* cur = TargetNext(pos);
       cur != NULL && cur->block == block; cur = TargetNext(cur)) {
    if (ReadsConditionFlags(cur)) {
      return false;
    }
    if (OverwritesConditionFlags(cur)) {
      return true;
    }
    if (cur == block->end_code) {
      break;
    }
  }
  return true;
}

static bool IsCompareWithZero(TargetInstruction* inst) {
  if (inst == NULL || inst->opcode != (TargetOpcode)AARCH64_OP(cmp) ||
      inst->operand[0] == NULL || inst->operand[1] == NULL) {
    return false;
  }
  if (inst->operand[1]->opcode == (TargetOpcode)AARCH64_OP(zr)) {
    return true;
  }
  return AARCH64IsIntConst(inst->operand[1]) &&
         AARCH64IntValue(inst->operand[1]) == 0;
}

static bool IsZeroOneValue(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  if (inst->opcode == (TargetOpcode)AARCH64_OP(cset)) {
    return true;
  }
  if (inst->opcode == (TargetOpcode)AARCH64_OP(and) &&
      inst->operand[2] == NULL && inst->operand[1] != NULL &&
      AARCH64IsIntConst(inst->operand[1]) &&
      AARCH64IntValue(inst->operand[1]) == 1) {
    return true;
  }
  if (inst->opcode == (TargetOpcode)AARCH64_OP(ubfx) &&
      inst->operand[2] != NULL && AARCH64IsIntConst(inst->operand[2]) &&
      AARCH64IntValue(inst->operand[2]) == 1) {
    return true;
  }
  return false;
}

static bool ExtractSingleBitTest(TargetInstruction* val,
                                 TargetBasicBlock* block,
                                 TargetInstruction* use,
                                 int max_bits,
                                 TargetInstruction** base_out,
                                 int* bit_out) {
  TargetInstruction* src = PeelMovSource(val, block, use);
  if (src == NULL || src->block != block || src->dest != NULL) {
    return false;
  }
  if (src->opcode == (TargetOpcode)AARCH64_OP(and) &&
      src->operand[2] == NULL && src->operand[0] != NULL &&
      !AARCH64IsConst(src->operand[0]) && src->operand[1] != NULL &&
      AARCH64IsIntConst(src->operand[1])) {
    uint64_t mask = (uint64_t)AARCH64IntValue(src->operand[1]);
    if (max_bits == 32) {
      mask &= 0xffffffffu;
    }
    if (mask != 0 && (mask & (mask - 1)) == 0) {
      int bit = 0;
      while (((mask >> bit) & 1u) == 0) {
        bit++;
      }
      if (bit >= 0 && bit < max_bits &&
          ValueUnclobberedBetween(src, use, src->operand[0])) {
        *base_out = src->operand[0];
        *bit_out = bit;
        return true;
      }
    }
  }
  if (src->opcode == (TargetOpcode)AARCH64_OP(ubfx) &&
      src->operand[0] != NULL && !AARCH64IsConst(src->operand[0]) &&
      src->operand[1] != NULL && AARCH64IsIntConst(src->operand[1]) &&
      src->operand[2] != NULL && AARCH64IsIntConst(src->operand[2]) &&
      AARCH64IntValue(src->operand[2]) == 1) {
    int64_t bit = AARCH64IntValue(src->operand[1]);
    if (bit >= 0 && bit < max_bits &&
        ValueUnclobberedBetween(src, use, src->operand[0])) {
      *base_out = src->operand[0];
      *bit_out = (int)bit;
      return true;
    }
  }
  return false;
}

static void CombineShiftAndMaskInBlock(TargetBasicBlock* block, void* data) {
  AARCH64Generator* generator = data;
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    if (inst->opcode != (TargetOpcode)AARCH64_OP(and) ||
        inst->operand[2] != NULL || inst->operand[0] == NULL ||
        inst->operand[1] == NULL || !AARCH64IsIntConst(inst->operand[1])) {
      continue;
    }
    int width_bits = InstructionSize(inst) == kSize64Bit ? 64 : 32;
    uint64_t mask = (uint64_t)AARCH64IntValue(inst->operand[1]);
    if (width_bits == 32) {
      mask &= 0xffffffffu;
    }
    if (mask == 0 || mask == UINT64_MAX || ((mask + 1) & mask) != 0) {
      continue;
    }
    int w = 0;
    for (uint64_t m = mask; m != 0; m >>= 1) {
      w++;
    }
    TargetInstruction* shift = inst->operand[0];
    if (shift->dest != NULL || shift->users.length != 1 ||
        shift->block != block || shift->operand[0] == NULL ||
        AARCH64IsConst(shift->operand[0]) || shift->operand[1] == NULL ||
        !AARCH64IsIntConst(shift->operand[1]) ||
        InstructionSize(shift) != InstructionSize(inst) ||
        (shift->opcode != (TargetOpcode)AARCH64_OP(lsr) &&
         shift->opcode != (TargetOpcode)AARCH64_OP(asr))) {
      continue;
    }
    int64_t lsb = AARCH64IntValue(shift->operand[1]);
    if (lsb <= 0 || w <= 0 || lsb + w > width_bits) {
      continue;
    }
    TargetInstruction* base = shift->operand[0];
    if (!ValueUnclobberedBetweenGen(&generator->base, shift, inst, base)) {
      continue;
    }
    TargetInstruction* lsb_const = shift->operand[1];
    TargetInstruction* w_const =
        TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, w);
    inst->opcode = (TargetOpcode)AARCH64_OP(ubfx);
    TargetReplaceOperand(inst, 0, base);
    TargetReplaceOperand(inst, 1, lsb_const);
    AddOperand(inst, 2, w_const);
    TargetBasicBlockRemoveInstruction(&generator->base, block, shift);
  }
}

static void SimplifyRedundantCsetsInBlock(TargetBasicBlock* block, void* data) {
  AARCH64Generator* generator = data;
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) && inst != TargetBasicBlockEnd(block);
       inst = next) {
    next = TargetNext(inst);
    if (inst->opcode != (TargetOpcode)AARCH64_OP(cset) ||
        inst->dest != NULL || inst->operand[0] == NULL) {
      continue;
    }
    AARCH64Opcode cond = (AARCH64Opcode)inst->operand[0]->opcode;
    if (cond != AARCH64_OP(eq) && cond != AARCH64_OP(ne)) {
      continue;
    }
    TargetInstruction* prev = TargetPrev(inst);
    while (prev != NULL && prev->block == block &&
           (prev == inst->operand[0] ||
            prev->opcode == (TargetOpcode)AARCH64_OP(loc))) {
      prev = TargetPrev(prev);
    }
    if (!IsCompareWithZero(prev) || !FlagsDeadAfter(block, inst)) {
      continue;
    }
    TargetInstruction* src = PeelMovSource(prev->operand[0], block, prev);
    if (src == NULL || src->block != block || src->dest != NULL ||
        InstructionSize(src) != InstructionSize(inst)) {
      if (prev->operand[0] != NULL &&
          prev->operand[0]->opcode == (TargetOpcode)AARCH64_OP(and) &&
          prev->operand[0]->block == block &&
          prev->operand[0]->dest == NULL &&
          prev->operand[0]->users.length == 1 &&
          prev->operand[0]->operand[0] != NULL &&
          prev->operand[0]->operand[1] != NULL &&
          prev->operand[0]->operand[2] == NULL &&
          InstructionSize(prev->operand[0]) == InstructionSize(prev) &&
          ValueUnclobberedBetween(prev->operand[0], prev,
                                  prev->operand[0]->operand[0]) &&
          ValueUnclobberedBetween(prev->operand[0], prev,
                                  prev->operand[0]->operand[1])) {
        TargetInstruction* and_inst = prev->operand[0];
        prev->opcode = (TargetOpcode)AARCH64_OP(tst);
        TargetReplaceOperand(prev, 0, and_inst->operand[0]);
        TargetReplaceOperand(prev, 1, and_inst->operand[1]);
        TargetBasicBlockRemoveInstruction(&generator->base, block, and_inst);
      }
      continue;
    }
    if (cond == AARCH64_OP(ne) && IsZeroOneValue(src)) {
      TargetRetargetInstruction(inst, src);
      TargetBasicBlockRemoveInstruction(&generator->base, block, inst);
      TargetBasicBlockRemoveInstruction(&generator->base, block, prev);
      continue;
    }
    if (cond == AARCH64_OP(eq) &&
        src->opcode == (TargetOpcode)AARCH64_OP(cset) &&
        src->operand[0] != NULL && src->users.length == 1 &&
        prev->operand[0] == src) {
      AARCH64Opcode inv = InvertCond((AARCH64Opcode)src->operand[0]->opcode);
      if (inv != AARCH64_OP(al)) {
        TargetInstruction* inv_inst =
            TargetNewInstruction((TargetOpcode)inv);
        TargetBasicBlockEmitBefore(&generator->base, block, inv_inst, src);
        TargetReplaceOperand(src, 0, inv_inst);
        TargetRetargetInstruction(inst, src);
        TargetBasicBlockRemoveInstruction(&generator->base, block, inst);
        TargetBasicBlockRemoveInstruction(&generator->base, block, prev);
        continue;
      }
    }
    if (cond == AARCH64_OP(eq) && IsZeroOneValue(src)) {
      TargetInstruction* one =
          TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, 1);
      inst->opcode = (TargetOpcode)AARCH64_OP(eor);
      TargetReplaceOperand(inst, 0, src);
      AddOperand(inst, 1, one);
      TargetBasicBlockRemoveInstruction(&generator->base, block, prev);
      continue;
    }
    int max_bits = InstructionSize(prev) == kSize64Bit ? 64 : 32;
    TargetInstruction* base = NULL;
    int bit = 0;
    if (src->users.length == 1 && prev->operand[0] == src &&
        ExtractSingleBitTest(src, block, inst, max_bits, &base, &bit) &&
        !AARCH64IsConst(base) && !AARCH64IsSymbol(base) &&
        base->opcode != (TargetOpcode)AARCH64_OP(literal)) {
      if (cond == AARCH64_OP(ne) &&
          InstructionSize(base) == InstructionSize(inst)) {
        TargetInstruction* lsb_const =
            TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, bit);
        TargetInstruction* w_const =
            TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, 1);
        inst->opcode = (TargetOpcode)AARCH64_OP(ubfx);
        TargetReplaceOperand(inst, 0, base);
        AddOperand(inst, 1, lsb_const);
        AddOperand(inst, 2, w_const);
        TargetBasicBlockRemoveInstruction(&generator->base, block, prev);
        TargetBasicBlockRemoveInstruction(&generator->base, block, src);
        continue;
      }
      if (src->opcode == (TargetOpcode)AARCH64_OP(and) &&
          src->operand[1] != NULL && AARCH64IsIntConst(src->operand[1])) {
        prev->opcode = (TargetOpcode)AARCH64_OP(tst);
        TargetReplaceOperand(prev, 0, base);
        TargetReplaceOperand(prev, 1, src->operand[1]);
        TargetBasicBlockRemoveInstruction(&generator->base, block, src);
        continue;
      }
    }
  }
}

static bool TryFoldBitTestBranch(AARCH64Generator* generator,
                                 TargetBasicBlock* block,
                                 TargetInstruction* branch,
                                 bool allow_tbz) {
  if (!allow_tbz) {
    return false;
  }
  if ((branch->opcode == (TargetOpcode)AARCH64_OP(cbz) ||
       branch->opcode == (TargetOpcode)AARCH64_OP(cbnz)) &&
      branch->operand[1] != NULL &&
      branch->operand[1]->opcode == (TargetOpcode)AARCH64_OP(label)) {
    FoldRedundantZextBeforeCompareZeroBranch(generator, block, branch);
    bool is_nonzero = branch->opcode == (TargetOpcode)AARCH64_OP(cbnz);
    TargetInstruction* cand = PeelMovSource(branch->operand[0], block, branch);
    if (cand != NULL && cand->block == block && cand->dest == NULL &&
        cand->opcode == (TargetOpcode)AARCH64_OP(eor) &&
        cand->operand[0] != NULL && cand->operand[1] != NULL &&
        AARCH64IsIntConst(cand->operand[1]) &&
        AARCH64IntValue(cand->operand[1]) == 1 &&
        IsZeroOneValue(PeelMovSource(cand->operand[0], block, cand))) {
      is_nonzero = !is_nonzero;
      cand = cand->operand[0];
    }
    int max_bits = InstructionSize(branch) == kSize64Bit ? 64 : 32;
    TargetInstruction* base = NULL;
    int bit = 0;
    if (!ExtractSingleBitTest(cand, block, branch, max_bits, &base, &bit)) {
      return false;
    }
    TargetInstruction* old_val = branch->operand[0];
    TargetInstruction* target = branch->operand[1];
    TargetInstruction* bit_const =
        TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, bit);
    branch->opcode =
        (TargetOpcode)(is_nonzero ? AARCH64_OP(tbnz) : AARCH64_OP(tbz));
    SetInstructionSize(branch, bit >= 32 ? kSize64Bit : kSize32Bit);
    TargetReplaceOperand(branch, 0, base);
    TargetReplaceOperand(branch, 1, bit_const);
    AddOperand(branch, 2, target);
    if (old_val != NULL && old_val->users.length == 0) {
      RemoveInstructionAndDeadTempDest(generator, block, old_val);
    }
    return true;
  }
  if (branch->opcode == (TargetOpcode)AARCH64_OP(b) &&
      branch->operand[0] != NULL && branch->operand[1] != NULL &&
      branch->operand[1]->opcode == (TargetOpcode)AARCH64_OP(label)) {
    AARCH64Opcode cond = (AARCH64Opcode)branch->operand[0]->opcode;
    if (cond != AARCH64_OP(eq) && cond != AARCH64_OP(ne)) {
      return false;
    }
    TargetInstruction* prev = TargetPrev(branch);
    while (prev != NULL && prev->block == block &&
           (prev == branch->operand[0] ||
            prev->opcode == (TargetOpcode)AARCH64_OP(loc))) {
      prev = TargetPrev(prev);
    }
    if (!IsCompareWithZero(prev) || !FlagsDeadAfter(block, branch)) {
      return false;
    }
    int max_bits = InstructionSize(prev) == kSize64Bit ? 64 : 32;
    TargetInstruction* base = NULL;
    int bit = 0;
    if (!ExtractSingleBitTest(prev->operand[0], block, branch, max_bits, &base,
                              &bit)) {
      return false;
    }
    TargetInstruction* target = branch->operand[1];
    TargetInstruction* bit_const =
        TargetGetIntConstant(&generator->base, NULL, kTargetType32Bit, bit);
    branch->opcode = (TargetOpcode)(cond == AARCH64_OP(ne) ? AARCH64_OP(tbnz)
                                                           : AARCH64_OP(tbz));
    SetInstructionSize(branch, bit >= 32 ? kSize64Bit : kSize32Bit);
    TargetReplaceOperand(branch, 0, base);
    TargetReplaceOperand(branch, 1, bit_const);
    AddOperand(branch, 2, target);
    TargetBasicBlockRemoveInstruction(&generator->base, block, prev);
    return true;
  }
  return false;
}

typedef struct {
  AARCH64Generator* generator;
  bool allow_tbz;
} BranchFoldCtx;

static void FoldCsetCompareZeroBranchesInBlock(TargetBasicBlock* block,
                                               void* data) {
  BranchFoldCtx* ctx = data;
  AARCH64Generator* generator = ctx->generator;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) &&
       inst != TargetBasicBlockEnd(block);
       inst = TargetNext(inst)) {
    FoldCsetCompareZeroBranch(generator, block, inst);
    TryFoldBitTestBranch(generator, block, inst, ctx->allow_tbz);
  }
}

static void FoldCsetCompareZeroBranches(AARCH64Generator* generator) {
  size_t count = 0;
  for (TargetInstruction* p = TargetFirstInstruction(&generator->base);
       p != NULL; p = TargetNext(p)) {
    if (AARCH64IsConst(p) ||
        p->opcode == (TargetOpcode)AARCH64_OP(loc) ||
        p->opcode == (TargetOpcode)AARCH64_OP(label) ||
        AARCH64IsVarRegister(p)) {
      continue;
    }
    count++;
    if (count >= 6000) {
      break;
    }
  }
  BranchFoldCtx ctx = {generator, count < 6000};
  TargetTraverseDominatorTree(&generator->base,
                              FoldCsetCompareZeroBranchesInBlock,
                              kTraversePreOrder, &ctx);
}

static TargetInstruction* EmitLocalZeroReg(AARCH64Generator* generator,
                                           TargetBasicBlock* cond_blk,
                                           TargetInstruction* before_pos) {
  if (generator->zero != NULL) {
    return generator->zero;
  }
  TargetInstruction* z = TargetNewInstruction((TargetOpcode)AARCH64_OP(zr));
  TargetBasicBlockEmitBefore(&generator->base, cond_blk, z, before_pos);
  return z;
}

static bool IsSafePureArmAlu(TargetInstruction* inst) {
  if (inst == NULL || inst->observable_checkpoint ||
      AARCH64HasFloatResult(inst)) {
    return false;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(add):
    case AARCH64_OP(sub):
    case AARCH64_OP(and):
    case AARCH64_OP(orr):
    case AARCH64_OP(eor):
    case AARCH64_OP(lsl):
    case AARCH64_OP(lsr):
    case AARCH64_OP(asr):
      return inst->operand[0] != NULL && inst->operand[1] != NULL &&
             inst->operand[2] == NULL && inst->operand[3] == NULL;
    case AARCH64_OP(ubfx):
      return inst->operand[0] != NULL && inst->operand[1] != NULL &&
             inst->operand[2] != NULL && inst->operand[3] == NULL &&
             AARCH64IsIntConst(inst->operand[1]) &&
             AARCH64IsIntConst(inst->operand[2]);
    default:
      return false;
  }
}

static bool ExtractSingleArmAssignment(TargetBasicBlock* arm,
                                       TargetBasicBlock* expected_succ,
                                       TargetInstruction** dest_out,
                                       TargetInstruction** val_out,
                                       TargetInstruction** alu_out,
                                       int* size_out) {
  if (arm == NULL || arm->is_unreachable || arm->contains_call ||
      arm->uses_floating_point || arm->code == NULL ||
      arm->in_edges.length != 1 || arm->out_edges.length != 1 ||
      (size_t)arm->out_edges.value.w[0] != expected_succ->block_id) {
    return false;
  }
  TargetInstruction* exec1 = NULL;
  TargetInstruction* exec2 = NULL;
  for (TargetInstruction* p = arm->code; p != NULL; p = TargetNext(p)) {
    AARCH64Opcode op = (AARCH64Opcode)p->opcode;
    if ((p->flags & TARGET_INST_KEEP_UNREACHABLE) != 0) {
      return false;
    }
    if (op == AARCH64_OP(loc) || op == AARCH64_OP(al)) {
      if (p == arm->end_code) break;
      continue;
    }
    if (op == AARCH64_OP(label)) {
      if (p != arm->code || p->users.length > 1) {
        return false;
      }
      if (p == arm->end_code) break;
      continue;
    }
    if (p == arm->end_code && op == AARCH64_OP(b) &&
        p->operand[0] != NULL &&
        p->operand[0]->opcode == (TargetOpcode)AARCH64_OP(al) &&
        p->operand[1] != NULL &&
        p->operand[1]->opcode == (TargetOpcode)AARCH64_OP(label) &&
        p->operand[1]->block == expected_succ) {
      break;
    }
    if (exec1 == NULL) {
      exec1 = p;
    } else if (exec2 == NULL) {
      exec2 = p;
    } else {
      return false;
    }
    if (p == arm->end_code) {
      break;
    }
  }
  if (exec1 == NULL) {
    return false;
  }
  if (exec2 == NULL) {
    if ((exec1->opcode == (TargetOpcode)AARCH64_OP(mov) ||
         exec1->opcode == (TargetOpcode)AARCH64_OP(mv)) &&
        exec1->dest != NULL && exec1->users.length == 0 &&
        exec1->operand[0] != NULL && exec1->operand[1] == NULL &&
        !AARCH64HasFloatResult(exec1) &&
        !AARCH64HasFloatResult(exec1->dest) &&
        AARCH64IsVarRegister(exec1->dest)) {
      *dest_out = exec1->dest;
      *val_out = exec1->operand[0];
      *alu_out = NULL;
      *size_out = InstructionSize(exec1) == kSize64Bit ? kSize64Bit
                                                       : kSize32Bit;
      return true;
    }
    if (IsSafePureArmAlu(exec1) && exec1->dest != NULL &&
        exec1->users.length == 0 && !AARCH64HasFloatResult(exec1->dest) &&
        AARCH64IsVarRegister(exec1->dest)) {
      *dest_out = exec1->dest;
      *val_out = exec1;
      *alu_out = exec1;
      *size_out = InstructionSize(exec1) == kSize64Bit ? kSize64Bit
                                                       : kSize32Bit;
      return true;
    }
    return false;
  }
  if (IsSafePureArmAlu(exec1) && exec1->dest == NULL &&
      exec1->users.length == 1 &&
      (exec2->opcode == (TargetOpcode)AARCH64_OP(mov) ||
       exec2->opcode == (TargetOpcode)AARCH64_OP(mv)) &&
      exec2->operand[0] == exec1 && exec2->operand[1] == NULL &&
      exec2->dest != NULL && exec2->users.length == 0 &&
      !AARCH64HasFloatResult(exec2) &&
      !AARCH64HasFloatResult(exec2->dest) &&
      InstructionSize(exec1) == InstructionSize(exec2) &&
      AARCH64IsVarRegister(exec2->dest)) {
    *dest_out = exec2->dest;
    *val_out = exec1;
    *alu_out = exec1;
    *size_out = InstructionSize(exec2) == kSize64Bit ? kSize64Bit : kSize32Bit;
    return true;
  }
  return false;
}

static bool IsEmptyPassthroughArm(TargetBasicBlock* arm,
                                  TargetBasicBlock* expected_succ) {
  if (arm == NULL || arm->is_unreachable || arm->contains_call ||
      arm->uses_floating_point || arm->in_edges.length != 1 ||
      arm->out_edges.length != 1 ||
      (size_t)arm->out_edges.value.w[0] != expected_succ->block_id) {
    return false;
  }
  for (TargetInstruction* p = arm->code; p != NULL; p = TargetNext(p)) {
    AARCH64Opcode op = (AARCH64Opcode)p->opcode;
    if ((p->flags & TARGET_INST_KEEP_UNREACHABLE) != 0) {
      return false;
    }
    if (op == AARCH64_OP(loc) || op == AARCH64_OP(al)) {
      if (p == arm->end_code) break;
      continue;
    }
    if (op == AARCH64_OP(label)) {
      if (p != arm->code || p->users.length > 1) {
        return false;
      }
      if (p == arm->end_code) break;
      continue;
    }
    if (p == arm->end_code && op == AARCH64_OP(b) &&
        p->operand[0] != NULL &&
        p->operand[0]->opcode == (TargetOpcode)AARCH64_OP(al) &&
        p->operand[1] != NULL &&
        p->operand[1]->opcode == (TargetOpcode)AARCH64_OP(label) &&
        p->operand[1]->block == expected_succ) {
      break;
    }
    return false;
  }
  return true;
}

static bool HasDominatingVarRegDef(TargetGenerator* gen,
                                   TargetBasicBlock* cond_blk,
                                   TargetInstruction* var) {
  if (!AARCH64IsVarRegister(var)) {
    return false;
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL ||
        (blk != cond_blk && !BitSetContains(&cond_blk->dominators, b))) {
      continue;
    }
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->dest == var) {
        return true;
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  return false;
}

static TargetInstruction* UnlinkArmAluToCond(TargetGenerator* gen,
                                             TargetBasicBlock* arm,
                                             TargetBasicBlock* cond_blk,
                                             TargetInstruction* alu,
                                             TargetInstruction* before_pos) {
  if (arm->code == alu) {
    arm->code = arm->end_code == alu ? NULL : TargetNext(alu);
  }
  if (arm->end_code == alu) {
    arm->end_code = TargetPrev(alu);
  }
  ListDeleteElement(&gen->code, &alu->header);
  alu->header.next = NULL;
  alu->header.prev = NULL;
  alu->dest = NULL;
  TargetBasicBlockEmitBefore(gen, cond_blk, alu, before_pos);
  return alu;
}

static TargetInstruction* MaterializeCselConst(AARCH64Generator* generator,
                                               TargetBasicBlock* cond_blk,
                                               TargetInstruction* before_pos,
                                               TargetInstruction* cinst,
                                               int size) {
  if (cinst->opcode == (TargetOpcode)AARCH64_OP(zr) ||
      (AARCH64IsIntConst(cinst) && AARCH64IntValue(cinst) == 0)) {
    return EmitLocalZeroReg(generator, cond_blk, before_pos);
  }
  TargetInstruction* mv =
      SetInstructionSize(TargetNewInstruction1((TargetOpcode)AARCH64_OP(mov),
                                               cinst),
                         size);
  TargetBasicBlockEmitBefore(&generator->base, cond_blk, mv, before_pos);
  return mv;
}

static bool ConvertDiamondsToCselOnce(AARCH64Generator* generator) {
  TargetGenerator* gen = &generator->base;
  if (gen->exception_edges.length != 0 || gen->basic_blocks.length < 3) {
    return false;
  }
  size_t nblocks = gen->basic_blocks.length;
  bool* touched = calloc(nblocks, sizeof(bool));
  bool changed = false;

  for (size_t b_id = 0; b_id < nblocks; b_id++) {
    TargetBasicBlock* b = gen->basic_blocks.value.p[b_id];
    if (b == NULL || b->is_unreachable || touched[b_id] || b->code == NULL ||
        b->end_code == NULL || b->out_edges.length != 2) {
      continue;
    }
    TargetInstruction* br = b->end_code;
    AARCH64Opcode brop = (AARCH64Opcode)br->opcode;
    TargetInstruction* taken_lbl = NULL;
    AARCH64Opcode cond_code = AARCH64_OP(al);
    if (brop == AARCH64_OP(b)) {
      if (br->operand[0] == NULL || br->operand[1] == NULL ||
          br->operand[1]->opcode != (TargetOpcode)AARCH64_OP(label)) {
        continue;
      }
      cond_code = (AARCH64Opcode)br->operand[0]->opcode;
      if (InvertCond(cond_code) == AARCH64_OP(al)) {
        continue;
      }
      taken_lbl = br->operand[1];
    } else if (brop == AARCH64_OP(cbz) || brop == AARCH64_OP(cbnz)) {
      if (br->operand[0] == NULL || br->operand[1] == NULL ||
          br->operand[1]->opcode != (TargetOpcode)AARCH64_OP(label)) {
        continue;
      }
      cond_code = brop == AARCH64_OP(cbz) ? AARCH64_OP(eq) : AARCH64_OP(ne);
      taken_lbl = br->operand[1];
    } else if (brop == AARCH64_OP(tbz) || brop == AARCH64_OP(tbnz)) {
      if (br->operand[0] == NULL || br->operand[1] == NULL ||
          !AARCH64IsIntConst(br->operand[1]) || br->operand[2] == NULL ||
          br->operand[2]->opcode != (TargetOpcode)AARCH64_OP(label)) {
        continue;
      }
      cond_code = brop == AARCH64_OP(tbz) ? AARCH64_OP(eq) : AARCH64_OP(ne);
      taken_lbl = br->operand[2];
    } else {
      continue;
    }

    TargetBasicBlock* taken_blk = taken_lbl->block;
    if (taken_blk == NULL || taken_blk == b ||
        taken_blk->block_id >= nblocks) {
      continue;
    }
    size_t e0 = (size_t)b->out_edges.value.w[0];
    size_t e1 = (size_t)b->out_edges.value.w[1];
    size_t fall_id = (e0 == taken_blk->block_id) ? e1 : e0;
    if (fall_id >= nblocks || fall_id == taken_blk->block_id ||
        fall_id == b_id) {
      continue;
    }
    TargetBasicBlock* fall_blk = gen->basic_blocks.value.p[fall_id];
    if (fall_blk == NULL || touched[taken_blk->block_id] ||
        touched[fall_id]) {
      continue;
    }

    TargetBasicBlock* join_blk = NULL;
    TargetInstruction* dest_var = NULL;
    TargetInstruction* val_true = NULL;
    TargetInstruction* val_false = NULL;
    TargetInstruction* alu_true = NULL;
    TargetInstruction* alu_false = NULL;
    int sel_size = 0;
    bool clear_taken = false;
    bool clear_fall = false;

    // 2-arm diamond: taken_blk -> join_blk and fall_blk -> join_blk.
    if (taken_blk->out_edges.length == 1 && fall_blk->out_edges.length == 1 &&
        taken_blk->out_edges.value.w[0] == fall_blk->out_edges.value.w[0]) {
      size_t jid = (size_t)taken_blk->out_edges.value.w[0];
      if (jid < nblocks && jid != b_id && jid != taken_blk->block_id &&
          jid != fall_id && !touched[jid]) {
        TargetBasicBlock* cand_join = gen->basic_blocks.value.p[jid];
        TargetInstruction* dt = NULL;
        TargetInstruction* df = NULL;
        int st = 0, sf = 0;
        if (cand_join != NULL && cand_join != gen->exit_block) {
          bool ok_t = ExtractSingleArmAssignment(taken_blk, cand_join, &dt,
                                                 &val_true, &alu_true, &st);
          bool ok_f = ExtractSingleArmAssignment(fall_blk, cand_join, &df,
                                                 &val_false, &alu_false, &sf);
          if (ok_t && ok_f && dt == df && st == sf) {
            join_blk = cand_join;
            dest_var = dt;
            sel_size = st;
            clear_taken = true;
            clear_fall = true;
          } else if (ok_t && !ok_f &&
                     IsEmptyPassthroughArm(fall_blk, cand_join) &&
                     HasDominatingVarRegDef(gen, b, dt)) {
            join_blk = cand_join;
            dest_var = dt;
            val_false = dt;
            alu_false = NULL;
            sel_size = st;
            clear_taken = true;
            clear_fall = true;
          } else if (!ok_t && ok_f &&
                     IsEmptyPassthroughArm(taken_blk, cand_join) &&
                     HasDominatingVarRegDef(gen, b, df)) {
            join_blk = cand_join;
            dest_var = df;
            val_true = df;
            alu_true = NULL;
            sel_size = sf;
            clear_taken = true;
            clear_fall = true;
          }
        }
      }
    }

    // 1-arm triangle (taken bypasses fall_blk: taken_blk == join_blk, fall_blk -> taken_blk).
    if (join_blk == NULL && fall_blk->out_edges.length == 1 &&
        (size_t)fall_blk->out_edges.value.w[0] == taken_blk->block_id &&
        taken_blk != gen->exit_block) {
      TargetInstruction* df = NULL;
      int sf = 0;
      if (ExtractSingleArmAssignment(fall_blk, taken_blk, &df, &val_false,
                                     &alu_false, &sf) &&
          HasDominatingVarRegDef(gen, b, df)) {
        join_blk = taken_blk;
        dest_var = df;
        val_true = df;
        alu_true = NULL;
        sel_size = sf;
        clear_taken = false;
        clear_fall = true;
      }
    }

    // 1-arm triangle (fall bypasses taken_blk: fall_blk == join_blk, taken_blk -> fall_blk).
    if (join_blk == NULL && taken_blk->out_edges.length == 1 &&
        (size_t)taken_blk->out_edges.value.w[0] == fall_id &&
        fall_blk != gen->exit_block) {
      TargetInstruction* dt = NULL;
      int st = 0;
      if (ExtractSingleArmAssignment(taken_blk, fall_blk, &dt, &val_true,
                                     &alu_true, &st) &&
          HasDominatingVarRegDef(gen, b, dt)) {
        join_blk = fall_blk;
        dest_var = dt;
        val_false = dt;
        alu_false = NULL;
        sel_size = st;
        clear_taken = true;
        clear_fall = false;
      }
    }

    if (join_blk == NULL || dest_var == NULL || join_blk->code == NULL) {
      continue;
    }

    // Ensure `join_blk` starts with an explicit label.
    TargetInstruction* join_lbl = join_blk->code;
    if (join_lbl->opcode != (TargetOpcode)AARCH64_OP(label)) {
      join_lbl = TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
      TargetBasicBlockEmitBefore(gen, join_blk, join_lbl, join_blk->code);
    }

    // Move pure ALU instructions (if any) into `b` before `br` (flag-safe).
    if (alu_true != NULL) {
      val_true = UnlinkArmAluToCond(gen, taken_blk, b, alu_true, br);
    }
    if (alu_false != NULL) {
      val_false = UnlinkArmAluToCond(gen, fall_blk, b, alu_false, br);
    }

    bool t_const = val_true->opcode == (TargetOpcode)AARCH64_OP(zr) ||
                   AARCH64IsIntConst(val_true);
    bool f_const = val_false->opcode == (TargetOpcode)AARCH64_OP(zr) ||
                   AARCH64IsIntConst(val_false);
    int64_t t_cval = val_true->opcode == (TargetOpcode)AARCH64_OP(zr)
                         ? 0
                         : (t_const ? AARCH64IntValue(val_true) : 0);
    int64_t f_cval = val_false->opcode == (TargetOpcode)AARCH64_OP(zr)
                         ? 0
                         : (f_const ? AARCH64IntValue(val_false) : 0);
    if (sel_size == kSize32Bit) {
      t_cval = (int64_t)(int32_t)t_cval;
      f_cval = (int64_t)(int32_t)f_cval;
    }

    // Materialize non-special constants BEFORE flag setup (mov never touches
    // NZCV, and keeping cmp/tst adjacent to csel is cleanest).
    TargetInstruction* r_true = val_true;
    TargetInstruction* r_false = val_false;
    if (t_const && f_const) {
      if (!((t_cval == 1 && f_cval == 0) || (t_cval == 0 && f_cval == 1) ||
            (t_cval == -1 && f_cval == 0) || (t_cval == 0 && f_cval == -1))) {
        r_true = MaterializeCselConst(generator, b, br, val_true, sel_size);
        r_false = MaterializeCselConst(generator, b, br, val_false, sel_size);
        t_const = false;
        f_const = false;
      }
    } else if (t_const && !f_const) {
      if (t_cval != 0 && t_cval != 1 && t_cval != -1) {
        r_true = MaterializeCselConst(generator, b, br, val_true, sel_size);
        t_const = false;
      }
    } else if (!t_const && f_const) {
      if (f_cval != 0 && f_cval != 1 && f_cval != -1) {
        r_false = MaterializeCselConst(generator, b, br, val_false, sel_size);
        f_const = false;
      }
    }

    // Emit explicit cmp/tst if `br` was cbz/cbnz/tbz/tbnz.
    if (brop == AARCH64_OP(cbz) || brop == AARCH64_OP(cbnz)) {
      int csz = InstructionSize(br) == kSize64Bit ? kSize64Bit : kSize32Bit;
      TargetInstruction* cmp = SetInstructionSize(
          TargetNewInstruction2((TargetOpcode)AARCH64_OP(cmp), br->operand[0],
                                EmitLocalZeroReg(generator, b, br)),
          csz);
      TargetBasicBlockEmitBefore(gen, b, cmp, br);
    } else if (brop == AARCH64_OP(tbz) || brop == AARCH64_OP(tbnz)) {
      int bit = (int)AARCH64IntValue(br->operand[1]);
      int tsz = bit >= 32 ? kSize64Bit : kSize32Bit;
      TargetInstruction* mask_c = TargetGetIntConstant(
          gen, NULL, tsz == kSize64Bit ? kTargetType64Bit : kTargetType32Bit,
          (int64_t)(UINT64_C(1) << bit));
      TargetInstruction* tst = SetInstructionSize(
          TargetNewInstruction2((TargetOpcode)AARCH64_OP(tst), br->operand[0],
                                mask_c),
          tsz);
      TargetBasicBlockEmitBefore(gen, b, tst, br);
    }

    TargetInstruction* sel = NULL;
    if (t_const && f_const) {
      bool inv = (t_cval == 0);
      AARCH64Opcode cc = inv ? InvertCond(cond_code) : cond_code;
      AARCH64Opcode set_op =
          (t_cval == -1 || f_cval == -1) ? AARCH64_OP(csetm) : AARCH64_OP(cset);
      TargetInstruction* cc_inst = TargetNewInstruction((TargetOpcode)cc);
      TargetBasicBlockEmitBefore(gen, b, cc_inst, br);
      sel = SetInstructionSize(
          TargetNewInstruction1((TargetOpcode)set_op, cc_inst), sel_size);
    } else if (t_const && !f_const) {
      TargetInstruction* zr = EmitLocalZeroReg(generator, b, br);
      if (t_cval == 0) {
        TargetInstruction* cc_inst =
            TargetNewInstruction((TargetOpcode)cond_code);
        TargetBasicBlockEmitBefore(gen, b, cc_inst, br);
        sel = SetInstructionSize(
            TargetNewInstruction3((TargetOpcode)AARCH64_OP(csel), zr, r_false,
                                  cc_inst),
            sel_size);
      } else {
        AARCH64Opcode cs_op =
            t_cval == 1 ? AARCH64_OP(csinc) : AARCH64_OP(csinv);
        TargetInstruction* cc_inst =
            TargetNewInstruction((TargetOpcode)InvertCond(cond_code));
        TargetBasicBlockEmitBefore(gen, b, cc_inst, br);
        sel = SetInstructionSize(
            TargetNewInstruction3((TargetOpcode)cs_op, r_false, zr, cc_inst),
            sel_size);
      }
    } else if (!t_const && f_const) {
      TargetInstruction* zr = EmitLocalZeroReg(generator, b, br);
      TargetInstruction* cc_inst =
          TargetNewInstruction((TargetOpcode)cond_code);
      TargetBasicBlockEmitBefore(gen, b, cc_inst, br);
      AARCH64Opcode cs_op =
          f_cval == 0 ? AARCH64_OP(csel)
                      : (f_cval == 1 ? AARCH64_OP(csinc) : AARCH64_OP(csinv));
      sel = SetInstructionSize(
          TargetNewInstruction3((TargetOpcode)cs_op, r_true, zr, cc_inst),
          sel_size);
    } else {
      TargetInstruction* cc_inst =
          TargetNewInstruction((TargetOpcode)cond_code);
      TargetBasicBlockEmitBefore(gen, b, cc_inst, br);
      sel = SetInstructionSize(
          TargetNewInstruction3((TargetOpcode)AARCH64_OP(csel), r_true, r_false,
                                cc_inst),
          sel_size);
    }
    sel->dest = dest_var;
    TargetBasicBlockEmitBefore(gen, b, sel, br);

    // Delete `br` plus cleared arm blocks.
    TargetBasicBlockRemoveInstruction(gen, b, br);
    if (clear_taken) {
      TargetBasicBlockClear(gen, taken_blk);
    }
    if (clear_fall) {
      TargetBasicBlockClear(gen, fall_blk);
    }

    // If `join_lbl` does not immediately follow `sel` in linear layout, jump
    // to `join_lbl`; if it does and has no other users, drop `join_lbl` too so
    // `b` and `join_blk` merge into one straight-line block!
    if (TargetNext(sel) != join_lbl) {
      TargetInstruction* al_inst =
          TargetNewInstruction((TargetOpcode)AARCH64_OP(al));
      TargetBasicBlockEmitAfter(gen, b, al_inst, sel);
      TargetInstruction* jmp = TargetNewInstruction2(
          (TargetOpcode)AARCH64_OP(b), al_inst, join_lbl);
      TargetBasicBlockEmitAfter(gen, b, jmp, al_inst);
    } else if (join_lbl->users.length == 0 &&
               (join_lbl->flags & (TARGET_INST_KEEP_UNREACHABLE |
                                   TARGET_INST_EXCEPTION_LANDING |
                                   AARCH64_EXPORTED_LABEL)) == 0) {
      TargetBasicBlockRemoveInstruction(gen, join_blk, join_lbl);
    }

    touched[b_id] = true;
    touched[taken_blk->block_id] = true;
    touched[fall_id] = true;
    touched[join_blk->block_id] = true;
    changed = true;
  }

  free(touched);
  if (changed) {
    TargetRebuildBasicBlocks(gen);
  }
  return changed;
}

static void ConvertDiamondsToCsel(AARCH64Generator* generator) {
  for (int round = 0; round < 3; round++) {
    if (!ConvertDiamondsToCselOnce(generator)) {
      break;
    }
  }
}

static bool IsFallthroughJumpToNext(TargetInstruction* inst) {
  if (inst == NULL || inst->block == NULL || inst->block->is_unreachable ||
      (inst->flags & TARGET_INST_KEEP_UNREACHABLE) != 0 ||
      inst->opcode != (TargetOpcode)AARCH64_OP(b) ||
      inst->operand[0] == NULL ||
      inst->operand[0]->opcode != (TargetOpcode)AARCH64_OP(al) ||
      inst->operand[1] == NULL ||
      inst->operand[1]->opcode != (TargetOpcode)AARCH64_OP(label)) {
    return false;
  }
  TargetInstruction* target = inst->operand[1];
  for (TargetInstruction* p = TargetNext(inst); p != NULL; p = TargetNext(p)) {
    if (p == target) {
      return p->block != NULL && !p->block->is_unreachable;
    }
    if (p->opcode == (TargetOpcode)AARCH64_OP(loc)) {
      continue;
    }
    return false;
  }
  return false;
}

static void SimplifyCFGForOptimize(AARCH64Generator* generator) {
  TargetGenerator* gen = &generator->base;
  if (generator->exception_ranges.length != 0 ||
      gen->exception_edges.length != 0 || gen->entry_block == NULL) {
    return;
  }
  bool changed = false;
  bool in_jump_table = false;
  TargetInstruction* next = NULL;
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = next) {
    next = TargetNext(inst);
    if (inst->block == NULL) {
      continue;
    }
    AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
    if (op != AARCH64_OP(label) && op != AARCH64_OP(named_label) &&
        op != AARCH64_OP(loc) && !AARCH64IsConst(inst) &&
        !AARCH64IsSymbol(inst) && !AARCH64IsVarRegister(inst) &&
        op != AARCH64_OP(al) && !inst->observable_checkpoint) {
      if (op == AARCH64_OP(br) &&
          (inst->flags & TARGET_INST_TABLE_JUMP) != 0) {
        in_jump_table = true;
      } else if (!(op == AARCH64_OP(b) && inst->operand[0] != NULL &&
                   inst->operand[0]->opcode == (TargetOpcode)AARCH64_OP(al))) {
        in_jump_table = false;
      }
    }
    if (!in_jump_table && IsFallthroughJumpToNext(inst)) {
      TargetBasicBlockRemoveInstruction(gen, inst->block, inst);
      changed = true;
    }
  }
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = next) {
    next = TargetNext(inst);
    if (inst->block == NULL || inst->block->is_unreachable ||
        inst == gen->entry_block->code) {
      continue;
    }
    if (inst->opcode == (TargetOpcode)AARCH64_OP(label) &&
        inst->users.length == 0 &&
        (inst->flags & (TARGET_INST_KEEP_UNREACHABLE |
                        TARGET_INST_EXCEPTION_LANDING |
                        AARCH64_EXPORTED_LABEL)) == 0) {
      TargetBasicBlockRemoveInstruction(gen, inst->block, inst);
      changed = true;
    }
  }
  if (changed) {
    TargetRebuildBasicBlocks(gen);
  }
}

void AARCH64Optimize(AARCH64Generator* generator) {
  SimplifyCFGForOptimize(generator);
  EliminateMoves(generator);
  RemoveUnusedExpressions(generator);
  CombineLoadOrStores(generator);
  CombineShiftedAdds(generator);
  TargetTraverseDominatorTree(&generator->base, CombineShiftAndMaskInBlock,
                              kTraversePreOrder, generator);
  TargetTraverseDominatorTree(&generator->base, SimplifyRedundantCsetsInBlock,
                              kTraversePreOrder, generator);
  FoldCsetCompareZeroBranches(generator);
  ConvertDiamondsToCsel(generator);
  SimplifyCFGForOptimize(generator);
  EliminateMoves(generator);
  RemoveUnusedExpressions(generator);
  TargetBuildBasicBlockInputsAndOutputs(&generator->base);
}
