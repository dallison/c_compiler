//
//  wasm32_reg_alloc.h
//  c_compiler
//
//  Wasm locals are an unbounded typed array, so "register allocation" is
//  just handing every value that needs to outlive the operand stack its own
//  local.  There is no spilling and no interference graph.
//

#ifndef wasm32_reg_alloc_h
#define wasm32_reg_alloc_h

#include "wasm32_codegen.h"

// Assign a local index to every instruction that produces a value.
// Parameters occupy locals 0..num_params-1, so allocated locals start after
// them and are grouped by value type as the binary format requires.
void Wasm32AllocateLocals(Wasm32Generator* wasm);

// The local index an instruction's value lives in, or -1 if it has none.
int Wasm32LocalIndex(TargetInstruction* inst);

#endif /* wasm32_reg_alloc_h */
