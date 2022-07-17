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
#include <stdlib.h>
#include <inttypes.h>
#include "compiler.h"
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
    case RV_OP(nrvoval):
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
  stack_frame_size += emitter->spill_region_size;
  
  stack_frame_size = (stack_frame_size + 15) & ~15;  // Aligned to 16 bytes.

  return stack_frame_size;
}

static bool EmptyStackFrame(RVEmitter* emitter) {
  return emitter->rv->base.stack_frame_size == 0 &&
         emitter->rv->base.num_calls == 0 && !emitter->rv->not_leaf;
}

static void DecrementStackPointer(RVEmitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size > 0x7ff) {
    // Too big for an immediate.  Load into t0 and use a sub instruction.
    fprintf(fp, "\tlui t0, %d\n", stack_frame_size >> 12);
    fprintf(fp, "\taddi t0, t0, %d\n", stack_frame_size & 0xfff);
    fprintf(fp, "\tsub sp, sp, t0\n");
  } else {
    fprintf(fp, "\taddi sp, sp, -%d\n", stack_frame_size);
  }
}

static void IncrementStackPointer(RVEmitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size > 0x7ff) {
    // Too big for an immediate.  Load into t0 and use an add instruction.
    fprintf(fp, "\tlui t0, %d\n", stack_frame_size >> 12);
    fprintf(fp, "\taddi t0, t0, %d\n", stack_frame_size & 0xfff);
    fprintf(fp, "\tadd sp, sp, t0\n");
  } else {
    fprintf(fp, "\taddi sp, sp, %d\n", stack_frame_size);
  }
}

// Load a floating point or integer register from the stack frame.
static void LoadRegisterFromFrame(RVEmitter* emitter, int reg,
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

static void GenerateOffsetFromFrame(RVEmitter* emitter, int reg,
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

  // A leaf procedure doesn't save the return address.
  if (is_leaf) {
    frame_pointer_offset += 8;
    saved_reg_offset += 8;
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
    fprintf(fp, "\tsd %s, %d(%s)\n",
            RVRegisterNameFromNum(saved_reg->reg_num, kRVRegTypeInt, buf1,
                                  sizeof(buf1)),
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

  if (!is_leaf) {
    fprintf(fp, "\t// End of stack frame\n");
  }
}

static void RestoreRegisters(RVEmitter* emitter, FILE* fp) {
  // Exit sequence:
  // ld s0, S-8(sp)   - restore frame pointer.
  // addi sp, sp, S   - increment sp

  int stack_frame_size = StackFrameSize(emitter);
  char buf1[8];

  bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
          !emitter->rv->not_leaf;
  bool varargs = emitter->rv->base.varargs;
  int space_above_frame_pointer =
      varargs ? (RV_NUM_INT_ARGS - emitter->rv->num_int_arg_regs) * 8 : 0;

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
    fprintf(fp, "\tfld %s, %d(sp)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeFloat, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
    BitSetIteratorNext(&it);
  }

  BitSetIteratorStart(&it, &emitter->regs->used_int_regs);
  while (!BitSetIteratorDone(&it)) {
    int reg = (int)BitSetIteratorValue(&it);
    fprintf(fp, "\tld %s, %d(sp)\n",
            RVRegisterNameFromNum(reg, kRVRegTypeInt, buf1, sizeof(buf1)),
            offset);
    offset -= 8;
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

// The rmov instructions are an explicit mov from operand[1] to
// operand[0].  Both are registers.
static void PrintRmov(RVEmitter* emitter, TargetInstruction* inst, FILE* fp) {
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
  switch (inst->opcode) {
    case RV_OP(rmov):
      mnemonic = "mv";
      break;
    case RV_OP(rmovf):
      mnemonic = "fmv.s";
      break;
    case RV_OP(rmovd):
      mnemonic = "fmv.d";
      break;
    default:
      assert(false);
  }
  char buf1[8], buf2[8];
  fprintf(
      fp, "\t%-12s%s, %s\n", mnemonic,
      RVRegisterName((RVRegister*)inst->operand[0]->reg, buf1, sizeof(buf1)),
      RVRegisterName((RVRegister*)inst->operand[1]->reg, buf2, sizeof(buf2)));
}

static const char* GetRegisterName(TargetInstruction* inst, char* buf, size_t size) {
  if (inst->dest != NULL) {
    return RVRegisterName((RVRegister*)inst->dest->reg, buf, size);
  }
  return RVRegisterName((RVRegister*)inst->reg, buf, size);
}

// Main instruction printer.
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
  if (inst->opcode == RV_OP(label)) {
    if ((inst->flags & RV_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    return;
  }

  if (inst->opcode == RV_OP(named_label)) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }
  
  if (inst->opcode == RV_OP(ivarreg) || inst->opcode == RV_OP(fvarreg)) {
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
  switch ((RVOpcode)inst->opcode) {
    case RV_OP(rmov):
    case RV_OP(rmovf):
    case RV_OP(rmovd):
      PrintRmov(emitter, inst, fp);
      return;

    case RV_OP(mv):
      // Don't emit mv x, x.
      if (inst->operand[0]->reg == inst->reg) {
        return;
      }
      break;
    case RV_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n", sym->symbol->name.value);
      } else {
        fprintf(fp, "\t.global %s\n", sym->symbol->name.value);
      }
      return;
    }
    case RV_OP(call):
    case RV_OP(callf): {
      assert(inst->operand[0]->opcode == RV_OP(symbol));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      fprintf(fp, "\t%-12s%s\n", "call", sym->symbol->name.value);
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
      fprintf(fp, "\t%s\n", lit->value.value);
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
      const int spill_addr = RV_SPILL_ADDR;
      if (!RVIsPossibleImmediate(offset)) {
        fprintf(fp, "\t%-12s%s, %d\n",
                "lui",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                offset >> 12);
        fprintf(fp, "\t%-12s%s, s0, %s\n",
                "sub",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf2, sizeof(buf2)));
        fprintf(fp, "\t%-12s%s, -%d(%s)\t// Spilled @%d\n",
                 reg->type == kRVRegTypeInt ? "sd" : "fsd",
                 RVRegisterName(reg, buf1, sizeof(buf1)),
                 offset & 0xfff,
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
      const int spill_addr = RV_SPILL_ADDR;
      if (!RVIsPossibleImmediate(offset)) {
        fprintf(fp, "\t%-12s%s, %d\n",
                "lui",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                offset >> 12);
        fprintf(fp, "\t%-12s%s, s0, %s\n",
                "sub",
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf1, sizeof(buf1)),
                RVRegisterNameFromNum(spill_addr, kRVRegTypeInt, buf2, sizeof(buf2)));
        fprintf(fp, "\t%-12s%s, -%d(%s)\t// Reloaded spilled @%d\n",
                 reg->type == kRVRegTypeInt ? "ld" : "fld",
                 RVRegisterName(reg, buf1, sizeof(buf1)),
                 offset & 0xfff,
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

      } else if (inst->operand[1]->opcode == RV_OP(symbol)) {
        assert((inst->flags & RV_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)),
                ((TargetSymbol*)inst->operand[1])->symbol->name.value,
                GetRegisterName(inst->operand[0], buf2,
                               sizeof(buf2)));
      } else if (inst->operand[1]->opcode == RV_OP(label)) {
        assert((inst->flags & RV_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)),
                func_name, inst->operand[1]->id,
                GetRegisterName(inst->operand[0], buf2,
                               sizeof(buf2)));
      } else if (inst->operand[1]->opcode == RV_OP(x0)) {
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
      assert(inst->operand[0]->reg != NULL);
      assert(inst->operand[1]->reg != NULL);
      if (TargetIsConst(inst->operand[2])) {
        int offset = (int)TargetIntValue(inst->operand[2]);
        assert(RVIsPossibleImmediate(offset));
        fprintf(fp, "%s, %d(%s)\n",
                GetRegisterName(inst->operand[0], buf1,
                               sizeof(buf1)),
                offset,
                GetRegisterName(inst->operand[1], buf2,
                               sizeof(buf2)));
      } else if (inst->operand[2]->opcode == RV_OP(symbol)) {
        assert((inst->flags & RV_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst->operand[0], buf1,
                               sizeof(buf1)),
                ((TargetSymbol*)inst->operand[2])->symbol->name.value,
                GetRegisterName(inst->operand[1], buf2,
                               sizeof(buf2)));
      } else if (inst->operand[2]->opcode == RV_OP(label)) {
        assert((inst->flags & RV_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst->operand[0], buf1,
                               sizeof(buf1)),
                func_name, inst->operand[2]->id,
                GetRegisterName(inst->operand[1], buf2,
                               sizeof(buf2)));
      } else if (inst->operand[2]->opcode == RV_OP(x0)) {
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
      assert(inst->operand[0]->reg != NULL);
      assert(inst->operand[1]->reg != NULL);
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
      if (dest->opcode == RV_OP(label)) {
        fprintf(fp, ".%s_label_%d\n", func_name, inst->operand[0]->id);
      } else if (dest->opcode == RV_OP(symbol)) {
        fprintf(fp, "%s\n", ((TargetSymbol*)dest)->symbol->name.value);
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
          } else if (inst->operand[i]->opcode == RV_OP(symbol)) {
            if ((inst->flags & RV_HI_RELOC) != 0) {
              fprintf(fp, "%s%%hi(%s)", sep,
                      ((TargetSymbol*)inst->operand[i])->symbol->name.value);
            } else {
              fprintf(fp, "%s%s", sep,
                      ((TargetSymbol*)inst->operand[i])->symbol->name.value);
            }
          } else if (inst->operand[i]->opcode == RV_OP(literal)) {
            TargetLiteral* literal = (TargetLiteral*)inst->operand[i];
            fprintf(fp, "%s.str.%d", sep, literal->literal_id);
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


void RVPrintFunction(RVEmitter* emitter, FILE* fp) {
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
