//
//  6502_branches.c
//  c_compiler
//
//  Created by David Allison on 3/29/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "6502_branches.h"
#include <assert.h>

static int BytesByAddressingMode(AddressingMode addr_mode) {
  switch (addr_mode) {
    case kAddrModeAbsolute:
      return 3;
    case kAddrModeAccumulator:
      return 1;
    default:
      return 2;
  }
}

// How many bytes in the instruction?
// Based on opcode and addressing mode:
static int BytesInInstruction(_6502Generator* g, TargetInstruction* inst) {
  AddressingMode addr_mode = (AddressingMode)((inst->flags >> 16) & 0xff);

  switch ((_6502Opcode)inst->opcode) {
    case _6502_OP(expr1):
    case _6502_OP(expr2):
    case _6502_OP(expr4):
    case _6502_OP(expr8):
    case _6502_OP(fake_bra):
      return 0;

    case _6502_OP(expr_addr_a):
    case _6502_OP(expr_addr_x):
    case _6502_OP(expr_addr_y):
      return 2;

    case _6502_OP(localvar):
    case _6502_OP(argument):
    case _6502_OP(ssavar):
    case _6502_OP(phi):
      return 0;
    case _6502_OP(literalreflo):
    case _6502_OP(literalrefhi):
      return 2;
    case _6502_OP(literalref):  // X:Y = addr of literal
      return 4;
      
    case _6502_OP(enter):
    case _6502_OP(enter_leaf):
    case _6502_OP(leave):
    case _6502_OP(leave_leaf): {
      int frame_size = g->base.stack_frame_size + 2;
      if (frame_size >= 256) {
        return 7;
      }
      return 5;
    }

    case _6502_OP(var_addr):
    case _6502_OP(var_addrb):
    case _6502_OP(arg_addr):
    case _6502_OP(arg_addrb):
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
      TargetInstruction* var = inst->operand[1];
      int offset = (int)TargetIntValue(var->operand[0]);
      if (offset >= 256) {
        return 9;
      }
      return 7;
    }

      case _6502_OP(var_addr_xy):
      case _6502_OP(var_addrb_xy):
      case _6502_OP(arg_addr_xy):
      case _6502_OP(arg_addrb_xy): {
      TargetInstruction* var = inst->operand[0];
      int offset = (int)TargetIntValue(var->operand[0]);
      if (offset >= 256) {
        return 9;
      }
      return 7;
      break;
    }
      
    case _6502_OP(brk):
      return 1;

    case _6502_OP(bpl):
    case _6502_OP(bmi):
    case _6502_OP(bvc):
    case _6502_OP(bvs):
    case _6502_OP(bcc):
    case _6502_OP(bcs):
    case _6502_OP(bne):
    case _6502_OP(beq):
      return 2;

    case _6502_OP(jsr):
    case _6502_OP(jmp):
      return 3;
    case _6502_OP(rti):
    case _6502_OP(rts):
      return 1;

    case _6502_OP(lda):
    case _6502_OP(ldx):
    case _6502_OP(ldy):
    case _6502_OP(sta):
    case _6502_OP(stx):
    case _6502_OP(sty):
    case _6502_OP(cmp):
    case _6502_OP(cpy):
    case _6502_OP(cpx):
    case _6502_OP(bit):

    case _6502_OP(ora):
    case _6502_OP(and):
    case _6502_OP(eor):

    case _6502_OP(adc):
    case _6502_OP(sbc):

    case _6502_OP(asl):
    case _6502_OP(rol):
    case _6502_OP(lsr):
    case _6502_OP(ror):

    case _6502_OP(dec):
    case _6502_OP(inc):
      return BytesByAddressingMode(addr_mode);

    case _6502_OP(dey):
    case _6502_OP(dex):
    case _6502_OP(iny):
    case _6502_OP(inx):

    case _6502_OP(php):
    case _6502_OP(clc):
    case _6502_OP(plp):
    case _6502_OP(sec):
    case _6502_OP(pha):
    case _6502_OP(cli):
    case _6502_OP(pla):
    case _6502_OP(sei):
    case _6502_OP(tay):
    case _6502_OP(clv):
    case _6502_OP(cld):
    case _6502_OP(sed):

    case _6502_OP(tya):
    case _6502_OP(txa):
    case _6502_OP(txs):
    case _6502_OP(tax):
    case _6502_OP(tsx):

    case _6502_OP(nop):
      return 1;

    // 65C02
    case _6502_OP(tsb):
    case _6502_OP(trb):
      return 1;
    case _6502_OP(stz):
      return BytesByAddressingMode(addr_mode);

    case _6502_OP(phy):
    case _6502_OP(ply):
    case _6502_OP(phx):
    case _6502_OP(plx):
      return 1;
    case _6502_OP(bra):
      return 2;
      break;
    case _6502_OP(symbol):   // Static symbol.
    case _6502_OP(literal):  // String literal.
    case _6502_OP(tmp):

    case _6502_OP(constb):
    case _6502_OP(consth):
    case _6502_OP(constw):
    case _6502_OP(constx):
    case _6502_OP(constf):
    case _6502_OP(constd):

    case _6502_OP(mov):
    case _6502_OP(movf):
    case _6502_OP(movd):

    case _6502_OP(movc):
    case _6502_OP(movfc):
    case _6502_OP(movdc):
    case _6502_OP(movxc):

    case _6502_OP(rmov):
    case _6502_OP(rmovf):
    case _6502_OP(rmovd):

    case _6502_OP(ret):

    case _6502_OP(label):

    case _6502_OP(fp):  // Frame pointer pseudo operation.
    case _6502_OP(sp):  // Stack pointer pseudo operation.
    case _6502_OP(tp):  // Thread pointer.

    // Function result registers.
    case _6502_OP(resultx):
    case _6502_OP(resultf):
    case _6502_OP(resultd):

    case _6502_OP(structreturn):  // Struct return address.

    case _6502_OP(asm):

    case _6502_OP(loc):
    case _6502_OP(named_label):
    case _6502_OP(ivarreg):
    case _6502_OP(fvarreg):
      return 0;

    case _6502_OP(jumptable):
      return 2;
      
    default:
      abort();
  }
}

void _6502CalculateInstructionAddresses(_6502Generator* g) {
  int addr = 0;
  TargetInstruction* inst = TargetFirstInstruction(&g->base);
  while (inst != NULL) {
    inst->uses = addr;
    // printf("instruction @%d is at address %d\n", inst->id, addr);
    addr += BytesInInstruction(g, inst);
    inst = TargetNext(inst);
  }
}

// Convert:
// bne label
//
// into:
// beq xx
// jmp label
// xx:
//
// Except:
// bra label -> jmp label

static bool ConvertBranch(_6502Generator* g, TargetInstruction* bra,
                          TargetInstruction* target) {
  if (bra->opcode == (TargetOpcode)_6502_OP(jmp)) {
    // Already converted.
    return false;
  }
  if (bra->opcode == (TargetOpcode)_6502_OP(bra)) {
    // printf("Converted unconditional branch @%d to jmp\n", bra->id);
    bra->opcode = (TargetOpcode)_6502_OP(jmp);
    return true;
  }
  TargetInstruction* label =
      TargetNewInstruction((TargetOpcode)_6502_OP(label));
  label->flags |= (int)kAddrModeImplied << 16;

  _6502Opcode new_op;
  switch ((_6502Opcode)bra->opcode) {
    case _6502_OP(bpl):
      new_op = _6502_OP(bmi);
      break;
    case _6502_OP(bmi):
      new_op = _6502_OP(bpl);
      break;
    case _6502_OP(bvc):
      new_op = _6502_OP(bcs);
      break;
    case _6502_OP(bvs):
      new_op = _6502_OP(bvc);
      break;
    case _6502_OP(bcc):
      new_op = _6502_OP(bcs);
      break;
    case _6502_OP(bcs):
      new_op = _6502_OP(bcc);
      break;
    case _6502_OP(bne):
      new_op = _6502_OP(beq);
      break;
    case _6502_OP(beq):
      new_op = _6502_OP(bne);
      break;
    default:
      abort();
  }
  bra->opcode = (TargetOpcode)new_op;
  TargetReplaceOperand(bra, 0, label);
  TargetInstruction* jmp =
      TargetNewInstruction1((TargetOpcode)_6502_OP(jmp), target);
  jmp->flags |= (int)kAddrModeAbsolute << 16;
  TargetEmitAfter(&g->base, jmp, bra);
  TargetEmitAfter(&g->base, label, jmp);
  return true;
}

static bool CheckBranchRanges(_6502Generator* g) {
  bool changed = false;
  for (size_t i = 0; i < g->branches.length; i++) {
    TargetInstruction* bra = g->branches.value.p[i];
    TargetInstruction* target = bra->operand[0];
    assert(target != NULL);

    int diff = target->uses - (bra->uses + 2);
    if (diff < 0) {
      // Back branch.
      if (diff < -128) {
        changed |= ConvertBranch(g, bra, target);
      }
    } else {
      if (diff > 127) {
        changed |= ConvertBranch(g, bra, target);
      }
    }
  }
  return changed;
}

void _6502ProcessBranches(_6502Generator* g) {
  bool changed;
  do {
    _6502CalculateInstructionAddresses(g);
    changed = CheckBranchRanges(g);
  } while (changed);
}
