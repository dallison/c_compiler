//
//  ir.c
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "ir.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {
  IROpcode opcode;
  const char* name;
} opcodes[] = {
    {IR_OP(nop), "nop"},
    {IR_OP(tmp), "tmp"},

    // Constants.
    {IR_OP(consti), "consti"},
    {IR_OP(constb), "constb"},
    {IR_OP(consts), "consts"},
    {IR_OP(constl), "constl"},
    {IR_OP(constf), "constf"},
    {IR_OP(constd), "constd"},
    {IR_OP(consta), "consta"},

    // Moves.
    {IR_OP(movi), "movi"},
    {IR_OP(movf), "movf"},
    {IR_OP(movd), "movd"},
    {IR_OP(mova), "mova"},

    {IR_OP(rmovi), "rmovi"},
    {IR_OP(rmovf), "rmovf"},
    {IR_OP(rmovd), "rmovd"},
    {IR_OP(rmova), "rmova"},

    {IR_OP(label), "label"},
    {IR_OP(named_label), "namedlabel"},

    // loads.
    {IR_OP(loadi), "loadi"},
    {IR_OP(loadb), "loadb"},
    {IR_OP(loadl), "loadl"},
    {IR_OP(loads), "loads"},
    {IR_OP(loadui), "loadui"},
    {IR_OP(loadub), "loadub"},
    {IR_OP(loadus), "loadus"},
    {IR_OP(loadf), "loadf"},
    {IR_OP(loadd), "loadd"},
    {IR_OP(loada), "loada"},

    // stores.
    {IR_OP(storei), "storei"},
    {IR_OP(storeb), "storeb"},
    {IR_OP(stores), "stores"},
    {IR_OP(storel), "storel"},
    {IR_OP(storef), "storef"},
    {IR_OP(stored), "stored"},
    {IR_OP(storea), "storea"},

    // Add.
    {IR_OP(addi), "addi"},
    {IR_OP(addf), "addf"},
    {IR_OP(addd), "addd"},
    {IR_OP(adda), "adda"},

    // Subtract.
    {IR_OP(subi), "subi"},
    {IR_OP(subf), "subf"},
    {IR_OP(subd), "subd"},
    {IR_OP(suba), "suba"},

    // Multiply.
    {IR_OP(muli), "muli"},
    {IR_OP(mulf), "mulf"},
    {IR_OP(muld), "muld"},

    // Divide.
    {IR_OP(divi), "divi"},
    {IR_OP(divf), "divf"},
    {IR_OP(divd), "divd"},

    // Modulus.
    {IR_OP(modi), "modi"},

    // Shifts.
    {IR_OP(lsri), "lsri"},
    {IR_OP(asri), "asri"},
    {IR_OP(lsli), "lsli"},

    // Bitwise.
    {IR_OP(ori), "ori"},
    {IR_OP(andi), "andi"},
    {IR_OP(xori), "xori"},

    {IR_OP(noti), "noti"},
    {IR_OP(nota), "nota"},
    {IR_OP(onescomp), "onescomp"},
    {IR_OP(negi), "negi"},
    {IR_OP(negf), "negf"},
    {IR_OP(negd), "negd"},

    // Compares.
    {IR_OP(cmpeqi), "cmpeqi"},
    {IR_OP(cmpnei), "cmpnei"},
    {IR_OP(cmplti), "cmplti"},
    {IR_OP(cmplei), "cmplei"},
    {IR_OP(cmpgti), "cmpgti"},
    {IR_OP(cmpgei), "cmpgei"},

    {IR_OP(cmpeqf), "cmpeqf"},
    {IR_OP(cmpnef), "cmpnef"},
    {IR_OP(cmpltf), "cmpltf"},
    {IR_OP(cmplef), "cmplef"},
    {IR_OP(cmpgtf), "cmpgtf"},
    {IR_OP(cmpgef), "cmpgef"},

    {IR_OP(cmpeqd), "cmpeqd"},
    {IR_OP(cmpned), "cmpned"},
    {IR_OP(cmpltd), "cmpltd"},
    {IR_OP(cmpled), "cmpled"},
    {IR_OP(cmpgtd), "cmpgtd"},
    {IR_OP(cmpged), "cmpged"},

    {IR_OP(cmpeqa), "cmpeqa"},
    {IR_OP(cmpnea), "cmpnea"},
    {IR_OP(cmplta), "cmplta"},
    {IR_OP(cmplea), "cmplea"},
    {IR_OP(cmpgta), "cmpgta"},
    {IR_OP(cmpgea), "cmpgea"},

    // Relative branches.
    {IR_OP(btrue), "btrue"},
    {IR_OP(bfalse), "bfalse"},
    {IR_OP(bra), "bra"},
    {IR_OP(cbra), "cbra"},

    // Call and return.
    {IR_OP(calla), "calla"},
    {IR_OP(ret), "ret"},

    // Procedure entry and exit.
    {IR_OP(enter), "enter"},
    {IR_OP(leave), "leave"},

    {IR_OP(localvar), "localvar"},
    {IR_OP(externvar), "externvar"},
    {IR_OP(argument), "argument"},
    {IR_OP(staticvar), "staticvar"},
    {IR_OP(tempvar), "tempvar"},
    {IR_OP(ssavar), "ssavar"},

    {IR_OP(structreturn), "structreturn"},
    {IR_OP(addressof), "addressof"},

    {IR_OP(literalref), "literalref"},
    {IR_OP(loc), "loc"},
    {IR_OP(named_label), "label"},

    {IR_OP(resulti), "resulti"},
    {IR_OP(resultf), "resultf"},
    {IR_OP(resultd), "resultd"},
    {IR_OP(resulta), "resulta"},

    {IR_OP(i2f), "i2f"},
    {IR_OP(i2d), "i2d"},
    {IR_OP(f2d), "f2d"},
    {IR_OP(d2f), "d2f"},
    {IR_OP(f2i), "f2i"},
    {IR_OP(d2i), "d2i"},

    {IR_OP(maski), "maski"},
    {IR_OP(signextendi), "signextendi"},

    {IR_OP(memzero), "memzero"},
    {IR_OP(memcpy), "memcpy"},

    {IR_OP(phi), "phi"},
    {IR_OP(asm), "asm"},

    {IR_OP(builtin_va_start), "builtin_va_start"},
    {IR_OP(builtin_va_arg), "builtin_va_arg"},
    {IR_OP(builtin_va_end), "builtin_va_end"},
    {IR_OP(builtin_va_copy), "builtin_va_copy"},
};

const char* IROpcodeName(IROpcode opcode) {
  for (int i = 0; i < last_ir_opcode; i++) {
    if (opcodes[i].opcode == opcode) {
      return opcodes[i].name;
    }
  }
  return "unknown";
}

static int next_ir_id = 1;

void IRResetNodeId() { next_ir_id = 1; }

void IRInit(IRNode* inst, IROpcode opcode) {
  ListElementInit(&inst->header);
  inst->id = next_ir_id++;
  if (inst->id == 143) {
    printf("");
  }
  inst->opcode = opcode;
  VectorInit(&inst->inputs);
  VectorInit(&inst->outputs);
  inst->block = NULL;
  inst->flags = 0;
  inst->data.ptr = NULL;
  inst->var.def = NULL;
  inst->type = NULL;
}

void IRDestruct(IRNode* inst) {
  VectorDestruct(&inst->inputs);
  VectorDestruct(&inst->outputs);
  TypeRecordDelete(inst->type);
}

void IRDelete(IRNode* inst) {
  IRDestruct(inst);
  free(inst);
}

IRNode* NewIR(IROpcode opcode) {
  IRNode* inst = malloc(sizeof(IRNode));
  IRInit(inst, opcode);
  return inst;
}

IRNode* IRNext(IRNode* node) { return (IRNode*)node->header.next; }

IRNode* IRPrev(IRNode* node) { return (IRNode*)node->header.prev; }

void IRSetType(IRNode* node, TypeRecord* type) {
  if (type == NULL) {
    return;
  }
  TypeRecordIncRef(type);
  node->type = type;
}

bool IRInList(IRNode* node) {
  return node->header.next != NULL || node->header.prev != NULL;
}

void IRAddInput(IRNode* from, IRNode* to) {
  if (to->opcode != IR_OP(label)) {
    // For nodes other than labels we need to have seen the input before
    // we use it.  This will be caused by forgetting to emit the instruction,
    // a common error that will result in failures later.
    assert(IRInList(to));
  }
  VectorAppend(&from->inputs, to);
  VectorAppend(&to->outputs, from);
  from->type = to->type;
}

void IRSetVarUse(IRNode* inst, Symbol* var) {
  assert(inst->flags == 0);
  inst->var.use = var;
  inst->flags |= kIRVarUse;
}

void IRSetVarDef(IRNode* inst, Symbol* var) {
  assert(inst->flags == 0);
  inst->var.def = var;
  inst->flags |= kIRVarDef;
}

void IRReplaceInput(IRNode* node, size_t index, IRNode* new) {
  IRNode* existing = node->inputs.value.p[index];

  // The existing input points to an IRNode.  This node will
  // have an output pointing back to this node.  Delete the node
  // from the output list.  Then point the input for this node
  // (at the given index) to the new node and add this node
  // as an output for the new node.
  for (size_t i = 0; i < existing->outputs.length; i++) {
    IRNode* output = existing->outputs.value.p[i];
    if (output == node) {
      VectorDeleteElement(&existing->outputs, i);

      node->inputs.value.p[index] = new;
      VectorAppend(&new->outputs, node);
      return;
    }
  }
  abort();
}

void IRRemoveInput(IRNode* node, size_t index) {
  IRNode* input = node->inputs.value.p[index];
  for (size_t i = 0; i < input->outputs.length; i++) {
    IRNode* output = input->outputs.value.p[i];
    if (output == node) {
      VectorDeleteElement(&input->outputs, i);
      VectorDeleteElement(&node->inputs, index);
      return;
    }
  }
  abort();
}

void IRRemoveNode(IRNode* node) {
  // Remove this node from the outputs of all inputs.
  for (size_t i = 0; i < node->inputs.length; i++) {
    IRNode* input = node->inputs.value.p[i];
    for (size_t j = 0; j < input->outputs.length; j++) {
      if (input->outputs.value.p[j] == node) {
        VectorDeleteElement(&input->outputs, j);
        break;
      }
    }
  }

  // Remove this node from the inputs of all outputs.
  for (size_t i = 0; i < node->outputs.length; i++) {
    IRNode* output = node->outputs.value.p[i];
    for (size_t j = 0; j < output->inputs.length; j++) {
      if (output->inputs.value.p[j] == node) {
        VectorDeleteElement(&output->inputs, j);
        break;
      }
    }
  }
}

IRNode* NewIR1(IROpcode opcode, IRNode* op) {
  IRNode* inst = NewIR(opcode);
  IRAddInput(inst, op);
  return inst;
}

IRNode* NewIR2(IROpcode opcode, IRNode* op1, IRNode* op2) {
  IRNode* inst = NewIR(opcode);
  IRAddInput(inst, op1);
  IRAddInput(inst, op2);
  return inst;
}

IRNode* NewIR3(IROpcode opcode, IRNode* op1, IRNode* op2, IRNode* op3) {
  IRNode* inst = NewIR(opcode);
  IRAddInput(inst, op1);
  IRAddInput(inst, op2);
  IRAddInput(inst, op3);
  return inst;
}

IRNode* NewIR4(IROpcode opcode, IRNode* op1, IRNode* op2, IRNode* op3,
               IRNode* op4) {
  IRNode* inst = NewIR(opcode);
  IRAddInput(inst, op1);
  IRAddInput(inst, op2);
  IRAddInput(inst, op3);
  IRAddInput(inst, op4);
  return inst;
}

static struct {
  bool (*type_func)(TypeRecord*);
  IROpcode opcode;
} type_table[] = {
    {TypeIsInt, IR_OP(consti)},
    {TypeIsBool, IR_OP(consti)},
    {TypeIsShort, IR_OP(consts)},
    {TypeIsChar, IR_OP(constb)},
    {TypeIsLong, IR_OP(constl)},
    {TypeIsLongLong, IR_OP(constl)},
    {TypeIsFloat, IR_OP(constf)},
    {TypeIsDouble, IR_OP(constd)},
    {TypeIsLongDouble, IR_OP(constd)},
    {TypeIsPointerOrArray, IR_OP(consta)},
    {NULL, IR_OP(nop)},
};

static IROpcode ConstantOpcode(TypeRecord* type) {
  if (type == NULL) {
    // Allow default of integer.
    return IR_OP(consti);
  }
  for (size_t i = 0; type_table[i].type_func != NULL; i++) {
    if (type_table[i].type_func(type)) {
      return type_table[i].opcode;
    }
  }
  assert(false);
}

IRNode* NewIntIRConstant(TypeRecord* type, int64_t value) {
  IRConstant* inst = malloc(sizeof(IRConstant));
  IRInit(&inst->base, ConstantOpcode(type));
  inst->value.ivalue = value;
  IRSetType(&inst->base, type);
  return &inst->base;
}

IRNode* NewFloatingPointIRConstant(TypeRecord* type, double value) {
  IRConstant* inst = malloc(sizeof(IRConstant));
  IRInit(&inst->base, ConstantOpcode(type));
  inst->value.fvalue = value;
  IRSetType(&inst->base, type);
  return &inst->base;
}

IRNode* NewIRLocation(SourceLocation location) {
  IRLocation* inst = malloc(sizeof(IRLocation));
  IRInit(&inst->base, IR_OP(loc));
  inst->location = location;
  return &inst->base;
}

IRNode* NewIRNamedLabel(const char* name) {
  IRNamedLabel* inst = malloc(sizeof(IRNamedLabel));
  IRInit(&inst->base, IR_OP(named_label));
  inst->name = name;
  return &inst->base;
}

IRNode* NewIRVariable(Symbol* sym) {
  IRVariable* inst = malloc(sizeof(IRVariable));
  IROpcode op = IR_OP(staticvar);
  if (sym->flags.is_argument) {
    op = IR_OP(argument);
  } else if (sym->flags.is_temp) {
    op = IR_OP(tempvar);
  } else if (sym->flags.is_local) {
    if (!StorageIs(sym->storage, STO(static))) {
      op = IR_OP(localvar);
    }
  } else {
    // Global variable.  If it's extern it will have no storage.
    if (StorageIs(sym->storage, STO(extern))) {
      op = IR_OP(externvar);
    }
  }
  IRInit(&inst->base, op);
  inst->symbol = sym;
  IRSetType(&inst->base, sym->type);
  return &inst->base;
}

// New PHI node.
IRNode* NewIRPhi(Symbol* sym) {
  IRVariable* inst = malloc(sizeof(IRVariable));
  IRInit(&inst->base, IR_OP(phi));
  inst->symbol = sym;
  IRSetType(&inst->base, sym->type);
  return &inst->base;
}

IRNode* NewIRSSAVar(Symbol* sym) {
  IRVariable* inst = malloc(sizeof(IRVariable));
  IRInit(&inst->base, IR_OP(ssavar));
  inst->symbol = sym;
  IRSetType(&inst->base, sym->type);
  return &inst->base;
}

void IRPrint(IRNode* inst) {
  printf("$%d %s(", inst->id, IROpcodeName(inst->opcode));
  const char* sep = "";
  for (size_t i = 0; i < inst->inputs.length; i++) {
    IRNode* op = (IRNode*)inst->inputs.value.p[i];
    printf("%s$%d", sep, op->id);
    sep = ", ";
  }
  printf(")");
  // Print outputs.
  printf(" [");
  sep = "";
  for (size_t i = 0; i < inst->outputs.length; i++) {
    IRNode* op = (IRNode*)inst->outputs.value.p[i];
    printf("%s$%d", sep, op->id);
    sep = ", ";
  }
  printf("]");
  IRConstant* constant = (IRConstant*)inst;
  IRVariable* var = (IRVariable*)inst;
  IRLocation* loc = (IRLocation*)inst;
  switch (inst->opcode) {
    case IR_OP(consti):
    case IR_OP(constb):
    case IR_OP(consts):
    case IR_OP(constl):
    case IR_OP(consta):
      printf(" %lld", constant->value.ivalue);
      break;
    case IR_OP(constd):
      printf(" %g", constant->value.fvalue);
      break;
    case IR_OP(localvar):
    case IR_OP(externvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(phi):
    case IR_OP(ssavar):
      printf(" %s", var->symbol->name.value);
      break;
    case IR_OP(loc): {
      const char* filename;
      int lineno;
      int start;
      int end;
      DecodeSourceLocation(loc->location, &filename, &lineno, &start, &end);
      printf(" %s, %d, %d, %d", filename, lineno, start, end);
      break;
    }
    default:;
  }
  printf(" *%zd", inst->outputs.length);
  if (IRIsVarDef(inst)) {
    printf(" DEF %s", inst->var.def->name.value);
  } else if (IRIsVarRef(inst)) {
    printf(" REF %s", inst->var.use->name.value);
  }
  printf("\n");
}

bool IRIsBranch(IRNode* node) {
  return node->opcode == IR_OP(btrue) || node->opcode == IR_OP(bfalse) ||
         node->opcode == IR_OP(bra) || node->opcode == IR_OP(cbra);
}

bool IRIsConditionalBranch(IRNode* node) {
  return node->opcode == IR_OP(btrue) || node->opcode == IR_OP(bfalse);
}

bool IRIsReturn(IRNode* node) { return node->opcode == IR_OP(ret); }

bool IRIsConst(IRNode* node) {
  return node->opcode >= IR_OP(constb) && node->opcode <= IR_OP(consta);
}

bool IRIsZero(IRNode* node) {
  return IRIsConst(node) && ((IRConstant*)node)->value.ivalue == 0;
}

bool IRIsVariable(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(externvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(ssavar):
    case IR_OP(phi):
      return true;
    default:
      return false;
  }
}

bool IRIsAutoVariable(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(tempvar):
      return true;
    default:
      return false;
  }
}

bool IRIsArgument(IRNode* node) { return node->opcode == IR_OP(argument); }

bool IRIsStaticVariable(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(staticvar):
    case IR_OP(externvar):
      return true;
    default:
      return false;
  }
}

bool IRIsThreadVariable(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(staticvar):
    case IR_OP(externvar): {
      IRVariable* var = (IRVariable*)node;
      return StorageIs(var->symbol->storage, STO(thread));
    }
    default:
      return false;
  }
}
bool IRIsVarDef(IRNode* inst) { return (inst->flags & kIRVarDef) != 0; }

bool IRIsVarRef(IRNode* inst) { return (inst->flags & kIRVarUse) != 0; }

bool IRIsExpression(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(consti):
    case IR_OP(constb):
    case IR_OP(consts):
    case IR_OP(constl):
    case IR_OP(constf):
    case IR_OP(constd):
    case IR_OP(consta):

    // Moves.
    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
    case IR_OP(rmovi):
    case IR_OP(rmovf):
    case IR_OP(rmovd):
    case IR_OP(rmova):
    case IR_OP(tmp):

    case IR_OP(loadi):
    case IR_OP(loadb):
    case IR_OP(loadl):
    case IR_OP(loads):
    case IR_OP(loadui):
    case IR_OP(loadub):
    case IR_OP(loadus):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):

      // Stores are expressions;
    case IR_OP(storei):
    case IR_OP(storeb):
    case IR_OP(stores):
    case IR_OP(storel):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):

    case IR_OP(addi):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(adda):

    // Subtract.
    case IR_OP(subi):
    case IR_OP(subf):
    case IR_OP(subd):
    case IR_OP(suba):

    // Multiply.
    case IR_OP(muli):
    case IR_OP(mulf):
    case IR_OP(muld):

    // Divide.
    case IR_OP(divi):
    case IR_OP(divf):
    case IR_OP(divd):

    // Modulus.
    case IR_OP(modi):

    // Shifts.
    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(lsli):

    // Bitwise.
    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):

    case IR_OP(noti):
    case IR_OP(nota):
    case IR_OP(onescomp):
    case IR_OP(negi):
    case IR_OP(negf):
    case IR_OP(negd):

    case IR_OP(localvar):   // Local variable.
    case IR_OP(externvar):  // External global variable.
    case IR_OP(argument):   // Function formal argument.
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(ssavar):  // SSA renamed variable.

    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):

    case IR_OP(maski):
    case IR_OP(signextendi):
    case IR_OP(phi):
    case IR_OP(calla):
    case IR_OP(builtin_va_arg):

    case IR_OP(cmpeqi):
    case IR_OP(cmpnei):
    case IR_OP(cmplti):
    case IR_OP(cmplei):
    case IR_OP(cmpgti):
    case IR_OP(cmpgei):

    case IR_OP(cmpeqf):
    case IR_OP(cmpnef):
    case IR_OP(cmpltf):
    case IR_OP(cmplef):
    case IR_OP(cmpgtf):
    case IR_OP(cmpgef):

    case IR_OP(cmpeqd):
    case IR_OP(cmpned):
    case IR_OP(cmpltd):
    case IR_OP(cmpled):
    case IR_OP(cmpgtd):
    case IR_OP(cmpged):

    case IR_OP(cmpeqa):
    case IR_OP(cmpnea):
    case IR_OP(cmplta):
    case IR_OP(cmplea):
    case IR_OP(cmpgta):
    case IR_OP(cmpgea):

    case IR_OP(literalref):
      return true;
    default:
      return false;
  }
}

bool IRIsCommutative(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(addi):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(adda):

    // Multiply.
    case IR_OP(muli):
    case IR_OP(mulf):
    case IR_OP(muld):

    // Bitwise.
    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):
    case IR_OP(phi):
      return true;
    default:
      return false;
  }
}

// Returns true if the IR node is a comparison.
bool IRIsComparison(IRNode* node) {
  return node->opcode >= IR_OP(cmpeqi) && node->opcode <= IR_OP(cmpgea);
}

bool IRIsStore(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(storei):
    case IR_OP(storeb):
    case IR_OP(stores):
    case IR_OP(storel):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return true;
    default:
      return false;
  }
}

bool IRIsLoad(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(storei):
    case IR_OP(loadi):
    case IR_OP(loadb):
    case IR_OP(loadl):
    case IR_OP(loads):
    case IR_OP(loadui):
    case IR_OP(loadub):
    case IR_OP(loadus):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      return true;
    default:
      return false;
  }
}
