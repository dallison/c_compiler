//
//  constexpr_pcode.h
//  c_compiler
//

#ifndef constexpr_pcode_h
#define constexpr_pcode_h

#include "constexpr.h"

// Lifetime markers travel through the source target's calling convention.
// Keep them representable on both 32-bit and 64-bit targets.
#define CONSTEXPR_PCODE_UNION_MEMBER_ADDRESS_MARKER UINT64_C(0xffffffff)
#define CONSTEXPR_PCODE_LIFETIME_END_MARKER UINT64_C(0xfffffffe)
#define CONSTEXPR_PCODE_LIFETIME_CONSTRUCTION_MARKER UINT64_C(0xfffffffd)

typedef enum {
  kConstexprPCodeEligible,
  kConstexprPCodeRequiresOverlay,
  kConstexprPCodeASTOnly,
} ConstexprPCodeCapability;

bool ConstexprPCodeValidateCall(ASTNode* node, const char** reason);
ConstexprPCodeCapability ConstexprPCodeCapabilityForExpression(ASTNode* node);
ConstexprPCodeCapability ConstexprPCodeCapabilityForFunction(Symbol* function);
bool ConstexprPCodeFunctionContainsThrow(Symbol* function);
bool ConstexprPCodeRequiresASTOverlay(ASTNode* node);
bool ConstexprPCodeAutoShouldAttempt(ASTNode* node);
void ConstexprPCodeAutoRecordASTEvaluation(ASTNode* node, int steps);
const char* ConstexprPCodeFailureReason(ConstEvalContext* ctx);
bool ConstexprPCodeEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                         int64_t* result);
bool ConstexprPCodeEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                          double* result);
bool ConstexprPCodeEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node);
bool ConstexprPCodeEvaluateCall(ConstEvalContext* ctx, ASTNode* node);
bool ConstexprPCodeEvaluateCallObjectResult(ConstEvalContext* ctx,
                                            ASTNode* node,
                                            ConstexprObject** result);
void ConstexprPCodeDeleteObject(ConstexprObject* object);
bool ConstexprPCodeEvaluateCallAsAddress(ConstEvalContext* ctx, ASTNode* node,
                                         ConstexprValue* result);
bool ConstexprPCodeEvaluateObjectConstantForSymbol(ConstEvalContext* ctx,
                                                   Symbol* symbol,
                                                   ASTNode* initializer);
void ConstexprPCodeClearImageCache(void);

#endif /* constexpr_pcode_h */
