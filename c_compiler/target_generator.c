//
//  target_generator.c
//  c_compiler_library
//
//  Created by David Allison on 2/24/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "target_generator.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int next_instruction_id = 1;

const char* TargetOpcodeName(int op) {
  TargetOpcode opcode = (TargetOpcode)op;
  switch (opcode) {
    case TARGET_OP(save):
      return "save";
    case TARGET_OP(restore):
      return "restore";

    case TARGET_OP(symbol):
      return "symbol";  // Static symbol.
    case TARGET_OP(literal):
      return "literal";  // String literal.
    case TARGET_OP(tmp):
      return "tmp";

    // Constants.
    case TARGET_OP(constb):
      return "constb";
    case TARGET_OP(consth):
      return "consth";
    case TARGET_OP(constw):
      return "constw";
    case TARGET_OP(constx):
      return "constx";
    case TARGET_OP(constf):
      return "constf";
    case TARGET_OP(constd):
      return "constd";

    case TARGET_OP(mov):
      return "mov";
    case TARGET_OP(movf):
      return "movf";
    case TARGET_OP(movd):
      return "movd";

    case TARGET_OP(movc):
      return "movc";
    case TARGET_OP(movfc):
      return "movfc";
    case TARGET_OP(movdc):
      return "movdc";
    case TARGET_OP(movxc):
      return "movxc";

    case TARGET_OP(rmov):
      return "rmov";
    case TARGET_OP(rmovf):
      return "rmovf";
    case TARGET_OP(rmovd):
      return "rmovd";

    case TARGET_OP(ret):
      return "ret";

    case TARGET_OP(label):
      return "label";

    case TARGET_OP(fp):
      return "fp";  // Frame pointer pseudo operation.
    case TARGET_OP(sp):
      return "sp";  // Stack pointer pseudo operation.

    // Function result registers.
    case TARGET_OP(resultx):
      return "resultx";
    case TARGET_OP(resultf):
      return "resultf";
    case TARGET_OP(resultd):
      return "resultd";

    case TARGET_OP(structreturn):
      return "structreturn";  // Struct return address.

    case TARGET_OP(asm):
      return "asm";
    case TARGET_OP(loc):
      return "loc";
  }
}

void TargetPrintInstruction(TargetInstruction* inst,
                            const char* (*name_func)(int)) {
  if (inst == NULL) {
    return;
  }
  printf("@%d %s(", inst->id, name_func(inst->opcode));
  TargetConstant* c = (TargetConstant*)inst;
  switch (inst->opcode) {
    case TARGET_OP(constb):
    case TARGET_OP(consth):
    case TARGET_OP(constw):
    case TARGET_OP(constx):
      printf("#%lld", c->value.ivalue);
      break;
    case TARGET_OP(constf):
    case TARGET_OP(constd):
      printf("#%g", c->value.dvalue);
      break;
    case TARGET_OP(symbol):
      printf("%s", ((TargetSymbol*)inst)->symbol->name.value);
      break;

    case TARGET_OP(loc): {
      TargetLocation* loc = (TargetLocation*)inst;
      const char* filename;
      int lineno;
      int start;
      int end;
      DecodeSourceLocation(loc->location, &filename, &lineno, &start, &end);
      printf("%s %d %d %d", filename, lineno, start, end);
      break;
    }

    default: {
      const char* sep = "";
      for (size_t i = 0; i < 3; i++) {
        if (inst->operand[i] != NULL) {
          printf("%s@%d", sep, inst->operand[i]->id);
          sep = ", ";
        }
      }
      break;
    }
  }
  printf(") *%d\n", inst->refs);
}

void TargetGeneratorInit(TargetGenerator* target, Generator* gen) {
  TypeRecord* func_type = gen->func;
  Symbol* func = func_type->info.function.symbol;
  const char* func_name = func->name.value;
  StringInit(&target->function_name, func_name);
  target->is_global = func->storage != kStorageStatic;
  target->num_calls = GeneratorNumCalls(gen);
  target->varargs = gen->func->info.function.varargs;

  ListInit(&target->code);
  target->stack_frame_size = 0;
  target->last_constant = NULL;
  target->first_symbol = NULL;
  target->last_symbol = NULL;
  VectorInit(&target->fixups);
  target->frame_pointer = NULL;
  target->stack_pointer = NULL;
  next_instruction_id = 1;

  // Create the memcpy symbol.
  TypeRecord* memcpy_base = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecord* memcpy_func = NewFunctionTypeRecord();
  memcpy_func->info.function.unknown_args = true;
  TypeRecordChain(memcpy_func, memcpy_base);
  target->memcpy = NewSymbol("memcpy", memcpy_func, kStorageExtern);

  // Create the memset symbol.
  TypeRecord* memset_base = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecord* memset_func = NewFunctionTypeRecord();
  memcpy_func->info.function.unknown_args = true;
  TypeRecordChain(memset_func, memset_base);
  target->memset = NewSymbol("memset", memset_func, kStorageExtern);
}

void TargetGeneratorDestruct(TargetGenerator* target) {
  ListDestruct(&target->code);
  SymbolDelete(target->memcpy);
  SymbolDelete(target->memset);
}

TargetInstruction* TargetFirstInstruction(TargetGenerator* target) {
  return (TargetInstruction*)target->code.first;
}

TargetInstruction* TargetLastInstruction(TargetGenerator* target) {
  return (TargetInstruction*)target->code.last;
}

TargetInstruction* TargetLastConstant(TargetGenerator* target) {
  return (TargetInstruction*)target->last_constant;
}

TargetInstruction* TargetFirstSymbol(TargetGenerator* target) {
  return (TargetInstruction*)target->first_symbol;
}

TargetInstruction* TargetLastSymbol(TargetGenerator* target) {
  return (TargetInstruction*)target->last_symbol;
}

TargetInstruction* TargetNext(TargetInstruction* inst) {
  return (TargetInstruction*)inst->header.next;
}

TargetInstruction* TargetPrev(TargetInstruction* inst) {
  return (TargetInstruction*)inst->header.prev;
}

void TargetDeleteInstruction(TargetGenerator* target, TargetInstruction* inst) {
  // Decrement the reference count for all operands.
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* op = inst->operand[i];
    if (op != NULL) {
      op->refs--;
    }
  }
  ListDeleteElement(&target->code, &inst->header);
}

int64_t TargetIntValue(TargetInstruction* inst) {
  assert(TargetIsConst(inst));
  TargetConstant* c = (TargetConstant*)inst;
  return c->value.ivalue;
}

// Get the lowered instruction from the IR node.  This is
// held in the data.ptr field of the node.  It must
// have already been set.
TargetInstruction* TargetGetLoweredNode(IRNode* node) {
  assert(node->data.ptr != NULL);
  return node->data.ptr;
}

TargetInstruction* TargetSetLoweredNode(IRNode* node, TargetInstruction* inst) {
  assert(node->data.ptr == NULL);
  node->data.ptr = inst;
  return inst;
}

void TargetInitInstruction(TargetInstruction* inst, TargetOpcode opcode) {
  ListElementInit(&inst->header);
  inst->id = next_instruction_id++;
  inst->opcode = opcode;
  inst->refs = 0;
  inst->uses = 0;
  inst->reg = NULL;
  inst->operand[0] = NULL;
  inst->operand[1] = NULL;
  inst->operand[2] = NULL;
  inst->flags = 0;
}

void TargetUpdateRefCount(TargetInstruction* inst) {
  for (size_t i = 0; i < 3; i++) {
    if (inst->operand[i] != NULL) {
      inst->operand[i]->refs++;
    }
  }
}

TargetInstruction* TargetNewInstruction(TargetOpcode opcode) {
  TargetInstruction* inst = malloc(sizeof(TargetInstruction));
  TargetInitInstruction(inst, opcode);
  return inst;
}

TargetInstruction* TargetNewInstruction1(TargetOpcode opcode,
                                         TargetInstruction* op1) {
  TargetInstruction* inst = (TargetInstruction*)TargetNewInstruction(opcode);
  inst->operand[0] = op1;
  TargetUpdateRefCount(inst);
  return inst;
}

TargetInstruction* TargetNewInstruction2(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2) {
  TargetInstruction* inst = (TargetInstruction*)TargetNewInstruction(opcode);
  inst->operand[0] = op1;
  inst->operand[1] = op2;
  TargetUpdateRefCount(inst);
  return inst;
}

TargetInstruction* TargetNewInstruction3(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2,
                                         TargetInstruction* op3) {
  TargetInstruction* inst = (TargetInstruction*)TargetNewInstruction(opcode);
  inst->operand[0] = op1;
  inst->operand[1] = op2;
  inst->operand[2] = op3;
  TargetUpdateRefCount(inst);
  return inst;
}

TargetInstruction* TargetEmit(TargetGenerator* target,
                              TargetInstruction* inst) {
  if (TargetNext(inst) != NULL || TargetPrev(inst) != NULL) {
    // Already in list, nothing to do.
    return inst;
  }
  ListAppend(&target->code, &inst->header);
  return inst;
}

TargetInstruction* TargetEmitBefore(TargetGenerator* target,
                                    TargetInstruction* inst,
                                    TargetInstruction* pos) {
  if (TargetNext(inst) != NULL || TargetPrev(inst) != NULL) {
    // Already in list, nothing to do.
    return inst;
  }
  if (pos == NULL) {
    return TargetEmit(target, inst);
  }
  ListInsertBefore(&target->code, &inst->header, &pos->header);
  return inst;
}

TargetInstruction* TargetEmitAfter(TargetGenerator* target,
                                   TargetInstruction* inst,
                                   TargetInstruction* pos) {
  if (TargetNext(inst) != NULL || TargetPrev(inst) != NULL) {
    // Already in list, nothing to do.
    return inst;
  }
  ListInsertAfter(&target->code, &inst->header, &pos->header);
  return inst;
}

TargetInstruction* TargetEmitConstant(TargetGenerator* target,
                                      TargetInstruction* c) {
  return target->last_constant =
             TargetEmitAfter(target, c, target->last_constant);
}

TargetInstruction* TargetEmitSymbol(TargetGenerator* target,
                                    TargetInstruction* c) {
  if (target->last_symbol == NULL) {
    return target->last_symbol = target->first_symbol =
               TargetEmitAfter(target, c, target->last_constant);
  }
  return target->last_symbol = TargetEmitAfter(target, c, target->last_symbol);
}

TargetInstruction* TargetFramePointer(TargetGenerator* target) {
  if (target->frame_pointer == NULL) {
    target->frame_pointer =
        TargetEmit(target, TargetNewInstruction(TARGET_OP(fp)));
  }
  return target->frame_pointer;
}

TargetInstruction* TargetStackPointer(TargetGenerator* target) {
  if (target->stack_pointer == NULL) {
    target->stack_pointer =
        TargetEmit(target, TargetNewInstruction(TARGET_OP(sp)));
  }
  return target->stack_pointer;
}

TargetInstruction* TargetNewLiteral(int id) {
  TargetLiteral* literal = malloc(sizeof(TargetLiteral));
  TargetInitInstruction(&literal->base, TARGET_OP(literal));
  literal->literal_id = id;
  return &literal->base;
}

// Constant opcodes, indexed by TargetType.
static TargetOpcode constant_ops[] = {
    TARGET_OP(constb), TARGET_OP(consth), TARGET_OP(constw), TARGET_OP(constx),
    TARGET_OP(constf), TARGET_OP(constd), TARGET_OP(constx),
};

bool TargetIsConst(TargetInstruction* inst) {
  return inst->opcode >= TARGET_OP(constb) && inst->opcode <= TARGET_OP(constd);
}

bool TargetIsZero(TargetInstruction* inst) {
  if (inst->opcode >= TARGET_OP(constb) && inst->opcode <= TARGET_OP(constd)) {
    return TargetIntValue(inst) == 0;
  }
  return false;
}

TargetInstruction* TargetNewIntConstant(IRNode* node, TargetType type,
                                        int64_t value) {
  TargetConstant* c = malloc(sizeof(TargetConstant));
  TargetInitInstruction(&c->base, constant_ops[type]);
  c->type = type;
  c->value.ivalue = value;
  if (node != NULL) {
    TargetSetLoweredNode(node, (TargetInstruction*)c);
  }
  return (TargetInstruction*)c;
}

TargetInstruction* TargetNewFloatingPointConstant(IRNode* node, TargetType type,
                                                  double value) {
  TargetConstant* c = malloc(sizeof(TargetConstant));
  TargetInitInstruction(&c->base, constant_ops[type]);
  c->type = type;
  c->value.dvalue = value;
  TargetSetLoweredNode(node, (TargetInstruction*)c);
  return (TargetInstruction*)c;
}

TargetInstruction* TargetNewLocation(IRLocation* loc) {
  TargetLocation* l = malloc(sizeof(TargetLocation));
  TargetInitInstruction(&l->base, TARGET_OP(loc));
  l->location = loc->location;
  return (TargetInstruction*)l;
}

TargetInstruction* TargetGetIntConstant(TargetGenerator* target, IRNode* node,
                                        TargetType type, int64_t value) {
  TargetInstruction* inst = TargetFirstInstruction(target);
  while (inst != NULL && TargetPrev(inst) != TargetLastConstant(target)) {
    if (TargetIsConst(inst)) {
      TargetConstant* c = (TargetConstant*)inst;
      if (c->type == type && c->value.ivalue == value) {
        if (node != NULL && node->data.ptr == NULL) {
          TargetSetLoweredNode(node, inst);
        }
        return inst;
      }
    }
    inst = TargetNext(inst);
  }
  return TargetEmitConstant(target, TargetNewIntConstant(node, type, value));
}

TargetInstruction* TargetGetFloatingPointConstant(TargetGenerator* target,
                                                  IRNode* node, TargetType type,
                                                  double value) {
  TargetInstruction* inst = TargetFirstInstruction(target);
  while (inst != NULL && TargetPrev(inst) != TargetLastConstant(target)) {
    TargetConstant* c = (TargetConstant*)inst;
    if (TargetIsConst(inst)) {
      if (c->type == type && c->value.dvalue == value) {
        if (node != NULL && node->data.ptr == NULL) {
          TargetSetLoweredNode(node, inst);
        }
        return inst;
      }
    }
    inst = TargetNext(inst);
  }
  return TargetEmitConstant(target,
                            TargetNewFloatingPointConstant(node, type, value));
}

TargetInstruction* NewTargetSymbol(Symbol* symbol) {
  TargetSymbol* inst = malloc(sizeof(TargetSymbol));
  TargetInitInstruction(&inst->base, TARGET_OP(symbol));
  inst->symbol = symbol;
  return &inst->base;
}

TargetInstruction* TargetGetSymbol(TargetGenerator* target, IRNode* node,
                                   Symbol* symbol) {
  TargetInstruction* inst = TargetFirstSymbol(target);
  while (inst != NULL && TargetPrev(inst) != TargetLastSymbol(target)) {
    TargetSymbol* s = (TargetSymbol*)inst;
    if (inst->opcode == TARGET_OP(symbol)) {
      if (s->symbol == symbol) {
        if (node != NULL && node->data.ptr == NULL) {
          TargetSetLoweredNode(node, inst);
        }
        return inst;
      }
    }
    inst = TargetNext(inst);
  }
  return TargetEmitSymbol(target, NewTargetSymbol(symbol));
}

TargetBranchFixup* NewBranchFixup(TargetInstruction* inst, IRNode* target,
                                  int operand) {
  TargetBranchFixup* fixup = malloc(sizeof(TargetBranchFixup));
  fixup->inst = inst;
  fixup->target = target;
  fixup->operand = operand;
  return fixup;
}

void TargetApplyFixups(TargetGenerator* target, IRNode* label_node) {
  for (size_t i = 0; i < target->fixups.length; i++) {
    TargetBranchFixup* fixup = target->fixups.value[i];
    if (fixup->target == label_node) {
      fixup->inst->operand[fixup->operand] = TargetGetLoweredNode(label_node);
    }
  }
}

void TargetRegisterInit(TargetRegister* reg, int num) {
  reg->num = num;
  reg->owner = NULL;
  reg->reserved = false;
}
