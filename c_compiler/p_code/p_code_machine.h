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

#define PCODE_OP(op) kCodeInst##op
typedef enum {
  // 32 bit instructions.
  PCODE_OP(add) = 0,
  PCODE_OP(sub) = 1,
  PCODE_OP(addf) = 2,
  PCODE_OP(addd) = 3,
  PCODE_OP(subf) = 4,
  PCODE_OP(subd) = 5,
  PCODE_OP(mul) = 6,
  PCODE_OP(mulf) = 7,
  PCODE_OP(muld) = 8,
  PCODE_OP(div) = 9,
  PCODE_OP(divu) = 10,
  PCODE_OP(divf) = 11,
  PCODE_OP(divd) = 12,
  PCODE_OP(mod) = 13,
  PCODE_OP(modu) = 14,
  PCODE_OP(lsr) = 15,
  PCODE_OP(asr) = 16,
  PCODE_OP(lsl) = 17,
  PCODE_OP(or) = 18,
  PCODE_OP(and) = 19,
  PCODE_OP(xor) = 20,
  PCODE_OP(not) = 21,
  PCODE_OP(inv) = 22,
  PCODE_OP(neg) = 23,
  PCODE_OP(negf) = 24,
  PCODE_OP(negd) = 25,
  PCODE_OP(cmpeq) = 26,
  PCODE_OP(cmpne) = 27,
  PCODE_OP(cmplt) = 28,
  PCODE_OP(cmple) = 29,
  PCODE_OP(cmpgt) = 30,
  PCODE_OP(cmpge) = 31,
  PCODE_OP(cmpltu) = 32,
  PCODE_OP(cmpleu) = 33,
  PCODE_OP(cmpgtu) = 34,
  PCODE_OP(cmpgeu) = 35,
  PCODE_OP(cmpeqf) = 36,
  PCODE_OP(cmpnef) = 37,
  PCODE_OP(cmpltf) = 38,
  PCODE_OP(cmplef) = 39,
  PCODE_OP(cmpgtf) = 40,
  PCODE_OP(cmpgef) = 41,
  PCODE_OP(cmpeqd) = 42,
  PCODE_OP(cmpned) = 43,
  PCODE_OP(cmpltd) = 44,
  PCODE_OP(cmpled) = 45,
  PCODE_OP(cmpgtd) = 46,
  PCODE_OP(cmpged) = 47,
  PCODE_OP(decsp) = 48,
  PCODE_OP(incsp) = 49,
  PCODE_OP(push) = 50,
  PCODE_OP(pushf) = 51,
  PCODE_OP(pushd) = 52,
  PCODE_OP(pushx) = 53,
  PCODE_OP(pop) = 54,
  PCODE_OP(popf) = 55,
  PCODE_OP(popd) = 56,
  PCODE_OP(popx) = 57,
  PCODE_OP(mov) = 58,
  PCODE_OP(movf) = 59,
  PCODE_OP(movd) = 60,
  PCODE_OP(ret) = 61,
  PCODE_OP(cbra) = 62,
  PCODE_OP(i2f) = 63,
  PCODE_OP(i2d) = 64,
  PCODE_OP(ui2f) = 65,
  PCODE_OP(ui2d) = 66,
  PCODE_OP(f2d) = 67,
  PCODE_OP(d2f) = 68,
  PCODE_OP(f2i) = 69,
  PCODE_OP(d2i) = 70,
  PCODE_OP(f2ui) = 71,
  PCODE_OP(d2ui) = 72,
  PCODE_OP(rcall) = 73,
  PCODE_OP(esc) = 74,

  // 64 bit instructions.
  PCODE_OP(ldw) = 0,
  PCODE_OP(ldh) = 1,
  PCODE_OP(ldb) = 2,
  PCODE_OP(lduw) = 3,
  PCODE_OP(ldub) = 4,
  PCODE_OP(lduh) = 5,
  PCODE_OP(ldx) = 6,
  PCODE_OP(ldf) = 7,
  PCODE_OP(ldd) = 8,
  PCODE_OP(stw) = 9,
  PCODE_OP(sth) = 10,
  PCODE_OP(stx) = 11,
  PCODE_OP(stf) = 12,
  PCODE_OP(std) = 13,
  PCODE_OP(stb) = 14,

  PCODE_OP(movc) = 15,
  PCODE_OP(movfc) = 16,

  PCODE_OP(bz) = 17,
  PCODE_OP(bnz) = 18,
  PCODE_OP(bra) = 19,
  PCODE_OP(addc) = 20,

  // 96 bit instructions.
  PCODE_OP(movdc) = 0,
  PCODE_OP(movxc) = 1,
  PCODE_OP(jmp) = 2,
  PCODE_OP(call) = 3,
  PCODE_OP(cjmp) = 4,
  PCODE_OP(adr) = 5,
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
#define PCODE_AP_REG 33  // Argument pointer.
#define PCODE_TP_REG 34  // Thread pointer.

// Unsaved temporaries.
#define PCODE_TMP1_REG 26  // Unsaved temp reg.
#define PCODE_TMP2_REG 27  // Unsaved temp reg.
#define PCODE_TMP3_REG 28  // Unsaved temp reg.
#define PCODE_TMP4_REG 29  // Unsaved temp reg.

#endif /* p_code_opcodes_h */
