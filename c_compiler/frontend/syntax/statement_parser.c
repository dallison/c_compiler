//
//  statement_parser.c
//  c_compiler
//
//  Created by David Allison on 10/31/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include <stdlib.h>

#include "expr_parser.h"
#include "statement_parser.h"
#include "compiler.h"

// A compound statement is a brace-enclosed sequence of statements.
// The AST node holding this is a CompoundStatementASTNode.
// The node simply contains a vector of statements.
static ASTNode* ParseCompoundStatement(Syntax* syntax, TokenClass followers,
                                       SourceLocation location) {
  // Compound statements open a scope.
  SyntaxOpenScope(syntax);
  Vector* statements = NewVector();
  Lex* lex = syntax->lex;

  // Add label for start of statements in block
  if (compiler->debug_output) {
    VectorAppend(statements, SyntaxNewPCLabel(location));
  }
  // Parse the sequence of statements or declarations, adding them to the vector.
  bool seen_statement = false;
  while (!LexEof(lex) && !LexLookingAt(lex, TOK(rbrace))) {
    if (!LexMatch(lex, TOK(semicolon))) {
      ASTNode* stmt;
      if (SyntaxLookingAtDeclaration(syntax)) {
        if (seen_statement) {
          SyntaxWarning(syntax, "declaration-after-statement",
                        "declaration after statement");
        }
        // Declaration.
        stmt = SyntaxParseLocalDeclaration(syntax);
      } else {
        seen_statement = true;
        stmt = SyntaxParseStatement(syntax, followers | TC(closebrace));
      }
      if (stmt != NULL) {
        VectorAppend(statements, stmt);
      }
    }
  }
   
  if (compiler->debug_output) {
    // Label at end of statements in block.
    VectorAppend(statements, SyntaxNewPCLabel(location));
  }
  
  // We have not consumed the close paren yet, do so now.
  SyntaxNeedBracket(syntax, TOK(rbrace), followers);

  // Close the scope.
  SyntaxCloseScope(syntax);
  return NewCompoundStatementASTNode(statements, location);
}

// An 'if' statement can have an optional 'else' clause.
// The AST node is an IfStatementASTNode and contains:
// 1. A condition (expression)
// 2. An 'if' clause stateemnt
// 3. An optional 'else' statement clause - NULL is absent.
static ASTNode* ParseIfStatement(Syntax* syntax, TokenClass followers,
                                 SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* cond = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  Lex* lex = syntax->lex;

  location = lex->current_token_location;
  ASTNode* if_part = SyntaxParseStatement(syntax, followers);
  ASTNode* else_part = NULL;
  if (LexMatch(lex, TOK(else))) {
    else_part = SyntaxParseStatement(syntax, followers);
  }
  return NewIfStatementASTNode(cond, if_part, else_part, location);
}

// A while statement.
static ASTNode* ParseWhileStatement(Syntax* syntax, TokenClass followers,
                                    SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* cond = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);
  location = syntax->lex->current_token_location;

  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  return NewCombinedStatementASTNode(AST_OP(while), cond, stmt, location);
}

// A do statement.
static ASTNode* ParseDoStatement(Syntax* syntax, TokenClass followers,
                                 SourceLocation location) {
  Lex* lex = syntax->lex;
  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  location = syntax->lex->current_token_location;

  if (!LexMatch(lex, TOK(while))) {
    SyntaxError(syntax, "Missing while in do loop");
  }
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* cond = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  return NewCombinedStatementASTNode(AST_OP(do), cond, stmt, location);
}

// Switch statement.
static ASTNode* ParseSwitchStatement(Syntax* syntax, TokenClass followers,
                                     SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  ASTNode* expr = SyntaxParseExpression(syntax, followers);
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  location = syntax->lex->current_token_location;
  syntax->switch_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->switch_count--;
  return NewSwitchStatementASTNode(expr, stmt, location);
}

static ASTNode* ParseAsmStatement(Syntax* syntax, TokenClass followers,
                         SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), TC(openbra));
  bool is_volatile = LexMatch(syntax->lex, TOK(volatile));
  // TODO: gcc asm syntax?
  String* text = NewString("");
  while (LexLookingAt(syntax->lex, TOK(string))) {
    StringAppend(text, syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), TC(exprsep) | TC(decl));
  // The AsmASTNode takes ownership of the text string.
  return NewAsmASTNode(text, is_volatile, location);
}

// For statement.
static ASTNode* ParseForStatement(Syntax* syntax, TokenClass followers,
                                  SourceLocation location) {
  SyntaxNeedBracket(syntax, TOK(lparen), followers);
  Lex* lex = syntax->lex;
  ASTNode* c1 = NULL;
  ASTNode* c2 = NULL;
  ASTNode* c3 = NULL;

  SyntaxOpenScope(syntax);
  if (!LexLookingAt(lex, TOK(semicolon))) {
    if (SyntaxLookingAtType(syntax)) {
      c1 = SyntaxParseLocalDeclaration(syntax);
    } else {
      c1 = SyntaxParseExpression(syntax, followers);
      SyntaxNeedSemicolon(syntax, followers | TC(expr));
    }
  } else {
    SyntaxNeedSemicolon(syntax, followers | TC(expr));
  }
  if (!LexLookingAt(lex, TOK(semicolon))) {
    c2 = SyntaxParseExpression(syntax, followers);
  }
  SyntaxNeedSemicolon(syntax, followers | TC(closebra));
  if (!LexLookingAt(lex, TOK(rparen))) {
    c3 = SyntaxParseExpression(syntax, followers);
  }
  SyntaxNeedBracket(syntax, TOK(rparen), followers);

  location = syntax->lex->current_token_location;
  syntax->loop_count++;
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  syntax->loop_count--;
  SyntaxCloseScope(syntax);
  return NewForStatementASTNode(c1, c2, c3, stmt, location);
}

static ASTNode* ParseBreakStatement(Syntax* syntax, TokenClass followers,
                                    SourceLocation location) {
  if (syntax->loop_count == 0 && syntax->switch_count == 0) {
    SyntaxError(syntax, "break outside loop or switch");
  }
  return NewASTNode(AST_OP(break), NULL, location);
}

static ASTNode* ParseContinueStatement(Syntax* syntax, TokenClass followers,
                                       SourceLocation location) {
  if (syntax->loop_count == 0) {
    SyntaxError(syntax, "continue outside loop");
  }
  return NewASTNode(AST_OP(continue), NULL, location);
}

static ASTNode* ParseCaseStatement(Syntax* syntax, TokenClass followers,
                                   SourceLocation location) {
  if (syntax->switch_count == 0) {
    SyntaxError(syntax, "case outside switch");
  }
  ASTNode* expr = SyntaxParseExpression(syntax, followers);
  if (!LexMatch(syntax->lex, TOK(colon))) {
    SyntaxError(syntax, "Missing colon after case");
  }
  // We don't want to create a deep tree of case statements for the common
  // code sequence:
  // case a:
  // case b:
  // ...
  // case c:
  //
  ASTNode* stmt = NULL;
  if (!LexLookingAt(syntax->lex, TOK(case)) &&
      !LexLookingAt(syntax->lex, TOK(default))) {
    stmt = SyntaxParseStatement(syntax, followers);
  }
  return NewCaseLabelASTNode(expr, stmt, location);
}

static ASTNode* ParseDefaultStatement(Syntax* syntax, TokenClass followers,
                                      SourceLocation location) {
  if (syntax->switch_count == 0) {
    SyntaxError(syntax, "default outside switch");
  }
  if (!LexMatch(syntax->lex, TOK(colon))) {
    SyntaxError(syntax, "Missing colon after default");
  }
  ASTNode* stmt = SyntaxParseStatement(syntax, followers);
  return NewCaseLabelASTNode(NULL, stmt, location);
}

static ASTNode* ParseReturnStatement(Syntax* syntax, TokenClass followers,
                                     SourceLocation location) {
  ASTNode* expr = NULL;
  if (!LexLookingAt(syntax->lex, TOK(semicolon))) {
    if (LexMatch(syntax->lex, TOK(asm))) {
      // Extension: Allow return asm("foo")
      // This is to allow us to use assembly language as a function
      // return value.  The assembly code must set up the return
      // value correctly as we don't interpreter it in any way.
      // For example:
      //    return asm("mv a0, s0\n")
      // will move the RISC-V frame pointer into the return value
      // and then return from the function.
      expr = ParseAsmStatement(syntax, followers, location);
    } else {
      expr = SyntaxParseExpression(syntax, followers);
    }
  }
  return NewCombinedStatementASTNode(AST_OP(return), expr, NULL,
                                     location);
}

static ASTNode* ParseGotoStatement(Syntax* syntax, TokenClass followers,
                                   SourceLocation location) {
  if (!LexLookingAt(syntax->lex, TOK(identifier))) {
    SyntaxError(syntax, "Label expected after goto");
    return NULL;
  } else {
    String* label_name = NewString(syntax->lex->spelling.value);
    LexNextToken(syntax->lex);
    return NewGotoStatementASTNode(label_name, location);
  }
}

// Table of statement parsers.
struct StatementParser {
  Token token;
  ASTNode* (*parser)(Syntax* syntax, TokenClass followers,
                     SourceLocation location);
  bool need_semicolon;
} statement_parsers[] = {
  {TOK(semicolon), NULL, false},
  {TOK(lbrace), ParseCompoundStatement, false},
  {TOK(if), ParseIfStatement, false},
  {TOK(while), ParseWhileStatement, false},
  {TOK(do), ParseDoStatement, true},
  {TOK(for), ParseForStatement, false},
  {TOK(switch), ParseSwitchStatement, false},
  {TOK(break), ParseBreakStatement, true},
  {TOK(continue), ParseContinueStatement, true},
  {TOK(case), ParseCaseStatement, false},
  {TOK(default), ParseDefaultStatement, false},
  {TOK(return), ParseReturnStatement, true},
  {TOK(goto), ParseGotoStatement, true},
  {TOK(asm), ParseAsmStatement, true},
};

#define NUM_STATEMENT_PARSERS \
  (sizeof(statement_parsers) / sizeof(statement_parsers[0]))

// Parse a single statement, returning an AST node or NULL.
ASTNode* SyntaxParseStatement(Syntax* syntax, TokenClass followers) {
  followers |= TC(stmt);
  Lex* lex = syntax->lex;
  ASTNode* stmt = NULL;
  bool need_semicolon = true;
  SourceLocation location = syntax->lex->current_token_location;

  bool found = false;
  for (size_t i = 0; i < NUM_STATEMENT_PARSERS; i++) {
    struct StatementParser* parser = &statement_parsers[i];
    if (syntax->lex->current_token == parser->token) {
      LexNextToken(lex);
      if (parser->parser != NULL) {
        stmt = (*parser->parser)(syntax, followers, location);
      }
      need_semicolon = parser->need_semicolon;
      found = true;
      break;
    }
  }
  if (!found) {
    // Expression or label statement.
    if (LexLookingAt(lex, TOK(identifier))) {
      // This will skip forward to the next non-space or non-comment
      // but will not change the current token.
      LexSkipSpacesAndComments(lex);
      if (lex->line.value[lex->pos] == ':') {
        String label_name;
        StringInit(&label_name, lex->spelling.value);
        LexNextToken(lex);  // Consume label name.
        LexNextToken(lex);  // Consume colon.
        ASTNode* label_stmt = SyntaxParseStatement(syntax, followers);
        stmt = NewLabelASTNode(label_name.value, label_stmt, false,
                               syntax->lex->current_token_location);
        need_semicolon = false;
        StringDestruct(&label_name);
      } else {
        ASTNode* expr = SyntaxParseExpression(syntax, followers);
        stmt = NewExpressionStatementASTNode(expr, location);
      }
    } else {
      ASTNode* expr = SyntaxParseExpression(syntax, followers);
      stmt = NewExpressionStatementASTNode(expr, location);
    }
    
  }

  if (need_semicolon) {
    SyntaxNeedSemicolon(syntax, followers);
  }
  if (stmt != NULL) {
    // This is the start of a statement.
    stmt->flags |= kASTStatementStart;
  }
  return stmt;
}
