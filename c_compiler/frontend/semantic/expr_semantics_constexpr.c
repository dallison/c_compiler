//
//  expr_semantics_constexpr.c
//  c_compiler
//
//  Constant-expression folding and static-address evaluation.
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

// Tracks functions already inspected while looking for consteval-sensitive use.
typedef struct {
  Vector visited_functions;
  bool found;
} ConstevalContextQuery;

static bool FunctionMayObserveConstantEvaluation( Symbol* symbol, ConstevalContextQuery* query);
static void FindConstevalContextUse(ASTNode* node, void* data, int child_id, VisitorMode mode);
static bool ExpressionMayObserveConstantEvaluation(ASTNode* node);
static void FindUnboundAutomatic(ASTNode* node, void* data, int child_id, VisitorMode mode);
static bool ExpressionHasUnboundAutomatic(ASTNode* node);
static bool BitFieldReadAwaitsPromotion(ASTNode* node);
static ASTNode* FoldConstantExpressionValue(ASTNode* node);
static bool StaticSubscriptIndexInBounds(BinaryASTNode* subscript, bool one_past_end);
static Symbol* StaticAddressTargetFromSubobject(ASTNode* expression, bool one_past_end);
static Symbol* StaticAddressTargetFromExpression(ASTNode* expression);

// Contract: Read-only AST walk; may append to `query->visited_functions`.
// Sets `query->found` when the function body may observe constant evaluation.
static bool FunctionMayObserveConstantEvaluation(
    Symbol* symbol, ConstevalContextQuery* query) {
  if (symbol == NULL || symbol->type == NULL ||
      !TypeIsFunction(symbol->type) ||
      symbol->type->info.function.is_consteval) {
    return false;
  }
  Symbol* definition =
      symbol->value.func_defn != NULL ? symbol->value.func_defn : symbol;
  if (definition == NULL || definition->type == NULL ||
      !TypeIsFunction(definition->type) ||
      definition->type->info.function.body == NULL) {
    return false;
  }
  for (size_t i = 0; i < query->visited_functions.length; i++) {
    if (query->visited_functions.value.p[i] == definition) {
      return false;
    }
  }
  VectorAppend(&query->visited_functions, definition);
  ASTNodeVisit(definition->type->info.function.body, FindConstevalContextUse, 0,
               query);
  return query->found;
}

// Contract: Visitor; sets `query->found`, may recurse into called function bodies.
static void FindConstevalContextUse(ASTNode* node, void* data, int child_id,
                                    VisitorMode mode) {
  (void)child_id;
  ConstevalContextQuery* query = data;
  if (mode != kVisitPreChildren || query->found || node == NULL) {
    return;
  }
  if (node->op == AST_OP(if) &&
      ((IfStatementASTNode*)node)->is_consteval) {
    query->found = true;
    return;
  }
  if (node->op == AST_OP(builtin_is_constant_evaluated)) {
    query->found = true;
    return;
  }
  if (node->op != AST_OP(call)) {
    return;
  }
  VectorASTNode* call = (VectorASTNode*)node;
  if (call->left == NULL || call->left->op != AST_OP(identifier)) {
    return;
  }
  FunctionMayObserveConstantEvaluation(
      ((IdentifierASTNode*)call->left)->symbol, query);
}

// Contract: Read-only subtree scan; does not mutate the AST.
static bool ExpressionMayObserveConstantEvaluation(ASTNode* node) {
  ConstevalContextQuery query;
  VectorInit(&query.visited_functions);
  query.found = false;
  ASTNodeVisit(node, FindConstevalContextUse, 0, &query);
  VectorDestruct(&query.visited_functions);
  return query.found;
}

static bool RuntimeFoldingMayObserveConstantEvaluation(ASTNode* node) {
  return CompilerCXXAtLeast(kLanguageStandardCXX20) &&
         compiler->constant_evaluation_required_depth == 0 &&
         ExpressionMayObserveConstantEvaluation(node);
}

bool SemanticEvaluateRuntimeTruth(ASTNode* node, int64_t* value) {
  return !RuntimeFoldingMayObserveConstantEvaluation(node) &&
         EvaluateTruthExpression(node, value);
}

bool SemanticEvaluateRuntimeInteger(ASTNode* node, int64_t* value) {
  return !RuntimeFoldingMayObserveConstantEvaluation(node) &&
         EvaluateIntegerExpression(node, value);
}

typedef struct {
  bool found;
} UnboundAutomaticFinder;

// Contract: Visitor; sets `finder->found` when an unbound automatic/temp is seen.
static void FindUnboundAutomatic(ASTNode* node, void* data, int child_id,
                                 VisitorMode mode) {
  (void)child_id;
  UnboundAutomaticFinder* finder = data;
  if (mode != kVisitPreChildren || finder->found ||
      node->op != AST_OP(identifier)) {
    return;
  }
  Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
  // A scalar compiler temporary is equally unbound: folding `tmp = n` to `n`
  // would drop the store that later reads of `tmp` depend on.
  if (symbol != NULL &&
      (symbol->flags.is_argument ||
       (symbol->flags.is_temp && symbol->type != NULL &&
        TypeIsScalar(symbol->type)) ||
       (symbol->flags.is_local &&
        !StorageIs(symbol->storage, STO(static) | STO(thread)) &&
        !symbol->flags.is_constexpr))) {
    finder->found = true;
  }
}

// Contract: Read-only scan via `FindUnboundAutomatic`.
static bool ExpressionHasUnboundAutomatic(ASTNode* node) {
  UnboundAutomaticFinder finder = {false};
  ASTNodeVisit(node, FindUnboundAutomatic, 0, &finder);
  return finder.found;
}

// A bit-field read whose parent operator will promote it to a different type.
// Folding it first would leave a constant of the field's declared type, and
// the promotion depends on seeing the field; the parent folds instead.
// Contract: Read-only; defer folding when parent will promote the bit-field read.
static bool BitFieldReadAwaitsPromotion(ASTNode* node) {
  if (node->parent == NULL) {
    return false;
  }
  switch (node->parent->op) {
    case AST_OP(uminus):
    case AST_OP(uplus):
    case AST_OP(onescomp):
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
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
      break;
    default:
      return false;
  }
  TypeRecord* promoted = BitFieldPromotionType(node);
  return promoted != NULL && !TypeEqual(promoted, node->type);
}

// Attempt to fold a constant expression by evaluating it and if
// successful, replacing it with a constant AST node with the value.
// Contract: Returns replacement constant or NULL without mutating when folding
// is unsafe (dependent, unbound automatic, consteval observation, address use).
ASTNode* FoldConstantExpression(ASTNode* node) {
  // A value-dependent expression cannot be evaluated until its template
  // arguments are known. Folding it now would freeze placeholder properties
  // such as the provisional size of a dependent type into the template body.
  if (CompilerIsCXX() && ExpressionIsTemplateDependent(node)) {
    return NULL;
  }
  if (compiler->current_function != NULL &&
      compiler->current_function->info.function.prototype.length != 0) {
    // A function body is analyzed before any invocation binds its formals.
    // Even a subtree that does not retain a reliable parent link to an
    // argument identifier must not be frozen from placeholder argument data.
    return NULL;
  }
  // Function parameters and ordinary automatic variables have no value until
  // the function executes. Speculatively interpreting such a subtree during
  // semantic analysis would freeze a placeholder value into the function
  // body, making every later invocation observe that value.
  if (ExpressionHasUnboundAutomatic(node)) {
    return NULL;
  }
  // Speculative runtime folding is not a manifestly constant-evaluated
  // context. Keep calls that can reach `if consteval` or
  // std::is_constant_evaluated() intact so target codegen selects their
  // runtime arms. Constant-required consumers (static_assert, constexpr
  // initialization, template arguments) evaluate the retained tree
  // explicitly and therefore still select the constant-evaluation arms.
  if (RuntimeFoldingMayObserveConstantEvaluation(node)) {
    return NULL;
  }
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
  // `&g[1]` designates the element itself, not its value.
  if ((node->flags & kASTNeedAddress) != 0 ||
      (node->parent != NULL && node->parent->op == AST_OP(address))) {
    return NULL;
  }
  if (BitFieldReadAwaitsPromotion(node)) {
    return NULL;
  }
  if (node == compiler->decltype_operand) {
    return NULL;
  }

  bool speculative = compiler->constant_evaluation_required_depth == 0;
  if (speculative) {
    compiler->speculative_constant_fold_depth++;
  }
  ASTNode* folded = FoldConstantExpressionValue(node);
  if (speculative) {
    compiler->speculative_constant_fold_depth--;
  }
  return folded;
}

// Contract: On success replaces `node` in its parent via `ASTNodeReplaceChild`
// (deletes `node`); returns the new constant or NULL. Requires `node->parent`.
static ASTNode* FoldConstantExpressionValue(ASTNode* node) {
  // Try to fold integer and floating point constant expressions.
  if (TypeIsInt128(node->type)) {
    int64_t lo = 0;
    int64_t hi = 0;
    if (EvaluateInt128Constant((ASTNode*)node, &lo, &hi)) {
      ASTNode* const_node =
          NewInt128ConstantASTNode(lo, hi, node->type, node->location);
      ASTNodeReplaceChild(node->parent, node->child_id, const_node, true);
      const_node->flags |= kASTAnalyzed;
      return const_node;
    }
  } else if (TypeIsIntegral(node->type)) {
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

/* `a[i]` of a fixed-size array designates an element only for a constant `i`
 * in bounds; `&a[N]` is the one-past-the-end address. */
// Contract: Read-only bounds check for static address extraction.
static bool StaticSubscriptIndexInBounds(BinaryASTNode* subscript,
                                         bool one_past_end) {
  TypeRecord* array_type =
      subscript->left != NULL ? subscript->left->type : NULL;
  if (array_type == NULL || !TypeIsFixedArray(array_type)) {
    return true;
  }
  int64_t index = 0;
  if (!EvaluateIntegerExpression(subscript->right, &index)) {
    return false;
  }
  int64_t length = array_type->info.array.size.fixed;
  return index >= 0 && (one_past_end ? index <= length : index < length);
}

// Contract: Recursive static-address target through casts/subscripts; read-only.
static Symbol* StaticAddressTargetFromSubobject(ASTNode* expression,
                                                bool one_past_end) {
  if (expression == NULL) {
    return NULL;
  }
  if (expression->op == AST_OP(cast)) {
    return StaticAddressTargetFromSubobject(((CastASTNode*)expression)->expr,
                                            one_past_end);
  }
  if (expression->op == AST_OP(subscript) &&
      ASTNodeGetShape(expression) == kASTShapeBinary) {
    BinaryASTNode* subscript = (BinaryASTNode*)expression;
    if (!StaticSubscriptIndexInBounds(subscript, one_past_end)) {
      return NULL;
    }
    return StaticAddressTargetFromSubobject(subscript->left, false);
  }
  return StaticAddressTargetFromExpression(expression);
}

// Contract: Resolves identifier/subscript to static storage symbol; read-only.
static Symbol* StaticAddressTargetFromExpression(ASTNode* expression) {
  if (expression == NULL) {
    return NULL;
  }
  if (expression->op == AST_OP(cast)) {
    return StaticAddressTargetFromExpression(
        ((CastASTNode*)expression)->expr);
  }
  if (expression->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)expression)->symbol;
    if (symbol != NULL &&
        (TypeIsFunction(symbol->type) ||
         StorageIs(symbol->storage, STO(static) | STO(extern)) ||
         CompilerSymbolIsMetaPromotedStatic(symbol))) {
      return symbol;
    }
    return NULL;
  }
  if (expression->op == AST_OP(subscript) &&
      ASTNodeGetShape(expression) == kASTShapeBinary) {
    return StaticAddressTargetFromSubobject(expression, false);
  }
  return NULL;
}

// Contract: Analyzes `initializer`, may mutate `symbol` (`value_set`,
// `value.other`, `constexpr_initializer` clone). May replace nodes under
// `expression` when pointer folding succeeds. Does not own `initializer`.
bool SemanticEvaluatePointerConstantForSymbol(Symbol* symbol,
                                              ASTNode* initializer) {
  if (symbol == NULL || initializer == NULL || symbol->type == NULL ||
      !TypeIsPointer(symbol->type)) {
    return false;
  }
  ASTNode* expression = ConstexprInitializerExpression(initializer);
  if (expression == NULL) {
    return false;
  }
  if (expression->op == AST_OP(call)) {
    ASTNode* synthesized =
        SemanticTryAnalyzeMetaSynthesisCall((VectorASTNode*)expression);
    if (synthesized != NULL) {
      expression = synthesized;
    }
  }
  expression = AnalyzeExpression(expression);
  if (expression == NULL) {
    return false;
  }
  if (expression->op == AST_OP(expr_init)) {
    expression = ((ExpressionInitializerASTNode*)expression)->expr;
    expression = AnalyzeExpression(expression);
    if (expression == NULL) {
      return false;
    }
  }
  if (expression->op == AST_OP(call)) {
    ASTNode* synthesized =
        SemanticTryAnalyzeMetaSynthesisCall((VectorASTNode*)expression);
    if (synthesized != NULL) {
      expression = AnalyzeExpression(synthesized);
    }
  }
  if (expression->op == AST_OP(cast)) {
    expression = AnalyzeExpression(((CastASTNode*)expression)->expr);
  }
  if (expression->op == AST_OP(string) ||
      expression->op == AST_OP(string_wide)) {
    ASTNode* clone = ASTNodeClone(expression, IdentityCloneNode, NULL, NULL);
    ASTNodeDelete(SymbolExtraOf(symbol)->constexpr_initializer);
    SymbolMutableExtra(symbol)->constexpr_initializer = clone;
    symbol->flags.value_set =
        SymbolExtraOf(symbol)->constexpr_initializer != NULL;
    return symbol->flags.value_set;
  }
  // `value.other` names only the object; the initializer keeps the exact
  // address (`&table[3]`) for the constant evaluator.
  Symbol* target = StaticAddressTargetFromExpression(expression);
  if (target == NULL && expression->op == AST_OP(address)) {
    target = StaticAddressTargetFromSubobject(
        ((UnaryASTNode*)expression)->sub, true);
  }
  if (target != NULL) {
    if (CompilerSymbolIsMetaPromotedStatic(target)) {
      ConstexprEnsureMetaPromotedStaticObject(target);
    }
    symbol->value.other = target;
    symbol->flags.value_set = true;
    ASTNode* clone = ASTNodeClone(expression, IdentityCloneNode, NULL, NULL);
    ASTNodeDelete(SymbolExtraOf(symbol)->constexpr_initializer);
    SymbolMutableExtra(symbol)->constexpr_initializer = clone;
    return true;
  }
  ASTNode* folded = ConstexprFoldPointerExpression(expression);
  ASTNode* folded_address =
      folded != NULL && folded->op == AST_OP(cast) &&
              folded->type != NULL && TypeIsPointer(folded->type)
          ? ((CastASTNode*)folded)->expr
          : folded;
  if (folded_address != NULL &&
      (folded_address->op == AST_OP(string) ||
       folded_address->op == AST_OP(string_wide) ||
       folded_address->op == AST_OP(address) ||
       folded_address->op == AST_OP(identifier) ||
       folded_address->op == AST_OP(plus))) {
    ASTNodeDelete(SymbolExtraOf(symbol)->constexpr_initializer);
    SymbolMutableExtra(symbol)->constexpr_initializer =
        ASTNodeClone(folded, IdentityCloneNode, NULL, NULL);
    symbol->flags.value_set =
        SymbolExtraOf(symbol)->constexpr_initializer != NULL;
    if (expression->parent != NULL) {
      ASTNodeReplaceChild(expression->parent, expression->child_id, folded,
                          true);
    } else {
      ASTNodeDelete(folded);
    }
    return symbol->flags.value_set;
  }
  ASTNodeDelete(folded);
  return false;
}

// Contract: When applicable, sets `symbol` constexpr-reference metadata and
// clones `initializer` into `symbol->constexpr_initializer`; read-only on AST
// aside from deleting any prior constexpr initializer on `symbol`.
bool MarkCXX26SymbolicConstexprReference(Symbol* symbol,
                                                ASTNode* initializer) {
  if (CompilerCXXAtLeast(kLanguageStandardCXX26) && symbol != NULL &&
      symbol->flags.is_local &&
      !StorageIs(symbol->storage, STO(static) | STO(thread)) &&
      symbol->flags.is_constexpr &&
      (TypeIsReference(symbol->type) || TypeIsPointer(symbol->type))) {
    ASTNode* expression = ConstexprInitializerExpression(initializer);
    while (expression != NULL &&
           (expression->op == AST_OP(cast) ||
            expression->op == AST_OP(address) ||
            expression->op == AST_OP(contents))) {
      expression = expression->op == AST_OP(cast)
                       ? ((CastASTNode*)expression)->expr
                       : ((UnaryASTNode*)expression)->sub;
    }
    while (expression != NULL &&
           (expression->op == AST_OP(dot) ||
            expression->op == AST_OP(arrow) ||
            expression->op == AST_OP(subscript))) {
      expression = ((BinaryASTNode*)expression)->left;
    }
    if (expression != NULL && expression->op == AST_OP(identifier)) {
      Symbol* target = ((IdentifierASTNode*)expression)->symbol;
      bool target_referenceable =
          target != NULL && target->flags.is_local &&
          !StorageIs(target->storage,
                     STO(static) | STO(thread));
      if (target_referenceable) {
        symbol->is_constexpr_representable = true;
        SymbolMutableExtra(symbol)->constexpr_reference_scope =
            compiler->current_function;
        ASTNodeDelete(SymbolExtraOf(symbol)->constexpr_initializer);
        SymbolMutableExtra(symbol)->constexpr_initializer =
            ASTNodeClone(initializer, IdentityCloneNode, NULL, NULL);
        return true;
      }
    }
  }
  return false;
}

// Contract: Dispatches constexpr evaluation for `symbol`; may analyze
// subexpressions and set `symbol->value` / flags. Order: reflection, CXX26 ref,
// reference/scalar/pointer helpers, then object evaluation (skips optional
// cache for non-constexpr automatics).
bool EvaluateConstantForSymbol(Symbol* symbol, ASTNode* initializer) {
  if (symbol != NULL &&
      ASTNodeAny(initializer, ExpressionRequiresASTConstexpr, NULL)) {
    symbol->requires_ast_constexpr = true;
  }
  if (symbol != NULL && TypeIsReflection(symbol->type)) {
    ASTNode* expression = ConstexprInitializerExpression(initializer);
    expression = AnalyzeExpression(expression);
    ReflectionValue* value =
        SemanticReflectionValueFromExpression(expression);
    if (value != NULL) {
      symbol->value.other = value;
      symbol->flags.value_set = true;
      return true;
    }
    return false;
  }
  if (MarkCXX26SymbolicConstexprReference(symbol, initializer)) {
    return true;
  }
  if (ConstexprEvaluateReferenceConstantForSymbol(symbol, initializer)) {
    return true;
  }
  if (EvaluateScalarConstantForSymbol(symbol, initializer)) {
    return true;
  }
  if (SemanticEvaluatePointerConstantForSymbol(symbol, initializer)) {
    return true;
  }
  // Speculatively caching an ordinary automatic const class object is optional.
  // Its initializer may refer to other automatic objects whose runtime values
  // are not bound in the P-code constexpr thunk.  Treating that partial result
  // as a constant freezes zero-filled members into later expressions.
  if (symbol->flags.is_local &&
      !StorageIs(symbol->storage, STO(static) | STO(thread)) &&
      !symbol->flags.is_constexpr && !symbol->flags.is_constinit) {
    return false;
  }
  return ConstexprEvaluateObjectConstantForSymbol(symbol, initializer);
}

// Contract: Runs `AnalyzeExpression` on `node` first, then classifies
// constant-ness from the analyzed shape; may mutate the AST during analysis.
bool IsConstantExpression(ASTNode* node) {
  node = AnalyzeExpression(node);
  switch (node->op) {
    case AST_OP(number):
    case AST_OP(charconst):
    case AST_OP(charwide):
    case AST_OP(fnumber):
    case AST_OP(string):
    case AST_OP(string_wide):
    case AST_OP(reflection_constant):
      return true;
    case AST_OP(identifier): {
      // Static identifiers that are arrays are constant.
      IdentifierASTNode* id_node = (IdentifierASTNode*)node;
      if (TypeIsIntConstant(id_node->base.type) ||
          TypeIsFloatingPointConstant(id_node->base.type) ||
          (TypeIsReflection(id_node->base.type) &&
           id_node->symbol != NULL && id_node->symbol->flags.value_set)) {
        return true;
      }
      // Functions are constant expressions.
      if (TypeIsFunction(id_node->base.type)) {
        return true;
      }
      if (id_node->symbol != NULL &&
          CompilerSymbolIsMetaPromotedStatic(id_node->symbol)) {
        return true;
      }
      if (id_node->symbol != NULL &&
          CompilerMetaPromotedPointerTarget(id_node->symbol) != NULL) {
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
    case AST_OP(member_ptr): {
      MemberPointerValue value;
      return CompilerIsCXX() && node->type != NULL &&
             MemberPointerTryEvaluateConstant(node, node->type, &value);
    }
    case AST_OP(cast): {
      CastASTNode* c = (CastASTNode*)node;
      return IsConstantExpression(c->expr);
    }
    case AST_OP(expr_init): {
      ExpressionInitializerASTNode* expr_init =
          (ExpressionInitializerASTNode*)node;
      return expr_init->expr != NULL && IsConstantExpression(expr_init->expr);
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
