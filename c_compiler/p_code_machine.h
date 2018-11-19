//
//  p_code_opcodes.h
//  c_compiler
//
//  Created by David Allison on 1/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef p_code_opcodes_h
#define p_code_opcodes_h

// These are the opcodes for the P-Code instructions.  They are used
// by the assembler and the interpreter.

#define OP(op) kCodeInst##op
typedef enum {
  // 32 bit instructions.
  OP(add) = 0,
  OP(sub) = 1,
  OP(addf) = 2,
  OP(addd) = 3,
  OP(subf) = 4,
  OP(subd) = 5,
  OP(mul) = 6,
  OP(mulf) = 7,
  OP(muld) = 8,
  OP(div) = 9,
  OP(divf) = 10,
  OP(divd) = 11,
  OP(mod) = 12,
  OP(lsr) = 13,
  OP(asr) = 14,
  OP(lsl) = 15,
  OP(or) = 16,
  OP(and) = 17,
  OP(xor) = 18,
  OP(not) = 19,
  OP(inv) = 20,
  OP(neg) = 21,
  OP(negf) = 22,
  OP(negd) = 23,
  OP(cmpeq) = 24,
  OP(cmpne) = 25,
  OP(cmplt) = 26,
  OP(cmple) = 27,
  OP(cmpgt) = 28,
  OP(cmpge) = 29,
  OP(cmpeqf) = 30,
  OP(cmpnef) = 31,
  OP(cmpltf) = 32,
  OP(cmplef) = 33,
  OP(cmpgtf) = 34,
  OP(cmpgef) = 35,
  OP(cmpeqd) = 36,
  OP(cmpned) = 37,
  OP(cmpltd) = 38,
  OP(cmpled) = 39,
  OP(cmpgtd) = 40,
  OP(cmpged) = 41,
  OP(decsp) = 42,
  OP(incsp) = 43,
  OP(push) = 44,
  OP(pushf) = 45,
  OP(pushd) = 46,
  OP(pushx) = 47,
  OP(pop) = 48,
  OP(popf) = 49,
  OP(popd) = 50,
  OP(popx) = 51,
  OP(mov) = 52,
  OP(movf) = 53,
  OP(movd) = 54,
  OP(ret) = 55,
  OP(cbra) = 56,
  OP(i2f) = 57,
  OP(i2d) = 58,
  OP(f2d) = 59,
  OP(d2f) = 60,
  OP(f2i) = 61,
  OP(d2i) = 62,
  OP(rcall) = 63,
  OP(esc) = 64,

  // 64 bit instructions.
  OP(ldw) = 0,
  OP(ldh) = 1,
  OP(ldb) = 2,
  OP(lduw) = 3,
  OP(ldub) = 4,
  OP(lduh) = 5,
  OP(ldx) = 6,
  OP(ldf) = 7,
  OP(ldd) = 8,
  OP(stw) = 9,
  OP(sth) = 10,
  OP(stx) = 11,
  OP(stf) = 12,
  OP(std) = 13,
  OP(stb) = 14,

  OP(movc) = 15,
  OP(movfc) = 16,

  OP(bz) = 17,
  OP(bnz) = 18,
  OP(bra) = 19,
  OP(addc) = 20,

  // 96 bit instructions.
  OP(movdc) = 0,
  OP(movxc) = 1,
  OP(jmp) = 2,
  OP(call) = 3,
} PCodeInstOpcode;

// How many of each register we have.  Since this isn't a real processor
// we can have as many as will fit into the bit field allocated for register
// number.  We have allocated 8 bits so this gives us 256 possible registers.
//
// Why so many?  We can avoid the complexity of spilling registers if can
// never run out of them.  With 256 registers it is almost impossible to run
// out.
#define PCODE_NUM_INT_REGS 256
#define PCODE_NUM_FLOAT_REGS 256
#define PCODE_NUM_DOUBLE_REGS 256

#define PCODE_PC_REG 32  // Program counter.
#define PCODE_FP_REG 31  // Frame pointer.
#define PCODE_SP_REG 30  // Stack pointer.
#define PCODE_AP_REG 28  // Argument pointer.

#endif /* p_code_opcodes_h */
