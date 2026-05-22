//
//  aarch64_emitter.c
//  c_compiler
//
//  Created by David Allison on 7/20/22.
//  Copyright © 2022 David Allison. All rights reserved.
//

#include "aarch64_emitter.h"
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include "compiler.h"
#include "aarch64_assembler.h"
#include "aarch64_codegen.h"
#include "aarch64_reg_alloc.h"
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
  switch ((AARCH64Opcode)inst->opcode) {
    case AARCH64_OP(tmp):
    case AARCH64_OP(fp):
    case AARCH64_OP(sp):
    case AARCH64_OP(lr):
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
static int StackFrameSize(AARCH64Emitter* emitter) {
  // Start off with local variable space.  This also includes
  // 16 bytes for the saved x29 and x30.
  int stack_frame_size = emitter->g->base.stack_frame_size + 16;

  bool varargs = emitter->g->base.varargs;

  if (varargs) {
    if (emitter->g->num_int_arg_regs < AARCH64_NUM_INT_ARGS) {
      // All args other than those declared and in registers must
      // be saved to the stack above the frame pointer and directly
      // under the first pushed arg->base.  This adds to the stack frame size.
      stack_frame_size += (AARCH64_NUM_INT_ARGS - emitter->g->num_int_arg_regs) * 8;
    }
  }
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
  return emitter->g->base.stack_frame_size == 0 &&
         emitter->g->base.num_calls == 0 && !emitter->g->not_leaf;
}

static void DecrementStackPointer(AARCH64Emitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  AddSubImmediate(emitter, "sp",  NULL, /*add=*/false, stack_frame_size, NULL, fp);
}

static void IncrementStackPointer(AARCH64Emitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size <= 0) {
    return;
  }
  AddSubImmediate(emitter, "sp",  NULL, /*add=*/true, stack_frame_size, NULL, fp);
}

// Load a floating point or integer register from the stack frame.
static void LoadRegisterFromFrame(AARCH64Emitter* emitter, int reg,
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

static void GenerateOffsetFromFrame(AARCH64Emitter* emitter, int reg,
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
  // LET S = stack frame size + 8 (for frame pointer save)
  // sub sp, sp, #S
  // stp x29,x30, [sp, #-16]
  // add x29, sp, #S-16
 
  int stack_frame_size = StackFrameSize(emitter);

  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
          !emitter->g->not_leaf;
  bool varargs = emitter->g->base.varargs;
  int space_above_frame_pointer =
      varargs ? (AARCH64_NUM_INT_ARGS - emitter->g->num_int_arg_regs) * 8 : 0;

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
  int return_address_offset = stack_frame_size - 8 - space_above_frame_pointer;
  int frame_pointer_offset = stack_frame_size - 16 - space_above_frame_pointer;

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

  // A leaf procedure doesn't save the return address.
  if (is_leaf) {
    frame_pointer_offset += 8;
    saved_reg_offset += 8;
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame, no need to store frame pointer.
  } else {
    fprintf(fp, "sub sp, sp, #32\n");
    fprintf(fp, "stp x29, x30, [sp, #16]\n");
    fprintf(fp, "add x29, sp, #16\n");
    DecrementStackPointer(emitter, stack_frame_size, fp);

    if (varargs) {
      int num_pushed_arg_regs = AARCH64_NUM_INT_ARGS - emitter->g->num_int_arg_regs;
      int offset_from_frame_pointer = 0;
      fprintf(fp, "\t// varargs function with %d declared args\n",
              emitter->g->num_int_arg_regs);
      for (int i = AARCH64_NUM_INT_ARGS - num_pushed_arg_regs; i < AARCH64_NUM_INT_ARGS;
           i++) {
        fprintf(fp, "\tstr x%d, [sp, #-8]!\n", i);
        offset_from_frame_pointer += 8;
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
    fprintf(fp, "\tstr %s, [%s, #%d]\n",
            AARCH64RegisterNameFromNum(saved_reg->reg_num, kAARCH64RegTypeInt, kSize64Bit, buf1,
                                  sizeof(buf1)),
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
  // ldr s0, S-8(sp)   - restore frame pointer.
  // addi sp, sp, S   - increment sp

  int stack_frame_size = StackFrameSize(emitter);
  char buf1[8];

  bool is_leaf = emitter->g->base.num_calls == 0 && OptLevel1() &&
          !emitter->g->not_leaf;
  bool varargs = emitter->g->base.varargs;
  int space_above_frame_pointer =
      varargs ? (AARCH64_NUM_INT_ARGS - emitter->g->num_int_arg_regs) * 8 : 0;

  if (space_above_frame_pointer < 0) {
    space_above_frame_pointer = 0;
  }
  int frame_pointer_offset = stack_frame_size - 16 - space_above_frame_pointer;
  // int return_address_offset = stack_frame_size - 8 - space_above_frame_pointer;

  // A leaf procedure doesn't save the return address.
  if (is_leaf) {
    frame_pointer_offset += 8;
  } else {
    fprintf(fp, "\t// Restored registers.\n");
  }
  Vector regs = {0};

  int offset = emitter->saved_reg_offset;

  BitSetIterator it;

  BitSetIteratorStart(&it, &emitter->regs->used_float_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tfldr %s, [sp, #%d]\n",
            AARCH64RegisterNameFromNum(reg, kAARCH64RegTypeFloat, kSize64Bit, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tldr %s, [sp, #%d]\n",
            AARCH64RegisterNameFromNum(reg, kAARCH64RegTypeInt, kSize64Bit, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  if (EmptyStackFrame(emitter)) {
    // Empty stack frame.
  } else {
    IncrementStackPointer(emitter, stack_frame_size-32, fp);
    fprintf(fp, "\tldp x29, x30, [sp, #16]\n");
    fprintf(fp, "\tadd sp, sp, #32\n");
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
  if (inst->dest != NULL) {
    return AARCH64RegisterName((AARCH64Register*)inst->dest->reg, size, buf, bufsize);
  }
  return AARCH64RegisterName((AARCH64Register*)inst->reg, size, buf, bufsize);
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
  if (inst->opcode == AARCH64_OP(label)) {
    if ((inst->flags & AARCH64_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    return;
  }

  if (inst->opcode == AARCH64_OP(named_label)) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }
  
  if (inst->opcode == AARCH64_OP(ivarreg) || inst->opcode == AARCH64_OP(fvarreg)) {
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

  int reg_size = GetRegisterSize(inst);
  
  // Special case instructions.
  switch ((AARCH64Opcode)inst->opcode) {
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
        fprintf(fp, "\t.local %s\n", sym->symbol->name.value);
      } else {
        fprintf(fp, "\t.global %s\n", sym->symbol->name.value);
      }
      return;
    }
    // case AARCH64_OP(bl):
    case AARCH64_OP(bl): {
      assert(inst->operand[0]->opcode == AARCH64_OP(symbol));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      fprintf(fp, "\t%-12s%s\n", "bl", sym->symbol->name.value);
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

      // Output text directly into assembly output.
      fprintf(fp, "\t%s\n", lit->value.value);
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
#if 0
      AARCH64Register* reg = (AARCH64Register*)inst->reg;
      int offset = (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      const int spill_addr = AARCH64_SPILL_ADDR;
      if (!AARCH64IsPossibleImmediate(offset)) {
        fprintf(fp, "\t%-12s%s, %d\n",
                "lui",
                AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, reg_size, buf1, sizeof(buf1)),
                offset >> 12);
        fprintf(fp, "\t%-12s%s, s0, %s\n",
                "sub",
                AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, reg_size, buf1, sizeof(buf1)),
                AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, reg_size, buf2, sizeof(buf2)));
        fprintf(fp, "\t%-12s%s, -%d(%s)\t// Spilled @%d\n",
                 reg->type == kAARCH64RegTypeInt ? "sd" : "fsd",
                 AARCH64RegisterName(reg, buf1, sizeof(buf1)),
                 offset & 0xfff,
                 AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, reg_size, buf2, sizeof(buf2)),
                 inst->operand[0]->id);
      } else {
        fprintf(fp, "\t%-12s%s, -%d(s0)\t// Spilled @%d\n",
                 reg->type == kAARCH64RegTypeInt ? "sd" : "fsd",
                 AARCH64RegisterName(reg, buf1, sizeof(buf1)),
                 offset,
                 inst->operand[0]->id);
      }
#endif
      return;
    }
      
    case AARCH64_OP(reload): {
#if 0
      AARCH64Register* reg = (AARCH64Register*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset = (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      const int spill_addr = AARCH64_SPILL_ADDR;
      if (!AARCH64IsPossibleImmediate(offset)) {
        fprintf(fp, "\t%-12s%s, %d\n",
                "lui",
                AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, buf1, sizeof(buf1)),
                offset >> 12);
        fprintf(fp, "\t%-12s%s, s0, %s\n",
                "sub",
                AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, buf1, sizeof(buf1)),
                AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, buf2, sizeof(buf2)));
        fprintf(fp, "\t%-12s%s, -%d(%s)\t// Reloaded spilled @%d\n",
                 reg->type == kAARCH64RegTypeInt ? "ld" : "fld",
                 AARCH64RegisterName(reg, buf1, sizeof(buf1)),
                 offset & 0xfff,
                 AARCH64RegisterNameFromNum(spill_addr, kAARCH64RegTypeInt, buf2, sizeof(buf2)),
                 spill->operand[0]->id);
      } else {
        fprintf(fp, "\t%-12s%s, -%d(s0)\t// Reloaded spilled @%d\n",
                 reg->type == kAARCH64RegTypeInt ? "ld" : "fld",
                 AARCH64RegisterName(reg, buf1, sizeof(buf1)),
                 offset,
                 spill->operand[0]->id);
      }
#endif
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
      } else if (inst->operand[1]->opcode == AARCH64_OP(symbol)) {
        assert((inst->flags & AARCH64_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst, reg_size,buf1, sizeof(buf1)),
                ((TargetSymbol*)inst->operand[1])->symbol->name.value,
                GetRegisterName(inst->operand[0], reg_size,buf2,
                               sizeof(buf2)));
      } else if (inst->operand[1]->opcode == AARCH64_OP(label)) {
        assert((inst->flags & AARCH64_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst, reg_size,buf1, sizeof(buf1)),
                func_name, inst->operand[1]->id,
                GetRegisterName(inst->operand[0], reg_size,buf2,
                               sizeof(buf2)));
#endif
      } else if (inst->operand[1]->opcode == AARCH64_OP(zr)) {
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
      } else if (inst->operand[2]->opcode == AARCH64_OP(symbol)) {
        assert((inst->flags & AARCH64_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst->operand[0], reg_size,buf1,
                               sizeof(buf1)),
                ((TargetSymbol*)inst->operand[2])->symbol->name.value,
                GetRegisterName(inst->operand[1],reg_size, buf2,
                               sizeof(buf2)));
      } else if (inst->operand[2]->opcode == AARCH64_OP(label)) {
        assert((inst->flags & AARCH64_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst->operand[0], reg_size,buf1,
                               sizeof(buf1)),
                func_name, inst->operand[2]->id,
                GetRegisterName(inst->operand[1],reg_size, buf2,
                               sizeof(buf2)));
#endif
      } else if (inst->operand[2]->opcode == AARCH64_OP(zr)) {
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
      char condbuf[8] = {0};
      if (cond->opcode != AARCH64_OP(al)) {
        snprintf(condbuf, sizeof(condbuf), ".%s", AARCH64OpcodeName(cond->opcode));
      }
       fprintf(fp, "%s .%s_label_%d\n",
               condbuf,
               func_name, inst->operand[1]->id);
      break;
    }
      
    case AARCH64_OP(br):
      fprintf(fp, "%s\n",
              GetRegisterName(inst->operand[0], kSize64Bit, buf1,
                             sizeof(buf1)));
      break;
      
    case AARCH64_OP(blr):
      assert(inst->operand[0] != NULL);
      fprintf(fp, "%s\n",
              GetRegisterName(inst->operand[0], kSize64Bit, buf1,
                             sizeof(buf1)));
      break;

    case AARCH64_OP(mov):
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
          } else if (inst->operand[i]->opcode == AARCH64_OP(symbol)) {
            if ((inst->flags & AARCH64_HI_RELOC) != 0) {
              fprintf(fp, "%s%%hi(%s)", sep,
                      ((TargetSymbol*)inst->operand[i])->symbol->name.value);
            } else {
              fprintf(fp, "%s%s", sep,
                      ((TargetSymbol*)inst->operand[i])->symbol->name.value);
            }
          } else if (inst->operand[i]->opcode == AARCH64_OP(literal)) {
            TargetLiteral* literal = (TargetLiteral*)inst->operand[i];
            fprintf(fp, "%s.str.%d", sep, literal->literal_id);
          } else if (inst->operand[i]->opcode == AARCH64_OP(oplsl)) {
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


void AARCH64PrintFunction(AARCH64Emitter* emitter, FILE* fp) {
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

