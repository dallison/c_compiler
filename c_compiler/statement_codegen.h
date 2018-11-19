//
//  statement_codegen.h
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef statement_codegen_h
#define statement_codegen_h

#include "ast.h"
#include "codegen.h"

void GenerateStatement(Generator* gen, ASTNode* node);

#endif /* statement_codegen_h */
