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
DECLARE_INST_FUNC(asrv);
DECLARE_INST_FUNC(bic);
DECLARE_INST_FUNC(bics);
DECLARE_INST_FUNC(eon);
DECLARE_INST_FUNC(eons);
DECLARE_INST_FUNC(lsl);
DECLARE_INST_FUNC(lslv);
DECLARE_INST_FUNC(lsr);
DECLARE_INST_FUNC(lsrv);
DECLARE_INST_FUNC(rorv);
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
DECLARE_INST_FUNC(ldxr);
DECLARE_INST_FUNC(ldxrb);
DECLARE_INST_FUNC(ldxrh);
DECLARE_INST_FUNC(ldaxr);
DECLARE_INST_FUNC(ldaxrb);
DECLARE_INST_FUNC(ldaxrh);
DECLARE_INST_FUNC(stxr);
DECLARE_INST_FUNC(stxrb);
DECLARE_INST_FUNC(stxrh);
DECLARE_INST_FUNC(stlxr);
DECLARE_INST_FUNC(stlxrb);
DECLARE_INST_FUNC(stlxrh);
DECLARE_INST_FUNC(ldar);
DECLARE_INST_FUNC(ldarb);
DECLARE_INST_FUNC(ldarh);
DECLARE_INST_FUNC(stlr);
DECLARE_INST_FUNC(stlrb);
DECLARE_INST_FUNC(stlrh);
DECLARE_INST_FUNC(dmb);
DECLARE_INST_FUNC(clrex);
DECLARE_INST_FUNC(mrs);

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
DECLARE_INST_FUNC(fcvtzs);
DECLARE_INST_FUNC(fcvtzu);
DECLARE_INST_FUNC(fcvt);     // Copy from int reg (no conversion)
DECLARE_INST_FUNC(fmov);
DECLARE_INST_FUNC(fcmp);
DECLARE_INST_FUNC(scvtf);
DECLARE_INST_FUNC(ucvtf);
DECLARE_INST_FUNC(fneg);
DECLARE_INST_FUNC(svc);
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
  INST2(ubxtb, uxtb);
  INST2(ubxth, uxth);
  INST(sxtb);
  INST(sxth);
  INST(sxtw);

  INST(and);
  INST(ands);
  INST(asr);
  INST(asri);
  INST(asrv);
  INST(bic);
  INST(bics);
  INST(eon);
  INST(lsl);
  INST(lslv);
  INST(lsr);
  INST(lsrv);
  INST(mov);
  INST(movk);
  INST(movn);
  INST(movz);
  INST(mvn);
  INST(orn);
  INST(orr);
  INST(ror);
  INST(rorv);
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
  INST2(bls, b.ls);
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
  INST(ldxr);
  INST(ldxrb);
  INST(ldxrh);
  INST(ldaxr);
  INST(ldaxrb);
  INST(ldaxrh);
  INST(stxr);
  INST(stxrb);
  INST(stxrh);
  INST(stlxr);
  INST(stlxrb);
  INST(stlxrh);
  INST(ldar);
  INST(ldarb);
  INST(ldarh);
  INST(stlr);
  INST(stlrb);
  INST(stlrh);
  INST(dmb);
  INST(clrex);
  INST(mrs);

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
  INST(fcvtzs);
  INST(fcvtzu);
  INST(fcvt);     // Copy from int reg (no conversion)
  INST(fmov);
  INST(fcmp);
  INST(scvtf);
  INST(ucvtf);
  INST(fneg);
  INST(svc);
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
  bool is_sp;
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
    int64_t i;
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
    // AArch64 encodes SP as register 31 in instruction words.
    reg.num = 31;
    reg.is_sp = true;
    return reg;
  }
  if (StringEqualCaseBlind(&ASM.lex.spelling, "xzr") ||
      StringEqualCaseBlind(&ASM.lex.spelling, "wzr")) {
    reg.width = toupper(ASM.lex.spelling.value[0]) == 'X' ? kX : kW;
    reg.num = 31;
    reg.size = reg.width == kX ? 3 : 2;
    LexNextToken(&ASM.lex);
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
      op.i = AssemblerEvaluateExpression(&ASM);
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
  } else if (StringEqualCaseBlind(&ASM.lex.spelling, "ASR")) {
    op->shift.type = kASR;
  } else {
    AssemblerError(&ASM, "Invalid shift %s", ASM.lex.spelling.value);
    return false;
  }
  LexNextToken(&ASM.lex);
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

static COMPILER_UNUSED bool AssemblerFunction(AARCH64Assembler* assembler, String* func,
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
                                    int immed, int sf, int op, int s,
                                    int shift) {
  // The add/sub immediate field is an unsigned 12-bit value.  A negative
  // immediate (common when computing the address of a stack local at a
  // negative frame-pointer offset, e.g. "add x0, x29, #-384") must be encoded
  // as the opposite operation with the negated immediate (here "sub x0, x29,
  // #384").  This also applies to the flag-setting forms: "cmp Rn, #-k"
  // (subs) becomes "cmn Rn, #k" (adds).  The flags are *identical* between the
  // two forms - subs uses AddWithCarry(Rn, NOT(-k), 1) = AddWithCarry(Rn,
  // k-1, 1) and adds uses AddWithCarry(Rn, k, 0); both yield the same
  // unsigned and signed sums, so N/Z/C/V all match - so flipping is safe.
  if (immed < 0) {
    immed = -immed;
    op ^= 1;  // ADD <-> SUB (bit 30 of the encoding).
  }
  CheckImmediateWidth(assembler, immed, 12);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (op << 30) | (s << 29) | (0x22 << 23) |
          (shift << 22) |
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
  // Handle symbol relocation modifiers used as ADD immediates.
  if (LexLookingAt(&ASM.lex, TOK(colon))) {
    LexNextToken(&ASM.lex);  // consume first ':'
    if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
      AssemblerError(&ASM, "Expected relocation modifier");
      return;
    }
    int reloc_type;
    bool shifted = false;
    if (StringEqualCaseBlind(&ASM.lex.spelling, "lo12")) {
      reloc_type = R_AARCH64_ADD_ABS_LO12_NC;
    } else if (StringEqualCaseBlind(&ASM.lex.spelling, "tprel_hi12")) {
      reloc_type = R_AARCH64_TLSLE_ADD_TPREL_HI12;
      shifted = true;
    } else if (StringEqualCaseBlind(&ASM.lex.spelling, "tprel_lo12_nc")) {
      reloc_type = R_AARCH64_TLSLE_ADD_TPREL_LO12_NC;
    } else {
      AssemblerError(&ASM, "Unsupported ADD relocation modifier");
      return;
    }
    LexNextToken(&ASM.lex);
    LexMatch(&ASM.lex, TOK(colon));  // consume second ':'
    AssemblerSymbol* sym = GetOrCreateSymbol(assembler, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    int32_t instruction_offset = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerRelocation* reloc =
        NewAssemblerRelocation(sym, reloc_type,
                               ASM.current_section, instruction_offset, 0);
    AssemblerAddRelocation(&ASM, reloc);
    if (shifted) {
      AssemblerEmitWord(&ASM, ASM.current_section,
                        ((uint32_t)(rd.width == kX) << 31) |
                            ((uint32_t)opcode << 30) | ((uint32_t)s << 29) |
                            (0x22u << 23) | (1u << 22) |
                            ((uint32_t)rn.num << 5) | (uint32_t)rd.num);
    } else {
      AssembleAddSubImmediate(assembler, &rd, &rn, 0, rd.width == kX, opcode,
                              s, 0);
    }
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
      {
        int shift = 0;
        if (LexMatch(&ASM.lex, TOK(comma))) {
          if (!GetShift(assembler, &src2, 12)) {
            return;
          }
          if (src2.shift.type != kLSL || src2.shift.amount != 12) {
            AssemblerError(&ASM,
                           "ADD/SUB immediate shift must be LSL #12");
            return;
          }
          shift = 1;
        }
        AssembleAddSubImmediate(assembler, &rd, &rn, src2.i,
                                rd.width == kX, opcode, s, shift);
      }
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

// A value of the form 0...01...1 (a run of ones at the low end).
static bool AARCH64IsMask64(uint64_t v) {
  return v != 0 && ((v + 1) & v) == 0;
}

// A value of the form 0...01...10...0 (a single contiguous run of ones).
static bool AARCH64IsShiftedMask64(uint64_t v) {
  return v != 0 && AARCH64IsMask64((v - 1) | v);
}

static unsigned AARCH64CountTrailingZeros64(uint64_t value) {
  unsigned count = 0;
  while ((value & 1) == 0) {
    ++count;
    value >>= 1;
  }
  return count;
}

static unsigned AARCH64CountLeadingZeros64(uint64_t value) {
  unsigned count = 0;
  for (uint64_t bit = 1ULL << 63; (value & bit) == 0; bit >>= 1) {
    ++count;
  }
  return count;
}

static unsigned AARCH64PopulationCount64(uint64_t value) {
  unsigned count = 0;
  while (value != 0) {
    value &= value - 1;
    ++count;
  }
  return count;
}

// Encode a logical (bitmask) immediate VALUE into the 13-bit N:immr:imms field
// used by AND/ORR/EOR/ANDS immediate forms.  Returns false when the value is
// not a representable bitmask immediate (e.g. zero or all-ones).  Follows the
// standard AArch64 reference algorithm (cf. LLVM
// AArch64_AM::processLogicalImmediate).
static bool AARCH64EncodeLogicalImmediate(uint64_t imm, int sf,
                                          unsigned* encoding) {
  unsigned reg_size = sf ? 64 : 32;
  if (reg_size != 64) {
    if ((imm >> reg_size) != 0 && imm != (uint64_t)(int64_t)(int32_t)imm) {
      // High bits set on a 32-bit operand that aren't a sign extension: not a
      // valid 32-bit pattern.
      return false;
    }
    imm &= 0xffffffffULL;
  }
  uint64_t all_ones = reg_size == 64 ? ~0ULL : 0xffffffffULL;
  if (imm == 0 || imm == all_ones) {
    return false;
  }

  // Determine the element size of the repeating pattern.
  unsigned size = reg_size;
  do {
    size /= 2;
    uint64_t mask = (1ULL << size) - 1;
    if ((imm & mask) != ((imm >> size) & mask)) {
      size *= 2;
      break;
    }
  } while (size > 2);

  uint64_t mask = (~0ULL) >> (64 - size);
  imm &= mask;

  unsigned i;
  unsigned cto;
  if (AARCH64IsShiftedMask64(imm)) {
    i = AARCH64CountTrailingZeros64(imm);
    cto = AARCH64CountTrailingZeros64(~(imm >> i));
  } else {
    imm |= ~mask;
    if (!AARCH64IsShiftedMask64(~imm)) {
      return false;
    }
    unsigned clo = AARCH64CountLeadingZeros64(~imm);
    i = 64 - clo;
    cto = size - AARCH64PopulationCount64(~imm);
  }

  unsigned immr = (size - i) & (size - 1);
  unsigned nimms = (~(size - 1)) << 1;
  nimms |= (cto - 1);
  unsigned n = ((nimms >> 6) & 1) ^ 1;
  *encoding = ((n & 1) << 12) | (immr << 6) | (nimms & 0x3f);
  return true;
}

static void AssembleLogicalImmediate(AARCH64Assembler* assembler,
                                    Register* rd, Register* rn,
                                    int64_t immed, int sf, int opc) {
  unsigned encoding;
  if (!AARCH64EncodeLogicalImmediate((uint64_t)immed, sf, &encoding)) {
    AssemblerError(&ASM, "Invalid logical immediate");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (opc << 29) | (0x24 << 23) |
                    (encoding << 10 | (rn->num << 5) | (rd->num)));
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

static void AssembleADR(AARCH64Assembler* assembler, int op) {
  Register rd = GetRegister(assembler);
  if (!NeedComma(assembler)) {
    return;
  }
  int32_t instruction_offset = (int32_t)AssemblerCurrentAddress(&ASM);
  bool known = false;
  int64_t addr = 0;
  AssemblerSymbol* sym = NULL;
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    sym = GetOrCreateSymbol(assembler, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    // Keep calls to weak definitions relocatable so the linker can select a
    // strong override.
    known = sym->defined && sym->section == ASM.current_section &&
            sym->binding != SYM_BIND(weak);
    addr = sym->value;
  } else {
    addr = AssemblerEvaluateKnownExpression(&ASM, &known);
  }
  // For adrp (op == 1) the immediate is a *page* delta computed from final
  // load addresses, not a section-relative byte offset.  Resolving it at
  // assembly time using section-relative offsets (the ADR-style math below)
  // produces a wrong page number.  Always defer adrp to the linker, which has
  // the final addresses, when it targets a symbol.
  if (op == 1 && sym != NULL) {
    known = false;
  }
  // For a symbol target the encoded value is the PC-relative displacement
  // (symbol_address - this_instruction_address).  A bare numeric operand, by
  // contrast, is already a PC-relative byte displacement: the computed-branch
  // table jump emits `adr t1, 12` to mean "pc + 12" (the start of the jump
  // table 12 bytes ahead).  Applying the symbol-style `addr - pc` math to such
  // a constant would treat it as an absolute section offset and produce a
  // wildly wrong displacement, so use the constant directly.
  int32_t offset =
      (sym != NULL) ? (int32_t)(addr - instruction_offset) : (int32_t)addr;
  int32_t immlo = (offset & 0x3);
  int32_t immhi = (offset >> 2) & 0x7ffff;
  if (!known) {
    if (sym != NULL) {
      AssemblerRelocation* reloc = NewAssemblerRelocation(
          sym, op == 0 ? R_AARCH64_ADR_PREL_LO21 : R_AARCH64_ADR_PREL_PG_HI21,
          ASM.current_section, instruction_offset, 0);
      AssemblerAddRelocation(&ASM, reloc);
    }
    immlo = 0;
    immhi = 0;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      (op << 31) | (immlo << 29) | (0x10 << 24) | (immhi << 5) | rd.num);
}

static void Assemble_adr(AARCH64Assembler* assembler) {
  AssembleADR(assembler, 0);
}

static void Assemble_adrp(AARCH64Assembler* assembler) {
  AssembleADR(assembler, 1);
}

static void AssembleDataProcessing3Source(AARCH64Assembler* assembler,
                                          int op54, int op31, int o0) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  NeedComma(assembler);
  Register ra = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rn) ||
      !CheckRegWidths(assembler, &rd, &rm) ||
      !CheckRegWidths(assembler, &rd, &ra)) {
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (0x1b << 24) | (op54 << 21) |
      (rm.num << 16) | (o0 << 15) | (ra.num << 10) |
      (rn.num << 5) | rd.num);
}

static void AssembleDataProcessing3SourceLong(AARCH64Assembler* assembler,
                                              int op54, int op31, int o0) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  NeedComma(assembler);
  Register ra = GetRegister(assembler);
  if (rd.width != kX || ra.width != kX || rn.width != kW || rm.width != kW) {
    AssemblerError(&ASM, "Invalid widening multiply register widths");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      (1 << 31) | (0x1b << 24) | (op54 << 21) | (op31 << 15) |
      (rm.num << 16) | (o0 << 15) | (ra.num << 10) |
      (rn.num << 5) | rd.num);
}

static void Assemble_madd(AARCH64Assembler* assembler) {
  AssembleDataProcessing3Source(assembler, 0, 0, 0);
}

static void Assemble_msub(AARCH64Assembler* assembler) {
  AssembleDataProcessing3Source(assembler, 0, 0, 1);
}

static void AssembleMulAlias(AARCH64Assembler* assembler, int negate) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rn) ||
      !CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  Register zero = ZeroReg(rd.width);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (0x1b << 24) |
      (rm.num << 16) | (negate << 15) | (zero.num << 10) |
      (rn.num << 5) | rd.num);
}

static void Assemble_mul(AARCH64Assembler* assembler) {
  AssembleMulAlias(assembler, 0);
}

static void Assemble_mneg(AARCH64Assembler* assembler) {
  AssembleMulAlias(assembler, 1);
}

static void AssembleNegAlias(AARCH64Assembler* assembler, int op, int set_flags) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Operand src = GetOperand(assembler);
  if (src.type != kRegister) {
    AssemblerError(&ASM, "Expected register");
    return;
  }
  Register zero = ZeroReg(rd.width);
  if (!CheckRegWidths(assembler, &rd, &src.reg)) {
    return;
  }
  AssembleAddSubShiftedRegister(
      assembler, &rd, &zero, &src, rd.width == kX, op, set_flags);
}

static void Assemble_neg(AARCH64Assembler* assembler) {
  AssembleNegAlias(assembler, 1, 0);
}

static void Assemble_negs(AARCH64Assembler* assembler) {
  AssembleNegAlias(assembler, 1, 1);
}

static void AssembleNgcAlias(AARCH64Assembler* assembler, int set_flags) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  Register zero = ZeroReg(rd.width);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (1 << 30) | (set_flags << 29) |
      (0xd0 << 21) | (rm.num << 16) | (zero.num << 5) | rd.num);
}

static void Assemble_ngc(AARCH64Assembler* assembler) {
  AssembleNgcAlias(assembler, 0);
}

static void Assemble_ngcs(AARCH64Assembler* assembler) {
  AssembleNgcAlias(assembler, 1);
}

static void AssembleDivide(AARCH64Assembler* assembler, int unsigned_divide) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rn) ||
      !CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (0xd6 << 21) |
      (rm.num << 16) | ((unsigned_divide ? 2 : 3) << 10) |
      (rn.num << 5) | rd.num);
}

static void AssembleShiftVariable(AARCH64Assembler* assembler, int shift_op) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rn) ||
      !CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (0xd6 << 21) | (rm.num << 16) |
      (shift_op << 10) | (rn.num << 5) | rd.num);
}

static void Assemble_lslv(AARCH64Assembler* assembler) {
  AssembleShiftVariable(assembler, 8);
}

static void Assemble_lsrv(AARCH64Assembler* assembler) {
  AssembleShiftVariable(assembler, 9);
}

static void Assemble_asrv(AARCH64Assembler* assembler) {
  AssembleShiftVariable(assembler, 10);
}

static void Assemble_rorv(AARCH64Assembler* assembler) {
  AssembleShiftVariable(assembler, 11);
}

static void Assemble_sdiv(AARCH64Assembler* assembler) {
  AssembleDivide(assembler, 0);
}

static void Assemble_udiv(AARCH64Assembler* assembler) {
  AssembleDivide(assembler, 1);
}

static void Assemble_smaddl(AARCH64Assembler* assembler) {
  AssembleDataProcessing3SourceLong(assembler, 1, 0, 0);
}

static void Assemble_smsubl(AARCH64Assembler* assembler) {
  AssembleDataProcessing3SourceLong(assembler, 1, 0, 1);
}

static void Assemble_umaddl(AARCH64Assembler* assembler) {
  AssembleDataProcessing3SourceLong(assembler, 5, 0, 0);
}

static void Assemble_umsubl(AARCH64Assembler* assembler) {
  AssembleDataProcessing3SourceLong(assembler, 5, 0, 1);
}

static void AssembleLongMulAlias(AARCH64Assembler* assembler, int op54, int negate) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (rd.width != kX || rn.width != kW || rm.width != kW) {
    AssemblerError(&ASM, "Invalid widening multiply register widths");
    return;
  }
  Register zero = ZeroReg(kX);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      (1 << 31) | (0x1b << 24) | (op54 << 21) |
      (rm.num << 16) | (negate << 15) | (zero.num << 10) |
      (rn.num << 5) | rd.num);
}

static void Assemble_smull(AARCH64Assembler* assembler) {
  AssembleLongMulAlias(assembler, 1, 0);
}

static void Assemble_smnegl(AARCH64Assembler* assembler) {
  AssembleLongMulAlias(assembler, 1, 1);
}

static void Assemble_umull(AARCH64Assembler* assembler) {
  AssembleLongMulAlias(assembler, 5, 0);
}

static void Assemble_umnegl(AARCH64Assembler* assembler) {
  AssembleLongMulAlias(assembler, 5, 1);
}

static void AssembleHighMul(AARCH64Assembler* assembler, int unsigned_mul) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (rd.width != kX || rn.width != kX || rm.width != kX) {
    AssemblerError(&ASM, "High multiply requires 64-bit registers");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      (1 << 31) | (0x1b << 24) | ((unsigned_mul ? 6 : 2) << 21) |
      (rm.num << 16) | (0x1f << 10) | (rn.num << 5) | rd.num);
}

static void Assemble_smulh(AARCH64Assembler* assembler) {
  AssembleHighMul(assembler, 0);
}

static void Assemble_umulh(AARCH64Assembler* assembler) {
  AssembleHighMul(assembler, 1);
}

static void AssembleBitFieldMove(AARCH64Assembler* assembler, int opc,
                                 Register* rd, Register* rn,
                                 int sf, int n, int immr, int imms) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (sf << 31) | (opc << 29) | (0x26 << 23) | (n << 22) |
                    ((immr & 0x3f) << 16) | ((imms & 0x3f) << 10) |
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
  // Use a positive modulo: C's truncated % yields a negative value for a
  // positive shift, which only happens to encode correctly for the 64-bit
  // width once masked to 6 bits.
  int immr = (int)((max_shift + 1 - shift) % (max_shift + 1));
  int imms = (int)(max_shift - shift);
  AssembleBitFieldMove(assembler, 2, &rd, &rn, is_64bit, is_64bit, immr, imms);
}

static void Assemble_asr(AARCH64Assembler* assembler) {
  AssembleShiftRight(assembler, 0);
}

static void Assemble_lsr(AARCH64Assembler* assembler) {
  AssembleShiftRight(assembler, 2);
}


static void AssembleMove(AARCH64Assembler* assembler);

static bool GetImmediate(AARCH64Assembler* assembler, int64_t* value) {
  LexMatch(&ASM.lex, TOK(hash));
  *value = AssemblerEvaluateExpression(&ASM);
  return true;
}

static bool GetLsbWidth(AARCH64Assembler* assembler, int64_t* lsb,
                        int64_t* width) {
  if (!GetImmediate(assembler, lsb) || !NeedComma(assembler)) {
    return false;
  }
  return GetImmediate(assembler, width);
}

static void AssembleBitfieldAlias(AARCH64Assembler* assembler, int opc,
                                  bool insert_at_lsb) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rn) || !NeedComma(assembler)) {
    return;
  }
  int64_t lsb = 0;
  int64_t width = 0;
  if (!GetLsbWidth(assembler, &lsb, &width)) {
    return;
  }
  int64_t max = rd.width == kX ? 64 : 32;
  if (lsb < 0 || width <= 0 || lsb + width > max) {
    AssemblerError(&ASM, "Invalid bitfield range");
    return;
  }
  int immr = insert_at_lsb ? (int)((max - lsb) & (max - 1)) : (int)lsb;
  int imms = insert_at_lsb ? (int)(width - 1) : (int)(lsb + width - 1);
  AssembleBitFieldMove(assembler, opc, &rd, &rn, rd.width == kX,
                       rd.width == kX, immr, imms);
}

static void Assemble_bfi(AARCH64Assembler* assembler) {
  AssembleBitfieldAlias(assembler, 1, true);
}

static void Assemble_bfxil(AARCH64Assembler* assembler) {
  AssembleBitfieldAlias(assembler, 1, false);
}

static void Assemble_sbfiz(AARCH64Assembler* assembler) {
  AssembleBitfieldAlias(assembler, 0, true);
}

static void Assemble_ubfiz(AARCH64Assembler* assembler) {
  AssembleBitfieldAlias(assembler, 2, true);
}

static void Assemble_sbfx(AARCH64Assembler* assembler) {
  AssembleBitfieldAlias(assembler, 0, false);
}

static void Assemble_ubfx(AARCH64Assembler* assembler) {
  AssembleBitfieldAlias(assembler, 2, false);
}

static void AssembleExtendAlias(AARCH64Assembler* assembler, int opc,
                                int imms, bool wide_dest) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (wide_dest && rd.width != kX) {
    AssemblerError(&ASM, "Expected 64-bit destination");
    return;
  }
  int sf = rd.width == kX;
  int n = sf;
  AssembleBitFieldMove(assembler, opc, &rd, &rn, sf, n, 0, imms);
}

static void Assemble_sbxt(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 0, 31, true);
}

static void Assemble_sbxtb(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 0, 7, true);
}

static void Assemble_sbxth(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 0, 15, true);
}

static void Assemble_sxtb(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 0, 7, true);
}

static void Assemble_sxth(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 0, 15, true);
}

static void Assemble_sxtw(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 0, 31, true);
}

static void Assemble_ubxt(AARCH64Assembler* assembler) {
  AssembleMove(assembler);
}

static void Assemble_ubxtb(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 2, 7, false);
}

static void Assemble_ubxth(AARCH64Assembler* assembler) {
  AssembleExtendAlias(assembler, 2, 15, false);
}

static void AssembleDataProcessing1Source(AARCH64Assembler* assembler, int opcode) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (!CheckRegWidths(assembler, &rd, &rn)) {
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (0x2d6 << 21) |
      (opcode << 10) | (rn.num << 5) | rd.num);
}

static void Assemble_rbit(AARCH64Assembler* assembler) {
  AssembleDataProcessing1Source(assembler, 0);
}

static void Assemble_rev16(AARCH64Assembler* assembler) {
  AssembleDataProcessing1Source(assembler, 1);
}

static void Assemble_rev32(AARCH64Assembler* assembler) {
  AssembleDataProcessing1Source(assembler, 2);
}

static void Assemble_rev(AARCH64Assembler* assembler) {
  AssembleDataProcessing1Source(assembler, 3);
}

static void Assemble_clz(AARCH64Assembler* assembler) {
  AssembleDataProcessing1Source(assembler, 4);
}

static void Assemble_cls(AARCH64Assembler* assembler) {
  AssembleDataProcessing1Source(assembler, 5);
}

static void Assemble_extr(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  NeedComma(assembler);
  int64_t lsb = 0;
  GetImmediate(assembler, &lsb);
  if (!CheckRegWidths(assembler, &rd, &rn) ||
      !CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  int max = rd.width == kX ? 63 : 31;
  if (lsb < 0 || lsb > max) {
    AssemblerError(&ASM, "Invalid extract shift");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (0x13 << 24) |
      (1 << 23) | ((rd.width == kX) << 22) | (rm.num << 16) |
      ((int)lsb << 10) | (rn.num << 5) | rd.num);
}

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
      if (!CheckRegWidths(assembler, &rd, &op.reg)) {
        return;
      }
      // MOV to/from SP is the ADD-immediate alias.  Register 31 means XZR in
      // logical instructions, so encoding these forms as ORR would silently
      // read zero or discard the result.
      if (rd.is_sp || op.reg.is_sp) {
        AssembleAddSubImmediate(assembler, &rd, &op.reg, 0,
                                rd.width == kX, /*op=*/0, /*s=*/0,
                                /*shift=*/0);
        break;
      }
      // General-register MOV is the ORR alias with the zero register as Rn.
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

static void Assemble_asri(AARCH64Assembler* assembler) {
  Assemble_asr(assembler);
}

static COMPILER_UNUSED void Assemble_eons(AARCH64Assembler* assembler) {
  AssembleLogical(assembler, 3, 1, /*immed_ok=*/false, /*one_operand=*/false);
}

static void Assemble_mvn(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Operand rm = GetOperand(assembler);
  if (rm.type != kRegister) {
    AssemblerError(&ASM, "Expected register");
    return;
  }
  Register zero = ZeroReg(rd.width);
  if (!CheckRegWidths(assembler, &rd, &rm.reg)) {
    return;
  }
  AssembleLogicalShiftedRegister(
      assembler, &rd, &zero, &rm, rd.width == kX, 1, 1);
}

static void Assemble_ror(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  int64_t shift = 0;
  GetImmediate(assembler, &shift);
  if (!CheckRegWidths(assembler, &rd, &rn)) {
    return;
  }
  int max = rd.width == kX ? 63 : 31;
  if (shift < 0 || shift > max) {
    AssemblerError(&ASM, "Invalid rotate amount");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (0x13 << 24) |
      (1 << 23) | ((rd.width == kX) << 22) | (rn.num << 16) |
      ((int)shift << 10) | (rn.num << 5) | rd.num);
}

static bool GetCondition(AARCH64Assembler* assembler, Condition* cond) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Expected condition code");
    return false;
  }
#define COND(name) \
  if (StringEqualCaseBlind(&ASM.lex.spelling, #name)) { \
    *cond = kCond_##name; \
    LexNextToken(&ASM.lex); \
    return true; \
  }
  COND(eq);
  COND(ne);
  COND(cs);
  COND(hs);
  COND(cc);
  COND(lo);
  COND(mi);
  COND(pl);
  COND(vs);
  COND(vc);
  COND(hi);
  COND(ls);
  COND(ge);
  COND(lt);
  COND(gt);
  COND(le);
  COND(al);
#undef COND
  AssemblerError(&ASM, "Unknown condition code %s", ASM.lex.spelling.value);
  return false;
}

static Condition InvertCondition(Condition cond) {
  if (cond == kCond_al) {
    return cond;
  }
  return cond ^ 1;
}

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
                    (((offset >> 2) & 0x7ffff) << 5) |    // imm19.
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
  int32_t instruction_offset = (int32_t)AssemblerCurrentAddress(&ASM);
  bool known = false;
  int64_t addr = 0;
  AssemblerSymbol* sym = NULL;
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    sym = GetOrCreateSymbol(assembler, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    // A weak definition can be replaced by a strong definition from another
    // object. Leave its branch relocatable so the linker can retarget it.
    known = sym->defined && sym->section == ASM.current_section &&
            sym->binding != SYM_BIND(weak);
    addr = sym->value;
  } else {
    addr = AssemblerEvaluateKnownExpression(&ASM, &known);
  }
  int32_t offset = (int32_t)(addr - instruction_offset);

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
  if (!known) {
    if (sym != NULL) {
      int reloc_type = l ? R_AARCH64_CALL26 : R_AARCH64_JUMP26;
      if (l &&
          (sym->binding == SYM_BIND(global) ||
           sym->binding == SYM_BIND(weak)) &&
          assembler->base.pic) {
        reloc_type = R_AARCH64_CALL_PLT;
      }
      AssemblerRelocation* reloc = NewAssemblerRelocation(
          sym, reloc_type,
          ASM.current_section, instruction_offset, 0);
      AssemblerAddRelocation(&ASM, reloc);
    }
    offset = 0;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (l << 31) |
                    (0x5 << 26) |
                    ((offset >> 2) & 0x3ffffff));    // imm26.
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

static void Assemble_svc(AARCH64Assembler* assembler) {
  LexMatch(&ASM.lex, TOK(hash));
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  if (immediate < 0 || immediate > 0xffff) {
    AssemblerError(&ASM, "SVC immediate must be in the range 0..65535");
    return;
  }
  AssemblerEmitWord(&ASM, ASM.current_section,
                    0xd4000001u | ((uint32_t)immediate << 5));
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
  int imm19 = (offset >> 2) & 0x7ffff;
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
  
  int imm14 = (offset >> 2) & 0x3fff;
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

static void AssembleConditionalCompare(AARCH64Assembler* assembler, int op,
                                       bool force_immediate) {
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Operand op2 = GetOperand(assembler);
  NeedComma(assembler);
  LexMatch(&ASM.lex, TOK(hash));
  int64_t nzcv = AssemblerEvaluateExpression(&ASM);
  NeedComma(assembler);
  Condition cond;
  if (!GetCondition(assembler, &cond)) {
    return;
  }
  if (nzcv < 0 || nzcv > 15) {
    AssemblerError(&ASM, "Invalid NZCV value");
    return;
  }
  if (op2.type == kRegister && !force_immediate) {
    if (!CheckRegWidths(assembler, &rn, &op2.reg)) {
      return;
    }
    AssemblerEmitWord(
        &ASM, ASM.current_section,
        ((rn.width == kX) << 31) | (op << 30) | (0x1d2 << 21) |
        (op2.reg.num << 16) | (cond << 12) |
        (rn.num << 5) | (int)nzcv);
  } else if (op2.type == kIntImmediate) {
    if (op2.i < 0 || op2.i > 31) {
      AssemblerError(&ASM, "Invalid conditional compare immediate");
      return;
    }
    AssemblerEmitWord(
        &ASM, ASM.current_section,
        ((rn.width == kX) << 31) | (op << 30) | (0x1d2 << 21) |
        (1 << 11) | (op2.i << 16) | (cond << 12) |
        (rn.num << 5) | (int)nzcv);
  } else {
    AssemblerError(&ASM, "Invalid conditional compare operand");
  }
}

static void Assemble_ccmn(AARCH64Assembler* assembler) {
  AssembleConditionalCompare(assembler, 0, false);
}

static void Assemble_ccmni(AARCH64Assembler* assembler) {
  AssembleConditionalCompare(assembler, 0, true);
}

static void Assemble_ccmp(AARCH64Assembler* assembler) {
  AssembleConditionalCompare(assembler, 1, false);
}

static void Assemble_ccmpi(AARCH64Assembler* assembler) {
  AssembleConditionalCompare(assembler, 1, true);
}

static void AssembleConditionalSelect(AARCH64Assembler* assembler, int op,
                                      int op2, bool alias_one_reg,
                                      bool alias_zero, bool invert_cond) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn;
  Register rm;
  if (alias_zero) {
    rn = ZeroReg(rd.width);
    rm = ZeroReg(rd.width);
  } else {
    rn = GetRegister(assembler);
    if (!alias_one_reg) {
      NeedComma(assembler);
      rm = GetRegister(assembler);
    } else {
      rm = rn;
    }
  }
  if (!alias_zero) {
    NeedComma(assembler);
  }
  Condition cond;
  if (!GetCondition(assembler, &cond)) {
    return;
  }
  if (invert_cond) {
    cond = InvertCondition(cond);
  }
  if (!CheckRegWidths(assembler, &rd, &rn) ||
      !CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      ((rd.width == kX) << 31) | (op << 30) | (0xd4 << 21) |
      (rm.num << 16) | (cond << 12) | (op2 << 10) |
      (rn.num << 5) | rd.num);
}

static void Assemble_csel(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 0, 0, false, false, false);
}

static void Assemble_csinc(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 0, 1, false, false, false);
}

static void Assemble_csinv(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 1, 0, false, false, false);
}

static void Assemble_csneg(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 1, 1, false, false, false);
}

static void Assemble_cset(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 0, 1, false, true, true);
}

static void Assemble_csetm(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 1, 0, false, true, true);
}

static void Assemble_cinc(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 0, 1, true, false, true);
}

static void Assemble_cinv(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 1, 0, true, false, true);
}

static void Assemble_cneg(AARCH64Assembler* assembler) {
  AssembleConditionalSelect(assembler, 1, 1, true, false, true);
}

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
                                int opc, int v, int addr_mode,
                                int32_t imm9) {
  CheckImmediateWidth(assembler, imm9, 9);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (size << 30) |
                    (0x7 << 27) |
                    (fp << 26) |
                    (opc << 22) |
                    (v << 26) |
                    ((imm9 & 0x1ff) << 12) |
                    (addr_mode << 10) |
                    (rn->num << 5) |
                    (rt->num));
}

static void AssembleLoadStoreUnsignedImmediate(AARCH64Assembler* assembler,
                                               Register* rt, Register* rn,
                                               int size, int fp, int opc,
                                               int v, int32_t offset) {
  int scale = 1 << size;
  if ((offset & (scale - 1)) != 0) {
    AssemblerError(&ASM, "Invalid load/store offset alignment");
    return;
  }
  int imm12 = offset / scale;
  CheckImmediateWidth(assembler, imm12, 12);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (size << 30) |
                    (0x39 << 24) |
                    (fp << 26) |
                    (opc << 22) |
                    (v << 26) |
                    (imm12 << 10) |
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
                                  int opc, int v, int l, int mode,
                                  int32_t imm7) {
  AssemblerEmitWord(
      &ASM, ASM.current_section,
                    (opc << 30) |
                    (0x28 << 24) |
                    (v << 26) |
                    (mode << 23) |
                    (l << 22) |
                    ((imm7 & 0x7f) << 15) |
                    (rt2->num << 10) |
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
    offset.type = kIntImmediate;
    offset.i = 0;
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
      if (offset.type != kIntImmediate) {
        AssemblerError(&ASM, "LDP/STP require an immediate offset");
        return;
      }
      int scale = rt.width == kX ? 8 : 4;
      if ((offset.i % scale) != 0) {
        AssemblerError(&ASM, "LDP/STP offset must be naturally aligned");
        return;
      }
      int imm7 = offset.i / scale;
      if (imm7 < -64 || imm7 > 63) {
        AssemblerError(&ASM, "LDP/STP offset is out of range");
        return;
      }
      // bits[24:23]: 1 = post-index, 2 = signed offset, 3 = pre-index.
      int mode = post_indexed ? 1 : (writeback ? 3 : 2);
      AssembleLoadStorePair(assembler, &rt, &rt2, &rn,
                            rt.width == kX ? 2 : 0, 0, is_load, mode, imm7);
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
          if (size < 0 && (rt.width == kX || rt.width == kW || rt.fp_or_simd)) {
            size = rt.size;
          }
          // TODO: extended register variant.
          AssembleLoadStoreRegister(assembler, &rt, &rn, &offset.reg, size, opc, 0, option, s);
        }
      } else if (offset.type == kIntImmediate){
        // Immediate offset.
        int opc = is_signed && is_load ? (rt.width == kX ? 2 : 3) : is_load;
        if (size < 0 && (rt.width == kX || rt.width == kW || rt.fp_or_simd)) {
          size = rt.size;
        }
        int scale = 1 << (size < 0 ? 0 : size);
        bool aligned = (offset.i & (scale - 1)) == 0;
        if (!post_indexed && !writeback && offset.i >= 0 && aligned) {
          AssembleLoadStoreUnsignedImmediate(assembler, &rt, &rn, size,
                                             rt.fp_or_simd, opc, 0, offset.i);
        } else {
          // Misaligned (or negative/indexed) offsets use the unscaled
          // LDUR/STUR form, which permits any byte offset in [-256, 255].
          int imm9 = offset.i;
          int mode = post_indexed ? 1 : (writeback ? 3 : 0);
          AssembleLoadStoreImmediate(assembler, &rt, &rn, size,
                                     rt.fp_or_simd, opc, 0, mode, imm9);
        }
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

static void Assemble_ldrb(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 0, 0, false);
}

static void Assemble_ldrh(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 1, 0, false);
}

static void Assemble_ldrsb(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 0, 1, false);
}

static void Assemble_ldrsh(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 1, 1, false);
}

static void Assemble_strb(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, 0, 0, false);
}

static void Assemble_strh(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, 1, 0, false);
}

static void Assemble_ldr(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, -1, 0, false);
}

static void Assemble_str(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, -1, 0, false);
}

static Register GetAtomicAddress(AARCH64Assembler* assembler) {
  Register bad = {.width = kBAD};
  if (!LexMatch(&ASM.lex, TOK(lsquare))) {
    AssemblerError(&ASM, "Expected [");
    return bad;
  }
  Register rn = GetRegister(assembler);
  if (rn.fp_or_simd || rn.width != kX) {
    AssemblerError(&ASM, "Atomic base must be an X register");
    return bad;
  }
  if (!LexMatch(&ASM.lex, TOK(rsquare))) {
    AssemblerError(&ASM, "Expected ]");
    return bad;
  }
  return rn;
}

static void AssembleLoadExclusive(AARCH64Assembler* assembler, bool acquire,
                                  int forced_size) {
  Register rt = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetAtomicAddress(assembler);
  if (rt.fp_or_simd || (rt.width != kW && rt.width != kX) ||
      rn.width == kBAD) {
    AssemblerError(&ASM, "Invalid exclusive load registers");
    return;
  }
  int size = forced_size >= 0 ? forced_size : (rt.width == kX ? 3 : 2);
  if (forced_size >= 0 && forced_size < 2 && rt.width != kW) {
    AssemblerError(&ASM, "Byte/halfword exclusive load requires W register");
    return;
  }
  uint32_t word = ((uint32_t)size << 30) | 0x085f7c00u |
                  (acquire ? 0x00008000u : 0) |
                  ((uint32_t)rn.num << 5) | (uint32_t)rt.num;
  AssemblerEmitWord(&ASM, ASM.current_section, word);
}

static void AssembleStoreExclusive(AARCH64Assembler* assembler, bool release,
                                   int forced_size) {
  Register rs = GetRegister(assembler);
  NeedComma(assembler);
  Register rt = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetAtomicAddress(assembler);
  if (rs.fp_or_simd || rs.width != kW || rt.fp_or_simd ||
      (rt.width != kW && rt.width != kX) || rn.width == kBAD) {
    AssemblerError(&ASM, "Invalid exclusive store registers");
    return;
  }
  int size = forced_size >= 0 ? forced_size : (rt.width == kX ? 3 : 2);
  if (forced_size >= 0 && forced_size < 2 && rt.width != kW) {
    AssemblerError(&ASM, "Byte/halfword exclusive store requires W register");
    return;
  }
  uint32_t word = ((uint32_t)size << 30) | 0x08007c00u |
                  (release ? 0x00008000u : 0) |
                  ((uint32_t)rs.num << 16) |
                  ((uint32_t)rn.num << 5) | (uint32_t)rt.num;
  AssemblerEmitWord(&ASM, ASM.current_section, word);
}

static void AssembleAcquireRelease(AARCH64Assembler* assembler, bool load,
                                   int forced_size) {
  Register rt = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetAtomicAddress(assembler);
  if (rt.fp_or_simd || (rt.width != kW && rt.width != kX) ||
      rn.width == kBAD) {
    AssemblerError(&ASM, "Invalid acquire/release registers");
    return;
  }
  int size = forced_size >= 0 ? forced_size : (rt.width == kX ? 3 : 2);
  if (forced_size >= 0 && forced_size < 2 && rt.width != kW) {
    AssemblerError(&ASM, "Byte/halfword acquire/release requires W register");
    return;
  }
  uint32_t base = load ? 0x08dffc00u : 0x089ffc00u;
  uint32_t word = ((uint32_t)size << 30) | base |
                  ((uint32_t)rn.num << 5) | (uint32_t)rt.num;
  AssemblerEmitWord(&ASM, ASM.current_section, word);
}

static void Assemble_ldxr(AARCH64Assembler* assembler) {
  AssembleLoadExclusive(assembler, false, -1);
}
static void Assemble_ldxrb(AARCH64Assembler* assembler) {
  AssembleLoadExclusive(assembler, false, 0);
}
static void Assemble_ldxrh(AARCH64Assembler* assembler) {
  AssembleLoadExclusive(assembler, false, 1);
}
static void Assemble_ldaxr(AARCH64Assembler* assembler) {
  AssembleLoadExclusive(assembler, true, -1);
}
static void Assemble_ldaxrb(AARCH64Assembler* assembler) {
  AssembleLoadExclusive(assembler, true, 0);
}
static void Assemble_ldaxrh(AARCH64Assembler* assembler) {
  AssembleLoadExclusive(assembler, true, 1);
}
static void Assemble_stxr(AARCH64Assembler* assembler) {
  AssembleStoreExclusive(assembler, false, -1);
}
static void Assemble_stxrb(AARCH64Assembler* assembler) {
  AssembleStoreExclusive(assembler, false, 0);
}
static void Assemble_stxrh(AARCH64Assembler* assembler) {
  AssembleStoreExclusive(assembler, false, 1);
}
static void Assemble_stlxr(AARCH64Assembler* assembler) {
  AssembleStoreExclusive(assembler, true, -1);
}
static void Assemble_stlxrb(AARCH64Assembler* assembler) {
  AssembleStoreExclusive(assembler, true, 0);
}
static void Assemble_stlxrh(AARCH64Assembler* assembler) {
  AssembleStoreExclusive(assembler, true, 1);
}
static void Assemble_ldar(AARCH64Assembler* assembler) {
  AssembleAcquireRelease(assembler, true, -1);
}
static void Assemble_ldarb(AARCH64Assembler* assembler) {
  AssembleAcquireRelease(assembler, true, 0);
}
static void Assemble_ldarh(AARCH64Assembler* assembler) {
  AssembleAcquireRelease(assembler, true, 1);
}
static void Assemble_stlr(AARCH64Assembler* assembler) {
  AssembleAcquireRelease(assembler, false, -1);
}
static void Assemble_stlrb(AARCH64Assembler* assembler) {
  AssembleAcquireRelease(assembler, false, 0);
}
static void Assemble_stlrh(AARCH64Assembler* assembler) {
  AssembleAcquireRelease(assembler, false, 1);
}

static void Assemble_dmb(AARCH64Assembler* assembler) {
  int option = 0xf;  // sy
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    if (StringEqualCaseBlind(&ASM.lex.spelling, "ish")) {
      option = 0xb;
    } else if (StringEqualCaseBlind(&ASM.lex.spelling, "ishld")) {
      option = 0x9;
    } else if (StringEqualCaseBlind(&ASM.lex.spelling, "ishst")) {
      option = 0xa;
    } else if (!StringEqualCaseBlind(&ASM.lex.spelling, "sy")) {
      AssemblerError(&ASM, "Unsupported DMB option");
      return;
    }
    LexNextToken(&ASM.lex);
  }
  AssemblerEmitWord(&ASM, ASM.current_section,
                    0xd50330bfu | ((uint32_t)option << 8));
}

static void Assemble_clrex(AARCH64Assembler* assembler) {
  AssemblerEmitWord(&ASM, ASM.current_section, 0xd5033f5fu);
}

static void Assemble_mrs(AARCH64Assembler* assembler) {
  Register rt = GetRegister(assembler);
  if (rt.width != kX || rt.is_sp) {
    AssemblerError(&ASM, "MRS destination must be an X register");
    return;
  }
  if (!NeedComma(assembler)) {
    return;
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier)) ||
      !StringEqualCaseBlind(&ASM.lex.spelling, "tpidr_el0")) {
    AssemblerError(&ASM, "Only TPIDR_EL0 is supported by MRS");
    return;
  }
  LexNextToken(&ASM.lex);
  AssemblerEmitWord(&ASM, ASM.current_section,
                    0xd53bd040u | (uint32_t)rt.num);
}

static void Assemble_ldp(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, 0, 0, true);
}

static void Assemble_stp(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, 0, 0, true);
}

static void Assemble_ldpsw(AARCH64Assembler* assembler) {
  Register rt = GetRegister(assembler);
  NeedComma(assembler);
  Register rt2 = GetRegister(assembler);
  NeedComma(assembler);
  if (!LexMatch(&ASM.lex, TOK(lsquare))) {
    AssemblerError(&ASM, "Expected [");
    return;
  }
  Register rn = GetRegister(assembler);
  int32_t offset = 0;
  if (LexMatch(&ASM.lex, TOK(comma))) {
    Operand op = GetOperand(assembler);
    if (op.type != kIntImmediate) {
      AssemblerError(&ASM, "Expected immediate offset");
      return;
    }
    offset = op.i;
  }
  if (!LexMatch(&ASM.lex, TOK(rsquare))) {
    AssemblerError(&ASM, "Expected ]");
    return;
  }
  int imm7 = offset / 4;
  CheckImmediateWidth(assembler, imm7, 7);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x69400000 | ((imm7 & 0x7f) << 15) |
      (rt2.num << 10) | (rn.num << 5) | rt.num);
}

static void AssembleUnscaledLoadStore(AARCH64Assembler* assembler, int is_load,
                                      int size, int is_signed) {
  Register rt = GetRegister(assembler);
  NeedComma(assembler);
  if (!LexMatch(&ASM.lex, TOK(lsquare))) {
    AssemblerError(&ASM, "Expected [");
    return;
  }
  Register rn = GetRegister(assembler);
  if (rn.fp_or_simd) {
    AssemblerError(&ASM, "Base must be an integer register");
    return;
  }
  int32_t imm9 = 0;
  if (LexMatch(&ASM.lex, TOK(comma))) {
    Operand offset = GetOperand(assembler);
    if (offset.type != kIntImmediate) {
      AssemblerError(&ASM, "Expected immediate offset");
      return;
    }
    imm9 = offset.i;
  }
  if (!LexMatch(&ASM.lex, TOK(rsquare))) {
    AssemblerError(&ASM, "Expected ]");
    return;
  }
  if (size < 0 && (rt.width == kX || rt.width == kW || rt.fp_or_simd)) {
    size = rt.size;
  }
  int opc = is_signed && is_load ? (rt.width == kX ? 2 : 3) : is_load;
  CheckImmediateWidth(assembler, imm9, 9);
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      (size << 30) | (0x7 << 27) | (rt.fp_or_simd << 26) |
      (opc << 22) | ((imm9 & 0x1ff) << 12) |
      (rn.num << 5) | rt.num);
}

static void Assemble_ldur(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 1, -1, 0);
}

static void Assemble_ldurb(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 1, 0, 0);
}

static void Assemble_ldurh(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 1, 1, 0);
}

static void Assemble_ldursb(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 1, 0, 1);
}

static void Assemble_ldursh(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 1, 1, 1);
}

static void Assemble_ldursw(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 1, 2, 1);
}

static void Assemble_prfm(AARCH64Assembler* assembler) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    LexNextToken(&ASM.lex);
  } else {
    GetOperand(assembler);
  }
  NeedComma(assembler);
  if (!LexMatch(&ASM.lex, TOK(lsquare))) {
    AssemblerError(&ASM, "Expected [");
    return;
  }
  Register rn = GetRegister(assembler);
  int32_t offset = 0;
  if (LexMatch(&ASM.lex, TOK(comma))) {
    Operand op = GetOperand(assembler);
    if (op.type != kIntImmediate) {
      AssemblerError(&ASM, "Expected immediate offset");
      return;
    }
    offset = op.i;
  }
  if (!LexMatch(&ASM.lex, TOK(rsquare))) {
    AssemblerError(&ASM, "Expected ]");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0xf9800000 | (((offset / 8) & 0xfff) << 10) | (rn.num << 5));
}

static void Assemble_stur(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 0, -1, 0);
}

static void Assemble_sturb(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 0, 0, 0);
}

static void Assemble_sturh(AARCH64Assembler* assembler) {
  AssembleUnscaledLoadStore(assembler, 0, 1, 0);
}

static void Assemble_fldr(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 1, -1, 0, false);
}

static void Assemble_fstr(AARCH64Assembler* assembler) {
  AssembleLoadStore(assembler, 0, -1, 0, false);
}

static void AssembleFPDataProcessing2(AARCH64Assembler* assembler, int opcode) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (!rd.fp_or_simd || !rn.fp_or_simd || !rm.fp_or_simd) {
    AssemblerError(&ASM, "Expected floating point registers");
    return;
  }
  if (!CheckRegWidths(assembler, &rd, &rn) ||
      !CheckRegWidths(assembler, &rd, &rm)) {
    return;
  }
  int ftype = rd.width == kD;
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x1e200000 | (ftype << 22) | (opcode << 10) |
      (rm.num << 16) | (rn.num << 5) | rd.num);
}

static void Assemble_fadd(AARCH64Assembler* assembler) {
  AssembleFPDataProcessing2(assembler, 0xa);
}

static void Assemble_fsub(AARCH64Assembler* assembler) {
  AssembleFPDataProcessing2(assembler, 0xe);
}

static void Assemble_fmul(AARCH64Assembler* assembler) {
  AssembleFPDataProcessing2(assembler, 0x2);
}

static void Assemble_fdiv(AARCH64Assembler* assembler) {
  AssembleFPDataProcessing2(assembler, 0x6);
}

static void Assemble_fsqrt(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (!rd.fp_or_simd || !rn.fp_or_simd ||
      !CheckRegWidths(assembler, &rd, &rn)) {
    AssemblerError(&ASM, "Expected matching floating point registers");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x1e21c000 | ((rd.width == kD) << 22) | (rn.num << 5) | rd.num);
}

static void Assemble_fmin(AARCH64Assembler* assembler) {
  AssembleFPDataProcessing2(assembler, 0x16);
}

static void Assemble_fmax(AARCH64Assembler* assembler) {
  AssembleFPDataProcessing2(assembler, 0x12);
}

static void AssembleFPIntBitcast(AARCH64Assembler* assembler, Register* rd,
                                 Register* rn) {
  bool is_64bit = rd->width == kD || rd->width == kX;
  uint32_t base;
  if (rd->fp_or_simd) {
    base = is_64bit ? 0x9e670000 : 0x1e270000;
  } else {
    base = is_64bit ? 0x9e660000 : 0x1e260000;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      base | (rn->num << 5) | rd->num);
}

static void AssembleFPConvertPrecision(AARCH64Assembler* assembler,
                                       RegisterWidth src_width,
                                       RegisterWidth dest_width) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (rd.fp_or_simd != rn.fp_or_simd) {
    AssembleFPIntBitcast(assembler, &rd, &rn);
    return;
  }
  if (!rd.fp_or_simd || !rn.fp_or_simd ||
      rd.width != dest_width || rn.width != src_width) {
    AssemblerError(&ASM, "Invalid floating point conversion registers");
    return;
  }
  int ftype = src_width == kD;
  int opc = dest_width == kD ? 1 : 0;
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x1e224000 | (ftype << 22) | (opc << 15) |
      (rn.num << 5) | rd.num);
}

static void Assemble_fcvtsd(AARCH64Assembler* assembler) {
  AssembleFPConvertPrecision(assembler, kS, kD);
}

static void Assemble_fcvtds(AARCH64Assembler* assembler) {
  AssembleFPConvertPrecision(assembler, kD, kS);
}

static void AssembleFPToInt(AARCH64Assembler* assembler, uint32_t base,
                            int unsigned_convert) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (rd.fp_or_simd || !rn.fp_or_simd) {
    AssemblerError(&ASM, "Expected integer destination and FP source");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      base | ((rd.width == kX) << 31) | ((rn.width == kD) << 22) |
      (unsigned_convert << 16) | (rn.num << 5) | rd.num);
}

static void Assemble_fcvtns(AARCH64Assembler* assembler) {
  AssembleFPToInt(assembler, 0x1e200000, 0);
}

static void Assemble_fcvtnu(AARCH64Assembler* assembler) {
  AssembleFPToInt(assembler, 0x1e200000, 1);
}

static void Assemble_fcvtzs(AARCH64Assembler* assembler) {
  AssembleFPToInt(assembler, 0x1e380000, 0);
}

static void Assemble_fcvtzu(AARCH64Assembler* assembler) {
  AssembleFPToInt(assembler, 0x1e380000, 1);
}

static void Assemble_fcvt(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (rd.fp_or_simd == rn.fp_or_simd) {
    AssemblerError(&ASM, "Expected one floating point and one integer register");
    return;
  }
  AssembleFPIntBitcast(assembler, &rd, &rn);
}

static void Assemble_fmov(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (rd.fp_or_simd != rn.fp_or_simd) {
    AssembleFPIntBitcast(assembler, &rd, &rn);
    return;
  }
  if (!rd.fp_or_simd || !CheckRegWidths(assembler, &rd, &rn)) {
    AssemblerError(&ASM, "Expected matching floating point registers");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x1e204000 | ((rd.width == kD) << 22) | (rn.num << 5) | rd.num);
}

static void Assemble_fcmp(AARCH64Assembler* assembler) {
  Register rn = GetRegister(assembler);
  NeedComma(assembler);
  Register rm = GetRegister(assembler);
  if (!rn.fp_or_simd || !rm.fp_or_simd) {
    AssemblerError(&ASM, "Expected floating point registers");
    return;
  }
  if (!CheckRegWidths(assembler, &rn, &rm)) {
    return;
  }
  int ftype = rn.width == kD;
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x1e202000 | (ftype << 22) | (rm.num << 16) | (rn.num << 5));
}

static void AssembleIntToFP(AARCH64Assembler* assembler, int unsigned_convert) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (!rd.fp_or_simd || rn.fp_or_simd) {
    AssemblerError(&ASM, "Expected FP destination and integer source");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x1e220000 | ((rn.width == kX) << 31) | ((rd.width == kD) << 22) |
      (unsigned_convert << 16) | (rn.num << 5) | rd.num);
}

static void Assemble_scvtf(AARCH64Assembler* assembler) {
  AssembleIntToFP(assembler, 0);
}

static void Assemble_ucvtf(AARCH64Assembler* assembler) {
  AssembleIntToFP(assembler, 1);
}

static void Assemble_fneg(AARCH64Assembler* assembler) {
  Register rd = GetRegister(assembler);
  NeedComma(assembler);
  Register rn = GetRegister(assembler);
  if (!rd.fp_or_simd || !rn.fp_or_simd ||
      !CheckRegWidths(assembler, &rd, &rn)) {
    AssemblerError(&ASM, "Expected matching floating point registers");
    return;
  }
  AssemblerEmitWord(
      &ASM, ASM.current_section,
      0x1e214000 | ((rd.width == kD) << 22) | (rn.num << 5) | rd.num);
}

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
    R_AARCH64_TLSLE_ADD_TPREL_HI12,
    R_AARCH64_TLSLE_ADD_TPREL_LO12_NC,
    R_AARCH64_TLS_TPREL,
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
