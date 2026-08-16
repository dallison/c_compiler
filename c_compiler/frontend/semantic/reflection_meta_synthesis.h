//
//  reflection_meta_synthesis.h
//  c_compiler
//

#ifndef reflection_meta_synthesis_h
#define reflection_meta_synthesis_h

#include "ast.h"

ASTNode* SemanticTryAnalyzeMetaSynthesisCallEarly(VectorASTNode* call);
ASTNode* SemanticTryAnalyzeMetaSynthesisCall(VectorASTNode* call);

#endif /* reflection_meta_synthesis_h */
