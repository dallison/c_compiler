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
ASTNode* SemanticAnalyzeSpliceQualified(SpliceQualifiedASTNode* node);
ASTNode* SemanticAnalyzeAddressedSplice(UnaryASTNode* address);
bool SemanticLowerMemberSplice(BinaryASTNode* access);
ASTNode* SemanticAnalyzeReflectionComparison(BinaryASTNode* comparison);
ASTNode* SemanticTryAnalyzeMetaCall(VectorASTNode* call);
ASTNode* SemanticAnalyzeConstevalBlock(ConstevalBlockASTNode* node);
ASTNode* SemanticAnalyzeConstevalBlockWithAccess(ConstevalBlockASTNode* node,
                                                 int class_access);
ASTNode* SemanticAnalyzeFunctionConstevalBlockDuringParse(
    ConstevalBlockASTNode* node, Vector* statements, size_t insert_index);

void SemanticAttachAnnotationAttributes(Vector* attributes, Symbol* symbol);

ReflectionEntityKind ReflectionKindForSymbol(Symbol* symbol);

ReflectionValue* SemanticReflectionValueFromExpression(ASTNode* expression);
ReflectionValue* SemanticEvaluateReflection(ASTNode* expression);
TypeRecord* SemanticMaterializeReflectedType(ReflectionValue* value,
                                             SourceLocation location);
Symbol* SemanticMaterializeReflectedTemplate(ReflectionValue* value,
                                             SourceLocation location);
Namespace* SemanticMaterializeReflectedNamespace(ReflectionValue* value);

TypeRecord* SemanticResolveDependentSpliceType(TypeRecord* type,
                                               ASTNode* diagnostic);
void SemanticResolveStructDependentSplices(Struct* str, ASTNode* diagnostic);
Symbol* SemanticTemplateSymbolFromReflection(ASTNode* reflection,
                                             SourceLocation location);

#endif /* reflection_semantics_h */
