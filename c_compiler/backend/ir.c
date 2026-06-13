//
//  ir.c
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "ir.h"
#include "compiler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// To avoid passing this around to lots of functions.
static SourceLocation current_location;

void IRSetLocation(SourceLocation loc) {
  current_location = loc;
}

static struct {
  IROpcode opcode;
  const char* name;
} opcodes[] = {
    {IR_OP(nop), "nop"},
    {IR_OP(tmp), "tmp"},

    // Constants.
    {IR_OP(const32), "const32"},
    {IR_OP(const8), "const8"},
    {IR_OP(const16), "const16"},
    {IR_OP(const64), "const64"},
    {IR_OP(constf), "constf"},
    {IR_OP(constd), "constd"},
    {IR_OP(consta), "consta"},

    // Moves.
    {IR_OP(movi), "movi"},
    {IR_OP(movf), "movf"},
    {IR_OP(movd), "movd"},
    {IR_OP(mova), "mova"},

//    {IR_OP(rmovi), "rmovi"},
//    {IR_OP(rmovf), "rmovf"},
//    {IR_OP(rmovd), "rmovd"},
//    {IR_OP(rmova), "rmova"},

    {IR_OP(label), "label"},
    {IR_OP(named_label), "namedlabel"},

    // loads.
    {IR_OP(load32), "load32"},
    {IR_OP(load8), "load8"},
    {IR_OP(load64), "load64"},
    {IR_OP(load16), "load16"},
    {IR_OP(loadu32), "loadu32"},
    {IR_OP(loadu8), "loadu8"},
    {IR_OP(loadu16), "loadu16"},
    {IR_OP(loadf), "loadf"},
    {IR_OP(loadd), "loadd"},
    {IR_OP(loada), "loada"},
    {IR_OP(structarg), "structarg"},

    // stores.
    {IR_OP(store32), "store32"},
    {IR_OP(store8), "store8"},
    {IR_OP(store16), "store16"},
    {IR_OP(store64), "store64"},
    {IR_OP(storef), "storef"},
    {IR_OP(stored), "stored"},
    {IR_OP(storea), "storea"},

    {IR_OP(getbit), "getbit"},

    // Store bit field in value of given size.
    {IR_OP(setbit), "setbit"},

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

    {IR_OP(pusharg), "pusharg"},

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

    {IR_OP(zeroextendi), "zeroextendi"},
    {IR_OP(signextendi), "signextendi"},
  {IR_OP(aligni), "signextendi"},

    {IR_OP(memzero), "memzero"},
    {IR_OP(memcpy), "memcpy"},
    {IR_OP(cast), "cast"},
  
    {IR_OP(phi), "phi"},
    {IR_OP(asm), "asm"},
    {IR_OP(nrvoval), "nvroval"},

  {IR_OP(decsp), "decsp"},
  {IR_OP(savesp), "savesp"},
  {IR_OP(restoresp), "restoresp"},

    {IR_OP(builtin_va_start), "builtin_va_start"},
    {IR_OP(builtin_va_arg), "builtin_va_arg"},
    {IR_OP(builtin_va_end), "builtin_va_end"},
    {IR_OP(builtin_va_copy), "builtin_va_copy"},
  
  {IR_OP(inc8), "inc8"},
  {IR_OP(inc16), "inc16"},
  {IR_OP(inc32), "inc32"},
  {IR_OP(inc64), "inc64"},
  {IR_OP(uinc8), "uinc8"},
  {IR_OP(uinc16), "uinc16"},
  {IR_OP(uinc32), "uinc32"},
  {IR_OP(uinc64), "uinc64"},
 {IR_OP(inca), "inca"},
  {IR_OP(incf), "incf"},
  {IR_OP(incd), "incd"},
  
    {IR_OP(dec8), "dec8"},
    {IR_OP(dec16), "dec16"},
    {IR_OP(dec32), "dec32"},
    {IR_OP(dec64), "dec64"},
  {IR_OP(udec8), "udec8"},
  {IR_OP(udec16), "udec16"},
  {IR_OP(udec32), "udec32"},
  {IR_OP(udec64), "udec64"},
   {IR_OP(deca), "deca"},
    {IR_OP(decf), "decf"},
    {IR_OP(decd), "decd"},
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
  inst->opcode = opcode;
  VectorInit(&inst->inputs);
  VectorInit(&inst->outputs);
  inst->block = NULL;
  inst->flags = 0;
  inst->data.ptr = NULL;
  inst->var.def = NULL;
  inst->type = NULL;
  inst->location = current_location;
  inst->aux = NULL;
  inst->dest = NULL;
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

IRNode* IRSetType(IRNode* node, TypeRecord* type) {
  if (type == NULL || type == node->type) {
    return node;
  }
  if (node->type != NULL) {
    // Replacing a type.
    TypeRecordDelete(node->type);
  }
  TypeRecordIncRef(type);
  node->type = type;
  return node;
}

bool IRInList(IRNode* node) {
  return node->header.next != NULL || node->header.prev != NULL;
}

void IRAddInput(IRNode* from, IRNode* to, bool copy_type) {
  if (to->opcode != IR_OP(label)) {
    // For nodes other than labels we need to have seen the input before
    // we use it.  This will be caused by forgetting to emit the instruction,
    // a common error that will result in failures later.
    assert(IRInList(to));
  }
  VectorAppend(&from->inputs, to);
  VectorAppend(&to->outputs, from);
  if (copy_type) {
    IRSetType(from, to->type);
  }
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
  IRAddInput(inst, op, true);
  return inst;
}

IRNode* NewIR2(IROpcode opcode, IRNode* op1, IRNode* op2) {
  IRNode* inst = NewIR(opcode);
  IRAddInput(inst, op1, true);
  IRAddInput(inst, op2, false);
  return inst;
}

IRNode* NewIR3(IROpcode opcode, IRNode* op1, IRNode* op2, IRNode* op3) {
  IRNode* inst = NewIR(opcode);
  IRAddInput(inst, op1, true);
  IRAddInput(inst, op2, false);
  IRAddInput(inst, op3, false);
  return inst;
}

IRNode* NewIR4(IROpcode opcode, IRNode* op1, IRNode* op2, IRNode* op3,
               IRNode* op4) {
  IRNode* inst = NewIR(opcode);
  IRAddInput(inst, op1, true);
  IRAddInput(inst, op2, false);
  IRAddInput(inst, op3, false);
  IRAddInput(inst, op4, false);
  return inst;
}

static struct {
  bool (*type_func)(TypeRecord*);
  IROpcode opcode;
} type_table[] = {
    {TypeIsFloat, IR_OP(constf)},
    {TypeIsDouble, IR_OP(constd)},
    {TypeIsLongDouble, IR_OP(constd)},
  {TypeIsPointerOrArray, IR_OP(consta)},
  {TypeIsStructOrUnion, IR_OP(consta)},
    {NULL, IR_OP(nop)},
};

static struct {
  int size;
  IROpcode opcode;
} int_size_to_opcode[] = {
  {1, IR_OP(const8)},
  {2, IR_OP(const16)},
  {4, IR_OP(const32)},
  {8, IR_OP(const64)},
  {0, IR_OP(nop)},
};

static IROpcode IntConstOpcode(TypeRecord* type) {
  int size = 0;
  if (type == NULL) {
    size = compiler->int_size;
  } else if (TypeIsShort(type)) {
    size = compiler->short_size;
  } else if (TypeIsBool(type)) {
    size = compiler->bool_size;
  } else if (TypeIsChar(type)) {
    size = 1;
  } else if (TypeIsLong(type)) {
    size = compiler->long_size;
  } else if (TypeIsLongLong(type)) {
    size = compiler->long_long_size;
  } else if (TypeIsInt(type)) {
    size = compiler->int_size;
  }
  for (size_t i = 0; int_size_to_opcode[i].size != 0; i++) {
    if (size == int_size_to_opcode[i].size) {
      return int_size_to_opcode[i].opcode;
    }
  }
  abort();
}

static IROpcode ConstantOpcode(TypeRecord* type) {
  if (type == NULL) {
    // Allow default of integer.
    return IntConstOpcode(NULL);
  }
  if (TypeIsIntegral(type)) {
    return IntConstOpcode(type);
  }
  for (size_t i = 0; type_table[i].type_func != NULL; i++) {
    if (type_table[i].type_func(type)) {
      return type_table[i].opcode;
    }
  }
  assert(false);
  return IR_OP(nop);
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

void IRPrint(IRNode* inst, FILE* fp) {
  fprintf(fp, "$%d %s(", inst->id, IROpcodeName(inst->opcode));
  const char* sep = "";
  for (size_t i = 0; i < inst->inputs.length; i++) {
    IRNode* op = (IRNode*)inst->inputs.value.p[i];
    fprintf(fp, "%s$%d", sep, op->id);
    sep = ", ";
  }
  fprintf(fp, ")");
  // Print outputs.
  fprintf(fp, " [");
  sep = "";
  for (size_t i = 0; i < inst->outputs.length; i++) {
    IRNode* op = (IRNode*)inst->outputs.value.p[i];
    fprintf(fp, "%s$%d", sep, op->id);
    sep = ", ";
  }
  fprintf(fp, "]");
  IRConstant* constant = (IRConstant*)inst;
  IRVariable* var = (IRVariable*)inst;
  IRLocation* loc = (IRLocation*)inst;
  switch (inst->opcode) {
    case IR_OP(const32):
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const64):
    case IR_OP(consta):
      fprintf(fp, " %" PRId64 "", constant->value.ivalue);
      break;
    case IR_OP(constd):
      fprintf(fp, " %g", constant->value.fvalue);
      break;
    case IR_OP(localvar):
    case IR_OP(externvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(phi):
    case IR_OP(ssavar):
      fprintf(fp, " %s", var->symbol->name.value);
      break;
    case IR_OP(loc): {
      const char* filename;
      int lineno;
      int start;
      int end;
      DecodeSourceLocation(loc->location, &filename, &lineno, &start, &end);
      fprintf(fp, " %s, %d, %d, %d", filename, lineno, start, end);
      break;
    }
    case IR_OP(calla):
      if ((inst->flags & kIRTailCall) != 0) {
        fprintf(fp, " [tail]");
      }
      break;
    default:
      break;
  }
  fprintf(fp, " *%zd", inst->outputs.length);
  if (IRIsVarDef(inst)) {
    fprintf(fp, " DEF %s", inst->var.def->name.value);
  } else if (IRIsVarRef(inst)) {
    fprintf(fp, " REF %s", inst->var.use->name.value);
  }
  if (inst->flags != 0) {
    fprintf(fp, " {");
    const char* sep = "";
    static const char* kFlagNames[] = {
      "vardef",
      "varuse",
      "tailcall",
      "returnjump",
      "rvocall",
      "nrvomarker",
      "jumptablebranch",
      "fromcall",
    };
    for (int i = 0; i < 32; i++) {
      if ((inst->flags & (1 << i)) != 0) {
        fprintf(fp,"%s%s", sep, kFlagNames[i]);
        sep = ",";
      }
    }
    fprintf(fp, "}");
  }
 
  if (inst->dest != NULL) {
    fprintf(fp, " -> $%d", inst->dest->id);
  }
  fprintf(fp, "\n");
}

bool IRIsBranch(IRNode* node) {
  return node->opcode == IR_OP(btrue) || node->opcode == IR_OP(bfalse) ||
         node->opcode == IR_OP(bra) || node->opcode == IR_OP(cbra);
}

bool IRIsUnconditionalBranch(IRNode* node) {
  return node->opcode == IR_OP(bra);
}

bool IRIsConditionalBranch(IRNode* node) {
  return node->opcode == IR_OP(btrue) || node->opcode == IR_OP(bfalse);
}

bool IRIsReturn(IRNode* node) { return node->opcode == IR_OP(ret); }

bool IRIsCall(IRNode* node) { return node->opcode == IR_OP(calla); }

bool IRIsConst(IRNode* node) {
  return node->opcode >= IR_OP(const8) && node->opcode <= IR_OP(consta);
}

bool IRIsZero(IRNode* node) {
  return IRIsConst(node) && ((IRConstant*)node)->value.ivalue == 0;
}

bool IRIsIntConst(IRNode* node) {
  return IRIsConst(node) && node->opcode != IR_OP(constf) && node->opcode != IR_OP(constd);
}

int64_t IRIntConstValue(IRNode* node) {
  IRConstant* c = (IRConstant*)node;
  return c->value.ivalue;
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
    case IR_OP(structreturn):
    case IR_OP(tmp):
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

bool IRIsConstant(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(const32):
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const64):
    case IR_OP(constf):
    case IR_OP(constd):
    case IR_OP(consta):
      return true;
    default:
      return false;
  }
}

bool IRIsExpression(IRNode* inst) {
  switch (inst->opcode) {
    case IR_OP(const32):
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const64):
    case IR_OP(constf):
    case IR_OP(constd):
    case IR_OP(consta):

    // Moves.
    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
//    case IR_OP(rmovi):
//    case IR_OP(rmovf):
//    case IR_OP(rmovd):
//    case IR_OP(rmova):
    case IR_OP(tmp):

    case IR_OP(load32):
    case IR_OP(load8):
    case IR_OP(load64):
    case IR_OP(load16):
    case IR_OP(loadu32):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
    case IR_OP(structarg):

      // Stores are expressions;
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
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

    case IR_OP(zeroextendi):
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
    case IR_OP(addressof):
    case IR_OP(cast):
    case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(dec8):
    case IR_OP(dec16):
    case IR_OP(dec32):
    case IR_OP(dec64):
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

bool IRIsStoreOnly(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return true;
    default:
      return false;
  }
}
bool IRIsStore(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(store32):
    case IR_OP(store8):
    case IR_OP(store16):
    case IR_OP(store64):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
    case IR_OP(addressof):
    case IR_OP(cast):
    case IR_OP(builtin_va_start):
    case IR_OP(builtin_va_end):
    case IR_OP(builtin_va_arg):
    case IR_OP(builtin_va_copy):
    case IR_OP(memzero):
    case IR_OP(memcpy):
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
    case IR_OP(setbit):
     return true;
    default:
      return IRIsIncDec(node);
  }
}

bool IRIsIncDec(IRNode* node) {
  switch (node->opcode) {
      case IR_OP(inc8):
    case IR_OP(inc16):
    case IR_OP(inc32):
    case IR_OP(inc64):
    case IR_OP(inca):
    case IR_OP(incf):
    case IR_OP(incd):
    case IR_OP(dec16):
    case IR_OP(dec32):
    case IR_OP(dec64):
    case IR_OP(deca):
    case IR_OP(decf):
    case IR_OP(decd):
      return true;
    default:
      return false;
  }
}
  
bool IRIsLoad(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(load32):
    case IR_OP(load8):
    case IR_OP(load64):
    case IR_OP(load16):
    case IR_OP(loadu32):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
    case IR_OP(structarg):
    case IR_OP(pusharg):
    case IR_OP(addressof):
    case IR_OP(cast):
    case IR_OP(getbit):
      return true;
    default:
      return IRIsIncDec(node);
  }
}


bool IRIsLoadOnly(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(load32):
    case IR_OP(load8):
    case IR_OP(load64):
    case IR_OP(load16):
    case IR_OP(loadu32):
    case IR_OP(loadu8):
    case IR_OP(loadu16):
    case IR_OP(loadf):
    case IR_OP(loadd):
    case IR_OP(loada):
      return true;
    default:
      return false;
  }
}
bool IRIsResult(IRNode* node) {
  switch (node->opcode) {
    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
       return true;
    default:
      return false;
  }
}
