//
//  arm_emitter.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "arm_emitter.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "compiler.h"
#include "arm_assembler.h"
#include "arm_codegen.h"
#include "arm_reg_alloc.h"
#include "target_basic_block.h"

static void Trap() {}

static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 21) {
    Trap();
  }
}

#if 0
// Build a MOV instruction out of MOVZ, MOVN and MOVK, based on the immediate
// value.
// A MOVZ, MOVN and MOVK instruction can take 16 bit immediate value shifted
// left by 0, 16, 32 or 48 (0 or 16 for 32 bit only.
// TODO: this goes in the assembler.
static void MoveImmediate(ARMEmitter* emitter,
                          const char* reg,
                          uint64_t value, FILE* fp) {
  if (value == 0) {
    fprintf(fp, "\tmovz %s, #0\n", reg);
    return;
  }
  if (reg[0] == 'w') {
    value &= 0xffffffff;
  }
  bool movz_done = false;
  int shift = 0;
  while (value != 0) {
    int v = value & 0xffff;
    if (v != 0) {
      const char* suffix = movz_done ? "k" : "z";
      if (shift != 0) {
        fprintf(fp, "\tmov%s %s, #%d, lsl #%d\n", suffix, reg, v, shift);
      } else {
        fprintf(fp, "\tmov%s %s, #%d\n", suffix, reg, v);
      }
      movz_done = true;
    }
    value >>= 16;
    shift += 16;
  }
}
#else
static void MoveImmediate(ARMEmitter* emitter,
                          const char* reg,
                          int64_t value, FILE* fp) {
  (void)emitter;
  value = (int32_t)value;
  // A plain "mov" immediate must be an 8-bit value rotated by an even amount;
  // 0..255 is always encodable.  Larger 16-bit values use movw, which accepts
  // any value in 0..0xffff.
  if (value >= 0 && value <= 0xff) {
    fprintf(fp, "\tmov %s, #%" PRId64 "\n", reg, value);
    return;
  }
  fprintf(fp, "\tmovw %s, #%d\n", reg, (int)(value & 0xffff));
  if ((value & ~0xffff) != 0) {
    fprintf(fp, "\tmovt %s, #%d\n", reg, (int)((value >> 16) & 0xffff));
  }
}

#endif

// Generate a sequence of add/sub instruction for an immediate.
// The add/sub instructions take a 12 bit immediate shifted left
// by 0 or 12 bits.  If the constant is negative, the add becomes
// a sub and vice versa.
static void AddSubImmediate(ARMEmitter* emitter,
                            const char* destreg,
                            const char* srcreg,
                         bool add,
                         int64_t value, const char* comment, FILE* fp) {
  if (value < 0) {
    value = -value;
    add = !add;
  }
  if (srcreg == NULL) {
    srcreg = destreg;
  }
  char commentbuf[256] = {0};
  if (comment != NULL) {
    snprintf(commentbuf, sizeof(commentbuf), "\t// %s", comment);
  }
  const char* op = add ? "add" : "sub";
  const char* tmpreg = "ip";
  if (value > 0xffffff) {
    // Too big for an immediate, put it into a register and use that.
    MoveImmediate(emitter, tmpreg, value, fp);
    fprintf(fp, "\t%s %s, %s, %s%s\n", op, destreg, srcreg, tmpreg, commentbuf);
    return;
  }
  int shift = 0;
  while (value != 0) {
    int v = value & 0xfff;
    if (v != 0) {
      if (shift != 0) {
        fprintf(fp, "\t%s %s, %s, #%d, lsl #%d%s\n", op, destreg, srcreg, v, shift, commentbuf);
      } else {
        fprintf(fp, "\t%s %s, %s, #%d%s\n", op, destreg, srcreg, v, commentbuf);
      }
      srcreg = destreg;
    }
    shift += 12;
    value >>= 12;
  }
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
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(tmp):
    case ARM_OP(fp):
    case ARM_OP(sp):
    case ARM_OP(lr):
    case ARM_OP(zr):
    case ARM_OP(literal):
    case ARM_OP(structreturn):
    case ARM_OP(resulti):
    case ARM_OP(resultf):
    case ARM_OP(resultd):
    case ARM_OP(r0):
    case ARM_OP(r1):
    case ARM_OP(r2):
    case ARM_OP(r3):
    case ARM_OP(r4):
    case ARM_OP(r5):
    case ARM_OP(r6):
    case ARM_OP(r7):
    case ARM_OP(r9):
    case ARM_OP(d0):
    case ARM_OP(d1):
    case ARM_OP(d2):
    case ARM_OP(d3):
    case ARM_OP(d4):
    case ARM_OP(d5):
    case ARM_OP(d6):
    case ARM_OP(d7):
    case ARM_OP(regarg):
    case ARM_OP(ivarreg):
    case ARM_OP(fvarreg):
    case ARM_OP(nrvoval):
    case  ARM_OP(eq):
    case  ARM_OP(ne):
    case  ARM_OP(cs):
    case  ARM_OP(hs):
    case  ARM_OP(cc):
    case  ARM_OP(lo):
    case  ARM_OP(mi):
    case   ARM_OP(pl):
    case   ARM_OP(vs):
    case   ARM_OP(vc):
    case   ARM_OP(hi):
    case   ARM_OP(ls):
    case   ARM_OP(ge):
    case   ARM_OP(lt):
    case   ARM_OP(gt):
    case   ARM_OP(le):
    case   ARM_OP(al):
    case ARM_OP(oplsl):
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
// +------------------------------+  <-- current frame pointer (s0)
// |      saved return address    |
// +------------------------------+
// |      saved frame pointer     |
// +------------------------------+ }-+
// |                              |   | Arguments passed in registers
// |       saved args             |   | that are not assigned to registers
// |                              |   | in the procedure
// +------------------------------+ }-+
// |                              |   | emitter->g->base.stack_frame_size
// |       local variables        |   | bytes long->base.  All local variables
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
static int StackFrameSize(ARMEmitter* emitter) {
  // Start off with local variable space.  This also includes
  // 8 bytes for the saved fp and lr.
  int stack_frame_size = emitter->g->base.stack_frame_size + ARM_STACK_FRAME_HEADER_SIZE;

  bool varargs = emitter->g->base.varargs;

  if (varargs) {
    if (emitter->g->num_int_arg_regs < ARM_NUM_INT_ARGS) {
      // All args other than those declared and in registers must
      // be saved to the stack above the frame pointer and directly
      // under the first pushed arg->base.  This adds to the stack frame size.
      stack_frame_size += (ARM_NUM_INT_ARGS - emitter->g->num_int_arg_regs) * 8;
    }
  }
  // A non-leaf procedure saves register variables on the stack as these
  // will be in saved registers.
  // if (!is_leaf) {
  //  stack_frame_size += emitter->g->base.num_int_reg_vars*8 +
  //  emitter->armnum_fp_reg_vars * 8;
  //}

  stack_frame_size += BitSetCount(&emitter->regs->used_int_regs) * 4;
  stack_frame_size += BitSetCount(&emitter->regs->used_float_regs) * 8;
  stack_frame_size += emitter->spill_region_size;
  
  stack_frame_size = (stack_frame_size + 7) & ~7;  // Aligned to 8 bytes.

  return stack_frame_size;
}

static bool EmptyStackFrame(ARMEmitter* emitter) {
  // A frame is only truly empty when there is nothing to place below the saved
  // fp/lr: no locals/saved-arg slots, no callee-saved register spills and no
  // register-spill region.  Callee-saved registers (used_int_regs /
  // used_float_regs) are stored at sp-relative offsets that assume the body of
  // the frame has been allocated, so a function that uses them needs a real
  // frame even if it has no locals and makes no calls.
  return emitter->g->base.stack_frame_size == 0 &&
         emitter->g->base.num_calls == 0 && !emitter->g->not_leaf &&
         BitSetCount(&emitter->regs->used_int_regs) == 0 &&
         BitSetCount(&emitter->regs->used_float_regs) == 0 &&
         emitter->spill_region_size == 0;
}

static void DecrementStackPointer(ARMEmitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  AddSubImmediate(emitter, "sp",  NULL, /*add=*/false, stack_frame_size, NULL, fp);
}

static COMPILER_UNUSED void IncrementStackPointer(ARMEmitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size <= 0) {
    return;
  }
  AddSubImmediate(emitter, "sp",  NULL, /*add=*/true, stack_frame_size, NULL, fp);
}

// Load a floating point or integer register from the stack frame.
static COMPILER_UNUSED void LoadRegisterFromFrame(ARMEmitter* emitter, int reg,
                                  int offset, bool is_fp,
                                  const char* symbol_name,
                                  FILE* fp) {
  char buf[256];
  const char* instruction = is_fp ? "vldr" : "ldr";
  ARMRegisterType reg_type = is_fp ? kARMRegTypeFloat : kARMRegTypeInt;
  if (offset < 0x7ff) {
    fprintf(fp, "\t%s %s, [fp, #-%d]",
            instruction,
            ARMRegisterNameFromNum(reg,
                                  reg_type, is_fp ? kSize32Bit : kSize32Bit, buf, sizeof(buf)),
            offset);
  } else {
    MoveImmediate(emitter, "ip", offset, fp);
    fprintf(fp, "\t%s %s, [fp, ip]", instruction,
    ARMRegisterNameFromNum(reg,
                          reg_type, is_fp ? kSize32Bit : kSize32Bit, buf, sizeof(buf)));
  }
  fprintf(fp, "\t\t// %s\n", symbol_name);
}

static COMPILER_UNUSED void GenerateOffsetFromFrame(ARMEmitter* emitter, int reg,
                                    int offset,
                                    const char* symbol_name,
                                    FILE* fp) {
  char buf[256];
  AddSubImmediate(emitter, ARMRegisterNameFromNum(reg,
                                                    kARMRegTypeInt, kSize32Bit, buf, sizeof(buf)),
                    "fp", /*add=*/false, offset, symbol_name, fp);

}


// Emit a block data transfer of the given integer registers
// using base as the address register.  Registers are listed in ascending
// number, which is the order the hardware (and the interpreter) transfers them
// to ascending addresses.
static void EmitIntRegBlock(FILE* fp, const char* op, const char* base,
                            const int* regs, int n) {
  char buf[8];
  fprintf(fp, "\t%s %s, {", op, base);
  for (int i = 0; i < n; i++) {
    fprintf(fp, "%s%s", i ? ", " : "",
            ARMRegisterNameFromNum(regs[i], kARMRegTypeInt, kSize32Bit, buf,
                                   sizeof(buf)));
  }
  fprintf(fp, "}\n");
}

static int CombinedSavedIntArgCount(ARMEmitter* emitter) {
  if (emitter->g->base.varargs) {
    return 0;
  }
  bool seen[ARM_NUM_INT_ARGS] = {false};
  int max_reg = -1;
  for (size_t i = 0; i < emitter->g->saved_regs.length; i++) {
    SavedArgumentRegister* saved = emitter->g->saved_regs.value.p[i];
    if (saved->is_fp || saved->base_reg_num != ARM_FP_REG ||
        saved->reg_num < 0 || saved->reg_num >= ARM_NUM_INT_ARGS) {
      continue;
    }
    seen[saved->reg_num] = true;
    if (saved->reg_num > max_reg) {
      max_reg = saved->reg_num;
    }
  }
  if (max_reg < 0) {
    return 0;
  }
  int count = max_reg + 1;
  int base = -(count + 2) * 4;
  for (int reg = 0; reg < count; reg++) {
    if (!seen[reg]) {
      return 0;
    }
    bool found = false;
    for (size_t i = 0; i < emitter->g->saved_regs.length; i++) {
      SavedArgumentRegister* saved = emitter->g->saved_regs.value.p[i];
      if (!saved->is_fp && saved->base_reg_num == ARM_FP_REG &&
          saved->reg_num == reg && saved->offset == base + reg * 4) {
        found = true;
        break;
      }
    }
    if (!found) {
      return 0;
    }
  }
  return count;
}

static bool IsCombinedSavedIntArg(SavedArgumentRegister* saved, int count) {
  return count > 0 && !saved->is_fp && saved->base_reg_num == ARM_FP_REG &&
         saved->reg_num >= 0 && saved->reg_num < count &&
         saved->offset == -(count + 2) * 4 + saved->reg_num * 4;
}

// Save all used registers on the stack.
static void SaveRegisters(ARMEmitter* emitter, FILE* fp) {
  // Entry sequence:
  // LET S = stack frame size + 8 (for frame pointer save)
  // sub sp, sp, #S
  // stp x29,x30, [sp, #-16]
  // add x29, sp, #S-16
 
  int stack_frame_size = StackFrameSize(emitter);

  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
          !emitter->g->not_leaf;
  bool varargs = emitter->g->base.varargs;
  int combined_int_args = CombinedSavedIntArgCount(emitter);
  int combined_int_arg_bytes = combined_int_args * 4;
  // The variadic register save area holds all four core argument registers in
  // contiguous 4-byte slots so va_arg can walk them like a packed argument
  // list (honouring 8-byte alignment for double / long long).
  int space_above_frame_pointer = varargs ? ARM_NUM_INT_ARGS * 4 : 0;

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
  // 16 bytes for the saved x29 and x30.  There is one saved argument
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
  //      |      saved lr           |
  // -8   +-------------------------+         +72
  //      |      saved x29          |
  // -16  +-------------------------+         +64
  //      |      saved x0           |
  // -24  +-------------------------+         +56
  //      |                         |
  //      |      local vars         | { 24 bytes
  //      |                         |
  // -48  +-------------------------+         +32
  //      |      saved x19          |
  // -56  +-------------------------+         +24
  //      |      saved x20          |
  // -64  +-------------------------+         +16
  //      |      saved x21          |
  // -72  +-------------------------+         +8
  //      |      saved x22          |
  // -80  +-------------------------+ <-- sp  +0
  //
  // If this is a varargs procedure we save all the integer
  // registers not declared as arguments on the stack above the
  // new frame pointer (s0).  This adjusts all the offets from the
  // stack pointer by the number of registers saved * 8.

  // Frame pointer and return address are positive offsets from the
  // decremented stack pointer.

  // Local vars are referenced as a negative offset from s0 and are immediately
  // below the saved argument registers.  This is the low address of the
  // start of the local variable region on the stack.
  int local_vars = emitter->g->base.stack_frame_size +
                     ARM_STACK_FRAME_HEADER_SIZE;

  int used_int_count = BitSetCount(&emitter->regs->used_int_regs);
  int first_saved_slot_size = used_int_count > 0 ? 4 : 8;

  // Offset from sp of first saved register.
  int saved_reg_offset = stack_frame_size - ARM_STACK_FRAME_HEADER_SIZE - 8 -
                          emitter->g->base.stack_frame_size -
                          space_above_frame_pointer -
                          emitter->spill_region_size +
                          (8 - first_saved_slot_size);  // First saved register.

  // A leaf procedure doesn't save the return address.
  if (is_leaf) {
    saved_reg_offset += 8;
  }

  if (EmptyStackFrame(emitter)) {
    if (!is_leaf) {
      int regs[] = {ARM_LR_REG};
      EmitIntRegBlock(fp, "stmdb", "sp!", regs, 1);
    }
  } else {
    if (varargs && space_above_frame_pointer > 0) {
      // Reserve the variadic register save area immediately below the caller's
      // stack arguments (i.e. above the saved fp/lr) and spill ALL four core
      // argument registers into contiguous 4-byte slots (r0@0, r1@4, r2@8,
      // r3@12).  Saving the full register file - including the named registers -
      // means the save area mirrors the abstract argument layout starting at r0,
      // so va_arg can apply the AAPCS 8-byte alignment rule (for double / long
      // long) relative to fp.  fp is set to the base of this region (see "add
      // fp, sp" below), i.e. it points at the r0 slot; va_start skips the named
      // registers by adding num_int_arg_regs*4.
      fprintf(fp, "\t// varargs function with %d declared args\n",
              emitter->g->num_int_arg_regs);
      DecrementStackPointer(emitter, space_above_frame_pointer, fp);
      // r0..r3 occupy contiguous 4-byte slots at [sp,#0..12]; store them with a
      // single increment-after block transfer instead of four str.
      int arg_regs[ARM_NUM_INT_ARGS];
      for (int i = 0; i < ARM_NUM_INT_ARGS; i++) {
        arg_regs[i] = i;
      }
      EmitIntRegBlock(fp, "stmia", "sp", arg_regs, ARM_NUM_INT_ARGS);
    }
    if (combined_int_args > 0) {
      int regs[ARM_NUM_INT_ARGS + 2];
      for (int i = 0; i < combined_int_args; i++) {
        regs[i] = i;
      }
      regs[combined_int_args] = ARM_FP_REG;
      regs[combined_int_args + 1] = ARM_LR_REG;
      EmitIntRegBlock(fp, "stmdb", "sp!", regs, combined_int_args + 2);
    } else {
      int regs[] = {ARM_FP_REG, ARM_LR_REG};
      EmitIntRegBlock(fp, "stmdb", "sp!", regs, 2);
    }
    // Point fp at the slot just above the saved fp/lr, so the 8-byte header
    // occupies offsets [-8,-1] from fp.  Every offset convention in this file
    // (saved-arg base of -16, local vars at var_offset - stack_frame_size -
    // HEADER, the callee-saved offset, etc.) assumes the header sits below fp.
    // Using "add fp, sp, #0" would place fp at the saved fp, pushing every
    // fp-relative region down by 8 bytes and making the lowest saved-arg slot
    // overlap the top callee-saved spill slot.  For a varargs function this also
    // lands fp at the base of the register save area reserved above.
    fprintf(fp, "\tadd fp, sp, #%d\n",
            ARM_STACK_FRAME_HEADER_SIZE + combined_int_arg_bytes);
    DecrementStackPointer(emitter,
                          stack_frame_size - ARM_STACK_FRAME_HEADER_SIZE -
                              space_above_frame_pointer -
                              combined_int_arg_bytes,
                          fp);
  }

  char buf1[8], buf2[8];


  if (emitter->g->saved_regs.length > 0) {
    fprintf(fp, "\t// Saved argument registers.\n");
  }
  for (size_t i = 0; i < emitter->g->saved_regs.length; i++) {
    SavedArgumentRegister* saved_reg = emitter->g->saved_regs.value.p[i];
    if (IsCombinedSavedIntArg(saved_reg, combined_int_args)) {
      continue;
    }
    int offset = saved_reg->offset;
    ARMRegisterType reg_type =
        saved_reg->is_fp ? kARMRegTypeFloat : kARMRegTypeInt;
    int saved_reg_size =
        (saved_reg->is_fp && saved_reg->is_double) ? kSize64Bit : kSize32Bit;
    fprintf(fp, "\t%s %s, [%s, #%d]\n",
            saved_reg->is_fp ? "vstr" : "str",
            ARMRegisterNameFromNum(saved_reg->reg_num, reg_type,
                                  saved_reg_size,
                                  buf1, sizeof(buf1)),
            ARMRegisterNameFromNum(saved_reg->base_reg_num, kARMRegTypeInt, kSize32Bit,
                                  buf2, sizeof(buf2)),
            offset);
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
  

  // Record offset for last saved register for reloading->base.
  emitter->saved_reg_offset = saved_reg_offset;
  
  BitSetIterator it;

  // Collect the used integer callee-saved registers (ascending register
  // number).  When there are at least three, store them with a single block
  // transfer.  Integer registers are 32-bit on ARM, so the save area uses
  // a 4-byte stride (lowest register at the lowest address, matching
  // stmia/ldmia and the interpreter).
  int int_regs[16];
  int num_int = 0;
  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int_regs[num_int++] = (int)BitSetIteratorValue(&it);
    BitSetIteratorNext(&it);
  }
  if (num_int > 0) {
    fprintf(fp, "\t// Saved integer registers.\n");
    if (num_int >= 3) {
      int base_low = saved_reg_offset - 4 * (num_int - 1);
      const char* base = "sp";
      if (base_low != 0) {
        AddSubImmediate(emitter, "ip", "sp", /*add=*/true, base_low, NULL, fp);
        base = "ip";
      }
      EmitIntRegBlock(fp, "stmia", base, int_regs, num_int);
    } else {
      int off = saved_reg_offset;
      for (int i = 0; i < num_int; i++) {
        fprintf(fp, "\tstr %s, [sp, #%d]\n",
                ARMRegisterNameFromNum(int_regs[i], kARMRegTypeInt, kSize32Bit,
                                       buf1, sizeof(buf1)),
                off);
        off -= 4;
      }
    }
    saved_reg_offset -= 4 * num_int;
  }
  
  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  if (!BitSetIteratorDone(&it)) {
    fprintf(fp, "\t// Saved floating point registers.\n");
  }
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    int offset = saved_reg_offset;
    saved_reg_offset -= 8;
    fprintf(fp, "\tvstr %s, [sp, #%d]\n",
            ARMRegisterNameFromNum(reg, kARMRegTypeFloat, kSize64Bit, buf1, sizeof(buf1)),
            offset);
    BitSetIteratorNext(&it);
  }

  if (!is_leaf) {
    fprintf(fp, "\t// End of stack frame\n");
  }
}

static void RestoreRegisters(ARMEmitter* emitter, FILE* fp) {
  // Exit sequence:
  // ldr s0, S-8(sp)   - restore frame pointer.
  // addi sp, sp, S   - increment sp

  int stack_frame_size = StackFrameSize(emitter);
  char buf1[8];

  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
          !emitter->g->not_leaf;
  bool varargs = emitter->g->base.varargs;
  int combined_int_args = CombinedSavedIntArgCount(emitter);
  int combined_int_arg_bytes = combined_int_args * 4;
  // The variadic register save area holds all four core argument registers in
  // contiguous 4-byte slots so va_arg can walk them like a packed argument
  // list (honouring 8-byte alignment for double / long long).
  int space_above_frame_pointer = varargs ? ARM_NUM_INT_ARGS * 4 : 0;

  if (space_above_frame_pointer < 0) {
    space_above_frame_pointer = 0;
  }

  // A leaf procedure doesn't save the return address.
  if (!is_leaf) {
    fprintf(fp, "\t// Restored registers.\n");
  }

  // If the function adjusted sp dynamically (VLA / alloca), sp is no longer a
  // fixed distance from the saved-register area, so the sp-relative reloads and
  // the fixed sp increment below would use the wrong addresses.  Restore sp from
  // the frame pointer to the position it had right after the fixed prologue
  // allocation.  Post-prologue: sp = fp - stack_frame_size + space_above.
  if (emitter->g->uses_dynamic_stack && !EmptyStackFrame(emitter)) {
    AddSubImmediate(emitter, "sp", "fp", /*add=*/false,
                    stack_frame_size - space_above_frame_pointer,
                    "restore sp (dynamic stack)", fp);
  }

  int offset = emitter->saved_reg_offset;

  BitSetIterator it;

  // Restore in the same order and with the same layout SaveRegisters used:
  // integer registers (a single ldmia when there are >= 3) first, then floating
  // point.
  int int_regs[16];
  int num_int = 0;
  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int_regs[num_int++] = (int)BitSetIteratorValue(&it);
    BitSetIteratorNext(&it);
  }
  if (num_int >= 3) {
    int base_low = offset - 4 * (num_int - 1);
    const char* base = "sp";
    if (base_low != 0) {
      AddSubImmediate(emitter, "ip", "sp", /*add=*/true, base_low, NULL, fp);
      base = "ip";
    }
    EmitIntRegBlock(fp, "ldmia", base, int_regs, num_int);
  } else {
    for (int i = 0; i < num_int; i++) {
      fprintf(fp, "\tldr %s, [sp, #%d]\n",
              ARMRegisterNameFromNum(int_regs[i], kARMRegTypeInt, kSize32Bit,
                                     buf1, sizeof(buf1)),
              offset - 4 * i);
    }
  }
  offset -= 4 * num_int;

  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tvldr %s, [sp, #%d]\n",
            ARMRegisterNameFromNum(reg, kARMRegTypeFloat, kSize64Bit, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  if (EmptyStackFrame(emitter)) {
    if (!is_leaf) {
      int regs[] = {ARM_LR_REG};
      EmitIntRegBlock(fp, "ldmia", "sp!", regs, 1);
    }
  } else {
    IncrementStackPointer(emitter,
                          stack_frame_size - ARM_STACK_FRAME_HEADER_SIZE -
                              space_above_frame_pointer -
                              combined_int_arg_bytes,
                          fp);
    if (combined_int_arg_bytes > 0) {
      IncrementStackPointer(emitter, combined_int_arg_bytes, fp);
    }
      int regs[] = {ARM_FP_REG, ARM_LR_REG};
      EmitIntRegBlock(fp, "ldmia", "sp!", regs, 2);
    // Release the variadic register save area reserved above the saved fp/lr.
    if (varargs && space_above_frame_pointer > 0) {
      IncrementStackPointer(emitter, space_above_frame_pointer, fp);
    }
  }
}

// In arm_codegen.c.
extern int ARMGetRegisterSize(TargetInstruction* inst);

static const char* AsmOpcodeName(ARMOpcode op) {
  switch (op) {
    case ARM_OP(fldr): return "vldr";
    case ARM_OP(fstr): return "vstr";
    case ARM_OP(fmov): return "vmov";
    case ARM_OP(fneg): return "vneg";
    case ARM_OP(fsqrt): return "vsqrt";
    case ARM_OP(fadd): return "vadd";
    case ARM_OP(fsub): return "vsub";
    case ARM_OP(fmul): return "vmul";
    case ARM_OP(fdiv): return "vdiv";
    case ARM_OP(fcmp): return "vcmp";
    case ARM_OP(fcvt): return "vmov";
    case ARM_OP(ldur): return "ldr";
    case ARM_OP(stur): return "str";
    case ARM_OP(ldurb): return "ldrb";
    case ARM_OP(sturb): return "strb";
    case ARM_OP(ldurh): return "ldrh";
    case ARM_OP(sturh): return "strh";
    case ARM_OP(ldursb): return "ldrsb";
    case ARM_OP(ldursh): return "ldrsh";
    case ARM_OP(ldursw): return "ldr";
    case ARM_OP(movk): return "movt";
    case ARM_OP(movz): return "movw";
    case ARM_OP(movn): return "mvn";
    default:
      return ARMOpcodeName(op);
  }
}

static const char* LoadStoreMnemonic(ARMOpcode op) {
  switch (op) {
    case ARM_OP(ldrb):
    case ARM_OP(ldurb):
    case ARM_OP(ldursb):
      return "ldrb";
    case ARM_OP(ldrh):
    case ARM_OP(ldurh):
    case ARM_OP(ldursh):
      return "ldrh";
    case ARM_OP(ldrsb):
      return "ldrsb";
    case ARM_OP(ldrsh):
      return "ldrsh";
    case ARM_OP(fldr):
      return "vldr";
    case ARM_OP(strb):
    case ARM_OP(sturb):
      return "strb";
    case ARM_OP(strh):
    case ARM_OP(sturh):
      return "strh";
    case ARM_OP(fstr):
      return "vstr";
    default:
      return AsmOpcodeName(op);
  }
}

static void PrintSymbolOperand(ARMEmitter* emitter, TargetInstruction* inst,
                               TargetInstruction* operand, const char* reg,
                               bool is_mov, FILE* fp) {
  (void)emitter;
  char symbuf[256];
  const char* sym =
      TargetSymbolName(((TargetSymbol*)operand)->symbol, symbuf, sizeof(symbuf));
  if ((inst->flags & ARM_HI_RELOC) != 0 ||
      (inst->flags & ARM_PCREL_HI_RELOC) != 0) {
    fprintf(fp, "\tmovw %s, %s\n", reg, sym);
  } else if ((inst->flags & ARM_LO_RELOC) != 0 ||
             (inst->flags & ARM_PCREL_LO_RELOC) != 0) {
    if (is_mov) {
      fprintf(fp, "\tmovt %s, %s\n", reg, sym);
    } else {
      fprintf(fp, "\tadd %s, %s, %s\n", reg, reg, sym);
    }
  } else {
    fprintf(fp, "\tldr %s, =%s\n", reg, sym);
  }
}

// The rmov instructions are an explicit mov from operand[1] to
// operand[0].  Both are registers.
static void PrintRmov(ARMEmitter* emitter, TargetInstruction* inst, FILE* fp) {
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

  const char* mnemonic = "";
  int rmov_size = ARMGetRegisterSize(inst);
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(mv):
      mnemonic = "mov";
      break;
    case ARM_OP(fmv_s):
      mnemonic = "vmov.f32";
      break;
    case ARM_OP(fmv_d):
      mnemonic = "vmov.f64";
      break;
    case ARM_OP(fmov):
      mnemonic = (rmov_size == kSize64Bit) ? "vmov.f64" : "vmov.f32";
      break;
    case ARM_OP(mov):
      mnemonic = "mov";
      break;

    default:
      assert(false);
  }
  char buf1[8], buf2[8];
  fprintf(
      fp, "\t%-12s%s, %s\n", mnemonic,
      ARMRegisterName((ARMRegister*)inst->operand[0]->reg, ARMGetRegisterSize(inst), buf1, sizeof(buf1)),
      ARMRegisterName((ARMRegister*)inst->operand[1]->reg, ARMGetRegisterSize(inst), buf2, sizeof(buf2)));
}

static const char* GetRegisterName(TargetInstruction* inst, int size, char* buf, size_t bufsize) {
  if (inst->dest != NULL) {
    return ARMRegisterName((ARMRegister*)inst->dest->reg, size, buf, bufsize);
  }
  return ARMRegisterName((ARMRegister*)inst->reg, size, buf, bufsize);
}

// Main instruction printer.
static void PrintInstruction(ARMEmitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    TargetBasicBlock* b = inst->block;
    fprintf(fp, "\n\t// *** Basic block %zd\n\n", b->block_id);
    emitter->current_block = inst->block;
  }
  if (((int)inst->opcode == (int)ARM_OP(label))) {
    if ((inst->flags & ARM_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    return;
  }

  if (((int)inst->opcode == (int)ARM_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }
  
  if (((int)inst->opcode == (int)ARM_OP(ivarreg)) || ((int)inst->opcode == (int)ARM_OP(fvarreg))) {
    return;
  }
  if (inst->id == 7) {
    printf("");
  }
  if (!IsPrintable(inst)) {
    return;
  }
  
  const bool show_id = 1;

  if (show_id) {
    fprintf(fp, "/* @%d */ ", inst->id);
  }
  
  // Buffers for register name printing->base.
  char buf1[8];
  char buf2[8];

  int reg_size = ARMGetRegisterSize(inst);
  
  // Special case instructions.
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(mv):
      // Don't emit mv x, x.
      if (inst->operand[0]->reg == inst->reg) {
        return;
      }
      if (inst->dest != NULL) {
        PrintRmov(emitter, inst, fp);
        return;
      }
      break;
    case ARM_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      char symbuf[256];
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n",
                TargetSymbolName(sym->symbol, symbuf, sizeof(symbuf)));
      } else {
        fprintf(fp, "\t.global %s\n",
                TargetSymbolName(sym->symbol, symbuf, sizeof(symbuf)));
      }
      return;
    }
    // case ARM_OP(bl):
    case ARM_OP(bl): {
      assert(((int)inst->operand[0]->opcode == (int)ARM_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      char symbuf[256];
      fprintf(fp, "\t%-12s%s\n", "bl",
              TargetSymbolName(sym->symbol, symbuf, sizeof(symbuf)));
      return;
    }

#if 0
    case ARM_OP(rcall):
    case ARM_OP(rcallf):
      fprintf(fp, "\t%-12s x1, %s, 0\n", "jalr",
              GetRegisterName(inst->operand[0], buf2,
                             sizeof(buf2)));

      return;
#endif
    case ARM_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case ARM_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case ARM_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t%s\n", lit->value.value);
      lit->base.disabled = true;
      return;
    }

    case ARM_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }

    case ARM_OP(spill): {
      // Store the spilled register's value into its slot in the spill region
      // (a negative offset from the frame pointer).  The matching reload below
      // reads it back from the same [fp, #-offset] location.  Without this store
      // the reload would read an uninitialized stack slot.
      ARMRegister* reg = (ARMRegister*)inst->reg;
      int offset =
          (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      const char* store = reg->type == kARMRegTypeInt ? "str" : "vstr";
      fprintf(fp, "\t%s %s, [fp, #-%d]\t// Spilled @%d\n", store,
              ARMRegisterName(reg, kSize32Bit, buf1, sizeof(buf1)), offset,
              inst->operand[0]->id);
      return;
    }
      
    case ARM_OP(reload): {
      ARMRegister* reg = (ARMRegister*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      const char* load = reg->type == kARMRegTypeInt ? "ldr" : "vldr";
      fprintf(fp, "\t%s %s, [fp, #-%d]\t// Reloaded spilled @%d\n",
              load,
              ARMRegisterName(reg, reg->type == kARMRegTypeInt ? kSize32Bit : kSize32Bit,
                              buf1, sizeof(buf1)),
              offset,
              spill->operand[0]->id);
      return;
    }

    case ARM_OP(ret):
      fprintf(fp, "\tbx lr\n");
      return;

    case ARM_OP(adrp): {
      const char* dest = GetRegisterName(inst, kSize32Bit, buf1, sizeof(buf1));
      if (inst->operand[0] != NULL &&
          ((int)inst->operand[0]->opcode == (int)ARM_OP(symbol))) {
        PrintSymbolOperand(emitter, inst, inst->operand[0], dest, true, fp);
      } else if (inst->operand[0] != NULL &&
                 ((int)inst->operand[0]->opcode == (int)ARM_OP(literal))) {
        TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
        fprintf(fp, "\tldr %s, =.str.%d\n", dest, literal->literal_id);
      }
      return;
    }

    case ARM_OP(adr): {
      const char* dest = GetRegisterName(inst, kSize32Bit, buf1, sizeof(buf1));
      if (inst->operand[0] != NULL &&
          ((int)inst->operand[0]->opcode == (int)ARM_OP(literal))) {
        TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
        fprintf(fp, "\tldr %s, =.str.%d\n", dest, literal->literal_id);
        return;
      }
      break;
    }

    case ARM_OP(csel): {
      const char* dest = GetRegisterName(inst, reg_size, buf1, sizeof(buf1));
      const char* op1 = GetRegisterName(inst->operand[0], reg_size, buf2, sizeof(buf2));
      const char* op2 = GetRegisterName(inst->operand[1], reg_size, buf2, sizeof(buf2));
      const char* cond = ARMOpcodeName(inst->operand[2]->opcode);
      fprintf(fp, "\tmov %s, %s\n", dest, op2);
      fprintf(fp, "\tmov%s %s, %s\n", cond, dest, op1);
      return;
    }

    case ARM_OP(cbz):
    case ARM_OP(cbnz): {
      const char* reg = GetRegisterName(inst->operand[0], reg_size, buf1, sizeof(buf1));
      const char* cond = (ARMOpcode)inst->opcode == ARM_OP(cbz) ? "eq" : "ne";
      fprintf(fp, "\tcmp %s, #0\n", reg);
      fprintf(fp, "\tb%s .%s_label_%d\n", cond, func_name, inst->operand[1]->id);
      return;
    }

    case ARM_OP(stp):
    case ARM_OP(ldp): {
      bool is_load = (ARMOpcode)inst->opcode == ARM_OP(ldp);
      const char* op0 = GetRegisterName(inst->operand[0], reg_size, buf1, sizeof(buf1));
      const char* op1 = GetRegisterName(inst->operand[1], reg_size, buf2, sizeof(buf2));
      const char* base = GetRegisterName(inst->operand[2], kSize32Bit, buf2, sizeof(buf2));
      int offset = TargetIsConst(inst->operand[3]) ? (int)TargetIntValue(inst->operand[3]) : 0;
      fprintf(fp, "\t%s %s, [%s, #%d]\n", is_load ? "ldr" : "str", op0, base, offset);
      fprintf(fp, "\t%s %s, [%s, #%d]\n", is_load ? "ldr" : "str", op1, base, offset + 4);
      return;
    }

    default:
      break;
  }

  // General case for instruction printing->base.

  TrapInstruction(inst);

  // Print opcode.
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(b):
      fprintf(fp, "\tb");
      break;
    case ARM_OP(mov):
    case ARM_OP(movz):
    case ARM_OP(movk):
    case ARM_OP(movn):
    case ARM_OP(blr):
    case ARM_OP(br):
    case ARM_OP(movw):
    case ARM_OP(movt):
    case ARM_OP(cset):
    case ARM_OP(csetm):
    case ARM_OP(adr):
      break;
    case ARM_OP(fadd):
    case ARM_OP(fsub):
    case ARM_OP(fmul):
    case ARM_OP(fdiv):
    case ARM_OP(fcmp):
    case ARM_OP(fmov):
    case ARM_OP(fneg):
    case ARM_OP(fsqrt): {
      // Floating-point data-processing ops need an explicit precision suffix so
      // the assembler selects the f32 vs f64 encoding.  The operand register
      // names (sN vs dN) are driven by the same instruction size.
      const char* base = AsmOpcodeName((ARMOpcode)inst->opcode);
      const char* suffix = (reg_size == kSize64Bit) ? ".f64" : ".f32";
      char mnem[24];
      snprintf(mnem, sizeof(mnem), "%s%s", base, suffix);
      fprintf(fp, "\t%-12s", mnem);
      break;
    }
    default:
      fprintf(fp, "\t%-12s", LoadStoreMnemonic((ARMOpcode)inst->opcode));
      break;
  }

  // Print operands.
  switch ((ARMOpcode)inst->opcode) {
    case ARM_OP(fcvtsd):
      // fcvtsd Dd, Sm : destination is double, source is single.
      fprintf(fp, "%s, %s\n",
              GetRegisterName(inst, kSize64Bit, buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[0], kSize32Bit, buf2,
                             sizeof(buf2)));
      return;
    case ARM_OP(fcvtds):
      // fcvtds Sd, Dm : destination is single, source is double.
      fprintf(fp, "%s, %s\n",
              GetRegisterName(inst, kSize32Bit, buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[0], kSize64Bit, buf2,
                             sizeof(buf2)));
      return;
    case  ARM_OP(ldr):
    case   ARM_OP(ldur):
    case   ARM_OP(ldrb):
    case   ARM_OP(ldrh):
    case   ARM_OP(ldurb):
    case   ARM_OP(ldurh):
    case   ARM_OP(ldrsb):
    case   ARM_OP(ldrsh):
    case   ARM_OP(ldursb):
    case   ARM_OP(ldursh):
    case   ARM_OP(ldursw):
    case ARM_OP(fldr):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      assert(inst->operand[0]->reg != NULL);
      if (TargetIsConst(inst->operand[1])) {
        int offset = (int)TargetIntValue(inst->operand[1]);
        assert(ARMIsPossibleImmediate(offset));
        fprintf(fp, "%s, [%s, #%d]\n",
                GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[0], kSize32Bit, buf2,
                               sizeof(buf2)), offset);
      } else if (((int)inst->operand[1]->opcode == (int)ARM_OP(symbol))) {
        PrintSymbolOperand(emitter, inst, inst->operand[1],
                           GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
                           false, fp);
      } else if (((int)inst->operand[1]->opcode == (int)ARM_OP(label))) {
        assert((inst->flags & ARM_PCREL_LO_RELOC) != 0);
        fprintf(fp, "\tadd %s, %s, .%s_label_%d\n",
                GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[0], kSize32Bit, buf2, sizeof(buf2)),
                func_name, inst->operand[1]->id);
      } else if (((int)inst->operand[1]->opcode == (int)ARM_OP(zr))) {
        fprintf(fp, "%s, [%s, #0]\n",
                GetRegisterName(inst, reg_size,buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[0], reg_size,buf2,
                               sizeof(buf2)));
      } else {
        assert(false);
      }
      break;

    case ARM_OP(str):
    case ARM_OP(stur):
    case ARM_OP(strb):
    case ARM_OP(strh):
    case ARM_OP(sturb):
    case ARM_OP(sturh):
    case ARM_OP(fstr):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      assert(inst->operand[0]->reg != NULL);
      assert(inst->operand[1]->reg != NULL);
      if (TargetIsConst(inst->operand[2])) {
        int offset = (int)TargetIntValue(inst->operand[2]);
        assert(ARMIsPossibleImmediate(offset));
        fprintf(fp, "%s, [%s, #%d]\n",
                GetRegisterName(inst->operand[0], reg_size, buf1,
                               sizeof(buf1)),
                GetRegisterName(inst->operand[1], kSize32Bit, buf2,
                               sizeof(buf2)), offset);
      } else if (((int)inst->operand[2]->opcode == (int)ARM_OP(symbol))) {
        PrintSymbolOperand(emitter, inst, inst->operand[2],
                           GetRegisterName(inst->operand[0], reg_size, buf1,
                                          sizeof(buf1)),
                           false, fp);
      } else if (((int)inst->operand[2]->opcode == (int)ARM_OP(label))) {
        assert((inst->flags & ARM_PCREL_LO_RELOC) != 0);
        fprintf(fp, "\tadd %s, %s, .%s_label_%d\n",
                GetRegisterName(inst->operand[0], reg_size, buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[1], kSize32Bit, buf2, sizeof(buf2)),
                func_name, inst->operand[2]->id);
      } else if (((int)inst->operand[2]->opcode == (int)ARM_OP(zr))) {
        fprintf(fp, "%s, [%s, #0]\n",
                GetRegisterName(inst->operand[0], reg_size,buf1,
                               sizeof(buf1)),
                GetRegisterName(inst->operand[1], reg_size,buf2,
                               sizeof(buf2)));

      } else {
        assert(false);
      }
      break;
      
    case ARM_OP(nop):
      fprintf(fp, "\n");
      break;
      
    case ARM_OP(b): {
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      TargetInstruction* cond = inst->operand[0];
      TargetInstruction* dest = inst->operand[1];
      if (((int)cond->opcode != (int)ARM_OP(al))) {
        // ARM conditional branches are written "bge", not "b.ge".
        fprintf(fp, "%s", ARMOpcodeName(cond->opcode));
      }
      fprintf(fp, " ");
      if (((int)dest->opcode == (int)ARM_OP(label))) {
        fprintf(fp, ".%s_label_%d\n", func_name, dest->id);
      } else if (((int)dest->opcode == (int)ARM_OP(symbol))) {
        char symbuf[256];
        fprintf(fp, "%s\n", TargetSymbolName(((TargetSymbol*)dest)->symbol,
                                             symbuf, sizeof(symbuf)));
      } else if (((int)dest->opcode == (int)ARM_OP(named_label))) {
        fprintf(fp, "%s\n", ((TargetNamedLabel*)dest)->name);
      } else {
        assert(false);
      }
      return;
    }
      
    case ARM_OP(br):
      fprintf(fp, "\tbx %s\n",
              GetRegisterName(inst->operand[0], kSize32Bit, buf1,
                             sizeof(buf1)));
      break;

    case ARM_OP(cset):
    case ARM_OP(csetm): {
      assert(inst->operand[0] != NULL);
      const char* dest = GetRegisterName(inst, reg_size, buf1, sizeof(buf1));
      const char* cond = ARMOpcodeName(inst->operand[0]->opcode);
      fprintf(fp, "\tmov %s, #0\n", dest);
      fprintf(fp, "\tmov%s %s, #1\n", cond, dest);
      break;
    }
      
    case ARM_OP(blr):
      assert(inst->operand[0] != NULL);
      fprintf(fp, "\tblx %s\n",
              GetRegisterName(inst->operand[0], reg_size, buf1,
                             sizeof(buf1)));
      break;

    case ARM_OP(movw):
    case ARM_OP(movt): {
      TargetInstruction* sym = inst->operand[0];
      if ((ARMOpcode)inst->opcode == ARM_OP(movt)) {
        sym = inst->operand[1];
      }
      fprintf(fp, "\t%-12s%s, ", ARMOpcodeName(inst->opcode),
              GetRegisterName(inst, reg_size, buf1, sizeof(buf1)));
      if (((int)sym->opcode == (int)ARM_OP(symbol))) {
        char symbuf[256];
        fprintf(fp, "%s\n", TargetSymbolName(((TargetSymbol*)sym)->symbol,
                                             symbuf, sizeof(symbuf)));
      } else if (((int)sym->opcode == (int)ARM_OP(literal))) {
        fprintf(fp, ".str.%d\n", ((TargetLiteral*)sym)->literal_id);
      } else {
        assert(false);
      }
      break;
    }

    case ARM_OP(adr):
      fprintf(fp, "\tadd %s, pc, #%" PRId64 "\n",
              GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
              TargetIntValue(inst->operand[0]));
      break;

    case ARM_OP(mov):
      if (TargetIsConst(inst->operand[0])) {
        MoveImmediate(emitter, GetRegisterName(inst, reg_size, buf1,sizeof(buf1)), TargetIntValue(inst->operand[0]), fp);
        break;
      }
      fprintf(fp, "\t%-12s", "mov");

      // Fall Through.
      
    default: {
      const char* sep = "";
      if (inst->reg != NULL) {
        fprintf(fp, "%s",
                GetRegisterName(inst, reg_size, buf1,sizeof(buf1)));
        sep = ", ";
      }
      for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
        if (inst->operand[i] != NULL) {
          if (TargetIsConst(inst->operand[i])) {
            fprintf(fp, "%s#%" PRId64 "", sep, TargetIntValue(inst->operand[i]));
          } else if (((int)inst->operand[i]->opcode == (int)ARM_OP(symbol))) {
            char symbuf[256];
            const char* opsym = TargetSymbolName(
                ((TargetSymbol*)inst->operand[i])->symbol, symbuf,
                sizeof(symbuf));
            if ((inst->flags & ARM_HI_RELOC) != 0 ||
                (inst->flags & ARM_PCREL_HI_RELOC) != 0) {
              fprintf(fp, "%s%s", sep, opsym);
            } else if ((inst->flags & ARM_LO_RELOC) != 0 ||
                       (inst->flags & ARM_PCREL_LO_RELOC) != 0) {
              fprintf(fp, "%s%s", sep, opsym);
            } else {
              fprintf(fp, "%s=%s", sep, opsym);
            }
          } else if (((int)inst->operand[i]->opcode == (int)ARM_OP(literal))) {
            TargetLiteral* literal = (TargetLiteral*)inst->operand[i];
            fprintf(fp, "%s.str.%d", sep, literal->literal_id);
          } else if (((int)inst->operand[i]->opcode == (int)ARM_OP(oplsl))) {
            fprintf(fp, ", lsl ");
            sep = "";
            continue;
          } else {
            fprintf(fp, "%s%s", sep,
                    GetRegisterName(inst->operand[i], reg_size,buf2,
                                   sizeof(buf2)));
          }
          sep = ", ";
        }
      }
      fprintf(fp, "\n");
    }
  }
  
}

void ARMEmitterInit(ARMEmitter* emitter, ARMGenerator* g) {
  emitter->g = g;
  emitter->regs = &g->register_allocator;
  emitter->saved_reg_offset = 0;
  emitter->spill_region_size = g->register_allocator.max_spilled_region_size;
  emitter->first_spill_offset = 0;
  emitter->current_block = NULL;
}

ARMEmitter* NewARMEmitter(ARMGenerator* rv) {
  ARMEmitter* emitter = malloc(sizeof(ARMEmitter));
  ARMEmitterInit(emitter, rv);
  return emitter;
}

void ARMEmitterDestruct(ARMEmitter* emitter) {
}

void ARMEmitterDelete(ARMEmitter* emitter) {
  ARMEmitterDestruct(emitter);
  free(emitter);
}


void ARMPrintFunction(ARMEmitter* emitter, FILE* fp) {
  const char* func_name = emitter->g->base.function_name.value;
  if (emitter->g->base.is_global) {
    fprintf(fp, "\t.global %s\n", func_name);
  } else {
    fprintf(fp, "\t.local  %s\n", func_name);
  }
  fprintf(fp, "\t.type %s, @function\n\n", func_name);
  fprintf(fp, "%s:\n", func_name);
  TargetInstruction* inst = TargetFirstInstruction(&emitter->g->base);
  while (inst != NULL) {
    PrintInstruction(emitter, inst, func_name, fp);
    inst = TargetNext(inst);
  }
  fprintf(fp, ".func_end_%s:\n", func_name);
  fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
}

