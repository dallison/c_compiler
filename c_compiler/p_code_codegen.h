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
  P_OP(constb),
  P_OP(consth),
  P_OP(constw),
  P_OP(constx),
  P_OP(constf),
  P_OP(constd),

  P_OP(mov),
  P_OP(movf),
  P_OP(movd),

  P_OP(movc),
  P_OP(movfc),
  P_OP(movdc),
  P_OP(movxc),

  P_OP(rmov),
  P_OP(rmovf),
  P_OP(rmovd),

  P_OP(ret),

  P_OP(label),

  P_OP(fp),  // Frame pointer pseudo operation.
  P_OP(sp),  // Stack pointer pseudo operation.

  // Function result registers.
  P_OP(resultx),
  P_OP(resultf),
  P_OP(resultd),

  P_OP(structreturn),  // Struct return address.

  P_OP(asm),

  P_OP(loc),

  // End of TargetOpcode enumeration.

  // Now follow PCode opcodes.
  P_OP(ap),  // Argument pointer pseudo operation.

  // Stack manipulation
  P_OP(decsp),
  P_OP(incsp),

  P_OP(push),
  P_OP(pushf),
  P_OP(pushd),
  P_OP(pushx),
  P_OP(pop),
  P_OP(popf),
  P_OP(popd),
  P_OP(popx),

  // Loads: ld d,[r, #c].
  P_OP(ldw),
  P_OP(ldh),
  P_OP(ldb),
  P_OP(lduw),
  P_OP(ldub),
  P_OP(lduh),
  P_OP(ldx),
  P_OP(ldf),
  P_OP(ldd),

  // Stores: st r,[d, #c]
  P_OP(stw),
  P_OP(sth),
  P_OP(stx),
  P_OP(stf),
  P_OP(std),
  P_OP(stb),

  // Add.
  P_OP(add),
  P_OP(addf),
  P_OP(addd),
  P_OP(addc),  // Add with constant.

  // Subtract.
  P_OP(sub),
  P_OP(subf),
  P_OP(subd),

  // Multiply.
  P_OP(mul),
  P_OP(mulf),
  P_OP(muld),

  // Divide.
  P_OP(div),
  P_OP(divf),
  P_OP(divd),

  // Modulus.
  P_OP(mod),

  // Shifts.
  P_OP(lsr),
  P_OP(asr),
  P_OP(lsl),

  // Bitwise.
  P_OP(or),
  P_OP(and),
  P_OP(xor),

  P_OP(not),
  P_OP(inv),
  P_OP(neg),
  P_OP(negf),
  P_OP(negd),

  // Compares.
  P_OP(cmpeq),
  P_OP(cmpne),
  P_OP(cmplt),
  P_OP(cmple),
  P_OP(cmpgt),
  P_OP(cmpge),

  P_OP(cmpeqf),
  P_OP(cmpnef),
  P_OP(cmpltf),
  P_OP(cmplef),
  P_OP(cmpgtf),
  P_OP(cmpgef),

  P_OP(cmpeqd),
  P_OP(cmpned),
  P_OP(cmpltd),
  P_OP(cmpled),
  P_OP(cmpgtd),
  P_OP(cmpged),

  // Relative branches.
  P_OP(bnz),
  P_OP(bz),
  P_OP(bra),

  // Computed branch.  Adds reg value to current PC and branches
  // to the calculated address.
  P_OP(cbra),

  P_OP(i2f),
  P_OP(i2d),
  P_OP(f2d),
  P_OP(d2f),
  P_OP(f2i),
  P_OP(d2i),

  // Absolute jump to register value.
  P_OP(jmp),

  // Call and return.
  P_OP(call),
  P_OP(callf),
  P_OP(calld),
  P_OP(rcall),
  P_OP(rcallf),
  P_OP(rcalld),
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
bool PCodeIsExpression(PCodeOpcode opcode);
bool PCodeIsSignedLoad(PCodeOpcode opcode);

#endif /* p_code_h */
