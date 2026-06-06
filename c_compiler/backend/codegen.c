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
#include <string.h>
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
#include "constprop.h"
#include "codemotion.h"

static void Trap() {}
static void TrapInstruction(IRNode* inst) {
  if (inst->id == 19) {
    Trap();
  }
}

static void TrapFunctionBeforeCodegen(Generator* gen) {
  if (StringEqual(&gen->func->info.function.symbol->name, "InitializeInstructions")) {
    Trap();
  }
}

static void TrapFunctionAfterCodegen(Generator* gen) {
  if (StringEqual(&gen->func->info.function.symbol->name, "foobar")) {
    Trap();
  }
}

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

static void PrintIR(ListElement* hdr, void* fp) {
  IRNode* node = (IRNode*)hdr;
  IRPrint(node, fp);
}

void GeneratorPrintIR(Generator* gen, FILE* fp) {
  for (int i = 0; i < 80; i++) {
    fprintf(fp, "-");
  }
  fprintf(fp, "\n");
  fprintf(fp, "**** IR for function %s\n\n", gen->func->info.function.symbol->name.value);
  ListTraverse(&gen->code, PrintIR, fp);
}

static void DestructIRNode(ListElement* hdr, void* data) {
  IRNode* node = (IRNode*)hdr;
  IRDestruct(node);
}

void GeneratorDestruct(Generator* gen) {
  TypeRecordDelete(gen->func);

  // Delete all IR nodes.
  ListTraverse(&gen->code, DestructIRNode, NULL);
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

// Check if the node is using (reading) a variable.  If so,
// mark the write instruction with the VarUse flag.  This is necessary
// so that the SSA conversions know that this is a read from a variable.
void CheckForVarUse(IRNode* read, ASTNode* node) {
  switch (node->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* id = (IdentifierASTNode*)node;
      IRSetVarUse(read, id->symbol);
      break;
    }
    case AST_OP(dot):
    case AST_OP(arrow): {
      BinaryASTNode* b = (BinaryASTNode*)node;
      CheckForVarUse(read, b->left);
      break;
    }
    case AST_OP(subscript): {
      BinaryASTNode* b = (BinaryASTNode*)node;
      CheckForVarUse(read, b->left);
      break;
    }

    default:
      break;
  }
}

// Check if the node is defining (writing to) a variable.  If so,
// mark the write instruction with the VarDef flag.  This is necessary
// so that the SSA conversions know that this is a write to a variable.
void CheckForVarDef(IRNode* write, ASTNode* node) {
  switch (node->op) {
    case AST_OP(identifier): {
      IdentifierASTNode* id = (IdentifierASTNode*)node;
      IRSetVarDef(write, id->symbol);
      break;
    }
    case AST_OP(init):
    case AST_OP(subscript):{
      BinaryASTNode* b = (BinaryASTNode*)node;
      CheckForVarDef(write, b->left);
      break;
    }
    case AST_OP(dot):
    case AST_OP(arrow): {
      BinaryASTNode* b = (BinaryASTNode*)node;
      CheckForVarDef(write, b->left);
      break;
    }
    case AST_OP(address): {
      UnaryASTNode* n = (UnaryASTNode*)node;
      CheckForVarDef(write, n->sub);
      break;
    }
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_va_copy): {
      VectorASTNode* v = (VectorASTNode*)node;
      CheckForVarDef(write, v->children->value.p[0]);
      break;
    }
    case AST_OP(return): {
      CombinedStatementASTNode* r = (CombinedStatementASTNode*)node;
      CheckForVarDef(write, r->cond);
    }
      
    default:
      break;
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
  TrapInstruction(inst);
  if (IRInList(inst)) {
    return inst;
  }
  ListAppend(&gen->code, &inst->header);
  return inst;
}

IRNode* GeneratorEmitBefore(Generator* gen, IRNode* inst, IRNode* pos) {
  TrapInstruction(inst);
  if (IRInList(inst)) {
    return inst;
  }
  ListInsertBefore(&gen->code, &inst->header, &pos->header);
  return inst;
}

IRNode* GeneratorEmitAfter(Generator* gen, IRNode* inst, IRNode* pos) {
  TrapInstruction(inst);
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

void GeneratorMoveInstructionBefore(Generator* gen, IRNode* inst, IRNode* pos) {
  ListDeleteElement(&gen->code, &inst->header);
  ListInsertBefore(&gen->code, &inst->header, &pos->header);
}

void GeneratorMoveInstructionAfter(Generator* gen, IRNode* inst, IRNode* pos) {
  ListDeleteElement(&gen->code, &inst->header);
  ListInsertAfter(&gen->code, &inst->header, &pos->header);
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
  if (value == 42) {
    printf("");
  }
  // No constant found, add a new one.
  entry = malloc(sizeof(PoolEntry));
  entry->value.ivalue = value;
  if (type != NULL) {
    entry->type = type->type;
  } else {
    entry->type = kTypeInt;
  }
  if (type == NULL) {
    type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
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

static void PrintBasicBlocks(Generator* gen, FILE* fp) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    BasicBlockPrint(gen, b, gen->entry_block, gen->exit_block, fp);
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
    if (IRIsVarRef(inst)) {
      SetInsert(&current->referenced_vars, inst->var.use);
    }
    
    // Call instruction?  Mark block as containing call.
    if (inst->opcode == IR_OP(calla) || inst->opcode == IR_OP(memcpy) ||
        inst->opcode == IR_OP(memzero)) {
      current->num_calls++;
    }

    if (inst->opcode == IR_OP(label)) {
      current->end_code = IRPrev(inst);
      
      // A label marks the start of a block.
      BasicBlock* b = GeneratorNewBasicBlock(gen);
      inst->block = b;
      b->code = inst;
      current = b;
    } else if (IRIsBranch(inst) || IRIsReturn(inst) || IRIsCall(inst)) {
      // A branch (and return) ends a block.
      current->return_block = IRIsReturn(inst);
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
    } else if (IRIsCall(inst)) {
      // A call just links to next block.
      IRNode* fallthrough = IRNext(inst);
      BasicBlockAddEdge(block, fallthrough->block);
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
    if (!BasicBlockEndsInBranchReturnOrCall(b)) {
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
      changed |= BasicBlockCalculateDominators(gen, b, &gen->basic_blocks);
    }
  }
  
  // Now check for isolated islands where the blocks form a loop
  // that cannot be accessed from outside the loop.  This is denoted
  // by the dominators of the block being all the blocks.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    if (BitSetCount(&b->dominators) == gen->basic_blocks.length) {
      BitSetClear(&b->dominators);
      b->is_unreachable = true;
      b->reachability_known = true;
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
    BasicBlockCalculateDominanceFrontier(gen, b, &gen->basic_blocks);
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

static void IncrementLoopNesting(BasicBlock* block, void* data) {
  block->loop_nesting++;
}

// Detect loops in the control flow graph by looking for back edges and
// incrementing the loop_nesting counter for all blocks inside the loop
// body.
//
// A back edge from A to B is an out edge from A to B for which A is
// a dominator of B.
//
// For each back edge detected we traverse the dominator tree for
// the loop header block (A in this example) and increment its
// loop_nesting counter.
static void DetectLoops(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* b = gen->basic_blocks.value.p[i];
    for (size_t j = 0; j < b->out_edges.length; j++) {
      BlockId out_id = b->out_edges.value.w[j];
      if (BitSetContains(&b->dominators, out_id)) {
        // Out edge is a dominator, therefore this is a back-edge.
        BasicBlock* loop_head = VectorGet(&gen->basic_blocks, out_id);
        BasicBlockAddBackEdge(b, loop_head);
        BasicBlockTraverseDominatorTree(gen, loop_head,
                                        IncrementLoopNesting, kTraversePreOrder, NULL);
      }
    }
  }
}

static void CoalesceBlocks(Generator* gen, BasicBlock* dest, BasicBlock* src) {
  // Move all instructions from the source block to the destination, placing
  // them before the last unconditional branch.
  IRNode* next = NULL;
  IRNode* last = src->code;
  for (IRNode* inst = BasicBlockBegin(src); !BasicBlockIsEmpty(src) && inst != BasicBlockEnd(src); inst = next) {
    next = IRNext(inst);
    last = inst;
    BasicBlockMoveInstructionBefore(gen, inst, dest->end_code);
  }
  
  // Copy all defined variables from src to dest.
  MapCopy(&dest->defined_vars, &src->defined_vars);
  
  // Copy all referenced vars too.
  SetCopy(&dest->referenced_vars, &src->referenced_vars);
  
  // Clear defined and referenced vars in src.
  MapClear(&src->defined_vars);
  SetClear(&src->referenced_vars);
  
  dest->num_calls += src->num_calls;
  
  if (src->return_block) {
    dest->return_block = true;
  }
  // The source block is now empty.
  if (IRIsConditionalBranch(last)) {
    // If it ended in a conditional
    // branch we need to add an unconditional branch to its fallthrough
    // output.  The fallthrough block is the one that is not used in
    // the conditional branch.
    assert(src->out_edges.length == 2);
    IRNode* taken = last->inputs.value.p[1];
    BasicBlock* fallthrough = VectorGet(&gen->basic_blocks, src->out_edges.value.w[0]);
    if (taken->block == fallthrough) {
      fallthrough = VectorGet(&gen->basic_blocks, src->out_edges.value.w[1]);
    }
    // If the fallthrough block doesn't begin with a label, add a label
    // for it.
    if (fallthrough->code->opcode != IR_OP(label)) {
      IRNode* label = GeneratorEmitBefore(gen, NewIR(IR_OP(label)), fallthrough->code);
      fallthrough->code = label;
    }
    IRNode* bra = GeneratorEmitBefore(gen, NewIR1(IR_OP(bra), fallthrough->code), dest->end_code);
    bra->block = dest;
  } else if (!IRIsUnconditionalBranch(last) && !IRIsReturn(last)) {
    // Block doesn't end in a branch or return. Add a single unconditional
    // branch to its single output edge.
    assert(src->out_edges.length == 1);
    BasicBlock* output = VectorGet(&gen->basic_blocks, src->out_edges.value.w[0]);
    IRNode* bra = GeneratorEmitBefore(gen, NewIR1(IR_OP(bra), output->code), dest->end_code);
    bra->block = dest;
  }
  
  // Remove branch at end of dest block.
  BasicBlockRemoveInstruction(gen, dest, dest->end_code);
  
  // Remove link between dest and src.
  BasicBlockRemoveEdge(dest, src);
  
  // Move all out edges from src to dest.
  Vector out_edges = {0};
  VectorCopy(&out_edges, &src->out_edges);
  for (size_t i = 0; i < out_edges.length; i++) {
    BasicBlock* output = VectorGet(&gen->basic_blocks, out_edges.value.w[i]);
    BasicBlockRemoveEdge(src, output);
    BasicBlockAddEdge(dest, output);
  }
  VectorDestruct(&out_edges);
}

// Look for blocks with one input edgeand that input block ends
// in an unconditional branch.  We can merge these block together,
// remove the branch and possibly add a branch to the fall-through
// block if the block being moved ends in a conditional branch.
//
// This is just a linear traversal of the basic blocks.
static void StraightenGraph(Generator* gen) {
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    if (block->in_edges.length == 1) {
      // Found a possible movable block.
      BasicBlock* input = VectorGet(&gen->basic_blocks, block->in_edges.value.w[0]);
      if (input->out_edges.length == 1) {
        assert(block == VectorGet(&gen->basic_blocks, input->out_edges.value.w[0]));
        IRNode* terminator = input->end_code;
        // Unconditional branch, not in a jump table.
        // Also check that the block is not just a label.
        bool is_candidate = terminator != NULL &&
              IRIsUnconditionalBranch(terminator) &&
            (terminator->flags & kIRJumpTableBranch) == 0;
        if (is_candidate) {
          // Don't merge a block that ends in a label or a result.
          is_candidate = block->end_code->opcode != IR_OP(label) &&
            !IRIsResult(block->end_code);
        }
        
        if (is_candidate) {
          CoalesceBlocks(gen, input, block);
        }
      }
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

  // Create all basic blocks.
  CreateBasicBlocks(gen, &branches);
  
  // Link the blocks into a graph.
  BuildBasicBlockGraph(gen, &branches);

  // All blocks with no output edges link to exit block.  Also blocks
  // that do not end in a branch or return fall through to next block.
  AddMissingLinks(gen);
    
  // PrintBasicBlocks(gen, stdout);
  
  // Straighten graph, coalescing blocks that just link to each other.
  StraightenGraph(gen);

  // Calculate dominators, dominance frontier and idom.
  // Cominators.
  CalculateDominators(gen);
 
  // PrintBasicBlocks(gen, stdout);
  
  // PImmediate dominator.
  CalculateImmediateDominator(gen);
 
  // Dominance frontier.
  CalculateDominanceFrontier(gen);
  
  // Build dominator tree.
  BuildDominatorTree(gen);
 
  // Detect loops.
  DetectLoops(gen);

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


// Checks for missing return.
typedef struct {
  BitSet visited;     // Map of visited block ids.
  bool result_known;  // We have reached a return or end of path.
  bool found_result;  // We have found an assignemnt to the result.
} ReturnVisitor;

static bool VisitBlockForResultInstruction(IRNode* inst, ReturnVisitor* v) {
  if (inst == NULL) {
    return false;
  }
  // Scalar result return;
  if (IRIsResult(inst)) {
    v->found_result = true;
    v->result_known = true;
    return true;
  }
  
  // Struct result return.
  if (inst->opcode == IR_OP(memcpy)) {
    IRNode* dest = inst->inputs.value.p[0];
    if (dest->opcode == IR_OP(structreturn)) {
      v->found_result = true;
      v->result_known = true;
      return true;
    }
  }
  
  // RVO (Return Value Optimization) call.
  // NRVO (Named Return Value Optimization) branch.
  if ((inst->flags & (kIRRvoCall | kIRNrvoMarker)) != 0) {
    v->found_result = true;
    v->result_known = true;
    return true;
  }
  
  return false;
}

static void VisitBlockForResult(Generator* gen, BasicBlock* block, ReturnVisitor* v) {
  if (block == NULL) {
    return;
  }
  if (v->result_known) {
    return;
  }
  if (BitSetContains(&v->visited, block->block_id)) {
    return;
  }
  BitSetInsert(&v->visited, block->block_id);
  
  IRNode* last_inst = block->end_code;
  if (last_inst == NULL) {
    return;
  }
  if (IRIsReturn(last_inst)) {
    v->result_known = true;
    return;
  }
  // Look in the block for a result assignment.
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (VisitBlockForResultInstruction(inst, v)) {
      return;
    }
  }
   
  if (block == gen->exit_block) {
    v->result_known = true;
    return;
  }
  ReturnVisitor saved = {{0}, v->result_known, v->found_result};
  BitSetCopy(&saved.visited, &v->visited);
  for (size_t i = 0; i < block->out_edges.length; i++) {
    BitSetCopy(&v->visited, &saved.visited);
    v->result_known = saved.result_known;
    v->found_result = saved.found_result;
    BlockId child_id = block->out_edges.value.w[i];
    BasicBlock* child = VectorGet(&gen->basic_blocks, child_id);
    VisitBlockForResult(gen, child, v);
    if (v->result_known && !v->found_result) {
      break;
    }
  }
  BitSetDestruct(&saved.visited);
}

// Check that we can't reach the end block.
static void CheckReturn(Generator* gen) {
  // Don't check void functions.
  if (TypeIsVoidFunction(gen->func->info.function.symbol->type)) {
    return;
  }
  
  // main is special.
  if (StringEqual(&gen->func->info.function.symbol->name, "main")) {
    return;
  }
  ReturnVisitor v = {{0}, false, false};
  VisitBlockForResult(gen, gen->basic_blocks.value.p[0], &v);
  if (v.result_known && !v.found_result) {
    // Falling off the end of a non-void function is undefined behaviour, but
    // like GCC/Clang we only warn rather than reject the program.
    SyntaxWarning(gen->syntax, "return-type",
                  "Control reaches the end of non-void function '%s'",
                  gen->func->info.function.symbol->name.value);
  }
}


// Look for uninitialized variables.  This is really easy in SSA form
// as you just have to look for a localvar node with more than zero references.
// As all writes to variables result in an ssavar node being created, any
// references to the original variable are uninitialized by definition.
//
// All variables are in the entry block.
//
// NOTE: this can't check arrays.  Consider:
//  char buf[16];
//  strcpy(buf, "hello");     // REF but is actually write.
static void DetectUninitializedVars(Generator* gen) {
  BasicBlock* block = gen->entry_block;
  for (IRNode* inst = BasicBlockBegin(block);
       !BasicBlockIsEmpty(block) && inst != BasicBlockEnd(block);
       inst = IRNext(inst)) {
    if (inst->opcode == IR_OP(localvar)) {
      if (inst->outputs.length > 0) {
        IRVariable* var = (IRVariable*)inst;
        if (TypeIsArray(var->symbol->type) ||
            TypeIsStructOrUnion(var->symbol->type)) {
          // Only scalars.  No way to check for arrays, etc.
          continue;
        }
        for (size_t i = 0; i < inst->outputs.length; i++) {
          IRNode* ref = inst->outputs.value.p[i];
          if (IRIsVarDef(ref)) {
            continue;
          }
          const char* filename;
          int lineno;
          int start, end;
          DecodeSourceLocation(ref->location, &filename, &lineno, &start, &end);
          ReportWarning(filename, lineno, "uninit-var",
                    "Variable '%s' is used uninitialized here in function '%s'",
                    var->symbol->name.value,
                    gen->func->info.function.symbol->name.value);
        }
      }
    }
  }
}

// A VLA as an argument may be used in the funtion and the size of
// all dimensions will need to be known.  We generate a code sequence
// to calculate the size of each dimension and emit the code.  It needs
// to be done in the entry block to ensure that it's available for all
// basic blocks (all blocks are dominated by the entry block).  If the
// size expressions aren't used they will be eliminated later.
// The IRNode for the size is stored in the type record's codegen_info.
static void GenerateVLASizeExpressions(Generator* gen, Vector* prototype) {
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* formal = prototype->value.p[i];
    if (TypeIsVLA(formal->type)) {
      GenerateVLASize(gen, formal->type);
    }
  }
}

void* GenerateFunction(Generator* gen) {
  CompoundStatementASTNode* body = (CompoundStatementASTNode*)gen->func->info.function.body;
  if (body->statements->length == 0) {
    // Empty function, just return.
    GeneratorEmit(gen, NewIR(IR_OP(ret)));
  } else {
    GeneratorEmit(gen, NewIR(IR_OP(enter)));

    // If the return value is a struct or union generate a holder.
    if (TypeIsStructOrUnion(gen->func->next)) {
      gen->struct_return_value = GeneratorEmit(gen, NewIR(IR_OP(structreturn)));
      IRSetType(gen->struct_return_value, gen->func->next);
    }
 
    TrapFunctionBeforeCodegen(gen);

    // Generate size expression for all VLAs in the prototype.
    GenerateVLASizeExpressions(gen, &gen->func->info.function.prototype);

    GenerateStatement(gen, &body->base);
    
    if (gen->return_label == NULL) {
      if (strcmp(gen->func->info.function.symbol->name.value, "main") == 0) {
        // main: add a resulti 0.
        IRNode* zero = GeneratorEmitConstant(gen,
                                             NewIntIRConstant(
                                                              NewTypeRecordWithSize(kTypeInt, kQualPlain), 0));
        GeneratorEmit(gen, NewIR1(IR_OP(resulti), zero));
      }
      GeneratorEmit(gen, NewIR(IR_OP(leave)));
      GeneratorEmit(gen, NewIR(IR_OP(ret)));
    } else {
      IRNode* return_label = GeneratorGetReturnLabel(gen);
      GeneratorEmit(gen, NewIR1(IR_OP(bra), return_label));
    }
  }

  if (compiler->print_back_end || compiler->ir_output_file != stdout) {
    GeneratorPrintIR(gen, compiler->ir_output_file);
  }
  
  BuildBasicBlocks(gen);

  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    fprintf(compiler->ir_output_file, "Before SSA conversion\n");
    PrintBasicBlocks(gen, compiler->ir_output_file);
  }
  
  CheckReturn(gen);

  // Remove any unreachable blocks before we go into SSA conversion.
  RemoveUnreachableBlocks(gen);

  //  printf("Before SSA\n");
  //  PrintBasicBlocks(gen);
  // Convert the IR graph to Static Single Assignment form.  This enables
  // optimizations.
  GeneratorConvertToSSA(gen);

  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    fprintf(compiler->ir_output_file, "After SSA conversion\n");
    PrintBasicBlocks(gen, compiler->ir_output_file);
  }

  // Find all uninitialized variables.
  DetectUninitializedVars(gen);

  if (OptLevel2()) {
    // Perform strength reduction optimization.  This simplifies instructions.
    StrengthReductionOptimization(gen);

    if (compiler->ir_optimizations.gvn) {
     // Do Global Value Numbering.  This finds and uses common subexpressions.
      GlobalValueNumberingOptimization(gen);
    }
    
    if (compiler->ir_optimizations.const_prop) {
    // Propagate constants.
      ConstantPropagationOptimization(gen);
    }
    
    if (compiler->ir_optimizations.code_motion) {
      // Perform code motion for loops.
      CodeMotionOptimization(gen);
    }
  }
    
  // printf("AFTER otimizations\n");
  // PrintBasicBlocks(gen);

  if (!compiler->keep_ssa) {
    // Remove any SSA nodes we added, converting back from SSA form.
    GeneratorRemoveSSA(gen);
  }
  
  // It's possible that the optimizations have made some blocks unreaachable
  // now, so see if we have any.
  RemoveUnreachableBlocks(gen);

  if (OptLevel2() && compiler->ir_optimizations.tail_call) {
    // Find all tail calls when not in SSA form.
    TailCallOptimization(gen);
  }

  if (compiler->print_back_end) {
     fprintf(compiler->ir_output_file, "After SSA has been removed\n");
  }

  if (compiler->print_back_end|| compiler->ir_output_file != stdout) {
    PrintBasicBlocks(gen, compiler->ir_output_file);
  }
  
  TrapFunctionAfterCodegen(gen);
  
  // Generate lowered code for the target.
  void* code = compiler->target->codegen(gen);

  // We now have a target-specific code sequence for the function.
  return code;
}
