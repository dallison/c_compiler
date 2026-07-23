#ifndef copyprop_h
#define copyprop_h

#include "codegen.h"

// Eliminate conservative, single-use IR move copies without extending a
// source value across basic-block boundaries.
void CopyPropagationOptimization(Generator* gen);

#endif
