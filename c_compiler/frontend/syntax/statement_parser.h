//
//  statement_parser.h
//  c_compiler
//
//  Created by David Allison on 10/31/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef statement_parser_h
#define statement_parser_h

#include "syntax.h"

ASTNode* SyntaxParseStatement(Syntax* syntax, TokenClass followers);

// Appends C++ scope-exit destructor calls for the block-scope automatic objects
// declared directly in `statements` (in reverse construction order) to the end
// of the vector.  Used both when closing a nested compound statement and for a
// function body's outermost block.  A no-op in C.
void SyntaxAppendCXXBlockScopeDestructors(Vector* statements);

#endif /* statement_parser_h */
