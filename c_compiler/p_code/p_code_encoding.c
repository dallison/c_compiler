#include "p_code_encoding.h"

uint32_t PCodeEncodeRegisters32(int opcode, int reg0, int reg1, int reg2) {
  return (uint32_t)opcode << 24 | (uint32_t)reg0 << 16 |
         (uint32_t)reg1 << 8 | (uint32_t)reg2;
}

uint32_t PCodeEncodeRegister32(int opcode, int reg) {
  return PCodeEncodeRegisters32(opcode, reg, 0, 0);
}

uint32_t PCodeEncodeImmediate24(int opcode, int64_t value) {
  return (uint32_t)opcode << 24 | ((uint32_t)value & UINT32_C(0x00ffffff));
}

uint32_t PCodeEncodeRegisters64(int opcode, int reg0, int reg1) {
  return UINT32_C(0x80000000) | (uint32_t)opcode << 24 |
         (uint32_t)reg0 << 16 | (uint32_t)reg1 << 8;
}

uint32_t PCodeEncodeRegister64(int opcode, int reg) {
  return PCodeEncodeRegisters64(opcode, reg, 0);
}

uint32_t PCodeEncodeRegister96(int opcode, int reg) {
  return UINT32_C(0xc0000000) | (uint32_t)opcode << 24 |
         (uint32_t)reg << 16;
}

uint32_t PCodeEncodeOpcode32(int opcode) {
  return (uint32_t)opcode << 24;
}

uint32_t PCodeEncodeOpcode64(int opcode) {
  return UINT32_C(0x80000000) | (uint32_t)opcode << 24;
}

uint32_t PCodeEncodeOpcode96(int opcode) {
  return UINT32_C(0xc0000000) | (uint32_t)opcode << 24;
}
