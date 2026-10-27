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
  (void)inst;
  return false;
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
  bpf->named_stack_args = 0;
  bpf->dynamic_sp_offset = -1;
  bpf->struct_return_reg = -1;
  bpf->vararg_save_offset = -1;
  bpf->outgoing_offset = 0;
  bpf->nrvo_pointer = NULL;
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
static TargetInstruction* Materialize(BPFGenerator* bpf, IRNode* node);
static TargetInstruction* FinishWithDest(BPFGenerator* bpf, IRNode* node,
                                         TargetInstruction* value);
static TargetInstruction* SignExtend(BPFGenerator* bpf, TargetInstruction* v,
                                     int bits);
static bool GetBaseAndOffset(BPFGenerator* bpf, IRNode* addr_node,
                             TargetInstruction** base,
                             TargetInstruction** offset);

// One guest thread.  Atomics are ordinary loads and stores of the same
// width; the sequence itself is the ordering.
static int AtomicBits(IRNode* node) {
  IRNode* sized = node;
  if (node->opcode == IR_OP(atomic_store) && node->inputs.length > 1) {
    sized = node->inputs.value.p[1];
  } else if ((node->opcode == IR_OP(atomic_compare_exchange_bool) ||
              node->opcode == IR_OP(atomic_compare_exchange_val) ||
              node->opcode == IR_OP(atomic_compare_exchange_n)) &&
             node->inputs.length > 2) {
    sized = node->inputs.value.p[2];
  }
  int bits = sized->type != NULL && sized->type->size > 0 ? sized->type->size * 8
                                                           : 32;
  if (bits <= 8) return 8;
  if (bits <= 16) return 16;
  if (bits <= 32) return 32;
  return 64;
}

static void AtomicAccessOps(int bits, BPFOpcode* load, BPFOpcode* store,
                            BPFOpcode* add, BPFOpcode* sub) {
  if (bits <= 8) {
    *load = BPF_OP(ldxb);
    *store = BPF_OP(stxb);
    *add = BPF_OP(add32);
    *sub = BPF_OP(sub32);
  } else if (bits <= 16) {
    *load = BPF_OP(ldxh);
    *store = BPF_OP(stxh);
    *add = BPF_OP(add32);
    *sub = BPF_OP(sub32);
  } else if (bits <= 32) {
    *load = BPF_OP(ldxw);
    *store = BPF_OP(stxw);
    *add = BPF_OP(add32);
    *sub = BPF_OP(sub32);
  } else {
    *load = BPF_OP(ldxdw);
    *store = BPF_OP(stxdw);
    *add = BPF_OP(add);
    *sub = BPF_OP(sub);
  }
}

static TargetInstruction* MaskAtomic(BPFGenerator* bpf, TargetInstruction* value,
                                     int bits) {
  if (bits >= 64) {
    return value;
  }
  uint64_t mask = ((uint64_t)1 << bits) - 1;
  return Emit(bpf, NewInstruction2(BPF_OP(and), value,
                                   GetIntConstant(bpf, NULL, kTargetType64Bit,
                                                  (int64_t)mask)));
}

static TargetInstruction* LowerAtomicLoad(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  BPFOpcode load, store, add, sub;
  int bits = AtomicBits(node);
  AtomicAccessOps(bits, &load, &store, &add, &sub);
  (void)store;
  (void)add;
  (void)sub;
  TargetInstruction* value = Emit(bpf, NewInstruction2(load, base, offset));
  if (bits < 64 && node->type != NULL && !TypeIsUnsigned(node->type)) {
    value = SignExtend(bpf, value, bits);
  }
  return FinishWithDest(bpf, node, value);
}

static TargetInstruction* LowerAtomicStore(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  TargetInstruction* value = Materialize(bpf, node->inputs.value.p[1]);
  BPFOpcode load, store, add, sub;
  AtomicAccessOps(AtomicBits(node), &load, &store, &add, &sub);
  (void)load;
  (void)add;
  (void)sub;
  return SetLoweredNode(node, Emit(bpf, NewInstruction3(store, base, offset, value)));
}

static TargetInstruction* LowerAtomicRmw(BPFGenerator* bpf, IRNode* node,
                                         bool add, bool return_new) {
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  TargetInstruction* delta = Materialize(bpf, node->inputs.value.p[1]);
  BPFOpcode load, store, add_op, sub_op;
  int bits = AtomicBits(node);
  AtomicAccessOps(bits, &load, &store, &add_op, &sub_op);
  TargetInstruction* old = Emit(bpf, NewInstruction2(load, base, offset));
  TargetInstruction* updated =
      Emit(bpf, NewInstruction2(add ? add_op : sub_op, old, delta));
  Emit(bpf, NewInstruction3(store, base, offset, updated));
  TargetInstruction* result = return_new ? updated : old;
  if (bits < 64 && node->type != NULL && !TypeIsUnsigned(node->type)) {
    result = SignExtend(bpf, result, bits);
  }
  return FinishWithDest(bpf, node, result);
}

static TargetInstruction* LowerAtomicCompareExchange(BPFGenerator* bpf,
                                                     IRNode* node) {
  bool expected_is_pointer =
      node->opcode == IR_OP(atomic_compare_exchange_n);
  bool returns_bool = node->opcode != IR_OP(atomic_compare_exchange_val);
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  BPFOpcode load, store, add, sub;
  int bits = AtomicBits(node);
  AtomicAccessOps(bits, &load, &store, &add, &sub);
  (void)add;
  (void)sub;
  TargetInstruction* old = Emit(bpf, NewInstruction2(load, base, offset));
  TargetInstruction* expected;
  TargetInstruction* expected_base = NULL;
  TargetInstruction* expected_offset = NULL;
  if (expected_is_pointer) {
    GetBaseAndOffset(bpf, node->inputs.value.p[1], &expected_base,
                     &expected_offset);
    expected = Emit(bpf, NewInstruction2(load, expected_base, expected_offset));
  } else {
    expected = Materialize(bpf, node->inputs.value.p[1]);
  }
  TargetInstruction* desired = Materialize(bpf, node->inputs.value.p[2]);
  TargetInstruction* mismatch = NewInstruction(BPF_OP(label));
  TargetInstruction* done = NewInstruction(BPF_OP(label));
  TargetInstruction* branch = Emit(
      bpf, NewInstruction3(BPF_OP(jne), MaskAtomic(bpf, old, bits),
                           MaskAtomic(bpf, expected, bits), mismatch));
  (void)branch;
  Emit(bpf, NewInstruction3(store, base, offset, desired));
  TargetInstruction* result = old;
  if (returns_bool) {
    result = Emit(bpf, NewInstruction1(BPF_OP(li),
                                       GetIntConstant(bpf, NULL, kTargetType32Bit,
                                                      1)));
  }
  TargetInstruction* skip = Emit(bpf, NewInstruction(BPF_OP(ja)));
  skip->operand[0] = done;
  Emit(bpf, mismatch);
  if (expected_is_pointer) {
    Emit(bpf, NewInstruction3(store, expected_base, expected_offset, old));
  }
  if (returns_bool) {
    TargetInstruction* zero = Emit(
        bpf, NewInstruction1(BPF_OP(li),
                             GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
    zero->dest = result;
  }
  Emit(bpf, done);
  return FinishWithDest(bpf, node, result);
}

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
      case IR_OP(constf): {
        float f = (float)((IRConstant*)node)->value.fvalue;
        uint32_t bits = 0;
        memcpy(&bits, &f, sizeof(bits));
        return Emit(bpf, NewInstruction1(
            BPF_OP(li), GetIntConstant(bpf, node, kTargetType64Bit, (int64_t)bits)));
      }
      case IR_OP(constd): {
        double d = ((IRConstant*)node)->value.fvalue;
        uint64_t bits = 0;
        memcpy(&bits, &d, sizeof(bits));
        return Emit(bpf, NewInstruction1(
            BPF_OP(li), GetIntConstant(bpf, node, kTargetType64Bit, (int64_t)bits)));
      }
      default:
        break;
    }
  }
  if ((node->flags & kIRNrvoMarker) != 0 && bpf->nrvo_pointer != NULL) {
    return bpf->nrvo_pointer;
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
  if ((addr_node->flags & kIRNrvoMarker) != 0 && bpf->nrvo_pointer != NULL) {
    *base = bpf->nrvo_pointer;
    *offset = GetIntConstant(bpf, NULL, kTargetType32Bit, 0);
    return true;
  }
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
    case IR_OP(loadf):
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
      // Result is 1 only when the input is zero.  Branching the other way
      // turns `!` into a booleanize and inverts every early-out.
      TargetInstruction* eq =
          Emit(bpf, NewInstruction1(BPF_OP(li), one));
      TargetInstruction* jeq =
          Emit(bpf, NewInstruction3(BPF_OP(jeq), src, zero, NULL));
      TargetInstruction* z = Emit(bpf, NewInstruction1(BPF_OP(li), zero));
      z->dest = eq;
      TargetInstruction* skip = Emit(bpf, NewInstruction(BPF_OP(label)));
      jeq->operand[2] = skip;
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

// A 32-bit ALU result is zero-extended to 64 by the interpreter.  A signed
// value used by a 64-bit operation, such as a negative pointer index, has to
// be sign-extended first or `p[-1]` adds 4GB.  An address is already 64 bits.
// The node type of a local or an adda is the object's type, so a 1-byte
// struct would otherwise be sign-extended down to its low 8 bits.
static bool HoldsAddress(IRNode* src) {
  if (src == NULL) {
    return false;
  }
  if (src->type != NULL &&
      (TypeIsPointer(src->type) || TypeIsStructOrUnion(src->type) ||
       TypeIsArray(src->type))) {
    return true;
  }
  return IRIsAutoVariable(src) || IRIsArgument(src) || IRIsStaticVariable(src) ||
         src->opcode == IR_OP(externvar) || src->opcode == IR_OP(adda) ||
         src->opcode == IR_OP(suba) || src->opcode == IR_OP(addressof) ||
         src->opcode == IR_OP(literalref) || src->opcode == IR_OP(structreturn);
}

static TargetInstruction* WidenFor64(BPFGenerator* bpf, TargetInstruction* v,
                                    IRNode* src) {
  if (v == NULL || TargetIsConst(v) || BPFIsSymbol(v) || BPFIsLabel(v)) {
    return v;
  }
  if (src == NULL || src->type == NULL || HoldsAddress(src) ||
      !TypeIsIntegral(src->type)) {
    return v;
  }
  TypeRecordCalculateSize(src->type);
  int size = src->type->size;
  if (size >= 8 || size <= 0) {
    return v;
  }
  if (TypeIsUnsigned(src->type)) {
    if (size >= 4) {
      return v;
    }
    uint64_t mask = ((uint64_t)1 << (size * 8)) - 1;
    TargetInstruction* m =
        GetIntConstant(bpf, NULL, kTargetType64Bit, (int64_t)mask);
    return Emit(bpf, NewInstruction2(BPF_OP(and), v, m));
  }
  return SignExtend(bpf, v, size * 8);
}

static TargetInstruction* LowerBinop(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* lhs = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* rhs = Materialize(bpf, node->inputs.value.p[1]);
  bool is32 = Is32BitType(node->type);
  if (!is32) {
    lhs = WidenFor64(bpf, lhs, node->inputs.value.p[0]);
    rhs = WidenFor64(bpf, rhs, node->inputs.value.p[1]);
  }
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

static Symbol* HelperSymbol(const char* name) {
  static const char* names[24];
  static Symbol* syms[24];
  static int count = 0;
  for (int i = 0; i < count; i++) {
    if (strcmp(names[i], name) == 0) {
      return syms[i];
    }
  }
  Symbol* sym = NewSymbol(name, NULL, kStorage_extern);
  names[count] = name;
  syms[count] = sym;
  count++;
  return sym;
}

static TargetInstruction* EmitHelperCall(BPFGenerator* bpf, const char* name,
                                        TargetInstruction** args, int nargs) {
  for (int i = 0; i < nargs; i++) {
    TargetInstruction* dest = IntArgumentRegister(bpf, i);
    TargetInstruction* move = Emit(bpf, NewInstruction1(BPF_OP(mov), args[i]));
    move->dest = dest;
  }
  TargetInstruction* call = Emit(
      bpf, NewInstruction1(BPF_OP(call), GetSymbol(bpf, NULL, HelperSymbol(name))));
  return Emit(bpf, NewInstruction1(BPF_OP(mov), call));
}

static int FloatOpIs64(IROpcode op) {
  switch (op) {
    case IR_OP(addd):
    case IR_OP(subd):
    case IR_OP(muld):
    case IR_OP(divd):
    case IR_OP(negd):
    case IR_OP(cmpeqd):
    case IR_OP(cmpned):
    case IR_OP(cmpltd):
    case IR_OP(cmpled):
    case IR_OP(cmpgtd):
    case IR_OP(cmpged):
    case IR_OP(cmp3wayd):
    case IR_OP(i2d):
    case IR_OP(d2i):
    case IR_OP(d2f):
    case IR_OP(movd):
    case IR_OP(incd):
    case IR_OP(decd):
      return 1;
    default:
      return 0;
  }
}

static TargetInstruction* LowerFloatBinop(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* args[2];
  args[0] = Materialize(bpf, node->inputs.value.p[0]);
  args[1] = Materialize(bpf, node->inputs.value.p[1]);
  const char* name = "__bpf_addsf3";
  switch (node->opcode) {
    case IR_OP(subf):
      name = "__bpf_subsf3";
      break;
    case IR_OP(mulf):
      name = "__bpf_mulsf3";
      break;
    case IR_OP(divf):
      name = "__bpf_divsf3";
      break;
    case IR_OP(addd):
      name = "__bpf_adddf3";
      break;
    case IR_OP(subd):
      name = "__bpf_subdf3";
      break;
    case IR_OP(muld):
      name = "__bpf_muldf3";
      break;
    case IR_OP(divd):
      name = "__bpf_divdf3";
      break;
    default:
      break;
  }
  return FinishWithDest(bpf, node, EmitHelperCall(bpf, name, args, 2));
}

static TargetInstruction* LowerFloatNeg(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[0]);
  int64_t bit = node->opcode == IR_OP(negd) ? (int64_t)((uint64_t)1 << 63)
                                            : (int64_t)0x80000000;
  TargetInstruction* mask = Emit(
      bpf, NewInstruction1(BPF_OP(li),
                           GetIntConstant(bpf, NULL, kTargetType64Bit, bit)));
  return FinishWithDest(
      bpf, node, Emit(bpf, NewInstruction2(BPF_OP(xor), src, mask)));
}

static TargetInstruction* LowerFloatConvert(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[0]);
  const char* name = NULL;
  IRNode* in = node->inputs.value.p[0];
  switch (node->opcode) {
    case IR_OP(i2f):
    case IR_OP(i2d): {
      int to_double = node->opcode == IR_OP(i2d);
      int is_unsigned = in->type != NULL && TypeIsUnsigned(in->type);
      src = WidenFor64(bpf, src, in);
      if (is_unsigned) {
        name = to_double ? "__bpf_u64_to_df" : "__bpf_u64_to_sf";
      } else {
        name = to_double ? "__bpf_i64_to_df" : "__bpf_i64_to_sf";
      }
      break;
    }
    case IR_OP(f2i):
      name = "__bpf_sf_to_i64";
      break;
    case IR_OP(d2i):
      name = "__bpf_df_to_i64";
      break;
    case IR_OP(f2d):
      name = "__bpf_sf_to_df";
      break;
    case IR_OP(d2f):
      name = "__bpf_df_to_sf";
      break;
    default:
      break;
  }
  TargetInstruction* args[1] = {src};
  return FinishWithDest(bpf, node, EmitHelperCall(bpf, name, args, 1));
}

static TargetInstruction* FloatCompareValue(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* args[2];
  args[0] = Materialize(bpf, node->inputs.value.p[0]);
  args[1] = Materialize(bpf, node->inputs.value.p[1]);
  const char* name =
      FloatOpIs64(node->opcode) ? "__bpf_cmpdf" : "__bpf_cmpsf";
  return EmitHelperCall(bpf, name, args, 2);
}

static TargetInstruction* LowerComparison(BPFGenerator* bpf, IRNode* node) {
  int float_cmp = node->opcode == IR_OP(cmpeqf) || node->opcode == IR_OP(cmpnef) ||
                  node->opcode == IR_OP(cmpltf) || node->opcode == IR_OP(cmplef) ||
                  node->opcode == IR_OP(cmpgtf) || node->opcode == IR_OP(cmpgef) ||
                  node->opcode == IR_OP(cmpeqd) || node->opcode == IR_OP(cmpned) ||
                  node->opcode == IR_OP(cmpltd) || node->opcode == IR_OP(cmpled) ||
                  node->opcode == IR_OP(cmpgtd) || node->opcode == IR_OP(cmpged);
  TargetInstruction* lhs = NULL;
  TargetInstruction* rhs = NULL;
  if (float_cmp) {
    lhs = FloatCompareValue(bpf, node);
    rhs = Emit(bpf, NewInstruction1(
                        BPF_OP(li),
                        GetIntConstant(bpf, NULL, kTargetType32Bit,
                                       (node->opcode == IR_OP(cmpgtf) ||
                                        node->opcode == IR_OP(cmpgtd))
                                           ? 1
                                           : 0)));
  } else {
    lhs = Materialize(bpf, node->inputs.value.p[0]);
    rhs = Materialize(bpf, node->inputs.value.p[1]);
    // jsge/jsgt are 64-bit.  A 32-bit neg or sub is zero-extended, so -4
    // compares as 4294967292 and `0 >= -4` is false.
    lhs = WidenFor64(bpf, lhs, node->inputs.value.p[0]);
    rhs = WidenFor64(bpf, rhs, node->inputs.value.p[1]);
  }
  TargetInstruction* one =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 1)));
  BPFOpcode branch = CmpBranchOp(node->opcode, CmpIsUnsigned(node));
  if (float_cmp) {
    switch (node->opcode) {
      case IR_OP(cmpeqf):
      case IR_OP(cmpeqd):
      case IR_OP(cmpgtf):
      case IR_OP(cmpgtd):
        branch = BPF_OP(jeq);
        break;
      case IR_OP(cmpnef):
      case IR_OP(cmpned):
        branch = BPF_OP(jne);
        break;
      case IR_OP(cmpltf):
      case IR_OP(cmpltd):
        branch = BPF_OP(jslt);
        break;
      case IR_OP(cmplef):
      case IR_OP(cmpled):
        branch = BPF_OP(jsle);
        break;
      default:
        // >= is true for -1? no: results 0 and 1, not the unordered 2.
        branch = BPF_OP(jle);
        rhs = Emit(bpf, NewInstruction1(
                            BPF_OP(li),
                            GetIntConstant(bpf, NULL, kTargetType32Bit, 1)));
        break;
    }
  }
  TargetInstruction* br = Emit(bpf, NewInstruction3(branch, lhs, rhs, NULL));
  TargetInstruction* zero =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  zero->dest = one;
  TargetInstruction* skip = Emit(bpf, NewInstruction(BPF_OP(label)));
  br->operand[2] = skip;
  return FinishWithDest(bpf, node, one);
}

static TargetInstruction* LowerThreeWay(BPFGenerator* bpf, IRNode* node) {
  if (node->opcode == IR_OP(cmp3wayf) || node->opcode == IR_OP(cmp3wayd)) {
    TargetInstruction* cmp = FloatCompareValue(bpf, node);
    TargetInstruction* two =
        Emit(bpf, NewInstruction1(BPF_OP(li),
                                  GetIntConstant(bpf, NULL, kTargetType32Bit, 2)));
    TargetInstruction* unordered =
        Emit(bpf, NewInstruction3(BPF_OP(jne), cmp, two, NULL));
    TargetInstruction* zero =
        Emit(bpf, NewInstruction1(BPF_OP(li),
                                  GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
    zero->dest = cmp;
    TargetInstruction* done = Emit(bpf, NewInstruction(BPF_OP(label)));
    unordered->operand[2] = done;
    return FinishWithDest(bpf, node, cmp);
  }
  TargetInstruction* a = WidenFor64(bpf, Materialize(bpf, node->inputs.value.p[0]),
                                    node->inputs.value.p[0]);
  TargetInstruction* b = WidenFor64(bpf, Materialize(bpf, node->inputs.value.p[1]),
                                    node->inputs.value.p[1]);
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

// A dense switch is a cbra followed by one bra per slot.  eBPF has no
// indirect jump, so each slot becomes a compare against the index.
static TargetInstruction* LowerComputedBranch(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* index = Materialize(bpf, node->inputs.value.p[0]);
  int slot = 0;
  for (IRNode* entry = IRNext(node);
       entry != NULL && entry->opcode == IR_OP(bra) &&
       (entry->flags & kIRJumpTableBranch) != 0;
       entry = IRNext(entry), slot++) {
    TargetInstruction* cmp = Emit(
        bpf, NewInstruction3(
                 BPF_OP(jne), index,
                 GetIntConstant(bpf, NULL, kTargetType32Bit, slot), NULL));
    TargetInstruction* ja = Emit(bpf, NewInstruction(BPF_OP(ja)));
    IRNode* target_node = entry->inputs.value.p[0];
    TargetInstruction* target =
        target_node != NULL ? target_node->data.ptr : NULL;
    if (target == NULL) {
      VectorAppend(&bpf->base.fixups, NewBranchFixup(ja, target_node, 0));
    } else {
      ja->operand[0] = target;
    }
    TargetInstruction* skip = Emit(bpf, NewInstruction(BPF_OP(label)));
    cmp->operand[2] = skip;
    entry->data.ptr = ja;
  }
  return SetLoweredNode(node, index);
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
  (void)hidden;
  // Materialize every argument before moving any of them into r1–r5.  A call
  // inside a later argument would otherwise clobber the registers already
  // holding the earlier ones, and the real call would see that garbage.
  size_t nargs = node->inputs.length > 0 ? node->inputs.length - 1 : 0;
  TargetInstruction** args = NULL;
  if (nargs > 0) {
    args = malloc(nargs * sizeof(TargetInstruction*));
    for (size_t i = 0; i < nargs; i++) {
      args[i] = Materialize(bpf, node->inputs.value.p[i + 1]);
    }
  }
  // A fresh move, not SetDest on the value.  Materializing a later argument
  // can emit a call, and the earlier value's instruction is already in the
  // stream before that call.  Pinning that instruction to r1–r5 would make
  // the call clobber the argument with nothing left to reload.
  size_t nreg = nargs < BPF_NUM_ARG_REGS ? nargs : BPF_NUM_ARG_REGS;
  for (size_t i = 0; i < nreg; i++) {
    TargetInstruction* dest = IntArgumentRegister(bpf, next_arg++);
    TargetInstruction* move = Emit(bpf, NewInstruction1(BPF_OP(mov), args[i]));
    move->dest = dest;
  }
  // r1–r5 are full.  Park the rest in the outgoing area and pass its address
  // in r0.  The callee copies them out before using r0.  The area is sized
  // to the widest call in this function.
  if (nargs > BPF_NUM_ARG_REGS) {
    int base_off = LocalOffset(bpf, bpf->outgoing_offset);
    for (size_t i = BPF_NUM_ARG_REGS; i < nargs; i++) {
      int off = base_off + (int)(i - BPF_NUM_ARG_REGS) * 8;
      if (off < -32768 || off > 32767) {
        fprintf(stderr,
                "eBPF backend: stack argument %zu offset %d does not fit in "
                "imm16\n",
                i, off);
        break;
      }
      Emit(bpf, NewInstruction3(BPF_OP(stxdw), FramePointer(bpf),
                                GetIntConstant(bpf, NULL, kTargetType32Bit, off),
                                args[i]));
    }
    TargetInstruction* addr =
        Emit(bpf, NewInstruction2(
                      BPF_OP(add), FramePointer(bpf),
                      GetIntConstant(bpf, NULL, kTargetType32Bit, base_off)));
    TargetInstruction* ptr = Emit(bpf, NewInstruction1(BPF_OP(mov), addr));
    ptr->dest = ResultReg(bpf);
  }
  free(args);
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

// The second IR operand is the bit difference (dest_bits - src_bits), not the
// width to keep.  A char promoted to int carries 24; masking with that keeps
// three bytes of garbage.  Keep the source type's low bits instead.
static int ExtendKeepBits(IRNode* node) {
  IRNode* src = node->inputs.length > 0 ? node->inputs.value.p[0] : NULL;
  int src_size = src != NULL && src->type != NULL ? src->type->size : 0;
  int dst_size = node->type != NULL ? node->type->size : src_size;
  if (src_size <= 0) {
    src_size = dst_size > 0 ? dst_size : 4;
  }
  int keep = dst_size > 0 && dst_size < src_size ? dst_size : src_size;
  if (keep >= 8) {
    return 64;
  }
  if (keep <= 0) {
    return 32;
  }
  return keep * 8;
}

static TargetInstruction* LowerZeroExtend(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[0]);
  int bits = ExtendKeepBits(node);
  if (bits >= 64) {
    return FinishWithDest(bpf, node, src);
  }
  uint64_t mask = ((uint64_t)1 << bits) - 1;
  TargetInstruction* m =
      GetIntConstant(bpf, NULL, kTargetType64Bit, (int64_t)mask);
  return FinishWithDest(bpf, node, Emit(bpf, NewInstruction2(BPF_OP(and), src, m)));
}

static TargetInstruction* LowerSignExtendOp(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src = Materialize(bpf, node->inputs.value.p[0]);
  return FinishWithDest(bpf, node, SignExtend(bpf, src, ExtendKeepBits(node)));
}

static TargetInstruction* LowerFloatIncDec(BPFGenerator* bpf, IRNode* node,
                                          bool inc) {
  IRNode* addr_node = node->inputs.value.p[0];
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, addr_node, &base, &offset);
  int is64 = node->opcode == IR_OP(incd) || node->opcode == IR_OP(decd);
  BPFOpcode ld = is64 ? BPF_OP(ldxdw) : BPF_OP(ldxw);
  BPFOpcode st = is64 ? BPF_OP(stxdw) : BPF_OP(stxw);
  TargetInstruction* cur = Emit(bpf, NewInstruction2(ld, base, offset));
  TargetInstruction* delta =
      node->inputs.length > 1 ? Materialize(bpf, node->inputs.value.p[1])
                              : Emit(bpf, NewInstruction1(
                                              BPF_OP(li),
                                              GetIntConstant(
                                                  bpf, NULL, kTargetType64Bit,
                                                  is64 ? (int64_t)0x3ff0000000000000ULL
                                                       : (int64_t)0x3f800000)));
  TargetInstruction* args[2] = {cur, delta};
  const char* name = inc ? (is64 ? "__bpf_adddf3" : "__bpf_addsf3")
                         : (is64 ? "__bpf_subdf3" : "__bpf_subsf3");
  TargetInstruction* updated = EmitHelperCall(bpf, name, args, 2);
  Emit(bpf, NewInstruction3(st, base, offset, updated));
  return FinishWithDest(bpf, node, updated);
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
  // memzero names only the destination.  The byte count is the object's size.
  IRNode* destination_node = node->inputs.value.p[0];
  IRVariable* variable = (IRVariable*)destination_node;
  int64_t size = (IRIsVariable(destination_node) && variable->symbol != NULL)
                     ? variable->symbol->type->size
                     : (node->type != NULL ? node->type->size : 0);
  TargetInstruction* dest = Materialize(bpf, destination_node);
  if (size <= 0) {
    return dest;
  }
  TargetInstruction* n =
      Emit(bpf, NewInstruction1(BPF_OP(li),
                                GetIntConstant(bpf, NULL, kTargetType64Bit, size)));
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

// The saved r0 sits in the slot just past r1–r5.  It points at the first
// unnamed stack argument.
static int VarargStackPtrOffset(BPFGenerator* bpf) {
  int slot = bpf->vararg_save_offset;
  if (slot < 0) {
    slot = 0;
  }
  return slot + BPF_NUM_ARG_REGS * 8;
}

// va_list is one pointer.  While unnamed register arguments remain, its low
// three bits are how many of those slots are left (the address itself is
// 8-aligned).  The last of them is r5, and the word after r5 is the caller's
// stack-argument pointer.  A pointer with a zero tag is already on that
// stack.  The tag travels with the pointer, so va_arg still works after the
// list is passed to another function.
static TargetInstruction* LowerVaStart(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  TargetInstruction* ap;
  if (bpf->num_int_arg_regs >= BPF_NUM_ARG_REGS) {
    ap = Emit(bpf, NewInstruction2(
                       BPF_OP(ldxdw), FramePointer(bpf),
                       GetIntConstant(bpf, NULL, kTargetType32Bit,
                                      LocalOffset(bpf, VarargStackPtrOffset(bpf)))));
  } else {
    int slot = bpf->vararg_save_offset;
    if (slot < 0) {
      slot = 0;
    }
    slot += bpf->num_int_arg_regs * 8;
    ap = Emit(bpf, NewInstruction2(
                       BPF_OP(add), FramePointer(bpf),
                       GetIntConstant(bpf, NULL, kTargetType32Bit,
                                      LocalOffset(bpf, slot))));
    int remain = BPF_NUM_ARG_REGS - bpf->num_int_arg_regs;
    ap = Emit(bpf, NewInstruction2(
                       BPF_OP(or), ap,
                       GetIntConstant(bpf, NULL, kTargetType32Bit, remain)));
  }
  Emit(bpf, NewInstruction3(BPF_OP(stxdw), base, offset, ap));
  return SetLoweredNode(node, ap);
}

static TargetInstruction* LowerVaArg(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* base;
  TargetInstruction* offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &base, &offset);
  TargetInstruction* raw = Emit(bpf, NewInstruction2(BPF_OP(ldxdw), base, offset));
  TargetInstruction* tag =
      Emit(bpf, NewInstruction2(BPF_OP(and), raw,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 7)));
  // ~7 in a sign-extended immediate is -8, which clears the tag.
  TargetInstruction* ptr =
      Emit(bpf, NewInstruction2(BPF_OP(and), raw,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, -8)));
  int size = node->type != NULL && node->type->size > 0 ? (int)node->type->size : 8;
  BPFOpcode load = BPF_OP(ldxdw);
  if (size <= 1) {
    load = BPF_OP(ldxb);
  } else if (size <= 2) {
    load = BPF_OP(ldxh);
  } else if (size <= 4) {
    load = BPF_OP(ldxw);
  }
  TargetInstruction* value =
      Emit(bpf, NewInstruction2(load, ptr,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0)));
  if (node->type != NULL && !TypeIsUnsigned(node->type) && size < 8 && size != 3) {
    value = SignExtend(bpf, value, size * 8);
  }
  TargetInstruction* eight = GetIntConstant(bpf, NULL, kTargetType32Bit, 8);
  TargetInstruction* next_slot =
      Emit(bpf, NewInstruction2(BPF_OP(add), ptr, eight));
  // Tag 0: already walking the caller's stack, so the next slot is adjacent.
  TargetInstruction* next = Emit(bpf, NewInstruction1(BPF_OP(mov), next_slot));
  TargetInstruction* on_stack =
      Emit(bpf, NewInstruction3(BPF_OP(jeq), tag,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 0),
                                NULL));
  // Tag 1: this slot is r5.  The word after it is the stack-argument pointer.
  // A larger tag steps to the next register slot and decrements.
  // Both stores write `next`, so the update below reads one register.
  TargetInstruction* from_stack =
      Emit(bpf, NewInstruction2(BPF_OP(ldxdw), ptr, eight));
  from_stack->dest = next;
  TargetInstruction* was_last =
      Emit(bpf, NewInstruction3(BPF_OP(jeq), tag,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 1),
                                NULL));
  TargetInstruction* tag_m1 =
      Emit(bpf, NewInstruction2(BPF_OP(sub), tag,
                                GetIntConstant(bpf, NULL, kTargetType32Bit, 1)));
  TargetInstruction* stepped =
      Emit(bpf, NewInstruction2(BPF_OP(or), next_slot, tag_m1));
  stepped->dest = next;
  TargetInstruction* done = Emit(bpf, NewInstruction(BPF_OP(label)));
  on_stack->operand[2] = done;
  was_last->operand[2] = done;
  Emit(bpf, NewInstruction3(BPF_OP(stxdw), base, offset, next));
  return FinishWithDest(bpf, node, value);
}

static TargetInstruction* LowerVaCopy(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* src_base;
  TargetInstruction* src_offset;
  TargetInstruction* dst_base;
  TargetInstruction* dst_offset;
  GetBaseAndOffset(bpf, node->inputs.value.p[1], &src_base, &src_offset);
  GetBaseAndOffset(bpf, node->inputs.value.p[0], &dst_base, &dst_offset);
  TargetInstruction* value =
      Emit(bpf, NewInstruction2(BPF_OP(ldxdw), src_base, src_offset));
  return SetLoweredNode(
      node, Emit(bpf, NewInstruction3(BPF_OP(stxdw), dst_base, dst_offset, value)));
}

static int BitfieldConst(IRNode* node) {
  if (node != NULL && IRIsConst(node)) {
    return (int)((IRConstant*)node)->value.ivalue;
  }
  return 0;
}

static int BitfieldWidth(IRNode* word) {
  if (word != NULL && word->type != NULL) {
    TypeRecordCalculateSize(word->type);
    int width = word->type->size * 8;
    if (width > 0 && width <= 64) {
      return width;
    }
  }
  return 32;
}

static TargetInstruction* MaskTo(BPFGenerator* bpf, TargetInstruction* value,
                                 int bits) {
  if (bits >= 64) {
    return value;
  }
  uint64_t mask = ((uint64_t)1 << bits) - 1;
  return Emit(bpf, NewInstruction2(
                       BPF_OP(and), value,
                       GetIntConstant(bpf, NULL, kTargetType64Bit, (int64_t)mask)));
}

// getbit(word, offset, size) extracts a field.  BPF has no insert/extract
// instruction, so this is a shift and a mask.  A signed field is sign-extended
// to its result type; the high bits stay clear so a later widen can extend
// them again.
static TargetInstruction* LowerGetBit(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* word = Materialize(bpf, node->inputs.value.p[0]);
  int offset = BitfieldConst(node->inputs.value.p[1]);
  int size = BitfieldConst(node->inputs.value.p[2]);
  int width = BitfieldWidth(node->inputs.value.p[0]);
  if (size <= 0) {
    size = 1;
  }
  word = MaskTo(bpf, word, width);
  if (offset > 0) {
    word = Emit(bpf, NewInstruction2(
                         BPF_OP(rsh), word,
                         GetIntConstant(bpf, NULL, kTargetType32Bit, offset)));
  }
  word = MaskTo(bpf, word, size);
  if (node->type != NULL && !TypeIsUnsigned(node->type) && size < 64) {
    word = SignExtend(bpf, word, size);
    int result_bits = node->type->size > 0 ? node->type->size * 8 : 32;
    word = MaskTo(bpf, word, result_bits);
  }
  return FinishWithDest(bpf, node, word);
}

// setbit(word, value, offset, size) replaces the field and leaves the other
// bits of the storage word alone.
static TargetInstruction* LowerSetBit(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* word = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* value = Materialize(bpf, node->inputs.value.p[1]);
  int offset = BitfieldConst(node->inputs.value.p[2]);
  int size = BitfieldConst(node->inputs.value.p[3]);
  int width = BitfieldWidth(node->inputs.value.p[0]);
  if (size <= 0) {
    size = 1;
  }
  uint64_t field_mask = (((uint64_t)1 << size) - 1) << offset;
  if (offset > 0) {
    value = Emit(bpf, NewInstruction2(
                          BPF_OP(lsh), value,
                          GetIntConstant(bpf, NULL, kTargetType32Bit, offset)));
  }
  value = Emit(bpf, NewInstruction2(
                        BPF_OP(and), value,
                        GetIntConstant(bpf, NULL, kTargetType64Bit,
                                       (int64_t)field_mask)));
  TargetInstruction* rest = Emit(
      bpf, NewInstruction2(BPF_OP(and), word,
                           GetIntConstant(bpf, NULL, kTargetType64Bit,
                                          (int64_t)~field_mask)));
  TargetInstruction* merged =
      Emit(bpf, NewInstruction2(BPF_OP(or), rest, value));
  return FinishWithDest(bpf, node, MaskTo(bpf, merged, width));
}

static bool FunctionUsesDynamicStack(Generator* gen) {
  for (IRNode* node = (IRNode*)gen->code.first; node != NULL;
       node = IRNext(node)) {
    if (node->opcode == IR_OP(decsp) || node->opcode == IR_OP(savesp) ||
        node->opcode == IR_OP(restoresp)) {
      return true;
    }
  }
  return false;
}

static int DynamicSPSlot(BPFGenerator* bpf) {
  return LocalOffset(bpf, bpf->dynamic_sp_offset);
}

static TargetInstruction* LoadDynamicSP(BPFGenerator* bpf) {
  return Emit(bpf, NewInstruction2(
                       BPF_OP(ldxdw), FramePointer(bpf),
                       GetIntConstant(bpf, NULL, kTargetType32Bit,
                                      DynamicSPSlot(bpf))));
}

static void StoreDynamicSP(BPFGenerator* bpf, TargetInstruction* value) {
  Emit(bpf, NewInstruction3(
                BPF_OP(stxdw), FramePointer(bpf),
                GetIntConstant(bpf, NULL, kTargetType32Bit, DynamicSPSlot(bpf)),
                value));
}

static TargetInstruction* LowerDecSP(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* sp = LoadDynamicSP(bpf);
  TargetInstruction* size = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* next =
      Emit(bpf, NewInstruction2(BPF_OP(sub), sp, size));
  StoreDynamicSP(bpf, next);
  return SetLoweredNode(node, next);
}

static TargetInstruction* LowerSaveSP(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* dest = Materialize(bpf, node->inputs.value.p[0]);
  TargetInstruction* saved = Emit(bpf, NewInstruction1(BPF_OP(mov), LoadDynamicSP(bpf)));
  saved->dest = dest;
  return SetLoweredNode(node, saved);
}

static TargetInstruction* LowerRestoreSP(BPFGenerator* bpf, IRNode* node) {
  TargetInstruction* value = Materialize(bpf, node->inputs.value.p[0]);
  StoreDynamicSP(bpf, value);
  return SetLoweredNode(node, value);
}

// Arguments past r5 are stored in the caller's frame.  Reserve one slot per
// extra argument of the widest call, instead of a fixed 16 that dropped the rest.
static int WidestStackArgCount(Generator* gen) {
  int max_stack = 0;
  for (IRNode* node = (IRNode*)gen->code.first; node != NULL; node = IRNext(node)) {
    if (!IRIsCall(node)) {
      continue;
    }
    int nargs = node->inputs.length > 0 ? (int)node->inputs.length - 1 : 0;
    int stack = nargs - BPF_NUM_ARG_REGS;
    if (stack > max_stack) {
      max_stack = stack;
    }
  }
  return max_stack;
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
        if ((pooled->flags & kIRNrvoMarker) != 0) {
          // The caller passed the return buffer in r1.  Capture it before
          // the body reuses that register, and build the object there.
          TargetInstruction* ptr =
              Emit(bpf, NewInstruction1(BPF_OP(mov), IntArgumentRegister(bpf, 0)));
          bpf->nrvo_pointer = ptr;
          pooled->data.ivalue = 0;
          break;
        }
        var_offset = (var_offset + align - 1) & ~(align - 1);
        pooled->data.ivalue = var_offset;
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
  // Incoming registers follow the prototype, not the variable-pool order.
  // A hidden aggregate result occupies r1; the first user argument is r2.
  bpf->num_int_arg_regs = 0;
  int hidden_result =
      gen->func != NULL && gen->func->next != NULL &&
              TypeReturnedThroughHiddenPointer(gen->func->next)
          ? 1
          : 0;
  Vector* prototype = &gen->func->info.function.prototype;
  int stack_index = 0;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* sym = prototype->value.p[i];
    if (sym == NULL) {
      continue;
    }
    bool on_stack =
        hidden_result + bpf->num_int_arg_regs >= BPF_NUM_ARG_REGS;
    IRNode* pooled = NULL;
    for (size_t j = 0; j < gen->variable_pool.length; j++) {
      IRNode* candidate = ((PoolEntry*)gen->variable_pool.value.p[j])->pooled;
      if (candidate->opcode == IR_OP(argument) &&
          ((IRVariable*)candidate)->symbol == sym) {
        pooled = candidate;
        break;
      }
    }
    // An unused parameter is absent from the variable pool, but the caller
    // still passed it.  Member functions often leave `this` unused; skipping
    // the slot would assign the next parameter to the same place.
    if (pooled == NULL) {
      if (on_stack) {
        stack_index++;
      } else {
        bpf->num_int_arg_regs++;
      }
      continue;
    }
    TypeRecord* type = sym->type != NULL ? sym->type : pooled->type;
    TypeRecordCalculateSize(type);
    int64_t size = type != NULL ? type->size : 8;
    BPFSavedArgument* saved = malloc(sizeof(BPFSavedArgument));
    saved->offset = pooled->data.ivalue;
    saved->size = size > 4 ? 8 : (int)size;
    saved->copy_bytes = 0;
    saved->copy_align = 0;
    // Aggregates are passed as a pointer to the caller's object.  The frame
    // slot is the parameter, so copy the bytes in; storing the pointer itself
    // makes every field access read the address bits.
    if (type != NULL && type->size > 0 &&
        TypeReturnedThroughHiddenPointer(type)) {
      saved->copy_bytes = (int)type->size;
      int align = TypeRecordAlignment(type);
      saved->copy_align = align > 0 ? align : 1;
    }
    if (on_stack) {
      saved->reg_num = -1;
      saved->stack_index = stack_index++;
    } else {
      saved->reg_num = BPF_REG_1 + hidden_result + bpf->num_int_arg_regs;
      saved->stack_index = -1;
      bpf->num_int_arg_regs++;
    }
    VectorAppend(&bpf->saved_args, saved);
  }
  bpf->named_stack_args = stack_index;
  // Named parameters occupy the first slots.  One extra slot holds the
  // caller's pointer to arguments past r5, so va_arg can continue there.
  if (bpf->base.varargs) {
    var_offset = (var_offset + 7) & ~7;
    bpf->vararg_save_offset = var_offset;
    var_offset += (BPF_NUM_ARG_REGS + 1) * 8;
  }
  // Room for arguments past r5.  Each is one register-width value or an
  // aggregate pointer.  Calls created while lowering (soft-float helpers)
  // take at most two register arguments and do not need a slot here.
  var_offset = (var_offset + 7) & ~7;
  bpf->outgoing_offset = var_offset;
  var_offset += WidestStackArgCount(gen) * 8;
  if (FunctionUsesDynamicStack(gen)) {
    var_offset = (var_offset + 7) & ~7;
    bpf->dynamic_sp_offset = var_offset;
    var_offset += 8;
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
    case IR_OP(prefetch):
    case IR_OP(nop):
    case last_ir_opcode:
      return NULL;

    case IR_OP(ssavar):
    case IR_OP(phi):
      break;

    case IR_OP(structreturn): {
      // The structreturn pseudo is not in the instruction stream, so it never
      // receives a register.  Users of the hidden result pointer must name
      // the move that captured r1; otherwise every use prints as r0 and the
      // return slot is a null store.
      TargetInstruction* result =
          EmitSymbol(bpf, NewInstruction(BPF_OP(structreturn)));
      TargetInstruction* mv =
          Emit(bpf, NewInstruction1(BPF_OP(mov), IntArgumentRegister(bpf, 0)));
      mv->dest = result;
      return SetLoweredNode(node, mv);
    }

    case IR_OP(literalref): {
      // The id lives in the constant input.  The literalref node itself is
      // not that constant; reading it as one emits `.str.0` for every string.
      IRConstant* id = node->inputs.value.p[0];
      return FinishWithDest(
          bpf, node,
          Emit(bpf, NewInstruction1(BPF_OP(lddw),
                                    TargetNewLiteral((int)id->value.ivalue))));
    }

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
    case IR_OP(constf): {
      float f = (float)((IRConstant*)node)->value.fvalue;
      uint32_t bits = 0;
      memcpy(&bits, &f, sizeof(bits));
      return GetIntConstant(bpf, node, kTargetType64Bit, (int64_t)bits);
    }
    case IR_OP(constd): {
      double d = ((IRConstant*)node)->value.fvalue;
      uint64_t bits = 0;
      memcpy(&bits, &d, sizeof(bits));
      return GetIntConstant(bpf, node, kTargetType64Bit, (int64_t)bits);
    }

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
      return LowerIncDec(bpf, node, true);
    case IR_OP(incf):
    case IR_OP(incd):
      return LowerFloatIncDec(bpf, node, true);
    case IR_OP(dec8):
    case IR_OP(dec16):
    case IR_OP(dec32):
    case IR_OP(dec64):
    case IR_OP(udec8):
    case IR_OP(udec16):
    case IR_OP(udec32):
    case IR_OP(udec64):
    case IR_OP(deca):
      return LowerIncDec(bpf, node, false);
    case IR_OP(decf):
    case IR_OP(decd):
      return LowerFloatIncDec(bpf, node, false);

    case IR_OP(getbit):
      return LowerGetBit(bpf, node);
    case IR_OP(setbit):
      return LowerSetBit(bpf, node);

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
      return LowerFloatBinop(bpf, node);
    case IR_OP(negf):
    case IR_OP(negd):
      return LowerFloatNeg(bpf, node);
    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):
      return LowerFloatConvert(bpf, node);
    case IR_OP(movf):
    case IR_OP(movd):
      return FinishWithDest(bpf, node,
                            Materialize(bpf, node->inputs.value.p[0]));

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
      return LowerComputedBranch(bpf, node);

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
    case IR_OP(atomic_load):
      return LowerAtomicLoad(bpf, node);
    case IR_OP(atomic_store):
      return LowerAtomicStore(bpf, node);
    case IR_OP(atomic_fetch_add):
      return LowerAtomicRmw(bpf, node, true, false);
    case IR_OP(atomic_fetch_sub):
      return LowerAtomicRmw(bpf, node, false, false);
    case IR_OP(atomic_add_fetch):
      return LowerAtomicRmw(bpf, node, true, true);
    case IR_OP(atomic_sub_fetch):
      return LowerAtomicRmw(bpf, node, false, true);
    case IR_OP(atomic_compare_exchange_bool):
    case IR_OP(atomic_compare_exchange_val):
    case IR_OP(atomic_compare_exchange_n):
      return LowerAtomicCompareExchange(bpf, node);
    case IR_OP(atomic_fence):
      return Emit(bpf, NewInstruction(BPF_OP(nop)));
    case IR_OP(decsp):
      return LowerDecSP(bpf, node);
    case IR_OP(savesp):
      return LowerSaveSP(bpf, node);
    case IR_OP(restoresp):
      return LowerRestoreSP(bpf, node);
    case IR_OP(builtin_va_start):
      return LowerVaStart(bpf, node);
    case IR_OP(builtin_va_arg):
      return LowerVaArg(bpf, node);
    case IR_OP(builtin_va_end):
      return SetLoweredNode(node, Emit(bpf, NewInstruction(BPF_OP(nop))));
    case IR_OP(builtin_va_copy):
      return LowerVaCopy(bpf, node);
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
