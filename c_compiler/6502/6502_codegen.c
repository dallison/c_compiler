//
//  6502_codegen.c
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_codegen.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "6502_branches.h"
#include "6502_spiller.h"
#include "6502_optimize.h"
#include "compiler.h"
#include "member_pointer.h"
#include "target_basic_block.h"
#include "6502_target.h"
#include "map.h"

static void LowerIRNode(W65C02Generator* g, IRNode* node);
static void LowerVariables(W65C02Generator* g);
static void LowerStructReturn(W65C02Generator* g, IRNode* node);
static void LowerLiteralReference(W65C02Generator* g,
                                  IRNode* node, bool force);
#define PRINT_PRELOWER 0

bool Is65c02(void) {
  return (compiler->target_flags & k65c02Target) != 0;
}

const char* W65C02OpcodeName(int op) {
  W65C02Opcode opcode = op;
  switch (opcode) {
    default:
      // Use the generic TargetOpcodeName for all non-6502 specific
      // opcodes.
      return TargetOpcodeName((TargetOpcode)opcode);

    case W65C02_OP(expr1):
      return "expr1";
    case W65C02_OP(expr2):
      return "expr2";
    case W65C02_OP(expr4):
      return "expr4";
    case W65C02_OP(expr8):
      return "expr8";
    case W65C02_OP(exprf):
      return "exprf";
    case W65C02_OP(exprd):
      return "exprd";
    case W65C02_OP(load_result):
      return "load_result";
    case W65C02_OP(load_result_value1):
      return "load_result_value1";
    case W65C02_OP(load_result_value2):
      return "load_result_value2";
    case W65C02_OP(load_indirect4):
      return "load_indirect4";
    case W65C02_OP(load_indirect8):
      return "load_indirect8";
    case W65C02_OP(store_indirect4):
      return "store_indirect4";
    case W65C02_OP(store_indirect8):
      return "store_indirect8";
    case W65C02_OP(expr_addr_a):
      return "expr_addr_a";
    case W65C02_OP(expr_addr_x):
      return "expr_addr_x";
    case W65C02_OP(expr_addr_y):
      return "expr_addr_y";
    case W65C02_OP(enter):
      return "enter";
    case W65C02_OP(leave):
      return "leave";
    case W65C02_OP(localvar):
      return "localvar";
    case W65C02_OP(argument):
      return "argument";
    case W65C02_OP(literalreflo):
      return "literalreflo";
    case W65C02_OP(literalrefhi):
      return "literalrefhi";
      case W65C02_OP(literalref):
        return "literalref";
    case W65C02_OP(stringliteralref):
      return "stringliteralref";
  case W65C02_OP(literalrefX):
      return "literalrefX";

    case W65C02_OP(enter_leaf):
      return "enter_leaf";
    case W65C02_OP(leave_leaf):
      return "leave_leaf";

    case W65C02_OP(var_addr):
      return "var_addr";
    case W65C02_OP(var_addrb):
      return "var_addrb";

    case W65C02_OP(arg_addr):
      return "arg_addr";
    case W65C02_OP(arg_addrb):
      return "arg_addrb";

      case W65C02_OP(var_addr_xy):
         return "var_addr_xy";
      case W65C02_OP(var_addrb_xy):
         return "var_addrb_xy";

      case W65C02_OP(arg_addr_xy):
         return "arg_addr_xy";
      case W65C02_OP(arg_addrb_xy):
         return "arg_addrb_xy";

    case W65C02_OP(var_value1):
      return "var_value1";
    case W65C02_OP(var_value1b):
      return "var_value1b";
    case W65C02_OP(var_value2):
      return "var_value2";
    case W65C02_OP(var_value2b):
      return "var_value2b";
    case W65C02_OP(var_value4):
      return "var_value4";
    case W65C02_OP(var_value4b):
      return "var_value4b";
    case W65C02_OP(var_value8):
      return "var_value8";
    case W65C02_OP(var_value8b):
      return "var_value8b";
    case W65C02_OP(arg_value1):
      return "arg_value1";
    case W65C02_OP(arg_value1b):
      return "arg_value1b";
    case W65C02_OP(arg_value2):
      return "arg_value2";
    case W65C02_OP(arg_value2b):
      return "arg_value2b";
    case W65C02_OP(arg_value4):
      return "arg_value4";
    case W65C02_OP(arg_value4b):
      return "arg_value4b";
    case W65C02_OP(arg_value8):
      return "arg_value8";
    case W65C02_OP(arg_value8b):
      return "arg_value8b";
    
    case W65C02_OP(pushreg2):
      return "pushreg2";
    case W65C02_OP(pushreg4):
      return "pushreg4";
    case W65C02_OP(pushreg8):
      return "pushreg8";

    case W65C02_OP(fake_bra):
      return "fake_bra";

      case W65C02_OP(spill1):
         return "spill1";
      case W65C02_OP(spill2):
          return "spill2";
      case W65C02_OP(spill4):
          return "spill4";
      case W65C02_OP(spill8):
          return "spill8";
      case W65C02_OP(reload1):
           return "reload1";
 case W65C02_OP(reload2):
      return "reload2";
      case W65C02_OP(reload4):
           return "reload4";
      case W65C02_OP(reload8):
           return "reload8";
    case W65C02_OP(reloadpoint):
      return "reloadpoint";

    case W65C02_OP(brk):
      return "brk";

    case W65C02_OP(bpl):
      return "bpl";
    case W65C02_OP(bmi):
      return "bmi";
    case W65C02_OP(bvc):
      return "bvc";
    case W65C02_OP(bvs):
      return "bvs";
    case W65C02_OP(bcc):
      return "bcc";
    case W65C02_OP(bcs):
      return "bcs";
    case W65C02_OP(bne):
      return "bne";
    case W65C02_OP(beq):
      return "beq";

    case W65C02_OP(jsr):
      return "jsr";
    case W65C02_OP(jmp):
      return "jmp";

    case W65C02_OP(rti):
      return "rti";
    case W65C02_OP(rts):
      return "rts";

    case W65C02_OP(lda):
      return "lda";
    case W65C02_OP(ldx):
      return "ldx";
    case W65C02_OP(ldy):
      return "ldy";
    case W65C02_OP(sta):
      return "sta";
    case W65C02_OP(stx):
      return "stx";
    case W65C02_OP(sty):
      return "sty";

    case W65C02_OP(cmp):
      return "cmp";
    case W65C02_OP(cpy):
      return "cpy";
    case W65C02_OP(cpx):
      return "cpx";
    case W65C02_OP(bit):
      return "bit";

    case W65C02_OP(ora):
      return "ora";
    case W65C02_OP(and):
      return "and";
    case W65C02_OP(eor):
      return "eor";

    case W65C02_OP(adc):
      return "adc";
    case W65C02_OP(sbc):
      return "sbc";

    case W65C02_OP(asl):
      return "asl";
    case W65C02_OP(rol):
      return "rol";
    case W65C02_OP(lsr):
      return "lsr";
    case W65C02_OP(ror):
      return "ror";

    case W65C02_OP(dec):
      return "dec";
    case W65C02_OP(inc):
      return "inc";
    case W65C02_OP(dey):
      return "dey";
    case W65C02_OP(dex):
      return "dex";
    case W65C02_OP(iny):
      return "iny";
    case W65C02_OP(inx):
      return "inx";

    case W65C02_OP(php):
      return "php";
    case W65C02_OP(clc):
      return "clc";
    case W65C02_OP(plp):
      return "plp";
    case W65C02_OP(sec):
      return "sec";
    case W65C02_OP(pha):
      return "pha";
    case W65C02_OP(cli):
      return "cli";
    case W65C02_OP(pla):
      return "pla";
    case W65C02_OP(sei):
      return "sei";
    case W65C02_OP(tay):
      return "tay";
    case W65C02_OP(clv):
      return "clv";
    case W65C02_OP(cld):
      return "cld";
    case W65C02_OP(sed):
      return "sed";

    case W65C02_OP(tya):
      return "tya";
    case W65C02_OP(txa):
      return "txa";
    case W65C02_OP(txs):
      return "txs";
    case W65C02_OP(tax):
      return "tax";
    case W65C02_OP(tsx):
      return "tsx";

    case W65C02_OP(nop):
      return "nop";

    // 65C02
    case W65C02_OP(tsb):
      return "tsb";
    case W65C02_OP(trb):
      return "trb";
    case W65C02_OP(stz):
      return "stz";
    case W65C02_OP(phy):
      return "phy";
    case W65C02_OP(ply):
      return "ply";
    case W65C02_OP(phx):
      return "phx";
    case W65C02_OP(plx):
      return "plz";
    case W65C02_OP(bra):
      return "bra";
    case W65C02_OP(ap):
      return "ap";
    case W65C02_OP(ssavar):
      return "ssavar";
    case W65C02_OP(phi):
      return "phi";
    case W65C02_OP(jumptable):
      return "jumptable";
    case W65C02_OP(ivarreg):
      return "ivarreg";
    case W65C02_OP(bvarreg):
      return "bvarreg";
    case W65C02_OP(lvarreg):
      return "lvarreg";
    case W65C02_OP(xvarreg):
      return "xvarreg";
    case W65C02_OP(fvarreg):
      return "fvarreg";
    case W65C02_OP(dvarreg):
      return "dvarreg";
  }
}

static COMPILER_UNUSED TargetInstruction* ArgumentPointer(W65C02Generator* g) {
  if (g->argument_pointer == NULL) {
    g->argument_pointer =
        TargetEmit(&g->base, TargetNewInstruction((TargetOpcode)W65C02_OP(ap)));
  }
  return g->argument_pointer;
}

bool W65C02IsBranch(TargetInstruction* inst) {
  return (inst->flags & k6502BlockEnd) != 0;
}

bool W65C02IsCall(TargetInstruction* inst) {
  return (inst->flags & k6502InstIsCall) != 0;
}

bool W65C02IsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W65C02_OP(rts);
}

bool W65C02IsSpill(TargetInstruction* inst) { return false; }

bool W65C02IsLabel(TargetInstruction* inst) {
  return (inst->flags & k6502BlockStart) != 0;
}

bool W65C02IsFloatingPoint(TargetInstruction* inst) { return false; }
bool W65C02IsConditionalBranch(TargetInstruction* inst) {
  return (inst->flags & k6502InstIsCondBranch) != 0;
}

bool W65C02IsFixedRegister(TargetInstruction* inst) {
  return false;
}

bool W65C02IsConst(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(const8):
    case W65C02_OP(const16):
    case W65C02_OP(const32):
    case W65C02_OP(const64):
    case W65C02_OP(constf):
    case W65C02_OP(constd):

      return true;
    default:
      return false;
  }
}

bool W65C02IsSymbol(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(symbol):
      return true;
    default:
      return false;
  }
}

bool W65C02IsJumpTableEntry(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)W65C02_OP(jumptable);
}

static TargetInstruction* W65C02GetBranchTarget(TargetInstruction* inst) {
  return inst->operand[0];
}

static TargetVirtuals virtuals = {
    .opcode_name = W65C02OpcodeName,
    .is_branch = W65C02IsBranch,
    .is_call = W65C02IsCall,
    .is_return = W65C02IsReturn,
    .is_spill = W65C02IsSpill,
    .is_label = W65C02IsLabel,
    .is_floating_point = W65C02IsFloatingPoint,
    .is_conditional_branch = W65C02IsConditionalBranch,
    .is_fixed_register = W65C02IsFixedRegister,
    .is_const = W65C02IsConst,
    .is_symbol = W65C02IsSymbol,
    .is_expression = W65C02IsExpression,
    .is_table_entry = W65C02IsJumpTableEntry,
    .get_branch_target = W65C02GetBranchTarget,
};

static bool IsLeaf(W65C02Generator* g) {
  bool is_leaf = g->base.num_calls == 0 /*&& OptLevel1()*/;
  return is_leaf;
}

static Symbol* CreateRuntimeSymbol(W65C02Generator* g, const char* name) {
  TypeRecord* base = NewTypeRecord(kTypeVoid, kQualPlain);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.unknown_args = true;
  TypeRecordChain(func, base);
  Symbol* sym = NewSymbol(name, func, STO(extern));
  VectorAppend(&g->runtime_symbols, sym);
  return sym;
}

static Intrinsic* CreatePlainIntrinsic(W65C02Generator* g, const char* name, Type type) {
  TypeRecord* base = NewTypeRecord(type, kQualPlain);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.unknown_args = true;
  TypeRecordChain(func, base);
  Symbol* sym = NewSymbol(name, func, STO(extern));
  Intrinsic* intrinsic = malloc(sizeof(Intrinsic));
  intrinsic->symbol = sym;
  intrinsic->index = g->next_intrinsic_index++;
  return intrinsic;
}

static Intrinsic* CreatePointerIntrinsic(W65C02Generator* g, const char* name, Type type) {
  TypeRecord* base = NewTypeRecord(type, kQualPlain);
  TypeRecord* ptr = NewPointerTo(kQualPlain, base);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.unknown_args = true;
  TypeRecordChain(func, ptr);
  Symbol* sym = NewSymbol(name, func, STO(extern));
  Intrinsic* intrinsic = malloc(sizeof(Intrinsic));
  intrinsic->symbol = sym;
  intrinsic->index = g->next_intrinsic_index++;
  return intrinsic;
}

static IRNode* FindPooledVariable(Generator* gen, Symbol* sym) {
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    IRVariable* pooled_var = (IRVariable*)entry->pooled;
    if (pooled_var->symbol == sym) {
      return entry->pooled;
    }
  }
  abort();
}


static void InitRegVars(RegisterVariableSet* vars, int max, W65C02RegisterType type, W65C02Opcode opcode) {
  memset(&vars->vars, 0, sizeof(vars->vars));
  vars->max_vars = max;
  vars->num_vars = 0;
  vars->type = type;
  vars->opcode = opcode;
}

void W65C02GeneratorInit(W65C02Generator* g, Generator* gen) {
  TargetGeneratorInit(&g->base, gen, &virtuals);
  g->gen = gen;

  VectorInit(&g->runtime_symbols);

#define RUNTIME_SYM(name) g->name = CreateRuntimeSymbol(g, "__" #name)

  RUNTIME_SYM(enter);
  RUNTIME_SYM(leave);
  RUNTIME_SYM(enter_leaf);
  RUNTIME_SYM(leave_leaf);

  RUNTIME_SYM(var_addr);
  RUNTIME_SYM(var_addrb);
  RUNTIME_SYM(arg_addr);
  RUNTIME_SYM(arg_addrb);

  RUNTIME_SYM(var_addr_xy);
  RUNTIME_SYM(var_addrb_xy);
  RUNTIME_SYM(arg_addr_xy);
  RUNTIME_SYM(arg_addrb_xy);

  RUNTIME_SYM(var_value1);
  RUNTIME_SYM(var_value1b);
  RUNTIME_SYM(arg_value1);
  RUNTIME_SYM(arg_value1b);

  RUNTIME_SYM(var_value2);
  RUNTIME_SYM(var_value2b);
  RUNTIME_SYM(arg_value2);
  RUNTIME_SYM(arg_value2b);

  RUNTIME_SYM(var_value4);
  RUNTIME_SYM(var_value4b);
  RUNTIME_SYM(arg_value4);
  RUNTIME_SYM(arg_value4b);

  RUNTIME_SYM(var_value8);
  RUNTIME_SYM(var_value8b);
  RUNTIME_SYM(arg_value8);
  RUNTIME_SYM(arg_value8b);

  RUNTIME_SYM(set_var_value1);
  RUNTIME_SYM(set_var_value1b);
  RUNTIME_SYM(set_arg_value1);
  RUNTIME_SYM(set_arg_value1b);

  RUNTIME_SYM(set_var_value2);
  RUNTIME_SYM(set_var_value2b);
  RUNTIME_SYM(set_arg_value2);
  RUNTIME_SYM(set_arg_value2b);

  RUNTIME_SYM(set_var_value4);
  RUNTIME_SYM(set_var_value4b);
  RUNTIME_SYM(set_arg_value4);
  RUNTIME_SYM(set_arg_value4b);

  RUNTIME_SYM(set_var_value8);
  RUNTIME_SYM(set_var_value8b);
  RUNTIME_SYM(set_arg_value8);
  RUNTIME_SYM(set_arg_value8b);
  
  RUNTIME_SYM(zero_var_value1);
  RUNTIME_SYM(zero_var_value1b);
  RUNTIME_SYM(zero_arg_value1);
  RUNTIME_SYM(zero_arg_value1b);

  RUNTIME_SYM(zero_var_value2);
  RUNTIME_SYM(zero_var_value2b);
  RUNTIME_SYM(zero_arg_value2);
  RUNTIME_SYM(zero_arg_value2b);

  RUNTIME_SYM(zero_var_value4);
  RUNTIME_SYM(zero_var_value4b);
  RUNTIME_SYM(zero_arg_value4);
  RUNTIME_SYM(zero_arg_value4b);

  RUNTIME_SYM(zero_var_value8);
  RUNTIME_SYM(zero_var_value8b);
  RUNTIME_SYM(zero_arg_value8);
  RUNTIME_SYM(zero_arg_value8b);

  RUNTIME_SYM(push_var1);
  RUNTIME_SYM(push_var1b);
  RUNTIME_SYM(push_var2);
  RUNTIME_SYM(push_var2b);
  RUNTIME_SYM(push_var4);
  RUNTIME_SYM(push_var4b);
  RUNTIME_SYM(push_var8);
  RUNTIME_SYM(push_var8b);
  RUNTIME_SYM(push_arg1);
  RUNTIME_SYM(push_arg1b);
  RUNTIME_SYM(push_arg2);
  RUNTIME_SYM(push_arg2b);
  RUNTIME_SYM(push_arg4);
  RUNTIME_SYM(push_arg4b);
  RUNTIME_SYM(push_arg8);
  RUNTIME_SYM(push_arg8b);
  RUNTIME_SYM(pusha);
  RUNTIME_SYM(pushxy);
  RUNTIME_SYM(pushxy0);
  RUNTIME_SYM(pushreg1);
  RUNTIME_SYM(pushreg2);
  RUNTIME_SYM(pushreg4);
  RUNTIME_SYM(pushreg8);
  RUNTIME_SYM(push4);
  RUNTIME_SYM(push8);
  RUNTIME_SYM(push4xy);
  RUNTIME_SYM(push8xy);
  RUNTIME_SYM(pulla);
  RUNTIME_SYM(pullxy);
  RUNTIME_SYM(pull4);
  RUNTIME_SYM(pull8);
  RUNTIME_SYM(incsp);
  RUNTIME_SYM(incsp2);
  RUNTIME_SYM(incsp4);
  RUNTIME_SYM(incsp6);
  RUNTIME_SYM(incsp8);
  RUNTIME_SYM(incsp10);
  RUNTIME_SYM(incsp12);
  RUNTIME_SYM(incsp14);
  RUNTIME_SYM(incsp16);
  RUNTIME_SYM(incsp0);
  RUNTIME_SYM(pushmem1);
  RUNTIME_SYM(pushmem2);
  RUNTIME_SYM(pushmem_xy1);
  RUNTIME_SYM(pushmem_xy2);
  RUNTIME_SYM(copymem1);
  RUNTIME_SYM(copymem2);
  RUNTIME_SYM(zeromem1);
  RUNTIME_SYM(zeromem2);
  RUNTIME_SYM(result1);
  RUNTIME_SYM(result2);
  RUNTIME_SYM(result4);
  RUNTIME_SYM(result8);
  RUNTIME_SYM(load_result);

  RUNTIME_SYM(inc1);
  RUNTIME_SYM(inc2);
  RUNTIME_SYM(inc21);
  RUNTIME_SYM(inc4);
  RUNTIME_SYM(inc8);
  RUNTIME_SYM(incf);
  RUNTIME_SYM(incd);

  RUNTIME_SYM(inc2b);

  RUNTIME_SYM(rinc1);
  RUNTIME_SYM(rinc2);
  RUNTIME_SYM(rinc21);
  RUNTIME_SYM(rinc4);
  RUNTIME_SYM(rinc8);
  RUNTIME_SYM(rincf);
  RUNTIME_SYM(rincd);

  RUNTIME_SYM(rinc2b);

  RUNTIME_SYM(dec1);
  RUNTIME_SYM(dec2);
  RUNTIME_SYM(dec21);
  RUNTIME_SYM(dec4);
  RUNTIME_SYM(dec8);
  RUNTIME_SYM(decf);
  RUNTIME_SYM(decd);

  RUNTIME_SYM(dec2b);

  RUNTIME_SYM(rdec1);
  RUNTIME_SYM(rdec2);
  RUNTIME_SYM(rdec21);
  RUNTIME_SYM(rdec4);
  RUNTIME_SYM(rdec8);
  RUNTIME_SYM(rdecf);
  RUNTIME_SYM(rdecd);

  RUNTIME_SYM(rdec2b);

  RUNTIME_SYM(umul1);
  RUNTIME_SYM(umul2);
  RUNTIME_SYM(umul4);
  RUNTIME_SYM(umul8);
  RUNTIME_SYM(smul1);
  RUNTIME_SYM(smul2);
  RUNTIME_SYM(smul4);
  RUNTIME_SYM(smul8);
  RUNTIME_SYM(fmul);

  RUNTIME_SYM(umul2_10);
  RUNTIME_SYM(smul2_10);

  RUNTIME_SYM(sdiv1);
  RUNTIME_SYM(sdiv2);
  RUNTIME_SYM(sdiv4);
  RUNTIME_SYM(sdiv8);

  RUNTIME_SYM(udiv1);
  RUNTIME_SYM(udiv2);
  RUNTIME_SYM(udiv4);
  RUNTIME_SYM(udiv8);
  RUNTIME_SYM(fdiv);

  RUNTIME_SYM(smod1);
  RUNTIME_SYM(smod2);
  RUNTIME_SYM(smod4);
  RUNTIME_SYM(smod8);

  RUNTIME_SYM(umod1);
  RUNTIME_SYM(umod2);
  RUNTIME_SYM(umod4);
  RUNTIME_SYM(umod8);

  RUNTIME_SYM(i1tof);
  RUNTIME_SYM(i2tof);
  RUNTIME_SYM(i4tof);
  RUNTIME_SYM(i8tof);
  RUNTIME_SYM(ui1tof);
  RUNTIME_SYM(ui2tof);
  RUNTIME_SYM(ui4tof);
  RUNTIME_SYM(ui8tof);

  RUNTIME_SYM(ftoi1);
  RUNTIME_SYM(ftoi2);
  RUNTIME_SYM(ftoi4);
  RUNTIME_SYM(ftoi8);
  RUNTIME_SYM(ftoui1);
  RUNTIME_SYM(ftoui2);
  RUNTIME_SYM(ftoui4);
  RUNTIME_SYM(ftoui8);

  RUNTIME_SYM(cmpeqf);
  RUNTIME_SYM(cmpnef);
  RUNTIME_SYM(cmpltf);
  RUNTIME_SYM(cmpgef);
 
  RUNTIME_SYM(fadd);
  RUNTIME_SYM(fsub);
  RUNTIME_SYM(fneg);
  RUNTIME_SYM(zeroreg4);
  RUNTIME_SYM(zeroreg8);

  RUNTIME_SYM(jump_table1);
  RUNTIME_SYM(jump_table2);
  RUNTIME_SYM(jump_table4);
  RUNTIME_SYM(jump_table8);

  RUNTIME_SYM(decsp);
  RUNTIME_SYM(savesp);
  RUNTIME_SYM(restoresp);
  RUNTIME_SYM(builtin_va_arg2);
  RUNTIME_SYM(builtin_va_arg4);
  RUNTIME_SYM(builtin_va_arg8);
  RUNTIME_SYM(builtin_va_arg);

  MapInitForCharPointerKeys(&g->intrinsics);
  g->next_intrinsic_index = 1;
  
#define PLAIN_INTRINSIC(name, type) { \
MapKeyValue kv = {.key.p = (void*)#name, .value.p = CreatePlainIntrinsic(g, "__builtin_" #name, kType##type)}; \
  MapInsert(&g->intrinsics, kv); \
}

#define POINTER_INTRINSIC(name, type) { \
MapKeyValue kv = {.key.p = (void*)#name, .value.p = CreatePointerIntrinsic(g, "__builtin_" #name, kType##type)}; \
  MapInsert(&g->intrinsics, kv); \
}
  
  PLAIN_INTRINSIC(isalnum, Bool);
  PLAIN_INTRINSIC(isalpha, Bool);
  PLAIN_INTRINSIC(isblank, Bool);
  PLAIN_INTRINSIC(iscntrl, Bool);
  PLAIN_INTRINSIC(isdigit, Bool);
  PLAIN_INTRINSIC(isgraph, Bool);
  PLAIN_INTRINSIC(islower, Bool);
  PLAIN_INTRINSIC(isprint, Bool);
  PLAIN_INTRINSIC(ispunct, Bool);
  PLAIN_INTRINSIC(isspace, Bool);
  PLAIN_INTRINSIC(isupper, Bool);
  PLAIN_INTRINSIC(isxdigit, Bool);
  PLAIN_INTRINSIC(tolower, Int);
  PLAIN_INTRINSIC(toupper, Int);
  POINTER_INTRINSIC(memcpy, Void);
  POINTER_INTRINSIC(memset, Void);
  PLAIN_INTRINSIC(memcmp, Int);

  g->struct_return_inst = NULL;
  VectorInit(&g->branches);
  InitRegVars(&g->reg_vars[0], kNumIVars, k6502RegTypeI, W65C02_OP(ivarreg));
  InitRegVars(&g->reg_vars[1], kNumBVars, k6502RegTypeB, W65C02_OP(bvarreg));
  InitRegVars(&g->reg_vars[2], kNumLVars, k6502RegTypeL, W65C02_OP(lvarreg));
  InitRegVars(&g->reg_vars[3], kNumXVars, k6502RegTypeX, W65C02_OP(xvarreg));
  InitRegVars(&g->reg_vars[4], kNumFVars, k6502RegTypeF, W65C02_OP(fvarreg));

  W65C02RegisterAllocatorInit(&g->register_allocator, g);
}
#undef RUNTIME_SYM

W65C02Generator* New6502Generator(Generator* gen) {
  W65C02Generator* g = malloc(sizeof(W65C02Generator));
  W65C02GeneratorInit(g, gen);
  return g;
}

static void DeleteIntrinsic(MapKeyValue* kv) {
  Intrinsic* intrinsic = kv->value.p;
  SymbolDelete(intrinsic->symbol);
  free(intrinsic);
}

void W65C02GeneratorDestruct(W65C02Generator* g) {
  TargetGeneratorDestruct(&g->base);
  VectorDestruct(&g->branches);
  W65C02RegisterAllocatorDestruct(&g->register_allocator);
  // The intrinsics map owns its Intrinsic values, each of which owns a Symbol.
  // The keys are static string literals so they need no freeing.
  MapDestructWithContents(&g->intrinsics, DeleteIntrinsic);
  VectorDestructWithContents(&g->runtime_symbols,
                             (VectorElementDestructor)SymbolDelete,
                             /*free_element=*/false);
}

void W65C02GeneratorDelete(W65C02Generator* g) {
  W65C02GeneratorDestruct(g);
  free(g);
}

static AddressingMode GetAddrMode(TargetInstruction* inst) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  assert(mode > kAddrModeUnknown && mode < kAddrModeInvalid);
  return mode;
}

static void SetAddrMode(TargetInstruction* inst, AddressingMode mode) {
  inst->flags &= ~0xffff0000;        // Clear current addressing mode.
  inst->flags |= (int)mode << 16;
}

static COMPILER_UNUSED bool IsIndirectMode(TargetInstruction* inst) {
  AddressingMode mode = GetAddrMode(inst);
  return mode == kAddrModeIndirectIndexed || mode == kAddrModeIndirect;
}

static void CheckAddrMode(W65C02Opcode opcode, int addressing_mode) {
  if (!Is65c02()) {
    if (opcode != W65C02_OP(jmp)) {
      assert(addressing_mode != kAddrModeIndirect);
    }
  }
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(W65C02Opcode opcode,
                                          TargetInstruction* op1,
                                          int addressing_mode) {
  CheckAddrMode(opcode, addressing_mode);
  TargetInstruction* inst = TargetNewInstruction1((TargetOpcode)opcode, op1);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static TargetInstruction* NewInstruction2(W65C02Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          int addressing_mode) {

  CheckAddrMode(opcode, addressing_mode);
 TargetInstruction* inst =
      TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static COMPILER_UNUSED TargetInstruction* NewInstruction3(W65C02Opcode opcode,
                                                          TargetInstruction* op1,
                                                          TargetInstruction* op2,
                                                          TargetInstruction* op3,
                                                          int addressing_mode) {
  CheckAddrMode(opcode, addressing_mode);
 TargetInstruction* inst =
      TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static TargetInstruction* Emit(W65C02Generator* g, TargetInstruction* inst) {
#if 0
  W65C02PrintInstruction(inst, stdout);
#endif
  return TargetEmit(&g->base, inst);
}

static COMPILER_UNUSED TargetInstruction* EmitBefore(W65C02Generator* g,
                                                     TargetInstruction* inst,
                                                     TargetInstruction* pos) {
  return TargetEmitBefore(&g->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitAfter(W65C02Generator* g,
                                                    TargetInstruction* inst,
                                                    TargetInstruction* pos) {
  return TargetEmitAfter(&g->base, inst, pos);
}

static COMPILER_UNUSED TargetInstruction* EmitConstant(W65C02Generator* g,
                                                       TargetInstruction* c) {
  return TargetEmitConstant(&g->base, c);
}

static COMPILER_UNUSED TargetInstruction* EmitSymbol(W65C02Generator* g,
                                                     TargetInstruction* c) {
  return TargetEmitSymbol(&g->base, c);
}

static COMPILER_UNUSED TargetInstruction* FramePointer(W65C02Generator* g,
                                                       AddressingMode mode) {
  TargetInstruction* inst = TargetFramePointer(&g->base);
  SetAddrMode(inst, mode);
  return inst;
}

static COMPILER_UNUSED TargetInstruction* StackPointer(W65C02Generator* g) {
  return TargetStackPointer(&g->base);
}

static COMPILER_UNUSED TargetInstruction* ThreadPointer(W65C02Generator* g) {
  return TargetThreadPointer(&g->base);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static bool HasLoweredNode(IRNode* node) {
  return node->data.ptr != NULL;
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  return TargetSetLoweredNode(node, inst);
}

static TargetInstruction* GetIntConstant(W65C02Generator* g, IRNode* node,
                                         TargetType type, int64_t value) {
  TargetInstruction* inst = TargetGetIntConstant(&g->base, node, type, value);
  SetAddrMode(inst, kAddrModeImmediate);
  return inst;
}

static TargetInstruction* ByteConst(W65C02Generator* g, int value) {
  return GetIntConstant(g, NULL, kTargetType8Bit, value);
}

static TargetInstruction* Zero(W65C02Generator* g) {
  return GetIntConstant(g, NULL, kTargetType8Bit, 0);
}

static TargetInstruction* One(W65C02Generator* g) {
  return GetIntConstant(g, NULL, kTargetType8Bit, 1);
}


static void AddLiteral(W65C02Generator* g, TargetConstant* con, const void* data,
                       size_t length) {
  con->literal_id = CompilerAddBufferLiteral(data, length);
}

static int Sizeof(TypeRecord* type) {
  if (type == NULL) {
    return 2;
  }
  if (TypeIsPointerOrArray(type) || TypeIsFunction(type)) {
    return 2;
  }
  assert(type->size != 0);
  return type->size;
}

static int SizeofArray(TypeRecord* type) {
  if (type == NULL) {
    return 2;
  }
  if (TypeIsArray(type)) {
    return type->size;
  }
  if (TypeIsPointer(type) || TypeIsFunction(type)) {
    return 2;
  }
  assert(type->size != 0);
  return type->size;
}

static TargetInstruction* CreateVariableRegister(W65C02Generator* g,
                                                      RegisterVariableSet* set,
                                                 TargetInstruction* var,
                                                      Symbol* sym) {
  int i = set->num_vars++;
  TargetSymbol* inst = malloc(sizeof(TargetSymbol));
  TargetInitInstruction(&inst->base, (TargetOpcode)set->opcode);
  SetAddrMode(&inst->base, kAddrModeZeroPage);
  inst->symbol = sym;
  set->vars[i].reg = &inst->base;
  set->vars[i].var = var;
  return Emit(g, set->vars[i].reg);
}

// Add a spill point for the given expression as the last emitted
// instruction.
static TargetInstruction* AddSpillPoint(W65C02Generator* g, TargetInstruction* expr) {
  W65C02RegisterAllocatorAddSpillPoint(&g->register_allocator,
                                         expr->id, TargetLastInstruction(&g->base));
  return expr;
}

static bool IsExpression(TargetInstruction* inst) {
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
      return true;
    default:
      return false;
  }
}

static void AddReloadPoint(W65C02Generator* g, TargetInstruction* expr) {
  switch ((W65C02Opcode)expr->opcode) {
    case W65C02_OP(expr1):
    case W65C02_OP(expr2):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
      Emit(g, NewInstruction1(W65C02_OP(reloadpoint), expr, kAddrModeImplied));
    default:
      break;
  }
}

static COMPILER_UNUSED TargetInstruction* GetFloatingPointConstant(W65C02Generator* g,
                                                                   IRNode* node,
                                                                   TargetType type,
                                                                   double value) {
  return TargetGetFloatingPointConstant(&g->base, node, type, value);
}

static TargetInstruction* GetSymbol(W65C02Generator* g, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&g->base, node, symbol);
}

static TargetInstruction* NewInstruction(W65C02Opcode opcode,
                                         int addressing_mode) {
  TargetInstruction* inst = TargetNewInstruction((TargetOpcode)opcode);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static TargetInstruction* EmitBranch(W65C02Generator* g, W65C02Opcode op,
                                     IRNode* target_node) {
  TargetInstruction* bra = Emit(g, NewInstruction(op, kAddrModeRelative));
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(bra, target_node, 0));
  } else {
    bra->operand[0] = target_node->data.ptr;
    TargetAddUser(target_node->data.ptr, bra);
  }
  VectorAppend(&g->branches, bra);
  return bra;
}

static TargetInstruction* EmitLabelReference(W65C02Generator* g,
                                             TargetInstruction* label,
                                             IRNode* target_node) {
  Emit(g, label);
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(label, target_node, 0));
  } else {
    label->operand[0] = target_node->data.ptr;
    TargetAddUser(label, target_node->data.ptr);
  }
  return label;
}

static void EmitResolvedBranch(W65C02Generator* g, W65C02Opcode op,
                               TargetInstruction* target) {
  VectorAppend(&g->branches,
               Emit(g, NewInstruction1(op, target, kAddrModeRelative)));
}

// The 6502 isn't RISC so instructions can load and store directly
// to memory.  Returns next node or NULL.
static IRNode* PreLower(W65C02Generator* g, Generator* gen, IRNode* node, bool* changed) {
  if (IRIsLoadOnly(node)) {
    // A load can be removed if it has only one use and that use isn't
    // a store or a load.  It's input can't also be a load or a call.
    IRNode* src = node->inputs.value.p[0];
    if (src->opcode == IR_OP(adda)) {
      // If the source address is formed by adding a constant, we
      // can put the constant into the load as an offset as the
      // second arg.
      IRNode* op1 = src->inputs.value.p[0];
      IRNode* op2 = src->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t value = IRIntConstValue(op2);
        int size = Sizeof(node->type);
        // Max index for Y is 255, but we need to allow for 'size' bytes
        // above the start index.
        if (value < (256 - size)) {
          IRAddInput(node, op2, false);     // Add 2nd operand to store.
          IRReplaceInput(node, 0, op1);
          if (src->outputs.length == 0) {
            GeneratorRemoveInstruction(gen, src);
            *changed = true;
          }
        }
      }
    }
  } else if (IRIsStoreOnly(node)) {
    // A store be eliminated if its input has one use (the store instruction)
    IRNode* dest = node->inputs.value.p[0];
    IRNode* src = node->inputs.value.p[1];
    if (!IRIsConst(src) && !IRIsVariable(src) && src->opcode != IR_OP(cast)
        && !TypeIsArray(dest->type) && !TypeIsStructOrUnion(dest->type)
        && (!IRIsExpression(dest) || IRIsVariable(dest))) {
      if (node->inputs.length == 2 && src->outputs.length == 1 &&
          src->dest == NULL && BasicBlockDominatedBy(gen,
                                                    src->block,
                                                    node->block) &&
          (node->block == dest->block || IRIsVariable(dest))) {
        src->dest = dest;
        IRNode* next = IRNext(node);
        BasicBlockRemoveInstruction(gen, node->block, node);
        *changed = true;
        return next;
      }
    }
    // If the dest address of the store is an 'adda' instruction with a
    // constant operand we can add a 3rd argument to the store instruction
    // to use the Y register as an index.
    if (dest->opcode == IR_OP(adda)) {
      IRNode* op1 = dest->inputs.value.p[0];
      IRNode* op2 = dest->inputs.value.p[1];
      if (IRIsConst(op2)) {
        int64_t value = IRIntConstValue(op2);
        int size = Sizeof(node->type);
        // Max index for Y is 255, but we need to allow for 'size' bytes
        // above the start index.
        if (value >= 0 && value < (256 - size)) {
          IRAddInput(node, op2, false);     // Add 3rd operand to store.
          IRReplaceInput(node, 0, op1);
          if (dest->outputs.length == 0) {
            GeneratorRemoveInstruction(gen, dest);
            *changed = true;
          }
        }
      }
    }
//  } else if (node->opcode == IR_OP(movi) || node->opcode == IR_OP(mova) ||
//             node->opcode == IR_OP(movf) ||  node->opcode == IR_OP(movd)) {
//    // We can eliminate a mov if it has only one output.
//    if (node->outputs.length == 1) {
//      IRNode* next = IRNext(node);
//      BasicBlockRemoveInstruction(gen, node->block, node);
//      *changed = true;
//      return next;
//    }
    // This was for rmov.
//    IRNode* dest = node->inputs.value.p[0];
//    IRNode* src = node->inputs.value.p[1];
//    if (!IRIsConst(src) && !IRIsVariable(src) && src->opcode != IR_OP(cast) &&
//        src->dest == NULL &&
//        src->opcode != IR_OP(tmp)) {
//      src->dest = dest;
//      IRNode* next = IRNext(node);
//      BasicBlockRemoveInstruction(gen, node->block, node);
//      *changed = true;
//      return next;
//    }
  } else if (node->opcode == IR_OP(f2d) || node->opcode == IR_OP(d2f)) {
    // Float and double are the same thing on 6502.  Remove these instructions.
    IRNode* next = IRNext(node);
    GeneratorReplaceInstruction(gen, node, node->inputs.value.p[0]);
    BasicBlockRemoveInstruction(gen, node->block, node);
    *changed = true;
    return next;
  }
  return IRNext(node);
}

static TargetInstruction* Operate(W65C02Generator* g, W65C02Opcode op, TargetInstruction* src,
                    int index) {
  AddressingMode mode = GetAddrMode(src);
  if (Is65c02() && mode == kAddrModeIndirectIndexed && index == 0) {
    // An index of 0 can use a kAddrModeIndirect: lda (xxx)
    mode = kAddrModeIndirect;
  }
  return Emit(g, NewInstruction2(
              op, src, ByteConst(g, index), mode));
}

// Operation instructions
#define INST(op)                                                         \
  static COMPILER_UNUSED TargetInstruction* op(W65C02Generator* g, TargetInstruction* src, int index) { \
    return Operate(g, W65C02_OP(op), src, index);                                \
  }

INST(lda)
INST(ora)
INST(eor)
INST(adc)
INST(sbc)
INST(cmp)
INST(inc)
INST(dec)

#undef INST

// Immediate instructions:
// ldai(g, value);
#define INST(op)                                                             \
  static COMPILER_UNUSED void op##i(W65C02Generator* g, int value) {          \
    Emit(g, NewInstruction1(W65C02_OP(op),                                    \
                            ByteConst(g, value), \
                            kAddrModeImmediate));                            \
  }

INST(lda)
INST(ldx)
INST(ldy)
INST(ora)
INST(eor)
INST(and)
INST(adc)
INST(sbc)
INST(cmp)
INST(cpy)
INST(cpx)

#undef INST

// Zero page Immediate instructions:
// ldazi(g, value);
// These load the immediate value of a zero page location.
#define INST(op)                                                              \
  static COMPILER_UNUSED void op##zi(W65C02Generator* g, TargetInstruction* reg, int offset) { \
    Emit(g, NewInstruction2(W65C02_OP(op), reg,                                \
                            ByteConst(g, offset), \
                            kAddrModeZeroPageImmediate));                     \
  }

INST(lda)
INST(ldx)
INST(ldy)

#undef INST
// Single instructions.
#define INST(op)                                             \
  static COMPILER_UNUSED void op(W65C02Generator* g) {        \
    Emit(g, NewInstruction(W65C02_OP(op), kAddrModeImplied)); \
  }

INST(inx)
INST(iny)
INST(dex)
INST(dey)
INST(clc)
INST(sec)
INST(txa)
INST(tya)
INST(tax)
INST(tay)

#undef INST

static TargetInstruction* ldx(W65C02Generator* g, TargetInstruction* src, int index) {
  AddressingMode mode = GetAddrMode(src);
  return Emit(g,
       NewInstruction2(W65C02_OP(ldx), src,
                       ByteConst(g, index), mode));
}

static TargetInstruction* ldy(W65C02Generator* g, TargetInstruction* src, int index) {
  AddressingMode mode = GetAddrMode(src);
  return Emit(g,
       NewInstruction2(W65C02_OP(ldy), src,
                       ByteConst(g, index), mode));
}

static TargetInstruction* sta(W65C02Generator* g, TargetInstruction* dest, int index) {
  AddressingMode mode = GetAddrMode(dest);
  if (Is65c02() && mode == kAddrModeIndirectIndexed && index == 0) {
    mode = kAddrModeIndirect;
  }
  return Emit(g,
       NewInstruction2(W65C02_OP(sta), dest,
                       ByteConst(g, index), mode));
}

static TargetInstruction* stx(W65C02Generator* g, TargetInstruction* dest, int index) {
  return Emit(g, NewInstruction2(W65C02_OP(stx), dest,
                          ByteConst(g, index),
                          GetAddrMode(dest)));
}

static TargetInstruction* sty(W65C02Generator* g, TargetInstruction* dest, int index) {
  return Emit(g, NewInstruction2(W65C02_OP(sty), dest,
                          ByteConst(g, index),
                          GetAddrMode(dest)));
}

static TargetInstruction* stz(W65C02Generator* g, TargetInstruction* dest, int index) {
  return Emit(g, NewInstruction2(W65C02_OP(stz), dest,
                          ByteConst(g, index),
                          GetAddrMode(dest)));
}

static TargetInstruction* jsr(W65C02Generator* g, Symbol* func) {
  return Emit(g, NewInstruction1(W65C02_OP(jsr), GetSymbol(g, NULL, func),
                                 kAddrModeAbsolute));
}

// If either the src or dest needs an index in Y reg, load it.  If
static void SetIndexReg(W65C02Generator* g, TargetInstruction* src,
                        TargetInstruction* dest, int index) {
  if (!Is65c02()) {
    if (GetAddrMode(src) == kAddrModeIndirect ||
        GetAddrMode(dest) == kAddrModeIndirect) {
      ldyi(g, index);
      return;
    }
  }
  if (GetAddrMode(src) == kAddrModeIndirectIndexed ||
      GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    ldyi(g, index);
  }
}

static void SetIndexReg2(W65C02Generator* g, TargetInstruction* src1,
                         TargetInstruction* src2, TargetInstruction* dest,
                         int index) {
  if (!Is65c02()) {
    if (GetAddrMode(src1) == kAddrModeIndirect ||
        GetAddrMode(src2) == kAddrModeIndirect||
        GetAddrMode(dest) == kAddrModeIndirect) {
      ldyi(g, index);
      return;
    }
  }
  if (GetAddrMode(src1) == kAddrModeIndirectIndexed ||
      GetAddrMode(src2) == kAddrModeIndirectIndexed ||
      GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    ldyi(g, index);
  }
}

static void SetIndexRegDown(W65C02Generator* g, TargetInstruction* src,
                            TargetInstruction* dest, int index, int limit) {
  if (!Is65c02()) {
    if (GetAddrMode(src) == kAddrModeIndirect ||
        GetAddrMode(dest) == kAddrModeIndirect) {
      ldyi(g, limit - 1);
      return;
    }
  }
  if (GetAddrMode(src) == kAddrModeIndirectIndexed ||
      GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    ldyi(g, limit - 1);
  }
}

// Inline copy with no indexing.
static void InlineCopy(W65C02Generator* g, TargetInstruction* to,
                       TargetInstruction* from, int size, int to_index,
                       int from_index) {
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, from, from, from_index);
    lda(g, from, from_index++);
    SetIndexReg(g, to, to, to_index);
    sta(g, to, to_index++);
  }
}

static void InlineCopyToIndirectIndexed(W65C02Generator* g, TargetInstruction* to,
                                        TargetInstruction* from, int size, int to_index,
                                        int from_index, AddressingMode from_mode) {
  AddressingMode old_to = GetAddrMode(to);
  AddressingMode to_mode = old_to;
  int j = to_index;
  int k = from_index;
  if (Is65c02() && j == 0) {
    to_mode = kAddrModeIndirect;
  }
  for (int i = 0; i < size; i++, j++, k++) {
    SetAddrMode(to, to_mode);
    SetIndexReg(g, from, from, k);
    lda(g, from, k);
    SetIndexReg(g, to, to, j);
    sta(g, to, j);
    to_mode = kAddrModeIndirectIndexed;
  }
  SetAddrMode(to, old_to);
}

// Loop to copy from:
// 1. (addr),Y (or addr,Y) to zp,X.
// 2. addr,X to (addr),Y or addr,Y
// if to_index is 0 the loop runs backwards from from_start_index + size - 1
// to 0 using X
// If to_index is not zero it runs forwards with a cpx at the end.
static void CopyWithLoopXY(W65C02Generator* g, TargetInstruction* to,
                         TargetInstruction* from, int size, int to_index,
                         int from_index, AddressingMode to_mode, AddressingMode from_mode) {
  AddressingMode old_to = GetAddrMode(to);
  AddressingMode old_from = GetAddrMode(from);

  SetAddrMode(to, to_mode);
  SetAddrMode(from, from_mode);

  ldyi(g, size + from_index - 1);
  if (to_index == 0) {
    ldxi(g, size + to_index - 1);
  } else {
    ldxi(g, to_index);
  }
  TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  lda(g, from, from_mode == kAddrModeAbsoluteSymbol ? 0 : -1);
  sta(g, to, to_mode == kAddrModeAbsoluteSymbol ? 0 : -1);
  dey(g);
  if (to_index == 0) {
    dex(g);
    EmitResolvedBranch(g, W65C02_OP(bpl), loop);
  } else {
    inx(g);
    cpxi(g, size+to_index);
    EmitResolvedBranch(g, W65C02_OP(bne), loop);
  }
  
  SetAddrMode(to, old_to);
  SetAddrMode(from, old_from);
}


// Copy from addr,X to (addr),Y or addr,Y.
//   LDY #to_index+size-1
//   LDX #from_index+size-1
// loop:
//   LDA from,X
//   STA (to),Y
//   DEY
//   DEX
//   CPX #from_index
//   BPL loop
static void CopyWithLoopYX(W65C02Generator* g, TargetInstruction* to,
                         TargetInstruction* from, int size, int to_index,
                         int from_index, AddressingMode to_mode, AddressingMode from_mode) {
  AddressingMode old_to = GetAddrMode(to);
  AddressingMode old_from = GetAddrMode(from);

  SetAddrMode(to, to_mode);
  SetAddrMode(from, from_mode);

  ldyi(g, size + to_index - 1);
  ldxi(g, size + from_index - 1);

  TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  lda(g, from, from_mode == kAddrModeAbsoluteSymbol ? 0 : -1);
  sta(g, to, to_mode == kAddrModeAbsoluteSymbol ? 0 : -1);
  dey(g);
  dex(g);
  if (from_index != 0) {
    cpxi(g, from_index);
  }
  EmitResolvedBranch(g, W65C02_OP(bpl), loop);
  
  SetAddrMode(to, old_to);
  SetAddrMode(from, old_from);
}

// Copy with loop using X as index for both to and from.
static void CopyWithLoopX(W65C02Generator* g, TargetInstruction* to,
                         TargetInstruction* from, int size, int to_index,
                         int from_index, AddressingMode to_mode, AddressingMode from_mode) {
  AddressingMode old_to = GetAddrMode(to);
  AddressingMode old_from = GetAddrMode(from);

  SetAddrMode(to, to_mode);
  SetAddrMode(from, from_mode);

  ldxi(g, size + to_index - 1);
  TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  lda(g, from, from_mode == kAddrModeAbsoluteSymbol ? 0 : -1);
  sta(g, to, to_mode == kAddrModeAbsoluteSymbol ? 0 : -1);
  dex(g);
  EmitResolvedBranch(g, W65C02_OP(bpl), loop);
  
  SetAddrMode(to, old_to);
  SetAddrMode(from, old_from);
}

static TargetInstruction* GetImmediateLiteral(W65C02Generator* g, TargetInstruction* from) {
  TargetConstant* c = (TargetConstant*)from;
  Literal* lit = CompilerFindLiteral(c->literal_id);
  assert(lit != NULL);
  lit->disabled = false;
  // Copy from literal into zero page.  Uses literal,X
  return Emit(g, NewInstruction1(
              W65C02_OP(literalrefX),
              &c->base,
              kAddrModeImplied));
}

// Copy from memory to absolute acddress.
static void CopyToAbsolute(W65C02Generator* g, TargetInstruction* to,
                           TargetInstruction* from, int to_start_index,
                           int from_start_index,
                           int size, AddressingMode to_mode,
                           AddressingMode from_mode) {

  AddressingMode loop_to_mode = to_mode;
  switch (to_mode) {
    case kAddrModeZeroPage:
      loop_to_mode = kAddrModeZeroPageIndexedX;
      break;
    case kAddrModeAbsoluteSymbol:
      loop_to_mode = kAddrModeAbsoluteSymbolIndexedY;
      break;
    default:
      abort();
      
  }
  AddressingMode loop_from_mode = from_mode;
  switch (from_mode) {
    case kAddrModeImmediate:
      loop_from_mode = kAddrModeLiteralIndexedX;
      break;
    case kAddrModeZeroPage:
      loop_from_mode = kAddrModeZeroPageIndexedX;
      break;
    case kAddrModeSymbolAddr:
      break;
    case kAddrModeAbsoluteSymbol:
      loop_from_mode = kAddrModeAbsoluteSymbolIndexedY;
      break;
    case kAddrModeIndirectIndexed:   // (zp),Y
      break;
    default:
      abort();
  }
  
  switch (from_mode) {
    case kAddrModeIndirectIndexed: {  // (zp),Y
      if ((size == 4 || size == 8) && to != from && to_start_index == 0 &&
          from_start_index == 0 &&
          to_mode == kAddrModeZeroPage) {
        // X is the zero-page address register and Y is the zero-page
        // destination register. This replaces the repeated indexed
        // load/store sequence with a compact runtime call.
        W65C02Opcode opcode = size == 4 ? W65C02_OP(load_indirect4)
                                       : W65C02_OP(load_indirect8);
        Emit(g, NewInstruction2(opcode, from, to, kAddrModeImplied));
        break;
      }
      if (size > 2) {
        CopyWithLoopXY(g, to, from, size, to_start_index, from_start_index, loop_to_mode, from_mode);
      } else {
        // Inline copy from (addr),Y to absolute.
        int j = to_start_index;
        int k = from_start_index;
        if (Is65c02() && k == 0) {
          from_mode = kAddrModeIndirect;
        }
        for (int i = 0; i < size; i++, j++, k++) {
          SetAddrMode(from, from_mode);
          if (from_mode != kAddrModeIndirect) {
            SetIndexReg(g, from, from, k);
          }
          lda(g, from, k);
          sta(g, to, j);
          from_mode = kAddrModeIndirectIndexed;
        }
      }
      break;
    }
      
    case kAddrModeSymbolAddr:
      InlineCopy(g, to, from, size, to_start_index, from_start_index);
      break;
      
    case kAddrModeAbsoluteSymbol:
      if (size > 2) {
        // Loop to copy from addr,Y to zp,X.
         CopyWithLoopXY(g, to, from, size, to_start_index, from_start_index,
                       loop_to_mode,loop_from_mode);
      } else {
        // Inline copy from addr,Y to absolute.
        InlineCopy(g, to, from, size, to_start_index, from_start_index);
      }
     break;
      
    case kAddrModeImmediate:
      // Copy from immediate into zero page.  More than 2 bytes are copied
      // using a loop.  If non-zero, copy from literal for >2 bytes.
      if (TargetIsZero(from)) {
        if (size > 2) {
          // Store 0 into zero page with an indexed X loop.
          SetAddrMode(to, loop_to_mode);
          if (to_start_index == 0) {
            ldxi(g, size-1);
            ldai(g, 0);
            TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
            sta(g, to, -1);     // There is no STZ zp,X
            dex(g);
            EmitResolvedBranch(g, W65C02_OP(bpl), loop);
          } else {
            ldxi(g, to_start_index);
            ldai(g, 0);
            TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
            sta(g, to, -1);     // There is no STZ zp,X
            inx(g);
            cpxi(g, to_start_index+size);
            EmitResolvedBranch(g, W65C02_OP(bne), loop);
          }
        } else {
          // Inline store.
          for (int i = 0; i < size; i++) {
            stz(g, to, i);
          }
        }
        break;
      }
      if (size > 2) {
        // 4 or 8 byte constant.  There is a literal for it.
        from = GetImmediateLiteral(g, from);
        CopyWithLoopX(g, to, from, size, to_start_index, 0,
                      loop_to_mode, loop_from_mode);
      } else {
        InlineCopy(g, to, from, size, to_start_index, 0);
      }
      break;
      
    case kAddrModeZeroPage:
      assert(from_start_index < size);
      
      if (size > 2) {
        CopyWithLoopX(g, to, from, size, to_start_index,
                      from_start_index, loop_to_mode, loop_from_mode);
      } else {
        InlineCopy(g, to, from, size, to_start_index, from_start_index);
      }
      break;
    default:
      abort();
      break;
  }
}

static void CopyToMemoryIndirectIndexed(W65C02Generator* g, TargetInstruction* to,
                           TargetInstruction* from, int to_start_index,
                         int from_start_index,
                           int size,
                         AddressingMode from_mode) {
  switch (from_mode) {
    case kAddrModeIndirectIndexed:
      break;
    case kAddrModeZeroPage:
      assert(from_start_index < size);
      if ((size == 4 || size == 8) && to != from && to_start_index == 0 &&
          from_start_index == 0) {
        // X is the zero-page source register and Y is the zero-page address
        // register. The helper copies the complete value through that pointer.
        W65C02Opcode opcode = size == 4 ? W65C02_OP(store_indirect4)
                                       : W65C02_OP(store_indirect8);
        Emit(g, NewInstruction2(opcode, from, to, kAddrModeImplied));
        break;
      }
      // Fall through.
      
   case kAddrModeAbsoluteSymbol:
      // Copying more than 1 byte to indirect is longer than a loop.
      if (size > 2) {
        CopyWithLoopYX(g, to, from, size, to_start_index, from_start_index,
                       kAddrModeIndirectIndexed, kAddrModeZeroPageIndexedX);
      } else {
        // Inline copy from zero page to (addr),Y.
        InlineCopyToIndirectIndexed(g, to, from, size, to_start_index, from_start_index,
                                    from_mode);
      }
      break;
      
    case kAddrModeSymbolAddr:
      InlineCopy(g, to, from, size, to_start_index, from_start_index);
      break;
      
    case kAddrModeImmediate:
      // Copy from immediate to (addr),Y.
      if (TargetIsZero(from)) {
        if (size > 1) {
          // Store 0 into (addr),Y with an indexed Y loop.
          if (to_start_index == 0) {
            // Y goes from size-1 to 0.
            ldyi(g, size-1);
            ldai(g, 0);
            TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
            sta(g, to, -1);
            dey(g);
            EmitResolvedBranch(g, W65C02_OP(bpl), loop);
          } else {
            // Y isn't zero-based.  Need to compare against size+start_index.
            ldyi(g, to_start_index);
            ldai(g, 0);
            TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
            sta(g, to, -1);
            iny(g);
            cpyi(g, size + to_start_index);
            EmitResolvedBranch(g, W65C02_OP(bne), loop);
          }
        } else {
          int j = to_start_index;
          ldai(g, 0);
          for (int i = 0; i < size; i++, j++) {
            SetIndexReg(g, to, to, j);
            sta(g, to, j);
          }
        }
        break;
      }
      if (size > 2) {
        // 4 or 8 byte constant.  There is a literal for it.
        from = GetImmediateLiteral(g, from);
        CopyWithLoopYX(g, to, from, size, 0, 0,
                      kAddrModeIndirectIndexed, kAddrModeLiteralIndexedX);
      } else {
        InlineCopy(g, to, from, size, to_start_index, from_start_index);
      }
      break;
    default:
      abort();
      break;
  }
}

// Copy memory in the fewest bytes possible.
static void Copy(W65C02Generator* g, TargetInstruction* to,
                 TargetInstruction* from, int from_start_index,
                 int to_start_index,
                 int size,
                 AddressingMode to_mode,
                 AddressingMode from_mode) {
  AddReloadPoint(g, to);
  AddReloadPoint(g, from);

  AddressingMode old_to = GetAddrMode(to);
  AddressingMode old_from = GetAddrMode(from);
  SetAddrMode(from, from_mode);
  SetAddrMode(to, to_mode);
  
  switch (to_mode) {
    case kAddrModeZeroPage:
      assert(to_start_index < size);
      // Fall through
      
    case kAddrModeAbsoluteSymbol:
      CopyToAbsolute(g, to, from, to_start_index, from_start_index, size, to_mode, from_mode);
      break;
    case kAddrModeIndirectIndexed:
      CopyToMemoryIndirectIndexed(g, to, from, to_start_index, from_start_index, size, from_mode);
      break;
    default:
      abort();
  }
  SetAddrMode(from, old_from);
  SetAddrMode(to, old_to);
}

// TODO: allow override of tls model per variable.
static COMPILER_UNUSED TargetInstruction* GetTlsVariableAddress(W65C02Generator* g,
                                                               IRNode* node) {
  switch (compiler->tls_model) {
    default:
      abort();
    case TLS(global_dynamic):
    case TLS(local_dynamic): {
      // These call __tls_get_addr to get the address of the a TLS variable
      // from the GOT.
      //
      // First load the address of the GOT entry, based on the symbol.
      return NULL;
    }
    case TLS(initial_exec): {
      // The address of the TLS variable is obtained from adding the offset
      // from the GOT to the thread pointer.
      return NULL;
    }
    case TLS(local_exec): {
      // Address is constructed from thread pointer plus an offset
      // provided by the linker.
      return NULL;
    }
  }
}

static COMPILER_UNUSED void GetTlsAddressAndOffset(W65C02Generator* g,
                                                   IRNode* addr_node,
                                                   TargetInstruction** addr,
                                                   TargetInstruction** offset) {
  switch (compiler->tls_model) {
    default:
      abort();
    case TLS(global_dynamic):
    case TLS(local_dynamic): {
      // These call __tls_get_addr to get the address of the a TLS variable
      // from the GOT.
      //
      // First load the address of the GOT entry, based on the symbol.
      break;
    }
    case TLS(initial_exec): {
      // The address of the TLS variable is obtained from adding the offset
      // from the GOT to the thread pointer.
      break;
    }
    case TLS(local_exec): {
      // Address is thread pointer plus an offset obtained from the
      // linker.
      break;
    }
  }
}

static void ApplyFixups(W65C02Generator* g, IRNode* label_node) {
  TargetApplyFixups(&g->base, label_node);
}

// Temp expression in zero page holding value.
static TargetInstruction* TempRegister(W65C02Generator* g, TypeRecord* type, int size) {
  if (TypeIsFloat(type)) {
    return Emit(g, NewInstruction(W65C02_OP(exprf), kAddrModeZeroPage));
  }
  if (TypeIsDouble(type)) {
    return Emit(g, NewInstruction(W65C02_OP(exprd), kAddrModeZeroPage));
  }
  switch (size) {
    case 1:
      return Emit(g, NewInstruction(W65C02_OP(expr1), kAddrModeZeroPage));
    default:        // Any other size is handled as a pointer.
    case 2:
      return Emit(g, NewInstruction(W65C02_OP(expr2), kAddrModeZeroPage));
    case 4:
      return Emit(g, NewInstruction(W65C02_OP(expr4), kAddrModeZeroPage));
    case 8:
      return Emit(g, NewInstruction(W65C02_OP(expr8), kAddrModeZeroPage));
  }
}

// Temp expression in zero page containing address of something.
static TargetInstruction* TempExpressionAddress(W65C02Generator* g, AddressingMode mode) {
  return Emit(g, NewInstruction(W65C02_OP(expr2), mode));
}

static W65C02Opcode VarValueFunction(int size, bool highzero) {
  switch (size) {
    case 1:
      return highzero ? W65C02_OP(var_value1) : W65C02_OP(var_value1b);
    case 2:
      return highzero ? W65C02_OP(var_value2) : W65C02_OP(var_value2b);
    case 4:
      return highzero ? W65C02_OP(var_value4) : W65C02_OP(var_value4b);
    case 8:
      return highzero ? W65C02_OP(var_value8) : W65C02_OP(var_value8b);
    default:
      abort();
  }
}

static W65C02Opcode ArgValueFunction(int size, bool highzero) {
  switch (size) {
    case 1:
      return highzero ? W65C02_OP(arg_value1) : W65C02_OP(arg_value1b);
    case 2:
      return highzero ? W65C02_OP(arg_value2)
                      : W65C02_OP(arg_value2b;) case 4 : return highzero
                                                        ? W65C02_OP(arg_value4)
                                                        : W65C02_OP(arg_value4b);
    case 8:
      return highzero ? W65C02_OP(arg_value8) : W65C02_OP(arg_value8b);
    default:
      abort();
  }
}

static IRNode* IgnoreCasts(IRNode* node) {
  while (node->opcode == IR_OP(cast)) {
    node = node->inputs.value.p[0];
  }
  return node;
}

static bool IsNRVONode(IRNode* node) {
  node = IgnoreCasts(node);
  if ((node->flags & kIRNrvoMarker) != 0) {
    return true;
  }
  if (node->opcode != IR_OP(localvar)) {
    return false;
  }
  Symbol* symbol = ((IRVariable*)node)->symbol;
  return symbol != NULL && symbol->is_nrvo;
}

static TargetInstruction* Materialize(W65C02Generator* g, IRNode* node, int size, bool put_in_zero_page) {
  // If the node is an argument push, indirect to its expression.
  if (node->opcode == IR_OP(pusharg)) {
    node = node->inputs.value.p[0];
  }
  TargetInstruction* inst = GetLoweredNode(node);
  AddReloadPoint(g, inst);
  TargetInstruction* result = NULL;
  int offset;
  if (size == -1) {
    size = Sizeof(node->type);
  }
  node = IgnoreCasts(node);
  
  if (IRIsConst(node)) {
    if (!put_in_zero_page) {
      return inst;
    }
    result = TempRegister(g, node->type, Sizeof(node->type));
    Copy(g, result, inst, 0, 0, size, GetAddrMode(result), GetAddrMode(inst));
    // The materialized constant lives in a zero-page register and may be live
    // across a call, so it must be spillable: register its spill point (the
    // store completes the value).  Omitting this makes the register allocator
    // pick it as a spill victim and then fail with "Can't find spill point".
    AddSpillPoint(g, result);
#if 0
    // Load a constant.
    result = TempRegister(g, node->type, Sizeof(node->type));
    TargetConstant* c = (TargetConstant*)inst;
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
    for (int i = 0; i < size; i++) {
      SetIndexReg(g, result, result, i);
      ldai(g, (int)(value >> (i * 8)) & 0xff);
      sta(g, result, i);
    }
    AddSpillPoint(g, result);
#endif
  } else {
    switch ((W65C02Opcode)inst->opcode) {
      case W65C02_OP(argument):
      case W65C02_OP(localvar): {
        if (IsNRVONode(node)) {
          // An NRVO object is the caller-provided aggregate result slot, not
          // storage in this function's local frame.
          assert(g->struct_return_inst != NULL);
          return g->struct_return_inst;
        }
        // Variable or argument.  Load value using one of the runtime helper
        // functions.
        offset = (int)TargetIntValue(inst->operand[0]);
        int size = Sizeof(node->type);
        if (TypeIsStructOrUnion(node->type)) {
          size = 2;
        }
        result = TempRegister(g, node->type, size);
        bool highzero = offset < 256;
        // Macro instruction, expands to:
        // lda #dest_addr
        // ldx #offset lo
        // ldy #offset hi (removed for single byte case)
        // JSR __var_value[b] (or arg_value[b])
        if (TargetOpcodeEq(inst->opcode, W65C02_OP(argument)) &&
            ((inst->flags & k6502NeedAddress) != 0 ||
             TypeIsStructOrUnion(node->type))) {
           W65C02Opcode arg_addr_op = offset >= 256 ? W65C02_OP(arg_addrb) : W65C02_OP(arg_addr);
          
            Emit(g,
                  NewInstruction2(arg_addr_op, result,
                                  inst,
                                  kAddrModeImplied));

        } else if (TargetOpcodeEq(inst->opcode, W65C02_OP(localvar)) &&
            ((inst->flags & k6502NeedAddress) != 0 ||
             TypeIsPointerOrArray(node->type) || TypeIsStructOrUnion(node->type))) {
          W65C02Opcode var_addr_op = offset >= 256 ? W65C02_OP(var_addrb) : W65C02_OP(var_addr);
        
          Emit(g,
                NewInstruction2(var_addr_op, result,
                                inst,
                                kAddrModeImplied));
        } else {
          W65C02Opcode var_value_op = TargetOpcodeEq(inst->opcode, W65C02_OP(argument))
                                         ? ArgValueFunction(size, highzero)
                                         : VarValueFunction(size, highzero);
          Emit(g,
               NewInstruction2(var_value_op, result,
                               inst,
                               kAddrModeZeroPage));
        }
        AddSpillPoint(g, result);
        break;
      }

      case W65C02_OP(ssavar):
      case W65C02_OP(phi): {
        TargetInstruction* var = inst->operand[0];
        offset = (int)TargetIntValue(var->operand[0]);
        result = TempRegister(g, node->type, Sizeof(node->type));
        bool highzero = offset < 256;
        // Macro instruction, expands to:
        // lda #dest_addr
        // ldx #offset lo
        // ldy #offset hi (removed for single byte case)
        // JSR __var_value[b] (or arg_value[b])
        W65C02Opcode var_value_op = TargetOpcodeEq(var->opcode, W65C02_OP(argument))
                                         ? ArgValueFunction(size, highzero)
                                         : VarValueFunction(size, highzero);
        Emit(g,
               NewInstruction2(var_value_op, result,
                               inst,
                               kAddrModeImplied));
        AddSpillPoint(g, result);
        break;
        }
      
      case W65C02_OP(symbol): {
        if (!put_in_zero_page && (inst->flags & k6502NeedAddress) == 0) {
          return inst;
        }
        // lda symbol+0
        // sta result+0
        // ldy #1
        // lda symbol+1, Y
        // sta result+1
        AddressingMode mode = kAddrModeAbsoluteSymbol;
        TargetSymbol* symbol = (TargetSymbol*)inst;
        if (TypeIsArray(node->type) ||
            TypeIsFunction(node->type) ||
            TypeIsFunction(symbol->symbol->type) ||
            TypeIsStructOrUnion(node->type) ||
            (inst->flags & k6502NeedAddress) != 0) {
          mode = kAddrModeSymbolAddr;
        }
        result = TempRegister(g, node->type, Sizeof(node->type));
        Copy(g, result, inst, 0, 0, Sizeof(node->type), GetAddrMode(result), mode);
        AddSpillPoint(g, result);
        break;
      }

      default:
        // An expression already in zero page.
        result = GetLoweredNode(node);
        break;
    }
  }
  return result;
}

static TargetInstruction* GetAddress(W65C02Generator* g, IRNode* addr_node, bool put_in_zero_page) {
  addr_node = IgnoreCasts(addr_node);
  
  AddressingMode mode;
  addr_node = IgnoreCasts(addr_node);
  if (TypeIsArray(addr_node->type) || TypeIsStructOrUnion(addr_node->type)) {
    mode = kAddrModeZeroPage;
  } else {
    mode = kAddrModeIndirectIndexed;
  }
  
  TargetInstruction* addr = GetLoweredNode(addr_node);
  TargetInstruction* result = NULL;
  int offset;
  switch ((W65C02Opcode)addr->opcode) {
    case W65C02_OP(argument):
    case W65C02_OP(localvar): {
      if (addr_node != NULL && IsNRVONode(addr_node)) {
        // This is an Named RVO variable.  It's address is the same
        // as the structreturn address.
        assert(g->struct_return_inst != NULL);
        return g->struct_return_inst;
      }
      offset = (int)TargetIntValue(addr->operand[0]);
      result = TempExpressionAddress(g, mode);
      // Macro instruction expands to:
      // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr[b] (or arg_addr[b])
      W65C02Opcode var_addr_op = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                                    ? W65C02_OP(arg_addr)
                                    : W65C02_OP(var_addr);
      if (offset >= 256) {
        var_addr_op = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                          ? W65C02_OP(arg_addrb)
                          : W65C02_OP(var_addrb);
      }
      Emit(g, NewInstruction2(var_addr_op, result,
                              addr,
                              mode));
      AddSpillPoint(g, result);
      break;
    }

    case W65C02_OP(symbol): {
      // lda #%lo(symbol)
      // sta result
      // lda #%hi(symbol)
      // sta result+1
      if (!put_in_zero_page) {
        return addr;
      }
      result = TempRegister(g, addr_node->type, 2);
      Copy(g, result, addr, 0, 0, 2, GetAddrMode(result), kAddrModeSymbolAddr);
     
      AddressingMode mode = kAddrModeIndirectIndexed;
      if (TypeIsArray(addr_node->type) || TypeIsFunction(addr_node->type) ||
          TypeIsStructOrUnion(addr_node->type) ||
          addr_node->opcode == IR_OP(addressof) ||
          (addr->flags & k6502NeedAddress) != 0) {
        mode = kAddrModeZeroPage;
      }
      SetAddrMode(result, mode);
      AddSpillPoint(g, result);
      break;
    }
    default:
      result = addr;
      break;
  }
  return result;
}

static void GetAddressXY(W65C02Generator* g, IRNode* addr_node) {
  TargetInstruction* addr = GetLoweredNode(addr_node);
  AddReloadPoint(g, addr);
  int offset;
  switch ((W65C02Opcode)addr->opcode) {
    case W65C02_OP(argument):
    case W65C02_OP(localvar): {
      if (IsNRVONode(addr_node)) {
        // This is an Named RVO variable.  It's address is the same
        // as the structreturn address.
        assert(g->struct_return_inst != NULL);
        ldx(g, g->struct_return_inst, 0);
        ldy(g, g->struct_return_inst, 0);  // ,1?
        return;
      }
      offset = (int)TargetIntValue(addr->operand[0]);
      // Macro instruction expands to:
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr_xy (or arg_addr_xy)
      W65C02Opcode var_addr_op = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                                    ? W65C02_OP(arg_addr_xy)
                                    : W65C02_OP(var_addr_xy);
  
      if (offset >= 256) {
        var_addr_op = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                          ? W65C02_OP(arg_addrb_xy)
                          : W65C02_OP(var_addrb_xy);
      }
      Emit(g, NewInstruction1(var_addr_op,
                              addr,
                              kAddrModeImplied));
      break;
    }

    case W65C02_OP(ssavar):
    case W65C02_OP(phi): {
      TargetInstruction* var = addr->operand[0];
      offset = (int)TargetIntValue(var->operand[0]);
      // Macro instruction expands to:
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr_xy (or arg_addr_xy)
      W65C02Opcode var_addr_op = TargetOpcodeEq(var->opcode, W65C02_OP(argument))
                                    ? W65C02_OP(arg_addr_xy)
                                    : W65C02_OP(var_addr_xy);
      if (offset >= 256) {
        var_addr_op = TargetOpcodeEq(var->opcode, W65C02_OP(argument))
                          ? W65C02_OP(arg_addrb_xy)
                          : W65C02_OP(var_addrb_xy);
      }
      Emit(g, NewInstruction1(var_addr_op,
                              var,
                              kAddrModeImplied));
      break;
    }

    case W65C02_OP(symbol): {
      // ldx #%lo(symbol)
      // ldy #%hi(symbol)
      SetIndexReg(g, addr, addr, 0);
      Emit(g, NewInstruction2(W65C02_OP(ldx), addr,
                              ByteConst(g, 0),
                              kAddrModeSymbolAddr));
      SetIndexReg(g, addr, addr, 1);
      Emit(g, NewInstruction2(W65C02_OP(ldy), addr,
                              ByteConst(g, 1),
                              kAddrModeSymbolAddr));
      break;
    }
    default:
      abort();
      break;
  }
}

static TargetInstruction* GetDestAddress(W65C02Generator* g, IRNode* node, bool put_in_zero_page) {
  // TODO: multiple destinations?
  if (node->dest != NULL) {
    IRNode* dest = node->dest;
    LowerIRNode(g, dest);
    return GetAddress(g, dest, put_in_zero_page);
  }
 
  if (node->opcode == IR_OP(structreturn)) {
    return GetLoweredNode(node);
  }
//  if (node->opcode == IR_OP(rmovi) ||node->opcode == IR_OP(rmova)  ||
//      node->opcode == IR_OP(rmovf) ||
//      node->opcode == IR_OP(rmovd)) {
//    return GetAddress(g, node->inputs.value.p[0], put_in_zero_page);
//  }
  return TempRegister(g, node->type, Sizeof(node->type));
}

// General case (we can't use CMP, CPX or CPY because those set the carry).
// If we need Y we use __t0 as a counter that decrements to zero.
//   LDA #size
//   STA __t0
//   LDY #0
//   LDX #0
//   CLC/SEC
// loop:
//   LDA op1
//   ADC op2/SBC op2
//   STA dest
//   INX
//   INY
//   DEC __t0
//   BNE loop
//
// If we don't need Y we can use it as a counter.
//   LDY #size
//   LDX #0
//   CLC/SEC
// loop:
//   LDA op1
//   ADC op2/SBC op2
//   STA dest
//   INX
//   DEY
//   BNE loop
static void AddSubIntegerLoop(W65C02Generator* g, IRNode* node,
                                            W65C02Opcode op, W65C02Opcode carry_ctl,
                                            int size, TargetInstruction* dest,
                                            TargetInstruction* op1,
                                            TargetInstruction* op2) {
  AddressingMode op1_mode = GetAddrMode(op1);
  AddressingMode op2_mode = GetAddrMode(op2);
  AddressingMode dest_mode = GetAddrMode(dest);
  
  AddressingMode op1_loop_mode = op1_mode;
  AddressingMode op2_loop_mode = op2_mode;
  AddressingMode dest_loop_mode = dest_mode;
  bool op1_needs_y = false;
  bool op2_needs_y = false;
  bool dest_needs_y = false;
  
  switch (op1_mode) {
    case kAddrModeZeroPage:
      op1_loop_mode = kAddrModeZeroPageIndexedX;
      break;
      
    case kAddrModeIndirectIndexed:
      op1_needs_y = true;
      break;
    case kAddrModeImmediate:
      if (!TargetIsZero(op1)) {
        op1 = GetImmediateLiteral(g, op1);
        op1_loop_mode = kAddrModeLiteralIndexedX;
      }
      break;
      
    default:
      break;
  }
  
  switch (op2_mode) {
    case kAddrModeZeroPage:
      op2_loop_mode = kAddrModeZeroPageIndexedX;
      break;
      
    case kAddrModeIndirectIndexed:
      op2_needs_y = true;
      break;
    case kAddrModeImmediate:
      if (!TargetIsZero(op2)) {
        op2 = GetImmediateLiteral(g, op2);
        op2_loop_mode = kAddrModeLiteralIndexedX;
      }
      break;
      
    default:
      break;
  }
 
  switch (dest_mode) {
    case kAddrModeZeroPage:
      dest_loop_mode = kAddrModeZeroPageIndexedX;
      break;
      
    case kAddrModeIndirectIndexed:
      dest_needs_y = true;
      break;
      
    default:
      break;
  }
  
  SetAddrMode(op1, op1_loop_mode);
  SetAddrMode(op2, op2_loop_mode);
  SetAddrMode(dest, dest_loop_mode);

  bool need_y = op1_needs_y || op2_needs_y || dest_needs_y;
  
  if (need_y) {
    ldai(g, size);
    Emit(g, NewInstruction1(W65C02_OP(sta), ByteConst(g, W65C02_T0_REG), kAddrModeZeroPageAbsolute));
    ldyi(g, 0);
  } else {
    ldyi(g, size);
  }
  ldxi(g, 0);
  TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  lda(g, op1, -1);
  Operate(g, op, op2, -1);
  sta(g, dest, -1);
  inx(g);
  if (need_y) {
    iny(g);
    Emit(g, NewInstruction1(W65C02_OP(dec), ByteConst(g, W65C02_T0_REG), kAddrModeZeroPageAbsolute));
  } else {
    dey(g);
  }
  EmitResolvedBranch(g, W65C02_OP(bne), loop);
  SetAddrMode(op1, op1_mode);
  SetAddrMode(op2, op2_mode);
  SetAddrMode(dest, dest_mode);
}

static TargetInstruction* AddSubInteger(W65C02Generator* g, IRNode* node,
                                        W65C02Opcode op, W65C02Opcode carry_ctl,
                                        int size, TargetInstruction* dest,
                                        TargetInstruction* op1,
                                        TargetInstruction* op2) {
  AddReloadPoint(g, op1);
  AddReloadPoint(g, op2);
  AddReloadPoint(g, dest);
  Emit(g, NewInstruction(carry_ctl, kAddrModeImplied));
  bool is_const = GetAddrMode(op2) == kAddrModeImmediate;
  if (size > 2) {
    AddSubIntegerLoop(g, node, op, carry_ctl, size, dest, op1, op2);
    return dest;
  }
  for (int i = 0; i < size; i++) {
    SetIndexReg2(g, op1, op2, dest, i);
    lda(g, op1, i);
    if (i == 0 && is_const) {
      // There's no point in adding or subtracting 0 from the lower byte,
      if (ByteConst(g, 0) != 0) {
        Operate(g, op, op2, i);
      }
    } else {
      Operate(g, op, op2, i);
    }
    sta(g, dest, i);
  }
  return dest;
}

static bool IsIdempotentOperation(W65C02Opcode op, TargetInstruction* operand, AddressingMode mode, int index) {
  switch (op) {
    case W65C02_OP(ora):
      // ORA #0 is idempotent
      if (mode == kAddrModeImmediate) {
        int64_t immed = TargetIntValue(operand);
        immed = (immed >> (index * 8)) & 0xff;
        if (immed == 0) {
          return true;
        }
      }
      break;
    case W65C02_OP(and):
      // AND #255 is idempotent
      if (mode == kAddrModeImmediate) {
        int64_t immed = TargetIntValue(operand);
        immed = (immed >> (index * 8)) & 0xff;
        if (immed == 255) {
          return true;
        }
      }
      break;
    default:
      break;
  }
  return false;
}

static TargetInstruction* SimpleIntegerOp(W65C02Generator* g, IRNode* node,
                                          W65C02Opcode op, int size,
                                          TargetInstruction* dest,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  AddReloadPoint(g, op1);
  AddReloadPoint(g, op2);
  AddReloadPoint(g, dest);
  for (int i = 0; i < size; i++) {
    bool idempotent = IsIdempotentOperation(op, op2, GetAddrMode(op2), i);
    SetIndexReg2(g, op1, op2, dest, i);
    lda(g, op1, i);
    if (!idempotent) {
      Operate(g, op, op2, i);
    }
    sta(g, dest, i);
  }

  return dest;
}

static TargetInstruction* AndOp(W65C02Generator* g, IRNode* node,
                                          int size,
                                          TargetInstruction* dest,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  AddReloadPoint(g, op1);
  AddReloadPoint(g, op2);
  AddReloadPoint(g, dest);
  AddressingMode mode = GetAddrMode(op2);
  bool is_immediate = mode == kAddrModeImmediate;
  for (int i = 0; i < size; i++) {
    int64_t immed = -1;
    if (is_immediate) {
      immed = TargetIntValue(op2);
      immed = (immed >> (i * 8)) & 0xff;
    }
    SetIndexReg2(g, op1, op2, dest, i);
    if (is_immediate && immed == 0) {
      // AND #0 is zero.
      ldai(g, 0);
    } else {
      lda(g, op1, i);
      if (!is_immediate || immed != 255) {
        // Omit AND #255
        Operate(g, W65C02_OP(and), op2, i);
      }
    }
    sta(g, dest, i);
  }

  return dest;
}
// Unary not, equivalent to.
// dest = (src == 0) ? 1 : 0
//
// For 2 byte:
//  lda src lo
//  ora src hi
//  beq label
//  lda #255
// label:
//  inc a
//  sta dest
//    Rest of dest is zeroed out.
static TargetInstruction* UnaryNotOp(W65C02Generator* g, IRNode* src_node,
                                     TargetInstruction* dest,
                                     TargetInstruction* src, int dest_size) {
  int size = Sizeof(src_node->type);
  AddReloadPoint(g, src);
  AddReloadPoint(g, dest);
  TargetInstruction* zero_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);

  AddressingMode dest_mode = GetAddrMode(dest);

  // TODO optimize this for size > 2
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, src, dest, i);
    if (i == 0) {
      lda(g, src, i);
    } else {
      ora(g, src, i);
    }
  }
  cmpi(g, 0);
  EmitResolvedBranch(g, W65C02_OP(beq), zero_label);
  ldai(g, 255);
  Emit(g, zero_label);
  Emit(g, NewInstruction(W65C02_OP(inc), kAddrModeAccumulator));
  
  // Store A in dest.  It's stored in low byte and the upper bytes are zeroed.
  // Dest will be in zero page.
  if (dest_mode != kAddrModeIndirectIndexed) {
    sta(g, dest, 0);
    for (int i = 1; i < dest_size; i++) {
      if (Is65c02()) {
         stz(g, dest, i);
      } else {
        ldai(g, 0);
        sta(g, dest, i);
      }
     }
    
  } else {
    sta(g, dest, 0);
    ldai(g, 0);
    for (int i = 1; i < dest_size; i++) {
       sta(g, dest, i);
     }
  }
 

  return dest;
}

// lda src
// EOR #0xff
// sta dest
static TargetInstruction* OnesComplement(W65C02Generator* g, IRNode* src_node,
                                         TargetInstruction* dest,
                                         TargetInstruction* src, int size) {
  AddReloadPoint(g, src);
  AddReloadPoint(g, dest);
  AddressingMode src_mode = GetAddrMode(src);
  AddressingMode dest_mode = GetAddrMode(dest);
  if (size > 2 && src_mode == kAddrModeZeroPage && dest_mode == kAddrModeZeroPage) {
    TargetInstruction* loop = NewInstruction(W65C02_OP(label), kAddrModeImplied);
    SetAddrMode(src, kAddrModeZeroPageIndexedX);
    SetAddrMode(dest, kAddrModeZeroPageIndexedX);
    ldxi(g, size-1);
    Emit(g, loop);
    lda(g, src, -1);
    eori(g, 0xff);
    sta(g, dest, -1);
    dex(g);
    EmitResolvedBranch(g, W65C02_OP(bpl), loop);
    SetAddrMode(src, src_mode);
    SetAddrMode(dest, dest_mode);
  } else {
    for (int i = 0; i < size; i++) {
      SetIndexReg(g, src, dest, i);
      lda(g, src, i);
      eori(g, 0xff);
      sta(g, dest, i);
    }
  }
  return dest;
}

// SEC
// lda #0
// SBC src
// sta dest
// ...
static TargetInstruction* NegateInteger(W65C02Generator* g, IRNode* src_node,
                                        TargetInstruction* dest,
                                        TargetInstruction* src, int size) {
  TargetType target_type;
  switch (size) {
    case 1:
      target_type = kTargetType8Bit;
      break;
    case 2:
      target_type = kTargetType16Bit;
      break;
    case 4:
      target_type = kTargetType32Bit;
      break;
    case 8:
      target_type = kTargetType64Bit;
      break;
    default:
      abort();

  }
  TargetInstruction* zero =
      GetIntConstant(g, NULL, target_type, 0);
  
  return AddSubInteger(g, src_node, W65C02_OP(sbc),
                       W65C02_OP(sec), size, dest, zero, src);
}

// To negate a floating point value, flip the sign bit (top bit).
static TargetInstruction* NegateFloatingPoint(W65C02Generator* g, IRNode* src_node,
                                              TargetInstruction* dest,
                                              TargetInstruction* src, int size) {
  if (src != dest) {
    Copy(g, dest, src, 0, 0, size, GetAddrMode(dest), GetAddrMode(src));
    src = dest;
  }
  ldyi(g, size-1);
  lda(g, src, size-1);
  eori(g, 0x80);
  sta(g, dest, size-1);
  return dest;
}

static void ShiftOnce(W65C02Generator* g, IRNode* node, W65C02Opcode op,
                      W65C02Opcode second_op, int size, TargetInstruction* dest,
                      TargetInstruction* op1) {
  AddressingMode mode = GetAddrMode(dest);
  if (op == W65C02_OP(asl)) {
    // Shifting left, start at lsb
    for (int i = 0; i < size; i++) {
      SetIndexReg(g, op1, dest, i);
      if (dest == op1 && mode != kAddrModeIndirect && mode != kAddrModeIndirectIndexed) {
        // Source and dest are the same, operate directly on the operand.
        Emit(g, NewInstruction2(i == 0 ? op : second_op, dest,
                                ByteConst(g, i),
                                GetAddrMode(dest)));
      } else {
        lda(g, op1, i);
        Emit(g, NewInstruction(i == 0 ? op : second_op, kAddrModeAccumulator));
        sta(g, dest, i);
      }
    }
  } else {
    // Shifting right, start at msb
    for (int i = size-1; i >= 0; i--) {
      SetIndexReg(g, op1, dest, i);
      if (dest == op1 && mode != kAddrModeIndirect && mode != kAddrModeIndirectIndexed) {
        // Source and dest are the same, operate directly on the operand.
        Emit(g, NewInstruction2(i == size-1 ? op : second_op, dest,
                                ByteConst(g, i),
                                GetAddrMode(dest)));
      } else {
        lda(g, op1, i);
        Emit(g, NewInstruction(i == size-1 ? op : second_op, kAddrModeAccumulator));
        sta(g, dest, i);
      }
    }
  }
}

static TargetInstruction* ShiftLeftByMultipleOf8(W65C02Generator* g, IRNode* node,
                                  int size, TargetInstruction* dest,
                                  TargetInstruction* src, int count) {
  int bytes = count / 8;
  int from_offset = size - bytes - 1;
  int to_offset = size - 1;
  while (from_offset >= 0) {
    SetIndexReg(g, src, dest, from_offset);
    lda(g, src, from_offset);
    sta(g, dest, to_offset);
    from_offset--;
    to_offset--;
  }
  ldai(g, 0);
  for (int i = 0; i < bytes; i++) {
    SetIndexReg(g, dest, dest, i);
    sta(g, dest, i);
  }
  return dest;
}

static void ShiftRightByMultipleOf8(W65C02Generator* g, IRNode* node,
                                                  bool is_unsigned,
                                  int size, TargetInstruction* dest,
                                  TargetInstruction* src, int count) {
  int bytes = count / 8;
  int from_offset = bytes;
  int to_offset = 0;
  while (from_offset < size) {
    SetIndexReg(g, src, dest, from_offset);
    lda(g, src, from_offset);
    sta(g, dest, to_offset);
    from_offset++;
    to_offset++;
  }
  if (!is_unsigned) {
    // A = top byte of src.
    // eor #0x80
    // cmp #0x80      // Carry set = positive
    // lda #0
    // sbc #0         // 255 if negative, zero if positive
    
    SetIndexReg(g, src, src, size-1);
    eori(g, 0x80);
    cmpi(g, 0x80);            // Set carry flag if top bit set.
    ldai(g, 0);
    sbci(g, 0);         // 255 if negative, zero if positive
  } else {
    ldai(g, 0);
  }
  for (int i = size - bytes; i < size; i++) {
    SetIndexReg(g, dest, dest, i);
    sta(g, dest, i);
  }
}

static void ShiftByMultipleOf8(W65C02Generator* g, IRNode* node,
                                  W65C02Opcode op, W65C02Opcode second_op,
                                  int size, TargetInstruction* dest,
                                  TargetInstruction* op1, int count) {
  if (op == W65C02_OP(asl)) {
     ShiftLeftByMultipleOf8(g, node, size, dest, op1, count);
     return;
  }
  ShiftRightByMultipleOf8(g, node, op != W65C02_OP(ror), size, dest, op1, count);
}

static TargetInstruction* ConstantShiftOp(W65C02Generator* g, IRNode* node,
                                  W65C02Opcode op, W65C02Opcode second_op,
                                  int size, TargetInstruction* dest,
                                  TargetInstruction* op1, int count) {
  if (count == 0) {
    return dest;
  }
  if (count >= size * 8) {
    Copy(g, dest, Zero(g), 0, 0, size, GetAddrMode(dest), kAddrModeImmediate);
    return dest;
  }
  AddReloadPoint(g, op1);
  AddReloadPoint(g, dest);
  AddressingMode mode = GetAddrMode(dest);
  if (count >= 8) {
    // Shifting by a multiple of 8.  We can do that by bytes instead of bits.
    ShiftByMultipleOf8(g, node, op, second_op, size, dest, op1, count);
    
    // We've shifted by a multiple of 8.  Now we can shift the remaining
    // 0-7 bits.
    count &= 7;
    if (count == 0) {
      return dest;
    }
    op1 = dest;
  }
  
  if (count > 1 && dest == op1 && mode != kAddrModeIndirect && mode != kAddrModeIndirectIndexed) {
    // Dest and src are the same and valid shift instruction operands.
    TargetInstruction* loop = NULL;
    ldxi(g, count);
    loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
    ShiftOnce(g, node, op, second_op, size, dest, dest);
    dex(g);
    EmitResolvedBranch(g, W65C02_OP(bne), loop);
    return dest;
  }
  
  // Shift the first iteration.
  ShiftOnce(g, node, op, second_op, size, dest, op1);
  if (count == 1) {
    return dest;
  }
  // If there are more iterations, shift in a loop with src and dest the same.
  TargetInstruction* loop = NULL;
  if (count > 2) {
    ldxi(g, count-1);
    loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  }
  ShiftOnce(g, node, op, second_op, size, dest, dest);
  if (loop != NULL) {
    dex(g);
    EmitResolvedBranch(g, W65C02_OP(bne), loop);
  }
  return dest;
}

static TargetInstruction* ShiftOp(W65C02Generator* g, IRNode* node,
                                  W65C02Opcode op, W65C02Opcode second_op,
                                  int size, TargetInstruction* dest,
                                  TargetInstruction* op1, TargetInstruction* count) {
  if (TargetIsConst(count)) {
    return ConstantShiftOp(g, node, op, second_op, size, dest, op1, (int)TargetIntValue(count));
  }
  
  // ldx count
  // beq skip
  // shift once
  // dex
  // loop:
  // shift once
  // dex
  // bne loop
  // skip:
  AddReloadPoint(g, op1);
  AddReloadPoint(g, dest);
  AddReloadPoint(g, count);
  ldx(g, count, 0);
  TargetInstruction* skip = NewInstruction(W65C02_OP(label), kAddrModeImplied);
  EmitResolvedBranch(g, W65C02_OP(beq), skip);
  // Shift the first iteration.
  ShiftOnce(g, node, op, second_op, size, dest, op1);
  dex(g);
  TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  ShiftOnce(g, node, op, second_op, size, dest, dest);
  dex(g);
  EmitResolvedBranch(g, W65C02_OP(bne), loop);
  Emit(g, skip);
  return dest;
}

static void ShiftOnceArithmeticRight(W65C02Generator* g, IRNode* node,
                                     int size, TargetInstruction* dest,
                      TargetInstruction* src) {
  cmpi(g, 0x80);            // Set carry flag if top bit set.
  AddressingMode addrmode = GetAddrMode(src);
  for (int j = size - 1; j >= 0; j--) {
     AddressingMode mode = addrmode;
     SetIndexRegDown(g, src, dest, j, size);
     if (Is65c02() && addrmode == kAddrModeIndirectIndexed && j == 0) {
       mode = kAddrModeIndirect;
     }
    // ROR is used here because we've already set the carry flag to the sign.
     if (dest == src && mode != kAddrModeIndirect && mode != kAddrModeIndirectIndexed) {
        Emit(g, NewInstruction2(W65C02_OP(ror), src, ByteConst(g, j), mode));
     } else {
       lda(g, src, j);
        Emit(g, NewInstruction(W65C02_OP(ror),  kAddrModeAccumulator));
       sta(g, dest, j);
     }
   }
}


static TargetInstruction* ConstantArithmeticRightShiftOp(W65C02Generator* g,
                                                 IRNode* node, int size,
                                                 TargetInstruction* dest,
                                                 TargetInstruction* src,
                                                 int count) {
  if (count == 0) {
    Copy(g, dest, src, 0, 0, size, GetAddrMode(dest), GetAddrMode(src));
    return dest;
  }
  if (count >= size * 8) {
    Copy(g, dest, Zero(g), 0, 0, size, GetAddrMode(dest), kAddrModeImmediate);
    return dest;
  }
  // lda hi
  // cmp $0x80
  // ror hi
  // ror lo
  AddReloadPoint(g, src);
  AddReloadPoint(g, dest);
  bool long_shift = count >= 8;
  if (long_shift) {
    ShiftRightByMultipleOf8(g, node, false, size, dest, src, count);
    // We've shifted by a multiple of 8.  Now we can shift the remaining
    // 0-7 bits.
    count &= 7;
    if (count == 0) {
      return dest;
    }
    src = dest;
  }
  
  TargetInstruction* loop = NULL;
  if (src == dest) {
    if (count == 1) {
      SetIndexReg(g, src, dest, size-1);
      if (!long_shift) {
        lda(g, src, size-1);    // For a long shift (>8) A contains top byte.
      }
      ShiftOnceArithmeticRight(g, node, size, dest, src);
    } else {
      ldxi(g, count);
      loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
      SetIndexReg(g, src, dest, size-1);
      lda(g, src, size-1);
      ShiftOnceArithmeticRight(g, node, size, dest, src);
      dex(g);
      EmitResolvedBranch(g, W65C02_OP(bne), loop);
    }
    return dest;
  }
  // First shift puts src into dest.  ShiftOnceArithmeticRight starts with a
  // `cmp #$80` to seed the carry from the sign bit, which reads A -- so the
  // high byte must be in A first.  (The src==dest path above loads it; this
  // path previously relied on whatever happened to be in A, which set the
  // carry wrong for non-negative values and rotated a 1 into the sign bit.)
  SetIndexReg(g, src, dest, size-1);
  lda(g, src, size-1);
  ShiftOnceArithmeticRight(g, node, size, dest, src);
  if (count == 1) {
    // Only one shift, we're done.
    return dest;
  }
  if (count > 2) {
    // Need a loop.
    ldxi(g, count - 1);
    loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  }
  // Rest of shifts use dest only.
  SetIndexReg(g, dest, dest, size-1);
  lda(g, dest, size-1);
  ShiftOnceArithmeticRight(g, node, size, dest, dest);
  if (loop != NULL) {
    dex(g);
    EmitResolvedBranch(g, W65C02_OP(bne), loop);
  }
  return dest;
}


static TargetInstruction* ArithmeticRightShiftOp(W65C02Generator* g,
                                                 IRNode* node, int size,
                                                 TargetInstruction* dest,
                                                 TargetInstruction* src,
                                                 TargetInstruction* count) {
  if (TargetIsConst(count)) {
    return ConstantArithmeticRightShiftOp(g, node, size, dest, src, (int)TargetIntValue(count));
  }
  // ldx count
  // beq skip
  // lda hi
  // cmp $0x80
  // ror hi
  // ror lo
  // dex:
  // loop:
  // shift
  // dex
  // bne skip
  // skip:
  AddReloadPoint(g, src);
  AddReloadPoint(g, dest);
  AddReloadPoint(g, count);
  ldx(g, count, 0);
  TargetInstruction* skip = NewInstruction(W65C02_OP(label), kAddrModeImplied);
  EmitResolvedBranch(g, W65C02_OP(beq), skip);
  SetIndexReg(g, src, dest, size-1);
  lda(g, src, size-1);
  
  // Firat shift puts src into dest.
  ShiftOnceArithmeticRight(g, node, size, dest, src);

  dex(g);
  TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  // Rest of shifts use dest only.
  SetIndexReg(g, dest, dest, size-1);
  lda(g, dest, size-1);
  ShiftOnceArithmeticRight(g, node, size, dest, dest);
  dex(g);
  EmitResolvedBranch(g, W65C02_OP(bne), loop);
  Emit(g, skip);
  return dest;
}

static bool DestInZeroPage(TargetInstruction* dest) {
  switch ((W65C02Opcode)dest->opcode) {
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
    case W65C02_OP(expr1):
    case W65C02_OP(expr4):
    case W65C02_OP(expr8):
    case W65C02_OP(exprf):
    case W65C02_OP(exprd):
      return true;
    case W65C02_OP(expr2):
      return GetAddrMode(dest) == kAddrModeZeroPage;
    default:
      break;
  }
  return false;
}

static TargetInstruction* CallArithmeticBinaryRuntime(
    W65C02Generator* g, IRNode* node, TargetInstruction* dest,
    TargetInstruction* op1, TargetInstruction* op2, Symbol* func) {
  AddReloadPoint(g, op1);
  AddReloadPoint(g, op2);
  AddReloadPoint(g, dest);
  bool dest_in_zero_page = DestInZeroPage(dest);
  
  TargetInstruction* tmp = NULL;
  
  if (dest_in_zero_page) {
    Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), dest, kAddrModeImplied));
  } else {
    tmp = TempRegister(g, node->type, Sizeof(node->type));
    AddSpillPoint(g, tmp);
    Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), tmp, kAddrModeImplied));
  }
  
  // lda #dest
  // ldx #src1
  // ldy #src2
  // jsr func
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_x), op1, kAddrModeImplied));
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_y), op2, kAddrModeImplied));
  jsr(g, func);
  if (!dest_in_zero_page) {
    AddReloadPoint(g, tmp);
    // Dest is indirect, copy to it.
    AddressingMode mode = kAddrModeIndirect;
    for (int i = 0; i < Sizeof(node->type); i++) {
       SetIndexReg(g, tmp, dest, i);
       Emit(g, NewInstruction2(W65C02_OP(lda), tmp, ByteConst(g, i), kAddrModeZeroPage));
       Emit(g, NewInstruction2(W65C02_OP(sta), dest, ByteConst(g, i), mode));
       mode = kAddrModeIndirectIndexed;
     }
     dest = tmp;
  }
  return dest;
}

static TargetInstruction* CallArithmeticUnaryRuntime(W65C02Generator* g,
                                                     IRNode* node,
                                                     TargetInstruction* dest,
                                                     TargetInstruction* op,
                                                     Symbol* func) {
  // lda #dest
  // ldx #src1
  // JSR func
  AddReloadPoint(g, op);
  AddReloadPoint(g, dest);
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), dest, kAddrModeImplied));
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_x), op, kAddrModeImplied));
  jsr(g, func);
  return dest;
}

// These macros fill out the arithmetic_runtimes array.  The
// preprocessor magic builds the name of a runtime function from
// an arithmetic opcode, signedness and size.  The functions
// are encoded as offsets into the W65C02Generator struct.
#define ALUFUNC1(op, s, u)                                   \
  {IR_OP(op), true, 1, offsetof(W65C02Generator, u##1)},      \
      {IR_OP(op), false, 1, offsetof(W65C02Generator, s##1)}, \
      {IR_OP(op), true, 2, offsetof(W65C02Generator, u##2)},  \
      {IR_OP(op), false, 2, offsetof(W65C02Generator, s##2)}, \
      {IR_OP(op), true, 4, offsetof(W65C02Generator, u##4)},  \
      {IR_OP(op), false, 4, offsetof(W65C02Generator, s##4)}, \
      {IR_OP(op), true, 8, offsetof(W65C02Generator, u##8)},  \
      {IR_OP(op), false, 8, offsetof(W65C02Generator, s##8)},

#define ALUFUNC2(op, s1, s2, u1, u2)                              \
  {IR_OP(op), true, 1, offsetof(W65C02Generator, u1##1##u2)},      \
      {IR_OP(op), false, 1, offsetof(W65C02Generator, s1##1##s2)}, \
      {IR_OP(op), true, 2, offsetof(W65C02Generator, u1##2##u2)},  \
      {IR_OP(op), false, 2, offsetof(W65C02Generator, s1##2##s2)}, \
      {IR_OP(op), true, 4, offsetof(W65C02Generator, u1##4##u2)},  \
      {IR_OP(op), false, 4, offsetof(W65C02Generator, s1##4##s2)}, \
      {IR_OP(op), true, 8, offsetof(W65C02Generator, u1##8##u2)},  \
      {IR_OP(op), false, 8, offsetof(W65C02Generator, s1##8##s2)},

static struct {
  IROpcode opcode;
  bool is_unsigned;
  int size;
  int func_offset;
} arithmetic_runtimes[] = {
    ALUFUNC1(muli, smul, umul)  // muli -> umul[1,2,4,8] (size is inserted)
    ALUFUNC1(divi, sdiv, udiv) ALUFUNC1(modi, smod, umod)
        ALUFUNC2(i2f, i, tof, ui, tof)
            ALUFUNC2(f2i, ftoi, , ftoui, )
  ALUFUNC2(i2d, i, tof, ui, tof)
      ALUFUNC2(d2i, ftoi, , ftoui, )
  {IR_OP(nop)}};

#undef ALUFUNC1
#undef ALUFUNC2

// Given an opcode, signedness and size, return a pointer to the symbol
// to be used for the arithmetic operation.
static Symbol* RuntimeFunction(W65C02Generator* g, IROpcode opcode,
                               bool is_unsigned, int size) {
  for (int i = 0; arithmetic_runtimes[i].opcode != IR_OP(nop); i++) {
    if (arithmetic_runtimes[i].opcode == opcode &&
        arithmetic_runtimes[i].is_unsigned == is_unsigned &&
        arithmetic_runtimes[i].size == size) {
      char* symaddr = ((char*)g + arithmetic_runtimes[i].func_offset);
      return *(Symbol**)symaddr;
    }
  }
  abort();
  return NULL;
}

// Some arithmetic operations need addresses for their operand and some
// need the values.
static void GetOpInstructions(W65C02Generator* g, IRNode* node,
                              TargetInstruction** ops) {
  for (size_t i = 0; i < node->inputs.length; i++) {
    IRNode* input = node->inputs.value.p[i];
    switch (node->opcode) {
      case IR_OP(adda):
      case IR_OP(addi):
      case IR_OP(subi):
      case IR_OP(suba):
      case IR_OP(lsri):
      case IR_OP(asri):
      case IR_OP(lsli):
      case IR_OP(ori):
      case IR_OP(andi):
      case IR_OP(xori):
      case IR_OP(noti):
      case IR_OP(nota):
      case IR_OP(onescomp):
      case IR_OP(negi):
         ops[i] = GetAddress(g, input, true);
        break;
        
      case IR_OP(addf):
      case IR_OP(addd):
      case IR_OP(subf):
      case IR_OP(subd):
      case IR_OP(muli):
      case IR_OP(divi):
      case IR_OP(modi):
      case IR_OP(mulf):
      case IR_OP(muld):
      case IR_OP(divf):
      case IR_OP(divd):
      case IR_OP(negf):
      case IR_OP(negd):
      case IR_OP(i2f):
      case IR_OP(i2d):
      case IR_OP(f2i):
      case IR_OP(d2i):
      case IR_OP(f2d):
      case IR_OP(d2f):
         ops[i] = Materialize(g, input, Sizeof(input->type), true);
        break;
     
      case IR_OP(movi):
      case IR_OP(movf):
      case IR_OP(movd):
      case IR_OP(mova):
        if (node->dest != NULL && node->outputs.length == 0) {
          // Just an assignment to another node, no copy needed.
          ops[i] = GetLoweredNode(input);
        } else {
          ops[i] = Materialize(g, input, Sizeof(input->type), true);
        }
        break;
      default:
        abort();
    }
  }
}

static void IncrementOnce(W65C02Generator* g, IRNode* node, TargetInstruction* src) {
  int size = Sizeof(node->type);
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, src, src, i);
    if (i > 0) {
      TargetInstruction* label = NewInstruction(W65C02_OP(label), kAddrModeImplied);
      EmitResolvedBranch(g, W65C02_OP(bne), label);
      inc(g, src, i);
      Emit(g, label);
    } else {
      inc(g, src, i);
    }
  }
}

// Increment op1 by op2.  op2 will be a constant.
static TargetInstruction* Increment(W65C02Generator* g, IRNode* node, IRNode* op1, IRNode* op2) {
  // For a 2-byte increment.  If only one iteration, omit loop.
  //  LDX #op2
  // loop:
  //  INC op1+0
  //  BNE xx
  //  INC op1+1
  // xx:
  //  DEX
  //  BNE loop
  int amount = (int)IRIntConstValue(op2);
  if (amount == 0) {
    return NULL;
  }
  TargetInstruction* src = GetAddress(g, op1, true);
  AddReloadPoint(g, src);

  AddressingMode mode = GetAddrMode(src);
  int size = Sizeof(node->type);
  
  if (mode == kAddrModeIndirect || mode == kAddrModeIndirectIndexed) {
    // No INC instructions for these addressing modes.  We need to add 1
    // to the src and store it back.
    return AddSubInteger(g, node, W65C02_OP(adc), W65C02_OP(clc), size, src,
                          src, ByteConst(g, amount));
}
  
  if (amount == 1) {
     IncrementOnce(g, node, src);
  } else {
    ldxi(g, amount);
    TargetInstruction* loop_label = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
    IncrementOnce(g, node, src);
    dex(g);
    EmitResolvedBranch(g, W65C02_OP(bne), loop_label);
  }
  
  TargetSetLoweredNode(node, src);
  return src;
}

static bool IsPowerOf2(int64_t v) {
  return v != 0 && (v & (v - 1)) == 0;
}

// Given a number that is a power of 2, what is the log (base 2) of it.
static int Log2(int64_t v) {
  static const int MultiplyDeBruijnBitPosition2[32] =
  {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return MultiplyDeBruijnBitPosition2[(uint32_t)(v * 0x077CB531U) >> 27];
}

static TargetInstruction* MultiplyBy10(W65C02Generator* g,
                                       IRNode* node,
                                       TargetInstruction* dest,
                                       TargetInstruction* op) {
  Symbol* mul_func = TypeIsUnsigned(node->type) ? g->umul2_10 : g->smul2_10;
  return SetLoweredNode(node, CallArithmeticUnaryRuntime(g, node, dest, op, mul_func));
}

static TargetInstruction* ReduceMultiplyOrDivide(W65C02Generator* g, IRNode* node) {
  TargetInstruction* result = NULL;
  switch (node->opcode) {
    case IR_OP(muli): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) || IRIsConst(op2)) {
        if (IRIsConst(op1) && IRIsConst(op2)) {
          break;
        }
        TargetInstruction* dest = GetDestAddress(g, node, true);
        TypeRecord* type = node->type;
        int size = Sizeof(type);

        if (size == 2 && IRIsConst(op2) && IRIntConstValue(op2) == 10) {
          // We have a special optimized multiply by 10 function.
          return MultiplyBy10(g, node, dest, Materialize(g, op1, 2, true));
        }
        // One is constant, if it's a power of 2 we can create a loop
        // of shifts.
        if (IRIsConst(op1)) {
          int64_t c = ((IRConstant*)op1)->value.ivalue;
          if (c == 0) {
             return Zero(g);
           }
           if (c == 1) {
             return SetLoweredNode(node, GetLoweredNode(op2));
           }
          if (IsPowerOf2(c)) {
            result = SetLoweredNode(node, ConstantShiftOp(g, node, W65C02_OP(asl), W65C02_OP(rol), size, dest, GetAddress(g, op2, true),
                           Log2(c)));
            AddSpillPoint(g, dest);
            return result;

          }
        } else {
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (c == 0) {
            return Zero(g);
          }
          if (c == 1) {
            return SetLoweredNode(node, GetLoweredNode(op1));
          }
          if (IsPowerOf2(c)) {
            result = SetLoweredNode(node, ConstantShiftOp(g, node, W65C02_OP(asl), W65C02_OP(rol), size, dest, GetAddress(g, op1, true),
                            Log2(c)));
            AddSpillPoint(g, dest);
             return result;
          }
        }
      }
      break;
      }
    case IR_OP(divi): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op2)) {
        TargetInstruction* dest = GetDestAddress(g, node, true);
        TypeRecord* type = node->type;
        int size = Sizeof(type);
        bool is_unsigned = type != NULL && TypeIsUnsigned(type);

        // One is constant, if it's a power of 2 we can create a loop
        // of shifts.
        if (IRIsConst(op2)) {
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (IsPowerOf2(c)) {
            if (is_unsigned) {
                return SetLoweredNode(node, ConstantShiftOp(g, node, W65C02_OP(lsr), W65C02_OP(ror), size, dest, GetAddress(g, op1, true),
                                Log2(c)));
            } else {
              result = SetLoweredNode(node, ConstantArithmeticRightShiftOp(g, node, size, dest, GetAddress(g, op1, true),
                                             Log2(c)));
              AddSpillPoint(g, dest);
               return result;
            }
          }
        }
      }
      break;
      }
  case IR_OP(modi):
      // Do this
      break;
    default:
      break;
  }
  return NULL;
}

static bool IsVariableNode(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(externvar):
    case IR_OP(ssavar):
    case IR_OP(phi):
#if 0  // Eh?  What are these doing here?
    case W65C02_OP(ivarreg):
      case W65C02_OP(bvarreg):
      case W65C02_OP(lvarreg):
      case W65C02_OP(xvarreg):
      case W65C02_OP(fvarreg):
      case W65C02_OP(dvarreg):
#endif
      return true;
    default:
      return false;
  }
}

static bool IsSameVariable(W65C02Generator* g, IRNode* node1, IRNode* node2) {
  IRNode* var1 = node1;
  IRNode* var2 = node2;
  if (node1->opcode == IR_OP(phi) || node1->opcode == IR_OP(ssavar)) {
    IRVariable* var = (IRVariable*)node1;
    var1 = FindPooledVariable(g->gen, var->symbol);
  }
  if (node2->opcode == IR_OP(phi) || node2->opcode == IR_OP(ssavar)) {
    IRVariable* var = (IRVariable*)node2;
    var2 = FindPooledVariable(g->gen, var->symbol);
  }
  return var1 == var2;
}

static void LowerExpression(W65C02Generator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return;
  }
  IROpcode op = node->opcode;
  
  // See if we can increment the a variable by a small amount.
  if (op == IR_OP(addi)) {
    IRNode* op1 = node->inputs.value.p[0];
    IRNode* op2 = node->inputs.value.p[1];
    if (IRIsConst(op2) && node->dest != NULL) {
      IRNode* dest = node->dest;
      // Adding a constant less than 4?
      if (IRIntConstValue(op2) < 4) {
        if (op1->opcode == IR_OP(load32)) {
          IRNode* loaded = op1->inputs.value.p[0];
          if (dest->id == loaded->id) {
            // Adding to itself.
            Increment(g, node, loaded, op2);
            return;
          }
        } else if (IsVariableNode(op1) && IsVariableNode(dest)) {
          if (IsSameVariable(g, op1, dest)) {
            Increment(g, node, op1, op2);
            return;
          }
        }
      }
    }
  }
  TargetInstruction* inst = ReduceMultiplyOrDivide(g, node);
  if (inst != NULL) {
    return;
  }
  
  TargetInstruction* ops[2] = {NULL, NULL};
  assert(node->inputs.length < 3);
  GetOpInstructions(g, node, ops);

  TargetInstruction* dest = GetDestAddress(g, node, false);
  TypeRecord* type = node->type;
  int size = Sizeof(type);
  bool is_unsigned = type != NULL && TypeIsUnsigned(type);

  switch (op) {
    case IR_OP(addi):
      inst = AddSubInteger(g, node, W65C02_OP(adc), W65C02_OP(clc), size, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(adda):
      inst = AddSubInteger(g, node, W65C02_OP(adc), W65C02_OP(clc), 2, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(addf):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fadd);
      break;
    case IR_OP(addd):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fadd);
      break;

    case IR_OP(subi):
      inst = AddSubInteger(g, node, W65C02_OP(sbc), W65C02_OP(sec), size, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(suba):
      inst = AddSubInteger(g, node, W65C02_OP(sbc), W65C02_OP(sec), 2, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(subf):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fsub);
      break;
    case IR_OP(subd):
      inst =
           CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fsub);
      break;

    case IR_OP(muli):
    case IR_OP(divi):
    case IR_OP(modi):
      inst = CallArithmeticBinaryRuntime(
          g, node, dest, ops[0], ops[1],
          RuntimeFunction(g, op, is_unsigned, size));
      break;

    case IR_OP(mulf):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fmul);
      break;
    case IR_OP(muld):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fmul);
      break;
    case IR_OP(divf):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fdiv);
      break;
    case IR_OP(divd):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fdiv);
      break;

    case IR_OP(lsri):
      inst = ShiftOp(g, node, W65C02_OP(lsr), W65C02_OP(ror), size, dest, ops[0],
                     ops[1]);
      break;
    case IR_OP(asri): {
      // Copy sign bit to carry bit.
      // TargetInstruction* carry = NewInstruction1(W65C02_OP(cmp),
      // GetIntConstant(g, node, kTargetType8Bit, 0x80), kAddrModeImmediate);
      inst = ArithmeticRightShiftOp(g, node, size, dest, ops[0],
                                    ops[1]);
      break;
    }
    case IR_OP(lsli):
      inst = ShiftOp(g, node, W65C02_OP(asl), W65C02_OP(rol), size, dest, ops[0],
                     ops[1]);
      break;

    case IR_OP(ori):
      inst =
          SimpleIntegerOp(g, node, W65C02_OP(ora), size, dest, ops[0], ops[1]);
      break;

    case IR_OP(andi):
      inst = AndOp(g, node, size, dest, ops[0], ops[1]);
      //inst =
      //    SimpleIntegerOp(g, node, W65C02_OP(and), size, dest, ops[0], ops[1]);
      break;
    case IR_OP(xori):
      inst =
          SimpleIntegerOp(g, node, W65C02_OP(eor), size, dest, ops[0], ops[1]);
      break;

    case IR_OP(noti):
    case IR_OP(nota):
      inst = UnaryNotOp(g, node->inputs.value.p[0], dest, ops[0], size);
      break;
    case IR_OP(onescomp):
      inst = OnesComplement(g, node->inputs.value.p[0], dest, ops[0], size);
      break;
    case IR_OP(negi):
      inst = NegateInteger(g, node->inputs.value.p[0], dest, ops[0], size);
      break;

    case IR_OP(negf):
    case IR_OP(negd):
      inst = NegateFloatingPoint(g, node->inputs.value.p[0], dest, ops[0], size);
//      inst = CallArithmeticUnaryRuntime(
//          g, node, dest, ops[0], g->fneg);
      break;
//    case IR_OP(negd):
//      inst = CallArithmeticUnaryRuntime(
//          g, node, dest, ops[0], g->fneg);
//      break;

    case IR_OP(i2f):
    case IR_OP(i2d): {
      IRNode* i = node->inputs.value.p[0];
      size = Sizeof(i->type);
      is_unsigned = TypeIsUnsigned(i->type);
      inst = CallArithmeticUnaryRuntime(
          g, node, dest, ops[0], RuntimeFunction(g, op, is_unsigned, size));
      break;
      }
    case IR_OP(f2i):
    case IR_OP(d2i):
      inst = CallArithmeticUnaryRuntime(
          g, node, dest, ops[0], RuntimeFunction(g, op, is_unsigned, size));
      break;
    case IR_OP(f2d):
    case IR_OP(d2f):
      abort();
      break;
      
    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova): {
      // inst = GetDestAddress(g, node, true);
      inst = dest;
      TargetInstruction* src = ops[0];
      // AddReloadPoint(g, src);

      Copy(g, dest, src, 0, 0, Sizeof(node->type), GetAddrMode(dest), GetAddrMode(src));
      break;
    }
//    case IR_OP(rmovi):
//    case IR_OP(rmovf):
//    case IR_OP(rmovd):
//    case IR_OP(rmova): {
//      IRNode* move_dest = node->inputs.value.p[0];
//      if (move_dest->outputs.length == 1) {
//        // The only output from the destination of the rmov is the
//        // rmov itself.  We can omit the rmov.
//        goto done;
//      }
//      inst = GetDestAddress(g, move_dest, false);
//      TargetInstruction* src = ops[1];
//      AddReloadPoint(g, src);
//      AddReloadPoint(g, dest);
//
//      int size = Sizeof(node->type);
//      for (int i = 0; i < size; i++) {
//        SetIndexReg(g, ops[1], inst, i);
//        lda(g, src, i);
//        sta(g, dest, i);
//      }
//      break;
//    }
    case IR_OP(tmp):
      inst = TempRegister(g, node->type, size);
      AddSpillPoint(g, dest);
      dest = inst;
      break;
    default:
      abort();
      assert(false);
      return;
  }

  TargetUpdateOperandUsers(inst);
  SetLoweredNode(node, inst);
  AddSpillPoint(g, dest);
  Emit(g, inst);
}

static void CompareEqualZero(W65C02Generator* g, IRNode* value_node,
                             IRNode* target_node) {
  // For 2 byte:
  //  lda byte1
  //  ORA byte2
  //  BEQ true_label

  int size = Sizeof(value_node->type);
  TargetInstruction* value = GetAddress(g, value_node, true);
  AddReloadPoint(g, value);

  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
  }
  
  cmpi(g, 0);
  TargetInstruction* bra = EmitBranch(g, W65C02_OP(beq), target_node);
  bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareEqualInteger(W65C02Generator* g, IRNode* cmp_node,
                                IRNode* target_node) {
  // For 2 byte:
  //  lda byte1
  //  CMP value1
  //  BNE false_label
  //  lda byte2
  //  CMP value2
  //  BEQ true_label
  // false_label:

  if (IRIsZero(cmp_node->inputs.value.p[1])) {
    // x == 0
    CompareEqualZero(g, cmp_node->inputs.value.p[0], target_node);
    return;
  }
  if (IRIsZero(cmp_node->inputs.value.p[0])) {
    // 0 == x
    CompareEqualZero(g, cmp_node->inputs.value.p[1], target_node);
    return;
  }

  int size = Sizeof(((IRNode*)(cmp_node->inputs.value.p[0]))->type);
  TargetInstruction* value1 = GetAddress(g, cmp_node->inputs.value.p[0], true);
  TargetInstruction* value2 = GetAddress(g, cmp_node->inputs.value.p[1], true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);

  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    if (i < (size - 1)) {
      EmitResolvedBranch(g, W65C02_OP(bne), false_label);
    } else {
      EmitBranch(g, W65C02_OP(beq), target_node);
    }
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(W65C02_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareNotEqualZero(W65C02Generator* g, IRNode* value_node,
                                IRNode* target_node) {
  // For 2 byte:
  //  lda byte1
  //  ORA byte2
  //  BNE true_label

  int size = Sizeof(value_node->type);
  TargetInstruction* value = GetAddress(g, value_node, true);
  AddReloadPoint(g, value);

  // TODO optimize for size > 2
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
  }
  TargetInstruction* bra = EmitBranch(g, W65C02_OP(bne), target_node);
  bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareNotEqualInteger(W65C02Generator* g, IRNode* cmp_node,
                                   IRNode* target_node) {
  // For 2 byte:
  //  lda byte1
  //  CMP value1
  //  BNE true_label
  //  lda byte2
  //  CMP value2
  //  BNE true_label
  // false_label:

  if (IRIsZero(cmp_node->inputs.value.p[1])) {
    // x != 0
    CompareNotEqualZero(g, cmp_node->inputs.value.p[0], target_node);
    return;
  }
  if (IRIsZero(cmp_node->inputs.value.p[0])) {
    // 0 != x
    CompareNotEqualZero(g, cmp_node->inputs.value.p[1], target_node);
    return;
  }

  int size = Sizeof(((IRNode*)(cmp_node->inputs.value.p[0]))->type);
  TargetInstruction* value1 = GetAddress(g, cmp_node->inputs.value.p[0], true);
  TargetInstruction* value2 = GetAddress(g, cmp_node->inputs.value.p[1], true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);

  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  for (int i = 0; i < size; i++) {
    if (!Is65c02() || i > 0) {
      SetIndexReg(g, value1, value2, i);
    }
    lda(g, value1, i);
    cmp(g, value2, i);
    EmitBranch(g, W65C02_OP(bne), target_node);
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(W65C02_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareLessUnsignedInteger(W65C02Generator* g, IRNode* lhs_node,
                                       IRNode* rhs_node, IRNode* target_node) {
  // For 2 byte:
  //  lda byte2
  //  CMP value2
  //  BCC true_label
  //  BNE false_label
  //  lda byte1
  //  CMP value1
  //  BCC true_label
  // false_label:

  if (IRIsZero(rhs_node) && ((lhs_node->flags & kIRFakeUnsigned) == 0)) {
    // x < 0
    // Always false.
    return;
  }
  if (IRIsZero(lhs_node)) {
    // 0 < x
    // Same as x > 0, same as x != 0
    CompareNotEqualZero(g, rhs_node, target_node);
    return;
  }

  int size = Sizeof(lhs_node->type);
  TargetInstruction* value1 = GetAddress(g, lhs_node, true);
  TargetInstruction* value2 = GetAddress(g, rhs_node, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);

  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  for (int i = size - 1; i >= 0; i--) {
    SetIndexRegDown(g, value1, value2, i, size);
    lda(g, value1, i);
    cmp(g, value2, i)->flags |= k6502GeneratesFlags;
    EmitBranch(g, W65C02_OP(bcc), target_node);
    if (i > 0) {
      EmitResolvedBranch(g, W65C02_OP(bne), false_label);
    }
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(W65C02_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareGreaterOrEqualUnsignedInteger(W65C02Generator* g,
                                                 IRNode* lhs_node,
                                                 IRNode* rhs_node,
                                                 IRNode* target_node) {
  // For 4 byte:
  //  lda byte4
  //  CMP value4
  //  BCC false_label
  //  BNE true_label
  //  lda byte3
  //  CMP value3
  //  BCC false_label
  //  BNE true_label
  //  lda byte2
  //  CMP value2
  //  BCC false_label
  //  BNE true_label
  //  lda byte1
  //  CMP value1
  //  BCS true_label
  // false_label:

  if (IRIsZero(rhs_node)) {
    // x >= 0
    // Always true.
    TargetInstruction* lab = EmitBranch(g, Is65c02() ? W65C02_OP(bra) : W65C02_OP(jmp), target_node);
    lab->flags |= k6502BlockEnd | k6502InstIsCondBranch;
    return;
  }
  if (IRIsZero(lhs_node)) {
    // 0 >= x
    // Same as x <= 0, same as x == 0 for unsigned values.
    CompareEqualZero(g, rhs_node, target_node);
    return;
  }

  int size = Sizeof(lhs_node->type);
  TargetInstruction* value1 = GetAddress(g, lhs_node, true);
  TargetInstruction* value2 = GetAddress(g, rhs_node, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);

  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  for (int i = size - 1; i >= 0; i--) {
    SetIndexRegDown(g, value1, value2, i, size);

    lda(g, value1, i);
    cmp(g, value2, i)->flags |= k6502GeneratesFlags;
    if (i == 0) {
      EmitBranch(g, W65C02_OP(bcs), target_node);
    } else {
      EmitResolvedBranch(g, W65C02_OP(bcc), false_label);
      EmitBranch(g, W65C02_OP(bne), target_node);
    }
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(W65C02_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareFloatingExpression(W65C02Generator* g, IRNode* lhs, IRNode* rhs,
                                      TargetInstruction* dest, int size,
                                      Symbol* func) {
  // lda #dest
  // ldx #src1
  // ldy #src2
  // jsr func
  // Put result (1 or 0) in dest byte.
  TargetInstruction* value1 = Materialize(g, lhs, size, true);
  TargetInstruction* value2 = Materialize(g, rhs, size, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), dest, kAddrModeImplied));
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_x), value1, kAddrModeImplied));
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_y), value2, kAddrModeImplied));
  jsr(g, func);
}

static COMPILER_UNUSED void CompareFloatingPoint(W65C02Generator* g,
                                                IRNode* node,
                                                TargetInstruction* dest,
                                                IRNode* lhs,
                                                IRNode* rhs) {
  int size = 4;
  switch (node->opcode) {
  case IR_OP(cmpeqf):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpeqf);
    break;
  case IR_OP(cmpnef):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpnef);
    break;
  case IR_OP(cmpltf):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpltf);
    break;
  case IR_OP(cmplef):
    CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpgef);
    break;
  case IR_OP(cmpgtf):
    CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpltf);
    break;
  case IR_OP(cmpgef):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpgef);
    break;
  case IR_OP(cmpeqd):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpeqf);
    break;
  case IR_OP(cmpned):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpnef);
    break;
  case IR_OP(cmpltd):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpltf);
    break;
  case IR_OP(cmpled):
    CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpgef);
    break;
  case IR_OP(cmpgtd):
    CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpltf);
    break;
  case IR_OP(cmpged):
    CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpgef);
    break;
    default:
      abort();
  }
}

// Subtract lhs - rhs into flags (signed).  N=1 means lhs < rhs.
static void CompareSignedSubtract(W65C02Generator* g, TargetInstruction* value1,
                                  TargetInstruction* value2, int size) {
  TargetInstruction* skip_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  if (size == 1) {
    sec(g);
  }
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    if (i == 0 && size > 1) {
      // The low-byte CMP sets the carry that the following SBC (next byte up)
      // consumes.  Mark it so the CMP #0 peephole does not delete it: that
      // peephole only reasons about the Z/N flags (which a preceding LDA also
      // sets) but CMP also forces carry, which LDA does not.
      cmp(g, value2, i)->flags |= k6502GeneratesFlags;
    } else {
      sbc(g, value2, i);
    }
  }
  EmitResolvedBranch(g, W65C02_OP(bvc), skip_label);
  eori(g, 0x80);
  Emit(g, skip_label);
}

// Compares 2 signed integers for < or >=.  Branches to target
static void CompareSignedInteger(W65C02Generator* g,
                                           IRNode* lhs_node,
                                           IRNode* rhs_node,
                                           IRNode* target_node,
                                           bool less_than) {
  TargetInstruction* value1 = GetAddress(g, lhs_node, true);
  TargetInstruction* value2 = GetAddress(g, rhs_node, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  CompareSignedSubtract(g, value1, value2, Sizeof(lhs_node->type));
  TargetInstruction* bra =
      EmitBranch(g, less_than ? W65C02_OP(bmi) : W65C02_OP(bpl), target_node);
  bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

// Branches to target if lhs <= rhs (signed).
static void CompareSignedLessOrEqual(W65C02Generator* g, IRNode* lhs_node,
                                     IRNode* rhs_node, IRNode* target_node) {
  TargetInstruction* value1 = GetAddress(g, lhs_node, true);
  TargetInstruction* value2 = GetAddress(g, rhs_node, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  CompareSignedSubtract(g, value1, value2, Sizeof(lhs_node->type));
  TargetInstruction* bra = EmitBranch(g, W65C02_OP(bmi), target_node);
  bra->flags |= k6502InstIsCondBranch;
  bra = EmitBranch(g, W65C02_OP(beq), target_node);
  bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

// If the branch comes from a comparison node we combine the comparison
// with the branch.  If it comes from another node we generate an equality
// comparison of that node with zero.
static void LowerConditionalBranch(W65C02Generator* g,
                                                 IRNode* node) {
  bool reverse = node->opcode == IR_OP(bfalse);

  assert(node->inputs.length == 2);
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];

  IRNode* input = node->inputs.value.p[0];
  bool compare_with_zero = !IRIsComparison(input);
  if (!compare_with_zero && HasLoweredNode(input)) {
    TargetInstruction* comp = GetLoweredNode(input);
    compare_with_zero = (comp->flags & k6502ComparisonGenerated) != 0;
  }
  if (compare_with_zero) {
    if (IRIsConst(input)) {
      // Compare constant.
      // If constant is zero, BRA is comparing false
      // otherwise, BRA is comparing true.
      int64_t cval = IRIntConstValue(input);
      TargetInstruction* bra = NULL;
      if (cval == 0) {
        if (node->opcode == IR_OP(bfalse)) {
          bra = EmitBranch(g, W65C02_OP(bra), target_node);
        }
      } else {
        if (node->opcode == IR_OP(btrue)) {
          bra = EmitBranch(g, W65C02_OP(bra), target_node);
        }
      }
      if (bra != NULL) {
        bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
      }
   } else {
      // Generate equality comparison with zero.
      if (reverse) {
        CompareEqualZero(g, expr, target_node);
      } else {
        CompareNotEqualZero(g, expr, target_node);
      }
    }
    return;
  }
  IRNode* lhs = input->inputs.value.p[0];
  IRNode* rhs = input->inputs.value.p[1];
  bool is_unsigned = TypeIsUnsigned(lhs->type);
  
  switch (expr->opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
    case IR_OP(cmpeqf):
    case IR_OP(cmpeqd):
      if (reverse) {
          CompareNotEqualInteger(g, input, target_node);
       } else {
          CompareEqualInteger(g, input, target_node);
      }
      break;
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
    case IR_OP(cmpnef):
    case IR_OP(cmpned):
      if (reverse) {
        CompareEqualInteger(g, input, target_node);
      } else {
        CompareNotEqualInteger(g, input, target_node);
      }
      break;
      
    case IR_OP(cmplti):
    case IR_OP(cmplta):
    case IR_OP(cmpltf):
    case IR_OP(cmpltd):
      if (reverse) {
        if (is_unsigned) {
          CompareGreaterOrEqualUnsignedInteger(
            g, lhs, rhs, target_node);
        } else {
          CompareSignedInteger(g, lhs, rhs, target_node, false);
        }
      } else {
        if (is_unsigned) {
            CompareLessUnsignedInteger(g, lhs,
                                   rhs, target_node);
          } else {
            CompareSignedInteger(g, lhs, rhs, target_node, true);
        }
      }
      break;
    case IR_OP(cmplei):
    case IR_OP(cmplea):
    case IR_OP(cmplef):
    case IR_OP(cmpled):
      if (reverse) {
        // > comparison
        if (is_unsigned) {
          CompareLessUnsignedInteger(g, rhs,
                                   lhs, target_node);
        } else {
            CompareSignedInteger(g, rhs, lhs, target_node, true);
        }
      } else {
        if (is_unsigned) {
          CompareGreaterOrEqualUnsignedInteger(
            g, rhs, lhs, target_node);
          } else {
            CompareSignedLessOrEqual(g, lhs, rhs, target_node);
        }
      }
      break;
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgtd):
      if (reverse) {
        if (is_unsigned) {
          CompareGreaterOrEqualUnsignedInteger(
            g, rhs, lhs, target_node);
          } else {
            CompareSignedInteger(g, rhs, lhs, target_node, false);
          }
      } else {
        if (is_unsigned) {
          CompareLessUnsignedInteger(g, rhs, lhs, target_node);
        } else {
          CompareSignedInteger(g, rhs, lhs, target_node, true);
        }
      }
      break;
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
    case IR_OP(cmpgef):
    case IR_OP(cmpged):
      if (reverse) {
        if (is_unsigned) {
          CompareLessUnsignedInteger(g, lhs, rhs, target_node);
        } else {
          CompareSignedInteger(g, lhs, rhs, target_node, true);
        }
      } else {
        if (is_unsigned) {
          CompareGreaterOrEqualUnsignedInteger(
            g, lhs, rhs, target_node);
        } else {
          CompareSignedInteger(g, lhs, rhs, target_node, false);
        }
      }
      break;
#if 0
    case IR_OP(cmpeqf):
    case IR_OP(cmpnef):
    case IR_OP(cmpltf):
    case IR_OP(cmplef):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgef):
    case IR_OP(cmpeqd):
    case IR_OP(cmpned):
    case IR_OP(cmpltd):
    case IR_OP(cmpled):
    case IR_OP(cmpgtd):
    case IR_OP(cmpged): {
      TargetInstruction* dest = TempRegister(g, node->type, 1);
      CompareFloatingPoint(g, expr, dest, lhs, rhs);
      lda(g, dest, 0);
      TargetInstruction* bra = EmitBranch(g, reverse ? W65C02_OP(beq) : W65C02_OP(bne), target_node);
      bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
      break;
    }
#endif
    default:
      abort();
  }

}

static void LowerBranch(W65C02Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* target_node = node->inputs.value.p[0];

  W65C02Opcode opcode = Is65c02() ? W65C02_OP(bra) : W65C02_OP(jmp);
  if ((node->flags & kIRJumpTableBranch) != 0) {
    // Need all jump table entries to be the same length.
    opcode = W65C02_OP(jumptable);
  }
  TargetInstruction* b = EmitBranch(g, opcode, target_node);
  b->flags |= k6502BlockEnd;
}

// Computed branch.  This is followed by a series of jumptable entries, each
// of which is 2 bytes long.
// LDA #X
// JSR __jump_tableN
// Table starts here.
static void LowerComputedBranch(W65C02Generator* g, IRNode* node) {
  TargetInstruction* byte_offset = Materialize(g, node->inputs.value.p[0], -1, true);
  ldazi(g, byte_offset, 0);
  TargetInstruction* jump;
  switch (node->type->size) {
    case 1:
      jump = jsr(g, g->jump_table1);
      break;
    case 2:
      jump = jsr(g, g->jump_table2);
      break;
    case 4:
      jump = jsr(g, g->jump_table4);
      break;
    case 8:
      jump = jsr(g, g->jump_table8);
      break;
    default:
      abort();
  }
  jump->flags |= TARGET_INST_TABLE_JUMP | k6502BlockEnd;
  SetLoweredNode(node, jump);
}

static void LowerLabel(W65C02Generator* g, IRNode* label) {
  TargetInstruction* inst =
      Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
  label->data.ptr = inst;
  inst->flags |= k6502BlockStart;
  ApplyFixups(g, label);
}

static void LowerNamedLabel(W65C02Generator* rv, IRNode* label) {
  IRNamedLabel* n = (IRNamedLabel*)label;
  TargetInstruction* inst = Emit(rv, TargetNewNamedLabel(n->name));
  inst->flags |= k6502BlockStart;
  label->data.ptr = inst;
}

static int GetStartIndex(IRNode* node, int i) {
  int index = 0;
  while (i < node->inputs.length) {
    index += (int)IRIntConstValue(node->inputs.value.p[i]);
    i++;
  }
  return index;
}

static TargetInstruction* LoadFromRegVariable(W65C02Generator* g, IRNode* load, IRNode* var, int size) {
  TargetInstruction* result;
  TargetInstruction* src = GetLoweredNode(var);
  if (load->dest == NULL && load->outputs.length == 1) {
    AddSpillPoint(g, src);
    return SetLoweredNode(load, src);
  }
  if (load->dest == NULL) {
    result = TempRegister(g, load->type, size);
  } else {
    result = GetAddress(g, load->dest, true);
  }
  Copy(g, result, src, 0, 0, size, GetAddrMode(result), GetAddrMode(src));
  AddSpillPoint(g, result);
  return SetLoweredNode(load, result);
}

// Load from a stack variable.
static TargetInstruction* LoadFromVariable(W65C02Generator* g, IRNode* load, IRNode* var, bool is_arg, int size) {
  TargetInstruction* inst = GetLoweredNode(var);
  switch ((W65C02Opcode)inst->opcode) {
      case W65C02_OP(ivarreg):
      case W65C02_OP(bvarreg):
      case W65C02_OP(lvarreg):
      case W65C02_OP(xvarreg):
      case W65C02_OP(fvarreg):
      case W65C02_OP(dvarreg):
        return LoadFromRegVariable(g, load, var, size);
    default:
      break;
  }
  AddReloadPoint(g, inst);
  TargetInstruction* result = NULL;
  int offset;

  load = IgnoreCasts(load);
  
 
  // Variable or argument.  Load value using one of the runtime helper
  // functions.
  offset = (int)TargetIntValue(inst->operand[0]);
  result = TempRegister(g, load->type, size);
  bool highzero = offset < 256;
  // Macro instruction, expands to:
  // lda #dest_addr
  // ldx #offset lo
  // ldy #offset hi (removed for single byte case)
  // JSR __var_value[b] (or arg_value[b])
  W65C02Opcode op;
  AddressingMode mode = kAddrModeZeroPage;
  if (is_arg) {
    // Argument.
    if (TypeIsStructOrUnion(load->type)) {
      // Address.
      op = offset >= 256 ? W65C02_OP(arg_addrb) : W65C02_OP(arg_addr);
    } else {
      op = ArgValueFunction(size, highzero);
    }
  } else {
    // Variable.
    if (TypeIsStructOrUnion(load->type) || TypeIsArray(load->type)) {
      // Address.
      op = offset >= 256 ? W65C02_OP(var_addrb) : W65C02_OP(var_addr);
      mode = kAddrModeIndirect;
    } else {
      op = VarValueFunction(size, highzero);
    }
  }

  Emit(g,
        NewInstruction2(op, result,
                        inst,
                        mode));
 
  AddSpillPoint(g, result);
  if (load->dest != NULL) {
    // We have a location to put the result.
    TargetInstruction* dest = GetAddress(g, load->dest, true);
    Copy(g, dest, result, 0, 0, size, GetAddrMode(dest), kAddrModeZeroPage);
    AddSpillPoint(g, dest);
  }
  return SetLoweredNode(load, result);
}

static bool IsLoweredVariableStorage(IRNode* node) {
  TargetInstruction* inst = GetLoweredNode(node);
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(localvar):
    case W65C02_OP(argument):
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
      return true;
    default:
      return false;
  }
}


static TargetInstruction* LoadFromStaticVariable(W65C02Generator* g, IRNode* load, IRNode* var, int size) {
  TargetInstruction* result;
  if (load->dest == NULL && load->outputs.length == 1 && IRIsVariable(var)) {
    result = GetLoweredNode(var);
  } else {
    TargetInstruction* src = Materialize(g, var, size, false);
    result = GetDestAddress(g, load, false);
    Copy(g, result, src, 0, 0, size, GetAddrMode(result), GetAddrMode(src));
    AddSpillPoint(g, result);
  }
  return SetLoweredNode(load, result);
}

static TargetInstruction* LoadIndirect(W65C02Generator* g, IRNode* load, IRNode* var, int size, int start_index) {
  TargetInstruction* dest;
  if (load->dest != NULL) {
    LowerIRNode(g, load->dest);
    dest = GetAddress(g, load->dest, true);
  } else {
    dest = TempRegister(g, load->type, size);
  }
  TargetInstruction* src = Materialize(g, var, size, true);
  AddressingMode src_mode = kAddrModeIndirectIndexed;
  if (dest == src) {
    // On 6502, because each byte is loaded independently, we can't have the
    // destination and src be the same.
    TargetInstruction* tmp = TempRegister(g, var->type, size);
    Copy(g, tmp, src, start_index, 0, size, kAddrModeZeroPage, kAddrModeIndirectIndexed);
    src = tmp;
    src_mode = kAddrModeZeroPage;
    start_index = 0;
 }
  Copy(g, dest, src, start_index, 0, size, GetAddrMode(dest), src_mode);
  AddSpillPoint(g, dest);
  return SetLoweredNode(load, dest);
}

static void LowerLoad(W65C02Generator* g, IRNode* node) {
  int size = 2;
  int start_index = GetStartIndex(node, 1);
  if (!TypeIsStructOrUnion(node->type) && !TypeIsArray(node->type)) {
    switch (node->opcode) {
      case IR_OP(loadu8):
      case IR_OP(load8):
        size = 1;
        break;
      case IR_OP(loadu16):
      case IR_OP(loada):
       case IR_OP(load16):
         size = 2;
         break;
      case IR_OP(loadu32):
      case IR_OP(loadf):
      case IR_OP(loadd):
      case IR_OP(load32):
        size = 4;
        break;
      case IR_OP(load64):
         size = 8;
         break;
      default:
        break;
    }
  }
  IRNode* src_node = node->inputs.value.p[0];
  if (node->inputs.length > 1) {
    // Load indirect with offset.
    LoadIndirect(g, node, src_node, size, start_index);
    return;
  }
  bool is_arg = true;
  if (src_node->opcode == IR_OP(tempvar)) {
    if (IsLoweredVariableStorage(src_node)) {
      LoadFromVariable(g, node, src_node, false, size);
    } else {
      LoadIndirect(g, node, src_node, size, start_index);
    }
    return;
  }
  switch (src_node->opcode) {
    case IR_OP(localvar):
      is_arg = false;
    case IR_OP(argument):
      LoadFromVariable(g, node, src_node, is_arg, size);
      return;
    case IR_OP(staticvar):
    case IR_OP(externvar):
      LoadFromStaticVariable(g, node, src_node, size);
      return;
    default:
      // Loading from an expression.
      LoadIndirect(g, node, src_node, size, start_index);
      return;
  }
  
#if 0
  int src_start_index = GetStartIndex(node, 1);
  addr_node = IgnoreCasts(addr_node);
  
  bool load_from_regvar = false;
  switch ((W65C02Opcode)GetLoweredNode(addr_node)->opcode) {
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
      load_from_regvar = true;
    break;
    default:
      break;
  }

  if (load_from_regvar) {
    TargetInstruction* src = GetLoweredNode(addr_node);
    bool need_copy = true;
    // If we have only output and we're loading from a variable, we can
    // just use the variable itself instead of making a copy.
    if (IRIsVariable(addr_node) && node->outputs.length == 1 && node->dest == NULL) {
      need_copy = false;
    }
    if (!need_copy) {
      AddSpillPoint(g, src);
      return SetLoweredNode(node, src);
    }
    int size = Sizeof(node->type);
    AddressingMode src_mode = GetAddrMode(src);
    if (!IRIsVariable(addr_node)) {
      src_mode = kAddrModeIndirectIndexed;
    }
    TargetInstruction* dest = GetDestAddress(g, node, false);
    Copy(g, dest, src, src_start_index, 0, size, GetAddrMode(dest), src_mode);
    AddSpillPoint(g, dest);
    return SetLoweredNode(node, dest);
  }
  
  TargetInstruction* dest = GetDestAddress(g, node, false);
  if (IRIsVariable(addr_node) && GetAddrMode(dest) == kAddrModeZeroPage && !TypeIsStructOrUnion(addr_node->type)) {
    // Loading from a variable into a reg, use var_value or arg_value instead
    // of splitting into a var_addr and instructions.
    return SetLoweredNode(node, Materialize(g, addr_node));
  }
  
  TargetInstruction* src = GetAddress(g, addr_node);
  AddressingMode src_mode = GetAddrMode(src);
  if (IsExpression(src)) {
    src_mode = src_start_index == 0 ? kAddrModeIndirect : kAddrModeIndirectIndexed;
  }
  AddressingMode dest_mode = DestInZeroPage(dest) ?  GetAddrMode(dest) : kAddrModeIndirect;

  int size = Sizeof(node->type);
  Copy(g, dest, src, src_start_index, 0, size, dest_mode, src_mode);
#if 0
  int j = start_index;
  for (int i = 0; i < Sizeof(node->type); i++, j++) {
    if (src_mode == kAddrModeIndirectIndexed) {
      ldyi(g, j);
    }
    Emit(g, NewInstruction2(W65C02_OP(lda), src, ByteConst(g, i), src_mode));
    if (dest_mode == kAddrModeIndirectIndexed && i != j) {
      ldyi(g, i);
    }
    Emit(g, NewInstruction2(W65C02_OP(sta), dest, ByteConst(g, i), dest_mode));
    if (dest_mode == kAddrModeIndirect) {
      dest_mode = kAddrModeIndirectIndexed;
    }
    if (src_mode == kAddrModeIndirect) {
      src_mode = kAddrModeIndirectIndexed;
    }
  }
#endif
  AddSpillPoint(g, dest);

  SetLoweredNode(node, dest);
  return dest;
#endif
}

static TargetInstruction* StoreIntoRegVariable(W65C02Generator* g, IRNode* store, IRNode* dest_node, IRNode* src_node, int size) {
  TargetInstruction* src = Materialize(g, src_node, size, false);
  TargetInstruction* dest = GetAddress(g, dest_node, false);
  Copy(g, dest, src, 0, 0, size, GetAddrMode(dest), GetAddrMode(src));
  return SetLoweredNode(store, src);
}

static TargetInstruction* StoreIntoVariable(W65C02Generator* g, IRNode* store, IRNode* dest_node, IRNode* src_node, bool is_arg, int size) {
  TargetInstruction* dest = GetLoweredNode(dest_node);
  switch ((W65C02Opcode)dest->opcode) {
      case W65C02_OP(ivarreg):
      case W65C02_OP(bvarreg):
      case W65C02_OP(lvarreg):
      case W65C02_OP(xvarreg):
      case W65C02_OP(fvarreg):
      case W65C02_OP(dvarreg):
        return StoreIntoRegVariable(g, store, dest_node, src_node, size);
    default:
      break;
  }
  
  TargetInstruction* src = Materialize(g, src_node, size, true);
  int offset = (int)TargetIntValue(dest->operand[0]);
  bool highzero = offset < 256;
  Symbol* func;

  if (IsNRVONode(dest_node)) {
    // This is an Named RVO variable.  It's address is the same
    // as the structreturn address.
    assert(g->struct_return_inst != NULL);
    dest = g->struct_return_inst;
  

    // var_addr is the address of the variable.
    // lda src+0
    // sta (var_addr)
    // ldy #1 or INY
    // lda src+1
    // sta (var_addr), Y
    Copy(g, dest, src, 0, 0, size, kAddrModeIndirectIndexed, GetAddrMode(src));
  } else {
    bool is_zero = TargetIsZero(src);
    if (is_zero) {
      // Storing zero.
      switch (size) {
         case 1:
           if (highzero) {
             func = is_arg ? g->zero_arg_value1 : g->zero_var_value1;
           } else {
             func = is_arg ? g->zero_arg_value1b : g->zero_var_value1b;
           }
           break;
         case 2:
           if (highzero) {
             func = is_arg ? g->zero_arg_value2 : g->zero_var_value2;
           } else {
             func = is_arg ? g->zero_arg_value2b : g->zero_var_value2b;
           }
           break;
         case 4:
           if (highzero) {
             func = is_arg ? g->zero_arg_value4 : g->zero_var_value4;
           } else {
             func = is_arg ? g->zero_arg_value4b : g->zero_var_value4b;
           }
           break;
         case 8:
           if (highzero) {
             func = is_arg ? g->zero_arg_value8 : g->zero_var_value8;
           } else {
               func = is_arg ? g->zero_arg_value8b : g->zero_var_value8b;
           }
            break;
         default:
           abort();
           break;
       }
    } else {
      switch (size) {
         case 1:
           if (highzero) {
             func = is_arg ? g->set_arg_value1 : g->set_var_value1;
           } else {
             func = is_arg ? g->set_arg_value1b : g->set_var_value1b;
           }
           break;
         case 2:
           if (highzero) {
             func = is_arg ? g->set_arg_value2 : g->set_var_value2;
           } else {
             func = is_arg ? g->set_arg_value2b : g->set_var_value2b;
           }
           break;
         case 4:
           if (highzero) {
             func = is_arg ? g->set_arg_value4 : g->set_var_value4;
           } else {
             func = is_arg ? g->set_arg_value4b : g->set_var_value4b;
           }
           break;
         case 8:
           if (highzero) {
             func = is_arg ? g->set_arg_value8 : g->set_var_value8;
           } else {
               func = is_arg ? g->set_arg_value8b : g->set_var_value8b;
           }
             break;
         default:
           abort();
           break;
       }
    }
    
    // lda #src (not for zero)
    // ldx #var addr lo
    // ldy #var addr hi (omitted for hi == 0)
    // jsr func
    if (!is_zero) {
      Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), src, kAddrModeImplied));
    }
    ldxi(g, offset & 0xff);
    if (!highzero) {
      ldyi(g, (offset >> 8) & 0xff);
    }
    jsr(g, func);
  }
  return SetLoweredNode(store, src);
}


static TargetInstruction* StoreIntoStaticVariable(W65C02Generator* g, IRNode* store, IRNode* dest_node, IRNode* src_node, int size) {
  TargetInstruction* src = GetAddress(g,src_node, false);
  TargetInstruction* dest = GetAddress(g,dest_node, false);
  Copy(g, dest, src, 0, 0, size, GetAddrMode(dest), GetAddrMode(src));
  return SetLoweredNode(store, src);
}


static TargetInstruction* StoreIndirect(W65C02Generator* g, IRNode* store, IRNode* dest_node, IRNode* src_node, int size, int start_index) {
  TargetInstruction* src = Materialize(g, src_node, size, false);
  TargetInstruction* dest = Materialize(g, dest_node, size, true);
  Copy(g, dest, src, 0, start_index, size, kAddrModeIndirectIndexed, GetAddrMode(src));
  return SetLoweredNode(store, src);
}

static void LowerStore(W65C02Generator* g, IRNode* node) {
  assert(node->inputs.length >= 2);

  // The value to store is the second operand.
  IRNode* dest_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];
  int start_index = GetStartIndex(node, 2);
  
  dest_node = IgnoreCasts(dest_node);
  
  int size;
  switch (node->opcode) {
   case IR_OP(store8):
     size = 1;
     break;
   case IR_OP(storea):
   case IR_OP(store16):
     size = 2;
     break;
   case IR_OP(store32):
   case IR_OP(storef):
   case IR_OP(stored):
     size = 4;
     break;
   case IR_OP(store64):
     size = 8;
     break;
   default:
     abort();
  }
  
  bool is_arg = false;
  
  if (node->inputs.length > 2) {
    // Storing indirect with an offset.
     StoreIndirect(g, node, dest_node, src_node, size, start_index);
    return;
  }
  if (dest_node->opcode == IR_OP(tempvar)) {
    if (IsLoweredVariableStorage(dest_node)) {
      StoreIntoVariable(g, node, dest_node, src_node, false, size);
    } else {
      StoreIndirect(g, node, dest_node, src_node, size, start_index);
    }
    return;
  }
  switch (dest_node->opcode) {
    case IR_OP(argument):
      is_arg = true;
    case IR_OP(localvar):
       StoreIntoVariable(g, node, dest_node, src_node, is_arg, size);
      return;

    case IR_OP(staticvar):
    case IR_OP(externvar):
       StoreIntoStaticVariable(g, node, dest_node, src_node, size);
      return;
    default:
       StoreIndirect(g, node, dest_node, src_node, size, start_index);
      return;
  }

#if 0
  AddressingMode src_mode = GetAddrMode(src);
  switch (addr_node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(ssavar):
    case IR_OP(phi): {
      TargetInstruction* addr = GetLoweredNode(addr_node);
      switch ((W65C02Opcode)addr->opcode) {
        case W65C02_OP(ivarreg):
        case W65C02_OP(bvarreg):
        case W65C02_OP(lvarreg):
        case W65C02_OP(xvarreg):
        case W65C02_OP(fvarreg):
        case W65C02_OP(dvarreg): {
          Copy(g, addr, src, 0, start_index, size, GetAddrMode(addr), src_mode);
          return SetLoweredNode(node, addr);
        }
        default:
          break;
      }
      bool done = false;
      if (start_index == 0 && !IsNRVONode(addr_node)) {
        int offset = (int)TargetIntValue(addr->operand[0]);
        bool highzero = offset < 256;
        Symbol* func;
        
        if (GetAddrMode(src) == kAddrModeZeroPage) {
          // Use the set_var/set_arg runtime function.
          switch (size) {
            case 1:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value1 : g->set_var_value1;
              } else {
                func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value1b : g->set_var_value1b;
              }
              break;
            case 2:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value2 : g->set_var_value2;
              } else {
                func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value2b : g->set_var_value2b;
              }
              break;
            case 4:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value4 : g->set_var_value4;
              } else {
                func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value4b : g->set_var_value4b;
              }
              break;
            case 8:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value8 : g->set_var_value8;
              } else {
                  func = addr_node->opcode == IR_OP(argument) ? g->set_arg_value8b : g->set_var_value8b;
              }
                break;
            default:
              abort();
              break;
          }
           // lda #src
           // ldx #var addr lo
           // ldy #var addr hi (omitted for hi == 0)
           // jsr func
           Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), src, kAddrModeImplied));
           ldxi(g, offset & 0xff);
           if (!highzero) {
             ldyi(g, (offset >> 8) & 0xff);
           }
           jsr(g, func);
          done = true;
        }
        
        if (!done && GetAddrMode(src) == kAddrModeImmediate && TargetIsZero(src)) {
          // Use the zero_var/set_arg runtime function.
          switch (size) {
            case 1:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value1 : g->zero_var_value1;
              } else {
                func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value1b : g->zero_var_value1b;
              }
              break;
            case 2:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value2 : g->zero_var_value2;
              } else {
                func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value2b : g->zero_var_value2b;
              }
              break;
            case 4:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value4 : g->zero_var_value4;
              } else {
                func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value4b : g->zero_var_value4b;
              }
              break;
            case 8:
              if (highzero) {
                func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value8 : g->zero_var_value8;
              } else {
                  func = addr_node->opcode == IR_OP(argument) ? g->zero_arg_value8b : g->zero_var_value8b;
              }
                break;
            default:
              abort();
              break;
          }
          // ldx #var addr lo
          // ldy #var addr hi (omitted for hi == 0)
          // jsr func
          ldxi(g, offset & 0xff);
          if (!highzero) {
            ldyi(g, (offset >> 8) & 0xff);
          }
          jsr(g, func);
          done = true;
        }
      }
      
      if (!done) {
        TargetInstruction* addr;
        if (IsNRVONode(addr_node)) {
          // This is an Named RVO variable.  It's address is the same
          // as the structreturn address.
          assert(g->struct_return_inst != NULL);
          addr = g->struct_return_inst;
        } else {
          // Store to a local variable.
          addr = GetAddress(g, addr_node);
        }
        if (addr_node->opcode == IR_OP(ssavar) || addr_node->opcode == IR_OP(phi)) {
          addr = addr->operand[0];
        }

        // var_addr is the address of the variable.
        // lda src+0
        // sta (var_addr)
        // ldy #1 or INY
        // lda src+1
        // sta (var_addr), Y
        Copy(g, addr, src, 0, start_index, size, kAddrModeIndirectIndexed, GetAddrMode(src));
      }
#if 0
      AddressingMode src_mode = GetAddrMode(src);
      if (size > 2 && j == 0) {
        if (src_mode == kAddrModeZeroPage) {
          SetAddrMode(src, kAddrModeZeroPageIndexedY);
          SetAddrMode(addr, kAddrModeIndirectIndexed);
          // Use loop with src index in Y.
          ldyi(g, size-1);
          TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
          lda(g, src, 0);
          sta(g, addr, 0);
          dey(g);
          EmitResolvedBranch(g, W65C02_OP(bne), loop);
          break;
        }
        if (src_mode == kAddrModeImmediate) {
          // Use loop with src index in Y.
          SetAddrMode(addr, kAddrModeIndirectIndexed);
          ldyi(g, size-1);
          TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
          lda(g, src, 0);
          sta(g, addr, 0);
          dey(g);
          EmitResolvedBranch(g, W65C02_OP(bne), loop);
          break;
        }
      }
       for (int i = 0; i < size; i++, j++) {
        SetIndexReg(g, src, src, i);
        lda(g, src, i);
        ldyi(g, j);
        Emit(g, NewInstruction2(W65C02_OP(sta), addr, Zero(g), dest_mode));
        dest_mode = kAddrModeIndirectIndexed;
      }
#endif
      break;
      }
    case IR_OP(externvar):
    case IR_OP(staticvar): {
      // Store to a static variable.
      TargetInstruction* addr = GetLoweredNode(addr_node);
      Copy(g, addr, src, 0, start_index, size, kAddrModeAbsoluteSymbolIndexed, GetAddrMode(src));
#if 0
      for (int i = 0; i < Sizeof(node->type); i++, j++) {
        SetIndexReg(g, src, addr, i);
        lda(g, src, i);
        ldyi(g, j);
        Emit(g, NewInstruction2(W65C02_OP(sta), addr,
                               ByteConst(g, j),
                                kAddrModeAbsoluteSymbolIndexed));
      }
#endif
      break;
    }
      
    default: {
      TargetInstruction* addr = Materialize(g, addr_node);
      int store_size;
      switch (node->opcode) {
        case IR_OP(store8):
          store_size = 1;
          break;
        case IR_OP(storea):
        case IR_OP(store16):
          store_size = 2;
          break;
        case IR_OP(storef):
        case IR_OP(store32):
        case IR_OP(stored):
          store_size = 4;
          break;
        case IR_OP(store64):
          store_size = 8;
          break;
        default:
          abort();
      }
      // Store indirect via addr.
      Copy(g, addr, src, 0, start_index, size, kAddrModeIndirectIndexed, GetAddrMode(src));
#if 0
      AddressingMode dest_mode = start_index == 0 ? kAddrModeIndirect : kAddrModeIndirectIndexed;
      int j = start_index;
      for (int i = 0; i < store_size; i++, j++) {
        SetIndexReg(g, src, addr, i);
        lda(g, src, i);
        if (i != j) {
          ldyi(g, j);
        }
        Emit(g, NewInstruction2(W65C02_OP(sta), addr, Zero(g), dest_mode));
        dest_mode = kAddrModeIndirectIndexed;
      }
#endif
      break;
    }
  }
  return SetLoweredNode(node, src);
#endif
}

static int BitSizeToByteSize(int bit_size) {
  if (bit_size > 32) {
    return 8;
  }
  if (bit_size > 16) {
    return 4;
  }
  if (bit_size > 8) {
    return 2;
  }
  return 1;
}
// Node inputs are:
// 0: value containing bit field
// 1: bit field position (0 = LSB)
// 2: bit field size (in bits)
//
// If bitfield is unsigned:
// Right shift by position and mask with 1's
// But right shifting is slow so we want to use an LDA to load the appropriate
// bytes
static void LowerGetBitField(W65C02Generator* g, IRNode* node) {
  IRNode* input_node = node->inputs.value.p[0];
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[1]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[2]);
  int byte_size = BitSizeToByteSize(bit_size);
  TargetInstruction* input = Materialize(g, input_node, Sizeof(input_node->type), true);
  int output_size = Sizeof(input_node->type);
  TargetInstruction* output = TempRegister(g, input_node->type, output_size);
  
  // input is in a zero page register.
  int byte_index = bit_pos / 8;
  uint64_t mask = (1 << bit_size) - 1;
  int hi_pos = bit_pos % 8;
  for (int i = byte_index; i < byte_index + byte_size; i++) {
    lda(g, input, i);
    if (hi_pos > 0) {
      // Shift to right of byte.
      for (int j = 0; j < hi_pos; j++) {
        Emit(g, NewInstruction(W65C02_OP(lsr), kAddrModeAccumulator));
      }
    }
    uint64_t byte_mask = mask & 0xff;
    if (byte_mask != 0xff) {
      // Mask to correct width in byte.
      andi(g, (int)byte_mask);
    }
    sta(g, output, i);
    mask >>= 8;
  }
  if (TypeIsUnsigned(node->type)) {
    // Remaining bytes of result are zero.
    ldai(g, 0);
    for (int i = byte_index + byte_size; i < output_size; i++) {
      sta(g, output, i);
    }
  } else {
    // If the top bit of the input is set fill the remaining bytes with
    // 0xff, otherwise fill them with zero.
    lda(g, input, byte_index + byte_size - 1);
    int top_bit = 1 << (((bit_pos + bit_size) % 8)- 1);
    int upper_bits = ~((1 << (bit_size % 8)) - 1) & 0xff;
    bool need_upper_bytes_set = (byte_index + byte_size) < output_size;
    if (need_upper_bytes_set) {
      ldxi(g, 0);
    }
    andi(g, top_bit);
    TargetInstruction* label = NewInstruction(W65C02_OP(label), kAddrModeImplied);
    EmitResolvedBranch(g, W65C02_OP(beq), label);
    ldai(g, upper_bits);
    if (need_upper_bytes_set) {
      ldxi(g, 0xff);
    }
    Emit(g, label);
    ora(g, output, byte_index + byte_size - 1);
    sta(g, output,  byte_index + byte_size - 1);
    for (int i = byte_index + byte_size; i < output_size; i++) {
      stx(g, output, i);
    }
  }
  SetLoweredNode(node, output);
}

// This is a load-modify-write operation.  Normally this is done using shifts
// ands and ors but on 6502 this is very expensive.  Instead we load the bytes
// individually, shift and mask them and store them back again.
// Inputs are:
// 0: address of where to store result
// 1: input value (normalized to bottom of word)
// 2: bit offset into output
// 3: bit size.
//
// Operation:
// Load value of destination.
// for each byte in bitfield:
//   if whole byte:
//     store byte in destination directly
//   else
//     load byte from destination address
//     AND with 1's in upper bits not used
//     ORA with used lower bits
//     store byte in destination
static void LowerSetBitField(W65C02Generator* g, IRNode* node) {
  IRNode* output_node = node->inputs.value.p[0];
  IRNode* input_node = node->inputs.value.p[1];
  int bit_pos = (int)IRIntConstValue(node->inputs.value.p[2]);
  int bit_size = (int)IRIntConstValue(node->inputs.value.p[3]);
  int byte_size = BitSizeToByteSize(bit_size);
  int output_size = Sizeof(output_node->type);

  int output_byte_index = bit_pos / 8;
  int output_bit_index = bit_pos % 8;
  int first_whole_byte = 0;
  int lshifted_bits = 0;

  TargetInstruction* input = Materialize(g, input_node, Sizeof(input_node->type), true);
  TargetInstruction* output = Materialize(g, output_node, output_size, false);
  

  if (output_bit_index != 0) {
    // Not on a byte boundary.  Shift input left by bit_pos bits.
    for (int i = 0; i < bit_pos; i++) {
      Emit(g, NewInstruction2(W65C02_OP(asl), input, ByteConst(g, 0), kAddrModeZeroPage));
      for (int j = 1; j < output_size; j++) {
        Emit(g, NewInstruction2(W65C02_OP(rol), input, ByteConst(g, j), kAddrModeZeroPage));
      }
    }
    lshifted_bits = bit_pos;
    
    // Load first output byte, mask it and or in the shifted input byte.
    lda(g, output, output_byte_index);
    int mask = (1 << bit_pos) - 1;
    andi(g, mask);
    ora(g, input, 0);
    sta(g, output, output_byte_index);
    first_whole_byte = 1;
  }
  // Middle bytes are whole.
  for (int i = first_whole_byte; i < byte_size - 1; i++) {
    lda(g, input, i);
    sta(g, output, output_byte_index + i);
  }
  // Now top byte.
  int bits_in_top_byte = (bit_size + lshifted_bits) % 8;
  int in_mask = (1 << bits_in_top_byte) - 1;
  int out_mask = ~in_mask & 0xff;
  if (bits_in_top_byte != 0) {
    lda(g, output, output_byte_index + byte_size - 1);
    andi(g, out_mask);
    sta(g, output, output_byte_index + byte_size - 1);
    lda(g, input, byte_size - 1);
    ora(g, output, output_byte_index + byte_size - 1);
    sta(g, output, output_byte_index + byte_size - 1);
  } else {
    lda(g, input, byte_size - 1);
    sta(g, output, output_byte_index + byte_size - 1);
  }
  SetLoweredNode(node, output);
}

static void LowerInc(W65C02Generator* g, IRNode* node) {
  IRNode* var = node->inputs.value.p[0];
  bool is_reg = false;
  TargetInstruction* addr = GetAddress(g, var, true);
  if (IRIsVariable(var)) {
    switch ((W65C02Opcode)addr->opcode) {
      case W65C02_OP(ivarreg):
      case W65C02_OP(bvarreg):
      case W65C02_OP(lvarreg):
      case W65C02_OP(xvarreg):
      case W65C02_OP(fvarreg):
      case W65C02_OP(dvarreg):
        is_reg = true;
        break;
      default:
        break;
    }
  }
  AddReloadPoint(g, addr);
  assert(IRIsConst(node->inputs.value.p[1]));
  TargetInstruction* amount = GetLoweredNode(node->inputs.value.p[1]);
  int amount_i = (int)TargetIntValue(amount);
  bool big = amount_i > 255;
  Symbol* func;
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), addr, kAddrModeImplied));
  if (amount_i != 1) {
    ldxi(g, amount_i & 0xff);
    if (big) {
      ldyi(g, (amount_i >> 8) & 0xff);
    }
  }
  int size = Sizeof(node->type);
  if (TypeIsDouble(node->type)) {
    func = is_reg ? g->rincd :g->incd;
    size = 4;
  } else if (TypeIsFloat(node->type)) {
    func = is_reg ? g->rincf : g->incf;
    size = 8;
  } else {
    switch (size) {
      case 1:
        func = is_reg ? g->rinc1 : g->inc1;
        break;
      case 2:
        if (amount_i == 1) {
          func = is_reg ? g->rinc21 : g->inc21;
        } else {
          func = big ? (is_reg ? g->rinc2b : g->inc2b) : (is_reg ? g->rinc2 : g->inc2);
        }
        break;
      case 4:
        func = is_reg ? g->rinc4 : g->inc4;
        break;
      case 8:
        func = is_reg ? g->rinc8 : g->inc8;
        break;
      default:
        abort();
    }
  }
  jsr(g, func);
  TargetInstruction* result = addr;
  if (node->outputs.length > 0 || node->dest != NULL) {
    // The value is used.  Need to load the incrementee into a temporary.
    TargetInstruction* dest;
    if (node->dest == NULL) {
      dest = TempRegister(g, node->type, size);
    } else {
       dest = GetAddress(g, node->dest, true);
    }
    AddReloadPoint(g, dest);
    Copy(g, dest, result, 0, 0, size, GetAddrMode(dest), GetAddrMode(addr));
    result = dest;
  }
  SetLoweredNode(node, result);
  AddSpillPoint(g, result);
}

static void LowerDec(W65C02Generator* g, IRNode* node) {
  IRNode* var = node->inputs.value.p[0];
  bool is_reg = false;
  TargetInstruction* addr = GetAddress(g, var, true);
  if (IRIsVariable(var)) {
    switch ((W65C02Opcode)addr->opcode) {
      case W65C02_OP(ivarreg):
      case W65C02_OP(bvarreg):
      case W65C02_OP(lvarreg):
      case W65C02_OP(xvarreg):
      case W65C02_OP(fvarreg):
      case W65C02_OP(dvarreg):
        is_reg = true;
        break;
      default:
        break;
    }
  }
  AddReloadPoint(g, addr);
  TargetInstruction* amount = GetLoweredNode(node->inputs.value.p[1]);
  int amount_i = (int)TargetIntValue(amount);
  Symbol* func;
  Emit(g, NewInstruction1(W65C02_OP(expr_addr_a), addr, kAddrModeImplied));
  bool big = amount_i > 255;
  if (amount_i != 1) {
    ldxi(g, amount_i & 0xff);
    if (big) {
      ldyi(g, (amount_i >> 8) & 0xff);
    }
  }
  int size = Sizeof(node->type);
  if (TypeIsDouble(node->type)) {
    func = is_reg ? g->rdecd : g->decd;
  } else if (TypeIsFloat(node->type)) {
    func = is_reg ? g->rdecf : g->decf;
  } else {
    switch (size) {
      case 1:
        func = is_reg ? g->rdec1 : g->dec1;
        break;
      case 2:
        if (amount_i == 1) {
          func = is_reg ? g->rdec21 : g->dec21;
        } else {
          func = big ? (is_reg ? g->rdec2b : g->dec2b) : (is_reg ? g->rdec2 : g->dec2);
        }
        break;
      case 4:
        func = is_reg ? g->rdec4 : g->dec4;
        break;
      case 8:
        func = is_reg ? g->rdecd : g->dec8;
      default:
        abort();
    }
  }
  jsr(g, func);
  TargetInstruction* result = addr;
  if (node->outputs.length > 0 || node->dest != NULL) {
    // The value is used.  Need to load the incrementee into a temporary.
    TargetInstruction* dest;
    if (node->dest == NULL) {
      dest = TempRegister(g, node->type, size);
    } else {
       dest = GetAddress(g, node->dest, true);
    }
    AddReloadPoint(g, dest);
    Copy(g, dest, result, 0, 0, size, GetAddrMode(dest), GetAddrMode(addr));
    result = dest;
  }
  SetLoweredNode(node, result);
  AddSpillPoint(g, result);
}

static struct {
  bool (*type_func)(TypeRecord*);
  int size;
  int pushed_size;
} push_map[] = {
    {TypeIsInt, 2, 2},      {TypeIsShort, 2, 2},         {TypeIsChar, 1, 2},
    {TypeIsLong, 4, 4},     {TypeIsLongLong, 8, 8},      {TypeIsFloat, 4, 4},
    {TypeIsDouble, 4, 4},   {TypeIsLongDouble, 4, 4},    {TypeIsPointerOrArray, 2, 2},
  {TypeIsFunction, 2, 2}, {TypeIsStructOrUnion, 2, 2},
  {TypeIsMemberPointerScalar, 2, 2},
  {TypeIsMemberPointerAggregate, 4, 4},
  {TypeIsBool, 1, 2}, {NULL, 0},
};



static TargetInstruction* PushExpression(W65C02Generator* g, IRNode* node,
                           TargetInstruction* inst, int size) {
  
  switch (size) {
    case 1:
      lda(g, inst, 0);
      return jsr(g, g->pusha);     // Pushed as 2 bytes.
    case 2:
      return Emit(g, NewInstruction1(W65C02_OP(pushreg2), inst, kAddrModeImplied));
    case 4:
      return Emit(g, NewInstruction1(W65C02_OP(pushreg4), inst, kAddrModeImplied));

    case 8:
      return Emit(g, NewInstruction1(W65C02_OP(pushreg8), inst, kAddrModeImplied));

    default:
      abort();
  }
}

// For one or two byte constants we load the value into A or X,Y and call a push
// function.
//
// For 4 and 8 byte constants we load the address of the constant literal into
// X,Y and call the push function.

static TargetInstruction* PushConstant(W65C02Generator* g, IRNode* node, int size) {
  uint64_t value = IRIntConstValue(node);
  switch (size) {
    case 1:         // Pushed as 2 bytes.
      Emit(g, NewInstruction1(
                  W65C02_OP(lda),
                  ByteConst(g, value & 0xff),
                  kAddrModeImmediate));
      return jsr(g, g->pusha);           // Pushes 2 bytes.
      break;
    case 2:
      Emit(g, NewInstruction1(
                  W65C02_OP(ldx),
                  ByteConst(g, value & 0xff),
                  kAddrModeImmediate));
      if (value > 255) {
        Emit(g, NewInstruction1(
                                W65C02_OP(ldy),
                                ByteConst(g, (value >> 8) & 0xff),
                                kAddrModeImmediate));
        return jsr(g, g->pushxy);
      } else {
        return jsr(g, g->pushxy0);
      }
      break;

    case 4: {
      TargetConstant* literal = (TargetConstant*)GetLoweredNode(node);
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      lit->disabled = false;
      Emit(g, NewInstruction1(
                  W65C02_OP(literalref),
                              &literal->base,
                  kAddrModeAbsolute));
      return jsr(g, g->push4xy);
      break;
    }
    case 8: {
      TargetConstant* literal = (TargetConstant*)GetLoweredNode(node);
      Literal* lit = CompilerFindLiteral(literal->literal_id);
      assert(lit != NULL);
      lit->disabled = false;
      Emit(g, NewInstruction1(
                  W65C02_OP(literalref),
                  &literal->base,
                  kAddrModeAbsolute));
      return jsr(g, g->push8xy);
      break;
    }
      
    default:
      abort();
  }
}

static TargetInstruction* PushLiteralRef(W65C02Generator* g, IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, TargetNewLiteral((int)id_node->value.ivalue));
  Emit(g, NewInstruction1(
              W65C02_OP(stringliteralref),
              literal,
              kAddrModeAbsolute));
  return jsr(g, g->pushxy);
}

// Although we would like to push a boolean or char as a single byte
// we can't because C says that they need to be passed as ints.
static TargetInstruction* PushVariable(W65C02Generator* g, IRNode* node, int size) {
  TargetInstruction* addr = GetLoweredNode(node);
  if (addr == NULL || addr->operand[0] == NULL ||
      !TargetIsConst(addr->operand[0])) {
    TargetInstruction* value = Materialize(g, node, size, true);
    AddReloadPoint(g, value);
    return PushExpression(g, node, value, size);
  }
  int offset = (int)TargetIntValue(addr->operand[0]);
  struct {
    int size;
    Symbol* push_var;   // 1-byte offset
    Symbol* push_arg;   // 1-byte offset
    Symbol* push_varb;  // 2-byte offset
    Symbol* push_argb;  // 2-byte offset
  } var_pushes[] = {
      {1, g->push_var1, g->push_arg1, g->push_var1b, g->push_arg1b},    // pushed as 2 bytes.
      {2, g->push_var2, g->push_arg2, g->push_var2b, g->push_arg2b},
      {4, g->push_var4, g->push_arg4, g->push_var4b, g->push_arg4b},
      {8, g->push_var8, g->push_arg8, g->push_var8b, g->push_arg8b},
      {0},
  };
  Symbol* push_sym = NULL;
  if (TypeIsArray(node->type) || TypeIsStructOrUnion(node->type)) {
    // Get the address of these and push them.
    W65C02Opcode addr_op;
    if (offset >= 256) {
      addr_op = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                    ? W65C02_OP(arg_addrb_xy)
                    : W65C02_OP(var_addrb_xy);
    } else {
      addr_op = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                    ? W65C02_OP(arg_addr_xy)
                    : W65C02_OP(var_addr_xy);
    }
    Emit(g, NewInstruction1(addr_op, addr, kAddrModeIndirect));
    return jsr(g, g->pushxy);
  }
  for (size_t i = 0; i < var_pushes[i].size != 0; i++) {
    if (var_pushes[i].size == size) {
      if (offset >= 256) {
        push_sym = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                       ? var_pushes[i].push_argb
                       : var_pushes[i].push_varb;
      } else {
        push_sym = TargetOpcodeEq(addr->opcode, W65C02_OP(argument))
                       ? var_pushes[i].push_arg
                       : var_pushes[i].push_var;
      }
      break;
    }
  }
  assert(push_sym != NULL);
   // ldx #offset lo
   // ldy #offset hi (removed for single byte case)
   // JSR __push_var(size)[b] (or push_arg)
   ldxi(g, offset);
   if (offset >= 256) {
     ldyi(g, offset >> 8);
   }
   return jsr(g, push_sym);
}


static TargetInstruction* Push(W65C02Generator* g, IRNode* node, int size) {
  TargetInstruction* inst = NULL;
  switch (node->opcode) {
    case IR_OP(argument):
    case IR_OP(localvar):
      return PushVariable(g, node, size);
      
    case IR_OP(ssavar):
    case IR_OP(phi): {
      IRVariable* var = (IRVariable*)node;
      IRNode* symbol = FindPooledVariable(g->gen, var->symbol);
      return PushVariable(g, symbol, size);
    }
    case IR_OP(literalref):
      return PushLiteralRef(g, node);
      
    default:
      if (IRIsConst(node)) {
        return PushConstant(g, node, size);
      } else {
        inst = Materialize(g, node, size, true);
        AddReloadPoint(g, inst);
        return PushExpression(g, node, inst, size);
      }
  }
}

static TargetInstruction* PushArg(W65C02Generator* g, IRNode* node) {
  if (node->type == NULL) {
    return Push(g, node, 2);
  }
  for (size_t i = 0; push_map[i].type_func != NULL; i++) {
    if (push_map[i].type_func(node->type)) {
      return Push(g, node, push_map[i].size);
    }
  }
  assert(false);
  return NULL;
}

// Get the number of bytes pushed for an argument.
static size_t GetPushedSize(IRNode* node) {
  assert(node->opcode == IR_OP(pusharg));
  IRNode* expr = node->inputs.value.p[0];
  if (expr->type == NULL) {
    return 2;
  }
  if (TypeIsStructOrUnion(expr->type)) {
    return Sizeof(expr->type);
  }
  for (size_t i = 0; push_map[i].type_func != NULL; i++) {
    if (push_map[i].type_func(expr->type)) {
      return push_map[i].pushed_size;
    }
  }
  assert(false);
  return 0;
}

// Passing a struct or union to a function needs to copy
// the memory from the address to the stack.
// mem_src: source address
// mem_size: size of memory
// JSR pushmem
static TargetInstruction* PushStructArg(W65C02Generator* g, IRNode* node,
                                        size_t* args_size) {
  size_t struct_size = Sizeof(node->type);
  *args_size += struct_size;
  bool from_call = (node->flags & kIRFromCall) != 0;
  Symbol* pushmem = from_call ? g->pushmem_xy1 : g->pushmem1;

  // Put size in __mem_size.
  Emit(g, NewInstruction1(
              W65C02_OP(lda),
              ByteConst(g,  struct_size & 0xff),
              kAddrModeImmediate));
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       ByteConst(g, W65C02_MSZ_REG),
                       kAddrModeZeroPageAbsolute));
  if (struct_size >= 256) {
    pushmem = from_call ? g->pushmem_xy2 : g->pushmem2;
    Emit(g, NewInstruction1(W65C02_OP(lda),
                            ByteConst(g,
                                           (struct_size >> 8) & 0xff),
                            kAddrModeImmediate));
    Emit(g, NewInstruction1(
                W65C02_OP(sta),
                ByteConst(g, W65C02_MSZ_REG + 1),
                kAddrModeZeroPageAbsolute));
  }

  if (!from_call) {
  // Put src in __mem_src
  TargetInstruction* src = GetAddress(g, node, true);
  AddReloadPoint(g, src);
  Emit(g, NewInstruction2(W65C02_OP(lda), src,
                          Zero(g),
                          GetAddrMode(src)));
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       GetIntConstant(g, NULL, kTargetType8Bit, W65C02_MSRC_REG),
                       kAddrModeZeroPageAbsolute));
  Emit(g, NewInstruction2(W65C02_OP(lda), src,
                          One(g),
                          GetAddrMode(src)));
  Emit(g, NewInstruction1(
              W65C02_OP(sta),
              ByteConst(g, W65C02_MSRC_REG + 1),
              kAddrModeZeroPageAbsolute));
  }
  
  // JSR pushmem
  return jsr(g, pushmem);
}

static bool IsIntrinsicCall(W65C02Generator* g, IRNode* node) {
  IRNode* callee = node->inputs.value.p[0];
  if (callee->opcode == IR_OP(staticvar)) {
    IRVariable* var = (IRVariable*)callee;
    if (StorageIs(var->symbol->storage, STO(static)|STO(auto))) {
      return false;
    }
    MapKeyType k = {.p = var->symbol->name.value};
    Intrinsic* in = MapFind(&g->intrinsics, k);
    if (in != NULL) {
      return true;
    }
  }
  return false;
}

static TargetInstruction* CallIntrinsic(W65C02Generator* g, IRNode* node) {
  if (!IsIntrinsicCall(g, node)) {
    return NULL;
  }
  IRNode* callee = node->inputs.value.p[0];
  assert(callee->opcode == IR_OP(staticvar));
  IRVariable* var = (IRVariable*)callee;
  assert(!StorageIs(var->symbol->storage, STO(static)|STO(auto)));
  MapKeyType k = {.p = var->symbol->name.value};
  Intrinsic* in = MapFind(&g->intrinsics, k);
  assert(in != NULL);
  
  TargetInstruction* result = NULL;
  if (node->dest != NULL) {
    LowerIRNode(g, node->dest);
    result = GetLoweredNode(node->dest);
  } else {
    result = TempRegister(g, node->type, Sizeof(node->type));
  }
  if (result != NULL) {
    result->flags |= k6502ExprIsCallResult;
  }

  switch (in->index) {
    case kIntrinsicIsalnum:
    case kIntrinsicIsalpha:
    case kIntrinsicIsblank:
    case kIntrinsicIscntrl:
    case kIntrinsicIsdigit:
    case kIntrinsicIsgraph:
    case kIntrinsicIslower:
    case kIntrinsicIsprint:
    case kIntrinsicIspunct:
    case kIntrinsicIsspace:
    case kIntrinsicIsupper:
    case kIntrinsicIsxdigit:
    case kIntrinsicTolower:
    case kIntrinsicToupper: {
      IRNode* arg_node = node->inputs.value.p[1];
      TargetInstruction* arg = Materialize(g, arg_node, 2, true);
      ldx(g, arg, 0);
      ldy(g, arg, 1);
      jsr(g, in->symbol);   // Call intrinsic, result in A.
      SetIndexReg(g, result, result, 0);
      sta(g, result, 0);
      stz(g, result, 1);
      return SetLoweredNode(node, result);
    }
       break;
    case kIntrinsicMemcpy: {
      // Put size in __mem_size.
      TargetInstruction* size = Materialize(g, node->inputs.value.p[3], 2, true);
      AddReloadPoint(g, size);
      SetIndexReg(g, size, size, 0);
      lda(g, size, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MSZ_REG),
                           kAddrModeZeroPageAbsolute));
      SetIndexReg(g, size, size, 1);
      lda(g, size, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MSZ_REG + 1),
                  kAddrModeZeroPageAbsolute));

      // Put src in __mem_src
      TargetInstruction* src = GetAddress(g, node->inputs.value.p[2], true);
      AddReloadPoint(g, src);
      SetIndexReg(g, src, src, 0);
      lda(g, src, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MSRC_REG),
                           kAddrModeZeroPageAbsolute));
      SetIndexReg(g, src, src, 1);
      lda(g, src, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MSRC_REG + 1),
                  kAddrModeZeroPageAbsolute));

      // Put dest in __mem_dest.
      TargetInstruction* dest = GetAddress(g, node->inputs.value.p[1], true);
      AddReloadPoint(g, dest);
      SetIndexReg(g, dest, dest, 1);
      lda(g, dest, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MDST_REG),
                           kAddrModeZeroPageAbsolute));
      SetIndexReg(g, dest, dest, 1);
      lda(g, dest, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MDST_REG + 1),
                  kAddrModeZeroPageAbsolute));

      // JSR __builtin_memcpy
      jsr(g, in->symbol);

      if (node->outputs.length > 0) {
        // Result is in __mem_dest.
        Emit(g,
             NewInstruction1(W65C02_OP(lda),
                             ByteConst(g, W65C02_MDST_REG),
                             kAddrModeZeroPageAbsolute));
        SetIndexReg(g, result, result, 0);
        sta(g, result, 0);
        
        Emit(g,
             NewInstruction1(W65C02_OP(lda),
                             ByteConst(g, W65C02_MDST_REG+1),
                             kAddrModeZeroPageAbsolute));

        SetIndexReg(g, result, result, 0);
        sta(g, result, 1);
      }
      return SetLoweredNode(node, result);
    }
    case kIntrinsicMemset: {
      // Put size in __mem_size.
      TargetInstruction* size = Materialize(g, node->inputs.value.p[3], 2, true);
      AddReloadPoint(g, size);
      SetIndexReg(g, size, size, 0);
      lda(g, size, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MSZ_REG),
                           kAddrModeZeroPageAbsolute));
      SetIndexReg(g, size, size, 1);
      lda(g, size, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MSZ_REG + 1),
                  kAddrModeZeroPageAbsolute));

      // Put value in __mem_src.  One byte only.
      TargetInstruction* src = GetLoweredNode(node->inputs.value.p[2]);
      AddReloadPoint(g, src);
      SetIndexReg(g, src, src, 0);
      lda(g, src, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MSRC_REG),
                           kAddrModeZeroPageAbsolute));
      
      // Put dest in __mem_dest.
      TargetInstruction* dest = GetAddress(g, node->inputs.value.p[1], true);
      AddReloadPoint(g, dest);
      lda(g, dest, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MDST_REG),
                           kAddrModeZeroPageAbsolute));
      lda(g, dest, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MDST_REG + 1),
                  kAddrModeZeroPageAbsolute));
      jsr(g, in->symbol);
      
      // Result is in __mem_dest.
      if (node->outputs.length > 0) {
        Emit(g,
             NewInstruction1(W65C02_OP(lda),
                             ByteConst(g, W65C02_MDST_REG),
                             kAddrModeZeroPageAbsolute));
        SetIndexReg(g, result, result, 0);
        sta(g, result, 0);
        
        Emit(g,
             NewInstruction1(W65C02_OP(lda),
                             ByteConst(g, W65C02_MDST_REG+1),
                             kAddrModeZeroPageAbsolute));

        SetIndexReg(g, result, result, 0);
        sta(g, result, 1);
      }
      return SetLoweredNode(node, result);
    }
    case kIntrinsicMemcmp: {
      // Put size in __mem_size.
      TargetInstruction* size = Materialize(g, node->inputs.value.p[3], 2, true);
      AddReloadPoint(g, size);
      SetIndexReg(g, size, size, 0);
      lda(g, size, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MSZ_REG),
                           kAddrModeZeroPageAbsolute));
      SetIndexReg(g, size, size, 1);
      lda(g, size, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MSZ_REG + 1),
                  kAddrModeZeroPageAbsolute));

      // Put s2 in __mem_src
      TargetInstruction* src = GetAddress(g, node->inputs.value.p[2], true);
      AddReloadPoint(g, src);
      SetIndexReg(g, src, src, 0);
      lda(g, src, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MSRC_REG),
                           kAddrModeZeroPageAbsolute));
      SetIndexReg(g, src, src, 1);
      lda(g, src, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MSRC_REG + 1),
                  kAddrModeZeroPageAbsolute));

      // Put s1 in __mem_dest.
      TargetInstruction* dest = GetAddress(g, node->inputs.value.p[1], true);
      AddReloadPoint(g, dest);
      SetIndexReg(g, dest, dest, 1);
      lda(g, dest, 0);
      Emit(g,
           NewInstruction1(W65C02_OP(sta),
                           ByteConst(g, W65C02_MDST_REG),
                           kAddrModeZeroPageAbsolute));
      SetIndexReg(g, dest, dest, 1);
      lda(g, dest, 1);
      Emit(g, NewInstruction1(
                  W65C02_OP(sta),
                  ByteConst(g,  W65C02_MDST_REG + 1),
                  kAddrModeZeroPageAbsolute));

      // JSR __builtin_memcmp
      jsr(g, in->symbol);
      
      SetIndexReg(g, result, result, 0);
      stx(g, result, 0);
      sty(g, result, 1);
      return SetLoweredNode(node, result);
    }
    default:
      fprintf(stderr, "Unknown intrinsic index %d for %s\n", in->index, in->symbol->name.value);
      abort();
  }

  return NULL;
}

static bool CanForwardTailCallArguments(W65C02Generator* g, IRNode* call);
static bool CanElideForwardingTailFrame(W65C02Generator* g);
static bool IsForwardedTailArgumentLoad(W65C02Generator* g, IRNode* node);

static void LowerPushArg(W65C02Generator* g, IRNode* node) {
  IRNode* call = node->outputs.value.p[0];
  if (IsIntrinsicCall(g, call)) {
    // Intrinsics don't push their arguments.
    // Literal references are normally lowered from pusharg, but since we
    // are not pushing any arguments, we need to lower that now.
    IRNode* arg = node->inputs.value.p[0];
    if (arg->opcode == IR_OP(literalref)) {
      LowerLiteralReference(g, arg, true);
    }
    SetLoweredNode(node, GetLoweredNode(arg));
    return;
  }
  IRNode* arg = node->inputs.value.p[0];
  if (CanForwardTailCallArguments(g, call)) {
    return;
  }
  if (arg->type != NULL && TypeIsStructOrUnion(arg->type)) {
    size_t pushed_size = 0;
    SetLoweredNode(node, PushStructArg(g, arg, &pushed_size));
    return;
  }
  SetLoweredNode(node, PushArg(g, arg));
}

static int64_t CalculateArgumentSize(Symbol* arg);

static TypeRecord* CalleeFunctionType(IRNode* callee) {
  if (callee == NULL || callee->type == NULL) {
    return NULL;
  }
  if (TypeIsFunction(callee->type)) {
    return callee->type;
  }
  if (TypeIsFunctionPointer(callee->type)) {
    return callee->type->next;
  }
  return NULL;
}

static bool FunctionUsesCalleeArgCleanup(TypeRecord* function_type) {
  // Keep void functions caller-cleaned for now.  C++ constructor/destructor
  // variants can have different hidden-argument layouts at their call sites
  // and emitted definitions.
  return function_type != NULL && TypeIsFunction(function_type) &&
         !function_type->info.function.varargs &&
         !function_type->info.function.unknown_args &&
         !TypeIsVoid(function_type->next) &&
         !TypeIsStructOrUnion(function_type->next);
}

static size_t FunctionArgumentStackSize(TypeRecord* function_type) {
  size_t size = 0;
  Vector* prototype = &function_type->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    size += (size_t)CalculateArgumentSize(prototype->value.p[i]);
  }
  return size;
}

// A fixed-arity 6502 callee removes its own arguments when it finally returns.
// A sibling tail call must therefore preserve the incoming area unchanged so
// that only the terminal callee pops it.
static bool CanForwardTailCallArguments(W65C02Generator* g, IRNode* call) {
  if ((call->flags & kIRTailCall) == 0 || call->inputs.length == 0 ||
      IsIntrinsicCall(g, call) ||
      compiler->current_function == NULL ||
      !TypeIsFunction(compiler->current_function)) {
    return false;
  }

  IRNode* callee = call->inputs.value.p[0];
  if (!IRIsStaticVariable(callee)) {
    return false;
  }
  Symbol* callee_symbol = ((IRVariable*)callee)->symbol;
  TypeRecord* callee_type = callee_symbol->type;
  TypeRecord* caller_type = compiler->current_function;
  if (!TypeIsFunction(callee_type) ||
      callee_type->info.function.varargs ||
      callee_type->info.function.unknown_args ||
      caller_type->info.function.varargs ||
      caller_type->info.function.unknown_args ||
      TypeIsStructOrUnion(call->type)) {
    return false;
  }

  size_t num_args = call->inputs.length - 1;
  if (num_args != caller_type->info.function.prototype.length ||
      num_args != callee_type->info.function.prototype.length) {
    return false;
  }

  for (size_t i = 0; i < num_args; i++) {
    IRNode* push = call->inputs.value.p[i + 1];
    if (push->opcode != IR_OP(pusharg) || push->inputs.length < 2 ||
        !IRIsIntConst(push->inputs.value.p[1]) ||
        (size_t)IRIntConstValue(push->inputs.value.p[1]) != i) {
      return false;
    }

    Symbol* caller_formal = caller_type->info.function.prototype.value.p[i];
    Symbol* callee_formal = callee_type->info.function.prototype.value.p[i];
    if (caller_formal == NULL || callee_formal == NULL ||
        TypeIsStructOrUnion(caller_formal->type) ||
        !TypeEqual(caller_formal->type, callee_formal->type)) {
      return false;
    }

    IRNode* value = push->inputs.value.p[0];
    if (IRIsLoad(value) && value->inputs.length == 1) {
      value = value->inputs.value.p[0];
    }
    if (!IRIsArgument(value) ||
        ((IRVariable*)value)->symbol != caller_formal) {
      return false;
    }
  }
  return true;
}

static bool IsForwardedTailArgumentLoad(W65C02Generator* g, IRNode* node) {
  if (!IRIsLoad(node) || node->outputs.length != 1) {
    return false;
  }
  IRNode* push = node->outputs.value.p[0];
  return push->opcode == IR_OP(pusharg) && push->outputs.length == 1 &&
         CanForwardTailCallArguments(g, push->outputs.value.p[0]);
}

// A function consisting solely of an exact argument-forwarding tail call
// needs no 6502 software-stack frame. Keeping X/Y and the incoming argument
// area untouched makes the tail call a single jump.
static bool CanElideForwardingTailFrame(W65C02Generator* g) {
  size_t num_tail_calls = 0;
  for (IRNode* inst = GeneratorFirstInstruction(g->gen); inst != NULL;
       inst = IRNext(inst)) {
    if (IRIsConstant(inst) || IRIsVariable(inst) ||
        IsForwardedTailArgumentLoad(g, inst)) {
      continue;
    }
    switch (inst->opcode) {
      case IR_OP(enter):
      case IR_OP(label):
      case IR_OP(leave):
      case IR_OP(ret):
      case IR_OP(bra):
      case IR_OP(nop):
        break;
      case IR_OP(pusharg):
        if (inst->outputs.length != 1 ||
            !CanForwardTailCallArguments(g, inst->outputs.value.p[0])) {
          return false;
        }
        break;
      case IR_OP(calla):
        if (!CanForwardTailCallArguments(g, inst)) {
          return false;
        }
        num_tail_calls++;
        break;
      case IR_OP(resulti):
      case IR_OP(resulta):
      case IR_OP(resultf):
      case IR_OP(resultd):
        if (inst->inputs.length != 1 ||
            !CanForwardTailCallArguments(g, inst->inputs.value.p[0])) {
          return false;
        }
        break;
      default:
        return false;
    }
  }
  return num_tail_calls == 1;
}

// First push all the args onto the stack right to left.
// Then set X,Y to the address of the result.
// Then, if the call is direct to a symbol:
//   JSR symbol
//
// If it's a call to an address:
//   JSR xx
//   BRA yy
// xx:
//   JMP (addr)
// yy:
//
// Then, increment the sp by the number of bytes pushed.
static void LowerCall(W65C02Generator* g, IRNode* node) {
  assert(node->inputs.length >= 1);
  IRNode* callee = node->inputs.value.p[0];
  
  TargetInstruction* instrinsic_call = CallIntrinsic(g, node);
  if (instrinsic_call != NULL) {
    return;
  }
  
  size_t args_size = 0;
  for (size_t i = 1; i < node->inputs.length;  i++) {
    IRNode* arg_node = node->inputs.value.p[i];
    args_size += GetPushedSize(arg_node);
  }
  bool callee_pops_args =
      FunctionUsesCalleeArgCleanup(CalleeFunctionType(callee));
  if (callee_pops_args) {
    assert(args_size ==
           FunctionArgumentStackSize(CalleeFunctionType(callee)));
  }

  if (CanForwardTailCallArguments(g, node)) {
    TargetInstruction* addr = Materialize(g, callee, 2, false);
    if (!CanElideForwardingTailFrame(g)) {
      TargetInstruction* leave =
          NewInstruction(IsLeaf(g) ? W65C02_OP(leave_leaf)
                                   : W65C02_OP(leave),
                         kAddrModeImplied);
      leave->flags |= k6502SkipArgCleanup;
      Emit(g, leave);
      if (!TypeIsVoid(node->type)) {
        Emit(g, NewInstruction1(
                    W65C02_OP(ldx), ByteConst(g, W65C02_RESULT_REG),
                    kAddrModeZeroPageAbsolute));
        Emit(g, NewInstruction1(
                    W65C02_OP(ldy), ByteConst(g, W65C02_RESULT_REG + 1),
                    kAddrModeZeroPageAbsolute));
      }
    }
    TargetInstruction* jump =
        Emit(g, NewInstruction1(W65C02_OP(jmp), addr, kAddrModeAbsolute));
    jump->flags |= k6502BlockEnd;
    SetLoweredNode(node, jump);
    return;
  }

  
  // Load return address into X,Y,
  TargetInstruction* result = NULL;
  // When the result destination is a zero-page register we don't let the
  // callee write into it directly: it returns the value through a temp buffer
  // that we copy into `result` after the call (see below).
  TargetInstruction* result_buf = NULL;
  int result_size = 0;
  // If the function returns a struct/union its result is in address specified
  // by first arg.
  if (TypeIsStructOrUnion(node->type)) {
    IRNode* push = node->inputs.value.p[1];   // pusharg instruction.
    result = GetLoweredNode(push->inputs.value.p[0]);   // Arg value.
  } else if (!TypeIsVoid(node->type)) {
    if (node->dest != NULL) {
      LowerIRNode(g, node->dest);
      result = GetLoweredNode(node->dest);
      if (W65C02IsExpression(result)) {
        // The callee returns its value by writing through the pointer we pass
        // in X,Y.  If that pointer were the address of `result`'s own zero-page
        // register and the callee happened to use (hence save and restore) that
        // register, its epilogue would restore the register *after* storing the
        // result, destroying it.  A call ends its basic block, so temp
        // registers are always free at this point; route the value through a
        // temp buffer -- which is necessarily caller-saved and so never touched
        // by the callee's restore -- and copy it into `result` after the call.
        result_size = Sizeof(node->type);
        result_buf = TempRegister(g, node->type, result_size);
        result_buf->flags |= k6502ExprIsCallResult;
        ldxzi(g, result_buf, 0);
        ldyi(g, 0);
      } else {
        // Not an expression, get address in X,Y
        GetAddressXY(g, node->dest);
      }
    } else {
      result = TempRegister(g, node->type, Sizeof(node->type));
      ldxzi(g, result, 0);
      ldyi(g, 0);
    }
    if (result != NULL && result_buf == NULL) {
      result->flags |= k6502ExprIsCallResult;
    }
  }
  // TargetInstruction* addr = GetLoweredNode(callee);
  TargetInstruction* addr = Materialize(g, callee, 2, false);
  TargetInstruction* call = NULL;
  if (TargetOpcodeEq(addr->opcode, W65C02_OP(symbol)) &&
      TypeIsFunction(callee->type)) {
    call = Emit(g, NewInstruction1(W65C02_OP(jsr), addr, kAddrModeAbsolute));
    call->flags |= k6502ProcedureCall;
  } else {
    // Calling an expression.
    addr = Materialize(g, node->inputs.value.p[0], -1, true);     // Address to call.
    TargetInstruction* call_label =
        NewInstruction(W65C02_OP(label), kAddrModeImplied);
    TargetInstruction* end_label =
        NewInstruction(W65C02_OP(label), kAddrModeImplied);
    call = Emit(g, NewInstruction1(W65C02_OP(jsr), call_label, kAddrModeAbsolute));
    call->flags |= k6502ProcedureCall;
    Emit(g, NewInstruction1(Is65c02() ? W65C02_OP(bra) : W65C02_OP(jmp), end_label, kAddrModeAbsolute));
    Emit(g, call_label);
    Emit(g, NewInstruction1(W65C02_OP(jmp), addr, kAddrModeIndirect));
    call = Emit(g, end_label);
  }
  if (args_size > 0 && !callee_pops_args) {
    if (TypeIsStructOrUnion(node->type)) {
      // Result is a struct/union.  Pop the first arg (the result address)
      // back into an expr2 so that we know were it was.
      jsr(g, g->pullxy);
      stx(g, result, 0);
      call = sty(g, result, 1);
      args_size -= 2;
    }
    if (args_size > 0) {
      Symbol* incsp;
      switch (args_size) {
        case 2:
          incsp = g->incsp2;
          break;
        case 4:
          incsp = g->incsp4;
          break;
        case 6:
          incsp = g->incsp6;
          break;
        case 8:
          incsp = g->incsp8;
          break;
        case 10:
          incsp = g->incsp10;
          break;
        case 12:
          incsp = g->incsp12;
          break;
        case 14:
          incsp = g->incsp14;
          break;
        case 16:
          incsp = g->incsp16;
          break;
        default:
          incsp = g->incsp;
          Emit(g, NewInstruction1(
                      W65C02_OP(lda),
                      ByteConst(g, args_size & 0xff),
                      kAddrModeImmediate));
          Emit(g,
               NewInstruction1(W65C02_OP(sta),
                               ByteConst(g, W65C02_T0_REG),
                               kAddrModeZeroPageAbsolute));
          if (args_size >= 256) {
            Emit(g, NewInstruction1(
                        W65C02_OP(lda),
                        ByteConst(g, (args_size >> 8) & 0xff),
                        kAddrModeImmediate));
            Emit(g,
                 NewInstruction1(W65C02_OP(sta),
                                 ByteConst(g, W65C02_T1_REG),
                                 kAddrModeZeroPageAbsolute));
            incsp = g->incsp0;
         }
         break;
      }

      call = jsr(g, incsp);
    }
  }
  // The callee has returned (and restored its saved registers).  Copy the
  // value it left in the temp buffer into its real zero-page register home.
  // The copy stays in this same basic block (we move the block-end marker onto
  // its last instruction) so the buffer is never live across the call boundary
  // and therefore never forced into a preserved register.
  if (result_buf != NULL) {
    AddSpillPoint(g, result_buf);
    Copy(g, result, result_buf, 0, 0, result_size,
         GetAddrMode(result), GetAddrMode(result_buf));
    call = TargetLastInstruction(&g->base);
  }
  call->flags |= k6502InstIsCall | k6502BlockEnd;
  if (result == NULL) {
    result = call;
  }
  SetLoweredNode(node, result);
  AddSpillPoint(g, result);
}

// Input is the value of the result in zero page.
// Calls:
// JSR resulti (where X is 2,4,8)
static void AssignResult(W65C02Generator* g, TargetInstruction* inst, int size) {
  Symbol* result_func;
  switch (size) {
    case 1:
       ldazi(g, inst, 0);
       result_func = g->result1;
       break;
    case 2:
      ldazi(g, inst, 0);
      result_func = g->result2;
      break;
    case 4:
      ldazi(g, inst, 0);
      result_func = g->result4;
      break;
    case 8:
      ldazi(g, inst, 0);
      result_func = g->result8;
      break;
    default:
      abort();
      break;
  }
  jsr(g, result_func);
}

static void LowerResult(W65C02Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  int size = Sizeof(node->type);
  IRNode* result = node->inputs.value.p[0];
  if (result->opcode == IR_OP(calla) &&
      CanForwardTailCallArguments(g, result)) {
    return;
  }
  TargetInstruction* rnode = Materialize(g, result, -1, true);
  if (!IsLeaf(g)) {
    if (size == 1 || size == 2) {
      Emit(g, NewInstruction1(size == 1 ? W65C02_OP(load_result_value1)
                                        : W65C02_OP(load_result_value2),
                              rnode, kAddrModeImplied));
      return;
    }
    // For 2 byte frame size:
    // ldx #frame_size LO
    // ldy #frame_size HI
    // jsr __load_result+2
    //
    // For 1 byte frame size:
    // ldx #frame_size LO
    // jsr __load_result
    Emit(g, NewInstruction(W65C02_OP(load_result), kAddrModeImplied));
  }
  switch (node->opcode) {
    case IR_OP(resulti):
      AssignResult(g, rnode, size);
      break;
    case IR_OP(resulta):
      AssignResult(g, rnode, 2);
      break;
    case IR_OP(resultf):
      AssignResult(g, rnode, 4);
      break;
    case IR_OP(resultd):
      AssignResult(g, rnode, 4);
      break;
    default:
      assert(false);
      break;
  }
}

// A literal reference is a move of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static void LowerLiteralReference(W65C02Generator* g,
                                                IRNode* node, bool force) {
  // If we pushing this as an arg we don't lower it here.
  if (!force && node->outputs.length == 1) {
    IRNode* user = node->outputs.value.p[0];
    if (user->opcode == IR_OP(pusharg)) {
      return;
    }
  }
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, TargetNewLiteral((int)id_node->value.ivalue));
  TargetInstruction* dest = NULL;
  if (node->dest != NULL) {
    dest = GetAddress(g, node->dest, true);
  } else {
    dest = TempRegister(g, node->type, 2);
  }

  Emit(g, NewInstruction1(W65C02_OP(literalreflo), literal,
                          kAddrModeImplied));
  SetIndexReg(g, dest, dest, 0);
  sta(g, dest, 0);
  Emit(g, NewInstruction1(W65C02_OP(literalrefhi), literal,
                          kAddrModeImplied));
  SetIndexReg(g, dest, dest, 1);
  sta(g, dest, 1);
  SetLoweredNode(node, dest);
  AddSpillPoint(g, dest);
}

static TargetInstruction* AddressOfRegVariable(W65C02Generator* g, IRNode* node, IRNode* var) {
  TargetInstruction* result = GetLoweredNode(var);
  if (node->dest != NULL) {
    TargetInstruction* dest = GetLoweredNode(node->dest);
    Copy(g, dest, result, 0, 0, 2, GetAddrMode(dest), kAddrModeZeroPage);
    AddSpillPoint(g, dest);
    result = dest;
  }
  return SetLoweredNode(node, result);
}

static TargetInstruction* AddressOfVariable(W65C02Generator* g, IRNode* node, IRNode* var, bool is_arg) {
  var = IgnoreCasts(var);
  if (IsNRVONode(var)) {
    assert(g->struct_return_inst != NULL);
    TargetInstruction* result = g->struct_return_inst;
    if (node->dest != NULL) {
      TargetInstruction* dest = GetLoweredNode(node->dest);
      Copy(g, dest, result, 0, 0, 2, GetAddrMode(dest),
           GetAddrMode(result));
      AddSpillPoint(g, dest);
      result = dest;
    }
    return SetLoweredNode(node, result);
  }

  TargetInstruction* inst = GetLoweredNode(var);
  AddReloadPoint(g, inst);
  
  switch ((W65C02Opcode)inst->opcode) {
    case W65C02_OP(ivarreg):
    case W65C02_OP(bvarreg):
    case W65C02_OP(lvarreg):
    case W65C02_OP(xvarreg):
    case W65C02_OP(fvarreg):
    case W65C02_OP(dvarreg):
      return AddressOfRegVariable(g, node, var);
    default:
      break;
  }
  TargetInstruction* result = NULL;
  int offset;

  // Variable or argument.  Load value using one of the runtime helper
  // functions.
  offset = (int)TargetIntValue(inst->operand[0]);

  bool dest_set = false;
  // If we have a destination set and it's an expression we can write the
  // result of the var_addr into it.
  if (node->dest != NULL && IsExpression(GetLoweredNode(node->dest))) {
    result = GetLoweredNode(node->dest);
    dest_set = true;
  } else {
    result = TempRegister(g, node->type, 2);
  }
  W65C02Opcode addr_op;
  if (IRIsArgument(var)) {
      addr_op = offset >= 256 ? W65C02_OP(arg_addrb) : W65C02_OP(arg_addr);
  } else {
    addr_op = offset >= 256 ? W65C02_OP(var_addrb) : W65C02_OP(var_addr);
  }
  Emit(g,
        NewInstruction2(addr_op, result,
                        inst,
                        kAddrModeImplied));

  AddSpillPoint(g, result);
  if (!dest_set && node->dest != NULL) {
    // We have a location to put the result.
    TargetInstruction* dest = GetAddress(g, node->dest, true);
    Copy(g, dest, result, 0, 0, 2, GetAddrMode(dest), kAddrModeZeroPage);
    AddSpillPoint(g, dest);
    result = dest;
  }
  return SetLoweredNode(node, result);
}


static TargetInstruction* AddressOfStaticVariable(W65C02Generator* g, IRNode* node, IRNode* var) {
  TargetInstruction* result = GetAddress(g, var, false);
  if (node->dest != NULL) {
    TargetInstruction* dest = GetAddress(g, node->dest, true);
    Copy(g, dest, result, 0, 0, 2, GetAddrMode(dest), kAddrModeSymbolAddr);
    AddSpillPoint(g, dest);
    result = dest;
  }
  result->flags |= k6502NeedAddress;
  return SetLoweredNode(node, result);
}

static TargetInstruction* AddressOfExpression(W65C02Generator* g, IRNode* node, IRNode* expr) {
  TargetInstruction* result = GetAddress(g, expr, true);
  if (node->dest != NULL) {
    TargetInstruction* dest = GetAddress(g, node->dest, true);
    Copy(g, dest, result, 0, 0, 2, GetAddrMode(dest), GetAddrMode(result));
    AddSpillPoint(g, dest);
    result = dest;
  }
  result->flags |= k6502NeedAddress;
  return SetLoweredNode(node, result);
}

static void LowerAddressOf(W65C02Generator* g, IRNode* node) {
  if (node->outputs.length == 0 && node->dest == NULL) {
    return;
  }
  IRNode* src_node = node->inputs.value.p[0];
  bool is_arg = true;
  switch (src_node->opcode) {
    case IR_OP(localvar):
      is_arg = false;
    case IR_OP(argument):
       AddressOfVariable(g, node, src_node, is_arg);
      break;
    case IR_OP(externvar):
    case IR_OP(staticvar):
       AddressOfStaticVariable(g, node, src_node);
      break;
      
    default:
      AddressOfExpression(g, node, src_node);
      break;
  }
}

static void LowerMemcpy(W65C02Generator* g, IRNode* node) {
  assert(node->inputs.length == 3);
  Symbol* copymem = g->copymem1;

  // Put size in __mem_size.
  TargetInstruction* size_node = GetLoweredNode(node->inputs.value.p[2]);
  int64_t size = TargetIntValue(size_node);
  ldai(g, size & 0xff);
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       ByteConst(g, W65C02_MSZ_REG),
                       kAddrModeZeroPageAbsolute));
  if (size >= 256) {
    copymem = g->copymem2;
    ldai(g, (size >> 8) & 0xff);
    Emit(g, NewInstruction1(
                W65C02_OP(sta),
                ByteConst(g,  W65C02_MSZ_REG + 1),
                kAddrModeZeroPageAbsolute));
  }

  // Put src in __mem_src
  TargetInstruction* src = GetAddress(g, node->inputs.value.p[1], true);
  AddReloadPoint(g, src);
  lda(g, src, 0);
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       ByteConst(g, W65C02_MSRC_REG),
                       kAddrModeZeroPageAbsolute));
  lda(g, src, 1);
  Emit(g, NewInstruction1(
              W65C02_OP(sta),
              ByteConst(g,  W65C02_MSRC_REG + 1),
              kAddrModeZeroPageAbsolute));

  // Put dest in __mem_dest.
  TargetInstruction* dest = GetAddress(g, node->inputs.value.p[0], true);
  AddReloadPoint(g, dest);
  lda(g, dest, 0);
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       ByteConst(g, W65C02_MDST_REG),
                       kAddrModeZeroPageAbsolute));
  lda(g, dest, 1);
  Emit(g, NewInstruction1(
              W65C02_OP(sta),
              ByteConst(g,  W65C02_MDST_REG + 1),
              kAddrModeZeroPageAbsolute));

  // JSR copymem
  jsr(g, copymem);
}

static void LowerMemzero(W65C02Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  Symbol* zeromem = g->zeromem1;

  // Put size in __mem_size.
  IRNode* dest_node = node->inputs.value.p[0];
  int size = SizeofArray(node->type);
  ldai(g, size & 0xff);
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       ByteConst(g,  W65C02_MSZ_REG),
                       kAddrModeZeroPageAbsolute));
  if (size >= 256) {
    zeromem = g->zeromem2;
    ldai(g, (size >> 8) & 0xff);
    Emit(g, NewInstruction1(
                W65C02_OP(sta),
                ByteConst(g,  W65C02_MSZ_REG + 1),
                kAddrModeZeroPageAbsolute));
  }

  // Put dest in __mem_dest.
  TargetInstruction* dest = GetAddress(g, dest_node, true);
  AddReloadPoint(g, dest);
  lda(g, dest, 0);
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       ByteConst(g, W65C02_MDST_REG),
                       kAddrModeZeroPageAbsolute));
  lda(g, dest, 1);
  Emit(g, NewInstruction1(
              W65C02_OP(sta),
              ByteConst(g,  W65C02_MDST_REG + 1),
              kAddrModeZeroPageAbsolute));

  // JSR zeromem
  jsr(g, zeromem);
}

static void LowerZeroExtend(W65C02Generator* g, IRNode* node) {
  if (node->outputs.length == 0 && node->dest == NULL) {
    // No users of this, ignore.
    return;
  }
  IRNode* src_node = node->inputs.value.p[0];
  TargetInstruction* src = Materialize(g, src_node, -1, false);
  AddReloadPoint(g, src);
  TargetInstruction* dest = GetDestAddress(g, node, false);
  int src_size = Sizeof(src_node->type);
  int dest_size = Sizeof(node->type);

  // Second operand is number of bits to mask to.
  int bits = (int)IRIntConstValue(node->inputs.value.p[1]);
  if (bits > 0) {
    // Lengthen
    // E.g. from 2 to 4
    // lda src+0
    // sta dest+0
    // lda src+1
    // sta dest+1
    // lda #0
    // sta dest+2
    // sta dest+3
    for (int i = 0; i < src_size; i++) {
      SetIndexReg(g, src, dest, i);
      lda(g, src, i);
      sta(g, dest, i);
    }
    ldai(g, 0);

    int remaining = dest_size - src_size;
    if (remaining > 2 && IsExpression(dest)) {
      AddressingMode dest_mode = GetAddrMode(dest);
      if (dest_mode != kAddrModeIndirectIndexed && dest_mode != kAddrModeIndirect) {
        SetAddrMode(dest, kAddrModeZeroPageIndexedY);
      }
      ldyi(g, dest_size-1);
      TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
      sta(g, dest, -1);
      dey(g);
      cpyi(g, src_size-1);
      EmitResolvedBranch(g, W65C02_OP(bne), loop);
      SetAddrMode(dest, dest_mode);
    } else {
      for (int i = src_size; i < dest_size; i++) {
        SetIndexReg(g, dest, dest, i);
        sta(g, dest, i);
      }
    }
  } else {
    // Shorten
    // E.g. from 2 to 1
    // lda src+0
    // sta dest+0
    for (int i = 0; i < dest_size; i++) {
      SetIndexReg(g, src, dest, i);
      lda(g, src, i);
      sta(g, dest, i);
    }
  }
  SetLoweredNode(node, dest);
  AddSpillPoint(g, dest);
}

static void LowerSignExtend(W65C02Generator* g, IRNode* node) {
  if (node->outputs.length == 0 && node->dest == NULL) {
    // No users of this, ignore.
    return ;
  }
  IRNode* src_node = node->inputs.value.p[0];
  int src_size = Sizeof(src_node->type);
  TargetInstruction* src = GetAddress(g, src_node, true);
  AddReloadPoint(g, src);

  TargetInstruction* dest = GetDestAddress(g, node, false);
  AddReloadPoint(g, dest);

  IRConstant* diff_value = node->inputs.value.p[1];
  int diff = (int)diff_value->value.ivalue;  // In bits.
  diff /= 8;                                 // In bytes.
  int dest_size = src_size + diff;
  if (diff > 0) {
    // Lengthen int.
    // Say from 2 to 4 bytes.
    //  lda src+0
    //  sta dest+0
    //  lda src+1
    //  sta dest+1
    //  AND #0x80
    //  BEQ xx
    //  LDA #255
    // xx:
    //  sta dest+2
    //  sta dest+3

    for (int i = 0; i < src_size; i++) {
      SetIndexReg(g, src, dest, i);
      lda(g, src, i);
      sta(g, dest, i);
    }
    Emit(g, NewInstruction1(W65C02_OP(and),
                            ByteConst(g,  0x80),
                            kAddrModeImmediate));
    TargetInstruction* label =
        NewInstruction(W65C02_OP(label), kAddrModeImplied);
    EmitResolvedBranch(g, W65C02_OP(beq), label);
    ldai(g, 255);
    Emit(g, label);

    int remaining = dest_size - src_size;
    if (remaining > 2) {
      AddressingMode old = GetAddrMode(dest);
      switch (old) {
        case kAddrModeZeroPage:
          SetAddrMode(dest, kAddrModeZeroPageIndexedY);
          break;
        case kAddrModeIndirectIndexed:
          break;
        case kAddrModeAbsoluteSymbol:
          SetAddrMode(dest, kAddrModeAbsoluteIndexedY);
          break;
        default:
          abort();
      }
      ldyi(g, dest_size-1);
      TargetInstruction* loop = Emit(g, NewInstruction(W65C02_OP(label), kAddrModeImplied));
      sta(g, dest, -1);
      dey(g);
      cpyi(g, src_size-1);
      EmitResolvedBranch(g, W65C02_OP(bne), loop);
      SetAddrMode(dest, old);
    } else {
      for (int i = src_size; i < dest_size; i++) {
        SetIndexReg(g, dest, dest, i);
        sta(g, dest, i);
      }
    }
  } else {
    // Shorten
    // Say from 4 to 2 bytes
    // we just need to set the top bit of the second byte of the dest if the top
    // bit of the 4th byte of the src is set.
    //
    // lda src+0
    // sta dest+0
    // lda src+3
    // AND #0x80
    // ORA src+1
    // sta dest+1

    for (int i = 0; i < dest_size - 1; i++) {
      SetIndexReg(g, src, dest, i);
      lda(g, src, i);
      sta(g, dest, i);
    }

    Emit(g,
         NewInstruction2(W65C02_OP(lda), src,
                         ByteConst(g, src_size - 1),
                         GetAddrMode(src)));
    Emit(g, NewInstruction1(W65C02_OP(and),
                            ByteConst(g, 0x80),
                            kAddrModeImmediate));
    Emit(g, NewInstruction2(
                W65C02_OP(ora), src,
                ByteConst(g,  dest_size - 1),
                GetAddrMode(src)));
    Emit(g, NewInstruction2(
                W65C02_OP(sta), dest,
                ByteConst(g, dest_size - 1),
                GetAddrMode(dest)));
  }
  SetLoweredNode(node, dest);
  AddSpillPoint(g, dest);
}

static void LowerAlign(W65C02Generator* g, IRNode* node) {
   SetLoweredNode(node, GetLoweredNode(node->inputs.value.p[0]));
}

static void LowerAsm(W65C02Generator* g, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, TargetNewLiteral((int)id_node->value.ivalue));

  AsmASTNode* asm_node = node->aux;
  if (asm_node == NULL ||
      (asm_node->outputs.length == 0 && asm_node->inputs.length == 0 &&
       asm_node->clobbers.length == 0 && asm_node->labels.length == 0)) {
    if (node->inputs.length > 1) {
      W65C02AsmInstruction* asm_inst = malloc(sizeof(W65C02AsmInstruction));
      TargetInitInstruction(&asm_inst->base, (TargetOpcode)W65C02_OP(asm));
      SetAddrMode(&asm_inst->base, kAddrModeImplied);
      asm_inst->base.flags |= k6502ExtendedAsm;
      asm_inst->base.operand[0] = literal;
      asm_inst->asm_node = NULL;
      asm_inst->num_operands = (int)node->inputs.length - 1;
      memset(asm_inst->immediate_values, 0, sizeof(asm_inst->immediate_values));
      memset(asm_inst->is_immediate, 0, sizeof(asm_inst->is_immediate));
      memset(asm_inst->zp_operands, 0, sizeof(asm_inst->zp_operands));
      for (size_t i = 1; i < node->inputs.length && i <= W65C02_MAX_ASM_OPERANDS; i++) {
        IRNode* input_node = node->inputs.value.p[i];
        if (IRIsIntConst(input_node)) {
          asm_inst->is_immediate[i - 1] = true;
          asm_inst->immediate_values[i - 1] = IRIntConstValue(input_node);
        }
      }
      TargetInstruction* result = Emit(g, &asm_inst->base);
      SetLoweredNode(node, result);
      return;
    }
    TargetInstruction* result =
        Emit(g, NewInstruction1(W65C02_OP(asm), literal, kAddrModeImplied));

    SetLoweredNode(node, result);
    return;
  }

  assert(asm_node->outputs.length + asm_node->inputs.length <= W65C02_MAX_ASM_OPERANDS);
  W65C02AsmInstruction* asm_inst = malloc(sizeof(W65C02AsmInstruction));
  TargetInitInstruction(&asm_inst->base, (TargetOpcode)W65C02_OP(asm));
  SetAddrMode(&asm_inst->base, kAddrModeImplied);
  asm_inst->base.flags |= k6502ExtendedAsm;
  asm_inst->base.operand[0] = literal;
  asm_inst->asm_node = asm_node;
  asm_inst->num_operands = (int)(asm_node->outputs.length + asm_node->inputs.length);
  memset(asm_inst->immediate_values, 0, sizeof(asm_inst->immediate_values));
  memset(asm_inst->is_immediate, 0, sizeof(asm_inst->is_immediate));
  memset(asm_inst->zp_operands, 0, sizeof(asm_inst->zp_operands));

  size_t ir_index = 1;
  for (size_t i = 0; i < asm_node->outputs.length; i++) {
    AsmOperand* operand = asm_node->outputs.value.p[i];
    IRNode* addr_node = node->inputs.value.p[ir_index++];
    int size = Sizeof(operand->expr->type);
    TargetInstruction* temp = TempRegister(g, operand->expr->type, size);
    AddSpillPoint(g, temp);
    asm_inst->zp_operands[i] = temp;
    TargetAddUser(temp, &asm_inst->base);
    if (operand->is_readwrite) {
      TargetInstruction* addr = GetAddress(g, addr_node, true);
      Copy(g, temp, addr, 0, 0, size, GetAddrMode(temp), GetAddrMode(addr));
      AddSpillPoint(g, temp);
    }
  }

  for (size_t i = 0; i < asm_node->inputs.length; i++) {
    AsmOperand* operand = asm_node->inputs.value.p[i];
    IRNode* input_node = node->inputs.value.p[ir_index++];
    int operand_index = (int)(asm_node->outputs.length + i);
    if (strchr(operand->constraint.value, 'i') != NULL && IRIsIntConst(input_node)) {
      asm_inst->is_immediate[operand_index] = true;
      asm_inst->immediate_values[operand_index] = IRIntConstValue(input_node);
      continue;
    }
    TargetInstruction* temp =
        Materialize(g, input_node, Sizeof(operand->expr->type), true);
    asm_inst->zp_operands[operand_index] = temp;
    TargetAddUser(temp, &asm_inst->base);
  }

  TargetInstruction* result = Emit(g, &asm_inst->base);
  for (size_t i = 0; i < asm_node->outputs.length; i++) {
    AsmOperand* operand = asm_node->outputs.value.p[i];
    IRNode* addr_node = node->inputs.value.p[1 + i];
    int size = Sizeof(operand->expr->type);
    TargetInstruction* addr = GetAddress(g, addr_node, true);
    TargetInstruction* temp = asm_inst->zp_operands[i];
    AddReloadPoint(g, temp);
    Copy(g, addr, temp, 0, 0, size, GetAddrMode(addr), GetAddrMode(temp));
  }

  SetLoweredNode(node, result);
}

static void LowerLocation(W65C02Generator* g, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  SetLoweredNode(node, Emit(g, TargetNewLocation(loc)));
}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last function argument.  The ap variable is set to the
// address of the last argument + 2.
static void LowerBuiltinVaStart(W65C02Generator* g, IRNode* node) {
  TargetInstruction* ap = GetAddress(g, node->inputs.value.p[0], true);
  TargetInstruction* arg = GetAddress(g, node->inputs.value.p[1], true);
  SetAddrMode(arg, kAddrModeZeroPage);
  
  AddSubInteger(g, node, W65C02_OP(adc), W65C02_OP(clc), 2, ap, arg, ByteConst(g, 2));
  
   SetLoweredNode(node, ap);
}

static void LowerBuiltinVaArg(W65C02Generator* g, IRNode* node) {
  IRNode* ap_node = node->inputs.value.p[0];
#if 0
  
#if 0
  // Load value of first arg.
  TargetInstruction* ap_addr = GetAddress(g, ap_node, true);
  AddReloadPoint(g, ap_addr);
  
  // Load the contents of ap into a tmp.
  TargetInstruction* ap = TempRegister(g, node->type, 2);
  AddReloadPoint(g, ap);

  for (int i = 0; i < 2; i++) {
    SetIndexReg(g, ap_addr, ap, i);
    lda(g, ap_addr, i);
    sta(g, ap, i);
  }
#endif
  // Load the value of ap.
  TargetInstruction* ap = Materialize(g, ap_node, 2, true);
  
  // Now load the contents of the arg, via ap.
  TargetInstruction* dest = GetDestAddress(g, node, false);
  AddReloadPoint(g, dest);
  Copy(g, dest, ap, 0, 0, Sizeof(node->type), GetAddrMode(dest), kAddrModeIndirectIndexed);
  AddSpillPoint(g, dest);
#if 0
    for (int i = 0; i < size; i++) {
      SetIndexReg(g, ap, dest, i);
      // Load the contents of (ap).
      AddressingMode mode = i == 0 ? kAddrModeIndirect : kAddrModeIndirectIndexed;
      Emit(g, NewInstruction2(W65C02_OP(lda), ap, ByteConst(g, i), mode));
      sta(g, dest, i);
    }
#endif
  
  // Increment ap by size.
  int size = (int)IRIntConstValue(node->inputs.value.p[1]);
  if (size < 2) {
    size = 2;         // Min 2 bytes.
  }
#if 0
  AddSubInteger(g, node, W65C02_OP(adc), W65C02_OP(clc), 2, ap_addr, ap, ByteConst(g,  size));
  AddSpillPoint(g, ap);
  AddSpillPoint(g, dest);
#endif
  
  AddSubInteger(g, node, W65C02_OP(adc), W65C02_OP(clc), 2, ap, ap, ByteConst(g, size));
  AddSpillPoint(g, ap);
  
  // In order to store the new value of ap back into the ap_node we need an
  // IRNode.  We don't have one because there's no IR_STORE node for this.
  // We have rhe second arg for the builtin_va_arg that we've already used
  // to get the size.  We can use that node if we convert it to something that
  // refers to the new value of ap.
  IRNode* new_ap_node = node->inputs.value.p[1];
  // Can't use SetLoweredNode becuase it's already been set.  Bypass the check.
  new_ap_node->data.ptr = ap;
  
  // Now store the new value of ap back.
  bool is_arg = false;
  switch (ap_node->opcode) {
    case IR_OP(argument):
      is_arg = true;
    case IR_OP(localvar):
      return StoreIntoVariable(g, node, ap_node, new_ap_node, is_arg, 2);

    case IR_OP(staticvar):
    case IR_OP(externvar):
      return StoreIntoStaticVariable(g, node, new_ap_node, ap_node, 2);
    default:
      return StoreIndirect(g, node, ap_node, new_ap_node, size, 0);
  }
  return SetLoweredNode(node, dest);
#endif
  // Call the __builtin_va_arg intrinsic.
  // __mem_src: address of ap
  // X,Y: size of type
  
  // Load __mem_src.  It will be zero page.
  TargetInstruction* ap_addr = GetAddress(g, ap_node, true);
  AddReloadPoint(g, ap_addr);
  SetAddrMode(ap_addr, kAddrModeZeroPage);
  
  lda(g, ap_addr, 0);
  Emit(g,
       NewInstruction1(W65C02_OP(sta),
                       ByteConst(g, W65C02_MSRC_REG),
                       kAddrModeZeroPageAbsolute));
  lda(g, ap_addr, 1);
  Emit(g, NewInstruction1(
              W65C02_OP(sta),
              ByteConst(g,  W65C02_MSRC_REG + 1),
              kAddrModeZeroPageAbsolute));
  
  // Load __mem_dest.  This will point to a zero page
  TargetInstruction* dest = GetDestAddress(g, node, true);
  AddReloadPoint(g, dest);
  
  if (DestInZeroPage(dest)) {
    ldazi(g, dest, 0);
    Emit(g,
         NewInstruction1(W65C02_OP(sta),
                         ByteConst(g, W65C02_MDST_REG),
                         kAddrModeZeroPageAbsolute));
    Emit(g, NewInstruction1(
                W65C02_OP(stz),
                ByteConst(g,  W65C02_MDST_REG + 1),
                kAddrModeZeroPageAbsolute));
    
  } else {
    SetIndexReg(g, dest, dest, 0);
    lda(g, dest, 0);
    Emit(g, NewInstruction1(
              W65C02_OP(stz),
              ByteConst(g,  W65C02_MDST_REG),
              kAddrModeZeroPageAbsolute));
    SetIndexReg(g, dest, dest, 1);
    lda(g, dest, 1);
    Emit(g, NewInstruction1(
              W65C02_OP(stz),
              ByteConst(g,  W65C02_MDST_REG + 1),
              kAddrModeZeroPageAbsolute));
    
  }
  
  int size = (int)IRIntConstValue(node->inputs.value.p[1]);
  if (size < 2) {
    size = 2;         // Min 2 bytes.
  }

  Symbol* va_arg_sym;
  switch (size) {
    case 2:
      va_arg_sym = g->builtin_va_arg2;
      break;
    case 4:
      va_arg_sym = g->builtin_va_arg4;
      break;
    case 8:
      va_arg_sym = g->builtin_va_arg8;
      break;
    default:
      va_arg_sym = g->builtin_va_arg;
      ldxi(g, size & 0xff);
      ldyi(g, (size >> 8) & 0xff);
      break;
  }

  jsr(g, va_arg_sym);

  SetLoweredNode(node, dest);
}

static void LowerBuiltinVaEnd(W65C02Generator* g, IRNode* node) {
  // Nothing to do for va_end.
}

static void LowerBuiltinVaCopy(W65C02Generator* g, IRNode* node) {
}

static void CompareEqualIntegerZeroExpression(W65C02Generator* g, IRNode* node,
                                          TargetInstruction* dest, int size) {
  // For 2 byte:
  //  ldx #1
  //  lda byte1
  //  ora byte2
  //  BEQ true_label
  // false_label:
  //  DEX
  // true_label:
  //  txa
  //  sta result

  TargetInstruction* value = GetAddress(g, node->inputs.value.p[0], true);
  AddReloadPoint(g, value);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
   }
  cmpi(g, 0);
  EmitResolvedBranch(g, W65C02_OP(beq), true_label);
  Emit(g, false_label);
  dex(g);
  Emit(g, true_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}

static void CompareEqualIntegerExpression(W65C02Generator* g, IRNode* node,
                                          TargetInstruction* dest, int size) {
  // For 2 byte:
  //  ldx #1
  //  lda byte1
  //  CMP value1
  //  BNE false_label
  //  lda byte2
  //  CMP value2
  //  BEQ true_label
  // false_label:
  //  DEX
  // true_label:
  //  txa
  //  sta result

  TargetInstruction* value1 = GetAddress(g, node->inputs.value.p[0], true);
  TargetInstruction* value2 = GetAddress(g, node->inputs.value.p[1], true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    if (i < (size - 1)) {
      EmitResolvedBranch(g, W65C02_OP(bne), false_label);
    } else {
      EmitResolvedBranch(g, W65C02_OP(beq), true_label);
    }
  }
  Emit(g, false_label);
  dex(g);
  Emit(g, true_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}
     
static void CompareNotEqualIntegerZeroExpression(W65C02Generator* g, IRNode* node,
                                          TargetInstruction* dest, int size) {
  // For 2 byte:
  //  ldx #1
  //  lda byte1
  //  ora byte2
  //  BNE true_label
  // false_label:
  //  DEX
  // true_label:
  //  txa
  //  sta result

  TargetInstruction* value = GetAddress(g, node->inputs.value.p[0], true);
  AddReloadPoint(g, value);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
   }
  cmpi(g, 0);
  EmitResolvedBranch(g, W65C02_OP(bne), true_label);
  Emit(g, false_label);
  dex(g);
  Emit(g, true_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}

static void CompareNotEqualIntegerExpression(W65C02Generator* g, IRNode* node,
                                          TargetInstruction* dest, int size) {
  // For 2 byte:
  //  ldx #0
  //  lda byte1
  //  CMP value1
  //  BNE true_label
  //  lda byte2
  //  CMP value2
  //  BEQ false_label
  // true_label:
  //  INX
  // false_label:
  //  txa
  //  sta result

  TargetInstruction* value1 = GetAddress(g, node->inputs.value.p[0], true);
  TargetInstruction* value2 = GetAddress(g, node->inputs.value.p[1], true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 0);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    if (i < (size - 1)) {
      EmitResolvedBranch(g, W65C02_OP(bne), true_label);
    } else {
      EmitResolvedBranch(g, W65C02_OP(beq), false_label);
    }
  }
  Emit(g, true_label);
  inx(g);
  Emit(g, false_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}

static void CompareLessUnsignedIntegerExpression(W65C02Generator* g, IRNode* lhs,
                                                 IRNode* rhs,
                                          TargetInstruction* dest, int size) {
  //  ldx #1
  //  lda byte2
  //  CMP value2
  //  BCC true_label
  //  BNE false_label
  //  lda byte1
  //  CMP value1
  //  BCC true_label
  // false_label:
  //  DEX
  // true_label:
  // stx dest


  TargetInstruction* value1 = GetAddress(g, lhs, true);
  TargetInstruction* value2 = GetAddress(g, rhs, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = size-1; i >= 0; i--) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i)->flags |= k6502GeneratesFlags;
    EmitResolvedBranch(g, W65C02_OP(bcc), true_label);
    if (i > 0) {
      EmitResolvedBranch(g, W65C02_OP(bne), false_label);
    }
  }
  Emit(g, false_label);
  dex(g);
  Emit(g, true_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}

// Compares 2 signed integers for < or >=.  Puts result in dest.
static void CompareSignedIntegerExpression(W65C02Generator* g, IRNode* lhs,
                                                 IRNode* rhs,
                                          TargetInstruction* dest, int size,
                                           bool less_than) {
  //  ldx #0
  //  sec
  //  lda byte1
  //  CMP value1
  //  lda byte2
  //  sbc value2
  //  bvc skip_label
  //  eor #0x80
  // skip_label:              N = 1 means less
  //  bpl false_label [or bmi for >=)
  //  inx
  // false_label:
  //  stx dest

  TargetInstruction* value1 = GetAddress(g, lhs, true);
  TargetInstruction* value2 = GetAddress(g, rhs, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 0);
  CompareSignedSubtract(g, value1, value2, size);
  EmitResolvedBranch(g, less_than ? W65C02_OP(bpl) : W65C02_OP(bmi), false_label);
  inx(g);
  Emit(g, false_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}

// Sets dest to 1 if lhs <= rhs (signed), else 0.
static void CompareSignedLessOrEqualExpression(W65C02Generator* g, IRNode* lhs,
                                               IRNode* rhs,
                                               TargetInstruction* dest,
                                               int size) {
  TargetInstruction* value1 = GetAddress(g, lhs, true);
  TargetInstruction* value2 = GetAddress(g, rhs, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* end_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 0);
  CompareSignedSubtract(g, value1, value2, size);
  EmitResolvedBranch(g, W65C02_OP(bmi), true_label);
  EmitResolvedBranch(g, W65C02_OP(beq), true_label);
  Emit(g, false_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
  EmitResolvedBranch(g, W65C02_OP(bra), end_label);
  Emit(g, true_label);
  inx(g);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
  Emit(g, end_label);
}


static void CompareGreaterOrEqualUnsignedIntegerExpression(W65C02Generator* g,
                                                 IRNode* lhs, IRNode* rhs,
                                                 TargetInstruction* dest, int size) {
  // For 4 byte:
  //  ldx #1
  //  lda byte4
  //  CMP value4
  //  BCC false_label
  //  BNE true_label
  //  lda byte3
  //  CMP value3
  //  BCC false_label
  //  BNE true_label
  //  lda byte2
  //  CMP value2
  //  BCC false_label
  //  BNE true_label
  //  lda byte1
  //  CMP value1
  //  BCS true_label
  // false_label:
  //  dex
  // true_label:
  //  stx dest

  TargetInstruction* value1 = GetAddress(g, lhs, true);
  TargetInstruction* value2 = GetAddress(g, rhs, true);
  AddReloadPoint(g, value1);
  AddReloadPoint(g, value2);
  TargetInstruction* false_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  for (int i = size - 1; i >= 0; i--) {
    SetIndexRegDown(g, value1, value2, i, size);

    lda(g, value1, i);
    cmp(g, value2, i)->flags |= k6502GeneratesFlags;
    if (i == 0) {
      EmitResolvedBranch(g, W65C02_OP(bcs), true_label);
    } else {
      EmitResolvedBranch(g, W65C02_OP(bcc), false_label);
      EmitResolvedBranch(g, W65C02_OP(bne), true_label);
    }
  }
  Emit(g, false_label);
  dex(g);
  Emit(g, true_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}

static void LowerCast(W65C02Generator* g, IRNode* node) {
  TargetInstruction* expr = GetLoweredNode(node->inputs.value.p[0]);
  if (node->dest != NULL) {
    LowerIRNode(g, node->dest);
    TargetInstruction* dest = GetLoweredNode(node->dest);
    Copy(g, dest, expr, 0, 0, Sizeof(node->type), GetAddrMode(dest), GetAddrMode(expr));
    return;
  }
  SetLoweredNode(node, expr);
}

// Multi-byte unsigned subtract value1 - value2.  Afterwards the carry is set
// iff value1 >= value2 (unsigned) and clear iff value1 < value2.
static void CompareUnsignedSubtract(W65C02Generator* g,
                                    TargetInstruction* value1,
                                    TargetInstruction* value2, int size) {
  sec(g);
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    sbc(g, value2, i);
  }
}

// Lower a three-way comparison (operator<=>) to a sign-extended integer
// -1/0/1 in the destination.  The 6502 has no floating-point support, so float
// operands fall through to the signed-integer path (matching the rest of the
// comparison lowering, which treats float bits as a signed integer).
static void LowerThreeWay(W65C02Generator* g, IRNode* node) {
  IRNode* lhs = node->inputs.value.p[0];
  IRNode* rhs = node->inputs.value.p[1];
  int size = Sizeof(lhs->type);
  bool is_unsigned =
      node->opcode == IR_OP(cmp3wayu) || node->opcode == IR_OP(cmp3waya);
  TargetInstruction* dest = GetDestAddress(g, node, false);
  AddReloadPoint(g, dest);
  TargetInstruction* a = GetAddress(g, lhs, true);
  TargetInstruction* b = GetAddress(g, rhs, true);
  AddReloadPoint(g, a);
  AddReloadPoint(g, b);

  TargetInstruction* skip_less =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  TargetInstruction* skip_greater =
      NewInstruction(W65C02_OP(label), kAddrModeImplied);
  // X accumulates the result (-1/0/1).  It survives the compares, which only
  // touch the accumulator and the flags.  Note that if a < b then a > b is
  // false, so the second test never overwrites the -1.
  ldxi(g, 0);
  if (is_unsigned) {
    CompareUnsignedSubtract(g, a, b, size);
    EmitResolvedBranch(g, W65C02_OP(bcs), skip_less);  // a >= b -> not less
  } else {
    CompareSignedSubtract(g, a, b, size);
    EmitResolvedBranch(g, W65C02_OP(bpl), skip_less);  // a >= b -> not less
  }
  ldxi(g, 0xFF);  // a < b  -> -1
  Emit(g, skip_less);
  if (is_unsigned) {
    CompareUnsignedSubtract(g, b, a, size);
    EmitResolvedBranch(g, W65C02_OP(bcs), skip_greater);  // b >= a -> not greater
  } else {
    CompareSignedSubtract(g, b, a, size);
    EmitResolvedBranch(g, W65C02_OP(bpl), skip_greater);
  }
  ldxi(g, 1);  // a > b  -> 1
  Emit(g, skip_greater);

  // Store the result byte (X), sign-extended to the destination's full width.
  int dest_size = Sizeof(node->type);
  txa(g);
  SetIndexReg(g, dest, dest, 0);
  sta(g, dest, 0);
  if (dest_size > 1) {
    andi(g, 0x80);
    TargetInstruction* non_negative =
        NewInstruction(W65C02_OP(label), kAddrModeImplied);
    EmitResolvedBranch(g, W65C02_OP(beq), non_negative);
    ldai(g, 0xFF);
    Emit(g, non_negative);
    for (int i = 1; i < dest_size; i++) {
      SetIndexReg(g, dest, dest, i);
      sta(g, dest, i);
    }
  }
  dest->flags |= k6502ComparisonGenerated;
  SetLoweredNode(node, dest);
}

static void LowerComparison(W65C02Generator* g, IRNode* node) {
  // Check if any the outputs of the node are not branches.  If all
  // the uses are branches we defer the generation of the comparison
  // to the branch.
  //
  bool is_expression = node->dest != NULL;
  for (size_t i = 0; !is_expression && i < node->outputs.length; i++) {
    IRNode* output = node->outputs.value.p[i];
    if (!IRIsConditionalBranch(output)) {
      is_expression = true;
      break;
    }
  }
  if (!is_expression) {
    return;
  }
  TargetInstruction* dest = GetDestAddress(g, node, false);
  // Size is the size of the inputs.  They will all be the same.
  IRNode* op1 = node->inputs.value.p[0];
  int size = Sizeof(op1->type);
  bool is_unsigned = TypeIsUnsigned(op1->type);
  AddReloadPoint(g, dest);

  IRNode* lhs = node->inputs.value.p[0];
  IRNode* rhs = node->inputs.value.p[1];
  switch (node->opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
    case IR_OP(cmpeqf):
    case IR_OP(cmpeqd):
      if (IRIsZero(node->inputs.value.p[1])) {
        CompareEqualIntegerZeroExpression(g, node, dest, size);
      } else {
        CompareEqualIntegerExpression(g, node, dest, size);
      }
      break;
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
    case IR_OP(cmpnef):
    case IR_OP(cmpned):
      if (IRIsZero(node->inputs.value.p[1])) {
        CompareNotEqualIntegerZeroExpression(g, node, dest, size);
      } else {
        CompareNotEqualIntegerExpression(g, node, dest, size);
      }
      break;
    case IR_OP(cmplti):
    case IR_OP(cmplta):
    case IR_OP(cmpltf):
    case IR_OP(cmpltd):
      if (is_unsigned) {
        CompareLessUnsignedIntegerExpression(g, lhs, rhs, dest, size);
      } else {
        CompareSignedIntegerExpression(g, lhs, rhs, dest, size, true);
      }
      break;
    case IR_OP(cmplei):
    case IR_OP(cmplea):
    case IR_OP(cmplef):
    case IR_OP(cmpled):
      if (is_unsigned) {
        CompareGreaterOrEqualUnsignedIntegerExpression(g, rhs, lhs, dest, size);
      } else {
        CompareSignedLessOrEqualExpression(g, lhs, rhs, dest, size);
      }
      break;
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgtd):
      // Same as less with args reversed.
      if (is_unsigned) {
        CompareLessUnsignedIntegerExpression(g, rhs, lhs, dest, size);
      } else {
        CompareSignedIntegerExpression(g, rhs, lhs, dest, size, true);
      }
      break;
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
    case IR_OP(cmpgef):
    case IR_OP(cmpged):
      if (is_unsigned) {
        CompareGreaterOrEqualUnsignedIntegerExpression(g, lhs, rhs, dest, size);
      } else {
        CompareSignedIntegerExpression(g, lhs, rhs, dest, size, false);
      }
      break;
#if 0
    case IR_OP(cmpeqf):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpeqf);
      break;
    case IR_OP(cmpnef):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpnef);
      break;
    case IR_OP(cmpltf):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpltf);
      break;
    case IR_OP(cmplef):
      CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpgef);
      break;
    case IR_OP(cmpgtf):
      CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpltf);
      break;
    case IR_OP(cmpgef):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpgef);
      break;
    case IR_OP(cmpeqd):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpeqf);
      break;
    case IR_OP(cmpned):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpnef);
      break;
    case IR_OP(cmpltd):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpltf);
      break;
    case IR_OP(cmpled):
      CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpgef);
      break;
    case IR_OP(cmpgtd):
      CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpltf);
      break;
    case IR_OP(cmpged):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpgef);
      break;
#endif
    default:
      abort();
  }
  // Mark result as having comparison generated.
  dest->flags |= k6502ComparisonGenerated;
   SetLoweredNode(node, dest);
}

static void LowerSSAVar(W65C02Generator* g, IRNode* node) {
  IRVariable* var = (IRVariable*)node;
  IRNode* symbol = FindPooledVariable(g->gen, var->symbol);
   SetLoweredNode(node, Emit(g, NewInstruction1(W65C02_OP(ssavar), GetLoweredNode(symbol), kAddrModeImplied)));
}

static void LowerPhiNode(W65C02Generator* g, IRNode* node) {
  IRVariable* var = (IRVariable*)node;
  IRNode* symbol = FindPooledVariable(g->gen, var->symbol);
   SetLoweredNode(node, Emit(g, NewInstruction1(W65C02_OP(phi), GetLoweredNode(symbol), kAddrModeImplied)));
}

static void LowerStackPointerOps(W65C02Generator* g, IRNode* node) {
  switch (node->opcode) {
    case IR_OP(decsp): {
      // ldx #reg
      // jsr __decsp
      TargetInstruction* size = Materialize(g, node->inputs.value.p[0], 2, true);
      Emit(g, NewInstruction1(W65C02_OP(expr_addr_x), size, kAddrModeImplied));
       SetLoweredNode(node, jsr(g, g->decsp));
      break;
    }
      
    case IR_OP(savesp): {
      // One operand, a temp to hold stack pointer.
      TargetInstruction* tmp = GetAddress(g, node->inputs.value.p[0], true);
      Emit(g, NewInstruction1(W65C02_OP(expr_addr_x), tmp, kAddrModeImplied));
       SetLoweredNode(node, jsr(g, g->savesp));
      break;
    }
    case IR_OP(restoresp): {
      TargetInstruction* tmp = GetAddress(g, node->inputs.value.p[0], true);
      Emit(g, NewInstruction1(W65C02_OP(expr_addr_x), tmp, kAddrModeImplied));
       SetLoweredNode(node, jsr(g, g->restoresp));
      break;
    }
    default:
      assert(false);
  }
  
}

static void LowerStructReturn(W65C02Generator* g, IRNode* node) {
  TargetInstruction* inst = Emit(g, NewInstruction(W65C02_OP(structreturn), kAddrModeZeroPage));
  g->struct_return_inst = inst;
   SetLoweredNode(node, inst);
}

static void LowerIRNode(W65C02Generator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return;
  }
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
      return;

    case IR_OP(structreturn):
      return LowerStructReturn(g, node);

    case IR_OP(structarg):
      // Same as its input.
       SetLoweredNode(node, GetAddress(g, node->inputs.value.p[0], true));
      return;

    case IR_OP(nop):
    case last_ir_opcode:
      return ;

    case IR_OP(ssavar):
      return LowerSSAVar(g, node);
      
    case IR_OP(phi):
      return LowerPhiNode(g, node);

    case IR_OP(literalref):
      return LowerLiteralReference(g, node, false);

    case IR_OP(addressof):
      return LowerAddressOf(g, node);

    case IR_OP(const32): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetType32Bit, c->value.ivalue);
      AddLiteral(g, (TargetConstant*)inst, &c->value.ivalue, 4);
      return ;
    }
    case IR_OP(const8): {
      IRConstant* c = (IRConstant*)node;
      (void)GetIntConstant(g, node, kTargetType8Bit, c->value.ivalue);
      return ;
    }
    case IR_OP(const16): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetType16Bit, c->value.ivalue);
      AddLiteral(g, (TargetConstant*)inst, &c->value.ivalue, 4);
      return ;
    }
    case IR_OP(consta): {
      IRConstant* c = (IRConstant*)node;
      (void)GetIntConstant(g, node, kTargetType32Bit, c->value.ivalue);
      return ;
    }
    case IR_OP(const64): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetType64Bit, c->value.ivalue);
      AddLiteral(g, (TargetConstant*)inst, &c->value.ivalue, 8);
      return ;
    }
    case IR_OP(constf): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeFloat, c->value.ivalue);
      float v = c->value.fvalue;
      AddLiteral(g, (TargetConstant*)inst, &v, 4);
      return ;
    }
    case IR_OP(constd): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeDouble, c->value.ivalue);
      float v = c->value.fvalue;
      AddLiteral(g, (TargetConstant*)inst, &v, 4);
      return ;
    }
    case IR_OP(enter): {
      if (CanElideForwardingTailFrame(g)) {
        LowerVariables(g);
        return;
      }
      // Entry sequence.  This macro instruction expands to either
      //   lda #frame_size ; jsr __enter*_res      (value-returning, <256)
      // where the runtime stores the X,Y result address into __result, or
      //   [stx/sty __result] ; ldx/ldy #size ; jsr __enter*
      // for void/aggregate returns and large frames.  The emitter decides
      // based on the flag below and the final frame size.
      {
        TargetInstruction* enter_inst =
            NewInstruction(IsLeaf(g) ? W65C02_OP(enter_leaf)
                                     : W65C02_OP(enter),
                           kAddrModeImplied);
        if (!TypeIsVoid(compiler->current_function->next) &&
            !TypeIsStructOrUnion(compiler->current_function->next)) {
          enter_inst->flags |= k6502EnterStoresResult;
        }
        Emit(g, enter_inst);
      }
      // The hidden aggregate-result pointer is loaded by a runtime helper that
      // uses caller-clobbered value registers. Load it before ordinary
      // arguments are assigned to those registers.
      if (g->gen->struct_return_value != NULL) {
        LowerStructReturn(g, g->gen->struct_return_value);
      }
      LowerVariables(g);
      return ;
    }
    case IR_OP(leave): {
      // Exit sequence (non-leaf)
      // This macro instruction expands to:
      // ldx #frame_size lo
      // ldy #frame-size hi
      // JSR __leave      
      if (IsLeaf(g)) {
        Emit(g, NewInstruction(W65C02_OP(leave_leaf), kAddrModeImplied));
      } else {
        Emit(g, NewInstruction(W65C02_OP(leave), kAddrModeImplied));
      }
      return ;
    }

    case IR_OP(ret):
       Emit(g, NewInstruction(W65C02_OP(rts), kAddrModeImplied));
      return;
      
    case IR_OP(load32):
    case IR_OP(load8):
    case IR_OP(load64):
    case IR_OP(load16):
    case IR_OP(loadu32):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      if (CanElideForwardingTailFrame(g)) {
        return;
      }
      return LowerLoad(g, node);

    // stores.
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(g, node);

    case IR_OP(getbit):
      return LowerGetBitField(g, node);
      
    case IR_OP(setbit):
      return LowerSetBitField(g, node);

      case IR_OP(inc8):
      case IR_OP(inc16):
      case IR_OP(inc32):
      case IR_OP(inc64):
    case IR_OP(uinc8):
    case IR_OP(uinc16):
    case IR_OP(uinc32):
    case IR_OP(uinc64):
    case IR_OP(inca):
      case IR_OP(incf):
      case IR_OP(incd):
        return LowerInc(g, node);
      case IR_OP(dec8):
       case IR_OP(dec16):
       case IR_OP(dec32):
       case IR_OP(dec64):
    case IR_OP(udec8):
     case IR_OP(udec16):
     case IR_OP(udec32):
     case IR_OP(udec64):
      case IR_OP(deca):
       case IR_OP(decf):
       case IR_OP(decd):
      return LowerDec(g, node);
      
    case IR_OP(addi):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(adda):

    case IR_OP(subi):
    case IR_OP(subf):
    case IR_OP(subd):
    case IR_OP(suba):

    case IR_OP(muli):
    case IR_OP(mulf):
    case IR_OP(muld):

    case IR_OP(divi):
    case IR_OP(divf):
    case IR_OP(divd):

    case IR_OP(modi):

    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(lsli):

    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):

    case IR_OP(noti):
    case IR_OP(nota):
    case IR_OP(onescomp):
    case IR_OP(negi):
    case IR_OP(negf):
    case IR_OP(negd):

    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):

    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
    case IR_OP(tmp):
      return LowerExpression(g, node);

    case IR_OP(cmpeqi):
    case IR_OP(cmpnei):
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):

    case IR_OP(cmpeqf):
    case IR_OP(cmpnef):
    case IR_OP(cmpltf):
    case IR_OP(cmplef):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgef):

    case IR_OP(cmpeqd):
    case IR_OP(cmpned):
    case IR_OP(cmpltd):
    case IR_OP(cmpled):
    case IR_OP(cmpgtd):
    case IR_OP(cmpged):

    case IR_OP(cmpeqa):
    case IR_OP(cmpnea):
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):
      return LowerComparison(g, node);

    case IR_OP(cmp3wayi):
    case IR_OP(cmp3wayu):
    case IR_OP(cmp3waya):
    case IR_OP(cmp3wayf):
    case IR_OP(cmp3wayd):
      return LowerThreeWay(g, node);

    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(g, node);

    case IR_OP(bra):
      return LowerBranch(g, node);

    case IR_OP(cbra):
      return LowerComputedBranch(g, node);

    case IR_OP(label):
      return LowerLabel(g, node);

    case IR_OP(named_label):
      return LowerNamedLabel(g, node);

    case IR_OP(pusharg):
      return LowerPushArg(g, node);

    case IR_OP(calla):
      return LowerCall(g, node);

    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
      return LowerResult(g, node);

    case IR_OP(memzero):
      return LowerMemzero(g, node);

    case IR_OP(memcpy):
      return LowerMemcpy(g, node);

    case IR_OP(cast):
      LowerCast(g, node);
      return;
      
    case IR_OP(zeroextendi):
      return LowerZeroExtend(g, node);

    case IR_OP(signextendi):
      return LowerSignExtend(g, node);

    case IR_OP(aligni):
      return LowerAlign(g, node);

    case IR_OP(asm):
      return LowerAsm(g, node);

    case IR_OP(loc):
      return LowerLocation(g, node);

    case IR_OP(builtin_va_start):
      return LowerBuiltinVaStart(g, node);

    case IR_OP(builtin_va_arg):
      return LowerBuiltinVaArg(g, node);

    case IR_OP(builtin_va_end):
      return LowerBuiltinVaEnd(g, node);

    case IR_OP(builtin_va_copy):
      return LowerBuiltinVaCopy(g, node);

    case IR_OP(atomic_load):
    case IR_OP(atomic_store):
    case IR_OP(atomic_fetch_add):
    case IR_OP(atomic_fetch_sub):
    case IR_OP(atomic_add_fetch):
    case IR_OP(atomic_sub_fetch):
    case IR_OP(atomic_compare_exchange_bool):
    case IR_OP(atomic_compare_exchange_val):
    case IR_OP(atomic_compare_exchange_n):
    case IR_OP(atomic_fence):
      assert(false);
      return;
      
    case IR_OP(decsp):
    case IR_OP(savesp):
    case IR_OP(restoresp):
      return LowerStackPointerOps(g, node);
      
    case IR_OP(nrvoval):
      abort();      // TODO
  }
  // If we get here we've failed to handle the IR node.
  assert(false);
}

// Calculate the size of an argument based on its type.
static int64_t CalculateArgumentSize(Symbol* arg) {
  if (TypeIsFloatingPoint(arg->type)) {
    if (TypeIsDouble(arg->type)) {
      return 4;
    }
    return 4;
  }
  if (TypeIsPointerOrArray(arg->type)) {
    return 2;
  }
  if (TypeIsStructOrUnion(arg->type)) {
    return arg->type->info.struct_info->size;
  }
  int size = Sizeof(arg->type);
  return size < 2 ? 2 : size;
}

static RegisterVariableSet* TypeToRegisterVarSet(W65C02Generator* g, TypeRecord* type) {
  W65C02RegisterType reg_type;
  if (TypeIsFloat(type) || TypeIsDouble(type)) {
    reg_type = k6502RegTypeF;
  } else if (TypeIsChar(type) || TypeIsBool(type)) {
    reg_type = k6502RegTypeB;
  } else if (TypeIsLong(type) || TypeIsMemberPointerAggregate(type)) {
    reg_type = k6502RegTypeL;
  } else if (TypeIsLongLong(type)) {
    reg_type = k6502RegTypeX;
  } else {
    reg_type = k6502RegTypeI;
  }
  for (int i = 0; i < kNumVarSets; i++) {
    if (g->reg_vars[i].type == reg_type) {
      return &g->reg_vars[i];
    }
  }
  abort();
}

static RegisterVariableSet* MaybeUseRegister(W65C02Generator* g, PoolEntry* entry) {
  TypeRecord* type = entry->value.symbol->type;
  if (TypeIsArray(type) || !TypeIsScalar(type)) {
    return false;
  }
  if (entry->value.symbol->flags.address_taken) {
    return false;
  }
  RegisterVariableSet* set = TypeToRegisterVarSet(g, type);
  if (entry->value.symbol->usage_info.reads == 1) {
    // No point in putting a single read into a register.
    return NULL;
  }
  if (set->num_vars < set->max_vars) {
    return set;
  }
  return NULL;
}

static void LoadArgValueIntoReg(W65C02Generator* g, PoolEntry* entry, TargetInstruction* var) {
  Symbol* arg = entry->value.symbol;
  int offset = arg->stack_offset;
  TargetInstruction* result = entry->pooled->data.ptr;
  int size = Sizeof(entry->value.symbol->type);
  bool highzero = offset < 256;
  // Macro instruction, expands to:
  // lda #dest_addr
  // ldx #offset lo
  // ldy #offset hi (removed for single byte case)
  // JSR __var_value[b] (or arg_value[b])
  W65C02Opcode var_value_op = ArgValueFunction(size, highzero);
  Emit(g,
       NewInstruction2(var_value_op, result,
                       var,
                       kAddrModeImplied));
  AddSpillPoint(g, result);

}

static void AssignRegisterOrOffset(W65C02Generator* g, PoolEntry* entry,
                                   Vector* args, int* var_offset) {
  TargetInstruction* inst;
  RegisterVariableSet* varset;
  if (entry->pooled->opcode == IR_OP(localvar) ||
      entry->pooled->opcode == IR_OP(tempvar)) {
    varset = MaybeUseRegister(g, entry);
    if (varset == NULL) {
      // Make space for variable on the stack.
      int32_t size = entry->value.symbol->type->size;
       *var_offset += size;
    }
    IRVariable* var = (IRVariable*)entry->pooled;
    inst = NewTargetSymbol(var->symbol);
    inst->operand[0] =
        GetIntConstant(g, entry->pooled, kTargetTypeAddress, *var_offset);
    inst->opcode = (TargetOpcode)W65C02_OP(localvar);
    SetAddrMode(inst, kAddrModeImplied);
    if (TypeIsStructOrUnion(entry->value.symbol->type)) {
      inst->flags |= k6502NeedAddress;
    }
    inst = Emit(g, inst);
    if (varset != NULL) {
      entry->pooled->data.ptr = CreateVariableRegister(g, varset, inst, entry->value.symbol);
     } else {
      entry->pooled->data.ptr = inst;
    }
  } else if (entry->pooled->opcode == IR_OP(argument)) {
    IRVariable* var = (IRVariable*)entry->pooled;
    inst = NewTargetSymbol(var->symbol);
    inst->operand[0] = GetIntConstant(g, entry->pooled, kTargetTypeAddress,
                                      var->symbol->stack_offset);
    inst->opcode = (TargetOpcode)W65C02_OP(argument);
    SetAddrMode(inst, kAddrModeImplied);
    if (TypeIsStructOrUnion(entry->value.symbol->type)) {
      inst->flags |= k6502NeedAddress;
    }
    inst = Emit(g, inst);
    if (CanElideForwardingTailFrame(g)) {
      entry->pooled->data.ptr = inst;
      return;
    }
    if ((varset = MaybeUseRegister(g, entry)) != NULL) {
      entry->pooled->data.ptr = CreateVariableRegister(g, varset, inst, entry->value.symbol);
      // Load the register from the stacked argument.
      LoadArgValueIntoReg(g, entry, inst);
    } else {
      entry->pooled->data.ptr = inst;
    }
  }
}

static int CompareRegisterVar(const void* a, const void* b) {
  const PoolEntry* var1 = *(const PoolEntry**)a;
  const PoolEntry* var2 = *(const PoolEntry**)b;

  Symbol* sym1 = var1->value.symbol;
  Symbol* sym2 = var2->value.symbol;
  int weight1 = sym1->usage_info.reads * (sym1->usage_info.used_in_loop + 1);
  int weight2 = sym2->usage_info.reads * (sym2->usage_info.used_in_loop + 1);

  return weight2 - weight1;
}

static void AssignRegisterVars(W65C02Generator* g, Vector* vars, Vector* args) {
  // Variables are allocated below the frame, arguments are above or in
  // registers.
  // If the argument is in a register, the top bit of the data.ivalue is
  // set and the low order bits are the register number.

  // Sort the pooled local variables in reverse order of usage.  Those
  // with the largest number of references will be at the start of the
  // vector.
  qsort(vars->value.p, vars->length, sizeof(IRNode*), CompareRegisterVar);

  int32_t var_offset = 3;       // 3 bytes for reg save mask.

  for (size_t i = 0; i < vars->length; i++) {
    PoolEntry* entry = vars->value.p[i];
    AssignRegisterOrOffset(g, entry, args, &var_offset);
  }
  
  // We now know the stack frame size.
  g->base.stack_frame_size = (int32_t)var_offset;
}

static void LowerVariables(W65C02Generator* g) {
  Vector local_vars = {0};
  
  // Collect all local variables so that we can assign some of them
  // to registers.
  for (size_t i = 0; i < g->gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)g->gen->variable_pool.value.p[i];
    switch (entry->pooled->opcode) {
      case IR_OP(localvar):
        if (TypeIsFunction(entry->value.symbol->type)) {
          // Local function is treated as an extern.
          goto function;
        }
        // Fall through.
      case IR_OP(tempvar):
        VectorAppend(&local_vars, entry);
        break;
      case IR_OP(argument):
        VectorAppend(&local_vars, entry);
        break;
      function:
      default: {
        // Static variables are referenced by a symbol instruction.
        IRVariable* var = (IRVariable*)entry->pooled;
        TargetInstruction* inst = GetSymbol(g, NULL, var->symbol);
        if (TypeIsStructOrUnion(var->symbol->type) || TypeIsArray(var->symbol->type) || TypeIsFunction(var->symbol->type)) {
          SetAddrMode(inst, kAddrModeSymbolAddr);
        } else {
          SetAddrMode(inst, kAddrModeAbsoluteSymbol);
        }
        entry->pooled->data.ptr = inst;
        break;
      }
    }
  }

  AssignRegisterVars(g, &local_vars,
                     &compiler->current_function->info.function.prototype);
  VectorDestruct(&local_vars);
}

void W65C02Lower(W65C02Generator* g, Generator* gen) {
  // Variables are allocated below the frame, arguments are above.
  int32_t arg_offset = 0;
  if (TypeIsFunctionReturningStructOrUnion(compiler->current_function)) {
    arg_offset = 2;
  }

  // Calculate the offset on the stack for all the arguments.  Store this offset
  // in the stack_offset field of the Symbol.
  Vector* prototype = &compiler->current_function->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* arg = prototype->value.p[i];
    arg->stack_offset = arg_offset;
    int64_t size = CalculateArgumentSize(arg);
    arg_offset += size;
  }
  g->incoming_arg_size = (size_t)arg_offset;
  g->callee_pops_args =
      FunctionUsesCalleeArgCleanup(compiler->current_function);
  
  // Run through IR pre-optimizing it for CISC.
  bool changed;
  do {
    changed = false;
    IRNode* node = GeneratorFirstInstruction(gen);
    while (node != NULL) {
      node = PreLower(g, gen, node, &changed);
    }
  } while (changed);

#if PRINT_PRELOWER
  printf("Prelowered IR\n");
  GeneratorPrintIR(gen, stdout);
  printf("===============\n");
#endif
  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(g, node);
    node = IRNext(node);
  }

  W65C02CalculateInstructionAddresses(g);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    W65C02Print(g, compiler->ir_output_file);
  }

  // Build basic blocks for.
  TargetBuildBasicBlocks(&g->base);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
  }
  
  // Pool all the variables.
  W65C02PoolVariables(g);
  
  // Disbled when moved to callee save
  // W65C02SpillExpressions(g);
 
  // Process branches to check their ranges.
  // W65C02ProcessBranches(g);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
     TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
   }

  if (OptLevel1()) {
    W65C02Optimize(g);
  }
  
  // Allocate registers to the instructions.
  W65C02AllocateRegisters(&g->register_allocator);

  // Register allocation exposes adjacent zero-page registers that originated
  // as separate IR values. Fold complete four- and eight-byte indirect copies
  // only after those physical byte ranges are known.
  W65C02CombineIndirectCopies(g);

  // Process branches to check their ranges.
  W65C02ProcessBranches(g);

}

void W65C02PrintInstruction(TargetInstruction* inst, FILE* fp) {
  TargetPrintInstruction(inst, W65C02OpcodeName, fp);
}

void W65C02Print(W65C02Generator* g, FILE* fp) {
  TargetInstruction* inst = TargetFirstInstruction(&g->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, W65C02OpcodeName, fp);
    inst = TargetNext(inst);
  }
}

bool W65C02IsExpression(TargetInstruction* inst) {
  return ((W65C02Opcode)inst->opcode >= W65C02_OP(expr1) &&
         (W65C02Opcode)inst->opcode <= W65C02_OP(exprd)) ||
  ((W65C02Opcode)inst->opcode >= W65C02_OP(ivarreg) &&
         (W65C02Opcode)inst->opcode <= W65C02_OP(dvarreg)) ||
  TargetOpcodeEq(inst->opcode, W65C02_OP(structreturn));
}

bool W65C02IsSignedLoad(TargetInstruction* inst) {
  return false;
}
