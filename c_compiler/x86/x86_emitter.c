//
//  x86_emitter.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

// This is the x86-64 assembly language emitter. It prints the x86-64
// instructions to the given file in assembly language.  The x86 assembler
// reads this file and generates the binary.

#include "x86_emitter.h"
#include "x86_profile.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "common_emitter.h"
#include "compiler.h"
#include "eh_metadata.h"
#include "x86_assembler.h"
#include "x86_codegen.h"
#include "x86_machine.h"
#include "x86_reg_alloc.h"
#include "target_basic_block.h"

// In assembler mode, $ introduces a hexadecimal immediate (see CollectHex in
// lex.c), not a decimal value as in GNU as AT&T syntax.

// i386 emission path forward declarations.
static int StackFrameSizeI386(X86Emitter* emitter);
static void DecrementStackPointerI386(X86Emitter* emitter, int stack_frame_size, FILE* fp);
static void IncrementStackPointerI386(X86Emitter* emitter, int stack_frame_size, FILE* fp);
static void SaveRegistersI386(X86Emitter* emitter, FILE* fp);
static void RestoreRegistersI386(X86Emitter* emitter, FILE* fp);
static void PrintInstructionI386(X86Emitter* emitter, TargetInstruction* inst, const char* func_name, FILE* fp);
static void RestoreStackPointerAtLandingPadI386(X86Emitter* emitter, FILE* fp);
static void ReloadStructReturnRegisterAtLandingPadI386(X86Emitter* emitter, FILE* fp);
static void X86PrintEHFrameI386(X86Emitter* emitter, FILE* fp, const char* func_name);
static void X86PrintGCCExceptTableI386(X86Emitter* emitter, FILE* fp, const char* func_name);

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
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(tmp):
    case X86_OP(fp):
    case X86_OP(sp):
    case X86_OP(literal):
    case X86_OP(structreturn):
    case X86_OP(resulti):
    case X86_OP(resulth):
    case X86_OP(resultf):
    case X86_OP(resultd):
    case X86_OP(resultv):
    case X86_OP(a0):
    case X86_OP(a1):
    case X86_OP(a2):
    case X86_OP(a3):
    case X86_OP(a4):
    case X86_OP(a5):
    case X86_OP(a6):
    case X86_OP(a7):
    case X86_OP(fa0):
    case X86_OP(fa1):
    case X86_OP(fa2):
    case X86_OP(fa3):
    case X86_OP(fa4):
    case X86_OP(fa5):
    case X86_OP(fa6):
    case X86_OP(fa7):
    case X86_OP(regarg):
    case X86_OP(ivarreg):
    case X86_OP(fvarreg):
    case X86_OP(x0):
    case X86_OP(t0):
    case X86_OP(nrvoval):
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
static int StackFrameSize(X86Emitter* emitter) {
  if (!X86_IS_64BIT(emitter->rv)) return StackFrameSizeI386(emitter);
  // Start off with local variable space.  This also includes
  // 16 bytes for the saved ra and s0.
  int stack_frame_size = emitter->rv->base.stack_frame_size +
                         emitter->rv->saved_arg_area_size + 16;

  bool varargs = emitter->rv->base.varargs;

  if (varargs) {
    // Reserve an in-frame register save area for all integer argument
    // registers.  Unlike aarch64/riscv (where the return address lives in a
    // link register), x86 has the return address on the stack at [rbp+0]
    // and the saved frame pointer at [rbp-8].  The extra 8 bytes keep the save
    // area clear of those slots once rbp is lowered to expose it (see
    // GenerateProlog / RestoreRegisters).
    stack_frame_size += (X86_P(emitter->rv)->vararg_save_area_size);
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

  // The prologue pushes the frame pointer and then subtracts
  // `stack_frame_size - 8` (see SaveRegisters), so the body's %rsp sits
  // `stack_frame_size` below the entry value.  The ABI puts %rsp at a multiple
  // of 16 at each call, so entry %rsp is 8 past one (the call pushed a return
  // address) and a frame that is a multiple of 16 leaves the body 8 past a
  // multiple too -- which breaks the guarantee for every call this function
  // makes, and flips it back and forth with call depth.  Adding 8 makes the
  // prologue's subtraction a multiple of 16 instead, so the body is aligned and
  // %rbp is reliably 8 past a multiple of 16.  Every %rsp-relative offset below
  // is derived from this same value, so they all move together.
  stack_frame_size += 8;

  return stack_frame_size;
}

static bool EmptyStackFrame(X86Emitter* emitter) {
  // A frame can be elided only when there is genuinely nothing to store on the
  // stack.  In particular, if any registers (callee-saved/used registers or
  // saved argument registers) have to be spilled to the stack, they are stored
  // at positive offsets from rsp which only address valid memory once a frame
  // has been allocated.  Skipping the frame in that case would write those
  // saves above rsp, clobbering the caller's frame (e.g. the caller's own
  // saved-register slots).  StackFrameSize() already reserves space for these,
  // so they must be accounted for here too.
  return emitter->rv->base.stack_frame_size == 0 &&
         emitter->rv->base.num_calls == 0 && !emitter->rv->not_leaf &&
         !emitter->rv->has_incoming_stack_args &&
         emitter->rv->saved_regs.length == 0 &&
         BitSetCount(&emitter->regs->used_int_regs) == 0 &&
         BitSetCount(&emitter->regs->used_float_regs) == 0;
}

static void PrintPercentReg(FILE* fp, const char* reg);

static bool FitsMemoryDisplacement(int64_t offset);

static void DecrementStackPointer(X86Emitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (!X86_IS_64BIT(emitter->rv)) { DecrementStackPointerI386(emitter, stack_frame_size, fp); return; }
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
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\tsubq ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, ", %%rsp\n");
  }
}

static COMPILER_UNUSED void IncrementStackPointer(X86Emitter* emitter,
                                                  int stack_frame_size,
                                                  FILE* fp) {
  if (!X86_IS_64BIT(emitter->rv)) { IncrementStackPointerI386(emitter, stack_frame_size, fp); return; }
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
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\taddq ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, ", %%rsp\n");
  }
}

// Load a floating point or integer register from the stack frame.
static COMPILER_UNUSED void LoadRegisterFromFrame(X86Emitter* emitter, int reg,
                                  int offset, bool is_fp,
                                  const char* symbol_name,
                                  FILE* fp) {
  char buf[256];
  const char* instruction = is_fp ? "movsd" : "movq";
  X86RegisterType reg_type = is_fp ? kX86RegTypeFloat : kX86RegTypeInt;
  if (FitsMemoryDisplacement(-offset)) {
    fprintf(fp, "\t%s -%d(%%rbp), ", instruction, offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, reg_type, buf, sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  } else {
    fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr), kX86RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, "\n\tmovq %%rbp, ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\tsubq ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr), kX86RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\t%s (", instruction);
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "), ");
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, reg_type, buf, sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  }
}

static COMPILER_UNUSED void GenerateOffsetFromFrame(X86Emitter* emitter, int reg,
                                    int offset,
                                    const char* symbol_name,
                                    FILE* fp) {
  char buf[256];
  if (FitsMemoryDisplacement(-offset)) {
    fprintf(fp, "\tleaq -%d(%%rbp), ", offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  } else {
    fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr), kX86RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, "\n\tmovq %%rbp, ");
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\tsubq ");
    PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr), kX86RegTypeInt,
                                                  buf, sizeof(buf)));
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\t// %s\n", symbol_name);
  }
}

// Save all used registers on the stack.
static void SaveRegisters(X86Emitter* emitter, FILE* fp) {
  if (!X86_IS_64BIT(emitter->rv)) { SaveRegistersI386(emitter, fp); return; }
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
      varargs ? (X86_P(emitter->rv)->vararg_save_area_size) : 0;

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
                   emitter->rv->saved_arg_area_size +
                   (X86_P(emitter->rv)->stack_frame_header_size);

  // Offset from sp of first saved register.
  int saved_reg_offset = stack_frame_size - (X86_P(emitter->rv)->stack_frame_header_size) - 8 -
                          emitter->rv->base.stack_frame_size -
                          emitter->rv->saved_arg_area_size -
                          space_above_frame_pointer -
                          emitter->spill_region_size;  // First saved register.

  // On the architectures this backend was ported from, a leaf procedure keeps
  // the return address in a register and its frame needs no slot for it, so the
  // save area could start 8 bytes higher.  x86-64 has no link register: `call`
  // pushes the return address, so the two header words above rbp are there
  // whatever the function does, and moving the save area up puts the first
  // saved register on top of the last local variable.

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame, no need to store frame pointer.
  } else {
    fprintf(fp, "\tpushq %%rbp\n");
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_push:\n",
              emitter->rv->base.function_name.value);
    }
    fprintf(fp, "\tleaq 8(%%rsp), %%rbp\n");
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_leaq:\n",
              emitter->rv->base.function_name.value);
    }
    int remaining = stack_frame_size - 8;
    if (remaining > 0) {
      DecrementStackPointer(emitter, remaining, fp);
    }

    if (varargs) {
      fprintf(fp, "\t// varargs function: save integer argument registers\n");
      // Lower rbp by the reserved save-area size so the area sits inside the
      // allocated frame (below the saved frame pointer), not above rbp where
      // the return address and caller frame live.  StackFrameSize() reserved
      // the same number of bytes, so local-variable offsets (negative from
      // rbp) remain correct.  va_start reads rbp as the reg_save_area base.
      // Immediates are emitted in hex (see PrintAsmImmediate); printing this
      // with %d would be re-parsed as hex by the assembler (e.g. 72 -> 0x72).
      fprintf(fp, "\tsubq ");
      PrintAsmImmediate(fp, space_above_frame_pointer);
      fprintf(fp, ", %%rbp\n");
      for (int i = 0; i < X86_MAX_INT_ARGS; i++) {
        fprintf(fp, "\tmovq ");
        PrintPercentReg(fp, X86RegisterNameFromNum(
                                (X86_P(emitter->rv)->int_arg_start) + i, kX86RegTypeInt,
                                buf1, sizeof(buf1)));
        fprintf(fp, ", %d(%%rbp)\n", i * 8);
      }
      // Save the vector argument registers (xmm0..xmm7) after the integer save
      // area so va_arg can fetch floating-point variadic arguments.  These are
      // saved unconditionally (the caller's %al vector count is not relied
      // upon); reading slots for arguments that were never passed is harmless.
      fprintf(fp, "\t// varargs function: save vector argument registers\n");
      for (int i = 0; i < X86_MAX_FP_ARGS; i++) {
        fprintf(fp, "\tstoresd ");
        PrintPercentReg(fp, X86RegisterNameFromNum(
                                (X86_P(emitter->rv)->fp_arg_start) + i, kX86RegTypeFloat,
                                buf1, sizeof(buf1)));
        fprintf(fp, ", %d(%%rbp)\n", (X86_P(emitter->rv)->vararg_fp_save_offset) + i * 16);
      }
    }
  }


  if (emitter->rv->saved_regs.length > 0) {
    fprintf(fp, "\t// Saved argument registers.\n");
  }
  for (size_t i = 0; i < emitter->rv->saved_regs.length; i++) {
    SavedArgumentRegister* saved_reg = emitter->rv->saved_regs.value.p[i];
    int offset = saved_reg->offset;
    if (saved_reg->copy_bytes > 0) {
      // Make a local copy of a large struct argument that was passed by
      // reference: reg_num holds the pointer to the caller's copy and we copy
      // copy_bytes (rounded up to whole words; the ABI pads struct args to a
      // multiple of 8 at both ends, so reading/writing full words is safe) into
      // offset(base_reg_num).  Use r11 as the scratch register since it is
      // never an argument register and is free at this point in the prologue.
      char scbuf[8];
      const char* src = X86RegisterNameFromNum(
          saved_reg->reg_num, kX86RegTypeInt, buf1, sizeof(buf1));
      const char* base = X86RegisterNameFromNum(
          saved_reg->base_reg_num, kX86RegTypeInt, buf2, sizeof(buf2));
      const char* scratch = X86RegisterNameFromNum(
          (X86_P(emitter->rv)->spill_addr), kX86RegTypeInt, scbuf, sizeof(scbuf));
      int words = (saved_reg->copy_bytes + 7) / 8;
      for (int w = 0; w < words; w++) {
        int o = w * 8;
        fprintf(fp, "\tmovq %d(", o);
        PrintPercentReg(fp, src);
        fprintf(fp, "), ");
        PrintPercentReg(fp, scratch);
        fprintf(fp, "\n\tmovq ");
        PrintPercentReg(fp, scratch);
        fprintf(fp, ", %d(", offset + o);
        PrintPercentReg(fp, base);
        fprintf(fp, ")\n");
      }
      continue;
    }
    const char* store = "movq";
    X86RegisterType reg_type = kX86RegTypeInt;
    if (saved_reg->is_fp) {
      store = saved_reg->value_bytes == 16
                  ? "movdqu"
                  : saved_reg->value_bytes == 4 ? "storess" : "storesd";
      reg_type = kX86RegTypeFloat;
    }
    fprintf(fp, "\t%s ", store);
    PrintPercentReg(fp, X86RegisterNameFromNum(saved_reg->reg_num,
                                                  reg_type, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(", offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(saved_reg->base_reg_num,
                                                  kX86RegTypeInt, buf2,
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
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeInt, buf1,
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
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeFloat, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(%%rsp)\n", offset);
    BitSetIteratorNext(&it);
  }

  if (!is_leaf) {
    fprintf(fp, "\t// End of stack frame\n");
  }
}

static void RestoreRegisters(X86Emitter* emitter, FILE* fp) {
  if (!X86_IS_64BIT(emitter->rv)) { RestoreRegistersI386(emitter, fp); return; }
  // Exit sequence: reload callee-saved registers, then pop the frame.

  char buf1[8];

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
          !emitter->rv->not_leaf;
  bool varargs = emitter->rv->base.varargs;
  int space_above_frame_pointer =
      varargs ? (X86_P(emitter->rv)->vararg_save_area_size) : 0;

  if (space_above_frame_pointer < 0) {
    space_above_frame_pointer = 0;
  }

  if (!is_leaf) {
    fprintf(fp, "\t// Restored registers.\n");
  }

  if (!EmptyStackFrame(emitter)) {
    // A VLA or over-aligned local can leave rsp below the fixed frame. Restore
    // the frame bottom from rbp before reading rsp-relative saved registers.
    // Varargs lowers rbp into its register-save area, so exclude that area
    // from the distance back to the fixed-frame bottom.
    int frame_adjustment =
        StackFrameSize(emitter) - space_above_frame_pointer;
    fprintf(fp, "\tmovq %%rbp, %%rsp\n");
    DecrementStackPointer(emitter, frame_adjustment, fp);
  }

  int offset = emitter->saved_reg_offset;

  BitSetIterator it;

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tmovq %d(%%rsp), ", offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeInt, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, "\n");
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tmovsd %d(%%rsp), ", offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeFloat, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, "\n");
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame.
  } else {
    // rbp may have been lowered by space_above_frame_pointer in the varargs
    // prologue, so the saved frame pointer sits at space_above_frame_pointer-8
    // above the (possibly lowered) rbp.  For non-varargs this is just -8.
    fprintf(fp, "\tleaq %d(%%rbp), %%rsp\n", space_above_frame_pointer - 8);
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
  if ((X86Opcode)offset_inst->opcode == X86_OP(add)) {
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
  if ((X86Opcode)offset_inst->opcode == X86_OP(add) &&
      offset_inst->operand[0] != NULL &&
      offset_inst->operand[1] != NULL &&
      TargetIsConst(offset_inst->operand[1]) &&
      !TargetIsConst(offset_inst->operand[0]) &&
      // The zero pseudo-register is never a real memory base: an
      // add(x0, const) encodes a pure constant offset, so the base must come
      // from the instruction's own base operand, not from this add.
      (int)offset_inst->operand[0]->opcode != (int)X86_OP(x0)) {
    *base_out = offset_inst->operand[0];
    return true;
  }
  return false;
}

static const char* GetRegisterName(TargetInstruction* inst, char* buf, size_t size) {
  const X86Profile* profile = X86CurrentRegisterNameProfile();
  if (inst == NULL) {
    return "";
  }
  if (inst->reg != NULL) {
    return X86RegisterName((X86Register*)inst->reg, buf, size);
  }
  if (inst->dest != NULL && inst->dest->block != NULL &&
      inst->dest->reg != NULL) {
    return X86RegisterName((X86Register*)inst->dest->reg, buf, size);
  }
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(add):
    case X86_OP(addl):
    case X86_OP(sub):
    case X86_OP(subl):
      if (inst->operand[0] != NULL) {
        return GetRegisterName(inst->operand[0], buf, size);
      }
      return "";
    case X86_OP(x0):
      return X86RegisterNameFromNum(profile->int_zero_reg, kX86RegTypeInt, buf,
                                   size);
    case X86_OP(t0):
      return X86RegisterNameFromNum(profile->int_temp_start_1, kX86RegTypeInt,
                                   buf, size);
    case X86_OP(fp):
      return X86RegisterNameFromNum(profile->fp_reg, kX86RegTypeInt, buf, size);
    case X86_OP(sp):
      return X86RegisterNameFromNum(profile->sp_reg, kX86RegTypeInt, buf, size);
    case X86_OP(a0):
    case X86_OP(a1):
    case X86_OP(a2):
    case X86_OP(a3):
    case X86_OP(a4):
    case X86_OP(a5):
    case X86_OP(a6):
    case X86_OP(a7):
      return X86RegisterNameFromNum(
          (int)inst->opcode - X86_OP(a0) + profile->int_arg_start,
          kX86RegTypeInt, buf, size);
    case X86_OP(fa0):
    case X86_OP(fa1):
    case X86_OP(fa2):
    case X86_OP(fa3):
    case X86_OP(fa4):
    case X86_OP(fa5):
    case X86_OP(fa6):
    case X86_OP(fa7):
      return X86RegisterNameFromNum(
          (int)inst->opcode - X86_OP(fa0) + profile->fp_arg_start,
          kX86RegTypeFloat, buf, size);
    case X86_OP(resulti):
    case X86_OP(call):
    case X86_OP(rcall):
      return X86RegisterNameFromNum(profile->ret_reg, kX86RegTypeInt, buf,
                                    size);
    case X86_OP(resulth):
      return X86RegisterNameFromNum(profile->int_return_value_1, kX86RegTypeInt,
                                    buf, size);
    case X86_OP(resultf):
    case X86_OP(resultd):
    case X86_OP(resultv):
    case X86_OP(callf):
    case X86_OP(rcallf):
      return X86RegisterNameFromNum(profile->fp_return_value_0,
                                    kX86RegTypeFloat, buf, size);
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

static AsmOperand* GetAsmOperand(X86AsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    return NULL;
  }
  if ((size_t)index < inst->asm_node->outputs.length) {
    return inst->asm_node->outputs.value.p[index];
  }
  return inst->asm_node->inputs.value.p[index - inst->asm_node->outputs.length];
}

static int FindAsmOperandByName(X86AsmInstruction* inst, const char* name,
                                size_t len) {
  for (int i = 0; i < inst->num_operands; i++) {
    AsmOperand* operand = GetAsmOperand(inst, i);
    if (operand != NULL && operand->name.length == len &&
        strncmp(operand->name.value, name, len) == 0) {
      return i;
    }
  }
  return -1;
}

static int FindAsmLabelByName(X86AsmInstruction* inst, const char* name,
                              size_t len) {
  for (size_t i = 0; i < inst->asm_node->labels.length; i++) {
    String* label = inst->asm_node->labels.value.p[i];
    if (label->length == len && strncmp(label->value, name, len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void PrintAsmOperand(FILE* fp, X86AsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    fprintf(fp, "<bad-asm-operand>");
    return;
  }
  if (inst->reg_nums[index] == INT_MIN) {
    PrintAsmImmediate(fp, inst->immediate_values[index]);
    return;
  }
  char buf[32];
  PrintPercentReg(fp, X86RegisterNameFromNum(inst->reg_nums[index],
                                                inst->is_fp[index]
                                                    ? kX86RegTypeFloat
                                                    : kX86RegTypeInt,
                                                buf, sizeof(buf)));
}

static void PrintAsmLabel(FILE* fp, X86AsmInstruction* inst, int index) {
  if (index < 0 || (size_t)index >= inst->asm_node->labels.length) {
    fprintf(fp, "<bad-asm-label>");
    return;
  }
  String* label = inst->asm_node->labels.value.p[index];
  fprintf(fp, "%s", label->value);
}

static void PrintExtendedAsm(FILE* fp, X86AsmInstruction* inst,
                             const char* text) {
  for (const char* p = text; *p != '\0'; p++) {
    if (*p != '%') {
      fputc(*p, fp);
      continue;
    }
    p++;
    if (*p == '%') {
      fputc('%', fp);
      continue;
    }
    char modifier = 0;
    if (*p == 'q' || *p == 'k' || *p == 'b' || *p == 'w' || *p == 'l') {
      modifier = *p++;
    }
    int index = -1;
    if (*p == '[') {
      const char* name = ++p;
      while (*p != '\0' && *p != ']') {
        p++;
      }
      index = modifier == 'l'
                  ? FindAsmLabelByName(inst, name, (size_t)(p - name))
                  : FindAsmOperandByName(inst, name, (size_t)(p - name));
    } else if (*p >= '0' && *p <= '9') {
      index = 0;
      while (*p >= '0' && *p <= '9') {
        index = index * 10 + (*p - '0');
        p++;
      }
      p--;
    }
    if (modifier == 'l') {
      PrintAsmLabel(fp, inst, index);
    } else {
      PrintAsmOperand(fp, inst, index);
    }
  }
}

static bool IsFloatRegisterInst(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  if (inst->reg != NULL) {
    return ((X86Register*)inst->reg)->type == kX86RegTypeFloat;
  }
  return X86IsFloatingPoint(inst);
}

static const char* MoveMnemonicForInst(TargetInstruction* src,
                                       TargetInstruction* dest) {
  if (dest != NULL && dest->reg != NULL &&
      ((X86Register*)dest->reg)->type == kX86RegTypeInt) {
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
    PrintPercentReg(fp, X86RegisterName((X86Register*)inst->dest->reg, buf,
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
    switch ((X86Opcode)inst->opcode) {
      case X86_OP(add):
      case X86_OP(addl):
      case X86_OP(sub):
      case X86_OP(subl):
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

static bool PrintTlsTporffMemoryOperand(FILE* fp, TargetInstruction* addr,
                                        TargetInstruction* offset, char* buf,
                                        size_t bufsz) {
  TargetInstruction* tp = addr;
  TargetInstruction* tporff = offset;
  if ((X86Opcode)addr->opcode == X86_OP(add)) {
    tp = addr->operand[0];
    tporff = addr->operand[1];
  }
  if (tp == NULL || tporff == NULL ||
      (X86Opcode)tporff->opcode != X86_OP(movxc) ||
      (tporff->flags & X86_TLS_RELOC) == 0 || tporff->operand[0] == NULL ||
      (int)tporff->operand[0]->opcode != (int)X86_OP(symbol)) {
    return false;
  }
  char namebuf[256];
  fprintf(fp, "%s@TPOFF(", TargetSymbolName(
                               ((TargetSymbol*)tporff->operand[0])->symbol,
                               namebuf, sizeof(namebuf)));
  PrintMemoryBaseRegFromInst(fp, tp, buf, bufsz);
  fprintf(fp, ")");
  return true;
}

static X86RegisterType InstResultRegType(TargetInstruction* inst) {
  if (inst == NULL) {
    return kX86RegTypeInt;
  }
  if (inst->reg != NULL) {
    return ((X86Register*)inst->reg)->type;
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return ((X86Register*)inst->dest->reg)->type;
  }
  if (X86IsFloatingPoint(inst)) {
    return kX86RegTypeFloat;
  }
  return kX86RegTypeInt;
}

static bool OperandHasIntReg(TargetInstruction* op) {
  if (op == NULL) {
    return false;
  }
  if (op->reg != NULL) {
    return ((X86Register*)op->reg)->type == kX86RegTypeInt;
  }
  return false;
}

static const char* BranchMnemonic(X86Opcode opcode) {
  return X86OpcodeName((int)opcode);
}

// True when this operand must be emitted as an immediate (it has no register).
// The zero pseudo-register x0 is the constant 0; x86-64 has no hardware zero
// register so it cannot be used as a real register operand.
static bool BranchOperandIsImmediate(TargetInstruction* op) {
  return op != NULL &&
         (TargetIsConst(op) || (int)op->opcode == (int)X86_OP(x0));
}

// The compare-and-branch form emits "cmp src, dst" (AT&T), computing dst - src,
// and the assembler requires any immediate to be the source.  When the
// destination operand is an immediate we must compare in the other direction
// (src - dst) and invert the condition.  Returns the inverted mnemonic.
static const char* SwappedBranchMnemonic(X86Opcode opcode) {
  switch (opcode) {
    case X86_OP(je):
      return "je";
    case X86_OP(jne):
      return "jne";
    case X86_OP(jl):
      return "jg";
    case X86_OP(jge):
      return "jle";
    case X86_OP(jb):
      return "ja";
    case X86_OP(jae):
      return "jbe";
    default:
      return NULL;
  }
}

static const char* AttMnemonic(X86Opcode opcode) {
  switch (opcode) {
    case X86_OP(imul):
      return "imulq";
    case X86_OP(imull):
      return "imull";
    case X86_OP(idiv):
      return "idivq";
    case X86_OP(div):
      return "divq";
    case X86_OP(mod):
      return "idivq";
    case X86_OP(not):
      return "notq";
    case X86_OP(neg):
      return "negq";
    case X86_OP(cmp):
      return "cmpq";
    case X86_OP(test):
      return "testq";
    case X86_OP(movslq):
      return "cltq";
    case X86_OP(fmv_s):
      return "movss";
    case X86_OP(fmv_d):
      return "movsd";
    case X86_OP(movq_xmm):
      return "movq";
    default:
      return X86OpcodeName((int)opcode);
  }
}

static void PrintAttOperand(FILE* fp, TargetInstruction* op, char* buf,
                            size_t len) {
  if (op == NULL) {
    return;
  }
  if (TargetIsConst(op)) {
    PrintAsmImmediate(fp, TargetIntValue(op));
  } else if (((int)op->opcode == (int)X86_OP(symbol))) {
    char namebuf[256];
    const char* symname =
        TargetSymbolName(((TargetSymbol*)op)->symbol, namebuf, sizeof(namebuf));
    if ((op->flags & X86_HI_RELOC) != 0) {
      fprintf(fp, "%%hi(%s)", symname);
    } else {
      fprintf(fp, "%s", symname);
    }
  } else if (((int)op->opcode == (int)X86_OP(literal))) {
    TargetLiteral* literal = (TargetLiteral*)op;
    char litname[64];
    LiteralAsmName(literal->literal_id, litname, sizeof(litname));
    fprintf(fp, "%s", litname);
  } else if (((int)op->opcode == (int)X86_OP(x0))) {
    fprintf(fp, "$0");
  } else {
    PrintPercentRegFromInst(fp, op, buf, len);
  }
}

// The logical register file has more slots than there are physical x86-64
// registers, so several distinct logical slots alias the same physical
// register (e.g. logical slot 14 and slot 28 both denote r8 -- see the
// kIntRegNames table in x86_reg_alloc.c).  Two-address handling must reason
// about the *physical* register a value occupies, not the logical slot: if a
// source and the destination land on the same physical register through
// different logical slots, a naive "mov src0 -> dest" still clobbers the other
// source.  Compare by the rendered physical register name, which is unique per
// physical register and distinct across int/xmm banks.
static bool SamePhysicalReg(TargetRegister* a, TargetRegister* b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  if (a == b) {
    return true;
  }
  char buf_a[32];
  char buf_b[32];
  X86RegisterName((X86Register*)a, buf_a, sizeof(buf_a));
  X86RegisterName((X86Register*)b, buf_b, sizeof(buf_b));
  return strcmp(buf_a, buf_b) == 0;
}

static void PrintMovToDestIfNeeded(FILE* fp, TargetInstruction* inst,
                                   char* buf1, char* buf2) {
  TargetRegister* dest_reg = inst->reg;
  if (dest_reg == NULL && inst->dest != NULL) {
    dest_reg = inst->dest->reg;
  }
  if (inst->operand[0] != NULL &&
      (TargetIsConst(inst->operand[0]) ||
       (X86Opcode)inst->operand[0]->opcode == X86_OP(x0)) &&
      dest_reg != NULL) {
    fprintf(fp, "\tmovq ");
    if (TargetIsConst(inst->operand[0])) {
      PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
    } else {
      fprintf(fp, "$0");
    }
    fprintf(fp, ", ");
    PrintPercentReg(
        fp, X86RegisterName((X86Register*)dest_reg, buf1, sizeof(buf1)));
    fprintf(fp, "\n");
    return;
  }
  if (inst->operand[0] != NULL &&
      inst->operand[0]->reg != NULL &&
      dest_reg != NULL &&
      !SamePhysicalReg(inst->operand[0]->reg, dest_reg)) {
    const char* mov = "movq";
    if (X86IsFloatingPoint(inst)) {
      bool is_double =
          (X86Opcode)inst->opcode == X86_OP(fmv_d) ||
          (X86Opcode)inst->opcode == X86_OP(addsd) ||
          (X86Opcode)inst->opcode == X86_OP(subsd) ||
          (X86Opcode)inst->opcode == X86_OP(mulsd) ||
          (X86Opcode)inst->opcode == X86_OP(divsd) ||
          (X86Opcode)inst->opcode == X86_OP(ucomisd) ||
          (X86Opcode)inst->opcode == X86_OP(cvtsi2sd) ||
          (X86Opcode)inst->opcode == X86_OP(cvttsd2si) ||
          (X86Opcode)inst->opcode == X86_OP(cvtss2sd) ||
          (X86Opcode)inst->opcode == X86_OP(cvtsd2ss) ||
          (X86Opcode)inst->opcode == X86_OP(fneg_sd);
      if ((inst->flags & X86_VECTOR_VALUE) != 0) {
        mov = "movdqu";
      } else if (((X86Register*)inst->operand[0]->reg)->type == kX86RegTypeInt &&
          inst->reg != NULL &&
          ((X86Register*)inst->reg)->type == kX86RegTypeFloat) {
        mov = "movq_xmm";
      } else {
        mov = is_double ? "movsd" : "movss";
      }
    }
    fprintf(fp, "\t%s ", mov);
    PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentReg(
        fp, X86RegisterName((X86Register*)dest_reg, buf2, sizeof(buf2)));
    fprintf(fp, "\n");
  }
}

static const char* SseScratch(void) {
  return X86CurrentRegisterNameProfile()->sse_scratch;
}

static const char* SseScratch2(void) {
  return X86CurrentRegisterNameProfile()->sse_scratch2;
}

static void PrepareSseSourceOperandTo(FILE* fp, TargetInstruction* op,
                                      const char* xmm, char* buf, size_t len) {
  if (op != NULL && op->reg != NULL &&
      ((X86Register*)op->reg)->type == kX86RegTypeInt) {
    fprintf(fp, "\tmovq_xmm ");
    PrintPercentRegFromInst(fp, op, buf, len);
    fprintf(fp, ", ");
    PrintPercentReg(fp, xmm);
    fprintf(fp, "\n");
  }
}

static void PrintSseSourceOperandFrom(FILE* fp, TargetInstruction* op,
                                      const char* xmm, char* buf, size_t len) {
  if (op == NULL) {
    return;
  }
  if (op->reg != NULL &&
      ((X86Register*)op->reg)->type == kX86RegTypeInt) {
    PrintPercentReg(fp, xmm);
    return;
  }
  PrintAttOperand(fp, op, buf, len);
}

static void PrepareSseSourceOperand(FILE* fp, TargetInstruction* op, char* buf,
                                    size_t len) {
  PrepareSseSourceOperandTo(fp, op, SseScratch(), buf, len);
}

static void PrintSseSourceOperand(FILE* fp, TargetInstruction* op, char* buf,
                                  size_t len) {
  PrintSseSourceOperandFrom(fp, op, SseScratch(), buf, len);
}

static void PrintBinaryRegOp(FILE* fp, const char* mnemonic,
                             TargetInstruction* inst, char* buf1, char* buf2) {
  X86Opcode opcode = (X86Opcode)inst->opcode;
  if (X86IsFloatingPoint(inst) &&
      InstResultRegType(inst) == kX86RegTypeInt) {
    const char* mov =
        opcode == X86_OP(addsd) || opcode == X86_OP(subsd) ||
                opcode == X86_OP(mulsd) || opcode == X86_OP(divsd)
            ? "movsd"
            : "movss";
    if (inst->operand[0] != NULL && inst->operand[0]->reg != NULL &&
        ((X86Register*)inst->operand[0]->reg)->type == kX86RegTypeInt) {
      fprintf(fp, "\tmovq_xmm ");
      PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", %%xmm14\n");
    } else {
      fprintf(fp, "\t%s ", mov);
      PrintSseSourceOperand(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", %%xmm14\n");
    }
    if (inst->operand[1] != NULL) {
      PrepareSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
    }
    fprintf(fp, "\t%s ", mnemonic);
    PrintSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
    fprintf(fp, ", %%xmm14\n\tmovq %%xmm14, ");
    PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
    fprintf(fp, "\n");
    return;
  }
  // These are two-address operations: PrintMovToDestIfNeeded copies operand[0]
  // into the destination register and operand[1] is then used as the source.
  // If operand[1] already lives in the destination register (e.g. a call result
  // coalesced into %rax that is the dest of "i * factorial(...)"), that initial
  // "mov operand[0] -> dest" would clobber operand[1].  For commutative
  // operations swap the two sources so the operand already sitting in dest is
  // preserved.  (sub/div are not commutative and are not handled here.)
  bool commutative = opcode == X86_OP(imul) || opcode == X86_OP(imull) ||
                     opcode == X86_OP(addss) || opcode == X86_OP(addsd) ||
                     opcode == X86_OP(mulss) || opcode == X86_OP(mulsd) ||
                     opcode == X86_OP(addps) || opcode == X86_OP(addpd) ||
                     opcode == X86_OP(mulps) || opcode == X86_OP(mulpd) ||
                     opcode == X86_OP(paddb) || opcode == X86_OP(paddw) ||
                     opcode == X86_OP(paddd) || opcode == X86_OP(paddq) ||
                     opcode == X86_OP(pand) || opcode == X86_OP(por) ||
                     opcode == X86_OP(pxor) ||
                     opcode == X86_OP(pcmpeqb) ||
                     opcode == X86_OP(pcmpeqw) ||
                     opcode == X86_OP(pcmpeqd);
  bool non_commutative_sse = opcode == X86_OP(subss) ||
                             opcode == X86_OP(subsd) ||
                             opcode == X86_OP(divss) ||
                             opcode == X86_OP(divsd) ||
                             opcode == X86_OP(subps) ||
                             opcode == X86_OP(subpd) ||
                             opcode == X86_OP(divps) ||
                             opcode == X86_OP(divpd) ||
                             opcode == X86_OP(psubb) ||
                             opcode == X86_OP(psubw) ||
                             opcode == X86_OP(psubd) ||
                             opcode == X86_OP(psubq) ||
                             opcode == X86_OP(pcmpgtb) ||
                             opcode == X86_OP(pcmpgtw) ||
                             opcode == X86_OP(pcmpgtd);
  bool swapped = false;
  if (commutative && inst->operand[0] != NULL && inst->operand[1] != NULL &&
      !TargetIsConst(inst->operand[1]) &&
      SamePhysicalReg(inst->operand[1]->reg, inst->reg) &&
      !SamePhysicalReg(inst->operand[0]->reg, inst->reg)) {
    TargetInstruction* tmp = inst->operand[0];
    inst->operand[0] = inst->operand[1];
    inst->operand[1] = tmp;
    swapped = true;
  }
  bool saved_sse_rhs = false;
  if (non_commutative_sse && inst->operand[0] != NULL &&
      inst->operand[1] != NULL &&
      SamePhysicalReg(inst->operand[1]->reg, inst->reg) &&
      !SamePhysicalReg(inst->operand[0]->reg, inst->reg)) {
    const char* mov =
        (inst->flags & X86_VECTOR_VALUE) != 0
            ? "movdqu"
            : opcode == X86_OP(subsd) || opcode == X86_OP(divsd)
                  ? "movsd"
                  : "movss";
    fprintf(fp, "\t%s ", mov);
    PrintSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
    fprintf(fp, ", %%xmm15\n");
    saved_sse_rhs = true;
  }
  PrintMovToDestIfNeeded(fp, inst, buf1, buf2);
  if (X86IsFloatingPoint(inst) && inst->operand[1] != NULL &&
      !saved_sse_rhs) {
    PrepareSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, "\t%s ", mnemonic);
  if (saved_sse_rhs) {
    PrintPercentReg(fp, "xmm15");
  } else if (X86IsFloatingPoint(inst)) {
    PrintSseSourceOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  } else {
    PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, ", ");
  PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
  fprintf(fp, "\n");
  if (swapped) {
    TargetInstruction* tmp = inst->operand[0];
    inst->operand[0] = inst->operand[1];
    inst->operand[1] = tmp;
  }
}

static void PrintCompareAndSet(FILE* fp, const char* set_mnemonic,
                               TargetInstruction* inst, char* buf1,
                               char* buf2) {
  if ((inst->flags & (X86_FCMP_SS | X86_FCMP_SD)) != 0) {
    // Floating-point comparison: the flags come from ucomiSS/SD rather than an
    // integer cmp.  AT&T order is "ucomi src, dst" and the condition codes are
    // evaluated as dst <cc> src, so emit "ucomi operand[1], operand[0]".
    const char* cmp_mnemonic =
        (inst->flags & X86_FCMP_SD) != 0 ? "ucomisd" : "ucomiss";
    const char* src_scratch = SseScratch();
    const char* dst_scratch = SseScratch2();
    PrepareSseSourceOperandTo(fp, inst->operand[1], src_scratch, buf1,
                              sizeof(buf1));
    PrepareSseSourceOperandTo(fp, inst->operand[0], dst_scratch, buf2,
                              sizeof(buf2));
    fprintf(fp, "\t%s ", cmp_mnemonic);
    PrintSseSourceOperandFrom(fp, inst->operand[1], src_scratch, buf1,
                              sizeof(buf1));
    fprintf(fp, ", ");
    PrintSseSourceOperandFrom(fp, inst->operand[0], dst_scratch, buf2,
                              sizeof(buf2));
    fprintf(fp, "\n");
    fprintf(fp, "\t%s ", set_mnemonic);
    PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
    fprintf(fp, "\n");
    fprintf(fp, "\tmovzbq ");
    PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
    fprintf(fp, "\n");
    return;
  }
  if (inst->operand[1] != NULL) {
    // The set* lowering treats operand[0] as the left operand and operand[1] as
    // the right operand of the relation (e.g. setl means operand[0] < operand[1],
    // matching a RISC-style slt rd, op0, op1).  In AT&T syntax "cmp src, dst"
    // computes dst - src and the condition codes are evaluated as dst <cc> src,
    // so emit "cmp operand[1], operand[0]" (dst = operand[0], src = operand[1]).
    // If operand[0] is an immediate, materialize it into scratch first because
    // x86 cannot encode an immediate compare destination.
    TargetInstruction* lhs = inst->operand[0];
    if (TargetIsConst(lhs) || (X86Opcode)lhs->opcode == X86_OP(x0)) {
      fprintf(fp, "\tmovq ");
      if (TargetIsConst(lhs)) {
        PrintAsmImmediate(fp, TargetIntValue(lhs));
      } else {
        fprintf(fp, "$0");
      }
      fprintf(fp, ", %%r11\n");
      lhs = NULL;
    }
    fprintf(fp, "\tcmp ");
    PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    if (lhs == NULL) {
      fprintf(fp, "%%r11");
    } else {
      PrintAttOperand(fp, lhs, buf2, sizeof(buf2));
    }
    fprintf(fp, "\n");
  } else if (inst->operand[0] != NULL) {
    TargetInstruction* op = inst->operand[0];
    if (TargetIsConst(op) || (X86Opcode)op->opcode == X86_OP(x0)) {
      fprintf(fp, "\tmovq ");
      if (TargetIsConst(op)) {
        PrintAsmImmediate(fp, TargetIntValue(op));
      } else {
        fprintf(fp, "$0");
      }
      fprintf(fp, ", %%r11\n");
      fprintf(fp, "\ttest %%r11, %%r11\n");
    } else {
      fprintf(fp, "\ttest ");
      PrintAttOperand(fp, op, buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintAttOperand(fp, op, buf2, sizeof(buf2));
      fprintf(fp, "\n");
    }
  }
  fprintf(fp, "\t%s ", set_mnemonic);
  PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
  fprintf(fp, "\n");
  // setcc only writes the low byte, leaving the upper bits of the destination
  // register stale.  The backend treats comparison/logical-not results as
  // full-width booleans (e.g. tested with "test reg,reg" or inverted with
  // notq), so zero-extend the byte into the whole register to keep it a clean
  // 0/1 value.
  fprintf(fp, "\tmovzbq ");
  PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
  fprintf(fp, ", ");
  PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
  fprintf(fp, "\n");
}

static void PrintIdivFamily(FILE* fp, X86Opcode opcode,
                            TargetInstruction* inst, char* buf1, char* buf2) {
  // The divide reads its dividend from rax and rdx, both of which are written
  // below before it runs.  A divisor sitting in either is therefore gone by the
  // time the divide wants it, so move it aside first, while it is still there.
  bool divisor_moved = false;
  if (inst->operand[1] != NULL && inst->operand[1]->reg != NULL) {
    char div_reg[32];
    X86RegisterName((X86Register*)inst->operand[1]->reg, div_reg,
                       sizeof(div_reg));
    if (strcmp(div_reg, "rax") == 0 || strcmp(div_reg, "rdx") == 0) {
      fprintf(fp, "\tmovq %%%s, %%r11\n", div_reg);
      divisor_moved = true;
    }
  }
  if (inst->operand[0] != NULL) {
    fprintf(fp, "\tmovq ");
    PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", %%rax\n");
  }
  if ((X86Opcode)opcode == X86_OP(div) ||
      ((X86Opcode)opcode == X86_OP(mod) &&
       (inst->flags & X86_UNSIGNED_MOD) != 0)) {
    fprintf(fp, "\txorq %%rdx, %%rdx\n");
  } else {
    fprintf(fp, "\tcqo\n");
  }
  const char* div_mnemonic = AttMnemonic(opcode);
  if ((X86Opcode)opcode == X86_OP(mod) &&
      (inst->flags & X86_UNSIGNED_MOD) != 0) {
    div_mnemonic = "divq";
  }
  fprintf(fp, "\t%s ", div_mnemonic);
  if (divisor_moved) {
    fprintf(fp, "%%r11");
  } else {
    PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, "\n");
  if (inst->reg != NULL) {
    const char* result =
        (X86Opcode)opcode == X86_OP(mod) ? "%rdx" : "%rax";
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
  X86Opcode opcode = (X86Opcode)inst->opcode;

  switch (opcode) {
    case X86_OP(imul):
    case X86_OP(imull):
    case X86_OP(addss):
    case X86_OP(addsd):
    case X86_OP(subss):
    case X86_OP(subsd):
    case X86_OP(mulss):
    case X86_OP(mulsd):
    case X86_OP(divss):
    case X86_OP(divsd):
    case X86_OP(paddb):
    case X86_OP(paddw):
    case X86_OP(paddd):
    case X86_OP(paddq):
    case X86_OP(psubb):
    case X86_OP(psubw):
    case X86_OP(psubd):
    case X86_OP(psubq):
    case X86_OP(pand):
    case X86_OP(por):
    case X86_OP(pxor):
    case X86_OP(pcmpeqb):
    case X86_OP(pcmpeqw):
    case X86_OP(pcmpeqd):
    case X86_OP(pcmpgtb):
    case X86_OP(pcmpgtw):
    case X86_OP(pcmpgtd):
    case X86_OP(addps):
    case X86_OP(addpd):
    case X86_OP(subps):
    case X86_OP(subpd):
    case X86_OP(mulps):
    case X86_OP(mulpd):
    case X86_OP(divps):
    case X86_OP(divpd):
      PrintBinaryRegOp(fp, AttMnemonic(opcode), inst, buf1, buf2);
      return;

    case X86_OP(idiv):
    case X86_OP(div):
    case X86_OP(mod):
      PrintIdivFamily(fp, opcode, inst, buf1, buf2);
      return;

    case X86_OP(not):
    case X86_OP(neg):
    case X86_OP(sqrtss):
    case X86_OP(sqrtsd):
      PrintMovToDestIfNeeded(fp, inst, buf1, buf2);
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      return;

    case X86_OP(fneg_ss):
    case X86_OP(fneg_sd):
      PrintMovToDestIfNeeded(fp, inst, buf1, buf2);
      fprintf(fp, "\tpushq %%r10\n\tpushq %%r11\n\tmovq_xmm ");
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, ", %%r11\n\tmovq $%s, %%r10\n\txorq %%r10, %%r11\n\tmovq_xmm %%r11, ",
              opcode == X86_OP(fneg_sd) ? "8000000000000000" : "80000000");
      PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      fprintf(fp, "\n\tpopq %%r11\n\tpopq %%r10\n");
      return;

    case X86_OP(cmp):
      if (TargetIsConst(inst->operand[0]) ||
          (X86Opcode)inst->operand[0]->opcode == X86_OP(x0)) {
        fprintf(fp, "\tmovq ");
        if (TargetIsConst(inst->operand[0])) {
          PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
        } else {
          fprintf(fp, "$0");
        }
        fprintf(fp, ", %%r11\n");
      }
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintAttOperand(fp, inst->operand[1], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      if (TargetIsConst(inst->operand[0]) ||
          (X86Opcode)inst->operand[0]->opcode == X86_OP(x0)) {
        fprintf(fp, "%%r11");
      } else {
        PrintAttOperand(fp, inst->operand[0], buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      return;

    case X86_OP(test):
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintAttOperand(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;

    case X86_OP(setl):
      PrintCompareAndSet(fp, "setl", inst, buf1, buf2);
      return;
    case X86_OP(setb):
      PrintCompareAndSet(fp, "setb", inst, buf1, buf2);
      return;
    case X86_OP(setg):
      PrintCompareAndSet(fp, "setg", inst, buf1, buf2);
      return;
    case X86_OP(setge):
      PrintCompareAndSet(fp, "setge", inst, buf1, buf2);
      return;
    case X86_OP(setae):
      PrintCompareAndSet(fp, "setae", inst, buf1, buf2);
      return;
    case X86_OP(sete):
      PrintCompareAndSet(fp, "sete", inst, buf1, buf2);
      return;
    case X86_OP(setne):
      PrintCompareAndSet(fp, "setne", inst, buf1, buf2);
      return;

    case X86_OP(ucomiss):
    case X86_OP(ucomisd): {
      const char* src_scratch = SseScratch();
      const char* dst_scratch = SseScratch2();
      PrepareSseSourceOperandTo(fp, inst->operand[1], src_scratch, buf1,
                                sizeof(buf1));
      PrepareSseSourceOperandTo(fp, inst->operand[0], dst_scratch, buf2,
                                sizeof(buf2));
      fprintf(fp, "\t%s ", AttMnemonic(opcode));
      PrintSseSourceOperandFrom(fp, inst->operand[1], src_scratch, buf1,
                                sizeof(buf1));
      fprintf(fp, ", ");
      PrintSseSourceOperandFrom(fp, inst->operand[0], dst_scratch, buf2,
                                sizeof(buf2));
      fprintf(fp, "\n");
      return;
    }

    case X86_OP(cvtsi2ss):
    case X86_OP(cvtsi2sd):
      PrintSseIntConvert(fp, AttMnemonic(opcode), inst, true, buf1, buf2);
      return;
    case X86_OP(cvttss2si):
    case X86_OP(cvttsd2si):
      PrintSseIntConvert(fp, AttMnemonic(opcode), inst, false, buf1, buf2);
      return;

    case X86_OP(cvtss2sd):
    case X86_OP(cvtsd2ss):
      // These are unary scalar conversions: the (single) source is operand[0]
      // and the converted value is written to the destination register
      // (`cvtXX2YY %src, %dst`).  PrintBinaryRegOp would treat operand[1] as the
      // source, which is NULL for a unary op and produces an empty operand.
      if (InstResultRegType(inst) == kX86RegTypeInt) {
        PrepareSseSourceOperand(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\t%s ", AttMnemonic(opcode));
        PrintSseSourceOperand(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", %%xmm15\n\tmovq %%xmm15, ");
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
        return;
      }
      PrintSseIntConvert(fp, AttMnemonic(opcode), inst, false, buf1, buf2);
      return;

    case X86_OP(movq_xmm):
    case X86_OP(fmv_s):
    case X86_OP(fmv_d):
      if (((X86Opcode)opcode == X86_OP(fmv_s) ||
           (X86Opcode)opcode == X86_OP(fmv_d)) &&
          TargetIsConst(inst->operand[0]) &&
          InstResultRegType(inst) == kX86RegTypeFloat) {
        fprintf(fp, "\tmovq ");
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
        fprintf(fp, ", %%r10\n\tmovq_xmm %%r10, ");
        PrintResultRegFromInst(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
        return;
      }
      if ((X86Opcode)opcode == X86_OP(movq_xmm) &&
          inst->operand[0] != NULL &&
          ((inst->dest != NULL && inst->dest->reg != NULL &&
            ((X86Register*)inst->dest->reg)->type ==
                kX86RegTypeFloat) ||
           ((inst->dest == NULL || inst->dest->reg == NULL) &&
            inst->reg != NULL &&
            ((X86Register*)inst->reg)->type ==
                kX86RegTypeFloat))) {
        if (TargetIsConst(inst->operand[0])) {
          fprintf(fp, "\tmovq ");
          PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
          fprintf(fp, ", %%r10\n\tmovq_xmm %%r10, ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
        if ((int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
          // x0 is a compiler pseudo-register, not a physical x86 zero
          // register. Materialize its bits before moving them into XMM;
          // printing x0 as a register otherwise aliases %rax and returns
          // whatever integer value the preceding call left there.
          fprintf(fp, "\tmovq $0, %%r10\n\tmovq_xmm %%r10, ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
        if (inst->operand[0]->reg != NULL &&
            ((X86Register*)inst->operand[0]->reg)->type ==
                kX86RegTypeInt) {
          fprintf(fp, "\tmovq_xmm ");
          PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      /* Fall through for generic movq_xmm / movss / movsd emission. */
    case X86_OP(movss):
    case X86_OP(movsd):
    case X86_OP(movd): {
      X86RegisterType dest_type = kX86RegTypeFloat;
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        dest_type = ((X86Register*)inst->dest->reg)->type;
      } else if (inst->reg != NULL) {
        dest_type = ((X86Register*)inst->reg)->type;
      }
      if (dest_type == kX86RegTypeInt) {
        if (inst->operand[0] != NULL) {
          if (TargetIsConst(inst->operand[0])) {
            fprintf(fp, "\tmovq ");
            PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          if (inst->operand[0]->reg != NULL &&
              ((X86Register*)inst->operand[0]->reg)->type ==
                  kX86RegTypeInt) {
            fprintf(fp, "\tmovq ");
            PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          if ((int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
            fprintf(fp, "\tmovq ");
            PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
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
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      if (inst->reg != NULL &&
          ((X86Register*)inst->reg)->type == kX86RegTypeFloat &&
          inst->operand[0] != NULL) {
        if (TargetIsConst(inst->operand[0])) {
          fprintf(fp, "\tmovq ");
          PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
          fprintf(fp, ", %%r10\n\tmovq_xmm %%r10, ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
        if ((inst->operand[0]->reg != NULL &&
             ((X86Register*)inst->operand[0]->reg)->type ==
                 kX86RegTypeInt) ||
            (int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
          fprintf(fp, "\tmovq_xmm ");
          PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
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
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      return;
    }

    case X86_OP(movslq):
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
  TargetRegister* dest_reg = inst->reg;
  if (dest_reg == NULL && inst->dest != NULL) {
    dest_reg = inst->dest->reg;
  }
  if (dest_reg == NULL) {
    return;
  }
  if (TargetIsConst(src) || (X86Opcode)src->opcode == X86_OP(x0)) {
    char buf[8];
    fprintf(fp, "\tmovq ");
    if (TargetIsConst(src)) {
      PrintAsmImmediate(fp, TargetIntValue(src));
    } else {
      fprintf(fp, "$0");
    }
    fprintf(fp, ", ");
    PrintPercentReg(fp,
                    X86RegisterName((X86Register*)dest_reg, buf,
                                       sizeof(buf)));
    fprintf(fp, "\n");
    return;
  }
  if (src->block == NULL) {
    return;
  }
  if (src->reg == NULL) {
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
                  X86RegisterName((X86Register*)dest_reg, buf2,
                                     sizeof(buf2)));
  fprintf(fp, "\n");
}

static void PrintRmov(X86Emitter* emitter, TargetInstruction* inst, FILE* fp) {
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
                  X86RegisterName((X86Register*)inst->operand[1]->reg, buf1,
                                     sizeof(buf1)));
  fprintf(fp, ", ");
  PrintPercentReg(fp,
                  X86RegisterName((X86Register*)inst->operand[0]->reg, buf2,
                                     sizeof(buf2)));
  fprintf(fp, "\n");
}

static void PrintAtomicCompareExchange(TargetInstruction* inst,
                                       const char* func_name, FILE* fp) {
  int size_log2 =
      (inst->flags & X86_ATOMIC_SIZE_MASK) >> X86_ATOMIC_SIZE_SHIFT;
  static const char* cmpxchg[] = {
      "atomic_cmpxchgb", "atomic_cmpxchgw",
      "atomic_cmpxchgl", "atomic_cmpxchgq"};
  static const char* stores[] = {"movb", "movw", "movl", "movq"};
  char b0[16], b1[16], b2[16];

  fprintf(fp, "\t%s ", cmpxchg[size_log2]);
  PrintPercentRegFromInst(fp, inst->operand[1], b0, sizeof(b0));
  fprintf(fp, ", (");
  PrintPercentRegFromInst(fp, inst->operand[0], b1, sizeof(b1));
  fprintf(fp, ")\n");

  bool expected_is_pointer =
      inst->opcode == (TargetOpcode)X86_OP(atomic_compare_exchange_n);
  bool returns_bool =
      inst->opcode !=
      (TargetOpcode)X86_OP(atomic_compare_exchange_val);
  if (expected_is_pointer) {
    fprintf(fp, "\tje .L%s_atomic_cmpxchg_done_%d\n", func_name, inst->id);
    fprintf(fp, "\t%s %%rax, (", stores[size_log2]);
    PrintPercentRegFromInst(fp, inst->operand[2], b0, sizeof(b0));
    fprintf(fp, ")\n");
    fprintf(fp, ".L%s_atomic_cmpxchg_done_%d:\n", func_name, inst->id);
  }
  if (returns_bool) {
    // CMPXCHG returns the observed value in RAX.  Update *expected before SETE
    // overwrites AL with the boolean result; MOV preserves the comparison
    // flags, so SETE remains valid on both paths.
    fprintf(fp, "\tsete ");
    PrintResultRegFromInst(fp, inst, b2, sizeof(b2));
    fprintf(fp, "\n");
    fprintf(fp, "\tmovzbq ");
    PrintResultRegFromInst(fp, inst, b0, sizeof(b0));
    fprintf(fp, ", ");
    PrintResultRegFromInst(fp, inst, b1, sizeof(b1));
    fprintf(fp, "\n");
  } else {
    fprintf(fp, "\tmovq %%rax, ");
    PrintResultRegFromInst(fp, inst, b0, sizeof(b0));
    fprintf(fp, "\n");
  }
}

static void PrintAtomicFetchAddSub(TargetInstruction* inst, FILE* fp) {
  int size_log2 =
      (inst->flags & X86_ATOMIC_SIZE_MASK) >> X86_ATOMIC_SIZE_SHIFT;
  static const char* xadd[] = {
      "atomic_xaddb", "atomic_xaddw", "atomic_xaddl", "atomic_xaddq"};
  char b0[16], b1[16];

  fprintf(fp, "\t%s ", xadd[size_log2]);
  PrintPercentRegFromInst(fp, inst->operand[1], b0, sizeof(b0));
  fprintf(fp, ", (");
  PrintPercentRegFromInst(fp, inst->operand[0], b1, sizeof(b1));
  fprintf(fp, ")\n");
}

static void RestoreStackPointerAtLandingPad(X86Emitter* emitter, FILE* fp);

static void ReloadStructReturnRegisterAtLandingPad(X86Emitter* emitter,
                                                   FILE* fp);

// Main instruction printer.
static void PrintInstruction(X86Emitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (!X86_IS_64BIT(emitter->rv)) {
    PrintInstructionI386(emitter, inst, func_name, fp);
    return;
  }
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    TargetBasicBlock* b = inst->block;
    fprintf(fp, "\n\t// *** Basic block %zd\n\n", b->block_id);
    emitter->current_block = inst->block;
  }
  if (((int)inst->opcode == (int)X86_OP(label))) {
    if ((inst->flags & X86_INST_TABLE_ENTRY) != 0) {
      fprintf(fp, "\t.p2align 3\n");
    }
    if ((inst->flags & X86_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    if ((inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
      RestoreStackPointerAtLandingPad(emitter, fp);
      ReloadStructReturnRegisterAtLandingPad(emitter, fp);
    }
    return;
  }

  if (((int)inst->opcode == (int)X86_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }

  if (((int)inst->opcode == (int)X86_OP(ivarreg)) || ((int)inst->opcode == (int)X86_OP(fvarreg))) {
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
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(atomic_compare_exchange_bool):
    case X86_OP(atomic_compare_exchange_val):
    case X86_OP(atomic_compare_exchange_n):
      PrintAtomicCompareExchange(inst, func_name, fp);
      return;
    case X86_OP(atomic_fetch_add_sub):
      PrintAtomicFetchAddSub(inst, fp);
      return;
//    case X86_OP(rmov):
//    case X86_OP(rmovf):
//    case X86_OP(rmovd):
//      PrintRmov(emitter, inst, fp);
//      return;

    case X86_OP(mv):
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
      // Single-operand value copy: inst->reg <- operand[0].
      // (The generic default emits operands reversed for AT&T syntax, so
      // handle this form explicitly here.)
      if (inst->reg != NULL) {
        fprintf(fp, "\t%s ", MoveMnemonicForInst(inst->operand[0], inst));
        PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
      }
      return;
    case X86_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      char namebuf[256];
      const char* symname =
          TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n", symname);
      } else if (SymbolHasWeakBinding(sym->symbol)) {
        fprintf(fp, "\t.weak %s\n", symname);
      } else {
        fprintf(fp, "\t.global %s\n", symname);
      }
      return;
    }
    case X86_OP(call):
    case X86_OP(callf): {
      assert(((int)inst->operand[0]->opcode == (int)X86_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      char namebuf[256];
      const char* symname =
          TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
      // The integrated assembler selects PLT32 for calls in PIC mode.
      // Keeping "@plt" in the textual name creates a distinct undefined
      // symbol instead of a relocation suffix.
      fprintf(fp, "\t%-12s%s\n", "call", symname);
      return;
    }

    case X86_OP(rcall):
    case X86_OP(rcallf):
      fprintf(fp, "\tcall *");
      PrintPercentRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;
    case X86_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case X86_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case X86_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t");
      if ((inst->flags & X86_INST_EXTENDED_ASM) != 0) {
        PrintExtendedAsm(fp, (X86AsmInstruction*)inst, lit->value.value);
      } else {
        fprintf(fp, "%s", lit->value.value);
      }
      fprintf(fp, "\n");
      lit->base.disabled = true;
      return;
    }

    case X86_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }

    case X86_OP(spill): {
      X86Register* reg = (X86Register*)inst->reg;
      int offset = (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      const char* mov = (inst->flags & X86_VECTOR_VALUE) != 0
                            ? "movdqu"
                            : reg->type == kX86RegTypeInt ? "movq" : "storesd";
      if (FitsMemoryDisplacement(-offset)) {
        fprintf(fp, "\t%s ", mov);
        PrintPercentReg(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, ", -%d(%%rbp)\t// Spilled @%d\n", offset,
                inst->operand[0]->id);
      } else {
        fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr),
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, "\n\tmovq %%rbp, ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\tsubq ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr),
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, ", ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\t%s ", mov);
        PrintPercentReg(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, ", (");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, ")\t// Spilled @%d\n", inst->operand[0]->id);
      }
      return;
    }

    case X86_OP(reload): {
      X86Register* reg = (X86Register*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      const char* mov = (spill->flags & X86_VECTOR_VALUE) != 0
                            ? "movdqu"
                            : reg->type == kX86RegTypeInt ? "movq" : "movsd";
      if (FitsMemoryDisplacement(-offset)) {
        fprintf(fp, "\t%s -%d(%%rbp), ", mov, offset);
        PrintPercentReg(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, "\t// Reloaded spilled @%d\n", spill->operand[0]->id);
      } else {
        fprintf(fp, "\tmovabs ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr),
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, "\n\tmovq %%rbp, ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\tsubq ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->spill_addr),
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, ", ");
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\t%s (", mov);
        PrintPercentReg(fp, X86RegisterNameFromNum((X86_P(emitter->rv)->int_temp_start_1),
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "), ");
        PrintPercentReg(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, "\t// Reloaded spilled @%d\n", spill->operand[0]->id);
      }
      return;
    }
    default:
      break;
  }

  // Print operands for the remaining instruction forms.
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(loadl):
    case X86_OP(loadl_z):
    case X86_OP(loadw):
    case X86_OP(loadb):
    case X86_OP(loadb_z):
    case X86_OP(loadw_z):
    case X86_OP(loadss):
    case X86_OP(loadq):
    case X86_OP(loadsd):
    case X86_OP(loadv): {
      const char* mov = "movq";
      switch ((X86Opcode)inst->opcode) {
        case X86_OP(loadl):
          // Signed 32-bit load must sign-extend into the full 64-bit register
          // (the backend operates on 64-bit registers); a plain movl would
          // zero-extend.
          mov = "movslq";
          break;
        case X86_OP(loadl_z):
          mov = "movl";
          break;
        case X86_OP(loadss):
          mov = "movss";
          break;
        case X86_OP(loadsd):
          mov = "movsd";
          break;
        case X86_OP(loadv):
          mov = "movdqu";
          break;
        case X86_OP(loadb):
          // Signed byte load must sign-extend into the full register; a plain
          // movb leaves the upper bits of the (64-bit) destination unchanged.
          mov = "movsbq";
          break;
        case X86_OP(loadb_z):
          mov = "movzbq";
          break;
        case X86_OP(loadw):
          mov = "movswq";
          break;
        case X86_OP(loadw_z):
          mov = "movzwq";
          break;
        default:
          break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      if (((X86Opcode)inst->opcode == X86_OP(loadss) ||
           (X86Opcode)inst->opcode == X86_OP(loadsd)) &&
          InstResultRegType(inst) == kX86RegTypeInt) {
        mov = ((X86Opcode)inst->opcode == X86_OP(loadsd)) ? "movq" : "movl";
      }
      fprintf(fp, "\t%s ", mov);
      int64_t offset = 0;
      TargetInstruction* base = inst->operand[0];
      if (((int)inst->operand[0]->opcode == (int)X86_OP(symbol)) &&
          MemoryOffsetFromOperand(inst->operand[1], &offset) && offset == 0) {
        char namebuf[256];
        fprintf(fp, "%s(%%rip), ",
                TargetSymbolName(((TargetSymbol*)inst->operand[0])->symbol,
                                 namebuf, sizeof(namebuf)));
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (MemoryBaseFromOffsetOperand(inst->operand[1], &base)) {
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
      } else if (((int)inst->operand[1]->opcode == (int)X86_OP(symbol))) {
        char namebuf[256];
        fprintf(fp, "%s(", TargetSymbolName(
                               ((TargetSymbol*)inst->operand[1])->symbol,
                               namebuf, sizeof(namebuf)));
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_OP(label))) {
        assert((inst->flags & X86_PCREL_LO_RELOC) != 0);
        fprintf(fp, ".%s_label_%d(", func_name, inst->operand[1]->id);
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_OP(x0))) {
        fprintf(fp, "0(");
        PrintMemoryBaseRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (PrintTlsTporffMemoryOperand(fp, inst->operand[0],
                                             inst->operand[1], buf2,
                                             sizeof(buf2))) {
        fprintf(fp, ", ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else {
        assert(false);
      }
      break;
    }

    case X86_OP(storel):
    case X86_OP(storew):
    case X86_OP(storeq):
    case X86_OP(storeb):
    case X86_OP(storess):
    case X86_OP(storesd):
    case X86_OP(storev): {
      const char* mov = "movq";
      switch ((X86Opcode)inst->opcode) {
        case X86_OP(storel):
          mov = "movl";
          break;
        case X86_OP(storess):
          mov = "storess";
          break;
        case X86_OP(storesd):
          mov = "storesd";
          break;
        case X86_OP(storev):
          mov = "movdqu";
          break;
        case X86_OP(storeb):
          mov = "movb";
          break;
        case X86_OP(storew):
          mov = "movw";
          break;
        default:
          break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      if (((X86Opcode)inst->opcode == X86_OP(storesd) ||
           (X86Opcode)inst->opcode == X86_OP(storess)) &&
          OperandHasIntReg(inst->operand[0])) {
        mov = ((X86Opcode)inst->opcode == X86_OP(storesd)) ? "movq" : "movl";
      }
      fprintf(fp, "\t%s ", mov);
      if (TargetIsConst(inst->operand[0])) {
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
      } else if ((int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
        // The x0 pseudo-register is the constant 0; x86-64 has no hardware zero
        // register, so store an immediate 0 rather than emitting whatever
        // physical register x0 was mapped to (which is not guaranteed to hold
        // 0).
        fprintf(fp, "$0");
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
      } else if (((int)inst->operand[2]->opcode == (int)X86_OP(symbol))) {
        char namebuf[256];
        fprintf(fp, "%s(", TargetSymbolName(
                               ((TargetSymbol*)inst->operand[2])->symbol,
                               namebuf, sizeof(namebuf)));
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_OP(label))) {
        assert((inst->flags & X86_PCREL_LO_RELOC) != 0);
        fprintf(fp, ".%s_label_%d(", func_name, inst->operand[2]->id);
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_OP(x0))) {
        fprintf(fp, "0(");
        PrintMemoryBaseRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (PrintTlsTporffMemoryOperand(fp, inst->operand[1],
                                             inst->operand[2], buf2,
                                             sizeof(buf2))) {
        fprintf(fp, "\n");
      } else {
        assert(false);
      }
      break;
    }

    case X86_OP(nop):
      if (!inst->observable_checkpoint) {
        fprintf(fp, (inst->flags & X86_MFENCE) != 0 ? "\tmfence\n"
                                                       : "\tnop\n");
      }
      break;

    case X86_OP(je):
    case X86_OP(jne):
    case X86_OP(jl):
    case X86_OP(jge):
    case X86_OP(jb):
    case X86_OP(jae):
      if (inst->operand[1] == NULL) {
        assert(inst->operand[0] != NULL);
        assert(((int)inst->operand[0]->opcode == (int)X86_OP(label)));
        fprintf(fp, "\t%s .%s_label_%d\n",
                BranchMnemonic((X86Opcode)inst->opcode),
                func_name, inst->operand[0]->id);
        break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[2] != NULL);
      if (BranchOperandIsImmediate(inst->operand[0]) &&
          !BranchOperandIsImmediate(inst->operand[1])) {
        // The destination of the cmp would be an immediate, which the
        // assembler cannot encode.  Compare in the opposite direction and
        // invert the branch condition.
        const char* mnemonic = SwappedBranchMnemonic((X86Opcode)inst->opcode);
        assert(mnemonic != NULL);
        fprintf(fp, "\tcmp ");
        PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        PrintAttOperand(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, "\n\t%s .%s_label_%d\n", mnemonic, func_name,
                inst->operand[2]->id);
        break;
      }
      fprintf(fp, "\tcmp ");
      PrintAttOperand(fp, inst->operand[1], buf2, sizeof(buf2));
      fprintf(fp, ", ");
      PrintAttOperand(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, "\n\t%s .%s_label_%d\n",
              BranchMnemonic((X86Opcode)inst->opcode),
              func_name, inst->operand[2]->id);
      break;

    case X86_OP(jz):
    case X86_OP(jnz):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[0]->reg != NULL);
      fprintf(fp, "\ttest ");
      PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintPercentRegFromInst(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n\t%s .%s_label_%d\n",
              BranchMnemonic((X86Opcode)inst->opcode),
              func_name, inst->operand[1]->id);
      break;

    case X86_OP(jmp): {
      assert(inst->operand[0] != NULL);
      if ((inst->flags & X86_INST_TABLE_ENTRY) != 0) {
        fprintf(fp, "\t.p2align 3\n");
      }
      TargetInstruction* dest = inst->operand[0];
      if (((int)dest->opcode == (int)X86_OP(label))) {
        fprintf(fp, "\tjmp .%s_label_%d\n", func_name, dest->id);
      } else if (((int)dest->opcode == (int)X86_OP(symbol))) {
        char namebuf[256];
        fprintf(fp, "\tjmp %s\n",
                TargetSymbolName(((TargetSymbol*)dest)->symbol, namebuf,
                                 sizeof(namebuf)));
      } else {
        fprintf(fp, "\tjmp *");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\n");
      }
      break;
    }

    case X86_OP(mov): {
      int64_t value = TargetIntValue(inst->operand[0]);
      fprintf(fp, "\tmovq ");
      PrintAsmImmediate(fp, value);
      fprintf(fp, ", ");
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(ret):
      fprintf(fp, "\tret\n");
      break;

    case X86_OP(add):
    case X86_OP(addl):
    case X86_OP(sub):
    case X86_OP(subl):
    case X86_OP(and):
    case X86_OP(or):
    case X86_OP(xor): {
      if (inst->operand[0] == NULL || inst->operand[1] == NULL) {
        break;
      }
      bool is_sub = (X86Opcode)inst->opcode == X86_OP(sub) ||
                    (X86Opcode)inst->opcode == X86_OP(subl);
      const char* op = "addq";
      if ((X86Opcode)inst->opcode == X86_OP(addl)) {
        op = "addl";
      } else if ((X86Opcode)inst->opcode == X86_OP(sub)) {
        op = "subq";
      } else if ((X86Opcode)inst->opcode == X86_OP(subl)) {
        op = "subl";
      } else if ((X86Opcode)inst->opcode == X86_OP(and)) {
        op = "andq";
      } else if ((X86Opcode)inst->opcode == X86_OP(or)) {
        op = "orq";
      } else if ((X86Opcode)inst->opcode == X86_OP(xor)) {
        op = "xorq";
      }
      // These are two-address operations: the result is computed in place into
      // the destination register, which first receives a copy of operand[0].
      // The destination register is operand[0] unless an explicit dest is set.
      TargetRegister* dest_reg =
          (inst->dest != NULL && inst->dest->reg != NULL) ? inst->dest->reg
                                                          : inst->reg;
      TargetInstruction* src0 = inst->operand[0];
      TargetInstruction* src1 = inst->operand[1];
      bool src0_is_zero =
          (X86Opcode)src0->opcode == X86_OP(x0) ||
          (TargetIsConst(src0) && TargetIntValue(src0) == 0);
      bool src1_is_zero = (X86Opcode)src1->opcode == X86_OP(x0);
      bool src1_in_dest = !TargetIsConst(src1) && !src1_is_zero &&
                          SamePhysicalReg(src1->reg, dest_reg) &&
                          !SamePhysicalReg(src0->reg, dest_reg);
      if (src1_in_dest && !is_sub) {
        // Commutative op with the second source already in the destination
        // register: swap the sources so the "mov src0 -> dest" below does not
        // clobber src1 (which lives in dest).  e.g. add base, idx where idx is
        // already in dest becomes add idx, base-in-dest.
        TargetInstruction* tmp = src0;
        src0 = src1;
        src1 = tmp;
      } else if (src1_in_dest && is_sub) {
        // Non-commutative subtract with the subtrahend in the destination
        // register.  Compute dest = src0 - src1 as "neg dest ; add src0, dest"
        // to avoid clobbering src1.
        fprintf(fp, "\tnegq ");
        PrintPercentReg(fp, X86RegisterName((X86Register*)dest_reg, buf2,
                                               sizeof(buf2)));
        if (!src0_is_zero) {
          fprintf(fp, "\n\taddq ");
          if (TargetIsConst(src0)) {
            PrintAsmImmediate(fp, TargetIntValue(src0));
          } else {
            PrintPercentRegFromInst(fp, src0, buf1, sizeof(buf1));
          }
          fprintf(fp, ", ");
          PrintPercentReg(fp,
                          X86RegisterName((X86Register*)dest_reg, buf2,
                                             sizeof(buf2)));
        }
        if ((X86Opcode)inst->opcode == X86_OP(subl)) {
          // The assembler currently has only a 64-bit unary negate. Preserve
          // the zero-extension semantics that a 32-bit subtraction provides.
          fprintf(fp, "\n\tmovl ");
          PrintPercentReg(fp,
                          X86RegisterName((X86Register*)dest_reg, buf2,
                                             sizeof(buf2)));
          fprintf(fp, ", ");
          PrintPercentReg(fp,
                          X86RegisterName((X86Register*)dest_reg, buf2,
                                             sizeof(buf2)));
        }
        fprintf(fp, "\n");
        break;
      }
      // The swap above may have moved the zero pseudo-register into src1, so
      // ask again for both operands rather than reusing the answers from before
      // the swap. x0 has no physical register behind it; printing it as one
      // names %rax.
      src0_is_zero =
          (X86Opcode)src0->opcode == X86_OP(x0) ||
          (TargetIsConst(src0) && TargetIntValue(src0) == 0);
      src1_is_zero = (X86Opcode)src1->opcode == X86_OP(x0);
      if ((TargetIsConst(src0) || src0_is_zero) && dest_reg != NULL) {
        fprintf(fp, "\tmovq ");
        if (TargetIsConst(src0)) {
          PrintAsmImmediate(fp, TargetIntValue(src0));
        } else {
          fprintf(fp, "$0");
        }
        fprintf(fp, ", ");
        PrintPercentReg(fp, X86RegisterName((X86Register*)dest_reg, buf2,
                                               sizeof(buf2)));
        fprintf(fp, "\n");
      } else if (src0->reg != NULL && dest_reg != NULL &&
                 !SamePhysicalReg(src0->reg, dest_reg)) {
        fprintf(fp, "\tmovq ");
        PrintPercentRegFromInst(fp, src0, buf1, sizeof(buf1));
        fprintf(fp, ", ");
        PrintPercentReg(fp, X86RegisterName((X86Register*)dest_reg, buf2,
                                               sizeof(buf2)));
        fprintf(fp, "\n");
      }
      if (TargetIsConst(src1) || src1_is_zero) {
        fprintf(fp, "\t%s ", op);
        if (TargetIsConst(src1)) {
          PrintAsmImmediate(fp, TargetIntValue(src1));
        } else {
          fprintf(fp, "$0");
        }
        fprintf(fp, ", ");
        PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else {
        fprintf(fp, "\t%s ", op);
        PrintPercentRegFromInst(fp, src1, buf1, sizeof(buf1));
        fprintf(fp, ", ");
        if (dest_reg != NULL) {
          PrintPercentReg(fp, X86RegisterName((X86Register*)dest_reg,
                                                 buf2, sizeof(buf2)));
        } else {
          PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
        }
        fprintf(fp, "\n");
      }
      break;
    }

    case X86_OP(bsf):
    case X86_OP(bsr):
    case X86_OP(bsfl):
    case X86_OP(bsrl): {
      bool is32 = (X86Opcode)inst->opcode == X86_OP(bsfl) ||
                  (X86Opcode)inst->opcode == X86_OP(bsrl);
      bool leading = (X86Opcode)inst->opcode == X86_OP(bsr) ||
                     (X86Opcode)inst->opcode == X86_OP(bsrl);
      fprintf(fp, "\tmovq ");
      PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", %%r11\n\t%s %s, %s\n",
              leading ? (is32 ? "bsrl" : "bsrq")
                      : (is32 ? "bsfl" : "bsfq"),
              is32 ? "%r11d" : "%r11", is32 ? "%r11d" : "%r11");
      if (leading) {
        fprintf(fp, "\t%s ", is32 ? "xorl" : "xorq");
        PrintAsmImmediate(fp, is32 ? 31 : 63);
        fprintf(fp, ", %s\n", is32 ? "%r11d" : "%r11");
      }
      fprintf(fp, "\tmovq %%r11, ");
      PrintResultRegFromInst(fp, inst, buf2, sizeof(buf2));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(shl):
    case X86_OP(shr):
    case X86_OP(sar):
    case X86_OP(rol):
    case X86_OP(ror):
    case X86_OP(roll):
    case X86_OP(rorl):
    case X86_OP(shll):
    case X86_OP(shrl):
    case X86_OP(sarl): {
      const char* op = "shlq";
      switch ((X86Opcode)inst->opcode) {
        case X86_OP(shr):
          op = "shrq";
          break;
        case X86_OP(sar):
          op = "sarq";
          break;
        case X86_OP(rol):
          op = "rolq";
          break;
        case X86_OP(ror):
          op = "rorq";
          break;
        case X86_OP(roll):
          op = "roll";
          break;
        case X86_OP(rorl):
          op = "rorl";
          break;
        case X86_OP(shll):
          op = "shll";
          break;
        case X86_OP(shrl):
          op = "shrl";
          break;
        case X86_OP(sarl):
          op = "sarl";
          break;
        default:
          break;
      }
      bool constant_count = TargetIsConst(inst->operand[1]);
      if (!constant_count) {
        // Variable shifts require %cl. Use reserved scratch register %r11 for
        // the result and save %rcx around the operation. This handles every
        // overlap between lhs, count, destination, and an unrelated live value
        // in %rcx without requiring allocator constraints.
        fprintf(fp, "\tmovq ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", %%r11\n\tpushq %%rcx\n\tmovq ");
        PrintPercentRegFromInst(fp, inst->operand[1], buf1, sizeof(buf1));
        fprintf(fp, ", %%rcx\n");
        bool shift32 = (X86Opcode)inst->opcode == X86_OP(shll) ||
                       (X86Opcode)inst->opcode == X86_OP(shrl) ||
                       (X86Opcode)inst->opcode == X86_OP(sarl) ||
                       (X86Opcode)inst->opcode == X86_OP(roll) ||
                       (X86Opcode)inst->opcode == X86_OP(rorl);
        fprintf(fp, "\t%s %%cl, %s\n\tpopq %%rcx\n\tmovq %%r11, ", op,
                shift32 ? "%r11d" : "%r11");
        if (inst->dest != NULL && inst->dest->reg != NULL) {
          PrintPercentReg(
              fp, X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                     sizeof(buf2)));
        } else {
          PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
        }
        fprintf(fp, "\n");
        break;
      }
      if ((X86Opcode)inst->opcode == X86_OP(roll) ||
          (X86Opcode)inst->opcode == X86_OP(rorl)) {
        fprintf(fp, "\tmovq ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", %%r11\n\t%s ", op);
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[1]));
        fprintf(fp, ", %%r11d\n\tmovq %%r11, ");
        PrintResultRegFromInst(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
        break;
      }
      if (inst->operand[0]->reg != NULL && inst->reg != NULL &&
          inst->operand[0]->reg != inst->reg) {
        fprintf(fp, "\tmovq ");
        PrintPercentRegFromInst(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        if (inst->dest != NULL && inst->dest->reg != NULL) {
          PrintPercentReg(
              fp, X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                     sizeof(buf2)));
        } else {
          PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
        }
        fprintf(fp, "\n");
      }
      fprintf(fp, "\t%s ", op);
      PrintAsmImmediate(fp, TargetIntValue(inst->operand[1]));
      fprintf(fp, ", ");
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentReg(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInst(fp, inst, buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(lea):
    case X86_OP(lea_rip): {
      // lea_rip of a symbol or string literal needs only operand[0]; the RIP
      // base is implicit.  The register-form lea still requires operand[1].
      if (inst->operand[0] == NULL) {
        break;
      }
      if ((X86Opcode)inst->opcode == X86_OP(lea) &&
          inst->operand[1] == NULL) {
        break;
      }
      fprintf(fp, ((inst->flags & X86_GOTPCREL_RELOC) != 0 &&
                   (int)inst->operand[0]->opcode != (int)X86_OP(literal))
                      ? "\tmov "
                      : "\tlea ");
      if (((int)inst->operand[0]->opcode == (int)X86_OP(symbol))) {
        TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
        // Use TargetSymbolName so function-local statics get the same mangled
        // name (.local.<name>.<id>) used by their data definition; the raw
        // symbol name would collide with a same-named file-scope global.
        char namebuf[256];
        const char* symname =
            TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
        if ((inst->flags & X86_TLSGD_RELOC) != 0) {
          fprintf(fp, "%s@TLSGD(%%rip), ", symname);
        } else if ((inst->flags & X86_GOTPCREL_RELOC) != 0) {
          // A GOTPCREL operand names a slot containing the symbol address, so
          // load that slot rather than materializing the slot's own address.
          fprintf(fp, "%s@GOTPCREL(%%rip), ", symname);
        } else if (((int)inst->opcode == (int)X86_OP(lea_rip))) {
          fprintf(fp, "%s(%%rip), ", symname);
        } else {
          fprintf(fp, "%s(", symname);
          PrintPercentRegFromInst(fp, inst->operand[1], buf2, sizeof(buf2));
          fprintf(fp, "), ");
        }
      } else if (((int)inst->operand[0]->opcode == (int)X86_OP(literal))) {
        TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
        char litname[64];
        LiteralAsmName(literal->literal_id, litname, sizeof(litname));
        fprintf(fp, "%s(%%rip), ", litname);
      } else if (((int)inst->opcode == (int)X86_OP(lea_rip)) &&
                 ((int)inst->operand[0]->opcode == (int)X86_OP(label))) {
        fprintf(fp, ".%s_label_%d(%%rip), ", func_name, inst->operand[0]->id);
      } else if (((int)inst->opcode == (int)X86_OP(lea_rip)) &&
                 TargetIsConst(inst->operand[0])) {
        // PC-relative lea with an immediate displacement (used to materialize
        // the current instruction pointer for computed branches).
        fprintf(fp, "%" PRId64 "(%%rip), ", TargetIntValue(inst->operand[0]));
      } else if (inst->operand[1] != NULL && TargetIsConst(inst->operand[1])) {
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

    case X86_OP(movabs): {
      fprintf(fp, "\tmovabs ");
      PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
      fprintf(fp, ", ");
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(movxc): {
      fprintf(fp, "\tmovabs ");
      if (inst->operand[0] != NULL &&
          (int)inst->operand[0]->opcode == (int)X86_OP(symbol)) {
        TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
        char namebuf[256];
        const char* symname =
            TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
        if ((inst->flags & X86_TLS_RELOC) != 0) {
          fprintf(fp, "%s@TPOFF, ", symname);
        } else {
          fprintf(fp, "%s, ", symname);
        }
      } else {
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
        fprintf(fp, ", ");
      }
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(tp): {
      fprintf(fp, "\tmovq %%fs:0, ");
      PrintPercentRegFromInst(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    default:
      PrintDefaultInstruction(fp, inst, buf1, buf2);
      break;
  }

}

void X86EmitterInit(X86Emitter* emitter, X86Generator* rv) {
  emitter->rv = rv;
  emitter->regs = &rv->register_allocator;
  emitter->saved_reg_offset = 0;
  emitter->spill_region_size = rv->register_allocator.max_spilled_region_size;
  emitter->first_spill_offset = 0;
  emitter->current_block = NULL;
}

X86Emitter* NewX86Emitter(X86Generator* rv) {
  X86Emitter* emitter = malloc(sizeof(X86Emitter));
  X86EmitterInit(emitter, rv);
  return emitter;
}

void X86EmitterDestruct(X86Emitter* emitter) {
}

void X86EmitterDelete(X86Emitter* emitter) {
  X86EmitterDestruct(emitter);
  free(emitter);
}

static const char* LSDATypeInfoSymbol(EHTypeInfo* info) {
  if (info == NULL) {
    return NULL;
  }
  if (info->canonical_typeinfo != NULL &&
      info->canonical_typeinfo->asm_name.length > 0) {
    return info->canonical_typeinfo->asm_name.value;
  }
  return NULL;
}

static int X86DwarfRegister(int reg) {
  static const int dwarf_regs[X86_MAX_INT_REGS] = {
      0,  0,  7,  11, 10, 10, 11, 12, 6, 3,  5,  4,  1,  2,  8,  9,
      10, 11, 12, 13, 14, 15, 12, 13, 14, 15, 10, 11, 8,  9,  10, 11,
  };
  if (reg >= 0 && reg < X86_MAX_INT_REGS) {
    return dwarf_regs[reg];
  }
  return reg;
}

static size_t X86SavedCFIRegisters(
    X86Emitter* emitter, DaveEHFrameSavedReg* saved_regs,
    size_t capacity) {
  BitSetIterator it;
  size_t count = 0;
  int offset = emitter->saved_reg_offset;
  int stack_frame_size = StackFrameSize(emitter);

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    if (count < capacity) {
      saved_regs[count].dwarf_reg = X86DwarfRegister(reg);
      saved_regs[count].cfa_offset = offset - stack_frame_size - 8;
      count++;
    }
    offset -= 8;
    BitSetIteratorNext(&it);
  }
  return count;
}

static void X86PrintEHFrame(X86Emitter* emitter, FILE* fp,
                               const char* func_name) {
  if (!X86_IS_64BIT(emitter->rv)) {
    X86PrintEHFrameI386(emitter, fp, func_name);
    return;
  }
  if (!X86_IS_64BIT(emitter->rv)) {
    X86PrintEHFrameI386(emitter, fp, func_name);
    return;
  }
  if (emitter->rv->base.varargs) {
    return;
  }

  DaveEHLSDARange lsda_ranges[64];
  DaveEHFrameSavedReg saved_regs[16];
  size_t lsda_count = 0;
  size_t saved_reg_count =
      X86SavedCFIRegisters(emitter, saved_regs,
                             sizeof(saved_regs) / sizeof(saved_regs[0]));
  for (size_t i = 0; i < emitter->rv->exception_ranges.length &&
                     lsda_count < sizeof(lsda_ranges) / sizeof(lsda_ranges[0]);
       i++) {
    X86ExceptionRange* range = emitter->rv->exception_ranges.value.p[i];
    DaveEHLSDARange* out = &lsda_ranges[lsda_count++];
    out->try_start_id = range->try_start->id;
    out->try_end_id = range->try_end->id;
    out->landing_pad_id = range->catch_label->id;
    out->is_cleanup = range->is_cleanup;
    out->catch_typeinfo =
        range->is_cleanup ? NULL : LSDATypeInfoSymbol(range->catch_typeinfo);
  }

  DaveEHFrameEmitInfo info = {
      .ranges = lsda_ranges,
      .range_count = lsda_count,
      .func_name = func_name,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = true,
      .cie_ra_reg = 16,
      .cie_cfa_reg = 7,
      .cie_fp_reg = 6,
      .entry_cfa_offset = 8,
      .frame_cfa_offset = 16,
      .fp_cfa_offset = 8,
      .saved_fp_offset = -16,
      .saved_ra_offset = -8,
      .saved_regs = saved_regs,
      .saved_reg_count = saved_reg_count,
  };
  DaveEHPrintEHFrameCIE(fp, &info, "");
  DaveEHPrintEHFrameFDE(fp, &info, "");
}

static void X86PrintGCCExceptTable(X86Emitter* emitter, FILE* fp,
                               const char* func_name) {
  if (!X86_IS_64BIT(emitter->rv)) {
    X86PrintGCCExceptTableI386(emitter, fp, func_name);
    return;
  }
  if (!X86_IS_64BIT(emitter->rv)) {
    X86PrintGCCExceptTableI386(emitter, fp, func_name);
    return;
  }
  DaveEHLSDARange lsda_ranges[64];
  size_t lsda_count = 0;
  for (size_t i = 0; i < emitter->rv->exception_ranges.length &&
                     lsda_count < sizeof(lsda_ranges) / sizeof(lsda_ranges[0]);
       i++) {
    X86ExceptionRange* range = emitter->rv->exception_ranges.value.p[i];
    DaveEHLSDARange* out = &lsda_ranges[lsda_count++];
    out->try_start_id = range->try_start->id;
    out->try_end_id = range->try_end->id;
    out->landing_pad_id = range->catch_label->id;
    out->is_cleanup = range->is_cleanup;
    out->catch_typeinfo =
        range->is_cleanup ? NULL : LSDATypeInfoSymbol(range->catch_typeinfo);
  }
  DaveEHFrameEmitInfo info = {
      .ranges = lsda_ranges,
      .range_count = lsda_count,
      .func_name = func_name,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = true,
      .cie_ra_reg = 16,
      .cie_cfa_reg = 7,
      .cie_fp_reg = 6,
      .entry_cfa_offset = 8,
      .frame_cfa_offset = 16,
      .fp_cfa_offset = 8,
      .saved_fp_offset = -16,
      .saved_ra_offset = -8,
  };
  DaveEHPrintGCCExceptTable(fp, &info);
}

// The unwinder resumes a frame at its landing pad with the stack pointer the
// throwing call site left it at.  Outgoing arguments are pushed, so that is
// below the bottom of the frame, whereas the rest of the function assumes the
// stack pointer is at the bottom: the exit sequence in particular reads the
// callee-saved registers at fixed offsets from it, and would hand the caller
// the outgoing arguments of the call that threw instead of its own registers.
// The frame pointer is restored by the unwinder from the CFI, so put the stack
// pointer back from that.  This is the inverse of the prologue, which lowers
// rsp to rbp + space_above_frame_pointer - stack_frame_size.
static void RestoreStackPointerAtLandingPad(X86Emitter* emitter, FILE* fp) {
  if (!X86_IS_64BIT(emitter->rv)) {
    RestoreStackPointerAtLandingPadI386(emitter, fp);
    return;
  }
  if (EmptyStackFrame(emitter)) {
    // No frame and so no frame pointer to recover the stack pointer from.
    // Nothing is stored relative to rsp either, so there is nothing to fix.
    return;
  }
  int space_above_frame_pointer =
      emitter->rv->base.varargs ? (X86_P(emitter->rv)->vararg_save_area_size) : 0;
  fprintf(fp, "\tleaq %d(%%rbp), %%rsp\n",
          space_above_frame_pointer - StackFrameSize(emitter));
}

static void ReloadStructReturnRegisterAtLandingPad(X86Emitter* emitter,
                                                   FILE* fp) {
  if (!X86_IS_64BIT(emitter->rv)) {
    ReloadStructReturnRegisterAtLandingPadI386(emitter, fp);
    return;
  }
  if (emitter->rv->struct_return_reg < 0) {
    return;
  }

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
                 !emitter->rv->not_leaf;
  int struct_return_slot = (is_leaf ? (X86_P(emitter->rv)->first_leaf_int_reg_var)
                                    : (X86_P(emitter->rv)->first_int_reg_var)) +
                           emitter->rv->struct_return_reg;
  char buf[8];
  fprintf(fp, "\tmovq %d(%%rbp), ", emitter->rv->struct_return_spill_offset);
  PrintPercentReg(fp, X86RegisterNameFromNum(
                          struct_return_slot, kX86RegTypeInt,
                          buf, sizeof(buf)));
  fprintf(fp, "\n");
}











// BEGIN I386_EMITTER_EXTRACTED
static int StackFrameSizeI386(X86Emitter* emitter) {
  int body = emitter->rv->base.stack_frame_size +
             emitter->rv->saved_arg_area_size;
  body += BitSetCount(&emitter->regs->used_int_regs) * 8;
  body += BitSetCount(&emitter->regs->used_float_regs) * 8;
  body += emitter->spill_region_size;
  body += emitter->rv->base.varargs ? X86_P(emitter->rv)->vararg_save_area_size : 0;

  // GNU i386: after push/mov the body has %esp % 16 == 8; choose F % 16 == 8 so
  // the steady-state body has %esp % 16 == 0 before nested `call`s.
  if (body <= 0) {
    if (emitter->rv->base.num_calls > 0 || emitter->rv->not_leaf) {
      return X86_P(emitter->rv)->frame_size_mod;
    }
    return 0;
  }
  body = (body + 7) & ~7;
  if ((body & 15) != X86_P(emitter->rv)->frame_size_mod) {
    body += 8;
  }
  return body;
}


static void DecrementStackPointerI386(X86Emitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  char buf[8];
  if (stack_frame_size <= 0) {
    return;
  }
  if (stack_frame_size <= 0x7fffffff) {
    fprintf(fp, "\tsubl ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", %%esp\n");
  } else {
    fprintf(fp, "\tmovl ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\tsubl ");
    PrintPercentReg(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, ", %%esp\n");
  }
}


static COMPILER_UNUSED void IncrementStackPointerI386(X86Emitter* emitter,
                                                  int stack_frame_size,
                                                  FILE* fp) {
  char buf[8];
  if (stack_frame_size <= 0) {
    return;
  }
  if (stack_frame_size <= 0x7fffffff) {
    fprintf(fp, "\taddl ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", %%esp\n");
  } else {
    fprintf(fp, "\tmovl ");
    PrintAsmImmediate(fp, stack_frame_size);
    fprintf(fp, ", ");
    PrintPercentReg(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, "\n\taddl ");
    PrintPercentReg(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                  kX86RegTypeInt, buf,
                                                  sizeof(buf)));
    fprintf(fp, ", %%esp\n");
  }
}

// Load a floating point or integer register from the stack frame.

static void SaveRegistersI386(X86Emitter* emitter, FILE* fp) {
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
      varargs ? X86_P(emitter->rv)->vararg_save_area_size : 0;

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

  // Local variables live at negative offsets from %ebp (see LocalVariableOffset).
  int local_vars = emitter->rv->base.stack_frame_size +
                   emitter->rv->saved_arg_area_size;

  // Callee-saved GPR/XMM slots sit at the top of the frame body (%esp relative).
  int saved_reg_offset = stack_frame_size - 8;

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame, no need to store frame pointer.
  } else {
    fprintf(fp, "\tpushl %%ebp\n");
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_push:\n",
              emitter->rv->base.function_name.value);
    }
    fprintf(fp, "\tmovl %%esp, %%ebp\n");
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_leaq:\n",
              emitter->rv->base.function_name.value);
    }
    if (stack_frame_size > 0) {
      DecrementStackPointer(emitter, stack_frame_size, fp);
    }
  }


  if (emitter->rv->saved_regs.length > 0) {
    fprintf(fp, "\t// Saved argument registers.\n");
  }
  for (size_t i = 0; i < emitter->rv->saved_regs.length; i++) {
    SavedArgumentRegister* saved_reg = emitter->rv->saved_regs.value.p[i];
    int offset = saved_reg->offset;
    if (saved_reg->copy_bytes > 0) {
      // Make a local copy of a large struct argument that was passed by
      // reference: reg_num holds the pointer to the caller's copy and we copy
      // copy_bytes (rounded up to whole words; the ABI pads struct args to a
      // multiple of 8 at both ends, so reading/writing full words is safe) into
      // offset(base_reg_num).  Use r11 as the scratch register since it is
      // never an argument register and is free at this point in the prologue.
      char scbuf[8];
      const char* src = X86RegisterNameFromNum(
          saved_reg->reg_num, kX86RegTypeInt, buf1, sizeof(buf1));
      const char* base = X86RegisterNameFromNum(
          saved_reg->base_reg_num, kX86RegTypeInt, buf2, sizeof(buf2));
      const char* scratch = X86RegisterNameFromNum(
          X86_P(emitter->rv)->spill_addr, kX86RegTypeInt, scbuf, sizeof(scbuf));
      int words = (saved_reg->copy_bytes + 7) / 8;
      for (int w = 0; w < words; w++) {
        int o = w * 8;
        fprintf(fp, "\tmovl %d(", o);
        PrintPercentReg(fp, src);
        fprintf(fp, "), ");
        PrintPercentReg(fp, scratch);
        fprintf(fp, "\n\tmovl ");
        PrintPercentReg(fp, scratch);
        fprintf(fp, ", %d(", offset + o);
        PrintPercentReg(fp, base);
        fprintf(fp, ")\n");
      }
      continue;
    }
    const char* store = "movl";
    X86RegisterType reg_type = kX86RegTypeInt;
    if (saved_reg->is_fp) {
      store = saved_reg->value_bytes == 16
                  ? "movdqu"
                  : saved_reg->value_bytes == 4 ? "storess" : "storesd";
      reg_type = kX86RegTypeFloat;
    }
    fprintf(fp, "\t%s ", store);
    PrintPercentReg(fp, X86RegisterNameFromNum(saved_reg->reg_num,
                                                  reg_type, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(", offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(saved_reg->base_reg_num,
                                                  kX86RegTypeInt, buf2,
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
  int spilled_region_hi_addr =
      local_vars + X86_P(emitter->rv)->stack_frame_header_size;
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
    fprintf(fp, "\tmovl ");
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeInt, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(%%esp)\n", offset);
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
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeFloat, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, ", %d(%%esp)\n", offset);
    BitSetIteratorNext(&it);
  }

  if (!is_leaf) {
    fprintf(fp, "\t// End of stack frame\n");
  }
}


static void RestoreRegistersI386(X86Emitter* emitter, FILE* fp) {
  char buf1[8];

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
                 !emitter->rv->not_leaf;

  if (!is_leaf) {
    fprintf(fp, "\t// Restored registers.\n");
  }

  int offset = emitter->saved_reg_offset;
  BitSetIterator it;

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tmovl %d(%%esp), ", offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeInt, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, "\n");
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tmovsd %d(%%esp), ", offset);
    PrintPercentReg(fp, X86RegisterNameFromNum(reg, kX86RegTypeFloat, buf1,
                                                  sizeof(buf1)));
    fprintf(fp, "\n");
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  if (!EmptyStackFrame(emitter)) {
    if (emitter->rv->struct_return_reg >= 0) {
      fprintf(fp, "\tmovl 8(%%ebp), %%eax\n");
    }
    fprintf(fp, "\tmovl %%ebp, %%esp\n");
    fprintf(fp, "\tpopl %%ebp\n");
  }
}


static void PrintPercentRegI386(FILE* fp, const char* reg) {
  if (reg[0] == '%') {
    fprintf(fp, "%s", reg);
  } else {
    fprintf(fp, "%%%s", reg);
  }
}

static bool I386ConstBits(TargetInstruction* op, int64_t* bits) {
  if (op == NULL) {
    return false;
  }
  if (TargetIsConst(op)) {
    *bits = TargetIntValue(op);
    return true;
  }
  if ((X86Opcode)op->opcode == X86_OP(x0)) {
    *bits = 0;
    return true;
  }
  if (op->operand[0] != NULL && TargetIsConst(op->operand[0])) {
    switch ((X86Opcode)op->opcode) {
      case X86_OP(mv):
      case X86_OP(movc):
      case X86_OP(movdc):
      case X86_OP(movd):
      case X86_OP(movq_xmm):
      case X86_OP(fmv_s):
      case X86_OP(fmv_d):
        *bits = TargetIntValue(op->operand[0]);
        return true;
      default:
        break;
    }
  }
  return false;
}

// Materialize a 32- or 64-bit bit-pattern into an XMM register.  i386 has no
// 64-bit GPR immediate, so doubles go through a pair of stack slots.
static void EmitI386ImmToXmm(FILE* fp, int64_t bits, bool is_double,
                             const char* xmm) {
  if (is_double) {
    fprintf(fp, "\tsubl $8, %%esp\n");
    fprintf(fp, "\tmovl $%x, (%%esp)\n", (unsigned)(uint32_t)(uint64_t)bits);
    fprintf(fp, "\tmovl $%x, 4(%%esp)\n",
            (unsigned)(uint32_t)((uint64_t)bits >> 32));
    fprintf(fp, "\tmovsd (%%esp), ");
    PrintPercentRegI386(fp, xmm);
    fprintf(fp, "\n\taddl $8, %%esp\n");
  } else {
    fprintf(fp, "\tpushl %%ebx\n\tmovl $%x, %%ebx\n\tmovd %%ebx, ",
            (unsigned)(uint32_t)(uint64_t)bits);
    PrintPercentRegI386(fp, xmm);
    fprintf(fp, "\n\tpopl %%ebx\n");
  }
}

static AsmOperand* GetAsmOperandI386(X86AsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    return NULL;
  }
  if ((size_t)index < inst->asm_node->outputs.length) {
    return inst->asm_node->outputs.value.p[index];
  }
  return inst->asm_node->inputs.value.p[index - inst->asm_node->outputs.length];
}

static int FindAsmOperandByNameI386(X86AsmInstruction* inst, const char* name,
                                size_t len) {
  for (int i = 0; i < inst->num_operands; i++) {
    AsmOperand* operand = GetAsmOperandI386(inst, i);
    if (operand != NULL && operand->name.length == len &&
        strncmp(operand->name.value, name, len) == 0) {
      return i;
    }
  }
  return -1;
}

static int FindAsmLabelByNameI386(X86AsmInstruction* inst, const char* name,
                              size_t len) {
  for (size_t i = 0; i < inst->asm_node->labels.length; i++) {
    String* label = inst->asm_node->labels.value.p[i];
    if (label->length == len && strncmp(label->value, name, len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void PrintAsmOperandI386(FILE* fp, X86AsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    fprintf(fp, "<bad-asm-operand>");
    return;
  }
  if (inst->reg_nums[index] == INT_MIN) {
    PrintAsmImmediate(fp, inst->immediate_values[index]);
    return;
  }
  char buf[32];
  PrintPercentRegI386(fp, X86RegisterNameFromNum(inst->reg_nums[index],
                                                inst->is_fp[index]
                                                    ? kX86RegTypeFloat
                                                    : kX86RegTypeInt,
                                                buf, sizeof(buf)));
}

static void PrintAsmLabelI386(FILE* fp, X86AsmInstruction* inst, int index) {
  if (index < 0 || (size_t)index >= inst->asm_node->labels.length) {
    fprintf(fp, "<bad-asm-label>");
    return;
  }
  String* label = inst->asm_node->labels.value.p[index];
  fprintf(fp, "%s", label->value);
}

static void PrintExtendedAsmI386(FILE* fp, X86AsmInstruction* inst,
                             const char* text) {
  for (const char* p = text; *p != '\0'; p++) {
    if (*p != '%') {
      fputc(*p, fp);
      continue;
    }
    p++;
    if (*p == '%') {
      fputc('%', fp);
      continue;
    }
    char modifier = 0;
    if (*p == 'q' || *p == 'k' || *p == 'b' || *p == 'w' || *p == 'l') {
      modifier = *p++;
    }
    int index = -1;
    if (*p == '[') {
      const char* name = ++p;
      while (*p != '\0' && *p != ']') {
        p++;
      }
      index = modifier == 'l'
                  ? FindAsmLabelByNameI386(inst, name, (size_t)(p - name))
                  : FindAsmOperandByNameI386(inst, name, (size_t)(p - name));
    } else if (*p >= '0' && *p <= '9') {
      index = 0;
      while (*p >= '0' && *p <= '9') {
        index = index * 10 + (*p - '0');
        p++;
      }
      p--;
    }
    if (modifier == 'l') {
      PrintAsmLabelI386(fp, inst, index);
    } else {
      PrintAsmOperandI386(fp, inst, index);
    }
  }
}

static bool IsFloatRegisterInstI386(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  if (inst->reg != NULL) {
    return ((X86Register*)inst->reg)->type == kX86RegTypeFloat;
  }
  return X86IsFloatingPoint(inst);
}

static const char* MoveMnemonicForInstI386(TargetInstruction* src,
                                       TargetInstruction* dest) {
  if (dest != NULL && dest->reg != NULL &&
      ((X86Register*)dest->reg)->type == kX86RegTypeInt) {
    if (IsFloatRegisterInstI386(src)) {
      return "movd";
    }
    return "movl";
  }
  if (IsFloatRegisterInstI386(src) || IsFloatRegisterInstI386(dest)) {
    return "movsd";
  }
  return "movl";
}

static void PrintPercentRegFromInstI386(FILE* fp, TargetInstruction* inst,
                                    char* buf, size_t len) {
  PrintPercentRegI386(fp, GetRegisterName(inst, buf, len));
}

static void PrintResultRegFromInstI386(FILE* fp, TargetInstruction* inst, char* buf,
                                   size_t len) {
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    PrintPercentRegI386(fp, X86RegisterName((X86Register*)inst->dest->reg, buf,
                                           len));
  } else {
    PrintPercentRegFromInstI386(fp, inst, buf, len);
  }
}

static void PrintMemoryBaseRegFromInstI386(FILE* fp, TargetInstruction* inst,
                                       char* buf, size_t len) {
  const char* reg = GetRegisterName(inst, buf, len);
  if (reg[0] == '\0' && inst != NULL) {
    char buf2[8];
    switch ((X86Opcode)inst->opcode) {
      case X86_OP(add):
      case X86_OP(addl):
      case X86_OP(sub):
      case X86_OP(subl):
        if (inst->operand[0] != NULL) {
          reg = GetRegisterName(inst->operand[0], buf2, sizeof(buf2));
        }
        break;
      default:
        break;
    }
  }
  if (reg[0] == '\0') {
    reg = "ebp";
  }
  PrintPercentRegI386(fp, reg);
}

static bool PrintTlsTporffMemoryOperandI386(FILE* fp, TargetInstruction* addr,
                                        TargetInstruction* offset, char* buf,
                                        size_t bufsz) {
  TargetInstruction* tp = addr;
  TargetInstruction* tporff = offset;
  if ((X86Opcode)addr->opcode == X86_OP(add)) {
    tp = addr->operand[0];
    tporff = addr->operand[1];
  }
  if (tp == NULL || tporff == NULL ||
      (X86Opcode)tporff->opcode != X86_OP(movxc) ||
      (tporff->flags & X86_TLS_RELOC) == 0 || tporff->operand[0] == NULL ||
      (int)tporff->operand[0]->opcode != (int)X86_OP(symbol)) {
    return false;
  }
  char namebuf[256];
  fprintf(fp, "%s@TPOFF(", TargetSymbolName(
                               ((TargetSymbol*)tporff->operand[0])->symbol,
                               namebuf, sizeof(namebuf)));
  PrintMemoryBaseRegFromInstI386(fp, tp, buf, bufsz);
  fprintf(fp, ")");
  return true;
}

static X86RegisterType InstResultRegTypeI386(TargetInstruction* inst) {
  if (inst == NULL) {
    return kX86RegTypeInt;
  }
  if (inst->reg != NULL) {
    return ((X86Register*)inst->reg)->type;
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return ((X86Register*)inst->dest->reg)->type;
  }
  if (X86IsFloatingPoint(inst)) {
    return kX86RegTypeFloat;
  }
  return kX86RegTypeInt;
}

static bool OperandHasIntRegI386(TargetInstruction* op) {
  if (op == NULL) {
    return false;
  }
  if (op->reg != NULL) {
    return ((X86Register*)op->reg)->type == kX86RegTypeInt;
  }
  return false;
}

static const char* BranchMnemonicI386(X86Opcode opcode) {
  return X86OpcodeName((int)opcode);
}

// True when this operand must be emitted as an immediate (it has no register).
// The zero pseudo-register x0 is the constant 0; x86-64 has no hardware zero
// register so it cannot be used as a real register operand.
static bool BranchOperandIsImmediateI386(TargetInstruction* op) {
  return op != NULL &&
         (TargetIsConst(op) || (int)op->opcode == (int)X86_OP(x0));
}

// The compare-and-branch form emits "cmp src, dst" (AT&T), computing dst - src,
// and the assembler requires any immediate to be the source.  When the
// destination operand is an immediate we must compare in the other direction
// (src - dst) and invert the condition.  Returns the inverted mnemonic.
static const char* SwappedBranchMnemonicI386(X86Opcode opcode) {
  switch (opcode) {
    case X86_OP(je):
      return "je";
    case X86_OP(jne):
      return "jne";
    case X86_OP(jl):
      return "jg";
    case X86_OP(jge):
      return "jle";
    case X86_OP(jb):
      return "ja";
    case X86_OP(jae):
      return "jbe";
    default:
      return NULL;
  }
}

static const char* AttMnemonicI386(X86Opcode opcode) {
  switch (opcode) {
    case X86_OP(imul):
      return "imull";
    case X86_OP(imull):
      return "imull";
    case X86_OP(idiv):
      return "idivl";
    case X86_OP(div):
      return "divl";
    case X86_OP(mod):
      return "idivl";
    case X86_OP(not):
      return "notl";
    case X86_OP(neg):
      return "negl";
    case X86_OP(cmp):
      return "cmpl";
    case X86_OP(test):
      return "testl";
    case X86_OP(movslq):
      return "movl";
    case X86_OP(fmv_s):
      return "movss";
    case X86_OP(fmv_d):
      return "movsd";
    case X86_OP(movq_xmm):
      return "movl";
    default:
      return X86OpcodeName((int)opcode);
  }
}

static void PrintAttOperandI386(FILE* fp, TargetInstruction* op, char* buf,
                            size_t len) {
  if (op == NULL) {
    return;
  }
  if (TargetIsConst(op)) {
    PrintAsmImmediate(fp, TargetIntValue(op));
  } else if (((int)op->opcode == (int)X86_OP(symbol))) {
    char namebuf[256];
    const char* symname =
        TargetSymbolName(((TargetSymbol*)op)->symbol, namebuf, sizeof(namebuf));
    if ((op->flags & X86_HI_RELOC) != 0) {
      fprintf(fp, "%%hi(%s)", symname);
    } else {
      fprintf(fp, "%s", symname);
    }
  } else if (((int)op->opcode == (int)X86_OP(literal))) {
    TargetLiteral* literal = (TargetLiteral*)op;
    char litname[64];
    LiteralAsmName(literal->literal_id, litname, sizeof(litname));
    fprintf(fp, "%s", litname);
  } else if (((int)op->opcode == (int)X86_OP(x0))) {
    fprintf(fp, "$0");
  } else {
    PrintPercentRegFromInstI386(fp, op, buf, len);
  }
}

// The logical register file has more slots than there are physical x86-64
// registers, so several distinct logical slots alias the same physical
// register (e.g. logical slot 14 and slot 28 both denote r8 -- see the
// kIntRegNames table in x86_reg_alloc.c).  Two-address handling must reason
// about the *physical* register a value occupies, not the logical slot: if a
// source and the destination land on the same physical register through
// different logical slots, a naive "mov src0 -> dest" still clobbers the other
// source.  Compare by the rendered physical register name, which is unique per
// physical register and distinct across int/xmm banks.
static bool SamePhysicalRegI386(TargetRegister* a, TargetRegister* b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  if (a == b) {
    return true;
  }
  char buf_a[32];
  char buf_b[32];
  X86RegisterName((X86Register*)a, buf_a, sizeof(buf_a));
  X86RegisterName((X86Register*)b, buf_b, sizeof(buf_b));
  return strcmp(buf_a, buf_b) == 0;
}

static void PrintMovToDestIfNeededI386(FILE* fp, TargetInstruction* inst,
                                   char* buf1, char* buf2) {
  TargetRegister* dest_reg = inst->reg;
  if (dest_reg == NULL && inst->dest != NULL) {
    dest_reg = inst->dest->reg;
  }
  if (inst->operand[0] != NULL &&
      (TargetIsConst(inst->operand[0]) ||
       (X86Opcode)inst->operand[0]->opcode == X86_OP(x0)) &&
      dest_reg != NULL) {
    if (((X86Register*)dest_reg)->type == kX86RegTypeFloat) {
      bool is_double =
          (X86Opcode)inst->opcode == X86_OP(fmv_d) ||
          (X86Opcode)inst->opcode == X86_OP(movsd) ||
          (X86Opcode)inst->opcode == X86_OP(fneg_sd);
      int64_t bits = TargetIsConst(inst->operand[0])
                         ? TargetIntValue(inst->operand[0])
                         : 0;
      EmitI386ImmToXmm(
          fp, bits, is_double,
          X86RegisterName((X86Register*)dest_reg, buf1, sizeof(buf1)));
      return;
    }
    fprintf(fp, "\tmovl ");
    if (TargetIsConst(inst->operand[0])) {
      PrintAsmImmediate(fp, (int64_t)(int32_t)TargetIntValue(inst->operand[0]));
    } else {
      fprintf(fp, "$0");
    }
    fprintf(fp, ", ");
    PrintPercentRegI386(
        fp, X86RegisterName((X86Register*)dest_reg, buf1, sizeof(buf1)));
    fprintf(fp, "\n");
    return;
  }
  if (inst->operand[0] != NULL &&
      inst->operand[0]->reg != NULL &&
      dest_reg != NULL &&
      !SamePhysicalRegI386(inst->operand[0]->reg, dest_reg)) {
    const char* mov = "movl";
    if (X86IsFloatingPoint(inst)) {
      bool is_double =
          (X86Opcode)inst->opcode == X86_OP(fmv_d) ||
          (X86Opcode)inst->opcode == X86_OP(addsd) ||
          (X86Opcode)inst->opcode == X86_OP(subsd) ||
          (X86Opcode)inst->opcode == X86_OP(mulsd) ||
          (X86Opcode)inst->opcode == X86_OP(divsd) ||
          (X86Opcode)inst->opcode == X86_OP(ucomisd) ||
          (X86Opcode)inst->opcode == X86_OP(cvtsi2sd) ||
          (X86Opcode)inst->opcode == X86_OP(cvttsd2si) ||
          (X86Opcode)inst->opcode == X86_OP(cvtss2sd) ||
          (X86Opcode)inst->opcode == X86_OP(cvtsd2ss) ||
          (X86Opcode)inst->opcode == X86_OP(fneg_sd);
      if ((inst->flags & X86_VECTOR_VALUE) != 0) {
        mov = "movdqu";
      } else if (((X86Register*)inst->operand[0]->reg)->type == kX86RegTypeInt &&
          inst->reg != NULL &&
          ((X86Register*)inst->reg)->type == kX86RegTypeFloat) {
        mov = "movd";
      } else {
        mov = is_double ? "movsd" : "movss";
      }
    }
    fprintf(fp, "\t%s ", mov);
    PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentRegI386(
        fp, X86RegisterName((X86Register*)dest_reg, buf2, sizeof(buf2)));
    fprintf(fp, "\n");
  }
}

static void PrepareSseSourceOperandToI386(FILE* fp, TargetInstruction* op,
                                          const char* xmm, char* buf,
                                          size_t len) {
  if (op != NULL && op->reg != NULL &&
      ((X86Register*)op->reg)->type == kX86RegTypeInt) {
    fprintf(fp, "\tmovd ");
    PrintPercentRegFromInstI386(fp, op, buf, len);
    fprintf(fp, ", ");
    PrintPercentRegI386(fp, xmm);
    fprintf(fp, "\n");
  }
}

static void PrintSseSourceOperandFromI386(FILE* fp, TargetInstruction* op,
                                          const char* xmm, char* buf,
                                          size_t len) {
  if (op == NULL) {
    return;
  }
  if (op->reg != NULL &&
      ((X86Register*)op->reg)->type == kX86RegTypeInt) {
    PrintPercentRegI386(fp, xmm);
    return;
  }
  PrintAttOperandI386(fp, op, buf, len);
}

static void PrepareSseSourceOperandI386(FILE* fp, TargetInstruction* op, char* buf,
                                    size_t len) {
  PrepareSseSourceOperandToI386(fp, op, SseScratch(), buf, len);
}

static void PrintSseSourceOperandI386(FILE* fp, TargetInstruction* op, char* buf,
                                  size_t len) {
  PrintSseSourceOperandFromI386(fp, op, SseScratch(), buf, len);
}

static void PrintBinaryRegOpI386(FILE* fp, const char* mnemonic,
                             TargetInstruction* inst, char* buf1, char* buf2) {
  X86Opcode opcode = (X86Opcode)inst->opcode;
  if (X86IsFloatingPoint(inst) &&
      InstResultRegTypeI386(inst) == kX86RegTypeInt) {
    const char* mov =
        opcode == X86_OP(addsd) || opcode == X86_OP(subsd) ||
                opcode == X86_OP(mulsd) || opcode == X86_OP(divsd)
            ? "movsd"
            : "movss";
    if (inst->operand[0] != NULL && inst->operand[0]->reg != NULL &&
        ((X86Register*)inst->operand[0]->reg)->type == kX86RegTypeInt) {
      fprintf(fp, "\tmovd ");
      PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", %%xmm6\n");
    } else {
      fprintf(fp, "\t%s ", mov);
      PrintSseSourceOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", %%xmm6\n");
    }
    if (inst->operand[1] != NULL) {
      PrepareSseSourceOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
    }
    fprintf(fp, "\t%s ", mnemonic);
    PrintSseSourceOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
    fprintf(fp, ", %%xmm6\n\tmovl %%xmm6, ");
    PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
    fprintf(fp, "\n");
    return;
  }
  // These are two-address operations: PrintMovToDestIfNeeded copies operand[0]
  // into the destination register and operand[1] is then used as the source.
  // If operand[1] already lives in the destination register (e.g. a call result
  // coalesced into %rax that is the dest of "i * factorial(...)"), that initial
  // "mov operand[0] -> dest" would clobber operand[1].  For commutative
  // operations swap the two sources so the operand already sitting in dest is
  // preserved.  (sub/div are not commutative and are not handled here.)
  bool commutative = opcode == X86_OP(imul) || opcode == X86_OP(imull) ||
                     opcode == X86_OP(addss) || opcode == X86_OP(addsd) ||
                     opcode == X86_OP(mulss) || opcode == X86_OP(mulsd) ||
                     opcode == X86_OP(addps) || opcode == X86_OP(addpd) ||
                     opcode == X86_OP(mulps) || opcode == X86_OP(mulpd) ||
                     opcode == X86_OP(paddb) || opcode == X86_OP(paddw) ||
                     opcode == X86_OP(paddd) || opcode == X86_OP(paddq) ||
                     opcode == X86_OP(pand) || opcode == X86_OP(por) ||
                     opcode == X86_OP(pxor) ||
                     opcode == X86_OP(pcmpeqb) ||
                     opcode == X86_OP(pcmpeqw) ||
                     opcode == X86_OP(pcmpeqd);
  bool non_commutative_sse = opcode == X86_OP(subss) ||
                             opcode == X86_OP(subsd) ||
                             opcode == X86_OP(divss) ||
                             opcode == X86_OP(divsd) ||
                             opcode == X86_OP(subps) ||
                             opcode == X86_OP(subpd) ||
                             opcode == X86_OP(divps) ||
                             opcode == X86_OP(divpd) ||
                             opcode == X86_OP(psubb) ||
                             opcode == X86_OP(psubw) ||
                             opcode == X86_OP(psubd) ||
                             opcode == X86_OP(psubq) ||
                             opcode == X86_OP(pcmpgtb) ||
                             opcode == X86_OP(pcmpgtw) ||
                             opcode == X86_OP(pcmpgtd);
  bool swapped = false;
  if (commutative && inst->operand[0] != NULL && inst->operand[1] != NULL &&
      !TargetIsConst(inst->operand[1]) &&
      SamePhysicalRegI386(inst->operand[1]->reg, inst->reg) &&
      !SamePhysicalRegI386(inst->operand[0]->reg, inst->reg)) {
    TargetInstruction* tmp = inst->operand[0];
    inst->operand[0] = inst->operand[1];
    inst->operand[1] = tmp;
    swapped = true;
  }
  bool saved_sse_rhs = false;
  if (non_commutative_sse && inst->operand[0] != NULL &&
      inst->operand[1] != NULL &&
      SamePhysicalRegI386(inst->operand[1]->reg, inst->reg) &&
      !SamePhysicalRegI386(inst->operand[0]->reg, inst->reg)) {
    const char* mov =
        (inst->flags & X86_VECTOR_VALUE) != 0
            ? "movdqu"
            : opcode == X86_OP(subsd) || opcode == X86_OP(divsd)
                  ? "movsd"
                  : "movss";
    fprintf(fp, "\t%s ", mov);
    PrintSseSourceOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
    fprintf(fp, ", %%xmm7\n");
    saved_sse_rhs = true;
  }
  PrintMovToDestIfNeededI386(fp, inst, buf1, buf2);
  if (X86IsFloatingPoint(inst) && inst->operand[1] != NULL &&
      !saved_sse_rhs) {
    PrepareSseSourceOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, "\t%s ", mnemonic);
  if (saved_sse_rhs) {
    PrintPercentRegI386(fp, X86CurrentRegisterNameProfile()->sse_scratch);
  } else if (X86IsFloatingPoint(inst)) {
    PrintSseSourceOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
  } else {
    PrintAttOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, ", ");
  PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
  fprintf(fp, "\n");
  if (swapped) {
    TargetInstruction* tmp = inst->operand[0];
    inst->operand[0] = inst->operand[1];
    inst->operand[1] = tmp;
  }
}

static void PrintSetccResultI386(FILE* fp, const char* set_mnemonic,
                                TargetInstruction* inst, char* buf1,
                                char* buf2) {
  char name[16];
  const char* dest = GetRegisterName(inst, name, sizeof(name));
  bool has_byte_register =
      strcmp(dest, "eax") == 0 || strcmp(dest, "ebx") == 0 ||
      strcmp(dest, "ecx") == 0 || strcmp(dest, "edx") == 0;
  if (!has_byte_register) {
    // i386 cannot name the low byte of esi or edi (sil/dil require a REX
    // prefix, where byte 0x40 is instead `inc %eax` in 32-bit mode).  Preserve
    // eax while using al as a byte-addressable staging register.
    fprintf(fp, "\tpushl %%eax\n\t%s %%eax\n\tmovzbl %%eax, ",
            set_mnemonic);
    PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
    fprintf(fp, "\n\tpopl %%eax\n");
    return;
  }
  fprintf(fp, "\t%s ", set_mnemonic);
  PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
  fprintf(fp, "\n\tmovzbl ");
  PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
  fprintf(fp, ", ");
  PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
  fprintf(fp, "\n");
}

static void PrintCompareAndSetI386(FILE* fp, const char* set_mnemonic,
                               TargetInstruction* inst, char* buf1,
                               char* buf2) {
  if ((inst->flags & (X86_FCMP_SS | X86_FCMP_SD)) != 0) {
    // Floating-point comparison: the flags come from ucomiSS/SD rather than an
    // integer cmp.  AT&T order is "ucomi src, dst" and the condition codes are
    // evaluated as dst <cc> src, so emit "ucomi operand[1], operand[0]".
    const char* cmp_mnemonic =
        (inst->flags & X86_FCMP_SD) != 0 ? "ucomisd" : "ucomiss";
    const char* src_scratch = SseScratch();
    const char* dst_scratch = SseScratch2();
    PrepareSseSourceOperandToI386(fp, inst->operand[1], src_scratch, buf1,
                                  sizeof(buf1));
    PrepareSseSourceOperandToI386(fp, inst->operand[0], dst_scratch, buf2,
                                  sizeof(buf2));
    fprintf(fp, "\t%s ", cmp_mnemonic);
    PrintSseSourceOperandFromI386(fp, inst->operand[1], src_scratch, buf1,
                                  sizeof(buf1));
    fprintf(fp, ", ");
    PrintSseSourceOperandFromI386(fp, inst->operand[0], dst_scratch, buf2,
                                  sizeof(buf2));
    fprintf(fp, "\n");
    PrintSetccResultI386(fp, set_mnemonic, inst, buf1, buf2);
    return;
  }
  if (inst->operand[1] != NULL) {
    // The set* lowering treats operand[0] as the left operand and operand[1] as
    // the right operand of the relation (e.g. setl means operand[0] < operand[1],
    // matching a RISC-style slt rd, op0, op1).  In AT&T syntax "cmp src, dst"
    // computes dst - src and the condition codes are evaluated as dst <cc> src,
    // so emit "cmp operand[1], operand[0]" (dst = operand[0], src = operand[1]).
    // If operand[0] is an immediate, materialize it into scratch first because
    // x86 cannot encode an immediate compare destination.
    TargetInstruction* lhs = inst->operand[0];
    if (TargetIsConst(lhs) || (X86Opcode)lhs->opcode == X86_OP(x0)) {
      fprintf(fp, "\tmovl ");
      if (TargetIsConst(lhs)) {
        PrintAsmImmediate(fp, TargetIntValue(lhs));
      } else {
        fprintf(fp, "$0");
      }
      fprintf(fp, ", %%ebx\n");
      lhs = NULL;
    }
    fprintf(fp, "\tcmp ");
    PrintAttOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    if (lhs == NULL) {
      fprintf(fp, "%%ebx");
    } else {
      PrintAttOperandI386(fp, lhs, buf2, sizeof(buf2));
    }
    fprintf(fp, "\n");
  } else if (inst->operand[0] != NULL) {
    TargetInstruction* op = inst->operand[0];
    if (TargetIsConst(op) || (X86Opcode)op->opcode == X86_OP(x0)) {
      fprintf(fp, "\tmovl ");
      if (TargetIsConst(op)) {
        PrintAsmImmediate(fp, TargetIntValue(op));
      } else {
        fprintf(fp, "$0");
      }
      fprintf(fp, ", %%ebx\n");
      fprintf(fp, "\ttest %%ebx, %%ebx\n");
    } else {
      fprintf(fp, "\ttest ");
      PrintAttOperandI386(fp, op, buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintAttOperandI386(fp, op, buf2, sizeof(buf2));
      fprintf(fp, "\n");
    }
  }
  // setcc only writes one byte; materialize a clean full-width 0/1 result.
  PrintSetccResultI386(fp, set_mnemonic, inst, buf1, buf2);
}

static void PrintIdivFamilyI386(FILE* fp, X86Opcode opcode,
                            TargetInstruction* inst, char* buf1, char* buf2) {
  // The divide reads its dividend from rax and rdx, both of which are written
  // below before it runs.  A divisor sitting in either is therefore gone by the
  // time the divide wants it, so move it aside first, while it is still there.
  bool divisor_moved = false;
  if (inst->operand[1] != NULL && inst->operand[1]->reg != NULL) {
    char div_reg[32];
    X86RegisterName((X86Register*)inst->operand[1]->reg, div_reg,
                       sizeof(div_reg));
    if (strcmp(div_reg, "rax") == 0 || strcmp(div_reg, "rdx") == 0) {
      fprintf(fp, "\tmovl %%%s, %%ebx\n", div_reg);
      divisor_moved = true;
    }
  }
  if (inst->operand[0] != NULL) {
    fprintf(fp, "\tmovl ");
    PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", %%eax\n");
  }
  if ((X86Opcode)opcode == X86_OP(div) ||
      ((X86Opcode)opcode == X86_OP(mod) &&
       (inst->flags & X86_UNSIGNED_MOD) != 0)) {
    fprintf(fp, "\txorl %%edx, %%edx\n");
  } else {
    fprintf(fp, "\tcdq\n");
  }
  const char* div_mnemonic = AttMnemonicI386(opcode);
  if ((X86Opcode)opcode == X86_OP(mod) &&
      (inst->flags & X86_UNSIGNED_MOD) != 0) {
    div_mnemonic = "divl";
  }
  fprintf(fp, "\t%s ", div_mnemonic);
  if (divisor_moved) {
    fprintf(fp, "%%ebx");
  } else {
    PrintAttOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
  }
  fprintf(fp, "\n");
  if (inst->reg != NULL) {
    const char* result =
        (X86Opcode)opcode == X86_OP(mod) ? "%edx" : "%eax";
    fprintf(fp, "\tmovl %s, ", result);
    PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
    fprintf(fp, "\n");
  }
}

static void PrintSseIntConvertI386(FILE* fp, const char* mnemonic,
                               TargetInstruction* inst, bool int_to_xmm,
                               char* buf1, char* buf2) {
  if (!int_to_xmm && inst->operand[0] != NULL) {
    PrepareSseSourceOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
  }
  fprintf(fp, "\t%s ", mnemonic);
  if (int_to_xmm) {
    PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
  } else {
    PrintSseSourceOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
    fprintf(fp, ", ");
    PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
  }
  fprintf(fp, "\n");
}

static void PrintDefaultInstructionI386(FILE* fp, TargetInstruction* inst,
                                    char* buf1, char* buf2) {
  X86Opcode opcode = (X86Opcode)inst->opcode;

  switch (opcode) {
    case X86_OP(imul):
    case X86_OP(imull):
    case X86_OP(addss):
    case X86_OP(addsd):
    case X86_OP(subss):
    case X86_OP(subsd):
    case X86_OP(mulss):
    case X86_OP(mulsd):
    case X86_OP(divss):
    case X86_OP(divsd):
    case X86_OP(paddb):
    case X86_OP(paddw):
    case X86_OP(paddd):
    case X86_OP(paddq):
    case X86_OP(psubb):
    case X86_OP(psubw):
    case X86_OP(psubd):
    case X86_OP(psubq):
    case X86_OP(pand):
    case X86_OP(por):
    case X86_OP(pxor):
    case X86_OP(pcmpeqb):
    case X86_OP(pcmpeqw):
    case X86_OP(pcmpeqd):
    case X86_OP(pcmpgtb):
    case X86_OP(pcmpgtw):
    case X86_OP(pcmpgtd):
    case X86_OP(addps):
    case X86_OP(addpd):
    case X86_OP(subps):
    case X86_OP(subpd):
    case X86_OP(mulps):
    case X86_OP(mulpd):
    case X86_OP(divps):
    case X86_OP(divpd):
      PrintBinaryRegOpI386(fp, AttMnemonicI386(opcode), inst, buf1, buf2);
      return;

    case X86_OP(idiv):
    case X86_OP(div):
    case X86_OP(mod):
      PrintIdivFamilyI386(fp, opcode, inst, buf1, buf2);
      return;

    case X86_OP(not):
    case X86_OP(neg):
    case X86_OP(sqrtss):
    case X86_OP(sqrtsd):
      PrintMovToDestIfNeededI386(fp, inst, buf1, buf2);
      fprintf(fp, "\t%s ", AttMnemonicI386(opcode));
      PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      return;

    case X86_OP(fneg_ss):
    case X86_OP(fneg_sd):
      PrintMovToDestIfNeededI386(fp, inst, buf1, buf2);
      if (opcode == X86_OP(fneg_sd)) {
        fprintf(fp, "\tsubl $8, %%esp\n\tstoresd ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp,
                ", (%%esp)\n\tpushl %%ebx\n\tmovl $80000000, %%ebx\n"
                "\txorl %%ebx, 4(%%esp)\n\tpopl %%ebx\n\tmovsd (%%esp), ");
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n\taddl $8, %%esp\n");
      } else {
        fprintf(fp, "\tpushl %%ebx\n\tmovd ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, ", %%ebx\n\txorl $80000000, %%ebx\n\tmovd %%ebx, ");
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n\tpopl %%ebx\n");
      }
      return;

    case X86_OP(cmp):
      if (TargetIsConst(inst->operand[0]) ||
          (X86Opcode)inst->operand[0]->opcode == X86_OP(x0)) {
        fprintf(fp, "\tmovl ");
        if (TargetIsConst(inst->operand[0])) {
          PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
        } else {
          fprintf(fp, "$0");
        }
        fprintf(fp, ", %%ebx\n");
      }
      fprintf(fp, "\t%s ", AttMnemonicI386(opcode));
      PrintAttOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      if (TargetIsConst(inst->operand[0]) ||
          (X86Opcode)inst->operand[0]->opcode == X86_OP(x0)) {
        fprintf(fp, "%%ebx");
      } else {
        PrintAttOperandI386(fp, inst->operand[0], buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      return;

    case X86_OP(test):
      fprintf(fp, "\t%s ", AttMnemonicI386(opcode));
      PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintAttOperandI386(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;

    case X86_OP(setl):
      PrintCompareAndSetI386(fp, "setl", inst, buf1, buf2);
      return;
    case X86_OP(setb):
      PrintCompareAndSetI386(fp, "setb", inst, buf1, buf2);
      return;
    case X86_OP(setg):
      PrintCompareAndSetI386(fp, "setg", inst, buf1, buf2);
      return;
    case X86_OP(setge):
      PrintCompareAndSetI386(fp, "setge", inst, buf1, buf2);
      return;
    case X86_OP(setae):
      PrintCompareAndSetI386(fp, "setae", inst, buf1, buf2);
      return;
    case X86_OP(sete):
      PrintCompareAndSetI386(fp, "sete", inst, buf1, buf2);
      return;
    case X86_OP(setne):
      PrintCompareAndSetI386(fp, "setne", inst, buf1, buf2);
      return;

    case X86_OP(ucomiss):
    case X86_OP(ucomisd): {
      const char* src_scratch = SseScratch();
      const char* dst_scratch = SseScratch2();
      PrepareSseSourceOperandToI386(fp, inst->operand[1], src_scratch, buf1,
                                    sizeof(buf1));
      PrepareSseSourceOperandToI386(fp, inst->operand[0], dst_scratch, buf2,
                                    sizeof(buf2));
      fprintf(fp, "\t%s ", AttMnemonicI386(opcode));
      PrintSseSourceOperandFromI386(fp, inst->operand[1], src_scratch, buf1,
                                    sizeof(buf1));
      fprintf(fp, ", ");
      PrintSseSourceOperandFromI386(fp, inst->operand[0], dst_scratch, buf2,
                                    sizeof(buf2));
      fprintf(fp, "\n");
      return;
    }

    case X86_OP(cvtsi2ss):
    case X86_OP(cvtsi2sd):
      PrintSseIntConvertI386(fp, AttMnemonicI386(opcode), inst, true, buf1, buf2);
      return;
    case X86_OP(cvttss2si):
    case X86_OP(cvttsd2si):
      PrintSseIntConvertI386(fp, AttMnemonicI386(opcode), inst, false, buf1, buf2);
      return;

    case X86_OP(cvtss2sd):
    case X86_OP(cvtsd2ss):
      // These are unary scalar conversions: the (single) source is operand[0]
      // and the converted value is written to the destination register
      // (`cvtXX2YY %src, %dst`).  PrintBinaryRegOp would treat operand[1] as the
      // source, which is NULL for a unary op and produces an empty operand.
      if (InstResultRegTypeI386(inst) == kX86RegTypeInt) {
        PrepareSseSourceOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\t%s ", AttMnemonicI386(opcode));
        PrintSseSourceOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", %%xmm7\n\tmovl %%xmm7, ");
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
        return;
      }
      PrintSseIntConvertI386(fp, AttMnemonicI386(opcode), inst, false, buf1, buf2);
      return;

    case X86_OP(movq_xmm):
    case X86_OP(fmv_s):
    case X86_OP(fmv_d):
      if (((X86Opcode)opcode == X86_OP(fmv_s) ||
           (X86Opcode)opcode == X86_OP(fmv_d)) &&
          TargetIsConst(inst->operand[0]) &&
          InstResultRegTypeI386(inst) == kX86RegTypeFloat) {
        const char* xmm = (inst->dest != NULL && inst->dest->reg != NULL)
                              ? X86RegisterName((X86Register*)inst->dest->reg,
                                                buf2, sizeof(buf2))
                              : GetRegisterName(inst, buf2, sizeof(buf2));
        EmitI386ImmToXmm(fp, TargetIntValue(inst->operand[0]),
                         (X86Opcode)opcode == X86_OP(fmv_d), xmm);
        return;
      }
      if ((X86Opcode)opcode == X86_OP(movq_xmm) &&
          inst->operand[0] != NULL &&
          ((inst->dest != NULL && inst->dest->reg != NULL &&
            ((X86Register*)inst->dest->reg)->type ==
                kX86RegTypeFloat) ||
           ((inst->dest == NULL || inst->dest->reg == NULL) &&
            inst->reg != NULL &&
            ((X86Register*)inst->reg)->type ==
                kX86RegTypeFloat))) {
        if (TargetIsConst(inst->operand[0])) {
          const char* xmm = (inst->dest != NULL && inst->dest->reg != NULL)
                                ? X86RegisterName((X86Register*)inst->dest->reg,
                                                  buf2, sizeof(buf2))
                                : GetRegisterName(inst, buf2, sizeof(buf2));
          EmitI386ImmToXmm(fp, TargetIntValue(inst->operand[0]), true, xmm);
          return;
        }
        if ((int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
          // x0 is a compiler pseudo-register, not a physical x86 zero
          // register. Materialize its bits before moving them into XMM;
          // printing x0 as a register otherwise aliases %rax and returns
          // whatever integer value the preceding call left there.
          fprintf(fp, "\tmovl $0, %%ebx\n\tmovd %%ebx, ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
        if (inst->operand[0]->reg != NULL &&
            ((X86Register*)inst->operand[0]->reg)->type ==
                kX86RegTypeInt) {
          fprintf(fp, "\tmovd ");
          PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      /* Fall through for generic movq_xmm / movss / movsd emission. */
    case X86_OP(movss):
    case X86_OP(movsd):
    case X86_OP(movd): {
      X86RegisterType dest_type = kX86RegTypeFloat;
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        dest_type = ((X86Register*)inst->dest->reg)->type;
      } else if (inst->reg != NULL) {
        dest_type = ((X86Register*)inst->reg)->type;
      }
      if (dest_type == kX86RegTypeInt) {
        if (inst->operand[0] != NULL) {
          if (TargetIsConst(inst->operand[0])) {
            fprintf(fp, "\tmovl ");
            PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          if (inst->operand[0]->reg != NULL &&
              ((X86Register*)inst->operand[0]->reg)->type ==
                  kX86RegTypeInt) {
            fprintf(fp, "\tmovl ");
            PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          if ((int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
            fprintf(fp, "\tmovl ");
            PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
            fprintf(fp, ", ");
            if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
            fprintf(fp, "\n");
            return;
          }
          fprintf(fp, "\tmovd ");
          PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      if (InstResultRegTypeI386(inst) == kX86RegTypeFloat &&
          inst->operand[0] != NULL) {
        int64_t bits = 0;
        if (I386ConstBits(inst->operand[0], &bits)) {
          const char* xmm = (inst->dest != NULL && inst->dest->reg != NULL)
                                ? X86RegisterName((X86Register*)inst->dest->reg,
                                                  buf2, sizeof(buf2))
                                : GetRegisterName(inst, buf2, sizeof(buf2));
          bool is_double = (X86Opcode)opcode == X86_OP(movsd) ||
                           (X86Opcode)opcode == X86_OP(movq_xmm) ||
                           (X86Opcode)opcode == X86_OP(fmv_d) ||
                           (X86Opcode)opcode == X86_OP(movd);
          EmitI386ImmToXmm(fp, bits, is_double, xmm);
          return;
        }
        if ((inst->operand[0]->reg != NULL &&
             ((X86Register*)inst->operand[0]->reg)->type ==
                 kX86RegTypeInt) ||
            (int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
          fprintf(fp, "\tmovd ");
          PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
          fprintf(fp, ", ");
          if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
          fprintf(fp, "\n");
          return;
        }
      }
      const char* mov = AttMnemonicI386(opcode);
      if (strcmp(mov, "movd") == 0 && inst->operand[0] != NULL &&
          inst->operand[0]->reg != NULL &&
          ((X86Register*)inst->operand[0]->reg)->type == kX86RegTypeFloat) {
        mov = "movsd";
      }
      fprintf(fp, "\t%s ", mov);
      PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      return;
    }

    case X86_OP(movslq):
      PrintMovToDestIfNeededI386(fp, inst, buf1, buf2);
      return;

    default: {
      fprintf(fp, "\t%s ", AttMnemonicI386(opcode));
      if (inst->reg != NULL) {
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
      }
      for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
        if (inst->operand[i] != NULL) {
          fprintf(fp, ", ");
          PrintAttOperandI386(fp, inst->operand[i], buf1, sizeof(buf1));
        }
      }
      fprintf(fp, "\n");
      return;
    }
  }
}

static void PrintDestMoveI386(TargetInstruction* inst, FILE* fp) {
  TargetInstruction* src = inst->operand[0];
  assert(src != NULL);
  TargetRegister* dest_reg = inst->reg;
  if (dest_reg == NULL && inst->dest != NULL) {
    dest_reg = inst->dest->reg;
  }
  if (dest_reg == NULL) {
    return;
  }
  if (TargetIsConst(src) || (X86Opcode)src->opcode == X86_OP(x0)) {
    char buf[8];
    int64_t bits =
        TargetIsConst(src) ? TargetIntValue(src) : 0;
    if (((X86Register*)dest_reg)->type == kX86RegTypeFloat) {
      EmitI386ImmToXmm(fp, bits, true,
                       X86RegisterName((X86Register*)dest_reg, buf,
                                       sizeof(buf)));
      return;
    }
    fprintf(fp, "\tmovl ");
    PrintAsmImmediate(fp, (int64_t)(int32_t)bits);
    fprintf(fp, ", ");
    PrintPercentRegI386(fp,
                    X86RegisterName((X86Register*)dest_reg, buf,
                                       sizeof(buf)));
    fprintf(fp, "\n");
    return;
  }
  if (src->block == NULL) {
    return;
  }
  if (src->reg == NULL) {
    return;
  }
  if (dest_reg == src->reg) {
    return;
  }

  char buf1[8], buf2[8];
  const char* mov = MoveMnemonicForInstI386(src, inst->dest);
  fprintf(fp, "\t%s ", mov);
  PrintAttOperandI386(fp, src, buf1, sizeof(buf1));
  fprintf(fp, ", ");
  PrintPercentRegI386(fp,
                  X86RegisterName((X86Register*)dest_reg, buf2,
                                     sizeof(buf2)));
  fprintf(fp, "\n");
}

static void PrintRmovI386(X86Emitter* emitter, TargetInstruction* inst, FILE* fp) {
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
  const char* mov = MoveMnemonicForInstI386(inst->operand[1], inst->operand[0]);
  fprintf(fp, "\t%s ", mov);
  PrintPercentRegI386(fp,
                  X86RegisterName((X86Register*)inst->operand[1]->reg, buf1,
                                     sizeof(buf1)));
  fprintf(fp, ", ");
  PrintPercentRegI386(fp,
                  X86RegisterName((X86Register*)inst->operand[0]->reg, buf2,
                                     sizeof(buf2)));
  fprintf(fp, "\n");
}

static void PrintAtomicCompareExchangeI386(TargetInstruction* inst,
                                       const char* func_name, FILE* fp) {
  int size_log2 =
      (inst->flags & X86_ATOMIC_SIZE_MASK) >> X86_ATOMIC_SIZE_SHIFT;
  static const char* cmpxchg[] = {
      "atomic_cmpxchgb", "atomic_cmpxchgw",
      "atomic_cmpxchgl", "atomic_cmpxchgq"};
  static const char* stores[] = {"movb", "movw", "movl", "movl"};
  char b0[16], b1[16], b2[16];

  fprintf(fp, "\t%s ", cmpxchg[size_log2]);
  PrintPercentRegFromInstI386(fp, inst->operand[1], b0, sizeof(b0));
  fprintf(fp, ", (");
  PrintPercentRegFromInstI386(fp, inst->operand[0], b1, sizeof(b1));
  fprintf(fp, ")\n");

  bool expected_is_pointer =
      inst->opcode == (TargetOpcode)X86_OP(atomic_compare_exchange_n);
  bool returns_bool =
      inst->opcode !=
      (TargetOpcode)X86_OP(atomic_compare_exchange_val);
  if (expected_is_pointer) {
    fprintf(fp, "\tje .L%s_atomic_cmpxchg_done_%d\n", func_name, inst->id);
    fprintf(fp, "\t%s %%eax, (", stores[size_log2]);
    PrintPercentRegFromInstI386(fp, inst->operand[2], b0, sizeof(b0));
    fprintf(fp, ")\n");
    fprintf(fp, ".L%s_atomic_cmpxchg_done_%d:\n", func_name, inst->id);
  }
  if (returns_bool) {
    // CMPXCHG returns the observed value in RAX.  Update *expected before SETE
    // overwrites AL with the boolean result; MOV preserves the comparison
    // flags, so SETE remains valid on both paths.
    fprintf(fp, "\tsete ");
    PrintResultRegFromInstI386(fp, inst, b2, sizeof(b2));
    fprintf(fp, "\n");
    fprintf(fp, "\tmovzbl ");
    PrintResultRegFromInstI386(fp, inst, b0, sizeof(b0));
    fprintf(fp, ", ");
    PrintResultRegFromInstI386(fp, inst, b1, sizeof(b1));
    fprintf(fp, "\n");
  } else {
    fprintf(fp, "\tmovl %%eax, ");
    PrintResultRegFromInstI386(fp, inst, b0, sizeof(b0));
    fprintf(fp, "\n");
  }
}

static void PrintAtomicFetchAddSubI386(TargetInstruction* inst, FILE* fp) {
  int size_log2 =
      (inst->flags & X86_ATOMIC_SIZE_MASK) >> X86_ATOMIC_SIZE_SHIFT;
  static const char* xadd[] = {
      "atomic_xaddb", "atomic_xaddw", "atomic_xaddl", "atomic_xaddl"};
  char b0[16], b1[16];

  fprintf(fp, "\t%s ", xadd[size_log2]);
  PrintPercentRegFromInstI386(fp, inst->operand[1], b0, sizeof(b0));
  fprintf(fp, ", (");
  PrintPercentRegFromInstI386(fp, inst->operand[0], b1, sizeof(b1));
  fprintf(fp, ")\n");
}

static void ReloadStructReturnRegisterAtLandingPad(X86Emitter* emitter,
                                                   FILE* fp);

// Main instruction printer.
static void PrintInstructionI386(X86Emitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    TargetBasicBlock* b = inst->block;
    fprintf(fp, "\n\t// *** Basic block %zd\n\n", b->block_id);
    emitter->current_block = inst->block;
  }
  if (((int)inst->opcode == (int)X86_OP(label))) {
    if ((inst->flags & X86_INST_TABLE_ENTRY) != 0) {
      fprintf(fp, "\t.p2align 3\n");
    }
    if ((inst->flags & X86_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    if ((inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
      RestoreStackPointerAtLandingPad(emitter, fp);
      ReloadStructReturnRegisterAtLandingPad(emitter, fp);
    }
    return;
  }

  if (((int)inst->opcode == (int)X86_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }

  if (((int)inst->opcode == (int)X86_OP(ivarreg)) || ((int)inst->opcode == (int)X86_OP(fvarreg))) {
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
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(atomic_compare_exchange_bool):
    case X86_OP(atomic_compare_exchange_val):
    case X86_OP(atomic_compare_exchange_n):
      PrintAtomicCompareExchangeI386(inst, func_name, fp);
      return;
    case X86_OP(atomic_fetch_add_sub):
      PrintAtomicFetchAddSubI386(inst, fp);
      return;
//    case X86_OP(rmov):
//    case X86_OP(rmovf):
//    case X86_OP(rmovd):
//      PrintRmovI386(emitter, inst, fp);
//      return;

    case X86_OP(mv):
      if (inst->dest != NULL) {
        PrintDestMoveI386(inst, fp);
        return;
      }
      // Don't emit mv x, x.
      if (inst->operand[0]->reg == inst->reg) {
        return;
      }
      if (inst->operand[1] != NULL) {
        PrintRmovI386(emitter, inst, fp);
        return;
      }
      // Single-operand value copy: inst->reg <- operand[0].
      // (The generic default emits operands reversed for AT&T syntax, so
      // handle this form explicitly here.)
      if (inst->reg != NULL) {
        int64_t bits = 0;
        if (((X86Register*)inst->reg)->type == kX86RegTypeFloat &&
            I386ConstBits(inst->operand[0], &bits)) {
          EmitI386ImmToXmm(fp, bits, true,
                           GetRegisterName(inst, buf2, sizeof(buf2)));
          return;
        }
        fprintf(fp, "\t%s ", MoveMnemonicForInstI386(inst->operand[0], inst));
        PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
      }
      return;
    case X86_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      char namebuf[256];
      const char* symname =
          TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n", symname);
      } else if (SymbolHasWeakBinding(sym->symbol)) {
        fprintf(fp, "\t.weak %s\n", symname);
      } else {
        fprintf(fp, "\t.global %s\n", symname);
      }
      return;
    }
    case X86_OP(call):
    case X86_OP(callf): {
      assert(((int)inst->operand[0]->opcode == (int)X86_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      char namebuf[256];
      const char* symname =
          TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
      // The integrated assembler selects PLT32 for calls in PIC mode.
      // Keeping "@plt" in the textual name creates a distinct undefined
      // symbol instead of a relocation suffix.
      fprintf(fp, "\t%-12s%s\n", "call", symname);
      return;
    }

    case X86_OP(rcall):
    case X86_OP(rcallf):
      fprintf(fp, "\tcall *");
      PrintPercentRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n");
      return;
    case X86_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case X86_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case X86_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t");
      if ((inst->flags & X86_INST_EXTENDED_ASM) != 0) {
        PrintExtendedAsmI386(fp, (X86AsmInstruction*)inst, lit->value.value);
      } else {
        fprintf(fp, "%s", lit->value.value);
      }
      fprintf(fp, "\n");
      lit->base.disabled = true;
      return;
    }

    case X86_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }

    case X86_OP(spill): {
      X86Register* reg = (X86Register*)inst->reg;
      int offset = (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      const char* mov = (inst->flags & X86_VECTOR_VALUE) != 0
                            ? "movdqu"
                            : reg->type == kX86RegTypeInt ? "movl" : "storesd";
      if (FitsMemoryDisplacement(-offset)) {
        fprintf(fp, "\t%s ", mov);
        PrintPercentRegI386(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, ", -%d(%%ebp)\t// Spilled @%d\n", offset,
                inst->operand[0]->id);
      } else {
        fprintf(fp, "\tmovl ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->spill_addr,
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, "\n\tmovl %%ebp, ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\tsubl ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->spill_addr,
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, ", ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\t%s ", mov);
        PrintPercentRegI386(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, ", (");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, ")\t// Spilled @%d\n", inst->operand[0]->id);
      }
      return;
    }

    case X86_OP(reload): {
      X86Register* reg = (X86Register*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      const char* mov = (spill->flags & X86_VECTOR_VALUE) != 0
                            ? "movdqu"
                            : reg->type == kX86RegTypeInt ? "movl" : "movsd";
      if (FitsMemoryDisplacement(-offset)) {
        fprintf(fp, "\t%s -%d(%%ebp), ", mov, offset);
        PrintPercentRegI386(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, "\t// Reloaded spilled @%d\n", spill->operand[0]->id);
      } else {
        fprintf(fp, "\tmovl ");
    PrintAsmImmediate(fp, offset);
    fprintf(fp, ", ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->spill_addr,
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, "\n\tmovl %%ebp, ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\tsubl ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->spill_addr,
                                                      kX86RegTypeInt, buf1,
                                                      sizeof(buf1)));
        fprintf(fp, ", ");
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "\n\t%s (", mov);
        PrintPercentRegI386(fp, X86RegisterNameFromNum(X86_P(emitter->rv)->int_temp_start_1,
                                                      kX86RegTypeInt, buf2,
                                                      sizeof(buf2)));
        fprintf(fp, "), ");
        PrintPercentRegI386(fp, X86RegisterName(reg, buf1, sizeof(buf1)));
        fprintf(fp, "\t// Reloaded spilled @%d\n", spill->operand[0]->id);
      }
      return;
    }
    default:
      break;
  }

  // Print operands for the remaining instruction forms.
  switch ((X86Opcode)inst->opcode) {
    case X86_OP(loadl):
    case X86_OP(loadl_z):
    case X86_OP(loadw):
    case X86_OP(loadb):
    case X86_OP(loadb_z):
    case X86_OP(loadw_z):
    case X86_OP(loadss):
    case X86_OP(loadq):
    case X86_OP(loadsd):
    case X86_OP(loadv): {
      const char* mov = "movl";
      switch ((X86Opcode)inst->opcode) {
        case X86_OP(loadl):
          // Signed 32-bit load must sign-extend into the full 64-bit register
          // (the backend operates on 64-bit registers); a plain movl would
          // zero-extend.
          mov = "movl";
          break;
        case X86_OP(loadl_z):
          mov = "movl";
          break;
        case X86_OP(loadss):
          mov = "movss";
          break;
        case X86_OP(loadsd):
          mov = "movsd";
          break;
        case X86_OP(loadq):
          mov = "movl";
          break;
        case X86_OP(loadv):
          mov = "movdqu";
          break;
        case X86_OP(loadb):
          // Signed byte load must sign-extend into the full register; a plain
          // movb leaves the upper bits of the (64-bit) destination unchanged.
          mov = "movsbl";
          break;
        case X86_OP(loadb_z):
          mov = "movzbl";
          break;
        case X86_OP(loadw):
          mov = "movswl";
          break;
        case X86_OP(loadw_z):
          mov = "movzwl";
          break;
        default:
          break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      if (((X86Opcode)inst->opcode == X86_OP(loadss) ||
           (X86Opcode)inst->opcode == X86_OP(loadsd)) &&
          InstResultRegTypeI386(inst) == kX86RegTypeInt) {
        mov = ((X86Opcode)inst->opcode == X86_OP(loadsd)) ? "movl" : "movl";
      }
      fprintf(fp, "\t%s ", mov);
      int64_t offset = 0;
      TargetInstruction* base = inst->operand[0];
      if (((int)inst->operand[0]->opcode == (int)X86_OP(symbol)) &&
          MemoryOffsetFromOperand(inst->operand[1], &offset) && offset == 0) {
        char namebuf[256];
        fprintf(fp, "%s, ",
                TargetSymbolName(((TargetSymbol*)inst->operand[0])->symbol,
                                 namebuf, sizeof(namebuf)));
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (MemoryBaseFromOffsetOperand(inst->operand[1], &base)) {
        MemoryOffsetFromOperand(inst->operand[1], &offset);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInstI386(fp, base, buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (MemoryOffsetFromOperand(inst->operand[1], &offset)) {
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (TargetIsConst(inst->operand[1])) {
        offset = TargetIntValue(inst->operand[1]);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_OP(symbol))) {
        char namebuf[256];
        fprintf(fp, "%s(", TargetSymbolName(
                               ((TargetSymbol*)inst->operand[1])->symbol,
                               namebuf, sizeof(namebuf)));
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_OP(label))) {
        assert((inst->flags & X86_PCREL_LO_RELOC) != 0);
        fprintf(fp, ".%s_label_%d(", func_name, inst->operand[1]->id);
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[1]->opcode == (int)X86_OP(x0))) {
        fprintf(fp, "0(");
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (PrintTlsTporffMemoryOperandI386(fp, inst->operand[0],
                                             inst->operand[1], buf2,
                                             sizeof(buf2))) {
        fprintf(fp, ", ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else {
        assert(false);
      }
      break;
    }

    case X86_OP(storel):
    case X86_OP(storew):
    case X86_OP(storeq):
    case X86_OP(storeb):
    case X86_OP(storess):
    case X86_OP(storesd):
    case X86_OP(storev): {
      const char* mov = "movl";
      switch ((X86Opcode)inst->opcode) {
        case X86_OP(storel):
          mov = "movl";
          break;
        case X86_OP(storess):
          mov = "storess";
          break;
        case X86_OP(storesd):
          mov = "storesd";
          break;
        case X86_OP(storev):
          mov = "movdqu";
          break;
        case X86_OP(storeb):
          mov = "movb";
          break;
        case X86_OP(storew):
          mov = "movw";
          break;
        default:
          break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      if (((X86Opcode)inst->opcode == X86_OP(storesd) ||
           (X86Opcode)inst->opcode == X86_OP(storess)) &&
          OperandHasIntRegI386(inst->operand[0])) {
        mov = ((X86Opcode)inst->opcode == X86_OP(storesd)) ? "movl" : "movl";
      }
      fprintf(fp, "\t%s ", mov);
      if (TargetIsConst(inst->operand[0])) {
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
      } else if ((int)inst->operand[0]->opcode == (int)X86_OP(x0)) {
        // The x0 pseudo-register is the constant 0; x86-64 has no hardware zero
        // register, so store an immediate 0 rather than emitting whatever
        // physical register x0 was mapped to (which is not guaranteed to hold
        // 0).
        fprintf(fp, "$0");
      } else {
        PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
      }
      fprintf(fp, ", ");
      int64_t offset = 0;
      TargetInstruction* base = inst->operand[1];
      if (MemoryBaseFromOffsetOperand(inst->operand[2], &base)) {
        MemoryOffsetFromOperand(inst->operand[2], &offset);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInstI386(fp, base, buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (MemoryOffsetFromOperand(inst->operand[2], &offset)) {
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (TargetIsConst(inst->operand[2])) {
        offset = TargetIntValue(inst->operand[2]);
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_OP(symbol))) {
        char namebuf[256];
        fprintf(fp, "%s(", TargetSymbolName(
                               ((TargetSymbol*)inst->operand[2])->symbol,
                               namebuf, sizeof(namebuf)));
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_OP(label))) {
        assert((inst->flags & X86_PCREL_LO_RELOC) != 0);
        fprintf(fp, ".%s_label_%d(", func_name, inst->operand[2]->id);
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else if (((int)inst->operand[2]->opcode == (int)X86_OP(x0))) {
        fprintf(fp, "0(");
        PrintMemoryBaseRegFromInstI386(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, ")\n");
      } else if (PrintTlsTporffMemoryOperandI386(fp, inst->operand[1],
                                             inst->operand[2], buf2,
                                             sizeof(buf2))) {
        fprintf(fp, "\n");
      } else {
        assert(false);
      }
      break;
    }

    case X86_OP(nop):
      if (!inst->observable_checkpoint) {
        fprintf(fp, (inst->flags & X86_MFENCE) != 0 ? "\tmfence\n"
                                                       : "\tnop\n");
      }
      break;

    case X86_OP(je):
    case X86_OP(jne):
    case X86_OP(jl):
    case X86_OP(jge):
    case X86_OP(jb):
    case X86_OP(jae):
      if (inst->operand[1] == NULL) {
        assert(inst->operand[0] != NULL);
        assert(((int)inst->operand[0]->opcode == (int)X86_OP(label)));
        fprintf(fp, "\t%s .%s_label_%d\n",
                BranchMnemonicI386((X86Opcode)inst->opcode),
                func_name, inst->operand[0]->id);
        break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[2] != NULL);
      if (BranchOperandIsImmediateI386(inst->operand[0]) &&
          !BranchOperandIsImmediateI386(inst->operand[1])) {
        // The destination of the cmp would be an immediate, which the
        // assembler cannot encode.  Compare in the opposite direction and
        // invert the branch condition.
        const char* mnemonic = SwappedBranchMnemonicI386((X86Opcode)inst->opcode);
        assert(mnemonic != NULL);
        fprintf(fp, "\tcmp ");
        PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        PrintAttOperandI386(fp, inst->operand[1], buf2, sizeof(buf2));
        fprintf(fp, "\n\t%s .%s_label_%d\n", mnemonic, func_name,
                inst->operand[2]->id);
        break;
      }
      fprintf(fp, "\tcmp ");
      PrintAttOperandI386(fp, inst->operand[1], buf2, sizeof(buf2));
      fprintf(fp, ", ");
      PrintAttOperandI386(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, "\n\t%s .%s_label_%d\n",
              BranchMnemonicI386((X86Opcode)inst->opcode),
              func_name, inst->operand[2]->id);
      break;

    case X86_OP(jz):
    case X86_OP(jnz):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[0]->reg != NULL);
      fprintf(fp, "\ttest ");
      PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", ");
      PrintPercentRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
      fprintf(fp, "\n\t%s .%s_label_%d\n",
              BranchMnemonicI386((X86Opcode)inst->opcode),
              func_name, inst->operand[1]->id);
      break;

    case X86_OP(jmp): {
      assert(inst->operand[0] != NULL);
      if ((inst->flags & X86_INST_TABLE_ENTRY) != 0) {
        fprintf(fp, "\t.p2align 3\n");
      }
      TargetInstruction* dest = inst->operand[0];
      if (((int)dest->opcode == (int)X86_OP(label))) {
        fprintf(fp, "\tjmp .%s_label_%d\n", func_name, dest->id);
      } else if (((int)dest->opcode == (int)X86_OP(symbol))) {
        char namebuf[256];
        fprintf(fp, "\tjmp %s\n",
                TargetSymbolName(((TargetSymbol*)dest)->symbol, namebuf,
                                 sizeof(namebuf)));
      } else {
        fprintf(fp, "\tjmp *");
        PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, "\n");
      }
      break;
    }

    case X86_OP(mov): {
      int64_t value = TargetIntValue(inst->operand[0]);
      fprintf(fp, "\tmovl ");
      PrintAsmImmediate(fp, value);
      fprintf(fp, ", ");
      PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(ret):
      if (inst->operand[0] != NULL && TargetIsConst(inst->operand[0])) {
        fprintf(fp, "\tret ");
        PrintAsmImmediate(fp, (int)TargetIntValue(inst->operand[0]));
        fprintf(fp, "\n");
      } else {
        fprintf(fp, "\tret\n");
      }
      break;

    case X86_OP(add):
    case X86_OP(addl):
    case X86_OP(adcl):
    case X86_OP(sub):
    case X86_OP(subl):
    case X86_OP(sbbl):
    case X86_OP(and):
    case X86_OP(or):
    case X86_OP(xor): {
      if (inst->operand[0] == NULL || inst->operand[1] == NULL) {
        break;
      }
      bool is_sub = (X86Opcode)inst->opcode == X86_OP(sub) ||
                    (X86Opcode)inst->opcode == X86_OP(subl) ||
                    (X86Opcode)inst->opcode == X86_OP(sbbl);
      const char* op = "addl";
      if ((X86Opcode)inst->opcode == X86_OP(addl)) {
        op = "addl";
      } else if ((X86Opcode)inst->opcode == X86_OP(adcl)) {
        op = "adcl";
      } else if ((X86Opcode)inst->opcode == X86_OP(sub)) {
        op = "subl";
      } else if ((X86Opcode)inst->opcode == X86_OP(subl)) {
        op = "subl";
      } else if ((X86Opcode)inst->opcode == X86_OP(sbbl)) {
        op = "sbbl";
      } else if ((X86Opcode)inst->opcode == X86_OP(and)) {
        op = "andl";
      } else if ((X86Opcode)inst->opcode == X86_OP(or)) {
        op = "orl";
      } else if ((X86Opcode)inst->opcode == X86_OP(xor)) {
        op = "xorl";
      }
      // These are two-address operations: the result is computed in place into
      // the destination register, which first receives a copy of operand[0].
      // The destination register is operand[0] unless an explicit dest is set.
      TargetRegister* dest_reg =
          (inst->dest != NULL && inst->dest->reg != NULL) ? inst->dest->reg
                                                          : inst->reg;
      TargetInstruction* src0 = inst->operand[0];
      TargetInstruction* src1 = inst->operand[1];
      bool src0_is_zero =
          (X86Opcode)src0->opcode == X86_OP(x0) ||
          (TargetIsConst(src0) && TargetIntValue(src0) == 0);
      bool src1_is_zero = (X86Opcode)src1->opcode == X86_OP(x0);
      bool src1_in_dest = !TargetIsConst(src1) && !src1_is_zero &&
                          SamePhysicalRegI386(src1->reg, dest_reg) &&
                          !SamePhysicalRegI386(src0->reg, dest_reg);
      if (src1_in_dest && !is_sub) {
        // Commutative op with the second source already in the destination
        // register: swap the sources so the "mov src0 -> dest" below does not
        // clobber src1 (which lives in dest).  e.g. add base, idx where idx is
        // already in dest becomes add idx, base-in-dest.
        TargetInstruction* tmp = src0;
        src0 = src1;
        src1 = tmp;
      } else if (src1_in_dest && is_sub) {
        // Non-commutative subtract with the subtrahend in the destination
        // register.  Compute dest = src0 - src1 as "neg dest ; add src0, dest"
        // to avoid clobbering src1.
        fprintf(fp, "\tnegl ");
        PrintPercentRegI386(fp, X86RegisterName((X86Register*)dest_reg, buf2,
                                               sizeof(buf2)));
        if (!src0_is_zero) {
          fprintf(fp, "\n\taddl ");
          if (TargetIsConst(src0)) {
            PrintAsmImmediate(fp, TargetIntValue(src0));
          } else {
            PrintPercentRegFromInstI386(fp, src0, buf1, sizeof(buf1));
          }
          fprintf(fp, ", ");
          PrintPercentRegI386(fp,
                          X86RegisterName((X86Register*)dest_reg, buf2,
                                             sizeof(buf2)));
        }
        if ((X86Opcode)inst->opcode == X86_OP(subl)) {
          // The assembler currently has only a 64-bit unary negate. Preserve
          // the zero-extension semantics that a 32-bit subtraction provides.
          fprintf(fp, "\n\tmovl ");
          PrintPercentRegI386(fp,
                          X86RegisterName((X86Register*)dest_reg, buf2,
                                             sizeof(buf2)));
          fprintf(fp, ", ");
          PrintPercentRegI386(fp,
                          X86RegisterName((X86Register*)dest_reg, buf2,
                                             sizeof(buf2)));
        }
        fprintf(fp, "\n");
        break;
      }
      // The swap above may have moved the zero pseudo-register into src1, so
      // ask again for both operands rather than reusing the answers from before
      // the swap. x0 has no physical register behind it; printing it as one
      // names %rax.
      src0_is_zero =
          (X86Opcode)src0->opcode == X86_OP(x0) ||
          (TargetIsConst(src0) && TargetIntValue(src0) == 0);
      src1_is_zero = (X86Opcode)src1->opcode == X86_OP(x0);
      if ((TargetIsConst(src0) || src0_is_zero) && dest_reg != NULL) {
        fprintf(fp, "\tmovl ");
        if (TargetIsConst(src0)) {
          PrintAsmImmediate(fp, TargetIntValue(src0));
        } else {
          fprintf(fp, "$0");
        }
        fprintf(fp, ", ");
        PrintPercentRegI386(fp, X86RegisterName((X86Register*)dest_reg, buf2,
                                               sizeof(buf2)));
        fprintf(fp, "\n");
      } else if (src0->reg != NULL && dest_reg != NULL &&
                 !SamePhysicalRegI386(src0->reg, dest_reg)) {
        fprintf(fp, "\tmovl ");
        PrintPercentRegFromInstI386(fp, src0, buf1, sizeof(buf1));
        fprintf(fp, ", ");
        PrintPercentRegI386(fp, X86RegisterName((X86Register*)dest_reg, buf2,
                                               sizeof(buf2)));
        fprintf(fp, "\n");
      }
      if (TargetIsConst(src1) || src1_is_zero) {
        fprintf(fp, "\t%s ", op);
        if (TargetIsConst(src1)) {
          PrintAsmImmediate(fp, TargetIntValue(src1));
        } else {
          fprintf(fp, "$0");
        }
        fprintf(fp, ", ");
        PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
        fprintf(fp, "\n");
      } else {
        fprintf(fp, "\t%s ", op);
        PrintPercentRegFromInstI386(fp, src1, buf1, sizeof(buf1));
        fprintf(fp, ", ");
        if (dest_reg != NULL) {
          PrintPercentRegI386(fp, X86RegisterName((X86Register*)dest_reg,
                                                 buf2, sizeof(buf2)));
        } else {
          PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        }
        fprintf(fp, "\n");
      }
      break;
    }

    case X86_OP(bsf):
    case X86_OP(bsr):
    case X86_OP(bsfl):
    case X86_OP(bsrl): {
      bool leading = (X86Opcode)inst->opcode == X86_OP(bsr) ||
                     (X86Opcode)inst->opcode == X86_OP(bsrl);
      fprintf(fp, "\tpushl %%ebx\n\tmovl ");
      PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
      fprintf(fp, ", %%ebx\n\t%s %%ebx, %%ebx\n",
              leading ? "bsrl" : "bsfl");
      if (leading) {
        fprintf(fp, "\txorl $31, %%ebx\n");
      }
      fprintf(fp, "\tmovl %%ebx, ");
      PrintResultRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      fprintf(fp, "\n\tpopl %%ebx\n");
      break;
    }

    case X86_OP(shl):
    case X86_OP(shr):
    case X86_OP(sar):
    case X86_OP(rol):
    case X86_OP(ror):
    case X86_OP(roll):
    case X86_OP(rorl):
    case X86_OP(shll):
    case X86_OP(shrl):
    case X86_OP(sarl): {
      const char* op = "shll";
      switch ((X86Opcode)inst->opcode) {
        case X86_OP(shr):
          op = "shrl";
          break;
        case X86_OP(sar):
          op = "sarl";
          break;
        case X86_OP(rol):
          op = "roll";
          break;
        case X86_OP(ror):
          op = "rorl";
          break;
        case X86_OP(roll):
          op = "roll";
          break;
        case X86_OP(rorl):
          op = "rorl";
          break;
        case X86_OP(shll):
          op = "shll";
          break;
        case X86_OP(shrl):
          op = "shrl";
          break;
        case X86_OP(sarl):
          op = "sarl";
          break;
        default:
          break;
      }
      bool constant_count = TargetIsConst(inst->operand[1]);
      if (!constant_count) {
        // Variable shifts require %cl. Use %ebx as scratch (callee-saved).
        // the result and save %rcx around the operation. This handles every
        // overlap between lhs, count, destination, and an unrelated live value
        // in %rcx without requiring allocator constraints.
        fprintf(fp, "\tpushl %%ebx\n\tmovl ");
        PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", %%ebx\n\tpushl %%ecx\n\tmovl ");
        PrintPercentRegFromInstI386(fp, inst->operand[1], buf1, sizeof(buf1));
        fprintf(fp, ", %%ecx\n");
        fprintf(fp, "\t%s %%cl, %%ebx\n\tpopl %%ecx\n\tmovl %%ebx, ", op);
        if (inst->dest != NULL && inst->dest->reg != NULL) {
          PrintPercentRegI386(
              fp, X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                     sizeof(buf2)));
        } else {
          PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        }
        fprintf(fp, "\n\tpopl %%ebx\n");
        break;
      }
      if ((X86Opcode)inst->opcode == X86_OP(roll) ||
          (X86Opcode)inst->opcode == X86_OP(rorl)) {
        fprintf(fp, "\tpushl %%ebx\n\tmovl ");
        PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", %%ebx\n\t%s ", op);
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[1]));
        fprintf(fp, ", %%ebx\n\tmovl %%ebx, ");
        PrintResultRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n\tpopl %%ebx\n");
        break;
      }
      if (inst->operand[0]->reg != NULL && inst->reg != NULL &&
          inst->operand[0]->reg != inst->reg) {
        fprintf(fp, "\tmovl ");
        PrintPercentRegFromInstI386(fp, inst->operand[0], buf1, sizeof(buf1));
        fprintf(fp, ", ");
        if (inst->dest != NULL && inst->dest->reg != NULL) {
          PrintPercentRegI386(
              fp, X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                     sizeof(buf2)));
        } else {
          PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        }
        fprintf(fp, "\n");
      }
      fprintf(fp, "\t%s ", op);
      PrintAsmImmediate(fp, TargetIntValue(inst->operand[1]));
      fprintf(fp, ", ");
      if (inst->dest != NULL && inst->dest->reg != NULL) {
        PrintPercentRegI386(fp,
                        X86RegisterName((X86Register*)inst->dest->reg, buf2,
                                           sizeof(buf2)));
      } else {
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
      }
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(lea):
    case X86_OP(lea_rip): {
      // lea_rip of a symbol or string literal needs only operand[0]; the RIP
      // base is implicit.  The register-form lea still requires operand[1].
      if (inst->operand[0] == NULL) {
        break;
      }
      if ((X86Opcode)inst->opcode == X86_OP(lea) &&
          inst->operand[1] == NULL) {
        break;
      }
      bool absolute_addr =
          ((X86Opcode)inst->opcode == X86_OP(lea_rip)) &&
          inst->operand[0] != NULL &&
          (((int)inst->operand[0]->opcode == (int)X86_OP(symbol)) ||
           ((int)inst->operand[0]->opcode == (int)X86_OP(literal)) ||
           ((int)inst->operand[0]->opcode == (int)X86_OP(label)) ||
           TargetIsConst(inst->operand[0]));
      if ((inst->flags & X86_GOTPCREL_RELOC) != 0 &&
          (((int)inst->operand[0]->opcode == (int)X86_OP(symbol)) ||
           ((int)inst->operand[0]->opcode == (int)X86_OP(literal)))) {
        char namebuf[256];
        const char* symname;
        if ((int)inst->operand[0]->opcode == (int)X86_OP(symbol)) {
          TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
          symname = TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
        } else {
          TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
          LiteralAsmName(literal->literal_id, namebuf, sizeof(namebuf));
          symname = namebuf;
        }
        fprintf(fp, "\tcall .L%s_pic_%d\n", func_name, inst->id);
        fprintf(fp, ".L%s_pic_%d:\n\tpopl ", func_name, inst->id);
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n\taddl _GLOBAL_OFFSET_TABLE_, ");
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n\tmovl %s@GOT(", symname);
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "), ");
        PrintPercentRegFromInstI386(fp, inst, buf2, sizeof(buf2));
        fprintf(fp, "\n");
        break;
      }
      if ((inst->flags & X86_GOTPCREL_RELOC) != 0) {
        fprintf(fp, "\tmovl ");
      } else if (absolute_addr) {
        fprintf(fp, "\tmovl ");
      } else {
        fprintf(fp, "\tlea ");
      }
      if (((int)inst->operand[0]->opcode == (int)X86_OP(symbol))) {
        TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
        // Use TargetSymbolName so function-local statics get the same mangled
        // name (.local.<name>.<id>) used by their data definition; the raw
        // symbol name would collide with a same-named file-scope global.
        char namebuf[256];
        const char* symname =
            TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
        if ((inst->flags & X86_TLSGD_RELOC) != 0 ||
            (inst->flags & X86_GOTPCREL_RELOC) != 0) {
          fprintf(fp, "/* PIC unsupported on i386 */ %s, ", symname);
        } else if (((int)inst->opcode == (int)X86_OP(lea_rip))) {
          fprintf(fp, "%s, ", symname);
        } else {
          fprintf(fp, "%s(", symname);
          PrintPercentRegFromInstI386(fp, inst->operand[1], buf2, sizeof(buf2));
          fprintf(fp, "), ");
        }
      } else if (((int)inst->operand[0]->opcode == (int)X86_OP(literal))) {
        TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
        char litname[64];
        LiteralAsmName(literal->literal_id, litname, sizeof(litname));
        fprintf(fp, "%s, ", litname);
      } else if (((int)inst->opcode == (int)X86_OP(lea_rip)) &&
                 ((int)inst->operand[0]->opcode == (int)X86_OP(label))) {
        fprintf(fp, ".%s_label_%d, ", func_name, inst->operand[0]->id);
      } else if (((int)inst->opcode == (int)X86_OP(lea_rip)) &&
                 TargetIsConst(inst->operand[0])) {
        // PC-relative lea with an immediate displacement (used to materialize
        // the current instruction pointer for computed branches).
        fprintf(fp, "%" PRId64 ", ", TargetIntValue(inst->operand[0]));
      } else if (inst->operand[1] != NULL && TargetIsConst(inst->operand[1])) {
        fprintf(fp, "%" PRId64 "(", TargetIntValue(inst->operand[1]));
        PrintPercentRegFromInstI386(fp, inst->operand[0], buf2, sizeof(buf2));
        fprintf(fp, "), ");
      } else if (inst->operand[0] != NULL && inst->operand[1] != NULL) {
        int64_t offset = 0;
        TargetInstruction* base = inst->operand[0];
        if (MemoryBaseFromOffsetOperand(inst->operand[1], &base)) {
          MemoryOffsetFromOperand(inst->operand[1], &offset);
        } else if (MemoryOffsetFromOperand(inst->operand[1], &offset)) {
          base = inst->operand[0];
        } else {
          PrintAttOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
          fprintf(fp, "(");
          PrintAttOperandI386(fp, inst->operand[0], buf2, sizeof(buf2));
          fprintf(fp, "), ");
          PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
          fprintf(fp, "\n");
          break;
        }
        fprintf(fp, "%" PRId64 "(", offset);
        PrintMemoryBaseRegFromInstI386(fp, base, buf2, sizeof(buf2));
        fprintf(fp, "), ");
      } else {
        if (inst->operand[0] != NULL && inst->operand[1] != NULL) {
          PrintAttOperandI386(fp, inst->operand[1], buf1, sizeof(buf1));
          fprintf(fp, "(");
          PrintAttOperandI386(fp, inst->operand[0], buf2, sizeof(buf2));
          fprintf(fp, "), ");
        } else {
          assert(false);
        }
      }
      PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(movabs): {
      fprintf(fp, "\tmovl ");
      PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
      fprintf(fp, ", ");
      PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(movxc): {
      fprintf(fp, "\tmovl ");
      if (inst->operand[0] != NULL &&
          (int)inst->operand[0]->opcode == (int)X86_OP(symbol)) {
        TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
        char namebuf[256];
        const char* symname =
            TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf));
        if ((inst->flags & X86_TLS_RELOC) != 0) {
          fprintf(fp, "%s@TPOFF, ", symname);
        } else {
          fprintf(fp, "%s, ", symname);
        }
      } else {
        PrintAsmImmediate(fp, TargetIntValue(inst->operand[0]));
        fprintf(fp, ", ");
      }
      PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    case X86_OP(tp): {
      fprintf(fp, "\tmovl %%fs:0, ");
      PrintPercentRegFromInstI386(fp, inst, buf1, sizeof(buf1));
      fprintf(fp, "\n");
      break;
    }

    default:
      PrintDefaultInstructionI386(fp, inst, buf1, buf2);
      break;
  }

}

static int X86DwarfRegisterI386(int reg) {
  // Logical GPR slots map 1:1 to i386 DWARF register numbers (EAX=0..EDI=7).
  static const int dwarf_regs[8] = {
      0, 1, 2, 3, 4, 5, 6, 7,
  };
  if (reg >= 0 && reg < 8) {
    return dwarf_regs[reg];
  }
  return reg;
}

static size_t X86SavedCFIRegistersI386(
    X86Emitter* emitter, DaveEHFrameSavedReg* saved_regs,
    size_t capacity) {
  BitSetIterator it;
  size_t count = 0;
  int offset = emitter->saved_reg_offset;
  int stack_frame_size = StackFrameSize(emitter);

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    if (count < capacity) {
      saved_regs[count].dwarf_reg = X86DwarfRegisterI386(reg);
      saved_regs[count].cfa_offset = offset - stack_frame_size;
      count++;
    }
    offset -= 8;
    BitSetIteratorNext(&it);
  }
  return count;
}

static void X86PrintEHFrameI386(X86Emitter* emitter, FILE* fp,
                               const char* func_name) {
  if (emitter->rv->base.varargs ||
      emitter->rv->exception_ranges.length == 0) {
    return;
  }

  DaveEHLSDARange lsda_ranges[64];
  DaveEHFrameSavedReg saved_regs[16];
  size_t lsda_count = 0;
  size_t saved_reg_count =
      X86SavedCFIRegistersI386(emitter, saved_regs,
                             sizeof(saved_regs) / sizeof(saved_regs[0]));
  for (size_t i = 0; i < emitter->rv->exception_ranges.length &&
                     lsda_count < sizeof(lsda_ranges) / sizeof(lsda_ranges[0]);
       i++) {
    X86ExceptionRange* range = emitter->rv->exception_ranges.value.p[i];
    DaveEHLSDARange* out = &lsda_ranges[lsda_count++];
    out->try_start_id = range->try_start->id;
    out->try_end_id = range->try_end->id;
    out->landing_pad_id = range->catch_label->id;
    out->is_cleanup = range->is_cleanup;
    out->catch_typeinfo =
        range->is_cleanup ? NULL : LSDATypeInfoSymbol(range->catch_typeinfo);
  }

  DaveEHFrameEmitInfo info = {
      .ranges = lsda_ranges,
      .range_count = lsda_count,
      .func_name = func_name,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = false,
      .cie_ra_reg = 16,
      .cie_cfa_reg = 4,
      .cie_fp_reg = 5,
      .entry_cfa_offset = 4,
      .frame_cfa_offset = 8,
      .fp_cfa_offset = 0,
      .saved_fp_offset = 0,
      .saved_ra_offset = 4,
      .saved_regs = saved_regs,
      .saved_reg_count = saved_reg_count,
  };
  DaveEHPrintEHFrameCIE(fp, &info, "");
  DaveEHPrintEHFrameFDE(fp, &info, "");
}

static void X86PrintGCCExceptTableI386(X86Emitter* emitter, FILE* fp,
                                      const char* func_name) {
  if (emitter->rv->exception_ranges.length == 0) {
    return;
  }
  DaveEHLSDARange lsda_ranges[64];
  size_t lsda_count = 0;
  for (size_t i = 0; i < emitter->rv->exception_ranges.length &&
                     lsda_count < sizeof(lsda_ranges) / sizeof(lsda_ranges[0]);
       i++) {
    X86ExceptionRange* range = emitter->rv->exception_ranges.value.p[i];
    DaveEHLSDARange* out = &lsda_ranges[lsda_count++];
    out->try_start_id = range->try_start->id;
    out->try_end_id = range->try_end->id;
    out->landing_pad_id = range->catch_label->id;
    out->is_cleanup = range->is_cleanup;
    out->catch_typeinfo =
        range->is_cleanup ? NULL : LSDATypeInfoSymbol(range->catch_typeinfo);
  }
  DaveEHFrameEmitInfo info = {
      .ranges = lsda_ranges,
      .range_count = lsda_count,
      .func_name = func_name,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = false,
      .cie_ra_reg = 16,
      .cie_cfa_reg = 4,
      .cie_fp_reg = 5,
      .entry_cfa_offset = 4,
      .frame_cfa_offset = 8,
      .fp_cfa_offset = 0,
      .saved_fp_offset = 0,
      .saved_ra_offset = 4,
  };
  DaveEHPrintGCCExceptTable(fp, &info);
}

// The unwinder resumes a frame at its landing pad with the stack pointer the
// throwing call site left it at.  Outgoing arguments are pushed, so that is
// below the bottom of the frame, whereas the rest of the function assumes the
// stack pointer is at the bottom: the exit sequence in particular reads the
// callee-saved registers at fixed offsets from it, and would hand the caller
// the outgoing arguments of the call that threw instead of its own registers.
// The frame pointer is restored by the unwinder from the CFI, so put the stack
// pointer back from that.  This is the inverse of the prologue, which lowers
// rsp to rbp + space_above_frame_pointer - stack_frame_size.
static void RestoreStackPointerAtLandingPadI386(X86Emitter* emitter, FILE* fp) {
  if (EmptyStackFrame(emitter)) {
    return;
  }
  fprintf(fp, "\tmovl %%ebp, %%esp\n");
  DecrementStackPointer(emitter, StackFrameSize(emitter), fp);
}

static void ReloadStructReturnRegisterAtLandingPadI386(X86Emitter* emitter,
                                                   FILE* fp) {
  if (emitter->rv->struct_return_reg < 0) {
    return;
  }

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
                 !emitter->rv->not_leaf;
  int struct_return_slot = (is_leaf ? X86_P(emitter->rv)->first_leaf_int_reg_var
                                    : X86_P(emitter->rv)->first_int_reg_var) +
                           emitter->rv->struct_return_reg;
  char buf[8];
  fprintf(fp, "\tmovl %d(%%ebp), ", emitter->rv->struct_return_spill_offset);
  PrintPercentReg(fp, X86RegisterNameFromNum(
                          struct_return_slot, kX86RegTypeInt,
                          buf, sizeof(buf)));
  fprintf(fp, "\n");
}

// END I386_EMITTER_EXTRACTED

void X86PrintFunction(X86Emitter* emitter, FILE* fp) {
  X86SetRegisterNameProfile(X86_P(emitter->rv));
  const char* func_name = emitter->rv->base.function_name.value;
  EmitFunctionSection(fp, func_name);
  if (emitter->rv->base.is_weak) {
    fprintf(fp, "\t.weak %s\n", func_name);
  } else if (emitter->rv->base.is_global) {
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
  X86PrintGCCExceptTable(emitter, fp, func_name);
  X86PrintEHFrame(emitter, fp, func_name);
}

void X86PrintCXXAdjustorThunks(FILE* fp) {
  if (compiler->cxx_this_adjustor_thunks.length == 0) {
    return;
  }
  fprintf(fp, "\t.text\n");
  for (size_t i = 0; i < compiler->cxx_this_adjustor_thunks.length; i++) {
    CXXThisAdjustorThunk* thunk = compiler->cxx_this_adjustor_thunks.value.p[i];
    if (thunk == NULL || thunk->thunk == NULL || thunk->target == NULL) {
      continue;
    }
    char thunk_buf[256];
    char target_buf[256];
    const char* thunk_name =
        TargetSymbolName(thunk->thunk, thunk_buf, sizeof(thunk_buf));
    const char* target_name =
        TargetSymbolName(thunk->target, target_buf, sizeof(target_buf));
    EmitFunctionSection(fp, thunk_name);
    fprintf(fp, "\t.weak  %s\n", thunk_name);
    fprintf(fp, "\t.type %s, @function\n\n", thunk_name);
    fprintf(fp, "%s:\n", thunk_name);
    if (thunk->this_adjustment != 0) {
      const X86Profile* tp = compiler->target != NULL
                                 ? X86ProfileFromTargetName(compiler->target->name.value)
                                 : NULL;
      if (tp != NULL && !tp->is_64bit) {
        fprintf(fp, "\taddl        $%d, %%edi\n", thunk->this_adjustment);
      } else {
        fprintf(fp, "\taddq        $%d, %%rdi\n", thunk->this_adjustment);
      }
    }
    fprintf(fp, "\tjmp         %s\n", target_name);
    fprintf(fp, ".func_end_%s:\n", thunk_name);
    fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", thunk_name, thunk_name,
            thunk_name);
  }
}
