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

// Resolve a call whose callee names an overloaded function template, once its
// arguments are concrete (used when instantiating a cloned template body).
// Returns the best concrete instantiation, or NULL if none is viable.
Symbol* CXXResolveOverloadedFunctionTemplateCall(Symbol* callee,
                                                 Vector* explicit_args,
                                                 Vector* actuals);

// Attempts to convert `from` to the class type `to` by constructing a temporary
// through a viable converting constructor, splicing the result in place of
// `from`.  Returns true if the conversion was performed.
bool TryConvertWithConvertingConstructor(ASTNode* from, TypeRecord* to,
                                         ConversionContext ctx);

// Finds a viable converting constructor for overload/constexpr conversion.
StructMember* CXXFindConvertingConstructorCandidate(TypeRecord* to, ASTNode* from,
                                                    bool allow_explicit);

#endif /* expr_semantics_h */
