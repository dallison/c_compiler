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
  kAddrModeImplied,            // none
  kAddrModeAccumulator,        // A
  kAddrModeImmediate,          // #xx
  kAddrModeRelative,           // PC relative (branches)
  kAddrModeAbsolute,           // Address (jmp, jsr)
  kAddrModeZeroPage,           // Single zero-page.
  kAddrModeIndirect,           // (address) only for JMP (a)
  kAddrModeAbsoluteIndexed,    // addr,X and addr,Y
  kAddrModeZeroPageIndexed,    // zp,X and zp,Y
  kAddrModeIndexedIndirect,    // (zp, X)
  kAddrModeIndirectIndexed,    // (zp), Y
} AddressingMode;

#define _6502_OP(op) k6502Op_##op

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

  // End of TargetOpcode enumeration.

  _6502_OP(movac),    // Move constant address (16 bit)
  _6502_OP(mova),     // Move address reg (16 bit)
  _6502_OP(movx),     // Move long reg (64 bit)
  _6502_OP(rmova),     // Move address reg (16 bit)
  _6502_OP(rmovx),     // Move address reg (64 bit)
  _6502_OP(resulta),  // 16 bit result.
  _6502_OP(resulti),  // 32 bit result.
  _6502_OP(tmpb),     // 8 bit temp.
  _6502_OP(tmpa),     // 16 bit temp.
  _6502_OP(tmpx),     // 64 bit temp.

  _6502_OP(enter),     // Subroutine entry.
  _6502_OP(rts),       // Return from subroutine.

  _6502_OP(localvar),
  _6502_OP(argument),
  
  // The lowering to 6502 can't go directly to the 6502
  // instruction set.  Instead we lower to operations similar
  // to P-Code and then use the emitter to produce the correct assembly
  // language for them.
  _6502_OP(ap),  // Argument pointer pseudo operation.
  
  // Stack manipulation
  _6502_OP(decsp),    // Decrement stack pointer by constant.
  _6502_OP(incsp),    // Increment stack pointer by constant.
  
  _6502_OP(pusha),     // Push address (16 bits) onto stack.
  _6502_OP(pushi),     // Push 32 bit integer onto stack.
  _6502_OP(pushh),     // Push 16 bit integer onto stack.
  _6502_OP(pushb),     // Push 8 bit integer onto stack.
  _6502_OP(pushf),    // Push single precision floating point.
  _6502_OP(pushd),    // Push double precision floating point.
  _6502_OP(pushx),    // Push 64 bit integer onto stack.
  _6502_OP(popa),     // Pop address (16 bits).
  _6502_OP(popi),     // Pop 32 bit integer.
  _6502_OP(poph),     // Pop 16 bit integer.
  _6502_OP(popb),     // Pop 8 bit integer.
  _6502_OP(popf),     // Pop single precision float.
  _6502_OP(popd),     // Pop double precision float.
  _6502_OP(popx),     // Pop 64 bit integer.
  
  // Loads: ld d,[r, #c].
  _6502_OP(lda),      // Load address.
  _6502_OP(ldw),      // Load signed 32 bit word.
  _6502_OP(ldh),      // Load signed 16 bit word.
  _6502_OP(ldb),      // Load signed byte.
  _6502_OP(lduw),     // Load unsigned 32 bit word.
  _6502_OP(ldub),     // Load unsigned byte.
  _6502_OP(lduh),     // Load unsigned 16 bit word.
  _6502_OP(ldx),      // Load 64 bit word.
  _6502_OP(ldf),      // Load single precision float.
  _6502_OP(ldd),      // Load double precision float.
  
  // Stores: st r,[d, #c]
  _6502_OP(sta),      // Store address (16 bits).
  _6502_OP(stw),      // Store 32 bit.
  _6502_OP(sth),      // Store 16 bit.
  _6502_OP(stx),      // Store 64 bit.
  _6502_OP(stf),      // Store single precision float.
  _6502_OP(std),      // Store double precision float.
  _6502_OP(stb),      // Store byte.
  
  // Add.
  _6502_OP(adda),     // Add address offset (16 bit int).
  _6502_OP(add),      // Add int.
  _6502_OP(addf),     // Add float.
  _6502_OP(addd),     // Add double.
  _6502_OP(addc),     // Add with constant.
  _6502_OP(addac),     // Add constant to address.

  // Subtract.
  _6502_OP(sub),      // Subtract int.
  _6502_OP(subf),     // Subtract float.
  _6502_OP(subd),     // Subtract double.
  
  // Multiply.
  _6502_OP(mula),     // Multiply address offset (16 bit(.
  _6502_OP(mul),      // Multiply int.
  _6502_OP(mulf),     // Multiply float.
  _6502_OP(muld),     // Multiply double.
  
  // Divide.
  _6502_OP(diva),     // Divide address offsets (16 bits).
  _6502_OP(div),      // Divide int.
  _6502_OP(divu),     // Divide int unsigned
  _6502_OP(divf),     // Divide float.
  _6502_OP(divd),     // Divide double.
  
  // Modulus.
  _6502_OP(mod),      // Integer modulus.
  _6502_OP(modu),     // Unsigned nteger modulus.
  
  // Shifts.
  _6502_OP(lsr),      // Logical shift right.
  _6502_OP(asr),      // Arithmetic shift right.
  _6502_OP(lsl),      // Logical shift left.
  
  // Bitwise.
  _6502_OP(or),       // OR.
  _6502_OP(and),      // AND.
  _6502_OP(xor),      // Exclusive OR.
  
  _6502_OP(not),      //  != 0 -> 0
  _6502_OP(inv),      // Ones complement.
  _6502_OP(neg),      // Negate int.
  _6502_OP(negf),     // Negate float.
  _6502_OP(negd),     // Negate double.
  
  // Compares.
  _6502_OP(cmpeqb),     // == byte.
  _6502_OP(cmpneb),     // != byte.
  _6502_OP(cmpltb),     // < byte.
  _6502_OP(cmpleb),     // <= byte.
  _6502_OP(cmpgtb),     // > byte.
  _6502_OP(cmpgeb),     // >= byte.
  _6502_OP(cmpltub),    // < unsigned byte.
  _6502_OP(cmpleub),    // <= unsigned byte.
  _6502_OP(cmpgtub),    // > unsigned byte.
  _6502_OP(cmpgeub),    // >= unsigned byte.
  
  _6502_OP(cmpeqa),     // == short.
  _6502_OP(cmpnea),     // != short.
  _6502_OP(cmplta),     // < short.
  _6502_OP(cmplea),     // <= short.
  _6502_OP(cmpgta),     // > short.
  _6502_OP(cmpgea),     // >= short.
  _6502_OP(cmpltua),    // < unsigned short.
  _6502_OP(cmpleua),    // <= unsigned short.
  _6502_OP(cmpgtua),    // > unsigned short.
  _6502_OP(cmpgeua),    // >= unsigned short.

  
  _6502_OP(cmpeqi),     // == int.
  _6502_OP(cmpnei),     // != int.
  _6502_OP(cmplti),     // < int.
  _6502_OP(cmplei),     // <= int.
  _6502_OP(cmpgti),     // > int.
  _6502_OP(cmpgei),     // >= int.
  _6502_OP(cmpltui),    // < unsigned int.
  _6502_OP(cmpleui),    // <= unsigned int.
  _6502_OP(cmpgtui),    // > unsigned int.
  _6502_OP(cmpgeui),    // >= unsigned int.

  _6502_OP(cmpeqx),     // == long.
  _6502_OP(cmpnex),     // != long.
  _6502_OP(cmpltx),     // < long.
  _6502_OP(cmplex),     // <= long.
  _6502_OP(cmpgtx),     // > long.
  _6502_OP(cmpgex),     // >= long.
  _6502_OP(cmpltux),    // < unsigned long.
  _6502_OP(cmpleux),    // <= unsigned long.
  _6502_OP(cmpgtux),    // > unsigned long.
  _6502_OP(cmpgeux),    // >= unsigned long.

  _6502_OP(cmpeqf),    // == float.
  _6502_OP(cmpnef),    // != float.
  _6502_OP(cmpltf),    // < float.
  _6502_OP(cmplef),    // <= float.
  _6502_OP(cmpgtf),    // > float.
  _6502_OP(cmpgef),    // >= float.
  
  _6502_OP(cmpeqd),    // == double.
  _6502_OP(cmpned),    // != double.
  _6502_OP(cmpltd),    // < double.
  _6502_OP(cmpled),    // <= double.
  _6502_OP(cmpgtd),    // > double.
  _6502_OP(cmpged),    // >= double.
  
  // Relative branches.
  _6502_OP(bt),       // Branch if condition true.
  _6502_OP(bf),        // Branch if condition false.
  _6502_OP(bra),       // Unconditional branch,
  
  // Computed branch.  Adds reg value to current PC and branches
  // to the calculated address.
  _6502_OP(cbra),
  
  _6502_OP(i2f),      // int to float.
  _6502_OP(i2d),      // int to double.
  _6502_OP(ui2f),     // unsigned int to float.
  _6502_OP(ui2d),     // unsigned int to double.
  _6502_OP(f2d),      // float to double.
  _6502_OP(d2f),      // double to float.
  _6502_OP(f2i),      // float to int.
  _6502_OP(d2i),      // double to int.
  _6502_OP(f2ui),     // float to unsigned int.
  _6502_OP(d2ui),     // double to unsigned int.
  
  _6502_OP(jmp),      // Jump to address.
  _6502_OP(cjmp),     // Jump to contents of address.
  _6502_OP(adr),      // PC relative address.
  _6502_OP(adrs),     // PC relative address of a string literal.
  _6502_OP(adrtls),      // PC relative TLS address.
  
  // Call and return.
  _6502_OP(calla),    // Call address with address result.
  _6502_OP(calli),     // Call address with 32 bit int result.
  _6502_OP(callx),     // Call address with 64 bit int result.
  _6502_OP(callf),    // Call address with float result.
  _6502_OP(calld),    // Call address with double result.
  _6502_OP(rcalla),   // Call register with address resutl.
  _6502_OP(rcalli),    // Call register with 32 bit int result.
  _6502_OP(rcallx),    // Call register with 64 bit int result.
  _6502_OP(rcallf),   // Call register with float result.
  _6502_OP(rcalld),   // Call register with double result.
} _6502Opcode;

// A 6502 Generator is derived from a TargetGenerator.  It has
// a '.base' field that is the TargetGenerator.
typedef struct _6502Generator {
  TargetGenerator base;
  TargetInstruction* argument_pointer;
  
  // Register allocator.
  _6502RegisterAllocator register_allocator;
} _6502Generator;

void _6502GeneratorInit(_6502Generator* g, Generator* gen);
_6502Generator* New6502Generator(Generator* gen);

void _6502GeneratorDestruct(_6502Generator* g);
void _6502GeneratorDelete(_6502Generator* g);

// Lower the IR to _6502.
void _6502Lower(_6502Generator* g, Generator* gen);
void _6502Print(_6502Generator* g);
void _6502PrintInstruction(TargetInstruction* inst, FILE* fp);

const char* _6502OpcodeName(int op);
bool _6502IsSignedLoad(_6502Opcode opcode);

#define _6502_CMP_EXPR 0x10000      // Compare instruction is an expression.

#endif /* _6502_codegen_h */
