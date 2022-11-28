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
    case kAddrModeAbsoluteSymbol:
    case kAddrModeAbsoluteSymbolIndexedX:
    case kAddrModeAbsoluteSymbolIndexedY:
    case kAddrModeLiteralIndexedX:
      return 3;
    case kAddrModeAccumulator:
      return 1;
    case kAddrModeZeroPageIndexedY:
      return 3;         // Really abs,Y.  There is no zero-page,Y
      
    default:
      return 2;
  }
}

static int FrameSize(W65C02Generator* g, bool is_leaf) {
  return g->base.stack_frame_size +
      g->register_allocator.max_spilled_region_size + 2 +
      (is_leaf ? 0 : 2);
}

// How many bytes in the instruction?
// Based on opcode and addressing mode:
static int BytesInInstruction(W65C02Generator* g, TargetInstruction* inst) {
  AddressingMode addr_mode = (AddressingMode)((inst->flags >> 16) & 0xff);

  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
    case W65C02_OP(fake_bra):
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
      return 0;

    case W65C02_OP(expr_addr_a):
    case W65C02_OP(expr_addr_x):
    case W65C02_OP(expr_addr_y):
      return 2;

    case W65C02_OP(localvar):
    case W65C02_OP(argument):
    case W65C02_OP(ssavar):
    case W65C02_OP(phi):
      return 0;
    case W65C02_OP(literalreflo):
    case W65C02_OP(literalrefhi):
      return 2;
    case W65C02_OP(literalref):  // X:Y = addr of literal
    case W65C02_OP(stringliteralref):  // X:Y = addr of literal
      return 4;
    case W65C02_OP(literalrefX):  // Placeholder for literal reference.
      return 0;
      
    case W65C02_OP(enter): {
      int frame_size = FrameSize(g, true);
      if (frame_size >= 256) {
        return 10;
      }
      return 8;
    }
    case W65C02_OP(spill1):
    case W65C02_OP(spill2):
    case W65C02_OP(spill4):
    case W65C02_OP(spill8):
    case W65C02_OP(reload1):
    case W65C02_OP(reload2):
    case W65C02_OP(reload4):
    case W65C02_OP(reload8):
      return 6;
      
    case W65C02_OP(enter_leaf): {
      int frame_size = FrameSize(g, true);
      if (frame_size >= 256) {
        return 10;
      }
      return 8;
    }
      
    case W65C02_OP(leave): {
      int frame_size = FrameSize(g, false);
      if (frame_size >= 256) {
        return 7;
      }
      return 5;
    }
    case W65C02_OP(leave_leaf): {
      int frame_size = FrameSize(g, true);
      if (frame_size >= 256) {
        return 7;
      }
      return 5;
    }

      case W65C02_OP(load_result): {
        int frame_size = FrameSize(g, true);
        if (frame_size >= 256) {
          return 7;
        }
        return 5;
      }

    case W65C02_OP(var_addr):
    case W65C02_OP(var_addrb):
    case W65C02_OP(arg_addr):
    case W65C02_OP(arg_addrb):
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
      if (offset >= 256) {
        return 9;
      }
      return 7;
    }

      case W65C02_OP(var_addr_xy):
      case W65C02_OP(var_addrb_xy):
      case W65C02_OP(arg_addr_xy):
      case W65C02_OP(arg_addrb_xy): {
      TargetInstruction* var = inst->operand[0];
      int offset = (int)TargetIntValue(var->operand[0]);
      if (offset >= 256) {
        return 9;
      }
      return 7;
      break;
    }
      
    case W65C02_OP(brk):
      return 1;

    case W65C02_OP(bpl):
    case W65C02_OP(bmi):
    case W65C02_OP(bvc):
    case W65C02_OP(bvs):
    case W65C02_OP(bcc):
    case W65C02_OP(bcs):
    case W65C02_OP(bne):
    case W65C02_OP(beq):
      return 2;

    case W65C02_OP(jsr):
    case W65C02_OP(jmp):
      return 3;
    case W65C02_OP(rti):
    case W65C02_OP(rts):
      return 1;

    case W65C02_OP(lda):
    case W65C02_OP(ldx):
    case W65C02_OP(ldy):
    case W65C02_OP(sta):
    case W65C02_OP(stx):
    case W65C02_OP(sty):
    case W65C02_OP(cmp):
    case W65C02_OP(cpy):
    case W65C02_OP(cpx):
    case W65C02_OP(bit):

    case W65C02_OP(ora):
    case W65C02_OP(and):
    case W65C02_OP(eor):

    case W65C02_OP(adc):
    case W65C02_OP(sbc):

    case W65C02_OP(asl):
    case W65C02_OP(rol):
    case W65C02_OP(lsr):
    case W65C02_OP(ror):

    case W65C02_OP(dec):
    case W65C02_OP(inc):
      return BytesByAddressingMode(addr_mode);

    case W65C02_OP(dey):
    case W65C02_OP(dex):
    case W65C02_OP(iny):
    case W65C02_OP(inx):

    case W65C02_OP(php):
    case W65C02_OP(clc):
    case W65C02_OP(plp):
    case W65C02_OP(sec):
    case W65C02_OP(pha):
    case W65C02_OP(cli):
    case W65C02_OP(pla):
    case W65C02_OP(sei):
    case W65C02_OP(tay):
    case W65C02_OP(clv):
    case W65C02_OP(cld):
    case W65C02_OP(sed):

    case W65C02_OP(tya):
    case W65C02_OP(txa):
    case W65C02_OP(txs):
    case W65C02_OP(tax):
    case W65C02_OP(tsx):

    case W65C02_OP(nop):
      return 1;

    // 65C02
    case W65C02_OP(tsb):
    case W65C02_OP(trb):
      return 1;
    case W65C02_OP(stz):
      return BytesByAddressingMode(addr_mode);

    case W65C02_OP(phy):
    case W65C02_OP(ply):
    case W65C02_OP(phx):
    case W65C02_OP(plx):
      return 1;
    case W65C02_OP(bra):
      return 2;
      break;
    case W65C02_OP(symbol):   // Static symbol.
    case W65C02_OP(literal):  // String literal.
    case W65C02_OP(tmp):

    case W65C02_OP(const8):
    case W65C02_OP(const16):
    case W65C02_OP(const32):
    case W65C02_OP(const64):
    case W65C02_OP(constf):
    case W65C02_OP(constd):

    case W65C02_OP(mov):
    case W65C02_OP(movf):
    case W65C02_OP(movd):

    case W65C02_OP(movc):
    case W65C02_OP(movfc):
    case W65C02_OP(movdc):
    case W65C02_OP(movxc):

    case W65C02_OP(rmov):
    case W65C02_OP(rmovf):
    case W65C02_OP(rmovd):

    case W65C02_OP(ret):

    case W65C02_OP(label):

    case W65C02_OP(fp):  // Frame pointer pseudo operation.
    case W65C02_OP(sp):  // Stack pointer pseudo operation.
    case W65C02_OP(tp):  // Thread pointer.

    // Function result registers.
    case W65C02_OP(resulti):
    case W65C02_OP(resultf):
    case W65C02_OP(resultd):

    case W65C02_OP(structreturn):  // Struct return address.

    case W65C02_OP(asm):

    case W65C02_OP(loc):
    case W65C02_OP(named_label):
      return 0;

    case W65C02_OP(jumptable):
      return 2;
    
    case W65C02_OP(reloadpoint):
      return 0;
     
    case W65C02_OP(pushreg2):
    case W65C02_OP(pushreg4):
    case W65C02_OP(pushreg8):
      return 3;
      
    default:
      abort();
  }
}

void W65C02CalculateInstructionAddresses(W65C02Generator* g) {
  int addr = 0;
  TargetInstruction* inst = TargetFirstInstruction(&g->base);
  while (inst != NULL) {
    inst->addr = addr;
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

static bool ConvertBranch(W65C02Generator* g, TargetInstruction* bra,
                          TargetInstruction* target) {
  if (bra->opcode == (TargetOpcode)W65C02_OP(jmp)) {
    // Already converted.
    return false;
  }
  if (bra->opcode == (TargetOpcode)W65C02_OP(bra)) {
    // printf("Converted unconditional branch @%d to jmp\n", bra->id);
    bra->opcode = (TargetOpcode)W65C02_OP(jmp);
    return true;
  }
  TargetInstruction* label =
      TargetNewInstruction((TargetOpcode)W65C02_OP(label));
  label->flags |= (int)kAddrModeImplied << 16;

  W65C02Opcode new_op;
  switch ((W65C02Opcode)bra->opcode) {
    case W65C02_OP(bpl):
      new_op = W65C02_OP(bmi);
      break;
    case W65C02_OP(bmi):
      new_op = W65C02_OP(bpl);
      break;
    case W65C02_OP(bvc):
      new_op = W65C02_OP(bcs);
      break;
    case W65C02_OP(bvs):
      new_op = W65C02_OP(bvc);
      break;
    case W65C02_OP(bcc):
      new_op = W65C02_OP(bcs);
      break;
    case W65C02_OP(bcs):
      new_op = W65C02_OP(bcc);
      break;
    case W65C02_OP(bne):
      new_op = W65C02_OP(beq);
      break;
    case W65C02_OP(beq):
      new_op = W65C02_OP(bne);
      break;
    default:
      abort();
  }
  bra->opcode = (TargetOpcode)new_op;
  TargetReplaceOperand(bra, 0, label);
  TargetInstruction* jmp =
      TargetNewInstruction1((TargetOpcode)W65C02_OP(jmp), target);
  jmp->flags |= (int)kAddrModeAbsolute << 16;
  TargetEmitAfter(&g->base, jmp, bra);
  TargetEmitAfter(&g->base, label, jmp);
  return true;
}

static bool CheckBranchRanges(W65C02Generator* g) {
  bool changed = false;
  for (size_t i = 0; i < g->branches.length; i++) {
    TargetInstruction* bra = g->branches.value.p[i];
    TargetInstruction* target = bra->operand[0];
    assert(target != NULL);
    if (bra->opcode == W65C02_OP(jumptable)) {
      continue;
    }
    int diff = target->addr - (bra->addr + 2);
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

void W65C02ProcessBranches(W65C02Generator* g) {
  bool changed;
  do {
    W65C02CalculateInstructionAddresses(g);
    changed = CheckBranchRanges(g);
  } while (changed);
#if 0
  // Debug print all branch addresses.
  for (size_t i = 0; i < g->branches.length; i++) {
    TargetInstruction* bra = g->branches.value.p[i];
    printf("branch at 0x%x\n", bra->addr);
  }
#endif
}
