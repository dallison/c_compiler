#ifndef AARCH64_ENCODE_H
#define AARCH64_ENCODE_H

#include <stdbool.h>
#include <stdint.h>

struct AARCH64Generator;
struct Assembler;

uint32_t AARCH64EncodeMoveWide(bool is_64bit, int opc, uint16_t immediate,
                               int halfword);
uint32_t AARCH64EncodeBranchRegister(int opc, int op3, int reg);
uint32_t AARCH64EncodeReturn(int reg);

bool AARCH64CanDirectEncodeFunction(struct AARCH64Generator* generator);
void AARCH64DirectEncodeFunction(struct AARCH64Generator* generator,
                                 struct Assembler* assembler);

#endif
