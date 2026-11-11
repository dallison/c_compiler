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
          if (IsMove(p) && p->dest == source) {
            def = p;
          }
        }
        if (def != NULL && MoveSizesCompatible(def, inst)) {
          TargetInstruction* forwarded = MoveSource(def);
          bool clobbered = false;
          for (TargetInstruction* p = TargetNext(def); p != NULL && p != inst;
               p = TargetNext(p)) {
            if (WritesFixedValue(p, forwarded) ||
                SameFixedRegister(p->dest, forwarded) ||
                (WritesRegisterOperand(p) &&
                 SameFixedRegister(p->operand[0], forwarded))) {
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

static bool FoldAddressAdd(AARCH64Generator* generator,
                           TargetInstruction* memory,
                           int base_index, int offset_index) {
  TargetInstruction* base = memory->operand[base_index];
  TargetInstruction* offset = memory->operand[offset_index];
  if (!IsAddressAdd(base) || offset == NULL ||
      !AARCH64IsIntConst(offset)) {
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
static bool IsRegisterAddressAdd(TargetInstruction* inst) {
  return inst != NULL &&
         inst->opcode == (TargetOpcode)AARCH64_OP(add) &&
         inst->dest == NULL &&
         inst->operand[0] != NULL && !AARCH64IsIntConst(inst->operand[0]) &&
         inst->operand[1] != NULL && !AARCH64IsIntConst(inst->operand[1]);
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
      IsUnscaledOnly(memory)) {
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
  if (shift != 0) {
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
  if (src == NULL) {
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
                                        TargetBasicBlock* block) {
  while (value != NULL && IsMove(value) && value->block == block) {
    value = MoveSource(value);
  }
  return value;
}

// Resolve a cbz/cbnz operand to the cset that produced it (expression-style
// cset, or cset that wrote a tmp the branch reads).
static TargetInstruction* FindCsetProducer(TargetInstruction* value,
                                           TargetBasicBlock* block) {
  value = PeelMovSource(value, block);
  if (value == NULL || value->block != block) {
    return NULL;
  }
  if (value->opcode == (TargetOpcode)AARCH64_OP(cset)) {
    return value;
  }
  // ubfx (objdump: uxtb/uxth) of a cset — peel one layer.
  if (value->opcode == (TargetOpcode)AARCH64_OP(ubfx) &&
      value->operand[0] != NULL) {
    TargetInstruction* inner = PeelMovSource(value->operand[0], block);
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
          inst->dest == value) {
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
  TargetInstruction* value = PeelMovSource(branch->operand[0], block);
  if (value == NULL || value->opcode != (TargetOpcode)AARCH64_OP(ubfx) ||
      value->operand[0] == NULL || value->operand[1] == NULL ||
      value->operand[2] == NULL || !AARCH64IsIntConst(value->operand[1]) ||
      !AARCH64IsIntConst(value->operand[2]) ||
      AARCH64IntValue(value->operand[1]) != 0) {
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
  TargetInstruction* value = FindCsetProducer(branch->operand[0], block);
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

static void FoldCsetCompareZeroBranchesInBlock(TargetBasicBlock* block,
                                               void* data) {
  AARCH64Generator* generator = data;
  for (TargetInstruction* inst = TargetBasicBlockBegin(block);
       !TargetBasicBlockIsEmpty(block) &&
       inst != TargetBasicBlockEnd(block);
       inst = TargetNext(inst)) {
    FoldCsetCompareZeroBranch(generator, block, inst);
  }
}

static void FoldCsetCompareZeroBranches(AARCH64Generator* generator) {
  TargetTraverseDominatorTree(&generator->base,
                              FoldCsetCompareZeroBranchesInBlock,
                              kTraversePreOrder, generator);
}

void AARCH64Optimize(AARCH64Generator* generator) {
  EliminateMoves(generator);
  RemoveUnusedExpressions(generator);
  CombineLoadOrStores(generator);
  CombineShiftedAdds(generator);
  FoldCsetCompareZeroBranches(generator);
  RemoveUnusedExpressions(generator);
  TargetBuildBasicBlockInputsAndOutputs(&generator->base);
}
