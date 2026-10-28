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
// File-scope and other callers that already know this is an asm statement.
// Declaration lookahead must not treat `asm("...")` as an implicit-int declarator.
ASTNode* SyntaxParseAsmStatement(Syntax* syntax, TokenClass followers);

// Parses a C++26/29 `consteval { ... }` block and returns a ConstevalBlockASTNode.
ASTNode* SyntaxParseConstevalBlock(Syntax* syntax, TokenClass followers);

// Parses the handler-seq of a function-try-block (`catch` clauses). The
// function scope that should see the parameters must already be open.
Vector* SyntaxParseCatchHandlers(Syntax* syntax, TokenClass followers);

// Wraps `func`'s body in a try statement with `catches`. Constructor and
// destructor handlers rethrow when control reaches their end.
void SyntaxWrapFunctionTryBlock(TypeRecord* func, Vector* catches);

// Statements that should receive a constructor preamble. A function-try-block
// keeps that preamble inside the try, so a constructor exception is caught.
Vector* FunctionTryBodyStatements(TypeRecord* func);

// Replaces a deferred range-for begin-expr / end-expr (AST_OP(range_begin) /
// AST_OP(range_end), emitted for a loop whose range type was dependent at parse
// time) with the form [stmt.ranged] prescribes for the now-known range type.
// The returned node is unanalyzed.
ASTNode* SyntaxResolveRangeForIterator(ASTNode* node);

// Appends C++ scope-exit destructor calls for the block-scope automatic objects
// declared directly in `statements` (in reverse construction order) to the end
// of the vector.  Used both when closing a nested compound statement and for a
// function body's outermost block.  A no-op in C.
void SyntaxAppendCXXBlockScopeDestructors(Vector* statements);

// True when `type` names a class with member `begin` and `end` ([stmt.ranged]).
bool SyntaxTypeHasRangeMemberBeginEnd(TypeRecord* type);

// Builds a deferred range-for begin/end expression (AST_OP(range_begin) /
// AST_OP(range_end)) for ADL lookup at the expansion/range-for point of
// definition.  The right child is the unqualified `begin`/`end` call.
ASTNode* SyntaxNewRangeForBoundExpr(Syntax* syntax, Symbol* range_sym,
                                    bool is_begin, SourceLocation location);

#endif /* statement_parser_h */
