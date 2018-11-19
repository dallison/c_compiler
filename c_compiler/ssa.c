//
//  ssa.c
//  c_compiler
//
//  Created by David Allison on 12/14/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "ssa.h"
#include <assert.h>
#include <stdlib.h>
#include "codegen.h"
#include "map.h"
#include "vector.h"

// Static Single Assignment (SSA) form transformations.

// Given a symbol, find the version stack associated with it.  If there
// isn't one, add one for it.
static Vector* FindVariableStack(Map* stacks, Symbol* symbol) {
  void* result = MapFind(stacks, symbol);
  if (result != NULL) {
    return result;
  }

  // Not found, add a new one for this symbol.
  Vector* versions = NewVector();
  MapInsert(stacks, symbol, versions);
  return versions;
}

// Change the IRNode associated with the most recent (top of stack)
// variable to that given.
static void RenameTopVariable(Map* stacks, Symbol* symbol, IRNode* ssavar) {
  Vector* versions = MapFind(stacks, symbol);
  assert(versions != NULL);
  IRNode** top = (IRNode**)&versions->value[versions->length - 1];
  *top = ssavar;
}

// Find the most recent (top of stack) IRNode associated with
// the given variable.  Returns NULL if there isn't one.
static IRNode* GetTopVariable(Map* stacks, Symbol* symbol) {
  Vector* versions = MapFind(stacks, symbol);
  if (versions == NULL) {
    return NULL;
  }
  if (versions->length == 0) {
    return NULL;
  }
  return versions->value[versions->length - 1];
}

// Follow the instructions to find a reference to the given
// symbol.  This might pass through address calculation instructions and
// terminate with a load or a direct reference.  Returns the instruction
// that references the variable.
static IRNode* FindVariableReference(IRNode* origin, Symbol* symbol) {
  IRNode* inst = origin;
  IRNode* prev = origin;
  while (inst != NULL) {
    if (IRIsVariable(inst)) {
      IRVariable* var = (IRVariable*)inst;
      if (var->symbol == symbol) {
        // Found variable.
        return prev;
      }
    }
    prev = inst;
    if (IRIsLoad(inst) || IRIsStore(inst) || inst->opcode == IR_OP(adda)) {
      inst = inst->inputs.value[0];
      continue;
    }
    break;
  }
  return NULL;
}

// Traverse the dominator tree depth first.  At each node, find all the variable
// definitions and push them onto a stack associated with that variable.  The
// stack contains references to the IRNode representing the variable.
static void RenameVariables(Generator* gen, BasicBlock* block,
                            Map* var_stacks) {
  // Push all variable definitions onto the var_stack.
  for (size_t i = 0; i < block->defined_vars.length; i++) {
    Symbol* sym = block->defined_vars.values[i].key;
    Vector* stack = FindVariableStack(var_stacks, sym);
    IRNode* var;
    // Either duplicate the top of the stack or add the original variable
    // to the stack.
    if (stack->length == 0) {
      var = GeneratorGetVariable(gen, sym);
    } else {
      var = stack->value[stack->length - 1];
    }
    VectorPush(stack, var);
  }

  // Now traverse all nodes in the block and:
  // 1. For each var definition, add a new 'ssavar' IR node and set the top
  //    stack entry for the symbol to that node.
  // 2. For each var reference, replace the IR node's variable reference
  //    with the most recent ssavar IR node.
  IRNode* inst = block->code;
  while (inst != NULL && IRPrev(inst) != block->end_code) {
    if (IRIsVarDef(inst)) {
      // Node defines a variable.
      if (inst->opcode == IR_OP(phi)) {
        // A PHI node doesn't have any references to the variables yet.
        RenameTopVariable(var_stacks, inst->var.def, inst);
      } else {
        assert(IRIsStore(inst));
        IRNode* ref = FindVariableReference(inst, inst->var.def);
        assert(ref != NULL);
        IRNode* ssavar = NewIRSSAVar(inst->var.def);
        // Emit ssavar at beginning of entry basic block.
        BasicBlockInsertVar(gen, gen->entry_block, ssavar);
        RenameTopVariable(var_stacks, inst->var.def, ssavar);
        IRReplaceInput(ref, 0, ssavar);
      }
    } else if (IRIsVarRef(inst)) {
      // Node references a variable.  This will be one of the 'load' IR
      // instructions.  The first input is replaced by the IRNode at the top of
      // the var stack for the symbol.  Only do this if the operand being loaded
      // is a variable node.  The load instructions can also refer to
      // expressions that refer to a variable indirectly.
      assert(IRIsLoad(inst));
      IRNode* ref = FindVariableReference(inst, inst->var.def);
      assert(ref != NULL);
      IRNode* var = GetTopVariable(var_stacks, inst->var.use);
      if (var != NULL) {
        IRReplaceInput(ref, 0, var);
      }
    }
    inst = IRNext(inst);
  }

  // Process all blocks dominated by this one.
  for (size_t i = 0; i < block->dominatees.length; i++) {
    BasicBlock* b =
        VectorGet(&gen->basic_blocks, (BlockId)block->dominatees.value[i]);
    RenameVariables(gen, b, var_stacks);
  }

  // Pop all stacks for all defined vars.
  for (size_t i = 0; i < block->defined_vars.length; i++) {
    Symbol* sym = block->defined_vars.values[i].key;
    Vector* stack = FindVariableStack(var_stacks, sym);

    // Store the most recent IRNode assigned to the defined variable to the
    // value held in the defined_vars map for this symbol.  This allows us
    // to know what SSA variable to use for the PHI node inputs.
    block->defined_vars.values[i].value = stack->value[stack->length - 1];
    VectorPop(stack);
  }
}

static void InsertPhiNodes(Generator* gen) {
  // Inserting PHI nodes in basic blocks adds a new variable definition to the
  // block.  We need to keep trying until we get all the PHI nodes inserted.
  bool changed;
  Vector df;  // Expanded dominance frontier (easier than a BitSet to traverse).
  VectorInit(&df);

  do {
    changed = false;
    for (size_t i = 0; i < gen->basic_blocks.length; i++) {
      BasicBlock* block = gen->basic_blocks.value[i];
      for (size_t j = 0; j < block->defined_vars.length; j++) {
        Symbol* sym = block->defined_vars.values[j].key;
        VectorClear(&df);
        BitSetExpand(&block->dominance_frontier, &df);
        for (size_t k = 0; k < df.length; k++) {
          BasicBlock* df_node =
              VectorGet(&gen->basic_blocks, (BlockId)df.value[k]);

          // Insert PHI node into block.  This will not add the a PHI node to
          // the same variable more than once.  It returns true if it adds a new
          // PHI node.
          bool new_phi = BasicBlockInsertPhi(gen, df_node, sym);
          if (new_phi) {
            // We've added a new PHI node.  This defines a new variable in the
            // destination block.  Add it.
            MapInsert(&df_node->defined_vars, sym, NULL);
          }
          changed |= new_phi;
        }
      }
    }
  } while (changed);
  VectorDestruct(&df);
}

static IRNode* FindSSAVar(Generator* gen, BasicBlock* block, Symbol* sym,
                          BitSet* visited) {
  void* latest_var = MapFind(&block->defined_vars, sym);
  if (latest_var != NULL) {
    return latest_var;
  }
  for (size_t i = 0; i < block->in_edges.length; i++) {
    BlockId id = (BlockId)block->in_edges.value[i];
    if (BitSetContains(visited, id)) {
      continue;
    }
    BitSetInsert(visited, id);
    BasicBlock* input = VectorGet(&gen->basic_blocks, id);
    latest_var = FindSSAVar(gen, input, sym, visited);
    if (latest_var != NULL) {
      return latest_var;
    }
  }
  return NULL;
}

// Add all inputs to the phi nodes in the block.  We do this by
// traversing all the block and looking up the defined_vars map
// to obtain the latest SSA variable for the given symbol.
static void AddPhiInputs(Generator* gen, BasicBlock* block) {
  IRNode* inst = block->code;
  BitSet visited;
  BitSetInit(&visited);
  while (inst != NULL && inst->opcode == IR_OP(phi)) {
    IRVariable* phi = (IRVariable*)inst;
    Symbol* sym = phi->symbol;
    for (size_t i = 0; i < block->in_edges.length; i++) {
      BasicBlock* input =
          VectorGet(&gen->basic_blocks, (BlockId)block->in_edges.value[i]);
      void* latest_var = FindSSAVar(gen, input, sym, &visited);
      if (latest_var != NULL) {
        IRAddInput((IRNode*)phi, latest_var);
      }
    }
    inst = IRNext(inst);
  }
  BitSetDestruct(&visited);
}

// Map entry comparison function for comparing a map whose
// keys are Symbol pointers.
static int SymbolCompare(const void* a, const void* b) {
  MapKeyValue* s1 = (MapKeyValue*)a;
  MapKeyValue* s2 = (MapKeyValue*)b;

  ptrdiff_t diff = s1->key - s2->key;
  return (int)diff;
}

// Rename all variables in basic blocks to generate a Static Single Assignment
// to them.
void GeneratorRenameVariables(Generator* gen) {
  // We need a mapping for each variable to a stack of values it has been
  // renamed to.
  Map var_stacks;
  MapInit(&var_stacks, SymbolCompare);

  // Perform the rename.
  RenameVariables(gen, gen->entry_block, &var_stacks);

  // Clean up.
  for (size_t i = 0; i < var_stacks.length; i++) {
    VectorDelete(var_stacks.values[i].value);
  }
  MapDestruct(&var_stacks);
}

// Convert the generated IR to Static Single Assignment (SSA) form.
void GeneratorConvertToSSA(Generator* gen) {
  // Insert PHI nodes for all varaibles.
  InsertPhiNodes(gen);

  // Rename all variables, creating a single assignment to each.
  GeneratorRenameVariables(gen);

  // Add inputs to all phi nodes in the blocks.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    AddPhiInputs(gen, gen->basic_blocks.value[i]);
  }
}

//
// Remove all SSA transformations.
//

// Remove phi nodes from the code.  The phi nodes are always
// at the beginning of the block.  To remove them we reassign
// all the nodes referring to them (their outputs) to their
// first input.  This input will be an SSA variable (ssavar IR node) and
// will be removed in a subsequent pass.
static void RemovePhiNodes(Generator* gen, BasicBlock* block) {
  IRNode* inst = block->code;
  while (inst != NULL && inst->opcode == IR_OP(phi)) {
    IRNode* next = IRNext(inst);

    // Replace all references to the PHI node with a reference the
    // original var node
    IRNode* input = GeneratorGetVariable(gen, inst->var.def);
    assert(input != NULL);

    size_t i = 0;
    while (i < inst->outputs.length) {
      IRNode* node = inst->outputs.value[i];
      bool modified = false;
      for (size_t j = 0; j < node->inputs.length; j++) {
        if (node->inputs.value[j] == inst) {
          IRReplaceInput(node, j, input);
          modified = true;
          break;
        }
      }
      if (!modified) {
        i++;
      }
    }

    BasicBlockRemoveInstruction(gen, block, inst);
    inst = next;
  }
}

// Remove all SSA varaibles.  Each ssavar node is removed and all references
// to it are replaced by a reference to the actual (pooled) variable for
// which it was initially created.
static void RemoveSSAVariables(Generator* gen, BasicBlock* block) {
  IRNode* inst = block->code;
  while (inst != NULL && IRPrev(inst) != block->end_code) {
    IRNode* next = IRNext(inst);
    if (inst->opcode == IR_OP(ssavar)) {
      // This is an SSA variable, remove it.

      IRVariable* var = (IRVariable*)inst;

      // Find the pooled real variable and replace all references to
      // the instruction being removed by a reference to the variable node.
      IRNode* input = GeneratorGetVariable(gen, var->symbol);
      size_t i = 0;
      while (i < inst->outputs.length) {
        IRNode* node = inst->outputs.value[i];
        bool modified = false;
        for (size_t j = 0; j < node->inputs.length; j++) {
          if (node->inputs.value[j] == inst) {
            IRReplaceInput(node, j, input);
            modified = true;
            break;
          }
        }
        if (!modified) {
          i++;
        }
      }

      BasicBlockRemoveInstruction(gen, block, inst);
    }
    inst = next;
  }
}

void GeneratorRemoveSSA(Generator* gen) {
  // Pass 1: remove phi nodes.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    RemovePhiNodes(gen, gen->basic_blocks.value[i]);
  }

  // Pass 2: remove SSA variables.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    RemoveSSAVariables(gen, gen->basic_blocks.value[i]);
  }
}
