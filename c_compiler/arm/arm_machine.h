//
//  arm_machine.h
//  c_compiler
//
//  ARM32 (AAPCS / EABI) machine constants.
//

#ifndef arm_machine_h
#define arm_machine_h

#define ARM_XR_REG 0  // AAPCS struct return hidden pointer in r0.

#define ARM_NUM_INT_REGS 16
#define ARM_NUM_FLOAT_REGS 32

#define ARM_INT_ZERO_REG 15  // r15 is PC; use r0 for zero via mov.

// Core registers.
#define ARM_SP_REG 13
#define ARM_LR_REG 14
#define ARM_PC_REG 15
#define ARM_FP_REG 11
#define ARM_IP_REG 12

// Integer return values.
#define ARM_INT_RETURN_VALUE_0 0
#define ARM_INT_RETURN_VALUE_1 1

// Integer args (AAPCS).
#define ARM_INT_ARG_START 0
#define ARM_INT_ARG_END 3
#define ARM_NUM_INT_ARGS (ARM_INT_ARG_END - ARM_INT_ARG_START + 1)

// Floating point args (VFP).
#define ARM_FP_ARG_START 0
#define ARM_FP_ARG_END 3
#define ARM_NUM_FP_ARGS (ARM_FP_ARG_END - ARM_FP_ARG_START + 1)

// Callee-saved integer registers.
#define ARM_INT_SAVED_START 4
#define ARM_INT_SAVED_END 11

// Caller-saved integer temps.
#define ARM_INT_TEMP_START 0
#define ARM_INT_TEMP_END 3

// IP registers for veneers/PLT.
#define ARM_IP1_REG 12
#define ARM_IP2_REG 12

// Spill scratch.
#define ARM_SPILL_ADDR 12

// Floating point return values.
#define ARM_FP_RETURN_VALUE_0 0
#define ARM_FP_RETURN_VALUE_1 1

// Callee-saved VFP (d8-d15 / s16-s31).
#define ARM_FP_SAVED_START 16
#define ARM_FP_SAVED_END 31

// Caller-saved VFP temps.
#define ARM_FP_TEMP_START 0
#define ARM_FP_TEMP_END 15

#define ARM_STACK_FRAME_HEADER_SIZE 8

// Register variables.
#define ARM_FIRST_INT_REG_VAR ARM_INT_SAVED_START
#define ARM_LAST_INT_REG_VAR ARM_INT_SAVED_END
#define ARM_FIRST_LEAF_INT_REG_VAR ARM_INT_TEMP_START
#define ARM_LAST_LEAF_INT_REG_VAR ARM_INT_TEMP_END

#define ARM_FIRST_FP_REG_VAR ARM_FP_SAVED_START
#define ARM_LAST_FP_REG_VAR ARM_FP_SAVED_END
#define ARM_FIRST_LEAF_FP_REG_VAR ARM_FP_TEMP_START
#define ARM_LAST_LEAF_FP_REG_VAR ARM_FP_TEMP_END

// ARM condition codes (nibble 31:28).
#define ARM_COND_EQ 0x0
#define ARM_COND_NE 0x1
#define ARM_COND_CS 0x2
#define ARM_COND_CC 0x3
#define ARM_COND_MI 0x4
#define ARM_COND_PL 0x5
#define ARM_COND_VS 0x6
#define ARM_COND_VC 0x7
#define ARM_COND_HI 0x8
#define ARM_COND_LS 0x9
#define ARM_COND_GE 0xa
#define ARM_COND_LT 0xb
#define ARM_COND_GT 0xc
#define ARM_COND_LE 0xd
#define ARM_COND_AL 0xe

#define ARM_COND(op) ((op) << 28)
#define ARM_AL ARM_COND(ARM_COND_AL)

#endif /* arm_machine_h */
