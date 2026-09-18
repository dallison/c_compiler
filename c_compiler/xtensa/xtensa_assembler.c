//
//  xtensa_assembler.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "xtensa_assembler.h"
#include <stdlib.h>
#include <string.h>
#include "elf.h"
#include "xtensa_isa.h"
#include "xtensa_machine.h"
#include "xtensa_reg_alloc.h"

static COMPILER_UNUSED int CompareString(const void* a, const void* b) {
  MapKeyValue* s1 = (MapKeyValue*)a;
  MapKeyValue* s2 = (MapKeyValue*)b;
  return strcmp(s1->key.p, s2->key.p);
}

//
// Forward declarations of instruction assembly functions.
//

#define DECLARE_INST_FUNC(mnemonic) \
  static void Assemble_##mnemonic(XTENSAAssembler*)

// Xtensa instruction handlers.  Public mnemonics are registered via INST2.
DECLARE_INST_FUNC(xt_mov);
DECLARE_INST_FUNC(xt_nop);
DECLARE_INST_FUNC(xt_not);
DECLARE_INST_FUNC(xt_neg);
DECLARE_INST_FUNC(xt_li);
DECLARE_INST_FUNC(xt_entry);
DECLARE_INST_FUNC(xt_retw);
DECLARE_INST_FUNC(xt_call8);
DECLARE_INST_FUNC(xt_callx8);
DECLARE_INST_FUNC(xt_j);
DECLARE_INST_FUNC(xt_jx);
DECLARE_INST_FUNC(xt_add);
DECLARE_INST_FUNC(xt_sub);
DECLARE_INST_FUNC(xt_and);
DECLARE_INST_FUNC(xt_or);
DECLARE_INST_FUNC(xt_xor);
DECLARE_INST_FUNC(xt_addi);
DECLARE_INST_FUNC(xt_slli);
DECLARE_INST_FUNC(xt_srai);
DECLARE_INST_FUNC(xt_srli);
DECLARE_INST_FUNC(xt_slt);
DECLARE_INST_FUNC(xt_sltu);
DECLARE_INST_FUNC(xt_slti);
DECLARE_INST_FUNC(xt_sltiu);
DECLARE_INST_FUNC(xt_andi);
DECLARE_INST_FUNC(xt_ori);
DECLARE_INST_FUNC(xt_xori);
DECLARE_INST_FUNC(xt_sll);
DECLARE_INST_FUNC(xt_srl);
DECLARE_INST_FUNC(xt_sra);
DECLARE_INST_FUNC(xt_mul);
DECLARE_INST_FUNC(xt_div);
DECLARE_INST_FUNC(xt_divu);
DECLARE_INST_FUNC(xt_rem);
DECLARE_INST_FUNC(xt_remu);
DECLARE_INST_FUNC(xt_l8ui);
DECLARE_INST_FUNC(xt_l16ui);
DECLARE_INST_FUNC(xt_l16si);
DECLARE_INST_FUNC(xt_l32i);
DECLARE_INST_FUNC(xt_s8i);
DECLARE_INST_FUNC(xt_s16i);
DECLARE_INST_FUNC(xt_s32i);
DECLARE_INST_FUNC(xt_beq);
DECLARE_INST_FUNC(xt_bne);
DECLARE_INST_FUNC(xt_blt);
DECLARE_INST_FUNC(xt_bge);
DECLARE_INST_FUNC(xt_bltu);
DECLARE_INST_FUNC(xt_bgeu);
DECLARE_INST_FUNC(xt_beqz);
DECLARE_INST_FUNC(xt_bnez);
DECLARE_INST_FUNC(xt_bltz);
DECLARE_INST_FUNC(xt_bgez);
DECLARE_INST_FUNC(xt_seqz);
DECLARE_INST_FUNC(xt_snez);
DECLARE_INST_FUNC(xt_sltz);
DECLARE_INST_FUNC(xt_sgtz);
DECLARE_INST_FUNC(xt_break);

#undef DECLARE_INST_FUNC

#define INST2(mnemonic, inst)         \
  do {                                \
    MapKeyValue kv;                   \
    kv.key.p = #inst;                 \
    kv.value.p = Assemble_##mnemonic; \
    MapInsert(instructions, kv);      \
  } while (0)

// Add all instructions to the handler map.  This maps the instruction
// spelling to a handler function.
static void InitializeInstructions(Map* instructions) {
  INST2(xt_mov, mov);
  INST2(xt_mov, mv);
  INST2(xt_nop, nop);
  INST2(xt_not, not);
  INST2(xt_neg, neg);
  INST2(xt_li, li);
  INST2(xt_li, la);
  INST2(xt_li, lla);
  INST2(xt_entry, entry);
  INST2(xt_retw, retw);
  INST2(xt_retw, ret);
  INST2(xt_call8, call8);
  INST2(xt_callx8, callx8);
  INST2(xt_j, j);
  INST2(xt_jx, jx);
  INST2(xt_add, add);
  INST2(xt_sub, sub);
  INST2(xt_and, and);
  INST2(xt_or, or);
  INST2(xt_xor, xor);
  INST2(xt_addi, addi);
  INST2(xt_slli, slli);
  INST2(xt_srai, srai);
  INST2(xt_srli, srli);
  INST2(xt_slt, slt);
  INST2(xt_sltu, sltu);
  INST2(xt_slti, slti);
  INST2(xt_sltiu, sltiu);
  INST2(xt_andi, andi);
  INST2(xt_ori, ori);
  INST2(xt_xori, xori);
  INST2(xt_sll, sll);
  INST2(xt_srl, srl);
  INST2(xt_sra, sra);
  INST2(xt_mul, mul);
  INST2(xt_div, div);
  INST2(xt_divu, divu);
  INST2(xt_rem, rem);
  INST2(xt_remu, remu);
  INST2(xt_l8ui, l8ui);
  INST2(xt_l16ui, l16ui);
  INST2(xt_l16si, l16si);
  INST2(xt_l32i, l32i);
  INST2(xt_s8i, s8i);
  INST2(xt_s16i, s16i);
  INST2(xt_s32i, s32i);
  INST2(xt_beq, beq);
  INST2(xt_bne, bne);
  INST2(xt_blt, blt);
  INST2(xt_bge, bge);
  INST2(xt_bltu, bltu);
  INST2(xt_bgeu, bgeu);
  INST2(xt_beqz, beqz);
  INST2(xt_bnez, bnez);
  INST2(xt_bltz, bltz);
  INST2(xt_bgez, bgez);
  INST2(xt_seqz, seqz);
  INST2(xt_snez, snez);
  INST2(xt_sltz, sltz);
  INST2(xt_sgtz, sgtz);
  INST2(xt_break, break);
}

#undef INST2

// Initialize the assembler.  Returns true if it worked.
bool XTENSAAssemblerInit(XTENSAAssembler* assembler, String* infile,
                         String* outfile) {
  static int reloc_types[] = {
      R_XTENSA_32, R_XTENSA_32, R_XTENSA_32, R_XTENSA_32, R_XTENSA_32,
      R_XTENSA_32, R_XTENSA_32, R_XTENSA_32, R_XTENSA_32,
  };

  if (!AssemblerInit(&assembler->base, ELF_MACHINE_TYPE_XTENSA, 0, reloc_types,
                     infile, outfile)) {
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
  return true;
}

XTENSAAssembler* NewXTENSAAssembler(String* infile, String* outfile) {
  XTENSAAssembler* assembler = malloc(sizeof(XTENSAAssembler));
  XTENSAAssemblerInit(assembler, infile, outfile);
  return assembler;
}

// Destruct the assembler.
void XTENSAAssemblerDestruct(XTENSAAssembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void XTENSAAssemblerDelete(XTENSAAssembler* assembler) {
  XTENSAAssemblerDestruct(assembler);
  free(assembler);
}

// Main assembly function.  This is called by the assembler driver.  It will be
// called twice, one for each pass.
// In pass 1 we parse everything and define all the symbols.
// In pass 2 we also parse everything but we also insert the binary instructions
//    and data into the buffers and expect all symbols to be defined.
void AssembleXTENSAInstruction(Assembler* base, String* word) {
  XTENSAAssembler* assembler = (XTENSAAssembler*)base;

  void* asm_func = MapFindPointerKey(&assembler->instructions, word->value);
  if (asm_func != NULL) {
    void (*func)(XTENSAAssembler*) = asm_func;
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
    {"sp", XTENSA_SP_REG},
    {"float", XTENSA_FP_REG},
    {"ra", XTENSA_RET_REG},
    {"zero", XTENSA_INT_ZERO_REG},
    {NULL, 0},
};

// Prefixed register naems.
static struct PrefixedReg {
  const char* prefix;  // Register prefix.
  int prefix_length;   // Length of prefix.
  int num_start;
  int range_start;
  int range_end;
  XTENSARegisterType type;
} prefixed_reg_names[] = {
    {"a", 1, 0, 0, XTENSA_NUM_INT_REGS - 1, kXTENSARegTypeInt},
    {"s", 1, 0, XTENSA_INT_SAVED_START_1, XTENSA_INT_SAVED_END_1,
     kXTENSARegTypeInt},
    {"s", 1, 2, XTENSA_INT_SAVED_START_2, XTENSA_INT_SAVED_END_2,
     kXTENSARegTypeInt},
    {"t", 1, 0, XTENSA_INT_TEMP_START_1, XTENSA_INT_TEMP_END_1,
     kXTENSARegTypeInt},
    {"t", 1, 3, XTENSA_INT_TEMP_START_2, XTENSA_INT_TEMP_END_2,
     kXTENSARegTypeInt},
    {"fa", 2, 0, XTENSA_FP_ARG_START, XTENSA_FP_ARG_END, kXTENSARegTypeFloat},
    {"fs", 2, 0, XTENSA_FP_SAVED_START_1, XTENSA_FP_SAVED_END_1,
     kXTENSARegTypeFloat},
    {"fs", 2, 2, XTENSA_FP_SAVED_START_2, XTENSA_FP_SAVED_END_2,
     kXTENSARegTypeFloat},
    {"ft", 2, 0, XTENSA_FP_TEMP_START_1, XTENSA_FP_TEMP_END_1,
     kXTENSARegTypeFloat},
    {"ft", 2, 2, XTENSA_FP_TEMP_START_2, XTENSA_FP_TEMP_END_2,
     kXTENSARegTypeFloat},
    {"f", 1, 0, 0, XTENSA_NUM_FLOAT_REGS, kXTENSARegTypeFloat},
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
                      XTENSARegisterType* reg_type) {
  for (size_t i = 0; known_reg_names[i].name != NULL; i++) {
    if (StringEqual(reg_name, known_reg_names[i].name)) {
      *reg_num = known_reg_names[i].number;
      *reg_type = kXTENSARegTypeInt;  // All int regs.
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

static bool RegisterName(XTENSAAssembler* assembler, int* num,
                         XTENSARegisterType* type) {
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

static int Register(XTENSAAssembler* assembler, XTENSARegisterType type_needed,
                    const char* type_name) {
  int num;
  XTENSARegisterType type;
  if (!RegisterName(assembler, &num, &type)) {
    AssemblerError(&ASM, "Expected %s register name", type_name);
    return 0;
  }

  if (type != type_needed) {
    const char* reg_type = type == kXTENSARegTypeInt ? "integer" : "float";
    AssemblerError(&ASM, "Invalid register type; got %s expected %s", reg_type,
                   type_name);
    return 0;
  }
  return num;
}

static bool ParseRegisterTriple(XTENSAAssembler* assembler,
                                XTENSARegisterType type_needed,
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

static bool ParseRegisterPair(XTENSAAssembler* assembler,
                              XTENSARegisterType type_needed,
                              const char* type_name, int* regs) {
  regs[0] = Register(assembler, type_needed, type_name);
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Missing comma");
    return false;
  }
  regs[1] = Register(assembler, type_needed, type_name);
  return true;
}

static AssemblerSymbol* GetOrCreateSymbol(XTENSAAssembler* assembler,
                                          const char* symbol_name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&ASM, symbol_name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(symbol_name, ASMO.current_section, SYM_TYPE(func),
                             SYM_BIND(local), 0);
    AssemblerInsertSymbol(&ASM, sym);
  }
  return sym;
}

static bool EmitXtensa(XTENSAAssembler* assembler,
                       XtensaInstruction instruction) {
  uint8_t bytes[3];
  if (!XtensaEncode(&instruction, bytes)) {
    AssemblerError(&ASM, "Xtensa instruction operand is out of range");
    return false;
  }
  for (size_t i = 0; i < sizeof(bytes); i++) {
    AssemblerEmitByte(&ASM, ASMO.current_section, bytes[i]);
  }
  return true;
}

static bool MatchComma(XTENSAAssembler* assembler) {
  if (LexMatch(&ASM.lex, TOK(comma))) {
    return true;
  }
  AssemblerError(&ASM, "Missing comma");
  return false;
}

static void Assemble_xt_mov(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaOr,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .rt = (uint8_t)regs[1],
                        });
}

static void Assemble_xt_nop(XTENSAAssembler* assembler) {
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaOr,
                            .rd = 1,
                            .rs = 1,
                            .rt = 1,
                        });
}

static void Assemble_xt_not(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  int scratch = 15;
  while (scratch == regs[0] || scratch == regs[1]) {
    --scratch;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)scratch,
                            .immediate = -1,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaXor,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .rt = (uint8_t)scratch,
                        });
}

static void Assemble_xt_neg(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaNeg,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                        });
}

static void EmitLoadConstant(XTENSAAssembler* assembler, int reg,
                             uint32_t value) {
  int top = (int)(value >> 28);
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)reg,
                            .immediate = top,
                        });
  for (int shift = 21; shift >= 0; shift -= 7) {
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaSlli,
                              .rd = (uint8_t)reg,
                              .rs = (uint8_t)reg,
                              .immediate = 7,
                          });
    int chunk = (int)((value >> shift) & 0x7f);
    if (chunk != 0) {
      EmitXtensa(assembler, (XtensaInstruction){
                                .kind = kXtensaAddi,
                                .rd = (uint8_t)reg,
                                .rs = (uint8_t)reg,
                                .immediate = chunk,
                            });
    }
  }
}

static void Assemble_xt_li(XTENSAAssembler* assembler) {
  int reg = Register(assembler, kXTENSARegTypeInt, "integer");
  if (!MatchComma(assembler)) {
    return;
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String symbol_name;
    StringInit(&symbol_name, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    AssemblerSymbol* symbol = GetOrCreateSymbol(assembler, symbol_name.value);

    uint32_t jump_place = (uint32_t)AssemblerCurrentAddress(&ASM);
    uint32_t literal = (jump_place + 3 + 3) & ~3u;
    uint32_t after = literal + 4;
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaJ,
                              .immediate = (int32_t)(after - (jump_place + 4)),
                          });
    while ((AssemblerCurrentAddress(&ASM) & 3) != 0) {
      AssemblerEmitByte(&ASM, ASMO.current_section, 0);
    }
    AssemblerAddRelocation(
        &ASM,
        NewAssemblerRelocation(symbol, R_XTENSA_32, ASMO.current_section,
                               (int32_t)AssemblerCurrentAddress(&ASM), 0));
    AssemblerEmitWord(&ASM, ASMO.current_section, 0);
    uint32_t load_place = (uint32_t)AssemblerCurrentAddress(&ASM);
    EmitXtensa(assembler,
               (XtensaInstruction){
                   .kind = kXtensaL32r,
                   .rd = (uint8_t)reg,
                   .immediate = (int32_t)(literal - ((load_place + 3) & ~3u)),
               });
    StringDestruct(&symbol_name);
    return;
  }

  int64_t value = AssemblerEvaluateExpression(&ASM);
  if (value >= -2048 && value <= 2047) {
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaMovi,
                              .rd = (uint8_t)reg,
                              .immediate = (int32_t)value,
                          });
  } else {
    EmitLoadConstant(assembler, reg, (uint32_t)value);
  }
}

static void Assemble_xt_entry(XTENSAAssembler* assembler) {
  int reg = Register(assembler, kXTENSARegTypeInt, "integer");
  if (!MatchComma(assembler)) {
    return;
  }
  int64_t frame_size = AssemblerEvaluateExpression(&ASM);
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaEntry,
                            .rs = (uint8_t)reg,
                            .immediate = (int32_t)frame_size,
                        });
}

static void Assemble_xt_retw(XTENSAAssembler* assembler) {
  EmitXtensa(assembler, (XtensaInstruction){.kind = kXtensaRetw});
}

static void Assemble_xt_call8(XTENSAAssembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing CALL8 target");
    return;
  }
  String name;
  StringInit(&name, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);
  AssemblerSymbol* symbol = GetOrCreateSymbol(assembler, name.value);
  uint32_t place = (uint32_t)AssemblerCurrentAddress(&ASM);
  int32_t offset = 0;
  if (symbol->defined && symbol->section == ASMO.current_section) {
    offset = (int32_t)(symbol->value - ((place & ~3u) + 4));
  } else {
    AssemblerAddRelocation(
        &ASM, NewAssemblerRelocation(symbol, R_XTENSA_SLOT0_OP,
                                     ASMO.current_section, (int32_t)place, 0));
  }
  EmitXtensa(assembler,
             (XtensaInstruction){.kind = kXtensaCall8, .immediate = offset});
  StringDestruct(&name);
}

static void Assemble_xt_callx8(XTENSAAssembler* assembler) {
  int reg = Register(assembler, kXTENSARegTypeInt, "integer");
  EmitXtensa(assembler,
             (XtensaInstruction){.kind = kXtensaCallx8, .rs = (uint8_t)reg});
}

static void Assemble_xt_j(XTENSAAssembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing jump target");
    return;
  }
  String name;
  StringInit(&name, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);
  AssemblerSymbol* symbol = GetOrCreateSymbol(assembler, name.value);
  uint32_t place = (uint32_t)AssemblerCurrentAddress(&ASM);
  int32_t offset = 0;
  if (symbol->defined && symbol->section == ASMO.current_section) {
    offset = (int32_t)(symbol->value - (place + 4));
  } else {
    AssemblerAddRelocation(
        &ASM, NewAssemblerRelocation(symbol, R_XTENSA_SLOT0_OP,
                                     ASMO.current_section, (int32_t)place, 0));
  }
  EmitXtensa(assembler,
             (XtensaInstruction){.kind = kXtensaJ, .immediate = offset});
  StringDestruct(&name);
}

static void Assemble_xt_jx(XTENSAAssembler* assembler) {
  int reg = Register(assembler, kXTENSARegTypeInt, "integer");
  EmitXtensa(assembler,
             (XtensaInstruction){.kind = kXtensaJx, .rs = (uint8_t)reg});
}

static void AssembleXtensaRRR(XTENSAAssembler* assembler,
                              XtensaInstructionKind kind) {
  int regs[3];
  if (!ParseRegisterTriple(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kind,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .rt = (uint8_t)regs[2],
                        });
}

#define XTENSA_RRR_HANDLER(name, kind)                         \
  static void Assemble_xt_##name(XTENSAAssembler* assembler) { \
    AssembleXtensaRRR(assembler, kind);                        \
  }

XTENSA_RRR_HANDLER(add, kXtensaAdd)
XTENSA_RRR_HANDLER(sub, kXtensaSub)
XTENSA_RRR_HANDLER(and, kXtensaAnd)
XTENSA_RRR_HANDLER(or, kXtensaOr)
XTENSA_RRR_HANDLER(xor, kXtensaXor)

#undef XTENSA_RRR_HANDLER

static void Assemble_xt_addi(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  if (immediate >= -128 && immediate <= 127) {
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaAddi,
                              .rd = (uint8_t)regs[0],
                              .rs = (uint8_t)regs[1],
                              .immediate = (int32_t)immediate,
                          });
  } else {
    EmitLoadConstant(assembler, 15, (uint32_t)immediate);
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaAdd,
                              .rd = (uint8_t)regs[0],
                              .rs = (uint8_t)regs[1],
                              .rt = 15,
                          });
  }
}

static void Assemble_xt_slli(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaSlli,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .immediate = (int32_t)immediate,
                        });
}

static void Assemble_xt_srai(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaSrai,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .immediate = (int32_t)immediate,
                        });
}

static void Assemble_xt_srli(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  if (immediate < 0 || immediate > 31) {
    AssemblerError(&ASM, "SRLI shift is out of range");
    return;
  }
  int source = regs[1];
  while (immediate > 15) {
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaSrli,
                              .rd = (uint8_t)regs[0],
                              .rs = (uint8_t)source,
                              .immediate = 15,
                          });
    source = regs[0];
    immediate -= 15;
  }
  if (immediate != 0 || source != regs[0]) {
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaSrli,
                              .rd = (uint8_t)regs[0],
                              .rs = (uint8_t)source,
                              .immediate = (int32_t)immediate,
                          });
  }
}

static void AssembleXtensaSetLess(XTENSAAssembler* assembler,
                                  bool is_unsigned) {
  int regs[3];
  if (!ParseRegisterTriple(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  int left = regs[1];
  int right = regs[2];
  if (regs[0] == left || regs[0] == right) {
    int scratch = 15;
    while (scratch == regs[0] || scratch == left || scratch == right) {
      --scratch;
    }
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaOr,
                              .rd = (uint8_t)scratch,
                              .rs = (uint8_t)regs[0],
                              .rt = (uint8_t)regs[0],
                          });
    if (left == regs[0]) {
      left = scratch;
    }
    if (right == regs[0]) {
      right = scratch;
    }
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 0,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = is_unsigned ? kXtensaBgeu : kXtensaBge,
                            .rs = (uint8_t)left,
                            .rt = (uint8_t)right,
                            .immediate = 2,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 1,
                        });
}

static void Assemble_xt_slt(XTENSAAssembler* assembler) {
  AssembleXtensaSetLess(assembler, false);
}

static void Assemble_xt_sltu(XTENSAAssembler* assembler) {
  AssembleXtensaSetLess(assembler, true);
}

static void AssembleXtensaSetLessImmediate(XTENSAAssembler* assembler,
                                           bool is_unsigned) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  int scratch = regs[1] == 15 ? 14 : 15;
  int source = regs[1];
  if (regs[0] == source) {
    int source_scratch = scratch == 15 ? 14 : 15;
    while (source_scratch == regs[0] || source_scratch == scratch) {
      --source_scratch;
    }
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaOr,
                              .rd = (uint8_t)source_scratch,
                              .rs = (uint8_t)source,
                              .rt = (uint8_t)source,
                          });
    source = source_scratch;
  }
  EmitLoadConstant(assembler, scratch, (uint32_t)immediate);
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 0,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = is_unsigned ? kXtensaBgeu : kXtensaBge,
                            .rs = (uint8_t)source,
                            .rt = (uint8_t)scratch,
                            .immediate = 2,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 1,
                        });
}

static void Assemble_xt_slti(XTENSAAssembler* assembler) {
  AssembleXtensaSetLessImmediate(assembler, false);
}

static void Assemble_xt_sltiu(XTENSAAssembler* assembler) {
  AssembleXtensaSetLessImmediate(assembler, true);
}

static void AssembleXtensaALUImmediate(XTENSAAssembler* assembler,
                                       XtensaInstructionKind kind) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  EmitLoadConstant(assembler, 15, (uint32_t)immediate);
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kind,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .rt = 15,
                        });
}

static void Assemble_xt_andi(XTENSAAssembler* assembler) {
  AssembleXtensaALUImmediate(assembler, kXtensaAnd);
}

static void Assemble_xt_ori(XTENSAAssembler* assembler) {
  AssembleXtensaALUImmediate(assembler, kXtensaOr);
}

static void Assemble_xt_xori(XTENSAAssembler* assembler) {
  AssembleXtensaALUImmediate(assembler, kXtensaXor);
}

static void AssembleXtensaVariableShift(XTENSAAssembler* assembler,
                                        XtensaInstructionKind kind) {
  int regs[3];
  if (!ParseRegisterTriple(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  EmitXtensa(assembler,
             (XtensaInstruction){
                 .kind = kind == kXtensaSll ? kXtensaSsl : kXtensaSsr,
                 .rs = (uint8_t)regs[2],
             });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kind,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                        });
}

static void Assemble_xt_sll(XTENSAAssembler* assembler) {
  AssembleXtensaVariableShift(assembler, kXtensaSll);
}

static void Assemble_xt_srl(XTENSAAssembler* assembler) {
  AssembleXtensaVariableShift(assembler, kXtensaSrl);
}

static void Assemble_xt_sra(XTENSAAssembler* assembler) {
  AssembleXtensaVariableShift(assembler, kXtensaSra);
}

static void AssembleXtensaMulDiv(XTENSAAssembler* assembler,
                                 XtensaInstructionKind kind) {
  int regs[3];
  if (!ParseRegisterTriple(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kind,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .rt = (uint8_t)regs[2],
                        });
}

#define XTENSA_MULDIV_HANDLER(name, kind)                      \
  static void Assemble_xt_##name(XTENSAAssembler* assembler) { \
    AssembleXtensaMulDiv(assembler, kind);                     \
  }

XTENSA_MULDIV_HANDLER(mul, kXtensaMull)
XTENSA_MULDIV_HANDLER(div, kXtensaQuos)
XTENSA_MULDIV_HANDLER(divu, kXtensaQuou)
XTENSA_MULDIV_HANDLER(rem, kXtensaRems)
XTENSA_MULDIV_HANDLER(remu, kXtensaRemu)

#undef XTENSA_MULDIV_HANDLER

static void Assemble_xt_break(XTENSAAssembler* assembler) {
  int64_t first = AssemblerEvaluateExpression(&ASM);
  int64_t second = 0;
  if (LexMatch(&ASM.lex, TOK(comma))) {
    second = AssemblerEvaluateExpression(&ASM);
  }
  if (first < 0 || first > 15 || second < 0 || second > 15) {
    AssemblerError(&ASM, "BREAK immediates must be in the range 0..15");
    return;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaBreak,
                            .immediate = (int32_t)((first << 4) | second),
                        });
}

static void AssembleXtensaMemory(XTENSAAssembler* assembler,
                                 XtensaInstructionKind kind) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  int64_t immediate = AssemblerEvaluateExpression(&ASM);
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kind,
                            .rd = (uint8_t)regs[0],
                            .rs = (uint8_t)regs[1],
                            .immediate = (int32_t)immediate,
                        });
}

#define XTENSA_MEMORY_HANDLER(name, kind)                      \
  static void Assemble_xt_##name(XTENSAAssembler* assembler) { \
    AssembleXtensaMemory(assembler, kind);                     \
  }

XTENSA_MEMORY_HANDLER(l8ui, kXtensaL8ui)
XTENSA_MEMORY_HANDLER(l16ui, kXtensaL16ui)
XTENSA_MEMORY_HANDLER(l16si, kXtensaL16si)
XTENSA_MEMORY_HANDLER(l32i, kXtensaL32i)
XTENSA_MEMORY_HANDLER(s8i, kXtensaS8i)
XTENSA_MEMORY_HANDLER(s16i, kXtensaS16i)
XTENSA_MEMORY_HANDLER(s32i, kXtensaS32i)

#undef XTENSA_MEMORY_HANDLER

static void EmitJumpToSymbol(XTENSAAssembler* assembler,
                             AssemblerSymbol* symbol) {
  uint32_t place = (uint32_t)AssemblerCurrentAddress(&ASM);
  int32_t offset = 0;
  if (symbol->defined && symbol->section == ASMO.current_section) {
    offset = (int32_t)(symbol->value - (place + 4));
  } else {
    AssemblerAddRelocation(
        &ASM, NewAssemblerRelocation(symbol, R_XTENSA_SLOT0_OP,
                                     ASMO.current_section, (int32_t)place, 0));
  }
  EmitXtensa(assembler,
             (XtensaInstruction){.kind = kXtensaJ, .immediate = offset});
}

static AssemblerSymbol* ParseBranchTarget(XTENSAAssembler* assembler) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Missing branch target");
    return NULL;
  }
  AssemblerSymbol* symbol =
      GetOrCreateSymbol(assembler, ASM.lex.spelling.value);
  LexNextToken(&ASM.lex);
  return symbol;
}

static void AssembleXtensaBranch(XTENSAAssembler* assembler,
                                 XtensaInstructionKind inverse) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs) ||
      !MatchComma(assembler)) {
    return;
  }
  AssemblerSymbol* target = ParseBranchTarget(assembler);
  if (target == NULL) {
    return;
  }
  // The eight-bit register branch is expanded to an inverse short branch over
  // a J.  This gives compiler branches the full J range without linker-time
  // instruction growth.
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = inverse,
                            .rs = (uint8_t)regs[0],
                            .rt = (uint8_t)regs[1],
                            .immediate = 2,
                        });
  EmitJumpToSymbol(assembler, target);
}

static void AssembleXtensaZeroBranch(XTENSAAssembler* assembler,
                                     XtensaInstructionKind inverse) {
  int reg = Register(assembler, kXTENSARegTypeInt, "integer");
  if (!MatchComma(assembler)) {
    return;
  }
  AssemblerSymbol* target = ParseBranchTarget(assembler);
  if (target == NULL) {
    return;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = inverse,
                            .rs = (uint8_t)reg,
                            .immediate = 2,
                        });
  EmitJumpToSymbol(assembler, target);
}

#define XTENSA_BRANCH_HANDLER(name, inverse)                   \
  static void Assemble_xt_##name(XTENSAAssembler* assembler) { \
    AssembleXtensaBranch(assembler, inverse);                  \
  }

XTENSA_BRANCH_HANDLER(beq, kXtensaBne)
XTENSA_BRANCH_HANDLER(bne, kXtensaBeq)
XTENSA_BRANCH_HANDLER(blt, kXtensaBge)
XTENSA_BRANCH_HANDLER(bge, kXtensaBlt)
XTENSA_BRANCH_HANDLER(bltu, kXtensaBgeu)
XTENSA_BRANCH_HANDLER(bgeu, kXtensaBltu)

#undef XTENSA_BRANCH_HANDLER

#define XTENSA_ZERO_BRANCH_HANDLER(name, inverse)              \
  static void Assemble_xt_##name(XTENSAAssembler* assembler) { \
    AssembleXtensaZeroBranch(assembler, inverse);              \
  }

XTENSA_ZERO_BRANCH_HANDLER(beqz, kXtensaBnez)
XTENSA_ZERO_BRANCH_HANDLER(bnez, kXtensaBeqz)
XTENSA_ZERO_BRANCH_HANDLER(bltz, kXtensaBgez)
XTENSA_ZERO_BRANCH_HANDLER(bgez, kXtensaBltz)

#undef XTENSA_ZERO_BRANCH_HANDLER

static void AssembleXtensaSetZeroCondition(XTENSAAssembler* assembler,
                                           XtensaInstructionKind skip_kind) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  int source = regs[1];
  if (regs[0] == source) {
    int scratch = regs[0] == 15 ? 14 : 15;
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaOr,
                              .rd = (uint8_t)scratch,
                              .rs = (uint8_t)source,
                              .rt = (uint8_t)source,
                          });
    source = scratch;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 0,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = skip_kind,
                            .rs = (uint8_t)source,
                            .immediate = 2,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 1,
                        });
}

static void Assemble_xt_seqz(XTENSAAssembler* assembler) {
  AssembleXtensaSetZeroCondition(assembler, kXtensaBnez);
}

static void Assemble_xt_snez(XTENSAAssembler* assembler) {
  AssembleXtensaSetZeroCondition(assembler, kXtensaBeqz);
}

static void Assemble_xt_sltz(XTENSAAssembler* assembler) {
  AssembleXtensaSetZeroCondition(assembler, kXtensaBgez);
}

static void Assemble_xt_sgtz(XTENSAAssembler* assembler) {
  int regs[2];
  if (!ParseRegisterPair(assembler, kXTENSARegTypeInt, "integer", regs)) {
    return;
  }
  int source = regs[1];
  int zero = 15;
  while (zero == regs[0] || zero == source) {
    --zero;
  }
  if (regs[0] == source) {
    int scratch = zero - 1;
    while (scratch == regs[0] || scratch == zero) {
      --scratch;
    }
    EmitXtensa(assembler, (XtensaInstruction){
                              .kind = kXtensaOr,
                              .rd = (uint8_t)scratch,
                              .rs = (uint8_t)source,
                              .rt = (uint8_t)source,
                          });
    source = scratch;
  }
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)zero,
                            .immediate = 0,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 0,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaBge,
                            .rs = (uint8_t)zero,
                            .rt = (uint8_t)source,
                            .immediate = 2,
                        });
  EmitXtensa(assembler, (XtensaInstruction){
                            .kind = kXtensaMovi,
                            .rd = (uint8_t)regs[0],
                            .immediate = 1,
                        });
}

#undef ASM
