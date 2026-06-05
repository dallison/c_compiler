//
//  expr_semantics.h
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef expr_semantics_h
#define expr_semantics_h

#include "semantics.h"
#include "syntax.h"

__attribute__((warn_unused_result)) ASTNode* AnalyzeExpression(ASTNode* node);
bool IsConstantExpression(ASTNode* node);

#endif /* expr_semantics_h */
