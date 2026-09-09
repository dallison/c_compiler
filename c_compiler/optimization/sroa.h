#ifndef sroa_h
#define sroa_h

#include "codegen.h"

// Split small local structs and fixed arrays whose only uses are
// constant-offset loads and stores into scalar temporaries.  Must run
// before SSA so the new scalars receive phis and can be register allocated.
void ScalarReplacementOptimization(Generator* gen);

#endif
