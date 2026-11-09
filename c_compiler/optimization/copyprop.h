#ifndef copyprop_h
#define copyprop_h

#include "codegen.h"

// Eliminate conservative, single-use IR move copies.  Cross-block copies are
// rewritten only when the source dominates the user and the function has no
// exception landing pads.
void CopyPropagationOptimization(Generator* gen);

// In SSA form, redirect reads of a variable copied from a symbol's only
// version to that version, and drop definitions that nothing reads.
void SSAVariableCopyPropagation(Generator* gen);

#endif
