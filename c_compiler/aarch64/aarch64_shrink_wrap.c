//
//  aarch64_shrink_wrap.c
//  c_compiler
//
//  AArch64 side of the shared shrink-wrap pass.  The fast path renames
//  callee-saved x19-x28 onto caller-saved registers and the prologue, moved
//  to the framed edge, copies them back.
//

#include "aarch64_reg_alloc.h"

#include <stdlib.h>
#include <string.h>
#include "aarch64_codegen.h"
#include "aarch64_machine.h"
#include "compiler.h"
#include "target_basic_block.h"
#include "target_shrink_wrap.h"

typedef struct {
  AARCH64RegisterAllocator* allocator;
  TargetInstruction* al;
} A64SW;

static bool IsSavedReg(AARCH64Register* reg) {
  int num = reg->base.num;
  switch (reg->type) {
    case kAARCH64RegTypeInt:
      return num >= AARCH64_INT_SAVED_START && num <= AARCH64_INT_SAVED_END;
    case kAARCH64RegTypeFloat:
      return num >= AARCH64_FP_SAVED_START && num <= AARCH64_FP_SAVED_END;
  }
  return false;
}

static bool NoFallthrough(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst == NULL) {
    return false;
  }
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (op == AARCH64_OP(ret) || op == AARCH64_OP(br)) {
    return true;
  }
  return op == AARCH64_OP(b) && inst->operand[0] != NULL &&
         inst->operand[0]->opcode == (TargetOpcode)AARCH64_OP(al);
}

static TargetInstruction** BranchTarget(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(b):
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz):
      return &inst->operand[1];
    case AARCH64_OP(tbz):
    case AARCH64_OP(tbnz):
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
  AARCH64Register* r = (AARCH64Register*)tr;
  if (r->type == kAARCH64RegTypeFloat) {
    return !IsSavedReg(r);
  }
  int n = r->base.num;
  if (n == AARCH64_FP_REG || n == AARCH64_LR_REG || n == AARCH64_SP_REG ||
      n == AARCH64_IP1_REG) {
    return false;
  }
  if (n >= AARCH64_INT_SAVED_START && n <= AARCH64_INT_SAVED_END) {
    *saved |= 1ull << (n - AARCH64_INT_SAVED_START);
  } else if (n >= AARCH64_INT_TEMP_START && n <= AARCH64_INT_TEMP_END) {
    *temp |= 1ull << (n - AARCH64_INT_TEMP_START);
  } else if (n >= 0 && n <= 8) {
    *arg |= 1ull << n;
  } else if (n == AARCH64_IP2_REG) {
    *arg |= 1ull << 17;
  }
  return true;
}

static bool IsMeta(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  if (inst->observable_checkpoint || AARCH64IsConst(inst)) {
    return true;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(label):
    case AARCH64_OP(loc):
    case AARCH64_OP(ivarreg):
    case AARCH64_OP(fvarreg):
    case AARCH64_OP(symbol):
    case AARCH64_OP(literal):
    case AARCH64_OP(oplsl):
    case AARCH64_OP(zr):
    case AARCH64_OP(resulti):
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
    case AARCH64_OP(eq):
    case AARCH64_OP(ne):
    case AARCH64_OP(cs):
    case AARCH64_OP(hs):
    case AARCH64_OP(cc):
    case AARCH64_OP(lo):
    case AARCH64_OP(mi):
    case AARCH64_OP(pl):
    case AARCH64_OP(vs):
    case AARCH64_OP(vc):
    case AARCH64_OP(hi):
    case AARCH64_OP(ls):
    case AARCH64_OP(ge):
    case AARCH64_OP(lt):
    case AARCH64_OP(gt):
    case AARCH64_OP(le):
    case AARCH64_OP(al):
      return true;
    default:
      return false;
  }
}

static TargetInstruction* RegProxy(void* ctx, TargetGenerator* gen, int reg_id,
                                   int width) {
  A64SW* sw = ctx;
  int size = width == 0 ? kSize32Bit : kSize64Bit;
  TargetInstruction* p = TargetNewInstruction(TARGET_OP(ivarreg));
  p->reg = &sw->allocator->int_regs[reg_id].base;
  p->block = gen->entry_block;
  p->flags = TARGET_INST_PROCESSED | ((size & 3) << 16);
  TargetTrackOrphanInstruction(gen, p);
  return p;
}

static bool FunctionOk(void* ctx, TargetGenerator* gen) {
  A64SW* sw = ctx;
  AARCH64RegisterAllocator* allocator = sw->allocator;
  AARCH64Generator* g = allocator->g;
  if (!OptLevel2() || gen->varargs || g->moves_sp ||
      g->saved_regs.length != 0 || g->struct_return_reg >= 0 ||
      gen->exception_edges.length != 0 || gen->entry_block == NULL ||
      gen->entry_block->in_edges.length != 0) {
    return false;
  }
  return gen->stack_frame_size > 0 || gen->num_calls > 0 || g->not_leaf ||
         BitSetCount(&allocator->used_int_regs) > 0 ||
         BitSetCount(&allocator->used_float_regs) > 0 ||
         allocator->max_spilled_region_size > 0;
}

static bool IsSave(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)AARCH64_OP(save);
}
static bool IsRestore(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)AARCH64_OP(restore);
}
static bool IsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)AARCH64_OP(ret);
}
static bool IsSpill(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)AARCH64_OP(spill);
}
static bool IsReload(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)AARCH64_OP(reload);
}
static bool IsLabel(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)AARCH64_OP(label);
}

static TargetSWKind Classify(void* ctx, TargetInstruction* inst, bool allow_save,
                             bool allow_reload, uint64_t* saved, uint64_t* temp,
                             uint64_t* arg) {
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (allow_save &&
      (op == AARCH64_OP(save) || op == AARCH64_OP(fp) || op == AARCH64_OP(sp) ||
       op == AARCH64_OP(lr))) {
    return kTargetSWMeta;
  }
  if (op == AARCH64_OP(spill)) {
    if (inst->reg == NULL || inst->operand[1] == NULL ||
        !TargetIsConst(inst->operand[1]) ||
        GetRegisterSize(inst) == kSize128Bit) {
      return kTargetSWBad;
    }
    AARCH64Register* sr = (AARCH64Register*)inst->reg;
    if (sr->type != kAARCH64RegTypeInt ||
        sr->base.num < AARCH64_INT_SAVED_START ||
        sr->base.num > AARCH64_INT_SAVED_END) {
      return kTargetSWBad;
    }
    *saved |= 1ull << (sr->base.num - AARCH64_INT_SAVED_START);
    return kTargetSWSpill;
  }
  if (op == AARCH64_OP(reload)) {
    if (!allow_reload || inst->reg == NULL || inst->operand[0] == NULL ||
        inst->operand[0]->opcode != (TargetOpcode)AARCH64_OP(spill) ||
        inst->operand[0]->operand[1] == NULL ||
        !TargetIsConst(inst->operand[0]->operand[1]) ||
        GetRegisterSize(inst->operand[0]) == kSize128Bit) {
      return kTargetSWBad;
    }
    AARCH64Register* rr = (AARCH64Register*)inst->reg;
    if (rr->type != kAARCH64RegTypeInt || rr->base.num == AARCH64_IP1_REG ||
        rr->base.num == AARCH64_IP2_REG ||
        !RecordReg(ctx, inst->reg, saved, temp, arg)) {
      return kTargetSWBad;
    }
    return kTargetSWReload;
  }
  if (IsMeta(ctx, inst)) {
    return kTargetSWMeta;
  }
  switch (op) {
    case AARCH64_OP(bl):
    case AARCH64_OP(blr):
    case AARCH64_OP(br):
    case AARCH64_OP(save):
    case AARCH64_OP(restore):
    case AARCH64_OP(ret):
    case AARCH64_OP(asm):
    case AARCH64_OP(named_label):
    case AARCH64_OP(tp):
    case AARCH64_OP(fp):
    case AARCH64_OP(sp):
    case AARCH64_OP(lr):
    case AARCH64_OP(xr):
    case AARCH64_OP(r9):
    case AARCH64_OP(structreturn):
    case AARCH64_OP(regarg):
    case AARCH64_OP(nrvoval):
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
    case AARCH64_OP(ldxr):
    case AARCH64_OP(ldaxr):
    case AARCH64_OP(stxr):
    case AARCH64_OP(stlxr):
    case AARCH64_OP(clrex):
    case AARCH64_OP(dmb):
      return kTargetSWBad;
    default:
      return kTargetSWExec;
  }
}

static uint64_t IncomingArgs(void* ctx) {
  AARCH64Generator* g = ((A64SW*)ctx)->allocator->g;
  uint64_t mask = 0;
  for (int k = 0; k < AARCH64_NUM_INT_ARGS; k++) {
    if (g->int_argument_registers[k] != NULL) {
      mask |= 1ull << k;
    }
  }
  return mask;
}

static int ScratchBase(void* ctx) {
  (void)ctx;
  return AARCH64_INT_TEMP_END - AARCH64_INT_TEMP_START + 1;
}

static int ExtraScratch(void* ctx, uint64_t incoming) {
  AARCH64Generator* g = ((A64SW*)ctx)->allocator->g;
  int extra = 0;
  for (int k = 1; k <= 7; k++) {
    if ((incoming & (1ull << k)) == 0) {
      extra++;
    }
  }
  if (g->base.stack_frame_size < 0x100000) {
    extra++;
  }
  extra++;  // x17 when the fast region does not use it.
  return extra;
}

static bool CoalesceCopy(void* ctx, TargetInstruction* inst,
                         uint64_t written_args, int* saved_bit, int* arg_index) {
  AARCH64Generator* g = ((A64SW*)ctx)->allocator->g;
  if ((inst->opcode != (TargetOpcode)AARCH64_OP(mov) &&
       inst->opcode != (TargetOpcode)AARCH64_OP(mv)) ||
      inst->dest == NULL || inst->dest->reg == NULL || inst->operand[0] == NULL ||
      inst->operand[1] != NULL || inst->operand[0]->users.length != 1 ||
      inst->operand[0]->opcode < (TargetOpcode)AARCH64_OP(r0) ||
      inst->operand[0]->opcode > (TargetOpcode)AARCH64_OP(r7)) {
    return false;
  }
  int k = (int)inst->operand[0]->opcode - (int)AARCH64_OP(r0);
  AARCH64Register* dr = (AARCH64Register*)inst->dest->reg;
  if (dr->type != kAARCH64RegTypeInt ||
      dr->base.num < AARCH64_INT_SAVED_START ||
      dr->base.num > AARCH64_INT_SAVED_END ||
      (k == 0 && !g->base.is_void) || (written_args & (1ull << k)) != 0) {
    return false;
  }
  *saved_bit = dr->base.num - AARCH64_INT_SAVED_START;
  *arg_index = k;
  return true;
}

static void TakeTemps(void* ctx, uint64_t temps, uint64_t* taken) {
  (void)ctx;
  for (int t = AARCH64_INT_TEMP_START; t <= AARCH64_INT_TEMP_END; t++) {
    if ((temps & (1ull << (t - AARCH64_INT_TEMP_START))) != 0) {
      *taken |= 1ull << t;
    }
  }
}

static int SavedRegId(void* ctx, int bit) {
  (void)ctx;
  return AARCH64_INT_SAVED_START + bit;
}

static int ScratchPool(void* ctx, int* out, int cap, uint64_t taken,
                       uint64_t total_arg, uint64_t incoming) {
  AARCH64Generator* g = ((A64SW*)ctx)->allocator->g;
  int n = 0;
  for (int t = AARCH64_INT_TEMP_END; t >= AARCH64_INT_TEMP_START && n < cap; t--) {
    if ((taken & (1ull << t)) == 0) {
      out[n++] = t;
    }
  }
  for (int k = 7; k >= 1 && n < cap; k--) {
    if ((incoming & (1ull << k)) == 0 && (total_arg & (1ull << k)) == 0 &&
        (taken & (1ull << k)) == 0) {
      out[n++] = k;
    }
  }
  if (n < cap && g->base.stack_frame_size < 0x100000 &&
      (total_arg & (1ull << 8)) == 0 && (taken & (1ull << 8)) == 0) {
    out[n++] = 8;
  }
  if (n < cap && (total_arg & (1ull << 17)) == 0 && (taken & (1ull << 17)) == 0) {
    out[n++] = 17;
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
  int c = spill->reg->num;
  *saved_bit = c - AARCH64_INT_SAVED_START;
  *reg_id = c;
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
  return !AARCH64IsVarRegister(proto->operand[0]) || inst->dest != proto->operand[0];
}

static bool ColdEntrySplit(void* ctx, TargetBasicBlock* entry) {
  (void)ctx;
  TargetInstruction* last_cmp = NULL;
  for (TargetInstruction* p = entry->code; p != NULL; p = TargetNext(p)) {
    if (p->opcode == (TargetOpcode)AARCH64_OP(cmp)) {
      last_cmp = p;
    }
    if (p == entry->end_code) {
      break;
    }
  }
  TargetInstruction* end = entry->end_code;
  return (end != NULL &&
          (end->opcode == (TargetOpcode)AARCH64_OP(cbz) ||
           end->opcode == (TargetOpcode)AARCH64_OP(cbnz)) &&
          GetRegisterSize(end) == kSize64Bit) ||
         (last_cmp != NULL && GetRegisterSize(last_cmp) == kSize64Bit &&
          last_cmp->operand[1] != NULL &&
          (last_cmp->operand[1]->opcode == (TargetOpcode)AARCH64_OP(zr) ||
           (TargetIsConst(last_cmp->operand[1]) &&
            TargetIntValue(last_cmp->operand[1]) == 0)));
}

static bool IsNontrivial(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (AARCH64IsStore(inst) || op == AARCH64_OP(tst) || op == AARCH64_OP(and) ||
      op == AARCH64_OP(ands) || op == AARCH64_OP(sub) || op == AARCH64_OP(subs)) {
    return true;
  }
  if (op == AARCH64_OP(cmp) && inst->operand[1] != NULL) {
    if (TargetIsConst(inst->operand[1])) {
      return TargetIntValue(inst->operand[1]) != 0;
    }
    return inst->operand[1]->opcode != (TargetOpcode)AARCH64_OP(zr);
  }
  return false;
}

static bool OmitClone(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return inst->observable_checkpoint || IsLabel(inst) || IsSpill(inst) ||
         inst->opcode == (TargetOpcode)AARCH64_OP(loc);
}

static bool RemapId(void* ctx, TargetRegister* reg, int* reg_id) {
  (void)ctx;
  AARCH64Register* r = (AARCH64Register*)reg;
  if (r->type != kAARCH64RegTypeInt || r->base.num < 0 || r->base.num >= 33) {
    return false;
  }
  *reg_id = r->base.num;
  return true;
}

static int ValueWidth(void* ctx, TargetInstruction* inst) {
  (void)ctx;
  return GetRegisterSize(inst) == kSize32Bit ? 0 : 1;
}

static bool IsVar(TargetInstruction* inst) { return AARCH64IsVarRegister(inst); }

static TargetInstruction* AlwaysCond(A64SW* sw, TargetGenerator* gen) {
  if (sw->al != NULL) {
    return sw->al;
  }
  for (TargetInstruction* p = TargetFirstInstruction(gen); p != NULL;
       p = TargetNext(p)) {
    if (p->opcode == (TargetOpcode)AARCH64_OP(al)) {
      sw->al = p;
      return p;
    }
  }
  sw->al = TargetNewInstruction((TargetOpcode)AARCH64_OP(al));
  sw->al->block = gen->entry_block;
  sw->al->flags = TARGET_INST_PROCESSED;
  TargetEmitBefore(gen, sw->al, gen->entry_block->code);
  gen->entry_block->code = sw->al;
  return sw->al;
}

static TargetInstruction* NewLabel(void* ctx) {
  (void)ctx;
  TargetInstruction* lbl = TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
  lbl->flags = TARGET_INST_PROCESSED;
  return lbl;
}

static TargetInstruction* NewBranch(void* ctx, TargetInstruction* label) {
  A64SW* sw = ctx;
  TargetInstruction* br = TargetNewInstruction2(
      (TargetOpcode)AARCH64_OP(b), AlwaysCond(sw, &sw->allocator->g->base),
      label);
  br->flags = TARGET_INST_PROCESSED;
  return br;
}

static TargetInstruction* NewReturn(void* ctx) {
  (void)ctx;
  TargetInstruction* ret = TargetNewInstruction((TargetOpcode)AARCH64_OP(ret));
  ret->flags = TARGET_INST_PROCESSED;
  return ret;
}

static TargetInstruction* CloneReload(void* ctx, TargetGenerator* gen,
                                     TargetInstruction* reload, int src_reg_id) {
  A64SW* sw = ctx;
  int rsz = GetRegisterSize(reload) == kSize32Bit ? kSize32Bit : kSize64Bit;
  TargetInstruction* src = RegProxy(sw, gen, src_reg_id, rsz == kSize32Bit ? 0 : 1);
  TargetInstruction* cp = SetInstructionSize(
      TargetNewInstruction1((TargetOpcode)AARCH64_OP(mov), src), rsz);
  cp->reg = reload->reg;
  cp->flags |= TARGET_INST_PROCESSED;
  return cp;
}

static TargetInstruction* CreateMove(void* ctx, int saved_reg_id,
                                    TargetInstruction* fast_proxy,
                                    TargetInstruction* slow_proxy) {
  A64SW* sw = ctx;
  TargetInstruction* mv = SetInstructionSize(
      TargetNewInstruction1((TargetOpcode)AARCH64_OP(mov), fast_proxy),
      kSize64Bit);
  mv->reg = &sw->allocator->int_regs[saved_reg_id].base;
  mv->dest = slow_proxy;
  mv->flags |= TARGET_INST_PROCESSED;
  return mv;
}

static TargetInstruction* CloneSpill(void* ctx, TargetInstruction* proto,
                                    int reg_id) {
  A64SW* sw = ctx;
  TargetInstruction* store = TargetNewInstruction2(
      (TargetOpcode)AARCH64_OP(spill), proto->operand[0], proto->operand[1]);
  store->reg = &sw->allocator->int_regs[reg_id].base;
  store->flags = proto->flags | TARGET_INST_PROCESSED;
  return store;
}

static void NoteWrapped(void* ctx, int reg_id) {
  ((A64SW*)ctx)->allocator->shrink_wrapped_regs |= UINT32_C(1) << reg_id;
}

static TargetInstruction* EmitTrampolines(void* ctx, TargetGenerator* gen,
                                         TargetInstruction* cursor,
                                         TargetInstruction* save_label,
                                         int num_slow,
                                         TargetInstruction** tramp_labels) {
  A64SW* sw = ctx;
  TargetInstruction* al = AlwaysCond(sw, gen);
  for (int s = 0; s < num_slow; s++) {
    TargetBasicBlock* tblk = NewTargetBasicBlock(gen->basic_blocks.length);
    VectorAppend(&gen->basic_blocks, tblk);
    TargetInstruction* tlbl = NewLabel(ctx);
    cursor = TargetShrinkWrapAppend(gen, tblk, tlbl, cursor);
    tramp_labels[s] = tlbl;

    TargetInstruction* idx_const = SetInstructionSize(
        TargetGetIntConstant(gen, NULL, kTargetType32Bit, (int64_t)s),
        kSize32Bit);
    TargetInstruction* mov_idx = SetInstructionSize(
        TargetNewInstruction1((TargetOpcode)AARCH64_OP(movz), idx_const),
        kSize32Bit);
    mov_idx->reg = &sw->allocator->int_regs[AARCH64_IP1_REG].base;
    mov_idx->flags |= TARGET_INST_PROCESSED;
    cursor = TargetShrinkWrapAppend(gen, tblk, mov_idx, cursor);
    if (s + 1 < num_slow) {
      TargetInstruction* jsave = TargetNewInstruction2(
          (TargetOpcode)AARCH64_OP(b), al, save_label);
      jsave->flags = TARGET_INST_PROCESSED;
      cursor = TargetShrinkWrapAppend(gen, tblk, jsave, cursor);
    }
  }
  return cursor;
}

static TargetInstruction* EmitDispatch(void* ctx, TargetGenerator* gen,
                                      TargetBasicBlock* save_block,
                                      TargetInstruction* cursor,
                                      TargetInstruction** slow_labels,
                                      int num_slow) {
  A64SW* sw = ctx;
  TargetInstruction* al = AlwaysCond(sw, gen);
  TargetInstruction* ip1 = RegProxy(sw, gen, AARCH64_IP1_REG, 0);
  TargetInstruction* eq = TargetNewInstruction((TargetOpcode)AARCH64_OP(eq));
  eq->block = save_block;
  eq->flags = TARGET_INST_PROCESSED;
  TargetTrackOrphanInstruction(gen, eq);
  for (int s = 0; s + 1 < num_slow; s++) {
    if (s == 0) {
      TargetInstruction* cbz = SetInstructionSize(
          TargetNewInstruction2((TargetOpcode)AARCH64_OP(cbz), ip1,
                                slow_labels[0]),
          kSize32Bit);
      cbz->flags |= TARGET_INST_PROCESSED;
      cursor = TargetShrinkWrapAppend(gen, save_block, cbz, cursor);
    } else {
      TargetInstruction* c_s = SetInstructionSize(
          TargetGetIntConstant(gen, NULL, kTargetType32Bit, (int64_t)s),
          kSize32Bit);
      TargetInstruction* cmp_s = SetInstructionSize(
          TargetNewInstruction2((TargetOpcode)AARCH64_OP(cmp), ip1, c_s),
          kSize32Bit);
      cmp_s->flags |= TARGET_INST_PROCESSED;
      cursor = TargetShrinkWrapAppend(gen, save_block, cmp_s, cursor);
      TargetInstruction* beq =
          TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), eq, slow_labels[s]);
      beq->flags = TARGET_INST_PROCESSED;
      cursor = TargetShrinkWrapAppend(gen, save_block, beq, cursor);
    }
  }
  TargetInstruction* jlast =
      TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al,
                            slow_labels[num_slow - 1]);
  jlast->flags = TARGET_INST_PROCESSED;
  return TargetShrinkWrapAppend(gen, save_block, jlast, cursor);
}

void AARCH64ShrinkWrap(AARCH64RegisterAllocator* allocator) {
  A64SW ctx = {allocator, NULL};
  TargetShrinkWrapOps ops;
  memset(&ops, 0, sizeof(ops));
  ops.ctx = &ctx;
  ops.max_slow_targets = 16;
  ops.num_saved_bits = AARCH64_INT_SAVED_END - AARCH64_INT_SAVED_START + 1;
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
  ops.note_wrapped = NoteWrapped;
  ops.emit_trampolines = EmitTrampolines;
  ops.emit_dispatch = EmitDispatch;
  TargetShrinkWrap(&allocator->g->base, &ops);
}
