//
//  gvn.c
//  c_compiler
//
//  Created by David Allison on 12/20/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "gvn.h"
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

Value* NewValue(int64_t key, int32_t value_number, IRNode* inst) {
  Value* v = malloc(sizeof(Value));
  v->key = key;
  v->value_number = value_number;
  v->instruction = inst;
  return v;
}

void ValueDelete(Value* value) { free(value); }

Value* ValueCopy(Value* v) {
  return NewValue(v->key, v->value_number, v->instruction);
}

// The key for the hash table is the 64 bit integer representing the key
// for an instruction.  The hash value is just this key.
static size_t HashInstruction(void* value, HashTable* table, HashMode mode) {
  int64_t hash;
  switch (mode) {
    case kHashInsert:
      // Insert mode, value is a Value pointer.
      hash = ((Value*)value)->key;
      break;
    case kHashSearch:
      // Search mode, value is key to find.
      hash = (int64_t)value;
      break;
  }
  return hash;
}

// Hash table insertion for a Value.
static bool InsertValue(void* entry, void* value, void** parent) {
  Vector* buckets = entry;
  if (buckets == NULL) {
    // No bucket list, create a new one.
    buckets = NewVector();
    *parent = buckets;
  }

  // Append to bucket list.
  VectorAppend(buckets, value);
  return true;
}

// Hash table search function.  Passed pointer to vector of values.
static void* FindValue(void* entry, void* value) {
  Vector* buckets = entry;
  if (buckets == NULL) {
    return NULL;
  }
  int64_t key = (int64_t)value;
  for (size_t i = 0; i < buckets->length; i++) {
    Value* v = buckets->value.p[i];
    if (key == v->key) {
      return v;
    }
  }
  return NULL;
}

void ValueSetInit(ValueSet* opt) {
  opt->next_value_number = 1;
  HashTableInit(&opt->values, "values", 111, HashInstruction, InsertValue,
                FindValue);
}

ValueSet* NewValueSet() {
  ValueSet* opt = malloc(sizeof(ValueSet));
  ValueSetInit(opt);
  return opt;
}

static void DeleteValueList(void* list, void* data) {
  Vector* vec = (Vector*)list;
  for (size_t i = 0; i < vec->length; i++) {
    Value* value = vec->value.p[i];
    ValueDelete(value);
  }
  VectorDelete((Vector*)list);
}

void ValueSetDelete(ValueSet* set) {
  // Delete all bucket lists held in the hash table.
  HashTableTraverse(&set->values, DeleteValueList, NULL);

  // Delete the hash table itself.
  HashTableDestruct(&set->values);
}

static COMPILER_UNUSED bool CanPoolFromDominator(Generator* gen, BasicBlock* block) {
  for (size_t i = 0; i < block->in_edges.length; i++) {
    BlockId id = block->in_edges.value.w[i];
    BasicBlock* in_block = gen->basic_blocks.value.p[id];
    if (BasicBlockDominatedBy(gen, block, in_block)) {
      return false;
    }
  }
  return true;
}

// This is the main function to calculate a unique key for
// an instruction.  The key is constructed as a 64 bit number as follows:
//
// +---------------------+-------------------------+-----------+
// |   operand 2 value   |   operand 1 value       |  opcode   |
// +---------------------+-------------------------+-----------+
//      22 bits              22 bits                  20 bits

// 22 bits for the value gives us 2^22 = 4 million possible values.
// That should be enough for one function.  20 bits gives us 1 million
// possible nodes in a function.

// The opcode is either the instruction id for a constant or variable
// or the instruction opcode for an expression instruction (add, mul, etc.)
// For commutative operations we sort the op values so that operand 1 value
// is less or equal to than operand 2 value.

static uint64_t CalculateInstructionKey(HashTable* table, IRNode* inst) {
  uint32_t id;
  
  // Lower 20 bits are either the node id or the opcode
  // for the instruction.  If the instruction is a variable
  // or constant we use the node id since we are pooling these.
  switch (inst->opcode) {
    case IR_OP(ssavar):
    case IR_OP(const32):
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const64):
    case IR_OP(constd):
    case IR_OP(constf):
    case IR_OP(consta):
    case IR_OP(localvar):   // Local variable.
    case IR_OP(externvar):  // External global variable.
    case IR_OP(argument):   // Function formal argument.
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(literalref):
    case IR_OP(phi):
    case IR_OP(calla):
    case IR_OP(tmp):
    // Loads must not be value-numbered together: this GVN has no memory
    // dependence/alias analysis, so two loads of the same address that look
    // identical may actually read different values when a call or store
    // between them changes memory (e.g. `if (g) ...; effect(); if (g != 1)`
    // where effect() writes the global g).  Keying loads by id keeps each one
    // distinct so a stale value is never reused.
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
    case IR_OP(structarg):
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
    case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(uinc8):
    case IR_OP(uinc16):
    case IR_OP(uinc32):
    case IR_OP(uinc64):
    case IR_OP(inca):
    case IR_OP(incf):
    case IR_OP(incd):
    case IR_OP(dec8):
    case IR_OP(dec16):
    case IR_OP(dec32):
    case IR_OP(dec64):
    case IR_OP(udec8):
    case IR_OP(udec16):
    case IR_OP(udec32):
    case IR_OP(udec64):
    case IR_OP(deca):
    case IR_OP(decf):
    case IR_OP(decd):
      id = inst->id + last_ir_opcode;
      break;
    default:
      id = inst->opcode;
      break;
  }

  uint64_t key = id;
  
  if (inst->opcode == IR_OP(phi)) {
    // Handle PHI nodes differently.  They may have more than two
    // operands and these operands may not be in dominators.  We
    // just use the node id as the key for these because that is
    // unique
    return key;
  }

  // Calls can also have more than two operands.
  // We can't reuse a call instruction anyway since they have
  // side effects.
  if (inst->opcode == IR_OP(calla)) {
    return key;
  }

  // Temp nodes need to be unique.
  if (inst->opcode == IR_OP(tmp)) {
    return key;
  }
  
  // Volatile things prevent this optimization.
  if (TypeIsVolatile(inst->type)) {
    return key;
  }
  
  // If we have a destination set, keep unique.  The key computed so far is
  // only the opcode for most instructions, which would make every dest-having
  // instruction of the same opcode compare equal (e.g. two `load -> tmp` that
  // feed the two arms of a `||`), so GVN would wrongly merge them and drop one
  // arm.  Use the instruction id to make the key genuinely unique.
  if (inst->dest != NULL) {
    return (uint64_t)inst->id + last_ir_opcode;
  }
  
  // We can only deal with 1 or 2 operands but inc and dec instructions
  // might have 3 inputs, third of which isn't really an input.
  int32_t op_values[2] = {0, 0};
  size_t n = inst->inputs.length > 2 ? 2 : inst->inputs.length;
  
  // Get the values for the operands.
  for (size_t i = 0; i < n; i++) {
    IRNode* input = inst->inputs.value.p[i];
    int64_t input_key = CalculateInstructionKey(table, input);
    Value* v = HashTableSearch(table, (void*)input_key);
    if (v == NULL) {
      // Operand is not available in this block's value set yet.
      return (uint64_t)inst->id + last_ir_opcode;
    }
    op_values[i] = v->value_number;
  }

  // If the instruction is commutative we sort the two operand
  // values to allow us to find reversed operations (add(a,b) is the same
  // as add(b,a)).
  if (IRIsCommutative(inst)) {
    if (op_values[0] > op_values[1]) {
      // Swap values.
      int32_t tmp = op_values[0];
      op_values[0] = op_values[1];
      op_values[1] = tmp;
    }
  }

  // The upper bits are the values of the operands, 22 bits for each.
  key |= (int64_t)op_values[0] << 20;
  key |= (int64_t)op_values[1] << (22 + 20);

  return key;
}

// Given an instruction use its calculated value to look up the
// table of known values.  It is found, meaning that there is
// another instruction with exactly the same value then we know
// that we can use that instruction instead.
//
// If the instruction isn't found, add a new entry in the hash
// table with the next value.
static IRNode* LookupInstruction(ValueSet* set, IRNode* inst) {
  uint64_t key = CalculateInstructionKey(&set->values, inst);
  Value* value = HashTableSearch(&set->values, (void*)key);
  if (value != NULL) {
    return value->instruction;
  }
  value = NewValue(key, set->next_value_number++, inst);
  HashTableInsert(&set->values, value);
  return inst;
}

// Copy the value list.  This needs to actually copy the vector
// of values because a block will modify the contents of the vector
// and we need to keep the dominating block unmodified for other
// dominated blocks.
static void* CopyValueList(void* list) {
  Vector* from = list;
  Vector* to = NewVector();
  VectorCopy(to, from);
  for (size_t i = 0; i < to->length; i++) {
    Value* value = ValueCopy(from->value.p[i]);
    to->value.p[i] = value;
  }
  return to;
}

static void PrintValue(Value* value) {
  printf("key: 0x%" PRIx64 ", value number: %d, node: $%d\n", value->key,
         value->value_number, value->instruction->id);
}

static void PrintValueList(void* list, void* data) {
  Vector* vec = (Vector*)list;
  for (size_t i = 0; i < vec->length; i++) {
    PrintValue(vec->value.p[i]);
  }
}

static COMPILER_UNUSED void PrintValueSet(ValueSet* set) {
  HashTableTraverse(&set->values, PrintValueList, NULL);
}

// Look at every instruction in the block.  If the instruction is an
// expression, calculate the key for it and look it up in the value set.
// If we find an instruction in the value set with the same key we replace
// the current instruction by the one we found.  Replacing the instruction
// means all references to it are moved to the other instruction and the
// instruction is removed from the code.
static void DoLocalValueNumbering(Generator* gen, ValueSet* set,
                                  BasicBlock* block) {
  IRNode* next = NULL;
  for (IRNode* inst = block->code; inst != NULL && block->end_code != NULL &&
       IRPrev(inst) != block->end_code; inst = next) {
    next = block->end_code == NULL ? NULL : IRNext(inst);
    if (IRIsExpression(inst)) {
      IRNode* prev_inst = LookupInstruction(set, inst);
      if (prev_inst != inst) {
        GeneratorReplaceInstruction(gen, inst, prev_inst);
        BasicBlockRemoveInstruction(gen, block, inst);
      }
    }
    inst = next;
  }
}

// Perform GVN (Global Value Numbering) on the basic block.  This traverses
// every instruction creating value numbers for each expression instruction.
// If it finds an instruction that has the same value we replace the current
// instruction with that previously seen one and remove the current
// instruction.  This has the effect of reusing instructions that have
// already calculated the value we need inside the block.
//
// It then does the same to all blocks dominated by this block.  It copies
// the value set from the dominator to the current block.
static void DoGlobalValueNumbering(Generator* gen, BasicBlock* block) {
  ValueSet* dominator_value_set = NULL;
  // TODO: check this and fix it.
  bool can_pool_from_dominator = true; // CanPoolFromDominator(gen, block);
  
  ValueSet* set = NULL;
  if (can_pool_from_dominator) {
    if (block->idom != NULL) {
      // If we have an immediate dominator we propagate the
      // value set from it to this node.
      dominator_value_set = block->idom->optimizer_data;
    }

    if (block->idom != NULL && block->idom->dominatees.length == 1) {
      // This block is the only one dominated by the dominator so we
      // can just reuse the value set from the dominator.
      set = dominator_value_set;
      block->idom->optimizer_data = NULL;  // This is no longer valid.
      block->optimizer_data = set;
    } else {
      // There is more than one block that is dominated by my dominator.
      // We need to copy the value set from the dominator.
      set = NewValueSet();
      block->optimizer_data = set;
      if (dominator_value_set != NULL) {
        HashTableCopy(&set->values, &dominator_value_set->values, CopyValueList);
        set->next_value_number = dominator_value_set->next_value_number;
      }
    }
  } else {
    set = NewValueSet();
    block->optimizer_data = set;
  }
  
  // Apply local value numbering algorithm on the block.
  DoLocalValueNumbering(gen, set, block);

  // printf("Value set for block %d\n", block->block_id);
  // PrintValueSet(set);
  // printf("\n");

  // Now perform value numbering on all blocks dominated by this one.
  for (size_t i = 0; i < block->dominatees.length; i++) {
    BlockId id = (BlockId)block->dominatees.value.p[i];
    BasicBlock* b = VectorGet(&gen->basic_blocks, id);
    DoGlobalValueNumbering(gen, b);
  }
}

void GlobalValueNumberingOptimization(Generator* gen) {
  DoGlobalValueNumbering(gen, gen->entry_block);

  // Done with all the value sets, delete them from every block.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    BasicBlock* block = gen->basic_blocks.value.p[i];
    if (block->optimizer_data != NULL) {
      ValueSetDelete((ValueSet*)block->optimizer_data);
      block->optimizer_data = NULL;
    }
  }
}
