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
int CompareFunctionTemplateSpecificity(Symbol* left, Symbol* right);

// Analyzes `func`'s body so that a placeholder (`auto` / `decltype(auto)`)
// return type is replaced by the deduced one.  A no-op for anything else.
// Call before a use that bakes the function's signature into another type.
void SemanticEnsureAutoReturnTypeDeduced(TypeRecord* func);

// Resolve a call whose callee names an overloaded function template, once its
// arguments are concrete (used when instantiating a cloned template body).
// Returns the best concrete instantiation, or NULL if none is viable.
Symbol* CXXResolveOverloadedFunctionTemplateCall(Symbol* callee,
                                                 Vector* explicit_args,
                                                 Vector* actuals);

// [over.over]/[temp.deduct.funcaddr]: resolve a (possibly overloaded) function
// name -- overload chain head `head`, optional explicit template arguments
// `explicit_args` -- used where a specific function type `target_fn` is required
// (the pointee of a destination function-pointer type).  Returns the unique
// matching concrete function (a non-template overload, or an instantiated
// function-template specialization), or NULL if there is no unique match.
Symbol* CXXResolveFunctionAddressForTargetType(Symbol* head,
                                               Vector* explicit_args,
                                               TypeRecord* target_fn);

// Node-level [over.over] helper: if `from` names a function template or
// overload set and `to` is a pointer-to-function (or reference/function type in
// a binding context), rewrite `from` in place to the unique matching concrete
// function.  Returns true on a successful rewrite, false otherwise.
bool CXXTryResolveFunctionAddressNode(ASTNode* from, TypeRecord* to);

// Attempts to convert `from` to the class type `to` by constructing a temporary
// through a viable converting constructor, splicing the result in place of
// `from`.  Returns true if the conversion was performed.
bool TryConvertWithConvertingConstructor(ASTNode* from, TypeRecord* to,
                                         ConversionContext ctx);

// Finds a viable converting constructor for overload/constexpr conversion.
StructMember* CXXFindConvertingConstructorCandidate(TypeRecord* to, ASTNode* from,
                                                    bool allow_explicit);

// Validate class copy-initialization of a named return operand, including the
// constructor that must remain viable when NRVO later elides the call.
void CXXValidateReturnInitialization(TypeRecord* to, ASTNode* from);

// Lower a bare braced-init-list used as an expression (function argument,
// return value or assignment right-hand side) into a temporary of `target`
// type initialized by the braces.  Returns the analyzed compound-literal
// expression, or `braced` unchanged if it is not a braced-init-list.  The
// braced node is re-parented into the result, so splice it in with
// delete_old_child = false.
ASTNode* LowerCXXBracedInitToTarget(ASTNode* braced, TypeRecord* target);

#endif /* expr_semantics_h */
