//
//  pcode_emitter.c
//  c_compiler
//
//  Created by David Allison on 12/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

// This is the P-CODE assembly language emitter. It prints the P-CODE
// instructions to the given file in assembly language.  The PCodeAssember
// reads this file and generates the binary.

#include "p_code_emitter.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "compiler.h"
#include "p_code_assembler.h"
#include "p_code_reg_alloc.h"

// Is the given instruction printable?  Some instructions do not
// produce any output as they are used for information for other
// instructions.
static bool IsPrintable(TargetInstruction* inst) {
  // Constants are encoded in the instructions that use them.
  if (TargetIsConst(inst)) {
    return false;
  }
  // These opcodes are not printable.
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(tmp):
    case P_OP(fp):
    case P_OP(sp):
    case P_OP(ap):
    case P_OP(tp):
    case P_OP(literal):
    case P_OP(structreturn):
    case P_OP(resulti):
    case P_OP(resultf):
    case P_OP(resultd):
      return false;
    default:
      break;
  }
  return true;
}

// The rmov instructions are an explicit mov from operand[0] to dest.  Both are
// registers.
static void PrintRmov(PCodeEmitter* emitter, TargetInstruction* inst,
                      FILE* fp) {
  assert(inst->operand[0] != NULL);
  assert(inst->dest != NULL);
  assert(inst->operand[0]->reg != NULL);
  assert(inst->dest->reg != NULL);

  PCodeRegister return_reg = {0};
  bool use_return_reg = false;
  PCodeOpcode operand_opcode = (PCodeOpcode)inst->operand[0]->opcode;
  if (operand_opcode == P_OP(call) || operand_opcode == P_OP(rcall)) {
    return_reg.base.num = PCODE_INT_RETURN_REG;
    return_reg.type = kPCodeRegTypeInt;
    use_return_reg = true;
  } else if (operand_opcode == P_OP(callf) || operand_opcode == P_OP(rcallf)) {
    return_reg.base.num = PCODE_FLOAT_RETURN_REG;
    return_reg.type = kPCodeRegTypeFloat;
    use_return_reg = true;
  } else if (operand_opcode == P_OP(calld) || operand_opcode == P_OP(rcalld)) {
    return_reg.base.num = PCODE_DOUBLE_RETURN_REG;
    return_reg.type = kPCodeRegTypeDouble;
    use_return_reg = true;
  }
  PCodeRegister* src_reg =
      use_return_reg ? &return_reg : (PCodeRegister*)inst->operand[0]->reg;

  // Don't output mov rx,rx.
  if (((PCodeRegister*)inst->dest->reg)->type == src_reg->type &&
      inst->dest->reg->num == src_reg->base.num) {
    return;
  }

  const char* mnemonic = "";
  switch (inst->opcode) {
    case P_OP(mov):
      mnemonic = "mov";
      break;
    case P_OP(movf):
      mnemonic = "movf";
      break;
    case P_OP(movd):
      mnemonic = "movd";
      break;
    default:
      assert(false);
  }
  char buf1[8], buf2[8];
  fprintf(fp, "\t%-8s%s, %s\n", mnemonic,
          PCodeRegisterName((PCodeRegister*)inst->dest->reg, buf1,
                            sizeof(buf1)),
          PCodeRegisterName(src_reg, buf2, sizeof(buf2)));
}

// Save all used registers on the stack.
static void SaveRegisters(PCodeEmitter* emitter, FILE* fp) {
  Vector regs = {0};
  BitSetExpand(&emitter->regs->used_int_regs, &regs);
  for (size_t i = 0; i < regs.length; i++) {
    int reg = (int)regs.value.w[i];
    if (reg == PCODE_INT_RETURN_REG) {
      continue;
    }
    fprintf(fp, "\tpushx   r%d\n", reg);
  }
  VectorClear(&regs);

  BitSetExpand(&emitter->regs->used_float_regs, &regs);
  for (size_t i = 0; i < regs.length; i++) {
    int reg = (int)regs.value.w[i];
    if (reg == PCODE_FLOAT_RETURN_REG) {
      continue;
    }
    fprintf(fp, "\tpushf    f%d\n", reg);
  }
  VectorClear(&regs);

  BitSetExpand(&emitter->regs->used_double_regs, &regs);
  for (size_t i = 0; i < regs.length; i++) {
    int reg = (int)regs.value.w[i];
    if (reg == PCODE_DOUBLE_RETURN_REG) {
      continue;
    }
    fprintf(fp, "\tpushd    d%d\n", reg);
  }
  VectorDestruct(&regs);
}

// Restore registers by popping them off the stack in the reverse
// order to which they were pushed.
static void RestoreRegisters(PCodeEmitter* emitter, FILE* fp) {
  Vector regs = {0};

  BitSetExpand(&emitter->regs->used_double_regs, &regs);
  for (size_t i = regs.length; i > 0; i--) {
    int reg = (int)regs.value.w[i - 1];
    if (reg == PCODE_DOUBLE_RETURN_REG) {
      continue;
    }
    fprintf(fp, "\tpopd    d%d\n", reg);
  }
  VectorClear(&regs);

  BitSetExpand(&emitter->regs->used_float_regs, &regs);
  for (size_t i = regs.length; i > 0; i--) {
    int reg = (int)regs.value.w[i - 1];
    if (reg == PCODE_FLOAT_RETURN_REG) {
      continue;
    }
    fprintf(fp, "\tpopf    f%d\n", reg);
  }
  VectorClear(&regs);

  BitSetExpand(&emitter->regs->used_int_regs, &regs);
  for (size_t i = regs.length; i > 0; i--) {
    int reg = (int)regs.value.w[i - 1];
    if (reg == PCODE_INT_RETURN_REG) {
      continue;
    }
    fprintf(fp, "\tpopx    r%d\n", reg);
  }
  VectorDestruct(&regs);
}

// Main instruction printer.
static void PrintInstruction(PCodeEmitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if (((int)inst->opcode == (int)P_OP(label))) {
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    return;
  }
  
  if (((int)inst->opcode == (int)P_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }
  
  if (!IsPrintable(inst)) {
    return;
  }

  // Buffers for register name printing.
  char buf1[8];
  char buf2[8];

  // Special case instructions.
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(mov):
    case P_OP(movf):
    case P_OP(movd):
      if (inst->dest != NULL) {
        PrintRmov(emitter, inst, fp);
        return;
      }
      break;
    case P_OP(symbol): {
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
    case P_OP(call):
    case P_OP(callf):
    case P_OP(calld): {
      assert(((int)inst->operand[0]->opcode == (int)P_OP(symbol)));
      TargetSymbol* sym = (TargetSymbol*)inst->operand[0];
      char namebuf[256];
      fprintf(fp, "\t%-8s %s\n", "call",
              TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf)));
      return;
    }

    case P_OP(rcall):
    case P_OP(rcallf):
    case P_OP(rcalld):
      fprintf(fp, "\t%-8s %s\n", "rcall",
              PCodeRegisterName((PCodeRegister*)inst->operand[0]->reg, buf2,
                                sizeof(buf2)));

      return;
    case P_OP(save):
      SaveRegisters(emitter, fp);
      return;

    case P_OP(restore):
      RestoreRegisters(emitter, fp);
      return;

    case P_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t%s\n", lit->value.value);
      lit->base.disabled = true;
      return;
    }

    case P_OP(loc): {
      if (!emitter->emit_locations) {
        return;
      }
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }
      
    default:
      break;
  }

  // General case for instruction printing.

  // Print opcode.  The trailing space guarantees a separator even when the
  // mnemonic fills the whole field (e.g. the 8-character cmp3way* opcodes).
  fprintf(fp, "\t%-8s ", PCodeOpcodeName(inst->opcode));

  // Print operands.
  switch ((PCodeOpcode)inst->opcode) {
    case P_OP(ldw):
    case P_OP(ldh):
    case P_OP(ldb):
    case P_OP(lduw):
    case P_OP(ldub):
    case P_OP(lduh):
    case P_OP(ldf):
    case P_OP(ldx):
    case P_OP(ldd):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->reg != NULL);
      assert(inst->operand[0]->reg != NULL);
      assert(TargetIsConst(inst->operand[1]));
      fprintf(fp, "%s, [%s, #%d]\n",
              PCodeRegisterName((PCodeRegister*)inst->reg, buf1, sizeof(buf1)),
              PCodeRegisterName((PCodeRegister*)inst->operand[0]->reg, buf2,
                                sizeof(buf2)),
              (int)TargetIntValue(inst->operand[1]));
      break;

    case P_OP(stw):
    case P_OP(sth):
    case P_OP(stx):
    case P_OP(stb):
    case P_OP(stf):
    case P_OP(std):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[2] != NULL);
      assert(inst->operand[0]->reg != NULL);
      assert(inst->operand[1]->reg != NULL);
      assert(TargetIsConst(inst->operand[2]));
      fprintf(fp, "%s, [%s, #%d]\n",
              PCodeRegisterName((PCodeRegister*)inst->operand[0]->reg, buf1,
                                sizeof(buf1)),
              PCodeRegisterName((PCodeRegister*)inst->operand[1]->reg, buf2,
                                sizeof(buf2)),
              (int)TargetIntValue(inst->operand[2]));
      break;
    case P_OP(bz):
    case P_OP(bnz):
      assert(inst->operand[0] != NULL);
      assert(inst->operand[1] != NULL);
      assert(inst->operand[0]->reg != NULL);
      fprintf(fp, "%s, .%s_label_%d\n",
              PCodeRegisterName((PCodeRegister*)inst->operand[0]->reg, buf1,
                                sizeof(buf1)),
              func_name, inst->operand[1]->id);
      break;
    case P_OP(bra):
      assert(inst->operand[0] != NULL);
      fprintf(fp, ".%s_label_%d\n", func_name, inst->operand[0]->id);
      break;

    default: {
      const char* sep = "";
      if (inst->reg != NULL) {
        fprintf(
            fp, "%s",
            PCodeRegisterName((PCodeRegister*)inst->reg, buf1, sizeof(buf1)));
        sep = ", ";
      }
      for (int i = 0; i < 2; i++) {
        if (inst->operand[i] != NULL) {
          if (TargetIsConst(inst->operand[i])) {
            TargetConstant* constant = (TargetConstant*)inst->operand[i];
            if (constant->type == kTargetTypeFloat ||
                constant->type == kTargetTypeDouble) {
              fprintf(fp, "%s#%g", sep, constant->value.dvalue);
            } else {
              fprintf(fp, "%s#%" PRId64, sep, constant->value.ivalue);
            }
          } else if (((int)inst->operand[i]->opcode == (int)P_OP(symbol))) {
            TargetSymbol* sym = (TargetSymbol*)inst->operand[i];
            char namebuf[256];
            fprintf(fp, "%s%s", sep,
                    TargetSymbolName(sym->symbol, namebuf, sizeof(namebuf)));
            if (StorageIs(sym->symbol->storage, STO(thread))) {
              fprintf(fp, "@tls");
            }
          } else if (((int)inst->operand[i]->opcode == (int)P_OP(literal))) {
            TargetLiteral* literal = (TargetLiteral*)inst->operand[i];
            fprintf(fp, "%s.str.%d", sep, literal->literal_id);
          } else {
            fprintf(fp, "%s%s", sep,
                    PCodeRegisterName((PCodeRegister*)inst->operand[i]->reg,
                                      buf2, sizeof(buf2)));
          }
          sep = ", ";
        }
      }
      fprintf(fp, "\n");
    }
  }
}

void PCodeEmitterInit(PCodeEmitter* emitter, PCodeGenerator* pcode) {
  emitter->pcode = pcode;
  emitter->regs = &pcode->register_allocator;
  emitter->emit_locations = true;
}

PCodeEmitter* NewPCodeEmitter(PCodeGenerator* pcode) {
  PCodeEmitter* emitter = malloc(sizeof(PCodeEmitter));
  PCodeEmitterInit(emitter, pcode);
  return emitter;
}

void PCodeEmitterDestruct(PCodeEmitter* emitter) {}

void PCodeEmitterDelete(PCodeEmitter* emitter) {
  PCodeEmitterDestruct(emitter);
  free(emitter);
}

void PCodePrintFunction(PCodeEmitter* emitter, FILE* fp) {
  const char* func_name = emitter->pcode->base.function_name.value;
  if (emitter->pcode->base.is_weak) {
    fprintf(fp, "\t.weak %s\n", func_name);
  } else if (emitter->pcode->base.is_global) {
    fprintf(fp, "\t.global %s\n", func_name);
  } else {
    fprintf(fp, "\t.local  %s\n", func_name);
  }
  fprintf(fp, "\t.type %s, @function\n\n", func_name);
  fprintf(fp, "%s:\n", func_name);
  
  TargetInstruction* inst = TargetFirstInstruction(&emitter->pcode->base);
  while (inst != NULL) {
    PrintInstruction(emitter, inst, func_name, fp);
    inst = TargetNext(inst);
  }

  fprintf(fp, ".func_end_%s:\n", func_name);
  fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
}
