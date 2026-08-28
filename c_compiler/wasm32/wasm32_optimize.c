//
//  wasm32_optimize.c
//  c_compiler
//

#include "wasm32_optimize.h"

// The next instruction that will actually be encoded, skipping pseudo
// operations and anything already marked dead.
static TargetInstruction* NextRealInstruction(TargetInstruction* inst) {
  TargetInstruction* next = TargetNext(inst);
  while (next != NULL) {
    if ((next->flags & TARGET_INST_DEAD) == 0 && !Wasm32IsPseudo(next)) {
      return next;
    }
    next = TargetNext(next);
  }
  return NULL;
}

// A branch whose target label is the very next thing in the instruction
// stream is a fallthrough and encodes to nothing.
static void RemoveFallthroughBranches(Wasm32Generator* wasm) {
  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    if (inst->opcode == (TargetOpcode)W_OP(br)) {
      TargetInstruction* target = inst->operand[1];
      if (target != NULL && NextRealInstruction(inst) == target) {
        inst->flags |= TARGET_INST_DEAD;
      }
    }
    inst = TargetNext(inst);
  }
}

void Wasm32Optimize(Wasm32Generator* wasm) {
  RemoveFallthroughBranches(wasm);
}
