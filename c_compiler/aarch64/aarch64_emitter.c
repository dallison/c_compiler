//
//  aarch64_emitter.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "aarch64_emitter.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "common_emitter.h"
#include "compiler.h"
#include "eh_metadata.h"
#include "aarch64_assembler.h"
#include "aarch64_encode.h"
#include "aarch64_codegen.h"
#include "aarch64_program.h"
#include "aarch64_reg_alloc.h"
#include "target_basic_block.h"

static void Trap() {}

static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 21) {
    Trap();
  }
}

#if 1
// Build a MOV instruction out of MOVZ, MOVN and MOVK, based on the immediate
// value.
// A MOVZ, MOVN and MOVK instruction can take 16 bit immediate value shifted
// left by 0, 16, 32 or 48 (0 or 16 for 32 bit only.
static void MoveImmediate(AARCH64Emitter* emitter,
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
static void MoveImmediate(AARCH64Emitter* emitter,
                          const char* reg,
                          int64_t value, FILE* fp) {
  if (reg[0] == 'w') {
    // Sign extend.
    value = (value << 32) >> 32;
  }
  fprintf(fp, "\tmov %s, #%" PRId64 "\n", reg, value);
}

#endif

// Generate a sequence of add/sub instruction for an immediate.
// The add/sub instructions take a 12 bit immediate shifted left
// by 0 or 12 bits.  If the constant is negative, the add becomes
// a sub and vice versa.
static void AddSubImmediate(AARCH64Emitter* emitter,
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
  const char* tmpreg = destreg[0] == 'x' ? "x8" : "w8";
  if (value > 0xffffff) {
    // Too big for an immediate, put it into a register and use that.
    MoveImmediate(emitter, tmpreg, (uint64_t)value, fp);
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
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(tmp):
    case AARCH64_OP(fp):
    case AARCH64_OP(sp):
    case AARCH64_OP(lr):
    case AARCH64_OP(xr):
    case AARCH64_OP(zr):
    case AARCH64_OP(literal):
    case AARCH64_OP(structreturn):
    case AARCH64_OP(resulti):
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
    case AARCH64_OP(r0):
    case AARCH64_OP(r1):
    case AARCH64_OP(r2):
    case AARCH64_OP(r3):
    case AARCH64_OP(r4):
    case AARCH64_OP(r5):
    case AARCH64_OP(r6):
    case AARCH64_OP(r7):
    case AARCH64_OP(r9):
    case AARCH64_OP(d0):
    case AARCH64_OP(d1):
    case AARCH64_OP(d2):
    case AARCH64_OP(d3):
    case AARCH64_OP(d4):
    case AARCH64_OP(d5):
    case AARCH64_OP(d6):
    case AARCH64_OP(d7):
    case AARCH64_OP(regarg):
    case AARCH64_OP(ivarreg):
    case AARCH64_OP(fvarreg):
    case AARCH64_OP(nrvoval):
    case  AARCH64_OP(eq):
    case  AARCH64_OP(ne):
    case  AARCH64_OP(cs):
    case  AARCH64_OP(hs):
    case  AARCH64_OP(cc):
    case  AARCH64_OP(lo):
    case  AARCH64_OP(mi):
    case   AARCH64_OP(pl):
    case   AARCH64_OP(vs):
    case   AARCH64_OP(vc):
    case   AARCH64_OP(hi):
    case   AARCH64_OP(ls):
    case   AARCH64_OP(ge):
    case   AARCH64_OP(lt):
    case   AARCH64_OP(gt):
    case   AARCH64_OP(le):
    case   AARCH64_OP(al):
    case AARCH64_OP(oplsl):
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
// |      saved return address    |
// +------------------------------+
// |      saved frame pointer     |
// +------------------------------+  <-- established x29
// |                              |
// |      saved varargs regs      |
// |                              |
// +------------------------------+ }-+ <-- effective x29 for varargs functions
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
// All local variables are accessed as a negative offset from x29.
//
// Spilled register values are a negative offset from x29.
// The only potentially large area is the space for
// local variables.  The rest are small and bounded.

// Size of the variadic register save area reserved immediately above the
// frame pointer for a variadic function.  This follows AAPCS64: the unnamed
// general-purpose argument registers (x_named..x7) form the GP save area and
// the unnamed SIMD/FP registers (d_named..d7) form the VR save area, with each
// VR slot occupying 16 bytes.  va_start records pointers/offsets into these two
// regions (plus the on-stack overflow area) in the va_list structure.
static int VarargsGpSaveSize(AARCH64Emitter* emitter) {
  int n = AARCH64_NUM_INT_ARGS - emitter->g->num_int_arg_regs;
  return n > 0 ? n * 8 : 0;
}

static int VarargsVrSaveSize(AARCH64Emitter* emitter) {
  int n = AARCH64_NUM_FP_ARGS - emitter->g->num_fp_arg_regs;
  return n > 0 ? n * 16 : 0;
}

static int VarargsSaveAreaSize(AARCH64Emitter* emitter) {
  if (!emitter->g->base.varargs) {
    return 0;
  }
  return VarargsGpSaveSize(emitter) + VarargsVrSaveSize(emitter);
}

// This is the size of the stack frame including the space
// for the local variables.
static int StackFrameSize(AARCH64Emitter* emitter) {
  // Start off with local variable space.  This also includes
  // 16 bytes for the saved x29 and x30.
  int stack_frame_size = emitter->g->base.stack_frame_size + 16;

  // Reserve the AAPCS64 variadic register save area (GP + VR) above the frame
  // pointer for a variadic function.
  stack_frame_size += VarargsSaveAreaSize(emitter);
  // A non-leaf procedure saves register variables on the stack as these
  // will be in saved registers.
  // if (!is_leaf) {
  //  stack_frame_size += emitter->g->base.num_int_reg_vars*8 +
  //  emitter->aarch64num_fp_reg_vars * 8;
  //}

  stack_frame_size += BitSetCount(&emitter->regs->used_int_regs) * 8;
  stack_frame_size += BitSetCount(&emitter->regs->used_float_regs) * 8;
  stack_frame_size += emitter->spill_region_size;
  
  stack_frame_size = (stack_frame_size + 15) & ~15;  // Aligned to 16 bytes.

  return stack_frame_size;
}

static bool EmptyStackFrame(AARCH64Emitter* emitter) {
  // A frame is only truly empty when there is nothing to store on the stack:
  // no locals, no calls (so no saved x29/x30), and no callee-saved or spilled
  // registers.  If any registers must be saved we still need to allocate a
  // frame; otherwise the save/restore stores would write above sp (the stack
  // top) into unmapped memory.
  return emitter->g->base.stack_frame_size == 0 &&
         emitter->g->base.num_calls == 0 && !emitter->g->not_leaf &&
         BitSetCount(&emitter->regs->used_int_regs) == 0 &&
         BitSetCount(&emitter->regs->used_float_regs) == 0 &&
         emitter->spill_region_size == 0;
}

static void DecrementStackPointer(AARCH64Emitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  AddSubImmediate(emitter, "sp",  NULL, /*add=*/false, stack_frame_size, NULL, fp);
}

static COMPILER_UNUSED void IncrementStackPointer(AARCH64Emitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size <= 0) {
    return;
  }
  AddSubImmediate(emitter, "sp",  NULL, /*add=*/true, stack_frame_size, NULL, fp);
}

// Load a floating point or integer register from the stack frame.
static COMPILER_UNUSED void LoadRegisterFromFrame(AARCH64Emitter* emitter, int reg,
                                  int offset, bool is_fp,
                                  const char* symbol_name,
                                  FILE* fp) {
  char buf[256];
  const char* instruction = is_fp ? "fldr" : "ldr";
  AARCH64RegisterType reg_type = is_fp ? kAARCH64RegTypeFloat : kAARCH64RegTypeInt;
  if (offset < 0x7ff) {
    fprintf(fp, "\tf%s %s, [x29, #-%d",
            instruction,
            AARCH64RegisterNameFromNum(reg,
                                  reg_type, kSize64Bit, buf, sizeof(buf)),
            offset);
  } else {
    MoveImmediate(emitter, "x9", offset, fp);
    fprintf(fp, "\t%s %s, [x29, x9]", instruction,
    AARCH64RegisterNameFromNum(reg,
                          reg_type, kSize64Bit, buf, sizeof(buf)));
  }
  fprintf(fp, "\t\t// %s\n", symbol_name);
}

static COMPILER_UNUSED void GenerateOffsetFromFrame(AARCH64Emitter* emitter, int reg,
                                    int offset,
                                    const char* symbol_name,
                                    FILE* fp) {
  char buf[256];
  AddSubImmediate(emitter, AARCH64RegisterNameFromNum(reg,
                                                    kAARCH64RegTypeInt,kSize64Bit, buf, sizeof(buf)),
                    "x29", /*add=*/false, offset, symbol_name, fp);

}


// Save all used registers on the stack.
static void SaveRegisters(AARCH64Emitter* emitter, FILE* fp) {
  // Entry sequence:
  // stp x29, x30, [sp, #-16]!
  // mov x29, sp
  // sub sp, sp, #S
 
  int stack_frame_size = StackFrameSize(emitter);

  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
          !emitter->g->not_leaf;
  bool varargs = emitter->g->base.varargs;
  int space_above_frame_pointer = VarargsSaveAreaSize(emitter);
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
                     AARCH64_STACK_FRAME_HEADER_SIZE;

  // Offset from sp of first saved register.
  int saved_reg_offset = stack_frame_size - AARCH64_STACK_FRAME_HEADER_SIZE - 8 -
                          emitter->g->base.stack_frame_size -
                          space_above_frame_pointer -
                          emitter->spill_region_size;  // First saved register.

  // Non-empty leaf frames use the same x29/x30 record as other frames. Keep
  // their callee-saved slots below the local and spill regions as well.
  if (EmptyStackFrame(emitter)) {
    // Empty stack frame, no need to store frame pointer.
  } else {
    fprintf(fp, "\tstp x29, x30, [sp, #-16]!\n");
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_push:\n", emitter->g->base.function_name.value);
    }
    fprintf(fp, "\tmov x29, sp\n");
    if (!varargs) {
      fprintf(fp, ".Leh_%s_after_leaq:\n", emitter->g->base.function_name.value);
    }
    DecrementStackPointer(emitter, stack_frame_size, fp);

    if (varargs && space_above_frame_pointer > 0) {
      // AAPCS64 variadic register save area.  StackFrameSize() reserved
      // space_above_frame_pointer bytes immediately above the frame pointer for
      // it, so lower x29 to expose that region (the saved x29/x30 record stays
      // at the top of the frame and is restored from sp in the epilogue,
      // independent of x29).  Layout, low to high address from the lowered x29:
      //   [x29 .. x29+gp_size)            GP save area: x_named..x7
      //   [x29+gp_size .. x29+gp+vr_size) VR save area: d_named..d7 (16B slots)
      // va_start records __gr_top, __vr_top, their negative offsets and the
      // on-stack overflow pointer (__stack = x29 + 16 + space_above) so that
      // va_arg can walk the three regions independently.
      int gp_size = VarargsGpSaveSize(emitter);
      fprintf(fp, "\t// varargs function: %d named int / %d named fp args\n",
              emitter->g->num_int_arg_regs, emitter->g->num_fp_arg_regs);
      fprintf(fp, "\tsub x29, x29, #%d\n", space_above_frame_pointer);
      int offset = 0;
      for (int i = emitter->g->num_int_arg_regs; i < AARCH64_NUM_INT_ARGS; i++) {
        fprintf(fp, "\tstr x%d, [x29, #%d]\n", i, offset);
        offset += 8;
      }
      offset = gp_size;
      for (int i = emitter->g->num_fp_arg_regs; i < AARCH64_NUM_FP_ARGS; i++) {
        fprintf(fp, "\tfstr d%d, [x29, #%d]\n", i, offset);
        offset += 16;
      }
    }
  }

  char buf1[8], buf2[8];


  if (emitter->g->saved_regs.length > 0) {
    fprintf(fp, "\t// Saved argument registers.\n");
  }
  for (size_t i = 0; i < emitter->g->saved_regs.length; i++) {
    SavedArgumentRegister* saved_reg = emitter->g->saved_regs.value.p[i];
    int offset = saved_reg->offset;
    AARCH64RegisterType reg_type =
        saved_reg->is_fp ? kAARCH64RegTypeFloat : kAARCH64RegTypeInt;
    fprintf(fp, "\t%s %s, [%s, #%d]\n",
            saved_reg->is_fp ? "fstr" : "str",
            AARCH64RegisterNameFromNum(saved_reg->reg_num, reg_type, kSize64Bit,
                                  buf1, sizeof(buf1)),
            AARCH64RegisterNameFromNum(saved_reg->base_reg_num, kAARCH64RegTypeInt, kSize64Bit,
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

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  if (!BitSetIteratorDone(&it)) {
    fprintf(fp, "\t// Saved integer registers.\n");
  }
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    int offset = saved_reg_offset;
    saved_reg_offset -= 8;
    fprintf(fp, "\tstr %s, [sp, #%d]\n",
            AARCH64RegisterNameFromNum(reg, kAARCH64RegTypeInt, kSize64Bit, buf1, sizeof(buf1)),
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
    fprintf(fp, "\tfstr %s, [sp, #%d]\n",
            AARCH64RegisterNameFromNum(reg, kAARCH64RegTypeFloat, kSize64Bit, buf1, sizeof(buf1)),
            offset);
    BitSetIteratorNext(&it);
  }

  if (!is_leaf) {
    fprintf(fp, "\t// End of stack frame\n");
  }
}

static void RestoreRegisters(AARCH64Emitter* emitter, FILE* fp) {
  // Exit sequence:
  // add sp, sp, #S
  // ldp x29, x30, [sp], #16

  int stack_frame_size = StackFrameSize(emitter);
  char buf1[8];

  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
          !emitter->g->not_leaf;

  // A leaf procedure doesn't save the return address.
  if (!is_leaf) {
    fprintf(fp, "\t// Restored registers.\n");
  }

  if (!EmptyStackFrame(emitter)) {
    // A VLA or over-aligned local may have moved sp below the fixed frame.
    // Reconstruct the frame bottom from x29 before reading saved registers or
    // applying the ordinary fixed-size epilogue.
    int frame_adjustment =
        stack_frame_size - VarargsSaveAreaSize(emitter);
    AddSubImmediate(emitter, "sp", "x29", /*add=*/false,
                    frame_adjustment, NULL, fp);
  }

  int offset = emitter->saved_reg_offset;

  BitSetIterator it;

  // Restore registers in exactly the same order (and therefore from exactly the
  // same offsets) that SaveRegisters stored them: integer registers first, then
  // floating-point.  The two sequences must iterate identically, otherwise a
  // register is reloaded from another register's slot (e.g. a callee-saved d8
  // restored from where x10 was spilled), silently corrupting its value.
  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tldr %s, [sp, #%d]\n",
            AARCH64RegisterNameFromNum(reg, kAARCH64RegTypeInt, kSize64Bit, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tfldr %s, [sp, #%d]\n",
            AARCH64RegisterNameFromNum(reg, kAARCH64RegTypeFloat, kSize64Bit, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame.
  } else {
    IncrementStackPointer(emitter, stack_frame_size, fp);
    fprintf(fp, "\tldp x29, x30, [sp], #16\n");
  }
}

static void RestoreExceptionLandingState(AARCH64Emitter* emitter, FILE* fp) {
  int stack_frame_size = StackFrameSize(emitter);
  // x29 points at the saved x29/x30 pair. Varargs functions lower x29 by
  // their register-save area after establishing the frame, so exclude that
  // area when reconstructing the current sp from x29.
  int frame_adjustment =
      stack_frame_size - VarargsSaveAreaSize(emitter);
  if (frame_adjustment <= 4095) {
    fprintf(fp, "\tsub sp, x29, #%d\n", frame_adjustment);
  } else {
    MoveImmediate(emitter, "x9", (uint64_t)frame_adjustment, fp);
    fprintf(fp, "\tsub sp, x29, x9\n");
  }

  // The callee-saved registers are deliberately left as the unwinder delivered
  // them.  Their frame slots hold the *caller's* values, stored on entry, but a
  // handler needs the values this function itself had at the call that threw,
  // and reloading the slots would overwrite exactly those.  The unwinder
  // reconstructs them from the CFI of the frames it pops, which is why the FDE
  // has to describe where the prologue put them.  The hidden result pointer is
  // this function's own, so that one is reloaded.
  if (emitter->g->struct_return_reg >= 0) {
    char buf[8];
    bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
                   !emitter->g->not_leaf;
    int reg_num = (is_leaf ? AARCH64_FIRST_LEAF_INT_REG_VAR
                           : AARCH64_FIRST_INT_REG_VAR) +
                  emitter->g->struct_return_reg;
    fprintf(fp, "\tldr %s, [x29, #%d]\n",
            AARCH64RegisterNameFromNum(reg_num, kAARCH64RegTypeInt,
                                      kSize64Bit, buf, sizeof(buf)),
            emitter->g->struct_return_spill_offset);
  }
}

// In aarch64_codegen.c.
extern int GetRegisterSize(TargetInstruction* inst);

// The rmov instructions are an explicit mov from operand[1] to
// operand[0].  Both are registers.
static void PrintRmov(AARCH64Emitter* emitter, TargetInstruction* inst, FILE* fp) {
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
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(mov):
      mnemonic = "mov";
      break;
    case AARCH64_OP(fmov):
      mnemonic = "fmov";
      break;
  
    default:
      assert(false);
  }
  char buf1[8], buf2[8];
  fprintf(
      fp, "\t%-12s%s, %s\n", mnemonic,
      AARCH64RegisterName((AARCH64Register*)inst->operand[0]->reg, GetRegisterSize(inst), buf1, sizeof(buf1)),
      AARCH64RegisterName((AARCH64Register*)inst->operand[1]->reg, GetRegisterSize(inst), buf2, sizeof(buf2)));
}

static const char* GetRegisterName(TargetInstruction* inst, int size, char* buf, size_t bufsize) {
  if (inst->reg != NULL) {
    return AARCH64RegisterName((AARCH64Register*)inst->reg, size, buf, bufsize);
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return AARCH64RegisterName((AARCH64Register*)inst->dest->reg, size, buf, bufsize);
  }
  return "";
}

static AsmOperand* GetAsmOperand(AARCH64AsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    return NULL;
  }
  if ((size_t)index < inst->asm_node->outputs.length) {
    return inst->asm_node->outputs.value.p[index];
  }
  return inst->asm_node->inputs.value.p[index - inst->asm_node->outputs.length];
}

static int FindAsmOperandByName(AARCH64AsmInstruction* inst, const char* name,
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

static void PrintAsmOperand(FILE* fp, AARCH64AsmInstruction* inst, int index,
                            char modifier) {
  if (index < 0 || index >= inst->num_operands) {
    fprintf(fp, "<bad-asm-operand>");
    return;
  }
  if (inst->reg_nums[index] == INT_MIN) {
    fprintf(fp, "#%" PRId64, inst->immediate_values[index]);
    return;
  }
  int size = inst->sizes[index];
  if (modifier == 'w') {
    size = kSize32Bit;
  } else if (modifier == 'x') {
    size = kSize64Bit;
  } else if (modifier == 'l') {
    fprintf(fp, "0");
    return;
  }
  char buf[32];
  fprintf(fp, "%s",
          AARCH64RegisterNameFromNum(inst->reg_nums[index],
                                     inst->is_fp[index] ? kAARCH64RegTypeFloat
                                                        : kAARCH64RegTypeInt,
                                     size, buf, sizeof(buf)));
}

static int FindAsmLabelByName(AARCH64AsmInstruction* inst, const char* name,
                              size_t len) {
  for (size_t i = 0; i < inst->asm_node->labels.length; i++) {
    String* label = inst->asm_node->labels.value.p[i];
    if (label->length == len && strncmp(label->value, name, len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void PrintAsmLabel(FILE* fp, AARCH64AsmInstruction* inst, int index) {
  if (index < 0 || (size_t)index >= inst->asm_node->labels.length) {
    fprintf(fp, "<bad-asm-label>");
    return;
  }
  String* label = inst->asm_node->labels.value.p[index];
  fprintf(fp, "%s", label->value);
}

static void PrintExtendedAsm(FILE* fp, AARCH64AsmInstruction* inst,
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
    if (*p == 'w' || *p == 'x' || *p == 'l') {
      modifier = *p++;
    }
    int index = -1;
    if (*p == '[') {
      const char* name = ++p;
      while (*p != '\0' && *p != ']') {
        p++;
      }
      if (modifier == 'l') {
        index = FindAsmLabelByName(inst, name, (size_t)(p - name));
      } else {
        index = FindAsmOperandByName(inst, name, (size_t)(p - name));
      }
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
      PrintAsmOperand(fp, inst, index, modifier);
    }
  }
}

// Emit mov/movz (etc.) when the result is assigned via inst->dest.
static void PrintDestMove(AARCH64Emitter* emitter, TargetInstruction* inst,
                          FILE* fp) {
  TargetInstruction* src = inst->operand[0];
  if (src == NULL || src->block == NULL) {
    return;
  }
  int reg_size = GetRegisterSize(inst);
  char buf1[8], buf2[8];
  const char* dest_name =
      GetRegisterName(inst, reg_size, buf1, sizeof(buf1));
  if (dest_name[0] == '\0') {
    return;
  }
  if (TargetIsConst(src)) {
    MoveImmediate(emitter, dest_name, (uint64_t)TargetIntValue(src), fp);
    return;
  }
  if (src->reg == NULL) {
    return;
  }
  if (inst->reg != NULL && src->reg == inst->reg) {
    return;
  }
  if (inst->dest != NULL && inst->dest->reg != NULL &&
      src->reg == inst->dest->reg) {
    return;
  }
  fprintf(fp, "\t%-12s%s, %s\n", "mov", dest_name,
          AARCH64RegisterName((AARCH64Register*)src->reg, reg_size, buf2,
                              sizeof(buf2)));
}

static int AtomicSizeLog2(TargetInstruction* inst) {
  return (inst->flags & AARCH64_ATOMIC_SIZE_MASK) >>
         AARCH64_ATOMIC_SIZE_SHIFT;
}

static int AtomicOrder(TargetInstruction* inst) {
  return (inst->flags & AARCH64_ATOMIC_ORDER_MASK) >>
         AARCH64_ATOMIC_ORDER_SHIFT;
}

static int AtomicFailureOrder(TargetInstruction* inst) {
  return (inst->flags & AARCH64_ATOMIC_FAILURE_ORDER_MASK) >>
         AARCH64_ATOMIC_FAILURE_ORDER_SHIFT;
}

static bool AtomicOrderHasAcquire(int order) {
  return order == 1 || order == 2 || order == 4 || order == 5;
}

static bool AtomicOrderHasRelease(int order) {
  return order == 3 || order == 4 || order == 5;
}

static const char* AtomicLoadMnemonic(int size_log2, bool acquire,
                                      bool exclusive) {
  if (exclusive) {
    if (size_log2 == 0) return acquire ? "ldaxrb" : "ldxrb";
    if (size_log2 == 1) return acquire ? "ldaxrh" : "ldxrh";
    return acquire ? "ldaxr" : "ldxr";
  }
  if (acquire) {
    if (size_log2 == 0) return "ldarb";
    if (size_log2 == 1) return "ldarh";
    return "ldar";
  }
  if (size_log2 == 0) return "ldrb";
  if (size_log2 == 1) return "ldrh";
  return "ldr";
}

static const char* AtomicStoreMnemonic(int size_log2, bool release,
                                       bool exclusive) {
  if (exclusive) {
    if (size_log2 == 0) return release ? "stlxrb" : "stxrb";
    if (size_log2 == 1) return release ? "stlxrh" : "stxrh";
    return release ? "stlxr" : "stxr";
  }
  if (release) {
    if (size_log2 == 0) return "stlrb";
    if (size_log2 == 1) return "stlrh";
    return "stlr";
  }
  if (size_log2 == 0) return "strb";
  if (size_log2 == 1) return "strh";
  return "str";
}

static const char* AtomicRegName(TargetInstruction* inst, int size_log2,
                                 char* buf, size_t bufsize) {
  return GetRegisterName(inst,
                         size_log2 == 3 ? kSize64Bit : kSize32Bit,
                         buf, bufsize);
}

static const char* AtomicOperandRegName(TargetInstruction* inst, int operand,
                                        int size_log2, char* buf,
                                        size_t bufsize) {
  assert(inst->operand[operand] != NULL &&
         inst->operand[operand]->reg != NULL);
  return AARCH64RegisterName(
      (AARCH64Register*)inst->operand[operand]->reg,
      size_log2 == 3 ? kSize64Bit : kSize32Bit, buf, bufsize);
}

static const char* AtomicAddressRegName(TargetInstruction* inst, int operand,
                                        char* buf, size_t bufsize) {
  assert(inst->operand[operand] != NULL &&
         inst->operand[operand]->reg != NULL);
  return AARCH64RegisterName((AARCH64Register*)inst->operand[operand]->reg,
                             kSize64Bit, buf, bufsize);
}

static void PrintAtomicInstruction(TargetInstruction* inst,
                                   const char* func_name, FILE* fp) {
  int size = AtomicSizeLog2(inst);
  int order = AtomicOrder(inst);
  int failure_order = AtomicFailureOrder(inst);
  bool acquire = AtomicOrderHasAcquire(order);
  bool release = AtomicOrderHasRelease(order);
  char b0[16], b1[16], b2[16], b3[16];
  const char* result =
      inst->reg == NULL ? NULL : AtomicRegName(inst, size, b0, sizeof(b0));

  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(atomic_load): {
      const char* addr = AtomicAddressRegName(inst, 0, b1, sizeof(b1));
      fprintf(fp, "\t%s %s, [%s]\n",
              AtomicLoadMnemonic(size, acquire, false), result, addr);
      return;
    }
    case AARCH64_OP(atomic_store): {
      const char* value =
          AtomicOperandRegName(inst, 0, size, b0, sizeof(b0));
      const char* addr = AtomicAddressRegName(inst, 1, b1, sizeof(b1));
      fprintf(fp, "\t%s %s, [%s]\n",
              AtomicStoreMnemonic(size, release, false), value, addr);
      return;
    }
    case AARCH64_OP(atomic_fence):
      if (order == 0) {
        fprintf(fp, "\t// relaxed atomic fence\n");
      } else if (order == 1 || order == 2) {
        fprintf(fp, "\tdmb ishld\n");
      } else if (order == 3) {
        fprintf(fp, "\tdmb ishst\n");
      } else {
        fprintf(fp, "\tdmb ish\n");
      }
      return;
    case AARCH64_OP(atomic_fetch_add):
    case AARCH64_OP(atomic_fetch_sub):
    case AARCH64_OP(atomic_add_fetch):
    case AARCH64_OP(atomic_sub_fetch): {
      bool add = inst->opcode == (TargetOpcode)AARCH64_OP(atomic_fetch_add) ||
                 inst->opcode == (TargetOpcode)AARCH64_OP(atomic_add_fetch);
      bool return_new =
          inst->opcode == (TargetOpcode)AARCH64_OP(atomic_add_fetch) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(atomic_sub_fetch);
      const char* addr = AtomicAddressRegName(inst, 0, b1, sizeof(b1));
      const char* value =
          AtomicOperandRegName(inst, 1, size, b2, sizeof(b2));
      const char* loaded = return_new
                               ? (size == 3 ? "x16" : "w16")
                               : result;
      const char* updated = return_new
                                ? result
                                : (size == 3 ? "x16" : "w16");
      fprintf(fp, ".L%s_atomic_retry_%d:\n", func_name, inst->id);
      fprintf(fp, "\t%s %s, [%s]\n",
              AtomicLoadMnemonic(size, acquire, true), loaded, addr);
      fprintf(fp, "\t%s %s, %s, %s\n", add ? "add" : "sub", updated,
              loaded, value);
      fprintf(fp, "\t%s w17, %s, [%s]\n",
              AtomicStoreMnemonic(size, release, true), updated, addr);
      fprintf(fp, "\tcbnz w17, .L%s_atomic_retry_%d\n", func_name,
              inst->id);
      return;
    }
    case AARCH64_OP(atomic_compare_exchange_bool):
    case AARCH64_OP(atomic_compare_exchange_val):
    case AARCH64_OP(atomic_compare_exchange_n): {
      bool expected_is_pointer =
          inst->opcode ==
          (TargetOpcode)AARCH64_OP(atomic_compare_exchange_n);
      bool returns_bool =
          inst->opcode !=
          (TargetOpcode)AARCH64_OP(atomic_compare_exchange_val);
      acquire = acquire || AtomicOrderHasAcquire(failure_order);
      const char* addr = AtomicAddressRegName(inst, 0, b1, sizeof(b1));
      const char* expected;
      if (expected_is_pointer) {
        const char* expected_addr =
            AtomicAddressRegName(inst, 1, b2, sizeof(b2));
        expected = size == 3 ? "x16" : "w16";
        fprintf(fp, "\t%s %s, [%s]\n",
                AtomicLoadMnemonic(size, false, false), expected,
                expected_addr);
      } else {
        expected = AtomicOperandRegName(inst, 1, size, b2, sizeof(b2));
        if (size == 0) {
          fprintf(fp, "\tuxtb w16, %s\n", expected);
          expected = "w16";
        } else if (size == 1) {
          fprintf(fp, "\tuxth w16, %s\n", expected);
          expected = "w16";
        }
      }
      const char* desired =
          AtomicOperandRegName(inst, 2, size, b3, sizeof(b3));
      fprintf(fp, ".L%s_atomic_retry_%d:\n", func_name, inst->id);
      fprintf(fp, "\t%s %s, [%s]\n",
              AtomicLoadMnemonic(size, acquire, true), result, addr);
      fprintf(fp, "\tcmp %s, %s\n", result, expected);
      fprintf(fp, "\tb.ne .L%s_atomic_mismatch_%d\n", func_name, inst->id);
      fprintf(fp, "\t%s w17, %s, [%s]\n",
              AtomicStoreMnemonic(size, release, true), desired, addr);
      if ((inst->flags & AARCH64_ATOMIC_WEAK) != 0) {
        fprintf(fp, "\tcbnz w17, .L%s_atomic_spurious_%d\n", func_name,
                inst->id);
      } else {
        fprintf(fp, "\tcbnz w17, .L%s_atomic_retry_%d\n", func_name,
                inst->id);
      }
      if (returns_bool) {
        fprintf(fp, "\tmov %s, #1\n",
                AtomicRegName(inst, 2, b0, sizeof(b0)));
      }
      fprintf(fp, "\tb .L%s_atomic_done_%d\n", func_name, inst->id);
      fprintf(fp, ".L%s_atomic_mismatch_%d:\n", func_name, inst->id);
      fprintf(fp, "\tclrex\n");
      if ((inst->flags & AARCH64_ATOMIC_WEAK) != 0) {
        fprintf(fp, ".L%s_atomic_spurious_%d:\n", func_name, inst->id);
      }
      if (expected_is_pointer) {
        const char* expected_addr =
            AtomicAddressRegName(inst, 1, b2, sizeof(b2));
        fprintf(fp, "\t%s %s, [%s]\n",
                AtomicStoreMnemonic(size, false, false), result,
                expected_addr);
      }
      if (returns_bool) {
        fprintf(fp, "\tmov %s, #0\n",
                AtomicRegName(inst, 2, b0, sizeof(b0)));
      }
      fprintf(fp, ".L%s_atomic_done_%d:\n", func_name, inst->id);
      return;
    }
    default:
      assert(false);
  }
}

static void PrintSpillSlotAddress(int offset, FILE* fp) {
  const char* spill = "x16";
  const char* frame = "x29";
  if (offset <= 4095) {
    fprintf(fp, "\tsub %s, %s, #%d\n", spill, frame, offset);
    return;
  }

  uint32_t value = (uint32_t)offset;
  fprintf(fp, "\tmovz %s, #%u\n", spill, value & 0xffffu);
  if ((value >> 16) != 0) {
    fprintf(fp, "\tmovk %s, #%u, lsl #16\n", spill, value >> 16);
  }
  fprintf(fp, "\tsub %s, %s, %s\n", spill, frame, spill);
}

// Main instruction printer.
static void PrintInstruction(AARCH64Emitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    TargetBasicBlock* b = inst->block;
    fprintf(fp, "\n\t// *** Basic block %zd\n\n", b->block_id);
    emitter->current_block = inst->block;
  }
  if (((int)inst->opcode == (int)AARCH64_OP(label))) {
    if ((inst->flags & AARCH64_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    if ((inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
      RestoreExceptionLandingState(emitter, fp);
    }
    return;
  }

  if (((int)inst->opcode == (int)AARCH64_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }
  
  if (((int)inst->opcode == (int)AARCH64_OP(ivarreg)) || ((int)inst->opcode == (int)AARCH64_OP(fvarreg))) {
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
  
  // Buffers for register name printing->base.
  char buf1[8];
  char buf2[8];
  // Buffer for symbol name printing (large enough for mangled local names).
  char symbuf[256];

  int reg_size = GetRegisterSize(inst);
  
  // Special case instructions.
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(tp):
      fprintf(fp, "\tmrs %s, tpidr_el0\n",
              GetRegisterName(inst, kSize64Bit, buf1, sizeof(buf1)));
      return;
    case AARCH64_OP(fmv_s):
    case AARCH64_OP(fmv_d): {
      assert(inst->dest != NULL);
      assert(inst->operand[0] != NULL);
      int size = (AARCH64Opcode)inst->opcode == AARCH64_OP(fmv_d)
                     ? kSize64Bit
                     : kSize32Bit;
      if (inst->reg == inst->operand[0]->reg) {
        return;
      }
      fprintf(fp, "\tfmov        %s, %s\n",
              GetRegisterName(inst, size, buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[0], size, buf2, sizeof(buf2)));
      return;
    }
    case AARCH64_OP(atomic_load):
    case AARCH64_OP(atomic_store):
    case AARCH64_OP(atomic_fetch_add):
    case AARCH64_OP(atomic_fetch_sub):
    case AARCH64_OP(atomic_add_fetch):
    case AARCH64_OP(atomic_sub_fetch):
    case AARCH64_OP(atomic_compare_exchange_bool):
    case AARCH64_OP(atomic_compare_exchange_val):
    case AARCH64_OP(atomic_compare_exchange_n):
    case AARCH64_OP(atomic_fence):
      PrintAtomicInstruction(inst, func_name, fp);
      return;
    case AARCH64_OP(mv):
      // Don't emit mv x, x.
      if (inst->operand[0]->reg == inst->reg) {
        return;
      }
      if (inst->dest != NULL) {
        PrintRmov(emitter, inst, fp);
        return;
      }
      break;
    case AARCH64_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n",
                TargetSymbolName(sym->symbol, symbuf, sizeof(symbuf)));
      } else if (SymbolHasWeakBinding(sym->symbol)) {
        fprintf(fp, "\t.weak %s\n",
                TargetSymbolName(sym->symbol, symbuf, sizeof(symbuf)));
      } else {
        fprintf(fp, "\t.global %s\n",
                TargetSymbolName(sym->symbol, symbuf, sizeof(symbuf)));
      }
      return;
    }
    // case AARCH64_OP(bl):
    case AARCH64_OP(bl): {
      assert(((int)inst->operand[0]->opcode == (int)AARCH64_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      fprintf(fp, "\t%-12s%s\n", "bl",
              TargetSymbolName(sym->symbol, symbuf, sizeof(symbuf)));
      return;
    }

#if 0
    case AARCH64_OP(rcall):
    case AARCH64_OP(rcallf):
      fprintf(fp, "\t%-12s x1, %s, 0\n", "jalr",
              GetRegisterName(inst->operand[0], buf2,
                             sizeof(buf2)));

      return;
#endif
    case AARCH64_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case AARCH64_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case AARCH64_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      fprintf(fp, "\t");
      if ((inst->flags & AARCH64_INST_EXTENDED_ASM) != 0) {
        PrintExtendedAsm(fp, (AARCH64AsmInstruction*)inst, lit->value.value);
      } else {
        fprintf(fp, "%s", lit->value.value);
      }
      fprintf(fp, "\n");
      lit->base.disabled = true;
      return;
    }

    case AARCH64_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }

    case AARCH64_OP(vadd):
    case AARCH64_OP(vsub):
    case AARCH64_OP(vand):
    case AARCH64_OP(vorr):
    case AARCH64_OP(veor):
    case AARCH64_OP(vcmeq):
    case AARCH64_OP(vcmgt):
    case AARCH64_OP(vcmge):
    case AARCH64_OP(vcmhi):
    case AARCH64_OP(vcmhs):
    case AARCH64_OP(vfadd):
    case AARCH64_OP(vfsub):
    case AARCH64_OP(vfmul):
    case AARCH64_OP(vfdiv): {
      int q = GetRegisterSize(inst) == kSize128Bit;
      int size_log = (inst->flags >> AARCH64_SIMD_ELEM_SHIFT) & 3;
      static const char* arrangements[2][4] = {
          {"8b", "4h", "2s", "1d"},
          {"16b", "8h", "4s", "2d"},
      };
      const char* arrangement = arrangements[q][size_log];
      if (inst->opcode == (TargetOpcode)AARCH64_OP(vand) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(vorr) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(veor)) {
        arrangement = q ? "16b" : "8b";
      }
      const char* mnemonic =
          inst->opcode == (TargetOpcode)AARCH64_OP(vadd) ? "add" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vsub) ? "sub" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vand) ? "and" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vorr) ? "orr" :
          inst->opcode == (TargetOpcode)AARCH64_OP(veor) ? "eor" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vcmeq) ? "cmeq" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vcmgt) ? "cmgt" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vcmge) ? "cmge" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vcmhi) ? "cmhi" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vcmhs) ? "cmhs" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vfadd) ? "fadd" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vfsub) ? "fsub" :
          inst->opcode == (TargetOpcode)AARCH64_OP(vfmul) ? "fmul" : "fdiv";
      fprintf(fp, "\t%-12sv%d.%s, v%d.%s, v%d.%s\n", mnemonic,
              inst->reg->num, arrangement,
              inst->operand[0]->reg->num, arrangement,
              inst->operand[1]->reg->num, arrangement);
      return;
    }

    case AARCH64_OP(spill): {
      AARCH64Register* reg = (AARCH64Register*)inst->reg;
      int offset = (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      int spill_size = GetRegisterSize(inst) == kSize128Bit ? kSize128Bit
                                                            : kSize64Bit;
      PrintSpillSlotAddress(offset, fp);
      fprintf(fp, "\t%s %s, [x16, #0]\t// Spilled @%d\n",
              reg->type == kAARCH64RegTypeInt ? "str" : "fstr",
              AARCH64RegisterName(reg, spill_size, buf1, sizeof(buf1)),
              inst->operand[0]->id);
      return;
    }
      
    case AARCH64_OP(reload): {
      AARCH64Register* reg = (AARCH64Register*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      int spill_size = GetRegisterSize(spill) == kSize128Bit ? kSize128Bit
                                                             : kSize64Bit;
      PrintSpillSlotAddress(offset, fp);
      fprintf(fp, "\t%s %s, [x16, #0]\t// Reloaded spilled @%d\n",
              reg->type == kAARCH64RegTypeInt ? "ldr" : "fldr",
              AARCH64RegisterName(reg, spill_size, buf1, sizeof(buf1)),
              spill->operand[0]->id);
      return;
    }
    default:
      break;
  }

  // General case for instruction printing->base.

  TrapInstruction(inst);

  // Print opcode.
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(b):
      fprintf(fp, "\t%s", AARCH64OpcodeName(inst->opcode));
      break;
    case AARCH64_OP(mov):
      // Ppcode might be done by MoveImmediate.
      break;
    default:
      fprintf(fp, "\t%-12s", AARCH64OpcodeName(inst->opcode));
      break;
  }

  // Print operands.
  switch ((AARCH64Opcode)inst->opcode) {
    case  AARCH64_OP(ldr):
    case   AARCH64_OP(ldur):
    case   AARCH64_OP(ldrb):
    case   AARCH64_OP(ldrh):
    case   AARCH64_OP(ldurb):
    case   AARCH64_OP(ldurh):
    case   AARCH64_OP(ldrsb):
    case   AARCH64_OP(ldrsh):
    case   AARCH64_OP(ldursb):
    case   AARCH64_OP(ldursh):
    case   AARCH64_OP(ldursw):
    case AARCH64_OP(fldr):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      assert(inst->operand[0]->reg != NULL);
      if (TargetIsConst(inst->operand[1])) {
        int offset = (int)TargetIntValue(inst->operand[1]);
        assert(AARCH64IsPossibleImmediate(offset));
        fprintf(fp, "%s, [%s, #%d]\n",
                GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[0], kSize64Bit, buf2,
                               sizeof(buf2)), offset);
#if 0
      } else if (((int)inst->operand[1]->opcode == (int)AARCH64_OP(symbol))) {
        assert((inst->flags & AARCH64_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst, reg_size,buf1, sizeof(buf1)),
                ((TargetSymbol*)inst->operand[1])->symbol->name.value,
                GetRegisterName(inst->operand[0], reg_size,buf2,
                               sizeof(buf2)));
      } else if (((int)inst->operand[1]->opcode == (int)AARCH64_OP(label))) {
        assert((inst->flags & AARCH64_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst, reg_size,buf1, sizeof(buf1)),
                func_name, inst->operand[1]->id,
                GetRegisterName(inst->operand[0], reg_size,buf2,
                               sizeof(buf2)));
#endif
      } else if (((int)inst->operand[1]->opcode == (int)AARCH64_OP(zr))) {
        fprintf(fp, "%s, [%s, #0]\n",
                GetRegisterName(inst, reg_size,buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[0], reg_size,buf2,
                               sizeof(buf2)));
      } else {
        assert(false);
      }
      break;

    case AARCH64_OP(str):
    case AARCH64_OP(stur):
    case AARCH64_OP(strb):
    case AARCH64_OP(strh):
    case AARCH64_OP(sturb):
    case AARCH64_OP(sturh):
    case AARCH64_OP(fstr):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      assert(inst->operand[0]->reg != NULL);
      assert(inst->operand[1]->reg != NULL);
      if (TargetIsConst(inst->operand[2])) {
        int offset = (int)TargetIntValue(inst->operand[2]);
        assert(AARCH64IsPossibleImmediate(offset));
        fprintf(fp, "%s, [%s, #%d]\n",
                GetRegisterName(inst->operand[0], reg_size, buf1,
                               sizeof(buf1)),
                GetRegisterName(inst->operand[1], kSize64Bit, buf2,
                               sizeof(buf2)), offset);
#if 0
      } else if (((int)inst->operand[2]->opcode == (int)AARCH64_OP(symbol))) {
        assert((inst->flags & AARCH64_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst->operand[0], reg_size,buf1,
                               sizeof(buf1)),
                ((TargetSymbol*)inst->operand[2])->symbol->name.value,
                GetRegisterName(inst->operand[1],reg_size, buf2,
                               sizeof(buf2)));
      } else if (((int)inst->operand[2]->opcode == (int)AARCH64_OP(label))) {
        assert((inst->flags & AARCH64_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst->operand[0], reg_size,buf1,
                               sizeof(buf1)),
                func_name, inst->operand[2]->id,
                GetRegisterName(inst->operand[1],reg_size, buf2,
                               sizeof(buf2)));
#endif
      } else if (((int)inst->operand[2]->opcode == (int)AARCH64_OP(zr))) {
        fprintf(fp, "%s, [%s, #0]\n",
                GetRegisterName(inst->operand[0], reg_size,buf1,
                               sizeof(buf1)),
                GetRegisterName(inst->operand[1], reg_size,buf2,
                               sizeof(buf2)));

      } else {
        assert(false);
      }
      break;
      
    case AARCH64_OP(nop):
      fprintf(fp, "\n");
      break;
      
  case AARCH64_OP(b): {
    assert(inst->operand[0] != NULL);
    assert(inst->operand[1] != NULL);
    TargetInstruction* cond = inst->operand[0];
    TargetInstruction* dest = inst->operand[1];
    char condbuf[8] = {0};
    if (((int)cond->opcode != (int)AARCH64_OP(al))) {
      snprintf(condbuf, sizeof(condbuf), ".%s", AARCH64OpcodeName(cond->opcode));
    }
    if (((int)dest->opcode == (int)AARCH64_OP(symbol))) {
      // Tail-call branch to a function symbol (not a local label).
      fprintf(fp, "%s %s\n", condbuf,
              TargetSymbolName(((TargetSymbol*)dest)->symbol, symbuf,
                               sizeof(symbuf)));
    } else if (((int)dest->opcode == (int)AARCH64_OP(named_label))) {
      fprintf(fp, "%s %s\n", condbuf, ((TargetNamedLabel*)dest)->name);
    } else {
      fprintf(fp, "%s .%s_label_%d\n", condbuf, func_name, dest->id);
    }
    break;
  }
      
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz): {
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[0]->reg != NULL);
      TargetInstruction* dest = inst->operand[1];
      fprintf(fp, "%s, ",
              GetRegisterName(inst->operand[0], reg_size, buf1,
                              sizeof(buf1)));
      if (dest->opcode == (TargetOpcode)AARCH64_OP(named_label)) {
        fprintf(fp, "%s\n", ((TargetNamedLabel*)dest)->name);
      } else {
        fprintf(fp, ".%s_label_%d\n", func_name, dest->id);
      }
      break;
    }

    case AARCH64_OP(br):
      fprintf(fp, "%s\n",
              GetRegisterName(inst->operand[0], kSize64Bit, buf1,
                             sizeof(buf1)));
      break;

    case AARCH64_OP(cset):
    case AARCH64_OP(csetm):
      assert(inst->operand[0] != NULL);
      fprintf(fp, "%s, %s\n",
              GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
              AARCH64OpcodeName(inst->operand[0]->opcode));
      break;

    case AARCH64_OP(csel): {
      // csel Rd, Rn, Rm, cond  ->  Rd = cond ? Rn : Rm
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      char buf3[8];
      fprintf(fp, "%s, %s, %s, %s\n",
              GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[0], reg_size, buf2, sizeof(buf2)),
              GetRegisterName(inst->operand[1], reg_size, buf3, sizeof(buf3)),
              AARCH64OpcodeName(inst->operand[2]->opcode));
      break;
    }
      
    case AARCH64_OP(blr):
      assert(inst->operand[0] != NULL);
      fprintf(fp, "%s\n",
              GetRegisterName(inst->operand[0], kSize64Bit, buf1,
                             sizeof(buf1)));
      break;

    // Floating-point conversions move between registers whose widths may
    // differ from each other: e.g. scvtf converts a 32-bit integer (w) into a
    // single (s) or double (d), and fcvtsd widens a single (s) to a double
    // (d).  A single instruction-wide "size" cannot describe both, so print the
    // destination at the instruction's own size and the source at its
    // producer's size.
    case AARCH64_OP(fcvtsd):
    case AARCH64_OP(fcvtds): {
      // fcvtsd widens a single (s) to a double (d); fcvtds narrows a double
      // (d) to a single (s).  The source width is fixed by the conversion, not
      // by the producer's tracked size (a float register variable lives in the
      // same physical register as a double and may report a 64-bit size).
      assert(inst->reg != NULL);
      assert(inst->operand[0] != NULL);
      int src_size = ((AARCH64Opcode)inst->opcode == AARCH64_OP(fcvtsd))
                         ? kSize32Bit
                         : kSize64Bit;
      fprintf(fp, "%s, %s\n",
              GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[0], src_size, buf2, sizeof(buf2)));
      break;
    }

    case AARCH64_OP(scvtf):
    case AARCH64_OP(ucvtf):
    case AARCH64_OP(fcvtns):
    case AARCH64_OP(fcvtnu):
    case AARCH64_OP(fcvtzs):
    case AARCH64_OP(fcvtzu):
    case AARCH64_OP(fcvt): {
      assert(inst->reg != NULL);
      assert(inst->operand[0] != NULL);
      int src_size = GetRegisterSize(inst->operand[0]);
      if (src_size == 0) {
        src_size = kSize64Bit;
      }
      fprintf(fp, "%s, %s\n",
              GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[0], src_size, buf2, sizeof(buf2)));
      break;
    }

    case AARCH64_OP(mov):
    case AARCH64_OP(movz):
    case AARCH64_OP(movk):
    case AARCH64_OP(movn):
      if (inst->dest != NULL) {
        PrintDestMove(emitter, inst, fp);
        return;
      }
      if (TargetIsConst(inst->operand[0])) {
        MoveImmediate(emitter, GetRegisterName(inst, reg_size, buf1, sizeof(buf1)),
                        (uint64_t)TargetIntValue(inst->operand[0]), fp);
        return;
      }
      if (inst->operand[1] == NULL && inst->operand[0] != NULL) {
        PrintDestMove(emitter, inst, fp);
        return;
      }
      if (inst->operand[0] != NULL && inst->operand[0]->reg != NULL &&
          inst->operand[1] != NULL && inst->operand[1]->reg != NULL) {
        PrintRmov(emitter, inst, fp);
        return;
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
            AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
            if (op == AARCH64_OP(adr) || op == AARCH64_OP(adrp)) {
              fprintf(fp, "%s%" PRId64 "", sep, TargetIntValue(inst->operand[i]));
            } else {
              fprintf(fp, "%s#%" PRId64 "", sep, TargetIntValue(inst->operand[i]));
            }
          } else if (((int)inst->operand[i]->opcode == (int)AARCH64_OP(symbol))) {
            if ((inst->flags & AARCH64_TPREL_HI_RELOC) != 0) {
              fprintf(fp, "%s:tprel_hi12:%s", sep,
                      TargetSymbolName(((TargetSymbol*)inst->operand[i])->symbol,
                                       symbuf, sizeof(symbuf)));
            } else if ((inst->flags & AARCH64_TPREL_LO_RELOC) != 0) {
              fprintf(fp, "%s:tprel_lo12_nc:%s", sep,
                      TargetSymbolName(((TargetSymbol*)inst->operand[i])->symbol,
                                       symbuf, sizeof(symbuf)));
            } else if ((inst->flags & AARCH64_HI_RELOC) != 0) {
              fprintf(fp, "%s%%hi(%s)", sep,
                      TargetSymbolName(((TargetSymbol*)inst->operand[i])->symbol,
                                       symbuf, sizeof(symbuf)));
            } else if ((inst->flags & AARCH64_LO_RELOC) != 0) {
              fprintf(fp, "%s:lo12:%s", sep,
                      TargetSymbolName(((TargetSymbol*)inst->operand[i])->symbol,
                                       symbuf, sizeof(symbuf)));
            } else {
              fprintf(fp, "%s%s", sep,
                      TargetSymbolName(((TargetSymbol*)inst->operand[i])->symbol,
                                       symbuf, sizeof(symbuf)));
            }
          } else if (((int)inst->operand[i]->opcode == (int)AARCH64_OP(literal))) {
            TargetLiteral* literal = (TargetLiteral*)inst->operand[i];
            fprintf(fp, "%s.str.%d", sep, literal->literal_id);
          } else if (((int)inst->operand[i]->opcode == (int)AARCH64_OP(oplsl))) {
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

static int ProgramRegNum(TargetInstruction* inst) {
  if (inst == NULL) {
    return -1;
  }
  if (inst->reg != NULL) {
    return inst->reg->num == AARCH64_SP_REG ? 31 : inst->reg->num;
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return inst->dest->reg->num == AARCH64_SP_REG ? 31
                                                  : inst->dest->reg->num;
  }
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(zr):
    case AARCH64_OP(sp):
      return 31;
    case AARCH64_OP(fp):
      return 29;
    case AARCH64_OP(lr):
      return 30;
    case AARCH64_OP(resulti):
    case AARCH64_OP(resultf):
    case AARCH64_OP(resultd):
    case AARCH64_OP(r0):
    case AARCH64_OP(d0):
      return 0;
    case AARCH64_OP(r1):
    case AARCH64_OP(d1):
      return 1;
    case AARCH64_OP(r2):
    case AARCH64_OP(d2):
      return 2;
    case AARCH64_OP(r3):
    case AARCH64_OP(d3):
      return 3;
    case AARCH64_OP(r4):
    case AARCH64_OP(d4):
      return 4;
    case AARCH64_OP(r5):
    case AARCH64_OP(d5):
      return 5;
    case AARCH64_OP(r6):
    case AARCH64_OP(d6):
      return 6;
    case AARCH64_OP(r7):
    case AARCH64_OP(d7):
      return 7;
    case AARCH64_OP(r9):
      return 9;
    case AARCH64_OP(structreturn):
      return 8;
    default:
      break;
  }
  return -1;
}

static bool ProgramRegIsFP(TargetInstruction* inst) {
  TargetRegister* reg = inst != NULL ? inst->reg : NULL;
  if (reg == NULL && inst != NULL && inst->dest != NULL) {
    reg = inst->dest->reg;
  }
  return reg != NULL &&
         ((AARCH64Register*)reg)->type == kAARCH64RegTypeFloat;
}

static bool ProgramIs64(TargetInstruction* inst) {
  return GetRegisterSize(inst) != kSize32Bit;
}

static AARCH64AsmCondition ProgramCondition(TargetInstruction* condition) {
  switch ((AARCH64Opcode)condition->opcode) {
    case AARCH64_OP(eq): return AARCH64_ASM_COND_EQ;
    case AARCH64_OP(ne): return AARCH64_ASM_COND_NE;
    case AARCH64_OP(cs):
    case AARCH64_OP(hs): return AARCH64_ASM_COND_CS;
    case AARCH64_OP(cc):
    case AARCH64_OP(lo): return AARCH64_ASM_COND_CC;
    case AARCH64_OP(mi): return AARCH64_ASM_COND_MI;
    case AARCH64_OP(pl): return AARCH64_ASM_COND_PL;
    case AARCH64_OP(vs): return AARCH64_ASM_COND_VS;
    case AARCH64_OP(vc): return AARCH64_ASM_COND_VC;
    case AARCH64_OP(hi): return AARCH64_ASM_COND_HI;
    case AARCH64_OP(ls): return AARCH64_ASM_COND_LS;
    case AARCH64_OP(ge): return AARCH64_ASM_COND_GE;
    case AARCH64_OP(lt): return AARCH64_ASM_COND_LT;
    case AARCH64_OP(gt): return AARCH64_ASM_COND_GT;
    case AARCH64_OP(le): return AARCH64_ASM_COND_LE;
    case AARCH64_OP(al): return AARCH64_ASM_COND_AL;
    default:
      assert(false);
      return AARCH64_ASM_COND_AL;
  }
}

static AARCH64AsmCondition ProgramInvertCondition(AARCH64AsmCondition cond) {
  if (cond == AARCH64_ASM_COND_AL) {
    return cond;
  }
  return (AARCH64AsmCondition)(cond ^ 1);
}

static void ProgramMoveImmediate(AsmModule* module, int reg, bool is_64bit,
                                 uint64_t value) {
  if (!is_64bit) {
    value &= 0xffffffffu;
  }
  if (value == 0) {
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeMoveWide(is_64bit, 2, 0, 0) | (uint32_t)reg);
    return;
  }
  bool emitted = false;
  int words = is_64bit ? 4 : 2;
  for (int i = 0; i < words; i++) {
    uint16_t part = (uint16_t)(value >> (i * 16));
    if (part == 0) {
      continue;
    }
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeMoveWide(is_64bit, emitted ? 3 : 2, part, i) |
                    (uint32_t)reg);
    emitted = true;
  }
}

static void ProgramAddSubImmediate(AsmModule* module, int dest, int source,
                                   bool is_64bit, bool add, int64_t value) {
  if (value < 0) {
    value = -value;
    add = !add;
  }
  if (value > 0xffffff) {
    ProgramMoveImmediate(module, 8, is_64bit, (uint64_t)value);
    AARCH64AsmRegister rd = {.num = dest,
                             .kind = is_64bit ? AARCH64_ASM_REG_X
                                             : AARCH64_ASM_REG_W};
    AARCH64AsmRegister rn = rd;
    rn.num = source;
    AARCH64AsmOperand rm = {
        .type = AARCH64_ASM_OPERAND_REGISTER,
        .reg = {.num = 8,
                .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W},
    };
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeAddSubShiftedRegister(&rd, &rn, &rm, !add,
                                                   false));
    return;
  }
  if (value == 0 && dest != source) {
    if (dest == 31 || source == 31) {
      AARCH64AsmRegister rd = {.num = dest,
                               .kind = is_64bit ? AARCH64_ASM_REG_X
                                               : AARCH64_ASM_REG_W,
                               .is_sp = dest == 31};
      AARCH64AsmRegister rn = rd;
      rn.num = source;
      rn.is_sp = source == 31;
      AARCH64ProgramEmitWord(
          module,
          AARCH64EncodeAddSubImmediate(&rd, &rn, 0, false, false, 0));
      return;
    }
    AARCH64AsmRegister rd = {.num = dest,
                             .kind = is_64bit ? AARCH64_ASM_REG_X
                                             : AARCH64_ASM_REG_W};
    AARCH64AsmRegister zr = AARCH64AsmZeroRegister(rd.kind);
    AARCH64AsmOperand rm = {
        .type = AARCH64_ASM_OPERAND_REGISTER,
        .reg = {.num = source, .kind = rd.kind},
    };
    AARCH64ProgramEmitWord(
        module,
        AARCH64EncodeLogicalShiftedRegister(&rd, &zr, &rm, is_64bit, 1, 0));
    return;
  }
  int shift = 0;
  while (value != 0) {
    int part = (int)(value & 0xfff);
    if (part != 0) {
      AARCH64AsmRegister rd = {.num = dest,
                               .kind = is_64bit ? AARCH64_ASM_REG_X
                                               : AARCH64_ASM_REG_W};
      AARCH64AsmRegister rn = rd;
      rn.num = source;
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeAddSubImmediate(&rd, &rn, part, !add, false,
                                               shift != 0));
      source = dest;
    }
    value >>= 12;
    shift += 12;
  }
}

static void ProgramMoveRegister(AsmModule* module, int dest, int source,
                                bool is_64bit, bool fp) {
  if (dest == source) {
    return;
  }
  if (fp) {
    AARCH64ProgramEmitWord(module,
                           AARCH64EncodeFPMove(is_64bit, dest, source));
    return;
  }
  if (dest == 31 || source == 31) {
    ProgramAddSubImmediate(module, dest, source, is_64bit, true, 0);
    return;
  }
  AARCH64AsmRegister rd = {
      .num = dest,
      .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
  };
  AARCH64AsmRegister zr = AARCH64AsmZeroRegister(rd.kind);
  AARCH64AsmOperand rm = {
      .type = AARCH64_ASM_OPERAND_REGISTER,
      .reg = {.num = source, .kind = rd.kind},
  };
  AARCH64ProgramEmitWord(
      module,
      AARCH64EncodeLogicalShiftedRegister(&rd, &zr, &rm, is_64bit, 1, 0));
}

static void ProgramMoveTargetRegister(AsmModule* module, int dest,
                                      TargetInstruction* source,
                                      bool is_64bit, bool fp) {
  int source_reg = ProgramRegNum(source);
  if (!fp && source_reg == 31 &&
      source->opcode != (TargetOpcode)AARCH64_OP(sp)) {
    AARCH64AsmRegister rd = {
        .num = dest,
        .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
    };
    AARCH64AsmRegister zr = AARCH64AsmZeroRegister(rd.kind);
    AARCH64AsmOperand rm = {
        .type = AARCH64_ASM_OPERAND_REGISTER,
        .reg = zr,
    };
    AARCH64ProgramEmitWord(
        module,
        AARCH64EncodeLogicalShiftedRegister(&rd, &zr, &rm, is_64bit, 1, 0));
    return;
  }
  ProgramMoveRegister(module, dest, source_reg, is_64bit, fp);
}

static void ProgramLoadStore(AsmModule* module, bool load, bool fp, int size,
                             bool sign, int value_reg, int base_reg,
                             int offset) {
  if (size == 4) {
    // 128-bit SIMD&FP: size field 00, opc 11 (load) or 10 (store), scale 16.
    int opc = load ? 3 : 2;
    if (offset >= 0 && (offset & 15) == 0 && offset / 16 <= 0xfff) {
      int immediate = offset / 16;
      AARCH64ProgramEmitWord(
          module, (0x39u << 24) | (1u << 26) | ((uint32_t)opc << 22) |
                      ((uint32_t)immediate << 10) | ((uint32_t)base_reg << 5) |
                      (uint32_t)value_reg);
    } else {
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeLoadStoreUnscaled(0, true, opc, value_reg,
                                                 base_reg, offset, 0));
    }
    return;
  }
  int opc = sign && load ? (size == 3 ? 2 : 3) : load;
  int scale = 1 << size;
  if (offset >= 0 && (offset & (scale - 1)) == 0 &&
      offset / scale <= 0xfff) {
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeLoadStoreUnsigned(size, fp, opc, value_reg,
                                               base_reg, offset));
  } else {
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeLoadStoreUnscaled(size, fp, opc, value_reg,
                                               base_reg, offset, 0));
  }
}

static void ProgramLabelName(String* result, const char* function,
                             TargetInstruction* label) {
  StringClear(result);
  if (label->opcode == (TargetOpcode)AARCH64_OP(named_label)) {
    StringAppend(result, ((TargetNamedLabel*)label)->name);
    return;
  }
  if (label->opcode == (TargetOpcode)AARCH64_OP(symbol)) {
    char unused[1];
    StringAppend(
        result,
        TargetSymbolName(((TargetSymbol*)label)->symbol, unused,
                         sizeof(unused)));
    return;
  }
  if (label->opcode == (TargetOpcode)AARCH64_OP(literal)) {
    char id[32];
    snprintf(id, sizeof(id), "%d", ((TargetLiteral*)label)->literal_id);
    StringAppend(result, ".str.");
    StringAppend(result, id);
    return;
  }
  char id[32];
  snprintf(id, sizeof(id), "%d", label->id);
  StringAppendChar(result, '.');
  StringAppend(result, function);
  StringAppend(result, "_label_");
  StringAppend(result, id);
}

static void ProgramNamedFunctionLabel(String* result, const char* prefix,
                                      const char* function,
                                      const char* suffix) {
  StringClear(result);
  StringAppend(result, prefix);
  StringAppend(result, function);
  StringAppend(result, suffix);
}

static void ProgramBranch(AsmModule* module, const char* mnemonic,
                          uint32_t word, AARCH64FixupKind fixup,
                          int relocation, const char* symbol,
                          bool force_relocation) {
  String text = {0};
  StringAppend(&text, mnemonic);
  StringAppendChar(&text, ' ');
  StringAppend(&text, symbol);
  AARCH64ProgramEmitFixup(module, word, fixup, relocation, symbol, 0,
                          force_relocation, text.value);
  StringDestruct(&text);
}

static void ProgramRegisterBranch(AsmModule* module, const char* mnemonic,
                                  uint32_t word, AARCH64FixupKind fixup,
                                  int relocation, int reg, bool is_64bit,
                                  const char* symbol) {
  char register_name[32];
  snprintf(register_name, sizeof(register_name), "%c%d", is_64bit ? 'x' : 'w',
           reg);
  String text = {0};
  StringAppend(&text, mnemonic);
  StringAppendChar(&text, ' ');
  StringAppend(&text, register_name);
  StringAppend(&text, ", ");
  StringAppend(&text, symbol);
  AARCH64ProgramEmitFixup(module, word, fixup, relocation, symbol, 0, false,
                          text.value);
  StringDestruct(&text);
}

static void ProgramSpillAddress(AsmModule* module, int offset) {
  if (offset <= 4095) {
    ProgramAddSubImmediate(module, 16, 29, true, false, offset);
  } else {
    ProgramMoveImmediate(module, 16, true, (uint32_t)offset);
    AARCH64AsmRegister rd = {.num = 16, .kind = AARCH64_ASM_REG_X};
    AARCH64AsmRegister rn = {.num = 29, .kind = AARCH64_ASM_REG_X};
    AARCH64AsmOperand rm = {
        .type = AARCH64_ASM_OPERAND_REGISTER,
        .reg = {.num = 16, .kind = AARCH64_ASM_REG_X},
    };
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeAddSubShiftedRegister(&rd, &rn, &rm, true,
                                                   false));
  }
}

static void ProgramSaveRegisters(AARCH64Emitter* emitter, AsmModule* module) {
  int stack_frame_size = StackFrameSize(emitter);
  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
                 !emitter->g->not_leaf;
  bool varargs = emitter->g->base.varargs;
  int space_above_frame_pointer = VarargsSaveAreaSize(emitter);
  int saved_reg_offset =
      stack_frame_size - AARCH64_STACK_FRAME_HEADER_SIZE - 8 -
      emitter->g->base.stack_frame_size - space_above_frame_pointer -
      emitter->spill_region_size;
  if (is_leaf) {
    AsmModuleComment(module, "Leaf procedure, no stack frame generated");
  }
  if (!EmptyStackFrame(emitter)) {
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeLoadStorePair(2, false, false, 3, 29, 30, 31,
                                           -16));
    if (!varargs) {
      String label = {0};
      ProgramNamedFunctionLabel(&label, ".Leh_",
                                emitter->g->base.function_name.value,
                                "_after_push");
      AsmModuleLabel(module, label.value);
      StringDestruct(&label);
    }
    ProgramMoveRegister(module, 29, 31, true, false);
    if (!varargs) {
      String label = {0};
      ProgramNamedFunctionLabel(&label, ".Leh_",
                                emitter->g->base.function_name.value,
                                "_after_leaq");
      AsmModuleLabel(module, label.value);
      StringDestruct(&label);
    }
    ProgramAddSubImmediate(module, 31, 31, true, false, stack_frame_size);
    if (varargs && space_above_frame_pointer > 0) {
      AsmModuleComment(module, "variadic argument register save area");
      ProgramAddSubImmediate(module, 29, 29, true, false,
                             space_above_frame_pointer);
      int offset = 0;
      for (int i = emitter->g->num_int_arg_regs; i < AARCH64_NUM_INT_ARGS;
           i++) {
        ProgramLoadStore(module, false, false, 3, false, i, 29, offset);
        offset += 8;
      }
      offset = VarargsGpSaveSize(emitter);
      for (int i = emitter->g->num_fp_arg_regs; i < AARCH64_NUM_FP_ARGS;
           i++) {
        ProgramLoadStore(module, false, true, 3, false, i, 29, offset);
        offset += 16;
      }
    }
  }
  if (emitter->g->saved_regs.length > 0) {
    AsmModuleComment(module, "Saved argument registers.");
  }
  for (size_t i = 0; i < emitter->g->saved_regs.length; i++) {
    SavedArgumentRegister* saved = emitter->g->saved_regs.value.p[i];
    ProgramLoadStore(module, false, saved->is_fp, 3, false, saved->reg_num,
                     saved->base_reg_num, saved->offset);
  }
  if (!is_leaf) {
    AsmModuleComment(module, "Local variable and spill area.");
  }
  int local_vars =
      emitter->g->base.stack_frame_size + AARCH64_STACK_FRAME_HEADER_SIZE;
  emitter->first_spill_offset = local_vars + 8;
  emitter->saved_reg_offset = saved_reg_offset;

  BitSetIterator it;
  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  if (!BitSetIteratorDone(&it)) {
    AsmModuleComment(module, "Saved integer registers.");
  }
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    ProgramLoadStore(module, false, false, 3, false, reg, 31,
                     saved_reg_offset);
    saved_reg_offset -= 8;
    BitSetIteratorNext(&it);
  }
  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  if (!BitSetIteratorDone(&it)) {
    AsmModuleComment(module, "Saved floating point registers.");
  }
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    ProgramLoadStore(module, false, true, 3, false, reg, 31,
                     saved_reg_offset);
    saved_reg_offset -= 8;
    BitSetIteratorNext(&it);
  }
}

static void ProgramRestoreRegisters(AARCH64Emitter* emitter,
                                    AsmModule* module) {
  int stack_frame_size = StackFrameSize(emitter);
  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
                 !emitter->g->not_leaf;
  if (!is_leaf) {
    AsmModuleComment(module, "Restored registers.");
  }
  if (!EmptyStackFrame(emitter)) {
    int frame_adjustment =
        stack_frame_size - VarargsSaveAreaSize(emitter);
    ProgramAddSubImmediate(module, 31, 29, true, false, frame_adjustment);
  }
  int offset = emitter->saved_reg_offset;
  BitSetIterator it;
  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    ProgramLoadStore(module, true, false, 3, false,
                     (int)BitSetIteratorValue(&it), 31, offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }
  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    ProgramLoadStore(module, true, true, 3, false,
                     (int)BitSetIteratorValue(&it), 31, offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }
  if (!EmptyStackFrame(emitter)) {
    ProgramAddSubImmediate(module, 31, 31, true, true, stack_frame_size);
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeLoadStorePair(2, false, true, 1, 29, 30, 31,
                                           16));
  }
}

static void ProgramRestoreExceptionLandingState(AARCH64Emitter* emitter,
                                                AsmModule* module) {
  int frame_adjustment =
      StackFrameSize(emitter) - VarargsSaveAreaSize(emitter);
  ProgramAddSubImmediate(module, 31, 29, true, false, frame_adjustment);
  if (emitter->g->struct_return_reg >= 0) {
    bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
                   !emitter->g->not_leaf;
    int reg = (is_leaf ? AARCH64_FIRST_LEAF_INT_REG_VAR
                       : AARCH64_FIRST_INT_REG_VAR) +
              emitter->g->struct_return_reg;
    ProgramLoadStore(module, true, false, 3, false, reg, 29,
                     emitter->g->struct_return_spill_offset);
  }
}

static void ProgramEmitAtomic(AARCH64Emitter* emitter, TargetInstruction* inst,
                              const char* function, AsmModule* module) {
  int size = AtomicSizeLog2(inst);
  int order = AtomicOrder(inst);
  int failure_order = AtomicFailureOrder(inst);
  bool acquire = AtomicOrderHasAcquire(order);
  bool release = AtomicOrderHasRelease(order);
  int result = ProgramRegNum(inst);
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(atomic_load):
      if (acquire) {
        AARCH64ProgramEmitWord(module, AARCH64EncodeAcquireRelease(
                                             size, true, result,
                                             ProgramRegNum(inst->operand[0])));
      } else {
        ProgramLoadStore(module, true, false, size, false, result,
                         ProgramRegNum(inst->operand[0]), 0);
      }
      return;
    case AARCH64_OP(atomic_store): {
      int value = ProgramRegNum(inst->operand[0]);
      int address = ProgramRegNum(inst->operand[1]);
      if (release) {
        AARCH64ProgramEmitWord(
            module,
            AARCH64EncodeAcquireRelease(size, false, value, address));
      } else {
        ProgramLoadStore(module, false, false, size, false, value, address, 0);
      }
      return;
    }
    case AARCH64_OP(atomic_fence):
      if (order == 0) {
        AsmModuleComment(module, "relaxed atomic fence");
      } else {
        AARCH64ProgramEmitWord(
            module, AARCH64EncodeDmb(order == 1 || order == 2
                                         ? 9
                                         : order == 3 ? 10 : 11));
      }
      return;
    case AARCH64_OP(atomic_fetch_add):
    case AARCH64_OP(atomic_fetch_sub):
    case AARCH64_OP(atomic_add_fetch):
    case AARCH64_OP(atomic_sub_fetch): {
      bool add = inst->opcode == (TargetOpcode)AARCH64_OP(atomic_fetch_add) ||
                 inst->opcode == (TargetOpcode)AARCH64_OP(atomic_add_fetch);
      bool return_new =
          inst->opcode == (TargetOpcode)AARCH64_OP(atomic_add_fetch) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(atomic_sub_fetch);
      int address = ProgramRegNum(inst->operand[0]);
      int value = ProgramRegNum(inst->operand[1]);
      int loaded = return_new ? 16 : result;
      int updated = return_new ? result : 16;
      char suffix[64];
      snprintf(suffix, sizeof(suffix), "_atomic_retry_%d", inst->id);
      String retry = {0};
      ProgramNamedFunctionLabel(&retry, ".L", function, suffix);
      AsmModuleLabel(module, retry.value);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeExclusiveLoad(size, acquire, loaded, address));
      AARCH64AsmRegister rd = {
          .num = updated,
          .kind = size == 3 ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
      };
      AARCH64AsmRegister rn = rd;
      rn.num = loaded;
      AARCH64AsmOperand rm = {
          .type = AARCH64_ASM_OPERAND_REGISTER,
          .reg = {.num = value, .kind = rd.kind},
      };
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeAddSubShiftedRegister(&rd, &rn, &rm, !add,
                                                     false));
      AARCH64ProgramEmitWord(
          module,
          AARCH64EncodeExclusiveStore(size, release, 17, updated, address));
      ProgramRegisterBranch(
          module, "cbnz", AARCH64EncodeCompareBranch(false, true, 17),
          kAARCH64FixupBranch19, R_AARCH64_CONDBR19, 17, false, retry.value);
      StringDestruct(&retry);
      return;
    }
    case AARCH64_OP(atomic_compare_exchange_bool):
    case AARCH64_OP(atomic_compare_exchange_val):
    case AARCH64_OP(atomic_compare_exchange_n): {
      bool expected_pointer =
          inst->opcode ==
          (TargetOpcode)AARCH64_OP(atomic_compare_exchange_n);
      bool returns_bool =
          inst->opcode !=
          (TargetOpcode)AARCH64_OP(atomic_compare_exchange_val);
      acquire |= AtomicOrderHasAcquire(failure_order);
      int address = ProgramRegNum(inst->operand[0]);
      int expected = ProgramRegNum(inst->operand[1]);
      int desired = ProgramRegNum(inst->operand[2]);
      if (expected_pointer) {
        ProgramLoadStore(module, true, false, size, false, 16, expected, 0);
        expected = 16;
      } else if (size < 2) {
        AARCH64ProgramEmitWord(
            module, AARCH64EncodeBitfield(false, 2, 16, expected, 0,
                                           size == 0 ? 7 : 15));
        expected = 16;
      }
      String retry = {0}, mismatch = {0}, spurious = {0}, done = {0};
      char suffix[64];
      snprintf(suffix, sizeof(suffix), "_atomic_retry_%d", inst->id);
      ProgramNamedFunctionLabel(&retry, ".L", function, suffix);
      snprintf(suffix, sizeof(suffix), "_atomic_mismatch_%d", inst->id);
      ProgramNamedFunctionLabel(&mismatch, ".L", function, suffix);
      snprintf(suffix, sizeof(suffix), "_atomic_spurious_%d", inst->id);
      ProgramNamedFunctionLabel(&spurious, ".L", function, suffix);
      snprintf(suffix, sizeof(suffix), "_atomic_done_%d", inst->id);
      ProgramNamedFunctionLabel(&done, ".L", function, suffix);
      AsmModuleLabel(module, retry.value);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeExclusiveLoad(size, acquire, result, address));
      AARCH64AsmRegister zr = AARCH64AsmZeroRegister(
          size == 3 ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W);
      AARCH64AsmRegister rn = zr;
      rn.num = result;
      AARCH64AsmOperand rm = {
          .type = AARCH64_ASM_OPERAND_REGISTER,
          .reg = {.num = expected, .kind = zr.kind},
      };
      AARCH64ProgramEmitWord(
          module,
          AARCH64EncodeAddSubShiftedRegister(&zr, &rn, &rm, true, true));
      ProgramBranch(module, "b.ne",
                    AARCH64EncodeConditionalBranch(
                        0, AARCH64_ASM_COND_NE, false),
                    kAARCH64FixupBranch19, R_AARCH64_CONDBR19, mismatch.value,
                    false);
      AARCH64ProgramEmitWord(
          module,
          AARCH64EncodeExclusiveStore(size, release, 17, desired, address));
      ProgramRegisterBranch(
          module, "cbnz", AARCH64EncodeCompareBranch(false, true, 17),
          kAARCH64FixupBranch19, R_AARCH64_CONDBR19, 17, false,
          (inst->flags & AARCH64_ATOMIC_WEAK) ? spurious.value
                                               : retry.value);
      if (returns_bool) {
        ProgramMoveImmediate(module, result, false, 1);
      }
      ProgramBranch(module, "b", AARCH64EncodeUnconditionalBranchImmediate(0,
                                                                            false),
                    kAARCH64FixupBranch26, R_AARCH64_JUMP26, done.value,
                    false);
      AsmModuleLabel(module, mismatch.value);
      AARCH64ProgramEmitWord(module, 0xd5033f5fu);
      if ((inst->flags & AARCH64_ATOMIC_WEAK) != 0) {
        AsmModuleLabel(module, spurious.value);
      }
      if (expected_pointer) {
        ProgramLoadStore(module, false, false, size, false, result,
                         ProgramRegNum(inst->operand[1]), 0);
      }
      if (returns_bool) {
        ProgramMoveImmediate(module, result, false, 0);
      }
      AsmModuleLabel(module, done.value);
      StringDestruct(&retry);
      StringDestruct(&mismatch);
      StringDestruct(&spurious);
      StringDestruct(&done);
      return;
    }
    default:
      assert(false);
  }
}

static bool ProgramBinaryRegisters(TargetInstruction* inst, int* dest,
                                   int* left, int* right) {
  *dest = ProgramRegNum(inst);
  *left = ProgramRegNum(inst->operand[0]);
  *right = ProgramRegNum(inst->operand[1]);
  return *dest >= 0 && *left >= 0 && *right >= 0;
}

static void ProgramUnsupportedInstruction(TargetInstruction* inst,
                                          AsmModule* module);

static bool ProgramShiftAmount(TargetInstruction* inst, int* amount) {
  if (inst->operand[2] == NULL && inst->operand[3] == NULL) {
    *amount = 0;
    return true;
  }
  if (inst->operand[2] == NULL || inst->operand[3] == NULL ||
      inst->operand[2]->opcode != (TargetOpcode)AARCH64_OP(oplsl) ||
      !TargetIsConst(inst->operand[3])) {
    return false;
  }
  *amount = (int)TargetIntValue(inst->operand[3]);
  return true;
}

static void ProgramEmitAddSub(TargetInstruction* inst, AsmModule* module,
                              bool subtract, bool set_flags,
                              bool compare_alias) {
  int dest;
  int left;
  TargetInstruction* right_value;
  bool is_64bit;
  if (compare_alias) {
    left = ProgramRegNum(inst->operand[0]);
    right_value = inst->operand[1];
    is_64bit = ProgramIs64(inst);
    dest = 31;
  } else {
    dest = ProgramRegNum(inst);
    left = ProgramRegNum(inst->operand[0]);
    right_value = inst->operand[1];
    is_64bit = ProgramIs64(inst);
  }
  AARCH64AsmRegister rd = {
      .num = dest,
      .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
  };
  AARCH64AsmRegister rn = rd;
  rn.num = left;
  if (right_value->opcode == (TargetOpcode)AARCH64_OP(symbol)) {
    char buffer[256];
    const char* symbol =
        TargetSymbolName(((TargetSymbol*)right_value)->symbol, buffer,
                         sizeof(buffer));
    int relocation;
    const char* modifier;
    bool shifted = false;
    if ((inst->flags & AARCH64_TPREL_HI_RELOC) != 0) {
      relocation = R_AARCH64_TLSLE_ADD_TPREL_HI12;
      modifier = "tprel_hi12";
      shifted = true;
    } else if ((inst->flags & AARCH64_TPREL_LO_RELOC) != 0) {
      relocation = R_AARCH64_TLSLE_ADD_TPREL_LO12_NC;
      modifier = "tprel_lo12_nc";
    } else {
      relocation = R_AARCH64_ADD_ABS_LO12_NC;
      modifier = "lo12";
    }
    char prefix[128];
    snprintf(prefix, sizeof(prefix), "%s %c%d, %c%d, :%s:",
             subtract ? "sub" : "add", is_64bit ? 'x' : 'w', dest,
             is_64bit ? 'x' : 'w', left, modifier);
    String text = {0};
    StringAppend(&text, prefix);
    StringAppend(&text, symbol);
    AARCH64ProgramEmitFixup(
        module,
        AARCH64EncodeAddSubImmediate(&rd, &rn, 0, subtract, set_flags,
                                     shifted),
        kAARCH64FixupRelocationOnly, relocation, symbol, 0, true, text.value);
    StringDestruct(&text);
    return;
  }
  if (TargetIsConst(right_value)) {
    int64_t immediate = TargetIntValue(right_value);
    if (left == 31 &&
        inst->operand[0]->opcode != (TargetOpcode)AARCH64_OP(sp) &&
        !subtract && !set_flags) {
      ProgramMoveImmediate(module, dest, is_64bit, (uint64_t)immediate);
      return;
    }
    if (!compare_alias && !set_flags) {
      ProgramAddSubImmediate(module, dest, left, is_64bit, !subtract,
                             immediate);
    } else {
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeAddSubImmediate(&rd, &rn, (int)immediate,
                                               subtract, set_flags, 0));
    }
  } else {
    int shift;
    if (!ProgramShiftAmount(inst, &shift)) {
      ProgramUnsupportedInstruction(inst, module);
      return;
    }
    AARCH64AsmOperand rm = {
        .type = AARCH64_ASM_OPERAND_REGISTER,
        .reg = {.num = ProgramRegNum(right_value), .kind = rd.kind},
        .shift = {.type = AARCH64_ASM_SHIFT_LSL, .amount = shift},
    };
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeAddSubShiftedRegister(&rd, &rn, &rm, subtract,
                                                   set_flags));
  }
}

static void ProgramEmitLogical(TargetInstruction* inst, AsmModule* module,
                               int opc, bool invert, bool test_alias) {
  bool is_64bit = ProgramIs64(inst);
  int dest = test_alias ? 31 : ProgramRegNum(inst);
  int left = ProgramRegNum(inst->operand[0]);
  TargetInstruction* right_value = inst->operand[1];
  AARCH64AsmRegister rd = {
      .num = dest,
      .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
  };
  AARCH64AsmRegister rn = rd;
  rn.num = left;
  if (TargetIsConst(right_value)) {
    uint64_t immediate = (uint64_t)TargetIntValue(right_value);
    uint64_t encoded_immediate = invert ? ~immediate : immediate;
    if (!is_64bit) {
      encoded_immediate = (uint32_t)encoded_immediate;
    }
    static const char* operations[] = {"and", "orr", "eor", "ands"};
    const char* operation = test_alias ? "tst" : operations[opc];
    char text[160];
    if (test_alias) {
      snprintf(text, sizeof(text), "%s %c%d, #0x%llx", operation,
               is_64bit ? 'x' : 'w', left,
               (unsigned long long)immediate);
    } else {
      snprintf(text, sizeof(text), "%s %c%d, %c%d, #0x%llx", operation,
               is_64bit ? 'x' : 'w', dest, is_64bit ? 'x' : 'w', left,
               (unsigned long long)encoded_immediate);
    }
    AARCH64ProgramEmitWordText(
        module,
        AARCH64EncodeLogicalImmediateInst(&rd, &rn, encoded_immediate,
                                          is_64bit, opc),
        text);
  } else {
    int shift;
    if (!ProgramShiftAmount(inst, &shift)) {
      ProgramUnsupportedInstruction(inst, module);
      return;
    }
    AARCH64AsmOperand rm = {
        .type = AARCH64_ASM_OPERAND_REGISTER,
        .reg = {.num = ProgramRegNum(right_value), .kind = rd.kind},
        .shift = {.type = AARCH64_ASM_SHIFT_LSL, .amount = shift},
    };
    AARCH64ProgramEmitWord(
        module, AARCH64EncodeLogicalShiftedRegister(&rd, &rn, &rm, is_64bit,
                                                     opc, invert));
  }
}

static void ProgramEmitInlineAsm(TargetInstruction* instruction,
                                 AsmModule* module) {
  TargetLiteral* literal = (TargetLiteral*)instruction->operand[0];
  StringLiteral* text = CompilerFindStringLiteral(literal->literal_id);
  assert(text != NULL);
  if ((instruction->flags & AARCH64_INST_EXTENDED_ASM) == 0) {
    AsmModuleText(module, "inline_asm", text->value.value,
                  text->value.length);
  } else {
    char* value = NULL;
    size_t length = 0;
    FILE* stream = open_memstream(&value, &length);
    if (stream == NULL) {
      module->failed = true;
      return;
    }
    PrintExtendedAsm(stream, (AARCH64AsmInstruction*)instruction,
                     text->value.value);
    if (fclose(stream) != 0) {
      module->failed = true;
    } else {
      AsmModuleText(module, "extended_asm", value, length);
    }
    free(value);
  }
  text->base.disabled = true;
}

static void ProgramUnsupportedInstruction(TargetInstruction* inst,
                                          AsmModule* module) {
  fprintf(stderr, "Unsupported programmatic AArch64 opcode %s\n",
          AARCH64OpcodeName(inst->opcode));
  module->failed = true;
}

static void ProgramEmitInstruction(AARCH64Emitter* emitter,
                                   TargetInstruction* inst,
                                   const char* function, AsmModule* module) {
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    char comment[128];
    snprintf(comment, sizeof(comment), "*** Basic block %zd",
             ((TargetBasicBlock*)inst->block)->block_id);
    AsmModuleComment(module, comment);
    emitter->current_block = inst->block;
  }
  if (inst->opcode == (TargetOpcode)AARCH64_OP(label)) {
    String name = {0};
    ProgramLabelName(&name, function, inst);
    if ((inst->flags & AARCH64_EXPORTED_LABEL) != 0) {
      AsmModuleSymbol(module, name.value, SYM_TYPE(none), SYM_BIND(local), 0, 1,
                      false, true, false);
    }
    AsmModuleLabel(module, name.value);
    StringDestruct(&name);
    if ((inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
      ProgramRestoreExceptionLandingState(emitter, module);
    }
    return;
  }
  if (inst->opcode == (TargetOpcode)AARCH64_OP(named_label)) {
    AsmModuleLabel(module, ((TargetNamedLabel*)inst)->name);
    return;
  }
  if (!IsPrintable(inst) || inst->observable_checkpoint) {
    return;
  }
  char id_comment[32];
  snprintf(id_comment, sizeof(id_comment), "@%d", inst->id);
  AsmModuleComment(module, id_comment);

  int dest = ProgramRegNum(inst);
  bool is_64bit = ProgramIs64(inst);
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(save):
      ProgramSaveRegisters(emitter, module);
      return;
    case AARCH64_OP(restore):
      ProgramRestoreRegisters(emitter, module);
      return;
    case AARCH64_OP(asm):
      ProgramEmitInlineAsm(inst, module);
      return;
    case AARCH64_OP(loc): {
      int file, line, column;
      SourceLocationNumbers(((TargetLocation*)inst)->location, &file, &line,
                            &column);
      AsmModuleLocation(module, file + 1, line, column + 1);
      return;
    }
    case AARCH64_OP(symbol): {
      TargetSymbol* target = (TargetSymbol*)inst;
      char name[256];
      const char* spelling =
          TargetSymbolName(target->symbol, name, sizeof(name));
      AsmExpr symbol_expression;
      AsmExprInitSymbol(&symbol_expression, spelling, 0);
      AsmModuleSymbol(
          module, symbol_expression.symbol.value, SYM_TYPE(none),
          StorageIs(target->symbol->storage, STO(static))
              ? SYM_BIND(local)
              : SymbolHasWeakBinding(target->symbol) ? SYM_BIND(weak)
                                                     : SYM_BIND(global),
          0, 1, false, true, false);
      AsmExprDestruct(&symbol_expression);
      return;
    }
    case AARCH64_OP(tp):
      AARCH64ProgramEmitWord(module, AARCH64EncodeMrsTpidrEl0(dest));
      return;
    case AARCH64_OP(fmv_s):
    case AARCH64_OP(fmv_d):
      ProgramMoveTargetRegister(
          module, dest, inst->operand[0],
          inst->opcode == (TargetOpcode)AARCH64_OP(fmv_d), true);
      return;
    case AARCH64_OP(mv):
      if (inst->operand[1] != NULL) {
        ProgramMoveTargetRegister(module, ProgramRegNum(inst->operand[0]),
                                  inst->operand[1], is_64bit,
                                  ProgramRegIsFP(inst->operand[0]));
      } else if (TargetIsConst(inst->operand[0])) {
        ProgramMoveImmediate(module, dest, is_64bit,
                             (uint64_t)TargetIntValue(inst->operand[0]));
      } else {
        ProgramMoveTargetRegister(module, dest, inst->operand[0], is_64bit,
                                  ProgramRegIsFP(inst));
      }
      return;
    case AARCH64_OP(mov):
      if (inst->operand[1] != NULL && inst->operand[0] != NULL &&
          inst->operand[0]->reg != NULL && inst->operand[1]->reg != NULL) {
        ProgramMoveTargetRegister(module, ProgramRegNum(inst->operand[0]),
                                  inst->operand[1], is_64bit,
                                  ProgramRegIsFP(inst->operand[0]));
      } else if (TargetIsConst(inst->operand[0])) {
        ProgramMoveImmediate(module, dest, is_64bit,
                             (uint64_t)TargetIntValue(inst->operand[0]));
      } else {
        ProgramMoveTargetRegister(module, dest, inst->operand[0], is_64bit,
                                  ProgramRegIsFP(inst));
      }
      return;
    case AARCH64_OP(movz):
    case AARCH64_OP(movk):
    case AARCH64_OP(movn):
      if (TargetIsConst(inst->operand[0])) {
        ProgramMoveImmediate(module, dest, is_64bit,
                             (uint64_t)TargetIntValue(inst->operand[0]));
      } else {
        ProgramMoveTargetRegister(module, dest, inst->operand[0], is_64bit,
                                  ProgramRegIsFP(inst));
      }
      return;
    case AARCH64_OP(bl): {
      char name[512];
      TargetSymbol* target = (TargetSymbol*)inst->operand[0];
      const char* spelling =
          TargetSymbolName(target->symbol, name, sizeof(name));
      bool plt = compiler->pic &&
                 !StorageIs(target->symbol->storage, STO(static));
      ProgramBranch(module, "bl",
                    AARCH64EncodeUnconditionalBranchImmediate(0, true),
                    kAARCH64FixupBranch26,
                    plt ? R_AARCH64_CALL_PLT : R_AARCH64_CALL26, spelling,
                    plt);
      return;
    }
    case AARCH64_OP(b): {
      TargetInstruction* condition = inst->operand[0];
      TargetInstruction* target = inst->operand[1];
      String name = {0};
      ProgramLabelName(&name, function, target);
      AARCH64AsmCondition cond = ProgramCondition(condition);
      if (cond == AARCH64_ASM_COND_AL) {
        ProgramBranch(module, "b",
                      AARCH64EncodeUnconditionalBranchImmediate(0, false),
                      kAARCH64FixupBranch26, R_AARCH64_JUMP26, name.value,
                      target->opcode == (TargetOpcode)AARCH64_OP(symbol) &&
                          compiler->pic);
      } else {
        char mnemonic[16];
        snprintf(mnemonic, sizeof(mnemonic), "b.%s",
                 AARCH64OpcodeName(condition->opcode));
        ProgramBranch(module, mnemonic,
                      AARCH64EncodeConditionalBranch(0, cond, false),
                      kAARCH64FixupBranch19, R_AARCH64_CONDBR19, name.value,
                      false);
      }
      StringDestruct(&name);
      return;
    }
    case AARCH64_OP(cbz):
    case AARCH64_OP(cbnz): {
      int reg = ProgramRegNum(inst->operand[0]);
      bool width = ProgramIs64(inst);
      String name = {0};
      ProgramLabelName(&name, function, inst->operand[1]);
      bool nonzero = inst->opcode == (TargetOpcode)AARCH64_OP(cbnz);
      ProgramRegisterBranch(
          module, nonzero ? "cbnz" : "cbz",
          AARCH64EncodeCompareBranch(width, nonzero, reg),
          kAARCH64FixupBranch19, R_AARCH64_CONDBR19, reg, width, name.value);
      StringDestruct(&name);
      return;
    }
    case AARCH64_OP(tbz):
    case AARCH64_OP(tbnz): {
      int reg = ProgramRegNum(inst->operand[0]);
      int bit = (int)TargetIntValue(inst->operand[1]);
      String name = {0};
      ProgramLabelName(&name, function, inst->operand[2]);
      bool nonzero = inst->opcode == (TargetOpcode)AARCH64_OP(tbnz);
      char prefix[64];
      snprintf(prefix, sizeof(prefix), "%s %c%d, #%d, ",
               nonzero ? "tbnz" : "tbz",
               ProgramIs64(inst) ? 'x' : 'w', reg, bit);
      String text = {0};
      StringAppend(&text, prefix);
      StringAppend(&text, name.value);
      AARCH64ProgramEmitFixup(
          module, AARCH64EncodeTestBranch(bit, nonzero, reg),
          kAARCH64FixupTestBranch14, R_AARCH64_TSTBR14, name.value, 0, false,
          text.value);
      StringDestruct(&text);
      StringDestruct(&name);
      return;
    }
    case AARCH64_OP(br):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeBranchRegister(
                      0, 0, ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(blr):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeBranchRegister(
                      1, 0, ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(ret):
      AARCH64ProgramEmitWord(module, AARCH64EncodeReturn(30));
      return;
    case AARCH64_OP(add):
      ProgramEmitAddSub(inst, module, false, false, false);
      return;
    case AARCH64_OP(adds):
      ProgramEmitAddSub(inst, module, false, true, false);
      return;
    case AARCH64_OP(sub):
      ProgramEmitAddSub(inst, module, true, false, false);
      return;
    case AARCH64_OP(subs):
      ProgramEmitAddSub(inst, module, true, true, false);
      return;
    case AARCH64_OP(cmp):
      ProgramEmitAddSub(inst, module, true, true, true);
      return;
    case AARCH64_OP(cmn):
      ProgramEmitAddSub(inst, module, false, true, true);
      return;
    case AARCH64_OP(adc):
    case AARCH64_OP(adcs):
    case AARCH64_OP(sbc):
    case AARCH64_OP(sbcs): {
      int rd, rn, rm;
      if (!ProgramBinaryRegisters(inst, &rd, &rn, &rm)) {
        ProgramUnsupportedInstruction(inst, module);
        return;
      }
      AARCH64AsmRegister dr = {
          .num = rd,
          .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
      };
      AARCH64AsmRegister nr = dr;
      nr.num = rn;
      AARCH64AsmRegister mr = dr;
      mr.num = rm;
      bool subtract = inst->opcode == (TargetOpcode)AARCH64_OP(sbc) ||
                      inst->opcode == (TargetOpcode)AARCH64_OP(sbcs);
      bool flags = inst->opcode == (TargetOpcode)AARCH64_OP(adcs) ||
                   inst->opcode == (TargetOpcode)AARCH64_OP(sbcs);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeAddSubWithCarry(&dr, &nr, &mr, subtract,
                                               flags));
      return;
    }
    case AARCH64_OP(and):
      ProgramEmitLogical(inst, module, 0, false, false);
      return;
    case AARCH64_OP(bic):
      ProgramEmitLogical(inst, module, 0, true, false);
      return;
    case AARCH64_OP(orr):
      ProgramEmitLogical(inst, module, 1, false, false);
      return;
    case AARCH64_OP(orn):
      ProgramEmitLogical(inst, module, 1, true, false);
      return;
    case AARCH64_OP(eor):
      ProgramEmitLogical(inst, module, 2, false, false);
      return;
    case AARCH64_OP(eon):
      ProgramEmitLogical(inst, module, 2, true, false);
      return;
    case AARCH64_OP(ands):
      ProgramEmitLogical(inst, module, 3, false, false);
      return;
    case AARCH64_OP(bics):
      ProgramEmitLogical(inst, module, 3, true, false);
      return;
    case AARCH64_OP(tst):
      ProgramEmitLogical(inst, module, 3, false, true);
      return;
    case AARCH64_OP(mvn):
    case AARCH64_OP(not): {
      AARCH64AsmRegister rd = {
          .num = dest,
          .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
      };
      AARCH64AsmRegister zr = AARCH64AsmZeroRegister(rd.kind);
      AARCH64AsmOperand rm = {
          .type = AARCH64_ASM_OPERAND_REGISTER,
          .reg = {.num = ProgramRegNum(inst->operand[0]), .kind = rd.kind},
      };
      AARCH64ProgramEmitWord(
          module,
          AARCH64EncodeLogicalShiftedRegister(&rd, &zr, &rm, is_64bit, 1, 1));
      return;
    }
    case AARCH64_OP(mul):
    case AARCH64_OP(mneg): {
      int rd, rn, rm;
      ProgramBinaryRegisters(inst, &rd, &rn, &rm);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeDataProcessing3Source(
                      is_64bit, 0,
                      inst->opcode == (TargetOpcode)AARCH64_OP(mneg), rd, rn,
                      rm, 31));
      return;
    }
    case AARCH64_OP(madd):
    case AARCH64_OP(msub): {
      int rd = dest;
      int rn = ProgramRegNum(inst->operand[0]);
      int rm = ProgramRegNum(inst->operand[1]);
      int ra = ProgramRegNum(inst->operand[2]);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeDataProcessing3Source(
                      is_64bit, 0,
                      inst->opcode == (TargetOpcode)AARCH64_OP(msub), rd, rn,
                      rm, ra));
      return;
    }
    case AARCH64_OP(sdiv):
    case AARCH64_OP(udiv): {
      int rd, rn, rm;
      ProgramBinaryRegisters(inst, &rd, &rn, &rm);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeDivide(
                      is_64bit,
                      inst->opcode == (TargetOpcode)AARCH64_OP(udiv), rd, rn,
                      rm));
      return;
    }
    case AARCH64_OP(lslv):
    case AARCH64_OP(lsrv):
    case AARCH64_OP(asrv):
    case AARCH64_OP(rorv): {
      int rd, rn, rm;
      ProgramBinaryRegisters(inst, &rd, &rn, &rm);
      int shift = inst->opcode == (TargetOpcode)AARCH64_OP(lslv)
                      ? 8
                      : inst->opcode == (TargetOpcode)AARCH64_OP(lsrv)
                            ? 9
                            : inst->opcode == (TargetOpcode)AARCH64_OP(asrv)
                                  ? 10
                                  : 11;
      AARCH64ProgramEmitWord(
          module,
          AARCH64EncodeVariableShift(is_64bit, shift, rd, rn, rm));
      return;
    }
    case AARCH64_OP(smulh):
    case AARCH64_OP(umulh): {
      int rd, rn, rm;
      ProgramBinaryRegisters(inst, &rd, &rn, &rm);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeHighMultiply(
                      inst->opcode == (TargetOpcode)AARCH64_OP(umulh), rd, rn,
                      rm));
      return;
    }
    case AARCH64_OP(smull):
    case AARCH64_OP(smnegl):
    case AARCH64_OP(umull):
    case AARCH64_OP(umnegl): {
      int rd, rn, rm;
      ProgramBinaryRegisters(inst, &rd, &rn, &rm);
      bool unsigned_multiply =
          inst->opcode == (TargetOpcode)AARCH64_OP(umull) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(umnegl);
      bool negate = inst->opcode == (TargetOpcode)AARCH64_OP(smnegl) ||
                    inst->opcode == (TargetOpcode)AARCH64_OP(umnegl);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeDataProcessing3Source(
                      true, unsigned_multiply ? 5 : 1, negate, rd, rn, rm,
                      31));
      return;
    }
    case AARCH64_OP(smaddl):
    case AARCH64_OP(smsubl):
    case AARCH64_OP(umaddl):
    case AARCH64_OP(umsubl): {
      bool unsigned_multiply =
          inst->opcode == (TargetOpcode)AARCH64_OP(umaddl) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(umsubl);
      bool subtract = inst->opcode == (TargetOpcode)AARCH64_OP(smsubl) ||
                      inst->opcode == (TargetOpcode)AARCH64_OP(umsubl);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeDataProcessing3Source(
                      true, unsigned_multiply ? 5 : 1, subtract, dest,
                      ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[1]),
                      ProgramRegNum(inst->operand[2])));
      return;
    }
    case AARCH64_OP(neg):
    case AARCH64_OP(negs): {
      AARCH64AsmRegister rd = {
          .num = dest,
          .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
      };
      AARCH64AsmRegister zr = AARCH64AsmZeroRegister(rd.kind);
      AARCH64AsmOperand rm = {
          .type = AARCH64_ASM_OPERAND_REGISTER,
          .reg = {.num = ProgramRegNum(inst->operand[0]), .kind = rd.kind},
      };
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeAddSubShiftedRegister(
                      &rd, &zr, &rm, true,
                      inst->opcode == (TargetOpcode)AARCH64_OP(negs)));
      return;
    }
    case AARCH64_OP(ngc):
    case AARCH64_OP(ngcs): {
      AARCH64AsmRegister rd = {
          .num = dest,
          .kind = is_64bit ? AARCH64_ASM_REG_X : AARCH64_ASM_REG_W,
      };
      AARCH64AsmRegister zr = AARCH64AsmZeroRegister(rd.kind);
      AARCH64AsmRegister rm = rd;
      rm.num = ProgramRegNum(inst->operand[0]);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeAddSubWithCarry(
                      &rd, &zr, &rm, true,
                      inst->opcode == (TargetOpcode)AARCH64_OP(ngcs)));
      return;
    }
    case AARCH64_OP(lsl):
    case AARCH64_OP(lsr):
    case AARCH64_OP(asr): {
      int source = ProgramRegNum(inst->operand[0]);
      int shift = (int)TargetIntValue(inst->operand[1]);
      int maximum = is_64bit ? 63 : 31;
      int opc = inst->opcode == (TargetOpcode)AARCH64_OP(asr) ? 0 : 2;
      int immr = inst->opcode == (TargetOpcode)AARCH64_OP(lsl)
                     ? (maximum + 1 - shift) % (maximum + 1)
                     : shift;
      int imms = inst->opcode == (TargetOpcode)AARCH64_OP(lsl)
                     ? maximum - shift
                     : maximum;
      AARCH64ProgramEmitWord(
          module,
          AARCH64EncodeBitfield(is_64bit, opc, dest, source, immr, imms));
      return;
    }
    case AARCH64_OP(ror):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeExtract(is_64bit, dest,
                                       ProgramRegNum(inst->operand[0]),
                                       ProgramRegNum(inst->operand[0]),
                                       (int)TargetIntValue(inst->operand[1])));
      return;
    case AARCH64_OP(rbit):
    case AARCH64_OP(rev16):
    case AARCH64_OP(rev32):
    case AARCH64_OP(rev):
    case AARCH64_OP(clz):
    case AARCH64_OP(cls): {
      int operation =
          inst->opcode == (TargetOpcode)AARCH64_OP(rbit)
              ? 0
              : inst->opcode == (TargetOpcode)AARCH64_OP(rev16)
                    ? 1
                    : inst->opcode == (TargetOpcode)AARCH64_OP(rev32)
                          ? 2
                          : inst->opcode == (TargetOpcode)AARCH64_OP(rev)
                                ? 3
                                : inst->opcode == (TargetOpcode)AARCH64_OP(clz)
                                      ? 4
                                      : 5;
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeDataProcessing1Source(
                      is_64bit, operation, dest,
                      ProgramRegNum(inst->operand[0])));
      return;
    }
    case AARCH64_OP(extr):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeExtract(
                      is_64bit, dest, ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[1]),
                      (int)TargetIntValue(inst->operand[2])));
      return;
    case AARCH64_OP(bfi):
    case AARCH64_OP(bfxil):
    case AARCH64_OP(sbfiz):
    case AARCH64_OP(ubfiz):
    case AARCH64_OP(sbfx):
    case AARCH64_OP(ubfx): {
      int lsb = (int)TargetIntValue(inst->operand[1]);
      int width = (int)TargetIntValue(inst->operand[2]);
      int modulus = is_64bit ? 64 : 32;
      bool insert =
          inst->opcode == (TargetOpcode)AARCH64_OP(bfi) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(sbfiz) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(ubfiz);
      int opc = inst->opcode == (TargetOpcode)AARCH64_OP(bfi) ||
                        inst->opcode == (TargetOpcode)AARCH64_OP(bfxil)
                    ? 1
                    : inst->opcode == (TargetOpcode)AARCH64_OP(sbfiz) ||
                              inst->opcode == (TargetOpcode)AARCH64_OP(sbfx)
                          ? 0
                          : 2;
      int immr = insert ? (modulus - lsb) & (modulus - 1) : lsb;
      int imms = insert ? width - 1 : lsb + width - 1;
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeBitfield(
                      is_64bit, opc, dest,
                      ProgramRegNum(inst->operand[0]), immr, imms));
      return;
    }
    case AARCH64_OP(sbxt):
    case AARCH64_OP(sbxtb):
    case AARCH64_OP(sbxth):
    case AARCH64_OP(sxtb):
    case AARCH64_OP(sxth):
    case AARCH64_OP(sxtw):
    case AARCH64_OP(ubxtb):
    case AARCH64_OP(ubxth): {
      int imms =
          inst->opcode == (TargetOpcode)AARCH64_OP(sbxtb) ||
                  inst->opcode == (TargetOpcode)AARCH64_OP(sxtb) ||
                  inst->opcode == (TargetOpcode)AARCH64_OP(ubxtb)
              ? 7
              : inst->opcode == (TargetOpcode)AARCH64_OP(sbxth) ||
                        inst->opcode == (TargetOpcode)AARCH64_OP(sxth) ||
                        inst->opcode == (TargetOpcode)AARCH64_OP(ubxth)
                    ? 15
                    : 31;
      bool zero = inst->opcode == (TargetOpcode)AARCH64_OP(ubxtb) ||
                  inst->opcode == (TargetOpcode)AARCH64_OP(ubxth);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeBitfield(
                      is_64bit, zero ? 2 : 0, dest,
                      ProgramRegNum(inst->operand[0]), 0, imms));
      return;
    }
    case AARCH64_OP(ubxt):
      ProgramMoveRegister(module, dest, ProgramRegNum(inst->operand[0]),
                          is_64bit, false);
      return;
    default:
      break;
  }

  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(ldr):
    case AARCH64_OP(ldur):
    case AARCH64_OP(ldrb):
    case AARCH64_OP(ldrh):
    case AARCH64_OP(ldurb):
    case AARCH64_OP(ldurh):
    case AARCH64_OP(ldrsb):
    case AARCH64_OP(ldrsh):
    case AARCH64_OP(ldursb):
    case AARCH64_OP(ldursh):
    case AARCH64_OP(ldursw):
    case AARCH64_OP(fldr): {
      AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
      int size = op == AARCH64_OP(ldrb) || op == AARCH64_OP(ldurb) ||
                         op == AARCH64_OP(ldrsb) ||
                         op == AARCH64_OP(ldursb)
                     ? 0
                     : op == AARCH64_OP(ldrh) || op == AARCH64_OP(ldurh) ||
                               op == AARCH64_OP(ldrsh) ||
                               op == AARCH64_OP(ldursh)
                           ? 1
                           : op == AARCH64_OP(ldursw)
                                 ? 2
                                 : GetRegisterSize(inst) == kSize128Bit ? 4
                                 : ProgramIs64(inst) ? 3 : 2;
      bool sign = op == AARCH64_OP(ldrsb) || op == AARCH64_OP(ldrsh) ||
                  op == AARCH64_OP(ldursb) || op == AARCH64_OP(ldursh) ||
                  op == AARCH64_OP(ldursw);
      int offset = TargetIsConst(inst->operand[1])
                       ? (int)TargetIntValue(inst->operand[1])
                       : 0;
      ProgramLoadStore(module, true, op == AARCH64_OP(fldr), size, sign, dest,
                       ProgramRegNum(inst->operand[0]), offset);
      return;
    }
    case AARCH64_OP(str):
    case AARCH64_OP(stur):
    case AARCH64_OP(strb):
    case AARCH64_OP(strh):
    case AARCH64_OP(sturb):
    case AARCH64_OP(sturh):
    case AARCH64_OP(fstr): {
      AARCH64Opcode op = (AARCH64Opcode)inst->opcode;
      int size = op == AARCH64_OP(strb) || op == AARCH64_OP(sturb)
                     ? 0
                     : op == AARCH64_OP(strh) || op == AARCH64_OP(sturh)
                           ? 1
                           : GetRegisterSize(inst) == kSize128Bit ? 4
                           : ProgramIs64(inst) ? 3 : 2;
      int offset = TargetIsConst(inst->operand[2])
                       ? (int)TargetIntValue(inst->operand[2])
                       : 0;
      ProgramLoadStore(module, false, op == AARCH64_OP(fstr), size, false,
                       ProgramRegNum(inst->operand[0]),
                       ProgramRegNum(inst->operand[1]), offset);
      return;
    }
    case AARCH64_OP(cset):
    case AARCH64_OP(csetm): {
      AARCH64AsmCondition condition =
          ProgramInvertCondition(ProgramCondition(inst->operand[0]));
      int op = inst->opcode == (TargetOpcode)AARCH64_OP(csetm);
      int op2 = inst->opcode == (TargetOpcode)AARCH64_OP(cset);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeConditionalSelect(is_64bit, op, op2, dest, 31,
                                                  31, condition));
      return;
    }
    case AARCH64_OP(csel):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeConditionalSelect(
                      is_64bit, 0, 0, dest, ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[1]),
                      ProgramCondition(inst->operand[2])));
      return;
    case AARCH64_OP(csinc):
    case AARCH64_OP(csinv):
    case AARCH64_OP(csneg):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeConditionalSelect(
                      is_64bit,
                      inst->opcode == (TargetOpcode)AARCH64_OP(csinc) ? 0 : 1,
                      inst->opcode == (TargetOpcode)AARCH64_OP(csinv) ? 0 : 1,
                      dest, ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[1]),
                      ProgramCondition(inst->operand[2])));
      return;
    case AARCH64_OP(cinc):
    case AARCH64_OP(cinv):
    case AARCH64_OP(cneg): {
      int op = inst->opcode == (TargetOpcode)AARCH64_OP(cinc) ? 0 : 1;
      int op2 = inst->opcode == (TargetOpcode)AARCH64_OP(cinv) ? 0 : 1;
      int source = ProgramRegNum(inst->operand[0]);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeConditionalSelect(
                      is_64bit, op, op2, dest, source, source,
                      ProgramInvertCondition(
                          ProgramCondition(inst->operand[1]))));
      return;
    }
    case AARCH64_OP(ccmn):
    case AARCH64_OP(ccmp): {
      bool immediate = TargetIsConst(inst->operand[1]);
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeConditionalCompare(
                      ProgramIs64(inst->operand[0]),
                      inst->opcode == (TargetOpcode)AARCH64_OP(ccmp),
                      immediate, ProgramRegNum(inst->operand[0]),
                      immediate ? (int)TargetIntValue(inst->operand[1])
                                : ProgramRegNum(inst->operand[1]),
                      (int)TargetIntValue(inst->operand[2]),
                      ProgramCondition(inst->operand[3])));
      return;
    }
    case AARCH64_OP(adr):
    case AARCH64_OP(adrp): {
      if (TargetIsConst(inst->operand[0])) {
        int64_t offset = TargetIntValue(inst->operand[0]);
        uint32_t word = ((uint32_t)(inst->opcode ==
                                    (TargetOpcode)AARCH64_OP(adrp))
                         << 31) |
                        (0x10u << 24) | (uint32_t)dest;
        if (inst->opcode == (TargetOpcode)AARCH64_OP(adr)) {
          uint32_t immediate = (uint32_t)offset & 0x1fffffu;
          word |= ((immediate & 3u) << 29) |
                  (((immediate >> 2) & 0x7ffffu) << 5);
        }
        AARCH64ProgramEmitWord(module, word);
      } else {
        String name = {0};
        ProgramLabelName(&name, function, inst->operand[0]);
        char prefix[64];
        snprintf(prefix, sizeof(prefix), "%s x%d, ",
                 inst->opcode == (TargetOpcode)AARCH64_OP(adrp) ? "adrp"
                                                                : "adr",
                 dest);
        String text = {0};
        StringAppend(&text, prefix);
        StringAppend(&text, name.value);
        AARCH64ProgramEmitFixup(
            module,
            ((uint32_t)(inst->opcode == (TargetOpcode)AARCH64_OP(adrp))
             << 31) |
                (0x10u << 24) | (uint32_t)dest,
            inst->opcode == (TargetOpcode)AARCH64_OP(adrp)
                ? kAARCH64FixupADRP21
                : kAARCH64FixupADR21,
            inst->opcode == (TargetOpcode)AARCH64_OP(adrp)
                ? R_AARCH64_ADR_PREL_PG_HI21
                : R_AARCH64_ADR_PREL_LO21,
            name.value, 0, inst->opcode == (TargetOpcode)AARCH64_OP(adrp),
            text.value);
        StringDestruct(&text);
        StringDestruct(&name);
      }
      return;
    }
    case AARCH64_OP(gotaddr): {
      String name = {0};
      ProgramLabelName(&name, function, inst->operand[0]);
      char prefix[64];
      snprintf(prefix, sizeof(prefix), "adrp x%d, :got:", dest);
      String text = {0};
      StringAppend(&text, prefix);
      StringAppend(&text, name.value);
      AARCH64ProgramEmitFixup(
          module, (1u << 31) | (0x10u << 24) | (uint32_t)dest,
          kAARCH64FixupRelocationOnly, R_AARCH64_ADR_GOT_PAGE, name.value, 0,
          true, text.value);
      StringClear(&text);
      snprintf(prefix, sizeof(prefix), "ldr x%d, [x%d, :got_lo12:", dest,
               dest);
      StringAppend(&text, prefix);
      StringAppend(&text, name.value);
      StringAppendChar(&text, ']');
      AARCH64ProgramEmitFixup(
          module,
          AARCH64EncodeLoadStoreUnsigned(3, false, 1, dest, dest, 0),
          kAARCH64FixupRelocationOnly, R_AARCH64_LD64_GOT_LO12_NC, name.value,
          0, true, text.value);
      StringDestruct(&text);
      StringDestruct(&name);
      return;
    }
    case AARCH64_OP(spill): {
      int offset = (int)TargetIntValue(inst->operand[1]) +
                   emitter->first_spill_offset;
      int size = GetRegisterSize(inst) == kSize128Bit ? 4 : 3;
      ProgramSpillAddress(module, offset);
      ProgramLoadStore(module, false, ProgramRegIsFP(inst), size, false, dest, 16,
                       0);
      return;
    }
    case AARCH64_OP(reload): {
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) +
                   emitter->first_spill_offset;
      int size = GetRegisterSize(spill) == kSize128Bit ? 4 : 3;
      ProgramSpillAddress(module, offset);
      ProgramLoadStore(module, true, ProgramRegIsFP(inst), size, false, dest, 16,
                       0);
      return;
    }
    case AARCH64_OP(atomic_load):
    case AARCH64_OP(atomic_store):
    case AARCH64_OP(atomic_fetch_add):
    case AARCH64_OP(atomic_fetch_sub):
    case AARCH64_OP(atomic_add_fetch):
    case AARCH64_OP(atomic_sub_fetch):
    case AARCH64_OP(atomic_compare_exchange_bool):
    case AARCH64_OP(atomic_compare_exchange_val):
    case AARCH64_OP(atomic_compare_exchange_n):
    case AARCH64_OP(atomic_fence):
      ProgramEmitAtomic(emitter, inst, function, module);
      return;
    case AARCH64_OP(nop):
      AARCH64ProgramEmitWord(module, 0xd503201fu);
      return;
    case AARCH64_OP(ldxr):
    case AARCH64_OP(ldaxr):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeExclusiveLoad(
                      is_64bit ? 3 : 2,
                      inst->opcode == (TargetOpcode)AARCH64_OP(ldaxr), dest,
                      ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(stxr):
    case AARCH64_OP(stlxr):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeExclusiveStore(
                      ProgramIs64(inst->operand[1]) ? 3 : 2,
                      inst->opcode == (TargetOpcode)AARCH64_OP(stlxr),
                      ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[1]),
                      ProgramRegNum(inst->operand[2])));
      return;
    case AARCH64_OP(ldar):
    case AARCH64_OP(stlr):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeAcquireRelease(
                      is_64bit ? 3 : 2,
                      inst->opcode == (TargetOpcode)AARCH64_OP(ldar),
                      inst->opcode == (TargetOpcode)AARCH64_OP(ldar)
                          ? dest
                          : ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[inst->opcode ==
                                                         (TargetOpcode)
                                                             AARCH64_OP(ldar)
                                                     ? 0
                                                     : 1])));
      return;
    case AARCH64_OP(dmb):
      AARCH64ProgramEmitWord(module, AARCH64EncodeDmb(11));
      return;
    case AARCH64_OP(clrex):
      AARCH64ProgramEmitWord(module, 0xd5033f5fu);
      return;
    default:
      break;
  }

  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(vadd):
    case AARCH64_OP(vsub):
    case AARCH64_OP(vand):
    case AARCH64_OP(vorr):
    case AARCH64_OP(veor):
    case AARCH64_OP(vcmeq):
    case AARCH64_OP(vcmgt):
    case AARCH64_OP(vcmge):
    case AARCH64_OP(vcmhi):
    case AARCH64_OP(vcmhs):
    case AARCH64_OP(vfadd):
    case AARCH64_OP(vfsub):
    case AARCH64_OP(vfmul):
    case AARCH64_OP(vfdiv): {
      int q = GetRegisterSize(inst) == kSize128Bit ? 1 : 0;
      int size_log = (inst->flags >> AARCH64_SIMD_ELEM_SHIFT) & 3;
      uint32_t base;
      bool bitwise = inst->opcode == (TargetOpcode)AARCH64_OP(vand) ||
                     inst->opcode == (TargetOpcode)AARCH64_OP(vorr) ||
                     inst->opcode == (TargetOpcode)AARCH64_OP(veor);
      if (inst->opcode == (TargetOpcode)AARCH64_OP(vfadd) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(vfsub) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(vfmul) ||
          inst->opcode == (TargetOpcode)AARCH64_OP(vfdiv)) {
        // Vector FP three-same: size=00 single, size=01 double.
        int fp_size = size_log == 3 ? 1 : 0;
        base = inst->opcode == (TargetOpcode)AARCH64_OP(vfadd) ? 0x0e20d400u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vfsub) ? 0x0ea0d400u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vfmul) ? 0x0e20dc00u :
                                                                0x0e20fc00u;
        base |= ((uint32_t)q << 30) | ((uint32_t)fp_size << 22);
      } else {
        base = inst->opcode == (TargetOpcode)AARCH64_OP(vadd) ? 0x0e208400u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vsub) ? 0x2e208400u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vand) ? 0x0e201c00u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vorr) ? 0x0ea01c00u :
               inst->opcode == (TargetOpcode)AARCH64_OP(veor) ? 0x2e201c00u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vcmeq) ? 0x2e208c00u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vcmgt) ? 0x0e203400u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vcmge) ? 0x0e203c00u :
               inst->opcode == (TargetOpcode)AARCH64_OP(vcmhi) ? 0x2e203400u :
                                                                0x2e203c00u;
        base |= ((uint32_t)q << 30);
        if (!bitwise) {
          base |= ((uint32_t)size_log << 22);
        }
      }
      AARCH64ProgramEmitWord(
          module, base | (ProgramRegNum(inst->operand[1]) << 16) |
                      (ProgramRegNum(inst->operand[0]) << 5) | dest);
      return;
    }
    case AARCH64_OP(fadd):
    case AARCH64_OP(fsub):
    case AARCH64_OP(fmul):
    case AARCH64_OP(fdiv):
    case AARCH64_OP(fmin):
    case AARCH64_OP(fmax): {
      int opcode = inst->opcode == (TargetOpcode)AARCH64_OP(fadd)
                       ? 0xa
                       : inst->opcode == (TargetOpcode)AARCH64_OP(fsub)
                             ? 0xe
                             : inst->opcode == (TargetOpcode)AARCH64_OP(fmul)
                                   ? 2
                                   : inst->opcode ==
                                             (TargetOpcode)AARCH64_OP(fdiv)
                                         ? 6
                                         : inst->opcode ==
                                                   (TargetOpcode)AARCH64_OP(fmin)
                                               ? 0x16
                                               : 0x12;
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeFPDataProcessing2(
                      is_64bit, opcode, dest,
                      ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[1])));
      return;
    }
    case AARCH64_OP(fsqrt):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeFPSqrt(is_64bit, dest,
                                      ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(fmov):
      if (ProgramRegIsFP(inst) == ProgramRegIsFP(inst->operand[0])) {
        ProgramMoveRegister(module, dest, ProgramRegNum(inst->operand[0]),
                            is_64bit, true);
      } else {
        AARCH64ProgramEmitWord(
            module, AARCH64EncodeFPBitcast(
                        ProgramRegIsFP(inst), is_64bit, dest,
                        ProgramRegNum(inst->operand[0])));
      }
      return;
    case AARCH64_OP(fneg):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeFPNegate(is_64bit, dest,
                                        ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(fcmp):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeFPCompare(
                      is_64bit,
                      ProgramRegNum(inst->operand[0]),
                      ProgramRegNum(inst->operand[1])));
      return;
    case AARCH64_OP(fcvtsd):
    case AARCH64_OP(fcvtds):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeFPConvertPrecision(
                      inst->opcode == (TargetOpcode)AARCH64_OP(fcvtds),
                      inst->opcode == (TargetOpcode)AARCH64_OP(fcvtsd), dest,
                      ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(scvtf):
    case AARCH64_OP(ucvtf):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeIntToFP(
                      ProgramIs64(inst->operand[0]), is_64bit,
                      inst->opcode == (TargetOpcode)AARCH64_OP(ucvtf), dest,
                      ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(fcvtns):
    case AARCH64_OP(fcvtnu):
    case AARCH64_OP(fcvtzs):
    case AARCH64_OP(fcvtzu):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeFPToInt(
                      inst->opcode == (TargetOpcode)AARCH64_OP(fcvtzs) ||
                              inst->opcode == (TargetOpcode)AARCH64_OP(fcvtzu)
                          ? 0x1e380000u
                          : 0x1e200000u,
                      is_64bit, ProgramIs64(inst->operand[0]),
                      inst->opcode == (TargetOpcode)AARCH64_OP(fcvtnu) ||
                          inst->opcode == (TargetOpcode)AARCH64_OP(fcvtzu),
                      dest, ProgramRegNum(inst->operand[0])));
      return;
    case AARCH64_OP(fcvt):
      AARCH64ProgramEmitWord(
          module, AARCH64EncodeFPBitcast(
                      ProgramRegIsFP(inst), is_64bit, dest,
                      ProgramRegNum(inst->operand[0])));
      return;
    default:
      ProgramUnsupportedInstruction(inst, module);
      return;
  }
}

void AARCH64EmitterInit(AARCH64Emitter* emitter, AARCH64Generator* g) {
  emitter->g = g;
  emitter->regs = &g->register_allocator;
  emitter->saved_reg_offset = 0;
  emitter->spill_region_size = g->register_allocator.max_spilled_region_size;
  emitter->first_spill_offset = 0;
  emitter->current_block = NULL;
}

AARCH64Emitter* NewAARCH64Emitter(AARCH64Generator* rv) {
  AARCH64Emitter* emitter = malloc(sizeof(AARCH64Emitter));
  AARCH64EmitterInit(emitter, rv);
  return emitter;
}

void AARCH64EmitterDestruct(AARCH64Emitter* emitter) {
}

void AARCH64EmitterDelete(AARCH64Emitter* emitter) {
  AARCH64EmitterDestruct(emitter);
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

static void AARCH64FillLSDAInfo(AARCH64Emitter* emitter, const char* func_name,
                                DaveEHLSDARange* lsda_ranges, size_t* count) {
  *count = 0;
  for (size_t i = 0; i < emitter->g->exception_ranges.length &&
                     *count < 64;
       i++) {
    AARCH64ExceptionRange* range = emitter->g->exception_ranges.value.p[i];
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
// through this frame recovers the caller's values instead of passing on
// whatever the throwing code left behind.  The registers are listed in the
// order SaveRegisters stores them, at ascending offsets from sp, which the
// prologue leaves stack_frame_size below x29 + 16 (= the CFA).  AArch64 numbers
// its DWARF integer registers as the architecture does, which is also what the
// register allocator uses, so no translation is needed.  Only the integer
// registers are described: the unwind context has no room for the
// floating-point ones.
static size_t AARCH64SavedCFIRegisters(AARCH64Emitter* emitter,
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
      saved_regs[count].cfa_offset =
          offset - stack_frame_size - AARCH64_STACK_FRAME_HEADER_SIZE;
      count++;
    }
    offset -= 8;
    BitSetIteratorNext(&it);
  }
  return count;
}

static void AARCH64PrintEHMetadata(AARCH64Emitter* emitter, FILE* fp,
                                   const char* func_name) {
  if (emitter->g->base.varargs) {
    return;
  }
  DaveEHLSDARange lsda_ranges[64];
  DaveEHFrameSavedReg saved_regs[AARCH64_NUM_INT_REGS];
  size_t lsda_count = 0;
  size_t saved_reg_count =
      AARCH64SavedCFIRegisters(emitter, saved_regs,
                               sizeof(saved_regs) / sizeof(saved_regs[0]));
  AARCH64FillLSDAInfo(emitter, func_name, lsda_ranges, &lsda_count);
  DaveEHFrameEmitInfo info = {
      .ranges = lsda_ranges,
      .range_count = lsda_count,
      .func_name = func_name,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = true,
      .cie_ra_reg = 30,
      .cie_cfa_reg = 31,
      .cie_fp_reg = 29,
      .entry_cfa_offset = 0,
      .frame_cfa_offset = 16,
      .fp_cfa_offset = 16,
      .saved_fp_offset = -16,
      .saved_ra_offset = -8,
      .saved_regs = saved_regs,
      .saved_reg_count = saved_reg_count,
  };
  DaveEHPrintGCCExceptTable(fp, &info);
  DaveEHPrintEHFrameCIE(fp, &info, "");
  DaveEHPrintEHFrameFDE(fp, &info, "");
}

static void AARCH64EmitEHMetadata(AARCH64Emitter* emitter, AsmModule* module,
                                  const char* function) {
  if (emitter->g->base.varargs) {
    return;
  }
  DaveEHLSDARange ranges[64];
  DaveEHFrameSavedReg saved_regs[AARCH64_NUM_INT_REGS];
  size_t range_count = 0;
  size_t saved_count =
      AARCH64SavedCFIRegisters(emitter, saved_regs,
                               sizeof(saved_regs) / sizeof(saved_regs[0]));
  AARCH64FillLSDAInfo(emitter, function, ranges, &range_count);
  DaveEHFrameEmitInfo info = {
      .ranges = ranges,
      .range_count = range_count,
      .func_name = function,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = true,
      .cie_ra_reg = 30,
      .cie_cfa_reg = 31,
      .cie_fp_reg = 29,
      .entry_cfa_offset = 0,
      .frame_cfa_offset = 16,
      .fp_cfa_offset = 16,
      .saved_fp_offset = -16,
      .saved_ra_offset = -8,
      .saved_regs = saved_regs,
      .saved_reg_count = saved_count,
  };
  DaveEHEmitGCCExceptTable(module, &info);
  DaveEHEmitEHFrameCIE(module, &info, "");
  DaveEHEmitEHFrameFDE(module, &info, "");
}

void AARCH64PrintFunction(AARCH64Emitter* emitter, FILE* fp) {
  const char* func_name = emitter->g->base.function_name.value;
  EmitFunctionSection(fp, func_name);
  if (emitter->g->base.is_weak) {
    fprintf(fp, "\t.weak %s\n", func_name);
  } else if (emitter->g->base.is_global) {
    fprintf(fp, "\t.global %s\n", func_name);
  } else {
    fprintf(fp, "\t.local  %s\n", func_name);
  }
  fprintf(fp, "\t.type %s, @function\n\n", func_name);
  fprintf(fp, "%s:\n", func_name);
  if (compiler->pic && emitter->g->base.is_global &&
      strcmp(func_name, "main") != 0) {
    fprintf(fp, "\t.word 0xD503241F  // bti c\n");
  }
  TargetInstruction* inst = TargetFirstInstruction(&emitter->g->base);
  while (inst != NULL) {
    PrintInstruction(emitter, inst, func_name, fp);
    inst = TargetNext(inst);
  }
  fprintf(fp, ".func_end_%s:\n", func_name);
  fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
  AARCH64PrintEHMetadata(emitter, fp, func_name);
}

void AARCH64EmitFunctionToModule(AARCH64Emitter* emitter, AsmModule* module) {
  const char* function = emitter->g->base.function_name.value;
  EmitFunctionSectionToModule(module, function);
  AsmModuleSymbol(module, function, SYM_TYPE(func),
                  emitter->g->base.is_weak
                      ? SYM_BIND(weak)
                      : emitter->g->base.is_global ? SYM_BIND(global)
                                                   : SYM_BIND(local),
                  0, 4, false, true, false);
  AsmModuleLabel(module, function);
  if (compiler->pic && emitter->g->base.is_global &&
      strcmp(function, "main") != 0) {
    AsmModuleComment(module, "bti c");
    AARCH64ProgramEmitWord(module, 0xd503241fu);
  }
  for (TargetInstruction* inst = TargetFirstInstruction(&emitter->g->base);
       inst != NULL && !module->failed; inst = TargetNext(inst)) {
    ProgramEmitInstruction(emitter, inst, function, module);
  }
  String end = {0};
  ProgramNamedFunctionLabel(&end, ".func_end_", function, "");
  AsmModuleLabel(module, end.value);
  AsmExpr size;
  AsmExprInitDifference(&size, end.value, function, 0);
  AsmModuleSymbolSize(module, function, &size);
  AsmExprDestruct(&size);
  StringDestruct(&end);

  AARCH64EmitEHMetadata(emitter, module, function);
}

void AARCH64EmitFunction(AARCH64Emitter* emitter, FILE* text_out) {
  AARCH64PrintFunction(emitter, text_out);
}

void AARCH64PrintCXXAdjustorThunks(FILE* fp) {
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
    if (thunk->this_adjustment > 0) {
      fprintf(fp, "\tadd x0, x0, #%d\n", thunk->this_adjustment);
    } else if (thunk->this_adjustment < 0) {
      fprintf(fp, "\tsub x0, x0, #%d\n", -thunk->this_adjustment);
    }
    fprintf(fp, "\tb %s\n", target_name);
    fprintf(fp, ".func_end_%s:\n", thunk_name);
    fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", thunk_name, thunk_name,
            thunk_name);
  }
}

void AARCH64EmitCXXAdjustorThunksToModule(AsmModule* module) {
  if (compiler->cxx_this_adjustor_thunks.length == 0) {
    return;
  }
  for (size_t i = 0; i < compiler->cxx_this_adjustor_thunks.length; i++) {
    CXXThisAdjustorThunk* thunk =
        compiler->cxx_this_adjustor_thunks.value.p[i];
    if (thunk == NULL || thunk->thunk == NULL || thunk->target == NULL) {
      continue;
    }
    char thunk_buf[1], target_buf[1];
    const char* thunk_name =
        TargetSymbolName(thunk->thunk, thunk_buf, sizeof(thunk_buf));
    const char* target_name =
        TargetSymbolName(thunk->target, target_buf, sizeof(target_buf));
    EmitFunctionSectionToModule(module, thunk_name);
    AsmModuleSymbol(module, thunk_name, SYM_TYPE(func), SYM_BIND(weak), 0, 4,
                    false, true, false);
    AsmModuleLabel(module, thunk_name);
    if (thunk->this_adjustment != 0) {
      ProgramAddSubImmediate(module, 0, 0, true,
                             thunk->this_adjustment > 0,
                             thunk->this_adjustment > 0
                                 ? thunk->this_adjustment
                                 : -thunk->this_adjustment);
    }
    ProgramBranch(module, "b",
                  AARCH64EncodeUnconditionalBranchImmediate(0, false),
                  kAARCH64FixupBranch26, R_AARCH64_JUMP26, target_name, true);
    String end = {0};
    ProgramNamedFunctionLabel(&end, ".func_end_", thunk_name, "");
    AsmModuleLabel(module, end.value);
    AsmExpr size;
    AsmExprInitDifference(&size, end.value, thunk_name, 0);
    AsmModuleSymbolSize(module, thunk_name, &size);
    AsmExprDestruct(&size);
    StringDestruct(&end);
  }
}

