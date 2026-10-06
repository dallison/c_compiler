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

typedef enum {
  kConstexprPCodeFailureUnsupported,
  kConstexprPCodeFailureInvalid,
} ConstexprPCodeFailureKind;

typedef struct {
  Vector bindings;  // ConstexprBinding*
  Vector objects;   // ConstexprObject*
  Vector heap_blocks;  // ConstexprHeapBlock*
  Vector exception_handles;  // ConstexprException*
  // ConstexprBinding* for materialized temporaries (a by-value argument, a
  // constructed prvalue), which their full-expression cleanup names after the
  // expression that created them has returned.
  Vector temporaries;
  int call_depth;
  int steps;
  int max_steps;
  int unwinding_exceptions;
  int destroy_at_depth;
  // call_depth of the running std::construct_at body, or 0.  Only that frame
  // may use placement new before C++26.
  int construct_at_call_depth;
  ASTNode* allocation_new_expression;
  const char* pcode_failure_reason;
  ConstexprPCodeFailureKind pcode_failure_kind;
  ConstexprException* exception;
} ConstEvalContext;

void ConstEvalContextInit(ConstEvalContext* ctx);
void ConstEvalContextDestruct(ConstEvalContext* ctx);
// A constant evaluation must release every allocation it makes
// ([expr.const]); one still live at the end is not a constant expression.
bool ConstEvalContextHasLiveAllocation(ConstEvalContext* ctx);
bool ConstEvalStep(ConstEvalContext* ctx);
bool ConstexprEvaluateThrowExpression(ConstEvalContext* ctx, ASTNode* node);

ASTNode* ConstexprInitializerExpression(ASTNode* initializer);
// Fold a core constant expression of pointer type to an address the static
// initializer can encode (a string literal, or the address of a symbol).
// `constexpr const char* p = "hi"` and `Basename(__FILE__, n)` need this:
// the characters are a constant, but the pointer is not an integer.
ASTNode* ConstexprFoldPointerExpression(ASTNode* expr);
ASTNode* ConstexprObjectInitializerForExpression(TypeRecord* type,
                                                 ASTNode* expression);
ASTNode* ConstexprTemplateArgumentObjectInitializerForExpression(
    TypeRecord* type, ASTNode* expression);
bool ConstexprObjectInitializersEquivalent(TypeRecord* type, ASTNode* left,
                                           ASTNode* right);
bool ConstexprObjectInitializerTemplateKey(TypeRecord* type, ASTNode* expression,
                                           String* result);

bool EvaluateIntegerExpressionInContext(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result);
bool EvaluateInt128Constant(ASTNode* node, int64_t* lo, int64_t* hi);
bool ConstexprBindVariableDeclaration(ConstEvalContext* ctx,
                                      VariableDeclarationASTNode* decl);
bool ConstexprBindExpansionRangeHidden(ConstEvalContext* ctx,
                                       VariableDeclarationASTNode* hidden_decl,
                                       ASTNode* init_expr);
bool ConstexprReferenceUsableInCurrentFunction(Symbol* symbol);
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
bool ConstexprEvaluateBitCastAsInteger(ConstEvalContext* ctx, ASTNode* node,
                                       int64_t* result);
bool ConstexprEvaluateBitCastAsFloating(ConstEvalContext* ctx, ASTNode* node,
                                        double* result);
// memcpy/memmove between objects whose static types differ.
bool ConstexprMemoryCopyTypesDiffer(ASTNode* call);
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
bool ConstexprEvaluatePointerDifference(ConstEvalContext* ctx, ASTNode* node,
                                        int64_t* result);
bool ConstexprSameObjectPointerDistance(ConstEvalContext* ctx,
                                        ASTNode* begin_expr, ASTNode* end_expr,
                                        size_t* count);
bool ConstexprEvaluateObjectAddress(ConstEvalContext* ctx, ASTNode* node,
                                    ConstexprObject** object);
bool ConstexprEvaluateAddressValue(ConstEvalContext* ctx, ASTNode* node,
                                   ConstexprValue* result);
bool ConstexprEvaluateObjectSlotInteger(ASTNode* node, size_t slot,
                                        int64_t* result);
// Evaluates a constant char pointer and copies `count` code units from it.
bool ConstexprEvaluateCharacterSequence(ASTNode* pointer, size_t count,
                                        String* result);

bool ConstexprEvaluateObjectConstantForSymbol(Symbol* symbol,
                                              ASTNode* initializer);
void ConstexprPersistObjectAddresses(ConstexprObject* object);
void ConstexprSetSymbolObjectValueState(Symbol* symbol, ValueState state);
Symbol* ConstexprFunctionDefinition(Symbol* symbol);
Symbol* ConstexprRawConstructorCallSymbol(ASTNode* node, ASTNode** receiver);
Symbol* ConstexprConstructorForObjectType(TypeRecord* type,
                                          size_t actual_count);
bool ConstexprMaterializeClassArgument(ConstEvalContext* ctx, ASTNode* arg,
                                       TypeRecord* object_type,
                                       ConstexprObject** object);
bool ConstexprEvaluateValue(ConstEvalContext* ctx, ASTNode* node,
                            TypeRecord* type, ConstexprValue* result);
ASTNode* ConstexprObjectInitializerForSymbol(Symbol* symbol,
                                             SourceLocation location);

#endif /* constexpr_h */
