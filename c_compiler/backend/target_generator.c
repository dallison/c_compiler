//
//  target_generator.c
//  c_compiler_library
//
//  Created by David Allison on 2/24/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "target_generator.h"
#include "target_basic_block.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "compiler.h"

static int next_instruction_id = 1;

static void Trap() {}
static void TrapInstruction(TargetInstruction* inst) {
  if (inst->id == 134) {
    // Set breakpoint here to trap on a certain instruction id.
    Trap();
  }
}

void TargetPrintInstruction(TargetInstruction* inst,
                            const char* (*name_func)(int), FILE* fp) {
  if (inst == NULL) {
    return;
  }
  if (inst->addr != -1) {
    fprintf(fp, "0x%04x ", inst->addr);
  }
  fprintf(fp, "@%d %s(", inst->id, name_func(inst->opcode));
  TargetConstant* c = (TargetConstant*)inst;
  switch (inst->opcode) {
    case TARGET_OP(const8):
    case TARGET_OP(const16):
    case TARGET_OP(const32):
    case TARGET_OP(const64):
      fprintf(fp, "#%" PRId64 "", c->value.ivalue);
      break;
    case TARGET_OP(constf):
    case TARGET_OP(constd):
      fprintf(fp, "#%g", c->value.dvalue);
      break;
    case TARGET_OP(symbol):
    case TARGET_OP(ivarreg):
    case TARGET_OP(fvarreg):
      fprintf(fp, "%s", ((TargetSymbol*)inst)->symbol->name.value);
      break;

    case TARGET_OP(loc): {
      TargetLocation* loc = (TargetLocation*)inst;
      const char* filename;
      int lineno;
      int start;
      int end;
      DecodeSourceLocation(loc->location, &filename, &lineno, &start, &end);
      fprintf(fp, "%s %d %d %d", filename, lineno, start, end);
      break;
    }

    default: {
      const char* sep = "";
      for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
        if (inst->operand[i] != NULL) {
          fprintf(fp, "%s@%d", sep, inst->operand[i]->id);
          sep = ", ";
        }
      }
      break;
    }
  }
  fprintf(fp, ")");
  if (inst->dest != NULL) {
    fprintf(fp, " -> @%d", inst->dest->id);
  }
  fprintf(fp, " *%zd", inst->users.length);
  if (inst->users.length > 0) {
    fprintf(fp, " [");
    const char* sep = "";
    for (size_t i = 0; i < inst->users.length; i++) {
      TargetInstruction* user = inst->users.value.p[i];
      fprintf(fp, "%s@%d", sep, user->id);
      sep = ",";
    }
    fprintf(fp, "]");
  }
  // Print flags (in hex)
  fprintf(fp, " F:%08x", inst->flags);
  fprintf(fp, " [%d]", inst->uses);
  fprintf(fp, "\n");
}

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
    case TARGET_OP(const8):
      return "constb";
    case TARGET_OP(const16):
      return "const16";
    case TARGET_OP(const32):
      return "const32";
    case TARGET_OP(const64):
      return "const64";
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

//    case TARGET_OP(rmov):
//      return "rmov";
//    case TARGET_OP(rmovf):
//      return "rmovf";
//    case TARGET_OP(rmovd):
//      return "rmovd";

    case TARGET_OP(ret):
      return "ret";

    case TARGET_OP(label):
      return "label";

    case TARGET_OP(fp):
      return "fp";  // Frame pointer pseudo operation.
    case TARGET_OP(sp):
      return "sp";  // Stack pointer pseudo operation.
    case TARGET_OP(tp):
      return "tp";  // Thread pointer pseudo operation.

    // Function result registers.
    case TARGET_OP(resulti):
      return "resulti";
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
    case TARGET_OP(named_label):
      return "namedlabel";
    case TARGET_OP(ivarreg):
      return "ivarreg";
    case TARGET_OP(fvarreg):
      return "fvarreg";
  }
}


void TargetGeneratorInit(TargetGenerator* target, Generator* gen, TargetVirtuals* virtuals) {
  target->virtuals = virtuals;
  TypeRecord* func_type = gen->func;
  Symbol* func = func_type->info.function.symbol;
  const char* func_name = func->name.value;
  StringInit(&target->function_name, func_name);
  target->is_global = !StorageIs(func->storage, STO(static));
  target->num_calls = GeneratorNumCalls(gen);
  target->varargs = gen->func->info.function.varargs;
  target->is_void = TypeIsVoid(func_type->next) ||
                      TypeIsStructOrUnion(func_type->next);
  VectorInit(&target->basic_blocks);
  target->entry_block = NULL;
  target->exit_block = NULL;

  ListInit(&target->code);
  target->stack_frame_size = 0;
  target->last_constant = NULL;
  target->first_symbol = NULL;
  target->last_symbol = NULL;
  VectorInit(&target->fixups);
  target->frame_pointer = NULL;
  target->stack_pointer = NULL;
  target->thread_pointer = NULL;
  next_instruction_id = 1;

  // Create the memcpy symbol.
  TypeRecord* memcpy_base = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecord* memcpy_func = NewFunctionTypeRecord();
  memcpy_func->info.function.unknown_args = true;
  TypeRecordChain(memcpy_func, memcpy_base);
  target->memcpy = NewSymbol("memcpy", memcpy_func, STO(extern));

  // Create the memset symbol.
  TypeRecord* memset_base = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecord* memset_func = NewFunctionTypeRecord();
  memcpy_func->info.function.unknown_args = true;
  TypeRecordChain(memset_func, memset_base);
  target->memset = NewSymbol("memset", memset_func, STO(extern));
  
  // Create the __tls_get_addr symbol.
  TypeRecord* tls_base = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecord* tls_func = NewFunctionTypeRecord();
  tls_func->info.function.unknown_args = true;
  TypeRecordChain(tls_func, tls_base);
  target->__tls_get_addr = NewSymbol("__tls_get_addr", tls_func, STO(extern));
}

void TargetGeneratorDestruct(TargetGenerator* gen) {
  ListDestruct(&gen->code);
  SymbolDelete(gen->memcpy);
  SymbolDelete(gen->memset);
  
  // Delete the basic blocks.
  for (size_t i = 0; i < gen->basic_blocks.length; i++) {
    TargetBasicBlock* block = gen->basic_blocks.value.p[i];
    TargetBasicBlockDelete(block);
  }
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
      TargetRemoveUser(op, inst);
    }
  }
  VectorDestruct(&inst->users);
  ListDeleteElement(&target->code, &inst->header);
}

void TargetAddUser(TargetInstruction* inst, TargetInstruction* user) {
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* op = inst->users.value.p[i];
    if (op == user) {
      return;
    }
  }
  VectorAppend(&inst->users, user);
  inst->uses++;
}

void TargetRemoveUser(TargetInstruction* inst, TargetInstruction* user) {
  for (size_t i = 0; i < inst->users.length; i++) {
    TargetInstruction* op = inst->users.value.p[i];
    if (op == user) {
      VectorDeleteElement(&inst->users, i);
      inst->uses--;
      if (inst->uses < 0) {
        printf("inst: %d, user: %d\n", inst->id, user->id);
      }
      assert(inst->uses >= 0);
      return;
    }
  }
}

// Move all references from old to new.
void TargetRetargetInstruction(TargetInstruction* old, TargetInstruction* new) {
  for (size_t i = 0; i < old->users.length; i++) {
    TargetInstruction* user = old->users.value.p[i];
    for (size_t j = 0; j < TARGET_MAX_OPERANDS; j++) {
      if (user->operand[j] == old) {
        user->operand[j] = new;
        TargetAddUser(new, user);
      }
      if (user->dest == old) {
        user->dest = new;
      }
    }
  }
  VectorClear(&old->users);
  old->uses = 0;
}

void TargetRetargetInstructionIf(TargetInstruction* old, TargetInstruction* new,
                                 bool (*predicate)(TargetInstruction*, void* data), void* data) {
  size_t num_retargeted = 0;
  for (size_t i = 0; i < old->users.length; i++) {
    TargetInstruction* user = old->users.value.p[i];
    for (size_t j = 0; j < TARGET_MAX_OPERANDS; j++) {
      if (user->operand[j] == old && predicate(user, data)) {
        user->operand[j] = new;
        TargetAddUser(new, user);
        num_retargeted++;
      }
    }
  }
  old->uses = (int)(old->users.length - num_retargeted);
  if (old->uses == 0) {
    VectorClear(&old->users);
  }
 
}

void TargetReplaceInstruction(TargetGenerator* target,
                              TargetInstruction* old,
                              TargetInstruction* new) {
  TargetRetargetInstruction(old, new);
  for (TargetInstruction* inst = TargetFirstInstruction(target);
       inst != NULL;
       inst = TargetNext(inst)) {
    if (inst->dest == old) {
      inst->dest = new;
    }
  }
  TargetDeleteInstruction(target, old);
}


void TargetReplaceOperand(TargetInstruction* inst, int op,
                          TargetInstruction* new) {
  TargetInstruction* old = inst->operand[op];
  if (old == new) {
    return;
  }
  // Remove reference from old to inst.
  assert(inst != new);
  TargetRemoveUser(old, inst);
  inst->operand[op] = new;
  if (new != NULL) {
    TargetAddUser(new, inst);
  }
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
  if (node->data.ptr != NULL) {
    return (TargetInstruction*)node->data.ptr;
  }
  node->data.ptr = inst;
  return inst;
}


void TargetInitInstruction(TargetInstruction* inst, TargetOpcode opcode) {
  ListElementInit(&inst->header);
  inst->id = next_instruction_id++;
  inst->opcode = opcode;
  inst->uses = 0;
  inst->addr = -1;
  inst->dest = NULL;
  inst->reg = NULL;
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    inst->operand[i] = NULL;
  }
  VectorInit(&inst->users);
  inst->block = NULL;
  inst->flags = 0;
}

void TargetUpdateOperandUsers(TargetInstruction* inst) {
  if (inst == NULL) {
    return;
  }
  for (size_t i = 0; i < TARGET_MAX_OPERANDS; i++) {
    if (inst->operand[i] != NULL) {
      TargetAddUser(inst->operand[i], inst);
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
  assert(inst != op1);
  inst->operand[0] = op1;
  TargetUpdateOperandUsers(inst);
  return inst;
}

TargetInstruction* TargetNewInstruction2(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2) {
  TargetInstruction* inst = (TargetInstruction*)TargetNewInstruction(opcode);
  assert(inst != op1);
  assert(inst != op2);
  inst->operand[0] = op1;
  inst->operand[1] = op2;
  TargetUpdateOperandUsers(inst);
  return inst;
}

TargetInstruction* TargetNewInstruction3(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2,
                                         TargetInstruction* op3) {
  TargetInstruction* inst = (TargetInstruction*)TargetNewInstruction(opcode);
  assert(inst != op1);
  assert(inst != op2);
  assert(inst != op3);
  inst->operand[0] = op1;
  inst->operand[1] = op2;
  inst->operand[2] = op3;
  TargetUpdateOperandUsers(inst);
  return inst;
}

TargetInstruction* TargetNewInstruction4(TargetOpcode opcode,
                                         TargetInstruction* op1,
                                         TargetInstruction* op2,
                                         TargetInstruction* op3,
                                         TargetInstruction* op4) {
  TargetInstruction* inst = (TargetInstruction*)TargetNewInstruction(opcode);
  assert(inst != op1);
  assert(inst != op2);
  assert(inst != op3);
  assert(inst != op4);
  inst->operand[0] = op1;
  inst->operand[1] = op2;
  inst->operand[2] = op3;
  inst->operand[3] = op4;
  TargetUpdateOperandUsers(inst);
  return inst;
}

TargetInstruction* TargetSetDest(TargetInstruction* inst,
                                TargetInstruction* dest) {
  assert(inst->dest == NULL);
  inst->dest = dest;
  // Don't add as user.
  // TargetAddUser(dest, inst);
  return inst;
}

TargetInstruction* TargetEmit(TargetGenerator* target,
                              TargetInstruction* inst) {
  if (TargetNext(inst) != NULL || TargetPrev(inst) != NULL) {
    // Already in list, nothing to do.
    return inst;
  }
  ListAppend(&target->code, &inst->header);
  TrapInstruction(inst);
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
  TrapInstruction(inst);
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
  TrapInstruction(inst);
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

TargetInstruction* TargetThreadPointer(TargetGenerator* target) {
  if (target->thread_pointer == NULL) {
    target->thread_pointer =
    TargetEmit(target, TargetNewInstruction(TARGET_OP(tp)));
  }
  return target->thread_pointer;
}
TargetInstruction* TargetNewLiteral(int id) {
  TargetLiteral* literal = malloc(sizeof(TargetLiteral));
  TargetInitInstruction(&literal->base, TARGET_OP(literal));
  literal->literal_id = id;
  return &literal->base;
}

// Constant opcodes, indexed by TargetType.
static TargetOpcode constant_ops[] = {
    TARGET_OP(const8), TARGET_OP(const16), TARGET_OP(const32), TARGET_OP(const64),
    TARGET_OP(constf), TARGET_OP(constd), TARGET_OP(const64),
};

bool TargetIsConst(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  return inst->opcode >= TARGET_OP(const8) && inst->opcode <= TARGET_OP(constd);
}

bool TargetIsZero(TargetInstruction* inst) {
  if (inst == NULL) {
    return false;
  }
  if (inst->opcode >= TARGET_OP(const8) && inst->opcode <= TARGET_OP(constd)) {
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
  c->literal_id = -1;
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
  c->literal_id = -1;
  TargetSetLoweredNode(node, (TargetInstruction*)c);
  return (TargetInstruction*)c;
}

TargetInstruction* TargetNewLocation(IRLocation* loc) {
  TargetLocation* l = malloc(sizeof(TargetLocation));
  TargetInitInstruction(&l->base, TARGET_OP(loc));
  l->location = loc->location;
  return (TargetInstruction*)l;
}

TargetInstruction* TargetNewNamedLabel(const char* name) {
  TargetNamedLabel* l = malloc(sizeof(TargetNamedLabel));
  TargetInitInstruction(&l->base, TARGET_OP(named_label));
  l->name = name;
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
    TargetBranchFixup* fixup = target->fixups.value.p[i];
    if (fixup->target == label_node) {
      TargetInstruction* label = TargetGetLoweredNode(label_node);
      fixup->inst->operand[fixup->operand] = label;
      TargetAddUser(label, fixup->inst);
    }
  }
}

void TargetRegisterInit(TargetRegister* reg, int num) {
  reg->num = num;
  reg->owner = NULL;
  reg->reserved = false;
}

const char* TargetSymbolName(Symbol* symbol, char* buf, size_t len) {
  if (symbol->flags.is_local && !TypeIsFunction(symbol->type)) {
    snprintf(buf, len, ".local.%s.%d", symbol->name.value, symbol->id);
  } else {
    if (compiler->prepend_underscore) {
      snprintf(buf, len, "_%s", symbol->name.value);
    } else {
      snprintf(buf, len, "%s", symbol->name.value);
    }
  }
  return buf;
}
