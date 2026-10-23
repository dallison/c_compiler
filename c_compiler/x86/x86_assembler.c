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
  kX86SizeNone = 0,
  kX86Size8 = 1,
  kX86Size16 = 2,
  kX86Size32 = 4,
  kX86Size64 = 8,
} X86Size;

typedef struct {
  int num;
  X86Size size;
  bool is_xmm;
  // %ah/%ch/%dh/%bh: byte registers 4-7 that only exist without a REX prefix.
  bool high8;
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
  // A %ah-style register is encoded, so the instruction must not have REX.
  bool no_rex;
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
  enc->no_rex = false;
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
    AssemblerError(&enc->assembler->base,
                   "64-bit operand size requires x86-64");
    return;
  }
  if (enc->rex < 0) {
    enc->rex = 0x40;
  }
  enc->rex |= 0x08;
}

// Sets REX bit `bit` when register number `num` is r8-r15.  REX is only
// emitted when some bit (or a byte register, see EncodeByteRegRex) needs it.
static void SetRexBit(X86Encode* enc, int num, int bit) {
  if ((num & 8) == 0) {
    return;
  }
  if (!enc->assembler->profile->supports_rex) {
    AssemblerError(&enc->assembler->base,
                   "Register r8-r15 not available in i386 mode");
    return;
  }
  if (enc->rex < 0) {
    enc->rex = 0x40;
  }
  enc->rex |= bit;
}

static void SetRexR(X86Encode* enc, int reg) { SetRexBit(enc, reg, 0x04); }
static void SetRexX(X86Encode* enc, int index) { SetRexBit(enc, index, 0x02); }
static void SetRexB(X86Encode* enc, int rm) { SetRexBit(enc, rm, 0x01); }

// Byte registers 4-7 are %spl/%bpl/%sil/%dil with a REX prefix and
// %ah/%ch/%dh/%bh without one.
static void EncodeByteRegRex(X86Encode* enc, const X86Reg* reg) {
  if (reg->is_xmm || reg->size != kX86Size8 || reg->num < 4 || reg->num > 7) {
    return;
  }
  if (reg->high8) {
    enc->no_rex = true;
    return;
  }
  if (!enc->assembler->profile->supports_rex) {
    AssemblerError(&enc->assembler->base,
                   "%%spl/%%bpl/%%sil/%%dil are not available in i386 mode");
    return;
  }
  if (enc->rex < 0) {
    enc->rex = 0x40;
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
  if (enc->no_rex && enc->rex >= 0) {
    AssemblerError(base, "Can't encode %%ah/%%bh/%%ch/%%dh in an instruction "
                         "requiring a REX prefix");
  }
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
  reg->high8 = false;
  if (ParseXmmSuffix(name, len, reg)) {
    return true;
  }
  static const char* const kHigh8[] = {"ah", "ch", "dh", "bh"};
  for (int i = 0; i < 4; i++) {
    if (len == 2 && strncmp(name, kHigh8[i], 2) == 0) {
      reg->num = 4 + i;
      reg->size = kX86Size8;
      reg->high8 = true;
      return true;
    }
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
      {"r10w", X86_REG_R10, kX86Size16},{"r11w", X86_REG_R11, kX86Size16},
      {"r12w", X86_REG_R12, kX86Size16},{"r13w", X86_REG_R13, kX86Size16},
      {"r14w", X86_REG_R14, kX86Size16},{"r15w", X86_REG_R15, kX86Size16},
      {"al", X86_REG_RAX, kX86Size8},   {"bl", X86_REG_RBX, kX86Size8},
      {"cl", X86_REG_RCX, kX86Size8},   {"dl", X86_REG_RDX, kX86Size8},
      {"sil", X86_REG_RSI, kX86Size8},  {"dil", X86_REG_RDI, kX86Size8},
      {"bpl", X86_REG_RBP, kX86Size8},  {"spl", X86_REG_RSP, kX86Size8},
      {"r8b", X86_REG_R8, kX86Size8},   {"r9b", X86_REG_R9, kX86Size8},
      {"r10b", X86_REG_R10, kX86Size8}, {"r11b", X86_REG_R11, kX86Size8},
      {"r12b", X86_REG_R12, kX86Size8}, {"r13b", X86_REG_R13, kX86Size8},
      {"r14b", X86_REG_R14, kX86Size8}, {"r15b", X86_REG_R15, kX86Size8},
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
    reg->high8 = false;
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

  if (LexLookingAt(&ASM.lex, TOK(percent))) {
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
    if (!LexLookingAt(&ASM.lex, TOK(percent))) {
      AssemblerError(&ASM, "Index register expected");
      return false;
    }
    if (!ParseRegister(assembler, &op->index, /*allow_rip=*/false, NULL)) {
      return false;
    }
    // SIB index 100b without REX.X means "no index", so %rsp can't be one.
    if (op->index.is_xmm || op->index.num == X86_REG_RSP) {
      AssemblerError(&ASM, "Invalid index register");
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

// The current number token starts `Nb(` or `Nf(`: a GNU local label used as
// a memory displacement (`lea 1b(%rip), %rax`).
static bool AsmLookingAtLocalLabelMemory(Assembler* assembler) {
  const String* line = &assembler->lex.line;
  size_t pos = assembler->lex.pos;
  if (pos + 1 >= line->length) {
    return false;
  }
  char direction = line->value[pos];
  char after = line->value[pos + 1];
  if (direction != 'b' && direction != 'B' && direction != 'f' &&
      direction != 'F') {
    return false;
  }
  if (isalnum((unsigned char)after) || after == '_' || after == '$' ||
      after == '.') {
    return false;
  }
  pos++;
  while (pos < line->length && isspace((unsigned char)line->value[pos])) {
    pos++;
  }
  return pos < line->length && line->value[pos] == '(';
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

// Parses `sym`, `sym@SUFFIX` and an optional `+ expr` / `- expr` addend.
// As in GNU as, suffixes are case-insensitive (`@PLT`, `@tpoff`).
// Parses `sym@suffix [+- addend]`, or, without a suffix, any expression that
// reduces to symbol + constant (`sym + 8`, `sym - base + 4`).  *sym is NULL
// when the expression is absolute, such as a difference of two labels.
static bool ParseSymbolAndAddend(X86Assembler* assembler, AssemblerSymbol** sym,
                                 String* suffix, int64_t* addend) {
  String name;
  StringInit(&name, NULL);
  StringInit(suffix, NULL);
  AssemblerExtractSymbolSuffix(&ASM.lex.spelling, &name, suffix);
  *addend = 0;
  if (suffix->length > 0) {
    *sym = GetOrCreateSymbol(assembler, name.value);
    StringDestruct(&name);
    LexNextToken(&ASM.lex);
    if (LexLookingAt(&ASM.lex, TOK(plus)) ||
        LexLookingAt(&ASM.lex, TOK(minus))) {
      *addend = AssemblerEvaluateExpression(&ASM);
    }
    return true;
  }
  // Whether the value is absolute can change once later labels are defined,
  // and with it the encoding, so a symbol reference requests a layout pass.
  ASM.parsing_layout_expression = true;
  bool ok = AssemblerEvaluateRelocatableExpression(&ASM, sym, addend);
  ASM.parsing_layout_expression = false;
  if (!ok) {
    if (ASMO.pass == ASM_OBJECT_FINAL_PASS) {
      AssemblerError(&ASM, "Expression must be a constant or symbol + constant");
      StringDestruct(&name);
      return false;
    }
    *sym = GetOrCreateSymbol(assembler, name.value);
    *addend = 0;
  }
  StringDestruct(&name);
  return true;
}

static bool ParseSymbolImmediate(X86Assembler* assembler, X86Op* op) {
  AssemblerSymbol* sym;
  String suffix;
  int64_t addend;
  if (!ParseSymbolAndAddend(assembler, &sym, &suffix, &addend)) {
    StringDestruct(&suffix);
    return false;
  }
  op->kind = kX86OpImm;
  op->imm = addend;
  if (sym == NULL) {
    StringDestruct(&suffix);
    return true;
  }
  op->sym = sym;
  op->sym_known = false;
  op->reloc_type = X86DefaultAbsReloc(assembler->profile);
  bool ok = true;
  if (!assembler->profile->is_64bit &&
      strcmp(sym->name.value, "_GLOBAL_OFFSET_TABLE_") == 0) {
    // The PIC sequence emitted by the i386 backend obtains the address of
    // its local pop instruction.  The relocation field is three bytes later.
    op->reloc_type = R_386_GOTPC;
    op->imm += 3;
  } else if (StringEqualCaseBlind(&suffix, "TPOFF")) {
    op->reloc_type =
        assembler->profile->is_64bit ? R_X86_64_TPOFF64 : R_386_TLS_LE;
  } else if (!assembler->profile->is_64bit &&
             StringEqualCaseBlind(&suffix, "GOTPC")) {
    op->reloc_type = R_386_GOTPC;
    op->imm += 3;
  } else if (suffix.length > 0) {
    AssemblerError(&ASM, "Unsupported symbol suffix '@%s'", suffix.value);
    ok = false;
  }
  StringDestruct(&suffix);
  return ok;
}

static bool ParseSegmentOverrideFromCurrent(X86Assembler* assembler,
                                            int* segment_prefix) {
  if (!LexLookingAt(&ASM.lex, TOK(identifier))) {
    return false;
  }
  static const struct {
    const char* name;
    int prefix;
  } kSegments[] = {{"es", 0x26}, {"cs", 0x2e}, {"ss", 0x36},
                   {"ds", 0x3e}, {"fs", 0x64}, {"gs", 0x65}};
  const char* name = ASM.lex.spelling.value;
  if (ASM.lex.spelling.length != 2) {
    return false;
  }
  for (size_t i = 0; i < sizeof(kSegments) / sizeof(kSegments[0]); i++) {
    if (strncmp(name, kSegments[i].name, 2) == 0) {
      *segment_prefix = kSegments[i].prefix;
      LexNextToken(&ASM.lex);
      if (!LexMatch(&ASM.lex, TOK(colon))) {
        AssemblerError(&ASM, "Expected ':' after segment register");
        return false;
      }
      return true;
    }
  }
  return false;
}

// `sym`, `sym+8`, `sym@GOTPCREL`, each optionally followed by `(base, index,
// scale)`.  Without a base the operand addresses the symbol directly.
static bool ParseSymbolMemory(X86Assembler* assembler, X86Op* op) {
  AssemblerSymbol* sym;
  String suffix;
  int64_t addend;
  if (!ParseSymbolAndAddend(assembler, &sym, &suffix, &addend)) {
    StringDestruct(&suffix);
    return false;
  }
  op->disp += addend;
  if (sym != NULL) {
    op->sym = sym;
    op->sym_known = sym->defined && sym->section == ASMO.current_section;
    op->sym_value = sym->value;
  }
  bool ok = true;
  if (StringEqualCaseBlind(&suffix, "TPOFF")) {
    op->reloc_type =
        assembler->profile->is_64bit ? R_X86_64_TPOFF32 : R_386_TLS_LE;
  } else if (!assembler->profile->is_64bit && StringEqualCaseBlind(&suffix, "NTPOFF")) {
    op->reloc_type = R_386_TLS_LE;
  } else if (!assembler->profile->is_64bit && StringEqualCaseBlind(&suffix, "GOT")) {
    op->reloc_type = R_386_GOT32;
  } else if (!assembler->profile->is_64bit && StringEqualCaseBlind(&suffix, "GOTOFF")) {
    op->reloc_type = R_386_GOTOFF;
  } else if (StringEqualCaseBlind(&suffix, "GOTPCREL")) {
    op->reloc_type = R_X86_64_GOTPCREL;
  } else if (StringEqualCaseBlind(&suffix, "TLSGD")) {
    op->reloc_type = R_X86_64_TLSGD;
  } else if (StringEqualCaseBlind(&suffix, "GOTTPOFF")) {
    op->reloc_type = R_X86_64_GOTTPOFF;
  } else if (suffix.length > 0) {
    AssemblerError(&ASM, "Unsupported symbol suffix '@%s'", suffix.value);
    ok = false;
  }
  StringDestruct(&suffix);
  if (!ok) {
    return false;
  }
  if (LexLookingAt(&ASM.lex, TOK(lparen))) {
    return ParseMemoryTail(assembler, op);
  }
  return true;
}

// The current `(` opens a base/index list rather than a parenthesized
// displacement expression.
static bool AsmLParenStartsAddress(Assembler* assembler) {
  const String* line = &assembler->lex.line;
  size_t pos = assembler->lex.pos;
  while (pos < line->length && isspace((unsigned char)line->value[pos])) {
    pos++;
  }
  return pos < line->length &&
         (line->value[pos] == '%' || line->value[pos] == ',');
}

// Any memory operand: `(%rax)`, `-8(%rbp)`, `104 + 32(%rdi)`, `1b(%rip)`,
// `sym+4(%rip)`, `table(,%rax,8)`, or an absolute address (`0x10`, `sym`).
static bool ParseMemoryOperand(X86Assembler* assembler, X86Op* op) {
  InitMemOp(op);
  if (LexLookingAt(&ASM.lex, TOK(lparen)) && AsmLParenStartsAddress(&ASM)) {
    return ParseMemoryTail(assembler, op);
  }
  if (LexLookingAt(&ASM.lex, TOK(number)) && AsmLookingAtLocalLabelMemory(&ASM)) {
    int number = (int)ASM.lex.number;
    LexNextToken(&ASM.lex);
    bool backward = ASM.lex.spelling.value[0] == 'b' ||
                    ASM.lex.spelling.value[0] == 'B';
    LexNextToken(&ASM.lex);
    op->sym = AssemblerLookupNumericLocalLabel(&ASM, number, backward);
    if (op->sym == NULL) {
      return false;
    }
    op->sym_known =
        op->sym->defined && op->sym->section == ASMO.current_section;
    op->sym_value = op->sym->value;
    return ParseMemoryTail(assembler, op);
  }
  if (AssemblerLookingAtSymbol(&ASM)) {
    return ParseSymbolMemory(assembler, op);
  }
  // An absolute displacement's size follows its value, which may depend on
  // labels defined later, so referencing a symbol requests a layout pass.
  AssemblerSymbol* sym = NULL;
  ASM.parsing_layout_expression = true;
  bool ok = AssemblerEvaluateRelocatableExpression(&ASM, &sym, &op->disp);
  ASM.parsing_layout_expression = false;
  if (!ok) {
    if (ASMO.pass == ASM_OBJECT_FINAL_PASS) {
      AssemblerError(&ASM, "Displacement must be a constant or symbol + constant");
      return false;
    }
    op->disp = 0;
  } else if (sym != NULL) {
    op->sym = sym;
    op->sym_known = sym->defined && sym->section == ASMO.current_section;
    op->sym_value = sym->value;
  }
  if (LexLookingAt(&ASM.lex, TOK(lparen))) {
    return ParseMemoryTail(assembler, op);
  }
  return true;
}

// AT&T operands: `$` marks an immediate, `%` a register or segment override,
// and anything else is memory.
static bool ParseOperand(X86Assembler* assembler, X86Op* op) {
  memset(op, 0, sizeof(*op));

  if (ASM.lex.dollar_prefixed) {
    if (AssemblerLookingAtSymbol(&ASM)) {
      return ParseSymbolImmediate(assembler, op);
    }
    op->kind = kX86OpImm;
    op->imm = AssemblerEvaluateExpression(&ASM);
    return true;
  }
  if (LexMatch(&ASM.lex, TOK(percent))) {
    int segment_prefix = 0;
    if (ParseSegmentOverrideFromCurrent(assembler, &segment_prefix)) {
      if (!ParseMemoryOperand(assembler, op)) {
        return false;
      }
      op->segment_prefix = segment_prefix;
      return true;
    }
    op->kind = kX86OpReg;
    return ParseRegister(assembler, &op->reg, /*allow_rip=*/false, NULL);
  }
  return ParseMemoryOperand(assembler, op);
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

// Offset in the section of the byte that will follow everything encoded so
// far (prefixes and REX are emitted ahead of enc->bytes).
static int64_t EncodeEndAddress(const X86Encode* enc) {
  return AssemblerCurrentAddress(&enc->assembler->base) +
         (int64_t)enc->num_prefixes + (enc->rex >= 0 ? 1 : 0) +
         (int64_t)enc->len;
}

static void EncodeMemOperand(X86Encode* enc, int reg_field, const X86Op* mem) {
  Assembler* base = &enc->assembler->base;
  const X86Profile* profile = enc->assembler->profile;
  if (mem->segment_prefix != 0) {
    EncodeLegacyPrefix(enc, (uint8_t)mem->segment_prefix);
  }

  // A bare symbol is addressed RIP-relative on x86-64: the same location as
  // GNU's absolute form, without needing a 32-bit address.  TLS offsets are
  // not addresses and stay absolute.
  bool rip = mem->rip_relative;
  if (!rip && profile->is_64bit && mem->sym != NULL && mem->base.num < 0 &&
      mem->index.num < 0 && mem->segment_prefix == 0 &&
      mem->reloc_type != R_X86_64_TPOFF32 &&
      mem->reloc_type != R_X86_64_TPOFF64) {
    rip = true;
  }
  if (rip) {
    if (!profile->is_64bit) {
      AssemblerError(base, "%%rip addressing requires x86-64");
      return;
    }
    EncodeModRM(enc, 0, reg_field, 5);
    int64_t next_ip = EncodeEndAddress(enc) + 4 + enc->tail_bytes;
    if (mem->sym != NULL && (!mem->sym_known || mem->reloc_type != 0)) {
      int64_t field = EncodeEndAddress(enc);
      EncodeDisp(enc, 4, 0);
      int reloc_type = mem->reloc_type != 0 ? mem->reloc_type : R_X86_64_PC32;
      if (mem->reloc_type == 0 && base->object.pic &&
          (mem->sym->binding == SYM_BIND(global) ||
           mem->sym->binding == SYM_BIND(weak))) {
        reloc_type = R_X86_64_GOTPCREL;
      }
      // The CPU adds the displacement to the address of the next
      // instruction, which is past the field and any trailing immediate.
      AssemblerRelocation* reloc = NewAssemblerRelocation(
          mem->sym, reloc_type, base->object.current_section, (int32_t)field,
          (int32_t)(mem->disp - (next_ip - field)));
      AssemblerAddRelocation(base, reloc);
    } else if (mem->sym != NULL) {
      EncodeDisp(enc, 4, (int32_t)(mem->sym_value + mem->disp - next_ip));
    } else {
      EncodeDisp(enc, 4, (int32_t)mem->disp);
    }
    return;
  }

  int base_reg = mem->base.num;
  int index = mem->index.num;
  int64_t disp = mem->disp;
  int mod = 0;
  int disp_size = 0;
  if (base_reg < 0 || mem->sym != NULL) {
    // No base means a 32-bit absolute (possibly indexed) displacement; a
    // symbol always needs the full field for its relocation.
    mod = base_reg < 0 ? 0 : 2;
    disp_size = 4;
  } else if (disp == 0 && (base_reg & 7) != (X86_REG_RBP & 7)) {
    // rm == 101b (rbp, r13) with mod == 0 means disp32, not [reg], so those
    // bases always need an explicit (possibly zero) displacement.
    mod = 0;
  } else if (PickMemDispSize(disp) == 1) {
    mod = 1;
    disp_size = 1;
  } else {
    mod = 2;
    disp_size = 4;
  }

  // rm == 100b (rsp, r12) always means "SIB byte follows".  Without a base,
  // x86-64 also needs a SIB byte, because mod 0 rm 101b is RIP-relative there.
  bool need_sib = index >= 0 ||
                  (base_reg >= 0 && (base_reg & 7) == (X86_REG_RSP & 7)) ||
                  (base_reg < 0 && profile->is_64bit);
  if (need_sib) {
    EncodeModRM(enc, mod, reg_field, 4);
    EncodeSIB(enc, mem->scale, index < 0 ? 4 : index,
              base_reg < 0 ? X86_REG_RBP : base_reg);
  } else {
    EncodeModRM(enc, mod, reg_field, base_reg < 0 ? X86_REG_RBP : base_reg);
  }
  if (disp_size == 0) {
    return;
  }
  if (mem->sym == NULL) {
    EncodeDisp(enc, disp_size, (int32_t)disp);
    return;
  }
  // ELF32 uses REL, so the addend belongs in the relocated field.
  bool rela = profile->uses_rela;
  int64_t field = EncodeEndAddress(enc);
  EncodeDisp(enc, 4, rela ? 0 : (int32_t)disp);
  int reloc_type = mem->reloc_type;
  if (reloc_type == 0) {
    reloc_type = profile->is_64bit ? R_X86_64_32S : R_386_32;
  }
  AssemblerRelocation* reloc = NewAssemblerRelocation(
      mem->sym, reloc_type, base->object.current_section, (int32_t)field,
      rela ? (int32_t)disp : 0);
  AssemblerAddRelocation(base, reloc);
}

static void EncodeRegOperand(X86Encode* enc, int reg_field, const X86Reg* reg);
static void EmitMovqXmmParsed(X86Assembler* assembler, const X86Op* src,
                              const X86Op* dst);
static void EmitMovdParsed(X86Assembler* assembler, const X86Op* src,
                           const X86Op* dst, bool gpr_to_xmm);

// The register r/m operand of indirect branches, movd/movq and int-to-SSE
// conversions, which only exist in 32- and 64-bit forms.
static void EncodeRegOperand(X86Encode* enc, int reg_field, const X86Reg* reg) {
  if (reg->is_xmm || reg->size == kX86Size8 || reg->size == kX86Size16) {
    AssemblerError(&enc->assembler->base,
                   "Expected a 32- or 64-bit integer register");
    return;
  }
  if (reg->size == kX86Size64) {
    SetRexW(enc);
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

static X86Size NativeIntSize(const X86Assembler* assembler) {
  return assembler->profile->is_64bit ? kX86Size64 : kX86Size32;
}

static X86Size SuffixSize(char suffix) {
  switch (suffix) {
    case 'b':
      return kX86Size8;
    case 'w':
      return kX86Size16;
    case 'l':
      return kX86Size32;
    case 'q':
      return kX86Size64;
    default:
      return kX86SizeNone;
  }
}

static bool AtStatementEnd(X86Assembler* assembler) {
  return ASM.lex.token_start >= ASM.lex.line.length ||
         LexLookingAt(&ASM.lex, TOK(eof)) ||
         LexLookingAt(&ASM.lex, TOK(semicolon)) ||
         LexLookingAt(&ASM.lex, TOK(hash));
}

// Parses a comma-separated operand list.  Returns the operand count, or -1
// after reporting an error.
static int ParseOperandList(X86Assembler* assembler, X86Op* ops, int max) {
  if (AtStatementEnd(assembler)) {
    return 0;
  }
  int count = 0;
  for (;;) {
    if (count == max) {
      AssemblerError(&ASM, "Too many operands");
      return -1;
    }
    if (!ParseOperand(assembler, &ops[count])) {
      return -1;
    }
    count++;
    if (!LexMatch(&ASM.lex, TOK(comma))) {
      return count;
    }
  }
}

static bool ParseExactOperands(X86Assembler* assembler, X86Op* ops, int count,
                               const char* mnemonic) {
  int parsed = ParseOperandList(assembler, ops, count);
  if (parsed < 0) {
    return false;
  }
  if (parsed != count) {
    AssemblerError(&ASM, "%s expects %d operand%s", mnemonic, count,
                   count == 1 ? "" : "s");
    return false;
  }
  for (int i = 0; i < count; i++) {
    if (ops[i].kind == kX86OpReg && ops[i].reg.is_xmm) {
      AssemblerError(&ASM, "%s expects integer operands", mnemonic);
      return false;
    }
  }
  return true;
}

// The operand size of an integer instruction: its b/w/l/q suffix, else the
// size of its register operands, else the native word.  The compiler's
// emitter prints full-width register names and relies on the suffix, so a
// suffix resizes register operands instead of conflicting with them.
static bool ResolveIntSize(X86Assembler* assembler, X86Size suffix,
                           X86Op** ops, int count, X86Size* size) {
  X86Size reg_size = kX86SizeNone;
  for (int i = 0; i < count; i++) {
    if (ops[i]->kind != kX86OpReg || ops[i]->reg.is_xmm) {
      continue;
    }
    if (reg_size == kX86SizeNone) {
      reg_size = ops[i]->reg.size;
    } else if (ops[i]->reg.size != reg_size && suffix == kX86SizeNone) {
      AssemblerError(&ASM, "Operand size mismatch");
      return false;
    }
  }
  *size = suffix != kX86SizeNone     ? suffix
          : reg_size != kX86SizeNone ? reg_size
                                     : NativeIntSize(assembler);
  for (int i = 0; i < count; i++) {
    X86Reg* reg = &ops[i]->reg;
    if (ops[i]->kind != kX86OpReg || reg->is_xmm || reg->size == *size) {
      continue;
    }
    if (reg->high8) {
      AssemblerError(&ASM, "%%ah/%%bh/%%ch/%%dh are byte registers");
      return false;
    }
    reg->size = *size;
  }
  return true;
}

static bool EncodeOperandSize(X86Encode* enc, X86Size size) {
  if (size == kX86Size16) {
    EncodeLegacyPrefix(enc, 0x66);
  } else if (size == kX86Size64) {
    if (!enc->assembler->profile->supports_rex) {
      AssemblerError(&enc->assembler->base,
                     "64-bit operand size requires x86-64");
      return false;
    }
    SetRexW(enc);
  }
  return true;
}

// ModRM (and SIB/displacement) for a register-or-memory operand.
static void EncodeRM(X86Encode* enc, int reg_field, const X86Op* rm) {
  if (rm->kind == kX86OpReg) {
    EncodeByteRegRex(enc, &rm->reg);
    EncodeModRM(enc, 3, reg_field, rm->reg.num);
  } else {
    EncodeMemOperand(enc, reg_field, rm);
  }
}

// EncodeRM with a register in the ModRM reg field.
static void EncodeRegRM(X86Encode* enc, const X86Reg* reg, const X86Op* rm) {
  EncodeByteRegRex(enc, reg);
  EncodeRM(enc, reg->num, rm);
}

// Encodes a register in the low bits of the opcode byte (push, bswap, ...).
static void EncodeOpcodePlusReg(X86Encode* enc, uint8_t opcode,
                                const X86Reg* reg) {
  EncodeByteRegRex(enc, reg);
  SetRexB(enc, reg->num);
  EncodeByte(enc, (uint8_t)(opcode + (reg->num & 7)));
}

static int ImmFieldSize(X86Size size) {
  return size == kX86Size8 ? 1 : size == kX86Size16 ? 2 : 4;
}

static bool CheckImmRange(X86Assembler* assembler, const X86Op* imm, int size) {
  if (imm->sym != NULL) {
    return true;
  }
  int64_t value = imm->imm;
  bool ok = true;
  if (size == 1) {
    ok = value >= -128 && value <= 255;
  } else if (size == 2) {
    ok = value >= -32768 && value <= 65535;
  } else if (size == 4) {
    ok = value >= INT32_MIN && value <= (int64_t)UINT32_MAX;
  }
  if (!ok) {
    AssemblerError(&ASM, "Immediate %" PRId64 " out of range for a %d-byte field",
                   value, size);
  }
  return ok;
}

// The relocation for a symbolic immediate in a `size`-byte field.  A 4-byte
// field the CPU sign-extends to 64 bits needs R_X86_64_32S.
static int ImmRelocType(const X86Assembler* assembler, int reloc_type,
                        int size, bool sign_extended) {
  const X86Profile* profile = assembler->profile;
  if (reloc_type == X86DefaultAbsReloc(profile)) {
    if (!profile->is_64bit) {
      return size == 4 ? R_386_32 : 0;
    }
    switch (size) {
      case 8:
        return R_X86_64_64;
      case 4:
        return sign_extended ? R_X86_64_32S : R_X86_64_32;
      case 2:
        return R_X86_64_16;
      default:
        return 0;
    }
  }
  if (profile->is_64bit && reloc_type == R_X86_64_TPOFF64) {
    return size == 8 ? R_X86_64_TPOFF64 : size == 4 ? R_X86_64_TPOFF32 : 0;
  }
  return size == 4 ? reloc_type : 0;
}

// Appends an immediate field, emits the instruction, and relocates a
// symbolic immediate.
static void FinishWithImm(X86Assembler* assembler, X86Encode* enc,
                          const X86Op* imm, int size, bool sign_extended) {
  bool rela = assembler->profile->uses_rela;
  int reloc_type = 0;
  if (imm->sym != NULL) {
    reloc_type =
        ImmRelocType(assembler, imm->reloc_type, size, sign_extended);
    if (reloc_type == 0) {
      AssemblerError(&ASM, "Unsupported relocation for a %d-byte immediate",
                     size);
      return;
    }
  }
  // ELF32 uses REL, so the addend belongs in the relocated field.
  EncodeImm(enc, size, imm->sym != NULL && rela ? 0 : imm->imm);
  EncodeFinish(enc);
  if (imm->sym != NULL) {
    int32_t offset = (int32_t)(AssemblerCurrentAddress(&ASM) - size);
    AssemblerRelocation* reloc =
        NewAssemblerRelocation(imm->sym, reloc_type, ASMO.current_section,
                               offset, rela ? (int32_t)imm->imm : 0);
    AssemblerAddRelocation(&ASM, reloc);
  }
}

static void EmitIntALU(X86Assembler* assembler, int ext, X86Size suffix) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "ALU instruction")) {
    return;
  }
  X86Op* src = &ops[0];
  X86Op* dst = &ops[1];
  X86Op* sized[] = {src, dst};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  bool byte = size == kX86Size8;
  if (src->kind == kX86OpImm && dst->kind != kX86OpImm) {
    int imm_size = ImmFieldSize(size);
    if (!CheckImmRange(assembler, src, imm_size)) {
      return;
    }
    // Always the full-width immediate form: an expression's value may only
    // be known in the final pass, and the size must not change between
    // passes.
    EncodeByte(&enc, byte ? 0x80 : 0x81);
    enc.tail_bytes = imm_size;
    EncodeRM(&enc, ext, dst);
    FinishWithImm(assembler, &enc, src, imm_size, true);
    return;
  }
  if (src->kind == kX86OpReg && dst->kind != kX86OpImm) {
    EncodeByte(&enc, (uint8_t)(ext * 8 + (byte ? 0 : 1)));
    EncodeRegRM(&enc, &src->reg, dst);
    EncodeFinish(&enc);
    return;
  }
  if (src->kind == kX86OpMem && dst->kind == kX86OpReg) {
    EncodeByte(&enc, (uint8_t)(ext * 8 + (byte ? 2 : 3)));
    EncodeRegRM(&enc, &dst->reg, src);
    EncodeFinish(&enc);
    return;
  }
  AssemblerError(&ASM, "Unsupported ALU operand combination");
}

// not/neg/mul/div/idiv (F6/F7 /ext) and inc/dec (FE/FF /ext).
static void EmitIntUnary(X86Assembler* assembler, uint8_t opcode, int ext,
                         X86Size suffix) {
  X86Op op;
  if (!ParseExactOperands(assembler, &op, 1, "Unary instruction")) {
    return;
  }
  if (op.kind == kX86OpImm) {
    AssemblerError(&ASM, "Unary instruction expects register or memory");
    return;
  }
  X86Op* sized[] = {&op};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 1, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  EncodeByte(&enc, (uint8_t)(size == kX86Size8 ? opcode : opcode + 1));
  EncodeRM(&enc, ext, &op);
  EncodeFinish(&enc);
}

// The shift count operand is `$imm` or %cl.  The compiler's emitter names
// the count register at full width (`%rcx`).
static bool IsShiftCountRegister(const X86Op* op) {
  return op->kind == kX86OpReg && !op->reg.is_xmm && !op->reg.high8 &&
         op->reg.num == X86_REG_RCX;
}

static void EmitIntShift(X86Assembler* assembler, int ext, X86Size suffix) {
  X86Op ops[2];
  int count = ParseOperandList(assembler, ops, 2);
  if (count < 0) {
    return;
  }
  if (count == 0) {
    AssemblerError(&ASM, "Shift expects an operand");
    return;
  }
  X86Op* amount = count == 2 ? &ops[0] : NULL;
  X86Op* dst = &ops[count - 1];
  if (dst->kind == kX86OpImm || (dst->kind == kX86OpReg && dst->reg.is_xmm)) {
    AssemblerError(&ASM, "Invalid shift destination");
    return;
  }
  X86Op* sized[] = {dst};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 1, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  bool byte = size == kX86Size8;
  if (amount == NULL) {
    EncodeByte(&enc, byte ? 0xd0 : 0xd1);
    EncodeRM(&enc, ext, dst);
    EncodeFinish(&enc);
  } else if (amount->kind == kX86OpImm && amount->sym == NULL) {
    EncodeByte(&enc, byte ? 0xc0 : 0xc1);
    enc.tail_bytes = 1;
    EncodeRM(&enc, ext, dst);
    EncodeImm(&enc, 1, amount->imm & 0xff);
    EncodeFinish(&enc);
  } else if (IsShiftCountRegister(amount)) {
    EncodeByte(&enc, byte ? 0xd2 : 0xd3);
    EncodeRM(&enc, ext, dst);
    EncodeFinish(&enc);
  } else {
    AssemblerError(&ASM, "Shift count must be immediate or %%cl");
  }
}

// shld/shrd: `$imm|%cl, reg, r/m`.
static void EmitShiftDouble(X86Assembler* assembler, uint8_t opcode,
                            X86Size suffix) {
  X86Op ops[3];
  if (!ParseExactOperands(assembler, ops, 3, "Double shift")) {
    return;
  }
  if (ops[1].kind != kX86OpReg || ops[2].kind == kX86OpImm) {
    AssemblerError(&ASM, "Double shift expects count, register, r/m");
    return;
  }
  X86Op* sized[] = {&ops[1], &ops[2]};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  if (size == kX86Size8) {
    AssemblerError(&ASM, "Double shift has no byte form");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  EncodeByte(&enc, 0x0f);
  if (ops[0].kind == kX86OpImm && ops[0].sym == NULL) {
    EncodeByte(&enc, opcode);
    enc.tail_bytes = 1;
    EncodeRegRM(&enc, &ops[1].reg, &ops[2]);
    EncodeImm(&enc, 1, ops[0].imm & 0xff);
  } else if (IsShiftCountRegister(&ops[0])) {
    EncodeByte(&enc, (uint8_t)(opcode + 1));
    EncodeRegRM(&enc, &ops[1].reg, &ops[2]);
  } else {
    AssemblerError(&ASM, "Shift count must be immediate or %%cl");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitIntTest(X86Assembler* assembler, X86Size suffix) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "test")) {
    return;
  }
  X86Op* sized[] = {&ops[0], &ops[1]};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  bool byte = size == kX86Size8;
  if (ops[0].kind == kX86OpImm && ops[1].kind != kX86OpImm) {
    int imm_size = ImmFieldSize(size);
    if (!CheckImmRange(assembler, &ops[0], imm_size)) {
      return;
    }
    EncodeByte(&enc, byte ? 0xf6 : 0xf7);
    enc.tail_bytes = imm_size;
    EncodeRM(&enc, 0, &ops[1]);
    FinishWithImm(assembler, &enc, &ops[0], imm_size, true);
    return;
  }
  // test is symmetric; either operand may be the memory one.
  const X86Op* reg = ops[0].kind == kX86OpReg ? &ops[0] : &ops[1];
  const X86Op* rm = reg == &ops[0] ? &ops[1] : &ops[0];
  if (reg->kind != kX86OpReg || rm->kind == kX86OpImm) {
    AssemblerError(&ASM, "Unsupported test operand combination");
    return;
  }
  EncodeByte(&enc, byte ? 0x84 : 0x85);
  EncodeRegRM(&enc, &reg->reg, rm);
  EncodeFinish(&enc);
}

static void EmitIntMov(X86Assembler* assembler, X86Size suffix) {
  X86Op ops[2];
  int count = ParseOperandList(assembler, ops, 2);
  if (count < 0) {
    return;
  }
  if (count != 2) {
    AssemblerError(&ASM, "mov expects 2 operands");
    return;
  }
  X86Op* src = &ops[0];
  X86Op* dst = &ops[1];
  if ((src->kind == kX86OpReg && src->reg.is_xmm) ||
      (dst->kind == kX86OpReg && dst->reg.is_xmm)) {
    if (suffix == kX86Size32 && assembler->profile->is_64bit) {
      EmitMovdParsed(assembler, src, dst,
                     dst->kind == kX86OpReg && dst->reg.is_xmm);
    } else if (suffix == kX86SizeNone || suffix == kX86Size64 ||
               suffix == kX86Size32) {
      EmitMovqXmmParsed(assembler, src, dst);
    } else {
      AssemblerError(&ASM, "Invalid XMM move size");
    }
    return;
  }
  if (src->kind == kX86OpImm && dst->kind == kX86OpImm) {
    AssemblerError(&ASM, "mov destination cannot be an immediate");
    return;
  }
  X86Op* sized[] = {src, dst};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  bool byte = size == kX86Size8;

  if (src->kind == kX86OpImm && dst->kind == kX86OpReg) {
    const X86Reg* reg = &dst->reg;
    if (size == kX86Size64) {
      // A symbol's address may not fit a sign-extended 32-bit field, so it
      // always gets the 64-bit form.
      if (src->sym == NULL && src->imm >= INT32_MIN && src->imm <= INT32_MAX) {
        EncodeByte(&enc, 0xc7);
        EncodeRM(&enc, 0, dst);
        FinishWithImm(assembler, &enc, src, 4, true);
      } else {
        EncodeOpcodePlusReg(&enc, 0xb8, reg);
        FinishWithImm(assembler, &enc, src, 8, false);
      }
      return;
    }
    int imm_size = ImmFieldSize(size);
    if (!CheckImmRange(assembler, src, imm_size)) {
      return;
    }
    EncodeOpcodePlusReg(&enc, byte ? 0xb0 : 0xb8, reg);
    FinishWithImm(assembler, &enc, src, imm_size, false);
    return;
  }
  if (src->kind == kX86OpImm && dst->kind == kX86OpMem) {
    int imm_size = ImmFieldSize(size);
    if (!CheckImmRange(assembler, src, imm_size)) {
      return;
    }
    EncodeByte(&enc, byte ? 0xc6 : 0xc7);
    // The trailing immediate must be accounted for in any RIP-relative
    // displacement emitted while encoding the memory operand.
    enc.tail_bytes = imm_size;
    EncodeRM(&enc, 0, dst);
    FinishWithImm(assembler, &enc, src, imm_size, true);
    return;
  }
  if (src->kind == kX86OpReg) {
    EncodeByte(&enc, byte ? 0x88 : 0x89);
    EncodeRegRM(&enc, &src->reg, dst);
    EncodeFinish(&enc);
    return;
  }
  if (src->kind == kX86OpMem && dst->kind == kX86OpReg) {
    EncodeByte(&enc, byte ? 0x8a : 0x8b);
    EncodeRegRM(&enc, &dst->reg, src);
    EncodeFinish(&enc);
    return;
  }
  AssemblerError(&ASM, "Unsupported mov operand combination");
}

static void EmitIntLea(X86Assembler* assembler, X86Size suffix) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "lea")) {
    return;
  }
  if (ops[0].kind != kX86OpMem || ops[1].kind != kX86OpReg) {
    AssemblerError(&ASM, "lea expects mem, reg operands");
    return;
  }
  X86Op* sized[] = {&ops[1]};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 1, &size)) {
    return;
  }
  if (size == kX86Size8) {
    AssemblerError(&ASM, "lea has no byte form");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  EncodeByte(&enc, 0x8d);
  EncodeRegRM(&enc, &ops[1].reg, &ops[0]);
  EncodeFinish(&enc);
}

// push/pop operate on the native word or, with a `w` suffix or a 16-bit
// register, on 16 bits.
static bool ResolveStackSize(X86Assembler* assembler, X86Size suffix,
                             X86Op* op, X86Size* size) {
  X86Op* sized[] = {op};
  if (!ResolveIntSize(assembler, suffix, sized, 1, size)) {
    return false;
  }
  if (*size != kX86Size16 && *size != NativeIntSize(assembler)) {
    AssemblerError(&ASM, "Invalid push/pop operand size in %s mode",
                   assembler->profile->is_64bit ? "64-bit" : "32-bit");
    return false;
  }
  return true;
}

static void EmitIntPush(X86Assembler* assembler, X86Size suffix) {
  X86Op op;
  if (!ParseExactOperands(assembler, &op, 1, "push")) {
    return;
  }
  X86Size size;
  if (!ResolveStackSize(assembler, suffix, &op, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  if (op.kind == kX86OpReg) {
    EncodeOpcodePlusReg(&enc, 0x50, &op.reg);
    EncodeFinish(&enc);
  } else if (op.kind == kX86OpImm) {
    int imm_size = size == kX86Size16 ? 2 : 4;
    if (op.sym == NULL && op.imm >= -128 && op.imm <= 127) {
      EncodeByte(&enc, 0x6a);
      imm_size = 1;
    } else {
      EncodeByte(&enc, 0x68);
    }
    if (!CheckImmRange(assembler, &op, imm_size)) {
      return;
    }
    FinishWithImm(assembler, &enc, &op, imm_size, true);
  } else {
    EncodeByte(&enc, 0xff);
    EncodeRM(&enc, 6, &op);
    EncodeFinish(&enc);
  }
}

static void EmitIntPop(X86Assembler* assembler, X86Size suffix) {
  X86Op op;
  if (!ParseExactOperands(assembler, &op, 1, "pop")) {
    return;
  }
  if (op.kind == kX86OpImm) {
    AssemblerError(&ASM, "pop expects register or memory");
    return;
  }
  X86Size size;
  if (!ResolveStackSize(assembler, suffix, &op, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size16) {
    EncodeLegacyPrefix(&enc, 0x66);
  }
  if (op.kind == kX86OpReg) {
    EncodeOpcodePlusReg(&enc, 0x58, &op.reg);
  } else {
    EncodeByte(&enc, 0x8f);
    EncodeRM(&enc, 0, &op);
  }
  EncodeFinish(&enc);
}

// xchg (86/87), cmpxchg (0F B0/B1) and xadd (0F C0/C1): `reg, r/m`.
static void EmitRegRMExchange(X86Assembler* assembler, int opcode,
                              bool two_byte, X86Size suffix) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "Exchange")) {
    return;
  }
  // xchg is symmetric, so `xchg (%rdi), %eax` is accepted too.
  const X86Op* reg = ops[0].kind == kX86OpReg ? &ops[0] : &ops[1];
  const X86Op* rm = reg == &ops[0] ? &ops[1] : &ops[0];
  if (reg->kind != kX86OpReg || rm->kind == kX86OpImm ||
      (two_byte && reg != &ops[0])) {
    AssemblerError(&ASM, "Exchange expects register, register-or-memory");
    return;
  }
  X86Op* sized[] = {&ops[0], &ops[1]};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  if (two_byte) {
    EncodeByte(&enc, 0x0f);
  }
  EncodeByte(&enc, (uint8_t)(size == kX86Size8 ? opcode : opcode + 1));
  EncodeRegRM(&enc, &reg->reg, rm);
  EncodeFinish(&enc);
}

static void EmitIntImul(X86Assembler* assembler, X86Size suffix) {
  X86Op ops[3];
  int count = ParseOperandList(assembler, ops, 3);
  if (count < 0) {
    return;
  }
  if (count == 0) {
    AssemblerError(&ASM, "imul expects operands");
    return;
  }
  if (count == 1) {
    if (ops[0].kind == kX86OpImm) {
      AssemblerError(&ASM, "imul expects register or memory");
      return;
    }
    X86Op* sized[] = {&ops[0]};
    X86Size size;
    if (!ResolveIntSize(assembler, suffix, sized, 1, &size)) {
      return;
    }
    X86Encode enc;
    EncodeInit(&enc, assembler);
    if (!EncodeOperandSize(&enc, size)) {
      return;
    }
    EncodeByte(&enc, size == kX86Size8 ? 0xf6 : 0xf7);
    EncodeRM(&enc, 5, &ops[0]);
    EncodeFinish(&enc);
    return;
  }
  // `imul $imm, %reg` is shorthand for `imul $imm, %reg, %reg`.
  if (count == 2 && ops[0].kind == kX86OpImm) {
    ops[2] = ops[1];
    count = 3;
  }
  const X86Op* imm = count == 3 ? &ops[0] : NULL;
  X86Op* src = &ops[count - 2];
  X86Op* dst = &ops[count - 1];
  if ((imm != NULL && imm->kind != kX86OpImm) || src->kind == kX86OpImm ||
      dst->kind != kX86OpReg) {
    AssemblerError(&ASM, "Unsupported imul operand combination");
    return;
  }
  X86Op* sized[] = {src, dst};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  if (size == kX86Size8) {
    AssemblerError(&ASM, "imul has no two- or three-operand byte form");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  if (imm == NULL) {
    EncodeByte(&enc, 0x0f);
    EncodeByte(&enc, 0xaf);
    EncodeRegRM(&enc, &dst->reg, src);
    EncodeFinish(&enc);
    return;
  }
  bool short_imm = imm->sym == NULL && imm->imm >= -128 && imm->imm <= 127;
  int imm_size = short_imm ? 1 : ImmFieldSize(size);
  if (!CheckImmRange(assembler, imm, imm_size)) {
    return;
  }
  EncodeByte(&enc, short_imm ? 0x6b : 0x69);
  enc.tail_bytes = imm_size;
  EncodeRegRM(&enc, &dst->reg, src);
  FinishWithImm(assembler, &enc, imm, imm_size, true);
}

// bsf/bsr (0F BC/BD) and the F3-prefixed tzcnt/lzcnt/popcnt: `r/m, reg`.
static void EmitBitScan(X86Assembler* assembler, int opcode, X86Size suffix) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "Bit scan")) {
    return;
  }
  if (ops[1].kind != kX86OpReg || ops[0].kind == kX86OpImm) {
    AssemblerError(&ASM, "Bit scan expects r/m, register");
    return;
  }
  X86Op* sized[] = {&ops[0], &ops[1]};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  if (size == kX86Size8) {
    AssemblerError(&ASM, "Bit scan has no byte form");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  if (opcode & 0x100) {
    EncodeLegacyPrefix(&enc, 0xf3);
  }
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, (uint8_t)opcode);
  EncodeRegRM(&enc, &ops[1].reg, &ops[0]);
  EncodeFinish(&enc);
}

// bt/bts/btr/btc: `reg|$imm, r/m`.
static void EmitBitTest(X86Assembler* assembler, int ext, X86Size suffix) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "Bit test")) {
    return;
  }
  if (ops[1].kind == kX86OpImm) {
    AssemblerError(&ASM, "Bit test expects a register or memory base");
    return;
  }
  X86Op* sized[] = {&ops[0], &ops[1]};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  if (size == kX86Size8) {
    AssemblerError(&ASM, "Bit test has no byte form");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  EncodeByte(&enc, 0x0f);
  if (ops[0].kind == kX86OpImm && ops[0].sym == NULL) {
    EncodeByte(&enc, 0xba);
    enc.tail_bytes = 1;
    EncodeRM(&enc, ext, &ops[1]);
    EncodeImm(&enc, 1, ops[0].imm & 0xff);
  } else if (ops[0].kind == kX86OpReg) {
    EncodeByte(&enc, (uint8_t)(0xa3 + (ext - 4) * 8));
    EncodeRegRM(&enc, &ops[0].reg, &ops[1]);
  } else {
    AssemblerError(&ASM, "Bit index must be a register or immediate");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitBswap(X86Assembler* assembler, X86Size suffix) {
  X86Op op;
  if (!ParseExactOperands(assembler, &op, 1, "bswap")) {
    return;
  }
  if (op.kind != kX86OpReg) {
    AssemblerError(&ASM, "bswap expects a register");
    return;
  }
  X86Op* sized[] = {&op};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 1, &size)) {
    return;
  }
  if (size != kX86Size32 && size != kX86Size64) {
    AssemblerError(&ASM, "bswap expects a 32- or 64-bit register");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  EncodeByte(&enc, 0x0f);
  EncodeOpcodePlusReg(&enc, 0xc8, &op.reg);
  EncodeFinish(&enc);
}

// movs/cmps/stos/lods/scas.  Their operands are implicit; GNU accepts them
// written out, and they are only checked for being memory.
static void EmitStringOp(X86Assembler* assembler, uint8_t opcode,
                         X86Size suffix) {
  X86Op ops[2];
  int count = ParseOperandList(assembler, ops, 2);
  if (count < 0) {
    return;
  }
  for (int i = 0; i < count; i++) {
    if (ops[i].kind == kX86OpImm) {
      AssemblerError(&ASM, "String instruction operands are memory");
      return;
    }
  }
  if (suffix == kX86SizeNone) {
    AssemblerError(&ASM, "String instruction needs a size suffix");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, suffix)) {
    return;
  }
  EncodeByte(&enc, (uint8_t)(suffix == kX86Size8 ? opcode : opcode + 1));
  EncodeFinish(&enc);
}

// `nop` is 0x90; `nopw`/`nopl` with an operand are the 0F 1F /0 multi-byte
// padding nops.
static void EmitNop(X86Assembler* assembler, X86Size suffix) {
  X86Op op;
  int count = ParseOperandList(assembler, &op, 1);
  if (count < 0) {
    return;
  }
  if (count == 0) {
    if (suffix != kX86SizeNone) {
      AssemblerError(&ASM, "nop%c expects an operand",
                     suffix == kX86Size16 ? 'w' : 'l');
      return;
    }
    AssemblerEmitByte(&ASM, ASMO.current_section, 0x90);
    return;
  }
  if (op.kind == kX86OpImm || (op.kind == kX86OpReg && op.reg.is_xmm)) {
    AssemblerError(&ASM, "nop expects register or memory");
    return;
  }
  X86Op* sized[] = {&op};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 1, &size)) {
    return;
  }
  if (size == kX86Size8) {
    AssemblerError(&ASM, "nop has no byte form");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, 0x1f);
  EncodeRM(&enc, 0, &op);
  EncodeFinish(&enc);
}

static void EmitSetcc(X86Assembler* assembler, int cc) {
  X86Op op;
  if (!ParseExactOperands(assembler, &op, 1, "setcc")) {
    return;
  }
  if (op.kind == kX86OpImm) {
    AssemblerError(&ASM, "setcc expects a byte register or memory");
    return;
  }
  // The emitter names the destination at full width (`sete %rax`).
  X86Op* sized[] = {&op};
  X86Size size;
  if (!ResolveIntSize(assembler, kX86Size8, sized, 1, &size)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, (uint8_t)(0x90 + cc));
  EncodeRM(&enc, 0, &op);
  EncodeFinish(&enc);
}

static void EmitCmov(X86Assembler* assembler, int cc, X86Size suffix) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "cmov")) {
    return;
  }
  if (ops[1].kind != kX86OpReg || ops[0].kind == kX86OpImm) {
    AssemblerError(&ASM, "cmov expects r/m, register");
    return;
  }
  X86Op* sized[] = {&ops[0], &ops[1]};
  X86Size size;
  if (!ResolveIntSize(assembler, suffix, sized, 2, &size)) {
    return;
  }
  if (size == kX86Size8) {
    AssemblerError(&ASM, "cmov has no byte form");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, (uint8_t)(0x40 + cc));
  EncodeRegRM(&enc, &ops[1].reg, &ops[0]);
  EncodeFinish(&enc);
}

// movzx/movsx/movslq: zero- or sign-extend `r/m` of src_size into a
// dst_size register.  A size of kX86SizeNone comes from the register.
static void EmitExtend(X86Assembler* assembler, bool is_signed,
                       X86Size src_size, X86Size dst_size) {
  X86Op ops[2];
  if (!ParseExactOperands(assembler, ops, 2, "Extending move")) {
    return;
  }
  X86Op* src = &ops[0];
  X86Op* dst = &ops[1];
  if (dst->kind != kX86OpReg || src->kind == kX86OpImm) {
    AssemblerError(&ASM, "Extending move expects r/m, register");
    return;
  }
  if (src_size == kX86SizeNone) {
    if (src->kind != kX86OpReg) {
      AssemblerError(&ASM, "Extending move from memory needs a size suffix");
      return;
    }
    src_size = src->reg.size;
  }
  X86Op* src_sized[] = {src};
  X86Op* dst_sized[] = {dst};
  X86Size size;
  if (!ResolveIntSize(assembler, src_size, src_sized, 1, &size) ||
      !ResolveIntSize(assembler, dst_size, dst_sized, 1, &size)) {
    return;
  }
  if (size <= src_size || src_size == kX86Size64) {
    AssemblerError(&ASM, "Extending move must widen its operand");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (!EncodeOperandSize(&enc, size)) {
    return;
  }
  if (src_size == kX86Size32) {
    if (!is_signed || size != kX86Size64) {
      AssemblerError(&ASM, "Only movslq extends a 32-bit operand");
      return;
    }
    EncodeByte(&enc, 0x63);
  } else {
    EncodeByte(&enc, 0x0f);
    EncodeByte(&enc, (uint8_t)((is_signed ? 0xbe : 0xb6) +
                               (src_size == kX86Size16 ? 1 : 0)));
  }
  EncodeRegRM(&enc, &dst->reg, src);
  EncodeFinish(&enc);
}

static void EmitBranch(X86Assembler* assembler, int opcode, bool is_call) {
  int32_t start = (int32_t)AssemblerCurrentAddress(&ASM);
  bool known = false;
  bool plt = false;
  int64_t target = 0;
  AssemblerSymbol* sym = NULL;

  if (AssemblerLookingAtSymbol(&ASM)) {
    String name;
    String suffix;
    StringInit(&name, NULL);
    StringInit(&suffix, NULL);
    AssemblerExtractSymbolSuffix(&ASM.lex.spelling, &name, &suffix);
    plt = StringEqualCaseBlind(&suffix, "PLT");
    if (suffix.length > 0 && !plt) {
      AssemblerError(&ASM, "Unsupported branch target suffix '@%s'",
                     suffix.value);
    }
    sym = GetOrCreateSymbol(assembler, name.value);
    StringDestruct(&name);
    StringDestruct(&suffix);
    LexNextToken(&ASM.lex);
    // Keep calls to weak definitions relocatable so the linker can select a
    // strong override. Strong definitions retain the toolchain's established
    // local-binding behavior.
    known = !plt && sym->defined && sym->section == ASMO.current_section &&
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
    int reloc_type = X86DefaultPcReloc(assembler->profile, is_call || plt);
    if (!assembler->profile->is_64bit && (plt || (is_call && ASMO.pic))) {
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
  dst.reg.size = kX86Size64;
  if (!EncodeOperandSize(&enc, kX86Size64)) {
    return;
  }
  EncodeOpcodePlusReg(&enc, 0xb8, &dst.reg);
  FinishWithImm(assembler, &enc, &imm, 8, false);
}

static const struct {
  const char* name;
  int cc;
} kX86ConditionCodes[] = {
    {"o", 0},    {"no", 1},  {"b", 2},   {"c", 2},   {"nae", 2}, {"ae", 3},
    {"nb", 3},   {"nc", 3},  {"e", 4},   {"z", 4},   {"ne", 5},  {"nz", 5},
    {"be", 6},   {"na", 6},  {"a", 7},   {"nbe", 7}, {"s", 8},   {"ns", 9},
    {"p", 10},   {"pe", 10}, {"np", 11}, {"po", 11}, {"l", 12},  {"nge", 12},
    {"ge", 13},  {"nl", 13}, {"le", 14}, {"ng", 14}, {"g", 15},  {"nle", 15},
};

static int LookupConditionCode(const char* name, size_t len) {
  for (size_t i = 0; i < sizeof(kX86ConditionCodes) / sizeof(kX86ConditionCodes[0]);
       i++) {
    if (strlen(kX86ConditionCodes[i].name) == len &&
        strncmp(name, kX86ConditionCodes[i].name, len) == 0) {
      return kX86ConditionCodes[i].cc;
    }
  }
  return -1;
}

typedef enum {
  kX86FamALU,
  kX86FamUnary,
  kX86FamIncDec,
  kX86FamShift,
  kX86FamShiftDouble,
  kX86FamTest,
  kX86FamMov,
  kX86FamMovabs,
  kX86FamLea,
  kX86FamPush,
  kX86FamPop,
  kX86FamXchg,
  kX86FamCmpxchg,
  kX86FamImul,
  kX86FamBitScan,
  kX86FamBitTest,
  kX86FamBswap,
  kX86FamString,
  kX86FamNop,
} X86Family;

// Integer instruction families that take an optional b/w/l/q size suffix.
static const struct {
  const char* name;
  X86Family family;
  int code;
} kX86Families[] = {
    {"add", kX86FamALU, 0},          {"or", kX86FamALU, 1},
    {"adc", kX86FamALU, 2},          {"sbb", kX86FamALU, 3},
    {"and", kX86FamALU, 4},          {"sub", kX86FamALU, 5},
    {"xor", kX86FamALU, 6},          {"cmp", kX86FamALU, 7},
    {"not", kX86FamUnary, 2},        {"neg", kX86FamUnary, 3},
    {"mul", kX86FamUnary, 4},        {"div", kX86FamUnary, 6},
    {"idiv", kX86FamUnary, 7},       {"inc", kX86FamIncDec, 0},
    {"dec", kX86FamIncDec, 1},       {"rol", kX86FamShift, 0},
    {"ror", kX86FamShift, 1},        {"rcl", kX86FamShift, 2},
    {"rcr", kX86FamShift, 3},        {"shl", kX86FamShift, 4},
    {"sal", kX86FamShift, 4},        {"shr", kX86FamShift, 5},
    {"sar", kX86FamShift, 7},        {"shld", kX86FamShiftDouble, 0xa4},
    {"shrd", kX86FamShiftDouble, 0xac}, {"test", kX86FamTest, 0},
    {"mov", kX86FamMov, 0},          {"movabs", kX86FamMovabs, 0},
    {"lea", kX86FamLea, 0},          {"push", kX86FamPush, 0},
    {"pop", kX86FamPop, 0},          {"xchg", kX86FamXchg, 0x86},
    {"cmpxchg", kX86FamCmpxchg, 0xb0}, {"xadd", kX86FamCmpxchg, 0xc0},
    {"imul", kX86FamImul, 0},        {"bsf", kX86FamBitScan, 0xbc},
    {"bsr", kX86FamBitScan, 0xbd},   {"tzcnt", kX86FamBitScan, 0x1bc},
    {"lzcnt", kX86FamBitScan, 0x1bd}, {"popcnt", kX86FamBitScan, 0x1b8},
    {"bt", kX86FamBitTest, 4},       {"bts", kX86FamBitTest, 5},
    {"btr", kX86FamBitTest, 6},      {"btc", kX86FamBitTest, 7},
    {"bswap", kX86FamBswap, 0},      {"movs", kX86FamString, 0xa4},
    {"cmps", kX86FamString, 0xa6},   {"stos", kX86FamString, 0xaa},
    {"lods", kX86FamString, 0xac},   {"scas", kX86FamString, 0xae},
    {"nop", kX86FamNop, 0},
};

static int LookupFamily(const char* name, size_t len) {
  for (size_t i = 0; i < sizeof(kX86Families) / sizeof(kX86Families[0]); i++) {
    if (strlen(kX86Families[i].name) == len &&
        strncmp(name, kX86Families[i].name, len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void EmitFamily(X86Assembler* assembler, int index, X86Size suffix) {
  int code = kX86Families[index].code;
  switch (kX86Families[index].family) {
    case kX86FamALU:
      EmitIntALU(assembler, code, suffix);
      break;
    case kX86FamUnary:
      EmitIntUnary(assembler, 0xf6, code, suffix);
      break;
    case kX86FamIncDec:
      EmitIntUnary(assembler, 0xfe, code, suffix);
      break;
    case kX86FamShift:
      EmitIntShift(assembler, code, suffix);
      break;
    case kX86FamShiftDouble:
      EmitShiftDouble(assembler, (uint8_t)code, suffix);
      break;
    case kX86FamTest:
      EmitIntTest(assembler, suffix);
      break;
    case kX86FamMov:
      EmitIntMov(assembler, suffix);
      break;
    case kX86FamMovabs:
      EmitMovabs(assembler);
      break;
    case kX86FamLea:
      EmitIntLea(assembler, suffix);
      break;
    case kX86FamPush:
      EmitIntPush(assembler, suffix);
      break;
    case kX86FamPop:
      EmitIntPop(assembler, suffix);
      break;
    case kX86FamXchg:
      EmitRegRMExchange(assembler, code, false, suffix);
      break;
    case kX86FamCmpxchg:
      EmitRegRMExchange(assembler, code, true, suffix);
      break;
    case kX86FamImul:
      EmitIntImul(assembler, suffix);
      break;
    case kX86FamBitScan:
      EmitBitScan(assembler, code, suffix);
      break;
    case kX86FamBitTest:
      EmitBitTest(assembler, code, suffix);
      break;
    case kX86FamBswap:
      EmitBswap(assembler, suffix);
      break;
    case kX86FamString:
      EmitStringOp(assembler, (uint8_t)code, suffix);
      break;
    case kX86FamNop:
      EmitNop(assembler, suffix);
      break;
  }
}

// `movzbl`, `movswq`, `movslq`, `movsx`, `movzx`, `movsxd`.
static bool ParseExtendMnemonic(const char* name, bool* is_signed,
                                X86Size* src_size, X86Size* dst_size) {
  size_t len = strlen(name);
  if (len < 5 || strncmp(name, "mov", 3) != 0 ||
      (name[3] != 's' && name[3] != 'z')) {
    return false;
  }
  *is_signed = name[3] == 's';
  *src_size = kX86SizeNone;
  *dst_size = kX86SizeNone;
  const char* rest = name + 4;
  if (strcmp(rest, "x") == 0) {
    return true;
  }
  if (*is_signed && strcmp(rest, "xd") == 0) {
    *src_size = kX86Size32;
    return true;
  }
  if (strlen(rest) != 2) {
    return false;
  }
  *src_size = SuffixSize(rest[0]);
  *dst_size = SuffixSize(rest[1]);
  return *src_size != kX86SizeNone && *dst_size != kX86SizeNone &&
         *src_size < *dst_size && (*is_signed || *src_size != kX86Size32);
}

// Instructions that are a fixed byte sequence.
static const struct {
  const char* name;
  const char* bytes;
  int length;
} kX86FixedInstructions[] = {
    {"pause", "\xf3\x90", 2},        {"ud2", "\x0f\x0b", 2},
    {"hlt", "\xf4", 1},              {"int3", "\xcc", 1},
    {"lfence", "\x0f\xae\xe8", 3},   {"sfence", "\x0f\xae\xf8", 3},
    {"clc", "\xf8", 1},              {"stc", "\xf9", 1},
    {"cld", "\xfc", 1},              {"std", "\xfd", 1},
    {"cmc", "\xf5", 1},              {"lahf", "\x9f", 1},
    {"sahf", "\x9e", 1},             {"xgetbv", "\x0f\x01\xd0", 3},
    {"rdtscp", "\x0f\x01\xf9", 3},   {"emms", "\x0f\x77", 2},
    {"endbr64", "\xf3\x0f\x1e\xfa", 4}, {"endbr32", "\xf3\x0f\x1e\xfb", 4},
    {"fninit", "\xdb\xe3", 2},       {"finit", "\x9b\xdb\xe3", 3},
    {"fnclex", "\xdb\xe2", 2},       {"fclex", "\x9b\xdb\xe2", 3},
    {"fwait", "\x9b", 1},            {"wait", "\x9b", 1},
    {"cbtw", "\x66\x98", 2},         {"cbw", "\x66\x98", 2},
    {"cwtl", "\x98", 1},             {"cwde", "\x98", 1},
    {"cwtd", "\x66\x99", 2},         {"cwd", "\x66\x99", 2},
    {"cltd", "\x99", 1},             {"cdq", "\x99", 1},
};

// Instructions with a single memory operand: `[9B] op0 [op1] /ext`.  A 9B
// (fwait) prefix makes the waiting form of an x87 "fn" instruction.
static const struct {
  const char* name;
  bool fwait;
  int op0;
  int op1;
  int ext;
} kX86MemoryInstructions[] = {
    {"fnstenv", false, 0xd9, -1, 6}, {"fstenv", true, 0xd9, -1, 6},
    {"fldenv", false, 0xd9, -1, 4},  {"fnstcw", false, 0xd9, -1, 7},
    {"fstcw", true, 0xd9, -1, 7},    {"fldcw", false, 0xd9, -1, 5},
    {"fnsave", false, 0xdd, -1, 6},  {"fsave", true, 0xdd, -1, 6},
    {"frstor", false, 0xdd, -1, 4},  {"fxsave", false, 0x0f, 0xae, 0},
    {"fxrstor", false, 0x0f, 0xae, 1}, {"ldmxcsr", false, 0x0f, 0xae, 2},
    {"stmxcsr", false, 0x0f, 0xae, 3}, {"clflush", false, 0x0f, 0xae, 7},
};

static void EmitMemoryInstruction(X86Assembler* assembler, size_t index) {
  X86Op op;
  if (!ParseExactOperands(assembler, &op, 1, kX86MemoryInstructions[index].name)) {
    return;
  }
  if (op.kind != kX86OpMem) {
    AssemblerError(&ASM, "%s expects a memory operand",
                   kX86MemoryInstructions[index].name);
    return;
  }
  if (kX86MemoryInstructions[index].fwait) {
    AssemblerEmitByte(&ASM, ASMO.current_section, 0x9b);
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeByte(&enc, (uint8_t)kX86MemoryInstructions[index].op0);
  if (kX86MemoryInstructions[index].op1 >= 0) {
    EncodeByte(&enc, (uint8_t)kX86MemoryInstructions[index].op1);
  }
  EncodeMemOperand(&enc, kX86MemoryInstructions[index].ext, &op);
  EncodeFinish(&enc);
}

// The instructions not in the instruction map: families with an optional
// size suffix, condition-code forms, extends, and fixed encodings.
static bool AssembleX86Generic(X86Assembler* assembler, const char* name) {
  size_t len = strlen(name);
  int index = LookupFamily(name, len);
  if (index >= 0) {
    EmitFamily(assembler, index, kX86SizeNone);
    return true;
  }
  bool is_signed;
  X86Size src_size;
  X86Size dst_size;
  if (ParseExtendMnemonic(name, &is_signed, &src_size, &dst_size)) {
    EmitExtend(assembler, is_signed, src_size, dst_size);
    return true;
  }
  if (len > 1) {
    X86Size suffix = SuffixSize(name[len - 1]);
    index = LookupFamily(name, len - 1);
    if (index >= 0 && kX86Families[index].family == kX86FamString &&
        name[len - 1] == 'd') {
      suffix = kX86Size32;
    }
    if (index >= 0 && suffix != kX86SizeNone) {
      EmitFamily(assembler, index, suffix);
      return true;
    }
  }
  int cc;
  if (name[0] == 'j' && (cc = LookupConditionCode(name + 1, len - 1)) >= 0) {
    EmitBranch(assembler, 0x80 + cc, false);
    return true;
  }
  if (strncmp(name, "set", 3) == 0 &&
      (cc = LookupConditionCode(name + 3, len - 3)) >= 0) {
    EmitSetcc(assembler, cc);
    return true;
  }
  if (strncmp(name, "cmov", 4) == 0) {
    if ((cc = LookupConditionCode(name + 4, len - 4)) >= 0) {
      EmitCmov(assembler, cc, kX86SizeNone);
      return true;
    }
    X86Size suffix = SuffixSize(name[len - 1]);
    if (len > 5 && suffix != kX86SizeNone && suffix != kX86Size8 &&
        (cc = LookupConditionCode(name + 4, len - 5)) >= 0) {
      EmitCmov(assembler, cc, suffix);
      return true;
    }
  }
  for (size_t i = 0;
       i < sizeof(kX86FixedInstructions) / sizeof(kX86FixedInstructions[0]);
       i++) {
    if (strcmp(name, kX86FixedInstructions[i].name) == 0) {
      for (int b = 0; b < kX86FixedInstructions[i].length; b++) {
        AssemblerEmitByte(&ASM, ASMO.current_section,
                          (uint8_t)kX86FixedInstructions[i].bytes[b]);
      }
      return true;
    }
  }
  for (size_t i = 0;
       i < sizeof(kX86MemoryInstructions) / sizeof(kX86MemoryInstructions[0]);
       i++) {
    if (strcmp(name, kX86MemoryInstructions[i].name) == 0) {
      EmitMemoryInstruction(assembler, i);
      return true;
    }
  }
  return false;
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

// Indirect branches always use the full address width, with no REX.W.
static void EncodeBranchTargetReg(X86Encode* enc, int reg_field,
                                  const X86Reg* reg) {
  X86Size width = enc->assembler->profile->is_64bit ? kX86Size64 : kX86Size32;
  if (reg->is_xmm || reg->size != width) {
    AssemblerError(&enc->assembler->base,
                   "Indirect branch target must be a %d-bit register",
                   (int)width * 8);
    return;
  }
  EncodeModRM(enc, 3, reg_field, reg->num);
}

static void EmitIndirectCall(X86Assembler* assembler) {
  (void)LexMatch(&ASM.lex, TOK(star));
  X86Op target;
  if (!ParseOperand(assembler, &target)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeByte(&enc, 0xff);
  if (target.kind == kX86OpReg) {
    EncodeBranchTargetReg(&enc, 2, &target.reg);
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
  EncodeByte(&enc, 0xff);
  if (target.kind == kX86OpReg) {
    EncodeBranchTargetReg(&enc, 4, &target.reg);
  } else if (target.kind == kX86OpMem) {
    EncodeMemOperand(&enc, 4, &target);
  } else {
    AssemblerError(&ASM, "Indirect jump expects register or memory target");
    return;
  }
  EncodeFinish(&enc);
}

static void EmitSSE(X86Assembler* assembler, uint8_t prefix66, uint8_t prefix_f2,
                    uint8_t prefix_f3, uint8_t opcode) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg || !dst.reg.is_xmm) {
    AssemblerError(&ASM, "SSE instruction expects XMM destination");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EmitOpcodeBytes(&enc, prefix66, prefix_f2, prefix_f3, opcode, true);
  if (src.kind == kX86OpReg && src.reg.is_xmm) {
    EncodeXmmRegOperand(&enc, dst.reg.num, &src.reg);
  } else if (src.kind == kX86OpMem) {
    EncodeMemOperand(&enc, dst.reg.num, &src);
  } else {
    AssemblerError(&ASM, "SSE instruction expects XMM or memory source");
    return;
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
    // encoding, so materialize the bit pattern in the reserved scratch GPR
    // (r11, never allocated) and then perform the ordinary GPR-to-XMM move.
    X86Reg scratch = {11, kX86Size64, false};
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

static void Assemble_atomic_cmpxchg8b(X86Assembler* assembler) {
  X86Op dst;
  if (!ParseOperand(assembler, &dst) || dst.kind != kX86OpMem) {
    AssemblerError(&ASM, "atomic_cmpxchg8b expects a memory operand");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeLegacyPrefix(&enc, 0xf0);
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, 0xc7);
  EncodeMemOperand(&enc, 1, &dst);
  EncodeFinish(&enc);
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
static void Assemble_rcall(X86Assembler* assembler) { EmitIndirectCall(assembler); }
static void Assemble_movss(X86Assembler* assembler) { EmitSSEMove(assembler, false, true, false); }
static void Assemble_movsd(X86Assembler* assembler) {
  // Without operands, `movsd` is the dword string move.
  if (AtStatementEnd(assembler)) {
    EmitStringOp(assembler, 0xa4, kX86Size32);
    return;
  }
  EmitSSEMove(assembler, true, false, false);
}
static void Assemble_storesd(X86Assembler* assembler) { EmitSSEMove(assembler, true, false, true); }
static void Assemble_storess(X86Assembler* assembler) { EmitSSEMove(assembler, false, true, true); }
static void Assemble_addss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x58); }
static void Assemble_addsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x58); }
static void Assemble_subss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x5c); }
static void Assemble_subsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x5c); }
static void Assemble_mulss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x59); }
static void Assemble_mulsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x59); }
static void Assemble_divss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x5e); }
static void Assemble_divsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x5e); }
static void Assemble_sqrtss(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x51); }
static void Assemble_sqrtsd(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x51); }
static void Assemble_ucomiss(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x2e); }
static void Assemble_ucomisd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x2e); }
static void Assemble_paddb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfc); }
static void Assemble_paddw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfd); }
static void Assemble_paddd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfe); }
static void Assemble_paddq(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xd4); }
static void Assemble_psubb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xf8); }
static void Assemble_psubw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xf9); }
static void Assemble_psubd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfa); }
static void Assemble_psubq(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xfb); }
static void Assemble_pand(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xdb); }
static void Assemble_por(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xeb); }
static void Assemble_pxor(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0xef); }
static void Assemble_pcmpeqb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x74); }
static void Assemble_pcmpeqw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x75); }
static void Assemble_pcmpeqd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x76); }
static void Assemble_pcmpgtb(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x64); }
static void Assemble_pcmpgtw(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x65); }
static void Assemble_pcmpgtd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x66); }
static void Assemble_addps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x58); }
static void Assemble_addpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x58); }
static void Assemble_subps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x5c); }
static void Assemble_subpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x5c); }
static void Assemble_mulps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x59); }
static void Assemble_mulpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x59); }
static void Assemble_divps(X86Assembler* assembler) { EmitSSE(assembler, false, false, false, 0x5e); }
static void Assemble_divpd(X86Assembler* assembler) { EmitSSE(assembler, true, false, false, 0x5e); }

static bool CheckConvertIntSize(X86Assembler* assembler, X86Size size,
                                X86Size suffix) {
  if (size != kX86Size32 && size != kX86Size64) {
    AssemblerError(&ASM, "Convert expects a 32- or 64-bit integer register");
    return false;
  }
  if (suffix != kX86SizeNone && suffix != size) {
    AssemblerError(&ASM, "Operand size does not match the instruction suffix");
    return false;
  }
  return true;
}

// cvtsi2ss/cvtsi2sd.  `suffix` is the l/q mnemonic suffix, if any; it gives
// the integer width of a memory source, which is 32 bits otherwise.
static void EmitSSEConvertFromInt(X86Assembler* assembler, uint8_t prefix_f2,
                                  uint8_t prefix_f3, uint8_t opcode,
                                  X86Size suffix) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg || !dst.reg.is_xmm ||
      (src.kind != kX86OpMem && (src.kind != kX86OpReg || src.reg.is_xmm))) {
    AssemblerError(&ASM, "Integer-to-SSE convert expects GPR/memory source, XMM dest");
    return;
  }
  X86Size size = suffix != kX86SizeNone ? suffix : kX86Size32;
  if (src.kind == kX86OpReg) {
    if (!CheckConvertIntSize(assembler, src.reg.size, suffix)) {
      return;
    }
    size = src.reg.size;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (size == kX86Size64) {
    SetRexW(&enc);
  }
  EmitOpcodeBytes(&enc, false, prefix_f2, prefix_f3, opcode, true);
  if (src.kind == kX86OpReg) {
    EncodeRegOperand(&enc, dst.reg.num, &src.reg);
  } else {
    EncodeMemOperand(&enc, dst.reg.num, &src);
  }
  EncodeFinish(&enc);
}

// cvt(t)ss2si/cvt(t)sd2si: XMM or memory source, GPR destination.
static void EmitSSEConvertToInt(X86Assembler* assembler, uint8_t prefix_f2,
                                uint8_t prefix_f3, uint8_t opcode,
                                X86Size suffix) {
  X86Op src, dst;
  if (!ParseOperand(assembler, &src) || !ExpectComma(assembler) ||
      !ParseOperand(assembler, &dst)) {
    return;
  }
  if (dst.kind != kX86OpReg || dst.reg.is_xmm ||
      (src.kind != kX86OpMem && (src.kind != kX86OpReg || !src.reg.is_xmm))) {
    AssemblerError(&ASM, "SSE-to-integer convert expects XMM/memory source, GPR dest");
    return;
  }
  if (!CheckConvertIntSize(assembler, dst.reg.size, suffix)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  if (dst.reg.size == kX86Size64) {
    SetRexW(&enc);
  }
  EmitOpcodeBytes(&enc, false, prefix_f2, prefix_f3, opcode, true);
  if (src.kind == kX86OpReg) {
    EncodeXmmRegOperand(&enc, dst.reg.num, &src.reg);
  } else {
    EncodeMemOperand(&enc, dst.reg.num, &src);
  }
  EncodeFinish(&enc);
}

#define SSE_CONVERT(name, helper, f2, f3, opcode)                             \
  static void Assemble_##name(X86Assembler* assembler) {                     \
    helper(assembler, f2, f3, opcode, kX86SizeNone);                          \
  }                                                                           \
  static void Assemble_##name##l(X86Assembler* assembler) {                  \
    helper(assembler, f2, f3, opcode, kX86Size32);                            \
  }                                                                           \
  static void Assemble_##name##q(X86Assembler* assembler) {                  \
    helper(assembler, f2, f3, opcode, kX86Size64);                            \
  }

SSE_CONVERT(cvtsi2ss, EmitSSEConvertFromInt, false, true, 0x2a)
SSE_CONVERT(cvtsi2sd, EmitSSEConvertFromInt, true, false, 0x2a)
SSE_CONVERT(cvttss2si, EmitSSEConvertToInt, false, true, 0x2c)
SSE_CONVERT(cvttsd2si, EmitSSEConvertToInt, true, false, 0x2c)
SSE_CONVERT(cvtss2si, EmitSSEConvertToInt, false, true, 0x2d)
SSE_CONVERT(cvtsd2si, EmitSSEConvertToInt, true, false, 0x2d)
#undef SSE_CONVERT

static void Assemble_cvtss2sd(X86Assembler* assembler) { EmitSSE(assembler, false, false, true, 0x5a); }
static void Assemble_cvtsd2ss(X86Assembler* assembler) { EmitSSE(assembler, false, true, false, 0x5a); }
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
  if (!AtStatementEnd(assembler)) {
    bool known = false;
    int64_t imm = AssemblerEvaluateKnownExpression(&ASM, &known);
    if (!known || imm < 0 || imm > 0xffff) {
      AssemblerError(&ASM, "ret immediate must be a 16-bit constant");
      return;
    }
    AssemblerEmitByte(&ASM, ASMO.current_section, 0xc2);
    AssemblerEmitHalf(&ASM, ASMO.current_section, (uint16_t)imm);
    return;
  }
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xc3);
}
static void Assemble_leave(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xc9);
}
static void Assemble_int(X86Assembler* assembler) {
  X86Op op;
  if (!ParseOperand(assembler, &op) || op.kind != kX86OpImm ||
      op.sym != NULL || op.imm < 0 || op.imm > 0xff) {
    AssemblerError(&ASM, "int expects an 8-bit immediate");
    return;
  }
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xcd);
  AssemblerEmitByte(&ASM, ASMO.current_section, (uint8_t)op.imm);
}
static void Assemble_pushf(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x9c);
}
static void Assemble_popf(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x9d);
}
static void EmitStatusWordStore(X86Assembler* assembler, bool fwait) {
  X86Op op;
  if (!ParseOperand(assembler, &op)) {
    return;
  }
  if (fwait) {
    AssemblerEmitByte(&ASM, ASMO.current_section, 0x9b);
  }
  if (op.kind == kX86OpReg && !op.reg.is_xmm && op.reg.num == X86_REG_RAX &&
      op.reg.size == kX86Size16) {
    AssemblerEmitByte(&ASM, ASMO.current_section, 0xdf);
    AssemblerEmitByte(&ASM, ASMO.current_section, 0xe0);
    return;
  }
  if (op.kind != kX86OpMem) {
    AssemblerError(&ASM, "fstsw expects %%ax or memory");
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeByte(&enc, 0xdd);
  EncodeMemOperand(&enc, 7, &op);
  EncodeFinish(&enc);
}
static void Assemble_fnstsw(X86Assembler* assembler) {
  EmitStatusWordStore(assembler, false);
}
static void Assemble_fstsw(X86Assembler* assembler) {
  EmitStatusWordStore(assembler, true);
}

void AssembleX86Instruction(Assembler* base, String* word);

// rep/lock and friends: emit the prefix byte, then the instruction that
// follows on the same line, if any (`rep movsb`, `rep; stosq`, `lock`).
static void EmitPrefixed(X86Assembler* assembler, uint8_t prefix) {
  AssemblerEmitByte(&ASM, ASMO.current_section, prefix);
  (void)LexMatch(&ASM.lex, TOK(semicolon));
  if (!AssemblerLookingAtSymbol(&ASM)) {
    return;
  }
  String word;
  StringInit(&word, NULL);
  StringSetString(&word, &ASM.lex.spelling);
  LexNextToken(&ASM.lex);
  AssembleX86Instruction(&ASM, &word);
  StringDestruct(&word);
}
static void Assemble_rep(X86Assembler* assembler) {
  EmitPrefixed(assembler, 0xf3);
}
static void Assemble_repne(X86Assembler* assembler) {
  EmitPrefixed(assembler, 0xf2);
}
static void Assemble_lock(X86Assembler* assembler) {
  EmitPrefixed(assembler, 0xf0);
}

static void Assemble_syscall(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x0f);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x05);
}

static void Assemble_rdtsc(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x0f);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x31);
}

static void Assemble_cpuid(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x0f);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xa2);
}

static void Assemble_mfence(X86Assembler* assembler) {
  (void)assembler;
  AssemblerEmitByte(&ASM, ASMO.current_section, 0x0f);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xae);
  AssemblerEmitByte(&ASM, ASMO.current_section, 0xf0);
}

// PREFETCHW m8 — hint that data will be modified (0F 0D /1).
static void EmitPrefetchHint(X86Assembler* assembler, uint8_t opcode2,
                             int modrm_reg) {
  X86Op src;
  if (!ParseOperand(assembler, &src)) {
    return;
  }
  X86Encode enc;
  EncodeInit(&enc, assembler);
  EncodeByte(&enc, 0x0f);
  EncodeByte(&enc, opcode2);
  if (src.kind == kX86OpReg) {
    X86Op mem;
    InitMemOp(&mem);
    mem.base = src.reg;
    EncodeMemOperand(&enc, modrm_reg, &mem);
  } else if (src.kind == kX86OpMem) {
    EncodeMemOperand(&enc, modrm_reg, &src);
  } else {
    AssemblerError(&ASM, "prefetch operand must be a memory address");
    return;
  }
  EncodeFinish(&enc);
}

static void Assemble_prefetchw(X86Assembler* assembler) {
  EmitPrefetchHint(assembler, 0x0d, 1);
}

static void Assemble_prefetchnta(X86Assembler* assembler) {
  EmitPrefetchHint(assembler, 0x18, 0);
}
static void Assemble_prefetcht0(X86Assembler* assembler) {
  EmitPrefetchHint(assembler, 0x18, 1);
}
static void Assemble_prefetcht1(X86Assembler* assembler) {
  EmitPrefetchHint(assembler, 0x18, 2);
}
static void Assemble_prefetcht2(X86Assembler* assembler) {
  EmitPrefetchHint(assembler, 0x18, 3);
}

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

// Instructions not listed here are handled by AssembleX86Generic.
static void InitializeInstructions(Map* instructions) {
  INST(movss);
  INST(movsd);
  INST(storess);
  INST(storesd);
  INST(movd);
  INST(movq_xmm);
  INST(movdqu);
  INST(atomic_cmpxchgb);
  INST(atomic_cmpxchgw);
  INST(atomic_cmpxchgl);
  INST(atomic_cmpxchgq);
  INST(atomic_cmpxchg8b);
  INST(atomic_xaddb);
  INST(atomic_xaddw);
  INST(atomic_xaddl);
  INST(atomic_xaddq);
  INST(cqo);
  INST(cdq);
  INST(cltq);
  INST(prefetchw);
  INST(prefetchnta);
  INST(prefetcht0);
  INST(prefetcht1);
  INST(prefetcht2);
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
  INST(cvtsi2ssl);
  INST(cvtsi2ssq);
  INST(cvtsi2sd);
  INST(cvtsi2sdl);
  INST(cvtsi2sdq);
  INST(cvttss2si);
  INST(cvttss2sil);
  INST(cvttss2siq);
  INST(cvttsd2si);
  INST(cvttsd2sil);
  INST(cvttsd2siq);
  INST(cvtss2si);
  INST(cvtss2sil);
  INST(cvtss2siq);
  INST(cvtsd2si);
  INST(cvtsd2sil);
  INST(cvtsd2siq);
  INST(cvtss2sd);
  INST(cvtsd2ss);
  INST(call);
  INST(rcall);
  INST(jmp);
  INST(ret);
  INST(leave);
  INST(syscall);
  INST(rdtsc);
  INST(cpuid);
  INST(mfence);
  INST(int);
  INST(pushf);
  INST(popf);
  INST(fnstsw);
  INST(fstsw);
  INST(rep);
  INST(repne);
  INST(lock);
  INST2(cqo, cqto);
  INST2(cltq, cdqe);
  INST2(call, callq);
  INST2(call, calll);
  INST2(jmp, jmpq);
  INST2(jmp, jmpl);
  INST2(ret, retq);
  INST2(ret, retl);
  INST2(leave, leaveq);
  INST2(leave, leavel);
  INST2(pushf, pushfq);
  INST2(pushf, pushfl);
  INST2(popf, popfq);
  INST2(popf, popfl);
  INST2(rep, repe);
  INST2(rep, repz);
  INST2(repne, repnz);
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
  } else if (!AssembleX86Generic(assembler, word->value)) {
    AssemblerError(&assembler->base, "Syntax error; unknown instruction: %s",
                   word->value);
    return;
  }
  if (!AtStatementEnd(assembler)) {
    AssemblerError(&ASM, "Junk at end of line after %s", word->value);
  }
}


bool X86AssemblerInit(X86Assembler* assembler, String* infile,
                           String* outfile) {
  return X86AssemblerInitWithProfile(assembler, infile, outfile, &kX86ProfileAMD64);
}
