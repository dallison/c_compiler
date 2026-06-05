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

#endif /* statement_parser_h */
