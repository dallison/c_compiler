//
//  expr_parser.h
//  c_compiler
//
//  Created by David Allison on 10/30/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef expr_parser_h
#define expr_parser_h

#include "syntax.h"

ASTNode* SyntaxParseExpression(Syntax* syntax, TokenClass followers);
ASTNode* SyntaxParseSingleExpression(Syntax* syntax, TokenClass followers);
ASTNode* NewCXXDeleteExpressionForPointer(Syntax* syntax, ASTNode* expr,
                                          bool is_array_delete,
                                          SourceLocation location,
                                          bool global_scope);

#endif /* expr_parser_h */
