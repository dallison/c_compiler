#include "bpf_codegen.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bpf_optimize.h"
#include "compiler.h"
#include "ir.h"
#include "symbol.h"
#include "target_basic_block.h"
#include "type.h"

static TargetInstruction* NewInstruction(BPFOpcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static TargetInstruction* NewInstruction1(BPFOpcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(BPFOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static TargetInstruction* NewInstruction3(BPFOpcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* Emit(BPFGenerator* bpf, TargetInstruction* inst) {
  return TargetEmit(&bpf->base, inst);
}

static TargetInstruction* EmitSymbol(BPFGenerator* bpf, TargetInstruction* c) {
  return TargetEmitSymbol(&bpf->base, c);
}

static TargetInstruction* FramePointer(BPFGenerator* bpf) {
  return TargetFramePointer(&bpf->base);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static TargetInstruction* SetLoweredNode(IRNode* node, TargetInstruction* inst) {
  return TargetSetLoweredNode(node, inst);
}

static TargetInstruction* GetIntConstant(BPFGenerator* bpf, IRNode* node,
                                         TargetType type, int64_t value) {
  return TargetGetIntConstant(&bpf->base, node, type, value);
}

static TargetInstruction* GetSymbol(BPFGenerator* bpf, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&bpf->base, node, symbol);
}

static TargetInstruction* IntArgumentRegister(BPFGenerator* bpf, int argnum) {
  if (argnum < 0 || argnum >= BPF_NUM_ARG_REGS) {
    argnum = 0;
  }
  if (bpf->int_argument_registers[argnum] == NULL) {
    bpf->int_argument_registers[argnum] =
        EmitSymbol(bpf, NewInstruction((BPFOpcode)(BPF_OP(r1) + argnum)));
  }
  return bpf->int_argument_registers[argnum];
}

static bool Is32BitType(TypeRecord* type) {
  if (type == NULL) {
    return false;
  }
  TypeRecordCalculateSize(type);
  return type->size > 0 && type->size <= 4 && !TypeIsPointer(type);
}

static BPFOpcode AluOp(IROpcode ir, bool is32) {
  switch (ir) {
    case IR_OP(addi):
    case IR_OP(adda):
      return is32 ? BPF_OP(add32) : BPF_OP(add);
    case IR_OP(subi):
    case IR_OP(suba):
      return is32 ? BPF_OP(sub32) : BPF_OP(sub);
    case IR_OP(muli):
      return is32 ? BPF_OP(mul32) : BPF_OP(mul);
    case IR_OP(divi):
      return is32 ? BPF_OP(div32) : BPF_OP(div);
    case IR_OP(modi):
      return is32 ? BPF_OP(mod32) : BPF_OP(mod);
    case IR_OP(ori):
      return is32 ? BPF_OP(or32) : BPF_OP(or);
    case IR_OP(andi):
      return is32 ? BPF_OP(and32) : BPF_OP(and);
    case IR_OP(xori):
      return is32 ? BPF_OP(xor32) : BPF_OP(xor);
    case IR_OP(lsli):
      return is32 ? BPF_OP(lsh32) : BPF_OP(lsh);
    case IR_OP(lsri):
      return is32 ? BPF_OP(rsh32) : BPF_OP(rsh);
    case IR_OP(asri):
      return is32 ? BPF_OP(arsh32) : BPF_OP(arsh);
    case IR_OP(negi):
      return is32 ? BPF_OP(neg32) : BPF_OP(neg);
    case IR_OP(movi):
    case IR_OP(mova):
    case IR_OP(tmp):
      return is32 ? BPF_OP(mov32) : BPF_OP(mov);
    default:
      return is32 ? BPF_OP(add32) : BPF_OP(add);
  }
}

const char* BPFOpcodeName(int op) {
  BPFOpcode opcode = (BPFOpcode)op;
  switch (opcode) {
    default:
      return TargetOpcodeName(opcode);
    case BPF_OP(mov32):
      return "mov32";
    case BPF_OP(add):
      return "add64";
    case BPF_OP(add32):
      return "add32";
    case BPF_OP(sub):
      return "sub64";
    case BPF_OP(sub32):
      return "sub32";
    case BPF_OP(mul):
      return "mul64";
    case BPF_OP(mul32):
      return "mul32";
    case BPF_OP(div):
      return "div64";
    case BPF_OP(div32):
      return "div32";
    case BPF_OP(mod):
      return "mod64";
    case BPF_OP(mod32):
      return "mod32";
    case BPF_OP(or):
      return "or64";
    case BPF_OP(and):
      return "and64";
    case BPF_OP(xor):
      return "xor64";
    case BPF_OP(lsh):
      return "lsh64";
    case BPF_OP(rsh):
      return "rsh64";
    case BPF_OP(arsh):
      return "arsh64";
    case BPF_OP(neg):
      return "neg64";
    case BPF_OP(li):
      return "li";
    case BPF_OP(lddw):
      return "lddw";
    case BPF_OP(ldxb):
      return "ldxb";
    case BPF_OP(ldxh):
      return "ldxh";
    case BPF_OP(ldxw):
      return "ldxw";
    case BPF_OP(ldxdw):
      return "ldxdw";
    case BPF_OP(stxb):
      return "stxb";
    case BPF_OP(stxh):
      return "stxh";
    case BPF_OP(stxw):
      return "stxw";
    case BPF_OP(stxdw):
      return "stxdw";
    case BPF_OP(ja):
      return "ja";
    case BPF_OP(jeq):
      return "jeq";
    case BPF_OP(jne):
      return "jne";
    case BPF_OP(jgt):
      return "jgt";
    case BPF_OP(jge):
      return "jge";
    case BPF_OP(jlt):
      return "jlt";
    case BPF_OP(jle):
      return "jle";
    case BPF_OP(jsgt):
      return "jsgt";
    case BPF_OP(jsge):
      return "jsge";
    case BPF_OP(jslt):
      return "jslt";
    case BPF_OP(jsle):
      return "jsle";
    case BPF_OP(call):
      return "call";
    case BPF_OP(exit):
      return "exit";
    case BPF_OP(r0):
      return "r0";
    case BPF_OP(r1):
      return "r1";
    case BPF_OP(r2):
      return "r2";
    case BPF_OP(r3):
      return "r3";
    case BPF_OP(r4):
      return "r4";
    case BPF_OP(r5):
      return "r5";
    case BPF_OP(nop):
      return "nop";
    case BPF_OP(spill):
      return "spill";
    case BPF_OP(reload):
      return "reload";
  }
}

bool BPFIsConst(TargetInstruction* inst) {
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(const8):
    case BPF_OP(const16):
    case BPF_OP(const32):
    case BPF_OP(const64):
    case BPF_OP(constf):
    case BPF_OP(constd):
      return true;
    default:
      return false;
  }
}

bool BPFIsSymbol(TargetInstruction* inst) {
  return (BPFOpcode)inst->opcode == BPF_OP(symbol);
}

bool BPFIsFixedRegister(TargetInstruction* inst) {
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(r0):
    case BPF_OP(r1):
    case BPF_OP(r2):
    case BPF_OP(r3):
    case BPF_OP(r4):
    case BPF_OP(r5):
    case BPF_OP(resulti):
    case BPF_OP(fp):
    case BPF_OP(sp):
    case BPF_OP(call):
      return true;
    default:
      return false;
  }
}

bool BPFIsLabel(TargetInstruction* inst) {
  BPFOpcode op = (BPFOpcode)inst->opcode;
  return op == BPF_OP(label) || op == BPF_OP(named_label);
}

bool BPFIsReturn(TargetInstruction* inst) {
  BPFOpcode op = (BPFOpcode)inst->opcode;
  return op == BPF_OP(ret) || op == BPF_OP(exit) || op == BPF_OP(restore);
}

bool BPFIsCall(TargetInstruction* inst) {
  return (BPFOpcode)inst->opcode == BPF_OP(call);
}

bool BPFIsSpill(TargetInstruction* inst) {
  BPFOpcode op = (BPFOpcode)inst->opcode;
  return op == BPF_OP(spill) || op == BPF_OP(reload);
}

bool BPFIsBranch(TargetInstruction* inst) {
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(ja):
    case BPF_OP(jeq):
    case BPF_OP(jne):
    case BPF_OP(jgt):
    case BPF_OP(jge):
    case BPF_OP(jlt):
    case BPF_OP(jle):
    case BPF_OP(jset):
    case BPF_OP(jsgt):
    case BPF_OP(jsge):
    case BPF_OP(jslt):
    case BPF_OP(jsle):
      return true;
    default:
      return false;
  }
}

bool BPFIsConditionalBranch(TargetInstruction* inst) {
  return BPFIsBranch(inst) && (BPFOpcode)inst->opcode != BPF_OP(ja);
}

bool BPFIsFloatingPoint(TargetInstruction* inst) {
  (void)inst;
  return false;
}

bool BPFIsExpression(TargetInstruction* inst) {
  switch ((BPFOpcode)inst->opcode) {
    case BPF_OP(label):
    case BPF_OP(named_label):
    case BPF_OP(asm):
    case BPF_OP(ja):
    case BPF_OP(jeq):
    case BPF_OP(jne):
    case BPF_OP(jgt):
    case BPF_OP(jge):
    case BPF_OP(jlt):
    case BPF_OP(jle):
    case BPF_OP(jset):
    case BPF_OP(jsgt):
    case BPF_OP(jsge):
    case BPF_OP(jslt):
    case BPF_OP(jsle):
    case BPF_OP(call):
    case BPF_OP(ret):
    case BPF_OP(exit):
    case BPF_OP(save):
    case BPF_OP(restore):
    case BPF_OP(stxb):
    case BPF_OP(stxh):
    case BPF_OP(stxw):
    case BPF_OP(stxdw):
    case BPF_OP(spill):
    case BPF_OP(loc):
    case BPF_OP(symbol):
    case BPF_OP(nop):
      return false;
    default:
      return !BPFIsFixedRegister(inst) && !BPFIsConst(inst);
  }
}

bool BPFGeneratesOutput(TargetInstruction* inst) {
  if (BPFIsFixedRegister(inst) || BPFIsSymbol(inst) || BPFIsConst(inst)) {
    return false;
  }
  return BPFIsExpression(inst);
}

TargetInstruction* BPFGetBranchTarget(TargetInstruction* inst) {
  if ((BPFOpcode)inst->opcode == BPF_OP(ja)) {
    return inst->operand[0];
  }
  if (inst->operand[2] != NULL) {
    return inst->operand[2];
  }
  return inst->operand[1];
}

static bool BPFIsJumpTableEntry(TargetInstruction* inst) {
  return (BPFOpcode)inst->opcode == BPF_OP(ja);
}

static TargetVirtuals virtuals = {
    .opcode_name = BPFOpcodeName,
    .is_branch = BPFIsBranch,
    .is_call = BPFIsCall,
    .is_return = BPFIsReturn,
    .is_spill = BPFIsSpill,
    .is_label = BPFIsLabel,
    .is_floating_point = BPFIsFloatingPoint,
    .is_conditional_branch = BPFIsConditionalBranch,
    .is_fixed_register = BPFIsFixedRegister,
    .is_const = BPFIsConst,
    .is_symbol = BPFIsSymbol,
    .is_expression = BPFIsExpression,
    .is_table_entry = BPFIsJumpTableEntry,
    .get_branch_target = BPFGetBranchTarget,
    .calls_may_stay_in_block = true,
};

void BPFGeneratorInit(BPFGenerator* bpf, Generator* gen) {
  TargetGeneratorInit(&bpf->base, gen, &virtuals);
  bpf->num_int_arg_regs = 0;
  bpf->struct_return_reg = -1;
  bpf->not_leaf = false;
  bpf->r0 = NULL;
  memset(bpf->int_argument_registers, 0, sizeof(bpf->int_argument_registers));
  VectorInit(&bpf->var_regs);
  VectorInit(&bpf->saved_args);
  BPFRegisterAllocatorInit(&bpf->register_allocator, bpf);
}

BPFGenerator* NewBPFGenerator(Generator* gen) {
  BPFGenerator* bpf = malloc(sizeof(BPFGenerator));
  BPFGeneratorInit(bpf, gen);
  return bpf;
}

void BPFGeneratorDestruct(BPFGenerator* bpf) {
  TargetGeneratorDestruct(&bpf->base);
  VectorDestructWithContents(&bpf->var_regs, NULL, true);
  VectorDestructWithContents(&bpf->saved_args, NULL, true);
  BPFRegisterAllocatorDestruct(&bpf->register_allocator);
}

void BPFGeneratorDelete(BPFGenerator* bpf) {
  BPFGeneratorDestruct(bpf);
  free(bpf);
}

static TargetInstruction* ResultReg(BPFGenerator* bpf) {
  if (bpf->r0 == NULL) {
    bpf->r0 = EmitSymbol(bpf, NewInstruction(BPF_OP(resulti)));
  }
  return bpf->r0;
}

static TargetInstruction* SetDestOrMove(BPFGenerator* bpf,
                                        TargetInstruction* from,
                                        TargetInstruction* to) {
  bool can_set = from->dest == NULL && BPFGeneratesOutput(from) &&
                 !BPFIsFixedRegister(from);
  if (can_set) {
    TargetSetDest(from, to);
    return from;
  }
  TargetInstruction* move = Emit(bpf, NewInstruction1(BPF_OP(mov), from));
  move->dest = to;
  return to;
}

static TargetInstruction* LowerIRNode(BPFGenerator* bpf, Generator* gen,
                                      IRNode* node);

static int LocalOffset(BPFGenerator* bpf, int32_t var_offset) {
  return -(bpf->base.stack_frame_size - var_offset);
}

static TargetInstruction* Materialize(BPFGenerator* bpf, IRNode* node) {
  if (IRIsConst(node)) {
    switch (node->opcode) {
      case IR_OP(const8):
        return Emit(bpf, NewInstruction1(
            BPF_OP(li), GetIntConstant(bpf, node, kTargetType8Bit,
                                       ((IRConstant*)node)->value.ivalue)));
      case IR_OP(const16):
        return Emit(bpf, NewInstruction1(
            BPF_OP(li), GetIntConstant(bpf, node, kTargetType16Bit,
                                       ((IRConstant*)node)->value.ivalue)));
      case IR_OP(const32):
      case IR_OP(consta):
        return Emit(bpf, NewInstruction1(
            BPF_OP(li), GetIntConstant(bpf, node, kTargetType32Bit,
                                       ((IRConstant*)node)->value.ivalue)));
      case IR_OP(const64):
        return Emit(bpf, NewInstruction1(
            BPF_OP(li), GetIntConstant(bpf, node, kTargetType64Bit,
                                       ((IRConstant*)node)->value.ivalue)));
      default:
        break;
    }
  }
  if (IRIsAutoVariable(node) || IRIsArgument(node)) {
    int32_t var_offset = node->data.ivalue;
    TargetInstruction* addr = FramePointer(bpf);
    int offset = LocalOffset(bpf, var_offset);
    return Emit(bpf, NewInstruction2(BPF_OP(add), addr,
                                     GetIntConstant(bpf, NULL, kTargetType32Bit,
                                                    offset)));
  }
  if (IRIsStaticVariable(node) || node->opcode == IR_OP(externvar) ||
      node->opcode == IR_OP(staticvar)) {
    return Emit(bpf, NewInstruction1(BPF_OP(lddw), GetLoweredNode(node)));
  }
  TargetInstruction* lowered = GetLoweredNode(node);
  if (lowered != NULL) {
    return lowered;
  }
  return LowerIRNode(bpf, NULL, node);
}

static TargetInstruction* FinishWithDest(BPFGenerator* bpf, IRNode* node,
                                         TargetInstruction* result) {
  if (node->dest != NULL) {
    if (node->dest->data.ptr == NULL && node->dest->opcode == IR_OP(tmp)) {
      TargetInstruction* tmp = Emit(bpf, NewInstruction(BPF_OP(tmp)));
      SetLoweredNode(node->dest, tmp);
    }
    TargetInstruction* dest = GetLoweredNode(node->dest);
    if (dest != NULL) {
      result = SetDestOrMove(bpf, result, dest);
    }
  }
  return SetLoweredNode(node, result);
}

static bool GetBaseAndOffset(BPFGenerator* bpf, IRNode* addr_node,
                             TargetInstruction** base,
                             TargetInstruction** offset) {
  if (IRIsAutoVariable(addr_node) || IRIsArgument(addr_node)) {
    *base = FramePointer(bpf);
    *offset = GetIntConstant(bpf, NULL, kTargetType32Bit,
                             LocalOffset(bpf, addr_node->data.ivalue));
    return true;
  }
  if (IRIsStaticVariable(addr_node) || addr_node->opcode == IR_OP(externvar) ||
      addr_node->opcode == IR_OP(staticvar)) {
    *base = Emit(bpf, NewInstruction1(BPF_OP(lddw), GetLoweredNode(addr_node)));
    *offset = GetIntConstant(bpf, NULL, kTargetType32Bit, 0);
    return true;
  }
  *base = Materialize(bpf, addr_node);
  *offset = GetIntConstant(bpf, NULL, kTargetType32Bit, 0);
  return true;
}

static BPFOpcode LoadOp(IROpcode op) {
  switch (op) {
    case IR_OP(load8):
    case IR_OP(loadu8):
      return BPF_OP(ldxb);
    case IR_OP(load16):
    case IR_OP(loadu16):
      return BPF_OP(ldxh);
    case IR_OP(load32):
    case IR_OP(loadu32):
      return BPF_OP(ldxw);
    default:
      return BPF_OP(ldxdw);
  }
}

static BPFOpcode StoreOp(IROpcode op) {
  switch (op) {
    case IR_OP(store8):
      return BPF_OP(stxb);
    case IR_OP(store16):
      return BPF_OP(stxh);
    case IR_OP(store32):
    case IR_OP(storef):
      return BPF_OP(stxw);
    default:
      return BPF_OP(stxdw);
  }
}

static TargetInstruction* SignExtend(BPFGenerator* bpf, TargetInstruction* v,
                                     int bits) {
  TargetInstruction* sh =
      GetIntConstant(bpf, NULL, kTargetType32Bit, 64 - bits);
  TargetInstruction* s1 = Emit(bpf, NewInstruction2(BPF_OP(lsh), v, sh));
  return Emit(bpf, NewInstruction2(BPF_OP(arsh), s1, sh));
}

static TargetInstruction* LowerLoad(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  TargetInstruction* load =
      Emit(bpf, NewInstruction2(LoadOp(node->opcode), base, offset));
  if (node->opcode == IR_OP(load8)) {
    load = SignExtend(bpf, load, 8);
  } else if (node->opcode == IR_OP(load16)) {
    load = SignExtend(bpf, load, 16);
  } else if (node->opcode == IR_OP(load32)) {
    load = SignExtend(bpf, load, 32);
  }
  return FinishWithDest(bpf, node, load);
}

static TargetInstruction* LowerStore(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  TargetInstruction* value = Materialize(bpf, node->inputs.value.p[1]);
  return Emit(bpf, NewInstruction3(StoreOp(node->opcode), base, offset, value));
}

static TargetInstruction* LowerUnary(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[0]);
  bool is32 = Is32BitType(node->type);
  if (node->opcode == IR_OP(onescomp) || node->opcode == IR_OP(noti) ||
      node->opcode == IR_OP(nota)) {
    TargetInstruction* ones =
        GetIntConstant(bpf, NULL, kTargetType64Bit, (int64_t)-1);
    BPFOpcode op = is32 ? BPF_OP(xor32) : BPF_OP(xor);
    if (node->opcode == IR_OP(noti) || node->opcode == IR_OP(nota)) {
      TargetInstruction* zero = GetIntConstant(bpf, NULL, kTargetType32Bit, 0);
      TargetInstruction* one = GetIntConstant(bpf, NULL, kTargetType32Bit, 1);
      TargetInstruction* eq =
          Emit(bpf, NewInstruction1(BPF_OP(li), one));
      TargetInstruction* jne =
          Emit(bpf, NewInstruction3(BPF_OP(jne), src, zero, NULL));
      TargetInstruction* skip = Emit(bpf, NewInstruction(BPF_OP(label)));
      jne->operand[2] = skip;
      TargetInstruction* z = Emit(bpf, NewInstruction1(BPF_OP(li), zero));
      z->dest = eq;
      Emit(bpf, skip);
      return FinishWithDest(bpf, node, eq);
    }
    return FinishWithDest(bpf, node, Emit(bpf, NewInstruction2(op, src, ones)));
  }
  if (node->opcode == IR_OP(negi)) {
    TargetInstruction* neg =
        Emit(bpf, NewInstruction1(AluOp(IR_OP(negi), is32), src));
    return FinishWithDest(bpf, node, neg);
  }
  return FinishWithDest(bpf, node, src);
}

static TargetInstruction* LowerBinop(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* lhs = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* rhs = Materialize(bpf, node->inputs.value.p[1]);
  bool is32 = Is32BitType(node->type);
  BPFOpcode op = AluOp(node->opcode, is32);
  return FinishWithDest(bpf, node, Emit(bpf, NewInstruction2(op, lhs, rhs)));
}

static BPFOpcode CmpBranchOp(IROpcode op, bool is_unsigned) {
  switch (op) {
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
      return BPF_OP(jeq);
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
      return BPF_OP(jne);
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      return is_unsigned ? BPF_OP(jlt) : BPF_OP(jslt);
    case IR_OP(cmplei):
    case IR_OP(cmplea):
      return is_unsigned ? BPF_OP(jle) : BPF_OP(jsle);
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      return is_unsigned ? BPF_OP(jgt) : BPF_OP(jsgt);
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
      return is_unsigned ? BPF_OP(jge) : BPF_OP(jsge);
    default:
      return BPF_OP(jeq);
  }
}

static bool CmpIsUnsigned(IRNode* node) {
  IRNode* a = node->inputs.value.p[0];
  if (node->opcode == IR_OP(cmplta) || node->opcode == IR_OP(cmplea) ||
      node->opcode == IR_OP(cmpgta) || node->opcode == IR_OP(cmpgea) ||
      node->opcode == IR_OP(cmpeqa) || node->opcode == IR_OP(cmpnea)) {
    return true;
  }
  return a->type != NULL && TypeIsUnsigned(a->type);
}

static TargetInstruction* LowerComparison(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* lhs = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* rhs = Materialize(bpf, node->inputs.value.p[1]);
  TargetInstruction* one =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 1)));
  TargetInstruction* br =
      Emit(bpf, NewInstruction3(CmpBranchOp(node->opcode, CmpIsUnsigned(node)),
                                lhs, rhs, NULL));
  TargetInstruction* zero =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  zero->dest = one;
  TargetInstruction* skip = Emit(bpf, NewInstruction(BPF_OP(label)));
  br->operand[2] = skip;
  return FinishWithDest(bpf, node, one);
}

static TargetInstruction* LowerThreeWay(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* a = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* b = Materialize(bpf, node->inputs.value.p[1]);
  bool is_unsigned = node->opcode == IR_OP(cmp3wayu) ||
                     node->opcode == IR_OP(cmp3waya);
  TargetInstruction* one =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 1)));
  TargetInstruction* gt =
      Emit(bpf, NewInstruction3(is_unsigned ? BPF_OP(jgt) : BPF_OP(jsgt), a, b,
                                NULL));
  TargetInstruction* lt_one =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, -1)));
  lt_one->dest = one;
  TargetInstruction* lt =
      Emit(bpf, NewInstruction3(is_unsigned ? BPF_OP(jlt) : BPF_OP(jslt), a, b,
                                NULL));
  TargetInstruction* z =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  z->dest = one;
  TargetInstruction* done = Emit(bpf, NewInstruction(BPF_OP(label)));
  gt->operand[2] = done;
  lt->operand[2] = done;
  return FinishWithDest(bpf, node, one);
}

static TargetInstruction* LowerConditionalBranch(BPFGenerator* bpf, IRNode* node) {
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];
  TargetInstruction* cond = Materialize(bpf, expr);
  BPFOpcode opc = node->opcode == IR_OP(btrue) ? BPF_OP(jne) : BPF_OP(jeq);
  TargetInstruction* zero = GetIntConstant(bpf, NULL, kTargetType32Bit, 0);
  TargetInstruction* inst = Emit(bpf, NewInstruction3(opc, cond, zero, NULL));
  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    VectorAppend(&bpf->base.fixups, NewBranchFixup(inst, target_node, 2));
  } else {
    inst->operand[2] = target;
  }
  return inst;
}

static TargetInstruction* LowerBranch(BPFGenerator* bpf, IRNode* node) {
  if (node->inputs.length == 0) {
    return NULL;
  }
  IRNode* target_node = node->inputs.value.p[0];
  TargetInstruction* inst = Emit(bpf, NewInstruction(BPF_OP(ja)));
  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    VectorAppend(&bpf->base.fixups, NewBranchFixup(inst, target_node, 0));
  } else {
    inst->operand[0] = target;
  }
  return inst;
}

static TargetInstruction* LowerLabel(BPFGenerator* bpf, IRNode* label) {
  TargetInstruction* inst = Emit(bpf, NewInstruction(BPF_OP(label)));
  label->data.ptr = inst;
  TargetApplyFixups(&bpf->base, label);
  return inst;
}

static TargetInstruction* LowerCall(BPFGenerator* bpf, IRNode* node) {
  bpf->not_leaf = true;
  IRNode* callee_node = node->inputs.value.p[0];
  TargetInstruction* addr =
      IRIsStaticVariable(callee_node)
          ? GetSymbol(bpf, NULL, ((IRVariable*)callee_node)->symbol)
          : Materialize(bpf, callee_node);
  int next_arg = 0;
  bool hidden = (node->flags & kIRStructReturnCall) != 0;
  for (size_t i = 1; i < node->inputs.length && next_arg < BPF_NUM_ARG_REGS;
       i++) {
    IRNode* arg_node = node->inputs.value.p[i];
    TargetInstruction* arg = Materialize(bpf, arg_node);
    TargetInstruction* dest = IntArgumentRegister(bpf, next_arg++);
    SetDestOrMove(bpf, arg, dest);
    (void)hidden;
  }
  TargetInstruction* call = Emit(bpf, NewInstruction1(BPF_OP(call), addr));
  // Calls return in r0, which is also the function-result register.  Copy
  // the value into a virtual immediately so later uses (a + call(), dest in
  // r0) cannot stomp it.
  TargetInstruction* value = Emit(bpf, NewInstruction1(BPF_OP(mov), call));
  return FinishWithDest(bpf, node, value);
}

static TargetInstruction* LowerResult(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* result = Materialize(bpf, node->inputs.value.p[0]);
  return SetLoweredNode(node, SetDestOrMove(bpf, result, ResultReg(bpf)));
}

static TargetInstruction* LowerZeroExtend(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[0]);
  int bits = 32;
  if (node->inputs.length > 1 && IRIsConst(node->inputs.value.p[1])) {
    bits = (int)((IRConstant*)node->inputs.value.p[1])->value.ivalue;
  } else if (node->type != NULL) {
    TypeRecordCalculateSize(node->type);
    bits = (int)(node->type->size * 8);
  }
  if (bits >= 64) {
    return FinishWithDest(bpf, node, src);
  }
  uint64_t mask = bits >= 64 ? (uint64_t)-1 : ((uint64_t)1 << bits) - 1;
  TargetInstruction* m =
      GetIntConstant(bpf, NULL, kTargetType64Bit, (int64_t)mask);
  return FinishWithDest(bpf, node, Emit(bpf, NewInstruction2(BPF_OP(and), src, m)));
}

static TargetInstruction* LowerSignExtendOp(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[0]);
  int bits = 32;
  if (node->inputs.length > 1 && IRIsConst(node->inputs.value.p[1])) {
    bits = (int)((IRConstant*)node->inputs.value.p[1])->value.ivalue;
  } else if (node->type != NULL) {
    TypeRecordCalculateSize(node->type);
    bits = (int)(node->type->size * 8);
  }
  return FinishWithDest(bpf, node, SignExtend(bpf, src, bits));
}

static TargetInstruction* LowerIncDec(BPFGenerator* bpf, IRNode* node, bool inc) {
  IRNode* addr_node = node->inputs.value.p[0];
  int64_t amount = 1;
  if (node->inputs.length > 1 && IRIsConst(node->inputs.value.p[1])) {
    amount = ((IRConstant*)node->inputs.value.p[1])->value.ivalue;
  }
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, addr_node, &base, &offset);
  BPFOpcode ld = BPF_OP(ldxdw);
  BPFOpcode st = BPF_OP(stxdw);
  BPFOpcode alu = BPF_OP(add);
  switch (node->opcode) {
    case IR_OP(inc8):
    case IR_OP(uinc8):
    case IR_OP(dec8):
    case IR_OP(udec8):
      ld = BPF_OP(ldxb);
      st = BPF_OP(stxb);
      alu = BPF_OP(add32);
      break;
    case IR_OP(inc16):
    case IR_OP(uinc16):
    case IR_OP(dec16):
    case IR_OP(udec16):
      ld = BPF_OP(ldxh);
      st = BPF_OP(stxh);
      alu = BPF_OP(add32);
      break;
    case IR_OP(inc32):
    case IR_OP(uinc32):
    case IR_OP(dec32):
    case IR_OP(udec32):
      ld = BPF_OP(ldxw);
      st = BPF_OP(stxw);
      alu = BPF_OP(add32);
      break;
    default:
      break;
  }
  TargetInstruction* val = Emit(bpf, NewInstruction2(ld, base, offset));
  int64_t delta = inc ? amount : -amount;
  TargetInstruction* add =
      Emit(bpf, NewInstruction2(alu, val,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, delta)));
  Emit(bpf, NewInstruction3(st, base, offset, add));
  return FinishWithDest(bpf, node, add);
}

static TargetInstruction* LowerMemcpy(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* dest = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[1]);
  TargetInstruction* n = Materialize(bpf, node->inputs.value.p[2]);
  TargetInstruction* i =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  TargetInstruction* loop = Emit(bpf, NewInstruction(BPF_OP(label)));
  TargetInstruction* cmp = Emit(bpf, NewInstruction3(BPF_OP(jge), i, n, NULL));
  TargetInstruction* sp = Emit(bpf, NewInstruction2(BPF_OP(add), src, i));
  TargetInstruction* dp = Emit(bpf, NewInstruction2(BPF_OP(add), dest, i));
  TargetInstruction* b =
      Emit(bpf, NewInstruction2(BPF_OP(ldxb), sp,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  Emit(bpf, NewInstruction3(BPF_OP(stxb), dp,
                            GetIntConstant(bpf, NULL, kTargetType32Bit, 0), b));
  Emit(bpf, NewInstruction2(BPF_OP(add), i,
                            GetIntConstant(bpf, NULL, kTargetType32Bit, 1)))
      ->dest = i;
  Emit(bpf, NewInstruction1(BPF_OP(ja), loop));
  TargetInstruction* done = Emit(bpf, NewInstruction(BPF_OP(label)));
  cmp->operand[2] = done;
  return dest;
}

static TargetInstruction* LowerMemzero(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* dest = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* n = Materialize(bpf, node->inputs.value.p[1]);
  TargetInstruction* i =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  TargetInstruction* loop = Emit(bpf, NewInstruction(BPF_OP(label)));
  TargetInstruction* cmp = Emit(bpf, NewInstruction3(BPF_OP(jge), i, n, NULL));
  TargetInstruction* dp = Emit(bpf, NewInstruction2(BPF_OP(add), dest, i));
  TargetInstruction* z =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  Emit(bpf, NewInstruction3(BPF_OP(stxb), dp,
                            GetIntConstant(bpf, NULL, kTargetType32Bit, 0), z));
  Emit(bpf, NewInstruction2(BPF_OP(add), i,
                            GetIntConstant(bpf, NULL, kTargetType32Bit, 1)))
      ->dest = i;
  Emit(bpf, NewInstruction1(BPF_OP(ja), loop));
  TargetInstruction* done = Emit(bpf, NewInstruction(BPF_OP(label)));
  cmp->operand[2] = done;
  return dest;
}

static void LowerVariables(BPFGenerator* bpf, Generator* gen) {
  int32_t var_offset = 0;
  bpf->num_int_arg_regs = 0;
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = gen->variable_pool.value.p[i];
    IRNode* pooled = entry->pooled;
    switch (pooled->opcode) {
      case IR_OP(localvar):
      case IR_OP(tempvar):
      case IR_OP(argument): {
        TypeRecord* type = pooled->type;
        if (pooled->opcode == IR_OP(argument) &&
            ((IRVariable*)pooled)->symbol != NULL) {
          type = ((IRVariable*)pooled)->symbol->type;
        }
        TypeRecordCalculateSize(type);
        int64_t size = type->size;
        if (size < 1) {
          size = 8;
        }
        int align = TypeRecordAlignment(type);
        if (align < 1) {
          align = 8;
        }
        if (align < 1) {
          align = 1;
        }
        var_offset = (var_offset + align - 1) & ~(align - 1);
        pooled->data.ivalue = var_offset;
        if (pooled->opcode == IR_OP(argument) &&
            bpf->num_int_arg_regs < BPF_NUM_ARG_REGS) {
          BPFSavedArgument* saved = malloc(sizeof(BPFSavedArgument));
          saved->reg_num = BPF_REG_1 + bpf->num_int_arg_regs;
          saved->offset = var_offset;
          saved->size = size > 4 ? 8 : (int)size;
          VectorAppend(&bpf->saved_args, saved);
          bpf->num_int_arg_regs++;
        }
        var_offset += (int32_t)size;
        break;
      }
      default: {
        IRVariable* var = (IRVariable*)pooled;
        TargetInstruction* inst = GetSymbol(bpf, NULL, var->symbol);
        pooled->data.ptr = inst;
        break;
      }
    }
  }
  bpf->base.stack_frame_size =
      ((var_offset + 7) & ~7) + BPF_NUM_SAVED_REGS * 8;
}

static TargetInstruction* LowerIRNode(BPFGenerator* bpf, Generator* gen,
                                      IRNode* node) {
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
    case IR_OP(rotli):
    case IR_OP(rotri):
    case IR_OP(clzi):
    case IR_OP(ctzi):
    case IR_OP(popcounti):
    case IR_OP(vadd):
    case IR_OP(vsub):
    case IR_OP(vmul):
    case IR_OP(vdiv):
    case IR_OP(vmod):
    case IR_OP(vlsl):
    case IR_OP(vlsr):
    case IR_OP(vasr):
    case IR_OP(vand):
    case IR_OP(vor):
    case IR_OP(vxor):
    case IR_OP(vneg):
    case IR_OP(vonescomp):
    case IR_OP(vcmpeq):
    case IR_OP(vcmpne):
    case IR_OP(vcmplt):
    case IR_OP(vcmple):
    case IR_OP(vcmpgt):
    case IR_OP(vcmpge):
    case IR_OP(vcmpltu):
    case IR_OP(vcmpleu):
    case IR_OP(vcmpgtu):
    case IR_OP(vcmpgeu):
    case IR_OP(vectorarg):
    case IR_OP(resultv):
    case IR_OP(capturev):
      fprintf(stderr, "eBPF backend: unsupported IR opcode %s\n",
              IROpcodeName(node->opcode));
      return Emit(bpf, NewInstruction(BPF_OP(nop)));

    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
      return NULL;

    case IR_OP(observable_checkpoint):
    case IR_OP(nop):
    case last_ir_opcode:
      return NULL;

    case IR_OP(ssavar):
    case IR_OP(phi):
      break;

    case IR_OP(structreturn): {
      TargetInstruction* result =
          SetLoweredNode(node, EmitSymbol(bpf, NewInstruction(BPF_OP(structreturn))));
      TargetInstruction* mv =
          Emit(bpf, NewInstruction1(BPF_OP(mov), IntArgumentRegister(bpf, 0)));
      mv->dest = result;
      return result;
    }

    case IR_OP(literalref):
      return FinishWithDest(
          bpf, node,
          Emit(bpf, NewInstruction1(BPF_OP(lddw),
                                    TargetNewLiteral((int)((IRConstant*)node)
                                                         ->value.ivalue))));

    case IR_OP(addressof):
      return FinishWithDest(bpf, node, Materialize(bpf, node->inputs.value.p[0]));

    case IR_OP(const32):
    case IR_OP(consta):
      return GetIntConstant(bpf, node, kTargetType32Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(const8):
      return GetIntConstant(bpf, node, kTargetType8Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(const16):
      return GetIntConstant(bpf, node, kTargetType16Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(const64):
      return GetIntConstant(bpf, node, kTargetType64Bit,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(constf):
    case IR_OP(constd):
      return GetIntConstant(bpf, node, kTargetType64Bit, 0);

    case IR_OP(enter):
      Emit(bpf, NewInstruction(BPF_OP(save)));
      LowerVariables(bpf, gen);
      return NULL;

    case IR_OP(leave):
      Emit(bpf, NewInstruction(BPF_OP(restore)));
      return NULL;

    case IR_OP(ret):
      return Emit(bpf, NewInstruction(BPF_OP(exit)));

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
      return LowerLoad(bpf, node);

    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(bpf, node);

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
      return LowerIncDec(bpf, node, true);
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
      return LowerIncDec(bpf, node, false);

    case IR_OP(getbit):
    case IR_OP(setbit):
      return Emit(bpf, NewInstruction(BPF_OP(nop)));

    case IR_OP(addi):
    case IR_OP(adda):
    case IR_OP(subi):
    case IR_OP(suba):
    case IR_OP(muli):
    case IR_OP(divi):
    case IR_OP(modi):
    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(lsli):
    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):
      return LowerBinop(bpf, node);

    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(subf):
    case IR_OP(subd):
    case IR_OP(mulf):
    case IR_OP(muld):
    case IR_OP(divf):
    case IR_OP(divd):
    case IR_OP(negf):
    case IR_OP(negd):
    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):
    case IR_OP(movf):
    case IR_OP(movd):
      fprintf(stderr, "eBPF backend: floating point is not supported\n");
      return Emit(bpf, NewInstruction(BPF_OP(nop)));

    case IR_OP(noti):
    case IR_OP(nota):
    case IR_OP(onescomp):
    case IR_OP(negi):
      return LowerUnary(bpf, node);

    case IR_OP(movi):
    case IR_OP(mova):
    case IR_OP(tmp):
      if (node->inputs.length == 0) {
        return FinishWithDest(bpf, node, Emit(bpf, NewInstruction(BPF_OP(tmp))));
      }
      return FinishWithDest(bpf, node, Materialize(bpf, node->inputs.value.p[0]));

    case IR_OP(nrvoval):
      return Emit(bpf, NewInstruction1(BPF_OP(mov),
                                       Materialize(bpf, node->inputs.value.p[0])));

    case IR_OP(cmpeqi):
    case IR_OP(cmpnei):
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):
    case IR_OP(cmpeqa):
    case IR_OP(cmpnea):
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):
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
      return LowerComparison(bpf, node);

    case IR_OP(cmp3wayi):
    case IR_OP(cmp3wayu):
    case IR_OP(cmp3waya):
    case IR_OP(cmp3wayf):
    case IR_OP(cmp3wayd):
      return LowerThreeWay(bpf, node);

    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(bpf, node);
    case IR_OP(bra):
      return LowerBranch(bpf, node);
    case IR_OP(cbra):
      return LowerBranch(bpf, node);

    case IR_OP(label):
      return LowerLabel(bpf, node);
    case IR_OP(named_label):
      return SetLoweredNode(node, Emit(bpf, TargetNewNamedLabel(
                                                ((IRNamedLabel*)node)->name)));

    case IR_OP(pusharg):
      return SetLoweredNode(node, Materialize(bpf, node->inputs.value.p[0]));
    case IR_OP(calla):
      return LowerCall(bpf, node);
    case IR_OP(structarg):
      return SetLoweredNode(node, Materialize(bpf, node->inputs.value.p[0]));
    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
      return LowerResult(bpf, node);
    case IR_OP(memzero):
      return LowerMemzero(bpf, node);
    case IR_OP(memcpy):
      return LowerMemcpy(bpf, node);
    case IR_OP(cast):
      return FinishWithDest(bpf, node, Materialize(bpf, node->inputs.value.p[0]));
    case IR_OP(zeroextendi):
      return LowerZeroExtend(bpf, node);
    case IR_OP(signextendi):
      return LowerSignExtendOp(bpf, node);
    case IR_OP(aligni): {
      TargetInstruction* v = Materialize(bpf, node->inputs.value.p[0]);
      TargetInstruction* a = Materialize(bpf, node->inputs.value.p[1]);
      TargetInstruction* one = GetIntConstant(bpf, NULL, kTargetType32Bit, 1);
      TargetInstruction* am1 = Emit(bpf, NewInstruction2(BPF_OP(sub), a, one));
      TargetInstruction* sum = Emit(bpf, NewInstruction2(BPF_OP(add), v, am1));
      TargetInstruction* mask = Emit(bpf, NewInstruction1(BPF_OP(neg), a));
      return FinishWithDest(bpf, node,
                            Emit(bpf, NewInstruction2(BPF_OP(and), sum, mask)));
    }
    case IR_OP(asm):
      return Emit(bpf, NewInstruction(BPF_OP(nop)));
    case IR_OP(loc):
      return SetLoweredNode(node, Emit(bpf, TargetNewLocation((IRLocation*)node)));
    case IR_OP(builtin_va_start):
    case IR_OP(builtin_va_arg):
    case IR_OP(builtin_va_end):
    case IR_OP(builtin_va_copy):
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
    case IR_OP(decsp):
    case IR_OP(savesp):
    case IR_OP(restoresp):
      return Emit(bpf, NewInstruction(BPF_OP(nop)));
    default:
      break;
  }
  fprintf(stderr, "eBPF backend: unhandled IR opcode %s\n",
          IROpcodeName(node->opcode));
  assert(false);
  return NULL;
}

void BPFLower(BPFGenerator* bpf, Generator* gen) {
  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(bpf, gen, node);
    node = IRNext(node);
  }
  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    BPFPrint(bpf, compiler->ir_output_file);
  }
  TargetBuildBasicBlocks(&bpf->base);
  if (OptLevel2()) {
    BPFOptimize(bpf);
  }
  BPFAllocateRegisters(&bpf->register_allocator);
}

void BPFPrint(BPFGenerator* bpf, FILE* fp) {
  TargetInstruction* inst = TargetFirstInstruction(&bpf->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, BPFOpcodeName, fp);
    inst = TargetNext(inst);
  }
}
