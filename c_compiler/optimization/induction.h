//
//  induction.h
//  c_compiler
//

#ifndef induction_h
#define induction_h

#include "codegen.h"

// Recognize derived address induction before SSA construction so the normal
// rename/phi machinery can place the optimizer-created pointer variable.
void DerivedInductionVariableOptimization(Generator* gen);

void InductionVariableOptimization(Generator* gen);

#endif /* induction_h */
