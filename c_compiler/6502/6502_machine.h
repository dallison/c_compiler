//
//  6502_machine.h
//  c_compiler
//
//  Created by David Allison on 5/17/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef _6502_machine_h
#define _6502_machine_h

// 6502 Runtime
// -------------
//
// THIS IS OUT OF DATE
//
// Zero page is used to hold sets of registers:
// Single byte registers: 8 bits
// 0x00: b0
// 0x01: b1
// 0x02: b2
// 0x03: b3

// Integer/address registers: 16 bits
// 0x04/0x04: i0
// 0x06/0x07: i1
// 0x08/0x09: i2
// 0x0a/0x0b: i3
//
// Long registers: 32 bits
// 0x0c..0x0f: l0
// 0x19..0x13: l1
// 0x14..0x17: l2
// 0x18..0x1f: l3
//
// Long long registers: 64 bits
// 0x20..0x27: x0
// 0x28..0x2f: x1
// 0x30..0x37: x2
// 0x38..0x3f: x3
//
// Single precision float - 32 bits
// 0x40..0x43: f0
// 0x44..0x47: f1
// 0x48..0x4c: f2
// 0x4c..0x4f: f3
//
// Double precision float - 64 bits
// 0x50..0x57: d0
// 0x58..0x5f: d1
// 0x60..0x67: d2
// 0x68..0x6f: d3
//
// Stack pointer: 16 bits - address of top of stack.
// 0x70/0x71: sp
//
// Frame pointer: 16 bits - address of bottom of stack frame.
// 0x72/0x73: fp
//
// Result address register:
// 0x74/0x75: result

// Temp registers.
// 0x76: t0
// 0x77: t1
//
// For memory pushes to stack
// 0x78, 0x79: mem_src
// 0x7a, 0x7b: mem_dest
// 0x7c, 0x7d: mem_size;

// Page 1 contains the 6502 processor stack.
//
// The program load address is 0x400 (1K) and extends up to the 48K boundary
// The addresses above 48K are ROM, containing common functions provided
// by the operating system and addresses via 16-bit vectors
//
// The main stack starts at the 48K boundary and grows down.  The heap
// starts just above the end of the program and extends up to the
// stack.  When they collide we are out of memory.

#define _6502_NUM_B_REGS 8
#define _6502_NUM_I_REGS 16
#define _6502_NUM_L_REGS 8
#define _6502_NUM_X_REGS 4
#define _6502_NUM_F_REGS 4
#define _6502_NUM_D_REGS 4

#define _6502_B_REG_START 0
#define _6502_I_REG_START (_6502_B_REG_START + _6502_NUM_B_REGS)
#define _6502_L_REG_START (_6502_I_REG_START + _6502_NUM_I_REGS*2)
#define _6502_X_REG_START (_6502_I_REG_START + _6502_NUM_I_REGS*4)
#define _6502_F_REG_START (_6502_X_REG_START + _6502_NUM_X_REGS*8)
#define _6502_D_REG_START (_6502_F_REG_START + _6502_NUM_F_REGS*4)

#define _6502_SP_REG (_6502_D_REG_START + _6502_NUM_D_REGS*8)
#define _6502_FP_REG (_6502_SP_REG + 2)
#define _6502_RESULT_REG (_6502_FP_REG + 2)
#define _6502_T0_REG (_6502_RESULT_REG + 2)
#define _6502_T1_REG (_6502_T0_REG + 1)
#define _6502_T2_REG (_6502_T1_REG + 1)
#define _6502_T3_REG (_6502_T2_REG + 1)
#define _6502_MSRC_REG (_6502_T3_REG + 1)
#define _6502_MDST_REG (_6502_MSRC_REG + 2)
#define _6502_MSZ_REG (_6502_MDST_REG + 2)


// The opcodes are in the first byte.  Most of them fall into the form
//   aaabbbcc
//
// Where aaa is the opcode, bbb is the addressing mode and cc is a selector.
// The valid values for cc are 00, 01 and 10.
//
// The values of aaa and cc are given for each opcode.
// The opcodes that do not fit into the standard form are given full values.

#define _6502_OPCODE(op) k6502Opcode##op
typedef enum {
  // cc= 01
  _6502_OPCODE(ora) = 0 << 5 | 1,
  _6502_OPCODE(and) = 1 << 5 | 1,
  _6502_OPCODE(eor) = 2 << 5 | 1,
  _6502_OPCODE(adc) = 3 << 5 | 1,
  _6502_OPCODE(sta) = 4 << 5 | 1,
  _6502_OPCODE(lda) = 5 << 5 | 1,
  _6502_OPCODE(cmp) = 6 << 5 | 1,
  _6502_OPCODE(sbc) = 7 << 5 | 1,

  // cc= 10
  _6502_OPCODE(asl) = 0 << 5 | 2,
  _6502_OPCODE(rol) = 1 << 5 | 2,
  _6502_OPCODE(lsr) = 2 << 5 | 2,
  _6502_OPCODE(ror) = 3 << 5 | 2,
  _6502_OPCODE(stx) = 4 << 5 | 2,
  _6502_OPCODE(ldx) = 5 << 5 | 2,
  _6502_OPCODE(dec) = 6 << 5 | 2,
  _6502_OPCODE(inc) = 7 << 5 | 2,

  // cc= 00
  _6502_OPCODE(bit) = 1 << 5 | 0,
  _6502_OPCODE(jmp) = 2 << 5 | 0,
  _6502_OPCODE(jmpr) = 3 << 5 | 0,
  _6502_OPCODE(sty) = 4 << 5 | 0,
  _6502_OPCODE(ldy) = 5 << 5 | 0,
  _6502_OPCODE(cpy) = 6 << 5 | 0,
  _6502_OPCODE(cpx) = 7 << 5 | 0,

  // Branches.
  _6502_OPCODE(bpl) = 0x10,
  _6502_OPCODE(bmi) = 0x30,
  _6502_OPCODE(bvc) = 0x50,
  _6502_OPCODE(bvs) = 0x70,
  _6502_OPCODE(bcc) = 0x90,
  _6502_OPCODE(bcs) = 0xb0,
  _6502_OPCODE(bne) = 0xd0,
  _6502_OPCODE(beq) = 0xf0,
  _6502_OPCODE(bra) = 0x80,

  _6502_OPCODE(brk) = 0x00,
  _6502_OPCODE(jsr) = 0x20,
  _6502_OPCODE(rti) = 0x40,
  _6502_OPCODE(rts) = 0x60,

  _6502_OPCODE(php) = 0x08,
  _6502_OPCODE(plp) = 0x28,
  _6502_OPCODE(pha) = 0x48,
  _6502_OPCODE(pla) = 0x68,
  _6502_OPCODE(dey) = 0x88,
  _6502_OPCODE(tay) = 0xa8,
  _6502_OPCODE(iny) = 0xc8,
  _6502_OPCODE(inx) = 0xe8,
  _6502_OPCODE(clc) = 0x18,
  _6502_OPCODE(sec) = 0x38,
  _6502_OPCODE(cli) = 0x58,
  _6502_OPCODE(sei) = 0x78,
  _6502_OPCODE(tya) = 0x98,
  _6502_OPCODE(clv) = 0xb8,
  _6502_OPCODE(cld) = 0xd8,
  _6502_OPCODE(sed) = 0xf8,
  _6502_OPCODE(txa) = 0x8a,
  _6502_OPCODE(txs) = 0x9a,
  _6502_OPCODE(tax) = 0xaa,
  _6502_OPCODE(tsx) = 0xba,
  _6502_OPCODE(dex) = 0xca,
  _6502_OPCODE(nop) = 0xea,
  
  // 65C02
  _6502_OPCODE(inca) = 0x1a,
  _6502_OPCODE(deca) = 0x3a,
  _6502_OPCODE(phy) = 0x5a,
  _6502_OPCODE(ply) = 0x7a,
  _6502_OPCODE(phx) = 0xda,
  _6502_OPCODE(plx) = 0xfa,
} _6502OpcodeValue;

// Addressing modes are dependent on the value of cc.
#define _6502_ADDR_MODE(cc,x) k6502AddrMode_##cc##_##x
typedef enum {
  // cc = 01
  _6502_ADDR_MODE(01, zpx) = 0,
  _6502_ADDR_MODE(01, zp) = 1,
  _6502_ADDR_MODE(01, imm) = 2,
  _6502_ADDR_MODE(01, abs) = 3,
  _6502_ADDR_MODE(01, zpy) = 4,
  _6502_ADDR_MODE(01, zpxa) = 5,
  _6502_ADDR_MODE(01, absy) = 6,
  _6502_ADDR_MODE(01, absx) = 7,
  
  // cc = 10
  _6502_ADDR_MODE(10, imm) = 0,
  _6502_ADDR_MODE(10, zp) = 1,
  _6502_ADDR_MODE(10, acc) = 2,
  _6502_ADDR_MODE(10, abs) = 3,
  _6502_ADDR_MODE(10, zpi) = 4,
  _6502_ADDR_MODE(10, zpx) = 5,
  _6502_ADDR_MODE(10, absx) = 7,

  // cc = 00
  _6502_ADDR_MODE(00, imm) = 0,
  _6502_ADDR_MODE(00, zp) = 1,
  _6502_ADDR_MODE(00, abs) = 3,
  _6502_ADDR_MODE(00, zpx) = 5,
  _6502_ADDR_MODE(00, absx) = 7,
  _6502_ADDR_MODE(00, bit) = 9,
} _6502AddrMode;



#endif /* _6502_machine_h */
