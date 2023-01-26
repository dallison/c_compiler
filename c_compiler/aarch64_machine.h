//
//  aarch64_machine.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#ifndef aarch64_machine_h
#define aarch64_machine_h

#define AARCH64_NUM_INT_REGS 33       // Includes SP special
#define AARCH64_NUM_FLOAT_REGS 32

#define AARCH64_INT_ZERO_REG 31

#define AARCH64_FP_REG 29
#define AARCH64_LR_REG 30
#define AARCH64_SP_REG 32     // Special register.
#define AARCH64_XR_REG 8      // Indirect result register.

#define AARCH64_IP1_REG 16
#define AARCH64_IP2_REG 17

// Integer return values.
#define AARCH64_INT_RETURN_VALUE_0 0
#define AARCH64_INT_RETURN_VALUE_1 1

// Integer args.
#define AARCH64_INT_ARG_START 0
#define AARCH64_INT_ARG_END 7
#define AARCH64_NUM_INT_ARGS (AARCH64_INT_ARG_END - AARCH64_INT_ARG_START + 1)

// Floating point args.
#define AARCH64_FP_ARG_START 0
#define AARCH64_FP_ARG_END 7
#define AARCH64_NUM_FP_ARGS (AARCH64_FP_ARG_END - AARCH64_FP_ARG_START + 1)

// Integer saved and temp regas.
#define AARCH64_INT_SAVED_START 19
#define AARCH64_INT_SAVED_END 28
#define AARCH64_INT_TEMP_START 9
#define AARCH64_INT_TEMP_END 15

// Floating point saved and temp regs.
#define AARCH64_FP_SAVED_START 8
#define AARCH64_FP_SAVED_END 15
#define AARCH64_FP_TEMP_START 16
#define AARCH64_FP_TEMP_END 31

#define AARCH64_SPILL_ADDR 16

#define AARCH64_STACK_FRAME_HEADER_SIZE 16    // TODO fix

// First and last register numbers for int register variables.
#define AARCH64_FIRST_INT_REG_VAR AARCH64_INT_SAVED_START
#define AARCH64_LAST_INT_REG_VAR AARCH64_INT_SAVED_END

#define AARCH64_FIRST_LEAF_INT_REG_VAR AARCH64_INT_TEMP_START
#define AARCH64_LAST_LEAF_INT_REG_VAR AARCH64_INT_TEMP_END

// First and last register numbers for floating point register variables.
#define AARCH64_FIRST_FP_REG_VAR AARCH64_FP_SAVED_START
#define AARCH64_LAST_FP_REG_VAR AARCH64_FP_SAVED_END
#define AARCH64_FIRST_LEAF_FP_REG_VAR AARCH64_FP_TEMP_START
#define AARCH64_LAST_LEAF_FP_REG_VAR AARCH64_FP_TEMP_END

#define AARCH64_FIRST_FP_REG_VAR AARCH64_FP_SAVED_START
#define AARCH64_LAST_FP_REG_VAR AARCH64_FP_SAVED_END
#define AARCH64_FIRST_LEAF_FP_REG_VAR AARCH64_FP_TEMP_START
#define AARCH64_LAST_LEAF_FP_REG_VAR AARCH64_FP_TEMP_END

#endif /* aarch64_machine_h */
