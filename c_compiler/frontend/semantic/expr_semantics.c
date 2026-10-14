//
//  expr_semantics.c
//  c_compiler
//
//  Public expression-analysis dispatch and post-processing.
//

#include "expr_semantics.h"
#include "expr_semantics_internal.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "concepts.h"
#include "contracts.h"
#include "constexpr.h"
#include "expr_evaluator.h"
#include "init_semantics.h"
#include "statement_semantics.h"
#include "compiler.h"
#include "errors.h"
#include "symbol_table.h"
#include "typo_correction.h"
#include "rtti.h"
#include "reflection.h"
#include "reflection_meta_synthesis.h"
#include "reflection_meta_traits.h"
#include "reflection_semantics.h"
#include "type_traits_semantics.h"
#include "type_compare.h"
#include "member_pointer.h"
#include "statement_parser.h"
#include "type_parse.h"
#include "type_class_internal.h"
#include "type_inheritance.h"
#include "type_special_member.h"
#include "type_template.h"

// Contract: Visitor callback only; mutates matching is_same_v identifiers by
// clearing kASTAnalyzed pre-children. Does not take ownership of node or data.
static void ClearStaleIsSameVAnalysis(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode);

// Synchronizes an identifier retained from dependent analysis with its symbol
// after `auto` or template deduction has made the symbol type concrete.
static void RefreshStaleIdentifierType(ASTNode* node);

// Contract: Runs before opcode dispatch. May replace the AST root via overload or
// `!=` rewrite; a non-NULL return ends analysis immediately (no post-process).
// Does not set kASTAnalyzed on the returned subtree.
static ASTNode* AnalyzeExpressionPreDispatch(ASTNode* node,
                                             BinaryASTNode* binary_node,
                                             UnaryASTNode* unary_node);

// Contract: Routes `op` through category helpers in the same order as the
// legacy monolithic switch. Updates `node` (may replace root); sets `*finished`
// when the legacy switch used `return` (no post-process for caller).
static void AnalyzeExpressionDispatchOpcode(
    ASTNode** node, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    VectorASTNode* vector_node, bool* finished);

// Phase 1 (monolith order): literals through unevaluated queries. Returns true
// when `op` is handled; writes *node_inout unless *finished is already set.
static bool AnalyzeExpressionDispatchOpcodePhaseOperators(
    ASTNode** node_inout, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    int op);

// Phase 2: typeid/reflection early finishes, then cast through comma/logical.
static bool AnalyzeExpressionDispatchOpcodePhaseTypeAndAccess(
    ASTNode** node_inout, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    VectorASTNode* vector_node, bool* finished, int op);

// Phase 3: initializers, compiler builtins, stmt-expr/coro/type-trait finish.
static bool AnalyzeExpressionDispatchOpcodePhaseInitBuiltinsAndCoroutine(
    ASTNode** node_inout, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    VectorASTNode* vector_node, bool* finished, int op);

// Contract: Leaf literals and requires-expressions; no mutation, types pre-set.
static ASTNode* DispatchExpressionLiterals(ASTNode* node);

// Contract: Primary names; may replace `node` via fold/materialize in callee.
static ASTNode* DispatchExpressionIdentifier(ASTNode* node);

// Contract: Binary +, -, *, /, %, shifts, and bitwise ops; may replace root.
static ASTNode* DispatchExpressionBinaryArithmetic(ASTNode* node,
                                                   BinaryASTNode* binary,
                                                   int op);

// Contract: Relational/equality (incl. reflection operands), <=>, and ?:.
static ASTNode* DispatchExpressionComparisonsAndConditional(
    ASTNode* node, BinaryASTNode* binary, int op);

// Contract: Simple and compound assignment; may replace root.
static ASTNode* DispatchExpressionAssignment(ASTNode* node,
                                             BinaryASTNode* binary);

// Contract: ++/--; may replace root.
static ASTNode* DispatchExpressionIncDec(ASTNode* node, UnaryASTNode* unary);

// Contract: Unary + and - on operands (not increment).
static void DispatchExpressionUnaryPlusMinus(UnaryASTNode* unary);

// Contract: Unevaluated type/trait queries (sizeof, alignof, noexcept).
static void DispatchExpressionUnevaluatedQueries(ASTNode* node,
                                               UnaryASTNode* unary, int op);

// Contract: typeid; sets `*finished`, replaces root; never runs post-process.
static void DispatchExpressionTypeid(ASTNode* node, ASTNode** node_inout,
                                     bool* finished);

// Contract: Reflection/splice nodes; sets `*finished`, replaces root.
static void DispatchExpressionReflectionAndSplice(ASTNode* node,
                                                ASTNode** node_inout,
                                                bool* finished, int op);

// Contract: C-style cast and compound literal.
static ASTNode* DispatchExpressionCastAndCompound(ASTNode* node, int op);

// Contract: Logical/bitwise unary ! and ~.
static void DispatchExpressionLogicalComplement(UnaryASTNode* unary, int op);

// Contract: & and * (incl. addressed splice); may set `*finished` and re-root.
static ASTNode* DispatchExpressionAddressAndDeref(
    ASTNode* node, UnaryASTNode* unary, ASTNode** node_inout, bool* finished,
    int op);

// Contract: Subscript (1D or multidimensional) and pack indexing.
static ASTNode* DispatchExpressionSubscriptAndPack(
    ASTNode* node, BinaryASTNode* binary, VectorASTNode* vector, int op);

// Contract: Calls (with std::get folds) and range-for iterator resolution.
static ASTNode* DispatchExpressionCallAndRange(ASTNode* node,
                                               VectorASTNode* vector,
                                               BinaryASTNode* binary, int op);

// Contract: ., ->, .* , ->* member access; may replace root.
static ASTNode* DispatchExpressionMemberAccess(ASTNode* node,
                                               BinaryASTNode* binary, int op);

// Contract: Pointer-to-member formation (unary).
static void DispatchExpressionPointerToMember(UnaryASTNode* unary);

// Contract: Comma (with overload) and short-circuit logical ops.
static ASTNode* DispatchExpressionCommaAndLogical(ASTNode* node,
                                                  BinaryASTNode* binary,
                                                  int op);

// Contract: init / expr_init / braced_init; may re-parent, set `*finished`.
static ASTNode* DispatchExpressionInitializers(ASTNode* node,
                                               BinaryASTNode* binary,
                                               ASTNode** node_inout,
                                               bool* finished, int op);

// Contract: Compiler builtins (varargs, atomics, source location, etc.).
static ASTNode* DispatchExpressionBuiltinIntrinsics(ASTNode* node,
                                                    VectorASTNode* vector,
                                                    int op);

// Contract: Statement-expression, throw, and coroutine yield/await.
static ASTNode* DispatchExpressionCoroutineAndStatementExpr(
    ASTNode* node, UnaryASTNode* unary, VectorASTNode* vector,
    ASTNode** node_inout, bool* finished, int op);

// Contract: Final pass after dispatch when *finished is false. May replace *node
// via constant folding; may clear kASTAnalyzed on deferred-auto init and return
// without marking analyzed. Otherwise sets kASTAnalyzed and a default type.
static ASTNode* AnalyzeExpressionPostProcess(ASTNode* node);


// Re-open analysis for stale template-dependent is_same_v after prior passes set
// kASTAnalyzed; only runs on identifier pre-children visit.
static void ClearStaleIsSameVAnalysis(ASTNode* node, void* data, int child_id,
                                      VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode != kVisitPreChildren || node == NULL ||
      node->op != AST_OP(identifier)) {
    return;
  }
  IdentifierASTNode* id = (IdentifierASTNode*)node;
  if (id->symbol != NULL && StringEqual(&id->symbol->name, "is_same_v") &&
      id->template_arguments != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}

// Pre-dispatch: inequality rewrite (operands analyzed first) then built-in
// overload resolution for binary/unary operator names. Any replacement is returned
// to the caller without post-processing or kASTAnalyzed on this entry path.
static ASTNode* AnalyzeExpressionPreDispatch(ASTNode* node,
                                             BinaryASTNode* binary_node,
                                             UnaryASTNode* unary_node) {
  if (node->op == AST_OP(noteq)) {
    binary_node->left = AnalyzeExpression(binary_node->left);
    binary_node->right = AnalyzeExpression(binary_node->right);
    if (binary_node->left != NULL && binary_node->right != NULL &&
        RewrittenEqualityBeatsInequality(binary_node)) {
      ASTNode* rewritten = TryRewriteComparisonOperator(binary_node);
      if (rewritten != NULL) {
        return rewritten;
      }
    }
  }
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
  return NULL;
}

// Literals/requires_expr: types are established at parse time; no further work.
static ASTNode* DispatchExpressionLiterals(ASTNode* node) {
  (void)node;
  return node;
}

// Names: concept fold, variable templates, and contract views may replace root.
static ASTNode* DispatchExpressionIdentifier(ASTNode* node) {
  return AnalyzeIdentifier((IdentifierASTNode*)node);
}

// Arithmetic and bitwise binaries; mult/div/mod insert numeric promotions.
static ASTNode* DispatchExpressionBinaryArithmetic(ASTNode* node,
                                                   BinaryASTNode* binary,
                                                   int op) {
  switch (op) {
    case AST_OP(plus):
      return AnalyzePlusOperator(binary);
    case AST_OP(minus):
      return AnalyzeMinusOperator(binary);
    case AST_OP(mult):
    case AST_OP(div):
      AnalyzeBinaryExpression(binary);
      InsertNumericConversions(binary, true);
      return node;
    case AST_OP(mod):
      AnalyzeBinaryExpression(binary);
      InsertNumericConversions(binary, true);
      {
        TypeRecord* modulus_type =
            TypeIsVector(binary->left->type)
                ? TypeVectorElement(binary->left->type)
                : binary->left->type;
        if (!TypeIsIntegral(modulus_type)) {
          SemanticError(node, "Modulus operator needs an integral type");
        }
      }
      return node;
    case AST_OP(lshift):
    case AST_OP(rshift):
      return AnalyzeShift(binary);
    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
      return AnalyzeBitwiseOperator(binary);
    default:
      return node;
  }
}

// Comparisons: reflection operands use meta comparison; otherwise builtin/overload
// paths; three-way and conditional keep legacy side-effect order.
static ASTNode* DispatchExpressionComparisonsAndConditional(
    ASTNode* node, BinaryASTNode* binary, int op) {
  switch (op) {
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
      if ((binary->left != NULL &&
           (binary->left->op == AST_OP(reflect) ||
            binary->left->op == AST_OP(reflection_constant) ||
            binary->left->op == AST_OP(token_sequence_literal) ||
            TypeIsReflection(binary->left->type))) ||
          (binary->right != NULL &&
           (binary->right->op == AST_OP(reflect) ||
            binary->right->op == AST_OP(reflection_constant) ||
            binary->right->op == AST_OP(token_sequence_literal) ||
            TypeIsReflection(binary->right->type)))) {
        return SemanticAnalyzeReflectionComparison(binary);
      }
      return AnalyzeComparisonOperator(binary);
    case AST_OP(spaceship):
      return AnalyzeThreeWayComparison(binary);
    case AST_OP(question):
      AnalyzeConditionalExpression(binary);
      return node;
    default:
      return node;
  }
}

static ASTNode* DispatchExpressionAssignment(ASTNode* node,
                                             BinaryASTNode* binary) {
  return AnalyzeAssignmentExpression(binary);
}

static ASTNode* DispatchExpressionIncDec(ASTNode* node, UnaryASTNode* unary) {
  return AnalyzeIncDec(unary);
}

static void DispatchExpressionUnaryPlusMinus(UnaryASTNode* unary) {
  AnalyzeUnaryExpression(unary);
}

// Unevaluated operands: sizeof/alignof analyze type operands; noexcept analyzes
// its subexpression under dedicated depth rules in the callee.
static void DispatchExpressionUnevaluatedQueries(ASTNode* node,
                                                 UnaryASTNode* unary, int op) {
  switch (op) {
    case AST_OP(sizeof):
    case AST_OP(alignof):
      AnalyzeSizeofExpression((SizeofASTNode*)node);
      break;
    case AST_OP(noexcept_expr):
      AnalyzeNoexceptExpression(unary);
      break;
    default:
      break;
  }
}

static void DispatchExpressionTypeid(ASTNode* node, ASTNode** node_inout,
                                     bool* finished) {
  *node_inout = AnalyzeTypeidExpression((TypeidASTNode*)node);
  *finished = true;
}

// Reflection/splice nodes replace the root and finish without post-process folding.
static void DispatchExpressionReflectionAndSplice(ASTNode* node,
                                                ASTNode** node_inout,
                                                bool* finished, int op) {
  switch (op) {
    case AST_OP(reflect):
    case AST_OP(reflection_constant):
    case AST_OP(token_sequence_literal):
      *node_inout = SemanticAnalyzeReflection((ReflectionASTNode*)node);
      break;
    case AST_OP(splice):
      *node_inout = SemanticAnalyzeSplice((SpliceASTNode*)node);
      break;
    case AST_OP(splice_qualified):
      *node_inout =
          SemanticAnalyzeSpliceQualified((SpliceQualifiedASTNode*)node);
      break;
    default:
      break;
  }
  *finished = true;
}

// Explicit cast may re-root; compound literal analyzes initializer in place.
static ASTNode* DispatchExpressionCastAndCompound(ASTNode* node, int op) {
  switch (op) {
    case AST_OP(cast):
      return AnalyzeCastExpression((CastASTNode*)node);
    case AST_OP(compound_literal):
      AnalyzeCompoundLiteral((CompoundLiteralASTNode*)node);
      return node;
    default:
      return node;
  }
}

static void DispatchExpressionLogicalComplement(UnaryASTNode* unary, int op) {
  (void)op;
  AnalyzeUnaryExpression(unary);
}

// Address of splice may replace the whole unary node; otherwise & and * mutate
// the unary subtree and keep the same root.
static ASTNode* DispatchExpressionAddressAndDeref(
    ASTNode* node, UnaryASTNode* unary, ASTNode** node_inout, bool* finished,
    int op) {
  switch (op) {
    case AST_OP(address):
      if (unary->sub != NULL && unary->sub->op == AST_OP(splice)) {
        ASTNode* addressed = SemanticAnalyzeAddressedSplice(unary);
        if (addressed != node) {
          *node_inout = addressed;
          *finished = true;
          return addressed;
        }
      }
      AnalyzeAddressOperator(unary);
      return node;
    case AST_OP(contents):
      AnalyzeContentsOperator(unary);
      return node;
    default:
      return node;
  }
}

// Subscript picks vector vs binary shape; pack_index only analyzes index slots.
static ASTNode* DispatchExpressionSubscriptAndPack(
    ASTNode* node, BinaryASTNode* binary, VectorASTNode* vector, int op) {
  switch (op) {
    case AST_OP(subscript):
      return ASTNodeGetShape(node) == kASTShapeVector
                 ? AnalyzeMultidimensionalSubscript(vector)
                 : AnalyzeArraySubscript(binary);
    case AST_OP(pack_index):
      AnalyzePackIndexExpression(binary);
      return node;
    default:
      return node;
  }
}

// Calls run tuple std::get folds before/after AnalyzeFunctionCall; range-for
// nodes re-enter AnalyzeExpression on the resolved iterator expression.
static ASTNode* DispatchExpressionCallAndRange(ASTNode* node,
                                               VectorASTNode* vector,
                                               BinaryASTNode* binary, int op) {
  (void)binary;
  switch (op) {
    case AST_OP(call): {
      ASTNode* folded = CXXTryFoldStdGetFromTupleCall(vector);
      if (folded != NULL) {
        return folded;
      }
      node = AnalyzeFunctionCall(vector);
      folded = CXXTryFoldStdGetFromTupleCall((VectorASTNode*)node);
      if (folded != NULL) {
        return folded;
      }
      return node;
    }
    case AST_OP(range_begin):
    case AST_OP(range_end):
      return AnalyzeExpression(SyntaxResolveRangeForIterator(node));
    default:
      return node;
  }
}

// Member access may lower splices, replace with lookup result, or fold static
// data members; arrowstar tries member operator overload before pmr fallback.
static ASTNode* DispatchExpressionMemberAccess(ASTNode* node,
                                               BinaryASTNode* binary, int op) {
  switch (op) {
    case AST_OP(dot):
    case AST_OP(arrow):
      if (!SemanticLowerMemberSplice(binary)) {
        ASTNode* analyzed = AnalyzeMemberReference(binary);
        return analyzed != (ASTNode*)binary
                   ? analyzed
                   : FoldStaticDataMemberReference(binary);
      }
      return node;
    case AST_OP(dotstar):
      return AnalyzeMemberPointerReference(binary);
    case AST_OP(arrowstar): {
      ASTNode* overloaded =
          TryAnalyzeOverloadedBinaryOperatorWithAnalyzedOperands(binary);
      if (overloaded != NULL) {
        return overloaded;
      }
      return AnalyzeMemberPointerReference(binary);
    }
    default:
      return node;
  }
}

static void DispatchExpressionPointerToMember(UnaryASTNode* unary) {
  AnalyzePointerToMember(unary);
}

// Comma analyzes both sides, tries overload, materializes class prvalues, then
// copies the right operand's type/value category; &&/|| use short-circuit rules.
static ASTNode* DispatchExpressionCommaAndLogical(ASTNode* node,
                                                  BinaryASTNode* binary,
                                                  int op) {
  switch (op) {
    case AST_OP(comma):
      binary->left = AnalyzeExpression(binary->left);
      binary->right = AnalyzeExpression(binary->right);
      {
        ASTNode* overloaded = TryAnalyzeOverloadedBinaryOperator(binary);
        if (overloaded != NULL) {
          return overloaded;
        }
      }
      binary->left = SemanticMaterializeClassPrvalue(binary->left);
      ASTNodeSetType(node, binary->right->type);
      node->value_category = binary->right->value_category;
      return node;
    case AST_OP(logand):
    case AST_OP(logor):
      AnalyzeLogicalOperator(binary);
      return node;
    default:
      return node;
  }
}

// Initialization: may replace the init node in its parent; braced forms finish
// without post-process so folding waits for AST_OP(init).
static ASTNode* DispatchExpressionInitializers(ASTNode* node,
                                               BinaryASTNode* binary,
                                               ASTNode** node_inout,
                                               bool* finished, int op) {
  switch (op) {
    case AST_OP(init):
      if ((node->flags & kASTValueInitMemzero) != 0) {
        binary->left = AnalyzeExpression(binary->left);
        if (binary->left != NULL) {
          binary->left->flags |= kASTNeedAddress;
          if (node->type == NULL) {
            ASTNodeSetType(node, binary->left->type);
          }
        }
        node->flags |= kASTAnalyzed;
        return node;
      }
      {
        ASTNode* analyzed =
            AnalyzeInitialization(node, binary->left, binary->right);
        if (binary->right == NULL) {
          ASTNode* parent = node->parent;
          int child_id = node->child_id;
          if (parent != NULL) {
            ASTNodeReplaceChild(parent, child_id, analyzed, true);
          } else {
            ASTNodeDelete(node);
          }
          return analyzed;
        }
        binary->right = analyzed;
      }
      return node;
    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)node;
      expr_init->expr = AnalyzeExpression(expr_init->expr);
      if (expr_init->expr == NULL) {
        *node_inout = node;
        *finished = true;
        return node;
      }
      ASTNodeSetType(node, expr_init->expr->type);
      node->value_category = expr_init->expr->value_category;
      return node;
    }
    case AST_OP(braced_init):
    case AST_OP(designated_init):
      *node_inout = node;
      *finished = true;
      return node;
    default:
      return node;
  }
}

// Compiler builtins: analyze children then set types/validation per intrinsic kind.
static ASTNode* DispatchExpressionBuiltinIntrinsics(ASTNode* node,
                                                    VectorASTNode* vector,
                                                    int op) {
  switch (op) {
    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
      AnalyzeVarargsBuiltin1(vector);
      return node;
    case AST_OP(builtin_va_arg):
      AnalyzeVarargsBuiltin2(vector);
      return node;
    case AST_OP(builtin_atomic_load):
      AnalyzeAtomicLoadBuiltin(vector);
      return node;
    case AST_OP(builtin_atomic_store):
      AnalyzeAtomicStoreBuiltin(vector);
      return node;
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
      AnalyzeAtomicFetchBuiltin(vector, false);
      return node;
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
      AnalyzeAtomicFetchBuiltin(vector, true);
      return node;
    case AST_OP(builtin_atomic_compare_exchange_bool):
      AnalyzeAtomicCompareExchangeBuiltin(vector, false, true);
      return node;
    case AST_OP(builtin_atomic_compare_exchange_val):
      AnalyzeAtomicCompareExchangeBuiltin(vector, false, false);
      return node;
    case AST_OP(builtin_atomic_compare_exchange_n):
      AnalyzeAtomicCompareExchangeBuiltin(vector, true, true);
      return node;
    case AST_OP(builtin_atomic_fence):
      AnalyzeAtomicBuiltinChildren(vector);
      if (vector->children->length > 0) {
        ValidateAtomicMemoryOrder(vector->children->value.p[0],
                                  kAtomicOrderFence);
      }
      ASTNodeSetType(node, NewTypeRecordWithSize(kTypeVoid, kQualPlain));
      return node;
    case AST_OP(builtin_source_file):
    case AST_OP(builtin_source_function):
    case AST_OP(builtin_source_pretty_function):
      AnalyzeSourceStringBuiltin(vector);
      return node;
    case AST_OP(builtin_source_line):
    case AST_OP(builtin_source_column):
      AnalyzeSourceIntegerBuiltin(vector);
      return node;
    case AST_OP(builtin_expect):
      AnalyzeBuiltinExpect(vector);
      return node;
    case AST_OP(builtin_clz):
    case AST_OP(builtin_ctz):
    case AST_OP(builtin_popcount):
    case AST_OP(builtin_rotl):
    case AST_OP(builtin_rotr):
    case AST_OP(builtin_bswap):
      AnalyzeBuiltinBitOperation(vector);
      return node;
    case AST_OP(builtin_prefetch):
      AnalyzeBuiltinPrefetch(vector);
      return node;
    case AST_OP(builtin_start_lifetime):
      AnalyzeBuiltinStartLifetime(vector);
      return node;
    case AST_OP(builtin_observable_checkpoint):
      AnalyzeBuiltinObservableCheckpoint(vector);
      return node;
    case AST_OP(builtin_trap):
    case AST_OP(builtin_unreachable):
      AnalyzeBuiltinTerminator(vector);
      return node;
    case AST_OP(builtin_constant_p):
      AnalyzeBuiltinConstantP(vector);
      return node;
    case AST_OP(builtin_is_constant_evaluated):
      AnalyzeBuiltinIsConstantEvaluated(vector);
      return node;
    case AST_OP(builtin_alloca):
      AnalyzeBuiltinAlloca(vector);
      return node;
    default:
      return node;
  }
}

// Type-trait builtins finish without post-process; stmt-expr types from the last
// inner expression; throw and coroutine keywords delegate to coroutine modules.
static ASTNode* DispatchExpressionCoroutineAndStatementExpr(
    ASTNode* node, UnaryASTNode* unary, VectorASTNode* vector,
    ASTNode** node_inout, bool* finished, int op) {
  switch (op) {
    case AST_OP(builtin_type_trait):
      *node_inout = AnalyzeTypeTraitBuiltin(vector);
      *finished = true;
      return node;
    case AST_OP(stmt_expr): {
      AnalyzeStatement(unary->sub);
      CompoundStatementASTNode* comp =
          (CompoundStatementASTNode*)unary->sub;
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
      return node;
    }
    case AST_OP(throw):
      AnalyzeThrowExpression((ThrowASTNode*)node);
      return node;
    case AST_OP(co_await):
      AnalyzeCoAwaitExpression(unary);
      return node;
    case AST_OP(co_yield):
      AnalyzeCoYieldExpression(unary);
      return node;
    default:
      return node;
  }
}

// Monolith cases from literals through noexcept: arithmetic, comparisons,
// assignment, increment, and unevaluated queries.
static bool AnalyzeExpressionDispatchOpcodePhaseOperators(
    ASTNode** node_inout, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    int op) {
  ASTNode* node = *node_inout;
  switch (op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
    case AST_OP(fnumber):
    case AST_OP(string):
    case AST_OP(string_wide):
    case AST_OP(macro):
    case AST_OP(requires_expr):
      node = DispatchExpressionLiterals(node);
      break;

    case AST_OP(identifier):
      node = DispatchExpressionIdentifier(node);
      break;

    case AST_OP(plus):
    case AST_OP(minus):
    case AST_OP(mult):
    case AST_OP(div):
    case AST_OP(mod):
    case AST_OP(lshift):
    case AST_OP(rshift):
    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
      node = DispatchExpressionBinaryArithmetic(node, binary_node, op);
      break;

    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
    case AST_OP(spaceship):
    case AST_OP(question):
      node = DispatchExpressionComparisonsAndConditional(node, binary_node, op);
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
      node = DispatchExpressionAssignment(node, binary_node);
      break;

    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(postinc):
    case AST_OP(postdec):
      node = DispatchExpressionIncDec(node, unary_node);
      break;

    case AST_OP(uplus):
    case AST_OP(uminus):
      DispatchExpressionUnaryPlusMinus(unary_node);
      break;

    case AST_OP(sizeof):
    case AST_OP(alignof):
    case AST_OP(noexcept_expr):
      DispatchExpressionUnevaluatedQueries(node, unary_node, op);
      break;

    default:
      return false;
  }
  *node_inout = node;
  return true;
}

// Monolith cases from typeid through comma/logical: reflection early finishes,
// then cast, unary, subscript/call/member, and sequencing operators.
static bool AnalyzeExpressionDispatchOpcodePhaseTypeAndAccess(
    ASTNode** node_inout, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    VectorASTNode* vector_node, bool* finished, int op) {
  ASTNode* node = *node_inout;
  switch (op) {
    case AST_OP(typeid):
      DispatchExpressionTypeid(node, node_inout, finished);
      return true;

    case AST_OP(reflect):
    case AST_OP(reflection_constant):
    case AST_OP(token_sequence_literal):
    case AST_OP(splice):
    case AST_OP(splice_qualified):
      DispatchExpressionReflectionAndSplice(node, node_inout, finished, op);
      return true;

    case AST_OP(cast):
    case AST_OP(compound_literal):
      node = DispatchExpressionCastAndCompound(node, op);
      break;

    case AST_OP(not):
    case AST_OP(onescomp):
      DispatchExpressionLogicalComplement(unary_node, op);
      break;

    case AST_OP(address):
    case AST_OP(contents):
      node = DispatchExpressionAddressAndDeref(node, unary_node, node_inout,
                                               finished, op);
      if (*finished) {
        return true;
      }
      break;

    case AST_OP(subscript):
    case AST_OP(pack_index):
      node = DispatchExpressionSubscriptAndPack(node, binary_node, vector_node,
                                                op);
      break;

    case AST_OP(call):
    case AST_OP(range_begin):
    case AST_OP(range_end):
      node = DispatchExpressionCallAndRange(node, vector_node, binary_node, op);
      break;

    case AST_OP(dot):
    case AST_OP(arrow):
    case AST_OP(dotstar):
    case AST_OP(arrowstar):
      node = DispatchExpressionMemberAccess(node, binary_node, op);
      break;

    case AST_OP(member_ptr):
      DispatchExpressionPointerToMember(unary_node);
      break;

    case AST_OP(comma):
    case AST_OP(logand):
    case AST_OP(logor):
      node = DispatchExpressionCommaAndLogical(node, binary_node, op);
      break;

    default:
      return false;
  }
  *node_inout = node;
  return true;
}

// Monolith tail: initializers (with braced early finish), intrinsics, stmt-expr,
// coroutines, and type-trait builtins that finish without post-process.
static bool AnalyzeExpressionDispatchOpcodePhaseInitBuiltinsAndCoroutine(
    ASTNode** node_inout, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    VectorASTNode* vector_node, bool* finished, int op) {
  ASTNode* node = *node_inout;
  switch (op) {
    case AST_OP(init):
    case AST_OP(expr_init):
    case AST_OP(braced_init):
    case AST_OP(designated_init):
      node = DispatchExpressionInitializers(node, binary_node, node_inout,
                                            finished, op);
      if (*finished) {
        return true;
      }
      break;

    case AST_OP(builtin_va_start):
    case AST_OP(builtin_va_end):
    case AST_OP(builtin_va_copy):
    case AST_OP(builtin_va_arg):
    case AST_OP(builtin_atomic_load):
    case AST_OP(builtin_atomic_store):
    case AST_OP(builtin_atomic_fetch_add):
    case AST_OP(builtin_atomic_fetch_sub):
    case AST_OP(builtin_atomic_add_fetch):
    case AST_OP(builtin_atomic_sub_fetch):
    case AST_OP(builtin_atomic_compare_exchange_bool):
    case AST_OP(builtin_atomic_compare_exchange_val):
    case AST_OP(builtin_atomic_compare_exchange_n):
    case AST_OP(builtin_atomic_fence):
    case AST_OP(builtin_source_file):
    case AST_OP(builtin_source_function):
    case AST_OP(builtin_source_pretty_function):
    case AST_OP(builtin_source_line):
    case AST_OP(builtin_source_column):
    case AST_OP(builtin_expect):
    case AST_OP(builtin_clz):
    case AST_OP(builtin_ctz):
    case AST_OP(builtin_popcount):
    case AST_OP(builtin_rotl):
    case AST_OP(builtin_rotr):
    case AST_OP(builtin_bswap):
    case AST_OP(builtin_prefetch):
    case AST_OP(builtin_start_lifetime):
    case AST_OP(builtin_observable_checkpoint):
    case AST_OP(builtin_trap):
    case AST_OP(builtin_unreachable):
    case AST_OP(builtin_constant_p):
    case AST_OP(builtin_is_constant_evaluated):
    case AST_OP(builtin_alloca):
      node = DispatchExpressionBuiltinIntrinsics(node, vector_node, op);
      break;

    case AST_OP(builtin_type_trait):
    case AST_OP(stmt_expr):
    case AST_OP(throw):
    case AST_OP(co_await):
    case AST_OP(co_yield):
      node = DispatchExpressionCoroutineAndStatementExpr(
          node, unary_node, vector_node, node_inout, finished, op);
      if (*finished) {
        return true;
      }
      break;

    default:
      return false;
  }
  *node_inout = node;
  return true;
}

// Ordered phase selection mirrors the legacy monolith case sequence; each phase
// covers a contiguous opcode range and preserves early-finish behavior.
static void AnalyzeExpressionDispatchOpcode(
    ASTNode** node_inout, BinaryASTNode* binary_node, UnaryASTNode* unary_node,
    VectorASTNode* vector_node, bool* finished) {
  *finished = false;
  const int op = (*node_inout)->op;

  if (AnalyzeExpressionDispatchOpcodePhaseOperators(
          node_inout, binary_node, unary_node, op)) {
    return;
  }
  if (AnalyzeExpressionDispatchOpcodePhaseTypeAndAccess(
          node_inout, binary_node, unary_node, vector_node, finished, op)) {
    return;
  }
  (void)AnalyzeExpressionDispatchOpcodePhaseInitBuiltinsAndCoroutine(
      node_inout, binary_node, unary_node, vector_node, finished, op);
}

// Post-process: constant fold (respecting noexcept depth), defer kASTAnalyzed on
// auto init still pending deduction, then mark analyzed and apply default int type.
static ASTNode* AnalyzeExpressionPostProcess(ASTNode* node) {
  if (node == NULL) {
    return NULL;
  }

  bool fold_unevaluated_operator =
      node->op == AST_OP(noexcept_expr) || node->op == AST_OP(sizeof) ||
      node->op == AST_OP(alignof);
  ASTNode* folded =
      compiler->noexcept_operand_depth == 0 || fold_unevaluated_operator
          ? FoldConstantExpression(node)
          : NULL;
  if (folded != NULL) {
    return folded;
  }

  if (node->op == AST_OP(init)) {
    BinaryASTNode* init_node = (BinaryASTNode*)node;
    if (init_node->left != NULL && init_node->left->op == AST_OP(identifier)) {
      Symbol* init_symbol = ((IdentifierASTNode*)init_node->left)->symbol;
      if (init_symbol != NULL && TypeContainsAuto(init_symbol->type)) {
        ASTNodeVisit(node, ClearDeferredAutoInitAnalysis, 0, NULL);
        return node;
      }
    }
  }

  node->flags |= kASTAnalyzed;

  if (node->type == NULL) {
    ASTNodeSetType(node, NewTypeRecordWithSize(kTypeInt, kQualPlain));
  }
  return node;
}

// An identifier AST normally snapshots its symbol's type. A lambda body can be
// parsed before an init-capture or local `auto` declaration is deduced, leaving
// that snapshot as a placeholder even when the symbol is later concrete.
static void RefreshStaleIdentifierType(ASTNode* node) {
  if (node == NULL || node->op != AST_OP(identifier)) {
    return;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  if (symbol == NULL || symbol->type == NULL ||
      TypeContainsAuto(symbol->type) || TypeIsUnknown(symbol->type) ||
      TypeContainsTemplateParameter(symbol->type)) {
    return;
  }
  if (node->type == NULL || TypeContainsAuto(node->type) ||
      TypeIsUnknown(node->type) ||
      TypeContainsTemplateParameter(node->type)) {
    ASTNodeSetType(node, symbol->type);
    node->flags &= ~kASTAnalyzed;
  }
}

// Entry: stale is_same_v refresh, analyzed guard, pre-dispatch, category opcode
// dispatch, then post-process unless dispatch set finished (legacy in-switch return).
ASTNode* AnalyzeExpression(ASTNode* node) {
  RefreshStaleIdentifierType(node);
  if (node != NULL && (node->flags & kASTAnalyzed) != 0 &&
      node->op == AST_OP(identifier)) {
    ClearStaleIsSameVAnalysis(node, NULL, 0, kVisitPreChildren);
  }
  if (node == NULL || (node->flags & kASTAnalyzed) != 0) {
    return node;
  }

  BinaryASTNode* binary_node = (BinaryASTNode*)node;
  UnaryASTNode* unary_node = (UnaryASTNode*)node;
  VectorASTNode* vector_node = (VectorASTNode*)node;

  ASTNode* predispatch =
      AnalyzeExpressionPreDispatch(node, binary_node, unary_node);
  if (predispatch != NULL) {
    return predispatch;
  }

  bool finished = false;
  AnalyzeExpressionDispatchOpcode(&node, binary_node, unary_node, vector_node,
                                  &finished);
  if (finished) {
    return node;
  }

  return AnalyzeExpressionPostProcess(node);
}
