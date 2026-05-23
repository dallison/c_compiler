//
//  p_code_assembler.h
//  c_compiler
//
//  Created by David Allison on 12/30/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef p_code_assembler_h
#define p_code_assembler_h

#include <stdio.h>
#include "assembler.h"
#include "map.h"
#include "p_code_emitter.h"

// PCode instruction format:
// Everything is little endian.

// Top 2 bits are used to encode the instruction length.
// If the top bit is zero then the instruction is 32 bits
// long.
// If the top bit is one then bit 30 is used to encode
// the length.
//
// Bit 30 = 0 -> 64 bit.
// Bit 30 = 1 -> 96 bit.

// ALU instructions (all reg operands) (32 bits)
// +---+---------+-------------+-------------+-----------+
// | 0 |  opcode  |  dest       |   src1      |  src2     |
// +---+---------+-------------+-------------+-----------+
//         7 bits     8 bits        8 bits      8 bits

// Load and store (64 bits)
// +----+---------+-------------+-------------+-----------+
// | 10 | opcode  |  dest       |   src       |  unused   |
// +----+---------+-------------+-------------+-----------+
//         6 bits     8 bits        8 bits      8 bits
// +------------------------------------------------------+
// |                  offset                              |
// +------------------------------------------------------+
//                      32 bits

// Constant operand (movc, addc, etc.) (64 bits)
// +----+---------+-------------+-------------+-----------+
// | 10 | opcode  |  dest       |   unused    |  unused   |
// +----+---------+-------------+-------------+-----------+
//         6 bits     8 bits        8 bits      8 bits
// +------------------------------------------------------+
// |                  constant                            |
// +------------------------------------------------------+
//                       32 bits

// Constant operand movxc and movdc (96 bits)
// +----+---------+-------------+-------------+-----------+
// | 11 | opcode  |  dest       |   unused    |  unused   |
// +----+---------+-------------+-------------+-----------+
//        6 bits     8 bits        8 bits      8 bits
// +------------------------------------------------------+
// |                  constant (low word)                 |
// +------------------------------------------------------+
//                       32 bits
// +------------------------------------------------------+
// |                  constant  (high word)               |
// +------------------------------------------------------+
//                       32 bits

// Constant operand addc (64 bits)
// +----+---------+-------------+-------------+-----------+
// | 10 | opcode  |  dest       |   src       |  unused   |
// +----+---------+-------------+-------------+-----------+
//        6 bits     8 bits        8 bits      8 bits
// +------------------------------------------------------+
// |                  constant (low word)                 |
// +------------------------------------------------------+
//                       32 bits

// Call (96 bits)
// +----+---------+-------------+-------------+-----------+
// | 11 | opcode  |  unused     |   unused    |  unused   |
// +----+---------+-------------+-------------+-----------+
//         6 bits     8 bits        8 bits      8 bits
// +------------------------------------------------------+
// |                  address (low word)                  |
// +------------------------------------------------------+
//                       32 bits
// +------------------------------------------------------+
// |                  address (high word)                 |
// +------------------------------------------------------+
//                       32 bits

// Stack pointer manipulation (decsp and incsp):
// +---+---------+---------------------------------------+
// | 0 | opcode  |          num bytes                    |
// +---+---------+---------------------------------------+
//         7 bits            24 bits

typedef struct {
  Assembler base;
  Map instructions;
  int32_t bss;  // Section id for .bss section.
} PCodeAssembler;

bool PCodeAssemblerInit(PCodeAssembler* assembler, String* infile,
                        String* outfile);
PCodeAssembler* NewPCodeAssembler(String* infile, String* outfile);
void PCodeAssemblerDestruct(PCodeAssembler* assembler);
void PCodeAssemblerDelete(PCodeAssembler* assembler);
void AssemblePCodeInstruction(Assembler* assembler, String* word);

#endif /* p_code_assembler_h */
