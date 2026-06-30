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
#include "errors.h"
#include "symbol_table.h"
#include "rtti.h"

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

static int OverloadBaseConversionRank(TypeRecord* actual, TypeRecord* target);
static int OverloadConversionRank(ASTNode* actual, TypeRecord* formal_type);

// A user-defined conversion (via a converting constructor) involves a standard
// conversion of the argument to the constructor's parameter.  While ranking
// that inner standard conversion we must not recursively consider yet another
// user-defined conversion (at most one is permitted), so this guard suppresses
// the user-defined-conversion search in OverloadConversionRank.
static bool g_suppress_user_defined_conversion_rank = false;

// Finds the unique non-explicit (unless allow_explicit) converting constructor
// of class type `to` that can be invoked with the single argument `from` using
// only standard conversions, or NULL if there is none or the choice is
// ambiguous.
static StructMember* FindConvertingConstructorCandidate(TypeRecord* to,
                                                        ASTNode* from,
                                                        bool allow_explicit);

static bool ReferenceCanBind(ASTNode* actual, TypeRecord* reference_type);
static Symbol* ResolveFreeFunctionWithADL(String* name, Vector* actuals,
                                          bool diagnose_ambiguous);
static Symbol* FunctionTemplateOverloadCandidate(Symbol* candidate,
                                                 VectorASTNode* node,
                                                 Vector* explicit_args);
static int OperatorCoAwaitCallScore(Symbol* candidate, VectorASTNode* node);
static Symbol* ResolveFreeOperatorCoAwaitForActual(ASTNode* actual,
                                                   bool diagnose_ambiguous);
static ASTNode* NewOperatorMemberCall(ASTNode* receiver, const char* op_name,
                                      Vector* actuals,
                                      SourceLocation location);
static ASTNode* NewOperatorFreeCall(Symbol* function, ASTNode* first_actual,
                                    Vector* remaining_actuals,
                                    SourceLocation location);

static ASTNode* AnalyzeIdentifier(IdentifierASTNode* node) {
  // A dependent qualified value name (`T::member`) that still carries its flag
  // here was never resolved during template instantiation, meaning the named
  // member does not exist in the substituted scope type.
  if ((node->base.flags & kASTDependentQualifiedName) != 0 &&
      node->symbol != NULL && node->symbol->type != NULL &&
      node->symbol->type->dependent_member_name != NULL) {
    SemanticError(&node->base, "no member named '%s' in the dependent scope",
                  node->symbol->type->dependent_member_name->value);
    ASTNodeSetType(&node->base, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return &node->base;
  }
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

static bool EvaluateConstantForSymbol(Symbol* symbol, ASTNode* initializer) {
  return EvaluateScalarConstantForSymbol(symbol, initializer) ||
         ConstexprEvaluateObjectConstantForSymbol(symbol, initializer);
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
  if (node->base.op == AST_OP(not)) {
    SemanticConvertType(node->sub,
                        NewTypeRecordWithSize(kTypeBool, kQualPlain),
                        kConvertContextualBool);
  }
  SemanticCheckScalarType(node->sub);
  switch (node->base.op) {
    case AST_OP(not):
      // Not operator is boolean.
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

static void AnalyzeCoAwaitExpression(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  if (compiler->current_function == NULL ||
      !compiler->current_function->info.function.is_coroutine) {
    SemanticError((ASTNode*)node, "co_await used outside a coroutine");
  }
  if (CompilerIsCXX() && TypeIsStructOrUnion(node->sub->type) &&
      node->sub->type->info.struct_info != NULL) {
    StructMember* member =
        FindStructMemberByName(node->sub->type->info.struct_info,
                               "operator co_await");
    if (member != NULL && member->is_member_function) {
      ASTNode* receiver = ASTNodeMove(node->sub);
      ASTNode* call = NewOperatorMemberCall(receiver, "operator co_await", NULL,
                                            node->base.location);
      ASTNodeReplaceChild((ASTNode*)node, 0, AnalyzeExpression(call), true);
    }
  }
  bool already_awaiter = false;
  if (CompilerIsCXX() && node->sub != NULL &&
      TypeIsStructOrUnion(node->sub->type) &&
      node->sub->type->info.struct_info != NULL) {
    StructMember* await_ready =
        FindStructMemberByName(node->sub->type->info.struct_info,
                               "await_ready");
    already_awaiter = await_ready != NULL && await_ready->is_member_function;
  }
  if (CompilerIsCXX() && !already_awaiter && node->sub != NULL &&
      node->sub->type != NULL) {
    Symbol* function =
        ResolveFreeOperatorCoAwaitForActual(node->sub,
                                            /*diagnose_ambiguous=*/true);
    if (function != NULL) {
      ASTNode* actual = ASTNodeMove(node->sub);
      ASTNode* call = NewOperatorFreeCall(function, actual, NULL,
                                          node->base.location);
      ASTNodeReplaceChild((ASTNode*)node, 0, AnalyzeExpression(call), true);
    }
  }
  TypeRecord* awaitable_type = node->sub != NULL ? node->sub->type : NULL;
  StructMember* await_resume = NULL;
  if (TypeIsStructOrUnion(awaitable_type) &&
      awaitable_type->info.struct_info != NULL) {
    StructMember* await_ready =
        FindStructMemberByName(awaitable_type->info.struct_info,
                               "await_ready");
    StructMember* await_suspend =
        FindStructMemberByName(awaitable_type->info.struct_info,
                               "await_suspend");
    await_resume = FindStructMemberByName(awaitable_type->info.struct_info,
                                          "await_resume");
    if (await_ready == NULL || !await_ready->is_member_function) {
      SemanticError((ASTNode*)node, "awaiter is missing await_ready");
    }
    if (await_suspend == NULL || !await_suspend->is_member_function) {
      SemanticError((ASTNode*)node, "awaiter is missing await_suspend");
    }
    if (await_resume == NULL || !await_resume->is_member_function) {
      SemanticError((ASTNode*)node, "awaiter is missing await_resume");
    }
  } else {
    SemanticError((ASTNode*)node, "co_await operand must be an awaiter object");
  }
  if (await_resume != NULL && await_resume->symbol != NULL &&
      TypeIsFunction(await_resume->symbol->type) &&
      await_resume->symbol->type->next != NULL) {
    ASTNodeSetType((ASTNode*)node, await_resume->symbol->type->next);
    return;
  }
  ASTNodeSetType((ASTNode*)node,
                 node->sub != NULL && node->sub->type != NULL
                     ? node->sub->type
                     : NewTypeRecordWithSize(kTypeInt, kQualPlain));
}

static void AnalyzeCoYieldExpression(UnaryASTNode* node) {
  node->sub = AnalyzeExpression(node->sub);
  if (compiler->current_function == NULL ||
      !compiler->current_function->info.function.is_coroutine) {
    SemanticError((ASTNode*)node, "co_yield used outside a coroutine");
  }
  TypeRecord* promise =
      compiler->current_function != NULL
          ? compiler->current_function->info.function.coroutine_promise_type
          : NULL;
  StructMember* yield_value = NULL;
  if (TypeIsStructOrUnion(promise) && promise->info.struct_info != NULL) {
    yield_value = FindStructMemberByName(promise->info.struct_info,
                                         "yield_value");
  }
  TypeRecord* awaiter_type =
      yield_value != NULL && yield_value->symbol != NULL &&
              TypeIsFunction(yield_value->symbol->type)
          ? yield_value->symbol->type->next
          : NULL;
  StructMember* await_resume = NULL;
  if (TypeIsStructOrUnion(awaiter_type) &&
      awaiter_type->info.struct_info != NULL) {
    await_resume = FindStructMemberByName(awaiter_type->info.struct_info,
                                          "await_resume");
  }
  if (await_resume != NULL && await_resume->symbol != NULL &&
      TypeIsFunction(await_resume->symbol->type) &&
      await_resume->symbol->type->next != NULL) {
    ASTNodeSetType((ASTNode*)node, await_resume->symbol->type->next);
    return;
  }
  if (yield_value != NULL) {
    SemanticError((ASTNode*)node,
                  "coroutine yield_value return type is invalid");
  }
  ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
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
    case AST_OP(comma):
      return "operator,";
    case AST_OP(assign):
      return "operator=";
    case AST_OP(pluseq):
      return "operator+=";
    case AST_OP(minuseq):
      return "operator-=";
    case AST_OP(multeq):
      return "operator*=";
    case AST_OP(diveq):
      return "operator/=";
    case AST_OP(percenteq):
      return "operator%=";
    case AST_OP(lshifteq):
      return "operator<<=";
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
      return "operator>>=";
    case AST_OP(andeq):
      return "operator&=";
    case AST_OP(oreq):
      return "operator|=";
    case AST_OP(exoreq):
      return "operator^=";
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
    case AST_OP(spaceship):
      return "operator<=>";
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
    case AST_OP(address):
      return "operator&";
    case AST_OP(contents):
      return "operator*";
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

static ASTNode* ReplaceVectorWithCall(VectorASTNode* node, ASTNode* call) {
  ASTNode* parent = node->base.parent;
  int child_id = node->base.child_id;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, call, true);
  }
  return AnalyzeExpression(call);
}

static ASTNode* NewOperatorMemberCall(ASTNode* receiver, const char* op_name,
                                      Vector* actuals,
                                      SourceLocation location) {
  ASTNode* member_name =
      NewStringConstantASTNode(NewString(op_name), NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member_name);
  return NewVectorASTNode(AST_OP(call), NULL, location, member_access,
                          actuals != NULL ? actuals : NewVector());
}

static ASTNode* NewOperatorFreeCall(Symbol* function, ASTNode* first_actual,
                                    Vector* remaining_actuals,
                                    SourceLocation location) {
  Vector* actuals = NewVector();
  if (first_actual != NULL) {
    VectorAppend(actuals, first_actual);
  }
  if (remaining_actuals != NULL) {
    VectorAppendVector(actuals, remaining_actuals);
  }
  return NewVectorASTNode(AST_OP(call), NULL, location,
                          NewIdentifierASTNode(function, location), actuals);
}

static Symbol* FollowUsingAliasForADL(Symbol* symbol) {
  int depth = 0;
  while (symbol != NULL && symbol->flags.is_using_alias &&
         symbol->alias_target != NULL && depth < 64) {
    symbol = symbol->alias_target;
    depth++;
  }
  return symbol;
}

static bool VectorContainsPointer(Vector* vec, void* value) {
  for (size_t i = 0; i < vec->length; i++) {
    if (vec->value.p[i] == value) {
      return true;
    }
  }
  return false;
}

static void ADLAddNamespace(Vector* namespaces, Namespace* ns) {
  // [basic.lookup.argdep]: the associated namespace of a class/enum is its
  // *innermost* enclosing namespace only - not every enclosing namespace up to
  // the global scope.  (Functions in the global namespace are still found, but
  // through ordinary unqualified lookup rather than ADL.)  Adding the whole
  // chain of ancestors would incorrectly make namespace-scope functions in an
  // enclosing namespace visible by ADL and could change overload resolution.
  if (ns != NULL && !VectorContainsPointer(namespaces, ns)) {
    VectorAppend(namespaces, ns);
  }
}

static TypeRecord* ADLCanonicalType(TypeRecord* type) {
  while (type != NULL &&
         (TypeIsReference(type) || TypeIsPointer(type) || TypeIsArray(type))) {
    type = type->next;
  }
  return type;
}

static void ADLCollectNamespacesForType(TypeRecord* type, Vector* namespaces,
                                        int depth) {
  if (type == NULL || depth > 8) {
    return;
  }
  type = ADLCanonicalType(type);
  if (type == NULL) {
    return;
  }
  if (TypeIsStructOrUnion(type) && type->info.struct_info != NULL) {
    Struct* str = type->info.struct_info;
    if (str->tag_symbol != NULL) {
      ADLAddNamespace(namespaces, str->tag_symbol->namespace_ != NULL
                                      ? str->tag_symbol->namespace_
                                      : compiler->global_namespace);
    }
    for (size_t i = 0; i < str->bases.length; i++) {
      CXXBaseSpecifier* base = str->bases.value.p[i];
      if (base != NULL) {
        ADLCollectNamespacesForType(base->type, namespaces, depth + 1);
      }
    }
  } else if (TypeIsEnum(type) && type->info.enum_info != NULL &&
             type->info.enum_info->tag_symbol != NULL) {
    Symbol* tag = type->info.enum_info->tag_symbol;
    ADLAddNamespace(namespaces, tag->namespace_ != NULL
                                    ? tag->namespace_
                                    : compiler->global_namespace);
  }
  if (type->template_arguments != NULL) {
    for (size_t i = 0; i < type->template_arguments->length; i++) {
      TemplateArgument* arg = type->template_arguments->value.p[i];
      if (arg != NULL && arg->kind == kTemplateParameterType) {
        ADLCollectNamespacesForType(arg->type, namespaces, depth + 1);
      }
    }
  }
}

static void AddFunctionOverloadCandidates(Vector* candidates, Symbol* first) {
  first = FollowUsingAliasForADL(first);
  for (Symbol* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    Symbol* effective = FollowUsingAliasForADL(candidate);
    if (effective != NULL && effective->type != NULL &&
        TypeIsFunction(effective->type) &&
        !VectorContainsPointer(candidates, effective)) {
      VectorAppend(candidates, effective);
    }
  }
}

static void AddNamedFunctionCandidates(String* name, Namespace* ns,
                                       Vector* candidates) {
  Symbol* found = (ns == NULL || ns == compiler->global_namespace)
                      ? FindGlobalSymbol(name)
                      : NamespaceFindSymbol(ns, name);
  AddFunctionOverloadCandidates(candidates, found);
}

static void AddADLFunctionCandidates(String* name, Vector* actuals,
                                     Vector* candidates) {
  if (!CompilerIsCXX() || actuals == NULL) {
    return;
  }
  Vector namespaces;
  VectorInit(&namespaces);
  for (size_t i = 0; i < actuals->length; i++) {
    ASTNode* actual = actuals->value.p[i];
    if (actual != NULL) {
      ADLCollectNamespacesForType(actual->type, &namespaces, 0);
    }
  }
  for (size_t i = 0; i < namespaces.length; i++) {
    AddNamedFunctionCandidates(name, namespaces.value.p[i], candidates);
  }
  VectorDestruct(&namespaces);
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

  StructMember* member =
      FindStructMemberByName(node->sub->type->info.struct_info, op_name);
  if (member != NULL && member->is_member_function) {
    ASTNode* receiver = ASTNodeMove(node->sub);
    ASTNode* call = NewOperatorMemberCall(receiver, op_name, NULL,
                                          node->base.location);
    return ReplaceUnaryWithCall(node, call);
  }

  String name;
  StringInit(&name, op_name);
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, node->sub);
  Symbol* function = ResolveFreeFunctionWithADL(&name, &actuals,
                                                /*diagnose_ambiguous=*/true);
  VectorDestruct(&actuals);
  if (function != NULL && TypeIsFunction(function->type)) {
    ASTNode* actual = ASTNodeMove(node->sub);
    ASTNode* call = NewOperatorFreeCall(function, actual, NULL,
                                        node->base.location);
    StringDestruct(&name);
    return ReplaceUnaryWithCall(node, call);
  }
  StringDestruct(&name);
  return NULL;
}

static const char* IncDecOperatorFunctionName(ASTOpcode op) {
  switch (op) {
    case AST_OP(preinc):
    case AST_OP(postinc):
      return "operator++";
    case AST_OP(predec):
    case AST_OP(postdec):
      return "operator--";
    default:
      return NULL;
  }
}

static bool IsPostIncDec(ASTOpcode op) {
  return op == AST_OP(postinc) || op == AST_OP(postdec);
}

static ASTNode* NewPostfixDummyArgument(SourceLocation location) {
  return NewIntConstantASTNode(0, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                               location);
}

static ASTNode* TryAnalyzeOverloadedIncDecOperator(UnaryASTNode* node) {
  const char* op_name = IncDecOperatorFunctionName(node->base.op);
  if (!CompilerIsCXX() || op_name == NULL) {
    return NULL;
  }

  node->sub = AnalyzeExpression(node->sub);
  if (!TypeIsStructOrUnion(node->sub->type)) {
    return NULL;
  }

  bool postfix = IsPostIncDec(node->base.op);
  StructMember* member =
      FindStructMemberByName(node->sub->type->info.struct_info, op_name);
  if (member != NULL && member->is_member_function) {
    Vector* actuals = NewVector();
    if (postfix) {
      VectorAppend(actuals, NewPostfixDummyArgument(node->base.location));
    }
    ASTNode* call =
        NewOperatorMemberCall(ASTNodeMove(node->sub), op_name, actuals,
                              node->base.location);
    return ReplaceUnaryWithCall(node, call);
  }

  String name;
  StringInit(&name, op_name);
  Vector lookup_actuals;
  VectorInit(&lookup_actuals);
  VectorAppend(&lookup_actuals, node->sub);
  if (postfix) {
    VectorAppend(&lookup_actuals, NewPostfixDummyArgument(node->base.location));
  }
  Symbol* function = ResolveFreeFunctionWithADL(&name, &lookup_actuals,
                                                /*diagnose_ambiguous=*/true);
  VectorDestruct(&lookup_actuals);
  if (function != NULL && TypeIsFunction(function->type)) {
    Vector* actuals = NULL;
    if (postfix) {
      actuals = NewVector();
      VectorAppend(actuals, NewPostfixDummyArgument(node->base.location));
    }
    ASTNode* call = NewOperatorFreeCall(function, ASTNodeMove(node->sub),
                                        actuals, node->base.location);
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

  if (TypeIsStructOrUnion(node->left->type)) {
    StructMember* member =
        FindStructMemberByName(node->left->type->info.struct_info, op_name);
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
      return ReplaceBinaryWithCall(node, call);
    }
  }

  String name;
  StringInit(&name, op_name);
  Vector lookup_actuals;
  VectorInit(&lookup_actuals);
  VectorAppend(&lookup_actuals, node->left);
  VectorAppend(&lookup_actuals, node->right);
  Symbol* function = ResolveFreeFunctionWithADL(&name, &lookup_actuals,
                                                /*diagnose_ambiguous=*/true);
  VectorDestruct(&lookup_actuals);
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
static ASTNode* AnalyzeShift(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
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
  return (ASTNode*)node;
}

// Bitwise operators need integers.
static ASTNode* AnalyzeBitwiseOperator(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
  if (!TypeIsIntegral(node->left->type) || !TypeIsIntegral(node->right->type)) {
    SemanticError((ASTNode*)node, "Bitwise operator needs integral types");
  } else {
    InsertNumericConversions(node, true);
  }
  return (ASTNode*)node;
}

static bool IsZeroIntegerConstant(ASTNode* node) {
  return node != NULL &&
         (node->op == AST_OP(number) || node->op == AST_OP(charconst)) &&
         ((ConstantASTNode*)node)->value.ivalue == 0;
}

// C++20 rewritten comparison candidates ([over.match.oper]): when a relational
// or `!=` operator has a class operand with no directly-usable overload, rewrite
// it in terms of `operator<=>` / `operator==`:
//   a @ b   (@ in < > <= >=)  ->  (a <=> b) @ 0
//   a != b                    ->  !(a == b)
// `==` itself is left to operator== (no <=> rewrite), matching the standard.
// (Reversed candidates such as `0 @ (b <=> a)` are not synthesized; symmetric
// member/free `operator<=>` and `operator==` cover the common cases.)
static ASTNode* TryRewriteComparisonOperator(BinaryASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL || node->right == NULL) {
    return NULL;
  }
  if (!TypeIsStructOrUnion(node->left->type) &&
      !TypeIsStructOrUnion(node->right->type)) {
    return NULL;
  }
  ASTOpcode op = node->base.op;
  SourceLocation loc = node->base.location;

  if (op == AST_OP(noteq)) {
    ASTNode* left = ASTNodeMove(node->left);
    ASTNode* right = ASTNodeMove(node->right);
    ASTNode* eq = NewBinaryASTNode(AST_OP(equal), NULL, loc, left, right);
    ASTNode* negated = NewUnaryASTNode(AST_OP(not), NULL, loc, eq);
    return ReplaceBinaryWithCall(node, negated);
  }
  if (op == AST_OP(less) || op == AST_OP(greater) || op == AST_OP(lesseq) ||
      op == AST_OP(greatereq)) {
    ASTNode* left = ASTNodeMove(node->left);
    ASTNode* right = ASTNodeMove(node->right);
    ASTNode* cmp = NewBinaryASTNode(AST_OP(spaceship), NULL, loc, left, right);
    ASTNode* zero = NewIntConstantASTNode(
        0, NewTypeRecordWithSize(kTypeInt, kQualPlain), loc);
    ASTNode* rewritten = NewBinaryASTNode(op, NULL, loc, cmp, zero);
    return ReplaceBinaryWithCall(node, rewritten);
  }
  return NULL;
}

static ASTNode* AnalyzeComparisonOperator(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  ASTNode* rewritten = TryRewriteComparisonOperator(node);
  if (rewritten != NULL) {
    return rewritten;
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
  if (TypeIsIntegral(node->left->type) && TypeIsIntegral(node->right->type) &&
      TypeIsUnsigned(node->left->type) != TypeIsUnsigned(node->right->type) &&
      !IsZeroIntegerConstant(node->left) && !IsZeroIntegerConstant(node->right)) {
    SemanticWarning((ASTNode*)node, "sign-compare",
                    "comparison of integers of different signs");
  }
  InsertNumericConversions(node, true);

  // Comparison operators produce boolean values.
  ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeBool, kQualPlain));
  return (ASTNode*)node;
}

// C++20 three-way comparison `a <=> b`.  For class operands this resolves a
// user-declared/defaulted operator<=>; for scalar operands it yields a value of
// the appropriate comparison-category type from <compare>.
static ASTNode* AnalyzeThreeWayComparison(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);
  // Apply the usual arithmetic / pointer conversions so both operands end up
  // with a common type that the cmp3way IR op can compare directly.
  InsertNumericConversions(node, true);
  // Built-in operands compare with strong ordering, except floating-point
  // operands (which may be unordered) that compare with partial ordering.
  const char* category_name = "strong_ordering";
  if (TypeIsFloatingPoint(node->left->type) ||
      TypeIsFloatingPoint(node->right->type)) {
    category_name = "partial_ordering";
  }
  TypeRecord* category_type = TypeFindCXXComparisonCategory(category_name);
  if (category_type == NULL) {
    SemanticError((ASTNode*)node,
                  "include <compare> to use the three-way comparison operator");
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }
  // The canonical tag type stores its size in struct_info; codegen reads
  // TypeRecord::size directly (e.g. when laying out the result temporary), so
  // hand the node a copy with the size materialized.
  ASTNodeSetType((ASTNode*)node,
                 TypeRecordCalculateSize(TypeRecordCopy(category_type)));
  return (ASTNode*)node;
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
  SemanticConvertType(node->left, NewTypeRecordWithSize(kTypeBool, kQualPlain),
                      kConvertContextualBool);
  if (!TypeIsScalar(node->left->type)) {
    SemanticError((ASTNode*)node, "Condition for ? operator must be scalar");
    ASTNodeSetType((ASTNode*)node, node->left->type);
    return;
  }
  BinaryASTNode* colon = (BinaryASTNode*)node->right;
  colon->left = AnalyzeExpression(colon->left);
  colon->right = AnalyzeExpression(colon->right);
  if (colon->left->op == AST_OP(throw) && colon->right->op == AST_OP(throw)) {
    ASTNodeSetType((ASTNode*)colon, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    return;
  }
  if (colon->left->op == AST_OP(throw)) {
    ASTNodeSetType((ASTNode*)colon, colon->right->type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    node->base.value_category = colon->right->value_category;
    return;
  }
  if (colon->right->op == AST_OP(throw)) {
    ASTNodeSetType((ASTNode*)colon, colon->left->type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    node->base.value_category = colon->left->value_category;
    return;
  }
  if (TypeIsVoid(colon->left->type) || TypeIsVoid(colon->right->type)) {
    if (!TypeIsVoid(colon->left->type) || !TypeIsVoid(colon->right->type)) {
      SemanticError((ASTNode*)node,
                    "Conditional operator with void expression requires both arms to be void");
    }
    ASTNodeSetType((ASTNode*)colon, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    return;
  }
  if (TryAnalyzeConditionalFunctionPointer(node, colon)) {
    return;
  }
  if (CompilerIsCXX() &&
      colon->left->value_category == colon->right->value_category &&
      colon->left->value_category != kValueCategoryPrvalue &&
      TypeEqual(colon->left->type, colon->right->type)) {
    ASTNodeSetType((ASTNode*)colon, colon->left->type);
    ASTNodeSetType((ASTNode*)node, colon->base.type);
    colon->base.value_category = colon->left->value_category;
    node->base.value_category = colon->left->value_category;
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

static ASTNode* MaterializeCXXByValueClassArgument(ASTNode* actual,
                                                   TypeRecord* formal_type);

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

static bool TypeEqualIgnoringQualifiers(TypeRecord* left, TypeRecord* right);

static void ConversionOperatorName(TypeRecord* type, String* name) {
  String type_name;
  StringInit(&type_name, "");
  TypeRecordToString(type, &type_name);
  StringInit(name, "operator ");
  for (size_t i = 0; i < type_name.length; i++) {
    char ch = type_name.value[i];
    if (ch == '*') {
      StringAppend(name, " pointer");
    } else if (ch == '&') {
      if (i + 1 < type_name.length && type_name.value[i + 1] == '&') {
        StringAppend(name, " rvalue_reference");
        i++;
      } else {
        StringAppend(name, " reference");
      }
    } else {
      StringAppendChar(name, ch);
    }
  }
  while (name->length > 0 && name->value[name->length - 1] == ' ') {
    name->value[name->length - 1] = '\0';
    name->length--;
  }
  StringDestruct(&type_name);
}

static bool ClassHasConversionOperatorTo(ASTNode* actual, TypeRecord* target) {
  if (!CompilerIsCXX() || actual == NULL || actual->type == NULL ||
      !TypeIsStructOrUnion(actual->type)) {
    return false;
  }
  String name;
  ConversionOperatorName(target, &name);
  StructMember* member = FindStructMember(actual->type->info.struct_info, &name);
  StringDestruct(&name);
  while (member != NULL) {
    if (member->is_member_function && member->symbol != NULL &&
        TypeIsFunction(member->symbol->type) &&
        TypeEqual(member->symbol->type->next, target)) {
      return true;
    }
    member = member->overload_next;
  }
  return false;
}

static TypeRecord* ReferenceConversionTarget(ASTNode* actual,
                                             TypeRecord* reference_type) {
  if (!CompilerIsCXX() || actual == NULL || actual->type == NULL ||
      !TypeIsStructOrUnion(actual->type) ||
      TypeEqualIgnoringQualifiers(actual->type, reference_type->next) ||
      TypeIsDerivedFrom(actual->type, reference_type->next) ||
      !ClassHasConversionOperatorTo(actual, reference_type)) {
    return reference_type->next;
  }
  return reference_type;
}

static ASTNode* NewCXXInitializerListBackingArray(TypeRecord* element_type,
                                                  BracedInitializerASTNode* braced,
                                                  SourceLocation location) {
  TypeRecord* array_type =
      NewBasicArrayTypeRecord(kQualPlain, (int)braced->initializers->length,
                              /*is_flexible=*/false);
  TypeRecordChain(array_type, TypeRecordCopy(element_type));
  TypeRecordCalculateSize(array_type);

  Vector* array_initializers = NewVector();
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    VectorAppend(array_initializers, ASTNodeMove(initializer));
  }
  ASTNode* array_init =
      NewBracedInitializerASTNode(array_initializers, array_type, location);
  Symbol* array_symbol = SyntaxNewTemporary(&compiler->syntax, array_type);
  array_symbol->location = location;
  ASTNode* array_id = NewIdentifierASTNode(array_symbol, location);
  array_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  return NewCompoundLiteralASTNode(array_id, location, array_init);
}

static ASTNode* LowerCXXInitializerListBracedInit(TypeRecord* target_type,
                                                  BracedInitializerASTNode* braced,
                                                  SourceLocation location) {
  TypeRecord* element_type = TypeCXXInitializerListElement(target_type);
  if (element_type == NULL) {
    return (ASTNode*)braced;
  }

  ASTNode* backing_array =
      NewCXXInitializerListBackingArray(element_type, braced, location);
  Vector* list_initializers = NewVector();
  VectorAppend(list_initializers,
               NewExpressionInitializerASTNode(backing_array, location));
  VectorAppend(list_initializers,
               NewExpressionInitializerASTNode(
                   NewIntConstantASTNode((int64_t)braced->initializers->length,
                                         NewSizeTypeRecord(), location),
                   location));
  return NewBracedInitializerASTNode(list_initializers, target_type, location);
}

static bool CXXInitializerListBracedInitIsViable(ASTNode* actual,
                                                 TypeRecord* formal_type) {
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(formal_type)) {
    return false;
  }
  TypeRecord* element_type = TypeCXXInitializerListElement(formal_type);
  if (element_type == NULL) {
    return false;
  }
  BracedInitializerASTNode* braced = (BracedInitializerASTNode*)actual;
  for (size_t i = 0; i < braced->initializers->length; i++) {
    ASTNode* initializer = braced->initializers->value.p[i];
    if (initializer == NULL || initializer->op != AST_OP(expr_init)) {
      return false;
    }
    ExpressionInitializerASTNode* expr_init =
        (ExpressionInitializerASTNode*)initializer;
    expr_init->expr = AnalyzeExpression(expr_init->expr);
    if (OverloadBaseConversionRank(expr_init->expr->type, element_type) < 0) {
      return false;
    }
  }
  return true;
}

static ASTNode* ConvertCXXInitializerListArgument(ASTNode* actual,
                                                  TypeRecord* formal_type) {
  TypeRecord* target = TypeIsReference(formal_type) ? formal_type->next
                                                    : formal_type;
  if (actual == NULL || actual->op != AST_OP(braced_init) ||
      !TypeIsCXXInitializerList(target)) {
    return actual;
  }
  ASTNode* initializer = LowerCXXInitializerListBracedInit(
      target, (BracedInitializerASTNode*)actual, actual->location);
  Symbol* list_symbol = SyntaxNewTemporary(&compiler->syntax,
                                           TypeRecordCopy(target));
  list_symbol->location = actual->location;
  ASTNode* list_id = NewIdentifierASTNode(list_symbol, actual->location);
  list_id->flags |= kASTNeedAddress | kASTIsDeclaration;
  ASTNode* lowered =
      NewCompoundLiteralASTNode(list_id, actual->location, initializer);
  return AnalyzeExpression(lowered);
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
        NormalConversion(e->expr,
                         ReferenceConversionTarget(e->expr, reference_type));
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
        bool constructor_call = false;
        if (e->expr->op == AST_OP(call)) {
          VectorASTNode* call = (VectorASTNode*)e->expr;
          if (call->left != NULL && call->left->op == AST_OP(identifier)) {
            Symbol* callee = ((IdentifierASTNode*)call->left)->symbol;
            constructor_call = callee != NULL && TypeIsFunction(callee->type) &&
                               callee->type->info.function.is_constructor;
          }
        }
        bool cxx_return_elision_initializer =
            CompilerIsCXX() && e->expr->op == AST_OP(call) &&
            e->expr->value_category != kValueCategoryXvalue &&
            !constructor_call &&
            TypeIsStructOrUnion(id_node->base.type) &&
            TypeEqual(e->expr->type, id_node->base.type);
        if (!cxx_return_elision_initializer &&
            e->expr->value_category == kValueCategoryXvalue &&
            TypeIsStructOrUnion(id_node->base.type) &&
            TypeIsStructOrUnion(e->expr->type) &&
            TypeEqualIgnoringQualifiers(e->expr->type, id_node->base.type)) {
          ASTNode* materialized =
              MaterializeCXXByValueClassArgument(e->expr, id_node->base.type);
          if (materialized != e->expr) {
            ASTNodeReplaceChild((ASTNode*)e, 0, materialized, false);
            e->expr = materialized;
          }
        }
        if (!cxx_return_elision_initializer) {
          NormalConversion(e->expr, id_node->base.type);
        }
      }
      break;
    }
    case AST_OP(braced_init): {
      BracedInitializerASTNode* braced = (BracedInitializerASTNode*)init;
      if (!is_reference_init && TypeIsCXXInitializerList(id_node->base.type) &&
          !TypeIsCXXInitializerList(init->type)) {
        init = LowerCXXInitializerListBracedInit(id_node->base.type, braced,
                                                init->location);
        break;
      }
      if (!is_reference_init && TypeIsScalar(id_node->base.type) &&
          braced->initializers->length == 1) {
        ASTNode* initializer = braced->initializers->value.p[0];
        if (initializer->op == AST_OP(designated_init)) {
          DesignatedInitializerASTNode* designated =
              (DesignatedInitializerASTNode*)initializer;
          NormalConversion(designated->init, id_node->base.type);
        }
      }
      break;
    }
    default:
      break;
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
  if (TypeIsConst(id_node->symbol->type) ||
      id_node->symbol->flags.is_constexpr ||
      id_node->symbol->flags.is_constinit) {
    EvaluateConstantForSymbol(id_node->symbol, init);
  }
  if ((id_node->symbol->flags.is_constexpr ||
       id_node->symbol->flags.is_constinit) &&
      !id_node->symbol->flags.value_set) {
    SemanticError(init,
                  id_node->symbol->flags.is_constinit
                      ? "constinit variable initializer is not a constant expression"
                      : "constexpr variable initializer is not a constant expression");
  }
  ASTNode* object_init = ConstexprObjectInitializerForSymbol(
      id_node->symbol, init->location);
  if (object_init != NULL) {
    init = object_init;
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
  if (BinaryOperatorFunctionName(node->base.op) != NULL) {
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
static ASTNode* AnalyzeIncDec(UnaryASTNode* node) {
  ASTNode* overloaded = TryAnalyzeOverloadedIncDecOperator(node);
  if (overloaded != NULL) {
    return overloaded;
  }
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
  return (ASTNode*)node;
}

// Array subscripting operator.
static ASTNode* AnalyzeArraySubscript(BinaryASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  node->right = AnalyzeExpression(node->right);
  if (CompilerIsCXX() && TypeIsStructOrUnion(node->left->type)) {
    StructMember* member =
        FindStructMemberByName(node->left->type->info.struct_info,
                               "operator[]");
    if (member != NULL && member->is_member_function) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, ASTNodeMove(node->right));
      ASTNode* call = NewOperatorMemberCall(ASTNodeMove(node->left),
                                            "operator[]", actuals,
                                            node->base.location);
      return ReplaceBinaryWithCall(node, call);
    }
    String name;
    StringInit(&name, "operator[]");
    Vector lookup_actuals;
    VectorInit(&lookup_actuals);
    VectorAppend(&lookup_actuals, node->left);
    VectorAppend(&lookup_actuals, node->right);
    Symbol* function = ResolveFreeFunctionWithADL(&name, &lookup_actuals,
                                                  /*diagnose_ambiguous=*/true);
    VectorDestruct(&lookup_actuals);
    if (function != NULL && TypeIsFunction(function->type)) {
      Vector* actuals = NewVector();
      VectorAppend(actuals, ASTNodeMove(node->right));
      ASTNode* call = NewOperatorFreeCall(function, ASTNodeMove(node->left),
                                          actuals, node->base.location);
      StringDestruct(&name);
      return ReplaceBinaryWithCall(node, call);
    }
    StringDestruct(&name);
  }
  SemanticConvertType(node->right,
                      NewTypeRecordWithSize(kTypeInt, kQualPlain), kConvertNormal);
  if (node->right != NULL && !TypeIsIntegral(node->right->type)) {
    SemanticError(node->right, "Subscripts must be integral types");
  }
  if (node->left != NULL && !TypeIsPointerOrArray(node->left->type)) {
    SemanticError(node->left, "Can only subscript arrays and pointers");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return (ASTNode*)node;
  }

  // Dereference the array type.
  TypeRecord* subtype = node->left->type->next;
  ASTNodeSetType((ASTNode*)node, subtype);
  node->base.value_category = kValueCategoryLvalue;
  return (ASTNode*)node;
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

static void CheckDeletedFunctionUse(Symbol* function, ASTNode* use) {
  if (!CompilerIsCXX() || function == NULL || function->type == NULL ||
      !TypeIsFunction(function->type) ||
      !function->type->info.function.is_deleted) {
    return;
  }
  String function_name;
  StringInit(&function_name, NULL);
  SymbolFunctionDiagnosticName(function, &function_name);
  if (function->type->info.function.is_implicitly_deleted) {
    SemanticError(use, "Use of implicitly deleted function %s",
                  function_name.value);
  } else {
    SemanticError(use, "Use of deleted function %s", function_name.value);
  }
  StringDestruct(&function_name);
}

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

static bool FunctionIsSourceLocationCurrent(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.symbol == NULL ||
      !StringEqual(&func->info.function.symbol->name, "current")) {
    return false;
  }
  Struct* owner = func->info.function.cxx_member_owner;
  return owner != NULL && owner->tag_name != NULL &&
         StringEqual(owner->tag_name, "source_location");
}

static ASTNode* NewSourceLocationDefaultArgument(Symbol* formal,
                                                 SourceLocation location) {
  if (formal == NULL) {
    return NULL;
  }
  ASTOpcode opcode = AST_OP(bad);
  if (StringEqual(&formal->name, "line")) {
    opcode = AST_OP(builtin_source_line);
  } else if (StringEqual(&formal->name, "column")) {
    opcode = AST_OP(builtin_source_column);
  } else if (StringEqual(&formal->name, "file")) {
    opcode = AST_OP(builtin_source_file);
  } else if (StringEqual(&formal->name, "function")) {
    opcode = AST_OP(builtin_source_pretty_function);
  }
  if (opcode == AST_OP(bad)) {
    return NULL;
  }
  return NewVectorASTNode(opcode, NULL, location,
                          NewRawIdentifierASTNode(NULL, location), NewVector());
}

static bool AppendDefaultCallArguments(VectorASTNode* call, TypeRecord* func) {
  if (call == NULL || call->children == NULL || func == NULL ||
      !TypeIsFunction(func)) {
    return true;
  }
  size_t num_actual_args = call->children->length;
  size_t num_formal_args = func->info.function.prototype.length;
  if (num_actual_args >= num_formal_args) {
    return true;
  }
  for (size_t i = num_actual_args; i < num_formal_args; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal == NULL || formal->default_argument == NULL) {
      return false;
    }
  }
  for (size_t i = num_actual_args; i < num_formal_args; i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    ASTNode* default_arg =
        FunctionIsSourceLocationCurrent(func)
            ? NewSourceLocationDefaultArgument(formal, call->base.location)
            : NULL;
    if (default_arg == NULL) {
      default_arg =
          ASTNodeClone(formal->default_argument, IdentityCloneNode, NULL, NULL);
    }
    default_arg->flags |= kASTDefaultArgument;
    default_arg->location = call->base.location;
    default_arg = AnalyzeExpression(default_arg);
    default_arg->parent = (ASTNode*)call;
    default_arg->child_id = (int)i + 1;
    VectorAppend(call->children, default_arg);
  }
  return true;
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

static ASTNode* NewAnalyzedBuiltinAddressOf(ASTNode* sub,
                                            SourceLocation location) {
  TypeRecord* pointer_type = NewPointerTo(kQualPlain, sub->type);
  ASTNode* address =
      NewUnaryASTNode(AST_OP(address), pointer_type, location, sub);
  ASTNodeSetType(address, pointer_type);
  sub->flags |= kASTNeedAddress;
  address->flags |= kASTAnalyzed;
  return address;
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
    receiver_clone = NewAnalyzedBuiltinAddressOf(receiver_clone, location);
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
  if (!member->is_member_function &&
      (member->symbol == NULL || member->symbol->type == NULL ||
       !TypeIsFunction(member->symbol->type))) {
    return false;
  }

  member = ResolveMemberFunctionOverload(member, node, member_access);
  member_node->member = member;
  if (member != NULL) {
    CheckDeletedFunctionUse(member->symbol, (ASTNode*)node);
  }

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
  bool use_virtual_dispatch =
      member->symbol->type->info.function.is_virtual &&
      !member->is_static && !CurrentFunctionIsCXXCtorOrDtor();
  bool polymorphic_special_member =
      (member->symbol->type->info.function.is_constructor ||
       member->symbol->type->info.function.is_destructor) &&
      owner != NULL && owner->virtual_members.length > 0;
  if (!member->is_static) {
    if (!member->symbol->type->info.function.is_const_member &&
        !member->symbol->type->info.function.is_constructor &&
        !member->symbol->type->info.function.is_destructor &&
        MemberReceiverIsConst(member_access)) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticSuffix(member->symbol, &function_name);
      SemanticError((ASTNode*)member_access,
                    "Cannot call non-const member function %s on const object%s",
                    member->symbol->name.value, function_name.value);
      StringDestruct(&function_name);
    }
    receiver = ASTNodeMove(member_access->left);
    if (member_access->base.op == AST_OP(dot)) {
      receiver = NewAnalyzedBuiltinAddressOf(receiver, receiver->location);
    }
    int this_adjustment = member_node->byte_offset;
    if (use_virtual_dispatch && member->cxx_vcall_offset != 0) {
      this_adjustment = member->cxx_vcall_offset;
    }
    if (!polymorphic_special_member && this_adjustment != 0) {
      TypeRecord* receiver_type = receiver->type;
      if (member->symbol->type->info.function.prototype.length > 0) {
        Symbol* this_arg =
            member->symbol->type->info.function.prototype.value.p[0];
        receiver_type = this_arg->type;
      }
      ASTNode* offset_node = NewIntConstantASTNode(
          this_adjustment,
          NewTypeRecordWithSize(kTypeInt, kQualPlain),
          receiver->location);
      receiver = NewBinaryASTNode(AST_OP(plus), receiver_type,
                                  receiver->location, receiver, offset_node);
      ASTNodeSetType(receiver, receiver_type);
      receiver->flags |= kASTAnalyzed;
    }
  }

  ASTNode* old_left = node->left;
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

// Returns true when the function currently being analyzed has been granted
// friendship by class `owner` (via a 'friend class' or 'friend function'
// declaration), and may therefore access its private and protected members.
static bool CurrentFunctionIsFriendOf(Struct* owner) {
  if (owner == NULL) {
    return false;
  }
  TypeRecord* current = compiler->current_function;
  if (current == NULL || !TypeIsFunction(current)) {
    return false;
  }
  // 'friend class C;': any member function of C is a friend.
  Struct* current_owner = current->info.function.cxx_member_owner;
  if (current_owner != NULL) {
    for (size_t i = 0; i < owner->friend_classes.length; i++) {
      if (owner->friend_classes.value.p[i] == current_owner) {
        return true;
      }
    }
  }
  // 'friend <function>;': match the befriended declaration either by symbol
  // identity or, since a friend declaration and the later definition may be
  // separate symbols, by name and signature.
  Symbol* current_symbol = current->info.function.symbol;
  for (size_t i = 0; i < owner->friend_functions.length; i++) {
    Symbol* friend_symbol = owner->friend_functions.value.p[i];
    if (friend_symbol == NULL) {
      continue;
    }
    if (friend_symbol == current_symbol) {
      return true;
    }
    if (current_symbol != NULL &&
        StringEqualString(&friend_symbol->name, &current_symbol->name) &&
        TypeEqual(friend_symbol->type, current_symbol->type)) {
      return true;
    }
  }
  return false;
}

static bool CurrentFunctionCanAccessMember(Struct* lookup_context,
                                           Struct* owner,
                                           CXXAccess original_access,
                                           CXXAccess effective_access) {
  if (effective_access == kAccessPublic) {
    return true;
  }
  Struct* current_owner = NULL;
  if (compiler->current_function != NULL &&
      TypeIsFunction(compiler->current_function)) {
    current_owner = compiler->current_function->info.function.cxx_member_owner;
  } else if (compiler->current_class_access_context != NULL) {
    // A static data member initializer is in the scope of its class and may
    // name the class's private and protected members.
    current_owner = compiler->current_class_access_context;
  } else {
    return false;
  }
  if (current_owner == owner) {
    return true;
  }
  if (CurrentFunctionIsFriendOf(owner) ||
      (lookup_context != owner && CurrentFunctionIsFriendOf(lookup_context))) {
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

static TypeRecord* NewStructTypeForAdjustment(Struct* str) {
  TypeRecord* type = NewTypeRecord(kTypeStruct, kQualPlain);
  type->info.struct_info = str;
  TypeRecordCalculateSize(type);
  return type;
}

static ASTNode* NewVirtualBaseOffsetLoad(ASTNode* receiver,
                                         int vbtable_index,
                                         SourceLocation location) {
  ASTNode* receiver_clone = CloneReceiverForVirtualLookup(receiver);
  ASTNode* vbptr_name =
      NewStringConstantASTNode(NewString("__vbptr"), NULL, location);
  ASTNode* vbptr =
      NewBinaryASTNode(AST_OP(arrow), NULL, location, receiver_clone,
                       vbptr_name);
  ASTNode* index =
      NewIntConstantASTNode(vbtable_index,
                            NewTypeRecordWithSize(kTypeInt, kQualPlain),
                            location);
  ASTNode* load =
      NewBinaryASTNode(AST_OP(subscript), NULL, location, vbptr, index);
  return AnalyzeExpression(load);
}

static ASTNode* AddStaticOffsetToRuntimeOffset(ASTNode* offset, int byte_offset,
                                               SourceLocation location) {
  if (byte_offset == 0) {
    return offset;
  }
  TypeRecord* int_type = NewTypeRecordWithSize(kTypeInt, kQualPlain);
  ASTNode* tail = NewIntConstantASTNode(byte_offset, int_type, location);
  ASTNode* combined =
      NewBinaryASTNode(AST_OP(plus), int_type, location, offset, tail);
  combined->flags |= kASTAnalyzed;
  return combined;
}

static void ApplyVirtualBaseAdjustmentToMemberReference(BinaryASTNode* node,
                                                        Struct* from,
                                                        Struct* owner,
                                                        int* member_offset) {
  if (node == NULL || from == NULL || owner == NULL || from == owner) {
    return;
  }
  TypeRecord* from_type = NewStructTypeForAdjustment(from);
  TypeRecord* owner_type = NewStructTypeForAdjustment(owner);
  CXXBaseAdjustment adjustment;
  bool found = TypeBaseAdjustment(from_type, owner_type, /*public_only=*/true,
                                  &adjustment);
  TypeRecordDelete(from_type);
  TypeRecordDelete(owner_type);
  if (!found || adjustment.kind != kCXXBaseAdjustmentVirtual) {
    return;
  }

  ASTNode* receiver = ASTNodeMove(node->left);
  if (node->base.op == AST_OP(dot)) {
    receiver = NewAnalyzedBuiltinAddressOf(receiver, receiver->location);
  }
  ASTNode* offset =
      NewVirtualBaseOffsetLoad(receiver, adjustment.vbtable_index,
                               node->base.location);
  offset = AddStaticOffsetToRuntimeOffset(offset, adjustment.byte_offset,
                                          node->base.location);
  TypeRecord* adjusted_type =
      NewPointerTo(kQualPlain, NewStructTypeForAdjustment(owner));
  ASTNode* adjusted =
      NewBinaryASTNode(AST_OP(plus), adjusted_type, node->base.location,
                       receiver, offset);
  adjusted->flags |= kASTAnalyzed;
  node->base.op = AST_OP(arrow);
  node->left = adjusted;
  node->left->parent = (ASTNode*)node;
  node->left->child_id = 0;
  if (member_offset != NULL) {
    *member_offset = -1;
  }
}

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

// Advance to the next non-static data member at or after index *i, returning it
// (and leaving *i at its position) or NULL when none remain.  Member functions
// and static members do not contribute to an object's layout and are skipped.
static StructMember* NextDataMember(Struct* s, size_t* i) {
  for (; *i < s->members.length; (*i)++) {
    StructMember* m = s->members.value.p[*i];
    if (m != NULL && m->symbol != NULL && !m->is_static &&
        !m->is_member_function) {
      return m;
    }
  }
  return NULL;
}

// Two distinct class TypeRecords can still denote the same type when the
// compiler has produced separate struct records for one specialization (e.g.
// unique_ptr<T[]>, whose records can differ even in member-function count).
// They are treated as equivalent when they have the same data-member layout:
// the same non-static data members, in order, with recursively equivalent
// types.  Member functions are ignored.  Classes with no data members are not
// matched here so unrelated empty structs are still rejected.
static bool ClassTypesLayoutEquivalent(TypeRecord* a, TypeRecord* b) {
  if (!TypeIsStructOrUnion(a) || !TypeIsStructOrUnion(b) ||
      a->info.struct_info == NULL || b->info.struct_info == NULL) {
    return false;
  }
  Struct* sa = a->info.struct_info;
  Struct* sb = b->info.struct_info;
  if (sa == sb) {
    return true;
  }
  if (sa->is_union != sb->is_union) {
    return false;
  }
  size_t ia = 0;
  size_t ib = 0;
  bool matched_any = false;
  for (;;) {
    StructMember* ma = NextDataMember(sa, &ia);
    StructMember* mb = NextDataMember(sb, &ib);
    if (ma == NULL || mb == NULL) {
      if (ma != mb) {
        return false;
      }
      break;
    }
    if (!StringEqualString(&ma->symbol->name, &mb->symbol->name)) {
      return false;
    }
    TypeRecord* ta = ma->symbol->type;
    TypeRecord* tb = mb->symbol->type;
    if (!TypeEqual(ta, tb) &&
        !(TypeIsStructOrUnion(ta) && TypeIsStructOrUnion(tb) &&
          ClassTypesLayoutEquivalent(ta, tb))) {
      return false;
    }
    matched_any = true;
    ia++;
    ib++;
  }
  return matched_any;
}

static int OverloadBaseConversionRank(TypeRecord* actual, TypeRecord* target) {
  if (TypeEqual(actual, target) || TypeEqualIgnoringQualifiers(actual, target)) {
    return 0;
  }
  if (TypeIsStructOrUnion(actual) || TypeIsStructOrUnion(target)) {
    // Two distinct class types convert only via a derived-to-base relationship
    // or as separate records for the same specialization.  Unrelated classes
    // (the common operator-overload mistake, e.g. `a + b` for unrelated `a` and
    // `b`) are rejected here instead of being treated as differing only in sign
    // by the scalar comparison below.
    if (TypeIsStructOrUnion(actual) && TypeIsStructOrUnion(target)) {
      if (TypeIsDerivedFrom(actual, target)) {
        return 2;
      }
      if (ClassTypesLayoutEquivalent(actual, target)) {
        return 0;
      }
    }
    return -1;
  }
  if (TypeEqualIgnoringSign(actual, target)) {
    return 1;
  }
  if (TypeIsEnum(actual) && TypeIsEnum(target)) {
    return actual->info.enum_info == target->info.enum_info ? 0 : -1;
  }
  if (TypeIsIntegral(actual) && TypeIsEnum(target)) {
    return 2;
  }
  if (TypeIsEnum(actual) && TypeIsIntegral(target)) {
    return 2;
  }
  if (TypeIsIntegral(actual) && TypeIsIntegral(target)) {
    return 2;
  }
  if (TypeIsPointerOrArray(actual) && TypeIsPointerOrArray(target)) {
    if (CompilerIsCXX() && TypeIsVoidPointer(actual) &&
        TypeIsPointer(target) && !TypeIsVoidPointer(target)) {
      return -1;
    }
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
    if (actual != NULL && actual->op == AST_OP(braced_init)) {
      return CXXInitializerListBracedInitIsViable(actual, target) ? 0 : -1;
    }
    int binding_rank = ReferenceBindingRank(actual, formal_type);
    if (binding_rank < 0) {
      return -1;
    }
    int base_rank = OverloadBaseConversionRank(actual->type, target);
    return base_rank < 0 ? -1 : base_rank * 10 + binding_rank;
  }

  if (actual != NULL && actual->op == AST_OP(braced_init)) {
    return CXXInitializerListBracedInitIsViable(actual, target) ? 0 : -1;
  }
  int base_rank = OverloadBaseConversionRank(actual->type, target);
  if (base_rank >= 0) {
    return base_rank * 10 + 5;
  }
  if (IsZeroIntegerConstant(actual) && TypeIsPointer(target)) {
    return 25;
  }
  // As a last resort consider a user-defined conversion through a converting
  // constructor of a class target.  This ranks worse than any standard
  // conversion sequence above, matching the standard's ordering.
  if (CompilerIsCXX() && !g_suppress_user_defined_conversion_rank &&
      TypeIsStructOrUnion(target) &&
      FindConvertingConstructorCandidate(target, actual,
                                         /*allow_explicit=*/false) != NULL) {
    return 100;
  }
  return -1;
}

static StructMember* FindConvertingConstructorCandidate(TypeRecord* to,
                                                        ASTNode* from,
                                                        bool allow_explicit) {
  if (!CompilerIsCXX() || to == NULL || from == NULL || from->type == NULL ||
      !TypeIsStructOrUnion(to) || to->info.struct_info == NULL ||
      to->info.struct_info->tag_name == NULL) {
    return NULL;
  }
  // A source of the same class (or a derived class) is handled by copy/move
  // construction and derived-to-base conversions, not by a converting
  // constructor, so leave those to the existing machinery.
  if (TypeIsStructOrUnion(from->type) &&
      (TypeEqualIgnoringQualifiers(from->type, to) ||
       TypeIsDerivedFrom(from->type, to))) {
    return NULL;
  }
  Struct* str = to->info.struct_info;
  StructMember* ctor = FindStructMember(str, str->tag_name);
  StructMember* best = NULL;
  int best_rank = -1;
  bool ambiguous = false;
  for (StructMember* c = ctor; c != NULL; c = c->overload_next) {
    if (!c->is_member_function || c->symbol == NULL ||
        c->symbol->type == NULL || !TypeIsFunction(c->symbol->type)) {
      continue;
    }
    FunctionInfo* fi = &c->symbol->type->info.function;
    if (!fi->is_constructor || fi->is_deleted) {
      continue;
    }
    if (fi->is_explicit && !allow_explicit) {
      continue;
    }
    // Skip compiler-synthesized special members (copy/move/default): they are
    // not converting constructors.
    if (c->symbol->flags.invented) {
      continue;
    }
    // prototype[0] is the implicit object parameter; a converting constructor
    // takes exactly one further argument (any remaining parameters must be
    // defaulted).
    size_t nparams = fi->prototype.length;
    if (nparams < 2) {
      continue;
    }
    bool rest_defaulted = true;
    for (size_t i = 2; i < nparams; i++) {
      Symbol* pi = fi->prototype.value.p[i];
      if (pi == NULL || pi->default_argument == NULL) {
        rest_defaulted = false;
        break;
      }
    }
    if (!rest_defaulted) {
      continue;
    }
    Symbol* param = fi->prototype.value.p[1];
    if (param == NULL || param->type == NULL) {
      continue;
    }
    bool saved = g_suppress_user_defined_conversion_rank;
    g_suppress_user_defined_conversion_rank = true;
    int rank = OverloadConversionRank(from, param->type);
    g_suppress_user_defined_conversion_rank = saved;
    if (rank < 0) {
      continue;
    }
    if (best == NULL || rank < best_rank) {
      best = c;
      best_rank = rank;
      ambiguous = false;
    } else if (rank == best_rank) {
      ambiguous = true;
    }
  }
  return ambiguous ? NULL : best;
}

// Converts `from` to the class type `to` by constructing a temporary through a
// viable converting constructor (`to(from)`), splicing the resulting
// prvalue temporary in place of `from`.  Returns true if such a conversion was
// performed.  `ctx == kConvertCast` additionally allows explicit constructors
// (matching the explicit-conversion semantics of a cast).
bool TryConvertWithConvertingConstructor(ASTNode* from, TypeRecord* to,
                                         ConversionContext ctx) {
  if (!CompilerIsCXX() || from == NULL || to == NULL ||
      !TypeIsStructOrUnion(to)) {
    return false;
  }
  StructMember* ctor = FindConvertingConstructorCandidate(
      to, from, /*allow_explicit=*/ctx == kConvertCast);
  if (ctor == NULL) {
    return false;
  }
  Struct* str = to->info.struct_info;
  SourceLocation location = from->location;
  TypeRecord* type = TypeRecordCopy(to);
  TypeRecordCalculateSize(type);

  ASTNode* parent = from->parent;
  int child_id = from->child_id;
  ASTNode* arg = ASTNodeMove(from);

  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member = NewStringConstantASTNode(NewString(str->tag_name->value),
                                             NULL, location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);
  Vector* actuals = NewVector();
  VectorAppend(actuals, arg);
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* result = NewIdentifierASTNode(temp, location);
  ASTNode* comma = NewBinaryASTNode(AST_OP(comma), type, location,
                                    constructor_call, result);
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, comma, true);
  }
  ASTNode* analyzed = AnalyzeExpression(comma);
  analyzed->value_category = kValueCategoryPrvalue;
  if (parent != NULL) {
    ASTNodeReplaceChild(parent, child_id, analyzed, false);
  }
  return true;
}

static bool CallActualIsPackExpansion(ASTNode* actual) {
  if (actual == NULL) {
    return false;
  }
  if ((actual->flags & kASTPackExpansion) != 0) {
    return true;
  }
  if (actual->op == AST_OP(contents)) {
    return CallActualIsPackExpansion(((UnaryASTNode*)actual)->sub);
  }
  if (actual->op != AST_OP(dot) && actual->op != AST_OP(arrow)) {
    return false;
  }
  BinaryASTNode* member_access = (BinaryASTNode*)actual;
  if (member_access->right == NULL ||
      member_access->right->op != AST_OP(structmember)) {
    return false;
  }
  StructMemberASTNode* member = (StructMemberASTNode*)member_access->right;
  return member->member != NULL && member->member->symbol != NULL &&
         member->member->symbol->flags.is_parameter_pack;
}

static bool CallHasPackExpansionActual(VectorASTNode* node) {
  if (node == NULL || node->children == NULL) {
    return false;
  }
  for (size_t i = 0; i < node->children->length; i++) {
    if (CallActualIsPackExpansion(node->children->value.p[i])) {
      return true;
    }
  }
  return false;
}

static size_t FunctionRequiredArgumentCount(TypeRecord* func,
                                            size_t first_formal_arg) {
  size_t required = 0;
  if (func == NULL || !TypeIsFunction(func) ||
      first_formal_arg > func->info.function.prototype.length) {
    return required;
  }
  for (size_t i = first_formal_arg; i < func->info.function.prototype.length;
       i++) {
    Symbol* formal = func->info.function.prototype.value.p[i];
    if (formal != NULL && formal->default_argument == NULL) {
      required = i - first_formal_arg + 1;
    }
  }
  return required;
}

static int FunctionCallScore(TypeRecord* func, VectorASTNode* node,
                             size_t first_formal_arg) {
  if (!TypeIsFunction(func) || func->info.function.unknown_args) {
    return -1;
  }
  if (CallHasPackExpansionActual(node)) {
    return 0;
  }
  size_t num_actual_args = node->children->length;
  size_t num_formal_args = func->info.function.prototype.length;
  if (first_formal_arg > num_formal_args) {
    return -1;
  }
  size_t num_user_formal_args = num_formal_args - first_formal_arg;
  size_t required_args =
      FunctionRequiredArgumentCount(func, first_formal_arg);
  if (num_actual_args < required_args) {
    return -1;
  }
  if (!func->info.function.varargs &&
      num_actual_args > num_user_formal_args) {
    return -1;
  }

  int score = 0;
  size_t num_checked_args =
      num_actual_args < num_user_formal_args ? num_actual_args
                                             : num_user_formal_args;
  for (size_t i = 0; i < num_checked_args; i++) {
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

static bool SymbolIsOperatorCoAwait(Symbol* symbol) {
  return symbol != NULL && strcmp(symbol->name.value, "operator co_await") == 0;
}

static bool FunctionNameSkipsADL(String* name) {
  return name != NULL &&
         (strcmp(name->value, "operator new") == 0 ||
          strcmp(name->value, "operator new[]") == 0 ||
          strcmp(name->value, "operator delete") == 0 ||
          strcmp(name->value, "operator delete[]") == 0);
}

static bool CoAwaitActualIsLValue(ASTNode* actual) {
  if (ASTNodeIsLValue(actual)) {
    return true;
  }
  return actual != NULL && actual->op == AST_OP(identifier) &&
         actual->type != NULL && !TypeIsFunction(actual->type);
}

static int CoAwaitOperatorActualScore(Symbol* candidate, ASTNode* actual) {
  if (candidate == NULL || candidate->type == NULL ||
      !TypeIsFunction(candidate->type) ||
      candidate->type->info.function.prototype.length != 1) {
    return -1;
  }
  Symbol* formal = candidate->type->info.function.prototype.value.p[0];
  if (actual == NULL || formal == NULL || formal->type == NULL) {
    return -1;
  }
  TypeRecord* target = TypeIsReference(formal->type) ? formal->type->next
                                                     : formal->type;
  if (TypeIsStructOrUnion(actual->type) || TypeIsStructOrUnion(target)) {
    if (!TypeIsStructOrUnion(actual->type) || !TypeIsStructOrUnion(target) ||
        actual->type->info.struct_info != target->info.struct_info) {
      return -1;
    }
  }
  if (TypeIsReference(formal->type)) {
    if (TypeIsConst(actual->type) && !TypeIsConst(target)) {
      return -1;
    }
    if (formal->type->declarator == kDeclRValueReference) {
      return CoAwaitActualIsLValue(actual) ? -1 : 0;
    }
    if (CoAwaitActualIsLValue(actual)) {
      return TypeIsConst(target) ? 1 : 0;
    }
    return TypeIsConst(target) ? 2 : -1;
  }
  return OverloadConversionRank(actual, formal->type);
}

static Symbol* ResolveFreeOperatorCoAwaitForActual(ASTNode* actual,
                                                   bool diagnose_ambiguous) {
  if (!CompilerIsCXX() || actual == NULL || actual->type == NULL) {
    return NULL;
  }
  String name;
  StringInit(&name, "operator co_await");
  Vector actuals;
  VectorInit(&actuals);
  VectorAppend(&actuals, actual);
  Vector candidates;
  VectorInit(&candidates);
  AddNamedFunctionCandidates(&name, compiler->global_namespace, &candidates);
  AddADLFunctionCandidates(&name, &actuals, &candidates);
  Symbol* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  VectorASTNode call = {0};
  call.children = &actuals;
  for (size_t i = 0; i < candidates.length; i++) {
    Symbol* candidate = candidates.value.p[i];
    if (candidate->type == NULL || !TypeIsFunction(candidate->type) ||
        candidate->type->next == NULL) {
      continue;
    }
    int score = OperatorCoAwaitCallScore(candidate, &call);
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
  if (ambiguous && diagnose_ambiguous) {
    SemanticError(actual, "Ambiguous overload for operator co_await");
  }
  VectorDestruct(&candidates);
  VectorDestruct(&actuals);
  StringDestruct(&name);
  return best;
}

static int OperatorCoAwaitCallScore(Symbol* candidate, VectorASTNode* node) {
  if (node == NULL || node->children == NULL || node->children->length != 1) {
    return -1;
  }
  return CoAwaitOperatorActualScore(candidate, node->children->value.p[0]);
}

static int OverloadCallScore(Symbol* candidate, VectorASTNode* node) {
  if (SymbolIsOperatorCoAwait(candidate)) {
    return OperatorCoAwaitCallScore(candidate, node);
  }
  return FunctionCallScore(candidate->type, node, 0);
}

// Appends a human-readable spelling of `type` to `result` for diagnostics.
static void AppendReadableType(TypeRecord* type, String* result) {
  if (type == NULL) {
    StringAppend(result, "<unknown>");
    return;
  }
  TypeRecordToString(type, result);
}

// Returns true if `candidate` is a function template (either an uninstantiated
// template or a specialization produced by instantiating one).
static bool SymbolIsTemplateFunction(Symbol* candidate) {
  if (candidate == NULL) {
    return false;
  }
  if (candidate->flags.is_template) {
    return true;
  }
  return candidate->type != NULL && TypeIsFunction(candidate->type) &&
         candidate->type->info.function.template_origin != NULL;
}

// Fills `reason` with an explanation of why the function type `func` is not a
// viable candidate for the call `node`, where `first_formal_arg` is the index of
// the first formal parameter matched against an explicit argument (1 for a
// non-static member function, whose leading parameter is the implicit object).
// The checks mirror FunctionCallScore so the explanation matches the rejection.
// Returns true if a concrete reason was produced.
static bool DescribeFunctionNonViability(TypeRecord* func, VectorASTNode* node,
                                         size_t first_formal_arg,
                                         String* reason) {
  if (func == NULL || !TypeIsFunction(func) ||
      func->info.function.unknown_args) {
    return false;
  }
  size_t num_actual = node->children->length;
  size_t num_formal = func->info.function.prototype.length;
  if (first_formal_arg > num_formal) {
    return false;
  }
  size_t num_user_formal = num_formal - first_formal_arg;
  size_t required = FunctionRequiredArgumentCount(func, first_formal_arg);

  if (num_actual < required) {
    if (required == num_user_formal) {
      StringPrintf(reason, "requires %zu argument%s, but %zu %s provided",
                   num_user_formal, num_user_formal == 1 ? "" : "s", num_actual,
                   num_actual == 1 ? "was" : "were");
    } else {
      StringPrintf(reason,
                   "requires at least %zu argument%s, but %zu %s provided",
                   required, required == 1 ? "" : "s", num_actual,
                   num_actual == 1 ? "was" : "were");
    }
    return true;
  }
  if (!func->info.function.varargs && num_actual > num_user_formal) {
    StringPrintf(reason, "requires %zu argument%s, but %zu %s provided",
                 num_user_formal, num_user_formal == 1 ? "" : "s", num_actual,
                 num_actual == 1 ? "was" : "were");
    return true;
  }

  size_t num_checked =
      num_actual < num_user_formal ? num_actual : num_user_formal;
  for (size_t i = 0; i < num_checked; i++) {
    ASTNode* actual = (ASTNode*)node->children->value.p[i];
    Symbol* formal =
        (Symbol*)func->info.function.prototype.value.p[i + first_formal_arg];
    if (OverloadConversionRank(actual, formal->type) >= 0) {
      continue;
    }
    String from;
    StringInit(&from, NULL);
    String to;
    StringInit(&to, NULL);
    AppendReadableType(actual != NULL ? actual->type : NULL, &from);
    AppendReadableType(formal->type, &to);
    StringPrintf(reason, "no known conversion from '%s' to '%s' for argument %zu",
                 from.value, to.value, i + 1);
    StringDestruct(&from);
    StringDestruct(&to);
    return true;
  }
  return false;
}

// For a compiler-synthesized special member (one the user never wrote), returns
// a short description such as "implicit copy constructor".  Returns NULL for an
// ordinary user-declared function so the caller falls back to the signature.
static const char* ImplicitSpecialMemberDescription(Symbol* candidate) {
  if (candidate == NULL || candidate->type == NULL ||
      !TypeIsFunction(candidate->type) ||
      !candidate->type->info.function.is_implicitly_declared) {
    return NULL;
  }
  if (candidate->type->info.function.is_constructor) {
    Vector* proto = &candidate->type->info.function.prototype;
    // prototype[0] is the implicit object parameter; the source operand of a
    // copy/move constructor, if any, is prototype[1].
    if (proto->length >= 2) {
      Symbol* source = proto->value.p[1];
      if (source != NULL && source->type != NULL &&
          TypeIsReference(source->type)) {
        return source->type->declarator == kDeclRValueReference
                   ? "implicit move constructor"
                   : "implicit copy constructor";
      }
    }
    return "implicit default constructor";
  }
  if (candidate->type->info.function.is_destructor) {
    return "implicit destructor";
  }
  return "implicitly-declared special member";
}

// Emits a "candidate ... not viable" note pointing at `candidate`'s declaration.
// `reason` (may be empty) explains why it was rejected; an empty reason produces
// a bare "candidate" note used for the equally-ranked candidates of an
// ambiguous call.  Compiler-synthesized members have no meaningful source
// location, so they are described in prose without one.
static void EmitCandidateNote(Symbol* candidate, const char* reason) {
  if (candidate == NULL) {
    return;
  }
  bool has_reason = reason != NULL && reason[0] != '\0';
  const char* implicit_desc = ImplicitSpecialMemberDescription(candidate);
  if (implicit_desc != NULL) {
    if (has_reason) {
      ReportNote(NULL, 0, "candidate (%s) not viable: %s", implicit_desc,
                 reason);
    } else {
      ReportNote(NULL, 0, "candidate (%s)", implicit_desc);
    }
    return;
  }
  String signature;
  StringInit(&signature, NULL);
  SymbolFunctionPrettyName(candidate, &signature);
  if (has_reason) {
    SemanticNoteAtLocation(candidate->location, "candidate '%s' not viable: %s",
                           signature.value, reason);
  } else {
    SemanticNoteAtLocation(candidate->location, "candidate '%s'",
                           signature.value);
  }
  StringDestruct(&signature);
}

// Emits one note per overload in `candidates` after a failed free-function /
// operator resolution.  When `ambiguous` is false (no viable candidate) each
// note explains the rejection; when true, the equally-best candidates (those
// whose score ties `best_score`) are listed as the competing matches.
static void EmitFreeCandidateNotes(Vector* candidates, VectorASTNode* call,
                                   bool ambiguous, int best_score) {
  for (size_t i = 0; i < candidates->length; i++) {
    Symbol* candidate = candidates->value.p[i];
    if (candidate == NULL) {
      continue;
    }
    bool is_template = SymbolIsTemplateFunction(candidate);
    if (ambiguous) {
      // The competing matches are the candidates whose conversion score ties the
      // winner.  Template candidates were resolved to specializations that are
      // not represented in this vector, so they are left out rather than
      // mislabeled.
      if (!is_template && OverloadCallScore(candidate, call) == best_score) {
        EmitCandidateNote(candidate, NULL);
      }
      continue;
    }
    if (is_template) {
      EmitCandidateNote(candidate,
                        "could not deduce template arguments for this call");
      continue;
    }
    String reason;
    StringInit(&reason, NULL);
    DescribeFunctionNonViability(candidate->type, call, 0, &reason);
    EmitCandidateNote(candidate, reason.value);
    StringDestruct(&reason);
  }
}

static Symbol* ResolveFunctionCandidateVector(String* name, Vector* candidates,
                                              Vector* actuals,
                                              Vector* explicit_args,
                                              bool diagnose_no_match,
                                              bool diagnose_ambiguous,
                                              ASTNode* diagnostic_node) {
  VectorASTNode call = {0};
  call.children = actuals;
  Symbol* best = NULL;
  int best_score = -1;
  bool ambiguous = false;
  if (explicit_args == NULL) {
    for (size_t i = 0; i < candidates->length; i++) {
      Symbol* candidate = candidates->value.p[i];
      if (candidate == NULL || candidate->flags.is_template ||
          (candidate->type != NULL && TypeIsFunction(candidate->type) &&
           candidate->type->info.function.template_origin != NULL)) {
        continue;
      }
      int score = OverloadCallScore(candidate, &call);
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
    for (size_t i = 0; i < candidates->length; i++) {
      Symbol* candidate = candidates->value.p[i];
      Symbol* effective =
          FunctionTemplateOverloadCandidate(candidate, &call, explicit_args);
      if (effective == NULL) {
        continue;
      }
      int score = OverloadCallScore(effective, &call);
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

  bool already_diagnosed =
      diagnostic_node != NULL &&
      (diagnostic_node->flags & kASTOverloadDiagnosed) != 0;
  if (best == NULL) {
    if (diagnose_no_match && !already_diagnosed) {
      String function_name;
      StringInit(&function_name, NULL);
      Symbol* first_candidate =
          candidates->length > 0 ? candidates->value.p[0] : NULL;
      if (first_candidate != NULL) {
        SymbolFunctionDiagnosticName(first_candidate, &function_name);
      } else {
        StringAppendString(&function_name, name);
      }
      SemanticError(diagnostic_node, "No matching overload for %s",
                    function_name.value);
      StringDestruct(&function_name);
      VectorASTNode call = {0};
      call.children = actuals;
      EmitFreeCandidateNotes(candidates, &call, /*ambiguous=*/false, -1);
      diagnostic_node->flags |= kASTOverloadDiagnosed;
    }
    return NULL;
  }
  if (ambiguous) {
    if (diagnose_ambiguous && !already_diagnosed) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticName(best, &function_name);
      SemanticError(diagnostic_node, "Ambiguous overload for %s",
                    function_name.value);
      StringDestruct(&function_name);
      VectorASTNode call = {0};
      call.children = actuals;
      EmitFreeCandidateNotes(candidates, &call, /*ambiguous=*/true, best_score);
      diagnostic_node->flags |= kASTOverloadDiagnosed;
    }
    return best;
  }
  return best;
}

static Symbol* ResolveFreeFunctionWithADL(String* name, Vector* actuals,
                                          bool diagnose_ambiguous) {
  Vector candidates;
  VectorInit(&candidates);
  AddNamedFunctionCandidates(name, compiler->global_namespace, &candidates);
  AddADLFunctionCandidates(name, actuals, &candidates);
  ASTNode* diagnostic_node =
      actuals != NULL && actuals->length > 0 ? actuals->value.p[0] : NULL;
  Symbol* best = ResolveFunctionCandidateVector(
      name, &candidates, actuals, NULL,
      /*diagnose_no_match=*/false, diagnose_ambiguous, diagnostic_node);
  VectorDestruct(&candidates);
  return best;
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
  DiagnosticSuppressBegin();
  Symbol* instantiated =
      TypeDeduceFunctionTemplateFromCallWithExplicitArgs(
          &compiler->syntax, candidate, explicit_args, node->children);
  DiagnosticSuppressEnd();
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

// Emits one note per member-function overload after a failed member-call /
// member-operator resolution.  When `ambiguous` is false each note explains why
// the candidate was rejected (argument mismatch, or a const object passed to a
// non-const method); when true, the equally-best candidates are listed.
static void EmitMemberCandidateNotes(StructMember* first, VectorASTNode* node,
                                     BinaryASTNode* member_access,
                                     bool ambiguous, int best_score) {
  bool receiver_const = MemberReceiverIsConst(member_access);
  for (StructMember* candidate = first; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->symbol == NULL) {
      continue;
    }
    bool is_template = SymbolIsTemplateFunction(candidate->symbol);
    if (ambiguous) {
      if (!is_template &&
          MemberOverloadCallScore(candidate, node, member_access, true) ==
              best_score) {
        EmitCandidateNote(candidate->symbol, NULL);
      }
      continue;
    }
    if (is_template) {
      EmitCandidateNote(candidate->symbol,
                        "could not deduce template arguments for this call");
      continue;
    }
    // A non-static method rejected only because the object is const (it would
    // be viable on a non-const object) gets a dedicated explanation.
    if (receiver_const && !candidate->is_static &&
        MemberOverloadCallScore(candidate, node, member_access, true) < 0 &&
        MemberOverloadCallScore(candidate, node, member_access, false) >= 0) {
      EmitCandidateNote(candidate->symbol,
                        "'this' argument has a const-qualified type, but the "
                        "method is not declared const");
      continue;
    }
    size_t first_formal_arg = candidate->is_static ? 0 : 1;
    String reason;
    StringInit(&reason, NULL);
    DescribeFunctionNonViability(candidate->symbol->type, node, first_formal_arg,
                                 &reason);
    EmitCandidateNote(candidate->symbol, reason.value);
    StringDestruct(&reason);
  }
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
  DiagnosticSuppressBegin();
  Symbol* instantiated =
      TypeDeduceFunctionTemplateFromCallWithExplicitArgsAndOffset(
          &compiler->syntax, candidate->symbol, explicit_args, node->children,
          first_formal_arg);
  DiagnosticSuppressEnd();
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

  bool already_diagnosed =
      ((ASTNode*)node)->flags & kASTOverloadDiagnosed;
  if (best == NULL) {
    if (already_diagnosed) {
      return first;
    }
    if (receiver_const_rejected) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticSuffix(first->symbol, &function_name);
      SemanticError((ASTNode*)member_access,
                    "Cannot call non-const member function %s on const object%s",
                    first->symbol->name.value, function_name.value);
      StringDestruct(&function_name);
      EmitMemberCandidateNotes(first, node, member_access, /*ambiguous=*/false,
                               -1);
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    } else if (first->overload_next != NULL ||
               first->symbol->type->info.function.is_constructor) {
      String function_name;
      StringInit(&function_name, NULL);
      SymbolFunctionDiagnosticName(first->symbol, &function_name);
      SemanticError((ASTNode*)node, "No matching overload for %s",
                    function_name.value);
      StringDestruct(&function_name);
      EmitMemberCandidateNotes(first, node, member_access, /*ambiguous=*/false,
                               -1);
      ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
    }
    return first;
  }
  if (ambiguous) {
    if (already_diagnosed) {
      return best;
    }
    String function_name;
    StringInit(&function_name, NULL);
    SymbolFunctionDiagnosticName(first->symbol, &function_name);
    SemanticError((ASTNode*)node, "Ambiguous overload for %s",
                  function_name.value);
    StringDestruct(&function_name);
    EmitMemberCandidateNotes(first, node, member_access, /*ambiguous=*/true,
                             best_score);
    ((ASTNode*)node)->flags |= kASTOverloadDiagnosed;
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
  if (!CompilerIsCXX() && !id->symbol->flags.is_overloaded) {
    return;
  }
  Vector candidates;
  VectorInit(&candidates);
  AddFunctionOverloadCandidates(&candidates, id->symbol);
  size_t ordinary_count = candidates.length;
  // [basic.lookup.argdep]/3: argument-dependent lookup produces no candidates
  // when ordinary unqualified lookup for the call name finds
  //   * a declaration that is neither a function nor a function template
  //     (e.g. a local variable, parameter, or type that shadows a namespace
  //     function), or
  //   * a block-scope function declaration that is not a using-declaration.
  // In those cases only the ordinary-lookup result is considered.  A
  // using-declaration that introduces a function does not suppress ADL; that
  // falls out naturally because we follow the alias to the underlying
  // (namespace-scope) function, which is not flagged block scope.  The invented
  // placeholder symbols created for calls to as-yet-undeclared functions carry
  // an unknown argument list and are precisely the names meant to be found
  // through ADL, so they must not trigger suppression.
  Symbol* ordinary = FollowUsingAliasForADL(id->symbol);
  bool ordinary_is_function = ordinary != NULL && ordinary->type != NULL &&
                              TypeIsFunction(ordinary->type);
  bool ordinary_suppresses_adl =
      !ordinary_is_function ||
      (ordinary->flags.is_block_scope &&
       !ordinary->type->info.function.unknown_args);
  bool allow_adl = CompilerIsCXX() &&
                   (node->left->flags & kASTQualifiedName) == 0 &&
                   !ordinary_suppresses_adl &&
                   !FunctionNameSkipsADL(&id->symbol->name);
  if (allow_adl) {
    AddADLFunctionCandidates(&id->symbol->name, node->children, &candidates);
  }
  bool has_adl_candidates = candidates.length > ordinary_count;
  bool ordinary_unknown =
      id->symbol != NULL && id->symbol->type != NULL &&
      TypeIsFunction(id->symbol->type) &&
      id->symbol->type->info.function.unknown_args;
  if (!id->symbol->flags.is_overloaded && !has_adl_candidates &&
      !ordinary_unknown) {
    VectorDestruct(&candidates);
    return;
  }
  Symbol* best = ResolveFunctionCandidateVector(
      &id->symbol->name, &candidates, node->children, id->template_arguments,
      /*diagnose_no_match=*/id->symbol->flags.is_overloaded || has_adl_candidates ||
          ordinary_unknown,
      /*diagnose_ambiguous=*/id->symbol->flags.is_overloaded ||
          has_adl_candidates || ordinary_unknown,
      (ASTNode*)node);
  VectorDestruct(&candidates);
  if (best == NULL) {
    return;
  }

  id->symbol = best;
  ASTNodeSetType(node->left, best->type);
  CheckDeletedFunctionUse(best, (ASTNode*)node);
}

static ASTNode* AnalyzeCXXFunctionalClassConstruction(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      !TypeIsStructOrUnion(node->left->type) ||
      node->left->type->info.struct_info == NULL ||
      node->left->type->info.struct_info->tag_name == NULL) {
    return NULL;
  }

  TypeRecord* placeholder_type = NULL;
  TypeRecord* construction_type = node->left->type;
  if (!TypeIsClassTemplatePlaceholder(construction_type) &&
      node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    /* For a plain class template the callee type carries no template
     * arguments. For an alias template (`using Alias = Foo<T>;`) the callee
     * type is the aliased `Foo<T>` and already has template arguments, but it
     * still needs the alias placeholder origin so CTAD deduces and instantiates
     * the underlying class template. Derive the placeholder from the symbol in
     * both cases. */
    bool is_alias_template =
        id->symbol != NULL && id->symbol->flags.is_template &&
        StorageIs(id->symbol->storage, STO(typedef));
    if (construction_type->template_arguments == NULL || is_alias_template) {
      placeholder_type = TypeClassTemplatePlaceholderFromSymbol(id->symbol);
      if (placeholder_type != NULL) {
        construction_type = placeholder_type;
      }
    }
  }
  TypeRecord* deduced_type = NULL;
  if (TypeIsClassTemplatePlaceholder(construction_type)) {
    bool alias_rejected = false;
    Symbol* class_template =
        TypeClassTemplatePlaceholderOrigin(construction_type);
    deduced_type = TypeDeduceClassTemplateFromPlaceholder(
        &compiler->syntax, construction_type, node->children,
        /*allow_explicit=*/true, &alias_rejected);
    if (deduced_type == NULL) {
      if (alias_rejected) {
        SemanticError((ASTNode*)node,
                      "Deduced template arguments do not match alias template");
      } else {
        SemanticError((ASTNode*)node,
                      "Could not deduce template arguments for %s",
                      class_template != NULL ? class_template->name.value
                                             : "<class template>");
      }
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
      if (placeholder_type != NULL) {
        TypeRecordDelete(placeholder_type);
      }
      return &node->base;
    }
    construction_type = deduced_type;
  }

  TypeRecord* type = TypeRecordCopy(construction_type);
  if (placeholder_type != NULL) {
    TypeRecordDelete(placeholder_type);
  }
  if (deduced_type != NULL) {
    TypeRecordDelete(deduced_type);
  }
  TypeRecordCalculateSize(type);
  SourceLocation location = node->base.location;
  String* constructor_name = type->info.struct_info->tag_name;
  // For a single-argument functional cast `T(arg)` (equivalent to the explicit
  // conversion `(T)arg`), if `arg` is a class object that supplies a
  // user-defined conversion operator yielding T, perform that conversion via
  // the C-style cast path (which considers conversion operators) instead of
  // trying to construct T from `arg`.  This is required when T cannot be built
  // from `arg` directly -- e.g. the comparison categories, whose value
  // constructor is private and whose cross-category conversions go through
  // `operator T()`.
  if (node->children->length == 1 &&
      ClassHasConversionOperatorTo(node->children->value.p[0], type)) {
    ASTNode* arg = ASTNodeMove(node->children->value.p[0]);
    ASTNode* cast = NewCastASTNode(type, location, arg);
    ASTNode* parent = node->base.parent;
    if (parent != NULL) {
      ASTNodeReplaceChild(parent, node->base.child_id, cast, true);
    }
    return AnalyzeExpression(cast);
  }
  StructMember* constructor =
      FindStructMember(type->info.struct_info, constructor_name);
  if (constructor == NULL || !constructor->is_member_function ||
      !constructor->symbol->type->info.function.is_constructor) {
    // The target class has no constructor (e.g. it is an aggregate).  Per
    // [expr.type.conv], a functional cast with a single parenthesized argument
    // `T(arg)` is equivalent to the explicit type conversion `(T)arg`.  Route it
    // through a C-style cast so that a user-defined conversion operator on the
    // argument (and the other standard conversions) are considered, matching the
    // behavior of the equivalent `(T)arg`.
    if (node->children->length == 1) {
      ASTNode* arg = ASTNodeMove(node->children->value.p[0]);
      ASTNode* cast = NewCastASTNode(type, location, arg);
      ASTNode* parent = node->base.parent;
      if (parent != NULL) {
        ASTNodeReplaceChild(parent, node->base.child_id, cast, true);
      }
      return AnalyzeExpression(cast);
    }
    return NULL;
  }
  bool has_non_invented_constructor = false;
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (candidate->is_member_function && candidate->symbol != NULL &&
        !candidate->symbol->flags.invented) {
      has_non_invented_constructor = true;
      break;
    }
  }
  if (!has_non_invented_constructor && node->children->length > 1) {
    return NULL;
  }

  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  const char* constructor_member_name = constructor->symbol != NULL
                                            ? constructor->symbol->name.value
                                            : constructor_name->value;
  ASTNode* member =
      NewStringConstantASTNode(NewString(constructor_member_name), NULL,
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

static ASTNode* MaterializeCXXByValueClassArgument(ASTNode* actual,
                                                   TypeRecord* formal_type) {
  if (!CompilerIsCXX() || actual == NULL || formal_type == NULL ||
      !TypeIsStructOrUnion(formal_type) ||
      !TypeIsStructOrUnion(actual->type) ||
      !TypeEqualIgnoringQualifiers(actual->type, formal_type) ||
      formal_type->info.struct_info == NULL ||
      formal_type->info.struct_info->tag_name == NULL ||
      formal_type->info.struct_info->is_aggregate) {
    return actual;
  }

  String* constructor_name = formal_type->info.struct_info->tag_name;
  StructMember* constructor =
      FindStructMember(formal_type->info.struct_info, constructor_name);
  if (constructor == NULL || !constructor->is_member_function ||
      constructor->symbol == NULL || constructor->symbol->type == NULL ||
      !constructor->symbol->type->info.function.is_constructor) {
    return actual;
  }
  bool has_nontrivial_unary_constructor = false;
  for (StructMember* candidate = constructor; candidate != NULL;
       candidate = candidate->overload_next) {
    if (!candidate->is_member_function || candidate->symbol == NULL ||
        candidate->symbol->type == NULL ||
        !TypeIsFunction(candidate->symbol->type) ||
        !candidate->symbol->type->info.function.is_constructor) {
      continue;
    }
    FunctionInfo* info = &candidate->symbol->type->info.function;
    if (info->varargs ||
        (info->prototype.length >= 2 && !info->is_trivial_special_member)) {
      has_nontrivial_unary_constructor = true;
      break;
    }
  }
  if (!has_nontrivial_unary_constructor) {
    return actual;
  }

  SourceLocation location = actual->location;
  Symbol* temp = SyntaxNewTemporary(&compiler->syntax, formal_type);
  temp->location = location;
  ASTNode* receiver = NewIdentifierASTNode(temp, location);
  ASTNode* member =
      NewStringConstantASTNode(NewString(constructor_name->value), NULL,
                               location);
  ASTNode* member_access =
      NewBinaryASTNode(AST_OP(dot), NULL, location, receiver, member);

  Vector* actuals = NewVector();
  VectorAppend(actuals, ASTNodeMove(actual));
  ASTNode* constructor_call =
      NewVectorASTNode(AST_OP(call), NULL, location, member_access, actuals);
  ASTNode* result = NewIdentifierASTNode(temp, location);
  ASTNode* comma =
      NewBinaryASTNode(AST_OP(comma), formal_type, location, constructor_call,
                       result);
  ASTNode* analyzed = AnalyzeExpression(comma);
  analyzed->value_category = kValueCategoryPrvalue;
  return analyzed;
}

// Handle calling an object that has an `operator()` member, i.e. a lambda's
// closure object or any other functor: `obj(args)` is rewritten into the member
// call `obj.operator()(args)`.  The receiver and argument nodes are detached
// from the original call (leaving NULL holes the caller no longer owns) and
// re-parented under the new member call.  Returns NULL when the callee is not a
// class/union with a callable `operator()`, leaving `node` untouched.
static ASTNode* TryAnalyzeOverloadedCallOperator(VectorASTNode* node) {
  if (!CompilerIsCXX() || node->left == NULL ||
      !TypeIsStructOrUnion(node->left->type)) {
    return NULL;
  }

  StructMember* member =
      FindStructMemberByName(node->left->type->info.struct_info, "operator()");
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }

  // Move the actuals out of the original call node into the member call.
  Vector* actuals = NewVector();
  for (size_t i = 0; i < node->children->length; i++) {
    ASTNode* actual = node->children->value.p[i];
    VectorSet(node->children, i, NULL);
    if (actual != NULL) {
      actual->parent = NULL;
    }
    VectorAppend(actuals, actual);
  }
  ASTNode* receiver = node->left;
  node->left = NULL;
  if (receiver != NULL) {
    receiver->parent = NULL;
  }
  ASTNode* call =
      NewOperatorMemberCall(receiver, "operator()", actuals,
                            node->base.location);
  return ReplaceVectorWithCall(node, call);
}

static bool FunctionTemplateHasDefinition(Symbol* symbol) {
  if (symbol == NULL || symbol->type == NULL || !TypeIsFunction(symbol->type)) {
    return false;
  }
  Symbol* definition =
      symbol->value.func_defn != NULL ? symbol->value.func_defn : symbol;
  return definition != NULL && definition->type != NULL &&
         TypeIsFunction(definition->type) &&
         definition->type->info.function.body != NULL;
}

static bool TemplateArgumentContainsTemplateParameter(TemplateArgument* arg) {
  if (arg == NULL) {
    return false;
  }
  if (arg->template_parameter_index >= 0 ||
      TypeContainsTemplateParameter(arg->type)) {
    return true;
  }
  for (size_t i = 0; arg->pack_arguments != NULL &&
                     i < arg->pack_arguments->length; i++) {
    if (TemplateArgumentContainsTemplateParameter(
            arg->pack_arguments->value.p[i])) {
      return true;
    }
  }
  return false;
}

static bool TemplateArgumentVectorContainsTemplateParameter(Vector* args) {
  for (size_t i = 0; args != NULL && i < args->length; i++) {
    if (TemplateArgumentContainsTemplateParameter(args->value.p[i])) {
      return true;
    }
  }
  return false;
}

static bool CallActualsContainTemplateParameter(VectorASTNode* call) {
  for (size_t i = 0; call != NULL && i < call->children->length; i++) {
    ASTNode* actual = call->children->value.p[i];
    if (actual != NULL && TypeContainsTemplateParameter(actual->type)) {
      return true;
    }
  }
  return false;
}

static ASTNode* AnalyzeFunctionCall(VectorASTNode* node) {
  node->left = AnalyzeExpression(node->left);
  size_t num_actual_args = node->children->length;
  for (size_t i = 0; i < num_actual_args; i++) {
    node->children->value.p[i] = AnalyzeExpression((ASTNode*)node->children->value.p[i]);
  }
  bool has_pack_expansion_actual = CallHasPackExpansionActual(node);
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    if (id->symbol != NULL && id->symbol->flags.is_template &&
        !id->symbol->flags.is_overloaded && TypeIsFunction(id->symbol->type) &&
        !has_pack_expansion_actual &&
        !TemplateArgumentVectorContainsTemplateParameter(id->template_arguments) &&
        !CallActualsContainTemplateParameter(node) &&
        FunctionTemplateHasDefinition(id->symbol)) {
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
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    ResolveOverloadedFunctionCall(node);
  }
  LowerMemberFunctionCall(node);
  /* A callee that names a class or alias template (e.g. `AliasHolder(11)`) is a
   * CTAD functional construction, not a dependent functor call. Its type
   * legitimately contains the template's own parameters; whether the call is
   * dependent is determined by the actual arguments. When the actuals are
   * concrete, route it to functional construction instead of deferring. */
  bool is_concrete_ctad_construction = false;
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* callee_id = (IdentifierASTNode*)node->left;
    if (callee_id->symbol != NULL && callee_id->symbol->flags.is_template &&
        callee_id->symbol->type != NULL &&
        TypeIsStructOrUnion(callee_id->symbol->type) &&
        !has_pack_expansion_actual &&
        !CallActualsContainTemplateParameter(node)) {
      is_concrete_ctad_construction = true;
    }
  }
  // A call whose callee type still depends on template parameters (e.g. a
  // functor or lambda received as a template parameter) cannot be resolved
  // until instantiation.  Flag it and give it a placeholder type so it is
  // re-analyzed once the template arguments are known.
  if (CompilerIsCXX() && node->left != NULL && node->left->type != NULL &&
      !is_concrete_ctad_construction &&
      TypeContainsTemplateParameter(node->left->type)) {
    node->base.flags |= kASTDependentFunctorCall;
    ASTNodeSetType((ASTNode*)node,
                   NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain));
    return (ASTNode*)node;
  }
  // A call on a concrete class object may be invoking its `operator()` (a
  // lambda's closure or other functor); rewrite it to the member call.
  ASTNode* overloaded_call = TryAnalyzeOverloadedCallOperator(node);
  if (overloaded_call != NULL) {
    return overloaded_call;
  }
  ASTNode* construction = AnalyzeCXXFunctionalClassConstruction(node);
  if (construction != NULL) {
    return construction;
  }
  LowerMemberFunctionCall(node);
  ResolveOverloadedFunctionCall(node);
  if (node->left != NULL && node->left->op == AST_OP(identifier)) {
    IdentifierASTNode* id = (IdentifierASTNode*)node->left;
    CheckDeletedFunctionUse(id->symbol, (ASTNode*)node);
    if (id->symbol != NULL && id->symbol->flags.is_template &&
        !has_pack_expansion_actual) {
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
  if (!subtype->info.function.unknown_args && !has_pack_expansion_actual) {
    if (AppendDefaultCallArguments(node, subtype)) {
      num_actual_args = node->children->length;
    }
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
      ASTNode* converted_initializer_list =
          ConvertCXXInitializerListArgument(actual, formal->type);
      if (converted_initializer_list != actual) {
        ASTNodeReplaceChild((ASTNode*)node, (int)i, converted_initializer_list,
                            true);
        actual = converted_initializer_list;
      }
      bool polymorphic_special_this =
          i == 0 &&
          (subtype->info.function.is_constructor ||
           subtype->info.function.is_destructor) &&
          subtype->info.function.cxx_member_owner != NULL &&
          subtype->info.function.cxx_member_owner->virtual_members.length > 0;
      if (TypeIsReference(formal->type)) {
        TypeRecord* reference_type = formal->type;
        bool discards_qualifiers =
            TypeIsConst(actual->type) && !TypeIsConst(reference_type->next);
        if (!polymorphic_special_this) {
          NormalConversion(actual,
                           ReferenceConversionTarget(actual, reference_type));
        }
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
        if (actual->value_category == kValueCategoryXvalue) {
          ASTNode* materialized =
              MaterializeCXXByValueClassArgument(actual, formal->type);
          if (materialized != actual) {
            ASTNodeReplaceChild((ASTNode*)node, (int)i, materialized, false);
            actual = materialized;
          }
        }
        if (!polymorphic_special_this) {
          NormalConversion(actual, formal->type);
        }
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
  
  if (call_ok && subtype->info.function.is_consteval) {
    bool ok = false;
    if (TypeIsFloatingPoint(return_type)) {
      double value;
      ok = EvaluateFloatingPointExpression((ASTNode*)node, &value);
    } else if (TypeIsIntegral(return_type)) {
      int64_t value;
      ok = EvaluateIntegerExpression((ASTNode*)node, &value);
    } else if (TypeIsFixedArray(return_type) || TypeIsStructOrUnion(return_type)) {
      ConstEvalContext ctx;
      ConstEvalContextInit(&ctx);
      ok = ConstexprEvaluateCallAsObject(&ctx, (ASTNode*)node);
      ConstEvalContextDestruct(&ctx);
    }
    if (!ok) {
      SemanticError((ASTNode*)node,
                    "consteval function call is not a constant expression");
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

static ASTNode* TryAnalyzeOverloadedArrowOperator(ASTNode* receiver,
                                                  SourceLocation location) {
  if (!CompilerIsCXX() || receiver == NULL ||
      !TypeIsStructOrUnion(receiver->type)) {
    return NULL;
  }

  StructMember* member =
      FindStructMemberByName(receiver->type->info.struct_info, "operator->");
  if (member == NULL || !member->is_member_function) {
    return NULL;
  }

  ASTNode* call = NewOperatorMemberCall(ASTNodeMove(receiver), "operator->",
                                        NULL, location);
  return AnalyzeExpression(call);
}

static void AnalyzeMemberReference(BinaryASTNode* node) {
  if (node->base.type != NULL) {
    // Already analyzed.
    return;
  }
  Struct* struct_info = NULL;

  node->left = AnalyzeExpression(node->left);
  if (node->base.op == AST_OP(arrow) &&
      !TypeIsStructOrUnionPointer(node->left->type)) {
    ASTNode* overloaded_arrow =
        TryAnalyzeOverloadedArrowOperator(node->left, node->base.location);
    if (overloaded_arrow != NULL) {
      node->left = overloaded_arrow;
      node->left->parent = (ASTNode*)node;
      node->left->child_id = 0;
    }
  }
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
  int member_offset = 0;
  StructMember* member =
      FindStructMemberWithAccessAndOffset(struct_info, member_name, &access,
                                          &member_owner, &member_offset);
  if (member == NULL) {
    SemanticError((ASTNode*)node, "%s is not a member of struct/union %s",
                  member_name->value, struct_info->tag_name->value);
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    return;
  }
  ApplyVirtualBaseAdjustmentToMemberReference(node, struct_info, member_owner,
                                              &member_offset);
  if (member_offset < 0) {
    member_offset = member->byte_offset;
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
  ((StructMemberASTNode*)node->right)->byte_offset = member_offset;
  ((StructMemberASTNode*)node->right)->template_arguments =
      TemplateArgumentVectorCopy(explicit_template_arguments);
  ASTNodeDelete(old_right);
  TypeRecord* member_type = member->symbol->type;
  if (!member->is_static && !member->is_member_function && !member->is_mutable &&
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
  if (node->is_pack_size) {
    bool valid_pack = (node->expr != NULL &&
                       (node->expr->flags & kASTPackExpansion) != 0);
    if (node->expr != NULL && node->expr->op == AST_OP(identifier)) {
      IdentifierASTNode* id = (IdentifierASTNode*)node->expr;
      valid_pack =
          id->symbol != NULL && id->symbol->flags.is_parameter_pack;
    }
    if (!valid_pack) {
      SemanticError((ASTNode*)node, "sizeof... requires a parameter pack");
    }
    ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
    return;
  }
  if (node->expr != NULL) {
    node->expr = AnalyzeExpression(node->expr);
    if (TypeIsVLA(node->expr->type)) {
      // sizeof(vla) is calculated at runtime.
    } else {
      node->base.value.ivalue = node->expr->type->size;
    }
  } else if (node->type_operand != NULL &&
             !TypeContainsTemplateParameter(node->type_operand)) {
    // A `sizeof(type-id)` whose operand has become concrete (e.g. after
    // template instantiation): re-measure the now-complete type.
    TypeRecordCalculateSize(node->type_operand);
    node->base.value.ivalue = node->type_operand->size;
  }
  ASTNodeSetType((ASTNode*)node, NewSizeTypeRecord());
}

// Finds the std::<name> class type, or NULL if it is not declared (e.g. the
// relevant standard header has not been included).
static TypeRecord* FindStdClassType(const char* name) {
  String std_name;
  StringInit(&std_name, "std");
  Namespace* std_ns = NamespaceFindChild(compiler->global_namespace, &std_name);
  StringDestruct(&std_name);
  if (std_ns == NULL) {
    return NULL;
  }
  String class_name;
  StringInit(&class_name, name);
  Symbol* sym = NamespaceFindSymbol(std_ns, &class_name);
  StringDestruct(&class_name);
  if (sym == NULL || sym->type == NULL || !TypeIsStructOrUnion(sym->type)) {
    return NULL;
  }
  return sym->type;
}

static bool TypeIsPolymorphicClass(TypeRecord* type) {
  return type != NULL && TypeIsStructOrUnion(type) &&
         type->info.struct_info != NULL &&
         FindStructMemberByName(type->info.struct_info, "__vptr") != NULL;
}

// Lowers a typeid operator into the underlying std::type_info access.  The
// static form yields the address of the type's emitted type_info object; the
// polymorphic form (a glvalue of polymorphic class type) reads the most-derived
// type_info out of the object's vtable header at runtime.  Returns a fully
// analyzed replacement expression.
static ASTNode* AnalyzeTypeidExpression(TypeidASTNode* node) {
  SourceLocation location = node->base.location;
  TypeRecord* type_info_type = FindStdClassType("type_info");
  if (type_info_type == NULL) {
    SemanticError((ASTNode*)node,
                  "typeid requires <typeinfo> to be included");
    ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
    node->base.flags |= kASTAnalyzed;
    return (ASTNode*)node;
  }

  bool polymorphic = false;
  TypeRecord* static_type = node->operand_type;
  ASTNode* operand = node->expr;
  if (operand != NULL) {
    operand = AnalyzeExpression(operand);
    node->expr = operand;
    static_type = operand->type;
    bool glvalue = operand->value_category == kValueCategoryLvalue ||
                   operand->value_category == kValueCategoryXvalue;
    if (glvalue && TypeIsPolymorphicClass(static_type)) {
      polymorphic = true;
    }
  }
  if (static_type != NULL && TypeIsReference(static_type)) {
    static_type = static_type->next;
  }

  TypeRecord* type_info_ptr = NewPointerTo(kQualPlain, type_info_type);

  ASTNode* result;
  if (polymorphic) {
    // *(const type_info*)( (&operand)->__vptr[-1] )
    ASTNode* address =
        NewUnaryASTNode(AST_OP(address), NULL, location, operand);
    ASTNode* vptr = NewBinaryASTNode(
        AST_OP(arrow), NULL, location, address,
        NewStringConstantASTNode(NewString("__vptr"), NULL, location));
    ASTNode* slot = NewBinaryASTNode(
        AST_OP(subscript), NULL, location, vptr,
        NewIntConstantASTNode(-1, NewTypeRecordWithSize(kTypeInt, kQualPlain),
                              location));
    ASTNode* as_ptr = NewCastASTNode(type_info_ptr, location, slot);
    result = NewUnaryASTNode(AST_OP(contents), NULL, location, as_ptr);
  } else {
    Symbol* type_info_symbol = RttiGetTypeInfoSymbol(static_type);
    if (type_info_symbol == NULL) {
      SemanticError((ASTNode*)node,
                    "cannot form typeid for this type");
      ASTNodeSetType((ASTNode*)node,
                     NewTypeRecordWithSize(kTypeInt, kQualPlain));
      node->base.flags |= kASTAnalyzed;
      return (ASTNode*)node;
    }
    // *(const type_info*)&__davecc_ti_<type>
    ASTNode* id = NewIdentifierASTNode(type_info_symbol, location);
    ASTNode* address = NewUnaryASTNode(AST_OP(address), NULL, location, id);
    ASTNode* as_ptr = NewCastASTNode(type_info_ptr, location, address);
    result = NewUnaryASTNode(AST_OP(contents), NULL, location, as_ptr);
  }
  return AnalyzeExpression(result);
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

// Analyzes a dynamic_cast.  Identity and upcast (to an unambiguous public base)
// are resolved statically as before.  Polymorphic downcasts and sidecasts are
// marked for a run-time check lowered during codegen.  Handles both the pointer
// form (null on failure) and the reference form (throws std::bad_cast).
static void AnalyzeDynamicCast(CastASTNode* node) {
  TypeRecord* to = node->cast_type;
  TypeRecord* from = node->expr->type;
  bool to_ref = TypeIsReference(to);
  TypeRecord* to_class = (to_ref || TypeIsPointer(to)) ? to->next : NULL;
  TypeRecord* from_class = to_ref ? from
                                  : (TypeIsPointer(from) ? from->next : NULL);

  if (to_class == NULL || from_class == NULL ||
      !TypeIsStructOrUnion(to_class) || !TypeIsStructOrUnion(from_class)) {
    SemanticError((ASTNode*)node,
                  "dynamic_cast requires pointer or reference to class type");
    ASTNodeSetType((ASTNode*)node, to);
    return;
  }

  bool identity = TypeEqual(to_class, from_class);
  bool upcast = !identity && TypeIsDerivedFrom(from_class, to_class);

  if (identity || upcast) {
    // Statically resolvable (no run-time check needed).
    node->dynamic_runtime = false;
    if (to_ref) {
      if (!TypeEqualIgnoringQualifiers(from_class, to_class)) {
        SemanticConvertType(node->expr, to_class, kConvertCast);
      }
      ASTNodeSetType((ASTNode*)node, to_class);
      node->base.value_category = to->declarator == kDeclRValueReference
                                      ? kValueCategoryXvalue
                                      : kValueCategoryLvalue;
    } else {
      SemanticConvertType(node->expr, to, kConvertCast);
      ASTNodeSetType((ASTNode*)node, to);
    }
    return;
  }

  // Downcast / sidecast: requires a polymorphic source.
  if (!TypeIsPolymorphicClass(from_class)) {
    SemanticError((ASTNode*)node,
                  "dynamic_cast of a non-polymorphic type");
    ASTNodeSetType((ASTNode*)node, to);
    return;
  }

  node->dynamic_runtime = true;
  if (to_ref) {
    ASTNodeSetType((ASTNode*)node, to_class);
    node->base.value_category = to->declarator == kDeclRValueReference
                                    ? kValueCategoryXvalue
                                    : kValueCategoryLvalue;
  } else {
    ASTNodeSetType((ASTNode*)node, to);
  }
}

static void AnalyzeCastExpression(CastASTNode* node) {
  node->expr = AnalyzeExpression(node->expr);
  if (node->kind == kCastDynamic) {
    AnalyzeDynamicCast(node);
    return;
  }
  if (node->kind == kCastConst) {
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
  SemanticConvertType(node->left, bool_type, kConvertContextualBool);
  node->right = AnalyzeExpression(node->right);
  SemanticConvertType(node->right, bool_type, kConvertContextualBool);
  ASTNodeSetType((ASTNode*)node, node->left->type);
  SemanticCheckScalarType(node->left);
  SemanticCheckScalarType(node->right);

  ASTNodeSetType((ASTNode*)node, bool_type);
}

static void AnalyzeThrowExpression(ThrowASTNode* node) {
  if (!CompilerExceptionsEnabled()) {
    SemanticError((ASTNode*)node,
                  "cannot use 'throw' with exception handling disabled "
                  "(-fno-exceptions)");
  }
  if (node->expr == NULL) {
    if (!SemanticInCatchHandler()) {
      SemanticError((ASTNode*)node,
                    "throw without operand is only valid in a catch handler");
    }
  } else {
    node->expr = AnalyzeExpression(node->expr);
    if (TypeIsVoid(node->expr->type) || TypeIsFunction(node->expr->type)) {
      SemanticError(node->expr, "Cannot throw expression of this type");
    }
  }
  // A throw in a 'noexcept' function is well-formed: per [except.spec], if the
  // exception escapes the function at runtime std::terminate is called.  That
  // runtime guard is inserted during codegen, so nothing is enforced here.
  ASTNodeSetType((ASTNode*)node, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
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

static TypeRecord* AtomicPointerPointee(VectorASTNode* node) {
  if (node->children->length == 0) {
    return NULL;
  }
  ASTNode* ptr = node->children->value.p[0];
  if (ptr == NULL || !TypeIsPointer(ptr->type) || ptr->type->next == NULL ||
      TypeIsVoid(ptr->type->next)) {
    SemanticError((ASTNode*)node,
                  "atomic builtin requires pointer to object type");
    return NewTypeRecordWithSize(kTypeInt | kTypeUnknown, kQualPlain);
  }
  return ptr->type->next;
}

static void AnalyzeAtomicBuiltinChildren(VectorASTNode* node) {
  for (size_t i = 0; i < node->children->length; i++) {
    node->children->value.p[i] =
        AnalyzeExpression((ASTNode*)node->children->value.p[i]);
  }
}

static void AnalyzeAtomicLoadBuiltin(VectorASTNode* node) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type = AtomicPointerPointee(node);
  ASTNodeSetType(&node->base, TypeRecordCopy(value_type));
}

static void AnalyzeAtomicStoreBuiltin(VectorASTNode* node) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type = AtomicPointerPointee(node);
  if (node->children->length > 1) {
    NormalConversion(node->children->value.p[1], value_type);
  }
  ASTNodeSetType(&node->base, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
}

static void AnalyzeAtomicFetchBuiltin(VectorASTNode* node, bool returns_new) {
  (void)returns_new;
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type = AtomicPointerPointee(node);
  if (!TypeIsIntegral(value_type) && !TypeIsPointer(value_type)) {
    SemanticError((ASTNode*)node,
                  "atomic arithmetic builtin requires integral or pointer type");
  }
  if (node->children->length > 1) {
    NormalConversion(node->children->value.p[1], value_type);
  }
  ASTNodeSetType(&node->base, TypeRecordCopy(value_type));
}

static void AnalyzeAtomicCompareExchangeBuiltin(VectorASTNode* node,
                                                bool expected_is_pointer,
                                                bool returns_bool) {
  AnalyzeAtomicBuiltinChildren(node);
  TypeRecord* value_type = AtomicPointerPointee(node);
  if (node->children->length > 1) {
    ASTNode* expected = node->children->value.p[1];
    if (expected_is_pointer) {
      if (expected == NULL || !TypeIsPointer(expected->type) ||
          expected->type->next == NULL ||
          !TypeEqual(expected->type->next, value_type)) {
        SemanticError(expected != NULL ? expected : (ASTNode*)node,
                      "atomic compare exchange expected argument has incompatible type");
      }
    } else {
      NormalConversion(expected, value_type);
    }
  }
  if (node->children->length > 2) {
    NormalConversion(node->children->value.p[2], value_type);
  }
  ASTNodeSetType(&node->base,
                 returns_bool ? NewTypeRecordWithSize(kTypeBool, kQualPlain)
                              : TypeRecordCopy(value_type));
}

static TypeRecord* NewConstCharPointerType(void) {
  TypeRecord* char_type = NewTypeRecordWithSize(kTypeChar, kQualConst);
  TypeRecord* pointer_type = NewPointerTo(kQualPlain, char_type);
  return TypeRecordCalculateSize(pointer_type);
}

static void AnalyzeSourceStringBuiltin(VectorASTNode* node) {
  ASTNodeSetType(&node->base, NewConstCharPointerType());
}

static void AnalyzeSourceIntegerBuiltin(VectorASTNode* node) {
  ASTNodeSetType(&node->base,
                 NewTypeRecordWithSize(kTypeInt | kTypeUnsigned, kQualPlain));
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
      node = AnalyzeShift(binary_node);
      break;

    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
      node = AnalyzeBitwiseOperator(binary_node);
      break;

    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
      node = AnalyzeComparisonOperator(binary_node);
      break;

    case AST_OP(spaceship):
      node = AnalyzeThreeWayComparison(binary_node);
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
      node = AnalyzeIncDec(unary_node);
      break;

    case AST_OP(uplus):
    case AST_OP(uminus):
      AnalyzeUnaryExpression(unary_node);
      break;

    case AST_OP(sizeof):
      AnalyzeSizeofExpression((SizeofASTNode*)node);
      break;

    case AST_OP(typeid):
      return AnalyzeTypeidExpression((TypeidASTNode*)node);

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
      node = AnalyzeArraySubscript(binary_node);
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
      {
        ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(binary_node);
        if (overloaded != NULL) {
          node = overloaded;
          break;
        }
      }

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

    case AST_OP(builtin_atomic_load):
      AnalyzeAtomicLoadBuiltin(vector_node);
      break;

    case AST_OP(builtin_atomic_store):
      AnalyzeAtomicStoreBuiltin(vector_node);
      break;

    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
      AnalyzeAtomicFetchBuiltin(vector_node, false);
      break;

    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
      AnalyzeAtomicFetchBuiltin(vector_node, true);
      break;

    case AST_OP(builtin_atomic_compare_exchange_bool):
      AnalyzeAtomicCompareExchangeBuiltin(vector_node, false, true);
      break;

    case AST_OP(builtin_atomic_compare_exchange_val):
      AnalyzeAtomicCompareExchangeBuiltin(vector_node, false, false);
      break;

    case AST_OP(builtin_atomic_compare_exchange_n):
      AnalyzeAtomicCompareExchangeBuiltin(vector_node, true, true);
      break;

    case AST_OP(builtin_atomic_fence):
      AnalyzeAtomicBuiltinChildren(vector_node);
      ASTNodeSetType(node, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
      break;

    case AST_OP(builtin_source_file):
    case AST_OP(builtin_source_function):
    case AST_OP(builtin_source_pretty_function):
      AnalyzeSourceStringBuiltin(vector_node);
      break;

    case AST_OP(builtin_source_line):
    case AST_OP(builtin_source_column):
      AnalyzeSourceIntegerBuiltin(vector_node);
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

    case AST_OP(throw):
      AnalyzeThrowExpression((ThrowASTNode*)node);
      break;

    case AST_OP(co_await):
      AnalyzeCoAwaitExpression(unary_node);
      break;

    case AST_OP(co_yield):
      AnalyzeCoYieldExpression(unary_node);
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
