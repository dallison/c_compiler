//
//  risc_v32_machine.h
//  c_compiler
//
//  Created by David Allison on 2/21/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v32_machine_h
#define risc_v32_machine_h

#define RV32_NUM_INT_REGS 32
#define RV32_NUM_FLOAT_REGS 32

#define RV32_INT_ZERO_REG 0

// Integer return values.
#define RV32_INT_RETURN_VALUE_0 10
#define RV32_INT_RETURN_VALUE_1 11

// Integer args.
#define RV32_INT_ARG_START 10
#define RV32_INT_ARG_END 17
#define RV32_NUM_INT_ARGS (RV32_INT_ARG_END - RV32_INT_ARG_START + 1)

// Floating point args.
#define RV32_FP_ARG_START 10
#define RV32_FP_ARG_END 17
#define RV32_NUM_FP_ARGS (RV32_FP_ARG_END - RV32_FP_ARG_START + 1)

// Saved registers are split into two blocks.
#define RV32_INT_SAVED_START_1 8
#define RV32_INT_SAVED_END_1 9
#define RV32_INT_SAVED_START_2 18
#define RV32_INT_SAVED_END_2 27

// Temps are split into two blocks.
#define RV32_INT_TEMP_START_1 5
#define RV32_INT_TEMP_END_1 7
#define RV32_INT_TEMP_START_2 28
#define RV32_INT_TEMP_END_2 31

// Register reserved for spill address calculation if necessary.
// This is the Global Pointer register in the ABI that appears
// not to be needed.
#define RV32_SPILL_ADDR 3

// Floating point return values.
#define RV32_FP_RETURN_VALUE_0 10
#define RV32_FP_RETURN_VALUE_1 11

// Floating points aved registers are split into two blocks.
#define RV32_FP_SAVED_START_1 8
#define RV32_FP_SAVED_END_1 9
#define RV32_FP_SAVED_START_2 18
#define RV32_FP_SAVED_END_2 27

// Temps are split into two blocks.
#define RV32_FP_TEMP_START_1 0
#define RV32_FP_TEMP_END_1 7
#define RV32_FP_TEMP_START_2 28
#define RV32_FP_TEMP_END_2 31

#define RV32_SP_REG 2   // Stack pointer.
#define RV32_FP_REG 8   // Frame pointer.
#define RV32_RET_REG 1  // Return address.

// Size of the stack frame header containing saved ra and s0.
#define RV32_STACK_FRAME_HEADER_SIZE 16

// Register variables: variables that are in registers rather than being on
// the stack.

// First and last register numbers for int register variables.
#define RV32_FIRST_INT_REG_VAR RV32_INT_SAVED_START_2
#define RV32_LAST_INT_REG_VAR RV32_INT_SAVED_END_2

#define RV32_FIRST_LEAF_INT_REG_VAR RV32_INT_TEMP_START_2
#define RV32_LAST_LEAF_INT_REG_VAR RV32_INT_TEMP_END_2

// First and last register numbers for floating point register variables.
#define RV32_FIRST_FP_REG_VAR RV32_FP_SAVED_START_2
#define RV32_LAST_FP_REG_VAR RV32_FP_SAVED_END_2
#define RV32_FIRST_LEAF_FP_REG_VAR RV32_FP_TEMP_START_2
#define RV32_LAST_LEAF_FP_REG_VAR RV32_FP_TEMP_END_2

// Instruction encodings.

// Opcode values.
#define RV32_OPCODE(op) kRV32InstOpcode_##op
typedef enum {
  RV32_OPCODE(op) = 0x33,
  RV32_OPCODE(op_imm) = 0x13,
  RV32_OPCODE(lui) = 0x37,
  RV32_OPCODE(auipc) = 0x17,
  RV32_OPCODE(jal) = 0x6f,
  RV32_OPCODE(jalr) = 0x67,
  RV32_OPCODE(branch) = 0x63,
  RV32_OPCODE(load) = 0x03,
  RV32_OPCODE(store) = 0x23,
  RV32_OPCODE(misc_mem) = 0x0f,
  RV32_OPCODE(system) = 0x73,
  RV32_OPCODE(op_imm_32) = 0x1b,
  RV32_OPCODE(op_32) = 0x3b,
  RV32_OPCODE(load_fp) = 0x07,
  RV32_OPCODE(store_fp) = 0x27,
  RV32_OPCODE(op_fp) = 0x53,
  RV32_OPCODE(amo) = 0x2f,
  // TODO: FMADD.S etc.
} RV32InstOpcode;

// Funct3 values.
#define RV32_F3(op) kRV32InstFunct3_##op
typedef enum {
  RV32_F3(jalr) = 0,
  RV32_F3(beq) = 0,
  RV32_F3(bne) = 1,
  RV32_F3(beqz) = 0,
  RV32_F3(bnez) = 1,
  RV32_F3(blt) = 4,
  RV32_F3(bge) = 5,
  RV32_F3(bltu) = 6,
  RV32_F3(bgeu) = 7,
  RV32_F3(lb) = 0,
  RV32_F3(lh) = 1,
  RV32_F3(lw) = 2,
  RV32_F3(lbu) = 4,
  RV32_F3(lhu) = 5,
  RV32_F3(sb) = 0,
  RV32_F3(sh) = 1,
  RV32_F3(sw) = 2,
  RV32_F3(addi) = 0,
  RV32_F3(slti) = 2,
  RV32_F3(sltiu) = 3,
  RV32_F3(xori) = 4,
  RV32_F3(ori) = 6,
  RV32_F3(andi) = 7,
  RV32_F3(slli) = 1,
  RV32_F3(srli) = 5,
  RV32_F3(srai) = 5,
  RV32_F3(add) = 0,
  RV32_F3(sub) = 0,
  RV32_F3(sll) = 1,
  RV32_F3(slt) = 2,
  RV32_F3(sltu) = 3,
  RV32_F3(xor) = 4,
  RV32_F3(srl) = 5,
  RV32_F3(sra) = 5,
  RV32_F3(or) = 6,
  RV32_F3(and) = 7,

  RV32_F3(lwu) = 6,
  RV32_F3(ld) = 3,
  RV32_F3(sd) = 3,
  RV32_F3(slli64) = 1,  // NOTE: 64 suffix to distinguish 32 bit slli
  RV32_F3(srli64) = 5,
  RV32_F3(srai64) = 5,
  RV32_F3(addiw) = 0,
  RV32_F3(slliw) = 1,
  RV32_F3(srliw) = 5,
  RV32_F3(sraiw) = 5,
  RV32_F3(addw) = 0,
  RV32_F3(subw) = 0,
  RV32_F3(sllw) = 1,
  RV32_F3(srlw) = 5,
  RV32_F3(sraw) = 5,

  RV32_F3(mul) = 0,
  RV32_F3(mulh) = 1,
  RV32_F3(mulhsu) = 2,
  RV32_F3(mulhu) = 3,
  RV32_F3(div) = 4,
  RV32_F3(divu) = 5,
  RV32_F3(rem) = 6,
  RV32_F3(remu) = 7,

  // RV64M instructions.
  RV32_F3(mulw) = 0,
  RV32_F3(divw) = 4,
  RV32_F3(divuw) = 5,
  RV32_F3(remw) = 6,
  RV32_F3(remuw) = 7,

  RV32_F3(flw) = 2,
  RV32_F3(fsw) = 2,

  RV32_F3(fsgnj_s) = 0,
  RV32_F3(fsgnjn_s) = 1,
  RV32_F3(fsgnjx_s) = 2,
  RV32_F3(fmin_s) = 0,
  RV32_F3(fmax_s) = 1,

  RV32_F3(fmv_x_w) = 0,
  RV32_F3(feq_s) = 2,
  RV32_F3(flt_s) = 1,
  RV32_F3(fle_s) = 0,
  RV32_F3(fclass_s) = 1,
  RV32_F3(fmv_w_x) = 0,

  RV32_F3(fld) = 3,
  RV32_F3(fsd) = 3,

  RV32_F3(fsgnj_d) = 0,
  RV32_F3(fsgnjn_d) = 1,
  RV32_F3(fsgnjx_d) = 2,
  RV32_F3(fmin_d) = 0,
  RV32_F3(fmax_d) = 1,

  RV32_F3(feq_d) = 2,
  RV32_F3(flt_d) = 1,
  RV32_F3(fle_d) = 0,
  RV32_F3(fclass_d) = 1,
  RV32_F3(fmv_x_d) = 0,
  RV32_F3(fmv_d_x) = 0,
} RV32InstFunct3;

// Funct7 values.
#define RV32_F7(op) kRV32InstFunct7_##op
typedef enum {
  RV32_F7(slli) = 0x0,
  RV32_F7(srli) = 0x0,
  RV32_F7(srai) = 0x20,
  RV32_F7(add) = 0x0,
  RV32_F7(sub) = 0x20,
  RV32_F7(sll) = 0x0,
  RV32_F7(slt) = 0x0,
  RV32_F7(sltu) = 0x0,
  RV32_F7(xor) = 0x0,
  RV32_F7(srl) = 0x0,
  RV32_F7(sra) = 0x20,
  RV32_F7(or) = 0x0,
  RV32_F7(and) = 0x0,
  RV32_F7(slliw) = 0x0,
  RV32_F7(srliw) = 0x0,
  RV32_F7(sraiw) = 0x20,
  RV32_F7(addw) = 0x0,
  RV32_F7(subw) = 0x20,
  RV32_F7(sllw) = 0x0,
  RV32_F7(srlw) = 0x0,
  RV32_F7(sraw) = 0x20,

  RV32_F7(mul) = 0x1,
  RV32_F7(mulh) = 0x1,
  RV32_F7(mulhsu) = 0x1,
  RV32_F7(mulhu) = 0x1,
  RV32_F7(div) = 0x1,
  RV32_F7(divu) = 0x1,
  RV32_F7(rem) = 0x1,
  RV32_F7(remu) = 0x1,

  RV32_F7(mulw) = 0x1,
  RV32_F7(divw) = 0x1,
  RV32_F7(divuw) = 0x1,
  RV32_F7(remw) = 0x1,
  RV32_F7(remuw) = 0x1,

  RV32_F7(fadd_s) = 0x0,
  RV32_F7(fsub_s) = 0x04,
  RV32_F7(fmul_s) = 0x08,
  RV32_F7(fdiv_s) = 0x0c,
  RV32_F7(fsqrt_s) = 0x2c,
  RV32_F7(fsgnj_s) = 0x10,
  RV32_F7(fsgnjn_s) = 0x10,
  RV32_F7(fsgnjx_s) = 0x10,
  RV32_F7(fmin_s) = 0x14,
  RV32_F7(fmax_s) = 0x14,
  RV32_F7(fcvt_w_s) = 0x60,
  RV32_F7(fcvt_wu_s) = 0x60,
  RV32_F7(fmv_x_w) = 0x70,
  RV32_F7(feq_s) = 0x50,
  RV32_F7(flt_s) = 0x50,
  RV32_F7(fle_s) = 0x50,
  RV32_F7(fclass_s) = 0x70,
  RV32_F7(fcvt_s_w) = 0x68,
  RV32_F7(fcvt_s_wu) = 0x68,
  RV32_F7(fmv_w_x) = 0x78,

  RV32_F7(fcvt_l_s) = 0x60,
  RV32_F7(fcvt_lu_s) = 0x60,
  RV32_F7(fcvt_s_l) = 0x68,
  RV32_F7(fcvt_s_lu) = 0x68,

  RV32_F7(fadd_d) = 0x1,
  RV32_F7(fsub_d) = 0x05,
  RV32_F7(fmul_d) = 0x09,
  RV32_F7(fdiv_d) = 0x0d,
  RV32_F7(fsqrt_d) = 0x2d,
  RV32_F7(fsgnj_d) = 0x11,
  RV32_F7(fsgnjn_d) = 0x11,
  RV32_F7(fsgnjx_d) = 0x11,
  RV32_F7(fmin_d) = 0x15,
  RV32_F7(fmax_d) = 0x15,
  RV32_F7(fcvt_w_d) = 0x61,
  RV32_F7(fcvt_wu_d) = 0x61,
  RV32_F7(feq_d) = 0x51,
  RV32_F7(flt_d) = 0x51,
  RV32_F7(fle_d) = 0x51,
  RV32_F7(fclass_d) = 0x71,
  RV32_F7(fcvt_d_w) = 0x69,
  RV32_F7(fcvt_d_wu) = 0x69,

  RV32_F7(fcvt_s_d) = 0x20,
  RV32_F7(fcvt_d_s) = 0x21,

  RV32_F7(fcvt_l_d) = 0x61,
  RV32_F7(fcvt_lu_d) = 0x61,
  RV32_F7(fmv_x_d) = 0x71,
  RV32_F7(fcvt_d_l) = 0x69,
  RV32_F7(fcvt_d_lu) = 0x69,
  RV32_F7(fmv_d_x) = 0x79,

} RV32InstFunct7;

#define RV32_FP_RS2(op) kRV32InstFpRs2_##op
typedef enum {
  RV32_FP_RS2(fcvt_w_s) = 0,
  RV32_FP_RS2(fcvt_wu_s) = 1,
  RV32_FP_RS2(fcvt_l_s) = 2,
  RV32_FP_RS2(fcvt_lu_s) = 3,
  RV32_FP_RS2(fcvt_s_w) = 0,
  RV32_FP_RS2(fcvt_s_wu) = 1,
  RV32_FP_RS2(fcvt_s_l) = 2,
  RV32_FP_RS2(fcvt_s_lu) = 3,
  RV32_FP_RS2(fcvt_s_d) = 1,
  RV32_FP_RS2(fcvt_d_s) = 0,
  RV32_FP_RS2(fcvt_w_d) = 0,
  RV32_FP_RS2(fcvt_wu_d) = 1,
  RV32_FP_RS2(fcvt_d_w) = 0,
  RV32_FP_RS2(fcvt_d_wu) = 1,
  RV32_FP_RS2(fcvt_l_d) = 2,
  RV32_FP_RS2(fcvt_lu_d) = 3,
  RV32_FP_RS2(fcvt_d_l) = 2,
  RV32_FP_RS2(fcvt_d_lu) = 3,
  RV32_FP_RS2(fmv_x_d) = 0,
  RV32_FP_RS2(fmv_d_x) = 0,
  RV32_FP_RS2(fmv_w_x) = 0,
  RV32_FP_RS2(fmv_x_w) = 0,

} RV32FpRs2;

#endif /* risc_v32_machine_h */
