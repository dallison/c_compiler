//
//  risc_v_machine.h
//  c_compiler
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_machine_h
#define risc_v_machine_h

#define RV_NUM_INT_REGS 32
#define RV_NUM_FLOAT_REGS 32

#define RV_INT_ZERO_REG 0

// Integer return values.
#define RV_INT_RETURN_VALUE_0 10
#define RV_INT_RETURN_VALUE_1 11

// Integer args.
#define RV_INT_ARG_START 10
#define RV_INT_ARG_END 17
#define RV_NUM_INT_ARGS (RV_INT_ARG_END - RV_INT_ARG_START + 1)

// Floating point args.
#define RV_FP_ARG_START 10
#define RV_FP_ARG_END 17
#define RV_NUM_FP_ARGS (RV_FP_ARG_END - RV_FP_ARG_START + 1)

// Saved registers are split into two blocks.
#define RV_INT_SAVED_START_1 8
#define RV_INT_SAVED_END_1 9
#define RV_INT_SAVED_START_2 18
#define RV_INT_SAVED_END_2 27

// Temps are split into two blocks.
#define RV_INT_TEMP_START_1 5
#define RV_INT_TEMP_END_1 7
#define RV_INT_TEMP_START_2 28
#define RV_INT_TEMP_END_2 31

// Floating point return values.
#define RV_FP_RETURN_VALUE_0 10
#define RV_FP_RETURN_VALUE_1 11

// Floating points aved registers are split into two blocks.
#define RV_FP_SAVED_START_1 8
#define RV_FP_SAVED_END_1 9
#define RV_FP_SAVED_START_2 18
#define RV_FP_SAVED_END_2 27

// Temps are split into two blocks.
#define RV_FP_TEMP_START_1 0
#define RV_FP_TEMP_END_1 7
#define RV_FP_TEMP_START_2 28
#define RV_FP_TEMP_END_2 31

#define RV_SP_REG 2   // Stack pointer.
#define RV_FP_REG 8   // Frame pointer.
#define RV_RET_REG 1  // Return address.

// Size of the stack frame header containing saved ra and s0.
#define RV_STACK_FRAME_HEADER_SIZE 16

// Register variables: variables that are in registers rather than being on
// the stack.
// We don't want to use all the registers for variables because it will
// cause more spilling to occur.  Let's keep 2 free for temp usage.

// NOTE: if these are changed, make sure to update the RV opcodes.
// Max number of int register variables.
#define RV_MAX_INT_REG_VARS 8

// Max number of floating point register variables.
#define RV_MAX_FP_REG_VARS 8

// First and last register numbers for int register variables.
#define RV_FIRST_INT_REG_VAR RV_INT_SAVED_START_2
#define RV_LAST_INT_REG_VAR (RV_FIRST_INT_REG_VAR + RV_MAX_INT_REG_VARS)

#define RV_FIRST_LEAF_INT_REG_VAR RV_INT_TEMP_START_2
#define RV_LAST_LEAF_INT_REG_VAR \
  (RV_FIRST_LEAF_INT_REG_VAR + RV_MAX_INT_REG_VARS)

// First and last register numbers for floating point register variables.
#define RV_FIRST_FP_REG_VAR RV_FP_SAVED_START_2
#define RV_LAST_FP_REG_VAR (RV_FIRST_FP_REG_VAR + RV_MAX_FP_REG_VARS)
#define RV_FIRST_LEAF_FP_REG_VAR RV_FP_TEMP_START_2
#define RV_LAST_LEAF_FP_REG_VAR (RV_FIRST_LEAF_FP_REG_VAR + RV_MAX_FP_REG_VARS)

// Instruction encodings.

// Opcode values.
#define RV_OPCODE(op) kRVInstOpcode_##op
typedef enum {
  RV_OPCODE(op) = 0x33,
  RV_OPCODE(op_imm) = 0x13,
  RV_OPCODE(lui) = 0x37,
  RV_OPCODE(auipc) = 0x17,
  RV_OPCODE(jal) = 0x6f,
  RV_OPCODE(jalr) = 0x67,
  RV_OPCODE(branch) = 0x63,
  RV_OPCODE(load) = 0x03,
  RV_OPCODE(store) = 0x23,
  RV_OPCODE(misc_mem) = 0x0f,
  RV_OPCODE(system) = 0x73,
  RV_OPCODE(op_imm_32) = 0x1b,
  RV_OPCODE(op_32) = 0x3b,
  RV_OPCODE(load_fp) = 0x07,
  RV_OPCODE(store_fp) = 0x27,
  RV_OPCODE(op_fp) = 0x53,
  // TODO: FMADD.S etc.
} RVInstOpcode;

// Funct3 values.
#define RV_F3(op) kRVInstFunct3_##op
typedef enum {
  RV_F3(jalr) = 0,
  RV_F3(beq) = 0,
  RV_F3(bne) = 1,
  RV_F3(beqz) = 0,
  RV_F3(bnez) = 1,
  RV_F3(blt) = 4,
  RV_F3(bge) = 5,
  RV_F3(bltu) = 6,
  RV_F3(bgeu) = 7,
  RV_F3(lb) = 0,
  RV_F3(lh) = 1,
  RV_F3(lw) = 2,
  RV_F3(lbu) = 4,
  RV_F3(lhu) = 5,
  RV_F3(sb) = 0,
  RV_F3(sh) = 1,
  RV_F3(sw) = 2,
  RV_F3(addi) = 0,
  RV_F3(slti) = 2,
  RV_F3(sltiu) = 3,
  RV_F3(xori) = 4,
  RV_F3(ori) = 6,
  RV_F3(andi) = 7,
  RV_F3(slli) = 1,
  RV_F3(srli) = 5,
  RV_F3(srai) = 5,
  RV_F3(add) = 0,
  RV_F3(sub) = 0,
  RV_F3(sll) = 1,
  RV_F3(slt) = 2,
  RV_F3(sltu) = 3,
  RV_F3(xor) = 4,
  RV_F3(srl) = 5,
  RV_F3(sra) = 5,
  RV_F3(or) = 6,
  RV_F3(and) = 7,

  RV_F3(lwu) = 6,
  RV_F3(ld) = 3,
  RV_F3(sd) = 3,
  RV_F3(slli64) = 1,  // NOTE: 64 suffix to distinguish 32 bit slli
  RV_F3(srli64) = 5,
  RV_F3(srai64) = 5,
  RV_F3(addiw) = 0,
  RV_F3(slliw) = 1,
  RV_F3(srliw) = 5,
  RV_F3(sraiw) = 5,
  RV_F3(addw) = 0,
  RV_F3(subw) = 0,
  RV_F3(sllw) = 1,
  RV_F3(srlw) = 5,
  RV_F3(sraw) = 5,

  RV_F3(mul) = 0,
  RV_F3(mulh) = 1,
  RV_F3(mulhsu) = 2,
  RV_F3(mulhu) = 3,
  RV_F3(div) = 4,
  RV_F3(divu) = 5,
  RV_F3(rem) = 6,
  RV_F3(remu) = 7,

  // RV64M instructions.
  RV_F3(mulw) = 0,
  RV_F3(divw) = 4,
  RV_F3(divuw) = 5,
  RV_F3(remw) = 6,
  RV_F3(remuw) = 7,

  RV_F3(flw) = 2,
  RV_F3(fsw) = 2,

  RV_F3(fsgnj_s) = 0,
  RV_F3(fsgnjn_s) = 1,
  RV_F3(fsgnjx_s) = 2,
  RV_F3(fmin_s) = 0,
  RV_F3(fmax_s) = 1,

  RV_F3(fmv_x_w) = 0,
  RV_F3(feq_s) = 2,
  RV_F3(flt_s) = 1,
  RV_F3(fle_s) = 0,
  RV_F3(fclass_s) = 1,
  RV_F3(fmv_w_x) = 0,

  RV_F3(fld) = 3,
  RV_F3(fsd) = 3,

  RV_F3(fsgnj_d) = 0,
  RV_F3(fsgnjn_d) = 1,
  RV_F3(fsgnjx_d) = 2,
  RV_F3(fmin_d) = 0,
  RV_F3(fmax_d) = 1,

  RV_F3(feq_d) = 2,
  RV_F3(flt_d) = 1,
  RV_F3(fle_d) = 0,
  RV_F3(fclass_d) = 1,
  RV_F3(fmv_x_d) = 0,
  RV_F3(fmv_d_x) = 0,
} RVInstFunct3;

// Funct7 values.
#define RV_F7(op) kRVInstFunct7_##op
typedef enum {
  RV_F7(slli) = 0x0,
  RV_F7(srli) = 0x0,
  RV_F7(srai) = 0x20,
  RV_F7(add) = 0x0,
  RV_F7(sub) = 0x20,
  RV_F7(sll) = 0x0,
  RV_F7(slt) = 0x0,
  RV_F7(sltu) = 0x0,
  RV_F7(xor) = 0x0,
  RV_F7(srl) = 0x0,
  RV_F7(sra) = 0x20,
  RV_F7(or) = 0x0,
  RV_F7(and) = 0x0,
  RV_F7(slliw) = 0x0,
  RV_F7(srliw) = 0x0,
  RV_F7(sraiw) = 0x20,
  RV_F7(addw) = 0x0,
  RV_F7(subw) = 0x20,
  RV_F7(sllw) = 0x0,
  RV_F7(srlw) = 0x0,
  RV_F7(sraw) = 0x20,

  RV_F7(mul) = 0x1,
  RV_F7(mulh) = 0x1,
  RV_F7(mulhsu) = 0x1,
  RV_F7(mulhu) = 0x1,
  RV_F7(div) = 0x1,
  RV_F7(divu) = 0x1,
  RV_F7(rem) = 0x1,
  RV_F7(remu) = 0x1,

  RV_F7(mulw) = 0x1,
  RV_F7(divw) = 0x1,
  RV_F7(divuw) = 0x1,
  RV_F7(remw) = 0x1,
  RV_F7(remuw) = 0x1,

  RV_F7(fadd_s) = 0x0,
  RV_F7(fsub_s) = 0x04,
  RV_F7(fmul_s) = 0x08,
  RV_F7(fdiv_s) = 0x0c,
  RV_F7(fsqrt_s) = 0x2c,
  RV_F7(fsgnj_s) = 0x10,
  RV_F7(fsgnjn_s) = 0x10,
  RV_F7(fsgnjx_s) = 0x10,
  RV_F7(fmin_s) = 0x14,
  RV_F7(fmax_s) = 0x14,
  RV_F7(fcvt_w_s) = 0x60,
  RV_F7(fcvt_wu_s) = 0x60,
  RV_F7(fmv_x_w) = 0x70,
  RV_F7(feq_s) = 0x50,
  RV_F7(flt_s) = 0x50,
  RV_F7(fle_s) = 0x50,
  RV_F7(fclass_s) = 0x70,
  RV_F7(fcvt_s_w) = 0x68,
  RV_F7(fcvt_s_wu) = 0x68,
  RV_F7(fmv_w_x) = 0x78,

  RV_F7(fcvt_l_s) = 0x60,
  RV_F7(fcvt_lu_s) = 0x60,
  RV_F7(fcvt_s_l) = 0x68,
  RV_F7(fcvt_s_lu) = 0x68,

  RV_F7(fadd_d) = 0x1,
  RV_F7(fsub_d) = 0x05,
  RV_F7(fmul_d) = 0x09,
  RV_F7(fdiv_d) = 0x0d,
  RV_F7(fsqrt_d) = 0x2d,
  RV_F7(fsgnj_d) = 0x11,
  RV_F7(fsgnjn_d) = 0x11,
  RV_F7(fsgnjx_d) = 0x11,
  RV_F7(fmin_d) = 0x15,
  RV_F7(fmax_d) = 0x15,
  RV_F7(fcvt_w_d) = 0x61,
  RV_F7(fcvt_wu_d) = 0x61,
  RV_F7(feq_d) = 0x51,
  RV_F7(flt_d) = 0x51,
  RV_F7(fle_d) = 0x51,
  RV_F7(fclass_d) = 0x71,
  RV_F7(fcvt_d_w) = 0x69,
  RV_F7(fcvt_d_wu) = 0x69,

  RV_F7(fcvt_s_d) = 0x20,
  RV_F7(fcvt_d_s) = 0x21,

  RV_F7(fcvt_l_d) = 0x61,
  RV_F7(fcvt_lu_d) = 0x61,
  RV_F7(fmv_x_d) = 0x71,
  RV_F7(fcvt_d_l) = 0x69,
  RV_F7(fcvt_d_lu) = 0x69,
  RV_F7(fmv_d_x) = 0x79,

} RVInstFunct7;

#define RV_FP_RS2(op) kRVInstFpRs2_##op
typedef enum {
  RV_FP_RS2(fcvt_w_s) = 0,
  RV_FP_RS2(fcvt_wu_s) = 1,
  RV_FP_RS2(fcvt_l_s) = 2,
  RV_FP_RS2(fcvt_lu_s) = 3,
  RV_FP_RS2(fcvt_s_w) = 0,
  RV_FP_RS2(fcvt_s_wu) = 1,
  RV_FP_RS2(fcvt_s_l) = 2,
  RV_FP_RS2(fcvt_s_lu) = 3,
  RV_FP_RS2(fcvt_s_d) = 1,
  RV_FP_RS2(fcvt_d_s) = 0,
  RV_FP_RS2(fcvt_w_d) = 0,
  RV_FP_RS2(fcvt_wu_d) = 1,
  RV_FP_RS2(fcvt_d_w) = 0,
  RV_FP_RS2(fcvt_d_wu) = 1,
  RV_FP_RS2(fcvt_l_d) = 2,
  RV_FP_RS2(fcvt_lu_d) = 3,
  RV_FP_RS2(fcvt_d_l) = 2,
  RV_FP_RS2(fcvt_d_lu) = 3,
  RV_FP_RS2(fmv_x_d) = 0,
  RV_FP_RS2(fmv_d_x) = 0,
  RV_FP_RS2(fmv_w_x) = 0,
  RV_FP_RS2(fmv_x_w) = 0,

} RVFpRs2;

#endif /* risc_v_machine_h */
