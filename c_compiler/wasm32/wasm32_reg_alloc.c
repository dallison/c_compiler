//
//  wasm32_reg_alloc.c
//  c_compiler
//

#include "wasm32_reg_alloc.h"

#include <stdlib.h>

int Wasm32LocalIndex(TargetInstruction* inst) {
  if (inst == NULL || inst->reg == NULL) {
    return -1;
  }
  return inst->reg->num;
}

// The local declaration vector groups locals by type, so a value's final
// index is only known once every type's population is counted.  The first
// pass records a per-type ordinal and the second turns it into an index.
void Wasm32AllocateLocals(Wasm32Generator* wasm) {
  for (int i = 0; i < WASM32_NUM_VALUE_TYPES; i++) {
    wasm->num_locals[i] = 0;
  }
  VectorClear(&wasm->local_values);

  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    // Parameters already own locals 0..num_params-1, and an instruction the
    // optimizer removed has no value left for anything to read.
    bool needs_local = (inst->flags & TARGET_INST_DEAD) == 0 &&
                       (inst->opcode == (TargetOpcode)W_OP(slot) ||
                        (Wasm32ProducesValue(inst) && inst->dest == NULL));
    if (needs_local) {
      int type = Wasm32TypeIndex(Wasm32InstructionType(inst));
      TargetRegister* reg = malloc(sizeof(TargetRegister));
      TargetRegisterInit(reg, wasm->num_locals[type]);
      reg->owner = inst;
      inst->reg = reg;
      wasm->num_locals[type]++;
      VectorAppend(&wasm->local_values, inst);
    }
    inst = TargetNext(inst);
  }

  int base = wasm->num_params;
  for (int i = 0; i < WASM32_NUM_VALUE_TYPES; i++) {
    wasm->type_base[i] = base;
    base += wasm->num_locals[i];
  }

  for (size_t i = 0; i < wasm->local_values.length; i++) {
    TargetInstruction* value = wasm->local_values.value.p[i];
    int type = Wasm32TypeIndex(Wasm32InstructionType(value));
    value->reg->num += wasm->type_base[type];
  }
}
