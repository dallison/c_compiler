//
//  arm_machine.h
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#ifndef arm_machine_h
#define arm_machine_h

#define ARM_NUM_INT_REGS 32
#define ARM_NUM_FLOAT_REGS 32

#define ARM_INT_ZERO_REG 0

// Integer return values.
#define ARM_INT_RETURN_VALUE_0 10
#define ARM_INT_RETURN_VALUE_1 11

// Integer args.
#define ARM_INT_ARG_START 10
#define ARM_INT_ARG_END 17
#define ARM_NUM_INT_ARGS (ARM_INT_ARG_END - ARM_INT_ARG_START + 1)

// Floating point args.
#define ARM_FP_ARG_START 10
#define ARM_FP_ARG_END 17
#define ARM_NUM_FP_ARGS (ARM_FP_ARG_END - ARM_FP_ARG_START + 1)

#endif /* arm_machine_h */
