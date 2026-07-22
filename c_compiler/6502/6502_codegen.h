//
//  6502_codegen.h
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef W65C02_codegen_h
#define W65C02_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "target_generator.h"
#include "6502_machine.h"
#include "6502_reg_alloc.h"

bool Is65c02(void);

// 6502 addressing modes. The value is stored in the top
// 16 bits of the TargetInstruction's flags member.
typedef enum {
  kAddrModeUnknown = 0,
  kAddrModeImplied = 1,        // not used.
  kAddrModeAccumulator,        // A
  kAddrModeImmediate,          // #xx
  kAddrModeZeroPageImmediate,  // #zp_offset
  kAddrModeZeroPageAbsolute,   // Zero page (non register)
  kAddrModeRelative,           // PC relative (branches)
  kAddrModeAbsolute,           // Address (jmp, jsr)
  kAddrModeAbsoluteSymbol,     // Absolute address
  kAddrModeAbsoluteSymbolIndexedY,     // Absolute address, Y
  kAddrModeAbsoluteSymbolIndexedX,     // Absolute address, X
  kAddrModeZeroPage,           // Single zero-page.
  kAddrModeIndirect,           // (address) only for JMP (a)
  kAddrModeAbsoluteIndexedX,    // addr,X
  kAddrModeAbsoluteIndexedY,    // addr,Y
  kAddrModeZeroPageIndexedX,    // zp,X
  kAddrModeZeroPageIndexedY,    // zp,Y
  kAddrModeIndexedIndirect,    // (zp, X)
  kAddrModeIndirectIndexed,    // (zp), Y
  kAddrModeSymbolLo,          // symbol (absolute) LO
  kAddrModeSymbolHi,          // symbol (abosolute) HI
  kAddrModeSymbolAddr,        // LO or HI depending on second operand
  kAddrModeLiteralIndexedX,     // literal, X
  kAddrModeInvalid,
} AddressingMode;

// Additional 6502 flags.  Held in upper 16 bits of flags.  Lowest 6 bits
// are the addressing mode.

#define W65C02_OP(op) k6502Op_##op

// Mark for beginning and end of basic blocks.
#define k6502BlockStart (1 << 22)
#define k6502BlockEnd (1 << 23)

// This instruction is a call instruction.
#define k6502InstIsCall (1 << 24)

// Instruction is conditional branch.
#define k6502InstIsCondBranch (1 << 25)

// Expression is result of a call.
#define k6502ExprIsCallResult (1 << 26)

// This a real procedure call instruction.
#define k6502ProcedureCall (1 << 27)

// This instruction generates flags.
#define k6502GeneratesFlags (1 << 27)

// This comparison was generated.  Used to in conditional branch.
#define k6502ComparisonGenerated (1 << 28)

// Use JMP instead of JSR for call.
#define k6502JmpForJSR (1 << 29)

// Set on an enter/enter_leaf instruction when the function returns a value
// (non-void, non-aggregate).  The emitter then uses the __enter*_res runtime
// entry points, which store the X,Y result address into __result themselves,
// instead of emitting a separate stx/sty pair in every function prologue.
#define k6502EnterStoresResult (1 << 30)

// Need address of symbol, not value.
#define k6502NeedAddress (1 << 30)

// Don't emit this instruction.  Used when we can't delete the instruction
// but need it not to be emitted (for example, a CMP #0).
#define k6502DontEmit (1 << 31)

#define k6502ExtendedAsm (1 << 21)
#define W65C02_MAX_ASM_OPERANDS 16

typedef struct {
  TargetInstruction base;
  AsmASTNode* asm_node;
  int num_operands;
  int64_t immediate_values[W65C02_MAX_ASM_OPERANDS];
  bool is_immediate[W65C02_MAX_ASM_OPERANDS];
  TargetInstruction* zp_operands[W65C02_MAX_ASM_OPERANDS];
} W65C02AsmInstruction;

// These opcodes are an extension of the TargetOpcode enumeration.
typedef enum {
  // The initial sequence for these oW65C02 must match the TargetOpcode
  // enumeration.
  W65C02_OP(save),
  W65C02_OP(restore),
  
  W65C02_OP(symbol),   // Static symbol.
  W65C02_OP(literal),  // String literal.
  W65C02_OP(tmp),
  
  // Constants.
  W65C02_OP(const8),
  W65C02_OP(const16),
  W65C02_OP(const32),
  W65C02_OP(const64),
  W65C02_OP(constf),
  W65C02_OP(constd),
  
  W65C02_OP(mov),
  W65C02_OP(movf),
  W65C02_OP(movd),
  
  W65C02_OP(movc),
  W65C02_OP(movfc),
  W65C02_OP(movdc),
  W65C02_OP(movxc),

//  W65C02_OP(rmov),
//  W65C02_OP(rmovf),
//  W65C02_OP(rmovd),
//
  W65C02_OP(ret),
  
  W65C02_OP(label),
  
  W65C02_OP(fp),  // Frame pointer pseudo operation.
  W65C02_OP(sp),  // Stack pointer pseudo operation.
  W65C02_OP(tp),   // Thread pointer.
  
  // Function result registers.
  W65C02_OP(resulti),
  W65C02_OP(resultf),
  W65C02_OP(resultd),
  
  W65C02_OP(structreturn),  // Struct return address.
  
  W65C02_OP(asm),
  
  W65C02_OP(loc),
  W65C02_OP(named_label),

  // Not used here.
  W65C02_OP(_ivarreg),
  W65C02_OP(_fvarreg),

  // End of TargetOpcode enumeration.

  W65C02_OP(expr1),
  W65C02_OP(expr2),
  W65C02_OP(expr4),
  W65C02_OP(expr8),
  W65C02_OP(exprf),
  W65C02_OP(exprd),
  W65C02_OP(load_result),
  W65C02_OP(load_indirect4),
  W65C02_OP(load_indirect8),
  W65C02_OP(store_indirect4),
  W65C02_OP(store_indirect8),
  W65C02_OP(expr_addr_a),
  W65C02_OP(expr_addr_x),
  W65C02_OP(expr_addr_y),
  W65C02_OP(localvar),
  W65C02_OP(argument),
  W65C02_OP(literalreflo),   // A = literal lo
  W65C02_OP(literalrefhi),   // A = literal hi
  W65C02_OP(literalref),   // X,Y = addr of literal
  W65C02_OP(stringliteralref),   // X,Y = addr of literal
  W65C02_OP(literalrefX),     // literal,X

  W65C02_OP(enter),
  W65C02_OP(leave),
  W65C02_OP(enter_leaf),
  W65C02_OP(leave_leaf),

  W65C02_OP(var_addr),
  W65C02_OP(var_addrb),
  W65C02_OP(arg_addr),
  W65C02_OP(arg_addrb),

  W65C02_OP(var_addr_xy),
  W65C02_OP(var_addrb_xy),
  W65C02_OP(arg_addr_xy),
  W65C02_OP(arg_addrb_xy),

  W65C02_OP(var_value1),
  W65C02_OP(var_value1b),

  W65C02_OP(var_value2),
  W65C02_OP(var_value2b),

  W65C02_OP(var_value4),
  W65C02_OP(var_value4b),

  W65C02_OP(var_value8),
  W65C02_OP(var_value8b),

  W65C02_OP(arg_value1),
  W65C02_OP(arg_value1b),

  W65C02_OP(arg_value2),
  W65C02_OP(arg_value2b),

  W65C02_OP(arg_value4),
  W65C02_OP(arg_value4b),

  W65C02_OP(arg_value8),
  W65C02_OP(arg_value8b),

  W65C02_OP(fake_bra),
  W65C02_OP(spill1),
  W65C02_OP(spill2),
  W65C02_OP(spill4),
  W65C02_OP(spill8),
  W65C02_OP(reload1),
  W65C02_OP(reload2),
  W65C02_OP(reload4),
  W65C02_OP(reload8),
  W65C02_OP(reloadpoint),

  W65C02_OP(ivarreg),
  W65C02_OP(bvarreg),
  W65C02_OP(lvarreg),
  W65C02_OP(xvarreg),
  W65C02_OP(fvarreg),
  W65C02_OP(dvarreg),

  W65C02_OP(pushreg2),
  W65C02_OP(pushreg4),
  W65C02_OP(pushreg8),

    W65C02_OP(brk),
    
    W65C02_OP(bpl),
    W65C02_OP(bmi),
    W65C02_OP(bvc),
    W65C02_OP(bvs),
    W65C02_OP(bcc),
    W65C02_OP(bcs),
    W65C02_OP(bne),
    W65C02_OP(beq),
    
    W65C02_OP(jsr),
    W65C02_OP(jmp),
    
    W65C02_OP(rti),
    W65C02_OP(rts),
    
    W65C02_OP(lda),
    W65C02_OP(ldx),
    W65C02_OP(ldy),
    W65C02_OP(sta),
    W65C02_OP(stx),
    W65C02_OP(sty),
    
    W65C02_OP(cmp),
    W65C02_OP(cpy),
    W65C02_OP(cpx),
    W65C02_OP(bit),
    
    W65C02_OP(ora),
    W65C02_OP(and),
    W65C02_OP(eor),
    
    W65C02_OP(adc),
    W65C02_OP(sbc),
    
    W65C02_OP(asl),
    W65C02_OP(rol),
    W65C02_OP(lsr),
    W65C02_OP(ror),
    
    W65C02_OP(dec),
    W65C02_OP(inc),
    W65C02_OP(dey),
    W65C02_OP(dex),
    W65C02_OP(iny),
    W65C02_OP(inx),
    
    W65C02_OP(php),
    W65C02_OP(clc),
    W65C02_OP(plp),
    W65C02_OP(sec),
    W65C02_OP(pha),
    W65C02_OP(cli),
    W65C02_OP(pla),
    W65C02_OP(sei),
    W65C02_OP(tay),
    W65C02_OP(clv),
    W65C02_OP(cld),
    W65C02_OP(sed),
    
    W65C02_OP(tya),
    W65C02_OP(txa),
    W65C02_OP(txs),
    W65C02_OP(tax),
    W65C02_OP(tsx),
    
    W65C02_OP(nop),
    
    // 65C02
    W65C02_OP(tsb),
    W65C02_OP(trb),
    W65C02_OP(stz),
    W65C02_OP(phy),
    W65C02_OP(ply),
    W65C02_OP(phx),
    W65C02_OP(plx),
    W65C02_OP(bra),
  
  W65C02_OP(ssavar),
  W65C02_OP(phi),
  
  W65C02_OP(jumptable),   // Jump table (2-byte address)

  // Pseudo instructions:
  W65C02_OP(ap),
} W65C02Opcode;

// Number of variable registers for each type.
#define kNumIVars 8
#define kNumBVars 2
#define kNumLVars 2
#define kNumXVars 1
#define kNumFVars 1

#define kMaxVars 8

typedef struct  {
  TargetInstruction* var;
  TargetInstruction* reg;
} RegisterVariable;

// A set of register variables.
typedef struct {
  int max_vars;
  int num_vars;
  W65C02RegisterType type;
  W65C02Opcode opcode;
  RegisterVariable vars[kMaxVars];  // Pointers to TargetSymbol.
} RegisterVariableSet;

#define kNumVarSets 6

// These are builtin versions of common C library functions.  The idea
// is that the hand-coded assembly language is quicker to execute than
// the generated code as it can use zero page directly and not have to
// get args passed on the stack.
typedef struct {
  Symbol* symbol;
  int index;   
} Intrinsic;

// Known intrinsics:
#define kIntrinsicIsalnum 1
#define kIntrinsicIsalpha 2
#define kIntrinsicIsblank 3
#define kIntrinsicIscntrl 4
#define kIntrinsicIsdigit 5
#define kIntrinsicIsgraph 6
#define kIntrinsicIslower 7
#define kIntrinsicIsprint 8
#define kIntrinsicIspunct 9
#define kIntrinsicIsspace 10
#define kIntrinsicIsupper 11
#define kIntrinsicIsxdigit 12
#define kIntrinsicTolower 13
#define kIntrinsicToupper 14
#define kIntrinsicMemcpy 15
#define kIntrinsicMemset 16
#define kIntrinsicMemcmp 17



// A 6502 Generator is derived from a TargetGenerator.  It has
// a '.base' field that is the TargetGenerator.
typedef struct W65C02Generator {
  TargetGenerator base;
  Generator* gen;
  TargetInstruction* argument_pointer;
  
  // Runtime helper functions.
  
  // Procedure entry and exit.
  // For enter:
  // X,Y; stack frame size.
  // For leave:
  // Y,X; stack frame size.
  Symbol* enter;
  Symbol* leave;
  Symbol* enter_leaf;
  Symbol* leave_leaf;

  // Load variable address into zero page:
  // A: offset into zero page where address will be stored
  // X: low byte of offset from fp for variable
  // Variables are below fp, args are above.
  // 'b' variants use Y as high byte of offset from fp.
  Symbol* var_addr;
  Symbol* var_addrb;
  Symbol* arg_addr;
  Symbol* arg_addrb;

  // Like var_addr but puts address in X,Y.
  Symbol* var_addr_xy;
  Symbol* var_addrb_xy;
  Symbol* arg_addr_xy;
  Symbol* arg_addrb_xy;

  // Load variable value into zero page.
  // A: offset into zero page where value will be stored
  // X: offset from fp for variable.
  // 'b' variants use Y as high byte of offset from fp.
  // 1 byte value.
  Symbol* var_value1;
  Symbol* var_value1b;
  Symbol* arg_value1;
  Symbol* arg_value1b;

  // 2 byte value.
  Symbol* var_value2;
  Symbol* var_value2b;
  Symbol* arg_value2;
  Symbol* arg_value2b;

  // 4 byte value.
  Symbol* var_value4;
  Symbol* var_value4b;
  Symbol* arg_value4;
  Symbol* arg_value4b;

  // 8 byte value.
  Symbol* var_value8;
  Symbol* var_value8b;
  Symbol* arg_value8;
  Symbol* arg_value8b;

  // Setting var values.
  Symbol* set_var_value1;
  Symbol* set_var_value1b;
  Symbol* set_arg_value1;
  Symbol* set_arg_value1b;

  // 2 byte value.
  Symbol* set_var_value2;
  Symbol* set_var_value2b;
  Symbol* set_arg_value2;
  Symbol* set_arg_value2b;

  // 4 byte value.
  Symbol* set_var_value4;
  Symbol* set_var_value4b;
  Symbol* set_arg_value4;
  Symbol* set_arg_value4b;

  // 8 byte value.
  Symbol* set_var_value8;
  Symbol* set_var_value8b;
  Symbol* set_arg_value8;
  Symbol* set_arg_value8b;

  // Setting var values to zero.
  Symbol* zero_var_value1;
  Symbol* zero_var_value1b;
  Symbol* zero_arg_value1;
  Symbol* zero_arg_value1b;

  // 2 byte value.
  Symbol* zero_var_value2;
  Symbol* zero_var_value2b;
  Symbol* zero_arg_value2;
  Symbol* zero_arg_value2b;

  // 4 byte value.
  Symbol* zero_var_value4;
  Symbol* zero_var_value4b;
  Symbol* zero_arg_value4;
  Symbol* zero_arg_value4b;

  // 8 byte value.
  Symbol* zero_var_value8;
  Symbol* zero_var_value8b;
  Symbol* zero_arg_value8;
  Symbol* zero_arg_value8b;
  
  // Push variable value onto stack.
  Symbol* push_var1;
  Symbol* push_var1b;
  Symbol* push_var2;
  Symbol* push_var2b;
  Symbol* push_var4;
  Symbol* push_var4b;
  Symbol* push_var8;
  Symbol* push_var8b;
  Symbol* push_arg1;
  Symbol* push_arg1b;
  Symbol* push_arg2;
  Symbol* push_arg2b;
  Symbol* push_arg4;
  Symbol* push_arg4b;
  Symbol* push_arg8;
  Symbol* push_arg8b;
  Symbol* pusha;
  Symbol* pushxy;
  Symbol* pushxy0;    // Y == 0
  Symbol* pushreg1;
  Symbol* pushreg2;
  Symbol* pushreg4;
  Symbol* pushreg8;
  Symbol* push4;
  Symbol* push8;
  Symbol* push4xy;
  Symbol* push8xy;
  Symbol* pulla;
  Symbol* pullxy;
  Symbol* pull4;
  Symbol* pull8;
  Symbol* incsp;
  Symbol* incsp2;
  Symbol* incsp4;
  Symbol* incsp6;
  Symbol* incsp8;
  Symbol* incsp10;
  Symbol* incsp12;
  Symbol* incsp14;
  Symbol* incsp16;
  Symbol* incsp0;
  Symbol* pushmem1;   // 1 byte size.
  Symbol* pushmem2;   // 2 byte size.
  Symbol* pushmem_xy1;   // 1 byte size, src in Y,X
  Symbol* pushmem_xy2;   // 2 byte size, src in Y,X
  Symbol* copymem1;   // 1 byte size.
  Symbol* copymem2;   // 2 byte size.
  Symbol* zeromem1;   // 1 byte size.
  Symbol* zeromem2;   // 2 byte size.
  Symbol* result1;
  Symbol* result2;
  Symbol* result4;
  Symbol* result8;
  
  Symbol* inc1;
  Symbol* inc2;     // inc 2 byte by n, hi != 0
  Symbol* inc21;    // inc 2 byte by 1
  Symbol* inc2b;    // inc 2 byte by n, hi = 0
  Symbol* inc4;
  Symbol* inc8;
  Symbol* incf;
  Symbol* incd;

  Symbol* rinc1;
  Symbol* rinc2;     // inc 2 byte by n, hi != 0
  Symbol* rinc21;    // inc 2 byte by 1
  Symbol* rinc2b;    // inc 2 byte by n, hi = 0
  Symbol* rinc4;
  Symbol* rinc8;
  Symbol* rincf;
  Symbol* rincd;

  Symbol* dec1;
  Symbol* dec2;
  Symbol* dec21;
  Symbol* dec2b;
  Symbol* dec4;
  Symbol* dec8;
  Symbol* decf;
  Symbol* decd;

  Symbol* rdec1;
  Symbol* rdec2;
  Symbol* rdec21;
  Symbol* rdec2b;
  Symbol* rdec4;
  Symbol* rdec8;
  Symbol* rdecf;
  Symbol* rdecd;

  Symbol* load_result;   // load __result from stack frame.

  Symbol* umul1;
  Symbol* umul2;
  Symbol* umul4;
  Symbol* umul8;
  Symbol* smul1;
  Symbol* smul2;
  Symbol* smul4;
  Symbol* smul8;
  Symbol* fmul;
  
  // Multiply int by 10.
  Symbol* umul2_10;
  Symbol* smul2_10;

  Symbol* sdiv1;
  Symbol* sdiv2;
  Symbol* sdiv4;
  Symbol* sdiv8;
  Symbol* udiv1;
  Symbol* udiv2;
  Symbol* udiv4;
  Symbol* udiv8;
  Symbol* fdiv;

  Symbol* smod1;
  Symbol* smod2;
  Symbol* smod4;
  Symbol* smod8;
  Symbol* umod1;
  Symbol* umod2;
  Symbol* umod4;
  Symbol* umod8;

  Symbol* i1tof;
  Symbol* i2tof;
  Symbol* i4tof;
  Symbol* i8tof;
  
  Symbol* ui1tof;
  Symbol* ui2tof;
  Symbol* ui4tof;
  Symbol* ui8tof;
  
  Symbol* ftoi1;
  Symbol* ftoi2;
  Symbol* ftoi4;
  Symbol* ftoi8;
  Symbol* ftoui1;
  Symbol* ftoui2;
  Symbol* ftoui4;
  Symbol* ftoui8;

  Symbol* cmpeqf;
  Symbol* cmpnef;
  Symbol* cmpltf;
  Symbol* cmpgef;
  
  Symbol* fadd;
  Symbol* fsub;
  Symbol* fneg;

  Symbol* zeroreg4;
  Symbol* zeroreg8;

  Symbol* jump_table1;
  Symbol* jump_table2;
  Symbol* jump_table4;
  Symbol* jump_table8;

  Symbol* decsp;
  Symbol* savesp;
  Symbol* restoresp;

  Symbol* builtin_va_arg2;
  Symbol* builtin_va_arg4;
  Symbol* builtin_va_arg8;
  Symbol* builtin_va_arg;

  Map intrinsics;
  int next_intrinsic_index;

  // Owns every Symbol created by CreateRuntimeSymbol (the __enter/__leave/...
  // runtime helpers), which are also referenced by named fields above.
  Vector runtime_symbols;

  Vector branches;
  
  TargetInstruction* struct_return_inst;
  
  // Register variables.  
  RegisterVariableSet reg_vars[kNumVarSets];
  
  // Register allocator.
  W65C02RegisterAllocator register_allocator;
} W65C02Generator;

void W65C02GeneratorInit(W65C02Generator* g, Generator* gen);
W65C02Generator* New6502Generator(Generator* gen);

void W65C02GeneratorDestruct(W65C02Generator* g);
void W65C02GeneratorDelete(W65C02Generator* g);

// Lower the IR to W65C02.
void W65C02Lower(W65C02Generator* g, Generator* gen);
void W65C02Print(W65C02Generator* g, FILE* fp);
void W65C02PrintInstruction(TargetInstruction* inst, FILE* fp);

const char* W65C02OpcodeName(int op);
bool W65C02IsSignedLoad(TargetInstruction* inst);
bool W65C02IsExpression(TargetInstruction* inst);

#define W65C02_CMP_EXPR 0x10000      // Compare instruction is an expression.

bool W65C02IsBranch(TargetInstruction* inst);

bool W65C02IsCall(TargetInstruction* inst);

bool W65C02IsReturn(TargetInstruction* inst);

bool W65C02IsSpill(TargetInstruction* inst);

bool W65C02IsLabel(TargetInstruction* inst);

bool W65C02IsFloatingPoint(TargetInstruction* inst);
bool W65C02IsConditionalBranch(TargetInstruction* inst) ;

bool W65C02IsFixedRegister(TargetInstruction* inst);
bool W65C02IsConst(TargetInstruction* inst);

bool W65C02IsSymbol(TargetInstruction* inst);

#endif /* W65C02_codegen_h */
