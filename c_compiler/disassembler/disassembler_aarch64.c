//
//  disassembler_aarch64.c
//  c_compiler
//

#include "disassembler_internal.h"

#include <inttypes.h>
#include <stdio.h>

static const char* CondName(int cond) {
  static const char* names[] = {
      "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
      "hi", "ls", "ge", "lt", "gt", "le", "al", "nv",
  };
  return names[cond & 15];
}

static const char* InvertedCondName(int cond) {
  if ((cond & 14) == 14) {
    return CondName(cond);
  }
  return CondName(cond ^ 1);
}

static const char* XReg(int reg, bool width64, bool sp_ok) {
  static char bufs[4][8];
  static int index = 0;
  char* buf = bufs[index++ & 3];
  if (reg == 31) {
    snprintf(buf, 8, "%s", sp_ok ? "sp" : (width64 ? "xzr" : "wzr"));
  } else {
    snprintf(buf, 8, "%c%d", width64 ? 'x' : 'w', reg);
  }
  return buf;
}

static const char* VReg(int reg, int size) {
  static char bufs[4][8];
  static int index = 0;
  char* buf = bufs[index++ & 3];
  snprintf(buf, 8, "%c%d", size == 8 ? 'd' : 's', reg);
  return buf;
}

static int64_t DecodeAArch64Branch26(uint32_t inst) {
  return DAsmSignExtend(inst & 0x03ffffffu, 26) << 2;
}

static int64_t DecodeAArch64Branch19(uint32_t inst) {
  return DAsmSignExtend((inst >> 5) & 0x7ffffu, 19) << 2;
}

static int64_t DecodeAArch64ADR(uint32_t inst) {
  uint32_t immlo = (inst >> 29) & 0x3;
  uint32_t immhi = (inst >> 5) & 0x7ffff;
  return DAsmSignExtend((immhi << 2) | immlo, 21);
}

static const char* UnscaledIntegerLoadStoreName(int size, int opc,
                                                bool* width64) {
  *width64 = size == 3 || opc == 2;
  if (opc == 0) {
    static const char* stores[] = {"sturb", "sturh", "stur", "stur"};
    return stores[size];
  }
  if (opc == 1) {
    static const char* loads[] = {"ldurb", "ldurh", "ldur", "ldur"};
    return loads[size];
  }
  if (opc == 2) {
    static const char* loads[] = {"ldursb", "ldursh", "ldursw", NULL};
    return loads[size];
  }
  if (opc == 3 && size < 2) {
    *width64 = false;
    return size == 0 ? "ldursb" : "ldursh";
  }
  return NULL;
}

bool DAsmDisassembleAArch64(const void* bytes, size_t length, uint64_t address,
                            DAsmInstruction* out) {
  if (length < 4) {
    return false;
  }
  uint32_t inst = DAsmRead32LE(bytes);
  DAsmInitInstruction(out, bytes, length, address, 4);

  if ((inst & 0x7c000000u) == 0x14000000u) {
    bool link = (inst & 0x80000000u) != 0;
    uint64_t target = address + DecodeAArch64Branch26(inst);
    DAsmFormat(out, "%s 0x%" PRIx64, link ? "bl" : "b", target);
    DAsmSetTarget(out, target);
    return true;
  }
  if ((inst & 0x1f000000u) == 0x10000000u) {
    bool page = (inst >> 31) != 0;
    uint64_t base = page ? (address & ~0xfffULL) : address;
    int64_t offset = DecodeAArch64ADR(inst);
    if (page) {
      offset <<= 12;
    }
    uint64_t target = base + offset;
    DAsmFormat(out, "%s %s, 0x%" PRIx64, page ? "adrp" : "adr",
               XReg(inst & 0x1f, true, false), target);
    DAsmSetTarget(out, target);
    return true;
  }
  if ((inst & 0xff000010u) == 0x54000000u) {
    uint64_t target = address + DecodeAArch64Branch19(inst);
    DAsmFormat(out, "b.%s 0x%" PRIx64, CondName(inst & 0xf), target);
    DAsmSetTarget(out, target);
    return true;
  }
  if ((inst & 0x7e000000u) == 0x34000000u) {
    bool sf = (inst >> 31) != 0;
    bool nonzero = ((inst >> 24) & 1) != 0;
    int rt = inst & 0x1f;
    uint64_t target = address + DecodeAArch64Branch19(inst);
    DAsmFormat(out, "%s %s, 0x%" PRIx64, nonzero ? "cbnz" : "cbz",
               XReg(rt, sf, false), target);
    DAsmSetTarget(out, target);
    return true;
  }
  if ((inst & 0x7e000000u) == 0x36000000u) {
    bool nonzero = ((inst >> 24) & 1) != 0;
    int bit = ((inst >> 19) & 0x20) | ((inst >> 19) & 0x1f);
    int rt = inst & 0x1f;
    int64_t off = DAsmSignExtend((inst >> 5) & 0x3fff, 14) << 2;
    uint64_t target = address + off;
    DAsmFormat(out, "%s %s, #%d, 0x%" PRIx64, nonzero ? "tbnz" : "tbz",
               XReg(rt, bit >= 32, false), bit, target);
    DAsmSetTarget(out, target);
    return true;
  }
  if ((inst & 0xfffffc1fu) == 0xd65f0000u) {
    int rn = (inst >> 5) & 0x1f;
    if (rn == 30) {
      DAsmFormat(out, "ret");
    } else {
      DAsmFormat(out, "ret %s", XReg(rn, true, false));
    }
    return true;
  }
  if ((inst & 0xfffffc1fu) == 0xd61f0000u ||
      (inst & 0xfffffc1fu) == 0xd63f0000u) {
    DAsmFormat(out, "%s %s", (inst & 0x00200000u) ? "blr" : "br",
               XReg((inst >> 5) & 0x1f, true, false));
    return true;
  }
  if ((inst & 0x1f000000u) == 0x11000000u) {
    bool sf = (inst >> 31) != 0;
    bool sub = (inst >> 30) & 1;
    bool set_flags = (inst >> 29) & 1;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    int imm = (inst >> 10) & 0xfff;
    if (((inst >> 22) & 1) != 0) {
      imm <<= 12;
    }
    const char* op = sub ? (set_flags ? "subs" : "sub")
                         : (set_flags ? "adds" : "add");
    if (set_flags && rd == 31) {
      op = sub ? "cmp" : "cmn";
      DAsmFormat(out, "%s %s, #%d", op, XReg(rn, sf, true), imm);
    } else {
      DAsmFormat(out, "%s %s, %s, #%d", op, XReg(rd, sf, true),
                 XReg(rn, sf, true), imm);
    }
    return true;
  }
  if ((inst & 0x1f800000u) == 0x12800000u) {
    bool sf = (inst >> 31) != 0;
    int opc = (inst >> 29) & 3;
    int hw = (inst >> 21) & 3;
    int imm = (inst >> 5) & 0xffff;
    int rd = inst & 0x1f;
    const char* op = opc == 0 ? "movn" : (opc == 2 ? "movz" : "movk");
    DAsmFormat(out, "%s %s, #0x%x, lsl #%d", op, XReg(rd, sf, false), imm,
               hw * 16);
    return true;
  }
  if ((inst & 0x1f000000u) == 0x0b000000u) {
    bool sf = (inst >> 31) != 0;
    bool sub = (inst >> 30) & 1;
    bool set_flags = (inst >> 29) & 1;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    int rm = (inst >> 16) & 0x1f;
    int shift = (inst >> 22) & 3;
    int amount = (inst >> 10) & 0x3f;
    static const char* shifts[] = {"lsl", "lsr", "asr", "ror"};
    const char* op = sub ? (set_flags ? "subs" : "sub")
                         : (set_flags ? "adds" : "add");
    DAsmFormat(out, "%s %s, %s, %s, %s #%d", op, XReg(rd, sf, true),
               XReg(rn, sf, true), XReg(rm, sf, false), shifts[shift], amount);
    return true;
  }
  if ((inst & 0x7fe0fc00u) == 0x1a000000u ||
      (inst & 0x7fe0fc00u) == 0x3a000000u ||
      (inst & 0x7fe0fc00u) == 0x5a000000u ||
      (inst & 0x7fe0fc00u) == 0x7a000000u) {
    bool sf = (inst >> 31) != 0;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    int rm = (inst >> 16) & 0x1f;
    int op = (inst >> 29) & 3;
    static const char* names[] = {"adc", "adcs", "sbc", "sbcs"};
    DAsmFormat(out, "%s %s, %s, %s", names[op], XReg(rd, sf, false),
               XReg(rn, sf, false), XReg(rm, sf, false));
    return true;
  }
  if ((inst & 0x1f000000u) == 0x0a000000u) {
    bool sf = (inst >> 31) != 0;
    int opc = (inst >> 29) & 3;
    int n = (inst >> 21) & 1;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    int rm = (inst >> 16) & 0x1f;
    const char* base[] = {"and", "orr", "eor", "ands"};
    const char* op = base[opc];
    if (n && opc == 0) op = "bic";
    if (n && opc == 1) op = "orn";
    if (n && opc == 2) op = "eon";
    if (n && opc == 3) op = "bics";
    DAsmFormat(out, "%s %s, %s, %s", op, XReg(rd, sf, false),
               XReg(rn, sf, false), XReg(rm, sf, false));
    return true;
  }
  if ((inst & 0x1f800000u) == 0x13000000u) {
    bool sf = (inst >> 31) != 0;
    int opc = (inst >> 29) & 3;
    int immr = (inst >> 16) & 0x3f;
    int imms = (inst >> 10) & 0x3f;
    int rn = (inst >> 5) & 0x1f;
    int rd = inst & 0x1f;
    static const char* names[] = {"sbfm", "bfm", "ubfm", NULL};
    if (names[opc] != NULL) {
      DAsmFormat(out, "%s %s, %s, #%d, #%d", names[opc],
                 XReg(rd, sf, false), XReg(rn, sf, false), immr, imms);
      return true;
    }
  }
  if ((inst & 0x1f800000u) == 0x12000000u) {
    bool sf = (inst >> 31) != 0;
    int opc = (inst >> 29) & 3;
    int n = (inst >> 22) & 1;
    int immr = (inst >> 16) & 0x3f;
    int imms = (inst >> 10) & 0x3f;
    int rn = (inst >> 5) & 0x1f;
    int rd = inst & 0x1f;
    static const char* names[] = {"and", "orr", "eor", "ands"};
    DAsmFormat(out, "%s %s, %s, #<%d:%d:%d>", names[opc],
               XReg(rd, sf, false), XReg(rn, sf, false), n, immr, imms);
    return true;
  }
  if ((inst & 0x1fe00000u) == 0x1a800000u) {
    bool sf = (inst >> 31) != 0;
    int op = (inst >> 30) & 1;
    int rm = (inst >> 16) & 0x1f;
    int cond = (inst >> 12) & 0xf;
    int op2 = (inst >> 10) & 1;
    int rn = (inst >> 5) & 0x1f;
    int rd = inst & 0x1f;
    if (rn == 31 && rm == 31 && op == 0 && op2 == 1) {
      DAsmFormat(out, "cset %s, %s", XReg(rd, sf, false),
                 InvertedCondName(cond));
    } else if (rn == 31 && rm == 31 && op == 1 && op2 == 0) {
      DAsmFormat(out, "csetm %s, %s", XReg(rd, sf, false),
                 InvertedCondName(cond));
    } else {
      const char* name = op == 0 ? (op2 == 0 ? "csel" : "csinc")
                                 : (op2 == 0 ? "csinv" : "csneg");
      DAsmFormat(out, "%s %s, %s, %s, %s", name, XReg(rd, sf, false),
                 XReg(rn, sf, false), XReg(rm, sf, false), CondName(cond));
    }
    return true;
  }
  if ((inst & 0x3f000000u) == 0x29000000u) {
    int opc = (inst >> 30) & 3;
    bool load = ((inst >> 22) & 1) != 0;
    int imm7 = (int)DAsmSignExtend((inst >> 15) & 0x7f, 7);
    int rt2 = (inst >> 10) & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    int rt = inst & 0x1f;
    bool is_64 = opc == 2;
    bool reg_width_64 = is_64 || (load && opc == 1);
    int scale = is_64 ? 8 : 4;
    const char* op = load ? (opc == 1 ? "ldpsw" : "ldp") : "stp";
    if (opc != 3) {
      DAsmFormat(out, "%s %s, %s, [%s, #%d]", op,
                 XReg(rt, reg_width_64, false),
                 XReg(rt2, reg_width_64, false), XReg(rn, true, true),
                 imm7 * scale);
      return true;
    }
  }
  if ((inst & 0x3b200c00u) == 0x38000000u) {
    int size = (inst >> 30) & 3;
    bool fp = ((inst >> 26) & 1) != 0;
    int opc = (inst >> 22) & 3;
    int imm = (int)DAsmSignExtend((inst >> 12) & 0x1ff, 9);
    int rn = (inst >> 5) & 0x1f;
    int rt = inst & 0x1f;
    if (fp) {
      if (opc < 2 && size >= 2) {
        DAsmFormat(out, "%s %s, [%s, #%d]", opc == 1 ? "ldur" : "stur",
                   VReg(rt, size == 3 ? 8 : 4), XReg(rn, true, true), imm);
        return true;
      }
    } else {
      bool width64 = false;
      const char* name = UnscaledIntegerLoadStoreName(size, opc, &width64);
      if (name != NULL) {
        DAsmFormat(out, "%s %s, [%s, #%d]", name, XReg(rt, width64, false),
                   XReg(rn, true, true), imm);
        return true;
      }
    }
  }
  if ((inst & 0x7fe00000u) == 0x1ac00000u) {
    bool sf = (inst >> 31) != 0;
    int rm = (inst >> 16) & 0x1f;
    int op = (inst >> 10) & 0x3f;
    int rn = (inst >> 5) & 0x1f;
    int rd = inst & 0x1f;
    const char* name = NULL;
    if (op == 2) name = "udiv";
    if (op == 3) name = "sdiv";
    if (op == 8) name = "lslv";
    if (op == 9) name = "lsrv";
    if (op == 10) name = "asrv";
    if (name != NULL) {
      DAsmFormat(out, "%s %s, %s, %s", name, XReg(rd, sf, false),
                 XReg(rn, sf, false), XReg(rm, sf, false));
      return true;
    }
  }
  if ((inst & 0xffe0001fu) == 0xd4000001u) {
    DAsmFormat(out, "svc #%d", (inst >> 5) & 0xffff);
    return true;
  }
  if ((inst & 0x1f000000u) == 0x1b000000u) {
    bool sf = (inst >> 31) != 0;
    int op54 = (inst >> 21) & 7;
    int rm = (inst >> 16) & 0x1f;
    int o0 = (inst >> 15) & 1;
    int ra = (inst >> 10) & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    int rd = inst & 0x1f;
    if (op54 == 0) {
      if (ra == 31) {
        DAsmFormat(out, "%s %s, %s, %s", o0 ? "mneg" : "mul",
                   XReg(rd, sf, false), XReg(rn, sf, false),
                   XReg(rm, sf, false));
      } else {
        DAsmFormat(out, "%s %s, %s, %s, %s", o0 ? "msub" : "madd",
                   XReg(rd, sf, false), XReg(rn, sf, false),
                   XReg(rm, sf, false), XReg(ra, sf, false));
      }
      return true;
    }
    if (sf && ra == 31 && (op54 == 1 || op54 == 5)) {
      DAsmFormat(out, "%s %s, %s, %s", op54 == 1 ? "smull" : "umull",
                 XReg(rd, true, false), XReg(rn, false, false),
                 XReg(rm, false, false));
      return true;
    }
    if (sf && ra == 31 && (op54 == 2 || op54 == 6)) {
      DAsmFormat(out, "%s %s, %s, %s", op54 == 2 ? "smulh" : "umulh",
                 XReg(rd, true, false), XReg(rn, true, false),
                 XReg(rm, true, false));
      return true;
    }
  }
  if ((inst & 0x3b000000u) == 0x39000000u ||
      (inst & 0x3b000000u) == 0x39000000u) {
    bool load = (inst >> 22) & 1;
    int size = (inst >> 30) & 3;
    int scale = 1 << size;
    int imm = ((inst >> 10) & 0xfff) * scale;
    int rn = (inst >> 5) & 0x1f;
    int rt = inst & 0x1f;
    const char* op = load ? (size == 0 ? "ldrb" : (size == 1 ? "ldrh" : "ldr"))
                          : (size == 0 ? "strb" : (size == 1 ? "strh" : "str"));
    DAsmFormat(out, "%s %s, [%s, #%d]", op, XReg(rt, size == 3, false),
               XReg(rn, true, true), imm);
    return true;
  }
  if ((inst & 0x1f000000u) == 0x1e000000u) {
    int type = (inst >> 22) & 3;
    int opcode = (inst >> 10) & 0x3f;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    int rm = (inst >> 16) & 0x1f;
    const char* op = NULL;
    if (opcode == 2) op = "fmul";
    if (opcode == 6) op = "fdiv";
    if (opcode == 10) op = "fadd";
    if (opcode == 14) op = "fsub";
    if (opcode == 18) op = "fmax";
    if (opcode == 22) op = "fmin";
    if (op != NULL) {
      DAsmFormat(out, "%s %s, %s, %s", op, VReg(rd, type == 1 ? 8 : 4),
                 VReg(rn, type == 1 ? 8 : 4), VReg(rm, type == 1 ? 8 : 4));
      return true;
    }
  }
  if ((inst & 0xfffffc00u) == 0x9e670000u ||
      (inst & 0xfffffc00u) == 0x1e270000u) {
    bool width64 = (inst & 0x80000000u) != 0;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    DAsmFormat(out, "fmov %s, %s", VReg(rd, width64 ? 8 : 4),
               XReg(rn, width64, false));
    return true;
  }
  if ((inst & 0xfffffc00u) == 0x9e660000u ||
      (inst & 0xfffffc00u) == 0x1e260000u) {
    bool width64 = (inst & 0x80000000u) != 0;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    DAsmFormat(out, "fmov %s, %s", XReg(rd, width64, false),
               VReg(rn, width64 ? 8 : 4));
    return true;
  }
  if ((inst & 0xffbffc00u) == 0x1e204000u) {
    bool width64 = ((inst >> 22) & 1) != 0;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    DAsmFormat(out, "fmov %s, %s", VReg(rd, width64 ? 8 : 4),
               VReg(rn, width64 ? 8 : 4));
    return true;
  }
  if ((inst & 0xffbffc00u) == 0x1e214000u) {
    bool width64 = ((inst >> 22) & 1) != 0;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    DAsmFormat(out, "fneg %s, %s", VReg(rd, width64 ? 8 : 4),
               VReg(rn, width64 ? 8 : 4));
    return true;
  }
  if ((inst & 0xffa0fc1fu) == 0x1e202000u) {
    bool width64 = ((inst >> 22) & 1) != 0;
    int rn = (inst >> 5) & 0x1f;
    int rm = (inst >> 16) & 0x1f;
    DAsmFormat(out, "fcmp %s, %s", VReg(rn, width64 ? 8 : 4),
               VReg(rm, width64 ? 8 : 4));
    return true;
  }
  if ((inst & 0x7f22fc00u) == 0x1e200000u) {
    bool int64 = (inst >> 31) != 0;
    bool fp64 = ((inst >> 22) & 1) != 0;
    bool unsigned_convert = ((inst >> 16) & 1) != 0;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    DAsmFormat(out, "%s %s, %s", unsigned_convert ? "fcvtnu" : "fcvtns",
               XReg(rd, int64, false), VReg(rn, fp64 ? 8 : 4));
    return true;
  }
  if ((inst & 0x7f22fc00u) == 0x1e220000u) {
    bool int64 = (inst >> 31) != 0;
    bool fp64 = ((inst >> 22) & 1) != 0;
    bool unsigned_convert = ((inst >> 16) & 1) != 0;
    int rd = inst & 0x1f;
    int rn = (inst >> 5) & 0x1f;
    DAsmFormat(out, "%s %s, %s", unsigned_convert ? "ucvtf" : "scvtf",
               VReg(rd, fp64 ? 8 : 4), XReg(rn, int64, false));
    return true;
  }

  DAsmUnknownInstruction(out, ".word 0x%08" PRIx64, inst);
  return true;
}
