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
  OP(divu) = 10,
  OP(divf) = 11,
  OP(divd) = 12,
  OP(mod) = 13,
  OP(modu) = 14,
  OP(lsr) = 15,
  OP(asr) = 16,
  OP(lsl) = 17,
  OP(or) = 18,
  OP(and) = 19,
  OP(xor) = 20,
  OP(not) = 21,
  OP(inv) = 22,
  OP(neg) = 23,
  OP(negf) = 24,
  OP(negd) = 25,
  OP(cmpeq) = 26,
  OP(cmpne) = 27,
  OP(cmplt) = 28,
  OP(cmple) = 29,
  OP(cmpgt) = 30,
  OP(cmpge) = 31,
  OP(cmpltu) = 32,
  OP(cmpleu) = 33,
  OP(cmpgtu) = 34,
  OP(cmpgeu) = 35,
  OP(cmpeqf) = 36,
  OP(cmpnef) = 37,
  OP(cmpltf) = 38,
  OP(cmplef) = 39,
  OP(cmpgtf) = 40,
  OP(cmpgef) = 41,
  OP(cmpeqd) = 42,
  OP(cmpned) = 43,
  OP(cmpltd) = 44,
  OP(cmpled) = 45,
  OP(cmpgtd) = 46,
  OP(cmpged) = 47,
  OP(decsp) = 48,
  OP(incsp) = 49,
  OP(push) = 50,
  OP(pushf) = 51,
  OP(pushd) = 52,
  OP(pushx) = 53,
  OP(pop) = 54,
  OP(popf) = 55,
  OP(popd) = 56,
  OP(popx) = 57,
  OP(mov) = 58,
  OP(movf) = 59,
  OP(movd) = 60,
  OP(ret) = 61,
  OP(cbra) = 62,
  OP(i2f) = 63,
  OP(i2d) = 64,
  OP(ui2f) = 65,
  OP(ui2d) = 66,
  OP(f2d) = 67,
  OP(d2f) = 68,
  OP(f2i) = 69,
  OP(d2i) = 70,
  OP(f2ui) = 71,
  OP(d2ui) = 72,
  OP(rcall) = 73,
  OP(esc) = 74,

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
  OP(cjmp) = 4,
  OP(adr) = 5,
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
#define PCODE_TMP_REG 29  // Unsaved temp reg.

#endif /* p_code_opcodes_h */
