//
//  reflection_meta_traits.h
//  c_compiler
//
//  Compile-time evaluation of std::meta type-trait mirrors marked
//  [[davecc::meta_intrinsic]].
//

#ifndef reflection_meta_traits_h
#define reflection_meta_traits_h

#include "ast.h"

ASTNode* SemanticTryAnalyzeMetaTraitCall(VectorASTNode* call);

#endif /* reflection_meta_traits_h */
