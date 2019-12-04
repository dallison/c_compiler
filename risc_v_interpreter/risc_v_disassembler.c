//
//  risc_v_disassembler.c
//  risc_v_interpreter
//
//  Created by David Allison on 4/27/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_disassembler.h"
#include <stdbool.h>


// Find a register by searching for a free one in a set of non-overlapping
// ranges.
static struct {
  RegisterType type;      // Register type.
  int start;                // Start of range.
  int end;                  // End of range.
  const char* prefix;       // Register name prefix
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

static const char *RegisterNameFromNum(int num, RegisterType type, char *buf, size_t len) {
  // See if the register is in one of the named ranges.
  for (size_t i = 0; i < NUM_REG_RANGES; i++) {
    if (register_ranges[i].type == type && register_ranges[i].start != RV_RET_REG) {
      if (num >= register_ranges[i].start && num <= register_ranges[i].end) {
        snprintf(buf, len, "%s%d",  register_ranges[i].prefix, num - register_ranges[i].start + register_ranges[i].base);
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

void DisassemblePrintRegister(FILE* fp, int reg, RegisterType type, const char* sep) {
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

static void PrintMnemonic(FILE* fp, const char* mnemonic) {
  fprintf(fp, "\t%-8s  ", mnemonic);
}

void DisassembleRiscVInstruction(Interpreter* interpreter, void* p, FILE* fp) {
  int32_t* pc = p;
  const char* symbol_name = "???";
  uint64_t offset = 0;
  if (interpreter->current_symbol != NULL) {
    symbol_name = interpreter->current_symbol->name;
    offset = (uint64_t)p - interpreter->current_symbol->start;
  }
  fprintf(fp, "%s+0x%llx: %p  ", symbol_name, offset, pc);
  
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
        case RV_F3(xor):
          PrintMnemonic(fp, "xor");
          break;
        case RV_F3(srl):     // and sra
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
      int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
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
          int funct7 = (inst >> 25) & 0x7e;   // NOTE: bottom bit cleared for R64.
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
      int64_t immed = inst >> 12;     // Auto sign extended to 64 bits.
      PrintMnemonic(fp, "lui");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      fprintf(fp, ", 0x%llx", immed);
      break;
    }
    case RV_OPCODE(auipc): {
      int64_t immed = inst >> 12;     // Auto sign extended to 64 bits.
      PrintMnemonic(fp, "auipc");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      fprintf(fp, ", 0x%llx        // 0x%llx", immed, (int64_t)pc + (immed << 12));
      break;
    }
    case RV_OPCODE(jal): {
      // immediate at bit 12 is encoded as imm[20|10:1|11|19:12]
      int64_t imm = inst >> 12;     // Auto sign extended to 64 bits.
      int64_t immed = (imm & 0xff) << 12 | ((imm >> 8) & 1) << 11 |
      ((imm >> 9) & 0x3ff) << 1 |
      ((imm >> 19) & 1) << 20;
      // Sign extend to 64 bits.
      immed <<= 63-20;
      immed >>= 63-20;
      PrintMnemonic(fp, "jal");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      fprintf(fp, ", 0x%llx", (int64_t)pc + immed);
      break;
    }
    case RV_OPCODE(jalr): {
      int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
      if (rd == 0 && rs1 == RV_RET_REG && immed == 0) {
        // ret instruction.
        PrintMnemonic(fp, "ret");
        break;
      }
      PrintMnemonic(fp, "jalr");
      DisassemblePrintRegister(fp, rd, kRegTypeInt, "");
      DisassemblePrintRegister(fp, rs1, kRegTypeInt, ", ");
      int64_t reg_value = interpreter->iregs[rs1];
      fprintf(fp, ", %llx        // 0x%llx", immed, reg_value + immed);
      break;
    }
    case RV_OPCODE(branch): {
      int64_t hi = inst >> 25;
      int64_t offset = (rd & 0x1e) | ((rd & 1) << 11) |
      ((hi & 0x3f) << 5) | ((hi >> 6) << 12);
      int funct3 = (inst >> 12) & 0x7;
      static const char* branches[] = {
        "beq",
        "bne",
        "",
        "",
        "blt",
        "bge",
        "bltu",
        "bgeu",
      };
      PrintMnemonic(fp, branches[funct3]);
      DisassemblePrintRegister(fp, rs1, kRegTypeInt, "");
      DisassemblePrintRegister(fp, rs2, kRegTypeInt, ", ");
      fprintf(fp, ", 0x%llx", (int64_t)pc + offset);
      break;
    }
    case RV_OPCODE(load): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
      static const char* loads[] = {
        "lb",
        "lh",
        "lw",
        "ld",
        "lbu",
        "lhu",
        "lwu",
      };
      PrintMnemonic(fp, loads[funct3]);
      PrintLoad(fp, rd, rs1, (int)immed);
      break;
    }
    case RV_OPCODE(store): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed_hi = inst >> 25;     // Auto sign extended to 64 bits.
      int64_t immed = immed_hi << 5 | rd;   // rd is the low 5 bits of offset.
      static const char* stores[] = {
        "sb",
        "sh",
        "sw",
        "sd",
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
        case 0:    // ecall
          PrintMnemonic(fp, "ecall");
          break;
        case 1:     // ebreak
          PrintMnemonic(fp, "ebreak");
          break;
      }
      break;
    }
    case RV_OPCODE(op_imm_32): {
      int funct3 = (inst >> 12) & 0x7;
      int64_t immed = inst >> 20;     // Auto sign extended to 64 bits.
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
      break;
    }
    case RV_OPCODE(store_fp): {
      break;
    }
    case RV_OPCODE(op_fp): {
      break;
    }
    default:
      break;
  }
  fputs("\n", fp);
}

