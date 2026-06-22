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
        // No references to an expression, remove it unless it writes into an
        // explicit destination (for example a conditional-expression merge
        // temporary).
        if (inst->dest == NULL) {
          TargetDeleteInstruction(&pcode->base, inst);
        }
      }
    } else if ((PCodeOpcode)((int)inst->opcode == (int)P_OP(decsp)) ||
              (PCodeOpcode)((int)inst->opcode == (int)P_OP(incsp))) {
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
