//
//  optimizer.c
//  c_compiler_library
//
//  Created by David Allison on 6/12/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "optimizer.h"
#include "basic_block.h"
#include "type_compare.h"
#include "type_core.h"
#include <limits.h>
#include <stdint.h>

// Is the node an integer constant with the value given?
static bool IsIntConstantWithValue(IRNode* node, int value) {
  if (node == NULL) {
    return false;
  }
  if (!IRIsConst(node)) {
    return false;
  }
  if (node->type == NULL || !TypeIsIntegral(node->type)) {
    return false;
  }
  IRConstant* c = (IRConstant*)node;
  return c->value.ivalue == value;
}

// Is the node an integer constant power of 2 up to maxbits bits?
// A power of two only has one bit set to 1, all the others are zero.
static bool IsIntConstantPowerOf2(IRNode* node, int maxbits) {
  if (node == NULL) {
    return false;
  }
  if (!IRIsConst(node)) {
    return false;
  }
  if (node->type == NULL || !TypeIsIntegral(node->type)) {
    return false;
  }
  IRConstant* c = (IRConstant*)node;
  if (maxbits <= 0 || maxbits > 64) {
    return false;
  }
  if (!TypeIsUnsigned(node->type) && c->value.ivalue <= 0) {
    return false;
  }
  uint64_t value = (uint64_t)c->value.ivalue;
  if (maxbits < 64) {
    uint64_t mask = (UINT64_C(1) << maxbits) - 1;
    if ((value & ~mask) != 0) {
      return false;
    }
    value &= mask;
  }
  return value != 0 && (value & (value - 1)) == 0;
}

// The node is an integer power of 2.  What is its log?  This is the
// number of zero bits up to the first one bit.
// For example, the value 128 (0x80) is 7.
static int LogBase2(IRNode* node) {
  IRConstant* c = (IRConstant*)node;
  int zerocount = 0;
  uint64_t value = (uint64_t)c->value.ivalue;
  while ((value & 1) == 0) {
    zerocount++;
    value >>= 1;
  }
  return zerocount;
}


// The node is an integer power of 2, generate a bitmask suitable for
// ANDing.  This is simply the number minus 1.
static int64_t BitMask(IRNode* node) {
  IRConstant* c = (IRConstant*)node;
  int64_t value = c->value.ivalue;
  return value - 1;
}

static int64_t WrapToType(TypeRecord* type, int64_t value) {
  int bits = type->size * 8;
  if (bits <= 0 || bits >= 64) {
    return value;
  }
  uint64_t mask = (UINT64_C(1) << bits) - 1;
  uint64_t bits_val = (uint64_t)value & mask;
  if (!TypeIsUnsigned(type) &&
      (bits_val & (UINT64_C(1) << (bits - 1))) != 0) {
    return (int64_t)(bits_val | ~mask);
  }
  return (int64_t)bits_val;
}

static bool IsIntAllOnes(IRNode* node) {
  if (node == NULL || !IRIsIntConst(node) || node->type == NULL ||
      !TypeIsIntegral(node->type)) {
    return false;
  }
  int bits = node->type->size * 8;
  if (bits <= 0 || bits > 64) {
    return false;
  }
  uint64_t value = (uint64_t)IRIntConstValue(node);
  if (bits < 64) {
    uint64_t mask = (UINT64_C(1) << bits) - 1;
    return (value & mask) == mask;
  }
  return value == UINT64_MAX;
}

static int ConstOperandIndex(IRNode* inst) {
  if (inst == NULL || inst->inputs.length != 2) {
    return -1;
  }
  bool left = IRIsIntConst(inst->inputs.value.p[0]);
  bool right = IRIsIntConst(inst->inputs.value.p[1]);
  if (left && !right) {
    return 0;
  }
  if (right && !left) {
    return 1;
  }
  return -1;
}

static bool FoldNestedAdd(Generator* gen, BasicBlock* block, IRNode* node) {
  int outer = ConstOperandIndex(node);
  if (outer < 0) {
    return false;
  }
  IRNode* inner = node->inputs.value.p[1 - outer];
  if (inner == NULL || inner->opcode != IR_OP(addi) || inner->dest != NULL ||
      inner->type == NULL || !TypeEqual(inner->type, node->type)) {
    return false;
  }
  int inner_const = ConstOperandIndex(inner);
  if (inner_const < 0) {
    return false;
  }
  int64_t sum = WrapToType(
      node->type, IRIntConstValue(node->inputs.value.p[outer]) +
                      IRIntConstValue(inner->inputs.value.p[inner_const]));
  IRNode* base = inner->inputs.value.p[1 - inner_const];
  if (sum == 0) {
    BasicBlockReplaceInstruction(gen, block, node, base);
    return true;
  }
  IRNode* combined = GeneratorGetIntConstant(gen, node->type, sum);
  IRReplaceInput(node, 0, base);
  IRReplaceInput(node, 1, combined);
  return true;
}

typedef struct {
  uint32_t magic;
  int shift;
  bool add;
} UnsignedDivMagic;

typedef struct {
  int32_t magic;
  int shift;
} SignedDivMagic;

// Hacker's Delight Fig. 10-3.  M, s, and the add indicator implement
// unsigned 32-bit n/d as a widening multiply and shifts for every n.
static UnsignedDivMagic UnsignedMagic32(uint32_t d) {
  UnsignedDivMagic mag = {0, 0, false};
  uint32_t nc = 0xffffffffu - ((uint32_t)(0u - d) % d);
  int p = 31;
  uint32_t q1 = 0x80000000u / nc;
  uint32_t r1 = 0x80000000u - q1 * nc;
  uint32_t q2 = 0x7fffffffu / d;
  uint32_t r2 = 0x7fffffffu - q2 * d;
  do {
    p++;
    if (r1 >= nc - r1) {
      q1 = 2 * q1 + 1;
      r1 = 2 * r1 - nc;
    } else {
      q1 = 2 * q1;
      r1 = 2 * r1;
    }
    if (r2 + 1 >= d - r2) {
      if (q2 >= 0x7fffffffu) {
        mag.add = true;
      }
      q2 = 2 * q2 + 1;
      r2 = 2 * r2 + 1 - d;
    } else {
      if (q2 >= 0x80000000u) {
        mag.add = true;
      }
      q2 = 2 * q2;
      r2 = 2 * r2 + 1;
    }
  } while (p < 64 && (q1 < d - 1 - r2 || (q1 == d - 1 - r2 && r1 == 0)));
  mag.magic = q2 + 1;
  mag.shift = p - 32;
  return mag;
}

// Hacker's Delight Fig. 10-1.  Positive d only; M may still be negative.
static SignedDivMagic SignedMagic32(int32_t d) {
  SignedDivMagic mag = {0, 0};
  uint32_t ad = (uint32_t)d;
  uint32_t two31 = 0x80000000u;
  uint32_t t = two31 + ((uint32_t)d >> 31);
  uint32_t anc = t - 1 - t % ad;
  int p = 31;
  uint32_t q1 = two31 / anc;
  uint32_t r1 = two31 - q1 * anc;
  uint32_t q2 = two31 / ad;
  uint32_t r2 = two31 - q2 * ad;
  uint32_t delta;
  do {
    p++;
    q1 = 2 * q1;
    r1 = 2 * r1;
    if (r1 >= anc) {
      q1 = q1 + 1;
      r1 = r1 - anc;
    }
    q2 = 2 * q2;
    r2 = 2 * r2;
    if (r2 >= ad) {
      q2 = q2 + 1;
      r2 = r2 - ad;
    }
    delta = ad - r2;
  } while (q1 < delta || (q1 == delta && r1 == 0));
  mag.magic = (int32_t)(q2 + 1);
  mag.shift = p - 32;
  return mag;
}

static IRNode* EmitBinBefore(Generator* gen, BasicBlock* block, IRNode* pos,
                             IROpcode opcode, IRNode* left, IRNode* right,
                             TypeRecord* type) {
  IRNode* inst = IRSetType(NewIR2(opcode, left, right), type);
  inst->location = pos->location;
  BasicBlockEmitBefore(gen, block, inst, pos);
  return inst;
}

static IRNode* ExtendBefore(Generator* gen, BasicBlock* block, IRNode* pos,
                            IRNode* value, TypeRecord* wide, bool is_signed) {
  int bit_diff = (wide->size - value->type->size) * 8;
  if (bit_diff == 0) {
    return value;
  }
  IROpcode op = is_signed ? IR_OP(signextendi) : IR_OP(zeroextendi);
  IRNode* bits = GeneratorGetIntConstant(gen, wide, bit_diff);
  return EmitBinBefore(gen, block, pos, op, value, bits, wide);
}

static IRNode* NarrowBefore(Generator* gen, BasicBlock* block, IRNode* pos,
                            IRNode* value, TypeRecord* narrow, bool is_signed) {
  if (value->type != NULL && value->type->size <= narrow->size) {
    return value;
  }
  int bit_diff = (narrow->size - value->type->size) * 8;
  IROpcode op = is_signed ? IR_OP(signextendi) : IR_OP(zeroextendi);
  IRNode* bits = GeneratorGetIntConstant(gen, narrow, bit_diff);
  return EmitBinBefore(gen, block, pos, op, value, bits, narrow);
}

// Replace n / C with a widening multiply and shifts.  32-bit (and narrower)
// integers only: 64-bit division would need a 128-bit product.
static bool FoldConstantDivision(Generator* gen, BasicBlock* block,
                                 IRNode* node) {
  if (node->type == NULL || !TypeIsIntegral(node->type) ||
      node->type->size <= 0 || node->type->size > 4) {
    return false;
  }
  IRNode* dividend = node->inputs.value.p[0];
  IRNode* divisor = node->inputs.value.p[1];
  if (dividend == NULL || dividend->type == NULL || !IRIsIntConst(divisor)) {
    return false;
  }
  TypeRecord* wide_u =
      NewTypeRecordWithSize(kTypeLongLong | kTypeUnsigned, kQualPlain);
  TypeRecord* wide_s = NewTypeRecordWithSize(kTypeLongLong, kQualPlain);
  if (wide_u == NULL || wide_s == NULL || wide_u->size != 8 ||
      wide_s->size != 8) {
    return false;
  }

  IRNode* result = NULL;
  if (TypeIsUnsigned(node->type)) {
    uint64_t d = (uint64_t)IRIntConstValue(divisor);
    if (node->type->size < 8) {
      d &= (UINT64_C(1) << (node->type->size * 8)) - 1;
    }
    if (d <= 1) {
      return false;
    }
    UnsignedDivMagic mag = UnsignedMagic32((uint32_t)d);
    IRNode* n64 = ExtendBefore(gen, block, node, dividend, wide_u, false);
    IRNode* magic = GeneratorGetIntConstant(gen, wide_u, (int64_t)mag.magic);
    IRNode* prod =
        EmitBinBefore(gen, block, node, IR_OP(muli), n64, magic, wide_u);
    if (!mag.add) {
      IRNode* shamt =
          GeneratorGetIntConstant(gen, wide_u, 32 + mag.shift);
      result = EmitBinBefore(gen, block, node, IR_OP(lsri), prod, shamt, wide_u);
    } else {
      IRNode* sh32 = GeneratorGetIntConstant(gen, wide_u, 32);
      IRNode* hi =
          EmitBinBefore(gen, block, node, IR_OP(lsri), prod, sh32, wide_u);
      IRNode* sum =
          EmitBinBefore(gen, block, node, IR_OP(addi), hi, n64, wide_u);
      IRNode* shamt = GeneratorGetIntConstant(gen, wide_u, mag.shift);
      result = mag.shift == 0 ? sum
                              : EmitBinBefore(gen, block, node, IR_OP(lsri),
                                              sum, shamt, wide_u);
    }
  } else {
    int64_t d = IRIntConstValue(divisor);
    if (d <= 1 || d > INT32_MAX) {
      return false;
    }
    SignedDivMagic mag = SignedMagic32((int32_t)d);
    IRNode* n64 = ExtendBefore(gen, block, node, dividend, wide_s, true);
    IRNode* magic = GeneratorGetIntConstant(gen, wide_s, mag.magic);
    IRNode* prod =
        EmitBinBefore(gen, block, node, IR_OP(muli), n64, magic, wide_s);
    IRNode* sh32 = GeneratorGetIntConstant(gen, wide_s, 32);
    IRNode* hi =
        EmitBinBefore(gen, block, node, IR_OP(asri), prod, sh32, wide_s);
    if (mag.magic < 0) {
      hi = EmitBinBefore(gen, block, node, IR_OP(addi), hi, n64, wide_s);
    }
    if (mag.shift > 0) {
      IRNode* shamt = GeneratorGetIntConstant(gen, wide_s, mag.shift);
      hi = EmitBinBefore(gen, block, node, IR_OP(asri), hi, shamt, wide_s);
    }
    IRNode* signbit =
        EmitBinBefore(gen, block, node, IR_OP(lsri), n64,
                      GeneratorGetIntConstant(gen, wide_s, 63), wide_s);
    result = EmitBinBefore(gen, block, node, IR_OP(addi), hi, signbit, wide_s);
  }

  result = NarrowBefore(gen, block, node, result, node->type,
                        !TypeIsUnsigned(node->type));
  BasicBlockReplaceInstruction(gen, block, node, result);
  return true;
}

// Perform strength reduction on the given node (in the given basic block). This
// looks at the operation and its operands.  If it can be simplified into
// something that is cheaper to execute, it is replaced by the better
// instruction.  For exmaple, multiplication is an expensive operation on most
// processors so if we can convert it to a shift the program might run faster.
static void ReduceNodeStrength(Generator* gen, BasicBlock* block,
                               IRNode* node) {
  // A non-NULL dest means this node's result must be written into a specific
  // merge temporary (e.g. the shared tmp for the arms of `a || b`, `a && b` or
  // a `?:`).  The simplifications below replace `node` with one of its inputs,
  // which silently drops that dest linkage -- the merge tmp would then never
  // receive the value (e.g. `x || (0 + f())` folds `0 + f()` to `f()` and loses
  // the assignment of f()'s result into the `||` result tmp).  Skip strength
  // reduction in that case; it only costs a minor, rare optimization.
  if (node->dest != NULL) {
    return;
  }
  for (size_t i = 0; i < node->inputs.length; i++) {
    if (IRCheckpointBetween(node->inputs.value.p[i], node)) {
      return;
    }
  }
  switch (node->opcode) {
    case IR_OP(addi):
      // Adding constant 0 is a nop.
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        FoldNestedAdd(gen, block, node);
      }
      break;
    case IR_OP(subi):
      // Subtracting zero is a nop.
      if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Subtracting from zero is a negation.
        if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
          node->opcode = IR_OP(negi);
          IRRemoveInput(node, 0);
        }
      }
      break;
    case IR_OP(muli):
      // Multiply by zero is zero.
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[0], 1)) {
        // Multiply by 1 is a nop.
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 1)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Multiply by a power of 2 less than the word width is a left shift.
        int maxbits = node->type->size * 8;
        if (IsIntConstantPowerOf2(node->inputs.value.p[0], maxbits)) {
          // Left is power of 2, convert to shift with left input moved to
          // the right and replaced by its log (base 2).
          node->opcode = IR_OP(lsli);
          IRNode* c = (IRNode*)node->inputs.value.p[0];
          IRNode* log2 = GeneratorGetIntConstant(
              gen, c->type, LogBase2(node->inputs.value.p[0]));
          IRRemoveInput(node, 0);
          IRAddInput(node, log2, false);
        } else if (IsIntConstantPowerOf2(node->inputs.value.p[1], maxbits)) {
          // Left is power of 2, convert to shift replaced by its log (base 2).
          node->opcode = IR_OP(lsli);
          IRNode* c = (IRNode*)node->inputs.value.p[1];
          IRNode* log2 = GeneratorGetIntConstant(
              gen, c->type, LogBase2(node->inputs.value.p[1]));
          IRReplaceInput(node, 1, log2);
        }
      }
      break;

    case IR_OP(divi):
      if (IsIntConstantWithValue(node->inputs.value.p[1], 1)) {
        // Division by 1 is nop.
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        // Division of zero by anything is zero.
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Division by a power of 2 less than the word width is a right shift.
        int maxbits = node->type->size * 8;
        if (TypeIsUnsigned(node->type) &&
            IsIntConstantPowerOf2(node->inputs.value.p[1], maxbits)) {
          // Left is power of 2, convert to shift with left input moved to
          // the right and replaced by its log (base 2).
          node->opcode = TypeIsUnsigned(node->type) ? IR_OP(lsri) : IR_OP(asri);
          IRNode* c = (IRNode*)node->inputs.value.p[1];
          IRNode* log2 = GeneratorGetIntConstant(
              gen, c->type, LogBase2(node->inputs.value.p[1]));
          IRReplaceInput(node, 1, log2);
        } else {
          FoldConstantDivision(gen, block, node);
        }
      }
      break;

    case IR_OP(modi):
      // Modulus of zero by anything is zero.
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else {
        // Modulus with a power of 2 is an AND (with the value - 1)
        // For example, x % 8 is the same as x & 0x7
        int maxbits = node->type->size * 8;
        if (TypeIsUnsigned(node->type) &&
            IsIntConstantPowerOf2(node->inputs.value.p[1], maxbits)) {
          // Left is power of 2, convert to AND with mask.
          node->opcode = IR_OP(andi);
          IRNode* c = (IRNode*)node->inputs.value.p[1];
          IRNode* mask = GeneratorGetIntConstant(gen, c->type, BitMask(c));
          IRReplaceInput(node, 1, mask);
        }
      }
      break;

    case IR_OP(andi):
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0) ||
          IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        IRNode* zero = IsIntConstantWithValue(node->inputs.value.p[0], 0)
                           ? node->inputs.value.p[0]
                           : node->inputs.value.p[1];
        BasicBlockReplaceInstruction(gen, block, node, zero);
      } else if (IsIntAllOnes(node->inputs.value.p[0])) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntAllOnes(node->inputs.value.p[1])) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      }
      break;

    case IR_OP(ori):
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      } else if (IsIntAllOnes(node->inputs.value.p[0]) ||
                 IsIntAllOnes(node->inputs.value.p[1])) {
        IRNode* ones = IsIntAllOnes(node->inputs.value.p[0])
                           ? node->inputs.value.p[0]
                           : node->inputs.value.p[1];
        BasicBlockReplaceInstruction(gen, block, node, ones);
      }
      break;

    case IR_OP(xori):
      if (IsIntConstantWithValue(node->inputs.value.p[0], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[1]);
      } else if (IsIntConstantWithValue(node->inputs.value.p[1], 0)) {
        BasicBlockReplaceInstruction(gen, block, node, node->inputs.value.p[0]);
      }
      break;

    case IR_OP(cmpeqi):
      if (node->inputs.length == 2 &&
          node->inputs.value.p[0] == node->inputs.value.p[1]) {
        BasicBlockReplaceInstruction(
            gen, block, node, GeneratorGetIntConstant(gen, node->type, 1));
      }
      break;

    case IR_OP(cmpnei):
      if (node->inputs.length == 2 &&
          node->inputs.value.p[0] == node->inputs.value.p[1]) {
        BasicBlockReplaceInstruction(
            gen, block, node, GeneratorGetIntConstant(gen, node->type, 0));
      }
      break;

    default:
      break;
  }
}

// Go through all the code in all basic blocks seeing if we can make
// instructions cheaper to execute.
void StrengthReductionOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    IRNode* next;
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = next) {
      next = IRNext(inst);
      ReduceNodeStrength(gen, block, inst);
    }
  }
}


// Instructions that may legitimately appear after a call without preventing a
// tail call: they are just the return bookkeeping (labels, the procedure
// leave/return, branches to the exit) and do not run any user code.
static bool IsTrivialReturnInstruction(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(label):
    case IR_OP(leave):
    case IR_OP(ret):
    case IR_OP(bra):
    case IR_OP(nop):
    case IR_OP(enter):
      return true;
    default:
      return false;
  }
}

// The last IR_OP(calla) in a block whose only out edge goes
// to a block marked with return_block is a tail call.
// Also, if the block is a return block the last call is a
// tail call.
void FindTailCalls(BasicBlock* block, void* data) {
  Generator* gen = data;
  if (!block->return_block) {
    if (block->out_edges.length != 1) {
      return;
    }
    BlockId next_id = block->out_edges.value.w[0];
    BasicBlock* next = VectorGet(&gen->basic_blocks, next_id);
    if (!next->return_block) {
      return;
    }
    // The successor return block runs *after* this block's call.  If it does
    // any real work (e.g. `printf(); fred++;` where the post-increment of the
    // static `fred` lands in the return block) the call is not in tail position
    // and turning it into a jump would drop that work.
    for (IRNode* inst = BasicBlockBegin(next);
         !BasicBlockIsEmpty(next) && inst != BasicBlockEnd(next);
         inst = IRNext(inst)) {
      if (!IsTrivialReturnInstruction(inst)) {
        return;
      }
    }
  }
  // Look for the last call in the block.  If we see a result instruction
  // after the call it's not a tail call.
  for (IRNode* inst = BasicBlockRBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockREnd(block);
       inst = IRPrev(inst)) {
    if (IRIsResult(inst)) {
      // Scalar result instruction.
      IRNode* value = inst->inputs.value.p[0];
      if (value->opcode == IR_OP(calla)) {
        value->flags |= kIRTailCall;
      }
      return;
    }
    if (inst->opcode == IR_OP(memcpy)) {
      IRNode* dest = inst->inputs.value.p[0];
      if (dest->opcode == IR_OP(structreturn)) {
        // Struct result instruction.  Can't do a tail call for this.
        return;
      }
    }
    if (inst->opcode == IR_OP(calla)) {
      inst->flags |= kIRTailCall;
      return;
    }
    // Any other instruction sitting between the call and the return means there
    // is real work after the call, so it is not in tail position.
    if (!IsTrivialReturnInstruction(inst)) {
      return;
    }
  }
}

void TailCallOptimization(Generator* gen) {
  BasicBlockTraverseDominatorTree(gen, gen->entry_block,
                                  FindTailCalls, kTraversePostOrder, gen);
}

