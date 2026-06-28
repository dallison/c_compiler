//
//  expr_evaluator.c
//  c_compiler
//
//  Created by David Allison on 11/3/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "expr_evaluator.h"
#include "assembler.h"
#include "constexpr.h"
#include "type.h"

bool EvaluateFloatingPointExpressionInContext(ConstEvalContext* ctx,
                                              ASTNode* node,
                                              double* result);

static SourceLocation BuiltinSourceLocation(ASTNode* node) {
  ASTNode* current = node;
  while (current != NULL && (current->flags & kASTDefaultArgument) != 0 &&
         current->parent != NULL) {
    current = current->parent;
  }
  return current != NULL ? current->location : SOURCE_LOCATION_MISSING;
}

static Symbol* EvaluatorCallSymbol(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(call)) {
    return NULL;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL) {
    return NULL;
  }
  if (call->left->op == AST_OP(identifier)) {
    return ((IdentifierASTNode*)call->left)->symbol;
  }
  if (call->left->op == AST_OP(dot) || call->left->op == AST_OP(arrow)) {
    BinaryASTNode* access = (BinaryASTNode*)call->left;
    if (access->right != NULL && access->right->op == AST_OP(structmember)) {
      StructMember* member = ((StructMemberASTNode*)access->right)->member;
      return member != NULL ? member->symbol : NULL;
    }
  }
  return NULL;
}

static bool FunctionOwnerIsSourceLocation(TypeRecord* func) {
  Struct* owner = func != NULL ? func->info.function.cxx_member_owner : NULL;
  return owner != NULL && owner->tag_name != NULL &&
         StringEqual(owner->tag_name, "source_location");
}

static Symbol* SourceLocationReceiverCallSymbol(ASTNode* receiver) {
  if (receiver == NULL) {
    return NULL;
  }
  if (receiver->op == AST_OP(address)) {
    return SourceLocationReceiverCallSymbol(((UnaryASTNode*)receiver)->sub);
  }
  if (receiver->op == AST_OP(comma)) {
    BinaryASTNode* comma = (BinaryASTNode*)receiver;
    Symbol* right = SourceLocationReceiverCallSymbol(comma->right);
    return right != NULL ? right : SourceLocationReceiverCallSymbol(comma->left);
  }
  return EvaluatorCallSymbol(receiver);
}

static bool EvaluateSourceLocationAccessor(ASTNode* node, int64_t* result) {
  if (node == NULL || node->op != AST_OP(call) || result == NULL) {
    return false;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  ASTNode* receiver = NULL;
  Symbol* accessor_symbol = NULL;
  if (call->left != NULL && call->left->op == AST_OP(identifier)) {
    accessor_symbol = ((IdentifierASTNode*)call->left)->symbol;
    if (call->children != NULL && call->children->length > 0) {
      receiver = call->children->value.p[0];
      if (receiver != NULL && receiver->op == AST_OP(address)) {
        receiver = ((UnaryASTNode*)receiver)->sub;
      }
    }
  } else if (call->left != NULL &&
             (call->left->op == AST_OP(dot) ||
              call->left->op == AST_OP(arrow))) {
    BinaryASTNode* access = (BinaryASTNode*)call->left;
    receiver = access->left;
    if (access->right == NULL || access->right->op != AST_OP(structmember)) {
      return false;
    }
    StructMember* accessor = ((StructMemberASTNode*)access->right)->member;
    accessor_symbol = accessor != NULL ? accessor->symbol : NULL;
  }
  if (accessor_symbol == NULL || accessor_symbol->type == NULL ||
      !TypeIsFunction(accessor_symbol->type) ||
      !FunctionOwnerIsSourceLocation(accessor_symbol->type)) {
    return false;
  }
  bool want_line = StringEqual(&accessor_symbol->name, "line");
  bool want_column = StringEqual(&accessor_symbol->name, "column");
  if (!want_line && !want_column) {
    return false;
  }

  Symbol* receiver_symbol = SourceLocationReceiverCallSymbol(receiver);
  if (receiver_symbol == NULL || receiver_symbol->type == NULL ||
      !TypeIsFunction(receiver_symbol->type) ||
      !FunctionOwnerIsSourceLocation(receiver_symbol->type)) {
    return false;
  }
  if (StringEqual(&receiver_symbol->name, "current")) {
    int fileno = 0;
    int lineno = 0;
    int colno = 0;
    SourceLocationNumbers(BuiltinSourceLocation(receiver), &fileno, &lineno,
                          &colno);
    (void)fileno;
    *result = want_line ? lineno : colno + 1;
    return true;
  }
  if (receiver_symbol->type->info.function.is_constructor) {
    *result = 0;
    return true;
  }
  return false;
}

bool EvaluateIntegerExpressionInContext(ConstEvalContext* ctx,
                                               ASTNode* node,
                                               int64_t* result) {
  if (node == NULL) {
    return false;
  }
  if (!ConstEvalStep(ctx)) {
    return false;
  }
  if (node->type != NULL &&
      !TypeIsIntegral(node->type) && !TypeIsFloatingPoint(node->type)) {
    return false;
  }
  ConstantASTNode* const_node = (ConstantASTNode*)node;
  IdentifierASTNode* id_node = (IdentifierASTNode*)node;
  BinaryASTNode* binary_node = (BinaryASTNode*)node;
  UnaryASTNode* unary_node = (UnaryASTNode*)node;

  int64_t left;
  int64_t right;

  switch (node->op) {
    case AST_OP(builtin_source_line): {
      int fileno = 0;
      int lineno = 0;
      int colno = 0;
      SourceLocationNumbers(BuiltinSourceLocation(node), &fileno, &lineno,
                            &colno);
      (void)fileno;
      (void)colno;
      *result = lineno;
      return true;
    }
    case AST_OP(builtin_source_column): {
      int fileno = 0;
      int lineno = 0;
      int colno = 0;
      SourceLocationNumbers(BuiltinSourceLocation(node), &fileno, &lineno,
                            &colno);
      (void)fileno;
      (void)lineno;
      *result = colno + 1;
      return true;
    }

    case AST_OP(assign):
    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(postinc):
    case AST_OP(postdec):
    case AST_OP(comma): {
      return ConstexprEvaluateMutationAsInteger(ctx, node, node->type, result);
    }
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
      *result = const_node->value.ivalue;
      return true;
    case AST_OP(fnumber):
      *result = (int64_t)const_node->value.fvalue;
      return true;
    case AST_OP(subscript):
    case AST_OP(dot):
    case AST_OP(arrow):
      return ConstexprEvaluateObjectAccessAsInteger(ctx, node, result);
    case AST_OP(contents):
      return ConstexprEvaluatePointerDereferenceAsInteger(ctx, node, result);
    case AST_OP(identifier): {
      if (ConstexprBindingAsInteger(ctx, id_node->symbol, result)) {
        return true;
      }
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
      if (!id_node->symbol->flags.value_set) {
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

    case AST_OP(call): {
      if (EvaluateSourceLocationAccessor(node, result)) {
        return true;
      }
      return ConstexprEvaluateCallAsInteger(ctx, node, result);
    }

    // Macros are always evaluated as the constant zero.
    case AST_OP(macro):
      *result = 0;
      return true;

#define EVAL_BINARY_OP(ast_op, op) \
    case AST_OP(ast_op): \
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left) && \
          EvaluateIntegerExpressionInContext(ctx, binary_node->right, &right)) { \
        *result = left op right; \
        return true; \
      } \
      break;

      EVAL_BINARY_OP(plus, +)
      EVAL_BINARY_OP(minus, -)
      EVAL_BINARY_OP(mult, *)


    case AST_OP(div):
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left) &&
          EvaluateIntegerExpressionInContext(ctx, binary_node->right, &right)) {
        if (right == 0) {
          return false;
        }
        *result = left / right;
        return true;
      }
      break;

    case AST_OP(mod):
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left) &&
          EvaluateIntegerExpressionInContext(ctx, binary_node->right, &right)) {
        if (right == 0) {
          return false;
        }
        *result = left % right;
        return true;
      }
      break;

      EVAL_BINARY_OP(lshift, <<)
      EVAL_BINARY_OP(rshifta, >>)

  
    case AST_OP(rshiftl):
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left) &&
          EvaluateIntegerExpressionInContext(ctx, binary_node->right, &right)) {
        *result = (uint64_t)left >> right;
        return true;
      }
      break;

      EVAL_BINARY_OP(less, <)
      EVAL_BINARY_OP(lesseq, <=)
      EVAL_BINARY_OP(greater, >)
      EVAL_BINARY_OP(greatereq, >=)
    case AST_OP(equal):
    case AST_OP(noteq):
      if (ConstexprEvaluatePointerComparison(ctx, node, result)) {
        return true;
      }
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left) &&
          EvaluateIntegerExpressionInContext(ctx, binary_node->right, &right)) {
        *result = node->op == AST_OP(equal) ? left == right : left != right;
        return true;
      }
      break;
      EVAL_BINARY_OP(and, &)
      EVAL_BINARY_OP(bitor, |)
      EVAL_BINARY_OP(exor, ^)

#define EVAL_UNARY_OP(ast_op, op) \
case AST_OP(ast_op): \
      if (EvaluateIntegerExpressionInContext(ctx, unary_node->sub, &left)) { \
        *result = op left; \
        return true; \
      } \
      break;
      
      EVAL_UNARY_OP(uminus, -)
      EVAL_UNARY_OP(uplus, +)
      EVAL_UNARY_OP(not, !)
      EVAL_UNARY_OP(onescomp, ~)
      EVAL_UNARY_OP(b2c, (char))
      EVAL_UNARY_OP(i2c, (char))
      EVAL_UNARY_OP(s2c, (char))
      EVAL_UNARY_OP(l2c, (char))
      EVAL_UNARY_OP(ll2c, (char))
      EVAL_UNARY_OP(c2b, (_Bool))

    case AST_OP(logand):
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left)) {
        if (left != 0) {
          if (EvaluateIntegerExpressionInContext(ctx, binary_node->right,
                                                 &right)) {
            *result = right != 0;
            return true;
          }
        }
      }
      break;

    case AST_OP(logor):
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left)) {
        if (left == 0) {
          if (EvaluateIntegerExpressionInContext(ctx, binary_node->right,
                                                 &right)) {
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
      if (EvaluateIntegerExpressionInContext(ctx, binary_node->left, &left)) {
        // Depending on the value of left (the left for the ? operator) we
        // either evaluate the left or right of the colon operator (right right
        // node of the ? operator).
        if (left != 0) {
          node = ((BinaryASTNode*)binary_node->right)->left;
        } else {
          node = ((BinaryASTNode*)binary_node->right)->right;
        }
        if (EvaluateIntegerExpressionInContext(ctx, node, &left)) {
          *result = left;
          return true;
        }
      }
      break;

    case AST_OP(cast): {
      CastASTNode* c = (CastASTNode*)node;
      if (EvaluateIntegerExpressionInContext(ctx, c->expr, &left)) {
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
      if (EvaluateIntegerExpressionInContext(ctx, unary_node->sub, &left)) {
        *result = left != 0 ? 1 : 0;
        return true;
      }
      break;

      
      EVAL_UNARY_OP(b2i, (int))
      EVAL_UNARY_OP(c2i, (int))
      EVAL_UNARY_OP(s2i, (int))
      EVAL_UNARY_OP(l2i, (int))
      EVAL_UNARY_OP(ll2i, (int))
      EVAL_UNARY_OP(f2i, (int))
      EVAL_UNARY_OP(d2i, (int))
      EVAL_UNARY_OP(ld2i, (int))

      EVAL_UNARY_OP(b2s, (short))
      EVAL_UNARY_OP(c2s, (short))
      EVAL_UNARY_OP(i2s, (short))
      EVAL_UNARY_OP(l2s, (short))
      EVAL_UNARY_OP(ll2s, (short))
      EVAL_UNARY_OP(f2s, (short))
      EVAL_UNARY_OP(d2s, (short))
      EVAL_UNARY_OP(ld2s, (short))

      EVAL_UNARY_OP(b2l, (long))
      EVAL_UNARY_OP(c2l, (long))
      EVAL_UNARY_OP(i2l, (long))
      EVAL_UNARY_OP(s2l, (long))
      EVAL_UNARY_OP(ll2l, (long))
      EVAL_UNARY_OP(f2l, (long))
      EVAL_UNARY_OP(d2l, (long))
      EVAL_UNARY_OP(ld2l, (long))

 
      EVAL_UNARY_OP(b2ll, (long long))
      EVAL_UNARY_OP(c2ll, (long long))
      EVAL_UNARY_OP(i2ll, (long long))
      EVAL_UNARY_OP(s2ll, (long long))
      EVAL_UNARY_OP(l2ll, (long long))
      EVAL_UNARY_OP(f2ll, (long long))
      EVAL_UNARY_OP(d2ll, (long long))
      EVAL_UNARY_OP(ld2ll, (long long))

    case AST_OP(sizeof): {
      SizeofASTNode* snode = (SizeofASTNode*)node;
      if (snode->is_pack_size) {
        return false;
      }
      if (snode->expr != NULL && TypeIsVLA(snode->expr->type)) {
        return false;
      }
      *result = snode->base.value.ivalue;
      return true;
    }

    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)node;
      return EvaluateIntegerExpressionInContext(ctx, e->expr, result);
      break;
    }
    default:
      return false;
  }
  return false;
}

#undef EVAL_BINARY_OP
#undef EVAL_UNARY_OP

bool EvaluateIntegerExpression(ASTNode* node, int64_t* result) {
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  bool ok = EvaluateIntegerExpressionInContext(&ctx, node, result);
  ConstEvalContextDestruct(&ctx);
  return ok;
}

bool EvaluateFloatingPointExpression(ASTNode* node, double* result) {
  ConstEvalContext ctx;
  ConstEvalContextInit(&ctx);
  bool ok = EvaluateFloatingPointExpressionInContext(&ctx, node, result);
  ConstEvalContextDestruct(&ctx);
  return ok;
}

bool EvaluateScalarConstantForSymbol(Symbol* symbol, ASTNode* initializer) {
  if (symbol == NULL || symbol->type == NULL || initializer == NULL) {
    return false;
  }
  initializer = ConstexprInitializerExpression(initializer);
  if (initializer == NULL) {
    return false;
  }
  if (TypeIsIntegral(symbol->type)) {
    symbol->flags.value_set =
        EvaluateIntegerExpression(initializer, &symbol->value.ivalue);
    return symbol->flags.value_set;
  }
  if (TypeIsFloatingPoint(symbol->type)) {
    symbol->flags.value_set =
        EvaluateFloatingPointExpression(initializer, &symbol->value.fvalue);
    return symbol->flags.value_set;
  }
  return false;
}

bool EvaluateFloatingPointExpressionInContext(ConstEvalContext* ctx, ASTNode* node, double* result) {
  if (node == NULL) {
    return false;
  }
  if (!ConstEvalStep(ctx)) {
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
    case AST_OP(assign):
    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(postinc):
    case AST_OP(postdec):
    case AST_OP(comma): {
      return ConstexprEvaluateMutationAsFloating(ctx, node, node->type, result);
    }
    case AST_OP(number):
    case AST_OP(charconst):
      *result = const_node->value.ivalue;
      return true;
    case AST_OP(fnumber):
      *result = const_node->value.fvalue;
      return true;
    case AST_OP(subscript):
    case AST_OP(dot):
    case AST_OP(arrow):
      return ConstexprEvaluateObjectAccessAsFloating(ctx, node, result);
    case AST_OP(contents):
      return ConstexprEvaluatePointerDereferenceAsFloating(ctx, node, result);
    case AST_OP(identifier): {
      if (ConstexprBindingAsFloating(ctx, id_node->symbol, result)) {
        return true;
      }
      TypeRecord* type = id_node->symbol->type;
      if ((type->qualifiers & kQualConst) == 0) {
        return false;
      }
      if (!id_node->symbol->flags.value_set) {
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

    case AST_OP(call): {
      return ConstexprEvaluateCallAsFloating(ctx, node, result);
    }

#define EVAL_BINARY_OP(ast_op, op) \
  case AST_OP(ast_op): \
    if (EvaluateFloatingPointExpressionInContext(ctx, binary_node->left, &left) && \
        EvaluateFloatingPointExpressionInContext(ctx, binary_node->right, &right)) { \
      *result = left op right; \
      return true; \
    } \
    break;

      EVAL_BINARY_OP(plus, +)
      EVAL_BINARY_OP(minus, -)
      EVAL_BINARY_OP(mult, *)
      
    case AST_OP(div):
      if (EvaluateFloatingPointExpressionInContext(ctx, binary_node->left, &left) &&
          EvaluateFloatingPointExpressionInContext(ctx, binary_node->right, &right)) {
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
    if (EvaluateFloatingPointExpressionInContext(ctx, unary_node->sub, &left)) { \
      *result = op left; \
      return true; \
    } \
  break;
      
      EVAL_UNARY_OP(uminus, -)
      EVAL_UNARY_OP(uplus, +)
      EVAL_UNARY_OP(not, !)


    case AST_OP(logand):
      if (EvaluateFloatingPointExpressionInContext(ctx, binary_node->left, &left)) {
        if (left != 0) {
          if (EvaluateFloatingPointExpressionInContext(ctx, binary_node->right, &right)) {
            *result = right != 0;
            return true;
          }
        }
      }
      break;

    case AST_OP(logor):
      if (EvaluateFloatingPointExpressionInContext(ctx, binary_node->left, &left)) {
        if (left == 0) {
          if (EvaluateFloatingPointExpressionInContext(ctx, binary_node->right, &right)) {
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
      if (EvaluateFloatingPointExpressionInContext(ctx, binary_node->left, &left)) {
        // Depending on the value of left (the left for the ? operator) we
        // either evaluate the left or right of the colon operator (right right
        // node of the ? operator).
        if (left != 0) {
          node = ((BinaryASTNode*)binary_node->right)->left;
        } else {
          node = ((BinaryASTNode*)binary_node->right)->right;
        }
        if (EvaluateFloatingPointExpressionInContext(ctx, node, &left)) {
          *result = left;
          return true;
        }
      }
      break;

    case AST_OP(cast): {
      CastASTNode* c = (CastASTNode*)node;
      if (EvaluateFloatingPointExpressionInContext(ctx, c->expr, &left)) {
        *result = left;
        return true;
      }
      break;
    }

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
         return EvaluateFloatingPointExpressionInContext(ctx, e->expr, result);
         break;
       }

    default:
      return false;
  }
  return false;
}

#undef EVAL_BINARY_OP
#undef EVAL_UNARY_OP
