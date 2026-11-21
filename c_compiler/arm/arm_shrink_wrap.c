//
//  arm_shrink_wrap.c
//  c_compiler
//
//  ARM side of the shared shrink-wrap pass.  The fast path renames
//  callee-saved r4-r8 and r10 onto r9, r12, and unused argument registers.
//  The prologue, moved to the framed edge, copies them back.  r9 and r12 are
//  not saved by this compiler.  r11 is the frame pointer.
//

#include "arm_reg_alloc.h"

#include <string.h>
#include "arm_codegen.h"
#include "arm_machine.h"
#include "compiler.h"
#include "target_basic_block.h"
#include "target_shrink_wrap.h"

typedef struct {
  ARMRegisterAllocator* allocator;
} ARMSW;

// Callee-saved registers the prologue actually saves.  r9 is the dedicated
// scratch and r11 is the frame pointer.
static const int kSaved[] = {4, 5, 6, 7, 8, 10};
enum { kNumSaved = 6 };

static int SavedBit(int n) {
  for (int i = 0; i < kNumSaved; i++) {
    if (kSaved[i] == n) {
      return i;
    }
  }
  return -1;
}

static bool FramePseudo(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(fp):
    case ARM_OP(sp):
    case ARM_OP(lr):
    case ARM_OP(tp):
      return true;
    default:
      return false;
  }
}

static bool IsSave(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(save);
}
static bool IsRestore(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(restore);
}
static bool IsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(ret);
}
static bool IsSpill(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(spill);
}
static bool IsReload(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(reload);
}
static bool IsLabel(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)ARM_OP(label);
}

static bool IsMeta(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst->observable_checkpoint || ARMIsConst(inst)) {
    return true;
  }
  ARMOpcode op = (ARMOpcode)inst->opcode;
  if (op >= ARM_OP(eq) && op <= ARM_OP(al)) {
    return true;
  }
  switch (op) {
    case ARM_OP(label):
    case ARM_OP(loc):
    case ARM_OP(ivarreg):
    case ARM_OP(fvarreg):
    case ARM_OP(symbol):
    case ARM_OP(literal):
    case ARM_OP(resulti):
    case ARM_OP(resultf):
    case ARM_OP(resultd):
    case ARM_OP(r0):
    case ARM_OP(r1):
    case ARM_OP(r2):
    case ARM_OP(r3):
    case ARM_OP(r4):
    case ARM_OP(r5):
    case ARM_OP(r6):
    case ARM_OP(r7):
    case ARM_OP(r8):
    case ARM_OP(r9):
      return true;
    default:
      return false;
  }
}

static bool NoFallthrough(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst == NULL) {
    return false;
  }
  ARMOpcode op = (ARMOpcode)inst->opcode;
  if (op == ARM_OP(ret) || op == ARM_OP(br)) {
    return true;
  }
  return op == ARM_OP(b) && inst->operand[0] != NULL &&
         inst->operand[0]->opcode == (TargetOpcode)ARM_OP(al);
}

static TargetInstruction** BranchTarget(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(b):
    case ARM_OP(cbz):
    case ARM_OP(cbnz):
      return &inst->operand[1];
    case ARM_OP(tbz):
    case ARM_OP(tbnz):
      return &inst->operand[2];
    default:
      return NULL;
  }
}

static bool RecordReg(void* ctx, TargetRegister* tr, uint64_t* saved,
                      uint64_t* temp, uint64_t* arg) {
  (void)ctx;
  if (tr == NULL) {
    return true;
  }
  ARMRegister* r = (ARMRegister*)tr;
  if (r->type == kARMRegTypeFloat) {
    return r->base.num < ARM_FP_SAVED_START || r->base.num > ARM_FP_SAVED_END;
  }
  int n = r->base.num;
  if (n == ARM_FP_REG || n == ARM_SP_REG || n == ARM_LR_REG || n == ARM_PC_REG) {
    return false;
  }
  int bit = SavedBit(n);
  if (bit >= 0) {
    *saved |= 1ull << bit;
  } else if (n >= 0 && n <= 3) {
    *temp |= 1ull << n;
    *arg |= 1ull << n;
  } else if (n == ARM_TMP_REG || n == ARM_IP_REG) {
    *temp |= 1ull << n;
  }
  return true;
}

static bool FunctionOk(void* ctx, TargetGenerator* gen) {
  ARMSW* sw = ctx;
  ARMGenerator* g = sw->allocator->g;
  if (!OptLevel2() || gen->varargs || g->uses_dynamic_stack ||
      g->saved_regs.length != 0 || g->struct_return_reg >= 0 ||
      gen->exception_edges.length != 0 || g->exception_ranges.length != 0 ||
      gen->entry_block == NULL || gen->entry_block->in_edges.length != 0) {
    return false;
  }
  return gen->stack_frame_size > 0 || gen->num_calls > 0 || g->not_leaf ||
         BitSetCount(&sw->allocator->used_int_regs) > 0 ||
         BitSetCount(&sw->allocator->used_float_regs) > 0 ||
         sw->allocator->max_spilled_region_size > 0;
}

static TargetSWKind Classify(void* ctx, TargetInstruction* inst, bool allow_save,
                             bool allow_reload, uint64_t* saved, uint64_t* temp,
                             uint64_t* arg) {
  ARMOpcode op = (ARMOpcode)inst->opcode;
  if (allow_save &&
      (op == ARM_OP(save) || op == ARM_OP(fp) || op == ARM_OP(sp) ||
       op == ARM_OP(lr))) {
    return kTargetSWMeta;
  }
  for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
    if (FramePseudo(inst->operand[k])) {
      return kTargetSWBad;
    }
  }
  if (op == ARM_OP(spill)) {
    if (inst->reg == NULL || inst->operand[1] == NULL ||
        !TargetIsConst(inst->operand[1])) {
      return kTargetSWBad;
    }
    if (SavedBit(inst->reg->num) < 0) {
      return kTargetSWBad;
    }
    *saved |= 1ull << SavedBit(inst->reg->num);
    return kTargetSWSpill;
  }
  if (op == ARM_OP(reload)) {
    if (!allow_reload || inst->reg == NULL || inst->operand[0] == NULL ||
        inst->operand[0]->opcode != (TargetOpcode)ARM_OP(spill) ||
        inst->operand[0]->operand[1] == NULL ||
        !TargetIsConst(inst->operand[0]->operand[1]) ||
        !RecordReg(ctx, inst->reg, saved, temp, arg)) {
      return kTargetSWBad;
    }
    return kTargetSWReload;
  }
  if (IsMeta(ctx, inst)) {
    return kTargetSWMeta;
  }
  switch (op) {
    case ARM_OP(bl):
    case ARM_OP(blr):
    case ARM_OP(br):
    case ARM_OP(save):
    case ARM_OP(restore):
    case ARM_OP(ret):
    case ARM_OP(asm):
    case ARM_OP(named_label):
    case ARM_OP(tp):
    case ARM_OP(fp):
    case ARM_OP(sp):
    case ARM_OP(lr):
    case ARM_OP(structreturn):
    case ARM_OP(atomic_load):
    case ARM_OP(atomic_store):
    case ARM_OP(atomic_fetch_add):
    case ARM_OP(atomic_fetch_sub):
    case ARM_OP(atomic_add_fetch):
    case ARM_OP(atomic_sub_fetch):
    case ARM_OP(atomic_compare_exchange_bool):
    case ARM_OP(atomic_compare_exchange_val):
    case ARM_OP(atomic_compare_exchange_n):
    case ARM_OP(atomic_fence):
      return kTargetSWBad;
    default:
      return kTargetSWExec;
  }
}

static uint64_t IncomingArgs(void* ctx) {
  ARMGenerator* g = ((ARMSW*)ctx)->allocator->g;
  uint64_t mask = 0;
  for (int k = 0; k < ARM_NUM_INT_ARGS; k++) {
    if (g->int_argument_registers[k] != NULL) {
      mask |= 1ull << k;
    }
  }
  return mask;
}

static int ScratchBase(void* ctx) {
  (void)ctx;
  return 4;  // r0-r3
}

static int ExtraScratch(void* ctx, uint64_t incoming) {
  (void)ctx;
  (void)incoming;
  return 2;  // r9 and r12
}

static bool CoalesceCopy(void* ctx, TargetInstruction* inst,
                         uint64_t written_args, int* saved_bit, int* arg_index) {
  ARMGenerator* g = ((ARMSW*)ctx)->allocator->g;
  if ((inst->opcode != (TargetOpcode)ARM_OP(mov) &&
       inst->opcode != (TargetOpcode)ARM_OP(mv)) ||
      inst->dest == NULL || inst->dest->reg == NULL || inst->operand[0] == NULL ||
      inst->operand[1] != NULL || inst->operand[0]->users.length != 1 ||
      inst->operand[0]->opcode < (TargetOpcode)ARM_OP(r0) ||
      inst->operand[0]->opcode > (TargetOpcode)ARM_OP(r3)) {
    return false;
  }
  int k = (int)inst->operand[0]->opcode - (int)ARM_OP(r0);
  ARMRegister* dr = (ARMRegister*)inst->dest->reg;
  int bit = dr->type == kARMRegTypeInt ? SavedBit(dr->base.num) : -1;
  if (bit < 0 || (k == 0 && !g->base.is_void) ||
      (written_args & (1ull << k)) != 0) {
    return false;
  }
  *saved_bit = bit;
  *arg_index = k;
  return true;
}

static void TakeTemps(void* ctx, uint64_t temps, uint64_t* taken) {
  (void)ctx;
  *taken |= temps & ((1ull << 4) - 1);
  if ((temps & (1ull << ARM_TMP_REG)) != 0) {
    *taken |= 1ull << ARM_TMP_REG;
  }
  if ((temps & (1ull << ARM_IP_REG)) != 0) {
    *taken |= 1ull << ARM_IP_REG;
  }
}

static int SavedRegId(void* ctx, int bit) {
  (void)ctx;
  return kSaved[bit];
}

static int ScratchPool(void* ctx, int* out, int cap, uint64_t taken,
                       uint64_t total_arg, uint64_t incoming) {
  ARMGenerator* g = ((ARMSW*)ctx)->allocator->g;
  int n = 0;
  int first[] = {ARM_TMP_REG, ARM_IP_REG};
  for (int i = 0; i < 2 && n < cap; i++) {
    int r = first[i];
    if ((taken & (1ull << r)) == 0) {
      out[n++] = r;
    }
  }
  for (int r = 3; r >= 0 && n < cap; r--) {
    if (r == 0 && !g->base.is_void) {
      continue;
    }
    if ((incoming & (1ull << r)) == 0 && (total_arg & (1ull << r)) == 0 &&
        (taken & (1ull << r)) == 0) {
      out[n++] = r;
    }
  }
  return n;
}

static bool SpillInfo(void* ctx, TargetInstruction* spill, int* saved_bit,
                      int* reg_id, int64_t* offset) {
  (void)ctx;
  if (spill->reg == NULL || spill->operand[1] == NULL ||
      !TargetIsConst(spill->operand[1])) {
    return false;
  }
  int bit = SavedBit(spill->reg->num);
  if (bit < 0) {
    return false;
  }
  *saved_bit = bit;
  *reg_id = spill->reg->num;
  *offset = TargetIntValue(spill->operand[1]);
  return true;
}

static bool ReloadOffset(void* ctx, TargetInstruction* reload, int64_t* offset) {
  (void)ctx;
  if (reload->operand[0] == NULL || reload->operand[0]->operand[1] == NULL ||
      !TargetIsConst(reload->operand[0]->operand[1])) {
    return false;
  }
  *offset = TargetIntValue(reload->operand[0]->operand[1]);
  return true;
}

static bool ClobbersSaved(void* ctx, TargetInstruction* proto,
                          TargetInstruction* inst) {
  (void)ctx;
  TargetRegister* wreg = inst->dest != NULL ? inst->dest->reg : inst->reg;
  if (wreg != proto->reg) {
    return false;
  }
  return !ARMIsVarRegister(proto->operand[0]) || inst->dest != proto->operand[0];
}

static bool ColdEntrySplit(void* ctx, TargetBasicBlock* entry) {
  (void)ctx;
  TargetInstruction* last_cmp = NULL;
  for (TargetInstruction* p = entry->code; p != NULL; p = TargetNext(p)) {
    if (p->opcode == (TargetOpcode)ARM_OP(cmp)) {
      last_cmp = p;
    }
    if (p == entry->end_code) {
      break;
    }
  }
  TargetInstruction* end = entry->end_code;
  return (end != NULL && (end->opcode == (TargetOpcode)ARM_OP(cbz) ||
                          end->opcode == (TargetOpcode)ARM_OP(cbnz))) ||
         (last_cmp != NULL && last_cmp->operand[1] != NULL &&
          TargetIsConst(last_cmp->operand[1]) &&
          TargetIntValue(last_cmp->operand[1]) == 0);
}

static bool IsNontrivial(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  ARMOpcode op = (ARMOpcode)inst->opcode;
  if (ARMIsStore(inst) || op == ARM_OP(tst) || op == ARM_OP(and) ||
      op == ARM_OP(add) || op == ARM_OP(sub) || op == ARM_OP(eor) ||
      op == ARM_OP(orr) || op == ARM_OP(mul)) {
    return true;
  }
  if (op == ARM_OP(cmp) && inst->operand[1] != NULL) {
    if (TargetIsConst(inst->operand[1])) {
      return TargetIntValue(inst->operand[1]) != 0;
    }
    return true;
  }
  return false;
}

static bool OmitClone(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return inst->observable_checkpoint || IsLabel(inst) || IsSpill(inst) ||
         inst->opcode == (TargetOpcode)ARM_OP(loc);
}

static bool RemapId(void* ctx, TargetRegister* reg, int* reg_id) {
  (void)ctx;
  ARMRegister* r = (ARMRegister*)reg;
  if (r->type != kARMRegTypeInt || r->base.num < 0 || r->base.num >= ARM_NUM_INT_REGS) {
    return false;
  }
  *reg_id = r->base.num;
  return true;
}

static int ValueWidth(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return ARMGetRegisterSize(inst) == kSize32Bit ? 0 : 1;
}

static bool IsVar(TargetInstruction* inst) { return ARMIsVarRegister(inst); }

static TargetInstruction* NewLabel(void* ctx) {
  (void)ctx;
  TargetInstruction* lbl = TargetNewInstruction((TargetOpcode)ARM_OP(label));
  lbl->flags = TARGET_INST_PROCESSED;
  return lbl;
}

static TargetInstruction* NewBranch(void* ctx, TargetInstruction* label) {
  ARMSW* sw = ctx;
  TargetInstruction* br = TargetNewInstruction2(
      (TargetOpcode)ARM_OP(b), ARMAlwaysCondition(sw->allocator->g), label);
  br->flags = TARGET_INST_PROCESSED;
  return br;
}

static TargetInstruction* NewReturn(void* ctx) {
  (void)ctx;
  TargetInstruction* ret = TargetNewInstruction((TargetOpcode)ARM_OP(ret));
  ret->flags = TARGET_INST_PROCESSED;
  return ret;
}

static TargetInstruction* RegProxy(void* ctx, TargetGenerator* gen, int reg_id,
                                   int width) {
  ARMSW* sw = ctx;
  int size = width == 0 ? kSize32Bit : kSize64Bit;
  TargetInstruction* p = TargetNewInstruction((TargetOpcode)ARM_OP(ivarreg));
  p->reg = &sw->allocator->int_regs[reg_id].base;
  p->block = gen->entry_block;
  p->flags = TARGET_INST_PROCESSED | ((size & 3) << 16);
  TargetTrackOrphanInstruction(gen, p);
  return p;
}

static TargetInstruction* CloneReload(void* ctx, TargetGenerator* gen,
                                     TargetInstruction* reload, int src_reg_id) {
  int wide = ARMGetRegisterSize(reload) == kSize32Bit ? 0 : 1;
  TargetInstruction* src = RegProxy(ctx, gen, src_reg_id, wide);
  TargetInstruction* cp = TargetNewInstruction1((TargetOpcode)ARM_OP(mov), src);
  cp->reg = reload->reg;
  cp->flags = TARGET_INST_PROCESSED | ((ARMGetRegisterSize(reload) & 3) << 16);
  return cp;
}

static TargetInstruction* CreateMove(void* ctx, int saved_reg_id,
                                    TargetInstruction* fast_proxy,
                                    TargetInstruction* slow_proxy) {
  (void)slow_proxy;
  ARMSW* sw = ctx;
  TargetInstruction* mv =
      TargetNewInstruction1((TargetOpcode)ARM_OP(mov), fast_proxy);
  mv->reg = &sw->allocator->int_regs[saved_reg_id].base;
  mv->flags = TARGET_INST_PROCESSED | (kSize32Bit << 16);
  return mv;
}

static TargetInstruction* CloneSpill(void* ctx, TargetInstruction* proto,
                                    int reg_id) {
  ARMSW* sw = ctx;
  TargetInstruction* store = TargetNewInstruction2(
      (TargetOpcode)ARM_OP(spill), proto->operand[0], proto->operand[1]);
  store->reg = &sw->allocator->int_regs[reg_id].base;
  store->flags = proto->flags | TARGET_INST_PROCESSED;
  return store;
}

void ARMShrinkWrap(ARMRegisterAllocator* allocator) {
  ARMSW ctx = {allocator};
  TargetShrinkWrapOps ops;
  memset(&ops, 0, sizeof(ops));
  ops.ctx = &ctx;
  ops.max_slow_targets = 1;
  ops.num_saved_bits = kNumSaved;
  ops.function_ok = FunctionOk;
  ops.is_save = IsSave;
  ops.is_restore = IsRestore;
  ops.is_return = IsReturn;
  ops.is_spill = IsSpill;
  ops.is_reload = IsReload;
  ops.is_label = IsLabel;
  ops.is_meta = IsMeta;
  ops.no_fallthrough = NoFallthrough;
  ops.branch_target = BranchTarget;
  ops.classify = Classify;
  ops.record_reg = RecordReg;
  ops.incoming_args = IncomingArgs;
  ops.scratch_base = ScratchBase;
  ops.extra_scratch = ExtraScratch;
  ops.coalesce_copy = CoalesceCopy;
  ops.take_temps = TakeTemps;
  ops.saved_reg_id = SavedRegId;
  ops.scratch_pool = ScratchPool;
  ops.spill_info = SpillInfo;
  ops.reload_offset = ReloadOffset;
  ops.clobbers_saved = ClobbersSaved;
  ops.cold_entry_split = ColdEntrySplit;
  ops.is_nontrivial = IsNontrivial;
  ops.omit_from_clone = OmitClone;
  ops.remap_id = RemapId;
  ops.value_width = ValueWidth;
  ops.is_var = IsVar;
  ops.new_label = NewLabel;
  ops.new_branch = NewBranch;
  ops.new_return = NewReturn;
  ops.proxy = RegProxy;
  ops.clone_reload = CloneReload;
  ops.create_move = CreateMove;
  ops.clone_spill = CloneSpill;
  TargetShrinkWrap(&allocator->g->base, &ops);
}
