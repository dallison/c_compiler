//
//  disassembler_riscv.c
//  c_compiler
//

#include "disassembler_internal.h"

#include <inttypes.h>
#include <stdio.h>

static const char* XReg(int reg) {
  static const char* names[] = {
      "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
      "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
      "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
      "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6",
  };
  return names[reg & 31];
}

static const char* FReg(int reg) {
  static const char* names[] = {
      "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
      "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
      "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
      "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
  };
  return names[reg & 31];
}

static int64_t BranchImm(uint32_t inst) {
  uint32_t imm = ((inst >> 31) & 0x1) << 12;
  imm |= ((inst >> 7) & 0x1) << 11;
  imm |= ((inst >> 25) & 0x3f) << 5;
  imm |= ((inst >> 8) & 0xf) << 1;
  return DAsmSignExtend(imm, 13);
}

static int64_t JalImm(uint32_t inst) {
  uint32_t imm = ((inst >> 31) & 0x1) << 20;
  imm |= ((inst >> 12) & 0xff) << 12;
  imm |= ((inst >> 20) & 0x1) << 11;
  imm |= ((inst >> 21) & 0x3ff) << 1;
  return DAsmSignExtend(imm, 21);
}

static int64_t StoreImm(uint32_t inst) {
  uint32_t imm = ((inst >> 25) << 5) | ((inst >> 7) & 0x1f);
  return DAsmSignExtend(imm, 12);
}

static void FormatUnknown32(DAsmInstruction* out, uint32_t inst) {
  DAsmUnknownInstruction(out, ".word 0x%08" PRIx64, inst);
}

bool DAsmDisassembleRiscV(const void* bytes, size_t length, uint64_t address,
                          DAsmInstruction* out) {
  if (length < 4) {
    return false;
  }
  const unsigned char* p = bytes;
  uint32_t inst = DAsmRead32LE(p);
  DAsmInitInstruction(out, bytes, length, address, 4);

  int opcode = inst & 0x7f;
  int rd = (inst >> 7) & 0x1f;
  int funct3 = (inst >> 12) & 0x7;
  int rs1 = (inst >> 15) & 0x1f;
  int rs2 = (inst >> 20) & 0x1f;
  int funct7 = (inst >> 25) & 0x7f;
  int64_t imm_i = DAsmSignExtend(inst >> 20, 12);

  switch (opcode) {
    case 0x37:
      DAsmFormat(out, "lui %s, 0x%x", XReg(rd), inst & 0xfffff000u);
      return true;
    case 0x17:
      DAsmFormat(out, "auipc %s, 0x%x", XReg(rd), inst & 0xfffff000u);
      return true;
    case 0x6f:
      DAsmFormat(out, "jal %s, 0x%" PRIx64, XReg(rd), address + JalImm(inst));
      DAsmSetTarget(out, address + JalImm(inst));
      return true;
    case 0x67:
      DAsmFormat(out, "jalr %s, %" PRId64 "(%s)", XReg(rd), imm_i, XReg(rs1));
      return true;
    case 0x63: {
      static const char* names[] = {"beq", "bne", NULL, NULL,
                                    "blt", "bge", "bltu", "bgeu"};
      if (names[funct3] != NULL) {
        DAsmFormat(out, "%s %s, %s, 0x%" PRIx64, names[funct3], XReg(rs1),
                   XReg(rs2), address + BranchImm(inst));
        DAsmSetTarget(out, address + BranchImm(inst));
        return true;
      }
      break;
    }
    case 0x03: {
      static const char* names[] = {"lb", "lh", "lw", "ld",
                                    "lbu", "lhu", "lwu", NULL};
      if (names[funct3] != NULL) {
        DAsmFormat(out, "%s %s, %" PRId64 "(%s)", names[funct3], XReg(rd),
                   imm_i, XReg(rs1));
        return true;
      }
      break;
    }
    case 0x23: {
      static const char* names[] = {"sb", "sh", "sw", "sd"};
      if (funct3 < 4) {
        DAsmFormat(out, "%s %s, %" PRId64 "(%s)", names[funct3], XReg(rs2),
                   StoreImm(inst), XReg(rs1));
        return true;
      }
      break;
    }
    case 0x2f: {
      int funct5 = (inst >> 27) & 0x1f;
      bool aq = ((inst >> 26) & 1) != 0;
      bool rl = ((inst >> 25) & 1) != 0;
      const char* name =
          funct5 == 0x00 ? "amoadd" :
          funct5 == 0x02 ? "lr" :
          funct5 == 0x03 ? "sc" : NULL;
      const char* width = funct3 == 2 ? "w" : (funct3 == 3 ? "d" : NULL);
      if (name != NULL && width != NULL) {
        const char* ordering = aq && rl ? ".aqrl" :
                               aq ? ".aq" : (rl ? ".rl" : "");
        if (funct5 == 0x02) {
          DAsmFormat(out, "%s.%s%s %s, (%s)", name, width, ordering,
                     XReg(rd), XReg(rs1));
        } else {
          DAsmFormat(out, "%s.%s%s %s, %s, (%s)", name, width, ordering,
                     XReg(rd), XReg(rs2), XReg(rs1));
        }
        return true;
      }
      break;
    }
    case 0x13: {
      if (funct3 == 1) {
        DAsmFormat(out, "slli %s, %s, %d", XReg(rd), XReg(rs1),
                   (inst >> 20) & 0x3f);
        return true;
      }
      if (funct3 == 5) {
        DAsmFormat(out, "%s %s, %s, %d", funct7 == 0x20 ? "srai" : "srli",
                   XReg(rd), XReg(rs1), (inst >> 20) & 0x3f);
        return true;
      }
      static const char* names[] = {"addi", NULL, "slti", "sltiu",
                                    "xori", NULL, "ori", "andi"};
      if (names[funct3] != NULL) {
        DAsmFormat(out, "%s %s, %s, %" PRId64, names[funct3], XReg(rd),
                   XReg(rs1), imm_i);
        return true;
      }
      break;
    }
    case 0x1b: {
      if (funct3 == 0) {
        DAsmFormat(out, "addiw %s, %s, %" PRId64, XReg(rd), XReg(rs1), imm_i);
        return true;
      }
      if (funct3 == 1 || funct3 == 5) {
        DAsmFormat(out, "%s %s, %s, %d",
                   funct3 == 1 ? "slliw" : (funct7 == 0x20 ? "sraiw" : "srliw"),
                   XReg(rd), XReg(rs1), (inst >> 20) & 0x1f);
        return true;
      }
      break;
    }
    case 0x33:
    case 0x3b: {
      bool word = opcode == 0x3b;
      const char* name = NULL;
      if (funct7 == 0x01) {
        static const char* m[] = {"mul", "mulh", "mulhsu", "mulhu",
                                  "div", "divu", "rem", "remu"};
        name = m[funct3];
      } else if (funct3 == 0) {
        name = funct7 == 0x20 ? "sub" : "add";
      } else if (funct3 == 1) {
        name = "sll";
      } else if (funct3 == 2) {
        name = "slt";
      } else if (funct3 == 3) {
        name = "sltu";
      } else if (funct3 == 4) {
        name = "xor";
      } else if (funct3 == 5) {
        name = funct7 == 0x20 ? "sra" : "srl";
      } else if (funct3 == 6) {
        name = "or";
      } else if (funct3 == 7) {
        name = "and";
      }
      if (name != NULL) {
        char suffixed[16];
        if (word) {
          snprintf(suffixed, sizeof(suffixed), "%sw", name);
          name = suffixed;
        }
        DAsmFormat(out, "%s %s, %s, %s", name, XReg(rd), XReg(rs1), XReg(rs2));
        return true;
      }
      break;
    }
    case 0x07:
    case 0x27: {
      bool load = opcode == 0x07;
      int64_t imm = load ? imm_i : StoreImm(inst);
      int freg = load ? rd : rs2;
      const char* name = funct3 == 2 ? (load ? "flw" : "fsw")
                                     : (funct3 == 3 ? (load ? "fld" : "fsd") : NULL);
      if (name != NULL) {
        DAsmFormat(out, "%s %s, %" PRId64 "(%s)", name, FReg(freg), imm,
                   XReg(rs1));
        return true;
      }
      break;
    }
    case 0x53: {
      const char* suffix = NULL;
      const char* name = NULL;
      bool unary = false;
      bool cmp = false;
      bool move_to_int = false;
      bool move_from_int = false;
      bool cvt_to_int = false;
      bool cvt_from_int = false;
      switch (funct7) {
        case 0x00: name = "fadd"; suffix = "s"; break;
        case 0x04: name = "fsub"; suffix = "s"; break;
        case 0x08: name = "fmul"; suffix = "s"; break;
        case 0x0c: name = "fdiv"; suffix = "s"; break;
        case 0x2c: name = "fsqrt"; suffix = "s"; unary = true; break;
        case 0x10:
          suffix = "s";
          name = funct3 == 0 ? "fsgnj" : (funct3 == 1 ? "fsgnjn" : "fsgnjx");
          break;
        case 0x14:
          suffix = "s";
          name = funct3 == 0 ? "fmin" : "fmax";
          break;
        case 0x50:
          suffix = "s";
          cmp = true;
          name = funct3 == 2 ? "feq" : (funct3 == 1 ? "flt" : "fle");
          break;
        case 0x60:
          suffix = "s";
          cvt_to_int = true;
          static const char* to_int_s[] = {"fcvt.w", "fcvt.wu", "fcvt.l", "fcvt.lu"};
          name = rs2 < 4 ? to_int_s[rs2] : NULL;
          break;
        case 0x68:
          suffix = "s";
          cvt_from_int = true;
          static const char* from_int_s[] = {"fcvt.s.w", "fcvt.s.wu", "fcvt.s.l", "fcvt.s.lu"};
          name = rs2 < 4 ? from_int_s[rs2] : NULL;
          break;
        case 0x70:
          suffix = "s";
          if (funct3 == 0) {
            name = "fmv.x";
            move_to_int = true;
          }
          break;
        case 0x78:
          suffix = "s";
          if (funct3 == 0) {
            name = "fmv";
            move_from_int = true;
          }
          break;
        case 0x01: name = "fadd"; suffix = "d"; break;
        case 0x05: name = "fsub"; suffix = "d"; break;
        case 0x09: name = "fmul"; suffix = "d"; break;
        case 0x0d: name = "fdiv"; suffix = "d"; break;
        case 0x2d: name = "fsqrt"; suffix = "d"; unary = true; break;
        case 0x11:
          suffix = "d";
          name = funct3 == 0 ? "fsgnj" : (funct3 == 1 ? "fsgnjn" : "fsgnjx");
          break;
        case 0x15:
          suffix = "d";
          name = funct3 == 0 ? "fmin" : "fmax";
          break;
        case 0x51:
          suffix = "d";
          cmp = true;
          name = funct3 == 2 ? "feq" : (funct3 == 1 ? "flt" : "fle");
          break;
        case 0x20:
          DAsmFormat(out, "fcvt.s.d %s, %s", FReg(rd), FReg(rs1));
          return true;
        case 0x21:
          DAsmFormat(out, "fcvt.d.s %s, %s", FReg(rd), FReg(rs1));
          return true;
        case 0x61:
          suffix = "d";
          cvt_to_int = true;
          static const char* to_int_d[] = {"fcvt.w", "fcvt.wu", "fcvt.l", "fcvt.lu"};
          name = rs2 < 4 ? to_int_d[rs2] : NULL;
          break;
        case 0x69:
          suffix = "d";
          cvt_from_int = true;
          static const char* from_int_d[] = {"fcvt.d.w", "fcvt.d.wu", "fcvt.d.l", "fcvt.d.lu"};
          name = rs2 < 4 ? from_int_d[rs2] : NULL;
          break;
        case 0x71:
          suffix = "d";
          if (funct3 == 0) {
            name = "fmv.x";
            move_to_int = true;
          }
          break;
        case 0x79:
          suffix = "d";
          if (funct3 == 0) {
            name = "fmv";
            move_from_int = true;
          }
          break;
      }
      if (name != NULL && suffix != NULL) {
        if (cmp) {
          DAsmFormat(out, "%s.%s %s, %s, %s", name, suffix, XReg(rd),
                     FReg(rs1), FReg(rs2));
        } else if (move_to_int || cvt_to_int) {
          DAsmFormat(out, "%s.%s %s, %s", name, suffix, XReg(rd), FReg(rs1));
        } else if (move_from_int || cvt_from_int) {
          DAsmFormat(out, "%s %s, %s", name, FReg(rd), XReg(rs1));
        } else if (unary) {
          DAsmFormat(out, "%s.%s %s, %s", name, suffix, FReg(rd), FReg(rs1));
        } else {
          DAsmFormat(out, "%s.%s %s, %s, %s", name, suffix, FReg(rd),
                     FReg(rs1), FReg(rs2));
        }
        return true;
      }
      break;
    }
    case 0x73:
      if (inst == 0x00000073) {
        DAsmFormat(out, "ecall");
        return true;
      }
      if (inst == 0x00100073) {
        DAsmFormat(out, "ebreak");
        return true;
      }
      break;
    case 0x0f:
      DAsmFormat(out, "fence");
      return true;
  }

  FormatUnknown32(out, inst);
  return true;
}
