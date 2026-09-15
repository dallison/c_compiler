//
//  vectorize.c
//  c_compiler
//
//  Conservative loop auto-vectorization of unit-stride counted loops, plus
//  SLP packing of adjacent isomorphic scalar ops in a single block.
//

#include "vectorize.h"

#include "basic_block.h"
#include "compiler.h"
#include "dstring.h"
#include "expr_codegen.h"
#include "ir.h"
#include "loop_info.h"
#include "map.h"
#include "syntax.h"
#include "type.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { kNativeVectorBytes = 16 };

typedef struct {
  Symbol* symbol;
  IRNode* update;
  BasicBlock* latch;
  IRNode* iv_var;
  int64_t trip;
  BasicBlock* body;
} CountedLoop;

typedef struct {
  IRNode* address;
  IRNode* base;
  Symbol* object;
  Symbol* pointer;
  IRNode* pointer_update;
} StridedAccess;

typedef struct {
  IRNode* store;
  IRNode* binop;
  IROpcode vector_opcode;
  TypeRecord* element;
  int elem_size;
  int vf;
  StridedAccess dest;
  StridedAccess left;
  StridedAccess right;
  IRNode* left_load;
  IRNode* right_load;
  IRNode* left_value;
  IRNode* right_value;
  bool left_splat;
  bool right_splat;
} VectorCandidate;

static Symbol* VariableSymbol(IRNode* node) {
  if (node == NULL) {
    return NULL;
  }
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

static bool IsPointerIncrement(IRNode* inst) {
  return inst != NULL && inst->opcode == IR_OP(inca);
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
        inst->opcode == IR_OP(signextendi) || inst->opcode == IR_OP(cast) ||
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
  size_t seen = 1;
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

static bool AnalyzeCountedLoop(Generator* gen, LoopInfo* loop,
                               CountedLoop* cand) {
  if (loop->children.length != 0 || loop->preheader == NULL ||
      BitSetCount(&loop->latches) != 1 || gen->exception_ranges.length != 0) {
    return false;
  }
  if (LoopIsUnsafe(gen, loop)) {
    return false;
  }

  BlockId latch_id = BitSetFindFirstSet(&loop->latches);
  BasicBlock* latch = VectorGet(&gen->basic_blocks, latch_id);
  CountedLoop found = {0};
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
      !HasZeroInitialValue(gen, loop, found.symbol)) {
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
  if (limit < 2 || limit > 1 << 20) {
    return false;
  }
  found.trip = limit;

  IRNode* taken_label = terminator->inputs.value.p[1];
  if (taken_label == NULL || taken_label->block == NULL ||
      !LoopInfoContainsBlock(loop, taken_label->block)) {
    return false;
  }
  found.body = taken_label->block;
  if (!BodyChainCoversLoop(gen, loop, found.body)) {
    return false;
  }

  *cand = found;
  return true;
}

// 8-bit targets have no SIMD and a 16-byte vector expands into too much code.
// Every other backend either keeps native SIMD (x86-64, AArch64) or software-
// expands the vector IR in ScalarizeVectorOperations before lowering.
static bool TargetSupportsAutoVectorization(void) {
  if (compiler->target_name == NULL) {
    return false;
  }
  return !StringEqual(compiler->target_name, "6502") &&
         !StringEqual(compiler->target_name, "65c02") &&
         !StringEqual(compiler->target_name, "65C02");
}

static int MemoryWidth(IRNode* inst) {
  if (inst == NULL) {
    return 0;
  }
  switch (inst->opcode) {
    case IR_OP(load8):
    case IR_OP(loadu8):
    case IR_OP(store8):
      return 1;
    case IR_OP(load16):
    case IR_OP(loadu16):
    case IR_OP(store16):
      return 2;
    case IR_OP(load32):
    case IR_OP(loadu32):
    case IR_OP(store32):
    case IR_OP(loadf):
    case IR_OP(storef):
      return 4;
    case IR_OP(load64):
    case IR_OP(store64):
    case IR_OP(loadd):
    case IR_OP(stored):
      return 8;
    default:
      return 0;
  }
}

static IROpcode VectorOpcodeForBinop(IROpcode opcode, TypeRecord* element) {
  bool integer = TypeIsIntegral(element);
  bool f32 = TypeUsesFloat32Representation(element);
  bool f64 = TypeUsesFloat64Representation(element);
  switch (opcode) {
    case IR_OP(addi):
      return integer ? IR_OP(vadd) : IR_OP(nop);
    case IR_OP(subi):
      return integer ? IR_OP(vsub) : IR_OP(nop);
    case IR_OP(andi):
      return integer ? IR_OP(vand) : IR_OP(nop);
    case IR_OP(ori):
      return integer ? IR_OP(vor) : IR_OP(nop);
    case IR_OP(xori):
      return integer ? IR_OP(vxor) : IR_OP(nop);
    case IR_OP(addf):
    case IR_OP(addd):
      return (f32 || f64) ? IR_OP(vadd) : IR_OP(nop);
    case IR_OP(subf):
    case IR_OP(subd):
      return (f32 || f64) ? IR_OP(vsub) : IR_OP(nop);
    case IR_OP(mulf):
    case IR_OP(muld):
      return (f32 || f64) ? IR_OP(vmul) : IR_OP(nop);
    case IR_OP(divf):
    case IR_OP(divd):
      return (f32 || f64) ? IR_OP(vdiv) : IR_OP(nop);
    default:
      return IR_OP(nop);
  }
}

static bool OpcodeIsNative(IROpcode opcode, TypeRecord* element) {
  if (TypeIsIntegral(element)) {
    return opcode == IR_OP(vadd) || opcode == IR_OP(vsub) ||
           opcode == IR_OP(vand) || opcode == IR_OP(vor) ||
           opcode == IR_OP(vxor);
  }
  if (TypeUsesFloat32Representation(element) ||
      TypeUsesFloat64Representation(element)) {
    return opcode == IR_OP(vadd) || opcode == IR_OP(vsub) ||
           opcode == IR_OP(vmul) || opcode == IR_OP(vdiv);
  }
  return false;
}

static bool IsIVValue(IRNode* node, const Symbol* iv) {
  if (node == NULL || iv == NULL) {
    return false;
  }
  if (IsLoadOfSymbol(node, iv)) {
    return true;
  }
  if ((node->opcode == IR_OP(signextendi) ||
       node->opcode == IR_OP(zeroextendi) || node->opcode == IR_OP(cast)) &&
      node->inputs.length >= 1) {
    return IsIVValue(node->inputs.value.p[0], iv);
  }
  return false;
}

static IRNode* PeelInvariantBase(IRNode* base) {
  while (base != NULL && base->opcode == IR_OP(adda) &&
         base->inputs.length == 2) {
    if (IRIsZero(base->inputs.value.p[0])) {
      base = base->inputs.value.p[1];
    } else if (IRIsZero(base->inputs.value.p[1])) {
      base = base->inputs.value.p[0];
    } else {
      break;
    }
  }
  if (base != NULL && base->opcode == IR_OP(addressof) &&
      base->inputs.length == 1) {
    base = base->inputs.value.p[0];
  }
  return base;
}

static bool IsSafeArrayObject(IRNode* base) {
  base = PeelInvariantBase(base);
  Symbol* symbol = VariableSymbol(base);
  if (symbol == NULL || symbol->type == NULL) {
    return false;
  }
  if (TypeIsVolatile(symbol->type) || TypeIsVolatile(base->type) ||
      TypeIsAtomic(symbol->type)) {
    return false;
  }
  return TypeIsArray(symbol->type) || TypeIsArray(base->type);
}

static bool MatchScale(IRNode* scale, const Symbol* iv, int elem_size) {
  if (scale == NULL || elem_size <= 0) {
    return false;
  }
  if (elem_size == 1 && IsIVValue(scale, iv)) {
    return true;
  }
  if (scale->opcode == IR_OP(muli) && scale->inputs.length == 2) {
    IRNode* index = NULL;
    IRNode* constant = NULL;
    for (size_t i = 0; i < 2; i++) {
      IRNode* candidate = scale->inputs.value.p[i];
      if (IRIsIntConst(candidate)) {
        constant = candidate;
      } else {
        index = candidate;
      }
    }
    return index != NULL && constant != NULL &&
           IRIntConstValue(constant) == elem_size && IsIVValue(index, iv);
  }
  if (scale->opcode == IR_OP(lsli) && scale->inputs.length == 2 &&
      IsIVValue(scale->inputs.value.p[0], iv) &&
      IRIsIntConst(scale->inputs.value.p[1])) {
    int64_t shift = IRIntConstValue(scale->inputs.value.p[1]);
    return shift >= 0 && shift < 31 && (1 << shift) == elem_size;
  }
  return false;
}

static IRNode* PointerUpdateInLatch(BasicBlock* latch, const Symbol* pointer) {
  for (IRNode* inst = BasicBlockBegin(latch);
       !BasicBlockIsEmpty(latch) && inst != BasicBlockEnd(latch);
       inst = IRNext(inst)) {
    if (IsPointerIncrement(inst) && IRIsVarDef(inst) &&
        inst->var.def == pointer && inst->inputs.length >= 2 &&
        IRIsIntConst(inst->inputs.value.p[1])) {
      return inst;
    }
  }
  return NULL;
}

static IRNode* PointerInitialBase(Generator* gen, const LoopInfo* loop,
                                  const Symbol* pointer) {
  IRNode* definition = LastDefinitionInBlock(loop->preheader, pointer);
  if (definition == NULL && loop->preheader->in_edges.length == 1) {
    BasicBlock* predecessor =
        VectorGet(&gen->basic_blocks, loop->preheader->in_edges.value.w[0]);
    if (!LoopInfoContainsBlock(loop, predecessor)) {
      definition = LastDefinitionInBlock(predecessor, pointer);
    }
  }
  if (definition == NULL || definition->inputs.length < 2) {
    return NULL;
  }
  return definition->inputs.value.p[1];
}

static bool MatchStridedAddress(Generator* gen, const LoopInfo* loop,
                                const CountedLoop* counted, IRNode* address,
                                int elem_size, StridedAccess* access) {
  StridedAccess found = {0};
  found.address = address;
  if (address == NULL) {
    return false;
  }

  if (address->opcode == IR_OP(loada) && address->inputs.length == 1) {
    Symbol* pointer = VariableSymbol(address->inputs.value.p[0]);
    if (pointer == NULL) {
      return false;
    }
    IRNode* update = PointerUpdateInLatch(counted->latch, pointer);
    if (update == NULL ||
        IRIntConstValue(update->inputs.value.p[1]) != elem_size) {
      return false;
    }
    IRNode* initial = PointerInitialBase(gen, loop, pointer);
    if (!IsSafeArrayObject(initial)) {
      return false;
    }
    found.pointer = pointer;
    found.pointer_update = update;
    found.base = PeelInvariantBase(initial);
    found.object = VariableSymbol(found.base);
    *access = found;
    return found.object != NULL;
  }

  if (address->opcode != IR_OP(adda) || address->inputs.length != 2) {
    return false;
  }
  IRNode* base = NULL;
  if (MatchScale(address->inputs.value.p[1], counted->symbol, elem_size)) {
    base = address->inputs.value.p[0];
  } else if (MatchScale(address->inputs.value.p[0], counted->symbol,
                        elem_size)) {
    base = address->inputs.value.p[1];
  }
  base = PeelInvariantBase(base);
  if (!IsSafeArrayObject(base)) {
    return false;
  }
  found.base = base;
  found.object = VariableSymbol(base);
  *access = found;
  return found.object != NULL;
}

static bool IsLoopInvariantValue(Generator* gen, const LoopInfo* loop,
                                 const CountedLoop* counted, IRNode* node) {
  if (node == NULL || IsIVValue(node, counted->symbol)) {
    return false;
  }
  if (IRIsConst(node) || IRIsVariable(node)) {
    return true;
  }
  if (!IRIsLoadOnly(node) || node->inputs.length == 0 ||
      MemoryWidth(node) == 0) {
    return false;
  }
  IRNode* address = node->inputs.value.p[0];
  Symbol* symbol = VariableSymbol(address);
  if (symbol == NULL || !IRIsVariable(address) ||
      LoopDefinesSymbol(gen, loop, symbol, NULL) ||
      TypeIsVolatile(node->type) || TypeIsVolatile(address->type)) {
    return false;
  }
  return true;
}

static bool SameObjectMayOverlap(const StridedAccess* dest,
                                 const StridedAccess* other) {
  if (dest->object == NULL || other->object == NULL) {
    return true;
  }
  if (dest->object != other->object) {
    return false;
  }
  return dest->address != other->address;
}

static bool ExtraMemoryOp(IRNode* inst, const VectorCandidate* cand,
                          const CountedLoop* counted) {
  if (IRIsStoreOnly(inst)) {
    if (IsStoreOfSymbol(inst, counted->symbol) || inst == cand->store) {
      return false;
    }
    return true;
  }
  if (!IRIsLoadOnly(inst) || MemoryWidth(inst) == 0) {
    return false;
  }
  if (IsLoadOfSymbol(inst, counted->symbol) || inst == cand->left_load ||
      inst == cand->right_load) {
    return false;
  }
  return true;
}

static bool AnalyzeVectorBody(Generator* gen, LoopInfo* loop,
                              const CountedLoop* counted,
                              VectorCandidate* cand) {
  VectorCandidate found = {0};
  BasicBlock* block = counted->body;
  while (block != loop->header) {
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (!IRIsStoreOnly(inst) || IsStoreOfSymbol(inst, counted->symbol) ||
          inst->inputs.length < 2) {
        continue;
      }
      if (found.store != NULL) {
        return false;
      }
      found.store = inst;
    }
    BasicBlock* successor;
    if (!UniqueSuccessor(gen, block, &successor)) {
      return false;
    }
    block = successor;
  }
  if (found.store == NULL) {
    return false;
  }

  found.elem_size = MemoryWidth(found.store);
  found.binop = found.store->inputs.value.p[1];
  if (found.elem_size <= 0 || found.binop == NULL ||
      found.binop->inputs.length != 2) {
    return false;
  }
  found.element = found.binop->type;
  if (found.element == NULL || found.element->size != found.elem_size ||
      TypeIsVolatile(found.element) || TypeIsAtomic(found.element) ||
      TypeIsBitInt(found.element)) {
    return false;
  }
  found.vector_opcode = VectorOpcodeForBinop(found.binop->opcode, found.element);
  if (!OpcodeIsNative(found.vector_opcode, found.element)) {
    return false;
  }
  if (kNativeVectorBytes % found.elem_size != 0) {
    return false;
  }
  found.vf = kNativeVectorBytes / found.elem_size;
  if (found.vf < 2 || counted->trip < found.vf ||
      counted->trip % found.vf != 0) {
    return false;
  }

  if (!MatchStridedAddress(gen, loop, counted, found.store->inputs.value.p[0],
                           found.elem_size, &found.dest)) {
    return false;
  }

  found.left_value = found.binop->inputs.value.p[0];
  found.right_value = found.binop->inputs.value.p[1];
  if (IRIsLoadOnly(found.left_value) &&
      MemoryWidth(found.left_value) == found.elem_size) {
    if (!MatchStridedAddress(gen, loop, counted,
                             found.left_value->inputs.value.p[0],
                             found.elem_size, &found.left)) {
      return false;
    }
    found.left_load = found.left_value;
  } else if (IsLoopInvariantValue(gen, loop, counted, found.left_value)) {
    found.left_splat = true;
  } else {
    return false;
  }
  if (IRIsLoadOnly(found.right_value) &&
      MemoryWidth(found.right_value) == found.elem_size) {
    if (!MatchStridedAddress(gen, loop, counted,
                             found.right_value->inputs.value.p[0],
                             found.elem_size, &found.right)) {
      return false;
    }
    found.right_load = found.right_value;
  } else if (IsLoopInvariantValue(gen, loop, counted, found.right_value)) {
    found.right_splat = true;
  } else {
    return false;
  }
  if (found.left_splat && found.right_splat) {
    return false;
  }
  if (!found.left_splat && SameObjectMayOverlap(&found.dest, &found.left)) {
    return false;
  }
  if (!found.right_splat && SameObjectMayOverlap(&found.dest, &found.right)) {
    return false;
  }

  block = counted->body;
  while (block != loop->header) {
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (ExtraMemoryOp(inst, &found, counted)) {
        return false;
      }
    }
    BasicBlock* successor;
    if (!UniqueSuccessor(gen, block, &successor)) {
      return false;
    }
    block = successor;
  }

  *cand = found;
  return true;
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

static IRNode* EmitBeforeTerminator(Generator* gen, BasicBlock* block,
                                    IRNode* inst) {
  if (IRIsBranch(block->end_code)) {
    BasicBlockEmitBefore(gen, block, inst, block->end_code);
  } else {
    GeneratorEmitBefore(gen, inst, IRNext(block->end_code));
    inst->block = block;
    block->end_code = inst;
  }
  return inst;
}

static IRNode* MaterializeInvariant(Generator* gen, LoopInfo* loop,
                                    IRNode* value) {
  if (IRIsConst(value) ||
      (value->block != NULL && !LoopInfoContainsBlock(loop, value->block))) {
    return value;
  }
  if (!IRIsLoadOnly(value) || value->inputs.length == 0) {
    return NULL;
  }
  IRNode* load = IRSetType(NewIR1(value->opcode, value->inputs.value.p[0]),
                           value->type);
  if (IRIsVarRef(value)) {
    IRSetVarUse(load, value->var.use);
  }
  return EmitBeforeTerminator(gen, loop->preheader, load);
}

static void EmitAt(Generator* gen, BasicBlock* block, IRNode* before,
                   IRNode* inst) {
  if (before != NULL) {
    BasicBlockEmitBefore(gen, block, inst, before);
  } else {
    EmitBeforeTerminator(gen, block, inst);
  }
}

static IRNode* MaterializeSplatAt(Generator* gen, BasicBlock* block,
                                 IRNode* before, TypeRecord* vector_type,
                                 TypeRecord* element, int vf,
                                 IRNode* lane_value) {
  if (lane_value == NULL || block == NULL) {
    return NULL;
  }
  Symbol* temporary = SyntaxNewTemporary(gen->syntax, vector_type);
  temporary->flags.address_taken = true;
  IRNode* object = GeneratorGetVariable(gen, temporary);
  object->block = gen->entry_block;
  for (int lane = 0; lane < vf; lane++) {
    IRNode* address = object;
    if (lane != 0) {
      address = IRSetType(
          NewIR2(IR_OP(adda), object,
                 GeneratorGetIntConstant(gen, NULL, lane * element->size)),
          NewPointerTo(kQualPlain, element));
      EmitAt(gen, block, before, address);
    }
    IRNode* store =
        NewIR2(GetStoreOpcodeForType(element), address, lane_value);
    EmitAt(gen, block, before, store);
  }
  return object;
}

static IRNode* MaterializeSplat(Generator* gen, LoopInfo* loop,
                               TypeRecord* vector_type, TypeRecord* element,
                               int vf, IRNode* lane_value) {
  IRNode* value = MaterializeInvariant(gen, loop, lane_value);
  if (value == NULL) {
    return NULL;
  }
  return MaterializeSplatAt(gen, loop->preheader, NULL, vector_type, element,
                            vf, value);
}

static void ScalePointerUpdate(Generator* gen, IRNode* update, int vf) {
  if (update == NULL || update->inputs.length < 2 ||
      !IRIsIntConst(update->inputs.value.p[1])) {
    return;
  }
  int64_t scale = IRIntConstValue(update->inputs.value.p[1]);
  IRNode* scale_node = update->inputs.value.p[1];
  IRReplaceInput(update, 1,
                 GeneratorGetIntConstant(gen, scale_node->type, scale * vf));
}

static void ScaleUniquePointer(Generator* gen, IRNode* update, int vf,
                               IRNode** seen, size_t* seen_count) {
  if (update == NULL) {
    return;
  }
  for (size_t i = 0; i < *seen_count; i++) {
    if (seen[i] == update) {
      return;
    }
  }
  seen[(*seen_count)++] = update;
  ScalePointerUpdate(gen, update, vf);
}

static bool VectorizeLoop(Generator* gen, LoopInfo* loop,
                          const CountedLoop* counted,
                          const VectorCandidate* cand) {
  TypeRecord* vector_type = NewVectorTypeRecord(cand->element, cand->vf);
  IRNode* left_address = cand->left_splat ? NULL : cand->left.address;
  IRNode* right_address = cand->right_splat ? NULL : cand->right.address;
  if (cand->left_splat) {
    left_address = MaterializeSplat(gen, loop, vector_type, cand->element,
                                   cand->vf, cand->left_value);
  }
  if (cand->right_splat) {
    right_address = MaterializeSplat(gen, loop, vector_type, cand->element,
                                    cand->vf, cand->right_value);
  }
  if (left_address == NULL || right_address == NULL) {
    return false;
  }

  IRNode* vector_op =
      IRSetType(NewIR3(cand->vector_opcode, cand->dest.address, left_address,
                       right_address),
                vector_type);
  vector_op->aux = vector_type;
  BasicBlockEmitBefore(gen, cand->store->block, vector_op, cand->store);

  BasicBlock* store_block = cand->store->block;
  BasicBlockRemoveInstruction(gen, store_block, cand->store);
  if (cand->binop->outputs.length == 0) {
    BasicBlockRemoveInstruction(gen, cand->binop->block, cand->binop);
  }
  if (cand->left_load != NULL && cand->left_load->outputs.length == 0) {
    BasicBlockRemoveInstruction(gen, cand->left_load->block, cand->left_load);
  }
  if (cand->right_load != NULL && cand->right_load->outputs.length == 0) {
    BasicBlockRemoveInstruction(gen, cand->right_load->block, cand->right_load);
  }

  IRNode* iv_step = counted->update->inputs.value.p[1];
  IRReplaceInput(counted->update, 1,
                 GeneratorGetIntConstant(gen, iv_step->type, cand->vf));

  IRNode* scaled[4] = {0};
  size_t scaled_count = 0;
  ScaleUniquePointer(gen, cand->dest.pointer_update, cand->vf, scaled,
                     &scaled_count);
  ScaleUniquePointer(gen, cand->left.pointer_update, cand->vf, scaled,
                     &scaled_count);
  ScaleUniquePointer(gen, cand->right.pointer_update, cand->vf, scaled,
                     &scaled_count);

  RebuildVariableMetadata(loop->preheader);
  BasicBlock* block = counted->body;
  while (block != loop->header) {
    RebuildVariableMetadata(block);
    BasicBlock* successor;
    if (!UniqueSuccessor(gen, block, &successor)) {
      break;
    }
    block = successor;
  }
  RebuildVariableMetadata(loop->header);
  return true;
}

enum { kSLPPackNodeCap = 256 };

typedef struct {
  IRNode* store;
  IRNode* dest_addr;
  IRNode* dest_base;
  IRNode* binop;
  IRNode* left;
  IRNode* right;
  IRNode* left_load;
  IRNode* right_load;
  Symbol* dest_obj;
  int64_t dest_off;
  TypeRecord* element;
  int elem_size;
  IROpcode vector_opcode;
} SLPLane;

typedef struct {
  IRNode* address;
  IRNode* base;
  Symbol* object;
  int64_t offset;
} ConstAccess;

typedef struct {
  bool splat;
  IRNode* splat_value;
  Symbol* object;
  int64_t first_off;
  IRNode* first_address;
} SLPOperand;

static bool DecodeConstAddress(IRNode* address, ConstAccess* out) {
  ConstAccess found = {0};
  found.address = address;
  if (address == NULL) {
    return false;
  }
  IRNode* base = address;
  int64_t offset = 0;
  if (address->opcode == IR_OP(adda) && address->inputs.length == 2) {
    IRNode* left = address->inputs.value.p[0];
    IRNode* right = address->inputs.value.p[1];
    if (IRIsIntConst(right)) {
      base = left;
      offset = IRIntConstValue(right);
    } else if (IRIsIntConst(left)) {
      base = right;
      offset = IRIntConstValue(left);
    } else {
      return false;
    }
  }
  base = PeelInvariantBase(base);
  if (!IsSafeArrayObject(base)) {
    return false;
  }
  found.base = base;
  found.object = VariableSymbol(base);
  found.offset = offset;
  if (found.object == NULL) {
    return false;
  }
  *out = found;
  return true;
}

static bool SameSplatValue(IRNode* a, IRNode* b) {
  if (a == b) {
    return true;
  }
  if (a == NULL || b == NULL) {
    return false;
  }
  if (IRIsIntConst(a) && IRIsIntConst(b)) {
    return IRIntConstValue(a) == IRIntConstValue(b);
  }
  if (IRIsConst(a) && IRIsConst(b) && !IRIsIntConst(a) && !IRIsIntConst(b)) {
    return ((IRConstant*)a)->value.fvalue == ((IRConstant*)b)->value.fvalue;
  }
  return false;
}

static bool OpcodeIsCommutative(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(addi):
    case IR_OP(andi):
    case IR_OP(ori):
    case IR_OP(xori):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(mulf):
    case IR_OP(muld):
      return true;
    default:
      return false;
  }
}

static bool AnalyzeSLPStore(IRNode* store, SLPLane* lane) {
  SLPLane found = {0};
  if (!IRIsStoreOnly(store) || store->inputs.length < 2) {
    return false;
  }
  found.elem_size = MemoryWidth(store);
  found.binop = store->inputs.value.p[1];
  if (found.elem_size <= 0 || found.binop == NULL ||
      found.binop->inputs.length != 2 || found.binop->block != store->block) {
    return false;
  }
  found.element = found.binop->type;
  if (found.element == NULL || found.element->size != found.elem_size ||
      TypeIsVolatile(found.element) || TypeIsAtomic(found.element) ||
      TypeIsBitInt(found.element)) {
    return false;
  }
  found.vector_opcode =
      VectorOpcodeForBinop(found.binop->opcode, found.element);
  if (!OpcodeIsNative(found.vector_opcode, found.element)) {
    return false;
  }
  ConstAccess dest;
  if (!DecodeConstAddress(store->inputs.value.p[0], &dest)) {
    return false;
  }
  if (dest.offset < 0 || (dest.offset % found.elem_size) != 0) {
    return false;
  }
  if (dest.object->type != NULL && dest.object->type->size > 0 &&
      dest.offset + found.elem_size > dest.object->type->size) {
    return false;
  }
  found.store = store;
  found.dest_addr = dest.address;
  found.dest_base = dest.base;
  found.dest_obj = dest.object;
  found.dest_off = dest.offset;
  found.left = found.binop->inputs.value.p[0];
  found.right = found.binop->inputs.value.p[1];
  if (IRIsLoadOnly(found.left)) {
    found.left_load = found.left;
  }
  if (IRIsLoadOnly(found.right)) {
    found.right_load = found.right;
  }
  *lane = found;
  return true;
}

static bool ClassifyOperand(IRNode* value, int elem_size, SLPOperand* op) {
  SLPOperand found = {0};
  if (value == NULL || elem_size <= 0) {
    return false;
  }
  if (IRIsLoadOnly(value) && MemoryWidth(value) == elem_size &&
      value->inputs.length >= 1) {
    ConstAccess access;
    if (DecodeConstAddress(value->inputs.value.p[0], &access)) {
      found.object = access.object;
      found.first_off = access.offset;
      found.first_address = access.address;
      *op = found;
      return true;
    }
  }
  if (!IRIsConst(value) && !IRIsLoadOnly(value) && !IRIsVariable(value)) {
    return false;
  }
  found.splat = true;
  found.splat_value = value;
  *op = found;
  return true;
}

static bool MatchOperand(const SLPLane* lane, bool use_left,
                         const SLPOperand* expected, int index, int elem_size) {
  IRNode* value = use_left ? lane->left : lane->right;
  if (expected->splat) {
    return SameSplatValue(value, expected->splat_value);
  }
  if (!IRIsLoadOnly(value) || MemoryWidth(value) != elem_size ||
      value->inputs.length == 0) {
    return false;
  }
  ConstAccess access;
  if (!DecodeConstAddress(value->inputs.value.p[0], &access)) {
    return false;
  }
  return access.object == expected->object &&
         access.offset == expected->first_off + (int64_t)index * elem_size;
}

static bool MatchLaneOperands(const SLPLane* lane, int index,
                              const SLPOperand* left, const SLPOperand* right,
                              bool commutative, int elem_size) {
  if (MatchOperand(lane, true, left, index, elem_size) &&
      MatchOperand(lane, false, right, index, elem_size)) {
    return true;
  }
  return commutative &&
         MatchOperand(lane, true, right, index, elem_size) &&
         MatchOperand(lane, false, left, index, elem_size);
}

static bool NodeInList(IRNode** list, size_t n, IRNode* node) {
  for (size_t i = 0; i < n; i++) {
    if (list[i] == node) {
      return true;
    }
  }
  return false;
}

static void AddPackNode(IRNode** list, size_t* n, IRNode* node) {
  if (node == NULL || NodeInList(list, *n, node)) {
    return;
  }
  if (*n < kSLPPackNodeCap) {
    list[(*n)++] = node;
  }
}

static bool HarmlessAddressOp(IRNode* inst) {
  if (inst == NULL) {
    return false;
  }
  switch (inst->opcode) {
    case IR_OP(label):
    case IR_OP(loc):
    case IR_OP(nop):
    case IR_OP(adda):
    case IR_OP(muli):
    case IR_OP(lsli):
    case IR_OP(signextendi):
    case IR_OP(zeroextendi):
    case IR_OP(cast):
    case IR_OP(addressof):
      return true;
    default:
      return IRIsConst(inst) || IRIsVariable(inst);
  }
}

static bool InstPrecedes(BasicBlock* block, IRNode* a, IRNode* b) {
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (inst == a) {
      return inst != b;
    }
    if (inst == b) {
      return false;
    }
  }
  return false;
}

static bool BinopHasPackDependence(const SLPLane* lanes, int vf) {
  for (int i = 0; i < vf; i++) {
    IRNode* binop = lanes[i].binop;
    for (size_t u = 0; u < binop->outputs.length; u++) {
      IRNode* user = binop->outputs.value.p[u];
      if (user == lanes[i].store) {
        continue;
      }
      for (int j = 0; j < vf; j++) {
        if (user == lanes[j].store || user == lanes[j].binop ||
            user == lanes[j].left || user == lanes[j].right) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool ExtraUsePrecedesFirstStore(const SLPLane* lanes, int vf,
                                       BasicBlock* block, IRNode* first_store) {
  for (int i = 0; i < vf; i++) {
    IRNode* binop = lanes[i].binop;
    for (size_t u = 0; u < binop->outputs.length; u++) {
      IRNode* user = binop->outputs.value.p[u];
      if (user == lanes[i].store) {
        continue;
      }
      if (user->block == block && InstPrecedes(block, user, first_store)) {
        return true;
      }
    }
  }
  return false;
}

static bool BinopHasExtraUses(const SLPLane* lane) {
  for (size_t u = 0; u < lane->binop->outputs.length; u++) {
    if (lane->binop->outputs.value.p[u] != lane->store) {
      return true;
    }
  }
  return false;
}

static void ReplaceUsesExcept(IRNode* old, IRNode* replacement, IRNode* except) {
  for (;;) {
    IRNode* user = NULL;
    for (size_t i = 0; i < old->outputs.length; i++) {
      if (old->outputs.value.p[i] != except) {
        user = old->outputs.value.p[i];
        break;
      }
    }
    if (user == NULL) {
      return;
    }
    bool replaced = false;
    for (size_t j = 0; j < user->inputs.length; j++) {
      if (user->inputs.value.p[j] == old) {
        IRReplaceInput(user, j, replacement);
        replaced = true;
        break;
      }
    }
    if (!replaced) {
      return;
    }
  }
}

static bool WindowIsPackable(BasicBlock* block, const SLPLane* lanes, int vf) {
  IRNode* pack_nodes[kSLPPackNodeCap];
  size_t pack_count = 0;
  for (int i = 0; i < vf; i++) {
    AddPackNode(pack_nodes, &pack_count, lanes[i].store);
    AddPackNode(pack_nodes, &pack_count, lanes[i].binop);
    AddPackNode(pack_nodes, &pack_count, lanes[i].dest_addr);
    if (lanes[i].left_load != NULL) {
      AddPackNode(pack_nodes, &pack_count, lanes[i].left_load);
      if (lanes[i].left_load->inputs.length > 0) {
        AddPackNode(pack_nodes, &pack_count,
                    lanes[i].left_load->inputs.value.p[0]);
      }
    }
    if (lanes[i].right_load != NULL) {
      AddPackNode(pack_nodes, &pack_count, lanes[i].right_load);
      if (lanes[i].right_load->inputs.length > 0) {
        AddPackNode(pack_nodes, &pack_count,
                    lanes[i].right_load->inputs.value.p[0]);
      }
    }
  }
  IRNode* last_store = lanes[vf - 1].store;
  bool in_window = false;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (!in_window) {
      if (!NodeInList(pack_nodes, pack_count, inst)) {
        continue;
      }
      in_window = true;
    }
    if (!NodeInList(pack_nodes, pack_count, inst) && !HarmlessAddressOp(inst)) {
      return false;
    }
    if (inst == last_store) {
      return true;
    }
  }
  return false;
}

static bool TryPackLanes(Generator* gen, BasicBlock* block, SLPLane* lanes,
                         int vf) {
  if (vf < 2) {
    return false;
  }
  const SLPLane* first = &lanes[0];
  if (kNativeVectorBytes % first->elem_size != 0 ||
      vf != kNativeVectorBytes / first->elem_size) {
    return false;
  }
  if (first->dest_obj->type != NULL && first->dest_obj->type->size > 0 &&
      first->dest_off + (int64_t)vf * first->elem_size >
          first->dest_obj->type->size) {
    return false;
  }
  for (int i = 0; i < vf; i++) {
    if (lanes[i].elem_size != first->elem_size ||
        lanes[i].vector_opcode != first->vector_opcode ||
        lanes[i].dest_obj != first->dest_obj ||
        lanes[i].binop->opcode != first->binop->opcode ||
        lanes[i].dest_off !=
            first->dest_off + (int64_t)i * first->elem_size) {
      return false;
    }
  }

  SLPOperand left;
  SLPOperand right;
  if (!ClassifyOperand(first->left, first->elem_size, &left) ||
      !ClassifyOperand(first->right, first->elem_size, &right)) {
    return false;
  }
  if (left.splat && right.splat) {
    return false;
  }
  bool commutative = OpcodeIsCommutative(first->binop->opcode);
  for (int i = 0; i < vf; i++) {
    if (!MatchLaneOperands(&lanes[i], i, &left, &right, commutative,
                           first->elem_size)) {
      return false;
    }
  }
  if (!left.splat && left.object == first->dest_obj &&
      left.first_off != first->dest_off) {
    return false;
  }
  if (!right.splat && right.object == first->dest_obj &&
      right.first_off != first->dest_off) {
    return false;
  }
  IRNode* first_store = first->store;
  if (BinopHasPackDependence(lanes, vf) ||
      ExtraUsePrecedesFirstStore(lanes, vf, block, first_store) ||
      !WindowIsPackable(block, lanes, vf)) {
    return false;
  }

  TypeRecord* vector_type = NewVectorTypeRecord(first->element, vf);
  IRNode* left_address = left.first_address;
  IRNode* right_address = right.first_address;
  if (left.splat) {
    left_address = MaterializeSplatAt(gen, block, first_store, vector_type,
                                      first->element, vf, left.splat_value);
  }
  if (right.splat) {
    right_address = MaterializeSplatAt(gen, block, first_store, vector_type,
                                       first->element, vf, right.splat_value);
  }
  if (left_address == NULL || right_address == NULL) {
    return false;
  }

  IRNode* vector_op =
      IRSetType(NewIR3(first->vector_opcode, first->dest_addr, left_address,
                       right_address),
                vector_type);
  vector_op->aux = vector_type;
  BasicBlockEmitBefore(gen, block, vector_op, first_store);

  for (int i = 0; i < vf; i++) {
    if (!BinopHasExtraUses(&lanes[i])) {
      continue;
    }
    IRNode* address = lanes[i].dest_base;
    if (lanes[i].dest_off != 0) {
      address = IRSetType(
          NewIR2(IR_OP(adda), lanes[i].dest_base,
                 GeneratorGetIntConstant(gen, NULL, lanes[i].dest_off)),
          NewPointerTo(kQualPlain, first->element));
      BasicBlockEmitBefore(gen, block, address, first_store);
    }
    IRNode* reload =
        IRSetType(NewIR1(GetLoadOpcodeForType(first->element), address),
                  first->element);
    IRSetVarUse(reload, first->dest_obj);
    BasicBlockEmitBefore(gen, block, reload, first_store);
    ReplaceUsesExcept(lanes[i].binop, reload, lanes[i].store);
  }

  for (int i = 0; i < vf; i++) {
    BasicBlockRemoveInstruction(gen, block, lanes[i].store);
    if (lanes[i].binop->outputs.length == 0) {
      BasicBlockRemoveInstruction(gen, lanes[i].binop->block, lanes[i].binop);
    }
  }
  IRNode* dead_loads[kSLPPackNodeCap];
  size_t dead_count = 0;
  for (int i = 0; i < vf; i++) {
    IRNode* operands[2] = {lanes[i].left, lanes[i].right};
    for (int o = 0; o < 2; o++) {
      IRNode* value = operands[o];
      if (value == NULL || !IRIsLoadOnly(value) || value->outputs.length != 0 ||
          value->inputs.length == 0) {
        continue;
      }
      ConstAccess access;
      if (!DecodeConstAddress(value->inputs.value.p[0], &access)) {
        continue;
      }
      AddPackNode(dead_loads, &dead_count, value);
    }
  }
  for (size_t i = 0; i < dead_count; i++) {
    BasicBlockRemoveInstruction(gen, dead_loads[i]->block, dead_loads[i]);
  }
  RebuildVariableMetadata(block);
  return true;
}

static bool SLPVectorizeBlock(Generator* gen, BasicBlock* block) {
  bool changed = false;
  for (;;) {
    size_t n = 0;
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      SLPLane probe;
      if (AnalyzeSLPStore(inst, &probe)) {
        n++;
      }
    }
    if (n < 2) {
      return changed;
    }
    SLPLane* lanes = calloc(n, sizeof(SLPLane));
    if (lanes == NULL) {
      return changed;
    }
    size_t filled = 0;
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (filled < n && AnalyzeSLPStore(inst, &lanes[filled])) {
        filled++;
      }
    }
    bool packed = false;
    for (size_t start = 0; start + 2 <= filled; start++) {
      int elem_size = lanes[start].elem_size;
      if (elem_size <= 0 || kNativeVectorBytes % elem_size != 0) {
        continue;
      }
      int vf = kNativeVectorBytes / elem_size;
      if (vf < 2 || start + (size_t)vf > filled) {
        continue;
      }
      if (TryPackLanes(gen, block, lanes + start, vf)) {
        packed = true;
        changed = true;
        break;
      }
    }
    free(lanes);
    if (!packed) {
      return changed;
    }
  }
}

static void SLPVectorize(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    if (block == NULL || BasicBlockIsEmpty(block)) {
      continue;
    }
    SLPVectorizeBlock(gen, block);
  }
}

void AutoVectorizeOptimization(Generator* gen) {
  if (!OptLevel2() || !TargetSupportsAutoVectorization()) {
    return;
  }

  if (gen->loops.length != 0) {
    Vector candidates;
    VectorInit(&candidates);
    for (size_t i = 0; i < gen->loops.length; i++) {
      VectorAppend(&candidates, gen->loops.value.p[i]);
    }

    for (size_t i = 0; i < candidates.length; i++) {
      LoopInfo* loop = candidates.value.p[i];
      CountedLoop counted;
      VectorCandidate cand;
      if (!AnalyzeCountedLoop(gen, loop, &counted) ||
          !AnalyzeVectorBody(gen, loop, &counted, &cand)) {
        continue;
      }
      VectorizeLoop(gen, loop, &counted, &cand);
    }
    VectorDestruct(&candidates);
  }

  SLPVectorize(gen);
}
