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
ASTNode* SyntaxParseConditionalExpression(Syntax* syntax, TokenClass followers);
ASTNode* SyntaxParseConstraintExpression(Syntax* syntax,
                                         TokenClass followers);
ASTNode* NewCXXDeleteExpressionForPointer(Syntax* syntax, ASTNode* expr,
                                          bool is_array_delete,
                                          SourceLocation location,
                                          bool global_scope);
// The class's own `name` ("operator new", "operator delete[]", ...) taking a
// single argument, or NULL when the class does not declare one.
Symbol* CXXClassUsualAllocationFunction(TypeRecord* type, const char* name);
// The cleanup-only `operator delete((void*)temp)` statement that frees a
// single-object new-expression's storage when its initialization throws.
ASTNode* NewCXXNewDeallocationCleanup(Symbol* temp, TypeRecord* allocated_type,
                                      bool global_scope,
                                      SourceLocation location);

#endif /* expr_parser_h */
