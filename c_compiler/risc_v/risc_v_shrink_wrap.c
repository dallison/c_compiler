//
//  risc_v_shrink_wrap.c
//  c_compiler
//
//  RISC-V side of the shared shrink-wrap pass.  The fast path renames
//  callee-saved s1 (x9) and s2-s11 (x18-x27) onto t0-t6 and unused argument
//  registers.  The prologue, moved to the framed edge, copies them back.
//  s0 is the frame pointer and ra is the return address.
//

#include "risc_v_reg_alloc.h"

#include <string.h>
#include "compiler.h"
#include "risc_v_codegen.h"
#include "risc_v_machine.h"
#include "target_basic_block.h"
#include "target_shrink_wrap.h"

typedef struct {
  RVRegisterAllocator* allocator;
} RVSW;

enum { kNumSaved = 11 };

static int SavedBit(int n) {
  if (n == RV_INT_SAVED_END_1) {
    return 0;
  }
  if (n >= RV_INT_SAVED_START_2 && n <= RV_INT_SAVED_END_2) {
    return 1 + (n - RV_INT_SAVED_START_2);
  }
  return -1;
}

static int SavedRegFromBit(int bit) {
  if (bit == 0) {
    return RV_INT_SAVED_END_1;
  }
  return RV_INT_SAVED_START_2 + (bit - 1);
}

static bool IsTemp(int n) {
  return (n >= RV_INT_TEMP_START_1 && n <= RV_INT_TEMP_END_1) ||
         (n >= RV_INT_TEMP_START_2 && n <= RV_INT_TEMP_END_2);
}

static bool FramePseudo(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(fp):
    case RV_OP(sp):
    case RV_OP(tp):
      return true;
    default:
      return false;
  }
}

static bool IsSave(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)RV_OP(save);
}
static bool IsRestore(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)RV_OP(restore);
}
static bool IsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)RV_OP(ret);
}
static bool IsSpill(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)RV_OP(spill);
}
static bool IsReload(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)RV_OP(reload);
}
static bool IsLabel(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)RV_OP(label);
}

static bool IsMeta(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst->observable_checkpoint || RVIsConst(inst)) {
    return true;
  }
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(label):
    case RV_OP(loc):
    case RV_OP(ivarreg):
    case RV_OP(fvarreg):
    case RV_OP(symbol):
    case RV_OP(literal):
    case RV_OP(resulti):
    case RV_OP(resultf):
    case RV_OP(resultd):
    case RV_OP(x0):
    case RV_OP(a0):
    case RV_OP(a1):
    case RV_OP(a2):
    case RV_OP(a3):
    case RV_OP(a4):
    case RV_OP(a5):
    case RV_OP(a6):
    case RV_OP(a7):
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
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(ret):
    case RV_OP(j):
    case RV_OP(jr):
      return true;
    default:
      return false;
  }
}

static TargetInstruction** BranchTarget(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(beq):
    case RV_OP(bne):
    case RV_OP(blt):
    case RV_OP(bge):
    case RV_OP(bltu):
    case RV_OP(bgeu):
      return &inst->operand[2];
    case RV_OP(beqz):
    case RV_OP(bnez):
      return &inst->operand[1];
    case RV_OP(j):
      return &inst->operand[0];
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
  RVRegister* r = (RVRegister*)tr;
  if (r->type == kRVRegTypeFloat) {
    return !((r->base.num >= RV_FP_SAVED_START_1 &&
              r->base.num <= RV_FP_SAVED_END_1) ||
             (r->base.num >= RV_FP_SAVED_START_2 &&
              r->base.num <= RV_FP_SAVED_END_2));
  }
  if (r->type == kRVRegTypeVector) {
    return r->base.num < RV_VECTOR_SAVED_START ||
           r->base.num > RV_VECTOR_SAVED_END;
  }
  int n = r->base.num;
  if (n == RV_RET_REG || n == RV_SP_REG || n == RV_FP_REG) {
    return false;
  }
  int bit = SavedBit(n);
  if (bit >= 0) {
    *saved |= 1ull << bit;
  } else if (IsTemp(n)) {
    *temp |= 1ull << n;
  } else if (n >= RV_INT_ARG_START && n <= RV_INT_ARG_END) {
    *arg |= 1ull << n;
  }
  return true;
}

static bool FunctionOk(void* ctx, TargetGenerator* gen) {
  RVSW* sw = ctx;
  RVGenerator* g = sw->allocator->rv;
  if (!OptLevel2() || gen->varargs || g->saved_regs.length != 0 ||
      g->struct_return_reg >= 0 || gen->exception_edges.length != 0 ||
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
  RVOpcode op = (RVOpcode)inst->opcode;
  if (allow_save && (op == RV_OP(save) || op == RV_OP(fp) || op == RV_OP(sp))) {
    return kTargetSWMeta;
  }
  for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
    if (FramePseudo(inst->operand[k])) {
      return kTargetSWBad;
    }
  }
  if (op == RV_OP(spill)) {
    if (inst->reg == NULL || inst->operand[1] == NULL ||
        !TargetIsConst(inst->operand[1]) || SavedBit(inst->reg->num) < 0) {
      return kTargetSWBad;
    }
    *saved |= 1ull << SavedBit(inst->reg->num);
    return kTargetSWSpill;
  }
  if (op == RV_OP(reload)) {
    if (!allow_reload || inst->reg == NULL || inst->operand[0] == NULL ||
        inst->operand[0]->opcode != (TargetOpcode)RV_OP(spill) ||
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
    case RV_OP(jal):
    case RV_OP(jalr):
    case RV_OP(call):
    case RV_OP(rcall):
    case RV_OP(callf):
    case RV_OP(rcallf):
    case RV_OP(ecall):
    case RV_OP(save):
    case RV_OP(restore):
    case RV_OP(ret):
    case RV_OP(asm):
    case RV_OP(named_label):
    case RV_OP(tp):
    case RV_OP(fp):
    case RV_OP(sp):
    case RV_OP(structreturn):
    case RV_OP(atomic_load):
    case RV_OP(atomic_store):
    case RV_OP(atomic_fetch_add):
    case RV_OP(atomic_fetch_sub):
    case RV_OP(atomic_add_fetch):
    case RV_OP(atomic_sub_fetch):
    case RV_OP(atomic_compare_exchange_bool):
    case RV_OP(atomic_compare_exchange_val):
    case RV_OP(atomic_compare_exchange_n):
    case RV_OP(atomic_fence):
      return kTargetSWBad;
    default:
      return kTargetSWExec;
  }
}

static uint64_t IncomingArgs(void* ctx) {
  RVGenerator* g = ((RVSW*)ctx)->allocator->rv;
  uint64_t mask = 0;
  for (int k = 0; k < RV_NUM_INT_ARGS; k++) {
    if (g->int_argument_registers[k] != NULL) {
      mask |= 1ull << (RV_INT_ARG_START + k);
    }
  }
  return mask;
}

static int ScratchBase(void* ctx) {
  (void)ctx;
  // t1, t2 and t3-t6 (t0 is excluded from the pool).
  return (RV_INT_TEMP_END_1 - RV_INT_TEMP_START_1) +
         (RV_INT_TEMP_END_2 - RV_INT_TEMP_START_2 + 1);
}

static int ExtraScratch(void* ctx, uint64_t incoming) {
  RVGenerator* g = ((RVSW*)ctx)->allocator->rv;
  int extra = 0;
  for (int n = RV_INT_ARG_START; n <= RV_INT_ARG_END; n++) {
    if (n == RV_INT_ARG_START && !g->base.is_void) {
      continue;
    }
    if ((incoming & (1ull << n)) == 0) {
      extra++;
    }
  }
  return extra;
}

static bool CoalesceCopy(void* ctx, TargetInstruction* inst,
                         uint64_t written_args, int* saved_bit, int* arg_index) {
  RVGenerator* g = ((RVSW*)ctx)->allocator->rv;
  if (inst->opcode != (TargetOpcode)RV_OP(mv) || inst->dest == NULL ||
      inst->dest->reg == NULL || inst->operand[0] == NULL ||
      inst->operand[1] != NULL || inst->operand[0]->users.length != 1 ||
      inst->operand[0]->opcode < (TargetOpcode)RV_OP(a0) ||
      inst->operand[0]->opcode > (TargetOpcode)RV_OP(a7)) {
    return false;
  }
  int k = RV_INT_ARG_START +
          ((int)inst->operand[0]->opcode - (int)RV_OP(a0));
  RVRegister* dr = (RVRegister*)inst->dest->reg;
  int bit = dr->type == kRVRegTypeInt ? SavedBit(dr->base.num) : -1;
  if (bit < 0 || (k == RV_INT_ARG_START && !g->base.is_void) ||
      (written_args & (1ull << k)) != 0) {
    return false;
  }
  *saved_bit = bit;
  *arg_index = k;
  return true;
}

static void TakeTemps(void* ctx, uint64_t temps, uint64_t* taken) {
  (void)ctx;
  for (int n = 0; n < 32; n++) {
    if (IsTemp(n) && (temps & (1ull << n)) != 0) {
      *taken |= 1ull << n;
    }
  }
}

static int SavedRegId(void* ctx, int bit) {
  (void)ctx;
  return SavedRegFromBit(bit);
}

static int ScratchPool(void* ctx, int* out, int cap, uint64_t taken,
                       uint64_t total_arg, uint64_t incoming) {
  RVGenerator* g = ((RVSW*)ctx)->allocator->rv;
  int n = 0;
  for (int t = RV_INT_TEMP_END_2; t >= RV_INT_TEMP_START_2 && n < cap; t--) {
    if ((taken & (1ull << t)) == 0) {
      out[n++] = t;
    }
  }
  // x5 (t0) is skipped: the emitter materializes large offsets in t0, so a
  // value renamed there would be clobbered inside the fast region.
  for (int t = RV_INT_TEMP_END_1; t > RV_INT_TEMP_START_1 && n < cap; t--) {
    if ((taken & (1ull << t)) == 0) {
      out[n++] = t;
    }
  }
  for (int a = RV_INT_ARG_END; a >= RV_INT_ARG_START && n < cap; a--) {
    if (a == RV_INT_ARG_START && !g->base.is_void) {
      continue;
    }
    if ((incoming & (1ull << a)) == 0 && (total_arg & (1ull << a)) == 0 &&
        (taken & (1ull << a)) == 0) {
      out[n++] = a;
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
  return !RVIsVarRegister(proto->operand[0]) || inst->dest != proto->operand[0];
}

static bool AgainstZero(TargetInstruction* op) {
  if (op == NULL) {
    return false;
  }
  if (op->opcode == (TargetOpcode)RV_OP(x0)) {
    return true;
  }
  return TargetIsConst(op) && TargetIntValue(op) == 0;
}

static bool ColdEntrySplit(void* ctx, TargetBasicBlock* entry) {
  (void)ctx;
  TargetInstruction* end = entry->end_code;
  if (end == NULL) {
    return false;
  }
  RVOpcode op = (RVOpcode)end->opcode;
  if (op == RV_OP(beqz) || op == RV_OP(bnez)) {
    return true;
  }
  if ((op == RV_OP(beq) || op == RV_OP(bne)) &&
      (AgainstZero(end->operand[0]) || AgainstZero(end->operand[1]))) {
    return true;
  }
  return false;
}

static bool IsNontrivial(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  RVOpcode op = (RVOpcode)inst->opcode;
  if (RVIsStore(inst) || op == RV_OP(add) || op == RV_OP(addi) ||
      op == RV_OP(sub) || op == RV_OP(and) || op == RV_OP(andi) ||
      op == RV_OP(or) || op == RV_OP(ori) || op == RV_OP(xor) ||
      op == RV_OP(xori) || op == RV_OP(mul)) {
    return true;
  }
  if ((op == RV_OP(beq) || op == RV_OP(bne) || op == RV_OP(blt) ||
       op == RV_OP(bge)) &&
      !AgainstZero(inst->operand[0]) && !AgainstZero(inst->operand[1])) {
    return true;
  }
  return false;
}

static bool OmitClone(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return inst->observable_checkpoint || IsLabel(inst) || IsSpill(inst) ||
         inst->opcode == (TargetOpcode)RV_OP(loc);
}

static bool RemapId(void* ctx, TargetRegister* reg, int* reg_id) {
  (void)ctx;
  RVRegister* r = (RVRegister*)reg;
  if (r->type != kRVRegTypeInt || r->base.num < 0 || r->base.num >= 32) {
    return false;
  }
  *reg_id = r->base.num;
  return true;
}

static int ValueWidth(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return ((inst->flags >> 16) & 3) == 1 ? 0 : 1;
}

static bool IsVar(TargetInstruction* inst) { return RVIsVarRegister(inst); }

static TargetInstruction* NewLabel(void* ctx) {
  (void)ctx;
  TargetInstruction* lbl = TargetNewInstruction((TargetOpcode)RV_OP(label));
  lbl->flags = TARGET_INST_PROCESSED;
  return lbl;
}

static TargetInstruction* NewBranch(void* ctx, TargetInstruction* label) {
  (void)ctx;
  TargetInstruction* br = TargetNewInstruction1((TargetOpcode)RV_OP(j), label);
  br->flags = TARGET_INST_PROCESSED;
  return br;
}

static TargetInstruction* NewReturn(void* ctx) {
  (void)ctx;
  TargetInstruction* ret = TargetNewInstruction((TargetOpcode)RV_OP(ret));
  ret->flags = TARGET_INST_PROCESSED;
  return ret;
}

static TargetInstruction* RegProxy(void* ctx, TargetGenerator* gen, int reg_id,
                                   int width) {
  RVSW* sw = ctx;
  int size = width == 0 ? 1 : 2;
  TargetInstruction* p = TargetNewInstruction((TargetOpcode)RV_OP(ivarreg));
  p->reg = &sw->allocator->int_regs[reg_id].base;
  p->block = gen->entry_block;
  p->flags = TARGET_INST_PROCESSED | ((size & 3) << 16);
  TargetTrackOrphanInstruction(gen, p);
  return p;
}

static TargetInstruction* CloneReload(void* ctx, TargetGenerator* gen,
                                     TargetInstruction* reload, int src_reg_id) {
  int wide = ((reload->flags >> 16) & 3) == 1 ? 0 : 1;
  TargetInstruction* src = RegProxy(ctx, gen, src_reg_id, wide);
  TargetInstruction* cp = TargetNewInstruction1((TargetOpcode)RV_OP(mv), src);
  cp->reg = reload->reg;
  cp->flags = reload->flags | TARGET_INST_PROCESSED;
  return cp;
}

static TargetInstruction* CreateMove(void* ctx, int saved_reg_id,
                                    TargetInstruction* fast_proxy,
                                    TargetInstruction* slow_proxy) {
  (void)slow_proxy;
  RVSW* sw = ctx;
  TargetInstruction* mv = TargetNewInstruction1((TargetOpcode)RV_OP(mv), fast_proxy);
  mv->reg = &sw->allocator->int_regs[saved_reg_id].base;
  mv->flags = TARGET_INST_PROCESSED | (2 << 16);
  return mv;
}

static TargetInstruction* CloneSpill(void* ctx, TargetInstruction* proto,
                                    int reg_id) {
  RVSW* sw = ctx;
  TargetInstruction* store = TargetNewInstruction2(
      (TargetOpcode)RV_OP(spill), proto->operand[0], proto->operand[1]);
  store->reg = &sw->allocator->int_regs[reg_id].base;
  store->flags = proto->flags | TARGET_INST_PROCESSED;
  return store;
}

void RVShrinkWrap(RVRegisterAllocator* allocator) {
  RVSW ctx = {allocator};
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
  TargetShrinkWrap(&allocator->rv->base, &ops);
}
