//
//  6502_codegen.c
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#include "6502_codegen.h"
#include <assert.h>
#include <stdlib.h>
#include "compiler.h"

const char* _6502OpcodeName(int op) {
  _6502Opcode opcode = op;
  switch (opcode) {
    default:
      // Use the generic TargetOpcodeName for all non-6502 specific
      // opcodes.
      return TargetOpcodeName((TargetOpcode)opcode);

    case _6502_OP(movac):
      return "movac";
    case _6502_OP(mova):
      return "mova";
    case _6502_OP(rmova):
      return "rmova";
    case _6502_OP(rmovx):
      return "rmovx";
    case _6502_OP(resulta):
      return "resulta";
    case _6502_OP(resulti):
      return "resulti";
    case _6502_OP(tmpb):
      return "tmpb";
    case _6502_OP(tmpa):
      return "tmpa";
    case _6502_OP(tmpx):
      return "tmpx";

    case _6502_OP(enter):
      return "enter";
    case _6502_OP(rts):
      return "rts";

    case _6502_OP(localvar):
      return "localvar";

    case _6502_OP(argument):
      return "argument";

    case _6502_OP(ap):
      return "ap";
    case _6502_OP(decsp):
      return "decsp";
    case _6502_OP(incsp):
      return "incsp";
      
    case _6502_OP(pusha):
      return "pusha";
    case _6502_OP(pushi):
      return "pushi";
    case _6502_OP(pushh):
      return "pushh";
    case _6502_OP(pushb):
      return "pushb";
    case _6502_OP(pushf):
      return "pushf";
    case _6502_OP(pushd):
      return "pushd";
    case _6502_OP(pushx):
      return "pushx";
    case _6502_OP(popa):
      return "popa";
    case _6502_OP(popi):
      return "popi";
    case _6502_OP(poph):
      return "poph";
    case _6502_OP(popb):
      return "popb";
    case _6502_OP(popf):
      return "popf";
    case _6502_OP(popd):
      return "popd";
    case _6502_OP(popx):
      return "popx";
      
    case _6502_OP(lda):
      return "lda";
    case _6502_OP(ldw):
      return "ldw";
    case _6502_OP(ldh):
      return "ldh";
    case _6502_OP(lduw):
      return "lduw";
    case _6502_OP(ldub):
      return "ldub";
    case _6502_OP(lduh):
      return "lduh";
    case _6502_OP(ldf):
      return "ldf";
    case _6502_OP(ldd):
      return "ldd";
    case _6502_OP(ldb):
      return "ldb";
    case _6502_OP(ldx):
      return "ldx";
      
    case _6502_OP(sta):
      return "sta";
    case _6502_OP(stw):
      return "stw";
    case _6502_OP(sth):
      return "sth";
    case _6502_OP(stx):
      return "stx";
    case _6502_OP(stf):
      return "stf";
    case _6502_OP(std):
      return "std";
    case _6502_OP(stb):
      return "stb";
      
      // Add.
    case _6502_OP(adda):
      return "adda";
    case _6502_OP(add):
      return "add";
    case _6502_OP(addf):
      return "addf";
    case _6502_OP(addd):
      return "addd";
    case _6502_OP(addc):
      return "addc";
    case _6502_OP(addac):
      return "addac";
      
      // Subtract.
    case _6502_OP(sub):
      return "sub";
    case _6502_OP(subf):
      return "subf";
    case _6502_OP(subd):
      return "subd";
      
      // Multiply.
    case _6502_OP(mula):
      return "mula";
    case _6502_OP(mul):
      return "mul";
    case _6502_OP(mulf):
      return "mulf";
    case _6502_OP(muld):
      return "muld";
      
      // Divide.
    case _6502_OP(div):
      return "div";
    case _6502_OP(divu):
      return "divu";
    case _6502_OP(divf):
      return "divf";
    case _6502_OP(divd):
      return "divd";
      
      // Modulus.
    case _6502_OP(mod):
      return "mod";
    case _6502_OP(modu):
      return "modu";
      
      // Shifts.
    case _6502_OP(lsr):
      return "lsr";
    case _6502_OP(asr):
      return "asr";
    case _6502_OP(lsl):
      return "lsl";
      
      // Bitwise.
    case _6502_OP(or):
      return "or";
    case _6502_OP(and):
      return "and";
    case _6502_OP(xor):
      return "xor";
      
    case _6502_OP(not):
      return "not";
    case _6502_OP(inv):
      return "inv";
    case _6502_OP(neg):
      return "neg";
    case _6502_OP(negf):
      return "negf";
    case _6502_OP(negd):
      return "negd";
      
      // Compares.
    case _6502_OP(cmpeqb):
      return "cmpeqb";
    case _6502_OP(cmpneb):
      return "cmpneb";
    case _6502_OP(cmpltb):
      return "cmpltb";
    case _6502_OP(cmpleb):
      return "cmpleb";
    case _6502_OP(cmpgtb):
      return "cmpgtb";
    case _6502_OP(cmpgeb):
      return "cmpgeb";
    case _6502_OP(cmpltub):
      return "cmpltub";
    case _6502_OP(cmpleub):
      return "cmpleub";
    case _6502_OP(cmpgtub):
      return "cmpgtub";
    case _6502_OP(cmpgeub):
      return "cmpgeub";

    case _6502_OP(cmpeqa):
      return "cmpeqa";
    case _6502_OP(cmpnea):
      return "cmpnea";
    case _6502_OP(cmplta):
      return "cmplta";
    case _6502_OP(cmplea):
      return "cmplea";
    case _6502_OP(cmpgta):
      return "cmpgta";
    case _6502_OP(cmpgea):
      return "cmpgea";
    case _6502_OP(cmpltua):
      return "cmpltua";
    case _6502_OP(cmpleua):
      return "cmpleua";
    case _6502_OP(cmpgtua):
      return "cmpgtua";
    case _6502_OP(cmpgeua):
      return "cmpgeua";

    case _6502_OP(cmpeqi):
      return "cmpeqi";
    case _6502_OP(cmpnei):
      return "cmpnei";
    case _6502_OP(cmplti):
      return "cmplti";
    case _6502_OP(cmplei):
      return "cmplei";
    case _6502_OP(cmpgti):
      return "cmpgti";
    case _6502_OP(cmpgei):
      return "cmpgei";
    case _6502_OP(cmpltui):
      return "cmpltui";
    case _6502_OP(cmpleui):
      return "cmpleui";
    case _6502_OP(cmpgtui):
      return "cmpgtui";
    case _6502_OP(cmpgeui):
      return "cmpgeui";

    case _6502_OP(cmpeqx):
      return "cmpeqx";
    case _6502_OP(cmpnex):
      return "cmpnex";
    case _6502_OP(cmpltx):
      return "cmpltx";
    case _6502_OP(cmplex):
      return "cmplex";
    case _6502_OP(cmpgtx):
      return "cmpgtx";
    case _6502_OP(cmpgex):
      return "cmpgex";
    case _6502_OP(cmpltux):
      return "cmpltux";
    case _6502_OP(cmpleux):
      return "cmpleux";
    case _6502_OP(cmpgtux):
      return "cmpgtux";
    case _6502_OP(cmpgeux):
      return "cmpgeux";

    case _6502_OP(cmpeqf):
      return "cmpeqf";
    case _6502_OP(cmpnef):
      return "cmpnef";
    case _6502_OP(cmpltf):
      return "cmpltf";
    case _6502_OP(cmplef):
      return "cmplef";
    case _6502_OP(cmpgtf):
      return "cmpgtf";
    case _6502_OP(cmpgef):
      return "cmpgef";
      
    case _6502_OP(cmpeqd):
      return "cmpeqd";
    case _6502_OP(cmpned):
      return "cmpned";
    case _6502_OP(cmpltd):
      return "cmpltd";
    case _6502_OP(cmpled):
      return "cmpled";
    case _6502_OP(cmpgtd):
      return "cmpgtd";
    case _6502_OP(cmpged):
      return "cmpged";
      
      // Relative branches.
    case _6502_OP(bt):
      return "bt";
    case _6502_OP(bf):
      return "bf";
    case _6502_OP(bra):
      return "bra";
     case _6502_OP(cbra):
      return "cbra";  // Computed branch.
      
    case _6502_OP(i2f):
      return "i2f";
    case _6502_OP(i2d):
      return "i2d";
    case _6502_OP(ui2f):
      return "ui2f";
    case _6502_OP(ui2d):
      return "ui2d";
    case _6502_OP(f2d):
      return "f2d";
    case _6502_OP(d2f):
      return "d2f";
    case _6502_OP(f2i):
      return "f2i";
    case _6502_OP(d2i):
      return "d2i";
    case _6502_OP(f2ui):
      return "f2ui";
    case _6502_OP(d2ui):
      return "d2ui";
      
      // Absolute jump.
    case _6502_OP(jmp):
      return "jmp";
    case _6502_OP(cjmp):
      return "cjmp";
      
    case _6502_OP(adr):
      return "adr";
    case _6502_OP(adrs):
      return "adrs";
    case _6502_OP(adrtls):
      return "adrtls";
      
      // Call and return.
    case _6502_OP(calla):
      return "calla";
    case _6502_OP(calli):
      return "calli";
    case _6502_OP(callx):
      return "callx";
    case _6502_OP(callf):
      return "callf";
    case _6502_OP(calld):
      return "calld";
    case _6502_OP(rcalla):
      return "rcalla";
    case _6502_OP(rcalli):
      return "rcalli";
    case _6502_OP(rcallx):
      return "rcallx";
    case _6502_OP(rcallf):
      return "rcallf";
    case _6502_OP(rcalld):
      return "rcalld";
    case _6502_OP(ret):
      return "ret";
  }
}

static TargetInstruction* ArgumentPointer(_6502Generator* g) {
  if (g->argument_pointer == NULL) {
    g->argument_pointer =
    TargetEmit(&g->base, TargetNewInstruction((TargetOpcode)_6502_OP(ap)));
  }
  return g->argument_pointer;
}

void _6502GeneratorInit(_6502Generator* g, Generator* gen) {
  TargetGeneratorInit(&g->base, gen);
  
  g->argument_pointer = NULL;
  _6502RegisterAllocatorInit(&g->register_allocator, g);
}

_6502Generator* New6502Generator(Generator* gen) {
  _6502Generator* g = malloc(sizeof(_6502Generator));
  _6502GeneratorInit(g, gen);
  return g;
}

void _6502GeneratorDestruct(_6502Generator* g) {
  TargetGeneratorDestruct(&g->base);
  _6502RegisterAllocatorDestruct(&g->register_allocator);
}

void _6502GeneratorDelete(_6502Generator* g) {
  _6502GeneratorDestruct(g);
  free(g);
}

// Some static utility functions that map to generic target functions.
static TargetInstruction* NewInstruction1(_6502Opcode opcode,
                                          TargetInstruction* op1) {
  return TargetNewInstruction1((TargetOpcode)opcode, op1);
}

static TargetInstruction* NewInstruction2(_6502Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2) {
  return TargetNewInstruction2((TargetOpcode)opcode, op1, op2);
}

static TargetInstruction* NewInstruction3(_6502Opcode opcode,
                                          TargetInstruction* op1,
                                          TargetInstruction* op2,
                                          TargetInstruction* op3) {
  return TargetNewInstruction3((TargetOpcode)opcode, op1, op2, op3);
}

static TargetInstruction* Emit(_6502Generator* g, TargetInstruction* inst) {
  return TargetEmit(&g->base, inst);
}

static TargetInstruction* EmitBefore(_6502Generator* g,
                                     TargetInstruction* inst,
                                     TargetInstruction* pos) {
  return TargetEmitBefore(&g->base, inst, pos);
}

static TargetInstruction* EmitAfter(_6502Generator* g,
                                    TargetInstruction* inst,
                                    TargetInstruction* pos) {
  return TargetEmitAfter(&g->base, inst, pos);
}

static TargetInstruction* EmitConstant(_6502Generator* g,
                                       TargetInstruction* c) {
  return TargetEmitConstant(&g->base, c);
}

static TargetInstruction* EmitSymbol(_6502Generator* g,
                                     TargetInstruction* c) {
  return TargetEmitSymbol(&g->base, c);
}

static TargetInstruction* FramePointer(_6502Generator* g) {
  return TargetFramePointer(&g->base);
}

static TargetInstruction* StackPointer(_6502Generator* g) {
  return TargetStackPointer(&g->base);
}

static TargetInstruction* ThreadPointer(_6502Generator* g) {
  return TargetThreadPointer(&g->base);
}

static TargetInstruction* GetLoweredNode(IRNode* node) {
  return TargetGetLoweredNode(node);
}

static TargetInstruction* SetLoweredNode(IRNode* node,
                                         TargetInstruction* inst) {
  return TargetSetLoweredNode(node, inst);
}

static TargetInstruction* GetIntConstant(_6502Generator* g, IRNode* node,
                                         TargetType type, int64_t value) {
  return TargetGetIntConstant(&g->base, node, type, value);
}

static TargetInstruction* GetFloatingPointConstant(_6502Generator* g,
                                                   IRNode* node,
                                                   TargetType type,
                                                   double value) {
  return TargetGetFloatingPointConstant(&g->base, node, type, value);
}

static TargetInstruction* GetSymbol(_6502Generator* g, IRNode* node,
                                    Symbol* symbol) {
  return TargetGetSymbol(&g->base, node, symbol);
}

static TargetInstruction* NewInstruction(_6502Opcode opcode) {
  return TargetNewInstruction((TargetOpcode)opcode);
}

static struct {
  bool (*type_func)(TypeRecord*);
  _6502Opcode load;
} load_opcodes[] = {
  {TypeIsInt, _6502_OP(ldw)},
  {TypeIsShort, _6502_OP(ldh)},
  {TypeIsChar, _6502_OP(ldb)},
  {TypeIsLong, _6502_OP(ldx)},
  {TypeIsLongLong, _6502_OP(ldx)},
  {TypeIsUnsignedInt, _6502_OP(lduw)},
  {TypeIsUnsignedShort, _6502_OP(lduh)},
  {TypeIsUnsignedChar, _6502_OP(ldub)},
  {TypeIsFloat, _6502_OP(ldf)},
  {TypeIsDouble, _6502_OP(ldd)},
  {TypeIsBool, _6502_OP(ldb)},
  {TypeIsPointerOrArray, _6502_OP(lda)},
  {TypeIsFunction, _6502_OP(lda)},
  {NULL, 0},
};

static TargetInstruction* LoadVariableValue(_6502Generator* g, IRNode* node,
                                            TargetInstruction* addr,
                                            TargetInstruction* offset) {
  _6502Opcode opcode = _6502_OP(ldw);
  for (size_t i = 0; load_opcodes[i].type_func != NULL; i++) {
    if (load_opcodes[i].type_func(node->type)) {
      opcode = load_opcodes[i].load;
      break;
    }
  }
  assert(opcode != 0);
  return Emit(g, NewInstruction2(opcode, addr, offset));
}

// TODO: allow override of tls model per variable.
static TargetInstruction* GetTlsVariableAddress(_6502Generator* g, IRNode* node) {
  switch (compiler->tls_model) {
    default:
      abort();
    case TLS(global_dynamic):
    case TLS(local_dynamic): {
      // These call __tls_get_addr to get the address of the a TLS variable
      // from the GOT.
      //
      // First load the address of the GOT entry, based on the symbol.
      TargetInstruction* addr =  Emit(g,
                                      NewInstruction1(_6502_OP(adr),
                                                      GetLoweredNode(node)));
      // Push onto stack.
      Emit(g, NewInstruction1(_6502_OP(pusha), addr));
      TargetInstruction* __tls_get_addr =
      GetSymbol(g, NULL, g->base.__tls_get_addr);
      TargetInstruction* result = Emit(g,
                                       NewInstruction1(_6502_OP(calla), __tls_get_addr));
      Emit(g,
           NewInstruction1(_6502_OP(incsp),
                           GetIntConstant(g, NULL, kTargetTypeAddress, 8)));
      return result;
    }
    case TLS(initial_exec): {
      // The address of the TLS variable is obtained from adding the offset
      // from the GOT to the thread pointer.
      TargetInstruction* addr =  Emit(g,
                                      NewInstruction1(_6502_OP(adrtls),
                                                      GetLoweredNode(node)));
      // Load the value as a 64-bit integer.
      TargetInstruction* zero = GetIntConstant(g, NULL, kTargetTypeExtended, 0);
      TargetInstruction* value = Emit(g, NewInstruction2(_6502_OP(lda), addr, zero));
      TargetInstruction* tp = ThreadPointer(g);
      return Emit(g, NewInstruction2(_6502_OP(addc), tp, value));
    }
    case TLS(local_exec): {
      // Address is constructed from thread pointer plus an offset
      // provided by the linker.
      TargetInstruction* addr = ThreadPointer(g);
      TargetInstruction* offset = Emit(g, NewInstruction1(_6502_OP(movac),
                                                              GetLoweredNode(node)));
      return Emit(g, NewInstruction2(_6502_OP(add), addr, offset));
    }
  }
}


static void GetTlsAddressAndOffset(_6502Generator* g, IRNode* addr_node,
                                   TargetInstruction** addr,
                                   TargetInstruction** offset) {
  switch (compiler->tls_model) {
    default:
      abort();
    case TLS(global_dynamic):
    case TLS(local_dynamic): {
      // These call __tls_get_addr to get the address of the a TLS variable
      // from the GOT.
      //
      // First load the address of the GOT entry, based on the symbol.
      TargetInstruction* tls_addr =  Emit(g,
                                          NewInstruction1(_6502_OP(adr),
                                                          GetLoweredNode(addr_node)));
      // Push onto stack.
      Emit(g, NewInstruction1(_6502_OP(pusha), tls_addr));
      TargetInstruction* __tls_get_addr = GetSymbol(g, NULL, g->base.__tls_get_addr);
      *addr = Emit(g, NewInstruction1(_6502_OP(calla), __tls_get_addr));
      Emit(g,
           NewInstruction1(_6502_OP(incsp),
                           GetIntConstant(g, NULL, kTargetTypeAddress, 8)));
      *offset = GetIntConstant(g, NULL, kTargetTypeAddress, 0);
      break;
    }
    case TLS(initial_exec): {
      // The address of the TLS variable is obtained from adding the offset
      // from the GOT to the thread pointer.
      TargetInstruction* tls_addr =  Emit(g,
                                          NewInstruction1(_6502_OP(adrtls),
                                                          GetLoweredNode(addr_node)));
      // Load the value as a 16-bit integer.
      *offset = GetIntConstant(g, NULL, kTargetTypeAddress, 0);
      tls_addr = Emit(g, NewInstruction2(_6502_OP(lda), tls_addr, *offset));
      *addr = Emit(g, NewInstruction2(_6502_OP(add), ThreadPointer(g), tls_addr));
      break;
    }
    case TLS(local_exec): {
      // Address is thread pointer plus an offset obtained from the
      // linker.
      TargetInstruction* tp_offset =
      Emit(g, NewInstruction1(_6502_OP(movac), GetLoweredNode(addr_node)));
      *addr = Emit(g, NewInstruction2(_6502_OP(add), ThreadPointer(g), tp_offset));
      *offset = GetIntConstant(g, NULL, kTargetTypeAddress, 0);
      break;
    }
  }
}


// Opcode for a comparison operation.  The type of the comparison
// is always bool, so we need to look at the type of the first
// input instead.
static _6502Opcode ComparisonOpcode(IRNode* node) {
  IROpcode op = node->opcode;
  IRNode* op1 = node->inputs.value.p[0];
  TypeRecord* type = op1->type;
  bool is_unsigned = type != NULL && TypeIsUnsigned(type);
  bool is_long = type != NULL && (TypeIsLong(type) || TypeIsLongLong(type));
  bool is_short = type != NULL && TypeIsShort(type);
  bool is_byte = type != NULL && (TypeIsChar(type) || TypeIsBool(type));
 
  switch (op) {
    case IR_OP(cmpeqi):
      if (is_long) {
        return _6502_OP(cmpeqx);
      } else if (is_short) {
        return _6502_OP(cmpeqa);
      } else if (is_byte) {
        return _6502_OP(cmpeqb);
      } else {
        return _6502_OP(cmpeqi);
      }
    case IR_OP(cmpnei):
      if (is_long) {
        return _6502_OP(cmpnex);
      } else if (is_short) {
        return _6502_OP(cmpnea);
      } else if (is_byte) {
        return _6502_OP(cmpneb);
      } else {
        return _6502_OP(cmpnei);
      }
    case IR_OP(cmplti):
      if (is_long) {
        return is_unsigned ? _6502_OP(cmpltux) : _6502_OP(cmpltx);
      } else if (is_short) {
        return is_unsigned ? _6502_OP(cmpltua) : _6502_OP(cmplta);
      } else if (is_byte) {
        return is_unsigned ? _6502_OP(cmpltub) : _6502_OP(cmpltb);
      } else {
        return is_unsigned ? _6502_OP(cmpltui) : _6502_OP(cmplti);
      }
    case IR_OP(cmplei):
      if (is_long) {
        return is_unsigned ? _6502_OP(cmpleux) : _6502_OP(cmplex);
      } else if (is_short) {
        return is_unsigned ? _6502_OP(cmpleua) : _6502_OP(cmplea);
      } else if (is_byte) {
        return is_unsigned ? _6502_OP(cmpleub) : _6502_OP(cmpleb);
      } else {
        return is_unsigned ? _6502_OP(cmpleui) : _6502_OP(cmplei);
      }
    case IR_OP(cmpgti):
      if (is_long) {
        return is_unsigned ? _6502_OP(cmpgtux) : _6502_OP(cmpgtx);
      } else if (is_short) {
        return is_unsigned ? _6502_OP(cmpgtua) : _6502_OP(cmpgta);
      } else if (is_byte) {
        return is_unsigned ? _6502_OP(cmpgtub) : _6502_OP(cmpgtb);
      } else {
        return is_unsigned ? _6502_OP(cmpgtui) : _6502_OP(cmpgti);
      }
    case IR_OP(cmpgei):
      if (is_long) {
        return is_unsigned ? _6502_OP(cmpgeux) : _6502_OP(cmpgex);
      } else if (is_short) {
        return is_unsigned ? _6502_OP(cmpgeua) : _6502_OP(cmpgea);
      } else if (is_byte) {
        return is_unsigned ? _6502_OP(cmpgeub) : _6502_OP(cmpgeb);
      } else {
        return is_unsigned ? _6502_OP(cmpgeui) : _6502_OP(cmpgei);
      }
      
    case IR_OP(cmpeqf):
      return _6502_OP(cmpeqf);
    case IR_OP(cmpnef):
      return _6502_OP(cmpnef);
    case IR_OP(cmpltf):
      return _6502_OP(cmpltf);
    case IR_OP(cmplef):
      return _6502_OP(cmplef);
    case IR_OP(cmpgtf):
      return _6502_OP(cmpgtf);
    case IR_OP(cmpgef):
      return _6502_OP(cmpgef);
      
    case IR_OP(cmpeqd):
      return _6502_OP(cmpeqd);
    case IR_OP(cmpned):
      return _6502_OP(cmpned);
    case IR_OP(cmpltd):
      return _6502_OP(cmpltd);
    case IR_OP(cmpled):
      return _6502_OP(cmpled);
    case IR_OP(cmpgtd):
      return _6502_OP(cmpgtd);
    case IR_OP(cmpged):
      return _6502_OP(cmpged);
      
    case IR_OP(cmpeqa):
      return _6502_OP(cmpeqa);
    case IR_OP(cmpnea):
      return _6502_OP(cmpnea);
    case IR_OP(cmplta):
      return is_unsigned ? _6502_OP(cmpltua) : _6502_OP(cmplta);
    case IR_OP(cmplea):
      return is_unsigned ? _6502_OP(cmpleua) : _6502_OP(cmplea);
    case IR_OP(cmpgta):
      return is_unsigned ? _6502_OP(cmpgtua) : _6502_OP(cmpgta);
    case IR_OP(cmpgea):
      return is_unsigned ? _6502_OP(cmpgeua) : _6502_OP(cmpgea);
      
    default:
      assert(false);
  }
}

static _6502Opcode IR2Opcode(IRNode* node) {
  IROpcode op = node->opcode;
  TypeRecord* type = node->type;
  bool is_unsigned = type != NULL && TypeIsUnsigned(type);
  bool is_long = type != NULL && (TypeIsLong(type) || TypeIsLongLong(type));
  bool is_short = type != NULL && TypeIsShort(type);
  switch (op) {
    case IR_OP(addi):
      return _6502_OP(add);
    case IR_OP(addf):
      return _6502_OP(addf);
    case IR_OP(addd):
      return _6502_OP(addd);
    case IR_OP(adda):
      return _6502_OP(add);
      
    case IR_OP(subi):
      return _6502_OP(sub);
    case IR_OP(subf):
      return _6502_OP(subf);
    case IR_OP(subd):
      return _6502_OP(subd);
    case IR_OP(suba):
      return _6502_OP(sub);
      
    case IR_OP(muli):
      return _6502_OP(mul);
    case IR_OP(mulf):
      return _6502_OP(mulf);
    case IR_OP(muld):
      return _6502_OP(muld);
      
    case IR_OP(divi):
      return is_unsigned ? _6502_OP(divu) : _6502_OP(div);
    case IR_OP(divf):
      return _6502_OP(divf);
    case IR_OP(divd):
      return _6502_OP(divd);
      
    case IR_OP(modi):
      return is_unsigned ? _6502_OP(modu) : _6502_OP(mod);
      
    case IR_OP(lsri):
      return _6502_OP(lsr);
    case IR_OP(asri):
      return _6502_OP(asr);
    case IR_OP(lsli):
      return _6502_OP(lsl);
      
    case IR_OP(ori):
      return _6502_OP(or);
    case IR_OP(andi):
      return _6502_OP(and);
    case IR_OP(xori):
      return _6502_OP(xor);
      
    case IR_OP(noti):
      return _6502_OP(not);
    case IR_OP(nota):
      return _6502_OP(not);
    case IR_OP(onescomp):
      return _6502_OP(inv);
    case IR_OP(negi):
      return _6502_OP(neg);
    case IR_OP(negf):
      return _6502_OP(negf);
    case IR_OP(negd):
      return _6502_OP(negd);

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
       return ComparisonOpcode(node);
      
    case IR_OP(i2f):
      return is_unsigned ? _6502_OP(ui2f) : _6502_OP(i2f);
    case IR_OP(i2d):
      return is_unsigned ? _6502_OP(ui2d) : _6502_OP(i2d);
    case IR_OP(f2d):
      return _6502_OP(f2d);
    case IR_OP(d2f):
      return _6502_OP(d2f);
    case IR_OP(f2i):
      return is_unsigned ? _6502_OP(f2ui) : _6502_OP(f2i);
    case IR_OP(d2i):
      return is_unsigned ? _6502_OP(d2ui) : _6502_OP(d2i);
      
    case IR_OP(movi):
      if (is_short) {
        return _6502_OP(mova);
      } else if (is_long) {
        return _6502_OP(movx);
      } else {
        return _6502_OP(mov);
      }
     case IR_OP(movf):
      return _6502_OP(movf);
    case IR_OP(movd):
      return _6502_OP(movd);
    case IR_OP(mova):
      return _6502_OP(mova);
    case IR_OP(rmovi):
      if (is_short) {
        return _6502_OP(rmova);
      } else if (is_long) {
        return _6502_OP(rmovx);
      } else {
        return _6502_OP(rmov);
      }
    case IR_OP(rmovf):
      return _6502_OP(rmovf);
    case IR_OP(rmovd):
      return _6502_OP(rmovd);
    case IR_OP(rmova):
      return _6502_OP(rmova);
    case IR_OP(tmp):
      return _6502_OP(tmp);
    default:
      assert(false);
  }
}

static void ApplyFixups(_6502Generator* g, IRNode* label_node) {
  TargetApplyFixups(&g->base, label_node);
}

// Do some strength reduction if we can.  Returns NULL or new instruciton
static TargetInstruction* ReduceExpressionStrength(_6502Generator* g,
                                                   IRNode* node, _6502Opcode opcode) {
  TargetInstruction* inst = NULL;
  switch (opcode) {
    default:
      // All others are handled below.
      break;
    case _6502_OP(add): {
      // We have an add with constant instruction.  Use it if we can.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      // Adds are commutative so we can have a const as first or
      // second operand.
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        int64_t lhs = ((IRConstant*)op1)->value.ivalue;
        int64_t rhs = ((IRConstant*)op2)->value.ivalue;
        inst = (TargetInstruction*)NewInstruction(_6502_OP(movc));
        inst->operand[0] =
        GetIntConstant(g, NULL, kTargetTypeWord, lhs + rhs);
        break;
      }
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Adding zero is a move instruction.
          inst = (TargetInstruction*)NewInstruction(_6502_OP(mov));
          inst->operand[0] = GetLoweredNode(op1);
        } else {
          inst = (TargetInstruction*)NewInstruction(_6502_OP(addc));
          inst->operand[0] = GetLoweredNode(op1);
          inst->operand[1] = GetLoweredNode(op2);
        }
      } else if (IRIsConst(op1)) {
        int64_t c = ((IRConstant*)op1)->value.ivalue;
        if (c == 0) {
          // Adding zero is a move instruction.
          inst = (TargetInstruction*)NewInstruction(_6502_OP(mov));
          inst->operand[0] = GetLoweredNode(op1);
        } else {
          inst = (TargetInstruction*)NewInstruction(_6502_OP(addc));
          inst->operand[0] = GetLoweredNode(op2);
          inst->operand[1] = GetLoweredNode(op1);
        }
      }
      break;
    }
      
    case _6502_OP(sub): {
      // A sub with a constant can be converted to an addc with a negative
      // constant.
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        int64_t lhs = ((IRConstant*)op1)->value.ivalue;
        int64_t rhs = ((IRConstant*)op2)->value.ivalue;
        inst = (TargetInstruction*)NewInstruction(_6502_OP(movc));
        inst->operand[0] =
        GetIntConstant(g, NULL, kTargetTypeWord, lhs - rhs);
        break;
      }
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 0) {
          // Subtract zero is mov
          inst = (TargetInstruction*)NewInstruction(_6502_OP(mov));
          inst->operand[0] = GetLoweredNode(op1);
        } else {
          // Add the negative of the constant.
          inst = (TargetInstruction*)NewInstruction(_6502_OP(addc));
          inst->operand[0] = GetLoweredNode(op1);
          inst->operand[1] = GetIntConstant(g, NULL, kTargetTypeWord, -c);
        }
      }
      break;
    }
      
    case _6502_OP(mul): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) || IRIsConst(op2)) {
        if (IRIsConst(op1) && IRIsConst(op2)) {
          // Both constants, fold.
          int64_t lhs = ((IRConstant*)op1)->value.ivalue;
          int64_t rhs = ((IRConstant*)op2)->value.ivalue;
          inst = (TargetInstruction*)NewInstruction(_6502_OP(movc));
          inst->operand[0] =
          GetIntConstant(g, NULL, kTargetTypeWord, lhs * rhs);
        } else {
          if (IRIsConst(op1)) {
            IRNode* tmp = op1;
            op1 = op2;
            op2 = tmp;
          }
          int64_t c = ((IRConstant*)op2)->value.ivalue;
          if (c == 0) {
            // Multiply by zero is zero.
            inst = GetIntConstant(g, NULL, kTargetTypeWord, 0);
          } else if (c == 1) {
            // Multiply by 1 is mov.
            inst = (TargetInstruction*)NewInstruction(_6502_OP(mov));
            inst->operand[0] = GetLoweredNode(op1);
          }
        }
      }
      break;
    }
    case _6502_OP(div): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        int64_t lhs = ((IRConstant*)op1)->value.ivalue;
        int64_t rhs = ((IRConstant*)op2)->value.ivalue;
        if (rhs != 0) {
          // Don't divide by zero.
          inst = (TargetInstruction*)NewInstruction(_6502_OP(movc));
          inst->operand[0] =
          GetIntConstant(g, NULL, kTargetTypeWord, lhs / rhs);
          break;
        }
      }
      if (IRIsConst(op2)) {
        int64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 1) {
          // Division by 1 is a mov
          inst = (TargetInstruction*)NewInstruction(_6502_OP(mov));
          inst->operand[0] = GetLoweredNode(op1);
        }
      }
      break;
    }
    case _6502_OP(divu): {
      assert(node->inputs.length == 2);
      IRNode* op1 = node->inputs.value.p[0];
      IRNode* op2 = node->inputs.value.p[1];
      if (IRIsConst(op1) && IRIsConst(op2)) {
        // Both constants, fold.
        uint64_t lhs = ((IRConstant*)op1)->value.ivalue;
        uint64_t rhs = ((IRConstant*)op2)->value.ivalue;
        if (rhs != 0) {
          // Don't divide by zero.
          inst = (TargetInstruction*)NewInstruction(_6502_OP(movc));
          inst->operand[0] =
          GetIntConstant(g, NULL, kTargetTypeWord, lhs / rhs);
          break;
        }
      }
      if (IRIsConst(op2)) {
        uint64_t c = ((IRConstant*)op2)->value.ivalue;
        if (c == 1) {
          // Division by 1 is a mov
          inst = (TargetInstruction*)NewInstruction(_6502_OP(mov));
          inst->operand[0] = GetLoweredNode(op1);
        }
      }
      break;
    }
  }
  return inst;
}


static TargetInstruction* LowerExpression(_6502Generator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  _6502Opcode opcode = IR2Opcode(node);
  assert(node->inputs.length <= 2);
  TargetInstruction* inst = ReduceExpressionStrength(g, node, opcode);
  
  if (inst == NULL) {
    inst = (TargetInstruction*)NewInstruction(opcode);
    for (size_t i = 0; i < node->inputs.length; i++) {
      IRNode* input = node->inputs.value.p[i];
      inst->operand[i] = GetLoweredNode(input);
    }
  }
  TargetUpdateRefCount(inst);
  SetLoweredNode(node, inst);
  return Emit(g, inst);
}

static TargetInstruction* LowerConditionalBranch(_6502Generator* g,
                                                 IRNode* node) {
  _6502Opcode opcode;
  switch (node->opcode) {
    case IR_OP(btrue):
      opcode = _6502_OP(bt);
      break;
    case IR_OP(bfalse):
      opcode = _6502_OP(bf);
      break;
    default:
      assert(false);
  }
  assert(node->inputs.length == 2);
  IRNode* expr = node->inputs.value.p[0];
  IRNode* target_node = node->inputs.value.p[1];
  
  TargetInstruction* inst = Emit(g, NewInstruction1(opcode,
                                                    GetLoweredNode(expr)));
  
  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&g->base.fixups, NewBranchFixup(inst, target_node, 1));
  } else {
    inst->operand[1] = target;
  }
  return inst;
}

static TargetInstruction* LowerBranch(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  IRNode* target_node = node->inputs.value.p[0];
  
  TargetInstruction* inst = Emit(g, NewInstruction(_6502_OP(jmp)));
  
  TargetInstruction* target = target_node->data.ptr;
  if (target == NULL) {
    // Forward branch, add fixup for target label.
    VectorAppend(&g->base.fixups, NewBranchFixup(inst, target_node, 0));
  } else {
    inst->operand[0] = target;
  }
  return inst;
}

static TargetInstruction* LowerComputedBranch(_6502Generator* g,
                                              IRNode* node) {
  assert(node->inputs.length == 1);
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);
  TargetInstruction* cbra = Emit(g, NewInstruction1(_6502_OP(cbra), value));
  SetLoweredNode(node, cbra);
  return cbra;
}

static TargetInstruction* LowerLabel(_6502Generator* g, IRNode* label) {
  TargetInstruction* inst = Emit(g, NewInstruction(_6502_OP(label)));
  label->data.ptr = inst;
  ApplyFixups(g, label);
  return inst;
}

static TargetInstruction* LowerVariable(_6502Generator* g, IRNode* node) {
  int32_t var_offset = 0;
  TargetInstruction* inst = NULL;
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(tempvar):
      var_offset = node->data.ivalue;
      inst = Emit(g, NewInstruction1(_6502_OP(localvar),
                                     GetIntConstant(g, node, kTargetTypeAddress,
                                                    var_offset)));
      break;
      
    case IR_OP(argument):
      var_offset = node->data.ivalue;
      inst = Emit(g, NewInstruction1(_6502_OP(argument),
                                     GetIntConstant(g, node, kTargetTypeAddress,
                                                    var_offset)));
      break;
      
    case IR_OP(staticvar):
    case IR_OP(externvar): {
      IRVariable* var = (IRVariable*)node;

      inst = GetSymbol(g, node, var->symbol);
      break;
    }
    case IR_OP(structreturn):
      inst = Emit(g, NewInstruction(_6502_OP(structreturn)));
      break;
    default:
      abort();
  }
  SetLoweredNode(node, inst);
  return inst;
}

// If there is only one user of this load we can eliminate it provided
// the  address comes from a variable and the offset is zero.
static TargetInstruction* DeferredLoad(IRNode* node) {
  if (node->outputs.length == 1) {
    return GetLoweredNode(node->inputs.value.p[0]);
  }
  return NULL;
}

static TargetInstruction* LowerLoad(_6502Generator* g, IRNode* node) {
  _6502Opcode opcode;
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];
  
  TargetInstruction* deferred_load = DeferredLoad(node);
  if (deferred_load != NULL) {
    SetLoweredNode(node, deferred_load);
    return deferred_load;
  }
  
  TargetInstruction* addr = GetLoweredNode(addr_node);
  
  switch (node->opcode) {
    case IR_OP(loadi):
      opcode = _6502_OP(ldw);
      break;
    case IR_OP(loadb):
      opcode = _6502_OP(ldb);
      break;
    case IR_OP(loadl):
      opcode = _6502_OP(ldx);
      break;
    case IR_OP(loads):
      opcode = _6502_OP(ldh);
      break;
    case IR_OP(loadui):
      opcode = _6502_OP(lduw);
      break;
    case IR_OP(loadub):
      opcode = _6502_OP(ldub);
      break;
    case IR_OP(loadus):
      opcode = _6502_OP(lduh);
      break;
    case IR_OP(loadf):
      opcode = _6502_OP(ldf);
      break;
    case IR_OP(loadd):
      opcode = _6502_OP(ldd);
      break;
    case IR_OP(loada):
      opcode = _6502_OP(lda);
      break;
    default:
      assert(false);
  }
  
  TargetInstruction* result =
    Emit(g, NewInstruction1(opcode, addr));
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerStore(_6502Generator* g, IRNode* node) {
  _6502Opcode opcode;
  assert(node->inputs.length == 2);
  
  // Address to store to is the first operand of the store IR node.
  // The value to store is the second operand.
  IRNode* addr_node = node->inputs.value.p[0];
  IRNode* src_node = node->inputs.value.p[1];
  
  // Get the src in a register.
  TargetInstruction* src = GetLoweredNode(src_node);
  TargetInstruction* addr = GetLoweredNode(addr_node);
  
  switch (node->opcode) {
    case IR_OP(storei):
      opcode = _6502_OP(stw);
      break;
    case IR_OP(storeb):
      opcode = _6502_OP(stb);
      break;
    case IR_OP(storel):
      opcode = _6502_OP(stx);
      break;
    case IR_OP(stores):
      opcode = _6502_OP(sth);
      break;
    case IR_OP(storef):
      opcode = _6502_OP(stf);
      break;
    case IR_OP(stored):
      opcode = _6502_OP(std);
      break;
    case IR_OP(storea):
      opcode = _6502_OP(stx);
      break;
    default:
      assert(false);
  }
  
  // NOTE: the first operand of the st instructions is the source register.
  TargetInstruction* result =
  Emit(g, NewInstruction2(opcode, src, addr));
  SetLoweredNode(node, result);
  return result;
}

static struct {
  bool (*type_func)(TypeRecord*);
  _6502Opcode opcode;
  size_t size;
} push_map[] = {
  {TypeIsInt, _6502_OP(pushi), 4},
  {TypeIsShort, _6502_OP(pushh), 2},
  {TypeIsChar, _6502_OP(pushb), 1},
  {TypeIsLong, _6502_OP(pushx), 8},
  {TypeIsLongLong, _6502_OP(pushx), 8},
  {TypeIsFloat, _6502_OP(pushf), 4},
  {TypeIsDouble, _6502_OP(pushd), 8},
  {TypeIsLongDouble, _6502_OP(pushd), 8},
  {TypeIsPointerOrArray, _6502_OP(pusha), 2},
  {TypeIsFunction, _6502_OP(pusha), 2},
  {TypeIsStructOrUnion, _6502_OP(pusha), 2},
  {NULL, _6502_OP(pusha), 0},
};

static void PushArg(_6502Generator* g, IRNode* node,
                    TargetInstruction* inst, size_t* size) {
  if (node->type == NULL) {
    Emit(g, NewInstruction1(_6502_OP(pusha), inst));
  }
  for (size_t i = 0; push_map[i].type_func != NULL; i++) {
    if (push_map[i].type_func(node->type)) {
      if (size != NULL) {
        *size += push_map[i].size;
      }
      Emit(g, NewInstruction1(push_map[i].opcode, inst));
      return;
    }
  }
  assert(false);
}

// Passing a struct or union to a function needs to copy
// the memory from the address to the stack.
static void PushStructArg(_6502Generator* g, IRNode* node, size_t *args_size) {
  size_t struct_size = node->type->size;
  *args_size += struct_size;
  
  // First make space on the stack.
  TargetInstruction* size =
  GetIntConstant(g, NULL, kTargetTypeAddress, struct_size);
  Emit(g, NewInstruction1(_6502_OP(decsp), size));
  
  TargetInstruction* dest_addr =
  Emit(g, NewInstruction1(_6502_OP(mov), StackPointer(g)));
  
  // Push size for memcpy.
  TargetInstruction* size_mov =
  Emit(g, NewInstruction1(_6502_OP(movc), size));
  Emit(g, NewInstruction1(_6502_OP(pushh), size_mov));
  
  // Push source address.
  // We have an address and register for the address, add them together.
  TargetInstruction* src_addr = Emit(g, NewInstruction1(_6502_OP(addc), GetLoweredNode(node)));
  Emit(g, NewInstruction1(_6502_OP(pusha), src_addr));
  
  // Push dest address.
  Emit(g, NewInstruction1(_6502_OP(pusha), dest_addr));
  
  // Call memcpy.
  TargetInstruction* memcpy = GetSymbol(g, NULL, g->base.memcpy);
  Emit(g, NewInstruction1(_6502_OP(calla), memcpy));
  Emit(g,
       NewInstruction1(_6502_OP(incsp),
                       GetIntConstant(g, NULL, kTargetTypeAddress, 6)));
  
}

static TargetInstruction* LowerCall(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length >= 1);
  size_t args_size = 0;
  for (size_t i = node->inputs.length - 1; i >= 1; i--) {
    IRNode* arg_node = node->inputs.value.p[i];
    if (TypeIsStructOrUnion(arg_node->type)) {
      PushStructArg(g, arg_node, &args_size);
    } else {
      TargetInstruction* arg = GetLoweredNode(arg_node);
      PushArg(g, arg_node, arg, &args_size);
    }
  }
  TargetInstruction* addr = GetLoweredNode(node->inputs.value.p[0]);
  _6502Opcode opcode;
  if (addr->opcode == _6502_OP(symbol)) {
    if (TypeIsFloat(node->type)) {
      opcode = _6502_OP(callf);
    } else if (TypeIsDouble(node->type)) {
      opcode = _6502_OP(calld);
    } else if (TypeIsLong(node->type) || TypeIsLongLong(node->type)){
      opcode = _6502_OP(callx);
    } else if (TypeIsInt(node->type)) {
      opcode = _6502_OP(calli);
    } else {
      opcode = _6502_OP(calla);
    }
  } else {
    if (TypeIsFloat(node->type)) {
      opcode = _6502_OP(rcallf);
    } else if (TypeIsDouble(node->type)) {
      opcode = _6502_OP(rcalld);
    } else if (TypeIsLong(node->type) || TypeIsLongLong(node->type)){
      opcode = _6502_OP(rcallx);
    } else if (TypeIsInt(node->type)) {
      opcode = _6502_OP(rcalli);
    } else {
      opcode = _6502_OP(rcalla);
    }
  }
  TargetInstruction* call = Emit(g, NewInstruction1(opcode, addr));
  if (args_size > 0) {
    Emit(g,
         NewInstruction1(_6502_OP(incsp), GetIntConstant(g, NULL, kTargetTypeAddress,
                                                     args_size)));
  }
  SetLoweredNode(node, call);
  return call;
}

static TargetInstruction* LowerResult(_6502Generator* g, IRNode* node) {
  assert(node->inputs.length == 1);
  _6502Opcode result_reg_opcode, opcode;
  switch (node->opcode) {
    case IR_OP(resulti):
    case IR_OP(resulta):
      result_reg_opcode = _6502_OP(resultx);
      opcode = _6502_OP(rmov);
      break;
    case IR_OP(resultf):
      result_reg_opcode = _6502_OP(resultf);
      opcode = _6502_OP(rmovf);
      break;
    case IR_OP(resultd):
      result_reg_opcode = _6502_OP(resultd);
      opcode = _6502_OP(rmovd);
      break;
    default:
      assert(false);
  }
  TargetInstruction* result = GetLoweredNode(node->inputs.value.p[0]);
  TargetInstruction* result_reg =
  Emit(g, NewInstruction(result_reg_opcode));
  return Emit(g, NewInstruction2(opcode, result_reg, result));
}

// A literal reference is a move of the literal offset (the first input
// to the literalref node) to the 'literal' with the given id.  This will
// be assembled as a reference to a symbol with the name .str.%d.
static TargetInstruction* LowerLiteralReference(_6502Generator* g,
                                                IRNode* node) {
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
  Emit(g, TargetNewLiteral((int)id_node->value.ivalue));
  
  TargetInstruction* result =
  Emit(g, NewInstruction1(_6502_OP(adrs), literal));
  
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerStructReference(_6502Generator* g,
                                               IRNode* node) {
  return SetLoweredNode(node, GetLoweredNode(node->inputs.value.p[0]));
}

static TargetInstruction* LowerMemcpy(_6502Generator* g, IRNode* node) {
  // The memcpy IR node's inputs are the same as those for the memcpy
  // function.  However, there are no load nodes for the desination
  // or source addresses.
  assert(node->inputs.length == 3);
  
  // First push the constant for the length.
  TargetInstruction* length =
  Emit(g, GetLoweredNode(node->inputs.value.p[2]));
  Emit(g, NewInstruction1(_6502_OP(pusha), length));
  
  // Now push src.
  IRNode* src_node = node->inputs.value.p[1];
  TargetInstruction* src_addr = GetLoweredNode(src_node);
  
  // We have an address and register for the address, add them together.
  src_addr = Emit(g, NewInstruction1(_6502_OP(addc), src_addr));
  src_node->data.ptr = src_addr;
  PushArg(g, src_node, src_addr, NULL);
  
  // And now push dest.
  IRNode* dest_node = node->inputs.value.p[0];
  TargetInstruction* dest_addr = GetLoweredNode(dest_node);
  
  // We have an address and register for the address, add them together.
  dest_node->data.ptr = dest_addr;
  
  PushArg(g, dest_node, dest_addr, NULL);
  
  TargetInstruction* memcpy = GetSymbol(g, NULL, g->base.memcpy);
  TargetInstruction* call = Emit(g, NewInstruction1(_6502_OP(calla), memcpy));
  Emit(g,
       NewInstruction1(_6502_OP(incsp),
                       GetIntConstant(g, NULL, kTargetTypeAddress, 6)));
  SetLoweredNode(node, call);
  
  return call;
}

static TargetInstruction* LowerMemzero(_6502Generator* g, IRNode* node) {
  // The memzero IR node has one input: the variable to zero.  We
  // emit this is as a call to memset using the size of the symbol.
  assert(node->inputs.length == 1);
  IRNode* addr_node = node->inputs.value.p[0];
  IRVariable* var = (IRVariable*)addr_node;
  
  // Third parameter to memset is the length.
  TargetInstruction* size = Emit(
                                 g,
                                 NewInstruction1(_6502_OP(movc), GetIntConstant(g, NULL, kTargetTypeAddress,
                                                                            var->symbol->type->size)));
  Emit(g, NewInstruction1(_6502_OP(pusha), size));
  
  // Push a zero constant as the second argument.
  TargetInstruction* zero = Emit(
                                 g, NewInstruction1(_6502_OP(movc),
                                                        GetIntConstant(g, NULL, kTargetTypeAddress, 0)));
  Emit(g, NewInstruction1(_6502_OP(pushh), zero));
  
  TargetInstruction* addr = GetLoweredNode(addr_node);
  
  addr_node->data.ptr = addr;
  
  PushArg(g, addr_node, addr, NULL);
  
  TargetInstruction* memset = GetSymbol(g, NULL, g->base.memset);
  TargetInstruction* call = Emit(g, NewInstruction1(_6502_OP(calla), memset));
  Emit(g,
       NewInstruction1(_6502_OP(incsp),
                       GetIntConstant(g, NULL, kTargetTypeAddress, 6)));
  SetLoweredNode(node, call);
  return call;
}

static TargetInstruction* LowerMask(_6502Generator* g, IRNode* node) {
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);
  value =
  Emit(g, NewInstruction2(_6502_OP(and), value,
                              GetLoweredNode(node->inputs.value.p[1])));
  SetLoweredNode(node, value);
  return value;
}

static TargetInstruction* LowerSignExtend(_6502Generator* g, IRNode* node) {
  // If the source instruction is a signed load then there is no need to
  // perform the sign extension since the load instructions already do
  // that.
  TargetInstruction* value = GetLoweredNode(node->inputs.value.p[0]);
  if (_6502IsSignedLoad((_6502Opcode)value->opcode)) {
    return SetLoweredNode(node, value);
  }
  IRConstant* diff_value = node->inputs.value.p[1];
  int64_t diff = diff_value->value.ivalue;
  TargetInstruction* diff_inst =
  Emit(g,
       NewInstruction1(_6502_OP(movc),
                       GetIntConstant(g, NULL, kTargetTypeAddress, diff)));
  TargetInstruction* lsl =
  Emit(g, NewInstruction2(_6502_OP(lsl), value, diff_inst));
  TargetInstruction* asr =
  Emit(g, NewInstruction2(_6502_OP(asr), lsl, diff_inst));
  
  SetLoweredNode(node, asr);
  return asr;
}

static TargetInstruction* LowerAsm(_6502Generator* g, IRNode* node) {
  // The first argument is a literal containing the assembly language.
  IRConstant* id_node = node->inputs.value.p[0];
  TargetInstruction* literal =
  Emit(g, TargetNewLiteral((int)id_node->value.ivalue));
  
  TargetInstruction* result = Emit(g, NewInstruction1(_6502_OP(asm), literal));
  
  SetLoweredNode(node, result);
  return result;
}

static TargetInstruction* LowerLocation(_6502Generator* g, IRNode* node) {
  IRLocation* loc = (IRLocation*)node;
  return SetLoweredNode(node, Emit(g, TargetNewLocation(loc)));
}

// The first input is the address of the 'ap' variable.  The second is the
// address of the last function argument.  The ap variable is set to the
// address of the last argument + 2.
static TargetInstruction* LowerBuiltinVaStart(_6502Generator* g,
                                              IRNode* node) {
  TargetInstruction* arg = GetLoweredNode(node->inputs.value.p[1]);
  TargetInstruction* add = Emit(g, NewInstruction2(_6502_OP(addc), arg,
                                                       GetIntConstant(g, NULL, kTargetTypeAddress, 2)));
  TargetInstruction* store =
  Emit(g, NewInstruction2(_6502_OP(sta), add, GetLoweredNode(node->inputs.value.p[0])));
  return SetLoweredNode(node, store);
}

static TargetInstruction* LowerBuiltinVaArg(_6502Generator* g,
                                            IRNode* node) {
  TargetInstruction* ap_load =
  Emit(g, NewInstruction1(_6502_OP(lda), GetLoweredNode(node->inputs.value.p[0])));
  
  _6502Opcode load_opcode = _6502_OP(lda);
  switch (node->type->size) {
    case 1:
      load_opcode = TypeIsUnsigned(node->type) ? _6502_OP(ldub) : _6502_OP(ldb);
      break;
    case 2:
      load_opcode = TypeIsUnsigned(node->type) ? _6502_OP(lduh) : _6502_OP(ldh);
      break;
    case 4:
      load_opcode = TypeIsUnsigned(node->type) ? _6502_OP(lduw) : _6502_OP(ldw);
      break;
    case 8:
      load_opcode =_6502_OP(ldx);
      break;
  }
  
  TargetInstruction* result = Emit(
                                   g, NewInstruction2(load_opcode, ap_load,
                                                          GetIntConstant(g, NULL, kTargetTypeAddress, 0)));
  
  TargetInstruction* size = GetLoweredNode(node->inputs.value.p[1]);
  TargetInstruction* addc =
  Emit(g, NewInstruction2(_6502_OP(addc), ap_load, size));
  Emit(g, NewInstruction2(_6502_OP(sta), addc, GetLoweredNode(node->inputs.value.p[0])));
  return SetLoweredNode(node, result);
}

static TargetInstruction* LowerBuiltinVaEnd(_6502Generator* g,
                                            IRNode* node) {
  // Nothing to do for va_end.
  return NULL;
}

static TargetInstruction* LowerBuiltinVaCopy(_6502Generator* g,
                                             IRNode* node) {
  return NULL;  // TODO
}

static TargetInstruction* LowerComparison(_6502Generator* g,
                                             IRNode* node) {
  // Check if any the outputs of the node are not branches.
  bool is_expression = false;
  for (size_t i = 0; i < node->outputs.length; i++) {
    IRNode* output = node->outputs.value.p[i];
    if (!IRIsConditionalBranch(output)) {
      is_expression = true;
      break;
    }
  }
  TargetInstruction* inst = LowerExpression(g, node);
  if (is_expression) {
    inst->flags |= _6502_CMP_EXPR;
  }
  return inst;
}

static TargetInstruction* LowerIRNode(_6502Generator* g, IRNode* node) {
  // If we have already lowered the IR node, return it.
  if (node->data.ptr != NULL) {
    return node->data.ptr;
  }
  switch (node->opcode) {
    case IR_OP(localvar):
    case IR_OP(argument):
    case IR_OP(tempvar):
    case IR_OP(staticvar):
    case IR_OP(externvar):
      break;
      
    case IR_OP(structreturn):
      return Emit(g, NewInstruction(_6502_OP(structreturn)));
      
    case IR_OP(nop):
    case last_ir_opcode:
      return NULL;
      
    case IR_OP(ssavar):
    case IR_OP(phi):
      // We should never see these as we've moved out of SSA form
      // before here.
      break;
      
    case IR_OP(literalref):
      return LowerLiteralReference(g, node);
      
    case IR_OP(structref):
      return LowerStructReference(g, node);
      
    case IR_OP(consti):
      return GetIntConstant(g, node, kTargetTypeWord,
                            ((IRConstant*)node)->value.ivalue);
    case IR_OP(constb):
      return GetIntConstant(g, node, kTargetTypeByte,
                            ((IRConstant*)node)->value.ivalue);
      
    case IR_OP(consts):
      return GetIntConstant(g, node, kTargetTypeHalf,
                            ((IRConstant*)node)->value.ivalue);
      
    case IR_OP(consta):
      return GetIntConstant(g, node, kTargetTypeAddress,
                            ((IRConstant*)node)->value.ivalue);
      
    case IR_OP(constl):
      return GetIntConstant(g, node, kTargetTypeExtended,
                            ((IRConstant*)node)->value.ivalue);
      
    case IR_OP(constf):
      return GetFloatingPointConstant(g, node, kTargetTypeFloat,
                                      ((IRConstant*)node)->value.fvalue);
      
    case IR_OP(constd):
      return GetFloatingPointConstant(g, node, kTargetTypeDouble,
                                      ((IRConstant*)node)->value.fvalue);
      
    case IR_OP(enter):
      // Entry sequence:
      // enter
      // This will generate a JSR to the __enter
      // followed by two 16 bit words:
      // .hword mask
      // .hword frame_size
      //
      // The mask is a bitmask for the registers to save.
      return Emit(g, NewInstruction1(_6502_OP(enter),
                                GetIntConstant(g, NULL, kTargetTypeAddress,
                                                g->base.stack_frame_size)));
    case IR_OP(leave):
      // Exit sequence:
      // rts
      // This will generate a JSR to the __ret
      // followed by two 16 bit words:
      // .hword mask
      // .hword frame_size
      // The register mask specifies the registers to restore.
      return Emit(g, NewInstruction1(_6502_OP(rts),
                                     GetIntConstant(g, NULL, kTargetTypeAddress,
                                                    g->base.stack_frame_size)));
     
      
    case IR_OP(ret):
      return Emit(g, NewInstruction(_6502_OP(ret)));
      
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
      return LowerLoad(g, node);
      
      // stores.
    case IR_OP(storei):
    case IR_OP(storeb):
    case IR_OP(stores):
    case IR_OP(storel):
    case IR_OP(storef):
    case IR_OP(stored):
    case IR_OP(storea):
      return LowerStore(g, node);
      
    case IR_OP(addi):
    case IR_OP(addf):
    case IR_OP(addd):
    case IR_OP(adda):
      
    case IR_OP(subi):
    case IR_OP(subf):
    case IR_OP(subd):
    case IR_OP(suba):
      
    case IR_OP(muli):
    case IR_OP(mulf):
    case IR_OP(muld):
      
    case IR_OP(divi):
    case IR_OP(divf):
    case IR_OP(divd):
      
    case IR_OP(modi):
      
    case IR_OP(lsri):
    case IR_OP(asri):
    case IR_OP(lsli):
      
    case IR_OP(ori):
    case IR_OP(andi):
    case IR_OP(xori):
      
    case IR_OP(noti):
    case IR_OP(nota):
    case IR_OP(onescomp):
    case IR_OP(negi):
    case IR_OP(negf):
    case IR_OP(negd):

      
    case IR_OP(i2f):
    case IR_OP(i2d):
    case IR_OP(f2d):
    case IR_OP(d2f):
    case IR_OP(f2i):
    case IR_OP(d2i):
      
    case IR_OP(movi):
    case IR_OP(movf):
    case IR_OP(movd):
    case IR_OP(mova):
    case IR_OP(rmovi):
    case IR_OP(rmovf):
    case IR_OP(rmovd):
    case IR_OP(rmova):
    case IR_OP(tmp):
      return LowerExpression(g, node);
      
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
      return LowerComparison(g, node);
      
    case IR_OP(btrue):
    case IR_OP(bfalse):
      return LowerConditionalBranch(g, node);
      
    case IR_OP(bra):
      return LowerBranch(g, node);
      
    case IR_OP(cbra):
      return LowerComputedBranch(g, node);
      
    case IR_OP(label):
      return LowerLabel(g, node);
      
    case IR_OP(calla):
      return LowerCall(g, node);
      
    case IR_OP(resulti):
    case IR_OP(resultf):
    case IR_OP(resultd):
    case IR_OP(resulta):
      return LowerResult(g, node);
      
    case IR_OP(memzero):
      return LowerMemzero(g, node);
      
    case IR_OP(memcpy):
      return LowerMemcpy(g, node);
      
    case IR_OP(maski):
      return LowerMask(g, node);
      
    case IR_OP(signextendi):
      return LowerSignExtend(g, node);
      
    case IR_OP(asm):
      return LowerAsm(g, node);
      
    case IR_OP(loc):
      return LowerLocation(g, node);
      
    case IR_OP(builtin_va_start):
      return LowerBuiltinVaStart(g, node);
      
    case IR_OP(builtin_va_arg):
      return LowerBuiltinVaArg(g, node);
      
    case IR_OP(builtin_va_end):
      return LowerBuiltinVaEnd(g, node);
      
    case IR_OP(builtin_va_copy):
      return LowerBuiltinVaCopy(g, node);
  }
  // If we get here we've failed to handle the IR node.
  assert(false);
}

// Calculate the size of an argument based on its type.
static int64_t CalculateArgumentSize(Symbol* arg) {
  if (TypeIsFloatingPoint(arg->type)) {
    if (TypeIsDouble(arg->type)) {
      return 8;
    }
    return 4;
  }
  if (TypeIsPointerOrArray(arg->type)) {
    return 8;
  }
  if (TypeIsStructOrUnion(arg->type)) {
    return arg->type->info.struct_info->size;
  }
  return arg->type->size < 4 ? 4 : arg->type->size;
}

void _6502Lower(_6502Generator* g, Generator* gen) {
  // Variables are allocated below the frame, arguments are above.
  int32_t var_offset = 0;
  int32_t arg_offset = 16;  // Leave room for return address and saved ap.
  
  // Calculate the offset on the stack for all the arguments.  Store this offset
  // in the stack_offset field of the Symbol.
  Vector* prototype = &compiler->current_function->info.function.prototype;
  for (size_t i = 0; i < prototype->length; i++) {
    Symbol* arg = prototype->value.p[i];
    arg->stack_offset = arg_offset;
    int64_t size = CalculateArgumentSize(arg);
    arg_offset += size;
  }
  
  for (size_t i = 0; i < gen->variable_pool.length; i++) {
    PoolEntry* entry = (PoolEntry*)gen->variable_pool.value.p[i];
    TargetInstruction* inst;
    if (entry->pooled->opcode == IR_OP(localvar) ||
        entry->pooled->opcode == IR_OP(tempvar)) {
      int32_t size = entry->value.symbol->type->size;
      var_offset = (var_offset + (size - 1)) & ~(size - 1);   // Align.
      IRVariable* var = (IRVariable*)entry->pooled;
      inst = NewTargetSymbol(var->symbol);
      inst->operand[0] = GetIntConstant(g, entry->pooled,
                                        kTargetTypeAddress,
                                        var_offset);
      inst->opcode = (TargetOpcode)_6502_OP(localvar);
      inst = Emit(g, inst);
      entry->pooled->data.ptr = inst;
      var_offset += size;
    } else if (entry->pooled->opcode == IR_OP(argument)) {
      IRVariable* var = (IRVariable*)entry->pooled;
      inst = NewTargetSymbol(var->symbol);
      inst->operand[0] = GetIntConstant(g, entry->pooled,
                                        kTargetTypeAddress,
                                        var_offset);
      inst->opcode = (TargetOpcode)_6502_OP(argument);
      inst = Emit(g, inst);
      entry->pooled->data.ptr = inst;
    } else if (entry->pooled->opcode == IR_OP(staticvar) ||
               entry->pooled->opcode == IR_OP(externvar)) {
      // Static variables are referenced by a symbol instruction.
      IRVariable* var = (IRVariable*)entry->pooled;
      TargetInstruction* inst = GetSymbol(g, NULL, var->symbol);
      entry->pooled->data.ptr = inst;
    }
  }
  
  // We now know the stack frame size.
  g->base.stack_frame_size = (int32_t)var_offset;
  
  IRNode* node = GeneratorFirstInstruction(gen);
  while (node != NULL) {
    LowerIRNode(g, node);
    node = IRNext(node);
  }
  
  //_6502Optimize(g);
  _6502Print(g);

  // Allocate registers to the instructions.
  _6502AllocateRegisters(&g->register_allocator);
}

void _6502PrintInstruction(TargetInstruction* inst, FILE* fp) {
  TargetPrintInstruction(inst, _6502OpcodeName, fp);
}

void _6502Print(_6502Generator* g) {
  TargetInstruction* inst = TargetFirstInstruction(&g->base);
  while (inst != NULL) {
    TargetPrintInstruction(inst, _6502OpcodeName, stdout);
    inst = TargetNext(inst);
  }
}

bool _6502IsExpression(_6502Opcode opcode) {
  switch (opcode) {
    case _6502_OP(save):
    case _6502_OP(restore):
    case _6502_OP(label):
    case _6502_OP(asm):
    case _6502_OP(decsp):
    case _6502_OP(incsp):
    case _6502_OP(pushi):
    case _6502_OP(pushh):
    case _6502_OP(pushb):
    case _6502_OP(pushf):
    case _6502_OP(pushd):
    case _6502_OP(pushx):
    case _6502_OP(popi):
    case _6502_OP(poph):
    case _6502_OP(popb):
    case _6502_OP(popf):
    case _6502_OP(popd):
    case _6502_OP(popx):
    case _6502_OP(stw):
    case _6502_OP(sth):
    case _6502_OP(stx):
    case _6502_OP(stf):
    case _6502_OP(std):
    case _6502_OP(stb):
    case _6502_OP(bt):
    case _6502_OP(bf):
    case _6502_OP(cbra):
    case _6502_OP(jmp):
    case _6502_OP(cjmp):
    case _6502_OP(calla):
    case _6502_OP(calli):
    case _6502_OP(callx):
    case _6502_OP(callf):
    case _6502_OP(calld):
    case _6502_OP(rcalla):
    case _6502_OP(rcalli):
    case _6502_OP(rcallx):
    case _6502_OP(rcallf):
    case _6502_OP(rcalld):
    case _6502_OP(ret):
    case _6502_OP(rmov):
    case _6502_OP(rmovf):
    case _6502_OP(rmovd):
    case _6502_OP(loc):
      return false;
      
    default:
      return true;
  }
}

bool _6502IsSignedLoad(_6502Opcode opcode) {
  switch (opcode) {
    case _6502_OP(ldb):
    case _6502_OP(ldh):
    case _6502_OP(ldw):
      return true;
    default:
      return false;
  }
}


