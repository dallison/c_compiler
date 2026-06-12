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

#endif /* statement_semantics_h */
