//
//  6502_codegen.h
//  c_compiler_library
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef _6502_codegen_h
#define _6502_codegen_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "target_generator.h"
#include "6502_machine.h"
#include "6502_reg_alloc.h"

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
  kAddrModeAbsoluteSymbol,     // Absolute address lda %abs(symbol)
  kAddrModeZeroPage,           // Single zero-page.
  kAddrModeZeroPageIndirect,           // Zero-page indirect.
  kAddrModeIndirect,           // (address) only for JMP (a)
  kAddrModeAbsoluteIndexedX,    // addr,X
  kAddrModeAbsoluteIndexedY,    // addr,Y
  kAddrModeZeroPageIndexedX,    // zp,X
  kAddrModeZeroPageIndexedY,    // zp,Y
  kAddrModeIndexedIndirect,    // (zp, X)
  kAddrModeIndirectIndexed,    // (zp), Y
  kAddrModeSymbolLo,          // symbol (absolute) LO
  kAddrModeSymbolHi,          // symbol (abosolute) HI
  kAddrModeInvalid,
} AddressingMode;

// Additional 6502 flags.  Held in upper 16 bits of flags.  Lowest 6 bits
// are the addressing mode.
#define k6502FlagSignedLoad (1 << 22)
#define k6502FlagContainsAddress (1 << 23)

#define _6502_OP(op) k6502Op_##op

// Mark for beginning and end of basic blocks.
#define k6502BlockStart (1 << 24)
#define k6502BlockEnd (1 << 25)

// This instruction is a call instruction.
#define k6502InstIsCall (1 << 26)

// Instruction is conditional branch.
#define k6502InstIsCondBranch (1 << 27)

// Expression is result of a call.
#define k6502ExprIsCallResult (1 << 28)

// This a real procedure call instruction.
#define k6502ProcedureCall (1 << 29)

// This comparison was generated.  Used to in conditional branch.
#define k6502ComparisonGenerated (1 << 30)

// These opcodes are an extension of the TargetOpcode enumeration.
typedef enum {
  // The initial sequence for these o_6502 must match the TargetOpcode
  // enumeration.
  _6502_OP(save),
  _6502_OP(restore),
  
  _6502_OP(symbol),   // Static symbol.
  _6502_OP(literal),  // String literal.
  _6502_OP(tmp),
  
  // Constants.
  _6502_OP(constb),
  _6502_OP(consth),
  _6502_OP(constw),
  _6502_OP(constx),
  _6502_OP(constf),
  _6502_OP(constd),
  
  _6502_OP(mov),
  _6502_OP(movf),
  _6502_OP(movd),
  
  _6502_OP(movc),
  _6502_OP(movfc),
  _6502_OP(movdc),
  _6502_OP(movxc),

  _6502_OP(rmov),
  _6502_OP(rmovf),
  _6502_OP(rmovd),
  
  _6502_OP(ret),
  
  _6502_OP(label),
  
  _6502_OP(fp),  // Frame pointer pseudo operation.
  _6502_OP(sp),  // Stack pointer pseudo operation.
  _6502_OP(tp),   // Thread pointer.
  
  // Function result registers.
  _6502_OP(resultx),
  _6502_OP(resultf),
  _6502_OP(resultd),
  
  _6502_OP(structreturn),  // Struct return address.
  
  _6502_OP(asm),
  
  _6502_OP(loc),
  _6502_OP(named_label),
  _6502_OP(ivarreg),
  _6502_OP(fvarreg),

  // End of TargetOpcode enumeration.

  _6502_OP(expr1),
  _6502_OP(expr2),
  _6502_OP(expr4),
  _6502_OP(expr8),
  _6502_OP(expr_addr_a),
  _6502_OP(expr_addr_x),
  _6502_OP(expr_addr_y),
  _6502_OP(localvar),
  _6502_OP(argument),
  _6502_OP(literalreflo),   // A = literal lo
  _6502_OP(literalrefhi),   // A = literal hi
  _6502_OP(literalref),   // X,Y = addr of literal
  _6502_OP(enter),
  _6502_OP(leave),
  _6502_OP(enter_leaf),
  _6502_OP(leave_leaf),
  
  _6502_OP(var_addr),
  _6502_OP(var_addrb),
  _6502_OP(arg_addr),
  _6502_OP(arg_addrb),

  _6502_OP(var_addr_xy),
  _6502_OP(var_addrb_xy),
  _6502_OP(arg_addr_xy),
  _6502_OP(arg_addrb_xy),

  _6502_OP(var_value1),
  _6502_OP(var_value1b),

  _6502_OP(var_value2),
  _6502_OP(var_value2b),

  _6502_OP(var_value4),
  _6502_OP(var_value4b),

  _6502_OP(var_value8),
  _6502_OP(var_value8b),

  _6502_OP(arg_value1),
  _6502_OP(arg_value1b),

  _6502_OP(arg_value2),
  _6502_OP(arg_value2b),

  _6502_OP(arg_value4),
  _6502_OP(arg_value4b),

  _6502_OP(arg_value8),
  _6502_OP(arg_value8b),

  _6502_OP(fake_bra),
  _6502_OP(spill1),
  _6502_OP(spill2),
  _6502_OP(spill4),
  _6502_OP(spill8),
  _6502_OP(unspill1),
  _6502_OP(unspill2),
  _6502_OP(unspill4),
  _6502_OP(unspill8),


    _6502_OP(brk),
    
    _6502_OP(bpl),
    _6502_OP(bmi),
    _6502_OP(bvc),
    _6502_OP(bvs),
    _6502_OP(bcc),
    _6502_OP(bcs),
    _6502_OP(bne),
    _6502_OP(beq),
    
    _6502_OP(jsr),
    _6502_OP(jmp),
    
    _6502_OP(rti),
    _6502_OP(rts),
    
    _6502_OP(lda),
    _6502_OP(ldx),
    _6502_OP(ldy),
    _6502_OP(sta),
    _6502_OP(stx),
    _6502_OP(sty),
    
    _6502_OP(cmp),
    _6502_OP(cpy),
    _6502_OP(cpx),
    _6502_OP(bit),
    
    _6502_OP(ora),
    _6502_OP(and),
    _6502_OP(eor),
    
    _6502_OP(adc),
    _6502_OP(sbc),
    
    _6502_OP(asl),
    _6502_OP(rol),
    _6502_OP(lsr),
    _6502_OP(ror),
    
    _6502_OP(dec),
    _6502_OP(inc),
    _6502_OP(dey),
    _6502_OP(dex),
    _6502_OP(iny),
    _6502_OP(inx),
    
    _6502_OP(php),
    _6502_OP(clc),
    _6502_OP(plp),
    _6502_OP(sec),
    _6502_OP(pha),
    _6502_OP(cli),
    _6502_OP(pla),
    _6502_OP(sei),
    _6502_OP(tay),
    _6502_OP(clv),
    _6502_OP(cld),
    _6502_OP(sed),
    
    _6502_OP(tya),
    _6502_OP(txa),
    _6502_OP(txs),
    _6502_OP(tax),
    _6502_OP(tsx),
    
    _6502_OP(nop),
    
    // 65C02
    _6502_OP(tsb),
    _6502_OP(trb),
    _6502_OP(stz),
    _6502_OP(phy),
    _6502_OP(ply),
    _6502_OP(phx),
    _6502_OP(plx),
    _6502_OP(bra),
  
  _6502_OP(ssavar),
  _6502_OP(phi),
  
  _6502_OP(jumptable),   // Jump table (2-byte address)

  // Pseudo instructions:
  _6502_OP(ap),


} _6502Opcode;

// A 6502 Generator is derived from a TargetGenerator.  It has
// a '.base' field that is the TargetGenerator.
typedef struct _6502Generator {
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
  Symbol* push4;
  Symbol* push8;
  Symbol* push4xy;
  Symbol* push8xy;
  Symbol* pulla;
  Symbol* pullxy;
  Symbol* pull4;
  Symbol* pull8;
  Symbol* incsp1;
  Symbol* incsp2;
  Symbol* pushmem1;   // 1 byte size.
  Symbol* pushmem2;   // 2 byte size.
  Symbol* copymem1;   // 1 byte size.
  Symbol* copymem2;   // 2 byte size.
  Symbol* zeromem1;   // 1 byte size.
  Symbol* zeromem2;   // 2 byte size.
  Symbol* result2;
  Symbol* result4;
  Symbol* result8;
  
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
  Symbol* dmul;
 
  Symbol* sdiv1;
  Symbol* sdiv2;
  Symbol* sdiv4;
  Symbol* sdiv8;
  Symbol* udiv1;
  Symbol* udiv2;
  Symbol* udiv4;
  Symbol* udiv8;
  Symbol* fdiv;
  Symbol* ddiv;

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
  Symbol* i1tod;
  Symbol* i2tod;
  Symbol* i4tod;
  Symbol* i8tod;
  
  Symbol* ui1tof;
  Symbol* ui2tof;
  Symbol* ui4tof;
  Symbol* ui8tof;
  Symbol* ui1tod;
  Symbol* ui2tod;
  Symbol* ui4tod;
  Symbol* ui8tod;

  Symbol* ftod;
  Symbol* dtof;
  
  Symbol* ftoi1;
  Symbol* ftoi2;
  Symbol* ftoi4;
  Symbol* ftoi8;
  Symbol* dtoi1;
  Symbol* dtoi2;
  Symbol* dtoi4;
  Symbol* dtoi8;
  Symbol* ftoui1;
  Symbol* ftoui2;
  Symbol* ftoui4;
  Symbol* ftoui8;
  Symbol* dtoui1;
  Symbol* dtoui2;
  Symbol* dtoui4;
  Symbol* dtoui8;

  Symbol* cmpeqf;
  Symbol* cmpnef;
  Symbol* cmpltf;
  Symbol* cmpgef;
  
  Symbol* cmpeqd;
  Symbol* cmpned;
  Symbol* cmpltd;
  Symbol* cmpged;

  Symbol* jump_table;

  Vector branches;
  
  // Register allocator.
  _6502RegisterAllocator register_allocator;
} _6502Generator;

void _6502GeneratorInit(_6502Generator* g, Generator* gen);
_6502Generator* New6502Generator(Generator* gen);

void _6502GeneratorDestruct(_6502Generator* g);
void _6502GeneratorDelete(_6502Generator* g);

// Lower the IR to _6502.
void _6502Lower(_6502Generator* g, Generator* gen);
void _6502Print(_6502Generator* g, FILE* fp);
void _6502PrintInstruction(TargetInstruction* inst, FILE* fp);

const char* _6502OpcodeName(int op);
bool _6502IsSignedLoad(TargetInstruction* inst);
bool _6502IsExpression(TargetInstruction* inst);

#define _6502_CMP_EXPR 0x10000      // Compare instruction is an expression.

bool _6502IsBranch(TargetInstruction* inst);

bool _6502IsCall(TargetInstruction* inst);

bool _6502IsReturn(TargetInstruction* inst);

bool _6502IsSpill(TargetInstruction* inst);

bool _6502IsLabel(TargetInstruction* inst);

bool _6502IsFloatingPoint(TargetInstruction* inst);
bool _6502IsConditionalBranch(TargetInstruction* inst) ;

bool _6502IsFixedRegister(TargetInstruction* inst);
bool _6502IsConst(TargetInstruction* inst);

bool _6502IsSymbol(TargetInstruction* inst);

#endif /* _6502_codegen_h */
