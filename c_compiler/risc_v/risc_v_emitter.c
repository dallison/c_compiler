//
//  risc_v_emitter.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

// This is the RISC-V assembly language emitter. It prints the RISC-V
// instructions to the given file in assembly language.  The RVAssember
// reads this file and generates the binary.

#include "risc_v_emitter.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "common_emitter.h"
#include "compiler.h"
#include "eh_metadata.h"
#include "risc_v_assembler.h"
#include "risc_v_codegen.h"
#include "risc_v_reg_alloc.h"
#include "target_basic_block.h"

// Is the given instruction printable?  Some instructions do not
// produce any output as they are used for information for other
// instructions.
static bool IsPrintable(TargetInstruction* inst) {
  // Constants are encoded in the instructions that use them.
  if (TargetIsConst(inst)) {
    return false;
  }
  // These opcodes are not printable.
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(tmp):
    case RV_OP(fp):
    case RV_OP(sp):
    case RV_OP(literal):
    case RV_OP(structreturn):
    case RV_OP(resulti):
    case RV_OP(resultf):
    case RV_OP(resultd):
    case RV_OP(a0):
    case RV_OP(a1):
    case RV_OP(a2):
    case RV_OP(a3):
    case RV_OP(a4):
    case RV_OP(a5):
    case RV_OP(a6):
    case RV_OP(a7):
    case RV_OP(fa0):
    case RV_OP(fa1):
    case RV_OP(fa2):
    case RV_OP(fa3):
    case RV_OP(fa4):
    case RV_OP(fa5):
    case RV_OP(fa6):
    case RV_OP(fa7):
    case RV_OP(regarg):
    case RV_OP(ivarreg):
    case RV_OP(fvarreg):
    case RV_OP(x0):
    case RV_OP(t0):
    case RV_OP(t1):
    case RV_OP(t2):
    case RV_OP(nrvoval):
      return false;
    default:
      break;
  }
  return true;
}

static const char* SymbolName(TargetSymbol* sym, char* buf, size_t len) {
  return TargetSymbolName(sym->symbol, buf, len);
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
// +------------------------------+  <-- current frame pointer (s0)
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
// the frame pointer (s0).
//
// Spilled register values are a negative offset from s0.
// The only potentially large area is the space for
// local variables.  The rest are small and bounded.

// This is the size of the stack frame including the space
// for the local variables.
static bool HasExceptionStructReturn(RVEmitter* emitter) {
  return emitter->rv->struct_return_reg >= 0 &&
         emitter->rv->exception_ranges.length > 0;
}

static int StackFrameSize(RVEmitter* emitter) {
  // Start off with local variable space.  This also includes
  // 16 bytes for the saved ra and s0.
  int stack_frame_size = emitter->rv->base.stack_frame_size + 16;

  bool varargs = emitter->rv->base.varargs;

  if (varargs) {
    if (emitter->rv->num_int_arg_regs < RV_NUM_INT_ARGS) {
      // All args other than those declared and in registers must
      // be saved to the stack above the frame pointer and directly
      // under the first pushed arg.  This adds to the stack frame size.
      stack_frame_size += (RV_NUM_INT_ARGS - emitter->rv->num_int_arg_regs) * 8;
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
  stack_frame_size += BitSetCount(&emitter->regs->used_vector_regs) * RV_VLEN_BYTES;
  stack_frame_size += emitter->spill_region_size;
  if (HasExceptionStructReturn(emitter)) {
    stack_frame_size += 8;
  }
  
  stack_frame_size = (stack_frame_size + 15) & ~15;  // Aligned to 16 bytes.

  return stack_frame_size;
}

static bool EmptyStackFrame(RVEmitter* emitter) {
  return emitter->rv->base.stack_frame_size == 0 &&
         emitter->rv->base.num_calls == 0 && !emitter->rv->not_leaf &&
         emitter->rv->saved_regs.length == 0 &&
         BitSetCount(&emitter->regs->used_int_regs) == 0 &&
         BitSetCount(&emitter->regs->used_float_regs) == 0 &&
         BitSetCount(&emitter->regs->used_vector_regs) == 0 &&
         emitter->spill_region_size == 0;
}

static void DecrementStackPointer(RVEmitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size > 0x7ff) {
    // Too big for an immediate.  Load into t0 and use a sub instruction.
    fprintf(fp, "\tli t0, %d\n", stack_frame_size);
    fprintf(fp, "\tsub sp, sp, t0\n");
  } else {
    fprintf(fp, "\taddi sp, sp, -%d\n", stack_frame_size);
  }
}

static COMPILER_UNUSED void IncrementStackPointer(RVEmitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size > 0x7ff) {
    // Too big for an immediate.  Load into t0 and use an add instruction.
    fprintf(fp, "\tli t0, %d\n", stack_frame_size);
    fprintf(fp, "\tadd sp, sp, t0\n");
  } else {
    fprintf(fp, "\taddi sp, sp, %d\n", stack_frame_size);
  }
}

// Load a floating point or integer register from the stack frame.
static COMPILER_UNUSED void LoadRegisterFromFrame(RVEmitter* emitter, int reg,
                                  int offset, bool is_fp,
                                  const char* symbol_name,
                                  FILE* fp) {
  char buf[256];
  const char* instruction = is_fp ? "fld" : "ld";
  RVRegisterType reg_type = is_fp ? kRVRegTypeFloat : kRVRegTypeInt;
  if (offset < 0x7ff) {
    fprintf(fp, "\tf%s %s, -%d(s0)",
            instruction,
            RVRegisterNameFromNum(reg,
                                  reg_type, buf, sizeof(buf)),
            offset);
  } else {
    fprintf(fp, "\tli t0, %d\n", offset);
    fprintf(fp, "\taddi t0, t0, s0\n");
    fprintf(fp, "\t%s %s, 0(t0)", instruction,
    RVRegisterNameFromNum(reg,
                          reg_type, buf, sizeof(buf)));
  }
  fprintf(fp, "\t\t// %s\n", symbol_name);
}

static COMPILER_UNUSED void GenerateOffsetFromFrame(RVEmitter* emitter, int reg,
                                    int offset,
                                    const char* symbol_name,
                                    FILE* fp) {
  char buf[256];
  if (offset < 0x7ff) {
    fprintf(fp, "\taddi %s, s0, -%d",
            RVRegisterNameFromNum(reg,
                                  kRVRegTypeInt, buf, sizeof(buf)),
            offset);
  } else {
    fprintf(fp, "\tli t0, %d\n", offset);
    fprintf(fp, "\tadd %s, s0, t0",
            RVRegisterNameFromNum(reg,
                          kRVRegTypeInt, buf, sizeof(buf)));
  }
  fprintf(fp, "\t\t// %s\n", symbol_name);
}

// Save all used registers on the stack.
static void SaveRegisters(RVEmitter* emitter, FILE* fp) {
  // Entry sequence:
  // LET S = stack frame size + 8 (for frame pointer save)
  // addi sp, sp, -S   - decrement sp for stack frame
  // sd ra, S-8(sp)    - store return address
  // sd s0, S-16(sp)    - store frame pointer (s0)
  // addi s0, sp, S    - new frame pointer is original sp.

  int stack_frame_size = StackFrameSize(emitter);

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
          !emitter->rv->not_leaf;
  bool varargs = emitter->rv->base.varargs;
  int space_above_frame_pointer =
      varargs ? (RV_NUM_INT_ARGS - emitter->rv->num_int_arg_regs) * 8 : 0;

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
  int return_address_offset = stack_frame_size - 8 - space_above_frame_pointer;
  int frame_pointer_offset = stack_frame_size - 16 - space_above_frame_pointer;

  // Local vars are referenced as a negative offset from s0 and are immediately
  // below the saved argument registers.  This is the low address of the
  // start of the local variable region on the stack.
  int local_vars = emitter->rv->base.stack_frame_size +
                     RV_STACK_FRAME_HEADER_SIZE;

  // Offset from sp of first saved register.
  int saved_reg_offset = stack_frame_size - RV_STACK_FRAME_HEADER_SIZE - 8 -
                          emitter->rv->base.stack_frame_size -
                          space_above_frame_pointer -
                          emitter->spill_region_size;  // First saved register.

  // A leaf procedure doesn't save the return address, so the frame pointer
  // moves up into its slot.  The saved-register area stays where it is: the
  // freed word is at the *top* of the header, and the local variable and
  // argument-home offsets are all measured from a full-size header, so
  // shifting the saved registers up would put the first of them on top of the
  // lowest local.
  if (is_leaf) {
    frame_pointer_offset += 8;
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame, no need to store frame pointer.
  } else {
    if (stack_frame_size > 0x7ff) {
      // Stack frame is too big for a single store, decrement
      // sp by 16 and store ra and s0
      DecrementStackPointer(emitter, 16, fp);
      if (!is_leaf) {
        fprintf(fp, "\tsd ra, 8(sp)\n");
      }
      fprintf(fp, "\tsd s0, 0(sp)\n");
      DecrementStackPointer(emitter, stack_frame_size - 16, fp);
    } else {
      DecrementStackPointer(emitter, stack_frame_size, fp);
        fprintf(fp, "\t// Saved return address (offset %d) and "
                "frame pointer (offset %d)\n",
                return_address_offset, frame_pointer_offset);
      if (!is_leaf) {
        fprintf(fp, "\tsd ra, %d(sp)\n", return_address_offset);
      }
      fprintf(fp, "\tsd s0, %d(sp)\n", frame_pointer_offset);
    }
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_push:\n",
              emitter->rv->base.function_name.value);
    }
    
    // Set new frame pointer to original top of stack.
    if (stack_frame_size > 0x7ff) {
      // Large stack frame: t0 still contains stack frame size.
      if (space_above_frame_pointer > 0) {
        fprintf(fp, "\taddi t0, t0, -%d\n", space_above_frame_pointer);
      }
      fprintf(fp, "\taddi t0, t0, 16\n");     // t0 is stack_frame_size - 16.
      fprintf(fp, "\tadd s0, sp, t0\n");
    } else {
      fprintf(fp, "\taddi s0, sp, %d\n",
              stack_frame_size - space_above_frame_pointer);
    }
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_leaq:\n", emitter->rv->base.function_name.value);
    }

    if (varargs) {
      int num_pushed_arg_regs = RV_NUM_INT_ARGS - emitter->rv->num_int_arg_regs;
      int offset_from_frame_pointer = 0;
      fprintf(fp, "\t// varargs function with %d declared args\n",
              emitter->rv->num_int_arg_regs);
      for (int i = RV_NUM_INT_ARGS - num_pushed_arg_regs; i < RV_NUM_INT_ARGS;
           i++) {
        fprintf(fp, "\tsd a%d, %d(s0)\n", i, offset_from_frame_pointer);
        offset_from_frame_pointer += 8;
      }
    }
  }


  char buf1[8], buf2[8];

  if (emitter->rv->saved_regs.length > 0) {
    fprintf(fp, "\t// Saved argument registers.\n");
  }
  for (size_t i = 0; i < emitter->rv->saved_regs.length; i++) {
    SavedArgumentRegister* saved_reg = emitter->rv->saved_regs.value.p[i];
    int offset = saved_reg->offset;
    const char* store = saved_reg->is_fp
                            ? (saved_reg->size == 4 ? "fsw" : "fsd")
                            : "sd";
    fprintf(fp, "\t%s %s, %d(%s)\n",
            store,
            RVRegisterNameFromNum(saved_reg->reg_num,
                                  saved_reg->is_fp ? kRVRegTypeFloat
                                                   : kRVRegTypeInt,
                                  buf1, sizeof(buf1)),
            offset,
            RVRegisterNameFromNum(saved_reg->base_reg_num, kRVRegTypeInt,
                                  buf2, sizeof(buf2)));
  }
  
  if (!is_leaf) {
    fprintf(fp, "\t// Local vars at offset -%d(s0)\n", local_vars);
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
     fprintf(fp, "\t// Spilled register region: %d bytes at -%d(s0) to -%d(s0)\n",
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
    fprintf(fp, "\tsd %s, %d(sp)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeInt, buf1, sizeof(buf1)),
            offset);
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
    fprintf(fp, "\tfsd %s, %d(sp)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeFloat, buf1, sizeof(buf1)),
            offset);
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_vector_regs);
  if (!BitSetIteratorDone(&it)) {
    fprintf(fp, "\t// Saved vector registers.\n");
  }
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    int offset = saved_reg_offset;
    saved_reg_offset -= RV_VLEN_BYTES;
    if (offset >= -2048 && offset <= 2047) {
      fprintf(fp, "\taddi t0, sp, %d\n", offset);
    } else {
      fprintf(fp, "\tli t0, %d\n", offset);
      fprintf(fp, "\tadd t0, sp, t0\n");
    }
    fprintf(fp, "\tvsetivli    zero, 16, e8, m1, ta, ma\n");
    fprintf(fp, "\tvse8.v %s, (t0)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeVector, buf1, sizeof(buf1)));
    BitSetIteratorNext(&it);
  }

  if (HasExceptionStructReturn(emitter)) {
    fprintf(fp, "\tsd a0, %d(sp)\t// hidden result pointer\n",
            saved_reg_offset);
  }

  if (!is_leaf) {
    fprintf(fp, "\t// End of stack frame\n");
  }
}

static void RestoreRegisters(RVEmitter* emitter, FILE* fp) {
  // Exit sequence:
  // ld s0, S-8(sp)   - restore frame pointer.
  // addi sp, sp, S   - increment sp

  char buf1[8];

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
          !emitter->rv->not_leaf;
  bool varargs = emitter->rv->base.varargs;
  int space_above_frame_pointer =
      varargs ? (RV_NUM_INT_ARGS - emitter->rv->num_int_arg_regs) * 8 : 0;

  if (space_above_frame_pointer < 0) {
    space_above_frame_pointer = 0;
  }
  // int return_address_offset = stack_frame_size - 8 - space_above_frame_pointer;

  // A leaf procedure doesn't save the return address.
  if (!is_leaf) {
    fprintf(fp, "\t// Restored registers.\n");
  }

  int offset = emitter->saved_reg_offset;

  BitSetIterator it;

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tld %s, %d(sp)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeInt, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tfld %s, %d(sp)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeFloat, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_vector_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    if (offset >= -2048 && offset <= 2047) {
      fprintf(fp, "\taddi t0, sp, %d\n", offset);
    } else {
      fprintf(fp, "\tli t0, %d\n", offset);
      fprintf(fp, "\tadd t0, sp, t0\n");
    }
    fprintf(fp, "\tvsetivli    zero, 16, e8, m1, ta, ma\n");
    fprintf(fp, "\tvle8.v %s, (t0)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeVector, buf1, sizeof(buf1)));
    offset -= RV_VLEN_BYTES;
    BitSetIteratorNext(&it);
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame.
  } else {
    fprintf(fp, "\taddi sp, s0, %d\n", space_above_frame_pointer);
    if (!is_leaf) {
      fprintf(fp, "\tld ra, -8(s0)\n");
      fprintf(fp, "\tld s0, -16(s0)\n");
    } else {
      fprintf(fp, "\tld s0, -8(s0)\n");
    }
    // IncrementStackPointer(emitter, stack_frame_size, fp);
  }
}

static void RestoreExceptionLandingState(RVEmitter* emitter, FILE* fp) {
  fprintf(fp, "\tmv sp, s0\n");
  DecrementStackPointer(emitter, StackFrameSize(emitter), fp);

  int struct_return_phys = -1;
  if (emitter->rv->struct_return_reg >= 0) {
    bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
                   !emitter->rv->not_leaf;
    struct_return_phys =
        (is_leaf ? RV_FIRST_LEAF_INT_REG_VAR : RV_FIRST_INT_REG_VAR) +
        emitter->rv->struct_return_reg;
  }
  // The callee-saved registers are deliberately left as the unwinder delivered
  // them.  Their frame slots hold the *caller's* values, stored on entry, but a
  // handler needs the values this function itself had at the call that threw,
  // and reloading the slots would overwrite exactly those.  The unwinder
  // reconstructs them from the CFI of the frames it pops, which is why the FDE
  // has to describe where the prologue put them.  Skip their slots to reach the
  // hidden result pointer, which is this function's own.
  int offset = emitter->saved_reg_offset -
               8 * BitSetCount(&emitter->regs->used_int_regs) -
               8 * BitSetCount(&emitter->regs->used_float_regs) -
               RV_VLEN_BYTES * BitSetCount(&emitter->regs->used_vector_regs);
  if (struct_return_phys >= 0 && HasExceptionStructReturn(emitter)) {
    char buf[8];
    fprintf(fp, "\tld %s, %d(sp)\t// hidden result pointer\n",
            RVRegisterNameFromNum(struct_return_phys, kRVRegTypeInt, buf,
                                  sizeof(buf)),
            offset);
  }
}

// The rmov instructions are an explicit mov from operand[1] to
// operand[0].  Both are registers.
static const char* MoveMnemonic(RVOpcode opcode, RVRegister* dest,
                                RVRegister* src) {
  if (dest->type == kRVRegTypeFloat || src->type == kRVRegTypeFloat) {
    if (opcode == RV_OP(fmv_d)) {
      return "fmv.d";
    }
    return "fmv.s";
  }
  return "mv";
}

static void PrintRmov(RVEmitter* emitter, TargetInstruction* inst, FILE* fp) {
  if (inst->operand[0] == NULL || inst->operand[1] == NULL) {
    return;
  }
  if (inst->operand[1]->block == NULL) {
    // Optimized out.
    return;
  }
  if (inst->operand[0]->reg == NULL || inst->operand[1]->reg == NULL) {
    // Register allocation removed an unused side of the copy.
    return;
  }
  assert(inst->operand[0]->reg != NULL);
  assert(inst->operand[1]->reg != NULL);
  
  // Don't output mov rx,rx.
  if (inst->operand[0]->reg == inst->operand[1]->reg) {
    return;
  }

  const char* mnemonic =
      MoveMnemonic((RVOpcode)inst->opcode, (RVRegister*)inst->operand[0]->reg,
                   (RVRegister*)inst->operand[1]->reg);
  char buf1[8], buf2[8];
  fprintf(
      fp, "\t%-12s%s, %s\n", mnemonic,
      RVRegisterName((RVRegister*)inst->operand[0]->reg, buf1, sizeof(buf1)),
      RVRegisterName((RVRegister*)inst->operand[1]->reg, buf2, sizeof(buf2)));
}

static void PrintDestMove(TargetInstruction* inst, FILE* fp) {
  TargetInstruction* src = inst->operand[0];
  assert(src != NULL);
  if (src->block == NULL) {
    return;
  }
  if (src->reg == NULL) {
    return;
  }
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

  const char* mnemonic = MoveMnemonic((RVOpcode)inst->opcode,
                                      (RVRegister*)dest_reg,
                                      (RVRegister*)src->reg);
  char buf1[8], buf2[8];
  fprintf(fp, "\t%-12s%s, %s\n", mnemonic,
          RVRegisterName((RVRegister*)dest_reg, buf1, sizeof(buf1)),
          RVRegisterName((RVRegister*)src->reg, buf2, sizeof(buf2)));
}

static const char* GetRegisterName(TargetInstruction* inst, char* buf, size_t size) {
  if (inst == NULL) {
    return "";
  }
  if (inst->reg != NULL) {
    return RVRegisterName((RVRegister*)inst->reg, buf, size);
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return RVRegisterName((RVRegister*)inst->dest->reg, buf, size);
  }
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(x0):
      return RVRegisterNameFromNum(RV_INT_ZERO_REG, kRVRegTypeInt, buf, size);
    case RV_OP(t0):
      return RVRegisterNameFromNum(RV_INT_TEMP_START_1, kRVRegTypeInt, buf,
                                   size);
    case RV_OP(t1):
      return RVRegisterNameFromNum(RV_INT_TEMP_START_1 + 1, kRVRegTypeInt, buf,
                                   size);
    case RV_OP(t2):
      return RVRegisterNameFromNum(RV_INT_TEMP_START_1 + 2, kRVRegTypeInt, buf,
                                   size);
    case RV_OP(fp):
      return RVRegisterNameFromNum(RV_FP_REG, kRVRegTypeInt, buf, size);
    case RV_OP(sp):
      return RVRegisterNameFromNum(RV_SP_REG, kRVRegTypeInt, buf, size);
    case RV_OP(a0):
    case RV_OP(a1):
    case RV_OP(a2):
    case RV_OP(a3):
    case RV_OP(a4):
    case RV_OP(a5):
    case RV_OP(a6):
    case RV_OP(a7):
      return RVRegisterNameFromNum(
          (int)inst->opcode - RV_OP(a0) + RV_INT_ARG_START, kRVRegTypeInt, buf,
          size);
    case RV_OP(resulti):
    case RV_OP(call):
    case RV_OP(rcall):
      return RVRegisterNameFromNum(RV_INT_RETURN_REG, kRVRegTypeInt, buf, size);
    case RV_OP(resultf):
    case RV_OP(resultd):
    case RV_OP(callf):
    case RV_OP(rcallf):
      return RVRegisterNameFromNum(RV_FLOAT_RETURN_REG, kRVRegTypeFloat, buf,
                                   size);
    default:
      return "";
  }
}

static AsmOperand* GetAsmOperand(RVAsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    return NULL;
  }
  if ((size_t)index < inst->asm_node->outputs.length) {
    return inst->asm_node->outputs.value.p[index];
  }
  return inst->asm_node->inputs.value.p[index - inst->asm_node->outputs.length];
}

static int AtomicSizeLog2(TargetInstruction* inst) {
  return (inst->flags & RV_ATOMIC_SIZE_MASK) >> RV_ATOMIC_SIZE_SHIFT;
}

static int AtomicOrder(TargetInstruction* inst) {
  return (inst->flags & RV_ATOMIC_ORDER_MASK) >> RV_ATOMIC_ORDER_SHIFT;
}

static int AtomicFailureOrder(TargetInstruction* inst) {
  return (inst->flags & RV_ATOMIC_FAILURE_ORDER_MASK) >>
         RV_ATOMIC_FAILURE_ORDER_SHIFT;
}

static bool AtomicOrderHasAcquire(int order) {
  return order == 1 || order == 2 || order == 4 || order == 5;
}

static bool AtomicOrderHasRelease(int order) {
  return order == 3 || order == 4 || order == 5;
}

static const char* AtomicSuffix(bool acquire, bool release) {
  if (acquire && release) {
    return ".aqrl";
  }
  if (acquire) {
    return ".aq";
  }
  if (release) {
    return ".rl";
  }
  return "";
}

static void PrintAtomicInstruction(TargetInstruction* inst,
                                   const char* func_name, FILE* fp) {
  int size = AtomicSizeLog2(inst);
  int order = AtomicOrder(inst);
  int failure_order = AtomicFailureOrder(inst);
  bool acquire = AtomicOrderHasAcquire(order);
  bool release = AtomicOrderHasRelease(order);
  bool seq_cst = order == 5;
  char b0[16], b1[16], b2[16], b3[16];
  const char* result = GetRegisterName(inst, b0, sizeof(b0));

  switch ((RVOpcode)inst->opcode) {
    case RV_OP(atomic_load): {
      const char* addr = GetRegisterName(inst->operand[0], b1, sizeof(b1));
      static const char* loads[] = {"lbu", "lhu", "lw", "ld"};
      if (seq_cst) {
        fprintf(fp, "\tfence\n");
      }
      fprintf(fp, "\t%s %s, 0(%s)\n", loads[size], result, addr);
      if (acquire) {
        fprintf(fp, "\tfence\n");
      }
      return;
    }
    case RV_OP(atomic_store): {
      const char* value =
          GetRegisterName(inst->operand[0], b0, sizeof(b0));
      const char* addr = GetRegisterName(inst->operand[1], b1, sizeof(b1));
      static const char* stores[] = {"sb", "sh", "sw", "sd"};
      if (release || seq_cst) {
        fprintf(fp, "\tfence\n");
      }
      fprintf(fp, "\t%s %s, 0(%s)\n", stores[size], value, addr);
      if (seq_cst) {
        fprintf(fp, "\tfence\n");
      }
      return;
    }
    case RV_OP(atomic_fence):
      if (order != 0) {
        fprintf(fp, "\tfence\n");
      }
      return;
    case RV_OP(atomic_fetch_add):
    case RV_OP(atomic_fetch_sub):
    case RV_OP(atomic_add_fetch):
    case RV_OP(atomic_sub_fetch): {
      bool add = inst->opcode == (TargetOpcode)RV_OP(atomic_fetch_add) ||
                 inst->opcode == (TargetOpcode)RV_OP(atomic_add_fetch);
      bool return_new =
          inst->opcode == (TargetOpcode)RV_OP(atomic_add_fetch) ||
          inst->opcode == (TargetOpcode)RV_OP(atomic_sub_fetch);
      const char* addr =
          GetRegisterName(inst->operand[0], b1, sizeof(b1));
      const char* value =
          GetRegisterName(inst->operand[1], b2, sizeof(b2));
      if (size < 2) {
        int value_mask = size == 0 ? 0xff : 0xffff;
        fprintf(fp, "\tandi t1, %s, 3\n", addr);
        fprintf(fp, "\tslli t1, t1, 3\n");
        fprintf(fp, "\tli t2, %d\n", value_mask);
        fprintf(fp, "\tsll t2, t2, t1\n");
        fprintf(fp, "\tsll t3, %s, t1\n", value);
        fprintf(fp, "\tand t3, t3, t2\n");
        fprintf(fp, "\tandi t0, %s, -4\n", addr);
        fprintf(fp, ".L%s_atomic_retry_%d:\n", func_name, inst->id);
        fprintf(fp, "\tlr.w%s %s, (t0)\n",
                AtomicSuffix(acquire, release), result);
        fprintf(fp, "\tnot t4, t2\n");
        fprintf(fp, "\tand t4, %s, t4\n", result);
        fprintf(fp, "\tand t5, %s, t2\n", result);
        fprintf(fp, "\t%s t5, t5, t3\n", add ? "add" : "sub");
        fprintf(fp, "\tand t5, t5, t2\n");
        fprintf(fp, "\tor t5, t5, t4\n");
        fprintf(fp, "\tsc.w%s t6, t5, (t0)\n",
                AtomicSuffix(order == 5, release));
        fprintf(fp, "\tbnez t6, .L%s_atomic_retry_%d\n", func_name,
                inst->id);
        if (return_new) {
          fprintf(fp, "\tmv %s, t5\n", result);
        }
        fprintf(fp, "\tsrl %s, %s, t1\n", result, result);
        fprintf(fp, "\tli t4, %d\n", value_mask);
        fprintf(fp, "\tand %s, %s, t4\n", result, result);
        return;
      }
      fprintf(fp, "\tmv t0, %s\n", value);
      if (!add) {
        fprintf(fp, "\tsub t0, zero, t0\n");
      }
      fprintf(fp, "\tamoadd.%c%s %s, t0, (%s)\n",
              size == 3 ? 'd' : 'w', AtomicSuffix(acquire, release), result,
              addr);
      if (return_new) {
        fprintf(fp, "\t%s %s, %s, t0\n", size == 3 ? "add" : "addw",
                result, result);
      }
      return;
    }
    case RV_OP(atomic_compare_exchange_bool):
    case RV_OP(atomic_compare_exchange_val):
    case RV_OP(atomic_compare_exchange_n): {
      bool expected_is_pointer =
          inst->opcode == (TargetOpcode)RV_OP(atomic_compare_exchange_n);
      bool returns_bool =
          inst->opcode != (TargetOpcode)RV_OP(atomic_compare_exchange_val);
      bool lr_acquire =
          acquire || AtomicOrderHasAcquire(failure_order) || order == 5;
      const char* addr =
          GetRegisterName(inst->operand[0], b1, sizeof(b1));
      const char* expected =
          GetRegisterName(inst->operand[1], b2, sizeof(b2));
      const char* desired =
          GetRegisterName(inst->operand[2], b3, sizeof(b3));
      if (size < 2) {
        int value_mask = size == 0 ? 0xff : 0xffff;
        if (expected_is_pointer) {
          fprintf(fp, "\t%s t3, 0(%s)\n", size == 0 ? "lbu" : "lhu",
                  expected);
        } else {
          fprintf(fp, "\tmv t3, %s\n", expected);
        }
        fprintf(fp, "\tandi t1, %s, 3\n", addr);
        fprintf(fp, "\tslli t1, t1, 3\n");
        fprintf(fp, "\tli t2, %d\n", value_mask);
        fprintf(fp, "\tand t3, t3, t2\n");
        fprintf(fp, "\tand t4, %s, t2\n", desired);
        fprintf(fp, "\tsll t2, t2, t1\n");
        fprintf(fp, "\tsll t3, t3, t1\n");
        fprintf(fp, "\tsll t4, t4, t1\n");
        fprintf(fp, "\tandi t0, %s, -4\n", addr);
        fprintf(fp, ".L%s_atomic_retry_%d:\n", func_name, inst->id);
        fprintf(fp, "\tlr.w%s %s, (t0)\n",
                AtomicSuffix(lr_acquire, order == 5), result);
        fprintf(fp, "\tand t6, %s, t2\n", result);
        fprintf(fp, "\tbne t6, t3, .L%s_atomic_mismatch_%d\n",
                func_name, inst->id);
        fprintf(fp, "\tnot t6, t2\n");
        fprintf(fp, "\tand t5, %s, t6\n", result);
        fprintf(fp, "\tor t5, t5, t4\n");
        fprintf(fp, "\tsc.w%s t6, t5, (t0)\n",
                AtomicSuffix(order == 5, release));
        if ((inst->flags & RV_ATOMIC_WEAK) != 0) {
          fprintf(fp, "\tbnez t6, .L%s_atomic_mismatch_%d\n", func_name,
                  inst->id);
        } else {
          fprintf(fp, "\tbnez t6, .L%s_atomic_retry_%d\n", func_name,
                  inst->id);
        }
        if (returns_bool) {
          fprintf(fp, "\tli %s, 1\n", result);
        } else {
          fprintf(fp, "\tsrl %s, %s, t1\n", result, result);
          fprintf(fp, "\tli t6, %d\n", value_mask);
          fprintf(fp, "\tand %s, %s, t6\n", result, result);
        }
        fprintf(fp, "\tj .L%s_atomic_done_%d\n", func_name, inst->id);
        fprintf(fp, ".L%s_atomic_mismatch_%d:\n", func_name, inst->id);
        fprintf(fp, "\tsrl %s, %s, t1\n", result, result);
        fprintf(fp, "\tli t6, %d\n", value_mask);
        fprintf(fp, "\tand %s, %s, t6\n", result, result);
        if (expected_is_pointer) {
          fprintf(fp, "\t%s %s, 0(%s)\n", size == 0 ? "sb" : "sh",
                  result, expected);
        }
        if (returns_bool) {
          fprintf(fp, "\tli %s, 0\n", result);
        }
        fprintf(fp, ".L%s_atomic_done_%d:\n", func_name, inst->id);
        return;
      }
      if (expected_is_pointer) {
        fprintf(fp, "\t%s t0, 0(%s)\n", size == 3 ? "ld" : "lw", expected);
      } else {
        fprintf(fp, "\tmv t0, %s\n", expected);
      }
      fprintf(fp, "\tmv t1, %s\n", desired);
      fprintf(fp, "\tmv t3, %s\n", addr);
      fprintf(fp, ".L%s_atomic_retry_%d:\n", func_name, inst->id);
      fprintf(fp, "\tlr.%c%s %s, (%s)\n", size == 3 ? 'd' : 'w',
              AtomicSuffix(lr_acquire, order == 5), result, "t3");
      fprintf(fp, "\tbne %s, t0, .L%s_atomic_mismatch_%d\n", result,
              func_name, inst->id);
      fprintf(fp, "\tsc.%c%s t2, t1, (%s)\n", size == 3 ? 'd' : 'w',
              AtomicSuffix(order == 5, release), "t3");
      if ((inst->flags & RV_ATOMIC_WEAK) != 0) {
        fprintf(fp, "\tbnez t2, .L%s_atomic_mismatch_%d\n", func_name,
                inst->id);
      } else {
        fprintf(fp, "\tbnez t2, .L%s_atomic_retry_%d\n", func_name,
                inst->id);
      }
      if (returns_bool) {
        fprintf(fp, "\tli %s, 1\n", result);
      }
      fprintf(fp, "\tj .L%s_atomic_done_%d\n", func_name, inst->id);
      fprintf(fp, ".L%s_atomic_mismatch_%d:\n", func_name, inst->id);
      if (expected_is_pointer) {
        fprintf(fp, "\t%s %s, 0(%s)\n", size == 3 ? "sd" : "sw", result,
                expected);
      }
      if (returns_bool) {
        fprintf(fp, "\tli %s, 0\n", result);
      }
      fprintf(fp, ".L%s_atomic_done_%d:\n", func_name, inst->id);
      return;
    }
    default:
      assert(false);
  }
}

static int FindAsmOperandByName(RVAsmInstruction* inst, const char* name,
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

static int FindAsmLabelByName(RVAsmInstruction* inst, const char* name,
                              size_t len) {
  for (size_t i = 0; i < inst->asm_node->labels.length; i++) {
    String* label = inst->asm_node->labels.value.p[i];
    if (label->length == len && strncmp(label->value, name, len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void PrintAsmOperand(FILE* fp, RVAsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    fprintf(fp, "<bad-asm-operand>");
    return;
  }
  if (inst->reg_nums[index] == INT_MIN) {
    fprintf(fp, "%" PRId64, inst->immediate_values[index]);
    return;
  }
  char buf[32];
  fprintf(fp, "%s", RVRegisterNameFromNum(inst->reg_nums[index],
                                          inst->is_fp[index] ? kRVRegTypeFloat
                                                             : kRVRegTypeInt,
                                          buf, sizeof(buf)));
}

static void PrintAsmLabel(FILE* fp, RVAsmInstruction* inst, int index) {
  if (index < 0 || (size_t)index >= inst->asm_node->labels.length) {
    fprintf(fp, "<bad-asm-label>");
    return;
  }
  String* label = inst->asm_node->labels.value.p[index];
  fprintf(fp, "%s", label->value);
}

static void PrintExtendedAsm(FILE* fp, RVAsmInstruction* inst,
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
    if (*p == 'l') {
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

// Main instruction printer.
static void PrintVsetivli(FILE* fp, TargetInstruction* inst) {
  int elem_log = (inst->flags >> RV_SIMD_ELEM_SHIFT) & 3;
  int vec_bytes = (inst->flags & RV_SIMD_128) ? 16 : 8;
  int sew = 1 << elem_log;
  int vl = vec_bytes / sew;
  static const char* sew_name[] = {"e8", "e16", "e32", "e64"};
  fprintf(fp, "\tvsetivli    zero, %d, %s, m1, ta, ma\n", vl, sew_name[elem_log]);
}

static void PrintVectorAddr(FILE* fp, const char* base, int offset) {
  if (offset >= -2048 && offset <= 2047) {
    fprintf(fp, "\taddi t0, %s, %d\n", base, offset);
  } else if (offset < 0) {
    fprintf(fp, "\tli t0, %d\n", -offset);
    fprintf(fp, "\tsub t0, %s, t0\n", base);
  } else {
    fprintf(fp, "\tli t0, %d\n", offset);
    fprintf(fp, "\tadd t0, %s, t0\n", base);
  }
}

static const char* VectorMemMnemonic(bool load, int elem_log) {
  static const char* loads[] = {"vle8.v", "vle16.v", "vle32.v", "vle64.v"};
  static const char* stores[] = {"vse8.v", "vse16.v", "vse32.v", "vse64.v"};
  return load ? loads[elem_log] : stores[elem_log];
}

static bool IsVectorCompare(RVOpcode opcode) {
  return opcode == RV_OP(vcmeq) || opcode == RV_OP(vcmne) ||
         opcode == RV_OP(vcmlt) || opcode == RV_OP(vcmle) ||
         opcode == RV_OP(vcmltu) || opcode == RV_OP(vcmleu);
}

static bool PrintVectorInstruction(TargetInstruction* inst, FILE* fp) {
  RVOpcode opcode = (RVOpcode)inst->opcode;
  if (opcode < RV_OP(vle) || opcode > RV_OP(vcmleu)) {
    return false;
  }
  char buf1[8], buf2[8], buf3[8];
  int elem_log = (inst->flags >> RV_SIMD_ELEM_SHIFT) & 3;
  if (opcode == RV_OP(vle)) {
    int offset = TargetIsConst(inst->operand[1])
                     ? (int)TargetIntValue(inst->operand[1])
                     : 0;
    const char* base = GetRegisterName(inst->operand[0], buf2, sizeof(buf2));
    PrintVsetivli(fp, inst);
    if (offset == 0) {
      fprintf(fp, "\t%-12s%s, (%s)\n", VectorMemMnemonic(true, elem_log),
              GetRegisterName(inst, buf1, sizeof(buf1)), base);
    } else {
      PrintVectorAddr(fp, base, offset);
      fprintf(fp, "\t%-12s%s, (t0)\n", VectorMemMnemonic(true, elem_log),
              GetRegisterName(inst, buf1, sizeof(buf1)));
    }
    return true;
  }
  if (opcode == RV_OP(vse)) {
    int offset = TargetIsConst(inst->operand[2])
                     ? (int)TargetIntValue(inst->operand[2])
                     : 0;
    const char* base = GetRegisterName(inst->operand[1], buf2, sizeof(buf2));
    PrintVsetivli(fp, inst);
    if (offset == 0) {
      fprintf(fp, "\t%-12s%s, (%s)\n", VectorMemMnemonic(false, elem_log),
              GetRegisterName(inst->operand[0], buf1, sizeof(buf1)), base);
    } else {
      PrintVectorAddr(fp, base, offset);
      fprintf(fp, "\t%-12s%s, (t0)\n", VectorMemMnemonic(false, elem_log),
              GetRegisterName(inst->operand[0], buf1, sizeof(buf1)));
    }
    return true;
  }
  PrintVsetivli(fp, inst);
  if (IsVectorCompare(opcode)) {
    fprintf(fp, "\t%-12sv0, %s, %s\n", RVOpcodeName(inst->opcode),
            GetRegisterName(inst->operand[0], buf2, sizeof(buf2)),
            GetRegisterName(inst->operand[1], buf3, sizeof(buf3)));
    fprintf(fp, "\tvmv.v.i     %s, 0\n",
            GetRegisterName(inst, buf1, sizeof(buf1)));
    fprintf(fp, "\tvmerge.vim  %s, %s, -1\n",
            GetRegisterName(inst, buf1, sizeof(buf1)),
            GetRegisterName(inst, buf2, sizeof(buf2)));
    return true;
  }
  fprintf(fp, "\t%-12s%s, %s, %s\n", RVOpcodeName(inst->opcode),
          GetRegisterName(inst, buf1, sizeof(buf1)),
          GetRegisterName(inst->operand[0], buf2, sizeof(buf2)),
          GetRegisterName(inst->operand[1], buf3, sizeof(buf3)));
  return true;
}

static void PrintVectorSpill(RVRegister* reg, int offset, bool reload,
                             int id, FILE* fp) {
  char buf1[8];
  if (offset >= -2048 && offset <= 2047) {
    fprintf(fp, "\taddi t0, s0, -%d\n", offset);
  } else {
    fprintf(fp, "\tli t0, %d\n", offset);
    fprintf(fp, "\tsub t0, s0, t0\n");
  }
  fprintf(fp, "\tvsetivli    zero, 16, e8, m1, ta, ma\n");
  fprintf(fp, "\t%s %s, (t0)\t// %s spilled @%d\n",
          reload ? "vle8.v" : "vse8.v",
          RVRegisterName(reg, buf1, sizeof(buf1)),
          reload ? "Reloaded" : "Spilled", id);
}

static void PrintInstruction(RVEmitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    TargetBasicBlock* b = inst->block;
    fprintf(fp, "\n\t// *** Basic block %zd\n\n", b->block_id);
    emitter->current_block = inst->block;
  }
  if (((int)inst->opcode == (int)RV_OP(label))) {
    if ((inst->flags & RV_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    if ((inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
      RestoreExceptionLandingState(emitter, fp);
    }
    return;
  }

  if (((int)inst->opcode == (int)RV_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }
  
  if (((int)inst->opcode == (int)RV_OP(ivarreg)) || ((int)inst->opcode == (int)RV_OP(fvarreg))) {
    return;
  }
  if (!IsPrintable(inst)) {
    return;
  }
  if (inst->observable_checkpoint) {
    return;
  }
  
  const bool show_id = 1;

  if (show_id) {
    fprintf(fp, "/* @%d */ ", inst->id);
  }

  if (PrintVectorInstruction(inst, fp)) {
    return;
  }
  
  // Buffers for register name printing.
  char buf1[8];
  char buf2[8];
  char buf3[256];

  // Special case instructions.
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(atomic_load):
    case RV_OP(atomic_store):
    case RV_OP(atomic_fetch_add):
    case RV_OP(atomic_fetch_sub):
    case RV_OP(atomic_add_fetch):
    case RV_OP(atomic_sub_fetch):
    case RV_OP(atomic_compare_exchange_bool):
    case RV_OP(atomic_compare_exchange_val):
    case RV_OP(atomic_compare_exchange_n):
    case RV_OP(atomic_fence):
      PrintAtomicInstruction(inst, func_name, fp);
      return;
//    case RV_OP(rmov):
//    case RV_OP(rmovf):
//    case RV_OP(rmovd):
//      PrintRmov(emitter, inst, fp);
//      return;

    case RV_OP(mv):
    case RV_OP(fmv_s):
    case RV_OP(fmv_d):
      if (inst->operand[1] != NULL) {
        PrintRmov(emitter, inst, fp);
      } else {
        PrintDestMove(inst, fp);
      }
      return;
    case RV_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n", SymbolName(sym, buf3, sizeof(buf3)));
      } else if (SymbolHasWeakBinding(sym->symbol)) {
        fprintf(fp, "\t.weak %s\n", SymbolName(sym, buf3, sizeof(buf3)));
      } else {
        fprintf(fp, "\t.global %s\n", SymbolName(sym, buf3, sizeof(buf3)));
      }
      return;
    }
    case RV_OP(call):
    case RV_OP(callf): {
      assert(((int)inst->operand[0]->opcode == (int)RV_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      fprintf(fp, "\t%-12s%s\n", "call", SymbolName(sym, buf3, sizeof(buf3)));
      return;
    }
    case RV_OP(tprel):
    case RV_OP(tlsgd): {
      assert(inst->operand[0] != NULL &&
             inst->operand[0]->opcode == (TargetOpcode)RV_OP(symbol));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      fprintf(fp, "\t%-12s%s, %s\n",
              (RVOpcode)inst->opcode == RV_OP(tlsgd) ? "tlsgd" : "tprel",
              GetRegisterName(inst, buf1, sizeof(buf1)),
              SymbolName(sym, buf3, sizeof(buf3)));
      return;
    }

    case RV_OP(rcall):
    case RV_OP(rcallf):
      fprintf(fp, "\t%-12s x1, %s, 0\n", "jalr",
              GetRegisterName(inst->operand[0], buf2,
                             sizeof(buf2)));

      return;
    case RV_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case RV_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case RV_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t");
      if ((inst->flags & RV_INST_EXTENDED_ASM) != 0) {
        PrintExtendedAsm(fp, (RVAsmInstruction*)inst, lit->value.value);
      } else {
        fprintf(fp, "%s", lit->value.value);
      }
      fprintf(fp, "\n");
      lit->base.disabled = true;
      return;
    }

    case RV_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }

    case RV_OP(spill): {
      RVRegister* reg = (RVRegister*)inst->reg;
      int offset = (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      if (reg->type == kRVRegTypeVector) {
        PrintVectorSpill(reg, offset, false, inst->operand[0]->id, fp);
        return;
      }
      const int spill_addr = RV_SPILL_ADDR;
      if (!RVIsPossibleImmediate(offset)) {
        fprintf(fp, "\t%-12s%s, %d\n",
                "li",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                offset);
        fprintf(fp, "\t%-12s%s, s0, %s\n",
                "sub",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf2, sizeof(buf2)));
        fprintf(fp, "\t%-12s%s, 0(%s)\t// Spilled @%d\n",
                 reg->type == kRVRegTypeInt ? "sd" : "fsd",
                 RVRegisterName(reg, buf1, sizeof(buf1)),
                 RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf2, sizeof(buf2)),
                 inst->operand[0]->id);
      } else {
        fprintf(fp, "\t%-12s%s, -%d(s0)\t// Spilled @%d\n",
                 reg->type == kRVRegTypeInt ? "sd" : "fsd",
                 RVRegisterName(reg, buf1, sizeof(buf1)),
                 offset,
                 inst->operand[0]->id);
      }
      return;
    }
      
    case RV_OP(reload): {
      RVRegister* reg = (RVRegister*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      if (reg->type == kRVRegTypeVector) {
        PrintVectorSpill(reg, offset, true, spill->operand[0]->id, fp);
        return;
      }
      const int spill_addr = RV_SPILL_ADDR;
      if (!RVIsPossibleImmediate(offset)) {
        fprintf(fp, "\t%-12s%s, %d\n",
                "li",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                offset);
        fprintf(fp, "\t%-12s%s, s0, %s\n",
                "sub",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf2, sizeof(buf2)));
        fprintf(fp, "\t%-12s%s, 0(%s)\t// Reloaded spilled @%d\n",
                 reg->type == kRVRegTypeInt ? "ld" : "fld",
                 RVRegisterName(reg, buf1, sizeof(buf1)),
                 RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf2, sizeof(buf2)),
                 spill->operand[0]->id);
      } else {
        fprintf(fp, "\t%-12s%s, -%d(s0)\t// Reloaded spilled @%d\n",
                 reg->type == kRVRegTypeInt ? "ld" : "fld",
                 RVRegisterName(reg, buf1, sizeof(buf1)),
                 offset,
                 spill->operand[0]->id);
      }
      return;
    }
    default:
      break;
  }

  // General case for instruction printing.

  // Print opcode.
  fprintf(fp, "\t%-12s", RVOpcodeName(inst->opcode));

  // Print operands.
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(lw):
    case RV_OP(lh):
    case RV_OP(lb):
    case RV_OP(lwu):
    case RV_OP(lbu):
    case RV_OP(lhu):
    case RV_OP(flw):
    case RV_OP(ld):
    case RV_OP(fld):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      assert(inst->operand[0]->reg != NULL);
      if (TargetIsConst(inst->operand[1])) {
        int offset = (int)TargetIntValue(inst->operand[1]);
        assert(RVIsPossibleImmediate(offset));
        fprintf(fp, "%s, %d(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)),
                offset,
                GetRegisterName(inst->operand[0], buf2,
                               sizeof(buf2)));

      } else if (((int)inst->operand[1]->opcode == (int)RV_OP(symbol))) {
        assert((inst->flags & RV_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)),
                SymbolName((TargetSymbol*)inst->operand[1], buf3, sizeof(buf3)),
                GetRegisterName(inst->operand[0], buf2,
                               sizeof(buf2)));
      } else if (((int)inst->operand[1]->opcode == (int)RV_OP(label))) {
        assert((inst->flags & RV_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)),
                func_name, inst->operand[1]->id,
                GetRegisterName(inst->operand[0], buf2,
                               sizeof(buf2)));
      } else if (((int)inst->operand[1]->opcode == (int)RV_OP(x0))) {
        fprintf(fp, "%s, 0(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[0], buf2,
                               sizeof(buf2)));
      } else {
        assert(false);
      }
      break;

    case RV_OP(sw):
    case RV_OP(sh):
    case RV_OP(sd):
    case RV_OP(sb):
    case RV_OP(fsw):
    case RV_OP(fsd):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      if (TargetIsConst(inst->operand[2])) {
        int offset = (int)TargetIntValue(inst->operand[2]);
        assert(RVIsPossibleImmediate(offset));
        fprintf(fp, "%s, %d(%s)\n",
                GetRegisterName(inst->operand[0], buf1,
                               sizeof(buf1)),
                offset,
                GetRegisterName(inst->operand[1], buf2,
                               sizeof(buf2)));
      } else if (((int)inst->operand[2]->opcode == (int)RV_OP(symbol))) {
        assert((inst->flags & RV_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst->operand[0], buf1,
                               sizeof(buf1)),
                SymbolName((TargetSymbol*)inst->operand[2], buf3, sizeof(buf3)),
                GetRegisterName(inst->operand[1], buf2,
                               sizeof(buf2)));
      } else if (((int)inst->operand[2]->opcode == (int)RV_OP(label))) {
        assert((inst->flags & RV_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst->operand[0], buf1,
                               sizeof(buf1)),
                func_name, inst->operand[2]->id,
                GetRegisterName(inst->operand[1], buf2,
                               sizeof(buf2)));
      } else if (((int)inst->operand[2]->opcode == (int)RV_OP(x0))) {
        fprintf(fp, "%s, 0(%s)\n",
                GetRegisterName(inst->operand[0], buf1,
                               sizeof(buf1)),
                GetRegisterName(inst->operand[1], buf2,
                               sizeof(buf2)));

      } else {
        assert(false);
      }
      break;
      
    case RV_OP(nop):
      fprintf(fp, "\n");
      break;
      
    case RV_OP(beq):
    case RV_OP(bne):
    case RV_OP(blt):
    case RV_OP(bge):
    case RV_OP(bltu):
    case RV_OP(bgeu):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      fprintf(fp, "%s, %s, .%s_label_%d\n",
              GetRegisterName(inst->operand[0], buf1,
                             sizeof(buf1)),
              GetRegisterName(inst->operand[1], buf2,
                             sizeof(buf2)),
              func_name, inst->operand[2]->id);
      break;

    // Branch zero, only one register and a target.
    case RV_OP(beqz):
    case RV_OP(bnez):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[0]->reg != NULL);
      fprintf(fp, "%s, .%s_label_%d\n",
              GetRegisterName(inst->operand[0], buf1,
                             sizeof(buf1)),
              func_name, inst->operand[1]->id);
      break;

    case RV_OP(j): {
      assert(inst->operand[0] != NULL);
      TargetInstruction* dest = inst->operand[0];
      if (((int)dest->opcode == (int)RV_OP(label))) {
        fprintf(fp, ".%s_label_%d\n", func_name, inst->operand[0]->id);
      } else if (((int)dest->opcode == (int)RV_OP(symbol))) {
        fprintf(fp, "%s\n", SymbolName((TargetSymbol*)dest, buf3, sizeof(buf3)));
      } else if (((int)dest->opcode == (int)RV_OP(named_label))) {
        fprintf(fp, "%s\n", ((TargetNamedLabel*)dest)->name);
      } else {
        assert(false);
      }
      break;
    }
      
    case RV_OP(jr):
      fprintf(fp, "%s\n",
              GetRegisterName(inst->operand[0], buf1,
                             sizeof(buf1)));
      break;
      
    case RV_OP(jalr):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      fprintf(fp, "%s, %s, %d\n",
              GetRegisterName(inst->operand[0], buf1,
                             sizeof(buf1)),
              GetRegisterName(inst->operand[1], buf2,
                             sizeof(buf2)),
              (int)TargetIntValue(inst->operand[2]));
      break;

    case RV_OP(li): {
      int64_t value = TargetIntValue(inst->operand[0]);
      
      fprintf(fp, "%s, %" PRId64 "\t\t// 0x%" PRIx64 "",
              GetRegisterName(inst, buf1, sizeof(buf1)),
              value,
              value);
      if (value >= 0 && value < 0xff) {
        // Possible ASCII.
        if (value >= ' ' && value < 0x7f) {
          fprintf(fp, " ASCII '%c'", (int)value);
        } else {
          fprintf(fp, " ASCII \\x%x", (int)value);
        }
      }
      fprintf(fp, "\n");
      break;
      }

    default: {
      const char* sep = "";
      if (inst->reg != NULL) {
        fprintf(fp, "%s",
                GetRegisterName(inst, buf1, sizeof(buf1)));
        sep = ", ";
      }
      for (int i = 0; i < 2; i++) {
        if (inst->operand[i] != NULL) {
          if (TargetIsConst(inst->operand[i])) {
            fprintf(fp, "%s%" PRId64 "", sep, TargetIntValue(inst->operand[i]));
          } else if (((int)inst->operand[i]->opcode == (int)RV_OP(symbol))) {
            if ((inst->flags & RV_HI_RELOC) != 0) {
              fprintf(fp, "%s%%hi(%s)", sep,
                      SymbolName((TargetSymbol*)inst->operand[i], buf3, sizeof(buf3)));
            } else {
              fprintf(fp, "%s%s", sep,
                      SymbolName((TargetSymbol*)inst->operand[i], buf3, sizeof(buf3)));
            }
          } else if (((int)inst->operand[i]->opcode == (int)RV_OP(literal))) {
            TargetLiteral* literal = (TargetLiteral*)inst->operand[i];
            char litname[64];
            LiteralAsmName(literal->literal_id, litname, sizeof(litname));
            fprintf(fp, "%s%s", sep, litname);
          } else {
            fprintf(fp, "%s%s", sep,
                    GetRegisterName(inst->operand[i], buf2,
                                   sizeof(buf2)));
          }
          sep = ", ";
        }
      }
      fprintf(fp, "\n");
    }
  }
  
}

void RVEmitterInit(RVEmitter* emitter, RVGenerator* rv) {
  emitter->rv = rv;
  emitter->regs = &rv->register_allocator;
  emitter->saved_reg_offset = 0;
  emitter->spill_region_size = rv->register_allocator.max_spilled_region_size;
  emitter->first_spill_offset = 0;
  emitter->current_block = NULL;
}

RVEmitter* NewRVEmitter(RVGenerator* rv) {
  RVEmitter* emitter = malloc(sizeof(RVEmitter));
  RVEmitterInit(emitter, rv);
  return emitter;
}

void RVEmitterDestruct(RVEmitter* emitter) {
}

void RVEmitterDelete(RVEmitter* emitter) {
  RVEmitterDestruct(emitter);
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

static void RVFillLSDAInfo(RVEmitter* emitter, DaveEHLSDARange* lsda_ranges,
                           size_t* count) {
  *count = 0;
  for (size_t i = 0; i < emitter->rv->exception_ranges.length && *count < 64;
       i++) {
    RVExceptionRange* range = emitter->rv->exception_ranges.value.p[i];
    DaveEHLSDARange* out = &lsda_ranges[(*count)++];
    out->try_start_id = range->try_start->id;
    out->try_end_id = range->try_end->id;
    out->landing_pad_id = range->catch_label->id;
    out->is_cleanup = range->is_cleanup;
    out->catch_typeinfo =
        range->is_cleanup ? NULL : LSDATypeInfoSymbol(range->catch_typeinfo);
  }
}

// Describe where the prologue put the callee-saved registers, so that unwinding
// through this frame recovers the caller's values.  Without these rules the
// unwinder passes whatever the throwing code left in those registers on to the
// handler, which then installs them over the caller's own.  The registers are
// listed in the order SaveRegisters stores them, and are addressed from sp,
// which the prologue leaves stack_frame_size below the CFA (= s0).  RISC-V
// numbers its DWARF integer registers exactly as the architecture does, which is
// also what the register allocator uses, so no translation is needed.
static size_t RVSavedCFIRegisters(RVEmitter* emitter,
                                  DaveEHFrameSavedReg* saved_regs,
                                  size_t capacity) {
  BitSetIterator it;
  size_t count = 0;
  int offset = emitter->saved_reg_offset;
  int stack_frame_size = StackFrameSize(emitter);

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    if (count < capacity) {
      saved_regs[count].dwarf_reg = reg;
      saved_regs[count].cfa_offset = offset - stack_frame_size;
      count++;
    }
    offset -= 8;
    BitSetIteratorNext(&it);
  }
  return count;
}

static void RVPrintEHMetadata(RVEmitter* emitter, FILE* fp,
                              const char* func_name) {
  if (emitter->rv->base.varargs) {
    return;
  }
  DaveEHLSDARange lsda_ranges[64];
  DaveEHFrameSavedReg saved_regs[RV_NUM_INT_REGS];
  size_t lsda_count = 0;
  size_t saved_reg_count =
      RVSavedCFIRegisters(emitter, saved_regs,
                          sizeof(saved_regs) / sizeof(saved_regs[0]));
  RVFillLSDAInfo(emitter, lsda_ranges, &lsda_count);
  DaveEHFrameEmitInfo info = {
      .ranges = lsda_ranges,
      .range_count = lsda_count,
      .func_name = func_name,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = true,
      .cie_ra_reg = 1,
      .cie_cfa_reg = 2,
      .cie_fp_reg = 8,
      .entry_cfa_offset = 0,
      .frame_cfa_offset = StackFrameSize(emitter),
      .fp_cfa_offset = 0,
      .saved_fp_offset = -16,
      .saved_ra_offset = -8,
      .saved_regs = saved_regs,
      .saved_reg_count = saved_reg_count,
  };
  DaveEHPrintGCCExceptTable(fp, &info);
  DaveEHPrintEHFrameCIE(fp, &info, "");
  DaveEHPrintEHFrameFDE(fp, &info, "");
}

void RVPrintFunction(RVEmitter* emitter, FILE* fp) {
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
  DaveEHPrintFuncTextLabel(fp, func_name);
  TargetInstruction* inst = TargetFirstInstruction(&emitter->rv->base);
  while (inst != NULL) {
    PrintInstruction(emitter, inst, func_name, fp);
    inst = TargetNext(inst);
  }
  fprintf(fp, ".func_end_%s:\n", func_name);
  fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
  RVPrintEHMetadata(emitter, fp, func_name);
}

void RVPrintCXXAdjustorThunks(FILE* fp) {
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
    fprintf(fp, "\t.weak %s\n", thunk_name);
    fprintf(fp, "\t.type %s, @function\n", thunk_name);
    fprintf(fp, "%s:\n", thunk_name);
    if (thunk->this_adjustment != 0) {
      fprintf(fp, "\tli t0, %d\n", thunk->this_adjustment);
      fprintf(fp, "\tadd a0, a0, t0\n");
    }
    fprintf(fp, "\tj %s\n", target_name);
    fprintf(fp, ".func_end_%s:\n", thunk_name);
    fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", thunk_name, thunk_name,
            thunk_name);
  }
}
