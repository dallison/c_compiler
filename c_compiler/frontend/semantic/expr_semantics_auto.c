//
//  expr_semantics_auto.c
//  c_compiler
//
//  Placeholder return deduction and deferred auto-member reanalysis.
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

static bool CXXCurrentFunctionCanEscalate(bool* deferred_template_pattern);
static bool SymbolTypeIsConcreteClass(TypeRecord* type);
// Contract: Mutates `func` (return type chain, deduction flags) and may analyze
// `func->body` or pull the definition from `symbol->value.func_defn`. Sets
// `current_function` / `current_class_access_context` for the duration of body
// analysis; restores them and `speculative_template_instantiation_depth`.
// Defers when the body is still a dependent template pattern or generic lambda
// `operator()`; may queue member definition materialization (not speculative).
void SemanticEnsureAutoReturnTypeDeduced(TypeRecord* func) {
  if (func == NULL || !TypeIsFunction(func) ||
      !TypeFunctionReturnContainsAuto(func)) {
    return;
  }
  Symbol* symbol = func->info.function.symbol;
  // A generic lambda's primary `operator()` body is shared by every deduced
  // specialization.  Analyzing it here (for example while type-checking a call
  // before the body is cloned) can lower `std::is_same_v` to a non-constant
  // lvalue load and break `if constexpr` in the cloned body.
  if (symbol != NULL && func->info.function.cxx_member_owner != NULL &&
      func->info.function.cxx_member_owner->tag_symbol != NULL &&
      func->info.function.cxx_member_owner->tag_symbol->flags.invented &&
      StringEqual(&symbol->name, "operator()") &&
      func->info.function.template_parameters.length > 0) {
    return;
  }
  Symbol* definition = symbol != NULL ? symbol->value.func_defn : NULL;
  TypeRecord* defn = definition != NULL ? definition->type : NULL;
  if (func->info.function.body == NULL && defn != NULL && defn != func &&
      TypeIsFunction(defn) && func->info.function.cxx_member_owner != NULL &&
      defn->info.function.cxx_member_owner !=
          func->info.function.cxx_member_owner) {
    // A class template specialization's member whose body was deferred
    // records the template pattern as its definition.  The pattern's return
    // statements are still dependent (`if constexpr` cannot discard either
    // branch), so the specialization's own body has to be cloned to deduce.
    // A speculative probe would only defer it again.
    int saved_speculative_depth =
        compiler->speculative_template_instantiation_depth;
    compiler->speculative_template_instantiation_depth = 0;
    TypeEnsureTemplateMemberFunctionDefinition(&compiler->syntax, symbol);
    compiler->speculative_template_instantiation_depth =
        saved_speculative_depth;
    if (func->info.function.body == NULL) {
      return;
    }
  }
  if (func->info.function.body == NULL) {
    // `auto Mask() const;` in a class and `auto G::Mask() const {...}` later:
    // the call resolves to the bodiless declaration, so deduce through the
    // definition and give the declaration the same return type.
    if (defn == NULL || defn == func || !TypeIsFunction(defn) ||
        defn->info.function.body == NULL) {
      return;
    }
    SemanticEnsureAutoReturnTypeDeduced(defn);
    if (TypeFunctionReturnContainsAuto(defn) || defn->next == NULL) {
      return;
    }
    TypeRecord* old_return = func->next;
    TypeRecordIncRef(defn->next);
    func->next = defn->next;
    func->info.function.is_auto_return_deduced = true;
    func->info.function.is_decltype_auto_return_deduced =
        defn->info.function.is_decltype_auto_return_deduced;
    TypeRecordDelete(old_return);
    return;
  }
  TypeRecord* saved_function = compiler->current_function;
  Struct* saved_class_access_context =
      compiler->current_class_access_context;
  compiler->current_function = func;
  // Deducing a member function's return type analyzes its body in the scope of
  // its owning class; without resetting the access context, a leaked context
  // from an enclosing scope (e.g. a lambda closure that triggered this
  // deduction) would wrongly deny the body access to its own private members.
  if (func->info.function.cxx_member_owner != NULL) {
    compiler->current_class_access_context =
        func->info.function.cxx_member_owner;
  }
  // A top-level deduction is permanent, so the body analysis is a real
  // instantiation and templates the body needs are queued.  A signature-only
  // probe (decltype, a type trait) already set a speculative depth; clearing
  // it here instantiated friend bodies such as Cord's AbslHashValue just to
  // form a return type.
  int saved_speculative_depth =
      compiler->speculative_template_instantiation_depth;
  AnalyzeStatement(func->info.function.body);
  compiler->current_function = saved_function;
  compiler->current_class_access_context = saved_class_access_context;
  StatementFinishAutoReturnDeduction(func, func->info.function.body);
  compiler->speculative_template_instantiation_depth = saved_speculative_depth;
}

// Contract: Read-only query of compiler immediate/consteval context depth flags.
bool CXXInImmediateFunctionContext(void) {
  return compiler->immediate_function_context_depth > 0 ||
         (CompilerCXXAtLeast(kLanguageStandardCXX23) &&
          compiler->constant_evaluation_required_depth > 0) ||
         (compiler->current_function != NULL &&
          TypeIsFunction(compiler->current_function) &&
          compiler->current_function->info.function.is_consteval);
}

// Contract: Read-only; sets `*deferred_template_pattern` when escalation must
// wait for template instantiation rather than mutating flags now.
static bool CXXCurrentFunctionCanEscalate(bool* deferred_template_pattern) {
  *deferred_template_pattern = false;
  if (!CompilerCXXAtLeast(kLanguageStandardCXX23) ||
      compiler->current_function == NULL ||
      !TypeIsFunction(compiler->current_function) ||
      compiler->current_function->info.function.is_consteval) {
    return false;
  }
  FunctionInfo* function = &compiler->current_function->info.function;
  Symbol* symbol = function->symbol;
  if (function->is_constexpr &&
      TypeContainsTemplateParameter(compiler->current_function)) {
    *deferred_template_pattern = function->is_constexpr;
    return *deferred_template_pattern;
  }
  bool lambda_call_operator =
      function->cxx_member_owner != NULL &&
      function->cxx_member_owner->tag_symbol != NULL &&
      function->cxx_member_owner->tag_symbol->flags.invented &&
      symbol != NULL && StringEqual(&symbol->name, "operator()");
  bool instantiated_constexpr = function->is_constexpr &&
      (function->template_origin != NULL ||
       (function->cxx_member_owner != NULL &&
        function->cxx_member_owner->tag_symbol != NULL &&
        function->cxx_member_owner->tag_symbol->type != NULL &&
        function->cxx_member_owner->tag_symbol->type->template_origin != NULL));
  return lambda_call_operator || function->is_defaulted ||
         instantiated_constexpr;
}

// Contract: When escalation applies, mutates `compiler->current_function` flags
// (`is_consteval`, `is_constexpr`, `is_inline`). Returns true without mutation
// when a dependent template pattern must defer immediacy to instantiation.
bool CXXEscalateCurrentFunction(void) {
  bool deferred_template_pattern;
  if (!CXXCurrentFunctionCanEscalate(&deferred_template_pattern)) {
    return false;
  }
  if (deferred_template_pattern) {
    return true;
  }
  FunctionInfo* function = &compiler->current_function->info.function;
  function->is_consteval = true;
  function->is_constexpr = true;
  function->is_inline = true;
  return true;
}

// P2564 makes immediacy a property of each concrete specialization.  A call
// site must know that property before the normal pending-instantiation pass, so
// analyze a newly selected constexpr specialization just far enough to type its
// body and discover an immediate-escalating expression.
// Contract: Temporarily sets function/access context and analyzes body once;
// restores depths and `functions_being_analyzed` membership.
void CXXAnalyzeImmediateEscalationCandidate(Symbol* symbol) {
  if (!CompilerCXXAtLeast(kLanguageStandardCXX23) || symbol == NULL ||
      symbol->type == NULL || !TypeIsFunction(symbol->type) ||
      !symbol->type->info.function.is_constexpr ||
      symbol->type->info.function.is_consteval ||
      symbol->type->info.function.body == NULL ||
      (symbol->type->info.function.body->flags & kASTAnalyzed) != 0) {
    return;
  }
  FunctionInfo* function = &symbol->type->info.function;
  bool instantiated_entity =
      function->template_origin != NULL ||
      (function->cxx_member_owner != NULL &&
       function->cxx_member_owner->tag_symbol != NULL &&
       function->cxx_member_owner->tag_symbol->type != NULL &&
       function->cxx_member_owner->tag_symbol->type->template_origin != NULL);
  if (!instantiated_entity) {
    return;
  }
  for (size_t i = 0; i < compiler->functions_being_analyzed.length; i++) {
    if (compiler->functions_being_analyzed.value.p[i] == symbol->type) {
      return;
    }
  }
  TypeRecord* saved_function = compiler->current_function;
  Struct* saved_access_context = compiler->current_class_access_context;
  int saved_immediate_depth = compiler->immediate_function_context_depth;
  int saved_constant_depth = compiler->constant_evaluation_required_depth;
  compiler->current_function = symbol->type;
  compiler->current_class_access_context =
      symbol->type->info.function.cxx_member_owner;
  compiler->immediate_function_context_depth = 0;
  compiler->constant_evaluation_required_depth = 0;
  VectorAppend(&compiler->functions_being_analyzed, symbol->type);
  AnalyzeStatement(symbol->type->info.function.body);
  StatementFinishAutoReturnDeduction(symbol->type, NULL);
  for (size_t i = compiler->functions_being_analyzed.length; i-- > 0;) {
    if (compiler->functions_being_analyzed.value.p[i] == symbol->type) {
      VectorDeleteElement(&compiler->functions_being_analyzed, i);
      break;
    }
  }
  compiler->immediate_function_context_depth = saved_immediate_depth;
  compiler->constant_evaluation_required_depth = saved_constant_depth;
  compiler->current_function = saved_function;
  compiler->current_class_access_context = saved_access_context;
}

// A member template's body is cloned as a pattern while its class is
// instantiated (`this->releaser_invoker = &Release;` in a constructor
// template).  Naming a function there is not an odr-use until the member
// template itself is instantiated.
// Contract: Read-only; true while `current_function` is a member template pattern.
bool AnalyzingTemplatePatternBody(void) {
  TypeRecord* enclosing = compiler->current_function;
  return enclosing != NULL && TypeIsFunction(enclosing) &&
         enclosing->info.function.symbol != NULL &&
         enclosing->info.function.symbol->flags.is_template;
}

// Contract: Read-only; false while type still contains `auto` or is unknown.
static bool SymbolTypeIsConcreteClass(TypeRecord* type) {
  if (type == NULL || TypeContainsAuto(type) || TypeIsUnknown(type)) {
    return false;
  }
  if (TypeIsReference(type)) {
    type = type->next;
  }
  return TypeIsStructOrUnion(type);
}

// Returns true for raw operator opcodes that may need C++ overload resolution
// after an operand's placeholder type becomes a concrete class.
static bool AutoDeductionMayChangeOperator(ASTOpcode op) {
  switch (op) {
    case AST_OP(postinc):
    case AST_OP(postdec):
    case AST_OP(uminus):
    case AST_OP(uplus):
    case AST_OP(contents):
    case AST_OP(not):
    case AST_OP(onescomp):
    case AST_OP(preinc):
    case AST_OP(predec):
    case AST_OP(plus):
    case AST_OP(minus):
    case AST_OP(mult):
    case AST_OP(div):
    case AST_OP(mod):
    case AST_OP(lshift):
    case AST_OP(rshift):
    case AST_OP(rshiftl):
    case AST_OP(rshifta):
    case AST_OP(and):
    case AST_OP(bitor):
    case AST_OP(exor):
    case AST_OP(less):
    case AST_OP(lesseq):
    case AST_OP(greater):
    case AST_OP(greatereq):
    case AST_OP(equal):
    case AST_OP(noteq):
    case AST_OP(spaceship):
    case AST_OP(pluseq):
    case AST_OP(minuseq):
    case AST_OP(multeq):
    case AST_OP(diveq):
    case AST_OP(percenteq):
    case AST_OP(lshifteq):
    case AST_OP(rshifteq):
    case AST_OP(rshifteql):
    case AST_OP(rshifteqa):
    case AST_OP(andeq):
    case AST_OP(oreq):
    case AST_OP(exoreq):
    case AST_OP(logand):
    case AST_OP(logor):
    case AST_OP(dotstar):
    case AST_OP(arrowstar):
      return true;
    default:
      return false;
  }
}

// Returns true when an expression operand now has a complete, non-dependent
// class type that participates in overloaded-operator lookup.
static bool AutoDeductionOperandIsConcreteClass(ASTNode* operand) {
  if (operand == NULL || operand->type == NULL ||
      TypeContainsAuto(operand->type) || TypeIsUnknown(operand->type) ||
      TypeContainsTemplateParameter(operand->type)) {
    return false;
  }
  TypeRecord* type = operand->type;
  while (TypeIsReference(type)) {
    type = type->next;
  }
  return TypeIsStructOrUnion(type);
}

// A range-for binding (`const auto& it = *begin`) can be used in the loop
// body before deduction replaces `auto`. Member accesses and operators in
// later statements can therefore retain placeholder semantics. Refresh
// identifier types post-order, then collect expressions that need reanalysis.
// Contract: Visitor; mutates stale identifier types and appends expressions to
// the vector in `data`.
static void CollectStaleAutoExpressions(ASTNode* node, void* data,
                                        int child_id, VisitorMode mode) {
  (void)child_id;
  if (mode != kVisitPostChildren || node == NULL) {
    return;
  }
  if (node->op == AST_OP(identifier)) {
    Symbol* symbol = ((IdentifierASTNode*)node)->symbol;
    if (symbol != NULL && symbol->type != NULL &&
        !TypeContainsAuto(symbol->type) && !TypeIsUnknown(symbol->type) &&
        !TypeContainsTemplateParameter(symbol->type) &&
        (node->type == NULL || TypeContainsAuto(node->type) ||
         TypeIsUnknown(node->type) ||
         TypeContainsTemplateParameter(node->type))) {
      ASTNodeSetType(node, symbol->type);
      node->flags &= ~kASTAnalyzed;
    }
    return;
  }
  if ((node->flags & kASTAnalyzed) == 0) {
    return;
  }
  if (node->op == AST_OP(dot) || node->op == AST_OP(arrow)) {
    BinaryASTNode* access = (BinaryASTNode*)node;
    if ((node->type == NULL || TypeContainsAuto(node->type) ||
         TypeIsUnknown(node->type)) &&
        access->left != NULL &&
        access->left->op == AST_OP(identifier)) {
      Symbol* symbol = ((IdentifierASTNode*)access->left)->symbol;
      if (symbol != NULL && SymbolTypeIsConcreteClass(symbol->type)) {
        VectorAppend((Vector*)data, node);
      }
    }
    return;
  }
  if (!AutoDeductionMayChangeOperator(node->op)) {
    return;
  }
  ASTNodeShape shape = ASTNodeGetShape(node);
  bool has_class_operand = false;
  if (shape == kASTShapeUnary) {
    has_class_operand = AutoDeductionOperandIsConcreteClass(
        ((UnaryASTNode*)node)->sub);
  } else if (shape == kASTShapeBinary) {
    BinaryASTNode* binary = (BinaryASTNode*)node;
    has_class_operand =
        AutoDeductionOperandIsConcreteClass(binary->left) ||
        AutoDeductionOperandIsConcreteClass(binary->right);
  }
  if (has_class_operand) {
    VectorAppend((Vector*)data, node);
  }
}

// Contract: Refreshes stale expressions under `current_function->body`, re-runs
// `AnalyzeExpression`, and may replace child slots via `ASTNodeReplaceChild`
// (old nodes retained when replaced). Re-entrant guard (`depth`); no-op outside
// a function with a body.
void SemanticReanalyzeAfterAutoDeduction(void) {
  static int depth = 0;
  if (depth > 0 || compiler->current_function == NULL ||
      !TypeIsFunction(compiler->current_function) ||
      compiler->current_function->info.function.body == NULL) {
    return;
  }
  Vector stale;
  VectorInit(&stale);
  ASTNodeVisit(compiler->current_function->info.function.body,
               CollectStaleAutoExpressions, 0, &stale);
  depth++;
  for (size_t i = 0; i < stale.length; i++) {
    ASTNode* node = stale.value.p[i];
    node->flags &= ~kASTAnalyzed;
    ASTNodeClearType(node);
    ASTNode* analyzed = AnalyzeExpression(node);
    if (analyzed != node && node->parent != NULL) {
      ASTNodeReplaceChild(node->parent, node->child_id, analyzed,
                          /*delete_old_child=*/false);
    }
  }
  depth--;
  VectorDestruct(&stale);
}

// Contract: Visitor pre-hook; clears `kASTAnalyzed` on each visited node so a
// deferred auto init can be re-analyzed. Does not mutate types or parent links.
void ClearDeferredAutoInitAnalysis(ASTNode* node, void* data,
                                          int child_id, VisitorMode mode) {
  (void)data;
  (void)child_id;
  if (mode == kVisitPreChildren && node != NULL) {
    node->flags &= ~kASTAnalyzed;
  }
}
