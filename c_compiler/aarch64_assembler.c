//
//  aarch64_assembler.c
//  c_compiler_library
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reseAARCH64ed.
//

#include "aarch64_assembler.h"
#include "map.h"
#include <assert.h>
#include <ctype.h>


// Shortcut macro avoid typing assembler->base. everywhere we want to access
// the base assembler.
#define ASM assembler->base

#define DECLARE_INST_FUNC(mnemonic) \
  static void Assemble_##mnemonic(AARCH64Assembler*)


DECLARE_INST_FUNC(adc);
DECLARE_INST_FUNC(add);
DECLARE_INST_FUNC(adcs);
DECLARE_INST_FUNC(adds);
DECLARE_INST_FUNC(adr);
DECLARE_INST_FUNC(adrp);
DECLARE_INST_FUNC(cmn);
DECLARE_INST_FUNC(cmp);
DECLARE_INST_FUNC(madd);
DECLARE_INST_FUNC(mneg);
DECLARE_INST_FUNC(msub);
DECLARE_INST_FUNC(mul);
DECLARE_INST_FUNC(neg);
DECLARE_INST_FUNC(ngc);
DECLARE_INST_FUNC(sbc);
DECLARE_INST_FUNC(negs);
DECLARE_INST_FUNC(ngcs);
DECLARE_INST_FUNC(sbcs);
DECLARE_INST_FUNC(sdiv);
DECLARE_INST_FUNC(smaddl);
DECLARE_INST_FUNC(smnegl);
DECLARE_INST_FUNC(smsubl);
DECLARE_INST_FUNC(smulh);
DECLARE_INST_FUNC(smull);
DECLARE_INST_FUNC(sub);
DECLARE_INST_FUNC(subs);
DECLARE_INST_FUNC(udiv);
DECLARE_INST_FUNC(umaddl);
DECLARE_INST_FUNC(umnegl);
DECLARE_INST_FUNC(umsubl);
DECLARE_INST_FUNC(umulh);
DECLARE_INST_FUNC(umull);

DECLARE_INST_FUNC(bfi);
DECLARE_INST_FUNC(bfxil);
DECLARE_INST_FUNC(cls);
DECLARE_INST_FUNC(clz);
DECLARE_INST_FUNC(extr);
DECLARE_INST_FUNC(rbit);
DECLARE_INST_FUNC(rev);
DECLARE_INST_FUNC(rev16);
DECLARE_INST_FUNC(rev32);
DECLARE_INST_FUNC(sbfiz);
DECLARE_INST_FUNC(ubfiz);
DECLARE_INST_FUNC(sbfx);
DECLARE_INST_FUNC(ubfx);
DECLARE_INST_FUNC(sbxt);
DECLARE_INST_FUNC(sbxtb);
DECLARE_INST_FUNC(sbxth);
DECLARE_INST_FUNC(ubxt);
DECLARE_INST_FUNC(ubxtb);
DECLARE_INST_FUNC(ubxth);
DECLARE_INST_FUNC(sxtb);
DECLARE_INST_FUNC(sxth);
DECLARE_INST_FUNC(sxtw);

DECLARE_INST_FUNC(and);
DECLARE_INST_FUNC(ands);
DECLARE_INST_FUNC(asr);
DECLARE_INST_FUNC(asri);
DECLARE_INST_FUNC(bic);
DECLARE_INST_FUNC(bics);
DECLARE_INST_FUNC(eon);
DECLARE_INST_FUNC(eons);
DECLARE_INST_FUNC(lsl);
DECLARE_INST_FUNC(lsr);
DECLARE_INST_FUNC(mov);
DECLARE_INST_FUNC(movk);
DECLARE_INST_FUNC(movn);
DECLARE_INST_FUNC(movz);
DECLARE_INST_FUNC(mvn);
DECLARE_INST_FUNC(orn);
DECLARE_INST_FUNC(orr);
DECLARE_INST_FUNC(ror);
DECLARE_INST_FUNC(tst);
DECLARE_INST_FUNC(eor);

DECLARE_INST_FUNC(beq);
DECLARE_INST_FUNC(bne);
DECLARE_INST_FUNC(bcs);
DECLARE_INST_FUNC(bhs);
DECLARE_INST_FUNC(bcc);
DECLARE_INST_FUNC(blo);
DECLARE_INST_FUNC(bmi);
DECLARE_INST_FUNC(bpl);
DECLARE_INST_FUNC(bvs);
DECLARE_INST_FUNC(bvc);
DECLARE_INST_FUNC(bhi);
DECLARE_INST_FUNC(bls);
DECLARE_INST_FUNC(bge);
DECLARE_INST_FUNC(blt);
DECLARE_INST_FUNC(bgt);
DECLARE_INST_FUNC(ble);
DECLARE_INST_FUNC(bal);
DECLARE_INST_FUNC(b);

DECLARE_INST_FUNC(bceq);
DECLARE_INST_FUNC(bcne);
DECLARE_INST_FUNC(bccs);
DECLARE_INST_FUNC(bchs);
DECLARE_INST_FUNC(bccc);
DECLARE_INST_FUNC(bclo);
DECLARE_INST_FUNC(bcmi);
DECLARE_INST_FUNC(bcpl);
DECLARE_INST_FUNC(bcvs);
DECLARE_INST_FUNC(bcvc);
DECLARE_INST_FUNC(bchi);
DECLARE_INST_FUNC(bcls);
DECLARE_INST_FUNC(bcge);
DECLARE_INST_FUNC(bclt);
DECLARE_INST_FUNC(bcgt);
DECLARE_INST_FUNC(bcle);
DECLARE_INST_FUNC(bcal);

DECLARE_INST_FUNC(bl);
DECLARE_INST_FUNC(blr);
DECLARE_INST_FUNC(br);
DECLARE_INST_FUNC(cbnz);
DECLARE_INST_FUNC(cbz);
DECLARE_INST_FUNC(tbnz);
DECLARE_INST_FUNC(tbz);


DECLARE_INST_FUNC(ccmn);
DECLARE_INST_FUNC(ccmni);
DECLARE_INST_FUNC(ccmp);
DECLARE_INST_FUNC(ccmpi);
DECLARE_INST_FUNC(cinc);
DECLARE_INST_FUNC(cinv);
DECLARE_INST_FUNC(cneg);
DECLARE_INST_FUNC(csel);
DECLARE_INST_FUNC(cset);
DECLARE_INST_FUNC(csetm);
DECLARE_INST_FUNC(csinc);
DECLARE_INST_FUNC(csinv);
DECLARE_INST_FUNC(csneg);

DECLARE_INST_FUNC(ldp);
DECLARE_INST_FUNC(ldpsw);
DECLARE_INST_FUNC(ldr);
DECLARE_INST_FUNC(ldur);
DECLARE_INST_FUNC(ldrb);
DECLARE_INST_FUNC(ldrh);
DECLARE_INST_FUNC(ldurb);
DECLARE_INST_FUNC(ldurh);
DECLARE_INST_FUNC(ldrsb);
DECLARE_INST_FUNC(ldrsh);
DECLARE_INST_FUNC(ldursb);
DECLARE_INST_FUNC(ldursh);
DECLARE_INST_FUNC(ldursw);
DECLARE_INST_FUNC(prfm);
DECLARE_INST_FUNC(stp);
DECLARE_INST_FUNC(str);
DECLARE_INST_FUNC(stur);
DECLARE_INST_FUNC(strb);
DECLARE_INST_FUNC(strh);
DECLARE_INST_FUNC(sturb);
DECLARE_INST_FUNC(sturh);

DECLARE_INST_FUNC(fldr);
DECLARE_INST_FUNC(fstr);
DECLARE_INST_FUNC(fadd);
DECLARE_INST_FUNC(fsub);
DECLARE_INST_FUNC(fmul);
DECLARE_INST_FUNC(fdiv);
DECLARE_INST_FUNC(fsqrt);
DECLARE_INST_FUNC(fmin);
DECLARE_INST_FUNC(fmax);
DECLARE_INST_FUNC(fcvtsd);     // Single to double
DECLARE_INST_FUNC(fcvtds);     // Double to single.
DECLARE_INST_FUNC(fcvtns);
DECLARE_INST_FUNC(fcvtnu);
DECLARE_INST_FUNC(fcvt);     // Copy from int reg (no conversion)
DECLARE_INST_FUNC(fmov);
DECLARE_INST_FUNC(fcmp);
DECLARE_INST_FUNC(scvtf);
DECLARE_INST_FUNC(ucvtf);
DECLARE_INST_FUNC(fneg);
DECLARE_INST_FUNC(ret);

#undef DECLARE_INST_FUNC

#define INST(mnemonic) \
do {\
MapKeyValue kv;\
kv.key.p = #mnemonic;\
kv.value.p = Assemble_##mnemonic;\
MapInsert(instructions, kv);\
} while(0)

#define INST2(mnemonic, inst) \
  do {\
    MapKeyValue kv;\
    kv.key.p = #inst;\
    kv.value.p = Assemble_##mnemonic;\
    MapInsert(instructions, kv);\
  } while(0)

// Add all instructions to the handler map.  This maps the instruction
// spelling to a handler function.
static void InitializeInstructions(Map* instructions) {
  INST(adc);
  INST(add);
  INST(adcs);
  INST(adds);
  INST(adr);
  INST(adrp);
  INST(cmn);
  INST(cmp);
  INST(madd);
  INST(mneg);
  INST(msub);
  INST(mul);
  INST(neg);
  INST(ngc);
  INST(sbc);
  INST(negs);
  INST(ngcs);
  INST(sbcs);
  INST(sdiv);
  INST(smaddl);
  INST(smnegl);
  INST(smsubl);
  INST(smulh);
  INST(smull);
  INST(sub);
  INST(subs);
  INST(udiv);
  INST(umaddl);
  INST(umnegl);
  INST(umsubl);
  INST(umulh);
  INST(umull);

  INST(bfi);
  INST(bfxil);
  INST(cls);
  INST(clz);
  INST(extr);
  INST(rbit);
  INST(rev);
  INST(rev16);
  INST(rev32);
  INST(sbfiz);
  INST(ubfiz);
  INST(sbfx);
  INST(ubfx);
  INST(sbxt);
  INST(sbxtb);
  INST(sbxth);
  INST(ubxt);
  INST(ubxtb);
  INST(ubxth);
  INST(sxtb);
  INST(sxth);
  INST(sxtw);

  INST(and);
  INST(ands);
  INST(asr);
  INST(asri);
  INST(bic);
  INST(bics);
  INST(eon);
  INST(eons);
  INST(lsl);
  INST(lsr);
  INST(mov);
  INST(movk);
  INST(movn);
  INST(movz);
  INST(mvn);
  INST(orn);
  INST(orr);
  INST(ror);
  INST(tst);
  INST(eor);

  INST2(beq, b.eq);
  INST2(bne, b.ne);
  INST2(bcs, b.cs);
  INST2(bhs, b.hs);
  INST2(bcc, b.cc);
  INST2(blo, b.lo);
  INST2(bmi, b.mi);
  INST2(bpl, b.pl);
  INST2(bvs, b.vs);
  INST2(bvc, b.vc);
  INST2(bhi, b.hi);
  INST2(bls, b.hs);
  INST2(bge, b.ge);
  INST2(blt, b.lt);
  INST2(bgt, b.gt);
  INST2(ble, b.le);
  INST2(bal, b.al);
  
  INST2(bceq, bc.eq);
  INST2(bcne, bc.ne);
  INST2(bccs, bc.cs);
  INST2(bchs, bc.hs);
  INST2(bccc, bc.cc);
  INST2(bclo, bc.lo);
  INST2(bcmi, bc.mi);
  INST2(bcpl, bc.pl);
  INST2(bcvs, bc.vs);
  INST2(bcvc, bc.vc);
  INST2(bchi, bc.hi);
  INST2(bcls, bc.hs);
  INST2(bcge, bc.ge);
  INST2(bclt, bc.lt);
  INST2(bcgt, bc.gt);
  INST2(bcle, bc.le);
  INST2(bcal, bc.al);
  
  INST(b);
  
  INST(bl);
  INST(blr);
  INST(br);
  INST(cbnz);
  INST(cbz);
  INST(tbnz);
  INST(tbz);


  INST(ccmn);
  INST(ccmni);
  INST(ccmp);
  INST(ccmpi);
  INST(cinc);
  INST(cinv);
  INST(cneg);
  INST(csel);
  INST(cset);
  INST(csetm);
  INST(csinc);
  INST(csinv);
  INST(csneg);

  INST(ldp);
  INST(ldpsw);
  INST(ldr);
  INST(ldur);
  INST(ldrb);
  INST(ldrh);
  INST(ldurb);
  INST(ldurh);
  INST(ldrsb);
  INST(ldrsh);
  INST(ldursb);
  INST(ldursh);
  INST(ldursw);
  INST(prfm);
  INST(stp);
  INST(str);
  INST(stur);
  INST(strb);
  INST(strh);
  INST(sturb);
  INST(sturh);

  INST(fldr);
  INST(fstr);
  INST(fadd);
  INST(fsub);
  INST(fmul);
  INST(fdiv);
  INST(fsqrt);
  INST(fmin);
  INST(fmax);
  INST(fcvtsd);     // Single to double
  INST(fcvtds);     // Double to single.
  INST(fcvtns);
  INST(fcvtnu);
  INST(fcvt);     // Copy from int reg (no conversion)
  INST(fmov);
  INST(fcmp);
  INST(scvtf);
  INST(ucvtf);
  INST(fneg);
  INST(ret);
}

#undef INST

#define UNDEFINED_INST(m)                                     \
  static void Assemble_##m(AARCH64Assembler* assembler) {          \
    AssemblerError(&ASM, "Unimplemented instruction %s", #m); \
  }

typedef enum {
  kBAD,
  kW,
  kX,
  kS,
  kD,
  kB,
  kH,
  kQ,
} RegisterWidth;

typedef struct {
  int num;
  RegisterWidth width;
  bool fp_or_simd;
  int size;     // Size field for load/store.
} Register;

typedef enum {
  kLSL,
  kLSR,
  kASR,
} ShiftType;

typedef enum {
  kUnknown,
  kRegister,
  kIntImmediate,
  kFloatImmediate,
} OpType;

typedef struct {
  ShiftType type;
  int amount;
} Shift;

typedef struct {
  OpType type;
  union {
    Register reg;
    int32_t i;
    double f;
  };
  Shift shift;
} Operand;

// Conditions, matching AArch64 condition encoding.
typedef enum {
  kCond_eq = 0,
  kCond_ne = 1,
  kCond_cs = 2,
  kCond_hs = kCond_cs,
  kCond_cc = 3,
  kCond_lo = kCond_cc,
  kCond_mi = 4,
  kCond_pl = 5,
  kCond_vs = 6,
  kCond_vc = 7,
  kCond_hi = 8,
  kCond_ls = 9,
  kCond_ge = 10,
  kCond_lt = 11,
  kCond_gt = 12,
  kCond_le = 13,
  kCond_al = 14,
} Condition;

static Register GetRegister(AARCH64Assembler* assembler) {
  Register reg = {.width = kBAD};
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Expected register name");
    return reg;
  }
  if (StringEqualCaseBlind(&ASM.lex.spelling, "sp")) {
    LexNextToken(&ASM.lex);
    reg.width = kX;
    reg.num = AARCH64_SP_REG;
    return reg;
  }
  char prefix = toupper(ASM.lex.spelling.value[0]);
  switch (prefix) {
    case 'W':
      reg.width = kW;
      reg.fp_or_simd = false;
      reg.size = 2;
      break;
    case 'X':
      reg.width = kX;
      reg.fp_or_simd = false;
      reg.size = 3;
      break;
    case 'S':
      reg.width = kS;
      reg.fp_or_simd = true;
      reg.size = 2;
      break;
    case 'D':
      reg.width = kD;
      reg.fp_or_simd = true;
      reg.size = 3;
      break;
    case 'B':
      reg.width = kB;
      reg.fp_or_simd = true;
      reg.size = 0;
       break;
    case 'H':
      reg.width = kH;
      reg.fp_or_simd = true;
      reg.size = 1;
      break;
    case 'Q':
      reg.width = kQ;
      reg.fp_or_simd = true;
      reg.size = 3;
      break;
   default:
      AssemblerError(&ASM, "Invalid register name %s", ASM.lex.spelling.value);
      break;
  }
  int i = 1;
  while (i < ASM.lex.spelling.length && isdigit(ASM.lex.spelling.value[i])) {
    reg.num = reg.num * 10 + ASM.lex.spelling.value[i] - '0';
    i++;
  }
  LexNextToken(&ASM.lex);
  return reg;
}

static void CheckImmediateWidth(AARCH64Assembler* assembler, int immed, int num_bits) {
  int i = immed;
  if (i < 0) {
    i = -i;
  }
  int mask = (1 << num_bits) - 1;
  i &= ~mask;
  if (i != 0) {
    AssemblerError(&ASM, "Immediate value 0x%x doesn't fit in %d bits", immed, num_bits);
  }
}

static char* RegisterName(Register* reg, char* buf, size_t buflen) {
  switch (reg->width) {
    case kX:
      buf[0] = 'x';
      break;
    case kW:
      buf[0] = 'w';
      break;
    case kS:
      buf[0] = 's';
      break;
   case kD:
      buf[0] = 'd';
      break;
    default:
      buf[0] = 'r';
      break;
  }
  snprintf(&buf[1], buflen - 1, "%d", reg->num);
  return buf;
}

static Register ZeroReg(int width) {
  Register r = {.width = width, .num = 31};
  return r;
}

static Operand GetOperand(AARCH64Assembler* assembler) {
  Operand op = {.type = kUnknown};
  bool negative_immed = false;
  // # is optional but encouraged.
  if (LexLookingAt(&ASM.lex, TOK(number))) {
    op.type = kIntImmediate;
  } else if (LexLookingAt(&ASM.lex, TOK(fnumber))) {
    op.type = kFloatImmediate;
  } else if (LexLookingAt(&ASM.lex, TOK(hash))) {
    LexNextToken(&ASM.lex);
    
    if (LexMatch(&ASM.lex, TOK(minus))) {
      negative_immed = true;
    }
    if (LexLookingAt(&ASM.lex, TOK(number))) {
      op.type = kIntImmediate;
    } else if (LexLookingAt(&ASM.lex, TOK(fnumber))) {
      op.type = kFloatImmediate;
   } else {
     AssemblerError(&ASM, "Expected immediate value after #");
    }
  } else {
    op.type = kRegister;
  }
  
  assert(op.type != kUnknown);
  switch (op.type) {
    case kRegister: {
      op.reg = GetRegister(assembler);
      break;
    }
    case kFloatImmediate:
      op.f = (int)AssemblerEvaluateExpression(&ASM);
      if (negative_immed) {
        op.f = -op.f;
      }
     // TODO: check for valid constants.
      LexNextToken(&ASM.lex);
      break;
    case kIntImmediate:
      op.i = (int)AssemblerEvaluateExpression(&ASM);
      if (negative_immed) {
        op.i = -op.i;
      }
      break;
    case kUnknown:
      abort();
      break;
  }
  return op;
}

static bool GetShift(AARCH64Assembler* assembler, Operand* op, int max_shift) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Shift name expected");
    return false;
  }
  if (StringEqualCaseBlind(&ASM.lex.spelling, "LSL")) {
    op->shift.type = kLSL;
  } else if (StringEqualCaseBlind(&ASM.lex.spelling, "LSR")) {
    op->shift.type = kLSR;
  } else if (StringEqualCaseBlind(&ASM.lex.spelling, "ASL")) {
    op->shift.type = kASR;
  } else {
    AssemblerError(&ASM, "Invalid shift %s", ASM.lex.spelling.value);
    return false;
  }
  LexMatch(&ASM.lex, TOK(hash));      // Optional.
  op->shift.amount = (int)AssemblerEvaluateExpression(&ASM);
  if (op->shift.amount < 0 || op->shift.amount > max_shift) {
    AssemblerError(&ASM, "Invalid shift amount %d", op->shift.amount);
    return false;
  }
  return true;
}

static bool NeedComma(AARCH64Assembler* assembler) {
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Expected comma after operand");
    return false;
  }
  return true;
}

static AssemblerSymbol* GetOrCreateSymbol(AARCH64Assembler* assembler,
                                          const char* symbol_name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, symbol_name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(symbol_name, ASM.current_section, SYM_TYPE(func),
                             SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  return sym;
}


static bool CheckRegWidths(AARCH64Assembler* assembler, Register* r1, Register* r2) {
  if (r1->width != r2->width) {
    char buf1[16];
    char buf2[16];
    AssemblerError(&ASM, "Registers %s and %s mismatch in width",
                   RegisterName(r1, buf1, sizeof(buf1)),
                   RegisterName(r2, buf2, sizeof(buf2)));
    return false;
  }
  return true;
}

static bool AssemblerFunction(AARCH64Assembler* assembler, String* func,
                              String* symbol) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    StringSet(func, ASM.lex.spelling.value);  // Already initialized.
    LexNextToken(&ASM.lex);
    if (LexMatch(&ASM.lex, TOK(lparen))) {
      if (LexLookingAt(&ASM.lex, TOK(identifier))) {
        // Symbol name.
        StringSet(symbol, ASM.lex.spelling.value);
        LexNextToken(&ASM.lex);
        if (!LexMatch(&ASM.lex, TOK(rparen))) {
          AssemblerError(&ASM, "Syntax error in assembler function: missing )");
          return false;
        }
      } else {
        AssemblerError(
            &ASM, "Syntax error in assembler function: missing symbol name");
        return false;
      }
    } else {
      AssemblerError(&ASM, "Syntax error in assembler function: missing (");
      return false;
    }
  } else {
    AssemblerError(&ASM, "Syntax error in assembler function: missing name");
    return false;
  }
  return true;
}

static void AssembleAddSubImmediate(AARCH64Assembler* assembler,
                                    Register* rd, Register* rn,
                                    int immed, int sf, int op, int s) {
  CheckImmediateWidth(assembler, immed, 12);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (op << 30) | (s << 29) | (0x22 << 23) |
                    ((immed & 0xfff) << 10 | (rn->num << 5) | (rd->num)));
}

static void AssembleAddSubShiftedRegister(AARCH64Assembler* assembler,
                                          Register* rd, Register* rn,
                                          Operand* op2, int sf, int op, int s) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (op << 30) | (s << 29) | (0xb << 24) |
                    (op2->shift.type << 22) | (op2->reg.num << 16) |
                    (op2->shift.amount << 10) | (rn->num << 5) | (rd->num));
}

static void AssembleAddSub(AARCH64Assembler* assembler, int opcode, int s, bool one_operand) {
  Register rd;
  if (!one_operand) {
    rd = GetRegister(assembler);
    if (!NeedComma(assembler)) {
      return;
    }
  }
  
  Register rn = GetRegister(assembler);
  if (!NeedComma(assembler)) {
    return;
  }
  if (one_operand) {
    rd = ZeroReg(rn.width);
  } else if (!CheckRegWidths(assembler, &rd, &rn)) {
    return;
  }
  Operand src2 = GetOperand(assembler);
  switch (src2.type) {
    case kRegister:
      if (!CheckRegWidths(assembler, &rd, &src2.reg)) {
        return;
      }
      if (LexMatch(&ASM.lex, TOK(comma))) {
        if (!GetShift(assembler, &src2, rd.width == kX ? 63 : 31)) {
          return;
        }
      }
      AssembleAddSubShiftedRegister(assembler, &rd, &rn, &src2, rd.width == kX, opcode, s);
      break;
    case kIntImmediate:
      AssembleAddSubImmediate(assembler, &rd, &rn, src2.i, rd.width == kX, opcode, s);
      break;
   case kFloatImmediate:
      AssemblerError(&ASM, "Unexpected floating point immediate");
     break;
    case kUnknown:
      return;
  }
}

static void Assemble_add(AARCH64Assembler* assembler) {
  AssembleAddSub(assembler, 0, 0, /*one_operand=*/false);
}

static void Assemble_adds(AARCH64Assembler* assembler) {
  AssembleAddSub(assembler, 0, 1, /*one_operand=*/false);
}

static void Assemble_sub(AARCH64Assembler* assembler) {
  AssembleAddSub(assembler, 1, 0, /*one_operand=*/false);
}

static void Assemble_subs(AARCH64Assembler* assembler) {
  AssembleAddSub(assembler, 1, 1, /*one_operand=*/false);
}

// Encoded as SUBS with zero as the rd.
static void Assemble_cmp(AARCH64Assembler* assembler) {
  AssembleAddSub(assembler, 1, 1, /*one_operand=*/true);
}

// Encoded as ADDS with zero as the rd.
static void Assemble_cmn(AARCH64Assembler* assembler) {
  AssembleAddSub(assembler, 0, 1, /*one_operand=*/true);
}


static void AssembleAddSubWithCarry(AARCH64Assembler* assembler, int op, int s) {
  Register rd = GetRegister(assembler);
  if (!NeedComma(assembler)) {
    return;
  }
  
  Register rn = GetRegister(assembler);
  if (!NeedComma(assembler)) {
    return;
  }
  if (!CheckRegWidths(assembler, &rd, &rn)) {
    return;
  }
  Register rm = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  
  int sf = rd.width == kX;
  
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (op << 30) | (s << 29) | (0xd0 << 21) |
                    (rm.num << 16) | (rn.num << 5) | (rd.num));
}

static void AssembleLogicalImmediate(AARCH64Assembler* assembler,
                                    Register* rd, Register* rn,
                                    int immed, int sf, int opc) {
  CheckImmediateWidth(assembler, immed, 13);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (opc << 29) | (0x24 << 23) |
                    (immed << 10 | (rn->num << 5) | (rd->num)));
}


static void AssembleLogicalShiftedRegister(AARCH64Assembler* assembler,
                                          Register* rd, Register* rn,
                                          Operand* op2, int sf, int opc, int n) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (opc << 29) | (0xa << 24) | (n << 21) |
                    (op2->shift.type << 22) | (op2->reg.num << 16) |
                    (op2->shift.amount << 10) | (rn->num << 5) | (rd->num));
}

static void AssembleLogical(AARCH64Assembler* assembler, int opcode, int n, bool immed_ok, bool one_operand) {
  Register rd;
  if (!one_operand) {
    rd = GetRegister(assembler);
    if (!NeedComma(assembler)) {
      return;
    }
  }
  
  Register rn = GetRegister(assembler);
  if (!NeedComma(assembler)) {
    return;
  }
  if (one_operand) {
    rd = ZeroReg(rn.width);
  } else if (!CheckRegWidths(assembler, &rd, &rn)) {
    return;
  }
  Operand src2 = GetOperand(assembler);
  switch (src2.type) {
    case kRegister:
      if (!CheckRegWidths(assembler, &rd, &src2.reg)) {
        return;
      }
      if (LexMatch(&ASM.lex, TOK(comma))) {
        if (!GetShift(assembler, &src2, rd.width == kX ? 63 : 31)) {
          return;
        }
      }
      AssembleLogicalShiftedRegister(assembler, &rd, &rn, &src2, rd.width == kX, opcode, n);
      break;
    case kIntImmediate:
      if (!immed_ok) {
        AssemblerError(&ASM, "Immediate operand not valid for this instruction");
      }
      AssembleLogicalImmediate(assembler, &rd, &rn, src2.i, rd.width == kX, opcode);
      break;
   case kFloatImmediate:
      AssemblerError(&ASM, "Unexpected floating point immediate");
     break;
    case kUnknown:
      return;
  }
}

static void Assemble_and(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 0, 0, /*immed_ok=*/true, /*one_operand=*/false);
}

static void Assemble_bic(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 0, 1, /*immed_ok=*/false, /*one_operand=*/false);
}

static void Assemble_orr(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 1, 0, /*immed_ok=*/true, /*one_operand=*/false);
}

static void Assemble_orn(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 1, 1, /*immed_ok=*/false, /*one_operand=*/false);
}

static void Assemble_eor(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 2, 0, /*immed_ok=*/true, /*one_operand=*/false);
}

static void Assemble_eon(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 2, 1, /*immed_ok=*/false, /*one_operand=*/false);
}

static void Assemble_ands(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 3, 0, /*immed_ok=*/true, /*one_operand=*/false);
}

static void Assemble_bics(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 3, 1, /*immed_ok=*/false, /*one_operand=*/false);
}

static void Assemble_tst(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 3, 0, /*immed_ok=*/true, /*one_operand=*/true);
}

static void Assemble_adc(AARCH64Assembler* assembler) {
  AssembleAddSubWithCarry(assembler, 0, 0);
}

static void Assemble_adcs(AARCH64Assembler* assembler) {
  AssembleAddSubWithCarry(assembler, 0, 1);
}

static void Assemble_sbc(AARCH64Assembler* assembler) {
  AssembleAddSubWithCarry(assembler, 1, 0);
}

static void Assemble_sbcs(AARCH64Assembler* assembler) {
  AssembleAddSubWithCarry(assembler, 1, 1);
}

UNDEFINED_INST(adr);
UNDEFINED_INST(adrp);
UNDEFINED_INST(madd);
UNDEFINED_INST(mneg);
UNDEFINED_INST(msub);
UNDEFINED_INST(mul);
UNDEFINED_INST(neg);
UNDEFINED_INST(ngc);
UNDEFINED_INST(negs);
UNDEFINED_INST(ngcs);
UNDEFINED_INST(sdiv);
UNDEFINED_INST(smaddl);
UNDEFINED_INST(smnegl);
UNDEFINED_INST(smsubl);
UNDEFINED_INST(smulh);
UNDEFINED_INST(smull);
UNDEFINED_INST(udiv);
UNDEFINED_INST(umaddl);
UNDEFINED_INST(umnegl);
UNDEFINED_INST(umsubl);
UNDEFINED_INST(umulh);
UNDEFINED_INST(umull);

static void AssembleBitFieldMove(AARCH64Assembler* assembler, int opc,
                                 Register* rd, Register* rn,
                                 int sf, int n, int immr, int imms) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (opc << 29) | (0x26 << 23) | (n << 22) |
                    (immr << 16) | (imms << 10) |
                    (rn->num << 5) | (rd->num));
}

static void AssembleShiftRight(AARCH64Assembler* assembler, int opc) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register  rn = GetRegister(assembler);
  CheckRegWidths(assembler, &rd, &rn);
  NeedComma(assembler);
  LexMatch(&ASM.lex, TOK(hash));
  bool is_64bit = rd.width == kX;
  int64_t shift = AssemblerEvaluateExpression(&ASM);
  int64_t max_shift = is_64bit ? 63 : 31;
  if (shift < 0 || shift > max_shift) {
    AssemblerError(&ASM, "Invalid shift value");
    return;
  }
  int immr = (int)shift;
  int imms = (int)max_shift;
  AssembleBitFieldMove(assembler, opc, &rd, &rn, is_64bit, is_64bit, immr, imms);
}


static void Assemble_lsl(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register  rn = GetRegister(assembler);
  CheckRegWidths(assembler, &rd, &rn);
  NeedComma(assembler);
  LexMatch(&ASM.lex, TOK(hash));
  bool is_64bit = rd.width == kX;
  int64_t shift = AssemblerEvaluateExpression(&ASM);
  int64_t max_shift = is_64bit ? 63 : 31;
  if (shift < 0 || shift > max_shift) {
    AssemblerError(&ASM, "Invalid shift value");
    return;
  }
  
  // Page C6-1678
  // immr is -shift % (32 or 64)
  // imms is (31 or 63) - shift
  int immr = (int)(-shift % (max_shift + 1));
  int imms = (int)(max_shift - shift);
  AssembleBitFieldMove(assembler, 2, &rd, &rn, is_64bit, is_64bit, immr, imms);
}

static void Assemble_asr(AARCH64Assembler* assembler) {
  AssembleShiftRight(assembler, 0);
}

static void Assemble_lsr(AARCH64Assembler* assembler) {
  AssembleShiftRight(assembler, 2);
}


UNDEFINED_INST(bfi);
UNDEFINED_INST(bfxil);
UNDEFINED_INST(cls);
UNDEFINED_INST(clz);
UNDEFINED_INST(extr);
UNDEFINED_INST(rbit);
UNDEFINED_INST(rev);
UNDEFINED_INST(rev16);
UNDEFINED_INST(rev32);
UNDEFINED_INST(sbfiz);
UNDEFINED_INST(ubfiz);
UNDEFINED_INST(sbfx);
UNDEFINED_INST(ubfx);
UNDEFINED_INST(sbxt);
UNDEFINED_INST(sbxtb);
UNDEFINED_INST(sbxth);
UNDEFINED_INST(ubxt);
UNDEFINED_INST(ubxtb);
UNDEFINED_INST(ubxth);
UNDEFINED_INST(sxtb);
UNDEFINED_INST(sxth);
UNDEFINED_INST(sxtw);

static void AssembleMovInstruction(AARCH64Assembler* assembler, Register* rd,
                                   int sf, int opc, int imm16, int hw) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (opc << 29) | (0x25 << 23) |
                    (hw << 21 | (imm16 << 5) | (rd->num)));
}

// Move an immediate value.
// Instructions we can use;
// MOVK - move a shifted 16 bit value and keep rest of bits untouched
// MOVN - move a shifted 16 bit value with zero in the other bits then invert
// MOVZ - move a shifted 16 bit value and zero out other bits.
//
// For 32-bit, shift can be 0 or 16.  For 64-bit, shift can be 0, 16, 32 or 48.
static void AssembleMoveImmediate(AARCH64Assembler* assembler, Register* rd, int64_t immed) {
  bool inverted = false;
  if (immed < 0) {
    // Negative can be encoded using a MOVN.
    immed = ~immed;
    inverted = true;
  }
  if (immed == 0) {
    // Special handling for zero or -1.
    if (inverted) {
      // MOVN.
      AssembleMovInstruction(assembler, rd, rd->width == kX, 0, 0, 0);
    } else {
      // MOVZ.
      AssembleMovInstruction(assembler, rd, rd->width == kX, 2, 0, 0);
    }
    return;
  }
  int num_words = rd->width == kX ? 4 : 2;

  if (inverted) {
    // The original immed was negative so we have to invert it after moving
    // into the register.  There is no MOVK equivalent that keeps the other
    // bits intact, so all we have is MOVN, whicb moves a 16 bit value, zeroes
    // out the other bits and inverts all the bits.  This means that we can
    // only really MOVN one 16-bit value into the register.
    
    // Let's count the number of non-zero 16 bit words in the immed fie;d.
    int num_non_zero_words = 0;
    int n = 0;
    for (int i = 0; i < num_words; i++) {
      int imm16 = (immed >> i*16) & 0xffff;
      if (imm16 != 0) {
        num_non_zero_words++;
        n = i;
      }
    }
    if (num_non_zero_words == 1) {
      // We can use a MOVN.
      int imm16 = (immed >> n*16) & 0xffff;
      AssembleMovInstruction(assembler, rd, rd->width == kX, 0, imm16, n);
      return;
    }
    // We can't use a MOVN, so it's a MOVZ/MOVK sequence.
    immed = ~immed;
  }
  
  bool keep = false;
  for (int i = 0; i < num_words; i++) {
    int imm16 = (immed >> i*16) & 0xffff;
    int opc;
    // We've already checked for immed being 0 so at least one of these has
    // to be true.
    if (imm16 != 0) {
      if (keep) {
        opc = 3;      // MOVK
      } else {
        opc = 2;      // MOVZ
      }
      AssembleMovInstruction(assembler, rd, rd->width == kX, opc, imm16, i);
      keep = true;
    }
  }
}

static void AssembleMove(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Operand op = GetOperand(assembler);
  switch (op.type) {
    case kRegister:
      // Encoded as an ORR immediate with Zero reg as Rn.
      if (!CheckRegWidths(assembler, &rd, &op.reg)) {
        return;
      }
      Register zero = ZeroReg(rd.width);
      AssembleLogicalShiftedRegister(assembler, &rd, &zero, &op, rd.width == kX, 1, 0);
      break;
    case kIntImmediate:
      AssembleMoveImmediate(assembler, &rd, op.i);
      break;
    case kFloatImmediate:
      // TODO: do this.
      break;
    case kUnknown:
      abort();
  }
}

static void AssembleMoveX(AARCH64Assembler* assembler, int opc) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Operand op = GetOperand(assembler);
  if (op.type != kIntImmediate) {
    AssemblerError(&ASM, "Immediate value expected");
    return;
  }
  
  int64_t shift = 0;
  if (LexMatch(&ASM.lex, TOK(comma))) {
    // Check for LSL.
    if (!LexLookingAt(&ASM.lex, TOK(identifier)) || !StringEqualCaseBlind(&ASM.lex.spelling, "LSL")) {
      AssemblerError(&ASM, "LSL expected for MOV");
      return;
    }
    LexNextToken(&ASM.lex);
    LexMatch(&ASM.lex, TOK(hash));      // Optional #.
    shift = AssemblerEvaluateExpression(&ASM);
    if (rd.width == kX) {
      switch (shift) {
        case 0:
        case 16:
        case 32:
        case 48:
          break;
        default:
          AssemblerError(&ASM, "Invalid shift for MOV immediate wide");
          return;
      }
    } else {
      switch (shift) {
        case 0:
        case 16:
          break;
        default:
          AssemblerError(&ASM, "Invalid shift for MOV immediate wide");
          return;
      }
    }
  }
  CheckImmediateWidth(assembler, op.i, 16);
  AssembleMovInstruction(assembler, &rd,
                         rd.width == kX,
                         opc, op.i, (int)shift / 16);
}

static void Assemble_mov(AARCH64Assembler* assembler) {
  AssembleMove(assembler);
}

static void Assemble_movk(AARCH64Assembler* assembler) {
  AssembleMoveX(assembler, 3);
}

static void Assemble_movn(AARCH64Assembler* assembler) {
  AssembleMoveX(assembler, 0);
}

static void Assemble_movz(AARCH64Assembler* assembler) {
  AssembleMoveX(assembler, 2);
}
UNDEFINED_INST(asri);
UNDEFINED_INST(eons);
UNDEFINED_INST(mvn);
UNDEFINED_INST(ror);

static void AssembleConditionalBranch(AARCH64Assembler* assembler,
                                      int cond, int consistent) {
  int64_t addr = AssemblerEvaluateExpression(&ASM);
  int32_t offset = (int32_t)(addr - AssemblerCurrentAddress(&ASM));

  // We have a 19 bit immediate which is a multiple of 4, so this give us
  // 21 bits of range.
  int32_t off = offset < 0 ? -offset : offset;
  if (off > (1 << 21)) {
    AssemblerError(&ASM, "Branch offset out of range");
    return;
  }
  if ((off & 3) != 0) {
    AssemblerError(&ASM, "Branch offset needs to be mutliple of 4");
    return;
  }
  // imm19 is encoded as the top 19 bits of the offset.
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (0x2a << 25) |
                    ((offset >> 2) << 5) |    // imm19.
                    (consistent << 4) | cond);
}

#define COND_BRANCH(cond) \
static void Assemble_b##cond(AARCH64Assembler* assembler) {\
  AssembleConditionalBranch(assembler, kCond_##cond, 0);\
} \
\
static void Assemble_bc##cond(AARCH64Assembler* assembler) {\
  AssembleConditionalBranch(assembler, kCond_##cond, 1);\
}

COND_BRANCH(eq);
COND_BRANCH(ne);
COND_BRANCH(cs);
COND_BRANCH(hs);
COND_BRANCH(cc);
COND_BRANCH(lo);
COND_BRANCH(mi);
COND_BRANCH(pl);
COND_BRANCH(vs);
COND_BRANCH(vc);
COND_BRANCH(hi);
COND_BRANCH(ls);
COND_BRANCH(ge);
COND_BRANCH(lt);
COND_BRANCH(gt);
COND_BRANCH(le);
COND_BRANCH(al);

#undef COND_BRANCH

static void AssembleUnconditionalBranchImmediate(AARCH64Assembler* assembler, int l) {
  // TODO: branch to a symbol with a relocation.
  int64_t addr = AssemblerEvaluateExpression(&ASM);
  int32_t offset = (int32_t)(addr - AssemblerCurrentAddress(&ASM));

  // We have a 26 bit immediate which is a multiple of 4, so this give us
  // 28 bits of range.
  int32_t off = offset < 0 ? -offset : offset;
  if (off > (1 << 28)) {
    AssemblerError(&ASM, "Branch offset out of range");
    return;
  }
  if ((off & 3) != 0) {
    AssemblerError(&ASM, "Branch offset needs to be mutliple of 4");
    return;
  }
  // imm19 is encoded as the top 19 bits of the offset.
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (l << 31) |
                    (0x5 << 26) |
                    ((offset >> 2) << 5));    // imm26.
}

static void Assemble_b(AARCH64Assembler* assembler) {
  AssembleUnconditionalBranchImmediate(assembler, 0);
}

static void Assemble_bl(AARCH64Assembler* assembler) {
  AssembleUnconditionalBranchImmediate(assembler, 1);
}

static void AssembleUnconditionalBranchRegister(AARCH64Assembler* assembler, int opc, int op3) {
  Register r = GetRegister(assembler);
  
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (0x6b << 25) |
                    (opc << 21) |
                    (0x1f << 16) |
                    (op3 << 10) |
                    (0x5 << 26) |
                    (r.num << 5));
}
        
static void Assemble_blr(AARCH64Assembler* assembler) {
  AssembleUnconditionalBranchRegister(assembler, 1, 0);
}

static void Assemble_br(AARCH64Assembler* assembler) {
  AssembleUnconditionalBranchRegister(assembler, 0, 0);
}

static void Assemble_ret(AARCH64Assembler* assembler) {
  // Register is optional with 30 as the default.
  int reg = 30;
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    Register r = GetRegister(assembler);
    reg = r.num;
  }
  
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (0x6b << 25) |
                    (2 << 21) |
                    (0x1f << 16) |
                    (0x5 << 26) |
                    (reg << 5));
}


static void AssembleCompareAndBranch(AARCH64Assembler* assembler, int op) {
  Register rt = GetRegister(assembler);
  NeedComma(assembler);
  
  int64_t addr = AssemblerEvaluateExpression(&ASM);
  int32_t offset = (int32_t)(addr - AssemblerCurrentAddress(&ASM));

  // We have a 19 bit immediate which is a multiple of 4, so this give us
  // 21 bits of range.
  int32_t off = offset < 0 ? -offset : offset;
  if (off > (1 << 21)) {
    AssemblerError(&ASM, "Branch offset out of range");
    return;
  }
  if ((off & 3) != 0) {
    AssemblerError(&ASM, "Branch offset needs to be mutliple of 4");
    return;
  }
  int sf = rt.width == kX;
  int imm19 = off >> 2;
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) |
                    (0x1a << 25) |
                    (op << 24) |
                    (imm19 << 5) |
                    (rt.num));
}


static void Assemble_cbnz(AARCH64Assembler* assembler) {
  AssembleCompareAndBranch(assembler, 1);
}

static void Assemble_cbz(AARCH64Assembler* assembler) {
  AssembleCompareAndBranch(assembler, 0);
}

static void AssembleTestAndBranch(AARCH64Assembler* assembler, int op) {
  Register rt = GetRegister(assembler);
  NeedComma(assembler);

  LexMatch(&ASM.lex, TOK(hash));
  int imm = (int)AssemblerEvaluateExpression(&ASM);

  NeedComma(assembler);
  int64_t addr = AssemblerEvaluateExpression(&ASM);
  int32_t offset = (int32_t)(addr - AssemblerCurrentAddress(&ASM));

  // We have a 14 bit immediate which is a multiple of 4, so this give us
  // 16 bits of range.
  int32_t off = offset < 0 ? -offset : offset;
  if (off > (1 << 16)) {
    AssemblerError(&ASM, "Branch offset out of range");
    return;
  }
  if ((off & 3) != 0) {
    AssemblerError(&ASM, "Branch offset needs to be mutliple of 4");
    return;
  }
  // Validate bit in imm.
  int max_bit = rt.width == kX ? 63 : 31;
  if (imm < 0 || imm > max_bit) {
    AssemblerError(&ASM, "Invalid bit number, need 0..%d", max_bit);
    return;
  }
  int b5 = imm >> 5;      // Bit 5.
  int b40 = imm & 0x1f;   // Bits 4 to 0.
  
  int imm14 = off >> 2;
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (b5 << 31) |
                    (0x1b << 25) |
                    (op << 24) |
                    (b40 << 19) |
                    (imm14 << 5) |
                    (rt.num));
}

static void Assemble_tbnz(AARCH64Assembler* assembler) {
  AssembleTestAndBranch(assembler, 1);
}

static void Assemble_tbz(AARCH64Assembler* assembler) {
  AssembleTestAndBranch(assembler, 0);
}

UNDEFINED_INST(ccmn);
UNDEFINED_INST(ccmni);
UNDEFINED_INST(ccmp);
UNDEFINED_INST(ccmpi);
UNDEFINED_INST(cinc);
UNDEFINED_INST(cinv);
UNDEFINED_INST(cneg);
UNDEFINED_INST(csel);
UNDEFINED_INST(cset);
UNDEFINED_INST(csetm);
UNDEFINED_INST(csinc);
UNDEFINED_INST(csinv);
UNDEFINED_INST(csneg);

static void AssembleLoadLiteral(AARCH64Assembler* assembler, Register* rt,
                                int opc, int v,
                                int32_t imm19) {
  CheckImmediateWidth(assembler, imm19, 19);

  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (opc << 30) |
                    (0x3 << 27) |
                    (v << 26) |
                    (imm19 << 5) |
                    (rt->num));
}

static void AssembleLoadStoreImmediate(AARCH64Assembler* assembler, Register* rt,
                                  Register* rn, int size, int fp,
                                int opc, int v, int pre_index,
                                int32_t imm9) {
  CheckImmediateWidth(assembler, imm9, 9);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (size << 30) |
                    (0x7 << 27) |
                    (fp << 26) |
                    (opc << 22) |
                    (v << 26) |
                    (imm9 << 12) |
                    (pre_index << 11) |
                    (1 << 10) |
                    (rn->num << 5) |
                    (rt->num));
}

// C4-587.
static void AssembleLoadStoreRegister(AARCH64Assembler* assembler, Register* rt,
                                  Register* rn, Register* rm,
                                      int size,
                                int opc, int v, int option,
                                int s) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (size << 30) |
                    (0x7 << 27) |
                    (v << 26) |
                    (opc << 22) |
                    (1 << 21) |
                    (rm->num << 16) |
                    (option << 13) |
                    (s << 12) |
                    (0x2 << 10) |
                    (rn->num << 5) |
                    (rt->num));
}

static void AssembleLoadStorePair(AARCH64Assembler* assembler, Register* rt,
                                  Register* rt2,
                                  Register* rn,
                                int opc, int v, int l,
                                int32_t imm7) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (opc << 30) |
                    (0x5 << 27) |
                    (opc << 22) |
                    (v << 26) |
                    (0x2 << 23) |
                    (l << 22) |
                    (imm7 << 15) |
                    (rt2->num << 10) |
                    (1 << 10) |
                    (rn->num << 5) |
                    (rt->num));
}

// Page C4-571.
static void AssembleLoadStore(AARCH64Assembler* assembler, int is_load,
                              int size,
                              int is_signed, bool is_pair) {
  Register rt = GetRegister(assembler);
  NeedComma(assembler);
  Register rt2;
  if (is_pair) {
    rt2 = GetRegister(assembler);
    NeedComma(assembler);
    if (rt.fp_or_simd || rt2.fp_or_simd) {
      AssemblerError(&ASM, "LDP/STP require non FP/SIMD registers");
      return;
    }
  }
  if (LexMatch(&ASM.lex, TOK(lsquare))) {
    Register rn = GetRegister(assembler);
    if (rn.fp_or_simd) {
      AssemblerError(&ASM, "Base must be an integer register");
      return;
    }
    Operand offset;
    bool pre_indexed = false;
    bool post_indexed = false;
    bool writeback = false;
    
    if (LexMatch(&ASM.lex, TOK(comma))) {
      // Preindexed.
      pre_indexed = true;
      offset = GetOperand(assembler);
    }
    if (!LexMatch(&ASM.lex, TOK(rsquare))) {
      AssemblerError(&ASM, "Expected close ]");
      return;
    }
    if (LexMatch(&ASM.lex, TOK(comma))) {
      // Postindexed.
      if (pre_indexed) {
        AssemblerError(&ASM, "Invalid addressing mode");
        return;
      }
      post_indexed = true;
      offset = GetOperand(assembler);
    }
    if (LexMatch(&ASM.lex, TOK(bang))) {
      // Writeback.
      if (post_indexed) {
        AssemblerError(&ASM, "Invalid writeback mode");
        return;
      }
      writeback = true;
    }

    if (is_pair) {
      int imm7 = offset.i / (rt.width == kX ? 8 : 4);
      AssembleLoadStorePair(assembler, &rt, &rt2, &rn, rt.width == kX, 0, is_load, imm7);
    } else {
      if (offset.type == kRegister) {
        // Register offset.
        // A register offset can have a shift.
        if (LexMatch(&ASM.lex, TOK(comma))) {
          if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
            AssemblerError(&ASM, "Expected register shift LSL, UXTW, SXTW or SXTX");
            return;
          }
          int option = 3;
          if (StringEqualCaseBlind(&ASM.lex.spelling, "LSL")) {
          } else if (StringEqualCaseBlind(&ASM.lex.spelling, "UXTW")) {
            option = 2;
          } else if (StringEqualCaseBlind(&ASM.lex.spelling, "SXTW")) {
            option = 6;
          } else if (StringEqualCaseBlind(&ASM.lex.spelling, "SXTX")) {
            option = 7;
          } else {
            AssemblerError(&ASM, "Invalid register shift");
            return;
          }
          LexMatch(&ASM.lex, TOK(hash));
          int amount = (int)AssemblerEvaluateExpression(&ASM);
          // Validate shift amount and encode in S.
          int s = 1;
          switch (amount) {
            case 0:
              s = 0;
              break;
            case 2:
              if (rt.width != kW) {
                AssemblerError(&ASM, "Invalid register shift amount");
                return;
              }
              break;
            case 3:
              if (rt.width != kX) {
                AssemblerError(&ASM, "Invalid register shift amount");
                return;
              }
              break;
            default:
              AssemblerError(&ASM, "Invalid register shift amount");
              return;
          }
          int opc = is_load;
          if (rt.width == kX || rt.width == kW) {
            size = rt.size;
          }
          // TODO: extended register variant.
          AssembleLoadStoreRegister(assembler, &rt, &rn, &offset.reg, size, opc, 0, option, s);
        }
      } else if (offset.type == kIntImmediate){
        // Immediate offset.
        // TODO: check imm9 size.
        int opc = is_signed << 1 | is_load;
        if (rt.width == kX || rt.width == kW) {
          size = rt.size;
        }

        int imm9 = offset.i / size;
        AssembleLoadStoreImmediate(assembler, &rt, &rn, size, rt.fp_or_simd, opc, 0, pre_indexed, imm9);
      } else {
        AssemblerError(&ASM, "Invalid offset for LDR/STR");
       return;
      }
    }
  } else {
    // Literal.
    if (!is_load) {
      AssemblerError(&ASM, "Only load is supported for literals");
      return;
    }
    if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
      AssemblerError(&ASM, "Missing symbol for literal");
      return;
    }
    String symbol_name;
    StringInit(&symbol_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
     AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);

    int reloc_type = R_AARCH64_CALL26;
//    if (sym->binding == SYM_BIND(global) && assembler->base.pic) {
//      reloc_type = R_AARCH64_CALL_PLT;
//    }
    AssemblerRelocation* reloc = NewAssemblerRelocation(
        sym, reloc_type,
        ASM.current_section, (int32_t)AssemblerCurrentAddress(&ASM), 0);
    AssemblerAddRelocation(&ASM, reloc);
    
    int opc = rt.width == kX;
    int v = 0;
    // TODO: floating point and ldrsw.
    AssembleLoadLiteral(assembler, &rt, opc, v, 0);
  }
}

void Assemble_ldrb(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 0, 0, false);
}

void Assemble_ldrh(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 1, 0, false);
}

void Assemble_ldrsb(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 0, 1, false);
}

void Assemble_ldrsh(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 1, 1, false);
}

void Assemble_strb(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, 0, 0, false);
}

void Assemble_strh(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, 1, 0, false);
}

void Assemble_ldr(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 2, 0, false);
}

void Assemble_str(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, 2, 0, false);
}

void Assemble_ldp(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 0, 0, true);
}

void Assemble_stp(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, 0, 0, true);
}

UNDEFINED_INST(ldpsw);
UNDEFINED_INST(ldur);
UNDEFINED_INST(ldurb);
UNDEFINED_INST(ldurh);
UNDEFINED_INST(ldursb);
UNDEFINED_INST(ldursh);
UNDEFINED_INST(ldursw);
UNDEFINED_INST(prfm);
UNDEFINED_INST(stur);
UNDEFINED_INST(sturb);
UNDEFINED_INST(sturh);

UNDEFINED_INST(fldr);
UNDEFINED_INST(fstr);
UNDEFINED_INST(fadd);
UNDEFINED_INST(fsub);
UNDEFINED_INST(fmul);
UNDEFINED_INST(fdiv);
UNDEFINED_INST(fsqrt);
UNDEFINED_INST(fmin);
UNDEFINED_INST(fmax);
UNDEFINED_INST(fcvtsd);     // Single to double
UNDEFINED_INST(fcvtds);     // Double to single.
UNDEFINED_INST(fcvtns);
UNDEFINED_INST(fcvtnu);
UNDEFINED_INST(fcvt);     // Copy from int reg (no conversion)
UNDEFINED_INST(fmov);
UNDEFINED_INST(fcmp);
UNDEFINED_INST(scvtf);
UNDEFINED_INST(ucvtf);
UNDEFINED_INST(fneg);

#undef INST
#undef UNDEFINED_INST
#undef ASM

// Main assembly function.  This is called by the assembler driver.  It will be
// called twice, one for each pass.
// In pass 1 we parse everything and define all the symbols.
// In pass 2 we also parse everything but we also insert the binary instructions
//    and data into the buffers and expect all symbols to be defined.
void AssembleAARCH64Instruction(Assembler* base, String* word) {
  AARCH64Assembler* assembler = (AARCH64Assembler*)base;

  void* asm_func = MapFindPointerKey(&assembler->instructions, word->value);
  if (asm_func != NULL) {
    void (*func)(AARCH64Assembler*) = asm_func;
    func(assembler);
  } else {
    AssemblerError(&assembler->base, "Syntax error; unknown instruction: %s",
                   word->value);
  }
}

bool AARCH64AssemblerInit(AARCH64Assembler* assembler, String* infile, String* outfile) {
  static int reloc_types[] = {
    R_AARCH64_ABS16, R_AARCH64_ABS32,       R_AARCH64_ABS64,       R_AARCH64_ABS16, R_AARCH64_ABS32,
    R_AARCH64_ABS64,    R_AARCH64_ABS16,    R_AARCH64_ABS32, R_AARCH64_ABS64,
    R_AARCH64_CALL26, R_AARCH64_ADR_PREL_PG_HI21,
  };
  if (!AssemblerInit(&assembler->base, ELF_MACHINE_TYPE_AARCH64, 0, reloc_types, infile, outfile)) {
    return false;
  }

  MapInitForCharPointerKeys(&assembler->instructions);

  InitializeInstructions(&assembler->instructions);

  // Add a NULL section at the start of the file.
  AssemblerAddSection(&assembler->base, NULL, SHT(null), 0, 0);
  // Add a .bss section.
  assembler->bss = AssemblerAddSection(&assembler->base, NewString(".bss"),
                                       SHT(nobits), SHF(alloc) | SHF(write), 8);
  return true;
}

AARCH64Assembler* NewAARCH64Assembler(String* infile, String* outfile) {
  AARCH64Assembler* assembler = malloc(sizeof(AARCH64Assembler));
  AARCH64AssemblerInit(assembler, infile, outfile);
  return assembler;
}

// Destruct the assembler.
void AARCH64AssemblerDestruct(AARCH64Assembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void AARCH64AssemblerDelete(AARCH64Assembler* assembler) {
  AARCH64AssemblerDestruct(assembler);
  free(assembler);
}
