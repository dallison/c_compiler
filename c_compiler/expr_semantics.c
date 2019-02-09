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

static void AnalyzeIdentifier(IdentifierASTNode* node) {
  if (TypeIsStructOrUnion(node->base.type) || TypeIsArray(node->base.type) ||
      TypeIsFunction(node->base.type)) {
    node->base.flags |= kASTNeedAddress;
  } else {
    // If this is a declaration, don't try to fold it.
    if ((node->base.flags & kASTIsDeclaration) != 0) {
      return;
    }
    // If the identifier is a constant, replace the node with a constant node.
    if (TypeIsIntConstant(node->base.type)) {
      ASTNode* const_node = NewIntConstantASTNode(
          node->symbol->value.ivalue, node->symbol->type, node->base.location);
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node,
                          true);
    } else if (TypeIsFloatingPointConstant(node->base.type)) {
      ASTNode* const_node = NewRealConstantASTNode(
          node->symbol->value.fvalue, node->symbol->type, node->base.location);
      ASTNodeReplaceChild(node->base.parent, node->base.child_id, const_node,
                          true);
    }
  }
  // Symbol has now been used.
  node->symbol->used = true;
}

// Attempt to fold a constant expression by evaluating it and if
// successful, replacing it with a constant AST node with the value.
static bool FoldConstantExpression(ASTNode* node) {
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
      return false;
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
      return true;
    }
  } else if (TypeIsFloatingPoint(node->type)) {
    double value;
    bool ok = EvaluateFloatingPointExpression((ASTNode*)node, &value);
    if (ok) {
      ASTNode* const_node =
          NewRealConstantASTNode(value, node->type, node->location);
      ASTNodeReplaceChild(node->parent, node->child_id, const_node, true);
      return true;
    }
  }
  return false;
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
  AnalyzeExpression(node->left);
  AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
}

// Analyze a unary expression by analyzing the sub expression
// and propagating the type up.  Also checks that the expression
// is scalar.
static void AnalyzeUnaryExpression(UnaryASTNode* node) {
  if (node == NULL) {
    return;
  }
  AnalyzeExpression(node->sub);
  ASTNodeSetType((ASTNode*)node, node->sub->type);
  SemanticCheckScalarType(node->sub);
}

// Ranks for types.  Larger ranks are closer to the end
// of the array.  These are pointers to functions that return true
// if the type is of the requested value.
bool (*type_ranks[])(TypeRecord*) = {
    TypeIsBool,       TypeIsChar,     TypeIsShort, TypeIsInt,
    TypeIsLong,       TypeIsLongLong, TypeIsFloat, TypeIsDouble,
    TypeIsLongDouble, TypeIsVoid,     NULL,
};

// Given a type, what is its rank.  According to the standard, types with higher
// precision are higher in rank, with _Bool being the lowest rank.  Floating
// point types have the highest rank.
static int GetRank(TypeRecord* type) {
  for (int i = 0; type_ranks[i] != NULL; i++) {
    if (type_ranks[i](type)) {
      return i;
    }
  }
  return -1;
}

// Check that we have a valid operands for a numeric expression
// and insert conversions as necessary.
static void InsertNumericConversions(BinaryASTNode* node) {
  if (TypeIsStructOrUnion(node->left->type) ||
      TypeIsStructOrUnion(node->right->type)) {
    SemanticTypeConversionError(node->left, node->right->type,
                                "Illegal binary operand types "
                                "'%s' and '%s'");
  } else if (TypeIsPointerOrArray(node->left->type) ||
             TypeIsPointerOrArray(node->right->type)) {
    // Both types are pointers or arrays, check for compatibility.
    // TODO:
  } else {
    // Convert smaller rank to larger.
    int left_rank = GetRank(node->left->type);
    int right_rank = GetRank(node->right->type);
    assert(left_rank != -1 && right_rank != -1);
    if (left_rank > right_rank) {
      // Convert right to left.
      SemanticConvertType(node->right, node->left->type);
      ASTNodeSetType((ASTNode*)node, node->left->type);
    } else if (left_rank < right_rank) {
      // Convert left to right.
      SemanticConvertType(node->left, node->right->type);
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
      // Multiply right side by size of left.
      int64_t size = node->left->type->next->size;
      // If the right node is a constant we can do the multiplication now.
      if (ASTNodeIsIntConstant(node->right)) {
        int64_t value = ASTNodeConstantValue(node->right);
        ASTNode* scale = NewIntConstantASTNode(value * size, node->left->type,
                                               node->base.location);
        node->right = scale;
      } else {
        ASTNode* scale = NewBinaryASTNode(
            AST_OP(mult), node->right->type, node->right->location, node->right,
            NewIntConstantASTNode(size, NewTypeRecord(kTypeInt, kQualPlain),
                                  node->right->location));
        node->right = scale;
      }
    } else {
      SemanticError((ASTNode*)node, "Can only add an integer to a pointer");
    }
  } else if (TypeIsPointer(node->right->type)) {
    if (TypeIsIntegral(node->left->type)) {
      // Multiply left side by size of right.
      int64_t size = node->right->type->next->size;
      ASTNode* scale = NewBinaryASTNode(
          AST_OP(mult), node->left->type, node->left->location, node->left,
          NewIntConstantASTNode(size, NewTypeRecord(kTypeInt, kQualPlain),
                                node->left->location));
      node->left = scale;
    } else {
      SemanticError((ASTNode*)node, "Can only add an integer to a pointer");
    }
  } else {
    InsertNumericConversions(node);
  }
}

// Like binary plus, a binary minus can subtract integers from pointers,
// but not the other way around.  It can also subtract two pointers.
static void AnalyzeMinusOperator(BinaryASTNode* node) {
  if (node == NULL) {
    return;
  }
  AnalyzeBinaryExpression(node);
  if (TypeIsPointerOrArray(node->left->type)) {
    if (TypeIsIntegral(node->right->type)) {
      // Multiply right side by size of left.
      int64_t size = node->left->type->next->size;
      // If the right node is a constant we can do the multiplication now.
      if (ASTNodeIsIntConstant(node->right)) {
        int64_t value = ASTNodeConstantValue(node->right);
        ASTNode* scale = NewIntConstantASTNode(value * size, node->left->type,
                                               node->base.location);
        node->right = scale;
      } else {
        ASTNode* scale = NewBinaryASTNode(
            AST_OP(mult), node->right->type, node->right->location, node->right,
            NewIntConstantASTNode(size, NewTypeRecord(kTypeInt, kQualPlain),
                                  node->right->location));
        node->right = scale;
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
        // We need to convert the node to a AST_OP(div) op with the left as
        // the current node and the right as the size of the type pointed to. To
        // do this, make a new node and with left and right being the current
        // left and right. Then set this node's op to AST_OP(div) and set the
        // left to the new node.
        ASTNode* new_minus =
            NewBinaryASTNode(AST_OP(minus), node->base.type,
                             node->base.location, node->left, node->right);
        int64_t size = node->left->type->next->size;
        ASTNode* size_node = NewIntConstantASTNode(
            size, NewTypeRecord(kTypeInt, kQualPlain), node->base.location);
        node->base.op = AST_OP(div);
        node->left = new_minus;
        node->right = size_node;

        // The type of the result is unsigned long (size_t).
        ASTNodeSetType(&node->base,
                       NewTypeRecord(kTypeLong | kTypeUnsigned, kQualPlain));
      }
    } else {
      SemanticError((ASTNode*)node, "Illegal pointer subtraction operation");
    }
  } else {
    InsertNumericConversions(node);
  }
}

// Both sides of a shift operator needs to be an integral type.
static void AnalyzeShift(BinaryASTNode* node) {
  AnalyzeBinaryExpression(node);
  if (!TypeIsIntegral(node->left->type) || !TypeIsIntegral(node->right->type)) {
    SemanticError((ASTNode*)node, "Shift operator needs integral types");
  }

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
    InsertNumericConversions(node);
  }
}

static void AnalyzeComparisonOperator(BinaryASTNode* node) {
  AnalyzeBinaryExpression(node);
  InsertNumericConversions(node);

  // Comparison operators produce boolean values.
  ASTNodeSetType((ASTNode*)node, NewTypeRecord(kTypeBool, kQualPlain));
}

static void AnalyzeConditionalExpression(BinaryASTNode* node) {
  AnalyzeExpression(node->left);
  if (!TypeIsScalar(node->left->type)) {
    SemanticError((ASTNode*)node, "Condition for ? operator must be scalar");
    ASTNodeSetType((ASTNode*)node, node->left->type);
    return;
  }
  BinaryASTNode* colon = (BinaryASTNode*)node->right;
  AnalyzeExpression(colon->left);
  AnalyzeExpression(colon->right);
  InsertNumericConversions(colon);
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

static void AnalyzeInitialization(ASTNode* node,
                                  IdentifierASTNode* id_node, ASTNode* init) {
  AnalyzeExpression(&id_node->base);
  AnalyzeExpression(init);
  ASTNodeSetType(node, id_node->base.type);
  ASTNodeSetType((ASTNode*)init, id_node->base.type);

  if (!IsAssignable((ASTNode*)id_node, true)) {
    SemanticError((ASTNode*)id_node,
                  "Cannot initialize a variable of this type");
    return;
  }

  bool is_static = id_node->symbol->storage == kStorageStatic ||
                   id_node->symbol->storage == kStorageExtern;

  // If we are initializing a constant that is integral or floating point
  // we can evaluate the expression, and if successful, assign the value
  // to the constant so we can use it as a constant in further expressions.
  if (TypeIsConst(id_node->symbol->type)) {
    if (TypeIsIntegral(init->type)) {
      int64_t value;
      bool ok = EvaluateIntegerExpression(init, &value);
      if (ok) {
        id_node->symbol->value.ivalue = value;
      }
    } else if (TypeIsFloatingPoint(init->type)) {
      double value;
      bool ok = EvaluateFloatingPointExpression(init, &value);
      if (ok) {
        id_node->symbol->value.fvalue = value;
      }
    }
  }

  ASTNode* simplified_init = AnalyzeInitializer(node->type, init);
  ASTNodeReplaceChild(node, 1, simplified_init, true);

  // If the symbol being initialized is static set a flag to tell the
  // code generator not to generate any code for it.
  if (is_static) {
    // TODO: check that all initializers are constant or a reference to existing
    // static variable.
    node->flags |= kASTStaticInit;
  }
}

static void AnalyzeAssignmentExpression(BinaryASTNode* node) {
  AnalyzeExpression(node->left);
  AnalyzeExpression(node->right);
  if (!IsAssignable(node->left, false)) {
    SemanticError(node->left, "Cannot assign to this expression");
  }

  // We need the address of this node, not its value.
  node->left->flags |= kASTNeedAddress;

  switch (node->base.op) {
    case AST_OP(assign):
      SemanticConvertType(node->right, node->left->type);
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
        int64_t size = node->left->type->next->size;
        ASTNode* scale = NewBinaryASTNode(
            AST_OP(mult), node->right->type, node->right->location, node->right,
            NewIntConstantASTNode(size, NewTypeRecord(kTypeInt, kQualPlain),
                                  node->right->location));
        node->right = scale;
      } else {
        SemanticConvertType(node->right, node->left->type);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;
    case AST_OP(multeq):
    case AST_OP(diveq):
      SemanticConvertType(node->right, node->left->type);
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
        SemanticConvertType(node->right, node->left->type);
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
        SemanticConvertType(node->right, node->left->type);
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
  AnalyzeExpression(node->left);
  AnalyzeExpression(node->right);
  if (node->right != NULL && !TypeIsIntegral(node->right->type)) {
    SemanticError(node->right, "Subscripts must be integral types");
  }
  if (node->left != NULL && !TypeIsPointerOrArray(node->left->type)) {
    SemanticError(node->left, "Can only subscript arrays and pointers");
    ASTNodeSetType((ASTNode*)node, NewTypeRecord(kTypeInt, kQualPlain));
    return;
  }

  // Dereference the array type.
  TypeRecord* subtype = node->left->type->next;
  ASTNodeSetType((ASTNode*)node, subtype);
}

static void AnalyzeFunctionCall(VectorASTNode* node) {
  AnalyzeExpression(node->left);
  size_t num_actual_args = node->children->length;
  for (size_t i = 0; i < num_actual_args; i++) {
    AnalyzeExpression((ASTNode*)node->children->value[i]);
  }
  if (node->left != NULL && !TypeIsFunctionPointer(node->left->type)) {
    SemanticError(node->left, "Cannot call a non-function");
    ASTNodeSetType((ASTNode*)node, NewTypeRecord(kTypeInt, kQualPlain));
    return;
  }
  if (node->left == NULL) {
    return;
  }

  // Set node type by dereferencing the function.  We've already checked that
  // the left type is a function or a pointer to a function.  The 'next' field
  // of the function is the type of this node (the return type of the function).
  TypeRecord* subtype = node->left->type;
  if (TypeIsPointer(node->left->type)) {
    subtype = subtype->next;
  }

  ASTNodeSetType((ASTNode*)node, subtype->next);

  // We are calling a function.  Let's check the arguments.
  size_t num_formal_args = subtype->info.function.prototype.length;
  if (!node->left->type->info.function.unknown_args) {
    if (num_actual_args < num_formal_args) {
      SemanticError((ASTNode*)node,
                    "Too few arguments supplied to varargs function call; need "
                    "%zd, got %zd",
                    num_formal_args, num_actual_args);
    }
    if (!node->left->type->info.function.varargs &&
        num_actual_args != num_formal_args) {
      SemanticError((ASTNode*)node,
                    "Incorrect number of arguments supplied to function call; "
                    "need %zd, got %zd",
                    num_formal_args, num_actual_args);
    }

    for (size_t i = 0; i < num_formal_args && i < num_actual_args; i++) {
      ASTNode* actual = (ASTNode*)node->children->value[i];
      Symbol* formal = (Symbol*)subtype->info.function.prototype.value[i];
      SemanticConvertType(actual, formal->type);

      // Composites (structs/unions), arrays and functions need addresses, not
      // values.
      if (TypeIsStructOrUnion(actual->type) || TypeIsArray(actual->type) ||
          TypeIsFunction(actual->type)) {
        actual->flags |= kASTNeedAddress;
      }
    }
  }
}

static void AnalyzeMemberReference(BinaryASTNode* node) {
  Struct* struct_info = NULL;

  AnalyzeExpression(node->left);
  AnalyzeExpression(node->right);
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
    ASTNodeSetType((ASTNode*)node, NewTypeRecord(kTypeInt, kQualPlain));
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
    ASTNodeSetType((ASTNode*)node, NewTypeRecord(kTypeInt, kQualPlain));
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
  AnalyzeExpression(node->sub);
  TypeRecord* ptr = NewPointerTypeRecord(kQualPlain);
  if (!HasAddress(node->sub)) {
    SemanticError(node->sub, "Cannot take the address of this expression");
    // Make a void* pointer type for this node.
    TypeRecord* void_type = NewTypeRecord(kTypeVoid, kQualPlain);
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
  AnalyzeExpression(node->sub);
  if (!TypeIsPointerOrArray(node->sub->type)) {
    SemanticError(node->sub, "Cannot take contents of this expression");
    // Fake an integer type for the result.
    ASTNodeSetType((ASTNode*)node, NewTypeRecord(kTypeInt, kQualPlain));
    return;
  }

  // Fake an integer type for the result.
  ASTNodeSetType((ASTNode*)node, NewTypeRecord(kTypeInt, kQualPlain));
  ASTNodeSetType((ASTNode*)node, node->sub->type->next);
}

static void AnalyzeSizeofExpression(SizeofASTNode* node) {
  if (node->expr != NULL) {
    AnalyzeExpression(node->expr);
    node->base.value.ivalue = node->expr->type->size;
  }
  ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
}

static void AnalyzeCastExpression(CastASTNode* node) {
  AnalyzeExpression(node->expr);

  if (TypeIsVoid(node->cast_type)) {
    // Casting to void is always allowed.
  } else {
    // Convert the expression to the given type.
    // This is different from a normal conversion in that there are
    // very few illegal casts.
    // Illegal casts:
    // 1. struct/union to/from anything
    // 2. void to anything but void
    bool bad_cast = false;
    if (TypeIsStructOrUnion(node->expr->type) ||
        TypeIsStructOrUnion(node->cast_type)) {
      bad_cast = true;
    } else if (TypeIsVoid(node->expr->type)) {
      bad_cast = true;
    }
    if (bad_cast) {
      SemanticTypeConversionError(&node->base, node->cast_type, "Illegal cast");
    }
  }

  // Result is the requested type.
  ASTNodeSetType((ASTNode*)node, node->cast_type);
}

static void AnalyzeLogicalOperator(BinaryASTNode* node) {
  AnalyzeBinaryExpression(node);
  TypeRecord* bool_type = NewTypeRecord(kTypeBool, kQualPlain);
  ASTNodeSetType((ASTNode*)node, bool_type);
}

static void AnalyzeVarargsBuiltin1(VectorASTNode* args) {
  for (size_t i = 0; i < args->children->length; i++) {
    ASTNode* child = args->children->value[i];
    AnalyzeExpression(child);
    child->flags |= kASTNeedAddress;  // Need address of all of these.
  }
  ASTNodeSetType(&args->base, NewTypeRecord(kTypeVoid, kQualPlain));
}

static void AnalyzeVarargsBuiltin2(VectorASTNode* args) {
  // Second arg is a constant whose type is set to the type of the arg.
  ASTNode* ap = args->children->value[0];
  ap->flags |= kASTNeedAddress;  // Need address of ap arg.
  ASTNode* type_node = args->children->value[1];
  ASTNodeSetType(&args->base, type_node->type);  // Type is type of second arg.
}

// Perform semantic analysis on a expression AST node.  This propagates type
// information from the node's children to the node and also performs checks to
// make sure the types follow the rules of the language.
void AnalyzeExpression(ASTNode* node) {
  if (node == NULL) {
    return;
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
      AnalyzeIdentifier((IdentifierASTNode*)node);
      break;

    case AST_OP(plus):
      AnalyzePlusOperator(binary_node);
      break;

    case AST_OP(minus):
      AnalyzeMinusOperator(binary_node);
      break;

    case AST_OP(mult):
    case AST_OP(div):
      AnalyzeBinaryExpression(binary_node);
      InsertNumericConversions(binary_node);
      break;

    case AST_OP(mod):
      AnalyzeBinaryExpression(binary_node);
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
      AnalyzeFunctionCall(vector_node);
      break;

    case AST_OP(dot):
    case AST_OP(arrow):
      AnalyzeMemberReference(binary_node);
      break;

    case AST_OP(comma):
      AnalyzeExpression(binary_node->left);
      AnalyzeExpression(binary_node->right);

      // Type of comma operator is type of right operand.
      ASTNodeSetType(node, binary_node->right->type);
      break;

    case AST_OP(logand):
    case AST_OP(logor):
      AnalyzeLogicalOperator(binary_node);
      break;

    case AST_OP(init):
      AnalyzeInitialization(node, (IdentifierASTNode*)binary_node->left,
                            binary_node->right);
      break;

    case AST_OP(expr_init):
    case AST_OP(braced_init):
    case AST_OP(designated_init):
      // Prevent folding of these expressions since we don't know its type
      // until the AST_OP(init) is analyzed.
      return;

    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
      AnalyzeVarargsBuiltin1(vector_node);
      break;
    
    case AST_OP(builtin_va_arg):
      AnalyzeVarargsBuiltin2(vector_node);
      break;

    default:
      assert(false);
      break;
  }

  // Attempt to fold a constant expression.
  bool folded = FoldConstantExpression(node);
  if (folded) {
    return;
  }

  if (node->type == NULL) {
    // Make sure we have type for the node.
    ASTNodeSetType(node, NewTypeRecord(kTypeInt, kQualPlain));
  }
}
