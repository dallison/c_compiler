//
//  6502_machine.h
//  c_compiler
//
//  Created by David Allison on 5/17/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef W65C02_machine_h
#define W65C02_machine_h

// 6502 Runtime
// -------------
//
// Zero page layout (offsets relative to regs_start / REG_BASE):
//
// 0x00..0x3f: Unified register file (8 x 64-bit slots, 64 bytes).
//   All integer and floating-point virtual registers share this space;
//   the register allocator tracks byte-level occupancy.
//
// 0x40/0x41: __sp   (16-bit stack pointer)
// 0x42/0x43: __fp   (16-bit frame pointer)
// 0x44/0x45: __result
// 0x46: __t0
// 0x47: __t1
// 0x48: __t2
// 0x49: __t3
// 0x4a/0x4b: __mem_src
// 0x4c/0x4d: __mem_dest
// 0x4e/0x4f: __mem_size
//
// Math/fp scratch follows immediately (see vars.s):
// 0x50..0x59: mt1 (10 bytes)
// 0x5a..0x5f: mt2 (6 bytes)
// 0x60..0x65: mt3 (6 bytes)
// 0x66..0x7d: fscratch (24 bytes)
// 0x7e: os_scratch_start

// Page 1 contains the 6502 processor stack.
//
// The program load address is 0x400 (1K) and extends up to the 48K boundary
// The addresses above 48K are ROM, containing common functions provided
// by the operating system and addresses via 16-bit vectors
//
// The main stack starts at the 48K boundary and grows down.  The heap
// starts just above the end of the program and extends up to the
// stack.  When they collide we are out of memory.

// Floating point double registers are the same as single precision regs.
#define W65C02_NUM_TEMP_B_REGS 2
#define W65C02_NUM_PRESERVED_B_REGS 6
#define W65C02_NUM_B_REGS (W65C02_NUM_TEMP_B_REGS + W65C02_NUM_PRESERVED_B_REGS)

#define W65C02_NUM_TEMP_I_REGS 4
#define W65C02_NUM_PRESERVED_I_REGS 12
#define W65C02_NUM_I_REGS (W65C02_NUM_TEMP_I_REGS + W65C02_NUM_PRESERVED_I_REGS)

#define W65C02_NUM_TEMP_L_REGS 2
#define W65C02_NUM_PRESERVED_L_REGS 6
#define W65C02_NUM_L_REGS (W65C02_NUM_TEMP_L_REGS + W65C02_NUM_PRESERVED_L_REGS)

#define W65C02_NUM_TEMP_X_REGS 1
#define W65C02_NUM_PRESERVED_X_REGS 3
#define W65C02_NUM_X_REGS (W65C02_NUM_TEMP_X_REGS + W65C02_NUM_PRESERVED_X_REGS)

#define W65C02_NUM_TEMP_F_REGS 1
#define W65C02_NUM_PRESERVED_F_REGS 3
#define W65C02_NUM_F_REGS (W65C02_NUM_TEMP_F_REGS + W65C02_NUM_PRESERVED_F_REGS)

#define W65C02_NUM_SLOTS 8
#define W65C02_SLOT_SIZE 8
#define W65C02_REG_FILE_BYTES (W65C02_NUM_SLOTS * W65C02_SLOT_SIZE)

#define W65C02_SP_REG W65C02_REG_FILE_BYTES
#define W65C02_FP_REG (W65C02_SP_REG + 2)
#define W65C02_RESULT_REG (W65C02_FP_REG + 2)
#define W65C02_T0_REG (W65C02_RESULT_REG + 2)
#define W65C02_T1_REG (W65C02_T0_REG + 1)
#define W65C02_T2_REG (W65C02_T1_REG + 1)
#define W65C02_T3_REG (W65C02_T2_REG + 1)
#define W65C02_MSRC_REG (W65C02_T3_REG + 1)
#define W65C02_MDST_REG (W65C02_MSRC_REG + 2)
#define W65C02_MSZ_REG (W65C02_MDST_REG + 2)
#define W65C02_ZP_RUNTIME_END (W65C02_MSZ_REG + 2)

#define W65C02_MT1_REG W65C02_ZP_RUNTIME_END
#define W65C02_MT1_BYTES 10
#define W65C02_MT2_REG (W65C02_MT1_REG + W65C02_MT1_BYTES)
#define W65C02_MT2_BYTES 6
#define W65C02_MT3_REG (W65C02_MT2_REG + W65C02_MT2_BYTES)
#define W65C02_MT3_BYTES 6
#define W65C02_FSCRATCH_REG (W65C02_MT3_REG + W65C02_MT3_BYTES)
#define W65C02_FSCRATCH_BYTES 24
#define W65C02_FSCRATCH_END (W65C02_FSCRATCH_REG + W65C02_FSCRATCH_BYTES)
#define W65C02_OS_SCRATCH_REG W65C02_FSCRATCH_END
#define W65C02_ZP_LAYOUT_END W65C02_OS_SCRATCH_REG

#define W65C02_ENTER_SAVE_MASK_BYTES 8


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
