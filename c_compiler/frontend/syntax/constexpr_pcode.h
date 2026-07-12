//
//  constexpr_pcode.h
//  c_compiler
//

#ifndef constexpr_pcode_h
#define constexpr_pcode_h

#include "constexpr.h"

bool ConstexprPCodeValidateCall(ASTNode* node, const char** reason);
bool ConstexprPCodeEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                         int64_t* result);
bool ConstexprPCodeEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                          double* result);
bool ConstexprPCodeEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node);
bool ConstexprPCodeEvaluateCallObjectResult(ConstEvalContext* ctx,
                                            ASTNode* node,
                                            ConstexprObject** result);
void ConstexprPCodeDeleteObject(ConstexprObject* object);
bool ConstexprPCodeEvaluateCallAsAddress(ConstEvalContext* ctx, ASTNode* node,
                                         ConstexprValue* result);
bool ConstexprPCodeEvaluateObjectConstantForSymbol(Symbol* symbol,
                                                   ASTNode* initializer);
void ConstexprPCodeClearImageCache(void);

#endif /* constexpr_pcode_h */
