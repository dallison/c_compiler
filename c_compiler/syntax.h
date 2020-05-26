//
//  syntax.h
//  c_compiler
//
//  Created by David Allison on 10/28/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef syntax_h
#define syntax_h

#include <stdarg.h>
#include <setjmp.h>

#include "ast.h"
#include "dstring.h"
#include "lex.h"
#include "symbol_table.h"
#include "vector.h"

extern jmp_buf error_abort_state;       // Where to abort to.
extern bool abort_on_error;

// The syntax analyzer.
typedef struct Syntax {
  Lex* lex;                             // Lexical analyzer providing tokens.
  ASTNode* ast;                         // Output AST.
  LocalSymbolTable* local_symbol_stack; // Local symbol tables.
  LocalSymbolTable* local_tag_stack;    // Local tags.
  char fake_name_buffer[32];            // Buffer to generate fake names.
  int fake_name_index;                  // Next fake name index.
  bool found_open_paren;                // We've consumed an open paren.
  int loop_count;                       // Number of nested loops.
  int switch_count;                     // Number of nested switch statements.
  Vector all_local_symbols;  // All symbols defined in a function (owned by this
                             // vector).
  Vector local_statics;      // All local statics defined in function.
  Vector all_symbols;        // All symbols (needed by assembler).
} Syntax;

// Token classes allow us to recover from syntax errors by
// skipping tokens until the current token matches a certain
// class.
#define TC(x) kTokenClass##x
typedef enum TokenClass {
  TC(stmt) = 1 << 0,            // A statement.
  TC(type) = 1 << 1,            // A type.
  TC(expr) = 1 << 2,            // An expression.
  TC(closebra) = 1 << 3,        // A closing bracket.
  TC(closebrace) = 1 << 4,      // A closing brace.
  TC(openbra) = 1 << 5,         // An opening bracket.
  TC(exprsep) = 1 << 6,         // An expression separator.
  TC(decl) = 1 << 7,            // A declaration.
  TC(semicolon) = 1 << 8,       // A semicolon
} TokenClass;

// Given a token, what class does it belong to?
TokenClass ClassifyToken(Token tok);

void SyntaxInit(Syntax* syntax, Lex* lex);
void SyntaxDestruct(Syntax* syntax);
void SyntaxResetForNewDeclaration(Syntax* syntax);

bool SyntaxAddSymbol(Syntax* syntax, Symbol* symbol);
Symbol* SyntaxFindSymbol(Syntax* syntax, String* name);

bool SyntaxAddTag(Syntax* syntax, Symbol* symbol);
Symbol* SyntaxFindTag(Syntax* syntax, String* name);
Symbol* SyntaxFindTopScopeTag(Syntax* syntax, String* name);

void SyntaxOpenScope(Syntax* syntax);
void SyntaxCloseScope(Syntax* syntax);

const char* SyntaxFakeName(Syntax* syntax);

void SyntaxError(Syntax* syntax, const char* format, ...);
void SyntaxWarning(Syntax* syntax, const char* warn, const char* format, ...);
Storage SyntaxParseStorage(Syntax* syntax);

ASTNode* SyntaxParseExternalDeclaration(Syntax* syntax);
ASTNode* SyntaxParseLocalDeclaration(Syntax* syntax);

void SyntaxNeedSemicolon(Syntax* syntax, TokenClass followers);
void SyntaxNeedBracket(Syntax* syntax, Token bracket, TokenClass followers);
void SyntaxRecover(Syntax* syntax, TokenClass tc);
bool SyntaxLookingAtType(Syntax* syntax);
bool SyntaxLookingAtDeclaration(Syntax* syntax);

Symbol* SyntaxNewTemporary(Syntax* syntax, struct TypeRecord* type);
ASTNode* SyntaxNewPCLabel(SourceLocation location);

#endif /* syntax_h */
