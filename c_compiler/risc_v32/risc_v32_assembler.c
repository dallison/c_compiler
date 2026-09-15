//
//  risc_v32_assembler.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v32_assembler.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "elf.h"
#include "risc_v32_machine.h"
#include "risc_v32_reg_alloc.h"

static COMPILER_UNUSED int CompareString(const void* a, const void* b) {
  MapKeyValue* s1 = (MapKeyValue*)a;
  MapKeyValue* s2 = (MapKeyValue*)b;
  return strcmp(s1->key.p, s2->key.p);
}

//
// Forward declarations of instruction assembly functions.
//

#define DECLARE_INST_FUNC(mnemonic) \
  static void Assemble_##mnemonic(RV32Assembler*)

DECLARE_INST_FUNC(mv);
DECLARE_INST_FUNC(fmv_s);
DECLARE_INST_FUNC(fmv_d);

DECLARE_INST_FUNC(ret);

// RV32I instructions.
DECLARE_INST_FUNC(lui);
DECLARE_INST_FUNC(auipc);
DECLARE_INST_FUNC(jal);
DECLARE_INST_FUNC(jalr);
DECLARE_INST_FUNC(beq);
DECLARE_INST_FUNC(bne);
DECLARE_INST_FUNC(blt);
DECLARE_INST_FUNC(bge);
DECLARE_INST_FUNC(bltu);
DECLARE_INST_FUNC(bgeu);
DECLARE_INST_FUNC(lb);
DECLARE_INST_FUNC(lh);
DECLARE_INST_FUNC(lw);
DECLARE_INST_FUNC(lbu);
DECLARE_INST_FUNC(lhu);
DECLARE_INST_FUNC(sb);
DECLARE_INST_FUNC(sh);
DECLARE_INST_FUNC(sw);
DECLARE_INST_FUNC(addi);
DECLARE_INST_FUNC(slti);
DECLARE_INST_FUNC(sltiu);
DECLARE_INST_FUNC(xori);
DECLARE_INST_FUNC(ori);
DECLARE_INST_FUNC(andi);
DECLARE_INST_FUNC(slli);
DECLARE_INST_FUNC(srli);
DECLARE_INST_FUNC(srai);
DECLARE_INST_FUNC(add);
DECLARE_INST_FUNC(sub);
DECLARE_INST_FUNC(sll);
DECLARE_INST_FUNC(slt);
DECLARE_INST_FUNC(sltu);
DECLARE_INST_FUNC(xor);
DECLARE_INST_FUNC(srl);
DECLARE_INST_FUNC(sra);
DECLARE_INST_FUNC(or);
DECLARE_INST_FUNC(and);
DECLARE_INST_FUNC(fence);
DECLARE_INST_FUNC(fence_i);
DECLARE_INST_FUNC(ecall);
DECLARE_INST_FUNC(ebreak);
DECLARE_INST_FUNC(csrrw);
DECLARE_INST_FUNC(csrrs);
DECLARE_INST_FUNC(csrrc);
DECLARE_INST_FUNC(csrrwi);
DECLARE_INST_FUNC(csrrsi);
DECLARE_INST_FUNC(csrrci);

// RV32M instructions.
DECLARE_INST_FUNC(mul);
DECLARE_INST_FUNC(mulh);
DECLARE_INST_FUNC(mulhsu);
DECLARE_INST_FUNC(mulhu);
DECLARE_INST_FUNC(div);
DECLARE_INST_FUNC(divu);
DECLARE_INST_FUNC(rem);
DECLARE_INST_FUNC(remu);

DECLARE_INST_FUNC(lr_w);
DECLARE_INST_FUNC(lr_w_aq);
DECLARE_INST_FUNC(lr_w_aqrl);
DECLARE_INST_FUNC(sc_w);
DECLARE_INST_FUNC(sc_w_rl);
DECLARE_INST_FUNC(sc_w_aqrl);
DECLARE_INST_FUNC(amoadd_w);
DECLARE_INST_FUNC(amoadd_w_aq);
DECLARE_INST_FUNC(amoadd_w_rl);
DECLARE_INST_FUNC(amoadd_w_aqrl);

// RV32F instructions.
DECLARE_INST_FUNC(flw);
DECLARE_INST_FUNC(fsw);
DECLARE_INST_FUNC(fmadd_s);
DECLARE_INST_FUNC(fmsub_s);
DECLARE_INST_FUNC(fnmsub_s);
DECLARE_INST_FUNC(fnmadd_s);
DECLARE_INST_FUNC(fadd_s);
DECLARE_INST_FUNC(fsub_s);
DECLARE_INST_FUNC(fmul_s);
DECLARE_INST_FUNC(fdiv_s);
DECLARE_INST_FUNC(fsqrt_s);
DECLARE_INST_FUNC(fsgnj_s);
DECLARE_INST_FUNC(fsgnjn_s);
DECLARE_INST_FUNC(fsgnjx_s);
DECLARE_INST_FUNC(fmin_s);
DECLARE_INST_FUNC(fmax_s);
DECLARE_INST_FUNC(fcvt_w_s);
DECLARE_INST_FUNC(fcvt_wu_s);
DECLARE_INST_FUNC(fmv_x_w);
DECLARE_INST_FUNC(feq_s);
DECLARE_INST_FUNC(flt_s);
DECLARE_INST_FUNC(fle_s);
DECLARE_INST_FUNC(fclass_s);
DECLARE_INST_FUNC(fcvt_s_w);
DECLARE_INST_FUNC(fcvt_s_wu);
DECLARE_INST_FUNC(fmv_w_x);

// RV32D instructions.
DECLARE_INST_FUNC(fld);
DECLARE_INST_FUNC(fsd);
DECLARE_INST_FUNC(fmadd_d);
DECLARE_INST_FUNC(fmsub_d);
DECLARE_INST_FUNC(fnmsub_d);
DECLARE_INST_FUNC(fnmadd_d);
DECLARE_INST_FUNC(fadd_d);
DECLARE_INST_FUNC(fsub_d);
DECLARE_INST_FUNC(fmul_d);
DECLARE_INST_FUNC(fdiv_d);
DECLARE_INST_FUNC(fsqrt_d);
DECLARE_INST_FUNC(fsgnj_d);
DECLARE_INST_FUNC(fsgnjn_d);
DECLARE_INST_FUNC(fsgnjx_d);
DECLARE_INST_FUNC(fmin_d);
DECLARE_INST_FUNC(fmax_d);
DECLARE_INST_FUNC(fcvt_s_d);
DECLARE_INST_FUNC(fcvt_d_s);
DECLARE_INST_FUNC(feq_d);
DECLARE_INST_FUNC(flt_d);
DECLARE_INST_FUNC(fle_d);
DECLARE_INST_FUNC(fclass_d);
DECLARE_INST_FUNC(fcvt_w_d);
DECLARE_INST_FUNC(fcvt_wu_d);
DECLARE_INST_FUNC(fcvt_d_w);
DECLARE_INST_FUNC(fcvt_d_wu);

// Pseudo ops
DECLARE_INST_FUNC(nop);
DECLARE_INST_FUNC(not);
DECLARE_INST_FUNC(neg);
DECLARE_INST_FUNC(fneg_s);
DECLARE_INST_FUNC(fneg_d);
DECLARE_INST_FUNC(li);
DECLARE_INST_FUNC(la);
DECLARE_INST_FUNC(lla);
DECLARE_INST_FUNC(tprel);
DECLARE_INST_FUNC(tlsgd);
DECLARE_INST_FUNC(seqz);
DECLARE_INST_FUNC(snez);
DECLARE_INST_FUNC(sltz);
DECLARE_INST_FUNC(sgtz);
DECLARE_INST_FUNC(beqz);
DECLARE_INST_FUNC(bnez);
DECLARE_INST_FUNC(j);
DECLARE_INST_FUNC(jr);
DECLARE_INST_FUNC(call);
DECLARE_INST_FUNC(rcall);
DECLARE_INST_FUNC(callf);
DECLARE_INST_FUNC(rcallf);

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
  INST(mv);
  INST2(fmv_s, fmv.s);
  INST2(fmv_d, fmv.d);

  INST(ret);

  // RV32I instructions.
  INST(lui);
  INST(auipc);
  INST(jal);
  INST(jalr);
  INST(beq);
  INST(bne);
  INST(blt);
  INST(bge);
  INST(bltu);
  INST(bgeu);
  INST(lb);
  INST(lh);
  INST(lw);
  INST(lbu);
  INST(lhu);
  INST(sb);
  INST(sh);
  INST(sw);
  INST(addi);
  INST(slti);
  INST(sltiu);
  INST(xori);
  INST(ori);
  INST(andi);
  INST(slli);
  INST(srli);
  INST(srai);
  INST(add);
  INST(sub);
  INST(sll);
  INST(slt);
  INST(sltu);
  INST(xor);
  INST(srl);
  INST(sra);
  INST(or);
  INST(and);
  INST(fence);
  INST(fence_i);
  INST(ecall);
  INST(ebreak);
  INST(csrrw);
  INST(csrrs);
  INST(csrrc);
  INST(csrrwi);
  INST(csrrsi);
  INST(csrrci);

  // RV32M instructions.
  INST(mul);
  INST(mulh);
  INST(mulhsu);
  INST(mulhu);
  INST(div);
  INST(divu);
  INST(rem);
  INST(remu);

  INST2(lr_w, lr.w);
  INST2(lr_w_aq, lr.w.aq);
  INST2(lr_w_aqrl, lr.w.aqrl);
  INST2(sc_w, sc.w);
  INST2(sc_w_rl, sc.w.rl);
  INST2(sc_w_aqrl, sc.w.aqrl);
  INST2(amoadd_w, amoadd.w);
  INST2(amoadd_w_aq, amoadd.w.aq);
  INST2(amoadd_w_rl, amoadd.w.rl);
  INST2(amoadd_w_aqrl, amoadd.w.aqrl);

  // RV32F instructions.
  INST(flw);
  INST(fsw);
  INST2(fmadd_s, fmadd.s);
  INST2(fmsub_s, fmsub.s);
  INST2(fnmsub_s, fnmsub.s);
  INST2(fnmadd_s, fnmadd.s);
  INST2(fadd_s, fadd.s);
  INST2(fsub_s, fsub.s);
  INST2(fmul_s, fmul.s);
  INST2(fdiv_s, fdiv.s);
  INST2(fsqrt_s, fsqrt.s);
  INST2(fsgnj_s, fsgnj.s);
  INST2(fsgnjn_s, fsgnjs.s);
  INST2(fsgnjx_s, fsgnjx.s);
  INST2(fmin_s, fmin.s);
  INST2(fmax_s, fmax.s);
  INST2(fcvt_w_s, fcvt.w.s);
  INST2(fcvt_wu_s, fcvt.wu.s);
  INST2(fmv_x_w, fmv.x.w);
  INST2(feq_s, feq.s);
  INST2(flt_s, flt.s);
  INST2(fle_s, fle.s);
  INST2(fclass_s, fclass.s);
  INST2(fcvt_s_w, fcvt.s.w);
  INST2(fcvt_s_wu, fcvt.w.su);
  INST2(fmv_w_x, fmv.w.x);

  // RV32D instructions.
  INST(fld);
  INST(fsd);
  INST2(fmadd_d, fmadd.d);
  INST2(fmsub_d, fmsub.d);
  INST2(fnmsub_d, fnmsub.d);
  INST2(fnmadd_d, fnmadd.d);
  INST2(fadd_d, fadd.d);
  INST2(fsub_d, fsub.d);
  INST2(fmul_d, fmul.d);
  INST2(fdiv_d, fdiv.d);
  INST2(fsqrt_d, fsqrt.d);
  INST2(fsgnj_d, fsgnj.d);
  INST2(fsgnjn_d, fsgnjn.d);
  INST2(fsgnjx_d, fsgnjx.d);
  INST2(fmin_d, fmin.d);
  INST2(fmax_d, fmax.d);
  INST2(fcvt_s_d, fcvt.s.d);
  INST2(fcvt_d_s, fcvt.d.s);
  INST2(feq_d, feq.d);
  INST2(flt_d, flt.d);
  INST2(fle_d, fle.d);
  INST2(fclass_d, fclass.d);
  INST2(fcvt_w_d, fcvt.w.d);
  INST2(fcvt_wu_d, fcvt.wu.d);
  INST2(fcvt_d_w, fcvt.d.w);
  INST2(fcvt_d_wu, fcvt.d.wu);

  // Pseudo ops
  INST(nop);
  INST(not);
  INST(neg);
  INST2(fneg_s, fneg.s);
  INST2(fneg_d, fneg.d);
  INST(li);
  INST(la);
  INST(lla);
  INST(tprel);
  INST(tlsgd);
  INST(seqz);
  INST(snez);
  INST(sltz);
  INST(sgtz);
  INST(beqz);
  INST(bnez);
  INST(j);
  INST(jr);
  INST(call);
  INST(rcall);
  INST(callf);
  INST(rcallf);
}

#undef INST

// Initialize the assembler.  Returns true if it worked.
bool RV32AssemblerInit(RV32Assembler* assembler, String* infile, String* outfile) {
  static int reloc_types[] = {
      R_RISCV_ADD16, R_RISCV_32,       R_RISCV_64,       R_RISCV_ADD16, R_RISCV_ADD32,
      R_RISCV_ADD64,    R_RISCV_SUB16,    R_RISCV_SUB32, R_RISCV_SUB64,
      R_RISCV_CALL_PLT, R_RISCV_GOT_HI20,
      R_RISCV_TPREL_HI20, R_RISCV_TPREL_LO12_I, R_RISCV_TPREL_ADD,
  };

  // 4 for the flags specifies the double float ABI (ilp32d).
  if (!AssemblerInit(&assembler->base, ELF_MACHINE_TYPE_RISC_V, 4, reloc_types, infile, outfile)) {
    return false;
  }
  assembler->base.object.is_64_bit = false;

  MapInit(&assembler->instructions, CompareString);

  InitializeInstructions(&assembler->instructions);

  // Add a NULL section at the start of the file.
  AssemblerAddSection(&assembler->base, NULL, SHT(null), 0, 0);
  // Add a .bss section.
  assembler->bss = AssemblerAddSection(&assembler->base, NewString(".bss"),
                                       SHT(nobits), SHF(alloc) | SHF(write), 4);
  assembler->next_la_label = 0;
  return true;
}

RV32Assembler* NewRV32Assembler(String* infile, String* outfile) {
  RV32Assembler* assembler = malloc(sizeof(RV32Assembler));
  RV32AssemblerInit(assembler, infile, outfile);
  return assembler;
}

// Destruct the assembler.
void RV32AssemblerDestruct(RV32Assembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void RV32AssemblerDelete(RV32Assembler* assembler) {
  RV32AssemblerDestruct(assembler);
  free(assembler);
}

// Main assembly function.  This is called by the assembler driver.  It will be
// called twice, one for each pass.
// In pass 1 we parse everything and define all the symbols.
// In pass 2 we also parse everything but we also insert the binary instructions
//    and data into the buffers and expect all symbols to be defined.
void AssembleRV32Instruction(Assembler* base, String* word) {
  RV32Assembler* assembler = (RV32Assembler*)base;

  void* asm_func = MapFindPointerKey(&assembler->instructions, word->value);
  if (asm_func != NULL) {
    void (*func)(RV32Assembler*) = asm_func;
    func(assembler);
  } else {
    AssemblerError(&assembler->base, "Syntax error; unknown instruction: %s",
                   word->value);
  }
}

// Known RISC-V32 register names.
static struct {
  const char* name;
  int number;
} known_reg_names[] = {
    {"sp", RV32_SP_REG},  {"float", RV32_FP_REG},
    {"ra", RV32_RET_REG}, {"zero", RV32_INT_ZERO_REG},
    {NULL, 0},
};

// Prefixed register naems.
static struct PrefixedReg {
  const char* prefix;  // Register prefix.
  int prefix_length;   // Length of prefix.
  int num_start;
  int range_start;
  int range_end;
  RV32RegisterType type;
} prefixed_reg_names[] = {
    {"x", 1, 0, 0, RV32_NUM_INT_REGS, kRV32RegTypeInt},
    {"a", 1, 0, RV32_INT_ARG_START, RV32_INT_ARG_END, kRV32RegTypeInt},
    {"s", 1, 0, RV32_INT_SAVED_START_1, RV32_INT_SAVED_END_1, kRV32RegTypeInt},
    {"s", 1, 2, RV32_INT_SAVED_START_2, RV32_INT_SAVED_END_2, kRV32RegTypeInt},
    {"t", 1, 0, RV32_INT_TEMP_START_1, RV32_INT_TEMP_END_1, kRV32RegTypeInt},
    {"t", 1, 3, RV32_INT_TEMP_START_2, RV32_INT_TEMP_END_2, kRV32RegTypeInt},
    {"fa", 2, 0, RV32_FP_ARG_START, RV32_FP_ARG_END, kRV32RegTypeFloat},
    {"fs", 2, 0, RV32_FP_SAVED_START_1, RV32_FP_SAVED_END_1, kRV32RegTypeFloat},
    {"fs", 2, 2, RV32_FP_SAVED_START_2, RV32_FP_SAVED_END_2, kRV32RegTypeFloat},
    {"ft", 2, 0, RV32_FP_TEMP_START_1, RV32_FP_TEMP_END_1, kRV32RegTypeFloat},
    {"ft", 2, 2, RV32_FP_TEMP_START_2, RV32_FP_TEMP_END_2, kRV32RegTypeFloat},
    {"f", 1, 0, 0, RV32_NUM_FLOAT_REGS, kRV32RegTypeFloat},
    {NULL, 0, 0, 0, 0, 0}};

static int ExtractRegNumber(String* reg_name, size_t i) {
  int num = 0;
  while (reg_name->value[i] != '\0') {
    num = num * 10 + reg_name->value[i++] - '0';
  }
  return num;
}

// Shortcut macro avoid typing assembler->base. everywhere we want to access
// the base assembler.
#define ASM assembler->base
#define ASMO (assembler->base.object)

static bool RegNumber(String* reg_name, int* reg_num,
                      RV32RegisterType* reg_type) {
  for (size_t i = 0; known_reg_names[i].name != NULL; i++) {
    if (StringEqual(reg_name, known_reg_names[i].name)) {
      *reg_num = known_reg_names[i].number;
      *reg_type = kRV32RegTypeInt;  // All int regs.
      return true;
    }
  }
  // Keep track of the last prefix length to avoid calling ExtractRegNumber
  // every iteration.
  int last_prefix_length = 0;
  int num = 0;  // Number extracted from register name.

  // Look for the register in the set of prefixed register names.
  // Each register name starts with a prefix and is followed by a number.
  // The prefix can be 1 or 2 characters and the number is offset from the
  // start of the range type ("s" for saved, "t" for temp, etc.)
  for (size_t i = 0; prefixed_reg_names[i].prefix != NULL; i++) {
    struct PrefixedReg* reg = &prefixed_reg_names[i];
    if (strncmp(reg->prefix, reg_name->value, reg->prefix_length) == 0) {
      if (last_prefix_length != reg->prefix_length) {
        // Extract register number only if the prefix length changed.
        num = ExtractRegNumber(reg_name, reg->prefix_length);
        last_prefix_length = reg->prefix_length;
      }

      // Get length of register range.
      int range_length = reg->range_end - reg->range_start + 1;

      // Say the register name is s3: therefore prefix = "s" and num = 3
      // The PrefixedReg entry for this is the second saved int range.
      // However, there are two ranges for this and we need to select the one
      // whose num_start is less than or equal to 3.  We also need to check that
      // 3 - 2 <= range_length so that we don't overrun the range.
      if (num >= reg->num_start && (num - reg->num_start) < range_length) {
        *reg_num = reg->range_start + num - reg->num_start;
        *reg_type = reg->type;
        return true;
      }
    }
  }
  return false;
}

static bool RegisterName(RV32Assembler* assembler, int* num,
                         RV32RegisterType* type) {
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String reg_name;
    StringInit(&reg_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);

    bool ok = RegNumber(&reg_name, num, type);
    StringDestruct(&reg_name);
    return ok;
  }
  return false;
}

static int Register(RV32Assembler* assembler, RV32RegisterType type_needed,
                    const char* type_name) {
  int num;
  RV32RegisterType type;
  if (!RegisterName(assembler, &num, &type)) {
    AssemblerError(&ASM, "Expected %s register name", type_name);
    return 0;
  }

  if (type != type_needed) {
    const char* reg_type = type == kRV32RegTypeInt ? "integer" : "float";
    AssemblerError(&ASM, "Invalid register type; got %s expected %s", reg_type,
                   type_name);
    return 0;
  }
  return num;
}

// An optional register, returning -1 if there was no register specified.
static int OptionalRegister(RV32Assembler* assembler) {
  int num;
  RV32RegisterType type;
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String reg_name;
    StringInit(&reg_name, ASM.lex.spelling.value);

    bool ok = RegNumber(&reg_name, &num, &type);
    if (ok) {
      // A register is present, advance token.
      LexNextToken(&ASM.lex);
    }
    StringDestruct(&reg_name);
    return ok ? num : -1;
  }
  return -1;
}

static bool ParseRegisterTriple(RV32Assembler* assembler,
                                RV32RegisterType type_needed,
                                const char* type_name, int* regs) {
  regs[0] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[1] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[2] = Register(assembler, type_needed, type_name);
  return true;
}

static bool ParseRegisterPair(RV32Assembler* assembler,
                              RV32RegisterType type_needed, const char* type_name,
                              int* regs) {
  regs[0] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[1] = Register(assembler, type_needed, type_name);
  return true;
}

static int32_t RTypeInstruction(int opcode, int rd, int rs1, int rs2,
                                int funct3, int funct7) {
  return funct7 << 25 | rs2 << 20 | rs1 << 15 | funct3 << 12 | rd << 7 | opcode;
}

static void AssembleAtomic(RV32Assembler* assembler, int funct5, int width,
                           bool aq, bool rl, bool load_reserved) {
  int rd = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  int rs2 = 0;
  if (!load_reserved) {
    rs2 = Register(assembler, kRV32RegTypeInt, "integer");
    if (!LexMatch(&ASM.lex, TOK(comma))) {
      AssemblerError(&ASM, "Missing comma");
      return;
    }
  }
  if (!LexMatch(&ASM.lex, TOK(lparen))) {
    AssemblerError(&ASM, "Expected (address register)");
    return;
  }
  int rs1 = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(rparen))) {
    AssemblerError(&ASM, "Missing close paren");
    return;
  }
  int funct7 = (funct5 << 2) | (aq ? 2 : 0) | (rl ? 1 : 0);
  AssemblerEmitWord(
      &ASM, ASMO.current_section,
      RTypeInstruction(RV32_OPCODE(amo), rd, rs1, rs2, width, funct7));
}

#define ASSEMBLE_ATOMIC_LR(name, width, aq, rl)                  \
  static void Assemble_##name(RV32Assembler* assembler) {          \
    AssembleAtomic(assembler, 0x02, width, aq, rl, true);         \
  }
#define ASSEMBLE_ATOMIC(name, funct5, width, aq, rl)             \
  static void Assemble_##name(RV32Assembler* assembler) {          \
    AssembleAtomic(assembler, funct5, width, aq, rl, false);      \
  }

ASSEMBLE_ATOMIC_LR(lr_w, 2, false, false)
ASSEMBLE_ATOMIC_LR(lr_w_aq, 2, true, false)
ASSEMBLE_ATOMIC_LR(lr_w_aqrl, 2, true, true)
ASSEMBLE_ATOMIC(sc_w, 0x03, 2, false, false)
ASSEMBLE_ATOMIC(sc_w_rl, 0x03, 2, false, true)
ASSEMBLE_ATOMIC(sc_w_aqrl, 0x03, 2, true, true)
ASSEMBLE_ATOMIC(amoadd_w, 0x00, 2, false, false)
ASSEMBLE_ATOMIC(amoadd_w_aq, 0x00, 2, true, false)
ASSEMBLE_ATOMIC(amoadd_w_rl, 0x00, 2, false, true)
ASSEMBLE_ATOMIC(amoadd_w_aqrl, 0x00, 2, true, true)

#undef ASSEMBLE_ATOMIC
#undef ASSEMBLE_ATOMIC_LR

static int32_t ITypeInstruction(int opcode, int rd, int rs1, int funct3,
                                int immed) {
  return immed << 20 | rs1 << 15 | funct3 << 12 | rd << 7 | opcode;
}

static int32_t STypeInstruction(int opcode, int rs1, int rs2, int funct3,
                                int immed) {
  return (immed >> 5) << 25 | rs2 << 20 | rs1 << 15 | funct3 << 12 |
         (immed & 0x1f) << 7 | opcode;
}

static int32_t UTypeInstruction(int opcode, int rd, int immed) {
  return immed << 12 | rd << 7 | opcode;
}

static int32_t BTypeInstruction(int opcode, int rs1, int rs2, int funct3,
                                int immed) {
  return ((immed >> 12) & 1) << 31 |                  // imm[12]
         ((immed >> 5) & 0x3f) << 25 |                // imm[10:5]
         rs2 << 20 |                                  // rs2
         rs1 << 15 |                                  // rs1
         funct3 << 12 | ((immed >> 1) & 0x0f) << 8 |  // imm[4:1]
         ((immed >> 11) & 1) << 7 |                   // imm[11]
         opcode;                                      // opcode
}

static int32_t JTypeInstruction(int opcode, int rd, int immed) {
  return ((immed >> 20) & 1) << 31 |     // imm[20]
         ((immed >> 1) & 0x3ff) << 21 |  // imm[10:1]
         ((immed >> 11) & 1) << 20 |     // imm[11]
         ((immed >> 12) & 0xff) << 12 |  // imm[19:12]
         rd << 7 |                       // rd
         opcode;                         // opcode
}

static AssemblerSymbol* GetOrCreateSymbol(RV32Assembler* assembler,
                                          const char* symbol_name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, symbol_name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(symbol_name, ASMO.current_section, SYM_TYPE(func),
                             SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  return sym;
}

// Assemble a load immediate into a register.
// If the immediate is constant and positive:
//   Look for first 1 bit from high to low.
//   a. less than or equal to 12 bits:
//      addi reg, x0, imm
//   b. more than 12 bits and less than or equal to 32 bits:
//      lui reg, imm[31:12]
//      addi reg, reg, imm[11:0]
//   c. more than 32 bits (uses x6 as a temporary):
//      li x6, imm[63:32]
//      slli x6, x6, 32
//      li reg, imm[31:0]
//      or reg, reg, x6
//
// It the immediate is constant and negative:
//   Look for first 0 bit from high to low.
//   Use same sequences as for positive.
//
// If the immediate is a symbol:
//   Output relocation R_RISCV_HI20
//     auipc reg, 0
//   Output reloation R_RISCV_LO12_I
//     addi reg, reg, 0

static void AssembleLoadImmediateConstant(RV32Assembler* assembler, int reg,
                                          int64_t immed) {
  int bit_width = 0;
  uint64_t value = (uint64_t)immed;
  if (immed < 0) {
    // Look for first 0 bit
    for (int i = 63; i >= 0; i--) {
      if ((value & (1ULL << i)) == 0) {
        bit_width = i + 1;
        break;
      }
    }
  } else {
    // Look for first 1 bit
    for (int i = 63; i >= 0; i--) {
      if ((value & (1ULL << i)) != 0) {
        bit_width = i + 1;
        break;
      }
    }
  }
  // printf("LoadImmediate %" PRId64 ", bit width: %d\n", immed, bit_width);
  if (bit_width < 12) {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(RV32_OPCODE(op_imm), reg, 0, RV32_F3(addi), (int)immed));

  } else if ((immed >= 0 && bit_width < 31) ||
             (immed < 0 && bit_width <= 32)) {
    int64_t hi20 = (immed + 0x800) >> 12;
    AssemblerEmitWord(&ASM, ASMO.current_section,
                      UTypeInstruction(RV32_OPCODE(lui), reg, (int)hi20));
    if ((immed & 0xfff) != 0) {
      AssemblerEmitWord(&ASM, ASMO.current_section,
                        ITypeInstruction(RV32_OPCODE(op_imm), reg, reg, RV32_F3(addi),
                                       (int)immed & 0xfff));
    }
  } else {
    int top_group = 63 / 11;
    while (top_group > 0 && ((value >> (top_group * 11)) & 0x7ff) == 0) {
      top_group--;
    }
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(RV32_OPCODE(op_imm), reg, 0, RV32_F3(addi),
                         (int)((value >> (top_group * 11)) & 0x7ff)));
    for (int group = top_group - 1; group >= 0; group--) {
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          ITypeInstruction(RV32_OPCODE(op_imm), reg, reg, RV32_F3(slli), 11));
      int chunk = (int)((value >> (group * 11)) & 0x7ff);
      if (chunk != 0) {
        AssemblerEmitWord(&ASM, ASMO.current_section,
                          ITypeInstruction(RV32_OPCODE(op_imm), reg, reg,
                                           RV32_F3(ori), chunk));
      }
    }
  }
}

static COMPILER_UNUSED bool AssemblerFunction(RV32Assembler* assembler, String* func,
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

static void AssembleALUReg(RV32Assembler* assembler, int opcode, int funct3,
                           int funct7, int* regs) {
  AssemblerEmitWord(
      &ASM, ASMO.current_section,
      RTypeInstruction(opcode, regs[0], regs[1], regs[2], funct3, funct7));
}

static void AssembleALUImm(RV32Assembler* assembler, int opcode, int funct3,
                           int immed, int* regs) {
  AssemblerEmitWord(&ASM, ASMO.current_section,
                    ITypeInstruction(opcode, regs[0], regs[1], funct3, immed));
}

// Loads and stores have a 12 bit signed offset.  So the offset must
// be in the range. -2048..2047.
static void CheckLoadStoreOffset(RV32Assembler* assembler, int offset) {
  bool offset_ok = true;
  if (offset < 0) {
    offset_ok = offset >= -2048;
  } else {
    offset_ok = offset < 2048;
  }
  if (!offset_ok) {
    AssemblerError(&assembler->base, "Invalid load/store offset %d", offset);
  }
}

// Load or store with a constant offset.
static void AssembleLoadStore(RV32Assembler* assembler, bool isload, int funct3,
                              int offset, int* regs) {
  CheckLoadStoreOffset(assembler, offset);
  if (isload) {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(RV32_OPCODE(load), regs[0], regs[1], funct3, offset));
  } else {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        STypeInstruction(RV32_OPCODE(store), regs[1], regs[0], funct3, offset));
  }
}

// Load or store with a symbol.  A LO12 relocation will be inserted.  The
// load/store opcodes are passed in so this serves both the integer
// (load/store) and floating-point (load_fp/store_fp) forms.
static void AssembleLoadStoreSymbolOp(RV32Assembler* assembler, bool isload,
                                      int load_opcode, int store_opcode,
                                      int funct3, String* symbol,
                                      String* asmfunc, int* regs) {
  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol->value);

  int reloc_type;
  bool pcrel = StringEqual(asmfunc, "pcrel_lo");
  if (isload) {
    reloc_type = pcrel ? R_RISCV_PCREL_LO12_I : R_RISCV_LO12_I;
  } else {
    reloc_type = pcrel ? R_RISCV_PCREL_LO12_S : R_RISCV_LO12_S;
  }
  AssemblerRelocation* reloc =
      NewAssemblerRelocation(sym, reloc_type, ASMO.current_section,
                             (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);

  if (isload) {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(load_opcode, regs[0], regs[1], funct3, 0));
  } else {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        STypeInstruction(store_opcode, regs[1], regs[0], funct3, 0));
  }
}

static void AssembleLoadStoreSymbol(RV32Assembler* assembler, bool isload,
                                    int funct3, String* symbol, String* asmfunc,
                                    int* regs) {
  AssembleLoadStoreSymbolOp(assembler, isload, RV32_OPCODE(load),
                            RV32_OPCODE(store), funct3, symbol, asmfunc, regs);
}

#define ASSEMBLE_INT_ALU_REG(inst)                                        \
  static void Assemble_##inst(RV32Assembler* assembler) {                   \
    int regs[3];                                                          \
    if (ParseRegisterTriple(assembler, kRV32RegTypeInt, "integer", regs)) { \
      AssembleALUReg(assembler, RV32_OPCODE(op), RV32_F3(inst), RV32_F7(inst),  \
                     regs);                                               \
    }                                                                     \
  }

#define ASSEMBLE_INT_ALU_REG_32(inst)                                        \
  static void Assemble_##inst(RV32Assembler* assembler) {                      \
    int regs[3];                                                             \
    if (ParseRegisterTriple(assembler, kRV32RegTypeInt, "integer", regs)) {    \
      AssembleALUReg(assembler, RV32_OPCODE(op_32), RV32_F3(inst), RV32_F7(inst),  \
                     regs);                                                  \
    }                                                                        \
  }

#define ASSEMBLE_INT_ALU_IMM(inst)                                          \
  static void Assemble_##inst(RV32Assembler* assembler) {                     \
    int regs[2];                                                            \
    if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {     \
      if (!LexMatch(&ASM.lex, TOK(comma))) {                                \
        AssemblerError(&ASM, "Missing comma");                              \
        return;                                                             \
      }                                                                     \
      int imm = (int)AssemblerEvaluateExpression(&ASM);                     \
      AssembleALUImm(assembler, RV32_OPCODE(op_imm), RV32_F3(inst), imm, regs); \
    }                                                                       \
  }

// This is the R64 shift immediates with a 6 bit shift amount.
#define ASSEMBLE_INT_SHIFT_IMM(inst)                                         \
  static void Assemble_##inst(RV32Assembler* assembler) {                      \
    int regs[3];                                                             \
    if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {      \
      if (!LexMatch(&ASM.lex, TOK(comma))) {                                 \
        AssemblerError(&ASM, "Missing comma");                               \
        return;                                                              \
      }                                                                      \
      int imm = (int)AssemblerEvaluateExpression(&ASM);                      \
      imm &= 0x3f;                                                           \
      regs[2] = imm;                                                         \
      AssembleALUReg(assembler, RV32_OPCODE(op_imm), RV32_F3(inst), RV32_F7(inst), \
                     regs);                                                  \
    }                                                                        \
  }

#define ASSEMBLE_INT_SHIFT_IMM_32(inst)                                             \
  static void Assemble_##inst(RV32Assembler* assembler) {                             \
    int regs[3];                                                                    \
    if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {             \
      if (!LexMatch(&ASM.lex, TOK(comma))) {                                        \
        AssemblerError(&ASM, "Missing comma");                                      \
        return;                                                                     \
      }                                                                             \
      int imm = (int)AssemblerEvaluateExpression(&ASM);                             \
      imm &= 0x1f;                                                                  \
      regs[2] = imm;                                                                \
      AssembleALUReg(assembler, RV32_OPCODE(op_imm_32), RV32_F3(inst), RV32_F7(inst),     \
                     regs);                                                         \
    }                                                                               \
  }

// Assemble a load or store instruction.
static void AssembleLoadStoreInstruction(RV32Assembler* assembler, bool isload,
                                         int funct3) {
  int regs[2];
  regs[0] = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (LexMatch(&ASM.lex, TOK(percent))) {
    String func;
    StringInit(&func, "");
    String symbol_name;
    StringInit(&symbol_name, "");
    bool ok = AssemblerFunction(assembler, &func, &symbol_name);
    if (!ok) {
      goto error;
    }
    if (!StringEqual(&func, "lo") && !StringEqual(&func, "pcrel_lo")) {
      AssemblerError(&ASM,
                     "Bad assembler function %%%s() for load/store instruction",
                     func.value);
      goto error;
    }
    if (!LexMatch(&ASM.lex, TOK(lparen))) {
      AssemblerError(&ASM, "Expected offset(reg) for load/store");
      goto error;
    }
    regs[1] = Register(assembler, kRV32RegTypeInt, "integer");
    if (!LexMatch(&ASM.lex, TOK(rparen))) {
      AssemblerError(&ASM, "Missing close paren");
      goto error;
    }
    AssembleLoadStoreSymbol(assembler, isload, funct3, &symbol_name, &func,
                            regs);

  error:
    StringDestruct(&func);
    StringDestruct(&symbol_name);
  } else {
    int offset = (int)AssemblerEvaluateExpression(&ASM);
    if (!LexMatch(&ASM.lex, TOK(lparen))) {
      AssemblerError(&ASM, "Expected offset(reg) for load/store");
      return;
    }
    regs[1] = Register(assembler, kRV32RegTypeInt, "integer");
    if (!LexMatch(&ASM.lex, TOK(rparen))) {
      AssemblerError(&ASM, "Missing close paren");
      return;
    }
    AssembleLoadStore(assembler, isload, funct3, offset, regs);
  }
}

#define ASSEMBLE_INT_LOAD_STORE(inst, isload)                     \
  static void Assemble_##inst(RV32Assembler* assembler) {           \
    AssembleLoadStoreInstruction(assembler, isload, RV32_F3(inst)); \
  }

#define UNDEFINED_INST(m)                                     \
  static void Assemble_##m(RV32Assembler* assembler) {          \
    AssemblerError(&ASM, "Unimplemented instruction %s", #m); \
  }

static void Assemble_mv(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    AssembleALUImm(assembler, RV32_OPCODE(op_imm), RV32_F3(addi), 0, regs);
  }
}

// Assembled as fsgnj.s rd, rs, rs
static void Assemble_fmv_s(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeFloat, "float", regs)) {
    regs[2] = regs[1];
    AssembleALUReg(assembler, RV32_OPCODE(op_fp), RV32_F3(fsgnj_s), RV32_F7(fsgnj_s),
                   regs);
  }
}

// Assembled as fsgnj.d rd, rs, rs
static void Assemble_fmv_d(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeFloat, "float", regs)) {
    regs[2] = regs[1];
    AssembleALUReg(assembler, RV32_OPCODE(op_fp), RV32_F3(fsgnj_d), RV32_F7(fsgnj_d),
                   regs);
  }
}

static void Assemble_ret(RV32Assembler* assembler) {
  AssemblerEmitWord(&ASM, ASMO.current_section,
                    ITypeInstruction(RV32_OPCODE(jalr), 0, 1, RV32_F3(jalr), 0));
}

static void AssembleJType(RV32Assembler* assembler, int opcode, int reg,
                          String* symbol_name, int reloc_type) {
  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name->value);

  AssemblerRelocation* reloc =
      NewAssemblerRelocation(sym, reloc_type, ASMO.current_section,
                             (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);

  AssemblerEmitWord(&ASM, ASMO.current_section,
                    JTypeInstruction(opcode, reg, 0));
}

static void AssembleUType(RV32Assembler* assembler, int opcode, int reg,
                          String* symbol_name, int rel_reloc_type, int pic_reloc_type) {
  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name->value);

  int reloc_type = rel_reloc_type;
  if (assembler->base.object.pic &&
      (sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak))) {
    reloc_type = pic_reloc_type;
  }
  AssemblerRelocation* reloc =
      NewAssemblerRelocation(sym, reloc_type, ASMO.current_section,
                             (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);

  AssemblerEmitWord(&ASM, ASMO.current_section,
                    UTypeInstruction(opcode, reg, 0));
}

static void Assemble_lui(RV32Assembler* assembler) {
  int reg = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  String symbol_name;
  StringInit(&symbol_name, "");
  if (LexMatch(&ASM.lex, TOK(percent))) {
    // Assembler function.
    String func;
    StringInit(&func, "");
    bool ok = AssemblerFunction(assembler, &func, &symbol_name);
    if (!ok) {
      StringDestruct(&func);
      return;
    }
    if (!StringEqual(&func, "hi")) {
      AssemblerError(&ASM, "Bad assembler function %%%s() for lui instruction",
                     func.value);
      StringDestruct(&func);
      return;
    }
    StringDestruct(&func);
    AssembleUType(assembler, RV32_OPCODE(lui), reg, &symbol_name, R_RISCV_HI20, R_RISCV_GOT_HI20);
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    StringSet(&symbol_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    AssembleUType(assembler, RV32_OPCODE(lui), reg, &symbol_name, R_RISCV_HI20, R_RISCV_GOT_HI20);
  } else {
    // Expression.
    int64_t value = AssemblerEvaluateExpression(&ASM);
    AssemblerEmitWord(&ASM, ASMO.current_section,
                      UTypeInstruction(RV32_OPCODE(lui), reg, (int32_t)value));
  }
}

static void Assemble_auipc(RV32Assembler* assembler) {
  int reg = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  String symbol_name;
  StringInit(&symbol_name, "");
  if (LexMatch(&ASM.lex, TOK(percent))) {
    // Assembler function.
    String func;
    StringInit(&func, "");
    bool ok = AssemblerFunction(assembler, &func, &symbol_name);
    if (!ok) {
      StringDestruct(&func);
      return;
    }
    if (!StringEqual(&func, "pcrel_hi")) {
      AssemblerError(&ASM,
                     "Bad assembler function %%%s() for auipc instruction",
                     func.value);
      StringDestruct(&func);
      return;
    }
    StringDestruct(&func);
  } else {
    if (LexLookingAt(&ASM.lex, TOK(identifier))) {
      StringSet(&symbol_name, ASM.lex.spelling.value);
      LexNextToken(&ASM.lex);
    } else {
      int64_t value = AssemblerEvaluateExpression(&ASM);
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          UTypeInstruction(RV32_OPCODE(auipc), reg, (int32_t)value));
      return;
    }
  }

  AssembleUType(assembler, RV32_OPCODE(auipc), reg, &symbol_name,
                R_RISCV_PCREL_HI20, R_RISCV_GOT_HI20);
}

static void Assemble_jal(RV32Assembler* assembler) {
  // The assembler should allow:
  // jal offset   -> jal x1, offset
  int reg = OptionalRegister(assembler);
  if (reg == -1) {
    reg = 1;  // x1.
  } else {
    if (!LexMatch(&ASM.lex, TOK(comma))) {
      AssemblerError(&ASM, "Missing comma");
      return;
    }
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for jal instruction");
    return;
  }
  String symbol_name;
  StringInit(&symbol_name, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);

  // TODO: what relocation?
  AssembleJType(assembler, RV32_OPCODE(jal), reg, &symbol_name, R_RISCV_NONE);
  StringDestruct(&symbol_name);
}

static void Assemble_j(RV32Assembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for j instruction");
    return;
  }
  String symbol_name;
  StringInit(&symbol_name, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);

  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);
  if (sym->is_label && sym->binding == SYM_BIND(local) && sym->defined) {
    // j offset   -> jal x0, offset
    int64_t addr = sym->value;
    int32_t offset = (int32_t)(addr - AssemblerCurrentAddress(&ASM));
    AssemblerEmitWord(&ASM, ASMO.current_section,
                      JTypeInstruction(RV32_OPCODE(jal), 0, offset));
  } else {
    // j symbol.
    // For PIC we use a R_RISCV_CALL_PLT and generate the auipc and jalr
    // For non-PIC we generate a R_RISCV_JAL relocation and a jal instruction.
    // R_RISCV_JAL relocation.
    int reloc_type = R_RISCV_JAL;
    if ((sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak)) &&
        assembler->base.object.pic) {
      reloc_type = R_RISCV_CALL_PLT;
    }
    AssemblerRelocation* reloc = NewAssemblerRelocation(
        sym, reloc_type,
        ASMO.current_section, (int32_t)AssemblerCurrentAddress(&ASM), 0);
    AssemblerAddRelocation(&ASM, reloc);
    if (reloc_type == R_RISCV_CALL_PLT) {
      // The general instruction sequence for a jump is:
      // auipc t0, %hi(addr)
      // jalr x0, t0, %lo(addr)(ra)
      //
      // But if the address is within range of a jal instruction immediate
      // the linker can relax the instrucitons to a jal and a nop:
      // jal t0, addr
      AssemblerEmitWord(&ASM, ASMO.current_section,
                        UTypeInstruction(RV32_OPCODE(auipc), RV32_INT_TEMP_START_1, 0));
      AssemblerEmitWord(&ASM, ASMO.current_section,
                        ITypeInstruction(RV32_OPCODE(jalr), 0, RV32_INT_TEMP_START_1,
                                         RV32_F3(jalr), 0));
    } else {
      // Non-PIC.
      AssemblerEmitWord(&ASM, ASMO.current_section,
                      JTypeInstruction(RV32_OPCODE(jal), 0, 0));
    }
  }
  StringDestruct(&symbol_name);
}

static void Assemble_jr(RV32Assembler* assembler) {
  int reg = Register(assembler,  kRV32RegTypeInt, "integer");
  AssemblerEmitWord(&ASM, ASMO.current_section,
                    ITypeInstruction(RV32_OPCODE(jalr), 0, reg,
                                     RV32_F3(jalr), 0));
}

static void Assemble_call(RV32Assembler* assembler) {
  // call offset   -> auipc ra, offset[31:21]
  //                  jalr ra, ra, offset[11:0]
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing symbol for call instruction");
    return;
  }
  String symbol_name;
  StringInit(&symbol_name, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);

  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);

  // Call relocation: this is a macro relocation that acts on two instructions
  // put the destination address in a register and use a jalr instruction to
  // jump to it, saving the return address.
  // Use a PLT relocation for interposable PIC calls.
  int reloc_type = R_RISCV_CALL;
  if ((sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak)) &&
      assembler->base.object.pic) {
    reloc_type = R_RISCV_CALL_PLT;
  }
  AssemblerRelocation* reloc = NewAssemblerRelocation(
      sym, reloc_type,
      ASMO.current_section, (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);

  // Call is followed by R_RISC_V_RELAX relocation to allow the linker to
  // relax the call instruction sequence to a jal instruction if the
  // address is within range.
  reloc = NewAssemblerRelocation(sym, R_RISCV_RELAX, ASMO.current_section,
                                 (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, reloc);

  // The general instruction sequence for a call is:
  // auipc ra, %hi(addr)
  // jalr ra, ra, %lo(addr)(ra)
  //
  // But if the address is within range of a jal instruction immediate
  // the linker can relax the instrucitons to a jal and a nop:
  // jal ra, addr
  AssemblerEmitWord(&ASM, ASMO.current_section,
                    UTypeInstruction(RV32_OPCODE(auipc), RV32_RET_REG, 0));
  AssemblerEmitWord(&ASM, ASMO.current_section,
                    ITypeInstruction(RV32_OPCODE(jalr), RV32_RET_REG, RV32_RET_REG,
                                     RV32_F3(jalr), 0));
  StringDestruct(&symbol_name);
}

static void Assemble_jalr(RV32Assembler* assembler) {
  int regs[2];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    if (!LexMatch(&ASM.lex, TOK(comma))) {
      AssemblerError(&ASM, "Missing comma");
      return;
    }
    int64_t immed = AssemblerEvaluateExpression(&ASM);
    AssemblerEmitWord(&ASM, ASMO.current_section,
                      ITypeInstruction(RV32_OPCODE(jalr), regs[0], regs[1],
                                       RV32_F3(jalr), (int32_t)immed));
  }
}

static void AssembleConditionalBranch(RV32Assembler* assembler, int funct3) {
  int regs[2];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    if (!LexMatch(&ASM.lex, TOK(comma))) {
      AssemblerError(&ASM, "Missing comma");
      return;
    }
    int64_t addr = AssemblerEvaluateExpression(&ASM);
    int32_t offset = (int32_t)(addr - AssemblerCurrentAddress(&ASM));
    if (offset >= -4096 && offset <= 4094) {
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          BTypeInstruction(RV32_OPCODE(branch), regs[0], regs[1], funct3,
                           offset));
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          ITypeInstruction(RV32_OPCODE(op_imm), 0, 0, RV32_F3(addi), 0));
    } else {
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          BTypeInstruction(RV32_OPCODE(branch), regs[0], regs[1], funct3 ^ 1,
                           8));
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          JTypeInstruction(RV32_OPCODE(jal), 0, offset - 4));
    }
  }
}

#define ASSEMBLE_CONDITIONAL_BRANCH(inst)               \
  static void Assemble_##inst(RV32Assembler* assembler) { \
    AssembleConditionalBranch(assembler, RV32_F3(inst));  \
  }

ASSEMBLE_CONDITIONAL_BRANCH(beq);
ASSEMBLE_CONDITIONAL_BRANCH(bne);
ASSEMBLE_CONDITIONAL_BRANCH(blt);
ASSEMBLE_CONDITIONAL_BRANCH(bge);
ASSEMBLE_CONDITIONAL_BRANCH(bltu);
ASSEMBLE_CONDITIONAL_BRANCH(bgeu);

#undef ASSEMBLE_CONDITIONAL_BRANCH

ASSEMBLE_INT_LOAD_STORE(lb, true);
ASSEMBLE_INT_LOAD_STORE(lh, true);
ASSEMBLE_INT_LOAD_STORE(lw, true);
ASSEMBLE_INT_LOAD_STORE(lbu, true);
ASSEMBLE_INT_LOAD_STORE(lhu, true);
ASSEMBLE_INT_LOAD_STORE(sb, false);
ASSEMBLE_INT_LOAD_STORE(sh, false);
ASSEMBLE_INT_LOAD_STORE(sw, false);

ASSEMBLE_INT_ALU_IMM(addi);
ASSEMBLE_INT_ALU_IMM(slti);
ASSEMBLE_INT_ALU_IMM(sltiu);
ASSEMBLE_INT_ALU_IMM(xori);
ASSEMBLE_INT_ALU_IMM(ori);
ASSEMBLE_INT_ALU_IMM(andi);

ASSEMBLE_INT_SHIFT_IMM(slli);
ASSEMBLE_INT_SHIFT_IMM(srli);
ASSEMBLE_INT_SHIFT_IMM(srai);

ASSEMBLE_INT_ALU_REG(add);
ASSEMBLE_INT_ALU_REG(sub);
ASSEMBLE_INT_ALU_REG(sll);
ASSEMBLE_INT_ALU_REG(slt);
ASSEMBLE_INT_ALU_REG(sltu);
ASSEMBLE_INT_ALU_REG(xor);
ASSEMBLE_INT_ALU_REG(srl);
ASSEMBLE_INT_ALU_REG(sra);
ASSEMBLE_INT_ALU_REG(or);
ASSEMBLE_INT_ALU_REG(and);

static void Assemble_fence(RV32Assembler* assembler) {
  AssemblerEmitWord(&ASM, ASMO.current_section, 0x0ff0000f);
}

static void Assemble_fence_i(RV32Assembler* assembler) {
  AssemblerEmitWord(&ASM, ASMO.current_section, 0x0000100f);
}

static void Assemble_ecall(RV32Assembler* assembler) {
  AssemblerEmitWord(&ASM, ASMO.current_section, RV32_OPCODE(system));
}

static void Assemble_ebreak(RV32Assembler* assembler) {
  AssemblerEmitWord(&ASM, ASMO.current_section, RV32_OPCODE(system) | (1 << 20));
}

UNDEFINED_INST(csrrw);
UNDEFINED_INST(csrrs);
UNDEFINED_INST(csrrc);
UNDEFINED_INST(csrrwi);
UNDEFINED_INST(csrrsi);
UNDEFINED_INST(csrrci);

// RV32M instructions.
ASSEMBLE_INT_ALU_REG(mul);
ASSEMBLE_INT_ALU_REG(mulh);
ASSEMBLE_INT_ALU_REG(mulhsu);
ASSEMBLE_INT_ALU_REG(mulhu);
ASSEMBLE_INT_ALU_REG(div);
ASSEMBLE_INT_ALU_REG(divu);
ASSEMBLE_INT_ALU_REG(rem);
ASSEMBLE_INT_ALU_REG(remu);

static void AssembleFpLoadStore(RV32Assembler* assembler, bool isload, int funct3,
                                int offset, int* regs) {
  if (isload) {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(RV32_OPCODE(load_fp), regs[0], regs[1], funct3, offset));
  } else {
    AssemblerEmitWord(&ASM, ASMO.current_section,
                      STypeInstruction(RV32_OPCODE(store_fp), regs[1], regs[0],
                                       funct3, offset));
  }
}

// Floating-point load/store.  Like the integer form, the offset may be a
// %lo()/%pcrel_lo() relocation referencing the paired auipc, e.g.
//   fld ft0, %pcrel_lo(.label)(t0)
static void AssembleFpLoadStoreInstruction(RV32Assembler* assembler, bool isload,
                                           int funct3) {
  int regs[2];
  regs[0] = Register(assembler, kRV32RegTypeFloat, "float");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (LexMatch(&ASM.lex, TOK(percent))) {
    String func;
    StringInit(&func, "");
    String symbol_name;
    StringInit(&symbol_name, "");
    bool ok = AssemblerFunction(assembler, &func, &symbol_name);
    if (!ok) {
      goto error;
    }
    if (!StringEqual(&func, "lo") && !StringEqual(&func, "pcrel_lo")) {
      AssemblerError(&ASM,
                     "Bad assembler function %%%s() for load/store instruction",
                     func.value);
      goto error;
    }
    if (!LexMatch(&ASM.lex, TOK(lparen))) {
      AssemblerError(&ASM, "Expected offset(reg) for load/store");
      goto error;
    }
    regs[1] = Register(assembler, kRV32RegTypeInt, "integer");
    if (!LexMatch(&ASM.lex, TOK(rparen))) {
      AssemblerError(&ASM, "Missing close paren");
      goto error;
    }
    AssembleLoadStoreSymbolOp(assembler, isload, RV32_OPCODE(load_fp),
                              RV32_OPCODE(store_fp), funct3, &symbol_name, &func,
                              regs);
  error:
    StringDestruct(&func);
    StringDestruct(&symbol_name);
  } else {
    int offset = (int)AssemblerEvaluateExpression(&ASM);
    if (!LexMatch(&ASM.lex, TOK(lparen))) {
      AssemblerError(&ASM, "Expected offset(reg) for load/store");
      return;
    }
    regs[1] = Register(assembler, kRV32RegTypeInt, "integer");
    if (!LexMatch(&ASM.lex, TOK(rparen))) {
      AssemblerError(&ASM, "Missing close paren");
      return;
    }
    AssembleFpLoadStore(assembler, isload, funct3, offset, regs);
  }
}

#define ASSEMBLE_FP_LOAD_STORE(inst, isload)                       \
  static void Assemble_##inst(RV32Assembler* assembler) {            \
    AssembleFpLoadStoreInstruction(assembler, isload, RV32_F3(inst)); \
  }

// Floating point rounding modes.
static struct {
  const char* name;
  int value;
} rounding_modes[] = {
    {"rne", 0}, {"rtz", 1}, {"rdn", 2}, {"rup", 3},
    {"rmm", 4}, {"dyn", 7}, {NULL, 0},
};

static int RoundingMode(RV32Assembler* assembler, const char* v) {
  for (int i = 0; rounding_modes[i].name != NULL; i++) {
    if (strcmp(v, rounding_modes[i].name) == 0) {
      return rounding_modes[i].value;
    }
  }
  AssemblerError(&ASM, "Invalid floating point rounding mode: %s", v);
  return 0;
}

#define ASSEMBLE_FP_ALU_REG(inst)                                         \
  static void Assemble_##inst(RV32Assembler* assembler) {                   \
    int regs[3];                                                          \
    if (ParseRegisterTriple(assembler, kRV32RegTypeFloat, "float", regs)) { \
      int rm = 0;                                                         \
      if (LexMatch(&ASM.lex, TOK(comma))) {                               \
        if (LexMatch(&ASM.lex, TOK(identifier))) {                        \
          rm = RoundingMode(assembler, ASM.lex.spelling.value);           \
          LexNextToken(&ASM.lex);                                         \
        }                                                                 \
      }                                                                   \
      AssembleALUReg(assembler, RV32_OPCODE(op_fp), rm, RV32_F7(inst), regs); \
    }                                                                     \
  }

#define ASSEMBLE_FP_ALU_F3(inst)                                            \
  static void Assemble_##inst(RV32Assembler* assembler) {                     \
    int regs[3];                                                            \
    if (ParseRegisterTriple(assembler, kRV32RegTypeFloat, "float", regs)) {   \
      AssembleALUReg(assembler, RV32_OPCODE(op_fp), RV32_F3(inst), RV32_F7(inst), \
                     regs);                                                 \
    }                                                                       \
  }

#define ASSEMBLE_FP_FLOAT_TO_INT(inst)                                    \
  static void Assemble_##inst(RV32Assembler* assembler) {                   \
    int regs[3];                                                          \
    regs[0] = Register(assembler, kRV32RegTypeInt, "integer");              \
    if (LexMatch(&ASM.lex, TOK(comma))) {                                 \
      regs[1] = Register(assembler, kRV32RegTypeFloat, "float");            \
      regs[2] = RV32_FP_RS2(inst);                                          \
      int rm = 0;                                                         \
      if (LexMatch(&ASM.lex, TOK(comma))) {                               \
        if (LexMatch(&ASM.lex, TOK(identifier))) {                        \
          rm = RoundingMode(assembler, ASM.lex.spelling.value);           \
          LexNextToken(&ASM.lex);                                         \
        }                                                                 \
      }                                                                   \
      AssembleALUReg(assembler, RV32_OPCODE(op_fp), rm, RV32_F7(inst), regs); \
    }                                                                     \
  }

#define ASSEMBLE_FP_INT_TO_FLOAT(inst)                                    \
  static void Assemble_##inst(RV32Assembler* assembler) {                   \
    int regs[3];                                                          \
    regs[0] = Register(assembler, kRV32RegTypeFloat, "float");              \
    if (LexMatch(&ASM.lex, TOK(comma))) {                                 \
      regs[1] = Register(assembler, kRV32RegTypeInt, "integer");            \
      regs[2] = RV32_FP_RS2(inst);                                          \
      int rm = 0;                                                         \
      if (LexMatch(&ASM.lex, TOK(comma))) {                               \
        if (LexMatch(&ASM.lex, TOK(identifier))) {                        \
          rm = RoundingMode(assembler, ASM.lex.spelling.value);           \
          LexNextToken(&ASM.lex);                                         \
        }                                                                 \
      }                                                                   \
      AssembleALUReg(assembler, RV32_OPCODE(op_fp), rm, RV32_F7(inst), regs); \
    }                                                                     \
  }

#define ASSEMBLE_FP_FLOAT_TO_FLOAT(inst)                                  \
  static void Assemble_##inst(RV32Assembler* assembler) {                   \
    int regs[3];                                                          \
    regs[0] = Register(assembler, kRV32RegTypeFloat, "float");              \
    if (LexMatch(&ASM.lex, TOK(comma))) {                                 \
      regs[1] = Register(assembler, kRV32RegTypeFloat, "float");            \
      regs[2] = RV32_FP_RS2(inst);                                          \
      int rm = 0;                                                         \
      if (LexMatch(&ASM.lex, TOK(comma))) {                               \
        if (LexMatch(&ASM.lex, TOK(identifier))) {                        \
          rm = RoundingMode(assembler, ASM.lex.spelling.value);           \
          LexNextToken(&ASM.lex);                                         \
        }                                                                 \
      }                                                                   \
      AssembleALUReg(assembler, RV32_OPCODE(op_fp), rm, RV32_F7(inst), regs); \
    }                                                                     \
  }

#define ASSEMBLE_FP_INT_TO_INT(inst)                                   \
  static void Assemble_##inst(RV32Assembler* assembler) {                \
    int regs[3];                                                       \
    ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs);      \
    regs[2] = RV32_FP_RS2(inst);                                         \
    AssembleALUReg(assembler, RV32_OPCODE(op_fp), 0, RV32_F7(inst), regs); \
  }

#define ASSEMBLE_FP_CMP(inst)                                               \
  static void Assemble_##inst(RV32Assembler* assembler) {                     \
    int regs[3];                                                            \
    regs[0] = Register(assembler, kRV32RegTypeInt, "integer");                \
    if (LexMatch(&ASM.lex, TOK(comma))) {                                   \
      ParseRegisterPair(assembler, kRV32RegTypeFloat, "float", &regs[1]);     \
      AssembleALUReg(assembler, RV32_OPCODE(op_fp), RV32_F3(inst), RV32_F7(inst), \
                     regs);                                                 \
    }                                                                       \
  }

// RV32F instructions.
ASSEMBLE_FP_LOAD_STORE(flw, true);
ASSEMBLE_FP_LOAD_STORE(fsw, false);
UNDEFINED_INST(fmadd_s);
UNDEFINED_INST(fmsub_s);
UNDEFINED_INST(fnmsub_s);
UNDEFINED_INST(fnmadd_s);
ASSEMBLE_FP_ALU_REG(fadd_s);
ASSEMBLE_FP_ALU_REG(fsub_s);
ASSEMBLE_FP_ALU_REG(fmul_s);
ASSEMBLE_FP_ALU_REG(fdiv_s);

UNDEFINED_INST(fsqrt_s);

ASSEMBLE_FP_ALU_F3(fsgnj_s);
ASSEMBLE_FP_ALU_F3(fsgnjn_s);
ASSEMBLE_FP_ALU_F3(fsgnjx_s);
ASSEMBLE_FP_ALU_F3(fmin_s);
ASSEMBLE_FP_ALU_F3(fmax_s);
ASSEMBLE_FP_FLOAT_TO_INT(fcvt_w_s);
ASSEMBLE_FP_FLOAT_TO_INT(fcvt_wu_s);
ASSEMBLE_FP_INT_TO_INT(fmv_x_w);
ASSEMBLE_FP_CMP(feq_s);
ASSEMBLE_FP_CMP(flt_s);
ASSEMBLE_FP_CMP(fle_s);
UNDEFINED_INST(fclass_s);
ASSEMBLE_FP_INT_TO_FLOAT(fcvt_s_w);
ASSEMBLE_FP_INT_TO_FLOAT(fcvt_s_wu);
ASSEMBLE_FP_INT_TO_FLOAT(fmv_w_x);

// RV32D instructions.
ASSEMBLE_FP_LOAD_STORE(fld, true);
ASSEMBLE_FP_LOAD_STORE(fsd, false);
UNDEFINED_INST(fmadd_d);
UNDEFINED_INST(fmsub_d);
UNDEFINED_INST(fnmsub_d);
UNDEFINED_INST(fnmadd_d);
ASSEMBLE_FP_ALU_REG(fadd_d);
ASSEMBLE_FP_ALU_REG(fsub_d);
ASSEMBLE_FP_ALU_REG(fmul_d);
ASSEMBLE_FP_ALU_REG(fdiv_d);
UNDEFINED_INST(fsqrt_d);
ASSEMBLE_FP_ALU_F3(fsgnj_d);
ASSEMBLE_FP_ALU_F3(fsgnjn_d);
ASSEMBLE_FP_ALU_F3(fsgnjx_d);
ASSEMBLE_FP_ALU_F3(fmin_d);
ASSEMBLE_FP_ALU_F3(fmax_d);
ASSEMBLE_FP_FLOAT_TO_FLOAT(fcvt_s_d);
ASSEMBLE_FP_FLOAT_TO_FLOAT(fcvt_d_s);
ASSEMBLE_FP_CMP(feq_d);
ASSEMBLE_FP_CMP(flt_d);
ASSEMBLE_FP_CMP(fle_d);
UNDEFINED_INST(fclass_d);
ASSEMBLE_FP_FLOAT_TO_INT(fcvt_w_d);
ASSEMBLE_FP_FLOAT_TO_INT(fcvt_wu_d);
ASSEMBLE_FP_INT_TO_FLOAT(fcvt_d_w);
ASSEMBLE_FP_INT_TO_FLOAT(fcvt_d_wu);

static void Assemble_nop(RV32Assembler* assembler) {
  // Assemble as nop as addi x0, x0, 0.
  int regs[2] = {0,0};
  AssembleALUImm(assembler, RV32_OPCODE(op_imm), RV32_F3(addi), 0, regs);
}

// Assembled as xori d, s, -1
static void Assemble_not(RV32Assembler* assembler) {
  int regs[2];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    AssembleALUImm(assembler, RV32_OPCODE(op_imm), RV32_F3(xori), -1, regs);
  }
}

// Assembled as sub d, x0, s
static void Assemble_neg(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    regs[2] = regs[1];
    regs[1] = 0;
    AssembleALUReg(assembler, RV32_OPCODE(op), RV32_F3(sub), RV32_F7(sub), regs);
  }
}

// Assembled as fsgnjn.s rd, rs, rs
static void Assemble_fneg_s(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeFloat, "float", regs)) {
    regs[2] = regs[1];
    AssembleALUReg(assembler, RV32_OPCODE(op_fp), RV32_F3(fsgnjn_s), RV32_F7(fsgnjn_s),
                   regs);
  }
}

// Assembled as fsgnjn.d rd, rs, rs
static void Assemble_fneg_d(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeFloat, "float", regs)) {
    regs[2] = regs[1];
    AssembleALUReg(assembler, RV32_OPCODE(op_fp), RV32_F3(fsgnjn_d), RV32_F7(fsgnjn_d),
                   regs);
  }
}

static void Assemble_li(RV32Assembler* assembler) {
  int reg = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String symbol_name;
    StringInit(&symbol_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);

    AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);

    //   Output relocation R_RISCV_HI20
    //     auipc reg, 0
    AssembleUType(assembler, RV32_OPCODE(auipc), reg, &symbol_name, R_RISCV_HI20, R_RISCV_GOT_HI20);

    //   Output relocation R_RISCV_LO12_I
    //     addi reg, reg, 0
    AssemblerRelocation* reloc =
        NewAssemblerRelocation(sym, R_RISCV_LO12_I, ASMO.current_section,
                               (int32_t)AssemblerCurrentAddress(&ASM), 0);
    AssemblerAddRelocation(&ASM, reloc);
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(RV32_OPCODE(op_imm), reg, reg, RV32_F3(addi), 0));

    StringDestruct(&symbol_name);
  } else {
    int64_t value = AssemblerEvaluateExpression(&ASM);
    AssembleLoadImmediateConstant(assembler, reg, value);
  }
}

static void Assemble_la(RV32Assembler* assembler) {
  int reg = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String symbol_name;
    StringInit(&symbol_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);

    // Since we are using PC relative relocations we need a label before the
    // auipc instruction relocation (PCREL_HI20).  This label is referred to by
    // the LO12_I relocation.  For PIC code the first reloation is
    // R_RISCV_GOT_HI20.
    char label_name[256];
    snprintf(label_name, sizeof(label_name), ".la_label_%" PRId64 "",
             AssemblerCurrentAddress(&ASM));
    AssemblerSymbol* label = GetOrCreateSymbol(assembler, label_name);
    label->value = AssemblerCurrentAddress(&ASM);
    label->exported = true;
    label->defined = true;
    
    // Output relocation R_RISCV_PCREL_HI20 or R_RISCV_GOT_HI20
    //   auipc reg, 0
    AssembleUType(assembler, RV32_OPCODE(auipc), reg, &symbol_name,
                  R_RISCV_PCREL_HI20, R_RISCV_GOT_HI20);

    AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);

    if (assembler->base.object.pic &&
        (sym->binding == SYM_BIND(global) || sym->binding == SYM_BIND(weak))) {
      // Position independent, Load the address from the GOT.
      //   Output relocation R_RISCV_PCREL_LO12_I
      //     ld reg, 0(reg)
      AssemblerRelocation* reloc = NewAssemblerRelocation(
          label, R_RISCV_PCREL_LO12_I, ASMO.current_section,
          (int32_t)AssemblerCurrentAddress(&ASM), 0);
      AssemblerAddRelocation(&ASM, reloc);
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          ITypeInstruction(RV32_OPCODE(load), reg, reg, RV32_F3(ld), 0));
    } else {
      //   Output relocation R_RISCV_PCREL_LO12_I
      //     addi reg, reg, 0
      AssemblerRelocation* reloc = NewAssemblerRelocation(
          label, R_RISCV_PCREL_LO12_I, ASMO.current_section,
          (int32_t)AssemblerCurrentAddress(&ASM), 0);
      AssemblerAddRelocation(&ASM, reloc);
      AssemblerEmitWord(
          &ASM, ASMO.current_section,
          ITypeInstruction(RV32_OPCODE(op_imm), reg, reg, RV32_F3(addi), 0));
    }

    StringDestruct(&symbol_name);
  } else {
    AssemblerError(&ASM, "Expected symbol name for la instruction");
  }
}

// Load a local address (pc relative but not using GOT)
static void Assemble_lla(RV32Assembler* assembler) {
  int reg = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String symbol_name;
    StringInit(&symbol_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);

    // Since we are using PC relative relocations we need a label before the
    // auipc instruction relocation (PCREL_HI20).  This label is referred to by
    // the LO12_I relocation.
    char label_name[256];
    snprintf(label_name, sizeof(label_name), ".la_label_%" PRId64 "",
             AssemblerCurrentAddress(&ASM));
    AssemblerSymbol* label = GetOrCreateSymbol(assembler, label_name);
    label->value = AssemblerCurrentAddress(&ASM);
    label->exported = true;
    label->defined = true;
    
    // Output relocation R_RISCV_PCREL_HI20
    //   auipc reg, 0
    AssembleUType(assembler, RV32_OPCODE(auipc), reg, &symbol_name,
                  R_RISCV_PCREL_HI20, R_RISCV_PCREL_HI20);

    //   Output relocation R_RISCV_PCREL_LO12_I
    //     addi reg, reg, 0
    AssemblerRelocation* reloc = NewAssemblerRelocation(
        label, R_RISCV_PCREL_LO12_I, ASMO.current_section,
        (int32_t)AssemblerCurrentAddress(&ASM), 0);
    AssemblerAddRelocation(&ASM, reloc);
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(RV32_OPCODE(op_imm), reg, reg, RV32_F3(addi), 0));
    
    StringDestruct(&symbol_name);
  } else {
    AssemblerError(&ASM, "Expected symbol name for lla instruction");
  }
}

static void Assemble_tprel(RV32Assembler* assembler) {
  int rd = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Expected TLS symbol name");
    return;
  }
  String symbol_name;
  StringInit(&symbol_name, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);
  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);

  AssemblerAddRelocation(
      &ASM, NewAssemblerRelocation(sym, R_RISCV_TPREL_HI20,
                                   ASMO.current_section,
                                   (int32_t)AssemblerCurrentAddress(&ASM), 0));
  AssemblerEmitWord(&ASM, ASMO.current_section,
                    UTypeInstruction(RV32_OPCODE(lui), rd, 0));

  AssemblerAddRelocation(
      &ASM, NewAssemblerRelocation(sym, R_RISCV_TPREL_ADD,
                                   ASMO.current_section,
                                   (int32_t)AssemblerCurrentAddress(&ASM), 0));
  AssemblerEmitWord(
      &ASM, ASMO.current_section,
      RTypeInstruction(RV32_OPCODE(op), rd, rd, 4, RV32_F3(add), RV32_F7(add)));

  AssemblerAddRelocation(
      &ASM, NewAssemblerRelocation(sym, R_RISCV_TPREL_LO12_I,
                                   ASMO.current_section,
                                   (int32_t)AssemblerCurrentAddress(&ASM), 0));
  AssemblerEmitWord(
      &ASM, ASMO.current_section,
      ITypeInstruction(RV32_OPCODE(op_imm), rd, rd, RV32_F3(addi), 0));
  StringDestruct(&symbol_name);
}

static void Assemble_tlsgd(RV32Assembler* assembler) {
  int rd = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Expected TLS symbol name");
    return;
  }
  String symbol_name;
  StringInit(&symbol_name, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);
  AssemblerSymbol* sym = GetOrCreateSymbol(assembler, symbol_name.value);

  char label_name[256];
  snprintf(label_name, sizeof(label_name), ".tlsgd_label_%" PRId64 "",
           AssemblerCurrentAddress(&ASM));
  AssemblerSymbol* label = GetOrCreateSymbol(assembler, label_name);
  label->value = AssemblerCurrentAddress(&ASM);
  label->exported = true;
  label->defined = true;

  AssemblerAddRelocation(
      &ASM, NewAssemblerRelocation(sym, R_RISCV_TLS_GD_HI20,
                                   ASMO.current_section,
                                   (int32_t)AssemblerCurrentAddress(&ASM), 0));
  AssemblerEmitWord(&ASM, ASMO.current_section,
                    UTypeInstruction(RV32_OPCODE(auipc), rd, 0));

  AssemblerRelocation* lo_reloc = NewAssemblerRelocation(
      label, R_RISCV_PCREL_LO12_I, ASMO.current_section,
      (int32_t)AssemblerCurrentAddress(&ASM), 0);
  AssemblerAddRelocation(&ASM, lo_reloc);
  AssemblerEmitWord(
      &ASM, ASMO.current_section,
      ITypeInstruction(RV32_OPCODE(op_imm), rd, rd, RV32_F3(addi), 0));
  StringDestruct(&symbol_name);
}

// Assembled as sltiu rd, rs, 1
static void Assemble_seqz(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    AssembleALUImm(assembler, RV32_OPCODE(op_imm), RV32_F3(sltiu), 1, regs);
  }
}

// Assembled as sltu rd, x0, rs
static void Assemble_snez(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    regs[2] = regs[1];
    regs[1] = 0;
    AssembleALUReg(assembler, RV32_OPCODE(op), RV32_F3(sltu), RV32_F7(sltu), regs);
  }
}

// Assembled as slt rd, rs, x0
static void Assemble_sltz(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    regs[2] = 0;
    AssembleALUReg(assembler, RV32_OPCODE(op), RV32_F3(slt), RV32_F7(slt), regs);
  }
}

// Assembled as slt rd, x0, x0
static void Assemble_sgtz(RV32Assembler* assembler) {
  int regs[3];
  if (ParseRegisterPair(assembler, kRV32RegTypeInt, "integer", regs)) {
    regs[2] = regs[1];
    regs[1] = 0;
    AssembleALUReg(assembler, RV32_OPCODE(op), RV32_F3(slt), RV32_F7(slt), regs);
  }
}

static void AssembleConditionalBranchZero(RV32Assembler* assembler, int funct3) {
  int reg = Register(assembler, kRV32RegTypeInt, "integer");
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return;
  }
  int64_t addr = AssemblerEvaluateExpression(&ASM);
  int32_t offset = (int32_t)(addr - AssemblerCurrentAddress(&ASM));
  if (offset >= -4096 && offset <= 4094) {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        BTypeInstruction(RV32_OPCODE(branch), reg, 0, funct3, offset));
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        ITypeInstruction(RV32_OPCODE(op_imm), 0, 0, RV32_F3(addi), 0));
  } else {
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        BTypeInstruction(RV32_OPCODE(branch), reg, 0, funct3 ^ 1, 8));
    AssemblerEmitWord(
        &ASM, ASMO.current_section,
        JTypeInstruction(RV32_OPCODE(jal), 0, offset - 4));
  }
}

#define ASSEMBLE_CONDITIONAL_BRANCH_ZERO(inst)             \
  static void Assemble_##inst(RV32Assembler* assembler) {    \
    AssembleConditionalBranchZero(assembler, RV32_F3(inst)); \
  }

ASSEMBLE_CONDITIONAL_BRANCH_ZERO(beqz);
ASSEMBLE_CONDITIONAL_BRANCH_ZERO(bnez);

UNDEFINED_INST(rcall);
UNDEFINED_INST(callf);
UNDEFINED_INST(rcallf);

#undef UNDEFINED_INST
#undef ASM
