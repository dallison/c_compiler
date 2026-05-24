//
//  x86_64_emitter.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

// This is the x86-64 assembly language emitter. It prints the x86-64
// instructions to the given file in assembly language.  The x86 assembler
// reads this file and generates the binary.

#include "x86_64_emitter.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "compiler.h"
#include "x86_64_assembler.h"
#include "x86_64_codegen.h"
#include "x86_64_machine.h"
#include "x86_64_reg_alloc.h"
#include "target_basic_block.h"

// In assembler mode, $ introduces a hexadecimal immediate (see CollectHex in
// lex.c), not a decimal value as in GNU as AT&T syntax.
static void PrintAsmImmediate(FILE* fp, int64_t value) {
  if (value < 0) {
    fputc('-', fp);
    value = -value;
  }
  fprintf(fp, "$%" PRIx64, (uint64_t)value);
}

// Is the given instruction printable?  Some instructions do not
// produce any output as they are used for information for other
// instructions.
static bool IsPrintable(TargetInstruction* inst) {
  // Constants are encoded in the instructions that use them.
  if (TargetIsConst(inst)) {
    return false;
  }
  // These opcodes are not printable.
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(tmp):
    case X86_64_OP(fp):
    case X86_64_OP(sp):
    case X86_64_OP(literal):
    case X86_64_OP(structreturn):
    case X86_64_OP(resulti):
    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
    case X86_64_OP(a0):
    case X86_64_OP(a1):
    case X86_64_OP(a2):
    case X86_64_OP(a3):
    case X86_64_OP(a4):
    case X86_64_OP(a5):
    case X86_64_OP(a6):
    case X86_64_OP(a7):
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
    case X86_64_OP(regarg):
    case X86_64_OP(ivarreg):
    case X86_64_OP(fvarreg):
    case X86_64_OP(x0):
    case X86_64_OP(t0):
    case X86_64_OP(nrvoval):
      return false;
    default:
      break;
  }
  return true;
}

// The stack frame looks like this:
//
// +------------------------------+
// .                              .
// .         pushed args          .
// .                              .
// +------------------------------+  <-- previous sp
// |                              |
// |      saved varargs regs      |
// |                              |
// +------------------------------+  <-- current frame pointer (rbp)
// |      saved return address    |
// +------------------------------+
// |      saved frame pointer     |
// +------------------------------+ }-+
// |                              |   | Arguments passed in registers
// |       saved args             |   | that are not assigned to registers
// |                              |   | in the procedure
// +------------------------------+ }-+
// |                              |   | emitter->rv->base.stack_frame_size
// |       local variables        |   | bytes long.  All local variables
// |                              |   | not in registers are here.
// +------------------------------+ }-+
// |                              |   |  Expression results saved when
// |       spilled registers      |   |  we run out of registers
// |                              |   |
// +------------------------------+ }-+
// |                              |   | Callee-saved registers are stored
// |       saved registers        |   | here.
// |                              |   |
// +------------------------------+  <-- current sp
//
// For varargs procedures, all integer argument registers
// other than those for declared arguments are saved into
// the area above the saved return address and adjacent
// to the previous sp.  This is to ensure a contiguous
// address range for those arguments passed after the
// last declared argument.
//
// All local variables are accessed as a negative offset from
// the frame pointer (rbp).
//
// Spilled register values are a negative offset from rbp.
// The only potentially large area is the space for
// local variables.  The rest are small and bounded.

// This is the size of the stack frame including the space
// for the local variables.
static int StackFrameSize(X86_64Emitter* emitter) {
  // Start off with local variable space.  This also includes
  // 16 bytes for the saved ra and s0.
  int stack_frame_size = emitter->rv->base.stack_frame_size + 16;

  bool varargs = emitter->rv->base.varargs;

  if (varargs) {
    if (emitter->rv->num_int_arg_regs < X86_64_NUM_INT_ARGS) {
      // All args other than those declared and in registers must
      // be saved to the stack above the frame pointer and directly
      // under the first pushed arg.  This adds to the stack frame size.
      stack_frame_size += (X86_64_NUM_INT_ARGS - emitter->rv->num_int_arg_regs) * 8;
    }
  }
  // A non-leaf procedure saves register variables on the stack as these
  // will be in saved registers.
  // if (!is_leaf) {
  //  stack_frame_size += emitter->rv->num_int_reg_vars*8 +
  //  emitter->rv->num_fp_reg_vars * 8;
  //}

  stack_frame_size += BitSetCount(&emitter->regs->used_int_regs) * 8;
  stack_frame_size += BitSetCount(&emitter->regs->used_float_regs) * 8;
  stack_frame_size += emitter->spill_region_size;
  
  stack_frame_size = (stack_frame_size + 15) & ~15;  // Aligned to 16 bytes.

  return stack_frame_size;
}

static bool EmptyStackFrame(X86_64Emitter* emitter) {
  return emitter->rv->base.stack_frame_size == 0 &&
         emitter->rv->base.num_calls == 0 && !emitter->rv->not_leaf;
}

static void PrintPercentReg(FILE* fp, const char* reg);

static bool FitsMemoryDisplacement(int64_t offset);

static void DecrementStackPointer(X86_64Emitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  char buf[8];
  if (stack_frame_size <= 0) {
    return;
  }
  if (stack_frame_size <= 0x7fffffff) {
    fprintf(fp, "\tsubq ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", %%rsp\n");
  } else {
    fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                  kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\tsubq ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                  kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, ", %%rsp\n");
  }
}

static COMPILER_UNUSED void IncrementStackPointer(X86_64Emitter* emitter,
                                                  int stack_frame_size,
                                                  FILE* fp) {
  char buf[8];
  if (stack_frame_size <= 0) {
    return;
  }
  if (stack_frame_size <= 0x7fffffff) {
    fprintf(fp, "\taddq ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", %%rsp\n");
  } else {
    fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                  kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\taddq ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                  kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, ", %%rsp\n");
  }
}

// Load a floating point or integer register from the stack frame.
static COMPILER_UNUSED void LoadRegisterFromFrame(X86_64Emitter* emitter, int reg,
                                  int offset, bool is_fp,
                                  const char* symbol_name,
                                  FILE* fp) {
  char buf[256];
  const char* instruction = is_fp ? "movsd" : "movq";
  X86_64RegisterType reg_type = is_fp ? kX86_64RegTypeFloat : kX86_64RegTypeInt;
  if (FitsMemoryDisplacement(-offset)) {
    fprintf(fp, "\t%s -%d(%%rbp), ", instruction, offset);
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, reg_type, buf, sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  } else {
    fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR, kX86_64RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, "\n\tmovq %%rbp, ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                  kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\tsubq ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR, kX86_64RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                  kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\t%s (", instruction);
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                  kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "), ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, reg_type, buf, sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  }
}

static COMPILER_UNUSED void GenerateOffsetFromFrame(X86_64Emitter* emitter, int reg,
                                    int offset,
                                    const char* symbol_name,
                                    FILE* fp) {
  char buf[256];
  if (FitsMemoryDisplacement(-offset)) {
    fprintf(fp, "\tleaq -%d(%%rbp), ", offset);
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  } else {
    fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR, kX86_64RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, "\n\tmovq %%rbp, ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\tsubq ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR, kX86_64RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, kX86_64RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  }
}

// Save all used registers on the stack.
static void SaveRegisters(X86_64Emitter* emitter, FILE* fp) {
  // Entry sequence:
  // LET S = stack frame size + 8 (for frame pointer save)
  // addi sp, sp, -S   - decrement sp for stack frame
  // sd ra, S-8(sp)    - store return address
  // sd s0, S-16(sp)    - store frame pointer (s0)
  // addi s0, sp, S    - new frame pointer is original sp.

  int stack_frame_size = StackFrameSize(emitter);
  char buf1[8], buf2[8];

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
          !emitter->rv->not_leaf;
  bool varargs = emitter->rv->base.varargs;
  int space_above_frame_pointer =
      varargs ? (X86_64_NUM_INT_ARGS - emitter->rv->num_int_arg_regs) * 8 : 0;

  if (space_above_frame_pointer < 0) {
    space_above_frame_pointer = 0;
  }
  if (is_leaf) {
    fprintf(fp, "\t// Leaf procedure, no stack frame generated\n");
  }

  //
  // The calculations are reasonably complex at first glance.
  //
  // Let's take an example.
  // Let's say that the stack_frame_size is 80 bytes.  This includes
  // 16 bytes for the saved ra and s0.  There is one saved argument
  // register, 24 bytes of local variable space and 4 saved registers.
  // Assume this not a varargs procedure so space_above_frame_pointer is zero.
  //
  // The return address register (ra) is stored 8 bytes below the previous
  // stack pointer.  The stack pointer points to the top of the current
  // stack frame (80 bytes below the old stack pointer), so the offset from
  // the current stack pointer to the return address is 80 - 8 bytes.
  //
  // The frame pointer (s0) is stored immediately below the return address,
  // so this is 80 - 16 bytes from the stack pointer.
  //
  // Next we have the local variables and saved argument registers.  These
  // occupy 32 bytes and are located immediately below the saved frame pointer.
  // However, local variables are referenced (using a negative offset) from the
  // new frame pointer (s0) which is set to the old stack pointer so the local
  // variables start at -(32 + 16) bytes from s0.  A variable at offset 12 into
  // the local variables area will be at offset -(32 + 16 - 12) bytes from s0.
  //
  // Lastly we have the the callee saved registers. They are located below the
  // local variables and are referenced from the stack pointer.  The first
  // saved register is thus at 80 - 16 - 8 - 32 bytes from the stack
  // pointer.
  //
  // If you do all the calculations for the above example, the
  // stack frame looks like this.  Assume that the saved argument
  // register is a0, and we are saving s2..s5.  The number on the
  // left of the diagram is the offset from the old stack pointer
  // or the current frame pointer (s0).  The number on the right
  // is the offset from the new stack pointer (sp).
  //
  // -0   +-------------------------+ <-- s0  +80
  //      |      saved ra           |
  // -8   +-------------------------+         +72
  //      |      saved s0           |
  // -16  +-------------------------+         +64
  //      |      saved a0           |
  // -24  +-------------------------+         +56
  //      |                         |
  //      |      local vars         | { 24 bytes
  //      |                         |
  // -48  +-------------------------+         +32
  //      |      saved s2           |
  // -56  +-------------------------+         +24
  //      |      saved s3           |
  // -64  +-------------------------+         +16
  //      |      saved s4           |
  // -72  +-------------------------+         +8
  //      |      saved s5           |
  // -80  +-------------------------+ <-- sp  +0
  //
  // If this is a varargs procedure we save all the integer
  // registers not declared as arguments on the stack above the
  // new frame pointer (s0).  This adjusts all the offets from the
  // stack pointer by the number of registers saved * 8.

  // Frame pointer and return address are positive offsets from the
  // decremented stack pointer.

  // Local vars are referenced as a negative offset from rbp and are immediately
  // below the saved argument registers.  This is the low address of the
  // start of the local variable region on the stack.
  int local_vars = emitter->rv->base.stack_frame_size +
                     X86_64_STACK_FRAME_HEADER_SIZE;

  // Offset from sp of first saved register.
  int saved_reg_offset = stack_frame_size - X86_64_STACK_FRAME_HEADER_SIZE - 8 -
                          emitter->rv->base.stack_frame_size -
                          space_above_frame_pointer -
                          emitter->spill_region_size;  // First saved register.

  // A leaf procedure doesn't save the return address.
  if (is_leaf) {
    saved_reg_offset += 8;
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame, no need to store frame pointer.
  } else {
    fprintf(fp, "\tpushq %%rbp\n");
    fprintf(fp, "\tleaq 8(%%rsp), %%rbp\n");
    int remaining = stack_frame_size - 8;
    if (remaining > 0) {
      DecrementStackPointer(emitter, remaining, fp);
    }

    if (varargs) {
      fprintf(fp, "\t// varargs function: save integer argument registers\n");
      for (int i = 0; i < X86_64_NUM_INT_ARGS; i++) {
        fprintf(fp, "\tmovq ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(
                                X86_64_INT_ARG_START + i, kX86_64RegTypeInt,
                                buf1, sizeof(buf1)));
        fprintf(fp, ", %d(%%rbp)\n", i * 8);
      }
    }
  }


  if (emitter->rv->saved_regs.length > 0) {
    fprintf(fp, "\t// Saved argument registers.\n");
  }
  for (size_t i = 0; i < emitter->rv->saved_regs.length; i++) {
    SavedArgumentRegister* saved_reg = emitter->rv->saved_regs.value.p[i];
    int offset = saved_reg->offset;
    fprintf(fp, "\tmovq ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(saved_reg->reg_num,
                                                  kX86_64RegTypeInt, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(", offset);
    PrintPercentReg(fp, X86_64RegisterNameFromNum(saved_reg->base_reg_num,
                                                  kX86_64RegTypeInt, buf2,
                                                  sizeof(buf2)));
    fprintf(fp, ")\n");
  }
  
  if (!is_leaf) {
    fprintf(fp, "\t// Local vars at offset -%d(rbp)\n", local_vars);
  }
  
  // Space for spilled registers.
   
  // Spill region is below local vars and is a positive number subtracted
  // from the frame pointer.  So the first spill offset is 8 bytes less
  // than the local vars end.
  //
  int spilled_region_hi_addr = local_vars;
  int first_spill_offset = spilled_region_hi_addr + 8;
  emitter->first_spill_offset = first_spill_offset;
  if (emitter->spill_region_size > 0) {
     fprintf(fp, "\t// Spilled register region: %d bytes at -%d(rbp) to -%d(rbp)\n",
             emitter->spill_region_size,
             first_spill_offset + emitter->spill_region_size - 8,
             first_spill_offset - 8);
   }


  // Record offset for last saved register for reloading.
  emitter->saved_reg_offset = saved_reg_offset;
  
  BitSetIterator it;

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  if (!BitSetIteratorDone(&it)) {
    fprintf(fp, "\t// Saved integer registers.\n");
  }
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    int offset = saved_reg_offset;
    saved_reg_offset -= 8;
    fprintf(fp, "\tmovq ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, kX86_64RegTypeInt, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(%%rsp)\n", offset);
    BitSetIteratorNext(&it);
  }
  
  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  if (!BitSetIteratorDone(&it)) {
    fprintf(fp, "\t// Saved floating point registers.\n");
  }
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    int offset = saved_reg_offset;
    saved_reg_offset -= 8;
    fprintf(fp, "\tstoresd ");
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, kX86_64RegTypeFloat, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(%%rsp)\n", offset);
    BitSetIteratorNext(&it);
  }

  if (!is_leaf) {
    fprintf(fp, "\t// End of stack frame\n");
  }
}

static void RestoreRegisters(X86_64Emitter* emitter, FILE* fp) {
  // Exit sequence: reload callee-saved registers, then pop the frame.

  char buf1[8];

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
          !emitter->rv->not_leaf;
  bool varargs = emitter->rv->base.varargs;
  int space_above_frame_pointer =
      varargs ? (X86_64_NUM_INT_ARGS - emitter->rv->num_int_arg_regs) * 8 : 0;

  if (space_above_frame_pointer < 0) {
    space_above_frame_pointer = 0;
  }
  (void)space_above_frame_pointer;

  if (!is_leaf) {
    fprintf(fp, "\t// Restored registers.\n");
  }

  int offset = emitter->saved_reg_offset;

  BitSetIterator it;

  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tmovsd %d(%%rsp), ", offset);
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, kX86_64RegTypeFloat, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, "\n");
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tmovq %d(%%rsp), ", offset);
    PrintPercentReg(fp, X86_64RegisterNameFromNum(reg, kX86_64RegTypeInt, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, "\n");
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame.
  } else {
    fprintf(fp, "\tleaq -8(%%rbp), %%rsp\n");
    fprintf(fp, "\tpopq %%rbp\n");
  }
}

static bool FitsMemoryDisplacement(int64_t offset) {
  return offset >= INT32_MIN && offset <= INT32_MAX;
}

static bool MemoryOffsetFromOperand(TargetInstruction* offset_inst,
                                    int64_t* offset_out) {
  if (offset_inst == NULL) {
    return false;
  }
  if (TargetIsConst(offset_inst)) {
    *offset_out = TargetIntValue(offset_inst);
    return true;
  }
  if ((X86_64Opcode)offset_inst->opcode == X86_64_OP(add)) {
    if (offset_inst->operand[0] != NULL &&
        TargetIsConst(offset_inst->operand[0]) &&
        offset_inst->operand[1] != NULL &&
        TargetIsConst(offset_inst->operand[1])) {
      *offset_out = TargetIntValue(offset_inst->operand[0]) +
                    TargetIntValue(offset_inst->operand[1]);
      return true;
    }
    if (offset_inst->operand[1] != NULL &&
        TargetIsConst(offset_inst->operand[1])) {
      *offset_out = TargetIntValue(offset_inst->operand[1]);
      return true;
    }
  }
  return false;
}

static bool MemoryBaseFromOffsetOperand(TargetInstruction* offset_inst,
                                        TargetInstruction** base_out) {
  if (offset_inst == NULL) {
    return false;
  }
  if ((X86_64Opcode)offset_inst->opcode == X86_64_OP(add) &&
      offset_inst->operand[0] != NULL &&
      offset_inst->operand[1] != NULL &&
      TargetIsConst(offset_inst->operand[1]) &&
      !TargetIsConst(offset_inst->operand[0])) {
    *base_out = offset_inst->operand[0];
    return true;
  }
  return false;
}

static const char* GetRegisterName(TargetInstruction* inst, char* buf, size_t size) {
  if (inst == NULL) {
    return "";
  }
  if (inst->reg != NULL) {
    return X86_64RegisterName((X86_64Register*)inst->reg, buf, size);
  }
  if (inst->dest != NULL && inst->dest->block != NULL &&
      inst->dest->reg != NULL) {
    return X86_64RegisterName((X86_64Register*)inst->dest->reg, buf, size);
  }
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(add):
    case X86_64_OP(addl):
    case X86_64_OP(sub):
    case X86_64_OP(subl):
      if (inst->operand[0] != NULL) {
        return GetRegisterName(inst->operand[0], buf, size);
      }
      return "";
    case X86_64_OP(x0):
      return X86_64RegisterNameFromNum(X86_64_INT_ZERO_REG, kX86_64RegTypeInt, buf,
                                       size);
    case X86_64_OP(t0):
      return X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1, kX86_64RegTypeInt,
                                       buf, size);
    case X86_64_OP(fp):
      return X86_64RegisterNameFromNum(X86_64_FP_REG, kX86_64RegTypeInt, buf, size);
    case X86_64_OP(sp):
      return X86_64RegisterNameFromNum(X86_64_SP_REG, kX86_64RegTypeInt, buf, size);
    case X86_64_OP(a0):
    case X86_64_OP(a1):
    case X86_64_OP(a2):
    case X86_64_OP(a3):
    case X86_64_OP(a4):
    case X86_64_OP(a5):
    case X86_64_OP(a6):
    case X86_64_OP(a7):
      return X86_64RegisterNameFromNum(
          (int)inst->opcode - X86_64_OP(a0) + X86_64_INT_ARG_START,
          kX86_64RegTypeInt, buf, size);
    case X86_64_OP(fa0):
    case X86_64_OP(fa1):
    case X86_64_OP(fa2):
    case X86_64_OP(fa3):
    case X86_64_OP(fa4):
    case X86_64_OP(fa5):
    case X86_64_OP(fa6):
    case X86_64_OP(fa7):
      return X86_64RegisterNameFromNum(
          (int)inst->opcode - X86_64_OP(fa0) + X86_64_FP_ARG_START,
          kX86_64RegTypeFloat, buf, size);
    case X86_64_OP(resulti):
    case X86_64_OP(call):
    case X86_64_OP(rcall):
      return X86_64RegisterNameFromNum(X86_64_INT_RETURN_REG, kX86_64RegTypeInt,
                                       buf, size);
    case X86_64_OP(resultf):
    case X86_64_OP(resultd):
    case X86_64_OP(callf):
    case X86_64_OP(rcallf):
      return X86_64RegisterNameFromNum(X86_64_FLOAT_RETURN_REG, kX86_64RegTypeFloat,
                                       buf, size);
    default:
      return "";
  }
}

static void PrintPercentReg(FILE* fp, const char* reg) {
  if (reg[0] == '%') {
    fprintf(fp, "%s", reg);
  } else {
    fprintf(fp, "%%%s", reg);
  }
}

static bool IsFloatRegisterInst(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  if (inst->reg != NULL) {
    return ((X86_64Register*)inst->reg)->type == kX86_64RegTypeFloat;
  }
  return X86_64IsFloatingPoint(inst);
}

static const char* MoveMnemonicForInst(TargetInstruction* src,
                                       TargetInstruction* dest) {
  if (dest != NULL && dest->reg != NULL &&
      ((X86_64Register*)dest->reg)->type == kX86_64RegTypeInt) {
    if (IsFloatRegisterInst(src)) {
      return "movq_xmm";
    }
    return "movq";
  }
  if (IsFloatRegisterInst(src) || IsFloatRegisterInst(dest)) {
    return "movsd";
  }
  return "movq";
}

static void PrintPercentRegFromInst(FILE* fp, TargetInstruction* inst,
                                    char* buf, size_t len) {
  PrintPercentReg(fp, GetRegisterName(inst, buf, len));
}

static void PrintResultRegFromInst(FILE* fp, TargetInstruction* inst, char* buf,
                                   size_t len) {
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    PrintPercentReg(fp, X86_64RegisterName((X86_64Register*)inst->dest->reg, buf,
                                           len));
  } else {
    PrintPercentRegFromInst(fp, inst, buf, len);
  }
}

static void PrintMemoryBaseRegFromInst(FILE* fp, TargetInstruction* inst,
                                       char* buf, size_t len) {
  const char* reg = GetRegisterName(inst, buf, len);
  if (reg[0] == '\0' && inst != NULL) {
    char buf2[8];
    switch ((X86_64Opcode)inst->opcode) {
      case X86_64_OP(add):
      case X86_64_OP(addl):
      case X86_64_OP(sub):
      case X86_64_OP(subl):
        if (inst->operand[0] != NULL) {
          reg = GetRegisterName(inst->operand[0], buf2, sizeof(buf2));
        }
        break;
      default:
        break;
    }
  }
  if (reg[0] == '\0') {
    reg = "rbp";
  }
  PrintPercentReg(fp, reg);
}

static X86_64RegisterType InstResultRegType(TargetInstruction* inst) {
  if (inst == NULL) {
    return kX86_64RegTypeInt;
  }
  if (inst->reg != NULL) {
    return ((X86_64Register*)inst->reg)->type;
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return ((X86_64Register*)inst->dest->reg)->type;
  }
  if (X86_64IsFloatingPoint(inst)) {
    return kX86_64RegTypeFloat;
  }
  return kX86_64RegTypeInt;
}

static bool OperandHasIntReg(TargetInstruction* op) {
  if (op == NULL) {
    return false;
  }
  if (op->reg != NULL) {
    return ((X86_64Register*)op->reg)->type == kX86_64RegTypeInt;
  }
  return false;
}

static const char* BranchMnemonic(X86_64Opcode opcode) {
  return X86_64OpcodeName((int)opcode);
}

static const char* AttMnemonic(X86_64Opcode opcode) {
  switch (opcode) {
    case X86_64_OP(imul):
      return "imulq";
    case X86_64_OP(imull):
      return "imull";
    case X86_64_OP(idiv):
      return "idivq";
    case X86_64_OP(div):
      return "divq";
    case X86_64_OP(mod):
      return "idivq";
    case X86_64_OP(not):
      return "notq";
    case X86_64_OP(neg):
      return "negq";
    case X86_64_OP(cmp):
      return "cmpq";
    case X86_64_OP(test):
      return "testq";
    case X86_64_OP(movslq):
      return "cltq";
    case X86_64_OP(fmv_s):
      return "movss";
    case X86_64_OP(fmv_d):
      return "movsd";
    case X86_64_OP(movq_xmm):
      return "movq";
    default:
      return X86_64OpcodeName((int)opcode);
  }
}

static void PrintAttOperand(FILE* fp, TargetInstruction* op, char* buf,
                            size_t len) {
  if (op == NULL) {
    return;
  }
  if (TargetIsConst(op)) {
    PrintAsmImmediate(fp, TargetIntValue(op));
  } else if (((int)op->opcode == (int)X86_64_OP(symbol))) {
    if ((op->flags & X86_64_HI_RELOC) != 0) {
      fprintf(fp, "%%hi(%s)", ((TargetSymbol*)op)->symbol->name.value);
    } else {
      fprintf(fp, "%s", ((TargetSymbol*)op)->symbol->name.value);
    }
  } else if (((int)op->opcode == (int)X86_64_OP(literal))) {
    TargetLiteral* literal = (TargetLiteral*)op;
    fprintf(fp, ".str.%d", literal->literal_id);
  } else if (((int)op->opcode == (int)X86_64_OP(x0))) {
    fprintf(fp, "$0");
  } else {
    PrintPercentRegFromInst(fp, op, buf, len);
  }
}

static void PrintMovToDestIfNeeded(FILE* fp, TargetInstruction* inst,
                                   char* buf1, char* buf2) {
  if (inst->operand[0] != NULL &&
      inst->operand[0]->reg != NULL &&
      inst->reg != NULL &&
      inst->operand[0]->reg != inst->reg) {
    const char* mov = "movq";
    if (X86_64IsFloatingPoint(inst)) {
      bool is_double =
          (X86_64Opcode)inst->opcode == X86_64_OP(fmv_d) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(addsd) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(subsd) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(mulsd) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(divsd) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(ucomisd) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(cvtsi2sd) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(cvttsd2si) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(cvtss2sd) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(cvtsd2ss) ||
          (X86_64Opcode)inst->opcode == X86_64_OP(fneg_sd);
      if (((X86_64Register*)inst->operand[0]->reg)->type == kX86_64RegTypeInt &&
          inst->reg != NULL &&
          ((X86_64Register*)inst->reg)->type == kX86_64RegTypeFloat) {
        mov = "movq_xmm";
      } else {
        mov = is_double ? "movsd" : "movss";
      }
    }
    fprintf(fp, "\t%s ", mov);
    PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
    fprintf(fp, "\n");
  }
}

static void PrepareSseSourceOperand(FILE* fp, TargetInstruction* op, char* buf,
                                    size_t len) {
  if (op != NULL && op->reg != NULL &&
      ((X86_64Register*)op->reg)->type == kX86_64RegTypeInt) {
    fprintf(fp, "\tmovq_xmm ");
    PrintPercentRegFromInst(fp, op, buf, len);
    fprintf(fp, ", %%xmm15\n");
  }
}

static void PrintSseSourceOperand(FILE* fp, TargetInstruction* op, char* buf,
                                  size_t len) {
  if (op == NULL) {
    return;
  }
  if (op->reg != NULL &&
      ((X86_64Register*)op->reg)->type == kX86_64RegTypeInt) {
    PrintPercentReg(fp, "xmm15");
    return;
  }
  PrintAttOperand(fp, op, buf, len);
}

static void PrintBinaryRegOp(FILE* fp, const char* mnemonic,
                             TargetInstruction* inst, char* buf1, char* buf2) {
  PrintMovToDestIfNeeded(fp, inst, buf1, buf2);
  if (X86_64IsFloatingPoint(inst) && inst->operand[1] != NULL) {
    PrepareSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, "\t%s ", mnemonic);
  if (X86_64IsFloatingPoint(inst)) {
    PrintSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  } else {
    PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, ", ");
  PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
  fprintf(fp, "\n");
}

static void PrintCompareAndSet(FILE* fp, const char* set_mnemonic,
                               TargetInstruction* inst, char* buf1,
                               char* buf2) {
  if (inst->operand[1] != NULL) {
    fprintf(fp, "\tcmp ");
    // AT&T syntax: cmp $imm, %reg (assembler expects reg destination, imm source).
    PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintAttOperand(fp, inst->operand[1], buf2, sizeof(buf2));
    fprintf(fp, "\n");
  } else if (inst->operand[0] != NULL) {
    fprintf(fp, "\ttest ");
    PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintAttOperand(fp, inst->operand[0], buf2, sizeof(buf2));
    fprintf(fp, "\n");
  }
  fprintf(fp, "\t%s ", set_mnemonic);
  PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
  fprintf(fp, "\n");
}

static void PrintIdivFamily(FILE* fp, X86_64Opcode opcode,
                            TargetInstruction* inst, char* buf1, char* buf2) {
  if (inst->operand[0] != NULL) {
    fprintf(fp, "\tmovq ");
    PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", %%rax\n");
  }
  if ((X86_64Opcode)opcode == X86_64_OP(div) ||
      ((X86_64Opcode)opcode == X86_64_OP(mod) &&
       (inst->flags & X86_64_UNSIGNED_MOD) != 0)) {
    fprintf(fp, "\txorq %%rdx, %%rdx\n");
  } else {
    fprintf(fp, "\tcqo\n");
  }
  const char* div_mnemonic = AttMnemonic(opcode);
  if ((X86_64Opcode)opcode == X86_64_OP(mod) &&
      (inst->flags & X86_64_UNSIGNED_MOD) != 0) {
    div_mnemonic = "divq";
  }
  fprintf(fp, "\t%s ", div_mnemonic);
  PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  fprintf(fp, "\n");
  if (inst->reg != NULL) {
    const char* result =
        (X86_64Opcode)opcode == X86_64_OP(mod) ? "%%rdx" : "%%rax";
    fprintf(fp, "\tmovq %s, ", result);
    PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
    fprintf(fp, "\n");
  }
}

static void PrintSseIntConvert(FILE* fp, const char* mnemonic,
                               TargetInstruction* inst, bool int_to_xmm,
                               char* buf1, char* buf2) {
  if (!int_to_xmm && inst->operand[0] != NULL) {
    PrepareSseSourceOperand(fp, inst->operand[0], buf1, sizeof(buf1));
  }
  fprintf(fp, "\t%s ", mnemonic);
  if (int_to_xmm) {
    PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
  } else {
    PrintSseSourceOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
  }
  fprintf(fp, "\n");
}

static void PrintDefaultInstruction(FILE* fp, TargetInstruction* inst,
                                    char* buf1, char* buf2) {
  X86_64Opcode opcode = (X86_64Opcode)inst->opcode;

  switch (opcode) {
    case X86_64_OP(imul):
    case X86_64_OP(imull):
    case X86_64_OP(addss):
    case X86_64_OP(addsd):
    case X86_64_OP(subss):
    case X86_64_OP(subsd):
    case X86_64_OP(mulss):
    case X86_64_OP(mulsd):
    case X86_64_OP(divss):
    case X86_64_OP(divsd):
      PrintBinaryRegOp(fp, AttMnemonic(opcode), inst, buf1, buf2);
      return;

    case X86_64_OP(idiv):
    case X86_64_OP(div):
    case X86_64_OP(mod):
      PrintIdivFamily(fp, opcode, inst, buf1, buf2);
      return;

    case X86_64_OP(not):
    case X86_64_OP(neg):
    case X86_64_OP(sqrtss):
    case X86_64_OP(sqrtsd):
    case X86_64_OP(fneg_ss):
    case X86_64_OP(fneg_sd):
      PrintMovToDestIfNeeded(fp, inst, buf1, buf2);
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      return;

    case X86_64_OP(cmp):
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintAttOperand(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;

    case X86_64_OP(test):
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintAttOperand(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;

    case X86_64_OP(setl):
      PrintCompareAndSet(fp, "setl", inst, buf1, buf2);
      return;
    case X86_64_OP(setb):
      PrintCompareAndSet(fp, "setb", inst, buf1, buf2);
      return;
    case X86_64_OP(setg):
      PrintCompareAndSet(fp, "setg", inst, buf1, buf2);
      return;
    case X86_64_OP(sete):
      PrintCompareAndSet(fp, "sete", inst, buf1, buf2);
      return;
    case X86_64_OP(setne):
      PrintCompareAndSet(fp, "setne", inst, buf1, buf2);
      return;

    case X86_64_OP(ucomiss):
    case X86_64_OP(ucomisd):
      PrepareSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
      PrepareSseSourceOperand(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintSseSourceOperand(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;

    case X86_64_OP(cvtsi2ss):
    case X86_64_OP(cvtsi2sd):
      PrintSseIntConvert(fp, AttMnemonic(opcode), inst, true, buf1, buf2);
      return;
    case X86_64_OP(cvttss2si):
    case X86_64_OP(cvttsd2si):
      PrintSseIntConvert(fp, AttMnemonic(opcode), inst, false, buf1, buf2);
      return;

    case X86_64_OP(cvtss2sd):
    case X86_64_OP(cvtsd2ss):
      PrintBinaryRegOp(fp, AttMnemonic(opcode), inst, buf1, buf2);
      return;

    case X86_64_OP(movq_xmm):
    case X86_64_OP(fmv_s):
    case X86_64_OP(fmv_d):
      if (((X86_64Opcode)opcode == X86_64_OP(fmv_s) ||
           (X86_64Opcode)opcode == X86_64_OP(fmv_d)) &&
          TargetIsConst(inst->operand[0]) &&
          InstResultRegType(inst) == kX86_64RegTypeFloat) {
        fprintf(fp, "\tmovq ");
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
        fprintf(fp, ", %%r10\n\tmovq_xmm %%r10, ");
        PrintResultRegFromInst(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
        return;
      }
      if ((X86_64Opcode)opcode == X86_64_OP(movq_xmm) &&
          inst->operand[0] != NULL &&
          inst->reg != NULL &&
          ((X86_64Register*)inst->reg)->type == kX86_64RegTypeFloat) {
        if (TargetIsConst(inst->operand[0])) {
          fprintf(fp, "\tmovq ");
          PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
          fprintf(fp, ", %%r10\n\tmovq_xmm %%r10, ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
        if ((int)inst->operand[0]->opcode == (int)X86_64_OP(x0)) {
          fprintf(fp, "\tmovq_xmm ");
          PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
        if (inst->operand[0]->reg != NULL &&
            ((X86_64Register*)inst->operand[0]->reg)->type ==
                kX86_64RegTypeInt) {
          fprintf(fp, "\tmovq_xmm ");
          PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      /* Fall through for generic movq_xmm / movss / movsd emission. */
    case X86_64_OP(movss):
    case X86_64_OP(movsd):
    case X86_64_OP(movd): {
      X86_64RegisterType dest_type = kX86_64RegTypeFloat;
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        dest_type = ((X86_64Register*)inst->dest->reg)->type;
      } else if (inst->reg != NULL) {
        dest_type = ((X86_64Register*)inst->reg)->type;
      }
      if (dest_type == kX86_64RegTypeInt) {
        if (inst->operand[0] != NULL) {
          if (TargetIsConst(inst->operand[0])) {
            fprintf(fp, "\tmovq ");
            PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          if (inst->operand[0]->reg != NULL &&
              ((X86_64Register*)inst->operand[0]->reg)->type ==
                  kX86_64RegTypeInt) {
            fprintf(fp, "\tmovq ");
            PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          if ((int)inst->operand[0]->opcode == (int)X86_64_OP(x0)) {
            fprintf(fp, "\tmovq ");
            PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          fprintf(fp, "\tmovq_xmm ");
          PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      if (inst->reg != NULL &&
          ((X86_64Register*)inst->reg)->type == kX86_64RegTypeFloat &&
          inst->operand[0] != NULL) {
        if (TargetIsConst(inst->operand[0])) {
          fprintf(fp, "\tmovq ");
          PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
          fprintf(fp, ", %%r10\n\tmovq_xmm %%r10, ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
        if ((inst->operand[0]->reg != NULL &&
             ((X86_64Register*)inst->operand[0]->reg)->type ==
                 kX86_64RegTypeInt) ||
            (int)inst->operand[0]->opcode == (int)X86_64_OP(x0)) {
          fprintf(fp, "\tmovq_xmm ");
          PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      const char* mov = AttMnemonic(opcode);
      fprintf(fp, "\t%s ", mov);
      PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      return;
    }

    case X86_64_OP(movslq):
      fprintf(fp, "\tcltq\n");
      return;

    default: {
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      if (inst->reg != NULL) {
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      }
      for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
        if (inst->operand[i] != NULL) {
          fprintf(fp, ", ");
          PrintAttOperand(fp, inst->operand[i], buf1, sizeof(buf1));
        }
      }
      fprintf(fp, "\n");
      return;
    }
  }
}

static void PrintDestMove(TargetInstruction* inst, FILE* fp) {
  TargetInstruction* src = inst->operand[0];
  assert(src != NULL);
  if (src->block == NULL) {
    return;
  }
  assert(src->reg != NULL);
  TargetRegister* dest_reg = inst->reg;
  if (dest_reg == NULL && inst->dest != NULL) {
    dest_reg = inst->dest->reg;
  }
  if (dest_reg == NULL) {
    return;
  }
  if (dest_reg == src->reg) {
    return;
  }

  char buf1[8], buf2[8];
  const char* mov = MoveMnemonicForInst(src, inst->dest);
  fprintf(fp, "\t%s ", mov);
  PrintAttOperand(fp, src, buf1, sizeof(buf1));
  fprintf(fp, ", ");
  PrintPercentReg(fp,
                  X86_64RegisterName((X86_64Register*)dest_reg, buf2,
                                     sizeof(buf2)));
  fprintf(fp, "\n");
}

static void PrintRmov(X86_64Emitter* emitter, TargetInstruction* inst, FILE* fp) {
  assert(inst->operand[0] != NULL);
  assert(inst->operand[1] != NULL);
  if (inst->operand[1]->block == NULL) {
    // Optimized out.
    return;
  }
  assert(inst->operand[0]->reg != NULL);
  assert(inst->operand[1]->reg != NULL);
  
  // Don't output mov rx,rx.
  if (inst->operand[0]->reg == inst->operand[1]->reg) {
    return;
  }

  char buf1[8], buf2[8];
  const char* mov = MoveMnemonicForInst(inst->operand[1], inst->operand[0]);
  fprintf(fp, "\t%s ", mov);
  PrintPercentReg(fp,
                  X86_64RegisterName((X86_64Register*)inst->operand[1]->reg, buf1,
                                     sizeof(buf1)));
  fprintf(fp, ", ");
  PrintPercentReg(fp,
                  X86_64RegisterName((X86_64Register*)inst->operand[0]->reg, buf2,
                                     sizeof(buf2)));
  fprintf(fp, "\n");
}

// Main instruction printer.
static void PrintInstruction(X86_64Emitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    TargetBasicBlock* b = inst->block;
    fprintf(fp, "\n\t// *** Basic block %zd\n\n", b->block_id);
    emitter->current_block = inst->block;
  }
  if (((int)inst->opcode == (int)X86_64_OP(label))) {
    if ((inst->flags & X86_64_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    return;
  }

  if (((int)inst->opcode == (int)X86_64_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }
  
  if (((int)inst->opcode == (int)X86_64_OP(ivarreg)) || ((int)inst->opcode == (int)X86_64_OP(fvarreg))) {
    return;
  }
  if (!IsPrintable(inst)) {
    return;
  }
  
  const bool show_id = 1;

  if (show_id) {
    fprintf(fp, "/* @%d */ ", inst->id);
  }
  
  // Buffers for register name printing.
  char buf1[8];
  char buf2[8];

  // Special case instructions.
  switch ((X86_64Opcode)inst->opcode) {
//    case X86_64_OP(rmov):
//    case X86_64_OP(rmovf):
//    case X86_64_OP(rmovd):
//      PrintRmov(emitter, inst, fp);
//      return;

    case X86_64_OP(mv):
      if (inst->dest != NULL) {
        PrintDestMove(inst, fp);
        return;
      }
      // Don't emit mv x, x.
      if (inst->operand[0]->reg == inst->reg) {
        return;
      }
      if (inst->operand[1] != NULL) {
        PrintRmov(emitter, inst, fp);
        return;
      }
      break;
    case X86_64_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n", sym->symbol->name.value);
      } else {
        fprintf(fp, "\t.global %s\n", sym->symbol->name.value);
      }
      return;
    }
    case X86_64_OP(call):
    case X86_64_OP(callf): {
      assert(((int)inst->operand[0]->opcode == (int)X86_64_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      if (compiler->pic) {
        fprintf(fp, "\t%-12s%s@plt\n", "call", sym->symbol->name.value);
      } else {
        fprintf(fp, "\t%-12s%s\n", "call", sym->symbol->name.value);
      }
      return;
    }

    case X86_64_OP(rcall):
    case X86_64_OP(rcallf):
      fprintf(fp, "\tcall *");
      PrintPercentRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;
    case X86_64_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case X86_64_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case X86_64_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t%s\n", lit->value.value);
      lit->base.disabled = true;
      return;
    }

    case X86_64_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }

    case X86_64_OP(spill): {
      X86_64Register* reg = (X86_64Register*)inst->reg;
      int offset = (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      const char* mov = reg->type == kX86_64RegTypeInt ? "movq" : "storesd";
      if (FitsMemoryDisplacement(-offset)) {
        fprintf(fp, "\t%s ", mov);
        PrintPercentReg(fp, X86_64RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, ", -%d(%%rbp)\t// Spilled @%d\n", offset,
                inst->operand[0]->id);
      } else {
        fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR,
                                                      kX86_64RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, "\n\tmovq %%rbp, ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                      kX86_64RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\tsubq ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR,
                                                      kX86_64RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, ", ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                      kX86_64RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\t%s ", mov);
        PrintPercentReg(fp, X86_64RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, ", (");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                      kX86_64RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, ")\t// Spilled @%d\n", inst->operand[0]->id);
      }
      return;
    }
      
    case X86_64_OP(reload): {
      X86_64Register* reg = (X86_64Register*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      const char* mov = reg->type == kX86_64RegTypeInt ? "movq" : "movsd";
      if (FitsMemoryDisplacement(-offset)) {
        fprintf(fp, "\t%s -%d(%%rbp), ", mov, offset);
        PrintPercentReg(fp, X86_64RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, "\t// Reloaded spilled @%d\n", spill->operand[0]->id);
      } else {
        fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR,
                                                      kX86_64RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, "\n\tmovq %%rbp, ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                      kX86_64RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\tsubq ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_SPILL_ADDR,
                                                      kX86_64RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, ", ");
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                      kX86_64RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\t%s (", mov);
        PrintPercentReg(fp, X86_64RegisterNameFromNum(X86_64_INT_TEMP_START_1,
                                                      kX86_64RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "), ");
        PrintPercentReg(fp, X86_64RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, "\t// Reloaded spilled @%d\n", spill->operand[0]->id);
      }
      return;
    }
    default:
      break;
  }

  // Print operands for the remaining instruction forms.
  switch ((X86_64Opcode)inst->opcode) {
    case X86_64_OP(loadl):
    case X86_64_OP(loadw):
    case X86_64_OP(loadb):
    case X86_64_OP(loadb_z):
    case X86_64_OP(loadw_z):
    case X86_64_OP(loadss):
    case X86_64_OP(loadq):
    case X86_64_OP(loadsd): {
      const char* mov = "movq";
      switch ((X86_64Opcode)inst->opcode) {
        case X86_64_OP(loadl):
          mov = "movl";
          break;
        case X86_64_OP(loadss):
          mov = "movss";
          break;
        case X86_64_OP(loadsd):
          mov = "movsd";
          break;
        case X86_64_OP(loadb):
        case X86_64_OP(loadb_z):
          mov = "movb";
          break;
        case X86_64_OP(loadw):
        case X86_64_OP(loadw_z):
          mov = "movw";
          break;
        default:
          break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      if (((X86_64Opcode)inst->opcode == X86_64_OP(loadss) ||
           (X86_64Opcode)inst->opcode == X86_64_OP(loadsd)) &&
          InstResultRegType(inst) == kX86_64RegTypeInt) {
        mov = ((X86_64Opcode)inst->opcode == X86_64_OP(loadsd)) ? "movq" : "movl";
      }
      fprintf(fp, "\t%s ", mov);
      int64_t offset = 0;
      TargetInstruction* base = inst->operand[0];
      if (MemoryBaseFromOffsetOperand(inst->operand[1], &base)) {
        MemoryOffsetFromOperand(inst->operand[1], &offset);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInst(fp, base, buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (MemoryOffsetFromOperand(inst->operand[1], &offset)) {
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (TargetIsConst(inst->operand[1])) {
        offset = TargetIntValue(inst->operand[1]);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_64_OP(symbol))) {
        fprintf(fp, "%s(", ((TargetSymbol*)inst->operand[1])->symbol->name.value);
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_64_OP(label))) {
        assert((inst->flags & X86_64_PCREL_LO_RELOC) != 0);
        fprintf(fp, ".%s_label_%d(", func_name, inst->operand[1]->id);
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_64_OP(x0))) {
        fprintf(fp, "0(");
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else {
        assert(false);
      }
      break;
    }

    case X86_64_OP(storel):
    case X86_64_OP(storew):
    case X86_64_OP(storeq):
    case X86_64_OP(storeb):
    case X86_64_OP(storess):
    case X86_64_OP(storesd): {
      const char* mov = "movq";
      switch ((X86_64Opcode)inst->opcode) {
        case X86_64_OP(storel):
          mov = "movl";
          break;
        case X86_64_OP(storess):
          mov = "storess";
          break;
        case X86_64_OP(storesd):
          mov = "storesd";
          break;
        case X86_64_OP(storeb):
          mov = "movb";
          break;
        case X86_64_OP(storew):
          mov = "movw";
          break;
        default:
          break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      if (((X86_64Opcode)inst->opcode == X86_64_OP(storesd) ||
           (X86_64Opcode)inst->opcode == X86_64_OP(storess)) &&
          OperandHasIntReg(inst->operand[0])) {
        mov = ((X86_64Opcode)inst->opcode == X86_64_OP(storesd)) ? "movq" : "movl";
      }
      fprintf(fp, "\t%s ", mov);
      if (TargetIsConst(inst->operand[0])) {
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
      } else {
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
      }
      fprintf(fp, ", ");
      int64_t offset = 0;
      TargetInstruction* base = inst->operand[1];
      if (MemoryBaseFromOffsetOperand(inst->operand[2], &base)) {
        MemoryOffsetFromOperand(inst->operand[2], &offset);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInst(fp, base, buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (MemoryOffsetFromOperand(inst->operand[2], &offset)) {
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (TargetIsConst(inst->operand[2])) {
        offset = TargetIntValue(inst->operand[2]);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_64_OP(symbol))) {
        fprintf(fp, "%s(", ((TargetSymbol*)inst->operand[2])->symbol->name.value);
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_64_OP(label))) {
        assert((inst->flags & X86_64_PCREL_LO_RELOC) != 0);
        fprintf(fp, ".%s_label_%d(", func_name, inst->operand[2]->id);
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_64_OP(x0))) {
        fprintf(fp, "0(");
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else {
        assert(false);
      }
      break;
    }
      
    case X86_64_OP(nop):
      fprintf(fp, "\tnop\n");
      break;
      
    case X86_64_OP(je):
    case X86_64_OP(jne):
    case X86_64_OP(jl):
    case X86_64_OP(jge):
    case X86_64_OP(jb):
    case X86_64_OP(jae):
      if (inst->operand[1] == NULL) {
        assert(inst->operand[0] != NULL);
        assert(((int)inst->operand[0]->opcode == (int)X86_64_OP(label)));
        fprintf(fp, "\t%s .%s_label_%d\n",
                BranchMnemonic((X86_64Opcode)inst->opcode),
                func_name, inst->operand[0]->id);
        break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[2] != NULL);
      fprintf(fp, "\tcmp ");
      PrintPercentRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
      fprintf(fp, ", ");
      PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, "\n\t%s .%s_label_%d\n",
              BranchMnemonic((X86_64Opcode)inst->opcode),
              func_name, inst->operand[2]->id);
      break;

    case X86_64_OP(jz):
    case X86_64_OP(jnz):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[0]->reg != NULL);
      fprintf(fp, "\ttest ");
      PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintPercentRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n\t%s .%s_label_%d\n",
              BranchMnemonic((X86_64Opcode)inst->opcode),
              func_name, inst->operand[1]->id);
      break;

    case X86_64_OP(jmp): {
      assert(inst->operand[0] != NULL);
      TargetInstruction* dest = inst->operand[0];
      if (((int)dest->opcode == (int)X86_64_OP(label))) {
        fprintf(fp, "\tjmp .%s_label_%d\n", func_name, dest->id);
      } else if (((int)dest->opcode == (int)X86_64_OP(symbol))) {
        fprintf(fp, "\tjmp %s\n", ((TargetSymbol*)dest)->symbol->name.value);
      } else {
        fprintf(fp, "\tjmp ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\n");
      }
      break;
    }

    case X86_64_OP(mov): {
      int64_t value = TargetIntValue(inst->operand[0]);
      fprintf(fp, "\tmovq ");
      PrintAsmImmediate(fp, value);
      fprintf(fp, ", ");
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_64_OP(ret):
      fprintf(fp, "\tret\n");
      break;

    case X86_64_OP(add):
    case X86_64_OP(addl):
    case X86_64_OP(sub):
    case X86_64_OP(subl):
    case X86_64_OP(and):
    case X86_64_OP(or):
    case X86_64_OP(xor): {
      if (inst->operand[0] == NULL || inst->operand[1] == NULL) {
        break;
      }
      const char* op = "addq";
      if ((X86_64Opcode)inst->opcode == X86_64_OP(addl)) {
        op = "addl";
      } else if ((X86_64Opcode)inst->opcode == X86_64_OP(sub)) {
        op = "subq";
      } else if ((X86_64Opcode)inst->opcode == X86_64_OP(subl)) {
        op = "subl";
      } else if ((X86_64Opcode)inst->opcode == X86_64_OP(and)) {
        op = "andq";
      } else if ((X86_64Opcode)inst->opcode == X86_64_OP(or)) {
        op = "orq";
      } else if ((X86_64Opcode)inst->opcode == X86_64_OP(xor)) {
        op = "xorq";
      }
      if (inst->operand[0]->reg != NULL &&
          inst->reg != NULL &&
          inst->operand[0]->reg != inst->reg) {
        fprintf(fp, "\tmovq ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
        fprintf(fp, "\n");
      }
      if (TargetIsConst(inst->operand[1])) {
        fprintf(fp, "\t%s ", op);
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[1]));
        fprintf(fp, ", ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else {
        fprintf(fp, "\t%s ", op);
        PrintPercentRegFromInst(fp, inst->operand[1], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
        fprintf(fp, "\n");
      }
      break;
    }

    case X86_64_OP(shl):
    case X86_64_OP(shr):
    case X86_64_OP(sar):
    case X86_64_OP(shll):
    case X86_64_OP(shrl):
    case X86_64_OP(sarl): {
      const char* op = "shlq";
      switch ((X86_64Opcode)inst->opcode) {
        case X86_64_OP(shr):
          op = "shrq";
          break;
        case X86_64_OP(sar):
          op = "sarq";
          break;
        case X86_64_OP(shll):
          op = "shll";
          break;
        case X86_64_OP(shrl):
          op = "shrl";
          break;
        case X86_64_OP(sarl):
          op = "sarl";
          break;
        default:
          break;
      }
      if (inst->operand[0]->reg != NULL &&
          inst->reg != NULL &&
          inst->operand[0]->reg != inst->reg) {
        fprintf(fp, "\tmovq ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
        fprintf(fp, "\n");
      }
      if (TargetIsConst(inst->operand[1])) {
        fprintf(fp, "\t%s ", op);
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[1]));
        fprintf(fp, ", ");
      } else {
        fprintf(fp, "\tmovq ");
        PrintPercentRegFromInst(fp, inst->operand[1], buf1, sizeof(buf1));
        fprintf(fp, ", %%rcx\n");
        fprintf(fp, "\t%s %%cl, ", op);
      }
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86_64RegisterName((X86_64Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      break;
    }

    case X86_64_OP(lea):
    case X86_64_OP(lea_rip): {
      if (inst->operand[0] == NULL || inst->operand[1] == NULL) {
        break;
      }
      fprintf(fp, "\tlea ");
      if (((int)inst->operand[0]->opcode == (int)X86_64_OP(symbol))) {
        TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
        if ((inst->flags & X86_64_GOTPCREL_RELOC) != 0) {
          fprintf(fp, "%s@GOTPCREL(%%rip), ", sym->symbol->name.value);
        } else if (((int)inst->opcode == (int)X86_64_OP(lea_rip))) {
          fprintf(fp, "%s(%%rip), ", sym->symbol->name.value);
        } else {
          fprintf(fp, "%s(", sym->symbol->name.value);
          PrintPercentRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
          fprintf(fp, "), ");
        }
      } else if (((int)inst->operand[0]->opcode == (int)X86_64_OP(literal))) {
        TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
        fprintf(fp, ".str.%d(%%rip), ", literal->literal_id);
      } else if (TargetIsConst(inst->operand[1])) {
        fprintf(fp, "%" PRId64 "(", TargetIntValue(inst->operand[1]));
        PrintPercentRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
      } else if (inst->operand[0] != NULL && inst->operand[1] != NULL) {
        int64_t offset = 0;
        TargetInstruction* base = inst->operand[0];
        if (MemoryBaseFromOffsetOperand(inst->operand[1], &base)) {
          MemoryOffsetFromOperand(inst->operand[1], &offset);
        } else if (MemoryOffsetFromOperand(inst->operand[1], &offset)) {
          base = inst->operand[0];
        } else {
          PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
          fprintf(fp, "(");
          PrintAttOperand(fp, inst->operand[0], buf2, sizeof(buf2));
          fprintf(fp, "), ");
          PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
          fprintf(fp, "\n");
          break;
        }
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInst(fp, base, buf2, sizeof(buf2));
        fprintf(fp, "), ");
      } else {
        if (inst->operand[0] != NULL && inst->operand[1] != NULL) {
          PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
          fprintf(fp, "(");
          PrintAttOperand(fp, inst->operand[0], buf2, sizeof(buf2));
          fprintf(fp, "), ");
        } else {
          assert(false);
        }
      }
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_64_OP(movabs): {
      fprintf(fp, "\tmovabs ");
      PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
      fprintf(fp, ", ");
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    default:
      PrintDefaultInstruction(fp, inst, buf1, buf2);
      break;
  }
  
}

void X86_64EmitterInit(X86_64Emitter* emitter, X86_64Generator* rv) {
  emitter->rv = rv;
  emitter->regs = &rv->register_allocator;
  emitter->saved_reg_offset = 0;
  emitter->spill_region_size = rv->register_allocator.max_spilled_region_size;
  emitter->first_spill_offset = 0;
  emitter->current_block = NULL;
}

X86_64Emitter* NewX86_64Emitter(X86_64Generator* rv) {
  X86_64Emitter* emitter = malloc(sizeof(X86_64Emitter));
  X86_64EmitterInit(emitter, rv);
  return emitter;
}

void X86_64EmitterDestruct(X86_64Emitter* emitter) {
}

void X86_64EmitterDelete(X86_64Emitter* emitter) {
  X86_64EmitterDestruct(emitter);
  free(emitter);
}


void X86_64PrintFunction(X86_64Emitter* emitter, FILE* fp) {
  const char* func_name = emitter->rv->base.function_name.value;
  if (emitter->rv->base.is_global) {
    fprintf(fp, "\t.global %s\n", func_name);
  } else {
    fprintf(fp, "\t.local  %s\n", func_name);
  }
  fprintf(fp, "\t.type %s, @function\n\n", func_name);
  fprintf(fp, "%s:\n", func_name);
  TargetInstruction* inst = TargetFirstInstruction(&emitter->rv->base);
  while (inst != NULL) {
    PrintInstruction(emitter, inst, func_name, fp);
    inst = TargetNext(inst);
  }
  fprintf(fp, ".func_end_%s:\n", func_name);
  fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
}
