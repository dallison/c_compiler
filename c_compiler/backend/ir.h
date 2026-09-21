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

void IRSetLocation(SourceLocation loc);

// Intermediate Representation (IR) opcodes.
// opN means operand #N.
typedef enum {
  IR_OP(nop),  // No operation.
  IR_OP(tmp),  // Temporary result.

  // Constants.
  IR_OP(const8),  // 8-bit constant.
  IR_OP(const16),  // 16-bit constant.
  IR_OP(const32),  // 32-bit constant.
  IR_OP(const64),  // 64-bit constant.
  IR_OP(constf),  // Single precision floating point.
  IR_OP(constd),  // Double precision floating point.
  IR_OP(consta),  // Address.

  // Moves.
  IR_OP(movi),  // Move int op0.
  IR_OP(movf),  // Move float op0.
  IR_OP(movd),  // Move double op0.
  IR_OP(mova),  // Move address op0.

//  IR_OP(rmovi),  // Move int op1 to op0
//  IR_OP(rmovf),  // Move float op1 to op0
//  IR_OP(rmovd),  // Move double op1 to op0
//  IR_OP(rmova),  // Move address op1 to op0

  IR_OP(label),  // Label.
  IR_OP(named_label),  // Named label.

  // loads.
  IR_OP(load32),   // Load signed 32-bit from [op0]
  IR_OP(load8),   // Load signed 8-bit from [op0]
  IR_OP(load64),   // Load 64-bit from [op0]
  IR_OP(load16),   // Load 16-bit from [op0]
  IR_OP(loadu32),  // Load unsigned 32-bit from [op0]
  IR_OP(loadu8),  // Load unsigned 8-bit from [op0]
  IR_OP(loadu16),  // Load unsigned 16-bit from [op0]
  IR_OP(loadf),   // Load 32-bit float from [op0]
  IR_OP(loadd),   // Load 64-bit float from [op0]
  IR_OP(loada),   // Load address from [op0]
  IR_OP(structarg),  // Load struct address from [op0] for call
  IR_OP(vectorarg),  // Load a native vector value from object address for call
  
  // stores.
  IR_OP(store32),  // Store 32-bit op1 in [op0]
  IR_OP(store8),  // Store 8-bit op1 in [op0]
  IR_OP(store16),  // Store 16-bit op1 in [op0]
  IR_OP(store64),  // Store 64-bit op1 in [op0]
  IR_OP(storef),  // Store 32-bit float op1 in [op0]
  IR_OP(stored),  // Store 64-bit float op1 in [op0]
  IR_OP(storea),  // Store address op1 in [op0]

  // Bitfields
  // Load bit field from value.  Inputs are:
  // (bit_offaet, bit_size)
  IR_OP(getbit),

  // Store bit field in value.
  IR_OP(setbit),

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
  IR_OP(rotli), // rotate op0 left by op1
  IR_OP(rotri), // rotate op0 right by op1
  IR_OP(clzi),  // leading zero count
  IR_OP(ctzi),  // trailing zero count
  IR_OP(popcounti), // population count

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

  // Target-independent fixed-vector operations. Operands and destination are
  // object addresses; the result type records lane type and lane count.
  IR_OP(vadd),
  IR_OP(vsub),
  IR_OP(vmul),
  IR_OP(vdiv),
  IR_OP(vmod),
  IR_OP(vlsl),
  IR_OP(vlsr),
  IR_OP(vasr),
  IR_OP(vand),
  IR_OP(vor),
  IR_OP(vxor),
  IR_OP(vneg),
  IR_OP(vonescomp),
  IR_OP(vcmpeq),
  IR_OP(vcmpne),
  IR_OP(vcmplt),
  IR_OP(vcmple),
  IR_OP(vcmpgt),
  IR_OP(vcmpge),
  IR_OP(vcmpltu),
  IR_OP(vcmpleu),
  IR_OP(vcmpgtu),
  IR_OP(vcmpgeu),

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

  // C++20 three-way comparison.  Result is a signed integer: -1 (op0 < op1),
  // 0 (op0 == op1), 1 (op0 > op1), and for floating point 2 (unordered).
  IR_OP(cmp3wayi),  // signed integer three-way
  IR_OP(cmp3wayu),  // unsigned integer three-way
  IR_OP(cmp3wayf),  // float three-way (2 = unordered)
  IR_OP(cmp3wayd),  // double three-way (2 = unordered)
  IR_OP(cmp3waya),  // address three-way (unsigned)

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
  IR_OP(addressof),     // Address of operand.
  
  IR_OP(literalref),  // Load literal op0
  IR_OP(loc),         // Source location
  // C++26 observable checkpoint.  This has semantic side effects for code
  // motion but lowers to no machine instruction and is not a memory clobber.
  IR_OP(observable_checkpoint),

  IR_OP(pusharg),     // Push function arg (optional)
  
  // Function results.
  IR_OP(resulti),  // Int result is op0
  IR_OP(resultf),  // Float result is op0
  IR_OP(resultd),  // Double result is op0
  IR_OP(resulta),  // Address result is op0
  IR_OP(resultv),  // Native vector result loaded from object address
  IR_OP(capturev), // Store a native vector call result to object address

  IR_OP(i2f),  // (float)op0
  IR_OP(i2d),  // (double)op0
  IR_OP(f2d),  // (double)op0
  IR_OP(d2f),  // (float)op0
  IR_OP(f2i),  // (int)op0
  IR_OP(d2i),  // (double)op0

  IR_OP(zeroextendi),  // shorten/lengthen by number of bits.
  IR_OP(signextendi),  // sign extend by number of bits.
  IR_OP(aligni),  // (op0 + (op1-1)) & ~(op1-1)

  IR_OP(memzero),  // memset(op0, 0, size)
  IR_OP(memcpy),   // memcpy(op0, op1, op2)
  IR_OP(cast),     // Type cast.
  
  IR_OP(phi),  // SSA form phi function.

  IR_OP(asm),  // Inline assembly language.

  IR_OP(nrvoval),   // Named RVO value.
  
  IR_OP(decsp),
  IR_OP(savesp),
  IR_OP(restoresp),

  // stdarg builtins.
  IR_OP(builtin_va_start),  // va_start(op0, op1)
  IR_OP(builtin_va_arg),    // va_arg(op0, op1)
  IR_OP(builtin_va_end),    // va_end(op0)
  IR_OP(builtin_va_copy),   // va_copy(op0, op1)

  // Atomic builtins.
  IR_OP(atomic_load),                  // atomic_load(ptr, order)
  IR_OP(atomic_store),                 // atomic_store(ptr, value, order)
  IR_OP(atomic_fetch_add),             // atomic_fetch_add(ptr, value, order)
  IR_OP(atomic_fetch_sub),             // atomic_fetch_sub(ptr, value, order)
  IR_OP(atomic_add_fetch),             // atomic_add_fetch(ptr, value, order)
  IR_OP(atomic_sub_fetch),             // atomic_sub_fetch(ptr, value, order)
  IR_OP(atomic_compare_exchange_bool), // atomic_cmpxchg_bool(ptr, old, new, succ, fail)
  IR_OP(atomic_compare_exchange_val),  // atomic_cmpxchg_val(ptr, old, new, succ, fail)
  IR_OP(atomic_compare_exchange_n),    // atomic_cmpxchg_n(ptr, expected*, new, weak, succ, fail)
  IR_OP(atomic_fence),                 // atomic_fence(order)

  // Increment and decrement.  First input is the operand, second is
  // a constant for the increment or decrement.
  IR_OP(inc8),
  IR_OP(inc16),
  IR_OP(inc32),
  IR_OP(inc64),

  IR_OP(uinc8),
  IR_OP(uinc16),
  IR_OP(uinc32),
  IR_OP(uinc64),

  IR_OP(inca),
  IR_OP(incf),
  IR_OP(incd),

  IR_OP(dec8),
  IR_OP(dec16),
  IR_OP(dec32),
  IR_OP(dec64),

  IR_OP(udec8),
  IR_OP(udec16),
  IR_OP(udec32),
  IR_OP(udec64),

  IR_OP(deca),
  IR_OP(decf),
  IR_OP(decd),
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
  // C++26 byte/value state carried independently from the physical bits.
  ValueState value_state;
  int flags;    // Bottom 16 bits for IR, top 16 for target.
  struct {
    void* ptr;
    int32_t ivalue;
    int64_t lvalue;
  } data;  // Opaque data for codegen.
  union {
    Symbol* def;
    Symbol* use;
  } var;
  SourceLocation location;
  void* aux;                        // Opaque frontend/codegen payload.
  struct IRNode* dest;             // Result goes into here (optional).
} IRNode;

// Flags for IR nodes.
#define kIRVarDef (1 << 0) // Defines a variable.
#define kIRVarUse (1 << 1)  // Uses a variable.
#define kIRTailCall (1 << 2) // Call is a tail call
#define kIRReturnJump (1 << 3)   // Jump to return.
#define kIRRvoCall (1 << 4)     // Return value optimized call.
#define kIRNrvoMarker (1 << 5)  // Named Return Value optimized symbol.
#define kIRJumpTableBranch (1 << 6)  // Jump table bra.
#define kIRFakeUnsigned (1 << 7)  // This type is not really unsigned.
#define kIRFromCall (1 << 8)  // Struct arg is from a call.
#define kIRStashedCallResult (1 << 9)  // Scalar call arg result is stashed.
#define kIRStructReturnCall (1 << 10)  // Call has a hidden aggregate-result arg.
#define kIRDeferredArgReload (1 << 11)  // Reload after nested argument calls.
#define kIRDeferredArgRebuildAddress (1 << 12)  // Rebuild spill address late.
#define kIRAsmMemoryClobber (1 << 13)  // Inline asm may read or write any memory.
#define kIRBitWidth64 (1 << 14)  // Bit intrinsic operates on 64-bit values.
#define kIRInvalidValueDefinition (1 << 15)  // Physical store preserves invalid state.

void IRInit(IRNode* inst, IROpcode opcode);
void IRDestruct(IRNode* inst);
void IRDelete(IRNode* inst);
void IRResetNodeId(void);
void IRRenumberList(List* code);
// After deserialize or inlining, drop or rebind kIRVarDef/kIRVarUse so SSA
// rename can find the address operand's IRVariable.
void IRRepairVarDefUse(List* code);

IRNode* IRSetType(IRNode* node, TypeRecord* type);

void IRAddInput(IRNode* from, IRNode* to, bool copy_type);
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

void IRPrint(IRNode* inst, FILE* fp);

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
  const char* name;
} IRNamedLabel;

IRNode* NewIRNamedLabel(const char* name);

typedef struct {
  IRNode base;
  Symbol* symbol;
} IRVariable;


IRNode* NewIRVariable(Symbol* sym);
IRNode* NewIRPhi(Symbol* sym);
IRNode* NewIRSSAVar(Symbol* sym);

bool IRIsBranch(IRNode* node);
bool IRIsUnconditionalBranch(IRNode* node);
bool IRIsConditionalBranch(IRNode* node);
bool IRIsReturn(IRNode* node);
bool IRIsCall(IRNode* node);
bool IRAsmClobbersMemory(IRNode* node);
bool IRIsConst(IRNode* node);
bool IRIsIntConst(IRNode* node);
int64_t IRIntConstValue(IRNode* node);
bool IRIsVariable(IRNode* node);
// Symbol for an IRVariable-sized node. tmp/structreturn are IRIsVariable
// but allocated as IRNode, so their "symbol" field is not present.
Symbol* IRGetVariableSymbol(IRNode* node);
bool IRIsAutoVariable(IRNode* node);
bool IRIsArgument(IRNode* node);
bool IRIsStaticVariable(IRNode* node);
bool IRIsThreadVariable(IRNode* node);
bool IRIsZero(IRNode* node);

bool IRIsVarDef(IRNode* inst);
bool IRIsVarRef(IRNode* inst);

bool IRIsExpression(IRNode* inst);
bool IRIsConstant(IRNode* inst);
bool IRIsCommutative(IRNode* inst);

bool IRIsComparison(IRNode* node);
bool IRComparisonIsUnsigned(IRNode* node);
bool IRIsStoreOnly(IRNode* node);
bool IRIsStore(IRNode* node);
bool IRIsLoad(IRNode* node);
bool IRIsLoadOnly(IRNode* node);
bool IRIsIncDec(IRNode* node);

bool IRVariableAddressEscapes(IRNode* var_node);

bool IRIsResult(IRNode* node);
bool IRHasSideEffects(IRNode* node);
bool IRIsObservableCheckpoint(IRNode* node);
bool IRCheckpointBetween(IRNode* earlier, IRNode* later);

#endif /* ir_h */
