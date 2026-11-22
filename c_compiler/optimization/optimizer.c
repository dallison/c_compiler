//
//  optimizer.c
//  c_compiler_library
//
//  Created by David Allison on 6/12/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "optimizer.h"
#include "basic_block.h"
#include "compiler.h"
#include "type_compare.h"
#include "type_core.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

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
    if ((d & (d - 1)) == 0 && StringEqual(compiler->target_name, "aarch64")) {
      // AArch64 lowers signed power-of-two division directly to 3-4 narrow
      // shifts/add without 64-bit magic multiplication.
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

    case IR_OP(suba): {
      // `(p + x) - p` → `x`.  MemIsIo-style `(uintptr_t)(memory+addr - memory)`
      // becomes the index, so `off > 0xffff` folds away for uint16 addresses.
      if (node->inputs.length != 2) {
        break;
      }
      IRNode* left = node->inputs.value.p[0];
      IRNode* right = node->inputs.value.p[1];
      while (left != NULL &&
             (left->opcode == IR_OP(cast) || left->opcode == IR_OP(mova)) &&
             left->dest == NULL && left->inputs.length == 1) {
        left = left->inputs.value.p[0];
      }
      while (right != NULL &&
             (right->opcode == IR_OP(cast) || right->opcode == IR_OP(mova)) &&
             right->dest == NULL && right->inputs.length == 1) {
        right = right->inputs.value.p[0];
      }
      if (left == NULL || right == NULL) {
        break;
      }
      IRNode* matched_idx = NULL;
      if (left->opcode == IR_OP(adda) && left->inputs.length == 2) {
        for (int s = 0; s < 2; s++) {
          IRNode* cand_base = left->inputs.value.p[s];
          while (cand_base != NULL &&
                 (cand_base->opcode == IR_OP(cast) ||
                  cand_base->opcode == IR_OP(mova)) &&
                 cand_base->dest == NULL && cand_base->inputs.length == 1) {
            cand_base = cand_base->inputs.value.p[0];
          }
          bool same_base = cand_base == right;
          if (!same_base && cand_base != NULL &&
              cand_base->opcode == IR_OP(loada) &&
              right->opcode == IR_OP(loada) &&
              cand_base->inputs.length == 1 && right->inputs.length == 1 &&
              !IRHasSideEffects(cand_base) && !IRHasSideEffects(right)) {
            IRNode* bptr = cand_base->inputs.value.p[0];
            IRNode* rptr = right->inputs.value.p[0];
            while (bptr != NULL && bptr->opcode == IR_OP(cast) &&
                   bptr->inputs.length == 1) {
              bptr = bptr->inputs.value.p[0];
            }
            while (rptr != NULL && rptr->opcode == IR_OP(cast) &&
                   rptr->inputs.length == 1) {
              rptr = rptr->inputs.value.p[0];
            }
            if (bptr == rptr ||
                (bptr != NULL && rptr != NULL &&
                 bptr->opcode == IR_OP(loada) && rptr->opcode == IR_OP(loada) &&
                 bptr->inputs.length == 1 && rptr->inputs.length == 1 &&
                 bptr->inputs.value.p[0] == rptr->inputs.value.p[0])) {
              same_base = true;
            }
          }
          if (same_base) {
            matched_idx = left->inputs.value.p[1 - s];
            break;
          }
        }
      }
      // After inlining MemIsIo the pointer lives in a local:
      //   p = memory + addr;  off = p - memory;
      // Look through loada(p) to the storea's adda without needing memopt CSE
      // of the two memory loads (they often remain distinct loada nodes).
      if (matched_idx == NULL && left->opcode == IR_OP(loada) &&
          left->inputs.length >= 1) {
        IRNode* local = left->inputs.value.p[0];
        for (IRNode* prev = IRPrev(node);
             prev != NULL && prev->block == block; prev = IRPrev(prev)) {
          if (!IRIsStoreOnly(prev) || prev->inputs.length < 2 ||
              prev->inputs.value.p[0] != local) {
            continue;
          }
          IRNode* stored = prev->inputs.value.p[1];
          while (stored != NULL &&
                 (stored->opcode == IR_OP(cast) ||
                  stored->opcode == IR_OP(mova)) &&
                 stored->dest == NULL && stored->inputs.length == 1) {
            stored = stored->inputs.value.p[0];
          }
          if (stored != NULL && stored->opcode == IR_OP(adda) &&
              stored->inputs.length == 2) {
            IRNode* base = stored->inputs.value.p[0];
            IRNode* idx = stored->inputs.value.p[1];
            while (base != NULL &&
                   (base->opcode == IR_OP(cast) ||
                    base->opcode == IR_OP(mova)) &&
                   base->dest == NULL && base->inputs.length == 1) {
              base = base->inputs.value.p[0];
            }
            bool same_base = base == right;
            if (!same_base && base != NULL && right->opcode == IR_OP(loada) &&
                base->opcode == IR_OP(loada) && base->inputs.length >= 1 &&
                right->inputs.length >= 1 &&
                base->inputs.value.p[0] == right->inputs.value.p[0]) {
              same_base = true;
            }
            if (same_base && idx != NULL) {
              matched_idx = idx;
            }
          }
          break;
        }
      }
      if (matched_idx != NULL && matched_idx->type != NULL &&
          node->type != NULL && TypeIsIntegral(matched_idx->type)) {
        IRNode* replacement = matched_idx;
        if (matched_idx->type->size > 0 &&
            matched_idx->type->size < node->type->size &&
            node->type->size <= 8) {
          int diff_bits =
              (int)(node->type->size - matched_idx->type->size) * 8;
          if (IRIsIntConst(matched_idx)) {
            int src_bits = (int)matched_idx->type->size * 8;
            uint64_t mask = (UINT64_C(1) << src_bits) - 1;
            uint64_t val = (uint64_t)IRIntConstValue(matched_idx) & mask;
            if (!TypeIsUnsigned(matched_idx->type) &&
                (val & (UINT64_C(1) << (src_bits - 1))) != 0) {
              val |= ~mask;
            }
            replacement = GeneratorGetIntConstant(gen, node->type, (int64_t)val);
          } else {
            IROpcode ext_op = TypeIsUnsigned(matched_idx->type)
                                  ? IR_OP(zeroextendi)
                                  : IR_OP(signextendi);
            TypeRecord* ext_type = node->type;
            if (!TypeIsIntegral(ext_type)) {
              Type base_t =
                  node->type->size == 8 ? (kTypeLong | kTypeLongLong)
                                        : kTypeInt;
              if (TypeIsUnsigned(matched_idx->type)) {
                base_t |= kTypeUnsigned;
              }
              ext_type = NewTypeRecordWithSize(base_t, kQualPlain);
            }
            replacement = IRSetType(
                NewIR2(ext_op, matched_idx,
                       GeneratorGetIntConstant(gen, ext_type, diff_bits)),
                ext_type);
            BasicBlockEmitBefore(gen, block, replacement, node);
          }
        }
        BasicBlockReplaceInstruction(gen, block, node, replacement);
      }
      break;
    }

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
        break;
      }
      // `(cmp != 0)` is identity when |cmp| is already a 0/1 comparison.
      // SET_CARRY_VALUE(result < 256) is the BBC hot case.  Use IRIsIntConst
      // (not IsIntConstantWithValue) so untyped/odd-sized 0 immediates match.
      if (node->inputs.length == 2) {
        IRNode* lhs = node->inputs.value.p[0];
        IRNode* rhs = node->inputs.value.p[1];
        if (IRIsIntConst(rhs) && IRIntConstValue(rhs) == 0 &&
            IRIsComparison(lhs)) {
          BasicBlockReplaceInstruction(gen, block, node, lhs);
        } else if (IRIsIntConst(lhs) && IRIntConstValue(lhs) == 0 &&
                   IRIsComparison(rhs)) {
          BasicBlockReplaceInstruction(gen, block, node, rhs);
        }
      }
      break;

    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei): {
      // `zext16(loadu16) > 0xffff` is always false, and `((x & 0x1f) + 1) < 1`
      // is always false.  Evaluate conservative non-negative [lo, hi] bounds.
      if (node->inputs.length != 2 || !IRIsIntConst(node->inputs.value.p[1])) {
        break;
      }
      IRNode* cur = node->inputs.value.p[0];
      int64_t add_acc = 0;
      int64_t base_lo = -1;
      int64_t base_hi = -1;
      int64_t fallback_lo = -1;
      int64_t fallback_hi = -1;
      TypeRecord* cmp_type = cur != NULL ? cur->type : NULL;
      if (cmp_type == NULL || !TypeIsIntegral(cmp_type) ||
          TypeIsInt128(cmp_type) || TypeIsBitInt(cmp_type) ||
          (cmp_type->size != 4 && cmp_type->size != 8)) {
        break;
      }
      int64_t type_max = cmp_type->size == 8 ? INT64_MAX : INT32_MAX;
      for (int depth = 0; cur != NULL && depth < 8; depth++) {
        IRNode* in0 = cur->inputs.length >= 1 ? cur->inputs.value.p[0] : NULL;
        if ((cur->opcode == IR_OP(cast) || cur->opcode == IR_OP(movi) ||
             cur->opcode == IR_OP(mova)) &&
            cur->inputs.length == 1 && cur->type != NULL &&
            in0 != NULL && in0->type != NULL &&
            (TypeIsIntegral(in0->type) || TypeIsPointer(in0->type)) &&
            in0->type->size == cur->type->size) {
          cur = in0;
          continue;
        }
        if (cur->opcode == IR_OP(zeroextendi) && cur->inputs.length == 2 &&
            in0 != NULL && in0->type != NULL && cur->type != NULL &&
            TypeIsIntegral(in0->type) && in0->type->size > 0 &&
            in0->type->size < cur->type->size) {
          int64_t cap = -1;
          if (in0->type->size == 1) {
            cap = 255;
          } else if (in0->type->size == 2) {
            cap = 65535;
          } else if (in0->type->size == 4) {
            cap = INT64_C(4294967295);
          }
          if (cap >= 0 && cap <= type_max - add_acc) {
            if (add_acc == 0) {
              if (fallback_lo < 0 || cap < fallback_hi) {
                fallback_lo = 0;
                fallback_hi = cap;
              }
              if (cap < type_max) {
                type_max = cap;
              }
              cur = in0;
              continue;
            }
            base_lo = 0;
            base_hi = cap;
            break;
          }
        }
        if (cur->opcode == IR_OP(signextendi) &&
            cur->inputs.length == 2 && in0 != NULL && in0->type != NULL &&
            cur->type != NULL && TypeIsIntegral(in0->type) &&
            in0->type->size > 0 && in0->type->size < cur->type->size &&
            add_acc == 0) {
          if (in0->type->size == 4) {
            type_max = INT32_MAX;
          } else if (in0->type->size == 2) {
            type_max = INT16_MAX;
          } else if (in0->type->size == 1) {
            type_max = INT8_MAX;
          }
          cur = in0;
          continue;
        }
        if (cur->opcode == IR_OP(zeroextendi) && cur->inputs.length == 2 &&
            IRIsIntConst(cur->inputs.value.p[1]) && cur->type != NULL &&
            in0 != NULL && in0->type != NULL &&
            in0->type->size == cur->type->size) {
          int64_t m = IRIntConstValue(cur->inputs.value.p[1]);
          if (m >= 0 && m <= type_max - add_acc) {
            base_lo = 0;
            base_hi = m;
          }
          break;
        }
        if (cur->opcode == IR_OP(andi) && cur->inputs.length == 2 &&
            IRIsIntConst(cur->inputs.value.p[1])) {
          int64_t m = IRIntConstValue(cur->inputs.value.p[1]);
          if (m >= 0 && m <= type_max - add_acc) {
            base_lo = 0;
            base_hi = m;
          }
          break;
        }
        if (cur->opcode == IR_OP(addi) && cur->inputs.length == 2 &&
            IRIsIntConst(cur->inputs.value.p[1]) &&
            !TypeIsUnsigned(cur->type)) {
          int64_t c = IRIntConstValue(cur->inputs.value.p[1]);
          if (c >= 0 && c <= type_max - add_acc) {
            add_acc += c;
            cur = in0;
            continue;
          }
          break;
        }
        if (IRIsLoadOnly(cur) && !IRHasSideEffects(cur) &&
            cur->inputs.length == 1 && in0 != NULL &&
            in0->opcode == IR_OP(ssavar)) {
          IRNode* var = in0;
          Symbol* sym = ((IRVariable*)var)->symbol;
          if (sym != NULL && !sym->flags.address_taken &&
              !sym->flags.is_argument && !TypeIsVolatile(sym->type) &&
              !TypeIsArray(sym->type) &&
              !SymbolIsAnonymousUnionMemberView(sym)) {
            IRNode* sole_store = NULL;
            bool ok = true;
            for (size_t u = 0; u < var->outputs.length; u++) {
              IRNode* usr = var->outputs.value.p[u];
              if (IRIsStoreOnly(usr) && usr->inputs.length == 2 &&
                  usr->inputs.value.p[0] == var && usr->dest == NULL) {
                if (sole_store != NULL) {
                  ok = false;
                  break;
                }
                sole_store = usr;
              } else if (usr->opcode != IR_OP(phi) &&
                         (!IRIsLoadOnly(usr) || usr->inputs.length != 1 ||
                          usr->inputs.value.p[0] != var)) {
                ok = false;
                break;
              }
            }
            IRNode* st_val =
                sole_store != NULL ? sole_store->inputs.value.p[1] : NULL;
            if (ok && sole_store != NULL && sole_store->block != NULL &&
                cur->block != NULL &&
                BitSetContains(&cur->block->dominators,
                               sole_store->block->block_id) &&
                st_val != NULL && st_val->type != NULL &&
                cur->type != NULL &&
                st_val->type->size == cur->type->size) {
              bool precedes = sole_store->block != cur->block;
              if (!precedes) {
                for (IRNode* p = sole_store;
                     p != NULL && p->block == cur->block; p = IRNext(p)) {
                  if (p == cur) {
                    precedes = true;
                    break;
                  }
                }
              }
              if (precedes) {
                cur = st_val;
                continue;
              }
            }
          }
        }
        if (cur->opcode == IR_OP(zeroextendi) && cur->inputs.length >= 1) {
          IRNode* zsrc = cur->inputs.value.p[0];
          if (zsrc != NULL &&
              (zsrc->opcode == IR_OP(loadu8) || zsrc->opcode == IR_OP(load8))) {
            if (255 <= type_max - add_acc) {
              base_lo = 0;
              base_hi = 255;
            }
            break;
          }
          if (zsrc != NULL && (zsrc->opcode == IR_OP(loadu16) ||
                               zsrc->opcode == IR_OP(load16))) {
            if (65535 <= type_max - add_acc) {
              base_lo = 0;
              base_hi = 65535;
            }
            break;
          }
        }
        if (cur->opcode == IR_OP(loadu8) ||
            (cur->opcode == IR_OP(load8) && cur->type != NULL &&
             TypeIsUnsigned(cur->type))) {
          if (255 <= type_max - add_acc) {
            base_lo = 0;
            base_hi = 255;
          }
        } else if (cur->opcode == IR_OP(loadu16) ||
                   (cur->opcode == IR_OP(load16) && cur->type != NULL &&
                    TypeIsUnsigned(cur->type))) {
          if (65535 <= type_max - add_acc) {
            base_lo = 0;
            base_hi = 65535;
          }
        }
        break;
      }
      int64_t lo;
      int64_t hi;
      if (base_lo >= 0 && base_hi >= base_lo) {
        lo = base_lo + add_acc;
        hi = base_hi + add_acc;
      } else if (fallback_lo >= 0 && fallback_hi >= fallback_lo) {
        lo = fallback_lo;
        hi = fallback_hi;
      } else {
        break;
      }
      int64_t lim = IRIntConstValue(node->inputs.value.p[1]);
      if (IRComparisonIsUnsigned(node)) {
        if (lim >= 0) {
          uint64_t ulim = (uint64_t)lim;
          uint64_t ulo = (uint64_t)lo;
          uint64_t uhi = (uint64_t)hi;
          if ((node->opcode == IR_OP(cmplti) && ulo >= ulim) ||
              (node->opcode == IR_OP(cmplei) && ulo > ulim) ||
              (node->opcode == IR_OP(cmpgti) && uhi <= ulim) ||
              (node->opcode == IR_OP(cmpgei) && uhi < ulim)) {
            BasicBlockReplaceInstruction(
                gen, block, node, GeneratorGetIntConstant(gen, node->type, 0));
          } else if ((node->opcode == IR_OP(cmplti) && uhi < ulim) ||
                     (node->opcode == IR_OP(cmplei) && uhi <= ulim) ||
                     (node->opcode == IR_OP(cmpgti) && ulo > ulim) ||
                     (node->opcode == IR_OP(cmpgei) && ulo >= ulim)) {
            BasicBlockReplaceInstruction(
                gen, block, node, GeneratorGetIntConstant(gen, node->type, 1));
          }
        }
      } else {
        if ((node->opcode == IR_OP(cmplti) && lo >= lim) ||
            (node->opcode == IR_OP(cmplei) && lo > lim) ||
            (node->opcode == IR_OP(cmpgti) && hi <= lim) ||
            (node->opcode == IR_OP(cmpgei) && hi < lim)) {
          BasicBlockReplaceInstruction(
              gen, block, node, GeneratorGetIntConstant(gen, node->type, 0));
        } else if ((node->opcode == IR_OP(cmplti) && hi < lim) ||
                   (node->opcode == IR_OP(cmplei) && hi <= lim) ||
                   (node->opcode == IR_OP(cmpgti) && lo > lim) ||
                   (node->opcode == IR_OP(cmpgei) && lo >= lim)) {
          BasicBlockReplaceInstruction(
              gen, block, node, GeneratorGetIntConstant(gen, node->type, 1));
        }
      }
      break;
    }

    case IR_OP(setbit): {
      // Single-bit flag updates → and/or (+lsl).  Rewriting Z (cmpeqi) as
      // well as N (aligned bit-test) lets memopt coalesce the two stores
      // into one RMW.  Keep a separate lsli for Z; folding it into orr's
      // oplsl marker is unsafe until RA preserves that shift.
      if (node->inputs.length < 4 || !IRIsIntConst(node->inputs.value.p[2]) ||
          !IRIsIntConst(node->inputs.value.p[3])) {
        break;
      }
      int bit_pos = (int)IRIntConstValue(node->inputs.value.p[2]);
      int bit_size = (int)IRIntConstValue(node->inputs.value.p[3]);
      if (bit_size != 1 || bit_pos < 0 || bit_pos >= 63) {
        break;
      }
      IRNode* word = node->inputs.value.p[0];
      TypeRecord* ty = node->type != NULL ? node->type : word->type;
      // Bitfield storage units are sometimes typed as the containing
      // aggregate's word without TypeIsIntegral; size is enough here.
      if (ty == NULL || ty->size <= 0 || ty->size > 8) {
        break;
      }
      // Integral immediates only (struct-typed setbit would yield consta).
      // Results stay typed like |word| so memopt can forward the load.
      TypeRecord* imm_ty = NewTypeRecordWithSize(
          ty->size <= 4 ? kTypeInt : (kTypeLong | kTypeLongLong), kQualPlain);
      TypeRecord* result_ty = word->type != NULL ? word->type : ty;
      uint64_t field = UINT64_C(1) << bit_pos;
      int64_t clear_imm =
          ty->size <= 4 ? (int64_t)(uint32_t)(~(uint32_t)field)
                        : (int64_t)(~field);
      IRNode* bit_value = node->inputs.value.p[1];
      IRNode* value = bit_value;
      while (value != NULL && value->opcode == IR_OP(zeroextendi) &&
             value->inputs.length >= 1) {
        value = value->inputs.value.p[0];
      }
      if (value == NULL) {
        break;
      }

      // `flags.s = (v & 0x80) != 0`: and already isolates bit 7.
      if (value->opcode == IR_OP(cmpnei) && value->inputs.length >= 2 &&
          IRIsIntConst(value->inputs.value.p[1]) &&
          IRIntConstValue(value->inputs.value.p[1]) == 0) {
        IRNode* masked = value->inputs.value.p[0];
        while (masked != NULL && masked->opcode == IR_OP(zeroextendi) &&
               masked->inputs.length >= 1) {
          masked = masked->inputs.value.p[0];
        }
        if (masked != NULL && masked->opcode == IR_OP(andi) &&
            masked->inputs.length >= 2) {
          IRNode* mask_node = NULL;
          if (IRIsIntConst(masked->inputs.value.p[1])) {
            mask_node = masked->inputs.value.p[1];
          } else if (IRIsIntConst(masked->inputs.value.p[0])) {
            mask_node = masked->inputs.value.p[0];
          }
          if (mask_node != NULL &&
              (uint64_t)IRIntConstValue(mask_node) == field) {
            IRNode* clear = GeneratorGetIntConstant(gen, imm_ty, clear_imm);
            IRNode* cleared = EmitBinBefore(gen, block, node, IR_OP(andi), word,
                                            clear, result_ty);
            IRNode* result = EmitBinBefore(gen, block, node, IR_OP(ori), cleared,
                                           masked, result_ty);
            BasicBlockReplaceInstruction(gen, block, node, result);
            break;
          }
        }
        // `(cmp != 0)` around another compare → use the inner compare.
        if (masked != NULL && IRIsComparison(masked)) {
          value = masked;
          bit_value = masked;
        }
      }

      // Any 0/1 comparison into a bit: clear the field then OR the (shifted)
      // compare.  Covers Z (`== 0`) and C (`result < 256`) after peeling
      // cmpnei(cmp, 0) above.
      if (IRIsComparison(value)) {
        IRNode* placed = bit_value;
        if (bit_pos != 0) {
          IRNode* shift = GeneratorGetIntConstant(gen, imm_ty, bit_pos);
          placed = EmitBinBefore(gen, block, node, IR_OP(lsli), bit_value,
                                 shift, result_ty);
        }
        IRNode* clear = GeneratorGetIntConstant(gen, imm_ty, clear_imm);
        IRNode* cleared = EmitBinBefore(gen, block, node, IR_OP(andi), word,
                                        clear, result_ty);
        IRNode* result = EmitBinBefore(gen, block, node, IR_OP(ori), cleared,
                                       placed, result_ty);
        BasicBlockReplaceInstruction(gen, block, node, result);
        break;
      }
      break;
    }

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

// _Unwind_Resume continues unwinding from the register state saved when the
// landing pad was installed, so the calling frame's spill slots must still be
// intact when it runs.
static bool CallNeedsLiveFrame(IRNode* call) {
  if (call->inputs.length == 0) return false;
  Symbol* callee = IRGetVariableSymbol(call->inputs.value.p[0]);
  return callee != NULL && callee->name.value != NULL &&
         strcmp(callee->name.value, "_Unwind_Resume") == 0;
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
      // Scalar result instruction.  The call returning the value is in tail
      // position only if nothing but bookkeeping runs between it and here
      // (`g = f(); return g;` stores the value after the call).
      IRNode* value = inst->inputs.value.p[0];
      if (value->opcode != IR_OP(calla) || CallNeedsLiveFrame(value)) {
        return;
      }
      for (IRNode* between = IRPrev(inst);; between = IRPrev(between)) {
        if (between == BasicBlockREnd(block)) {
          return;
        }
        if (between == value) {
          break;
        }
        if (between->opcode != IR_OP(loc) &&
            !IsTrivialReturnInstruction(between)) {
          return;
        }
      }
      value->flags |= kIRTailCall;
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
      if (!CallNeedsLiveFrame(inst)) inst->flags |= kIRTailCall;
      return;
    }
    // Any other instruction sitting between the call and the return means there
    // is real work after the call, so it is not in tail position.
    if (!IsTrivialReturnInstruction(inst)) {
      return;
    }
  }
}

static void ThreadTailCallMergeReturns(Generator* gen) {
  for (size_t b = 0; b < gen->basic_blocks.length; b++) {
    BasicBlock* merge = gen->basic_blocks.value.p[b];
    if (merge == NULL || merge == gen->entry_block ||
        BasicBlockIsEmpty(merge) || merge->out_edges.length != 1) {
      continue;
    }
    BasicBlock* ret_blk =
        VectorGet(&gen->basic_blocks, merge->out_edges.value.w[0]);
    if (ret_blk == NULL || !ret_blk->return_block) {
      continue;
    }
    bool trivial_ret = true;
    for (IRNode* inst = BasicBlockBegin(ret_blk);
         !BasicBlockIsEmpty(ret_blk) && inst != BasicBlockEnd(ret_blk);
         inst = IRNext(inst)) {
      if (!IsTrivialReturnInstruction(inst)) {
        trivial_ret = false;
        break;
      }
    }
    if (!trivial_ret) {
      continue;
    }

    IRNode* merge_label = BasicBlockBegin(merge);
    if (merge_label->opcode != IR_OP(label)) {
      continue;
    }
    IRNode* ld = NULL;
    IRNode* res = NULL;
    IRNode* term = NULL;
    bool ok = true;
    for (IRNode* inst = IRNext(merge_label); inst != BasicBlockEnd(merge);
         inst = IRNext(inst)) {
      if (inst->opcode == IR_OP(loc)) {
        continue;
      }
      if (ld == NULL && IRIsLoadOnly(inst) && inst->inputs.length == 1 &&
          inst->outputs.length == 1) {
        ld = inst;
      } else if (ld != NULL && res == NULL && IRIsResult(inst) &&
                 inst->inputs.length == 1 && inst->inputs.value.p[0] == ld) {
        res = inst;
      } else if (res != NULL && term == NULL && inst->opcode == IR_OP(bra) &&
                 inst->inputs.length == 1 &&
                 IRNext(inst) == BasicBlockEnd(merge)) {
        term = inst;
      } else {
        ok = false;
        break;
      }
    }
    if (!ok || ld == NULL || res == NULL || term == NULL) {
      continue;
    }
    IRNode* ret_label = term->inputs.value.p[0];
    if (ret_label == NULL || ret_label->opcode != IR_OP(label)) {
      continue;
    }
    IRNode* var = ld->inputs.value.p[0];
    if (var == NULL || var->opcode != IR_OP(tempvar) || var->type == NULL) {
      continue;
    }
    Symbol* sym = ((IRVariable*)var)->symbol;
    if (sym == NULL || sym->flags.address_taken || TypeIsVolatile(sym->type)) {
      continue;
    }
    for (size_t u_idx = 0; u_idx < var->outputs.length; u_idx++) {
      IRNode* u = var->outputs.value.p[u_idx];
      if (u == ld) {
        continue;
      }
      if (!IRIsStoreOnly(u) || u->inputs.length != 2 ||
          u->inputs.value.p[0] != var) {
        ok = false;
        break;
      }
    }
    if (!ok) {
      continue;
    }

    for (size_t e = 0; e < merge->in_edges.length; e++) {
      BasicBlock* pred =
          VectorGet(&gen->basic_blocks, merge->in_edges.value.w[e]);
      if (pred == NULL || pred == merge || BasicBlockIsEmpty(pred) ||
          pred->out_edges.length != 1 ||
          pred->out_edges.value.w[0] != merge->block_id) {
        continue;
      }
      IRNode* br = BasicBlockRBegin(pred);
      if (br->opcode != IR_OP(bra) || br->inputs.length != 1 ||
          br->inputs.value.p[0] != merge_label) {
        continue;
      }
      IRNode* cur = IRPrev(br);
      while (cur != BasicBlockREnd(pred) && cur->opcode == IR_OP(loc)) {
        cur = IRPrev(cur);
      }
      if (cur == BasicBlockREnd(pred) || !IRIsStoreOnly(cur) ||
          cur->outputs.length != 0 || cur->inputs.length != 2 ||
          cur->inputs.value.p[0] != var) {
        continue;
      }
      IRNode* st = cur;
      cur = IRPrev(st);
      while (cur != BasicBlockREnd(pred) && cur->opcode == IR_OP(loc)) {
        cur = IRPrev(cur);
      }
      if (cur == BasicBlockREnd(pred) || cur != st->inputs.value.p[1] ||
          cur->opcode != IR_OP(calla) || CallNeedsLiveFrame(cur) ||
          cur->outputs.length != 1 || cur->dest != NULL || cur->type == NULL ||
          cur->type->size != var->type->size ||
          TypeIsUnsigned(cur->type) != TypeIsUnsigned(var->type)) {
        continue;
      }
      IRNode* new_res = IRSetType(NewIR1(res->opcode, cur), res->type);
      BasicBlockEmitBefore(gen, pred, new_res, st);
      BasicBlockRemoveInstruction(gen, pred, st);
      IRReplaceInput(br, 0, ret_label);
      br->flags |= (term->flags & kIRReturnJump);
      pred->out_edges.value.w[0] = ret_blk->block_id;
    }
  }
}

void TailCallOptimization(Generator* gen) {
  ThreadTailCallMergeReturns(gen);
  BasicBlockTraverseDominatorTree(gen, gen->entry_block,
                                  FindTailCalls, kTraversePostOrder, gen);
}

