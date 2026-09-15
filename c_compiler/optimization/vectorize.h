#ifndef vectorize_h
#define vectorize_h

#include "codegen.h"

// Rewrite innermost constant-trip counted loops of independent unit-stride
// loads and a binary operator into vector IR, and pack adjacent isomorphic
// scalar ops in a block (SLP).  Intended for -O2 after SSA has been removed.
// x86-64 and AArch64 keep a native SIMD subset; other backends software-expand
// the same IR before lowering.  8-bit targets skip this pass.
void AutoVectorizeOptimization(Generator* gen);

#endif
