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
#include "6502_assembler.h"
#include "6502_reg_alloc.h"
#include "compiler.h"

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
    case _6502_OP(symbol):
    case _6502_OP(expr1):
    case _6502_OP(expr2):
    case _6502_OP(expr4):
    case _6502_OP(expr8):
    case _6502_OP(fake_bra):
    case _6502_OP(ssavar):
    case _6502_OP(phi):
      return false;
    default:
      break;
  }
  return true;
}

static int RegisterSize(_6502Register* reg) {
  switch (reg->type) {
    case k6502RegTypeI:
      return 2;
    case k6502RegTypeB:
      return 1;
    case k6502RegTypeD:
      return 8;
    case k6502RegTypeF:
    case k6502RegTypeL:
      return 4;
    case k6502RegTypeX:
      return 8;
  }
}

static void EnterSubroutine(_6502Emitter* emitter, TargetInstruction* inst,
                            FILE* fp) {}

static void ReturnFromSubroutine(_6502Emitter* emitter, TargetInstruction* inst,
                                 FILE* fp) {}

static void PrintRegister(_6502Register* reg, FILE* fp) {}

static struct {
  int offset;
  const char* name;
} zero_page_locations[] = {
    {_6502_SP_REG, "__sp"},
    {_6502_FP_REG, "__fp"},
    {_6502_T0_REG, "__t0"},
    {_6502_T1_REG, "__t1"},
    {_6502_RESULT_REG, "__result"},
    {_6502_RESULT_REG+1, "__result+1"},
    {_6502_MSRC_REG, "__mem_src"},
    {_6502_MSRC_REG + 1, "__mem_src+1"},
    {_6502_MDST_REG, "__mem_dest"},
    {_6502_MDST_REG + 1, "__mem_dest+1"},
    {_6502_MSZ_REG, "__mem_size"},
    {_6502_MSZ_REG + 1, "__mem_size+1"},
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

static void PrintOperand(_6502Emitter* emitter, TargetInstruction* inst,
                         const char* func_name, FILE* fp) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0xff);
  TargetInstruction* operand = inst->operand[0];
  if (operand == NULL) {
    if (mode == kAddrModeAccumulator) {
      fprintf(fp, " A");
    }
    return;
  }
  int offset = 0;
  _6502Register* reg = (_6502Register*)operand->reg;
  char buf[32];
  switch (mode) {
    case kAddrModeAbsolute:
      if (inst->operand[0]->opcode == (TargetOpcode)_6502_OP(symbol)) {
        fprintf(fp, "%s", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      } else if (inst->operand[0]->opcode == (TargetOpcode)_6502_OP(label)) {
        fprintf(fp, ".%s_label_%d", func_name, inst->operand[0]->id);
      } else {
        abort();
      }
      break;
    case kAddrModeAbsoluteSymbol:
      offset = (int)TargetIntValue(inst->operand[1]);
      if (offset == 0) {
        fprintf(fp, "%%lo(%s)", TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      } else {
        fprintf(fp, "%%hi(%s)",
                TargetSymbolName(((TargetSymbol*)operand)->symbol, buf, sizeof(buf)));
      }
      break;
    case kAddrModeImplied:
      break;
    case kAddrModeAccumulator:
      fprintf(fp, " A");
      break;
    case kAddrModeIndirect:
      fprintf(
          fp, "(%s)",
          _6502RegisterAsString(inst->operand[0]->reg, 0, buf, sizeof(buf)));
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
      fprintf(fp, "%s", _6502RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
    case kAddrModeZeroPageAbsolute:
      offset = (int)TargetIntValue(inst->operand[0]);
      fprintf(fp, "%s", ZeroPageLocation(offset));
      break;
    case kAddrModeImmediate: {
      int value = (int)TargetIntValue(operand);
      if (inst->operand[1] != NULL) {
        int bytenum = (int)TargetIntValue(inst->operand[1]);
        value = (value >> (bytenum * 8)) & 0xff;
      }
      fprintf(fp, " #%d", value);
      break;
    }
    case kAddrModeZeroPageImmediate:
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "#%s", _6502RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
    case kAddrModeZeroPageIndirect:
      fprintf(fp, "(%s)", _6502RegisterAsString(reg, 0, buf, sizeof(buf)));
      break;
    case kAddrModeIndirectIndexed:
      fprintf(fp, "(%s), Y", _6502RegisterAsString(reg, 0, buf, sizeof(buf)));
      break;
    case kAddrModeIndexedIndirect:
      fprintf(fp, "(%s, X)", _6502RegisterAsString(reg, 0, buf, sizeof(buf)));
      break;
    case kAddrModeAbsoluteIndexedX:
      break;
    case kAddrModeZeroPageIndexedX:
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s, X",
              _6502RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
    case kAddrModeAbsoluteIndexedY:
      break;
    case kAddrModeZeroPageIndexedY:
      offset = (int)TargetIntValue(inst->operand[1]);
      fprintf(fp, "%s, Y",
              _6502RegisterAsString(reg, offset, buf, sizeof(buf)));
      break;
    default:
      abort();
  }
}

// Main instruction printer.
static void PrintInstruction(_6502Emitter* emitter, TargetInstruction* inst,
                             const char* func_name, FILE* fp) {
  const bool trace = false;
  if (trace) {
    fprintf(fp, "// ");
    _6502PrintInstruction(inst, fp);
  }
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
  static char buf[4096];
  _6502Opcode opcode = (_6502Opcode)inst->opcode;
  switch (opcode) {
    case _6502_OP(literalreflo): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      const char* label = lit->type == kLiteralBuffer ? "lit" : "str";
      fprintf(fp, "\tlda         #%%lo(.%s.%d)\n", label,
              literal->literal_id);
      break;
    }
      
    case _6502_OP(literalrefhi): {
      TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      const char* label = lit->type == kLiteralBuffer ? "lit" : "str";
      fprintf(fp, "\tlda         #%%hi(.%s.%d)\n", label,
               literal->literal_id);
        break;
    }

 case _6502_OP(literalref): {
   TargetLiteral* literal = (TargetLiteral*)inst->operand[0];
   Literal* lit = CompilerFindLiteral(literal->literal_id);
   assert(lit != NULL);
   const char* label = lit->type == kLiteralBuffer ? "lit" : "str";
   fprintf(fp, "\tldx         #%%lo(.%s.%d)\n", label,
           literal->literal_id);
   fprintf(fp, "\tldy         #%%hi(.%s.%d)\n", label,
            literal->literal_id);
   break;
 }

    case _6502_OP(enter):
    case _6502_OP(enter_leaf): {
      int frame_size = emitter->g->base.stack_frame_size + 2;
      if (opcode == _6502_OP(enter)) {
        frame_size += 2;      // Space for return address.
      }
      fprintf(fp, "\t%-12s #%d\n", "ldx", frame_size & 0xff);
      const char* suffix = "";
      if (frame_size >= 256) {
        suffix = "+2";
        fprintf(fp, "\t%-12s #%d\n", "ldy", (frame_size >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s __%s%s\n", "jsr",
              opcode == _6502_OP(enter_leaf) ? "enter_leaf" : "enter", suffix);
      break;
    }
    case _6502_OP(leave):
    case _6502_OP(leave_leaf): {
      int frame_size = emitter->g->base.stack_frame_size + 2;
      if (opcode == _6502_OP(leave)) {
        frame_size += 2;      // Space for return address.
      }
      fprintf(fp, "\t%-12s #%d\n", "ldy", frame_size & 0xff);
      const char* suffix = "";
      if (frame_size >= 256) {
        suffix = "+2";
        fprintf(fp, "\t%-12s #%d\n", "ldx", (frame_size >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s __%s%s\n", "jsr",
              opcode == _6502_OP(leave_leaf) ? "leave_leaf" : "leave", suffix);
      break;
    }

    case _6502_OP(var_addr):
    case _6502_OP(var_addrb): {
      // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr[b] (or arg_addr[b])
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      int offset = (int)TargetIntValue(var->operand[0]);
      fprintf(fp, "\t%-12s #%s\t\t\t// %s\n", "lda",
              _6502RegisterAsString(result->reg, 0, buf, sizeof(buf)),
              ((TargetSymbol*)var)->symbol->name.value);
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      if (offset >= 256) {
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s __%s\n", "jsr",
              opcode == _6502_OP(var_addr) ? "var_addr" : "var_addrb");
      break;
    }

    case _6502_OP(arg_addr):
    case _6502_OP(arg_addrb): {
      // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __arg_addr[b] (or arg_addr[b])
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      int offset = (int)TargetIntValue(var->operand[0]);
      fprintf(fp, "\t%-12s #%s\t\t\t// %s\n", "lda",
              _6502RegisterAsString(result->reg, 0, buf, sizeof(buf)),
              ((TargetSymbol*)var)->symbol->name.value);
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      if (offset >= 256) {
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s __%s\n", "jsr",
              opcode == _6502_OP(arg_addr) ? "arg_addr" : "arg_addrb");
      break;
    }

    case _6502_OP(var_addr_xy):
    case _6502_OP(var_addrb_xy): {
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
              opcode == _6502_OP(var_addr) ? "var_addr_xy" : "var_addrb_xy",
              ((TargetSymbol*)var)->symbol->name.value);
      break;
    }
 
      case _6502_OP(arg_addr_xy):
      case _6502_OP(arg_addrb_xy): {
        // ldx #offset lo
        // ldy #offset hi (removed for single byte case)
        // JSR __arg_addr[b]_xy (or arg_addr[b]_xy)
        TargetInstruction* var = inst->operand[0];
        int offset = (int)TargetIntValue(var->operand[0]);
        fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
        if (offset >= 256) {
          fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
        }
        fprintf(fp, "\t%-12s __%s\t\t\// %s\n", "jsr",
                opcode == _6502_OP(arg_addr) ? "arg_addr_xy" : "arg_addrb_xy",
                ((TargetSymbol*)var)->symbol->name.value);
        break;
      }

    case _6502_OP(spill1):
    case _6502_OP(spill2):
    case _6502_OP(spill4):
    case _6502_OP(spill8):
    case _6502_OP(unspill1):
    case _6502_OP(unspill2):
    case _6502_OP(unspill4):
    case _6502_OP(unspill8):
    case _6502_OP(var_value1):
    case _6502_OP(var_value1b):
    case _6502_OP(var_value2):
    case _6502_OP(var_value2b):
    case _6502_OP(var_value4):
    case _6502_OP(var_value4b):
    case _6502_OP(var_value8):
    case _6502_OP(var_value8b):
    case _6502_OP(arg_value1):
    case _6502_OP(arg_value1b):
    case _6502_OP(arg_value2):
    case _6502_OP(arg_value2b):
    case _6502_OP(arg_value4):
    case _6502_OP(arg_value4b):
    case _6502_OP(arg_value8):
    case _6502_OP(arg_value8b): {
      // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_value[b] (or arg_addr[b])
      TargetInstruction* result = inst->operand[0];
      TargetInstruction* var = inst->operand[1];
      int offset = (int)TargetIntValue(var->operand[0]);
      const char* op = _6502OpcodeName(inst->opcode);
      fprintf(fp, "\t%-12s #%s\t\t\t// %s\n", "lda",
              _6502RegisterAsString(result->reg, 0, buf, sizeof(buf)),
              ((TargetSymbol*)var)->symbol->name.value);
      fprintf(fp, "\t%-12s #%d\n", "ldx", offset & 0xff);
      if (offset >= 256) {
        fprintf(fp, "\t%-12s #%d\n", "ldy", (offset >> 8) & 0xff);
      }
      fprintf(fp, "\t%-12s __%s\n", "jsr", op);
      break;
    }

    case _6502_OP(expr_addr_a): {
      TargetInstruction* expr = inst->operand[0];
      fprintf(fp, "\t%-12s #%s\n", "lda",
              _6502RegisterAsString(expr->reg, 0, buf, sizeof(buf)));
      break;
    }
    case _6502_OP(expr_addr_x): {
      TargetInstruction* expr = inst->operand[0];
      fprintf(fp, "\t%-12s #%s\n", "ldx",
              _6502RegisterAsString(expr->reg, 0, buf, sizeof(buf)));
      break;
    }
    case _6502_OP(expr_addr_y): {
      TargetInstruction* expr = inst->operand[0];
      fprintf(fp, "\t%-12s #%s\n", "ldy",
              _6502RegisterAsString(expr->reg, 0, buf, sizeof(buf)));
      break;
    }

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
      
    case _6502_OP(jumptable):
      fprintf(fp, "\t.short .%s_label_%d\n", func_name, inst->operand[0]->id);
      break;
      
    default:
      fprintf(fp, "\t%-12s", _6502OpcodeName(inst->opcode));
      PrintOperand(emitter, inst, func_name, fp);
      fprintf(fp, "\n");
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
