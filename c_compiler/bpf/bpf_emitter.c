#include "bpf_emitter.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "bpf_assembler.h"
#include "common_emitter.h"
#include "compiler.h"
#include "symbol.h"

void BPFEmitterInit(BPFEmitter* emitter, BPFGenerator* bpf) {
  emitter->bpf = bpf;
  emitter->regs = &bpf->register_allocator;
  emitter->saved_reg_bytes = BPF_NUM_SAVED_REGS * 8;
  emitter->spill_bytes = bpf->register_allocator.max_spill_size;
}

void BPFEmitterDestruct(BPFEmitter* emitter) { (void)emitter; }

static int RegNum(TargetInstruction* inst) {
  if (inst == NULL) {
    return 0;
  }
  if (inst->reg != NULL) {
    return inst->reg->num;
  }
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(resulti):
    case BPF_OP(r0):
      return BPF_REG_0;
    case BPF_OP(r1):
      return BPF_REG_1;
    case BPF_OP(r2):
      return BPF_REG_2;
    case BPF_OP(r3):
      return BPF_REG_3;
    case BPF_OP(r4):
      return BPF_REG_4;
    case BPF_OP(r5):
      return BPF_REG_5;
    case BPF_OP(fp):
    case BPF_OP(sp):
      return BPF_REG_FP;
    default:
      return 0;
  }
}

static int DestReg(TargetInstruction* inst) {
  if (inst->dest != NULL) {
    return RegNum(inst->dest);
  }
  return RegNum(inst);
}

static const char* LabelName(const char* func_name, TargetInstruction* inst,
                             char* buf, size_t len) {
  if (inst == NULL) {
    snprintf(buf, len, ".L%s_missing", func_name);
    return buf;
  }
  if ((BPFOpcode)inst->opcode == BPF_OP(named_label)) {
    return ((TargetNamedLabel*)inst)->name;
  }
  snprintf(buf, len, ".L%s_%d", func_name, inst->id);
  return buf;
}

static const char* SymbolName(TargetInstruction* inst, char* buf, size_t len) {
  if (inst != NULL && BPFIsSymbol(inst)) {
    return TargetSymbolName(((TargetSymbol*)inst)->symbol, buf, len);
  }
  snprintf(buf, len, "0");
  return buf;
}

static bool IsPrintable(TargetInstruction* inst) {
  if (TargetIsConst(inst)) {
    return false;
  }
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(tmp):
    case BPF_OP(fp):
    case BPF_OP(sp):
    case BPF_OP(tp):
    case BPF_OP(literal):
    case BPF_OP(structreturn):
    case BPF_OP(resulti):
    case BPF_OP(resultf):
    case BPF_OP(resultd):
    case BPF_OP(r0):
    case BPF_OP(r1):
    case BPF_OP(r2):
    case BPF_OP(r3):
    case BPF_OP(r4):
    case BPF_OP(r5):
    case BPF_OP(ivarreg):
    case BPF_OP(fvarreg):
    case BPF_OP(symbol):
      return false;
    default:
      return true;
  }
}

static const char* AluName(BPFOpcode op) {
  switch (op) {
    case BPF_OP(mov):
    case BPF_OP(li):
    case BPF_OP(movc):
    case BPF_OP(movxc):
    case BPF_OP(lddw):
      return "mov64";
    case BPF_OP(mov32):
      return "mov32";
    case BPF_OP(add):
      return "add64";
    case BPF_OP(add32):
      return "add32";
    case BPF_OP(sub):
      return "sub64";
    case BPF_OP(sub32):
      return "sub32";
    case BPF_OP(mul):
      return "mul64";
    case BPF_OP(mul32):
      return "mul32";
    case BPF_OP(div):
      return "div64";
    case BPF_OP(div32):
      return "div32";
    case BPF_OP(mod):
      return "mod64";
    case BPF_OP(mod32):
      return "mod32";
    case BPF_OP(or):
      return "or64";
    case BPF_OP(or32):
      return "or32";
    case BPF_OP(and):
      return "and64";
    case BPF_OP(and32):
      return "and32";
    case BPF_OP(xor):
      return "xor64";
    case BPF_OP(xor32):
      return "xor32";
    case BPF_OP(lsh):
      return "lsh64";
    case BPF_OP(lsh32):
      return "lsh32";
    case BPF_OP(rsh):
      return "rsh64";
    case BPF_OP(rsh32):
      return "rsh32";
    case BPF_OP(arsh):
      return "arsh64";
    case BPF_OP(arsh32):
      return "arsh32";
    default:
      return "add64";
  }
}

static const char* JccName(BPFOpcode op) {
  switch (op) {
    case BPF_OP(jeq):
      return "jeq";
    case BPF_OP(jgt):
      return "jgt";
    case BPF_OP(jge):
      return "jge";
    case BPF_OP(jlt):
      return "jlt";
    case BPF_OP(jle):
      return "jle";
    case BPF_OP(jset):
      return "jset";
    case BPF_OP(jne):
      return "jne";
    case BPF_OP(jsgt):
      return "jsgt";
    case BPF_OP(jsge):
      return "jsge";
    case BPF_OP(jslt):
      return "jslt";
    case BPF_OP(jsle):
      return "jsle";
    default:
      return "jeq";
  }
}

static const char* LoadName(BPFOpcode op) {
  switch (op) {
    case BPF_OP(ldxb):
      return "ldxb";
    case BPF_OP(ldxh):
      return "ldxh";
    case BPF_OP(ldxw):
      return "ldxw";
    default:
      return "ldxdw";
  }
}

static const char* StoreName(BPFOpcode op) {
  switch (op) {
    case BPF_OP(stxb):
      return "stxb";
    case BPF_OP(stxh):
      return "stxh";
    case BPF_OP(stxw):
      return "stxw";
    default:
      return "stxdw";
  }
}

static int LocalOffset(BPFEmitter* emitter, int var_offset) {
  return -(emitter->bpf->base.stack_frame_size - var_offset);
}

static void PrintAlu(BPFEmitter* emitter, TargetInstruction* inst, FILE* fp) {
  (void)emitter;
  int dst = DestReg(inst);
  TargetInstruction* src0 = inst->operand[0];
  TargetInstruction* src1 = inst->operand[1];
  const char* name = AluName((BPFOpcode)inst->opcode);
  if (src1 == NULL) {
    if (TargetIsConst(src0) || BPFIsSymbol(src0)) {
      if (BPFIsSymbol(src0)) {
        char buf[256];
        fprintf(fp, "\tlddw r%d, %s\n", dst, SymbolName(src0, buf, sizeof(buf)));
      } else {
        int64_t v = TargetIntValue(src0);
        if ((int64_t)(int32_t)v == v) {
          fprintf(fp, "\t%s r%d, %" PRId64 "\n", name, dst, v);
        } else {
          fprintf(fp, "\tlddw r%d, %" PRId64 "\n", dst, v);
        }
      }
      return;
    }
    int s = RegNum(src0);
    if (s != dst) {
      fprintf(fp, "\tmov64 r%d, r%d\n", dst, s);
    }
    return;
  }
  int s0 = RegNum(src0);
  if (TargetIsConst(src0)) {
    fprintf(fp, "\tmov64 r%d, %" PRId64 "\n", dst, TargetIntValue(src0));
  } else if (s0 != dst) {
    fprintf(fp, "\tmov64 r%d, r%d\n", dst, s0);
  }
  if (TargetIsConst(src1)) {
    fprintf(fp, "\t%s r%d, %" PRId64 "\n", name, dst, TargetIntValue(src1));
  } else {
    fprintf(fp, "\t%s r%d, r%d\n", name, dst, RegNum(src1));
  }
}

static void PrintMemOff(int base, TargetInstruction* off, FILE* fp) {
  int64_t o = 0;
  if (off != NULL && TargetIsConst(off)) {
    o = TargetIntValue(off);
  }
  if (o == 0) {
    fprintf(fp, "[r%d]", base);
  } else if (o < 0) {
    fprintf(fp, "[r%d%" PRId64 "]", base, o);
  } else {
    fprintf(fp, "[r%d+%" PRId64 "]", base, o);
  }
}

static void PrintInstruction(BPFEmitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (!IsPrintable(inst)) {
    return;
  }
  BPFOpcode op = (BPFOpcode)inst->opcode;
  char buf[256];
  switch (op) {
    case BPF_OP(save): {
      int off = 0;
      for (int r = BPF_FIRST_SAVED_REG; r <= BPF_LAST_SAVED_REG; r++) {
        off += 8;
        fprintf(fp, "\tstxdw [r10-%d], r%d\n", off, r);
      }
      for (size_t i = 0; i < emitter->bpf->saved_args.length; i++) {
        BPFSavedArgument* a = emitter->bpf->saved_args.value.p[i];
        int loff = LocalOffset(emitter, a->offset);
        const char* st = a->size == 4 ? "stxw" : "stxdw";
        fprintf(fp, "\t%s [r10%d], r%d\n", st, loff, a->reg_num);
      }
      return;
    }
    case BPF_OP(restore):
    case BPF_OP(ret):
    case BPF_OP(exit): {
      int off = 0;
      for (int r = BPF_FIRST_SAVED_REG; r <= BPF_LAST_SAVED_REG; r++) {
        off += 8;
        fprintf(fp, "\tldxdw r%d, [r10-%d]\n", r, off);
      }
      fprintf(fp, "\texit\n");
      return;
    }
    case BPF_OP(label):
      fprintf(fp, ".L%s_%d:\n", func_name, inst->id);
      return;
    case BPF_OP(named_label):
      fprintf(fp, "%s:\n", ((TargetNamedLabel*)inst)->name);
      return;
    case BPF_OP(nop):
      fprintf(fp, "\tnop\n");
      return;
    case BPF_OP(li):
    case BPF_OP(lddw):
    case BPF_OP(mov):
    case BPF_OP(mov32):
    case BPF_OP(movc):
    case BPF_OP(movxc):
      PrintAlu(emitter, inst, fp);
      return;
    case BPF_OP(add):
    case BPF_OP(add32):
    case BPF_OP(sub):
    case BPF_OP(sub32):
    case BPF_OP(mul):
    case BPF_OP(mul32):
    case BPF_OP(div):
    case BPF_OP(div32):
    case BPF_OP(mod):
    case BPF_OP(mod32):
    case BPF_OP(or):
    case BPF_OP(or32):
    case BPF_OP(and):
    case BPF_OP(and32):
    case BPF_OP(xor):
    case BPF_OP(xor32):
    case BPF_OP(lsh):
    case BPF_OP(lsh32):
    case BPF_OP(rsh):
    case BPF_OP(rsh32):
    case BPF_OP(arsh):
    case BPF_OP(arsh32):
      PrintAlu(emitter, inst, fp);
      return;
    case BPF_OP(neg):
    case BPF_OP(neg32):
      fprintf(fp, "\t%s r%d\n", op == BPF_OP(neg32) ? "neg32" : "neg64",
              DestReg(inst));
      return;
    case BPF_OP(ldxb):
    case BPF_OP(ldxh):
    case BPF_OP(ldxw):
    case BPF_OP(ldxdw):
    case BPF_OP(reload):
      fprintf(fp, "\t%s r%d, ", LoadName(op == BPF_OP(reload) ? BPF_OP(ldxdw) : op),
              DestReg(inst));
      PrintMemOff(RegNum(inst->operand[0]), inst->operand[1], fp);
      fprintf(fp, "\n");
      return;
    case BPF_OP(stxb):
    case BPF_OP(stxh):
    case BPF_OP(stxw):
    case BPF_OP(stxdw):
    case BPF_OP(spill):
      fprintf(fp, "\t%s ", StoreName(op == BPF_OP(spill) ? BPF_OP(stxdw) : op));
      PrintMemOff(RegNum(inst->operand[0]), inst->operand[1], fp);
      fprintf(fp, ", r%d\n", RegNum(inst->operand[2] != NULL ? inst->operand[2]
                                                            : inst->operand[1]));
      return;
    case BPF_OP(ja):
      fprintf(fp, "\tja %s\n",
              LabelName(func_name, inst->operand[0], buf, sizeof(buf)));
      return;
    case BPF_OP(jeq):
    case BPF_OP(jgt):
    case BPF_OP(jge):
    case BPF_OP(jlt):
    case BPF_OP(jle):
    case BPF_OP(jset):
    case BPF_OP(jne):
    case BPF_OP(jsgt):
    case BPF_OP(jsge):
    case BPF_OP(jslt):
    case BPF_OP(jsle): {
      TargetInstruction* rhs = inst->operand[1];
      TargetInstruction* lab = inst->operand[2] != NULL ? inst->operand[2]
                                                        : inst->operand[1];
      if (rhs != NULL && TargetIsConst(rhs) && inst->operand[2] != NULL) {
        fprintf(fp, "\t%s r%d, %" PRId64 ", %s\n", JccName(op),
                RegNum(inst->operand[0]), TargetIntValue(rhs),
                LabelName(func_name, lab, buf, sizeof(buf)));
      } else if (rhs != NULL && inst->operand[2] != NULL) {
        fprintf(fp, "\t%s r%d, r%d, %s\n", JccName(op), RegNum(inst->operand[0]),
                RegNum(rhs), LabelName(func_name, lab, buf, sizeof(buf)));
      } else {
        fprintf(fp, "\t%s r%d, 0, %s\n", JccName(op), RegNum(inst->operand[0]),
                LabelName(func_name, lab, buf, sizeof(buf)));
      }
      return;
    }
    case BPF_OP(call): {
      TargetInstruction* callee = inst->operand[0];
      if (callee != NULL && BPFIsSymbol(callee)) {
        fprintf(fp, "\tcall %s\n", SymbolName(callee, buf, sizeof(buf)));
      } else {
        fprintf(fp, "\tcall r%d\n", RegNum(callee));
      }
      return;
    }
    case BPF_OP(loc):
      return;
    case BPF_OP(asm):
      if (inst->operand[0] != NULL) {
        fprintf(fp, "\t// inline asm %d\n", inst->operand[0]->id);
      }
      return;
    default:
      fprintf(fp, "\t// unimplemented opcode %s\n", BPFOpcodeName(op));
      return;
  }
}

void BPFPrintFunction(BPFEmitter* emitter, FILE* fp) {
  const char* func_name = emitter->bpf->base.function_name.value;
  EmitFunctionSection(fp, func_name);
  if (emitter->bpf->base.is_weak) {
    fprintf(fp, "\t.weak %s\n", func_name);
  } else if (emitter->bpf->base.is_global) {
    fprintf(fp, "\t.global %s\n", func_name);
  } else {
    fprintf(fp, "\t.local %s\n", func_name);
  }
  fprintf(fp, "\t.type %s, @function\n\n", func_name);
  fprintf(fp, "%s:\n", func_name);
  TargetInstruction* inst = TargetFirstInstruction(&emitter->bpf->base);
  while (inst != NULL) {
    PrintInstruction(emitter, inst, func_name, fp);
    inst = TargetNext(inst);
  }
  fprintf(fp, ".func_end_%s:\n", func_name);
  fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
}

void BPFPrintCXXAdjustorThunks(FILE* fp) {
  if (compiler->cxx_this_adjustor_thunks.length == 0) {
    return;
  }
  fprintf(fp, "\t.text\n");
  for (size_t i = 0; i < compiler->cxx_this_adjustor_thunks.length; i++) {
    CXXThisAdjustorThunk* thunk = compiler->cxx_this_adjustor_thunks.value.p[i];
    if (thunk == NULL || thunk->thunk == NULL || thunk->target == NULL) {
      continue;
    }
    char thunk_buf[256];
    char target_buf[256];
    const char* thunk_name =
        TargetSymbolName(thunk->thunk, thunk_buf, sizeof(thunk_buf));
    const char* target_name =
        TargetSymbolName(thunk->target, target_buf, sizeof(target_buf));
    EmitFunctionSection(fp, thunk_name);
    fprintf(fp, "\t.weak %s\n", thunk_name);
    fprintf(fp, "\t.type %s, @function\n", thunk_name);
    fprintf(fp, "%s:\n", thunk_name);
    if (thunk->this_adjustment != 0) {
      fprintf(fp, "\tadd64 r1, %d\n", thunk->this_adjustment);
    }
    fprintf(fp, "\tja %s\n", target_name);
    fprintf(fp, ".func_end_%s:\n", thunk_name);
    fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", thunk_name, thunk_name,
            thunk_name);
  }
}
