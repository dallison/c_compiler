//
//  arm_assembler.c
//  c_compiler_library
//
//  ARM32 (ARM mode, unified syntax) assembler.
//

#include "arm_assembler.h"
#include "arm_machine.h"
#include "elf.h"
#include "map.h"
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
  // An ARM data-processing immediate is an 8-bit value rotated *right* by
  // 2*rot4, where rot4 (0..15) lives in bits[11:8] and the 8-bit value in
  // bits[7:0].  To encode `value`, find rot4 such that
  //   value == imm8 ROR (2*rot4)   <=>   imm8 == value ROL (2*rot4)
  // fits in 8 bits.  (The previous implementation stored the rotate-right
  // count it used to shrink the value rather than rot4, and placed it at
  // bit 7 instead of bit 8 -- so e.g. #256 was encoded as 0xC0 = 192.)
  uint32_t v = (uint32_t)value;
  for (int rot4 = 0; rot4 < 16; rot4++) {
    uint32_t shift = (uint32_t)(2 * rot4);
    uint32_t imm8 = (shift == 0) ? v : ((v << shift) | (v >> (32 - shift)));
    if (imm8 <= 0xffu) {
      *encoded = imm8 | ((uint32_t)rot4 << 8);
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

static COMPILER_UNUSED uint32_t EncodeMul(int cond, int rd, int rn, int rs, int rm) {
  return ARM_COND(cond) | (0x9 << 4) | (rd << 16) | (rn << 12) | (rs << 8) | rm;
}

static COMPILER_UNUSED uint32_t EncodeLongMul(int cond, int rd_lo, int rd_hi,
                                              int rm, int rs, bool signed_mul) {
  return ARM_COND(cond) | (signed_mul ? 0 : (1 << 22)) | (0x9 << 4) |
         (rd_hi << 16) | (rd_lo << 12) | (rs << 8) | rm | (1 << 21);
}

static COMPILER_UNUSED uint32_t EncodeLoadStore(int cond, int p, int u, int b,
                                                int w, int l, int rn, int rd,
                                                int32_t offset) {
  return ARM_COND(cond) | (p << 24) | (u << 23) | (b << 22) | (w << 21) |
         (l << 20) | (rn << 16) | (rd << 12) | (offset & 0xfff);
}

static uint32_t EncodeBranch(int cond, int link, int32_t offset_words) {
  return ARM_COND(cond) | (0x5 << 25) | (link << 24) |
         (offset_words & 0xffffff);
}

static uint32_t EncodeBranchExchange(int cond, int rm, bool link) {
  // BX:  cond 0001 0010 1111 1111 1111 0001 Rm  (0x12fff1 << 4 | Rm)
  // BLX: cond 0001 0010 1111 1111 1111 0011 Rm  (0x12fff3 << 4 | Rm)
  return ARM_COND(cond) | ((link ? 0x12fff3 : 0x12fff1) << 4) | rm;
}

// Single-precision VFP register field placement.  A register number n is
// encoded as a 4-bit field holding n>>1 plus a single bit holding n&1.
static uint32_t EncVfpSd(int n) { return (((n >> 1) & 0xf) << 12) | ((n & 1) << 22); }
static uint32_t EncVfpSn(int n) { return (((n >> 1) & 0xf) << 16) | ((n & 1) << 7); }
static uint32_t EncVfpSm(int n) { return (((n >> 1) & 0xf) << 0) | ((n & 1) << 5); }
// Double-precision registers place their low 4 bits in the Vd/Vn/Vm field and
// the high bit in the D/N/M bit (the opposite split from single precision).
static uint32_t EncVfpDd(int n) { return ((n & 0xf) << 12) | (((n >> 4) & 1) << 22); }
static uint32_t EncVfpDn(int n) { return ((n & 0xf) << 16) | (((n >> 4) & 1) << 7); }
static uint32_t EncVfpDm(int n) { return ((n & 0xf) << 0) | (((n >> 4) & 1) << 5); }

static AssemblerSymbol* GetOrCreateSymbol(ARMAssembler* assembler,
                                          const char* name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(name, ASM.current_section, SYM_TYPE(none),
                             SYM_BIND(local),
                             0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  return sym;
}

static void EmitInst(ARMAssembler* assembler, uint32_t inst) {
  AssemblerEmitWord(&ASM, ASM.current_section, (int32_t)inst);
}

// Scratch register used to materialize immediates that cannot be encoded
// directly in a data-processing instruction.  r12 (ip) is the intra-procedure
// scratch register and is safe to clobber between calls.
#define ARM_SCRATCH_REG 12

// Load a 32-bit immediate into a register using movw (+movt for the high half).
static void LoadImmediateToReg(ARMAssembler* assembler, int reg, int32_t value) {
  uint32_t v = (uint32_t)value;
  uint32_t lo = v & 0xffff;
  uint32_t hi = (v >> 16) & 0xffff;
  // movw reg, #lo
  EmitInst(assembler, ARM_AL | (0x3 << 24) | (((lo >> 12) & 0xf) << 16) |
                          (reg << 12) | (lo & 0xfff));
  if (hi != 0) {
    // movt reg, #hi
    EmitInst(assembler, ARM_AL | (0x3 << 24) | (1 << 22) |
                            (((hi >> 12) & 0xf) << 16) | (reg << 12) |
                            (hi & 0xfff));
  }
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
  // -1 means "no index register"; an index of 0 is the valid register r0.
  op->index = -1;
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
      // Optional shifted-register operand: "rm, lsl #imm" (also lsr/asr/ror).
      // ParseOperand only ever parses the final operand of a data-processing
      // instruction, so a comma following the register can only introduce a
      // shift specifier.
      if (LexMatch(&ASM.lex, TOK(comma))) {
        const char* shift_names[] = {"lsl", "lsr", "asr", "ror"};
        int shift_type = 0;
        if (LexLookingAt(&ASM.lex, TOK(identifier))) {
          for (int i = 0; i < 4; i++) {
            if (strcmp(ASM.lex.spelling.value, shift_names[i]) == 0) {
              shift_type = i;
              break;
            }
          }
        }
        LexNextToken(&ASM.lex);  // consume shift mnemonic
        LexMatch(&ASM.lex, TOK(hash));
        op->shift_type = shift_type;
        op->shift_amount = (int)AssemblerEvaluateExpression(&ASM);
      }
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
      // An immediate displacement is written "#<expr>"; consume the optional '#'.
      LexMatch(&ASM.lex, TOK(hash));
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

static void AssembleDataProcCond(ARMAssembler* assembler, int cond, int opcode,
                                 bool set_flags) {
  ARMReg dst;
  ARMOp src;
  if (!ParseRegPair(assembler, &dst, &src)) {
    return;
  }
  // mov (0xd) and mvn (0xf) ignore Rn (it is SBZ); all others operate on Rd.
  bool no_rn = (opcode == 0xd || opcode == 0xf);
  if (src.kind == kARMOpReg) {
    int rn = no_rn ? 0 : dst.num;
    EmitInst(assembler, EncodeDataProcReg(cond, opcode, set_flags ? 1 : 0,
                                          rn, dst.num, src.reg.num,
                                          src.shift_type, src.shift_amount));
  } else if (src.kind == kARMOpImm) {
    int rn = no_rn ? 0 : dst.num;
    int enc_opcode = opcode;
    int32_t imm = src.imm;
    uint32_t enc = 0;
    if (!EncodeImmRotate(imm, &enc)) {
      // mov <-> mvn can encode an otherwise-impossible immediate via its
      // bitwise complement: "mov rd, #x" == "mvn rd, #~x".
      int swapped = opcode == 0xd ? 0xf : opcode == 0xf ? 0xd : opcode;
      if (swapped != opcode && EncodeImmRotate(~imm, &enc)) {
        enc_opcode = swapped;
        imm = ~imm;
      }
    }
    if (!EncodeImmRotate(imm, &enc)) {
      // mov of a non-encodable immediate: materialize it directly with
      // movw/movt into the destination register.
      if (opcode == 0xd) {
        LoadImmediateToReg(assembler, dst.num, src.imm);
        return;
      }
      // mvn (and other ops): load into scratch and use the register form.
      LoadImmediateToReg(assembler, ARM_SCRATCH_REG, src.imm);
      EmitInst(assembler, EncodeDataProcReg(cond, opcode, set_flags ? 1 : 0, rn,
                                            dst.num, ARM_SCRATCH_REG, 0, 0));
      return;
    }
    uint32_t inst =
        EncodeDataProcImm(cond, enc_opcode, set_flags ? 1 : 0, rn,
                          dst.num, imm);
    if (inst == 0) {
      AssemblerError(&ASM, "Immediate out of range");
      return;
    }
    EmitInst(assembler, inst);
  }
}

static void AssembleDataProc(ARMAssembler* assembler, int opcode,
                             bool set_flags) {
  AssembleDataProcCond(assembler, ARM_COND_AL, opcode, set_flags);
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
                               dst.num, src.reg.num, src.shift_type,
                               src.shift_amount));
  } else if (src.kind == kARMOpImm) {
    int enc_opcode = opcode;
    int32_t imm = src.imm;
    uint32_t enc = 0;
    if (!EncodeImmRotate(imm, &enc)) {
      // A negative immediate often can't be rotate-encoded directly but its
      // negation can, by flipping add<->sub (and adc<->sbc).
      int swapped = opcode;
      bool complement = false;  // true: use ~imm; false: use -imm.
      switch (opcode) {
        case 0x4: swapped = 0x2; break;  // add  -> sub
        case 0x2: swapped = 0x4; break;  // sub  -> add
        case 0x5: swapped = 0x6; break;  // adc  -> sbc
        case 0x6: swapped = 0x5; break;  // sbc  -> adc
        case 0x0: swapped = 0xe; complement = true; break;  // and  -> bic
        case 0xe: swapped = 0x0; complement = true; break;  // bic  -> and
        default: break;
      }
      int32_t alt = complement ? ~imm : -imm;
      if (swapped != opcode && EncodeImmRotate(alt, &enc)) {
        enc_opcode = swapped;
        imm = alt;
      }
    }
    if (!EncodeImmRotate(imm, &enc)) {
      // The immediate cannot be rotate-encoded directly: load it into the
      // scratch register and use the register form of the instruction.
      LoadImmediateToReg(assembler, ARM_SCRATCH_REG, src.imm);
      EmitInst(assembler,
               EncodeDataProcReg(ARM_COND_AL, opcode, set_flags ? 1 : 0, rn.num,
                                 dst.num, ARM_SCRATCH_REG, 0, 0));
      return;
    }
    uint32_t inst =
        EncodeDataProcImm(ARM_COND_AL, enc_opcode, set_flags ? 1 : 0, rn.num,
                          dst.num, imm);
    if (inst == 0) {
      AssemblerError(&ASM, "Immediate out of range");
      return;
    }
    EmitInst(assembler, inst);
  }
}

// Comparison instructions (cmp/cmn/tst/teq) take two operands - a source
// register and an operand2 - and have no destination register (Rd is SBZ).
// They always set the condition flags.
static void AssembleCompare(ARMAssembler* assembler, int opcode) {
  ARMReg rn;
  if (!ParseRegister(assembler, &rn) || !ExpectComma(assembler)) {
    return;
  }
  ARMOp src;
  if (!ParseOperand(assembler, &src)) {
    return;
  }
  if (src.kind == kARMOpReg) {
    EmitInst(assembler,
             EncodeDataProcReg(ARM_COND_AL, opcode, 1, rn.num, 0, src.reg.num,
                               0, 0));
  } else if (src.kind == kARMOpImm) {
    int enc_opcode = opcode;
    int32_t imm = src.imm;
    uint32_t enc = 0;
    if (!EncodeImmRotate(imm, &enc)) {
      // "cmp rn, #-N" is equivalent to "cmn rn, #N" (and vice versa) since cmp
      // sets flags for rn-imm and cmn for rn+imm.
      int swapped = opcode == 0xa ? 0xb : opcode == 0xb ? 0xa : opcode;
      if (swapped != opcode && EncodeImmRotate(-imm, &enc)) {
        enc_opcode = swapped;
        imm = -imm;
      }
    }
    if (!EncodeImmRotate(imm, &enc)) {
      LoadImmediateToReg(assembler, ARM_SCRATCH_REG, src.imm);
      EmitInst(assembler, EncodeDataProcReg(ARM_COND_AL, opcode, 1, rn.num, 0,
                                            ARM_SCRATCH_REG, 0, 0));
      return;
    }
    uint32_t inst = EncodeDataProcImm(ARM_COND_AL, enc_opcode, 1, rn.num, 0, imm);
    if (inst == 0) {
      AssemblerError(&ASM, "Immediate out of range");
      return;
    }
    EmitInst(assembler, inst);
  }
}

static void Assemble_mov(ARMAssembler* assembler) { AssembleDataProc(assembler, 0xd, false); }
static void Assemble_moveq(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_EQ, 0xd, false); }
static void Assemble_movne(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_NE, 0xd, false); }
static void Assemble_movcs(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_CS, 0xd, false); }
static void Assemble_movhs(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_CS, 0xd, false); }
static void Assemble_movcc(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_CC, 0xd, false); }
static void Assemble_movlo(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_CC, 0xd, false); }
static void Assemble_movmi(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_MI, 0xd, false); }
static void Assemble_movpl(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_PL, 0xd, false); }
static void Assemble_movvs(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_VS, 0xd, false); }
static void Assemble_movvc(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_VC, 0xd, false); }
static void Assemble_movhi(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_HI, 0xd, false); }
static void Assemble_movls(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_LS, 0xd, false); }
static void Assemble_movge(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_GE, 0xd, false); }
static void Assemble_movlt(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_LT, 0xd, false); }
static void Assemble_movgt(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_GT, 0xd, false); }
static void Assemble_movle(ARMAssembler* a) { AssembleDataProcCond(a, ARM_COND_LE, 0xd, false); }
static void Assemble_mvn(ARMAssembler* assembler) { AssembleDataProc(assembler, 0xf, false); }
static void Assemble_neg(ARMAssembler* assembler) {
  // "neg rd, rm" is an alias for "rsb rd, rm, #0".
  ARMReg dst, src;
  if (!ParseRegister(assembler, &dst) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &src)) {
    return;
  }
  uint32_t inst = EncodeDataProcImm(ARM_COND_AL, 0x3, 0, src.num, dst.num, 0);
  EmitInst(assembler, inst);
}
static void Assemble_add(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x4, false); }
static void Assemble_adds(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x4, true); }
static void Assemble_sub(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x2, false); }
static void Assemble_subs(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x2, true); }
static void Assemble_rsb(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x3, false); }
static void Assemble_adc(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x5, false); }
static void Assemble_sbc(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x6, false); }
static void Assemble_sbcs(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x6, true); }
static void Assemble_and(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x0, false); }
static void Assemble_orr(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0xc, false); }
static void Assemble_orrs(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0xc, true); }
static void Assemble_eor(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0x1, false); }
static void Assemble_bic(ARMAssembler* assembler) { AssembleDataProc3(assembler, 0xe, false); }
static void Assemble_cmp(ARMAssembler* assembler) { AssembleCompare(assembler, 0xa); }
static void Assemble_cmn(ARMAssembler* assembler) { AssembleCompare(assembler, 0xb); }
static void Assemble_tst(ARMAssembler* assembler) { AssembleCompare(assembler, 0x8); }
static void Assemble_teq(ARMAssembler* assembler) { AssembleCompare(assembler, 0x9); }

static void Assemble_mul(ARMAssembler* assembler) {
  // mul rd, rn, rm  ->  rd = rn * rm.
  // Encoding: cond 0000000 0 Rd(19:16) 0000(15:12) Rm(11:8) 1001 Rn(3:0).
  ARMReg dst, rn, rm;
  if (!ParseRegister(assembler, &dst) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rn) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rm)) {
    return;
  }
  EmitInst(assembler, ARM_COND(ARM_COND_AL) | (dst.num << 16) | (rm.num << 8) |
                          (0x9 << 4) | rn.num);
}

// mls rd, rn, rm, ra  ->  rd = ra - rn * rm.
// Encoding: cond 0000 0110 Rd(19:16) Ra(15:12) Rm(11:8) 1001 Rn(3:0).
static void Assemble_mls(ARMAssembler* assembler) {
  ARMReg rd, rn, rm, ra;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rn) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rm) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &ra)) {
    return;
  }
  EmitInst(assembler, ARM_COND(ARM_COND_AL) | (0x06 << 20) | (rd.num << 16) |
                          (ra.num << 12) | (rm.num << 8) | (0x9 << 4) | rn.num);
}

// Integer divide (ARMv7-A with the integer divide extension / interpreter).
//   udiv: cond 0111 0011 Rd(19:16) 1111 Rm(11:8) 0001 Rn(3:0)
//   sdiv: cond 0111 0001 Rd(19:16) 1111 Rm(11:8) 0001 Rn(3:0)
static void AssembleDivide(ARMAssembler* assembler, bool is_signed) {
  ARMReg rd, rn, rm;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rn) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rm)) {
    return;
  }
  uint32_t op = is_signed ? 0x71u : 0x73u;
  EmitInst(assembler, ARM_COND(ARM_COND_AL) | (op << 20) | (rd.num << 16) |
                          (0xf << 12) | (rm.num << 8) | (0x1 << 4) | rn.num);
}

static void Assemble_udiv(ARMAssembler* assembler) { AssembleDivide(assembler, false); }
static void Assemble_sdiv(ARMAssembler* assembler) { AssembleDivide(assembler, true); }

// Shifts are MOV with a shift applied:
//   <shift> rd, rm, #imm   ->  mov rd, rm, <shift> #imm
//   <shift> rd, rm, rs     ->  mov rd, rm, <shift> rs
// shift_type: LSL=0, LSR=1, ASR=2, ROR=3.
static void AssembleShift(ARMAssembler* assembler, int shift_type) {
  ARMReg rd, rm;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rm) || !ExpectComma(assembler)) {
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    ARMReg rs;
    if (!ParseRegister(assembler, &rs)) {
      return;
    }
    // Register-controlled shift: cond 0001 1010 0000 Rd Rs 0 type 1 Rm.
    EmitInst(assembler, ARM_COND(ARM_COND_AL) | (0xd << 21) | (rd.num << 12) |
                            (rs.num << 8) | (shift_type << 5) | (0x1 << 4) |
                            rm.num);
    return;
  }
  LexMatch(&ASM.lex, TOK(hash));
  int32_t amount = (int32_t)AssemblerEvaluateExpression(&ASM) & 0x1f;
  EmitInst(assembler, EncodeDataProcReg(ARM_COND_AL, 0xd, 0, 0, rd.num, rm.num,
                                        shift_type, amount));
}

static void Assemble_lsl(ARMAssembler* assembler) { AssembleShift(assembler, 0); }
static void Assemble_lsr(ARMAssembler* assembler) { AssembleShift(assembler, 1); }
static void Assemble_asr(ARMAssembler* assembler) { AssembleShift(assembler, 2); }
static void Assemble_ror(ARMAssembler* assembler) { AssembleShift(assembler, 3); }

static void AssembleBitUnary(ARMAssembler* assembler, uint32_t encoding) {
  ARMReg rd, rm;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rm)) {
    return;
  }
  EmitInst(assembler, ARM_COND(ARM_COND_AL) | encoding | (rd.num << 12) |
                          rm.num);
}

static void Assemble_clz(ARMAssembler* assembler) {
  AssembleBitUnary(assembler, 0x016f0f10);
}

static void Assemble_rbit(ARMAssembler* assembler) {
  AssembleBitUnary(assembler, 0x06ff0f30);
}

static void AssembleLoadStore(ARMAssembler* assembler, bool load, bool byte,
                              bool half, bool sign) {
  ARMReg rd;
  ARMOp addr;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &addr) || addr.kind != kARMOpMem) {
    AssemblerError(&ASM, "Expected register and memory operand");
    return;
  }
  if (addr.index >= 0) {
    AssemblerError(&ASM, "Indexed load/store not yet supported in emitter output");
    return;
  }
  // P=1 for offset and pre-indexed addressing; post-indexed (writeback without
  // an explicit '!') is not produced by the emitter.
  int p = (addr.preindex || !addr.writeback) ? 1 : 0;
  int w = addr.writeback ? 1 : 0;
  if (!p && w) {
    AssemblerError(&ASM, "Post-indexed writeback not supported");
    return;
  }
  int32_t off = addr.imm;
  // ARM encodes the offset magnitude plus a separate add/subtract (U) bit
  // rather than a signed displacement.
  int u = off >= 0 ? 1 : 0;
  uint32_t mag = (uint32_t)(off >= 0 ? off : -off);
  uint32_t inst;
  if (half || sign) {
    // Halfword/signed transfer: bits 7:4 are 1011 (H), 1101 (SB), or 1111
    // (SH).
    if (mag > 0xff) {
      AssemblerError(&ASM, "Halfword offset out of range");
      return;
    }
    uint32_t op = sign ? (half ? 0xfu : 0xdu) : 0xbu;
    inst = ARM_COND(ARM_COND_AL) | (p << 24) | (u << 23) | (1 << 22) |
           (w << 21) | ((load ? 1 : 0) << 20) | (addr.base << 16) |
           (rd.num << 12) | ((mag & 0xf0) << 4) | (op << 4) | (mag & 0xf);
  } else {
    // Word/byte single data transfer: cond 01 0 P U B W L Rn Rt imm12.
    if (mag > 0xfff) {
      AssemblerError(&ASM, "Load/store offset out of range");
      return;
    }
    inst = ARM_COND(ARM_COND_AL) | (1 << 26) | (p << 24) | (u << 23) |
           ((byte ? 1 : 0) << 22) | (w << 21) | ((load ? 1 : 0) << 20) |
           (addr.base << 16) | (rd.num << 12) | (mag & 0xfff);
  }
  EmitInst(assembler, inst);
}

static void Assemble_ldr(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, false, false, false); }
static void Assemble_str(ARMAssembler* assembler) { AssembleLoadStore(assembler, false, false, false, false); }
static void Assemble_ldrb(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, true, false, false); }
static void Assemble_strb(ARMAssembler* assembler) { AssembleLoadStore(assembler, false, true, false, false); }
static void Assemble_ldrh(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, false, true, false); }
static void Assemble_strh(ARMAssembler* assembler) { AssembleLoadStore(assembler, false, false, true, false); }
static void Assemble_ldrsb(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, false, false, true); }
static void Assemble_ldrsh(ARMAssembler* assembler) { AssembleLoadStore(assembler, true, false, true, true); }

static void AssembleLoadExclusive(ARMAssembler* assembler, int size) {
  ARMReg rt;
  ARMOp addr;
  if (!ParseRegister(assembler, &rt) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &addr) || addr.kind != kARMOpMem ||
      addr.index >= 0 || addr.imm != 0 || addr.writeback) {
    AssemblerError(&ASM, "Expected register and [base] operands");
    return;
  }
  uint32_t base =
      size == 0 ? 0xe1d00f9fu : size == 1 ? 0xe1f00f9fu : 0xe1900f9fu;
  EmitInst(assembler, base | ((uint32_t)addr.base << 16) |
                          ((uint32_t)rt.num << 12));
}

static void AssembleStoreExclusive(ARMAssembler* assembler, int size) {
  ARMReg rd;
  ARMReg rt;
  ARMOp addr;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rt) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &addr) || addr.kind != kARMOpMem ||
      addr.index >= 0 || addr.imm != 0 || addr.writeback) {
    AssemblerError(&ASM, "Expected status, value and [base] operands");
    return;
  }
  uint32_t base =
      size == 0 ? 0xe1c00f90u : size == 1 ? 0xe1e00f90u : 0xe1800f90u;
  EmitInst(assembler, base | ((uint32_t)addr.base << 16) |
                          ((uint32_t)rd.num << 12) | (uint32_t)rt.num);
}

static void Assemble_ldrex(ARMAssembler* assembler) {
  AssembleLoadExclusive(assembler, 2);
}
static void Assemble_ldrexb(ARMAssembler* assembler) {
  AssembleLoadExclusive(assembler, 0);
}
static void Assemble_ldrexh(ARMAssembler* assembler) {
  AssembleLoadExclusive(assembler, 1);
}
static void Assemble_strex(ARMAssembler* assembler) {
  AssembleStoreExclusive(assembler, 2);
}
static void Assemble_strexb(ARMAssembler* assembler) {
  AssembleStoreExclusive(assembler, 0);
}
static void Assemble_strexh(ARMAssembler* assembler) {
  AssembleStoreExclusive(assembler, 1);
}

static void Assemble_dmb(ARMAssembler* assembler) {
  int option = 0xf;
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    if (strcmp(ASM.lex.spelling.value, "ish") == 0) {
      option = 0xb;
    } else if (strcmp(ASM.lex.spelling.value, "ishst") == 0) {
      option = 0xa;
    } else if (strcmp(ASM.lex.spelling.value, "ishld") == 0) {
      option = 0x9;
    } else if (strcmp(ASM.lex.spelling.value, "sy") != 0) {
      AssemblerError(&ASM, "Unknown dmb option");
      return;
    }
    LexNextToken(&ASM.lex);
  }
  EmitInst(assembler, 0xf57ff050u | (uint32_t)option);
}

static void Assemble_clrex(ARMAssembler* assembler) {
  EmitInst(assembler, 0xf57ff01fu);
}

static bool ParseNamedOperand(ARMAssembler* assembler, const char* name) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier)) ||
      strcmp(ASM.lex.spelling.value, name) != 0) {
    AssemblerError(&ASM, "Expected %s", name);
    return false;
  }
  LexNextToken(&ASM.lex);
  return true;
}

static void Assemble_mrc(ARMAssembler* assembler) {
  if (!ParseNamedOperand(assembler, "p15") || !ExpectComma(assembler)) {
    return;
  }
  int64_t opc1 = AssemblerEvaluateExpression(&ASM);
  ARMReg rt;
  if (opc1 != 0 || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rt) || !ExpectComma(assembler) ||
      !ParseNamedOperand(assembler, "c13") || !ExpectComma(assembler) ||
      !ParseNamedOperand(assembler, "c0") || !ExpectComma(assembler)) {
    return;
  }
  int64_t opc2 = AssemblerEvaluateExpression(&ASM);
  if (opc2 != 3) {
    AssemblerError(&ASM, "Only mrc p15, 0, Rt, c13, c0, 3 is supported");
    return;
  }
  EmitInst(assembler, 0xee1d0f70u | ((uint32_t)rt.num << 12));
}

static void Assemble_tprel(ARMAssembler* assembler) {
  ARMReg rd;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Expected destination register and TLS symbol");
    return;
  }
  String symbol;
  StringInit(&symbol, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);

  // Load a standard R_ARM_TLS_LE32-relocated literal and branch over it.
  EmitInst(assembler, 0xe59f0000u | ((uint32_t)rd.num << 12));
  EmitInst(assembler, 0xea000000u);
  int32_t here = (int32_t)AssemblerCurrentAddress(&ASM);
  AssemblerRelocation* reloc =
      NewAssemblerRelocation(GetOrCreateSymbol(assembler, symbol.value),
                             R_ARM_TLS_LE32, ASM.current_section, here, 0);
  AssemblerAddRelocation(&ASM, reloc);
  EmitInst(assembler, 0);
  StringDestruct(&symbol);
}

static void AssembleBranch(ARMAssembler* assembler, int cond, bool link) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String sym;
    StringInit(&sym, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    int32_t here = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerSymbol* symbol = GetOrCreateSymbol(assembler, sym.value);
    int reloc_type = link ? R_ARM_CALL : R_ARM_JUMP24;
    if (ASM.pic && (symbol->binding == SYM_BIND(global) ||
                    symbol->binding == SYM_BIND(weak))) {
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
static void Assemble_bhs(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_CS, false); }
static void Assemble_bcs(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_CS, false); }
static void Assemble_blo(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_CC, false); }
static void Assemble_bcc(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_CC, false); }
static void Assemble_bhi(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_HI, false); }
static void Assemble_bls(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_LS, false); }
static void Assemble_bmi(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_MI, false); }
static void Assemble_bpl(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_PL, false); }
static void Assemble_bvs(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_VS, false); }
static void Assemble_bvc(ARMAssembler* assembler) { AssembleBranch(assembler, ARM_COND_VC, false); }

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

// Parse and encode a block data transfer:
//   <ldm/stm><addr-mode> Rn[!], {reglist}
// W (writeback) is set when the base is followed by '!'.
static void Assemble_block_transfer(ARMAssembler* assembler, bool load,
                                    bool preindex, bool add_offset) {
  ARMReg rn;
  if (!ParseRegister(assembler, &rn)) {
    return;
  }
  bool writeback = LexMatch(&ASM.lex, TOK(bang));
  if (!ExpectComma(assembler)) {
    return;
  }
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
  // cond=AL, block transfer (100), P/U select IA/IB/DA/DB.
  uint32_t insn = 0xE8000000u | ((uint32_t)rn.num << 16) | reglist;
  if (preindex) {
    insn |= 0x01000000u;  // P bit.
  }
  if (add_offset) {
    insn |= 0x00800000u;  // U bit.
  }
  if (load) {
    insn |= 0x00100000u;  // L bit.
  }
  if (writeback) {
    insn |= 0x00200000u;  // W bit.
  }
  EmitInst(assembler, insn);
}

static void Assemble_stmia(ARMAssembler* assembler) {
  Assemble_block_transfer(assembler, /*load=*/false, /*preindex=*/false,
                          /*add_offset=*/true);
}

static void Assemble_ldmia(ARMAssembler* assembler) {
  Assemble_block_transfer(assembler, /*load=*/true, /*preindex=*/false,
                          /*add_offset=*/true);
}

static void Assemble_stmdb(ARMAssembler* assembler) {
  Assemble_block_transfer(assembler, /*load=*/false, /*preindex=*/true,
                          /*add_offset=*/false);
}

static void Assemble_ldmdb(ARMAssembler* assembler) {
  Assemble_block_transfer(assembler, /*load=*/true, /*preindex=*/true,
                          /*add_offset=*/false);
}

static void Assemble_nop(ARMAssembler* assembler) {
  (void)assembler;
  EmitInst(assembler, ARM_AL | 0x0320f000);  // mov r0, r0
}

static void Assemble_svc(ARMAssembler* assembler) {
  LexMatch(&ASM.lex, TOK(hash));
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  if (immediate < 0 || immediate > 0xffffff) {
    AssemblerError(&ASM, "SVC immediate must be in the range 0..16777215");
    return;
  }
  EmitInst(assembler, ARM_AL | 0x0f000000u | (uint32_t)immediate);
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
  LexMatch(&ASM.lex, TOK(hash));
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String sym;
    StringInit(&sym, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    int32_t here = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerRelocation* reloc =
        NewAssemblerRelocation(GetOrCreateSymbol(assembler, sym.value),
                               R_ARM_MOVW_ABS_NC, ASM.current_section, here, 0);
    AssemblerAddRelocation(&ASM, reloc);
    EmitInst(assembler, ARM_AL | (0x3 << 24) | (rd.num << 12) | (0 << 4));
    StringDestruct(&sym);
    return;
  }
  int32_t value = (int32_t)AssemblerEvaluateExpression(&ASM);
  uint32_t imm16 = (uint32_t)value & 0xffff;
  uint32_t imm4 = ((uint32_t)value >> 12) & 0xf;
  EmitInst(assembler, ARM_AL | (0x3 << 24) | (imm4 << 16) | (rd.num << 12) |
                        (imm16 & 0xfff));
}

static void Assemble_movt(ARMAssembler* assembler) {
  ARMReg rd;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler)) {
    return;
  }
  LexMatch(&ASM.lex, TOK(hash));
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String sym;
    StringInit(&sym, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    int32_t here = (int32_t)AssemblerCurrentAddress(&ASM);
    AssemblerRelocation* reloc =
        NewAssemblerRelocation(GetOrCreateSymbol(assembler, sym.value),
                               R_ARM_MOVT_ABS, ASM.current_section, here, 0);
    AssemblerAddRelocation(&ASM, reloc);
    EmitInst(assembler, ARM_AL | (0x3 << 24) | (1 << 22) | (rd.num << 12) | (0 << 4));
    StringDestruct(&sym);
    return;
  }
  // "movt rd, #imm16" places the 16-bit immediate directly into bits[31:16] of
  // rd at runtime; the immediate operand IS that 16-bit value (the compiler has
  // already extracted the high half), so it is encoded the same way as movw's
  // immediate -- not shifted right by 16.
  int32_t value = (int32_t)AssemblerEvaluateExpression(&ASM);
  uint32_t imm16 = (uint32_t)value & 0xffff;
  uint32_t imm4 = ((uint32_t)value >> 12) & 0xf;
  EmitInst(assembler, ARM_AL | (0x3 << 24) | (1 << 22) | (imm4 << 16) |
                        (rd.num << 12) | (imm16 & 0xfff));
}

static bool IsDoubleReg(const ARMReg* r) { return r->type == kARMRegTypeFloatD; }

// vmov.f32 sd, sm  -- single-precision register copy.
static void Assemble_vmov_f32(ARMAssembler* assembler) {
  ARMReg dst, src;
  if (!ParseRegister(assembler, &dst) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &src)) {
    return;
  }
  if (IsDoubleReg(&dst)) {
    EmitInst(assembler, 0xeeb00b40 | EncVfpDd(dst.num) | EncVfpDm(src.num));
  } else {
    EmitInst(assembler, 0xeeb00a40 | EncVfpSd(dst.num) | EncVfpSm(src.num));
  }
}

// vmov with mixed register kinds:
//   vmov sn, rt        -- move the bits of GP register rt into single reg sn.
//   vmov rt, sn        -- move the bits of single reg sn into GP register rt.
//   vmov sd, sm        -- register copy (single or double precision).
//   vmov dm, rt, rt2   -- move two GP registers into a double FP register.
//   vmov rt, rt2, dm   -- move a double FP register into two GP registers.
static void Assemble_vmov(ARMAssembler* assembler) {
  ARMReg a, b;
  if (!ParseRegister(assembler, &a) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &b)) {
    return;
  }
  // Optional third operand: the 64-bit core<->double transfer forms.
  if (LexMatch(&ASM.lex, TOK(comma))) {
    ARMReg c;
    if (!ParseRegister(assembler, &c)) {
      return;
    }
    if (IsDoubleReg(&a) && b.type == kARMRegTypeInt && c.type == kARMRegTypeInt) {
      // vmov Dm, Rt, Rt2
      EmitInst(assembler, 0xec400b10 | EncVfpDm(a.num) | (b.num << 12) |
                              (c.num << 16));
    } else if (a.type == kARMRegTypeInt && b.type == kARMRegTypeInt &&
               IsDoubleReg(&c)) {
      // vmov Rt, Rt2, Dm
      EmitInst(assembler, 0xec500b10 | EncVfpDm(c.num) | (a.num << 12) |
                              (b.num << 16));
    } else {
      AssemblerError(&ASM, "Invalid vmov operands");
    }
    return;
  }
  bool a_float = a.type != kARMRegTypeInt;
  bool b_float = b.type != kARMRegTypeInt;
  if (a_float && b_float) {
    if (IsDoubleReg(&a)) {
      EmitInst(assembler, 0xeeb00b40 | EncVfpDd(a.num) | EncVfpDm(b.num));
    } else {
      EmitInst(assembler, 0xeeb00a40 | EncVfpSd(a.num) | EncVfpSm(b.num));
    }
  } else if (a_float && !b_float) {
    // vmov sn, rt
    EmitInst(assembler, 0xee000a10 | EncVfpSn(a.num) | (b.num << 12));
  } else if (!a_float && b_float) {
    // vmov rt, sn
    EmitInst(assembler, 0xee100a10 | EncVfpSn(b.num) | (a.num << 12));
  } else {
    AssemblerError(&ASM, "Invalid vmov operands");
  }
}

static void AssembleVfpMem(ARMAssembler* assembler, bool load) {
  ARMReg rd;
  ARMOp addr;
  if (!ParseRegister(assembler, &rd) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &addr) || addr.kind != kARMOpMem) {
    AssemblerError(&ASM, "Expected register and memory operand");
    return;
  }
  bool dbl = IsDoubleReg(&rd);
  int32_t off = addr.imm;
  int u = off >= 0 ? 1 : 0;
  uint32_t imm8 = (uint32_t)(off < 0 ? -off : off) / 4;
  uint32_t inst = 0xed000000u | ((uint32_t)u << 23) | (load ? (1u << 20) : 0u) |
                  ((uint32_t)addr.base << 16) | ((dbl ? 0xbu : 0xau) << 8) |
                  (imm8 & 0xff);
  inst |= dbl ? EncVfpDd(rd.num) : EncVfpSd(rd.num);
  EmitInst(assembler, inst);
}

static void Assemble_vldr(ARMAssembler* assembler) { AssembleVfpMem(assembler, true); }
static void Assemble_vstr(ARMAssembler* assembler) { AssembleVfpMem(assembler, false); }

// VFP data-processing: vadd/vsub/vmul/vdiv. Precision (sz bit 8) is taken from
// the operand register kind (sN vs dN).
static void AssembleVfpDataProc(ARMAssembler* assembler, uint32_t base) {
  ARMReg sd, sn, sm;
  if (!ParseRegister(assembler, &sd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &sn) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &sm)) {
    return;
  }
  if (IsDoubleReg(&sd)) {
    EmitInst(assembler, base | (1u << 8) | EncVfpDd(sd.num) | EncVfpDn(sn.num) |
                            EncVfpDm(sm.num));
  } else {
    EmitInst(assembler,
             base | EncVfpSd(sd.num) | EncVfpSn(sn.num) | EncVfpSm(sm.num));
  }
}

static void Assemble_vadd(ARMAssembler* a) { AssembleVfpDataProc(a, 0xee300a00); }
static void Assemble_vsub(ARMAssembler* a) { AssembleVfpDataProc(a, 0xee300a40); }
static void Assemble_vmul(ARMAssembler* a) { AssembleVfpDataProc(a, 0xee200a00); }
static void Assemble_vdiv(ARMAssembler* a) { AssembleVfpDataProc(a, 0xee800a00); }

// Single-operand VFP: vcmp/vneg/vabs/vsqrt sd, sm.
static void AssembleVfp2(ARMAssembler* assembler, uint32_t base) {
  ARMReg sd, sm;
  if (!ParseRegister(assembler, &sd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &sm)) {
    return;
  }
  if (IsDoubleReg(&sd)) {
    EmitInst(assembler, base | (1u << 8) | EncVfpDd(sd.num) | EncVfpDm(sm.num));
  } else {
    EmitInst(assembler, base | EncVfpSd(sd.num) | EncVfpSm(sm.num));
  }
}

static void Assemble_vcmp(ARMAssembler* a) { AssembleVfp2(a, 0xeeb40a40); }
static void Assemble_vneg(ARMAssembler* a) { AssembleVfp2(a, 0xeeb10a40); }
static void Assemble_fneg(ARMAssembler* a) { AssembleVfp2(a, 0xeeb10a40); }
static void Assemble_vabs(ARMAssembler* a) { AssembleVfp2(a, 0xeeb00ac0); }
static void Assemble_vsqrt(ARMAssembler* a) { AssembleVfp2(a, 0xeeb10ac0); }

// fcvtsd Dd, Sm : convert single-precision in Sm to double-precision in Dd
// (vcvt.f64.f32, op field 0x7, source single so bit 8 = 0).
static void Assemble_fcvtsd(ARMAssembler* a) {
  ARMReg dd, sm;
  if (!ParseRegister(a, &dd) || !ExpectComma(a) || !ParseRegister(a, &sm)) {
    return;
  }
  EmitInst(a, 0xeeb70a40 | EncVfpDd(dd.num) | EncVfpSm(sm.num));
}

// fcvtds Sd, Dm : convert double-precision in Dm to single-precision in Sd
// (vcvt.f32.f64, op field 0x7, source double so bit 8 = 1).
static void Assemble_fcvtds(ARMAssembler* a) {
  ARMReg sd, dm;
  if (!ParseRegister(a, &sd) || !ExpectComma(a) || !ParseRegister(a, &dm)) {
    return;
  }
  EmitInst(a, 0xeeb70b40 | EncVfpSd(sd.num) | EncVfpDm(dm.num));
}

static void Assemble_vmrs(ARMAssembler* assembler) {
  // vmrs APSR_nzcv, FPSCR -- consume the two fixed register operands.
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    LexNextToken(&ASM.lex);
  }
  LexMatch(&ASM.lex, TOK(comma));
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    LexNextToken(&ASM.lex);
  }
  EmitInst(assembler, 0xeef1fa10);
}

// scvtf sd, rt  -- convert signed int in GP register rt to float in sd.
// Custom encoding using coprocessor 11 (0xb) to distinguish from real VFP.
static void Assemble_scvtf(ARMAssembler* assembler) {
  ARMReg sd, rt;
  if (!ParseRegister(assembler, &sd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rt)) {
    return;
  }
  // Bit 7 (free in this custom cp11 encoding) marks a double-precision result.
  if (IsDoubleReg(&sd)) {
    EmitInst(assembler, 0xee000b10 | (1u << 7) | EncVfpDd(sd.num) | (rt.num << 16));
  } else {
    EmitInst(assembler, 0xee000b10 | EncVfpSd(sd.num) | (rt.num << 16));
  }
}

// ucvtf sd, rt  -- convert unsigned int to float (bit20 marks unsigned).
static void Assemble_ucvtf(ARMAssembler* assembler) {
  ARMReg sd, rt;
  if (!ParseRegister(assembler, &sd) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &rt)) {
    return;
  }
  if (IsDoubleReg(&sd)) {
    EmitInst(assembler, 0xee400b10 | (1u << 7) | EncVfpDd(sd.num) | (rt.num << 16));
  } else {
    EmitInst(assembler, 0xee400b10 | EncVfpSd(sd.num) | (rt.num << 16));
  }
}

// fcvtnu/fcvtns rt, sm -- convert float in sm to int in GP register rt.
static void Assemble_fcvtnu(ARMAssembler* assembler) {
  ARMReg rt, sm;
  if (!ParseRegister(assembler, &rt) || !ExpectComma(assembler) ||
      !ParseRegister(assembler, &sm)) {
    return;
  }
  if (IsDoubleReg(&sm)) {
    EmitInst(assembler, 0xee100b10 | (1u << 7) | (rt.num << 12) | EncVfpDm(sm.num));
  } else {
    EmitInst(assembler, 0xee100b10 | (rt.num << 12) | EncVfpSm(sm.num));
  }
}

static void Assemble_fcvtns(ARMAssembler* assembler) { Assemble_fcvtnu(assembler); }

#define DECLARE_INST_FUNC(mnemonic) static void Assemble_##mnemonic(ARMAssembler*)

DECLARE_INST_FUNC(mov);
DECLARE_INST_FUNC(moveq);
DECLARE_INST_FUNC(movne);
DECLARE_INST_FUNC(movcs);
DECLARE_INST_FUNC(movhs);
DECLARE_INST_FUNC(movcc);
DECLARE_INST_FUNC(movlo);
DECLARE_INST_FUNC(movmi);
DECLARE_INST_FUNC(movpl);
DECLARE_INST_FUNC(movvs);
DECLARE_INST_FUNC(movvc);
DECLARE_INST_FUNC(movhi);
DECLARE_INST_FUNC(movls);
DECLARE_INST_FUNC(movge);
DECLARE_INST_FUNC(movlt);
DECLARE_INST_FUNC(movgt);
DECLARE_INST_FUNC(movle);
DECLARE_INST_FUNC(mvn);
DECLARE_INST_FUNC(neg);
DECLARE_INST_FUNC(add);
DECLARE_INST_FUNC(adds);
DECLARE_INST_FUNC(sub);
DECLARE_INST_FUNC(subs);
DECLARE_INST_FUNC(rsb);
DECLARE_INST_FUNC(adc);
DECLARE_INST_FUNC(sbc);
DECLARE_INST_FUNC(sbcs);
DECLARE_INST_FUNC(and);
DECLARE_INST_FUNC(orr);
DECLARE_INST_FUNC(orrs);
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
DECLARE_INST_FUNC(ldrsb);
DECLARE_INST_FUNC(ldrsh);
DECLARE_INST_FUNC(ldrex);
DECLARE_INST_FUNC(ldrexb);
DECLARE_INST_FUNC(ldrexh);
DECLARE_INST_FUNC(strex);
DECLARE_INST_FUNC(strexb);
DECLARE_INST_FUNC(strexh);
DECLARE_INST_FUNC(dmb);
DECLARE_INST_FUNC(clrex);
DECLARE_INST_FUNC(mrc);
DECLARE_INST_FUNC(tprel);
DECLARE_INST_FUNC(b);
DECLARE_INST_FUNC(bl);
DECLARE_INST_FUNC(beq);
DECLARE_INST_FUNC(bne);
DECLARE_INST_FUNC(blt);
DECLARE_INST_FUNC(ble);
DECLARE_INST_FUNC(bgt);
DECLARE_INST_FUNC(bge);
DECLARE_INST_FUNC(bhs);
DECLARE_INST_FUNC(bcs);
DECLARE_INST_FUNC(blo);
DECLARE_INST_FUNC(bcc);
DECLARE_INST_FUNC(bhi);
DECLARE_INST_FUNC(bls);
DECLARE_INST_FUNC(bmi);
DECLARE_INST_FUNC(bpl);
DECLARE_INST_FUNC(bvs);
DECLARE_INST_FUNC(bvc);
DECLARE_INST_FUNC(mls);
DECLARE_INST_FUNC(udiv);
DECLARE_INST_FUNC(sdiv);
DECLARE_INST_FUNC(lsl);
DECLARE_INST_FUNC(lsr);
DECLARE_INST_FUNC(asr);
DECLARE_INST_FUNC(ror);
DECLARE_INST_FUNC(clz);
DECLARE_INST_FUNC(rbit);
DECLARE_INST_FUNC(bx);
DECLARE_INST_FUNC(blx);
DECLARE_INST_FUNC(push);
DECLARE_INST_FUNC(pop);
DECLARE_INST_FUNC(stmia);
DECLARE_INST_FUNC(ldmia);
DECLARE_INST_FUNC(stmdb);
DECLARE_INST_FUNC(ldmdb);
DECLARE_INST_FUNC(nop);
DECLARE_INST_FUNC(svc);
DECLARE_INST_FUNC(ret);
DECLARE_INST_FUNC(movw);
DECLARE_INST_FUNC(movt);
DECLARE_INST_FUNC(vmov_f32);
DECLARE_INST_FUNC(vmov);
DECLARE_INST_FUNC(vldr);
DECLARE_INST_FUNC(vstr);
DECLARE_INST_FUNC(vadd);
DECLARE_INST_FUNC(vsub);
DECLARE_INST_FUNC(vmul);
DECLARE_INST_FUNC(vdiv);
DECLARE_INST_FUNC(vcmp);
DECLARE_INST_FUNC(vneg);
DECLARE_INST_FUNC(fneg);
DECLARE_INST_FUNC(vabs);
DECLARE_INST_FUNC(vsqrt);
DECLARE_INST_FUNC(fcvtsd);
DECLARE_INST_FUNC(fcvtds);
DECLARE_INST_FUNC(vmrs);
DECLARE_INST_FUNC(scvtf);
DECLARE_INST_FUNC(ucvtf);
DECLARE_INST_FUNC(fcvtnu);
DECLARE_INST_FUNC(fcvtns);

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
  INST(moveq);
  INST(movne);
  INST(movcs);
  INST(movhs);
  INST(movcc);
  INST(movlo);
  INST(movmi);
  INST(movpl);
  INST(movvs);
  INST(movvc);
  INST(movhi);
  INST(movls);
  INST(movge);
  INST(movlt);
  INST(movgt);
  INST(movle);
  INST(mvn);
  INST(neg);
  INST(add);
  INST(adds);
  INST(sub);
  INST(subs);
  INST(rsb);
  INST(adc);
  INST(sbc);
  INST(sbcs);
  INST(and);
  INST(orr);
  INST(orrs);
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
  INST(ldrsb);
  INST(ldrsh);
  INST(ldrex);
  INST(ldrexb);
  INST(ldrexh);
  INST(strex);
  INST(strexb);
  INST(strexh);
  INST(dmb);
  INST(clrex);
  INST(mrc);
  INST(tprel);
  INST(b);
  INST(bl);
  INST(beq);
  INST(bne);
  INST(blt);
  INST(ble);
  INST(bgt);
  INST(bge);
  INST(bhs);
  INST(bcs);
  INST(blo);
  INST(bcc);
  INST(bhi);
  INST(bls);
  INST(bmi);
  INST(bpl);
  INST(bvs);
  INST(bvc);
  INST(mls);
  INST(udiv);
  INST(sdiv);
  INST(lsl);
  INST(lsr);
  INST(asr);
  INST(ror);
  INST(clz);
  INST(rbit);
  INST(bx);
  INST(blx);
  INST(push);
  INST(pop);
  INST(stmia);
  INST(ldmia);
  INST(stmdb);
  INST(ldmdb);
  INST2("stmfd", stmdb);
  INST2("ldmfd", ldmia);
  INST(nop);
  INST(svc);
  INST2("swi", svc);
  INST(ret);
  INST(movw);
  INST(movt);
  INST2("vmov.f32", vmov_f32);
  INST2("vmov.f64", vmov_f32);
  INST(vmov);
  INST(vldr);
  INST(vstr);
  INST(vadd);
  INST(vsub);
  INST(vmul);
  INST(vdiv);
  INST(vcmp);
  INST(vneg);
  INST(fneg);
  INST(vabs);
  INST(vsqrt);
  INST2("vadd.f32", vadd);
  INST2("vadd.f64", vadd);
  INST2("vsub.f32", vsub);
  INST2("vsub.f64", vsub);
  INST2("vmul.f32", vmul);
  INST2("vmul.f64", vmul);
  INST2("vdiv.f32", vdiv);
  INST2("vdiv.f64", vdiv);
  INST2("vcmp.f32", vcmp);
  INST2("vcmp.f64", vcmp);
  INST2("vneg.f32", vneg);
  INST2("vneg.f64", vneg);
  INST2("vabs.f32", vabs);
  INST2("vabs.f64", vabs);
  INST2("vsqrt.f32", vsqrt);
  INST2("vsqrt.f64", vsqrt);
  INST(fcvtsd);
  INST(fcvtds);
  INST(vmrs);
  INST(scvtf);
  INST(ucvtf);
  INST(fcvtnu);
  INST(fcvtns);
}

#undef INST
#undef INST2

bool ARMAssemblerInit(ARMAssembler* assembler, String* infile, String* outfile) {
  // Indexed by the RelocationType enum (assembler.h):
  //   kRelocSet16, kRelocSet32, kRelocSet64,
  //   kRelocAdd16, kRelocAdd32, kRelocAdd64,
  //   kRelocSub16, kRelocSub32, kRelocSub64.
  // These map data directives (.short/.word/.quad referencing a symbol) to the
  // ARM relocation that the linker applies as set (= S+A), add (+= S+A) or
  // subtract (-= S+A).  The previous table was a raw list of unrelated ARM
  // relocations, so a ".word symbol" (kRelocSet32) was emitted as R_ARM_REL32
  // (PC-relative) instead of R_ARM_ABS32, corrupting every global pointer
  // initializer (e.g. FILE* stdout = &s_stdout).  ARM has no 16-bit absolute
  // relocation, so kRelocSet16 uses R_ARM_ADD16, which acts as a set because
  // the in-place value of relocated data is zero (RELA carries the addend).
  static int reloc_types[] = {
      R_ARM_ADD16, R_ARM_ABS32, R_ARM_ABS32,
      R_ARM_ADD16, R_ARM_ADD32, R_ARM_ADD32,
      R_ARM_SUB16, R_ARM_SUB32, R_ARM_SUB32,
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
