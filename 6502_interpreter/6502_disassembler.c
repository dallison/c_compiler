//
//  6502_disassembler.c
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_disassembler.h"

static void* DisassembleGroup8(uint16_t addr, void* p, int inst, FILE* fp) {
  static const char* mnemonics[16] = {
    "PHP",
    "CLC",
    "PLP",
    "SEC",
    "PHA",
    "CLI",
    "PLA",
    "SEI",
    "DEY",
    "TYA",
    "TAY",
    "CLV",
    "INY",
    "CLD",
    "INX",
    "SED",
  };
  fprintf(fp, "%s\n", mnemonics[inst]);
  return (char*)p + 1;
}

static void* DisassembleALU(uint16_t addr, void* p, int hi, int lo, FILE* fp) {
  static const char* mnemonic[8] = {
    "ORA",
    "AND",
    "EOR",
    "ADC",
    "STA",
    "LDA",
    "CMP",
    "SBC",
  };
  
  fprintf(fp, "%s ", mnemonic[hi / 2]);
  int operand = 0;
  switch (lo) {
    case 1:
      // Zero page indexed
      operand = *(uint8_t*)p;
      if ((hi & 1) == 0) {
        fprintf(fp, "(0x%x,X)\n", operand);
      } else {
        fprintf(fp, "(0x%x),Y\n", operand);
      }
      return (char*)p + 1;
    case 5:
      // Zero page and indexed
      operand = *(uint8_t*)p;
     if ((hi & 1) == 0) {
        fprintf(fp, "0x%x\n", operand);
      } else {
        fprintf(fp, "0x%x,X\n", operand);
      }
      return (char*)p + 1;

    case 9:
      // Immediate and absolute indexed Y
      if ((hi & 1) == 0) {
        operand = *(uint8_t*)p;
        fprintf(fp, "#0x%x\n", operand);
        return (char*)p + 1;
      } else {
        operand = *(uint16_t*)p;
        fprintf(fp, "0x%x,Y\n", operand);
        return (char*)p + 2;
      }
    case 13:
      // Absolute and indexed X
      operand = *(uint16_t*)p;
      if ((hi & 1) == 0) {
        fprintf(fp, "0x%x", operand);
      } else {
        fprintf(fp, "0x%x,X", operand);
      }
      fprintf(fp, "\n");
      return (char*)p + 2;
    default:
      return p;
  }
}

static void* DisassembleShiftAndMisc(uint16_t addr, void* p, int hi, int lo, FILE* fp) {
  static const char* mnemonic[8] = {
    "ASL",
    "ROL",
    "LSR",
    "ROR",
    "STX",
    "LDX",
    "DEC",
    "INC",
  };
  
  // The second half of the hi value for lo == 10 are
  // different.
  static const char* special_mnemonic[8] = {
    "TXA",
    "TXS",
    "TAX",
    "TSX",
    "DEX",
    "PHX",
    "NOP",
    "PLX",
  };
  
  if (lo != 10) {
    fprintf(fp, "%s ", mnemonic[hi / 2]);
  }
  
  int operand = 0;
  switch (lo) {
    case 6:
      // Zero page and indexed
      operand = *(uint8_t*)p;
      if ((hi & 1) == 0) {
        fprintf(fp, "0x%x", operand);
      } else {
        fprintf(fp, "0x%x,X", operand);
      }
      fprintf(fp, "\n");
      return (char*)p + 1;

    case 10:
      // Accumulator and special
      if (hi < 8) {
        if ((hi & 1) == 0) {
          // Shifts and rotates.
          fprintf(fp, "%s A", mnemonic[hi / 2]);
        } else {
          // Even more special.
          switch (hi) {
            case 1:
              fprintf(fp, "INC A");
              break;
            case 3:
              fprintf(fp, "DEC A");
              break;
            case 5:
              fprintf(fp, "PHY");
              break;
            case 7:
              fprintf(fp, "PLY");
              break;
          }
        }
      } else {
        fprintf(fp, "%s", special_mnemonic[hi - 8]);
      }
      fprintf(fp, "\n");
      return (char*)p ;
      
    case 14:
      // Absolute and indexed.
      operand = *(uint16_t*)p;
      if ((hi & 1) == 0) {
        fprintf(fp, "0x%x", operand);
      } else {
        fprintf(fp, "0x%x,X", operand);
      }
      fprintf(fp, "\n");
      return (char*)p + 2;
    default:
      return p;
  }
}

static void* DisassembleGroup0(uint16_t addr, void* p, int inst, FILE* fp) {
  enum OpType {
    kNoOperand,
    kBranch,
    kImmediate,
    kAbsolute,
  };
  
  struct {
    const char* mnemonic;
    enum OpType type;
  } instructions[16] = {
    {"BRK", kImmediate},
    {"BPL", kBranch},
    {"JSR", kAbsolute},
    {"BMI", kBranch},
    {"RTI", kNoOperand},
    {"BVC", kBranch},
    {"RTS", kNoOperand},
    {"BVS", kBranch},
    {"BRA", kBranch},
    {"BCC", kBranch},
    {"LDY", kImmediate},
    {"BCS", kBranch},
    {"CPY", kImmediate},
    {"BNE", kBranch},
    {"CPX", kImmediate},
    {"BEQ", kBranch},
  };
  fprintf(fp, "%s", instructions[inst].mnemonic);
  uint16_t operand = 0;
  switch (instructions[inst].type) {
    case kBranch:
      operand = *(int8_t*)p;
      fprintf(fp, " 0x%x\n", addr + operand);
      return (char*)p + 1;
    case kImmediate:
      operand = *(uint8_t*)p;
      fprintf(fp, " #0x%x\n", operand);
      return (char*)p + 1;
    case kNoOperand:
      fprintf(fp,"\n");
      return p;
    case kAbsolute:
      operand = *(uint16_t*)p;
      fprintf(fp, " 0x%x\n", operand & 0xffff);
      return (char*)p + 2;
  }
}

static void* DisassembleGroup4(uint16_t addr, void* p, int hi, FILE* fp) {
  static const char* mnemonic[16] = {
    "TSB",
    "TRB",
    "BIT",
    "BIT",
    "MVP",
    "MVN",
    "STZ",
    "STZ",
    "STY",
    "STY",
    "LDY",
    "LDY",
    "CPY",
    "PEI",
    "CPX",
    "PEA",
  };
  
  fprintf(fp, "%s ", mnemonic[hi]);
  // All zero page, some indexed.
  int operand = *(uint8_t*)p;
  if (hi == 7 || hi == 9 || hi == 11) {
    fprintf(fp, "0x%x,X", operand);
  } else {
    fprintf(fp, "0x%x", operand);
  }
  fprintf(fp, "\n");
  return (char*)p + 1;
}

static void* DisassembleGroup2(uint16_t addr, void* p, int hi, FILE* fp) {
  static const char* mnemonic[16] = {
    "COP",
    "ORA",
    "JSL",
    "AND",
    "WDM",
    "EOR",
    "PER",
    "ADC",
    "BRL",
    "STA",
    "LDX",
    "LDA",
    "REP",
    "CMP",
    "SEP",
    "SBC",
  };
  fprintf(fp, "%s ", mnemonic[hi]);
  int operand = *(uint16_t*)p;
  // Odd numbers are (zp) 65c02.
  if ((hi & 1) == 1) {
    fprintf(fp, "(0x%x)\n", operand);
  } else if (hi == 0xa) {
    // LDX #op
    fprintf(fp, "#0x%x\n", operand & 0xff);
  } else {
    fprintf(fp, "\n");    // Not on 65c02 or 6502.
  }
  return (char*)p + 1;
}

static void* DisassembleGroup12(uint16_t addr, void* p, int hi, FILE* fp) {
  static const char* mnemonic[16] = {
    "TSB",
    "TRB",
    "BIT",
    "BIT",
    "JMP",
    "JMP",
    "JMP",
    "JMP",
    "STY",
    "STZ",
    "LDY",
    "LDY",
    "CPY",
    "JMI",
    "CPX",
    "JSR",
  };
  
  fprintf(fp, "%s ", mnemonic[hi]);
  // All absolute , some indexed, some indirect
  int operand = *(uint16_t*)p;
  if (hi == 6) {
    // JMP (a)
    fprintf(fp, "(0x%x)", operand);
  } else if (hi == 7) {
    // JMP (a,X)
    fprintf(fp, "(0x%x,X)", operand);
  } else if (hi == 3 || hi == 11) {
    fprintf(fp, "0x%x,X", operand);
  } else {
    fprintf(fp, "0x%x", operand);
  }
  fprintf(fp, "\n");
  return (char*)p + 2;
}

void* Disassemble6502Instruction(Interpreter* interpreter, uint16_t addr, void* p, FILE* fp) {
  fprintf(fp, "%4x  ", addr);
  int lo = *(char*)p & 0xf;
  int hi = (*(char*)p & 0xf0) >> 4;
  void* operand = (char*)p + 1;
  
  switch (lo) {
    case 0:
      // Branches mostly.
      return DisassembleGroup0(addr, operand, hi, fp);
    case 1:
    case 5:
    case 9:
    case 13:
      // ALU.
      return DisassembleALU(addr, operand, hi, lo, fp);
    case 2:
      return DisassembleGroup2(addr, operand, hi, fp);
    case 3:
    case 7:
    case 11:
    case 15:
    default:
      fprintf(fp, "???\n");
      // Not used in 6502.
      return operand;
    case 10:
    case 14:
    case 6:
      return DisassembleShiftAndMisc(addr, operand, hi, lo, fp);
    case 8:
      return DisassembleGroup8(addr, p, hi, fp);
    case 4:
      return DisassembleGroup4(addr, operand, hi, fp);
    case 12:
      return DisassembleGroup12(addr, operand, hi, fp);
  }
}
