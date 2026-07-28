//
//  constexpr.h
//  c_compiler
//

#ifndef constexpr_h
#define constexpr_h

#include "syntax.h"

typedef struct ConstexprValue ConstexprValue;
typedef struct ConstexprObject ConstexprObject;

typedef struct {
  Vector bindings;  // ConstexprBinding*
  Vector objects;   // ConstexprObject*
  int call_depth;
  int steps;
  int max_steps;
} ConstEvalContext;

void ConstEvalContextInit(ConstEvalContext* ctx);
void ConstEvalContextDestruct(ConstEvalContext* ctx);
bool ConstEvalStep(ConstEvalContext* ctx);

ASTNode* ConstexprInitializerExpression(ASTNode* initializer);

bool EvaluateIntegerExpressionInContext(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result);
bool EvaluateFloatingPointExpressionInContext(ConstEvalContext* ctx,
                                              ASTNode* node, double* result);

bool ConstexprBindingAsInteger(ConstEvalContext* ctx, Symbol* symbol,
                               int64_t* result);
bool ConstexprBindingAsFloating(ConstEvalContext* ctx, Symbol* symbol,
                                double* result);
bool ConstexprEvaluateCallAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                    int64_t* result);
bool ConstexprEvaluateCallAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                     double* result);
bool ConstexprEvaluateCallAsObject(ConstEvalContext* ctx, ASTNode* node);
bool ConstexprEvaluateCall(ConstEvalContext* ctx, ASTNode* node);
bool ConstexprEvaluateConstructorCallForSymbol(ConstEvalContext* ctx,
                                               ASTNode* node,
                                               Symbol* symbol);
bool ConstexprEvaluateMutationAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                        TypeRecord* type, int64_t* result);
bool ConstexprEvaluateMutationAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                         TypeRecord* type, double* result);
bool ConstexprEvaluateObjectAccessAsInteger(ConstEvalContext* ctx,
                                            ASTNode* node, int64_t* result);
bool ConstexprEvaluateObjectAccessAsFloating(ConstEvalContext* ctx,
                                             ASTNode* node, double* result);
bool ConstexprEvaluatePointerDereferenceAsInteger(ConstEvalContext* ctx,
                                                  ASTNode* node,
                                                  int64_t* result);
bool ConstexprEvaluatePointerDereferenceAsFloating(ConstEvalContext* ctx,
                                                   ASTNode* node,
                                                   double* result);
bool ConstexprEvaluatePointerComparison(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result);
bool ConstexprEvaluateObjectAddress(ConstEvalContext* ctx, ASTNode* node,
                                    ConstexprObject** object);

bool ConstexprEvaluateObjectConstantForSymbol(Symbol* symbol,
                                              ASTNode* initializer);
Symbol* ConstexprFunctionDefinition(Symbol* symbol);
bool ConstexprMaterializeClassArgument(ConstEvalContext* ctx, ASTNode* arg,
                                       TypeRecord* object_type,
                                       ConstexprObject** object);
ASTNode* ConstexprObjectInitializerForSymbol(Symbol* symbol,
                                             SourceLocation location);

#endif /* constexpr_h */
