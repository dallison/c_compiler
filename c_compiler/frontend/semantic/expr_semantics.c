//
//  expr_semantics.c
//  c_compiler
//
//  Created by David Allison on 11/7/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#include "expr_semantics.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "statement_semantics.h"
#include "compiler.h"
#include "symbol_table.h"

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

static bool ASTNodeIsLValue(ASTNode* node) {
  return node != NULL && node->value_category == kValueCategoryLvalue;
}

static bool ASTNodeIsXValue(ASTNode* node) {
  return node != NULL && node->value_category == kValueCategoryXvalue;
}

static bool ASTNodeIsGLValue(ASTNode* node) {
  return ASTNodeIsLValue(node) || ASTNodeIsXValue(node);
}

static bool ReferenceCanBind(ASTNode* actual, TypeRecord* reference_type);

static ASTNode* AnalyzeIdentifier(IdentifierASTNode* node) {
  if (node->base.parent == NULL || node->base.parent->op != AST_OP(init)) {
    // Symbol has now been used.
    node->symbol->flags.used = true;

    // Warn about uses of a symbol marked __attribute__((deprecated)).
    if ((node->base.flags & kASTIsDeclaration) == 0) {
      Attribute* dep = SymbolFindAttribute(node->symbol, "deprecated");
      if (dep != NULL) {
        const char* msg = AttributeArgString(dep, 0);
        if (msg != NULL) {
          // The argument token retains its surrounding double quotes; trim them
          // for a cleaner message.
          char trimmed[256];
          size_t len = strlen(msg);
          if (len >= 2 && msg[0] == '"' && msg[len - 1] == '"') {
            size_t inner = len - 2;
            if (inner >= sizeof(trimmed)) {
              inner = sizeof(trimmed) - 1;
            }
            memcpy(trimmed, msg + 1, inner);
            trimmed[inner] = '\0';
            msg = trimmed;
          }
          SemanticWarning(&node->base, "deprecated-declarations",
                          "'%s' is deprecated: %s", node->symbol->name.value,
                          msg);
        } else {
          SemanticWarning(&node->base, "deprecated-declarations",
                          "'%s' is deprecated", node->symbol->name.value);
        }
      }
    }
  }
  
  if ((node->base.flags & kASTIsDeclaration) == 0) {
    node->base.value_category = kValueCategoryLvalue;
    if (TypeIsReference(node->symbol->type)) {
      ASTNodeSetType(&node->base, node->symbol->type->next);
    } else {
      ASTNodeSetType(&node->base, node->symbol->type);
    }
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
    int left_rank = GetRank(node->left->type);
    int right_rank = GetRank(node->right->type);
    if (promote_to_int) {
      // Integer promotions: operands of rank lower than int are promoted to
      // int.  This must happen on the actual operand types (including integer
      // constants such as `(short)1`) before any of the constant-adaption
      // below, otherwise small constants would skip promotion.
      if (left_rank > 0 && left_rank < kIntRank) {
        Type t = kTypeInt;
        if (TypeIsUnsigned(node->left->type)) {
          t |= kTypeUnsigned;
        }
        NormalConversion(node->left, NewTypeRecordWithSize(t, kQualPlain));
        ASTNodeSetType((ASTNode*)node, node->left->type);
        left_rank = GetRank(node->left->type);
      }
      if (right_rank > 0 && right_rank < kIntRank) {
        Type t = kTypeInt;
        if (TypeIsUnsigned(node->right->type)) {
          t |= kTypeUnsigned;
        }
        NormalConversion(node->right, NewTypeRecordWithSize(t, kQualPlain));
        ASTNodeSetType((ASTNode*)node, node->right->type);
        right_rank = GetRank(node->right->type);
      }
    }
    // After integer promotion, an integer constant no wider than int adapts to
    // the other operand's type (so e.g. `someUnsignedLong + 1` keeps its type)
    // by being treated as the lowest rank.  A constant with an explicit
    // long/long long type keeps its rank so the usual arithmetic conversions
    // widen the result correctly (e.g. `i + 2L` becomes long).
    if (IsIntConstant(node->left) && left_rank <= kIntRank &&
        !(TypeIsUnsigned(node->left->type) &&
          !TypeIsUnsigned(node->right->type) &&
          left_rank == right_rank)) {
      left_rank = 0;
    }
    if (IsIntConstant(node->right) && right_rank <= kIntRank &&
        !(TypeIsUnsigned(node->right->type) &&
          !TypeIsUnsigned(node->left->type) &&
          left_rank == right_rank)) {
      right_rank = 0;
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
    } else if (TypeIsUnsigned(node->left->type) !=
               TypeIsUnsigned(node->right->type)) {
      if (TypeIsUnsigned(node->left->type)) {
        NormalConversion(node->right, node->left->type);
        ASTNodeSetType((ASTNode*)node, node->left->type);
      } else {
        NormalConversion(node->left, node->right->type);
        ASTNodeSetType((ASTNode*)node, node->right->type);
      }
    }
  }
}

static const char* BinaryOperatorFunctionName(ASTOpcode op) {
  switch (op) {
    case AST_OP(plus):
      return "operator+";
    case AST_OP(minus):
      return "operator-";
    case AST_OP(mult):
      return "operator*";
    case AST_OP(div):
      return "operator/";
    case AST_OP(mod):
      return "operator%";
    case AST_OP(lshift):
      return "operator<<";
    case AST_OP(rshift):
    case AST_OP(rshiftl):
    case AST_OP(rshifta):
      return "operator>>";
    case AST_OP(and):
      return "operator&";
    case AST_OP(bitor):
      return "operator|";
    case AST_OP(exor):
      return "operator^";
    case AST_OP(logand):
      return "operator&&";
    case AST_OP(logor):
      return "operator||";
    case AST_OP(assign):
      return "operator=";
    case AST_OP(equal):
      return "operator==";
    case AST_OP(noteq):
      return "operator!=";
    case AST_OP(less):
      return "operator<";
    case AST_OP(lesseq):
      return "operator<=";
    case AST_OP(greater):
      return "operator>";
    case AST_OP(greatereq):
      return "operator>=";
    default:
      return NULL;
  }
}

static const char* UnaryOperatorFunctionName(ASTOpcode op) {
  switch (op) {
    case AST_OP(uplus):
      return "operator+";
    case AST_OP(uminus):
      return "operator-";
    case AST_OP(not):
      return "operator!";
    case AST_OP(onescomp):
      return "operator~";
    default:
      return NULL;
  }
}

static ASTNode* ReplaceBinaryWithCall(BinaryASTNode* node, ASTNode* call) {
  ASTNode* parent = node->base.parent;
  int child_id = node->base.child_id;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  return AnalyzeExpression(call);
}

static ASTNode* ReplaceUnaryWithCall(UnaryASTNode* node, ASTNode* call) {
  ASTNode* parent = node->base.parent;
  int child_id = node->base.child_id;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  return AnalyzeExpression(call);
}

static ASTNode* TryAnalyzeOverloadedUnaryOperator(UnaryASTNode* node) {
  const char* op_name = UnaryOperatorFunctionName(node->base.op);
  if (!CompilerIsCXX() || op_name == NULL) {
    return NULL;
  }

  node->sub = AnalyzeExpression(node->sub);
  if (!TypeIsStructOrUnion(node->sub->type)) {
    return NULL;
  }

  String name;
  StringInit(&name, op_name);
  StructMember* member = FindStructMember(node->sub->type->info.struct_info,
                                          &name);
  if (member != NULL && member->is_member_function) {
    ASTNode* receiver = ASTNodeMove(node->sub);
    ASTNode* member_name =
        NewStringConstantASTNode(NewString(op_name), NULL,
                                 node->base.location);
    ASTNode* member_access = NewBinaryASTNode(AST_OP(dot), NULL,
                                              node->base.location, receiver,
                                              member_name);
    ASTNode* call = NewVectorASTNode(AST_OP(call), NULL, node->base.location,
                                     member_access, NewVector());
    StringDestruct(&name);
    return ReplaceUnaryWithCall(node, call);
  }

  Symbol* function = FindGlobalSymbol(&name);
  if (function != NULL && TypeIsFunction(function->type)) {
    ASTNode* actual = ASTNodeMove(node->sub);
    Vector* actuals = NewVector();
    VectorAppend(actuals, actual);
    ASTNode* call = NewVectorASTNode(
        AST_OP(call), NULL, node->base.location,
        NewIdentifierASTNode(function, node->base.location), actuals);
    StringDestruct(&name);
    return ReplaceUnaryWithCall(node, call);
  }
  StringDestruct(&name);
  return NULL;
}

static ASTNode* TryAnalyzeOverloadedBinaryOperator(BinaryASTNode* node) {
  const char* op_name = BinaryOperatorFunctionName(node->base.op);
  if (!CompilerIsCXX() || op_name == NULL) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(node->left->type) &&
      !TypeIsStructOrUnion(node->right->type)) {
    return NULL;
  }

  String name;
  StringInit(&name, op_name);
  if (TypeIsStructOrUnion(node->left->type)) {
    StructMember* member =
        FindStructMember(node->left->type->info.struct_info, &name);
    if (member != NULL && member->is_member_function) {
      ASTNode* left = ASTNodeMove(node->left);
      ASTNode* right = ASTNodeMove(node->right);
      ASTNode* member_name =
          NewStringConstantASTNode(NewString(op_name), NULL,
                                   node->base.location);
      ASTNode* member_access = NewBinaryASTNode(AST_OP(dot), NULL,
                                                node->base.location, left,
                                                member_name);
      Vector* actuals = NewVector();
      VectorAppend(actuals, right);
      ASTNode* call = NewVectorASTNode(AST_OP(call), NULL,
                                       node->base.location, member_access,
                                       actuals);
      StringDestruct(&name);
      return ReplaceBinaryWithCall(node, call);
    }
  }

  Symbol* function = FindGlobalSymbol(&name);
  if (function != NULL && TypeIsFunction(function->type)) {
    ASTNode* left = ASTNodeMove(node->left);
    ASTNode* right = ASTNodeMove(node->right);
    Vector* actuals = NewVector();
    VectorAppend(actuals, left);
    VectorAppend(actuals, right);
    ASTNode* call = NewVectorASTNode(
        AST_OP(call), NULL, node->base.location,
        NewIdentifierASTNode(function, node->base.location), actuals);
    StringDestruct(&name);
    return ReplaceBinaryWithCall(node, call);
  }
  StringDestruct(&name);
  return NULL;
}

static ASTNode* TryAnalyzeOverloadedBinaryOperatorWithAnalyzedOperands(
    BinaryASTNode* node) {
  if (BinaryOperatorFunctionName(node->base.op) == NULL) {
    return NULL;
  }
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  return TryAnalyzeOverloadedBinaryOperator(node);
}

// A binary plus operator allows an integer to be added to a pointer (or array).
// The integer is scaled (multiplied) by the size of the thing pointed to.
static ASTNode* AnalyzePlusOperator(BinaryASTNode* node) {
  if (node == NULL) {
    return NULL;
  }
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
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
  return &node->base;
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

  // Each operand of a shift is integer-promoted independently and the type of
  // the result is the promoted type of the LEFT operand (C11 6.5.7p3).  The
  // usual arithmetic conversions are NOT applied, so the right operand's type
  // (e.g. a `long long` shift count) must not widen the result.
  int left_rank = GetRank(node->left->type);
  if (left_rank > 0 && left_rank < kIntRank) {
    Type t = kTypeInt;
    if (TypeIsUnsigned(node->left->type)) {
      t |= kTypeUnsigned;
    }
    NormalConversion(node->left, NewTypeRecordWithSize(t, kQualPlain));
  }
  int right_rank = GetRank(node->right->type);
  if (right_rank > 0 && right_rank < kIntRank) {
    Type t = kTypeInt;
    if (TypeIsUnsigned(node->right->type)) {
      t |= kTypeUnsigned;
    }
    NormalConversion(node->right, NewTypeRecordWithSize(t, kQualPlain));
  }
  ASTNodeSetType((ASTNode*)node, node->left->type);

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

static bool IsZeroIntegerConstant(ASTNode* node) {
  return node != NULL &&
         (node->op == AST_OP(number) || node->op == AST_OP(charconst)) &&
         ((ConstantASTNode*)node)->value.ivalue == 0;
}

static void AnalyzeComparisonOperator(BinaryASTNode* node) {
  AnalyzeBinaryExpression(node);
  if (TypeIsIntegral(node->left->type) && TypeIsIntegral(node->right->type) &&
      TypeIsUnsigned(node->left->type) != TypeIsUnsigned(node->right->type) &&
      !IsZeroIntegerConstant(node->left) && !IsZeroIntegerConstant(node->right)) {
    SemanticWarning((ASTNode*)node, "sign-compare",
                    "comparison of integers of different signs");
  }
  InsertNumericConversions(node, true);

  // Comparison operators produce boolean values.
  ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeBool, kQualPlain));
}

typedef enum {
  kConditionalFunctionArmOther,
  kConditionalFunctionArmNull,
  kConditionalFunctionArmFunction,
  kConditionalFunctionArmFunctionPointer,
} ConditionalFunctionArmKind;

static ConditionalFunctionArmKind GetConditionalFunctionArmKind(
    ASTNode* arm, TypeRecord** function_type) {
  *function_type = NULL;
  if (TypeIsFunction(arm->type)) {
    *function_type = arm->type;
    return kConditionalFunctionArmFunction;
  }
  if (TypeIsFunctionPointer(arm->type)) {
    *function_type = arm->type->next;
    return kConditionalFunctionArmFunctionPointer;
  }
  return IsNullPointer(arm) ? kConditionalFunctionArmNull
                            : kConditionalFunctionArmOther;
}

static bool TryAnalyzeConditionalFunctionPointer(BinaryASTNode* node,
                                                 BinaryASTNode* colon) {
  TypeRecord* left_function = NULL;
  TypeRecord* right_function = NULL;
  ConditionalFunctionArmKind left_kind =
      GetConditionalFunctionArmKind(colon->left, &left_function);
  ConditionalFunctionArmKind right_kind =
      GetConditionalFunctionArmKind(colon->right, &right_function);

  if (left_kind == kConditionalFunctionArmOther ||
      right_kind == kConditionalFunctionArmOther ||
      (left_kind == kConditionalFunctionArmNull &&
       right_kind == kConditionalFunctionArmNull)) {
    return false;
  }

  TypeRecord* common_function =
      left_function != NULL ? left_function : right_function;
  if (common_function == NULL ||
      (left_function != NULL && right_function != NULL &&
       !TypeEqual(left_function, right_function))) {
    return false;
  }

  TypeRecord* common_type = NULL;
  bool owns_common_type = false;
  if (left_kind == kConditionalFunctionArmFunctionPointer) {
    common_type = colon->left->type;
  } else if (right_kind == kConditionalFunctionArmFunctionPointer) {
    common_type = colon->right->type;
  } else {
    common_type = NewPointerTo(kQualPlain, common_function);
    owns_common_type = true;
  }

  if (left_kind == kConditionalFunctionArmFunction) {
    colon->left->flags |= kASTNeedAddress;
  }
  if (right_kind == kConditionalFunctionArmFunction) {
    colon->right->flags |= kASTNeedAddress;
  }
  ASTNodeSetType(colon->left, common_type);
  ASTNodeSetType(colon->right, common_type);
  ASTNodeSetType((ASTNode*)colon, common_type);
  ASTNodeSetType((ASTNode*)node, colon->base.type);
  if (owns_common_type) {
    TypeRecordDelete(common_type);
  }
  return true;
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
  // If either arm has void type (e.g. a statement expression whose last
  // statement is not an expression) the result of the conditional is void.
  if (TypeIsVoid(colon->left->type) || TypeIsVoid(colon->right->type)) {
    ASTNodeSetType((ASTNode*)colon, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    return;
  }
  if (TryAnalyzeConditionalFunctionPointer(node, colon)) {
    return;
  }
  InsertNumericConversions(colon, false);
  ASTNodeSetType((ASTNode*)node, colon->left->type);

  ASTNodeSetType((ASTNode*)node, colon->base.type);
}

// Does the node have an address?  In other words, can you take its
// address using the & operator?
static bool HasAddress(ASTNode* node) {
  if (ASTNodeIsGLValue(node)) {
    return !IsBitfieldReference(node);
  }
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

static ASTNode* MaterializeTemporary(ASTNode* expr, TypeRecord* type) {
  SourceLocation location = expr->location;
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  ASTNode* temp_id = NewIdentifierASTNode(temp, location);
  temp_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* initializer = NewExpressionInitializerASTNode(expr, location);
  ASTNode* materialized =
      NewCompoundLiteralASTNode(temp_id, location, initializer);
  materialized->value_category = kValueCategoryLvalue;
  return AnalyzeExpression(materialized);
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
  if (TypeContainsAuto(id_node->symbol->type)) {
    if (!SemanticDeduceAutoType(id_node->symbol, init, node)) {
      return init;
    }
    ASTNodeSetType(&id_node->base, id_node->symbol->type);
    ASTNodeSetType(node, id_node->symbol->type);
  }
  bool is_reference_init = TypeIsReference(id_node->symbol->type);
  switch (init->op) {
    case AST_OP(expr_init):{
      ExpressionInitializerASTNode* e = (ExpressionInitializerASTNode*)init;
      if (is_reference_init) {
        TypeRecord* reference_type = id_node->symbol->type;
        bool rvalue_ref =
            reference_type->declarator == kDeclRValueReference;
        bool discards_qualifiers =
            TypeIsConst(e->expr->type) && !TypeIsConst(reference_type->next);
        NormalConversion(e->expr, reference_type->next);
        if (discards_qualifiers) {
          SemanticError(e->expr, "Reference initializer discards qualifiers");
        } else if (!ReferenceCanBind(e->expr, reference_type)) {
          if (rvalue_ref) {
            SemanticError(e->expr,
                          "Rvalue reference initializer must not be an lvalue");
          } else if (TypeIsConst(reference_type->next)) {
            SemanticError(e->expr,
                          "Const reference initializer has incompatible type");
          } else {
            SemanticError(e->expr, "Reference initializer must be an lvalue");
          }
        }
        if (ReferenceCanBind(e->expr, reference_type) &&
            !HasAddress(e->expr)) {
          ASTNode* materialized =
              MaterializeTemporary(e->expr, reference_type->next);
          ASTNodeReplaceChild((ASTNode*)e, 0, materialized, false);
          e->expr = materialized;
        }
        e->expr->flags |= kASTNeedAddress;
      } else {
        NormalConversion(e->expr, id_node->base.type);
      }
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

// Convert the right operand of an arithmetic compound assignment (+=, -=, *=,
// /=).  In general `a OP= b` is `a = (typeof a)((T)a OP (T)b)` where T is the
// usual-arithmetic-conversion type of a and b.  When the left operand is a
// (single-precision) float and the right is a wider floating type, the
// arithmetic must be done in double precision and only narrowed to float on
// the store; otherwise (e.g. `a += 56.78`) the double literal would be rounded
// to float first, losing precision.  In that case the right operand is left in
// double and code generation performs the widening/narrowing.  All other cases
// keep the historical behaviour of converting the right operand to the left
// type.
static void ConvertCompoundAssignmentOperand(BinaryASTNode* node) {
  if (TypeIsFloat(node->left->type) &&
      (TypeIsDouble(node->right->type) || TypeIsLongDouble(node->right->type))) {
    NormalConversion(node->right, NewTypeRecordWithSize(kTypeDouble, kQualPlain));
    return;
  }
  NormalConversion(node->right, node->left->type);
}

static ASTNode* AnalyzeAssignmentExpression(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  if (node->base.op == AST_OP(assign)) {
    ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
    if (overloaded != NULL) {
      return overloaded;
    }
  }
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
        ConvertCompoundAssignmentOperand(node);
      }
      ASTNodeSetType((ASTNode*)node, node->left->type);
      break;
    case AST_OP(multeq):
    case AST_OP(diveq):
      ConvertCompoundAssignmentOperand(node);
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
  if (CompilerIsCXX()) {
    node->base.value_category = kValueCategoryLvalue;
  }
  return (ASTNode*)node;
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
  if (CompilerIsCXX() &&
      (node->base.op == AST_OP(preinc) || node->base.op == AST_OP(predec))) {
    node->base.value_category = kValueCategoryLvalue;
  }
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
  node->base.value_category = kValueCategoryLvalue;
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
  // __attribute__((noinline)) blocks inlining outright.
  if (func->symbol != NULL && func->symbol->flags.noinline) {
    return false;
  }
  // __attribute__((always_inline)) forces inlining even without the 'inline'
  // keyword (and bypasses the size heuristic below).
  bool force_inline = func->symbol != NULL && func->symbol->flags.always_inline;
  if ((!func->is_inline && !force_inline) || !func->symbol->flags.is_defined ||
      func->body == NULL ||
      func->symbol == compiler->current_function->info.function.symbol ||
      func->unknown_args || func->varargs) {
    return false;
  }
  const int kMaxInlineNodeCount = 100;    // Arbitrary.
  GotoFinder finder = {0, false};
  ASTNodeVisit(func->body, ExamineBody, 0, &finder);
  if (finder.found_goto) {
    // Can't inline a function containing a goto regardless of always_inline.
    return false;
  }
  if (force_inline) {
    return true;
  }
  return finder.node_count < kMaxInlineNodeCount;
}

// The class of argument a printf/scanf conversion expects.  Used by the
// format-string checker for lenient type matching.
typedef enum {
  kFmtNone,      // No argument (%%) or unknown conversion: skip.
  kFmtInvalid,   // Invalid conversion: diagnose and skip.
  kFmtInteger,   // %d %i %u %o %x %c and friends.
  kFmtDouble,    // %f %e %g %a (after default promotion the arg is a double).
  kFmtString,    // %s (a pointer).
  kFmtPointer,   // %p and %n (a pointer).
} FmtClass;

static bool IsValidFormatConversion(char conv, bool is_scanf) {
  if (conv == '\0') {
    return false;
  }
  if (is_scanf) {
    return strchr("diouxXaAeEfFgGsScCpn[%", conv) != NULL;
  }
  return strchr("diouxXcCaAeEfFgGsSpn%", conv) != NULL;
}

static FmtClass FormatConversionClass(char conv, bool is_scanf) {
  if (!IsValidFormatConversion(conv, is_scanf)) {
    return kFmtInvalid;
  }
  // For scanf every conversion takes a pointer to the destination.
  if (is_scanf) {
    return (conv == '%') ? kFmtNone : kFmtPointer;
  }
  switch (conv) {
    case 'd': case 'i': case 'u': case 'o':
    case 'x': case 'X': case 'c':
      return kFmtInteger;
    case 'f': case 'F': case 'e': case 'E':
    case 'g': case 'G': case 'a': case 'A':
      return kFmtDouble;
    case 's':
      return kFmtString;
    case 'p': case 'n':
      return kFmtPointer;
    default:
      return kFmtNone;
  }
}

// Checks one variadic argument against the class a conversion expects, emitting
// a (lenient) -Wformat warning only on clear mismatches.
static void CheckFormatArg(ASTNode* call, ASTNode* arg, FmtClass cls,
                           int arg_number) {
  if (arg == NULL || arg->type == NULL || cls == kFmtNone) {
    return;
  }
  TypeRecord* t = arg->type;
  bool ok = true;
  const char* expected = NULL;
  switch (cls) {
    case kFmtInteger:
      // Accept any integer/enum; flag floating point and pointers.
      ok = TypeIsIntegral(t) || TypeIsEnum(t) || TypeIsBool(t);
      expected = "integer";
      break;
    case kFmtDouble:
      ok = TypeIsFloatingPoint(t);
      expected = "floating-point";
      break;
    case kFmtString:
      ok = TypeIsPointerOrArray(t);
      expected = "string (char *)";
      break;
    case kFmtPointer:
      ok = TypeIsPointerOrArray(t);
      expected = "pointer";
      break;
    case kFmtInvalid:
    case kFmtNone:
      return;
  }
  if (!ok) {
    SemanticWarning(call, "format",
                    "format argument %d has the wrong type (expected %s)",
                    arg_number, expected);
  }
}

// If the callee carries a format(printf/scanf, fmt, first) attribute and the
// format argument is a string literal, validate the variadic arguments against
// the conversions in the format string (count and rough types).
static void CheckFormatCall(VectorASTNode* node, Symbol* callee) {
  Attribute* fmt = SymbolFindAttribute(callee, "format");
  if (fmt == NULL) {
    return;
  }
  const char* archetype = AttributeArgString(fmt, 0);
  long fmt_pos = 0;
  long first_pos = 0;
  if (archetype == NULL || !AttributeArgInt(fmt, 1, &fmt_pos) ||
      !AttributeArgInt(fmt, 2, &first_pos)) {
    return;
  }
  bool is_scanf = strstr(archetype, "scanf") != NULL;
  bool is_printf = strstr(archetype, "printf") != NULL;
  if (!is_scanf && !is_printf) {
    return;  // Unsupported archetype (e.g. strftime).
  }
  // first_pos == 0 means the arguments are not available to check here (e.g.
  // a vprintf-style function taking a va_list).
  if (first_pos == 0) {
    return;
  }

  size_t nargs = node->children->length;
  if (fmt_pos < 1 || (size_t)fmt_pos > nargs) {
    return;
  }
  size_t arg_index = (size_t)first_pos - 1;  // 0-based index into children.
  ASTNode* fmt_arg = (ASTNode*)node->children->value.p[fmt_pos - 1];
  if (fmt_arg == NULL || fmt_arg->op != AST_OP(string)) {
    SemanticWarning((ASTNode*)node, "format-nonliteral",
                    "format string is not a string literal");
    if (arg_index >= nargs) {
      SemanticWarning((ASTNode*)node, "format-security",
                      "format string is not a string literal and has no format arguments");
    }
    return;
  }
  String* format = ((ConstantASTNode*)fmt_arg)->value.string;
  const char* p = (format->value != NULL) ? format->value : "";
  if (*p == '\0') {
    SemanticWarning((ASTNode*)node, "format-zero-length",
                    "zero-length format string");
  }

  int conversions = 0;
  while (*p != '\0') {
    if (*p != '%') {
      p++;
      continue;
    }
    p++;  // Consume '%'.
    if (*p == '%') {
      p++;
      continue;
    }
    bool suppress = false;
    // Flags.
    while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') {
      p++;
    }
    if (is_scanf && *p == '*') {
      suppress = true;  // Assignment-suppressing: consumes no argument.
      p++;
    }
    // Width: digits, or '*' (printf consumes an int argument for it).
    if (*p == '*') {
      if (!is_scanf) {
        CheckFormatArg((ASTNode*)node, arg_index < nargs
                           ? (ASTNode*)node->children->value.p[arg_index]
                           : NULL,
                       kFmtInteger, (int)arg_index + 1);
        arg_index++;
        conversions++;
      }
      p++;
    } else {
      while (isdigit((unsigned char)*p)) {
        p++;
      }
    }
    // Precision.
    if (*p == '.') {
      p++;
      if (*p == '*') {
        if (!is_scanf) {
          CheckFormatArg((ASTNode*)node, arg_index < nargs
                             ? (ASTNode*)node->children->value.p[arg_index]
                             : NULL,
                         kFmtInteger, (int)arg_index + 1);
          arg_index++;
          conversions++;
        }
        p++;
      } else {
        while (isdigit((unsigned char)*p)) {
          p++;
        }
      }
    }
    // Length modifiers.
    while (*p == 'h' || *p == 'l' || *p == 'L' || *p == 'j' || *p == 'z' ||
           *p == 't') {
      p++;
    }
    if (*p == '\0') {
      SemanticWarning((ASTNode*)node, "format-invalid-specifier",
                      "incomplete format specifier");
      break;
    }
    char conv = *p;
    p++;
    FmtClass cls = FormatConversionClass(conv, is_scanf);
    if (cls == kFmtInvalid) {
      SemanticWarning((ASTNode*)node, "format-invalid-specifier",
                      "invalid conversion specifier '%c' in format string",
                      conv);
      continue;
    }
    if (cls == kFmtNone || suppress) {
      continue;
    }
    conversions++;
    if (arg_index >= nargs) {
      SemanticWarning((ASTNode*)node, "format",
                      "too few arguments for format string");
      return;
    }
    CheckFormatArg((ASTNode*)node,
                   (ASTNode*)node->children->value.p[arg_index], cls,
                   (int)arg_index + 1);
    arg_index++;
  }
  if (arg_index < nargs) {
    SemanticWarning((ASTNode*)node, "format",
                    "too many arguments for format string");
  }
  (void)conversions;
}

static void RenumberVectorChildren(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* child = node->children->value.p[i];
    child->parent = &node->base;
    child->child_id = (int)i;
  }
}

static bool MemberReceiverIsConst(BinaryASTNode* node);
static const char* CXXAccessName(CXXAccess access);
static bool CurrentFunctionCanAccessMember(Struct* lookup_context,
                                           Struct* owner,
                                           CXXAccess original_access,
                                           CXXAccess effective_access);
static StructMember* ResolveMemberFunctionOverload(StructMember* first,
                                                   VectorASTNode* node,
                                                   BinaryASTNode* member_access);

static bool CurrentFunctionIsCXXCtorOrDtor(void) {
  return compiler->current_function != NULL &&
         TypeIsFunction(compiler->current_function) &&
         (compiler->current_function->info.function.is_constructor ||
          compiler->current_function->info.function.is_destructor);
}

static ASTNode* IdentityCloneNode(ASTNode* node, void* data) {
  (void)data;
  return node;
}

static ASTNode* CloneReceiverForVirtualLookup(ASTNode* receiver) {
  if (receiver->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)receiver;
    ASTNode* clone = NewIdentifierASTNode(id->symbol, receiver->location);
    ASTNodeSetType(clone, receiver->type);
    clone->flags |= receiver->flags & kASTNeedAddress;
    return clone;
  }
  return ASTNodeClone(receiver, IdentityCloneNode, NULL, NULL);
}

static TypeRecord* CopyFunctionTypeForVirtualCall(TypeRecord* function_type) {
  TypeRecord* copy = TypeRecordCopy(function_type);
  VectorInit(&copy->info.function.prototype);
  for (size_t i = 0; i < function_type->info.function.prototype.length; i++) {
    Symbol* formal = function_type->info.function.prototype.value.p[i];
    VectorAppend(&copy->info.function.prototype, SymbolClone(formal));
  }
  return copy;
}

static ASTNode* NewVirtualCalleeFromReceiver(ASTNode* receiver,
                                            StructMember* member,
                                            bool receiver_is_pointer,
                                            SourceLocation location) {
  if (receiver == NULL || member == NULL || member->symbol == NULL ||
      !TypeIsFunction(member->symbol->type) ||
      member->symbol->type->info.function.virtual_index < 0) {
    return NULL;
  }
  ASTNode* receiver_clone = CloneReceiverForVirtualLookup(receiver);
  if (!receiver_is_pointer) {
    receiver_clone =
        NewUnaryASTNode(AST_OP(address), NULL, location, receiver_clone);
  }
  ASTNode* vptr_name =
      NewStringConstantASTNode(NewString("__vptr"), NULL, location);
  ASTNode* vptr =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver_clone,
                       vptr_name);
  ASTNode* index =
      NewIntConstantASTNode(member->symbol->type->info.function.virtual_index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* slot =
      NewBinaryASTNode(AST_OP(subscript), NULL, location, vptr, index);
  TypeRecord* function_type =
      CopyFunctionTypeForVirtualCall(member->symbol->type);
  TypeRecord* function_pointer = NewPointerTo(kQualPlain, function_type);
  slot = AnalyzeExpression(slot);
  ASTNodeSetType(slot, function_pointer);
  return slot;
}

static bool LowerMemberFunctionCall(VectorASTNode* node) {
  if (node->left == NULL ||
      (node->left->op != AST_OP(dot) && node->left->op != AST_OP(arrow))) {
    return false;
  }

  BinaryASTNode* member_access = (BinaryASTNode*)node->left;
  if (member_access->right == NULL ||
      member_access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member_node =
      (StructMemberASTNode*)member_access->right;
  StructMember* member = member_node->member;
  if (!member->is_member_function) {
    return false;
  }

  member = ResolveMemberFunctionOverload(member, node, member_access);
  member_node->member = member;

  Struct* owner = member->symbol->type->info.function.cxx_member_owner;
  CXXAccess access = member->access;
  if (member_access->right->op == AST_OP(structmember)) {
    access = ((StructMemberASTNode*)member_access->right)->access;
  }
  Struct* lookup_context = NULL;
  if (member_access->base.op == AST_OP(arrow) &&
      TypeIsStructOrUnionPointer(member_access->left->type)) {
    lookup_context = member_access->left->type->next->info.struct_info;
  } else if (TypeIsStructOrUnion(member_access->left->type)) {
    lookup_context = member_access->left->type->info.struct_info;
  }
  if (!CurrentFunctionCanAccessMember(lookup_context, owner, member->access,
                                      access)) {
    const char* owner_name =
        owner != NULL && owner->tag_name != NULL
            ? owner->tag_name->value
            : "<anonymous>";
    SemanticError((ASTNode*)member_access, "%s is a %s member of %s",
                  member->symbol->name.value, CXXAccessName(access),
                  owner_name);
  }

  ASTNode* receiver = NULL;
  if (!member->is_static) {
    if (!member->symbol->type->info.function.is_const_member &&
        !member->symbol->type->info.function.is_constructor &&
        !member->symbol->type->info.function.is_destructor &&
        MemberReceiverIsConst(member_access)) {
      SemanticError((ASTNode*)member_access,
                    "Cannot call non-const member function %s on const object",
                    member->symbol->name.value);
    }
    receiver = ASTNodeMove(member_access->left);
    if (member_access->base.op == AST_OP(dot)) {
      receiver = NewUnaryASTNode(AST_OP(address), NULL, receiver->location,
                                 receiver);
      receiver = AnalyzeExpression(receiver);
    }
  }

  ASTNode* old_left = node->left;
  bool use_virtual_dispatch =
      member->symbol->type->info.function.is_virtual &&
      !member->is_static && !CurrentFunctionIsCXXCtorOrDtor();
  ASTNode* virtual_callee = NULL;
  if (use_virtual_dispatch) {
    virtual_callee =
        NewVirtualCalleeFromReceiver(receiver, member,
                                     true,
                                     old_left->location);
  }
  node->left = virtual_callee != NULL
                   ? virtual_callee
                   : NewIdentifierASTNode(member->symbol, old_left->location);
  node->left->parent = &node->base;
  node->left->child_id = 0;
  if (virtual_callee == NULL) {
    node->left = AnalyzeExpression(node->left);
  }
  ASTNodeDelete(old_left);

  if (receiver != NULL) {
    if (node->children->length == 0) {
      VectorAppend(node->children, receiver);
    } else {
      VectorInsertBefore(node->children, 0, receiver);
    }
    RenumberVectorChildren(node);
  }
  return true;
}

static const char* CXXAccessName(CXXAccess access) {
  switch (access) {
    case kAccessPublic:
      return "public";
    case kAccessProtected:
      return "protected";
    case kAccessPrivate:
      return "private";
  }
  return "unknown";
}

static bool CurrentFunctionCanAccessMember(Struct* lookup_context,
                                           Struct* owner,
                                           CXXAccess original_access,
                                           CXXAccess effective_access) {
  if (effective_access == kAccessPublic) {
    return true;
  }
  if (compiler->current_function == NULL ||
      !TypeIsFunction(compiler->current_function)) {
    return false;
  }
  Struct* current_owner =
      compiler->current_function->info.function.cxx_member_owner;
  if (current_owner == owner) {
    return true;
  }
  if (original_access == kAccessPrivate) {
    return false;
  }
  if (effective_access == kAccessPrivate) {
    return current_owner == lookup_context;
  }
  return effective_access == kAccessProtected &&
         (current_owner == lookup_context ||
          StructIsDerivedFrom(current_owner, lookup_context,
                              /*public_only=*/false));
}

static bool MemberReceiverIsConst(BinaryASTNode* node) {
  if (node->base.op == AST_OP(arrow)) {
    return TypeIsPointerOrArray(node->left->type) &&
           TypeIsConst(node->left->type->next);
  }
  return TypeIsConst(node->left->type);
}

static bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right);

static bool ReferenceCanBind(ASTNode* actual, TypeRecord* reference_type) {
  if (!TypeIsReference(reference_type)) {
    return false;
  }
  if (TypeIsConst(actual->type) && !TypeIsConst(reference_type->next)) {
    return false;
  }
  if (reference_type->declarator == kDeclRValueReference) {
    return !ASTNodeIsLValue(actual);
  }
  if (ASTNodeIsLValue(actual)) {
    return true;
  }
  return TypeIsConst(reference_type->next);
}

static int ReferenceBindingRank(ASTNode* actual, TypeRecord* reference_type) {
  if (!ReferenceCanBind(actual, reference_type)) {
    return -1;
  }
  bool target_const = TypeIsConst(reference_type->next);
  if (reference_type->declarator == kDeclRValueReference) {
    return 0;
  }
  if (ASTNodeIsLValue(actual)) {
    return target_const ? 1 : 0;
  }
  return 2;
}

static int OverloadBaseConversionRank(TypeRecord* actual, TypeRecord* target) {
  if (TypeEqual(actual, target) || TypeEqualIgnoringQualifiers(actual, target)) {
    return 0;
  }
  if (TypeEqualIgnoringSign(actual, target)) {
    return 1;
  }
  if (TypeIsIntegral(actual) && TypeIsIntegral(target)) {
    return 2;
  }
  if (TypeIsPointerOrArray(actual) && TypeIsPointerOrArray(target)) {
    if (TypeAssignmentCompatible(actual, target)) {
      return 1;
    }
    if (TypeIsVoidPointer(actual) || TypeIsVoidPointer(target)) {
      return 2;
    }
  }
  return -1;
}

static int OverloadConversionRank(ASTNode* actual, TypeRecord* formal_type) {
  TypeRecord* target = formal_type;
  bool reference = TypeIsReference(formal_type);
  if (reference) {
    target = formal_type->next;
    int binding_rank = ReferenceBindingRank(actual, formal_type);
    if (binding_rank < 0) {
      return -1;
    }
    int base_rank = OverloadBaseConversionRank(actual->type, target);
    return base_rank < 0 ? -1 : base_rank * 10 + binding_rank;
  }

  int base_rank = OverloadBaseConversionRank(actual->type, target);
  if (base_rank >= 0) {
    return base_rank * 10 + 5;
  }
  if (IsZeroIntegerConstant(actual) && TypeIsPointer(target)) {
    return 25;
  }
  return -1;
}

static int FunctionCallScore(TypeRecord* func, VectorASTNode* node,
                             size_t first_formal_arg) {
  if (!TypeIsFunction(func) || func->info.function.unknown_args) {
    return -1;
  }
  size_t num_actual_args = node->children->length;
  size_t num_formal_args = func->info.function.prototype.length;
  if (first_formal_arg > num_formal_args) {
    return -1;
  }
  size_t num_user_formal_args = num_formal_args - first_formal_arg;
  if (num_actual_args < num_user_formal_args) {
    return -1;
  }
  if (!func->info.function.varargs &&
      num_actual_args != num_user_formal_args) {
    return -1;
  }

  int score = 0;
  for (size_t i = 0; i < num_user_formal_args; i++) {
    ASTNode* actual = (ASTNode*)node->children->value.p[i];
    Symbol* formal =
        (Symbol*)func->info.function.prototype.value.p[i + first_formal_arg];
    int rank = OverloadConversionRank(actual, formal->type);
    if (rank < 0) {
      return -1;
    }
    score += rank;
  }
  return score;
}

static int OverloadCallScore(Symbol* candidate, VectorASTNode* node) {
  return FunctionCallScore(candidate->type, node, 0);
}

static Symbol* FunctionTemplateOverloadCandidate(Symbol* candidate,
                                                 VectorASTNode* node,
                                                 Vector* explicit_args) {
  if (candidate == NULL || candidate->type == NULL ||
      !TypeIsFunction(candidate->type)) {
    return candidate;
  }
  if (candidate->type->info.function.template_origin != NULL) {
    return NULL;
  }
  if (!candidate->flags.is_template) {
    return explicit_args == NULL ? candidate : NULL;
  }
  Symbol* instantiated =
      TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
          &compiler->syntax, candidate, explicit_args, node->children);
  return instantiated == candidate ? NULL : instantiated;
}

static int MemberOverloadCallScore(StructMember* candidate,
                                   VectorASTNode* node,
                                   BinaryASTNode* member_access,
                                   bool check_receiver_const) {
  size_t first_formal_arg = candidate->is_static ? 0 : 1;
  if (check_receiver_const && !candidate->is_static &&
      !candidate->symbol->type->info.function.is_const_member &&
      !candidate->symbol->type->info.function.is_constructor &&
      !candidate->symbol->type->info.function.is_destructor &&
      MemberReceiverIsConst(member_access)) {
    return -1;
  }
  int score = FunctionCallScore(candidate->symbol->type, node, first_formal_arg);
  if (score >= 0 && !candidate->is_static &&
      candidate->symbol->type->info.function.is_const_member &&
      !MemberReceiverIsConst(member_access)) {
    score++;
  }
  return score;
}

static StructMember* MemberTemplateOverloadCandidate(StructMember* candidate,
                                                     VectorASTNode* node,
                                                     Vector* explicit_args) {
  if (candidate == NULL || candidate->symbol == NULL ||
      candidate->symbol->type == NULL ||
      !TypeIsFunction(candidate->symbol->type)) {
    return candidate;
  }
  if (candidate->symbol->type->info.function.template_origin != NULL) {
    return NULL;
  }
  if (!candidate->symbol->flags.is_template) {
    return explicit_args == NULL ? candidate : NULL;
  }
  size_t first_formal_arg = candidate->is_static ? 0 : 1;
  Symbol* instantiated =
      TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
          &compiler->syntax, candidate->symbol, explicit_args, node->children,
          first_formal_arg);
  if (instantiated == candidate->symbol) {
    return NULL;
  }
  StructMember* member = NewStructMember(instantiated);
  member->is_member_function = true;
  member->is_static = candidate->is_static;
  member->access = candidate->access;
  return member;
}

static StructMember* ResolveMemberFunctionOverload(StructMember* first,
                                                   VectorASTNode* node,
                                                   BinaryASTNode* member_access) {
  StructMember* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  bool receiver_const = MemberReceiverIsConst(member_access);
  bool receiver_const_rejected = false;
  Vector* explicit_args = NULL;
  if (member_access->right != NULL &&
      member_access->right->op == AST_OP(structmember)) {
    explicit_args =
        ((StructMemberASTNode*)member_access->right)->template_arguments;
  }

  if (explicit_args == NULL) {
    for (StructMember* candidate = first; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->symbol != NULL &&
          (candidate->symbol->flags.is_template ||
           (candidate->symbol->type != NULL &&
            TypeIsFunction(candidate->symbol->type) &&
            candidate->symbol->type->info.function.template_origin != NULL))) {
        continue;
      }
      int score = MemberOverloadCallScore(candidate, node, member_access, true);
      if (score < 0) {
        if (receiver_const &&
            MemberOverloadCallScore(candidate, node, member_access, false) >= 0) {
          receiver_const_rejected = true;
        }
        continue;
      }
      if (best == NULL || score < best_score) {
        best = candidate;
        best_score = score;
        ambiguous = false;
      } else if (score == best_score) {
        ambiguous = true;
      }
    }
  }

  if (best_score < 0 || best_score > 5) {
    for (StructMember* candidate = first; candidate != NULL;
         candidate = candidate->overload_next) {
      StructMember* effective =
          MemberTemplateOverloadCandidate(candidate, node, explicit_args);
      if (effective == NULL) {
        continue;
      }
      int score = MemberOverloadCallScore(effective, node, member_access, true);
      if (score < 0) {
        if (receiver_const &&
            MemberOverloadCallScore(effective, node, member_access, false) >= 0) {
          receiver_const_rejected = true;
        }
        continue;
      }
      if (best == NULL || score < best_score) {
        best = effective;
        best_score = score;
        ambiguous = false;
      } else if (score == best_score) {
        bool best_is_template =
            best->symbol != NULL && best->symbol->type != NULL &&
            TypeIsFunction(best->symbol->type) &&
            best->symbol->type->info.function.template_origin != NULL;
        bool effective_is_template =
            effective->symbol != NULL && effective->symbol->type != NULL &&
            TypeIsFunction(effective->symbol->type) &&
            effective->symbol->type->info.function.template_origin != NULL;
        if (best_is_template && !effective_is_template) {
          best = effective;
          ambiguous = false;
        } else if (best != effective &&
                   best_is_template == effective_is_template) {
          ambiguous = true;
        }
      }
    }
  }

  if (best == NULL) {
    if (receiver_const_rejected) {
      SemanticError((ASTNode*)member_access,
                    "Cannot call non-const member function %s on const object",
                    first->symbol->name.value);
    } else if (first->overload_next != NULL ||
               first->symbol->type->info.function.is_constructor) {
      SemanticError((ASTNode*)node, "No matching overload for %s",
                    first->symbol->name.value);
    }
    return first;
  }
  if (ambiguous) {
    SemanticError((ASTNode*)node, "Ambiguous overload for %s",
                  first->symbol->name.value);
    return best;
  }
  return best;
}

static void ResolveOverloadedFunctionCall(VectorASTNode* node) {
  if (node->left == NULL || node->left->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node->left;
  if (id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.template_origin != NULL) {
    return;
  }
  if (!id->symbol->flags.is_overloaded) {
    return;
  }

  Symbol* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  if (id->template_arguments == NULL) {
    for (Symbol* candidate = id->symbol; candidate != NULL;
         candidate = candidate->overload_next) {
      if (candidate->flags.is_template ||
          (candidate->type != NULL && TypeIsFunction(candidate->type) &&
           candidate->type->info.function.template_origin != NULL)) {
        continue;
      }
      int score = OverloadCallScore(candidate, node);
      if (score < 0) {
        continue;
      }
      if (best == NULL || score < best_score) {
        best = candidate;
        best_score = score;
        ambiguous = false;
      } else if (score == best_score) {
        ambiguous = true;
      }
    }
  }

  if (best_score < 0 || best_score > 5) {
    for (Symbol* candidate = id->symbol; candidate != NULL;
         candidate = candidate->overload_next) {
      Symbol* effective =
          FunctionTemplateOverloadCandidate(candidate, node,
                                            id->template_arguments);
      if (effective == NULL) {
        continue;
      }
      int score = OverloadCallScore(effective, node);
      if (score < 0) {
        continue;
      }
      if (best == NULL || score < best_score) {
        best = effective;
        best_score = score;
        ambiguous = false;
      } else if (score == best_score) {
        bool best_is_template =
            best->type != NULL && TypeIsFunction(best->type) &&
            best->type->info.function.template_origin != NULL;
        bool effective_is_template =
            effective->type != NULL && TypeIsFunction(effective->type) &&
            effective->type->info.function.template_origin != NULL;
        if (best_is_template && !effective_is_template) {
          best = effective;
          ambiguous = false;
        } else if (best != effective &&
                   best_is_template == effective_is_template) {
          ambiguous = true;
        }
      }
    }
  }

  if (best == NULL) {
    SemanticError((ASTNode*)node, "No matching overload for %s",
                  id->symbol->name.value);
    return;
  }
  if (ambiguous) {
    SemanticError((ASTNode*)node, "Ambiguous overload for %s",
                  id->symbol->name.value);
    return;
  }

  id->symbol = best;
  ASTNodeSetType(node->left, best->type);
}

static ASTNode* AnalyzeCXXFunctionalClassConstruction(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      !TypeIsStructOrUnion(node->left->type) ||
      node->left->type->info.struct_info == NULL ||
      node->left->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }

  TypeRecord* type = TypeRecordCopy(node->left->type);
  TypeRecordCalculateSize(type);
  SourceLocation location = node->base.location;
  String* constructor_name = type->info.struct_info->tag_name;
  StructMember* constructor =
      FindStructMember(type->info.struct_info, constructor_name);
  if (constructor == NULL || !constructor->is_member_function ||
      !constructor->symbol->type->info.function.is_constructor) {
    return NULL;
  }

  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(constructor_name->value), NULL,
                               location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);

  Vector* actuals = NewVector();
  for (size_t i = 0; i < node->children->length; i++) {
    VectorAppend(actuals, ASTNodeMove(node->children->value.p[i]));
  }
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* result = NewIdentifierASTNode(temp, location);
  ASTNode* comma =
      NewBinaryASTNode(AST_OP(comma), type, location, constructor_call, result);

  ASTNode* parent = node->base.parent;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, node->base.child_id, comma, true);
  }
  ASTNode* analyzed = AnalyzeExpression(comma);
  analyzed->value_category = kValueCategoryPrvalue;
  return analyzed;
}

static ASTNode* AnalyzeFunctionCall(VectorASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  size_t num_actual_args = node->children->length;
  for (size_t i = 0; i < num_actual_args; i++) {
    node->children->value.p[i] = AnalyzeExpression((ASTNode*)node->children->value.p[i]);
  }
  ASTNode* construction = AnalyzeCXXFunctionalClassConstruction(node);
  if (construction != NULL) {
    return construction;
  }
  LowerMemberFunctionCall(node);
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && id->symbol->flags.is_template &&
        !id->symbol->flags.is_overloaded && TypeIsFunction(id->symbol->type)) {
      Symbol* instantiated =
          TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
              &compiler->syntax, id->symbol, id->template_arguments,
              node->children);
      if (instantiated != id->symbol) {
        id->symbol = instantiated;
        ASTNodeSetType(node->left, instantiated->type);
      }
    }
  }
  ResolveOverloadedFunctionCall(node);
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && id->symbol->flags.is_template) {
      if (TypeIsFunction(id->symbol->type) &&
          !TypeCanDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
              id->symbol, id->template_arguments, node->children, 0)) {
        SemanticError((ASTNode*)node, "Template argument deduction failed");
      } else {
        SemanticError((ASTNode*)node,
                      "Template instantiation is not supported yet");
      }
    }
  }
  num_actual_args = node->children->length;
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

  TypeRecord* return_type = subtype->next;
  if (TypeIsReference(return_type)) {
    ASTNodeSetType((ASTNode*)node, return_type->next);
    node->base.value_category =
        return_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)node, return_type);
  }

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
      if (TypeIsReference(formal->type)) {
        TypeRecord* reference_type = formal->type;
        bool discards_qualifiers =
            TypeIsConst(actual->type) && !TypeIsConst(reference_type->next);
        NormalConversion(actual, reference_type->next);
        if (discards_qualifiers) {
          SemanticError(actual, "Reference argument discards qualifiers");
        } else if (!ReferenceCanBind(actual, reference_type)) {
          if (reference_type->declarator == kDeclRValueReference) {
            SemanticError(actual,
                          "Rvalue reference argument must not be an lvalue");
          } else if (TypeIsConst(reference_type->next)) {
            SemanticError(actual, "Const reference argument has incompatible type");
          } else {
            SemanticError(actual, "Reference argument must be an lvalue");
          }
        }
        if (ReferenceCanBind(actual, reference_type) && !HasAddress(actual)) {
          ASTNode* materialized =
              MaterializeTemporary(actual, reference_type->next);
          ASTNodeReplaceChild((ASTNode*)node, (int)i, materialized, false);
          actual = materialized;
        }
        actual->flags |= kASTNeedAddress;
      } else {
        NormalConversion(actual, formal->type);
      }

      // Composites (structs/unions), arrays and functions need addresses, not
      // values.
      if (TypeIsStructOrUnion(actual->type) || TypeIsArray(actual->type) ||
          TypeIsFunction(actual->type)) {
        actual->flags |= kASTNeedAddress;
      }
    }
  }

  // Default argument promotions apply to the variadic part of a call (the
  // arguments matched by "...") and to every argument of an unprototyped
  // function.  In particular a 'float' actual is promoted to 'double'; without
  // this a single-precision value would be passed where the callee (e.g.
  // printf's %f) expects a double.
  {
    bool unknown = subtype->info.function.unknown_args;
    bool varargs = subtype->info.function.varargs;
    if (unknown || varargs) {
      size_t start = unknown ? 0 : num_formal_args;
      for (size_t i = start; i < num_actual_args; i++) {
        ASTNode* actual = (ASTNode*)node->children->value.p[i];
        if (TypeIsFloat(actual->type)) {
          NormalConversion(actual, NewTypeRecordWithSize(kTypeDouble, kQualPlain));
        }
      }
    }
  }
  
  // Validate printf/scanf-style format strings on functions annotated with
  // __attribute__((format(...))).
  if (call_ok && node->left->op == AST_OP(identifier)) {
    Symbol* callee = ((IdentifierASTNode*)node->left)->symbol;
    if (callee != NULL) {
      CheckFormatCall(node, callee);
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
  CXXAccess access = kAccessPublic;
  Struct* member_owner = NULL;
  StructMember* member =
      FindStructMemberWithAccess(struct_info, member_name, &access,
                                 &member_owner);
  if (member == NULL) {
    SemanticError((ASTNode*)node, "%s is not a member of struct/union %s",
                  member_name->value, struct_info->tag_name->value);
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  Vector* explicit_template_arguments =
      ((ConstantASTNode*)node->right)->template_arguments;

  if (!member->is_member_function &&
      !CurrentFunctionCanAccessMember(struct_info, member_owner,
                                      member->access, access)) {
    const char* owner_name =
        member_owner != NULL && member_owner->tag_name != NULL
            ? member_owner->tag_name->value
            : "<anonymous>";
    SemanticError((ASTNode*)node, "%s is a %s member of %s",
                  member_name->value, CXXAccessName(access),
                  owner_name);
  }

  // Replace the right node with a StructMember AST node.
  ASTNode* old_right = node->right;
  node->right = NewStructMemberASTNode(member, node->right->location);
  ((StructMemberASTNode*)node->right)->access = access;
  ((StructMemberASTNode*)node->right)->template_arguments =
      TemplateArgumentVectorCopy(explicit_template_arguments);
  ASTNodeDelete(old_right);
  TypeRecord* member_type = member->symbol->type;
  if (!member->is_static && !member->is_member_function &&
      MemberReceiverIsConst(node)) {
    member_type = TypeRecordCopy(member_type);
    member_type->qualifiers |= kQualConst;
  }
  ASTNodeSetType((ASTNode*)node, member_type);
  if (!member->is_member_function) {
    node->base.value_category = kValueCategoryLvalue;
  }
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
  node->base.value_category = kValueCategoryLvalue;
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

static bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right) {
  if (left == NULL || right == NULL || left->declarator != right->declarator) {
    return false;
  }
  switch (left->declarator) {
    case kDeclPointer:
    case kDeclReference:
    case kDeclRValueReference:
    case kDeclArray:
      return TypeEqualIgnoringQualifiers(left->next, right->next);
    case kDeclFunction:
      return TypeEqual(left, right);
    case kDeclPrimitive:
      if (TypeIsStructOrUnion(left) || TypeIsStructOrUnion(right)) {
        return TypeIsStructOrUnion(left) && TypeIsStructOrUnion(right) &&
               left->info.struct_info == right->info.struct_info;
      }
      if (TypeIsEnum(left) || TypeIsEnum(right)) {
        return TypeIsEnum(left) && TypeIsEnum(right) &&
               left->info.enum_info == right->info.enum_info;
      }
      return left->type == right->type;
  }
}

static void ValidateCXXConstCast(CastASTNode* node) {
  TypeRecord* to = node->cast_type;
  TypeRecord* from = node->expr->type;
  bool to_indirect = TypeIsPointer(to) || TypeIsReference(to);
  bool from_indirect = TypeIsPointer(from) || TypeIsReference(from);
  if (!to_indirect || !from_indirect || to->next == NULL ||
      from->next == NULL ||
      !TypeEqualIgnoringQualifiers(to->next, from->next)) {
    SemanticError((ASTNode*)node,
                  "const_cast requires pointer or reference to the same type");
  }
}

static void AnalyzeCastExpression(CastASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  if (node->kind == kCastDynamic) {
    SemanticError((ASTNode*)node, "dynamic_cast is not supported yet");
  } else if (node->kind == kCastConst) {
    ValidateCXXConstCast(node);
  }
  if (TypeIsReference(node->cast_type)) {
    if (!TypeEqualIgnoringQualifiers(node->expr->type, node->cast_type->next)) {
      SemanticConvertType(node->expr, node->cast_type->next, kConvertCast);
    }
    ASTNodeSetType((ASTNode*)node, node->cast_type->next);
    node->base.value_category =
        node->cast_type->declarator == kDeclRValueReference
            ? kValueCategoryXvalue
            : kValueCategoryLvalue;
  } else {
    SemanticConvertType(node->expr, node->cast_type, kConvertCast);
    // Result is the requested type.
    ASTNodeSetType((ASTNode*)node, node->cast_type);
  }
}

static void  AnalyzeCompoundLiteral(CompoundLiteralASTNode* node) {
  node->initializer = AnalyzeInitialization(&node->base,
                                             (IdentifierASTNode*)node->sym,
                        node->initializer);
  node->base.value_category = kValueCategoryLvalue;
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

  if (BinaryOperatorFunctionName(node->op) != NULL) {
    ASTNode* overloaded =
        TryAnalyzeOverloadedBinaryOperatorWithAnalyzedOperands(binary_node);
    if (overloaded != NULL) {
      return overloaded;
    }
  }
  if (UnaryOperatorFunctionName(node->op) != NULL) {
    ASTNode* overloaded = TryAnalyzeOverloadedUnaryOperator(unary_node);
    if (overloaded != NULL) {
      return overloaded;
    }
  }

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
      node = AnalyzePlusOperator(binary_node);
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
      node = AnalyzeAssignmentExpression(binary_node);
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
      node->value_category = binary_node->right->value_category;
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
      node->value_category = expr_init->expr->value_category;
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

    case AST_OP(stmt_expr): {
      // GCC statement expression: analyze the compound statement; the value
      // (and type) is that of the final statement if it is an expression
      // statement, otherwise void.
      AnalyzeStatement(unary_node->sub);
      CompoundStatementASTNode* comp =
          (CompoundStatementASTNode*)unary_node->sub;
      TypeRecord* type = NewTypeRecordWithSize(kTypeVoid, kQualPlain);
      if (comp->statements->length > 0) {
        ASTNode* last =
            comp->statements->value.p[comp->statements->length - 1];
        if (last->op == AST_OP(expr)) {
          ExpressionStatementASTNode* es = (ExpressionStatementASTNode*)last;
          if (es->expr != NULL && es->expr->type != NULL) {
            type = es->expr->type;
            node->value_category = es->expr->value_category;
          }
        }
      }
      ASTNodeSetType(node, type);
      break;
    }

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
