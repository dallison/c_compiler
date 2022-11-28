//
//  expr_semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "expr_semantics.h"
#include <assert.h>
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "statement_semantics.h"
#include "compiler.h"

// This is the semantic analyzer for expressions.  It propagates type
// information from the leaves of the AST (Abstract Syntax Tree) up
// to the root of the tree.  The types of the leaf nodes are known
// before we do the analysis because they come from constants or
// symbols with known types.  Various operators modify the types
// as they propagate upwards in the tree.
//
// This also performs type conversion using the rules of the language.
// It also is responsible for checking that the semantic rules of the
// language are being followed.

static ASTNode* AnalyzeIdentifier(IdentifierASTNode* node) {
  if (node->base.parent == NULL || node->base.parent->op != AST_OP(init)) {
    // Symbol has now been used.
    node->symbol->flags.used = true;
  }
  
  if (TypeIsStructOrUnion(node->base.type) || TypeIsArray(node->base.type) ||
      TypeIsFunction(node->base.type)) {
    node->base.flags |= kASTNeedAddress;
  } else {
    // If this is a declaration, don't try to fold it.
    if ((node->base.flags & kASTIsDeclaration) != 0) {
      return &node->base;
    }
    if (!node->symbol->flags.value_set) {
      return &node->base;
    }
    // If the identifier is a constant, replace the node with a constant node.
    if (TypeIsIntConstant(node->base.type)) {
      ASTNode* const_node = NewIntConstantASTNode(
          node->symbol->value.ivalue, node->symbol->type, node->base.location);
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node,
                          true);
      const_node->flags |= kASTAnalyzed;
      return const_node;
    } else if (TypeIsFloatingPointConstant(node->base.type)) {
      ASTNode* const_node = NewRealConstantASTNode(
          node->symbol->value.fvalue, node->symbol->type, node->base.location);
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node,
                          true);
      const_node->flags |= kASTAnalyzed;
      return const_node;
    }
  }
  return &node->base;
}

// Attempt to fold a constant expression by evaluating it and if
// successful, replacing it with a constant AST node with the value.
static ASTNode* FoldConstantExpression(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
    case AST_OP(fnumber):
    case AST_OP(string):
    case AST_OP(string_wide):
    case AST_OP(macro):
    case AST_OP(identifier):
      // These are leaf nodes so they are already folded.
      return NULL;
    case AST_OP(expr_init):
      // Never fold this.
      return NULL;
    default:
      break;
  }

  // Try to fold integer and floating point constant expressions.
  if (TypeIsIntegral(node->type)) {
    int64_t value;
    bool ok = EvaluateIntegerExpression((ASTNode*)node, &value);
    if (ok) {
      ASTNode* const_node =
          NewIntConstantASTNode(value, node->type, node->location);
      ASTNodeReplaceChild(node->parent, node->child_id, const_node, true);
      const_node->flags |= kASTAnalyzed;
      return const_node;
    }
  } else if (TypeIsFloatingPoint(node->type)) {
    double value;
    bool ok = EvaluateFloatingPointExpression((ASTNode*)node, &value);
    if (ok) {
      ASTNode* const_node =
          NewRealConstantASTNode(value, node->type, node->location);
      ASTNodeReplaceChild(node->parent, node->child_id, const_node, true);
      const_node->flags |= kASTAnalyzed;
      return const_node;
    }
  }
  return NULL;
}

static bool IsNullPointer(ASTNode* node) {
  switch (node->op) {
    case AST_OP(number): {
      ConstantASTNode* c = (ConstantASTNode*)node;
      return c->value.ivalue == 0;
      }
    case AST_OP(question): {      // Conditional expression:
      node = ((BinaryASTNode*)node)->right;     // Colon.
      ASTNode* left = ((BinaryASTNode*)node)->left;
      ASTNode* right = ((BinaryASTNode*)node)->right;
      return IsNullPointer(left) && IsNullPointer(right);
    }
    case AST_OP(cast): {      // cast
      CastASTNode* c = (CastASTNode*)node;
      return c->expr != NULL && IsNullPointer(c->expr);
    }
    default:
    return false;
  }
}

// General analysis of a binary expression.  Does a recursive analysis
// of the chilren and then makes sure the types are scalar.  No binary
// expressions use structs or unions as operands (well the AST_OP(dot)
// and AST_OP(arrow) ones do, but those don't call this).
// This also sets the type of the node to that of the left child.  This
// will most likely be overwritten by the caller but it's safe to do it.
static void AnalyzeBinaryExpression(BinaryASTNode* node) {
  if (node == NULL) {
    return;
  }
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
}

// Ranks for types.  Larger ranks are closer to the end
// of the array.  These are pointers to functions that return true
// if the type is of the requested value.
bool (*type_ranks[])(TypeRecord*) = {
    TypeIsBool,       TypeIsChar,     TypeIsShort, TypeIsInt,
    TypeIsLong,       TypeIsLongLong, TypeIsFloat, TypeIsDouble,
    TypeIsLongDouble, TypeIsVoid,     NULL,
};

// The 'int' rank, for promotion to int.
#define kIntRank 4

// Given a type, what is its rank.  According to the standard, types with higher
// precision are higher in rank, with _Bool being the lowest rank.  Floating
// point types have the highest rank.
static int GetRank(TypeRecord* type) {
  for (int i = 0; type_ranks[i] != NULL; i++) {
    if (type_ranks[i](type)) {
      return i+1;
    }
  }
  return -1;
}

// Analyze a unary expression by analyzing the sub expression
// and propagating the type up.  Also checks that the expression
// is scalar and promotes types smaller than int to int if needed.
static void AnalyzeUnaryExpression(UnaryASTNode* node) {
  if (node == NULL) {
    return;
  }
  node->sub = AnalyzeExpression(node->sub);
  SemanticCheckScalarType(node->sub);
  switch (node->base.op) {
    case AST_OP(not):
      // Not operator is boolean.
      NormalConversion(node->sub, NewTypeRecordWithSize(kTypeBool, kQualPlain));
      break;
    case AST_OP(uminus): {
      int rank = GetRank(node->sub->type);
      if (rank < kIntRank) {
        Type t = kTypeInt;
        if (TypeIsUnsigned(node->sub->type)) {
          t |= kTypeUnsigned;
        }
        NormalConversion(node->sub, NewTypeRecordWithSize(t, kQualPlain));
      }
      break;
    }
    default:
      break;
      
  }
  ASTNodeSetType((ASTNode*)node, node->sub->type);
}

static bool IsIntConstant(ASTNode* node) {
  return node->op == AST_OP(number) || node->op == AST_OP(charconst);
}

// Check that we have a valid operands for a numeric expression
// and insert conversions as necessary.
static void InsertNumericConversions(BinaryASTNode* node, bool promote_to_int) {
  if (TypeIsStructOrUnion(node->left->type) ||
      TypeIsStructOrUnion(node->right->type)) {
    SemanticTypeConversionError(node->left, node->right->type,
                                "Illegal binary operand types "
                                "'%s' and '%s'");
  } else if (TypeIsPointerOrArray(node->left->type) ||
             TypeIsPointerOrArray(node->right->type)) {
    // One of the types is a pointer or array, check for compatibility.
    if (TypeIsPointerOrArray(node->left->type) && TypeIsPointerOrArray(node->right->type)) {
      // Both pointers
      if (!TypeEqual(node->left->type, node->left->type)) {
        SemanticTypeConversionWarning(node->left, node->right->type,
                                      "pointer-types",
                                    "Illegal pointer types "
                                    "'%s' and '%s'");
      }
      ASTNodeSetType((ASTNode*)node, node->right->type);
    } else {
      // One is a pointer, the other isn't.
      ASTNode* ptr = node->right;
      ASTNode* nonptr = node->left;
      if (TypeIsPointerOrArray(node->left->type)) {
        ASTNode* t = ptr;
        ptr = nonptr;
        nonptr = t;
      }
      if (!IsNullPointer(nonptr)) {
        SemanticTypeConversionWarning(ptr, nonptr->type,
                                      "pointer-types",
                                    "Illegal pointer types "
                                    "'%s' and '%s'");

      }
      ASTNodeSetType((ASTNode*)node, ptr->type);
    }
  } else {
    int left_rank = IsIntConstant(node->left) ? 0 : GetRank(node->left->type);
    int right_rank = IsIntConstant(node->right) ? 0 : GetRank(node->right->type);
    if (promote_to_int) {
      // Promote values smaller than int to int.
      if (left_rank > 0 && left_rank < kIntRank) {
        Type t = kTypeInt;
        if (TypeIsUnsigned(node->left->type)) {
          t |= kTypeUnsigned;
        }
        NormalConversion(node->left, NewTypeRecordWithSize(t, kQualPlain));
        ASTNodeSetType((ASTNode*)node, node->left->type);
      }
      if (right_rank > 0 && right_rank < kIntRank) {
        Type t = kTypeInt;
        if (TypeIsUnsigned(node->right->type)) {
          t |= kTypeUnsigned;
        }
        NormalConversion(node->right, NewTypeRecordWithSize(t, kQualPlain));
        ASTNodeSetType((ASTNode*)node, node->right->type);
      }
    }
    // Convert smaller rank to larger.
    assert(left_rank != -1 && right_rank != -1);
    if (left_rank > right_rank) {
      // Convert right to left.
      NormalConversion(node->right, node->left->type);
      ASTNodeSetType((ASTNode*)node, node->left->type);
    } else if (left_rank < right_rank) {
      // Convert left to right.
      NormalConversion(node->left, node->right->type);
      ASTNodeSetType((ASTNode*)node, node->right->type);
    }
  }
}

// A binary plus operator allows an integer to be added to a pointer (or array).
// The integer is scaled (multiplied) by the size of the thing pointed to.
static void AnalyzePlusOperator(BinaryASTNode* node) {
  if (node == NULL) {
    return;
  }
  AnalyzeBinaryExpression(node);
  if (TypeIsPointerOrArray(node->left->type)) {
    if (TypeIsIntegral(node->right->type)) {
      // Scale right side by size of left.
      // If the right node is a constant we can do the multiplication now.
      if (ASTNodeIsIntConstant(node->right)) {
        int64_t size = node->left->type->next->size;
        int64_t value = ASTNodeConstantValue(node->right);
        ASTNode* scale = NewIntConstantASTNode(value * size, node->right->type,
                                               node->base.location);
        node->right = scale;
      } else {
        ASTNode* scale = NewPtrScaleASTNode(node->left->type->next, AST_OP(mult),
                                            node->right, node->right->location);
        node->right = scale;
        scale->parent = (ASTNode*)node;
        ASTNodeSetType(scale, node->left->type);
      }
    } else {
      SemanticError((ASTNode*)node, "Can only add an integer to a pointer");
    }
  } else if (TypeIsPointerOrArray(node->right->type)) {
    if (TypeIsIntegral(node->left->type)) {
      // Scale left side by size of right.
      if (ASTNodeIsIntConstant(node->left)) {
         int64_t size = node->right->type->next->size;
         int64_t value = ASTNodeConstantValue(node->left);
         ASTNode* scale = NewIntConstantASTNode(value * size, node->left->type,
                                                node->base.location);
         node->left = scale;
       } else {
         ASTNode* scale = NewPtrScaleASTNode(
           node->right->type->next, AST_OP(mult), node->left, node->left->location);
         node->left = scale;
         scale->parent = (ASTNode*)node;
         ASTNodeSetType(scale, node->right->type);
       }
    } else {
      SemanticError((ASTNode*)node, "Can only add an integer to a pointer");
    }
  } else {
    InsertNumericConversions(node, true);
  }
}

// Like binary plus, a binary minus can subtract integers from pointers,
// but not the other way around.  It can also subtract two pointers.
static ASTNode* AnalyzeMinusOperator(BinaryASTNode* node) {
  if (node == NULL) {
    return &node->base;
  }
  AnalyzeBinaryExpression(node);
  if (TypeIsPointerOrArray(node->left->type)) {
    if (TypeIsIntegral(node->right->type)) {
      // Scale right side by size of left.
      // If the right node is a constant we can do the multiplication now.
      if (ASTNodeIsIntConstant(node->right)) {
        int64_t size = node->left->type->next->size;
        int64_t value = ASTNodeConstantValue(node->right);
        ASTNode* scale = NewIntConstantASTNode(value * size, node->left->type,
                                               node->base.location);
        node->right = scale;
      } else {
        ASTNode* scale = NewPtrScaleASTNode(
            node->left->type->next, AST_OP(mult), node->right, node->right->location);
        node->right = scale;
        scale->parent = (ASTNode*)node;
        ASTNodeSetType(scale, node->left->type);
      }
    } else if (TypeIsPointerOrArray(node->right->type)) {
      // Pointer - pointer: divide the result by the size of the type
      // pointed to.
      if (!TypeIsPointerToSameType(node->left->type, node->right->type)) {
        SemanticTypeConversionError(
            node->left, node->right->type,
            "Illegal pointer subtraction; "
            "pointers are not the same type: '%s' and '%s'");
      } else {
        // Scale the pointer difference by the size of the type.
        // Tree goes from this:
        //                minus
        //                 / \
        //               x     y
        //
        // To:
        //             ptr_scale
        //              /
        //           minus
        //            / \
        //           x   y
        //
        
        ASTNode* parent = node->base.parent;
        ASTNode* scale = NewPtrScaleASTNode(node->right->type->next,
                                             AST_OP(div),
                                             ASTNodeMove(&node->base),
                                             node->right->location);
        ASTNodeReplaceChild(parent, node->base.child_id, scale, false);
  
        // The type of the result is unsigned long (size_t).
        ASTNodeSetType(scale,
                       NewTypeRecordWithSize(kTypeLong | kTypeUnsigned, kQualPlain));
        return scale;
      }
    } else {
      SemanticError((ASTNode*)node, "Illegal pointer subtraction operation");
    }
  } else {
    InsertNumericConversions(node, true);
  }
  return &node->base;
}

// Both sides of a shift operator needs to be an integral type.
static void AnalyzeShift(BinaryASTNode* node) {
  AnalyzeBinaryExpression(node);
  if (!TypeIsIntegral(node->left->type) || !TypeIsIntegral(node->right->type)) {
    SemanticError((ASTNode*)node, "Shift operator needs integral types");
  }

  InsertNumericConversions(node, true);

  // Convert node opcode to correct shift type.   An unsigned type uses a
  // logical shift an a signed type uses an arithmetic (sign extension) shift.
  if (node->base.op == AST_OP(rshift)) {
    // Right shift only.
    if (TypeIsUnsigned(node->base.type)) {
      node->base.op = AST_OP(rshiftl);
    } else {
      node->base.op = AST_OP(rshifta);
    }
  }
}

// Bitwise operators need integers.
static void AnalyzeBitwiseOperator(BinaryASTNode* node) {
  AnalyzeBinaryExpression(node);
  if (!TypeIsIntegral(node->left->type) || !TypeIsIntegral(node->right->type)) {
    SemanticError((ASTNode*)node, "Bitwise operator needs integral types");
  } else {
    InsertNumericConversions(node, true);
  }
}

static void AnalyzeComparisonOperator(BinaryASTNode* node) {
  AnalyzeBinaryExpression(node);
  InsertNumericConversions(node, true);

  // Comparison operators produce boolean values.
  ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeBool, kQualPlain));
}

static void AnalyzeConditionalExpression(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  if (!TypeIsScalar(node->left->type)) {
    SemanticError((ASTNode*)node, "Condition for ? operator must be scalar");
    ASTNodeSetType((ASTNode*)node, node->left->type);
    return;
  }
  SemanticConvertType(node->left, NewTypeRecordWithSize(kTypeBool, kQualPlain), kConvertNormal);
  BinaryASTNode* colon = (BinaryASTNode*)node->right;
  colon->left = AnalyzeExpression(colon->left);
  colon->right = AnalyzeExpression(colon->right);
  InsertNumericConversions(colon, false);
  ASTNodeSetType((ASTNode*)node, colon->left->type);

  ASTNodeSetType((ASTNode*)node, colon->base.type);
}

// Does the node have an address?  In other words, can you take its
// address using the & operator?
static bool HasAddress(ASTNode* node) {
  switch (node->op) {
    case AST_OP(identifier):
      // Variables are fine.
      return true;
    case AST_OP(subscript):
      // Array indexes are ok.
      return true;
    case AST_OP(contents):
      // *pointer is good.
      return true;
    case AST_OP(dot):
    case AST_OP(arrow):
      // struct/union member references are ok as long as they are
      // not bitfields
      return !IsBitfieldReference(node);
    case AST_OP(cast):
      return true;
    case AST_OP(compound_literal):
      return true;
    default:
      return false;
  }
}

// Is the node assignable?  That means, is it non-const and
// has an address.
static bool IsAssignable(ASTNode* node, bool is_init) {
  if (is_init) {
    // During initialization, constants and arrays can be assigned to.
    return !TypeIsFunction(node->type);
  }

  // Constants are not assignable.
  if (TypeIsConst(node->type)) {
    return false;
  }

  // Functions and arrays can't be assigned to.
  if (TypeIsFunction(node->type) || TypeIsArray(node->type)) {
    return false;
  }

  // Multiple assignment are ok.
  if (node->op == AST_OP(assign)) {
    return true;
  }

  return IsBitfieldReference(node) || HasAddress(node);
}

static ASTNode* AnalyzeInitialization(ASTNode* node,
                                  IdentifierASTNode* id_node, ASTNode* init) {
  (void)AnalyzeExpression(&id_node->base);
  init = AnalyzeExpression(init);
  switch (init->op) {
    case AST_OP(expr_init):{
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)init;
      NormalConversion(e->expr, id_node->base.type);
      break;
    default:
      break;
    }
  }
  ASTNodeSetType((ASTNode*)init, id_node->base.type);
  ASTNodeSetType(node, id_node->base.type);

  if (!IsAssignable((ASTNode*)id_node, true)) {
    SemanticError((ASTNode*)id_node,
                  "Cannot initialize a variable of this type");
    return init;
  }

  
  bool is_static = StorageIs(id_node->symbol->storage, STO(static)) ||
                   StorageIs(id_node->symbol->storage, STO(extern));

  // If we are initializing a constant that is integral or floating point
  // we can evaluate the expression, and if successful, assign the value
  // to the constant so we can use it as a constant in further expressions.
  if (TypeIsConst(id_node->symbol->type)) {
    if (TypeIsIntegral(init->type)) {
      id_node->symbol->flags.value_set = EvaluateIntegerExpression(init, &id_node->symbol->value.ivalue);
    } else if (TypeIsFloatingPoint(init->type)) {
      id_node->symbol->flags.value_set = EvaluateFloatingPointExpression(init, &id_node->symbol->value.fvalue);
    }
  }
  ASTNode* simplified_init = AnalyzeInitializer(node->type, init, is_static);
  ASTNodeReplaceChild(node, 1, simplified_init, true);

  // If the symbol being initialized is static set a flag to tell the
  // code generator not to generate any code for it.
  if (is_static) {
    node->flags |= kASTStaticInit;
  }
  return simplified_init;
}

static void AnalyzeAssignmentExpression(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  if (!IsAssignable(node->left, false)) {
    SemanticError(node->left, "Cannot assign to this expression");
  }

  // We need the address of this node, not its value.
  node->left->flags |= kASTNeedAddress;

  switch (node->base.op) {
    case AST_OP(assign):
      NormalConversion(node->right, node->left->type);
      ASTNodeSetType((ASTNode*)node, node->left->type);

      break;
    case AST_OP(pluseq):
    case AST_OP(minuseq):
      // Pointers are incremented or decremented by a scaled value.
      if (TypeIsPointer(node->left->type)) {
        if (!TypeIsIntegral(node->right->type)) {
          SemanticError((ASTNode*)node,
                        "Cannot add or subtract non-integers from pointers");
        }
        ASTNode* scale = NewPtrScaleASTNode(node->left->type->next,
                                            AST_OP(mult),
                                            node->right,
                                            node->right->location);
        node->right = scale;
        scale->parent = (ASTNode*)node;
        ASTNodeSetType(scale, node->left->type);
      } else {
        NormalConversion(node->right, node->left->type);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;
    case AST_OP(multeq):
    case AST_OP(diveq):
      NormalConversion(node->right, node->left->type);
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;
    case AST_OP(percenteq):
    case AST_OP(lshifteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
      if (!TypeIsIntegral(node->left->type) ||
          !TypeIsIntegral(node->right->type)) {
        SemanticError((ASTNode*)node, "Integer type expected");
      } else {
        NormalConversion(node->right, node->left->type);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;

    // For right shift we need to use either arithmetic or logical
    // shift depending on the type.
    case AST_OP(rshifteq):
      if (!TypeIsIntegral(node->left->type) ||
          !TypeIsIntegral(node->right->type)) {
        SemanticError((ASTNode*)node, "Integer type expected");
      } else {
        NormalConversion(node->right, node->left->type);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      if (TypeIsUnsigned(node->base.type)) {
        node->base.op = AST_OP(rshifteql);
      } else {
        node->base.op = AST_OP(rshifteqa);
      }
      break;

    default:
      assert(false);
  }
}

// Increment and decrement operators, both pre and post.
static void AnalyzeIncDec(UnaryASTNode* node) {
  AnalyzeUnaryExpression(node);
  if (!IsAssignable(node->sub, false)) {
    SemanticError(node->sub, "Cannot increment or decrement this value");
  }
  // NOTE: the scaling by the size of the pointer is handled by code
  // generation, not here.

  // Here we mark the sub node as needing the address, not value.
  node->sub->flags |= kASTNeedAddress;
}

// Array subscripting operator.
static void AnalyzeArraySubscript(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  SemanticConvertType(node->right,
                      NewTypeRecordWithSize(kTypeInt, kQualPlain), kConvertNormal);
  if (node->right != NULL && !TypeIsIntegral(node->right->type)) {
    SemanticError(node->right, "Subscripts must be integral types");
  }
  if (node->left != NULL && !TypeIsPointerOrArray(node->left->type)) {
    SemanticError(node->left, "Can only subscript arrays and pointers");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }

  // Dereference the array type.
  TypeRecord* subtype = node->left->type->next;
  ASTNodeSetType((ASTNode*)node, subtype);
}

// Inliner data.
typedef struct {
  Map argument_map;     // Map of argument to new symbol pointers.
  ASTNode* end_label;   // End label for return conversion.
  Symbol* return_value; // Return value symbol.
  ASTNode* top_stmt;    // Top level compound statement.
} Inliner;


// This is called while cloning the function body for inlining.  The
// data is a pointer to an Inliner.  The node is a cloned node.
// There are two conversion that need to happen:
// 1. A reference to an identifier that is a formal argument of the function
//    being inlined needs to refer to a new symbol that has been assigned
//    the actual value.
// 2. A return statement needs to be converted to a goto, after first
//    assigning the return value (if any) to the temporary symbol allocated
//    to hold the result.
//
// In addiiton, a cloned switch statement no longer has any case label
// or default information.  We perform a new semantic analysis on that
// after the clone of that node.
//
// Returns either the node passed or a new node (in the case of return
// converted to assignment and goto).
static ASTNode* InlineFunctionBodyStatement(ASTNode* node, void* data) {
  Inliner* inliner = data;
  // For an identifier
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* id_node = (IdentifierASTNode*)node;
    MapKeyType key;
    key.p = id_node->symbol;
    void* new_sym = MapFind(&inliner->argument_map, key);
    if (new_sym != NULL) {
      id_node->symbol = new_sym;
    }
    return node;
  }
  if (node->op == AST_OP(return)) {
    CombinedStatementASTNode* ret_node = (CombinedStatementASTNode*)node;
    Vector* new_ret = NewVector();
    if (inliner->return_value != NULL && ret_node->cond != NULL) {
      ASTNode* value = ASTNodeMove(ret_node->cond);
      ASTNode* ret_value =
          NewIdentifierASTNode(inliner->return_value, ret_node->base.location);
      ASTNode* ret_assign = NewBinaryASTNode(AST_OP(assign),
                                             value->type,
                                             value->location,
                                             ret_value,
                                             value);
      ASTNode* assign_expr = NewExpressionStatementASTNode(ret_assign, ret_assign->location);
      AnalyzeStatement(assign_expr);
      VectorAppend(new_ret, assign_expr);
    }
    // Now make a goto node to the end_label.
    LabelASTNode* end_label = (LabelASTNode*)inliner->end_label;
    GotoStatementASTNode* goto_node = (GotoStatementASTNode*)
                    NewGotoStatementASTNode(
                                  NewString(end_label->name.value),
                                  ret_node->base.location);
    goto_node->label = inliner->end_label;
    goto_node->lca = inliner->top_stmt;
    VectorAppend(new_ret, goto_node);
    
    // Don't need the return now.
    ASTNodeDelete(node);
    
    // Build a new Compound statement containing the assignment to the
    // return value (if necessary) and the goto.
    return NewCompoundStatementASTNode(new_ret, ret_node->base.location);
  }
  
  // After cloning a switch statement we have lost the analysis of
  // case and default labels.  Need to do it again now.
  if (node->op == AST_OP(switch)) {
    AnalyzeStatement(node);
  }
  
  return node;
}

// Build declaration list for all formal args and initialize them with
// the actual args.  Since these are no longer arguments we need
// to copy the Symbols too.  The ownership of all actual expressions is
// changed to the variable declaration.  The inliner's argument_map
// will contains a mapping of old symbol to new symbol.
static ASTNode* CopyArguments(FunctionInfo* info, VectorASTNode* call, Inliner* inliner) {
  SourceLocation location = call->base.location;
  Vector* decls = NewVector();
  for (size_t i = 0; i < info->prototype.length; i++) {
    Symbol* formal = SymbolClone(info->prototype.value.p[i]);
    formal->flags.is_argument = false;
    formal->flags.is_local = true;
    VectorAppend(&compiler->syntax.all_local_symbols, formal);
    
    // Insert old and new into inliner's argument map so we can translate
    // the argument references to the local variables.
    MapKeyValue kv;
    kv.key.p = info->prototype.value.p[i];
    kv.value.p = formal;
    MapInsert(&inliner->argument_map, kv);
    
    ASTNode* actual = ASTNodeMove(call->children->value.p[i]);
    ASTNode* assign = NewBinaryASTNode(AST_OP(assign),
                                       actual->type,
                                       actual->location,
                                       NewIdentifierASTNode(formal, location),
                                       actual);
    assign = AnalyzeExpression(assign);
    VectorAppend(decls,
                 NewVariableDeclarationASTNode(formal,
                                               assign,
                                               location));
  }
  
  // Allocate a temporary for the return value if it's not void.
  if (!TypeIsVoid(call->base.type)) {
    Symbol* temp = SyntaxNewTemporary(&compiler->syntax, call->base.type);
    inliner->return_value = temp;
    VectorAppend(decls, NewVariableDeclarationASTNode(temp, NULL, location));
  } else {
    inliner->return_value = NULL;
  }
  return NewDeclarationListASTNode(decls, location);
}

// Inline a function call.
// 1. Create new symbols for all formal args.
// 2. Assign all actual values to new symbols.
// 3. Allocate (but don't emit) end_label.
// 4. Clone body, replacing all returns by a goto to end_label and references
//    to the arguments with references to the new symbols.
// 5. Emit end_label.
static ASTNode* InlineFunctionCall(FunctionInfo* info, VectorASTNode* call) {
  Vector* statements = NewVector();
  SourceLocation location = call->base.location;
  Inliner inliner;
  MapInitForPointerKeys(&inliner.argument_map);
  
  VectorAppend(statements,
               CopyArguments(info, call, &inliner));
  ASTNode* inlined = NewCompoundStatementASTNode(statements, info->body->location);
  inliner.top_stmt = inlined;
  
  inliner.end_label = NewLabelASTNode(SyntaxFakeName(&compiler->syntax),
                                      NULL,
                                      false,
                                      location);
    
  // Clone function body replacing:
  // 1. Variable references to arguments with new symbols.
  // 2. return statements with goto statements to end_label.
  ASTNode* new_body = ASTNodeClone(info->body,
                                   InlineFunctionBodyStatement,
                                   &inliner, NULL);
  VectorAppend(statements, new_body);
  VectorAppend(statements, inliner.end_label);
  
  // Attach the new statements to the compound statement.
  for (size_t i = 0; i < statements->length; i++) {
    ASTNode* stmt = statements->value.p[i];
    stmt->parent = inlined;
    stmt->child_id = (int)i;
  }
  ASTNode* ret_node = NULL;
  if (inliner.return_value != NULL) {
    // Void function, no return value;
    ret_node = NewIdentifierASTNode(inliner.return_value, location);
  }
  MapDestruct(&inliner.argument_map);
  return NewInlineCallASTNode(call->base.type, location, inlined, ret_node);
}

typedef struct {
  int node_count;
  bool found_goto;
} GotoFinder;

static void ExamineBody(ASTNode* node, void* data, int child_id, VisitorMode mode) {
  GotoFinder* finder = data;
  finder->node_count++;
  if (node->op == AST_OP(goto)) {
    finder->found_goto = true;
  }
}

// We can only inline a function if:
// 1. It is defined and has a body
// 2. It is not the current function.
// 3. It's not a varargs function or has unknown args.
// 4. It has no goto statements.
// 5. The number of AST nodes is reasonably small.
//
// Why the goto prohibition.  Well, the GotoStatementASTNode contains
// a resolved reference to its label.  We clone the body to
// inline it, so this reference is no longer valid after the
// copy.  It's really hard to find the new resolved label without
// traversing the whole function looking for the label.  Goto statements
// are pretty rare anyway so this isn't a big deal really.
//
// NOTE: a tail-recursive inline function will not be inlined because
// the tail recursion is converted into a goto statement.
static bool FunctionCanBeInlined(FunctionInfo* func) {
  if (!OptLevel2()) {
    // Only at -O2 and above.
    return false;
  }
  if (!func->is_inline || !func->symbol->flags.is_defined ||
      func->body == NULL ||
      func->symbol == compiler->current_function->info.function.symbol ||
      func->unknown_args || func->varargs) {
    return false;
  }
  const int kMaxInlineNodeCount = 100;    // Arbitrary.
  GotoFinder finder = {0, false};
  ASTNodeVisit(func->body, ExamineBody, 0, &finder);
  if (finder.found_goto) {
    return false;
  }
  return finder.node_count < kMaxInlineNodeCount;
}

static ASTNode* AnalyzeFunctionCall(VectorASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  size_t num_actual_args = node->children->length;
  for (size_t i = 0; i < num_actual_args; i++) {
    node->children->value.p[i] = AnalyzeExpression((ASTNode*)node->children->value.p[i]);
  }
  if (node->left != NULL && !TypeIsFunctionPointer(node->left->type)) {
    SemanticError(node->left, "Cannot call a non-function");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return &node->base;
  }
  if (node->left == NULL) {
    return &node->base;
  }

  // Set node type by dereferencing the function.  We've already checked that
  // the left type is a function or a pointer to a function.  The 'next' field
  // of the function is the type of this node (the return type of the function).
  TypeRecord* subtype = node->left->type;
  if (TypeIsPointer(node->left->type)) {
    subtype = subtype->next;
  }

  ASTNodeSetType((ASTNode*)node, subtype->next);

  bool call_ok = true;
  // We are calling a function.  Let's check the arguments.
  size_t num_formal_args = subtype->info.function.prototype.length;
  if (!subtype->info.function.unknown_args) {
    if (num_actual_args < num_formal_args) {
      SemanticError((ASTNode*)node,
                    "Too few arguments supplied to varargs function call; need "
                    "%zd, got %zd",
                    num_formal_args, num_actual_args);
      call_ok = false;
    }
    if (!subtype->info.function.varargs &&
        num_actual_args != num_formal_args) {
      SemanticError((ASTNode*)node,
                    "Incorrect number of arguments supplied to function call; "
                    "need %zd, got %zd",
                    num_formal_args, num_actual_args);
      call_ok = false;
    }

    for (size_t i = 0; i < num_formal_args && i < num_actual_args; i++) {
      ASTNode* actual = (ASTNode*)node->children->value.p[i];
      Symbol* formal = (Symbol*)subtype->info.function.prototype.value.p[i];
      NormalConversion(actual, formal->type);

      // Composites (structs/unions), arrays and functions need addresses, not
      // values.
      if (TypeIsStructOrUnion(actual->type) || TypeIsArray(actual->type) ||
          TypeIsFunction(actual->type)) {
        actual->flags |= kASTNeedAddress;
      }
    }
  }
  
  // Inline function call if possible.  Only possible if we are calling
  // a function (not a function pointer) and it was tagged as inline.
  if (call_ok && TypeIsFunction(node->left->type)) {
    FunctionInfo* func = &node->left->type->info.function;
    if (FunctionCanBeInlined(func)) {
      // Clone the function's body and replace the call by
      // an inline_call node.
      ASTNode* inline_call = InlineFunctionCall(func, node);
      ASTNodeReplaceChild(node->base.parent,
                          node->base.child_id, inline_call, true);
      return inline_call;
    }
  }
  return &node->base;
}

static void AnalyzeMemberReference(BinaryASTNode* node) {
  if (node->base.type != NULL) {
    // Already analyzed.
    return;
  }
  Struct* struct_info = NULL;

  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  if (node->base.op == AST_OP(arrow)) {
    // Op is ->, needs to be a pointer to a struct/union.
    if (!TypeIsStructOrUnionPointer(node->left->type)) {
      SemanticError((ASTNode*)node,
                    "Left of -> is not a pointer to a "
                    "struct/union; did you mean to use '.'");
    } else {
      // Dereference the pointer to get the struct info.
      struct_info = node->left->type->next->info.struct_info;
    }
  } else if (!TypeIsStructOrUnion(node->left->type)) {
    if (TypeIsStructOrUnionPointer(node->left->type)) {
      SemanticError((ASTNode*)node,
                    "Left of '.' is a pointer; did you mean to use ->?");
    } else {
      SemanticError((ASTNode*)node, "Left of '.' is not a struct/union");
    }
  } else {
    // Node is AST_OP(dot) and left is a struct/union.
    struct_info = node->left->type->info.struct_info;
  }

  if (struct_info == NULL) {
    // Error case, assign type as integer.
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }

  if (TypeIsStructOrUnion(node->left->type)) {
    // Left is a struct, only need address.
    node->left->flags |= kASTNeedAddress;
  }

  // The right side of the AST_OP(dot) and AST_OP(arrow) node is a string
  // constant containing the member name.
  String* member_name = ((ConstantASTNode*)node->right)->value.string;

  // Look up struct member.
  StructMember* member = FindStructMember(struct_info, member_name);
  if (member == NULL) {
    SemanticError((ASTNode*)node, "%s is not a member of struct/union %s",
                  member_name->value, struct_info->tag_name->value);
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }

  // Replace the right node with a StructMember AST node.
  ASTNode* old_right = node->right;
  node->right = NewStructMemberASTNode(member, node->right->location);
  ASTNodeDelete(old_right);
  ASTNodeSetType((ASTNode*)node, member->symbol->type);
}

// Address-of operator.  If the operand has an address the type is
// a pointer to the type of the operand.
static void AnalyzeAddressOperator(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
  if (!HasAddress(node->sub)) {
    SemanticError(node->sub, "Cannot take the address of this expression");
    // Make a void* pointer type for this node.
    TypeRecord* void_type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
    TypeRecordChain(ptr, void_type);
    ASTNodeSetType((ASTNode*)node, ptr);
    return;
  }
  TypeRecordChain(ptr, node->sub->type);
  ASTNodeSetType((ASTNode*)node, ptr);

  // Tell downstream that we need the address of this node, not its
  // contents.
  node->sub->flags |= kASTNeedAddress;
}

// Contents-of operator.  If the operand is a pointer the result type
// is the type pointed to.
static void AnalyzeContentsOperator(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  if (!TypeIsPointerOrArray(node->sub->type)) {
    SemanticError(node->sub, "Cannot take contents of this expression");
    // Fake an integer type for the result.
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }

  // Fake an integer type for the result.
  // ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  ASTNodeSetType((ASTNode*)node, node->sub->type->next);
}

static void AnalyzeSizeofExpression(SizeofASTNode* node) {
  if (node->expr != NULL) {
    node->expr = AnalyzeExpression(node->expr);
    if (TypeIsVLA(node->expr->type)) {
      // sizeof(vla) is calculated at runtime.
    } else {
      node->base.value.ivalue = node->expr->type->size;
    }
  }
  ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
}

static void AnalyzeCastExpression(CastASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  SemanticConvertType(node->expr, node->cast_type, kConvertCast);

  // Result is the requested type.
  ASTNodeSetType((ASTNode*)node, node->cast_type);
}

static void  AnalyzeCompoundLiteral(CompoundLiteralASTNode* node) {
  node->initializer = AnalyzeInitialization(&node->base,
                                             (IdentifierASTNode*)node->sym,
                        node->initializer);
}

static void AnalyzeLogicalOperator(BinaryASTNode* node) {
  TypeRecord* bool_type = NewTypeRecordWithSize(kTypeBool, kQualPlain);
  
  node->left = AnalyzeExpression(node->left);
  SemanticConvertType(node->left, bool_type, kConvertNormal);
  node->right = AnalyzeExpression(node->right);
  SemanticConvertType(node->right, bool_type, kConvertNormal);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);

  ASTNodeSetType((ASTNode*)node, bool_type);
}

static void SetNeedAddress(ASTNode* node) {
  node->flags |= kASTNeedAddress;
  if (node->op == AST_OP(identifier)) {
    IdentifierASTNode* idnode = (IdentifierASTNode*)node;
    idnode->symbol->flags.address_taken = true;
  }
}

static void AnalyzeVarargsBuiltin1(VectorASTNode* args) {
  for (size_t i = 0; i < args->children->length; i++) {
    ASTNode* child = args->children->value.p[i];
    args->children->value.p[i] = AnalyzeExpression(child);
    child = args->children->value.p[i];
    SetNeedAddress(child);     // Need address of all of these.
  }
  ASTNodeSetType(&args->base, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

static void AnalyzeVarargsBuiltin2(VectorASTNode* args) {
  // Second arg is a constant whose type is set to the type of the arg.
  ASTNode* ap = args->children->value.p[0];
  SetNeedAddress(ap);  // Need address of ap arg.
  ASTNode* type_node = args->children->value.p[1];
  ASTNodeSetType(&args->base, type_node->type);  // Type is type of second arg.
}

// Perform semantic analysis on a expression AST node.  This propagates type
// information from the node's children to the node and also performs checks to
// make sure the types follow the rules of the language.
ASTNode* AnalyzeExpression(ASTNode* node) {
  if (node == NULL || (node->flags & kASTAnalyzed) != 0) {
    return node;
  }

  BinaryASTNode* binary_node = (BinaryASTNode*)node;
  UnaryASTNode* unary_node = (UnaryASTNode*)node;
  VectorASTNode* vector_node = (VectorASTNode*)node;

  switch (node->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
    case AST_OP(fnumber):
    case AST_OP(string):
    case AST_OP(string_wide):
    case AST_OP(macro):
      // These leaf nodes already have a type.
      break;

    case AST_OP(identifier):
      node = AnalyzeIdentifier((IdentifierASTNode*)node);
      break;

    case AST_OP(plus):
      AnalyzePlusOperator(binary_node);
      break;

    case AST_OP(minus):
      node = AnalyzeMinusOperator(binary_node);
      break;

    case AST_OP(mult):
    case AST_OP(div):
      AnalyzeBinaryExpression(binary_node);
      InsertNumericConversions(binary_node, true);
      break;

    case AST_OP(mod):
      AnalyzeBinaryExpression(binary_node);
      InsertNumericConversions(binary_node, true);
      if (!TypeIsIntegral(binary_node->left->type)) {
        SemanticError(node, "Modulus operator needs an integral type");
      }
      break;

    case AST_OP(lshift):
    case AST_OP(rshift):
      AnalyzeShift(binary_node);
      break;

    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
      AnalyzeBitwiseOperator(binary_node);
      break;

    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
      AnalyzeComparisonOperator(binary_node);
      break;

    case AST_OP(question):
      AnalyzeConditionalExpression(binary_node);
      break;

    case AST_OP(assign):
    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteq):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
      AnalyzeAssignmentExpression(binary_node);
      break;

    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(postinc):
    case AST_OP(postdec):
      AnalyzeIncDec(unary_node);
      break;

    case AST_OP(uplus):
    case AST_OP(uminus):
      AnalyzeUnaryExpression(unary_node);
      break;

    case AST_OP(sizeof):
      AnalyzeSizeofExpression((SizeofASTNode*)node);
      break;

    case AST_OP(cast):
      AnalyzeCastExpression((CastASTNode*)node);
      break;

    case AST_OP(compound_literal):
      AnalyzeCompoundLiteral((CompoundLiteralASTNode*)node);
      break;
      
    case AST_OP(not):
      AnalyzeUnaryExpression(unary_node);
      break;

    case AST_OP(onescomp):
      AnalyzeUnaryExpression(unary_node);
      break;

    case AST_OP(address):
      AnalyzeAddressOperator(unary_node);
      break;

    case AST_OP(contents):
      AnalyzeContentsOperator(unary_node);
      break;

    case AST_OP(subscript):  // Array subscript.
      AnalyzeArraySubscript(binary_node);
      break;

    case AST_OP(call):  // Function call.
      node = AnalyzeFunctionCall(vector_node);
      break;

    case AST_OP(dot):
    case AST_OP(arrow):
      AnalyzeMemberReference(binary_node);
      break;

    case AST_OP(comma):
      binary_node->left = AnalyzeExpression(binary_node->left);
      binary_node->right = AnalyzeExpression(binary_node->right);

      // Type of comma operator is type of right operand.
      ASTNodeSetType(node, binary_node->right->type);
      break;

    case AST_OP(logand):
    case AST_OP(logor):
      AnalyzeLogicalOperator(binary_node);
      break;

    case AST_OP(init):
      binary_node->right = AnalyzeInitialization(node,
                                                 (IdentifierASTNode*)binary_node->left,
                            binary_node->right);
      break;

    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* expr_init = (ExpressionInitializerASTNode*)node;
      expr_init->expr = AnalyzeExpression(expr_init->expr);
      ASTNodeSetType(node, expr_init->expr->type);
      break;
    }
      
    case AST_OP(braced_init):
    case AST_OP(designated_init):
      // Prevent folding of these expressions since we don't know its type
      // until the AST_OP(init) is analyzed.
      return node;

    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
      AnalyzeVarargsBuiltin1(vector_node);
      break;
    
    case AST_OP(builtin_va_arg):
      AnalyzeVarargsBuiltin2(vector_node);
      break;

    default:
      break;
  }

  // Attempt to fold a constant expression.
  ASTNode* folded = FoldConstantExpression(node);
  if (folded != NULL) {
    return folded;
  }

  // Set flag to prevent double analysis.
  node->flags |= kASTAnalyzed;
  
  if (node->type == NULL) {
    // Make sure we have type for the node.
    ASTNodeSetType(node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  }
  return node;
}

bool IsConstantExpression(ASTNode* node) {
  node = AnalyzeExpression(node);
  switch (node->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
    case AST_OP(fnumber):
    case AST_OP(string):
    case AST_OP(string_wide):
      return true;
    case AST_OP(identifier): {
      // Static identifiers that are arrays are constant.
      IdentifierASTNode* id_node = (IdentifierASTNode*)node;
      if (TypeIsIntConstant(id_node->base.type) ||
          TypeIsFloatingPointConstant(id_node->base.type)) {
        return true;
      }
      // Functions are constant expressions.
      if (TypeIsFunction(id_node->base.type)) {
        return true;
      }
      if (!TypeIsArray(id_node->base.type)) {
        return false;
      }
      if (StorageIs(id_node->symbol->storage, STO(static)|STO(extern))) {
        return true;
      }
      return false;
    }
    case AST_OP(address): {
      // Address of a static variable is a constant.
      UnaryASTNode* addr = (UnaryASTNode*)node;
      if (addr->sub->op == AST_OP(identifier)) {
        IdentifierASTNode* id_node = (IdentifierASTNode*)addr->sub;
        if (StorageIs(id_node->symbol->storage, STO(static)|STO(extern))) {
          return true;
        }
      }
      return false;
    }
    case AST_OP(cast): {
      CastASTNode* c = (CastASTNode*)node;
      return IsConstantExpression(c->expr);
    }
  
      case AST_OP(compound_literal): {
        CompoundLiteralASTNode* lit = (CompoundLiteralASTNode*)node;
        return IsConstantExpression(lit->initializer);
      }

    case AST_OP(braced_init): {
      BracedInitializerASTNode* b = (BracedInitializerASTNode*)node;
      for (size_t i = 0; i < b->initializers->length; i++) {
        if (!IsConstantExpression(b->initializers->value.p[i])) {
          return false;
        }
      }
      return true;
    }
    
    case AST_OP(designated_init): {
      DesignatedInitializerASTNode* d = (DesignatedInitializerASTNode*)node;
      return IsConstantExpression(d->init);
      break;
    }
    default:
      return false;
  }
}
