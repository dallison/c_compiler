#ifndef P_CODE_ENCODING_H
#define P_CODE_ENCODING_H

#include <stdint.h>

uint32_t PCodeEncodeRegisters32(int opcode, int reg0, int reg1, int reg2);
uint32_t PCodeEncodeRegister32(int opcode, int reg);
uint32_t PCodeEncodeImmediate24(int opcode, int64_t value);
uint32_t PCodeEncodeRegisters64(int opcode, int reg0, int reg1);
uint32_t PCodeEncodeRegister64(int opcode, int reg);
uint32_t PCodeEncodeRegister96(int opcode, int reg);
uint32_t PCodeEncodeOpcode32(int opcode);
uint32_t PCodeEncodeOpcode64(int opcode);
uint32_t PCodeEncodeOpcode96(int opcode);

#endif
