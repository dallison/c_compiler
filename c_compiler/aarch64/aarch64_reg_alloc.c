//
//  aarch64_reg_alloc.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "aarch64_reg_alloc.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include "aarch64_codegen.h"
#include "aarch64_machine.h"
#include "target_basic_block.h"
#include "compiler.h"

static void InitializeRegister(AARCH64Register* reg, int num, AARCH64RegisterType type) {
  TargetRegisterInit(&reg->base, num);
  reg->type = type;
}

void AARCH64RegisterAllocatorInit(AARCH64RegisterAllocator* allocator,
                             struct AARCH64Generator* g) {
  allocator->g = g;

  for (int i = 0; i < AARCH64_NUM_INT_REGS; i++) {
    InitializeRegister(&allocator->int_regs[i], i, kAARCH64RegTypeInt);
  }

  for (int i = 0; i < AARCH64_NUM_FLOAT_REGS; i++) {
    InitializeRegister(&allocator->float_regs[i], i, kAARCH64RegTypeFloat);
  }

  BitSetInit(&allocator->used_int_regs);
  BitSetInit(&allocator->used_float_regs);
  allocator->max_spilled_region_size = 0;
  allocator->shrink_wrapped_regs = 0;
}

AARCH64RegisterAllocator* NewAARCH64RegisterAllocator(struct AARCH64Generator* g) {
  AARCH64RegisterAllocator* reg_alloc = malloc(sizeof(AARCH64RegisterAllocator));
  AARCH64RegisterAllocatorInit(reg_alloc, g);
  return reg_alloc;
}

void AARCH64RegisterAllocatorDestruct(AARCH64RegisterAllocator* allocator) {
  BitSetDestruct(&allocator->used_int_regs);
  BitSetDestruct(&allocator->used_float_regs);
}

void AARCH64RegisterAllocatorDelete(AARCH64RegisterAllocator* alloc) {
  AARCH64RegisterAllocatorDestruct(alloc);
  free(alloc);
}

// Size for registers.
#define kSize32Bit 1
#define kSize64Bit 2


static AARCH64RegisterType RegisterTypeFromInstruction(TargetInstruction* inst) {
  // A value whose opcode does not imply a register class (e.g. a `tmp` merge
  // slot for ?: / && / ||) records its floating-pointness via this flag so the
  // allocator places it in an FP register rather than defaulting to integer.
  if ((inst->flags & AARCH64_INST_FP) != 0) {
    return kAARCH64RegTypeFloat;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(constf):
    case AARCH64_OP(constd):
    case AARCH64_OP(fmv_s):
    case AARCH64_OP(fmv_d):
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
    case AARCH64_OP(fldr):
    case  AARCH64_OP(fstr):
    case  AARCH64_OP(fadd):
    case  AARCH64_OP(fsub):
    case  AARCH64_OP(fmul):
    case  AARCH64_OP(fdiv):
    case  AARCH64_OP(fsqrt):
    case  AARCH64_OP(fmin):
    case  AARCH64_OP(fmax):
    case  AARCH64_OP(fcvtsd):     // Single to double
    case  AARCH64_OP(fcvtds):     // Double to single.
    case  AARCH64_OP(fcvt):     // Copy from int reg (no conversion)
    case  AARCH64_OP(fmov):
    case  AARCH64_OP(fcmp):
    case  AARCH64_OP(scvtf):
    case  AARCH64_OP(ucvtf):
    case  AARCH64_OP(fneg):
    case AARCH64_OP(vadd):
    case AARCH64_OP(vsub):
    case AARCH64_OP(vand):
    case AARCH64_OP(vorr):
    case AARCH64_OP(veor):
    case AARCH64_OP(vcmeq):
    case AARCH64_OP(vcmgt):
    case AARCH64_OP(vcmge):
    case AARCH64_OP(vcmhi):
    case AARCH64_OP(vcmhs):
    case AARCH64_OP(vfadd):
    case AARCH64_OP(vfsub):
    case AARCH64_OP(vfmul):
    case AARCH64_OP(vfdiv):
    case AARCH64_OP(fvarreg):
      return kAARCH64RegTypeFloat;

    case  AARCH64_OP(fcvtns):
    case  AARCH64_OP(fcvtnu):
    case  AARCH64_OP(fcvtzs):
    case  AARCH64_OP(fcvtzu):
      return kAARCH64RegTypeInt;

    default:
      if ((AARCH64Opcode)inst->opcode >= AARCH64_OP(fldr) &&
          (AARCH64Opcode)inst->opcode <= AARCH64_OP(fneg)) {
        return kAARCH64RegTypeFloat;
      }
      return kAARCH64RegTypeInt;
  }
}

bool AARCH64HasFloatResult(TargetInstruction* inst) {
  return RegisterTypeFromInstruction(inst) == kAARCH64RegTypeFloat;
}

static bool IsSavedReg(AARCH64Register* reg) {
  int num = reg->base.num;
  switch (reg->type) {
    case kAARCH64RegTypeInt:
      return num >= AARCH64_INT_SAVED_START &&
             num <= AARCH64_INT_SAVED_END;
    case kAARCH64RegTypeFloat:
      return num >= AARCH64_FP_SAVED_START &&
             num <= AARCH64_FP_SAVED_END;
  }
  return false;
}

static bool SWIsNoFallthroughTerminator(TargetInstruction* inst) {
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

static TargetInstruction** SWBranchTargetSlot(TargetInstruction* inst) {
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

static bool SWRecordReg(TargetRegister* tr, uint32_t* saved_mask,
                        uint32_t* temp_mask, uint32_t* arg_mask) {
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
    *saved_mask |= 1u << (n - AARCH64_INT_SAVED_START);
  } else if (n >= AARCH64_INT_TEMP_START && n <= AARCH64_INT_TEMP_END) {
    *temp_mask |= 1u << (n - AARCH64_INT_TEMP_START);
  } else if (n >= 0 && n <= 8) {
    *arg_mask |= 1u << n;
  } else if (n == AARCH64_IP2_REG) {
    *arg_mask |= 1u << 17;
  }
  return true;
}

static bool SWIsNonExecMeta(TargetInstruction* inst) {
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

static bool SWCheckBlockFrameFree(TargetBasicBlock* block, bool allow_save,
                                  bool allow_reload, uint32_t* saved_mask,
                                  uint32_t* temp_mask, uint32_t* arg_mask,
                                  int* exec_count) {
  int cnt = 0;
  uint32_t sm = 0;
  uint32_t tm = 0;
  uint32_t am = 0;
  if (block == NULL || block->code == NULL) {
    return false;
  }
  for (TargetInstruction* inst = block->code; inst != NULL;
       inst = TargetNext(inst)) {
    AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
    if (allow_save &&
        (op == AARCH64_OP(save) || op == AARCH64_OP(fp) ||
         op == AARCH64_OP(sp) || op == AARCH64_OP(lr))) {
      if (inst == block->end_code) {
        break;
      }
      continue;
    }
    if (op == AARCH64_OP(spill)) {
      if (inst->reg == NULL || inst->operand[1] == NULL ||
          !TargetIsConst(inst->operand[1]) ||
          GetRegisterSize(inst) == kSize128Bit) {
        return false;
      }
      AARCH64Register* sr = (AARCH64Register*)inst->reg;
      if (sr->type != kAARCH64RegTypeInt ||
          sr->base.num < AARCH64_INT_SAVED_START ||
          sr->base.num > AARCH64_INT_SAVED_END) {
        return false;
      }
      sm |= 1u << (sr->base.num - AARCH64_INT_SAVED_START);
      if (inst == block->end_code) {
        break;
      }
      continue;
    }
    if (op == AARCH64_OP(reload)) {
      if (!allow_reload || inst->reg == NULL || inst->operand[0] == NULL ||
          inst->operand[0]->opcode != (TargetOpcode)AARCH64_OP(spill) ||
          inst->operand[0]->operand[1] == NULL ||
          !TargetIsConst(inst->operand[0]->operand[1]) ||
          GetRegisterSize(inst->operand[0]) == kSize128Bit) {
        return false;
      }
      AARCH64Register* rr = (AARCH64Register*)inst->reg;
      if (rr->type != kAARCH64RegTypeInt || rr->base.num == AARCH64_IP1_REG ||
          rr->base.num == AARCH64_IP2_REG ||
          !SWRecordReg(inst->reg, &sm, &tm, &am)) {
        return false;
      }
      cnt++;
      if (inst == block->end_code) {
        break;
      }
      continue;
    }
    if (!SWIsNonExecMeta(inst)) {
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
          return false;
        default:
          break;
      }
      cnt++;
      if (!SWRecordReg(inst->reg, &sm, &tm, &am)) {
        return false;
      }
      if (inst->dest != NULL && !SWRecordReg(inst->dest->reg, &sm, &tm, &am)) {
        return false;
      }
      for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
        TargetInstruction* o = inst->operand[k];
        if (o != NULL) {
          TargetRegister* oreg =
              o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL);
          if (!SWRecordReg(oreg, &sm, &tm, &am)) {
            return false;
          }
        }
      }
    }
    if (inst == block->end_code) {
      break;
    }
  }
  *saved_mask = sm;
  *temp_mask = tm;
  *arg_mask = am;
  *exec_count = cnt;
  return true;
}

static int SWPopCount(uint32_t x) {
  int c = 0;
  while (x != 0) {
    c += (int)(x & 1u);
    x >>= 1;
  }
  return c;
}

static TargetInstruction* SWEnsureBlockLabel(TargetGenerator* gen,
                                             TargetBasicBlock* block) {
  for (TargetInstruction* p = block->code; p != NULL; p = TargetNext(p)) {
    if (p->opcode == (TargetOpcode)AARCH64_OP(label)) {
      return p;
    }
    if (!SWIsNonExecMeta(p) || p == block->end_code) {
      break;
    }
  }
  TargetInstruction* lbl = TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
  lbl->block = block;
  TargetEmitBefore(gen, lbl, block->code);
  block->code = lbl;
  return lbl;
}

static TargetInstruction* SWAppendBlockInst(TargetGenerator* gen,
                                            TargetBasicBlock* block,
                                            TargetInstruction* inst,
                                            TargetInstruction* cursor) {
  inst->block = block;
  ListInsertAfter(&gen->code, &inst->header, &cursor->header);
  if (block->code == NULL) {
    block->code = inst;
  }
  block->end_code = inst;
  return inst;
}

static TargetInstruction* SWNewRegProxy(TargetGenerator* gen,
                                        AARCH64Register* reg, int size) {
  TargetInstruction* p = TargetNewInstruction(TARGET_OP(ivarreg));
  p->reg = &reg->base;
  p->block = gen->entry_block;
  p->flags = TARGET_INST_PROCESSED | ((size & 3) << 16);
  TargetTrackOrphanInstruction(gen, p);
  return p;
}

static void ShrinkWrapFunction(AARCH64RegisterAllocator* allocator) {
  AARCH64Generator* g = allocator->g;
  TargetGenerator* gen = &g->base;
  if (!OptLevel2() || gen->varargs || g->moves_sp ||
      g->saved_regs.length != 0 || g->struct_return_reg >= 0 ||
      gen->exception_edges.length != 0 || gen->entry_block == NULL ||
      gen->entry_block->in_edges.length != 0) {
    return;
  }
  bool has_frame = gen->stack_frame_size > 0 || gen->num_calls > 0 ||
                   g->not_leaf ||
                   BitSetCount(&allocator->used_int_regs) > 0 ||
                   BitSetCount(&allocator->used_float_regs) > 0 ||
                   allocator->max_spilled_region_size > 0;
  if (!has_frame) {
    return;
  }

  // Locate the unique `save` in entry_block and unique `restore`+`ret`
  // epilogue block `E` (any additional `restore` blocks must be tail calls
  // that stay in the slow region).
  TargetInstruction* save_inst = NULL;
  TargetBasicBlock* epilogue = NULL;
  int save_count = 0;
  int ret_epilogue_count = 0;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    bool blk_has_restore = false;
    bool blk_has_ret = false;
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->opcode == (TargetOpcode)AARCH64_OP(save)) {
        save_count++;
        if (blk == gen->entry_block) {
          save_inst = p;
        }
      } else if (p->opcode == (TargetOpcode)AARCH64_OP(restore)) {
        blk_has_restore = true;
      } else if (p->opcode == (TargetOpcode)AARCH64_OP(ret)) {
        blk_has_ret = true;
      }
      if (p == blk->end_code) {
        break;
      }
    }
    if (blk_has_restore && blk_has_ret) {
      ret_epilogue_count++;
      epilogue = blk;
    } else if (blk_has_ret && !blk_has_restore) {
      return;
    }
  }
  if (save_count != 1 || save_inst == NULL || ret_epilogue_count != 1 ||
      epilogue == NULL || epilogue == gen->entry_block) {
    return;
  }

  // Verify epilogue ends with restore + ret; if epilogue also has frame-free
  // instructions before `restore` (e.g. `pc += 3` in void functions), split
  // `epilogue` right before `restore` into a fallthrough block + pure epilogue.
  TargetInstruction* restore_inst = NULL;
  bool seen_restore = false;
  bool seen_ret = false;
  bool has_pre_restore_exec = false;
  for (TargetInstruction* p = epilogue->code; p != NULL; p = TargetNext(p)) {
    AARCH64Opcode op = (AARCH64Opcode)p->opcode;
    if (op == AARCH64_OP(restore) && !seen_restore && !seen_ret) {
      seen_restore = true;
      restore_inst = p;
    } else if (op == AARCH64_OP(ret) && seen_restore && !seen_ret) {
      seen_ret = true;
    } else if (!SWIsNonExecMeta(p)) {
      if (seen_restore) {
        return;
      }
      has_pre_restore_exec = true;
    }
    if (p == epilogue->end_code) {
      break;
    }
  }
  if (!seen_restore || !seen_ret || restore_inst == NULL) {
    return;
  }
  if (has_pre_restore_exec && restore_inst != epilogue->code) {
    TargetBasicBlock* pre_blk = epilogue;
    TargetInstruction* pre_end = TargetPrev(restore_inst);
    if (pre_end == NULL || pre_end->block != pre_blk) {
      return;
    }
    TargetBasicBlock* pure_epilogue =
        NewTargetBasicBlock(gen->basic_blocks.length);
    VectorAppend(&gen->basic_blocks, pure_epilogue);
    pure_epilogue->code = restore_inst;
    pure_epilogue->end_code = pre_blk->end_code;
    pre_blk->end_code = pre_end;
    for (TargetInstruction* p = pure_epilogue->code; p != NULL;
         p = TargetNext(p)) {
      p->block = pure_epilogue;
      if (p == pure_epilogue->end_code) {
        break;
      }
    }
    for (size_t o = 0; o < pre_blk->out_edges.length; o++) {
      size_t succ_id = (size_t)pre_blk->out_edges.value.w[o];
      VectorAppend(&pure_epilogue->out_edges, (void*)(uintptr_t)succ_id);
      if (succ_id < gen->basic_blocks.length) {
        TargetBasicBlock* succ = gen->basic_blocks.value.p[succ_id];
        if (succ != NULL) {
          for (size_t e = 0; e < succ->in_edges.length; e++) {
            if ((size_t)succ->in_edges.value.w[e] == pre_blk->block_id) {
              succ->in_edges.value.w[e] = (intptr_t)pure_epilogue->block_id;
            }
          }
        }
      }
    }
    pre_blk->out_edges.length = 0;
    VectorAppend(&pre_blk->out_edges,
                 (void*)(uintptr_t)pure_epilogue->block_id);
    VectorAppend(&pure_epilogue->in_edges,
                 (void*)(uintptr_t)pre_blk->block_id);
    BitSetCopy(&pure_epilogue->dominators, &pre_blk->dominators);
    BitSetInsert(&pure_epilogue->dominators, pure_epilogue->block_id);
    pure_epilogue->idom = pre_blk;
    epilogue = pure_epilogue;
  }

  // Phase 1: 100% read-only region analysis.  Nothing in `gen` is mutated
  // until every eligibility, fallthrough, spill-sinking, exit-count, and
  // register-budget check has succeeded.
  size_t orig_blocks = gen->basic_blocks.length;
  size_t cap = orig_blocks + 32;
  bool* eligible_pure = calloc(cap, sizeof(bool));
  bool* eligible_clone = calloc(cap, sizeof(bool));
  bool* can_reach_ret = calloc(cap, sizeof(bool));
  bool* sim_pure_fast = calloc(cap, sizeof(bool));
  bool* sim_clone = calloc(cap, sizeof(bool));
  bool* in_fast = calloc(cap, sizeof(bool));
  uint32_t* blk_saved = calloc(cap, sizeof(uint32_t));
  uint32_t* blk_temp = calloc(cap, sizeof(uint32_t));
  uint32_t* blk_arg = calloc(cap, sizeof(uint32_t));
  int* blk_exec = calloc(cap, sizeof(int));
  TargetBasicBlock** fall_succ = calloc(cap, sizeof(TargetBasicBlock*));
  TargetBasicBlock** clone_of = calloc(cap, sizeof(TargetBasicBlock*));

  uint32_t incoming_arg_mask = 0;
  for (int k = 0; k < AARCH64_NUM_INT_ARGS; k++) {
    if (g->int_argument_registers[k] != NULL) {
      incoming_arg_mask |= 1u << k;
    }
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk == epilogue || blk == gen->exit_block ||
        blk->code == NULL) {
      continue;
    }
    uint32_t sm = 0, tm = 0, am = 0;
    int ec = 0;
    eligible_pure[b] = SWCheckBlockFrameFree(blk, blk == gen->entry_block,
                                             /*allow_reload=*/false, &sm, &tm,
                                             &am, &ec);
    eligible_clone[b] =
        eligible_pure[b] ||
        (blk != gen->entry_block &&
         SWCheckBlockFrameFree(blk, /*allow_save=*/false,
                               /*allow_reload=*/true, &sm, &tm, &am, &ec));
    if (eligible_clone[b]) {
      blk_saved[b] = sm;
      blk_temp[b] = tm;
      blk_arg[b] = am;
      blk_exec[b] = ec;
    }
    if (!SWIsNoFallthroughTerminator(blk->end_code)) {
      for (TargetInstruction* nxt = TargetNext(blk->end_code); nxt != NULL;
           nxt = TargetNext(nxt)) {
        if (nxt->block != NULL && nxt->block != blk) {
          fall_succ[b] = nxt->block;
          break;
        }
      }
    }
  }

  // Detect incoming-arg `mov r_k -> varreg_c` copies in `entry_block` that can
  // coalesce `c` onto `x_k` in `F`.
  int arg_coalesce_for_saved[10];
  for (int i = 0; i < 10; i++) {
    arg_coalesce_for_saved[i] = -1;
  }
  uint32_t written_before = 0;
  for (TargetInstruction* p = gen->entry_block->code; p != NULL;
       p = TargetNext(p)) {
    if (SWIsNonExecMeta(p)) {
      if (p == gen->entry_block->end_code) {
        break;
      }
      continue;
    }
    if ((p->opcode == (TargetOpcode)AARCH64_OP(mov) ||
         p->opcode == (TargetOpcode)AARCH64_OP(mv)) &&
        p->dest != NULL && p->dest->reg != NULL && p->operand[0] != NULL &&
        p->operand[1] == NULL &&
        p->operand[0]->opcode >= (TargetOpcode)AARCH64_OP(r0) &&
        p->operand[0]->opcode <= (TargetOpcode)AARCH64_OP(r7) &&
        p->operand[0]->users.length == 1) {
      int k = (int)p->operand[0]->opcode - (int)AARCH64_OP(r0);
      AARCH64Register* dr = (AARCH64Register*)p->dest->reg;
      if (dr->type == kAARCH64RegTypeInt &&
          dr->base.num >= AARCH64_INT_SAVED_START &&
          dr->base.num <= AARCH64_INT_SAVED_END &&
          (k > 0 || gen->is_void) && (written_before & (1u << k)) == 0) {
        int sbit = dr->base.num - AARCH64_INT_SAVED_START;
        if (arg_coalesce_for_saved[sbit] < 0) {
          arg_coalesce_for_saved[sbit] = k;
        }
      }
    }
    uint32_t d1 = 0, d2 = 0, am = 0;
    SWRecordReg(p->reg, &d1, &d2, &am);
    if (p->dest != NULL) {
      SWRecordReg(p->dest->reg, &d1, &d2, &am);
    }
    written_before |= am;
    if (p == gen->entry_block->end_code) {
      break;
    }
  }

  // Upper bound on caller-saved scratch registers available for `F`.
  int extra_scratch = 0;
  for (int k = 1; k <= 7; k++) {
    if ((incoming_arg_mask & (1u << k)) == 0) {
      extra_scratch++;
    }
  }
  if (gen->stack_frame_size < 0x100000) {
    extra_scratch++;
  }
  extra_scratch++;  // x17 when unused in F (checked via total_arg below).
  int coalesced_args = 0;
  for (int i = 0; i < 10; i++) {
    if (arg_coalesce_for_saved[i] >= 0) {
      coalesced_args++;
    }
  }
  int max_scratch_pool = 7 + extra_scratch + coalesced_args;

  // Only grow `F` into blocks that have a frame-free path to `epilogue`.
  bool reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      if (!eligible_clone[b] || can_reach_ret[b]) {
        continue;
      }
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == epilogue->block_id ||
            (succ_id < orig_blocks && can_reach_ret[succ_id])) {
          can_reach_ret[b] = true;
          reach_prog = true;
          break;
        }
      }
    }
  }

  bool has_fast_return = false;
  size_t num_slow_targets = 0;
  int num_sunk_slots = 0;
  int num_mapped = 0;
  int rcode = 0;
  size_t entry_id = gen->entry_block->block_id;
  if (!eligible_pure[entry_id] || !can_reach_ret[entry_id]) {
    rcode = 1;
    goto cleanup;
  }
  uint32_t total_saved = blk_saved[entry_id];
  uint32_t total_temp = blk_temp[entry_id];
  uint32_t total_arg = blk_arg[entry_id];
  if (SWPopCount(total_saved) > max_scratch_pool - SWPopCount(total_temp)) {
    goto cleanup;
  }
  sim_pure_fast[entry_id] = true;

  int cloned_insts = 0;
  int num_clones = 0;
  bool progress = true;
  while (progress) {
    progress = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      if (blk == NULL || sim_pure_fast[b] || sim_clone[b] ||
          !eligible_pure[b] || !can_reach_ret[b] ||
          blk->in_edges.length == 0) {
        continue;
      }
      bool all_pure_fast = true;
      for (size_t e = 0; e < blk->in_edges.length; e++) {
        size_t pred_id = (size_t)blk->in_edges.value.w[e];
        if (pred_id >= orig_blocks || !sim_pure_fast[pred_id]) {
          all_pure_fast = false;
          break;
        }
      }
      if (all_pure_fast) {
        if (b != entry_id) {
          TargetInstruction* prev = TargetPrev(blk->code);
          if (prev != NULL && prev->block != NULL &&
              (prev->block->block_id >= orig_blocks ||
               !sim_pure_fast[prev->block->block_id]) &&
              !SWIsNoFallthroughTerminator(prev)) {
            continue;
          }
        }
        uint32_t ns = total_saved | blk_saved[b];
        uint32_t nt = total_temp | blk_temp[b];
        if (SWPopCount(ns) <= max_scratch_pool - SWPopCount(nt)) {
          sim_pure_fast[b] = true;
          total_saved = ns;
          total_temp = nt;
          total_arg |= blk_arg[b];
          progress = true;
        }
      }
    }
    if (progress) {
      continue;
    }
    // The cheapest block first, so that the budget completes as many paths
    // to the epilogue as it can, and blocks inside loops last: cloning one
    // only peels the loop's first test.
    for (int pass = 0; pass < 2 && !progress; pass++) {
      size_t best = orig_blocks;
      for (size_t b = 0; b < orig_blocks && num_clones < 24; b++) {
        TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
        if (blk == NULL || sim_pure_fast[b] || sim_clone[b] ||
            (pass == 0 && blk->loop_nesting > 0) || !eligible_clone[b] ||
            !can_reach_ret[b] || blk_exec[b] > 22 ||
            cloned_insts + blk_exec[b] > 40 || blk->in_edges.length == 0) {
          continue;
        }
        bool has_fast_pred = false;
        bool has_slow_pred = false;
        bool forward_only = true;
        for (size_t e = 0; e < blk->in_edges.length; e++) {
          size_t pred_id = (size_t)blk->in_edges.value.w[e];
          if (pred_id < orig_blocks &&
              (sim_pure_fast[pred_id] || sim_clone[pred_id])) {
            has_fast_pred = true;
            if (pred_id >= b) {
              forward_only = false;
              break;
            }
            if (sim_clone[pred_id] || !eligible_pure[b]) {
              has_slow_pred = true;
            }
          } else {
            has_slow_pred = true;
          }
        }
        if (!has_fast_pred || !has_slow_pred || !forward_only) {
          continue;
        }
        uint32_t ns = total_saved | blk_saved[b];
        uint32_t nt = total_temp | blk_temp[b];
        if (SWPopCount(ns) > max_scratch_pool - SWPopCount(nt)) {
          continue;
        }
        if (best == orig_blocks || blk_exec[b] < blk_exec[best]) {
          best = b;
        }
      }
      if (best < orig_blocks) {
        sim_clone[best] = true;
        total_saved |= blk_saved[best];
        total_temp |= blk_temp[best];
        total_arg |= blk_arg[best];
        cloned_insts += blk_exec[best];
        num_clones++;
        progress = true;
      }
    }
  }

  // Prune any block in `sim_pure_fast` / `sim_clone` that cannot reach
  // `epilogue` solely through `sim_pure_fast` / `sim_clone` blocks.
  bool* reaches_fast_ret = calloc(orig_blocks, sizeof(bool));
  reach_prog = true;
  while (reach_prog) {
    reach_prog = false;
    for (size_t b = 0; b < orig_blocks; b++) {
      if ((!sim_pure_fast[b] && !sim_clone[b]) || reaches_fast_ret[b]) {
        continue;
      }
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      for (size_t o = 0; o < blk->out_edges.length; o++) {
        size_t succ_id = (size_t)blk->out_edges.value.w[o];
        if (succ_id == epilogue->block_id ||
            (succ_id < orig_blocks && reaches_fast_ret[succ_id])) {
          reaches_fast_ret[b] = true;
          reach_prog = true;
          break;
        }
      }
    }
  }
  total_saved = 0;
  total_temp = 0;
  total_arg = 0;
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!reaches_fast_ret[b]) {
      sim_pure_fast[b] = false;
      sim_clone[b] = false;
    } else {
      total_saved |= blk_saved[b];
      total_temp |= blk_temp[b];
      total_arg |= blk_arg[b];
    }
  }
  free(reaches_fast_ret);
  if (!sim_pure_fast[entry_id]) {
    goto cleanup;
  }

  // Validate fast returns, slow boundary exits, fallthroughs, and temp regs.
  TargetBasicBlock* slow_targets[16];
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b] && !sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (size_t o = 0; o < blk->out_edges.length; o++) {
      size_t succ_id = (size_t)blk->out_edges.value.w[o];
      TargetBasicBlock* succ = gen->basic_blocks.value.p[succ_id];
      if (succ == NULL || succ == gen->exit_block) {
        continue;
      }
      if (succ == epilogue) {
        has_fast_return = true;
      } else if (succ_id >= orig_blocks ||
                 (!sim_pure_fast[succ_id] && !sim_clone[succ_id])) {
        bool seen = false;
        for (size_t s = 0; s < num_slow_targets; s++) {
          if (slow_targets[s] == succ) {
            seen = true;
            break;
          }
        }
        if (!seen) {
          if (num_slow_targets >= 16) {
            goto cleanup;
          }
          slow_targets[num_slow_targets++] = succ;
        }
      }
    }
  }
  if (!has_fast_return || num_slow_targets == 0) {
    goto cleanup;
  }

  // Reject when `entry_block` immediately exits to `Slow` unless a 64-bit
  // pointer is NULL (e.g. `ChargeCycles`: `if (bbc != NULL) { ... calls ... }
  // UpdatePace(...)`): the NULL path is cold, so the 100%-taken non-NULL path
  // would pay extra register-transfer instructions on every call.
  if (gen->entry_block->out_edges.length == 2) {
    size_t o0 = (size_t)gen->entry_block->out_edges.value.w[0];
    size_t o1 = (size_t)gen->entry_block->out_edges.value.w[1];
    bool f0 = o0 < orig_blocks && (sim_pure_fast[o0] || sim_clone[o0]);
    bool f1 = o1 < orig_blocks && (sim_pure_fast[o1] || sim_clone[o1]);
    if (f0 != f1) {
      TargetInstruction* last_cmp = NULL;
      for (TargetInstruction* p = gen->entry_block->code; p != NULL;
           p = TargetNext(p)) {
        if (p->opcode == (TargetOpcode)AARCH64_OP(cmp)) {
          last_cmp = p;
        }
        if (p == gen->entry_block->end_code) {
          break;
        }
      }
      TargetInstruction* end = gen->entry_block->end_code;
      bool ptr_zero_cmp =
          (end != NULL &&
           (end->opcode == (TargetOpcode)AARCH64_OP(cbz) ||
            end->opcode == (TargetOpcode)AARCH64_OP(cbnz)) &&
           GetRegisterSize(end) == kSize64Bit) ||
          (last_cmp != NULL && GetRegisterSize(last_cmp) == kSize64Bit &&
           last_cmp->operand[1] != NULL &&
           (last_cmp->operand[1]->opcode == (TargetOpcode)AARCH64_OP(zr) ||
            (TargetIsConst(last_cmp->operand[1]) &&
             TargetIntValue(last_cmp->operand[1]) == 0)));
      if (ptr_zero_cmp) {
        goto cleanup;
      }
    }
  }

  // Skip shrink-wrapping when the fast region only contains defensive null /
  // zero sanity guards (`ptr == NULL`, `n <= 0`, `!running`): those guards are
  // virtually never taken at runtime, so copying registers into and out of
  // scratch registers on the 100%-taken body would cost more than it saves.
  bool has_nontrivial_fast_work = false;
  for (size_t b = 0; b < orig_blocks && !has_nontrivial_fast_work; b++) {
    if (!sim_pure_fast[b] && !sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      AARCH64Opcode op = (AARCH64Opcode)p->opcode;
      if (AARCH64IsStore(p) || op == AARCH64_OP(tst) ||
          op == AARCH64_OP(and) || op == AARCH64_OP(ands) ||
          op == AARCH64_OP(sub) || op == AARCH64_OP(subs)) {
        has_nontrivial_fast_work = true;
        break;
      }
      if (op == AARCH64_OP(cmp) && p->operand[1] != NULL) {
        if (TargetIsConst(p->operand[1])) {
          if (TargetIntValue(p->operand[1]) != 0) {
            has_nontrivial_fast_work = true;
            break;
          }
        } else if (p->operand[1]->opcode != (TargetOpcode)AARCH64_OP(zr)) {
          has_nontrivial_fast_work = true;
          break;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }
  if (!has_nontrivial_fast_work) {
    goto cleanup;
  }

  for (size_t i = 0; i < num_slow_targets / 2; i++) {
    TargetBasicBlock* tmp = slow_targets[i];
    slow_targets[i] = slow_targets[num_slow_targets - 1 - i];
    slow_targets[num_slow_targets - 1 - i] = tmp;
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b] || b == entry_id) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* prev = TargetPrev(blk->code);
    if (prev != NULL && prev->block != NULL &&
        (prev->block->block_id >= orig_blocks ||
         !sim_pure_fast[prev->block->block_id]) &&
        !SWIsNoFallthroughTerminator(prev)) {
      goto cleanup;
    }
  }

  // Validate every `spill` and `reload` in `F`:
  // - At most 16 sunk spill slots, at most one distinct slot per saved reg `c`.
  // - Along every path in `F` from `sp` to any slow target that can reach a
  //   reload of `sp`'s slot (and to any `sim_clone` reload of `sp`'s slot),
  //   register `c` is never overwritten after `sp`.
  int64_t sunk_slot_off[16];
  int sunk_slot_reg[16];
  TargetInstruction* sunk_slot_proto[16];
  int saved_reg_slot_idx[10];
  for (int i = 0; i < 10; i++) {
    saved_reg_slot_idx[i] = -1;
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b] && !sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->opcode == (TargetOpcode)AARCH64_OP(spill)) {
        int c = p->reg->num;
        int sbit = c - AARCH64_INT_SAVED_START;
        int64_t soff = TargetIntValue(p->operand[1]);
        int idx = -1;
        for (int s = 0; s < num_sunk_slots; s++) {
          if (sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx >= 0) {
          goto cleanup;
        } else {
          if (num_sunk_slots >= 16 || saved_reg_slot_idx[sbit] >= 0) {
            goto cleanup;
          }
          idx = num_sunk_slots++;
          sunk_slot_off[idx] = soff;
          sunk_slot_reg[idx] = c;
          sunk_slot_proto[idx] = p;
          saved_reg_slot_idx[sbit] = idx;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  for (int idx = 0; idx < num_sunk_slots; idx++) {
    int64_t soff = sunk_slot_off[idx];
    int c = sunk_slot_reg[idx];
    TargetInstruction* proto = sunk_slot_proto[idx];
    // Every block containing a reload of `soff` must be dominated by `proto`'s
    // block, and along every block in `F` reachable from `proto` that can reach
    // such a reload, register `c` must not be clobbered after `proto`.
    bool clobbered = false;
    bool* reaches_reload = calloc(orig_blocks, sizeof(bool));
    for (size_t b = 0; b < orig_blocks && !clobbered; b++) {
      TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
      if (blk == NULL || blk->code == NULL) {
        continue;
      }
      for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
        if (p->opcode == (TargetOpcode)AARCH64_OP(reload) &&
            p->operand[0] != NULL && p->operand[0]->operand[1] != NULL &&
            TargetIsConst(p->operand[0]->operand[1]) &&
            TargetIntValue(p->operand[0]->operand[1]) == soff) {
          if (!BitSetContains(&blk->dominators, proto->block->block_id)) {
            clobbered = true;
            break;
          }
          reaches_reload[b] = true;
          break;
        }
        if (p == blk->end_code) {
          break;
        }
      }
    }
    if (!clobbered) {
      bool rprog = true;
      while (rprog) {
        rprog = false;
        for (size_t b = 0; b < orig_blocks; b++) {
          if (reaches_reload[b] || b == proto->block->block_id) {
            continue;
          }
          TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
          if (blk == NULL) {
            continue;
          }
          for (size_t o = 0; o < blk->out_edges.length; o++) {
            size_t sid = (size_t)blk->out_edges.value.w[o];
            if (sid < orig_blocks && reaches_reload[sid]) {
              reaches_reload[b] = true;
              rprog = true;
              break;
            }
          }
        }
      }
      BitSet after_proto;
      BitSetInit(&after_proto);
      TargetBasicBlockReachableAfter(gen, proto->block, &after_proto);
      for (size_t b = 0; b < orig_blocks && !clobbered; b++) {
        if (!sim_pure_fast[b] && !sim_clone[b]) {
          continue;
        }
        bool is_proto_blk = (b == proto->block->block_id);
        if (!is_proto_blk &&
            (!BitSetContains(&after_proto, b) || !reaches_reload[b])) {
          continue;
        }
        TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
        bool past_proto = !is_proto_blk;
        for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
          if (p == proto) {
            past_proto = true;
          } else if (past_proto &&
                     p->opcode != (TargetOpcode)AARCH64_OP(spill) &&
                     !SWIsNonExecMeta(p)) {
            TargetRegister* wreg =
                p->dest != NULL ? p->dest->reg : p->reg;
            if (wreg == &allocator->int_regs[c].base) {
              if (!AARCH64IsVarRegister(proto->operand[0]) ||
                  p->dest != proto->operand[0]) {
                clobbered = true;
                break;
              }
            }
          }
          if (p == blk->end_code) {
            break;
          }
        }
      }
      BitSetDestruct(&after_proto);
    }
    free(reaches_reload);
    if (clobbered) {
      goto cleanup;
    }
  }

  // Verify every `reload` in `sim_clone` blocks maps to a validated sunk slot
  // whose `sp` dominates that block.
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p->opcode == (TargetOpcode)AARCH64_OP(reload)) {
        int64_t soff = TargetIntValue(p->operand[0]->operand[1]);
        int idx = -1;
        for (int s = 0; s < num_sunk_slots; s++) {
          if (sunk_slot_off[s] == soff) {
            idx = s;
            break;
          }
        }
        if (idx < 0 ||
            !BitSetContains(&blk->dominators,
                            sunk_slot_proto[idx]->block->block_id)) {
          goto cleanup;
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  // Assign each callee-saved int register used in `F` to a caller-saved
  // register: coalesce incoming-argument copies first (`x0..x7`), then free
  // `x15..x9`, then unused `x7..x1`, `x8`, `x17`.
  int saved_list[10];
  int temp_list[10];
  uint32_t taken_caller = 0;
  for (int t = AARCH64_INT_TEMP_START; t <= AARCH64_INT_TEMP_END; t++) {
    if ((total_temp & (1u << (t - AARCH64_INT_TEMP_START))) != 0) {
      taken_caller |= 1u << t;
    }
  }
  for (int bit = 0; bit <= AARCH64_INT_SAVED_END - AARCH64_INT_SAVED_START;
       bit++) {
    if ((total_saved & (1u << bit)) == 0) {
      continue;
    }
    int k = arg_coalesce_for_saved[bit];
    if (k >= 0 && (taken_caller & (1u << k)) == 0) {
      // Check that no instruction in `F` other than the single incoming-arg
      // move in `entry_block` uses `x_k`.
      bool k_used_elsewhere = false;
      for (size_t b = 0; b < orig_blocks && !k_used_elsewhere; b++) {
        if (!sim_pure_fast[b] && !sim_clone[b]) {
          continue;
        }
        if (b != entry_id && (blk_arg[b] & (1u << k)) != 0) {
          k_used_elsewhere = true;
          break;
        }
      }
      if (!k_used_elsewhere) {
        int count_k_in_entry = 0;
        for (TargetInstruction* p = gen->entry_block->code; p != NULL;
             p = TargetNext(p)) {
          if (SWIsNonExecMeta(p) ||
              p->opcode == (TargetOpcode)AARCH64_OP(spill)) {
            if (p == gen->entry_block->end_code) {
              break;
            }
            continue;
          }
          uint32_t d1 = 0, d2 = 0, am = 0;
          SWRecordReg(p->reg, &d1, &d2, &am);
          if (p->dest != NULL) {
            SWRecordReg(p->dest->reg, &d1, &d2, &am);
          }
          for (int opi = 0; opi < TARGET_MAX_OPERANDS; opi++) {
            TargetInstruction* o = p->operand[opi];
            if (o != NULL) {
              SWRecordReg(
                  o->reg != NULL ? o->reg
                                 : (o->dest != NULL ? o->dest->reg : NULL),
                  &d1, &d2, &am);
            }
          }
          if ((am & (1u << k)) != 0) {
            count_k_in_entry++;
          }
          if (p == gen->entry_block->end_code) {
            break;
          }
        }
        if (count_k_in_entry == 1) {
          saved_list[num_mapped] = AARCH64_INT_SAVED_START + bit;
          temp_list[num_mapped] = k;
          taken_caller |= 1u << k;
          num_mapped++;
        }
      }
    }
  }

  int candidate_pool[16];
  int pool_len = 0;
  for (int t = AARCH64_INT_TEMP_END; t >= AARCH64_INT_TEMP_START; t--) {
    if ((taken_caller & (1u << t)) == 0) {
      candidate_pool[pool_len++] = t;
    }
  }
  for (int k = 7; k >= 1; k--) {
    if ((incoming_arg_mask & (1u << k)) == 0 &&
        (total_arg & (1u << k)) == 0 && (taken_caller & (1u << k)) == 0) {
      candidate_pool[pool_len++] = k;
    }
  }
  if (gen->stack_frame_size < 0x100000 && (total_arg & (1u << 8)) == 0 &&
      (taken_caller & (1u << 8)) == 0) {
    candidate_pool[pool_len++] = 8;
  }
  if ((total_arg & (1u << 17)) == 0 && (taken_caller & (1u << 17)) == 0) {
    candidate_pool[pool_len++] = 17;
  }

  int pool_pos = 0;
  for (int bit = 0; bit <= AARCH64_INT_SAVED_END - AARCH64_INT_SAVED_START;
       bit++) {
    if ((total_saved & (1u << bit)) == 0) {
      continue;
    }
    int c = AARCH64_INT_SAVED_START + bit;
    bool already = false;
    for (int m = 0; m < num_mapped; m++) {
      if (saved_list[m] == c) {
        already = true;
        break;
      }
    }
    if (already) {
      continue;
    }
    if (pool_pos >= pool_len) {
      goto cleanup;
    }
    saved_list[num_mapped] = c;
    temp_list[num_mapped] = candidate_pool[pool_pos++];
    num_mapped++;
  }

  // Phase 2: All checks passed.  Materialize clones and apply shrink-wrap.
  TargetInstruction* al_cond = NULL;
  for (TargetInstruction* p = TargetFirstInstruction(gen); p != NULL;
       p = TargetNext(p)) {
    if (p->opcode == (TargetOpcode)AARCH64_OP(al)) {
      al_cond = p;
      break;
    }
  }
  if (al_cond == NULL) {
    al_cond = TargetNewInstruction((TargetOpcode)AARCH64_OP(al));
    al_cond->block = gen->entry_block;
    al_cond->flags = TARGET_INST_PROCESSED;
    TargetEmitBefore(gen, al_cond, gen->entry_block->code);
    gen->entry_block->code = al_cond;
  }

  for (size_t b = 0; b < orig_blocks; b++) {
    if (sim_pure_fast[b]) {
      in_fast[b] = true;
    }
  }

  // Create clone blocks for every `sim_clone[b]` first so inter-clone branches
  // and fallthroughs can resolve to each other.
  TargetInstruction* orig_clone_lbl[32] = {0};
  TargetInstruction* new_clone_lbl[32] = {0};
  size_t clone_src_ids[32] = {0};
  int clone_idx = 0;
  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_clone[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* orig_lbl = SWEnsureBlockLabel(gen, blk);
    TargetBasicBlock* clone = NewTargetBasicBlock(gen->basic_blocks.length);
    size_t cid = clone->block_id;
    VectorAppend(&gen->basic_blocks, clone);
    TargetInstruction* clone_lbl =
        TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
    clone_lbl->block = clone;
    clone_lbl->flags = TARGET_INST_PROCESSED;
    TargetEmit(gen, clone_lbl);
    clone->code = clone_lbl;
    clone->end_code = clone_lbl;
    TargetInstruction* cur = clone_lbl;
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != orig_lbl && p->opcode != (TargetOpcode)AARCH64_OP(label) &&
          p->opcode != (TargetOpcode)AARCH64_OP(loc) &&
          p->opcode != (TargetOpcode)AARCH64_OP(spill) &&
          !p->observable_checkpoint) {
        TargetInstruction* cp;
        if (p->opcode == (TargetOpcode)AARCH64_OP(reload)) {
          int64_t soff = TargetIntValue(p->operand[0]->operand[1]);
          int src_c = -1;
          for (int s = 0; s < num_sunk_slots; s++) {
            if (sunk_slot_off[s] == soff) {
              src_c = sunk_slot_reg[s];
              break;
            }
          }
          assert(src_c >= 0);
          int rsz = GetRegisterSize(p) == kSize32Bit ? kSize32Bit : kSize64Bit;
          TargetInstruction* src_proxy =
              SWNewRegProxy(gen, &allocator->int_regs[src_c], rsz);
          cp = SetInstructionSize(
              TargetNewInstruction1((TargetOpcode)AARCH64_OP(mov), src_proxy),
              rsz);
          cp->reg = p->reg;
          cp->flags |= TARGET_INST_PROCESSED;
        } else {
          cp = TargetNewInstruction(p->opcode);
          cp->flags = p->flags;
          cp->reg = p->reg;
          cp->dest = p->dest;
          for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
            cp->operand[k] = p->operand[k];
          }
        }
        cur = SWAppendBlockInst(gen, clone, cp, cur);
      }
      if (p == blk->end_code) {
        break;
      }
    }
    clone_of[b] = clone;
    in_fast[cid] = true;
    fall_succ[cid] = fall_succ[b];
    orig_clone_lbl[clone_idx] = orig_lbl;
    new_clone_lbl[clone_idx] = clone_lbl;
    clone_src_ids[clone_idx] = b;
    clone_idx++;
  }

  // Redirect branches and fallthroughs among all fast blocks (both pure-fast
  // and cloned) from `orig_clone_lbl[i]` -> `new_clone_lbl[i]`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = SWBranchTargetSlot(p);
      if (slot != NULL && *slot != NULL) {
        for (int c = 0; c < clone_idx; c++) {
          if (*slot == orig_clone_lbl[c]) {
            *slot = new_clone_lbl[c];
            break;
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
    if (fall_succ[b] != NULL && fall_succ[b]->block_id < orig_blocks &&
        clone_of[fall_succ[b]->block_id] != NULL) {
      fall_succ[b] = clone_of[fall_succ[b]->block_id];
    }
  }

  // Materialize explicit labels and fallthrough branches on every fast block.
  TargetInstruction* epilogue_lbl = SWEnsureBlockLabel(gen, epilogue);
  TargetInstruction* slow_lbls[16];
  for (size_t s = 0; s < num_slow_targets; s++) {
    slow_lbls[s] = SWEnsureBlockLabel(gen, slow_targets[s]);
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b] || fall_succ[b] == NULL) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (!SWIsNoFallthroughTerminator(blk->end_code)) {
      TargetInstruction* target_lbl = SWEnsureBlockLabel(gen, fall_succ[b]);
      TargetInstruction* br =
          TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                                target_lbl);
      br->flags = TARGET_INST_PROCESSED;
      SWAppendBlockInst(gen, blk, br, blk->end_code);
    }
  }

  TargetInstruction* fast_p32[33] = {0};
  TargetInstruction* fast_p64[33] = {0};
  TargetInstruction* slow_p32[33] = {0};
  TargetInstruction* slow_p64[33] = {0};
  for (int i = 0; i < num_mapped; i++) {
    int c = saved_list[i];
    int t = temp_list[i];
    allocator->shrink_wrapped_regs |= UINT32_C(1) << c;
    fast_p32[c] = SWNewRegProxy(gen, &allocator->int_regs[t], kSize32Bit);
    fast_p64[c] = SWNewRegProxy(gen, &allocator->int_regs[t], kSize64Bit);
    slow_p32[c] = SWNewRegProxy(gen, &allocator->int_regs[c], kSize32Bit);
    slow_p64[c] = SWNewRegProxy(gen, &allocator->int_regs[c], kSize64Bit);
  }

  // First preserve `c` references in slow blocks whose operand was an
  // instruction defined in `F` whose `.reg` is about to change from `c` to `t`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b < cap && in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    if (blk == NULL || blk->code == NULL) {
      continue;
    }
    for (TargetInstruction* u = blk->code; u != NULL; u = TargetNext(u)) {
      if (u->dest != NULL && !AARCH64IsVarRegister(u->dest) &&
          u->dest->block != NULL && u->dest->block->block_id < cap &&
          in_fast[u->dest->block->block_id] && u->dest->reg != NULL) {
        AARCH64Register* dr = (AARCH64Register*)u->dest->reg;
        if (dr->type == kAARCH64RegTypeInt && dr->base.num < 33 &&
            slow_p64[dr->base.num] != NULL) {
          u->dest = GetRegisterSize(u->dest) == kSize32Bit
                        ? slow_p32[dr->base.num]
                        : slow_p64[dr->base.num];
        }
      }
      for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
        TargetInstruction* o = u->operand[k];
        if (o != NULL && !AARCH64IsVarRegister(o) &&
            o->opcode != (TargetOpcode)AARCH64_OP(spill) &&
            o->block != NULL && o->block->block_id < cap &&
            in_fast[o->block->block_id] && o->reg != NULL) {
          AARCH64Register* orr = (AARCH64Register*)o->reg;
          if (orr->type == kAARCH64RegTypeInt && orr->base.num < 33 &&
              slow_p64[orr->base.num] != NULL) {
            u->operand[k] = GetRegisterSize(o) == kSize32Bit
                                ? slow_p32[orr->base.num]
                                : slow_p64[orr->base.num];
          }
        }
      }
      if (u == blk->end_code) {
        break;
      }
    }
  }

  // Rewrite `c -> t` across all instructions in `F`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      if (p != save_inst && p->opcode != (TargetOpcode)AARCH64_OP(spill) &&
          !SWIsNonExecMeta(p)) {
        if (p->reg != NULL) {
          AARCH64Register* pr = (AARCH64Register*)p->reg;
          if (pr->type == kAARCH64RegTypeInt && pr->base.num < 33 &&
              fast_p64[pr->base.num] != NULL) {
            p->reg = fast_p64[pr->base.num]->reg;
          }
        }
        if (p->dest != NULL) {
          TargetRegister* dtr = p->dest->reg;
          if (dtr != NULL) {
            AARCH64Register* dr = (AARCH64Register*)dtr;
            if (dr->type == kAARCH64RegTypeInt && dr->base.num < 33 &&
                fast_p64[dr->base.num] != NULL) {
              p->dest = GetRegisterSize(p->dest) == kSize32Bit
                            ? fast_p32[dr->base.num]
                            : fast_p64[dr->base.num];
            }
          }
        }
        for (int k = 0; k < TARGET_MAX_OPERANDS; k++) {
          TargetInstruction* o = p->operand[k];
          if (o == NULL || o->opcode == (TargetOpcode)AARCH64_OP(spill)) {
            continue;
          }
          TargetRegister* otr =
              o->reg != NULL ? o->reg : (o->dest != NULL ? o->dest->reg : NULL);
          if (otr != NULL) {
            AARCH64Register* orr = (AARCH64Register*)otr;
            if (orr->type == kAARCH64RegTypeInt && orr->base.num < 33 &&
                fast_p64[orr->base.num] != NULL) {
              p->operand[k] = GetRegisterSize(o) == kSize32Bit
                                  ? fast_p32[orr->base.num]
                                  : fast_p64[orr->base.num];
            }
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  // Unlink `save_inst` from `entry_block` and unlink all `spill` instructions
  // from pure-fast blocks so they sink into `CommonSave`.
  if (gen->entry_block->code == save_inst) {
    gen->entry_block->code = TargetNext(save_inst);
  }
  if (gen->entry_block->end_code == save_inst) {
    gen->entry_block->end_code = TargetPrev(save_inst);
  }
  ListDeleteElement(&gen->code, &save_inst->header);

  for (size_t b = 0; b < orig_blocks; b++) {
    if (!sim_pure_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* p = blk->code;
    while (p != NULL) {
      bool is_end = (p == blk->end_code);
      TargetInstruction* nxt = TargetNext(p);
      if (p->opcode == (TargetOpcode)AARCH64_OP(spill)) {
        if (blk->code == p) {
          blk->code = nxt;
        }
        if (blk->end_code == p) {
          blk->end_code = TargetPrev(p);
        }
        ListDeleteElement(&gen->code, &p->header);
        TargetTrackOrphanInstruction(gen, p);
      }
      if (is_end) {
        break;
      }
      p = nxt;
    }
  }

  // Place all fast blocks sequentially right after `entry_block`, followed by
  // `E_fast: ret`, any multi-exit trampolines, and the single `CommonSave`
  // block right before `Slow` so `.eh_frame` stays contiguous and exact.
  TargetInstruction* cursor = gen->entry_block->end_code;
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b] || b == entry_id) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    TargetInstruction* p = blk->code;
    while (p != NULL) {
      bool is_end = (p == blk->end_code);
      TargetInstruction* nxt = TargetNext(p);
      if (p != cursor) {
        ListDeleteElement(&gen->code, &p->header);
        ListInsertAfter(&gen->code, &p->header, &cursor->header);
        cursor = p;
      }
      if (is_end) {
        break;
      }
      p = nxt;
    }
  }

  // Fast frameless return block `E_fast`.
  TargetBasicBlock* fast_ret_blk =
      NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, fast_ret_blk);
  TargetInstruction* fast_ret_lbl =
      TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
  fast_ret_lbl->flags = TARGET_INST_PROCESSED;
  cursor = SWAppendBlockInst(gen, fast_ret_blk, fast_ret_lbl, cursor);
  TargetInstruction* fast_ret_inst =
      TargetNewInstruction((TargetOpcode)AARCH64_OP(ret));
  fast_ret_inst->flags = TARGET_INST_PROCESSED;
  cursor = SWAppendBlockInst(gen, fast_ret_blk, fast_ret_inst, cursor);

  TargetInstruction* save_lbl =
      TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
  save_lbl->flags = TARGET_INST_PROCESSED;

  TargetInstruction* tramp_lbls[16] = {0};
  if (num_slow_targets == 1) {
    tramp_lbls[0] = save_lbl;
  } else {
    for (size_t s = 0; s < num_slow_targets; s++) {
      TargetBasicBlock* tblk = NewTargetBasicBlock(gen->basic_blocks.length);
      VectorAppend(&gen->basic_blocks, tblk);
      TargetInstruction* tlbl =
          TargetNewInstruction((TargetOpcode)AARCH64_OP(label));
      tlbl->flags = TARGET_INST_PROCESSED;
      cursor = SWAppendBlockInst(gen, tblk, tlbl, cursor);
      tramp_lbls[s] = tlbl;

      TargetInstruction* idx_const = SetInstructionSize(
          TargetGetIntConstant(gen, NULL, kTargetType32Bit, (int64_t)s),
          kSize32Bit);
      TargetInstruction* mov_idx = SetInstructionSize(
          TargetNewInstruction1((TargetOpcode)AARCH64_OP(movz), idx_const),
          kSize32Bit);
      mov_idx->reg = &allocator->int_regs[AARCH64_IP1_REG].base;
      mov_idx->flags |= TARGET_INST_PROCESSED;
      cursor = SWAppendBlockInst(gen, tblk, mov_idx, cursor);

      if (s + 1 < num_slow_targets) {
        TargetInstruction* jsave =
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                                  save_lbl);
        jsave->flags = TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, tblk, jsave, cursor);
      }
    }
  }

  // Redirect branches out of `F` to `fast_ret_lbl` and `tramp_lbls[s]`.
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    if (b >= cap || !in_fast[b]) {
      continue;
    }
    TargetBasicBlock* blk = gen->basic_blocks.value.p[b];
    for (TargetInstruction* p = blk->code; p != NULL; p = TargetNext(p)) {
      TargetInstruction** slot = SWBranchTargetSlot(p);
      if (slot != NULL && *slot != NULL) {
        if (*slot == epilogue_lbl) {
          *slot = fast_ret_lbl;
        } else {
          for (size_t s = 0; s < num_slow_targets; s++) {
            if (*slot == slow_lbls[s]) {
              *slot = tramp_lbls[s];
              break;
            }
          }
        }
      }
      if (p == blk->end_code) {
        break;
      }
    }
  }

  // Emit `CommonSave`: `save_lbl: save; mov c_i, t_i; dispatch -> slow_lbls`.
  TargetBasicBlock* save_blk = NewTargetBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, save_blk);
  cursor = SWAppendBlockInst(gen, save_blk, save_lbl, cursor);
  cursor = SWAppendBlockInst(gen, save_blk, save_inst, cursor);
  for (int i = 0; i < num_mapped; i++) {
    int c = saved_list[i];
    TargetInstruction* mv = SetInstructionSize(
        TargetNewInstruction1((TargetOpcode)AARCH64_OP(mov), fast_p64[c]),
        kSize64Bit);
    mv->reg = &allocator->int_regs[c].base;
    mv->dest = slow_p64[c];
    mv->flags |= TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, mv, cursor);
  }
  for (int idx = 0; idx < num_sunk_slots; idx++) {
    TargetInstruction* proto = sunk_slot_proto[idx];
    TargetInstruction* sp_store = TargetNewInstruction2(
        (TargetOpcode)AARCH64_OP(spill), proto->operand[0], proto->operand[1]);
    sp_store->reg = &allocator->int_regs[sunk_slot_reg[idx]].base;
    sp_store->flags = proto->flags | TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, sp_store, cursor);
  }
  if (num_slow_targets == 1) {
    TargetInstruction* jslow =
        TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                              slow_lbls[0]);
    jslow->flags = TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, jslow, cursor);
  } else {
    TargetInstruction* ip1_proxy =
        SWNewRegProxy(gen, &allocator->int_regs[AARCH64_IP1_REG], kSize32Bit);
    TargetInstruction* eq_cond =
        TargetNewInstruction((TargetOpcode)AARCH64_OP(eq));
    eq_cond->block = save_blk;
    eq_cond->flags = TARGET_INST_PROCESSED;
    TargetTrackOrphanInstruction(gen, eq_cond);
    for (size_t s = 0; s + 1 < num_slow_targets; s++) {
      if (s == 0) {
        TargetInstruction* cbz_s0 = SetInstructionSize(
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(cbz), ip1_proxy,
                                  slow_lbls[0]),
            kSize32Bit);
        cbz_s0->flags |= TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, save_blk, cbz_s0, cursor);
      } else {
        TargetInstruction* c_s = SetInstructionSize(
            TargetGetIntConstant(gen, NULL, kTargetType32Bit, (int64_t)s),
            kSize32Bit);
        TargetInstruction* cmp_s = SetInstructionSize(
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(cmp), ip1_proxy,
                                  c_s),
            kSize32Bit);
        cmp_s->flags |= TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, save_blk, cmp_s, cursor);
        TargetInstruction* beq_s =
            TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), eq_cond,
                                  slow_lbls[s]);
        beq_s->flags = TARGET_INST_PROCESSED;
        cursor = SWAppendBlockInst(gen, save_blk, beq_s, cursor);
      }
    }
    TargetInstruction* jlast =
        TargetNewInstruction2((TargetOpcode)AARCH64_OP(b), al_cond,
                              slow_lbls[num_slow_targets - 1]);
    jlast->flags = TARGET_INST_PROCESSED;
    cursor = SWAppendBlockInst(gen, save_blk, jlast, cursor);
  }

cleanup:
  (void)rcode;
  free(eligible_pure);
  free(eligible_clone);
  free(can_reach_ret);
  free(sim_pure_fast);
  free(sim_clone);
  free(in_fast);
  free(blk_saved);
  free(blk_temp);
  free(blk_arg);
  free(blk_exec);
  free(fall_succ);
  free(clone_of);
}

// Physical register touched by a post-allocation instruction.  SP is 32 and
// is reported so frame-pointer checks can see it; xzr (31) is included when
// the instruction actually names that register.
static int HoistIntReg(TargetInstruction* inst) {
  if (inst == NULL || inst->reg == NULL) {
    return -1;
  }
  AARCH64Register* reg = (AARCH64Register*)inst->reg;
  if (reg->type != kAARCH64RegTypeInt) {
    return -1;
  }
  int num = reg->base.num;
  if (num < 0 || num > 32) {
    return -1;
  }
  return num;
}

static void HoistNoteReg(uint64_t* bits, int num) {
  if (num >= 0 && num <= 32) {
    *bits |= UINT64_C(1) << num;
  }
}

// `call_clobbers` is for copy propagation, which must not forward a
// caller-saved register across a call.  Liveness for hoisting leaves it off
// so a value the allocator kept in a caller-saved register still looks live.
static void HoistInstEffects(TargetInstruction* inst, uint64_t* uses,
                             uint64_t* defs, bool call_clobbers) {
  *uses = 0;
  *defs = 0;
  if (inst == NULL) {
    return;
  }
  AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
  if (op == AARCH64_OP(loc) || op == AARCH64_OP(label) ||
      op == AARCH64_OP(named_label) || op == AARCH64_OP(eq) ||
      op == AARCH64_OP(ne) || op == AARCH64_OP(al)) {
    return;
  }
  if (op == AARCH64_OP(asm)) {
    *uses = ~UINT64_C(0);
    *defs = ~UINT64_C(0);
    return;
  }
  if (op == AARCH64_OP(reload)) {
    HoistNoteReg(defs, HoistIntReg(inst));
    return;
  }
  if (op == AARCH64_OP(spill)) {
    HoistNoteReg(uses, HoistIntReg(inst));
    return;
  }
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    HoistNoteReg(uses, HoistIntReg(inst->operand[i]));
  }
  if (AARCH64GeneratesOutput(inst)) {
    HoistNoteReg(defs, HoistIntReg(inst));
  }
  if (call_clobbers && (op == AARCH64_OP(bl) || op == AARCH64_OP(blr))) {
    for (int r = 0; r <= 18; r++) {
      *defs |= UINT64_C(1) << r;
    }
  }
  if (call_clobbers && op == AARCH64_OP(restore)) {
    for (int r = AARCH64_INT_SAVED_START; r <= AARCH64_LR_REG; r++) {
      *defs |= UINT64_C(1) << r;
    }
  }
}

static bool HoistIsBranch(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(b):
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz):
    case AARCH64_OP(tbz):
    case AARCH64_OP(tbnz):
    case AARCH64_OP(br):
    case AARCH64_OP(ret):
      return true;
    default:
      return false;
  }
}

static TargetInstruction* HoistNextInBlock(TargetBasicBlock* block,
                                           TargetInstruction* inst) {
  if (inst == NULL || block == NULL || inst == block->end_code) {
    return NULL;
  }
  return TargetNext(inst);
}

static bool HoistLoopHasSideEffect(TargetGenerator* gen, BitSet* blocks) {
  BitSetIterator it;
  BitSetIteratorStart(&it, blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
        if (op == AARCH64_OP(bl) || op == AARCH64_OP(blr) ||
            op == AARCH64_OP(asm) || op == AARCH64_OP(save) ||
            op == AARCH64_OP(restore)) {
          return true;
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

// Slot index stored on the spill, before the emitter adds the frame base.
// Negative slots exist (the aggregate-return slot); failure is a separate
// result so a real offset of -1 is not treated as "no slot".
static bool HoistSpillSlot(TargetInstruction* spill, int* off) {
  if (spill == NULL || (AARCH64Opcode)spill->opcode != AARCH64_OP(spill) ||
      spill->operand[1] == NULL || !TargetIsConst(spill->operand[1])) {
    return false;
  }
  *off = (int)TargetIntValue(spill->operand[1]);
  return true;
}

static bool HoistRefsValue(TargetInstruction* inst, TargetInstruction* value) {
  if (inst == NULL || value == NULL || inst == value) {
    return false;
  }
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] == value) {
      return true;
    }
  }
  return inst->dest == value;
}

// True for a block whose terminator is after `save` in the instruction list.
// Shrink-wrapping moves `save` and retargets branches, but it does not add
// the new block to the predecessor edge lists, so a dominator or edge
// dataflow still sees the fast path falling into the slow path.  The list
// order matches emission: fast blocks, then `save`, then the framed body.
static bool* HoistFramedAtEnd(TargetGenerator* gen) {
  size_t n = gen->basic_blocks.length;
  bool* framed = calloc(n, sizeof(bool));
  if (framed == NULL) {
    return NULL;
  }
  bool seen_save = false;
  for (TargetInstruction* inst = TargetFirstInstruction(gen); inst != NULL;
       inst = TargetNext(inst)) {
    if ((AARCH64Opcode)inst->opcode == AARCH64_OP(save)) {
      seen_save = true;
    }
    if (!seen_save || inst->block == NULL) {
      continue;
    }
    size_t id = inst->block->block_id;
    if (id < n && gen->basic_blocks.value.p[id] == inst->block) {
      framed[id] = true;
    }
  }
  return framed;
}

typedef struct {
  int header;
  int depth;
  int nlatches;
  int nentries;
  int* latches;
  int* entries;
  BitSet blocks;
} HoistLoop;

typedef struct {
  TargetInstruction* spill;
  AARCH64Opcode imm_op;
  int size;
  int64_t imm;
  bool is_imm;
  TargetInstruction* members[48];
  int nmembers;
} HoistGroup;

static bool HoistDominatesLatches(TargetGenerator* gen, TargetBasicBlock* block,
                                  HoistLoop* loop) {
  if (block == NULL || loop->nlatches <= 0) {
    return false;
  }
  for (int i = 0; i < loop->nlatches; i++) {
    TargetBasicBlock* latch = gen->basic_blocks.value.p[loop->latches[i]];
    if (latch == NULL ||
        !BitSetContains(&latch->dominators, block->block_id)) {
      return false;
    }
  }
  return true;
}

static uint64_t HoistLiveAtInsert(TargetBasicBlock* pred, uint64_t* live_out,
                                  uint64_t* extra_live) {
  uint64_t live = 0;
  if (pred->block_id < 0) {
    return ~UINT64_C(0);
  }
  live = live_out[pred->block_id] | extra_live[pred->block_id];
  if (pred->end_code != NULL && HoistIsBranch(pred->end_code)) {
    uint64_t uses = 0;
    uint64_t defs = 0;
    HoistInstEffects(pred->end_code, &uses, &defs, false);
    live = (live & ~defs) | uses;
  }
  return live;
}

static bool HoistEntriesReady(TargetGenerator* gen, HoistLoop* loop,
                              bool need_frame, const bool* framed) {
  if (loop->nentries <= 0) {
    return false;
  }
  for (int i = 0; i < loop->nentries; i++) {
    size_t pred_id = (size_t)loop->entries[i];
    TargetBasicBlock* pred = gen->basic_blocks.value.p[pred_id];
    if (pred == NULL || pred->code == NULL || pred->end_code == NULL) {
      return false;
    }
    for (size_t e = 0; e < pred->out_edges.length; e++) {
      size_t succ = pred->out_edges.value.w[e];
      if (!BitSetContains(&loop->blocks, succ)) {
        return false;
      }
    }
    if (need_frame && (framed == NULL || !framed[pred_id])) {
      return false;
    }
  }
  return true;
}

static void HoistInsert(TargetGenerator* gen, TargetBasicBlock* pred,
                        TargetInstruction* inst) {
  if (HoistIsBranch(pred->end_code)) {
    TargetBasicBlockEmitBefore(gen, pred, inst, pred->end_code);
  } else {
    TargetBasicBlockEmitAfter(gen, pred, inst, pred->end_code);
  }
}

static TargetInstruction* HoistMakeReload(AARCH64RegisterAllocator* allocator,
                                          TargetInstruction* spill, int reg,
                                          int size) {
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)AARCH64_OP(reload), spill);
  SetInstructionSize(copy, size);
  copy->reg = &allocator->int_regs[reg].base;
  copy->flags |= TARGET_INST_PROCESSED;
  return copy;
}

static TargetInstruction* HoistMakeImmediate(AARCH64RegisterAllocator* allocator,
                                             HoistGroup* group, int reg) {
  TargetInstruction* proto = group->members[0];
  TargetInstruction* copy = TargetNewInstruction1(
      (TargetOpcode)group->imm_op, proto->operand[0]);
  SetInstructionSize(copy, group->size);
  copy->reg = &allocator->int_regs[reg].base;
  copy->flags |= TARGET_INST_PROCESSED;
  return copy;
}

static void HoistOneGroup(AARCH64RegisterAllocator* allocator, HoistLoop* loop,
                          HoistGroup* group, int reg, uint64_t* extra_live) {
  TargetGenerator* gen = &allocator->g->base;
  TargetBasicBlock* first_pred = gen->basic_blocks.value.p[loop->entries[0]];
  TargetInstruction* primary =
      group->is_imm ? HoistMakeImmediate(allocator, group, reg)
                    : HoistMakeReload(allocator, group->spill, reg, group->size);
  HoistInsert(gen, first_pred, primary);
  extra_live[first_pred->block_id] |= UINT64_C(1) << reg;
  for (int e = 1; e < loop->nentries; e++) {
    TargetBasicBlock* pred = gen->basic_blocks.value.p[loop->entries[e]];
    TargetInstruction* copy =
        group->is_imm ? HoistMakeImmediate(allocator, group, reg)
                      : HoistMakeReload(allocator, group->spill, reg, group->size);
    HoistInsert(gen, pred, copy);
    extra_live[pred->block_id] |= UINT64_C(1) << reg;
  }
  for (int m = 0; m < group->nmembers; m++) {
    TargetInstruction* old = group->members[m];
    TargetBasicBlock* block = old->block;
    // ReloadSpills points the consumer at the reload without recording the
    // use.  Retarget the recorded users, then any operand that still names
    // the old instruction.  Remove deletes a user that still references the
    // instruction, so both updates have to happen first.
    TargetRetargetInstruction(old, primary);
    for (size_t b = 0; b < gen->basic_blocks.length; b++) {
      TargetBasicBlock* scan = gen->basic_blocks.value.p[b];
      if (scan == NULL || scan->code == NULL) {
        continue;
      }
      for (TargetInstruction* inst = scan->code; inst != NULL;
           inst = HoistNextInBlock(scan, inst)) {
        bool ref = false;
        for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
          if (inst->operand[i] == old) {
            inst->operand[i] = primary;
            ref = true;
          }
        }
        if (inst->dest == old) {
          inst->dest = primary;
          ref = true;
        }
        if (ref) {
          TargetAddUser(primary, inst);
        }
        if (inst == scan->end_code) {
          break;
        }
      }
    }
    if (block != NULL) {
      TargetBasicBlockRemoveInstruction(gen, block, old);
    }
  }
}

static int HoistPickReg(const uint64_t blocked) {
  static const int kRegs[] = {10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};
  for (size_t i = 0; i < sizeof(kRegs) / sizeof(kRegs[0]); i++) {
    if ((blocked & (UINT64_C(1) << kRegs[i])) == 0) {
      return kRegs[i];
    }
  }
  return -1;
}

// True when every real reference is inside the loop and there is at least
// one.  Recorded users that no longer name the value are stale (the spill
// node keeps consumers whose operands were rewritten to a reload) and do
// not count.
static bool HoistGroupStaysInLoop(TargetGenerator* gen, HoistGroup* group,
                                  BitSet* blocks) {
  bool any = false;
  for (int m = 0; m < group->nmembers; m++) {
    TargetInstruction* value = group->members[m];
    for (size_t u = 0; u < value->users.length; u++) {
      TargetInstruction* user = value->users.value.p[u];
      if (!HoistRefsValue(user, value)) {
        continue;
      }
      if (user->block == NULL ||
          !BitSetContains(blocks, user->block->block_id)) {
        return false;
      }
      any = true;
    }
  }
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[b];
    if (block == NULL || block->code == NULL) {
      continue;
    }
    bool in_loop = BitSetContains(blocks, block->block_id);
    for (TargetInstruction* inst = block->code; inst != NULL;
         inst = HoistNextInBlock(block, inst)) {
      for (int m = 0; m < group->nmembers; m++) {
        if (!HoistRefsValue(inst, group->members[m])) {
          continue;
        }
        if (!in_loop) {
          return false;
        }
        any = true;
      }
      if (inst == block->end_code) {
        break;
      }
    }
  }
  return any;
}

static int HoistGroupWeight(const HoistGroup* group) {
  // A reload is a memory access; a materialized immediate is one alu op.
  return group->nmembers * 8 + (group->is_imm ? 0 : 4);
}

static void HoistGroupsInLoop(AARCH64RegisterAllocator* allocator,
                              HoistLoop* loop, uint64_t* live_in,
                              uint64_t* live_out, uint64_t* extra_live,
                              const bool* framed) {
  TargetGenerator* gen = &allocator->g->base;
  if (HoistLoopHasSideEffect(gen, &loop->blocks)) {
    return;
  }
  bool spill_entries = HoistEntriesReady(gen, loop, true, framed);
  bool imm_entries = HoistEntriesReady(gen, loop, false, framed);
  if (!spill_entries && !imm_entries) {
    return;
  }

  uint64_t used = 0;
  bool frame_stable = true;
  BitSetIterator it;
  BitSetIteratorStart(&it, &loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        uint64_t uses = 0;
        uint64_t defs = 0;
        HoistInstEffects(inst, &uses, &defs, false);
        used |= uses | defs;
        if ((defs & ((UINT64_C(1) << AARCH64_SP_REG) |
                     (UINT64_C(1) << AARCH64_FP_REG))) != 0) {
          frame_stable = false;
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }
  if (!frame_stable) {
    spill_entries = false;
  }

  int killed_off[64];
  int nkilled = 0;
  BitSetIteratorStart(&it, &loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        int off = 0;
        if ((AARCH64Opcode)inst->opcode == AARCH64_OP(spill) &&
            nkilled < 64 && HoistSpillSlot(inst, &off)) {
          killed_off[nkilled++] = off;
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }

  HoistGroup groups[32];
  int ngroups = 0;
  BitSetIteratorStart(&it, &loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    TargetBasicBlock* block =
        gen->basic_blocks.value.p[BitSetIteratorValue(&it)];
    if (block != NULL && block->code != NULL) {
      for (TargetInstruction* inst = block->code; inst != NULL;
           inst = HoistNextInBlock(block, inst)) {
        bool take = false;
        TargetInstruction* spill = NULL;
        bool is_imm = false;
        int64_t imm = 0;
        AARCH64Opcode imm_op = AARCH64_OP(mov);
        if ((AARCH64Opcode)inst->opcode == AARCH64_OP(reload) &&
            inst->dest == NULL) {
          spill = inst->operand[0];
          int off = 0;
          int reg = HoistIntReg(inst);
          bool have_slot = spill != NULL && HoistSpillSlot(spill, &off);
          bool killed = false;
          for (int k = 0; k < nkilled; k++) {
            if (killed_off[k] == off) {
              killed = true;
              break;
            }
          }
          // An orphan spill handle has no block of its own.  Store-backs in
          // the loop share its slot and are caught by `killed`.
          bool spill_outside =
              spill != NULL &&
              (spill->block == NULL ||
               !BitSetContains(&loop->blocks, spill->block->block_id));
          bool dominates = HoistDominatesLatches(gen, block, loop);
          if (spill_entries && have_slot && reg >= 0 && !killed &&
              spill_outside && dominates) {
            take = true;
          }
        } else if (imm_entries && inst->dest == NULL &&
                   inst->operand[0] != NULL &&
                   TargetIsConst(inst->operand[0]) &&
                   ((AARCH64Opcode)inst->opcode == AARCH64_OP(mov) ||
                    (AARCH64Opcode)inst->opcode == AARCH64_OP(movz) ||
                    (AARCH64Opcode)inst->opcode == AARCH64_OP(movn)) &&
                   HoistIntReg(inst) >= 0 &&
                   HoistDominatesLatches(gen, block, loop)) {
          is_imm = true;
          imm_op = (AARCH64Opcode)inst->opcode;
          imm = TargetIntValue(inst->operand[0]);
          take = true;
        }
        if (take) {
          int found = -1;
          int size = GetRegisterSize(inst);
          for (int g = 0; g < ngroups; g++) {
            if (is_imm) {
              if (groups[g].is_imm && groups[g].imm_op == imm_op &&
                  groups[g].size == size && groups[g].imm == imm) {
                found = g;
                break;
              }
            } else if (!groups[g].is_imm && groups[g].spill == spill) {
              found = g;
              break;
            }
          }
          if (found < 0 && ngroups < 32) {
            found = ngroups++;
            groups[found].spill = spill;
            groups[found].is_imm = is_imm;
            groups[found].imm_op = imm_op;
            groups[found].size = size;
            groups[found].imm = imm;
            groups[found].nmembers = 0;
          }
          if (found >= 0 && groups[found].nmembers < 48) {
            groups[found].members[groups[found].nmembers++] = inst;
          }
        }
        if (inst == block->end_code) {
          break;
        }
      }
    }
    BitSetIteratorNext(&it);
  }

  uint64_t blocked = used | live_in[loop->header];
  for (int e = 0; e < loop->nentries; e++) {
    TargetBasicBlock* pred = gen->basic_blocks.value.p[loop->entries[e]];
    blocked |= HoistLiveAtInsert(pred, live_out, extra_live);
  }
  // Heaviest groups first so a scarce register covers the hottest reload.
  for (int a = 0; a < ngroups; a++) {
    for (int b = a + 1; b < ngroups; b++) {
      if (HoistGroupWeight(&groups[b]) > HoistGroupWeight(&groups[a])) {
        HoistGroup tmp = groups[a];
        groups[a] = groups[b];
        groups[b] = tmp;
      }
    }
  }
  for (int g = 0; g < ngroups; g++) {
    if (!HoistGroupStaysInLoop(gen, &groups[g], &loop->blocks)) {
      continue;
    }
    int reg = HoistPickReg(blocked);
    if (reg < 0) {
      break;
    }
    HoistOneGroup(allocator, loop, &groups[g], reg, extra_live);
    blocked |= UINT64_C(1) << reg;
  }
}

static int HoistLoopDeeper(const void* lhs, const void* rhs) {
  const HoistLoop* a = lhs;
  const HoistLoop* b = rhs;
  return b->depth - a->depth;
}

static void HoistLoopInvariantValues(AARCH64RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  size_t nblocks = gen->basic_blocks.length;
  if (!OptLevel2() || gen->entry_block == NULL || nblocks == 0) {
    return;
  }

  uint64_t* live_in = calloc(nblocks, sizeof(uint64_t));
  uint64_t* live_out = calloc(nblocks, sizeof(uint64_t));
  uint64_t* extra_live = calloc(nblocks, sizeof(uint64_t));
  if (live_in == NULL || live_out == NULL || extra_live == NULL) {
    free(live_in);
    free(live_out);
    free(extra_live);
    return;
  }
  // Live sets only grow, so a full reverse sweep converges.  Poison the
  // result if it does not: hoisting then sees every register as live and
  // leaves the function unchanged.
  int guard = 0;
  int guard_max = (int)nblocks * 33 + 2;
  bool grew = true;
  while (grew && guard++ < guard_max) {
    grew = false;
    for (size_t id = nblocks; id-- > 0;) {
      TargetBasicBlock* block = gen->basic_blocks.value.p[id];
      uint64_t out = 0;
      if (block != NULL) {
        for (size_t e = 0; e < block->out_edges.length; e++) {
          size_t succ = block->out_edges.value.w[e];
          if (succ < nblocks) {
            out |= live_in[succ];
          }
        }
      }
      live_out[id] = out;
      uint64_t cur = out;
      if (block != NULL && block->code != NULL && block->end_code != NULL) {
        for (TargetInstruction* inst = block->end_code; inst != NULL;
             inst = TargetPrev(inst)) {
          uint64_t uses = 0;
          uint64_t defs = 0;
          HoistInstEffects(inst, &uses, &defs, false);
          cur = (cur & ~defs) | uses;
          if (inst == block->code) {
            break;
          }
        }
      }
      if (cur != live_in[id]) {
        live_in[id] = cur;
        grew = true;
      }
    }
  }
  if (grew) {
    for (size_t i = 0; i < nblocks; i++) {
      live_in[i] = ~UINT64_C(0);
      live_out[i] = ~UINT64_C(0);
    }
  }

  HoistLoop* loops = calloc(nblocks, sizeof(HoistLoop));
  int nloops = 0;
  if (loops != NULL) {
    for (size_t header_id = 0; header_id < nblocks; header_id++) {
      TargetBasicBlock* header = gen->basic_blocks.value.p[header_id];
      if (header == NULL || header->is_unreachable) {
        continue;
      }
      int nlatch = 0;
      for (size_t i = 0; i < header->in_edges.length; i++) {
        size_t pred = header->in_edges.value.w[i];
        TargetBasicBlock* latch = gen->basic_blocks.value.p[pred];
        if (latch != NULL &&
            BitSetContains(&latch->dominators, header_id)) {
          nlatch++;
        }
      }
      if (nlatch == 0 || nloops >= (int)nblocks) {
        continue;
      }
      HoistLoop* loop = &loops[nloops];
      BitSetInit(&loop->blocks);
      BitSetInsert(&loop->blocks, header_id);
      size_t* work = malloc(nblocks * sizeof(size_t));
      size_t nw = 0;
      if (work == NULL) {
        BitSetDestruct(&loop->blocks);
        break;
      }
      for (size_t i = 0; i < header->in_edges.length; i++) {
        size_t pred = header->in_edges.value.w[i];
        TargetBasicBlock* latch = gen->basic_blocks.value.p[pred];
        if (latch != NULL &&
            BitSetContains(&latch->dominators, header_id) &&
            !BitSetContains(&loop->blocks, pred)) {
          BitSetInsert(&loop->blocks, pred);
          work[nw++] = pred;
        }
      }
      while (nw > 0) {
        size_t id = work[--nw];
        TargetBasicBlock* block = gen->basic_blocks.value.p[id];
        if (block == NULL) {
          continue;
        }
        for (size_t i = 0; i < block->in_edges.length; i++) {
          size_t pred = block->in_edges.value.w[i];
          if (!BitSetContains(&loop->blocks, pred)) {
            BitSetInsert(&loop->blocks, pred);
            work[nw++] = pred;
          }
        }
      }
      free(work);
      loop->header = (int)header_id;
      loop->depth = header->loop_nesting;
      loop->latches = malloc((size_t)nlatch * sizeof(int));
      loop->entries = malloc(header->in_edges.length * sizeof(int));
      loop->nlatches = 0;
      loop->nentries = 0;
      if (loop->latches == NULL || loop->entries == NULL) {
        free(loop->latches);
        free(loop->entries);
        BitSetDestruct(&loop->blocks);
        break;
      }
      for (size_t i = 0; i < header->in_edges.length; i++) {
        size_t pred = header->in_edges.value.w[i];
        if (BitSetContains(&loop->blocks, pred)) {
          loop->latches[loop->nlatches++] = (int)pred;
        } else {
          loop->entries[loop->nentries++] = (int)pred;
        }
      }
      nloops++;
    }
    qsort(loops, (size_t)nloops, sizeof(HoistLoop), HoistLoopDeeper);
    bool* framed = HoistFramedAtEnd(gen);
    for (int i = 0; i < nloops; i++) {
      HoistGroupsInLoop(allocator, &loops[i], live_in, live_out, extra_live,
                        framed);
    }
    free(framed);
    for (int i = 0; i < nloops; i++) {
      BitSetDestruct(&loops[i].blocks);
      free(loops[i].latches);
      free(loops[i].entries);
    }
  }
  free(loops);
  free(live_in);
  free(live_out);
  free(extra_live);
}

// `mov xD, xS` left by allocation.  Forward xS to same-block users when
// nothing in between overwrites either register.  32-bit moves zero the
// upper half and are left alone, as are moves that also write a dest, whose
// readers are not among the users.
static bool FoldPhysicalCopy(TargetGenerator* gen, TargetBasicBlock* block,
                             TargetInstruction* mov) {
  AARCH64Opcode op = (AARCH64Opcode)mov->opcode;
  if ((op != AARCH64_OP(mov) && op != AARCH64_OP(mv)) ||
      mov->operand[1] != NULL || mov->operand[0] == NULL ||
      mov->dest != NULL || TargetIsConst(mov->operand[0]) ||
      mov->users.length == 0 || mov->block != block) {
    return false;
  }
  int size = GetRegisterSize(mov);
  if (size != 0 && size != kSize64Bit) {
    return false;
  }
  int dst = HoistIntReg(mov);
  int src = HoistIntReg(mov->operand[0]);
  if (dst < 0 || src < 0 || dst > 30 || src > 30) {
    return false;
  }
  // Argument and scratch registers are read implicitly by calls and by
  // spill addressing.  Forwarding through them drops that write.
  if (dst <= 8 || dst == AARCH64_IP1_REG || dst == AARCH64_IP2_REG ||
      src == AARCH64_IP1_REG || src == AARCH64_IP2_REG) {
    return false;
  }
  uint64_t dst_bit = UINT64_C(1) << dst;
  uint64_t src_bit = UINT64_C(1) << src;
  size_t nusers = mov->users.length;
  size_t seen = 0;
  for (TargetInstruction* inst = HoistNextInBlock(block, mov); inst != NULL;
       inst = HoistNextInBlock(block, inst)) {
    bool is_user = false;
    for (size_t u = 0; u < nusers; u++) {
      TargetInstruction* user = mov->users.value.p[u];
      if (user == inst) {
        is_user = true;
        break;
      }
      if (user == NULL || user->block != block || user->dest == mov) {
        return false;
      }
    }
    uint64_t uses = 0;
    uint64_t defs = 0;
    HoistInstEffects(inst, &uses, &defs, true);
    if (!is_user) {
      if ((uses & dst_bit) != 0 || (defs & dst_bit) != 0 ||
          (defs & src_bit) != 0) {
        return false;
      }
    } else {
      seen++;
      if (((defs & dst_bit) != 0 || (defs & src_bit) != 0) && seen < nusers) {
        return false;
      }
      if (seen == nusers) {
        break;
      }
    }
    if (inst == block->end_code) {
      break;
    }
  }
  if (seen != nusers) {
    return false;
  }
  TargetInstruction* source = mov->operand[0];
  TargetRetargetInstruction(mov, source);
  TargetBasicBlockRemoveInstruction(gen, block, mov);
  return true;
}

static void FoldPhysicalCopies(AARCH64RegisterAllocator* allocator) {
  TargetGenerator* gen = &allocator->g->base;
  if (!OptLevel2() || gen->entry_block == NULL) {
    return;
  }
  bool changed = true;
  for (int pass = 0; changed && pass < 8; pass++) {
    changed = false;
    for (size_t b = 0; b < gen->basic_blocks.length; b++) {
      TargetBasicBlock* block = gen->basic_blocks.value.p[b];
      if (block == NULL || block->code == NULL) {
        continue;
      }
      bool folded = true;
      while (folded) {
        folded = false;
        for (TargetInstruction* inst = block->code; inst != NULL;
             inst = HoistNextInBlock(block, inst)) {
          if (FoldPhysicalCopy(gen, block, inst)) {
            folded = true;
            changed = true;
            break;
          }
          if (inst == block->end_code) {
            break;
          }
        }
      }
    }
  }
}
// Shrink wrapping, hoisting and copy folding work on the physical registers
// coloring assigned.
void AARCH64AllocateRegisters(AARCH64RegisterAllocator* allocator) {
  if (getenv("DAVECC_RA_CHECK") != NULL) {
    TargetRACheck(&allocator->g->base, AARCH64RAInfo(), NULL, stderr, 3);
  }
  AARCH64AllocateRegistersByColoring(allocator);
  ShrinkWrapFunction(allocator);
  HoistLoopInvariantValues(allocator);
  FoldPhysicalCopies(allocator);
}


const char* AARCH64RegisterName(AARCH64Register* reg, int size, char* buf, size_t len) {
  return AARCH64RegisterNameFromNum(reg->base.num, reg->type, size, buf, len);
}

const char* AARCH64RegisterNameFromNum(int num, AARCH64RegisterType type, int size,
                                       char* buf, size_t len) {
  switch (type) {
    case kAARCH64RegTypeInt:
      if (num == AARCH64_FP_REG) {
        snprintf(buf, len, "x29");
        break;
      }
      if (num == AARCH64_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (num == AARCH64_LR_REG) {
        snprintf(buf, len, "x30");
        break;
      }
      if (num == AARCH64_INT_ZERO_REG) {
        snprintf(buf, len, "%s", size == kSize32Bit ? "wzr" : "xzr");
        break;
      }

      // Honor the requested operand width: a 32-bit operation must use the
      // "w" register so that, in particular, stores write 4 bytes (str w<n>)
      // rather than 8 (str x<n>), which would clobber the adjacent stack slot.
      // An unspecified size (0) keeps the historical 64-bit "x" name.
      if (size == kSize32Bit) {
        snprintf(buf, len, "w%d", num);
      } else {
        snprintf(buf, len, "x%d", num);
      }
      return buf;

    case kAARCH64RegTypeFloat:
      // Honor the requested operand width: a 32-bit (single precision) value
      // must use the "s" register so that, in particular, stores write 4 bytes
      // (fstr s<n>) rather than 8 (fstr d<n>), which would clobber the adjacent
      // stack slot, and arithmetic rounds at single precision.  An unspecified
      // size (0) keeps the 64-bit "d" name.
      if (size == kSize32Bit) {
        snprintf(buf, len, "s%d", num);
      } else if (size == kSize128Bit) {
        snprintf(buf, len, "q%d", num);
      } else {
        snprintf(buf, len, "d%d", num);
      }
      return buf;
  }
  return buf;
}
