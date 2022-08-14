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
    case IR_OP(ssavar):
    case IR_OP(phi):
    case IR_OP(localvar):
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
    int64_t mask = bits - 1;
    value &= mask;
  }
  return GeneratorGetIntConstant(gen, con->base.type, value);
}

static bool AllConstantInputs(IRNode* node) {
  for (size_t i = 0; i < node->inputs.length; i++) {
    if (!IRIsIntConst(node->inputs.value.p[i])) {
      return false;
    }
  }
  return true;
}

static void PropagateConstants(Generator* gen, ConstantPropagator* p,
                                  BasicBlock* block) {
  IRNode* next = NULL;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = next) {
    next = IRNext(inst);
    if (inst->dest != NULL) {
      continue;
    }
    switch (inst->opcode) {
      case IR_OP(store32):
      case IR_OP(store8):
      case IR_OP(store16):
      case IR_OP(store64):
        if (IsVariableReference(inst->inputs.value.p[0])) {
          if (IRIsIntConst(inst->inputs.value.p[1])) {
            // Store of a constant to a variable.
            IRNode* var = inst->inputs.value.p[0];
            if (TypeIsVolatile(var->type)) {
              // Volatile prevents this optimization.
              break;
            }
            IRNode* val = inst->inputs.value.p[1];
            assert(!TypeIsVoid(val->type));
            MapKeyValue kv = {.key.p = var, .value.p = val};
            MapInsert(&p->constants, kv);
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
        if (IsVariableReference(inst->inputs.value.p[0])) {
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
  }

#define FOLD_DIV(op) \
        if (AllConstantInputs(inst)) { \
          int64_t lhs = IRIntConstValue(inst->inputs.value.p[0]); \
          int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]); \
          if (rhs != 0) {\
            IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, lhs op rhs); \
            GeneratorReplaceInstruction(gen, inst, const_inst); \
            BasicBlockRemoveInstruction(gen, block, inst); \
          } \
        }
#define FOLD_UNARY(op) \
        if (AllConstantInputs(inst)) { \
          int64_t sub = IRIntConstValue(inst->inputs.value.p[0]); \
          IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, op sub); \
          GeneratorReplaceInstruction(gen, inst, const_inst); \
          BasicBlockRemoveInstruction(gen, block, inst); \
        }

      case IR_OP(addi):
        FOLD_BINARY(+);
        break;
      case IR_OP(subi):
        FOLD_BINARY(-);
        break;
      case IR_OP(muli):
        FOLD_BINARY(*);
        break;
      case IR_OP(divi):
        FOLD_DIV(/);
        break;
      case IR_OP(modi):
        FOLD_DIV(%);
        break;
      case IR_OP(xori):
        FOLD_BINARY(^);
        break;
      case IR_OP(andi):
        FOLD_BINARY(&);
        break;
      case IR_OP(ori):
        FOLD_BINARY(|);
        break;
      case IR_OP(lsri):
         if (AllConstantInputs(inst)) {
           uint64_t lhs = IRIntConstValue(inst->inputs.value.p[0]);
           int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]);
           IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, lhs >> rhs);
           GeneratorReplaceInstruction(gen, inst, const_inst);
           BasicBlockRemoveInstruction(gen, block, inst);
         }
         break;
      case IR_OP(asri):
        FOLD_BINARY(>>);
        break;
      case IR_OP(lsli):
        FOLD_BINARY(<<);
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
        FOLD_BINARY(&);
        break;
      case IR_OP(signextendi):
        if (AllConstantInputs(inst)) {
          int64_t lhs = IRIntConstValue(inst->inputs.value.p[0]);
          int64_t rhs = IRIntConstValue(inst->inputs.value.p[1]);
          IRNode* const_inst = GeneratorGetIntConstant(gen, inst->type, (lhs << rhs) >> rhs);
          GeneratorReplaceInstruction(gen, inst, const_inst);
          BasicBlockRemoveInstruction(gen, block, inst);
        }
        break;
      case IR_OP(btrue):
        if (IRIsIntConst(inst->inputs.value.p[0])) {
          int64_t v = IRIntConstValue(inst->inputs.value.p[0]);
          if (v == 0) {
            BasicBlockRemoveInstruction(gen, block, inst);
          }
        }
        break;
      case IR_OP(bfalse):
        if (IRIsIntConst(inst->inputs.value.p[0])) {
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
