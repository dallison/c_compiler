//
//  sccp.c
//  c_compiler
//
//  Sparse conditional constant propagation without in-SSA CFG mutation.
//

#include "sccp.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  kSCCPUndefined,
  kSCCPConstant,
  kSCCPOverdefined,
} SCCPState;

typedef struct {
  SCCPState state;
  int64_t value;
} SCCPValue;

typedef struct {
  Generator* gen;
  SCCPValue* values;
  size_t value_count;
  size_t block_count;
  BitSet executable_blocks;
  BitSet executable_edges;
  bool changed;
} SCCPContext;

static Symbol* VariableSymbol(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
    case IR_OP(ssavar):
    case IR_OP(phi):
      return ((IRVariable*)node)->symbol;
    default:
      return NULL;
  }
}

static bool IsSafeSSASymbol(Symbol* symbol) {
  return symbol != NULL && !symbol->flags.address_taken &&
         !TypeIsArray(symbol->type) &&
         (symbol->flags.is_argument || symbol->flags.is_temp ||
          (symbol->flags.is_local &&
           !StorageIs(symbol->storage, STO(static))));
}

static SCCPValue UndefinedValue(void) {
  SCCPValue value = {kSCCPUndefined, 0};
  return value;
}

static SCCPValue ConstantValue(int64_t constant) {
  SCCPValue value = {kSCCPConstant, constant};
  return value;
}

static SCCPValue OverdefinedValue(void) {
  SCCPValue value = {kSCCPOverdefined, 0};
  return value;
}

static SCCPValue TypedConstantValue(IRNode* node, uint64_t value) {
  if (node->type == NULL || !TypeIsIntegral(node->type) ||
      node->type->size <= 0 || node->type->size >= 8) {
    return ConstantValue((int64_t)value);
  }
  int bits = node->type->size * 8;
  uint64_t mask = (1ULL << bits) - 1;
  value &= mask;
  if (!TypeIsUnsigned(node->type) &&
      (value & (1ULL << (bits - 1))) != 0) {
    value |= ~mask;
  }
  return ConstantValue((int64_t)value);
}

static SCCPValue ValueOf(SCCPContext* context, IRNode* node) {
  if (node == NULL || node->id < 0 ||
      (size_t)node->id >= context->value_count) {
    return OverdefinedValue();
  }
  return context->values[node->id];
}

static void UpdateValue(SCCPContext* context, IRNode* node,
                        SCCPValue incoming) {
  if (node == NULL || incoming.state == kSCCPUndefined ||
      node->id < 0 || (size_t)node->id >= context->value_count) {
    return;
  }
  SCCPValue* current = &context->values[node->id];
  if (current->state == kSCCPOverdefined) {
    return;
  }
  if (current->state == kSCCPUndefined) {
    *current = incoming;
    context->changed = true;
    return;
  }
  if (incoming.state == kSCCPOverdefined ||
      (incoming.state == kSCCPConstant &&
       current->value != incoming.value)) {
    *current = OverdefinedValue();
    context->changed = true;
  }
}

static size_t EdgeIndex(const SCCPContext* context, BlockId from,
                        BlockId to) {
  return (size_t)from * context->block_count + (size_t)to;
}

static bool EdgeIsExecutable(const SCCPContext* context, BlockId from,
                             BlockId to) {
  return BitSetContains((BitSet*)&context->executable_edges,
                        EdgeIndex(context, from, to));
}

static void MarkEdgeExecutable(SCCPContext* context, BlockId from,
                               BlockId to) {
  size_t edge = EdgeIndex(context, from, to);
  if (!BitSetContains(&context->executable_edges, edge)) {
    BitSetInsert(&context->executable_edges, edge);
    BitSetInsert(&context->executable_blocks, to);
    context->changed = true;
  }
}

static SCCPValue MergeValue(SCCPValue lhs, SCCPValue rhs) {
  if (lhs.state == kSCCPUndefined) {
    return rhs;
  }
  if (rhs.state == kSCCPUndefined) {
    return lhs;
  }
  if (lhs.state == kSCCPOverdefined ||
      rhs.state == kSCCPOverdefined ||
      lhs.value != rhs.value) {
    return OverdefinedValue();
  }
  return lhs;
}

static SCCPValue EvaluatePhi(SCCPContext* context, IRNode* phi) {
  BasicBlock* block = phi->block;
  if (phi->inputs.length == 0 ||
      phi->inputs.length != block->in_edges.length) {
    // Some reassignable variables deliberately retain a non-canonical phi
    // shape and are repaired when SSA is removed.  They represent a runtime
    // value, not lattice UNDEFINED.
    return OverdefinedValue();
  }
  SCCPValue result = UndefinedValue();
  size_t count = block->in_edges.length;
  for (size_t i = 0; i < count; i++) {
    BlockId predecessor = block->in_edges.value.w[i];
    if (!EdgeIsExecutable(context, predecessor, block->block_id)) {
      continue;
    }
    result = MergeValue(result, ValueOf(context, phi->inputs.value.p[i]));
    if (result.state == kSCCPOverdefined) {
      break;
    }
  }
  return result;
}

static bool IsIntegerStore(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store32):
    case IR_OP(store64):
    case IR_OP(storea):
      return true;
    default:
      return false;
  }
}

static bool IsIntegerLoad(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(load8):
    case IR_OP(load16):
    case IR_OP(load32):
    case IR_OP(load64):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadu32):
    case IR_OP(loada):
      return true;
    default:
      return false;
  }
}

static SCCPValue TruncateLoad(IRNode* inst, SCCPValue value) {
  if (value.state != kSCCPConstant) {
    return value;
  }
  int bits;
  bool sign_extend;
  switch (inst->opcode) {
    case IR_OP(load8):
      bits = 8;
      sign_extend = true;
      break;
    case IR_OP(load16):
      bits = 16;
      sign_extend = true;
      break;
    case IR_OP(load32):
      bits = 32;
      sign_extend = true;
      break;
    case IR_OP(loadu8):
      bits = 8;
      sign_extend = false;
      break;
    case IR_OP(loadu16):
      bits = 16;
      sign_extend = false;
      break;
    case IR_OP(loadu32):
      bits = 32;
      sign_extend = false;
      break;
    default:
      return value;
  }
  uint64_t mask = bits == 64 ? UINT64_MAX : ((1ULL << bits) - 1);
  uint64_t truncated = (uint64_t)value.value & mask;
  if (sign_extend && bits < 64 &&
      (truncated & (1ULL << (bits - 1))) != 0) {
    truncated |= ~mask;
  }
  return ConstantValue((int64_t)truncated);
}

static bool ReadOperands(SCCPContext* context, IRNode* inst,
                         SCCPValue* lhs, SCCPValue* rhs) {
  if (inst->inputs.length == 0) {
    return false;
  }
  *lhs = ValueOf(context, inst->inputs.value.p[0]);
  *rhs = inst->inputs.length > 1
             ? ValueOf(context, inst->inputs.value.p[1])
             : UndefinedValue();
  return true;
}

static SCCPValue EvaluateIntegerExpression(SCCPContext* context,
                                           IRNode* inst) {
  SCCPValue lhs;
  SCCPValue rhs;
  if (!ReadOperands(context, inst, &lhs, &rhs)) {
    return OverdefinedValue();
  }

  bool unary = inst->opcode == IR_OP(negi) ||
               inst->opcode == IR_OP(onescomp) ||
               inst->opcode == IR_OP(noti) ||
               inst->opcode == IR_OP(movi) ||
               inst->opcode == IR_OP(mova) ||
               inst->opcode == IR_OP(clzi) ||
               inst->opcode == IR_OP(ctzi) ||
               inst->opcode == IR_OP(popcounti);
  if (lhs.state == kSCCPOverdefined ||
      (!unary && rhs.state == kSCCPOverdefined)) {
    return OverdefinedValue();
  }
  if (lhs.state == kSCCPUndefined ||
      (!unary && rhs.state == kSCCPUndefined)) {
    return UndefinedValue();
  }

  uint64_t ulhs = (uint64_t)lhs.value;
  uint64_t urhs = (uint64_t)rhs.value;
  IRNode* lhs_node = inst->inputs.value.p[0];
  bool is_unsigned =
      lhs_node->type != NULL && TypeIsUnsigned(lhs_node->type);
  int width = lhs_node->type != NULL && TypeIsBitInt(lhs_node->type)
                  ? lhs_node->type->bit_width
                  : (inst->type != NULL && inst->type->size > 0
                         ? inst->type->size * 8
                         : 64);

  switch (inst->opcode) {
    case IR_OP(movi):
      return TypedConstantValue(inst, (uint64_t)lhs.value);
    case IR_OP(mova):
      return lhs;
    case IR_OP(addi):
      return TypedConstantValue(inst, ulhs + urhs);
    case IR_OP(adda):
      return ConstantValue((int64_t)(ulhs + urhs));
    case IR_OP(subi):
      return TypedConstantValue(inst, ulhs - urhs);
    case IR_OP(suba):
      return ConstantValue((int64_t)(ulhs - urhs));
    case IR_OP(muli):
      return TypedConstantValue(inst, ulhs * urhs);
    case IR_OP(divi):
      if (rhs.value == 0 ||
          (!is_unsigned && lhs.value == INT64_MIN && rhs.value == -1)) {
        return OverdefinedValue();
      }
      return TypedConstantValue(
          inst, is_unsigned ? ulhs / urhs
                            : (uint64_t)(lhs.value / rhs.value));
    case IR_OP(modi):
      if (rhs.value == 0 ||
          (!is_unsigned && lhs.value == INT64_MIN && rhs.value == -1)) {
        return OverdefinedValue();
      }
      return TypedConstantValue(
          inst, is_unsigned ? ulhs % urhs
                            : (uint64_t)(lhs.value % rhs.value));
    case IR_OP(andi):
      return TypedConstantValue(inst, ulhs & urhs);
    case IR_OP(zeroextendi):
      if (lhs_node->type != NULL && inst->type != NULL &&
          lhs_node->type->size != inst->type->size) {
        int bytes = lhs_node->type->size;
        if (inst->type->size < bytes) {
          bytes = inst->type->size;
        }
        int bits = bytes * 8;
        uint64_t mask =
            bits >= 64 ? UINT64_MAX : ((1ULL << bits) - 1);
        return TypedConstantValue(inst, ulhs & mask);
      }
      // Same-width zeroextendi nodes are also used for explicit bit masks.
      return TypedConstantValue(inst, ulhs & urhs);
    case IR_OP(ori):
      return TypedConstantValue(inst, ulhs | urhs);
    case IR_OP(xori):
      return TypedConstantValue(inst, ulhs ^ urhs);
    case IR_OP(lsli):
      if (rhs.value < 0 || rhs.value >= width || rhs.value >= 64) {
        return OverdefinedValue();
      }
      return TypedConstantValue(inst, ulhs << rhs.value);
    case IR_OP(lsri):
      if (rhs.value < 0 || rhs.value >= width || rhs.value >= 64) {
        return OverdefinedValue();
      }
      return TypedConstantValue(inst, ulhs >> rhs.value);
    case IR_OP(asri):
      if (rhs.value < 0 || rhs.value >= width || rhs.value >= 64) {
        return OverdefinedValue();
      }
      return TypedConstantValue(inst,
                                (uint64_t)(lhs.value >> rhs.value));
    case IR_OP(rotli):
    case IR_OP(rotri): {
      int amount = (int)(rhs.value % width);
      if (amount < 0) amount += width;
      uint64_t mask =
          width >= 64 ? UINT64_MAX : (UINT64_C(1) << width) - 1;
      uint64_t value = ulhs & mask;
      if (amount == 0) return TypedConstantValue(inst, value);
      uint64_t rotated =
          inst->opcode == IR_OP(rotli)
              ? (value << amount) | (value >> (width - amount))
              : (value >> amount) | (value << (width - amount));
      return TypedConstantValue(inst, rotated & mask);
    }
    case IR_OP(clzi): {
      uint64_t mask =
          width >= 64 ? UINT64_MAX : (UINT64_C(1) << width) - 1;
      uint64_t value = ulhs & mask;
      int count = 0;
      for (int bit = width - 1;
           bit >= 0 && (value & (UINT64_C(1) << bit)) == 0; --bit) {
        count++;
      }
      return TypedConstantValue(inst, count);
    }
    case IR_OP(ctzi): {
      uint64_t mask =
          width >= 64 ? UINT64_MAX : (UINT64_C(1) << width) - 1;
      uint64_t value = ulhs & mask;
      int count = 0;
      while (count < width && (value & (UINT64_C(1) << count)) == 0) {
        count++;
      }
      return TypedConstantValue(inst, count);
    }
    case IR_OP(popcounti): {
      uint64_t mask =
          width >= 64 ? UINT64_MAX : (UINT64_C(1) << width) - 1;
      uint64_t value = ulhs & mask;
      int count = 0;
      while (value != 0) {
        value &= value - 1;
        count++;
      }
      return TypedConstantValue(inst, count);
    }
    case IR_OP(signextendi):
      if (lhs_node->type != NULL && inst->type != NULL &&
          lhs_node->type->size != inst->type->size) {
        int bytes = lhs_node->type->size;
        if (inst->type->size < bytes) {
          bytes = inst->type->size;
        }
        int bits = bytes * 8;
        if (bits >= 64) {
          return lhs;
        }
        uint64_t mask = (1ULL << bits) - 1;
        uint64_t extended = ulhs & mask;
        if ((extended & (1ULL << (bits - 1))) != 0) {
          extended |= ~mask;
        }
        return TypedConstantValue(inst, extended);
      }
      if (rhs.value < 0 || rhs.value >= 64) {
        return OverdefinedValue();
      }
      return TypedConstantValue(
          inst,
          (uint64_t)((int64_t)((uint64_t)lhs.value << rhs.value) >>
                     rhs.value));
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
      return ConstantValue(lhs.value == rhs.value);
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
      return ConstantValue(lhs.value != rhs.value);
    case IR_OP(cmplti):
    case IR_OP(cmplta):
      return ConstantValue(is_unsigned ? ulhs < urhs
                                       : lhs.value < rhs.value);
    case IR_OP(cmplei):
    case IR_OP(cmplea):
      return ConstantValue(is_unsigned ? ulhs <= urhs
                                       : lhs.value <= rhs.value);
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
      return ConstantValue(is_unsigned ? ulhs > urhs
                                       : lhs.value > rhs.value);
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
      return ConstantValue(is_unsigned ? ulhs >= urhs
                                       : lhs.value >= rhs.value);
    case IR_OP(negi):
      return TypedConstantValue(inst, 0 - ulhs);
    case IR_OP(onescomp):
      return TypedConstantValue(inst, ~ulhs);
    case IR_OP(noti):
      return ConstantValue(!lhs.value);
    default:
      return OverdefinedValue();
  }
}

static bool IsSupportedIntegerExpression(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(movi):
    case IR_OP(mova):
    case IR_OP(addi):
    case IR_OP(adda):
    case IR_OP(subi):
    case IR_OP(suba):
    case IR_OP(muli):
    case IR_OP(divi):
    case IR_OP(modi):
    case IR_OP(andi):
    case IR_OP(ori):
    case IR_OP(xori):
    case IR_OP(lsli):
    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(rotli):
    case IR_OP(rotri):
    case IR_OP(clzi):
    case IR_OP(ctzi):
    case IR_OP(popcounti):
    case IR_OP(zeroextendi):
    case IR_OP(signextendi):
    case IR_OP(cmpeqi):
    case IR_OP(cmpeqa):
    case IR_OP(cmpnei):
    case IR_OP(cmpnea):
    case IR_OP(cmplti):
    case IR_OP(cmplta):
    case IR_OP(cmplei):
    case IR_OP(cmplea):
    case IR_OP(cmpgti):
    case IR_OP(cmpgta):
    case IR_OP(cmpgei):
    case IR_OP(cmpgea):
    case IR_OP(negi):
    case IR_OP(onescomp):
    case IR_OP(noti):
      return true;
    default:
      return false;
  }
}

static void EvaluateDefinition(SCCPContext* context, IRNode* inst) {
  if (!IRIsVarDef(inst) || inst->inputs.length == 0) {
    return;
  }
  IRNode* defined = inst->inputs.value.p[0];
  if (VariableSymbol(defined) != inst->var.def ||
      defined->opcode != IR_OP(ssavar)) {
    return;
  }
  if (IsSafeSSASymbol(inst->var.def) && IsIntegerStore(inst) &&
      inst->inputs.length > 1) {
    UpdateValue(context, defined,
                ValueOf(context, inst->inputs.value.p[1]));
  } else {
    UpdateValue(context, defined, OverdefinedValue());
  }
}

static void EvaluateInstruction(SCCPContext* context, IRNode* inst) {
  if (IRIsIntConst(inst)) {
    UpdateValue(context, inst, ConstantValue(IRIntConstValue(inst)));
    return;
  }
  if (inst->opcode == IR_OP(phi)) {
    UpdateValue(context, inst, EvaluatePhi(context, inst));
    return;
  }
  EvaluateDefinition(context, inst);

  if (IRIsCall(inst)) {
    UpdateValue(context, inst, OverdefinedValue());
    return;
  }
  if (IRIsStore(inst)) {
    if (inst->outputs.length != 0) {
      UpdateValue(context, inst, OverdefinedValue());
    }
    return;
  }
  if (inst->opcode == IR_OP(ssavar) || IRIsBranch(inst) ||
      IRIsReturn(inst) ||
      inst->opcode == IR_OP(label) || inst->opcode == IR_OP(enter) ||
      inst->opcode == IR_OP(leave)) {
    return;
  }
  if (inst->dest != NULL || IRHasSideEffects(inst)) {
    UpdateValue(context, inst, OverdefinedValue());
    return;
  }
  if (IsIntegerLoad(inst)) {
    Symbol* symbol = inst->inputs.length == 0
                         ? NULL
                         : VariableSymbol(inst->inputs.value.p[0]);
    if (!IsSafeSSASymbol(symbol)) {
      UpdateValue(context, inst, OverdefinedValue());
      return;
    }
    UpdateValue(context, inst,
                TruncateLoad(inst,
                             ValueOf(context, inst->inputs.value.p[0])));
    return;
  }
  if (IsSupportedIntegerExpression(inst)) {
    UpdateValue(context, inst, EvaluateIntegerExpression(context, inst));
    return;
  }
  if (IRIsVariable(inst)) {
    if (inst->opcode != IR_OP(ssavar) && inst->opcode != IR_OP(phi)) {
      UpdateValue(context, inst, OverdefinedValue());
    }
    return;
  }
  if (IRIsExpression(inst)) {
    UpdateValue(context, inst, OverdefinedValue());
  }
}

static void MarkSuccessors(SCCPContext* context, BasicBlock* block) {
  if (block->out_edges.length == 0 || block->end_code == NULL) {
    return;
  }
  IRNode* terminator = block->end_code;
  if (IRIsConditionalBranch(terminator) &&
      terminator->inputs.length >= 2) {
    if (block->out_edges.length == 1) {
      MarkEdgeExecutable(context, block->block_id,
                         block->out_edges.value.w[0]);
      return;
    }
    SCCPValue condition =
        ValueOf(context, terminator->inputs.value.p[0]);
    if (condition.state == kSCCPUndefined) {
      return;
    }
    if (condition.state == kSCCPOverdefined) {
      for (size_t i = 0; i < block->out_edges.length; i++) {
        MarkEdgeExecutable(context, block->block_id,
                           block->out_edges.value.w[i]);
      }
      return;
    }
    BasicBlock* taken =
        ((IRNode*)terminator->inputs.value.p[1])->block;
    bool take = terminator->opcode == IR_OP(btrue)
                    ? condition.value != 0
                    : condition.value == 0;
    for (size_t i = 0; i < block->out_edges.length; i++) {
      BlockId successor = block->out_edges.value.w[i];
      if ((successor == taken->block_id) == take) {
        MarkEdgeExecutable(context, block->block_id, successor);
      }
    }
    return;
  }
  for (size_t i = 0; i < block->out_edges.length; i++) {
    MarkEdgeExecutable(context, block->block_id,
                       block->out_edges.value.w[i]);
  }
}

static void InitializeValues(SCCPContext* context) {
  for (IRNode* inst = GeneratorFirstInstruction(context->gen);
       inst != NULL; inst = IRNext(inst)) {
    if (inst->id < 0 || (size_t)inst->id >= context->value_count) {
      continue;
    }
    if (IRIsIntConst(inst)) {
      context->values[inst->id] =
          ConstantValue(IRIntConstValue(inst));
      continue;
    }
    if (IRIsVariable(inst) && inst->opcode != IR_OP(ssavar) &&
        inst->opcode != IR_OP(phi)) {
      context->values[inst->id] = OverdefinedValue();
    }
  }
}

static bool Analyze(SCCPContext* context) {
  BitSetInsert(&context->executable_blocks,
               context->gen->entry_block->block_id);
  bool any_change = false;
  do {
    context->changed = false;
    for (size_t i = 0; i < context->gen->basic_blocks.length; i++) {
      BasicBlock* block = context->gen->basic_blocks.value.p[i];
      if (!BitSetContains(&context->executable_blocks, block->block_id)) {
        continue;
      }
      for (IRNode* inst = BasicBlockBegin(block);
           !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
           inst = IRNext(inst)) {
        EvaluateInstruction(context, inst);
      }
      MarkSuccessors(context, block);
    }
    any_change |= context->changed;
  } while (context->changed);
  return any_change;
}

static bool IsRewritableConstant(IRNode* inst) {
  if (IRIsIntConst(inst) || inst->opcode == IR_OP(ssavar) ||
      (IRIsVariable(inst) && inst->opcode != IR_OP(phi)) ||
      IRIsBranch(inst) || IRIsReturn(inst) || IRIsStore(inst) ||
      IRIsCall(inst) || inst->dest != NULL || IRHasSideEffects(inst) ||
      inst->type == NULL || TypeIsVoid(inst->type) ||
      inst->outputs.length == 0) {
    return false;
  }
  return true;
}

static void RemovePhiPredecessor(BasicBlock* block, size_t index) {
  IRNode* inst = block->code;
  while (inst != NULL && inst->opcode == IR_OP(ssavar)) {
    inst = IRNext(inst);
  }
  while (inst != NULL && inst->opcode == IR_OP(phi)) {
    if (index < inst->inputs.length) {
      IRRemoveInput(inst, index);
    }
    inst = IRNext(inst);
  }
}

static bool SimplifyBranches(SCCPContext* context, SCCPStats* stats) {
  bool changed = false;
  for (size_t i = 0; i < context->gen->basic_blocks.length; i++) {
    BasicBlock* block = context->gen->basic_blocks.value.p[i];
    if (!BitSetContains(&context->executable_blocks, block->block_id) ||
        block->end_code == NULL ||
        !IRIsConditionalBranch(block->end_code) ||
        block->end_code->inputs.length < 2) {
      continue;
    }
    IRNode* branch = block->end_code;
    SCCPValue condition =
        ValueOf(context, branch->inputs.value.p[0]);
    if (condition.state != kSCCPConstant) {
      continue;
    }
    if (block->out_edges.length == 1) {
      bool take = branch->opcode == IR_OP(btrue)
                      ? condition.value != 0
                      : condition.value == 0;
      if (!take) {
        BasicBlockRemoveInstruction(context->gen, block, branch);
        changed = true;
      }
      if (stats != NULL) {
        stats->branches_simplified++;
      }
      continue;
    }
    BasicBlock* taken = ((IRNode*)branch->inputs.value.p[1])->block;
    bool take = branch->opcode == IR_OP(btrue)
                    ? condition.value != 0
                    : condition.value == 0;
    size_t edge = 0;
    while (edge < block->out_edges.length) {
      BlockId successor_id = block->out_edges.value.w[edge];
      bool keep = (successor_id == taken->block_id) == take;
      if (keep) {
        edge++;
        continue;
      }
      BasicBlock* successor =
          context->gen->basic_blocks.value.p[successor_id];
      size_t input_index = 0;
      while (input_index < successor->in_edges.length &&
             successor->in_edges.value.w[input_index] != block->block_id) {
        input_index++;
      }
      if (input_index < successor->in_edges.length) {
        RemovePhiPredecessor(successor, input_index);
      }
      BasicBlockRemoveEdge(block, successor);
      changed = true;
    }
    if (!take) {
      BasicBlockRemoveInstruction(context->gen, block, branch);
    }
    if (stats != NULL) {
      stats->branches_simplified++;
    }
  }
  if (changed) {
    for (size_t i = 0; i < context->gen->basic_blocks.length; i++) {
      BasicBlock* block = context->gen->basic_blocks.value.p[i];
      block->reachability_known = false;
      block->is_unreachable = false;
    }
  }
  return changed;
}

static bool Rewrite(SCCPContext* context, SCCPStats* stats) {
  bool changed = SimplifyBranches(context, stats);
  for (size_t i = 0; i < context->gen->basic_blocks.length; i++) {
    BasicBlock* block = context->gen->basic_blocks.value.p[i];
    if (!BitSetContains(&context->executable_blocks, block->block_id)) {
      continue;
    }
    IRNode* next = NULL;
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = next) {
      next = IRNext(inst);
      SCCPValue value = ValueOf(context, inst);
      if (value.state != kSCCPConstant || !IsRewritableConstant(inst)) {
        continue;
      }
      IRNode* constant =
          GeneratorGetIntConstant(context->gen, inst->type, value.value);
      if (stats != NULL) {
        stats->constants_folded++;
        if (inst->opcode == IR_OP(phi)) {
          stats->phis_folded++;
        }
      }
      GeneratorReplaceInstruction(context->gen, inst, constant);
      BasicBlockRemoveInstruction(context->gen, block, inst);
      changed = true;
    }
  }
  return changed;
}

bool SparseConditionalConstantPropagation(Generator* gen, SCCPStats* stats) {
  if (stats != NULL) {
    memset(stats, 0, sizeof(*stats));
  }
  size_t max_id = 0;
  for (IRNode* inst = GeneratorFirstInstruction(gen);
       inst != NULL; inst = IRNext(inst)) {
    if (inst->id >= 0 && (size_t)inst->id > max_id) {
      max_id = (size_t)inst->id;
    }
  }

  SCCPContext context;
  memset(&context, 0, sizeof(context));
  context.gen = gen;
  context.value_count = max_id + 1;
  context.block_count = gen->basic_blocks.length;
  context.values = calloc(context.value_count, sizeof(SCCPValue));
  BitSetInit(&context.executable_blocks);
  BitSetInit(&context.executable_edges);
  InitializeValues(&context);
  Analyze(&context);
  bool changed = Rewrite(&context, stats);
  BitSetDestruct(&context.executable_edges);
  BitSetDestruct(&context.executable_blocks);
  free(context.values);
  return changed;
}
