//
//  6502_emitter.c
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_emitter.h"
#include <assert.h>
#include <stdlib.h>
#include "compiler.h"
#include "6502_assembler.h"
#include "6502_reg_alloc.h"

// Is the given instruction printable?  Some instructions do not
// produce any output as they are used for information for other
// instructions.
static bool IsPrintable(TargetInstruction* inst) {
  // Constants are encoded in the instructions that use them.
  if (TargetIsConst(inst)) {
    return false;
  }
  // These opcodes are not printable.
  switch ((_6502Opcode)inst->opcode) {
    case _6502_OP(tmp):
    case _6502_OP(fp):
    case _6502_OP(sp):
    case _6502_OP(ap):
    case _6502_OP(tp):
    case _6502_OP(literal):
    case _6502_OP(structreturn):
    case _6502_OP(resultx):
    case _6502_OP(resultf):
    case _6502_OP(resultd):
    case _6502_OP(ret):
    case _6502_OP(localvar):
    case _6502_OP(argument):
      return false;
    default:
      break;
  }
  return true;
}


// Generate a .hword with a bitmask for all the registers to save.
static void GenerateRegisterMask(_6502Emitter* emitter, FILE* fp) {
  int16_t mask = 0;
  Vector regs = {0};
  BitSetExpand(&emitter->regs->used_b_regs, &regs);
  if (regs.length > 0) {
    // Save all B regs.
    mask |= _6502_RMASK_B;
  }
  
  VectorClear(&regs);
  
  BitSetExpand(&emitter->regs->used_a_regs, &regs);
  for (size_t i = 1; i < regs.length; i++) {
    int reg = (int)regs.value.p[i];
    mask |= 1 << (_6502_RMASK_A + reg);
  }
  VectorClear(&regs);
  
  BitSetExpand(&emitter->regs->used_i_regs, &regs);
  for (size_t i = 1; i < regs.length; i++) {
    int reg = (int)regs.value.p[i];
    mask |= 1 << (_6502_RMASK_I + reg);
  }
  VectorClear(&regs);
  
  BitSetExpand(&emitter->regs->used_x_regs, &regs);
  for (size_t i = 1; i < regs.length; i++) {
    int reg = (int)regs.value.p[i];
    mask |= 1 << (_6502_RMASK_X + reg);
  }
  VectorClear(&regs);

  BitSetExpand(&emitter->regs->used_f_regs, &regs);
  for (size_t i = 1; i < regs.length; i++) {
    int reg = (int)regs.value.p[i];
    mask |= 1 << (_6502_RMASK_F + reg);
  }
  VectorClear(&regs);
  
  BitSetExpand(&emitter->regs->used_d_regs, &regs);
  for (size_t i = 1; i < regs.length; i++) {
    int reg = (int)regs.value.p[i];
    mask |= 1 << (_6502_RMASK_D + reg);
  }
  VectorDestruct(&regs);
  fprintf(fp, "\t.hword %d\n", mask);
}


static int RegisterSize(_6502Register* reg) {
  switch (reg->type) {
    case k6502RegTypeA:
      return 2;
    case k6502RegTypeB:
      return 1;
    case k6502RegTypeD:
      return 8;
    case k6502RegTypeF:
      return 4;
    case k6502RegTypeI:
      return 4;
    case k6502RegTypeX:
      return 8;
  }
}

static bool InitIndex(bool done, _6502Register* reg, FILE* fp) {
  if (done) {
    return false;
  }
  switch (reg->location) {
    case kRegSymbol:
    case kRegConstant:
    case kRegZeroPage:
      break;
    case kRegLocalVar:
    case kRegArgument:
      // TODO: offsets bigger than 255?
      fprintf(fp, "\tLDY #%d\n", reg->u.offset);
      return true;
      
  }
  return false;
}

// Return true if Y has been incremented.
static bool IncrementIndex(bool done, _6502Register* reg, int index, int max, FILE* fp) {
  if (done) {
    return false;
  }
  switch (reg->location) {
    case kRegSymbol:
    case kRegZeroPage:
    case kRegConstant:
      break;
    case kRegLocalVar:
    case kRegArgument:
      if (index < max - 1) {
        fprintf(fp, "\tINY\n");
        return true;
      }
      break;
  }
  return false;
}

static void LoadByte(_6502Register* reg, int byte, FILE* fp) {
  static char buf[4096];
  fprintf(fp, "\tLDA %s\n", _6502RegisterAsString(reg, byte, buf, sizeof(buf)));
}

static void OperateByte(_6502Register* reg, int byte, const char *op,
                        FILE* fp) {
  static char buf[4096];
  fprintf(fp, "\t%s %s\n", op, _6502RegisterAsString(reg, byte, buf, sizeof(buf)));
}

static void StoreByte(_6502Register* reg, int byte, FILE* fp) {
  static char buf[4096];
  fprintf(fp, "\tSTA %s\n", _6502RegisterAsString(reg, byte, buf, sizeof(buf)));
}

static void StoreZero(_6502Register* reg, int byte, FILE* fp) {
  static char buf[4096];
  if (reg->location == kRegLocalVar || reg->location == kRegArgument) {
    // The STZ instruction doesn't have a (zp),Y addressing mode so we have
    // to store A instead.
    fprintf(fp, "\tSTA %s\n", _6502RegisterAsString(reg, byte, buf, sizeof(buf)));
  } else {
    fprintf(fp, "\tSTZ %s\n", _6502RegisterAsString(reg, byte, buf, sizeof(buf)));
  }
}

static void CopyConstant(_6502Register* dest, int64_t value, FILE* fp) {
  int size = RegisterSize(dest);
  // TODO: optimize this by sorting byte values and checking for duplicates.
  InitIndex(false, dest, fp);

  int last_value = -1;
  for (int i = 0; i < size; i++) {
    int v = (int)((value >> (i * 8)) & 0xff);
    if (v == 0) {
      if (dest->location == kRegArgument || dest->location == kRegLocalVar) {
        // We don't have an STZ instruction for (zp),y so we have to load
        // a zero into A
        if (last_value != v) {
          fprintf(fp, "\tLDA #0\n");
        }
      }
      StoreZero(dest, i, fp);
    } else {
      if (last_value != v) {
        fprintf(fp, "\tLDA #%d\n", v);
      }
      StoreByte(dest, i, fp);
    }
    IncrementIndex(false, dest, i, size, fp);
    last_value = v;
  }
}

static void CopyRegister(int size, _6502Register* dest, _6502Register* src,
                         FILE *fp) {
  
  InitIndex(InitIndex(false, dest, fp), src, fp);
  for (int i = 0; i < size; i++) {
    LoadByte(src, i, fp);
    StoreByte(dest, i, fp);
    IncrementIndex(IncrementIndex(false, dest, i, size, fp),
                   src, i, size, fp);
  }
  
}


static void Copy(int size, TargetInstruction* dest, TargetInstruction* src,
                 FILE *fp) {
  _6502Register* dest_reg = (_6502Register*)dest->reg;
  if (TargetIsConst(src)) {
    // Store a constant.
    int64_t value = TargetIntValue(src);
    CopyConstant(dest_reg, value, fp);
  } else {
    _6502Register* src_reg = (_6502Register*)src->reg;
    CopyRegister(size, dest_reg, src_reg, fp);
  }
}

// The rmov instructions are an explicit mov from operand[1] to
// operand[0].  Both are registers.
static void PrintRmov(_6502Emitter* emitter, TargetInstruction* inst,
                      FILE* fp) {
  assert(inst->operand[0] != NULL);
  assert(inst->operand[1] != NULL);
  assert(inst->operand[0]->reg != NULL);
  assert(inst->operand[1]->reg != NULL);
  
  // Don't output mov rx,rx.
  if (inst->operand[0]->reg == inst->operand[1]->reg) {
    return;
  }
  
  switch ((_6502Opcode)inst->opcode) {
    case _6502_OP(rmova):
      CopyRegister(2, (_6502Register*)inst->operand[0]->reg, (_6502Register*)inst->operand[1]->reg, fp);
      break;
    case _6502_OP(rmov):
      CopyRegister(4, (_6502Register*)inst->operand[0]->reg, (_6502Register*)inst->operand[1]->reg, fp);
      break;
    case _6502_OP(rmovx):
      CopyRegister(8, (_6502Register*)inst->operand[0]->reg, (_6502Register*)inst->operand[1]->reg, fp);
      break;
    case _6502_OP(rmovf):
      CopyRegister(4, (_6502Register*)inst->operand[0]->reg, (_6502Register*)inst->operand[1]->reg, fp);
      break;
    case _6502_OP(rmovd):
      CopyRegister(8, (_6502Register*)inst->operand[0]->reg, (_6502Register*)inst->operand[1]->reg, fp);
      break;
    default:
      assert(false);
  }
}

// Comparisons in 6502 are reasonably complex.  There's a good tutorial
// on them at http://www.6502.org/tutorials/compare_beyond.html

enum CompareType {
  kCompareInvalid,
  kCompareEqual,
  kCompareNotEqual,
  kCompareLess,
  kCompareLessEqual,
  kCompareGreater,
  kCompareGreaterEqual,
};

static struct CompareInfo {
  _6502Opcode opcode;
  int size;
  bool is_signed;
  bool is_fp;
  enum CompareType cmp_type;
} comparisons[] = {
  // Byte
  {_6502_OP(cmpeqb), 1, true, false, kCompareEqual},
  {_6502_OP(cmpneb), 1, true, false, kCompareNotEqual},
  {_6502_OP(cmpltb), 1, true, false, kCompareLess},
  {_6502_OP(cmpleb), 1, true, false, kCompareLessEqual},
  {_6502_OP(cmpgtb), 1, true, false, kCompareGreater},
  {_6502_OP(cmpgeb), 1, true, false, kCompareGreaterEqual},
  {_6502_OP(cmpltub), 1, false, false, kCompareLess},
  {_6502_OP(cmpleub), 1, false, false, kCompareLessEqual},
  {_6502_OP(cmpgtub), 1, false, false, kCompareGreater},
  {_6502_OP(cmpgeub), 1, false, false, kCompareGreaterEqual},

  // 16-bit
  {_6502_OP(cmpeqa), 2, true, false, kCompareEqual},
  {_6502_OP(cmpnea), 2, true, false, kCompareNotEqual},
  {_6502_OP(cmplta), 2, true, false, kCompareLess},
  {_6502_OP(cmplea), 2, true, false, kCompareLessEqual},
  {_6502_OP(cmpgta), 2, true, false, kCompareGreater},
  {_6502_OP(cmpgea), 2, true, false, kCompareGreaterEqual},
  {_6502_OP(cmpltua), 2, false, false, kCompareLess},
  {_6502_OP(cmpleua), 2, false, false, kCompareLessEqual},
  {_6502_OP(cmpgtua), 2, false, false, kCompareGreater},
  {_6502_OP(cmpgeua), 2, false, false, kCompareGreaterEqual},

  // 32-bit
  {_6502_OP(cmpeqi), 4, true, false, kCompareEqual},
  {_6502_OP(cmpnei), 4, true, false, kCompareNotEqual},
  {_6502_OP(cmplti), 4, true, false, kCompareLess},
  {_6502_OP(cmplei), 4, true, false, kCompareLessEqual},
  {_6502_OP(cmpgti), 4, true, false, kCompareGreater},
  {_6502_OP(cmpgei), 4, true, false, kCompareGreaterEqual},
  {_6502_OP(cmpltui), 4, false, false, kCompareLess},
  {_6502_OP(cmpleui), 4, false, false, kCompareLessEqual},
  {_6502_OP(cmpgtui), 4, false, false, kCompareGreater},
  {_6502_OP(cmpgeui), 4, false, false, kCompareGreaterEqual},

  // 64-bit
  {_6502_OP(cmpeqx), 8, true, false, kCompareEqual},
  {_6502_OP(cmpnex), 8, true, false, kCompareNotEqual},
  {_6502_OP(cmpltx), 8, true, false, kCompareLess},
  {_6502_OP(cmplex), 8, true, false, kCompareLessEqual},
  {_6502_OP(cmpgtx), 8, true, false, kCompareGreater},
  {_6502_OP(cmpgex), 8, true, false, kCompareGreaterEqual},
  {_6502_OP(cmpltux), 8, false, false, kCompareLess},
  {_6502_OP(cmpleux), 8, false, false, kCompareLessEqual},
  {_6502_OP(cmpgtux), 8, false, false, kCompareGreater},
  {_6502_OP(cmpgeux), 8, false, false, kCompareGreaterEqual},

  // 32-bit float
  {_6502_OP(cmpeqf), 4, true, true, kCompareEqual},
  {_6502_OP(cmpnef), 4, true, true, kCompareNotEqual},
  {_6502_OP(cmpltf), 4, true, true, kCompareLess},
  {_6502_OP(cmplef), 4, true, true, kCompareLessEqual},
  {_6502_OP(cmpgtf), 4, true, true, kCompareGreater},
  {_6502_OP(cmpgef), 4, true, true, kCompareGreaterEqual},

  // 64-bit double
  {_6502_OP(cmpeqd), 8, true, true, kCompareEqual},
  {_6502_OP(cmpned), 8, true, true, kCompareNotEqual},
  {_6502_OP(cmpltd), 8, true, true, kCompareLess},
  {_6502_OP(cmpled), 8, true, true, kCompareLessEqual},
  {_6502_OP(cmpgtd), 8, true, true, kCompareGreater},
  {_6502_OP(cmpged), 8, true, true, kCompareGreaterEqual},
};

#define NUM_COMPARE_INFO (sizeof(comparisons) / sizeof(comparisons[0]))

static struct CompareInfo* FindComparison(_6502Opcode opcode) {
  for (int i = 0; i < NUM_COMPARE_INFO; i++) {
    if (comparisons[i].opcode == opcode) {
      return &comparisons[i];
    }
  }
  abort();
}

static void Compare(TargetInstruction* inst, const char* func_name, FILE* fp) {
  char false_label_name[32], true_label_name[32];
  snprintf(false_label_name, sizeof(false_label_name), ".%s_cmp_false_%d", func_name, inst->id);
  snprintf(true_label_name, sizeof(true_label_name), ".%s_cmp_true_%d", func_name, inst->id);
  struct CompareInfo* cmp_info = FindComparison((_6502Opcode)inst->opcode);
  
  // Set the comparison type and the signed bit in the flags (top 16 bits)
  // of the instruction so that the branch instruction can pick them up.
  inst->flags |= cmp_info->cmp_type << 20 | cmp_info->is_signed << 24;
  
  TargetInstruction* op1 = inst->operand[0];
  TargetInstruction* op2 = inst->operand[1];

  _6502Register* reg1 = (_6502Register*)op1->reg;
  _6502Register* reg2 = (_6502Register*)op2->reg;

  bool need_value = (inst->flags & _6502_CMP_EXPR) != 0;
  if (need_value) {
    // If we need to generate a value, we put it in X.  Start off with
    // the value 0 in X and if the comparison succeeds increment it.
    fprintf(fp, "\tLDX #0\n");
  }
  switch (cmp_info->cmp_type) {
    case kCompareInvalid:
      abort();
      break;
    case kCompareEqual:
      // Comparing for equality is simply a matter of comparing each
      // byte and branching to a label at the end if false.
      for (int i = 0; i < cmp_info->size; i++) {
        LoadByte(reg1, i, fp);
        OperateByte(reg2, i, "CMP", fp);
        fprintf(fp, "BNE %s\n", false_label_name);
      }
      if (need_value) {
        fprintf(fp, "\tINX\n");
      }
      fprintf(fp, "%s:\n", false_label_name);
      break;
    case kCompareNotEqual:
      for (int i = 0; i < cmp_info->size; i++) {
        LoadByte(reg1, i, fp);
        OperateByte(reg2, i, "CMP", fp);
        fprintf(fp, "BEQ %s\n", false_label_name);
      }
      if (need_value) {
        fprintf(fp, "\tINX\n");
      }
      fprintf(fp, "%s:\n", false_label_name);
      break;
    case kCompareGreater:
      break;
    case kCompareLess:
      if (cmp_info->is_signed) {
        const char* cmp_inst = "CMP";
        char vc_label[32];
        snprintf(vc_label, sizeof(vc_label), ".cmp_vc_%d", inst->id);
        for (int i = 0; i < cmp_info->size; i++) {
          LoadByte(reg1, i, fp);
          OperateByte(reg2, i, cmp_inst, fp);
          cmp_inst = "SBC";
        }
        fprintf(fp, "\tBVC %s\n", vc_label);
        fprintf(fp, "\tEOR #0x80\n");
        fprintf(fp, "%s:\n", vc_label);
        if (need_value) {
          fprintf(fp, "\tBPL %s\n", false_label_name);
          fprintf(fp, "\tINX\n");
          fprintf(fp, "%s:\n", false_label_name);
        }
      } else {
        for (int i = cmp_info->size; i >= 0; i--) {
          LoadByte(reg1, i, fp);
          OperateByte(reg2, i, "CMP", fp);
          fprintf(fp, "BCC %s\n", true_label_name);
          if (i > 0) {
            fprintf(fp, "BNE %s\n", true_label_name);
          }
        }
      
        fprintf(fp, "%s:\n", true_label_name);
        if (need_value) {
          fprintf(fp, "\tINX\n");
        }
        fprintf(fp, "%s:\n", false_label_name);
      }
      break;
    case kCompareLessEqual:
      break;
    case kCompareGreaterEqual:
      break;
  }
  
  if (need_value) {
    static char buf[4096];
    // Result of comparison will be 1 in X if true.  This is always in
    // a b register.
    fprintf(fp, "\tSTX %s\n", _6502RegisterAsString((_6502Register*)inst->reg, 0,
                                                buf, sizeof(buf)));
  }
 
}

static void EmitBranch(TargetInstruction* inst, const char* label_name, const char* func_name, FILE* fp) {
  TargetInstruction* cmp = inst->operand[0];
  _6502Opcode opcode = (_6502Opcode)inst->opcode;
  bool bt = opcode == _6502_OP(bt);
  bool is_signed = (bool)(cmp->flags >> 24);
  const char* branch;
  char false_label_name[32];
  snprintf(false_label_name, sizeof(false_label_name), ".%s_bra_false_%d", func_name, inst->id);
  enum CompareType cmp_type =( cmp->flags >> 20) & 0xf;

  if (is_signed) {
    switch (cmp_type) {
      case kCompareInvalid:
        abort();
        break;
      case kCompareLess:
        branch = bt ? "BMI" : "BPL";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
      case kCompareEqual:
        branch = bt ? "BEQ" : "BNE";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
      case kCompareGreater:
        branch = bt ? "BPL" : "BMI";
        fprintf(fp, "\tBNE %s\n", false_label_name);
        fprintf(fp, "\t%s %s\n", branch, label_name);
        fprintf(fp, "%s:\n", false_label_name);
        break;
      case kCompareNotEqual:
        branch = bt ? "BNE" : "BEQ";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
      case kCompareLessEqual:
        branch = bt ? "BMI" : "BPL";
        fprintf(fp, "\tBEQ %s\n", false_label_name);
        fprintf(fp, "\t%s %s\n", branch, label_name);
        fprintf(fp, "%s:\n", false_label_name);
        break;
      case kCompareGreaterEqual:
        branch = bt ? "BPL" : "BMI";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
    }
  } else {
    switch (cmp_type) {
      case kCompareInvalid:
        abort();
        break;
      case kCompareLess:
        branch = bt ? "BCC" : "BCS";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
      case kCompareEqual:
        branch = bt ? "BEQ" : "BNE";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
      case kCompareGreater:
        branch = bt ? "BCS" : "BCC";
        fprintf(fp, "\tBNE %s\n", false_label_name);
        fprintf(fp, "\t%s %s\n", branch, label_name);
        fprintf(fp, "%s:\n", false_label_name);
        break;
      case kCompareNotEqual:
        branch = bt ? "BNE" : "BEQ";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
      case kCompareLessEqual:
        branch = bt ? "BCC" : "BCS";
        fprintf(fp, "\tBEQ %s\n", false_label_name);
        fprintf(fp, "\t%s %s\n", branch, label_name);
        fprintf(fp, "%s:\n", false_label_name);
        break;
      case kCompareGreaterEqual:
        branch = bt ? "BCS" : "BCC";
        fprintf(fp, "\t%s %s\n", branch, label_name);
        break;
    }
  }
}

static void EnterSubroutine(_6502Emitter* emitter, TargetInstruction* inst,
                             FILE* fp) {
  fprintf(fp, "\tJSR __enter\n");
  GenerateRegisterMask(emitter, fp);
  fprintf(fp, "\t.hword %d\n", emitter->g->base.stack_frame_size);
}

static void ReturnFromSubroutine(_6502Emitter* emitter, TargetInstruction* inst,
                            FILE* fp) {
  fprintf(fp, "\tJSR __rts\n");
  GenerateRegisterMask(emitter, fp);
  fprintf(fp, "\t.hword %d\n", emitter->g->base.stack_frame_size);
}

static void OperateThroughAccumulator(int size, _6502Register* dest_reg,
                                      TargetInstruction* op1,
                                      TargetInstruction* op2, const char* mnemonic, FILE* fp) {
  _6502Register* reg1 = (_6502Register*)op1->reg;
  _6502Register* reg2 = (_6502Register*)op2->reg;

  for (int i = 0; i < size; i++) {
    LoadByte(reg1, i, fp);
    OperateByte(reg2, i, mnemonic, fp);
    StoreByte(reg2, i, fp);
  }
}

// Add two  values together and store in a register.
static void Add(int size, _6502Register* dest_reg, TargetInstruction* op1, TargetInstruction* op2, FILE* fp) {
  fprintf(fp, "\tCLC\n");
  OperateThroughAccumulator(size, dest_reg, op1, op2, "ADC", fp);
}

static void Sub(int size, _6502Register* dest_reg, TargetInstruction* op1, TargetInstruction* op2, FILE* fp) {
  fprintf(fp, "\tSEC\n");
  OperateThroughAccumulator(size, dest_reg, op1, op2, "SBC", fp);
}

static void And(int size, _6502Register* dest_reg, TargetInstruction* op1, TargetInstruction* op2, FILE* fp) {
  OperateThroughAccumulator(size, dest_reg, op1, op2, "AND", fp);
}

static void Ora(int size, _6502Register* dest_reg, TargetInstruction* op1, TargetInstruction* op2, FILE* fp) {
  OperateThroughAccumulator(size, dest_reg, op1, op2, "ORA", fp);
}

static void Eor(int size, _6502Register* dest_reg, TargetInstruction* op1, TargetInstruction* op2, FILE* fp) {
  OperateThroughAccumulator(size, dest_reg, op1, op2, "EOR", fp);
}

static void Inc(int size, _6502Register* dest_reg, TargetInstruction* inst, TargetInstruction* op1, const char* func_name, FILE* fp) {
  char label[32];
  snprintf(label, sizeof(label), ".%s_inc_%d", func_name, inst->id);
  for (int i = 0; i < size; i++) {
    OperateByte((_6502Register*)op1->reg, i, "INC", fp);
    fprintf(fp, "\tBCC %s\n", label);
  }
  fprintf(fp, "%s:\n", label);
}

static void Push(int size, _6502Register* reg, FILE* fp) {
  InitIndex(false, reg, fp);
  for (int i = 0; i < size; i++) {
    LoadByte(reg, i, fp);
    fprintf(fp, "\tPHA\n");
    IncrementIndex(false, reg, i, size, fp);
  }
  fprintf(fp, "\tJSR __push_reg_%d\n", size);
}

static void Pull(int size, _6502Register* reg, FILE* fp) {
  fprintf(fp, "\tJSR __pull_reg_%d\n", size);
  InitIndex(false, reg, fp);
  for (int i = 0; i < size; i++) {
    fprintf(fp, "\tPLA\n");
    StoreByte(reg, i, fp);
    IncrementIndex(false, reg, i, size, fp);
  }
}

// Main instruction printer.
static void PrintInstruction(_6502Emitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  fprintf(fp, "// ");
  _6502PrintInstruction(inst, fp);
  if (inst->opcode == _6502_OP(label)) {
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    return;
  }
  
  if (inst->opcode == _6502_OP(named_label)) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }

  if (!IsPrintable(inst)) {
    return;
  }
  
  // Buffers for register name printing.
  static char buf2[4096];
  
  switch ((_6502Opcode)inst->opcode) {
    case _6502_OP(movac):
    case _6502_OP(movc):
    case _6502_OP(movxc):
    case _6502_OP(movfc):
    case _6502_OP(movdc):
      assert(TargetIsConst(inst->operand[0]));
      Copy(RegisterSize((_6502Register*)inst->reg), inst, inst->operand[0], fp);
      break;
      
    case _6502_OP(rmova):
    case _6502_OP(rmov):
    case _6502_OP(rmovx):
    case _6502_OP(rmovf):
    case _6502_OP(rmovd):
      PrintRmov(emitter, inst, fp);
      return;
      
    case _6502_OP(symbol): {
      TargetSymbol* sym = (TargetSymbol*)inst;
      if (StorageIs(sym->symbol->storage, STO(static))) {
        fprintf(fp, "\t.local %s\n", sym->symbol->name.value);
      } else {
        fprintf(fp, "\t.global %s\n", sym->symbol->name.value);
      }
      return;
    }
      
    case _6502_OP(decsp): {
      assert(TargetIsConst(inst->operand[0]));
      int size = (int)TargetIntValue(inst->operand[0]);
      int size_lo = size & 0xff;
      int size_hi = size >> 8;
      fprintf(fp, "\tSEC\n");
      fprintf(fp, "\tLDA sp\n");
      fprintf(fp, "\tSBC #%d\n", size_lo);
      fprintf(fp, "\tSTA sp\n");
      fprintf(fp, "\tLDA sp+1\n");
      fprintf(fp, "\tSBC #%d\n", size_hi);
      fprintf(fp, "\tSTA sp+1\n");
      break;
    }

    case _6502_OP(incsp): {
      assert(TargetIsConst(inst->operand[0]));
      int size = (int)TargetIntValue(inst->operand[0]);
      int size_lo = size & 0xff;
      int size_hi = size >> 8;
      fprintf(fp, "\tCLC\n");
      fprintf(fp, "\tLDA sp\n");
      fprintf(fp, "\tADC #%d\n", size_lo);
      fprintf(fp, "\tSTA sp\n");
      fprintf(fp, "\tLDA sp+1\n");
      fprintf(fp, "\tADC #%d\n", size_hi);
      fprintf(fp, "\tSTA sp+1\n");
      break;
    }
      
    case _6502_OP(calla):
    case _6502_OP(calli):
    case _6502_OP(callf):
    case _6502_OP(calld): {
      assert(inst->operand[0]->opcode == _6502_OP(symbol));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      fprintf(fp, "\t%-8s %s\n", "JSR", sym->symbol->name.value);
      return;
    }
      
    case _6502_OP(rcalla):
    case _6502_OP(rcalli):
    case _6502_OP(rcallf):
    case _6502_OP(rcalld):
      fprintf(fp, "\t%-8s %s\n", "JSR",
              _6502RegisterAsString((_6502Register*)inst->operand[0]->reg, 2, buf2,
                                sizeof(buf2)));
      
      return;
    case _6502_OP(enter):
      EnterSubroutine(emitter, inst, fp);
      return;
      
    case _6502_OP(rts):
      ReturnFromSubroutine(emitter, inst, fp);
      return;
      
    case _6502_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);
      
      // Output text directly into assembly output.
      fprintf(fp, "\t%s\n", lit->value.value);
      lit->disabled = true;
      return;
    }
      
    case _6502_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }
      
    case _6502_OP(pushb):
    case _6502_OP(pusha):
    case _6502_OP(pushi):
    case _6502_OP(pushx):
    case _6502_OP(pushf):
    case _6502_OP(pushd): {
      int size = RegisterSize((_6502Register*)inst->operand[0]->reg);
      Push(size, (_6502Register*)inst->operand[0]->reg, fp);
      break;
    }
   
    case _6502_OP(popb):
    case _6502_OP(popa):
    case _6502_OP(popi):
    case _6502_OP(popx):
    case _6502_OP(popf):
    case _6502_OP(popd): {
      int size = RegisterSize((_6502Register*)inst->operand[0]->reg);
      Pull(size, (_6502Register*)inst->operand[0]->reg, fp);
      break;
    }

    case _6502_OP(ldw):
    case _6502_OP(ldh):
    case _6502_OP(ldb):
    case _6502_OP(lduw):
    case _6502_OP(ldub):
    case _6502_OP(lduh):
    case _6502_OP(ldf):
    case _6502_OP(ldx):
    case _6502_OP(ldd):
      assert(inst->operand[0] != NULL);
      assert(inst->reg != NULL);
      Copy(RegisterSize((_6502Register*)inst->reg), inst, inst->operand[0], fp);
      break;
      
    case _6502_OP(stw):
    case _6502_OP(sth):
    case _6502_OP(stx):
    case _6502_OP(stb):
    case _6502_OP(stf):
    case _6502_OP(std):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      Copy(RegisterSize((_6502Register*)inst->reg), inst->operand[1], inst->operand[0], fp);
      
      break;
    case _6502_OP(bt):
    case _6502_OP(bf): {
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      char label_name[32];
      snprintf(label_name, sizeof(label_name),
               ".%s_label_%d", func_name, inst->operand[1]->id);
      EmitBranch(inst, label_name, func_name, fp);
      break;
    }
    case _6502_OP(bra):
      assert(inst->operand[0] != NULL);
      fprintf(fp, "\tBRA .%s_label_%d\n", func_name, inst->operand[0]->id);
      break;
    case _6502_OP(jmp):
      assert(inst->operand[0] != NULL);
      fprintf(fp, "\tJMP .%s_label_%d\n", func_name, inst->operand[0]->id);
      break;
  
    case _6502_OP(cmpeqb):
    case _6502_OP(cmpneb):
    case _6502_OP(cmpltb):
    case _6502_OP(cmpleb):
    case _6502_OP(cmpgtb):
    case _6502_OP(cmpgeb):
    case _6502_OP(cmpltub):
    case _6502_OP(cmpleub):
    case _6502_OP(cmpgtub):
    case _6502_OP(cmpgeub):
      
    case _6502_OP(cmpeqa):
    case _6502_OP(cmpnea):
    case _6502_OP(cmplta):
    case _6502_OP(cmplea):
    case _6502_OP(cmpgta):
    case _6502_OP(cmpgea):
    case _6502_OP(cmpltua):
    case _6502_OP(cmpleua):
    case _6502_OP(cmpgtua):
    case _6502_OP(cmpgeua):
      
    case _6502_OP(cmpeqi):
    case _6502_OP(cmpnei):
    case _6502_OP(cmplti):
    case _6502_OP(cmplei):
    case _6502_OP(cmpgti):
    case _6502_OP(cmpgei):
    case _6502_OP(cmpltui):
    case _6502_OP(cmpleui):
    case _6502_OP(cmpgtui):
    case _6502_OP(cmpgeui):
      
    case _6502_OP(cmpeqx):
    case _6502_OP(cmpnex):
    case _6502_OP(cmpltx):
    case _6502_OP(cmplex):
    case _6502_OP(cmpgtx):
    case _6502_OP(cmpgex):
    case _6502_OP(cmpltux):
    case _6502_OP(cmpleux):
    case _6502_OP(cmpgtux):
    case _6502_OP(cmpgeux):
      
    case _6502_OP(cmpeqf):
    case _6502_OP(cmpnef):
    case _6502_OP(cmpltf):
    case _6502_OP(cmplef):
    case _6502_OP(cmpgtf):
    case _6502_OP(cmpgef):
    case _6502_OP(cmpeqd):
    case _6502_OP(cmpned):
    case _6502_OP(cmpltd):
    case _6502_OP(cmpled):
    case _6502_OP(cmpgtd):
    case _6502_OP(cmpged):
      Compare(inst, func_name, fp);
      break;
  
      // Add.
    case _6502_OP(adda):     // Add address offset (16 bit int).
      Add(2, (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      break;
    case _6502_OP(addac):     // Add constant to address.
      if (TargetIntValue(inst->operand[1]) == 1) {
        Inc(2, (_6502Register*)inst->reg, inst, inst->operand[0], func_name, fp);
      } else {
        Add(2, (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      }
      break;
    case _6502_OP(add):      // Add int.
      Add(RegisterSize((_6502Register*)inst->reg),
          (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      break;
    case _6502_OP(addc):     // Add with constant.
      if (TargetIntValue(inst->operand[1]) == 1) {
        Inc(RegisterSize((_6502Register*)inst->reg),
            (_6502Register*)inst->reg, inst, inst->operand[0], func_name, fp);
      } else {
      Add(RegisterSize((_6502Register*)inst->reg),
          (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      }
      break;
    case _6502_OP(addf):     // Add float.
    case _6502_OP(addd):     // Add double.
      break;
      
      // Subtract.
    case _6502_OP(sub):      // Subtract int.
      Sub(RegisterSize((_6502Register*)inst->reg),
          (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      break;
    case _6502_OP(subf):     // Subtract float.
    case _6502_OP(subd):     // Subtract double.
      break;
      
      // Multiply.
    case _6502_OP(mula):     // Multiply address offset (16 bit(.
    case _6502_OP(mul):      // Multiply int.
    case _6502_OP(mulf):     // Multiply float.
    case _6502_OP(muld):     // Multiply double.
      
      // Divide.
    case _6502_OP(diva):     // Divide address offsets (16 bits).
    case _6502_OP(div):      // Divide int.
    case _6502_OP(divu):     // Divide int unsigned
    case _6502_OP(divf):     // Divide float.
    case _6502_OP(divd):     // Divide double.
      
      // Modulus.
    case _6502_OP(mod):      // Integer modulus.
    case _6502_OP(modu):     // Unsigned nteger modulus.
      
      // Shifts.
    case _6502_OP(lsr):      // Logical shift right.
    case _6502_OP(asr):      // Arithmetic shift right.
    case _6502_OP(lsl):      // Logical shift left.
      
      // Bitwise.
    case _6502_OP(or):       // OR.
      Ora(RegisterSize((_6502Register*)inst->reg),
          (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      break;
    case _6502_OP(and):      // AND.
      And(RegisterSize((_6502Register*)inst->reg),
          (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      break;
    case _6502_OP(xor):      // Exclusive OR.
      Eor(RegisterSize((_6502Register*)inst->reg),
          (_6502Register*)inst->reg, inst->operand[0], inst->operand[1], fp);
      break;

    case _6502_OP(not):      //  != 0 -> 0
    case _6502_OP(inv):      // Ones complement.
      break;
    case _6502_OP(neg):      // Negate int.
    case _6502_OP(negf):     // Negate float.
    case _6502_OP(negd):     // Negate double.
      break;
      
    default:
      _6502PrintInstruction(inst, stdout);
      abort();
      break;
  }
}

void _6502EmitterInit(_6502Emitter* emitter, _6502Generator* g) {
  emitter->g = g;
  emitter->regs = &g->register_allocator;
}

_6502Emitter* New6502Emitter(_6502Generator* pcode) {
  _6502Emitter* emitter = malloc(sizeof(_6502Emitter));
  _6502EmitterInit(emitter, pcode);
  return emitter;
}

void _6502EmitterDestruct(_6502Emitter* emitter) {}

void _6502EmitterDelete(_6502Emitter* emitter) {
  _6502EmitterDestruct(emitter);
  free(emitter);
}

void _6502PrintFunction(_6502Emitter* emitter, FILE* fp) {
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

