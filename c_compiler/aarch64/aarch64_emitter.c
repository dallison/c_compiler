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
#include "compiler.h"
#include "eh_metadata.h"
#include "aarch64_assembler.h"
#include "aarch64_encode.h"
#include "aarch64_codegen.h"
#include "aarch64_object.h"
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

      AARCH64ObjectModule* object_module = emitter->object_module;
      if (object_module != NULL) {
        if ((inst->flags & AARCH64_INST_EXTENDED_ASM) != 0) {
          char* fragment_ptr = NULL;
          size_t fragment_size = 0;
          FILE* fragment_stream =
              open_memstream(&fragment_ptr, &fragment_size);
          if (fragment_stream == NULL) {
            return;
          }
          PrintExtendedAsm(fragment_stream, (AARCH64AsmInstruction*)inst,
                           lit->value.value);
          fclose(fragment_stream);
          AARCH64AssembleFragment(object_module, "extended_asm", fragment_ptr,
                                  fragment_size);
          free(fragment_ptr);
        } else {
          AARCH64AssembleFragment(object_module, "inline_asm", lit->value.value,
                                  lit->value.length);
        }
      } else {
        fprintf(fp, "\t");
        if ((inst->flags & AARCH64_INST_EXTENDED_ASM) != 0) {
          PrintExtendedAsm(fp, (AARCH64AsmInstruction*)inst, lit->value.value);
        } else {
          fprintf(fp, "%s", lit->value.value);
        }
        fprintf(fp, "\n");
      }
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

    case AARCH64_OP(spill): {
      AARCH64Register* reg = (AARCH64Register*)inst->reg;
      int offset = (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      PrintSpillSlotAddress(offset, fp);
      fprintf(fp, "\t%s %s, [x16, #0]\t// Spilled @%d\n",
              reg->type == kAARCH64RegTypeInt ? "str" : "fstr",
              AARCH64RegisterName(reg, kSize64Bit, buf1, sizeof(buf1)),
              inst->operand[0]->id);
      return;
    }
      
    case AARCH64_OP(reload): {
      AARCH64Register* reg = (AARCH64Register*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      PrintSpillSlotAddress(offset, fp);
      fprintf(fp, "\t%s %s, [x16, #0]\t// Reloaded spilled @%d\n",
              reg->type == kAARCH64RegTypeInt ? "ldr" : "fldr",
              AARCH64RegisterName(reg, kSize64Bit, buf1, sizeof(buf1)),
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

void AARCH64EmitterInit(AARCH64Emitter* emitter, AARCH64Generator* g) {
  emitter->g = g;
  emitter->regs = &g->register_allocator;
  emitter->saved_reg_offset = 0;
  emitter->spill_region_size = g->register_allocator.max_spilled_region_size;
  emitter->first_spill_offset = 0;
  emitter->current_block = NULL;
  emitter->object_module = NULL;
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

void AARCH64PrintFunction(AARCH64Emitter* emitter, FILE* fp) {
  const char* func_name = emitter->g->base.function_name.value;
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

void AARCH64EmitFunction(AARCH64Emitter* emitter, FILE* text_out,
                         AARCH64ObjectModule* object_module) {
  emitter->object_module = object_module;
  if (object_module == NULL || !AARCH64CanDirectEncodeFunction(emitter->g)) {
    AARCH64PrintFunction(emitter, text_out);
    if (object_module != NULL) {
      AARCH64ObjectModuleEndFunction(object_module);
    }
    emitter->object_module = NULL;
    return;
  }

  const char* func_name = emitter->g->base.function_name.value;
  if (emitter->g->base.is_weak) {
    fprintf(text_out, "\t.weak %s\n", func_name);
  } else if (emitter->g->base.is_global) {
    fprintf(text_out, "\t.global %s\n", func_name);
  } else {
    fprintf(text_out, "\t.local  %s\n", func_name);
  }
  fprintf(text_out, "\t.type %s, @function\n\n", func_name);
  fprintf(text_out, "%s:\n", func_name);
  if (compiler->pic && emitter->g->base.is_global &&
      strcmp(func_name, "main") != 0) {
    fprintf(text_out, "\t.word 0xD503241F  // bti c\n");
  }
  AARCH64ObjectModuleEndFunction(object_module);
  if (!AARCH64ObjectModuleAppendFunction(object_module, emitter->g)) {
    emitter->object_module = NULL;
    return;
  }

  FILE* suffix = AARCH64ObjectModuleAssemblyStream(object_module);
  if (suffix == NULL) {
    emitter->object_module = NULL;
    return;
  }
  fprintf(suffix, ".func_end_%s:\n", func_name);
  fprintf(suffix, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
  AARCH64ObjectModuleEndFunction(object_module);
  emitter->object_module = NULL;
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

