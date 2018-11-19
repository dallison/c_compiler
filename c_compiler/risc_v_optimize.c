//
//  risc_v_optimize.c
//  c_compiler_library
//
//  Created by David Allison on 4/18/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include "risc_v_optimize.h"
#include <assert.h>
#include "risc_v_codegen.h"

// This file contains functions to optimize the instruction sequence for RISC-V.
// Now that we have lowered the IR to actual RISC-V instructions we can look for
// sequences that can be made more optimal given the details of the instruction
// set.
//
// A requirement here is all the optimization be safe so that the output code is
// as correct as the input code.

// Remove any expressions that have no references.
static void RemoveUnusedExpressions(RVGenerator* rv) {
  // Go backwards through the code so that if we remove an instruction, anything
  // that it references will be processed after it.
  TargetInstruction* inst = TargetLastInstruction(&rv->base);
  while (inst != NULL) {
    TargetInstruction* prev = TargetPrev(inst);
    assert(inst != prev);
    if (RVIsExpression((RVOpcode)inst->opcode)) {
      if (inst->refs == 0) {
        // No references to an expression, remove it.
        TargetDeleteInstruction(&rv->base, inst);
      }
    } else if (inst->opcode == RV_OP(rmov)) {
      // An rmov can be eliminated if it has zero references and
      // its first operand is a tmp with one reference.
      TargetInstruction* src = inst->operand[0];
      if (inst->refs == 0 && src != NULL && src->opcode == RV_OP(tmp) &&
          src->refs == 1) {
        TargetDeleteInstruction(&rv->base, inst);
      }
    }
    inst = prev;
  }
}

// The output from the codegen for loading and storing symbols is this:
// la x, symbol_name
// lw y, 0(x)  (any other load or store)
//
// The 'la' instruction is a macro for loading the symbol address
// via a relocation and expands to:
// auipc x, %pcrel_hi(symbol_name)
// addi x, x, %pcrel_lo(symbol_name)
// lw y, 0(x)
//
// The addi can be combined with the load(store):
// auipc x, %pcrel_hi(symbol_name)
// lw y, %pcrel_lo(symbol_name), x
//
//
// Also, the sequence:
// addi x1, x2, n
// ld x, a(x1)  - any load or store
//
// can become:
// ld x, (n+a)(x2)
// as long as n+a is a possible immediate value.

static void CombineLoadOrStores(RVGenerator* rv) {
  TargetInstruction* inst = TargetFirstInstruction(&rv->base);
  while (inst != NULL) {
    TargetInstruction* next = TargetNext(inst);
    if (RVIsLoad((RVOpcode)inst->opcode)) {
      TargetInstruction* base = inst->operand[0];
      if (base->opcode == (TargetOpcode)RV_OP(la)) {
        // Insert label for auipc instruction.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)RV_OP(label));
        label->flags = RV_EXPORTED_LABEL;
        TargetEmitBefore(&rv->base, label, base);
        base->opcode = (TargetOpcode)RV_OP(auipc);
        base->flags |= RV_PCREL_HI_RELOC;
        inst->operand[1]->refs--;
        inst->operand[1] = label;
        inst->operand[1]->refs++;
        inst->flags |= RV_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)RV_OP(addi)) {
        // Load from an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = RVIntValue(inst->operand[1]);
        int immed = RVIntValue(base->operand[1]);
        if (RVIsPossibleImmediate(offset + immed)) {
          base->refs--;
          base->operand[0]->refs++;
          inst->operand[0] = base->operand[0];
          inst->operand[1] = TargetGetIntConstant(
              &rv->base, NULL, kTargetTypeWord, offset + immed);
        }
      }
    } else if (RVIsStore((RVOpcode)inst->opcode)) {
      TargetInstruction* base = inst->operand[1];
      if (base->opcode == (TargetOpcode)RV_OP(la)) {
        // Insert label for auipc instruction.
        TargetInstruction* label =
            TargetNewInstruction((TargetOpcode)RV_OP(label));
        label->flags = RV_EXPORTED_LABEL;
        TargetEmitBefore(&rv->base, label, base);
        base->opcode = (TargetOpcode)RV_OP(auipc);
        base->flags |= RV_PCREL_HI_RELOC;
        inst->operand[2]->refs--;
        inst->operand[2] = label;
        inst->operand[2]->refs++;
        inst->flags |= RV_PCREL_LO_RELOC;
      } else if (base->opcode == (TargetOpcode)RV_OP(addi)) {
        // Store to an address calculated using an addi instruction.  See if we
        // can combine them.
        int offset = RVIntValue(inst->operand[2]);
        int immed = RVIntValue(base->operand[1]);
        if (RVIsPossibleImmediate(offset + immed)) {
          base->refs--;
          base->operand[0]->refs++;
          inst->operand[1] = base->operand[0];
          inst->operand[2] = TargetGetIntConstant(
              &rv->base, NULL, kTargetTypeWord, offset + immed);
        }
      }
    }
    inst = next;
  }
}

// Look for instructions that have an operand that is a single-use mv from x0.
// For these, replace the operand with x0.
//
// For example:
// mv t0, x0
// sb t0, 0(t2)
//
// is replaced by:
// sb x0, 0(t2)
// (the mv instruction is eliminated).

static void PropagateZeroes(RVGenerator* rv) {
  TargetInstruction* inst = TargetLastInstruction(&rv->base);
  while (inst != NULL) {
    TargetInstruction* prev = TargetPrev(inst);
    for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
      TargetInstruction* operand = inst->operand[i];
      if (operand != NULL) {
        if (operand->opcode == RV_OP(mv) && operand->refs == 1) {
          TargetInstruction* mv = operand;
          if (mv->operand[0]->opcode == (TargetOpcode)RV_OP(x0)) {
            // Found mv xx, x0.  Replace instruction operand with x0.
            inst->operand[i] = mv->operand[0];

            // Another reference to x0.
            mv->operand[0]->refs++;

            // We can now eliminate the mv instruction.
            TargetDeleteInstruction(&rv->base, mv);
          }
        }
      }
    }
    inst = prev;
  }
}

void RVOptimize(RVGenerator* rv) {
  // Remove all unused expressions.
  RemoveUnusedExpressions(rv);

  // A load or store from an address calculated using the 'la' instruction can
  // be combined with the 'la'.
  CombineLoadOrStores(rv);

  // x0 is always available as a source register.  Look for single-use mv
  // instructions from x0 and propagate x0 to the destination.
  PropagateZeroes(rv);

  // Remove all newly unused expressions due to optimizations.
  RemoveUnusedExpressions(rv);
}
