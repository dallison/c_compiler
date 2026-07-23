//
//  induction.c
//  c_compiler
//
//  Canonical induction analysis and conservative derived-address reduction.
//

#include "induction.h"

#include "loop_info.h"

typedef struct {
  Symbol* symbol;
  IRNode* update;
  BasicBlock* latch;
  bool decrement;
} CanonicalInductionVariable;

typedef struct {
  IRNode* address;
  IRNode* base;
  IRNode* scale_constant;
  BasicBlock* block;
} DerivedAddress;

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

static void RecordDefinition(BasicBlock* block, Symbol* symbol) {
  MapKeyValue kv = {.key.p = symbol, .value.p = NULL};
  MapInsert(&block->defined_vars, kv);
}

static void RecordReference(BasicBlock* block, Symbol* symbol) {
  SetInsert(&block->referenced_vars, symbol);
}

static void RebuildVariableMetadata(BasicBlock* block) {
  MapClear(&block->defined_vars);
  SetClear(&block->referenced_vars);
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (IRIsVarDef(inst)) {
      RecordDefinition(block, inst->var.def);
    }
    if (IRIsVarRef(inst)) {
      RecordReference(block, inst->var.use);
    }
  }
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

static bool IsIntegerDecrement(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(dec8):
    case IR_OP(dec16):
    case IR_OP(dec32):
    case IR_OP(dec64):
    case IR_OP(udec8):
    case IR_OP(udec16):
    case IR_OP(udec32):
    case IR_OP(udec64):
      return true;
    default:
      return false;
  }
}

static IRNode* LastDefinitionInBlock(BasicBlock* block,
                                     const Symbol* symbol) {
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

static bool IsIntegerInductionUpdate(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(uinc8):
    case IR_OP(uinc16):
    case IR_OP(uinc32):
    case IR_OP(uinc64):
    case IR_OP(inca):
    case IR_OP(dec8):
    case IR_OP(dec16):
    case IR_OP(dec32):
    case IR_OP(dec64):
    case IR_OP(udec8):
    case IR_OP(udec16):
    case IR_OP(udec32):
    case IR_OP(udec64):
    case IR_OP(deca):
      return true;
    default:
      return false;
  }
}

static bool FindCanonicalInductionVariable(
    Generator* gen, LoopInfo* loop, CanonicalInductionVariable* result) {
  if (loop->parent != NULL || loop->children.length != 0 ||
      loop->preheader == NULL || BitSetCount(&loop->latches) != 1) {
    return false;
  }
  BlockId latch_id = BitSetFindFirstSet(&loop->latches);
  BasicBlock* latch = VectorGet(&gen->basic_blocks, latch_id);
  CanonicalInductionVariable found = {0};
  for (IRNode* inst = BasicBlockBegin(latch);
       !BasicBlockIsEmpty(latch) && inst != BasicBlockEnd(latch);
       inst = IRNext(inst)) {
    if ((!IsIntegerIncrement(inst) && !IsIntegerDecrement(inst)) ||
        !IRIsVarDef(inst) || inst->inputs.length < 2 ||
        !IRIsIntConst(inst->inputs.value.p[1]) ||
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
    found.latch = latch;
    found.decrement = IsIntegerDecrement(inst);
  }
  if (found.update == NULL) {
    return false;
  }
  // Pointer arithmetic cannot be speculated for a zero-iteration loop if the
  // initial index might be outside the source object.  Limit this first
  // transform to the common, provably safe base + 0 starting address.
  if (found.decrement ||
      !HasZeroInitialValue(gen, loop, found.symbol)) {
    return false;
  }
  *result = found;
  return true;
}

static bool IsMemoryAddressUser(IRNode* inst, IRNode* address) {
  if (inst->dest != NULL || inst->inputs.length == 0 ||
      inst->inputs.value.p[0] != address) {
    return false;
  }
  switch (inst->opcode) {
    case IR_OP(load8):
    case IR_OP(load16):
    case IR_OP(load32):
    case IR_OP(load64):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadu32):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store32):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return true;
    default:
      return false;
  }
}

static bool IsInvariantBase(Generator* gen, const LoopInfo* loop,
                            IRNode* base) {
  Symbol* symbol = VariableSymbol(base);
  if (symbol != NULL) {
    if (TypeIsVolatile(base->type)) {
      return false;
    }
    // The address of an array is invariant even when its elements are written.
    return TypeIsArray(base->type) ||
           !LoopDefinesSymbol(gen, loop, symbol, NULL);
  }
  if ((base->opcode != IR_OP(loada) &&
       base->opcode != IR_OP(addressof)) ||
      base->dest != NULL || base->inputs.length != 1 ||
      IRHasSideEffects(base)) {
    return false;
  }
  symbol = VariableSymbol(base->inputs.value.p[0]);
  bool can_speculate =
      base->opcode == IR_OP(addressof) ||
      IRIsArgument(base->inputs.value.p[0]) ||
      IRIsStaticVariable(base->inputs.value.p[0]);
  return symbol != NULL && can_speculate && !symbol->flags.address_taken &&
         !TypeIsVolatile(base->type) &&
         !LoopDefinesSymbol(gen, loop, symbol, NULL);
}

static bool MatchDerivedAddress(Generator* gen, const LoopInfo* loop,
                                const CanonicalInductionVariable* iv,
                                IRNode* address, DerivedAddress* result) {
  if (address->opcode != IR_OP(adda) || address->dest != NULL ||
      address->inputs.length != 2 || address->outputs.length != 1) {
    return false;
  }
  IRNode* scale = NULL;
  IRNode* base = NULL;
  for (size_t i = 0; i < 2; i++) {
    IRNode* candidate = address->inputs.value.p[i];
    if (candidate->opcode == IR_OP(muli)) {
      scale = candidate;
      base = address->inputs.value.p[1 - i];
      break;
    }
  }
  if (scale == NULL || scale->dest != NULL || scale->inputs.length != 2 ||
      scale->outputs.length != 1 || !IsInvariantBase(gen, loop, base)) {
    return false;
  }

  IRNode* index_load = NULL;
  IRNode* scale_constant = NULL;
  for (size_t i = 0; i < 2; i++) {
    IRNode* candidate = scale->inputs.value.p[i];
    if (IRIsIntConst(candidate)) {
      scale_constant = candidate;
    } else {
      index_load = candidate;
    }
  }
  if (index_load == NULL || scale_constant == NULL ||
      IRIntConstValue(scale_constant) <= 0 || index_load->dest != NULL ||
      index_load->inputs.length != 1 || index_load->outputs.length != 1 ||
      !IRIsLoadOnly(index_load) || IRHasSideEffects(index_load) ||
      VariableSymbol(index_load->inputs.value.p[0]) != iv->symbol ||
      !IRIsVarRef(index_load) || index_load->var.use != iv->symbol) {
    return false;
  }
  IRNode* user = address->outputs.value.p[0];
  if (!IsMemoryAddressUser(user, address)) {
    return false;
  }

  result->address = address;
  result->base = base;
  result->scale_constant = scale_constant;
  result->block = address->block;
  return true;
}

static bool FindDerivedAddress(Generator* gen, const LoopInfo* loop,
                               const CanonicalInductionVariable* iv,
                               DerivedAddress* result) {
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (MatchDerivedAddress(gen, loop, iv, inst, result)) {
        return true;
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

static IRNode* EmitBeforeTerminator(Generator* gen, BasicBlock* block,
                                    IRNode* inst) {
  if (IRIsBranch(block->end_code)) {
    BasicBlockEmitBefore(gen, block, inst, block->end_code);
  } else {
    // A fallthrough preheader has no terminator.  Append after its final
    // semantic instruction so initializers already in the block run first.
    GeneratorEmitBefore(gen, inst, IRNext(block->end_code));
    inst->block = block;
    block->end_code = inst;
  }
  return inst;
}

static IRNode* MaterializeBaseInPreheader(Generator* gen, LoopInfo* loop,
                                          IRNode* base) {
  if (VariableSymbol(base) != NULL) {
    return base;
  }
  IRNode* clone = IRSetType(NewIR1(base->opcode, base->inputs.value.p[0]),
                            base->type);
  if (IRIsVarRef(base)) {
    IRSetVarUse(clone, base->var.use);
    RecordReference(loop->preheader, base->var.use);
  }
  return EmitBeforeTerminator(gen, loop->preheader, clone);
}

static void StrengthReduceDerivedAddress(
    Generator* gen, LoopInfo* loop, const CanonicalInductionVariable* iv,
    const DerivedAddress* derived) {
  Symbol* pointer =
      SyntaxNewTemporary(gen->syntax, TypeRecordCopy(derived->address->type));
  IRNode* pointer_variable = GeneratorGetVariable(gen, pointer);
  // GeneratorGetVariable normally runs while the entry block is being built.
  // This optimizer runs later, but the pooled variable is still inserted in
  // the entry prefix and must participate in block-aware SSA traversal.
  pointer_variable->block = gen->entry_block;

  IRNode* initial_base =
      MaterializeBaseInPreheader(gen, loop, derived->base);

  IRNode* initialize =
      NewIR2(IR_OP(storea), pointer_variable, initial_base);
  IRSetVarDef(initialize, pointer);
  EmitBeforeTerminator(gen, loop->preheader, initialize);
  RecordDefinition(loop->preheader, pointer);

  IRNode* pointer_load =
      IRSetType(NewIR1(IR_OP(loada), pointer_variable),
                derived->address->type);
  IRSetVarUse(pointer_load, pointer);
  BasicBlockEmitBefore(gen, derived->block, pointer_load, derived->address);
  RecordReference(derived->block, pointer);
  IRNode* memory_user = derived->address->outputs.value.p[0];
  // The original subscript annotation names the source array/pointer by
  // following its address expression.  That path now ends at |pointer|, so
  // leave aliasing to the explicit memory operation instead of asking SSA to
  // rename an unrelated symbol through the new address.
  memory_user->flags &= ~(kIRVarUse | kIRVarDef);
  GeneratorReplaceInstruction(gen, derived->address, pointer_load);

  IRNode* pointer_update = NewIR2(
      iv->decrement ? IR_OP(deca) : IR_OP(inca), pointer_variable,
      derived->scale_constant);
  IRSetType(pointer_update, derived->address->type);
  IRSetVarDef(pointer_update, pointer);
  EmitBeforeTerminator(gen, iv->latch, pointer_update);
  RecordDefinition(iv->latch, pointer);
  RebuildVariableMetadata(derived->block);
}

void DerivedInductionVariableOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->loops.length; i++) {
    LoopInfo* loop = gen->loops.value.p[i];
    CanonicalInductionVariable iv;
    DerivedAddress derived;
    if (FindCanonicalInductionVariable(gen, loop, &iv) &&
        FindDerivedAddress(gen, loop, &iv, &derived)) {
      StrengthReduceDerivedAddress(gen, loop, &iv, &derived);
    }
  }
}

static bool HeaderHasInductionPhi(const LoopInfo* loop,
                                  const Symbol* symbol) {
  BasicBlock* header = loop->header;
  for (IRNode* inst = BasicBlockBegin(header);
       !BasicBlockIsEmpty(header) && inst != BasicBlockEnd(header);
       inst = IRNext(inst)) {
    if (inst->opcode == IR_OP(phi) && IRIsVarDef(inst) &&
        inst->var.def == symbol && inst->inputs.length == 2) {
      return true;
    }
    if (inst->opcode != IR_OP(ssavar) && inst->opcode != IR_OP(phi) &&
        !IRIsVariable(inst)) {
      break;
    }
  }
  return false;
}

static IRNode* FindReloadAfterUpdate(IRNode* update) {
  BasicBlock* block = update->block;
  IRNode* address = update->inputs.value.p[0];
  for (IRNode* inst = IRNext(update);
       inst != NULL && inst->block == block && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (IRIsLoadOnly(inst) && inst->inputs.length > 0 &&
        inst->inputs.value.p[0] == address && IRIsVarRef(inst) &&
        inst->var.use == update->var.def && inst->dest == NULL &&
        !IRHasSideEffects(inst) && TypeEqual(inst->type, update->type)) {
      return inst;
    }
    if (IRHasSideEffects(inst)) {
      return NULL;
    }
  }
  return NULL;
}

static bool FeedsAddressCalculation(IRNode* value) {
  for (size_t i = 0; i < value->outputs.length; i++) {
    IRNode* user = value->outputs.value.p[i];
    if (user->opcode == IR_OP(adda)) {
      return true;
    }
    if (user->opcode != IR_OP(muli) && user->opcode != IR_OP(lsli)) {
      continue;
    }
    for (size_t j = 0; j < user->outputs.length; j++) {
      if (((IRNode*)user->outputs.value.p[j])->opcode == IR_OP(adda)) {
        return true;
      }
    }
  }
  return false;
}

static bool HasScaledAddressUse(Generator* gen, const LoopInfo* loop,
                                const Symbol* symbol) {
  BitSetIterator it;
  BitSetIteratorStart(&it, (BitSet*)&loop->blocks);
  while (!BitSetIteratorDone(&it)) {
    BasicBlock* block =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&it));
    for (IRNode* inst = BasicBlockBegin(block);
         !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
         inst = IRNext(inst)) {
      if (IRIsLoadOnly(inst) && IRIsVarRef(inst) &&
          inst->var.use == symbol && FeedsAddressCalculation(inst)) {
        return true;
      }
    }
    BitSetIteratorNext(&it);
  }
  return false;
}

static void OptimizeLoop(Generator* gen, LoopInfo* loop) {
  // Keep the first milestone pressure-neutral.  Reusing update results in a
  // nested loop extends values across another loop's live range and currently
  // exposes backend allocator aliasing on several targets.
  if (loop->parent != NULL || loop->children.length != 0) {
    return;
  }
  BitSetIterator latch_it;
  BitSetIteratorStart(&latch_it, &loop->latches);
  while (!BitSetIteratorDone(&latch_it)) {
    BasicBlock* latch =
        VectorGet(&gen->basic_blocks, BitSetIteratorValue(&latch_it));
    IRNode* next = NULL;
    for (IRNode* inst = BasicBlockBegin(latch);
         !BasicBlockIsEmpty(latch) && inst != BasicBlockEnd(latch);
         inst = next) {
      next = IRNext(inst);
      if (!IsIntegerInductionUpdate(inst) || !IRIsVarDef(inst) ||
          inst->inputs.length < 2 || !IRIsIntConst(inst->inputs.value.p[1]) ||
          !HeaderHasInductionPhi(loop, inst->var.def) ||
          HasScaledAddressUse(gen, loop, inst->var.def)) {
        continue;
      }
      IRNode* reload = FindReloadAfterUpdate(inst);
      if (reload == NULL || reload->outputs.length == 0) {
        continue;
      }
      if (next == reload) {
        next = IRNext(reload);
      }
      GeneratorReplaceInstruction(gen, reload, inst);
      BasicBlockRemoveInstruction(gen, latch, reload);
    }
    BitSetIteratorNext(&latch_it);
  }
}

void InductionVariableOptimization(Generator* gen) {
  for (size_t i = 0; i < gen->loops.length; i++) {
    OptimizeLoop(gen, gen->loops.value.p[i]);
  }
}
