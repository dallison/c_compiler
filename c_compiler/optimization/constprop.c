//
//  constprop.c
//  c_compiler_library
//
//  Created by David Allison on 7/4/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "constprop.h"
#include "map.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>

// Constant propagation and folding.
//
// Process each basic block in dominator tree pre-order.  For each instruction
// look at the opcode:
// * store instruction to a variable: if the value being stored is
//   a constant, keep variable vs constant in map.
// * load of a variable: search for variable in map and if found,
//   replace load with constant itself.
// * arithmetic instruction: if all operands are constant, do calculation
//   and replace instruction with new constant.
//
// The IR must be in SSA form for this to work.

typedef struct {
  // Map of IRNode* for variable vs IRNode for constant.
  Map constants;
} ConstantPropagator;

static bool IsVariableReference(IRNode* inst) {
  switch (inst->opcode) {
    // Only propagate through non-SSA stack variables and arguments.
    // SSA names and phis can be updated across loop iterations.
    case IR_OP(localvar):
    case IR_OP(argument):
      return true;
    default:
      return false;
  }
}

static IRNode* SignExtendedConstant(Generator* gen, IRConstant* con, int bits) {
  int64_t value = con->value.ivalue;
  if (bits < 64) {
    value <<= 64 - bits;
    value >>= 64 - bits;
  }
  return GeneratorGetIntConstant(gen, con->base.type, value);
}

static IRNode* TruncatedConstant(Generator* gen, IRConstant* con, int bits) {
  int64_t value = con->value.ivalue;
  if (bits < 64) {
    int64_t mask = (1LL << bits) - 1;
    value &= mask;
  }
  return GeneratorGetIntConstant(gen, con->base.type, value);
}

static bool AllConstantInputs(IRNode* node) {
  for (size_t i = 0; i < node->inputs.length; i++) {
    IRNode* input = node->inputs.value.p[i];
    if (input == NULL || !IRIsIntConst(input)) {
      return false;
    }
  }
  return true;
}

static int64_t FoldZeroExtension(IRNode* inst, int64_t value,
                                 int64_t operand) {
  IRNode* source = inst->inputs.value.p[0];
  if (source->type == NULL || inst->type == NULL ||
      source->type->size == inst->type->size) {
    // Same-width zeroextendi nodes are explicit bit masks.
    return value & operand;
  }
  int bytes = source->type->size;
  if (inst->type->size < bytes) {
    bytes = inst->type->size;
  }
  int bits = bytes * 8;
  uint64_t mask = bits >= 64 ? UINT64_MAX : ((1ULL << bits) - 1);
  return (int64_t)((uint64_t)value & mask);
}

static bool FoldSignExtension(IRNode* inst, int64_t value, int64_t operand,
                              int64_t* result) {
  IRNode* source = inst->inputs.value.p[0];
  if (source->type != NULL && inst->type != NULL &&
      source->type->size != inst->type->size) {
    int bytes = source->type->size;
    if (inst->type->size < bytes) {
      bytes = inst->type->size;
    }
    int bits = bytes * 8;
    if (bits >= 64) {
      *result = value;
      return true;
    }
    uint64_t mask = (1ULL << bits) - 1;
    uint64_t extended = (uint64_t)value & mask;
    if ((extended & (1ULL << (bits - 1))) != 0) {
      extended |= ~mask;
    }
    *result = (int64_t)extended;
    return true;
  }
  if (operand < 0 || operand >= 64) {
    return false;
  }
  *result = (int64_t)((uint64_t)value << operand) >> operand;
  return true;
}


static void PropagateAddConstant(Generator* gen, BasicBlock* block, IRNode* inst) {
  if (inst->inputs.length < 2 ||
      inst->inputs.value.p[0] == NULL ||
      inst->inputs.value.p[1] == NULL ||
      !IRIsIntConst(inst->inputs.value.p[1])) {
    return;
  }
  IRNode* src = inst->inputs.value.p[0];
  if (src->outputs.length != 1 || src->inputs.length < 2 ||
      !IRIsIntConst(src->inputs.value.p[1]) ||
      IRCheckpointBetween(src, inst)) {
    return;
  }

  int64_t src_value = IRIntConstValue(src->inputs.value.p[1]);
  int64_t my_value = IRIntConstValue(inst->inputs.value.p[1]);
  int64_t new_value;
  if (src->opcode == IR_OP(addi)) {
    if (__builtin_add_overflow(src_value, my_value, &new_value)) {
      return;
    }
  } else if (src->opcode == IR_OP(subi)) {
    if (__builtin_sub_overflow(my_value, src_value, &new_value)) {
      return;
    }
  } else {
    return;
  }
  if (new_value == INT64_MIN) {
    return;
  }

  IRReplaceInput(inst, 0, src->inputs.value.p[0]);
  if (new_value < 0) {
    inst->opcode = IR_OP(subi);
    new_value = -new_value;
  }
  IRNode* value_node = inst->inputs.value.p[1];
  value_node =
      GeneratorEmitConstant(gen, NewIntIRConstant(value_node->type, new_value));
  IRReplaceInput(inst, 1, value_node);
  BasicBlockRemoveInstruction(gen, block, src);
}

static void PropagateSubConstant(Generator* gen, BasicBlock* block, IRNode* inst) {
  if (inst->inputs.length < 2 ||
      inst->inputs.value.p[0] == NULL ||
      inst->inputs.value.p[1] == NULL ||
      !IRIsIntConst(inst->inputs.value.p[1])) {
    return;
  }
  // We are subtracting a constant.
  IRNode* src = inst->inputs.value.p[0];
  if (src->outputs.length != 1 || src->inputs.length < 2 ||
      !IRIsIntConst(src->inputs.value.p[1]) ||
      IRCheckpointBetween(src, inst)) {
    return;
  }

  int64_t src_value = IRIntConstValue(src->inputs.value.p[1]);
  int64_t my_value = IRIntConstValue(inst->inputs.value.p[1]);
  int64_t new_value;
  if (src->opcode == IR_OP(addi)) {
    if (__builtin_sub_overflow(my_value, src_value, &new_value)) {
      return;
    }
  } else if (src->opcode == IR_OP(subi)) {
    if (__builtin_add_overflow(my_value, src_value, &new_value)) {
      return;
    }
  } else {
    return;
  }
  if (new_value == INT64_MIN) {
    return;
  }

  IRReplaceInput(inst, 0, src->inputs.value.p[0]);
  if (new_value < 0) {
    inst->opcode = IR_OP(addi);
    new_value = -new_value;
  }
  IRNode* value_node = inst->inputs.value.p[1];
  value_node =
      GeneratorEmitConstant(gen, NewIntIRConstant(value_node->type, new_value));
  IRReplaceInput(inst, 1, value_node);
  BasicBlockRemoveInstruction(gen, block, src);
}
 
// General case for a single constant operation.
#define PROP_CONST_OP(ir_opcode, op) \
  if (inst->inputs.length >= 2 && \
      inst->inputs.value.p[0] != NULL && \
      inst->inputs.value.p[1] != NULL && \
      IRIsIntConst(inst->inputs.value.p[1])) {\
    IRNode* src = inst->inputs.value.p[0];\
    if (src->opcode == IR_OP(ir_opcode) && \
        !IRCheckpointBetween(src, inst) && \
        IRIsIntConst(src->inputs.value.p[1])) {\
      if (src->outputs.length == 1) {\
        IRReplaceInput(inst, 0, src->inputs.value.p[0]);\
        int64_t src_value = IRIntConstValue(src->inputs.value.p[1]);\
        int64_t my_value = IRIntConstValue(inst->inputs.value.p[1]);\
        int64_t new_value = src_value op my_value;\
        IRNode* value_node = inst->inputs.value.p[1];\
        value_node = GeneratorEmitConstant(gen, NewIntIRConstant(value_node->type, new_value));\
        IRReplaceInput(inst, 1, value_node);\
        BasicBlockRemoveInstruction(gen, block, src);\
      }\
    }\
  }

static void PropagateDivConstant(Generator* gen, BasicBlock* block,
                                 IRNode* inst) {
  if (inst->inputs.length < 2 || !IRIsIntConst(inst->inputs.value.p[1])) {
    return;
  }
  IRNode* src = inst->inputs.value.p[0];
  if (src == NULL || src->opcode != IR_OP(divi) ||
      src->outputs.length != 1 || src->inputs.length < 2 ||
      !IRIsIntConst(src->inputs.value.p[1]) ||
      IRCheckpointBetween(src, inst)) {
    return;
  }

  int64_t src_value = IRIntConstValue(src->inputs.value.p[1]);
  int64_t my_value = IRIntConstValue(inst->inputs.value.p[1]);
  int64_t combined;
  if (src_value == 0 || my_value == 0 ||
      __builtin_mul_overflow(src_value, my_value, &combined) ||
      combined == 0) {
    return;
  }

  IRReplaceInput(inst, 0, src->inputs.value.p[0]);
  IRNode* value_node = inst->inputs.value.p[1];
  value_node =
      GeneratorEmitConstant(gen, NewIntIRConstant(value_node->type, combined));
  IRReplaceInput(inst, 1, value_node);
  BasicBlockRemoveInstruction(gen, block, src);
}

static void PropagateShiftConstant(Generator* gen, BasicBlock* block,
                                   IRNode* inst, IROpcode opcode) {
  if (inst->inputs.length < 2 || !IRIsIntConst(inst->inputs.value.p[1])) {
    return;
  }
  IRNode* src = inst->inputs.value.p[0];
  if (src == NULL || src->opcode != opcode || src->outputs.length != 1 ||
      src->inputs.length < 2 || !IRIsIntConst(src->inputs.value.p[1]) ||
      IRCheckpointBetween(src, inst)) {
    return;
  }

  int64_t src_count = IRIntConstValue(src->inputs.value.p[1]);
  int64_t my_count = IRIntConstValue(inst->inputs.value.p[1]);
  int64_t combined;
  int width = inst->type == NULL ? 0 : inst->type->size * 8;
  if (src_count < 0 || my_count < 0 ||
      __builtin_add_overflow(src_count, my_count, &combined) ||
      combined >= width) {
    return;
  }

  IRReplaceInput(inst, 0, src->inputs.value.p[0]);
  IRNode* value_node = inst->inputs.value.p[1];
  value_node =
      GeneratorEmitConstant(gen, NewIntIRConstant(value_node->type, combined));
  IRReplaceInput(inst, 1, value_node);
  BasicBlockRemoveInstruction(gen, block, src);
}

static void PropagateConstants(Generator* gen, ConstantPropagator* p,
                                  BasicBlock* block) {
  IRNode* next = NULL;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = next) {
    next = IRNext(inst);
    if (IRAsmClobbersMemory(inst) || IRIsObservableCheckpoint(inst)) {
      // Values cached for stack variables may be stale after inline assembly
      // that can access arbitrary memory.
      MapClear(&p->constants);
    }
    if (inst->dest != NULL) {
      continue;
    }
    switch (inst->opcode) {
      case IR_OP(store32):
      case IR_OP(store8):
      case IR_OP(store16):
      case IR_OP(store64):
        if (inst->inputs.length >= 1 &&
            inst->inputs.value.p[0] != NULL &&
            IsVariableReference(inst->inputs.value.p[0])) {
          IRNode* var = inst->inputs.value.p[0];
          if (TypeIsVolatile(var->type)) {
            // Volatile prevents this optimization.
            break;
          }
          if (IRIsIntConst(inst->inputs.value.p[1])) {
            // Store of a constant to a variable.
            IRNode* val = inst->inputs.value.p[1];
            assert(!TypeIsVoid(val->type));
            MapKeyValue kv = {.key.p = var, .value.p = val};
            MapInsert(&p->constants, kv);
          } else {
            // Non-constant store invalidates any prior constant value.
            MapKeyType key = {.p = var};
            MapRemove(&p->constants, key);
          }
        }
        break;
        
      case IR_OP(load32):   // Load signed 32-bit from [op0]
      case IR_OP(load8):   // Load signed 8-bit from [op0]
      case IR_OP(load64):   // Load 64-bit from [op0]
      case IR_OP(load16):   // Load 16-bit from [op0]
      case IR_OP(loadu32):  // Load unsigned 32-bit from [op0]
      case IR_OP(loadu8):  // Load unsigned 8-bit from [op0]
      case IR_OP(loadu16):  // Load unsigned 16-bit from [op0]
        if (inst->inputs.length >= 1 &&
            inst->inputs.value.p[0] != NULL &&
            IsVariableReference(inst->inputs.value.p[0])) {
          IRNode* var = inst->inputs.value.p[0];
          IRConstant* con = MapFindPointerKey(&p->constants, var);
          if (con == NULL) {
            continue;
          }
          IRNode* const_inst = NULL;
          switch (inst->opcode) {
            case IR_OP(load32):   // Load signed 32-bit from [op0]
              const_inst = SignExtendedConstant(gen, con, 32);
              break;
            case IR_OP(load8):   // Load signed 8-bit from [op0]
              const_inst = SignExtendedConstant(gen, con, 8);
              break;
            case IR_OP(load64):   // Load 64-bit from [op0]
              const_inst = SignExtendedConstant(gen, con, 64);
              break;
            case IR_OP(load16):   // Load 16-bit from [op0]
              const_inst = SignExtendedConstant(gen, con, 16);
              break;
            case IR_OP(loadu32):  // Load unsigned 32-bit from [op0]
              const_inst = TruncatedConstant(gen, con, 32);
              break;
            case IR_OP(loadu8):  // Load unsigned 8-bit from [op0]
              const_inst = TruncatedConstant(gen, con, 8);
              break;
            case IR_OP(loadu16):  // Load unsigned 16-bit from [op0]
              const_inst = TruncatedConstant(gen, con, 16);
              break;
            // case IR_OP(loadf):   // Load 32-bit float from [op0]
            //case IR_OP(loadd):   // Load 64-bit float from [op0]
            default:
              assert(false);
              break;
          }
          assert(const_inst != NULL);
          // Retarget all uses of inst with const_inst.
          GeneratorReplaceInstruction(gen, inst, const_inst);
          BasicBlockRemoveInstruction(gen, block, inst);
        }
        break;

#define FOLD_BINARY(op) \
  if (AllConstantInputs(inst)) { \
    int64_t lhs = IRIntConstValue(inst->inputs.value.p[0]); \
    int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]); \
    IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, lhs op rhs); \
    GeneratorReplaceInstruction(gen, inst, const_inst); \
    BasicBlockRemoveInstruction(gen, block, inst); \
    continue; \
  }

#define FOLD_DIV(op) \
        if (AllConstantInputs(inst)) { \
          int64_t lhs = IRIntConstValue(inst->inputs.value.p[0]); \
          int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]); \
          if (rhs != 0) {\
            IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, lhs op rhs); \
            GeneratorReplaceInstruction(gen, inst, const_inst); \
            BasicBlockRemoveInstruction(gen, block, inst); \
            continue; \
          } \
        }
#define FOLD_UNARY(op) \
        if (AllConstantInputs(inst)) { \
          int64_t sub = IRIntConstValue(inst->inputs.value.p[0]); \
          IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, op sub); \
          GeneratorReplaceInstruction(gen, inst, const_inst); \
          BasicBlockRemoveInstruction(gen, block, inst); \
          continue; \
        }

      case IR_OP(addi):
        FOLD_BINARY(+);
        PropagateAddConstant(gen, block, inst);
        break;
      case IR_OP(subi):
        FOLD_BINARY(-);
        PropagateSubConstant(gen, block, inst);
        break;
      case IR_OP(muli):
        FOLD_BINARY(*);
        PROP_CONST_OP(muli, *);
        break;
      case IR_OP(divi):
        FOLD_DIV(/);
        PropagateDivConstant(gen, block, inst);
        break;
      case IR_OP(modi):
        FOLD_DIV(%);
        break;
      case IR_OP(xori):
        FOLD_BINARY(^);
        PROP_CONST_OP(xori, ^);
       break;
      case IR_OP(andi):
        FOLD_BINARY(&);
        PROP_CONST_OP(andi, &);
        break;
      case IR_OP(ori):
        FOLD_BINARY(|);
        PROP_CONST_OP(ori, |);
        break;
      case IR_OP(lsri):
         if (AllConstantInputs(inst)) {
           uint64_t lhs = IRIntConstValue(inst->inputs.value.p[0]);
           int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]);
           IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, lhs >> rhs);
           GeneratorReplaceInstruction(gen, inst, const_inst);
           BasicBlockRemoveInstruction(gen, block, inst);
           continue;
         }
         PropagateShiftConstant(gen, block, inst, IR_OP(lsri));
         break;
      case IR_OP(asri):
        FOLD_BINARY(>>);
        PropagateShiftConstant(gen, block, inst, IR_OP(asri));
        break;
      case IR_OP(lsli):
        FOLD_BINARY(<<);
        PropagateShiftConstant(gen, block, inst, IR_OP(lsli));
        break;
      case IR_OP(cmpeqi):
        FOLD_BINARY(==);
        break;
      case IR_OP(cmpnei):
        FOLD_BINARY(!=);
        break;
      case IR_OP(cmplti):
        FOLD_BINARY(<);
        break;
      case IR_OP(cmplei):
        FOLD_BINARY(<=);
        break;
      case IR_OP(cmpgti):
        FOLD_BINARY(>);
        break;
      case IR_OP(cmpgei):
        FOLD_BINARY(>=);
        break;
      case IR_OP(negi):
        FOLD_UNARY(-);
        break;
      case IR_OP(onescomp):
        FOLD_UNARY(~);
        break;
      case IR_OP(noti):
        FOLD_UNARY(!);
        break;
      case IR_OP(zeroextendi):
        if (AllConstantInputs(inst)) {
          int64_t lhs = IRIntConstValue(inst->inputs.value.p[0]);
          int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]);
          IRNode* const_inst = GeneratorGetIntConstant(
              gen, inst->type, FoldZeroExtension(inst, lhs, rhs));
          GeneratorReplaceInstruction(gen, inst, const_inst);
          BasicBlockRemoveInstruction(gen, block, inst);
          continue;
        }
        break;
      case IR_OP(signextendi):
        if (AllConstantInputs(inst)) {
          int64_t lhs = IRIntConstValue(inst->inputs.value.p[0]);
          int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]);
          int64_t value;
          if (FoldSignExtension(inst, lhs, rhs, &value)) {
            IRNode* const_inst =
                GeneratorGetIntConstant(gen, inst->type, value);
            GeneratorReplaceInstruction(gen, inst, const_inst);
            BasicBlockRemoveInstruction(gen, block, inst);
            continue;
          }
        }
        break;
      case IR_OP(btrue):
        if (inst->inputs.length >= 1 &&
            inst->inputs.value.p[0] != NULL &&
            IRIsIntConst(inst->inputs.value.p[0])) {
          int64_t v = IRIntConstValue(inst->inputs.value.p[0]);
          if (v == 0) {
            BasicBlockRemoveInstruction(gen, block, inst);
          }
        }
        break;
      case IR_OP(bfalse):
        if (inst->inputs.length >= 1 &&
            inst->inputs.value.p[0] != NULL &&
            IRIsIntConst(inst->inputs.value.p[0])) {
          int64_t v = IRIntConstValue(inst->inputs.value.p[0]);
          if (v != 0) {
            BasicBlockRemoveInstruction(gen, block, inst);
          }
        }
        break;
     default:
        break;
    }
    inst = next;
  }
  
  // Propagate constants in all blocks dominated by this one, copying
  // the constants map for each one.
  for (size_t i = 0; i < block->dominatees.length; i++) {
    BlockId id = (BlockId)block->dominatees.value.p[i];
    BasicBlock* b = VectorGet(&gen->basic_blocks, id);
    
    ConstantPropagator sub_p;
    MapClone(&sub_p.constants, &p->constants);
    PropagateConstants(gen, &sub_p, b);
    MapDestruct(&sub_p.constants);
  }
}


void ConstantPropagationOptimization(Generator* gen) {
  ConstantPropagator p;
  MapInitForPointerKeys(&p.constants);
  PropagateConstants(gen, &p, gen->entry_block);
  MapDestruct(&p.constants);
}
