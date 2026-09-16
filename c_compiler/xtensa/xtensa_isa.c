//
//  xtensa_isa.c
//  c_compiler
//

#include "xtensa_isa.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t Read24(const uint8_t* bytes) {
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16);
}

static void Write24(uint8_t* bytes, uint32_t word) {
  bytes[0] = (uint8_t)word;
  bytes[1] = (uint8_t)(word >> 8);
  bytes[2] = (uint8_t)(word >> 16);
}

static int32_t SignExtend(uint32_t value, unsigned bits) {
  uint32_t sign = 1u << (bits - 1);
  return (int32_t)((value ^ sign) - sign);
}

static bool FitsSigned(int32_t value, unsigned bits) {
  int32_t minimum = -(1 << (bits - 1));
  int32_t maximum = (1 << (bits - 1)) - 1;
  return value >= minimum && value <= maximum;
}

static bool ValidRegister(unsigned reg) { return reg < 16; }

static uint32_t RRR(unsigned op2, unsigned op1, unsigned r, unsigned s,
                    unsigned t) {
  return (op2 << 20) | (op1 << 16) | (r << 12) | (s << 8) | (t << 4);
}

static uint32_t RRI8(unsigned op0, unsigned r, unsigned s, unsigned t,
                     uint8_t immediate) {
  return ((uint32_t)immediate << 16) | (r << 12) | (s << 8) | (t << 4) | op0;
}

static bool EncodeRRR(const XtensaInstruction* inst, unsigned op2,
                      uint8_t out[3]) {
  if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs) ||
      !ValidRegister(inst->rt)) {
    return false;
  }
  Write24(out, RRR(op2, 0, inst->rd, inst->rs, inst->rt));
  return true;
}

static bool EncodeMemory(const XtensaInstruction* inst, unsigned r,
                         unsigned scale, uint8_t out[3]) {
  if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs) ||
      inst->immediate < 0 || (inst->immediate % (int32_t)scale) != 0) {
    return false;
  }
  uint32_t encoded = (uint32_t)inst->immediate / scale;
  if (encoded > 255) {
    return false;
  }
  Write24(out, RRI8(2, r, inst->rs, inst->rd, (uint8_t)encoded));
  return true;
}

static bool EncodeBranch8(const XtensaInstruction* inst, unsigned r,
                          uint8_t out[3]) {
  if (!ValidRegister(inst->rs) || !ValidRegister(inst->rt) ||
      !FitsSigned(inst->immediate, 8)) {
    return false;
  }
  Write24(out, RRI8(7, r, inst->rs, inst->rt, (uint8_t)inst->immediate));
  return true;
}

static bool EncodeBranch12(const XtensaInstruction* inst, unsigned op,
                           uint8_t out[3]) {
  if (!ValidRegister(inst->rs) || !FitsSigned(inst->immediate, 12)) {
    return false;
  }
  uint32_t word = ((uint32_t)inst->immediate & 0xfff) << 12;
  word |= (uint32_t)inst->rs << 8;
  word |= op << 4;
  word |= 6;
  Write24(out, word);
  return true;
}

bool XtensaEncode(const XtensaInstruction* inst, uint8_t out[3]) {
  if (inst == NULL || out == NULL) {
    return false;
  }
  switch (inst->kind) {
    case kXtensaAdd:
      return EncodeRRR(inst, 8, out);
    case kXtensaSub:
      return EncodeRRR(inst, 12, out);
    case kXtensaAnd:
      return EncodeRRR(inst, 1, out);
    case kXtensaOr:
      return EncodeRRR(inst, 2, out);
    case kXtensaXor:
      return EncodeRRR(inst, 3, out);
    case kXtensaNeg:
      if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs)) {
        return false;
      }
      Write24(out, RRR(6, 0, inst->rd, 0, inst->rs));
      return true;
    case kXtensaAddi:
      if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs) ||
          !FitsSigned(inst->immediate, 8)) {
        return false;
      }
      Write24(out, RRI8(2, 12, inst->rd, inst->rs, (uint8_t)inst->immediate));
      return true;
    case kXtensaMovi:
      if (!ValidRegister(inst->rd) || !FitsSigned(inst->immediate, 12)) {
        return false;
      }
      Write24(out, RRI8(2, 10, (inst->immediate >> 8) & 0xf, inst->rd,
                        (uint8_t)inst->immediate));
      return true;
    case kXtensaL8ui:
      return EncodeMemory(inst, 0, 1, out);
    case kXtensaL16ui:
      return EncodeMemory(inst, 1, 2, out);
    case kXtensaL16si:
      return EncodeMemory(inst, 9, 2, out);
    case kXtensaL32i:
      return EncodeMemory(inst, 2, 4, out);
    case kXtensaS8i:
      return EncodeMemory(inst, 4, 1, out);
    case kXtensaS16i:
      return EncodeMemory(inst, 5, 2, out);
    case kXtensaS32i:
      return EncodeMemory(inst, 6, 4, out);
    case kXtensaBeq:
      return EncodeBranch8(inst, 1, out);
    case kXtensaBne:
      return EncodeBranch8(inst, 9, out);
    case kXtensaBlt:
      return EncodeBranch8(inst, 2, out);
    case kXtensaBge:
      return EncodeBranch8(inst, 10, out);
    case kXtensaBltu:
      return EncodeBranch8(inst, 3, out);
    case kXtensaBgeu:
      return EncodeBranch8(inst, 11, out);
    case kXtensaBeqz:
      return EncodeBranch12(inst, 1, out);
    case kXtensaBnez:
      return EncodeBranch12(inst, 5, out);
    case kXtensaBltz:
      return EncodeBranch12(inst, 9, out);
    case kXtensaBgez:
      return EncodeBranch12(inst, 13, out);
    case kXtensaL32r: {
      if (!ValidRegister(inst->rd) || inst->immediate >= 0 ||
          inst->immediate < -262144 || (inst->immediate & 3) != 0) {
        return false;
      }
      uint32_t word = (((uint32_t)(inst->immediate >> 2) & 0xffff) << 8) |
                      ((uint32_t)inst->rd << 4) | 1;
      Write24(out, word);
      return true;
    }
    case kXtensaJ:
      if (!FitsSigned(inst->immediate, 18)) {
        return false;
      }
      Write24(out, ((uint32_t)inst->immediate & 0x3ffff) << 6 | 6);
      return true;
    case kXtensaJx:
      if (!ValidRegister(inst->rs)) {
        return false;
      }
      Write24(out, ((uint32_t)inst->rs << 8) | 0xa0);
      return true;
    case kXtensaCall8:
      if ((inst->immediate & 3) != 0 || !FitsSigned(inst->immediate >> 2, 18)) {
        return false;
      }
      Write24(out, (((uint32_t)(inst->immediate >> 2) & 0x3ffff) << 6) | 0x25);
      return true;
    case kXtensaCallx8:
      if (!ValidRegister(inst->rs)) {
        return false;
      }
      Write24(out, ((uint32_t)inst->rs << 8) | 0xe0);
      return true;
    case kXtensaEntry:
      if (!ValidRegister(inst->rs) || inst->rs > 3 || inst->immediate < 0 ||
          (inst->immediate & 7) != 0 || (uint32_t)inst->immediate / 8 > 0xfff) {
        return false;
      }
      Write24(out, ((uint32_t)(inst->immediate / 8) << 12) |
                       ((uint32_t)inst->rs << 8) | 0x36);
      return true;
    case kXtensaRetw:
      Write24(out, 0x90);
      return true;
    case kXtensaBreak:
      if ((uint32_t)inst->immediate > 0xff) {
        return false;
      }
      Write24(out, 0x004000 | ((uint32_t)inst->immediate << 4));
      return true;
    case kXtensaSlli:
      if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs) ||
          inst->immediate < 0 || inst->immediate > 31) {
        return false;
      }
      Write24(out, ((uint32_t)(inst->immediate >> 4) << 20) | (1u << 16) |
                       ((uint32_t)inst->rd << 12) | ((uint32_t)inst->rs << 8) |
                       ((uint32_t)inst->immediate & 0xf) << 4);
      return true;
    case kXtensaSrai:
      if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs) ||
          inst->immediate < 0 || inst->immediate > 31) {
        return false;
      }
      Write24(out, ((uint32_t)(2 | (inst->immediate >> 4)) << 20) | (1u << 16) |
                       ((uint32_t)inst->rd << 12) |
                       (((uint32_t)inst->immediate & 0xf) << 8) |
                       ((uint32_t)inst->rs << 4));
      return true;
    case kXtensaSrli:
      if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs) ||
          inst->immediate < 0 || inst->immediate > 15) {
        return false;
      }
      Write24(out, (4u << 20) | (1u << 16) | ((uint32_t)inst->rd << 12) |
                       ((uint32_t)inst->immediate << 8) |
                       ((uint32_t)inst->rs << 4));
      return true;
    case kXtensaSsl:
    case kXtensaSsr:
      if (!ValidRegister(inst->rs)) {
        return false;
      }
      Write24(out, RRR(4, 0, inst->kind == kXtensaSsl ? 1 : 0, inst->rs, 0));
      return true;
    case kXtensaSll:
    case kXtensaSrl:
    case kXtensaSra:
      if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs)) {
        return false;
      }
      if (inst->kind == kXtensaSll) {
        Write24(out, RRR(10, 1, inst->rd, inst->rs, 0));
      } else {
        Write24(out, RRR(inst->kind == kXtensaSrl ? 9 : 11, 1, inst->rd, 0,
                         inst->rs));
      }
      return true;
    case kXtensaMull:
    case kXtensaQuou:
    case kXtensaQuos:
    case kXtensaRemu:
    case kXtensaRems: {
      unsigned op2 =
          inst->kind == kXtensaMull
              ? 8
              : (inst->kind == kXtensaQuou
                     ? 12
                     : (inst->kind == kXtensaQuos
                            ? 13
                            : (inst->kind == kXtensaRemu ? 14 : 15)));
      if (!ValidRegister(inst->rd) || !ValidRegister(inst->rs) ||
          !ValidRegister(inst->rt)) {
        return false;
      }
      Write24(out, RRR(op2, 2, inst->rd, inst->rs, inst->rt));
      return true;
    }
    case kXtensaInvalid:
      return false;
  }
  return false;
}

static void ClearInstruction(XtensaInstruction* inst) {
  memset(inst, 0, sizeof(*inst));
  inst->kind = kXtensaInvalid;
  inst->size = 3;
}

bool XtensaDecode(const uint8_t* bytes, size_t available,
                  XtensaInstruction* inst) {
  if (bytes == NULL || inst == NULL || available < 2) {
    return false;
  }
  ClearInstruction(inst);
  unsigned op0 = bytes[0] & 0xf;
  if (op0 >= 8) {
    inst->size = 2;
    return false;
  }
  if (available < 3) {
    return false;
  }
  uint32_t word = Read24(bytes);
  unsigned t = (word >> 4) & 0xf;
  unsigned s = (word >> 8) & 0xf;
  unsigned r = (word >> 12) & 0xf;
  unsigned op1 = (word >> 16) & 0xf;
  unsigned op2 = (word >> 20) & 0xf;

  if (op0 == 0) {
    if ((word & 0xff) == 0xe0 && r == 0 && op1 == 0 && op2 == 0) {
      inst->kind = kXtensaCallx8;
      inst->rs = (uint8_t)s;
      return true;
    }
    if (word == 0x90) {
      inst->kind = kXtensaRetw;
      return true;
    }
    if ((word & 0xff) == 0xa0 && r == 0 && op1 == 0 && op2 == 0) {
      inst->kind = kXtensaJx;
      inst->rs = (uint8_t)s;
      return true;
    }
    if (r == 4 && op1 == 0 && op2 == 0) {
      inst->kind = kXtensaBreak;
      inst->immediate = (int32_t)((s << 4) | t);
      return true;
    }
    if (op1 == 1 && (op2 == 0 || op2 == 1)) {
      inst->kind = kXtensaSlli;
      inst->rd = (uint8_t)r;
      inst->rs = (uint8_t)s;
      inst->immediate = (int32_t)((op2 << 4) | t);
      return true;
    }
    if (op1 == 1 && (op2 == 2 || op2 == 3)) {
      inst->kind = kXtensaSrai;
      inst->rd = (uint8_t)r;
      inst->rs = (uint8_t)t;
      inst->immediate = (int32_t)(((op2 & 1) << 4) | s);
      return true;
    }
    if (op1 == 1 && op2 == 4) {
      inst->kind = kXtensaSrli;
      inst->rd = (uint8_t)r;
      inst->rs = (uint8_t)t;
      inst->immediate = (int32_t)s;
      return true;
    }
    if (op1 == 1 && (op2 == 9 || op2 == 10 || op2 == 11)) {
      inst->kind =
          op2 == 9 ? kXtensaSrl : (op2 == 10 ? kXtensaSll : kXtensaSra);
      inst->rd = (uint8_t)r;
      inst->rs = (uint8_t)(op2 == 10 ? s : t);
      return true;
    }
    if (op1 == 2 && (op2 == 8 || op2 >= 12)) {
      inst->kind =
          op2 == 8
              ? kXtensaMull
              : (op2 == 12
                     ? kXtensaQuou
                     : (op2 == 13 ? kXtensaQuos
                                  : (op2 == 14 ? kXtensaRemu : kXtensaRems)));
      inst->rd = (uint8_t)r;
      inst->rs = (uint8_t)s;
      inst->rt = (uint8_t)t;
      return true;
    }
    if (op1 != 0) {
      return false;
    }
    if (op2 == 4 && t == 0 && (r == 0 || r == 1)) {
      inst->kind = r == 1 ? kXtensaSsl : kXtensaSsr;
      inst->rs = (uint8_t)s;
      inst->rd = 0;
      return true;
    }
    inst->rd = (uint8_t)r;
    inst->rs = (uint8_t)s;
    inst->rt = (uint8_t)t;
    switch (op2) {
      case 1:
        inst->kind = kXtensaAnd;
        return true;
      case 2:
        inst->kind = kXtensaOr;
        return true;
      case 3:
        inst->kind = kXtensaXor;
        return true;
      case 6:
        if (s == 0) {
          inst->kind = kXtensaNeg;
          inst->rs = (uint8_t)t;
          inst->rt = 0;
          return true;
        }
        return false;
      case 8:
        inst->kind = kXtensaAdd;
        return true;
      case 12:
        inst->kind = kXtensaSub;
        return true;
      default:
        return false;
    }
  }

  if (op0 == 1) {
    inst->kind = kXtensaL32r;
    inst->rd = (uint8_t)t;
    inst->immediate = SignExtend(word >> 8, 16) << 2;
    return true;
  }

  if (op0 == 2) {
    uint8_t immediate = (uint8_t)(word >> 16);
    inst->rd = (uint8_t)t;
    inst->rs = (uint8_t)s;
    switch (r) {
      case 0:
        inst->kind = kXtensaL8ui;
        inst->immediate = immediate;
        return true;
      case 1:
        inst->kind = kXtensaL16ui;
        inst->immediate = (int32_t)immediate * 2;
        return true;
      case 2:
        inst->kind = kXtensaL32i;
        inst->immediate = (int32_t)immediate * 4;
        return true;
      case 4:
        inst->kind = kXtensaS8i;
        inst->immediate = immediate;
        return true;
      case 5:
        inst->kind = kXtensaS16i;
        inst->immediate = (int32_t)immediate * 2;
        return true;
      case 6:
        inst->kind = kXtensaS32i;
        inst->immediate = (int32_t)immediate * 4;
        return true;
      case 9:
        inst->kind = kXtensaL16si;
        inst->immediate = (int32_t)immediate * 2;
        return true;
      case 10:
        inst->kind = kXtensaMovi;
        inst->immediate = SignExtend((s << 8) | immediate, 12);
        inst->rs = 0;
        return true;
      case 12:
        inst->kind = kXtensaAddi;
        inst->rd = (uint8_t)s;
        inst->rs = (uint8_t)t;
        inst->immediate = (int8_t)immediate;
        return true;
      default:
        return false;
    }
  }

  if (op0 == 5 && (word & 0x30) == 0x20) {
    inst->kind = kXtensaCall8;
    inst->immediate = SignExtend(word >> 6, 18) << 2;
    return true;
  }

  if (op0 == 6) {
    unsigned branch_op = (word >> 4) & 0xf;
    if ((word & 0x30) == 0) {
      inst->kind = kXtensaJ;
      inst->immediate = SignExtend(word >> 6, 18);
      return true;
    }
    if (branch_op == 3) {
      inst->kind = kXtensaEntry;
      inst->rs = (word >> 8) & 0xf;
      inst->immediate = (int32_t)((word >> 12) & 0xfff) * 8;
      return true;
    }
    inst->rs = (word >> 8) & 0xf;
    inst->immediate = SignExtend(word >> 12, 12);
    switch (branch_op) {
      case 1:
        inst->kind = kXtensaBeqz;
        return true;
      case 5:
        inst->kind = kXtensaBnez;
        return true;
      case 9:
        inst->kind = kXtensaBltz;
        return true;
      case 13:
        inst->kind = kXtensaBgez;
        return true;
      default:
        return false;
    }
  }

  if (op0 == 7) {
    inst->rs = (word >> 8) & 0xf;
    inst->rt = (word >> 4) & 0xf;
    inst->immediate = (int8_t)(word >> 16);
    switch (r) {
      case 1:
        inst->kind = kXtensaBeq;
        return true;
      case 2:
        inst->kind = kXtensaBlt;
        return true;
      case 3:
        inst->kind = kXtensaBltu;
        return true;
      case 9:
        inst->kind = kXtensaBne;
        return true;
      case 10:
        inst->kind = kXtensaBge;
        return true;
      case 11:
        inst->kind = kXtensaBgeu;
        return true;
      default:
        return false;
    }
  }
  return false;
}

bool XtensaPatchSlot0(uint8_t bytes[3], uint32_t place, uint32_t target) {
  XtensaInstruction inst;
  if (!XtensaDecode(bytes, 3, &inst)) {
    return false;
  }
  switch (inst.kind) {
    case kXtensaCall8:
      inst.immediate = (int32_t)(target - ((place & ~3u) + 4));
      break;
    case kXtensaL32r:
      inst.immediate = (int32_t)(target - ((place + 3) & ~3u));
      break;
    case kXtensaJ:
    case kXtensaBeq:
    case kXtensaBne:
    case kXtensaBlt:
    case kXtensaBge:
    case kXtensaBltu:
    case kXtensaBgeu:
    case kXtensaBeqz:
    case kXtensaBnez:
    case kXtensaBltz:
    case kXtensaBgez:
      inst.immediate = (int32_t)(target - (place + 4));
      break;
    default:
      return false;
  }
  return XtensaEncode(&inst, bytes);
}

const char* XtensaInstructionName(XtensaInstructionKind kind) {
  static const char* names[] = {
      "invalid", "add",   "sub",   "and",   "or",   "xor",   "neg",    "addi",
      "movi",    "l8ui",  "l16ui", "l16si", "l32i", "s8i",   "s16i",   "s32i",
      "beq",     "bne",   "blt",   "bge",   "bltu", "bgeu",  "beqz",   "bnez",
      "bltz",    "bgez",  "l32r",  "j",     "jx",   "call8", "callx8", "entry",
      "retw",    "break", "slli",  "srai",  "srli", "ssl",   "ssr",    "sll",
      "srl",     "sra",   "mull",  "quou",  "quos", "remu",  "rems",
  };
  if ((unsigned)kind >= sizeof(names) / sizeof(names[0])) {
    return "invalid";
  }
  return names[kind];
}

const char* XtensaRegisterName(unsigned reg) {
  static char names[16][4];
  static bool initialized;
  if (reg >= 16) {
    return NULL;
  }
  if (!initialized) {
    for (unsigned i = 0; i < 16; i++) {
      snprintf(names[i], sizeof(names[i]), "a%u", i);
    }
    initialized = true;
  }
  return names[reg];
}

bool XtensaParseRegister(const char* text, unsigned* reg) {
  if (text == NULL || reg == NULL) {
    return false;
  }
  while (isspace((unsigned char)*text)) {
    text++;
  }
  if (strcmp(text, "sp") == 0) {
    *reg = 1;
    return true;
  }
  if (*text != 'a' && *text != 'A') {
    return false;
  }
  char* end;
  unsigned long value = strtoul(text + 1, &end, 10);
  if (*end != '\0' || value >= 16) {
    return false;
  }
  *reg = (unsigned)value;
  return true;
}
