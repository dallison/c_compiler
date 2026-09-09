#ifndef unroll_h
#define unroll_h

#include "codegen.h"

// Completely unroll innermost counted loops whose trip count is a small
// positive constant.  Intended for -O3 after SSA has been removed: the body
// is cloned into the preheader with the induction variable substituted for
// constants, then the original loop is deleted.
void LoopUnrollOptimization(Generator* gen);

#endif
