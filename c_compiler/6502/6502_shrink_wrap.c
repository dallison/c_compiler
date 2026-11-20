//
//  6502_shrink_wrap.c
//  c_compiler
//
//  6502 side of the shared shrink-wrap pass.  __enter saves preserved
//  zero-page registers and builds the frame, so a path that returns without
//  either skips it.  The fast path may use only the temp registers.  There
//  is no caller-saved file to remap a preserved register onto.
//

#include "6502_reg_alloc.h"

#include <string.h>
#include "6502_codegen.h"
#include "compiler.h"
#include "target_basic_block.h"
#include "target_shrink_wrap.h"

static bool IsEnter(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op == W65C02_OP(enter) || op == W65C02_OP(enter_leaf);
}

static bool IsLeave(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op == W65C02_OP(leave) || op == W65C02_OP(leave_leaf);
}

static bool IsSpillOp(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op >= W65C02_OP(spill1) && op <= W65C02_OP(spill8);
}

static bool IsReloadOp(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op >= W65C02_OP(reload1) && op <= W65C02_OP(reload8);
}

static bool IsReturnOp(TargetInstruction* inst) {
  // leave jumps to __leave, which restores and returns.  There is no rts in
  // the function.  The fast path, which never entered, returns with rts.
  return IsLeave(inst) || inst->opcode == (TargetOpcode)W65C02_OP(rts);
}

static bool IsLabelOp(TargetInstruction* inst) {
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op == W65C02_OP(label) || op == W65C02_OP(named_label);
}

static bool FunctionOk(void* ctx, TargetGenerator* gen) {
  (void)ctx;
  // A non-void return hands the value back through the slot __enter recorded.
  // Skipping __enter would lose that slot, so only a void function can return
  // with a plain rts.
  return OptLevel1() && gen->is_void && !gen->varargs &&
         gen->exception_edges.length == 0 && gen->entry_block != NULL &&
         gen->entry_block->in_edges.length == 0;
}

static bool IsMeta(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst->observable_checkpoint || W65C02IsConst(inst)) {
    return true;
  }
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(label):
    case W65C02_OP(named_label):
    case W65C02_OP(loc):
    case W65C02_OP(symbol):
    case W65C02_OP(literal):
    case W65C02_OP(tmp):
    case W65C02_OP(fp):
    case W65C02_OP(sp):
    case W65C02_OP(ap):
    case W65C02_OP(tp):
    case W65C02_OP(resulti):
    case W65C02_OP(resultf):
    case W65C02_OP(resultd):
    case W65C02_OP(localvar):
    case W65C02_OP(argument):
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
    case W65C02_OP(reloadpoint):
      return true;
    default:
      return false;
  }
}

static bool NeedsFrame(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(localvar):
    case W65C02_OP(argument):
    case W65C02_OP(fp):
    case W65C02_OP(sp):
    case W65C02_OP(ap):
      return true;
    default:
      return false;
  }
}

static bool IsHelper(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(jsr):
    case W65C02_OP(pushreg2):
    case W65C02_OP(pushreg4):
    case W65C02_OP(pushreg8):
    case W65C02_OP(pushmem1):
    case W65C02_OP(pushmem2):
    case W65C02_OP(pushmem_xy1):
    case W65C02_OP(pushmem_xy2):
    case W65C02_OP(copymem1):
    case W65C02_OP(copymem2):
    case W65C02_OP(zeromem1):
    case W65C02_OP(zeromem2):
    case W65C02_OP(structreturn):
    case W65C02_OP(load_result):
    case W65C02_OP(load_result_value1):
    case W65C02_OP(load_result_value2):
    case W65C02_OP(fake_bra):
    case W65C02_OP(phi):
    case W65C02_OP(ssavar):
    case W65C02_OP(jumptable):
    case W65C02_OP(asm):
    case W65C02_OP(brk):
    case W65C02_OP(rti):
      return true;
    default:
      return (inst->flags & (k6502InstIsCall | k6502ProcedureCall)) != 0;
  }
}

static bool NoFallthrough(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst == NULL) {
    return false;
  }
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  return op == W65C02_OP(rts) || op == W65C02_OP(jmp) || op == W65C02_OP(bra);
}

static TargetInstruction** BranchTarget(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  W65C02Opcode op = (W65C02Opcode)inst->opcode;
  if (op == W65C02_OP(jmp) || op == W65C02_OP(bra) ||
      (W65C02IsBranch(inst) && op != W65C02_OP(jsr))) {
    return &inst->operand[0];
  }
  return NULL;
}

static bool RecordReg(void* ctx, TargetRegister* reg, uint64_t* saved,
                      uint64_t* temp, uint64_t* arg) {
  (void)ctx;
  (void)saved;
  (void)temp;
  (void)arg;
  if (reg == NULL) {
    return true;
  }
  // Preserved cells, the frame pointer and the stack pointer are saved or
  // established by __enter.  A temp is not.
  return ((W65C02Register*)reg)->temp;
}

static TargetSWKind Classify(void* ctx, TargetInstruction* inst, bool allow_save,
                             bool allow_reload, uint64_t* saved, uint64_t* temp,
                             uint64_t* arg) {
  (void)allow_reload;
  (void)saved;
  (void)temp;
  (void)arg;
  if (allow_save && IsEnter(inst)) {
    return kTargetSWMeta;
  }
  if (IsSpillOp(inst) || IsReloadOp(inst) || IsEnter(inst) || IsLeave(inst) ||
      IsReturnOp(inst) || IsHelper(inst)) {
    return kTargetSWBad;
  }
  if (IsMeta(ctx, inst)) {
    return kTargetSWMeta;
  }
  if (NeedsFrame(inst)) {
    return kTargetSWBad;
  }
  for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
    if (NeedsFrame(inst->operand[k])) {
      return kTargetSWBad;
    }
  }
  return kTargetSWExec;
}

static bool ImmediateZero(TargetInstruction* inst) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  return mode == kAddrModeImmediate && inst->operand[0] != NULL &&
         TargetIsConst(inst->operand[0]) &&
         TargetIntValue(inst->operand[0]) == 0;
}

static bool ColdEntrySplit(void* ctx, TargetBasicBlock* entry) {
  (void)ctx;
  for (TargetInstruction* p = entry->code; p != NULL; p = TargetNext(p)) {
    W65C02Opcode op = (W65C02Opcode)p->opcode;
    if ((op == W65C02_OP(cmp) || op == W65C02_OP(cpx) ||
         op == W65C02_OP(cpy)) &&
        ImmediateZero(p)) {
      return true;
    }
    if (p == entry->end_code) {
      break;
    }
  }
  return false;
}

static bool IsNontrivial(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(sta):
    case W65C02_OP(stx):
    case W65C02_OP(sty):
    case W65C02_OP(stz):
    case W65C02_OP(inc):
    case W65C02_OP(dec):
    case W65C02_OP(inx):
    case W65C02_OP(dex):
    case W65C02_OP(iny):
    case W65C02_OP(dey):
    case W65C02_OP(asl):
    case W65C02_OP(lsr):
    case W65C02_OP(rol):
    case W65C02_OP(ror):
    case W65C02_OP(adc):
    case W65C02_OP(sbc):
    case W65C02_OP(and):
    case W65C02_OP(ora):
    case W65C02_OP(eor):
    case W65C02_OP(bit):
    case W65C02_OP(tsb):
    case W65C02_OP(trb):
      return true;
    case W65C02_OP(cmp):
    case W65C02_OP(cpx):
    case W65C02_OP(cpy):
      return !ImmediateZero(inst);
    default:
      return false;
  }
}

static bool OmitClone(void* ctx, TargetInstruction* inst) {
  return IsLabelOp(inst) || IsSpillOp(inst) || IsReloadOp(inst) ||
         inst->observable_checkpoint || IsMeta(ctx, inst);
}

static int Zero(void* ctx) {
  (void)ctx;
  return 0;
}

static int ExtraNone(void* ctx, uint64_t incoming) {
  (void)ctx;
  (void)incoming;
  return 0;
}

static uint64_t NoIncoming(void* ctx) {
  (void)ctx;
  return 0;
}

static TargetInstruction* NewLabel(void* ctx) {
  (void)ctx;
  TargetInstruction* lbl = TargetNewInstruction((TargetOpcode)W65C02_OP(label));
  lbl->flags = TARGET_INST_PROCESSED;
  return lbl;
}

static TargetInstruction* NewBranch(void* ctx, TargetInstruction* label) {
  (void)ctx;
  TargetInstruction* jmp =
      TargetNewInstruction1((TargetOpcode)W65C02_OP(jmp), label);
  jmp->flags = TARGET_INST_PROCESSED | k6502BlockEnd |
               ((int)kAddrModeAbsolute << 16);
  return jmp;
}

static TargetInstruction* NewReturn(void* ctx) {
  (void)ctx;
  TargetInstruction* rts = TargetNewInstruction((TargetOpcode)W65C02_OP(rts));
  rts->flags = TARGET_INST_PROCESSED | ((int)kAddrModeImplied << 16);
  return rts;
}

void W65C02ShrinkWrap(W65C02RegisterAllocator* allocator) {
  (void)allocator;
  TargetShrinkWrapOps ops;
  memset(&ops, 0, sizeof(ops));
  ops.max_slow_targets = 1;
  ops.num_saved_bits = 0;
  ops.function_ok = FunctionOk;
  ops.is_save = IsEnter;
  ops.is_restore = IsLeave;
  ops.is_return = IsReturnOp;
  ops.is_spill = IsSpillOp;
  ops.is_reload = IsReloadOp;
  ops.is_label = IsLabelOp;
  ops.is_meta = IsMeta;
  ops.no_fallthrough = NoFallthrough;
  ops.branch_target = BranchTarget;
  ops.classify = Classify;
  ops.record_reg = RecordReg;
  ops.incoming_args = NoIncoming;
  ops.scratch_base = Zero;
  ops.extra_scratch = ExtraNone;
  ops.cold_entry_split = ColdEntrySplit;
  ops.is_nontrivial = IsNontrivial;
  ops.omit_from_clone = OmitClone;
  ops.new_label = NewLabel;
  ops.new_branch = NewBranch;
  ops.new_return = NewReturn;
  TargetShrinkWrap(&allocator->g->base, &ops);
}
