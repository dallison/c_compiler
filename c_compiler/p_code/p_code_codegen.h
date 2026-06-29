//
//  p_code.h
//  c_compiler
//
//  Created by David Allison on 12/22/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef p_code_h
#define p_code_h

#include "codegen.h"
#include "hashtable.h"
#include "list.h"
#include "p_code_reg_alloc.h"
#include "target_generator.h"

#define P_OP(op) kPCodek##op

// These opcodes are an extension of the TargetOpcode enumeration.
typedef enum {
  // The initial sequence for these opcode must match the TargetOpcode
  // enumeration.
  P_OP(save),
  P_OP(restore),

  P_OP(symbol),   // Static symbol.
  P_OP(literal),  // String literal.
  P_OP(tmp),

  // Constants.
  P_OP(const8),
  P_OP(const16),
  P_OP(const32),
  P_OP(const64),
  P_OP(constf),
  P_OP(constd),

  P_OP(mov),
  P_OP(movf),
  P_OP(movd),

  P_OP(movc),
  P_OP(movfc),
  P_OP(movdc),
  P_OP(movxc),

//  P_OP(rmov),
//  P_OP(rmovf),
//  P_OP(rmovd),

  P_OP(ret),

  P_OP(label),

  P_OP(fp),  // Frame pointer pseudo operation.
  P_OP(sp),  // Stack pointer pseudo operation.
  P_OP(tp),   // Thread pointer.

  // Function result registers.
  P_OP(resulti),
  P_OP(resultf),
  P_OP(resultd),

  P_OP(structreturn),  // Struct return address.

  P_OP(asm),

  P_OP(loc),
  P_OP(named_label),
  P_OP(ivarreg),
  P_OP(fvarreg),

  // End of TargetOpcode enumeration.

  // Now follow PCode opcodes.
  P_OP(ap),  // Argument pointer pseudo operation.
  
  // Stack manipulation
  P_OP(decsp),    // Decrement stack pointer by constant.
  P_OP(incsp),    // Increment stack pointer by constant.

  P_OP(push),     // Push 32 bit integer onto stack.
  P_OP(pushf),    // Push single precision floating point.
  P_OP(pushd),    // Push double precision floating point.
  P_OP(pushx),    // Push 64 bit integer onto stack.
  P_OP(pop),      // Pop 32 bit integer.
  P_OP(popf),     // Pop single precision float.
  P_OP(popd),     // Pop double precision float.
  P_OP(popx),     // Pop 64 bit integer.

  // Loads: ld d,[r, #c].
  P_OP(ldw),      // Load signed 32 bit word.
  P_OP(ldh),      // Load signed 16 bit word.
  P_OP(ldb),      // Load signed byte.
  P_OP(lduw),     // Load unsigned 32 bit word.
  P_OP(ldub),     // Load unsigned byte.
  P_OP(lduh),     // Load unsigned 16 bit word.
  P_OP(ldx),      // Load 64 bit word.
  P_OP(ldf),      // Load single precision float.
  P_OP(ldd),      // Load double precision float.

  // Stores: st r,[d, #c]
  P_OP(stw),      // Store 32 bit.
  P_OP(sth),      // Store 16 bit.
  P_OP(stx),      // Store 64 bit.
  P_OP(stf),      // Store single precision float.
  P_OP(std),      // Store double precision float.
  P_OP(stb),      // Store byte.

  // Add.
  P_OP(add),      // Add int.
  P_OP(addf),     // Add float.
  P_OP(addd),     // Add double.
  P_OP(addc),     // Add with constant.

  // Subtract.
  P_OP(sub),      // Subtract int.
  P_OP(subf),     // Subtract float.
  P_OP(subd),     // Subtract double.

  // Multiply.
  P_OP(mul),      // Multiply int.
  P_OP(mulf),     // Multiply float.
  P_OP(muld),     // Multiply double.

  // Divide.
  P_OP(div),      // Divide int.
  P_OP(divu),     // Divide int unsigned
  P_OP(divf),     // Divide float.
  P_OP(divd),     // Divide double.

  // Modulus.
  P_OP(mod),      // Integer modulus.
  P_OP(modu),     // Unsigned nteger modulus.

  // Shifts.
  P_OP(lsr),      // Logical shift right.
  P_OP(asr),      // Arithmetic shift right.
  P_OP(lsl),      // Logical shift left.

  // Bitwise.
  P_OP(or),       // OR.
  P_OP(and),      // AND.
  P_OP(xor),      // Exclusive OR.

  P_OP(not),      //  != 0 -> 0
  P_OP(inv),      // Ones complement.
  P_OP(neg),      // Negate int.
  P_OP(negf),     // Negate float.
  P_OP(negd),     // Negate double.

  // Compares.
  P_OP(cmpeq),     // == int.
  P_OP(cmpne),     // != int.
  P_OP(cmplt),     // < int.
  P_OP(cmple),     // <= int.
  P_OP(cmpgt),     // > int.
  P_OP(cmpge),     // >= int.
  P_OP(cmpltu),    // < unsigned int.
  P_OP(cmpleu),    // <= unsigned int.
  P_OP(cmpgtu),    // > unsigned int.
  P_OP(cmpgeu),    // >= unsigned int.

  P_OP(cmpeqf),    // == float.
  P_OP(cmpnef),    // != float.
  P_OP(cmpltf),    // < float.
  P_OP(cmplef),    // <= float.
  P_OP(cmpgtf),    // > float.
  P_OP(cmpgef),    // >= float.

  P_OP(cmpeqd),    // == double.
  P_OP(cmpned),    // != double.
  P_OP(cmpltd),    // < double.
  P_OP(cmpled),    // <= double.
  P_OP(cmpgtd),    // > double.
  P_OP(cmpged),    // >= double.

  // C++20 three-way comparison: result is -1 / 0 / 1 (2 = unordered floats).
  P_OP(cmp3way),    // signed int three-way.
  P_OP(cmp3wayu),   // unsigned int three-way.
  P_OP(cmp3wayf),   // float three-way (2 = unordered).
  P_OP(cmp3wayd),   // double three-way (2 = unordered).

  // Relative branches.
  P_OP(bnz),       // Branch if non-zero.
  P_OP(bz),        // Branch if zero
  P_OP(bra),       // Unconditional branch,

  // Computed branch.  Adds reg value to current PC and branches
  // to the calculated address.
  P_OP(cbra),

  P_OP(i2f),      // int to float.
  P_OP(i2d),      // int to double.
  P_OP(ui2f),     // unsigned int to float.
  P_OP(ui2d),     // unsigned int to double.
  P_OP(f2d),      // float to double.
  P_OP(d2f),      // double to float.
  P_OP(f2i),      // float to int.
  P_OP(d2i),      // double to int.
  P_OP(f2ui),     // float to unsigned int.
  P_OP(d2ui),     // double to unsigned int.

  P_OP(jmp),      // Jump to address.
  P_OP(cjmp),     // Jump to contents of address.
  P_OP(adr),      // PC relative address.
  P_OP(adrs),     // PC relative address of a string literal.
  P_OP(adrtls),      // PC relative TLS address.

  // Call and return.
  P_OP(call),     // Call address with int result.
  P_OP(callf),    // Call address with float result.
  P_OP(calld),    // Call address with double result.
  P_OP(rcall),    // Call register with int resutl.
  P_OP(rcallf),   // Call register with float result.
  P_OP(rcalld),   // Call register with double result.
} PCodeOpcode;

// A PCode Generator is derived from a TargetGenerator.  It has
// a '.base' field that is the TargetGenerator.
typedef struct PCodeGenerator {
  TargetGenerator base;

  TargetInstruction* argument_pointer;

  // Register allocator.
  PCodeRegisterAllocator register_allocator;
} PCodeGenerator;

void PCodeGeneratorInit(PCodeGenerator* pcode, Generator* gen);
PCodeGenerator* NewPCodeGenerator(Generator* gen);

void PCodeGeneratorDestruct(PCodeGenerator* pcode);
void PCodeGeneratorDelete(PCodeGenerator* pcode);

// Lower the IR to PCode.
void PCodeLower(PCodeGenerator* pcode, Generator* gen);
void PCodePrint(PCodeGenerator* pcode);

const char* PCodeOpcodeName(int op);
bool PCodeIsExpression(TargetInstruction* inst);
bool PCodeIsSignedLoad(TargetInstruction* inst);

#endif /* p_code_h */
