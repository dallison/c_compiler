#ifndef dce_h
#define dce_h

#include "codegen.h"

// Remove unused, side-effect-free IR expressions while use/def edges are
// still available.
void DeadCodeEliminationOptimization(Generator* gen);

#endif
