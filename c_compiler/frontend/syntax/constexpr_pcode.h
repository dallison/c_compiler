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
// C++26 object typing.  Each carries a ConstexprPCodeTypeToken.
#define CONSTEXPR_PCODE_OBJECT_MARKER UINT64_C(0xfffffffc)
#define CONSTEXPR_PCODE_ALLOCATED_OBJECT_MARKER UINT64_C(0xfffffffb)
#define CONSTEXPR_PCODE_VOID_POINTER_CAST_MARKER UINT64_C(0xfffffffa)
#define CONSTEXPR_PCODE_PLACEMENT_NEW_MARKER UINT64_C(0xfffffff9)

typedef enum {
  kConstexprPCodeEligible,
  kConstexprPCodeRequiresOverlay,
  kConstexprPCodeASTOnly,
} ConstexprPCodeCapability;

bool ConstexprPCodeValidateCall(ASTNode* node, const char** reason);
uint64_t ConstexprPCodeTypeToken(TypeRecord* type);
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
const char* ConstexprPCodeCStringAt(uint64_t address);

#endif /* constexpr_pcode_h */
