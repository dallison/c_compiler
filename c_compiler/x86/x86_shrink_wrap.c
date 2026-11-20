//
//  x86_shrink_wrap.c
//  c_compiler
//
//  x86 side of the shared shrink-wrap pass.  Register numbers here are the
//  allocator's slots, which the emitter turns into names.  On x86-64 the
//  callee-saved slots are rbx (9) and r12-r15 (18-21).  On i386 they are
//  ebx, esi, and edi.  The frame pointer and stack pointer stay off the
//  fast path.  The prologue, moved to the framed edge, copies scratch
//  registers back.
//

#include "x86_reg_alloc.h"

#include <string.h>
#include "compiler.h"
#include "target_basic_block.h"
#include "target_shrink_wrap.h"
#include "x86_codegen.h"
#include "x86_profile.h"

typedef struct {
  X86RegisterAllocator* allocator;
} X86SW;

static const X86Profile* Prof(X86SW* sw) { return sw->allocator->rv->profile; }

static int SavedBit(const X86Profile* p, int slot) {
  static const int k64[] = {9, 18, 19, 20, 21};
  static const int k32[] = {3, 6, 7};
  const int* saved = p->is_64bit ? k64 : k32;
  int n = p->is_64bit ? 5 : 3;
  for (int i = 0; i < n; i++) {
    if (saved[i] == slot) {
      return i;
    }
  }
  return -1;
}

static int SavedSlot(const X86Profile* p, int bit) {
  static const int k64[] = {9, 18, 19, 20, 21};
  static const int k32[] = {3, 6, 7};
  return (p->is_64bit ? k64 : k32)[bit];
}

static bool CallerSaved(const X86Profile* p, int slot) {
  if (p->is_64bit) {
    return slot == 1 || slot == 3 || slot == 5 ||
           (slot >= 10 && slot <= 15);
  }
  return slot >= 0 && slot <= 2;
}

static bool FramePseudo(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(fp):
    case X86_OP(sp):
    case X86_OP(tp):
      return true;
    default:
      return false;
  }
}

static bool IsSave(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_OP(save);
}
static bool IsRestore(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_OP(restore);
}
static bool IsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_OP(ret);
}
static bool IsSpill(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_OP(spill);
}
static bool IsReload(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_OP(reload);
}
static bool IsLabel(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)X86_OP(label);
}

static bool IsCondBranch(X86Opcode op) {
  switch (op) {
    case X86_OP(je):
    case X86_OP(jne):
    case X86_OP(jl):
    case X86_OP(jge):
    case X86_OP(jb):
    case X86_OP(jae):
    case X86_OP(jz):
    case X86_OP(jnz):
      return true;
    default:
      return false;
  }
}

static bool IsMeta(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst->observable_checkpoint || X86IsConst(inst)) {
    return true;
  }
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(label):
    case X86_OP(loc):
    case X86_OP(ivarreg):
    case X86_OP(fvarreg):
    case X86_OP(symbol):
    case X86_OP(literal):
    case X86_OP(resulti):
    case X86_OP(resultf):
    case X86_OP(resultd):
    case X86_OP(a0):
    case X86_OP(a1):
    case X86_OP(a2):
    case X86_OP(a3):
    case X86_OP(a4):
    case X86_OP(a5):
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
  X86Opcode op = (X86Opcode)inst->opcode;
  return op == X86_OP(ret) || op == X86_OP(jmp);
}

// A conditional branch is either a bare label (operand 0) or a compare whose
// target label sits in operand 2.  jz and jnz keep the tested value in
// operand 0 and the label in operand 1.
static TargetInstruction** BranchTarget(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  X86Opcode op = (X86Opcode)inst->opcode;
  if (op == X86_OP(jmp)) {
    return &inst->operand[0];
  }
  if (op == X86_OP(jz) || op == X86_OP(jnz)) {
    return &inst->operand[1];
  }
  if (IsCondBranch(op)) {
    return inst->operand[2] != NULL ? &inst->operand[2] : &inst->operand[0];
  }
  return NULL;
}

static bool RecordReg(void* ctx, TargetRegister* tr, uint64_t* saved,
                      uint64_t* temp, uint64_t* arg) {
  X86SW* sw = ctx;
  const X86Profile* p = Prof(sw);
  if (tr == NULL) {
    return true;
  }
  X86Register* r = (X86Register*)tr;
  if (r->type == kX86RegTypeFloat) {
    return r->base.num < 8;
  }
  int n = r->base.num;
  if (n == p->sp_reg || n == p->fp_reg) {
    return false;
  }
  int bit = SavedBit(p, n);
  if (bit >= 0) {
    *saved |= 1ull << bit;
  } else if (CallerSaved(p, n)) {
    *temp |= 1ull << n;
    if (n >= p->int_arg_start && n <= p->int_arg_end) {
      *arg |= 1ull << n;
    }
  }
  return true;
}

static bool FunctionOk(void* ctx, TargetGenerator* gen) {
  X86SW* sw = ctx;
  X86Generator* g = sw->allocator->rv;
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
  X86Opcode op = (X86Opcode)inst->opcode;
  if (allow_save && (op == X86_OP(save) || op == X86_OP(fp) || op == X86_OP(sp))) {
    return kTargetSWMeta;
  }
  for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
    if (FramePseudo(inst->operand[k])) {
      return kTargetSWBad;
    }
  }
  if (op == X86_OP(spill)) {
    const X86Profile* p = Prof(ctx);
    if (inst->reg == NULL || inst->operand[1] == NULL ||
        !TargetIsConst(inst->operand[1]) || SavedBit(p, inst->reg->num) < 0) {
      return kTargetSWBad;
    }
    *saved |= 1ull << SavedBit(p, inst->reg->num);
    return kTargetSWSpill;
  }
  if (op == X86_OP(reload)) {
    if (!allow_reload || inst->reg == NULL || inst->operand[0] == NULL ||
        inst->operand[0]->opcode != (TargetOpcode)X86_OP(spill) ||
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
    case X86_OP(call):
    case X86_OP(callf):
    case X86_OP(rcall):
    case X86_OP(rcallf):
    case X86_OP(save):
    case X86_OP(restore):
    case X86_OP(ret):
    case X86_OP(asm):
    case X86_OP(named_label):
    case X86_OP(tp):
    case X86_OP(fp):
    case X86_OP(sp):
    case X86_OP(structreturn):
    case X86_OP(atomic_compare_exchange_bool):
    case X86_OP(atomic_compare_exchange_val):
    case X86_OP(atomic_compare_exchange_n):
    case X86_OP(atomic_fetch_add_sub):
    case X86_OP(atomic64):
      return kTargetSWBad;
    default:
      return kTargetSWExec;
  }
}

static uint64_t IncomingArgs(void* ctx) {
  X86SW* sw = ctx;
  X86Generator* g = sw->allocator->rv;
  const X86Profile* p = g->profile;
  uint64_t mask = 0;
  int n = p->num_int_args;
  if (n > X86_MAX_INT_ARGS) {
    n = X86_MAX_INT_ARGS;
  }
  for (int k = 0; k < n; k++) {
    if (g->int_argument_registers[k] != NULL) {
      mask |= 1ull << (p->int_arg_start + k);
    }
  }
  return mask;
}

// Slots the x86-64 emitter never names inside a straight-line expansion.  The
// emitter uses %r11, %r10, %rax, %rcx and %rdx as temporaries (division,
// shifts, and compares with two constants), so none of them can hold a renamed
// value.  i386 has no such slot: ecx, edx and eax are all used that way, so its
// pool is empty and a function that needs a scratch register is left alone.
static const int kX64ScratchSlots[] = {15, 14, 11, 10};  // r9, r8, rsi, rdi.

// The pool and budget never include a register that a function's arguments
// occupy.  x86-64 can rename into the four slots above; i386 cannot rename at
// all.
static int ScratchBase(void* ctx) {
  (void)ctx;
  return 0;
}

static int ExtraScratch(void* ctx, uint64_t incoming) {
  const X86Profile* p = Prof(ctx);
  int extra = 0;
  if (!p->is_64bit) {
    return 0;
  }
  for (size_t i = 0; i < sizeof(kX64ScratchSlots) / sizeof(kX64ScratchSlots[0]); i++) {
    if ((incoming & (1ull << kX64ScratchSlots[i])) == 0) {
      extra++;
    }
  }
  return extra;
}

static bool CoalesceCopy(void* ctx, TargetInstruction* inst,
                         uint64_t written_args, int* saved_bit, int* arg_index) {
  X86SW* sw = ctx;
  const X86Profile* p = Prof(sw);
  if (inst->opcode != (TargetOpcode)X86_OP(mv) || inst->dest == NULL ||
      inst->dest->reg == NULL || inst->operand[0] == NULL ||
      inst->operand[1] != NULL || inst->operand[0]->users.length != 1 ||
      p->num_int_args <= 0 ||
      inst->operand[0]->opcode < (TargetOpcode)X86_OP(a0) ||
      inst->operand[0]->opcode >= (TargetOpcode)X86_OP(a0) + p->num_int_args) {
    return false;
  }
  int slot = p->int_arg_start +
             ((int)inst->operand[0]->opcode - (int)X86_OP(a0));
  X86Register* dr = (X86Register*)inst->dest->reg;
  int bit = dr->type == kX86RegTypeInt ? SavedBit(p, dr->base.num) : -1;
  if (bit < 0 || (slot == p->int_arg_start && !sw->allocator->rv->base.is_void) ||
      (written_args & (1ull << slot)) != 0) {
    return false;
  }
  *saved_bit = bit;
  *arg_index = slot;
  return true;
}

static void TakeTemps(void* ctx, uint64_t temps, uint64_t* taken) {
  const X86Profile* p = Prof(ctx);
  for (int slot = 0; slot < 32; slot++) {
    if (CallerSaved(p, slot) && (temps & (1ull << slot)) != 0) {
      *taken |= 1ull << slot;
    }
  }
}

static int SavedRegId(void* ctx, int bit) {
  return SavedSlot(Prof(ctx), bit);
}

static int ScratchPool(void* ctx, int* out, int cap, uint64_t taken,
                       uint64_t total_arg, uint64_t incoming) {
  const X86Profile* p = Prof(ctx);
  int n = 0;
  if (!p->is_64bit) {
    return 0;
  }
  for (size_t i = 0; i < sizeof(kX64ScratchSlots) / sizeof(kX64ScratchSlots[0]) && n < cap;
       i++) {
    int slot = kX64ScratchSlots[i];
    if ((incoming & (1ull << slot)) == 0 && (total_arg & (1ull << slot)) == 0 &&
        (taken & (1ull << slot)) == 0) {
      out[n++] = slot;
    }
  }
  return n;
}

static bool SpillInfo(void* ctx, TargetInstruction* spill, int* saved_bit,
                      int* reg_id, int64_t* offset) {
  const X86Profile* p = Prof(ctx);
  if (spill->reg == NULL || spill->operand[1] == NULL ||
      !TargetIsConst(spill->operand[1])) {
    return false;
  }
  int bit = SavedBit(p, spill->reg->num);
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
  return !X86IsVarRegister(proto->operand[0]) || inst->dest != proto->operand[0];
}

static bool IsZero(TargetInstruction* op) {
  return op != NULL && TargetIsConst(op) && TargetIntValue(op) == 0;
}

static bool ColdEntrySplit(void* ctx, TargetBasicBlock* entry) {
  (void)ctx;
  TargetInstruction* end = entry->end_code;
  if (end == NULL) {
    return false;
  }
  X86Opcode op = (X86Opcode)end->opcode;
  if (op != X86_OP(je) && op != X86_OP(jz) && op != X86_OP(jne) &&
      op != X86_OP(jnz)) {
    return false;
  }
  for (TargetInstruction* p = entry->code; p != NULL; p = TargetNext(p)) {
    X86Opcode cop = (X86Opcode)p->opcode;
    if ((cop == X86_OP(cmp) || cop == X86_OP(test)) &&
        (IsZero(p->operand[0]) || IsZero(p->operand[1]))) {
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
  X86Opcode op = (X86Opcode)inst->opcode;
  if (X86IsStore(inst) || op == X86_OP(add) || op == X86_OP(addl) ||
      op == X86_OP(sub) || op == X86_OP(subl) || op == X86_OP(and) ||
      op == X86_OP(or) || op == X86_OP(xor) || op == X86_OP(lea)) {
    return true;
  }
  if ((op == X86_OP(cmp) || op == X86_OP(test)) &&
      !IsZero(inst->operand[0]) && !IsZero(inst->operand[1])) {
    return true;
  }
  return false;
}

static bool OmitClone(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return inst->observable_checkpoint || IsLabel(inst) || IsSpill(inst) ||
         inst->opcode == (TargetOpcode)X86_OP(loc);
}

static bool RemapId(void* ctx, TargetRegister* reg, int* reg_id) {
  (void)ctx;
  X86Register* r = (X86Register*)reg;
  if (r->type != kX86RegTypeInt || r->base.num < 0 || r->base.num >= X86_MAX_INT_REGS) {
    return false;
  }
  *reg_id = r->base.num;
  return true;
}

static int ValueWidth(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return ((inst->flags >> 16) & 3) == 1 ? 0 : 1;
}

static bool IsVar(TargetInstruction* inst) { return X86IsVarRegister(inst); }

static TargetInstruction* NewLabel(void* ctx) {
  (void)ctx;
  TargetInstruction* lbl = TargetNewInstruction((TargetOpcode)X86_OP(label));
  lbl->flags = TARGET_INST_PROCESSED;
  return lbl;
}

static TargetInstruction* NewBranch(void* ctx, TargetInstruction* label) {
  (void)ctx;
  TargetInstruction* br = TargetNewInstruction1((TargetOpcode)X86_OP(jmp), label);
  br->flags = TARGET_INST_PROCESSED;
  return br;
}

static TargetInstruction* NewReturn(void* ctx) {
  (void)ctx;
  TargetInstruction* ret = TargetNewInstruction((TargetOpcode)X86_OP(ret));
  ret->flags = TARGET_INST_PROCESSED;
  return ret;
}

static TargetInstruction* RegProxy(void* ctx, TargetGenerator* gen, int reg_id,
                                   int width) {
  X86SW* sw = ctx;
  int size = width == 0 ? 1 : 2;
  TargetInstruction* p = TargetNewInstruction((TargetOpcode)X86_OP(ivarreg));
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
  TargetInstruction* cp = TargetNewInstruction1((TargetOpcode)X86_OP(mv), src);
  cp->reg = reload->reg;
  cp->flags = reload->flags | TARGET_INST_PROCESSED;
  return cp;
}

static TargetInstruction* CreateMove(void* ctx, int saved_reg_id,
                                    TargetInstruction* fast_proxy,
                                    TargetInstruction* slow_proxy) {
  (void)slow_proxy;
  X86SW* sw = ctx;
  int size = Prof(sw)->is_64bit ? 2 : 1;
  TargetInstruction* mv = TargetNewInstruction1((TargetOpcode)X86_OP(mv), fast_proxy);
  mv->reg = &sw->allocator->int_regs[saved_reg_id].base;
  mv->flags = TARGET_INST_PROCESSED | ((size & 3) << 16);
  return mv;
}

static TargetInstruction* CloneSpill(void* ctx, TargetInstruction* proto,
                                    int reg_id) {
  X86SW* sw = ctx;
  TargetInstruction* store = TargetNewInstruction2(
      (TargetOpcode)X86_OP(spill), proto->operand[0], proto->operand[1]);
  store->reg = &sw->allocator->int_regs[reg_id].base;
  store->flags = proto->flags | TARGET_INST_PROCESSED;
  return store;
}

void X86ShrinkWrap(X86RegisterAllocator* allocator) {
  X86SW ctx = {allocator};
  TargetShrinkWrapOps ops;
  memset(&ops, 0, sizeof(ops));
  ops.ctx = &ctx;
  ops.max_slow_targets = 1;
  ops.num_saved_bits = Prof(&ctx)->is_64bit ? 5 : 3;
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
