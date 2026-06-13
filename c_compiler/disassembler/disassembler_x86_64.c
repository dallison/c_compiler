//
//  disassembler_x86_64.c
//  c_compiler
//

#include "disassembler_internal.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const unsigned char* bytes;
  size_t length;
  size_t pos;
  uint64_t address;
  unsigned char rex;
  unsigned char prefix66;
  unsigned char prefixf2;
  unsigned char prefixf3;
} X86Decoder;

static const char* RegName(int reg, int width) {
  static char bufs[8][8];
  static int index = 0;
  static const char* r64[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp",
                              "rsi", "rdi", "r8",  "r9",  "r10", "r11",
                              "r12", "r13", "r14", "r15"};
  static const char* r32[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp",
                              "esi", "edi", "r8d", "r9d", "r10d", "r11d",
                              "r12d", "r13d", "r14d", "r15d"};
  static const char* r16[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
                              "r8w", "r9w", "r10w", "r11w", "r12w", "r13w",
                              "r14w", "r15w"};
  static const char* r8[] = {"al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
                             "r8b", "r9b", "r10b", "r11b", "r12b", "r13b",
                             "r14b", "r15b"};
  if (reg < 16) {
    if (width == 8) return r64[reg];
    if (width == 4) return r32[reg];
    if (width == 2) return r16[reg];
    return r8[reg];
  }
  char* buf = bufs[index++ & 7];
  snprintf(buf, 8, "r%d", reg);
  return buf;
}

static const char* XMMName(int reg) {
  static char bufs[4][8];
  static int index = 0;
  char* buf = bufs[index++ & 3];
  snprintf(buf, 8, "xmm%d", reg);
  return buf;
}

static bool Need(X86Decoder* d, size_t n) { return d->pos + n <= d->length; }

static unsigned char Get8(X86Decoder* d) {
  return Need(d, 1) ? d->bytes[d->pos++] : 0;
}

static uint32_t Get32(X86Decoder* d) {
  if (!Need(d, 4)) {
    d->pos = d->length;
    return 0;
  }
  uint32_t v = DAsmRead32LE(d->bytes + d->pos);
  d->pos += 4;
  return v;
}

static uint16_t Get16(X86Decoder* d) {
  if (!Need(d, 2)) {
    d->pos = d->length;
    return 0;
  }
  uint16_t v = (uint16_t)d->bytes[d->pos] |
               ((uint16_t)d->bytes[d->pos + 1] << 8);
  d->pos += 2;
  return v;
}

static uint64_t Get64(X86Decoder* d) {
  if (!Need(d, 8)) {
    d->pos = d->length;
    return 0;
  }
  uint64_t v = DAsmRead64LE(d->bytes + d->pos);
  d->pos += 8;
  return v;
}

static void ParsePrefixes(X86Decoder* d) {
  while (Need(d, 1)) {
    unsigned char b = d->bytes[d->pos];
    if (b == 0x66) {
      d->prefix66 = b;
    } else if (b == 0xf2) {
      d->prefixf2 = b;
    } else if (b == 0xf3) {
      d->prefixf3 = b;
    } else if (b >= 0x40 && b <= 0x4f) {
      d->rex = b;
    } else {
      break;
    }
    d->pos++;
  }
}

static int RexW(X86Decoder* d) { return (d->rex >> 3) & 1; }
static int RexR(X86Decoder* d) { return (d->rex >> 2) & 1; }
static int RexX(X86Decoder* d) { return (d->rex >> 1) & 1; }
static int RexB(X86Decoder* d) { return d->rex & 1; }

static int OperandWidth(X86Decoder* d) { return RexW(d) ? 8 : (d->prefix66 ? 2 : 4); }

static int32_t Sign8(unsigned char b) { return (int8_t)b; }

static void FormatMem(char* out, size_t out_size, X86Decoder* d, int mod, int rm) {
  int base = rm | (RexB(d) << 3);
  int index = -1;
  int scale = 1;
  int32_t disp = 0;
  bool has_base = true;
  bool rip = false;

  if (mod != 3 && (rm & 7) == 4) {
    unsigned char sib = Get8(d);
    scale = 1 << ((sib >> 6) & 3);
    index = ((sib >> 3) & 7) | (RexX(d) << 3);
    base = (sib & 7) | (RexB(d) << 3);
    if ((sib & 7) == 5 && mod == 0) {
      has_base = false;
      disp = (int32_t)Get32(d);
    }
    if ((sib & 7) == 4 && !RexX(d)) {
      index = -1;
    }
  } else if (mod == 0 && (rm & 7) == 5) {
    rip = true;
    disp = (int32_t)Get32(d);
  }
  if (mod == 1) {
    disp = Sign8(Get8(d));
  } else if (mod == 2) {
    disp = (int32_t)Get32(d);
  }

  size_t off = 0;
  off += snprintf(out + off, out_size - off, "[");
  if (rip) {
    off += snprintf(out + off, off < out_size ? out_size - off : 0,
                    "rip%s%" PRId32, disp >= 0 ? "+" : "", disp);
  } else {
    if (has_base) {
      off += snprintf(out + off, off < out_size ? out_size - off : 0, "%s",
                      RegName(base, 8));
    }
    if (index >= 0) {
      off += snprintf(out + off, off < out_size ? out_size - off : 0, "%s%s",
                      has_base ? "+" : "", RegName(index, 8));
      if (scale != 1) {
        off += snprintf(out + off, off < out_size ? out_size - off : 0, "*%d",
                        scale);
      }
    }
    if (disp != 0 || (!has_base && index < 0)) {
      off += snprintf(out + off, off < out_size ? out_size - off : 0,
                      "%s%" PRId32, disp >= 0 && (has_base || index >= 0) ? "+" : "",
                      disp);
    }
  }
  snprintf(out + off, off < out_size ? out_size - off : 0, "]");
}

static void DecodeRM(char* out, size_t size, X86Decoder* d, unsigned char modrm,
                     int width) {
  int mod = (modrm >> 6) & 3;
  int rm = modrm & 7;
  if (mod == 3) {
    snprintf(out, size, "%s", RegName(rm | (RexB(d) << 3), width));
  } else {
    FormatMem(out, size, d, mod, rm);
  }
}

static const char* JccName(int cc) {
  static const char* names[] = {"jo",  "jno", "jb",  "jae", "je",  "jne",
                                "jbe", "ja",  "js",  "jns", "jp",  "jnp",
                                "jl",  "jge", "jle", "jg"};
  return names[cc & 15];
}

static bool FormatBinaryModRM(X86Decoder* d, DAsmInstruction* out,
                              const char* op, bool reg_is_dst, int width) {
  if (!Need(d, 1)) return false;
  unsigned char modrm = Get8(d);
  int reg = ((modrm >> 3) & 7) | (RexR(d) << 3);
  char rm_text[80];
  DecodeRM(rm_text, sizeof(rm_text), d, modrm, width);
  if (reg_is_dst) {
    DAsmFormat(out, "%s %s, %s", op, RegName(reg, width), rm_text);
  } else {
    DAsmFormat(out, "%s %s, %s", op, rm_text, RegName(reg, width));
  }
  return true;
}

bool DAsmDisassembleX86_64(const void* bytes, size_t length, uint64_t address,
                           DAsmInstruction* out) {
  if (length == 0) {
    return false;
  }
  X86Decoder d = {
      .bytes = bytes,
      .length = length,
      .address = address,
  };
  ParsePrefixes(&d);
  if (!Need(&d, 1)) return false;
  unsigned char op = Get8(&d);
  const char* text = NULL;
  int width = OperandWidth(&d);

  if (op >= 0x50 && op <= 0x57) {
    DAsmInitInstruction(out, bytes, length, address, d.pos);
    DAsmFormat(out, "push %s", RegName((op - 0x50) | (RexB(&d) << 3), 8));
    return true;
  }
  if (op >= 0x58 && op <= 0x5f) {
    DAsmInitInstruction(out, bytes, length, address, d.pos);
    DAsmFormat(out, "pop %s", RegName((op - 0x58) | (RexB(&d) << 3), 8));
    return true;
  }
  if (op >= 0xb8 && op <= 0xbf) {
    int reg = (op - 0xb8) | (RexB(&d) << 3);
    uint64_t imm = RexW(&d) ? Get64(&d) : (d.prefix66 ? Get16(&d) : Get32(&d));
    DAsmInitInstruction(out, bytes, length, address, d.pos);
    DAsmFormat(out, "mov %s, 0x%" PRIx64, RegName(reg, width), imm);
    return true;
  }
  if (op >= 0xb0 && op <= 0xb7) {
    int reg = (op - 0xb0) | (RexB(&d) << 3);
    unsigned char imm = Get8(&d);
    DAsmInitInstruction(out, bytes, length, address, d.pos);
    DAsmFormat(out, "mov %s, 0x%02x", RegName(reg, 1), imm);
    return true;
  }

  switch (op) {
    case 0x90:
      text = "nop";
      break;
    case 0xc3:
      text = "ret";
      break;
    case 0xc9:
      text = "leave";
      break;
    case 0xc2: {
      uint32_t imm = Get8(&d);
      imm |= (uint32_t)Get8(&d) << 8;
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "ret %u", imm);
      return true;
    }
    case 0xc8: {
      uint32_t frame = Get8(&d);
      frame |= (uint32_t)Get8(&d) << 8;
      unsigned char nesting = Get8(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "enter %u, %u", frame, nesting);
      return true;
    }
    case 0xcc:
      text = "int3";
      break;
    case 0x9c:
      text = "pushf";
      break;
    case 0x9d:
      text = "popf";
      break;
    case 0x99:
      text = RexW(&d) ? "cqo" : "cdq";
      break;
    case 0x98:
      text = RexW(&d) ? "cltq" : "cwde";
      break;
    case 0xfa:
      text = "cli";
      break;
    case 0xe8: {
      int32_t rel = (int32_t)Get32(&d);
      uint64_t target = address + d.pos + rel;
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "call 0x%" PRIx64, target);
      DAsmSetTarget(out, target);
      return true;
    }
    case 0xe9: {
      int32_t rel = (int32_t)Get32(&d);
      uint64_t target = address + d.pos + rel;
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "jmp 0x%" PRIx64, target);
      DAsmSetTarget(out, target);
      return true;
    }
    case 0xeb: {
      int32_t rel = Sign8(Get8(&d));
      uint64_t target = address + d.pos + rel;
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "jmp 0x%" PRIx64, target);
      DAsmSetTarget(out, target);
      return true;
    }
    case 0x01:
      text = "add";
      break;
    case 0x00:
    case 0x02:
      text = "add";
      break;
    case 0x04: {
      unsigned char imm = Get8(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "add al, 0x%02x", imm);
      return true;
    }
    case 0x09:
      text = "or";
      break;
    case 0x08:
    case 0x0a:
      text = "or";
      break;
    case 0x21:
      text = "and";
      break;
    case 0x20:
    case 0x22:
      text = "and";
      break;
    case 0x29:
      text = "sub";
      break;
    case 0x28:
    case 0x2a:
      text = "sub";
      break;
    case 0x31:
      text = "xor";
      break;
    case 0x30:
    case 0x32:
      text = "xor";
      break;
    case 0x39:
      text = "cmp";
      break;
    case 0x38:
    case 0x3a:
      text = "cmp";
      break;
    case 0x89:
      text = "mov";
      break;
    case 0x8b:
      text = "mov";
      break;
    case 0x8d:
      text = "lea";
      break;
    case 0x85:
      text = "test";
      break;
    case 0x84:
      text = "test";
      break;
    case 0x88:
      text = "mov";
      break;
    case 0x8a:
      text = "mov";
      break;
    case 0x03:
      text = "add";
      break;
    case 0x0b:
      text = "or";
      break;
    case 0x23:
      text = "and";
      break;
    case 0x2b:
      text = "sub";
      break;
    case 0x33:
      text = "xor";
      break;
    case 0x3b:
      text = "cmp";
      break;
    case 0xc7: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, width);
      uint32_t imm = d.prefix66 ? Get16(&d) : Get32(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "mov %s, 0x%" PRIx32, rm_text, imm);
      return true;
    }
    case 0xc6: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 1);
      unsigned char imm = Get8(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "mov %s, 0x%02x", rm_text, imm);
      return true;
    }
    case 0x63: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int reg = ((modrm >> 3) & 7) | (RexR(&d) << 3);
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 4);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "movsxd %s, %s", RegName(reg, 8), rm_text);
      return true;
    }
    case 0x69:
    case 0x6b: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int reg = ((modrm >> 3) & 7) | (RexR(&d) << 3);
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, width);
      int64_t imm = op == 0x6b ? Sign8(Get8(&d)) : (int32_t)Get32(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "imul %s, %s, %" PRId64, RegName(reg, width), rm_text,
                 imm);
      return true;
    }
    case 0x81:
    case 0x83: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int group = (modrm >> 3) & 7;
      static const char* names[] = {"add", "or", "adc", "sbb",
                                    "and", "sub", "xor", "cmp"};
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, width);
      int64_t imm = op == 0x83 ? Sign8(Get8(&d))
                    : d.prefix66 ? (int16_t)Get16(&d)
                                  : (int32_t)Get32(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "%s %s, %" PRId64, names[group], rm_text, imm);
      return true;
    }
    case 0xff: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int group = (modrm >> 3) & 7;
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 8);
      const char* names[] = {"inc", "dec", "call", "call", "jmp", "jmp",
                             "push", NULL};
      if (names[group] != NULL) {
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        DAsmFormat(out, "%s %s", names[group], rm_text);
        return true;
      }
      break;
    }
    case 0xf7: {
      static const char* names[] = {"test", NULL, "not", "neg",
                                    "mul", "imul", "div", "idiv"};
      size_t before = d.pos;
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int group = (modrm >> 3) & 7;
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, op == 0xc0 ? 1 : width);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      if (group == 0) {
        int32_t imm = (int32_t)Get32(&d);
        out->size = d.pos;
        out->num_bytes = d.pos < length ? d.pos : length;
        memcpy(out->bytes, bytes, out->num_bytes);
        DAsmFormat(out, "test %s, %" PRId32, rm_text, imm);
      } else if (names[group] != NULL) {
        DAsmFormat(out, "%s %s", names[group], rm_text);
      } else {
        d.pos = before;
        break;
      }
      return true;
    }
    case 0xf6: {
      static const char* names[] = {"test", NULL, "not", "neg",
                                    "mul", "imul", "div", "idiv"};
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int group = (modrm >> 3) & 7;
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 1);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      if (group == 0) {
        unsigned char imm = Get8(&d);
        out->size = d.pos;
        out->num_bytes = d.pos < length ? d.pos : length;
        memcpy(out->bytes, bytes, out->num_bytes);
        DAsmFormat(out, "test %s, 0x%02x", rm_text, imm);
      } else if (names[group] != NULL) {
        DAsmFormat(out, "%s %s", names[group], rm_text);
      } else {
        break;
      }
      return true;
    }
    case 0x80:
    case 0x82: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int group = (modrm >> 3) & 7;
      static const char* names[] = {"add", "or", "adc", "sbb",
                                    "and", "sub", "xor", "cmp"};
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 1);
      unsigned char imm = Get8(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "%s %s, 0x%02x", names[group], rm_text, imm);
      return true;
    }
    case 0xc1:
    case 0xc0:
    case 0xd3: {
      static const char* names[] = {"rol", "ror", "rcl", "rcr",
                                    "shl", "shr", NULL, "sar"};
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int group = (modrm >> 3) & 7;
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, width);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      if (names[group] == NULL) break;
      if (op == 0xc1 || op == 0xc0) {
        unsigned char imm = Get8(&d);
        out->size = d.pos;
        out->num_bytes = d.pos < length ? d.pos : length;
        memcpy(out->bytes, bytes, out->num_bytes);
        DAsmFormat(out, "%s %s, %u", names[group], rm_text, imm);
      } else {
        DAsmFormat(out, "%s %s, cl", names[group], rm_text);
      }
      return true;
    }
    case 0xd0:
    case 0xd2: {
      static const char* names[] = {"rol", "ror", "rcl", "rcr",
                                    "shl", "shr", NULL, "sar"};
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      int group = (modrm >> 3) & 7;
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 1);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      if (names[group] == NULL) break;
      DAsmFormat(out, "%s %s, %s", names[group], rm_text,
                 op == 0xd2 ? "cl" : "1");
      return true;
    }
    case 0x8f: {
      if (!Need(&d, 1)) return false;
      unsigned char modrm = Get8(&d);
      char rm_text[80];
      DecodeRM(rm_text, sizeof(rm_text), &d, modrm, width);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "pop %s", rm_text);
      return true;
    }
    case 0xa0: {
      uint64_t imm = Get64(&d);
      DAsmInitInstruction(out, bytes, length, address, d.pos);
      DAsmFormat(out, "mov al, [0x%" PRIx64 "]", imm);
      return true;
    }
    case 0x0f: {
      if (!Need(&d, 1)) return false;
      unsigned char op2 = Get8(&d);
      if (op2 == 0x05) {
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        DAsmFormat(out, "syscall");
        return true;
      }
      if (op2 >= 0x90 && op2 <= 0x9f) {
        if (!Need(&d, 1)) return false;
        unsigned char modrm = Get8(&d);
        char rm_text[80];
        DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 1);
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        DAsmFormat(out, "set%s %s", JccName(op2 & 15) + 1, rm_text);
        return true;
      }
      if (op2 >= 0x40 && op2 <= 0x4f) {
        if (!Need(&d, 1)) return false;
        unsigned char modrm = Get8(&d);
        int reg = ((modrm >> 3) & 7) | (RexR(&d) << 3);
        char rm_text[80];
        DecodeRM(rm_text, sizeof(rm_text), &d, modrm, width);
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        DAsmFormat(out, "cmov%s %s, %s", JccName(op2 & 15) + 1,
                   RegName(reg, width), rm_text);
        return true;
      }
      if (op2 >= 0x80 && op2 <= 0x8f) {
        int32_t rel = (int32_t)Get32(&d);
        uint64_t target = address + d.pos + rel;
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        DAsmFormat(out, "%s 0x%" PRIx64, JccName(op2 & 15), target);
        DAsmSetTarget(out, target);
        return true;
      }
      if (op2 == 0xaf || op2 == 0xb6 || op2 == 0xb7 || op2 == 0xbe ||
          op2 == 0xbf) {
        if (!Need(&d, 1)) return false;
        unsigned char modrm = Get8(&d);
        int reg = ((modrm >> 3) & 7) | (RexR(&d) << 3);
        char rm_text[80];
        DecodeRM(rm_text, sizeof(rm_text), &d, modrm,
                 (op2 == 0xb6 || op2 == 0xbe) ? 1 : 2);
        const char* opname = op2 == 0xaf ? "imul"
                             : (op2 == 0xb6 || op2 == 0xb7) ? "movzx"
                                                            : "movsx";
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        DAsmFormat(out, "%s %s, %s", opname, RegName(reg, width), rm_text);
        return true;
      }
      if (op2 == 0x10 || op2 == 0x11 || op2 == 0x2a || op2 == 0x2c || op2 == 0x2e ||
          op2 == 0x51 || op2 == 0x57 || op2 == 0x58 || op2 == 0x59 ||
          op2 == 0x5a || op2 == 0x5c || op2 == 0x5e || op2 == 0x6e ||
          op2 == 0x7e) {
        if (!Need(&d, 1)) return false;
        unsigned char modrm = Get8(&d);
        int reg = ((modrm >> 3) & 7) | (RexR(&d) << 3);
        char rm_text[80];
        DecodeRM(rm_text, sizeof(rm_text), &d, modrm, 8);
        const char* scalar = d.prefixf2 ? "sd" : (d.prefixf3 ? "ss" : "");
        if (op2 == 0x2e) {
          scalar = d.prefix66 ? "sd" : "ss";
        }
        const char* base = op2 == 0x10 ? "mov"
                           : op2 == 0x11 ? "mov"
                           : op2 == 0x2a ? "cvtsi2"
                           : op2 == 0x2c ? "cvtt"
                           : op2 == 0x2e ? "ucomi"
                           : op2 == 0x51 ? "sqrt"
                           : op2 == 0x57 ? "xor"
                           : op2 == 0x58 ? "add"
                           : op2 == 0x59 ? "mul"
                           : op2 == 0x5a ? "cvt"
                           : op2 == 0x5c ? "sub"
                           : op2 == 0x6e ? "movd"
                           : op2 == 0x7e ? "movd"
                                         : "div";
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        if (op2 == 0x2a) {
          DAsmFormat(out, "%s%s %s, %s", base, scalar, XMMName(reg), rm_text);
        } else if (op2 == 0x2e) {
          DAsmFormat(out, "%s%s %s, %s", base, scalar, XMMName(reg), rm_text);
        } else if (op2 == 0x2c) {
          DAsmFormat(out, "%s%s2si %s, %s", base, scalar, RegName(reg, width),
                     rm_text);
        } else if (op2 == 0x5a) {
          const char* dst_scalar = d.prefixf2 ? "ss" : "sd";
          DAsmFormat(out, "%s%s2%s %s, %s", base, scalar, dst_scalar,
                     XMMName(reg), rm_text);
        } else if (op2 == 0x6e) {
          DAsmFormat(out, "%s %s, %s", RexW(&d) ? "movq" : "movd",
                     XMMName(reg), rm_text);
        } else if (op2 == 0x7e) {
          if (d.prefix66 && RexW(&d)) {
            DAsmFormat(out, "movq %s, %s", rm_text, XMMName(reg));
          } else if (d.prefixf3) {
            DAsmFormat(out, "movq %s, %s", op2 == 0x11 ? rm_text : XMMName(reg),
                       op2 == 0x11 ? XMMName(reg) : rm_text);
          } else {
            DAsmFormat(out, "movd %s, %s", rm_text, XMMName(reg));
          }
        } else if (op2 == 0x11) {
          DAsmFormat(out, "%s%s %s, %s", base, scalar, rm_text, XMMName(reg));
        } else {
          DAsmFormat(out, "%s%s %s, %s", base, scalar, XMMName(reg), rm_text);
        }
        return true;
      }
      break;
    }
    default:
      if (op >= 0x70 && op <= 0x7f) {
        int32_t rel = Sign8(Get8(&d));
        uint64_t target = address + d.pos + rel;
        DAsmInitInstruction(out, bytes, length, address, d.pos);
        DAsmFormat(out, "%s 0x%" PRIx64, JccName(op & 15), target);
        DAsmSetTarget(out, target);
        return true;
      }
      break;
  }

  if (text != NULL) {
    DAsmInitInstruction(out, bytes, length, address, d.pos);
    if (op == 0x8b || op == 0x8d || op == 0x03 || op == 0x0b ||
        op == 0x23 || op == 0x2b || op == 0x33 || op == 0x3b) {
      FormatBinaryModRM(&d, out, text, true, width);
      out->size = d.pos;
      out->num_bytes = d.pos < length ? d.pos : length;
      memcpy(out->bytes, bytes, out->num_bytes);
    } else if (op == 0x00 || op == 0x01 || op == 0x08 || op == 0x09 ||
               op == 0x20 || op == 0x21 || op == 0x28 || op == 0x29 ||
               op == 0x30 || op == 0x31 || op == 0x38 || op == 0x39 ||
               op == 0x88 || op == 0x89 || op == 0x84 || op == 0x85) {
      int op_width = (op == 0x00 || op == 0x08 || op == 0x20 || op == 0x28 ||
                      op == 0x30 || op == 0x38 || op == 0x88 || op == 0x84)
                         ? 1
                         : width;
      FormatBinaryModRM(&d, out, text, false, op_width);
      out->size = d.pos;
      out->num_bytes = d.pos < length ? d.pos : length;
      memcpy(out->bytes, bytes, out->num_bytes);
    } else if (op == 0x02 || op == 0x0a || op == 0x22 || op == 0x2a ||
               op == 0x32 || op == 0x3a || op == 0x8a) {
      FormatBinaryModRM(&d, out, text, true, 1);
      out->size = d.pos;
      out->num_bytes = d.pos < length ? d.pos : length;
      memcpy(out->bytes, bytes, out->num_bytes);
    } else {
      DAsmFormat(out, "%s", text);
    }
    return true;
  }

  DAsmInitInstruction(out, bytes, length, address, d.pos == 0 ? 1 : d.pos);
  DAsmUnknownInstruction(out, ".byte 0x%02" PRIx64, op);
  return true;
}
