//
//  unroll.c
//  c_compiler
//
//  Complete unroll of tiny constant-trip innermost loops.
//

#include "unroll.h"

#include "basic_block.h"
#include "compiler.h"
#include "ir.h"
#include "loop_info.h"
#include "map.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
  kMaxUnrollTrip = 8,
  kMaxUnrolledNodes = 64,
};

typedef struct {
  Symbol* symbol;
  IRNode* update;
  BasicBlock* latch;
  IRNode* iv_var;
  int64_t trip;
  BasicBlock* body;
  BasicBlock* exit_block;
} UnrollCandidate;

static Symbol* VariableSymbol(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(externvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(ssavar):
    case IR_OP(phi):
    case IR_OP(structreturn):
      return ((IRVariable*)node)->symbol;
    default:
      return NULL;
  }
}

static bool IsIntegerIncrement(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(uinc8):
    case IR_OP(uinc16):
    case IR_OP(uinc32):
    case IR_OP(uinc64):
      return true;
    default:
      return false;
  }
}

static IROpcode StoreOpcodeForInc(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(inc8):
    case IR_OP(uinc8):
      return IR_OP(store8);
    case IR_OP(inc16):
    case IR_OP(uinc16):
      return IR_OP(store16);
    case IR_OP(inc32):
    case IR_OP(uinc32):
      return IR_OP(store32);
    case IR_OP(inc64):
    case IR_OP(uinc64):
      return IR_OP(store64);
    default:
      return IR_OP(nop);
  }
}

static bool IsLoadOfSymbol(IRNode* inst, const Symbol* symbol) {
  if (!IRIsLoadOnly(inst) || inst->inputs.length == 0 || symbol == NULL) {
    return false;
  }
  return VariableSymbol(inst->inputs.value.p[0]) == symbol;
}

static bool IsStoreOfSymbol(IRNode* inst, const Symbol* symbol) {
  if (!IRIsStoreOnly(inst) || inst->inputs.length == 0 || symbol == NULL) {
    return false;
  }
  return VariableSymbol(inst->inputs.value.p[0]) == symbol;
}

static IRNode* LastDefinitionInBlock(BasicBlock* block, const Symbol* symbol) {
  IRNode* definition = NULL;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (IRIsVarDef(inst) && inst->var.def == symbol) {
      definition = inst;
    }
  }
  return definition;
}

static bool HasZeroInitialValue(Generator* gen, const LoopInfo* loop,
                                const Symbol* symbol) {
  BasicBlock* block = loop->preheader;
  IRNode* definition = LastDefinitionInBlock(block, symbol);
  if (definition == NULL && block->in_edges.length == 1) {
    BasicBlock* predecessor =
        VectorGet(&gen->basic_blocks, block->in_edges.value.w[0]);
    if (!LoopInfoContainsBlock(loop, predecessor)) {
      definition = LastDefinitionInBlock(predecessor, symbol);
    }
  }
  return definition != NULL && definition->inputs.length >= 2 &&
         IRIsZero(definition->inputs.value.p[1]);
}

static bool LoopDefinesSymbol(Generator* gen, const LoopInfo* loop,
                              const Symbol* symbol, IRNode* except) {
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (inst != except && IRIsVarDef(inst) && inst->var.def == symbol) {
        return true;
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

static bool LoopIsUnsafe(Generator* gen, const LoopInfo* loop) {
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (inst->dest != NULL || IRIsCall(inst) || IRIsReturn(inst) ||
          IRIsObservableCheckpoint(inst) || inst->opcode == IR_OP(asm) ||
          inst->opcode == IR_OP(memcpy) || inst->opcode == IR_OP(memzero) ||
          inst->opcode == IR_OP(phi) || inst->opcode == IR_OP(ssavar) ||
          inst->opcode == IR_OP(cbra) ||
          (inst->flags & kIRJumpTableBranch) != 0) {
        return true;
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

static bool HeaderIsTestOnly(BasicBlock* header, const Symbol* iv) {
  for (IRNode* inst = BasicBlockBegin(header);
       !BasicBlockIsEmpty(header) && inst != BasicBlockEnd(header);
       inst = IRNext(inst)) {
    if (inst->opcode == IR_OP(label) || inst->opcode == IR_OP(loc) ||
        inst->opcode == IR_OP(nop) || inst->opcode == IR_OP(zeroextendi) ||
        IRIsConst(inst) || IRIsVariable(inst) || IRIsComparison(inst) ||
        IRIsConditionalBranch(inst) || IsLoadOfSymbol(inst, iv)) {
      continue;
    }
    return false;
  }
  return true;
}

static bool UniqueSuccessor(Generator* gen, BasicBlock* block,
                            BasicBlock** successor) {
  if (block->out_edges.length != 1) {
    return false;
  }
  *successor = VectorGet(&gen->basic_blocks, block->out_edges.value.w[0]);
  return true;
}

static bool BodyChainCoversLoop(Generator* gen, const LoopInfo* loop,
                                BasicBlock* body) {
  size_t seen = 1;  // header
  BasicBlock* block = body;
  while (block != loop->header) {
    if (!LoopInfoContainsBlock(loop, block) ||
        IRIsConditionalBranch(block->end_code)) {
      return false;
    }
    seen++;
    BasicBlock* successor;
    if (!UniqueSuccessor(gen, block, &successor)) {
      return false;
    }
    block = successor;
    if (seen > BitSetCount((BitSet*)&loop->blocks)) {
      return false;
    }
  }
  return seen == BitSetCount((BitSet*)&loop->blocks);
}

static IRNode* TryFoldBinary(Generator* gen, IROpcode opcode, IRNode* lhs,
                             IRNode* rhs, TypeRecord* type) {
  if (lhs == NULL || rhs == NULL || type == NULL || !IRIsIntConst(lhs) ||
      !IRIsIntConst(rhs)) {
    return NULL;
  }
  int64_t x = IRIntConstValue(lhs);
  int64_t y = IRIntConstValue(rhs);
  int64_t result;
  switch (opcode) {
    case IR_OP(addi):
      result = x + y;
      break;
    case IR_OP(subi):
      result = x - y;
      break;
    case IR_OP(muli):
      result = x * y;
      break;
    case IR_OP(andi):
      result = x & y;
      break;
    case IR_OP(ori):
      result = x | y;
      break;
    case IR_OP(xori):
      result = x ^ y;
      break;
    case IR_OP(lsli):
      if (y < 0 || y >= 64) {
        return NULL;
      }
      result = x << y;
      break;
    case IR_OP(lsri):
      if (y < 0 || y >= 64) {
        return NULL;
      }
      result = (int64_t)((uint64_t)x >> (uint64_t)y);
      break;
    case IR_OP(asri):
      if (y < 0 || y >= 64) {
        return NULL;
      }
      result = x >> y;
      break;
    default:
      return NULL;
  }
  return GeneratorGetIntConstant(gen, type, result);
}

static size_t CountCloneableNodes(Generator* gen, const LoopInfo* loop,
                                  BasicBlock* body, const UnrollCandidate* cand) {
  size_t count = 0;
  BasicBlock* block = body;
  while (block != loop->header) {
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (inst->opcode == IR_OP(label) || inst->opcode == IR_OP(loc) ||
          inst->opcode == IR_OP(nop) || IRIsConst(inst) || IRIsVariable(inst) ||
          IRIsBranch(inst) || inst == cand->update ||
          IsLoadOfSymbol(inst, cand->symbol) ||
          IsStoreOfSymbol(inst, cand->symbol)) {
        continue;
      }
      count++;
    }
    BasicBlock* successor;
    if (!UniqueSuccessor(gen, block, &successor)) {
      return SIZE_MAX;
    }
    block = successor;
  }
  return count;
}

static bool AnalyzeCountedLoop(Generator* gen, LoopInfo* loop,
                               UnrollCandidate* cand) {
  if (loop->children.length != 0 || loop->preheader == NULL ||
      BitSetCount(&loop->latches) != 1 || gen->exception_ranges.length != 0) {
    return false;
  }
  if (LoopIsUnsafe(gen, loop)) {
    return false;
  }

  BlockId latch_id = BitSetFindFirstSet(&loop->latches);
  BasicBlock* latch = VectorGet(&gen->basic_blocks, latch_id);
  UnrollCandidate found = {0};
  found.latch = latch;
  for (IRNode* inst = BasicBlockBegin(latch);
       !BasicBlockIsEmpty(latch) && inst != BasicBlockEnd(latch);
       inst = IRNext(inst)) {
    if (!IsIntegerIncrement(inst) || !IRIsVarDef(inst) ||
        inst->inputs.length < 2 || !IRIsIntConst(inst->inputs.value.p[1]) ||
        IRIntConstValue(inst->inputs.value.p[1]) != 1) {
      continue;
    }
    Symbol* symbol = VariableSymbol(inst->inputs.value.p[0]);
    if (symbol == NULL || symbol != inst->var.def ||
        LoopDefinesSymbol(gen, loop, symbol, inst)) {
      continue;
    }
    if (found.update != NULL) {
      return false;
    }
    found.symbol = symbol;
    found.update = inst;
    found.iv_var = inst->inputs.value.p[0];
  }
  if (found.update == NULL ||
      !HasZeroInitialValue(gen, loop, found.symbol) ||
      StoreOpcodeForInc(found.update->opcode) == IR_OP(nop)) {
    return false;
  }
  if (!HeaderIsTestOnly(loop->header, found.symbol)) {
    return false;
  }

  IRNode* terminator = loop->header->end_code;
  if (!IRIsConditionalBranch(terminator) || terminator->opcode != IR_OP(btrue) ||
      terminator->inputs.length < 2) {
    return false;
  }
  IRNode* cond = terminator->inputs.value.p[0];
  if (cond == NULL || cond->opcode != IR_OP(cmplti) ||
      cond->inputs.length < 2 ||
      !IsLoadOfSymbol(cond->inputs.value.p[0], found.symbol) ||
      !IRIsIntConst(cond->inputs.value.p[1])) {
    return false;
  }
  int64_t limit = IRIntConstValue(cond->inputs.value.p[1]);
  if (limit < 1 || limit > kMaxUnrollTrip) {
    return false;
  }
  found.trip = limit;

  IRNode* taken_label = terminator->inputs.value.p[1];
  if (taken_label == NULL || taken_label->block == NULL ||
      !LoopInfoContainsBlock(loop, taken_label->block)) {
    return false;
  }
  found.body = taken_label->block;

  BasicBlock* exit_block = NULL;
  for (size_t i = 0; i < loop->header->out_edges.length; i++) {
    BasicBlock* successor =
        VectorGet(&gen->basic_blocks, loop->header->out_edges.value.w[i]);
    if (!LoopInfoContainsBlock(loop, successor)) {
      if (exit_block != NULL) {
        return false;
      }
      exit_block = successor;
    }
  }
  if (exit_block == NULL || exit_block->code == NULL) {
    return false;
  }
  found.exit_block = exit_block;

  if (!BodyChainCoversLoop(gen, loop, found.body)) {
    return false;
  }

  IRNode* preheader_end = loop->preheader->end_code;
  if (preheader_end == NULL || preheader_end->opcode != IR_OP(bra) ||
      preheader_end->inputs.length == 0 ||
      ((IRNode*)preheader_end->inputs.value.p[0])->block != loop->header) {
    return false;
  }

  size_t body_nodes = CountCloneableNodes(gen, loop, found.body, &found);
  if (body_nodes == SIZE_MAX ||
      body_nodes * (size_t)found.trip > kMaxUnrolledNodes) {
    return false;
  }

  *cand = found;
  return true;
}

static IRNode* RemapInput(Map* remap, IRNode* input) {
  if (input == NULL) {
    return NULL;
  }
  IRNode* mapped = MapFindPointerKey(remap, input);
  return mapped != NULL ? mapped : input;
}

static void Remember(Map* remap, IRNode* from, IRNode* to) {
  MapKeyValue kv = {.key.p = from, .value.p = to};
  MapInsert(remap, kv);
}

static void CloneBodyIteration(Generator* gen, const LoopInfo* loop,
                               const UnrollCandidate* cand, int64_t iteration,
                               IRNode* insert_before) {
  Map remap;
  MapInitForPointerKeys(&remap);

  TypeRecord* iv_type =
      cand->iv_var->type != NULL ? cand->iv_var->type : cand->update->type;
  IRNode* iv_const = GeneratorGetIntConstant(gen, iv_type, iteration);
  Remember(&remap, cand->update, GeneratorGetIntConstant(gen, iv_type,
                                                         iteration + 1));

  BasicBlock* block = cand->body;
  while (block != loop->header) {
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (inst->opcode == IR_OP(label) || inst->opcode == IR_OP(loc) ||
          inst->opcode == IR_OP(nop) || IRIsConst(inst) || IRIsVariable(inst) ||
          IRIsBranch(inst) || inst == cand->update ||
          IsStoreOfSymbol(inst, cand->symbol)) {
        continue;
      }
      if (IsLoadOfSymbol(inst, cand->symbol)) {
        Remember(&remap, inst, iv_const);
        continue;
      }

      if (inst->inputs.length == 2) {
        IRNode* folded = TryFoldBinary(
            gen, inst->opcode, RemapInput(&remap, inst->inputs.value.p[0]),
            RemapInput(&remap, inst->inputs.value.p[1]), inst->type);
        if (folded != NULL) {
          Remember(&remap, inst, folded);
          continue;
        }
      }

      IRNode* copy = NewIR(inst->opcode);
      IRSetType(copy, inst->type);
      copy->flags = inst->flags;
      copy->var = inst->var;
      copy->location = inst->location;
      copy->value_state = inst->value_state;
      copy->data = inst->data;
      copy->aux = inst->aux;
      for (size_t i = 0; i < inst->inputs.length; i++) {
        IRNode* mapped = RemapInput(&remap, inst->inputs.value.p[i]);
        IRAddInput(copy, mapped, false);
      }
      BasicBlockEmitBefore(gen, loop->preheader, copy, insert_before);
      Remember(&remap, inst, copy);
    }
    BasicBlock* successor = NULL;
    if (!UniqueSuccessor(gen, block, &successor)) {
      MapDestruct(&remap);
      return;
    }
    block = successor;
  }

  MapDestruct(&remap);
}

static void RebuildVariableMetadata(BasicBlock* block) {
  MapClear(&block->defined_vars);
  SetClear(&block->referenced_vars);
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (IRIsVarDef(inst)) {
      MapKeyValue kv = {.key.p = inst->var.def, .value.p = NULL};
      MapInsert(&block->defined_vars, kv);
    }
    if (IRIsVarRef(inst)) {
      SetInsert(&block->referenced_vars, inst->var.use);
    }
  }
}

static bool UnrollLoop(Generator* gen, LoopInfo* loop,
                      const UnrollCandidate* cand) {
  BasicBlock* preheader = loop->preheader;
  IRNode* bra = preheader->end_code;
  IRNode* exit_label = cand->exit_block->code;
  if (exit_label->opcode != IR_OP(label)) {
    exit_label = GeneratorEmitBefore(gen, NewIR(IR_OP(label)), exit_label);
    exit_label->block = cand->exit_block;
    cand->exit_block->code = exit_label;
  }

  for (int64_t iteration = 0; iteration < cand->trip; iteration++) {
    CloneBodyIteration(gen, loop, cand, iteration, bra);
  }

  IROpcode store_op = StoreOpcodeForInc(cand->update->opcode);
  TypeRecord* iv_type =
      cand->iv_var->type != NULL ? cand->iv_var->type : cand->update->type;
  IRNode* final_iv = GeneratorGetIntConstant(gen, iv_type, cand->trip);
  IRNode* store = NewIR2(store_op, cand->iv_var, final_iv);
  IRSetVarDef(store, cand->symbol);
  BasicBlockEmitBefore(gen, preheader, store, bra);

  IRReplaceInput(bra, 0, exit_label);
  BasicBlockRemoveEdge(preheader, loop->header);
  BasicBlockAddEdge(preheader, cand->exit_block);
  RebuildVariableMetadata(preheader);

  Vector doomed;
  VectorInit(&doomed);
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    VectorAppend(&doomed,
                 VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it)));
    BitSetIteratorNext(&it);
  }
  for (size_t i = 0; i < doomed.length; i++) {
    BasicBlockClear(gen, doomed.value.p[i]);
  }
  VectorDestruct(&doomed);
  return true;
}

void LoopUnrollOptimization(Generator* gen) {
  if (!OptLevel3() || gen->loops.length == 0) {
    return;
  }

  Vector candidates;
  VectorInit(&candidates);
  for (size_t i = 0; i < gen->loops.length; i++) {
    VectorAppend(&candidates, gen->loops.value.p[i]);
  }

  for (size_t i = 0; i < candidates.length; i++) {
    LoopInfo* loop = candidates.value.p[i];
    UnrollCandidate cand;
    if (!AnalyzeCountedLoop(gen, loop, &cand)) {
      continue;
    }
    UnrollLoop(gen, loop, &cand);
  }
  VectorDestruct(&candidates);
}
