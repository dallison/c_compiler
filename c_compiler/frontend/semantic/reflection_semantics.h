//
//  reflection_semantics.h
//  c_compiler
//

#ifndef reflection_semantics_h
#define reflection_semantics_h

#include "ast.h"
#include "reflection.h"

ASTNode* SemanticAnalyzeReflection(ReflectionASTNode* node);
ASTNode* SemanticAnalyzeSplice(SpliceASTNode* node);
ASTNode* SemanticAnalyzeAddressedSplice(UnaryASTNode* address);
bool SemanticLowerMemberSplice(BinaryASTNode* access);
ASTNode* SemanticAnalyzeReflectionComparison(BinaryASTNode* comparison);
ASTNode* SemanticTryAnalyzeMetaCall(VectorASTNode* call);

ReflectionValue* SemanticReflectionValueFromExpression(ASTNode* expression);

#endif /* reflection_semantics_h */
