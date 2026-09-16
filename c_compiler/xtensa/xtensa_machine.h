//
//  xtensa_machine.h
//  c_compiler
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef xtensa_machine_h
#define xtensa_machine_h

#define XTENSA_NUM_INT_REGS 16
#define XTENSA_NUM_FLOAT_REGS 16

// Xtensa has no architectural zero register.  a0 is reserved here so the
// inherited zero pseudo can be recognized and materialized by the emitter.
#define XTENSA_INT_ZERO_REG 0

// Incoming integer values and function return values use a2-a7.  CALL8
// outgoing values use a10-a15; the emitter performs that windowed-ABI mapping.
#define XTENSA_INT_RETURN_VALUE_0 2
#define XTENSA_INT_RETURN_VALUE_1 3

#define XTENSA_INT_ARG_START 2
#define XTENSA_INT_ARG_END 7
#define XTENSA_NUM_INT_ARGS (XTENSA_INT_ARG_END - XTENSA_INT_ARG_START + 1)

// The initial backend does not expose a hard-float calling convention.  These
// ranges describe the hardware FP register file for later scalar-FP lowering.
#define XTENSA_FP_ARG_START 2
#define XTENSA_FP_ARG_END 7
#define XTENSA_NUM_FP_ARGS (XTENSA_FP_ARG_END - XTENSA_FP_ARG_START + 1)

// Register windows preserve a2-a7 across CALL8.  There is no conventional
// callee-save sequence in the windowed ABI.
#define XTENSA_INT_SAVED_START_1 2
#define XTENSA_INT_SAVED_END_1 7
#define XTENSA_INT_SAVED_START_2 2
#define XTENSA_INT_SAVED_END_2 1

#define XTENSA_INT_TEMP_START_1 8
#define XTENSA_INT_TEMP_END_1 10
#define XTENSA_INT_TEMP_START_2 11
#define XTENSA_INT_TEMP_END_2 14

// Register reserved for spill address calculation if necessary.
// This is the Global Pointer register in the ABI that appears
// not to be needed.
#define XTENSA_SPILL_ADDR 8

#define XTENSA_FP_RETURN_VALUE_0 0
#define XTENSA_FP_RETURN_VALUE_1 1

#define XTENSA_FP_SAVED_START_1 8
#define XTENSA_FP_SAVED_END_1 15
#define XTENSA_FP_SAVED_START_2 1
#define XTENSA_FP_SAVED_END_2 0

#define XTENSA_FP_TEMP_START_1 0
#define XTENSA_FP_TEMP_END_1 7
#define XTENSA_FP_TEMP_START_2 12
#define XTENSA_FP_TEMP_END_2 15

#define XTENSA_SP_REG 1   // Stack pointer.
#define XTENSA_FP_REG 1   // Fixed frames are addressed from the stack pointer.
#define XTENSA_RET_REG 0  // Return address.

// Size of the stack frame header containing saved ra and s0.
#define XTENSA_STACK_FRAME_HEADER_SIZE 16

// Register variables: variables that are in registers rather than being on
// the stack.

// First and last register numbers for int register variables.
#define XTENSA_FIRST_INT_REG_VAR XTENSA_INT_SAVED_START_2
#define XTENSA_LAST_INT_REG_VAR XTENSA_INT_SAVED_END_2

#define XTENSA_FIRST_LEAF_INT_REG_VAR XTENSA_INT_TEMP_START_2
#define XTENSA_LAST_LEAF_INT_REG_VAR XTENSA_INT_TEMP_END_2

// First and last register numbers for floating point register variables.
#define XTENSA_FIRST_FP_REG_VAR XTENSA_FP_SAVED_START_2
#define XTENSA_LAST_FP_REG_VAR XTENSA_FP_SAVED_END_2
#define XTENSA_FIRST_LEAF_FP_REG_VAR XTENSA_FP_TEMP_START_2
#define XTENSA_LAST_LEAF_FP_REG_VAR XTENSA_FP_TEMP_END_2

// Instruction encodings.

// Opcode values.
#define XTENSA_OPCODE(op) kXTENSAInstOpcode_##op
typedef enum {
  XTENSA_OPCODE(op) = 0x33,
  XTENSA_OPCODE(op_imm) = 0x13,
  XTENSA_OPCODE(lui) = 0x37,
  XTENSA_OPCODE(auipc) = 0x17,
  XTENSA_OPCODE(jal) = 0x6f,
  XTENSA_OPCODE(jalr) = 0x67,
  XTENSA_OPCODE(branch) = 0x63,
  XTENSA_OPCODE(load) = 0x03,
  XTENSA_OPCODE(store) = 0x23,
  XTENSA_OPCODE(misc_mem) = 0x0f,
  XTENSA_OPCODE(system) = 0x73,
  XTENSA_OPCODE(op_imm_32) = 0x1b,
  XTENSA_OPCODE(op_32) = 0x3b,
  XTENSA_OPCODE(load_fp) = 0x07,
  XTENSA_OPCODE(store_fp) = 0x27,
  XTENSA_OPCODE(op_fp) = 0x53,
  XTENSA_OPCODE(amo) = 0x2f,
  // TODO: FMADD.S etc.
} XTENSAInstOpcode;

// Funct3 values.
#define XTENSA_F3(op) kXTENSAInstFunct3_##op
typedef enum {
  XTENSA_F3(jalr) = 0,
  XTENSA_F3(beq) = 0,
  XTENSA_F3(bne) = 1,
  XTENSA_F3(beqz) = 0,
  XTENSA_F3(bnez) = 1,
  XTENSA_F3(blt) = 4,
  XTENSA_F3(bge) = 5,
  XTENSA_F3(bltu) = 6,
  XTENSA_F3(bgeu) = 7,
  XTENSA_F3(lb) = 0,
  XTENSA_F3(lh) = 1,
  XTENSA_F3(lw) = 2,
  XTENSA_F3(lbu) = 4,
  XTENSA_F3(lhu) = 5,
  XTENSA_F3(sb) = 0,
  XTENSA_F3(sh) = 1,
  XTENSA_F3(sw) = 2,
  XTENSA_F3(addi) = 0,
  XTENSA_F3(slti) = 2,
  XTENSA_F3(sltiu) = 3,
  XTENSA_F3(xori) = 4,
  XTENSA_F3(ori) = 6,
  XTENSA_F3(andi) = 7,
  XTENSA_F3(slli) = 1,
  XTENSA_F3(srli) = 5,
  XTENSA_F3(srai) = 5,
  XTENSA_F3(add) = 0,
  XTENSA_F3(sub) = 0,
  XTENSA_F3(sll) = 1,
  XTENSA_F3(slt) = 2,
  XTENSA_F3(sltu) = 3,
  XTENSA_F3(xor) = 4,
  XTENSA_F3(srl) = 5,
  XTENSA_F3(sra) = 5,
  XTENSA_F3(or) = 6,
  XTENSA_F3(and) = 7,

  XTENSA_F3(lwu) = 6,
  XTENSA_F3(ld) = 3,
  XTENSA_F3(sd) = 3,
  XTENSA_F3(slli64) = 1,  // NOTE: 64 suffix to distinguish 32 bit slli
  XTENSA_F3(srli64) = 5,
  XTENSA_F3(srai64) = 5,
  XTENSA_F3(addiw) = 0,
  XTENSA_F3(slliw) = 1,
  XTENSA_F3(srliw) = 5,
  XTENSA_F3(sraiw) = 5,
  XTENSA_F3(addw) = 0,
  XTENSA_F3(subw) = 0,
  XTENSA_F3(sllw) = 1,
  XTENSA_F3(srlw) = 5,
  XTENSA_F3(sraw) = 5,

  XTENSA_F3(mul) = 0,
  XTENSA_F3(mulh) = 1,
  XTENSA_F3(mulhsu) = 2,
  XTENSA_F3(mulhu) = 3,
  XTENSA_F3(div) = 4,
  XTENSA_F3(divu) = 5,
  XTENSA_F3(rem) = 6,
  XTENSA_F3(remu) = 7,

  // RV64M instructions.
  XTENSA_F3(mulw) = 0,
  XTENSA_F3(divw) = 4,
  XTENSA_F3(divuw) = 5,
  XTENSA_F3(remw) = 6,
  XTENSA_F3(remuw) = 7,

  XTENSA_F3(flw) = 2,
  XTENSA_F3(fsw) = 2,

  XTENSA_F3(fsgnj_s) = 0,
  XTENSA_F3(fsgnjn_s) = 1,
  XTENSA_F3(fsgnjx_s) = 2,
  XTENSA_F3(fmin_s) = 0,
  XTENSA_F3(fmax_s) = 1,

  XTENSA_F3(fmv_x_w) = 0,
  XTENSA_F3(feq_s) = 2,
  XTENSA_F3(flt_s) = 1,
  XTENSA_F3(fle_s) = 0,
  XTENSA_F3(fclass_s) = 1,
  XTENSA_F3(fmv_w_x) = 0,

  XTENSA_F3(fld) = 3,
  XTENSA_F3(fsd) = 3,

  XTENSA_F3(fsgnj_d) = 0,
  XTENSA_F3(fsgnjn_d) = 1,
  XTENSA_F3(fsgnjx_d) = 2,
  XTENSA_F3(fmin_d) = 0,
  XTENSA_F3(fmax_d) = 1,

  XTENSA_F3(feq_d) = 2,
  XTENSA_F3(flt_d) = 1,
  XTENSA_F3(fle_d) = 0,
  XTENSA_F3(fclass_d) = 1,
  XTENSA_F3(fmv_x_d) = 0,
  XTENSA_F3(fmv_d_x) = 0,
} XTENSAInstFunct3;

// Funct7 values.
#define XTENSA_F7(op) kXTENSAInstFunct7_##op
typedef enum {
  XTENSA_F7(slli) = 0x0,
  XTENSA_F7(srli) = 0x0,
  XTENSA_F7(srai) = 0x20,
  XTENSA_F7(add) = 0x0,
  XTENSA_F7(sub) = 0x20,
  XTENSA_F7(sll) = 0x0,
  XTENSA_F7(slt) = 0x0,
  XTENSA_F7(sltu) = 0x0,
  XTENSA_F7(xor) = 0x0,
  XTENSA_F7(srl) = 0x0,
  XTENSA_F7(sra) = 0x20,
  XTENSA_F7(or) = 0x0,
  XTENSA_F7(and) = 0x0,
  XTENSA_F7(slliw) = 0x0,
  XTENSA_F7(srliw) = 0x0,
  XTENSA_F7(sraiw) = 0x20,
  XTENSA_F7(addw) = 0x0,
  XTENSA_F7(subw) = 0x20,
  XTENSA_F7(sllw) = 0x0,
  XTENSA_F7(srlw) = 0x0,
  XTENSA_F7(sraw) = 0x20,

  XTENSA_F7(mul) = 0x1,
  XTENSA_F7(mulh) = 0x1,
  XTENSA_F7(mulhsu) = 0x1,
  XTENSA_F7(mulhu) = 0x1,
  XTENSA_F7(div) = 0x1,
  XTENSA_F7(divu) = 0x1,
  XTENSA_F7(rem) = 0x1,
  XTENSA_F7(remu) = 0x1,

  XTENSA_F7(mulw) = 0x1,
  XTENSA_F7(divw) = 0x1,
  XTENSA_F7(divuw) = 0x1,
  XTENSA_F7(remw) = 0x1,
  XTENSA_F7(remuw) = 0x1,

  XTENSA_F7(fadd_s) = 0x0,
  XTENSA_F7(fsub_s) = 0x04,
  XTENSA_F7(fmul_s) = 0x08,
  XTENSA_F7(fdiv_s) = 0x0c,
  XTENSA_F7(fsqrt_s) = 0x2c,
  XTENSA_F7(fsgnj_s) = 0x10,
  XTENSA_F7(fsgnjn_s) = 0x10,
  XTENSA_F7(fsgnjx_s) = 0x10,
  XTENSA_F7(fmin_s) = 0x14,
  XTENSA_F7(fmax_s) = 0x14,
  XTENSA_F7(fcvt_w_s) = 0x60,
  XTENSA_F7(fcvt_wu_s) = 0x60,
  XTENSA_F7(fmv_x_w) = 0x70,
  XTENSA_F7(feq_s) = 0x50,
  XTENSA_F7(flt_s) = 0x50,
  XTENSA_F7(fle_s) = 0x50,
  XTENSA_F7(fclass_s) = 0x70,
  XTENSA_F7(fcvt_s_w) = 0x68,
  XTENSA_F7(fcvt_s_wu) = 0x68,
  XTENSA_F7(fmv_w_x) = 0x78,

  XTENSA_F7(fcvt_l_s) = 0x60,
  XTENSA_F7(fcvt_lu_s) = 0x60,
  XTENSA_F7(fcvt_s_l) = 0x68,
  XTENSA_F7(fcvt_s_lu) = 0x68,

  XTENSA_F7(fadd_d) = 0x1,
  XTENSA_F7(fsub_d) = 0x05,
  XTENSA_F7(fmul_d) = 0x09,
  XTENSA_F7(fdiv_d) = 0x0d,
  XTENSA_F7(fsqrt_d) = 0x2d,
  XTENSA_F7(fsgnj_d) = 0x11,
  XTENSA_F7(fsgnjn_d) = 0x11,
  XTENSA_F7(fsgnjx_d) = 0x11,
  XTENSA_F7(fmin_d) = 0x15,
  XTENSA_F7(fmax_d) = 0x15,
  XTENSA_F7(fcvt_w_d) = 0x61,
  XTENSA_F7(fcvt_wu_d) = 0x61,
  XTENSA_F7(feq_d) = 0x51,
  XTENSA_F7(flt_d) = 0x51,
  XTENSA_F7(fle_d) = 0x51,
  XTENSA_F7(fclass_d) = 0x71,
  XTENSA_F7(fcvt_d_w) = 0x69,
  XTENSA_F7(fcvt_d_wu) = 0x69,

  XTENSA_F7(fcvt_s_d) = 0x20,
  XTENSA_F7(fcvt_d_s) = 0x21,

  XTENSA_F7(fcvt_l_d) = 0x61,
  XTENSA_F7(fcvt_lu_d) = 0x61,
  XTENSA_F7(fmv_x_d) = 0x71,
  XTENSA_F7(fcvt_d_l) = 0x69,
  XTENSA_F7(fcvt_d_lu) = 0x69,
  XTENSA_F7(fmv_d_x) = 0x79,

} XTENSAInstFunct7;

#define XTENSA_FP_RS2(op) kXTENSAInstFpRs2_##op
typedef enum {
  XTENSA_FP_RS2(fcvt_w_s) = 0,
  XTENSA_FP_RS2(fcvt_wu_s) = 1,
  XTENSA_FP_RS2(fcvt_l_s) = 2,
  XTENSA_FP_RS2(fcvt_lu_s) = 3,
  XTENSA_FP_RS2(fcvt_s_w) = 0,
  XTENSA_FP_RS2(fcvt_s_wu) = 1,
  XTENSA_FP_RS2(fcvt_s_l) = 2,
  XTENSA_FP_RS2(fcvt_s_lu) = 3,
  XTENSA_FP_RS2(fcvt_s_d) = 1,
  XTENSA_FP_RS2(fcvt_d_s) = 0,
  XTENSA_FP_RS2(fcvt_w_d) = 0,
  XTENSA_FP_RS2(fcvt_wu_d) = 1,
  XTENSA_FP_RS2(fcvt_d_w) = 0,
  XTENSA_FP_RS2(fcvt_d_wu) = 1,
  XTENSA_FP_RS2(fcvt_l_d) = 2,
  XTENSA_FP_RS2(fcvt_lu_d) = 3,
  XTENSA_FP_RS2(fcvt_d_l) = 2,
  XTENSA_FP_RS2(fcvt_d_lu) = 3,
  XTENSA_FP_RS2(fmv_x_d) = 0,
  XTENSA_FP_RS2(fmv_d_x) = 0,
  XTENSA_FP_RS2(fmv_w_x) = 0,
  XTENSA_FP_RS2(fmv_x_w) = 0,

} XTENSAFpRs2;

#endif /* xtensa_machine_h */
