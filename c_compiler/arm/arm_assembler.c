//
//  arm_assembler.c
//  c_compiler_library
//
//  ARM32 (ARM mode, unified syntax) assembler.
//

#include "arm_assembler.h"
#include "arm_machine.h"
#include "elf.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define ASM assembler->base

typedef enum {
  kARMRegTypeInt = 0,
  kARMRegTypeFloatS = 1,
  kARMRegTypeFloatD = 2,
} ARMRegisterType;

typedef struct {
  ARMRegisterType type;
  int num;
} ARMReg;

typedef enum {
  kARMOpReg,
  kARMOpImm,
  kARMOpMem,
} ARMOpKind;

typedef struct {
  ARMOpKind kind;
  ARMReg reg;
  int32_t imm;
  int base;
  int index;
  int shift_type;  // 0=LSL, 1=LSR, 2=ASR, 3=ROR
  int shift_amount;
  bool writeback;
  bool preindex;
  bool add_offset;
} ARMOp;

static int CompareString(const void* a, const void* b) {
  MapKeyValue* s1 = (MapKeyValue*)a;
  MapKeyValue* s2 = (MapKeyValue*)b;
  return strcmp(s1->key.p, s2->key.p);
}

static uint32_t EncodeImmRotate(int32_t value, uint32_t* encoded) {
  uint32_t v = (uint32_t)value;
  if (v <= 0xff) {
    *encoded = v;
    return 1;
  }
  for (int rot = 1; rot < 16; rot++) {
    uint32_t rotated = (v >> (2 * rot)) | (v << (32 - 2 * rot));
    if (rotated <= 0xff) {
      *encoded = rotated | ((uint32_t)rot << 7);
      return 1;
    }
  }
  return 0;
}

static uint32_t EncodeDataProcReg(int cond, int opcode, int s, int rn, int rd,
                                  int rm, int shift, int shift_imm) {
  uint32_t inst = ARM_COND(cond) | (opcode << 21) | (rn << 16) | (rd << 12) |
                  (shift_imm << 7) | (shift << 5) | rm;
  if (s) {
    inst |= 1 << 20;
  }
  return inst;
}

static uint32_t EncodeDataProcImm(int cond, int opcode, int s, int rn, int rd,
                                  int32_t imm) {
  uint32_t enc = 0;
  if (!EncodeImmRotate(imm, &enc)) {
    return 0;
  }
  return ARM_COND(cond) | (1 << 25) | (opcode << 21) | (s << 20) | (rn << 16) |
         (rd << 12) | enc;
}

static uint32_t EncodeMul(int cond, int rd, int rn, int rs, int rm) {
  return ARM_COND(cond) | (0x9 << 4) | (rd << 16) | (rn << 12) | (rs << 8) | rm;
}

static uint32_t EncodeLongMul(int cond, int rd_lo, int rd_hi, int rm, int rs,
                              bool signed_mul) {
  return ARM_COND(cond) | (signed_mul ? 0 : (1 << 22)) | (0x9 << 4) |
         (rd_hi << 16) | (rd_lo << 12) | (rs << 8) | rm | (1 << 21);
}

static uint32_t EncodeLoadStore(int cond, int p, int u, int b, int w, int l,
                                int rn, int rd, int32_t offset) {
  return ARM_COND(cond) | (p << 24) | (u << 23) | (b << 22) | (w << 21) |
         (l << 20) | (rn << 16) | (rd << 12) | (offset & 0xfff);
}

static uint32_t EncodeBranch(int cond, int link, int32_t offset_words) {
  return ARM_COND(cond) | (0x5 << 25) | (link << 24) |
         (offset_words & 0xffffff);
}

static uint32_t EncodeBranchExchange(int cond, int rm, bool link) {
  return ARM_COND(cond) | (0x12ff1 << 4) | rm | (link ? (1 << 21) : 0);
}

static AssemblerSymbol* GetOrCreateSymbol(ARMAssembler* assembler,
                                          const char* name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(name, ASM.current_section, SYM_TYPE(none),
                             SYM_BIND(local),
                             AssemblerCurrentAddress(&ASM));
  }
  return sym;
}

static void EmitInst(ARMAssembler* assembler, uint32_t inst) {
  AssemblerEmitWord(&ASM, ASM.current_section, (int32_t)inst);
}

static bool ExpectComma(ARMAssembler* assembler) {
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Expected comma");
    return false;
  }
  return true;
}

static bool ParseIntRegName(const char* name, ARMReg* reg) {
  if (strcmp(name, "sp") == 0) {
    reg->type = kARMRegTypeInt;
    reg->num = ARM_SP_REG;
    return true;
  }
  if (strcmp(name, "lr") == 0) {
    reg->type = kARMRegTypeInt;
    reg->num = ARM_LR_REG;
    return true;
  }
  if (strcmp(name, "pc") == 0) {
    reg->type = kARMRegTypeInt;
    reg->num = ARM_PC_REG;
    return true;
  }
  if (strcmp(name, "fp") == 0) {
    reg->type = kARMRegTypeInt;
    reg->num = ARM_FP_REG;
    return true;
  }
  if (strcmp(name, "ip") == 0) {
    reg->type = kARMRegTypeInt;
    reg->num = ARM_IP_REG;
    return true;
  }
  if (name[0] == 'r' && isdigit((unsigned char)name[1])) {
    reg->type = kARMRegTypeInt;
    reg->num = (int)strtol(name + 1, NULL, 10);
    return reg->num >= 0 && reg->num <= 15;
  }
  if (name[0] == 's' && isdigit((unsigned char)name[1])) {
    reg->type = kARMRegTypeFloatS;
    reg->num = (int)strtol(name + 1, NULL, 10);
    return reg->num >= 0 && reg->num < ARM_NUM_FLOAT_REGS;
  }
  if (name[0] == 'd' && isdigit((unsigned char)name[1])) {
    reg->type = kARMRegTypeFloatD;
    reg->num = (int)strtol(name + 1, NULL, 10);
    return reg->num >= 0 && reg->num < 16;
  }
  return false;
}

static bool ParseRegister(ARMAssembler* assembler, ARMReg* reg) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    if (ParseIntRegName(ASM.lex.spelling.value, reg)) {
      LexNextToken(&ASM.lex);
      return true;
    }
  }
  AssemblerError(&ASM, "Register expected");
  return false;
}

static bool ParseOperand(ARMAssembler* assembler, ARMOp* op) {
  memset(op, 0, sizeof(*op));
  LexMatch(&ASM.lex, TOK(hash));
  if (LexLookingAt(&ASM.lex, TOK(number)) || LexLookingAt(&ASM.lex, TOK(minus)) ||
      LexLookingAt(&ASM.lex, TOK(plus))) {
    op->kind = kARMOpImm;
    op->imm = (int32_t)AssemblerEvaluateExpression(&ASM);
    return true;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    if (ParseIntRegName(ASM.lex.spelling.value, &op->reg)) {
      LexNextToken(&ASM.lex);
      op->kind = kARMOpReg;
      return true;
    }
    op->kind = kARMOpMem;
    op->base = -1;
    op->add_offset = true;
    op->preindex = false;
    op->imm = (int32_t)AssemblerEvaluateExpression(&ASM);
    if (LexMatch(&ASM.lex, TOK(lparen))) {
      ARMReg base;
      if (!ParseRegister(assembler, &base)) {
        return false;
      }
      op->base = base.num;
      if (LexMatch(&ASM.lex, TOK(comma))) {
        if (LexLookingAt(&ASM.lex, TOK(number)) ||
            LexLookingAt(&ASM.lex, TOK(minus))) {
          op->imm = (int32_t)AssemblerEvaluateExpression(&ASM);
        } else {
          ARMReg idx;
          if (!ParseRegister(assembler, &idx)) {
            return false;
          }
          op->index = idx.num;
        }
      }
      if (!LexMatch(&ASM.lex, TOK(rparen))) {
        AssemblerError(&ASM, "Expected )");
        return false;
      }
      if (LexMatch(&ASM.lex, TOK(bang))) {
        op->writeback = true;
        op->preindex = true;
      }
      return true;
    }
    return true;
  }
  if (LexMatch(&ASM.lex, TOK(lsquare))) {
    op->kind = kARMOpMem;
    ARMReg base;
    if (!ParseRegister(assembler, &base)) {
      return false;
    }
    op->base = base.num;
    op->add_offset = true;
    if (LexMatch(&ASM.lex, TOK(comma))) {
      if (LexLookingAt(&ASM.lex, TOK(number)) ||
          LexLookingAt(&ASM.lex, TOK(minus)) || LexLookingAt(&ASM.lex, TOK(plus))) {
        op->imm = (int32_t)AssemblerEvaluateExpression(&ASM);
      } else {
        ARMReg idx;
        if (!ParseRegister(assembler, &idx)) {
          return false;
        }
        op->index = idx.num;
        if (LexMatch(&ASM.lex, TOK(comma))) {
          if (!LexMatch(&ASM.lex, TOK(identifier)) ||
              strcmp(ASM.lex.spelling.value, "lsl") != 0) {
            AssemblerError(&ASM, "Expected lsl");
            return false;
          }
          LexNextToken(&ASM.lex);
          if (!LexMatch(&ASM.lex, TOK(hash))) {
            AssemblerError(&ASM, "Expected #");
            return false;
          }
          op->shift_type = 0;
          op->shift_amount = (int)AssemblerEvaluateExpression(&ASM);
        }
      }
      if (LexMatch(&ASM.lex, TOK(bang))) {
        op->writeback = true;
        op->preindex = true;
      }
    }
    if (!LexMatch(&ASM.lex, TOK(rsquare))) {
      AssemblerError(&ASM, "Expected ]");
      return false;
    }
    return true;
  }
  AssemblerError(&ASM, "Expected operand");
  return false;
}

static bool ParseRegPair(ARMAssembler* assembler, ARMReg* dst, ARMOp* src) {
  if (!ParseRegister(assembler, dst)) {
    return false;
  }
  if (!ExpectComma(assembler)) {
    return false;
  }
  return ParseOperand(assembler, src);
}

static void AssembleDataProc(ARMAssembler* assembler, int opcode, bool set_flags) {
  ARMReg dst;
  ARMOp src;
  if (!ParseRegPair(assembler, &dst, &src)) {
    return;
  }
  if (src.kind == kARMOpReg) {
    int rn = (opcode == 0xd || opcode == 0xe) ? 0 : dst.num;
    EmitInst(assembler, EncodeDataProcReg(ARM_COND_AL, opcode, set_flags ? 1 : 0,
                                          rn, dst.num, src.reg.num, 0, 0));
  } else if (src.kind == kARMOpImm) {
    int rn = (opcode == 0xd || opcode == 0xe) ? 0 : dst.num;
    uint32_t inst =
        EncodeDataProcImm(ARM_COND_AL, opcode, set_flags ? 1 : 0, rn,
                          dst.num, src.imm);
    if (inst == 0) {
      AssemblerError(&ASM, "Immediate out of range");
      return;
    }
    EmitInst(assembler, inst);
  }
}

static void AssembleDataProc3(ARMAssembler* assembler, int opcode,
                              bool set_flags) {
  ARMReg dst;
  if (!ParseRegister(assembler, &dst)) {
    return;
  }
  if (!ExpectComma(assembler)) {
    return;
  }
  ARMReg rn;
  if (!ParseRegister(assembler, &rn)) {
    return;
  }
  if (!ExpectComma(assembler)) {
    return;
  }
  ARMOp src;
  if (!ParseOperand(assembler, &src)) {
    return;
  }
  if (src.kind == kARMOpReg) {
    EmitInst(assembler,
             EncodeDataProcReg(ARM_COND_AL, opcode, set_flags ? 1 : 0, rn.num,
                               dst.num, src.reg.num, 0, 0));
  } else if (src.kind == kARMOpImm) {
    uint32_t inst =
        EncodeDataProcImm(ARM_COND_AL, opcode, set_flags ? 1 : 0, rn.num,
                          dst.num, src.imm);
    if (inst == 0) {
      AssemblerError(&ASM, "Immediate out of range");
      return;
    }
    EmitInst(assembler, inst);
  }
}

static void Assemble_mov(ARMAssembler* assembler) { AssembleDataProc(assembler, 0xd, false); }
static void Assemble_mvn(ARMAssembler* assembler) { AssembleDataProc(assembler, 0xe, false); }
static void Assemble_neg(ARMAssembler* assembler) {
  ARMReg dst;
  ARMOp zero;
  if (!ParseRegister(assembler, &dst)) {
    return;
  }
  zero.kind = kARMOpImm;
  zero.imm = 0;
  uint32_t inst = EncodeDataProcImm(ARM_COND_AL, 0x3, 0, dst.num, dst.num, 0);
  EmitInst(assembler, inst);
}
static void Assemble_add(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x4, false); }
static void Assemble_sub(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x2, false); }
static void Assemble_rsb(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x3, false); }
static void Assemble_adc(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x5, false); }
static void Assemble_sbc(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x6, false); }
static void Assemble_and(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x0, false); }
static void Assemble_orr(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0xc, false); }
static void Assemble_eor(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x1, false); }
static void Assemble_bic(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0xc, false); }
static void Assemble_cmp(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0xa, true); }
static void Assemble_cmn(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0xb, true); }
static void Assemble_tst(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x8, true); }
static void Assemble_teq(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x9, true); }

static void Assemble_mul(ARMAssembler* assembler) {
  ARMReg dst, rn, rm;
  if (!ParseRegister(assembler, &dst) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rn) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rm)) {
    return;
  }
  EmitInst(assembler, EncodeMul(ARM_COND_AL, dst.num, rn.num, rm.num, rm.num));
}

static void AssembleLoadStore(ARMAssembler* assembler, bool load, bool byte,
                              bool half) {
  ARMReg rd;
  ARMOp addr;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &addr) || addr.kind != kARMOpMem) {
    AssemblerError(&ASM, "Expected register and memory operand");
    return;
  }
  int b = byte ? 1 : half ? 0 : 0;
  int p = addr.preindex || addr.index < 0 ? 1 : 0;
  int u = addr.add_offset ? 1 : 0;
  int w = addr.writeback ? 1 : 0;
  int32_t off = addr.imm;
  if (addr.index >= 0) {
    AssemblerError(&ASM, "Indexed load/store not yet supported in emitter output");
    return;
  }
  if (!p && w) {
    AssemblerError(&ASM, "Post-indexed writeback not supported");
    return;
  }
  uint32_t inst = EncodeLoadStore(ARM_COND_AL, p, u, byte ? 1 : 0, w, load ? 1 : 0,
                                   addr.base, rd.num, off);
  if (half) {
    inst |= 1 << 5;
  }
  EmitInst(assembler, inst);
}

static void Assemble_ldr(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, false, false); }
static void Assemble_str(ARMAssembler* assembler) { AssembleLoadStore(assembler, false, false, false); }
static void Assemble_ldrb(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, true, false); }
static void Assemble_strb(ARMAssembler* assembler) { AssembleLoadStore(assembler, false, true, false); }
static void Assemble_ldrh(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, false, true); }
static void Assemble_strh(ARMAssembler* assembler) { AssembleLoadStore(assembler, false, false, true); }

static void AssembleBranch(ARMAssembler* assembler, int cond, bool link) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String sym;
    StringInit(&sym, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    int32_t here = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerSymbol* symbol = GetOrCreateSymbol(assembler, sym.value);
    int reloc_type = link ? R_ARM_CALL : R_ARM_JUMP24;
    if (ASM.pic && symbol->binding == SYM_BIND(global)) {
      reloc_type = R_ARM_PLT32;
    }
    AssemblerRelocation* reloc = NewAssemblerRelocation(
        symbol, reloc_type, ASM.current_section, here, 0);
    AssemblerAddRelocation(&ASM, reloc);
    EmitInst(assembler, EncodeBranch(cond, link ? 1 : 0, 0));
    StringDestruct(&sym);
    return;
  }
  int32_t target = (int32_t)AssemblerEvaluateExpression(&ASM);
  int32_t here = (int32_t)AssemblerCurrentAddress(&ASM) + 8;
  int32_t off = (target - here) / 4;
  if (off < -0x800000 || off > 0x7fffff) {
    AssemblerError(&ASM, "Branch out of range");
    return;
  }
  EmitInst(assembler, EncodeBranch(cond, link ? 1 : 0, off));
}

static void Assemble_b(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_AL, false); }
static void Assemble_bl(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_AL, true); }
static void Assemble_beq(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_EQ, false); }
static void Assemble_bne(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_NE, false); }
static void Assemble_blt(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_LT, false); }
static void Assemble_ble(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_LE, false); }
static void Assemble_bgt(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_GT, false); }
static void Assemble_bge(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_GE, false); }

static void Assemble_bx(ARMAssembler* assembler) {
  ARMReg rm;
  if (!ParseRegister(assembler, &rm)) {
    return;
  }
  EmitInst(assembler, EncodeBranchExchange(ARM_COND_AL, rm.num, false));
}

static void Assemble_blx(ARMAssembler* assembler) {
  ARMReg rm;
  if (!ParseRegister(assembler, &rm)) {
    return;
  }
  EmitInst(assembler, EncodeBranchExchange(ARM_COND_AL, rm.num, true));
}

static void Assemble_push(ARMAssembler* assembler) {
  if (!LexMatch(&ASM.lex, TOK(lbrace))) {
    AssemblerError(&ASM, "Expected {");
    return;
  }
  uint16_t reglist = 0;
  bool first = true;
  while (!LexMatch(&ASM.lex, TOK(rbrace))) {
    if (!first) {
      if (!ExpectComma(assembler)) {
        return;
      }
    }
    ARMReg reg;
    if (!ParseRegister(assembler, &reg)) {
      return;
    }
    reglist |= (uint16_t)(1 << reg.num);
    first = false;
  }
  // stmfd sp!, {regs...}
  EmitInst(assembler, 0xE92D0000u | reglist);
}

static void Assemble_pop(ARMAssembler* assembler) {
  if (!LexMatch(&ASM.lex, TOK(lbrace))) {
    AssemblerError(&ASM, "Expected {");
    return;
  }
  uint16_t reglist = 0;
  bool first = true;
  while (!LexMatch(&ASM.lex, TOK(rbrace))) {
    if (!first) {
      if (!ExpectComma(assembler)) {
        return;
      }
    }
    ARMReg reg;
    if (!ParseRegister(assembler, &reg)) {
      return;
    }
    reglist |= (uint16_t)(1 << reg.num);
    first = false;
  }
  // ldmfd sp!, {regs...}
  EmitInst(assembler, 0xE8BD0000u | reglist);
}

static void Assemble_nop(ARMAssembler* assembler) {
  (void)assembler;
  EmitInst(assembler, ARM_AL | 0x0320f000);  // mov r0, r0
}

static void Assemble_ret(ARMAssembler* assembler) {
  (void)assembler;
  EmitInst(assembler, EncodeBranchExchange(ARM_COND_AL, ARM_LR_REG, false));
}

static void Assemble_movw(ARMAssembler* assembler) {
  ARMReg rd;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler)) {
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String sym;
    StringInit(&sym, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    int32_t here = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerRelocation* reloc =
        NewAssemblerRelocation(GetOrCreateSymbol(assembler, sym.value),
                               R_ARM_MOVW_ABS_NC, ASM.current_section, here, 0);
    AssemblerAddRelocation(&ASM, reloc);
    EmitInst(assembler, ARM_AL | (0x3 << 23) | (rd.num << 12) | (0 << 4));
    StringDestruct(&sym);
    return;
  }
  int32_t value = (int32_t)AssemblerEvaluateExpression(&ASM);
  uint32_t imm16 = (uint32_t)value & 0xffff;
  uint32_t imm4 = ((uint32_t)value >> 12) & 0xf;
  EmitInst(assembler, ARM_AL | (0x3 << 23) | (imm4 << 16) | (rd.num << 12) |
                        (imm16 & 0xfff) | ((imm16 >> 12) << 4));
}

static void Assemble_movt(ARMAssembler* assembler) {
  ARMReg rd;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler)) {
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String sym;
    StringInit(&sym, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    int32_t here = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerRelocation* reloc =
        NewAssemblerRelocation(GetOrCreateSymbol(assembler, sym.value),
                               R_ARM_MOVT_ABS, ASM.current_section, here, 0);
    AssemblerAddRelocation(&ASM, reloc);
    EmitInst(assembler, ARM_AL | (0x3 << 23) | (1 << 22) | (rd.num << 12) | (0 << 4));
    StringDestruct(&sym);
    return;
  }
  int32_t value = (int32_t)AssemblerEvaluateExpression(&ASM);
  uint32_t imm16 = ((uint32_t)value >> 16) & 0xffff;
  uint32_t imm4 = ((uint32_t)value >> 28) & 0xf;
  EmitInst(assembler, ARM_AL | (0x3 << 23) | (1 << 22) | (imm4 << 16) |
                        (rd.num << 12) | (imm16 & 0xfff) | ((imm16 >> 12) << 4));
}

static void Assemble_vmov_f32(ARMAssembler* assembler) {
  ARMReg dst, src;
  if (!ParseRegister(assembler, &dst) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &src)) {
    return;
  }
  (void)src;
  EmitInst(assembler, 0xeeb00a40 | (dst.num << 12) | (src.num << 0));  // vmov.f32
}

static void Assemble_vldr(ARMAssembler* assembler) {
  ARMReg rd;
  ARMOp addr;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &addr) || addr.kind != kARMOpMem) {
    return;
  }
  int32_t off = addr.imm / 4;
  EmitInst(assembler, 0xed100a00 | (rd.num << 12) | (addr.base << 16) |
                        (off & 0xff));
}

static void Assemble_vstr(ARMAssembler* assembler) {
  ARMReg rd;
  ARMOp addr;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &addr) || addr.kind != kARMOpMem) {
    return;
  }
  int32_t off = addr.imm / 4;
  EmitInst(assembler, 0xed000a00 | (rd.num << 12) | (addr.base << 16) |
                        (off & 0xff));
}

#define DECLARE_INST_FUNC(mnemonic) static void Assemble_##mnemonic(ARMAssembler*)

DECLARE_INST_FUNC(mov);
DECLARE_INST_FUNC(mvn);
DECLARE_INST_FUNC(neg);
DECLARE_INST_FUNC(add);
DECLARE_INST_FUNC(sub);
DECLARE_INST_FUNC(rsb);
DECLARE_INST_FUNC(adc);
DECLARE_INST_FUNC(sbc);
DECLARE_INST_FUNC(and);
DECLARE_INST_FUNC(orr);
DECLARE_INST_FUNC(eor);
DECLARE_INST_FUNC(bic);
DECLARE_INST_FUNC(cmp);
DECLARE_INST_FUNC(cmn);
DECLARE_INST_FUNC(tst);
DECLARE_INST_FUNC(teq);
DECLARE_INST_FUNC(mul);
DECLARE_INST_FUNC(ldr);
DECLARE_INST_FUNC(str);
DECLARE_INST_FUNC(ldrb);
DECLARE_INST_FUNC(strb);
DECLARE_INST_FUNC(ldrh);
DECLARE_INST_FUNC(strh);
DECLARE_INST_FUNC(b);
DECLARE_INST_FUNC(bl);
DECLARE_INST_FUNC(beq);
DECLARE_INST_FUNC(bne);
DECLARE_INST_FUNC(blt);
DECLARE_INST_FUNC(ble);
DECLARE_INST_FUNC(bgt);
DECLARE_INST_FUNC(bge);
DECLARE_INST_FUNC(bx);
DECLARE_INST_FUNC(blx);
DECLARE_INST_FUNC(push);
DECLARE_INST_FUNC(pop);
DECLARE_INST_FUNC(nop);
DECLARE_INST_FUNC(ret);
DECLARE_INST_FUNC(movw);
DECLARE_INST_FUNC(movt);
DECLARE_INST_FUNC(vmov_f32);
DECLARE_INST_FUNC(vldr);
DECLARE_INST_FUNC(vstr);

#undef DECLARE_INST_FUNC

#define INST(mnemonic) \
  do { \
    MapKeyValue kv = {.key.p = #mnemonic, .value.p = Assemble_##mnemonic}; \
    MapInsert(instructions, kv); \
  } while (0)

#define INST2(name, mnemonic) \
  do { \
    MapKeyValue kv = {.key.p = name, .value.p = Assemble_##mnemonic}; \
    MapInsert(instructions, kv); \
  } while (0)

static void InitializeInstructions(Map* instructions) {
  INST(mov);
  INST(mvn);
  INST(neg);
  INST(add);
  INST(sub);
  INST(rsb);
  INST(adc);
  INST(sbc);
  INST(and);
  INST(orr);
  INST(eor);
  INST(bic);
  INST(cmp);
  INST(cmn);
  INST(tst);
  INST(teq);
  INST(mul);
  INST(ldr);
  INST(str);
  INST(ldrb);
  INST(strb);
  INST(ldrh);
  INST(strh);
  INST(b);
  INST(bl);
  INST(beq);
  INST(bne);
  INST(blt);
  INST(ble);
  INST(bgt);
  INST(bge);
  INST(bx);
  INST(blx);
  INST(push);
  INST(pop);
  INST(nop);
  INST(ret);
  INST(movw);
  INST(movt);
  INST2("vmov.f32", vmov_f32);
  INST(vldr);
  INST(vstr);
}

#undef INST
#undef INST2

bool ARMAssemblerInit(ARMAssembler* assembler, String* infile, String* outfile) {
  static int reloc_types[] = {
      R_ARM_ABS32, R_ARM_REL32, R_ARM_CALL, R_ARM_JUMP24, R_ARM_GOT_BREL,
      R_ARM_PLT32, R_ARM_GLOB_DAT, R_ARM_JUMP_SLOT, R_ARM_RELATIVE,
      R_ARM_TARGET1,
  };
  if (!AssemblerInit(&assembler->base, ELF_MACHINE_TYPE_ARM, EF_ARM_EABI_FLAGS,
                     reloc_types, infile, outfile)) {
    return false;
  }
  MapInit(&assembler->instructions, CompareString);
  InitializeInstructions(&assembler->instructions);
  AssemblerAddSection(&assembler->base, NULL, SHT(null), 0, 0);
  assembler->bss =
      AssemblerAddSection(&assembler->base, NewString(".bss"),
                          SHT(nobits), SHF(alloc) | SHF(write), 8);
  return true;
}

ARMAssembler* NewARMAssembler(String* infile, String* outfile) {
  ARMAssembler* assembler = malloc(sizeof(ARMAssembler));
  ARMAssemblerInit(assembler, infile, outfile);
  return assembler;
}

void ARMAssemblerDestruct(ARMAssembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void ARMAssemblerDelete(ARMAssembler* assembler) {
  ARMAssemblerDestruct(assembler);
  free(assembler);
}

void AssembleARMInstruction(Assembler* base, String* word) {
  ARMAssembler* assembler = (ARMAssembler*)base;
  void* asm_func = MapFindPointerKey(&assembler->instructions, word->value);
  if (asm_func != NULL) {
    void (*func)(ARMAssembler*) = asm_func;
    func(assembler);
  } else {
    AssemblerError(&assembler->base, "Syntax error; unknown instruction: %s",
                   word->value);
  }
}
