//
//  x86_64_machine.h
//  c_compiler
//
//  SysV AMD64 ABI constants for the x86_64 backend.
//

#ifndef x86_64_machine_h
#define x86_64_machine_h

// Integer register numbers (ModR/M encoding).
#define X86_REG_RAX 0
#define X86_REG_RCX 1
#define X86_REG_RDX 2
#define X86_REG_RBX 3
#define X86_REG_RSP 4
#define X86_REG_RBP 5
#define X86_REG_RSI 6
#define X86_REG_RDI 7
#define X86_REG_R8 8
#define X86_REG_R9 9
#define X86_REG_R10 10
#define X86_REG_R11 11
#define X86_REG_R12 12
#define X86_REG_R13 13
#define X86_REG_R14 14
#define X86_REG_R15 15

#define X86_NUM_INT_REGS 16

// SysV AMD64 ABI roles.
#define X86_RET_REG X86_REG_RAX
#define X86_SP_REG X86_REG_RSP
#define X86_FP_REG X86_REG_RBP

#define X86_INT_ARG0 X86_REG_RDI
#define X86_INT_ARG1 X86_REG_RSI
#define X86_INT_ARG2 X86_REG_RDX
#define X86_INT_ARG3 X86_REG_RCX
#define X86_INT_ARG4 X86_REG_R8
#define X86_INT_ARG5 X86_REG_R9

#define X86_INT_SAVED_START X86_REG_RBX
#define X86_INT_SAVED_END X86_REG_R15

#define X86_STACK_ALIGNMENT 16
#define X86_RED_ZONE_SIZE 128

// Logical register file used by the register allocator (same layout as RISC-V).
#define X86_64_NUM_INT_REGS 32
#define X86_64_NUM_FLOAT_REGS 32

#define X86_64_INT_ZERO_REG 0

#define X86_64_INT_RETURN_VALUE_0 X86_REG_RAX
#define X86_64_INT_RETURN_VALUE_1 X86_REG_RDX

// The SysV AMD64 ABI passes the first six integer/pointer arguments in
// registers (rdi, rsi, rdx, rcx, r8, r9); everything else is passed on the
// stack.  The logical register file reserves slots 10..15 for them.  Slots 16
// and 17 alias r10/r11, which are caller-saved scratch registers, so they must
// NOT be treated as argument registers: doing so makes the caller place args 7
// and 8 in r10/r11 while the va_arg lowering (gp_offset capped at 48 == 6*8)
// expects them on the stack, and the argument-shuffle code clobbers r10/r11
// while computing other argument addresses.
#define X86_64_INT_ARG_START 10
#define X86_64_INT_ARG_END 15
#define X86_64_NUM_INT_ARGS (X86_64_INT_ARG_END - X86_64_INT_ARG_START + 1)

#define X86_64_FP_ARG_START 0
#define X86_64_FP_ARG_END 7
#define X86_64_NUM_FP_ARGS (X86_64_FP_ARG_END - X86_64_FP_ARG_START + 1)

// Size of the in-frame register save area that a varargs function reserves so
// va_start/va_arg can address the incoming argument registers.  The SysV
// AMD64 va_list reg_save_area holds the six integer argument registers
// (rdi..r9, 8 bytes each, offsets 0..47) followed by the eight vector argument
// registers (xmm0..xmm7, 16 bytes each, offsets 48..175).  The trailing +8
// keeps the area clear of the saved frame pointer / return address slots once
// rbp is lowered to expose it (see GenerateProlog / RestoreRegisters).  This
// value is exactly how much the varargs prologue lowers rbp, and the same
// value is added back when addressing incoming stack arguments, so it must be
// used consistently in every such computation.
#define X86_64_VARARG_SAVE_AREA_SIZE \
  (X86_64_NUM_INT_ARGS * 8 + X86_64_NUM_FP_ARGS * 16 + 8)
// Byte offset of the first vector register slot within the save area.
#define X86_64_VARARG_FP_SAVE_OFFSET (X86_64_NUM_INT_ARGS * 8)

#define X86_64_INT_SAVED_START_1 8
#define X86_64_INT_SAVED_END_1 9
#define X86_64_INT_SAVED_START_2 18
#define X86_64_INT_SAVED_END_2 27

#define X86_64_INT_TEMP_START_1 5
#define X86_64_INT_TEMP_END_1 7
#define X86_64_INT_TEMP_START_2 28
#define X86_64_INT_TEMP_END_2 31

#define X86_64_SPILL_ADDR 3

#define X86_64_FP_RETURN_VALUE_0 0
#define X86_64_FP_RETURN_VALUE_1 1

#define X86_64_FP_SAVED_START_1 8
#define X86_64_FP_SAVED_END_1 15
#define X86_64_FP_SAVED_START_2 18
#define X86_64_FP_SAVED_END_2 27

#define X86_64_FP_TEMP_START_1 0
#define X86_64_FP_TEMP_END_1 7
#define X86_64_FP_TEMP_START_2 28
#define X86_64_FP_TEMP_END_2 31

#define X86_64_SP_REG 2
#define X86_64_FP_REG 8
#define X86_64_RET_REG 1

#define X86_64_STACK_FRAME_HEADER_SIZE 16

#define X86_64_FIRST_INT_REG_VAR X86_64_INT_SAVED_START_2
#define X86_64_LAST_INT_REG_VAR X86_64_INT_SAVED_END_2
#define X86_64_FIRST_LEAF_INT_REG_VAR X86_64_INT_TEMP_START_2
#define X86_64_LAST_LEAF_INT_REG_VAR X86_64_INT_TEMP_END_2

#define X86_64_FIRST_FP_REG_VAR X86_64_FP_SAVED_START_2
#define X86_64_LAST_FP_REG_VAR X86_64_FP_SAVED_END_2
#define X86_64_FIRST_LEAF_FP_REG_VAR X86_64_FP_TEMP_START_2
#define X86_64_LAST_LEAF_FP_REG_VAR X86_64_FP_TEMP_END_2

#endif /* x86_64_machine_h */
