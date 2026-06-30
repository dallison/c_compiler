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

// Attempts to convert `from` to the class type `to` by constructing a temporary
// through a viable converting constructor, splicing the result in place of
// `from`.  Returns true if the conversion was performed.
bool TryConvertWithConvertingConstructor(ASTNode* from, TypeRecord* to,
                                         ConversionContext ctx);

#endif /* expr_semantics_h */
