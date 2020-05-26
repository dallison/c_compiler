//
//  expr_evaluator.c
//  c_compiler
//
//  Created by David Allison on 11/3/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "expr_evaluator.h"
#include "assembler.h"
#include "type.h"

bool EvaluateIntegerExpression(ASTNode* node, int64_t* result) {
  if (node == NULL) {
    return false;
  }
  if (node->type == NULL) {
    return false;
  }
  if (!TypeIsIntegral(node->type) && !TypeIsFloatingPoint(node->type)) {
    return false;
  }
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
      if (StorageIs(id_node->symbol->storage, STO(assembler))) {
        // Assembler symbol, extract the value from the 'other'
        // value field.
        AssemblerSymbol* asm_sym = id_node->symbol->value.other;
        if (asm_sym == NULL) {
          return false;
        }
        if (!asm_sym->defined) {
          return false;
        }
        *result = asm_sym->value;
        return true;
      }
      TypeRecord* type = id_node->symbol->type;
      if ((type->qualifiers & kQualConst) == 0) {
        return false;
      }
      if (TypeIsIntegral(type)) {
        *result = id_node->symbol->value.ivalue;
      } else if (TypeIsFloatingPoint(type)) {
        *result = id_node->symbol->value.fvalue;
      } else {
        return false;
      }
      return true;
    }

    // Macros are always evaluated as the constant zero.
    case AST_OP(macro):
      *result = 0;
      return true;

#define EVAL_BINARY_OP(ast_op, op) \
    case AST_OP(ast_op): \
      if (EvaluateIntegerExpression(binary_node->left, &left) && \
          EvaluateIntegerExpression(binary_node->right, &right)) { \
        *result = left op right; \
        return true; \
      } \
      break;

      EVAL_BINARY_OP(plus, +)
      EVAL_BINARY_OP(minus, -)
      EVAL_BINARY_OP(mult, *)


    case AST_OP(div):
      if (EvaluateIntegerExpression(binary_node->left, &left) &&
          EvaluateIntegerExpression(binary_node->right, &right)) {
        if (right == 0) {
          return false;
        }
        *result = left / right;
        return true;
      }
      break;

    case AST_OP(mod):
      if (EvaluateIntegerExpression(binary_node->left, &left) &&
          EvaluateIntegerExpression(binary_node->right, &right)) {
        if (right == 0) {
          return false;
        }
        *result = left % right;
        return true;
      }
      break;

      EVAL_BINARY_OP(lshift, <<)
      EVAL_BINARY_OP(rshifta, <<)

  
    case AST_OP(rshiftl):
      if (EvaluateIntegerExpression(binary_node->left, &left) &&
          EvaluateIntegerExpression(binary_node->right, &right)) {
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

#define EVAL_UNARY_OP(ast_op, op) \
case AST_OP(ast_op): \
      if (EvaluateIntegerExpression(unary_node->sub, &left)) { \
        *result = op left; \
        return true; \
      } \
      break;
      
      EVAL_UNARY_OP(uminus, -)
      EVAL_UNARY_OP(uplus, +)
      EVAL_UNARY_OP(not, !)
      EVAL_UNARY_OP(onescomp, ~)

    case AST_OP(logand):
      if (EvaluateIntegerExpression(binary_node->left, &left)) {
        if (left != 0) {
          if (EvaluateIntegerExpression(binary_node->right, &right)) {
            *result = right != 0;
            return true;
          }
        }
      }
      break;

    case AST_OP(logor):
      if (EvaluateIntegerExpression(binary_node->left, &left)) {
        if (left == 0) {
          if (EvaluateIntegerExpression(binary_node->right, &right)) {
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
      if (EvaluateIntegerExpression(binary_node->left, &left)) {
        // Depending on the value of left (the left for the ? operator) we
        // either evaluate the left or right of the colon operator (right right
        // node of the ? operator).
        if (left != 0) {
          node = ((BinaryASTNode*)binary_node->right)->left;
        } else {
          node = ((BinaryASTNode*)binary_node->right)->right;
        }
        if (EvaluateIntegerExpression(node, &left)) {
          *result = left;
          return true;
        }
      }
      break;

    case AST_OP(cast): {
      CastASTNode* c = (CastASTNode*)node;
      if (EvaluateIntegerExpression(c->expr, &left)) {
        *result = left;
        return true;
      }
      break;
    }

 
    // Conversions.
    case AST_OP(i2b):
    case AST_OP(s2b):
    case AST_OP(l2b):
    case AST_OP(ll2b):
    case AST_OP(f2b):
    case AST_OP(d2b):
    case AST_OP(ld2b):
      if (EvaluateIntegerExpression(unary_node->sub, &left)) {
        *result = left != 0 ? 1 : 0;
        return true;
      }
      break;

      
      EVAL_UNARY_OP(b2i, (int))
      EVAL_UNARY_OP(s2i, (int))
      EVAL_UNARY_OP(l2i, (int))
      EVAL_UNARY_OP(ll2i, (int))
      EVAL_UNARY_OP(f2i, (int))
      EVAL_UNARY_OP(d2i, (int))
      EVAL_UNARY_OP(ld2i, (int))

      EVAL_UNARY_OP(b2s, (short))
      EVAL_UNARY_OP(i2s, (short))
      EVAL_UNARY_OP(l2s, (short))
      EVAL_UNARY_OP(ll2s, (short))
      EVAL_UNARY_OP(f2s, (short))
      EVAL_UNARY_OP(d2s, (short))
      EVAL_UNARY_OP(ld2s, (short))

      EVAL_UNARY_OP(b2l, (long))
      EVAL_UNARY_OP(i2l, (long))
      EVAL_UNARY_OP(s2l, (long))
      EVAL_UNARY_OP(ll2l, (long))
      EVAL_UNARY_OP(f2l, (long))
      EVAL_UNARY_OP(d2l, (long))
      EVAL_UNARY_OP(ld2l, (long))

 
      EVAL_UNARY_OP(b2ll, (long long))
      EVAL_UNARY_OP(i2ll, (long long))
      EVAL_UNARY_OP(s2ll, (long long))
      EVAL_UNARY_OP(l2ll, (long long))
      EVAL_UNARY_OP(f2ll, (long long))
      EVAL_UNARY_OP(d2ll, (long long))
      EVAL_UNARY_OP(ld2ll, (long long))

    case AST_OP(sizeof): {
      SizeofASTNode* snode = (SizeofASTNode*)node;
      *result = snode->base.value.ivalue;
      return true;
    }

    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)node;
      return EvaluateIntegerExpression(e->expr, result);
      break;
    }
    default:
      return false;
  }
  return false;
}

#undef EVAL_BINARY_OP
#undef EVAL_UNARY_OP

bool EvaluateFloatingPointExpression(ASTNode* node, double* result) {
  if (node == NULL) {
    return false;
  }
  if (node->type == NULL) {
    return false;
  }
  if (!TypeIsIntegral(node->type) && !TypeIsFloatingPoint(node->type)) {
    return false;
  }
  ConstantASTNode* const_node = (ConstantASTNode*)node;
  IdentifierASTNode* id_node = (IdentifierASTNode*)node;
  BinaryASTNode* binary_node = (BinaryASTNode*)node;
  UnaryASTNode* unary_node = (UnaryASTNode*)node;

  double left;
  double right;

  switch (node->op) {
    case AST_OP(number):
    case AST_OP(charconst):
      *result = const_node->value.ivalue;
      return true;
    case AST_OP(fnumber):
      *result = (int64_t)const_node->value.fvalue;
      return true;
    case AST_OP(identifier): {
      TypeRecord* type = id_node->symbol->type;
      if ((type->qualifiers & kQualConst) == 0) {
        return false;
      }
      if (TypeIsIntegral(type)) {
        *result = id_node->symbol->value.ivalue;
      } else if (TypeIsFloatingPoint(type)) {
        *result = id_node->symbol->value.fvalue;
      } else {
        return false;
      }
      return true;
    }

#define EVAL_BINARY_OP(ast_op, op) \
  case AST_OP(ast_op): \
    if (EvaluateFloatingPointExpression(binary_node->left, &left) && \
        EvaluateFloatingPointExpression(binary_node->right, &right)) { \
      *result = left op right; \
      return true; \
    } \
    break;

      EVAL_BINARY_OP(plus, +)
      EVAL_BINARY_OP(minus, -)
      EVAL_BINARY_OP(mult, *)
      
    case AST_OP(div):
      if (EvaluateFloatingPointExpression(binary_node->left, &left) &&
          EvaluateFloatingPointExpression(binary_node->right, &right)) {
        if (right == 0) {
          return false;
        }
        *result = left / right;
        return true;
      }
      break;

      EVAL_BINARY_OP(less, <)
      EVAL_BINARY_OP(lesseq, <=)
      EVAL_BINARY_OP(greater, >)
      EVAL_BINARY_OP(greatereq, >=)
      EVAL_BINARY_OP(equal, ==)
      EVAL_BINARY_OP(noteq, !=)

#define EVAL_UNARY_OP(ast_op, op) \
  case AST_OP(ast_op): \
    if (EvaluateFloatingPointExpression(unary_node->sub, &left)) { \
      *result = op left; \
      return true; \
    } \
  break;
      
      EVAL_UNARY_OP(uminus, -)
      EVAL_UNARY_OP(uplus, +)
      EVAL_UNARY_OP(not, !)


    case AST_OP(logand):
      if (EvaluateFloatingPointExpression(binary_node->left, &left)) {
        if (left != 0) {
          if (EvaluateFloatingPointExpression(binary_node->right, &right)) {
            *result = right != 0;
            return true;
          }
        }
      }
      break;

    case AST_OP(logor):
      if (EvaluateFloatingPointExpression(binary_node->left, &left)) {
        if (left == 0) {
          if (EvaluateFloatingPointExpression(binary_node->right, &right)) {
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
      if (EvaluateFloatingPointExpression(binary_node->left, &left)) {
        // Depending on the value of left (the left for the ? operator) we
        // either evaluate the left or right of the colon operator (right right
        // node of the ? operator).
        if (left != 0) {
          node = ((BinaryASTNode*)binary_node->right)->left;
        } else {
          node = ((BinaryASTNode*)binary_node->right)->right;
        }
        if (EvaluateFloatingPointExpression(node, &left)) {
          *result = left;
          return true;
        }
      }
      break;

      EVAL_UNARY_OP(cast, )

      EVAL_UNARY_OP(b2f, (float))
      EVAL_UNARY_OP(i2f, (float))
      EVAL_UNARY_OP(s2f, (float))
      EVAL_UNARY_OP(l2f, (float))
      EVAL_UNARY_OP(ll2f, (float))
      EVAL_UNARY_OP(d2f, (float))
      EVAL_UNARY_OP(ld2f, (float))

 
      EVAL_UNARY_OP(b2d, (double))
      EVAL_UNARY_OP(i2d, (double))
      EVAL_UNARY_OP(s2d, (double))
      EVAL_UNARY_OP(l2d, (double))
      EVAL_UNARY_OP(ll2d, (double))
      EVAL_UNARY_OP(f2d, (double))
      EVAL_UNARY_OP(ld2d, (double))

      EVAL_UNARY_OP(b2ld, (double))
      EVAL_UNARY_OP(i2ld, (double))
      EVAL_UNARY_OP(s2ld, (double))
      EVAL_UNARY_OP(l2ld, (double))
      EVAL_UNARY_OP(ll2ld, (double))
      EVAL_UNARY_OP(f2ld, (double))
      EVAL_UNARY_OP(d2ld, (double))

      case AST_OP(expr_init): {
         ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)node;
         return EvaluateFloatingPointExpression(e->expr, result);
         break;
       }

    default:
      return false;
  }
  return false;
}

#undef EVAL_BINARY_OP
#undef EVAL_UNARY_OP

