//
//  risc_v_disassembler.c
//  risc_v_interpreter
//
//  Created by David Allison on 4/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_disassembler.h"
#include <stdbool.h>
#include <inttypes.h>

// Find a register by searching for a free one in a set of non-overlapping
// ranges.
static struct {
  RegisterType type;   // Register type.
  int start;           // Start of range.
  int end;             // End of range.
  const char* prefix;  // Register name prefix
  int base;
} register_ranges[] = {
    {kRegTypeInt, RV_RET_REG, RV_RET_REG, "ra", 0},
    {kRegTypeInt, RV_INT_TEMP_START_1, RV_INT_TEMP_END_1, "t", 0},
    {kRegTypeInt, RV_INT_TEMP_START_2, RV_INT_TEMP_END_2, "t", 3},
    {kRegTypeInt, RV_INT_ARG_START, RV_INT_ARG_END, "a", 0},
    {kRegTypeInt, RV_INT_SAVED_START_1, RV_INT_SAVED_END_1, "s", 0},
    {kRegTypeInt, RV_INT_SAVED_START_2, RV_INT_SAVED_END_2, "s", 2},
    {kRegTypeFloat, RV_FP_TEMP_START_1, RV_FP_TEMP_END_1, "ft", 0},
    {kRegTypeFloat, RV_FP_TEMP_START_2, RV_FP_TEMP_END_2, "ft", 2},
    {kRegTypeFloat, RV_FP_ARG_START, RV_FP_ARG_END, "fa", 0},
    {kRegTypeFloat, RV_FP_SAVED_START_1, RV_FP_SAVED_END_1, "fs", 0},
    {kRegTypeFloat, RV_FP_SAVED_START_2, RV_FP_SAVED_END_2, "fs", 2},
};

#define NUM_REG_RANGES (sizeof(register_ranges) / sizeof(register_ranges[0]))

static const char* RegisterNameFromNum(int num, RegisterType type, char* buf,
                                       size_t len) {
  // See if the register is in one of the named ranges.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type &&
        register_ranges[i].start != RV_RET_REG) {
      if (num >= register_ranges[i].start && num <= register_ranges[i].end) {
        snprintf(buf, len, "%s%d", register_ranges[i].prefix,
                 num - register_ranges[i].start + register_ranges[i].base);
        return buf;
      }
    }
  }
  switch (type) {
    case kRegTypeInt:
      if (num == RV_SP_REG) {
        snprintf(buf, len, "sp");
        break;
      }
      if (num == RV_FP_REG) {
        snprintf(buf, len, "s0");
        break;
      }
      if (num == RV_RET_REG) {
        snprintf(buf, len, "ra");
        break;
      }

      snprintf(buf, len, "x%d", num);
      break;

    case kRegTypeFloat:
      snprintf(buf, len, "f%d", num);
      break;
  }
  return buf;
}

void DisassemblePrintRegister(FILE* fp, int reg, RegisterType type,
                              const char* sep) {
  char buf[16];
  fprintf(fp, "%s%s", sep, RegisterNameFromNum(reg, type, buf, sizeof(buf)));
}

static void PrintOp(FILE* fp, int rd, int rs1, int rs2) {
  DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
  DisassemblePrintRegister(fp, rs1, kRegTypeInt, ", ");
  DisassemblePrintRegister(fp, rs2, kRegTypeInt, ", ");
}

static void PrintOpImm(FILE* fp, int rd, int rs1, int imm) {
  DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
  DisassemblePrintRegister(fp, rs1, kRegTypeInt, ", ");
  fprintf(fp, ", %d", imm);
}

static void PrintLoad(FILE* fp, int rd, int rs1, int offset) {
  DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
  fprintf(fp, ", %d(", offset);
  DisassemblePrintRegister(fp, rs1, kRegTypeInt, "");
  fprintf(fp, ")");
}

static void PrintStore(FILE* fp, int rs1, int rs2, int offset) {
  DisassemblePrintRegister(fp, rs2, kRegTypeInt, "");
  fprintf(fp, ", %d(", offset);
  DisassemblePrintRegister(fp, rs1, kRegTypeInt, "");
  fprintf(fp, ")");
}

static struct {
  const char* name;
  int value;
} rounding_modes[] = {
    {"rne", 0}, {"rtz", 1}, {"rdn", 2}, {"rup", 3},
    {"rmm", 4}, {"dyn", 7}, {NULL, 0},
};

static void PrintOpFp(FILE* fp, int rd, int rs1, int rs2, int rm) {
  DisassemblePrintRegister(fp, rd, kRegTypeFloat, "");
  DisassemblePrintRegister(fp, rs1, kRegTypeFloat, ", ");
  DisassemblePrintRegister(fp, rs2, kRegTypeFloat, ", ");
  if (rm != 0) {
    for (int i = 0; rounding_modes[i].name != NULL; i++) {
      if (rounding_modes[i].value == rm) {
        fprintf(fp, ", %s", rounding_modes[i].name);
        break;
      }
    }
  }
}

static void PrintOpFpCvt(FILE* fp, int rd, int rs1, int rm, RegisterType t_rd,
                         RegisterType t_rs1) {
  DisassemblePrintRegister(fp, rd, t_rd, "");
  DisassemblePrintRegister(fp, rs1, t_rs1, ", ");
  if (rm != 0) {
    for (int i = 0; rounding_modes[i].name != NULL; i++) {
      if (rounding_modes[i].value == rm) {
        fprintf(fp, ", %s", rounding_modes[i].name);
        break;
      }
    }
  }
}

static void PrintOpFpCmp(FILE* fp, int rd, int rs1, int rs2) {
  DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
  DisassemblePrintRegister(fp, rs1, kRegTypeFloat, ", ");
  DisassemblePrintRegister(fp, rs2, kRegTypeFloat, ", ");
}

static void PrintOpFpMov(FILE* fp, int rd, int rs1, int rm, RegisterType t_rd,
                         RegisterType t_rs1) {
  DisassemblePrintRegister(fp, rd, t_rd, "");
  DisassemblePrintRegister(fp, rs1, t_rs1, ", ");
}

static void PrintMnemonic(FILE* fp, const char* mnemonic) {
  fprintf(fp, "\t%-8s  ", mnemonic);
}

void DisassembleRiscVInstruction(RISCVInterpreter* interpreter, void* p,
                                 FILE* fp) {
  int32_t* pc = p;
  const char* symbol_name = "???";
  uint64_t offset = 0;
  if (interpreter->current_symbol != NULL) {
    symbol_name = interpreter->current_symbol->name;
    offset = (uint64_t)p - interpreter->current_symbol->start;
  }
  fprintf(fp, "%s+0x%" PRIx64 ": %p  ", symbol_name, offset, pc);

  // Fetch instruction.
  int32_t inst = *pc;

  // Decocde common parts of instruction.
  RVInstOpcode opcode = inst & 0x7f;
  int rd = (inst >> 7) & 0x1f;
  int rs1 = (inst >> 15) & 0x1f;
  int rs2 = (inst >> 20) & 0x1f;

  // Highest level switch (on opcode).
  switch (opcode) {
    case RV_OPCODE(op): {
      int funct3 = (inst >> 12) & 0x7;
      int funct7 = (inst >> 25) & 0x7f;
      if (funct7 == RV_F7(mul)) {
        switch (funct3) {
          case RV_F3(mul):
            PrintMnemonic(fp, "mul");
            break;
          case RV_F3(mulh):
            PrintMnemonic(fp, "mulh");
            break;
          case RV_F3(mulhsu):
            PrintMnemonic(fp, "mulhsu");
            break;
          case RV_F3(mulhu):
            PrintMnemonic(fp, "mulhu");
            break;
          case RV_F3(div):
            PrintMnemonic(fp, "div");
            break;
          case RV_F3(divu):
            PrintMnemonic(fp, "divu");
            break;
          case RV_F3(rem):
            PrintMnemonic(fp, "rem");
            break;
          case RV_F3(remu):
            PrintMnemonic(fp, "remu");
            break;
        }
        PrintOp(fp, rd, rs1, rs2);
        break;
      }
      switch (funct3) {
        case RV_F3(add):  // add and sub:
          if (funct7 == RV_F7(sub)) {
            PrintMnemonic(fp, "sub");
          } else {
            PrintMnemonic(fp, "add");
          }
          break;
        case RV_F3(sll):
          PrintMnemonic(fp, "sll");
          break;
        case RV_F3(slt):
          PrintMnemonic(fp, "slt");
          break;
        case RV_F3(sltu):
          PrintMnemonic(fp, "sltu");
          break;
        case RV_F3 (xor):
          PrintMnemonic(fp, "xor");
          break;
        case RV_F3(srl):  // and sra
          if (funct7 == RV_F7(sra)) {
            PrintMnemonic(fp, "sra");
          } else {
            PrintMnemonic(fp, "srl");
          }
          break;

        case RV_F3(or):
          PrintMnemonic(fp, "or");
          break;
        case RV_F3(and):
          PrintMnemonic(fp, "and");
          break;
        default:
          break;
      }

      PrintOp(fp, rd, rs1, rs2);
      break;
    }

    case RV_OPCODE(op_imm): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
      switch (funct3) {
        case RV_F3(addi):
          PrintMnemonic(fp, "addi");
          break;
        case RV_F3(slti):
          PrintMnemonic(fp, "slti");
          break;
        case RV_F3(sltiu):
          PrintMnemonic(fp, "sltu");
          break;
        case RV_F3(xori):
          PrintMnemonic(fp, "xori");
          break;
        case RV_F3(ori):
          PrintMnemonic(fp, "ori");
          break;
        case RV_F3(andi):
          PrintMnemonic(fp, "andi");
          break;
        case RV_F3(slli):
          immed = (inst >> 20) & 0x3f;
          PrintMnemonic(fp, "slli");
          break;
        case RV_F3(srli): {
          int funct7 =
              (inst >> 25) & 0x7e;  // NOTE: bottom bit cleared for R64.
          immed = (inst >> 20) & 0x3f;
          if (funct7 == RV_F7(srai)) {
            PrintMnemonic(fp, "srai");
          } else {
            PrintMnemonic(fp, "srli");
          }
          break;
        }
        default:
          break;
      }
      PrintOpImm(fp, rd, rs1, (int)immed);
      break;
    }

    case RV_OPCODE(lui): {
      int64_t immed = inst >> 12;  // Auto sign extended to 64 bits.
      PrintMnemonic(fp, "lui");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      fprintf(fp, ", 0x%" PRIx64 "", immed);
      break;
    }
    case RV_OPCODE(auipc): {
      int64_t immed = inst >> 12;  // Auto sign extended to 64 bits.
      PrintMnemonic(fp, "auipc");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      fprintf(fp, ", 0x%" PRIx64 "        // 0x%" PRIx64 "", immed,
              (int64_t)pc + (immed << 12));
      break;
    }
    case RV_OPCODE(jal): {
      // immediate at bit 12 is encoded as imm[20|10:1|11|19:12]
      int64_t imm = inst >> 12;  // Auto sign extended to 64 bits.
      int64_t immed = (imm & 0xff) << 12 | ((imm >> 8) & 1) << 11 |
                      ((imm >> 9) & 0x3ff) << 1 | ((imm >> 19) & 1) << 20;
      // Sign extend to 64 bits.
      immed <<= 63 - 20;
      immed >>= 63 - 20;
      PrintMnemonic(fp, "jal");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      fprintf(fp, ", 0x%" PRIx64 "", (int64_t)pc + immed);
      break;
    }
    case RV_OPCODE(jalr): {
      int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
      if (rd == 0 && rs1 == RV_RET_REG && immed == 0) {
        // ret instruction.
        PrintMnemonic(fp, "ret");
        break;
      }
      PrintMnemonic(fp, "jalr");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      DisassemblePrintRegister(fp, rs1, kRegTypeInt, ", ");
      int64_t reg_value = interpreter->iregs[rs1];
      fprintf(fp, ", %" PRIx64 "        // 0x%" PRIx64 "", immed, reg_value + immed);
      break;
    }
    case RV_OPCODE(branch): {
      int64_t hi = inst >> 25;
      int64_t offset = (rd & 0x1e) | ((rd & 1) << 11) | ((hi & 0x3f) << 5) |
                       ((hi >> 6) << 12);
      int funct3 = (inst >> 12) & 0x7;
      static const char* branches[] = {
          "beq", "bne", "", "", "blt", "bge", "bltu", "bgeu",
      };
      PrintMnemonic(fp, branches[funct3]);
      DisassemblePrintRegister(fp, rs1, kRegTypeInt, "");
      DisassemblePrintRegister(fp, rs2, kRegTypeInt, ", ");
      fprintf(fp, ", 0x%" PRIx64 "", (int64_t)pc + offset);
      break;
    }
    case RV_OPCODE(load): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
      static const char* loads[] = {
          "lb", "lh", "lw", "ld", "lbu", "lhu", "lwu",
      };
      PrintMnemonic(fp, loads[funct3]);
      PrintLoad(fp, rd, rs1, (int)immed);
      break;
    }
    case RV_OPCODE(store): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed_hi = inst >> 25;       // Auto sign extended to 64 bits.
      int64_t immed = immed_hi << 5 | rd;  // rd is the low 5 bits of offset.
      static const char* stores[] = {
          "sb", "sh", "sw", "sd",
      };
      PrintMnemonic(fp, stores[funct3]);
      PrintStore(fp, rs1, rs2, (int)immed);
      break;
    }
    case RV_OPCODE(misc_mem): {
      break;
    }
    case RV_OPCODE(system): {
      int op = inst >> 20;
      switch (op) {
        case 0:  // ecall
          PrintMnemonic(fp, "ecall");
          break;
        case 1:  // ebreak
          PrintMnemonic(fp, "ebreak");
          break;
      }
      break;
    }
    case RV_OPCODE(op_imm_32): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
      switch (funct3) {
        case RV_F3(addiw):
          PrintMnemonic(fp, "addiw");
          break;
        case RV_F3(slliw):
          immed = (inst >> 20) & 0x3f;
          PrintMnemonic(fp, "slliw");
          break;

        case RV_F3(srliw): {
          int funct7 = (inst >> 25) & 0x7f;
          immed = (inst >> 20) & 0x3f;
          if (funct7 == RV_F7(sraiw)) {
            PrintMnemonic(fp, "sraiw");
          } else {
            PrintMnemonic(fp, "srliw");
          }
          break;
        }
      }
      PrintOpImm(fp, rd, rs1, (int32_t)immed);
      break;
    }
    case RV_OPCODE(op_32): {
      int funct3 = (inst >> 12) & 0x7;
      int funct7 = (inst >> 25) & 0x7f;
      switch (funct3) {
        case RV_F3(addw):
          if (funct7 == RV_F7(subw)) {
            PrintMnemonic(fp, "subw");
          } else {
            PrintMnemonic(fp, "addw");
          }
          break;
        case RV_F3(sllw):
          PrintMnemonic(fp, "sllw");
          break;

        case RV_F3(srlw):
          if (funct7 == RV_F7(sraw)) {
            PrintMnemonic(fp, "sraw");
          } else {
            PrintMnemonic(fp, "srlw");
          }
          break;
      }
      PrintOp(fp, rd, rs1, rs2);
      break;
    }
    case RV_OPCODE(load_fp): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed = inst >> 20;  // Auto sign extended to 64 bits.
      const char* mnemonic = funct3 == RV_F3(flw) ? "flw" : "fld";
      PrintMnemonic(fp, mnemonic);
      PrintLoad(fp, rd, rs1, (int)immed);
      break;
    }
    case RV_OPCODE(store_fp): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed_hi = inst >> 25;       // Auto sign extended to 64 bits.
      int64_t immed = immed_hi << 5 | rd;  // rd is the low 5 bits of offset.
      const char* mnemonic = funct3 == RV_F3(fsw) ? "fsw" : "fsd";
      PrintMnemonic(fp, mnemonic);
      PrintStore(fp, rs1, rs2, (int)immed);
    }
    case RV_OPCODE(op_fp): {
      int rm = (inst >> 12) & 0x7;
      int funct7 = (inst >> 25) & 0x7f;
      enum Format {
        kFormatALU,  // Regular ALU.
        kFormatCVT,  // Converstions.
        kFormatMUL,  // Multiply accumulate.
        kFormatCMP,  // Comparison.
        kFormatMOV,  // Bit Move.
      } format = kFormatALU;
      RegisterType t_rd = kRegTypeFloat;
      RegisterType t_rs1 = kRegTypeInt;

      switch (funct7) {
        case RV_F7(fadd_s):
          PrintMnemonic(fp, "fadd.s");
          break;
        case RV_F7(fsub_s):
          PrintMnemonic(fp, "fsub.s");
          break;
        case RV_F7(fmul_s):
          PrintMnemonic(fp, "fmul.s");
          break;
        case RV_F7(fdiv_s):
          PrintMnemonic(fp, "fdiv.s");
          break;
        case RV_F7(fsqrt_s):
          PrintMnemonic(fp, "fsqrt.s");
          break;
        case RV_F7(fmin_s):
          if (rm == RV_F3(fmin_s)) {
            PrintMnemonic(fp, "fmin.s");
          } else {
            PrintMnemonic(fp, "fmax.s");
          }
          break;
         case RV_F7(fadd_d):
          PrintMnemonic(fp, "fadd.d");
          break;
        case RV_F7(fsub_d):
          PrintMnemonic(fp, "fsub.d");
          break;
        case RV_F7(fmul_d):
          PrintMnemonic(fp, "fmul.d");
          break;
        case RV_F7(fdiv_d):
          PrintMnemonic(fp, "fdiv.d");
          break;
        case RV_F7(fsqrt_d):
          PrintMnemonic(fp, "fsqrt.d");
          break;
        case RV_F7(fmin_d):
          if (rm == RV_F3(fmin_s)) {
            PrintMnemonic(fp, "fmin.d");
          } else {
            PrintMnemonic(fp, "fmax.d");
          }
          break;
 
        // Floating point comparisons.
        case RV_F7(feq_s):  // And flt.s, fle.s
          switch (rm) {
            case RV_F3(feq_s:)
              PrintMnemonic(fp, "feq.s");
              break;
            case RV_F3(flt_s:)
              PrintMnemonic(fp, "flt.s");
              break;
            case RV_F3(fle_s):
              PrintMnemonic(fp, "fle.s");
              break;
          }
          break;

        case RV_F7(feq_d):  // And flt.d, fle.d
          switch (rm) {
          case RV_F3(feq_d:)
            PrintMnemonic(fp, "feq.d");
            break;
          case RV_F3(flt_d:)
            PrintMnemonic(fp, "flt.d");
            break;
          case RV_F3(fle_d):
            PrintMnemonic(fp, "fle.d");
            break;
          }
          break;

        // Moves.
        case RV_F7(fmv_w_x):
          PrintMnemonic(fp, "fmv.w.x");
          format = kFormatMOV;
          t_rs1 = kRegTypeInt;
          break;

        case RV_F7(fmv_x_w):
          PrintMnemonic(fp, "fmv.x.w");
          format = kFormatMOV;
          t_rd = kRegTypeInt;
          break;
          
          case RV_F7(fmv_d_x):
            PrintMnemonic(fp, "fmv.d.x");
          format = kFormatMOV;
          t_rs1 = kRegTypeInt;
            break;

          case RV_F7(fmv_x_d):
            PrintMnemonic(fp, "fmv.x.d");
          format = kFormatMOV;
          t_rd = kRegTypeInt;
            break;

        case RV_F7(fcvt_s_w):  //  and RV_F7(fcvt_s_wu):
          switch (rs2) {
            case 0:
              PrintMnemonic(fp, "fcvt.s.w");
              break;
            case 1:
              PrintMnemonic(fp, "fcvt.s.wu");
              break;
            case 2:
                PrintMnemonic(fp, "fcvt.s.l");
                break;
            case 3:
              PrintMnemonic(fp, "fcvt.s.lu");
              break;

          }
          format = kFormatCVT;
          t_rs1 = kRegTypeInt;
          break;

        case RV_F7(fcvt_w_s):  // and RV_F7(fcvt_wu_s):
          switch (rs2) {
             case 0:
               PrintMnemonic(fp, "fcvt.w.s");
               break;
             case 1:
               PrintMnemonic(fp, "fcvt.wu.s");
               break;
             case 2:
                 PrintMnemonic(fp, "fcvt.l.s");
                 break;
             case 3:
               PrintMnemonic(fp, "fcvt.lu.s");
               break;

           }
          format = kFormatCVT;
          t_rd = kRegTypeInt;
          break;
          
        case RV_F7(fcvt_s_d):
          PrintMnemonic(fp, "fcvt.s.d");
          break;
          
        case RV_F7(fcvt_d_s):
          PrintMnemonic(fp, "fcvt.d.s");
          break;
 

        case RV_F7(fsgnj_s):  // All sign injection instructions.
          switch (rm) {
            case RV_F3(fsgnj_s):
              PrintMnemonic(fp, "fsgnj.s");
              break;
            case RV_F3(fsgnjn_s):
              PrintMnemonic(fp, "fsgnjn.s");
              break;
            case RV_F3(fsgnjx_s):
              PrintMnemonic(fp, "fsgnjx.s");
              break;
          }
          break;

        case RV_F7(fsgnj_d):  // All sign injection instructions.
          switch (rm) {
            case RV_F3(fsgnj_d):
              PrintMnemonic(fp, "fsgnj.d");
              break;
            case RV_F3(fsgnjn_d):
              PrintMnemonic(fp, "fsgnjn.d");
              break;
            case RV_F3(fsgnjx_d):
              PrintMnemonic(fp, "fsgnjx.d");
              break;
          }
          break;
      }

      switch (format) {
        case kFormatALU:
          PrintOpFp(fp, rd, rs1, rs2, rm);
          break;
        case kFormatCVT:
          PrintOpFpCvt(fp, rd, rs1, rm, t_rd, t_rs1);
          break;
        case kFormatMUL:
          break;
        case kFormatCMP:
          PrintOpFpCmp(fp, rd, rs1, rs2);
          break;
        case kFormatMOV:
          PrintOpFpMov(fp, rd, rs2, rm, t_rd, t_rs1);
          break;
      }
    }
    default:
      break;
  }
  fputs("\n", fp);
}
