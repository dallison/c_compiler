//
//  ir.h
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef ir_h
#define ir_h

#include <stdint.h>
#include "list.h"
#include "symbol.h"
#include "type.h"
#include "vector.h"

#define IR_OP(op) kIROpcode_##op

// Intermediate Representation (IR) opcodes.
typedef enum {
  IR_OP(nop),
  IR_OP(tmp),

  // Constants.
  IR_OP(constb),
  IR_OP(consts),
  IR_OP(consti),
  IR_OP(constl),
  IR_OP(constf),
  IR_OP(constd),
  IR_OP(consta),

  // Moves.
  IR_OP(movi),
  IR_OP(movf),
  IR_OP(movd),
  IR_OP(mova),

  IR_OP(rmovi),
  IR_OP(rmovf),
  IR_OP(rmovd),
  IR_OP(rmova),

  IR_OP(label),

  // loads.
  IR_OP(loadi),
  IR_OP(loadb),
  IR_OP(loadl),
  IR_OP(loads),
  IR_OP(loadui),
  IR_OP(loadub),
  IR_OP(loadus),
  IR_OP(loadf),
  IR_OP(loadd),
  IR_OP(loada),

  // stores.
  IR_OP(storei),
  IR_OP(storeb),
  IR_OP(stores),
  IR_OP(storel),
  IR_OP(storef),
  IR_OP(stored),
  IR_OP(storea),

  // Add.
  IR_OP(addi),
  IR_OP(addf),
  IR_OP(addd),
  IR_OP(adda),

  // Subtract.
  IR_OP(subi),
  IR_OP(subf),
  IR_OP(subd),
  IR_OP(suba),

  // Multiply.
  IR_OP(muli),
  IR_OP(mulf),
  IR_OP(muld),

  // Divide.
  IR_OP(divi),
  IR_OP(divf),
  IR_OP(divd),

  // Modulus.
  IR_OP(modi),

  // Shifts.
  IR_OP(lsri),
  IR_OP(asri),
  IR_OP(lsli),

  // Bitwise.
  IR_OP(ori),
  IR_OP(andi),
  IR_OP(xori),

  IR_OP(noti),
  IR_OP(nota),
  IR_OP(onescomp),
  IR_OP(negi),
  IR_OP(negf),
  IR_OP(negd),

  // Compares (NOTE: keep these contiguous).
  IR_OP(cmpeqi),
  IR_OP(cmpnei),
  IR_OP(cmplti),
  IR_OP(cmplei),
  IR_OP(cmpgti),
  IR_OP(cmpgei),

  IR_OP(cmpeqf),
  IR_OP(cmpnef),
  IR_OP(cmpltf),
  IR_OP(cmplef),
  IR_OP(cmpgtf),
  IR_OP(cmpgef),

  IR_OP(cmpeqd),
  IR_OP(cmpned),
  IR_OP(cmpltd),
  IR_OP(cmpled),
  IR_OP(cmpgtd),
  IR_OP(cmpged),

  IR_OP(cmpeqa),
  IR_OP(cmpnea),
  IR_OP(cmplta),
  IR_OP(cmplea),
  IR_OP(cmpgta),
  IR_OP(cmpgea),

  // Relative branches.
  IR_OP(btrue),
  IR_OP(bfalse),
  IR_OP(bra),
  IR_OP(cbra),  // Computed branch.

  // Call and return.
  IR_OP(calla),
  IR_OP(ret),

  // Procedure entry and exit.
  IR_OP(enter),
  IR_OP(leave),

  IR_OP(localvar),   // Local variable.
  IR_OP(externvar),  // External global variable.
  IR_OP(argument),   // Function formal argument.
  IR_OP(staticvar),
  IR_OP(tempvar),
  IR_OP(ssavar),        // SSA renamed variable.
  IR_OP(structreturn),  // Struct return value.

  IR_OP(literalref),
  IR_OP(structref),
  IR_OP(loc),

  // Function results.
  IR_OP(resulti),
  IR_OP(resultf),
  IR_OP(resultd),
  IR_OP(resulta),

  IR_OP(i2f),
  IR_OP(i2d),
  IR_OP(f2d),
  IR_OP(d2f),
  IR_OP(f2i),
  IR_OP(d2i),

  IR_OP(maski),
  IR_OP(signextendi),

  IR_OP(memzero),
  IR_OP(memcpy),

  IR_OP(phi),  // SSA form phi function.

  IR_OP(asm),  // Inline assembly language.

  // stdarg builtins.
  IR_OP(builtin_va_start),
  IR_OP(builtin_va_arg),
  IR_OP(builtin_va_end),
  IR_OP(builtin_va_copy),

  last_ir_opcode,
} IROpcode;

const char* IROpcodeName(IROpcode opcode);

struct BasicBlock;

typedef struct IRNode {
  ListElement header;
  int id;
  IROpcode opcode;
  Vector inputs;
  Vector outputs;
  struct BasicBlock* block;
  TypeRecord* type;
  int flags;
  struct {
    void* ptr;
    int32_t ivalue;
    int64_t lvalue;
  } data;  // Opaque data for codegen.
  union {
    Symbol* def;
    Symbol* use;
  } var;
} IRNode;

// Flags for IR nodes.
#define kIRVarDef 1  // Defines a variable.
#define kIRVarUse 2  // Uses a variable.

void IRInit(IRNode* inst, IROpcode opcode);
void IRDestruct(IRNode* inst);
void IRDelete(IRNode* inst);
void IRResetNodeId(void);

void IRSetType(IRNode* node, TypeRecord* type);

void IRAddInput(IRNode* from, IRNode* to);
void IRReplaceInput(IRNode* node, size_t index, IRNode* new);
void IRRemoveNode(IRNode* node);
void IRRemoveInput(IRNode* node, size_t index);

void IRSetVarUse(IRNode* inst, Symbol* var);
void IRSetVarDef(IRNode* inst, Symbol* var);

IRNode* NewIR(IROpcode opcode);
IRNode* NewIR1(IROpcode opcode, IRNode* op);
IRNode* NewIR2(IROpcode opcode, IRNode* op1, IRNode* op2);
IRNode* NewIR3(IROpcode opcode, IRNode* op1, IRNode* op2, IRNode* op3);
IRNode* NewIR4(IROpcode opcode, IRNode* op1, IRNode* op2, IRNode* op3,
               IRNode* op4);

IRNode* IRNext(IRNode* node);
IRNode* IRPrev(IRNode* node);
bool IRInList(IRNode* node);

void IRPrint(IRNode* inst);

typedef struct {
  IRNode base;
  union {
    int64_t ivalue;
    double fvalue;
  } value;
} IRConstant;

IRNode* NewIntIRConstant(TypeRecord* type, int64_t value);
IRNode* NewFloatingPointIRConstant(TypeRecord* type, double value);

typedef struct {
  IRNode base;
  SourceLocation location;
} IRLocation;

IRNode* NewIRLocation(SourceLocation location);

typedef struct {
  IRNode base;
  Symbol* symbol;
} IRVariable;

IRNode* NewIRVariable(Symbol* sym);
IRNode* NewIRPhi(Symbol* sym);
IRNode* NewIRSSAVar(Symbol* sym);

bool IRIsBranch(IRNode* node);
bool IRIsConditionalBranch(IRNode* node);
bool IRIsReturn(IRNode* node);
bool IRIsConst(IRNode* node);
bool IRIsVariable(IRNode* node);
bool IRIsAutoVariable(IRNode* node);
bool IRIsArgument(IRNode* node);
bool IRIsStaticVariable(IRNode* node);
bool IRIsZero(IRNode* node);

bool IRIsVarDef(IRNode* inst);
bool IRIsVarRef(IRNode* inst);

bool IRIsExpression(IRNode* inst);
bool IRIsCommutative(IRNode* inst);

bool IRIsComparison(IRNode* node);
bool IRIsStore(IRNode* node);
bool IRIsLoad(IRNode* node);

#endif /* ir_h */
