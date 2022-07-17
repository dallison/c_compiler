//
//  main.c
//  expr_eval
//
//  Created by David Allison on 11/8/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "compiler.h"
#include "lex.h"

static ASTNode* Expression(Lex* lex);

static int num_errors;

// Primary expression: (expr) or a number.
// Returns an ASTNode, never NULL.
static ASTNode* PrimaryExpression(Lex* lex) {
  // Parenthesized expression.
  if (LexMatch(lex, TOK(lparen))) {
    ASTNode* expr = Expression(lex);
    if (!LexMatch(lex, TOK(rparen))) {
      LexError(lex, "Missing )");
      num_errors++;
    }
    return expr;
  }
  // Integer constant - convert to double.
  if (LexLookingAt(lex, TOK(number))) {
    double value = lex->number;
    LexNextToken(lex);
    return NewRealConstantASTNode(value, NULL,
                                  lex->current_token_location);
  } else if (LexLookingAt(lex, TOK(fnumber))) {
    double value = lex->fnumber;
    LexNextToken(lex);
    return NewRealConstantASTNode(value, NULL,
                                  lex->current_token_location);
  }
  LexError(lex, "Syntax error");
  num_errors++;
  // Return node with zero constant to simplify evaluation.
  return NewRealConstantASTNode(0, NULL,
                                lex->current_token_location);
}

// Unary expression, allow unary minus only.
static ASTNode* UnaryExpression(Lex* lex) {
  if (LexMatch(lex, TOK(minus))) {
    ASTNode* sub = UnaryExpression(lex);
    return NewUnaryASTNode(AST_OP(uminus),
                           NULL,
                           lex->current_token_location, sub);
  }
  return PrimaryExpression(lex);
}

// Multiple or divide.
static ASTNode* MultiplicativeExpression(Lex* lex) {
  ASTNode* result = UnaryExpression(lex);
  for (;;) {
    if (LexMatch(lex, TOK(star))) {
      ASTNode* right = UnaryExpression(lex);
      result = NewBinaryASTNode(AST_OP(mult), NULL, lex->current_token_location,
                                result, right);
    } else if (LexMatch(lex, TOK(slash))) {
      ASTNode* right = UnaryExpression(lex);
      result = NewBinaryASTNode(AST_OP(div), NULL, lex->current_token_location,
                                result, right);
    } else {
      break;
    }
  }
  return result;
}

// Add or subtract.
static ASTNode* AdditiveExpression(Lex* lex) {
  ASTNode* result = MultiplicativeExpression(lex);
  for (;;) {
    if (LexMatch(lex, TOK(plus))) {
      ASTNode* right = MultiplicativeExpression(lex);
      result = NewBinaryASTNode(AST_OP(plus), NULL, lex->current_token_location,
                                result, right);
    } else if (LexMatch(lex, TOK(minus))) {
      ASTNode* right = MultiplicativeExpression(lex);
      result = NewBinaryASTNode(AST_OP(minus), NULL,
                                lex->current_token_location, result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* Expression(Lex* lex) { return AdditiveExpression(lex); }

// Evaluate expression AST recursively.
static double EvaluateAST(ASTNode* ast) {
  BinaryASTNode* binary_node = (BinaryASTNode*)ast;
  UnaryASTNode* unary_node = (UnaryASTNode*)ast;
  switch (ast->op) {
    case AST_OP(fnumber):
      return ((ConstantASTNode*)ast)->value.fvalue;
    case AST_OP(uminus):
      return -EvaluateAST(unary_node->sub);
    case AST_OP(mult):
      return EvaluateAST(binary_node->left) * EvaluateAST(binary_node->right);
    case AST_OP(div): {
      double divisor = EvaluateAST(binary_node->right);
      if (divisor == 0) {
        printf("Division by zero\n");
        num_errors++;
        return 0;
      }
      return EvaluateAST(binary_node->left) / divisor;
    }
    case AST_OP(plus):
      return EvaluateAST(binary_node->left) + EvaluateAST(binary_node->right);
    case AST_OP(minus):
      return EvaluateAST(binary_node->left) - EvaluateAST(binary_node->right);
    default:
      printf("ERROR\n");
      num_errors++;
      return 0;
  }
}

// Given an expression in a String, evaluate it and return result.
// Takes ownership of string.
static double EvaluateExpression(String* expr) {
  compiler = calloc(sizeof(Compiler), 1);
  compiler->max_errors = 10;
  Preprocessor preprocessor;
  PreprocessorInit(&preprocessor);

  Lex lex;
  LexInitFromString(&lex, "", expr, &preprocessor);
  LexNextToken(&lex);
  ASTNode* ast = Expression(&lex);
  double value = EvaluateAST(ast);

  ASTNodeDelete(ast);
  LexDestruct(&lex);
  PreprocessorDestruct(&preprocessor);
  return value;
}

int main(int argc, const char* argv[]) {
  if (argc < 2) {
    printf("usage: give me an expression\n");
    exit(1);
  }
  String* expr = NewString("");
  for (int i = 1; i < argc; i++) {
    StringAppend(expr, argv[i]);
    StringAppend(expr, " ");
  }
  StringAppend(expr, "\n");
  double value = EvaluateExpression(expr);
  if (num_errors == 0) {
    printf("%g\n", value);
  }
}
