//
//  xtensa_emitter.c
//  c_compiler_library
//
//  Created by David Allison on 3/2/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

// This is the RISC-V32 assembly language emitter. It prints the RISC-V32
// instructions to the given file in assembly language.  The XTENSAAssember
// reads this file and generates the binary.

#include "xtensa_emitter.h"
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "common_emitter.h"
#include "compiler.h"
#include "eh_metadata.h"
#include "target_basic_block.h"
#include "xtensa_assembler.h"
#include "xtensa_codegen.h"
#include "xtensa_reg_alloc.h"

// Is the given instruction printable?  Some instructions do not
// produce any output as they are used for information for other
// instructions.
static bool IsPrintable(TargetInstruction* inst) {
  // Constants are encoded in the instructions that use them.
  if (TargetIsConst(inst)) {
    return false;
  }
  // These opcodes are not printable.
  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(tmp):
    case XTENSA_OP(fp):
    case XTENSA_OP(sp):
    case XTENSA_OP(literal):
    case XTENSA_OP(structreturn):
    case XTENSA_OP(resulti):
    case XTENSA_OP(resultf):
    case XTENSA_OP(resultd):
    case XTENSA_OP(a0):
    case XTENSA_OP(a1):
    case XTENSA_OP(a2):
    case XTENSA_OP(a3):
    case XTENSA_OP(a4):
    case XTENSA_OP(a5):
    case XTENSA_OP(a6):
    case XTENSA_OP(a7):
    case XTENSA_OP(fa0):
    case XTENSA_OP(fa1):
    case XTENSA_OP(fa2):
    case XTENSA_OP(fa3):
    case XTENSA_OP(fa4):
    case XTENSA_OP(fa5):
    case XTENSA_OP(fa6):
    case XTENSA_OP(fa7):
    case XTENSA_OP(regarg):
    case XTENSA_OP(ivarreg):
    case XTENSA_OP(fvarreg):
    case XTENSA_OP(x0):
    case XTENSA_OP(t0):
    case XTENSA_OP(t1):
    case XTENSA_OP(t2):
    case XTENSA_OP(nrvoval):
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

static int VarargsSaveBytes(const XTENSAEmitter* emitter) {
  if (!emitter->rv->base.varargs) {
    return 0;
  }
  int named = emitter->rv->num_int_arg_regs;
  if (named >= XTENSA_NUM_INT_ARGS) {
    return 0;
  }
  int remaining = XTENSA_NUM_INT_ARGS - named;
  int pad = (named & 1) ? 4 : 0;
  return pad + remaining * 4;
}

// This is the size of the stack frame including the space
// for the local variables.
static bool HasExceptionStructReturn(XTENSAEmitter* emitter) {
  return emitter->rv->struct_return_reg >= 0 &&
         emitter->rv->exception_ranges.length > 0;
}

static int StackFrameSize(XTENSAEmitter* emitter) {
  // Start off with local variable space.  This also includes
  // 16 bytes for the saved ra and s0.
  int stack_frame_size = emitter->rv->base.stack_frame_size + 16;

  bool varargs = emitter->rv->base.varargs;

  if (varargs) {
    stack_frame_size += VarargsSaveBytes(emitter);
  }
  // A non-leaf procedure saves register variables on the stack as these
  // will be in saved registers.
  // if (!is_leaf) {
  //  stack_frame_size += emitter->rv->num_int_reg_vars*8 +
  //  emitter->rv->num_fp_reg_vars * 8;
  //}

  stack_frame_size += emitter->spill_region_size;
  if (HasExceptionStructReturn(emitter)) {
    stack_frame_size += 8;
  }

  stack_frame_size = (stack_frame_size + 15) & ~15;  // Aligned to 16 bytes.

  return stack_frame_size;
}

static bool EmptyStackFrame(XTENSAEmitter* emitter) {
  return emitter->rv->base.stack_frame_size == 0 &&
         emitter->rv->base.num_calls == 0 && !emitter->rv->not_leaf &&
         emitter->rv->saved_regs.length == 0 &&
         BitSetCount(&emitter->regs->used_int_regs) == 0 &&
         BitSetCount(&emitter->regs->used_float_regs) == 0 &&
         emitter->spill_region_size == 0;
}

static void DecrementStackPointer(XTENSAEmitter* emitter, int stack_frame_size,
                                  FILE* fp) {
  if (stack_frame_size > 0x7ff) {
    // Too big for an immediate.  Load into t0 and use a sub instruction.
    fprintf(fp, "\tli t0, %d\n", stack_frame_size);
    fprintf(fp, "\tsub sp, sp, t0\n");
  } else {
    fprintf(fp, "\taddi sp, sp, -%d\n", stack_frame_size);
  }
}

static COMPILER_UNUSED void IncrementStackPointer(XTENSAEmitter* emitter,
                                                  int stack_frame_size,
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
static COMPILER_UNUSED void LoadRegisterFromFrame(XTENSAEmitter* emitter,
                                                  int reg, int offset,
                                                  bool is_fp,
                                                  const char* symbol_name,
                                                  FILE* fp) {
  char buf[256];
  const char* instruction = is_fp ? "fld" : "lw";
  XTENSARegisterType reg_type = is_fp ? kXTENSARegTypeFloat : kXTENSARegTypeInt;
  if (offset < 0x7ff) {
    fprintf(fp, "\tf%s %s, -%d(s0)", instruction,
            XTENSARegisterNameFromNum(reg, reg_type, buf, sizeof(buf)), offset);
  } else {
    fprintf(fp, "\tli t0, %d\n", offset);
    fprintf(fp, "\taddi t0, t0, s0\n");
    fprintf(fp, "\t%s %s, 0(t0)", instruction,
            XTENSARegisterNameFromNum(reg, reg_type, buf, sizeof(buf)));
  }
  fprintf(fp, "\t\t// %s\n", symbol_name);
}

static COMPILER_UNUSED void GenerateOffsetFromFrame(XTENSAEmitter* emitter,
                                                    int reg, int offset,
                                                    const char* symbol_name,
                                                    FILE* fp) {
  char buf[256];
  if (offset < 0x7ff) {
    fprintf(fp, "\taddi %s, s0, -%d",
            XTENSARegisterNameFromNum(reg, kXTENSARegTypeInt, buf, sizeof(buf)),
            offset);
  } else {
    fprintf(fp, "\tli t0, %d\n", offset);
    fprintf(
        fp, "\tadd %s, s0, t0",
        XTENSARegisterNameFromNum(reg, kXTENSARegTypeInt, buf, sizeof(buf)));
  }
  fprintf(fp, "\t\t// %s\n", symbol_name);
}

// Save all used registers on the stack.
static void SaveRegisters(XTENSAEmitter* emitter, FILE* fp) {
  int stack_frame_size = StackFrameSize(emitter);
  int local_vars =
      emitter->rv->base.stack_frame_size + XTENSA_STACK_FRAME_HEADER_SIZE;

  // ENTRY rotates the CALL8 register window and allocates the frame.  Every
  // function needs it, including leaf functions, because arguments arrive in
  // the caller's a10-a15 and become this function's a2-a7 only after ENTRY.
  fprintf(fp, "\tentry a1, %d\n", stack_frame_size);
  if (!emitter->rv->base.varargs) {
    fprintf(fp, ".Leh_%s_after_push:\n", emitter->rv->base.function_name.value);
    fprintf(fp, ".Leh_%s_after_leaq:\n", emitter->rv->base.function_name.value);
  }

  if (emitter->rv->base.varargs) {
    int first = emitter->rv->num_int_arg_regs;
    int offset = (first & 1) ? 4 : 0;
    for (int i = first; i < XTENSA_NUM_INT_ARGS; i++) {
      int displacement = stack_frame_size + offset;
      if (displacement <= 1020 && (displacement & 3) == 0) {
        fprintf(fp, "\ts32i a%d, a1, %d\n", XTENSA_INT_ARG_START + i,
                displacement);
      } else {
        fprintf(fp, "\tli a15, %d\n", displacement);
        fprintf(fp, "\tadd a15, a1, a15\n");
        fprintf(fp, "\ts32i a%d, a15, 0\n", XTENSA_INT_ARG_START + i);
      }
      offset += 4;
    }
  }

  char reg_buf[16], base_buf[16];
  for (size_t i = 0; i < emitter->rv->saved_regs.length; i++) {
    SavedArgumentRegister* saved = emitter->rv->saved_regs.value.p[i];
    if (saved->is_fp) {
      fprintf(fp, "\t// floating argument home deferred\n");
      continue;
    }
    int displacement = saved->offset + stack_frame_size;
    const char* reg_name = XTENSARegisterNameFromNum(
        saved->reg_num, kXTENSARegTypeInt, reg_buf, sizeof(reg_buf));
    const char* base_name = XTENSARegisterNameFromNum(
        saved->base_reg_num, kXTENSARegTypeInt, base_buf, sizeof(base_buf));
    if (displacement >= 0 && displacement <= 1020 && (displacement & 3) == 0) {
      fprintf(fp, "\ts32i %s, %s, %d\n", reg_name, base_name, displacement);
    } else {
      fprintf(fp, "\tli a15, %d\n", displacement);
      fprintf(fp, "\tadd a15, %s, a15\n", base_name);
      fprintf(fp, "\ts32i %s, a15, 0\n", reg_name);
    }
  }

  emitter->first_spill_offset = local_vars + 4;
  emitter->saved_reg_offset = 0;
}

static void RestoreRegisters(XTENSAEmitter* emitter, FILE* fp) {
  (void)emitter;
  (void)fp;
  // RETW restores the caller's register window, including its stack pointer.
}

static void RestoreExceptionLandingState(XTENSAEmitter* emitter, FILE* fp) {
  fprintf(fp, "\tmv sp, s0\n");
  DecrementStackPointer(emitter, StackFrameSize(emitter), fp);

  int struct_return_phys = -1;
  if (emitter->rv->struct_return_reg >= 0) {
    bool is_leaf = emitter->rv->base.num_calls == 0 && OptLevel1() &&
                   !emitter->rv->not_leaf;
    struct_return_phys =
        (is_leaf ? XTENSA_FIRST_LEAF_INT_REG_VAR : XTENSA_FIRST_INT_REG_VAR) +
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
               8 * BitSetCount(&emitter->regs->used_float_regs);
  if (struct_return_phys >= 0 && HasExceptionStructReturn(emitter)) {
    char buf[8];
    fprintf(fp, "\tlw %s, %d(sp)\t// hidden result pointer\n",
            XTENSARegisterNameFromNum(struct_return_phys, kXTENSARegTypeInt,
                                      buf, sizeof(buf)),
            offset);
  }
}

// The rmov instructions are an explicit mov from operand[1] to
// operand[0].  Both are registers.
static const char* MoveMnemonic(XTENSAOpcode opcode, XTENSARegister* dest,
                                XTENSARegister* src) {
  if (dest->type == kXTENSARegTypeFloat || src->type == kXTENSARegTypeFloat) {
    if (opcode == XTENSA_OP(fmv_d)) {
      return "fmv.d";
    }
    return "fmv.s";
  }
  return "mov";
}

static bool IsZeroValue(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  if ((XTENSAOpcode)inst->opcode == XTENSA_OP(x0)) {
    return true;
  }
  if (inst->reg == NULL) {
    return false;
  }
  XTENSARegister* reg = (XTENSARegister*)inst->reg;
  return reg->type == kXTENSARegTypeInt && reg->base.num == XTENSA_INT_ZERO_REG;
}

static void PrintRmov(XTENSAEmitter* emitter, TargetInstruction* inst,
                      FILE* fp) {
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

  char buf1[8], buf2[8];
  const char* destination = XTENSARegisterName(
      (XTENSARegister*)inst->operand[0]->reg, buf1, sizeof(buf1));
  if (IsZeroValue(inst->operand[1])) {
    fprintf(fp, "\t%-12s%s, 0\n", "li", destination);
    return;
  }

  // Don't output mov rx,rx.
  if (inst->operand[0]->reg == inst->operand[1]->reg) {
    return;
  }

  const char* mnemonic = MoveMnemonic((XTENSAOpcode)inst->opcode,
                                      (XTENSARegister*)inst->operand[0]->reg,
                                      (XTENSARegister*)inst->operand[1]->reg);
  fprintf(fp, "\t%-12s%s, %s\n", mnemonic, destination,
          XTENSARegisterName((XTENSARegister*)inst->operand[1]->reg, buf2,
                             sizeof(buf2)));
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
  char buf1[8], buf2[8];
  if (IsZeroValue(src)) {
    fprintf(fp, "\t%-12s%s, 0\n", "li",
            XTENSARegisterName((XTENSARegister*)dest_reg, buf1, sizeof(buf1)));
    return;
  }
  if (dest_reg == src->reg) {
    return;
  }

  const char* mnemonic =
      MoveMnemonic((XTENSAOpcode)inst->opcode, (XTENSARegister*)dest_reg,
                   (XTENSARegister*)src->reg);
  fprintf(fp, "\t%-12s%s, %s\n", mnemonic,
          XTENSARegisterName((XTENSARegister*)dest_reg, buf1, sizeof(buf1)),
          XTENSARegisterName((XTENSARegister*)src->reg, buf2, sizeof(buf2)));
}

static const char* GetRegisterName(TargetInstruction* inst, char* buf,
                                   size_t size) {
  if (inst == NULL) {
    return "";
  }
  if (IsZeroValue(inst)) {
    return "a15";
  }
  if (inst->reg != NULL) {
    return XTENSARegisterName((XTENSARegister*)inst->reg, buf, size);
  }
  if (inst->dest != NULL && inst->dest->reg != NULL) {
    return XTENSARegisterName((XTENSARegister*)inst->dest->reg, buf, size);
  }
  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(t0):
      return XTENSARegisterNameFromNum(XTENSA_INT_TEMP_START_1,
                                       kXTENSARegTypeInt, buf, size);
    case XTENSA_OP(t1):
      return XTENSARegisterNameFromNum(XTENSA_INT_TEMP_START_1 + 1,
                                       kXTENSARegTypeInt, buf, size);
    case XTENSA_OP(t2):
      return XTENSARegisterNameFromNum(XTENSA_INT_TEMP_START_1 + 2,
                                       kXTENSARegTypeInt, buf, size);
    case XTENSA_OP(fp):
      return XTENSARegisterNameFromNum(XTENSA_FP_REG, kXTENSARegTypeInt, buf,
                                       size);
    case XTENSA_OP(sp):
      return XTENSARegisterNameFromNum(XTENSA_SP_REG, kXTENSARegTypeInt, buf,
                                       size);
    case XTENSA_OP(a0):
    case XTENSA_OP(a1):
    case XTENSA_OP(a2):
    case XTENSA_OP(a3):
    case XTENSA_OP(a4):
    case XTENSA_OP(a5):
    case XTENSA_OP(a6):
    case XTENSA_OP(a7):
      return XTENSARegisterNameFromNum(
          (int)inst->opcode - XTENSA_OP(a0) + XTENSA_INT_ARG_START,
          kXTENSARegTypeInt, buf, size);
    case XTENSA_OP(resulti):
    case XTENSA_OP(call):
    case XTENSA_OP(rcall):
      return XTENSARegisterNameFromNum(XTENSA_INT_RETURN_REG, kXTENSARegTypeInt,
                                       buf, size);
    case XTENSA_OP(resultf):
    case XTENSA_OP(resultd):
    case XTENSA_OP(callf):
    case XTENSA_OP(rcallf):
      return XTENSARegisterNameFromNum(XTENSA_FLOAT_RETURN_REG,
                                       kXTENSARegTypeFloat, buf, size);
    default:
      return "";
  }
}

static AsmOperand* GetAsmOperand(XTENSAAsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    return NULL;
  }
  if ((size_t)index < inst->asm_node->outputs.length) {
    return inst->asm_node->outputs.value.p[index];
  }
  return inst->asm_node->inputs.value.p[index - inst->asm_node->outputs.length];
}

static int AtomicSizeLog2(TargetInstruction* inst) {
  return (inst->flags & XTENSA_ATOMIC_SIZE_MASK) >> XTENSA_ATOMIC_SIZE_SHIFT;
}

static int AtomicOrder(TargetInstruction* inst) {
  return (inst->flags & XTENSA_ATOMIC_ORDER_MASK) >> XTENSA_ATOMIC_ORDER_SHIFT;
}

static int AtomicFailureOrder(TargetInstruction* inst) {
  return (inst->flags & XTENSA_ATOMIC_FAILURE_ORDER_MASK) >>
         XTENSA_ATOMIC_FAILURE_ORDER_SHIFT;
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

  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(atomic_load): {
      const char* addr = GetRegisterName(inst->operand[0], b1, sizeof(b1));
      static const char* loads[] = {"lbu", "lhu", "lw", "lw"};
      if (seq_cst) {
        fprintf(fp, "\tfence\n");
      }
      fprintf(fp, "\t%s %s, 0(%s)\n", loads[size], result, addr);
      if (acquire) {
        fprintf(fp, "\tfence\n");
      }
      return;
    }
    case XTENSA_OP(atomic_store): {
      const char* value = GetRegisterName(inst->operand[0], b0, sizeof(b0));
      const char* addr = GetRegisterName(inst->operand[1], b1, sizeof(b1));
      static const char* stores[] = {"sb", "sh", "sw", "sw"};
      if (release || seq_cst) {
        fprintf(fp, "\tfence\n");
      }
      fprintf(fp, "\t%s %s, 0(%s)\n", stores[size], value, addr);
      if (seq_cst) {
        fprintf(fp, "\tfence\n");
      }
      return;
    }
    case XTENSA_OP(atomic_fence):
      if (order != 0) {
        fprintf(fp, "\tfence\n");
      }
      return;
    case XTENSA_OP(atomic_fetch_add):
    case XTENSA_OP(atomic_fetch_sub):
    case XTENSA_OP(atomic_add_fetch):
    case XTENSA_OP(atomic_sub_fetch): {
      bool add = inst->opcode == (TargetOpcode)XTENSA_OP(atomic_fetch_add) ||
                 inst->opcode == (TargetOpcode)XTENSA_OP(atomic_add_fetch);
      bool return_new =
          inst->opcode == (TargetOpcode)XTENSA_OP(atomic_add_fetch) ||
          inst->opcode == (TargetOpcode)XTENSA_OP(atomic_sub_fetch);
      const char* addr = GetRegisterName(inst->operand[0], b1, sizeof(b1));
      const char* value = GetRegisterName(inst->operand[1], b2, sizeof(b2));
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
        fprintf(fp, "\tlr.w%s %s, (t0)\n", AtomicSuffix(acquire, release),
                result);
        fprintf(fp, "\tnot t4, t2\n");
        fprintf(fp, "\tand t4, %s, t4\n", result);
        fprintf(fp, "\tand t5, %s, t2\n", result);
        fprintf(fp, "\t%s t5, t5, t3\n", add ? "add" : "sub");
        fprintf(fp, "\tand t5, t5, t2\n");
        fprintf(fp, "\tor t5, t5, t4\n");
        fprintf(fp, "\tsc.w%s t6, t5, (t0)\n",
                AtomicSuffix(order == 5, release));
        fprintf(fp, "\tbnez t6, .L%s_atomic_retry_%d\n", func_name, inst->id);
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
      fprintf(fp, "\tamoadd.w%s %s, t0, (%s)\n", AtomicSuffix(acquire, release),
              result, addr);
      if (return_new) {
        fprintf(fp, "\tadd %s, %s, t0\n", result, result);
      }
      return;
    }
    case XTENSA_OP(atomic_compare_exchange_bool):
    case XTENSA_OP(atomic_compare_exchange_val):
    case XTENSA_OP(atomic_compare_exchange_n): {
      bool expected_is_pointer =
          inst->opcode == (TargetOpcode)XTENSA_OP(atomic_compare_exchange_n);
      bool returns_bool =
          inst->opcode != (TargetOpcode)XTENSA_OP(atomic_compare_exchange_val);
      bool lr_acquire =
          acquire || AtomicOrderHasAcquire(failure_order) || order == 5;
      const char* addr = GetRegisterName(inst->operand[0], b1, sizeof(b1));
      const char* expected = GetRegisterName(inst->operand[1], b2, sizeof(b2));
      const char* desired = GetRegisterName(inst->operand[2], b3, sizeof(b3));
      if (size < 2) {
        int value_mask = size == 0 ? 0xff : 0xffff;
        if (expected_is_pointer) {
          fprintf(fp, "\t%s t3, 0(%s)\n", size == 0 ? "lbu" : "lhu", expected);
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
        fprintf(fp, "\tlr.w%s %s, (t0)\n", AtomicSuffix(lr_acquire, order == 5),
                result);
        fprintf(fp, "\tand t6, %s, t2\n", result);
        fprintf(fp, "\tbne t6, t3, .L%s_atomic_mismatch_%d\n", func_name,
                inst->id);
        fprintf(fp, "\tnot t6, t2\n");
        fprintf(fp, "\tand t5, %s, t6\n", result);
        fprintf(fp, "\tor t5, t5, t4\n");
        fprintf(fp, "\tsc.w%s t6, t5, (t0)\n",
                AtomicSuffix(order == 5, release));
        if ((inst->flags & XTENSA_ATOMIC_WEAK) != 0) {
          fprintf(fp, "\tbnez t6, .L%s_atomic_mismatch_%d\n", func_name,
                  inst->id);
        } else {
          fprintf(fp, "\tbnez t6, .L%s_atomic_retry_%d\n", func_name, inst->id);
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
          fprintf(fp, "\t%s %s, 0(%s)\n", size == 0 ? "sb" : "sh", result,
                  expected);
        }
        if (returns_bool) {
          fprintf(fp, "\tli %s, 0\n", result);
        }
        fprintf(fp, ".L%s_atomic_done_%d:\n", func_name, inst->id);
        return;
      }
      if (expected_is_pointer) {
        fprintf(fp, "\tlw t0, 0(%s)\n", expected);
      } else {
        fprintf(fp, "\tmv t0, %s\n", expected);
      }
      fprintf(fp, "\tmv t1, %s\n", desired);
      fprintf(fp, "\tmv t3, %s\n", addr);
      fprintf(fp, ".L%s_atomic_retry_%d:\n", func_name, inst->id);
      fprintf(fp, "\tlr.w%s %s, (%s)\n", AtomicSuffix(lr_acquire, order == 5),
              result, "t3");
      fprintf(fp, "\tbne %s, t0, .L%s_atomic_mismatch_%d\n", result, func_name,
              inst->id);
      fprintf(fp, "\tsc.w%s t2, t1, (%s)\n", AtomicSuffix(order == 5, release),
              "t3");
      if ((inst->flags & XTENSA_ATOMIC_WEAK) != 0) {
        fprintf(fp, "\tbnez t2, .L%s_atomic_mismatch_%d\n", func_name,
                inst->id);
      } else {
        fprintf(fp, "\tbnez t2, .L%s_atomic_retry_%d\n", func_name, inst->id);
      }
      if (returns_bool) {
        fprintf(fp, "\tli %s, 1\n", result);
      }
      fprintf(fp, "\tj .L%s_atomic_done_%d\n", func_name, inst->id);
      fprintf(fp, ".L%s_atomic_mismatch_%d:\n", func_name, inst->id);
      if (expected_is_pointer) {
        fprintf(fp, "\tsw %s, 0(%s)\n", result, expected);
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

static int FindAsmOperandByName(XTENSAAsmInstruction* inst, const char* name,
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

static int FindAsmLabelByName(XTENSAAsmInstruction* inst, const char* name,
                              size_t len) {
  for (size_t i = 0; i < inst->asm_node->labels.length; i++) {
    String* label = inst->asm_node->labels.value.p[i];
    if (label->length == len && strncmp(label->value, name, len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void PrintAsmOperand(FILE* fp, XTENSAAsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    fprintf(fp, "<bad-asm-operand>");
    return;
  }
  if (inst->reg_nums[index] == INT_MIN) {
    fprintf(fp, "%" PRId64, inst->immediate_values[index]);
    return;
  }
  char buf[32];
  fprintf(fp, "%s",
          XTENSARegisterNameFromNum(
              inst->reg_nums[index],
              inst->is_fp[index] ? kXTENSARegTypeFloat : kXTENSARegTypeInt, buf,
              sizeof(buf)));
}

static void PrintAsmLabel(FILE* fp, XTENSAAsmInstruction* inst, int index) {
  if (index < 0 || (size_t)index >= inst->asm_node->labels.length) {
    fprintf(fp, "<bad-asm-label>");
    return;
  }
  String* label = inst->asm_node->labels.value.p[index];
  fprintf(fp, "%s", label->value);
}

static void PrintExtendedAsm(FILE* fp, XTENSAAsmInstruction* inst,
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
static void PrintInstruction(XTENSAEmitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (inst->block == NULL) {
    return;
  }
  if (inst->block != emitter->current_block) {
    TargetBasicBlock* b = inst->block;
    fprintf(fp, "\n\t// *** Basic block %zd\n\n", b->block_id);
    emitter->current_block = inst->block;
  }
  if (((int)inst->opcode == (int)XTENSA_OP(label))) {
    if ((inst->flags & XTENSA_EXPORTED_LABEL) != 0) {
      // Label may be exported.
      fprintf(fp, "\t.local .%s_label_%d\n", func_name, inst->id);
    }
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    if ((inst->flags & TARGET_INST_EXCEPTION_LANDING) != 0) {
      RestoreExceptionLandingState(emitter, fp);
    }
    return;
  }

  if (((int)inst->opcode == (int)XTENSA_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }

  if (((int)inst->opcode == (int)XTENSA_OP(ivarreg)) ||
      ((int)inst->opcode == (int)XTENSA_OP(fvarreg))) {
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

  // Buffers for register name printing.
  char buf1[8];
  char buf2[8];
  char buf3[256];

  // Special case instructions.
  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(lw):
    case XTENSA_OP(lwu):
    case XTENSA_OP(lh):
    case XTENSA_OP(lhu):
    case XTENSA_OP(lb):
    case XTENSA_OP(lbu): {
      XTENSAOpcode opcode = (XTENSAOpcode)inst->opcode;
      const char* mnemonic =
          (opcode == XTENSA_OP(lw) || opcode == XTENSA_OP(lwu))
              ? "l32i"
              : (opcode == XTENSA_OP(lh)
                     ? "l16si"
                     : (opcode == XTENSA_OP(lhu) ? "l16ui" : "l8ui"));
      int scale =
          (opcode == XTENSA_OP(lw) || opcode == XTENSA_OP(lwu))
              ? 4
              : ((opcode == XTENSA_OP(lh) || opcode == XTENSA_OP(lhu)) ? 2 : 1);
      int maximum = 255 * scale;
      const char* destination = GetRegisterName(inst, buf1, sizeof(buf1));
      const char* base = GetRegisterName(inst->operand[0], buf2, sizeof(buf2));
      bool zero_offset = IsZeroValue(inst->operand[1]);
      if (!TargetIsConst(inst->operand[1]) && !zero_offset) {
        if ((XTENSAOpcode)inst->operand[0]->opcode == XTENSA_OP(fp)) {
          fprintf(fp, "\tli a15, %d\n",
                  StackFrameSize(emitter) + emitter->stack_pointer_adjustment);
          fprintf(fp, "\tadd a15, %s, a15\n", base);
          base = "a15";
        }
        fprintf(fp, "\tadd a15, %s, %s\n", base,
                GetRegisterName(inst->operand[1], buf3, sizeof(buf3)));
        fprintf(fp, "\t%s %s, a15, 0\n", mnemonic, destination);
        if (opcode == XTENSA_OP(lb)) {
          fprintf(fp, "\tslli %s, %s, 24\n", destination, destination);
          fprintf(fp, "\tsrai %s, %s, 24\n", destination, destination);
        }
        return;
      }
      int offset = zero_offset ? 0 : (int)TargetIntValue(inst->operand[1]);
      if ((XTENSAOpcode)inst->operand[0]->opcode == XTENSA_OP(fp)) {
        offset += StackFrameSize(emitter) + emitter->stack_pointer_adjustment;
      }
      if (offset >= 0 && offset <= maximum && offset % scale == 0) {
        fprintf(fp, "\t%s %s, %s, %d\n", mnemonic, destination, base, offset);
      } else {
        fprintf(fp, "\tli a15, %d\n", offset);
        fprintf(fp, "\tadd a15, %s, a15\n", base);
        fprintf(fp, "\t%s %s, a15, 0\n", mnemonic, destination);
      }
      if (opcode == XTENSA_OP(lb)) {
        fprintf(fp, "\tslli %s, %s, 24\n", destination, destination);
        fprintf(fp, "\tsrai %s, %s, 24\n", destination, destination);
      }
      return;
    }

    case XTENSA_OP(sw):
    case XTENSA_OP(sh):
    case XTENSA_OP(sb): {
      XTENSAOpcode opcode = (XTENSAOpcode)inst->opcode;
      const char* mnemonic = opcode == XTENSA_OP(sw)
                                 ? "s32i"
                                 : (opcode == XTENSA_OP(sh) ? "s16i" : "s8i");
      int scale =
          opcode == XTENSA_OP(sw) ? 4 : (opcode == XTENSA_OP(sh) ? 2 : 1);
      int maximum = 255 * scale;
      const char* value = GetRegisterName(inst->operand[0], buf1, sizeof(buf1));
      const char* base = GetRegisterName(inst->operand[1], buf2, sizeof(buf2));
      if (IsZeroValue(inst->operand[0])) {
        fprintf(fp, "\tli a14, 0\n");
        value = "a14";
      }
      bool zero_offset = IsZeroValue(inst->operand[2]);
      if (!TargetIsConst(inst->operand[2]) && !zero_offset) {
        if ((XTENSAOpcode)inst->operand[1]->opcode == XTENSA_OP(fp)) {
          fprintf(fp, "\tli a15, %d\n",
                  StackFrameSize(emitter) + emitter->stack_pointer_adjustment);
          fprintf(fp, "\tadd a15, %s, a15\n", base);
          base = "a15";
        }
        fprintf(fp, "\tadd a15, %s, %s\n", base,
                GetRegisterName(inst->operand[2], buf3, sizeof(buf3)));
        fprintf(fp, "\t%s %s, a15, 0\n", mnemonic, value);
        return;
      }
      int offset = zero_offset ? 0 : (int)TargetIntValue(inst->operand[2]);
      if ((XTENSAOpcode)inst->operand[1]->opcode == XTENSA_OP(fp)) {
        offset += StackFrameSize(emitter) + emitter->stack_pointer_adjustment;
      }
      if (offset >= 0 && offset <= maximum && offset % scale == 0) {
        fprintf(fp, "\t%s %s, %s, %d\n", mnemonic, value, base, offset);
      } else {
        fprintf(fp, "\tli a15, %d\n", offset);
        fprintf(fp, "\tadd a15, %s, a15\n", base);
        fprintf(fp, "\t%s %s, a15, 0\n", mnemonic, value);
      }
      return;
    }

    case XTENSA_OP(atomic_load):
    case XTENSA_OP(atomic_store):
    case XTENSA_OP(atomic_fetch_add):
    case XTENSA_OP(atomic_fetch_sub):
    case XTENSA_OP(atomic_add_fetch):
    case XTENSA_OP(atomic_sub_fetch):
    case XTENSA_OP(atomic_compare_exchange_bool):
    case XTENSA_OP(atomic_compare_exchange_val):
    case XTENSA_OP(atomic_compare_exchange_n):
    case XTENSA_OP(atomic_fence):
      PrintAtomicInstruction(inst, func_name, fp);
      return;
      //    case XTENSA_OP(rmov):
      //    case XTENSA_OP(rmovf):
      //    case XTENSA_OP(rmovd):
      //      PrintRmov(emitter, inst, fp);
      //      return;

    case XTENSA_OP(mv):
    case XTENSA_OP(fmv_s):
    case XTENSA_OP(fmv_d):
      if (inst->operand[1] != NULL) {
        PrintRmov(emitter, inst, fp);
      } else {
        PrintDestMove(inst, fp);
      }
      return;
    case XTENSA_OP(symbol): {
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
    case XTENSA_OP(call):
    case XTENSA_OP(callf): {
      assert(((int)inst->operand[0]->opcode == (int)XTENSA_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      for (int reg = XTENSA_INT_ARG_START; reg <= XTENSA_INT_ARG_END; reg++) {
        fprintf(fp, "\tmov a%d, a%d\n", reg + 8, reg);
      }
      fprintf(fp, "\t%-12s%s\n", "call8", SymbolName(sym, buf3, sizeof(buf3)));
      if ((XTENSAOpcode)inst->opcode == XTENSA_OP(call)) {
        fprintf(fp, "\tmov a2, a10\n");
        fprintf(fp, "\tmov a3, a11\n");
      }
      return;
    }
    case XTENSA_OP(tprel):
    case XTENSA_OP(tlsgd): {
      assert(inst->operand[0] != NULL &&
             inst->operand[0]->opcode == (TargetOpcode)XTENSA_OP(symbol));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      fprintf(
          fp, "\t%-12s%s, %s\n",
          (XTENSAOpcode)inst->opcode == XTENSA_OP(tlsgd) ? "tlsgd" : "tprel",
          GetRegisterName(inst, buf1, sizeof(buf1)),
          SymbolName(sym, buf3, sizeof(buf3)));
      return;
    }

    case XTENSA_OP(rcall):
    case XTENSA_OP(rcallf):
      fprintf(fp, "\tmov a8, %s\n",
              GetRegisterName(inst->operand[0], buf2, sizeof(buf2)));
      for (int reg = XTENSA_INT_ARG_START; reg <= XTENSA_INT_ARG_END; reg++) {
        fprintf(fp, "\tmov a%d, a%d\n", reg + 8, reg);
      }
      fprintf(fp, "\t%-12s%s\n", "callx8", "a8");
      if ((XTENSAOpcode)inst->opcode == XTENSA_OP(rcall)) {
        fprintf(fp, "\tmov a2, a10\n");
        fprintf(fp, "\tmov a3, a11\n");
      }

      return;
    case XTENSA_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case XTENSA_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case XTENSA_OP(ret):
      fprintf(fp, "\tretw\n");
      return;

    case XTENSA_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t");
      if ((inst->flags & XTENSA_INST_EXTENDED_ASM) != 0) {
        PrintExtendedAsm(fp, (XTENSAAsmInstruction*)inst, lit->value.value);
      } else {
        fprintf(fp, "%s", lit->value.value);
      }
      fprintf(fp, "\n");
      lit->base.disabled = true;
      return;
    }

    case XTENSA_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }

    case XTENSA_OP(spill): {
      XTENSARegister* reg = (XTENSARegister*)inst->reg;
      int offset =
          (int)TargetIntValue(inst->operand[1]) + emitter->first_spill_offset;
      int displacement =
          StackFrameSize(emitter) - offset + emitter->stack_pointer_adjustment;
      if (reg->type != kXTENSARegTypeInt) {
        fprintf(fp, "\t// floating spill deferred\n");
        return;
      }
      fprintf(fp, "\ts32i %s, a1, %d\t// Spilled @%d\n",
              XTENSARegisterName(reg, buf1, sizeof(buf1)), displacement,
              inst->operand[0]->id);
      return;
    }

    case XTENSA_OP(reload): {
      XTENSARegister* reg = (XTENSARegister*)inst->reg;
      TargetInstruction* spill = inst->operand[0];
      int offset =
          (int)TargetIntValue(spill->operand[1]) + emitter->first_spill_offset;
      int displacement =
          StackFrameSize(emitter) - offset + emitter->stack_pointer_adjustment;
      if (reg->type != kXTENSARegTypeInt) {
        fprintf(fp, "\t// floating reload deferred\n");
        return;
      }
      fprintf(fp, "\tl32i %s, a1, %d\t// Reloaded spilled @%d\n",
              XTENSARegisterName(reg, buf1, sizeof(buf1)), displacement,
              spill->operand[0]->id);
      return;
    }
    default:
      break;
  }

  // General case for instruction printing.

  for (int operand = 0; operand < 4 && inst->operand[operand] != NULL;
       ++operand) {
    if (IsZeroValue(inst->operand[operand])) {
      fprintf(fp, "\tli a15, 0\n");
      break;
    }
  }

  // Print opcode.
  const char* opcode_name = (XTENSAOpcode)inst->opcode == XTENSA_OP(jalr) &&
                                    (inst->flags & TARGET_INST_TABLE_JUMP) != 0
                                ? "jx"
                                : XTENSAOpcodeName(inst->opcode);
  fprintf(fp, "\t%-12s", opcode_name);

  // Print operands.
  switch ((XTENSAOpcode)inst->opcode) {
    case XTENSA_OP(lw):
    case XTENSA_OP(lh):
    case XTENSA_OP(lb):
    case XTENSA_OP(lwu):
    case XTENSA_OP(lbu):
    case XTENSA_OP(lhu):
    case XTENSA_OP(flw):
    case XTENSA_OP(ld):
    case XTENSA_OP(fld):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      assert(inst->operand[0]->reg != NULL);
      if (TargetIsConst(inst->operand[1])) {
        int offset = (int)TargetIntValue(inst->operand[1]);
        assert(XTENSAIsPossibleImmediate(offset));
        fprintf(fp, "%s, %d(%s)\n", GetRegisterName(inst, buf1, sizeof(buf1)),
                offset, GetRegisterName(inst->operand[0], buf2, sizeof(buf2)));

      } else if (((int)inst->operand[1]->opcode == (int)XTENSA_OP(symbol))) {
        assert((inst->flags & XTENSA_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)),
                SymbolName((TargetSymbol*)inst->operand[1], buf3, sizeof(buf3)),
                GetRegisterName(inst->operand[0], buf2, sizeof(buf2)));
      } else if (((int)inst->operand[1]->opcode == (int)XTENSA_OP(label))) {
        assert((inst->flags & XTENSA_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst, buf1, sizeof(buf1)), func_name,
                inst->operand[1]->id,
                GetRegisterName(inst->operand[0], buf2, sizeof(buf2)));
      } else if (((int)inst->operand[1]->opcode == (int)XTENSA_OP(x0))) {
        fprintf(fp, "%s, 0(%s)\n", GetRegisterName(inst, buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[0], buf2, sizeof(buf2)));
      } else {
        assert(false);
      }
      break;

    case XTENSA_OP(sw):
    case XTENSA_OP(sh):
    case XTENSA_OP(sd):
    case XTENSA_OP(sb):
    case XTENSA_OP(fsw):
    case XTENSA_OP(fsd):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      if (TargetIsConst(inst->operand[2])) {
        int offset = (int)TargetIntValue(inst->operand[2]);
        assert(XTENSAIsPossibleImmediate(offset));
        fprintf(fp, "%s, %d(%s)\n",
                GetRegisterName(inst->operand[0], buf1, sizeof(buf1)), offset,
                GetRegisterName(inst->operand[1], buf2, sizeof(buf2)));
      } else if (((int)inst->operand[2]->opcode == (int)XTENSA_OP(symbol))) {
        assert((inst->flags & XTENSA_LO_RELOC) != 0);
        fprintf(fp, "%s, %%lo(%s)(%s)\n",
                GetRegisterName(inst->operand[0], buf1, sizeof(buf1)),
                SymbolName((TargetSymbol*)inst->operand[2], buf3, sizeof(buf3)),
                GetRegisterName(inst->operand[1], buf2, sizeof(buf2)));
      } else if (((int)inst->operand[2]->opcode == (int)XTENSA_OP(label))) {
        assert((inst->flags & XTENSA_PCREL_LO_RELOC) != 0);
        fprintf(fp, "%s, %%pcrel_lo(.%s_label_%d)(%s)\n",
                GetRegisterName(inst->operand[0], buf1, sizeof(buf1)),
                func_name, inst->operand[2]->id,
                GetRegisterName(inst->operand[1], buf2, sizeof(buf2)));
      } else if (((int)inst->operand[2]->opcode == (int)XTENSA_OP(x0))) {
        fprintf(fp, "%s, 0(%s)\n",
                GetRegisterName(inst->operand[0], buf1, sizeof(buf1)),
                GetRegisterName(inst->operand[1], buf2, sizeof(buf2)));

      } else {
        assert(false);
      }
      break;

    case XTENSA_OP(nop):
      fprintf(fp, "\n");
      break;

    case XTENSA_OP(beq):
    case XTENSA_OP(bne):
    case XTENSA_OP(blt):
    case XTENSA_OP(bge):
    case XTENSA_OP(bltu):
    case XTENSA_OP(bgeu):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      fprintf(fp, "%s, %s, .%s_label_%d\n",
              GetRegisterName(inst->operand[0], buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[1], buf2, sizeof(buf2)), func_name,
              inst->operand[2]->id);
      break;

    // Branch zero, only one register and a target.
    case XTENSA_OP(beqz):
    case XTENSA_OP(bnez):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[0]->reg != NULL);
      fprintf(fp, "%s, .%s_label_%d\n",
              GetRegisterName(inst->operand[0], buf1, sizeof(buf1)), func_name,
              inst->operand[1]->id);
      break;

    case XTENSA_OP(j): {
      assert(inst->operand[0] != NULL);
      TargetInstruction* dest = inst->operand[0];
      if (((int)dest->opcode == (int)XTENSA_OP(label))) {
        fprintf(fp, ".%s_label_%d\n", func_name, inst->operand[0]->id);
      } else if (((int)dest->opcode == (int)XTENSA_OP(symbol))) {
        fprintf(fp, "%s\n",
                SymbolName((TargetSymbol*)dest, buf3, sizeof(buf3)));
      } else if (((int)dest->opcode == (int)XTENSA_OP(named_label))) {
        fprintf(fp, "%s\n", ((TargetNamedLabel*)dest)->name);
      } else {
        assert(false);
      }
      break;
    }

    case XTENSA_OP(jr):
      fprintf(fp, "%s\n",
              GetRegisterName(inst->operand[0], buf1, sizeof(buf1)));
      break;

    case XTENSA_OP(jalr):
      if ((inst->flags & TARGET_INST_TABLE_JUMP) != 0) {
        assert(inst->operand[0] != NULL);
        fprintf(fp, "%s\n",
                GetRegisterName(inst->operand[0], buf1, sizeof(buf1)));
        break;
      }
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      fprintf(fp, "%s, %s, %d\n",
              GetRegisterName(inst->operand[0], buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[1], buf2, sizeof(buf2)),
              (int)TargetIntValue(inst->operand[2]));
      break;

    case XTENSA_OP(li): {
      int64_t value = TargetIntValue(inst->operand[0]);

      fprintf(fp, "%s, %" PRId64 "\t\t// 0x%" PRIx64 "",
              GetRegisterName(inst, buf1, sizeof(buf1)), value, value);
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

    case XTENSA_OP(addi): {
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      int64_t value = XTENSAIntValue(inst->operand[1]);
      if ((XTENSAOpcode)inst->operand[0]->opcode == XTENSA_OP(fp)) {
        value += StackFrameSize(emitter) + emitter->stack_pointer_adjustment;
      }
      fprintf(fp, "%s, %s, %" PRId64 "\n",
              GetRegisterName(inst, buf1, sizeof(buf1)),
              GetRegisterName(inst->operand[0], buf2, sizeof(buf2)), value);
      if ((XTENSAOpcode)inst->operand[0]->opcode == XTENSA_OP(sp) &&
          inst->reg != NULL && inst->reg->num == XTENSA_SP_REG) {
        emitter->stack_pointer_adjustment -= (int)value;
      }
      break;
    }

    default: {
      const char* sep = "";
      if (inst->reg != NULL) {
        fprintf(fp, "%s", GetRegisterName(inst, buf1, sizeof(buf1)));
        sep = ", ";
      }
      for (int i = 0; i < 2; i++) {
        if (inst->operand[i] != NULL) {
          if (TargetIsConst(inst->operand[i])) {
            fprintf(fp, "%s%" PRId64 "", sep, TargetIntValue(inst->operand[i]));
          } else if (((int)inst->operand[i]->opcode ==
                      (int)XTENSA_OP(symbol))) {
            if ((inst->flags & XTENSA_HI_RELOC) != 0) {
              fprintf(fp, "%s%%hi(%s)", sep,
                      SymbolName((TargetSymbol*)inst->operand[i], buf3,
                                 sizeof(buf3)));
            } else {
              fprintf(fp, "%s%s", sep,
                      SymbolName((TargetSymbol*)inst->operand[i], buf3,
                                 sizeof(buf3)));
            }
          } else if (((int)inst->operand[i]->opcode ==
                      (int)XTENSA_OP(literal))) {
            TargetLiteral* literal = (TargetLiteral*)inst->operand[i];
            fprintf(fp, "%s.str.%d", sep, literal->literal_id);
          } else if (((int)inst->operand[i]->opcode == (int)XTENSA_OP(label))) {
            fprintf(fp, "%s.%s_label_%d", sep, func_name, inst->operand[i]->id);
          } else {
            fprintf(fp, "%s%s", sep,
                    GetRegisterName(inst->operand[i], buf2, sizeof(buf2)));
          }
          sep = ", ";
        }
      }
      fprintf(fp, "\n");
    }
  }
}

void XTENSAEmitterInit(XTENSAEmitter* emitter, XTENSAGenerator* rv) {
  emitter->rv = rv;
  emitter->regs = &rv->register_allocator;
  emitter->saved_reg_offset = 0;
  emitter->spill_region_size = rv->register_allocator.max_spilled_region_size;
  emitter->first_spill_offset = 0;
  emitter->stack_pointer_adjustment = 0;
  emitter->current_block = NULL;
}

XTENSAEmitter* NewXTENSAEmitter(XTENSAGenerator* rv) {
  XTENSAEmitter* emitter = malloc(sizeof(XTENSAEmitter));
  XTENSAEmitterInit(emitter, rv);
  return emitter;
}

void XTENSAEmitterDestruct(XTENSAEmitter* emitter) {}

void XTENSAEmitterDelete(XTENSAEmitter* emitter) {
  XTENSAEmitterDestruct(emitter);
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

static void XTENSAFillLSDAInfo(XTENSAEmitter* emitter,
                               DaveEHLSDARange* lsda_ranges, size_t* count) {
  *count = 0;
  for (size_t i = 0; i < emitter->rv->exception_ranges.length && *count < 64;
       i++) {
    XTENSAExceptionRange* range = emitter->rv->exception_ranges.value.p[i];
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
// which the prologue leaves stack_frame_size below the CFA (= s0).  RISC-V32
// numbers its DWARF integer registers exactly as the architecture does, which
// is also what the register allocator uses, so no translation is needed.
static size_t XTENSASavedCFIRegisters(XTENSAEmitter* emitter,
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

static void XTENSAPrintEHMetadata(XTENSAEmitter* emitter, FILE* fp,
                                  const char* func_name) {
  if (emitter->rv->base.varargs) {
    return;
  }
  DaveEHLSDARange lsda_ranges[64];
  DaveEHFrameSavedReg saved_regs[XTENSA_NUM_INT_REGS];
  size_t lsda_count = 0;
  size_t saved_reg_count = XTENSASavedCFIRegisters(
      emitter, saved_regs, sizeof(saved_regs) / sizeof(saved_regs[0]));
  XTENSAFillLSDAInfo(emitter, lsda_ranges, &lsda_count);
  DaveEHFrameEmitInfo info = {
      .ranges = lsda_ranges,
      .range_count = lsda_count,
      .func_name = func_name,
      .has_frame = !EmptyStackFrame(emitter),
      .is_64bit = false,
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

void XTENSAPrintFunction(XTENSAEmitter* emitter, FILE* fp) {
  const char* func_name = emitter->rv->base.function_name.value;
  EmitFunctionSection(fp, func_name);
  fprintf(fp, "\t.align 2\n");
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
  XTENSAPrintEHMetadata(emitter, fp, func_name);
}

void XTENSAPrintCXXAdjustorThunks(FILE* fp) {
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
