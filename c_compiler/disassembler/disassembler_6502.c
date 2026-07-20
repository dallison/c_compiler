//
//  disassembler_6502.c
//  c_compiler
//

#include "disassembler_internal.h"

#include <inttypes.h>

typedef enum {
  k6502Implied,
  k6502Accumulator,
  k6502Immediate,
  k6502ZeroPage,
  k6502ZeroPageX,
  k6502ZeroPageY,
  k6502Absolute,
  k6502AbsoluteX,
  k6502AbsoluteY,
  k6502Indirect,
  k6502IndexedIndirect,
  k6502IndirectIndexed,
  k6502ZeroPageIndirect,
  k6502AbsoluteIndexedIndirect,
  k6502Relative,
} DAsm6502AddressMode;

typedef struct {
  const char* mnemonic;
  DAsm6502AddressMode mode;
} DAsm6502Opcode;

#define OP(code, name, mode) [code] = {name, k6502##mode}

static const DAsm6502Opcode opcodes[256] = {
    OP(0x00, "brk", Implied),    OP(0x01, "ora", IndexedIndirect),
    OP(0x04, "tsb", ZeroPage),   OP(0x05, "ora", ZeroPage),
    OP(0x06, "asl", ZeroPage),   OP(0x08, "php", Implied),
    OP(0x09, "ora", Immediate),  OP(0x0a, "asl", Accumulator),
    OP(0x0c, "tsb", Absolute),   OP(0x0d, "ora", Absolute),
    OP(0x0e, "asl", Absolute),   OP(0x10, "bpl", Relative),
    OP(0x11, "ora", IndirectIndexed),
    OP(0x12, "ora", ZeroPageIndirect),
    OP(0x14, "trb", ZeroPage),   OP(0x15, "ora", ZeroPageX),
    OP(0x16, "asl", ZeroPageX),  OP(0x18, "clc", Implied),
    OP(0x19, "ora", AbsoluteY),  OP(0x1a, "inc", Accumulator),
    OP(0x1c, "trb", Absolute),   OP(0x1d, "ora", AbsoluteX),
    OP(0x1e, "asl", AbsoluteX),  OP(0x20, "jsr", Absolute),
    OP(0x21, "and", IndexedIndirect),
    OP(0x24, "bit", ZeroPage),   OP(0x25, "and", ZeroPage),
    OP(0x26, "rol", ZeroPage),   OP(0x28, "plp", Implied),
    OP(0x29, "and", Immediate),  OP(0x2a, "rol", Accumulator),
    OP(0x2c, "bit", Absolute),   OP(0x2d, "and", Absolute),
    OP(0x2e, "rol", Absolute),   OP(0x30, "bmi", Relative),
    OP(0x31, "and", IndirectIndexed),
    OP(0x32, "and", ZeroPageIndirect),
    OP(0x34, "bit", ZeroPageX),  OP(0x35, "and", ZeroPageX),
    OP(0x36, "rol", ZeroPageX),  OP(0x38, "sec", Implied),
    OP(0x39, "and", AbsoluteY),  OP(0x3a, "dec", Accumulator),
    OP(0x3c, "bit", AbsoluteX),  OP(0x3d, "and", AbsoluteX),
    OP(0x3e, "rol", AbsoluteX),  OP(0x40, "rti", Implied),
    OP(0x41, "eor", IndexedIndirect),
    OP(0x45, "eor", ZeroPage),   OP(0x46, "lsr", ZeroPage),
    OP(0x48, "pha", Implied),    OP(0x49, "eor", Immediate),
    OP(0x4a, "lsr", Accumulator),
    OP(0x4c, "jmp", Absolute),   OP(0x4d, "eor", Absolute),
    OP(0x4e, "lsr", Absolute),   OP(0x50, "bvc", Relative),
    OP(0x51, "eor", IndirectIndexed),
    OP(0x52, "eor", ZeroPageIndirect),
    OP(0x55, "eor", ZeroPageX),  OP(0x56, "lsr", ZeroPageX),
    OP(0x58, "cli", Implied),    OP(0x59, "eor", AbsoluteY),
    OP(0x5a, "phy", Implied),    OP(0x5d, "eor", AbsoluteX),
    OP(0x5e, "lsr", AbsoluteX),  OP(0x60, "rts", Implied),
    OP(0x61, "adc", IndexedIndirect),
    OP(0x64, "stz", ZeroPage),   OP(0x65, "adc", ZeroPage),
    OP(0x66, "ror", ZeroPage),   OP(0x68, "pla", Implied),
    OP(0x69, "adc", Immediate),  OP(0x6a, "ror", Accumulator),
    OP(0x6c, "jmp", Indirect),   OP(0x6d, "adc", Absolute),
    OP(0x6e, "ror", Absolute),   OP(0x70, "bvs", Relative),
    OP(0x71, "adc", IndirectIndexed),
    OP(0x72, "adc", ZeroPageIndirect),
    OP(0x74, "stz", ZeroPageX),  OP(0x75, "adc", ZeroPageX),
    OP(0x76, "ror", ZeroPageX),  OP(0x78, "sei", Implied),
    OP(0x79, "adc", AbsoluteY),  OP(0x7a, "ply", Implied),
    OP(0x7c, "jmp", AbsoluteIndexedIndirect),
    OP(0x7d, "adc", AbsoluteX),  OP(0x7e, "ror", AbsoluteX),
    OP(0x80, "bra", Relative),   OP(0x81, "sta", IndexedIndirect),
    OP(0x84, "sty", ZeroPage),   OP(0x85, "sta", ZeroPage),
    OP(0x86, "stx", ZeroPage),   OP(0x88, "dey", Implied),
    OP(0x89, "bit", Immediate),  OP(0x8a, "txa", Implied),
    OP(0x8c, "sty", Absolute),   OP(0x8d, "sta", Absolute),
    OP(0x8e, "stx", Absolute),   OP(0x90, "bcc", Relative),
    OP(0x91, "sta", IndirectIndexed),
    OP(0x92, "sta", ZeroPageIndirect),
    OP(0x94, "sty", ZeroPageX),  OP(0x95, "sta", ZeroPageX),
    OP(0x96, "stx", ZeroPageY),  OP(0x98, "tya", Implied),
    OP(0x99, "sta", AbsoluteY),  OP(0x9a, "txs", Implied),
    OP(0x9c, "stz", Absolute),   OP(0x9d, "sta", AbsoluteX),
    OP(0x9e, "stz", AbsoluteX),  OP(0xa0, "ldy", Immediate),
    OP(0xa1, "lda", IndexedIndirect),
    OP(0xa2, "ldx", Immediate),  OP(0xa4, "ldy", ZeroPage),
    OP(0xa5, "lda", ZeroPage),   OP(0xa6, "ldx", ZeroPage),
    OP(0xa8, "tay", Implied),    OP(0xa9, "lda", Immediate),
    OP(0xaa, "tax", Implied),    OP(0xac, "ldy", Absolute),
    OP(0xad, "lda", Absolute),   OP(0xae, "ldx", Absolute),
    OP(0xb0, "bcs", Relative),   OP(0xb1, "lda", IndirectIndexed),
    OP(0xb2, "lda", ZeroPageIndirect),
    OP(0xb4, "ldy", ZeroPageX),  OP(0xb5, "lda", ZeroPageX),
    OP(0xb6, "ldx", ZeroPageY),  OP(0xb8, "clv", Implied),
    OP(0xb9, "lda", AbsoluteY),  OP(0xba, "tsx", Implied),
    OP(0xbc, "ldy", AbsoluteX),  OP(0xbd, "lda", AbsoluteX),
    OP(0xbe, "ldx", AbsoluteY),  OP(0xc0, "cpy", Immediate),
    OP(0xc1, "cmp", IndexedIndirect),
    OP(0xc4, "cpy", ZeroPage),   OP(0xc5, "cmp", ZeroPage),
    OP(0xc6, "dec", ZeroPage),   OP(0xc8, "iny", Implied),
    OP(0xc9, "cmp", Immediate),  OP(0xca, "dex", Implied),
    OP(0xcb, "wai", Implied),    OP(0xcc, "cpy", Absolute),
    OP(0xcd, "cmp", Absolute),   OP(0xce, "dec", Absolute),
    OP(0xd0, "bne", Relative),   OP(0xd1, "cmp", IndirectIndexed),
    OP(0xd2, "cmp", ZeroPageIndirect),
    OP(0xd5, "cmp", ZeroPageX),  OP(0xd6, "dec", ZeroPageX),
    OP(0xd8, "cld", Implied),    OP(0xd9, "cmp", AbsoluteY),
    OP(0xda, "phx", Implied),    OP(0xdb, "stp", Implied),
    OP(0xdd, "cmp", AbsoluteX),  OP(0xde, "dec", AbsoluteX),
    OP(0xe0, "cpx", Immediate),  OP(0xe1, "sbc", IndexedIndirect),
    OP(0xe4, "cpx", ZeroPage),   OP(0xe5, "sbc", ZeroPage),
    OP(0xe6, "inc", ZeroPage),   OP(0xe8, "inx", Implied),
    OP(0xe9, "sbc", Immediate),  OP(0xea, "nop", Implied),
    OP(0xec, "cpx", Absolute),   OP(0xed, "sbc", Absolute),
    OP(0xee, "inc", Absolute),   OP(0xf0, "beq", Relative),
    OP(0xf1, "sbc", IndirectIndexed),
    OP(0xf2, "sbc", ZeroPageIndirect),
    OP(0xf5, "sbc", ZeroPageX),  OP(0xf6, "inc", ZeroPageX),
    OP(0xf8, "sed", Implied),    OP(0xf9, "sbc", AbsoluteY),
    OP(0xfa, "plx", Implied),    OP(0xfd, "sbc", AbsoluteX),
    OP(0xfe, "inc", AbsoluteX),
};

#undef OP

static size_t InstructionSize(DAsm6502AddressMode mode) {
  switch (mode) {
    case k6502Implied:
    case k6502Accumulator:
      return 1;
    case k6502Immediate:
    case k6502ZeroPage:
    case k6502ZeroPageX:
    case k6502ZeroPageY:
    case k6502IndexedIndirect:
    case k6502IndirectIndexed:
    case k6502ZeroPageIndirect:
    case k6502Relative:
      return 2;
    case k6502Absolute:
    case k6502AbsoluteX:
    case k6502AbsoluteY:
    case k6502Indirect:
    case k6502AbsoluteIndexedIndirect:
      return 3;
  }
  return 1;
}

bool DAsmDisassemble6502(const void* bytes, size_t length, uint64_t address,
                         DAsmInstruction* inst) {
  if (length == 0) {
    return false;
  }
  const unsigned char* p = bytes;
  const DAsm6502Opcode* opcode = &opcodes[p[0]];
  if (opcode->mnemonic == NULL) {
    DAsmInitInstruction(inst, bytes, length, address, 1);
    DAsmUnknownInstruction(inst, ".byte 0x%02" PRIx64, inst->bytes[0]);
    return true;
  }

  size_t size = InstructionSize(opcode->mode);
  if (length < size) {
    DAsmInitInstruction(inst, bytes, length, address, 1);
    DAsmUnknownInstruction(inst, ".byte 0x%02" PRIx64, inst->bytes[0]);
    return true;
  }
  DAsmInitInstruction(inst, bytes, length, address, size);

  uint16_t operand16 = size == 3 ? DAsmRead16LE(p + 1) : 0;
  switch (opcode->mode) {
    case k6502Implied:
      DAsmFormat(inst, "%s", opcode->mnemonic);
      break;
    case k6502Accumulator:
      DAsmFormat(inst, "%s a", opcode->mnemonic);
      break;
    case k6502Immediate:
      DAsmFormat(inst, "%s #0x%02x", opcode->mnemonic, p[1]);
      break;
    case k6502ZeroPage:
      DAsmFormat(inst, "%s 0x%02x", opcode->mnemonic, p[1]);
      break;
    case k6502ZeroPageX:
      DAsmFormat(inst, "%s 0x%02x,x", opcode->mnemonic, p[1]);
      break;
    case k6502ZeroPageY:
      DAsmFormat(inst, "%s 0x%02x,y", opcode->mnemonic, p[1]);
      break;
    case k6502Absolute:
      DAsmFormat(inst, "%s 0x%04x", opcode->mnemonic, operand16);
      if (p[0] == 0x20 || p[0] == 0x4c) {
        DAsmSetTarget(inst, operand16);
      }
      break;
    case k6502AbsoluteX:
      DAsmFormat(inst, "%s 0x%04x,x", opcode->mnemonic, operand16);
      break;
    case k6502AbsoluteY:
      DAsmFormat(inst, "%s 0x%04x,y", opcode->mnemonic, operand16);
      break;
    case k6502Indirect:
      DAsmFormat(inst, "%s (0x%04x)", opcode->mnemonic, operand16);
      break;
    case k6502IndexedIndirect:
      DAsmFormat(inst, "%s (0x%02x,x)", opcode->mnemonic, p[1]);
      break;
    case k6502IndirectIndexed:
      DAsmFormat(inst, "%s (0x%02x),y", opcode->mnemonic, p[1]);
      break;
    case k6502ZeroPageIndirect:
      DAsmFormat(inst, "%s (0x%02x)", opcode->mnemonic, p[1]);
      break;
    case k6502AbsoluteIndexedIndirect:
      DAsmFormat(inst, "%s (0x%04x,x)", opcode->mnemonic, operand16);
      break;
    case k6502Relative: {
      uint16_t target =
          (uint16_t)(address + 2 + (int8_t)p[1]);
      DAsmFormat(inst, "%s 0x%04x", opcode->mnemonic, target);
      DAsmSetTarget(inst, target);
      break;
    }
  }
  return true;
}
