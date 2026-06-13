//
//  disassembler_arm.c
//  c_compiler
//

#include "disassembler_internal.h"

#include <inttypes.h>
#include <stdio.h>

static const char* CondName(int cond) {
  static const char* names[] = {
      "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
      "hi", "ls", "ge", "lt", "gt", "le", "",   "nv",
  };
  return names[cond & 15];
}

static const char* RegName(int reg) {
  static const char* names[] = {"r0", "r1", "r2",  "r3", "r4", "r5",
                                "r6", "r7", "r8",  "r9", "r10", "fp",
                                "ip", "sp", "lr", "pc"};
  return names[reg & 15];
}

static const char* FRegName(int reg, bool double_reg) {
  static char bufs[4][8];
  static int index = 0;
  char* buf = bufs[index++ & 3];
  snprintf(buf, 8, "%c%d", double_reg ? 'd' : 's', reg);
  return buf;
}

static int DecodeVfpSd(uint32_t inst) {
  return (((inst >> 12) & 0xf) << 1) | ((inst >> 22) & 1);
}

static int DecodeVfpSn(uint32_t inst) {
  return (((inst >> 16) & 0xf) << 1) | ((inst >> 7) & 1);
}

static int DecodeVfpSm(uint32_t inst) {
  return ((inst & 0xf) << 1) | ((inst >> 5) & 1);
}

static int DecodeVfpDd(uint32_t inst) {
  return ((inst >> 12) & 0xf) | (((inst >> 22) & 1) << 4);
}

static int DecodeVfpDn(uint32_t inst) {
  return ((inst >> 16) & 0xf) | (((inst >> 7) & 1) << 4);
}

static int DecodeVfpDm(uint32_t inst) {
  return (inst & 0xf) | (((inst >> 5) & 1) << 4);
}

static uint32_t RotateRight(uint32_t value, int amount) {
  amount &= 31;
  return amount == 0 ? value : ((value >> amount) | (value << (32 - amount)));
}

static uint32_t DecodeDataImm(uint32_t inst) {
  return RotateRight(inst & 0xff, ((inst >> 8) & 0xf) * 2);
}

static int64_t DecodeBranchImm(uint32_t inst) {
  return DAsmSignExtend(inst & 0x00ffffffu, 24) << 2;
}

static void FormatRegList(char* out, size_t size, uint32_t mask) {
  size_t off = 0;
  off += snprintf(out + off, size - off, "{");
  for (int i = 0; i < 16; i++) {
    if ((mask & (1u << i)) == 0) {
      continue;
    }
    off += snprintf(out + off, off < size ? size - off : 0, "%s%s",
                    off > 1 ? ", " : "", RegName(i));
  }
  snprintf(out + off, off < size ? size - off : 0, "}");
}

bool DAsmDisassembleARM(const void* bytes, size_t length, uint64_t address,
                        DAsmInstruction* out) {
  if (length < 4) {
    return false;
  }
  uint32_t inst = DAsmRead32LE(bytes);
  DAsmInitInstruction(out, bytes, length, address, 4);

  int cond = inst >> 28;
  const char* cc = CondName(cond);

  if ((inst & 0x0e000000u) == 0x0a000000u) {
    bool link = (inst & 0x01000000u) != 0;
    uint64_t target = address + 8 + DecodeBranchImm(inst);
    DAsmFormat(out, "%s%s 0x%" PRIx64, link ? "bl" : "b", cc, target);
    DAsmSetTarget(out, target);
    return true;
  }
  if ((inst & 0x0ffffff0u) == 0x012fff10u) {
    DAsmFormat(out, "bx%s %s", cc, RegName(inst & 0xf));
    return true;
  }
  if ((inst & 0x0ffffff0u) == 0x012fff30u) {
    DAsmFormat(out, "blx%s %s", cc, RegName(inst & 0xf));
    return true;
  }
  if ((inst & 0x0c000000u) == 0x04000000u) {
    bool load = (inst & 0x00100000u) != 0;
    bool byte = (inst & 0x00400000u) != 0;
    bool up = (inst & 0x00800000u) != 0;
    bool pre = (inst & 0x01000000u) != 0;
    bool write = (inst & 0x00200000u) != 0;
    int rn = (inst >> 16) & 0xf;
    int rd = (inst >> 12) & 0xf;
    int imm = inst & 0xfff;
    const char* op = load ? (byte ? "ldrb" : "ldr") : (byte ? "strb" : "str");
    if (pre) {
      DAsmFormat(out, "%s%s %s, [%s%s#%s%d]%s", op, cc, RegName(rd),
                 RegName(rn), imm == 0 ? "" : ", ", up ? "" : "-", imm,
                 write ? "!" : "");
    } else {
      DAsmFormat(out, "%s%s %s, [%s], #%s%d", op, cc, RegName(rd), RegName(rn),
                 up ? "" : "-", imm);
    }
    return true;
  }
  if ((inst & 0x0e000090u) == 0x00000090u) {
    bool load = (inst & 0x00100000u) != 0;
    bool sign = (inst & 0x00000040u) != 0;
    bool half = (inst & 0x00000020u) != 0;
    int rn = (inst >> 16) & 0xf;
    int rd = (inst >> 12) & 0xf;
    int imm = ((inst >> 4) & 0xf0) | (inst & 0xf);
    const char* op = NULL;
    if (load && sign && half) op = "ldrsh";
    if (load && sign && !half) op = "ldrsb";
    if (load && !sign && half) op = "ldrh";
    if (!load && !sign && half) op = "strh";
    if (op != NULL) {
      DAsmFormat(out, "%s%s %s, [%s, #%d]", op, cc, RegName(rd), RegName(rn),
                 imm);
      return true;
    }
  }
  if ((inst & 0x0e000000u) == 0x08000000u) {
    bool load = (inst & 0x00100000u) != 0;
    bool up = (inst & 0x00800000u) != 0;
    bool pre = (inst & 0x01000000u) != 0;
    bool write = (inst & 0x00200000u) != 0;
    int rn = (inst >> 16) & 0xf;
    char regs[96];
    FormatRegList(regs, sizeof(regs), inst & 0xffff);
    const char* mode = pre ? (up ? "ib" : "db") : (up ? "ia" : "da");
    DAsmFormat(out, "%s%s%s %s%s, %s", load ? "ldm" : "stm", mode, cc,
               RegName(rn), write ? "!" : "", regs);
    return true;
  }
  if ((inst & 0x0ff00f10u) == 0x0c400b10u ||
      (inst & 0x0ff00f10u) == 0x0c500b10u) {
    bool from_fp = (inst & 0x00100000u) != 0;
    int rt = (inst >> 12) & 0xf;
    int rt2 = (inst >> 16) & 0xf;
    int dm = DecodeVfpDm(inst);
    if (from_fp) {
      DAsmFormat(out, "vmov%s %s, %s, %s", cc, RegName(rt), RegName(rt2),
                 FRegName(dm, true));
    } else {
      DAsmFormat(out, "vmov%s %s, %s, %s", cc, FRegName(dm, true),
                 RegName(rt), RegName(rt2));
    }
    return true;
  }
  if ((inst & 0x0fbf0fd0u) == (0xeeb00a40u & 0x0fbf0fd0u) ||
      (inst & 0x0fbf0fd0u) == (0xeeb00b40u & 0x0fbf0fd0u)) {
    bool double_reg = ((inst >> 8) & 1) != 0;
    int vd = double_reg ? DecodeVfpDd(inst) : DecodeVfpSd(inst);
    int vm = double_reg ? DecodeVfpDm(inst) : DecodeVfpSm(inst);
    DAsmFormat(out, "vmov%s.f%d %s, %s", cc, double_reg ? 64 : 32,
               FRegName(vd, double_reg), FRegName(vm, double_reg));
    return true;
  }
  if ((inst & 0x0fb00f10u) == 0x0e000a10u ||
      (inst & 0x0fb00f10u) == 0x0e100a10u) {
    bool from_fp = (inst & 0x00100000u) != 0;
    int rt = (inst >> 12) & 0xf;
    int sn = DecodeVfpSn(inst);
    if (from_fp) {
      DAsmFormat(out, "vmov%s %s, %s", cc, RegName(rt), FRegName(sn, false));
    } else {
      DAsmFormat(out, "vmov%s %s, %s", cc, FRegName(sn, false), RegName(rt));
    }
    return true;
  }
  if ((inst & 0x0ff00f10u) == 0x0e000b10u ||
      (inst & 0x0ff00f10u) == 0x0e400b10u) {
    bool unsigned_convert = (inst & 0x00400000u) != 0;
    bool double_reg = (inst & 0x00000080u) != 0;
    int rt = (inst >> 16) & 0xf;
    int sd = double_reg ? DecodeVfpDd(inst) : DecodeVfpSd(inst);
    DAsmFormat(out, "%s%s %s, %s", unsigned_convert ? "ucvtf" : "scvtf", cc,
               FRegName(sd, double_reg), RegName(rt));
    return true;
  }
  if ((inst & 0x0ff00f10u) == 0x0e100b10u) {
    bool double_reg = (inst & 0x00000080u) != 0;
    int rt = (inst >> 12) & 0xf;
    int sm = double_reg ? DecodeVfpDm(inst) : DecodeVfpSm(inst);
    DAsmFormat(out, "fcvtns%s %s, %s", cc, RegName(rt),
               FRegName(sm, double_reg));
    return true;
  }
  if ((inst & 0x0f000000u) == 0x0d000000u &&
      ((inst >> 8) & 0xf) >= 0xa) {
    bool load = (inst & 0x00100000u) != 0;
    bool up = (inst & 0x00800000u) != 0;
    bool double_reg = ((inst >> 8) & 1) != 0;
    int rn = (inst >> 16) & 0xf;
    int rd = double_reg ? DecodeVfpDd(inst) : DecodeVfpSd(inst);
    int imm = (inst & 0xff) * 4;
    DAsmFormat(out, "%s%s %s, [%s, #%s%d]", load ? "vldr" : "vstr", cc,
               FRegName(rd, double_reg), RegName(rn), up ? "" : "-", imm);
    return true;
  }
  if ((inst & 0x0f000000u) == 0x0f000000u) {
    DAsmFormat(out, "svc%s #0x%x", cc, inst & 0x00ffffffu);
    return true;
  }
  if ((inst & 0x0fe000f0u) == 0x00000090u) {
    int rd = (inst >> 16) & 0xf;
    int rn = (inst >> 12) & 0xf;
    int rs = (inst >> 8) & 0xf;
    int rm = inst & 0xf;
    DAsmFormat(out, "mul%s %s, %s, %s%s%s", cc,
               rn == 0 ? RegName(rd) : RegName(rn), RegName(rm), RegName(rs),
               rn == 0 ? "" : ", ", rn == 0 ? "" : RegName(rd));
    return true;
  }
  if ((inst & 0x0ff00000u) == 0x03000000u ||
      (inst & 0x0ff00000u) == 0x03400000u) {
    bool top_half = (inst & 0x00400000u) != 0;
    int rd = (inst >> 12) & 0xf;
    uint32_t imm = ((inst >> 4) & 0xf000u) | (inst & 0xfffu);
    DAsmFormat(out, "%s%s %s, #0x%x", top_half ? "movt" : "movw", cc,
               RegName(rd), imm);
    return true;
  }
  if ((inst & 0x0c000000u) == 0x00000000u) {
    bool imm = (inst & 0x02000000u) != 0;
    int opcode = (inst >> 21) & 0xf;
    bool set_flags = (inst & 0x00100000u) != 0;
    int rn = (inst >> 16) & 0xf;
    int rd = (inst >> 12) & 0xf;
    int rm = inst & 0xf;
    uint32_t operand = imm ? DecodeDataImm(inst) : (uint32_t)rm;
    static const char* names[] = {"and", "eor", "sub", "rsb", "add", "adc",
                                  "sbc", "rsc", "tst", "teq", "cmp", "cmn",
                                  "orr", "mov", "bic", "mvn"};
    const char* suffix = set_flags && opcode != 8 && opcode != 9 &&
                                 opcode != 10 && opcode != 11
                             ? "s"
                             : "";
    if (opcode == 8 || opcode == 9 || opcode == 10 || opcode == 11) {
      if (imm) {
        DAsmFormat(out, "%s%s %s, #%" PRIu32, names[opcode], cc, RegName(rn),
                   operand);
      } else {
        DAsmFormat(out, "%s%s %s, %s", names[opcode], cc, RegName(rn),
                   RegName(rm));
      }
    } else if (opcode == 13 || opcode == 15) {
      if (imm) {
        DAsmFormat(out, "%s%s%s %s, #%" PRIu32, names[opcode], suffix, cc,
                   RegName(rd), operand);
      } else {
        DAsmFormat(out, "%s%s%s %s, %s", names[opcode], suffix, cc,
                   RegName(rd), RegName(rm));
      }
    } else {
      if (imm) {
        DAsmFormat(out, "%s%s%s %s, %s, #%" PRIu32, names[opcode], suffix, cc,
                   RegName(rd), RegName(rn), operand);
      } else {
        DAsmFormat(out, "%s%s%s %s, %s, %s", names[opcode], suffix, cc,
                   RegName(rd), RegName(rn), RegName(rm));
      }
    }
    return true;
  }
  if ((inst & 0x0fb00e50u) == (0xee300a00u & 0x0fb00e50u) ||
      (inst & 0x0fb00e50u) == (0xee300a40u & 0x0fb00e50u) ||
      (inst & 0x0fb00e50u) == (0xee200a00u & 0x0fb00e50u) ||
      (inst & 0x0fb00e50u) == (0xee800a00u & 0x0fb00e50u)) {
    bool double_reg = ((inst >> 8) & 1) != 0;
    int vd = double_reg ? DecodeVfpDd(inst) : DecodeVfpSd(inst);
    int vn = double_reg ? DecodeVfpDn(inst) : DecodeVfpSn(inst);
    int vm = double_reg ? DecodeVfpDm(inst) : DecodeVfpSm(inst);
    const char* op = NULL;
    uint32_t key = inst & 0x0fb00e50u;
    if (key == (0xee300a00u & 0x0fb00e50u)) op = "vadd";
    if (key == (0xee300a40u & 0x0fb00e50u)) op = "vsub";
    if (key == (0xee200a00u & 0x0fb00e50u)) op = "vmul";
    if (key == (0xee800a00u & 0x0fb00e50u)) op = "vdiv";
    if (op != NULL) {
      DAsmFormat(out, "%s%s.f%d %s, %s, %s", op, cc, double_reg ? 64 : 32,
                 FRegName(vd, double_reg), FRegName(vn, double_reg),
                 FRegName(vm, double_reg));
      return true;
    }
  }

  DAsmUnknownInstruction(out, ".word 0x%08" PRIx64, inst);
  return true;
}
