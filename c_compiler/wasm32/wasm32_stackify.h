//
//  wasm32_stackify.h
//  c_compiler
//
//  Turning a control flow graph back into wasm's structured control flow.
//

#ifndef wasm32_stackify_h
#define wasm32_stackify_h

#include "wasm32_codegen.h"

// Rewrite the lowered instruction list so that all control flow is
// expressed with wasm's nested block, loop and if scopes and relative
// branch depths.  Runs after lowering and before locals are assigned.
void Wasm32Stackify(Wasm32Generator* wasm);

#endif /* wasm32_stackify_h */
