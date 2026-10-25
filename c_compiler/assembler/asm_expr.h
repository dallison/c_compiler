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
// Parses an assembler expression and returns it.  The caller owns the node.
ASTNode* AssemblerCaptureExpression(Assembler* assembler);
void AssemblerReleaseExpression(void* expression);
void AssemblerSetEqvExpression(AssemblerSymbol* symbol, ASTNode* expression);

#endif /* asm_expr_h */
