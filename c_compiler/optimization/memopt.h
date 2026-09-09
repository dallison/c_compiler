#ifndef memopt_h
#define memopt_h

#include "codegen.h"

// Block-local store-to-load forwarding, load CSE, and dead-store elimination
// using object-based alias analysis.  Does not follow values through
// arbitrary pointer variables; those addresses do not decode to a base object.
void MemoryOptimization(Generator* gen);

#endif
