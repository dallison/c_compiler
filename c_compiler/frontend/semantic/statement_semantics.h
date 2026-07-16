//
//  statement_semantics.h
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef statement_semantics_h
#define statement_semantics_h

#include "semantics.h"
#include "syntax.h"

void AnalyzeStatement(ASTNode* node);
void CheckUnusedLabels(ASTNode* body);

// Inserts C++ scope-exit destructor calls for automatic objects at each
// return/break/continue in a fully-analyzed, non-dependent function body.  Must
// run with compiler->current_function (and access context) set to `func`.
void CXXInsertScopeExitDestructors(TypeRecord* func);

#endif /* statement_semantics_h */
