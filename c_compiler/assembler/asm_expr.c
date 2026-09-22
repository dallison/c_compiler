//
//  asm_expr.c
//  c_compiler
//
//  Created by David Allison on 10/5/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include <stdint.h>

#include <assert.h>
#include <string.h>
#include "asm_expr.h"
#include "assembler.h"

static ASTNode* ParseExpression(Assembler* assembler);

static ASTNode* ParseIdentifier(Assembler* assembler) {
  Lex* lex = assembler->syntax.lex;
  if (assembler->object.pass == 1 && assembler->parsing_layout_expression) {
    assembler->object.requires_layout_pass = true;
  }

  String name;
  StringInit(&name, lex->spelling.value);
  LexNextToken(lex);

  // Find the symbol by searching all symbol tables.  It must exist.
  AssemblerSymbol* symbol = AssemblerFindSymbol(assembler, name.value);
  if (symbol == NULL) {
    // In assembler mode we have symbols but we pre-declare them
    // if they don't exist.
    symbol = NewAssemblerSymbol(name.value, assembler->object.current_section,
                                SYM_TYPE(none), SYM_BIND(local),
                                AssemblerCurrentAddress(assembler));
    symbol->is_forward_declared = true;
    // Not inserted into the symbol table; track it so it is freed at destruct.
    AssemblerTrackOrphanSymbol(assembler, symbol);
  }
  StringDestruct(&name);
  return NewRawIdentifierASTNode(symbol, lex->current_token_location);
}

static ASTNode* ParseStringLiteral(Assembler* assembler) {
  Lex* lex = assembler->syntax.lex;
  String* contents = NewString(lex->spelling.value);
  LexNextToken(lex);

  // Adjacent string literals are joined together.
  while (LexLookingAt(lex, TOK(string))) {
    StringAppend(contents, lex->spelling.value);
    LexNextToken(lex);
  }

  TypeRecord* array =
      NewBasicArrayTypeRecord(kQualPlain, (int)contents->length + 1, false);
  TypeRecord* type = NewTypeRecord(kTypeChar, kQualPlain);
  TypeRecordChain(array, type);
  return NewStringConstantASTNode(
      contents, array, assembler->syntax.lex->current_token_location);
}

static ASTNode* ParseWideStringLiteral(Assembler* assembler) {
  Lex* lex = assembler->syntax.lex;
  String* contents =
      NewStringWithLength(lex->spelling.value, lex->spelling.length + 4);
  LexNextToken(lex);

  // Adjacent wide string literals are joined together.
  while (LexLookingAt(lex, TOK(string_wide))) {
    StringAppend(contents, lex->spelling.value);
    LexNextToken(lex);
  }

  TypeRecord* array =
      NewBasicArrayTypeRecord(kQualPlain, (int)contents->length + 4, false);
  TypeRecord* type = NewTypeRecord(kTypeInt, kQualPlain);
  TypeRecordChain(array, type);
  return NewWideStringConstantASTNode(
      contents, array, assembler->syntax.lex->current_token_location);
}

static ASTNode* ParseCharacterConstant(Assembler* assembler) {
  Lex* lex = assembler->syntax.lex;
  int value = (int)lex->number;
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecord(kTypeChar, kQualPlain);
  return NewCharConstantASTNode(value, type,
                                assembler->syntax.lex->current_token_location);
}

static ASTNode* ParseWideCharacterConstant(Assembler* assembler) {
  Lex* lex = assembler->syntax.lex;
  int value = (int)lex->number;
  LexNextToken(lex);
  TypeRecord* type = NewTypeRecord(kTypeInt, kQualPlain);
  return NewCharConstantASTNode(value, type,
                                assembler->syntax.lex->current_token_location);
}

static ASTNode* ParsePrimaryExpression(Assembler* assembler) {
  Lex* lex = assembler->syntax.lex;

  // Check for parenthesized expression.
  // This either looks at the flag 'found_open_paren' in the Syntax
  // struct or looks for an open paren.  The syntax is slightly ambiguous
  // and the open paren can be the start of a cast expression or postfix
  // expression.
  if (assembler->syntax.found_open_paren || LexMatch(lex, TOK(lparen))) {
    assembler->syntax.found_open_paren = false;
    ASTNode* node = ParseExpression(assembler);
    SyntaxNeedBracket(&assembler->syntax, TOK(rparen), TC(closebra));
    return node;
  }

  // Check for identifier.
  if (LexLookingAt(lex, TOK(identifier))) {
    return ParseIdentifier(assembler);
  }

  // Integer constant, or GNU `Nb` / `Nf` local-label reference.
  if (LexLookingAt(lex, TOK(number))) {
    int64_t value = lex->number;
    SourceLocation location = lex->current_token_location;
    LexNextToken(lex);
    if (LexLookingAt(lex, TOK(identifier)) && lex->spelling.value[0] != '\0' &&
        lex->spelling.value[1] == '\0') {
      char direction = lex->spelling.value[0];
      if (direction == 'b' || direction == 'B' || direction == 'f' ||
          direction == 'F') {
        bool backward = direction == 'b' || direction == 'B';
        LexNextToken(lex);
        AssemblerSymbol* symbol =
            AssemblerLookupNumericLocalLabel(assembler, (int)value, backward);
        if (symbol == NULL) {
          TypeRecord* type = NewTypeRecord(kTypeLong | kTypeUnsigned, kQualPlain);
          return NewIntConstantASTNode(0, type, location);
        }
        return NewRawIdentifierASTNode(symbol, location);
      }
    }
    TypeRecord* type = NewTypeRecord(kTypeLong | kTypeUnsigned, kQualPlain);
    return NewIntConstantASTNode(value, type, location);
  }

  // Check for string literal.
  if (LexLookingAt(lex, TOK(string))) {
    return ParseStringLiteral(assembler);
  }

  // Wide string literal
  if (LexLookingAt(lex, TOK(string_wide))) {
    return ParseWideStringLiteral(assembler);
  }

  // Character constant.
  if (LexLookingAt(lex, TOK(charconst))) {
    return ParseCharacterConstant(assembler);
  }

  // Wide character constant.
  if (LexLookingAt(lex, TOK(charconst_wide))) {
    return ParseWideCharacterConstant(assembler);
  }

  // Invalid primary expression, error out, recover and return 0.
  AssemblerError(assembler,
                 "Expression syntax error; primary expression expected");
  TypeRecord* type = NewTypeRecord(kTypeInt | kTypeUnknown, kQualPlain);
  return (ASTNode*)NewIntConstantASTNode(
      0, type, assembler->syntax.lex->current_token_location);
}

static ASTNode* ParseUnaryExpression(Assembler* assembler) {
  if (LexMatch(assembler->syntax.lex, TOK(plus))) {
    ASTNode* sub = ParseUnaryExpression(assembler);
    return NewUnaryASTNode(AST_OP(uplus), NULL,
                           assembler->syntax.lex->current_token_location, sub);
  }

  if (LexMatch(assembler->syntax.lex, TOK(minus))) {
    ASTNode* sub = ParseUnaryExpression(assembler);
    return NewUnaryASTNode(AST_OP(uminus), NULL,
                           assembler->syntax.lex->current_token_location, sub);
  }

  if (LexMatch(assembler->syntax.lex, TOK(tilde))) {
    ASTNode* sub = ParseUnaryExpression(assembler);
    return NewUnaryASTNode(AST_OP(onescomp), NULL,
                           assembler->syntax.lex->current_token_location, sub);
  }

  if (LexMatch(assembler->syntax.lex, TOK(bang))) {
    ASTNode* sub = ParseUnaryExpression(assembler);
    return NewUnaryASTNode(AST_OP(not), NULL,
                           assembler->syntax.lex->current_token_location, sub);
  }

  return ParsePrimaryExpression(assembler);
}

static ASTNode* ParseMultiplicativeExpression(Assembler* assembler) {
  ASTNode* result = ParseUnaryExpression(assembler);
  for (;;) {
    if (LexMatch(assembler->syntax.lex, TOK(star))) {
      ASTNode* right = ParseUnaryExpression(assembler);
      result = NewBinaryASTNode(AST_OP(mult), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(slash))) {
      ASTNode* right = ParseUnaryExpression(assembler);
      result = NewBinaryASTNode(AST_OP(div), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(percent))) {
      ASTNode* right = ParseUnaryExpression(assembler);
      result = NewBinaryASTNode(AST_OP(mod), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseAdditiveExpression(Assembler* assembler) {
  ASTNode* result = ParseMultiplicativeExpression(assembler);
  for (;;) {
    if (LexMatch(assembler->syntax.lex, TOK(plus))) {
      ASTNode* right = ParseMultiplicativeExpression(assembler);
      result = NewBinaryASTNode(AST_OP(plus), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(minus))) {
      ASTNode* right = ParseMultiplicativeExpression(assembler);
      result = NewBinaryASTNode(AST_OP(minus), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseShiftExpression(Assembler* assembler) {
  ASTNode* result = ParseAdditiveExpression(assembler);
  for (;;) {
    if (LexMatch(assembler->syntax.lex, TOK(lessless))) {
      ASTNode* right = ParseAdditiveExpression(assembler);
      result = NewBinaryASTNode(AST_OP(lshift), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(greatergreater))) {
      ASTNode* right = ParseAdditiveExpression(assembler);
      result = NewBinaryASTNode(AST_OP(rshiftl), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseRelationalExpression(Assembler* assembler) {
  ASTNode* result = ParseShiftExpression(assembler);
  for (;;) {
    if (LexMatch(assembler->syntax.lex, TOK(less))) {
      ASTNode* right = ParseShiftExpression(assembler);
      result = NewBinaryASTNode(AST_OP(less), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(lesseq))) {
      ASTNode* right = ParseShiftExpression(assembler);
      result = NewBinaryASTNode(AST_OP(lesseq), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(greater))) {
      ASTNode* right = ParseShiftExpression(assembler);
      result = NewBinaryASTNode(AST_OP(greater), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(greatereq))) {
      ASTNode* right = ParseShiftExpression(assembler);
      result = NewBinaryASTNode(AST_OP(greatereq), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseEqualityExpression(Assembler* assembler) {
  ASTNode* result = ParseRelationalExpression(assembler);
  for (;;) {
    if (LexMatch(assembler->syntax.lex, TOK(equalequal))) {
      ASTNode* right = ParseRelationalExpression(assembler);
      result = NewBinaryASTNode(AST_OP(equal), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else if (LexMatch(assembler->syntax.lex, TOK(bangeq))) {
      ASTNode* right = ParseRelationalExpression(assembler);
      result = NewBinaryASTNode(AST_OP(noteq), NULL,
                                assembler->syntax.lex->current_token_location,
                                result, right);
    } else {
      break;
    }
  }
  return result;
}

static ASTNode* ParseAndExpression(Assembler* assembler) {
  ASTNode* result = ParseEqualityExpression(assembler);
  while (LexMatch(assembler->syntax.lex, TOK(amp))) {
    ASTNode* right = ParseEqualityExpression(assembler);
    result = NewBinaryASTNode(AST_OP(and), NULL,
                              assembler->syntax.lex->current_token_location,
                              result, right);
  }
  return result;
}

static ASTNode* ParseExclusiveOrExpression(Assembler* assembler) {
  ASTNode* result = ParseAndExpression(assembler);
  while (LexMatch(assembler->syntax.lex, TOK(caret))) {
    ASTNode* right = ParseAndExpression(assembler);
    result = NewBinaryASTNode(AST_OP(exor), NULL,
                              assembler->syntax.lex->current_token_location,
                              result, right);
  }
  return result;
}

static ASTNode* ParseInclusiveOrExpression(Assembler* assembler) {
  ASTNode* result = ParseExclusiveOrExpression(assembler);
  while (LexMatch(assembler->syntax.lex, TOK(bar))) {
    ASTNode* right = ParseExclusiveOrExpression(assembler);
    result = NewBinaryASTNode(AST_OP(bitor), NULL,
                              assembler->syntax.lex->current_token_location,
                              result, right);
  }
  return result;
}

static ASTNode* ParseLogicalAndExpression(Assembler* assembler) {
  ASTNode* result = ParseInclusiveOrExpression(assembler);
  while (LexMatch(assembler->syntax.lex, TOK(ampamp))) {
    ASTNode* right = ParseInclusiveOrExpression(assembler);
    result = NewBinaryASTNode(AST_OP(logand), NULL,
                              assembler->syntax.lex->current_token_location,
                              result, right);
  }
  return result;
}

static ASTNode* ParseLogicalOrExpression(Assembler* assembler) {
  ASTNode* result = ParseLogicalAndExpression(assembler);
  while (LexMatch(assembler->syntax.lex, TOK(barbar))) {
    ASTNode* right = ParseLogicalAndExpression(assembler);
    result = NewBinaryASTNode(AST_OP(logor), NULL,
                              assembler->syntax.lex->current_token_location,
                              result, right);
  }
  return result;
}

static ASTNode* ParseConditionalExpression(Assembler* assembler) {
  ASTNode* result = ParseLogicalOrExpression(assembler);
  if (LexMatch(assembler->syntax.lex, TOK(question))) {
    ASTNode* left = ParseExpression(assembler);
    ASTNode* right = NULL;
    if (LexMatch(assembler->syntax.lex, TOK(colon))) {
      right = ParseConditionalExpression(assembler);
      right = NewBinaryASTNode(AST_OP(colon), NULL,
                               assembler->syntax.lex->current_token_location,
                               left, right);
    } else {
      AssemblerError(assembler, "Missing : in conditional expression");
    }
    result = NewBinaryASTNode(AST_OP(question), NULL,
                              assembler->syntax.lex->current_token_location,
                              result, right);
  }
  return result;
}

static ASTNode* ParseExpression(Assembler* assembler) {
  return ParseConditionalExpression(assembler);
}

static bool EvaluateExpression(Assembler* assembler, ASTNode* node, int depth, int64_t* result) {
  ConstantASTNode* const_node = (ConstantASTNode*)node;
  IdentifierASTNode* id_node = (IdentifierASTNode*)node;
  BinaryASTNode* binary_node = (BinaryASTNode*)node;
  UnaryASTNode* unary_node = (UnaryASTNode*)node;

  int64_t left;
  int64_t right;

  switch (node->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
      *result = const_node->value.ivalue;
      return true;
    case AST_OP(fnumber):
      *result = (int64_t)const_node->value.fvalue;
      return true;
    case AST_OP(identifier): {
      AssemblerSymbol* asm_sym = (AssemblerSymbol*)id_node->symbol;
#if 0
      if (asm_sym->is_label && depth > 0) {
        AssemblerError(assembler, "Use of label %s in expression", asm_sym->name.value);
        return false;
      }
#endif
      if (!asm_sym->defined) {
        return false;
      }
      *result = asm_sym->value;
      return true;
    }

    // Macros are always evaluated as the constant zero.
    case AST_OP(macro):
      *result = 0;
      return true;

#define EVAL_BINARY_OP(ast_op, op)                        \
  case AST_OP(ast_op):                                    \
    if (EvaluateExpression(assembler,binary_node->left, depth+1,&left) &&   \
        EvaluateExpression(assembler,binary_node->right, depth+1,&right)) { \
      *result = left op right;                            \
      return true;                                        \
    }                                                     \
    break;

      EVAL_BINARY_OP(plus, +)
      EVAL_BINARY_OP(minus, -)
      EVAL_BINARY_OP(mult, *)

    case AST_OP(div):
      if (EvaluateExpression(assembler,binary_node->left, depth+1, &left) &&
          EvaluateExpression(assembler,binary_node->right, depth+1, &right)) {
        if (right == 0) {
          return false;
        }
        *result = left / right;
        return true;
      }
      break;

    case AST_OP(mod):
      if (EvaluateExpression(assembler,binary_node->left, depth+1,&left) &&
          EvaluateExpression(assembler,binary_node->right, depth+1,&right)) {
        if (right == 0) {
          return false;
        }
        *result = left % right;
        return true;
      }
      break;

      EVAL_BINARY_OP(lshift, <<)

    case AST_OP(rshiftl):
      if (EvaluateExpression(assembler,binary_node->left, depth+1,&left) &&
          EvaluateExpression(assembler,binary_node->right,depth+1, &right)) {
        *result = (uint64_t)left >> right;
        return true;
      }
      break;

      EVAL_BINARY_OP(less, <)
      EVAL_BINARY_OP(lesseq, <=)
      EVAL_BINARY_OP(greater, >)
      EVAL_BINARY_OP(greatereq, >=)
      EVAL_BINARY_OP(equal, ==)
      EVAL_BINARY_OP(noteq, !=)
      EVAL_BINARY_OP(and, &)
      EVAL_BINARY_OP(bitor, |)
      EVAL_BINARY_OP(exor, ^)

#define EVAL_UNARY_OP(ast_op, op)                     \
  case AST_OP(ast_op):                                \
    if (EvaluateExpression(assembler,unary_node->sub, depth+1, &left)) { \
      *result = op left;                              \
      return true;                                    \
    }                                                 \
    break;

      EVAL_UNARY_OP(uminus, -)
      EVAL_UNARY_OP(uplus, +)
      EVAL_UNARY_OP(not, !)
      EVAL_UNARY_OP(onescomp, ~)

    case AST_OP(logand):
      if (EvaluateExpression(assembler,binary_node->left,depth+1, &left)) {
        if (left != 0) {
          if (EvaluateExpression(assembler,binary_node->right, depth+1,&right)) {
            *result = right != 0;
            return true;
          }
        }
      }
      break;

    case AST_OP(logor):
      if (EvaluateExpression(assembler,binary_node->left, depth+1,&left)) {
        if (left == 0) {
          if (EvaluateExpression(assembler,binary_node->right, depth+1,&right)) {
            *result = right != 0;
            return true;
          }
        } else {
          *result = 1;
          return true;
        }
      }
      break;

    case AST_OP(question):
      if (EvaluateExpression(assembler,binary_node->left, depth+1,&left)) {
        // Depending on the value of left (the left for the ? operator) we
        // either evaluate the left or right of the colon operator (right right
        // node of the ? operator).
        if (left != 0) {
          node = ((BinaryASTNode*)binary_node->right)->left;
        } else {
          node = ((BinaryASTNode*)binary_node->right)->right;
        }
        if (EvaluateExpression(assembler,node,depth+1, &left)) {
          *result = left;
          return true;
        }
      }
      break;

    default:
      return false;
  }
  return false;
}

bool AssemblerEvaluateExpressionInternal(Assembler* assembler,
                                         int64_t* result) {
  ASTNode* node = ParseExpression(assembler);
  if (node == NULL) {
    return false;
  }
  bool v = EvaluateExpression(assembler, node, 0, result);
  ASTNodeDelete(node);
  return v;
}

#undef EVAL_BINARY_OP
#undef EVAL_UNARY_OP
