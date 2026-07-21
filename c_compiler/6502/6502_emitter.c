//
//  6502_emitter.c
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_emitter.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "6502_assembler.h"
#include "6502_reg_alloc.h"
#include "compiler.h"
#include "map.h"

// Stack frame
// +--------------------+
// |                    |
// |    Previous        |
// |      frame         |
// |                    |
// +--------------------+    <- previous sp, new fp
// |   save mask (24)   |
// +--------------------+    <- reg save mask @fp-2
// |                    |
// |                    |    <- variables (accessed via fp-X)
// |                    |
// |                    |
// |                    |
// +--------------------+
// |                    |
// |    spilled regs    |
// |                    |
// +--------------------+
// |   result addr      |    <- not present for leaf procs
// +--------------------+
// |   saved fp         |
// +--------------------+    <- current sp
//
static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 528) {
    printf("");
  }
}

static int SpillRegion(W65C02Emitter* emitter) {
  return emitter->g->base.stack_frame_size;
}

static int FrameSize(W65C02Emitter* emitter, bool is_leaf) {
  return emitter->g->base.stack_frame_size +
      emitter->regs->max_spilled_region_size + 2 +
      (is_leaf ? 0 : 2);
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
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(tmp):
    case W65C02_OP(fp):
    case W65C02_OP(sp):
    case W65C02_OP(ap):
    case W65C02_OP(tp):
    case W65C02_OP(literal):
    case W65C02_OP(resulti):
    case W65C02_OP(resultf):
    case W65C02_OP(resultd):
    case W65C02_OP(ret):
    case W65C02_OP(localvar):
    case W65C02_OP(argument):
    case W65C02_OP(symbol):
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
    case W65C02_OP(fake_bra):
    case W65C02_OP(ssavar):
    case W65C02_OP(phi):
    case W65C02_OP(reloadpoint):
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
    case W65C02_OP(literalrefX):
      return false;
    default:
      break;
  }
  return true;
}

static TargetInstruction* NextPrintable(TargetInstruction* inst) {
  for (inst = TargetNext(inst); inst != NULL; inst = TargetNext(inst)) {
    if (IsPrintable(inst)) {
      return inst;
    }
  }
  return NULL;
}

static TargetInstruction* PreviousPrintable(TargetInstruction* inst) {
  for (inst = TargetPrev(inst); inst != NULL; inst = TargetPrev(inst)) {
    if (IsPrintable(inst)) {
      return inst;
    }
  }
  return NULL;
}

static bool AddressResultDiesBeforeReuse(TargetInstruction* result,
                                         TargetInstruction* push) {
  for (TargetInstruction* inst = TargetNext(push); inst != NULL;
       inst = TargetNext(inst)) {
    if (!IsPrintable(inst)) {
      continue;
    }
    if (TargetOpcodeEq(inst->opcode, W65C02_OP(jsr)) ||
        TargetOpcodeEq(inst->opcode, W65C02_OP(jmp))) {
      return true;
    }
    if (TargetOpcodeEq(inst->opcode, W65C02_OP(label)) ||
        TargetOpcodeEq(inst->opcode, W65C02_OP(named_label)) ||
        W65C02IsBranch(inst)) {
      return false;
    }
    for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
      if (inst->operand[i] == result) {
        return false;
      }
    }
  }
  return false;
}

static bool IsAddressPushPair(TargetInstruction* address,
                              TargetInstruction* push) {
  if (address == NULL || push == NULL ||
      !TargetOpcodeEq(push->opcode, W65C02_OP(pushreg2))) {
    return false;
  }
  switch ((W65C02Opcode)address->opcode) {
    case W65C02_OP(var_addr):
    case W65C02_OP(var_addrb):
    case W65C02_OP(arg_addr):
    case W65C02_OP(arg_addrb):
      break;
    default:
      return false;
  }

  TargetInstruction* result = address->operand[0];
  if (push->operand[0] != result) {
    return false;
  }
  return AddressResultDiesBeforeReuse(result, push);
}

typedef struct {
  TargetInstruction* pull;
  TargetInstruction* store_low;
  TargetInstruction* store_high;
  TargetInstruction* result;
} PullRegisterMatch;

typedef struct {
  PullRegisterMatch pulled;
  TargetInstruction* increment;
  TargetInstruction* address;
  TargetInstruction* push_result;
  TargetInstruction* push_address;
  int drop_size;
} ReplaceTopMatch;

static bool IsRuntimeCall(TargetInstruction* inst, Symbol* symbol) {
  return inst != NULL && TargetOpcodeEq(inst->opcode, W65C02_OP(jsr)) &&
         inst->operand[0] != NULL &&
         TargetOpcodeEq(inst->operand[0]->opcode, W65C02_OP(symbol)) &&
         ((TargetSymbol*)inst->operand[0])->symbol == symbol;
}

static int StackIncrementSize(W65C02Emitter* emitter,
                              TargetInstruction* inst) {
  if (IsRuntimeCall(inst, emitter->g->incsp2)) return 2;
  if (IsRuntimeCall(inst, emitter->g->incsp4)) return 4;
  if (IsRuntimeCall(inst, emitter->g->incsp6)) return 6;
  if (IsRuntimeCall(inst, emitter->g->incsp8)) return 8;
  if (IsRuntimeCall(inst, emitter->g->incsp10)) return 10;
  if (IsRuntimeCall(inst, emitter->g->incsp12)) return 12;
  if (IsRuntimeCall(inst, emitter->g->incsp14)) return 14;
  if (IsRuntimeCall(inst, emitter->g->incsp16)) return 16;
  return 0;
}

static bool MatchPullRegister(W65C02Emitter* emitter,
                              TargetInstruction* pull,
                              PullRegisterMatch* match) {
  if (!IsRuntimeCall(pull, emitter->g->pullxy)) {
    return false;
  }
  TargetInstruction* store_low = NextPrintable(pull);
  if (store_low == NULL) {
    return false;
  }
  TargetInstruction* store_high = NextPrintable(store_low);
  if (store_high == NULL ||
      !TargetOpcodeEq(store_low->opcode, W65C02_OP(stx)) ||
      !TargetOpcodeEq(store_high->opcode, W65C02_OP(sty)) ||
      store_low->operand[0] != store_high->operand[0] ||
      TargetIntValue(store_low->operand[1]) != 0 ||
      TargetIntValue(store_high->operand[1]) != 1) {
    return false;
  }
  *match =
      (PullRegisterMatch){pull, store_low, store_high, store_low->operand[0]};
  return true;
}

static bool FindPullRegisterMatch(W65C02Emitter* emitter,
                                  TargetInstruction* inst,
                                  PullRegisterMatch* match) {
  TargetInstruction* candidate = inst;
  for (int i = 0; candidate != NULL && i < 3; i++) {
    if (MatchPullRegister(emitter, candidate, match)) {
      return true;
    }
    candidate = PreviousPrintable(candidate);
  }
  return false;
}

static bool MatchReplaceTop(W65C02Emitter* emitter, TargetInstruction* pull,
                            ReplaceTopMatch* match) {
  PullRegisterMatch pulled;
  if (!MatchPullRegister(emitter, pull, &pulled)) {
    return false;
  }
  TargetInstruction* increment = NextPrintable(pulled.store_high);
  int drop_size = StackIncrementSize(emitter, increment);
  if (drop_size == 0) {
    return false;
  }

  TargetInstruction* next = NextPrintable(increment);
  TargetInstruction* result = pulled.result;
  if (next != NULL && TargetOpcodeEq(next->opcode, W65C02_OP(pushreg2)) &&
      next->operand[0] == result) {
    *match =
        (ReplaceTopMatch){pulled, increment, NULL, next, NULL, drop_size};
    return true;
  }

  TargetInstruction* address = next;
  if (address == NULL) {
    return false;
  }
  TargetInstruction* push_result = NextPrintable(address);
  if (push_result == NULL) {
    return false;
  }
  TargetInstruction* push_address = NextPrintable(push_result);
  if (push_address == NULL ||
      !TargetOpcodeEq(push_result->opcode, W65C02_OP(pushreg2)) ||
      push_result->operand[0] != result ||
      !IsAddressPushPair(address, push_address)) {
    return false;
  }
  *match = (ReplaceTopMatch){
      pulled, increment, address, push_result, push_address, drop_size};
  return true;
}

static bool FindReplaceTopMatch(W65C02Emitter* emitter,
                                TargetInstruction* inst,
                                ReplaceTopMatch* match) {
  TargetInstruction* candidate = inst;
  for (int i = 0; candidate != NULL && i < 7; i++) {
    if (MatchReplaceTop(emitter, candidate, match)) {
      return true;
    }
    candidate = PreviousPrintable(candidate);
  }
  return false;
}

static COMPILER_UNUSED int RegisterSize(W65C02Register* reg) {
  switch (reg->type) {
    case k6502RegTypeI:
      return 2;
    case k6502RegTypeB:
      return 1;
    case k6502RegTypeF:
    case k6502RegTypeL:
      return 4;
    case k6502RegTypeX:
      return 8;
  }
}

static COMPILER_UNUSED void PrintRegister(W65C02Register* reg, FILE* fp) {}

static struct {
  int offset;
  const char* name;
} zero_page_locations[] = {
    {W65C02_SP_REG, "__sp"},
    {W65C02_FP_REG, "__fp"},
    {W65C02_T0_REG, "__t0"},
    {W65C02_T1_REG, "__t1"},
    {W65C02_RESULT_REG, "__result"},
    {W65C02_RESULT_REG+1, "__result+1"},
    {W65C02_MSRC_REG, "__mem_src"},
    {W65C02_MSRC_REG + 1, "__mem_src+1"},
    {W65C02_MDST_REG, "__mem_dest"},
    {W65C02_MDST_REG + 1, "__mem_dest+1"},
    {W65C02_MSZ_REG, "__mem_size"},
    {W65C02_MSZ_REG + 1, "__mem_size+1"},
    {-1, NULL},
};

static const char* ZeroPageLocation(int offset) {
  for (size_t i = 0; zero_page_locations[i].offset != -1; i++) {
    if (zero_page_locations[i].offset == offset) {
      return zero_page_locations[i].name;
    }
  }
  abort();
}

static void PrintOperand(W65C02Emitter* emitter, TargetInstruction* inst,
                         const char* func_name, FILE* fp) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  TargetInstruction* operand = inst->operand[0];
  if (operand == NULL) {
    if (mode == kAddrModeAccumulator) {
      fprintf(fp, " A");
    }
    return;
  }
  int offset = 0;
  W65C02Register* reg = (W65C02Register*)operand->reg;
  // C++ mangled names routinely exceed 32 bytes.  Truncating an operand here
  // creates a relocation for a different, undefined symbol while the label
  // definition retains its full name.
  char buf[1024];
  switch (mode) {
    case kAddrModeInvalid:
    case kAddrModeUnknown:
      abort();
      
    case kAddrModeAbsolute:
      if (inst->operand[0]->opcode == (TargetOpcode)W65C02_OP(symbol)) {
        fprintf(fp, "%s", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      } else if (inst->operand[0]->opcode == (TargetOpcode)W65C02_OP(label)) {
        fprintf(fp, ".%s_label_%d", func_name, inst->operand[0]->id);
      } else {
        abort();
      }
      break;
    case kAddrModeSymbolAddr:
      offset = (int)TargetIntValue(inst->operand[1]);
      if (offset == 0) {
        fprintf(fp, "#%%lo(%s)", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      } else {
        fprintf(fp, "#%%hi(%s)",
                TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      }
      break;
    case kAddrModeAbsoluteSymbol:
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s+%d", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)),  offset);
      break;
    case kAddrModeAbsoluteSymbolIndexedY:
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s+%d, Y", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)),  offset);
      break;
    case kAddrModeAbsoluteSymbolIndexedX:
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s+%d, X", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)),  offset);
      break;
    case kAddrModeImplied:
      break;
    case kAddrModeAccumulator:
      fprintf(fp, " A");
      break;
    case kAddrModeIndirect:
      if (!Is65c02()) {
        if (TargetOpcodeNe(inst->opcode, W65C02_OP(jmp))) {
          fprintf(stderr, "Invalid 6502 indirect instruction\n");
          abort();
        }
      }
      fprintf(
          fp, "(%s)",
          W65C02RegisterAsString((W65C02Register*)operand->reg, 0, buf, sizeof(buf)));
      break;
    case kAddrModeRelative:
      fprintf(fp, ".%s_label_%d", func_name, operand->id);
      break;
    case kAddrModeSymbolHi:
      fprintf(fp, "%%lo(%s)", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      break;
    case kAddrModeSymbolLo:
      fprintf(fp, "%%hi(%s)", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      break;
    case kAddrModeZeroPage:
      assert(inst->operand[1] != NULL);
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s", W65C02RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
    case kAddrModeZeroPageAbsolute:
      offset = (int)TargetIntValue(inst->operand[0]);
      fprintf(fp, "%s", ZeroPageLocation(offset));
      break;
    case kAddrModeImmediate: {
      assert(TargetIsConst(inst->operand[0]));
      TargetConstant* c = (TargetConstant*)inst->operand[0];
      int64_t value;      // Contains binary for whole value.
      switch (c->type) {
        case kTargetTypeFloat:
        case kTargetTypeDouble: {
          float f = c->value.dvalue;    // Convert to float.
          value = *(int32_t*)&f;
          break;
        }
        default:
          value = c->value.ivalue;
          break;
      }
      if (inst->operand[1] != NULL) {
        int bytenum = (int)TargetIntValue(inst->operand[1]);
        value = (value >> (bytenum * 8)) & 0xff;
      }
      fprintf(fp, " #%d", (int)value & 0xff);
      break;
    }
    case kAddrModeZeroPageImmediate:
      assert(reg != NULL);
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "#%s", W65C02RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
    case kAddrModeIndirectIndexed:
      assert(reg != NULL);
      fprintf(fp, "(%s), Y", W65C02RegisterAsString(reg, 0, buf, sizeof(buf)));
      break;
    case kAddrModeIndexedIndirect:
      assert(reg != NULL);
      fprintf(fp, "(%s, X)", W65C02RegisterAsString(reg, 0, buf, sizeof(buf)));
      break;
    case kAddrModeAbsoluteIndexedX:
      break;
    case kAddrModeLiteralIndexedX: {
      assert(TargetOpcodeEq(operand->opcode, W65C02_OP(literalrefX)));
      TargetConstant* literal = (TargetConstant*)operand->operand[0];
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      const char* label = lit->type == kLiteralBuffer ? "lit" : "str";
      fprintf(fp, ".%s.%d, X", label, literal->literal_id);
      break;
    }
    case kAddrModeZeroPageIndexedX:
      assert(reg != NULL);
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s, X",
              W65C02RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
    case kAddrModeAbsoluteIndexedY:
      break;
    case kAddrModeZeroPageIndexedY:
      assert(reg != NULL);
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s, Y",
              W65C02RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
   }
}

static AsmOperand* GetAsmOperand(W65C02AsmInstruction* inst, int index) {
  if (inst->asm_node == NULL || index < 0 || index >= inst->num_operands) {
    return NULL;
  }
  if ((size_t)index < inst->asm_node->outputs.length) {
    return inst->asm_node->outputs.value.p[index];
  }
  return inst->asm_node->inputs.value.p[index - inst->asm_node->outputs.length];
}

static int FindAsmOperandByName(W65C02AsmInstruction* inst, const char* name,
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

static int FindAsmLabelByName(W65C02AsmInstruction* inst, const char* name,
                              size_t len) {
  if (inst->asm_node == NULL) {
    return -1;
  }
  for (size_t i = 0; i < inst->asm_node->labels.length; i++) {
    String* label = inst->asm_node->labels.value.p[i];
    if (label->length == len && strncmp(label->value, name, len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static void PrintAsmOperand(FILE* fp, W65C02AsmInstruction* inst, int index) {
  if (index < 0 || index >= inst->num_operands) {
    fprintf(fp, "<bad-asm-operand>");
    return;
  }
  if (inst->is_immediate[index]) {
    fprintf(fp, "%" PRId64, inst->immediate_values[index]);
    return;
  }
  TargetInstruction* zp = inst->zp_operands[index];
  if (zp == NULL || zp->reg == NULL) {
    fprintf(fp, "<bad-asm-operand>");
    return;
  }
  char buf[32];
  fprintf(fp, "%s", W65C02RegisterAsString((W65C02Register*)zp->reg, 0, buf,
                                           sizeof(buf)));
}

static void PrintAsmLabel(FILE* fp, W65C02AsmInstruction* inst, int index) {
  if (inst->asm_node == NULL || index < 0 ||
      (size_t)index >= inst->asm_node->labels.length) {
    fprintf(fp, "<bad-asm-label>");
    return;
  }
  String* label = inst->asm_node->labels.value.p[index];
  fprintf(fp, "%s", label->value);
}

static void PrintExtendedAsm(FILE* fp, W65C02AsmInstruction* inst,
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
static void PrintInstruction(W65C02Emitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  if ((inst->flags & k6502DontEmit) != 0) {
    return;
  }
  const bool trace = false;     // Print IR instructions.
  const bool show_id = false;    // Show IR instruction id in output.
  const bool chkaddr = false;   // Set to true to check instruction addresses.
  if (trace) {
    fprintf(fp, "// ");
    W65C02PrintInstruction(inst, fp);
  }
  if (chkaddr) {
    fprintf(fp, ".chkaddr 0x%x\n", inst->addr);
  }
  TrapInstruction(inst);
  if (TargetOpcodeEq(inst->opcode, W65C02_OP(label))) {
    fprintf(fp, ".%s_label_%d:\n", func_name, inst->id);
    return;
  }

  if (TargetOpcodeEq(inst->opcode, W65C02_OP(named_label))) {
    TargetNamedLabel* label = (TargetNamedLabel*)inst;
    fprintf(fp, "%s:\n", label->name);
    return;
  }

  if (!IsPrintable(inst)) {
    return;
  }

  if (show_id) {
    fprintf(fp, "/* @%d */ ", inst->id);
  }

  // Buffers for register name printing.
  static char buf[4096];
  W65C02Opcode opcode = (W65C02Opcode)inst->opcode;
  PullRegisterMatch pulled;
  bool has_pulled = FindPullRegisterMatch(emitter, inst, &pulled);
  ReplaceTopMatch replace_top;
  bool has_replace_top = FindReplaceTopMatch(emitter, inst, &replace_top);
  if (has_pulled &&
      (inst == pulled.store_low || inst == pulled.store_high)) {
    return;
  }
  if (has_replace_top &&
      (inst == replace_top.increment || inst == replace_top.push_result ||
       inst == replace_top.push_address)) {
    return;
  }
  if (has_pulled && inst == pulled.pull) {
    const char* reg_name = W65C02RegisterAsString(
        (W65C02Register*)pulled.result->reg, 0, buf, sizeof(buf));
    if (has_replace_top && replace_top.pulled.pull == inst) {
      fprintf(fp, "\t%-12s #%d\n", "lda", replace_top.drop_size);
      fprintf(fp, "\t%-12s #%s\n", "ldx", reg_name);
      fprintf(fp, "\tjsr         __replace_top_reg2\n");
    } else {
      fprintf(fp, "\t%-12s #%s\n", "ldx", reg_name);
      fprintf(fp, "\tjsr         __pullreg2\n");
    }
    return;
  }
  switch (opcode) {
    case W65C02_OP(literalreflo): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      const char* label = lit->type == kLiteralBuffer ? "lit" : "str";
      fprintf(fp, "\tlda         #%%lo(.%s.%d)\n", label,
              literal->literal_id);
      break;
    }
      
    case W65C02_OP(literalrefhi): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      const char* label = lit->type == kLiteralBuffer ? "lit" : "str";
      fprintf(fp, "\tlda         #%%hi(.%s.%d)\n", label,
               literal->literal_id);
        break;
    }

 case W65C02_OP(literalref): {
   TargetConstant* literal = (TargetConstant*)inst->operand[0];
   Literal* lit = CompilerFindLiteral(literal->literal_id);
   assert(lit != NULL);
   const char* label = lit->type == kLiteralBuffer ? "lit" : "str";
   fprintf(fp, "\tldx         #%%lo(.%s.%d)\n", label,
           literal->literal_id);
   fprintf(fp, "\tldy         #%%hi(.%s.%d)\n", label,
            literal->literal_id);
   break;
 }

    case W65C02_OP(stringliteralref): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      (void)lit;
      const char* label = "str";
      fprintf(fp, "\tldx         #%%lo(.%s.%d)\n", label,
              literal->literal_id);
      fprintf(fp, "\tldy         #%%hi(.%s.%d)\n", label,
               literal->literal_id);
      break;
    }
      
    case W65C02_OP(load_result): {
      // X,Y = offset from fp to result address which is frame_size + 3 - 2
      int offset = FrameSize(emitter, false) + 1;
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      const char* suffix = "";
      if (offset >= 256) {
        suffix = "+2";
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s __%s%s\n", "jsr", "load_result",
               suffix);
      break;
    }

    case W65C02_OP(enter):
    case W65C02_OP(enter_leaf): {
      // TODO: if the function is void or returns struct there's no need
      // for the extra 2 bytes for the return address.
      int frame_size = FrameSize(emitter, opcode == W65C02_OP(enter_leaf));
      fprintf(fp, "\t%-12s #%d\n", "ldx", frame_size & 0xff);
      const char* suffix1 = "";
      const char* suffix2 = "";
      if (frame_size >= 256) {
        suffix2 = "+2";
        fprintf(fp, "\t%-12s #%d\n", "ldy", (frame_size >> 8) & 0xff);
      }
      uint32_t mask = W65C02RegisterAllocatorBuildRegMask(emitter->regs);
      if (mask == 0) {
        suffix1 = "_nomask";
      }
      fprintf(fp, "\t%-12s __%s%s%s\n", "jsr",
              opcode == W65C02_OP(enter_leaf) ? "enter_leaf" : "enter", suffix1, suffix2);
      
      if (mask != 0) {
        // Write out 24 bit save mask.
        fprintf(fp, "\t%-12s", ".byte");
        char* sep = " ";
        for (int i = 0; i < 3; i++) {
          fprintf(fp, "%s0x%02x", sep, (mask >> i*8) & 0xff);
          sep = ",";
        }
        fprintf(fp, "\t\t// Save mask %s\n", W65C02RegisterAllocatorPrintRegMask(mask, buf));
      }
      break;
    }
    case W65C02_OP(leave):
    case W65C02_OP(leave_leaf): {
      int frame_size = FrameSize(emitter, opcode == W65C02_OP(leave_leaf));
      frame_size += 3;                  // Space for reg save mask.
      fprintf(fp, "\t%-12s #%d\n", "ldy", frame_size & 0xff);
      const char* suffix1 = "";
      const char* suffix2 = "";
      if (frame_size >= 256) {
        suffix2 = "+2";
        fprintf(fp, "\t%-12s #%d\n", "ldx", (frame_size >> 8) & 0xff);
      }
      const char* leave_func;

      if (opcode == W65C02_OP(leave_leaf)) {
        if (emitter->g->base.is_void) {
          leave_func = "leave_leaf_void";
        } else {
          leave_func = "leave_leaf";
        }
      } else {
        if (emitter->g->base.is_void) {
          leave_func = "leave_void";
        } else {
          leave_func = "leave";
        }
      }
      uint32_t mask = W65C02RegisterAllocatorBuildRegMask(emitter->regs);
      if (mask == 0) {
        suffix1 = "_nomask";
      }
      bool use_jmp = (inst->flags & k6502JmpForJSR) != 0;
      fprintf(fp, "\t%-12s __%s%s%s\n", use_jmp ? "jmp" : "jsr",
              leave_func, suffix1, suffix2);
      break;
    }
    case W65C02_OP(var_addr):
    case W65C02_OP(var_addrb): {
      // // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr[b] (or arg_addr[b])
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      int offset = (int)TargetIntValue(var->operand[0]);
//      fprintf(fp, "\t%-12s #%s\t\t\t// %s\n", "lda",
//              W65C02RegisterAsString((W65C02Register*)result->reg, 0, buf, sizeof(buf)),
//              ((TargetSymbol*)var)->symbol->name.value);
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      if (offset >= 256) {
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      TargetInstruction* push = NextPrintable(inst);
      if ((has_replace_top && replace_top.address == inst) ||
          IsAddressPushPair(inst, push)) {
        W65C02Register* reg = (W65C02Register*)result->reg;
        if (opcode == W65C02_OP(var_addr) &&
            reg->type == k6502RegTypeI &&
            reg->base.num < W65C02_NUM_I_REGS) {
          fprintf(fp, "\tjsr         __var_addr_push_i%d\n", reg->base.num);
          break;
        }
        fprintf(fp, "\t%-12s #%s\n", "lda",
                W65C02RegisterAsString(reg, 0, buf, sizeof(buf)));
        fprintf(fp, "\t%-12s __%s\n", "jsr",
                opcode == W65C02_OP(var_addr) ? "var_addr_push"
                                              : "var_addrb_push");
        break;
      }
      fprintf(fp, "\t%-12s __%s_i%d\t\t\t// %s\n", "jsr",
              opcode == W65C02_OP(var_addr) ? "var_addr" : "var_addrb", result->reg->num,
              ((TargetSymbol*)var)->symbol->name.value);
      break;
    }

    case W65C02_OP(arg_addr):
    case W65C02_OP(arg_addrb): {
      // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __arg_addr[b] (or arg_addr[b])
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      int offset = (int)TargetIntValue(var->operand[0]);
//      fprintf(fp, "\t%-12s #%s\t\t\t// %s\n", "lda",
//              W65C02RegisterAsString((W65C02Register*)result->reg, 0, buf, sizeof(buf)),
//              ((TargetSymbol*)var)->symbol->name.value);
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      if (offset >= 256) {
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      TargetInstruction* push = NextPrintable(inst);
      if ((has_replace_top && replace_top.address == inst) ||
          IsAddressPushPair(inst, push)) {
        fprintf(fp, "\t%-12s #%s\n", "lda",
                W65C02RegisterAsString((W65C02Register*)result->reg, 0, buf,
                                       sizeof(buf)));
        fprintf(fp, "\t%-12s __%s\n", "jsr",
                opcode == W65C02_OP(arg_addr) ? "arg_addr_push"
                                              : "arg_addrb_push");
        break;
      }
      fprintf(fp, "\t%-12s __%s_i%d\t\t\t// %s\n", "jsr",
              opcode == W65C02_OP(arg_addr) ? "arg_addr" : "arg_addrb", result->reg->num,
              ((TargetSymbol*)var)->symbol->name.value);
      break;
    }

    case W65C02_OP(var_addr_xy):
    case W65C02_OP(var_addrb_xy): {
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr[b]_xy (or arg_addr[b]_xy)
      TargetInstruction* var = inst->operand[0];
      int offset = (int)TargetIntValue(var->operand[0]);
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      if (offset >= 256) {
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s__%s\t\t\t// %s\n", "jsr",
              opcode == W65C02_OP(var_addr_xy) ? "var_addr_xy" : "var_addrb_xy",
              ((TargetSymbol*)var)->symbol->name.value);
      break;
    }
 
      case W65C02_OP(arg_addr_xy):
      case W65C02_OP(arg_addrb_xy): {
        // ldx #offset lo
        // ldy #offset hi (removed for single byte case)
        // JSR __arg_addr[b]_xy (or arg_addr[b]_xy)
        TargetInstruction* var = inst->operand[0];
        int offset = (int)TargetIntValue(var->operand[0]);
        fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
        if (offset >= 256) {
          fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
        }
        fprintf(fp, "\t%-12s __%s\t\t// %s\n", "jsr",
                opcode == W65C02_OP(arg_addr) ? "arg_addr_xy" : "arg_addrb_xy",
                ((TargetSymbol*)var)->symbol->name.value);
        break;
      }

    case W65C02_OP(pushreg2):
    case W65C02_OP(pushreg4):
    case W65C02_OP(pushreg8): {
      if (opcode == W65C02_OP(pushreg2) &&
          IsAddressPushPair(PreviousPrintable(inst), inst)) {
        break;
      }
      TargetInstruction* r = inst->operand[0];
      W65C02Register* reg = (W65C02Register*)r->reg;
      int push_size = opcode == W65C02_OP(pushreg2)
                          ? 2
                          : opcode == W65C02_OP(pushreg4) ? 4 : 8;
      const char* reg_name =
          W65C02RegisterAsString(reg, 0, buf, sizeof(buf));
      if (RegisterSize(reg) == push_size) {
        fprintf(fp, "\tjsr         __push%s\n", reg_name + 2);
      } else {
        // A multi-byte value can be allocated to consecutive smaller
        // zero-page registers.  Push the requested value width, not merely
        // the native width implied by the first register's name.
        fprintf(fp, "\t%-12s #%s\n", "ldx", reg_name);
        fprintf(fp, "\tjsr         __pushreg%d\n", push_size);
      }
      break;
    }
      
    case W65C02_OP(structreturn): {
      W65C02Register* reg = (W65C02Register*)inst->reg;
      if (reg->type == k6502RegTypeI &&
          reg->base.num < W65C02_NUM_I_REGS) {
        fprintf(fp, "\t%-12s #0\n", "ldx");
        fprintf(fp, "\tjsr         __arg_value2_i%d\n", reg->base.num);
        break;
      }
      fprintf(fp, "\t%-12s #%s\t\t\t// struct return address\n", "lda",
              W65C02RegisterAsString(reg, 0, buf, sizeof(buf)));
      fprintf(fp, "\t%-12s #0\n", "ldx");
      fprintf(fp, "\t%-12s __%s\n", "jsr", "arg_value2");
      break;
    }
      
    case W65C02_OP(spill1):
    case W65C02_OP(spill2):
    case W65C02_OP(spill4):
    case W65C02_OP(spill8): {
      const char* spill_func;
      switch (opcode) {
          case W65C02_OP(spill1):
          spill_func = "spill1";
          break;
          case W65C02_OP(spill2):
            spill_func = "spill2";
            break;
          case W65C02_OP(spill4):
            spill_func = "spill4";
            break;
          case W65C02_OP(spill8):
          spill_func = "spill8";
          break;
        default:
          abort();
      }
      int spill_offset = SpillRegion(emitter) + (int)TargetIntValue(inst->operand[1]);
  
      // JSR spill
      // .byte reg
      // .byte spill_offset lo, spill_offset_hi
      fprintf(fp, "\t%-12s __%s\n", "jsr",
              spill_func);
      fprintf(fp, "\t.byte %s\n", W65C02RegisterAsString((W65C02Register*)inst->reg, 0, buf, sizeof(buf)));
      fprintf(fp, "\t.byte 0x%02x,0x%02x\n", spill_offset & 0xff, (spill_offset >> 8) & 0xff);
       break;
    }
    case W65C02_OP(reload1):
    case W65C02_OP(reload2):
    case W65C02_OP(reload4):
    case W65C02_OP(reload8): {
           const char* reload_func;
            switch (opcode) {
               case W65C02_OP(reload1):
               reload_func = "reload1";
               break;
               case W65C02_OP(reload2):
                 reload_func = "reload2";
                 break;
               case W65C02_OP(reload4):
                 reload_func = "reload4";
                 break;
               case W65C02_OP(reload8):
               reload_func = "reload8";
               break;
             default:
               abort();
           }
      TargetInstruction* spill = inst->operand[0];
      int spill_offset = SpillRegion(emitter) + (int)TargetIntValue(spill->operand[1]);
    
      fprintf(fp, "\t%-12s __%s\n", "jsr",
              reload_func);
      fprintf(fp, "\t.byte %s\n", W65C02RegisterAsString((W65C02Register*)inst->reg, 0, buf, sizeof(buf)));
      fprintf(fp, "\t.byte 0x%02x,0x%02x\n", spill_offset & 0xff, (spill_offset >> 8) & 0xff);
      break;
    }
      
    case W65C02_OP(var_value1):
    case W65C02_OP(var_value1b):
    case W65C02_OP(var_value2):
    case W65C02_OP(var_value2b):
    case W65C02_OP(var_value4):
    case W65C02_OP(var_value4b):
    case W65C02_OP(var_value8):
    case W65C02_OP(var_value8b):
    case W65C02_OP(arg_value1):
    case W65C02_OP(arg_value1b):
    case W65C02_OP(arg_value2):
    case W65C02_OP(arg_value2b):
    case W65C02_OP(arg_value4):
    case W65C02_OP(arg_value4b):
    case W65C02_OP(arg_value8):
    case W65C02_OP(arg_value8b): {
      // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_value[b] (or arg_addr[b])
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      switch ((W65C02Opcode)var->opcode) {
         case W65C02_OP(phi):
         case W65C02_OP(ssavar):
           var = var->operand[0];
           break;
         default:
           break;
       }
      int offset = (int)TargetIntValue(var->operand[0]);
      const char* op = W65C02OpcodeName(inst->opcode);
      //fprintf(fp, "\t%-12s #%s\t\t\t// %s\n", "lda",
     //         W65C02RegisterAsString((W65C02Register*)result->reg, 0, buf, sizeof(buf)),
      //        ((TargetSymbol*)var)->symbol->name.value);
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      if (offset >= 256) {
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s __%s%s\t\t\t// %s\n", "jsr", op,
              W65C02RegisterAsString((W65C02Register*)result->reg, 0, buf, sizeof(buf)) + 1,
              ((TargetSymbol*)var)->symbol->name.value);
      break;
    }

    case W65C02_OP(expr_addr_a): {
      TargetInstruction* expr = inst->operand[0];
      fprintf(fp, "\t%-12s #%s\n", "lda",
              W65C02RegisterAsString((W65C02Register*)expr->reg, 0, buf, sizeof(buf)));
      break;
    }
    case W65C02_OP(expr_addr_x): {
      TargetInstruction* expr = inst->operand[0];
      fprintf(fp, "\t%-12s #%s\n", "ldx",
              W65C02RegisterAsString((W65C02Register*)expr->reg, 0, buf, sizeof(buf)));
      break;
    }
    case W65C02_OP(expr_addr_y): {
      TargetInstruction* expr = inst->operand[0];
      fprintf(fp, "\t%-12s #%s\n", "ldy",
              W65C02RegisterAsString((W65C02Register*)expr->reg, 0, buf, sizeof(buf)));
      break;
    }

    case W65C02_OP(load_indirect4):
    case W65C02_OP(load_indirect8):
    case W65C02_OP(store_indirect4):
    case W65C02_OP(store_indirect8): {
      TargetInstruction* x_reg = inst->operand[0];
      TargetInstruction* y_reg = inst->operand[1];
      fprintf(fp, "\t%-12s #%s\n", "ldx",
              W65C02RegisterAsString((W65C02Register*)x_reg->reg, 0, buf,
                                     sizeof(buf)));
      fprintf(fp, "\t%-12s #%s\n", "ldy",
              W65C02RegisterAsString((W65C02Register*)y_reg->reg, 0, buf,
                                     sizeof(buf)));
      const char* helper =
          opcode == W65C02_OP(load_indirect4)
              ? "load_indirect4"
              : opcode == W65C02_OP(load_indirect8)
                    ? "load_indirect8"
                    : opcode == W65C02_OP(store_indirect4)
                          ? "store_indirect4"
                          : "store_indirect8";
      fprintf(fp, "\t%-12s __%s\n", "jsr", helper);
      break;
    }

    case W65C02_OP(asm): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      StringLiteral* lit = CompilerFindStringLiteral(literal->literal_id);
      assert(lit != NULL);

      // Output text directly into assembly output.
      fprintf(fp, "\t");
      if ((inst->flags & k6502ExtendedAsm) != 0) {
        PrintExtendedAsm(fp, (W65C02AsmInstruction*)inst, lit->value.value);
      } else {
        fprintf(fp, "%s", lit->value.value);
      }
      fprintf(fp, "\n");
      lit->base.disabled = true;
      return;
    }

    case W65C02_OP(loc): {
      int fileno, lineno, colno;
      TargetLocation* loc = (TargetLocation*)inst;
      SourceLocationNumbers(loc->location, &fileno, &lineno, &colno);
      fprintf(fp, "\t.loc %d %d %d\n", fileno + 1, lineno, colno + 1);
      return;
    }
      
    case W65C02_OP(jumptable):
      fprintf(fp, "\t.short .%s_label_%d\n", func_name, inst->operand[0]->id);
      break;
      
    default:
      fprintf(fp, "\t%-12s", W65C02OpcodeName(inst->opcode));
      PrintOperand(emitter, inst, func_name, fp);
      fprintf(fp, "\n");
      break;
  }
}

void W65C02EmitterInit(W65C02Emitter* emitter, W65C02Generator* g) {
  emitter->g = g;
  emitter->regs = &g->register_allocator;
}

W65C02Emitter* New6502Emitter(W65C02Generator* pcode) {
  W65C02Emitter* emitter = malloc(sizeof(W65C02Emitter));
  W65C02EmitterInit(emitter, pcode);
  return emitter;
}

void W65C02EmitterDestruct(W65C02Emitter* emitter) {}

void W65C02EmitterDelete(W65C02Emitter* emitter) {
  W65C02EmitterDestruct(emitter);
  free(emitter);
}

void W65C02PrintFunction(W65C02Emitter* emitter, FILE* fp) {
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

  TargetInstruction* inst = TargetFirstInstruction(&emitter->g->base);
  while (inst != NULL) {
    PrintInstruction(emitter, inst, func_name, fp);
    inst = TargetNext(inst);
  }

  fprintf(fp, ".func_end_%s:\n", func_name);
  fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", func_name, func_name,
          func_name);
}

void W65C02PrintCXXAdjustorThunks(FILE* fp) {
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
    uint16_t adjustment = (uint16_t)thunk->this_adjustment;

    fprintf(fp, "\t.weak %s\n", thunk_name);
    fprintf(fp, "\t.type %s, @function\n", thunk_name);
    fprintf(fp, "%s:\n", thunk_name);
    // The first software-stack argument is `this`. Preserve X/Y because they
    // carry the result destination through the tail call.
    fprintf(fp, "\ttxa\n");
    fprintf(fp, "\tpha\n");
    fprintf(fp, "\ttya\n");
    fprintf(fp, "\tpha\n");
    fprintf(fp, "\tclc\n");
    fprintf(fp, "\tlda (__sp)\n");
    fprintf(fp, "\tadc #0x%02x\n", adjustment & 0xff);
    fprintf(fp, "\tsta (__sp)\n");
    fprintf(fp, "\tldy #1\n");
    fprintf(fp, "\tlda (__sp), y\n");
    fprintf(fp, "\tadc #0x%02x\n", adjustment >> 8);
    fprintf(fp, "\tsta (__sp), y\n");
    fprintf(fp, "\tpla\n");
    fprintf(fp, "\ttay\n");
    fprintf(fp, "\tpla\n");
    fprintf(fp, "\ttax\n");
    fprintf(fp, "\tjmp %s\n", target_name);
    fprintf(fp, ".func_end_%s:\n", thunk_name);
    fprintf(fp, "\t.size %s, .func_end_%s-%s\n\n", thunk_name, thunk_name,
            thunk_name);
  }
}
