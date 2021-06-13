//
//  constprop.h
//  c_compiler_library
//
//  Created by David Allison on 7/4/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

// Constant propagation and folding optimization (O2)
// This determines that a variable has a constant value and propagates that
// constant to all uses of that variable.  Using a constant is cheaper than
// using a variable to this will improve the runtime speed and also save
// registers use for variables.
//
// This also looks for instructions that have constant operands and
// evaluates them.  While there are some constant expressions that can be
// evaluated in the front-end, there are others that are generated during
// code generation (address and index calculations, for example).  Also,
// this has more information available about the control flow and can
// determine that variables have constant values.
//
// This relies on the SSA form of the CFG.  More magical abilities of SSA.
//
// It only does integer propagation and folding since this is a cross
// compiler and the behavior of the target's floating point support might
// be different from the compiler's.
#ifndef constprop_h
#define constprop_h

#include "codegen.h"

void ConstantPropagationOptimization(Generator* gen);

#endif /* constprop_h */
