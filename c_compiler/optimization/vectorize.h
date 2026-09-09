#ifndef vectorize_h
#define vectorize_h

#include "codegen.h"

// Rewrite innermost constant-trip counted loops of independent unit-stride
// loads and a binary operator into native vector IR, and pack adjacent
// isomorphic scalar ops in a block (SLP).  Intended for -O2 after SSA has
// been removed, on targets that keep 16-byte SIMD operations.
void AutoVectorizeOptimization(Generator* gen);

#endif
