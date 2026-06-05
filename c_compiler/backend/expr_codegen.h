//
//  expr_codegen.h
//  c_compiler
//
//  Created by David Allison on 11/21/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef expr_codegen_h
#define expr_codegen_h

#include "ast.h"
#include "codegen.h"

IRNode* GenerateExpression(Generator* gen, ASTNode* node);

#endif /* expr_codegen_h */
