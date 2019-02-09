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
// opN means operand #N.
typedef enum {
  IR_OP(nop),  // No operation.
  IR_OP(tmp),  // Temporary result.

  // Constants.
  IR_OP(constb),  // 8-bit constant.
  IR_OP(consts),  // 16-bit constant.
  IR_OP(consti),  // 32-bit constant.
  IR_OP(constl),  // 64-bit constant.
  IR_OP(constf),  // Single precision floating point.
  IR_OP(constd),  // Double precision floating point.
  IR_OP(consta),  // Address.

  // Moves.
  IR_OP(movi),  // Move int op0.
  IR_OP(movf),  // Move float op0.
  IR_OP(movd),  // Move double op0.
  IR_OP(mova),  // Move address op0.

  IR_OP(rmovi),  // Move int op1 to op0
  IR_OP(rmovf),  // Move float op1 to op0
  IR_OP(rmovd),  // Move double op1 to op0
  IR_OP(rmova),  // Move address op1 to op0

  IR_OP(label),  // Label.

  // loads.
  IR_OP(loadi),   // Load signed 32-bit from [op0]
  IR_OP(loadb),   // Load signed 8-bit from [op0]
  IR_OP(loadl),   // Load 64-bit from [op0]
  IR_OP(loads),   // Load 16-bit from [op0]
  IR_OP(loadui),  // Load unsigned 32-bit from [op0]
  IR_OP(loadub),  // Load unsigned 8-bit from [op0]
  IR_OP(loadus),  // Load unsigned 16-bit from [op0]
  IR_OP(loadf),   // Load 32-bit float from [op0]
  IR_OP(loadd),   // Load 64-bit float from [op0]
  IR_OP(loada),   // Load address from [op0]

  // stores.
  IR_OP(storei),  // Store 32-bit op1 in [op0]
  IR_OP(storeb),  // Store 8-bit op1 in [op0]
  IR_OP(stores),  // Store 16-bit op1 in [op0]
  IR_OP(storel),  // Store 64-bit op1 in [op0]
  IR_OP(storef),  // Store 32-bit float op1 in [op0]
  IR_OP(stored),  // Store 64-bit float op1 in [op0]
  IR_OP(storea),  // Store address op1 in [op0]

  // Add.
  IR_OP(addi),  // op0 + op1 (ints)
  IR_OP(addf),  // op0 + op1 (floats)
  IR_OP(addd),  // op0 + op1 (doubles)
  IR_OP(adda),  // op0 + op1 (addresses)

  // Subtract.
  IR_OP(subi),  // op0 - op1 (ints)
  IR_OP(subf),  // op0 - op1 (floats)
  IR_OP(subd),  // op0 - op1 (doubles)
  IR_OP(suba),  // op0 - op1 (addresses)

  // Multiply.
  IR_OP(muli),  // op0 * op1 (ints)
  IR_OP(mulf),  // op0 * op1 (floats)
  IR_OP(muld),  // op0 * op1 (doubles)

  // Divide.
  IR_OP(divi),  // op0 / op1 (ints)
  IR_OP(divf),  // op0 / op1 (floats)
  IR_OP(divd),  // op0 / op1 (doubles)

  // Modulus.
  IR_OP(modi),  // op0 % op1

  // Shifts.
  IR_OP(lsri),  // op0 >> op1 (logical shift)
  IR_OP(asri),  // op0 >> op1 (arithmetic shift)
  IR_OP(lsli),  // op0 <<op1

  // Bitwise.
  IR_OP(ori),   // op0 | op1
  IR_OP(andi),  // op0 & op1
  IR_OP(xori),  // op0 ^ op1

  IR_OP(noti),      // !op0 (int)
  IR_OP(nota),      // !op0 (address)
  IR_OP(onescomp),  // ~op0
  IR_OP(negi),      // -op0 (int)
  IR_OP(negf),      // -op0 (float)
  IR_OP(negd),      // -op0 (double)

  // Compares (NOTE: keep these contiguous).
  IR_OP(cmpeqi),  // op0 == op1 (ints)
  IR_OP(cmpnei),  // op0 != op1 (ints)
  IR_OP(cmplti),  // op0 < op1 (ints)
  IR_OP(cmplei),  // op0 <= op1 (ints)
  IR_OP(cmpgti),  // op0 > op1 (ints)
  IR_OP(cmpgei),  // op0 >= op1 (ints)

  IR_OP(cmpeqf),  // op0 == op1 (floats)
  IR_OP(cmpnef),  // op0 != op1 (floats)
  IR_OP(cmpltf),  // op0 < op1 (floats)
  IR_OP(cmplef),  // op0 <= op1 (floats)
  IR_OP(cmpgtf),  // op0 > op1 (floats)
  IR_OP(cmpgef),  // op0 >= op1 (floats)

  IR_OP(cmpeqd),  // op0 == op1 (doubles)
  IR_OP(cmpned),  // op0 != op1 (doubles)
  IR_OP(cmpltd),  // op0 < op1 (doubles)
  IR_OP(cmpled),  // op0 <= op1 (doubles)
  IR_OP(cmpgtd),  // op0 > op1 (doubles)
  IR_OP(cmpged),  // op0 >= op1 (doubles)

  IR_OP(cmpeqa),  // op0 == op1 (addresses)
  IR_OP(cmpnea),  // op0 != op1 (addresses)
  IR_OP(cmplta),  // op0 < op1 (addresses)
  IR_OP(cmplea),  // op0 <= op1 (addresses)
  IR_OP(cmpgta),  // op0 > op1 (addresses)
  IR_OP(cmpgea),  // op0 >= op1 (addresses)

  // Relative branches.
  IR_OP(btrue),   // If op0 goto op1
  IR_OP(bfalse),  // If !op0 goto op1
  IR_OP(bra),     // Goto op0
  IR_OP(cbra),    // Computed branch.

  // Call and return.
  IR_OP(calla),  // Call op0 with args op1,...
  IR_OP(ret),    // Return

  // Procedure entry and exit.
  IR_OP(enter),  // Enter procedure.
  IR_OP(leave),  // Leave procedure

  IR_OP(localvar),      // Local variable.
  IR_OP(externvar),     // External global variable.
  IR_OP(argument),      // Function formal argument.
  IR_OP(staticvar),     // Static variable.
  IR_OP(tempvar),       // Temporary variable
  IR_OP(ssavar),        // SSA renamed variable.
  IR_OP(structreturn),  // Struct return value.

  IR_OP(literalref),  // Load literal op0
  IR_OP(structref),   // Load struct address [op0]
  IR_OP(loc),         // Source location

  // Function results.
  IR_OP(resulti),  // Int result is op0
  IR_OP(resultf),  // Float result is op0
  IR_OP(resultd),  // Double result is op0
  IR_OP(resulta),  // Address result is op0

  IR_OP(i2f),  // (float)op0
  IR_OP(i2d),  // (double)op0
  IR_OP(f2d),  // (double)op0
  IR_OP(d2f),  // (float)op0
  IR_OP(f2i),  // (int)op0
  IR_OP(d2i),  // (double)op0

  IR_OP(maski),        // op0 & op1
  IR_OP(signextendi),  // (op0 << op1) >> op1

  IR_OP(memzero),  // memset(op0, 0, size)
  IR_OP(memcpy),   // memcpy(op0, op1, op2)

  IR_OP(phi),  // SSA form phi function.

  IR_OP(asm),  // Inline assembly language.

  // stdarg builtins.
  IR_OP(builtin_va_start),  // va_start(op0, op1)
  IR_OP(builtin_va_arg),    // va_arg(op0, op1)
  IR_OP(builtin_va_end),    // va_end(op0)
  IR_OP(builtin_va_copy),   // va_copy(op0, op1)

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
