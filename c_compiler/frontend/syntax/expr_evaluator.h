//
//  expr_evaluator.h
//  c_compiler
//
//  Created by David Allison on 11/3/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef expr_evaluator_h
#define expr_evaluator_h

#include "constexpr.h"

bool EvaluateIntegerExpression(ASTNode* node, int64_t* result);
bool EvaluateFloatingPointExpression(ASTNode* node, double* result);
bool EvaluateScalarConstantForSymbol(Symbol* symbol, ASTNode* initializer);

#endif /* expr_evaluator_h */
