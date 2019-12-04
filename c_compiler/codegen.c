//
//  codegen.c
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include "assembler.h"
#include "ast.h"
#include "compiler.h"
#include "errors.h"
#include "gvn.h"
#include "list.h"
#include "optimizer.h"
#include "ssa.h"
#include "statement_codegen.h"
#include <assert.h>

void GeneratorInit(Generator* gen, Syntax* syntax, TypeRecord* func) {
  gen->syntax = syntax;
  gen->func = func;
  TypeRecordIncRef(func);
  ListInit(&gen->code);
  gen->last_constant = NULL;
  gen->last_variable = NULL;
  gen->break_label = NULL;
  gen->continue_label = NULL;
  gen->struct_return_value = NULL;
  gen->current_struct_address = NULL;
  gen->return_label = NULL;
  VectorInit(&gen->int_constant_pool);
  VectorInit(&gen->fp_constant_pool);
  VectorInit(&gen->variable_pool);
  VectorInit(&gen->basic_blocks);

  IRResetNodeId();
}

static void PrintIR(ListElement* hdr) {
  IRNode* node = (IRNode*)hdr;
  IRPrint(node);
}

void GeneratorPrintIR(Generator* gen) { ListTraverse(&gen->code, PrintIR); }

static void DestructIRNode(ListElement* hdr) {
  IRNode* node = (IRNode*)hdr;
  IRDestruct(node);
}

void GeneratorDestruct(Generator* gen) {
  TypeRecordDelete(gen->func);

  // Delete all IR nodes.
  ListTraverse(&gen->code, DestructIRNode);
  ListDestruct(&gen->code);

  VectorDestruct(&gen->int_constant_pool);
  VectorDestruct(&gen->fp_constant_pool);
  VectorDestruct(&gen->variable_pool);

  // Delete the basic blocks.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    BasicBlockDelete(block);
  }
}

// We've been asked for the label for the return.  If there
// isn't one, generate it now and store it.
IRNode* GeneratorGetReturnLabel(Generator* gen) {
  if (gen->return_label == NULL) {
    gen->return_label = GeneratorEmit(gen, NewIR(IR_OP(label)));
    GeneratorEmit(gen, NewIR(IR_OP(leave)));
    GeneratorEmit(gen, NewIR(IR_OP(ret)));
  }
  return gen->return_label;
}

IRNode* GeneratorFirstInstruction(Generator* gen) {
  return (IRNode*)gen->code.first;
}

IRNode* GeneratorLastInstruction(Generator* gen) {
  return (IRNode*)gen->code.last;
}

void GeneratorError(Generator* gen, ASTNode* node, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VGeneratorError(gen, node, format, ap);
  va_end(ap);
}

void VGeneratorError(Generator* gen, ASTNode* node, const char* format,
                     va_list ap) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportError(filename, lineno, format, ap);
}

void GeneratorWarning(Generator* gen, ASTNode* node, const char* warn,
                      const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  VGeneratorWarning(gen, node, warn, format, ap);
  va_end(ap);
}

void VGeneratorWarning(Generator* gen, ASTNode* node, const char* warn,
                       const char* format, va_list ap) {
  const char* filename;
  int lineno;
  int start, end;
  DecodeSourceLocation(node->location, &filename, &lineno, &start, &end);
  VReportWarning(filename, lineno, warn, format, ap);
}

IRNode* GeneratorEmit(Generator* gen, IRNode* inst) {
  if (IRInList(inst)) {
    return inst;
  }
  ListAppend(&gen->code, &inst->header);
  return inst;
}

IRNode* GeneratorEmitBefore(Generator* gen, IRNode* inst, IRNode* pos) {
  if (IRInList(inst)) {
    return inst;
  }
  ListInsertBefore(&gen->code, &inst->header, &pos->header);
  return inst;
}

IRNode* GeneratorEmitAfter(Generator* gen, IRNode* inst, IRNode* pos) {
  if (pos == NULL) {
    // No instruction to emit after means emit at end of list.
    return GeneratorEmit(gen, inst);
  }
  if (IRInList(inst)) {
    return inst;
  }
  ListInsertAfter(&gen->code, &inst->header, &pos->header);
  return inst;
}

// Constants are at the start of the code.
IRNode* GeneratorEmitConstant(Generator* gen, IRNode* inst) {
  if (IRInList(inst)) {
    return inst;
  }
  if (gen->last_constant == NULL) {
    gen->last_constant =
        GeneratorEmitBefore(gen, inst, (IRNode*)gen->code.first);
  } else {
    gen->last_constant = GeneratorEmitAfter(gen, inst, gen->last_constant);
  }
  return inst;
}

// Variables follow constants.
IRNode* GeneratorEmitVariable(Generator* gen, IRNode* inst) {
  if (IRInList(inst)) {
    return inst;
  }
  if (gen->last_variable == NULL) {
    gen->last_variable = GeneratorEmitBefore(gen, inst, (IRNode*)gen->code.first);
  } else {
    gen->last_variable = GeneratorEmitAfter(gen, inst, gen->last_variable);
  }
  return inst;
}

void GeneratorRemoveInstruction(Generator* gen, IRNode* inst) {
  ListDeleteElement(&gen->code, &inst->header);
  IRRemoveNode(inst);
  free(inst);
}

void GeneratorReplaceInstruction(Generator* gen, IRNode* old, IRNode* new) {
  // Replace references to the old node with those to the new one.
  for (size_t i = 0; i < old->outputs.length; i++) {
    IRNode* ref = old->outputs.value.p[i];
    for (size_t j = 0; j < ref->inputs.length; j++) {
      IRNode* input = ref->inputs.value.p[j];
      if (input == old) {
        ref->inputs.value.p[j] = new;
        VectorAppend(&new->outputs, ref);
      }
    }
  }
  VectorClear(&old->outputs);
}

IRNode* GeneratorGetIntConstant(Generator* gen, TypeRecord* type,
                                int64_t value) {
  PoolEntry* entry;
  Type type_spec = kTypeInt;
  if (type != NULL) {
    type_spec = type->type;
  }
  for (size_t i = 0; i < gen->int_constant_pool.length; i++) {
    entry = gen->int_constant_pool.value.p[i];
    if (entry->value.ivalue == value && entry->type == type_spec) {
      return entry->pooled;
    }
  }

  // No constant found, add a new one.
  entry = malloc(sizeof(PoolEntry));
  entry->value.ivalue = value;
  if (type != NULL) {
    entry->type = type->type;
  } else {
    entry->type = kTypeInt;
  }
  entry->pooled = GeneratorEmitConstant(gen, NewIntIRConstant(type, value));
  VectorAppend(&gen->int_constant_pool, entry);
  return entry->pooled;
}

IRNode* GeneratorGetFloatingPointConstant(Generator* gen, TypeRecord* type,
                                          double value) {
  PoolEntry* entry;
  Type type_spec = kTypeDouble;
  if (type != NULL) {
    type_spec = type->type;
  }
  for (size_t i = 0; i < gen->fp_constant_pool.length; i++) {
    entry = gen->fp_constant_pool.value.p[i];
    if (entry->value.fvalue == value && entry->type == type_spec) {
      return entry->pooled;
    }
  }

  // No constant found, add a new one.
  entry = malloc(sizeof(PoolEntry));
  entry->value.fvalue = value;
  if (type != NULL) {
    entry->type = type->type;
  } else {
    entry->type = kTypeDouble;
  }
  entry->pooled =
      GeneratorEmitConstant(gen, NewFloatingPointIRConstant(type, value));
  VectorAppend(&gen->int_constant_pool, entry);
  return entry->pooled;
}

IRNode* GeneratorGetVariable(Generator* gen, Symbol* sym) {
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = gen->variable_pool.value.p[i];
    if (entry->value.symbol == sym) {
      return entry->pooled;
    }
  }

  // No variable found, add a new one.
  PoolEntry* entry = malloc(sizeof(PoolEntry));
  entry->value.symbol = sym;
  entry->type = sym->type->type;
  entry->pooled = GeneratorEmitVariable(gen, NewIRVariable(sym));
  VectorAppend(&gen->variable_pool, entry);
  return entry->pooled;
}

//
// Basic block building.
//

static BasicBlock* GeneratorNewBasicBlock(Generator* gen) {
  BasicBlock* b = NewBasicBlock(gen->basic_blocks.length);
  VectorAppend(&gen->basic_blocks, b);
  return b;
}

static BasicBlock* FindBasicBlock(Generator* gen, BlockId id) {
  return VectorGet(&gen->basic_blocks, id);
}

static void PrintBasicBlocks(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    BasicBlockPrint(b, gen->entry_block, gen->exit_block);
  }
}

static void CreateBasicBlocks(Generator* gen,
                              Vector* branches) {
  gen->entry_block = GeneratorNewBasicBlock(gen);
  BasicBlock* current = gen->entry_block;

  // First instruction is in first block.
  current->code = GeneratorFirstInstruction(gen);
  
  // Find the boundary IRNodes (labels, branches and returns).  Each
  // one of these either ends a block or starts a new one.
  for (IRNode* inst = current->code; inst != NULL; inst = IRNext(inst)) {
    // Check if this node defines (writes to) a variable.  If so
    // keep track of this in the basic block.
    if (IRIsVarDef(inst)) {
      MapKeyValue kv;
      kv.key.p = inst->var.def;
      kv.value.p = NULL;
      MapInsert(&current->defined_vars, kv);
    }
    
    if (inst->opcode == IR_OP(label)) {
      current->end_code = IRPrev(inst);
      
      // A label marks the start of a block.
      BasicBlock* b = GeneratorNewBasicBlock(gen);
      inst->block = b;
      b->code = inst;
      current = b;
    } else if (IRIsBranch(inst) || IRIsReturn(inst)) {
      // A branch (and return) ends a block.
      current->end_code = inst;
      inst->block = current;
      VectorAppend(branches, inst);
      
      // Allocate a new block starting at the next instruction provided it's
      // not a label (because that will be created in next iteration).
      IRNode* next = IRNext(inst);
      if (next != NULL && next->opcode != IR_OP(label)) {
        BasicBlock* b = GeneratorNewBasicBlock(gen);
        b->code = next;
        current = b;
      }
    } else {
      inst->block = current;
      // Call instruction?  Mark block as containing call.
      if (inst->opcode == IR_OP(calla) || inst->opcode == IR_OP(memcpy) ||
          inst->opcode == IR_OP(memzero)) {
        current->num_calls++;
      }
    }
  }
  current->end_code = GeneratorLastInstruction(gen);
  
  // Allocate exit block.
  gen->exit_block = GeneratorNewBasicBlock(gen);
}

// Process all branches and link their targets to the appropriate
// block.
static void BuildBasicBlockGraph(Generator* gen,
                                  Vector* branches) {
  for (size_t i = 0; i < branches->length; i++) {
    IRNode* inst = branches->value.p[i];
    BasicBlock* block = inst->block;
    if (IRIsConditionalBranch(inst)) {
      // Conditional branch links to both its taken and fallthrough blocks.
      IRNode* fallthrough = IRNext(inst);
      IRNode* taken = inst->inputs.value.p[1];
      BasicBlockAddEdge(block, fallthrough->block);
      BasicBlockAddEdge(block, taken->block);
    } else if (inst->opcode == IR_OP(cbra)) {
      // Table jump is followed by a branch table.  These are bra instructions.
      // Find all of them and link to this block.
      IRNode* bra = IRNext(inst);
      while (bra->opcode == IR_OP(bra)) {
        BasicBlockAddEdge(block, bra->block);
        bra = IRNext(bra);
      }
    } else if (IRIsReturn(inst)) {
      // Return always links to the exit block.
      BasicBlockAddEdge(block, gen->exit_block);
    } else {
      // Unconditional branch only links to its target.
      IRNode* target = inst->inputs.value.p[0];
      BasicBlockAddEdge(block, target->block);
    }
  }
}

// All blocks with no output edges link to exit block.  Also blocks
// that do not end in a branch or return fall through to next block.
static void AddMissingLinks(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    if (b == gen->exit_block) {
      continue;
    }
    if (!BasicBlockEndsInBranchOrReturn(b)) {
      // No branch or return, fall through to next block.
      BasicBlockAddEdge(b, FindBasicBlock(gen, b->block_id + 1));
    }
    if (b->out_edges.length == 0) {
      BasicBlockAddEdge(b, gen->exit_block);
    }
  }
}

// Calculate the dominators for all basic blocks.
static void CalculateDominators(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    BasicBlockInitDominators(b, b == gen->entry_block,
                             gen->basic_blocks.length);
  }
  
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t i = 0; i < gen->basic_blocks.length; i++) {
      BasicBlock* b = gen->basic_blocks.value.p[i];
      changed |= BasicBlockCalculateDominators(b, &gen->basic_blocks);
    }
  }
}

static void CalculateImmediateDominator(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    BasicBlockCalculateImmediateDominator(b, &gen->basic_blocks);
  }
}

static void CalculateDominanceFrontier(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    BasicBlockCalculateDominanceFrontier(b, &gen->basic_blocks);
  }
}

// Build dominator tree.  If a block has an
// immediate dominator (idom) add the block to the idom's
// dominatees set.
static void BuildDominatorTree(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    if (b->idom != NULL) {
      VectorAppend(&b->idom->dominatees, (void*)b->block_id);
    }
  }
}

// We have all the instructions available.  Divide them into basic blocks where
// a basic block is a sequence of instructions with no branches (flow must hit
// every instruction in the block if the block is entered).
//
// Block boundaries are marked by labels and branches.
static void BuildBasicBlocks(Generator* gen) {
  // The branches vector holds a list of all the branches we encounter in the
  // code.  Each branch adds an edge from its block to the block starting with
  // the label to which it is branching.
  Vector branches;
  VectorInit(&branches);

  // Phase 1: create all basic blocks.
  CreateBasicBlocks(gen, &branches);
  
  // Phase 2: Link the blocks into a graph.
  BuildBasicBlockGraph(gen, &branches);

  // Phase 3: all blocks with no output edges link to exit block.  Also blocks
  // that do not end in a branch or return fall through to next block.
  AddMissingLinks(gen);
  
  // Phase 4: calculate dominators, dominance frontier and idom.
  // Phase 4a: dominators.
  CalculateDominators(gen);
 
  // Phase 4b: immediate dominator.
  CalculateImmediateDominator(gen);
 
  // Phase 4c: dominance frontier.
  CalculateDominanceFrontier(gen);
  
  // Phase 5: build dominator tree.
  BuildDominatorTree(gen);
 
  // Tidy up.
  // Delete the branches vector.
  VectorDestruct(&branches);
}

// Remove all unreachable basic blocks.  These will never be
// executed.  The blocks aren't removed from the set of blocks
// in the generator, we merely remove all the instructions from
// them.
static void RemoveUnreachableBlocks(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];

    if (BasicBlockIsUnreachable(gen, b)) {
      BasicBlockClear(gen, b);
    }
  }
}

// Look in all the basic blocks for one that contains a call instruction.
// We have already marked these during basic block generation.
int GeneratorNumCalls(Generator* gen) {
  int calls = 0;
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    calls += b->num_calls;
  }
  return calls;
}

void* GenerateFunction(Generator* gen) {
  // If the return value is a struct or union generate a holder.
  if (TypeIsStructOrUnion(gen->func->next)) {
    gen->struct_return_value = GeneratorEmit(gen, NewIR(IR_OP(structreturn)));
    IRSetType(gen->struct_return_value, gen->func->next);
  }

  if (gen->func->info.function.body.length == 0) {
    // Empty function, just return.
    GeneratorEmit(gen, NewIR(IR_OP(ret)));
  } else {
    GeneratorEmit(gen, NewIR(IR_OP(enter)));
    size_t num_statments = gen->func->info.function.body.length;
    for (size_t i = 0; i < num_statments; i++) {
      ASTNode* node = gen->func->info.function.body.value.p[i];
      GenerateStatement(gen, node);
    }

    if (gen->return_label == NULL) {
      GeneratorEmit(gen, NewIR(IR_OP(leave)));
      GeneratorEmit(gen, NewIR(IR_OP(ret)));
    } else {
      IRNode* return_label = GeneratorGetReturnLabel(gen);
      GeneratorEmit(gen, NewIR1(IR_OP(bra), return_label));
    }
  }

  GeneratorPrintIR(gen);

  BuildBasicBlocks(gen);

  printf("Before gvn\n");
  PrintBasicBlocks(gen);

  if (compiler->optimize) {
    // Remove any unreachable blocks before we go into SSA conversion.
    RemoveUnreachableBlocks(gen);

    //  printf("Before SSA\n");
    //  PrintBasicBlocks(gen);
    // Convert the IR graph to Static Single Assignment form.  This enables
    // optimizations.
    GeneratorConvertToSSA(gen);

    PrintBasicBlocks(gen);

    // Perform strength reduction optimization.  This simplifies instructions.
    StrengthReductionOptimization(gen);

    // Do Global Value Numbering.  This finds and uses common subexpressions.
    GlobalValueNumberingOptimization(gen);

    // printf("AFTER otimizations\n");
    // PrintBasicBlocks(gen);

    // Remove any SSA nodes we added, converting back from SSA form.
    GeneratorRemoveSSA(gen);

    // It's possible that the optimizations have made some blocks unreaachable
    // now, so see if we have any.
    RemoveUnreachableBlocks(gen);

    printf("After SSA\n");
  } else {
    RemoveUnreachableBlocks(gen);
  }

  PrintBasicBlocks(gen);

  // Generate lowered code for the target.
  void* code = compiler->target->codegen(gen);

  // We now have a target-specific code sequence for the function.
  return code;
}
