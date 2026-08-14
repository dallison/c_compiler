#ifndef contracts_semantics_h
#define contracts_semantics_h

#include "ast.h"
#include "type.h"

void SemanticAnalyzeFunctionContracts(TypeRecord* func, ASTNode* declaration);
void SemanticAnalyzeContractAssert(ContractAssertASTNode* node);

#endif
