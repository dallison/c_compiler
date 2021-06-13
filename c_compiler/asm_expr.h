//
//  asm_expr.h
//  c_compiler
//
//  Created by David Allison on 10/5/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef asm_expr_h
#define asm_expr_h

#include "assembler.h"

bool AssemblerEvaluateExpressionInternal(Assembler* assembler, int64_t* result);

#endif /* asm_expr_h */
