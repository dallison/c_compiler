//
//  wasm32_optimize.h
//  c_compiler
//

#ifndef wasm32_optimize_h
#define wasm32_optimize_h

#include "wasm32_codegen.h"

// Peephole cleanups run on the lowered wasm instructions before locals are
// assigned.  Dead instructions are marked rather than unlinked so the
// branch fixups that already point at them stay valid.
void Wasm32Optimize(Wasm32Generator* wasm);

// The same cleanups again once control flow has been structured.  Stackifying
// writes the dispatch selector through a temporary of its own, which is
// exactly the shape the copy forwarding exists to collapse, and it is only
// there to be found after the fact.
void Wasm32OptimizeStackified(Wasm32Generator* wasm);

#endif /* wasm32_optimize_h */
