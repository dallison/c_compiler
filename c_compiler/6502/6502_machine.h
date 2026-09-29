//
//  6502_machine.h
//  c_compiler
//
//  Created by David Allison on 5/17/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef W65C02_machine_h
#define W65C02_machine_h

// Zero page register file.  The C ABI occupies 0x00..0x8f.  0x90..0xff is
// ROM and interrupt scratch.
//
//   0x00..0x05  b0..b5     8-bit
//   0x06..0x15  i0..i7     16-bit int / pointer
//   0x16..0x25  l0..l3     32-bit long
//   0x26..0x3d  x0..x2     64-bit long long
//   0x3e..0x4d  f0..f3     32-bit float (double uses these same slots)
//   0x4e..0x4f  __sp
//   0x50..0x51  __fp
//   0x52..0x53  __result
//   0x54..0x57  __t0..__t3
//   0x58..0x77  mt1..mt3 and __mem_* (overlap)
//   0x78..0x8f  float unpack scratch
//
// There is no separate double register file.  double is binary32.

#define W65C02_NUM_TEMP_B_REGS 2
#define W65C02_NUM_PRESERVED_B_REGS 4
#define W65C02_NUM_B_REGS (W65C02_NUM_TEMP_B_REGS + W65C02_NUM_PRESERVED_B_REGS)

#define W65C02_NUM_TEMP_I_REGS 4
#define W65C02_NUM_PRESERVED_I_REGS 4
#define W65C02_NUM_I_REGS (W65C02_NUM_TEMP_I_REGS + W65C02_NUM_PRESERVED_I_REGS)

#define W65C02_NUM_TEMP_L_REGS 2
#define W65C02_NUM_PRESERVED_L_REGS 2
#define W65C02_NUM_L_REGS (W65C02_NUM_TEMP_L_REGS + W65C02_NUM_PRESERVED_L_REGS)

#define W65C02_NUM_TEMP_X_REGS 1
#define W65C02_NUM_PRESERVED_X_REGS 2
#define W65C02_NUM_X_REGS (W65C02_NUM_TEMP_X_REGS + W65C02_NUM_PRESERVED_X_REGS)

#define W65C02_NUM_TEMP_F_REGS 1
#define W65C02_NUM_PRESERVED_F_REGS 3
#define W65C02_NUM_F_REGS (W65C02_NUM_TEMP_F_REGS + W65C02_NUM_PRESERVED_F_REGS)

#define W65C02_B_REG_START 0
#define W65C02_I_REG_START (W65C02_B_REG_START + W65C02_NUM_B_REGS)
#define W65C02_L_REG_START (W65C02_I_REG_START + W65C02_NUM_I_REGS*2)
#define W65C02_X_REG_START (W65C02_L_REG_START + W65C02_NUM_L_REGS*4)
#define W65C02_F_REG_START (W65C02_X_REG_START + W65C02_NUM_X_REGS*8)

#define W65C02_SP_REG (W65C02_F_REG_START + W65C02_NUM_F_REGS*4)
#define W65C02_FP_REG (W65C02_SP_REG + 2)
#define W65C02_RESULT_REG (W65C02_FP_REG + 2)
#define W65C02_T0_REG (W65C02_RESULT_REG + 2)
#define W65C02_T1_REG (W65C02_T0_REG + 1)
#define W65C02_T2_REG (W65C02_T1_REG + 1)
#define W65C02_T3_REG (W65C02_T2_REG + 1)
#define W65C02_MSRC_REG (W65C02_T3_REG + 1)
#define W65C02_MDST_REG (W65C02_MSRC_REG + 2)
#define W65C02_MSZ_REG (W65C02_MDST_REG + 2)

// Integer mul/div scratch starts at __mem_src.  Float unpack follows it.
// Together they must finish at or before W65C02_ZP_LIMIT.
#define W65C02_MATH_SCRATCH_BYTES 32
#define W65C02_FLOAT_SCRATCH_BYTES 24
#define W65C02_ZP_LIMIT 0x90
#define W65C02_ABI_END \
  (W65C02_MSRC_REG + W65C02_MATH_SCRATCH_BYTES + W65C02_FLOAT_SCRATCH_BYTES)


// The opcodes are in the first byte.  Most of them fall into the form
//   aaabbbcc
//
// Where aaa is the opcode, bbb is the addressing mode and cc is a selector.
// The valid values for cc are 00, 01 and 10.
//
// The values of aaa and cc are given for each opcode.
// The opcodes that do not fit into the standard form are given full values.

#define W65C02_OPCODE(op) k6502Opcode##op
typedef enum {
  // cc= 01
  W65C02_OPCODE(ora) = 0 << 5 | 1,
  W65C02_OPCODE(and) = 1 << 5 | 1,
  W65C02_OPCODE(eor) = 2 << 5 | 1,
  W65C02_OPCODE(adc) = 3 << 5 | 1,
  W65C02_OPCODE(sta) = 4 << 5 | 1,
  W65C02_OPCODE(lda) = 5 << 5 | 1,
  W65C02_OPCODE(cmp) = 6 << 5 | 1,
  W65C02_OPCODE(sbc) = 7 << 5 | 1,

  // cc= 10
  W65C02_OPCODE(asl) = 0 << 5 | 2,
  W65C02_OPCODE(rol) = 1 << 5 | 2,
  W65C02_OPCODE(lsr) = 2 << 5 | 2,
  W65C02_OPCODE(ror) = 3 << 5 | 2,
  W65C02_OPCODE(stx) = 4 << 5 | 2,
  W65C02_OPCODE(ldx) = 5 << 5 | 2,
  W65C02_OPCODE(dec) = 6 << 5 | 2,
  W65C02_OPCODE(inc) = 7 << 5 | 2,

  // cc= 00
  W65C02_OPCODE(bit) = 1 << 5 | 0,
  W65C02_OPCODE(jmp) = 2 << 5 | 0,
  W65C02_OPCODE(jmpr) = 3 << 5 | 0,
  W65C02_OPCODE(sty) = 4 << 5 | 0,
  W65C02_OPCODE(ldy) = 5 << 5 | 0,
  W65C02_OPCODE(cpy) = 6 << 5 | 0,
  W65C02_OPCODE(cpx) = 7 << 5 | 0,

  // Branches.
  W65C02_OPCODE(bpl) = 0x10,
  W65C02_OPCODE(bmi) = 0x30,
  W65C02_OPCODE(bvc) = 0x50,
  W65C02_OPCODE(bvs) = 0x70,
  W65C02_OPCODE(bcc) = 0x90,
  W65C02_OPCODE(bcs) = 0xb0,
  W65C02_OPCODE(bne) = 0xd0,
  W65C02_OPCODE(beq) = 0xf0,
  W65C02_OPCODE(bra) = 0x80,

  W65C02_OPCODE(brk) = 0x00,
  W65C02_OPCODE(jsr) = 0x20,
  W65C02_OPCODE(rti) = 0x40,
  W65C02_OPCODE(rts) = 0x60,

  W65C02_OPCODE(php) = 0x08,
  W65C02_OPCODE(plp) = 0x28,
  W65C02_OPCODE(pha) = 0x48,
  W65C02_OPCODE(pla) = 0x68,
  W65C02_OPCODE(dey) = 0x88,
  W65C02_OPCODE(tay) = 0xa8,
  W65C02_OPCODE(iny) = 0xc8,
  W65C02_OPCODE(inx) = 0xe8,
  W65C02_OPCODE(clc) = 0x18,
  W65C02_OPCODE(sec) = 0x38,
  W65C02_OPCODE(cli) = 0x58,
  W65C02_OPCODE(sei) = 0x78,
  W65C02_OPCODE(tya) = 0x98,
  W65C02_OPCODE(clv) = 0xb8,
  W65C02_OPCODE(cld) = 0xd8,
  W65C02_OPCODE(sed) = 0xf8,
  W65C02_OPCODE(txa) = 0x8a,
  W65C02_OPCODE(txs) = 0x9a,
  W65C02_OPCODE(tax) = 0xaa,
  W65C02_OPCODE(tsx) = 0xba,
  W65C02_OPCODE(dex) = 0xca,
  W65C02_OPCODE(nop) = 0xea,
  
  // 65C02
  W65C02_OPCODE(inca) = 0x1a,
  W65C02_OPCODE(deca) = 0x3a,
  W65C02_OPCODE(phy) = 0x5a,
  W65C02_OPCODE(ply) = 0x7a,
  W65C02_OPCODE(phx) = 0xda,
  W65C02_OPCODE(plx) = 0xfa,
} W65C02OpcodeValue;

// Addressing modes are dependent on the value of cc.
#define W65C02_ADDR_MODE(cc,x) k6502AddrMode_##cc##_##x
typedef enum {
  // cc = 01
  W65C02_ADDR_MODE(01, zpx) = 0,
  W65C02_ADDR_MODE(01, zp) = 1,
  W65C02_ADDR_MODE(01, imm) = 2,
  W65C02_ADDR_MODE(01, abs) = 3,
  W65C02_ADDR_MODE(01, zpy) = 4,
  W65C02_ADDR_MODE(01, zpxa) = 5,
  W65C02_ADDR_MODE(01, absy) = 6,
  W65C02_ADDR_MODE(01, absx) = 7,
  
  // cc = 10
  W65C02_ADDR_MODE(10, imm) = 0,
  W65C02_ADDR_MODE(10, zp) = 1,
  W65C02_ADDR_MODE(10, acc) = 2,
  W65C02_ADDR_MODE(10, abs) = 3,
  W65C02_ADDR_MODE(10, zpi) = 4,
  W65C02_ADDR_MODE(10, zpx) = 5,
  W65C02_ADDR_MODE(10, absx) = 7,

  // cc = 00
  W65C02_ADDR_MODE(00, imm) = 0,
  W65C02_ADDR_MODE(00, zp) = 1,
  W65C02_ADDR_MODE(00, abs) = 3,
  W65C02_ADDR_MODE(00, zpx) = 5,
  W65C02_ADDR_MODE(00, absx) = 7,
  W65C02_ADDR_MODE(00, bit) = 9,
} W65C02AddrMode;



#endif /* W65C02_machine_h */
