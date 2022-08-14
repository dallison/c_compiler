//
//  p_code_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 5/19/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "p_code_optimize.h"
#include <assert.h>
#include "p_code_codegen.h"

// Perform some simple peephole optimizations for the P-CODE code
// sequence.  This looks at the P-CODE instructions to see if there
// are things we can simplify or remove.

// Remove any expressions that have no references.
static void RemoveUnusedExpressions(PCodeGenerator* pcode) {
  // Go backwards through the code so that if we remove an instruction, anything
  // that it references will be processed after it.
  TargetInstruction* inst = TargetLastInstruction(&pcode->base);
  while (inst != NULL) {
    TargetInstruction* prev = TargetPrev(inst);
    assert(inst != prev);
    if (PCodeIsExpression(inst)) {
      if (inst->users.length == 0) {
        // No references to an expression, remove it.
        TargetDeleteInstruction(&pcode->base, inst);
      }
    } else if (inst->opcode == P_OP(mov) && inst->dest != NULL) {
      // A mov can be eliminated if it has zero references and
      // its dest is a tmp with one reference.
      TargetInstruction* dest = inst->dest;
      if (inst->users.length == 0 && dest->opcode == P_OP(tmp) &&
          dest->users.length == 1) {
        TargetDeleteInstruction(&pcode->base, inst);
      }
    } else if ((PCodeOpcode)inst->opcode == P_OP(decsp) ||
              (PCodeOpcode)inst->opcode == P_OP(incsp)) {
      // Incsp or Descp with zero bytes can go away.
      TargetConstant* size = (TargetConstant*)inst->operand[0];
      if (size->value.ivalue == 0) {
        TargetDeleteInstruction(&pcode->base, inst);
      }
      
    }
    inst = prev;
  }
}

void PCodeOptimize(PCodeGenerator* pcode) {
  // Remove all unused expressions.
  RemoveUnusedExpressions(pcode);
}
