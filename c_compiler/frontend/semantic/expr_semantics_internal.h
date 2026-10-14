//
//  expr_semantics_internal.h
//  c_compiler
//
//  Declarations shared by the expression semantic-analysis feature files.
//

#ifndef expr_semantics_internal_h
#define expr_semantics_internal_h

#include "expr_semantics.h"

#define kArrayElementwiseCopyMaxRank 16

extern bool g_suppress_user_defined_conversion_rank;
extern bool (*type_ranks[])(TypeRecord*);

// Describes which atomic operation is imposing memory-order restrictions.
typedef enum {
  kAtomicOrderLoad,
  kAtomicOrderStore,
  kAtomicOrderReadModifyWrite,
  kAtomicOrderFailure,
  kAtomicOrderFence,
} AtomicOrderUse;

bool AnalyzingTemplatePatternBody(void);
void CXXAnalyzeImmediateEscalationCandidate(Symbol* symbol);
bool CXXEscalateCurrentFunction(void);
bool CXXInImmediateFunctionContext(void);
void ClearDeferredAutoInitAnalysis(ASTNode* node, void* data, int child_id, VisitorMode mode);
void SemanticEnsureAutoReturnTypeDeduced(TypeRecord* func);
bool ASTNodeIsCXXBracedTemporary(ASTNode* node);
ASTNode* AnalyzeFunctionCall(VectorASTNode* node);
void ApplyVirtualBaseAdjustmentToMemberReference(BinaryASTNode* node, Struct* from, Struct* owner, int* member_offset);
const char* CXXAccessName(CXXAccess access);
String* CXXFindBaseTagNameForDestructorSpelling(Struct* str, const char* base_name);
Symbol* CXXStructTemplateOrigin(Struct* str);
bool CXXSameAccessClass(Struct* a, Struct* b);
void CheckDeletedFunctionUse(Symbol* function, ASTNode* use);
bool CurrentFunctionCanAccessMember(Struct* lookup_context, Struct* owner, CXXAccess original_access, CXXAccess effective_access);
StructMember* FindCXXMemberOverloadHead(Struct* owner, String* name);
ASTNode* IdentityCloneNode(ASTNode* node, void* data);
bool LowerMemberFunctionCall(VectorASTNode* node);
bool MemberReceiverIsConst(BinaryASTNode* node);
bool MemberReceiverIsVolatile(BinaryASTNode* node);
bool MemberReceiverMatchesRefQualifier(TypeRecord* func, BinaryASTNode* member_access);
ASTNode* NewAnalyzedBuiltinAddressOf(ASTNode* sub, SourceLocation location);
ASTNode* NewVirtualCalleeFromFunction(ASTNode* receiver, TypeRecord* function_type, bool receiver_is_pointer, SourceLocation location);
bool TemplateArgumentVectorContainsTemplateParameter(Vector* args);
bool TypeIsEffectivelyConst(TypeRecord* type);
ASTNode* AnalyzeCastExpression(CastASTNode* node);
void MaterializeBracedClassTemplateTemporary( CompoundLiteralASTNode* node);
TypeRecord* MaterializeConversionClass(TypeRecord* type);
ASTNode* TryBindReferenceToBaseSubobject(ASTNode* expr, TypeRecord* referent);
bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right);
bool ASTNodeIsGLValue(ASTNode* node);
bool ASTNodeIsLValue(ASTNode* node);
bool ExpressionRequiresASTConstexpr(ASTNode* node, void* data);
void SemanticResumeUserDefinedConversionRank(bool saved);
bool SemanticSuspendUserDefinedConversionRank(void);
bool EvaluateConstantForSymbol(Symbol* symbol, ASTNode* initializer);
ASTNode* FoldConstantExpression(ASTNode* node);
bool IsConstantExpression(ASTNode* node);
bool MarkCXX26SymbolicConstexprReference(Symbol* symbol, ASTNode* initializer);
int CXXBracedInitTargetRank(ASTNode* actual, TypeRecord* target);
bool CXXBracedInitSelectsInitializerListConstructor(ASTNode* actual,
                                                    TypeRecord* target);
StructMember* FindConvertingConstructorCandidate(TypeRecord* to, ASTNode* from, bool allow_explicit, bool allow_same_class);
int OverloadConversionRank(ASTNode* actual, TypeRecord* formal_type);
bool ReferenceCanBind(ASTNode* actual, TypeRecord* reference_type);
bool TryConvertWithConvertingConstructorImpl( ASTNode* from, TypeRecord* to, ConversionContext ctx, bool allow_same_class);
bool TypeDependsOnEnclosingTemplateParameter(TypeRecord* type, int base);
bool TypeHasDependentMemberName(TypeRecord* type);
bool TypeIsDerivedFromInCurrentClass(TypeRecord* from, TypeRecord* to);
ASTNode* AnalyzeExpression(ASTNode* node);
void CheckFormatCall(VectorASTNode* node, Symbol* callee);
void SpecializePrintfCall(VectorASTNode* node, Symbol* callee);
ASTNode* AnalyzeInitialization(ASTNode* node, ASTNode* target, ASTNode* init);
int CXXInitializerListBracedInitRank(ASTNode* actual, TypeRecord* formal_type);
bool ClassHasConversionOperatorTo(ASTNode* actual, TypeRecord* target);
ASTNode* ConvertCXXInitializerListArgument(ASTNode* actual, TypeRecord* formal_type);
void ConvertCompoundAssignmentOperand(BinaryASTNode* node);
int FixedArrayRank(TypeRecord* type);
bool HasAddress(ASTNode* node);
bool IsAssignable(ASTNode* node, bool is_init);
ASTNode* LowerCXXBracedInitToTarget(ASTNode* braced, TypeRecord* target);
bool LvaluePathIsRepeatable(ASTNode* node);
ASTNode* MaterializeTemporary(ASTNode* expr, TypeRecord* type);
ASTNode* NewArrayElementwiseCopy(ASTNode* source, TypeRecord* type, int64_t* indices, int rank, SourceLocation location);
TypeRecord* ReferenceConversionTarget(ASTNode* actual, TypeRecord* reference_type);
ASTNode* SemanticMaterializeClassPrvalue(ASTNode* expr);
bool CXXConstructorCallElidesPrvalueSource(VectorASTNode* call);
bool CXXMemberInitializerMayElidePrvalue(VectorASTNode* call);
bool CalleeNamesItsFunction(ASTNode* callee);
bool FunctionCanBeInlined(FunctionInfo* func);
FunctionInfo* FunctionInfoForCallInlining(FunctionInfo* func);
ASTNode* InlineFunctionCall(FunctionInfo* info, VectorASTNode* call);
ASTNode* LowerVirtualCallerContracts(VectorASTNode* call);
void AnalyzeAtomicBuiltinChildren(VectorASTNode* node);
void AnalyzeAtomicCompareExchangeBuiltin(VectorASTNode* node, bool expected_is_pointer, bool returns_bool);
void AnalyzeAtomicFetchBuiltin(VectorASTNode* node, bool returns_new);
void AnalyzeAtomicLoadBuiltin(VectorASTNode* node);
void AnalyzeAtomicStoreBuiltin(VectorASTNode* node);
void AnalyzeBuiltinBitOperation(VectorASTNode* node);
void AnalyzeBuiltinConstantP(VectorASTNode* node);
void AnalyzeBuiltinExpect(VectorASTNode* node);
void AnalyzeBuiltinIsConstantEvaluated(VectorASTNode* node);
void AnalyzeBuiltinObservableCheckpoint(VectorASTNode* node);
void AnalyzeBuiltinPrefetch(VectorASTNode* node);
void AnalyzeBuiltinStartLifetime(VectorASTNode* node);
void AnalyzeBuiltinAlloca(VectorASTNode* node);
void AnalyzeBuiltinTerminator(VectorASTNode* node);
void AnalyzeSourceIntegerBuiltin(VectorASTNode* node);
void AnalyzeSourceStringBuiltin(VectorASTNode* node);
ASTNode* AnalyzeTypeTraitBuiltin(VectorASTNode* node);
void AnalyzeVarargsBuiltin1(VectorASTNode* args);
void AnalyzeVarargsBuiltin2(VectorASTNode* args);
int ValidateAtomicMemoryOrder(ASTNode* order, AtomicOrderUse use);
void AnalyzeCompoundLiteral(CompoundLiteralASTNode* node);
ASTNode* CXXLambdaConversionExpression(ASTNode* from);
bool LambdaCallMatchesFunctionPointer(ASTNode* lambda, TypeRecord* pointer);
ASTNode* LambdaToFunctionPointer(ASTNode* lambda, TypeRecord* target_fn);
void AnalyzeAddressOperator(UnaryASTNode* node);
void AnalyzeContentsOperator(UnaryASTNode* node);
ASTNode* AnalyzeMemberPointerReference(BinaryASTNode* node);
ASTNode* AnalyzeMemberReference(BinaryASTNode* node);
void AnalyzePackIndexExpression(BinaryASTNode* node);
void AnalyzePointerToMember(UnaryASTNode* node);
void AnalyzeSizeofExpression(SizeofASTNode* node);
ASTNode* AnalyzeTypeidExpression(TypeidASTNode* node);
ASTNode* FoldStaticDataMemberReference(BinaryASTNode* node);
bool LowerMemberPointerFunctionCall(VectorASTNode* node);
StructMember* MemberPointerMemberFromExpression(ASTNode* node);
bool TypeIsPolymorphicClass(TypeRecord* type);
void AddADLFunctionCandidates(String* name, Vector* actuals, Vector* candidates);
void AddFunctionOverloadCandidates(Vector* candidates, Symbol* first);
void AddNamedFunctionCandidates(String* name, Namespace* ns, Vector* candidates);
ASTNode* AnalyzeArraySubscript(BinaryASTNode* node);
ASTNode* AnalyzeAssignmentExpression(BinaryASTNode* node);
void AnalyzeBinaryExpression(BinaryASTNode* node);
ASTNode* AnalyzeBitwiseOperator(BinaryASTNode* node);
void AnalyzeCoAwaitExpression(UnaryASTNode* node);
void AnalyzeCoYieldExpression(UnaryASTNode* node);
ASTNode* AnalyzeComparisonOperator(BinaryASTNode* node);
void AnalyzeConditionalExpression(BinaryASTNode* node);
ASTNode* AnalyzeIncDec(UnaryASTNode* node);
ASTNode* AnalyzeMinusOperator(BinaryASTNode* node);
ASTNode* AnalyzeMultidimensionalSubscript(VectorASTNode* node);
void AnalyzeNoexceptExpression(UnaryASTNode* node);
ASTNode* AnalyzePlusOperator(BinaryASTNode* node);
ASTNode* AnalyzeShift(BinaryASTNode* node);
ASTNode* AnalyzeThreeWayComparison(BinaryASTNode* node);
void AnalyzeUnaryExpression(UnaryASTNode* node);
const char* BinaryOperatorFunctionName(ASTOpcode op);
TypeRecord* BitFieldPromotionType(ASTNode* operand);
Symbol* FollowUsingAliasForADL(Symbol* symbol);
void InsertNumericConversions(BinaryASTNode* node, bool promote_to_int);
bool IsNullPointerConstant(ASTNode* node);
TypeRecord* NewLogicalResultType(void);
ASTNode* NewOperatorMemberCall(ASTNode* receiver, const char* op_name, Vector* actuals, SourceLocation location);
ASTNode* ReplaceVectorWithCall(VectorASTNode* node, ASTNode* call);
bool RewrittenEqualityBeatsInequality(BinaryASTNode* node);
ASTNode* TryAnalyzeOverloadedBinaryOperator(BinaryASTNode* node);
ASTNode* TryAnalyzeOverloadedBinaryOperatorWithAnalyzedOperands( BinaryASTNode* node);
ASTNode* TryAnalyzeOverloadedUnaryOperator(UnaryASTNode* node);
ASTNode* TryRewriteComparisonOperator(BinaryASTNode* node);
const char* UnaryOperatorFunctionName(ASTOpcode op);
Symbol* CXXResolveFunctionAddressForTargetType(Symbol* head, Vector* explicit_args, TypeRecord* target_fn);
bool CallHasPackExpansionActual(VectorASTNode* node);
int FunctionCallScore(TypeRecord* func, VectorASTNode* node, size_t first_formal_arg);
void InstantiateResolvedFunctionTemplateCall(VectorASTNode* node);
void ReportUnsatisfiedFunctionTemplateConstraints(VectorASTNode* node, Symbol* templ, Vector* explicit_args, size_t first_formal_arg);
Symbol* ResolveFreeFunctionWithADL(String* name, Vector* actuals, bool diagnose_ambiguous);
Symbol* ResolveFreeOperatorCoAwaitForActual(ASTNode* actual, bool diagnose_ambiguous);
StructMember* ResolveMemberFunctionOverload(StructMember* first, VectorASTNode* node, BinaryASTNode* member_access);
void ResolveOverloadedFunctionCall(VectorASTNode* node);
bool SymbolIsTemplateFunction(Symbol* candidate);
void AnalyzeLogicalOperator(BinaryASTNode* node);
void AnalyzeThrowExpression(ThrowASTNode* node);
void SetNeedAddress(ASTNode* node);
ASTNode* AnalyzeIdentifier(IdentifierASTNode* node);
ASTNode* CXXFoldIsSameVExpression(ASTNode* expr);
ASTNode* CXXMaterializeVariableTemplateExpression(ASTNode* expr);
ASTNode* CXXTryFoldStdGetFromTupleCall(VectorASTNode* call);
ASTNode* CXXTryReplaceWithVariableTemplateValue( ASTNode* use_site, Symbol* var_sym, Vector* template_args, SourceLocation location);
ASTNode* FoldConstantArgument(ASTNode* actual);

#endif /* expr_semantics_internal_h */
