//
//  constexpr.h
//  c_compiler
//

#ifndef constexpr_h
#define constexpr_h

#include "syntax.h"

typedef struct ConstexprValue ConstexprValue;
typedef struct ConstexprObject ConstexprObject;
typedef struct ConstexprException ConstexprException;
struct ReflectionValue;

typedef struct ConstexprHeapBlock ConstexprHeapBlock;

typedef struct {
  Vector bindings;  // ConstexprBinding*
  Vector objects;   // ConstexprObject*
  Vector heap_blocks;  // ConstexprHeapBlock*
  Vector exception_handles;  // ConstexprException*
  int call_depth;
  int steps;
  int max_steps;
  int unwinding_exceptions;
  ConstexprException* exception;
} ConstEvalContext;

void ConstEvalContextInit(ConstEvalContext* ctx);
void ConstEvalContextDestruct(ConstEvalContext* ctx);
bool ConstEvalStep(ConstEvalContext* ctx);
bool ConstexprEvaluateThrowExpression(ConstEvalContext* ctx, ASTNode* node);

ASTNode* ConstexprInitializerExpression(ASTNode* initializer);
ASTNode* ConstexprObjectInitializerForExpression(TypeRecord* type,
                                                 ASTNode* expression);

bool EvaluateIntegerExpressionInContext(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result);
bool ConstexprBindVariableDeclaration(ConstEvalContext* ctx,
                                      VariableDeclarationASTNode* decl);
bool ConstexprBindExpansionRangeHidden(ConstEvalContext* ctx,
                                       VariableDeclarationASTNode* hidden_decl,
                                       ASTNode* init_expr);
bool EvaluateFloatingPointExpressionInContext(ConstEvalContext* ctx,
                                              ASTNode* node, double* result);
struct ReflectionValue* ConstexprEvaluateReflectionExpression(
    ConstEvalContext* ctx, ASTNode* node);

bool ConstexprBindingAsInteger(ConstEvalContext* ctx, Symbol* symbol,
                               int64_t* result);
bool ConstexprBindingAsFloating(ConstEvalContext* ctx, Symbol* symbol,
                                double* result);
bool ConstexprHasBinding(ConstEvalContext* ctx, Symbol* symbol);
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
struct ReflectionValue* ConstexprEvaluatePointerDereferenceAsReflection(
    ConstEvalContext* ctx, ASTNode* node);
bool ConstexprEvaluatePointerComparison(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result);
bool ConstexprSameObjectPointerDistance(ConstEvalContext* ctx,
                                        ASTNode* begin_expr, ASTNode* end_expr,
                                        size_t* count);
bool ConstexprEvaluateObjectAddress(ConstEvalContext* ctx, ASTNode* node,
                                    ConstexprObject** object);
bool ConstexprEvaluateObjectSlotInteger(ASTNode* node, size_t slot,
                                        int64_t* result);
// Evaluates a constant char pointer and copies `count` code units from it.
bool ConstexprEvaluateCharacterSequence(ASTNode* pointer, size_t count,
                                        String* result);

bool ConstexprEvaluateObjectConstantForSymbol(Symbol* symbol,
                                              ASTNode* initializer);
Symbol* ConstexprFunctionDefinition(Symbol* symbol);
Symbol* ConstexprRawConstructorCallSymbol(ASTNode* node, ASTNode** receiver);
Symbol* ConstexprConstructorForObjectType(TypeRecord* type,
                                          size_t actual_count);
bool ConstexprMaterializeClassArgument(ConstEvalContext* ctx, ASTNode* arg,
                                       TypeRecord* object_type,
                                       ConstexprObject** object);
ASTNode* ConstexprObjectInitializerForSymbol(Symbol* symbol,
                                             SourceLocation location);

#endif /* constexpr_h */
