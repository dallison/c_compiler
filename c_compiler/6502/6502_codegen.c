//
//  6502_codegen.c
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_codegen.h"
#include <assert.h>
#include <stdlib.h>
#include "6502_branches.h"
#include "6502_spiller.h"
#include "6502_optimize.h"
#include "compiler.h"
#include "target_basic_block.h"

static TargetInstruction* LowerIRNode(_6502Generator* g, IRNode* node);

const char* _6502OpcodeName(int op) {
  _6502Opcode opcode = op;
  switch (opcode) {
    default:
      // Use the generic TargetOpcodeName for all non-6502 specific
      // opcodes.
      return TargetOpcodeName((TargetOpcode)opcode);

    case _6502_OP(expr1):
      return "expr1";
    case _6502_OP(expr2):
      return "expr2";
    case _6502_OP(expr4):
      return "expr4";
    case _6502_OP(expr8):
      return "expr8";
    case _6502_OP(expr_addr_a):
      return "expr_addr_a";
    case _6502_OP(expr_addr_x):
      return "expr_addr_x";
    case _6502_OP(expr_addr_y):
      return "expr_addr_y";
    case _6502_OP(enter):
      return "enter";
    case _6502_OP(leave):
      return "leave";
    case _6502_OP(localvar):
      return "localvar";
    case _6502_OP(argument):
      return "argument";
    case _6502_OP(literalreflo):
      return "literalreflo";
    case _6502_OP(literalrefhi):
      return "literalrefhi";
      case _6502_OP(literalref):
        return "literalrefl";

    case _6502_OP(enter_leaf):
      return "enter_leaf";
    case _6502_OP(leave_leaf):
      return "leave_leaf";

    case _6502_OP(var_addr):
      return "var_addr";
    case _6502_OP(var_addrb):
      return "var_addrb";

    case _6502_OP(arg_addr):
      return "arg_addr";
    case _6502_OP(arg_addrb):
      return "arg_addrb";

      case _6502_OP(var_addr_xy):
         return "var_addr_xy";
      case _6502_OP(var_addrb_xy):
         return "var_addrb_xy";

      case _6502_OP(arg_addr_xy):
         return "arg_addr_xy";
      case _6502_OP(arg_addrb_xy):
         return "arg_addrb_xy";

    case _6502_OP(var_value1):
      return "var_value1";
    case _6502_OP(var_value1b):
      return "var_value1b";
    case _6502_OP(var_value2):
      return "var_value2";
    case _6502_OP(var_value2b):
      return "var_value2b";
    case _6502_OP(var_value4):
      return "var_value4";
    case _6502_OP(var_value4b):
      return "var_value4b";
    case _6502_OP(var_value8):
      return "var_value8";
    case _6502_OP(var_value8b):
      return "var_value8b";
    case _6502_OP(arg_value1):
      return "arg_value1";
    case _6502_OP(arg_value1b):
      return "arg_value1b";
    case _6502_OP(arg_value2):
      return "arg_value2";
    case _6502_OP(arg_value2b):
      return "arg_value2b";
    case _6502_OP(arg_value4):
      return "arg_value4";
    case _6502_OP(arg_value4b):
      return "arg_value4b";
    case _6502_OP(arg_value8):
      return "arg_value8";
    case _6502_OP(arg_value8b):
      return "arg_value8b";
    case _6502_OP(fake_bra):
      return "fake_bra";

      case _6502_OP(spill1):
         return "spill1";
      case _6502_OP(spill2):
          return "spill2";
      case _6502_OP(spill4):
          return "spill4";
      case _6502_OP(spill8):
          return "spill8";
      case _6502_OP(unspill1):
           return "unspill1";
      case _6502_OP(unspill2):
           return "unspill2";
      case _6502_OP(unspill4):
           return "unspill4";
      case _6502_OP(unspill8):
           return "unspill8";

    case _6502_OP(brk):
      return "brk";

    case _6502_OP(bpl):
      return "bpl";
    case _6502_OP(bmi):
      return "bmi";
    case _6502_OP(bvc):
      return "bvc";
    case _6502_OP(bvs):
      return "bvs";
    case _6502_OP(bcc):
      return "bcc";
    case _6502_OP(bcs):
      return "bcs";
    case _6502_OP(bne):
      return "bne";
    case _6502_OP(beq):
      return "beq";

    case _6502_OP(jsr):
      return "jsr";
    case _6502_OP(jmp):
      return "jmp";

    case _6502_OP(rti):
      return "rti";
    case _6502_OP(rts):
      return "rts";

    case _6502_OP(lda):
      return "lda";
    case _6502_OP(ldx):
      return "ldx";
    case _6502_OP(ldy):
      return "ldy";
    case _6502_OP(sta):
      return "sta";
    case _6502_OP(stx):
      return "stx";
    case _6502_OP(sty):
      return "sty";

    case _6502_OP(cmp):
      return "cmp";
    case _6502_OP(cpy):
      return "cpy";
    case _6502_OP(cpx):
      return "cpx";
    case _6502_OP(bit):
      return "bit";

    case _6502_OP(ora):
      return "ora";
    case _6502_OP(and):
      return "and";
    case _6502_OP(eor):
      return "eor";

    case _6502_OP(adc):
      return "adc";
    case _6502_OP(sbc):
      return "sbc";

    case _6502_OP(asl):
      return "asl";
    case _6502_OP(rol):
      return "rol";
    case _6502_OP(lsr):
      return "lsr";
    case _6502_OP(ror):
      return "ror";

    case _6502_OP(dec):
      return "dec";
    case _6502_OP(inc):
      return "inc";
    case _6502_OP(dey):
      return "dey";
    case _6502_OP(dex):
      return "dex";
    case _6502_OP(iny):
      return "iny";
    case _6502_OP(inx):
      return "inx";

    case _6502_OP(php):
      return "php";
    case _6502_OP(clc):
      return "clc";
    case _6502_OP(plp):
      return "plp";
    case _6502_OP(sec):
      return "sec";
    case _6502_OP(pha):
      return "pha";
    case _6502_OP(cli):
      return "cli";
    case _6502_OP(pla):
      return "pla";
    case _6502_OP(sei):
      return "sei";
    case _6502_OP(tay):
      return "tay";
    case _6502_OP(clv):
      return "clv";
    case _6502_OP(cld):
      return "cld";
    case _6502_OP(sed):
      return "sed";

    case _6502_OP(tya):
      return "tya";
    case _6502_OP(txa):
      return "txa";
    case _6502_OP(txs):
      return "txs";
    case _6502_OP(tax):
      return "tax";
    case _6502_OP(tsx):
      return "tsx";

    case _6502_OP(nop):
      return "nop";

    // 65C02
    case _6502_OP(tsb):
      return "tsb";
    case _6502_OP(trb):
      return "trb";
    case _6502_OP(stz):
      return "stz";
    case _6502_OP(phy):
      return "phy";
    case _6502_OP(ply):
      return "ply";
    case _6502_OP(phx):
      return "phx";
    case _6502_OP(plx):
      return "plz";
    case _6502_OP(bra):
      return "bra";
    case _6502_OP(ap):
      return "ap";
    case _6502_OP(ssavar):
      return "ssavar";
    case _6502_OP(phi):
      return "phi";
    case _6502_OP(jumptable):
      return "jumptable";
  }
}

static TargetInstruction* ArgumentPointer(_6502Generator* g) {
  if (g->argument_pointer == NULL) {
    g->argument_pointer =
        TargetEmit(&g->base, TargetNewInstruction((TargetOpcode)_6502_OP(ap)));
  }
  return g->argument_pointer;
}

bool _6502IsBranch(TargetInstruction* inst) {
  return (inst->flags & k6502BlockEnd) != 0;
}

bool _6502IsCall(TargetInstruction* inst) {
  return (inst->flags & k6502InstIsCall) != 0;
}

bool _6502IsReturn(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)_6502_OP(rts);
}

bool _6502IsSpill(TargetInstruction* inst) { return false; }

bool _6502IsLabel(TargetInstruction* inst) {
  return (inst->flags & k6502BlockStart) != 0;
}

bool _6502IsFloatingPoint(TargetInstruction* inst) { return false; }
bool _6502IsConditionalBranch(TargetInstruction* inst) {
  return (inst->flags & k6502InstIsCondBranch) != 0;
}

bool _6502IsFixedRegister(TargetInstruction* inst) {
  return false;
}

bool _6502IsConst(TargetInstruction* inst) {
  switch ((_6502Opcode)inst->opcode) {
    case _6502_OP(constb):
    case _6502_OP(consth):
    case _6502_OP(constw):
    case _6502_OP(constx):
    case _6502_OP(constf):
    case _6502_OP(constd):

      return true;
    default:
      return false;
  }
}

bool _6502IsSymbol(TargetInstruction* inst) {
  switch ((_6502Opcode)inst->opcode) {
    case _6502_OP(symbol):
      return true;
    default:
      return false;
  }
}

bool _6502IsJumpTableEntry(TargetInstruction* inst) {
  return inst->opcode == (TargetOpcode)_6502_OP(jumptable);
}

static TargetInstruction* _6502GetBranchTarget(TargetInstruction* inst) {
  return inst->operand[0];
}

static TargetVirtuals virtuals = {
    .opcode_name = _6502OpcodeName,
    .is_branch = _6502IsBranch,
    .is_call = _6502IsCall,
    .is_return = _6502IsReturn,
    .is_spill = _6502IsSpill,
    .is_label = _6502IsLabel,
    .is_floating_point = _6502IsFloatingPoint,
    .is_conditional_branch = _6502IsConditionalBranch,
    .is_fixed_register = _6502IsFixedRegister,
    .is_const = _6502IsConst,
    .is_symbol = _6502IsSymbol,
    .is_expression = _6502IsExpression,
    .is_table_entry = _6502IsJumpTableEntry,
    .get_branch_target = _6502GetBranchTarget,
};

static bool IsLeaf(_6502Generator* g) {
  bool is_leaf = g->base.num_calls == 0 /*&& OptLevel1()*/;
  return is_leaf;
}

static Symbol* CreateRuntimeSymbol(_6502Generator* g, const char* name) {
  TypeRecord* base = NewTypeRecord(kTypeVoid, kQualPlain);
  TypeRecord* func = NewFunctionTypeRecord();
  func->info.function.unknown_args = true;
  TypeRecordChain(func, base);
  return NewSymbol(name, func, STO(extern));
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

void _6502GeneratorInit(_6502Generator* g, Generator* gen) {
  TargetGeneratorInit(&g->base, gen, &virtuals);
  g->gen = gen;

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
  RUNTIME_SYM(push4);
  RUNTIME_SYM(push8);
  RUNTIME_SYM(push4xy);
  RUNTIME_SYM(push8xy);
  RUNTIME_SYM(pulla);
  RUNTIME_SYM(pullxy);
  RUNTIME_SYM(pull4);
  RUNTIME_SYM(pull8);
  RUNTIME_SYM(incsp1);
  RUNTIME_SYM(incsp2);
  RUNTIME_SYM(pushmem1);
  RUNTIME_SYM(pushmem2);
  RUNTIME_SYM(copymem1);
  RUNTIME_SYM(copymem2);
  RUNTIME_SYM(zeromem1);
  RUNTIME_SYM(zeromem2);
  RUNTIME_SYM(result2);
  RUNTIME_SYM(result4);
  RUNTIME_SYM(result8);
  RUNTIME_SYM(load_result);

  RUNTIME_SYM(umul1);
  RUNTIME_SYM(umul2);
  RUNTIME_SYM(umul4);
  RUNTIME_SYM(umul8);
  RUNTIME_SYM(smul1);
  RUNTIME_SYM(smul2);
  RUNTIME_SYM(smul4);
  RUNTIME_SYM(smul8);
  RUNTIME_SYM(fmul);
  RUNTIME_SYM(dmul);

  RUNTIME_SYM(sdiv1);
  RUNTIME_SYM(sdiv2);
  RUNTIME_SYM(sdiv4);
  RUNTIME_SYM(sdiv8);

  RUNTIME_SYM(udiv1);
  RUNTIME_SYM(udiv2);
  RUNTIME_SYM(udiv4);
  RUNTIME_SYM(udiv8);
  RUNTIME_SYM(fdiv);
  RUNTIME_SYM(ddiv);

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
  RUNTIME_SYM(i1tod);
  RUNTIME_SYM(i2tod);
  RUNTIME_SYM(i4tod);
  RUNTIME_SYM(i8tod);
  RUNTIME_SYM(ui1tof);
  RUNTIME_SYM(ui2tof);
  RUNTIME_SYM(ui4tof);
  RUNTIME_SYM(ui8tof);
  RUNTIME_SYM(ui1tod);
  RUNTIME_SYM(ui2tod);
  RUNTIME_SYM(ui4tod);
  RUNTIME_SYM(ui8tod);

  RUNTIME_SYM(ftod);
  RUNTIME_SYM(dtof);

  RUNTIME_SYM(ftoi1);
  RUNTIME_SYM(ftoi2);
  RUNTIME_SYM(ftoi4);
  RUNTIME_SYM(ftoi8);
  RUNTIME_SYM(dtoi1);
  RUNTIME_SYM(dtoi2);
  RUNTIME_SYM(dtoi4);
  RUNTIME_SYM(dtoi8);
  RUNTIME_SYM(ftoui1);
  RUNTIME_SYM(ftoui2);
  RUNTIME_SYM(ftoui4);
  RUNTIME_SYM(ftoui8);
  RUNTIME_SYM(dtoui1);
  RUNTIME_SYM(dtoui2);
  RUNTIME_SYM(dtoui4);
  RUNTIME_SYM(dtoui8);

  RUNTIME_SYM(cmpeqf);
  RUNTIME_SYM(cmpnef);
  RUNTIME_SYM(cmpltf);
  RUNTIME_SYM(cmpgef);

  RUNTIME_SYM(cmpeqd);
  RUNTIME_SYM(cmpned);
  RUNTIME_SYM(cmpltd);
  RUNTIME_SYM(cmpged);
  
  RUNTIME_SYM(jump_table);

  VectorInit(&g->branches);
  _6502RegisterAllocatorInit(&g->register_allocator, g);
}
#undef RUNTIME_SYM

_6502Generator* New6502Generator(Generator* gen) {
  _6502Generator* g = malloc(sizeof(_6502Generator));
  _6502GeneratorInit(g, gen);
  return g;
}

void _6502GeneratorDestruct(_6502Generator* g) {
  TargetGeneratorDestruct(&g->base);
  VectorDestruct(&g->branches);
  _6502RegisterAllocatorDestruct(&g->register_allocator);
}

void _6502GeneratorDelete(_6502Generator* g) {
  _6502GeneratorDestruct(g);
  free(g);
}

static AddressingMode GetAddrMode(TargetInstruction* inst) {
  AddressingMode mode = (AddressingMode)((inst->flags >> 16) & 0x1f);
  assert(mode > kAddrModeUnknown && mode < kAddrModeInvalid);
  return mode;
}

static void SetAddrMode(TargetInstruction* inst, AddressingMode mode) {
  inst->flags |= (int)mode << 16;
}

static bool IsIndirectMode(TargetInstruction* inst) {
  AddressingMode mode = GetAddrMode(inst);
  return mode == kAddrModeIndirectIndexed || mode == kAddrModeIndirect;
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(_6502Opcode opcode,
                                          TargetInstruction* op1,
                                          int addressing_mode) {
  TargetInstruction* inst = TargetNewInstruction1((TargetOpcode)opcode, op1);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static TargetInstruction* NewInstruction2(_6502Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          int addressing_mode) {
  TargetInstruction* inst =
      TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static TargetInstruction* NewInstruction3(_6502Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3,
                                          int addressing_mode) {
  TargetInstruction* inst =
      TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static TargetInstruction* Emit(_6502Generator* g, TargetInstruction* inst) {
#if 0
  _6502PrintInstruction(inst, stdout);
#endif
  return TargetEmit(&g->base, inst);
}

static TargetInstruction* EmitBefore(_6502Generator* g, TargetInstruction* inst,
                                     TargetInstruction* pos) {
  return TargetEmitBefore(&g->base, inst, pos);
}

static TargetInstruction* EmitAfter(_6502Generator* g, TargetInstruction* inst,
                                    TargetInstruction* pos) {
  return TargetEmitAfter(&g->base, inst, pos);
}

static TargetInstruction* EmitConstant(_6502Generator* g,
                                       TargetInstruction* c) {
  return TargetEmitConstant(&g->base, c);
}

static TargetInstruction* EmitSymbol(_6502Generator* g, TargetInstruction* c) {
  return TargetEmitSymbol(&g->base, c);
}

static TargetInstruction* FramePointer(_6502Generator* g, AddressingMode mode) {
  TargetInstruction* inst = TargetFramePointer(&g->base);
  SetAddrMode(inst, mode);
  return inst;
}

static TargetInstruction* StackPointer(_6502Generator* g) {
  return TargetStackPointer(&g->base);
}

static TargetInstruction* ThreadPointer(_6502Generator* g) {
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

static TargetInstruction* GetIntConstant(_6502Generator* g, IRNode* node,
                                         TargetType type, int64_t value) {
  TargetInstruction* inst = TargetGetIntConstant(&g->base, node, type, value);
  SetAddrMode(inst, kAddrModeImmediate);
  return inst;
}

static TargetInstruction* ByteConst(_6502Generator* g, int value) {
  return GetIntConstant(g, NULL, kTargetTypeByte, value);
}

static TargetInstruction* Zero(_6502Generator* g) {
  return GetIntConstant(g, NULL, kTargetTypeByte, 0);
}

static TargetInstruction* One(_6502Generator* g) {
  return GetIntConstant(g, NULL, kTargetTypeByte, 1);
}

static TargetInstruction* GetFloatingPointConstant(_6502Generator* g,
                                                   IRNode* node,
                                                   TargetType type,
                                                   double value) {
  return TargetGetFloatingPointConstant(&g->base, node, type, value);
}

static TargetInstruction* GetSymbol(_6502Generator* g, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&g->base, node, symbol);
}

static TargetInstruction* NewInstruction(_6502Opcode opcode,
                                         int addressing_mode) {
  TargetInstruction* inst = TargetNewInstruction((TargetOpcode)opcode);
  SetAddrMode(inst, addressing_mode);
  return inst;
}

static TargetInstruction* EmitBranch(_6502Generator* g, _6502Opcode op,
                                     IRNode* target_node) {
  TargetInstruction* bra = Emit(g, NewInstruction(op, kAddrModeRelative));
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(bra, target_node, 0));
  } else {
    bra->operand[0] = target_node->data.ptr;
  }
  VectorAppend(&g->branches, bra);
  return bra;
}

static TargetInstruction* EmitLabelReference(_6502Generator* g,
                                             TargetInstruction* label,
                                             IRNode* target_node) {
  Emit(g, label);
  if (target_node->data.ptr == NULL) {
    VectorAppend(&g->base.fixups, NewBranchFixup(label, target_node, 0));
  } else {
    label->operand[0] = target_node->data.ptr;
  }
  return label;
}

static void EmitResolvedBranch(_6502Generator* g, _6502Opcode op,
                               TargetInstruction* target) {
  VectorAppend(&g->branches,
               Emit(g, NewInstruction1(op, target, kAddrModeRelative)));
}

// The 6502 isn't RISC so instructions can load and store directly
// to memory.  Returns next node or NULL.
static IRNode* Preoptimize(_6502Generator* g, Generator* gen, IRNode* node, bool* changed) {
  if (IRIsLoad(node)) {
    // A load can be removed if it has only one use and that use isn't
    // a store or a load.  It's input can't also be a load.
    if (!IRIsLoad(node->inputs.value.p[0]) && node->outputs.length == 1) {
      IRNode* use = node->outputs.value.p[0];
      if (!IRIsStoreOnly(use) && !IRIsLoad(use)) {
        IRNode* input = node->inputs.value.p[0];
        GeneratorReplaceInstruction(gen, node, input);
        BasicBlockRemoveInstruction(gen, node->block, node);
        *changed = true;
      }
    }
  } else if (IRIsStoreOnly(node)) {
    // A store be eliminated if its input has one use (the store instruction)
    IRNode* dest = node->inputs.value.p[0];
    IRNode* src = node->inputs.value.p[1];
    if (!IRIsConst(src) && !IRIsVariable(src)) {
      if (src->outputs.length == 1 && src->dest == NULL) {
        src->dest = dest;
        src->flags |= kIRDestIsIndirect;      // Dest is indirect address.
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
        int size = node->type->size;
        // Max index for Y is 255, but we need to allow for 'size' bytes
        // above the start index.
        if (value < (256 - size)) {
          IRAddInput(node, op2, false);     // Add 3rd operand to store.
          IRReplaceInput(node, 0, op1);
          if (dest->outputs.length == 0) {
            GeneratorRemoveInstruction(gen, dest);
            *changed = true;
          }
        }
      }
    }
  } else if (node->opcode == IR_OP(rmovi) || node->opcode == IR_OP(rmova) ||
             node->opcode == IR_OP(rmovf) ||  node->opcode == IR_OP(rmovd)) {
    // We can eliminate rmov by assigning the dest of the src.
    IRNode* dest = node->inputs.value.p[0];
    IRNode* src = node->inputs.value.p[1];
    if (src->dest == NULL) {
      src->dest = dest;
      IRNode* next = IRNext(node);
      BasicBlockRemoveInstruction(gen, node->block, node);
      *changed = true;
      return next;
    }
  }
  return IRNext(node);
}

static void Operate(_6502Generator* g, _6502Opcode op, TargetInstruction* src,
                    int index) {
  AddressingMode mode = GetAddrMode(src);
  if (mode == kAddrModeIndirectIndexed && index == 0) {
    // An index of 0 can use a kAddrModeIndirect: lda (xxx)
    mode = kAddrModeIndirect;
  }
  Emit(g, NewInstruction2(
              op, src, ByteConst(g, index), mode));
}

// Operation instructions
#define INST(op)                                                         \
  static void op(_6502Generator* g, TargetInstruction* src, int index) { \
    Operate(g, _6502_OP(op), src, index);                                \
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
  static void op##i(_6502Generator* g, int value) {                          \
    Emit(g, NewInstruction1(_6502_OP(op),                                    \
                            ByteConst(g, value), \
                            kAddrModeImmediate));                            \
  }

INST(lda)
INST(ldx)
INST(ldy)
INST(ora)
INST(eor)
INST(adc)
INST(sbc)
INST(cmp)

#undef INST

// Zero page Immediate instructions:
// ldazi(g, value);
// These load the immediate value of a zero page location.
#define INST(op)                                                              \
  static void op##zi(_6502Generator* g, TargetInstruction* reg, int offset) { \
    Emit(g, NewInstruction2(_6502_OP(op), reg,                                \
                            ByteConst(g, offset), \
                            kAddrModeZeroPageImmediate));                     \
  }

INST(lda)
INST(ldx)
INST(ldy)

#undef INST
// Single instructions.
#define INST(op)                                             \
  static void op(_6502Generator* g) {                        \
    Emit(g, NewInstruction(_6502_OP(op), kAddrModeImplied)); \
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

static void ldx(_6502Generator* g, TargetInstruction* src, int index) {
  AddressingMode mode = GetAddrMode(src);
  Emit(g,
       NewInstruction2(_6502_OP(ldx), src,
                       ByteConst(g, index), mode));
}

static void ldy(_6502Generator* g, TargetInstruction* src, int index) {
  AddressingMode mode = GetAddrMode(src);
  Emit(g,
       NewInstruction2(_6502_OP(ldy), src,
                       ByteConst(g, index), mode));
}

static void sta(_6502Generator* g, TargetInstruction* dest, int index) {
  AddressingMode mode = GetAddrMode(dest);
  if (mode == kAddrModeIndirectIndexed && index == 0) {
    mode = kAddrModeIndirect;
  }
  Emit(g,
       NewInstruction2(_6502_OP(sta), dest,
                       ByteConst(g, index), mode));
}

static void stx(_6502Generator* g, TargetInstruction* dest, int index) {
  Emit(g, NewInstruction2(_6502_OP(stx), dest,
                          ByteConst(g, index),
                          GetAddrMode(dest)));
}

static void sty(_6502Generator* g, TargetInstruction* dest, int index) {
  Emit(g, NewInstruction2(_6502_OP(sty), dest,
                          ByteConst(g, index),
                          GetAddrMode(dest)));
}

static void stz(_6502Generator* g, TargetInstruction* dest, int index) {
  Emit(g, NewInstruction2(_6502_OP(stz), dest,
                          ByteConst(g, index),
                          GetAddrMode(dest)));
}

static TargetInstruction* jsr(_6502Generator* g, Symbol* func) {
  return Emit(g, NewInstruction1(_6502_OP(jsr), GetSymbol(g, NULL, func),
                                 kAddrModeAbsolute));
}

// If either the src or dest needs an index in Y reg, load it.  If
static void SetIndexReg(_6502Generator* g, TargetInstruction* src,
                        TargetInstruction* dest, int index) {
  if (GetAddrMode(src) == kAddrModeIndirectIndexed ||
      GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    if (index == 1) {
      ldyi(g, 1);
    } else if (index > 1) {
      iny(g);
    }
  }
}

static void SetIndexReg2(_6502Generator* g, TargetInstruction* src1,
                         TargetInstruction* src2, TargetInstruction* dest,
                         int index) {
  if (GetAddrMode(src1) == kAddrModeIndirectIndexed ||
      GetAddrMode(src2) == kAddrModeIndirectIndexed ||
      GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    if (index == 1) {
      ldyi(g, 1);
    } else if (index > 1) {
      iny(g);
    }
  }
}

static void SetIndexRegDown(_6502Generator* g, TargetInstruction* src,
                            TargetInstruction* dest, int index, int limit) {
  if (GetAddrMode(src) == kAddrModeIndirectIndexed ||
      GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    if (index == limit - 1) {
      ldyi(g, limit - 1);
    } else if (index > 0) {
      dey(g);
    }
  }
}

// TODO: allow override of tls model per variable.
static TargetInstruction* GetTlsVariableAddress(_6502Generator* g,
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

static void GetTlsAddressAndOffset(_6502Generator* g, IRNode* addr_node,
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

static void ApplyFixups(_6502Generator* g, IRNode* label_node) {
  TargetApplyFixups(&g->base, label_node);
}

// Do some strength reduction if we can.  Returns NULL or new instruciton
static TargetInstruction* ReduceExpressionStrength(_6502Generator* g,
                                                   IRNode* node,
                                                   _6502Opcode opcode) {
  return NULL;
}

// Temp expression in zero page holding value.
static TargetInstruction* TempRegister(_6502Generator* g, int size) {
  switch (size) {
    case 1:
      return Emit(g, NewInstruction(_6502_OP(expr1), kAddrModeZeroPage));
    default:        // Any other size is handled as a pointer.
    case 2:
      return Emit(g, NewInstruction(_6502_OP(expr2), kAddrModeZeroPage));
    case 4:
      return Emit(g, NewInstruction(_6502_OP(expr4), kAddrModeZeroPage));
    case 8:
      return Emit(g, NewInstruction(_6502_OP(expr8), kAddrModeZeroPage));
  }
}

// Temp expression in zero page containing address of something.
static TargetInstruction* TempExpressionAddress(_6502Generator* g, AddressingMode mode) {
  return Emit(g, NewInstruction(_6502_OP(expr2), mode));
}

static _6502Opcode VarValueFunction(int size, bool highzero) {
  switch (size) {
    case 1:
      return highzero ? _6502_OP(var_value1) : _6502_OP(var_value1b);
    case 2:
      return highzero ? _6502_OP(var_value2) : _6502_OP(var_value2b);
    case 4:
      return highzero ? _6502_OP(var_value4) : _6502_OP(var_value4b);
    case 8:
      return highzero ? _6502_OP(var_value8) : _6502_OP(var_value8b);
    default:
      abort();
  }
}

static _6502Opcode ArgValueFunction(int size, bool highzero) {
  switch (size) {
    case 1:
      return highzero ? _6502_OP(arg_value1) : _6502_OP(arg_value1b);
    case 2:
      return highzero ? _6502_OP(arg_value2)
                      : _6502_OP(arg_value2b;) case 4 : return highzero
                                                        ? _6502_OP(arg_value4)
                                                        : _6502_OP(arg_value4b);
    case 8:
      return highzero ? _6502_OP(arg_value8) : _6502_OP(arg_value8b);
    default:
      abort();
  }
}

static TargetInstruction* Materialize(_6502Generator* g, IRNode* node) {
  TargetInstruction* inst = GetLoweredNode(node);
  TargetInstruction* result = NULL;
  int offset;
  int size = node->type->size;
  if (IRIsConst(node)) {
    // Load a constant.
    result = TempRegister(g, node->type->size);
    uint64_t value = TargetIntValue(inst);
    for (int i = 0; i < size; i++) {
      SetIndexReg(g, result, result, i);
      ldai(g, (int)(value >> (i * 8)) & 0xff);
      sta(g, result, i);
    }
  } else {
    switch ((_6502Opcode)inst->opcode) {
      case _6502_OP(argument):
      case _6502_OP(localvar): {
        // Variable or argument.  Load value using one of the runtime helper
        // functions.
        offset = (int)TargetIntValue(inst->operand[0]);
        result = TempRegister(g, node->type->size);
        bool highzero = offset < 256;
        // Macro instruction, expands to:
        // lda #dest_addr
        // ldx #offset lo
        // ldy #offset hi (removed for single byte case)
        // JSR __var_value[b] (or arg_value[b])
        _6502Opcode var_value_op = inst->opcode == _6502_OP(argument)
                                       ? ArgValueFunction(size, highzero)
                                       : VarValueFunction(size, highzero);
        Emit(g,
             NewInstruction2(var_value_op, result,
                             inst,
                             kAddrModeImplied));
        break;
      }

      case _6502_OP(ssavar):
      case _6502_OP(phi): {
        TargetInstruction* var = inst->operand[0];
        offset = (int)TargetIntValue(var->operand[0]);
        result = TempRegister(g, node->type->size);
        bool highzero = offset < 256;
        // Macro instruction, expands to:
        // lda #dest_addr
        // ldx #offset lo
        // ldy #offset hi (removed for single byte case)
        // JSR __var_value[b] (or arg_value[b])
        _6502Opcode var_value_op = var->opcode == _6502_OP(argument)
                                         ? ArgValueFunction(size, highzero)
                                         : VarValueFunction(size, highzero);
        Emit(g,
               NewInstruction2(var_value_op, result,
                               var,
                               kAddrModeImplied));
        break;
        }
      

      case _6502_OP(symbol): {
        // lda symbol+0
        // sta result+0
        // ldy #1
        // lda symbol+1, Y
        // sta result+1
        result = TempRegister(g, node->type->size);
        for (int i = 0; i < node->type->size; i++) {
          SetIndexReg(g, inst, result, i);
          lda(g, inst, i);
          sta(g, result, i);
        }
        SetLoweredNode(node, result);
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

static TargetInstruction* GetAddressFromTarget(_6502Generator* g, TargetInstruction* addr, AddressingMode mode) {
  TargetInstruction* result = NULL;
  int offset;
  switch ((_6502Opcode)addr->opcode) {
    case _6502_OP(argument):
    case _6502_OP(localvar): {
      offset = (int)TargetIntValue(addr->operand[0]);
      result = TempExpressionAddress(g, mode);
      // Macro instruction expands to:
      // lda #dest_addr
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr[b] (or arg_addr[b])
      _6502Opcode var_addr_op = addr->opcode == _6502_OP(argument)
                                    ? _6502_OP(arg_addr)
                                    : _6502_OP(var_addr);
      if (offset >= 256) {
        var_addr_op = addr->opcode == _6502_OP(argument) ? _6502_OP(arg_addrb)
                                                         : _6502_OP(var_addrb);
      }
      Emit(g, NewInstruction2(var_addr_op, result,
                              addr,
                              kAddrModeImplied));
      break;
    }

    case _6502_OP(ssavar):
    case _6502_OP(phi): {
      TargetInstruction* var = addr->operand[0];
      return GetAddressFromTarget(g, var, mode);
    }

    case _6502_OP(symbol): {
      // lda #%abs(symbol)+0
      // sta result
      // lda #%abs(symbol)+1
      // sta result+1
      TargetSymbol* symbol = (TargetSymbol*)addr;
      printf("symbol: %p", symbol);
      result = TempRegister(g, 2);
      for (int i = 0; i < 2; i++) {
        SetIndexReg(g, addr, result, i);
        Emit(g, NewInstruction2(_6502_OP(lda), addr,
                                ByteConst(g, i),
                                kAddrModeAbsoluteSymbol));
        sta(g, result, i);
      }
      break;
    }
    default:
      result = addr;
      break;
  }
  return result;
}

// Gets the address of a variable into zero page.
static TargetInstruction* GetAddress(_6502Generator* g, IRNode* addr_node) {
  AddressingMode mode;
  if (TypeIsArray(addr_node->type) || TypeIsStructOrUnion(addr_node->type)) {
    mode = kAddrModeZeroPage;
  } else {
    mode = kAddrModeIndirectIndexed;
  }
  TargetInstruction* addr = GetLoweredNode(addr_node);
  return GetAddressFromTarget(g, addr, mode);
}

static void GetAddressXY(_6502Generator* g, IRNode* addr_node) {
  TargetInstruction* addr = GetLoweredNode(addr_node);
  int offset;
  switch ((_6502Opcode)addr->opcode) {
    case _6502_OP(argument):
    case _6502_OP(localvar): {
      offset = (int)TargetIntValue(addr->operand[0]);
      // Macro instruction expands to:
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr_xy (or arg_addr_xy)
      _6502Opcode var_addr_op = addr->opcode == _6502_OP(argument)
                                    ? _6502_OP(arg_addr_xy)
                                    : _6502_OP(var_addr_xy);
  
      if (offset >= 256) {
        var_addr_op = addr->opcode == _6502_OP(argument) ? _6502_OP(arg_addrb_xy)
                                                         : _6502_OP(var_addrb_xy);
      }
      Emit(g, NewInstruction1(var_addr_op,
                              addr,
                              kAddrModeImplied));
      break;
    }

    case _6502_OP(ssavar):
    case _6502_OP(phi): {
      TargetInstruction* var = addr->operand[0];
      offset = (int)TargetIntValue(var->operand[0]);
      // Macro instruction expands to:
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr_xy (or arg_addr_xy)
      _6502Opcode var_addr_op = var->opcode == _6502_OP(argument)
                                    ? _6502_OP(arg_addr_xy)
                                    : _6502_OP(var_addr_xy);
      if (offset >= 256) {
        var_addr_op = addr->opcode == _6502_OP(argument) ? _6502_OP(arg_addrb_xy)
                                                         : _6502_OP(var_addrb_xy);
      }
      Emit(g, NewInstruction1(var_addr_op,
                              var,
                              kAddrModeImplied));
      break;
    }

    case _6502_OP(symbol): {
      // ldx #%abs(symbol)+0
      // ldy #%abs(symbol)+1
      SetIndexReg(g, addr, addr, 0);
      Emit(g, NewInstruction2(_6502_OP(ldx), addr,
                              ByteConst(g, 0),
                              kAddrModeAbsoluteSymbol));
      SetIndexReg(g, addr, addr, 1);
      Emit(g, NewInstruction2(_6502_OP(ldy), addr,
                              ByteConst(g, 1),
                              kAddrModeAbsoluteSymbol));
      break;
    }
    default:
      abort();
      break;
  }
}

static TargetInstruction* GetDestAddress(_6502Generator* g, IRNode* node) {
  if (node->dest != NULL) {
    // Dest node might not be lowered yet (ssavar might be after this
    // node).  Lower it now.
    LowerIRNode(g, node->dest);
    return GetAddress(g, node->dest);
  }
  if (node->opcode == IR_OP(rmovi) ||node->opcode == IR_OP(rmova)  ||
      node->opcode == IR_OP(rmovf) ||
      node->opcode == IR_OP(rmovd)) {
    return GetAddress(g, node->inputs.value.p[0]);
  }
  return TempRegister(g, node->type->size);
}

static TargetInstruction* AddSubInteger(_6502Generator* g, IRNode* node,
                                        _6502Opcode op, _6502Opcode carry_ctl,
                                        int size, TargetInstruction* dest,
                                        TargetInstruction* op1,
                                        TargetInstruction* op2) {
  Emit(g, NewInstruction(carry_ctl, kAddrModeImplied));
  for (int i = 0; i < size; i++) {
    SetIndexReg2(g, op1, op2, dest, i);
    lda(g, op1, i);
    Operate(g, op, op2, i);
    sta(g, dest, i);
  }
  return dest;
}

static bool IsIdempotentOperation(_6502Opcode op, TargetInstruction* operand, AddressingMode mode, int index) {
  switch (op) {
    case _6502_OP(ora):
      // ORA #0 is idempotent
      if (mode == kAddrModeImmediate) {
        int immed = (int)TargetIntValue(operand);
        immed = (immed >> (index * 8)) & 0xff;
        if (immed == 0) {
          return true;
        }
      }
      break;
    case _6502_OP(and):
      // AND #255 is idempotent
      if (mode == kAddrModeImmediate) {
        int immed = (int)TargetIntValue(operand);
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

static TargetInstruction* SimpleIntegerOp(_6502Generator* g, IRNode* node,
                                          _6502Opcode op, int size,
                                          TargetInstruction* dest,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
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

// Unary not, equivalent to.
// dest = (src == 0) ? 1 : 0
//
// For 2 byte:
//  ldx #0
//  lda src lo
//  ORA src hi
//  BEQ zero_label
//  INX
// zero_label:
//  stx dest
//    Rest of dest is zeroed out.
static TargetInstruction* UnaryNotOp(_6502Generator* g, IRNode* src_node,
                                     TargetInstruction* dest,
                                     TargetInstruction* src, int dest_size) {
  int size = src_node->type->size;
  TargetInstruction* zero_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);

  AddressingMode dest_mode = GetAddrMode(dest);

  Emit(g, NewInstruction1(_6502_OP(ldx),
                          Zero(g),
                          kAddrModeImmediate));
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, src, dest, i);
    if (i == 0) {
      lda(g, src, i);
    } else {
      ora(g, src, i);
    }
  }

  EmitResolvedBranch(g, _6502_OP(beq), zero_label);
  Emit(g, NewInstruction(_6502_OP(inx), kAddrModeImplied));
  Emit(g, zero_label);

  // Store X in dest.  It's stored in low byte and the upper bytes are zeroed.
  // Dest will be in zero page.
  if (dest_mode != kAddrModeIndirectIndexed) {
    stx(g, dest, 0);
    for (int i = 1; i < dest_size; i++) {
       stz(g, dest, i);
     }
    
  } else {
    txa(g);
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
static TargetInstruction* OnesComplement(_6502Generator* g, IRNode* src_node,
                                         TargetInstruction* dest,
                                         TargetInstruction* src, int size) {
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, src, dest, i);
    lda(g, src, i);
    eori(g, 0xff);
    sta(g, dest, i);
  }
  return dest;
}

// SEC
// lda #0
// SBC src
// sta dest
// ...
static TargetInstruction* NegateInteger(_6502Generator* g, IRNode* src_node,
                                        TargetInstruction* dest,
                                        TargetInstruction* src, int size) {
  Emit(g, NewInstruction(_6502_OP(sec), kAddrModeImplied));
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, src, dest, i);
    ldai(g, 0);
    Operate(g, _6502_OP(sbc), src, i);
    sta(g, dest, i);
  }
  return dest;
}

static void ShiftOnce(_6502Generator* g, IRNode* node, _6502Opcode op,
                      _6502Opcode second_op, int size, TargetInstruction* dest,
                      TargetInstruction* op1) {
  if (op == _6502_OP(asl)) {
    // Shifting left, start at lsb
    for (int i = 0; i < size; i++) {
      SetIndexReg(g, op1, dest, i);
      if (dest == op1) {
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
      if (dest == op1) {
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

static TargetInstruction* ShiftOp(_6502Generator* g, IRNode* node,
                                  _6502Opcode op, _6502Opcode second_op,
                                  int size, TargetInstruction* dest,
                                  TargetInstruction* op1, int count) {
  if (count == 0) {
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
    ldxi(g, size-1);
    loop = Emit(g, NewInstruction(_6502_OP(label), kAddrModeImplied));
  }
  ShiftOnce(g, node, op, second_op, size, dest, dest);
  if (loop != NULL) {
    dex(g);
    EmitResolvedBranch(g, _6502_OP(bne), loop);
  }
  return dest;
}

static void ShiftOnceArithmeticRight(_6502Generator* g, IRNode* node,
                                     int size, TargetInstruction* dest,
                      TargetInstruction* src) {
  AddressingMode addrmode = GetAddrMode(src);
  for (int j = size - 1; j >= 0; j--) {
     AddressingMode mode = addrmode;
     SetIndexRegDown(g, src, dest, j, size);
     if (addrmode == kAddrModeIndirectIndexed && j == 0) {
       mode = kAddrModeIndirect;
     }
     if (dest == src) {
       Emit(g, NewInstruction2(_6502_OP(ror), src, ByteConst(g, j), mode));
     } else {
       lda(g, src, j);
       Emit(g, NewInstruction(_6502_OP(ror),  kAddrModeAccumulator));
       sta(g, dest, j);
     }
   }
}

static TargetInstruction* ArithmeticRightShiftOp(_6502Generator* g,
                                                 IRNode* node, int size,
                                                 TargetInstruction* dest,
                                                 TargetInstruction* src,
                                                 int count) {
  // lda hi
  // cmp $0x80
  // ror hi
  // ror lo
  SetIndexReg(g, src, dest, size-1);
  lda(g, src, size-1);
  cmpi(g, 0x80);            // Set carry flag if top bit set.
  
  // Firat shift puts src into dest.
  ShiftOnceArithmeticRight(g, node, size, dest, src);
  if (count == 1) {
    // Only one shift, we're done.
    return dest;
  }
  TargetInstruction* loop = NULL;
  if (count > 2) {
    // Need a loop.
    ldxi(g, size - 1);
    loop = Emit(g, NewInstruction(_6502_OP(label), kAddrModeImplied));
  }
  // Rest of shifts use dest only.
  SetIndexReg(g, dest, dest, size-1);
  lda(g, dest, size-1);
  cmpi(g, 0x80);            // Set carry flag if top bit set.
  ShiftOnceArithmeticRight(g, node, size, dest, dest);
  if (loop != NULL) {
    dex(g);
    EmitResolvedBranch(g, _6502_OP(bne), loop);
  }
  return dest;
}

static TargetInstruction* CallArithmeticBinaryRuntime(
    _6502Generator* g, IRNode* node, TargetInstruction* dest,
    TargetInstruction* op1, TargetInstruction* op2, Symbol* func) {
  TargetInstruction* tmp = NULL;
  if ((node->flags & kIRDestIsIndirect) != 0) {
    tmp = TempRegister(g, node->type->size);
    Emit(g, NewInstruction1(_6502_OP(expr_addr_a), tmp, kAddrModeImplied));
  } else {
    Emit(g, NewInstruction1(_6502_OP(expr_addr_a), dest, kAddrModeImplied));
  }
  // lda #dest
  // ldx #src1
  // ldy #src2
  // jsr func
  Emit(g, NewInstruction1(_6502_OP(expr_addr_x), op1, kAddrModeImplied));
  Emit(g, NewInstruction1(_6502_OP(expr_addr_y), op2, kAddrModeImplied));
  jsr(g, func);
  if ((node->flags & kIRDestIsIndirect) != 0) {
    // Dest is indirect, copy to it.
    AddressingMode mode = kAddrModeIndirect;
    for (int i = 0; i < node->type->size; i++) {
       SetIndexReg(g, tmp, dest, i);
       Emit(g, NewInstruction2(_6502_OP(lda), tmp, ByteConst(g, i), kAddrModeZeroPage));
       Emit(g, NewInstruction2(_6502_OP(sta), dest, ByteConst(g, i), mode));
       mode = kAddrModeIndirectIndexed;
     }
  }
  return dest;
}

static TargetInstruction* CallArithmeticUnaryRuntime(_6502Generator* g,
                                                     IRNode* node,
                                                     TargetInstruction* dest,
                                                     TargetInstruction* op,
                                                     Symbol* func) {
  // lda #dest
  // ldx #src1
  // JSR func
  Emit(g, NewInstruction1(_6502_OP(expr_addr_a), dest, kAddrModeImplied));
  Emit(g, NewInstruction1(_6502_OP(expr_addr_x), op, kAddrModeImplied));
  jsr(g, func);
  return dest;
}

// These macros fill out the arithmetic_runtimes array.  The
// preprocessor magic builds the name of a runtime function from
// an arithmetic opcode, signedness and size.  The functions
// are encoded as offsets into the _6502Generator struct.
#define ALUFUNC1(op, s, u)                                   \
  {IR_OP(op), true, 1, offsetof(_6502Generator, u##1)},      \
      {IR_OP(op), false, 1, offsetof(_6502Generator, s##1)}, \
      {IR_OP(op), true, 2, offsetof(_6502Generator, u##2)},  \
      {IR_OP(op), false, 2, offsetof(_6502Generator, s##2)}, \
      {IR_OP(op), true, 4, offsetof(_6502Generator, u##4)},  \
      {IR_OP(op), false, 4, offsetof(_6502Generator, s##4)}, \
      {IR_OP(op), true, 8, offsetof(_6502Generator, u##8)},  \
      {IR_OP(op), false, 8, offsetof(_6502Generator, s##8)},

#define ALUFUNC2(op, s1, s2, u1, u2)                              \
  {IR_OP(op), true, 1, offsetof(_6502Generator, u1##1##u2)},      \
      {IR_OP(op), false, 1, offsetof(_6502Generator, s1##1##s2)}, \
      {IR_OP(op), true, 2, offsetof(_6502Generator, u1##2##u2)},  \
      {IR_OP(op), false, 2, offsetof(_6502Generator, s1##2##s2)}, \
      {IR_OP(op), true, 4, offsetof(_6502Generator, u1##4##u2)},  \
      {IR_OP(op), false, 4, offsetof(_6502Generator, s1##4##s2)}, \
      {IR_OP(op), true, 8, offsetof(_6502Generator, u1##8##u2)},  \
      {IR_OP(op), false, 8, offsetof(_6502Generator, s1##8##s2)},

static struct {
  IROpcode opcode;
  bool is_unsigned;
  int size;
  int func_offset;
} arithmetic_runtimes[] = {
    ALUFUNC1(muli, umul, smul)  // muli -> umul[1,2,4,8] (size is inserted)
    ALUFUNC1(divi, udiv, sdiv) ALUFUNC1(modi, umod, smod)
        ALUFUNC2(i2f, i, tof, ui, tof) ALUFUNC2(i2d, i, tod, ui, tod)
            ALUFUNC2(f2i, ftoi, , ftoui, )  // Size is appended to these
    ALUFUNC2(d2i, dtoi, , dtoui, ){IR_OP(nop)}};

#undef ALUFUNC1
#undef ALUFUNC2

// Given an opcode, signedness and size, return a pointer to the symbol
// to be used for the arithmetic operation.
static Symbol* RuntimeFunction(_6502Generator* g, IROpcode opcode,
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
static void GetOpInstructions(_6502Generator* g, IRNode* node,
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
        ops[i] = GetAddress(g, input);
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
      case IR_OP(rmovi):
      case IR_OP(rmovf):
      case IR_OP(rmovd):
        ops[i] = Materialize(g, input);
        break;
        break;
      default:
        abort();
    }
  }
}

static void IncrementOnce(_6502Generator* g, IRNode* node, TargetInstruction* src) {
  int size = node->type->size;
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, src, src, i);
    if (i > 0) {
      TargetInstruction* label = NewInstruction(_6502_OP(label), kAddrModeImplied);
      EmitResolvedBranch(g, _6502_OP(bne), label);
      inc(g, src, i);
      Emit(g, label);
    } else {
      inc(g, src, i);
    }
  }
}

// Increment op1 by op2.  op2 will be a constant.
static TargetInstruction* Increment(_6502Generator* g, IRNode* node, IRNode* op1, IRNode* op2) {
  // For a 2-byte increment.  If only one iteration, omit loop.
  //  LDX #op2
  // loop:
  //  INC op1+0
  //  BNE xx
  //  INC op1+1
  // xx:
  //  DEX
  //  BME loop
  int amount = (int)IRIntConstValue(op2);
  if (amount == 0) {
    return NULL;
  }
  TargetInstruction* src = GetAddress(g, op1);
  
  AddressingMode mode = GetAddrMode(src);
  int size = node->type->size;
  
  if (mode == kAddrModeIndirect || mode == kAddrModeIndirectIndexed) {
    // No INC instructions for these addressing modes.  We need to add 1
    // to the src and store it back.
    return AddSubInteger(g, node, _6502_OP(adc), _6502_OP(clc), size, src,
                          src, ByteConst(g, amount));
}
  
  if (amount == 1) {
     IncrementOnce(g, node, src);
  } else {
    ldxi(g, amount);
    TargetInstruction* loop_label = Emit(g, NewInstruction(_6502_OP(label), kAddrModeImplied));
    IncrementOnce(g, node, src);
    dex(g);
    EmitResolvedBranch(g, _6502_OP(bne), loop_label);
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

static TargetInstruction* ReduceMultiplyOrDivide(_6502Generator* g, IRNode* node) {
  switch (node->opcode) {
    case IR_OP(muli): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) || IRIsConst(op2)) {
        if (IRIsConst(op1) && IRIsConst(op2)) {
          break;
        }
        TargetInstruction* dest = GetDestAddress(g, node);
        TypeRecord* type = node->type;
        int size = type == NULL ? 2 : type->size;

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
            return SetLoweredNode(node, ShiftOp(g, node, _6502_OP(asl), _6502_OP(rol), size, dest, GetAddress(g, op2),
                           Log2(c)));

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
            return SetLoweredNode(node, ShiftOp(g, node, _6502_OP(asl), _6502_OP(rol), size, dest, GetAddress(g, op1),
                            Log2(c)));
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
        TargetInstruction* dest = GetDestAddress(g, node);
        TypeRecord* type = node->type;
        int size = type == NULL ? 2 : type->size;
        bool is_unsigned = type != NULL && TypeIsUnsigned(type);

        // One is constant, if it's a power of 2 we can create a loop
        // of shifts.
        if (IRIsConst(op2)) {
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (IsPowerOf2(c)) {
            if (is_unsigned) {
                return SetLoweredNode(node, ShiftOp(g, node, _6502_OP(lsr), _6502_OP(ror), size, dest, GetAddress(g, op1),
                                Log2(c)));
            } else {
              return SetLoweredNode(node, ArithmeticRightShiftOp(g, node, size, dest, GetAddress(g, op1),
                                             Log2(c)));
              
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
    case IR_OP(ssavar):
    case IR_OP(phi):
      return true;
    default:
      return false;
  }
}

static bool IsSameVariable(_6502Generator* g, IRNode* node1, IRNode* node2) {
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

static TargetInstruction* LowerExpression(_6502Generator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  IROpcode op = node->opcode;
  
  // See if we can increment the a variable by a small amount.
  if (op == IR_OP(addi)) {
    IRNode* op1 = node->inputs.value.p[0];
    IRNode* op2 = node->inputs.value.p[1];
    if (IRIsConst(op2) && node->dest != NULL) {
      // Adding a constant less than 4?
      if (IRIntConstValue(op2) < 4) {
        if (op1->opcode == IR_OP(loadi)) {
          IRNode* loaded = op1->inputs.value.p[0];
          if (node->dest->id == loaded->id) {
            // Adding to itself.
            return Increment(g, node, loaded, op2);
          }
        } else if (IsVariableNode(op1) && IsVariableNode(node->dest)) {
          if (IsSameVariable(g, op1, node->dest)) {
            return Increment(g, node, op1, op2);
          }
        }
      }
    }
  }
  TargetInstruction* inst = ReduceMultiplyOrDivide(g, node);
  if (inst != NULL) {
    return inst;
  }
  
  TargetInstruction* ops[2] = {NULL, NULL};
  assert(node->inputs.length < 3);
  GetOpInstructions(g, node, ops);

  TargetInstruction* dest = GetDestAddress(g, node);
  TypeRecord* type = node->type;
  int size = type == NULL ? 2 : type->size;
  bool is_unsigned = type != NULL && TypeIsUnsigned(type);

  switch (op) {
    case IR_OP(addi):
      inst = AddSubInteger(g, node, _6502_OP(adc), _6502_OP(clc), size, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(adda):
      inst = AddSubInteger(g, node, _6502_OP(adc), _6502_OP(clc), 2, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(addf):
    case IR_OP(addd):
      break;

    case IR_OP(subi):
      inst = AddSubInteger(g, node, _6502_OP(sbc), _6502_OP(sec), size, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(suba):
      inst = AddSubInteger(g, node, _6502_OP(sbc), _6502_OP(sec), 2, dest,
                           ops[0], ops[1]);
      break;
    case IR_OP(subf):
    case IR_OP(subd):
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
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->dmul);
      break;
    case IR_OP(divf):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->fdiv);
      break;
    case IR_OP(divd):
      inst =
          CallArithmeticBinaryRuntime(g, node, dest, ops[0], ops[1], g->ddiv);
      break;

    case IR_OP(lsri):
      inst = ShiftOp(g, node, _6502_OP(lsr), _6502_OP(ror), size, dest, ops[0],
                     (int)TargetIntValue(ops[1]));
      break;
    case IR_OP(asri): {
      // Copy sign bit to carry bit.
      // TargetInstruction* carry = NewInstruction1(_6502_OP(cmp),
      // GetIntConstant(g, node, kTargetTypeByte, 0x80), kAddrModeImmediate);
      inst = ArithmeticRightShiftOp(g, node, size, dest, ops[0],
                                    (int)TargetIntValue(ops[1]));
      break;
    }
    case IR_OP(lsli):
      inst = ShiftOp(g, node, _6502_OP(asl), _6502_OP(rol), size, dest, ops[0],
                     (int)TargetIntValue(ops[1]));
      break;

    case IR_OP(ori):
      inst =
          SimpleIntegerOp(g, node, _6502_OP(ora), size, dest, ops[0], ops[1]);
      break;

    case IR_OP(andi):
      inst =
          SimpleIntegerOp(g, node, _6502_OP(and), size, dest, ops[0], ops[1]);
      break;
    case IR_OP(xori):
      inst =
          SimpleIntegerOp(g, node, _6502_OP(eor), size, dest, ops[0], ops[1]);
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
      abort();
      break;

    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2i):
    case IR_OP(d2i):
      inst = CallArithmeticUnaryRuntime(
          g, node, dest, ops[0], RuntimeFunction(g, op, is_unsigned, size));
      break;
    case IR_OP(f2d):
      inst = CallArithmeticUnaryRuntime(g, node, dest, ops[0], g->ftod);
      break;
    case IR_OP(d2f):
      inst = CallArithmeticUnaryRuntime(g, node, dest, ops[0], g->dtof);
      break;

    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova): {
      inst = GetDestAddress(g, node);
      TargetInstruction* src = GetAddress(g, node->inputs.value.p[0]);

      for (int i = 0; i < node->type->size; i++) {
        SetIndexReg(g, ops[i], inst, i);
        lda(g, src, i);
        sta(g, dest, i);
      }
      break;
    }
    case IR_OP(rmovi):
    case IR_OP(rmovf):
    case IR_OP(rmovd):
    case IR_OP(rmova): {
      IRNode* move_dest = node->inputs.value.p[0];
      if (move_dest->outputs.length == 1) {
        // The only output from the destination of the rmov is the
        // rmov itself.  We can omit the rmov.
        return NULL;
      }
      inst = GetDestAddress(g, node->inputs.value.p[0]);
      TargetInstruction* src = GetAddress(g, node->inputs.value.p[1]);

      for (int i = 0; i < node->type->size; i++) {
        SetIndexReg(g, ops[i], inst, i);
        lda(g, src, i);
        sta(g, dest, i);
      }
      break;
    }
    case IR_OP(tmp):
      inst = TempRegister(g, size);
      break;
    default:
      abort();
      assert(false);
      return 0;
  }

  TargetUpdateOperandUsers(inst);
  SetLoweredNode(node, inst);
  return Emit(g, inst);
}

static void CompareEqualZero(_6502Generator* g, IRNode* value_node,
                             IRNode* target_node) {
  // For 2 byte:
  //  lda byte1
  //  ORA byte2
  //  BEQ true_label

  int size = ((IRNode*)(value_node))->type->size;
  TargetInstruction* value = GetAddress(g, value_node);

  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
  }
  TargetInstruction* bra = EmitBranch(g, _6502_OP(beq), target_node);
  bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareEqualInteger(_6502Generator* g, IRNode* cmp_node,
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

  int size = ((IRNode*)(cmp_node->inputs.value.p[0]))->type->size;
  TargetInstruction* value1 = GetAddress(g, cmp_node->inputs.value.p[0]);
  TargetInstruction* value2 = GetAddress(g, cmp_node->inputs.value.p[1]);

  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    if (i < (size - 1)) {
      EmitResolvedBranch(g, _6502_OP(bne), false_label);
    } else {
      EmitBranch(g, _6502_OP(beq), target_node);
    }
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(_6502_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareNotEqualZero(_6502Generator* g, IRNode* value_node,
                                IRNode* target_node) {
  // For 2 byte:
  //  lda byte1
  //  ORA byte2
  //  BNE true_label

  int size = ((IRNode*)(value_node))->type->size;
  TargetInstruction* value = GetAddress(g, value_node);

  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
  }
  TargetInstruction* bra = EmitBranch(g, _6502_OP(bne), target_node);
  bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareNotEqualInteger(_6502Generator* g, IRNode* cmp_node,
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

  int size = ((IRNode*)(cmp_node->inputs.value.p[0]))->type->size;
  TargetInstruction* value1 = GetAddress(g, cmp_node->inputs.value.p[0]);
  TargetInstruction* value2 = GetAddress(g, cmp_node->inputs.value.p[1]);

  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    EmitBranch(g, _6502_OP(bne), target_node);
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(_6502_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareLessUnsignedInteger(_6502Generator* g, IRNode* lhs_node,
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

  if (IRIsZero(rhs_node)) {
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

  int size = lhs_node->type->size;
  TargetInstruction* value1 = GetAddress(g, lhs_node);
  TargetInstruction* value2 = GetAddress(g, rhs_node);

  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  for (int i = size - 1; i >= 0; i--) {
    SetIndexRegDown(g, value1, value2, i, size);
    lda(g, value1, i);
    cmp(g, value2, i);
    EmitBranch(g, _6502_OP(bcc), target_node);
    if (i > 0) {
      EmitResolvedBranch(g, _6502_OP(bne), false_label);
    }
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(_6502_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

static void CompareGreaterOrEqualUnsignedInteger(_6502Generator* g,
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
    TargetInstruction* lab = EmitBranch(g, _6502_OP(bra), target_node);
    lab->flags |= k6502BlockEnd | k6502InstIsCondBranch;
    return;
  }
  if (IRIsZero(lhs_node)) {
    // 0 >= x
    // Same as x <= 0, same as x == 0 for unsigned values.
    CompareEqualZero(g, rhs_node, target_node);
    return;
  }

  int size = lhs_node->type->size;
  TargetInstruction* value1 = GetAddress(g, lhs_node);
  TargetInstruction* value2 = GetAddress(g, rhs_node);

  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  for (int i = size - 1; i >= 0; i--) {
    SetIndexRegDown(g, value1, value2, i, size);

    lda(g, value1, i);
    cmp(g, value2, i);
    if (i == 0) {
      EmitBranch(g, _6502_OP(bcs), target_node);
    } else {
      EmitResolvedBranch(g, _6502_OP(bcc), false_label);
      EmitBranch(g, _6502_OP(bne), target_node);
    }
  }
  Emit(g, false_label);

  TargetInstruction* fake_branch =
      NewInstruction(_6502_OP(fake_bra), kAddrModeImplied);
  EmitLabelReference(g, fake_branch, target_node);
  fake_branch->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

// Compares 2 signed integers for < or >=.  Branches to target
static void CompareSignedInteger(_6502Generator* g,
                                           IRNode* lhs_node,
                                           IRNode* rhs_node,
                                           IRNode* target_node,
                                           bool less_than) {
  //  lda byte1
  //  CMP value1
  //  lda byte2
  //  sbc value2
  //  bvc skip_label
  //  eor #0x80
  // skip_label:              N = 1 means less
  //  bmi target_node [or bpl for >=)


  TargetInstruction* value1 = GetAddress(g, lhs_node);
  TargetInstruction* value2 = GetAddress(g, rhs_node);
  TargetInstruction* skip_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  sec(g);
  int size = lhs_node->type->size;
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    if (i == 0) {
      cmp(g, value2, i);
    } else {
      sbc(g, value2, i);
    }
  }
  EmitResolvedBranch(g, _6502_OP(bvc), skip_label);
  eori(g, 0x80);
  Emit(g, skip_label);
  TargetInstruction* bra = EmitBranch(g, less_than ? _6502_OP(bmi) : _6502_OP(bpl), target_node);
  bra->flags |= k6502BlockEnd | k6502InstIsCondBranch;
}

// If the branch comes from a comparison node we combine the comparison
// with the branch.  If it comes from another node we generate an equality
// comparison of that node with zero.
static TargetInstruction* LowerConditionalBranch(_6502Generator* g,
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
    // Generate equality comparison with zero.
    if (reverse) {
      CompareEqualZero(g, expr, target_node);
    } else {
      CompareNotEqualZero(g, expr, target_node);
    }
    return NULL;
  }
  IRNode* lhs = input->inputs.value.p[0];
  IRNode* rhs = input->inputs.value.p[1];
  bool is_unsigned = TypeIsUnsigned(lhs->type);
  
  switch (expr->opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
      if (reverse) {
          CompareNotEqualInteger(g, input, target_node);
       } else {
          CompareEqualInteger(g, input, target_node);
      }
      break;
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
      if (reverse) {
        CompareEqualInteger(g, input, target_node);
      } else {
        CompareNotEqualInteger(g, input, target_node);
      }
      break;
      
    case IR_OP(cmplti):
    case IR_OP(cmplta):
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
            CompareSignedInteger(g, rhs, lhs, target_node, false);
        }
      }
      break;
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
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
      break;
    default:
      abort();
  }

  return NULL;
}

static TargetInstruction* LowerBranch(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* target_node = node->inputs.value.p[0];

  _6502Opcode opcode = _6502_OP(bra);
  if ((node->flags & kIRJumpTableBranch) != 0) {
    // Need all jump table entries to be the same length.
    opcode = _6502_OP(jumptable);
  }
  TargetInstruction* b = EmitBranch(g, opcode, target_node);
  b->flags |= k6502BlockEnd;
  return NULL;
}

// Computed branch.  This is followed by a series of jumptable entries, each
// of which is 2 bytes long.
// LDA #X
// JSR __jump_table
// Table starts here.
static TargetInstruction* LowerComputedBranch(_6502Generator* g, IRNode* node) {
  TargetInstruction* byte_offset = Materialize(g, node->inputs.value.p[0]);
  ldazi(g, byte_offset, 0);
  TargetInstruction* jump = jsr(g, g->jump_table);
  jump->flags |= TARGET_INST_TABLE_JUMP | k6502BlockEnd;
  SetLoweredNode(node, jump);
  return jump;
}

static TargetInstruction* LowerLabel(_6502Generator* g, IRNode* label) {
  TargetInstruction* inst =
      Emit(g, NewInstruction(_6502_OP(label), kAddrModeImplied));
  label->data.ptr = inst;
  inst->flags |= k6502BlockStart;
  ApplyFixups(g, label);
  return inst;
}

static TargetInstruction* LowerNamedLabel(_6502Generator* rv, IRNode* label) {
  IRNamedLabel* n = (IRNamedLabel*)label;
  TargetInstruction* inst = Emit(rv, TargetNewNamedLabel(n->name));
  inst->flags |= k6502BlockStart;
  label->data.ptr = inst;
  return inst;
}

static TargetInstruction* LowerLoad(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];

  TargetInstruction* dest = GetDestAddress(g, node);
  TargetInstruction* src = GetAddress(g, addr_node);
  AddressingMode src_mode = kAddrModeIndirect;
  AddressingMode dest_mode = (node->flags & kIRDestIsIndirect) != 0 ? kAddrModeIndirect : GetAddrMode(dest);

  for (int i = 0; i < node->type->size; i++) {
    SetIndexReg(g, src, dest, i);
    Emit(g, NewInstruction2(_6502_OP(lda), src, ByteConst(g, i), src_mode));
    Emit(g, NewInstruction2(_6502_OP(sta), dest, ByteConst(g, i), dest_mode));
    src_mode = kAddrModeIndirectIndexed;
    if (dest_mode == kAddrModeIndirect) {
      dest_mode = kAddrModeIndirectIndexed;
    }
  }
  SetLoweredNode(node, dest);
  return dest;
}

static TargetInstruction* LowerStore(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length >= 2);

  // Address to store to is the first operand of the store IR node.
  // The value to store is the second operand.
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];
  int start_index = 0;
  
  // We might have a 3rd argument - starting index for the store.
  if (node->inputs.length == 3) {
    start_index = (int)IRIntConstValue(node->inputs.value.p[2]);
  }
  
  TargetInstruction* src = GetAddress(g, src_node);
  AddressingMode src_mode = GetAddrMode(src);
  switch (addr_node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(ssavar):
    case IR_OP(phi): {
      // Store to a local variable.
      TargetInstruction* addr = GetAddress(g, addr_node);
       assert(node->inputs.length == 2);
      TargetInstruction* addr_inst = GetLoweredNode(addr_node);
      if (addr_node->opcode == IR_OP(ssavar) || addr_node->opcode == IR_OP(phi)) {
        addr_inst = addr_inst->operand[0];
      }
        int offset = (int)TargetIntValue(addr_inst->operand[0]);
        // var_addr is the address of the variable.
        // lda src+0
        // sta (var_addr)
        // ldy #1 or INY
        // lda src+1
        // sta (var_addr), Y
        for (int i = 0; i < node->type->size; i++) {
          SetIndexReg(g, src, addr, i);
          lda(g, src, i);
          sta(g, addr, i);
        }
        break;
      }
    case IR_OP(staticvar): {
      // Store to a static variable.
      assert(node->inputs.length == 2);
      TargetInstruction* addr = GetAddress(g, addr_node);
      for (int i = 0; i < node->type->size; i++) {
        SetIndexReg(g, src, addr, i);
        lda(g, src, i);
        Emit(g, NewInstruction2(_6502_OP(lda), src,
                                ByteConst(g, i),
                                src_mode));
        Emit(g, NewInstruction2(_6502_OP(sta), addr,
                               ByteConst(g, i),
                                kAddrModeAbsoluteSymbol));
      }
      break;
    }
      
    default: {
      TargetInstruction* addr = Materialize(g, addr_node);
      int store_size;
      switch (node->opcode) {
        case IR_OP(storeb):
          store_size = 1;
          break;
        case IR_OP(storea):
        case IR_OP(stores):
          store_size = 2;
          break;
        case IR_OP(storei):
        case IR_OP(storef):
          store_size = 4;
          break;
        case IR_OP(storel):
        case IR_OP(stored):
          store_size = 8;
          break;
        default:
          abort();
      }
      // Store indirect via addr.
      AddressingMode dest_mode = start_index == 0 ? kAddrModeIndirect : kAddrModeIndirectIndexed;
      int j = start_index;
      for (int i = 0; i < store_size; i++, j++) {
        SetIndexReg(g, src, src, i);
        lda(g, src, i);
        if (i != j) {
          ldyi(g, j);
        }
        Emit(g, NewInstruction2(_6502_OP(sta), addr, Zero(g), dest_mode));
        dest_mode = kAddrModeIndirectIndexed;
      }
      break;
    }
  }
  
#if 0
  TargetInstruction* addr = Materialize(g, addr_node);
  int offset;
  switch ((_6502Opcode)addr->opcode) {
    case _6502_OP(argument):
    case _6502_OP(localvar): {
      assert(node->inputs.length == 2);
      offset = (int)TargetIntValue(addr->operand[0]);
      // lda #dest_addr (_6502_OP(expr_addr_a))
      // ldx #offset lo
      // ldy #offset hi (removed for single byte case)
      // JSR __var_addr[1,2]
      ldxi(g, offset & 0xff);
      Symbol* var_addr_sym =
          addr->opcode == _6502_OP(argument) ? g->arg_addr : g->var_addr;
      if (offset >= 256) {
        ldyi(g, offset >> 8);
        var_addr_sym =
            addr->opcode == _6502_OP(argument) ? g->arg_addrb : g->var_addrb;
      }
      jsr(g, var_addr_sym);

      // var_addr is the address of the variable.
      // lda src+0
      // sta (var_addr)
      // ldy #1 or INY
      // lda src+1
      // sta (var_addr), Y
      for (int i = 0; i < node->type->size; i++) {
        SetIndexReg(g, src, addr, i);
        lda(g, src, i);
        sta(g, addr, i);
      }
      break;
    }

    case _6502_OP(symbol): {
      // lda src+0
      // sta %abs(symbol)
      // lda src+1
      // sta %abs(symbol)+1
      // ...
      assert(node->inputs.length == 2);
      for (int i = 0; i < node->type->size; i++) {
        SetIndexReg(g, src, addr, i);
        lda(g, src, i);
        Emit(g, NewInstruction2(_6502_OP(lda), src,
                                ByteConst(g, i),
                                src_mode));
        Emit(g, NewInstruction2(_6502_OP(sta), addr,
                               ByteConst(g, i),
                                kAddrModeAbsoluteSymbol));
      }
      break;
    }
    case _6502_OP(expr2): {
      // Store indirect via addr.
      AddressingMode dest_mode = start_index == 0 ? kAddrModeIndirect : kAddrModeIndirectIndexed;
      int j = start_index;
      for (int i = 0; i < node->type->size; i++, j++) {
        SetIndexReg(g, src, src, i);
        lda(g, src, i);
        ldyi(g, j);
        Emit(g, NewInstruction2(_6502_OP(sta), addr, Zero(g), dest_mode));
        dest_mode = kAddrModeIndirectIndexed;
      }
      break;
      }
    default:
      abort();
  }
  #endif

  return NULL;
}

static struct {
  bool (*type_func)(TypeRecord*);
  int size;
} push_map[] = {
    {TypeIsInt, 2},      {TypeIsShort, 2},         {TypeIsChar, 1},
    {TypeIsLong, 4},     {TypeIsLongLong, 8},      {TypeIsFloat, 4},
    {TypeIsDouble, 8},   {TypeIsLongDouble, 8},    {TypeIsPointerOrArray, 2},
  {TypeIsFunction, 2}, {TypeIsStructOrUnion, 2}, {TypeIsBool, 1}, {NULL, 0},
};

static void PushExpression(_6502Generator* g, IRNode* node,
                           TargetInstruction* inst, int size) {
  switch (size) {
    case 1:
      lda(g, inst, 0);
      jsr(g, g->pusha);
      break;
    case 2:
      ldx(g, inst, 0);
      ldy(g, inst, 1);
      jsr(g, g->pushxy);
      break;

    case 4:
      ldxzi(g, inst, 0);
      jsr(g, g->push4);
      break;

    case 8:
      ldxzi(g, inst, 0);
      jsr(g, g->push8);
      break;

    default:
      abort();
  }
}

// For one or two byte constants we load the value into A or X,Y and call a push
// function.
//
// For 4 and 8 byte constants we load the address of the constant literal into
// X,Y and call the push function.

static void PushConstant(_6502Generator* g, IRNode* node, int size) {
  uint64_t value = IRIntConstValue(node);
  switch (size) {
    case 1:
      Emit(g, NewInstruction1(
                  _6502_OP(lda),
                  ByteConst(g, value & 0xff),
                  kAddrModeImmediate));
      jsr(g, g->pusha);
      break;

    case 2:
      Emit(g, NewInstruction1(
                  _6502_OP(ldx),
                  ByteConst(g, value & 0xff),
                  kAddrModeImmediate));
      if (value > 255) {
        Emit(g, NewInstruction1(
                                _6502_OP(ldy),
                                ByteConst(g, (value >> 8) & 0xff),
                                kAddrModeImmediate));
        jsr(g, g->pushxy);
      } else {
        jsr(g, g->pushxy0);
      }
      break;

    case 4:
      Emit(g, NewInstruction1(
                  _6502_OP(literalref),
                              GetLoweredNode(node),
                  kAddrModeAbsolute));
      jsr(g, g->push4xy);
      break;

    case 8:
      Emit(g, NewInstruction1(
                  _6502_OP(literalref),
                  GetLoweredNode(node),
                  kAddrModeAbsolute));
      jsr(g, g->push8xy);
      break;

    default:
      abort();
  }
}

static void PushVariable(_6502Generator* g, IRNode* node, int size) {
  TargetInstruction* addr = GetLoweredNode(node);
  int offset = (int)TargetIntValue(addr->operand[0]);
  struct {
    int size;
    Symbol* push_var;   // 1-byte offset
    Symbol* push_arg;   // 1-byte offset
    Symbol* push_varb;  // 2-byte offset
    Symbol* push_argb;  // 2-byte offset
  } var_pushes[] = {
      {1, g->push_var1, g->push_arg1, g->push_var1b, g->push_arg1b},
      {2, g->push_var2, g->push_arg2, g->push_var2b, g->push_arg2b},
      {4, g->push_var4, g->push_arg4, g->push_var4b, g->push_arg4b},
      {8, g->push_var8, g->push_arg8, g->push_var8b, g->push_arg8b},
      {0},
  };
  Symbol* push_sym = NULL;
  if (TypeIsArray(node->type) || TypeIsStructOrUnion(node->type)) {
    // Get the address of these and push them.
    _6502Opcode addr_op;
    if (offset >= 256) {
      addr_op = addr->opcode == _6502_OP(argument) ? _6502_OP(arg_addrb_xy)
                                                    : _6502_OP(var_addrb_xy);
    } else {
      addr_op = addr->opcode == _6502_OP(argument) ? _6502_OP(arg_addr_xy)
                                                    : _6502_OP(var_addr_xy);
    }
    Emit(g, NewInstruction1(addr_op, addr, kAddrModeImplied));
    jsr(g, g->pushxy);
    return;
  }
  for (size_t i = 0; i < var_pushes[i].size != 0; i++) {
    if (var_pushes[i].size == size) {
      if (offset >= 256) {
        push_sym = addr->opcode == _6502_OP(argument) ? var_pushes[i].push_argb
                                                      : var_pushes[i].push_varb;
      } else {
        push_sym = addr->opcode == _6502_OP(argument) ? var_pushes[i].push_arg
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
    jsr(g, push_sym);
}


static void Push(_6502Generator* g, IRNode* node, int size) {
  TargetInstruction* inst = NULL;
  switch (node->opcode) {
    case IR_OP(argument):
    case IR_OP(localvar):
      PushVariable(g, node, size);
      break;
      
    case IR_OP(ssavar):
    case IR_OP(phi): {
      IRVariable* var = (IRVariable*)node;
      IRNode* symbol = FindPooledVariable(g->gen, var->symbol);
      PushVariable(g, symbol, size);
      break;
    }
    default:
      if (IRIsConst(node)) {
        PushConstant(g, node, size);
      } else {
        inst = Materialize(g, node);
        PushExpression(g, node, inst, size);
      }
      break;
  }
}

static void PushArg(_6502Generator* g, IRNode* node, size_t* size) {
  if (node->type == NULL) {
    Push(g, node, 2);
  }
  for (size_t i = 0; push_map[i].type_func != NULL; i++) {
    if (push_map[i].type_func(node->type)) {
      if (size != NULL) {
        *size += push_map[i].size;
      }
      Push(g, node, push_map[i].size);
      return;
    }
  }
  assert(false);
}

// Passing a struct or union to a function needs to copy
// the memory from the address to the stack.
// mem_src: source address
// mem_size: size of memory
// JSR pushmem
static void PushStructArg(_6502Generator* g, IRNode* node, size_t* args_size) {
  size_t struct_size = node->type->size;
  *args_size += struct_size;
  Symbol* pushmem = g->pushmem1;

  // Put size in __mem_size.
  Emit(g, NewInstruction1(
              _6502_OP(lda),
              ByteConst(g,  struct_size & 0xff),
              kAddrModeImmediate));
  Emit(g,
       NewInstruction1(_6502_OP(sta),
                       ByteConst(g, _6502_MSZ_REG),
                       kAddrModeZeroPageAbsolute));
  if (struct_size >= 256) {
    pushmem = g->pushmem2;
    Emit(g, NewInstruction1(_6502_OP(lda),
                            ByteConst(g,
                                           (struct_size >> 8) & 0xff),
                            kAddrModeImmediate));
    Emit(g, NewInstruction1(
                _6502_OP(sta),
                ByteConst(g, _6502_MSZ_REG + 1),
                kAddrModeZeroPageAbsolute));
  }

  // Put src in __mem_src
  TargetInstruction* src = GetAddress(g, node);
  Emit(g, NewInstruction2(_6502_OP(lda), src,
                          Zero(g),
                          GetAddrMode(src)));
  Emit(g,
       NewInstruction1(_6502_OP(sta),
                       GetIntConstant(g, NULL, kTargetTypeByte, _6502_MSRC_REG),
                       kAddrModeZeroPageAbsolute));
  Emit(g, NewInstruction2(_6502_OP(lda), src,
                          One(g),
                          GetAddrMode(src)));
  Emit(g, NewInstruction1(
              _6502_OP(sta),
              ByteConst(g, _6502_MSRC_REG + 1),
              kAddrModeZeroPageAbsolute));

  // JSR pushmem
  jsr(g, pushmem);
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
static TargetInstruction* LowerCall(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length >= 1);
  size_t args_size = 0;
  for (size_t i = node->inputs.length - 1; i >= 1; i--) {
    IRNode* arg_node = node->inputs.value.p[i];
    if (TypeIsStructOrUnion(arg_node->type)) {
      PushStructArg(g, arg_node, &args_size);
    } else {
      PushArg(g, arg_node, &args_size);
    }
  }
  // Load return address into A (in zero page).
  TargetInstruction* result = NULL;
  if (!TypeIsVoid(node->type)) {
    if (node->dest != NULL) {
      LowerIRNode(g, node->dest);
      result = GetLoweredNode(node->dest);
      if (_6502IsExpression(result)) {
        ldxzi(g, result, 0);
        ldyi(g, 0);
      } else {
        // Not an expression, get address in X,Y
        GetAddressXY(g, node->dest);
      }
    } else {
      result = TempRegister(g, node->type->size);
      ldxzi(g, result, 0);
      ldyi(g, 0);
    }
    result->flags |= k6502ExprIsCallResult;
  }
  TargetInstruction* addr = GetLoweredNode(node->inputs.value.p[0]);
  TargetInstruction* call = NULL;
  if (addr->opcode == _6502_OP(symbol)) {
    call = Emit(g, NewInstruction1(_6502_OP(jsr), addr, kAddrModeAbsolute));
    call->flags |= k6502ProcedureCall;
  } else {
    // Calling an expression.
    addr = Materialize(g, node->inputs.value.p[0]);     // Address to call.
    TargetInstruction* call_label =
        NewInstruction(_6502_OP(label), kAddrModeImplied);
    TargetInstruction* end_label =
        NewInstruction(_6502_OP(label), kAddrModeImplied);
    call = Emit(g, NewInstruction1(_6502_OP(jsr), call_label, kAddrModeAbsolute));
    call->flags |= k6502ProcedureCall;
    Emit(g, NewInstruction1(_6502_OP(bra), end_label, kAddrModeAbsolute));
    Emit(g, call_label);
    Emit(g, NewInstruction1(_6502_OP(jmp), addr, kAddrModeIndirect));
    call = Emit(g, end_label);
  }
  if (args_size > 0) {
    Symbol* incsp = g->incsp1;
    Emit(g, NewInstruction1(
                _6502_OP(ldx),
                ByteConst(g, args_size & 0xff),
                kAddrModeImmediate));
    if (args_size >= 256) {
      Emit(g, NewInstruction1(
                  _6502_OP(ldy),
                  ByteConst(g, (args_size >> 8) & 0xff),
                  kAddrModeImmediate));
      incsp = g->incsp2;
    }

    call = jsr(g, incsp);
  }
  call->flags |= k6502InstIsCall;
  if (result == NULL) {
    result = call;
  }
  SetLoweredNode(node, result);
  return result;
}

// Input is the value of the result in zero page.
// Calls:
// JSR resultX (where X is 2,4,8)
// or, for single byte.
// LDA v
// STA (__result)
static void AssignResult(_6502Generator* g, TargetInstruction* inst, int size) {
  Symbol* result_func;
  switch (size) {
    case 1:
      lda(g, inst, 0);
      Emit(g, NewInstruction1(
                   _6502_OP(sta),
                   ByteConst(g,  _6502_RESULT_REG),
                   kAddrModeIndirect));
      return;
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

static TargetInstruction* LowerResult(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  int size = node->type->size;
  TargetInstruction* rnode = Materialize(g, node->inputs.value.p[0]);
  if (!IsLeaf(g)) {
    jsr(g, g->load_result);
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
      AssignResult(g, rnode, 8);
      break;
    default:
      assert(false);
      break;
  }
  return NULL;
}

// A literal reference is a move of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static TargetInstruction* LowerLiteralReference(_6502Generator* g,
                                                IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, TargetNewLiteral((int)id_node->value.ivalue));
  TargetInstruction* dest = NULL;
  if (node->dest != NULL) {
    dest = GetAddress(g, node->dest);
  } else {
    dest = TempRegister(g, 2);
  }

  Emit(g, NewInstruction1(_6502_OP(literalreflo), literal,
                          kAddrModeImplied));
  SetIndexReg(g, dest, dest, 0);
  sta(g, dest, 0);
  Emit(g, NewInstruction1(_6502_OP(literalrefhi), literal,
                          kAddrModeImplied));
  SetIndexReg(g, dest, dest, 1);
  sta(g, dest, 1);
  SetLoweredNode(node, dest);
  return dest;
}

static TargetInstruction* LowerAddressOf(_6502Generator* g, IRNode* node) {
  TargetInstruction* result = GetAddress(g, node->inputs.value.p[0]);
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerMemcpy(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length == 3);
  Symbol* copymem = g->copymem1;

  // Put size in __mem_size.
  TargetInstruction* size_node = GetLoweredNode(node->inputs.value.p[2]);
  int64_t size = TargetIntValue(size_node);
  ldai(g, size & 0xff);
  Emit(g,
       NewInstruction1(_6502_OP(sta),
                       ByteConst(g, _6502_MSZ_REG),
                       kAddrModeZeroPageAbsolute));
  if (size >= 256) {
    copymem = g->copymem2;
    ldai(g, (size >> 8) & 0xff);
    Emit(g, NewInstruction1(
                _6502_OP(sta),
                ByteConst(g,  _6502_MSZ_REG + 1),
                kAddrModeZeroPageAbsolute));
  }

  // Put src in __mem_src
  TargetInstruction* src = GetAddress(g, node->inputs.value.p[1]);
  lda(g, src, 0);
  Emit(g,
       NewInstruction1(_6502_OP(sta),
                       ByteConst(g, _6502_MSRC_REG),
                       kAddrModeZeroPageAbsolute));
  lda(g, src, 1);
  Emit(g, NewInstruction1(
              _6502_OP(sta),
              ByteConst(g,  _6502_MSRC_REG + 1),
              kAddrModeZeroPageAbsolute));

  // Put dest in __mem_dest.
  TargetInstruction* dest = GetDestAddress(g, node->inputs.value.p[0]);
  lda(g, dest, 0);
  Emit(g,
       NewInstruction1(_6502_OP(sta),
                       ByteConst(g, _6502_MSRC_REG),
                       kAddrModeZeroPageAbsolute));
  lda(g, dest, 0);
  Emit(g, NewInstruction1(
              _6502_OP(sta),
              ByteConst(g,  _6502_MSRC_REG + 1),
              kAddrModeZeroPageAbsolute));

  // JSR copymem
  jsr(g, copymem);
  return NULL;
}

static TargetInstruction* LowerMemzero(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  Symbol* zeromem = g->zeromem1;

  // Put size in __mem_size.
  TargetInstruction* size_node = GetLoweredNode(node->inputs.value.p[2]);
  int64_t size = TargetIntValue(size_node);
  ldai(g, size & 0xff);
  Emit(g,
       NewInstruction1(_6502_OP(sta),
                       ByteConst(g,  _6502_MSZ_REG),
                       kAddrModeZeroPageAbsolute));
  if (size >= 256) {
    zeromem = g->zeromem2;
    ldai(g, (size >> 8) & 0xff);
    Emit(g, NewInstruction1(
                _6502_OP(sta),
                ByteConst(g,  _6502_MSZ_REG + 1),
                kAddrModeZeroPageAbsolute));
  }

  // Put dest in __mem_dest.
  TargetInstruction* dest = GetDestAddress(g, node->inputs.value.p[0]);
  lda(g, dest, 0);
  Emit(g,
       NewInstruction1(_6502_OP(sta),
                       ByteConst(g, _6502_MSRC_REG),
                       kAddrModeZeroPageAbsolute));
  lda(g, dest, 0);
  Emit(g, NewInstruction1(
              _6502_OP(sta),
              ByteConst(g,  _6502_MSRC_REG + 1),
              kAddrModeZeroPageAbsolute));

  // JSR zeromem
  jsr(g, zeromem);
  return NULL;
}

static TargetInstruction* LowerZeroExtend(_6502Generator* g, IRNode* node) {
  if (node->outputs.length == 0) {
    // No users of this, ignore.
    return NULL;
  }
  IRNode* src = node->inputs.value.p[0];
  TargetInstruction* value = Materialize(g, src);
  TargetInstruction* dest = GetDestAddress(g, node);
  value = SimpleIntegerOp(g, node, _6502_OP(and), node->type->size, dest, value,
                          GetLoweredNode(node->inputs.value.p[1]));
  SetLoweredNode(node, value);
  return value;
}

static TargetInstruction* LowerSignExtend(_6502Generator* g, IRNode* node) {
  if (node->outputs.length == 0) {
    // No users of this, ignore.
    return NULL;
  }
  IRNode* src_node = node->inputs.value.p[0];
  int src_size = src_node->type->size;
  TargetInstruction* src = GetAddress(g, src_node);

  TargetInstruction* dest = GetDestAddress(g, node);

  IRConstant* diff_value = node->inputs.value.p[1];
  int diff = (int)diff_value->value.ivalue;  // In bits.
  diff /= 8;                                 // In bytes.
  int dest_size = src_size - diff;
  if (diff < 0) {
    // Lengthen int.
    // Say from 2 to 4 bytes.
    //  lda src+0
    //  sta dest+0
    //  lda src+1
    //  sta dest+1
    //  ldx #0
    //  AND #0x80
    //  BEQ xx
    //  DEX
    // xx:
    //  TXA
    //  sta dest+2
    //  sta dest+3

    for (int i = 0; i < src_size; i++) {
      SetIndexReg(g, src, dest, i);
      lda(g, src, i);
      sta(g, dest, i);
    }
    Emit(g, NewInstruction1(_6502_OP(ldx),
                            Zero(g),
                            kAddrModeImmediate));
    Emit(g, NewInstruction1(_6502_OP(and),
                            ByteConst(g,  0x80),
                            kAddrModeImmediate));
    TargetInstruction* label =
        NewInstruction(_6502_OP(label), kAddrModeImplied);
    EmitResolvedBranch(g, _6502_OP(beq), label);
    Emit(g, NewInstruction(_6502_OP(dex), kAddrModeImplied));
    Emit(g, label);
    Emit(g, NewInstruction(_6502_OP(txa), kAddrModeImplied));

    for (int i = src_size; i < dest_size; i++) {
      SetIndexReg(g, dest, dest, i);
      sta(g, dest, i);
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
         NewInstruction2(_6502_OP(lda), src,
                         ByteConst(g, src_size - 1),
                         GetAddrMode(src)));
    Emit(g, NewInstruction1(_6502_OP(and),
                            ByteConst(g, 0x80),
                            kAddrModeImmediate));
    Emit(g, NewInstruction2(
                _6502_OP(ora), src,
                ByteConst(g,  dest_size - 1),
                GetAddrMode(src)));
    Emit(g, NewInstruction2(
                _6502_OP(sta), dest,
                ByteConst(g, dest_size - 1),
                GetAddrMode(dest)));
  }
  SetLoweredNode(node, dest);
  return dest;
}

static TargetInstruction* LowerAlign(_6502Generator* g, IRNode* node) {
  // TODO:
  return NULL;
}

static TargetInstruction* LowerAsm(_6502Generator* g, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
      Emit(g, TargetNewLiteral((int)id_node->value.ivalue));

  TargetInstruction* result =
      Emit(g, NewInstruction1(_6502_OP(asm), literal, kAddrModeImplied));

  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerLocation(_6502Generator* g, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  return SetLoweredNode(node, Emit(g, TargetNewLocation(loc)));
}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last function argument.  The ap variable is set to the
// address of the last argument + 2.
static TargetInstruction* LowerBuiltinVaStart(_6502Generator* g, IRNode* node) {
  TargetInstruction* ap = GetAddress(g, node->inputs.value.p[0]);
  TargetInstruction* arg = GetAddress(g, node->inputs.value.p[1]);
  
  AddSubInteger(g, node, _6502_OP(adc), _6502_OP(clc), 2, ap, arg, ByteConst(g, 2));
  
  return SetLoweredNode(node, ap);
}

static TargetInstruction* LowerBuiltinVaArg(_6502Generator* g, IRNode* node) {
  // Load value of first arg.
  TargetInstruction* ap_addr = GetAddress(g, node->inputs.value.p[0]);

  // Load the contents of ap into a tmp.
  TargetInstruction* ap = TempRegister(g, 2);

  for (int i = 0; i < 2; i++) {
    SetIndexReg(g, ap_addr, ap, i);
    lda(g, ap_addr, i);
    sta(g, ap, i);
  }
  
  // Now load the contents of the arg, via ap.
  TargetInstruction* dest = GetDestAddress(g, node);
  for (int i = 0; i < node->type->size; i++) {
    SetIndexReg(g, ap, dest, i);
    // Load the contents of (ap).
    AddressingMode mode = i == 0 ? kAddrModeIndirect : kAddrModeIndirectIndexed;
    Emit(g, NewInstruction2(_6502_OP(lda), ap, ByteConst(g, i), mode));
    sta(g, dest, i);
  }
  
  // Increment ap by size.
  int size = (int)IRIntConstValue(node->inputs.value.p[1]);
  AddSubInteger(g, node, _6502_OP(adc), _6502_OP(clc), 2, ap_addr, ap, ByteConst(g,  size));

  return  SetLoweredNode(node, dest);
;
}

static TargetInstruction* LowerBuiltinVaEnd(_6502Generator* g, IRNode* node) {
  // Nothing to do for va_end.
  return NULL;
}

static TargetInstruction* LowerBuiltinVaCopy(_6502Generator* g, IRNode* node) {
  return NULL;  // TODO
}

static void CompareEqualIntegerZeroExpression(_6502Generator* g, IRNode* node,
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

  TargetInstruction* value = GetAddress(g, node->inputs.value.p[0]);
  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
   }
  EmitResolvedBranch(g, _6502_OP(beq), true_label);
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

static void CompareEqualIntegerExpression(_6502Generator* g, IRNode* node,
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

  TargetInstruction* value1 = GetAddress(g, node->inputs.value.p[0]);
  TargetInstruction* value2 = GetAddress(g, node->inputs.value.p[1]);
  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    if (i < (size - 1)) {
      EmitResolvedBranch(g, _6502_OP(bne), false_label);
    } else {
      EmitResolvedBranch(g, _6502_OP(beq), true_label);
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
     
static void CompareNotEqualIntegerZeroExpression(_6502Generator* g, IRNode* node,
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

  TargetInstruction* value = GetAddress(g, node->inputs.value.p[0]);
  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value, value, i);
    if (i == 0) {
      lda(g, value, i);
    } else {
      ora(g, value, i);
    }
   }
  EmitResolvedBranch(g, _6502_OP(bne), true_label);
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

static void CompareNotEqualIntegerExpression(_6502Generator* g, IRNode* node,
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

  TargetInstruction* value1 = GetAddress(g, node->inputs.value.p[0]);
  TargetInstruction* value2 = GetAddress(g, node->inputs.value.p[1]);
  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  ldxi(g, 0);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    if (i < (size - 1)) {
      EmitResolvedBranch(g, _6502_OP(bne), true_label);
    } else {
      EmitResolvedBranch(g, _6502_OP(beq), false_label);
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

static void CompareLessUnsignedIntegerExpression(_6502Generator* g, IRNode* lhs,
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


  TargetInstruction* value1 = GetAddress(g, lhs);
  TargetInstruction* value2 = GetAddress(g, rhs);
  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  
  for (int i = size-1; i >= 0; i--) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    cmp(g, value2, i);
    EmitResolvedBranch(g, _6502_OP(bcc), true_label);
    if (i > 0) {
      EmitResolvedBranch(g, _6502_OP(bne), false_label);
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
static void CompareSignedIntegerExpression(_6502Generator* g, IRNode* lhs,
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


  TargetInstruction* value1 = GetAddress(g, lhs);
  TargetInstruction* value2 = GetAddress(g, rhs);
  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  TargetInstruction* skip_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  ldxi(g, 0);
  sec(g);
  
  for (int i = 0; i < size; i++) {
    SetIndexReg(g, value1, value2, i);
    lda(g, value1, i);
    if (i == 0) {
      cmp(g, value2, i);
    } else {
      sbc(g, value2, i);
    }
  }
  EmitResolvedBranch(g, _6502_OP(bvc), skip_label);
  eori(g, 0x80);
  Emit(g, skip_label);
  EmitResolvedBranch(g, less_than ? _6502_OP(bpl) : _6502_OP(bmi), false_label);
  inx(g);
  Emit(g, false_label);
  if (GetAddrMode(dest) == kAddrModeIndirectIndexed) {
    txa(g);
    sta(g, dest, 0);
  } else {
    stx(g, dest, 0);
  }
}


static void CompareGreaterOrEqualUnsignedIntegerExpression(_6502Generator* g,
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

  TargetInstruction* value1 = GetAddress(g, lhs);
  TargetInstruction* value2 = GetAddress(g, rhs);
  TargetInstruction* false_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  TargetInstruction* true_label =
      NewInstruction(_6502_OP(label), kAddrModeImplied);
  ldxi(g, 1);
  for (int i = size - 1; i >= 0; i--) {
    SetIndexRegDown(g, value1, value2, i, size);

    lda(g, value1, i);
    cmp(g, value2, i);
    if (i == 0) {
      EmitResolvedBranch(g, _6502_OP(bcs), true_label);
    } else {
      EmitResolvedBranch(g, _6502_OP(bcc), false_label);
      EmitResolvedBranch(g, _6502_OP(bne), true_label);
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

static void CompareFloatingExpression(_6502Generator* g, IRNode* lhs, IRNode* rhs,
                                      TargetInstruction* dest, int size,
                                      Symbol* func) {
  // lda #dest
  // ldx #src1
  // ldy #src2
  // jsr func
  // Put result (1 or 0) in dest byte.
  TargetInstruction* value1 = Materialize(g, lhs);
  TargetInstruction* value2 = Materialize(g, rhs);
  Emit(g, NewInstruction1(_6502_OP(expr_addr_a), dest, kAddrModeImplied));
  Emit(g, NewInstruction1(_6502_OP(expr_addr_x), value1, kAddrModeImplied));
  Emit(g, NewInstruction1(_6502_OP(expr_addr_y), value2, kAddrModeImplied));
  jsr(g, func);
}

static TargetInstruction* LowerComparison(_6502Generator* g, IRNode* node) {
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
    return NULL;
  }
  TargetInstruction* dest = GetDestAddress(g, node);
  // Size is the size of the inputs.  They will all be the same.
  IRNode* op1 = node->inputs.value.p[0];
  int size = op1->type->size;
  bool is_unsigned = TypeIsUnsigned(op1->type);
  
  IRNode* lhs = node->inputs.value.p[0];
  IRNode* rhs = node->inputs.value.p[1];
  switch (node->opcode) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
      if (IRIsZero(node->inputs.value.p[1])) {
        CompareEqualIntegerZeroExpression(g, node, dest, size);
      } else {
        CompareEqualIntegerExpression(g, node, dest, size);
      }
      break;
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
      if (IRIsZero(node->inputs.value.p[1])) {
        CompareNotEqualIntegerZeroExpression(g, node, dest, size);
      } else {
        CompareNotEqualIntegerExpression(g, node, dest, size);
      }
      break;
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      if (is_unsigned) {
        CompareLessUnsignedIntegerExpression(g, lhs, rhs, dest, size);
      } else {
        CompareSignedIntegerExpression(g, lhs, rhs, dest, size, true);
      }
      break;
    case IR_OP(cmplei):
    case IR_OP(cmplea):
      // Same as >= with args reversed.
      if (is_unsigned) {
        CompareGreaterOrEqualUnsignedIntegerExpression(g, lhs, rhs, dest, size);
      } else {
        CompareSignedIntegerExpression(g, rhs, lhs, dest, size, false);
      }
      break;
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      // Same as less with args reversed.
      if (is_unsigned) {
        CompareLessUnsignedIntegerExpression(g, rhs, lhs, dest, size);
      } else {
        CompareSignedIntegerExpression(g, rhs, lhs, dest, size, true);
      }
      break;
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
      if (is_unsigned) {
        CompareGreaterOrEqualUnsignedIntegerExpression(g, lhs, rhs, dest, size);
      } else {
        CompareSignedIntegerExpression(g, lhs, rhs, dest, size, false);
      }
      break;
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
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpeqd);
      break;
    case IR_OP(cmpned):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpned);
      break;
    case IR_OP(cmpltd):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpltd);
      break;
    case IR_OP(cmpled):
      CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpged);
      break;
    case IR_OP(cmpgtd):
      CompareFloatingExpression(g, rhs, lhs, dest, size, g->cmpltd);
      break;
    case IR_OP(cmpged):
      CompareFloatingExpression(g, lhs, rhs, dest, size, g->cmpged);
      break;
    default:
      abort();
  }
  // Mark result as having comparison generated.
  dest->flags |= k6502ComparisonGenerated;
  return SetLoweredNode(node, dest);
}

static TargetInstruction* LowerSSAVar(_6502Generator* g, IRNode* node) {
  IRVariable* var = (IRVariable*)node;
  IRNode* symbol = FindPooledVariable(g->gen, var->symbol);
  return SetLoweredNode(node, Emit(g, NewInstruction1(_6502_OP(ssavar), GetLoweredNode(symbol), kAddrModeImplied)));
}

static TargetInstruction* LowerPhiNode(_6502Generator* g, IRNode* node) {
  IRVariable* var = (IRVariable*)node;
  IRNode* symbol = FindPooledVariable(g->gen, var->symbol);
  return SetLoweredNode(node, Emit(g, NewInstruction1(_6502_OP(phi), GetLoweredNode(symbol), kAddrModeImplied)));
}

static void AddLiteral(_6502Generator* g, TargetConstant* con, const void* data,
                       size_t length) {
  con->literal_id = CompilerAddBufferLiteral(data, length);
}

static TargetInstruction* LowerIRNode(_6502Generator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
      break;

    case IR_OP(structreturn):
    // return Emit(g, NewInstruction(_6502_OP(structreturn)));

    case IR_OP(structarg):
      // Same as its input.
      return SetLoweredNode(node, GetAddress(g, node->inputs.value.p[0]));

    case IR_OP(nop):
    case last_ir_opcode:
      return NULL;

    case IR_OP(ssavar):
      return LowerSSAVar(g, node);
      
    case IR_OP(phi):
      return LowerPhiNode(g, node);
      break;

    case IR_OP(literalref):
      return LowerLiteralReference(g, node);

    case IR_OP(addressof):
      return LowerAddressOf(g, node);

    case IR_OP(consti): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeWord, c->value.ivalue);
      AddLiteral(g, (TargetConstant*)inst, &c->value.ivalue, 4);
      return inst;
    }
    case IR_OP(constb): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeByte, c->value.ivalue);
      return inst;
    }
    case IR_OP(consts): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeHalf, c->value.ivalue);
      return inst;
    }
    case IR_OP(consta): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeAddress, c->value.ivalue);
      return inst;
    }
    case IR_OP(constl): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeExtended, c->value.ivalue);
      AddLiteral(g, (TargetConstant*)inst, &c->value.ivalue, 8);
      return inst;
    }
    case IR_OP(constf): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeFloat, c->value.ivalue);
      AddLiteral(g, (TargetConstant*)inst, &c->value.fvalue, 4);
      return inst;
    }
    case IR_OP(constd): {
      IRConstant* c = (IRConstant*)node;
      TargetInstruction* inst =
          GetIntConstant(g, node, kTargetTypeDouble, c->value.ivalue);
      AddLiteral(g, (TargetConstant*)inst, &c->value.fvalue, 8);
      return inst;
    }
    case IR_OP(enter): {
      // Entry sequence:
      // stx __result (if not void)
      // sty __result+1 (if not void)
      // This macro instruction expands to:
      // (frame size is size of total frame)
      // ldx #frame_size lo
      // ldy #frame-size hi
      // JSR __enter
      //
      if (!TypeIsVoid(compiler->current_function->next)) {
        Emit(g, NewInstruction1(
                    _6502_OP(stx),
                    ByteConst(g,  _6502_RESULT_REG),
                    kAddrModeZeroPageAbsolute));
        Emit(g, NewInstruction1(
                    _6502_OP(sty),
                    ByteConst(g,  _6502_RESULT_REG+1),
                    kAddrModeZeroPageAbsolute));
      }
      if (IsLeaf(g)) {
        Emit(g, NewInstruction(_6502_OP(enter_leaf), kAddrModeImplied));
      } else {
        Emit(g, NewInstruction(_6502_OP(enter), kAddrModeImplied));
      }
      return NULL;
    }
    case IR_OP(leave): {
      // Exit sequence (non-leaf)
      // This macro instruction expands to:
      // ldx #frame_size lo
      // ldy #frame-size hi
      // JSR __leave
      if (IsLeaf(g)) {
        Emit(g, NewInstruction(_6502_OP(leave_leaf), kAddrModeImplied));
      } else {
        Emit(g, NewInstruction(_6502_OP(leave), kAddrModeImplied));
      }
      return NULL;
    }

    case IR_OP(ret):
      return Emit(g, NewInstruction(_6502_OP(rts), kAddrModeImplied));

    case IR_OP(loadi):
    case IR_OP(loadb):
    case IR_OP(loadl):
    case IR_OP(loads):
    case IR_OP(loadui):
    case IR_OP(loadub):
    case IR_OP(loadus):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      return LowerLoad(g, node);

    // stores.
    case IR_OP(storei):
    case IR_OP(storeb):
    case IR_OP(stores):
    case IR_OP(storel):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(g, node);

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
    case IR_OP(rmovi):
    case IR_OP(rmovf):
    case IR_OP(rmovd):
    case IR_OP(rmova):
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
      return SetLoweredNode(node, Materialize(g, node->inputs.value.p[0]));

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
  }
  // If we get here we've failed to handle the IR node.
  assert(false);
  return NULL;
}

// Calculate the size of an argument based on its type.
static int64_t CalculateArgumentSize(Symbol* arg) {
  if (TypeIsFloatingPoint(arg->type)) {
    if (TypeIsDouble(arg->type)) {
      return 8;
    }
    return 4;
  }
  if (TypeIsPointerOrArray(arg->type)) {
    return 2;
  }
  if (TypeIsStructOrUnion(arg->type)) {
    return arg->type->info.struct_info->size;
  }
  return arg->type->size;
}

void _6502Lower(_6502Generator* g, Generator* gen) {
  // Variables are allocated below the frame, arguments are above.
  int32_t var_offset = 0;
  int32_t arg_offset = 0;

  // Calculate the offset on the stack for all the arguments.  Store this offset
  // in the stack_offset field of the Symbol.
  Vector* prototype = &compiler->current_function->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* arg = prototype->value.p[i];
    arg->stack_offset = arg_offset;
    int64_t size = CalculateArgumentSize(arg);
    arg_offset += size;
  }

  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    TargetInstruction* inst;
    if (entry->pooled->opcode == IR_OP(localvar) ||
        entry->pooled->opcode == IR_OP(tempvar)) {
      int32_t size = entry->value.symbol->type->size;
      var_offset += size;
      IRVariable* var = (IRVariable*)entry->pooled;
      inst = NewTargetSymbol(var->symbol);
      inst->operand[0] =
          GetIntConstant(g, entry->pooled, kTargetTypeAddress, var_offset);
      inst->opcode = (TargetOpcode)_6502_OP(localvar);
      SetAddrMode(inst, kAddrModeImplied);
      inst = Emit(g, inst);
      entry->pooled->data.ptr = inst;
    } else if (entry->pooled->opcode == IR_OP(argument)) {
      IRVariable* var = (IRVariable*)entry->pooled;
      inst = NewTargetSymbol(var->symbol);
      inst->operand[0] = GetIntConstant(g, entry->pooled, kTargetTypeAddress,
                                        var->symbol->stack_offset);
      inst->opcode = (TargetOpcode)_6502_OP(argument);
      SetAddrMode(inst, kAddrModeImplied);
      inst = Emit(g, inst);
      entry->pooled->data.ptr = inst;
    } else if (entry->pooled->opcode == IR_OP(staticvar) ||
               entry->pooled->opcode == IR_OP(externvar)) {
      // Static variables are referenced by a symbol instruction.
      IRVariable* var = (IRVariable*)entry->pooled;
      TargetInstruction* inst = GetSymbol(g, NULL, var->symbol);
      SetAddrMode(inst, kAddrModeAbsoluteSymbol);
      entry->pooled->data.ptr = inst;
    }
  }

  // We now know the stack frame size.
  g->base.stack_frame_size = (int32_t)var_offset;

  // Run through IR pre-optimizing it for CISC.
  bool changed;
  do {
    changed = false;
    IRNode* node = GeneratorFirstInstruction(gen);
    while (node != NULL) {
      node = Preoptimize(g, gen, node, &changed);
    }
  } while (changed);

#if 0
  printf("Preoptimized IR\n");
  GeneratorPrintIR(gen, stdout);
  printf("===============\n");
#endif
  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(g, node);
    node = IRNext(node);
  }

  _6502CalculateInstructionAddresses(g);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    _6502Print(g, compiler->ir_output_file);
  }

  // Build basic blocks for.
  TargetBuildBasicBlocks(&g->base);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
  }
  // Pool all the variables.
  _6502PoolVariables(g);
  
   _6502SpillExpressions(g);
 
  // Process branches to check their ranges.
  _6502ProcessBranches(g);

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
     TargetPrintBasicBlocks(&g->base, compiler->ir_output_file);
   }

  _6502Optimize(g);

  // Allocate registers to the instructions.
  _6502AllocateRegisters(&g->register_allocator);
}

void _6502PrintInstruction(TargetInstruction* inst, FILE* fp) {
  TargetPrintInstruction(inst, _6502OpcodeName, fp);
}

void _6502Print(_6502Generator* g, FILE* fp) {
  TargetInstruction* inst = TargetFirstInstruction(&g->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, _6502OpcodeName, fp);
    inst = TargetNext(inst);
  }
}

bool _6502IsExpression(TargetInstruction* inst) {
  return (_6502Opcode)inst->opcode >= _6502_OP(expr1) &&
         (_6502Opcode)inst->opcode <= _6502_OP(expr8);
}

bool _6502IsSignedLoad(TargetInstruction* inst) {
  return false;
}
