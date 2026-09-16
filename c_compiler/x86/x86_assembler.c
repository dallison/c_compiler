//
//  x86_assembler.c
//  c_compiler
//
//  AT&T-syntax x86 assembler using the generic ELF assembler driver.
//

#include "x86_assembler.h"
#include "x86_profile.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
#include "map.h"
#include "x86_machine.h"

typedef enum {
  kX86Size8 = 1,
  kX86Size16 = 2,
  kX86Size32 = 4,
  kX86Size64 = 8,
} X86Size;

typedef struct {
  int num;
  X86Size size;
  bool is_xmm;
} X86Reg;

typedef enum {
  kX86OpReg,
  kX86OpImm,
  kX86OpMem,
} X86OpKind;

typedef struct {
  X86OpKind kind;
  X86Reg reg;
  int64_t imm;
  int64_t disp;
  X86Reg base;
  X86Reg index;
  int scale;
  AssemblerSymbol* sym;
  bool sym_known;
  int64_t sym_value;
  bool rip_relative;
  int segment_prefix;
  int reloc_type;
} X86Op;

typedef struct {
  X86Assembler* assembler;
  uint8_t bytes[16];
  size_t len;
  int rex;
  // Legacy / mandatory prefixes (0x66, 0xf2, 0xf3) must be emitted before the
  // REX prefix.  They are buffered separately so that EncodeFinish can place
  // them ahead of REX regardless of when the REX bits become known (e.g. the
  // REX.B for a memory operand of an SSE move is only computed while encoding
  // the operand, after the opcode bytes).
  uint8_t prefixes[4];
  size_t num_prefixes;
  // Number of bytes that follow the displacement field (e.g. a trailing
  // immediate).  Needed so RIP-relative displacements account for the full
  // instruction length.
  int tail_bytes;
  size_t modrm_pos;
  bool has_modrm;
  size_t disp_pos;
  int disp_size;
  size_t imm_pos;
  int imm_size;
} X86Encode;

static int CompareString(const void* a, const void* b) {
  MapKeyValue* s1 = (MapKeyValue*)a;
  MapKeyValue* s2 = (MapKeyValue*)b;
  return strcmp(s1->key.p, s2->key.p);
}

static AssemblerSymbol* GetOrCreateSymbol(X86Assembler* assembler,
                                          const char* name) {
  AssemblerSymbol* sym = AssemblerFindSymbol(&assembler->base, name);
  if (sym == NULL) {
    sym = NewAssemblerSymbol(name, assembler->base.object.current_section,
                             SYM_TYPE(none), SYM_BIND(local), 0);
    sym->is_forward_declared = true;
    AssemblerInsertSymbol(&assembler->base, sym);
  }
  return sym;
}

static void EncodeInit(X86Encode* enc, X86Assembler* assembler) {
  enc->assembler = assembler;
  enc->len = 0;
  enc->rex = -1;
  enc->num_prefixes = 0;
  enc->tail_bytes = 0;
  enc->has_modrm = false;
  enc->disp_pos = 0;
  enc->disp_size = 0;
  enc->imm_pos = 0;
  enc->imm_size = 0;
}

static void EncodeByte(X86Encode* enc, uint8_t byte) {
  assert(enc->len < sizeof(enc->bytes));
  enc->bytes[enc->len++] = byte;
}

// Buffer a legacy / mandatory prefix that must precede the REX prefix.
static void EncodeLegacyPrefix(X86Encode* enc, uint8_t byte) {
  assert(enc->num_prefixes < sizeof(enc->prefixes));
  enc->prefixes[enc->num_prefixes++] = byte;
}

static void SetRexW(X86Encode* enc) {
  if (!enc->assembler->profile->supports_rex) {
    return;
  }
  if (enc->rex < 0) {
    enc->rex = 0x40;
  }
  enc->rex |= 0x08;
}

static void SetRexR(X86Encode* enc, int reg) {
  if (!enc->assembler->profile->supports_rex) {
    if (reg & 8) {
      AssemblerError(&enc->assembler->base, "Register r8-r15 not available in i386 mode");
    }
    return;
  }
  if (enc->rex < 0) {
    enc->rex = 0x40;
  }
  if (reg & 8) {
    enc->rex |= 0x04;
  }
}

static void SetRexX(X86Encode* enc, int index) {
  if (!enc->assembler->profile->supports_rex) {
    if (index & 8) {
      AssemblerError(&enc->assembler->base, "Register r8-r15 not available in i386 mode");
    }
    return;
  }
  if (enc->rex < 0) {
    enc->rex = 0x40;
  }
  if (index & 8) {
    enc->rex |= 0x02;
  }
}

static void SetRexB(X86Encode* enc, int rm) {
  if (!enc->assembler->profile->supports_rex) {
    if (rm & 8) {
      AssemblerError(&enc->assembler->base, "Register r8-r15 not available in i386 mode");
    }
    return;
  }
  if (enc->rex < 0) {
    enc->rex = 0x40;
  }
  if (rm & 8) {
    enc->rex |= 0x01;
  }
}

static void EncodeModRM(X86Encode* enc, int mod, int reg, int rm) {
  SetRexR(enc, reg);
  SetRexB(enc, rm);
  enc->modrm_pos = enc->len;
  enc->has_modrm = true;
  EncodeByte(enc, (uint8_t)((mod << 6) | ((reg & 7) << 3) | (rm & 7)));
}

static void EncodeSIB(X86Encode* enc, int scale, int index, int base) {
  SetRexX(enc, index);
  SetRexB(enc, base);
  int scale_bits = 0;
  switch (scale) {
    case 1:
      scale_bits = 0;
      break;
    case 2:
      scale_bits = 1;
      break;
    case 4:
      scale_bits = 2;
      break;
    case 8:
      scale_bits = 3;
      break;
    default:
      AssemblerError(&enc->assembler->base, "Invalid SIB scale %d", scale);
      scale_bits = 0;
      break;
  }
  int index_field = (index < 0) ? 4 : (index & 7);
  EncodeByte(enc, (uint8_t)((scale_bits << 6) | (index_field << 3) | (base & 7)));
}

static void EncodeDisp(X86Encode* enc, int size, int32_t disp) {
  enc->disp_pos = enc->len;
  enc->disp_size = size;
  if (size == 1) {
    EncodeByte(enc, (uint8_t)disp);
  } else if (size == 4) {
    for (int i = 0; i < 4; i++) {
      EncodeByte(enc, (uint8_t)(disp >> (8 * i)));
    }
  } else {
    AssemblerError(&enc->assembler->base, "Unsupported displacement size %d",
                   size);
  }
}

static void EncodeImm(X86Encode* enc, int size, int64_t imm) {
  enc->imm_pos = enc->len;
  enc->imm_size = size;
  for (int i = 0; i < size; i++) {
    EncodeByte(enc, (uint8_t)(imm >> (8 * i)));
  }
}

static void EncodeFinish(X86Encode* enc) {
  Assembler* base = &enc->assembler->base;
  for (size_t i = 0; i < enc->num_prefixes; i++) {
    AssemblerEmitByte(base, base->object.current_section, enc->prefixes[i]);
  }
  if (enc->rex >= 0) {
    AssemblerEmitByte(base, base->object.current_section, (uint8_t)enc->rex);
  }
  for (size_t i = 0; i < enc->len; i++) {
    AssemblerEmitByte(base, base->object.current_section, enc->bytes[i]);
  }
}

static bool ParseXmmSuffix(const char* name, size_t len, X86Reg* reg) {
  if (len < 4 || strncmp(name, "xmm", 3) != 0) {
    return false;
  }
  const char* num_str = name + 3;
  size_t num_len = len - 3;
  if (num_len == 0) {
    return false;
  }
  int num = 0;
  for (size_t i = 0; i < num_len; i++) {
    if (!isdigit((unsigned char)num_str[i])) {
      return false;
    }
    num = num * 10 + (num_str[i] - '0');
  }
  if (num < 0 || num > 15) {
    return false;
  }
  reg->num = num;
  reg->size = kX86Size32;
  reg->is_xmm = true;
  return true;
}

static bool ParseRegSuffix(const char* name, size_t len, X86Reg* reg) {
  reg->is_xmm = false;
  if (ParseXmmSuffix(name, len, reg)) {
    return true;
  }
  static struct {
    const char* name;
    int num;
    X86Size size;
  } regs[] = {
      {"rax", X86_REG_RAX, kX86Size64}, {"rbx", X86_REG_RBX, kX86Size64},
      {"rcx", X86_REG_RCX, kX86Size64}, {"rdx", X86_REG_RDX, kX86Size64},
      {"rsi", X86_REG_RSI, kX86Size64}, {"rdi", X86_REG_RDI, kX86Size64},
      {"rbp", X86_REG_RBP, kX86Size64}, {"rsp", X86_REG_RSP, kX86Size64},
      {"r8", X86_REG_R8, kX86Size64},   {"r9", X86_REG_R9, kX86Size64},
      {"r10", X86_REG_R10, kX86Size64}, {"r11", X86_REG_R11, kX86Size64},
      {"r12", X86_REG_R12, kX86Size64}, {"r13", X86_REG_R13, kX86Size64},
      {"r14", X86_REG_R14, kX86Size64}, {"r15", X86_REG_R15, kX86Size64},
      {"eax", X86_REG_RAX, kX86Size32}, {"ebx", X86_REG_RBX, kX86Size32},
      {"ecx", X86_REG_RCX, kX86Size32}, {"edx", X86_REG_RDX, kX86Size32},
      {"esi", X86_REG_RSI, kX86Size32}, {"edi", X86_REG_RDI, kX86Size32},
      {"ebp", X86_REG_RBP, kX86Size32}, {"esp", X86_REG_RSP, kX86Size32},
      {"r8d", X86_REG_R8, kX86Size32},  {"r9d", X86_REG_R9, kX86Size32},
      {"r10d", X86_REG_R10, kX86Size32},{"r11d", X86_REG_R11, kX86Size32},
      {"r12d", X86_REG_R12, kX86Size32},{"r13d", X86_REG_R13, kX86Size32},
      {"r14d", X86_REG_R14, kX86Size32},{"r15d", X86_REG_R15, kX86Size32},
      {"ax", X86_REG_RAX, kX86Size16},  {"bx", X86_REG_RBX, kX86Size16},
      {"cx", X86_REG_RCX, kX86Size16},  {"dx", X86_REG_RDX, kX86Size16},
      {"si", X86_REG_RSI, kX86Size16},  {"di", X86_REG_RDI, kX86Size16},
      {"bp", X86_REG_RBP, kX86Size16},  {"sp", X86_REG_RSP, kX86Size16},
      {"r8w", X86_REG_R8, kX86Size16},  {"r9w", X86_REG_R9, kX86Size16},
      {"al", X86_REG_RAX, kX86Size8},   {"bl", X86_REG_RBX, kX86Size8},
      {"cl", X86_REG_RCX, kX86Size8},   {"dl", X86_REG_RDX, kX86Size8},
      {"sil", X86_REG_RSI, kX86Size8},  {"dil", X86_REG_RDI, kX86Size8},
      {"bpl", X86_REG_RBP, kX86Size8},  {"spl", X86_REG_RSP, kX86Size8},
      {"r8b", X86_REG_R8, kX86Size8},   {"r9b", X86_REG_R9, kX86Size8},
      {"rip", X86_REG_RBP, kX86Size64}, /* placeholder num for %rip */
  };
  for (size_t i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
    if (strlen(regs[i].name) == len &&
        strncmp(name, regs[i].name, len) == 0) {
      reg->num = regs[i].num;
      reg->size = regs[i].size;
      return true;
    }
  }
  return false;
}

#define ASM assembler->base
#define ASMO (assembler->base.object)

static bool ParseRegister(X86Assembler* assembler, X86Reg* reg,
                          bool allow_rip, bool* is_rip) {
  if (is_rip != NULL) {
    *is_rip = false;
  }
  if (LexMatch(&ASM.lex, TOK(percent))) {
    // AT&T register syntax.
  }
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    AssemblerError(&ASM, "Register expected");
    return false;
  }
  const char* name = ASM.lex.spelling.value;
  size_t len = ASM.lex.spelling.length;
  if (len == 3 && strncmp(name, "rip", 3) == 0) {
    if (!allow_rip) {
      AssemblerError(&ASM, "Unexpected %%rip");
      return false;
    }
    if (is_rip != NULL) {
      *is_rip = true;
    }
    reg->num = 0;
    reg->size = kX86Size64;
    reg->is_xmm = false;
    LexNextToken(&ASM.lex);
    return true;
  }
  if (!ParseRegSuffix(name, len, reg)) {
    AssemblerError(&ASM, "Unknown register %.*s", (int)len, name);
    return false;
  }
  LexNextToken(&ASM.lex);
  return true;
}

static bool ParseMemoryTail(X86Assembler* assembler, X86Op* op) {
  if (!LexMatch(&ASM.lex, TOK(lparen))) {
    AssemblerError(&ASM, "Expected ( in memory operand");
    return false;
  }

  if (LexLookingAt(&ASM.lex, TOK(percent)) ||
      LexLookingAt(&ASM.lex, TOK(identifier))) {
    bool is_rip = false;
    X86Reg reg;
    if (!ParseRegister(assembler, &reg, /*allow_rip=*/true, &is_rip)) {
      return false;
    }
    if (is_rip) {
      op->rip_relative = true;
    } else {
      op->base = reg;
    }
  }

  if (LexMatch(&ASM.lex, TOK(comma))) {
    if (!ParseRegister(assembler, &op->index, /*allow_rip=*/false, NULL)) {
      return false;
    }
    if (LexMatch(&ASM.lex, TOK(comma))) {
      op->scale = (int)AssemblerEvaluateExpression(&ASM);
    }
  }

  if (!LexMatch(&ASM.lex, TOK(rparen))) {
    AssemblerError(&ASM, "Expected ) after memory operand");
    return false;
  }
  return true;
}

static bool AsmLookingAtLParen(Assembler* assembler) {
  size_t pos = assembler->lex.pos;
  while (pos < assembler->lex.line.length &&
         isspace((unsigned char)assembler->lex.line.value[pos])) {
    pos++;
  }
  return pos < assembler->lex.line.length &&
         assembler->lex.line.value[pos] == '(';
}

static void InitMemOp(X86Op* op) {
  op->kind = kX86OpMem;
  op->disp = 0;
  op->base = (X86Reg){.num = -1, .size = kX86Size64, .is_xmm = false};
  op->index = (X86Reg){.num = -1, .size = kX86Size64, .is_xmm = false};
  op->scale = 1;
  op->sym = NULL;
  op->sym_known = false;
  op->rip_relative = false;
  op->segment_prefix = 0;
  op->reloc_type = 0;
}

static int X86DefaultAbsReloc(const X86Profile* profile) {
  return profile->is_64bit ? R_X86_64_64 : R_386_32;
}

static int X86DefaultPcReloc(const X86Profile* profile, bool is_call) {
  if (!profile->is_64bit) {
    return R_386_PC32;
  }
  return is_call ? R_X86_64_PLT32 : R_X86_64_PC32;
}

static bool ParseBareSymbolImmediate(X86Assembler* assembler, X86Op* op) {
  String symbol_name;
  String suffix;
  StringInit(&symbol_name, NULL);
  StringInit(&suffix, NULL);
  AssemblerExtractSymbolSuffix(&ASM.lex.spelling, &symbol_name, &suffix);
  op->kind = kX86OpImm;
  op->imm = 0;
  op->sym = GetOrCreateSymbol(assembler, symbol_name.value);
  op->sym_known = false;
  op->reloc_type = X86DefaultAbsReloc(assembler->profile);
  if (!assembler->profile->is_64bit &&
      StringEqual(&symbol_name, "_GLOBAL_OFFSET_TABLE_")) {
    op->reloc_type = R_386_GOTPC;
    op->imm = 3;
  } else if (StringEqual(&suffix, "TPOFF")) {
    op->reloc_type =
        assembler->profile->is_64bit ? R_X86_64_TPOFF64 : R_386_32;
  } else if (!assembler->profile->is_64bit &&
             StringEqual(&suffix, "GOTPC")) {
    op->reloc_type = R_386_GOTPC;
    // The PIC sequence emitted by the i386 backend obtains the address of
    // its local pop instruction.  The relocation field is three bytes later.
    op->imm = 3;
  } else if (suffix.length > 0) {
    AssemblerError(&ASM, "Unsupported symbol suffix '@%s'", suffix.value);
    StringDestruct(&symbol_name);
    StringDestruct(&suffix);
    return false;
  }
  LexNextToken(&ASM.lex);
  StringDestruct(&symbol_name);
  StringDestruct(&suffix);
  return true;
}

static bool ParseSegmentOverrideFromCurrent(X86Assembler* assembler,
                                            int* segment_prefix) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    return false;
  }
  const char* name = ASM.lex.spelling.value;
  size_t len = ASM.lex.spelling.length;
  if (len == 2 && strncmp(name, "fs", 2) == 0) {
    *segment_prefix = 0x64;
  } else if (len == 2 && strncmp(name, "gs", 2) == 0) {
    *segment_prefix = 0x65;
  } else {
    return false;
  }
  LexNextToken(&ASM.lex);
  if (!LexMatch(&ASM.lex, TOK(colon))) {
    AssemblerError(&ASM, "Expected ':' after segment register");
    return false;
  }
  return true;
}

static bool ParseSegmentOverride(X86Assembler* assembler, int* segment_prefix) {
  if (!LexMatch(&ASM.lex, TOK(percent))) {
    return false;
  }
  return ParseSegmentOverrideFromCurrent(assembler, segment_prefix);
}

static bool ParseMemory(X86Assembler* assembler, X86Op* op) {
  InitMemOp(op);

  if (LexLookingAt(&ASM.lex, TOK(percent))) {
    size_t save_pos = ASM.lex.pos;
    int segment_prefix = 0;
    if (ParseSegmentOverride(assembler, &segment_prefix)) {
      op->segment_prefix = segment_prefix;
      if (LexLookingAt(&ASM.lex, TOK(number))) {
        op->disp = AssemblerEvaluateExpression(&ASM);
        return true;
      }
      if (LexLookingAt(&ASM.lex, TOK(lparen))) {
        return ParseMemoryTail(assembler, op);
      }
      AssemblerError(&ASM, "Expected displacement or '(' after segment override");
      return false;
    }
    ASM.lex.pos = save_pos;
  }

  if (LexLookingAt(&ASM.lex, TOK(lparen))) {
    return ParseMemoryTail(assembler, op);
  }

  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    String symbol_name;
    String suffix;
    StringInit(&symbol_name, NULL);
    StringInit(&suffix, NULL);
    AssemblerExtractSymbolSuffix(&ASM.lex.spelling, &symbol_name, &suffix);
    op->sym = GetOrCreateSymbol(assembler, symbol_name.value);
    op->sym_known =
        op->sym->defined && op->sym->section == ASMO.current_section;
    op->sym_value = op->sym->value;
    if (StringEqual(&suffix, "TPOFF")) {
      op->reloc_type = R_X86_64_TPOFF32;
    } else if (!assembler->profile->is_64bit &&
               StringEqual(&suffix, "GOT")) {
      op->reloc_type = R_386_GOT32;
    } else if (StringEqual(&suffix, "GOTPCREL")) {
      op->reloc_type = R_X86_64_GOTPCREL;
    } else if (StringEqual(&suffix, "TLSGD")) {
      op->reloc_type = R_X86_64_TLSGD;
    } else if (StringEqual(&suffix, "GOTTPOFF")) {
      op->reloc_type = R_X86_64_GOTTPOFF;
    } else if (suffix.length > 0) {
      AssemblerError(&ASM, "Unsupported symbol suffix '@%s'", suffix.value);
      StringDestruct(&symbol_name);
      StringDestruct(&suffix);
      return false;
    }
    LexNextToken(&ASM.lex);
    StringDestruct(&symbol_name);
    StringDestruct(&suffix);
    return ParseMemoryTail(assembler, op);
  }

  op->disp = AssemblerEvaluateExpression(&ASM);
  return ParseMemoryTail(assembler, op);
}

static bool ParseOperand(X86Assembler* assembler, X86Op* op) {
  memset(op, 0, sizeof(*op));

  if (LexLookingAt(&ASM.lex, TOK(number))) {
    if (AsmLookingAtLParen(&ASM)) {
      InitMemOp(op);
      op->disp = AssemblerEvaluateExpression(&ASM);
      return ParseMemoryTail(assembler, op);
    }
    op->kind = kX86OpImm;
    op->imm = AssemblerEvaluateExpression(&ASM);
    return true;
  }
  if (LexLookingAt(&ASM.lex, TOK(minus))) {
    LexNextToken(&ASM.lex);
    if (LexLookingAt(&ASM.lex, TOK(number)) && AsmLookingAtLParen(&ASM)) {
      InitMemOp(op);
      op->disp = -AssemblerEvaluateExpression(&ASM);
      return ParseMemoryTail(assembler, op);
    }
    op->kind = kX86OpImm;
    op->imm = -AssemblerEvaluateExpression(&ASM);
    return true;
  }
  if (LexMatch(&ASM.lex, TOK(percent))) {
    int segment_prefix = 0;
    if (ParseSegmentOverrideFromCurrent(assembler, &segment_prefix)) {
      InitMemOp(op);
      op->segment_prefix = segment_prefix;
      if (LexLookingAt(&ASM.lex, TOK(number))) {
        op->disp = AssemblerEvaluateExpression(&ASM);
        return true;
      }
      if (LexLookingAt(&ASM.lex, TOK(lparen))) {
        return ParseMemoryTail(assembler, op);
      }
      AssemblerError(&ASM, "Expected displacement or '(' after segment override");
      return false;
    }
    op->kind = kX86OpReg;
    return ParseRegister(assembler, &op->reg, /*allow_rip=*/false, NULL);
  }
  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    X86Reg reg;
    if (ParseRegSuffix(ASM.lex.spelling.value, ASM.lex.spelling.length, &reg)) {
      op->kind = kX86OpReg;
      op->reg = reg;
      LexNextToken(&ASM.lex);
      return true;
    }
    if (!AsmLookingAtLParen(&ASM)) {
      return ParseBareSymbolImmediate(assembler, op);
    }
    return ParseMemory(assembler, op);
  }
  if (LexLookingAt(&ASM.lex, TOK(lparen))) {
    return ParseMemory(assembler, op);
  }

  AssemblerError(&ASM, "Expected operand");
  return false;
}

static bool ExpectComma(X86Assembler* assembler) {
  if (!LexMatch(&ASM.lex, TOK(comma))) {
    AssemblerError(&ASM, "Expected comma");
    return false;
  }
  return true;
}

static int PickMemDispSize(int64_t disp) {
  if (disp >= -128 && disp <= 127) {
    return 1;
  }
  return 4;
}

static void EncodeMemOperand(X86Encode* enc, int reg_field, const X86Op* mem) {
  Assembler* base = &enc->assembler->base;
  if (mem->segment_prefix != 0) {
    EncodeLegacyPrefix(enc, (uint8_t)mem->segment_prefix);
  }
  if (mem->segment_prefix != 0 && mem->base.num < 0 && mem->index.num < 0 &&
      !mem->rip_relative && mem->sym == NULL) {
    SetRexW(enc);
    EncodeModRM(enc, 0, reg_field, 4);
    EncodeSIB(enc, 1, 4, 5);
    enc->disp_pos = enc->len;
    enc->disp_size = 4;
    EncodeDisp(enc, 4, (int32_t)mem->disp);
    return;
  }
  if (mem->rip_relative || (mem->base.num < 0 && mem->sym != NULL)) {
    if (!enc->assembler->profile->supports_rex) {
      EncodeModRM(enc, 0, reg_field, 5);
      enc->disp_pos = enc->len;
      enc->disp_size = 4;
      if (mem->sym != NULL &&
          (!mem->sym_known || mem->reloc_type != 0)) {
        // ELF32 uses REL, so the addend belongs in the relocated field.
        EncodeDisp(enc, 4, (int32_t)mem->disp);
        int reloc_type =
            mem->reloc_type != 0 ? mem->reloc_type : R_386_32;
        int32_t disp_offset = (int32_t)(AssemblerCurrentAddress(base) +
                                        enc->num_prefixes + enc->len);
        AssemblerRelocation* reloc = NewAssemblerRelocation(
            mem->sym, reloc_type, base->object.current_section, disp_offset,
            0);
        AssemblerAddRelocation(base, reloc);
      } else {
        int64_t target =
            mem->sym_known ? mem->sym_value + mem->disp : mem->disp;
        EncodeDisp(enc, 4, (int32_t)target);
      }
      return;
    }
    SetRexW(enc);
    EncodeModRM(enc, 0, reg_field, 5);
    enc->disp_pos = enc->len;
    enc->disp_size = 4;
    int64_t next_ip = AssemblerCurrentAddress(base) + enc->num_prefixes +
                      (enc->rex >= 0 ? 1 : 0) + enc->len + 4 + enc->tail_bytes;
    if (mem->sym != NULL &&
        (!mem->sym_known || mem->reloc_type != 0)) {
      for (int i = 0; i < 4; i++) {
        EncodeByte(enc, 0);
      }
      int reloc_type = mem->reloc_type != 0 ? mem->reloc_type : R_X86_64_PC32;
      if (mem->reloc_type == 0 && base->object.pic &&
          (mem->sym->binding == SYM_BIND(global) ||
           mem->sym->binding == SYM_BIND(weak))) {
        reloc_type = R_X86_64_GOTPCREL;
      }
      // The relocation patches the 4-byte displacement field, which sits 4
      // bytes before the end of the instruction (next_ip).  The standard
      // RIP-relative addend is -4 (plus any explicit displacement), because
      // the CPU computes the effective address relative to next_ip = P + 4.
      int32_t disp_offset = (int32_t)(next_ip - 4);
      AssemblerRelocation* reloc = NewAssemblerRelocation(
          mem->sym, reloc_type, base->object.current_section, disp_offset,
          (int32_t)(mem->disp - 4));
      AssemblerAddRelocation(base, reloc);
    } else {
      int64_t target = mem->sym_known ? mem->sym_value + mem->disp : mem->disp;
      EncodeDisp(enc, 4, (int32_t)(target - next_ip));
    }
    return;
  }

  int base_reg = mem->base.num;
  int index = mem->index.num;
  int64_t disp = mem->disp;
  if (mem->sym != NULL &&
      (mem->reloc_type == R_X86_64_TPOFF64 ||
       mem->reloc_type == R_X86_64_TPOFF32 ||
       mem->reloc_type == R_386_GOT32)) {
    if (base_reg < 0) {
      AssemblerError(&enc->assembler->base,
                     "TPOFF memory operand requires a base register");
      return;
    }
    bool need_sib = (base_reg & 7) == (X86_REG_RSP & 7) || index >= 0;
    if (need_sib) {
      EncodeModRM(enc, 2, reg_field, 4);
      EncodeSIB(enc, mem->scale, index < 0 ? 4 : index, base_reg);
    } else {
      EncodeModRM(enc, 2, reg_field, base_reg);
    }
    enc->disp_pos = enc->len;
    enc->disp_size = 4;
    int32_t in_place_addend =
        enc->assembler->profile->uses_rela ? 0 : (int32_t)mem->disp;
    EncodeDisp(enc, 4, in_place_addend);
    int64_t next_ip = AssemblerCurrentAddress(base) + enc->num_prefixes +
                      (enc->rex >= 0 ? 1 : 0) + enc->len + enc->tail_bytes;
    int32_t disp_offset = (int32_t)(next_ip - 4);
    AssemblerRelocation* reloc = NewAssemblerRelocation(
        mem->sym, mem->reloc_type, base->object.current_section, disp_offset,
        enc->assembler->profile->uses_rela ? (int32_t)mem->disp : 0);
    AssemblerAddRelocation(base, reloc);
    return;
  }
  if (mem->sym != NULL) {
    if (!mem->sym_known) {
      AssemblerError(&enc->assembler->base,
                     "Undefined symbol in memory operand");
      return;
    }
    disp += mem->sym_value;
  }

  // rm == 100b (rsp, r12) always means "SIB byte follows", so any base whose
  // low three bits are 100 must be encoded with a SIB byte even when there is
  // no index register.
  bool need_sib = (base_reg >= 0 && (base_reg & 7) == (X86_REG_RSP & 7)) ||
                  (index >= 0);
  if (base_reg < 0) {
    base_reg = X86_REG_RBP;
    if (index < 0) {
      need_sib = true;
      index = 4;
    }
  }

  int mod = 0;
  int disp_size = 0;
  // rm == 101b (rbp, r13) with mod == 0 means RIP/disp32, not [reg], so those
  // bases always need an explicit (possibly zero) displacement byte.
  if (disp == 0 && (base_reg & 7) != (X86_REG_RBP & 7) && index < 0) {
    mod = 0;
  } else if (PickMemDispSize(disp) == 1) {
    mod = 1;
    disp_size = 1;
  } else {
    mod = 2;
    disp_size = 4;
  }

  if (need_sib) {
    EncodeModRM(enc, mod, reg_field, 4);
    EncodeSIB(enc, mem->scale, index < 0 ? 4 : index, base_reg);
  } else {
    EncodeModRM(enc, mod, reg_field, base_reg);
  }
  if (disp_size != 0) {
    EncodeDisp(enc, disp_size, (int32_t)disp);
  }
}

static void EncodeRegOperand(X86Encode* enc, int reg_field, const X86Reg* reg);
static void EmitMovqXmmParsed(X86Assembler* assembler, const X86Op* src,
                              const X86Op* dst);

static void EncodeRegOperand(X86Encode* enc, int reg_field, const X86Reg* reg) {
  if (reg->is_xmm) {
    AssemblerError(&enc->assembler->base, "Expected integer register");
    return;
  }
  if (reg->size == kX86Size64) {
    SetRexW(enc);
  } else if (reg->size == kX86Size8 && reg->num >= 4 && reg->num <= 7) {
    if (enc->rex < 0) {
      enc->rex = 0x40;
    }
  }
  EncodeModRM(enc, 3, reg_field, reg->num);
}

static void EncodeXmmRegOperand(X86Encode* enc, int reg_field,
                                const X86Reg* reg) {
  if (!reg->is_xmm) {
    AssemblerError(&enc->assembler->base, "Expected XMM register");
    return;
  }
  EncodeModRM(enc, 3, reg_field, reg->num);
}

static void EmitOpcodeBytes(X86Encode* enc, uint8_t prefix66, uint8_t prefix_f2,
                            uint8_t prefix_f3, uint8_t opcode,
                            bool two_byte) {
  // Mandatory prefixes are buffered separately (EncodeFinish emits them before
  // the REX prefix).  This is required because the REX bits for some operands
  // (e.g. an extended base register of a memory operand) are only determined
  // after this function runs, while encoding the operand.
  if (prefix66) {
    EncodeLegacyPrefix(enc, 0x66);
  }
  if (prefix_f2) {
    EncodeLegacyPrefix(enc, 0xf2);
  }
  if (prefix_f3) {
    EncodeLegacyPrefix(enc, 0xf3);
  }
  if (two_byte) {
    EncodeByte(enc, 0x0f);
  }
  EncodeByte(enc, opcode);
}

static void EmitALURegImm(X86Assembler* assembler, int op_ext, X86Size size,
                          const X86Reg* dst, int64_t imm) {
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  if (size == kX86Size64) {
    SetRexW(&enc);
  }
  if (size == kX86Size8 && imm >= -128 && imm <= 127) {
    EncodeByte(&enc, 0x83);
    EncodeRegOperand(&enc, op_ext, dst);
    EncodeImm(&enc, 1, imm);
  } else if ((size == kX86Size64 || size == kX86Size32) &&
             imm >= INT32_MIN && imm <= INT32_MAX) {
    EncodeByte(&enc, 0x81);
    EncodeRegOperand(&enc, op_ext, dst);
    EncodeImm(&enc, 4, imm);
  } else if (size == kX86Size16 && imm >= INT16_MIN && imm <= INT16_MAX) {
    EncodeByte(&enc, 0x81);
    EncodeRegOperand(&enc, op_ext, dst);
    EncodeImm(&enc, 2, imm);
  } else {
    AssemblerError(&ASM, "Immediate out of range for ALU instruction");
    return;
  }
  EncodeFinish(&enc);
}

// The code emitter always prints 64-bit register names regardless of the
// access width, so the operand size of a mov comes from the mnemonic
// (movb/movw/movl/movq), not from the register operand.  force_bits selects
// that size (0 means use the register's own size, i.e. 64-bit names -> 64).
static void EmitMovSized(X86Assembler* assembler, int force_bits) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }

  X86Size size = kX86Size64;
  switch (force_bits) {
    case 8: size = kX86Size8; break;
    case 16: size = kX86Size16; break;
    case 32: size = kX86Size32; break;
    case 64: size = kX86Size64; break;
    default: break;
  }
  // Override the (always-64-bit) register operand sizes so the encoder picks
  // the right REX.W / operand-size prefix.
  if (force_bits != 0) {
    if (src.kind == kX86OpReg && !src.reg.is_xmm) {
      src.reg.size = size;
    }
    if (dst.kind == kX86OpReg && !dst.reg.is_xmm) {
      dst.reg.size = size;
    }
  }
  bool byte = (size == kX86Size8);

  X86Encode enc;
  EncodeInit(&enc, assembler);

  if (dst.kind == kX86OpReg && src.kind == kX86OpImm) {
    X86Reg reg = dst.reg;
    if (reg.size == kX86Size64) {
      if (src.sym != NULL) {
        SetRexW(&enc);
        EncodeByte(&enc, (uint8_t)(0xb8 + (reg.num & 7)));
        SetRexB(&enc, reg.num);
        enc.imm_pos = enc.len;
        enc.imm_size = 8;
        EncodeImm(&enc, 8, 0);
        EncodeFinish(&enc);
        int offset = (int32_t)(AssemblerCurrentAddress(&ASM) - enc.imm_size);
        AssemblerRelocation* reloc = NewAssemblerRelocation(
            src.sym,
            src.reloc_type != 0 ? src.reloc_type
                                : X86DefaultAbsReloc(assembler->profile),
            ASMO.current_section, offset, 0);
        AssemblerAddRelocation(&ASM, reloc);
        return;
      }
      if (src.imm >= INT32_MIN && src.imm <= INT32_MAX) {
        SetRexW(&enc);
        EncodeByte(&enc, 0xc7);
        EncodeRegOperand(&enc, 0, &reg);
        EncodeImm(&enc, 4, src.imm);
      } else {
        SetRexW(&enc);
        EncodeByte(&enc, (uint8_t)(0xb8 + (reg.num & 7)));
        SetRexB(&enc, reg.num);
        EncodeImm(&enc, 8, src.imm);
      }
    } else if (reg.size == kX86Size32) {
      if (src.sym != NULL) {
        EncodeByte(&enc, 0xc7);
        EncodeRegOperand(&enc, 0, &reg);
        enc.imm_pos = enc.len;
        enc.imm_size = 4;
        EncodeImm(&enc, 4, 0);
        EncodeFinish(&enc);
        int offset = (int32_t)(AssemblerCurrentAddress(&ASM) - enc.imm_size);
        AssemblerRelocation* reloc = NewAssemblerRelocation(
            src.sym,
            src.reloc_type != 0 ? src.reloc_type
                                : X86DefaultAbsReloc(assembler->profile),
            ASMO.current_section, offset, 0);
        AssemblerAddRelocation(&ASM, reloc);
        return;
      }
      EncodeByte(&enc, 0xc7);
      EncodeRegOperand(&enc, 0, &reg);
      EncodeImm(&enc, 4, src.imm);
    } else if (reg.size == kX86Size16) {
      EncodeLegacyPrefix(&enc, 0x66);
      EncodeByte(&enc, 0xc7);
      EncodeRegOperand(&enc, 0, &reg);
      EncodeImm(&enc, 2, src.imm);
    } else {
      EncodeByte(&enc, 0xb0 + (reg.num & 7));
      SetRexB(&enc, reg.num);
      EncodeImm(&enc, 1, src.imm);
    }
    EncodeFinish(&enc);
    return;
  }

  if (dst.kind == kX86OpReg && src.kind == kX86OpReg) {
    if (dst.reg.is_xmm || src.reg.is_xmm) {
      EmitMovqXmmParsed(assembler, &src, &dst);
      return;
    }
    if (dst.reg.size != src.reg.size) {
      AssemblerError(&ASM, "mov operand size mismatch");
      return;
    }
    if (dst.reg.size == kX86Size64) {
      SetRexW(&enc);
    } else if (dst.reg.size == kX86Size16) {
      EncodeLegacyPrefix(&enc, 0x66);
    }
    EncodeByte(&enc, byte ? 0x88 : 0x89);
    EncodeRegOperand(&enc, src.reg.num, &dst.reg);
    EncodeFinish(&enc);
    return;
  }

  if (dst.kind == kX86OpReg && src.kind == kX86OpMem) {
    if (size == kX86Size64) {
      SetRexW(&enc);
    } else if (size == kX86Size16) {
      EncodeLegacyPrefix(&enc, 0x66);
    }
    EncodeByte(&enc, byte ? 0x8a : 0x8b);
    EncodeMemOperand(&enc, dst.reg.num, &src);
    EncodeFinish(&enc);
    return;
  }

  if (dst.kind == kX86OpMem && src.kind == kX86OpReg) {
    if (size == kX86Size64) {
      SetRexW(&enc);
    } else if (size == kX86Size16) {
      EncodeLegacyPrefix(&enc, 0x66);
    }
    EncodeByte(&enc, byte ? 0x88 : 0x89);
    EncodeMemOperand(&enc, src.reg.num, &dst);
    EncodeFinish(&enc);
    return;
  }

  if (dst.kind == kX86OpMem && src.kind == kX86OpImm && src.sym == NULL) {
    int imm_size = byte ? 1 : (size == kX86Size16 ? 2 : 4);
    if (size == kX86Size64) {
      SetRexW(&enc);
    } else if (size == kX86Size16) {
      EncodeLegacyPrefix(&enc, 0x66);
    }
    EncodeByte(&enc, byte ? 0xc6 : 0xc7);
    // The trailing immediate must be accounted for in any RIP-relative
    // displacement emitted while encoding the memory operand.
    enc.tail_bytes = imm_size;
    EncodeMemOperand(&enc, 0, &dst);
    enc.tail_bytes = 0;
    EncodeImm(&enc, imm_size, src.imm);
    EncodeFinish(&enc);
    return;
  }

  AssemblerError(&ASM, "Unsupported mov operand combination");
}

static COMPILER_UNUSED void EmitMov(X86Assembler* assembler) {
  EmitMovSized(assembler, 0);
}

// movzx / movsx: opcode is the second byte of a 0F-prefixed instruction.
//   0F B6 movzbl/q, 0F BE movsbl/q, 0F B7 movzwl/q, 0F BF movswl/q.
// The destination is always a (64-bit named) register; the source is a byte or
// word in memory or a register.
static void EmitMovExtend(X86Assembler* assembler, uint8_t opcode) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg || dst.reg.is_xmm) {
    AssemblerError(&ASM, "movzx/movsx requires a register destination");
    return;
  }

  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (dst.reg.size == kX86Size64) {
    SetRexW(&enc);
  }
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, opcode);
  if (src.kind == kX86OpReg) {
    EncodeModRM(&enc, 3, dst.reg.num, src.reg.num);
  } else if (src.kind == kX86OpMem) {
    EncodeMemOperand(&enc, dst.reg.num, &src);
  } else {
    AssemblerError(&ASM, "movzx/movsx invalid source operand");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitPushPop(X86Assembler* assembler, bool is_push) {
  X86Op op;
  if (!ParseOperand(assembler, &op) || op.kind != kX86OpReg) {
    AssemblerError(&ASM, "%s expects a register operand",
                   is_push ? "push" : "pop");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (op.reg.size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  if (op.reg.size == kX86Size64) {
    SetRexW(&enc);
    EncodeByte(&enc, (uint8_t)((is_push ? 0x50 : 0x58) + (op.reg.num & 7)));
    SetRexB(&enc, op.reg.num);
  } else if (op.reg.size == kX86Size32) {
    EncodeByte(&enc, (uint8_t)((is_push ? 0x50 : 0x58) + (op.reg.num & 7)));
    SetRexB(&enc, op.reg.num);
  } else {
    AssemblerError(&ASM, "push/pop unsupported operand size");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitALU(X86Assembler* assembler, int reg_opcode, int op_ext,
                    int force_bits) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (force_bits == 32) {
    if (src.kind == kX86OpReg && !src.reg.is_xmm) {
      src.reg.size = kX86Size32;
    }
    if (dst.kind == kX86OpReg && !dst.reg.is_xmm) {
      dst.reg.size = kX86Size32;
    }
  }

  if (dst.kind == kX86OpReg && src.kind == kX86OpImm) {
    if (src.sym != NULL) {
      if (src.reloc_type != R_386_GOTPC ||
          dst.reg.size != kX86Size32) {
        AssemblerError(&ASM, "Unsupported symbolic ALU immediate");
        return;
      }
      X86Encode enc;
      EncodeInit(&enc, assembler);
      EncodeByte(&enc, 0x81);
      EncodeRegOperand(&enc, op_ext, &dst.reg);
      EncodeImm(&enc, 4, src.imm);
      EncodeFinish(&enc);
      int32_t offset =
          (int32_t)(AssemblerCurrentAddress(&ASM) - sizeof(int32_t));
      AssemblerRelocation* reloc = NewAssemblerRelocation(
          src.sym, src.reloc_type, ASMO.current_section, offset, 0);
      AssemblerAddRelocation(&ASM, reloc);
      return;
    }
    EmitALURegImm(assembler, op_ext, dst.reg.size, &dst.reg, src.imm);
    return;
  }

  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (dst.kind == kX86OpReg && src.kind == kX86OpReg) {
    if (dst.reg.size == kX86Size64) {
      SetRexW(&enc);
    } else if (dst.reg.size == kX86Size16) {
      EncodeLegacyPrefix(&enc, 0x66);
    }
    EncodeByte(&enc, (uint8_t)reg_opcode);
    EncodeRegOperand(&enc, src.reg.num, &dst.reg);
    EncodeFinish(&enc);
    return;
  }

  if (dst.kind == kX86OpReg && src.kind == kX86OpMem) {
    if (dst.reg.size == kX86Size64) {
      SetRexW(&enc);
    }
    EncodeByte(&enc, (uint8_t)(reg_opcode + 2));
    EncodeMemOperand(&enc, dst.reg.num, &src);
    EncodeFinish(&enc);
    return;
  }

  if (dst.kind == kX86OpMem && src.kind == kX86OpReg) {
    if (src.reg.size == kX86Size64) {
      SetRexW(&enc);
    }
    EncodeByte(&enc, (uint8_t)reg_opcode);
    EncodeMemOperand(&enc, src.reg.num, &dst);
    EncodeFinish(&enc);
    return;
  }

  AssemblerError(&ASM, "Unsupported ALU operand combination");
}

static void EmitCmp(X86Assembler* assembler) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }

  X86Encode enc;
  EncodeInit(&enc, assembler);

  if (dst.kind == kX86OpReg && src.kind == kX86OpImm) {
    EmitALURegImm(assembler, 7, dst.reg.size, &dst.reg, src.imm);
    return;
  }

  if (dst.kind == kX86OpImm && src.kind == kX86OpReg) {
    EmitALURegImm(assembler, 7, src.reg.size, &src.reg, dst.imm);
    return;
  }

  if (dst.kind == kX86OpReg && src.kind == kX86OpReg) {
    if (dst.reg.size == kX86Size64) {
      SetRexW(&enc);
    }
    EncodeByte(&enc, 0x39);
    EncodeRegOperand(&enc, src.reg.num, &dst.reg);
    EncodeFinish(&enc);
    return;
  }

  if (dst.kind == kX86OpReg && src.kind == kX86OpMem) {
    if (dst.reg.size == kX86Size64) {
      SetRexW(&enc);
    }
    EncodeByte(&enc, 0x3b);
    EncodeMemOperand(&enc, dst.reg.num, &src);
    EncodeFinish(&enc);
    return;
  }

  AssemblerError(&ASM, "Unsupported cmp operand combination");
}

static void EmitLea(X86Assembler* assembler) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg || src.kind != kX86OpMem) {
    AssemblerError(&ASM, "lea expects mem, reg operands");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (dst.reg.size == kX86Size64) {
    SetRexW(&enc);
  }
  EncodeByte(&enc, 0x8d);
  EncodeMemOperand(&enc, dst.reg.num, &src);
  EncodeFinish(&enc);
}

static void EmitBranch(X86Assembler* assembler, int opcode, bool is_call) {
  int32_t start = (int32_t)AssemblerCurrentAddress(&ASM);
  bool known = false;
  int64_t target = 0;
  AssemblerSymbol* sym = NULL;

  if (LexLookingAt(&ASM.lex, TOK(identifier))) {
    sym = GetOrCreateSymbol(assembler, ASM.lex.spelling.value);
    LexNextToken(&ASM.lex);
    // Keep calls to weak definitions relocatable so the linker can select a
    // strong override. Strong definitions retain the toolchain's established
    // local-binding behavior.
    known = sym->defined && sym->section == ASMO.current_section &&
            sym->binding != SYM_BIND(weak);
    target = sym->value;
  } else {
    target = AssemblerEvaluateKnownExpression(&ASM, &known);
  }

  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (opcode < 0) {
    EncodeByte(&enc, (uint8_t)(is_call ? 0xe8 : 0xe9));
  } else {
    EncodeByte(&enc, 0x0f);
    EncodeByte(&enc, (uint8_t)opcode);
  }

  size_t total_len = (enc.rex >= 0 ? 1 : 0) + enc.len + 4;
  int32_t rel = known ? (int32_t)(target - (start + (int32_t)total_len))
                      : (assembler->profile->is_64bit ? 0 : -4);
  enc.disp_pos = enc.len;
  EncodeDisp(&enc, 4, rel);
  EncodeFinish(&enc);

  if (!known && sym != NULL) {
    int reloc_type = X86DefaultPcReloc(assembler->profile, is_call);
    if (!assembler->profile->is_64bit && is_call && ASMO.pic) {
      reloc_type = R_386_PLT32;
    }
    int32_t addend = assembler->profile->is_64bit ? -4 : 0;
    AssemblerRelocation* reloc = NewAssemblerRelocation(
        sym, reloc_type, ASMO.current_section,
        (int32_t)(start + (enc.rex >= 0 ? 1 : 0) + enc.disp_pos), addend);
    AssemblerAddRelocation(&ASM, reloc);
  }
}

static void EmitMovabs(X86Assembler* assembler) {
  X86Op imm, dst;
  if (!ParseOperand(assembler, &imm) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (imm.kind != kX86OpImm || dst.kind != kX86OpReg || dst.reg.is_xmm) {
    AssemblerError(&ASM, "movabs expects $imm, reg");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (imm.sym != NULL) {
    SetRexW(&enc);
    EncodeByte(&enc, (uint8_t)(0xb8 + (dst.reg.num & 7)));
    SetRexB(&enc, dst.reg.num);
    enc.imm_pos = enc.len;
    enc.imm_size = 8;
    EncodeImm(&enc, 8, 0);
    EncodeFinish(&enc);
    int offset = (int32_t)(AssemblerCurrentAddress(&ASM) - enc.imm_size);
    AssemblerRelocation* reloc = NewAssemblerRelocation(
        imm.sym,
        imm.reloc_type != 0 ? imm.reloc_type
                            : X86DefaultAbsReloc(assembler->profile),
        ASMO.current_section, offset, 0);
    AssemblerAddRelocation(&ASM, reloc);
    return;
  }
  SetRexW(&enc);
  EncodeByte(&enc, (uint8_t)(0xb8 + (dst.reg.num & 7)));
  SetRexB(&enc, dst.reg.num);
  EncodeImm(&enc, 8, imm.imm);
  EncodeFinish(&enc);
}

static void EmitShift(X86Assembler* assembler, int op_ext, X86Size size) {
  X86Op count, dst;
  if (!ParseOperand(assembler, &count) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size64) {
    SetRexW(&enc);
  } else if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  bool use_cl = false;
  if (count.kind == kX86OpReg && !count.reg.is_xmm &&
      count.reg.num == X86_REG_RCX && count.reg.size == kX86Size8) {
    use_cl = true;
  } else if (count.kind == kX86OpReg && !count.reg.is_xmm &&
             count.reg.num == X86_REG_RCX &&
             (count.reg.size == kX86Size64 || count.reg.size == kX86Size32)) {
    use_cl = true;
  }
  if (use_cl) {
    EncodeByte(&enc, 0xd3);
    if (dst.kind == kX86OpReg) {
      EncodeRegOperand(&enc, op_ext, &dst.reg);
    } else if (dst.kind == kX86OpMem) {
      EncodeMemOperand(&enc, op_ext, &dst);
    } else {
      AssemblerError(&ASM, "Invalid shift destination");
    }
  } else if (count.kind == kX86OpImm) {
    EncodeByte(&enc, 0xc1);
    if (dst.kind == kX86OpReg) {
      EncodeRegOperand(&enc, op_ext, &dst.reg);
    } else if (dst.kind == kX86OpMem) {
      EncodeMemOperand(&enc, op_ext, &dst);
    } else {
      AssemblerError(&ASM, "Invalid shift destination");
    }
    EncodeImm(&enc, 1, count.imm & 0xff);
  } else {
    AssemblerError(&ASM, "Shift count must be immediate or %%cl");
  }
  EncodeFinish(&enc);
}

static void EmitBitScan(X86Assembler* assembler, uint8_t opcode,
                        X86Size size) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg) {
    AssemblerError(&ASM, "Bit scan destination must be a register");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size64) {
    SetRexW(&enc);
  } else if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, opcode);
  if (src.kind == kX86OpReg) {
    EncodeRegOperand(&enc, dst.reg.num, &src.reg);
  } else if (src.kind == kX86OpMem) {
    EncodeMemOperand(&enc, dst.reg.num, &src);
  } else {
    AssemblerError(&ASM, "Invalid bit scan source");
  }
  EncodeFinish(&enc);
}

static void EmitUnary(X86Assembler* assembler, int op_ext, X86Size size) {
  X86Op dst;
  if (!ParseOperand(assembler, &dst)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size64) {
    SetRexW(&enc);
  } else if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  EncodeByte(&enc, 0xf7);
  if (dst.kind == kX86OpReg) {
    EncodeRegOperand(&enc, op_ext, &dst.reg);
  } else if (dst.kind == kX86OpMem) {
    EncodeMemOperand(&enc, op_ext, &dst);
  } else {
    AssemblerError(&ASM, "Unary instruction expects register or memory");
  }
  EncodeFinish(&enc);
}

static void EmitImul(X86Assembler* assembler, X86Size size) {
  X86Op op1, op2, op3;
  if (!ParseOperand(assembler, &op1)) {
    return;
  }
  if (!ExpectComma(assembler)) {
    return;
  }
  if (!ParseOperand(assembler, &op2)) {
    return;
  }
  bool three_operand = LexMatch(&ASM.lex, TOK(comma));
  if (three_operand && !ParseOperand(assembler, &op3)) {
    return;
  }

  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size64) {
    SetRexW(&enc);
  } else if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }

  if (three_operand) {
    if (op1.kind != kX86OpImm) {
      AssemblerError(&ASM, "Three-operand imul expects immediate first operand");
      return;
    }
    if (op3.kind != kX86OpReg || op2.kind != kX86OpReg) {
      AssemblerError(&ASM, "Three-operand imul expects reg, reg operands");
      return;
    }
    if (op1.imm >= -128 && op1.imm <= 127) {
      EncodeByte(&enc, 0x6b);
      EncodeRegOperand(&enc, op3.reg.num, &op2.reg);
      EncodeImm(&enc, 1, op1.imm);
    } else {
      EncodeByte(&enc, 0x69);
      EncodeRegOperand(&enc, op3.reg.num, &op2.reg);
      EncodeImm(&enc, 4, op1.imm);
    }
  } else {
    EncodeByte(&enc, 0x0f);
    EncodeByte(&enc, 0xaf);
    if (op2.kind == kX86OpReg && op1.kind == kX86OpReg) {
      EncodeRegOperand(&enc, op2.reg.num, &op1.reg);
    } else if (op1.kind == kX86OpReg && op2.kind == kX86OpMem) {
      EncodeMemOperand(&enc, op1.reg.num, &op2);
    } else if (op1.kind == kX86OpMem && op2.kind == kX86OpReg) {
      EncodeMemOperand(&enc, op2.reg.num, &op1);
    } else {
      AssemblerError(&ASM, "Unsupported imul operand combination");
    }
  }
  EncodeFinish(&enc);
}

static void EmitDivOp(X86Assembler* assembler, int op_ext, X86Size size) {
  X86Op divisor;
  if (!ParseOperand(assembler, &divisor)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size64) {
    SetRexW(&enc);
  } else if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  EncodeByte(&enc, 0xf7);
  if (divisor.kind == kX86OpReg) {
    EncodeRegOperand(&enc, op_ext, &divisor.reg);
  } else if (divisor.kind == kX86OpMem) {
    EncodeMemOperand(&enc, op_ext, &divisor);
  } else {
    AssemblerError(&ASM, "Divide expects register or memory divisor");
  }
  EncodeFinish(&enc);
}

static void EmitTest(X86Assembler* assembler, X86Size size) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size64) {
    SetRexW(&enc);
  }
  if (dst.kind == kX86OpReg && src.kind == kX86OpImm) {
    EncodeByte(&enc, 0xf7);
    EncodeRegOperand(&enc, 0, &dst.reg);
    EncodeImm(&enc, 4, src.imm);
  } else if (dst.kind == kX86OpReg && src.kind == kX86OpReg) {
    EncodeByte(&enc, 0x85);
    EncodeRegOperand(&enc, src.reg.num, &dst.reg);
  } else {
    AssemblerError(&ASM, "Unsupported test operand combination");
  }
  EncodeFinish(&enc);
}

static void EmitSetcc(X86Assembler* assembler, int opcode) {
  X86Op dst;
  if (!ParseOperand(assembler, &dst) || dst.kind != kX86OpReg ||
      dst.reg.is_xmm) {
    AssemblerError(&ASM, "setcc expects byte integer register destination");
    return;
  }
  X86Reg byte_reg = dst.reg;
  byte_reg.size = kX86Size8;
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, (uint8_t)opcode);
  EncodeRegOperand(&enc, 0, &byte_reg);
  EncodeFinish(&enc);
}

static void EmitNoOperands(X86Assembler* assembler, bool rex_w, uint8_t opcode) {
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (rex_w) {
    SetRexW(&enc);
  }
  EncodeByte(&enc, opcode);
  EncodeFinish(&enc);
}

static void EmitIndirectCall(X86Assembler* assembler) {
  (void)LexMatch(&ASM.lex, TOK(star));
  X86Op target;
  if (!ParseOperand(assembler, &target)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  SetRexW(&enc);
  EncodeByte(&enc, 0xff);
  if (target.kind == kX86OpReg) {
    EncodeRegOperand(&enc, 2, &target.reg);
  } else if (target.kind == kX86OpMem) {
    EncodeMemOperand(&enc, 2, &target);
  } else {
    AssemblerError(&ASM, "Indirect call expects register or memory target");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitIndirectJmp(X86Assembler* assembler) {
  (void)LexMatch(&ASM.lex, TOK(star));
  X86Op target;
  if (!ParseOperand(assembler, &target)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  SetRexW(&enc);
  EncodeByte(&enc, 0xff);
  if (target.kind == kX86OpReg) {
    EncodeRegOperand(&enc, 4, &target.reg);
  } else if (target.kind == kX86OpMem) {
    EncodeMemOperand(&enc, 4, &target);
  } else {
    AssemblerError(&ASM, "Indirect jump expects register or memory target");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitSSE(X86Assembler* assembler, uint8_t prefix66, uint8_t prefix_f2,
                    uint8_t prefix_f3, uint8_t opcode, bool int_dst) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (int_dst) {
    if (dst.kind != kX86OpReg || dst.reg.is_xmm || src.kind != kX86OpReg ||
        !src.reg.is_xmm) {
      AssemblerError(&ASM, "SSE convert expects xmm source, integer destination");
      return;
    }
    SetRexR(&enc, dst.reg.num);
    SetRexB(&enc, src.reg.num);
    EmitOpcodeBytes(&enc, prefix66, prefix_f2, prefix_f3, opcode, true);
    EncodeModRM(&enc, 3, dst.reg.num, src.reg.num);
  } else {
    if (dst.kind != kX86OpReg || !dst.reg.is_xmm) {
      AssemblerError(&ASM, "SSE instruction expects XMM destination");
      return;
    }
    EmitOpcodeBytes(&enc, prefix66, prefix_f2, prefix_f3, opcode, true);
    if (src.kind == kX86OpReg && src.reg.is_xmm) {
      EncodeXmmRegOperand(&enc, dst.reg.num, &src.reg);
    } else if (src.kind == kX86OpMem) {
      EncodeMemOperand(&enc, dst.reg.num, &src);
    } else if (src.kind == kX86OpReg && !src.reg.is_xmm) {
      AssemblerError(&ASM, "SSE instruction expects XMM or memory source");
      return;
    } else {
      AssemblerError(&ASM, "Unsupported SSE operand combination");
      return;
    }
  }
  EncodeFinish(&enc);
}

static void EmitSSEMove(X86Assembler* assembler, uint8_t prefix_f2,
                        uint8_t prefix_f3, bool store) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EmitOpcodeBytes(&enc, false, prefix_f2, prefix_f3, store ? 0x11 : 0x10, true);
  if (store) {
    if (src.kind != kX86OpReg || !src.reg.is_xmm) {
      AssemblerError(&ASM, "SSE store expects XMM source");
      return;
    }
    if (dst.kind == kX86OpReg && dst.reg.is_xmm) {
      EncodeXmmRegOperand(&enc, dst.reg.num, &src.reg);
    } else if (dst.kind == kX86OpMem) {
      EncodeMemOperand(&enc, src.reg.num, &dst);
    } else {
      AssemblerError(&ASM, "Unsupported SSE store operands");
    }
  } else {
    if (dst.kind != kX86OpReg || !dst.reg.is_xmm) {
      AssemblerError(&ASM, "SSE load expects XMM destination");
      return;
    }
    if (src.kind == kX86OpReg && src.reg.is_xmm) {
      EncodeXmmRegOperand(&enc, dst.reg.num, &src.reg);
    } else if (src.kind == kX86OpMem) {
      EncodeMemOperand(&enc, dst.reg.num, &src);
    } else {
      AssemblerError(&ASM, "Unsupported SSE load operands");
    }
  }
  EncodeFinish(&enc);
}

static void Assemble_movdqu(X86Assembler* assembler) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (dst.kind == kX86OpReg && dst.reg.is_xmm) {
    EmitOpcodeBytes(&enc, false, false, true, 0x6f, true);
    if (src.kind == kX86OpReg && src.reg.is_xmm) {
      EncodeXmmRegOperand(&enc, dst.reg.num, &src.reg);
    } else if (src.kind == kX86OpMem) {
      EncodeMemOperand(&enc, dst.reg.num, &src);
    } else {
      AssemblerError(&ASM, "movdqu load expects XMM or memory source");
      return;
    }
  } else if (src.kind == kX86OpReg && src.reg.is_xmm &&
             dst.kind == kX86OpMem) {
    EmitOpcodeBytes(&enc, false, false, true, 0x7f, true);
    EncodeMemOperand(&enc, src.reg.num, &dst);
  } else {
    AssemblerError(&ASM, "movdqu expects an XMM register and memory operand");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitMovdParsed(X86Assembler* assembler, const X86Op* src,
                           const X86Op* dst, bool gpr_to_xmm) {
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EmitOpcodeBytes(&enc, true, false, false, gpr_to_xmm ? 0x6e : 0x7e, true);
  if (gpr_to_xmm) {
    if (dst->kind != kX86OpReg || !dst->reg.is_xmm) {
      AssemblerError(&ASM, "movd to xmm expects XMM destination");
      return;
    }
    if (src->kind == kX86OpReg && !src->reg.is_xmm) {
      EncodeRegOperand(&enc, dst->reg.num, &src->reg);
    } else if (src->kind == kX86OpMem) {
      EncodeMemOperand(&enc, dst->reg.num, src);
    } else {
      AssemblerError(&ASM, "movd expects GPR or memory source");
    }
  } else {
    if (src->kind != kX86OpReg || !src->reg.is_xmm) {
      AssemblerError(&ASM, "movd from xmm expects XMM source");
      return;
    }
    if (dst->kind == kX86OpReg && !dst->reg.is_xmm) {
      EncodeRegOperand(&enc, src->reg.num, &dst->reg);
    } else if (dst->kind == kX86OpMem) {
      EncodeMemOperand(&enc, src->reg.num, dst);
    } else {
      AssemblerError(&ASM, "movd expects GPR or memory destination");
    }
  }
  EncodeFinish(&enc);
}

static void EmitMovqXmmParsed(X86Assembler* assembler, const X86Op* src,
                              const X86Op* dst) {
  if (src->kind == kX86OpImm && src->sym == NULL &&
      dst->kind == kX86OpReg && dst->reg.is_xmm) {
    // movq_xmm is a compiler pseudo-instruction. x86 has no immediate-to-XMM
    // encoding, so materialize the bit pattern in the reserved scratch GPR and
    // then perform the ordinary GPR-to-XMM move.
    X86Reg scratch = {10, kX86Size64, false};
    X86Encode immediate;
    EncodeInit(&immediate, assembler);
    if (src->imm >= INT32_MIN && src->imm <= INT32_MAX) {
      SetRexW(&immediate);
      EncodeByte(&immediate, 0xc7);
      EncodeRegOperand(&immediate, 0, &scratch);
      EncodeImm(&immediate, 4, src->imm);
    } else {
      SetRexW(&immediate);
      EncodeByte(&immediate, (uint8_t)(0xb8 + (scratch.num & 7)));
      SetRexB(&immediate, scratch.num);
      EncodeImm(&immediate, 8, src->imm);
    }
    EncodeFinish(&immediate);

    X86Op scratch_op = {0};
    scratch_op.kind = kX86OpReg;
    scratch_op.reg = scratch;
    EmitMovqXmmParsed(assembler, &scratch_op, dst);
    return;
  }

  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (src->kind == kX86OpReg && src->reg.is_xmm &&
      dst->kind == kX86OpReg && !dst->reg.is_xmm) {
    SetRexW(&enc);
    EmitOpcodeBytes(&enc, true, false, false, 0x7e, true);
    EncodeRegOperand(&enc, src->reg.num, &dst->reg);
  } else if (src->kind == kX86OpReg && !src->reg.is_xmm &&
             dst->kind == kX86OpReg && dst->reg.is_xmm) {
    SetRexW(&enc);
    EmitOpcodeBytes(&enc, true, false, false, 0x6e, true);
    EncodeRegOperand(&enc, dst->reg.num, &src->reg);
  } else if (src->kind == kX86OpMem && dst->kind == kX86OpReg &&
             dst->reg.is_xmm) {
    EmitOpcodeBytes(&enc, false, false, true, 0x7e, true);
    EncodeMemOperand(&enc, dst->reg.num, src);
  } else if (src->kind == kX86OpReg && src->reg.is_xmm &&
             dst->kind == kX86OpMem) {
    EmitOpcodeBytes(&enc, false, false, true, 0x7e, true);
    EncodeMemOperand(&enc, src->reg.num, dst);
  } else if (src->kind == kX86OpReg && src->reg.is_xmm &&
             dst->kind == kX86OpReg && dst->reg.is_xmm) {
    EmitOpcodeBytes(&enc, false, false, true, 0x7e, true);
    EncodeXmmRegOperand(&enc, dst->reg.num, &src->reg);
  } else {
    AssemblerError(&ASM, "Unsupported movq xmm/gpr operand combination");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitMovqXmm(X86Assembler* assembler) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  EmitMovqXmmParsed(assembler, &src, &dst);
}

static void EmitFnegSs(X86Assembler* assembler) {
  X86Op dst;
  if (!ParseOperand(assembler, &dst) || dst.kind != kX86OpReg || !dst.reg.is_xmm) {
    AssemblerError(&ASM, "fneg_ss expects XMM destination/source");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EmitOpcodeBytes(&enc, false, false, false, 0x57, true);
  EncodeXmmRegOperand(&enc, dst.reg.num, &dst.reg);
  EncodeFinish(&enc);
}

static void EmitFnegSd(X86Assembler* assembler) {
  X86Op dst;
  if (!ParseOperand(assembler, &dst) || dst.kind != kX86OpReg || !dst.reg.is_xmm) {
    AssemblerError(&ASM, "fneg_sd expects XMM destination/source");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EmitOpcodeBytes(&enc, false, true, false, 0x57, true);
  EncodeXmmRegOperand(&enc, dst.reg.num, &dst.reg);
  EncodeFinish(&enc);
}

static void Assemble_movw(X86Assembler* assembler) { EmitMovSized(assembler, 16); }
static void Assemble_movabs(X86Assembler* assembler) { EmitMovabs(assembler); }
static void Assemble_andq(X86Assembler* assembler) { EmitALU(assembler, 0x21, 4, 64); }
static void Assemble_andl(X86Assembler* assembler) { EmitALU(assembler, 0x21, 4, 32); }
static void Assemble_orq(X86Assembler* assembler) { EmitALU(assembler, 0x09, 1, 64); }
static void Assemble_orl(X86Assembler* assembler) { EmitALU(assembler, 0x09, 1, 32); }
static void Assemble_xorq(X86Assembler* assembler) { EmitALU(assembler, 0x31, 6, 64); }
static void Assemble_xorl(X86Assembler* assembler) { EmitALU(assembler, 0x31, 6, 32); }
static void Assemble_addl(X86Assembler* assembler) { EmitALU(assembler, 0x01, 0, 32); }
static void Assemble_adcl(X86Assembler* assembler) { EmitALU(assembler, 0x11, 2, 32); }
static void Assemble_subl(X86Assembler* assembler) { EmitALU(assembler, 0x29, 5, 32); }
static void Assemble_sbbl(X86Assembler* assembler) { EmitALU(assembler, 0x19, 3, 32); }
static void Assemble_cmpb(X86Assembler* assembler) { EmitCmp(assembler); }
static void Assemble_cmpl(X86Assembler* assembler) { EmitCmp(assembler); }
static void Assemble_cmpw(X86Assembler* assembler) { EmitCmp(assembler); }
static void Assemble_test(X86Assembler* assembler) { EmitTest(assembler, kX86Size64); }
static void Assemble_testq(X86Assembler* assembler) { EmitTest(assembler, kX86Size64); }
static void Assemble_testl(X86Assembler* assembler) { EmitTest(assembler, kX86Size32); }
static void Assemble_not(X86Assembler* assembler) { EmitUnary(assembler, 2, kX86Size64); }
static void Assemble_notq(X86Assembler* assembler) { EmitUnary(assembler, 2, kX86Size64); }
static void Assemble_notl(X86Assembler* assembler) { EmitUnary(assembler, 2, kX86Size32); }
static void Assemble_neg(X86Assembler* assembler) { EmitUnary(assembler, 3, kX86Size64); }
static void Assemble_negq(X86Assembler* assembler) { EmitUnary(assembler, 3, kX86Size64); }
static void Assemble_negl(X86Assembler* assembler) { EmitUnary(assembler, 3, kX86Size32); }
static void Assemble_imul(X86Assembler* assembler) { EmitImul(assembler, kX86Size64); }
static void Assemble_imulq(X86Assembler* assembler) { EmitImul(assembler, kX86Size64); }
static void Assemble_imull(X86Assembler* assembler) { EmitImul(assembler, kX86Size32); }
static void Assemble_idiv(X86Assembler* assembler) { EmitDivOp(assembler, 7, kX86Size64); }
static void Assemble_idivq(X86Assembler* assembler) { EmitDivOp(assembler, 7, kX86Size64); }
static void Assemble_idivl(X86Assembler* assembler) { EmitDivOp(assembler, 7, kX86Size32); }
static void Assemble_div(X86Assembler* assembler) { EmitDivOp(assembler, 6, kX86Size64); }
static void Assemble_divq(X86Assembler* assembler) { EmitDivOp(assembler, 6, kX86Size64); }
static void Assemble_divl(X86Assembler* assembler) { EmitDivOp(assembler, 6, kX86Size32); }
static void Assemble_shlq(X86Assembler* assembler) { EmitShift(assembler, 4, kX86Size64); }
static void Assemble_shrq(X86Assembler* assembler) { EmitShift(assembler, 5, kX86Size64); }
static void Assemble_sarq(X86Assembler* assembler) { EmitShift(assembler, 7, kX86Size64); }
static void Assemble_shll(X86Assembler* assembler) { EmitShift(assembler, 4, kX86Size32); }
static void Assemble_shrl(X86Assembler* assembler) { EmitShift(assembler, 5, kX86Size32); }
static void Assemble_sarl(X86Assembler* assembler) { EmitShift(assembler, 7, kX86Size32); }
static void Assemble_shl(X86Assembler* assembler) { EmitShift(assembler, 4, kX86Size64); }
static void Assemble_shr(X86Assembler* assembler) { EmitShift(assembler, 5, kX86Size64); }
static void Assemble_sar(X86Assembler* assembler) { EmitShift(assembler, 7, kX86Size64); }
static void Assemble_rolq(X86Assembler* assembler) { EmitShift(assembler, 0, kX86Size64); }
static void Assemble_rorq(X86Assembler* assembler) { EmitShift(assembler, 1, kX86Size64); }
static void Assemble_roll(X86Assembler* assembler) { EmitShift(assembler, 0, kX86Size32); }
static void Assemble_rorl(X86Assembler* assembler) { EmitShift(assembler, 1, kX86Size32); }
static void Assemble_bsfq(X86Assembler* assembler) { EmitBitScan(assembler, 0xbc, kX86Size64); }
static void Assemble_bsrq(X86Assembler* assembler) { EmitBitScan(assembler, 0xbd, kX86Size64); }
static void Assemble_bsfl(X86Assembler* assembler) { EmitBitScan(assembler, 0xbc, kX86Size32); }
static void Assemble_bsrl(X86Assembler* assembler) { EmitBitScan(assembler, 0xbd, kX86Size32); }
static void Assemble_sete(X86Assembler* assembler) { EmitSetcc(assembler, 0x94); }
static void Assemble_setne(X86Assembler* assembler) { EmitSetcc(assembler, 0x95); }
static void Assemble_setl(X86Assembler* assembler) { EmitSetcc(assembler, 0x9c); }
static void Assemble_setb(X86Assembler* assembler) { EmitSetcc(assembler, 0x92); }
static void Assemble_setg(X86Assembler* assembler) { EmitSetcc(assembler, 0x9f); }
static void Assemble_setge(X86Assembler* assembler) { EmitSetcc(assembler, 0x9d); }
static void Assemble_setae(X86Assembler* assembler) { EmitSetcc(assembler, 0x93); }

static void EmitAtomicCmpxchg(X86Assembler* assembler, X86Size size) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (src.kind != kX86OpReg || src.reg.is_xmm ||
      dst.kind != kX86OpMem) {
    AssemblerError(&ASM,
                   "atomic_cmpxchg expects integer register and memory operands");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeLegacyPrefix(&enc, 0xf0);
  if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  } else if (size == kX86Size64) {
    SetRexW(&enc);
  }
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, size == kX86Size8 ? 0xb0 : 0xb1);
  EncodeMemOperand(&enc, src.reg.num, &dst);
  EncodeFinish(&enc);
}

static void Assemble_atomic_cmpxchgb(X86Assembler* assembler) {
  EmitAtomicCmpxchg(assembler, kX86Size8);
}
static void Assemble_atomic_cmpxchgw(X86Assembler* assembler) {
  EmitAtomicCmpxchg(assembler, kX86Size16);
}
static void Assemble_atomic_cmpxchgl(X86Assembler* assembler) {
  EmitAtomicCmpxchg(assembler, kX86Size32);
}
static void Assemble_atomic_cmpxchgq(X86Assembler* assembler) {
  EmitAtomicCmpxchg(assembler, kX86Size64);
}

static void EmitAtomicXadd(X86Assembler* assembler, X86Size size) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (src.kind != kX86OpReg || src.reg.is_xmm ||
      dst.kind != kX86OpMem) {
    AssemblerError(&ASM,
                   "atomic_xadd expects integer register and memory operands");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeLegacyPrefix(&enc, 0xf0);
  if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  } else if (size == kX86Size64) {
    SetRexW(&enc);
  }
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, size == kX86Size8 ? 0xc0 : 0xc1);
  EncodeMemOperand(&enc, src.reg.num, &dst);
  EncodeFinish(&enc);
}

static void Assemble_atomic_xaddb(X86Assembler* assembler) {
  EmitAtomicXadd(assembler, kX86Size8);
}
static void Assemble_atomic_xaddw(X86Assembler* assembler) {
  EmitAtomicXadd(assembler, kX86Size16);
}
static void Assemble_atomic_xaddl(X86Assembler* assembler) {
  EmitAtomicXadd(assembler, kX86Size32);
}
static void Assemble_atomic_xaddq(X86Assembler* assembler) {
  EmitAtomicXadd(assembler, kX86Size64);
}

static void Assemble_cqo(X86Assembler* assembler) { EmitNoOperands(assembler, true, 0x99); }
static void Assemble_cdq(X86Assembler* assembler) { EmitNoOperands(assembler, false, 0x99); }
static void Assemble_cltq(X86Assembler* assembler) { EmitNoOperands(assembler, true, 0x98); }
// movslq (a.k.a. movsxd): sign-extend an r/m32 source into a 64-bit register.
// Encoded as REX.W + 0x63 /r.  (The no-operand cltq form is no longer emitted
// by the code generator; sign-extension of a value already in a register is
// lowered to a shift pair instead.)
static void EmitMovsxd(X86Assembler* assembler) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg || dst.reg.is_xmm) {
    AssemblerError(&ASM, "movslq requires a register destination");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  SetRexW(&enc);
  EncodeByte(&enc, 0x63);
  if (src.kind == kX86OpReg) {
    EncodeModRM(&enc, 3, dst.reg.num, src.reg.num);
  } else if (src.kind == kX86OpMem) {
    EncodeMemOperand(&enc, dst.reg.num, &src);
  } else {
    AssemblerError(&ASM, "movslq invalid source operand");
    return;
  }
  EncodeFinish(&enc);
}

static void Assemble_movslq(X86Assembler* assembler) { EmitMovsxd(assembler); }
static void Assemble_rcall(X86Assembler* assembler) { EmitIndirectCall(assembler); }
static void Assemble_movss(X86Assembler* assembler) { EmitSSEMove(assembler, false, true, false); }
static void Assemble_movsd(X86Assembler* assembler) { EmitSSEMove(assembler, true, false, false); }
static void Assemble_storesd(X86Assembler* assembler) { EmitSSEMove(assembler, true, false, true); }
static void Assemble_storess(X86Assembler* assembler) { EmitSSEMove(assembler, false, true, true); }
static void Assemble_addss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x58, false); }
static void Assemble_addsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x58, false); }
static void Assemble_subss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x5c, false); }
static void Assemble_subsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x5c, false); }
static void Assemble_mulss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x59, false); }
static void Assemble_mulsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x59, false); }
static void Assemble_divss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x5e, false); }
static void Assemble_divsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x5e, false); }
static void Assemble_sqrtss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x51, false); }
static void Assemble_sqrtsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x51, false); }
static void Assemble_ucomiss(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x2e, false); }
static void Assemble_ucomisd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x2e, false); }
static void Assemble_paddb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfc, false); }
static void Assemble_paddw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfd, false); }
static void Assemble_paddd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfe, false); }
static void Assemble_paddq(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xd4, false); }
static void Assemble_psubb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xf8, false); }
static void Assemble_psubw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xf9, false); }
static void Assemble_psubd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfa, false); }
static void Assemble_psubq(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfb, false); }
static void Assemble_pand(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xdb, false); }
static void Assemble_por(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xeb, false); }
static void Assemble_pxor(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xef, false); }
static void Assemble_pcmpeqb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x74, false); }
static void Assemble_pcmpeqw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x75, false); }
static void Assemble_pcmpeqd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x76, false); }
static void Assemble_pcmpgtb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x64, false); }
static void Assemble_pcmpgtw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x65, false); }
static void Assemble_pcmpgtd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x66, false); }
static void Assemble_addps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x58, false); }
static void Assemble_addpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x58, false); }
static void Assemble_subps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x5c, false); }
static void Assemble_subpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x5c, false); }
static void Assemble_mulps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x59, false); }
static void Assemble_mulpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x59, false); }
static void Assemble_divps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x5e, false); }
static void Assemble_divpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x5e, false); }

static void EmitSSEConvertFromInt(X86Assembler* assembler, uint8_t prefix_f2,
                                  uint8_t prefix_f3, uint8_t opcode) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg || !dst.reg.is_xmm ||
      (src.kind == kX86OpReg && src.reg.is_xmm)) {
    AssemblerError(&ASM, "Integer-to-SSE convert expects GPR/memory source, XMM dest");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (src.kind == kX86OpReg && src.reg.size == kX86Size64) {
    SetRexW(&enc);
  }
  EmitOpcodeBytes(&enc, false, prefix_f2, prefix_f3, opcode, true);
  if (src.kind == kX86OpReg) {
    EncodeRegOperand(&enc, dst.reg.num, &src.reg);
  } else if (src.kind == kX86OpMem) {
    EncodeMemOperand(&enc, dst.reg.num, &src);
  } else {
    AssemblerError(&ASM, "Integer-to-SSE convert expects GPR or memory source");
  }
  EncodeFinish(&enc);
}

static void Assemble_cvtsi2ss(X86Assembler* assembler) {
  EmitSSEConvertFromInt(assembler, false, true, 0x2a);
}
static void Assemble_cvtsi2sd(X86Assembler* assembler) {
  EmitSSEConvertFromInt(assembler, true, false, 0x2a);
}
static void Assemble_cvttss2si(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x2c, true); }
static void Assemble_cvttsd2si(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x2c, true); }
static void Assemble_cvtss2sd(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x5a, false); }
static void Assemble_cvtsd2ss(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x5a, false); }
static void Assemble_movd(X86Assembler* assembler) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  EmitMovdParsed(assembler, &src, &dst,
                 dst.kind == kX86OpReg && dst.reg.is_xmm);
}
static void Assemble_movq_xmm(X86Assembler* assembler) { EmitMovqXmm(assembler); }
static void Assemble_fneg_ss(X86Assembler* assembler) { EmitFnegSs(assembler); }
static void Assemble_fneg_sd(X86Assembler* assembler) { EmitFnegSd(assembler); }

static void Assemble_mov(X86Assembler* assembler) { EmitMovSized(assembler, 64); }
static void Assemble_movq(X86Assembler* assembler) { EmitMovSized(assembler, 64); }
static void Assemble_movl(X86Assembler* assembler) { EmitMovSized(assembler, 32); }
static void Assemble_movb(X86Assembler* assembler) { EmitMovSized(assembler, 8); }
static void Assemble_movsbq(X86Assembler* assembler) { EmitMovExtend(assembler, 0xbe); }
static void Assemble_movzbq(X86Assembler* assembler) { EmitMovExtend(assembler, 0xb6); }
static void Assemble_movswq(X86Assembler* assembler) { EmitMovExtend(assembler, 0xbf); }
static void Assemble_movzwq(X86Assembler* assembler) { EmitMovExtend(assembler, 0xb7); }
static void Assemble_push(X86Assembler* assembler) {
  EmitPushPop(assembler, true);
}
static void Assemble_pushq(X86Assembler* assembler) {
  EmitPushPop(assembler, true);
}
static void Assemble_pushl(X86Assembler* assembler) {
  EmitPushPop(assembler, true);
}
static void Assemble_pop(X86Assembler* assembler) {
  EmitPushPop(assembler, false);
}
static void Assemble_popq(X86Assembler* assembler) {
  EmitPushPop(assembler, false);
}
static void Assemble_popl(X86Assembler* assembler) {
  EmitPushPop(assembler, false);
}
static void Assemble_add(X86Assembler* assembler) { EmitALU(assembler, 0x01, 0, 64); }
static void Assemble_addq(X86Assembler* assembler) { EmitALU(assembler, 0x01, 0, 64); }
static void Assemble_sub(X86Assembler* assembler) { EmitALU(assembler, 0x29, 5, 64); }
static void Assemble_subq(X86Assembler* assembler) { EmitALU(assembler, 0x29, 5, 64); }
static void Assemble_and(X86Assembler* assembler) { EmitALU(assembler, 0x21, 4, 64); }
static void Assemble_or(X86Assembler* assembler) { EmitALU(assembler, 0x09, 1, 64); }
static void Assemble_xor(X86Assembler* assembler) { EmitALU(assembler, 0x31, 6, 64); }
static void Assemble_cmp(X86Assembler* assembler) { EmitCmp(assembler); }
static void Assemble_cmpq(X86Assembler* assembler) { EmitCmp(assembler); }
static void Assemble_lea(X86Assembler* assembler) { EmitLea(assembler); }
static void Assemble_leaq(X86Assembler* assembler) { EmitLea(assembler); }
static void Assemble_call(X86Assembler* assembler) {
  if (LexLookingAt(&ASM.lex, TOK(star))) {
    EmitIndirectCall(assembler);
  } else {
    EmitBranch(assembler, -1, true);
  }
}
static void Assemble_jmp(X86Assembler* assembler) {
  if (LexLookingAt(&ASM.lex, TOK(star))) {
    EmitIndirectJmp(assembler);
    return;
  }
  EmitBranch(assembler, -1, false);
}
static void Assemble_ret(X86Assembler* assembler) {
  if (LexLookingAt(&ASM.lex, TOK(number))) {
    bool known = false;
    int64_t imm = AssemblerEvaluateKnownExpression(&ASM, &known);
    if (!known || imm < 0 || imm > 0xffff) {
      AssemblerError(&ASM, "ret immediate must be a 16-bit constant");
      return;
    }
    AssemblerEmitByte(&ASM, ASMO.current_section, 0xc2);
    AssemblerEmitWord(&ASM, ASMO.current_section, (int32_t)imm);
    return;
  }
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xc3);
}
static void Assemble_leave(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xc9);
}
static void Assemble_nop(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x90);
}

static void Assemble_syscall(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x0f);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x05);
}

static void Assemble_mfence(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x0f);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xae);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xf0);
}

#define JCC(name, opcode)                                                    \
  static void Assemble_##name(X86Assembler* assembler) {                  \
    EmitBranch(assembler, opcode, false);                                    \
  }

JCC(je, 0x84);
JCC(jz, 0x84);
JCC(jne, 0x85);
JCC(jnz, 0x85);
JCC(jl, 0x8c);
JCC(jg, 0x8f);
JCC(jle, 0x8e);
JCC(jge, 0x8d);
JCC(jb, 0x82);
JCC(jae, 0x83);
JCC(ja, 0x87);
JCC(jbe, 0x86);

#undef JCC

#define INST(mnemonic)                                                       \
  do {                                                                       \
    MapKeyValue kv = {0};                                                    \
    kv.key.p = #mnemonic;                                                    \
    kv.value.p = Assemble_##mnemonic;                                          \
    MapInsert(instructions, kv);                                             \
  } while (0)

#define INST2(mnemonic, alias)                                               \
  do {                                                                       \
    MapKeyValue kv = {0};                                                    \
    kv.key.p = #alias;                                                       \
    kv.value.p = Assemble_##mnemonic;                                          \
    MapInsert(instructions, kv);                                             \
  } while (0)

static void InitializeInstructions(Map* instructions) {
  INST(mov);
  INST(movq);
  INST(movl);
  INST(movb);
  INST(movw);
  INST(movsbq);
  INST(movzbq);
  INST(movswq);
  INST(movzwq);
  INST2(movzbq, movzbl);
  INST2(movsbq, movsbl);
  INST2(movswq, movswl);
  INST2(movzwq, movzwl);
  INST(movabs);
  INST(movss);
  INST(movsd);
  INST(storess);
  INST(storesd);
  INST(movd);
  INST(movq_xmm);
  INST(movdqu);
  INST(push);
  INST(pushq);
  INST(pushl);
  INST(pop);
  INST(popq);
  INST(popl);
  INST(add);
  INST(addq);
  INST(addl);
  INST(adcl);
  INST(sub);
  INST(subq);
  INST(subl);
  INST(sbbl);
  INST(and);
  INST(andl);
  INST(andq);
  INST(or);
  INST(orl);
  INST(orq);
  INST(xor);
  INST(xorq);
  INST(xorl);
  INST(imul);
  INST(imulq);
  INST(imull);
  INST(idiv);
  INST(idivq);
  INST(idivl);
  INST(div);
  INST(divq);
  INST(divl);
  INST(not);
  INST(notq);
  INST(notl);
  INST(neg);
  INST(negq);
  INST(negl);
  INST(shl);
  INST(shlq);
  INST(shr);
  INST(shrq);
  INST(sar);
  INST(sarq);
  INST(shll);
  INST(shrl);
  INST(sarl);
  INST(rolq);
  INST(rorq);
  INST(roll);
  INST(rorl);
  INST(bsfq);
  INST(bsrq);
  INST(bsfl);
  INST(bsrl);
  INST(cmp);
  INST(cmpq);
  INST(cmpb);
  INST(cmpl);
  INST(cmpw);
  INST(test);
  INST(testq);
  INST(testl);
  INST(lea);
  INST(leaq);
  INST(call);
  INST(rcall);
  INST(jmp);
  INST(ret);
  INST(nop);
  INST(syscall);
  INST(mfence);
  INST(je);
  INST(jz);
  INST(jne);
  INST(jnz);
  INST(jl);
  INST(jg);
  INST(jle);
  INST(jge);
  INST(jb);
  INST(jae);
  INST(ja);
  INST(jbe);
  INST(sete);
  INST(setne);
  INST(setl);
  INST(setb);
  INST(setg);
  INST(setge);
  INST(setae);
  INST(atomic_cmpxchgb);
  INST(atomic_cmpxchgw);
  INST(atomic_cmpxchgl);
  INST(atomic_cmpxchgq);
  INST(atomic_xaddb);
  INST(atomic_xaddw);
  INST(atomic_xaddl);
  INST(atomic_xaddq);
  INST(cqo);
  INST(cdq);
  INST(cltq);
  INST(movslq);
  INST(addss);
  INST(addsd);
  INST(subss);
  INST(subsd);
  INST(mulss);
  INST(mulsd);
  INST(divss);
  INST(divsd);
  INST(sqrtss);
  INST(sqrtsd);
  INST(ucomiss);
  INST(ucomisd);
  INST(paddb);
  INST(paddw);
  INST(paddd);
  INST(paddq);
  INST(psubb);
  INST(psubw);
  INST(psubd);
  INST(psubq);
  INST(pand);
  INST(por);
  INST(pxor);
  INST(pcmpeqb);
  INST(pcmpeqw);
  INST(pcmpeqd);
  INST(pcmpgtb);
  INST(pcmpgtw);
  INST(pcmpgtd);
  INST(addps);
  INST(addpd);
  INST(subps);
  INST(subpd);
  INST(mulps);
  INST(mulpd);
  INST(divps);
  INST(divpd);
  INST(cvtsi2ss);
  INST(cvtsi2sd);
  INST(cvttss2si);
  INST(cvttsd2si);
  INST(cvtss2sd);
  INST(cvtsd2ss);
  INST(fneg_ss);
  INST(fneg_sd);
  INST(leave);
}

#undef INST
#undef INST2

bool X86AssemblerInitWithProfile(X86Assembler* assembler, String* infile,
                                 String* outfile, const X86Profile* profile) {
  assembler->profile = profile != NULL ? profile : &kX86ProfileAMD64;

  static int amd64_reloc_types[] = {
      R_X86_64_16,    R_X86_64_32,    R_X86_64_64,    R_X86_64_16,
      R_X86_64_32,    R_X86_64_64,    R_X86_64_16,    R_X86_64_32,
      R_X86_64_64,    R_X86_64_PLT32, R_X86_64_GOTPCREL, R_X86_64_TPOFF64,
  };
  static int i386_reloc_types[] = {
      R_386_32, R_386_32, R_386_32, R_386_32, R_386_32, R_386_32,
      R_386_32, R_386_32, R_386_32, R_386_PC32, R_386_32, R_386_32,
  };
  int* reloc_types = assembler->profile->is_64bit ? amd64_reloc_types
                                                  : i386_reloc_types;

  if (!AssemblerInit(&assembler->base, assembler->profile->elf_machine, 0, reloc_types,
                     infile, outfile)) {
    return false;
  }

  MapInit(&assembler->instructions, CompareString);
  InitializeInstructions(&assembler->instructions);
  AssemblerAddSection(&assembler->base, NULL, SHT(null), 0, 0);
  assembler->bss =
      AssemblerAddSection(&assembler->base, NewString(".bss"),
                          SHT(nobits), SHF(alloc) | SHF(write), 16);
  return true;
}

X86Assembler* NewX86Assembler(String* infile, String* outfile) {
  X86Assembler* assembler = malloc(sizeof(X86Assembler));
  X86AssemblerInit(assembler, infile, outfile);
  return assembler;
}

void X86AssemblerDestruct(X86Assembler* assembler) {
  AssemblerDestruct(&assembler->base);
  MapDestruct(&assembler->instructions);
}

void X86AssemblerDelete(X86Assembler* assembler) {
  X86AssemblerDestruct(assembler);
  free(assembler);
}

void AssembleX86Instruction(Assembler* base, String* word) {
  X86Assembler* assembler = (X86Assembler*)base;
  void* asm_func = MapFindPointerKey(&assembler->instructions, word->value);
  if (asm_func != NULL) {
    void (*func)(X86Assembler*) = asm_func;
    func(assembler);
  } else {
    AssemblerError(&assembler->base, "Syntax error; unknown instruction: %s",
                   word->value);
  }
}


bool X86AssemblerInit(X86Assembler* assembler, String* infile,
                           String* outfile) {
  return X86AssemblerInitWithProfile(assembler, infile, outfile, &kX86ProfileAMD64);
}
